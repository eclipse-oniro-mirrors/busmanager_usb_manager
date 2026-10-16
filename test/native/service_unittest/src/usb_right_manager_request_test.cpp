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
#include "usb_accessory.h"
#include "serial_device_identity.h"
#include "usb_srv_support.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;

constexpr int32_t TEST_UID_1 = 100;
constexpr int32_t TEST_UID_2 = 200;
constexpr int32_t TEST_UID_CONSOLE = 0;

/**
 * @brief Test class for UsbRightManager request/cancel flow.
 *
 * Covers the three RequestRight overloads (host / accessory / serial) and the
 * CancelDeviceRight revoke path, which are not exercised by the existing
 * error / expiry suites.
 */
class UsbRightManagerRequestTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<UsbRightManager> usbRightManager_;
};

void UsbRightManagerRequestTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerRequestTest::SetUpTestCase enter");
}

void UsbRightManagerRequestTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerRequestTest::TearDownTestCase enter");
}

void UsbRightManagerRequestTest::SetUp()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerRequestTest::SetUp enter");
    usbRightManager_ = std::make_shared<UsbRightManager>();
    ASSERT_NE(usbRightManager_, nullptr);
}

void UsbRightManagerRequestTest::TearDown()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerRequestTest::TearDown enter");
    usbRightManager_.reset();
}

#ifdef USB_MANAGER_FEATURE_HOST
/**
 * @tc.name: UsbRightManager_RequestRight_AlreadyHasRight
 * @tc.desc: RequestRight short-circuits to UEC_OK when the right has already been granted.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestRight_AlreadyHasRight, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_AlreadyHasRight start");

    std::string busDev = "1-2";
    std::string deviceName = "1-2";
    std::string bundleName = "com.example.req.granted";
    std::string tokenId = "10000002";

    if (!usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1)) {
        GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
    }

    int32_t result = usbRightManager_->RequestRight(busDev, deviceName, bundleName, tokenId, TEST_UID_1);

    EXPECT_EQ(result, UEC_OK);

    usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_AlreadyHasRight end");
}

/**
 * @tc.name: UsbRightManager_RequestRight_ConsoleUserBypass
 * @tc.desc: RequestRight for the console user bypasses both the database and the dialog.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestRight_ConsoleUserBypass, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_ConsoleUserBypass start");

    std::string busDev = "1-3";
    std::string deviceName = "1-3";
    std::string bundleName = "com.example.req.console";
    std::string tokenId = "10000003";

    int32_t result = usbRightManager_->RequestRight(busDev, deviceName, bundleName, tokenId, TEST_UID_CONSOLE);

    EXPECT_EQ(result, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_ConsoleUserBypass end");
}
#endif // USB_MANAGER_FEATURE_HOST

/**
 * @tc.name: UsbRightManager_RequestAccessoryRight_AlreadyHasRight
 * @tc.desc: Accessory RequestRight short-circuits with result=true when the right already exists.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestAccessoryRight_AlreadyHasRight, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestAccessoryRight_AlreadyHasRight start");

    USBAccessory access("manufacturer", "product", "description", "version", "serial");
    std::string seriaValue = "accessory-2";
    std::string bundleName = "com.example.acc.granted";
    std::string tokenId = "10000005";
    bool result = false;

    if (!usbRightManager_->AddDeviceRight(seriaValue, bundleName, tokenId, TEST_UID_1)) {
        GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
    }

    int32_t ret = usbRightManager_->RequestRight(access, seriaValue, bundleName, tokenId, TEST_UID_1, result);

    EXPECT_EQ(ret, UEC_OK);
    EXPECT_TRUE(result);

    usbRightManager_->RemoveDeviceRight(seriaValue, bundleName, tokenId, TEST_UID_1);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestAccessoryRight_AlreadyHasRight end");
}

/**
 * @tc.name: UsbRightManager_RequestSerialRight_InvalidTokenDenied
 * @tc.desc: Serial RequestRight with a non-numeric token is rejected before the dialog is shown.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestSerialRight_InvalidTokenDenied, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestSerialRight_InvalidTokenDenied start");

    SerialDeviceIdentity identity;
    identity.busDev = "1-4";
    identity.deviceName = "1-4";
    std::string bundleName = "com.example.serial.invalidtoken";
    std::string tokenId = "not_a_number";

    int32_t ret = usbRightManager_->RequestRight(1, identity, bundleName, tokenId, TEST_UID_1);

    EXPECT_EQ(ret, UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestSerialRight_InvalidTokenDenied end");
}

/**
 * @tc.name: UsbRightManager_RequestSerialRight_AlreadyHasRight
 * @tc.desc: Serial RequestRight short-circuits to UEC_OK when the right already exists.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestSerialRight_AlreadyHasRight, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestSerialRight_AlreadyHasRight start");

    SerialDeviceIdentity identity;
    identity.busDev = "1-6";
    identity.deviceName = "1-6";
    std::string bundleName = "com.example.serial.granted";
    std::string tokenId = "10000007";

    if (!usbRightManager_->AddDeviceRight(identity.deviceName, bundleName, tokenId, TEST_UID_1)) {
        GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
    }

    int32_t ret = usbRightManager_->RequestRight(3, identity, bundleName, tokenId, TEST_UID_1);

    EXPECT_EQ(ret, UEC_OK);

    usbRightManager_->RemoveDeviceRight(identity.deviceName, bundleName, tokenId, TEST_UID_1);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestSerialRight_AlreadyHasRight end");
}

/**
 * @tc.name: UsbRightManager_CancelDeviceRight_NonConsoleReturnsOk
 * @tc.desc: CancelDeviceRight for a non-console user always returns UEC_OK.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_CancelDeviceRight_NonConsoleReturnsOk, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CancelDeviceRight_NonConsoleReturnsOk start");

    std::string deviceName = "1-7";
    std::string bundleName = "com.example.cancel";
    std::string tokenId = "10000008";

    int32_t ret = usbRightManager_->CancelDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CancelDeviceRight_NonConsoleReturnsOk end");
}

/**
 * @tc.name: UsbRightManager_CancelDeviceRight_AfterAddRevokesRight
 * @tc.desc: CancelDeviceRight revokes a previously granted right so HasRight returns false.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_CancelDeviceRight_AfterAddRevokesRight, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CancelDeviceRight_AfterAddRevokesRight start");

    std::string deviceName = "1-8";
    std::string bundleName = "com.example.cancel.revoke";
    std::string tokenId = "10000009";

    if (!usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1)) {
        GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
    }

    bool hasRightBefore = usbRightManager_->HasRight(deviceName, bundleName, tokenId, TEST_UID_1);
    EXPECT_TRUE(hasRightBefore);

    int32_t ret = usbRightManager_->CancelDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1);
    EXPECT_EQ(ret, UEC_OK);

    bool hasRightAfter = usbRightManager_->HasRight(deviceName, bundleName, tokenId, TEST_UID_1);
    EXPECT_FALSE(hasRightAfter);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CancelDeviceRight_AfterAddRevokesRight end");
}

/**
 * @tc.name: UsbRightManager_HasSetFuncRight_NonHdcFunction
 * @tc.desc: HasSetFuncRight returns UEC_OK for functions that do not include the HDC bit.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_HasSetFuncRight_NonHdcFunction, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasSetFuncRight_NonHdcFunction start");

    EXPECT_EQ(usbRightManager_->HasSetFuncRight(UsbSrvSupport::FUNCTION_NONE), UEC_OK);
    EXPECT_EQ(usbRightManager_->HasSetFuncRight(UsbSrvSupport::FUNCTION_ACM), UEC_OK);
    EXPECT_EQ(usbRightManager_->HasSetFuncRight(UsbSrvSupport::FUNCTION_ECM), UEC_OK);
    EXPECT_EQ(usbRightManager_->HasSetFuncRight(UsbSrvSupport::FUNCTION_ACM |
        UsbSrvSupport::FUNCTION_ECM), UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasSetFuncRight_NonHdcFunction end");
}

/**
 * @tc.name: UsbRightManager_RequestRight_IsolationAcrossUsers
 * @tc.desc: A right granted to one user must not be visible to another user.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestRight_IsolationAcrossUsers, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_IsolationAcrossUsers start");

    std::string deviceName = "1-9";
    std::string bundleName = "com.example.iso.user";
    std::string tokenId = "10000010";

    if (!usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1)) {
        GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
    }

    bool hasRightUid1 = usbRightManager_->HasRight(deviceName, bundleName, tokenId, TEST_UID_1);
    bool hasRightUid2 = usbRightManager_->HasRight(deviceName, bundleName, tokenId, TEST_UID_2);

    EXPECT_TRUE(hasRightUid1);
    EXPECT_FALSE(hasRightUid2);

    usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_IsolationAcrossUsers end");
}

/**
 * @tc.name: UsbRightManager_RequestRight_IsolationAcrossTokens
 * @tc.desc: A right granted under one token must not be visible under a different token.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestRight_IsolationAcrossTokens, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_IsolationAcrossTokens start");

    std::string deviceName = "1-10";
    std::string bundleName = "com.example.iso.token";
    std::string tokenId1 = "10000011";
    std::string tokenId2 = "10000012";

    if (!usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId1, TEST_UID_1)) {
        GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
    }

    bool hasRightToken1 = usbRightManager_->HasRight(deviceName, bundleName, tokenId1, TEST_UID_1);
    bool hasRightToken2 = usbRightManager_->HasRight(deviceName, bundleName, tokenId2, TEST_UID_1);

    EXPECT_TRUE(hasRightToken1);
    EXPECT_FALSE(hasRightToken2);

    usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId1, TEST_UID_1);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_IsolationAcrossTokens end");
}

/**
 * @tc.name: UsbRightManager_RequestRight_IsolationAcrossBundles
 * @tc.desc: A right granted to one bundle must not be visible to another bundle.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestRight_IsolationAcrossBundles, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_IsolationAcrossBundles start");

    std::string deviceName = "1-11";
    std::string bundleName1 = "com.example.iso.bundle1";
    std::string bundleName2 = "com.example.iso.bundle2";
    std::string tokenId = "10000013";

    if (!usbRightManager_->AddDeviceRight(deviceName, bundleName1, tokenId, TEST_UID_1)) {
        GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
    }

    bool hasRightBundle1 = usbRightManager_->HasRight(deviceName, bundleName1, tokenId, TEST_UID_1);
    bool hasRightBundle2 = usbRightManager_->HasRight(deviceName, bundleName2, tokenId, TEST_UID_1);

    EXPECT_TRUE(hasRightBundle1);
    EXPECT_FALSE(hasRightBundle2);

    usbRightManager_->RemoveDeviceRight(deviceName, bundleName1, tokenId, TEST_UID_1);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_IsolationAcrossBundles end");
}

/**
 * @tc.name: UsbRightManager_RequestRight_IsolationAcrossDevices
 * @tc.desc: A right granted for one device must not be visible for a different device.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestRight_IsolationAcrossDevices, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_IsolationAcrossDevices start");

    std::string deviceName1 = "1-12";
    std::string deviceName2 = "1-13";
    std::string bundleName = "com.example.iso.device";
    std::string tokenId = "10000014";

    if (!usbRightManager_->AddDeviceRight(deviceName1, bundleName, tokenId, TEST_UID_1)) {
        GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
    }

    bool hasRightDevice1 = usbRightManager_->HasRight(deviceName1, bundleName, tokenId, TEST_UID_1);
    bool hasRightDevice2 = usbRightManager_->HasRight(deviceName2, bundleName, tokenId, TEST_UID_1);

    EXPECT_TRUE(hasRightDevice1);
    EXPECT_FALSE(hasRightDevice2);

    usbRightManager_->RemoveDeviceRight(deviceName1, bundleName, tokenId, TEST_UID_1);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestRight_IsolationAcrossDevices end");
}

/**
 * @tc.name: UsbRightManager_RequestAccessoryRight_ConsoleUserBypass
 * @tc.desc: Accessory RequestRight for the console user bypasses the dialog and returns result=true.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestAccessoryRight_ConsoleUserBypass, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestAccessoryRight_ConsoleUserBypass start");

    USBAccessory access("manufacturer", "product", "description", "version", "serial");
    std::string seriaValue = "accessory-console";
    std::string bundleName = "com.example.acc.console";
    std::string tokenId = "10000015";
    bool result = false;

    int32_t ret = usbRightManager_->RequestRight(access, seriaValue, bundleName, tokenId, TEST_UID_CONSOLE, result);

    EXPECT_EQ(ret, UEC_OK);
    EXPECT_TRUE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestAccessoryRight_ConsoleUserBypass end");
}

/**
 * @tc.name: UsbRightManager_RequestSerialRight_ConsoleUserBypass
 * @tc.desc: Serial RequestRight for the console user bypasses the dialog and returns UEC_OK.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestSerialRight_ConsoleUserBypass, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestSerialRight_ConsoleUserBypass start");

    SerialDeviceIdentity identity;
    identity.busDev = "1-14";
    identity.deviceName = "1-14";
    std::string bundleName = "com.example.serial.console";
    std::string tokenId = "10000016";

    int32_t ret = usbRightManager_->RequestRight(4, identity, bundleName, tokenId, TEST_UID_CONSOLE);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestSerialRight_ConsoleUserBypass end");
}

/**
 * @tc.name: UsbRightManager_RequestSerialRight_ConsoleUserWithInvalidToken
 * @tc.desc: Console user bypass happens before token validation, so an invalid token still yields UEC_OK.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestSerialRight_ConsoleUserWithInvalidToken, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestSerialRight_ConsoleUserWithInvalidToken start");

    SerialDeviceIdentity identity;
    identity.busDev = "1-16";
    identity.deviceName = "1-16";
    std::string bundleName = "com.example.serial.console.invalid";
    std::string tokenId = "still_not_a_number";

    int32_t ret = usbRightManager_->RequestRight(5, identity, bundleName, tokenId, TEST_UID_CONSOLE);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestSerialRight_ConsoleUserWithInvalidToken end");
}

/**
 * @tc.name: UsbRightManager_HasSetFuncRight_HdcFunction
 * @tc.desc: HasSetFuncRight for the HDC bit is gated on persist.hdc.control and developer mode.
 *           The exact result is environment-dependent, so only the valid return set is asserted.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_HasSetFuncRight_HdcFunction, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasSetFuncRight_HdcFunction start");

    int32_t ret = usbRightManager_->HasSetFuncRight(UsbSrvSupport::FUNCTION_HDC);

    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_CHECK_HDC);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasSetFuncRight_HdcFunction end");
}

/**
 * @tc.name: UsbRightManager_CancelDeviceRight_InvalidUserIdReturnsOk
 * @tc.desc: CancelDeviceRight with a negative (non-console) user ID still returns UEC_OK.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_CancelDeviceRight_InvalidUserIdReturnsOk, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CancelDeviceRight_InvalidUserIdReturnsOk start");

    std::string deviceName = "1-17";
    std::string bundleName = "com.example.cancel.invaliduid";
    std::string tokenId = "10000018";

    int32_t ret = usbRightManager_->CancelDeviceRight(deviceName, bundleName, tokenId, -1);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CancelDeviceRight_InvalidUserIdReturnsOk end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_IdempotentReadd
 * @tc.desc: Adding the same right twice updates the record instead of duplicating it, so HasRight stays true.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_AddDeviceRight_IdempotentReadd, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_IdempotentReadd start");

    std::string deviceName = "1-18";
    std::string bundleName = "com.example.readd";
    std::string tokenId = "10000019";

    if (!usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1)) {
        GTEST_SKIP() << "UsbRightDbHelper RDB store unavailable";
    }
    EXPECT_TRUE(usbRightManager_->HasRight(deviceName, bundleName, tokenId, TEST_UID_1));

    EXPECT_TRUE(usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1));
    EXPECT_TRUE(usbRightManager_->HasRight(deviceName, bundleName, tokenId, TEST_UID_1));

    EXPECT_TRUE(usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, TEST_UID_1));
    EXPECT_FALSE(usbRightManager_->HasRight(deviceName, bundleName, tokenId, TEST_UID_1));
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_IdempotentReadd end");
}

/**
 * @tc.name: UsbRightManager_HasSetFuncRight_NonHdcFunctions
 * @tc.desc: HasSetFuncRight returns UEC_OK for every non-HDC function bit.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_HasSetFuncRight_NonHdcFunctions, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasSetFuncRight_NonHdcFunctions start");

    EXPECT_EQ(usbRightManager_->HasSetFuncRight(UsbSrvSupport::FUNCTION_MTP), UEC_OK);
    EXPECT_EQ(usbRightManager_->HasSetFuncRight(UsbSrvSupport::FUNCTION_PTP), UEC_OK);
    EXPECT_EQ(usbRightManager_->HasSetFuncRight(UsbSrvSupport::FUNCTION_RNDIS), UEC_OK);
    EXPECT_EQ(usbRightManager_->HasSetFuncRight(UsbSrvSupport::FUNCTION_NCM), UEC_OK);
    EXPECT_EQ(usbRightManager_->HasSetFuncRight(UsbSrvSupport::FUNCTION_STORAGE), UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasSetFuncRight_NonHdcFunctions end");
}

/**
 * @tc.name: UsbRightManager_RequestSerialRight_EmptyTokenDenied
 * @tc.desc: Serial RequestRight with an empty token is rejected by the token regex before the dialog is shown.
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerRequestTest, UsbRightManager_RequestSerialRight_EmptyTokenDenied, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestSerialRight_EmptyTokenDenied start");

    SerialDeviceIdentity identity;
    identity.busDev = "1-19";
    identity.deviceName = "1-19";
    std::string bundleName = "com.example.serial.emptytoken";
    std::string tokenId = "";

    int32_t ret = usbRightManager_->RequestRight(6, identity, bundleName, tokenId, TEST_UID_1);

    EXPECT_EQ(ret, UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RequestSerialRight_EmptyTokenDenied end");
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS
