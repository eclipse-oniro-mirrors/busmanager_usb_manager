/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef USB_DEVICE_H
#define USB_DEVICE_H
#include <iostream>
#include <singleton.h>
#include <sstream>
#include <vector>
#include <map>
#include "parcel.h"
#include "string_ex.h"
#include "cJSON.h"
#include "iremote_object.h"
#include "usb_config.h"
#include "usb_interface.h"

namespace OHOS {
namespace USB {
constexpr uint32_t USB_CONFIG_MAX_NUM = 128;

enum AuthorizeStatus {
    DISABLED = 0,
    ENABLED = 1,
    NEW_ARRIVED = 2,
};

class UsbDevice : public Parcelable {
public:
    UsbDevice(std::string name, std::string manufacturerName, std::string productName, std::string version,
        uint8_t devAddr, uint8_t busNum, int32_t vendorId, int32_t productId, int32_t baseClass, int32_t subClass,
        int32_t protocol, std::vector<USBConfig> configs)
    {
        this->name_ = name;
        this->manufacturerName_ = manufacturerName;
        this->productName_ = productName;
        this->version_ = version;
        this->devAddr_ = devAddr;
        this->busNum_ = busNum;
        this->vendorId_ = vendorId;
        this->productId_ = productId;
        this->baseClass_ = baseClass;
        this->subClass_ = subClass;
        this->protocol_ = protocol;
        this->configs_ = configs;
    }
    bool Marshalling(Parcel &parcel) const override
    {
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Uint8, parcel, this->busNum_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Uint8, parcel, this->devAddr_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Int32, parcel, this->vendorId_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Int32, parcel, this->productId_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Int32, parcel, this->baseClass_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Int32, parcel, this->subClass_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Int32, parcel, this->protocol_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Uint8, parcel, this->iManufacturer_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Uint8, parcel, this->iProduct_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Uint8, parcel, this->iSerialNumber_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Uint8, parcel, this->bMaxPacketSize0_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Uint16, parcel, this->bcdUSB_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Uint16, parcel, this->bcdDevice_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(String, parcel, this->name_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(String, parcel, this->manufacturerName_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(String, parcel, this->productName_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(String, parcel, this->version_);
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(String, parcel, this->serial_);

        uint32_t configCount = this->configs_.size();
        WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Int32, parcel, configCount);
        for (uint32_t i = 0; i < configCount && i < USB_CONFIG_MAX_NUM; i++) {
            this->configs_[i].Marshalling(parcel);
        }
        return true;
    }

    static UsbDevice *Unmarshalling(Parcel &data)
    {
        UsbDevice *usbDevice = new (std::nothrow) UsbDevice;
        if (usbDevice == nullptr) {
            return nullptr;
        }

        usbDevice->busNum_ = data.ReadUint8();
        usbDevice->devAddr_ = data.ReadUint8();
        usbDevice->vendorId_ = data.ReadInt32();
        usbDevice->productId_ = data.ReadInt32();
        usbDevice->baseClass_ = data.ReadInt32();
        usbDevice->subClass_ = data.ReadInt32();
        usbDevice->protocol_ = data.ReadInt32();
        usbDevice->iManufacturer_ = data.ReadUint8();
        usbDevice->iProduct_ = data.ReadUint8();
        usbDevice->iSerialNumber_ = data.ReadUint8();
        usbDevice->bMaxPacketSize0_ = data.ReadUint8();
        usbDevice->bcdUSB_ = data.ReadUint16();
        usbDevice->bcdDevice_ = data.ReadUint16();
        usbDevice->name_ = data.ReadString();
        usbDevice->manufacturerName_ = data.ReadString();
        usbDevice->productName_ = data.ReadString();
        usbDevice->version_ = data.ReadString();
        usbDevice->serial_ = data.ReadString();

        uint32_t configCount = data.ReadUint32();
        for (uint32_t i = 0; i < configCount && i < USB_CONFIG_MAX_NUM; i++) {
            USBConfig *pConfig = USBConfig::Unmarshalling(data);
            if (pConfig == nullptr) {
                continue;
            }
            usbDevice->configs_.push_back(*pConfig);
            delete pConfig;
            pConfig = nullptr;
        }
        return usbDevice;
    }
    explicit UsbDevice(const cJSON *device)
    {
        if (device == nullptr) {
            USB_HILOGE(MODULE_USB_INNERKIT, "device pointer is nullptr");
        }
        busNum_ = GetIntValue(device, "busNum");
        devAddr_ = GetIntValue(device, "devAddress");
        serial_ = GetIntValue(device, "serial");
        name_ = GetStringValue(device, "name");
        manufacturerName_ = GetStringValue(device, "manufacturerName");
        productName_ = GetStringValue(device, "productName");
        version_ = GetStringValue(device, "version");
        vendorId_ = GetIntValue(device, "vendorId");
        productId_ = GetIntValue(device, "productId");
        baseClass_ = GetIntValue(device, "clazz");
        subClass_ = GetIntValue(device, "subClass");
        protocol_ = GetIntValue(device, "protocol");
        cJSON* configs = cJSON_GetObjectItem(device, "configs");
        for (int i = 0; i < cJSON_GetArraySize(configs); i++) {
            cJSON* jsonConfig =  cJSON_GetArrayItem(configs, i);
            if (jsonConfig == nullptr) {
                USB_HILOGE(MODULE_USB_INNERKIT, "get item nullptr");
                continue;
            }
            USBConfig config (jsonConfig);
            configs_.emplace_back(config);
        }
    }

    UsbDevice() {}
    ~UsbDevice() {}

    static int GetIntValue(const cJSON *jsonObject, const char *key)
    {
        cJSON *item = cJSON_GetObjectItem(jsonObject, key);
        if (item != nullptr && cJSON_IsNumber(item)) {
            return item->valueint;
        } else {
            USB_HILOGE(MODULE_USB_INNERKIT, "Invalid or missing %s field", key);
            return 0;
        }
    }

    static std::string GetStringValue(const cJSON *jsonObject, const char *key)
    {
        cJSON *item = cJSON_GetObjectItem(jsonObject, key);
        if (item != nullptr && cJSON_IsString(item)) {
            return item->valuestring;
        } else {
            USB_HILOGE(MODULE_USB_INNERKIT, "Invalid or missing %s field", key);
            return "";
        }
    }

    const std::string &GetName() const
    {
        return name_;
    }

    const std::string &GetManufacturerName() const
    {
        return manufacturerName_;
    }

    const std::string &GetProductName() const
    {
        return productName_;
    }

    const std::string &GetVersion() const
    {
        return version_;
    }

    int32_t GetVendorId() const
    {
        return vendorId_;
    }

    int32_t GetProductId() const
    {
        return productId_;
    }

    int32_t GetClass() const
    {
        return baseClass_;
    }

    int32_t GetSubclass() const
    {
        return subClass_;
    }

    int32_t GetProtocol() const
    {
        return protocol_;
    }

    int32_t GetConfigCount() const
    {
        return configs_.size();
    }

    int32_t GetConfig(uint32_t index, USBConfig &config) const
    {
        if (index >= configs_.size()) {
            return ERR_INVALID_VALUE;
        }
        config = configs_[index];
        return ERR_OK;
    }

    void SetConfigs(const std::vector<USBConfig> &configs)
    {
        this->configs_ = configs;
    }

    uint8_t GetDevAddr() const
    {
        return devAddr_;
    }

    uint8_t GetBusNum() const
    {
        return busNum_;
    }

    uint8_t GetDescConfigCount()
    {
        return descConfigCount_;
    }

    void SetDevAddr(uint8_t addr)
    {
        devAddr_ = addr;
    }

    void SetBusNum(uint8_t num)
    {
        busNum_ = num;
    }

    void SetName(const std::string &name)
    {
        name_ = name;
    }

    void SetManufacturerName(const std::string &manufacturerName)
    {
        manufacturerName_ = manufacturerName;
    }

    void SetProductName(const std::string &productName)
    {
        productName_ = productName;
    }

    void SetVersion(const std::string &version)
    {
        version_ = version;
    }

    void SetVendorId(int32_t vendorId)
    {
        vendorId_ = vendorId;
    }

    void SetProductId(int32_t productId)
    {
        productId_ = productId;
    }

    void SetClass(int32_t deviceClass)
    {
        baseClass_ = deviceClass;
    }

    void SetSubclass(int32_t subClass)
    {
        subClass_ = subClass;
    }

    void SetProtocol(int32_t protocol)
    {
        protocol_ = protocol;
    }

    void SetDescConfigCount(uint8_t count)
    {
        descConfigCount_ = count;
    }

    std::vector<USBConfig> &GetConfigs()
    {
        return configs_;
    }

    std::string ToString() const
    {
        std::ostringstream ss;
        ss << "name_=" << name_ << ","
           << "manufacturerName_=" << manufacturerName_ << ","
           << "productName_=" << productName_ << ","
           << "version_=" << version_ << ","
           << "serial_=" << serial_ << ","
           << "busNum_=" << (int32_t)busNum_ << ","
           << "devAddr_=" << (int32_t)devAddr_ << ","
           << "vendorId_=" << vendorId_ << ","
           << "productId_=" << productId_ << ","
           << "baseClass_=" << baseClass_ << ","
           << "subClass_=" << subClass_ << ","
           << "protocol_=" << protocol_ << "";
        std::string str = "UsbDevice[" + ss.str() + "];    ";
        ss.str("");
        std::string strConfigs;
        for (size_t i = 0; i < configs_.size(); ++i) {
            const USBConfig &config = configs_[i];
            strConfigs += config.ToString();
        }
        str += strConfigs;
        return str;
    }

    void SetiManufacturer(uint8_t manufacturer)
    {
        this->iManufacturer_ = manufacturer;
    }

    uint8_t GetiManufacturer()
    {
        return this->iManufacturer_;
    }

    void SetiProduct(uint8_t product)
    {
        this->iProduct_ = product;
    }

    uint8_t GetiProduct()
    {
        return this->iProduct_;
    }

    void SetiSerialNumber(uint8_t sn)
    {
        this->iSerialNumber_ = sn;
    }

    uint8_t GetiSerialNumber()
    {
        return this->iSerialNumber_;
    }

    void SetmSerial(std::string serial)
    {
        this->serial_ = serial;
    }

    const std::string GetmSerial() const
    {
        return this->serial_;
    }

    void SetbMaxPacketSize0(uint8_t maxSize)
    {
        this->bMaxPacketSize0_ = maxSize;
    }

    uint8_t GetbMaxPacketSize0()
    {
        return this->bMaxPacketSize0_;
    }

    void SetbcdUSB(uint16_t bcdUSB)
    {
        this->bcdUSB_ = bcdUSB;
    }

    uint16_t GetbcdUSB()
    {
        return this->bcdUSB_;
    }

    void SetbcdDevice(uint16_t bcdDevice)
    {
        this->bcdDevice_ = bcdDevice;
    }

    uint16_t GetbcdDevice()
    {
        return this->bcdDevice_;
    }

    AuthorizeStatus GetAuthorizeStatus()
    {
        return this->authorizeStatus_;
    }

    void SetAuthorizeStatus(AuthorizeStatus authorized)
    {
        this->authorizeStatus_ = authorized;
    }

    const std::string getJsonString() const
    {
        cJSON* device = cJSON_CreateObject();
        if (!device) {
            USB_HILOGE(MODULE_USB_INNERKIT, "Create device error");
            return "";
        }
        cJSON_AddNumberToObject(device, "busNum", static_cast<double>(busNum_));
        cJSON_AddNumberToObject(device, "devAddress", static_cast<double>(devAddr_));
        cJSON_AddStringToObject(device, "serial", "");
        cJSON_AddStringToObject(device, "name", name_.c_str());
        cJSON_AddStringToObject(device, "manufacturerName", manufacturerName_.c_str());
        cJSON_AddStringToObject(device, "productName", productName_.c_str());
        cJSON_AddStringToObject(device, "version", version_.c_str());
        cJSON_AddNumberToObject(device, "vendorId", static_cast<double>(vendorId_));
        cJSON_AddNumberToObject(device, "productId", static_cast<double>(productId_));
        cJSON_AddNumberToObject(device, "clazz", static_cast<double>(baseClass_));
        cJSON_AddNumberToObject(device, "subClass", static_cast<double>(subClass_));
        cJSON_AddNumberToObject(device, "protocol", static_cast<double>(protocol_));
        cJSON* configs = cJSON_CreateArray();
        if (!configs) {
            USB_HILOGE(MODULE_USB_INNERKIT, "Create configs error");
            cJSON_Delete(device);
            return "";
        }
        for (auto &cfg : configs_) {
            cJSON* pConfig =  cJSON_Parse(cfg.getJsonString().c_str());
            cJSON_AddItemToArray(configs, pConfig);
        }
        cJSON_AddItemToObject(device, "configs", configs);
        char *pDevice = cJSON_PrintUnformatted(device);
        cJSON_Delete(device);
        if (!pDevice) {
            USB_HILOGE(MODULE_USB_INNERKIT, "Print device error");
            return "";
        }
        std::string deviceStr(pDevice);
        cJSON_free(pDevice);
        return deviceStr;
    }

private:
    std::string name_;
    std::string manufacturerName_;
    std::string productName_;
    std::string version_;
    std::string serial_;
    AuthorizeStatus authorizeStatus_ = ENABLED;
    uint8_t devAddr_ = UINT8_MAX;
    uint8_t busNum_ = UINT8_MAX;
    uint8_t descConfigCount_ = UINT8_MAX;

    uint8_t bMaxPacketSize0_ = UINT8_MAX;
    uint16_t bcdUSB_ = UINT16_MAX;
    uint16_t bcdDevice_ = UINT16_MAX;
    uint8_t iManufacturer_ = UINT8_MAX;
    uint8_t iProduct_ = UINT8_MAX;
    uint8_t iSerialNumber_ = UINT8_MAX;

    int32_t vendorId_ = INVALID_USB_INT_VALUE;
    int32_t productId_ = INVALID_USB_INT_VALUE;
    int32_t baseClass_ = INVALID_USB_INT_VALUE;
    int32_t subClass_ = INVALID_USB_INT_VALUE;
    int32_t protocol_ = INVALID_USB_INT_VALUE;
    std::vector<USBConfig> configs_;
};
} // namespace USB
} // namespace OHOS

#endif // USB_DEVICE_H
