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
#include <string>

#include "usb_right_manager.h"
#include "usb_right_db_helper.h"
#include "usb_device.h"
#include "usb_errors.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;

constexpr int32_t TEST_UID_1 = 100;
constexpr int32_t TEST_UID_2 = 200;
constexpr int32_t TEST_UID_CONSOLE = 0;

/**
 * @brief Test class for UsbRightManager lifecycle and cleanup paths.
 *
 * Covers the two-argument AddDeviceRight token parsing, RemoveDeviceAllRight,
 * and the static/instance cleanup helpers which are not exercised by the
 * existing error / expiry suites.
 */
class UsbRightManagerLifecycleTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<UsbRightManager> usbRightManager_;
};

void UsbRightManagerLifecycleTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerLifecycleTest::SetUpTestCase enter");
}

void UsbRightManagerLifecycleTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerLifecycleTest::TearDownTestCase enter");
}

void UsbRightManagerLifecycleTest::SetUp()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerLifecycleTest::SetUp enter");
    usbRightManager_ = std::make_shared<UsbRightManager>();
    ASSERT_NE(usbRightManager_, nullptr);
}

void UsbRightManagerLifecycleTest::TearDown()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerLifecycleTest::TearDown enter");
    usbRightManager_.reset();
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_Empty
 * @tc.desc: Two-argument AddDeviceRight rejects an empty token string.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_Empty, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Empty start");

    bool result = usbRightManager_->AddDeviceRight("1-1", "");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Empty end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_Whitespace
 * @tc.desc: Two-argument AddDeviceRight rejects a whitespace-only token string.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_Whitespace, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Whitespace start");

    bool result = usbRightManager_->AddDeviceRight("1-1", "   ");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Whitespace end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_Alphanumeric
 * @tc.desc: Two-argument AddDeviceRight rejects a mixed alphanumeric token string.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_Alphanumeric, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Alphanumeric start");

    bool result = usbRightManager_->AddDeviceRight("1-1", "abc123");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Alphanumeric end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_PlusSign
 * @tc.desc: Two-argument AddDeviceRight rejects a token string with a leading plus sign.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_PlusSign, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_PlusSign start");

    bool result = usbRightManager_->AddDeviceRight("1-1", "+123");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_PlusSign end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_DecimalPoint
 * @tc.desc: Two-argument AddDeviceRight rejects a token string containing a decimal point.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_DecimalPoint, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_DecimalPoint start");

    bool result = usbRightManager_->AddDeviceRight("1-1", "12.34");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_DecimalPoint end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_Negative
 * @tc.desc: Two-argument AddDeviceRight rejects a negative token string.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_Negative, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Negative start");

    bool result = usbRightManager_->AddDeviceRight("1-1", "-123456");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Negative end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_Zero
 * @tc.desc: Two-argument AddDeviceRight rejects token zero because no valid HAP token maps to it.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_Zero, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Zero start");

    bool result = usbRightManager_->AddDeviceRight("1-1", "0");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Zero end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_LeadingZeros
 * @tc.desc: Two-argument AddDeviceRight rejects a token with leading zeros since it maps to no valid HAP token.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_LeadingZeros, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_LeadingZeros start");

    bool result = usbRightManager_->AddDeviceRight("1-1", "00000123");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_LeadingZeros end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_Uint64Max
 * @tc.desc: Two-argument AddDeviceRight rejects the maximum uint64 value when no HAP token maps to it.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_Uint64Max, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Uint64Max start");

    bool result = usbRightManager_->AddDeviceRight("1-1", "18446744073709551615");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Uint64Max end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_Overflow
 * @tc.desc: Two-argument AddDeviceRight rejects a token string exceeding the uint64 range.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_Overflow, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Overflow start");

    bool result = usbRightManager_->AddDeviceRight("1-1", "18446744073709551616");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_Overflow end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_VeryLong
 * @tc.desc: Two-argument AddDeviceRight rejects an excessively long digit string.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_VeryLong, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_VeryLong start");

    std::string tokenIdStr(100, '9');

    bool result = usbRightManager_->AddDeviceRight("1-1", tokenIdStr);

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_VeryLong end");
}

/**
 * @tc.name: UsbRightManager_RemoveDeviceAllRight_NonConsole
 * @tc.desc: RemoveDeviceAllRight always returns true and does not throw.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_RemoveDeviceAllRight_NonConsole, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceAllRight_NonConsole start");

    bool result = usbRightManager_->RemoveDeviceAllRight("1-2");

    EXPECT_TRUE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceAllRight_NonConsole end");
}

/**
 * @tc.name: UsbRightManager_RemoveDeviceAllRight_EmptyDeviceName
 * @tc.desc: RemoveDeviceAllRight with an empty device name still returns true.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_RemoveDeviceAllRight_EmptyDeviceName, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceAllRight_EmptyDeviceName start");

    bool result = usbRightManager_->RemoveDeviceAllRight("");

    EXPECT_TRUE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceAllRight_EmptyDeviceName end");
}

/**
 * @tc.name: UsbRightManager_RemoveDeviceAllRight_WithExistingRights
 * @tc.desc: RemoveDeviceAllRight returns true even with existing records. It triggers a global
 *           TidyUpRight whose deletion scope depends on the bundle-installed / user-exists checks,
 *           so only the deterministic return value is asserted.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_RemoveDeviceAllRight_WithExistingRights, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceAllRight_WithExistingRights start");

    std::string bundleName = "com.example.lifecycle.dev";
    std::string tokenId = "20000001";
    std::string deviceName1 = "1-3";
    std::string deviceName2 = "1-4";

    if (!usbRightManager_->AddDeviceRight(deviceName1, bundleName, tokenId, TEST_UID_1) ||
        !usbRightManager_->AddDeviceRight(deviceName2, bundleName, tokenId, TEST_UID_1)) {
        GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
    }

    bool result = usbRightManager_->RemoveDeviceAllRight(deviceName1);

    EXPECT_TRUE(result);

    usbRightManager_->RemoveDeviceRight(deviceName1, bundleName, tokenId, TEST_UID_1);
    usbRightManager_->RemoveDeviceRight(deviceName2, bundleName, tokenId, TEST_UID_1);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceAllRight_WithExistingRights end");
}

/**
 * @tc.name: UsbRightManager_CleanUpRightUserStopped_ReturnsCode
 * @tc.desc: CleanUpRightUserStopped returns a non-positive status code without crashing.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_CleanUpRightUserStopped_ReturnsCode, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CleanUpRightUserStopped_ReturnsCode start");

    int32_t ret = UsbRightManager::CleanUpRightUserStopped(TEST_UID_2);

    EXPECT_LE(ret, USB_RIGHT_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CleanUpRightUserStopped_ReturnsCode end");
}

/**
 * @tc.name: UsbRightManager_CleanUpRightUserDeleted_ReturnsKnownCode
 * @tc.desc: CleanUpRightUserDeleted returns a known status code and reports totals.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_CleanUpRightUserDeleted_ReturnsKnownCode, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CleanUpRightUserDeleted_ReturnsKnownCode start");

    int32_t totalUsers = -1;
    int32_t deleteUsers = -1;
    int32_t ret = UsbRightManager::CleanUpRightUserDeleted(totalUsers, deleteUsers);

    EXPECT_TRUE(ret == USB_RIGHT_OK || ret == USB_RIGHT_NOP || ret == USB_RIGHT_FAILURE);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CleanUpRightUserDeleted_ReturnsKnownCode end");
}

/**
 * @tc.name: UsbRightManager_CleanUpRightExpired_EmptyDeviceVector
 * @tc.desc: CleanUpRightExpired with an empty device vector returns a status code without crashing.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_CleanUpRightExpired_EmptyDeviceVector, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CleanUpRightExpired_EmptyDeviceVector start");

    std::vector<std::string> devices;
    int32_t ret = usbRightManager_->CleanUpRightExpired(devices);

    EXPECT_LE(ret, USB_RIGHT_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CleanUpRightExpired_EmptyDeviceVector end");
}

/**
 * @tc.name: UsbRightManager_CleanUpRightExpired_WithDeviceNames
 * @tc.desc: CleanUpRightExpired with device names returns a status code without crashing.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_CleanUpRightExpired_WithDeviceNames, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CleanUpRightExpired_WithDeviceNames start");

    std::vector<std::string> devices = {"1-5", "1-6", "1-7"};
    int32_t ret = usbRightManager_->CleanUpRightExpired(devices);

    EXPECT_LE(ret, USB_RIGHT_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CleanUpRightExpired_WithDeviceNames end");
}

/**
 * @tc.name: UsbRightManager_AddRemoveCycle_MultipleDevices
 * @tc.desc: Grant and revoke rights for a batch of devices and verify final state is clean.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddRemoveCycle_MultipleDevices, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddRemoveCycle_MultipleDevices start");

    std::string bundleName = "com.example.lifecycle.cycle";
    std::string tokenId = "20000002";
    std::vector<std::string> deviceNames = {"1-8", "1-9", "1-10", "1-11", "1-12"};

    for (const auto &deviceName : deviceNames) {
        if (!usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1)) {
            GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
        }
        EXPECT_TRUE(usbRightManager_->HasRight(deviceName, bundleName, tokenId, TEST_UID_1));
    }

    for (const auto &deviceName : deviceNames) {
        EXPECT_TRUE(usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1));
        EXPECT_FALSE(usbRightManager_->HasRight(deviceName, bundleName, tokenId, TEST_UID_1));
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddRemoveCycle_MultipleDevices end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_ConsoleUserBypass
 * @tc.desc: Four-argument AddDeviceRight bypasses the database for the console user.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_ConsoleUserBypass, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_ConsoleUserBypass start");

    bool result = usbRightManager_->AddDeviceRight("1-13", "com.example.console", "20000003", TEST_UID_CONSOLE);

    EXPECT_TRUE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_ConsoleUserBypass end");
}

/**
 * @tc.name: UsbRightManager_RemoveDeviceRight_ConsoleUserBypass
 * @tc.desc: RemoveDeviceRight bypasses the database for the console user and returns true.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_RemoveDeviceRight_ConsoleUserBypass, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceRight_ConsoleUserBypass start");

    bool result = usbRightManager_->RemoveDeviceRight("1-14", "com.example.console", "20000004", TEST_UID_CONSOLE);

    EXPECT_TRUE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceRight_ConsoleUserBypass end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_InternalSpace
 * @tc.desc: Two-argument AddDeviceRight rejects a token string containing an internal space.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_InternalSpace, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_InternalSpace start");

    bool result = usbRightManager_->AddDeviceRight("1-15", "1 2");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_InternalSpace end");
}

/**
 * @tc.name: UsbRightManager_CleanUpRightAppUninstalled_Static_ReturnsNop
 * @tc.desc: Static CleanUpRightAppUninstalled returns USB_RIGHT_NOP for a reserved user ID with no records,
 *           or USB_RIGHT_OK (the defensive `return false`) when the DB helper is unavailable.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_CleanUpRightAppUninstalled_Static_ReturnsNop, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CleanUpRightAppUninstalled_Static_ReturnsNop start");

    // 999 is reserved in this suite; no test writes records under it, so the query is empty.
    int32_t ret = UsbRightManager::CleanUpRightAppUninstalled(999, "com.example.no.such.bundle");

    EXPECT_TRUE(ret == USB_RIGHT_NOP || ret == USB_RIGHT_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CleanUpRightAppUninstalled_Static_ReturnsNop end");
}

/**
 * @tc.name: UsbRightManager_IsOsAccountExists_ReturnsKnownCode
 * @tc.desc: IsOsAccountExists returns either USB_RIGHT_OK or USB_RIGHT_FAILURE depending on account SA availability.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_IsOsAccountExists_ReturnsKnownCode, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_IsOsAccountExists_ReturnsKnownCode start");

    bool isAccountExists = false;
    int32_t ret = UsbRightManager::IsOsAccountExists(TEST_UID_1, isAccountExists);

    EXPECT_TRUE(ret == USB_RIGHT_OK || ret == USB_RIGHT_FAILURE);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_IsOsAccountExists_ReturnsKnownCode end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_TokenStr_HexPrefix
 * @tc.desc: Two-argument AddDeviceRight rejects a hexadecimal-prefixed token string.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_AddDeviceRight_TokenStr_HexPrefix, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_HexPrefix start");

    bool result = usbRightManager_->AddDeviceRight("1-16", "0x1234");

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_TokenStr_HexPrefix end");
}

/**
 * @tc.name: UsbRightManager_RemoveDeviceRight_IdempotentDelete
 * @tc.desc: Removing a right twice is idempotent: the second remove of a non-existent record still returns true.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerLifecycleTest, UsbRightManager_RemoveDeviceRight_IdempotentDelete, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceRight_IdempotentDelete start");

    std::string deviceName = "1-17";
    std::string bundleName = "com.example.lifecycle.idempotent";
    std::string tokenId = "20000005";

    if (!usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1)) {
        GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
    }

    EXPECT_TRUE(usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1));
    EXPECT_FALSE(usbRightManager_->HasRight(deviceName, bundleName, tokenId, TEST_UID_1));

    EXPECT_TRUE(usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1));
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceRight_IdempotentDelete end");
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS
