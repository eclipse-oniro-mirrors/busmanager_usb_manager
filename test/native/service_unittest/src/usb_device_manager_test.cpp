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

#include "usb_device_manager_test.h"
#include <csignal>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>

#include "hilog_wrapper.h"
#include "usb_common_test.h"
#include "usb_srv_support.h"

using namespace testing::ext;
using namespace OHOS::USB;
using namespace OHOS;
using namespace std;

namespace OHOS {
namespace USB {

void UsbDeviceManagerTest::SetUp(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerTest SetUp");
    deviceManager_ = std::make_shared<UsbDeviceManager>();
}

void UsbDeviceManagerTest::TearDown(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerTest TearDown");
    deviceManager_ = nullptr;
}

void UsbDeviceManagerFunctionTest::SetUp(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerFunctionTest SetUp");
    deviceManager_ = std::make_shared<UsbDeviceManager>();
}

void UsbDeviceManagerFunctionTest::TearDown(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerFunctionTest TearDown");
    deviceManager_ = nullptr;
}

void UsbDeviceManagerConvertTest::SetUp(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerConvertTest SetUp");
}

void UsbDeviceManagerConvertTest::TearDown(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerConvertTest TearDown");
}

void UsbDeviceManagerEventTest::SetUp(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerEventTest SetUp");
    deviceManager_ = std::make_shared<UsbDeviceManager>();
}

void UsbDeviceManagerEventTest::TearDown(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerEventTest TearDown");
    deviceManager_ = nullptr;
}

void UsbDeviceManagerDumpTest::SetUp(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerDumpTest SetUp");
    deviceManager_ = std::make_shared<UsbDeviceManager>();
}

void UsbDeviceManagerDumpTest::TearDown(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerDumpTest TearDown");
    deviceManager_ = nullptr;
}

/**
 * @tc.name: UsbDeviceManagerConstructor001
 * @tc.desc: Test UsbDeviceManager constructor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerTest, UsbDeviceManagerConstructor001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDeviceManagerConstructor001");
    ASSERT_NE(deviceManager_, nullptr);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbDeviceManagerConstructor001");
}

/**
 * @tc.name: GetCurrentFunctions001
 * @tc.desc: Test GetCurrentFunctions with valid reference parameter
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerTest, GetCurrentFunctions001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetCurrentFunctions001");
    int32_t funcs = 0;
    int32_t ret = deviceManager_->GetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "GetCurrentFunctions ret=%{public}d, funcs=%{public}d", ret, funcs);
    ASSERT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetCurrentFunctions001");
}

/**
 * @tc.name: GetCurrentFunctions002
 * @tc.desc: Test GetCurrentFunctions method without parameters
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerTest, GetCurrentFunctions002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetCurrentFunctions002");
    int32_t funcs = deviceManager_->GetCurrentFunctions();
    USB_HILOGI(MODULE_USB_SERVICE, "GetCurrentFunctions funcs=%{public}d", funcs);
    ASSERT_GE(funcs, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetCurrentFunctions002");
}

/**
 * @tc.name: IsSettableFunctions001
 * @tc.desc: Test IsSettableFunctions with FUNCTION_NONE
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions001");
    bool result = UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_NONE);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions001");
}

/**
 * @tc.name: IsSettableFunctions002
 * @tc.desc: Test IsSettableFunctions with FUNCTION_HDC
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions002");
    bool result = UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_HDC);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions002");
}

/**
 * @tc.name: IsSettableFunctions003
 * @tc.desc: Test IsSettableFunctions with FUNCTION_ACM
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions003");
    bool result = UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_ACM);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions003");
}

/**
 * @tc.name: IsSettableFunctions004
 * @tc.desc: Test IsSettableFunctions with FUNCTION_ECM
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions004");
    bool result = UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_ECM);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions004");
}

/**
 * @tc.name: IsSettableFunctions005
 * @tc.desc: Test IsSettableFunctions with FUNCTION_MTP
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions005");
    bool result = UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_MTP);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions005");
}

/**
 * @tc.name: IsSettableFunctions006
 * @tc.desc: Test IsSettableFunctions with FUNCTION_PTP
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions006");
    bool result = UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_PTP);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions006");
}

/**
 * @tc.name: IsSettableFunctions007
 * @tc.desc: Test IsSettableFunctions with combined functions
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions007");
    int32_t funcs = UsbSrvSupport::FUNCTION_HDC | UsbSrvSupport::FUNCTION_ACM;
    bool result = UsbDeviceManager::IsSettableFunctions(funcs);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions007");
}

/**
 * @tc.name: IsSettableFunctions008
 * @tc.desc: Test IsSettableFunctions with invalid function
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions008");
    int32_t funcs = 0x80000000;
    bool result = UsbDeviceManager::IsSettableFunctions(funcs);
    ASSERT_FALSE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions008");
}

/**
 * @tc.name: ConvertFromString001
 * @tc.desc: Test ConvertFromString with FUNCTION_NAME_NONE
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString001");
    uint32_t result = UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_NONE);
    ASSERT_EQ(result, UsbSrvSupport::FUNCTION_NONE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString001");
}

/**
 * @tc.name: ConvertFromString002
 * @tc.desc: Test ConvertFromString with FUNCTION_NAME_ACM
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString002");
    uint32_t result = UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_ACM);
    ASSERT_EQ(result, UsbSrvSupport::FUNCTION_ACM);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString002");
}

/**
 * @tc.name: ConvertFromString003
 * @tc.desc: Test ConvertFromString with FUNCTION_NAME_ECM
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString003");
    uint32_t result = UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_ECM);
    ASSERT_EQ(result, UsbSrvSupport::FUNCTION_ECM);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString003");
}

/**
 * @tc.name: ConvertFromString004
 * @tc.desc: Test ConvertFromString with FUNCTION_NAME_HDC
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString004");
    uint32_t result = UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_HDC);
    ASSERT_EQ(result, UsbSrvSupport::FUNCTION_HDC);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString004");
}

/**
 * @tc.name: ConvertFromString005
 * @tc.desc: Test ConvertFromString with FUNCTION_NAME_MTP
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString005");
    uint32_t result = UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_MTP);
    ASSERT_EQ(result, UsbSrvSupport::FUNCTION_MTP);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString005");
}

/**
 * @tc.name: ConvertFromString006
 * @tc.desc: Test ConvertFromString with FUNCTION_NAME_PTP
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString006");
    uint32_t result = UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_PTP);
    ASSERT_EQ(result, UsbSrvSupport::FUNCTION_PTP);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString006");
}

/**
 * @tc.name: ConvertFromString007
 * @tc.desc: Test ConvertFromString with FUNCTION_NAME_RNDIS
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString007");
    uint32_t result = UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_RNDIS);
    ASSERT_EQ(result, UsbSrvSupport::FUNCTION_RNDIS);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString007");
}

/**
 * @tc.name: ConvertFromString008
 * @tc.desc: Test ConvertFromString with combined functions string
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString008");
    std::string funcs = std::string(UsbSrvSupport::FUNCTION_NAME_ACM) + "," +
                        std::string(UsbSrvSupport::FUNCTION_NAME_HDC);
    uint32_t result = UsbDeviceManager::ConvertFromString(funcs);
    uint32_t expected = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_HDC;
    ASSERT_EQ(result, expected);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString008");
}

/**
 * @tc.name: ConvertFromString009
 * @tc.desc: Test ConvertFromString with empty string
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString009");
    uint32_t result = UsbDeviceManager::ConvertFromString("");
    ASSERT_EQ(result, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString009");
}

/**
 * @tc.name: ConvertFromString010
 * @tc.desc: Test ConvertFromString with invalid function name
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString010");
    uint32_t result = UsbDeviceManager::ConvertFromString("invalid_function");
    ASSERT_EQ(result, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString010");
}

/**
 * @tc.name: ConvertFromString011
 * @tc.desc: Test ConvertFromString with combined valid and invalid functions
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString011, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString011");
    std::string funcs = std::string(UsbSrvSupport::FUNCTION_NAME_ACM) + ",invalid";
    uint32_t result = UsbDeviceManager::ConvertFromString(funcs);
    ASSERT_EQ(result, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString011");
}

/**
 * @tc.name: ConvertToString001
 * @tc.desc: Test ConvertToString with FUNCTION_NONE
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString001");
    std::string result = UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_NONE);
    ASSERT_EQ(result, std::string(UsbSrvSupport::FUNCTION_NAME_NONE));
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString001");
}

/**
 * @tc.name: ConvertToString002
 * @tc.desc: Test ConvertToString with FUNCTION_HDC
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString002");
    std::string result = UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_HDC);
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result, std::string(UsbSrvSupport::FUNCTION_NAME_HDC));
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString002");
}

/**
 * @tc.name: ConvertToString003
 * @tc.desc: Test ConvertToString with FUNCTION_ACM
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString003");
    std::string result = UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_ACM);
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result, std::string(UsbSrvSupport::FUNCTION_NAME_ACM));
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString003");
}

/**
 * @tc.name: ConvertToString004
 * @tc.desc: Test ConvertToString with combined functions
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString004");
    uint32_t funcs = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_HDC;
    std::string result = UsbDeviceManager::ConvertToString(funcs);
    ASSERT_FALSE(result.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "result=%{public}s", result.c_str());
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString004");
}

/**
 * @tc.name: ConvertToString005
 * @tc.desc: Test ConvertToString with invalid function value
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString005");
    std::string result = UsbDeviceManager::ConvertToString(0);
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result, std::string(UsbSrvSupport::FUNCTION_NAME_NONE));
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString005");
}

/**
 * @tc.name: UpdateFunctions001
 * @tc.desc: Test UpdateFunctions with same function value
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions001");
    int32_t funcs = deviceManager_->GetCurrentFunctions();
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions001");
}

/**
 * @tc.name: UpdateFunctions002
 * @tc.desc: Test UpdateFunctions with different function value
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions002");
    int32_t funcs = UsbSrvSupport::FUNCTION_ACM;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions002");
}

/**
 * @tc.name: SetPhyConnectState001
 * @tc.desc: Test SetPhyConnectState with true
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, SetPhyConnectState001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPhyConnectState001");
    deviceManager_->SetPhyConnectState(true);
    USB_HILOGI(MODULE_USB_SERVICE, "SetPhyConnectState to true successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetPhyConnectState001");
}

/**
 * @tc.name: SetPhyConnectState002
 * @tc.desc: Test SetPhyConnectState with false
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, SetPhyConnectState002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPhyConnectState002");
    deviceManager_->SetPhyConnectState(false);
    USB_HILOGI(MODULE_USB_SERVICE, "SetPhyConnectState to false successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetPhyConnectState002");
}

/**
 * @tc.name: IsGadgetConnected001
 * @tc.desc: Test IsGadgetConnected initial state
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, IsGadgetConnected001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsGadgetConnected001");
    bool result = deviceManager_->IsGadgetConnected();
    ASSERT_FALSE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsGadgetConnected001");
}

/**
 * @tc.name: SetChargeFlag001
 * @tc.desc: Test SetChargeFlag with true
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, SetChargeFlag001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetChargeFlag001");
    deviceManager_->SetChargeFlag(true);
    USB_HILOGI(MODULE_USB_SERVICE, "SetChargeFlag to true successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetChargeFlag001");
}

/**
 * @tc.name: SetChargeFlag002
 * @tc.desc: Test SetChargeFlag with false
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, SetChargeFlag002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetChargeFlag002");
    deviceManager_->SetChargeFlag(false);
    USB_HILOGI(MODULE_USB_SERVICE, "SetChargeFlag to false successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetChargeFlag002");
}

/**
 * @tc.name: HandleEvent001
 * @tc.desc: Test HandleEvent with ACT_UPDEVICE status
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, HandleEvent001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HandleEvent001");
    deviceManager_->HandleEvent(1); // ACT_UPDEVICE
    USB_HILOGI(MODULE_USB_SERVICE, "HandleEvent with ACT_UPDEVICE successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HandleEvent001");
}

/**
 * @tc.name: HandleEvent002
 * @tc.desc: Test HandleEvent with ACT_DOWNDEVICE status
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, HandleEvent002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HandleEvent002");
    deviceManager_->HandleEvent(2); // ACT_DOWNDEVICE
    USB_HILOGI(MODULE_USB_SERVICE, "HandleEvent with ACT_DOWNDEVICE successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HandleEvent002");
}

/**
 * @tc.name: HandleEvent003
 * @tc.desc: Test HandleEvent with ACT_ACCESSORYUP status
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, HandleEvent003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HandleEvent003");
    deviceManager_->HandleEvent(3); // ACT_ACCESSORYUP
    USB_HILOGI(MODULE_USB_SERVICE, "HandleEvent with ACT_ACCESSORYUP successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HandleEvent003");
}

/**
 * @tc.name: HandleEvent004
 * @tc.desc: Test HandleEvent with ACT_ACCESSORYDOWN status
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, HandleEvent004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HandleEvent004");
    deviceManager_->HandleEvent(4); // ACT_ACCESSORYDOWN
    USB_HILOGI(MODULE_USB_SERVICE, "HandleEvent with ACT_ACCESSORYDOWN successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HandleEvent004");
}

/**
 * @tc.name: HandleEvent005
 * @tc.desc: Test HandleEvent with invalid status
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, HandleEvent005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HandleEvent005");
    deviceManager_->HandleEvent(999); // Invalid status
    USB_HILOGI(MODULE_USB_SERVICE, "HandleEvent with invalid status completed");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HandleEvent005");
}

/**
 * @tc.name: UserChangeProcess001
 * @tc.desc: Test UserChangeProcess with current FUNCTIONS_HDC
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UserChangeProcess001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UserChangeProcess001");
    int32_t ret = deviceManager_->UserChangeProcess();
    USB_HILOGI(MODULE_USB_SERVICE, "UserChangeProcess ret=%{public}d", ret);
    ASSERT_EQ(ret, HDF_FAILURE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UserChangeProcess001");
}

/**
 * @tc.name: GetDumpHelp001
 * @tc.desc: Test GetDumpHelp outputs help information
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, GetDumpHelp001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDumpHelp001");
    int32_t fd = 1; // stdout
    deviceManager_->GetDumpHelp(fd);
    USB_HILOGI(MODULE_USB_SERVICE, "GetDumpHelp executed successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDumpHelp001");
}

/**
 * @tc.name: DumpGetSupportFunc001
 * @tc.desc: Test DumpGetSupportFunc outputs function list
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, DumpGetSupportFunc001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DumpGetSupportFunc001");
    int32_t fd = 1; // stdout
    deviceManager_->DumpGetSupportFunc(fd);
    USB_HILOGI(MODULE_USB_SERVICE, "DumpGetSupportFunc executed successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DumpGetSupportFunc001");
}

/**
 * @tc.name: Dump001
 * @tc.desc: Test Dump with -a argument
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump001");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device", "-a"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with -a executed successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump001");
}

/**
 * @tc.name: Dump002
 * @tc.desc: Test Dump with -f Q argument
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump002");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device", "-f", "Q"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with -f Q executed successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump002");
}

/**
 * @tc.name: Dump003
 * @tc.desc: Test Dump with invalid arguments count
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump003");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with invalid args executed");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump003");
}

/**
 * @tc.name: Dump004
 * @tc.desc: Test Dump with too many arguments
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump004");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device", "-a", "extra", "args"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with too many args executed");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump004");
}

/**
 * @tc.name: Dump005
 * @tc.desc: Test Dump with invalid command argument
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump005");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device", "-x", "test"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with invalid command executed");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump005");
}

/**
 * @tc.name: ConvertFromString012
 * @tc.desc: Test ConvertFromString with FUNCTION_NAME_NCM
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString012, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString012");
    uint32_t result = UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_NCM);
    ASSERT_EQ(result, UsbSrvSupport::FUNCTION_NCM);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString012");
}

/**
 * @tc.name: ConvertFromString013
 * @tc.desc: Test ConvertFromString with FUNCTION_NAME_STORAGE
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString013, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString013");
    uint32_t result = UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_STORAGE);
    ASSERT_EQ(result, UsbSrvSupport::FUNCTION_STORAGE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString013");
}

/**
 * @tc.name: ConvertToString006
 * @tc.desc: Test ConvertToString with FUNCTION_MTP
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString006");
    std::string result = UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_MTP);
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result, std::string(UsbSrvSupport::FUNCTION_NAME_MTP));
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString006");
}

/**
 * @tc.name: ConvertToString007
 * @tc.desc: Test ConvertToString with FUNCTION_STORAGE
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString007");
    std::string result = UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_STORAGE);
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result, std::string(UsbSrvSupport::FUNCTION_NAME_STORAGE));
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString007");
}

/**
 * @tc.name: ConvertToString008
 * @tc.desc: Test ConvertToString with FUNCTION_RNDIS
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString008");
    std::string result = UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_RNDIS);
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result, std::string(UsbSrvSupport::FUNCTION_NAME_RNDIS));
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString008");
}

/**
 * @tc.name: ConvertToString009
 * @tc.desc: Test ConvertToString with FUNCTION_NCM
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString009");
    std::string result = UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_NCM);
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result, std::string(UsbSrvSupport::FUNCTION_NAME_NCM));
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString009");
}

/**
 * @tc.name: IsSettableFunctions009
 * @tc.desc: Test IsSettableFunctions with FUNCTION_RNDIS
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions009");
    bool result = UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_RNDIS);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions009");
}

/**
 * @tc.name: IsSettableFunctions010
 * @tc.desc: Test IsSettableFunctions with FUNCTION_NCM
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions010");
    bool result = UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_NCM);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions010");
}

/**
 * @tc.name: IsSettableFunctions011
 * @tc.desc: Test IsSettableFunctions with FUNCTION_STORAGE
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions011, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions011");
    bool result = UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_STORAGE);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions011");
}

/**
 * @tc.name: ConvertFromString014
 * @tc.desc: Test ConvertFromString with three combined functions
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString014, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString014");
    std::string funcs = std::string(UsbSrvSupport::FUNCTION_NAME_ACM) + "," +
                        std::string(UsbSrvSupport::FUNCTION_NAME_HDC) + "," +
                        std::string(UsbSrvSupport::FUNCTION_NAME_MTP);
    uint32_t result = UsbDeviceManager::ConvertFromString(funcs);
    uint32_t expected = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_HDC |
                       UsbSrvSupport::FUNCTION_MTP;
    ASSERT_EQ(result, expected);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString014");
}

/**
 * @tc.name: ConvertFromString015
 * @tc.desc: Test ConvertFromString with malformed comma input
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString015, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString015");
    uint32_t result = UsbDeviceManager::ConvertFromString(",");
    ASSERT_EQ(result, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString015");
}

/**
 * @tc.name: ConvertToString010
 * @tc.desc: Test ConvertToString with all combined settable functions
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString010");
    uint32_t funcs = UsbSrvSupport::FUNCTION_HDC | UsbSrvSupport::FUNCTION_ACM |
                     UsbSrvSupport::FUNCTION_ECM | UsbSrvSupport::FUNCTION_MTP;
    std::string result = UsbDeviceManager::ConvertToString(funcs);
    ASSERT_FALSE(result.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "result=%{public}s", result.c_str());
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString010");
}

/**
 * @tc.name: UpdateFunctions003
 * @tc.desc: Test UpdateFunctions with FUNCTION_MTP
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions003");
    int32_t funcs = UsbSrvSupport::FUNCTION_MTP;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions003");
}

/**
 * @tc.name: UpdateFunctions004
 * @tc.desc: Test UpdateFunctions with combined ACM and HDC
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions004");
    int32_t funcs = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_HDC;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions004");
}

/**
 * @tc.name: HandleEvent006
 * @tc.desc: Test HandleEvent with ACT_ACCESSORYSEND status
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, HandleEvent006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HandleEvent006");
    deviceManager_->HandleEvent(5); // ACT_ACCESSORYSEND
    USB_HILOGI(MODULE_USB_SERVICE, "HandleEvent with ACT_ACCESSORYSEND successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HandleEvent006");
}

/**
 * @tc.name: GetCurrentFunctions003
 * @tc.desc: Test GetCurrentFunctions multiple times to check consistency
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerTest, GetCurrentFunctions003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetCurrentFunctions003");
    int32_t funcs1 = deviceManager_->GetCurrentFunctions();
    int32_t funcs2 = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(funcs1, funcs2);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetCurrentFunctions003");
}

/**
 * @tc.name: Dump006
 * @tc.desc: Test Dump with -f and numeric argument
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump006");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device", "-f", "4"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with -f 4 executed successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump006");
}

/**
 * @tc.name: ConvertFromString016
 * @tc.desc: Test ConvertFromString with FUNCTION_NAME_DEVMODE_AUTH
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString016, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString016");
    uint32_t result = UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_DEVMODE_AUTH);
    ASSERT_EQ(result, UsbSrvSupport::FUNCTION_DEVMODE_AUTH);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString016");
}

/**
 * @tc.name: ConvertToString011
 * @tc.desc: Test ConvertToString with FUNCTION_DEVMODE_AUTH
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString011, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString011");
    std::string result = UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_DEVMODE_AUTH);
    ASSERT_FALSE(result.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "result=%{public}s", result.c_str());
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString011");
}

/**
 * @tc.name: IsSettableFunctions012
 * @tc.desc: Test IsSettableFunctions with negative function value
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions012, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions012");
    bool result = UsbDeviceManager::IsSettableFunctions(-1);
    ASSERT_FALSE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions012");
}

/**
 * @tc.name: UpdateFunctions005
 * @tc.desc: Test UpdateFunctions with FUNCTION_NONE
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions005");
    int32_t funcs = UsbSrvSupport::FUNCTION_NONE;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions005");
}

/**
 * @tc.name: GetCurrentFunctions004
 * @tc.desc: Test GetCurrentFunctions with different initial values
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerTest, GetCurrentFunctions004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetCurrentFunctions004");
    deviceManager_->UpdateFunctions(UsbSrvSupport::FUNCTION_HDC);
    int32_t funcs1 = deviceManager_->GetCurrentFunctions();
    deviceManager_->UpdateFunctions(UsbSrvSupport::FUNCTION_ACM);
    int32_t funcs2 = deviceManager_->GetCurrentFunctions();
    ASSERT_NE(funcs1, funcs2);
    ASSERT_EQ(funcs2, UsbSrvSupport::FUNCTION_ACM);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetCurrentFunctions004");
}

/**
 * @tc.name: Dump007
 * @tc.desc: Test Dump with -f and invalid numeric argument
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump007");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device", "-f", "9999"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with -f 9999 executed");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump007");
}

/**
 * @tc.name: ConvertToString012
 * @tc.desc: Test ConvertToString with function value exceeding settable
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString012, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString012");
    uint32_t funcs = 0xFFFFFFFF;
    std::string result = UsbDeviceManager::ConvertToString(funcs);
    ASSERT_FALSE(result.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "result=%{public}s", result.c_str());
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString012");
}

/**
 * @tc.name: ConvertFromString017
 * @tc.desc: Test ConvertFromString with trailing comma
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString017, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString017");
    std::string funcs = std::string(UsbSrvSupport::FUNCTION_NAME_HDC) + ",";
    uint32_t result = UsbDeviceManager::ConvertFromString(funcs);
    uint32_t expected = UsbSrvSupport::FUNCTION_HDC;
    ASSERT_EQ(result, expected);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString017");
}

/**
 * @tc.name: ConvertFromString018
 * @tc.desc: Test ConvertFromString with leading comma
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString018, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString018");
    std::string funcs = "," + std::string(UsbSrvSupport::FUNCTION_NAME_ACM);
    uint32_t result = UsbDeviceManager::ConvertFromString(funcs);
    ASSERT_EQ(result, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString018");
}

/**
 * @tc.name: ConvertToString013
 * @tc.desc: Test ConvertToString with FUNCTION_PTP
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString013, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString013");
    std::string result = UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_PTP);
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result, std::string(UsbSrvSupport::FUNCTION_NAME_PTP));
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString013");
}

/**
 * @tc.name: ConvertToString014
 * @tc.desc: Test ConvertToString with FUNCTION_ECM
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString014, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString014");
    std::string result = UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_ECM);
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result, std::string(UsbSrvSupport::FUNCTION_NAME_ECM));
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString014");
}

/**
 * @tc.name: IsSettableFunctions013
 * @tc.desc: Test IsSettableFunctions with all settable functions combined
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions013, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions013");
    int32_t funcs = UsbSrvSupport::FUNCTION_HDC | UsbSrvSupport::FUNCTION_ACM |
                    UsbSrvSupport::FUNCTION_ECM | UsbSrvSupport::FUNCTION_MTP |
                    UsbSrvSupport::FUNCTION_PTP | UsbSrvSupport::FUNCTION_RNDIS |
                    UsbSrvSupport::FUNCTION_NCM | UsbSrvSupport::FUNCTION_STORAGE;
    bool result = UsbDeviceManager::IsSettableFunctions(funcs);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions013");
}

/**
 * @tc.name: IsSettableFunctions014
 * @tc.desc: Test IsSettableFunctions with zero
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions014, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions014");
    bool result = UsbDeviceManager::IsSettableFunctions(0);
    ASSERT_TRUE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions014");
}

/**
 * @tc.name: UpdateFunctions006
 * @tc.desc: Test UpdateFunctions with FUNCTION_PTP
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions006");
    int32_t funcs = UsbSrvSupport::FUNCTION_PTP;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions006");
}

/**
 * @tc.name: UpdateFunctions007
 * @tc.desc: Test UpdateFunctions with FUNCTION_STORAGE
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions007");
    int32_t funcs = UsbSrvSupport::FUNCTION_STORAGE;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions007");
}

/**
 * @tc.name: UpdateFunctions008
 * @tc.desc: Test UpdateFunctions with FUNCTION_RNDIS
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions008");
    int32_t funcs = UsbSrvSupport::FUNCTION_RNDIS;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions008");
}

/**
 * @tc.name: UpdateFunctions009
 * @tc.desc: Test UpdateFunctions with FUNCTION_NCM
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions009");
    int32_t funcs = UsbSrvSupport::FUNCTION_NCM;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions009");
}

/**
 * @tc.name: UpdateFunctions010
 * @tc.desc: Test UpdateFunctions with combined MTP and HDC
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions010");
    int32_t funcs = UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_HDC;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions010");
}

/**
 * @tc.name: UpdateFunctions011
 * @tc.desc: Test UpdateFunctions with combined PTP and HDC
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions011, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions011");
    int32_t funcs = UsbSrvSupport::FUNCTION_PTP | UsbSrvSupport::FUNCTION_HDC;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions011");
}

/**
 * @tc.name: UpdateFunctions012
 * @tc.desc: Test UpdateFunctions with combined ACM, ECM, and HDC
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions012, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions012");
    int32_t funcs = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM |
                    UsbSrvSupport::FUNCTION_HDC;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions012");
}

/**
 * @tc.name: HandleEvent007
 * @tc.desc: Test multiple HandleEvent calls in sequence
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, HandleEvent007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HandleEvent007");
    deviceManager_->HandleEvent(1); // ACT_UPDEVICE
    deviceManager_->HandleEvent(2); // ACT_DOWNDEVICE
    deviceManager_->HandleEvent(1); // ACT_UPDEVICE again
    USB_HILOGI(MODULE_USB_SERVICE, "Multiple HandleEvent calls completed");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HandleEvent007");
}

/**
 * @tc.name: SetPhyConnectState003
 * @tc.desc: Test SetPhyConnectState toggling between states
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, SetPhyConnectState003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPhyConnectState003");
    deviceManager_->SetPhyConnectState(true);
    deviceManager_->SetPhyConnectState(false);
    deviceManager_->SetPhyConnectState(true);
    USB_HILOGI(MODULE_USB_SERVICE, "SetPhyConnectState toggle completed");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetPhyConnectState003");
}

/**
 * @tc.name: SetChargeFlag003
 * @tc.desc: Test SetChargeFlag toggling between states
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, SetChargeFlag003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetChargeFlag003");
    deviceManager_->SetChargeFlag(true);
    deviceManager_->SetChargeFlag(false);
    deviceManager_->SetChargeFlag(true);
    USB_HILOGI(MODULE_USB_SERVICE, "SetChargeFlag toggle completed");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetChargeFlag003");
}

/**
 * @tc.name: UserChangeProcess002
 * @tc.desc: Test UserChangeProcess after function change
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UserChangeProcess002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UserChangeProcess002");
    deviceManager_->UpdateFunctions(UsbSrvSupport::FUNCTION_MTP);
    int32_t ret = deviceManager_->UserChangeProcess();
    USB_HILOGI(MODULE_USB_SERVICE, "UserChangeProcess ret=%{public}d", ret);
    ASSERT_EQ(ret, HDF_FAILURE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UserChangeProcess002");
}

/**
 * @tc.name: UserChangeProcess003
 * @tc.desc: Test UserChangeProcess with PTP function
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UserChangeProcess003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UserChangeProcess003");
    deviceManager_->UpdateFunctions(UsbSrvSupport::FUNCTION_PTP);
    int32_t ret = deviceManager_->UserChangeProcess();
    USB_HILOGI(MODULE_USB_SERVICE, "UserChangeProcess ret=%{public}d", ret);
    ASSERT_EQ(ret, HDF_FAILURE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UserChangeProcess003");
}

/**
 * @tc.name: Dump008
 * @tc.desc: Test Dump with -f and 0 argument
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump008");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device", "-f", "0"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with -f 0 executed successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump008");
}

/**
 * @tc.name: Dump009
 * @tc.desc: Test Dump with -f and 1 argument
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump009");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device", "-f", "1"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with -f 1 executed successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump009");
}

/**
 * @tc.name: Dump010
 * @tc.desc: Test Dump with -f and 2 argument
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump010");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device", "-f", "2"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with -f 2 executed successfully");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump010");
}

/**
 * @tc.name: ConvertFromString019
 * @tc.desc: Test ConvertFromString with four combined functions
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString019, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString019");
    std::string funcs = std::string(UsbSrvSupport::FUNCTION_NAME_ACM) + "," +
                        std::string(UsbSrvSupport::FUNCTION_NAME_ECM) + "," +
                        std::string(UsbSrvSupport::FUNCTION_NAME_HDC) + "," +
                        std::string(UsbSrvSupport::FUNCTION_NAME_MTP);
    uint32_t result = UsbDeviceManager::ConvertFromString(funcs);
    uint32_t expected = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM |
                       UsbSrvSupport::FUNCTION_HDC | UsbSrvSupport::FUNCTION_MTP;
    ASSERT_EQ(result, expected);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString019");
}

/**
 * @tc.name: ConvertFromString020
 * @tc.desc: Test ConvertFromString with whitespace in function name
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString020, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString020");
    std::string funcs = " " + std::string(UsbSrvSupport::FUNCTION_NAME_HDC);
    uint32_t result = UsbDeviceManager::ConvertFromString(funcs);
    ASSERT_EQ(result, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString020");
}

/**
 * @tc.name: ConvertToString015
 * @tc.desc: Test ConvertToString with combined MTP and PTP
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString015, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString015");
    uint32_t funcs = UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_PTP;
    std::string result = UsbDeviceManager::ConvertToString(funcs);
    ASSERT_FALSE(result.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "result=%{public}s", result.c_str());
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString015");
}

/**
 * @tc.name: ConvertToString016
 * @tc.desc: Test ConvertToString with combined STORAGE and HDC
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString016, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString016");
    uint32_t funcs = UsbSrvSupport::FUNCTION_STORAGE | UsbSrvSupport::FUNCTION_HDC;
    std::string result = UsbDeviceManager::ConvertToString(funcs);
    ASSERT_FALSE(result.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "result=%{public}s", result.c_str());
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString016");
}

/**
 * @tc.name: GetCurrentFunctions005
 * @tc.desc: Test GetCurrentFunctions after multiple function updates
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerTest, GetCurrentFunctions005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetCurrentFunctions005");
    deviceManager_->UpdateFunctions(UsbSrvSupport::FUNCTION_HDC);
    int32_t funcs1 = deviceManager_->GetCurrentFunctions();
    deviceManager_->UpdateFunctions(UsbSrvSupport::FUNCTION_ACM);
    int32_t funcs2 = deviceManager_->GetCurrentFunctions();
    deviceManager_->UpdateFunctions(UsbSrvSupport::FUNCTION_MTP);
    int32_t funcs3 = deviceManager_->GetCurrentFunctions();
    ASSERT_NE(funcs1, funcs2);
    ASSERT_NE(funcs2, funcs3);
    ASSERT_EQ(funcs3, UsbSrvSupport::FUNCTION_MTP);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetCurrentFunctions005");
}

/**
 * @tc.name: UpdateFunctions013
 * @tc.desc: Test UpdateFunctions with FUNCTION_DEVMODE_AUTH
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions013, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions013");
    int32_t funcs = UsbSrvSupport::FUNCTION_DEVMODE_AUTH;
    deviceManager_->UpdateFunctions(funcs);
    int32_t currentFuncs = deviceManager_->GetCurrentFunctions();
    ASSERT_EQ(currentFuncs, funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions013");
}

/**
 * @tc.name: ConvertToString017
 * @tc.desc: Test ConvertToString with FUNCTION_DEVMODE_AUTH and STORAGE
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString017, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString017");
    uint32_t funcs = UsbSrvSupport::FUNCTION_DEVMODE_AUTH | UsbSrvSupport::FUNCTION_STORAGE;
    std::string result = UsbDeviceManager::ConvertToString(funcs);
    ASSERT_FALSE(result.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "result=%{public}s", result.c_str());
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString017");
}

/**
 * @tc.name: IsGadgetConnected002
 * @tc.desc: Test IsGadgetConnected after event handling
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, IsGadgetConnected002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsGadgetConnected002");
    deviceManager_->HandleEvent(1); // ACT_UPDEVICE
    bool result = deviceManager_->IsGadgetConnected();
    USB_HILOGI(MODULE_USB_SERVICE, "IsGadgetConnected=%{public}d", result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsGadgetConnected002");
}

/**
 * @tc.name: Dump011
 * @tc.desc: Test Dump with -f and negative argument
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump011, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump011");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device", "-f", "-1"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with -f -1 executed");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump011");
}

/**
 * @tc.name: Dump012
 * @tc.desc: Test Dump with -f and very large numeric argument
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerDumpTest, Dump012, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Dump012");
    int32_t fd = 1; // stdout
    std::vector<std::string> args = {"usb_device", "-f", "2147483647"};
    deviceManager_->Dump(fd, args);
    USB_HILOGI(MODULE_USB_SERVICE, "Dump with -f 2147483647 executed");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Dump012");
}

/**
 * @tc.name: ConvertFromString021
 * @tc.desc: Test ConvertFromString with duplicate function names
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString021, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString021");
    std::string funcs = std::string(UsbSrvSupport::FUNCTION_NAME_HDC) + "," +
                        std::string(UsbSrvSupport::FUNCTION_NAME_HDC);
    uint32_t result = UsbDeviceManager::ConvertFromString(funcs);
    uint32_t expected = UsbSrvSupport::FUNCTION_HDC;
    ASSERT_EQ(result, expected);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString021");
}

/**
 * @tc.name: ConvertToString018
 * @tc.desc: Test ConvertToString with minimum function value
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString018, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString018");
    std::string result = UsbDeviceManager::ConvertToString(1);
    ASSERT_FALSE(result.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "result=%{public}s", result.c_str());
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString018");
}

/**
 * @tc.name: UpdateFunctions014
 * @tc.desc: Test UpdateFunctions sequence with various function combinations
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerFunctionTest, UpdateFunctions014, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UpdateFunctions014");
    deviceManager_->UpdateFunctions(UsbSrvSupport::FUNCTION_HDC);
    ASSERT_EQ(deviceManager_->GetCurrentFunctions(), UsbSrvSupport::FUNCTION_HDC);
    deviceManager_->UpdateFunctions(UsbSrvSupport::FUNCTION_ACM);
    ASSERT_EQ(deviceManager_->GetCurrentFunctions(), UsbSrvSupport::FUNCTION_ACM);
    deviceManager_->UpdateFunctions(UsbSrvSupport::FUNCTION_HDC | UsbSrvSupport::FUNCTION_ACM);
    ASSERT_EQ(deviceManager_->GetCurrentFunctions(),
              UsbSrvSupport::FUNCTION_HDC | UsbSrvSupport::FUNCTION_ACM);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UpdateFunctions014");
}

/**
 * @tc.name: IsSettableFunctions015
 * @tc.desc: Test IsSettableFunctions with partial invalid function mask
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, IsSettableFunctions015, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : IsSettableFunctions015");
    int32_t funcs = UsbSrvSupport::FUNCTION_HDC | 0x80000000;
    bool result = UsbDeviceManager::IsSettableFunctions(funcs);
    ASSERT_FALSE(result);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : IsSettableFunctions015");
}

/**
 * @tc.name: HandleEvent008
 * @tc.desc: Test HandleEvent with zero status
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerEventTest, HandleEvent008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HandleEvent008");
    deviceManager_->HandleEvent(0); // Invalid status
    USB_HILOGI(MODULE_USB_SERVICE, "HandleEvent with 0 status completed");
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HandleEvent008");
}

/**
 * @tc.name: GetCurrentFunctions006
 * @tc.desc: Test GetCurrentFunctions returns expected initial value
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerTest, GetCurrentFunctions006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetCurrentFunctions006");
    int32_t funcs = deviceManager_->GetCurrentFunctions();
    ASSERT_GE(funcs, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Initial functions=%{public}d", funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetCurrentFunctions006");
}

/**
 * @tc.name: ConvertFromString022
 * @tc.desc: Test ConvertFromString with FUNCTION_NAME_RNDIS
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertFromString022, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertFromString022");
    uint32_t result = UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_RNDIS);
    ASSERT_EQ(result, UsbSrvSupport::FUNCTION_RNDIS);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertFromString022");
}

/**
 * @tc.name: ConvertToString019
 * @tc.desc: Test ConvertToString with very large function value
 * @tc.type: FUNC
 */
HWTEST_F(UsbDeviceManagerConvertTest, ConvertToString019, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ConvertToString019");
    uint32_t funcs = 0xFFFFFFFE;
    std::string result = UsbDeviceManager::ConvertToString(funcs);
    ASSERT_FALSE(result.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "result=%{public}s", result.c_str());
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ConvertToString019");
}

} // namespace USB
} // namespace OHOS
