/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ohusb_manager_capi_race_test.h"

#include <chrono>
#include <map>
#include <thread>

#include "hilog_wrapper.h"
#include "ohusb_manager.h"
#include "usb_common_test.h"
#include "usb_errors.h"
#include "usb_srv_client.h"

using namespace testing::ext;
using namespace OHOS::USB;
using namespace OHOS::USB::Common;

namespace OHOS {
namespace USB {
namespace CapiTest {

namespace {
constexpr int32_t RACE_CYCLES = 5;
constexpr int32_t RACE_BURST = 20;
constexpr int32_t RECOVER_TIMEOUT_MS = 10000;
constexpr int32_t RECOVER_POLL_INTERVAL_MS = 100;
constexpr uint8_t USB_HUB_CLASS = 0x09;

constexpr OH_UsbManager_ErrorCode ERR_SUCCESS = OH_USBMANAGER_SUCCESS;
constexpr OH_UsbManager_ErrorCode ERR_PERM = OH_USBMANAGER_ERROR_PERMISSION_DENIED;
constexpr OH_UsbManager_ErrorCode ERR_SERVICE = OH_USBMANAGER_ERROR_SERVICE_EXCEPTION;
constexpr OH_UsbManager_ErrorCode ERR_NO_DEVICE = OH_USBMANAGER_ERROR_NO_DEVICE;
constexpr OH_UsbManager_ErrorCode ERR_IO = OH_USBMANAGER_ERROR_IO_ERROR;

struct CodeTally {
    std::map<OH_UsbManager_ErrorCode, int32_t> counts;

    void Add(OH_UsbManager_ErrorCode code)
    {
        counts[code]++;
    }

    int32_t Get(OH_UsbManager_ErrorCode code) const
    {
        auto it = counts.find(code);
        return (it == counts.end()) ? 0 : it->second;
    }
};

bool IsValidErrCode(OH_UsbManager_ErrorCode code)
{
    switch (code) {
        case OH_USBMANAGER_SUCCESS:
        case OH_USBMANAGER_ERROR_PERMISSION_DENIED:
        case OH_USBMANAGER_ERROR_SERVICE_EXCEPTION:
        case OH_USBMANAGER_ERROR_NO_DEVICE:
        case OH_USBMANAGER_ERROR_NO_MEMORY:
        case OH_USBMANAGER_ERROR_IO_ERROR:
        case OH_USBMANAGER_ERROR_INVALID_PARAMETER:
            return true;
        default:
            return false;
    }
}

bool GetFirstNonHubDevice(OH_UsbManager_UsbDevice &outDevice)
{
    OH_UsbManager_UsbDevice *devices = nullptr;
    uint32_t count = 0;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(&devices, &count);
    if (ret != ERR_SUCCESS || devices == nullptr || count == 0) {
        OH_UsbManager_FreeUsbDeviceList(devices, count);
        return false;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (devices[i].clazz != USB_HUB_CLASS) {
            outDevice = devices[i];
            OH_UsbManager_FreeUsbDeviceList(devices, count);
            return true;
        }
    }
    OH_UsbManager_FreeUsbDeviceList(devices, count);
    return false;
}

// After a global disable/enable cycle the device re-enumerates and its bus/dev address may
// change, so search for it by vendor/product id instead of bus/dev address.
bool FindDeviceByVidPid(int32_t vendorId, int32_t productId, OH_UsbManager_UsbDevice &outDevice)
{
    OH_UsbManager_UsbDevice *devices = nullptr;
    uint32_t count = 0;
    bool found = false;
    if (OH_UsbManager_GetUsbDeviceList(&devices, &count) != ERR_SUCCESS) {
        OH_UsbManager_FreeUsbDeviceList(devices, count);
        return false;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (devices[i].clazz != USB_HUB_CLASS && devices[i].vendorId == vendorId &&
            devices[i].productId == productId) {
            outDevice = devices[i];
            found = true;
            break;
        }
    }
    OH_UsbManager_FreeUsbDeviceList(devices, count);
    return found;
}

bool WaitForDeviceByVidPid(int32_t vendorId, int32_t productId, int32_t timeoutMs,
    OH_UsbManager_UsbDevice &outDevice)
{
    int32_t waited = 0;
    while (waited < timeoutMs) {
        if (FindDeviceByVidPid(vendorId, productId, outDevice)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(RECOVER_POLL_INTERVAL_MS));
        waited += RECOVER_POLL_INTERVAL_MS;
    }
    return false;
}

// One burst of ConnectDevice calls on the given device, tallying the returned codes.
void ConnectDeviceBurst(const OH_UsbManager_UsbDevice &device, int32_t count, CodeTally &tally)
{
    OH_UsbManager_UsbPipe pipe = {};
    for (int32_t i = 0; i < count; ++i) {
        OH_UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
        tally.Add(ret);
        if (ret == ERR_SUCCESS) {
            OH_UsbManager_ClosePipe(&pipe);
        }
    }
}

// One burst of GetFileDescriptor calls on the given pipe, tallying the returned codes.
void GetFileDescriptorBurst(const OH_UsbManager_UsbPipe &pipe, int32_t count, CodeTally &tally)
{
    for (int32_t i = 0; i < count; ++i) {
        int32_t fd = -1;
        tally.Add(OH_UsbManager_GetFileDescriptor(&pipe, &fd));
    }
}

// After a global disable/enable cycle the device re-enumerates. Wait for it, then re-open the
// pipe and refresh the device info. Returns false when the device does not come back or the
// pipe cannot be re-opened.
bool ReconnectAfterGlobalEnable(int32_t vendorId, int32_t productId, OH_UsbManager_UsbDevice &device,
    OH_UsbManager_UsbPipe &pipe)
{
    OH_UsbManager_UsbDevice refreshed = {};
    if (!WaitForDeviceByVidPid(vendorId, productId, RECOVER_TIMEOUT_MS, refreshed)) {
        return false;
    }
    OH_UsbManager_ClosePipe(&pipe);
    if (OH_UsbManager_ConnectDevice(&refreshed, &pipe) != ERR_SUCCESS) {
        return false;
    }
    int32_t fd = -1;
    if (OH_UsbManager_GetFileDescriptor(&pipe, &fd) != ERR_SUCCESS) {
        return false;
    }
    device = refreshed;
    return true;
}

void LogTally(const std::string &tag, const CodeTally &tally)
{
    int32_t total = 0;
    for (auto &entry : tally.counts) {
        total += entry.second;
    }
    USB_HILOGI(MODULE_USB_INNERKIT,
        "%{public}s result tally: success=%{public}d perm=%{public}d service=%{public}d noDevice=%{public}d "
        "io=%{public}d total=%{public}d",
        tag.c_str(), tally.Get(ERR_SUCCESS), tally.Get(ERR_PERM), tally.Get(ERR_SERVICE),
        tally.Get(ERR_NO_DEVICE), tally.Get(ERR_IO), total);
    if (tally.Get(ERR_IO) > 0 || tally.Get(ERR_NO_DEVICE) > 0) {
        GTEST_LOG_(INFO) << tag << ": driver-layer error conversion observed, io=" << tally.Get(ERR_IO)
                         << " noDevice=" << tally.Get(ERR_NO_DEVICE);
    }
}
} // namespace

void OHUsbManagerRaceTest::SetUpTestCase(void)
{
    UsbCommonTest::GrantPermissionSysNative();
    USB_HILOGI(MODULE_USB_INNERKIT, "OHUsbManagerRaceTest::SetUpTestCase");
}

void OHUsbManagerRaceTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "OHUsbManagerRaceTest::TearDownTestCase");
}

void OHUsbManagerRaceTest::SetUp(void) {}

void OHUsbManagerRaceTest::TearDown(void) {}

/**
 * @tc.name: GetFileDescriptorWithoutConnect001
 * @tc.desc: GetFileDescriptor on an existing device with permission but no open pipe. In
 *           pass-through mode the driver layer fails in FindHandleByDev and returns HDF_FAILURE
 *           (-1) (in HAL mode HDF_DEV_ERR_NO_DEVICE -202); the CAPI layer converts both to
 *           NO_DEVICE.
 * @tc.type: FUNC
 */
HWTEST_F(OHUsbManagerRaceTest, GetFileDescriptorWithoutConnect001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "GetFileDescriptorWithoutConnect001 start");
    OH_UsbManager_UsbDevice device = {};
    if (!GetFirstNonHubDevice(device)) {
        GTEST_SKIP() << "No USB device, skip";
    }
    OH_UsbManager_UsbPipe pipe = {};
    pipe.busNum = device.busNum;
    pipe.devAddress = device.devAddress;
    int32_t fd = -1;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_GetFileDescriptor(&pipe, &fd);
    EXPECT_EQ(ret, ERR_NO_DEVICE);
}

/**
 * @tc.name: ConnectDeviceGlobalDisableRace001
 * @tc.desc: Loop ConnectDevice while all USB interfaces are globally disabled and re-enabled
 *           via UsbSrvClient::ManageGlobalInterface (kernel hub deauthorize/authorize, which
 *           removes and re-enumerates downstream devices like unplug/replug). A parallel caller
 *           thread races with the removal to hit the driver-layer error window. Records the CAPI
 *           error code distribution to observe whether driver-layer HDF errors (-1/-17/-202)
 *           surface as OH_USBMANAGER_ERROR_IO_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(OHUsbManagerRaceTest, ConnectDeviceGlobalDisableRace001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConnectDeviceGlobalDisableRace001 start");
    OH_UsbManager_UsbDevice device = {};
    if (!GetFirstNonHubDevice(device)) {
        GTEST_SKIP() << "No USB device, skip";
    }
    int32_t vendorId = device.vendorId;
    int32_t productId = device.productId;

    OH_UsbManager_UsbPipe pipe = {};
    ASSERT_EQ(OH_UsbManager_ConnectDevice(&device, &pipe), ERR_SUCCESS);
    OH_UsbManager_ClosePipe(&pipe);

    CodeTally tally;
    for (int32_t cycle = 0; cycle < RACE_CYCLES; ++cycle) {
        ConnectDeviceBurst(device, RACE_BURST, tally);
        int32_t mRet = UEC_OK;
        std::thread racer([&device, &tally]() { ConnectDeviceBurst(device, RACE_BURST, tally); });
        mRet = UsbSrvClient::GetInstance().ManageGlobalInterface(true);
        racer.join();
        if (mRet != UEC_OK) {
            GTEST_SKIP() << "ManageGlobalInterface not permitted, ret=" << mRet << ", skip race test";
        }
        ConnectDeviceBurst(device, RACE_BURST, tally);
        mRet = UsbSrvClient::GetInstance().ManageGlobalInterface(false);
        EXPECT_EQ(mRet, UEC_OK);
        EXPECT_TRUE(WaitForDeviceByVidPid(vendorId, productId, RECOVER_TIMEOUT_MS, device));
    }

    for (auto &entry : tally.counts) {
        EXPECT_TRUE(IsValidErrCode(entry.first));
    }
    OH_UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    EXPECT_EQ(ret, ERR_SUCCESS);
    OH_UsbManager_ClosePipe(&pipe);
    LogTally("ConnectDeviceGlobalDisableRace001", tally);
}

/**
 * @tc.name: GetFileDescriptorGlobalDisableRace001
 * @tc.desc: Loop GetFileDescriptor on an opened pipe while all USB interfaces are globally
 *           disabled and re-enabled via UsbSrvClient::ManageGlobalInterface (kernel hub
 *           deauthorize/authorize, which removes and re-enumerates downstream devices like
 *           unplug/replug). The pipe goes stale while the device is gone, and a parallel caller
 *           thread races with the removal to hit the driver-layer error window. Records the CAPI
 *           error code distribution to observe whether the driver-layer HDF_DEV_ERR_NO_DEVICE
 *           (-202) / HDF_FAILURE (-1) surfaces as OH_USBMANAGER_ERROR_NO_DEVICE.
 * @tc.type: FUNC
 */
HWTEST_F(OHUsbManagerRaceTest, GetFileDescriptorGlobalDisableRace001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "GetFileDescriptorGlobalDisableRace001 start");
    OH_UsbManager_UsbDevice device = {};
    if (!GetFirstNonHubDevice(device)) {
        GTEST_SKIP() << "No USB device, skip";
    }
    int32_t vendorId = device.vendorId;
    int32_t productId = device.productId;

    OH_UsbManager_UsbPipe pipe = {};
    ASSERT_EQ(OH_UsbManager_ConnectDevice(&device, &pipe), ERR_SUCCESS);

    CodeTally tally;
    for (int32_t cycle = 0; cycle < RACE_CYCLES; ++cycle) {
        GetFileDescriptorBurst(pipe, RACE_BURST, tally);
        int32_t mRet = UEC_OK;
        std::thread racer([&pipe, &tally]() { GetFileDescriptorBurst(pipe, RACE_BURST, tally); });
        mRet = UsbSrvClient::GetInstance().ManageGlobalInterface(true);
        racer.join();
        if (mRet != UEC_OK) {
            OH_UsbManager_ClosePipe(&pipe);
            GTEST_SKIP() << "ManageGlobalInterface not permitted, ret=" << mRet << ", skip race test";
        }
        GetFileDescriptorBurst(pipe, RACE_BURST, tally);
        mRet = UsbSrvClient::GetInstance().ManageGlobalInterface(false);
        EXPECT_EQ(mRet, UEC_OK);
        if (!ReconnectAfterGlobalEnable(vendorId, productId, device, pipe)) {
            ADD_FAILURE() << "device not back or pipe reopen failed, cycle=" << cycle;
            break;
        }
    }
    OH_UsbManager_ClosePipe(&pipe);

    for (auto &entry : tally.counts) {
        EXPECT_TRUE(IsValidErrCode(entry.first));
    }
    OH_UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    EXPECT_EQ(ret, ERR_SUCCESS);
    int32_t fd = -1;
    EXPECT_EQ(OH_UsbManager_GetFileDescriptor(&pipe, &fd), ERR_SUCCESS);
    OH_UsbManager_ClosePipe(&pipe);
    LogTally("GetFileDescriptorGlobalDisableRace001", tally);
}
} // CapiTest
} // USB
} // OHOS
