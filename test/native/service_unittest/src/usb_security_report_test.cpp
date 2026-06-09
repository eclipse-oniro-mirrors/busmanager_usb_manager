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
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>

#include "usb_security_report.h"
#include "hilog_wrapper.h"
#include "nlohmann/json.hpp"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;
using json = nlohmann::json;

constexpr int64_t TEST_EVENT_ID_1 = 1001;
constexpr int64_t TEST_EVENT_ID_2 = 1002;
constexpr int64_t TEST_EVENT_ID_3 = 1003;
constexpr const char* TEST_VERSION = "1.0.0";
constexpr const char* TEST_VERSION_INVALID = "";

class UsbSecurityReportTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void UsbSecurityReportTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReportTest::SetUpTestCase enter");
}

void UsbSecurityReportTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReportTest::TearDownTestCase enter");
}

void UsbSecurityReportTest::SetUp()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReportTest::SetUp enter");
}

void UsbSecurityReportTest::TearDown()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReportTest::TearDown enter");
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_GetCurrentTime_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_GetCurrentTime_001 enter");
    uint64_t time1 = UsbSecurityReport::GetCurrentTime();
    ASSERT_GT(time1, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    uint64_t time2 = UsbSecurityReport::GetCurrentTime();
    ASSERT_GT(time2, time1);
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_ReportSecurityInfo_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_ReportSecurityInfo_001 enter");
    json securityInfo;
    securityInfo["deviceId"] = "test_device_001";
    securityInfo["eventType"] = "usb_device_connected";
    securityInfo["timestamp"] = UsbSecurityReport::GetCurrentTime();

    UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_1, TEST_VERSION, securityInfo, false);
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_ReportSecurityInfo_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_ReportSecurityInfo_002 enter");
    json securityInfo;
    securityInfo["deviceId"] = "test_device_002";
    securityInfo["eventType"] = "usb_device_disconnected";
    securityInfo["timestamp"] = UsbSecurityReport::GetCurrentTime();

    UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_2, TEST_VERSION, securityInfo, true);
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_ReportSecurityInfo_EmptyJson_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_ReportSecurityInfo_EmptyJson_001 enter");
    json emptyJson;
    UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_3, TEST_VERSION, emptyJson, false);
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_ReportSecurityInfo_LargeJson_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_ReportSecurityInfo_LargeJson_001 enter");
    json largeJson;
    for (int32_t i = 0; i < 100; ++i) {
        largeJson["field_" + std::to_string(i)] = "value_" + std::to_string(i);
    }
    largeJson["timestamp"] = UsbSecurityReport::GetCurrentTime();

    UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_1, TEST_VERSION, largeJson, false);
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_ReportSecurityInfo_MultipleEvents_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_ReportSecurityInfo_MultipleEvents_001 enter");
    for (int32_t i = 0; i < 10; ++i) {
        json securityInfo;
        securityInfo["deviceId"] = "test_device_" + std::to_string(i);
        securityInfo["eventType"] = "usb_event_" + std::to_string(i);
        securityInfo["timestamp"] = UsbSecurityReport::GetCurrentTime();

        UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_1 + i, TEST_VERSION, securityInfo, false);
    }
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_ReportSecurityInfo_ThreadSafety_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_ReportSecurityInfo_ThreadSafety_001 enter");
    const int32_t numThreads = 10;
    std::vector<std::thread> threads;
    std::atomic<int32_t> counter(0);

    for (int32_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([i, &counter]() {
            json securityInfo;
            securityInfo["deviceId"] = "test_device_" + std::to_string(i);
            securityInfo["eventType"] = "thread_event_" + std::to_string(i);
            securityInfo["timestamp"] = UsbSecurityReport::GetCurrentTime();

            UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_1 + i, TEST_VERSION, securityInfo, false);
            counter++;
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    ASSERT_EQ(counter, numThreads);
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_ReportSecurityInfo_AsyncMode_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_ReportSecurityInfo_AsyncMode_001 enter");
    json securityInfo;
    securityInfo["deviceId"] = "test_device_async";
    securityInfo["eventType"] = "async_event";
    securityInfo["timestamp"] = UsbSecurityReport::GetCurrentTime();

    UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_1, TEST_VERSION, securityInfo, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_ReportSecurityInfo_SyncMode_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_ReportSecurityInfo_SyncMode_001 enter");
    json securityInfo;
    securityInfo["deviceId"] = "test_device_sync";
    securityInfo["eventType"] = "sync_event";
    securityInfo["timestamp"] = UsbSecurityReport::GetCurrentTime();

    UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_1, TEST_VERSION, securityInfo, false);
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_ReportSecurityInfo_DifferentVersions_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_ReportSecurityInfo_DifferentVersions_001 enter");
    json securityInfo;
    securityInfo["deviceId"] = "test_device_version";
    securityInfo["eventType"] = "version_event";
    securityInfo["timestamp"] = UsbSecurityReport::GetCurrentTime();

    UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_1, "1.0.0", securityInfo, false);
    UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_2, "2.0.0", securityInfo, false);
    UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_3, "3.0.0", securityInfo, false);
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_GetCurrentTime_Monotonic_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_GetCurrentTime_Monotonic_001 enter");
    uint64_t prevTime = UsbSecurityReport::GetCurrentTime();

    for (int32_t i = 0; i < 10; ++i) {
        uint64_t currentTime = UsbSecurityReport::GetCurrentTime();
        ASSERT_GE(currentTime, prevTime);
        prevTime = currentTime;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_ReportSecurityInfo_SpecialCharacters_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_ReportSecurityInfo_SpecialCharacters_001 enter");
    json securityInfo;
    securityInfo["deviceId"] = "test_<>\"&device";
    securityInfo["eventType"] = "event_with_特殊字符";
    securityInfo["description"] = "Description with \\n\\t\\r";
    securityInfo["timestamp"] = UsbSecurityReport::GetCurrentTime();

    UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_1, TEST_VERSION, securityInfo, false);
}

HWTEST_F(UsbSecurityReportTest, UsbSecurityReport_ReportSecurityInfo_NullValues_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSecurityReport_ReportSecurityInfo_NullValues_001 enter");
    json securityInfo;
    securityInfo["deviceId"] = nullptr;
    securityInfo["eventType"] = "null_event";
    securityInfo["timestamp"] = UsbSecurityReport::GetCurrentTime();

    UsbSecurityReport::ReportSecurityInfo(TEST_EVENT_ID_1, TEST_VERSION, securityInfo, false);
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS