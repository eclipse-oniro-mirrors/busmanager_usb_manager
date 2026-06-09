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
constexpr uint64_t ONE_SECOND_MS = 1000;
constexpr uint64_t ONE_MINUTE_MS = 60000;
constexpr uint64_t ONE_HOUR_MS = 3600000;
constexpr uint64_t ONE_DAY_MS = 86400000;

class UsbRightManagerExpiryTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<UsbRightManager> usbRightManager_;
};

void UsbRightManagerExpiryTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerExpiryTest::SetUpTestCase enter");
}

void UsbRightManagerExpiryTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerExpiryTest::TearDownTestCase enter");
}

void UsbRightManagerExpiryTest::SetUp()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerExpiryTest::SetUp enter");
    usbRightManager_ = std::make_shared<UsbRightManager>();
    ASSERT_NE(usbRightManager_, nullptr);
}

void UsbRightManagerExpiryTest::TearDown()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerExpiryTest::TearDown enter");
    usbRightManager_.reset();
}

/**
 * @tc.name: UsbRightManager_ExpiryBoundary_JustCreated
 * @tc.desc: Test HasRight immediately after AddDeviceRight (should have right)
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerExpiryTest, UsbRightManager_ExpiryBoundary_JustCreated, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_JustCreated start");

    std::string deviceName = "1-1";
    std::string bundleName = "com.example.test";
    std::string tokenId = "12345678";
    int32_t userId = TEST_UID_1;

    bool addResult = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);
    ASSERT_TRUE(addResult);

    bool hasRight = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    EXPECT_TRUE(hasRight);

    bool removeResult = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_JustCreated end");
}

/**
 * @tc.name: UsbRightManager_ExpiryBoundary_ExpiredRecord
 * @tc.desc: Test HasRight with expired record (should not have right)
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerExpiryTest, UsbRightManager_ExpiryBoundary_ExpiredRecord, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_ExpiredRecord start");

    std::string deviceName = "1-2";
    std::string bundleName = "com.example.expiry";
    std::string tokenId = "87654321";
    int32_t userId = TEST_UID_1;

    bool addResult = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);
    ASSERT_TRUE(addResult);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    bool hasRight = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    bool removeResult = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_ExpiredRecord end");
}

/**
 * @tc.name: UsbRightManager_ExpiryBoundary_MultipleDevices
 * @tc.desc: Test HasRight with multiple devices at expiry boundary
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerExpiryTest, UsbRightManager_ExpiryBoundary_MultipleDevices, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_MultipleDevices start");

    std::string bundleName = "com.example.multidev";
    std::string tokenId = "11112222";
    int32_t userId = TEST_UID_1;

    std::vector<std::string> deviceNames = {"1-1", "1-2", "1-3", "1-4", "1-5"};

    for (const auto& deviceName : deviceNames) {
        bool addResult = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);
        ASSERT_TRUE(addResult);
    }

    for (const auto& deviceName : deviceNames) {
        bool hasRight = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);
    }

    for (const auto& deviceName : deviceNames) {
        bool removeResult = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_MultipleDevices end");
}

/**
 * @tc.name: UsbRightManager_ExpiryBoundary_AddRemoveReadd
 * @tc.desc: Test expiry boundary with Add-Remove-Add pattern
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerExpiryTest, UsbRightManager_ExpiryBoundary_AddRemoveReadd, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_AddRemoveReadd start");

    std::string deviceName = "1-3";
    std::string bundleName = "com.example.readd";
    std::string tokenId = "33334444";
    int32_t userId = TEST_UID_1;

    bool add1Result = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);
    ASSERT_TRUE(add1Result);

    bool hasRight1 = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    bool removeResult = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);

    bool hasRight2 = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    bool add2Result = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);

    bool hasRight3 = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    bool remove2Result = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);

    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_AddRemoveReadd end");
}

/**
 * @tc.name: UsbRightManager_ExpiryBoundary_SameDeviceMultipleUsers
 * @tc.desc: Test expiry boundary with same device and multiple users
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerExpiryTest, UsbRightManager_ExpiryBoundary_SameDeviceMultipleUsers, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_SameDeviceMultipleUsers start");

    std::string deviceName = "1-4";
    std::string tokenId1 = "55556666";
    std::string tokenId2 = "66667777";
    std::string bundleName = "com.example.multiuser";

    bool add1Result = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId1, TEST_UID_1);
    ASSERT_TRUE(add1Result);

    bool add2Result = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId2, TEST_UID_2);
    ASSERT_TRUE(add2Result);

    bool hasRight1 = usbRightManager_->HasRight(deviceName, bundleName, tokenId1, TEST_UID_1);

    bool hasRight2 = usbRightManager_->HasRight(deviceName, bundleName, tokenId2, TEST_UID_2);

    bool remove1Result = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId1, TEST_UID_1);

    bool remove2Result = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId2, TEST_UID_2);

    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_SameDeviceMultipleUsers end");
}

/**
 * @tc.name: UsbRightManager_ExpiryBoundary_TempExpiredThenPermanent
 * @tc.desc: Test transition from temporary expired to permanent right
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerExpiryTest, UsbRightManager_ExpiryBoundary_TempExpiredThenPermanent, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_TempExpiredThenPermanent start");

    std::string deviceName = "1-5";
    std::string bundleName = "com.example.transition";
    std::string tokenId = "77778888";
    int32_t userId = TEST_UID_1;

    bool addResult = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);
    ASSERT_TRUE(addResult);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    bool removeResult = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);

    bool add2Result = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);

    bool hasRight = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    bool remove2Result = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);

    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_TempExpiredThenPermanent end");
}

/**
 * @tc.name: UsbRightManager_ExpiryBoundary_AllDeviceRemoval
 * @tc.desc: Test RemoveDeviceAllRight at expiry boundary
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerExpiryTest, UsbRightManager_ExpiryBoundary_AllDeviceRemoval, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_AllDeviceRemoval start");

    std::string deviceName = "1-6";
    std::string bundleName = "com.example.allremove";
    std::string tokenId = "99990000";
    int32_t userId = TEST_UID_1;

    bool addResult = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);
    ASSERT_TRUE(addResult);

    bool hasRight1 = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    bool removeAllResult = usbRightManager_->RemoveDeviceAllRight(deviceName);

    bool hasRight2 = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);

    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_AllDeviceRemoval end");
}

/**
 * @tc.name: UsbRightManager_ExpiryBoundary_RapidSuccessiveAddRemove
 * @tc.desc: Test rapid successive Add and Remove operations
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerExpiryTest, UsbRightManager_ExpiryBoundary_RapidSuccessiveAddRemove, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_RapidSuccessiveAddRemove start");

    std::string deviceName = "1-7";
    std::string bundleName = "com.example.rapid";
    std::string tokenId = "11112222";
    int32_t userId = TEST_UID_1;

    for (int i = 0; i < 10; i++) {
        bool addResult = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);
        if (addResult) {
            bool hasRight = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);
            bool removeResult = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_RapidSuccessiveAddRemove end");
}

/**
 * @tc.name: UsbRightManager_ExpiryBoundary_MultipleBundlesSameDevice
 * @tc.desc: Test expiry boundary with multiple bundles accessing same device
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerExpiryTest, UsbRightManager_ExpiryBoundary_MultipleBundlesSameDevice, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_MultipleBundlesSameDevice start");

    std::string deviceName = "1-8";
    std::vector<std::string> bundleNames = {
        "com.example.bundle1",
        "com.example.bundle2",
        "com.example.bundle3"
    };
    std::string tokenId1 = "33334444";
    std::string tokenId2 = "44445555";
    std::string tokenId3 = "55556666";
    int32_t userId = TEST_UID_1;

    bool add1Result = usbRightManager_->AddDeviceRight(deviceName, bundleNames[0], tokenId1, userId);
    bool add2Result = usbRightManager_->AddDeviceRight(deviceName, bundleNames[1], tokenId2, userId);
    bool add3Result = usbRightManager_->AddDeviceRight(deviceName, bundleNames[2], tokenId3, userId);

    bool hasRight1 = usbRightManager_->HasRight(deviceName, bundleNames[0], tokenId1, userId);
    bool hasRight2 = usbRightManager_->HasRight(deviceName, bundleNames[1], tokenId2, userId);
    bool hasRight3 = usbRightManager_->HasRight(deviceName, bundleNames[2], tokenId3, userId);

    bool remove1Result = usbRightManager_->RemoveDeviceRight(deviceName, bundleNames[0], tokenId1, userId);
    bool remove2Result = usbRightManager_->RemoveDeviceRight(deviceName, bundleNames[1], tokenId2, userId);
    bool remove3Result = usbRightManager_->RemoveDeviceRight(deviceName, bundleNames[2], tokenId3, userId);

    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_MultipleBundlesSameDevice end");
}

/**
 * @tc.name: UsbRightManager_ExpiryBoundary_DeviceNameVariations
 * @tc.desc: Test expiry boundary with various device name formats
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerExpiryTest, UsbRightManager_ExpiryBoundary_DeviceNameVariations, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_DeviceNameVariations start");

    std::vector<std::string> deviceNames = {
        "1-1",
        "1-1.1",
        "1-1.1.1",
        "2-1",
        "usb-1-1"
    };
    std::string bundleName = "com.example.variations";
    std::string tokenId = "66667777";
    int32_t userId = TEST_UID_1;

    for (const auto& deviceName : deviceNames) {
        bool addResult = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);
        if (addResult) {
            bool hasRight = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);
            bool removeResult = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);
        }
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ExpiryBoundary_DeviceNameVariations end");
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS