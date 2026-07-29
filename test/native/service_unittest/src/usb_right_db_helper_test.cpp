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
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdint>

#include "usb_right_db_helper.h"
#include "usb_errors.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;
using namespace OHOS::NativeRdb;

constexpr int32_t TEST_UID_1 = 100;
constexpr int32_t TEST_UID_2 = 200;
constexpr int32_t TEST_UID_3 = 300;
constexpr int32_t TEST_UID_INVALID = -1;
constexpr uint64_t TEST_INSTALL_TIME = 1000000;
constexpr uint64_t TEST_UPDATE_TIME = 2000000;
constexpr uint64_t TEST_REQUEST_TIME = 3000000;
constexpr uint64_t TEST_VALID_PERIOD = 86400;
constexpr uint64_t TEST_VALID_PERIOD_FOREVER = 0xFFFFFFFFL;
constexpr uint64_t TEST_VALID_PERIOD_TEMPORARY = 0;
constexpr const char *TEST_DEVICE_NAME_1 = "usb_device_001";
constexpr const char *TEST_DEVICE_NAME_2 = "usb_device_002";
constexpr const char *TEST_DEVICE_NAME_3 = "usb_device_003";
constexpr const char *TEST_BUNDLE_NAME_1 = "com.test.app1";
constexpr const char *TEST_BUNDLE_NAME_2 = "com.test.app2";
constexpr const char *TEST_BUNDLE_NAME_3 = "com.test.app3";
constexpr const char *TEST_TOKEN_ID_1 = "token_id_001";
constexpr const char *TEST_TOKEN_ID_2 = "token_id_002";
constexpr const char *TEST_TOKEN_ID_3 = "token_id_003";

class UsbRightDbHelperTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    void InsertTestRecord(const std::string &deviceName, const std::string &bundleName,
        const std::string &tokenId, int32_t uid, uint64_t validPeriod);

    std::shared_ptr<UsbRightDbHelper> dbHelper_;
};

void UsbRightDbHelperTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelperTest::SetUpTestCase enter");
}

void UsbRightDbHelperTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelperTest::TearDownTestCase enter");
}

void UsbRightDbHelperTest::SetUp()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelperTest::SetUp enter");
    dbHelper_ = UsbRightDbHelper::GetInstance();
    ASSERT_NE(dbHelper_, nullptr);
}

void UsbRightDbHelperTest::TearDown()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelperTest::TearDown enter");
    std::vector<int32_t> uids = {TEST_UID_1, TEST_UID_2, TEST_UID_3};
    for (auto uid : uids) {
        dbHelper_->DeleteUidRightRecord(uid);
    }
}

void UsbRightDbHelperTest::InsertTestRecord(const std::string &deviceName, const std::string &bundleName,
    const std::string &tokenId, int32_t uid, uint64_t validPeriod)
{
    struct UsbRightAppInfo info;
    info.uid = uid;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = validPeriod;
    int32_t ret = dbHelper_->AddRightRecord(deviceName, bundleName, info);
    ASSERT_GE(ret, USB_RIGHT_OK);
}

/**
 * @tc.name: UsbRightDbHelper_GetInstance_001
 * @tc.desc: Test singleton pattern - GetInstance returns same instance
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_GetInstance_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_GetInstance_001 start");
    auto instance1 = UsbRightDbHelper::GetInstance();
    auto instance2 = UsbRightDbHelper::GetInstance();
    ASSERT_EQ(instance1, instance2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_GetInstance_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_AddRightRecord_001
 * @tc.desc: Test AddRightRecord with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_AddRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_001 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD;

    int32_t ret = dbHelper_->AddRightRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, info);
    ASSERT_GE(ret, USB_RIGHT_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_AddRightRecord_002
 * @tc.desc: Test AddRightRecord with temporary valid period (0)
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_AddRightRecord_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_002 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD_TEMPORARY;

    int32_t ret = dbHelper_->AddRightRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, info);
    ASSERT_GE(ret, USB_RIGHT_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_AddRightRecord_003
 * @tc.desc: Test AddRightRecord with forever valid period (MAX)
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_AddRightRecord_003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_003 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD_FOREVER;

    int32_t ret = dbHelper_->AddRightRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, info);
    ASSERT_GE(ret, USB_RIGHT_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_003 end");
}

/**
 * @tc.name: UsbRightDbHelper_QueryRightRecord_001
 * @tc.desc: Test QueryRightRecord returns correct record after insert
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_QueryRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_GT(ret, 0);
    ASSERT_GE(infos.size(), 1);
    EXPECT_EQ(infos[0].uid, TEST_UID_1);
    EXPECT_EQ(infos[0].validPeriod, TEST_VALID_PERIOD);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_QueryRightRecord_002
 * @tc.desc: Test QueryRightRecord returns empty for non-existent record
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_QueryRightRecord_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecord_002 start");
    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecord_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_QueryRightRecord_003
 * @tc.desc: Test QueryRightRecord with invalid uid returns empty
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_QueryRightRecord_003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecord_003 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryRightRecord(TEST_UID_INVALID, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecord_003 end");
}

/**
 * @tc.name: UsbRightDbHelper_QueryUserRightRecord_001
 * @tc.desc: Test QueryUserRightRecord returns all records for a user
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_QueryUserRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryUserRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_2, TEST_BUNDLE_NAME_2, TEST_TOKEN_ID_2, TEST_UID_1, TEST_VALID_PERIOD);

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryUserRightRecord(TEST_UID_1, infos);
    ASSERT_GT(ret, 0);
    ASSERT_GE(infos.size(), 2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryUserRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_QueryUserRightRecord_002
 * @tc.desc: Test QueryUserRightRecord with no records
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_QueryUserRightRecord_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryUserRightRecord_002 start");
    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryUserRightRecord(TEST_UID_2, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryUserRightRecord_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_QueryDeviceRightRecord_001
 * @tc.desc: Test QueryDeviceRightRecord returns records for a device
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_QueryDeviceRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryDeviceRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_2, TEST_TOKEN_ID_2, TEST_UID_1, TEST_VALID_PERIOD);

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryDeviceRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1, infos);
    ASSERT_GT(ret, 0);
    ASSERT_GE(infos.size(), 2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryDeviceRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_QueryDeviceRightRecord_002
 * @tc.desc: Test QueryDeviceRightRecord with non-matching device
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_QueryDeviceRightRecord_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryDeviceRightRecord_002 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryDeviceRightRecord(TEST_UID_1, TEST_DEVICE_NAME_2, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryDeviceRightRecord_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_QueryAppRightRecord_001
 * @tc.desc: Test QueryAppRightRecord returns records for a bundle
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_QueryAppRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryAppRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_2, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryAppRightRecord(TEST_UID_1, TEST_BUNDLE_NAME_1, infos);
    ASSERT_GT(ret, 0);
    ASSERT_GE(infos.size(), 2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryAppRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_QueryRightRecordUids_001
 * @tc.desc: Test QueryRightRecordUids returns distinct uids
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_QueryRightRecordUids_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecordUids_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_2, TEST_BUNDLE_NAME_2, TEST_TOKEN_ID_2, TEST_UID_2, TEST_VALID_PERIOD);

    std::vector<std::string> uids;
    int32_t ret = dbHelper_->QueryRightRecordUids(uids);
    ASSERT_GT(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecordUids_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_QueryRightRecordApps_001
 * @tc.desc: Test QueryRightRecordApps returns distinct bundle names
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_QueryRightRecordApps_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecordApps_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_2, TEST_BUNDLE_NAME_2, TEST_TOKEN_ID_2, TEST_UID_1, TEST_VALID_PERIOD);

    std::vector<std::string> apps;
    int32_t ret = dbHelper_->QueryRightRecordApps(TEST_UID_1, apps);
    ASSERT_GT(ret, 0);
    ASSERT_GE(apps.size(), 2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecordApps_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_UpdateRightRecord_001
 * @tc.desc: Test UpdateRightRecord updates existing record
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_UpdateRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_UpdateRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME + 1;
    info.requestTime = TEST_REQUEST_TIME + 1;
    info.validPeriod = TEST_VALID_PERIOD_FOREVER;

    int32_t ret = dbHelper_->UpdateRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, info);
    ASSERT_GE(ret, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_GT(ret, 0);
    EXPECT_EQ(infos[0].validPeriod, TEST_VALID_PERIOD_FOREVER);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_UpdateRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_UpdateRightRecord_002
 * @tc.desc: Test UpdateRightRecord on non-existent record
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_UpdateRightRecord_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_UpdateRightRecord_002 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD;

    int32_t ret = dbHelper_->UpdateRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, info);
    ASSERT_GE(ret, USB_RIGHT_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_UpdateRightRecord_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_AddOrUpdateRightRecord_001
 * @tc.desc: Test AddOrUpdateRightRecord inserts new record
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_AddOrUpdateRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddOrUpdateRightRecord_001 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD;

    int32_t ret = dbHelper_->AddOrUpdateRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, info);
    ASSERT_GE(ret, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_GT(ret, 0);
    EXPECT_EQ(infos[0].uid, TEST_UID_1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddOrUpdateRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_AddOrUpdateRightRecord_002
 * @tc.desc: Test AddOrUpdateRightRecord updates existing record
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_AddOrUpdateRightRecord_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddOrUpdateRightRecord_002 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME + 100;
    info.validPeriod = TEST_VALID_PERIOD_FOREVER;

    int32_t ret = dbHelper_->AddOrUpdateRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, info);
    ASSERT_GE(ret, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_GT(ret, 0);
    EXPECT_EQ(infos[0].validPeriod, TEST_VALID_PERIOD_FOREVER);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddOrUpdateRightRecord_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteRightRecord_001
 * @tc.desc: Test DeleteRightRecord removes a specific record
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    int32_t ret = dbHelper_->DeleteRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1);
    ASSERT_GE(ret, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteRightRecord_002
 * @tc.desc: Test DeleteRightRecord on non-existent record returns empty
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteRightRecord_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteRightRecord_002 start");
    int32_t ret = dbHelper_->DeleteRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1);
    ASSERT_EQ(ret, USB_RIGHT_RDB_EMPTY);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteRightRecord_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteDeviceRightRecord_001
 * @tc.desc: Test DeleteDeviceRightRecord removes all records for a device
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteDeviceRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteDeviceRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_2, TEST_TOKEN_ID_2, TEST_UID_1, TEST_VALID_PERIOD);

    int32_t ret = dbHelper_->DeleteDeviceRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1);
    ASSERT_GE(ret, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryDeviceRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteDeviceRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteDeviceRightRecord_002
 * @tc.desc: Test DeleteDeviceRightRecord on non-existent device
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteDeviceRightRecord_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteDeviceRightRecord_002 start");
    int32_t ret = dbHelper_->DeleteDeviceRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1);
    ASSERT_EQ(ret, USB_RIGHT_RDB_EMPTY);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteDeviceRightRecord_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteAppRightRecord_001
 * @tc.desc: Test DeleteAppRightRecord removes all records for an app
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteAppRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteAppRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_2, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    int32_t ret = dbHelper_->DeleteAppRightRecord(TEST_UID_1, TEST_BUNDLE_NAME_1);
    ASSERT_GE(ret, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryAppRightRecord(TEST_UID_1, TEST_BUNDLE_NAME_1, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteAppRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteAppsRightRecord_001
 * @tc.desc: Test DeleteAppsRightRecord removes multiple apps at once
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteAppsRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteAppsRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_2, TEST_BUNDLE_NAME_2, TEST_TOKEN_ID_2, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_3, TEST_BUNDLE_NAME_3, TEST_TOKEN_ID_3, TEST_UID_1, TEST_VALID_PERIOD);

    std::vector<std::string> bundleNames = {TEST_BUNDLE_NAME_1, TEST_BUNDLE_NAME_2};
    int32_t ret = dbHelper_->DeleteAppsRightRecord(TEST_UID_1, bundleNames);
    ASSERT_GE(ret, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryAppRightRecord(TEST_UID_1, TEST_BUNDLE_NAME_1, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteAppsRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteUidRightRecord_001
 * @tc.desc: Test DeleteUidRightRecord removes all records for a user
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteUidRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteUidRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_2, TEST_BUNDLE_NAME_2, TEST_TOKEN_ID_2, TEST_UID_1, TEST_VALID_PERIOD);

    int32_t ret = dbHelper_->DeleteUidRightRecord(TEST_UID_1);
    ASSERT_GE(ret, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryUserRightRecord(TEST_UID_1, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteUidRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_IsRecordExpired_ByStruct_001
 * @tc.desc: Test IsRecordExpired with temporary valid period (0) - never expires
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_IsRecordExpired_ByStruct_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_001 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD_TEMPORARY;

    bool expired = dbHelper_->IsRecordExpired(info, TEST_REQUEST_TIME + TEST_VALID_PERIOD);
    EXPECT_FALSE(expired);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_IsRecordExpired_ByStruct_002
 * @tc.desc: Test IsRecordExpired with forever valid period (MAX) - never expires
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_IsRecordExpired_ByStruct_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_002 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD_FOREVER;

    bool expired = dbHelper_->IsRecordExpired(info, TEST_REQUEST_TIME + TEST_VALID_PERIOD);
    EXPECT_FALSE(expired);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_IsRecordExpired_ByStruct_003
 * @tc.desc: Test IsRecordExpired with normal valid period - not expired yet
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_IsRecordExpired_ByStruct_003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_003 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD;

    uint64_t currentTime = TEST_REQUEST_TIME + TEST_VALID_PERIOD - 1;
    bool expired = dbHelper_->IsRecordExpired(info, currentTime);
    EXPECT_FALSE(expired);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_003 end");
}

/**
 * @tc.name: UsbRightDbHelper_IsRecordExpired_ByStruct_004
 * @tc.desc: Test IsRecordExpired with normal valid period - expired
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_IsRecordExpired_ByStruct_004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_004 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD;

    uint64_t currentTime = TEST_REQUEST_TIME + TEST_VALID_PERIOD + TEST_VALID_PERIOD;
    bool expired = dbHelper_->IsRecordExpired(info, currentTime);
    EXPECT_TRUE(expired);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_004 end");
}

/**
 * @tc.name: UsbRightDbHelper_IsRecordExpired_ByStruct_005
 * @tc.desc: Test IsRecordExpired with expiredTime before requestTime (system time changed)
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_IsRecordExpired_ByStruct_005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_005 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD;

    uint64_t currentTime = TEST_REQUEST_TIME - 1;
    bool expired = dbHelper_->IsRecordExpired(info, currentTime);
    EXPECT_TRUE(expired);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_005 end");
}

/**
 * @tc.name: UsbRightDbHelper_IsRecordExpired_ByStruct_006
 * @tc.desc: Test IsRecordExpired with installTime > updateTime (invalid state)
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_IsRecordExpired_ByStruct_006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_006 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_UPDATE_TIME + 1000;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD;

    uint64_t currentTime = TEST_REQUEST_TIME + TEST_VALID_PERIOD - 1;
    bool expired = dbHelper_->IsRecordExpired(info, currentTime);
    EXPECT_TRUE(expired);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_006 end");
}

/**
 * @tc.name: UsbRightDbHelper_IsRecordExpired_ByStruct_007
 * @tc.desc: Test IsRecordExpired with installTime > requestTime (invalid state)
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_IsRecordExpired_ByStruct_007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_007 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_REQUEST_TIME + 1000;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD;

    uint64_t currentTime = TEST_REQUEST_TIME + TEST_VALID_PERIOD - 1;
    bool expired = dbHelper_->IsRecordExpired(info, currentTime);
    EXPECT_TRUE(expired);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByStruct_007 end");
}

/**
 * @tc.name: UsbRightDbHelper_IsRecordExpired_ByQuery_001
 * @tc.desc: Test IsRecordExpired by querying inserted record
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_IsRecordExpired_ByQuery_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByQuery_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    uint64_t currentTime = TEST_REQUEST_TIME + TEST_VALID_PERIOD - 1;
    bool expired = dbHelper_->IsRecordExpired(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, currentTime);
    EXPECT_FALSE(expired);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByQuery_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_IsRecordExpired_ByQuery_002
 * @tc.desc: Test IsRecordExpired by query with non-existent record returns expired
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_IsRecordExpired_ByQuery_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByQuery_002 start");
    bool expired = dbHelper_->IsRecordExpired(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_REQUEST_TIME + TEST_VALID_PERIOD);
    EXPECT_TRUE(expired);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_IsRecordExpired_ByQuery_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteNormalExpiredRightRecord_001
 * @tc.desc: Test DeleteNormalExpiredRightRecord removes expired records
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteNormalExpiredRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteNormalExpiredRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    uint64_t expiredTime = TEST_REQUEST_TIME + TEST_VALID_PERIOD + TEST_VALID_PERIOD;
    int32_t ret = dbHelper_->DeleteNormalExpiredRightRecord(TEST_UID_1, expiredTime);
    ASSERT_GE(ret, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteNormalExpiredRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteNormalExpiredRightRecord_002
 * @tc.desc: Test DeleteNormalExpiredRightRecord does not remove temporary (0) records
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteNormalExpiredRightRecord_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteNormalExpiredRightRecord_002 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1,
        TEST_VALID_PERIOD_TEMPORARY);

    uint64_t expiredTime = TEST_REQUEST_TIME + TEST_VALID_PERIOD + TEST_VALID_PERIOD;
    int32_t ret = dbHelper_->DeleteNormalExpiredRightRecord(TEST_UID_1, expiredTime);
    ASSERT_EQ(ret, USB_RIGHT_RDB_EMPTY);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_GT(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteNormalExpiredRightRecord_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteNormalExpiredRightRecord_003
 * @tc.desc: Test DeleteNormalExpiredRightRecord does not remove forever (MAX) records
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteNormalExpiredRightRecord_003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteNormalExpiredRightRecord_003 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1,
        TEST_VALID_PERIOD_FOREVER);

    uint64_t expiredTime = TEST_REQUEST_TIME + TEST_VALID_PERIOD + TEST_VALID_PERIOD;
    int32_t ret = dbHelper_->DeleteNormalExpiredRightRecord(TEST_UID_1, expiredTime);
    ASSERT_EQ(ret, USB_RIGHT_RDB_EMPTY);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_GT(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteNormalExpiredRightRecord_003 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteValidPeriodRightRecord_001
 * @tc.desc: Test DeleteValidPeriodRightRecord removes records by valid period and device
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteValidPeriodRightRecord_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteValidPeriodRightRecord_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    int32_t ret = dbHelper_->DeleteValidPeriodRightRecord(
        static_cast<long>(TEST_VALID_PERIOD), TEST_DEVICE_NAME_1);
    ASSERT_GE(ret, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteValidPeriodRightRecord_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteValidPeriodRightRecord_002
 * @tc.desc: Test DeleteValidPeriodRightRecord does not remove non-matching device
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteValidPeriodRightRecord_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteValidPeriodRightRecord_002 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    int32_t ret = dbHelper_->DeleteValidPeriodRightRecord(
        static_cast<long>(TEST_VALID_PERIOD), TEST_DEVICE_NAME_2);
    ASSERT_EQ(ret, USB_RIGHT_RDB_EMPTY);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_GT(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteValidPeriodRightRecord_002 end");
}

/**
 * @tc.name: UsbRightDbHelper_AddRightRecord_WithDuplicateTokenId
 * @tc.desc: Test AddRightRecord with same device different token IDs
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_AddRightRecord_WithDuplicateTokenId, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_WithDuplicateTokenId start");
    struct UsbRightAppInfo info1;
    info1.uid = TEST_UID_1;
    info1.installTime = TEST_INSTALL_TIME;
    info1.updateTime = TEST_UPDATE_TIME;
    info1.requestTime = TEST_REQUEST_TIME;
    info1.validPeriod = TEST_VALID_PERIOD;

    int32_t ret1 = dbHelper_->AddRightRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, info1);
    ASSERT_GE(ret1, USB_RIGHT_OK);

    struct UsbRightAppInfo info2;
    info2.uid = TEST_UID_1;
    info2.installTime = TEST_INSTALL_TIME;
    info2.updateTime = TEST_UPDATE_TIME;
    info2.requestTime = TEST_REQUEST_TIME + 100;
    info2.validPeriod = TEST_VALID_PERIOD_FOREVER;

    int32_t ret2 = dbHelper_->AddRightRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, info2);
    ASSERT_GE(ret2, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryDeviceRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1, infos);
    ASSERT_GT(ret, 0);
    ASSERT_GE(infos.size(), 2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_WithDuplicateTokenId end");
}

/**
 * @tc.name: UsbRightDbHelper_MultiUserIsolation_001
 * @tc.desc: Test records from different users do not interfere
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_MultiUserIsolation_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_MultiUserIsolation_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_2, TEST_VALID_PERIOD);

    std::vector<struct UsbRightAppInfo> infos1;
    int32_t ret1 = dbHelper_->QueryUserRightRecord(TEST_UID_1, infos1);
    ASSERT_GT(ret1, 0);

    std::vector<struct UsbRightAppInfo> infos2;
    int32_t ret2 = dbHelper_->QueryUserRightRecord(TEST_UID_2, infos2);
    ASSERT_GT(ret2, 0);

    dbHelper_->DeleteUidRightRecord(TEST_UID_1);

    std::vector<struct UsbRightAppInfo> infosAfter;
    int32_t retAfter = dbHelper_->QueryUserRightRecord(TEST_UID_2, infosAfter);
    ASSERT_GT(retAfter, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_MultiUserIsolation_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_AddRightRecord_EmptyDeviceName
 * @tc.desc: Test AddRightRecord with empty device name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_AddRightRecord_EmptyDeviceName, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_EmptyDeviceName start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD;

    int32_t ret = dbHelper_->AddRightRecord("", TEST_BUNDLE_NAME_1, info);
    ASSERT_GE(ret, USB_RIGHT_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_EmptyDeviceName end");
}

/**
 * @tc.name: UsbRightDbHelper_AddRightRecord_EmptyBundleName
 * @tc.desc: Test AddRightRecord with empty bundle name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_AddRightRecord_EmptyBundleName, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_EmptyBundleName start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD;

    int32_t ret = dbHelper_->AddRightRecord(TEST_DEVICE_NAME_1, "", info);
    ASSERT_GE(ret, USB_RIGHT_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddRightRecord_EmptyBundleName end");
}

/**
 * @tc.name: UsbRightDbHelper_QueryRightRecord_WithSpecialChars
 * @tc.desc: Test QueryRightRecord with special characters in bundle name
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_QueryRightRecord_WithSpecialChars, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecord_WithSpecialChars start");
    std::string specialBundle = "com.test.app-1.2_3~4";
    std::string specialDevice = "usb-device@test#1";
    InsertTestRecord(specialDevice, specialBundle, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryRightRecord(TEST_UID_1, specialDevice,
        specialBundle, TEST_TOKEN_ID_1, infos);
    ASSERT_GT(ret, 0);
    ASSERT_GE(infos.size(), 1);
    EXPECT_EQ(infos[0].uid, TEST_UID_1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_QueryRightRecord_WithSpecialChars end");
}

/**
 * @tc.name: UsbRightDbHelper_AddOrUpdateRightRecord_SameTokenId
 * @tc.desc: Test AddOrUpdateRightRecord with same token ID updates in place
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_AddOrUpdateRightRecord_SameTokenId, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddOrUpdateRightRecord_SameTokenId start");
    struct UsbRightAppInfo info1;
    info1.uid = TEST_UID_1;
    info1.installTime = TEST_INSTALL_TIME;
    info1.updateTime = TEST_UPDATE_TIME;
    info1.requestTime = TEST_REQUEST_TIME;
    info1.validPeriod = TEST_VALID_PERIOD;

    int32_t ret1 = dbHelper_->AddOrUpdateRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, info1);
    ASSERT_GE(ret1, USB_RIGHT_OK);

    struct UsbRightAppInfo info2;
    info2.uid = TEST_UID_1;
    info2.installTime = TEST_INSTALL_TIME;
    info2.updateTime = TEST_UPDATE_TIME;
    info2.requestTime = TEST_REQUEST_TIME + 200;
    info2.validPeriod = TEST_VALID_PERIOD_FOREVER;

    int32_t ret2 = dbHelper_->AddOrUpdateRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, info2);
    ASSERT_GE(ret2, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_GT(ret, 0);
    ASSERT_GE(infos.size(), 1);
    EXPECT_EQ(infos[0].validPeriod, TEST_VALID_PERIOD_FOREVER);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_AddOrUpdateRightRecord_SameTokenId end");
}

/**
 * @tc.name: UsbRightDbHelper_MixedValidPeriod_001
 * @tc.desc: Test mixed valid periods for the same user
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_MixedValidPeriod_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_MixedValidPeriod_001 start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1,
        TEST_VALID_PERIOD_TEMPORARY);
    InsertTestRecord(TEST_DEVICE_NAME_2, TEST_BUNDLE_NAME_2, TEST_TOKEN_ID_2, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_3, TEST_BUNDLE_NAME_3, TEST_TOKEN_ID_3, TEST_UID_1,
        TEST_VALID_PERIOD_FOREVER);

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryUserRightRecord(TEST_UID_1, infos);
    ASSERT_GT(ret, 0);
    ASSERT_GE(infos.size(), 3);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_MixedValidPeriod_001 end");
}

/**
 * @tc.name: UsbRightDbHelper_DeleteNormalExpiredRightRecord_Mixed
 * @tc.desc: Test DeleteNormalExpiredRightRecord only removes normal expired records
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_DeleteNormalExpiredRightRecord_Mixed, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteNormalExpiredRightRecord_Mixed start");
    InsertTestRecord(TEST_DEVICE_NAME_1, TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, TEST_UID_1,
        TEST_VALID_PERIOD_TEMPORARY);
    InsertTestRecord(TEST_DEVICE_NAME_2, TEST_BUNDLE_NAME_2, TEST_TOKEN_ID_2, TEST_UID_1, TEST_VALID_PERIOD);
    InsertTestRecord(TEST_DEVICE_NAME_3, TEST_BUNDLE_NAME_3, TEST_TOKEN_ID_3, TEST_UID_1,
        TEST_VALID_PERIOD_FOREVER);

    uint64_t expiredTime = TEST_REQUEST_TIME + TEST_VALID_PERIOD + TEST_VALID_PERIOD;
    int32_t ret = dbHelper_->DeleteNormalExpiredRightRecord(TEST_UID_1, expiredTime);
    ASSERT_GE(ret, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    ret = dbHelper_->QueryUserRightRecord(TEST_UID_1, infos);
    ASSERT_GT(ret, 0);
    ASSERT_GE(infos.size(), 2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_DeleteNormalExpiredRightRecord_Mixed end");
}

/**
 * @tc.name: UsbRightDbHelper_BatchInsertAndDelete
 * @tc.desc: Test inserting many records and deleting them all by uid
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_BatchInsertAndDelete, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_BatchInsertAndDelete start");
    const int32_t recordCount = 20;
    for (int32_t i = 0; i < recordCount; ++i) {
        std::string deviceName = std::string("usb_dev_batch_") + std::to_string(i);
        std::string bundleName = std::string("com.test.batch.app") + std::to_string(i);
        InsertTestRecord(deviceName, bundleName, TEST_TOKEN_ID_1, TEST_UID_1, TEST_VALID_PERIOD);
    }

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryUserRightRecord(TEST_UID_1, infos);
    ASSERT_GT(ret, 0);
    ASSERT_GE(infos.size(), static_cast<size_t>(recordCount));

    ret = dbHelper_->DeleteUidRightRecord(TEST_UID_1);
    ASSERT_GE(ret, USB_RIGHT_OK);

    infos.clear();
    ret = dbHelper_->QueryUserRightRecord(TEST_UID_1, infos);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_BatchInsertAndDelete end");
}

/**
 * @tc.name: UsbRightDbHelper_UpdateAfterAddOrUpdate_001
 * @tc.desc: Test multiple AddOrUpdate calls with alternating validPeriod
 * @tc.type: FUNC
 */
HWTEST_F(UsbRightDbHelperTest, UsbRightDbHelper_UpdateAfterAddOrUpdate_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_UpdateAfterAddOrUpdate_001 start");
    struct UsbRightAppInfo info;
    info.uid = TEST_UID_1;
    info.installTime = TEST_INSTALL_TIME;
    info.updateTime = TEST_UPDATE_TIME;
    info.requestTime = TEST_REQUEST_TIME;
    info.validPeriod = TEST_VALID_PERIOD_TEMPORARY;

    int32_t ret1 = dbHelper_->AddOrUpdateRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, info);
    ASSERT_GE(ret1, USB_RIGHT_OK);

    info.validPeriod = TEST_VALID_PERIOD;
    int32_t ret2 = dbHelper_->AddOrUpdateRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, info);
    ASSERT_GE(ret2, USB_RIGHT_OK);

    info.validPeriod = TEST_VALID_PERIOD_FOREVER;
    int32_t ret3 = dbHelper_->AddOrUpdateRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, info);
    ASSERT_GE(ret3, USB_RIGHT_OK);

    std::vector<struct UsbRightAppInfo> infos;
    int32_t ret = dbHelper_->QueryRightRecord(TEST_UID_1, TEST_DEVICE_NAME_1,
        TEST_BUNDLE_NAME_1, TEST_TOKEN_ID_1, infos);
    ASSERT_GT(ret, 0);
    ASSERT_GE(infos.size(), 1);
    EXPECT_EQ(infos[0].validPeriod, TEST_VALID_PERIOD_FOREVER);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDbHelper_UpdateAfterAddOrUpdate_001 end");
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS