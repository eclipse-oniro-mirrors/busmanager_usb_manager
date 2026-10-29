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
#include <string>
#include <string_view>
#include <cstdint>

#include "usb_device_manager_static_tdd_test.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

class UsbDeviceManagerStaticTddTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerStaticTddTest::SetUpTestCase enter");
    }
    static void TearDownTestCase(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerStaticTddTest::TearDownTestCase enter");
    }
    void SetUp(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerStaticTddTest::SetUp enter");
    }
    void TearDown(void)
    {
        USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceManagerStaticTddTest::TearDown enter");
    }
};

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_None_ReturnsTrue, TestSize.Level1)
{
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_NONE));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_Zero_ReturnsTrue, TestSize.Level1)
{
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(0));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_Acm_ReturnsTrue, TestSize.Level1)
{
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_ACM));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_Ecm_ReturnsTrue, TestSize.Level1)
{
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_ECM));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_Hdc_ReturnsTrue, TestSize.Level1)
{
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_HDC));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_Mtp_ReturnsTrue, TestSize.Level1)
{
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_MTP));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_Ptp_ReturnsTrue, TestSize.Level1)
{
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_PTP));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_Rndis_ReturnsTrue, TestSize.Level1)
{
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_RNDIS));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_Ncm_ReturnsTrue, TestSize.Level1)
{
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_NCM));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_Storage_ReturnsTrue, TestSize.Level1)
{
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_STORAGE));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_AcmEcmCombined_ReturnsTrue, TestSize.Level1)
{
    int32_t combined = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM;
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(combined));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_HdcMtpStorageCombined_ReturnsTrue, TestSize.Level1)
{
    int32_t combined = UsbSrvSupport::FUNCTION_HDC | UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_STORAGE;
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(combined));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_AllSettable_ReturnsTrue, TestSize.Level1)
{
    int32_t all = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM | UsbSrvSupport::FUNCTION_HDC |
                  UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_PTP | UsbSrvSupport::FUNCTION_RNDIS |
                  UsbSrvSupport::FUNCTION_NCM | UsbSrvSupport::FUNCTION_STORAGE;
    EXPECT_TRUE(UsbDeviceManager::IsSettableFunctions(all));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_DevModeAuth_ReturnsFalse, TestSize.Level1)
{
    EXPECT_FALSE(UsbDeviceManager::IsSettableFunctions(UsbSrvSupport::FUNCTION_DEVMODE_AUTH));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_DevModeAuthBitNotSettable, TestSize.Level1)
{
    EXPECT_FALSE(UsbDeviceManager::IsSettableFunctions(static_cast<int32_t>(UsbSrvSupport::FUNCTION_DEVMODE_AUTH)));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_ExtraBit_ReturnsFalse, TestSize.Level1)
{
    int32_t withExtra = UsbSrvSupport::FUNCTION_ACM | (1 << 10);
    EXPECT_FALSE(UsbDeviceManager::IsSettableFunctions(withExtra));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_NegativeOne_ReturnsFalse, TestSize.Level1)
{
    EXPECT_FALSE(UsbDeviceManager::IsSettableFunctions(-1));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_HighBit_ReturnsFalse, TestSize.Level1)
{
    EXPECT_FALSE(UsbDeviceManager::IsSettableFunctions(static_cast<int32_t>(0x80000000)));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_AllBitsSet_ReturnsFalse, TestSize.Level1)
{
    EXPECT_FALSE(UsbDeviceManager::IsSettableFunctions(static_cast<int32_t>(0xFFFFFFFF)));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_AcmPlusDevModeAuth_ReturnsFalse, TestSize.Level1)
{
    int32_t combined = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_DEVMODE_AUTH;
    EXPECT_FALSE(UsbDeviceManager::IsSettableFunctions(combined));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, IsSettableFunctions_AllSettablePlusDevModeAuth_ReturnsFalse, TestSize.Level1)
{
    int32_t all = UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM | UsbSrvSupport::FUNCTION_HDC |
                  UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_PTP | UsbSrvSupport::FUNCTION_RNDIS |
                  UsbSrvSupport::FUNCTION_NCM | UsbSrvSupport::FUNCTION_STORAGE | UsbSrvSupport::FUNCTION_DEVMODE_AUTH;
    EXPECT_FALSE(UsbDeviceManager::IsSettableFunctions(all));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_None_ReturnsZero, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("none"), 0u);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_Acm_ReturnsOne, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("acm"), UsbSrvSupport::FUNCTION_ACM);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_Ecm_ReturnsTwo, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("ecm"), UsbSrvSupport::FUNCTION_ECM);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_Hdc_ReturnsFour, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("hdc"), UsbSrvSupport::FUNCTION_HDC);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_Mtp_ReturnsEight, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("mtp"), UsbSrvSupport::FUNCTION_MTP);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_Ptp_ReturnsSixteen, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("ptp"), UsbSrvSupport::FUNCTION_PTP);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_Rndis_ReturnsThirtyTwo, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("rndis"), UsbSrvSupport::FUNCTION_RNDIS);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_Ncm_Returns256, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("ncm"), UsbSrvSupport::FUNCTION_NCM);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_Storage_Returns512, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("storage"), UsbSrvSupport::FUNCTION_STORAGE);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_DevModeAuth_Returns4096, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("devmode_auth"), UsbSrvSupport::FUNCTION_DEVMODE_AUTH);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_EmptyString_ReturnsErrorNotZero, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString("");
    EXPECT_NE(result, 0u);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_AcmEcm_ReturnsThree, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("acm,ecm"), 3u);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_HdcMtpStorage_Returns524, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("hdc,mtp,storage"), 524u);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_AllSettable_Returns831, TestSize.Level1)
{
    std::string all = "acm,ecm,hdc,mtp,ptp,rndis,ncm,storage";
    EXPECT_EQ(UsbDeviceManager::ConvertFromString(all), 831u);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_UnknownToken_ReturnsError, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString("xyz");
    EXPECT_NE(result, 0u);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_MixedValidUnknown_ReturnsError, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString("acm,xyz");
    EXPECT_NE(result, UsbSrvSupport::FUNCTION_ACM);
    EXPECT_NE(result, 0u);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_LeadingSpace_ReturnsError, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString(" acm");
    EXPECT_NE(result, UsbSrvSupport::FUNCTION_ACM);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_TrailingSpace_ReturnsError, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString("acm ");
    EXPECT_NE(result, UsbSrvSupport::FUNCTION_ACM);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_TrailingComma_ReturnsError, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString("acm,");
    EXPECT_NE(result, UsbSrvSupport::FUNCTION_ACM);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_LeadingComma_ReturnsError, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString(",acm");
    EXPECT_NE(result, UsbSrvSupport::FUNCTION_ACM);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_UpperCase_ReturnsError, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString("ACM");
    EXPECT_NE(result, UsbSrvSupport::FUNCTION_ACM);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_DuplicateAcm_ReturnsAcm, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString("acm,acm"), UsbSrvSupport::FUNCTION_ACM);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_NoneWithExtra_ReturnsError, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString("none,acm");
    EXPECT_NE(result, 0u);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_NoneAlone_ReturnsZero, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertFromString(UsbSrvSupport::FUNCTION_NAME_NONE), 0u);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_DevModeAuthRoundTripFails, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString("devmode_auth");
    EXPECT_EQ(result, UsbSrvSupport::FUNCTION_DEVMODE_AUTH);
    EXPECT_FALSE(UsbDeviceManager::IsSettableFunctions(static_cast<int32_t>(result)));
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_None_ReturnsNoneString, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(0), "none");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_Acm_ReturnsAcmString, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_ACM), "acm");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_Ecm_ReturnsEcmString, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_ECM), "ecm");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_Hdc_ReturnsHdcString, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_HDC), "hdc");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_Mtp_ReturnsMtpString, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_MTP), "mtp");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_Ptp_ReturnsPtpString, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_PTP), "ptp");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_Rndis_ReturnsRndisString, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_RNDIS), "rndis");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_Ncm_ReturnsNcmString, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_NCM), "ncm");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_Storage_ReturnsStorageString, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_STORAGE), "storage");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_AcmEcm_ReturnsCommaSeparated, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(3), "acm,ecm");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_HdcMtpStorage_ReturnsCommaSeparated, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(524), "hdc,mtp,storage");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_AllSettable_ReturnsFullList, TestSize.Level1)
{
    std::string result = UsbDeviceManager::ConvertToString(831);
    EXPECT_EQ(result, "acm,ecm,hdc,mtp,ptp,rndis,ncm,storage");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_DevModeAuth_ReturnsNone, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_DEVMODE_AUTH), "none");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_AboveMax_ReturnsNone, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(832), "none");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_Uint32Max_ReturnsNone, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UINT32_MAX), "none");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_OneAboveMax_ReturnsNone, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(832), "none");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_ExactlyMax_ReturnsFullList, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(831), "acm,ecm,hdc,mtp,ptp,rndis,ncm,storage");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, RoundTrip_None_ReturnsNone, TestSize.Level1)
{
    uint32_t func = UsbDeviceManager::ConvertFromString("none");
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, "none");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, RoundTrip_Acm_ReturnsAcm, TestSize.Level1)
{
    uint32_t func = UsbDeviceManager::ConvertFromString("acm");
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, "acm");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, RoundTrip_Hdc_ReturnsHdc, TestSize.Level1)
{
    uint32_t func = UsbDeviceManager::ConvertFromString("hdc");
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, "hdc");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, RoundTrip_Storage_ReturnsStorage, TestSize.Level1)
{
    uint32_t func = UsbDeviceManager::ConvertFromString("storage");
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, "storage");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, RoundTrip_AcmEcmCombined_ReturnsCommaSeparated, TestSize.Level1)
{
    uint32_t func = UsbDeviceManager::ConvertFromString("acm,ecm");
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, "acm,ecm");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, RoundTrip_HdcMtpStorage_ReturnsCommaSeparated, TestSize.Level1)
{
    uint32_t func = UsbDeviceManager::ConvertFromString("hdc,mtp,storage");
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, "hdc,mtp,storage");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, RoundTrip_AllSettable_ReturnsFullList, TestSize.Level1)
{
    std::string input = "acm,ecm,hdc,mtp,ptp,rndis,ncm,storage";
    uint32_t func = UsbDeviceManager::ConvertFromString(input);
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, input);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, RoundTrip_DevModeAuth_BrokenRoundTrip, TestSize.Level1)
{
    uint32_t func = UsbDeviceManager::ConvertFromString("devmode_auth");
    EXPECT_EQ(func, UsbSrvSupport::FUNCTION_DEVMODE_AUTH);
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, "none");
    EXPECT_NE(str, "devmode_auth");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, RoundTrip_Ncm_ReturnsNcm, TestSize.Level1)
{
    uint32_t func = UsbDeviceManager::ConvertFromString("ncm");
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, "ncm");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, RoundTrip_Rndis_ReturnsRndis, TestSize.Level1)
{
    uint32_t func = UsbDeviceManager::ConvertFromString("rndis");
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, "rndis");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, RoundTrip_Ptp_ReturnsPtp, TestSize.Level1)
{
    uint32_t func = UsbDeviceManager::ConvertFromString("ptp");
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, "ptp");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_ToString_Back_ForEachSingleFunction, TestSize.Level1)
{
    struct { std::string name; uint32_t value; } cases[] = {
        {"acm", UsbSrvSupport::FUNCTION_ACM},
        {"ecm", UsbSrvSupport::FUNCTION_ECM},
        {"hdc", UsbSrvSupport::FUNCTION_HDC},
        {"mtp", UsbSrvSupport::FUNCTION_MTP},
        {"ptp", UsbSrvSupport::FUNCTION_PTP},
        {"rndis", UsbSrvSupport::FUNCTION_RNDIS},
        {"ncm", UsbSrvSupport::FUNCTION_NCM},
        {"storage", UsbSrvSupport::FUNCTION_STORAGE},
    };
    for (auto &c : cases) {
        uint32_t func = UsbDeviceManager::ConvertFromString(c.name);
        EXPECT_EQ(func, c.value);
        std::string str = UsbDeviceManager::ConvertToString(func);
        EXPECT_EQ(str, c.name);
    }
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_PreservesInsertionOrder, TestSize.Level1)
{
    uint32_t func = UsbSrvSupport::FUNCTION_HDC | UsbSrvSupport::FUNCTION_ACM;
    std::string str = UsbDeviceManager::ConvertToString(func);
    EXPECT_EQ(str, "acm,hdc");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_StorageAlone_ReturnsStorage, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_STORAGE), "storage");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_NcmAlone_ReturnsNcm, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_NCM), "ncm");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_RndisAlone_ReturnsRndis, TestSize.Level1)
{
    EXPECT_EQ(UsbDeviceManager::ConvertToString(UsbSrvSupport::FUNCTION_RNDIS), "rndis");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertToString_MtpPtpCombined_ReturnsMtpPtp, TestSize.Level1)
{
    uint32_t func = UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_PTP;
    EXPECT_EQ(UsbDeviceManager::ConvertToString(func), "mtp,ptp");
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_TwoCommasInARow_ReturnsError, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString("acm,,ecm");
    EXPECT_NE(result, 3u);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_OnlyComma_ReturnsError, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString(",");
    EXPECT_NE(result, 0u);
}

HWTEST_F(UsbDeviceManagerStaticTddTest, ConvertFromString_EmptyTokenBetweenCommas_ReturnsError, TestSize.Level1)
{
    uint32_t result = UsbDeviceManager::ConvertFromString(",,");
    EXPECT_NE(result, 0u);
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS
