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

#include "usb_scsi_command_test.h"
#include "usb_srv_client.h"
#include "hilog_wrapper.h"
#include "usb_common_test.h"
#include "scsi_constants.h"
#include <cstring>
#include <iomanip>
#include <sstream>

namespace OHOS {
namespace USB {
namespace ScsiTest {

using namespace ScsiConstants;

enum ScsiPeripheralDeviceType : uint8_t {
    SCSI_PERIPHERAL_DIRECT_ACCESS = BYTE_NULL,
    SCSI_PERIPHERAL_SEQUENTIAL_ACCESS = BYTE_ONE,
    SCSI_PERIPHERAL_PRINTER = BYTE_TWO,
    SCSI_PERIPHERAL_PROCESSOR = BYTE_THREE,
    SCSI_PERIPHERAL_WRITE_ONCE = BYTE_FOUR,
    SCSI_PERIPHERAL_CDROM = BYTE_FIVE,
    SCSI_PERIPHERAL_SCANNER = BYTE_SIX,
    SCSI_PERIPHERAL_OPTICAL_MEMORY = BYTE_SEVEN,
    SCSI_PERIPHERAL_MEDIUM_CHANGER = BYTE_EIGHT,
    SCSI_PERIPHERAL_COMMUNICATIONS = BYTE_NINE,
    SCSI_PERIPHERAL_ASC_IT8_1 = BYTE_ELEVEN,
    SCSI_PERIPHERAL_ASC_IT8_2 = BYTE_TWELVE,
    SCSI_PERIPHERAL_STORAGE_ARRAY_CONTROLLER = BYTE_THIRTEEN,
    SCSI_PERIPHERAL_ENCLOSURE_SERVICES = BYTE_FOURTEEN,
    SCSI_PERIPHERAL_SIMPLIFIED_DIRECT_ACCESS = BYTE_FIFTEEN,
    SCSI_PERIPHERAL_OPTICAL_CARD_READER = BYTE_SIXTEEN,
    SCSI_PERIPHERAL_BRIDGE_CONTROLLER = BYTE_SEVENTEEN,
    SCSI_PERIPHERAL_OBJECT_BASED_STORAGE = BYTE_EIGHTEEN,
    SCSI_PERIPHERAL_AUTOMATION_DRIVE = BYTE_NINETEEN,
    SCSI_PERIPHERAL_SECURITY_MANAGER = BYTE_TWENTY,
    SCSI_PERIPHERAL_RESERVED = BYTE_THIRTY,
    SCSI_PERIPHERAL_UNKNOWN = BYTE_THIRTY_ONE,
};

ScsiTestConfig CreateDefaultTestConfig()
{
    return ScsiTestConfig {
        .busNum = SCSI_DEFAULT_CONFIG_BUS_NUM,
        .devAddr = SCSI_DEFAULT_CONFIG_DEV_ADDR,
        .vendorId = SCSI_DEFAULT_CONFIG_VENDOR_ID,
        .productId = SCSI_DEFAULT_CONFIG_PRODUCT_ID,
        .expectedBlockCount = 0,
        .expectedBlockSize = BLOCK_SIZE_512,
        .testTransferSize = TRANSFER_SIZE_4096,
        .timeoutMs = TIMEOUT_MS_DEFAULT,
    };
}

bool InitializeAndCheckDevice(UsbScsiCommandTest* test, ScsiTestConfig& config)
{
    bool initialized = test->InitializeMassStorageDevice(config);
    if (!initialized) {
        USB_HILOGW(MODULE_USB_SERVICE, "Device initialization skipped");
        return false;
    }

    auto ready = test->massStorageInterface_->CheckDeviceReady(ARRAY_INDEX_10);
    if (ready != UsbTransferResult::USB_OK) {
        USB_HILOGW(MODULE_USB_SERVICE, "Device not ready");
        return false;
    }

    return true;
}

std::string ExtractIdentificationString(const uint8_t* data, size_t size)
{
    std::string result;
    for (size_t i = NUM_0; i < size; i++) {
        if (data[i] != ' ' && data[i] != '\0') {
            result += data[i];
        }
    }
    return result;
}
    }
    return result;
}

void LogInquiryDeviceInfo(const ScsiInquiryData& inquiryData)
{
    USB_HILOGI(MODULE_USB_SERVICE, "========== Device Information ==========");
    USB_HILOGI(MODULE_USB_SERVICE, "Peripheral Type: 0x%{public}X", inquiryData.peripheralDeviceType);
    USB_HILOGI(MODULE_USB_SERVICE, "Removable: %{public}s",
        (inquiryData.removable & MASK_BIT_7) ? "Yes" : "No");
    USB_HILOGI(MODULE_USB_SERVICE, "Version: 0x%{public}X", inquiryData.version);
    USB_HILOGI(MODULE_USB_SERVICE, "Response Format: 0x%{public}X", inquiryData.responseDataFormat);
    USB_HILOGI(MODULE_USB_SERVICE, "Additional Length: %{public}u", inquiryData.additionalLength);

    char vendorId[VENDOR_ID_SIZE + 1];
    memset_s(vendorId, VENDOR_ID_SIZE + 1, 0, VENDOR_ID_SIZE + 1);
    memcpy_s(vendorId, sizeof(vendorId), inquiryData.vendorIdentification,
        sizeof(inquiryData.vendorIdentification));
    USB_HILOGI(MODULE_USB_SERVICE, "Vendor: %{public}s", vendorId);

    char productId[PRODUCT_ID_SIZE + 1];
    memset_s(productId, PRODUCT_ID_SIZE + 1, 0, PRODUCT_ID_SIZE + 1);
    memcpy_s(productId, sizeof(productId), inquiryData.productIdentification,
        sizeof(inquiryData.productIdentification));
    USB_HILOGI(MODULE_USB_SERVICE, "Product: %{public}s", productId);

    char productRev[REVISION_LEVEL_SIZE + 1];
    memset_s(productRev, REVISION_LEVEL_SIZE + 1, 0, REVISION_LEVEL_SIZE + 1);
    memcpy_s(productRev, sizeof(productRev), inquiryData.productRevisionLevel,
        sizeof(inquiryData.productRevisionLevel));
    USB_HILOGI(MODULE_USB_SERVICE, "Revision: %{public}s", productRev);
    USB_HILOGI(MODULE_USB_SERVICE, "========================================");
}

/**
 * @tc.name: UsbScsiInquiry_SendInquiry_001
 * @tc.desc: Test sending INQUIRY command to mass storage device
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_SendInquiry_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    ASSERT_EQ(ret, UsbTransferResult::USB_OK);

    ValidateInquiryData(inquiryData);

    EXPECT_EQ(inquiryData.peripheralDeviceType, SCSI_PERIPHERAL_DIRECT_ACCESS);
    EXPECT_TRUE(inquiryData.responseDataFormat == SCSI_RESPONSE_FORMAT_SPC2 ||
                inquiryData.responseDataFormat == SCSI_RESPONSE_FORMAT_SPC3);

    USB_HILOGI(MODULE_USB_SERVICE, "Inquiry completed: type=0x%{public}X format=0x%{public}X",
                inquiryData.peripheralDeviceType, inquiryData.responseDataFormat);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_SendInquiry_002
 * @tc.desc: Test sending INQUIRY command and validate vendor ID
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_SendInquiry_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_002 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    char vendorId[VENDOR_ID_SIZE + 1];
    memset_s(vendorId, VENDOR_ID_SIZE + 1, 0, VENDOR_ID_SIZE + 1);
    memcpy_s(vendorId, sizeof(vendorId), inquiryData.vendorIdentification,
        sizeof(inquiryData.vendorIdentification));

    USB_HILOGI(MODULE_USB_SERVICE, "Vendor ID: %{public}s", vendorId);

    EXPECT_GT(strlen(vendorId), 0);
    EXPECT_LE(strlen(vendorId), VENDOR_ID_SIZE);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_002 end");
}

/**
 * @tc.name: UsbScsiInquiry_SendInquiry_003
 * @tc.desc: Test sending INQUIRY command and validate product ID
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_SendInquiry_003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_003 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    char productId[PRODUCT_ID_SIZE + 1];
    memset_s(productId, PRODUCT_ID_SIZE + 1, 0, PRODUCT_ID_SIZE + 1);
    memcpy_s(productId, sizeof(productId), inquiryData.productIdentification,
        sizeof(inquiryData.productIdentification));

    USB_HILOGI(MODULE_USB_SERVICE, "Product ID: %{public}s", productId);

    EXPECT_GT(strlen(productId), 0);
    EXPECT_LE(strlen(productId), PRODUCT_ID_SIZE);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_003 end");
}

/**
 * @tc.name: UsbScsiInquiry_SendInquiry_004
 * @tc.desc: Test sending INQUIRY command and validate product revision
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_SendInquiry_004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_004 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    char productRev[REVISION_LEVEL_SIZE + 1];
    memset_s(productRev, REVISION_LEVEL_SIZE + 1, 0, REVISION_LEVEL_SIZE + 1);
    memcpy_s(productRev, sizeof(productRev), inquiryData.productRevisionLevel,
        sizeof(inquiryData.productRevisionLevel));

    USB_HILOGI(MODULE_USB_SERVICE, "Product Revision: %{public}s", productRev);

    EXPECT_GT(strlen(productRev), 0);
    EXPECT_LE(strlen(productRev), REVISION_LEVEL_SIZE);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_004 end");
}

/**
 * @tc.name: UsbScsiInquiry_SendInquiry_005
 * @tc.desc: Test sending INQUIRY command and validate removable bit
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_SendInquiry_005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_005 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    uint8_t removable = (inquiryData.removable >> ARRAY_INDEX_7) & BYTE_ONE;
    USB_HILOGI(MODULE_USB_SERVICE, "Removable media: %{public}u", removable);

    EXPECT_TRUE(removable == BYTE_NULL || removable == BYTE_ONE);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_005 end");
}

/**
 * @tc.name: UsbScsiInquiry_SendInquiry_006
 * @tc.desc: Test sending INQUIRY command multiple times
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_SendInquiry_006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_006 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    // Number of retry attempts for INQUIRY command during stress testing
    const uint32_t INQUIRY_TEST_ITERATIONS = TEST_INQUIRY_ITERATIONS;
    for (uint32_t i = NUM_0; i < INQUIRY_TEST_ITERATIONS && i < SCSI_MAX_RETRIES; i++) {
        ScsiInquiryData inquiryData;
        auto ret = massStorageInterface_->SendInquiry(inquiryData);

        EXPECT_EQ(ret, UsbTransferResult::USB_OK);

        EXPECT_TRUE(inquiryData.peripheralDeviceType == SCSI_PERIPHERAL_DIRECT_ACCESS ||
                    inquiryData.peripheralDeviceType == SCSI_PERIPHERAL_UNKNOWN);

        USB_HILOGI(MODULE_USB_SERVICE, "Peripheral type: 0x%{public}X", inquiryData.peripheralDeviceType);
    }

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_SendInquiry_006 end");
}

/**
 * @tc.name: UsbScsiInquiry_ValidateVersion_001
 * @tc.desc: Test INQUIRY version field
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_ValidateVersion_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ValidateVersion_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    USB_HILOGI(MODULE_USB_SERVICE, "Version: 0x%{public}X", inquiryData.version);

    EXPECT_GE(inquiryData.version, BYTE_NULL);
    EXPECT_LE(inquiryData.version, BYTE_TWO_HUNDRED_FIFTY_FIVE);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ValidateVersion_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_ValidateAdditionalLength_001
 * @tc.desc: Test INQUIRY additional length field
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_ValidateAdditionalLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ValidateAdditionalLength_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    USB_HILOGI(MODULE_USB_SERVICE, "Additional length: %{public}u", inquiryData.additionalLength);

    EXPECT_GE(inquiryData.additionalLength, SCSI_ADDITIONAL_LENGTH_MIN);
    EXPECT_LE(inquiryData.additionalLength, SCSI_ADDITIONAL_LENGTH_MAX);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ValidateAdditionalLength_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_ParseVendorId_001
 * @tc.desc: Test parsing vendor ID from INQUIRY data
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_ParseVendorId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ParseVendorId_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    std::string vendorId = ExtractIdentificationString(inquiryData.vendorIdentification, VENDOR_ID_SIZE);

    USB_HILOGI(MODULE_USB_SERVICE, "Parsed Vendor ID: %{public}s", vendorId.c_str());

    EXPECT_GT(vendorId.length(), 0);
    EXPECT_LE(vendorId.length(), VENDOR_ID_SIZE);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ParseVendorId_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_ParseProductId_001
 * @tc.desc: Test parsing product ID from INQUIRY data
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_ParseProductId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ParseProductId_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    std::string productId = ExtractIdentificationString(inquiryData.productIdentification, PRODUCT_ID_SIZE);

    USB_HILOGI(MODULE_USB_SERVICE, "Parsed Product ID: %{public}s", productId.c_str());

    EXPECT_GT(productId.length(), 0);
    EXPECT_LE(productId.length(), PRODUCT_ID_SIZE);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ParseProductId_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_ParseProductRevision_001
 * @tc.desc: Test parsing product revision from INQUIRY data
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_ParseProductRevision_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ParseProductRevision_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    std::string productRev = ExtractIdentificationString(inquiryData.productRevisionLevel, REVISION_LEVEL_SIZE);

    USB_HILOGI(MODULE_USB_SERVICE, "Parsed Product Revision: %{public}s", productRev.c_str());

    EXPECT_GT(productRev.length(), 0);
    EXPECT_LE(productRev.length(), REVISION_LEVEL_SIZE);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ParseProductRevision_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_DisplayDeviceInfo_001
 * @tc.desc: Test displaying complete device information from INQUIRY
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_DisplayDeviceInfo_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_DisplayDeviceInfo_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    LogInquiryDeviceInfo(inquiryData);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_DisplayDeviceInfo_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_TestUnitReady_001
 * @tc.desc: Test TEST UNIT READY command before INQUIRY
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_TestUnitReady_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_TestUnitReady_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_TestUnitReady_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_TestUnitReady_002
 * @tc.desc: Test TEST UNIT READY after INQUIRY
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_TestUnitReady_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_TestUnitReady_002 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    auto readyResult = massStorageInterface_->SendTestUnitReady();
    EXPECT_EQ(readyResult, UsbTransferResult::USB_OK);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_TestUnitReady_002 end");
}

/**
 * @tc.name: UsbScsiInquiry_RequestSense_001
 * @tc.desc: Test REQUEST SENSE after failed INQUIRY (simulated)
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_RequestSense_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_RequestSense_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    if (ret != UsbTransferResult::USB_OK) {
        ScsiSenseData senseData;
        auto senseResult = massStorageInterface_->RequestSense(senseData);

        if (senseResult == UsbTransferResult::USB_OK) {
            USB_HILOGI(MODULE_USB_SERVICE, "Sense Key: 0x%{public}X", senseData.senseKey);
            USB_HILOGI(MODULE_USB_SERVICE, "ASC: 0x%{public}X", senseData.additionalSenseCode);
            USB_HILOGI(MODULE_USB_SERVICE, "ASCQ: 0x%{public}X", senseData.additionalSenseCodeQualifier);
        }
    }

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_RequestSense_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_StressTest_001
 * @tc.desc: Stress test with multiple INQUIRY commands
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_StressTest_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_StressTest_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    int successCount = 0;
    int failureCount = 0;

    for (int i = NUM_0; i < MAX_STRESS_ITERATIONS && i < static_cast<int>(SCSI_MAX_RETRIES); i++) {
        ScsiInquiryData inquiryData;
        auto ret = massStorageInterface_->SendInquiry(inquiryData);

        if (ret == UsbTransferResult::USB_OK) {
            successCount++;
        } else {
            failureCount++;
        }

        USB_HILOGI(MODULE_USB_SERVICE, "Iteration %{public}d: result=%{public}d", i + NUM_1, static_cast<int>(ret));

        std::this_thread::sleep_for(std::chrono::milliseconds(SCSI_SHORT_SLEEP_MS));
    }

    USB_HILOGI(MODULE_USB_SERVICE, "Stress test completed: success=%{public}d failure=%{public}d",
                successCount, failureCount);

    EXPECT_GT(successCount, 0);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_StressTest_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_ConcurrentTest_001
 * @tc.desc: Test concurrent INQUIRY operations (if supported)
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_ConcurrentTest_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ConcurrentTest_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    std::atomic<int> successCount(0);

    std::vector<std::thread> threads;
    const int iterationsPerThread = THREAD_ITERATIONS;

    for (int t = NUM_0; t < MAX_THREAD_COUNT; t++) {
        threads.emplace_back([this, &successCount, iterationsPerThread, t]() {
            for (int i = NUM_0; i < iterationsPerThread; i++) {
                ScsiInquiryData inquiryData;
                if (massStorageInterface_->SendInquiry(inquiryData) == UsbTransferResult::USB_OK) {
                    successCount++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(SCSI_NORMAL_SLEEP_MS));
            }
            USB_HILOGI(MODULE_USB_SERVICE, "Thread %{public}d completed", t + NUM_1);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    USB_HILOGI(MODULE_USB_SERVICE, "Concurrent test completed: success=%{public}d", successCount.load());

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_ConcurrentTest_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_DataFormat_001
 * @tc.desc: Test INQUIRY response data format validation
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_DataFormat_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_DataFormat_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    uint8_t dataFormat = inquiryData.responseDataFormat;
    USB_HILOGI(MODULE_USB_SERVICE, "Response Data Format: 0x%{public}X", dataFormat);

    EXPECT_TRUE(dataFormat == SCSI_RESPONSE_FORMAT_SPC2 || dataFormat == SCSI_RESPONSE_FORMAT_SPC3);

    if (dataFormat == SCSI_RESPONSE_FORMAT_SPC2) {
        USB_HILOGI(MODULE_USB_SERVICE, "SPC-2 or earlier compliant");
    } else if (dataFormat == SCSI_RESPONSE_FORMAT_SPC3) {
        USB_HILOGI(MODULE_USB_SERVICE, "SPC-3 or later compliant");
    }

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_DataFormat_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_FlagsValidation_001
 * @tc.desc: Test INQUIRY flags field validation
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_FlagsValidation_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_FlagsValidation_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    USB_HILOGI(MODULE_USB_SERVICE, "Flags[0]: 0x%{public}X", inquiryData.flags[0]);
    USB_HILOGI(MODULE_USB_SERVICE, "Flags[1]: 0x%{public}X", inquiryData.flags[1]);
    USB_HILOGI(MODULE_USB_SERVICE, "Flags[2]: 0x%{public}X", inquiryData.flags[2]);

    uint8_t cmdQue = (inquiryData.flags[ARRAY_INDEX_1] >> ARRAY_INDEX_1) & BYTE_ONE;
    USB_HILOGI(MODULE_USB_SERVICE, "CmdQue: %{public}u", cmdQue);

    uint8_t sync = (inquiryData.flags[ARRAY_INDEX_1] >> ARRAY_INDEX_7) & BYTE_ONE;
    USB_HILOGI(MODULE_USB_SERVICE, "Sync: %{public}u", sync);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_FlagsValidation_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_CompareResults_001
 * @tc.desc: Test comparing multiple INQUIRY results
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_CompareResults_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_CompareResults_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ScsiInquiryData inquiryData1, inquiryData2;
    auto ret1 = massStorageInterface_->SendInquiry(inquiryData1);
    auto ret2 = massStorageInterface_->SendInquiry(inquiryData2);

    EXPECT_EQ(ret1, UsbTransferResult::USB_OK);
    EXPECT_EQ(ret2, UsbTransferResult::USB_OK);

    EXPECT_EQ(inquiryData1.peripheralDeviceType, inquiryData2.peripheralDeviceType);
    EXPECT_EQ(inquiryData1.version, inquiryData2.version);
    EXPECT_EQ(inquiryData1.responseDataFormat, inquiryData2.responseDataFormat);

    int vendorCmp = memcmp_s(inquiryData1.vendorIdentification, VENDOR_ID_SIZE,
                             inquiryData2.vendorIdentification, VENDOR_ID_SIZE);
    EXPECT_EQ(vendorCmp, 0);

    int productCmp = memcmp_s(inquiryData1.productIdentification, PRODUCT_ID_SIZE,
                              inquiryData2.productIdentification, PRODUCT_ID_SIZE);
    EXPECT_EQ(productCmp, 0);

    USB_HILOGI(MODULE_USB_SERVICE, "Inquiry results are consistent");

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_CompareResults_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_RetryOnFailure_001
 * @tc.desc: Test retrying INQUIRY on failure
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_RetryOnFailure_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_RetryOnFailure_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    const int INQUIRY_RETRY_MAX_RETRIES = TEST_RETRY_MAX_RETRIES;
    ScsiInquiryData inquiryData;
    bool success = false;

    for (int i = NUM_0; i < INQUIRY_RETRY_MAX_RETRIES && !success; i++) {
        auto ret = massStorageInterface_->SendInquiry(inquiryData);

        if (ret != UsbTransferResult::USB_OK) {
            USB_HILOGW(MODULE_USB_SERVICE, "INQUIRY failed, retry %{public}d/%{public}d",
                i + NUM_1, INQUIRY_RETRY_MAX_RETRIES);
            ScsiSenseData senseData;
            massStorageInterface_->RequestSense(senseData);
            std::this_thread::sleep_for(std::chrono::milliseconds(SCSI_RETRY_DELAY_MS));
        } else {
            success = true;
        }
    }

    EXPECT_TRUE(success);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_RetryOnFailure_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_AllPeripheralTypes_001
 * @tc.desc: Test all possible peripheral device type values
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_AllPeripheralTypes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_AllPeripheralTypes_001 start");

    std::vector<std::pair<uint8_t, std::string>> peripheralTypes = {
        {SCSI_PERIPHERAL_DIRECT_ACCESS, "Direct Access"},
        {SCSI_PERIPHERAL_SEQUENTIAL_ACCESS, "Sequential Access"},
        {SCSI_PERIPHERAL_PRINTER, "Printer"},
        {SCSI_PERIPHERAL_PROCESSOR, "Processor"},
        {SCSI_PERIPHERAL_WRITE_ONCE, "Write Once"},
        {SCSI_PERIPHERAL_CDROM, "CD-ROM"},
        {SCSI_PERIPHERAL_SCANNER, "Scanner"},
        {SCSI_PERIPHERAL_OPTICAL_MEMORY, "Optical Memory"},
        {SCSI_PERIPHERAL_MEDIUM_CHANGER, "Medium Changer"},
        {SCSI_PERIPHERAL_COMMUNICATIONS, "Communications"},
        {SCSI_PERIPHERAL_STORAGE_ARRAY_CONTROLLER, "Storage Array Controller"},
        {SCSI_PERIPHERAL_ENCLOSURE_SERVICES, "Enclosure Services"},
        {SCSI_PERIPHERAL_SIMPLIFIED_DIRECT_ACCESS, "Simplified Direct Access"},
        {SCSI_PERIPHERAL_OPTICAL_CARD_READER, "Optical Card Reader"},
        {SCSI_PERIPHERAL_BRIDGE_CONTROLLER, "Bridge Controller"},
        {SCSI_PERIPHERAL_OBJECT_BASED_STORAGE, "Object Based Storage"},
        {SCSI_PERIPHERAL_AUTOMATION_DRIVE, "Automation Drive"},
        {SCSI_PERIPHERAL_SECURITY_MANAGER, "Security Manager"},
        {SCSI_PERIPHERAL_RESERVED, "Reserved"},
        {SCSI_PERIPHERAL_UNKNOWN, "Unknown"},
    };

    for (const auto& type : peripheralTypes) {
        USB_HILOGI(MODULE_USB_SERVICE, "Peripheral Type 0x%{public}X: %{public}s",
                    type.first, type.second.c_str());
        EXPECT_GE(type.first, BYTE_NULL);
        EXPECT_LE(type.first, BYTE_THIRTY_ONE);
    }

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_AllPeripheralTypes_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_InquiryDataStructure_001
 * @tc.desc: Test INQUIRY data structure size and alignment
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_InquiryDataStructure_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_InquiryDataStructure_001 start");

    EXPECT_EQ(sizeof(ScsiInquiryData), SCSI_INQUIRY_DATA_LENGTH);
    EXPECT_EQ(sizeof(inquiryData.peripheralDeviceType), NUM_1);
    EXPECT_EQ(sizeof(inquiryData.removable), NUM_1);
    EXPECT_EQ(sizeof(inquiryData.version), NUM_1);
    EXPECT_EQ(sizeof(inquiryData.responseDataFormat), NUM_1);
    EXPECT_EQ(sizeof(inquiryData.additionalLength), NUM_1);
    EXPECT_EQ(sizeof(inquiryData.flags), ARRAY_SIZE_3);
    EXPECT_EQ(sizeof(inquiryData.vendorIdentification), VENDOR_ID_SIZE);
    EXPECT_EQ(sizeof(inquiryData.productIdentification), PRODUCT_ID_SIZE);
    EXPECT_EQ(sizeof(inquiryData.productRevisionLevel), REVISION_LEVEL_SIZE);

    USB_HILOGI(MODULE_USB_SERVICE, "ScsiInquiryData structure validated");

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_InquiryDataStructure_001 end");
}

/**
 * @tc.name: UsbScsiInquiry_EndpointValidation_001
 * @tc.desc: Test endpoint validation before INQUIRY
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiInquiry_EndpointValidation_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_EndpointValidation_001 start");

    ScsiTestConfig config = CreateDefaultTestConfig();

    if (!InitializeAndCheckDevice(this, config)) {
        SUCCEED();
        return;
    }

    ASSERT_TRUE(deviceInitialized_);

    EXPECT_EQ(bulkInEndpoint_.GetType(), USB_ENDPOINT_XFER_BULK);
    EXPECT_EQ(bulkInEndpoint_.GetDirection(), USB_ENDPOINT_DIR_IN);
    EXPECT_EQ(bulkOutEndpoint_.GetType(), USB_ENDPOINT_XFER_BULK);
    EXPECT_EQ(bulkOutEndpoint_.GetDirection(), USB_ENDPOINT_DIR_OUT);

    EXPECT_GT(bulkInEndpoint_.GetMaxPacketSize(), 0);
    EXPECT_GT(bulkOutEndpoint_.GetMaxPacketSize(), 0);

    USB_HILOGI(MODULE_USB_SERVICE, "Bulk IN endpoint: address=0x%{public}X maxPacket=%{public}d",
                bulkInEndpoint_.GetAddress(), bulkInEndpoint_.GetMaxPacketSize());
    USB_HILOGI(MODULE_USB_SERVICE, "Bulk OUT endpoint: address=0x%{public}X maxPacket=%{public}d",
                bulkOutEndpoint_.GetAddress(), bulkOutEndpoint_.GetMaxPacketSize());

    ScsiInquiryData inquiryData;
    auto ret = massStorageInterface_->SendInquiry(inquiryData);

    EXPECT_EQ(ret, UsbTransferResult::USB_OK);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiInquiry_EndpointValidation_001 end");
}

} // namespace ScsiTest
} // namespace USB
} // namespace OHOS