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

#include "usb_cli_serialization.h"
#include "cJSON.h"
#include "usb_config.h"
#include "usb_endpoint.h"
#include "usb_interface.h"

namespace OHOS {
namespace USB {

static cJSON* SerializeEndpoint(USBEndpoint &endpoint)
{
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return nullptr;
    }
    cJSON_AddNumberToObject(json, "address", static_cast<double>(endpoint.GetAddress()));
    cJSON_AddNumberToObject(json, "attributes", static_cast<double>(endpoint.GetAttributes()));
    cJSON_AddNumberToObject(json, "interval", static_cast<double>(endpoint.GetInterval()));
    cJSON_AddNumberToObject(json, "maxPacketSize", static_cast<double>(endpoint.GetMaxPacketSize()));
    cJSON_AddNumberToObject(json, "direction", static_cast<double>(endpoint.GetDirection()));
    cJSON_AddNumberToObject(json, "number", static_cast<double>(endpoint.GetEndpointNumber()));
    cJSON_AddNumberToObject(json, "type", static_cast<double>(endpoint.GetType()));
    cJSON_AddNumberToObject(json, "interfaceId", static_cast<double>(endpoint.GetInterfaceId()));
    return json;
}

static cJSON* SerializeInterface(UsbInterface &iface)
{
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return nullptr;
    }
    cJSON_AddNumberToObject(json, "id", static_cast<double>(iface.GetId()));
    cJSON_AddNumberToObject(json, "protocol", static_cast<double>(iface.GetProtocol()));
    cJSON_AddNumberToObject(json, "clazz", static_cast<double>(iface.GetClass()));
    cJSON_AddNumberToObject(json, "subClass", static_cast<double>(iface.GetSubClass()));
    cJSON_AddNumberToObject(json, "alternateSetting", static_cast<double>(iface.GetAlternateSetting()));
    cJSON_AddStringToObject(json, "name", iface.GetName().c_str());

    cJSON *endpoints = cJSON_CreateArray();
    if (!endpoints) {
        cJSON_Delete(json);
        return nullptr;
    }
    auto &eps = iface.GetEndpoints();
    for (size_t i = 0; i < eps.size(); ++i) {
        cJSON *ep = SerializeEndpoint(eps[i]);
        if (ep) {
            cJSON_AddItemToArray(endpoints, ep);
        }
    }
    cJSON_AddItemToObject(json, "endpoints", endpoints);
    return json;
}

static cJSON* SerializeConfig(USBConfig &config)
{
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return nullptr;
    }
    cJSON_AddNumberToObject(json, "id", static_cast<double>(config.GetId()));
    cJSON_AddNumberToObject(json, "attributes", static_cast<double>(config.GetAttributes()));
    cJSON_AddNumberToObject(json, "maxPower", static_cast<double>(config.GetMaxPower()));
    cJSON_AddStringToObject(json, "name", config.GetName().c_str());
    cJSON_AddBoolToObject(json, "isRemoteWakeup", config.IsRemoteWakeup());
    cJSON_AddBoolToObject(json, "isSelfPowered", config.IsSelfPowered());

    cJSON *interfaces = cJSON_CreateArray();
    if (!interfaces) {
        cJSON_Delete(json);
        return nullptr;
    }
    for (uint32_t i = 0; i < config.GetInterfaceCount(); ++i) {
        UsbInterface iface;
        if (config.GetInterface(i, iface)) {
            cJSON *intf = SerializeInterface(iface);
            if (intf) {
                cJSON_AddItemToArray(interfaces, intf);
            }
        }
    }
    cJSON_AddItemToObject(json, "interfaces", interfaces);
    return json;
}

cJSON* SerializeDevice(UsbDevice &device)
{
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return nullptr;
    }
    cJSON_AddNumberToObject(json, "busNum", static_cast<double>(device.GetBusNum()));
    cJSON_AddNumberToObject(json, "devAddress", static_cast<double>(device.GetDevAddr()));
    cJSON_AddStringToObject(json, "serial", "");
    cJSON_AddStringToObject(json, "name", device.GetName().c_str());
    cJSON_AddStringToObject(json, "manufacturerName", device.GetManufacturerName().c_str());
    cJSON_AddStringToObject(json, "productName", device.GetProductName().c_str());
    cJSON_AddStringToObject(json, "version", device.GetVersion().c_str());
    cJSON_AddNumberToObject(json, "vendorId", static_cast<double>(device.GetVendorId()));
    cJSON_AddNumberToObject(json, "productId", static_cast<double>(device.GetProductId()));
    cJSON_AddNumberToObject(json, "clazz", static_cast<double>(device.GetClass()));
    cJSON_AddNumberToObject(json, "subClass", static_cast<double>(device.GetSubclass()));
    cJSON_AddNumberToObject(json, "protocol", static_cast<double>(device.GetProtocol()));

    cJSON *configs = cJSON_CreateArray();
    if (!configs) {
        cJSON_Delete(json);
        return nullptr;
    }
    auto &cfgs = device.GetConfigs();
    for (size_t i = 0; i < cfgs.size(); ++i) {
        cJSON *cfg = SerializeConfig(cfgs[i]);
        if (cfg) {
            cJSON_AddItemToArray(configs, cfg);
        }
    }
    cJSON_AddItemToObject(json, "configs", configs);
    return json;
}

cJSON* SerializeAccessory(const USBAccessory &accessory)
{
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return nullptr;
    }
    cJSON_AddStringToObject(json, "manufacturer", accessory.GetManufacturer().c_str());
    cJSON_AddStringToObject(json, "product", accessory.GetProduct().c_str());
    cJSON_AddStringToObject(json, "description", accessory.GetDescription().c_str());
    cJSON_AddStringToObject(json, "version", accessory.GetVersion().c_str());
    cJSON_AddStringToObject(json, "serialNumber", "");
    return json;
}

cJSON* SerializeSerialPort(const UsbSerialPort &port)
{
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return nullptr;
    }
    cJSON_AddNumberToObject(json, "portId", static_cast<double>(port.portId_));
    
    // deviceName: "busNum-devAddr"
    std::string deviceName = std::to_string(port.busNum_) + "-" + std::to_string(port.devAddr_);
    cJSON_AddStringToObject(json, "deviceName", deviceName.c_str());
    
    return json;
}

std::string FormatSuccess(const std::vector<cJSON*> &items)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return "{\"type\":\"result\",\"status\":\"failed\",\"errCode\":\"INTERNAL_ERROR\",\"errMsg\":\"Failed to create JSON object\",\"suggestion\":\"Please check if the device is connected and the connection is normal.\"}";
    }
    cJSON_AddStringToObject(root, "type", "result");
    cJSON_AddStringToObject(root, "status", "success");

    cJSON *data = cJSON_CreateObject();
    if (!data) {
        cJSON_Delete(root);
        return "{\"type\":\"result\",\"status\":\"failed\",\"errCode\":\"INTERNAL_ERROR\",\"errMsg\":\"Failed to create data object\",\"suggestion\":\"Please check if the device is connected and the connection is normal.\"}";
    }

    cJSON *list = cJSON_CreateArray();
    if (!list) {
        cJSON_Delete(data);
        cJSON_Delete(root);
        return "{\"type\":\"result\",\"status\":\"failed\",\"errCode\":\"INTERNAL_ERROR\",\"errMsg\":\"Failed to create list array\",\"suggestion\":\"Please check if the device is connected and the connection is normal.\"}";
    }
    for (auto *item : items) {
        if (item) {
            cJSON_AddItemToArray(list, item);
        }
    }
    cJSON_AddNumberToObject(data, "number", static_cast<double>(cJSON_GetArraySize(list)));
    cJSON_AddItemToObject(data, "list", list);
    cJSON_AddItemToObject(root, "data", data);

    char *output = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!output) {
        return "{\"type\":\"result\",\"status\":\"failed\",\"errCode\":\"INTERNAL_ERROR\",\"errMsg\":\"Failed to format JSON output\",\"suggestion\":\"Please check if the device is connected and the connection is normal.\"}";
    }
    std::string result(output);
    cJSON_free(output);
    return result;
}

static std::string GetErrorCodeString(int32_t code)
{
    if (code == 1) {
        return "MISSING_SUBCOMMAND";
    }
    if (code == 2) {
        return "UNKNOWN_SUBCOMMAND";
    }
    if (code == UEC_INTERFACE_NO_INIT || code == UEC_SERVICE_NO_INIT) {
        return "USB_SERVICE_UNAVAILABLE";
    }
    if (code == UEC_INTERFACE_PERMISSION_DENIED || code == UEC_SERVICE_PERMISSION_DENIED) {
        return "PERMISSION_DENIED";
    }
    if (code == UEC_SERIAL_PORT_NOT_EXIST) {
        return "SERIAL_PORT_NOT_FOUND";
    }
    return "QUERY_FAILED";
}

static std::string GetErrorSuggestion(int32_t code)
{
    if (code == 1) {
        return "Run './ohos-usbManager --help' for available commands.";
    }
    return "Please check if the device is connected and the connection is normal.";
}

std::string FormatError(int32_t code, const std::string &message)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return "{\"type\":\"result\",\"status\":\"failed\",\"errCode\":\"INTERNAL_ERROR\",\"errMsg\":\"Failed to create JSON object\",\"suggestion\":\"Please check if the device is connected and the connection is normal.\"}";
    }
    cJSON_AddStringToObject(root, "type", "result");
    cJSON_AddStringToObject(root, "status", "failed");
    cJSON_AddStringToObject(root, "errCode", GetErrorCodeString(code).c_str());
    cJSON_AddStringToObject(root, "errMsg", message.c_str());
    cJSON_AddStringToObject(root, "suggestion", GetErrorSuggestion(code).c_str());

    char *output = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!output) {
        return "{\"type\":\"result\",\"status\":\"failed\",\"errCode\":\"INTERNAL_ERROR\",\"errMsg\":\"Failed to format JSON output\",\"suggestion\":\"Please check if the device is connected and the connection is normal.\"}";
    }
    std::string result(output);
    cJSON_free(output);
    return result;
}

}
}
