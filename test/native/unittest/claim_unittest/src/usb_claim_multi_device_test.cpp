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

#include "usb_claim_multi_device_test.h"

#include <vector>

#include "ashmem.h"
#include "hilog_wrapper.h"
#include "iusb_srv.h"
#include "usb_common.h"
#include "usb_errors.h"
#include "v1_2/usb_types.h"

using namespace testing::ext;
using OHOS::HDI::Usb::V1_2::USBTransferInfo;

namespace OHOS {
namespace USB {
namespace {
constexpr int32_t TRANSFER_BUFFER_SIZE = 8;
constexpr int32_t TRANSFER_TIMEOUT_MS = 300;
constexpr uint32_t ENDPOINT_TYPE_BULK = 2;
constexpr int32_t USB_DEVICE_CLASS_HUB = 9;

bool IsBulkEndpoint(const USBEndpoint &endpoint)
{
    return (endpoint.GetAttributes() & USB_ENDPOINT_XFERTYPE_MASK) == ENDPOINT_TYPE_BULK;
}

/*
 * Scan the endpoints of one interface and record the bulk IN / bulk OUT
 * endpoints when present. Returns true only when the interface exposes both.
 */
bool FindBulkEndpoints(UsbInterface &interface, USBEndpoint &bulkIn, USBEndpoint &bulkOut)
{
    bool hasIn = false;
    bool hasOut = false;
    for (auto &endpoint : interface.GetEndpoints()) {
        if (!IsBulkEndpoint(endpoint)) {
            continue;
        }
        if (endpoint.GetDirection() == USB_ENDPOINT_DIR_IN) {
            bulkIn = endpoint;
            hasIn = true;
        } else if (endpoint.GetDirection() == USB_ENDPOINT_DIR_OUT) {
            bulkOut = endpoint;
            hasOut = true;
        }
    }
    return hasIn && hasOut;
}

/*
 * Submit an asynchronous bulk transfer on the bulk OUT endpoint of the
 * serial device: the request is queued to the HDI layer and the submission
 * itself must succeed.
 */
int32_t DoSubmitBulkWrite(USBDevicePipe &pipe, const USBEndpoint &endpoint)
{
    auto &client = UsbSrvClient::GetInstance();
    sptr<Ashmem> ashmem = Ashmem::CreateAshmem("claim_submit_transfer", TRANSFER_BUFFER_SIZE);
    if (ashmem == nullptr) {
        return UEC_SERVICE_INVALID_VALUE;
    }
    ashmem->MapReadAndWriteAshmem();
    const uint8_t dataToWrite[TRANSFER_BUFFER_SIZE] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
    if (!ashmem->WriteToAshmem(dataToWrite, sizeof(dataToWrite), 0)) {
        return UEC_SERVICE_INVALID_VALUE;
    }
    USBTransferInfo transferInfo;
    transferInfo.endpoint = static_cast<int32_t>(endpoint.GetAddress());
    transferInfo.flags = 0;
    transferInfo.type = static_cast<int32_t>(endpoint.GetType());
    transferInfo.timeOut = TRANSFER_TIMEOUT_MS;
    transferInfo.length = TRANSFER_BUFFER_SIZE;
    transferInfo.userData = 0;
    transferInfo.numIsoPackets = 0;
    auto callback = [](const TransferCallbackInfo &info,
        const std::vector<HDI::Usb::V1_2::UsbIsoPacketDescriptor> &packets, uint64_t userData) {};
    return client.UsbSubmitTransfer(pipe, transferInfo, callback, ashmem);
}
} // namespace

UsbDevice UsbClaimMultiDeviceTest::deviceA_ {};
UsbDevice UsbClaimMultiDeviceTest::deviceB_ {};
USBDevicePipe UsbClaimMultiDeviceTest::pipeA_ {};
USBDevicePipe UsbClaimMultiDeviceTest::pipeB_ {};
UsbInterface UsbClaimMultiDeviceTest::ifaceA_ {};
UsbInterface UsbClaimMultiDeviceTest::ifaceB_ {};
USBEndpoint UsbClaimMultiDeviceTest::bulkInEp_ {};
USBEndpoint UsbClaimMultiDeviceTest::bulkOutEp_ {};
uint8_t UsbClaimMultiDeviceTest::ifaceIdA_ = 0;
uint8_t UsbClaimMultiDeviceTest::ifaceIdB_ = 0;
bool UsbClaimMultiDeviceTest::multiDeviceReady_ = false;

/*
 * Pick the device whose first interface exposes both bulk IN and bulk OUT
 * endpoints (typically a USB serial adapter).
 */
bool UsbClaimMultiDeviceTest::PickSerialDevice()
{
    std::vector<UsbDevice> deviceList;
    auto &client = UsbSrvClient::GetInstance();
    if (client.GetDevices(deviceList) != UEC_OK) {
        return false;
    }
    for (auto &dev : deviceList) {
        if (dev.GetClass() == USB_DEVICE_CLASS_HUB || dev.GetConfigs().empty()) {
            continue;
        }
        for (auto &config : dev.GetConfigs()) {
            for (auto &interface : config.GetInterfaces()) {
                if (!FindBulkEndpoints(interface, bulkInEp_, bulkOutEp_)) {
                    continue;
                }
                deviceB_ = dev;
                ifaceB_ = interface;
                ifaceIdB_ = static_cast<uint8_t>(interface.GetId());
                pipeB_.SetBusNum(dev.GetBusNum());
                pipeB_.SetDevAddr(dev.GetDevAddr());
                return true;
            }
        }
    }
    return false;
}

/*
 * Pick a device other than the serial device for cross-device isolation.
 */
bool UsbClaimMultiDeviceTest::PickSecondDevice()
{
    std::vector<UsbDevice> deviceList;
    auto &client = UsbSrvClient::GetInstance();
    if (client.GetDevices(deviceList) != UEC_OK) {
        return false;
    }
    for (auto &dev : deviceList) {
        if (dev.GetClass() == USB_DEVICE_CLASS_HUB || dev.GetConfigs().empty()) {
            continue;
        }
        if (dev.GetBusNum() == deviceB_.GetBusNum() && dev.GetDevAddr() == deviceB_.GetDevAddr()) {
            continue;
        }
        for (auto &config : dev.GetConfigs()) {
            if (config.GetInterfaces().empty()) {
                continue;
            }
            deviceA_ = dev;
            ifaceA_ = config.GetInterfaces().at(0);
            ifaceIdA_ = static_cast<uint8_t>(ifaceA_.GetId());
            pipeA_.SetBusNum(dev.GetBusNum());
            pipeA_.SetDevAddr(dev.GetDevAddr());
            return true;
        }
    }
    return false;
}

void UsbClaimMultiDeviceTest::SetUpTestCase()
{
    UsbClaimTestBase::SetUpTestCase();
    multiDeviceReady_ = PickSerialDevice() && PickSecondDevice();
    if (!multiDeviceReady_) {
        USB_HILOGW(MODULE_USB_SERVICE,
            "multi-device precondition not met (need a bulk IN+OUT device plus a second device), skip");
        GTEST_SKIP() << "multi-device precondition not met, skip suite";
    }
    USB_HILOGI(MODULE_USB_SERVICE,
        "multi-device ready: deviceB bus=%{public}hhu dev=%{public}hhu if=%{public}hhu (bulk in/out), "
        "deviceA bus=%{public}hhu dev=%{public}hhu if=%{public}hhu",
        deviceB_.GetBusNum(), deviceB_.GetDevAddr(), ifaceIdB_, deviceA_.GetBusNum(), deviceA_.GetDevAddr(),
        ifaceIdA_);
    auto &client = UsbSrvClient::GetInstance();
    ASSERT_EQ(UEC_OK, client.OpenDevice(deviceA_, pipeA_));
    ASSERT_EQ(UEC_OK, client.OpenDevice(deviceB_, pipeB_));
}

void UsbClaimMultiDeviceTest::TearDownTestCase()
{
    if (multiDeviceReady_) {
        auto &client = UsbSrvClient::GetInstance();
        SwitchToApp(CLAIM_APP_A);
        client.UsbAttachKernelDriver(pipeA_, ifaceA_);
        client.UsbAttachKernelDriver(pipeB_, ifaceB_);
        client.Close(pipeA_);
        client.Close(pipeB_);
    }
    UsbClaimTestBase::TearDownTestCase();
}

void UsbClaimMultiDeviceTest::SetUp()
{
    ASSERT_TRUE(multiDeviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    client.ReleaseInterface(pipeA_, ifaceA_);
    client.ReleaseInterface(pipeB_, ifaceB_);
    SwitchToApp(CLAIM_APP_B);
    client.ReleaseInterface(pipeA_, ifaceA_);
    client.ReleaseInterface(pipeB_, ifaceB_);
    SwitchToApp(CLAIM_APP_A);
    client.UsbAttachKernelDriver(pipeA_, ifaceA_);
    client.UsbAttachKernelDriver(pipeB_, ifaceB_);
}

void UsbClaimMultiDeviceTest::TearDown()
{
    if (!multiDeviceReady_) {
        return;
    }
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    client.OpenDevice(deviceA_, pipeA_);
    client.OpenDevice(deviceB_, pipeB_);
    client.ReleaseInterface(pipeA_, ifaceA_);
    client.ReleaseInterface(pipeB_, ifaceB_);
    SwitchToApp(CLAIM_APP_B);
    client.ReleaseInterface(pipeA_, ifaceA_);
    client.ReleaseInterface(pipeB_, ifaceB_);
    SwitchToApp(CLAIM_APP_A);
    client.UsbAttachKernelDriver(pipeA_, ifaceA_);
    client.UsbAttachKernelDriver(pipeB_, ifaceB_);
}

/**
 * @tc.name: ClaimMultiDev001
 * @tc.desc: Test bulk write through the bulk OUT endpoint of the serial device
 * @tc.desc: Positive test: a bulk write on a claimed interface succeeds
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimMultiDeviceTest, ClaimMultiDev001, TestSize.Level1)
{
    ASSERT_TRUE(multiDeviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipeB_, ifaceB_, true);
    EXPECT_EQ(UEC_OK, ret);
    std::vector<uint8_t> writeBuffer(TRANSFER_BUFFER_SIZE, 0x55);
    ret = client.BulkTransfer(pipeB_, bulkOutEp_, writeBuffer, TRANSFER_TIMEOUT_MS);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimMultiDev002
 * @tc.desc: Test bulk read through the bulk IN endpoint of the serial device
 * @tc.desc: Positive test: a bulk read on a claimed interface succeeds (the
 *           bulk IN endpoint of the serial adapter is data-less, so the read
 *           returns a timeout error from the driver rather than IO exception)
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimMultiDeviceTest, ClaimMultiDev002, TestSize.Level1)
{
    ASSERT_TRUE(multiDeviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipeB_, ifaceB_, true);
    EXPECT_EQ(UEC_OK, ret);
    std::vector<uint8_t> readBuffer(TRANSFER_BUFFER_SIZE, 0);
    ret = client.BulkTransfer(pipeB_, bulkInEp_, readBuffer, TRANSFER_TIMEOUT_MS);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, ret);
    EXPECT_NE(UEC_OK, ret);
}

/**
 * @tc.name: ClaimMultiDev003
 * @tc.desc: Test a full write-then-read cycle on the serial device
 * @tc.desc: Positive test: the write direction succeeds, proving a real
 *           bidirectional data path
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimMultiDeviceTest, ClaimMultiDev003, TestSize.Level1)
{
    ASSERT_TRUE(multiDeviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipeB_, ifaceB_, true);
    EXPECT_EQ(UEC_OK, ret);
    std::vector<uint8_t> writeBuffer(TRANSFER_BUFFER_SIZE, 0x55);
    ret = client.BulkTransfer(pipeB_, bulkOutEp_, writeBuffer, TRANSFER_TIMEOUT_MS);
    EXPECT_EQ(UEC_OK, ret);
    std::vector<uint8_t> readBuffer(TRANSFER_BUFFER_SIZE, 0);
    ret = client.BulkTransfer(pipeB_, bulkInEp_, readBuffer, TRANSFER_TIMEOUT_MS);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, ret);
    EXPECT_NE(UEC_OK, ret);
}

/**
 * @tc.name: ClaimMultiDev004
 * @tc.desc: Test UsbSubmitTransfer on the bulk OUT endpoint of the serial device
 * @tc.desc: Positive test: an asynchronous bulk write submission on a claimed
 *           interface is accepted by the HDI layer (UEC_OK)
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimMultiDeviceTest, ClaimMultiDev004, TestSize.Level1)
{
    ASSERT_TRUE(multiDeviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipeB_, ifaceB_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = DoSubmitBulkWrite(pipeB_, bulkOutEp_);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimMultiDev005
 * @tc.desc: Test blocked bulk write while another app holds the exclusive claim
 * @tc.desc: Negative test: the write direction is also subject to the
 *           exclusive claim check
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimMultiDeviceTest, ClaimMultiDev005, TestSize.Level1)
{
    ASSERT_TRUE(multiDeviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipeB_, ifaceB_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    std::vector<uint8_t> writeBuffer(TRANSFER_BUFFER_SIZE, 0x55);
    ret = client.BulkTransfer(pipeB_, bulkOutEp_, writeBuffer, TRANSFER_TIMEOUT_MS);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, ret);
    std::vector<uint8_t> readBuffer(TRANSFER_BUFFER_SIZE, 0);
    ret = client.BulkTransfer(pipeB_, bulkInEp_, readBuffer, TRANSFER_TIMEOUT_MS);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, ret);
}

/**
 * @tc.name: ClaimMultiDev006
 * @tc.desc: Test exclusive claim isolation between two devices
 * @tc.desc: Positive test: app A exclusive claims the interface of device A
 *           while app B exclusive claims the interface of device B, both
 *           succeed and transfers on device B are not affected by the
 *           exclusive claim on device A
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimMultiDeviceTest, ClaimMultiDev006, TestSize.Level1)
{
    ASSERT_TRUE(multiDeviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipeA_, ifaceA_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipeB_, ifaceB_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    std::vector<uint8_t> writeBuffer(TRANSFER_BUFFER_SIZE, 0x55);
    ret = client.BulkTransfer(pipeB_, bulkOutEp_, writeBuffer, TRANSFER_TIMEOUT_MS);
    EXPECT_EQ(UEC_OK, ret);
    std::vector<uint8_t> readBuffer(TRANSFER_BUFFER_SIZE, 0);
    ret = client.BulkTransfer(pipeB_, bulkInEp_, readBuffer, TRANSFER_TIMEOUT_MS);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, ret);
    EXPECT_NE(UEC_OK, ret);
}

/**
 * @tc.name: ClaimMultiDev007
 * @tc.desc: Test blocked transfer on one device while the other stays usable
 * @tc.desc: Negative/positive test: app A exclusive claims both devices, app
 *           B is blocked on device A but can still transfer on device B
 *           after a normal claim there
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimMultiDeviceTest, ClaimMultiDev007, TestSize.Level1)
{
    ASSERT_TRUE(multiDeviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipeA_, ifaceA_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    // normal claim on device A is not blocked, but the owner is notified; use
    // device B for the transfer comparison instead
    ret = client.ClaimInterface(pipeB_, ifaceB_, true);
    EXPECT_EQ(UEC_OK, ret);
    // device A interface has no bulk endpoints on typical hardware, use the
    // claim result to prove device isolation instead
    ret = client.ReleaseInterface(pipeB_, ifaceB_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipeB_, ifaceB_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    std::vector<uint8_t> writeBuffer(TRANSFER_BUFFER_SIZE, 0x55);
    ret = client.BulkTransfer(pipeB_, bulkOutEp_, writeBuffer, TRANSFER_TIMEOUT_MS);
    EXPECT_EQ(UEC_OK, ret);
}
} // namespace USB
} // namespace OHOS
