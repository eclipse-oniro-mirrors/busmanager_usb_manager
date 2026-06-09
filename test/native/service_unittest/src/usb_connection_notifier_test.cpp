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
#include <thread>
#include <chrono>

#include "usb_connection_notifier.h"
#include "usb_srv_support.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;

constexpr const char* TEST_FUNC_CHARGE = "charge";
constexpr const char* TEST_FUNC_MTP = "mtp";
constexpr const char* TEST_FUNC_PTP = "ptp";
constexpr const char* TEST_FUNC_MORE = "more";

class UsbConnectionNotifierTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<UsbConnectionNotifier> usbConnectionNotifier_;
};

void UsbConnectionNotifierTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifierTest::SetUpTestCase enter");
}

void UsbConnectionNotifierTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifierTest::TearDownTestCase enter");
}

void UsbConnectionNotifierTest::SetUp()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifierTest::SetUp enter");
    usbConnectionNotifier_ = UsbConnectionNotifier::GetInstance();
    ASSERT_NE(usbConnectionNotifier_, nullptr);
}

void UsbConnectionNotifierTest::TearDown()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifierTest::TearDown enter");
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_GetInstance_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_GetInstance_001 enter");
    auto notifier1 = UsbConnectionNotifier::GetInstance();
    auto notifier2 = UsbConnectionNotifier::GetInstance();
    ASSERT_EQ(notifier1, notifier2);
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_SendNotification_Charge_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_SendNotification_Charge_001 enter");
    usbConnectionNotifier_->SendNotification(TEST_FUNC_CHARGE);
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_SendNotification_Mtp_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_SendNotification_Mtp_001 enter");
    usbConnectionNotifier_->SendNotification(TEST_FUNC_MTP);
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_SendNotification_Ptp_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_SendNotification_Ptp_001 enter");
    usbConnectionNotifier_->SendNotification(TEST_FUNC_PTP);
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_SendNotification_More_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_SendNotification_More_001 enter");
    usbConnectionNotifier_->SendNotification(TEST_FUNC_MORE);
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_CancelNotification_Host_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_CancelNotification_Host_001 enter");
    usbConnectionNotifier_->CancelNotification(true);
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_CancelNotification_Device_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_CancelNotification_Device_001 enter");
    usbConnectionNotifier_->CancelNotification(false);
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_SendHdcNotification_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_SendHdcNotification_001 enter");
    usbConnectionNotifier_->SendHdcNotification();
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_CancelHdcNotification_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_CancelHdcNotification_001 enter");
    usbConnectionNotifier_->CancelHdcNotification();
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_NotifierMap_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_NotifierMap_001 enter");
    ASSERT_FALSE(usbConnectionNotifier_->notifierMap.empty());
    ASSERT_EQ(usbConnectionNotifier_->notifierMap.size(), 6);
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_FunctionConstants_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_FunctionConstants_001 enter");
    ASSERT_EQ(USB_FUNC_CHARGE, "USB_func_charge_notifier");
    ASSERT_EQ(USB_FUNC_MTP, "USB_func_mtp_notifier");
    ASSERT_EQ(USB_FUNC_PTP, "USB_func_ptp_notifier");
    ASSERT_EQ(USB_FUNC_MORE, "USB_func_more_notifier");
    ASSERT_EQ(USB_HDC_NOTIFIER_TITLE, "USB_hdc_notifier_title");
    ASSERT_EQ(USB_HDC_NOTIFIER_CONTENT, "USB_hdc_notifier_content");
    ASSERT_EQ(USB_FUNC_REVERSE_CHARGE, "USB_func_charge_connected_notifier");
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_SendCancelSequence_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_SendCancelSequence_001 enter");
    usbConnectionNotifier_->SendNotification(TEST_FUNC_MTP);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    usbConnectionNotifier_->CancelNotification(false);
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_MultipleNotifications_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_MultipleNotifications_001 enter");
    usbConnectionNotifier_->SendNotification(TEST_FUNC_CHARGE);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    usbConnectionNotifier_->SendNotification(TEST_FUNC_MTP);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    usbConnectionNotifier_->SendNotification(TEST_FUNC_PTP);
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_HdcNotificationSequence_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_HdcNotificationSequence_001 enter");
    usbConnectionNotifier_->SendHdcNotification();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    usbConnectionNotifier_->CancelHdcNotification();
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_UnknownFunction_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_UnknownFunction_001 enter");
    usbConnectionNotifier_->SendNotification("unknown_function");
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_RapidNotifications_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_RapidNotifications_001 enter");
    for (int32_t i = 0; i < 5; ++i) {
        usbConnectionNotifier_->SendNotification(TEST_FUNC_CHARGE);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_EmptyString_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_EmptyString_001 enter");
    usbConnectionNotifier_->SendNotification("");
}

HWTEST_F(UsbConnectionNotifierTest, UsbConnectionNotifier_AllFunctionTypes_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbConnectionNotifier_AllFunctionTypes_001 enter");
    std::vector<std::string> functions = {
        TEST_FUNC_CHARGE, TEST_FUNC_MTP, TEST_FUNC_PTP, TEST_FUNC_MORE
    };

    for (const auto& func : functions) {
        usbConnectionNotifier_->SendNotification(func);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        usbConnectionNotifier_->CancelNotification(false);
    }
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS