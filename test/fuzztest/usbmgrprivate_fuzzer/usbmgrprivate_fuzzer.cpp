/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "usb_service.h"
#include "usb_device.h"
#include "nlohmann/json.hpp"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "usbmgrprivate_fuzzer.h"
#include "usb_errors.h"

using namespace OHOS::HDI::Usb::Serial::V1_0;
using namespace OHOS::HDI::Usb::V1_0;
using namespace OHOS::HDI::Usb::V1_2;
namespace OHOS {
const uint32_t OFFSET = 2;
const uint32_t MAX_CONFIG_COUNT = 3;
const uint32_t MAX_INTERFACE_COUNT = 3;
const uint32_t INTERFACE_FIELD_BYTES = 5;
namespace USB {
    void GetDeviceVidPidSerialNumberFuzzTest(const uint8_t* rawData, size_t size)
    {
        if (rawData == nullptr || size < OFFSET) {
            return;
        }

        auto serviceInstance = UsbService::GetGlobalInstance();
        uint8_t busNum = *reinterpret_cast<const uint8_t *>(rawData);
        uint8_t devAddr = *reinterpret_cast<const uint8_t *>(rawData + sizeof(uint8_t));
        std::string name = std::to_string(busNum) + "-" + std::to_string(devAddr);
        std::string uniqueName = std::to_string(busNum) + "+" + std::to_string(devAddr);
        std::string deviceName = name;
        std::string strDesc;
        serviceInstance->deviceVidPidMap_.insert(std::pair<std::string, std::string>(name, uniqueName));
        if (serviceInstance->GetDeviceVidPidSerialNumber(deviceName, strDesc) != UEC_OK) {
            USB_HILOGE(MODULE_USB_SERVICE, "can not find deviceName.");
        }
    }

    void UsbDeviceTypeChangeFuzzTest(const uint8_t* rawData, size_t size)
    {
        if (rawData == nullptr || size < sizeof(UsbDeviceType) + sizeof(UsbDeviceTypeInfo)) {
            return;
        }
        auto serviceInstance = UsbService::GetGlobalInstance();
        UsbDeviceType info = *reinterpret_cast<const UsbDeviceType *>(rawData);
        std::vector<UsbDeviceType> disableType;
        std::vector<UsbDeviceTypeInfo> deviceTypes;
        disableType.push_back(info);
        serviceInstance->UsbDeviceTypeChange(disableType, deviceTypes);
    }

    void UpdateDeviceVidPidMapFuzzTest(const uint8_t* rawData, size_t size)
    {
        if (rawData == nullptr || size < sizeof(SerialPort)) {
            return;
        }
        size_t offsetInfo = sizeof(int32_t);
        auto serviceInstance = UsbService::GetGlobalInstance();

        SerialPort info;
        info.portId = *reinterpret_cast<const int32_t *>(rawData);
        info.deviceInfo.busNum = *reinterpret_cast<const unsigned char *>(rawData + offsetInfo);
        offsetInfo += sizeof(unsigned char);
        info.deviceInfo.devAddr = *reinterpret_cast<const unsigned char *>(rawData + offsetInfo);
        offsetInfo += sizeof(unsigned char);
        info.deviceInfo.vid = *reinterpret_cast<const int32_t *>(rawData + offsetInfo);
        offsetInfo += sizeof(int32_t);
        info.deviceInfo.pid = *reinterpret_cast<const int32_t *>(rawData + offsetInfo);
        info.deviceInfo.serialNum = "fuzz_test_serialNum";

        std::vector<SerialPort> serialPortList;
        serialPortList.push_back(info);
        serviceInstance->UpdateDeviceVidPidMap(serialPortList);
    }

    void SerialPortChangeFuzzTest(const uint8_t* rawData, size_t size)
    {
        if (rawData == nullptr || size < sizeof(SerialPort)) {
            return;
        }
        size_t offsetInfo = sizeof(int32_t);
        auto serviceInstance = UsbService::GetGlobalInstance();

        SerialPort info;
        info.portId = *reinterpret_cast<const int32_t *>(rawData);
        info.deviceInfo.busNum = *reinterpret_cast<const unsigned char *>(rawData + offsetInfo);
        offsetInfo += sizeof(unsigned char);
        info.deviceInfo.devAddr = *reinterpret_cast<const unsigned char *>(rawData + offsetInfo);
        offsetInfo += sizeof(unsigned char);
        info.deviceInfo.vid = *reinterpret_cast<const int32_t *>(rawData + offsetInfo);
        offsetInfo += sizeof(int32_t);
        info.deviceInfo.pid = *reinterpret_cast<const int32_t *>(rawData + offsetInfo);
        info.deviceInfo.serialNum = "fuzz_test_serialNum";

        std::vector<UsbSerialPort> serialInfoList;
        std::vector<SerialPort> serialPortList;
        serialPortList.push_back(info);
        serviceInstance->SerialPortChange(serialInfoList, serialPortList);
    }

    void FreeTokenIdFuzzTest(const uint8_t* rawData, size_t size)
    {
        if (rawData == nullptr || size < (sizeof(int32_t) * OFFSET)) {
            return;
        }
        auto serviceInstance = UsbService::GetGlobalInstance();
        int32_t portId = *reinterpret_cast<const int32_t *>(rawData);
        uint32_t tokenId = *reinterpret_cast<const uint32_t *>(rawData + sizeof(int32_t));
        serviceInstance->FreeTokenId(portId, tokenId);
    }

    void UsbTransInfoChangeFuzzTest(const uint8_t* rawData, size_t size)
    {
        if (rawData == nullptr || size < sizeof(SerialPort)) {
            return;
        }
        auto serviceInstance = UsbService::GetGlobalInstance();
        USBTransferInfo info;
        UsbTransInfo param = *reinterpret_cast<const UsbTransInfo *>(rawData);
        serviceInstance->UsbTransInfoChange(info, param);
    }

    void UsbCtrlTransferChangeFuzzTest(const uint8_t* rawData, size_t size)
    {
        if (rawData == nullptr || size < sizeof(UsbCtlSetUp)) {
            return;
        }
        auto serviceInstance = UsbService::GetGlobalInstance();
        UsbCtrlTransfer param;
        UsbCtrlTransferParams transferParams;
        UsbCtlSetUp ctlSetup = *reinterpret_cast<const UsbCtlSetUp *>(rawData);
        serviceInstance->UsbCtrlTransferChange(param, ctlSetup);
        serviceInstance->UsbCtrlTransferChange(transferParams, ctlSetup);
    }

    void GetActiveInterfacesJsonFuzzTest(const uint8_t* rawData, size_t size)
    {
        if (rawData == nullptr || size < OFFSET) {
            return;
        }
        auto serviceInstance = UsbService::GetGlobalInstance();
        if (serviceInstance == nullptr || serviceInstance->usbHostManager_ == nullptr) {
            return;
        }

        size_t offset = 0;
        uint8_t busNum = rawData[offset++];
        uint8_t devAddr = rawData[offset++];
        UsbDevice device;
        device.SetBusNum(busNum);
        device.SetDevAddr(devAddr);

        std::vector<USBConfig> configs;
        uint8_t configCount = offset < size ? rawData[offset++] % MAX_CONFIG_COUNT : 0;
        for (uint8_t i = 0; i < configCount; ++i) {
            USBConfig config;
            config.SetId(static_cast<int32_t>(i));
            config.SetName("fuzz_cfg");

            std::vector<UsbInterface> interfaces;
            uint8_t interfaceCount = offset < size ? rawData[offset++] % MAX_INTERFACE_COUNT : 0;
            for (uint8_t j = 0; j < interfaceCount; ++j) {
                if (offset + sizeof(uint8_t) * INTERFACE_FIELD_BYTES > size) {
                    break;
                }
                UsbInterface intf;
                intf.SetId(static_cast<int32_t>(rawData[offset++]));
                intf.SetClass(static_cast<int32_t>(rawData[offset++]));
                intf.SetSubClass(static_cast<int32_t>(rawData[offset++]));
                intf.SetProtocol(static_cast<int32_t>(rawData[offset++]));
                intf.SetAlternateSetting(static_cast<int32_t>(rawData[offset++]));
                interfaces.push_back(intf);
            }
            config.SetInterfaces(interfaces);
            configs.push_back(config);
        }
        device.SetConfigs(configs);

        nlohmann::json interfacesJson;
        serviceInstance->usbHostManager_->GetActiveInterfacesJson(&device, interfacesJson);
    }

    bool UsbMgrPrivateFuzzTest(const uint8_t* rawData, size_t size)
    {
        if (rawData == nullptr) {
            return false;
        }

        GetDeviceVidPidSerialNumberFuzzTest(rawData, size);
        UsbDeviceTypeChangeFuzzTest(rawData, size);
        UpdateDeviceVidPidMapFuzzTest(rawData, size);
        SerialPortChangeFuzzTest(rawData, size);
        UsbTransInfoChangeFuzzTest(rawData, size);
        UsbCtrlTransferChangeFuzzTest(rawData, size);
        GetActiveInterfacesJsonFuzzTest(rawData, size);

        return true;
    }
} // USB
} // OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::USB::UsbMgrPrivateFuzzTest(data, size);
    return 0;
}
