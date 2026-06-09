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

#include "usb_function_switch_window.h"
#include "usb_srv_support.h"
#include "usb_errors.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;

constexpr int32_t TEST_FUNCTION_CHARGE_ONLY = -1;
constexpr int32_t TEST_FUNCTION_MTP = UsbSrvSupport::FUNCTION_MTP;
constexpr int32_t TEST_FUNCTION_PTP = UsbSrvSupport::FUNCTION_PTP;
constexpr int32_t TEST_FUNCTION_ACM = UsbSrvSupport::FUNCTION_ACM;
constexpr int32_t TEST_FUNCTION_HDC = UsbSrvSupport::FUNCTION_HDC;

class UsbFunctionSwitchWindowTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<UsbFunctionSwitchWindow> usbFunctionSwitchWindow_;
};

void UsbFunctionSwitchWindowTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindowTest::SetUpTestCase enter");
}

void UsbFunctionSwitchWindowTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindowTest::TearDownTestCase enter");
}

void UsbFunctionSwitchWindowTest::SetUp()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindowTest::SetUp enter");
    usbFunctionSwitchWindow_ = UsbFunctionSwitchWindow::GetInstance();
    ASSERT_NE(usbFunctionSwitchWindow_, nullptr);
}

void UsbFunctionSwitchWindowTest::TearDown()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindowTest::TearDown enter");
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_GetInstance_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_GetInstance_001 enter");
    auto window1 = UsbFunctionSwitchWindow::GetInstance();
    auto window2 = UsbFunctionSwitchWindow::GetInstance();
    ASSERT_EQ(window1, window2);
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_Init_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_Init_001 enter");
    int32_t ret = usbFunctionSwitchWindow_->Init();
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_SetCurrentFunctionLabel_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_SetCurrentFunctionLabel_001 enter");
    int32_t ret = usbFunctionSwitchWindow_->SetCurrentFunctionLabel(TEST_FUNCTION_MTP);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_SetCurrentFunctionLabel_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_SetCurrentFunctionLabel_002 enter");
    int32_t ret = usbFunctionSwitchWindow_->SetCurrentFunctionLabel(TEST_FUNCTION_PTP);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_SetCurrentFunctionLabel_003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_SetCurrentFunctionLabel_003 enter");
    int32_t ret = usbFunctionSwitchWindow_->SetCurrentFunctionLabel(TEST_FUNCTION_CHARGE_ONLY);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_RemoveCurrentFunctionLabel_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_RemoveCurrentFunctionLabel_001 enter");
    int32_t ret = usbFunctionSwitchWindow_->RemoveCurrentFunctionLabel();
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_CheckDialogInstallStatus_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_CheckDialogInstallStatus_001 enter");
    bool installed = usbFunctionSwitchWindow_->CheckDialogInstallStatus();
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_PopUpFunctionSwitchWindow_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_PopUpFunctionSwitchWindow_001 enter");
    bool result = usbFunctionSwitchWindow_->PopUpFunctionSwitchWindow();
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_DismissFunctionSwitchWindow_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_DismissFunctionSwitchWindow_001 enter");
    bool result = usbFunctionSwitchWindow_->DismissFunctionSwitchWindow();
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_MultipleFunctionSwitches_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_MultipleFunctionSwitches_001 enter");
    usbFunctionSwitchWindow_->SetCurrentFunctionLabel(TEST_FUNCTION_MTP);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    usbFunctionSwitchWindow_->SetCurrentFunctionLabel(TEST_FUNCTION_PTP);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    usbFunctionSwitchWindow_->SetCurrentFunctionLabel(TEST_FUNCTION_CHARGE_ONLY);
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_ShowDismissSequence_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_ShowDismissSequence_001 enter");
    usbFunctionSwitchWindow_->PopUpFunctionSwitchWindow();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    usbFunctionSwitchWindow_->DismissFunctionSwitchWindow();
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_ThreadSafety_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_ThreadSafety_001 enter");
    const int32_t numThreads = 5;
    std::vector<std::thread> threads;

    for (int32_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i]() {
            usbFunctionSwitchWindow_->SetCurrentFunctionLabel(
                (i % 2 == 0) ? TEST_FUNCTION_MTP : TEST_FUNCTION_PTP);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_FunctionChoose_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_FunctionChoose_001 enter");
    ASSERT_EQ(FUNCTION_CHOOSE_CHARGE_ONLY, -1);
    ASSERT_EQ(FUNCTION_CHOOSE_TRANSFER_FILE, UsbSrvSupport::FUNCTION_MTP);
    ASSERT_EQ(FUNCTION_CHOOSE_TRANSFER_PIC, UsbSrvSupport::FUNCTION_PTP);
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_SupportedFunc_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_SupportedFunc_001 enter");
    ASSERT_EQ(SUPPORTED_FUNC_NONE, -1);
    ASSERT_EQ(SUPPORTED_FUNC_CHARGE, 0);
    ASSERT_EQ(SUPPORTED_FUNC_MTP, 8);
    ASSERT_EQ(SUPPORTED_FUNC_PTP, 16);
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_WindowAction_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_WindowAction_001 enter");
    ASSERT_EQ(FUNCTION_SWITCH_WINDOW_ACTION_DEFAULT, 0);
    ASSERT_EQ(FUNCTION_SWITCH_WINDOW_ACTION_SHOW, 1);
    ASSERT_EQ(FUNCTION_SWITCH_WINDOW_ACTION_DISMISS, 2);
    ASSERT_EQ(FUNCTION_SWITCH_WINDOW_ACTION_FORBID, 3);
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_InvalidFunction_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_InvalidFunction_001 enter");
    int32_t ret = usbFunctionSwitchWindow_->SetCurrentFunctionLabel(9999);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbFunctionSwitchWindowTest, UsbFunctionSwitchWindow_RapidSequence_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionSwitchWindow_RapidSequence_001 enter");
    for (int32_t i = 0; i < 5; ++i) {
        usbFunctionSwitchWindow_->SetCurrentFunctionLabel(TEST_FUNCTION_MTP);
        usbFunctionSwitchWindow_->RemoveCurrentFunctionLabel();
    }
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS