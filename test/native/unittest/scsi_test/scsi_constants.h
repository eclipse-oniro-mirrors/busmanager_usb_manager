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

#ifndef SCSI_CONSTANTS_H
#define SCSI_CONSTANTS_H

#include <cstdint>

// ==================== Common Numeric Macros ====================
// Frequently used numeric values for test cases, assignments, and array indexing
// Usage: NUM_0, NUM_1, NUM_100, etc. for direct numeric values in code

// Basic numeric constants (0-20)
#define NUM_0   0
#define NUM_1   1
#define NUM_2   2
#define NUM_3   3
#define NUM_4   4
#define NUM_5   5
#define NUM_6   6
#define NUM_7   7
#define NUM_8   8
#define NUM_9   9
#define NUM_10  10
#define NUM_11  11
#define NUM_12  12
#define NUM_13  13
#define NUM_14  14
#define NUM_15  15
#define NUM_16  16
#define NUM_17  17
#define NUM_18  18
#define NUM_19  19
#define NUM_20  20

// Extended numeric constants
#define NUM_24     24
#define NUM_31     31
#define NUM_64     64
#define NUM_100    100
#define NUM_255    255
#define NUM_256    256
#define NUM_5000   5000
#define NUM_30000  30000

// Block size constants
#define NUM_512    512
#define NUM_1024   1024
#define NUM_2048   2048
#define NUM_4096   4096
#define NUM_8192   8192
#define NUM_16384  16384
#define NUM_32768  32768
#define NUM_65535  65535
#define NUM_65536  65536

// Array size constants
#define ARRAY_SIZE_4   4
#define ARRAY_SIZE_8   8
#define ARRAY_SIZE_16  16
#define ARRAY_SIZE_32  32
#define ARRAY_SIZE_64  64
#define ARRAY_SIZE_128 128
#define ARRAY_SIZE_256 256

// Hex numeric constants
#define HEX_00  0x00
#define HEX_01  0x01
#define HEX_02  0x02
#define HEX_03  0x03
#define HEX_04  0x04
#define HEX_05  0x05
#define HEX_06  0x06
#define HEX_07  0x07
#define HEX_08  0x08
#define HEX_09  0x09
#define HEX_0A  0x0A
#define HEX_0B  0x0B
#define HEX_0C  0x0C
#define HEX_0D  0x0D
#define HEX_0E  0x0E
#define HEX_0F  0x0F
#define HEX_10  0x10
#define HEX_1F  0x1F
#define HEX_20  0x20
#define HEX_24  0x24
#define HEX_25  0x25
#define HEX_28  0x28
#define HEX_35  0x35
#define HEX_40  0x40
#define HEX_50  0x50
#define HEX_64  0x64
#define HEX_70  0x70
#define HEX_71  0x71
#define HEX_80  0x80
#define HEX_88  0x88
#define HEX_9E  0x9E
#define HEX_A0  0xA0
#define HEX_A1  0xA1
#define HEX_AA  0xAA
#define HEX_B5  0xB5
#define HEX_BC  0xBC
#define HEX_D0  0xD0
#define HEX_E0  0xE0
#define HEX_FF  0xFF
#define HEX_100 0x100

// Mask constants
#define MASK_8BIT       HEX_FF
#define MASK_LOW_4BITS  HEX_0F
#define MASK_HIGH_4BITS HEX_F0
#define MASK_BIT_0      HEX_01
#define MASK_BIT_1      HEX_02
#define MASK_BIT_2      HEX_04
#define MASK_BIT_3      HEX_08
#define MASK_BIT_4      HEX_10
#define MASK_BIT_5      HEX_20
#define MASK_BIT_6      HEX_40
#define MASK_BIT_7      HEX_80

namespace OHOS {
namespace USB {

namespace ScsiConstants {

// ==================== SCSI Protocol Constants ====================

// Data direction flags for SCSI commands
constexpr uint8_t SCSI_DATA_DIR_IN = 0x80;   // Data transfer from device to host
constexpr uint8_t SCSI_DATA_DIR_OUT = 0x00;  // Data transfer from host to device

// Default Logical Unit Number (LUN) for mass storage devices
constexpr uint8_t SCSI_LUN_DEFAULT = 0x00;

// SCSI command status codes
constexpr uint8_t SCSI_STATUS_SUCCESS = 0x00;  // Command completed successfully

// ==================== Data Structure Sizes ====================

// SCSI data structure lengths
constexpr uint32_t SCSI_INQUIRY_DATA_LENGTH = 36;   // Size of INQUIRY response data
constexpr uint32_t SCSI_SENSE_DATA_LENGTH = 18;     // Size of SENSE response data
constexpr uint32_t SCSI_CBW_CDB_LENGTH = 16;        // Command Descriptor Block length in CBW
constexpr uint32_t SCSI_MODE_SENSE_MIN_LENGTH = 64; // Minimum MODE SENSE response length

// Size constants for string fields in INQUIRY data
constexpr size_t VENDOR_ID_SIZE = 8;       // Vendor identification field size
constexpr size_t PRODUCT_ID_SIZE = 16;     // Product identification field size
constexpr size_t REVISION_LEVEL_SIZE = 4;  // Product revision level field size

// SCSI command and data structure sizes
constexpr size_t SCSI_COMMAND_MAX_SIZE = 16;       // Maximum SCSI command block size
constexpr size_t SCSI_INFORMATION_SIZE = 4;        // Size of SCSI information field
constexpr size_t SCSI_KEY_SPECIFIC_SIZE = 3;       // Size of SCSI key-specific field
constexpr size_t SCSI_MODE_PAGE_HEADER_SIZE = 4;   // Mode page header size (6-byte variant)
constexpr size_t SCSI_MODE_BLOCK_DESC_SIZE = 8;    // Mode parameter block descriptor size

// ==================== Retry and Timing Constants ====================

// Maximum number of retry attempts for SCSI operations
constexpr uint32_t SCSI_MAX_RETRIES = 100;           // Maximum retries for device operations

// Delays between operations in milliseconds
constexpr uint32_t SCSI_RETRY_DELAY_MS = 100;        // Delay between retry attempts
constexpr uint32_t SCSI_SHORT_SLEEP_MS = 5;          // Short sleep for high-frequency operations
constexpr uint32_t SCSI_NORMAL_SLEEP_MS = 10;        // Normal sleep between operations

// ==================== Block Size Constants ====================

// Common SCSI device block sizes
constexpr uint32_t BLOCK_SIZE_512 = 512;   // Standard 512-byte block size
constexpr uint32_t BLOCK_SIZE_1024 = 1024; // 1KB block size
constexpr uint32_t BLOCK_SIZE_2048 = 2048; // 2KB block size (CD-ROM)
constexpr uint32_t BLOCK_SIZE_4096 = 4096; // 4KB block size (modern drives)

// ==================== Transfer Size Constants ====================

// Test transfer sizes for read/write operations
constexpr uint32_t TRANSFER_SIZE_4096 = 4096;   // 4KB transfer size
constexpr uint32_t TRANSFER_SIZE_8192 = 8192;   // 8KB transfer size
constexpr uint32_t TRANSFER_SIZE_16384 = 16384; // 16KB transfer size

// Maximum transfer limits
constexpr uint32_t MAX_TRANSFER_BLOCKS = 65535;  // Maximum blocks per transfer (16-bit limit)

// ==================== Timeout Constants ====================

// Operation timeout values in milliseconds
constexpr uint32_t TIMEOUT_MS_DEFAULT = 5000;  // Default timeout for standard operations
constexpr uint32_t TIMEOUT_MS_SHORT = 1000;    // Short timeout for quick operations
constexpr uint32_t TIMEOUT_MS_LONG = 30000;    // Long timeout for complex operations

// ==================== USB Mass Storage Signatures ====================

// USB Mass Storage Bulk-Only Transport signatures
constexpr uint32_t CBW_SIGNATURE = 0x43425355;  // "USBC" - Command Block Wrapper signature
constexpr uint32_t CSW_SIGNATURE = 0x53425355;  // "USBS" - Command Status Wrapper signature

// Command Block Tag
constexpr uint32_t TAG_INITIAL = 1;       // Initial tag value for CBW
constexpr uint32_t DEFAULT_TAG = 100;     // Default tag value used in tests

// ==================== SCSI Response Format Constants ====================

// SCSI Primary Commands (SPC) version response format codes
constexpr uint8_t SCSI_RESPONSE_FORMAT_SPC2 = 0x02; // SPC-2 or earlier format
constexpr uint8_t SCSI_RESPONSE_FORMAT_SPC3 = 0x03; // SPC-3 or later format

// ==================== SCSI Peripheral Device Types ====================

// Peripheral device type codes from SCSI INQUIRY
constexpr uint8_t SCSI_PERIPHERAL_DIRECT_ACCESS = 0x00; // Direct-access device (disk)
constexpr uint8_t SCSI_PERIPHERAL_UNKNOWN = 0x1F;      // Unknown or no device type

// ==================== SCSI Mode Parameter Offsets ====================

// Mode parameter header offsets (6-byte variant)
constexpr uint8_t SCSI_MODE_DATA_LENGTH_OFFSET = 0;          // Data length field offset
constexpr uint8_t SCSI_MODE_MEDIUM_TYPE_OFFSET = 1;          // Medium type field offset
constexpr uint8_t SCSI_MODE_DEVICE_SPECIFIC_OFFSET = 2;      // Device-specific offset
constexpr uint8_t SCSI_MODE_BLOCK_DESC_LENGTH_OFFSET = 3;    // Block descriptor length offset

// ==================== SCSI Sense Data Constants ====================

// Sense data error codes
constexpr uint8_t SCSI_ERROR_CODE_CURRENT = 0x70;  // Current errors
constexpr uint8_t SCSI_ERROR_CODE_DEFERRED = 0x71; // Deferred errors

// Additional length field range
constexpr uint8_t SCSI_ADDITIONAL_LENGTH_MIN = 31;  // Minimum valid additional length
constexpr uint8_t SCSI_ADDITIONAL_LENGTH_MAX = 255; // Maximum additional length

// ==================== SCSI Command Data Lengths ====================

// Expected data lengths for specific SCSI commands
constexpr uint8_t SCSI_INQUIRY_TEST_UNIT_READY_LEN = 36; // INQUIRY command expected length
constexpr uint8_t SCSI_INQUIRY_REQUEST_SENSE_LEN = 18;   // REQUEST SENSE expected length

// ==================== Test Device Configuration ====================

// Default test device configuration parameters
constexpr uint32_t SCSI_DEFAULT_CONFIG_BUS_NUM = 1;         // Default USB bus number
constexpr uint32_t SCSI_DEFAULT_CONFIG_DEV_ADDR = 1;        // Default device address
constexpr uint16_t SCSI_DEFAULT_CONFIG_VENDOR_ID = 0x1234;  // Test vendor ID
constexpr uint16_t SCSI_DEFAULT_CONFIG_PRODUCT_ID = 0x5678; // Test product ID

// ==================== USB Mass Storage Class Constants ====================

// USB Mass Storage Class codes
constexpr uint32_t MASS_STORAGE_CLASS = 0x08;              // Mass Storage class code
constexpr uint32_t MASS_STORAGE_SCSI_SUBCLASS = 0x06;      // SCSI transparent subclass
constexpr uint32_t MASS_STORAGE_BULK_ONLY_PROTOCOL = 0x50; // Bulk-Only transport protocol

// ==================== Bit Mask Constants ====================

// Generic bit masks for 8-bit values
constexpr uint8_t MASK_8BIT = 0xFF;   // All 8 bits set
constexpr uint8_t MASK_LOW_4BITS = 0x0F;  // Lower 4 bits (bits 0-3)
constexpr uint8_t MASK_HIGH_4BITS = 0xF0; // Upper 4 bits (bits 4-7)

// Individual bit position masks
constexpr uint8_t MASK_BIT_1 = 0x01;  // Bit 0 mask
constexpr uint8_t MASK_BIT_2 = 0x02;  // Bit 1 mask
constexpr uint8_t MASK_BIT_3 = 0x04;  // Bit 2 mask
constexpr uint8_t MASK_BIT_4 = 0x08;  // Bit 3 mask
constexpr uint8_t MASK_BIT_5 = 0x10;  // Bit 4 mask
constexpr uint8_t MASK_BIT_6 = 0x20;  // Bit 5 mask
constexpr uint8_t MASK_BIT_7 = 0x40;  // Bit 6 mask
constexpr uint8_t MASK_BIT_8 = 0x80;  // Bit 7 mask
constexpr uint8_t MASK_BIT_7_ALIAS = 0x80; // Bit 7 mask (alias for readability)

// ==================== Test Configuration Constants ====================

// Test configuration indices for array accesses and loop iterations
constexpr uint32_t ARRAY_INDEX_0 = 0;   // First array index
constexpr uint32_t ARRAY_INDEX_1 = 1;   // Second array index
constexpr uint32_t ARRAY_INDEX_2 = 2;   // Third array index
constexpr uint32_t ARRAY_INDEX_3 = 3;   // Fourth array index
constexpr uint32_t ARRAY_INDEX_4 = 4;   // Fifth array index
constexpr uint32_t ARRAY_INDEX_5 = 5;   // Sixth array index
constexpr uint32_t ARRAY_INDEX_6 = 6;   // Seventh array index
constexpr uint32_t ARRAY_INDEX_7 = 7;   // Eighth array index
constexpr uint32_t ARRAY_INDEX_8 = 8;   // Ninth array index
constexpr uint32_t ARRAY_INDEX_9 = 9;   // Tenth array index
constexpr uint32_t ARRAY_INDEX_10 = 10; // Eleventh array index
constexpr uint32_t ARRAY_INDEX_12 = 12; // Twelfth array index
constexpr uint32_t ARRAY_INDEX_20 = 20; // Twentieth array index

// ==================== Test Parameter Constants ====================

// LBA (Logical Block Address) test values
constexpr uint32_t MAX_LBA_VALUE = 0xFFFFFFFF;    // Maximum 32-bit LBA value
constexpr uint32_t DEFAULT_TEST_LBA = 0;          // Default starting LBA for tests
constexpr uint32_t TEST_LBA_OFFSET = 10;          // Offset LBA for multi-block tests
constexpr uint32_t TEST_LBA_WRITE_CACHE = 5;      // LBA for write cache effectiveness tests

// Block count test parameters
constexpr uint32_t MIN_TEST_BLOCKS = 1;           // Minimum blocks for valid operation
constexpr uint32_t MAX_TEST_BLOCKS = 256;         // Maximum blocks for standard tests

// Iteration counts for stress and concurrent testing
constexpr uint32_t MAX_TEST_ITERATIONS = 20;      // Maximum iterations for test loops
constexpr uint32_t MAX_STRESS_ITERATIONS = 20;    // Maximum iterations for stress tests
constexpr uint32_t MAX_THREAD_COUNT = 3;          // Maximum number of concurrent threads
constexpr uint32_t THREAD_ITERATIONS = 3;         // Iterations per thread in concurrent tests

// Synchronize cache test parameters
constexpr uint32_t SYNC_CACHE_TEST_COUNT = 10;    // Number of sync cache operations
constexpr uint32_t SYNC_CACHE_TEST_CYCLES = 5;    // Number of write-sync-read cycles

// ==================== Test Iteration Constants ====================

// Number of retry attempts for various operations
constexpr uint32_t TEST_INQUIRY_ITERATIONS = 5;          // INQUIRY test iterations
constexpr uint32_t TEST_RETRY_MAX_RETRIES = 3;           // Maximum retries for failed operations
constexpr uint32_t TEST_DEVICE_READY_RETRIES = 10;       // Device ready check retries

// Thread concurrent test parameters
constexpr uint32_t TEST_THREAD_ITERATIONS = 3;           // Iterations per thread in concurrent tests

// Single block count for basic operations
constexpr uint32_t TEST_SINGLE_BLOCK = 1;                // Single block read/write
constexpr uint32_t TEST_TWO_BLOCKS = 2;                  // Two block read/write
constexpr uint32_t TEST_FIVE_BLOCKS = 5;                 // Five block read/write
constexpr uint32_t TEST_TEN_BLOCKS = 10;                 // Ten block read/write

// Multi-block test parameters
constexpr uint32_t TEST_MULTI_BLOCK_COUNT = 10;          // Multi-block operation count

// Write cache test iterations
constexpr uint32_t TEST_WRITE_CACHE_ITERATIONS = 10;     // Write cache effectiveness test iterations

// ==================== Generic Byte Value Constants ====================

// Common byte values used in SCSI protocols and testing
constexpr uint8_t BYTE_NULL = 0x00;   // Null/zero byte
constexpr uint8_t BYTE_ONE = 0x01;    // Value 1
constexpr uint8_t BYTE_TWO = 0x02;    // Value 2
constexpr uint8_t BYTE_THREE = 0x03;  // Value 3
constexpr uint8_t BYTE_FOUR = 0x04;   // Value 4
constexpr uint8_t BYTE_FIVE = 0x05;   // Value 5
constexpr uint8_t BYTE_SIX = 0x06;    // Value 6
constexpr uint8_t BYTE_SEVEN = 0x07;  // Value 7
constexpr uint8_t BYTE_EIGHT = 0x08;  // Value 8
constexpr uint8_t BYTE_NINE = 0x09;   // Value 9
constexpr uint8_t BYTE_TEN = 0x0A;    // Value 10
constexpr uint8_t BYTE_ELEVEN = 0x0B; // Value 11
constexpr uint8_t BYTE_TWELVE = 0x0C; // Value 12
constexpr uint8_t BYTE_THIRTEEN = 0x0D; // Value 13
constexpr uint8_t BYTE_FOURTEEN = 0x0E; // Value 14
constexpr uint8_t BYTE_FIFTEEN = 0x0F; // Value 15
constexpr uint8_t BYTE_SIXTEEN = 0x10; // Value 16
constexpr uint8_t BYTE_SEVENTEEN = 0x11; // Value 17
constexpr uint8_t BYTE_EIGHTEEN = 0x12; // Value 18
constexpr uint8_t BYTE_NINETEEN = 0x13; // Value 19
constexpr uint8_t BYTE_TWENTY = 0x14;  // Value 20
constexpr uint8_t BYTE_TWENTY_ONE = 0x15; // Value 21
constexpr uint8_t BYTE_TWENTY_TWO = 0x16; // Value 22
constexpr uint8_t BYTE_TWENTY_THREE = 0x17; // Value 23
constexpr uint8_t BYTE_TWENTY_FOUR = 0x18; // Value 24
constexpr uint8_t BYTE_TWENTY_FIVE = 0x19; // Value 25
constexpr uint8_t BYTE_TWENTY_SIX = 0x1A; // Value 26
constexpr uint8_t BYTE_TWENTY_SEVEN = 0x1B; // Value 27
constexpr uint8_t BYTE_TWENTY_EIGHT = 0x1C; // Value 28
constexpr uint8_t BYTE_TWENTY_NINE = 0x1D; // Value 29
constexpr uint8_t BYTE_THIRTY = 0x1E;  // Value 30
constexpr uint8_t BYTE_THIRTY_ONE = 0x1F; // Value 31
constexpr uint8_t BYTE_THIRTY_TWO = 0x20; // Value 32
constexpr uint8_t BYTE_THIRTY_THREE = 0x21; // Value 33
constexpr uint8_t BYTE_THIRTY_FOUR = 0x22; // Value 34
constexpr uint8_t BYTE_THIRTY_FIVE = 0x23; // Value 35
constexpr uint8_t BYTE_THIRTY_SIX = 0x24; // Value 36
constexpr uint8_t BYTE_THIRTY_SEVEN = 0x25; // Value 37
constexpr uint8_t BYTE_THIRTY_EIGHT = 0x26; // Value 38
constexpr uint8_t BYTE_THIRTY_NINE = 0x27; // Value 39
constexpr uint8_t BYTE_FORTY = 0x28;  // Value 40
constexpr uint8_t BYTE_FORTY_ONE = 0x29; // Value 41
constexpr uint8_t BYTE_FORTY_TWO = 0x2A; // Value 42
constexpr uint8_t BYTE_FORTY_THREE = 0x2B; // Value 43
constexpr uint8_t BYTE_FORTY_FOUR = 0x2C; // Value 44
constexpr uint8_t BYTE_FORTY_FIVE = 0x2D; // Value 45
constexpr uint8_t BYTE_FORTY_SIX = 0x2E; // Value 46
constexpr uint8_t BYTE_FORTY_SEVEN = 0x2F; // Value 47
constexpr uint8_t BYTE_FORTY_EIGHT = 0x30; // Value 48
constexpr uint8_t BYTE_FORTY_NINE = 0x31; // Value 49
constexpr uint8_t BYTE_FIFTY = 0x32;  // Value 50
constexpr uint8_t BYTE_FIFTY_ONE = 0x33; // Value 51
constexpr uint8_t BYTE_FIFTY_TWO = 0x34; // Value 52
constexpr uint8_t BYTE_FIFTY_THREE = 0x35; // Value 53
constexpr uint8_t BYTE_FIFTY_FOUR = 0x36; // Value 54
constexpr uint8_t BYTE_FIFTY_FIVE = 0x37; // Value 55
constexpr uint8_t BYTE_FIFTY_SIX = 0x38; // Value 56
constexpr uint8_t BYTE_FIFTY_SEVEN = 0x39; // Value 57
constexpr uint8_t BYTE_FIFTY_EIGHT = 0x3A; // Value 58
constexpr uint8_t BYTE_FIFTY_NINE = 0x3B; // Value 59
constexpr uint8_t BYTE_SIXTY = 0x3C;  // Value 60
constexpr uint8_t BYTE_SIXTY_ONE = 0x3D; // Value 61
constexpr uint8_t BYTE_SIXTY_TWO = 0x3E; // Value 62
constexpr uint8_t BYTE_SIXTY_THREE = 0x3F; // Value 63
constexpr uint8_t BYTE_SIXTY_FOUR = 0x40; // Value 64
constexpr uint8_t BYTE_EIGHTY = 0x50; // Value 80
constexpr uint8_t BYTE_EIGHTY_FIVE = 0x55; // Value 85
constexpr uint8_t BYTE_EIGHTY_SIX = 0x56; // Value 86
constexpr uint8_t BYTE_EIGHTY_EIGHT = 0x58; // Value 88
constexpr uint8_t BYTE_HUNDRED = 0x64; // Value 100
constexpr uint8_t BYTE_HUNDRED_ONE = 0x65; // Value 101
constexpr uint8_t BYTE_HUNDRED_TWO = 0x66; // Value 102
constexpr uint8_t BYTE_ONE_HUNDRED_TWELVE = 0x70; // Value 112
constexpr uint8_t BYTE_ONE_HUNDRED_THIRTEEN = 0x71; // Value 113
constexpr uint8_t BYTE_ONE_HUNDRED_NINETEEN = 0x77; // Value 119
constexpr uint8_t BYTE_ONE_HUNDRED_TWENTY_EIGHT = 0x80; // Value 128
constexpr uint8_t BYTE_ONE_HUNDRED_THIRTY_SIX = 0x88; // Value 136
constexpr uint8_t BYTE_ONE_HUNDRED_THIRTY_SEVEN = 0x89; // Value 137
constexpr uint8_t BYTE_ONE_HUNDRED_THIRTY_EIGHT = 0x8A; // Value 138
constexpr uint8_t BYTE_ONE_HUNDRED_FORTY_FOUR = 0x90; // Value 144
constexpr uint8_t BYTE_ONE_HUNDRED_FIFTY_THREE = 0x99; // Value 153
constexpr uint8_t BYTE_ONE_HUNDRED_SEVENTY = 0xAA; // Value 170
constexpr uint8_t BYTE_ONE_HUNDRED_EIGHTY_SEVEN = 0xBB; // Value 187
constexpr uint8_t BYTE_TWO_HUNDRED_FOUR = 0xCC; // Value 204
constexpr uint8_t BYTE_TWO_HUNDRED_TWENTY_ONE = 0xDD; // Value 221
constexpr uint8_t BYTE_TWO_HUNDRED_THIRTY_EIGHT = 0xEE; // Value 238
constexpr uint8_t BYTE_TWO_HUNDRED_FIFTY_FIVE = 0xFF; // Value 255

// Bit position shift constants for byte extraction
constexpr uint8_t SHIFT_RIGHT_8 = 0x08; // Shift right by 8 bits
constexpr uint8_t SHIFT_RIGHT_16 = 0x10; // Shift right by 16 bits
constexpr uint8_t SHIFT_RIGHT_24 = 0x18; // Shift right by 24 bits

// ==================== SCSI Command Opcodes ====================

// SCSI command opcodes (from SCSI Primary Commands specification)
constexpr uint8_t SCSI_OPCODE_TEST_UNIT_READY = 0x00;         // TEST UNIT READY command
constexpr uint8_t SCSI_OPCODE_REQUEST_SENSE = 0x03;           // REQUEST SENSE command
constexpr uint8_t SCSI_OPCODE_INQUIRY = 0x12;                 // INQUIRY command
constexpr uint8_t SCSI_OPCODE_MODE_SENSE_6 = 0x1A;            // MODE SENSE (6-byte) command
constexpr uint8_t SCSI_OPCODE_READ_CAPACITY_10 = 0x25;        // READ CAPACITY (10-byte) command
constexpr uint8_t SCSI_OPCODE_READ_10 = 0x28;                 // READ (10-byte) command
constexpr uint8_t SCSI_OPCODE_WRITE_10 = 0x2A;                // WRITE (10-byte) command
constexpr uint8_t SCSI_OPCODE_SYNCHRONIZE_CACHE_10 = 0x35;    // SYNCHRONIZE CACHE (10-byte) command

// ==================== SCSI Sense Keys ====================

// SCSI sense key definitions (error categories)
constexpr uint8_t SCSI_SENSE_NO_SENSE = 0x00;           // No specific sense key
constexpr uint8_t SCSI_SENSE_RECOVERED_ERROR = 0x01;    // Error was recovered by device
constexpr uint8_t SCSI_SENSE_NOT_READY = 0x02;          // Device not ready
constexpr uint8_t SCSI_SENSE_MEDIUM_ERROR = 0x03;       // Medium (disk/tape) error
constexpr uint8_t SCSI_SENSE_HARDWARE_ERROR = 0x04;     // Hardware failure
constexpr uint8_t SCSI_SENSE_ILLEGAL_REQUEST = 0x05;    // Invalid command/parameters
constexpr uint8_t SCSI_SENSE_UNIT_ATTENTION = 0x06;     // Unit attention condition
constexpr uint8_t SCSI_SENSE_DATA_PROTECT = 0x07;       // Write-protected media
constexpr uint8_t SCSI_SENSE_BLANK_CHECK = 0x08;        // Blank media encountered
constexpr uint8_t SCSI_SENSE_ABORTED_COMMAND = 0x0B;    // Command was aborted

// Additional sense keys
constexpr uint8_t SCSI_SENSE_VOLUME_OVERFLOW = 0x0D;    // Volume overflow
constexpr uint8_t SCSI_SENSE_MISCOMPARE = 0x0E;         // Source data miscompare

// ==================== SCSI Additional Sense Codes (ASC) ====================

// Common Additional Sense Codes
constexpr uint8_t SCSI_ASC_NO_ADDITIONAL_SENSE_INFO = 0x00;              // No additional sense information
constexpr uint8_t SCSI_ASC_NOT_READY_TO_READY_TRANSITION = 0x28;         // Device became ready
constexpr uint8_t SCSI_ASC_MEDIUM_NOT_PRESENT = 0x3A;                    // No medium present
constexpr uint8_t SCSI_ASC_UNRECOVERED_READ_ERROR = 0x11;                // Unrecoverable read error
constexpr uint8_t SCSI_ASC_WRITE_ERROR = 0x0C;                           // Write error
constexpr uint8_t SCSI_ASC_INVALID_FIELD_IN_CDB = 0x24;                  // Invalid field in CDB
constexpr uint8_t SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST = 0x26;       // Invalid field in parameter list
constexpr uint8_t SCSI_ASC_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE = 0x21;    // LBA out of range
constexpr uint8_t SCSI_ASC_WRITE_PROTECTED = 0x27;                       // Medium is write-protected
constexpr uint8_t SCSI_ASC_NOT_READY = 0x04;                             // Not ready, no additional info

// ==================== SCSI Mode Page Codes ====================

// Mode page codes for MODE SENSE command
constexpr uint8_t SCSI_MODE_PAGE_RETURN_ALL = 0x3F;              // Return all mode pages
constexpr uint8_t SCSI_MODE_PAGE_VENDOR_SPECIFIC = 0x00;         // Vendor-specific page
constexpr uint8_t SCSI_MODE_PAGE_READ_WRITE_ERROR_RECOVERY = 0x01; // Read/write error recovery
constexpr uint8_t SCSI_MODE_PAGE_DISCONNECT_RECONNECT = 0x02;    // Disconnect/reconnect
constexpr uint8_t SCSI_MODE_PAGE_FORMAT_DEVICE = 0x03;           // Format device
constexpr uint8_t SCSI_MODE_PAGE_RIGID_DISK_GEOMETRY = 0x04;     // Rigid disk geometry
constexpr uint8_t SCSI_MODE_PAGE_FLEXIBLE_DISK = 0x05;           // Flexible disk
constexpr uint8_t SCSI_MODE_PAGE_VERIFY_ERROR_RECOVERY = 0x07;   // Verify error recovery
constexpr uint8_t SCSI_MODE_PAGE_CACHING = 0x08;                 // Caching mode page
constexpr uint8_t SCSI_MODE_PAGE_PERIPHERAL_DEVICE = 0x09;       // Peripheral device page
constexpr uint8_t SCSI_MODE_PAGE_CONTROL_MODE = 0x0A;            // Control mode page
constexpr uint8_t SCSI_MODE_PAGE_MEDIUM_TYPES = 0x0B;            // Medium types
constexpr uint8_t SCSI_MODE_PAGE_NOTCH_PARTITION = 0x0C;         // Notch and partition
constexpr uint8_t SCSI_MODE_PAGE_POWER_CONDITION = 0x1A;         // Power condition
constexpr uint8_t SCSI_MODE_PAGE_INFORMATIONAL_EXCEPTIONS = 0x1C; // Informational exceptions
constexpr uint8_t SCSI_MODE_PAGE_TIMEOUT_AND_PROTECT = 0x1D;     // Timeout and protect
constexpr uint8_t SCSI_MODE_PAGE_LOGICAL_UNIT = 0x1E;            // Logical unit
constexpr uint8_t SCSI_MODE_PAGE_BLOCK_DEVICE = 0x3B;            // Block device characteristics

// ==================== SCSI Caching Page Flag Bits ====================

// Caching page flag bit definitions
constexpr uint8_t SCSI_CACHE_PAGE_IC = 0x01;   // Initiator Control
constexpr uint8_t SCSI_CACHE_PAGE_ABPF = 0x02; // Abort Pre-Fetch
constexpr uint8_t SCSI_CACHE_PAGE_CAP = 0x04;  // Caching Analysis Permitted
constexpr uint8_t SCSI_CACHE_PAGE_DISC = 0x08; // Disconnect
constexpr uint8_t SCSI_CACHE_PAGE_SIZE = 0x10; // Size
constexpr uint8_t SCSI_CACHE_PAGE_WCE = 0x20;  // Write Cache Enable
constexpr uint8_t SCSI_CACHE_PAGE_MF = 0x40;   // Media Formatted
constexpr uint8_t SCSI_CACHE_PAGE_RCD = 0x80;  // Read Cache Disable

}

}
}

#endif