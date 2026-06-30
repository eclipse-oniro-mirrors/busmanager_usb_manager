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

#ifndef USB_SCSI_COMMAND_TEST_H
#define USB_SCSI_COMMAND_TEST_H

#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <memory>
#include "usb_mass_storage_interface.h"
#include "usb_mass_storage_factory.h"

namespace OHOS {
namespace USB {
namespace ScsiTest {

constexpr uint32_t SCSI_COMMAND_MAX_SIZE = 16;
constexpr uint32_t SCSI_SENSE_DATA_SIZE = 18;
constexpr uint32_t SCSI_BLOCK_SIZE = 512;
constexpr uint32_t SCSI_MAX_LBA = 0xFFFFFFFF;

enum ScsiCommandOpcode : uint8_t {
    SCSI_OPCODE_TEST_UNIT_READY = 0x00,
    SCSI_OPCODE_INQUIRY = 0x12,
    SCSI_OPCODE_MODE_SENSE_6 = 0x1A,
    SCSI_OPCODE_MODE_SENSE_10 = 0x5A,
    SCSI_OPCODE_READ_CAPACITY_10 = 0x25,
    SCSI_OPCODE_READ_CAPACITY_16 = 0x9E,
    SCSI_OPCODE_READ_10 = 0x28,
    SCSI_OPCODE_READ_16 = 0x88,
    SCSI_OPCODE_WRITE_10 = 0x2A,
    SCSI_OPCODE_WRITE_16 = 0x8A,
    SCSI_OPCODE_REQUEST_SENSE = 0x03,
    SCSI_OPCODE_START_STOP_UNIT = 0x1B,
    SCSI_OPCODE_PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E,
    SCSI_OPCODE_SYNCHRONIZE_CACHE_10 = 0x35,
    SCSI_OPCODE_VERIFY_10 = 0x2F,
    SCSI_OPCODE_FORMAT_UNIT = 0x04,
    SCSI_OPCODE_REASSIGN_BLOCKS = 0x07,
    SCSI_OPCODE_SEND_DIAGNOSTIC = 0x1D,
    SCSI_OPCODE_RECEIVE_DIAGNOSTIC = 0x1C,
    SCSI_OPCODE_MODE_SELECT_6 = 0x15,
    SCSI_OPCODE_MODE_SELECT_10 = 0x55,
};

enum ScsiSenseKey : uint8_t {
    SCSI_SENSE_NO_SENSE = 0x00,
    SCSI_SENSE_RECOVERED_ERROR = 0x01,
    SCSI_SENSE_NOT_READY = 0x02,
    SCSI_SENSE_MEDIUM_ERROR = 0x03,
    SCSI_SENSE_HARDWARE_ERROR = 0x04,
    SCSI_SENSE_ILLEGAL_REQUEST = 0x05,
    SCSI_SENSE_UNIT_ATTENTION = 0x06,
    SCSI_SENSE_DATA_PROTECT = 0x07,
    SCSI_SENSE_BLANK_CHECK = 0x08,
    SCSI_SENSE_VENDOR_SPECIFIC = 0x09,
    SCSI_SENSE_COPY_ABORTED = 0x0A,
    SCSI_SENSE_ABORTED_COMMAND = 0x0B,
    SCSI_SENSE_VOLUME_OVERFLOW = 0x0D,
    SCSI_SENSE_MISCOMPARE = 0x0E,
};

enum ScsiAdditionalSenseCode : uint8_t {
    SCSI_ASC_NO_ADDITIONAL_SENSE_INFO = 0x00,
    SCSI_ASC_NOT_READY_TO_READY_TRANSITION = 0x28,
    SCSI_ASC_MEDIUM_NOT_PRESENT = 0x3A,
    SCSI_ASC_UNRECOVERED_READ_ERROR = 0x11,
    SCSI_ASC_WRITE_ERROR = 0x0C,
    SCSI_ASC_INVALID_FIELD_IN_CDB = 0x24,
    SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST = 0x26,
    SCSI_ASC_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE = 0x21,
    SCSI_ASC_WRITE_PROTECTED = 0x27,
    SCSI_ASC_MEDIUM_MAY_HAVE_CHANGED = 0x28,
    SCSI_ASC_NOT_READY = 0x04,
    SCSI_ASC_INITIALIZING_COMMAND_REQUIRED = 0x04,
    SCSI_ASC_MUSIC_CD = 0x64,
    SCSI_ASC_DATA_CD = 0x65,
};

struct ScsiCommandBlock {
    uint8_t opcode;
    uint8_t lun;
    uint8_t blockLengthMsb;
    uint8_t blockLengthLsb;
    uint8_t dataLength[4];
    uint8_t reserved[7];
    uint8_t control;
};

struct ScsiCommandBlockWrapper {
    uint32_t signature;
    uint32_t tag;
    uint32_t dataTransferLength;
    uint8_t flags;
    uint8_t lun;
    uint8_t cdbLength;
    uint8_t reserved;
    ScsiCommandBlock cbw;
};

struct ScsiCommandStatusWrapper {
    uint32_t signature;
    uint32_t tag;
    uint32_t dataResidue;
    uint8_t status;
};

struct ScsiInquiryData {
    uint8_t peripheralDeviceType;
    uint8_t removable;
    uint8_t version;
    uint8_t responseDataFormat;
    uint8_t additionalLength;
    uint8_t flags[3];
    uint8_t vendorIdentification[8];
    uint8_t productIdentification[16];
    uint8_t productRevisionLevel[4];
};

struct ScsiReadCapacityData {
    uint32_t lastLogicalBlockAddress;
    uint32_t blockLengthInBytes;
};

struct ScsiSenseData {
    uint8_t errorCode;
    uint8_t segmentNumber;
    uint8_t senseKey;
    uint8_t information[4];
    uint8_t additionalSenseLength;
    uint8_t commandSpecificInformation[4];
    uint8_t additionalSenseCode;
    uint8_t additionalSenseCodeQualifier;
    uint8_t fieldReplaceableUnitCode;
    uint8_t senseKeySpecific[3];
};

struct ScsiTestConfig {
    uint8_t busNum;
    uint8_t devAddr;
    uint16_t vendorId;
    uint16_t productId;
    uint32_t expectedBlockCount;
    uint32_t expectedBlockSize;
    uint32_t testTransferSize;
    uint32_t timeoutMs;
};

class UsbScsiCommandTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    bool InitializeMassStorageDevice(const ScsiTestConfig& config);
    bool CleanupMassStorageDevice();

    void ValidateInquiryData(const ScsiInquiryData& data);
    void ValidateReadCapacityData(const ScsiReadCapacityData& data);
    void ValidateSenseData(const ScsiSenseData& data);

    std::string GetSenseKeyString(uint8_t senseKey);
    std::string GetAdditionalSenseCodeString(uint8_t asc);

    bool CheckDeviceReady(uint32_t maxRetries = 5);
    bool WaitForDeviceReady(uint32_t timeoutMs = 5000);

protected:
    ScsiTestConfig testConfig_;
    std::unique_ptr<IUsbMassStorageInterface> massStorageInterface_;
    bool deviceInitialized_ = false;
};

} // namespace ScsiTest
} // namespace USB
} // namespace OHOS

#endif // USB_SCSI_COMMAND_TEST_H