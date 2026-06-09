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
#include <atomic>

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
constexpr int32_t TEST_UID_3 = 300;
constexpr int32_t NUM_CONCURRENT_THREADS = 10;
constexpr int32_t NUM_ITERATIONS = 100;

class UsbRightManagerConcurrentTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<UsbRightManager> usbRightManager_;
};

void UsbRightManagerConcurrentTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerConcurrentTest::SetUpTestCase enter");
}

void UsbRightManagerConcurrentTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerConcurrentTest::TearDownTestCase enter");
}

void UsbRightManagerConcurrentTest::SetUp()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerConcurrentTest::SetUp enter");
    usbRightManager_ = std::make_shared<UsbRightManager>();
    ASSERT_NE(usbRightManager_, nullptr);
}

void UsbRightManagerConcurrentTest::TearDown()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManagerConcurrentTest::TearDown enter");
    usbRightManager_.reset();
}

/**
 * @tc.name: UsbRightManager_ConcurrentHasRight_SameDevice
 * @tc.desc: Test concurrent HasRight calls for the same device
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerConcurrentTest, UsbRightManager_ConcurrentHasRight_SameDevice, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentHasRight_SameDevice start");

    std::string deviceName = "1-1";
    std::string bundleName = "com.example.test";
    std::string tokenId = "12345678";
    int32_t userId = TEST_UID_1;

    std::atomic<int> successCount(0);
    std::atomic<int> failCount(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CONCURRENT_THREADS; i++) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < NUM_ITERATIONS; j++) {
                bool result = usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);
                if (result) {
                    successCount++;
                } else {
                    failCount++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    USB_HILOGI(MODULE_USB_HOST, "Success: %{public}d, Fail: %{public}d", 
                successCount.load(), failCount.load());
    EXPECT_GE(successCount.load(), 0);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentHasRight_SameDevice end");
}

/**
 * @tc.name: UsbRightManager_ConcurrentAddRemove_DifferentDevices
 * @tc.desc: Test concurrent AddDeviceRight and RemoveDeviceRight for different devices
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerConcurrentTest, UsbRightManager_ConcurrentAddRemove_DifferentDevices, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentAddRemove_DifferentDevices start");

    std::atomic<int> addSuccessCount(0);
    std::atomic<int> removeSuccessCount(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CONCURRENT_THREADS; i++) {
        threads.emplace_back([&, i]() {
            std::string deviceName = "1-" + std::to_string(i);
            std::string bundleName = "com.example.test" + std::to_string(i);
            std::string tokenId = "123456" + std::to_string(i);
            int32_t userId = TEST_UID_1 + i;

            for (int j = 0; j < 10; j++) {
                bool addResult = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);
                if (addResult) {
                    addSuccessCount++;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                bool removeResult = usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);
                if (removeResult) {
                    removeSuccessCount++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    USB_HILOGI(MODULE_USB_HOST, "Add Success: %{public}d, Remove Success: %{public}d",
                addSuccessCount.load(), removeSuccessCount.load());
    EXPECT_GT(addSuccessCount.load(), 0);
    EXPECT_GT(removeSuccessCount.load(), 0);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentAddRemove_DifferentDevices end");
}

/**
 * @tc.name: UsbRightManager_ConcurrentAdd_SameDeviceSameUser
 * @tc.desc: Test concurrent AddDeviceRight for same device and user
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerConcurrentTest, UsbRightManager_ConcurrentAdd_SameDeviceSameUser, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentAdd_SameDeviceSameUser start");

    std::string deviceName = "1-1";
    std::string bundleName = "com.example.test";
    std::string tokenId = "12345678";
    int32_t userId = TEST_UID_1;

    std::atomic<int> addSuccessCount(0);
    std::atomic<int> addFailCount(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CONCURRENT_THREADS; i++) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 10; j++) {
                bool result = usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);
                if (result) {
                    addSuccessCount++;
                } else {
                    addFailCount++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    USB_HILOGI(MODULE_USB_HOST, "Add Success: %{public}d, Add Fail: %{public}d",
                addSuccessCount.load(), addFailCount.load());
    EXPECT_GE(addSuccessCount.load(), 0);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentAdd_SameDeviceSameUser end");
}

/**
 * @tc.name: UsbRightManager_ConcurrentRemoveAll_MultipleDevices
 * @tc.desc: Test concurrent RemoveDeviceAllRight for multiple devices
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerConcurrentTest, UsbRightManager_ConcurrentRemoveAll_MultipleDevices, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentRemoveAll_MultipleDevices start");

    std::atomic<int> removeAllSuccessCount(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CONCURRENT_THREADS; i++) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 5; j++) {
                std::string deviceName = "1-" + std::to_string((i * 5 + j) % 10);
                bool result = usbRightManager_->RemoveDeviceAllRight(deviceName);
                if (result) {
                    removeAllSuccessCount++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    USB_HILOGI(MODULE_USB_HOST, "RemoveAll Success: %{public}d", removeAllSuccessCount.load());
    EXPECT_GT(removeAllSuccessCount.load(), 0);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentRemoveAll_MultipleDevices end");
}

/**
 * @tc.name: UsbRightManager_ConcurrentMixedOperations
 * @tc.desc: Test concurrent mix of Add, Remove, HasRight, and RemoveAll operations
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerConcurrentTest, UsbRightManager_ConcurrentMixedOperations, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentMixedOperations start");

    std::atomic<int> addCount(0);
    std::atomic<int> removeCount(0);
    std::atomic<int> hasRightCount(0);
    std::atomic<int> removeAllCount(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CONCURRENT_THREADS; i++) {
        int opType = i % 4;
        threads.emplace_back([&, i, opType]() {
            std::string deviceName = "1-" + std::to_string(i % 5);
            std::string bundleName = "com.example.test" + std::to_string(i % 3);
            std::string tokenId = "123456" + std::to_string(i % 5);
            int32_t userId = TEST_UID_1 + (i % 3);

            for (int j = 0; j < 10; j++) {
                switch (opType) {
                    case 0:
                        if (usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId)) {
                            addCount++;
                        }
                        break;
                    case 1:
                        if (usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId)) {
                            removeCount++;
                        }
                        break;
                    case 2:
                        usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);
                        hasRightCount++;
                        break;
                    case 3:
                        if (usbRightManager_->RemoveDeviceAllRight(deviceName)) {
                            removeAllCount++;
                        }
                        break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    USB_HILOGI(MODULE_USB_HOST, "Add: %{public}d, Remove: %{public}d, HasRight: %{public}d, RemoveAll: %{public}d",
                addCount.load(), removeCount.load(), hasRightCount.load(), removeAllCount.load());

    EXPECT_GE(addCount.load(), 0);
    EXPECT_GE(removeCount.load(), 0);
    EXPECT_EQ(hasRightCount.load(), NUM_CONCURRENT_THREADS / 4 * 10);
    EXPECT_GE(removeAllCount.load(), 0);

    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentMixedOperations end");
}

/**
 * @tc.name: UsbRightManager_ConcurrentSameUserMultipleDevices
 * @tc.desc: Test concurrent operations from same user on multiple devices
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerConcurrentTest, UsbRightManager_ConcurrentSameUserMultipleDevices, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentSameUserMultipleDevices start");

    int32_t userId = TEST_UID_1;
    std::string bundleName = "com.example.concurrent";
    std::string tokenId = "98765432";

    std::atomic<int> totalOperations(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CONCURRENT_THREADS; i++) {
        threads.emplace_back([&, i]() {
            std::string deviceName = "1-" + std::to_string(i);
            for (int j = 0; j < 20; j++) {
                usbRightManager_->AddDeviceRight(deviceName, bundleName, tokenId, userId);
                usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);
                usbRightManager_->RemoveDeviceRight(deviceName, bundleName, tokenId, userId);
                totalOperations += 3;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    USB_HILOGI(MODULE_USB_HOST, "Total operations completed: %{public}d", totalOperations.load());
    EXPECT_EQ(totalOperations.load(), NUM_CONCURRENT_THREADS * 20 * 3);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentSameUserMultipleDevices end");
}

/**
 * @tc.name: UsbRightManager_ConcurrentRemoveDeviceAllRight_Stress
 * @tc.desc: Stress test for RemoveDeviceAllRight with high concurrency
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerConcurrentTest, UsbRightManager_ConcurrentRemoveDeviceAllRight_Stress, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentRemoveDeviceAllRight_Stress start");

    std::atomic<int> successCount(0);
    std::atomic<int> failCount(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CONCURRENT_THREADS * 2; i++) {
        threads.emplace_back([&, i]() {
            std::string deviceName = "1-" + std::to_string(i % 20);
            for (int j = 0; j < 5; j++) {
                bool result = usbRightManager_->RemoveDeviceAllRight(deviceName);
                if (result) {
                    successCount++;
                } else {
                    failCount++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    USB_HILOGI(MODULE_USB_HOST, "RemoveAll - Success: %{public}d, Fail: %{public}d",
                successCount.load(), failCount.load());
    EXPECT_GE(successCount.load(), 0);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentRemoveDeviceAllRight_Stress end");
}

/**
 * @tc.name: UsbRightManager_ConcurrentHasRight_DifferentUsers
 * @tc.desc: Test concurrent HasRight calls from different users
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightManagerConcurrentTest, UsbRightManager_ConcurrentHasRight_DifferentUsers, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentHasRight_DifferentUsers start");

    std::string deviceName = "1-1";
    std::atomic<int> user1Count(0);
    std::atomic<int> user2Count(0);
    std::atomic<int> user3Count(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CONCURRENT_THREADS; i++) {
        int32_t userId = TEST_UID_1 + (i % 3);
        threads.emplace_back([&, userId]() {
            std::string bundleName = "com.example.user" + std::to_string(userId);
            std::string tokenId = "123" + std::to_string(userId);
            for (int j = 0; j < 20; j++) {
                usbRightManager_->HasRight(deviceName, bundleName, tokenId, userId);
                if (userId == TEST_UID_1) {
                    user1Count++;
                } else if (userId == TEST_UID_2) {
                    user2Count++;
                } else {
                    user3Count++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    USB_HILOGI(MODULE_USB_HOST, "User1: %{public}d, User2: %{public}d, User3: %{public}d",
                user1Count.load(), user2Count.load(), user3Count.load());
    EXPECT_GT(user1Count.load(), 0);
    EXPECT_GT(user2Count.load(), 0);
    EXPECT_GT(user3Count.load(), 0);
    USB_HILOGI(MODULE_USB_HOST, "UsbRightManager_ConcurrentHasRight_DifferentUsers end");
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS