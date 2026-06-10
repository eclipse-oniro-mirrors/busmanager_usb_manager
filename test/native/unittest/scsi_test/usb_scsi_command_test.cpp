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
#include "hilog_wrapper.h"
#include "usb_common_test.h"
#include <cstring>
#include <algorithm>

namespace OHOS {
namespace USB {
namespace ScsiTest {

constexpr uint32_t CBW_SIGNATURE = 0x43425355;
constexpr uint32_t CSW_SIGNATURE = 0x53425355;

void UsbScsiCommandTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommandTest::SetUpTestCase enter");
    UsbCommonTest::GrantPermissionSysNative();
}

void UsbScsiCommandTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommandTest::TearDownTestCase enter");
}

void UsbScsiCommandTest::SetUp()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommandTest::SetUp enter");
    deviceInitialized_ = false;

    testConfig_.busNum = 1;
    testConfig_.devAddr = 2;
    testConfig_.vendorId = 0x1234;
    testConfig_.productId = 0x5678;
    testConfig_.expectedBlockCount = 0;
    testConfig_.expectedBlockSize = 512;
    testConfig_.testTransferSize = 4096;
    testConfig_.timeoutMs = 5000;
}

void UsbScsiCommandTest::TearDown()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommandTest::TearDown enter");
    if (deviceInitialized_) {
        CleanupMassStorageDevice();
    }
}

bool UsbScsiCommandTest::InitializeMassStorageDevice(const ScsiTestConfig& config)
{
    USB_HILOGI(MODULE_USB_SERVICE, "InitializeMassStorageDevice busNum=%{public}u devAddr=%{public}u",
                config.busNum, config.devAddr);

    massStorageInterface_ = UsbMassStorageFactory::CreateInstance();
    if (!massStorageInterface_) {
        USB_HILOGE(MODULE_USB_SERVICE, "Failed to create mass storage interface");
        return false;
    }

    auto ret = massStorageInterface_->Initialize(config.busNum, config.devAddr);
    if (ret != UsbTransferResult::USB_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "Initialize failed ret=%{public}d", static_cast<int32_t>(ret));
        return false;
    }

    testConfig_ = config;
    deviceInitialized_ = true;
    USB_HILOGI(MODULE_USB_SERVICE, "Mass storage device initialized successfully");
    return true;
}

bool UsbScsiCommandTest::CleanupMassStorageDevice()
{
    USB_HILOGI(MODULE_USB_SERVICE, "CleanupMassStorageDevice enter");

    if (!deviceInitialized_ || !massStorageInterface_) {
        return false;
    }

    auto ret = massStorageInterface_->Cleanup();
    if (ret != UsbTransferResult::USB_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "Cleanup failed ret=%{public}d", static_cast<int32_t>(ret));
    }

    massStorageInterface_.reset();
    deviceInitialized_ = false;
    USB_HILOGI(MODULE_USB_SERVICE, "CleanupMassStorageDevice completed");
    return true;
}

void UsbScsiCommandTest::BuildScsiCommandBlock(ScsiCommandBlock& cb, uint8_t opcode,
                                                uint32_t lba, uint32_t transferLength)
{
    memset(&cb, 0, sizeof(ScsiCommandBlock));
    cb.opcode = opcode;

    switch (opcode) {
        case SCSI_OPCODE_TEST_UNIT_READY:
            cb.blockLengthMsb = 0;
            cb.blockLengthLsb = 0;
            break;

        case SCSI_OPCODE_INQUIRY:
            cb.blockLengthMsb = 0;
            cb.blockLengthLsb = 36;
            cb.dataLength[0] = 0;
            cb.dataLength[1] = 0;
            break;

        case SCSI_OPCODE_READ_CAPACITY_10:
            cb.blockLengthMsb = (lba >> 24) & 0xFF;
            cb.dataLength[0] = (lba >> 16) & 0xFF;
            cb.dataLength[1] = (lba >> 8) & 0xFF;
            cb.dataLength[2] = lba & 0xFF;
            cb.dataLength[3] = 0;
            cb.control = 0;
            break;

        case SCSI_OPCODE_READ_10:
            cb.blockLengthMsb = (lba >> 24) & 0xFF;
            cb.dataLength[0] = (lba >> 16) & 0xFF;
            cb.dataLength[1] = (lba >> 8) & 0xFF;
            cb.dataLength[2] = lba & 0xFF;
            cb.dataLength[3] = 0;
            cb.blockLengthLsb = (transferLength >> 8) & 0xFF;
            cb.reserved[0] = transferLength & 0xFF;
            break;

        case SCSI_OPCODE_WRITE_10:
            cb.blockLengthMsb = (lba >> 24) & 0xFF;
            cb.dataLength[0] = (lba >> 16) & 0xFF;
            cb.dataLength[1] = (lba >> 8) & 0xFF;
            cb.dataLength[2] = lba & 0xFF;
            cb.dataLength[3] = 0;
            cb.blockLengthLsb = (transferLength >> 8) & 0xFF;
            cb.reserved[0] = transferLength & 0xFF;
            break;

        case SCSI_OPCODE_REQUEST_SENSE:
            cb.blockLengthMsb = 0;
            cb.blockLengthLsb = 18;
            break;

        case SCSI_OPCODE_MODE_SENSE_6:
            cb.blockLengthMsb = 0;
            cb.blockLengthLsb = transferLength;
            break;

        case SCSI_OPCODE_SYNCHRONIZE_CACHE_10:
            cb.blockLengthMsb = (lba >> 24) & 0xFF;
            cb.dataLength[0] = (lba >> 16) & 0xFF;
            cb.dataLength[1] = (lba >> 8) & 0xFF;
            cb.dataLength[2] = lba & 0xFF;
            cb.dataLength[3] = 0;
            cb.blockLengthLsb = (transferLength >> 8) & 0xFF;
            cb.reserved[0] = transferLength & 0xFF;
            break;

        default:
            break;
    }
}

void UsbScsiCommandTest::ValidateInquiryData(const ScsiInquiryData& data)
{
    USB_HILOGI(MODULE_USB_SERVICE, "ValidateInquiryData: peripheralType=0x%{public}X removable=%{public}u",
                data.peripheralDeviceType, data.removable);

    ASSERT_TRUE(data.peripheralDeviceType == 0x00 ||
                data.peripheralDeviceType == 0x1F);

    ASSERT_TRUE(data.responseDataFormat == 0x02 ||
                data.responseDataFormat == 0x03);

    char vendorId[sizeof(data.vendorIdentification) + 1] = {0};
    memcpy(vendorId, data.vendorIdentification, sizeof(data.vendorIdentification));
    USB_HILOGI(MODULE_USB_SERVICE, "Vendor: %{public}s", vendorId);

    char productId[sizeof(data.productIdentification) + 1] = {0};
    memcpy(productId, data.productIdentification, sizeof(data.productIdentification));
    USB_HILOGI(MODULE_USB_SERVICE, "Product: %{public}s", productId);
}

void UsbScsiCommandTest::ValidateReadCapacityData(const ScsiReadCapacityData& data)
{
    USB_HILOGI(MODULE_USB_SERVICE, "ValidateReadCapacityData: lastLBA=%{public}u blockSize=%{public}u",
                data.lastLogicalBlockAddress, data.blockLengthInBytes);

    ASSERT_GT(data.lastLogicalBlockAddress, 0);
    ASSERT_TRUE(data.blockLengthInBytes == 512 ||
                data.blockLengthInBytes == 1024 ||
                data.blockLengthInBytes == 2048 ||
                data.blockLengthInBytes == 4096);
}

void UsbScsiCommandTest::ValidateSenseData(const ScsiSenseData& data)
{
    USB_HILOGI(MODULE_USB_SERVICE, "ValidateSenseData: errorCode=0x%{public}X senseKey=0x%{public}X "
                "ASC=0x%{public}X ASCQ=0x%{public}X",
                data.errorCode, data.senseKey, data.additionalSenseCode,
                data.additionalSenseCodeQualifier);

    std::string senseKeyStr = GetSenseKeyString(data.senseKey);
    USB_HILOGI(MODULE_USB_SERVICE, "Sense Key: %{public}s", senseKeyStr.c_str());

    std::string ascStr = GetAdditionalSenseCodeString(data.additionalSenseCode);
    USB_HILOGI(MODULE_USB_SERVICE, "ASC: %{public}s", ascStr.c_str());
}

std::string UsbScsiCommandTest::GetSenseKeyString(uint8_t senseKey)
{
    switch (senseKey) {
        case SCSI_SENSE_NO_SENSE:
            return "NO SENSE";
        case SCSI_SENSE_RECOVERED_ERROR:
            return "RECOVERED ERROR";
        case SCSI_SENSE_NOT_READY:
            return "NOT READY";
        case SCSI_SENSE_MEDIUM_ERROR:
            return "MEDIUM ERROR";
        case SCSI_SENSE_HARDWARE_ERROR:
            return "HARDWARE ERROR";
        case SCSI_SENSE_ILLEGAL_REQUEST:
            return "ILLEGAL REQUEST";
        case SCSI_SENSE_UNIT_ATTENTION:
            return "UNIT ATTENTION";
        case SCSI_SENSE_DATA_PROTECT:
            return "DATA PROTECT";
        case SCSI_SENSE_BLANK_CHECK:
            return "BLANK CHECK";
        case SCSI_SENSE_ABORTED_COMMAND:
            return "ABORTED COMMAND";
        case SCSI_SENSE_VOLUME_OVERFLOW:
            return "VOLUME OVERFLOW";
        case SCSI_SENSE_MISCOMPARE:
            return "MISCOMPARE";
        default:
            return "UNKNOWN";
    }
}

std::string UsbScsiCommandTest::GetAdditionalSenseCodeString(uint8_t asc)
{
    switch (asc) {
        case SCSI_ASC_NO_ADDITIONAL_SENSE_INFO:
            return "NO ADDITIONAL SENSE INFO";
        case SCSI_ASC_NOT_READY_TO_READY_TRANSITION:
            return "NOT READY TO READY TRANSITION";
        case SCSI_ASC_MEDIUM_NOT_PRESENT:
            return "MEDIUM NOT PRESENT";
        case SCSI_ASC_UNRECOVERED_READ_ERROR:
            return "UNRECOVERED READ ERROR";
        case SCSI_ASC_WRITE_ERROR:
            return "WRITE ERROR";
        case SCSI_ASC_INVALID_FIELD_IN_CDB:
            return "INVALID FIELD IN CDB";
        case SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST:
            return "INVALID FIELD IN PARAMETER LIST";
        case SCSI_ASC_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE:
            return "LBA OUT OF RANGE";
        case SCSI_ASC_WRITE_PROTECTED:
            return "WRITE PROTECTED";
        case SCSI_ASC_NOT_READY:
            return "NOT READY";
        default:
            return "UNKNOWN ASC";
    }
}

bool UsbScsiCommandTest::CheckDeviceReady(uint32_t maxRetries)
{
    USB_HILOGI(MODULE_USB_SERVICE, "CheckDeviceReady maxRetries=%{public}u", maxRetries);

    for (uint32_t i = 0; i < maxRetries; i++) {
        auto ret = massStorageInterface_->CheckDeviceReady(1);
        if (ret == UsbTransferResult::USB_OK) {
            USB_HILOGI(MODULE_USB_SERVICE, "Device ready at attempt %{public}u", i + 1);
            return true;
        }

        ScsiSenseData senseData;
        auto senseRet = massStorageInterface_->RequestSense(senseData);
        if (senseRet == UsbTransferResult::USB_OK) {
            USB_HILOGI(MODULE_USB_SERVICE, "SenseData: key=0x%{public}X ASC=0x%{public}X",
                        senseData.senseKey, senseData.additionalSenseCode);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    USB_HILOGE(MODULE_USB_SERVICE, "Device not ready after %{public}u retries", maxRetries);
    return false;
}

bool UsbScsiCommandTest::WaitForDeviceReady(uint32_t timeoutMs)
{
    USB_HILOGI(MODULE_USB_SERVICE, "WaitForDeviceReady timeout=%{public}u ms", timeoutMs);

    auto startTime = std::chrono::steady_clock::now();
    uint32_t retryCount = 0;

    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - startTime).count();

        if (elapsed >= timeoutMs) {
            USB_HILOGE(MODULE_USB_SERVICE, "Device ready timeout after %{public}u ms", timeoutMs);
            return false;
        }

        auto ret = massStorageInterface_->SendTestUnitReady();
        if (ret == UsbTransferResult::USB_OK) {
            USB_HILOGI(MODULE_USB_SERVICE, "Device ready after %{public}u ms", elapsed);
            return true;
        }

        retryCount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}

/**
 * @tc.name: UsbScsiCommand_BuildCbw_001
 * @tc.desc: Test building CBW with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_BuildCbw_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCbw_001 start");

    ScsiCommandBlockWrapper cbw;
    memset(&cbw, 0, sizeof(ScsiCommandBlockWrapper));
    cbw.signature = CBW_SIGNATURE;
    cbw.tag = 100;
    cbw.dataTransferLength = 512;
    cbw.flags = 0x80;
    cbw.lun = 0;
    cbw.cdbLength = 10;

    EXPECT_EQ(cbw.signature, CBW_SIGNATURE);
    EXPECT_EQ(cbw.tag, 100);
    EXPECT_EQ(cbw.dataTransferLength, 512);
    EXPECT_EQ(cbw.flags, 0x80);
    EXPECT_EQ(cbw.lun, 0);
    EXPECT_EQ(cbw.cdbLength, 10);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCbw_001 end");
}

/**
 * @tc.name: UsbScsiCommand_BuildCbw_002
 * @tc.desc: Test building CBW with different transfer lengths
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_BuildCbw_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCbw_002 start");

    std::vector<uint32_t> transferLengths = {0, 512, 1024, 4096, 65536, 1048576};

    for (auto length : transferLengths) {
        ScsiCommandBlockWrapper cbw;
        memset(&cbw, 0, sizeof(ScsiCommandBlockWrapper));
        cbw.signature = CBW_SIGNATURE;
        cbw.tag = length;
        cbw.dataTransferLength = length;
        cbw.flags = length % 2 == 0 ? 0x80 : 0;
        cbw.lun = length % 16;
        cbw.cdbLength = 10;

        EXPECT_EQ(cbw.signature, CBW_SIGNATURE);
        EXPECT_EQ(cbw.dataTransferLength, length);
        USB_HILOGI(MODULE_USB_SERVICE, "CBW length=%{public}u created successfully", length);
    }

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCbw_002 end");
}

/**
 * @tc.name: UsbScsiCommand_BuildCommandBlock_001
 * @tc.desc: Test building SCSI command block for INQUIRY
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_BuildCommandBlock_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_001 start");

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_OPCODE_INQUIRY, 0, 0);

    EXPECT_EQ(cb.opcode, SCSI_OPCODE_INQUIRY);
    EXPECT_EQ(cb.blockLengthMsb, 0);
    EXPECT_EQ(cb.blockLengthLsb, 36);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_001 end");
}

/**
 * @tc.name: UsbScsiCommand_BuildCommandBlock_002
 * @tc.desc: Test building SCSI command block for TEST UNIT READY
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_BuildCommandBlock_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_002 start");

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_OPCODE_TEST_UNIT_READY, 0, 0);

    EXPECT_EQ(cb.opcode, SCSI_OPCODE_TEST_UNIT_READY);
    EXPECT_EQ(cb.blockLengthMsb, 0);
    EXPECT_EQ(cb.blockLengthLsb, 0);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_002 end");
}

/**
 * @tc.name: UsbScsiCommand_BuildCommandBlock_003
 * @tc.desc: Test building SCSI command block for READ CAPACITY 10
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_BuildCommandBlock_003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_003 start");

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_OPCODE_READ_CAPACITY_10, 0, 0);

    EXPECT_EQ(cb.opcode, SCSI_OPCODE_READ_CAPACITY_10);
    EXPECT_EQ(cb.blockLengthMsb, 0);
    EXPECT_EQ(cb.dataLength[0], 0);
    EXPECT_EQ(cb.dataLength[1], 0);
    EXPECT_EQ(cb.dataLength[2], 0);
    EXPECT_EQ(cb.dataLength[3], 0);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_003 end");
}

/**
 * @tc.name: UsbScsiCommand_BuildCommandBlock_004
 * @tc.desc: Test building SCSI command block for READ 10
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_BuildCommandBlock_004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_004 start");

    uint32_t lba = 0x00001000;
    uint32_t transferLength = 8;

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_OPCODE_READ_10, lba, transferLength);

    EXPECT_EQ(cb.opcode, SCSI_OPCODE_READ_10);
    EXPECT_EQ(cb.blockLengthMsb, (lba >> 24) & 0xFF);
    EXPECT_EQ(cb.dataLength[0], (lba >> 16) & 0xFF);
    EXPECT_EQ(cb.dataLength[1], (lba >> 8) & 0xFF);
    EXPECT_EQ(cb.dataLength[2], lba & 0xFF);
    EXPECT_EQ(cb.blockLengthLsb, (transferLength >> 8) & 0xFF);
    EXPECT_EQ(cb.reserved[0], transferLength & 0xFF);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_004 end");
}

/**
 * @tc.name: UsbScsiCommand_BuildCommandBlock_005
 * @tc.desc: Test building SCSI command block for WRITE 10
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_BuildCommandBlock_005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_005 start");

    uint32_t lba = 0x00002000;
    uint32_t transferLength = 16;

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_OPCODE_WRITE_10, lba, transferLength);

    EXPECT_EQ(cb.opcode, SCSI_OPCODE_WRITE_10);
    EXPECT_EQ(cb.blockLengthMsb, (lba >> 24) & 0xFF);
    EXPECT_EQ(cb.dataLength[0], (lba >> 16) & 0xFF);
    EXPECT_EQ(cb.dataLength[1], (lba >> 8) & 0xFF);
    EXPECT_EQ(cb.dataLength[2], lba & 0xFF);
    EXPECT_EQ(cb.blockLengthLsb, (transferLength >> 8) & 0xFF);
    EXPECT_EQ(cb.reserved[0], transferLength & 0xFF);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_005 end");
}

/**
 * @tc.name: UsbScsiCommand_BuildCommandBlock_006
 * @tc.desc: Test building SCSI command block for REQUEST SENSE
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_BuildCommandBlock_006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_006 start");

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_OPCODE_REQUEST_SENSE, 0, 0);

    EXPECT_EQ(cb.opcode, SCSI_OPCODE_REQUEST_SENSE);
    EXPECT_EQ(cb.blockLengthMsb, 0);
    EXPECT_EQ(cb.blockLengthLsb, 18);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_006 end");
}

/**
 * @tc.name: UsbScsiCommand_BuildCommandBlock_007
 * @tc.desc: Test building SCSI command block with various LBA values
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_BuildCommandBlock_007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_007 start");

    std::vector<uint32_t> lbaValues = {0, 1, 255, 256, 65535, 65536, 0xFFFFFF, 0xFFFFFFFF};

    for (auto lba : lbaValues) {
        ScsiCommandBlock cb;
        BuildScsiCommandBlock(cb, SCSI_OPCODE_READ_10, lba, 1);

        EXPECT_EQ(cb.opcode, SCSI_OPCODE_READ_10);
        EXPECT_EQ(cb.blockLengthMsb, (lba >> 24) & 0xFF);
        EXPECT_EQ(cb.dataLength[0], (lba >> 16) & 0xFF);
        EXPECT_EQ(cb.dataLength[1], (lba >> 8) & 0xFF);
        EXPECT_EQ(cb.dataLength[2], lba & 0xFF);

        USB_HILOGI(MODULE_USB_SERVICE, "LBA=0x%{public}X command built successfully", lba);
    }

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_007 end");
}

/**
 * @tc.name: UsbScsiCommand_BuildCommandBlock_008
 * @tc.desc: Test building SCSI command block for SYNCHRONIZE CACHE 10
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_BuildCommandBlock_008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_008 start");

    uint32_t lba = 0;
    uint32_t blocks = 0;

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_OPCODE_SYNCHRONIZE_CACHE_10, lba, blocks);

    EXPECT_EQ(cb.opcode, SCSI_OPCODE_SYNCHRONIZE_CACHE_10);
    EXPECT_EQ(cb.blockLengthMsb, 0);
    EXPECT_EQ(cb.dataLength[0], 0);
    EXPECT_EQ(cb.dataLength[1], 0);
    EXPECT_EQ(cb.dataLength[2], 0);
    EXPECT_EQ(cb.blockLengthLsb, 0);
    EXPECT_EQ(cb.reserved[0], 0);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_BuildCommandBlock_008 end");
}

/**
 * @tc.name: UsbScsiCommand_ValidateInquiryData_001
 * @tc.desc: Test validation of valid INQUIRY data
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_ValidateInquiryData_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_ValidateInquiryData_001 start");

    ScsiInquiryData inquiryData;
    memset(&inquiryData, 0, sizeof(ScsiInquiryData));
    inquiryData.peripheralDeviceType = 0x00;
    inquiryData.removable = 0x80;
    inquiryData.version = 0x06;
    inquiryData.responseDataFormat = 0x02;
    inquiryData.additionalLength = 0x1F;
    memcpy(inquiryData.vendorIdentification, "VENDOR", 6);
    memcpy(inquiryData.productIdentification, "PRODUCT", 7);
    memcpy(inquiryData.productRevisionLevel, "1.00", 4);

    ValidateInquiryData(inquiryData);

    EXPECT_EQ(inquiryData.peripheralDeviceType, 0x00);
    EXPECT_EQ(inquiryData.responseDataFormat, 0x02);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_ValidateInquiryData_001 end");
}

/**
 * @tc.name: UsbScsiCommand_ValidateReadCapacityData_001
 * @tc.desc: Test validation of valid READ CAPACITY data
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_ValidateReadCapacityData_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_ValidateReadCapacityData_001 start");

    ScsiReadCapacityData capacityData;
    capacityData.lastLogicalBlockAddress = 0x0007A119;
    capacityData.blockLengthInBytes = 512;

    ValidateReadCapacityData(capacityData);

    EXPECT_GT(capacityData.lastLogicalBlockAddress, 0);
    EXPECT_EQ(capacityData.blockLengthInBytes, 512);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_ValidateReadCapacityData_001 end");
}

/**
 * @tc.name: UsbScsiCommand_ValidateReadCapacityData_002
 * @tc.desc: Test validation of READ CAPACITY with different block sizes
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_ValidateReadCapacityData_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_ValidateReadCapacityData_002 start");

    std::vector<uint32_t> blockSizes = {512, 1024, 2048, 4096};

    for (auto blockSize : blockSizes) {
        ScsiReadCapacityData capacityData;
        capacityData.lastLogicalBlockAddress = 0x0007A119;
        capacityData.blockLengthInBytes = blockSize;

        ValidateReadCapacityData(capacityData);

        EXPECT_GT(capacityData.lastLogicalBlockAddress, 0);
        EXPECT_EQ(capacityData.blockLengthInBytes, blockSize);

        USB_HILOGI(MODULE_USB_SERVICE, "BlockSize=%{public}u validated successfully", blockSize);
    }

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_ValidateReadCapacityData_002 end");
}

/**
 * @tc.name: UsbScsiCommand_ValidateSenseData_001
 * @tc.desc: Test validation of SENSE data with NO SENSE
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_ValidateSenseData_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_ValidateSenseData_001 start");

    ScsiSenseData senseData;
    memset(&senseData, 0, sizeof(ScsiSenseData));
    senseData.errorCode = 0x70;
    senseData.senseKey = SCSI_SENSE_NO_SENSE;
    senseData.additionalSenseCode = SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;

    ValidateSenseData(senseData);

    EXPECT_EQ(senseData.senseKey, SCSI_SENSE_NO_SENSE);
    EXPECT_EQ(senseData.additionalSenseCode, SCSI_ASC_NO_ADDITIONAL_SENSE_INFO);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_ValidateSenseData_001 end");
}

/**
 * @tc.name: UsbScsiCommand_ValidateSenseData_002
 * @tc.desc: Test validation of SENSE data with NOT READY
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_ValidateSenseData_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_ValidateSenseData_002 start");

    ScsiSenseData senseData;
    memset(&senseData, 0, sizeof(ScsiSenseData));
    senseData.errorCode = 0x70;
    senseData.senseKey = SCSI_SENSE_NOT_READY;
    senseData.additionalSenseCode = SCSI_ASC_NOT_READY;

    ValidateSenseData(senseData);

    EXPECT_EQ(senseData.senseKey, SCSI_SENSE_NOT_READY);
    EXPECT_EQ(senseData.additionalSenseCode, SCSI_ASC_NOT_READY);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_ValidateSenseData_002 end");
}

/**
 * @tc.name: UsbScsiCommand_GetSenseKeyString_001
 * @tc.desc: Test sense key string conversion for all keys
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_GetSenseKeyString_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_GetSenseKeyString_001 start");

    std::vector<std::pair<uint8_t, std::string>> testCases = {
        {SCSI_SENSE_NO_SENSE, "NO SENSE"},
        {SCSI_SENSE_RECOVERED_ERROR, "RECOVERED ERROR"},
        {SCSI_SENSE_NOT_READY, "NOT READY"},
        {SCSI_SENSE_MEDIUM_ERROR, "MEDIUM ERROR"},
        {SCSI_SENSE_HARDWARE_ERROR, "HARDWARE ERROR"},
        {SCSI_SENSE_ILLEGAL_REQUEST, "ILLEGAL REQUEST"},
        {SCSI_SENSE_UNIT_ATTENTION, "UNIT ATTENTION"},
        {SCSI_SENSE_DATA_PROTECT, "DATA PROTECT"},
        {SCSI_SENSE_ABORTED_COMMAND, "ABORTED COMMAND"},
    };

    for (const auto& testCase : testCases) {
        std::string result = GetSenseKeyString(testCase.first);
        EXPECT_EQ(result, testCase.second);
        USB_HILOGI(MODULE_USB_SERVICE, "SenseKey 0x%{public}X -> %{public}s",
                    testCase.first, result.c_str());
    }

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_GetSenseKeyString_001 end");
}

/**
 * @tc.name: UsbScsiCommand_GetAdditionalSenseCodeString_001
 * @tc.desc: Test ASC string conversion for common codes
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_GetAdditionalSenseCodeString_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_GetAdditionalSenseCodeString_001 start");

    std::vector<std::pair<uint8_t, std::string>> testCases = {
        {SCSI_ASC_NO_ADDITIONAL_SENSE_INFO, "NO ADDITIONAL SENSE INFO"},
        {SCSI_ASC_NOT_READY_TO_READY_TRANSITION, "NOT READY TO READY TRANSITION"},
        {SCSI_ASC_MEDIUM_NOT_PRESENT, "MEDIUM NOT PRESENT"},
        {SCSI_ASC_UNRECOVERED_READ_ERROR, "UNRECOVERED READ ERROR"},
        {SCSI_ASC_WRITE_ERROR, "WRITE ERROR"},
        {SCSI_ASC_INVALID_FIELD_IN_CDB, "INVALID FIELD IN CDB"},
        {SCSI_ASC_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE, "LBA OUT OF RANGE"},
        {SCSI_ASC_WRITE_PROTECTED, "WRITE PROTECTED"},
    };

    for (const auto& testCase : testCases) {
        std::string result = GetAdditionalSenseCodeString(testCase.first);
        EXPECT_EQ(result, testCase.second);
        USB_HILOGI(MODULE_USB_SERVICE, "ASC 0x%{public}X -> %{public}s",
                    testCase.first, result.c_str());
    }

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_GetAdditionalSenseCodeString_001 end");
}

/**
 * @tc.name: UsbScsiCommand_Constants_001
 * @tc.desc: Test SCSI command constants
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_Constants_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_Constants_001 start");

    EXPECT_EQ(SCSI_OPCODE_TEST_UNIT_READY, 0x00);
    EXPECT_EQ(SCSI_OPCODE_INQUIRY, 0x12);
    EXPECT_EQ(SCSI_OPCODE_MODE_SENSE_6, 0x1A);
    EXPECT_EQ(SCSI_OPCODE_READ_CAPACITY_10, 0x25);
    EXPECT_EQ(SCSI_OPCODE_READ_10, 0x28);
    EXPECT_EQ(SCSI_OPCODE_WRITE_10, 0x2A);
    EXPECT_EQ(SCSI_OPCODE_REQUEST_SENSE, 0x03);
    EXPECT_EQ(SCSI_OPCODE_SYNCHRONIZE_CACHE_10, 0x35);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_Constants_001 end");
}

/**
 * @tc.name: UsbScsiCommand_Signature_001
 * @tc.desc: Test CBW and CSW signature values
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_Signature_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_Signature_001 start");

    EXPECT_EQ(CBW_SIGNATURE, 0x43425355);
    EXPECT_EQ(CSW_SIGNATURE, 0x53425355);

    USB_HILOGI(MODULE_USB_SERVICE, "CBW Signature: 0x%{public}X", CBW_SIGNATURE);
    USB_HILOGI(MODULE_USB_SERVICE, "CSW Signature: 0x%{public}X", CSW_SIGNATURE);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_Signature_001 end");
}

/**
 * @tc.name: UsbScsiCommand_DataStructureSize_001
 * @tc.desc: Test size of SCSI data structures
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_DataStructureSize_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_DataStructureSize_001 start");

    EXPECT_EQ(sizeof(ScsiCommandBlock), 16);
    EXPECT_EQ(sizeof(ScsiCommandBlockWrapper), 31);
    EXPECT_EQ(sizeof(ScsiCommandStatusWrapper), 13);
    EXPECT_EQ(sizeof(ScsiInquiryData), 36);
    EXPECT_EQ(sizeof(ScsiReadCapacityData), 8);
    EXPECT_EQ(sizeof(ScsiSenseData), 18);

    USB_HILOGI(MODULE_USB_SERVICE, "ScsiCommandBlock size: %{public}zu", sizeof(ScsiCommandBlock));
    USB_HILOGI(MODULE_USB_SERVICE, "ScsiCommandBlockWrapper size: %{public}zu",
                sizeof(ScsiCommandBlockWrapper));
    USB_HILOGI(MODULE_USB_SERVICE, "ScsiCommandStatusWrapper size: %{public}zu",
                sizeof(ScsiCommandStatusWrapper));
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_DataStructureSize_001 end");
}

/**
 * @tc.name: UsbScsiCommand_OpcodeValidation_001
 * @tc.desc: Test SCSI opcode range validation
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_OpcodeValidation_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_OpcodeValidation_001 start");

    std::vector<uint8_t> validOpcodes = {
        SCSI_OPCODE_TEST_UNIT_READY,
        SCSI_OPCODE_INQUIRY,
        SCSI_OPCODE_MODE_SENSE_6,
        SCSI_OPCODE_READ_CAPACITY_10,
        SCSI_OPCODE_READ_10,
        SCSI_OPCODE_WRITE_10,
        SCSI_OPCODE_REQUEST_SENSE,
        SCSI_OPCODE_SYNCHRONIZE_CACHE_10,
    };

    for (auto opcode : validOpcodes) {
        EXPECT_GE(opcode, 0x00);
        EXPECT_LE(opcode, 0xFF);
        USB_HILOGI(MODULE_USB_SERVICE, "Opcode 0x%{public}X is valid", opcode);
    }

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_OpcodeValidation_001 end");
}

/**
 * @tc.name: UsbScsiCommand_SenseKeyValidation_001
 * @tc.desc: Test SCSI sense key range validation
 * @tc.type: FUNC
 */
HWTEST_F(UsbScsiCommandTest, UsbScsiCommand_SenseKeyValidation_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_SenseKeyValidation_001 start");

    std::vector<uint8_t> validSenseKeys = {
        SCSI_SENSE_NO_SENSE,
        SCSI_SENSE_RECOVERED_ERROR,
        SCSI_SENSE_NOT_READY,
        SCSI_SENSE_MEDIUM_ERROR,
        SCSI_SENSE_HARDWARE_ERROR,
        SCSI_SENSE_ILLEGAL_REQUEST,
        SCSI_SENSE_UNIT_ATTENTION,
        SCSI_SENSE_DATA_PROTECT,
        SCSI_SENSE_ABORTED_COMMAND,
    };

    for (auto senseKey : validSenseKeys) {
        EXPECT_GE(senseKey, 0x00);
        EXPECT_LE(senseKey, 0x0E);
        USB_HILOGI(MODULE_USB_SERVICE, "SenseKey 0x%{public}X is valid", senseKey);
    }

    USB_HILOGI(MODULE_USB_SERVICE, "UsbScsiCommand_SenseKeyValidation_001 end");
}

} // namespace ScsiTest
} // namespace USB
} // namespace OHOS