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
#include <vector>
#include <thread>
#include <atomic>

#include "usb_right_database.h"
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
constexpr int64_t TEST_INSTALL_TIME = 1234567890;
constexpr int64_t TEST_UPDATE_TIME = 1234567891;
constexpr int64_t TEST_REQUEST_TIME = 1234567892;
constexpr uint64_t TEST_VALID_PERIOD = 86400;
constexpr const char* TEST_DEVICE_NAME_1 = "usb_device_001";
constexpr const char* TEST_DEVICE_NAME_2 = "usb_device_002";
constexpr const char* TEST_BUNDLE_NAME_1 = "com.test.app1";
constexpr const char* TEST_BUNDLE_NAME_2 = "com.test.app2";
constexpr const char* TEST_TOKEN_ID_1 = "token_id_001";
constexpr const char* TEST_TOKEN_ID_2 = "token_id_002";

class UsbRightDatabaseTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<UsbRightDataBase> usbRightDatabase_;
};

void UsbRightDatabaseTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabaseTest::SetUpTestCase enter");
}

void UsbRightDatabaseTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabaseTest::TearDownTestCase enter");
}

void UsbRightDatabaseTest::SetUp()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabaseTest::SetUp enter");
    usbRightDatabase_ = UsbRightDataBase::GetInstance();
    ASSERT_NE(usbRightDatabase_, nullptr);
}

void UsbRightDatabaseTest::TearDown()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabaseTest::TearDown enter");
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_GetInstance_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_GetInstance_001 enter");
    auto db1 = UsbRightDataBase::GetInstance();
    auto db2 = UsbRightDataBase::GetInstance();
    ASSERT_EQ(db1, db2);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_Insert_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_Insert_001 enter");
    ValuesBucket insertValues;
    insertValues.PutInt("uid", TEST_UID_1);
    insertValues.PutLong("installTime", TEST_INSTALL_TIME);
    insertValues.PutLong("updateTime", TEST_UPDATE_TIME);
    insertValues.PutLong("requestTime", TEST_REQUEST_TIME);
    insertValues.PutLong("validPeriod", TEST_VALID_PERIOD);
    insertValues.PutString("deviceName", TEST_DEVICE_NAME_1);
    insertValues.PutString("bundleName", TEST_BUNDLE_NAME_1);
    insertValues.PutString("tokenId", TEST_TOKEN_ID_1);

    int64_t ret = usbRightDatabase_->Insert(insertValues);
    ASSERT_GT(ret, 0);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_Insert_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_Insert_002 enter");
    ValuesBucket insertValues;
    insertValues.PutInt("uid", TEST_UID_2);
    insertValues.PutLong("installTime", TEST_INSTALL_TIME);
    insertValues.PutLong("updateTime", TEST_UPDATE_TIME);
    insertValues.PutLong("requestTime", TEST_REQUEST_TIME);
    insertValues.PutLong("validPeriod", TEST_VALID_PERIOD);
    insertValues.PutString("deviceName", TEST_DEVICE_NAME_2);
    insertValues.PutString("bundleName", TEST_BUNDLE_NAME_2);
    insertValues.PutString("tokenId", TEST_TOKEN_ID_2);

    int64_t ret = usbRightDatabase_->Insert(insertValues);
    ASSERT_GT(ret, 0);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_Update_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_Update_001 enter");
    ValuesBucket values;
    values.PutLong("updateTime", TEST_UPDATE_TIME + 1);

    RdbPredicates predicates(USB_RIGHT_TABLE_NAME);
    predicates.EqualTo("uid", std::to_string(TEST_UID_1));

    int32_t changedRows = 0;
    int32_t ret = usbRightDatabase_->Update(changedRows, values, predicates);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_Update_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_Update_002 enter");
    ValuesBucket values;
    values.PutString("deviceName", TEST_DEVICE_NAME_2);

    std::string whereClause = "uid = ?";
    std::vector<std::string> whereArgs = {std::to_string(TEST_UID_1)};

    int32_t changedRows = 0;
    int32_t ret = usbRightDatabase_->Update(changedRows, values, whereClause, whereArgs);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_Delete_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_Delete_001 enter");
    RdbPredicates predicates(USB_RIGHT_TABLE_NAME);
    predicates.EqualTo("uid", std::to_string(TEST_UID_1));

    int32_t ret = usbRightDatabase_->Delete(predicates);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_Delete_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_Delete_002 enter");
    std::string whereClause = "uid = ?";
    std::vector<std::string> whereArgs = {std::to_string(TEST_UID_2)};

    int32_t changedRows = 0;
    int32_t ret = usbRightDatabase_->Delete(changedRows, whereClause, whereArgs);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_QuerySql_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_QuerySql_001 enter");
    std::string sql = "SELECT * FROM " + std::string(USB_RIGHT_TABLE_NAME) + " WHERE uid = ?";
    std::vector<std::string> selectionArgs = {std::to_string(TEST_UID_1)};

    auto resultSet = usbRightDatabase_->QuerySql(sql, selectionArgs);
    ASSERT_NE(resultSet, nullptr);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_Query_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_Query_001 enter");
    RdbPredicates predicates(USB_RIGHT_TABLE_NAME);
    predicates.EqualTo("uid", std::to_string(TEST_UID_1));
    std::vector<std::string> columns = {"uid", "deviceName", "bundleName"};

    auto resultSet = usbRightDatabase_->Query(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_ExecuteSql_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_ExecuteSql_001 enter");
    std::string sql = "UPDATE " + std::string(USB_RIGHT_TABLE_NAME) + " SET updateTime = ? WHERE uid = ?";
    std::vector<ValueObject> bindArgs;
    bindArgs.push_back(ValueObject(static_cast<int64_t>(TEST_UPDATE_TIME + 2)));
    bindArgs.push_back(ValueObject(static_cast<int32_t>(TEST_UID_1)));

    int32_t ret = usbRightDatabase_->ExecuteSql(sql, bindArgs);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_BeginTransaction_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_BeginTransaction_001 enter");
    int32_t ret = usbRightDatabase_->BeginTransaction();
    ASSERT_GE(ret, UEC_OK);
    usbRightDatabase_->RollBack();
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_Commit_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_Commit_001 enter");
    usbRightDatabase_->BeginTransaction();

    ValuesBucket insertValues;
    insertValues.PutInt("uid", TEST_UID_3);
    insertValues.PutLong("installTime", TEST_INSTALL_TIME);
    insertValues.PutLong("updateTime", TEST_UPDATE_TIME);
    insertValues.PutLong("requestTime", TEST_REQUEST_TIME);
    insertValues.PutLong("validPeriod", TEST_VALID_PERIOD);
    insertValues.PutString("deviceName", TEST_DEVICE_NAME_1);
    insertValues.PutString("bundleName", TEST_BUNDLE_NAME_1);
    insertValues.PutString("tokenId", TEST_TOKEN_ID_1);

    usbRightDatabase_->Insert(insertValues);
    int32_t ret = usbRightDatabase_->Commit();
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_RollBack_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_RollBack_001 enter");
    usbRightDatabase_->BeginTransaction();

    ValuesBucket insertValues;
    insertValues.PutInt("uid", TEST_UID_3 + 1);
    insertValues.PutLong("installTime", TEST_INSTALL_TIME);
    insertValues.PutLong("updateTime", TEST_UPDATE_TIME);
    insertValues.PutLong("requestTime", TEST_REQUEST_TIME);
    insertValues.PutLong("validPeriod", TEST_VALID_PERIOD);
    insertValues.PutString("deviceName", TEST_DEVICE_NAME_2);
    insertValues.PutString("bundleName", TEST_BUNDLE_NAME_2);
    insertValues.PutString("tokenId", TEST_TOKEN_ID_2);

    usbRightDatabase_->Insert(insertValues);
    int32_t ret = usbRightDatabase_->RollBack();
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_ThreadSafety_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_ThreadSafety_001 enter");
    const int32_t numThreads = 10;
    std::vector<std::thread> threads;
    std::atomic<int32_t> successCount(0);

    for (int32_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i, &successCount]() {
            ValuesBucket insertValues;
            insertValues.PutInt("uid", TEST_UID_1 + i);
            insertValues.PutLong("installTime", TEST_INSTALL_TIME);
            insertValues.PutLong("updateTime", TEST_UPDATE_TIME);
            insertValues.PutLong("requestTime", TEST_REQUEST_TIME);
            insertValues.PutLong("validPeriod", TEST_VALID_PERIOD);
            insertValues.PutString("deviceName", TEST_DEVICE_NAME_1);
            insertValues.PutString("bundleName", TEST_BUNDLE_NAME_1);
            insertValues.PutString("tokenId", TEST_TOKEN_ID_1);

            int64_t ret = usbRightDatabase_->Insert(insertValues);
            if (ret > 0) {
                successCount++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    ASSERT_GE(successCount, numThreads - 1);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_Transaction_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_Transaction_001 enter");
    usbRightDatabase_->BeginTransaction();

    for (int32_t i = 0; i < 5; ++i) {
        ValuesBucket insertValues;
        insertValues.PutInt("uid", TEST_UID_1 + i + 100);
        insertValues.PutLong("installTime", TEST_INSTALL_TIME);
        insertValues.PutLong("updateTime", TEST_UPDATE_TIME);
        insertValues.PutLong("requestTime", TEST_REQUEST_TIME);
        insertValues.PutLong("validPeriod", TEST_VALID_PERIOD);
        insertValues.PutString("deviceName", TEST_DEVICE_NAME_1);
        insertValues.PutString("bundleName", TEST_BUNDLE_NAME_1);
        insertValues.PutString("tokenId", TEST_TOKEN_ID_1);

        usbRightDatabase_->Insert(insertValues);
    }

    int32_t ret = usbRightDatabase_->Commit();
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_Transaction_RollBack_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_Transaction_RollBack_001 enter");
    usbRightDatabase_->BeginTransaction();

    for (int32_t i = 0; i < 5; ++i) {
        ValuesBucket insertValues;
        insertValues.PutInt("uid", TEST_UID_1 + i + 200);
        insertValues.PutLong("installTime", TEST_INSTALL_TIME);
        insertValues.PutLong("updateTime", TEST_UPDATE_TIME);
        insertValues.PutLong("requestTime", TEST_REQUEST_TIME);
        insertValues.PutLong("validPeriod", TEST_VALID_PERIOD);
        insertValues.PutString("deviceName", TEST_DEVICE_NAME_2);
        insertValues.PutString("bundleName", TEST_BUNDLE_NAME_2);
        insertValues.PutString("tokenId", TEST_TOKEN_ID_2);

        usbRightDatabase_->Insert(insertValues);
    }

    int32_t ret = usbRightDatabase_->RollBack();
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_EmptyValues_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_EmptyValues_001 enter");
    ValuesBucket emptyValues;
    int64_t ret = usbRightDatabase_->Insert(emptyValues);
    ASSERT_GE(ret, 0);
}

HWTEST_F(UsbRightDatabaseTest, UsbRightDatabase_Query_EmptyResult_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightDatabase_Query_EmptyResult_001 enter");
    RdbPredicates predicates(USB_RIGHT_TABLE_NAME);
    predicates.EqualTo("uid", "999999");
    std::vector<std::string> columns = {"uid", "deviceName", "bundleName"};

    auto resultSet = usbRightDatabase_->Query(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS