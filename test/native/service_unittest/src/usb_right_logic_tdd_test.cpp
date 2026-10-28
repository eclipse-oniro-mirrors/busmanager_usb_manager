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
#include <cstdint>
#include <string>

#include "usb_right_logic_tdd_test.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

namespace {

constexpr int64_t USB_RIGHT_VALID_PERIOD_MIN = 0;
constexpr int64_t USB_RIGHT_VALID_PERIOD_MAX = 0xFFFFFFFFL;
constexpr uint64_t USB_RIGHT_VALID_PERIOD_SET = 0x0FFFFFFFL;
constexpr uint32_t TEST_PRIMARY_KEY_ID = 1;
constexpr int32_t TEST_UID_DEFAULT = 100;
constexpr uint64_t DEFAULT_INSTALL_TIME = 1000;
constexpr uint64_t DEFAULT_UPDATE_TIME = 1000;

UsbRightAppInfo MakeInfo(uint64_t requestTime, uint64_t validPeriod,
    uint64_t installTime = DEFAULT_INSTALL_TIME, uint64_t updateTime = DEFAULT_UPDATE_TIME)
{
    UsbRightAppInfo info = {};
    info.primaryKeyId = TEST_PRIMARY_KEY_ID;
    info.uid = TEST_UID_DEFAULT;
    info.installTime = installTime;
    info.updateTime = updateTime;
    info.requestTime = requestTime;
    info.validPeriod = validPeriod;
    return info;
}

} // namespace

class UsbRightLogicTddTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        USB_HILOGI(MODULE_USB_HOST, "UsbRightLogicTddTest::SetUpTestCase enter");
    }
    static void TearDownTestCase(void)
    {
        USB_HILOGI(MODULE_USB_HOST, "UsbRightLogicTddTest::TearDownTestCase enter");
    }
    void SetUp(void)
    {
        USB_HILOGI(MODULE_USB_HOST, "UsbRightLogicTddTest::SetUp enter");
    }
    void TearDown(void)
    {
        USB_HILOGI(MODULE_USB_HOST, "UsbRightLogicTddTest::TearDown enter");
    }
};

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ValidPeriodMin_ReturnsFalse, TestSize.Level1)
{
    auto info = MakeInfo(1000, USB_RIGHT_VALID_PERIOD_MIN);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 2000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ValidPeriodMin_ZeroRequestTime_ReturnsFalse, TestSize.Level1)
{
    auto info = MakeInfo(0, USB_RIGHT_VALID_PERIOD_MIN);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 0));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ValidPeriodMin_FutureTime_ReturnsFalse, TestSize.Level1)
{
    auto info = MakeInfo(5000, USB_RIGHT_VALID_PERIOD_MIN);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 1000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ValidPeriodMax_ReturnsFalse, TestSize.Level1)
{
    auto info = MakeInfo(1000, USB_RIGHT_VALID_PERIOD_MAX);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 2000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ValidPeriodMax_FarFuture_ReturnsFalse, TestSize.Level1)
{
    auto info = MakeInfo(1000, USB_RIGHT_VALID_PERIOD_MAX);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, UINT64_MAX));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ExpiredTimeBeforeRequestTime_ReturnsTrue, TestSize.Level1)
{
    auto info = MakeInfo(5000, 1000);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 4000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ExpiredTimeEqualsRequestTime_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 1000);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 1000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_WithinValidPeriod_ReturnsFalse, TestSize.Level1)
{
    auto info = MakeInfo(1000, 5000);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 3000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ExactlyAtExpiry_ReturnsTrue, TestSize.Level1)
{
    auto info = MakeInfo(1000, 5000);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 6000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_OneSecondBeforeExpiry_ReturnsFalse, TestSize.Level1)
{
    auto info = MakeInfo(1000, 5000);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 5999));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_OneSecondAfterExpiry_ReturnsTrue, TestSize.Level1)
{
    auto info = MakeInfo(1000, 5000);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 6001));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_InstallTimeAfterUpdateTime_ReturnsTrue, TestSize.Level1)
{
    auto info = MakeInfo(1000, 5000, 2000, 1000);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 7000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_InstallTimeAfterRequestTime_ReturnsTrue, TestSize.Level1)
{
    auto info = MakeInfo(1000, 5000, 2000, 3000);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 7000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_InstallTimeEqualsUpdateTime_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 5000, 1000, 1000);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 3000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_InstallTimeBeforeUpdateTime_ValidPeriod_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 5000, 500, 800);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 3000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_InstallTimeBeforeUpdateTime_Expired_ReturnsTrue, TestSize.Level1)
{
    auto info = MakeInfo(1000, 5000, 500, 800);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 7000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ValidPeriodSet_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(1000, USB_RIGHT_VALID_PERIOD_SET);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 1000 + USB_RIGHT_VALID_PERIOD_SET - 1));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ValidPeriodSet_Expired, TestSize.Level1)
{
    auto info = MakeInfo(1000, USB_RIGHT_VALID_PERIOD_SET);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 1000 + USB_RIGHT_VALID_PERIOD_SET));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ValidPeriodOne_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 1);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 1000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ValidPeriodOne_Expired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 1);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 1001));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ZeroExpiredTime_ZeroRequestTime_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(0, USB_RIGHT_VALID_PERIOD_MIN);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 0));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ZeroExpiredTime_NonZeroRequestTime_ReturnsTrue, TestSize.Level1)
{
    auto info = MakeInfo(1000, 5000);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 0));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ZeroValidPeriod_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 0);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 999999));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_MaxValidPeriod_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 0xFFFFFFFFL);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, UINT64_MAX));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ValidPeriodTwo_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 2);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 1001));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_ValidPeriodTwo_Expired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 2);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 1002));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_LargeValidPeriod_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 100000);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 50000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_LargeValidPeriod_Expired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 100000);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 101000));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_LargeValidPeriod_OneBeforeExpiry_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(1000, 100000);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 100999));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_AllTimesZero_ValidPeriodMin_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(0, 0, 0, 0);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 0));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_AllTimesZero_ValidPeriodMax_NotExpired, TestSize.Level1)
{
    auto info = MakeInfo(0, USB_RIGHT_VALID_PERIOD_MAX, 0, 0);
    EXPECT_FALSE(UsbRightDbHelper::IsRecordExpired(info, 0));
}

HWTEST_F(UsbRightLogicTddTest, IsRecordExpired_AllTimesZero_ValidPeriodOne_Expired, TestSize.Level1)
{
    auto info = MakeInfo(0, 1, 0, 0);
    EXPECT_TRUE(UsbRightDbHelper::IsRecordExpired(info, 0));
}

class UsbSrvSupportTddTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbSrvSupportTddTest::SetUpTestCase enter");
    }
    static void TearDownTestCase(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbSrvSupportTddTest::TearDownTestCase enter");
    }
    void SetUp(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbSrvSupportTddTest::SetUp enter");
    }
    void TearDown(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbSrvSupportTddTest::TearDown enter");
    }
};

HWTEST_F(UsbSrvSupportTddTest, Function_None_IsZero, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::Function::FUNCTION_NONE, 0u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Acm_IsOne, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::Function::FUNCTION_ACM, 1u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Ecm_IsTwo, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::Function::FUNCTION_ECM, 2u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Hdc_IsFour, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::Function::FUNCTION_HDC, 4u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Mtp_IsEight, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::Function::FUNCTION_MTP, 8u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Ptp_IsSixteen, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::Function::FUNCTION_PTP, 16u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Rndis_IsThirtyTwo, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::Function::FUNCTION_RNDIS, 32u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Ncm_Is256, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::Function::FUNCTION_NCM, 256u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Storage_Is512, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::Function::FUNCTION_STORAGE, 512u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_DevModeAuth_Is4096, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::Function::FUNCTION_DEVMODE_AUTH, 4096u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_AcmOrEcm_Equals3, TestSize.Level1)
{
    uint32_t result = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM;
    EXPECT_EQ(result, 3u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_MtpOrPtp_Equals24, TestSize.Level1)
{
    uint32_t result = UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_PTP;
    EXPECT_EQ(result, 24u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_AllSettable_Equals831, TestSize.Level1)
{
    uint32_t all = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM | UsbSrvSupport::FUNCTION_HDC |
                   UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_PTP | UsbSrvSupport::FUNCTION_RNDIS |
                   UsbSrvSupport::FUNCTION_NCM | UsbSrvSupport::FUNCTION_STORAGE;
    EXPECT_EQ(all, 831u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_AllIncludingDevModeAuth_Equals4927, TestSize.Level1)
{
    uint32_t all = 831u | UsbSrvSupport::FUNCTION_DEVMODE_AUTH;
    EXPECT_EQ(all, 4927u);
}

HWTEST_F(UsbSrvSupportTddTest, PowerRole_None_IsZero, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int32_t>(UsbSrvSupport::PowerRole::NONE), 0);
}

HWTEST_F(UsbSrvSupportTddTest, PowerRole_Source_IsOne, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int32_t>(UsbSrvSupport::PowerRole::SOURCE), 1);
}

HWTEST_F(UsbSrvSupportTddTest, PowerRole_Sink_IsTwo, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int32_t>(UsbSrvSupport::PowerRole::SINK), 2);
}

HWTEST_F(UsbSrvSupportTddTest, DataRole_None_IsZero, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int32_t>(UsbSrvSupport::DataRole::NONE), 0);
}

HWTEST_F(UsbSrvSupportTddTest, DataRole_Host_IsOne, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int32_t>(UsbSrvSupport::DataRole::HOST), 1);
}

HWTEST_F(UsbSrvSupportTddTest, DataRole_Device_IsTwo, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int32_t>(UsbSrvSupport::DataRole::DEVICE), 2);
}

HWTEST_F(UsbSrvSupportTddTest, PortMode_None_IsZero, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int32_t>(UsbSrvSupport::PortMode::NONE), 0);
}

HWTEST_F(UsbSrvSupportTddTest, PortMode_Device_IsOne, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int32_t>(UsbSrvSupport::PortMode::DEVICE), 1);
}

HWTEST_F(UsbSrvSupportTddTest, PortMode_Host_IsTwo, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int32_t>(UsbSrvSupport::PortMode::HOST), 2);
}

HWTEST_F(UsbSrvSupportTddTest, FunctionName_None_IsNone, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::FUNCTION_NAME_NONE, "none");
}

HWTEST_F(UsbSrvSupportTddTest, FunctionName_Hdc_IsHdc, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::FUNCTION_NAME_HDC, "hdc");
}

HWTEST_F(UsbSrvSupportTddTest, FunctionName_Acm_IsAcm, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::FUNCTION_NAME_ACM, "acm");
}

HWTEST_F(UsbSrvSupportTddTest, FunctionName_Ecm_IsEcm, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::FUNCTION_NAME_ECM, "ecm");
}

HWTEST_F(UsbSrvSupportTddTest, FunctionName_Mtp_IsMtp, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::FUNCTION_NAME_MTP, "mtp");
}

HWTEST_F(UsbSrvSupportTddTest, FunctionName_Ptp_IsPtp, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::FUNCTION_NAME_PTP, "ptp");
}

HWTEST_F(UsbSrvSupportTddTest, FunctionName_Rndis_IsRndis, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::FUNCTION_NAME_RNDIS, "rndis");
}

HWTEST_F(UsbSrvSupportTddTest, FunctionName_Ncm_IsNcm, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::FUNCTION_NAME_NCM, "ncm");
}

HWTEST_F(UsbSrvSupportTddTest, FunctionName_Storage_IsStorage, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::FUNCTION_NAME_STORAGE, "storage");
}

HWTEST_F(UsbSrvSupportTddTest, FunctionName_DevModeAuth_IsDevModeAuth, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::FUNCTION_NAME_DEVMODE_AUTH, "devmode_auth");
}

HWTEST_F(UsbSrvSupportTddTest, Connected_IsConnected, TestSize.Level1)
{
    EXPECT_EQ(UsbSrvSupport::CONNECTED, "connected");
}

HWTEST_F(UsbSrvSupportTddTest, Function_DevModeAuth_NotInSettableMask, TestSize.Level1)
{
    uint32_t settable = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM | UsbSrvSupport::FUNCTION_HDC |
                        UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_PTP | UsbSrvSupport::FUNCTION_RNDIS |
                        UsbSrvSupport::FUNCTION_NCM | UsbSrvSupport::FUNCTION_STORAGE;
    EXPECT_EQ((settable & UsbSrvSupport::FUNCTION_DEVMODE_AUTH), 0u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_None_AND_Acm_IsZero, TestSize.Level1)
{
    EXPECT_EQ((UsbSrvSupport::FUNCTION_NONE & UsbSrvSupport::FUNCTION_ACM), 0u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Acm_AND_Acm_IsAcm, TestSize.Level1)
{
    EXPECT_EQ((UsbSrvSupport::FUNCTION_ACM & UsbSrvSupport::FUNCTION_ACM), UsbSrvSupport::FUNCTION_ACM);
}

HWTEST_F(UsbSrvSupportTddTest, Function_None_OR_Acm_IsAcm, TestSize.Level1)
{
    EXPECT_EQ((UsbSrvSupport::FUNCTION_NONE | UsbSrvSupport::FUNCTION_ACM), UsbSrvSupport::FUNCTION_ACM);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Acm_XOR_Acm_IsZero, TestSize.Level1)
{
    EXPECT_EQ((UsbSrvSupport::FUNCTION_ACM ^ UsbSrvSupport::FUNCTION_ACM), 0u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_AllSettable_NOT_IsComplement, TestSize.Level1)
{
    uint32_t all = 831u;
    EXPECT_EQ((~all & UsbSrvSupport::FUNCTION_DEVMODE_AUTH), UsbSrvSupport::FUNCTION_DEVMODE_AUTH);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Storage_Is512Not831, TestSize.Level1)
{
    EXPECT_NE(UsbSrvSupport::FUNCTION_STORAGE, 831u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Ncm_Is256Not831, TestSize.Level1)
{
    EXPECT_NE(UsbSrvSupport::FUNCTION_NCM, 831u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_AcmOrEcmOrHcd_Equals7, TestSize.Level1)
{
    uint32_t result = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM | UsbSrvSupport::FUNCTION_HDC;
    EXPECT_EQ(result, 7u);
}

HWTEST_F(UsbSrvSupportTddTest, PowerRole_None_NotEqual_Source, TestSize.Level1)
{
    EXPECT_NE(static_cast<int32_t>(UsbSrvSupport::PowerRole::NONE),
              static_cast<int32_t>(UsbSrvSupport::PowerRole::SOURCE));
}

HWTEST_F(UsbSrvSupportTddTest, DataRole_Host_NotEqual_Device, TestSize.Level1)
{
    EXPECT_NE(static_cast<int32_t>(UsbSrvSupport::DataRole::HOST),
              static_cast<int32_t>(UsbSrvSupport::DataRole::DEVICE));
}

HWTEST_F(UsbSrvSupportTddTest, PortMode_Device_NotEqual_Host, TestSize.Level1)
{
    EXPECT_NE(static_cast<int32_t>(UsbSrvSupport::PortMode::DEVICE),
              static_cast<int32_t>(UsbSrvSupport::PortMode::HOST));
}

HWTEST_F(UsbSrvSupportTddTest, Function_DevModeAuth_IsPowerOfTwo, TestSize.Level1)
{
    uint32_t val = UsbSrvSupport::FUNCTION_DEVMODE_AUTH;
    EXPECT_EQ((val & (val - 1)), 0u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Ncm_IsPowerOfTwo, TestSize.Level1)
{
    uint32_t val = UsbSrvSupport::FUNCTION_NCM;
    EXPECT_EQ((val & (val - 1)), 0u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Storage_IsPowerOfTwo, TestSize.Level1)
{
    uint32_t val = UsbSrvSupport::FUNCTION_STORAGE;
    EXPECT_EQ((val & (val - 1)), 0u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_Acm_IsPowerOfTwo, TestSize.Level1)
{
    uint32_t val = UsbSrvSupport::FUNCTION_ACM;
    EXPECT_EQ((val & (val - 1)), 0u);
}

HWTEST_F(UsbSrvSupportTddTest, Function_None_IsNotPowerOfTwo, TestSize.Level1)
{
    uint32_t val = UsbSrvSupport::FUNCTION_NONE;
    EXPECT_EQ(val, 0u);
}

class UsbdDeviceActionTddTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbdDeviceActionTddTest::SetUpTestCase enter");
    }
    static void TearDownTestCase(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbdDeviceActionTddTest::TearDownTestCase enter");
    }
    void SetUp(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbdDeviceActionTddTest::SetUp enter");
    }
    void TearDown(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbdDeviceActionTddTest::TearDown enter");
    }
};

HWTEST_F(UsbdDeviceActionTddTest, ACT_DEVUP_IsZero, TestSize.Level1)
{
    EXPECT_EQ(ACT_DEVUP, 0);
}

HWTEST_F(UsbdDeviceActionTddTest, ACT_DEVDOWN_IsOne, TestSize.Level1)
{
    EXPECT_EQ(ACT_DEVDOWN, 1);
}

HWTEST_F(UsbdDeviceActionTddTest, ACT_UPDEVICE_IsTwo, TestSize.Level1)
{
    EXPECT_EQ(ACT_UPDEVICE, 2);
}

HWTEST_F(UsbdDeviceActionTddTest, ACT_DOWNDEVICE_IsThree, TestSize.Level1)
{
    EXPECT_EQ(ACT_DOWNDEVICE, 3);
}

HWTEST_F(UsbdDeviceActionTddTest, ACT_ACCESSORYUP_IsFour, TestSize.Level1)
{
    EXPECT_EQ(ACT_ACCESSORYUP, 4);
}

HWTEST_F(UsbdDeviceActionTddTest, ACT_ACCESSORYDOWN_IsFive, TestSize.Level1)
{
    EXPECT_EQ(ACT_ACCESSORYDOWN, 5);
}

HWTEST_F(UsbdDeviceActionTddTest, ACT_ACCESSORYSEND_IsSix, TestSize.Level1)
{
    EXPECT_EQ(ACT_ACCESSORYSEND, 6);
}

HWTEST_F(UsbdDeviceActionTddTest, ACT_CUSTOMCONTROLREQUEST_IsSeven, TestSize.Level1)
{
    EXPECT_EQ(ACT_CUSTOMCONTROLREQUEST, 7);
}

HWTEST_F(UsbdDeviceActionTddTest, ACT_DEVUP_NotEqual_ACT_DEVDOWN, TestSize.Level1)
{
    EXPECT_NE(ACT_DEVUP, ACT_DEVDOWN);
}

HWTEST_F(UsbdDeviceActionTddTest, ACT_UPDEVICE_NotEqual_ACT_DOWNDEVICE, TestSize.Level1)
{
    EXPECT_NE(ACT_UPDEVICE, ACT_DOWNDEVICE);
}

class UsbErrCodeTddTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbErrCodeTddTest::SetUpTestCase enter");
    }
    static void TearDownTestCase(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbErrCodeTddTest::TearDownTestCase enter");
    }
    void SetUp(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbErrCodeTddTest::SetUp enter");
    }
    void TearDown(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbErrCodeTddTest::TearDown enter");
    }
};

HWTEST_F(UsbErrCodeTddTest, UEC_OK_IsZero, TestSize.Level1)
{
    EXPECT_EQ(UEC_OK, 0);
}

HWTEST_F(UsbErrCodeTddTest, UEC_INTERFACE_INVALID_VALUE_NotZero, TestSize.Level1)
{
    EXPECT_NE(UEC_INTERFACE_INVALID_VALUE, 0);
}

HWTEST_F(UsbErrCodeTddTest, UEC_INTERFACE_PERMISSION_DENIED_NotZero, TestSize.Level1)
{
    EXPECT_NE(UEC_INTERFACE_PERMISSION_DENIED, 0);
}

HWTEST_F(UsbErrCodeTddTest, UEC_SERVICE_INVALID_VALUE_NotZero, TestSize.Level1)
{
    EXPECT_NE(UEC_SERVICE_INVALID_VALUE, 0);
}

HWTEST_F(UsbErrCodeTddTest, UEC_SERVICE_INVALID_VALUE_NotEqual_UEC_OK, TestSize.Level1)
{
    EXPECT_NE(UEC_SERVICE_INVALID_VALUE, UEC_OK);
}

HWTEST_F(UsbErrCodeTddTest, UEC_INTERFACE_INVALID_VALUE_NotEqual_UEC_SERVICE_INVALID_VALUE, TestSize.Level1)
{
    EXPECT_NE(UEC_INTERFACE_INVALID_VALUE, UEC_SERVICE_INVALID_VALUE);
}

HWTEST_F(UsbErrCodeTddTest, USB_MODULE_TYPE_SERVICE_IsZero, TestSize.Level1)
{
    EXPECT_EQ(USB_MODULE_TYPE_SERVICE, 0);
}

HWTEST_F(UsbErrCodeTddTest, USB_MODULE_TYPE_KIT_IsOne, TestSize.Level1)
{
    EXPECT_EQ(USB_MODULE_TYPE_KIT, 1);
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS
