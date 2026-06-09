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
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

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
constexpr int32_t TEST_UID_INVALID = -1;
constexpr int32_t TEST_UID_CONSOLE = 0;
constexpr uint64_t USB_RIGHT_VALID_PERIOD_MIN = 0;
constexpr uint64_t USB_RIGHT_VALID_PERIOD_SET = 86400;
constexpr uint64_t USB_RIGHT_VALID_PERIOD_MAX = 0xFFFFFFFF;

class UsbRightManagerErrorTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<UsbRightManager> usbRightManager_;
};

void UsbRightManagerErrorTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerErrorTest::SetUpTestCase enter");
}

void UsbRightManagerErrorTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerErrorTest::TearDownTestCase enter");
}

void UsbRightManagerErrorTest::SetUp()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerErrorTest::SetUp enter");
    usbRightManager_ = std::make_shared<UsbRightManager>();
    ASSERT_NE(usbRightManager_, nullptr);
}

void UsbRightManagerErrorTest::TearDown()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerErrorTest::TearDown enter");
    usbRightManager_.reset();
}

/**
 * @tc.name: UsbRightManager_HasRight_WithConsoleUser
 * @tc.desc: Test HasRight with console user ID (should bypass)
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_HasRight_WithConsoleUser, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasRight_WithConsoleUser start");

    std::string deviceName = "1-1";
    std::string bundleName = "com.example.test";
    std::string tokenId = "12345678";
    int32_t userId = TEST_UID_CONSOLE;

    bool result = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    EXPECT_TRUE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasRight_WithConsoleUser end");
}

/**
 * @tc.name: UsbRightManager_HasRight_WithInvalidUserId
 * @tc.desc: Test HasRight with invalid user ID
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_HasRight_WithInvalidUserId, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasRight_WithInvalidUserId start");

    std::string deviceName = "1-1";
    std::string bundleName = "com.example.test";
    std::string tokenId = "12345678";
    int32_t userId = TEST_UID_INVALID;

    bool result = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasRight_WithInvalidUserId end");
}

/**
 * @tc.name: UsbRightManager_HasRight_WithEmptyDeviceName
 * @tc.desc: Test HasRight with empty device name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_HasRight_WithEmptyDeviceName, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasRight_WithEmptyDeviceName start");

    std::string deviceName = "";
    std::string bundleName = "com.example.test";
    std::string tokenId = "12345678";
    int32_t userId = TEST_UID_1;

    bool result = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasRight_WithEmptyDeviceName end");
}

/**
 * @tc.name: UsbRightManager_HasRight_WithEmptyBundleName
 * @tc.desc: Test HasRight with empty bundle name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_HasRight_WithEmptyBundleName, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasRight_WithEmptyBundleName start");

    std::string deviceName = "1-1";
    std::string bundleName = "";
    std::string tokenId = "12345678";
    int32_t userId = TEST_UID_1;

    bool result = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasRight_WithEmptyBundleName end");
}

/**
 * @tc.name: UsbRightManager_RemoveDeviceRight_WithConsoleUser
 * @tc.desc: Test RemoveDeviceRight with console user ID (should bypass)
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_RemoveDeviceRight_WithConsoleUser, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceRight_WithConsoleUser start");

    std::string deviceName = "1-1";
    std::string bundleName = "com.example.test";
    std::string tokenId = "12345678";
    int32_t userId = TEST_UID_CONSOLE;

    bool result = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);

    EXPECT_TRUE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceRight_WithConsoleUser end");
}

/**
 * @tc.name: UsbRightManager_CancelDeviceRight_WithConsoleUser
 * @tc.desc: Test CancelDeviceRight with console user ID
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_CancelDeviceRight_WithConsoleUser, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CancelDeviceRight_WithConsoleUser start");

    std::string deviceName = "1-1";
    std::string bundleName = "com.example.test";
    std::string tokenId = "12345678";
    int32_t userId = TEST_UID_CONSOLE;

    int32_t result = usbRightManager_->CancelDeviceRight(deviceName, bundleName, tokenId, userId);

    EXPECT_EQ(result, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_CancelDeviceRight_WithConsoleUser end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_WithInvalidTokenId
 * @tc.desc: Test AddDeviceRight with invalid token ID (non-numeric)
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_AddDeviceRight_WithInvalidTokenId, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_WithInvalidTokenId start");

    std::string deviceName = "1-1";
    std::string tokenIdStr = "invalid_token";

    bool result = usbRightManager_->AddDeviceRight(deviceName, tokenIdStr);

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_WithInvalidTokenId end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_WithNegativeTokenId
 * @tc.desc: Test AddDeviceRight with negative token ID
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_AddDeviceRight_WithNegativeTokenId, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_WithNegativeTokenId start");

    std::string deviceName = "1-1";
    std::string tokenIdStr = "-123456";

    bool result = usbRightManager_->AddDeviceRight(deviceName, tokenIdStr);

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_WithNegativeTokenId end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_WithLargeTokenId
 * @tc.desc: Test AddDeviceRight with token ID exceeding uint64_t range
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_AddDeviceRight_WithLargeTokenId, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_WithLargeTokenId start");

    std::string deviceName = "1-1";
    std::string tokenIdStr = "18446744073709551616";

    bool result = usbRightManager_->AddDeviceRight(deviceName, tokenIdStr);

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_WithLargeTokenId end");
}

/**
 * @tc.name: UsbRightManager_RemoveDeviceAllRight_WithEmptyDeviceName
 * @tc.desc: Test RemoveDeviceAllRight with empty device name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_RemoveDeviceAllRight_WithEmptyDeviceName, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceAllRight_WithEmptyDeviceName start");

    std::string deviceName = "";

    bool result = usbRightManager_->RemoveDeviceAllRight(deviceName);

    EXPECT_TRUE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_RemoveDeviceAllRight_WithEmptyDeviceName end");
}

/**
 * @tc.name: UsbRightManager_GetProductName_WithInvalidDevName
 * @tc.desc: Test GetProductName with invalid device name format
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_GetProductName_WithInvalidDevName, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_GetProductName_WithInvalidDevName start");

    std::string devName = "invalid-format";
    std::string productName;

    bool result = usbRightManager_->GetProductName(devName, productName);

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_GetProductName_WithInvalidDevName end");
}

/**
 * @tc.name: UsbRightManager_Init_Success
 * @tc.desc: Test UsbRightManager Init method
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_Init_Success, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_Init_Success start");

    int32_t result = usbRightManager_->Init();

    EXPECT_EQ(result, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_Init_Success end");
}

/**
 * @tc.name: UsbRightManager_AddDeviceRight_WithConsoleUser
 * @tc.desc: Test AddDeviceRight with console user ID
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_AddDeviceRight_WithConsoleUser, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_WithConsoleUser start");

    std::string deviceName = "1-1";
    std::string bundleName = "com.example.test";
    std::string tokenId = "12345678";
    int32_t userId = TEST_UID_CONSOLE;

    bool result = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);

    EXPECT_TRUE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_AddDeviceRight_WithConsoleUser end");
}

/**
 * @tc.name: UsbRightManager_HasRight_WithLongDeviceName
 * @tc.desc: Test HasRight with very long device name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerErrorTest, UsbRightManager_HasRight_WithLongDeviceName, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasRight_WithLongDeviceName start");

    std::string deviceName(1000, 'A');
    std::string bundleName = "com.example.test";
    std::string tokenId = "12345678";
    int32_t userId = TEST_UID_1;

    bool result = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    EXPECT_FALSE(result);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_HasRight_WithLongDeviceName end");
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS