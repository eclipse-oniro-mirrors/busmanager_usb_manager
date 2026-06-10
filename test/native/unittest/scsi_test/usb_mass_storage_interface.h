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

#ifndef USB_MASS_STORAGE_INTERFACE_H
#define USB_MASS_STORAGE_INTERFACE_H

#include <vector>
#include <memory>
#include <cstdint>
#include "usb_device.h"
#include "usb_device_pipe.h"
#include "usb_interface.h"
#include "usb_endpoint.h"

namespace OHOS {
namespace USB {

class UsbMassStorageImpl;

struct UsbDeviceInfo {
    uint8_t busNum;
    uint8_t devAddr;
    uint16_t vendorId;
    uint16_t productId;
    uint8_t baseClass;
    uint8_t subClass;
    uint8_t protocol;
    std::string name;
    std::string manufacturerName;
    std::string productName;
    uint32_t totalBlocks;
    uint32_t blockSize;
};

struct UsbEndpointInfo {
    uint8_t interfaceId;
    uint8_t endpointAddress;
    uint32_t maxPacketSize;
    uint32_t transferType;
    uint32_t direction;
};

enum class UsbTransferResult {
    USB_OK = 0,
    USB_INVALID_PARAM = -1,
    USB_DEVICE_NOT_FOUND = -2,
    USB_NO_PERMISSION = -3,
    USB_DEVICE_BUSY = -4,
    USB_TIMEOUT = -5,
    USB_TRANSFER_FAILED = -6,
    USB_INTERFACE_ERROR = -7,
    USB_ENDPOINT_ERROR = -8,
    USB_SYNC_ERROR = -9,
};

struct ScsiCommandData {
    uint8_t opcode;
    uint8_t lun;
    uint32_t lba;
    uint32_t transferLength;
    std::vector<uint8_t> commandBlock;
    std::vector<uint8_t> dataBuffer;
    uint32_t timeout;
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

enum class ScsiOpcode : uint8_t {
    SCSI_TEST_UNIT_READY = 0x00,
    SCSI_INQUIRY = 0x12,
    SCSI_MODE_SENSE_6 = 0x1A,
    SCSI_READ_CAPACITY_10 = 0x25,
    SCSI_READ_10 = 0x28,
    SCSI_WRITE_10 = 0x2A,
    SCSI_REQUEST_SENSE = 0x03,
    SCSI_SYNCHRONIZE_CACHE_10 = 0x35,
};

class IUsbMassStorageInterface {
public:
    virtual ~IUsbMassStorageInterface() = default;

    virtual UsbTransferResult Initialize(uint8_t busNum, uint8_t devAddr) = 0;
    virtual UsbTransferResult Cleanup() = 0;
    virtual UsbTransferResult CheckDeviceReady(uint32_t maxRetries = 5) = 0;
    virtual UsbTransferResult GetDeviceInfo(UsbDeviceInfo& info) = 0;

    virtual UsbTransferResult SendInquiry(ScsiInquiryData& inquiryData) = 0;
    virtual UsbTransferResult SendTestUnitReady() = 0;
    virtual UsbTransferResult SendReadCapacity(ScsiReadCapacityData& capacityData) = 0;
    virtual UsbTransferResult RequestSense(ScsiSenseData& senseData) = 0;
    virtual UsbTransferResult Read(uint32_t lba, uint32_t blocks, std::vector<uint8_t>& data) = 0;
    virtual UsbTransferResult Write(uint32_t lba, const std::vector<uint8_t>& data) = 0;
    virtual UsbTransferResult ModeSense(uint8_t pageCode, std::vector<uint8_t>& modeData) = 0;
    virtual UsbTransferResult SynchronizeCache() = 0;

    virtual UsbTransferResult BulkRead(std::vector<uint8_t>& data, uint32_t timeout) = 0;
    virtual UsbTransferResult BulkWrite(const std::vector<uint8_t>& data, uint32_t timeout) = 0;
    virtual UsbTransferResult ControlTransfer(uint8_t requestType, uint8_t request,
                                           uint16_t value, uint16_t index,
                                           std::vector<uint8_t>& data, uint32_t timeout) = 0;

    virtual bool IsInitialized() const = 0;
    virtual uint32_t GetTotalBlocks() const = 0;
    virtual uint32_t GetBlockSize() const = 0;
    virtual UsbDeviceInfo GetDeviceInfo() const = 0;
    virtual std::vector<UsbEndpointInfo> GetBulkEndpoints() const = 0;

    virtual std::string GetLastErrorMessage() const = 0;
};

} // namespace USB
} // namespace OHOS

#endif // USB_MASS_STORAGE_INTERFACE_H