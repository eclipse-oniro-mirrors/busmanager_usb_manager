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

#include "usb_port_manager_mock_test.h"

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
constexpr int32_t USB_PORT_ID_INVALID_LARGE = 100;
constexpr int32_t USB_PORT_ID_NEGATIVE = -1;
constexpr int32_t USB_POWER_ROLE_INVALID = 5;
constexpr int32_t USB_DATA_ROLE_INVALID = 5;
constexpr int32_t USB_PORT_MODE_INVALID = 100;

sptr<MockUsbImpl> UsbPortManagerMockTest::mockUsbImpl_ = nullptr;
sptr<UsbService> UsbPortManagerMockTest::usbSrv_ = nullptr;

void UsbPortManagerMockTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest SetUpTestCase");
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

void UsbPortManagerMockTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest TearDownTestCase");
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

void UsbPortManagerMockTest::SetUp(void) {}

void UsbPortManagerMockTest::TearDown(void) {}

/**
 * @tc.name: GetPorts001
 * @tc.desc: Test functions to GetPorts
 * @tc.desc: Positive test: get ports successfully
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetPorts001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetPorts001");
    std::vector<UsbPort> portList;
    auto ret = usbSrv_->GetPorts(portList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::GetPorts001 ret=%{public}d size=%{public}zu", ret,
        portList.size());
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: GetPorts002
 * @tc.desc: Test functions to GetPorts multiple times
 * @tc.desc: Positive test: multiple get ports calls
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetPorts002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetPorts002");
    for (int32_t i = 0; i < 5; i++) {
        std::vector<UsbPort> portList;
        auto ret = usbSrv_->GetPorts(portList);
        USB_HILOGI(MODULE_USB_SERVICE, "GetPorts iteration %{public}d ret=%{public}d", i, ret);
        EXPECT_EQ(0, ret);
    }
}

/**
 * @tc.name: GetSupportedModes001
 * @tc.desc: Test functions to GetSupportedModes
 * @tc.desc: Positive test: valid port id
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetSupportedModes001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModes001");
    int32_t result = 0;
    auto ret = usbSrv_->GetSupportedModes(DEFAULT_PORT_ID, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::GetSupportedModes001 ret=%{public}d result=%{public}d",
        ret, result);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: GetSupportedModes002
 * @tc.desc: Test functions to GetSupportedModes
 * @tc.desc: Negative test: invalid port id zero
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetSupportedModes002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModes002");
    int32_t result = 0;
    auto ret = usbSrv_->GetSupportedModes(0, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::GetSupportedModes002 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: GetSupportedModes003
 * @tc.desc: Test functions to GetSupportedModes
 * @tc.desc: Negative test: large invalid port id
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetSupportedModes003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModes003");
    int32_t result = 0;
    auto ret = usbSrv_->GetSupportedModes(USB_PORT_ID_INVALID_LARGE, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::GetSupportedModes003 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: GetSupportedModes004
 * @tc.desc: Test functions to GetSupportedModes
 * @tc.desc: Negative test: negative port id
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetSupportedModes004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModes004");
    int32_t result = 0;
    auto ret = usbSrv_->GetSupportedModes(USB_PORT_ID_NEGATIVE, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::GetSupportedModes004 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: GetSupportedModes005
 * @tc.desc: Test functions to GetSupportedModes
 * @tc.desc: Negative test: PORT_MODE_NONE
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetSupportedModes005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModes005");
    int32_t result = 0;
    auto ret = usbSrv_->GetSupportedModes(UsbSrvSupport::PORT_MODE_NONE, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::GetSupportedModes005 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: GetSupportedModes006
 * @tc.desc: Test functions to GetSupportedModes
 * @tc.desc: Negative test: PORT_MODE_HOST as port id
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetSupportedModes006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModes006");
    int32_t result = 0;
    auto ret = usbSrv_->GetSupportedModes(UsbSrvSupport::PORT_MODE_HOST, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::GetSupportedModes006 ret=%{public}d", ret);
}

/**
 * @tc.name: SetPortRole001
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Positive test: valid parameters - source host
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole001");
    auto ret = usbSrv_->SetPortRole(
        DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole001 ret=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: SetPortRole002
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Positive test: valid parameters - sink device
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole002");
    auto ret = usbSrv_->SetPortRole(
        DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole002 ret=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: SetPortRole003
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Positive test: valid parameters - source device
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole003");
    auto ret = usbSrv_->SetPortRole(
        DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_DEVICE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole003 ret=%{public}d", ret);
}

/**
 * @tc.name: SetPortRole004
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Positive test: valid parameters - sink host
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole004");
    auto ret = usbSrv_->SetPortRole(
        DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole004 ret=%{public}d", ret);
}

/**
 * @tc.name: SetPortRole005
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: invalid power role
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole005");
    auto ret = usbSrv_->SetPortRole(DEFAULT_PORT_ID, USB_POWER_ROLE_INVALID, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole005 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole006
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: invalid data role
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole006");
    auto ret = usbSrv_->SetPortRole(DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, USB_DATA_ROLE_INVALID);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole006 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole007
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: both roles invalid
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole007");
    auto ret = usbSrv_->SetPortRole(DEFAULT_PORT_ID, USB_POWER_ROLE_INVALID, USB_DATA_ROLE_INVALID);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole007 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole008
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: invalid port id zero
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole008");
    auto ret = usbSrv_->SetPortRole(0, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole008 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole009
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: large invalid port id
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole009");
    auto ret = usbSrv_->SetPortRole(
        USB_PORT_ID_INVALID_LARGE, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole009 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole010
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: negative port id
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole010");
    auto ret = usbSrv_->SetPortRole(
        USB_PORT_ID_NEGATIVE, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole010 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole011
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: PORT_MODE_DEVICE as port id
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole011, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole011");
    auto ret = usbSrv_->SetPortRole(
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole011 ret=%{public}d", ret);
}

/**
 * @tc.name: SetPortRole012
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: PORT_MODE_HOST as port id
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole012, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole012");
    auto ret = usbSrv_->SetPortRole(
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole012 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole013
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: power role negative value
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole013, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole013");
    auto ret = usbSrv_->SetPortRole(DEFAULT_PORT_ID, -1, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole013 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole014
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: data role negative value
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole014, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole014");
    auto ret = usbSrv_->SetPortRole(DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, -1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole014 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole015
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Positive test: switch between roles
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole015, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole015");
    auto ret1 = usbSrv_->SetPortRole(
        DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_EQ(0, ret1);
    auto ret2 = usbSrv_->SetPortRole(
        DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    EXPECT_EQ(0, ret2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole015 ret1=%{public}d ret2=%{public}d", ret1,
        ret2);
}

/**
 * @tc.name: SetPortRole016
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Positive test: set same role twice
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole016, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole016");
    auto ret1 = usbSrv_->SetPortRole(
        DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_EQ(0, ret1);
    auto ret2 = usbSrv_->SetPortRole(
        DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_EQ(0, ret2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole016 ret1=%{public}d ret2=%{public}d", ret1,
        ret2);
}

/**
 * @tc.name: SetPortRole017
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: large power role value
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole017, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole017");
    auto ret = usbSrv_->SetPortRole(DEFAULT_PORT_ID, 100, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole017 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole018
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: large data role value
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole018, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole018");
    auto ret = usbSrv_->SetPortRole(DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, 100);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole018 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole019
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: all invalid large values
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole019, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole019");
    auto ret = usbSrv_->SetPortRole(100, 100, 100);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole019 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole020
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: Negative test: all negative values
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRole020, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole020");
    auto ret = usbSrv_->SetPortRole(-1, -1, -1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRole020 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: GetPortsThenGetSupportedModes001
 * @tc.desc: Test GetPorts then GetSupportedModes for each port
 * @tc.desc: Positive test: combined flow
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetPortsThenGetSupportedModes001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetPortsThenGetSupportedModes001");
    std::vector<UsbPort> portList;
    auto ret = usbSrv_->GetPorts(portList);
    EXPECT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "GetPorts returned %{public}zu ports", portList.size());
}

/**
 * @tc.name: SetPortRoleStress001
 * @tc.desc: Stress test SetPortRole with valid parameters
 * @tc.desc: Positive test: stress valid params
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRoleStress001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRoleStress001");
    for (int32_t i = 0; i < 10; i++) {
        auto ret = usbSrv_->SetPortRole(
            DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
        EXPECT_EQ(0, ret);
        USB_HILOGI(MODULE_USB_SERVICE, "SetPortRole iteration %{public}d ret=%{public}d", i, ret);
    }
}

/**
 * @tc.name: SetPortRoleBoundary001
 * @tc.desc: Test SetPortRole with boundary power role values
 * @tc.desc: Negative test: boundary power roles
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRoleBoundary001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRoleBoundary001");
    auto ret = usbSrv_->SetPortRole(DEFAULT_PORT_ID, 0, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRoleBoundary001 ret=%{public}d", ret);
}

/**
 * @tc.name: SetPortRoleBoundary002
 * @tc.desc: Test SetPortRole with boundary data role values
 * @tc.desc: Negative test: boundary data roles
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRoleBoundary002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRoleBoundary002");
    auto ret = usbSrv_->SetPortRole(DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRoleBoundary002 ret=%{public}d", ret);
}

/**
 * @tc.name: GetSupportedModesInvalidId001
 * @tc.desc: Test GetSupportedModes with INT_MAX port id
 * @tc.desc: Negative test: INT_MAX port id
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetSupportedModesInvalidId001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModesInvalidId001");
    int32_t result = 0;
    auto ret = usbSrv_->GetSupportedModes(0x7FFFFFFF, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::GetSupportedModesInvalidId001 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: GetSupportedModesInvalidId002
 * @tc.desc: Test GetSupportedModes with INT_MIN port id
 * @tc.desc: Negative test: INT_MIN port id
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetSupportedModesInvalidId002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModesInvalidId002");
    int32_t result = 0;
    auto ret = usbSrv_->GetSupportedModes(0x80000000, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::GetSupportedModesInvalidId002 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRoleAfterGetPorts001
 * @tc.desc: Test SetPortRole after GetPorts
 * @tc.desc: Positive test: get then set flow
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRoleAfterGetPorts001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRoleAfterGetPorts001");
    std::vector<UsbPort> portList;
    auto ret = usbSrv_->GetPorts(portList);
    EXPECT_EQ(0, ret);
    ret = usbSrv_->SetPortRole(DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "SetPortRoleAfterGetPorts001 ret=%{public}d", ret);
}

/**
 * @tc.name: SetPortRoleMaxPortId001
 * @tc.desc: Test SetPortRole with max port id value
 * @tc.desc: Negative test: max port id
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, SetPortRoleMaxPortId001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRoleMaxPortId001");
    auto ret = usbSrv_->SetPortRole(0x7FFFFFFF, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerMockTest::SetPortRoleMaxPortId001 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: GetSupportedModesStress001
 * @tc.desc: Stress test GetSupportedModes
 * @tc.desc: Positive test: stress get supported modes
 * @tc.type: FUNC
 */
HWTEST_F(UsbPortManagerMockTest, GetSupportedModesStress001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModesStress001");
    for (int32_t i = 0; i < 10; i++) {
        int32_t result = 0;
        auto ret = usbSrv_->GetSupportedModes(DEFAULT_PORT_ID, result);
        USB_HILOGI(MODULE_USB_SERVICE, "iteration %{public}d ret=%{public}d result=%{public}d", i, ret, result);
    }
}
} // namespace USB
} // namespace OHOS
