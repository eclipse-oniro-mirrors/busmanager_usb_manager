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

#include "usb_mass_storage_interface.h"
#include "usb_srv_client.h"
#include "hilog_wrapper.h"
#include "usb_common_test.h"
#include <cstring>

namespace OHOS {
namespace USB {

constexpr uint32_t MASS_STORAGE_CLASS = 0x08;
constexpr uint32_t MASS_STORAGE_SCSI_SUBCLASS = 0x06;
constexpr uint32_t MASS_STORAGE_BULK_ONLY_PROTOCOL = 0x50;
constexpr uint32_t CBW_SIGNATURE = 0x43425355;
constexpr uint32_t CSW_SIGNATURE = 0x53425355;
constexpr uint32_t DEFAULT_TIMEOUT_MS = 5000;
constexpr uint32_t MAX_LBA_VALUE = 0xFFFFFFFF;

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

class UsbMassStorageImpl : public IUsbMassStorageInterface {
public:
    UsbMassStorageImpl();
    ~UsbMassStorageImpl() override;

    UsbTransferResult Initialize(uint8_t busNum, uint8_t devAddr) override;
    UsbTransferResult Cleanup() override;
    UsbTransferResult CheckDeviceReady(uint32_t maxRetries) override;
    UsbTransferResult GetDeviceInfo(UsbDeviceInfo& info) override;

    UsbTransferResult SendInquiry(ScsiInquiryData& inquiryData) override;
    UsbTransferResult SendTestUnitReady() override;
    UsbTransferResult SendReadCapacity(ScsiReadCapacityData& capacityData) override;
    UsbTransferResult RequestSense(ScsiSenseData& senseData) override;
    UsbTransferResult Read(uint32_t lba, uint32_t blocks, std::vector<uint8_t>& data) override;
    UsbTransferResult Write(uint32_t lba, const std::vector<uint8_t>& data) override;
    UsbTransferResult ModeSense(uint8_t pageCode, std::vector<uint8_t>& modeData) override;
    UsbTransferResult SynchronizeCache() override;

    UsbTransferResult BulkRead(std::vector<uint8_t>& data, uint32_t timeout) override;
    UsbTransferResult BulkWrite(const std::vector<uint8_t>& data, uint32_t timeout) override;
    UsbTransferResult ControlTransfer(uint8_t requestType, uint8_t request,
                                           uint16_t value, uint16_t index,
                                           std::vector<uint8_t>& data, uint32_t timeout) override;

    bool IsInitialized() const override { return initialized_; }
    uint32_t GetTotalBlocks() const override { return totalBlocks_; }
    uint32_t GetBlockSize() const override { return blockSize_; }
    UsbDeviceInfo GetDeviceInfo() const override { return deviceInfo_; }
    std::vector<UsbEndpointInfo> GetBulkEndpoints() const override { return bulkEndpoints_; }
    std::string GetLastErrorMessage() const override { return lastError_; }

private:
    UsbTransferResult FindMassStorageDevice(uint8_t busNum, uint8_t devAddr);
    UsbTransferResult InitializeEndpoints();
    UsbTransferResult ClaimMassStorageInterface();

    UsbTransferResult SendCbw(const ScsiCommandBlockWrapper& cbw);
    UsbTransferResult ReceiveCsw(ScsiCommandStatusWrapper& csw);

    void BuildScsiCommandBlock(ScsiCommandBlock& cb, uint8_t opcode,
                                uint32_t lba, uint32_t transferLength);
    std::string GetErrorMessage(int32_t errorCode) const;
    bool ValidateEndpointDirection(const USBEndpoint& endpoint, uint32_t expectedDirection);

    UsbSrvClient& GetUsbClient();
    
    std::unique_ptr<UsbSrvClient> usbClient_;
    USBDevicePipe devicePipe_;
    UsbDevice usbDevice_;
    UsbInterface massStorageInterface_;
    USBEndpoint bulkInEndpoint_;
    USBEndpoint bulkOutEndpoint_;

    UsbDeviceInfo deviceInfo_;
    std::vector<UsbEndpointInfo> bulkEndpoints_;

    bool initialized_;
    bool interfaceClaimed_;
    uint32_t tagCounter_;

    std::string lastError_;
};

UsbMassStorageImpl::UsbMassStorageImpl()
    : usbClient_(nullptr),
      initialized_(false),
      interfaceClaimed_(false),
      tagCounter_(1)
{
    usbClient_ = std::make_unique<UsbSrvClient>(UsbSrvClient::GetInstance());
}

UsbMassStorageImpl::~UsbMassStorageImpl()
{
    if (initialized_) {
        Cleanup();
    }
}

UsbSrvClient& UsbMassStorageImpl::GetUsbClient()
{
    return UsbSrvClient::GetInstance();
}

UsbTransferResult UsbMassStorageImpl::Initialize(uint8_t busNum, uint8_t devAddr)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbMassStorageImpl::Initialize busNum=%{public}u devAddr=%{public}u",
                busNum, devAddr);

    if (initialized_) {
        USB_HILOGW(MODULE_USB_SERVICE, "Already initialized");
        return USB_OK;
    }

    lastError_.clear();

    UsbTransferResult ret = FindMassStorageDevice(busNum, devAddr);
    if (ret != USB_OK) {
        lastError_ = GetErrorMessage(ret);
        return ret;
    }

    ret = InitializeEndpoints();
    if (ret != USB_OK) {
        lastError_ = GetErrorMessage(ret);
        Cleanup();
        return ret;
    }

    ret = ClaimMassStorageInterface();
    if (ret != USB_OK) {
        lastError_ = GetErrorMessage(ret);
        Cleanup();
        return ret;
    }

    initialized_ = true;
    USB_HILOGI(MODULE_USB_SERVICE, "Initialized successfully, blocks=%{public}u blockSize=%{public}u",
                totalBlocks_, blockSize_);

    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::Cleanup()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbMassStorageImpl::Cleanup enter");

    if (!initialized_) {
        return USB_OK;
    }

    bool releaseRet = false;
    bool closeRet = false;

    if (interfaceClaimed_) {
        releaseRet = GetUsbClient().ReleaseInterface(devicePipe_, massStorageInterface_);
        interfaceClaimed_ = false;
    }

    if (releaseRet) {
        closeRet = GetUsbClient().Close(devicePipe_);
    }

    initialized_ = false;
    interfaceClaimed_ = false;
    totalBlocks_ = 0;
    blockSize_ = 0;
    bulkEndpoints_.clear();

    USB_HILOGI(MODULE_USB_SERVICE, "Cleanup completed");
    return releaseRet && closeRet ? USB_OK : USB_SYNC_ERROR;
}

UsbTransferResult UsbMassStorageImpl::FindMassStorageDevice(uint8_t busNum, uint8_t devAddr)
{
    USB_HILOGI(MODULE_USB_SERVICE, "FindMassStorageDevice busNum=%{public}u devAddr=%{public}u",
                busNum, devAddr);

    std::vector<UsbDevice> deviceList;
    int32_t ret = GetUsbClient().GetDevices(deviceList);
    if (ret != 0) {
        lastError_ = "GetDevices failed: " + GetErrorMessage(ret);
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s", lastError_.c_str());
        return USB_DEVICE_NOT_FOUND;
    }

    for (const auto& device : deviceList) {
        if (device.GetBusNum() == busNum &&
            device.GetDevAddr() == devAddr &&
            device.GetClass() == MASS_STORAGE_CLASS &&
            device.GetSubClass() == MASS_STORAGE_SCSI_SUBCLASS) {
            usbDevice_ = device;

            deviceInfo_.busNum = busNum;
            deviceInfo_.devAddr = devAddr;
            deviceInfo_.vendorId = device.GetVendorId();
            deviceInfo_.productId = device.GetProductId();
            deviceInfo_.baseClass = device.GetClass();
            deviceInfo_.subClass = device.GetSubClass();
            deviceInfo_.protocol = device.GetProtocol();
            deviceInfo_.name = device.GetName();
            deviceInfo_.manufacturerName = device.GetManufacturerName();
            deviceInfo_.productName = device.GetProductName();

            USB_HILOGI(MODULE_USB_SERVICE, "Device found: VID=0x%{public}X PID=0x%{public}X",
                        deviceInfo_.vendorId, deviceInfo_.productId);
            return USB_OK;
        }
    }

    lastError_ = "No matching mass storage device found";
    USB_HILOGE(MODULE_USB_SERVICE, "%{public}s", lastError_.c_str());
    return USB_DEVICE_NOT_FOUND;
}

UsbTransferResult UsbMassStorageImpl::InitializeEndpoints()
{
    USB_HILOGI(MODULE_USB_SERVICE, "InitializeEndpoints enter");

    USBConfig configDesc;
    if (usbDevice_.GetConfig(0, configDesc) != 0) {
        lastError_ = "GetConfig failed";
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s", lastError_.c_str());
        return USB_INTERFACE_ERROR;
    }

    const std::vector<UsbInterface>& interfaces = configDesc.GetInterfaces();
    bool bulkInFound = false;
    bool bulkOutFound = false;

    for (const auto& interface : interfaces) {
        if (interface.GetClass() == MASS_STORAGE_CLASS) {
            massStorageInterface_ = interface;

            int32_t ret = GetUsbClient().ClaimInterface(devicePipe_, interface, true);
            if (ret != 0) {
                lastError_ = "ClaimInterface failed: " + GetErrorMessage(ret);
                USB_HILOGE(MODULE_USB_SERVICE, "%{public}s", lastError_.c_str());
                return USB_INTERFACE_ERROR;
            }

            interfaceClaimed_ = true;

            for (const auto& endpoint : interface.GetEndpoints()) {
                if (endpoint.GetType() == USB_ENDPOINT_XFER_BULK) {
                    UsbEndpointInfo epInfo;
                    epInfo.interfaceId = endpoint.GetInterfaceId();
                    epInfo.endpointAddress = endpoint.GetAddress();
                    epInfo.maxPacketSize = endpoint.GetMaxPacketSize();
                    epInfo.transferType = endpoint.GetType();
                    epInfo.direction = endpoint.GetDirection();

                    if (endpoint.GetDirection() == USB_ENDPOINT_DIR_IN) {
                        bulkInEndpoint_ = endpoint;
                        bulkInFound = true;
                        USB_HILOGI(MODULE_USB_SERVICE, "Found Bulk IN endpoint: address=0x%{public}X max=%{public}u",
                                    epInfo.endpointAddress, epInfo.maxPacketSize);
                    } else if (endpoint.GetDirection() == USB_ENDPOINT_DIR_OUT) {
                        bulkOutEndpoint_ = endpoint;
                        bulkOutFound = true;
                        USB_HILOGI(MODULE_USB_SERVICE, "Found Bulk OUT endpoint: address=0x%{public}X max=%{public}u",
                                    epInfo.endpointAddress, epInfo.maxPacketSize);
                    }

                    bulkEndpoints_.push_back(epInfo);
                }
            }

            break;
        }
    }

    if (!bulkInFound || !bulkOutFound) {
        lastError_ = "Bulk endpoints not found";
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s", lastError_.c_str());
        return USB_ENDPOINT_ERROR;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "Endpoints initialized successfully");
    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::ClaimMassStorageInterface()
{
    USB_HILOGI(MODULE_USB_SERVICE, "ClaimMassStorageInterface enter");
    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::CheckDeviceReady(uint32_t maxRetries)
{
    USB_HILOGI(MODULE_USB_SERVICE, "CheckDeviceReady maxRetries=%{public}u", maxRetries);

    for (uint32_t i = 0; i < maxRetries; i++) {
        auto ret = SendTestUnitReady();
        if (ret == USB_OK) {
            USB_HILOGI(MODULE_USB_SERVICE, "Device ready at attempt %{public}u", i + 1);
            return USB_OK;
        }

        ScsiSenseData senseData;
        RequestSense(senseData);

        USB_HILOGW(MODULE_USB_SERVICE, "Attempt %{public}u failed, retrying...", i + 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    lastError_ = "Device not ready after " + std::to_string(maxRetries) + " retries";
    USB_HILOGE(MODULE_USB_SERVICE, "%{public}s", lastError_.c_str());
    return USB_TIMEOUT;
}

UsbTransferResult UsbMassStorageImpl::GetDeviceInfo(UsbDeviceInfo& info)
{
    if (!initialized_) {
        return USB_INVALID_PARAM;
    }
    info = deviceInfo_;
    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::SendInquiry(ScsiInquiryData& inquiryData)
{
    USB_HILOGI(MODULE_USB_SERVICE, "SendInquiry enter");

    if (!initialized_) {
        lastError_ = "Device not initialized";
        return USB_INVALID_PARAM;
    }

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_INQUIRY, 0, 0);

    ScsiCommandBlockWrapper cbw;
    memset(&cbw, 0, sizeof(ScsiCommandBlockWrapper));
    cbw.signature = CBW_SIGNATURE;
    cbw.tag = tagCounter_++;
    cbw.dataTransferLength = sizeof(ScsiInquiryData);
    cbw.flags = 0x80;
    cbw.lun = 0;
    cbw.cdbLength = sizeof(ScsiCommandBlock);
    cbw.cbw = cb;

    auto ret = SendCbw(cbw);
    if (ret != USB_OK) {
        lastError_ = "SendCbw failed: " + GetErrorMessage(ret);
        return ret;
    }

    std::vector<uint8_t> inquiryBuffer(sizeof(ScsiInquiryData));
    ret = BulkRead(inquiryBuffer, DEFAULT_TIMEOUT_MS);
    if (ret != USB_OK) {
        lastError_ = "BulkRead failed: " + GetErrorMessage(ret);
        return ret;
    }

    memcpy(&inquiryData, inquiryBuffer.data(), sizeof(ScsiInquiryData));

    ScsiCommandStatusWrapper csw;
    ret = ReceiveCsw(csw);
    if (ret != USB_OK) {
        lastError_ = "ReceiveCsw failed: " + GetErrorMessage(ret);
        return ret;
    }

    if (csw.status != 0) {
        lastError_ = "CSW status error: " + std::to_string(csw.status);
        return USB_TRANSFER_FAILED;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "SendInquiry completed successfully");
    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::SendTestUnitReady()
{
    USB_HILOGI(MODULE_USB_SERVICE, "SendTestUnitReady enter");

    if (!initialized_) {
        lastError_ = "Device not initialized";
        return USB_INVALID_PARAM;
    }

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_TEST_UNIT_READY, 0, 0);

    ScsiCommandBlockWrapper cbw;
    memset(&cbw, 0, sizeof(ScsiCommandBlockWrapper));
    cbw.signature = CBW_SIGNATURE;
    cbw.tag = tagCounter_++;
    cbw.dataTransferLength = 0;
    cbw.flags = 0;
    cbw.lun = 0;
    cbw.cdbLength = sizeof(ScsiCommandBlock);
    cbw.cbw = cb;

    auto ret = SendCbw(cbw);
    if (ret != USB_OK) {
        lastError_ = "SendCbw failed: " + GetErrorMessage(ret);
        return ret;
    }

    ScsiCommandStatusWrapper csw;
    ret = ReceiveCsw(csw);
    if (ret != USB_OK) {
        lastError_ = "ReceiveCsw failed: " + GetErrorMessage(ret);
        return ret;
    }

    if (csw.status != 0) {
        lastError_ = "CSW status error: " + std::to_string(csw.status);
        return USB_TRANSFER_FAILED;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "SendTestUnitReady completed");
    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::SendReadCapacity(ScsiReadCapacityData& capacityData)
{
    USB_HILOGI(MODULE_USB_SERVICE, "SendReadCapacity enter");

    if (!initialized_) {
        lastError_ = "Device not initialized";
        return USB_INVALID_PARAM;
    }

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_READ_CAPACITY_10, 0, 0);

    ScsiCommandBlockWrapper cbw;
    memset(&cbw, 0, sizeof(ScsiCommandBlockWrapper));
    cbw.signature = CBW_SIGNATURE;
    cbw.tag = tagCounter_++;
    cbw.dataTransferLength = sizeof(ScsiReadCapacityData);
    cbw.flags = 0x80;
    cbw.lun = 0;
    cbw.cdbLength = sizeof(ScsiCommandBlock);
    cbw.cbw = cb;

    auto ret = SendCbw(cbw);
    if (ret != USB_OK) {
        lastError_ = "SendCbw failed: " + GetErrorMessage(ret);
        return ret;
    }

    std::vector<uint8_t> capacityBuffer(sizeof(ScsiReadCapacityData));
    ret = BulkRead(capacityBuffer, DEFAULT_TIMEOUT_MS);
    if (ret != USB_OK) {
        lastError_ = "BulkRead failed: " + GetErrorMessage(ret);
        return ret;
    }

    memcpy(&capacityData, capacityBuffer.data(), sizeof(ScsiReadCapacityData));
    capacityData.lastLogicalBlockAddress = ntohl(capacityData.lastLogicalBlockAddress);
    capacityData.blockLengthInBytes = ntohl(capacityData.blockLengthInBytes);

    totalBlocks_ = capacityData.lastLogicalBlockAddress + 1;
    blockSize_ = capacityData.blockLengthInBytes;

    ScsiCommandStatusWrapper csw;
    ret = ReceiveCsw(csw);
    if (ret != USB_OK) {
        lastError_ = "ReceiveCsw failed: " + GetErrorMessage(ret);
        return ret;
    }

    if (csw.status != 0) {
        lastError_ = "CSW status error: " + std::to_string(csw.status);
        return USB_TRANSFER_FAILED;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "ReadCapacity: totalBlocks=%{public}u blockSize=%{public}u",
                totalBlocks_, blockSize_);

    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::RequestSense(ScsiSenseData& senseData)
{
    USB_HILOGI(MODULE_USB_SERVICE, "RequestSense enter");

    if (!initialized_) {
        lastError_ = "Device not initialized";
        return USB_INVALID_PARAM;
    }

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_REQUEST_SENSE, 0, 0);

    ScsiCommandBlockWrapper cbw;
    memset(&cbw, 0, sizeof(ScsiCommandBlockWrapper));
    cbw.signature = CBW_SIGNATURE;
    cbw.tag = tagCounter_++;
    cbw.dataTransferLength = sizeof(ScsiSenseData);
    cbw.flags = 0x80;
    cbw.lun = 0;
    cbw.cdbLength = sizeof(ScsiCommandBlock);
    cbw.cbw = cb;

    auto ret = SendCbw(cbw);
    if (ret != USB_OK) {
        lastError_ = "SendCbw failed: " + GetErrorMessage(ret);
        return ret;
    }

    std::vector<uint8_t> senseBuffer(sizeof(ScsiSenseData));
    ret = BulkRead(senseBuffer, DEFAULT_TIMEOUT_MS);
    if (ret != USB_OK) {
        lastError_ = "BulkRead failed: " + GetErrorMessage(ret);
        return ret;
    }

    memcpy(&senseData, senseBuffer.data(), sizeof(ScsiSenseData));

    ScsiCommandStatusWrapper csw;
    ret = ReceiveCsw(csw);
    if (ret != USB_OK) {
        lastError_ = "ReceiveCsw failed: " + GetErrorMessage(ret);
        return ret;
    }

    if (csw.status != 0) {
        lastError_ = "CSW status error: " + std::to_string(csw.status);
        return USB_TRANSFER_FAILED;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "RequestSense completed");
    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::Read(uint32_t lba, uint32_t blocks, std::vector<uint8_t>& data)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Read lba=%{public}u blocks=%{public}u", lba, blocks);

    if (!initialized_) {
        lastError_ = "Device not initialized";
        return USB_INVALID_PARAM;
    }

    if (blockSize_ == 0) {
        lastError_ = "Block size not initialized";
        return USB_INVALID_PARAM;
    }

    uint32_t transferLength = blocks * blockSize_;
    if (transferLength == 0) {
        lastError_ = "Invalid transfer length";
        return USB_INVALID_PARAM;
    }

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_READ_10, lba, blocks);

    ScsiCommandBlockWrapper cbw;
    memset(&cbw, 0, sizeof(ScsiCommandBlockWrapper));
    cbw.signature = CBW_SIGNATURE;
    cbw.tag = tagCounter_++;
    cbw.dataTransferLength = transferLength;
    cbw.flags = 0x80;
    cbw.lun = 0;
    cbw.cdbLength = sizeof(ScsiCommandBlock);
    cbw.cbw = cb;

    auto ret = SendCbw(cbw);
    if (ret != USB_OK) {
        lastError_ = "SendCbw failed: " + GetErrorMessage(ret);
        return ret;
    }

    data.resize(transferLength);
    ret = BulkRead(data, DEFAULT_TIMEOUT_MS);
    if (ret != USB_OK) {
        lastError_ = "BulkRead failed: " + GetErrorMessage(ret);
        return ret;
    }

    ScsiCommandStatusWrapper csw;
    ret = ReceiveCsw(csw);
    if (ret != USB_OK) {
        lastError_ = "ReceiveCsw failed: " + GetErrorMessage(ret);
        return ret;
    }

    if (csw.status != 0) {
        lastError_ = "CSW status error: " + std::to_string(csw.status);
        return USB_TRANSFER_FAILED;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "Read completed successfully, size=%{public}zu", data.size());
    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::Write(uint32_t lba, const std::vector<uint8_t>& data)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Write lba=%{public}u size=%{public}zu", lba, data.size());

    if (!initialized_) {
        lastError_ = "Device not initialized";
        return USB_INVALID_PARAM;
    }

    if (blockSize_ == 0) {
        lastError_ = "Block size not initialized";
        return USB_INVALID_PARAM;
    }

    if (data.size() % blockSize_ != 0) {
        lastError_ = "Data size must be multiple of block size";
        return USB_INVALID_PARAM;
    }

    uint32_t blocks = data.size() / blockSize_;
    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_WRITE_10, lba, blocks);

    uint32_t transferLength = data.size();
    ScsiCommandBlockWrapper cbw;
    memset(&cbw, 0, sizeof(ScsiCommandBlockWrapper));
    cbw.signature = CBW_SIGNATURE;
    cbw.tag = tagCounter_++;
    cbw.dataTransferLength = transferLength;
    cbw.flags = 0;
    cbw.lun = 0;
    cbw.cdbLength = sizeof(ScsiCommandBlock);
    cbw.cbw = cb;

    auto ret = SendCbw(cbw);
    if (ret != USB_OK) {
        lastError_ = "SendCbw failed: " + GetErrorMessage(ret);
        return ret;
    }

    ret = BulkWrite(data, DEFAULT_TIMEOUT_MS);
    if (ret != USB_OK) {
        lastError_ = "BulkWrite failed: " + GetErrorMessage(ret);
        return ret;
    }

    ScsiCommandStatusWrapper csw;
    ret = ReceiveCsw(csw);
    if (ret != USB_OK) {
        lastError_ = "ReceiveCsw failed: " + GetErrorMessage(ret);
        return ret;
    }

    if (csw.status != 0) {
        lastError_ = "CSW status error: " + std::to_string(csw.status);
        return USB_TRANSFER_FAILED;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "Write completed successfully");
    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::ModeSense(uint8_t pageCode, std::vector<uint8_t>& modeData)
{
    USB_HILOGI(MODULE_USB_SERVICE, "ModeSense pageCode=0x%{public}X", pageCode);

    if (!initialized_) {
        lastError_ = "Device not initialized";
        return USB_INVALID_PARAM;
    }

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_MODE_SENSE_6, 0, 64);

    cb.blockLengthMsb = 0;
    cb.blockLengthLsb = 64;
    cb.dataLength[0] = 0;
    cb.dataLength[1] = pageCode;
    cb.dataLength[2] = 0;
    cb.dataLength[3] = 0;

    ScsiCommandBlockWrapper cbw;
    memset(&cbw, 0, sizeof(ScsiCommandBlockWrapper));
    cbw.signature = CBW_SIGNATURE;
    cbw.tag = tagCounter_++;
    cbw.dataTransferLength = 64;
    cbw.flags = 0x80;
    cbw.lun = 0;
    cbw.cdbLength = sizeof(ScsiCommandBlock);
    cbw.cbw = cb;

    auto ret = SendCbw(cbw);
    if (ret != USB_OK) {
        lastError_ = "SendCbw failed: " + GetErrorMessage(ret);
        return ret;
    }

    modeData.resize(64);
    ret = BulkRead(modeData, DEFAULT_TIMEOUT_MS);
    if (ret != USB_OK) {
        lastError_ = "BulkRead failed: " + GetErrorMessage(ret);
        return ret;
    }

    ScsiCommandStatusWrapper csw;
    ret = ReceiveCsw(csw);
    if (ret != USB_OK) {
        lastError_ = "ReceiveCsw failed: " + GetErrorMessage(ret);
        return ret;
    }

    if (csw.status != 0) {
        lastError_ = "CSW status error: " + std::to_string(csw.status);
        return USB_TRANSFER_FAILED;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "ModeSense completed");
    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::SynchronizeCache()
{
    USB_HILOGI(MODULE_USB_SERVICE, "SynchronizeCache enter");

    if (!initialized_) {
        lastError_ = "Device not initialized";
        return USB_INVALID_PARAM;
    }

    ScsiCommandBlock cb;
    BuildScsiCommandBlock(cb, SCSI_SYNCHRONIZE_CACHE_10, 0, 0);

    ScsiCommandBlockWrapper cbw;
    memset(&cbw, 0, sizeof(ScsiCommandBlockWrapper));
    cbw.signature = CBW_SIGNATURE;
    cbw.tag = tagCounter_++;
    cbw.dataTransferLength = 0;
    cbw.flags = 0;
    cbw.lun = 0;
    cbw.cdbLength = sizeof(ScsiCommandBlock);
    cbw.cbw = cb;

    auto ret = SendCbw(cbw);
    if (ret != USB_OK) {
        lastError_ = "SendCbw failed: " + GetErrorMessage(ret);
        return ret;
    }

    ScsiCommandStatusWrapper csw;
    ret = ReceiveCsw(csw);
    if (ret != USB_OK) {
        lastError_ = "ReceiveCsw failed: " + GetErrorMessage(ret);
        return ret;
    }

    if (csw.status != 0) {
        lastError_ = "CSW status error: " + std::to_string(csw.status);
        return USB_TRANSFER_FAILED;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "SynchronizeCache completed");
    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::BulkRead(std::vector<uint8_t>& data, uint32_t timeout)
{
    if (!initialized_) {
        lastError_ = "Device not initialized";
        return USB_INVALID_PARAM;
    }

    if (data.empty()) {
        data.resize(1);
    }

    auto ret = GetUsbClient().BulkTransfer(devicePipe_, bulkInEndpoint_, data, timeout);
    if (ret != 0) {
        lastError_ = "BulkTransfer read failed: " + GetErrorMessage(ret);
        return USB_TRANSFER_FAILED;
    }

    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::BulkWrite(const std::vector<uint8_t>& data, uint32_t timeout)
{
    if (!initialized_) {
        lastError_ = "Device not initialized";
        return USB_INVALID_PARAM;
    }

    if (data.empty()) {
        lastError_ = "No data to write";
        return USB_INVALID_PARAM;
    }

    std::vector<uint8_t> writeData = data;
    auto ret = GetUsbClient().BulkTransfer(devicePipe_, bulkOutEndpoint_, writeData, timeout);
    if (ret != 0) {
        lastError_ = "BulkTransfer write failed: " + GetErrorMessage(ret);
        return USB_TRANSFER_FAILED;
    }

    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::ControlTransfer(uint8_t requestType, uint8_t request,
                                             uint16_t value, uint16_t index,
                                             std::vector<uint8_t>& data, uint32_t timeout)
{
    if (!initialized_) {
        lastError_ = "Device not initialized";
        return USB_INVALID_PARAM;
    }

    HDI::Usb::V1_0::UsbCtrlTransfer ctrl = {requestType, request, value, index, timeout, 0};

    auto ret = GetUsbClient().ControlTransfer(devicePipe_, ctrl, data);
    if (ret != 0) {
        lastError_ = "ControlTransfer failed: " + GetErrorMessage(ret);
        return USB_TRANSFER_FAILED;
    }

    return USB_OK;
}

UsbTransferResult UsbMassStorageImpl::SendCbw(const ScsiCommandBlockWrapper& cbw)
{
    std::vector<uint8_t> cbwBuffer(sizeof(ScsiCommandBlockWrapper));
    memcpy(cbwBuffer.data(), &cbw, sizeof(ScsiCommandBlockWrapper));

    return BulkWrite(cbwBuffer, DEFAULT_TIMEOUT_MS);
}

UsbTransferResult UsbMassStorageImpl::ReceiveCsw(ScsiCommandStatusWrapper& csw)
{
    std::vector<uint8_t> cswBuffer(sizeof(ScsiCommandStatusWrapper));
    auto ret = BulkRead(cswBuffer, DEFAULT_TIMEOUT_MS);
    if (ret != USB_OK) {
        return ret;
    }

    memcpy(&csw, cswBuffer.data(), sizeof(ScsiCommandStatusWrapper));

    if (csw.signature != CSW_SIGNATURE) {
        lastError_ = "Invalid CSW signature: 0x" + std::to_string(csw.signature);
        return USB_SYNC_ERROR;
    }

    if (csw.status != 0) {
        USB_HILOGW(MODULE_USB_SERVICE, "CSW status=%{public}u residue=%{public}u",
                    csw.status, csw.dataResidue);
    }

    return USB_OK;
}

void UsbMassStorageImpl::BuildScsiCommandBlock(ScsiCommandBlock& cb, uint8_t opcode,
                                                 uint32_t lba, uint32_t transferLength)
{
    memset(&cb, 0, sizeof(ScsiCommandBlock));
    cb.opcode = opcode;

    switch (opcode) {
        case SCSI_TEST_UNIT_READY:
            break;

        case SCSI_INQUIRY:
            cb.blockLengthLsb = 36;
            break;

        case SCSI_READ_CAPACITY_10:
            cb.blockLengthMsb = (lba >> 24) & 0xFF;
            cb.dataLength[0] = (lba >> 16) & 0xFF;
            cb.dataLength[1] = (lba >> 8) & 0xFF;
            cb.dataLength[2] = lba & 0xFF;
            cb.dataLength[3] = 0;
            cb.control = 0;
            break;

        case SCSI_READ_10:
            cb.blockLengthMsb = (lba >> 24) & 0xFF;
            cb.dataLength[0] = (lba >> 16) & 0xFF;
            cb.dataLength[1] = (lba >> 8) & 0xFF;
            cb.dataLength[2] = lba & 0xFF;
            cb.dataLength[3] = 0;
            cb.blockLengthLsb = (transferLength >> 8) & 0xFF;
            cb.reserved[0] = transferLength & 0xFF;
            break;

        case SCSI_WRITE_10:
            cb.blockLengthMsb = (lba >> 24) & 0xFF;
            cb.dataLength[0] = (lba >> 16) & 0xFF;
            cb.dataLength[1] = (lba >> 8) & 0xFF;
            cb.dataLength[2] = lba & 0xFF;
            cb.dataLength[3] = 0;
            cb.blockLengthLsb = (transferLength >> 8) & 0xFF;
            cb.reserved[0] = transferLength & 0xFF;
            break;

        case SCSI_REQUEST_SENSE:
            cb.blockLengthLsb = 18;
            break;

        case SCSI_MODE_SENSE_6:
            cb.blockLengthLsb = transferLength;
            break;

        case SCSI_SYNCHRONIZE_CACHE_10:
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

std::string UsbMassStorageImpl::GetErrorMessage(int32_t errorCode) const
{
    switch (errorCode) {
        case 0: return "Success";
        case -1: return "Invalid parameter";
        case -2: return "Device not found";
        case -3: return "No permission";
        case -4: return "Device busy";
        case -5: return "Timeout";
        case -6: return "Transfer failed";
        case -7: return "Interface error";
        case -8: return "Endpoint error";
        case -9: return "Sync error";
        default: return "Unknown error: " + std::to_string(errorCode);
    }
}

bool UsbMassStorageImpl::ValidateEndpointDirection(const USBEndpoint& endpoint, uint32_t expectedDirection)
{
    return endpoint.GetDirection() == expectedDirection;
}

} // namespace USB
} // namespace OHOS