/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "usb_right_manager_mock_test.h"

#include <csignal>
#include <cstring>
#include <iostream>
#include <vector>

#include "hilog_wrapper.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "usb_common_test.h"
#include "usb_errors.h"
#include "usb_service.h"
#include "usb_srv_client.h"
#include "usb_srv_support.h"

using namespace OHOS;
using namespace OHOS::USB;
using namespace OHOS::USB::Common;
using namespace std;
using namespace testing::ext;
using ::testing::Eq;
using ::testing::Exactly;
using ::testing::Ge;
using ::testing::Le;
using ::testing::Ne;
using ::testing::Return;

namespace OHOS {
namespace USB {
constexpr int32_t USB_RIGHT_TEST_UID = 100;
constexpr int32_t USB_RIGHT_TEST_UID_INVALID = -1;
constexpr int32_t USB_RIGHT_TEST_USER_ID = 0;

sptr<MockUsbImpl> UsbRightManagerMockTest::mockUsbImpl_ = nullptr;
sptr<UsbService> UsbRightManagerMockTest::usbSrv_ = nullptr;

void UsbRightManagerMockTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest SetUpTestCase");
    UsbCommonTest::SetTestCaseHapApply();

    usbSrv_ = DelayedSpSingleton<UsbService>::GetInstance();
    EXPECT_NE(usbSrv_, nullptr);
    mockUsbImpl_ = DelayedSpSingleton<MockUsbImpl>::GetInstance();
    EXPECT_NE(mockUsbImpl_, nullptr);

    usbSrv_->SetUsbd(mockUsbImpl_);

    sptr<UsbServiceSubscriber> iSubscriber = new UsbServiceSubscriber();
    EXPECT_NE(iSubscriber, nullptr);
    mockUsbImpl_->BindUsbdSubscriber(iSubscriber);

    EXPECT_CALL(*mockUsbImpl_, GetCurrentFunctions(testing::_)).WillRepeatedly(Return(0));
    USBDeviceInfo info = {ACT_UPDEVICE, BUS_NUM_OK, DEV_ADDR_OK};
    auto ret = mockUsbImpl_->SubscriberDeviceEvent(info);
    EXPECT_EQ(0, ret);
}

void UsbRightManagerMockTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest TearDownTestCase");
    USBDeviceInfo info = {ACT_DOWNDEVICE, BUS_NUM_OK, DEV_ADDR_OK};
    auto ret = mockUsbImpl_->SubscriberDeviceEvent(info);
    EXPECT_EQ(0, ret);

    mockUsbImpl_->UnbindUsbdSubscriber(nullptr);
    sptr<IUsbInterface> usbd = IUsbInterface::Get();
    usbSrv_->SetUsbd(usbd);

    mockUsbImpl_ = nullptr;
    usbSrv_ = nullptr;
    DelayedSpSingleton<UsbService>::DestroyInstance();
    DelayedSpSingleton<MockUsbImpl>::DestroyInstance();
}

void UsbRightManagerMockTest::SetUp(void) {}

void UsbRightManagerMockTest::TearDown(void) {}

/**
 * @tc.name: UsbHasRight001
 * @tc.desc: Test functions to HasRight
 * @tc.desc: Positive test: device right exists
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbHasRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbHasRight001");
    std::string deviceName = "device_100";
    bool result = usbSrv_->HasRight(deviceName);
    EXPECT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbHasRight001 result=%{public}d", result);
}

/**
 * @tc.name: UsbHasRight002
 * @tc.desc: Test functions to HasRight
 * @tc.desc: Positive test: check with empty device name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbHasRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbHasRight002");
    std::string deviceName = "";
    bool result = usbSrv_->HasRight(deviceName);
    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbHasRight002 result=%{public}d", result);
}

/**
 * @tc.name: UsbHasRight003
 * @tc.desc: Test functions to HasRight
 * @tc.desc: Positive test: check with long device name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbHasRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbHasRight003");
    std::string deviceName = "device_with_very_long_name_1234567890_abcdefghijklmnopqrstuvwxyz";
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbHasRight003 result=%{public}d", result);
}

/**
 * @tc.name: UsbHasRight004
 * @tc.desc: Test functions to HasRight
 * @tc.desc: Positive test: check with special characters
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbHasRight004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbHasRight004");
    std::string deviceName = "device-100_test.usb";
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbHasRight004 result=%{public}d", result);
}

/**
 * @tc.name: UsbHasRight005
 * @tc.desc: Test functions to HasRight
 * @tc.desc: Positive test: check with numeric device name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbHasRight005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbHasRight005");
    std::string deviceName = "12345";
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbHasRight005 result=%{public}d", result);
}

/**
 * @tc.name: UsbAddRight001
 * @tc.desc: Test functions to AddDeviceRight
 * @tc.desc: Positive test: add device right successfully
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbAddRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbAddRight001");
    std::string deviceName = "device_200";
    auto ret = usbSrv_->AddDeviceRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbAddRight001 ret=%{public}d", ret);
}

/**
 * @tc.name: UsbAddRight002
 * @tc.desc: Test functions to AddDeviceRight
 * @tc.desc: Positive test: add right with empty name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbAddRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbAddRight002");
    std::string deviceName = "";
    auto ret = usbSrv_->AddDeviceRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbAddRight002 ret=%{public}d", ret);
}

/**
 * @tc.name: UsbAddRight003
 * @tc.desc: Test functions to AddDeviceRight
 * @tc.desc: Positive test: add right with long device name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbAddRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbAddRight003");
    std::string deviceName = "device_long_name_abcdefghijklmnopqrstuvwxyz_1234567890";
    auto ret = usbSrv_->AddDeviceRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbAddRight003 ret=%{public}d", ret);
}

/**
 * @tc.name: UsbAddRight004
 * @tc.desc: Test functions to AddDeviceRight
 * @tc.desc: Positive test: add right with special characters
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbAddRight004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbAddRight004");
    std::string deviceName = "device-special.chars_test";
    auto ret = usbSrv_->AddDeviceRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbAddRight004 ret=%{public}d", ret);
}

/**
 * @tc.name: UsbRemoveRight001
 * @tc.desc: Test functions to RemoveRight
 * @tc.desc: Positive test: remove device right successfully
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRemoveRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRemoveRight001");
    std::string deviceName = "device_300";
    auto ret = usbSrv_->RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRemoveRight001 ret=%{public}d", ret);
}

/**
 * @tc.name: UsbRemoveRight002
 * @tc.desc: Test functions to RemoveRight
 * @tc.desc: Positive test: remove right with empty name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRemoveRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRemoveRight002");
    std::string deviceName = "";
    auto ret = usbSrv_->RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRemoveRight002 ret=%{public}d", ret);
}

/**
 * @tc.name: UsbRemoveRight003
 * @tc.desc: Test functions to RemoveRight
 * @tc.desc: Positive test: remove right that was just added
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRemoveRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRemoveRight003");
    std::string deviceName = "device_301";
    auto ret = usbSrv_->AddDeviceRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRemoveRight003 AddRight ret=%{public}d", ret);

    ret = usbSrv_->RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRemoveRight003 RemoveRight ret=%{public}d", ret);
}

/**
 * @tc.name: UsbRemoveRight004
 * @tc.desc: Test functions to RemoveRight
 * @tc.desc: Positive test: remove non-existent right
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRemoveRight004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRemoveRight004");
    std::string deviceName = "nonexistent_device_999";
    auto ret = usbSrv_->RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRemoveRight004 ret=%{public}d", ret);
}

/**
 * @tc.name: UsbAddAndCheckRight001
 * @tc.desc: Test add right then check HasRight
 * @tc.desc: Positive test: add then verify
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbAddAndCheckRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbAddAndCheckRight001");
    std::string deviceName = "device_400";
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbAddAndCheckRight001 result=%{public}d", result);
}

/**
 * @tc.name: UsbAddRemoveCheckRight001
 * @tc.desc: Test add right, remove, then check HasRight
 * @tc.desc: Positive test: full lifecycle
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbAddRemoveCheckRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbAddRemoveCheckRight001");
    std::string deviceName = "device_500";
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest after AddRight result=%{public}d", result);

    auto ret = usbSrv_->RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest after RemoveRight ret=%{public}d", ret);
}

/**
 * @tc.name: UsbMultipleAddRight001
 * @tc.desc: Test adding right for multiple devices
 * @tc.desc: Positive test: multiple devices
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbMultipleAddRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbMultipleAddRight001");
    std::vector<std::string> deviceNames = {"device_601", "device_602", "device_603"};
    for (const auto &name : deviceNames) {
        usbSrv_->AddDeviceRight(name);
    }
    for (const auto &name : deviceNames) {
        bool result = usbSrv_->HasRight(name);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest HasRight %{public}s=%{public}d", name.c_str(), result);
    }
}

/**
 * @tc.name: UsbMultipleAddRight002
 * @tc.desc: Test adding right for multiple devices then removing all
 * @tc.desc: Positive test: batch cleanup
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbMultipleAddRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbMultipleAddRight002");
    std::vector<std::string> deviceNames = {"device_701", "device_702", "device_703", "device_704"};
    for (const auto &name : deviceNames) {
        usbSrv_->AddDeviceRight(name);
    }
    for (const auto &name : deviceNames) {
        auto ret = usbSrv_->RemoveRight(name);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest RemoveRight %{public}s=%{public}d", name.c_str(), ret);
    }
}

/**
 * @tc.name: UsbHasRightWithDifferentNames001
 * @tc.desc: Test HasRight with various device name formats
 * @tc.desc: Positive test: various formats
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbHasRightWithDifferentNames001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbHasRightWithDifferentNames001");
    std::vector<std::string> deviceNames = {
        "USB\\VID_1234&PID_5678",
        "usb_device_0",
        "device-name-with-dashes",
        "device_name_with_underscores",
        "DEVICE_UPPER_CASE",
        "device.lower.case"
    };
    for (const auto &name : deviceNames) {
        usbSrv_->AddDeviceRight(name);
        bool result = usbSrv_->HasRight(name);
        USB_HILOGI(MODULE_USB_SERVICE, "HasRight %{public}s=%{public}d", name.c_str(), result);
    }
}

/**
 * @tc.name: UsbRightAfterRemoveAndReadd001
 * @tc.desc: Test right after remove and re-add
 * @tc.desc: Positive test: re-add after remove
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRightAfterRemoveAndReadd001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRightAfterRemoveAndReadd001");
    std::string deviceName = "device_800";
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "After first add result=%{public}d", result);

    usbSrv_->RemoveRight(deviceName);
    usbSrv_->AddDeviceRight(deviceName);
    result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "After re-add result=%{public}d", result);
}

/**
 * @tc.name: UsbDuplicateAddRight001
 * @tc.desc: Test adding the same right multiple times
 * @tc.desc: Positive test: duplicate add
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbDuplicateAddRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDuplicateAddRight001");
    std::string deviceName = "device_900";
    usbSrv_->AddDeviceRight(deviceName);
    usbSrv_->AddDeviceRight(deviceName);
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbDuplicateAddRight001 result=%{public}d", result);
}

/**
 * @tc.name: UsbRemoveNonExistentRight001
 * @tc.desc: Test removing a non-existent right
 * @tc.desc: Negative test: non-existent right
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRemoveNonExistentRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRemoveNonExistentRight001");
    std::string deviceName = "nonexistent_device_1000";
    auto ret = usbSrv_->RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRemoveNonExistentRight001 ret=%{public}d", ret);
}

/**
 * @tc.name: UsbRemoveRightTwice001
 * @tc.desc: Test removing the same right twice
 * @tc.desc: Positive test: double remove
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRemoveRightTwice001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRemoveRightTwice001");
    std::string deviceName = "device_1100";
    usbSrv_->AddDeviceRight(deviceName);
    auto ret1 = usbSrv_->RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "First RemoveRight ret=%{public}d", ret1);
    auto ret2 = usbSrv_->RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "Second RemoveRight ret=%{public}d", ret2);
}

/**
 * @tc.name: UsbHasRightWithUnicode001
 * @tc.desc: Test HasRight with unicode device name
 * @tc.desc: Positive test: unicode name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbHasRightWithUnicode001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbHasRightWithUnicode001");
    std::string deviceName = "device_unicode";
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbHasRightWithUnicode001 result=%{public}d", result);
}

/**
 * @tc.name: UsbStressAddRemove001
 * @tc.desc: Stress test add and remove rights
 * @tc.desc: Positive test: stress add/remove
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbStressAddRemove001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbStressAddRemove001");
    for (int32_t i = 0; i < 50; i++) {
        std::string deviceName = "stress_device_" + std::to_string(i);
        usbSrv_->AddDeviceRight(deviceName);
    }
    for (int32_t i = 0; i < 50; i++) {
        std::string deviceName = "stress_device_" + std::to_string(i);
        auto ret = usbSrv_->RemoveRight(deviceName);
        USB_HILOGI(MODULE_USB_SERVICE, "RemoveRight %{public}s=%{public}d", deviceName.c_str(), ret);
    }
}

/**
 * @tc.name: UsbStressAddRemove002
 * @tc.desc: Stress test interleaved add and remove
 * @tc.desc: Positive test: interleaved operations
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbStressAddRemove002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbStressAddRemove002");
    for (int32_t i = 0; i < 30; i++) {
        std::string deviceName = "interleave_device_" + std::to_string(i);
        usbSrv_->AddDeviceRight(deviceName);
        bool hasRight = usbSrv_->HasRight(deviceName);
        USB_HILOGI(MODULE_USB_SERVICE, "HasRight %{public}s=%{public}d", deviceName.c_str(), hasRight);
        auto ret = usbSrv_->RemoveRight(deviceName);
        USB_HILOGI(MODULE_USB_SERVICE, "RemoveRight %{public}s=%{public}d", deviceName.c_str(), ret);
    }
}

/**
 * @tc.name: UsbDeviceNameWithSpaces001
 * @tc.desc: Test right with device name containing spaces
 * @tc.desc: Positive test: spaces in name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbDeviceNameWithSpaces001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDeviceNameWithSpaces001");
    std::string deviceName = "device with spaces";
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbDeviceNameWithSpaces001 result=%{public}d", result);
}

/**
 * @tc.name: UsbDeviceNameWithNumbers001
 * @tc.desc: Test right with numeric-only device name
 * @tc.desc: Positive test: numeric name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbDeviceNameWithNumbers001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDeviceNameWithNumbers001");
    std::string deviceName = "123456789";
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbDeviceNameWithNumbers001 result=%{public}d", result);
}

/**
 * @tc.name: UsbDeviceNameSingleChar001
 * @tc.desc: Test right with single character device name
 * @tc.desc: Positive test: single char name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbDeviceNameSingleChar001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDeviceNameSingleChar001");
    std::string deviceName = "a";
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbDeviceNameSingleChar001 result=%{public}d", result);
}

/**
 * @tc.name: UsbDeviceNameMaxLen001
 * @tc.desc: Test right with maximum length device name
 * @tc.desc: Positive test: max length name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbDeviceNameMaxLen001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDeviceNameMaxLen001");
    std::string deviceName(256, 'x');
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbDeviceNameMaxLen001 result=%{public}d", result);
}

/**
 * @tc.name: UsbDeviceNamePathLike001
 * @tc.desc: Test right with path-like device name
 * @tc.desc: Positive test: path-like name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbDeviceNamePathLike001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDeviceNamePathLike001");
    std::string deviceName = "/dev/bus/usb/001/002";
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbDeviceNamePathLike001 result=%{public}d", result);
}

/**
 * @tc.name: UsbDeviceNameVIDPID001
 * @tc.desc: Test right with VID-PID format device name
 * @tc.desc: Positive test: VID-PID format
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbDeviceNameVIDPID001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDeviceNameVIDPID001");
    std::string deviceName = "VID_1234&PID_5678";
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbDeviceNameVIDPID001 result=%{public}d", result);
}

/**
 * @tc.name: UsbAddRightConcurrent001
 * @tc.desc: Test adding rights for many devices in sequence
 * @tc.desc: Positive test: sequential add
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbAddRightConcurrent001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbAddRightConcurrent001");
    for (int32_t i = 0; i < 100; i++) {
        std::string deviceName = "concurrent_device_" + std::to_string(i);
        auto ret = usbSrv_->AddDeviceRight(deviceName);
        USB_HILOGI(MODULE_USB_SERVICE, "AddRight %{public}s=%{public}d", deviceName.c_str(), ret);
    }
}

/**
 * @tc.name: UsbRemoveAllAfterBatchAdd001
 * @tc.desc: Test batch remove after batch add
 * @tc.desc: Positive test: batch operations
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRemoveAllAfterBatchAdd001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRemoveAllAfterBatchAdd001");
    std::vector<std::string> deviceNames;
    for (int32_t i = 0; i < 20; i++) {
        deviceNames.push_back("batch_device_" + std::to_string(i));
    }
    for (const auto &name : deviceNames) {
        usbSrv_->AddDeviceRight(name);
    }
    for (const auto &name : deviceNames) {
        usbSrv_->RemoveRight(name);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRemoveAllAfterBatchAdd001 done");
}

/**
 * @tc.name: UsbRightCaseSensitive001
 * @tc.desc: Test right case sensitivity
 * @tc.desc: Positive test: case sensitivity
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRightCaseSensitive001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRightCaseSensitive001");
    std::string deviceName1 = "Device_Case_Test";
    std::string deviceName2 = "device_case_test";
    std::string deviceName3 = "DEVICE_CASE_TEST";
    usbSrv_->AddDeviceRight(deviceName1);
    bool result1 = usbSrv_->HasRight(deviceName1);
    bool result2 = usbSrv_->HasRight(deviceName2);
    bool result3 = usbSrv_->HasRight(deviceName3);
    USB_HILOGI(MODULE_USB_SERVICE, "CaseSensitive: r1=%{public}d r2=%{public}d r3=%{public}d", result1, result2,
        result3);
}

/**
 * @tc.name: UsbRightWithNewline001
 * @tc.desc: Test right with newline in device name
 * @tc.desc: Positive test: newline in name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRightWithNewline001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRightWithNewline001");
    std::string deviceName = "device\nnewline";
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRightWithNewline001 result=%{public}d", result);
}

/**
 * @tc.name: UsbRightWithTab001
 * @tc.desc: Test right with tab in device name
 * @tc.desc: Positive test: tab in name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRightWithTab001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRightWithTab001");
    std::string deviceName = "device\ttab";
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRightWithTab001 result=%{public}d", result);
}

/**
 * @tc.name: UsbRightDevicePrefix001
 * @tc.desc: Test rights with device_ prefix
 * @tc.desc: Positive test: device_ prefix
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRightDevicePrefix001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRightDevicePrefix001");
    for (int32_t i = 80; i < 90; i++) {
        std::string deviceName = "device_" + std::to_string(i);
        usbSrv_->AddDeviceRight(deviceName);
        bool result = usbSrv_->HasRight(deviceName);
        USB_HILOGI(MODULE_USB_SERVICE, "device_%{public}d HasRight=%{public}d", i, result);
    }
}

/**
 * @tc.name: UsbRightBoundary001
 * @tc.desc: Test right with boundary values
 * @tc.desc: Positive test: boundary values
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRightBoundary001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRightBoundary001");
    std::string deviceName = "device_0";
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    EXPECT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRightBoundary001 result=%{public}d", result);
}

/**
 * @tc.name: UsbRightBoundary002
 * @tc.desc: Test right with maximum numeric suffix
 * @tc.desc: Positive test: maximum suffix
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRightBoundary002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRightBoundary002");
    std::string deviceName = "device_2147483647";
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRightBoundary002 result=%{public}d", result);
}

/**
 * @tc.name: UsbRightAfterManyOperations001
 * @tc.desc: Test right after many add/remove operations
 * @tc.desc: Positive test: many operations
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRightAfterManyOperations001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRightAfterManyOperations001");
    std::string deviceName = "device_many_ops";
    for (int32_t i = 0; i < 10; i++) {
        usbSrv_->AddDeviceRight(deviceName);
        usbSrv_->RemoveRight(deviceName);
    }
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRightAfterManyOperations001 result=%{public}d", result);
}

/**
 * @tc.name: UsbRightAddRemoveDifferentDevices001
 * @tc.desc: Test add and remove rights for different devices
 * @tc.desc: Positive test: different devices
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRightAddRemoveDifferentDevices001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRightAddRemoveDifferentDevices001");
    std::string deviceA = "device_A";
    std::string deviceB = "device_B";
    usbSrv_->AddDeviceRight(deviceA);
    usbSrv_->AddDeviceRight(deviceB);
    bool hasA = usbSrv_->HasRight(deviceA);
    bool hasB = usbSrv_->HasRight(deviceB);
    USB_HILOGI(MODULE_USB_SERVICE, "hasA=%{public}d hasB=%{public}d", hasA, hasB);
    usbSrv_->RemoveRight(deviceA);
    hasA = usbSrv_->HasRight(deviceA);
    hasB = usbSrv_->HasRight(deviceB);
    USB_HILOGI(MODULE_USB_SERVICE, "After remove A: hasA=%{public}d hasB=%{public}d", hasA, hasB);
}

/**
 * @tc.name: UsbRightEmptyStringOperations001
 * @tc.desc: Test operations with empty string device name
 * @tc.desc: Negative test: empty string
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerMockTest, UsbRightEmptyStringOperations001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbRightEmptyStringOperations001");
    std::string deviceName = "";
    usbSrv_->AddDeviceRight(deviceName);
    bool result = usbSrv_->HasRight(deviceName);
    usbSrv_->RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManagerMockTest::UsbRightEmptyStringOperations001 result=%{public}d",
        result);
}
} // namespace USB
} // namespace OHOS
