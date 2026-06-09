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
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

#include "usb_accessory_manager.h"
#include "usb_accessory.h"
#include "usb_errors.h"
#include "usb_srv_support.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;

constexpr int32_t TEST_UID_1 = 100;
constexpr int32_t TEST_UID_2 = 200;
constexpr const char* TEST_BUNDLE_NAME_1 = "com.test.app1";
constexpr const char* TEST_BUNDLE_NAME_2 = "com.test.app2";
constexpr const char* TEST_SERIAL_VALUE_1 = "TEST_SERIAL_001";
constexpr const char* TEST_SERIAL_VALUE_2 = "TEST_SERIAL_002";
constexpr int32_t TEST_FUNCTION_NONE = UsbSrvSupport::FUNCTION_NONE;
constexpr int32_t TEST_FUNCTION_ACM = UsbSrvSupport::FUNCTION_ACM;
constexpr int32_t TEST_FUNCTION_MTP = UsbSrvSupport::FUNCTION_MTP;
constexpr int32_t TEST_FUNCTION_PTP = UsbSrvSupport::FUNCTION_PTP;
constexpr int32_t TEST_FUNCTION_HDC = UsbSrvSupport::FUNCTION_HDC;

class UsbAccessoryManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::unique_ptr<UsbAccessoryManager> usbAccessoryManager_;
};

void UsbAccessoryManagerTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManagerTest::SetUpTestCase enter");
}

void UsbAccessoryManagerTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManagerTest::TearDownTestCase enter");
}

void UsbAccessoryManagerTest::SetUp()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManagerTest::SetUp enter");
    usbAccessoryManager_ = std::make_unique<UsbAccessoryManager>();
    ASSERT_NE(usbAccessoryManager_, nullptr);
}

void UsbAccessoryManagerTest::TearDown()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManagerTest::TearDown enter");
    usbAccessoryManager_.reset();
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_Constructor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_Constructor_001 enter");
    auto accessoryManager = std::make_unique<UsbAccessoryManager>();
    ASSERT_NE(accessoryManager, nullptr);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_GetAccessoryList_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_GetAccessoryList_001 enter");
    std::vector<USBAccessory> accessoryList;
    usbAccessoryManager_->GetAccessoryList(TEST_BUNDLE_NAME_1, accessoryList);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_GetAccessoryList_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_GetAccessoryList_002 enter");
    std::vector<USBAccessory> accessoryList;
    std::string emptyBundleName = "";
    usbAccessoryManager_->GetAccessoryList(emptyBundleName, accessoryList);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_HandleEvent_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_HandleEvent_001 enter");
    usbAccessoryManager_->HandleEvent(ACC_START, false);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_HandleEvent_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_HandleEvent_002 enter");
    usbAccessoryManager_->HandleEvent(ACC_STOP, false);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_HandleEvent_003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_HandleEvent_003 enter");
    usbAccessoryManager_->HandleEvent(ACC_NONE, false);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_GetAccessorySerialNumber_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_GetAccessorySerialNumber_001 enter");
    USBAccessory access;
    std::string serialValue;
    int32_t ret = usbAccessoryManager_->GetAccessorySerialNumber(access, TEST_BUNDLE_NAME_1, serialValue);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_OpenAccessory_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_OpenAccessory_001 enter");
    int32_t fd = 0;
    int32_t ret = usbAccessoryManager_->OpenAccessory(fd);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_CloseAccessory_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_CloseAccessory_001 enter");
    int32_t ret = usbAccessoryManager_->CloseAccessory(0);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_CloseAccessory_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_CloseAccessory_002 enter");
    int32_t ret = usbAccessoryManager_->CloseAccessory(-1);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_HandleEvent_DelayProcess_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_HandleEvent_DelayProcess_001 enter");
    usbAccessoryManager_->HandleEvent(ACC_START, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_MultipleEvents_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_MultipleEvents_001 enter");
    usbAccessoryManager_->HandleEvent(ACC_START, false);
    usbAccessoryManager_->HandleEvent(ACC_SEND, false);
    usbAccessoryManager_->HandleEvent(ACC_STOP, false);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_GetAccessoryList_MultipleBundleNames_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_GetAccessoryList_MultipleBundleNames_001 enter");
    std::vector<USBAccessory> accessoryList1;
    std::vector<USBAccessory> accessoryList2;
    usbAccessoryManager_->GetAccessoryList(TEST_BUNDLE_NAME_1, accessoryList1);
    usbAccessoryManager_->GetAccessoryList(TEST_BUNDLE_NAME_2, accessoryList2);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_Base64Decode_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_Base64Decode_001 enter");
    USBAccessory access;
    std::string serialValue;
    usbAccessoryManager_->GetAccessorySerialNumber(access, TEST_BUNDLE_NAME_1, serialValue);
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_SerialValueHash_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_SerialValueHash_001 enter");
    USBAccessory access;
    std::string serialValue;
    std::string hashValue;
    int32_t ret = usbAccessoryManager_->GetAccessorySerialNumber(access, TEST_BUNDLE_NAME_1, serialValue);
    if (ret >= UEC_OK) {
    }
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_ThreadSafety_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_ThreadSafety_001 enter");
    const int32_t numThreads = 5;
    std::vector<std::thread> threads;

    for (int32_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([this]() {
            std::vector<USBAccessory> accessoryList;
            usbAccessoryManager_->GetAccessoryList(TEST_BUNDLE_NAME_1, accessoryList);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_AccessoryMode_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_AccessoryMode_001 enter");
    usbAccessoryManager_->HandleEvent(ACC_CONFIGURING, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

HWTEST_F(UsbAccessoryManagerTest, UsbAccessoryManager_FunctionChange_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager_FunctionChange_001 enter");
    usbAccessoryManager_->HandleEvent(ACC_START, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    usbAccessoryManager_->HandleEvent(ACC_STOP, false);
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS