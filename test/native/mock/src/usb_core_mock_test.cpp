/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "usb_core_mock_test.h"

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

using OHOS::HDI::Usb::V1_0::PortInfo;
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
constexpr int32_t USB_FUNCTION_INVALID = -1;
constexpr int32_t USB_PORT_ID_INVALID = 5;
constexpr int32_t USB_POWER_ROLE_INVALID = 5;
constexpr int32_t USB_DATA_ROLE_INVALID = 5;
sptr<MockUsbImpl> UsbCoreMockTest::mockUsbImpl_ = nullptr;
sptr<UsbService> UsbCoreMockTest::usbSrv_ = nullptr;

bool UsbCoreMockTest::GetBundleName(std::string &bundleName)
{
    pid_t uid = IPCSkeleton::GetCallingUid();
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "systemAbilityManager is nullptr");
        return false;
    }
    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "remoteObject is nullptr");
        return false;
    }

    sptr<AppExecFwk::IBundleMgr> bundleMgr(new AppExecFwk::BundleMgrProxy(remoteObject));
    if (bundleMgr == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "bundleMgr is nullptr");
        return false;
    }
    bundleMgr->GetNameForUid(uid, bundleName);
    return true;
}

void UsbCoreMockTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest UsbCoreMockTest");
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

void UsbCoreMockTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest TearDownTestCase");
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

void UsbCoreMockTest::SetUp(void) {}

void UsbCoreMockTest::TearDown(void) {}

/**
 * @tc.name: GetCurrentFunctions001
 * @tc.desc: Test functions to GetCurrentFunctions()
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, GetCurrentFunctions001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetCurrentFunctions001 : SetConfig");
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_NONE);
    EXPECT_CALL(*mockUsbImpl_, GetCurrentFunctions(testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->GetCurrentFunctions(funcs);
    EXPECT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::ret=%{public}d", ret);
}

/**
 * @tc.name: SetCurrentFunctions001
 * @tc.desc: Test functions to SetCurrentFunctions(int32_t funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetCurrentFunctions001, TestSize.Level1)
{
    EXPECT_CALL(*mockUsbImpl_, SetCurrentFunctions(testing::_)).WillRepeatedly(Return(0));
    int32_t isok = usbSrv_->SetCurrentFunctions(UsbSrvSupport::FUNCTION_ACM);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetCurrentFunctions=%{public}d", isok);
    ASSERT_EQ(0, isok);
}

/**
 * @tc.name: SetCurrentFunctions002
 * @tc.desc: Test functions to SetCurrentFunctions(int32_t funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetCurrentFunctions002, TestSize.Level1)
{
    EXPECT_CALL(*mockUsbImpl_, SetCurrentFunctions(testing::_)).WillRepeatedly(Return(0));
    int32_t isok = usbSrv_->SetCurrentFunctions(UsbSrvSupport::FUNCTION_ECM);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetCurrentFunctions=%{public}d", isok);
    ASSERT_EQ(0, isok);
}

/**
 * @tc.name: SetCurrentFunctions003
 * @tc.desc: Test functions to SetCurrentFunctions(int32_t funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetCurrentFunctions003, TestSize.Level1)
{
    int32_t funcs = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM;
    EXPECT_CALL(*mockUsbImpl_, SetCurrentFunctions(testing::_)).WillRepeatedly(Return(0));
    int32_t isok = usbSrv_->SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetCurrentFunctions=%{public}d", isok);
    ASSERT_EQ(0, isok);
}

/**
 * @tc.name: SetCurrentFunctions004
 * @tc.desc: Test functions to SetCurrentFunctions(int32_t funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetCurrentFunctions004, TestSize.Level1)
{
    EXPECT_CALL(*mockUsbImpl_, SetCurrentFunctions(testing::_)).WillRepeatedly(Return(0));
    int32_t isok = usbSrv_->SetCurrentFunctions(UsbSrvSupport::FUNCTION_HDC);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetCurrentFunctions=%{public}d", isok);
    ASSERT_EQ(0, isok);
}

/**
 * @tc.name: SetCurrentFunctions005
 * @tc.desc: Test functions to SetCurrentFunctions(int32_t funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetCurrentFunctions005, TestSize.Level1)
{
    int32_t funcs = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_HDC;
    EXPECT_CALL(*mockUsbImpl_, SetCurrentFunctions(testing::_)).WillRepeatedly(Return(0));
    int32_t isok = usbSrv_->SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetCurrentFunctions=%{public}d", isok);
    ASSERT_EQ(0, isok);
}

/**
 * @tc.name: SetCurrentFunctions006
 * @tc.desc: Test functions to SetCurrentFunctions(int32_t funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetCurrentFunctions006, TestSize.Level1)
{
    int32_t funcs = UsbSrvSupport::FUNCTION_ECM | UsbSrvSupport::FUNCTION_HDC;
    EXPECT_CALL(*mockUsbImpl_, SetCurrentFunctions(testing::_)).WillRepeatedly(Return(0));
    int32_t isok = usbSrv_->SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetCurrentFunctions=%{public}d", isok);
    ASSERT_EQ(0, isok);
}

/**
 * @tc.name: SetCurrentFunctions007
 * @tc.desc: Test functions to SetCurrentFunctions(int32_t funcs)
 * @tc.desc: Negative test: parameters exception, funcs error
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetCurrentFunctions007, TestSize.Level1)
{
    EXPECT_CALL(*mockUsbImpl_, SetCurrentFunctions(testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    int32_t isok = usbSrv_->SetCurrentFunctions(UsbSrvSupport::FUNCTION_MTP);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetCurrentFunctions=%{public}d", isok);
    ASSERT_NE(0, isok);
}

/**
 * @tc.name: SetCurrentFunctions008
 * @tc.desc: Test functions to SetCurrentFunctions(int32_t funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetCurrentFunctions008, TestSize.Level1)
{
    EXPECT_CALL(*mockUsbImpl_, SetCurrentFunctions(testing::_)).WillRepeatedly(Return(0));
    int32_t isok = usbSrv_->SetCurrentFunctions(UsbSrvSupport::FUNCTION_NONE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetCurrentFunctions=%{public}d", isok);
    ASSERT_EQ(0, isok);

    EXPECT_CALL(*mockUsbImpl_, SetCurrentFunctions(testing::_)).WillRepeatedly(Return(0));
    isok = usbSrv_->SetCurrentFunctions(UsbSrvSupport::FUNCTION_HDC);
    ASSERT_EQ(0, isok);
}

/**
 * @tc.name: UsbFunctionsFromString001
 * @tc.desc: Test functions to UsbFunctionsFromString(string funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbFunctionsFromString001, TestSize.Level1)
{
    int32_t funcCode = usbSrv_->UsbFunctionsFromString(UsbSrvSupport::FUNCTION_NAME_NONE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::functionsFromString=%{public}d", funcCode);
    ASSERT_NE(UEC_SERVICE_INVALID_VALUE, funcCode);
}

/**
 * @tc.name: UsbFunctionsFromString002
 * @tc.desc: Test functions to UsbFunctionsFromString(string funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbFunctionsFromString002, TestSize.Level1)
{
    int32_t funcCode = usbSrv_->UsbFunctionsFromString(UsbSrvSupport::FUNCTION_NAME_HDC);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::functionsFromString=%{public}d", funcCode);
    ASSERT_NE(UEC_SERVICE_INVALID_VALUE, funcCode);
}

/**
 * @tc.name: UsbFunctionsFromString003
 * @tc.desc: Test functions to UsbFunctionsFromString(string funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbFunctionsFromString003, TestSize.Level1)
{
    int32_t funcCode = usbSrv_->UsbFunctionsFromString(UsbSrvSupport::FUNCTION_NAME_ACM);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::functionsFromString=%{public}d", funcCode);
    ASSERT_NE(UEC_SERVICE_INVALID_VALUE, funcCode);
}

/**
 * @tc.name: UsbFunctionsFromString004
 * @tc.desc: Test functions to UsbFunctionsFromString(string funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbFunctionsFromString004, TestSize.Level1)
{
    int32_t funcCode = usbSrv_->UsbFunctionsFromString(UsbSrvSupport::FUNCTION_NAME_ECM);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::functionsFromString=%{public}d", funcCode);
    ASSERT_NE(UEC_SERVICE_INVALID_VALUE, funcCode);
}

/**
 * @tc.name: UsbFunctionsFromString005
 * @tc.desc: Test functions to UsbFunctionsFromString(string funcs)
 * @tc.desc: Negative test: parameters exception, funcs error
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbFunctionsFromString005, TestSize.Level1)
{
    std::string funcs = "qwerts";
    int32_t funcCode = usbSrv_->UsbFunctionsFromString(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::functionsFromString=%{public}d", funcCode);
    ASSERT_EQ(UEC_SERVICE_INVALID_VALUE, funcCode);
}

/**
 * @tc.name: UsbFunctionsFromString006
 * @tc.desc: Test functions to UsbFunctionsFromString(string funcs)
 * @tc.desc: Negative test: parameters exception, funcs error
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbFunctionsFromString006, TestSize.Level1)
{
    std::string funcs = "zxcbvx";
    int32_t funcCode = usbSrv_->UsbFunctionsFromString(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::functionsFromString=%{public}d", funcCode);
    ASSERT_EQ(UEC_SERVICE_INVALID_VALUE, funcCode);
}

/**
 * @tc.name: UsbFunctionsToString001
 * @tc.desc: Test functions to UsbFunctionsToString(int32_t funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbFunctionsToString001, TestSize.Level1)
{
    std::string funcName = usbSrv_->UsbFunctionsToString(UsbSrvSupport::FUNCTION_NONE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::UsbFunctionsToString=%{public}s", funcName.c_str());
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::UsbFunctionsToString=%{public}zu", funcName.size());
    ASSERT_TRUE(!(funcName.empty()));
}

/**
 * @tc.name: UsbFunctionsToString002
 * @tc.desc: Test functions to UsbFunctionsToString(int32_t funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbFunctionsToString002, TestSize.Level1)
{
    std::string funcName = usbSrv_->UsbFunctionsToString(UsbSrvSupport::FUNCTION_HDC);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::UsbFunctionsToString=%{public}s", funcName.c_str());
    ASSERT_TRUE(!(funcName.empty()));
}

/**
 * @tc.name: UsbFunctionsToString003
 * @tc.desc: Test functions to UsbFunctionsToString(int32_t funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbFunctionsToString003, TestSize.Level1)
{
    std::string funcName = usbSrv_->UsbFunctionsToString(UsbSrvSupport::FUNCTION_ACM);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionServiceTest::UsbFunctionsToString=%{public}s", funcName.c_str());
    ASSERT_TRUE(!(funcName.empty()));
}

/**
 * @tc.name: UsbFunctionsToString004
 * @tc.desc: Test functions to UsbFunctionsToString(int32_t funcs)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbFunctionsToString004, TestSize.Level1)
{
    std::string funcName = usbSrv_->UsbFunctionsToString(UsbSrvSupport::FUNCTION_ECM);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionServiceTest::UsbFunctionsToString=%{public}s", funcName.c_str());
    ASSERT_TRUE(!(funcName.empty()));
}

/**
 * @tc.name: UsbFunctionsToString005
 * @tc.desc: Test functions to UsbFunctionsToString(int32_t funcs)
 * @tc.desc: Negative test: parameters exception, funcs error
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbFunctionsToString005, TestSize.Level1)
{
    std::string funcName = usbSrv_->UsbFunctionsToString(USB_FUNCTION_INVALID);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbFunctionServiceTest::UsbFunctionsToString=%{public}s", funcName.c_str());
    ASSERT_TRUE(!(funcName.empty()));
}

/**
 * @tc.name: UsbHasRight001
 * @tc.desc: bool HasRight(const std::string deviceName)
 * @tc.desc: Negative test: first AddRight ,second HasRight
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbHasRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbHasRight001: SetConfig");
    std::string deviceName = "device_80";
    bool result = usbSrv_->HasRight(deviceName);
    ASSERT_TRUE(result);
}

/**
 * @tc.name: UsbHasRight003
 * @tc.desc: bool HasRight(const std::string deviceName)
 * @tc.desc: Negative test: first AddRight ,second HasRight
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, UsbHasRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbHasRight003: SetConfig");
    std::string deviceName = "device_80";
    bool result = usbSrv_->HasRight(deviceName);
    ASSERT_TRUE(result);

    int32_t ret = usbSrv_->RemoveRight(deviceName);
    ASSERT_EQ(0, ret);

    deviceName = "device_81";
    result = usbSrv_->HasRight(deviceName);
    ASSERT_TRUE(result);
}

/**
 * @tc.name: GetPorts001
 * @tc.desc: Test functions to GetPorts
 * @tc.desc: int32_t GetPorts(std::vector<UsbPort *> &usbports);
 * @tc.desc: positive：the parameters are correct
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, GetPorts001, TestSize.Level1)
{
    std::vector<UsbPort> portList;
    auto ports = usbSrv_->GetPorts(portList);
    USB_HILOGD(MODULE_USB_SERVICE, "Get UsbPort size=%{public}zu", portList.size());
    ASSERT_EQ(0, ports);
}

/**
 * @tc.name: GetSupportedModes001
 * @tc.desc: Test functions to GetSupportedModes
 * @tc.desc: int32_t GetSupportedModes(int32_t portId, int32_t &result);
 * @tc.desc: nagative: portid eroor
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, GetSupportedModes001, TestSize.Level1)
{
    int32_t result;
    auto modes = usbSrv_->GetSupportedModes(UsbSrvSupport::PORT_MODE_NONE, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::GetSupportedModes001 status=%{public}d", modes);
    ASSERT_NE(modes, 0);
}

/**
 * @tc.name: GetSupportedModes002
 * @tc.desc: Test functions to GetSupportedModes
 * @tc.desc: int32_t GetSupportedModes(int32_t portId, int32_t &result);
 * @tc.desc: nagative：portid eroor
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, GetSupportedModes002, TestSize.Level1)
{
    int32_t result;
    auto modes = usbSrv_->GetSupportedModes(USB_PORT_ID_INVALID, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::GetSupportedModes004 status=%{public}d", modes);
    ASSERT_NE(modes, 0);
}

/**
 * @tc.name: GetSupportedModes003
 * @tc.desc: Test functions to GetSupportedModes
 * @tc.desc: int32_t GetSupportedModes(int32_t portId, int32_t &result);
 * @tc.desc: nagative：portid eroor
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, GetSupportedModes003, TestSize.Level1)
{
    int32_t result = 0;
    auto modes = usbSrv_->GetSupportedModes(0xFFFFFFFF, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::GetSupportedModes004 status=%{public}d", modes);
    ASSERT_NE(modes, 0);
}

/**
 * @tc.name: GetSupportedModes004
 * @tc.desc: Test functions to GetSupportedModes
 * @tc.desc: int32_t GetSupportedModes(int32_t portId, int32_t &result);
 * @tc.desc: positive：the parameters are correct
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, GetSupportedModes004, TestSize.Level1)
{
    int32_t result;
    auto modes = usbSrv_->GetSupportedModes(UsbSrvSupport::PORT_MODE_DEVICE, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::GetSupportedModes004 status=%{public}d", modes);
    ASSERT_EQ(0, modes);
}

/**
 * @tc.name: SetPortRole001
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: int32_t SetPortRole(int32_t portId,int32_t powerRole,int32_t dataRole);
 * @tc.desc: positive：the parameters are correct
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetPortRole001, TestSize.Level1)
{
    auto ret = usbSrv_->SetPortRole(
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetPortRole001 status=%{public}d", ret);
    ASSERT_EQ(0, ret);
}

/**
 * @tc.name: SetPortRole002
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: int32_t SetPortRole(int32_t portId,int32_t powerRole,int32_t dataRole);
 * @tc.desc: nagative：portid error
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetPortRole002, TestSize.Level1)
{
    auto ret = usbSrv_->SetPortRole(
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetPortRole002 status=%{public}d", ret);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole003
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: int32_t SetPortRole(int32_t portId,int32_t powerRole,int32_t dataRole);
 * @tc.desc: nagative：powerRole error
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetPortRole003, TestSize.Level1)
{
    auto ret = usbSrv_->SetPortRole(
        UsbSrvSupport::PORT_MODE_DEVICE, USB_POWER_ROLE_INVALID, UsbSrvSupport::DATA_ROLE_DEVICE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetPortRole003 status=%{public}d", ret);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole004
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: int32_t SetPortRole(int32_t portId,int32_t powerRole,int32_t dataRole);
 * @tc.desc: nagative：dataRole error
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetPortRole004, TestSize.Level1)
{
    auto ret = usbSrv_->SetPortRole(
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SOURCE, USB_DATA_ROLE_INVALID);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetPortRole004 status=%{public}d", ret);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole005
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: int32_t SetPortRole(int32_t portId,int32_t powerRole,int32_t dataRole);
 * @tc.desc: nagative：powerRole、dataRole error
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetPortRole005, TestSize.Level1)
{
    auto ret =
        usbSrv_->SetPortRole(UsbSrvSupport::PORT_MODE_DEVICE, USB_POWER_ROLE_INVALID, USB_DATA_ROLE_INVALID);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetPortRole005 status=%{public}d", ret);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole006
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: int32_t SetPortRole(int32_t portId,int32_t powerRole,int32_t dataRole);
 * @tc.desc: nagative：powerRole、dataRole error
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetPortRole006, TestSize.Level1)
{
    auto ret = usbSrv_->SetPortRole(USB_PORT_ID_INVALID, UsbSrvSupport::POWER_ROLE_SOURCE, USB_DATA_ROLE_INVALID);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetPortRole006 status=%{public}d", ret);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole007
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: int32_t SetPortRole(int32_t portId,int32_t powerRole,int32_t dataRole);
 * @tc.desc: nagative：powerRole、dataRole error
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetPortRole007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole007 : SetPortRole");
    auto ret = usbSrv_->SetPortRole(UsbSrvSupport::PORT_MODE_HOST, USB_POWER_ROLE_INVALID, USB_DATA_ROLE_INVALID);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetPortRole007 status=%{public}d", ret);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: SetPortRole008
 * @tc.desc: Test functions to SetPortRole
 * @tc.desc: int32_t SetPortRole(int32_t portId,int32_t powerRole,int32_t dataRole);
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(UsbCoreMockTest, SetPortRole008, TestSize.Level1)
{
    auto ret = usbSrv_->SetPortRole(
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::DATA_ROLE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbCoreMockTest::SetPortRole008 status=%{public}d", ret);
    ASSERT_EQ(0, ret);
}
} // namespace USB
} // namespace OHOS
