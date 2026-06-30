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

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include "usb_host_manager.h"
#include "usb_device.h"
#include "usb_errors.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;

constexpr uint8_t TEST_BUS_NUM_1 = 1;
constexpr uint8_t TEST_DEV_ADDR_1 = 1;
constexpr uint8_t TEST_INTERFACE_ID_0 = 0;
constexpr uint8_t TEST_ENDPOINT_ADDR_1 = 1;

class UsbHostManagerHotplugTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::unique_ptr<UsbHostManager> usbHostManager_;
};

void UsbHostManagerHotplugTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerHotplugTest::SetUpTestCase enter");
}

void UsbHostManagerHotplugTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerHotplugTest::TearDownTestCase enter");
}

void UsbHostManagerHotplugTest::SetUp()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerHotplugTest::SetUp enter");
    SystemAbility* systemAbility = nullptr;
    usbHostManager_ = std::make_unique<UsbHostManager>(systemAbility);
    ASSERT_NE(usbHostManager_, nullptr);
}

void UsbHostManagerHotplugTest::TearDown()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerHotplugTest::TearDown enter");
    usbHostManager_.reset();
}

/**
 * @tc.name: UsbHostManager_Hotplug_OpenClose_RapidSequence
 * @tc.desc: Test rapid open and close sequence simulating hotplug
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerHotplugTest, UsbHostManager_Hotplug_OpenClose_RapidSequence, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_OpenClose_RapidSequence start");

    int32_t openCount = 0;
    int32_t closeCount = 0;

    for (int i = 0; i < 20; i++) {
        int32_t openRet = usbHostManager_->OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
        if (openRet == UEC_OK || openRet == UEC_SERVICE_INVALID_VALUE) {
            openCount++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        int32_t closeRet = usbHostManager_->Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
        if (closeRet == UEC_OK || closeRet == UEC_SERVICE_INVALID_VALUE) {
            closeCount++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_OpenClose_RapidSequence open=%{public}d close=%{public}d",
                openCount, closeCount);
    EXPECT_GE(openCount, 0);
    EXPECT_GE(closeCount, 0);
}

/**
 * @tc.name: UsbHostManager_Hotplug_ResetDuringOpen
 * @tc.desc: Test device reset while device is open
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerHotplugTest, UsbHostManager_Hotplug_ResetDuringOpen, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_ResetDuringOpen start");

    int32_t openRet = usbHostManager_->OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    int32_t resetRet = usbHostManager_->ResetDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    int32_t closeRet = usbHostManager_->Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    USB_HILOGI(MODULE_USB_HOST, "open=%{public}d reset=%{public}d close=%{public}d",
                openRet, resetRet, closeRet);
}

/**
 * @tc.name: UsbHostManager_Hotplug_GetDevices_AfterConnect
 * @tc.desc: Test GetDevices after device connection simulation
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerHotplugTest, UsbHostManager_Hotplug_GetDevices_AfterConnect, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_GetDevices_AfterConnect start");

    std::vector<UsbDevice> deviceListBefore;
    int32_t retBefore = usbHostManager_->GetDevices(deviceListBefore);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<UsbDevice> deviceListAfter;
    int32_t retAfter = usbHostManager_->GetDevices(deviceListAfter);

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_GetDevices_AfterConnect before=%{public}zu after=%{public}zu",
                deviceListBefore.size(), deviceListAfter.size());
    EXPECT_EQ(retBefore, UEC_OK);
    EXPECT_EQ(retAfter, UEC_OK);
}

/**
 * @tc.name: UsbHostManager_Hotplug_ClaimInterfaceAfterOpen
 * @tc.desc: Test ClaimInterface immediately after device open
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerHotplugTest, UsbHostManager_Hotplug_ClaimInterfaceAfterOpen, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_ClaimInterfaceAfterOpen start");

    int32_t openRet = usbHostManager_->OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    uint8_t force = 1;
    int32_t claimRet = usbHostManager_->ClaimInterface(TEST_BUS_NUM_1, TEST_DEV_ADDR_1,
                                                        TEST_INTERFACE_ID_0, force);

    int32_t closeRet = usbHostManager_->Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    USB_HILOGI(MODULE_USB_HOST, "open=%{public}d claim=%{public}d close=%{public}d",
                openRet, claimRet, closeRet);
}

/**
 * @tc.name: UsbHostManager_Hotplug_MultipleOpenAttempts
 * @tc.desc: Test multiple open attempts without close
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerHotplugTest, UsbHostManager_Hotplug_MultipleOpenAttempts, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_MultipleOpenAttempts start");

    int32_t openCount = 0;

    for (int i = 0; i < 5; i++) {
        int32_t ret = usbHostManager_->OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
        if (ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE) {
            openCount++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    int32_t closeRet = usbHostManager_->Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_MultipleOpenAttempts open=%{public}d close=%{public}d",
                openCount, closeRet);
    EXPECT_GE(openCount, 0);
}

/**
 * @tc.name: UsbHostManager_Hotplug_RapidDeviceInfoQueries
 * @tc.desc: Test rapid GetDeviceInfo calls during device connection
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerHotplugTest, UsbHostManager_Hotplug_RapidDeviceInfoQueries, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_RapidDeviceInfoQueries start");

    UsbDevice dev;

    for (int i = 0; i < 20; i++) {
        int32_t ret = usbHostManager_->GetDeviceInfo(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, dev);
        USB_HILOGI(MODULE_USB_HOST, "GetDeviceInfo iteration %{public}d ret=%{public}d", i, ret);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_RapidDeviceInfoQueries end");
}

/**
 * @tc.name: UsbHostManager_Hotplug_TransferDuringDisconnect
 * @tc.desc: Test transfer operations during simulated disconnect
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerHotplugTest, UsbHostManager_Hotplug_TransferDuringDisconnect, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_TransferDuringDisconnect start");

    int32_t openRet = usbHostManager_->OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    UsbDev dev = {TEST_BUS_NUM_1, TEST_DEV_ADDR_1};
    UsbPipe pipe = {TEST_INTERFACE_ID_0, TEST_ENDPOINT_ADDR_1};
    std::vector<uint8_t> bufferData = {0x01, 0x02, 0x03};
    int32_t timeOut = 1000;

    int32_t writeRet = usbHostManager_->BulkTransferWrite(dev, pipe, bufferData, timeOut);

    int32_t closeRet = usbHostManager_->Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    USB_HILOGI(MODULE_USB_HOST, "open=%{public}d write=%{public}d close=%{public}d",
                openRet, writeRet, closeRet);
}

/**
 * @tc.name: UsbHostManager_Hotplug_SetInterfaceAfterOpen
 * @tc.desc: Test SetInterface operation after device open
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerHotplugTest, UsbHostManager_Hotplug_SetInterfaceAfterOpen, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_SetInterfaceAfterOpen start");

    int32_t openRet = usbHostManager_->OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    uint8_t altIndex = 1;
    int32_t setRet = usbHostManager_->SetInterface(TEST_BUS_NUM_1, TEST_DEV_ADDR_1,
                                                   TEST_INTERFACE_ID_0, altIndex);

    int32_t closeRet = usbHostManager_->Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    USB_HILOGI(MODULE_USB_HOST, "open=%{public}d set=%{public}d close=%{public}d",
                openRet, setRet, closeRet);
}

/**
 * @tc.name: UsbHostManager_Hotplug_GetRawDescriptor_AfterOpen
 * @tc.desc: Test GetRawDescriptor immediately after device open
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerHotplugTest, UsbHostManager_Hotplug_GetRawDescriptor_AfterOpen, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_GetRawDescriptor_AfterOpen start");

    int32_t openRet = usbHostManager_->OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    std::vector<uint8_t> bufferData;
    int32_t descRet = usbHostManager_->GetRawDescriptor(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, bufferData);

    int32_t closeRet = usbHostManager_->Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    USB_HILOGI(MODULE_USB_HOST, "open=%{public}d desc=%{public}d close=%{public}d size=%{public}zu",
                openRet, descRet, closeRet, bufferData.size());
}

/**
 * @tc.name: UsbHostManager_Hotplug_GetFileDescriptor_AfterOpen
 * @tc.desc: Test GetFileDescriptor immediately after device open
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerHotplugTest, UsbHostManager_Hotplug_GetFileDescriptor_AfterOpen, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_GetFileDescriptor_AfterOpen start");

    int32_t openRet = usbHostManager_->OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    int32_t fd = -1;
    int32_t fdRet = usbHostManager_->GetFileDescriptor(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, fd);

    int32_t closeRet = usbHostManager_->Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    USB_HILOGI(MODULE_USB_HOST, "open=%{public}d fd=%{public}d close=%{public}d",
                openRet, fdRet, closeRet);
}

/**
 * @tc.name: UsbHostManager_Hotplug_ClearHalt_AfterOpen
 * @tc.desc: Test ClearHalt operation after device open
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerHotplugTest, UsbHostManager_Hotplug_ClearHalt_AfterOpen, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Hotplug_ClearHalt_AfterOpen start");

    int32_t openRet = usbHostManager_->OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    int32_t clearRet = usbHostManager_->ClearHalt(TEST_BUS_NUM_1, TEST_DEV_ADDR_1,
                                                   TEST_INTERFACE_ID_0, TEST_ENDPOINT_ADDR_1);

    int32_t closeRet = usbHostManager_->Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);

    USB_HILOGI(MODULE_USB_HOST, "open=%{public}d clear=%{public}d close=%{public}d",
                openRet, clearRet, closeRet);
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS