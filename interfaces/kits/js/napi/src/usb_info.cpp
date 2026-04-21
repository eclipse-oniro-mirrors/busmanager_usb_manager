/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unistd.h>

#include <sys/time.h>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <uv.h>

#include "v1_2/usb_types.h"
#include "ashmem.h"
#include "hilog_wrapper.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_common.h"
#include "napi_util.h"
#include "securec.h"
#include "usb_async_context.h"
#include "usb_device_pipe.h"
#include "usb_endpoint.h"
#include "usb_errors.h"
#include "usb_napi_errors.h"
#include "usb_srv_client.h"
#include "usb_accessory.h"
#include "hitrace_meter.h"
#include "hdf_base.h"
#include "struct_parcel.h"
using namespace OHOS;
using namespace OHOS::USB;
using namespace OHOS::HDI::Usb::V1_0;
using namespace OHOS::HDI::Usb::V1_2;

static constexpr int32_t INDEX_0 = 0;
static constexpr int32_t INDEX_1 = 1;
static constexpr int32_t INDEX_2 = 2;
static constexpr int32_t INDEX_3 = 3;
static constexpr int32_t PARAM_COUNT_0 = 0;
static constexpr int32_t PARAM_COUNT_1 = 1;
static constexpr int32_t PARAM_COUNT_2 = 2;
static constexpr int32_t PARAM_COUNT_3 = 3;
static constexpr int32_t PARAM_COUNT_4 = 4;
static constexpr int32_t STR_DEFAULT_SIZE = 256;
static constexpr int32_t DEFAULT_DESCRIPTION_SIZE = 32;
static constexpr int32_t DEFAULT_ACCESSORY_DESCRIPTION_SIZE = 256;
static int32_t g_accFd = 0;

const int32_t USB_TRANSFER_SHORT_NOT_OK = 0;
const int32_t USB_TRANSFER_FREE_BUFFER = 1;
const int32_t USB_TRANSFER_FREE_TRANSFER = 2;
const int32_t USB_TRANSFER_ADD_ZERO_PACKET = 3;
const int32_t TRANSFER_TYPE_ISOCHRONOUS = 1;
const int32_t TRANSFER_TYPE_BULK = 2;
const int32_t TRANSFER_TYPE_INTERRUPT = 3;
const int32_t TRANSFER_COMPLETED = 0;
const int32_t TRANSFER_ERROR = 1;
const int32_t TRANSFER_TIMED_OUT = 2;
const int32_t TRANSFER_CANCELED = 3;
const int32_t TRANSFER_STALL = 4;
const int32_t TRANSFER_NO_DEVICE = 5;
const int32_t TRANSFER_OVERFLOW = 6;
const int32_t IO_ERROR = -1;
const int32_t INVALID_PARAM = -2;
const int32_t NO_DEVICE = -4;
const int32_t NOT_FOUND = -5;
const int32_t ERROR_BUSY = -6;
const int32_t NO_MEM = -11;
const int32_t DEFAULT_SUBMIT_BUFFER_SIZE = 1024;
enum UsbManagerFeature {
    FEATURE_HOST = 0,
    FEATURE_DEVICE = 1,
    FEATURE_PORT = 2,
};

static bool HasFeature(UsbManagerFeature feature)
{
    switch (feature) {
        case FEATURE_HOST:
#ifndef USB_MANAGER_FEATURE_HOST
            return false;
#else
            return true;
#endif // USB_MANAGER_FEATURE_HOST
        case FEATURE_DEVICE:
#ifndef USB_MANAGER_FEATURE_DEVICE
            return false;
#else
            return true;
#endif // USB_MANAGER_FEATURE_DEVICE
        case FEATURE_PORT:
#ifndef USB_MANAGER_FEATURE_PORT
            return false;
#else
            return true;
#endif // USB_MANAGER_FEATURE_PORT
        default:;
    }
    return false;
}

static void ParseUsbDevicePipe(const napi_env env, const napi_value &obj, USBDevicePipe &pipe)
{
    napi_valuetype valueType;
    napi_typeof(env, obj, &valueType);
    USB_ASSERT_RETURN_VOID(
        env, valueType == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");

    int32_t busNum = 0;
    NapiUtil::JsObjectToInt(env, obj, "busNum", busNum);
    pipe.SetBusNum(static_cast<uint8_t>(busNum));
    int32_t devAddr = 0;
    NapiUtil::JsObjectToInt(env, obj, "devAddress", devAddr);
    pipe.SetDevAddr(static_cast<uint8_t>(devAddr));
}

static void ProcessPromise(const napi_env env, const USBAsyncContext &asyncContext, napi_value &result)
{
    if (asyncContext.deferred) {
        if (asyncContext.status == napi_ok) {
            napi_resolve_deferred(env, asyncContext.deferred, result);
        } else {
            napi_reject_deferred(env, asyncContext.deferred, result);
        }
    }
}

static void CreateUsbDevicePipe(const napi_env env, napi_value &obj, const USBDevicePipe &pipe)
{
    napi_create_object(env, &obj);
    NapiUtil::SetValueInt32(env, "busNum", pipe.GetBusNum(), obj);
    NapiUtil::SetValueInt32(env, "devAddress", pipe.GetDevAddr(), obj);
}

static void CreatAccessoryHandle(const napi_env env, napi_value &obj, int32_t fd)
{
    napi_create_object(env, &obj);
    NapiUtil::SetValueInt32(env, "accessoryFd", fd, obj);
}

static void CtoJSUsbEndpoint(const napi_env &env, napi_value &obj, const USBEndpoint &usbEndpoint)
{
    napi_create_object(env, &obj);
    NapiUtil::SetValueUint32(env, "address", usbEndpoint.GetAddress(), obj);
    NapiUtil::SetValueUint32(env, "attributes", usbEndpoint.GetAttributes(), obj);
    NapiUtil::SetValueInt32(env, "interval", usbEndpoint.GetInterval(), obj);
    NapiUtil::SetValueInt32(env, "maxPacketSize", usbEndpoint.GetMaxPacketSize(), obj);
    NapiUtil::SetValueUint32(env, "direction", usbEndpoint.GetDirection(), obj);
    NapiUtil::SetValueUint32(env, "number", usbEndpoint.GetEndpointNumber(), obj);
    NapiUtil::SetValueUint32(env, "type", usbEndpoint.GetType(), obj);
    NapiUtil::SetValueInt32(env, "interfaceId", usbEndpoint.GetInterfaceId(), obj);
}

static void CtoJSUsbInterface(const napi_env &env, napi_value &obj, const UsbInterface &usbInterface)
{
    napi_create_object(env, &obj);
    NapiUtil::SetValueInt32(env, "id", usbInterface.GetId(), obj);
    NapiUtil::SetValueInt32(env, "protocol", usbInterface.GetProtocol(), obj);
    NapiUtil::SetValueInt32(env, "clazz", usbInterface.GetClass(), obj);
    NapiUtil::SetValueInt32(env, "subClass", usbInterface.GetSubClass(), obj);
    NapiUtil::SetValueInt32(env, "alternateSetting", usbInterface.GetAlternateSetting(), obj);
    NapiUtil::SetValueUtf8String(env, "name", usbInterface.GetName(), obj);

    napi_value arr;
    napi_create_array(env, &arr);
    for (int32_t i = 0; i < usbInterface.GetEndpointCount(); ++i) {
        auto usbEndpoint = usbInterface.GetEndpoint(i);
        if (!usbEndpoint.has_value()) {
            USB_HILOGE(MODULE_USB_NAPI, "GetEndpoint failed i=%{public}d", i);
            return;
        }

        napi_value objTmp;
        CtoJSUsbEndpoint(env, objTmp, usbEndpoint.value());
        napi_set_element(env, arr, i, objTmp);
    }
    napi_set_named_property(env, obj, "endpoints", arr);
}

static void CtoJSUsbConfig(const napi_env &env, napi_value &obj, const USBConfig &usbConfig)
{
    napi_create_object(env, &obj);
    NapiUtil::SetValueInt32(env, "id", usbConfig.GetId(), obj);
    NapiUtil::SetValueUint32(env, "attributes", usbConfig.GetAttributes(), obj);
    NapiUtil::SetValueBool(env, "isRemoteWakeup", usbConfig.IsRemoteWakeup(), obj);
    NapiUtil::SetValueBool(env, "isSelfPowered", usbConfig.IsSelfPowered(), obj);
    NapiUtil::SetValueInt32(env, "maxPower", usbConfig.GetMaxPower(), obj);
    NapiUtil::SetValueUtf8String(env, "name", usbConfig.GetName(), obj);
    napi_value arr;
    napi_create_array(env, &arr);
    for (uint32_t i = 0; i < usbConfig.GetInterfaceCount(); ++i) {
        UsbInterface usbInterface;
        usbConfig.GetInterface(i, usbInterface);
        napi_value objTmp;
        CtoJSUsbInterface(env, objTmp, usbInterface);
        napi_set_element(env, arr, i, objTmp);
    }
    napi_set_named_property(env, obj, "interfaces", arr);
}

static void CtoJSUsbDevice(const napi_env &env, napi_value &obj, const UsbDevice &usbDevice)
{
    napi_create_object(env, &obj);
    NapiUtil::SetValueUtf8String(env, "name", usbDevice.GetName(), obj);
    NapiUtil::SetValueUtf8String(env, "serial", usbDevice.GetmSerial(), obj);
    NapiUtil::SetValueUtf8String(env, "manufacturerName", usbDevice.GetManufacturerName(), obj);
    NapiUtil::SetValueUtf8String(env, "productName", usbDevice.GetProductName(), obj);
    NapiUtil::SetValueUtf8String(env, "version", usbDevice.GetVersion(), obj);
    NapiUtil::SetValueInt32(env, "vendorId", usbDevice.GetVendorId(), obj);
    NapiUtil::SetValueInt32(env, "productId", usbDevice.GetProductId(), obj);
    NapiUtil::SetValueInt32(env, "clazz", usbDevice.GetClass(), obj);
    NapiUtil::SetValueInt32(env, "subClass", usbDevice.GetSubclass(), obj);
    NapiUtil::SetValueInt32(env, "protocol", usbDevice.GetProtocol(), obj);
    NapiUtil::SetValueInt32(env, "devAddress", usbDevice.GetDevAddr(), obj);
    NapiUtil::SetValueInt32(env, "busNum", usbDevice.GetBusNum(), obj);
    napi_value arr;
    napi_create_array(env, &arr);
    for (int32_t i = 0; i < usbDevice.GetConfigCount(); ++i) {
        USBConfig usbConfig;
        usbDevice.GetConfig(i, usbConfig);
        napi_value objTmp;
        CtoJSUsbConfig(env, objTmp, usbConfig);
        napi_set_element(env, arr, i, objTmp);
    }
    napi_set_named_property(env, obj, "configs", arr);
}

static void CtoJSUSBAccessory(const napi_env &env, napi_value &obj, const USBAccessory &accessory)
{
    napi_create_object(env, &obj);
    NapiUtil::SetValueUtf8String(env, "manufacturer", accessory.GetManufacturer(), obj);
    NapiUtil::SetValueUtf8String(env, "product", accessory.GetProduct(), obj);
    NapiUtil::SetValueUtf8String(env, "description", accessory.GetDescription(), obj);
    NapiUtil::SetValueUtf8String(env, "version", accessory.GetVersion(), obj);
    NapiUtil::SetValueUtf8String(env, "serialNumber", accessory.GetSerialNumber(), obj);
}

static UsbSrvClient &g_usbClient = UsbSrvClient::GetInstance();

/* ============================================= Parsers ============================================= */
// js to c
static void ParseEndpointObj(const napi_env env, const napi_value endpointObj, USBEndpoint &ep)
{
    int32_t address = 0;
    NapiUtil::JsObjectToInt(env, endpointObj, "address", address);
    int32_t attributes = 0;
    NapiUtil::JsObjectToInt(env, endpointObj, "attributes", attributes);
    int32_t interval = 0;
    NapiUtil::JsObjectToInt(env, endpointObj, "interval", interval);
    int32_t maxPacketSize = 0;
    NapiUtil::JsObjectToInt(env, endpointObj, "maxPacketSize", maxPacketSize);
    int32_t direction = 0;
    NapiUtil::JsObjectToInt(env, endpointObj, "direction", direction);
    USB_ASSERT_RETURN_VOID(env, (direction == USB_ENDPOINT_DIR_IN || direction == USB_ENDPOINT_DIR_OUT),
        OHEC_COMMON_PARAM_ERROR, "The interface should have the endpoints property.");
    int32_t number = 0;
    NapiUtil::JsObjectToInt(env, endpointObj, "number", number);
    int32_t type = 0;
    NapiUtil::JsObjectToInt(env, endpointObj, "type", type);
    int32_t interfaceId = 0;
    NapiUtil::JsObjectToInt(env, endpointObj, "interfaceId", interfaceId);
    ep = USBEndpoint(address, attributes, interval, maxPacketSize);
    ep.SetInterfaceId(interfaceId);
}

static bool ParseEndpointsObjs(const napi_env env, const napi_value interfaceObj, std::vector<USBEndpoint> &eps)
{
    napi_value endpointsObjs;
    bool isGetObjSuccess = NapiUtil::JsObjectGetProperty(env, interfaceObj, "endpoints", endpointsObjs);
    USB_ASSERT_RETURN_FALSE(
        env, isGetObjSuccess == true, OHEC_COMMON_PARAM_ERROR, "The interface should have the endpoints property.");

    bool result = false;
    NAPI_CHECK_RETURN_FALSE(napi_is_array(env, endpointsObjs, &result), "Get endpoints type failed");
    USB_ASSERT_RETURN_FALSE(env, result == true, OHEC_COMMON_PARAM_ERROR, "The type of endpoints must be array.");

    uint32_t endpointCount = 0;
    NAPI_CHECK_RETURN_FALSE(napi_get_array_length(env, endpointsObjs, &endpointCount), "Get array length failed");

    for (uint32_t k = 0; k < endpointCount; ++k) {
        napi_value endpointObj;
        NAPI_CHECK_RETURN_FALSE(napi_get_element(env, endpointsObjs, k, &endpointObj), "Get endpoints element failed");
        USBEndpoint ep;
        ParseEndpointObj(env, endpointObj, ep);
        eps.push_back(ep);
    }

    return true;
}

struct PipeControlParam {
    int32_t request;
    int32_t target;
    uint32_t reqType;
    int32_t value;
    int32_t index;
    uint8_t *data;
    size_t dataLength;
};

static bool ParsePipeControlParam(const napi_env env, const napi_value jsObj, PipeControlParam &controlParam)
{
    int32_t request = 0;
    NapiUtil::JsObjectToInt(env, jsObj, "request", request);
    int32_t target = 0;
    NapiUtil::JsObjectToInt(env, jsObj, "target", target);
    uint32_t reqType = 0;
    NapiUtil::JsObjectToUint(env, jsObj, "reqType", reqType);
    int32_t value = 0;
    NapiUtil::JsObjectToInt(env, jsObj, "value", value);
    int32_t index = 0;
    NapiUtil::JsObjectToInt(env, jsObj, "index", index);

    napi_value dataValue;
    bool hasProperty = NapiUtil::JsObjectGetProperty(env, jsObj, "data", dataValue);
    USB_ASSERT_RETURN_FALSE(
        env, hasProperty == true, OHEC_COMMON_PARAM_ERROR, "The controlParam should have the data property.");

    uint8_t *data = nullptr;
    size_t dataLength = 0;
    size_t offset = 0;
    NapiUtil::JsUint8ArrayParseReserveZeroBuffer(env, dataValue, &data, dataLength, offset);
    controlParam.request = request;
    controlParam.target = target;
    controlParam.reqType = reqType;
    controlParam.value = value;
    controlParam.index = index;
    controlParam.data = data;
    controlParam.dataLength = dataLength;
    return true;
}

struct UsbPipeControlParam {
    uint32_t reqType;
    int32_t request;
    int32_t value;
    int32_t index;
    int32_t length;
    uint8_t *data;
    size_t dataLength;
};

static void ParseUsbPipeControlParam(const napi_env env, const napi_value jsObj, UsbPipeControlParam &controlParam)
{
    uint32_t reqType = 0;
    NapiUtil::JsObjectToUint(env, jsObj, "bmRequestType", reqType);
    int32_t request = 0;
    NapiUtil::JsObjectToInt(env, jsObj, "bRequest", request);
    int32_t value = 0;
    NapiUtil::JsObjectToInt(env, jsObj, "wValue", value);
    int32_t index = 0;
    NapiUtil::JsObjectToInt(env, jsObj, "wIndex", index);
    int32_t length = 0;
    NapiUtil::JsObjectToInt(env, jsObj, "wLength", length);

    napi_value dataValue;
    bool hasProperty = NapiUtil::JsObjectGetProperty(env, jsObj, "data", dataValue);
    USB_ASSERT_RETURN_VOID(
        env, hasProperty == true, OHEC_COMMON_PARAM_ERROR, "The controlParam should have the data property.");

    uint8_t *data = nullptr;
    size_t dataLength = 0;
    size_t offset = 0;
    NapiUtil::JsUint8ArrayParseReserveZeroBuffer(env, dataValue, &data, dataLength, offset);
    controlParam.reqType = reqType;
    controlParam.request = request;
    controlParam.value = value;
    controlParam.index = index;
    controlParam.length = length;
    controlParam.data = data;
    controlParam.dataLength = dataLength;
}

static void ParseInterfaceObj(const napi_env env, const napi_value interfaceObj, UsbInterface &interface)
{
    int32_t id = 0;
    NapiUtil::JsObjectToInt(env, interfaceObj, "id", id);
    int32_t protocol = 0;
    NapiUtil::JsObjectToInt(env, interfaceObj, "protocol", protocol);
    int32_t clzz = 0;
    NapiUtil::JsObjectToInt(env, interfaceObj, "clazz", clzz);
    int32_t subClass = 0;
    NapiUtil::JsObjectToInt(env, interfaceObj, "subClass", subClass);
    int32_t alternateSetting = 0;
    NapiUtil::JsObjectToInt(env, interfaceObj, "alternateSetting", alternateSetting);
    std::string name;
    NapiUtil::JsObjectToString(env, interfaceObj, "name", DEFAULT_DESCRIPTION_SIZE, name);
    std::vector<USBEndpoint> eps;

    bool ret = ParseEndpointsObjs(env, interfaceObj, eps);
    if (!ret) {
        USB_HILOGE(MODULE_USB_NAPI, "Parse endpointers error.");
        return;
    }

    interface = UsbInterface(id, protocol, clzz, subClass, alternateSetting, name, eps);
}

static bool ParseInterfacesObjs(const napi_env env, const napi_value configObj, std::vector<UsbInterface> &interfaces)
{
    napi_value interfacesObjs;
    bool isGetObjSuccess = NapiUtil::JsObjectGetProperty(env, configObj, "interfaces", interfacesObjs);
    USB_ASSERT_RETURN_FALSE(
        env, isGetObjSuccess == true, OHEC_COMMON_PARAM_ERROR, "The config should have the interfaces property.");

    bool result = false;
    NAPI_CHECK_RETURN_FALSE(napi_is_array(env, interfacesObjs, &result), "Get interfaces type failed");
    USB_ASSERT_RETURN_FALSE(env, result == true, OHEC_COMMON_PARAM_ERROR, "The type of interfaces must be array.");

    uint32_t interfaceCount = 0;
    NAPI_CHECK_RETURN_FALSE(napi_get_array_length(env, interfacesObjs, &interfaceCount), "Get array length failed");

    for (uint32_t i = 0; i < interfaceCount; ++i) {
        napi_value interfaceObj;
        NAPI_CHECK_RETURN_FALSE(
            napi_get_element(env, interfacesObjs, i, &interfaceObj), "Get interfaces element failed");

        UsbInterface interface;
        ParseInterfaceObj(env, interfaceObj, interface);
        interfaces.push_back(interface);
    }

    return true;
}

static void ParseConfigObj(const napi_env env, const napi_value configObj, USBConfig &config)
{
    int32_t id = 0;
    NapiUtil::JsObjectToInt(env, configObj, "id", id);
    int32_t attributes = 0;
    NapiUtil::JsObjectToInt(env, configObj, "attributes", attributes);
    int32_t maxPower = 0;
    NapiUtil::JsObjectToInt(env, configObj, "maxPower", maxPower);
    std::string name;
    NapiUtil::JsObjectToString(env, configObj, "name", DEFAULT_DESCRIPTION_SIZE, name);
    bool isRemoteWakeup = false;
    NapiUtil::JsObjectToBool(env, configObj, "isRemoteWakeup", isRemoteWakeup);
    bool isSelfPowered = false;
    NapiUtil::JsObjectToBool(env, configObj, "isSelfPowered", isSelfPowered);

    std::vector<UsbInterface> interfaces;
    bool ret = ParseInterfacesObjs(env, configObj, interfaces);
    if (!ret) {
        USB_HILOGE(MODULE_USB_NAPI, "Parse interfaces error.");
        return;
    }

    config = USBConfig(id, attributes, name, maxPower, interfaces);
}

static void ParseConfigsObjs(const napi_env env, const napi_value deviceObj, std::vector<USBConfig> &configs)
{
    napi_value configsObj;
    bool hasProperty = NapiUtil::JsObjectGetProperty(env, deviceObj, "configs", configsObj);
    USB_ASSERT_RETURN_VOID(
        env, hasProperty == true, OHEC_COMMON_PARAM_ERROR, "The device should have the configs property.");
    napi_valuetype valueType;
    napi_typeof(env, configsObj, &valueType);
    USB_ASSERT_RETURN_VOID(
        env, valueType == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of configs must be object.");

    uint32_t configCount = 0;
    napi_get_array_length(env, configsObj, &configCount);
    for (uint32_t i = 0; i < configCount; ++i) {
        napi_value configObj;
        napi_get_element(env, configsObj, i, &configObj);
        USBConfig config;
        ParseConfigObj(env, configObj, config);
        configs.push_back(config);
    }
}

static void ParseDeviceObj(const napi_env env, const napi_value deviceObj, UsbDevice &dev)
{
    std::string name;
    NapiUtil::JsObjectToString(env, deviceObj, "name", DEFAULT_DESCRIPTION_SIZE, name);
    std::string manufacturerName;
    NapiUtil::JsObjectToString(env, deviceObj, "manufacturerName", DEFAULT_DESCRIPTION_SIZE, manufacturerName);
    std::string productName;
    NapiUtil::JsObjectToString(env, deviceObj, "productName", DEFAULT_DESCRIPTION_SIZE, productName);
    std::string version;
    NapiUtil::JsObjectToString(env, deviceObj, "version", DEFAULT_DESCRIPTION_SIZE, version);
    std::string serial;
    NapiUtil::JsObjectToString(env, deviceObj, "serial", DEFAULT_DESCRIPTION_SIZE, serial);
    int32_t devAddr = 0;
    NapiUtil::JsObjectToInt(env, deviceObj, "devAddress", devAddr);
    int32_t busNum = 0;
    NapiUtil::JsObjectToInt(env, deviceObj, "busNum", busNum);
    int32_t vendorId = 0;
    NapiUtil::JsObjectToInt(env, deviceObj, "vendorId", vendorId);
    int32_t productId = 0;
    NapiUtil::JsObjectToInt(env, deviceObj, "productId", productId);
    int32_t clazz = 0;
    NapiUtil::JsObjectToInt(env, deviceObj, "clazz", clazz);
    int32_t subClass = 0;
    NapiUtil::JsObjectToInt(env, deviceObj, "subClass", subClass);
    int32_t protocol = 0;
    NapiUtil::JsObjectToInt(env, deviceObj, "protocol", protocol);
    std::vector<USBConfig> configs;
    ParseConfigsObjs(env, deviceObj, configs);
    dev = UsbDevice(name, manufacturerName, productName, version, devAddr, busNum, vendorId, productId, clazz, subClass,
        protocol, configs);
}

static void ParseAccessoryObj(const napi_env env, const napi_value accessoryObj, USBAccessory &accessory)
{
    std::string manufacturer;
    NapiUtil::JsObjectToString(env, accessoryObj, "manufacturer", DEFAULT_ACCESSORY_DESCRIPTION_SIZE, manufacturer);
    std::string product;
    NapiUtil::JsObjectToString(env, accessoryObj, "product", DEFAULT_ACCESSORY_DESCRIPTION_SIZE, product);
    std::string description;
    NapiUtil::JsObjectToString(env, accessoryObj, "description", DEFAULT_ACCESSORY_DESCRIPTION_SIZE, description);
    std::string version;
    NapiUtil::JsObjectToString(env, accessoryObj, "version", DEFAULT_ACCESSORY_DESCRIPTION_SIZE, version);
    std::string serialNumber;
    NapiUtil::JsObjectToString(env, accessoryObj, "serialNumber", DEFAULT_ACCESSORY_DESCRIPTION_SIZE, serialNumber);
    accessory = USBAccessory(manufacturer, product, description, version, serialNumber);
}

/* ============================================= Usb Core ============================================= */

static napi_value CoreGetDevices(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    USB_ASSERT(env, (argc == PARAM_COUNT_0), OHEC_COMMON_PARAM_ERROR, "The function takes no arguments.");

    std::vector<UsbDevice> deviceList;
    int32_t ret = g_usbClient.GetDevices(deviceList);

    napi_value result;
    napi_create_array(env, &result);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_NAPI, "end call get device failed ret : %{public}d", ret);
        return result;
    }

    int32_t i = 0;
    for (const auto &ent1 : deviceList) {
        napi_value element;
        napi_create_object(env, &element);
        napi_value device;
        CtoJSUsbDevice(env, device, ent1);
        napi_set_element(env, result, i, device);
        ++i;
    }

    return result;
}

static napi_value DeviceGetAccessoryList(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_DEVICE)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    USB_ASSERT(env, (argc == PARAM_COUNT_0), OHEC_COMMON_PARAM_ERROR, "The function takes no arguments.");

    std::vector<USBAccessory> accessoryList;
    int32_t ret = g_usbClient.GetAccessoryList(accessoryList);
    if (ret == UEC_OK) {
        napi_value result;
        napi_create_array(env, &result);
        int32_t i = 0;
        for (const auto &ent1 : accessoryList) {
            napi_value element;
            napi_create_object(env, &element);
            napi_value device;
            CtoJSUSBAccessory(env, device, ent1);
            napi_set_element(env, result, i, device);
            ++i;
        }
        return result;
    } else {
        ThrowBusinessError(env, UEC_COMMON_SERVICE_EXCEPTION,
            "Service exception");
    }
    return nullptr;
}

static napi_value CoreConnectDevice(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_value deviceObj = argv[INDEX_0];
    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, deviceObj, &type), "Get deviceObj type failed");
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of device must be USBDevice.");
    UsbDevice dev;
    ParseDeviceObj(env, deviceObj, dev);

    USBDevicePipe pipe;
    int32_t ret = g_usbClient.OpenDevice(dev, pipe);
    napi_value pipObj = nullptr;
    if (ret == UEC_OK) {
        CreateUsbDevicePipe(env, pipObj, pipe);
    } else if (ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_INTERFACE_PERMISSION_DENIED) {
        ThrowBusinessError(env, UEC_COMMON_HAS_NO_RIGHT,
            "need call requestRight to get the permission");
    } else {
        napi_get_undefined(env, &pipObj);
    }

    return pipObj;
}

static napi_value DeviceOpenAccessory(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_DEVICE)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_value accessoryObj = argv[INDEX_0];
    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, accessoryObj, &type), "Get accessoryObj type failed");
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of device must be USBAccessory.");
    USBAccessory accessory;
    ParseAccessoryObj(env, accessoryObj, accessory);

    int32_t fd = -1;
    int32_t ret = g_usbClient.OpenAccessory(accessory, fd);

    napi_value handleObj = nullptr;
    if (ret == UEC_OK) {
        g_accFd = fd;
        CreatAccessoryHandle(env, handleObj, fd);
    } else if (ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_INTERFACE_PERMISSION_DENIED) {
        ThrowBusinessError(env, UEC_COMMON_HAS_NO_RIGHT,
            "Call requestAccessoryRight to get the permission first");
    } else if (ret == UEC_SERVICE_ACCESSORY_NOT_MATCH) {
        ThrowBusinessError(env, UEC_ACCESSORY_NOT_MATCH,
            "Get accessory through getAccessoryList");
    } else if (ret == UEC_SERVICE_ACCESSORY_OPEN_NATIVE_NODE_FAILED) {
        ThrowBusinessError(env, UEC_ACCESSORY_OPEN_FAILED,
            "Failed to open the native accessory node");
    } else if (ret == UEC_SERVICE_ACCESSORY_REOPEN) {
        ThrowBusinessError(env, UEC_ACCESSORY_CAN_NOT_REOPEN,
            "Cannot reopen accessory");
    } else {
        ThrowBusinessError(env, UEC_COMMON_SERVICE_EXCEPTION,
            "Service exception");
    }
    return handleObj;
}

static napi_value DeviceCloseAccessory(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_DEVICE)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_value accessoryFdObj = argv[INDEX_0];
    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, accessoryFdObj, &type), "Get accessoryObj type failed");
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of device must be USBAccessoryHandle.");
    int32_t accessoryFd;
    NapiUtil::JsObjectToInt(env, argv[INDEX_0], "accessoryFd", accessoryFd);
    if (accessoryFd == 0 || accessoryFd != g_accFd || g_accFd == 0) {
        ThrowBusinessError(env, OHEC_COMMON_PARAM_ERROR,
            "Parameter accessoryHandle error, need openAccessory first.");
    }
    close(accessoryFd);
    accessoryFd = 0;
    int32_t ret = g_usbClient.CloseAccessory(g_accFd);
    g_accFd = 0;
    if (ret != UEC_OK) {
        ThrowBusinessError(env, UEC_COMMON_SERVICE_EXCEPTION,
            "Service exception");
    }
    return nullptr;
}

static napi_value DeviceAddRight(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_2;
    napi_value argv[PARAM_COUNT_2] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_2), OHEC_COMMON_PARAM_ERROR, "The function at least takes two argument.");

    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, argv[INDEX_0], &type), "Get args 1 type failed");
    USB_ASSERT(env, type == napi_string, OHEC_COMMON_PARAM_ERROR, "The type of bundleName must be string.");
    std::string bundleName;
    NapiUtil::JsValueToString(env, argv[INDEX_0], STR_DEFAULT_SIZE, bundleName);

    NAPI_CHECK(env, napi_typeof(env, argv[INDEX_1], &type), "Get args 2 type failed");
    USB_ASSERT(env, type == napi_string, OHEC_COMMON_PARAM_ERROR, "The type of deviceName must be string.");
    std::string deviceName;
    NapiUtil::JsValueToString(env, argv[INDEX_1], STR_DEFAULT_SIZE, deviceName);

    napi_value result;
    int32_t ret = g_usbClient.AddRight(bundleName, deviceName);
    USB_HILOGD(MODULE_USB_NAPI, "Device call AddRight ret: %{public}d", ret);
    if (ret == UEC_OK) {
        napi_get_boolean(env, true, &result);
    } else {
        USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI),
            OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
        USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED),
            OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
        napi_get_boolean(env, false, &result);
    }
    return result;
}

static napi_value DeviceAddAccessoryRight(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_DEVICE)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_2;
    napi_value argv[PARAM_COUNT_2] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_2), OHEC_COMMON_PARAM_ERROR, "The function at least takes two argument.");

    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, argv[INDEX_0], &type), "Get args 1 type failed");
    USB_ASSERT(env, type == napi_number, OHEC_COMMON_PARAM_ERROR, "The type of tokenId must be number.");
    uint32_t tokenId;
    napi_get_value_uint32(env, argv[INDEX_0], &tokenId);

    napi_value accessoryObj = argv[INDEX_1];
    napi_valuetype type1;
    NAPI_CHECK(env, napi_typeof(env, accessoryObj, &type1), "Get accessoryObj type failed");
    USB_ASSERT(env, type1 == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of device must be USBAccessory.");
    USBAccessory accessory;
    ParseAccessoryObj(env, accessoryObj, accessory);

    int32_t ret = g_usbClient.AddAccessoryRight(tokenId, accessory);
    if (ret == UEC_OK) {
        return nullptr;
    } else if (ret == UEC_SERVICE_GET_TOKEN_INFO_FAILED) {
        ThrowBusinessError(env, OHEC_COMMON_PARAM_ERROR, "");
    } else if (ret == UEC_SERVICE_ACCESSORY_NOT_MATCH) {
        ThrowBusinessError(env, OHEC_COMMON_PARAM_ERROR,
            "Get accessory through getAccessoryList");
    } else if (ret == UEC_SERVICE_DATABASE_OPERATION_FAILED) {
        ThrowBusinessError(env, UEC_COMMON_RIGHT_DATABASE_ERROR,
            "Database request operation exception");
    } else {
        USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI),
            OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
        USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED),
            OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
        ThrowBusinessError(env, UEC_COMMON_SERVICE_EXCEPTION,
            "Service exception");
    }
    USB_HILOGD(MODULE_USB_NAPI, "Device call AddAccessoryRight ret: %{public}d", ret);
    return nullptr;
}

static napi_value DeviceAddAccessRight(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_2;
    napi_value argv[PARAM_COUNT_2] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_2), OHEC_COMMON_PARAM_ERROR, "The function at least takes two argument.");

    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, argv[INDEX_0], &type), "Get args 1 type failed");
    USB_ASSERT(env, type == napi_string, OHEC_COMMON_PARAM_ERROR, "The type of tokenId must be string.");
    std::string tokenId;
    NapiUtil::JsValueToString(env, argv[INDEX_0], STR_DEFAULT_SIZE, tokenId);

    NAPI_CHECK(env, napi_typeof(env, argv[INDEX_1], &type), "Get args 2 type failed");
    USB_ASSERT(env, type == napi_string, OHEC_COMMON_PARAM_ERROR, "The type of deviceName must be string.");
    std::string deviceName;
    NapiUtil::JsValueToString(env, argv[INDEX_1], STR_DEFAULT_SIZE, deviceName);

    napi_value result;
    int32_t ret = g_usbClient.AddAccessRight(tokenId, deviceName);
    USB_HILOGD(MODULE_USB_NAPI, "Device call AddRight ret: %{public}d", ret);
    if (ret == UEC_OK) {
        napi_get_boolean(env, true, &result);
    } else {
        USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI),
            OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
        USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED),
            OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
        napi_get_boolean(env, false, &result);
    }
    return result;
}

static napi_value DeviceRemoveRight(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, argv[INDEX_0], &type), "Get args 1 type failed");
    USB_ASSERT(env, type == napi_string, OHEC_COMMON_PARAM_ERROR, "The type of deviceName must be string.");
    std::string deviceName;
    NapiUtil::JsValueToString(env, argv[INDEX_0], STR_DEFAULT_SIZE, deviceName);

    napi_value result;
    int32_t ret = g_usbClient.RemoveRight(deviceName);
    USB_HILOGD(MODULE_USB_NAPI, "Device call RemoveRight ret: %{public}d", ret);
    if (ret == UEC_OK) {
        napi_get_boolean(env, true, &result);
    } else {
        napi_get_boolean(env, false, &result);
    }

    return result;
}

static napi_value DeviceCancelAccessoryRight(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_DEVICE)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_value accessoryObj = argv[INDEX_0];
    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, accessoryObj, &type), "Get accessoryObj type failed");
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of device must be USBAccessory.");
    USBAccessory accessory;
    ParseAccessoryObj(env, accessoryObj, accessory);

    if (g_accFd != 0) {
        close(g_accFd);
        g_accFd = 0;
        g_usbClient.CloseAccessory(g_accFd);
    }

    int32_t ret = g_usbClient.CancelAccessoryRight(accessory);
    if (ret == UEC_OK) {
        return nullptr;
    } else if (ret == UEC_SERVICE_ACCESSORY_NOT_MATCH) {
        ThrowBusinessError(env, UEC_ACCESSORY_NOT_MATCH,
            "Get accessory through getAccessoryList");
    } else if (ret == UEC_SERVICE_DATABASE_OPERATION_FAILED) {
        ThrowBusinessError(env, UEC_COMMON_RIGHT_DATABASE_ERROR,
            "Database request operation exception");
    } else {
        ThrowBusinessError(env, UEC_COMMON_SERVICE_EXCEPTION,
            "Service exception");
    }
    USB_HILOGD(MODULE_USB_NAPI, "Device call RemoveRight ret: %{public}d", ret);
    return nullptr;
}

static napi_value CoreHasRight(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value args[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, args[INDEX_0], &type), "Get args 1 type failed");
    USB_ASSERT(env, type == napi_string, OHEC_COMMON_PARAM_ERROR, "The type of deviceName must be string");
    std::string deviceName;
    NapiUtil::JsValueToString(env, args[INDEX_0], STR_DEFAULT_SIZE, deviceName);

    bool result = g_usbClient.HasRight(deviceName);
    USB_HILOGD(MODULE_USB_NAPI, "client called result %{public}d", result);

    napi_value napiValue = nullptr;
    napi_get_boolean(env, result, &napiValue);

    return napiValue;
}

static napi_value DeviceHasAccessoryRight(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_DEVICE)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value args[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_value accessoryObj = args[INDEX_0];
    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, accessoryObj, &type), "Get accessoryObj type failed");
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of device must be USBAccessory.");
    USBAccessory accessory;
    ParseAccessoryObj(env, accessoryObj, accessory);
    bool result = false;
    int32_t ret = g_usbClient.HasAccessoryRight(accessory, result);
    if (ret == UEC_OK) {
        napi_value napiValue = nullptr;
        napi_get_boolean(env, result, &napiValue);
        return napiValue;
    } else if (ret == UEC_SERVICE_ACCESSORY_NOT_MATCH) {
        ThrowBusinessError(env, UEC_ACCESSORY_NOT_MATCH,
            "Get accessory through getAccessoryList");
    } else if (ret == UEC_SERVICE_DATABASE_OPERATION_FAILED) {
        ThrowBusinessError(env, UEC_COMMON_RIGHT_DATABASE_ERROR,
            "Database request operation exception");
    } else {
        ThrowBusinessError(env, UEC_COMMON_SERVICE_EXCEPTION,
            "Service exception");
    }

    return nullptr;
}

static auto g_requestRightExecute = [](napi_env env, void *data) {
    USBRightAsyncContext *asyncContext = reinterpret_cast<USBRightAsyncContext *>(data);
    int32_t ret = g_usbClient.RequestRight(asyncContext->deviceName);
    if (ret == UEC_OK) {
        asyncContext->status = napi_ok;
    } else {
        asyncContext->status = napi_generic_failure;
    }
};

static auto g_requestRightComplete = [](napi_env env, napi_status status, void *data) {
    USBRightAsyncContext *asyncContext = reinterpret_cast<USBRightAsyncContext *>(data);
    napi_value queryResult = nullptr;
    napi_get_boolean(env, asyncContext->status == napi_ok, &queryResult);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, queryResult);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
};

static napi_value CoreRequestRight(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value args[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, args[INDEX_0], &type), "Get args 1 type failed");
    USB_ASSERT(env, type == napi_string, OHEC_COMMON_PARAM_ERROR, "The type of deviceName must be string.");
    std::string deviceName;
    NapiUtil::JsValueToString(env, args[INDEX_0], STR_DEFAULT_SIZE, deviceName);

    auto asyncContext = new (std::nothrow) USBRightAsyncContext();
    if (asyncContext == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "Create USBRightAsyncContext failed.");
        return nullptr;
    }

    asyncContext->env = env;
    asyncContext->deviceName = deviceName;

    napi_value result = nullptr;
    napi_create_promise(env, &asyncContext->deferred, &result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "RequestRight", NAPI_AUTO_LENGTH, &resource);

    napi_create_async_work(env, nullptr, resource, g_requestRightExecute, g_requestRightComplete,
        reinterpret_cast<void *>(asyncContext), &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);

    return result;
}

static auto g_requestAccessoryRightExecute = [](napi_env env, void *data) {
    if (data == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create async work, data is nullptr");
        return;
    }
    USBAccessoryRightAsyncContext *asyncContext = reinterpret_cast<USBAccessoryRightAsyncContext *>(data);
    bool result = false;
    asyncContext->errCode = g_usbClient.RequestAccessoryRight(asyncContext->accessory, result);
    asyncContext->hasRight = result;
};

static auto g_requestAccessoryRightComplete = [](napi_env env, napi_status status, void *data) {
    if (data == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create async work, data is nullptr");
        return;
    }
    USBAccessoryRightAsyncContext *asyncContext = reinterpret_cast<USBAccessoryRightAsyncContext *>(data);
    napi_value queryResult = nullptr;

    if (asyncContext->errCode == UEC_OK) {
        asyncContext->status = napi_ok;
        napi_get_boolean(env, asyncContext->hasRight, &queryResult);
    } else if (asyncContext->errCode == UEC_SERVICE_ACCESSORY_NOT_MATCH) {
        asyncContext->status = napi_generic_failure;
        queryResult = CreateBusinessError(env, UEC_ACCESSORY_NOT_MATCH, "");
    } else if (asyncContext->errCode == UEC_SERVICE_DATABASE_OPERATION_FAILED) {
        asyncContext->status = napi_generic_failure;
        queryResult = CreateBusinessError(env, UEC_COMMON_RIGHT_DATABASE_ERROR, "");
    } else {
        asyncContext->status = napi_generic_failure;
        queryResult = CreateBusinessError(env, UEC_COMMON_SERVICE_EXCEPTION, "");
    }
    ProcessPromise(env, *asyncContext, queryResult);
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
    asyncContext = nullptr;
};

static napi_value DeviceRequestAccessoryRight(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_DEVICE)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value args[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_value accessoryObj = args[INDEX_0];
    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, accessoryObj, &type), "Get accessoryObj type failed");
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of device must be USBAccessory.");
    USBAccessory accessory;
    ParseAccessoryObj(env, accessoryObj, accessory);

    auto asyncContext = new (std::nothrow) USBAccessoryRightAsyncContext();
    if (asyncContext == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "Create USBAccessoryRightAsyncContext failed.");
        return nullptr;
    }

    asyncContext->env = env;
    asyncContext->accessory = accessory;

    napi_value result = nullptr;
    napi_create_promise(env, &asyncContext->deferred, &result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "RequestRight", NAPI_AUTO_LENGTH, &resource);

    napi_create_async_work(env, nullptr, resource, g_requestAccessoryRightExecute, g_requestAccessoryRightComplete,
        reinterpret_cast<void *>(asyncContext), &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);

    return result;
}

static napi_value CoreUsbFunctionsFromString(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_DEVICE)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};

    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, argv[INDEX_0], &type), "Get args 1 type failed");
    USB_ASSERT(env, type == napi_string, OHEC_COMMON_PARAM_ERROR, "The type of funcs must be string.");

    // get value string argument of napi converted.
    std::string funcs;
    NapiUtil::JsValueToString(env, argv[INDEX_0], STR_DEFAULT_SIZE, funcs);

    int32_t numFuncs = g_usbClient.UsbFunctionsFromString(funcs);
    USB_HILOGI(MODULE_USB_NAPI, "usb functions from string failed ret = %{public}d", numFuncs);
    USB_ASSERT_RETURN_UNDEF(env, (numFuncs != UEC_SERVICE_PERMISSION_DENIED_SYSAPI),
        OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
    USB_ASSERT_RETURN_UNDEF(env, (numFuncs != UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED),
        OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
    napi_value result;
    napi_create_int32(env, numFuncs, &result);

    return result;
}

static napi_value CoreUsbFunctionsToString(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_DEVICE)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};

    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, argv[INDEX_0], &type), "Get args 1 type failed");
    USB_ASSERT(env, type == napi_number, OHEC_COMMON_PARAM_ERROR, "The type of funcs must be number.");

    int32_t funcs;
    napi_get_value_int32(env, argv[INDEX_0], &funcs);
    std::string strFuncs = g_usbClient.UsbFunctionsToString(funcs);
    USB_ASSERT_RETURN_UNDEF(env, (strFuncs != PERMISSION_DENIED_SYSAPI), OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
    USB_ASSERT_RETURN_UNDEF(env, (strFuncs != SYS_APP_PERMISSION_DENIED_SYSAPI),
        OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
    napi_value result;
    napi_create_string_utf8(env, strFuncs.c_str(), NAPI_AUTO_LENGTH, &result);

    return result;
}

static auto g_setCurrentFunctionExecute = [](napi_env env, void *data) {
    USBFunctionAsyncContext *asyncContext = reinterpret_cast<USBFunctionAsyncContext *>(data);
    int32_t ret = g_usbClient.SetCurrentFunctions(asyncContext->functions);
    asyncContext->errCode = ret;
};

static auto g_setCurrentFunctionComplete = [](napi_env env, napi_status status, void *data) {
    USBFunctionAsyncContext *asyncContext = reinterpret_cast<USBFunctionAsyncContext *>(data);
    napi_value queryResult = nullptr;

    if (asyncContext->errCode == UEC_OK) {
        asyncContext->status = napi_ok;
        napi_get_boolean(env, true, &queryResult);
    } else if (asyncContext->errCode == UEC_SERVICE_PERMISSION_DENIED_SYSAPI) {
        asyncContext->status = napi_generic_failure;
        queryResult = CreateBusinessError((env), OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
    } else if (asyncContext->errCode == UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED) {
        asyncContext->status = napi_generic_failure;
        queryResult = CreateBusinessError((env), OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
    } else if (asyncContext->errCode == UEC_SERVICE_PERMISSION_CHECK_HDC) {
        asyncContext->status = napi_generic_failure;
        queryResult = CreateBusinessError((env), UEC_COMMON_HDC_NOT_ALLOWED, "");
    } else if (asyncContext->errCode == UEC_SERVICE_FUNCTION_NOT_SUPPORT) {
        asyncContext->status = napi_generic_failure;
        queryResult = CreateBusinessError((env), UEC_COMMON_FUNCTION_NOT_SUPPORT, "");
    } else {
        asyncContext->status = napi_generic_failure;
        napi_get_boolean(env, false, &queryResult);
    }
    ProcessPromise(env, *asyncContext, queryResult);
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
};

static napi_value CoreSetCurrentFunctions(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_DEVICE)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};

    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, argv[INDEX_0], &type), "Get args 1 type failed");
    USB_ASSERT(env, type == napi_number, OHEC_COMMON_PARAM_ERROR, "The type of funcs must be number.");

    int32_t funcs = 0;
    napi_get_value_int32(env, argv[INDEX_0], &funcs);

    auto asyncContext = new (std::nothrow) USBFunctionAsyncContext();
    if (asyncContext == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "Create USBFunctionAsyncContext failed");
        return nullptr;
    }

    asyncContext->env = env;
    asyncContext->functions = funcs;
    napi_value result = nullptr;
    napi_create_promise(env, &asyncContext->deferred, &result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetCurrentFunctions", NAPI_AUTO_LENGTH, &resource);

    napi_create_async_work(env, nullptr, resource, g_setCurrentFunctionExecute, g_setCurrentFunctionComplete,
        reinterpret_cast<void *>(asyncContext), &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);

    return result;
}

static napi_value CoreGetCurrentFunctions(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_DEVICE)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc == PARAM_COUNT_0), OHEC_COMMON_PARAM_ERROR, "The function takes no arguments.");

    int32_t cfuncs;
    int32_t ret = g_usbClient.GetCurrentFunctions(cfuncs);
    napi_value result;
    USB_HILOGI(MODULE_USB_NAPI, "get current functions failed ret = %{public}d", ret);
    USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI), OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
    USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED),
        OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
    if (ret != UEC_OK) {
        napi_get_undefined(env, &result);
        USB_HILOGE(MODULE_USB_NAPI, "end call get ports failed ret : %{public}d", ret);
        return result;
    }
    napi_create_int32(env, cfuncs, &result);

    return result;
}

static napi_value CoreGetPorts(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_PORT)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc == PARAM_COUNT_0), OHEC_COMMON_PARAM_ERROR, "The function takes no arguments.");

    std::vector<UsbPort> ports;
    int32_t ret = g_usbClient.GetPorts(ports);
    napi_value result;
    USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI), OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
    USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED),
        OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
    if (ret != UEC_OK) {
        napi_get_undefined(env, &result);
        USB_HILOGE(MODULE_USB_NAPI, "end call get ports failed ret : %{public}d", ret);
        return result;
    }

    napi_create_array(env, &result);
    for (uint32_t i = 0; i < ports.size(); ++i) {
        napi_value port;
        napi_create_object(env, &port);
        NapiUtil::SetValueInt32(env, "id", ports[i].id, port);
        NapiUtil::SetValueInt32(env, "supportedModes", ports[i].supportedModes, port);
        napi_value usbPortStatus;
        napi_create_object(env, &usbPortStatus);
        NapiUtil::SetValueInt32(env, "currentMode", ports[i].usbPortStatus.currentMode, usbPortStatus);
        NapiUtil::SetValueInt32(env, "currentPowerRole", ports[i].usbPortStatus.currentPowerRole, usbPortStatus);
        NapiUtil::SetValueInt32(env, "currentDataRole", ports[i].usbPortStatus.currentDataRole, usbPortStatus);
        napi_set_named_property(env, port, "status", usbPortStatus);
        napi_set_element(env, result, i, port);
    }

    return result;
}

/* ============================================= Usb Port ============================================= */

static napi_value PortGetSupportedModes(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_PORT)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value args[PARAM_COUNT_1] = {nullptr};

    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");

    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, args[INDEX_0], &type), "Get args 1 type failed");
    USB_ASSERT(env, type == napi_number, OHEC_COMMON_PARAM_ERROR, "The type of portId must be number.");

    int32_t id = 0;
    int32_t result = 0;
    napi_get_value_int32(env, args[INDEX_0], &id);
    int32_t ret = g_usbClient.GetSupportedModes(id, result);
    USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI), OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
    USB_ASSERT_RETURN_UNDEF(env, (ret != UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED),
        OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");

    if (ret) {
        USB_HILOGD(MODULE_USB_NAPI, "false ret = %{public}d", ret);
    }
    napi_value napiValue = nullptr;
    NAPI_CHECK(env, napi_create_int32(env, result, &napiValue), "Create int32 failed");

    return napiValue;
}

static auto g_setPortRoleExecute = [](napi_env env, void *data) {
    USBPortRoleAsyncContext *asyncContext = reinterpret_cast<USBPortRoleAsyncContext *>(data);
    int32_t ret = g_usbClient.SetPortRole(asyncContext->portId, asyncContext->powerRole, asyncContext->dataRole);
    asyncContext->errCode = ret;
};

static auto g_setPortRoleComplete = [](napi_env env, napi_status status, void *data) {
    USBPortRoleAsyncContext *asyncContext = reinterpret_cast<USBPortRoleAsyncContext *>(data);
    napi_value queryResult = nullptr;

    if (asyncContext->errCode == UEC_OK) {
        asyncContext->status = napi_ok;
        napi_get_boolean(env, true, &queryResult);
    } else if (asyncContext->errCode == UEC_SERVICE_PERMISSION_DENIED_SYSAPI) {
        asyncContext->status = napi_generic_failure;
        queryResult = CreateBusinessError((env), OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
    } else if (asyncContext->errCode == UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED) {
        asyncContext->status = napi_generic_failure;
        queryResult = CreateBusinessError((env), OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
    } else if (asyncContext->errCode == UEC_SERVICE_NOT_SUPPORT_SWITCH_PORT) {
        asyncContext->status = napi_generic_failure;
        queryResult = CreateBusinessError((env), UEC_COMMON_PORTROLE_SWITCH_NOT_ALLOWED, "");
    } else {
        asyncContext->status = napi_generic_failure;
        napi_get_boolean(env, false, &queryResult);
    }
    ProcessPromise(env, *asyncContext, queryResult);
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
};

static napi_value PortSetPortRole(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_PORT)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_3;
    napi_value args[PARAM_COUNT_3] = {nullptr};

    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_3), OHEC_COMMON_PARAM_ERROR, "The function at least takes three arguments.");

    napi_valuetype type;
    NAPI_CHECK(env, napi_typeof(env, args[INDEX_0], &type), "Get args 1 type failed");
    USB_ASSERT(env, type == napi_number, OHEC_COMMON_PARAM_ERROR, "The type of portId must be number.");
    NAPI_CHECK(env, napi_typeof(env, args[INDEX_1], &type), "Get args 2 type failed");
    USB_ASSERT(env, type == napi_number, OHEC_COMMON_PARAM_ERROR, "The type of powerRole must be number.");
    NAPI_CHECK(env, napi_typeof(env, args[INDEX_2], &type), "Get args 3 type failed");
    USB_ASSERT(env, type == napi_number, OHEC_COMMON_PARAM_ERROR, "The type of dataRole must be number.");

    int32_t id = 0;
    napi_get_value_int32(env, args[INDEX_0], &id);
    int32_t powerRole = 0;
    napi_get_value_int32(env, args[INDEX_1], &powerRole);
    int32_t dataRole = 0;
    napi_get_value_int32(env, args[INDEX_2], &dataRole);

    auto asyncContext = new (std::nothrow) USBPortRoleAsyncContext();
    if (asyncContext == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "Create USBPortRoleAsyncContext failed");
        return nullptr;
    }

    asyncContext->env = env;
    asyncContext->portId = id;
    asyncContext->dataRole = dataRole;
    asyncContext->powerRole = powerRole;

    napi_value result = nullptr;
    napi_create_promise(env, &asyncContext->deferred, &result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "PortSetPortRole", NAPI_AUTO_LENGTH, &resource);

    napi_create_async_work(env, nullptr, resource, g_setPortRoleExecute, g_setPortRoleComplete,
        reinterpret_cast<void *>(asyncContext), &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);

    return result;
}

static napi_value PipeClaimInterface(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_3;
    napi_value argv[PARAM_COUNT_3] = {nullptr};

    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_2), OHEC_COMMON_PARAM_ERROR,
        "The function at least takes two arguments.");

    napi_value obj = argv[INDEX_0];
    napi_valuetype type;
    napi_typeof(env, obj, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");

    USBDevicePipe pipe;
    ParseUsbDevicePipe(env, obj, pipe);

    UsbInterface interface;
    napi_value obj2 = argv[INDEX_1];
    napi_typeof(env, obj2, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of iface must be USBInterface.");
    ParseInterfaceObj(env, obj2, interface);

    bool isForce = false;
    if (argc >= PARAM_COUNT_3) {
        napi_typeof(env, argv[INDEX_2], &type);
        if (type == napi_boolean) {
            napi_get_value_bool(env, argv[INDEX_2], &isForce);
        } else {
            USB_HILOGW(MODULE_USB_NAPI, "The type of force must be boolean.");
        }
    }

    int32_t ret = pipe.ClaimInterface(interface, isForce);
    USB_HILOGD(MODULE_USB_NAPI, "pipe call ClaimInterface ret: %{public}d", ret);
    napi_value result;
    napi_create_int32(env, ret, &result);

    return result;
}

static napi_value PipeReleaseInterface(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_2;
    napi_value argv[PARAM_COUNT_2] = {nullptr};

    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_2), OHEC_COMMON_PARAM_ERROR, "The function at least takes two arguments.");

    napi_value obj = argv[INDEX_0];
    napi_valuetype type;
    napi_typeof(env, obj, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");

    USBDevicePipe pipe;
    ParseUsbDevicePipe(env, obj, pipe);

    UsbInterface interface;
    napi_value obj2 = argv[INDEX_1];
    napi_typeof(env, obj2, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of iface must be USBInterface.");
    ParseInterfaceObj(env, obj2, interface);
    int32_t ret = pipe.ReleaseInterface(interface);
    USB_HILOGD(MODULE_USB_NAPI, "pipe call PipeReleaseInterface ret: %{public}d", ret);
    napi_value result;
    napi_create_int32(env, ret, &result);

    return result;
}

static napi_value PipeSetInterface(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_2;
    napi_value argv[PARAM_COUNT_2] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_2), OHEC_COMMON_PARAM_ERROR, "The function at least takes two arguments.");

    napi_value pipeObj = argv[INDEX_0];
    napi_valuetype type;
    napi_typeof(env, pipeObj, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");

    USBDevicePipe pipe;
    ParseUsbDevicePipe(env, pipeObj, pipe);

    napi_value interfaceObj = argv[INDEX_1];
    napi_typeof(env, interfaceObj, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of iface must be USBInterface.");

    UsbInterface interface;
    ParseInterfaceObj(env, interfaceObj, interface);
    int32_t ret = g_usbClient.SetInterface(pipe, interface);
    napi_value result;
    napi_create_int32(env, ret, &result);

    return result;
}

static napi_value PipeSetConfiguration(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_2;
    napi_value argv[PARAM_COUNT_2] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_2), OHEC_COMMON_PARAM_ERROR, "The function at least takes two arguments.");

    napi_valuetype type;
    napi_value pipeObj = argv[INDEX_0];

    napi_typeof(env, pipeObj, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");
    USBDevicePipe pipe;
    ParseUsbDevicePipe(env, pipeObj, pipe);

    napi_value configObj = argv[INDEX_1];
    napi_typeof(env, configObj, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of config must be USBConfig.");
    USBConfig config;
    ParseConfigObj(env, configObj, config);

    int32_t ret = g_usbClient.SetConfiguration(pipe, config);
    napi_value result;
    napi_create_int32(env, ret, &result);

    return result;
}

static napi_value PipeGetRawDescriptors(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};

    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");
    napi_value obj = argv[INDEX_0];
    napi_valuetype type;
    napi_typeof(env, obj, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");

    USBDevicePipe pipe;
    ParseUsbDevicePipe(env, obj, pipe);

    napi_value result;
    std::vector<uint8_t> bufferData;
    int32_t ret = g_usbClient.GetRawDescriptors(pipe, bufferData);
    if (ret == UEC_OK) {
        NapiUtil::Uint8ArrayToJsValue(env, bufferData, bufferData.size(), result);
    } else {
        napi_get_undefined(env, &result);
    }

    return result;
}

static napi_value PipeGetFileDescriptor(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};

    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");
    napi_value obj = argv[INDEX_0];
    napi_valuetype type;
    napi_typeof(env, obj, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");

    USBDevicePipe pipe;
    ParseUsbDevicePipe(env, obj, pipe);

    int32_t fd = -1;
    napi_value result;
    g_usbClient.GetFileDescriptor(pipe, fd);
    napi_create_int32(env, fd, &result);

    return result;
}

static auto g_controlTransferExecute = [](napi_env env, void *data) {
    USBControlTransferAsyncContext *asyncContext = (USBControlTransferAsyncContext *)data;
    std::vector<uint8_t> bufferData(asyncContext->buffer, asyncContext->buffer + asyncContext->bufferLength);
    if ((asyncContext->reqType & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_OUT) {
        delete[] asyncContext->buffer;
        asyncContext->buffer = nullptr;
    }

    const UsbCtrlTransfer tctrl = {
        asyncContext->reqType, asyncContext->request, asyncContext->value, asyncContext->index, asyncContext->timeOut};
    int32_t ret;
    size_t bufLen = bufferData.size();
    do {
        ret = asyncContext->pipe.ControlTransfer(tctrl, bufferData);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_NAPI, "ControlTransferExecute failed");
            break;
        }

        if ((asyncContext->reqType & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_IN &&
            asyncContext->bufferLength > 0 && bufferData.size() > 0) {
            bufLen = bufferData.size();
            if (bufLen > asyncContext->bufferLength) {
                USB_HILOGW(MODULE_USB_NAPI, "read warn, expect read len: %{public}u, actualLength: %{public}zu",
                    asyncContext->bufferLength, bufLen);
                bufLen = asyncContext->bufferLength;
            }
            ret = memcpy_s(asyncContext->buffer, asyncContext->bufferLength, bufferData.data(), bufLen);
        }
    } while (0);

    if (ret == UEC_OK) {
        asyncContext->status = napi_ok;
        asyncContext->dataSize = bufLen;
    } else {
        asyncContext->status = napi_generic_failure;
        asyncContext->dataSize = 0;
    }
};

static auto g_controlTransferComplete = [](napi_env env, napi_status status, void *data) {
    USBControlTransferAsyncContext *asyncContext = reinterpret_cast<USBControlTransferAsyncContext *>(data);
    napi_value queryResult = nullptr;

    if (asyncContext->status == napi_ok) {
        napi_create_int32(env, asyncContext->dataSize, &queryResult);
    } else {
        USB_HILOGD(MODULE_USB_NAPI, "ControlTransfer failed");
        napi_create_int32(env, -1, &queryResult);
    }
    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, queryResult);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
};

static std::tuple<bool, USBDevicePipe, PipeControlParam, int32_t> GetControlTransferParam(
    napi_env env, napi_callback_info info)
{
    size_t argc = PARAM_COUNT_3;
    napi_value argv[PARAM_COUNT_3] = {nullptr};
    napi_status status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "ControlTransfer failed to get cb info");
        return {false, {}, {}, {}};
    }

    if (argc < PARAM_COUNT_2) {
        USB_HILOGE(MODULE_USB_NAPI, "The function at least takes two arguments.");
        ThrowBusinessError(env, OHEC_COMMON_PARAM_ERROR, "The function at least takes two arguments.");
        return {false, {}, {}, {}};
    }

    // pipe param
    napi_valuetype type;
    napi_typeof(env, argv[INDEX_0], &type);
    if (type != napi_object) {
        USB_HILOGE(MODULE_USB_NAPI, "index 0 wrong argument type, object expected.");
        ThrowBusinessError(env, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");
        return {false, {}, {}, {}};
    }

    USBDevicePipe pipe;
    ParseUsbDevicePipe(env, argv[INDEX_0], pipe);

    // control params
    PipeControlParam controlParam = {0};
    bool ret = ParsePipeControlParam(env, argv[INDEX_1], controlParam);
    if (!ret) {
        USB_HILOGE(MODULE_USB_NAPI, "index 1 wrong argument type, object expected.");
        return {false, {}, {}, {}};
    }

    // timeOut param
    int32_t timeOut = 0;
    if (argc > PARAM_COUNT_2) {
        napi_typeof(env, argv[INDEX_2], &type);
        if (type == napi_number) {
            napi_get_value_int32(env, argv[INDEX_2], &timeOut);
        } else {
            USB_HILOGW(MODULE_USB_NAPI, "index 2 wrong argument type, number expected.");
        }
    }

    return {true, pipe, controlParam, timeOut};
}

static napi_value PipeControlTransfer(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    auto [res, pipe, controlParam, timeOut] = GetControlTransferParam(env, info);
    if (!res) {
        USB_HILOGE(MODULE_USB_NAPI, "GetControlTransferParam failed.");
        return nullptr;
    }

    auto asyncContext = new (std::nothrow) USBControlTransferAsyncContext();
    if (asyncContext == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "New USBControlTransferAsyncContext failed.");
        return nullptr;
    }

    asyncContext->env = env;
    asyncContext->pipe = pipe;
    asyncContext->request = controlParam.request;
    asyncContext->target = controlParam.target;
    asyncContext->reqType = controlParam.reqType;
    asyncContext->value = controlParam.value;
    asyncContext->index = controlParam.index;

    if ((asyncContext->reqType & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_OUT) {
        uint8_t *nativeArrayBuffer = nullptr;
        if (controlParam.dataLength > 0) {
            nativeArrayBuffer = new (std::nothrow) uint8_t[controlParam.dataLength];
            if (nativeArrayBuffer == nullptr) {
                delete asyncContext;
                return nullptr;
            }

            errno_t ret = memcpy_s(nativeArrayBuffer, controlParam.dataLength,
                controlParam.data, controlParam.dataLength);
            if (ret != EOK) {
                USB_HILOGE(MODULE_USB_NAPI, "memcpy_s failed");
                delete asyncContext;
                delete[] nativeArrayBuffer;
                return nullptr;
            }
        }
        asyncContext->buffer = nativeArrayBuffer;
    } else {
        asyncContext->buffer = controlParam.data;
    }

    asyncContext->bufferLength = controlParam.dataLength;
    asyncContext->timeOut = timeOut;
    napi_value result = nullptr;
    napi_create_promise(env, &asyncContext->deferred, &result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "PipeControlTransfer", NAPI_AUTO_LENGTH, &resource);

    napi_create_async_work(env, nullptr, resource, g_controlTransferExecute, g_controlTransferComplete,
        reinterpret_cast<void *>(asyncContext), &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);

    return result;
}

static auto g_usbControlTransferExecute = [](napi_env env, void *data) {
    USBDeviceControlTransferAsyncContext *asyncContext = (USBDeviceControlTransferAsyncContext *)data;
    std::vector<uint8_t> bufferData(asyncContext->buffer, asyncContext->buffer + asyncContext->bufferLength);
    if ((asyncContext->reqType & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_OUT && asyncContext->buffer != nullptr) {
        delete[] asyncContext->buffer;
        asyncContext->buffer = nullptr;
    }

    const UsbCtrlTransferParams tctrl = {asyncContext->reqType, asyncContext->request,
        asyncContext->value, asyncContext->index, asyncContext->length, asyncContext->timeOut};
    int32_t ret;
    size_t bufLen = bufferData.size();
    do {
        ret = asyncContext->pipe.UsbControlTransfer(tctrl, bufferData);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_NAPI, "ControlTransferExecute failed");
            break;
        }

        if ((asyncContext->reqType & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_IN &&
            asyncContext->bufferLength > 0 && bufferData.size() > 0) {
            bufLen = bufferData.size();
            if (bufLen > asyncContext->bufferLength) {
                USB_HILOGW(MODULE_USB_NAPI, "read warn, expect read len: %{public}u, actualLength: %{public}zu",
                    asyncContext->bufferLength, bufLen);
                bufLen = asyncContext->bufferLength;
            }
            ret = memcpy_s(asyncContext->buffer, asyncContext->bufferLength, bufferData.data(), bufLen);
        }
    } while (0);

    if (ret == UEC_OK) {
        asyncContext->status = napi_ok;
        asyncContext->dataSize = bufLen;
    } else {
        asyncContext->status = napi_generic_failure;
        asyncContext->dataSize = 0;
    }
};

static auto g_usbControlTransferComplete = [](napi_env env, napi_status status, void *data) {
    USBDeviceControlTransferAsyncContext *asyncContext = reinterpret_cast<USBDeviceControlTransferAsyncContext *>(data);
    napi_value queryResult = nullptr;

    if (asyncContext->status == napi_ok) {
        napi_create_int32(env, asyncContext->dataSize, &queryResult);
    } else {
        USB_HILOGD(MODULE_USB_NAPI, "usbControlTransfer failed");
        napi_create_int32(env, -1, &queryResult);
    }
    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, queryResult);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
};

static std::tuple<bool, USBDevicePipe, UsbPipeControlParam, int32_t> GetUsbControlTransferParam(
    napi_env env, napi_callback_info info)
{
    size_t argc = PARAM_COUNT_3;
    napi_value argv[PARAM_COUNT_3] = {nullptr};
    napi_status status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "ControlTransfer failed to get cb info");
        return {false, {}, {}, {}};
    }

    if (argc < PARAM_COUNT_2) {
        USB_HILOGE(MODULE_USB_NAPI, "The function at least takes two arguments.");
        ThrowBusinessError(env, OHEC_COMMON_PARAM_ERROR, "The function at least takes two arguments.");
        return {false, {}, {}, {}};
    }

    // pipe param
    napi_valuetype type;
    napi_typeof(env, argv[INDEX_0], &type);
    if (type != napi_object) {
        USB_HILOGE(MODULE_USB_NAPI, "index 0 wrong argument type, object expected.");
        ThrowBusinessError(env, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");
        return {false, {}, {}, {}};
    }

    USBDevicePipe pipe;
    ParseUsbDevicePipe(env, argv[INDEX_0], pipe);

    // control params
    napi_typeof(env, argv[INDEX_1], &type);
    if (type != napi_object) {
        USB_HILOGE(MODULE_USB_NAPI, "index 1 wrong argument type, object expected.");
        ThrowBusinessError(env, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDeviceRequestParams.");
        return {false, {}, {}, {}};
    }

    UsbPipeControlParam controlParam = {0};
    ParseUsbPipeControlParam(env, argv[INDEX_1], controlParam);

    // timeOut param
    int32_t timeOut = 0;
    if (argc > PARAM_COUNT_2) {
        napi_typeof(env, argv[INDEX_2], &type);
        if (type == napi_number) {
            napi_get_value_int32(env, argv[INDEX_2], &timeOut);
        } else {
            USB_HILOGW(MODULE_USB_NAPI, "index 2 wrong argument type, number expected.");
        }
    }

    return {true, pipe, controlParam, timeOut};
}

static napi_value PipeUsbControlTransfer(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    auto [res, pipe, controlParam, timeOut] = GetUsbControlTransferParam(env, info);
    if (!res) {
        USB_HILOGE(MODULE_USB_NAPI, "GetUsbControlTransferParam failed.");
        return nullptr;
    }

    auto asyncContext = new (std::nothrow) USBDeviceControlTransferAsyncContext();
    if (asyncContext == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "New USBDeviceControlTransferAsyncContext failed.");
        return nullptr;
    }

    asyncContext->env = env;
    asyncContext->pipe = pipe;
    asyncContext->reqType = controlParam.reqType;
    asyncContext->request = controlParam.request;
    asyncContext->value = controlParam.value;
    asyncContext->index = controlParam.index;
    asyncContext->length = controlParam.length;

    if ((asyncContext->reqType & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_OUT) {
        uint8_t *nativeArrayBuffer = nullptr;
        if (controlParam.dataLength > 0) {
            nativeArrayBuffer = new (std::nothrow) uint8_t[controlParam.dataLength];
            if (nativeArrayBuffer == nullptr) {
                delete asyncContext;
                return nullptr;
            }

            errno_t ret = memcpy_s(nativeArrayBuffer, controlParam.dataLength,
                controlParam.data, controlParam.dataLength);
            if (ret != EOK) {
                USB_HILOGE(MODULE_USB_NAPI, "memcpy_s failed");
                delete asyncContext;
                delete[] nativeArrayBuffer;
                return nullptr;
            }
        }
        asyncContext->buffer = nativeArrayBuffer;
    } else {
        asyncContext->buffer = controlParam.data;
    }

    asyncContext->bufferLength = controlParam.dataLength;
    asyncContext->timeOut = timeOut;
    napi_value result = nullptr;
    napi_create_promise(env, &asyncContext->deferred, &result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "PipeUsbControlTransfer", NAPI_AUTO_LENGTH, &resource);

    napi_create_async_work(env, nullptr, resource, g_usbControlTransferExecute, g_usbControlTransferComplete,
        reinterpret_cast<void *>(asyncContext), &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);

    return result;
}

static auto g_bulkTransferExecute = [](napi_env env, void *data) {
    USBBulkTransferAsyncContext *asyncContext = reinterpret_cast<USBBulkTransferAsyncContext *>(data);
    std::vector<uint8_t> bufferData(asyncContext->buffer, asyncContext->buffer + asyncContext->bufferLength);
    if (asyncContext->endpoint.GetDirection() == USB_ENDPOINT_DIR_OUT) {
        delete[] asyncContext->buffer;
        asyncContext->buffer = nullptr;
    }

    int32_t ret;
    size_t bufLen = bufferData.size();
    do {
        ret = asyncContext->pipe.BulkTransfer(asyncContext->endpoint, bufferData, asyncContext->timeOut);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_NAPI, "BulkTransferExecute failed");
            break;
        }

        if (asyncContext->endpoint.GetDirection() == USB_ENDPOINT_DIR_IN &&
            asyncContext->bufferLength > 0 && bufferData.size() > 0) {
            bufLen = bufferData.size();
            if (bufLen > asyncContext->bufferLength) {
                USB_HILOGW(MODULE_USB_NAPI, "read warn, expect read len: %{public}u, actualLength: %{public}zu",
                    asyncContext->bufferLength, bufLen);
                bufLen = asyncContext->bufferLength;
            }
            ret = memcpy_s(asyncContext->buffer, asyncContext->bufferLength, bufferData.data(), bufLen);
        }
    } while (0);

    USB_HILOGD(MODULE_USB_NAPI, "call pipe result %{public}d", ret);
    if (ret == UEC_OK) {
        asyncContext->status = napi_ok;
        asyncContext->dataSize = bufLen;
    } else {
        asyncContext->status = napi_generic_failure;
        asyncContext->dataSize = 0;
    }
};

static auto g_bulkTransferComplete = [](napi_env env, napi_status status, void *data) {
    USBBulkTransferAsyncContext *asyncContext = reinterpret_cast<USBBulkTransferAsyncContext *>(data);
    napi_value queryResult = nullptr;
    if (asyncContext->status == napi_ok) {
        napi_create_int32(env, asyncContext->dataSize, &queryResult);
    } else {
        USB_HILOGE(MODULE_USB_NAPI, "BulkTransfer failed");
        napi_create_int32(env, -1, &queryResult);
    }
    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, queryResult);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
};

static bool GetDescriptorOnBulkTransferParam(napi_env env, napi_value data,
    USBBulkTransferAsyncContext &asyncContext, const USBEndpoint &ep)
{
    uint8_t *buffer = nullptr;
    size_t offset = 0;
    size_t bufferSize = 0;
    bool hasBuffer = NapiUtil::JsUint8ArrayParseReserveZeroBuffer(env, data, &buffer, bufferSize, offset);
    if (!hasBuffer) {
        USB_HILOGE(MODULE_USB_NAPI, "BulkTransfer wrong argument, buffer is null");
        return false;
    }
    asyncContext.env = env;
    asyncContext.endpoint = ep;

    if (ep.GetDirection() == USB_ENDPOINT_DIR_OUT && bufferSize > 0) {
        uint8_t *nativeArrayBuffer = new (std::nothrow) uint8_t[bufferSize];
        RETURN_IF_WITH_RET(nativeArrayBuffer == nullptr, false);

        errno_t ret = memcpy_s(nativeArrayBuffer, bufferSize, buffer, bufferSize);
        if (ret != EOK) {
            USB_HILOGE(MODULE_USB_NAPI, "memcpy_s failed");
            delete[] nativeArrayBuffer;
            nativeArrayBuffer = nullptr;
            return false;
        }

        asyncContext.buffer = nativeArrayBuffer;
    } else {
        asyncContext.buffer = buffer;
    }
    asyncContext.bufferLength = bufferSize;
    return true;
}

static bool GetBulkTransferParams(napi_env env, napi_callback_info info, USBBulkTransferAsyncContext &asyncContext)
{
    size_t argc = PARAM_COUNT_4;
    napi_value argv[PARAM_COUNT_4] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT_RETURN_FALSE(
        env, (argc >= PARAM_COUNT_3), OHEC_COMMON_PARAM_ERROR,
        "The function at least takes three arguments.");

    napi_valuetype type;
    USBDevicePipe pipe;
    napi_typeof(env, argv[INDEX_0], &type);
    USB_ASSERT_RETURN_FALSE(
        env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");
    ParseUsbDevicePipe(env, argv[INDEX_0], pipe);
    asyncContext.pipe = pipe;

    USBEndpoint ep;
    napi_typeof(env, argv[INDEX_1], &type);
    USB_ASSERT_RETURN_FALSE(
        env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of endpoint must be USBEndpoint.");
    ParseEndpointObj(env, argv[INDEX_1], ep);

    int32_t timeOut = 0;
    if (argc > PARAM_COUNT_3) {
        napi_typeof(env, argv[INDEX_3], &type);
        if (type == napi_number) {
            napi_get_value_int32(env, argv[INDEX_3], &timeOut);
        } else {
            USB_HILOGW(MODULE_USB_NAPI, "The type of timeOut must be number.");
        }
    }

    if (!GetDescriptorOnBulkTransferParam(env, argv[INDEX_2], asyncContext, ep)) {
        USB_HILOGE(MODULE_USB_NAPI, "get asyncContext failed.");
        return false;
    }
    asyncContext.timeOut = timeOut;
    return true;
}

static napi_value PipeBulkTransfer(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    auto asyncContext = new (std::nothrow) USBBulkTransferAsyncContext();
    if (asyncContext == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "Create USBBulkTransferAsyncContext failed.");
        return nullptr;
    }

    napi_value result = nullptr;
    napi_create_promise(env, &asyncContext->deferred, &result);
    if (!GetBulkTransferParams(env, info, *asyncContext)) {
        USB_HILOGE(MODULE_USB_NAPI, "end call invalid arg");
        asyncContext->status = napi_invalid_arg;
        napi_value queryResult = nullptr;
        napi_create_int32(env, -1, &queryResult);
        if (asyncContext->deferred) {
            napi_resolve_deferred(env, asyncContext->deferred, queryResult);
        }
        delete asyncContext;
        asyncContext = nullptr;
        return result;
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "PipeBulkTransfer", NAPI_AUTO_LENGTH, &resource);

    napi_status status = napi_create_async_work(env, nullptr, resource, g_bulkTransferExecute, g_bulkTransferComplete,
        reinterpret_cast<void *>(asyncContext), &asyncContext->work);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "create async work failed");
        return result;
    }
    napi_queue_async_work(env, asyncContext->work);

    return result;
}

static bool ParseTransferParams(const napi_env &env, const napi_value &object,
    USBTransferAsyncContext *asyncContext)
{
    napi_value valuePipe = nullptr;
    USBDevicePipe pipe;
    napi_valuetype valueType;
    napi_get_named_property(env, object, "devPipe", &valuePipe);
    napi_typeof(env, valuePipe, &valueType);
    USB_ASSERT_RETURN_FALSE(env, valueType == napi_object, false, "The valueType of pipe must be USBDevicePipe.");
    ParseUsbDevicePipe(env, valuePipe, pipe);
    asyncContext->pipe = pipe;

    int32_t flags = static_cast<int32_t>(TransferFlagsJs::TRANSFER_FLAGS_UNKNOWN);
    NapiUtil::JsObjectToInt(env, object, "flags", flags);
    if (flags == static_cast<int32_t>(TransferFlagsJs::TRANSFER_FLAGS_UNKNOWN)) {
        USB_HILOGE(MODULE_USB_NAPI, "TRANSFER_FLAGS_UNKNOWN, flags: %{public}d", flags);
        return false;
    }
    asyncContext->flags = flags;
    NapiUtil::JsObjectToInt(env, object, "endpoint", asyncContext->endpoint);
    int32_t tranferType = static_cast<int32_t>(EndpointTransferTypeJs::TRANSFER_TYPE_UNKNOWN);
    NapiUtil::JsObjectToInt(env, object, "type", tranferType);
    if (tranferType == static_cast<int32_t>(EndpointTransferTypeJs::TRANSFER_TYPE_UNKNOWN)) {
        USB_HILOGE(MODULE_USB_NAPI, "TRANSFER_TYPE_UNKNOWN, tranferType: %{public}d", tranferType);
        return false;
    }
    asyncContext->type = tranferType;
    NapiUtil::JsObjectToInt(env, object, "timeout", asyncContext->timeOut);
    NapiUtil::JsObjectToInt(env, object, "length", asyncContext->length);
    napi_value valueCallBack;
    NapiUtil::JsObjectGetProperty(env, object, "callback", valueCallBack);
    napi_typeof(env, valueCallBack, &valueType);
    USB_ASSERT_RETURN_FALSE(env, valueType == napi_function, false, "The type of endpoint must be function.");
    napi_create_reference(env, valueCallBack, 1, &asyncContext->callbackRef);

    napi_value valueUint8Array;
    NapiUtil::JsObjectGetProperty(env, object, "buffer", valueUint8Array);

    size_t offset = 0;
    bool hasBuffer = NapiUtil::JsUint8ArrayParseReserveZeroBuffer(env, valueUint8Array, &asyncContext->buffer,
        asyncContext->bufferLength, offset);
    if (!hasBuffer) {
        USB_HILOGE(MODULE_USB_NAPI, "Transfer wrong argument, buffer is null");
        return false;
    }
    NapiUtil::JsObjectToUint(env, object, "isoPacketCount", asyncContext->numIsoPackets);
    return true;
}

static bool GetTransferParamsFromJsObj(const napi_env &env, const napi_callback_info &info,
    USBTransferAsyncContext *asyncContext)
{
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT_RETURN_FALSE(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR,
        "The function at least takes one arguments.");

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[INDEX_0], &valueType);
    USB_ASSERT_RETURN_FALSE(
        env, valueType == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDataTransfer type.");

    bool parseResult = ParseTransferParams(env, argv[INDEX_0], asyncContext);
    if (!parseResult) {
        USB_HILOGE(MODULE_USB_NAPI, "Transfer wrong params, ParseTransferParams faied");
        return false;
    }

    return true;
}

static napi_value ParmsInput(napi_env env, AsyncCallBackContext &asyncCBWork)
{
    napi_value res = nullptr;
    napi_create_object(env, &res);

    napi_value target = nullptr;
    napi_create_object(env, &target);
    napi_value status = nullptr;
    napi_create_int32(env, asyncCBWork.status, &status);

    napi_value actualLength = nullptr;
    napi_create_int32(env, asyncCBWork.actualLength, &actualLength);

    napi_set_named_property(env, res, "status", status);
    napi_set_named_property(env, res, "actualLength", actualLength);

    if (asyncCBWork.isoInfo.empty()) {
        return res;
    }
    napi_value isoObjArray = nullptr;
    napi_create_array(env, &isoObjArray);
    const uint32_t isoCount = asyncCBWork.isoInfo.size();
    for (uint32_t i = 0; i < isoCount; i++) {
        napi_value iso = nullptr;
        napi_create_object(env, &iso);
        napi_value isoLength = nullptr;
        napi_value isoActualLength = nullptr;
        napi_value isoStatus = nullptr;
        napi_create_int32(env, asyncCBWork.isoInfo[i].isoLength, &isoLength);
        napi_create_int32(env, asyncCBWork.isoInfo[i].isoActualLength, &isoActualLength);
        napi_create_int32(env, asyncCBWork.isoInfo[i].isoStatus, &isoStatus);
        napi_set_named_property(env, iso, "length", isoLength);
        napi_set_named_property(env, iso, "actualLength", isoActualLength);
        napi_set_named_property(env, iso, "status", isoStatus);
        napi_set_element(env, isoObjArray, i, iso);
    }
    napi_set_named_property(env, res, "isoPacketDescs", isoObjArray);
    return res;
}

static int32_t ReadDataToBuffer(USBTransferAsyncContext *asyncContext, const TransferCallbackInfo &info)
{
    uint8_t endpointId = static_cast<uint8_t>(asyncContext->endpoint) & USB_ENDPOINT_DIR_MASK;
    size_t actBufLen = info.actualLength;
    if (endpointId == USB_ENDPOINT_DIR_IN && asyncContext->bufferLength > 0 && info.actualLength > 0) {
        asyncContext->ashmem->MapReadAndWriteAshmem();
        auto ashmemBuffer = asyncContext->ashmem->ReadFromAshmem(info.actualLength, 0);
        if (ashmemBuffer == nullptr) {
            asyncContext->ashmem->UnmapAshmem();
            asyncContext->ashmem->CloseAshmem();
            return actBufLen;
        }

        if (actBufLen > asyncContext->bufferLength) {
            USB_HILOGW(MODULE_USB_NAPI, "read warn, expect read len: %{public}zu, actualLength: %{public}zu",
                asyncContext->bufferLength, actBufLen);
            actBufLen = asyncContext->bufferLength;
        }
        int32_t ret = memcpy_s(asyncContext->buffer, asyncContext->bufferLength, ashmemBuffer, actBufLen);
        if (ret != EOK) {
            USB_HILOGE(MODULE_USB_NAPI, "memcpy_s fatal failed error: %{public}d", ret);
        }
    }
    asyncContext->ashmem->UnmapAshmem();
    asyncContext->ashmem->CloseAshmem();
    return actBufLen;
}

static void JsCallBack(USBTransferAsyncContext *asyncContext, const TransferCallbackInfo &info,
    const std::vector<HDI::Usb::V1_2::UsbIsoPacketDescriptor> &isoInfo)
{
    USB_HILOGI(MODULE_USB_NAPI, "JsCallBack enter.");
    int32_t actBufLen = ReadDataToBuffer(asyncContext, info);
    AsyncCallBackContext *asyncCBWork = new (std::nothrow) AsyncCallBackContext;
    if (asyncCBWork == nullptr) {
        delete asyncContext;
        asyncContext = nullptr;
        return;
    }
    asyncCBWork->env = asyncContext->env;
    asyncCBWork->actualLength = actBufLen;
    asyncCBWork->status = info.status;
    asyncCBWork->isoInfo = isoInfo;
    asyncCBWork->callbackRef = asyncContext->callbackRef;
    auto task = [asyncCBWork, asyncContext]() {
        std::shared_ptr<AsyncCallBackContext> context(
            static_cast<AsyncCallBackContext*>(asyncCBWork),
            [asyncContext](AsyncCallBackContext* ptr) {
                delete ptr;
                delete asyncContext;
            });
        napi_handle_scope scope;
        napi_open_handle_scope(asyncCBWork->env, &scope);
        napi_status res = napi_ok;
        napi_value resultJsCb;
        napi_get_reference_value(asyncCBWork->env, asyncCBWork->callbackRef, &resultJsCb);
        napi_value argv[2] = {nullptr};
        argv[1] = ParmsInput(asyncCBWork->env, *asyncCBWork);
        napi_value result;
        res = napi_call_function(asyncCBWork->env, nullptr, resultJsCb, PARAM_COUNT_2, argv, &result);
        if (res != napi_ok) {
            USB_HILOGE(MODULE_USB_NAPI, "napi_call_function failed, res: %{public}d", res);
        }
        napi_close_handle_scope(asyncCBWork->env, scope);
    };
    if (napi_status::napi_ok != napi_send_event(asyncCBWork->env, task, napi_eprio_immediate)) {
        USB_HILOGE(MODULE_USB_NAPI, "OnJsCallbackVolumeEvent: Failed to SendEvent");
        delete asyncCBWork;
        asyncCBWork = nullptr;
    }
}

static void GetUSBTransferInfo(USBTransferInfo &obj, USBTransferAsyncContext *asyncContext)
{
    obj.endpoint = asyncContext->endpoint;
    obj.type = asyncContext->type;
    obj.timeOut = asyncContext->timeOut;
    obj.length = asyncContext->length;
    obj.numIsoPackets = asyncContext->numIsoPackets;
    std::uintptr_t ptrValue = reinterpret_cast<std::uintptr_t>(asyncContext);
    obj.userData = static_cast<uint64_t>(ptrValue);
}

static bool CreateAndWriteAshmem(USBTransferAsyncContext *asyncContext, HDI::Usb::V1_2::USBTransferInfo &obj)
{
    StartTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_USB, "NAPI:Ashmem::CreateAshmem");
    int32_t bufLen = asyncContext->length <= 0 ? DEFAULT_SUBMIT_BUFFER_SIZE : asyncContext->length;
    asyncContext->ashmem = Ashmem::CreateAshmem(asyncContext->name.c_str(), bufLen);
    FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_USB);
    if (asyncContext->ashmem == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "Ashmem::CreateAshmem failed");
        return false;
    }
    uint8_t endpointId = static_cast<uint8_t>(asyncContext->endpoint) & USB_ENDPOINT_DIR_MASK;
    if (endpointId == USB_ENDPOINT_DIR_OUT && asyncContext->length > 0) {
        std::vector<uint8_t> bufferData(asyncContext->buffer, asyncContext->buffer + asyncContext->bufferLength);
        obj.length = static_cast<int32_t>(bufferData.size());
        asyncContext->ashmem->MapReadAndWriteAshmem();
        StartTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_USB, "NAPI:WriteToAshmem");
        if (!asyncContext->ashmem->WriteToAshmem(asyncContext->buffer, bufferData.size(), 0)) {
            FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_USB);
            asyncContext->ashmem->UnmapAshmem();
            asyncContext->ashmem->CloseAshmem();
            USB_HILOGE(MODULE_USB_NAPI, "napi UsbSubmitTransfer Failed to UsbSubmitTransfer to ashmem.");
            return false;
        }
        FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_USB);
    }
    return true;
}

static int32_t UsbSubmitTransferErrorCode(int32_t &error)
{
    switch (error) {
        case IO_ERROR:
            return USB_SUBMIT_TRANSFER_IO_ERROR;
        case INVALID_PARAM:
            return OHEC_COMMON_PARAM_ERROR;
        case NO_DEVICE:
            return USB_SUBMIT_TRANSFER_NO_DEVICE_ERROR;
        case NOT_FOUND:
            return USB_SUBMIT_TRANSFER_NOT_FOUND_ERROR;
        case ERROR_BUSY:
            return USB_SUBMIT_TRANSFER_RESOURCE_BUSY_ERROR;
        case NO_MEM:
            return USB_SUBMIT_TRANSFER_NO_MEM_ERROR;
        case UEC_SERVICE_PERMISSION_DENIED:
            return UEC_COMMON_HAS_NO_RIGHT;
        default:
            return USB_SUBMIT_TRANSFER_OTHER_ERROR;
    }
}

static napi_value UsbSubmitTransfer(napi_env env, napi_callback_info info)
{
    HITRACE_METER_NAME(HITRACE_TAG_USB, "NAPI:UsbSubmitTransfer");
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    auto timesUse = std::make_shared<TimesUse>();
    timesUse->beginTime = std::chrono::steady_clock::now();
    auto asyncContext = new (std::nothrow) USBTransferAsyncContext();
    if (asyncContext == nullptr) {
        return nullptr;
    }
    if (!GetTransferParamsFromJsObj(env, info, asyncContext)) {
        ThrowBusinessError(env, OHEC_COMMON_PARAM_ERROR, "BusinessError 401:Parameter error.");
        delete asyncContext;
        asyncContext = nullptr;
        return nullptr;
    }
    asyncContext->env = env;
    HDI::Usb::V1_2::USBTransferInfo obj;
    GetUSBTransferInfo(obj, asyncContext);
    if (obj.numIsoPackets > MAX_NUM_OF_ISO_PACKAGE || !CreateAndWriteAshmem(asyncContext, obj)) {
        delete asyncContext;
        asyncContext = nullptr;
        return nullptr;
    }
    static auto func = [] (const TransferCallbackInfo &info,
        const std::vector<HDI::Usb::V1_2::UsbIsoPacketDescriptor> &isoInfo, uint64_t userData) -> void {
        USBTransferAsyncContext *asyncContext = reinterpret_cast<USBTransferAsyncContext *>(userData);
        return JsCallBack(asyncContext, info, isoInfo);
    };
    StartTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_USB, "NAPI:UsbSubmitTransfer");
    int32_t ret = asyncContext->pipe.UsbSubmitTransfer(obj, func, asyncContext->ashmem);
    FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_USB);
    if (ret != napi_ok) {
        asyncContext->ashmem->UnmapAshmem();
        asyncContext->ashmem->CloseAshmem();
        delete asyncContext;
        asyncContext = nullptr;
        ret = UsbSubmitTransferErrorCode(ret);
        ThrowBusinessError(env, ret, "");
        return nullptr;
    }
    timesUse->endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timesUse->endTime - timesUse->beginTime);
    USB_HILOGE(MODULE_USB_NAPI, "UsbSubmitTransfer usedTime:%{public}lld ms", duration.count());
    return nullptr;
}

static bool ParseCancelParams(const napi_env &env, const napi_value &object,
    std::shared_ptr<USBTransferAsyncContext> asyncContext)
{
    auto timesUse = std::make_shared<TimesUse>();
    timesUse->beginTime = std::chrono::steady_clock::now();
    napi_value valuePipe = nullptr;
    USBDevicePipe pipe;
    napi_valuetype valueType;
    napi_get_named_property(env, object, "devPipe", &valuePipe);
    napi_typeof(env, valuePipe, &valueType);
    USB_ASSERT_RETURN_FALSE(env, valueType == napi_object, false, "The valueType of pipe must be USBDevicePipe.");
    ParseUsbDevicePipe(env, valuePipe, pipe);
    asyncContext->pipe = pipe;

    NapiUtil::JsObjectToInt(env, object, "endpoint", asyncContext->endpoint);
    timesUse->endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timesUse->endTime - timesUse->beginTime);
    USB_HILOGE(MODULE_USB_NAPI, "UsbCancelTransfer usedTime:%{public}lld ms", duration.count());
    return true;
}

static bool GetCancelParamsFromJsObj(const napi_env &env, const napi_callback_info &info,
    std::shared_ptr<USBTransferAsyncContext> asyncContext)
{
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};
    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT_RETURN_FALSE(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR,
        "The function at least takes one arguments.");

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[INDEX_0], &valueType);
    USB_ASSERT_RETURN_FALSE(
        env, valueType == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDataTransfer type.");

    bool parseResult = ParseCancelParams(env, argv[INDEX_0], asyncContext);
    if (!parseResult) {
        USB_HILOGE(MODULE_USB_NAPI, "Transfer wrong params, ParseCancelParams faied");
        return false;
    }

    return true;
}

static napi_value UsbCancelTransfer(napi_env env, napi_callback_info info)
{
    HITRACE_METER_NAME(HITRACE_TAG_USB, "NAPI:UsbCancelTransfer");
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    auto asyncContext = std::make_shared<USBTransferAsyncContext>();
    if (asyncContext == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "Create USBTransferAsyncContext failed.");
        return nullptr;
    }
    if (!GetCancelParamsFromJsObj(env, info, asyncContext)) {
        USB_HILOGE(MODULE_USB_NAPI, "end call invalid arg");
        ThrowBusinessError(env, OHEC_COMMON_PARAM_ERROR, "BusinessError 401:Parameter error.");
        return nullptr;
    }

    StartTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_USB, "NAPI:pipe.UsbCancelTransfer");
    int32_t ret = asyncContext->pipe.UsbCancelTransfer(asyncContext->endpoint);
    FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_USB);
    if (ret != napi_ok) {
        ret = UsbSubmitTransferErrorCode(ret);
        ThrowBusinessError(env, ret, "");
        return nullptr;
    }
    return nullptr;
}

static napi_value PipeResetDevice(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};

    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function at least takes one argument.");
    napi_value deciveObj = argv[INDEX_0];
    napi_valuetype type;
    napi_typeof(env, deciveObj, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");

    USBDevicePipe pipe;
    ParseUsbDevicePipe(env, deciveObj, pipe);

    napi_value napiValue;
    int32_t ret = g_usbClient.ResetDevice(pipe);
    if (ret == UEC_OK) {
        napi_get_boolean(env, true, &napiValue);
    } else if (ret == HDF_DEV_ERR_NO_DEVICE || ret == UEC_INTERFACE_NAME_NOT_FOUND) {
        ThrowBusinessError(env, USB_SUBMIT_TRANSFER_NO_DEVICE_ERROR,
            "Submit transfer no device.");
        napi_get_boolean(env, false, &napiValue);
    } else if (ret == UEC_SERVICE_PERMISSION_DENIED) {
        ThrowBusinessError(env, UEC_COMMON_HAS_NO_RIGHT,
            "No permission.");
        napi_get_boolean(env, false, &napiValue);
    } else if (ret == HDF_FAILURE) {
        ThrowBusinessError(env, USB_DEVICE_PIPE_CHECK_ERROR,
            "Check devicePipe failed.");
        napi_get_boolean(env, false, &napiValue);
    } else if (ret == UEC_SERVICE_INVALID_VALUE) {
        ThrowBusinessError(env, UEC_COMMON_SERVICE_EXCEPTION,
            "Service exception");
        napi_get_boolean(env, false, &napiValue);
    } else {
        ThrowBusinessError(env, USB_SUBMIT_TRANSFER_OTHER_ERROR,
            "Other USB error");
        napi_get_boolean(env, false, &napiValue);
    }
    return napiValue;
}

static void SetEnumProperty(napi_env env, napi_value object, const std::string &name, int32_t value)
{
    if (name.empty()) {
        USB_HILOGE(MODULE_USB_NAPI, "Property name cannot be an empty string");
        return;
    }

    napi_value tempValue = nullptr;
    napi_status status = napi_create_int32(env, value, &tempValue);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create int32 value for enum %{public}s", name.c_str());
        return;
    }
    status = napi_set_named_property(env, object, name.c_str(), tempValue);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to set property %{public}s", name.c_str());
        return;
    }
}

static napi_value NapiCreateFlagsEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "USB_TRANSFER_SHORT_NOT_OK", USB_TRANSFER_SHORT_NOT_OK);
    SetEnumProperty(env, object, "USB_TRANSFER_FREE_BUFFER", USB_TRANSFER_FREE_BUFFER);
    SetEnumProperty(env, object, "USB_TRANSFER_FREE_TRANSFER", USB_TRANSFER_FREE_TRANSFER);
    SetEnumProperty(env, object, "USB_TRANSFER_ADD_ZERO_PACKET", USB_TRANSFER_ADD_ZERO_PACKET);
    return object;
}

static napi_value NapiCreateTypeEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "TRANSFER_TYPE_ISOCHRONOUS", TRANSFER_TYPE_ISOCHRONOUS);
    SetEnumProperty(env, object, "TRANSFER_TYPE_BULK", TRANSFER_TYPE_BULK);
    SetEnumProperty(env, object, "TRANSFER_TYPE_INTERRUPT", TRANSFER_TYPE_INTERRUPT);
    return object;
}

static napi_value NapiCreateStatusEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "TRANSFER_COMPLETED", TRANSFER_COMPLETED);
    SetEnumProperty(env, object, "TRANSFER_ERROR", TRANSFER_ERROR);
    SetEnumProperty(env, object, "TRANSFER_TIMED_OUT", TRANSFER_TIMED_OUT);
    SetEnumProperty(env, object, "TRANSFER_CANCELED", TRANSFER_CANCELED);
    SetEnumProperty(env, object, "TRANSFER_STALL", TRANSFER_STALL);
    SetEnumProperty(env, object, "TRANSFER_NO_DEVICE", TRANSFER_NO_DEVICE);
    SetEnumProperty(env, object, "TRANSFER_OVERFLOW", TRANSFER_OVERFLOW);
    return object;
}

static napi_value PipeClose(napi_env env, napi_callback_info info)
{
    if (!HasFeature(FEATURE_HOST)) {
        ThrowBusinessError(env, CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    size_t argc = PARAM_COUNT_1;
    napi_value argv[PARAM_COUNT_1] = {nullptr};

    NAPI_CHECK(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Get call back info failed");
    USB_ASSERT(env, (argc >= PARAM_COUNT_1), OHEC_COMMON_PARAM_ERROR, "The function takes one argument.");

    napi_value obj = argv[INDEX_0];
    napi_valuetype type;
    napi_typeof(env, obj, &type);
    USB_ASSERT(env, type == napi_object, OHEC_COMMON_PARAM_ERROR, "The type of pipe must be USBDevicePipe.");

    USBDevicePipe pipe;
    ParseUsbDevicePipe(env, obj, pipe);
    int32_t ret = pipe.Close();
    napi_value result;
    napi_create_int32(env, ret, &result);

    return result;
}

static napi_value GetVersion(napi_env env, napi_callback_info info)
{
    auto version = g_usbClient.GetVersion();
    USB_HILOGD(MODULE_USB_NAPI, "version is %{public}s", version.c_str());
    napi_value result;
    napi_create_string_utf8(env, version.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value ToInt32Value(napi_env env, int32_t value)
{
    napi_value staticValue = nullptr;
    napi_create_int32(env, value, &staticValue);
    return staticValue;
}

static napi_value PowerRoleTypeEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "NONE", NONE);
    SetEnumProperty(env, object, "SOURCE", SOURCE);
    SetEnumProperty(env, object, "SINK", SINK);
    return object;
}

static napi_value DataRoleTypeEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "HOST", HOST);
    SetEnumProperty(env, object, "DEVICE", DEVICE);
    return object;
}

static napi_value PortModeTypeEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "UFP", UFP);
    SetEnumProperty(env, object, "DFP", DFP);
    SetEnumProperty(env, object, "DRP", DRP);
    SetEnumProperty(env, object, "NUM_MODES", NUM_MODES);
    return object;
}

static napi_value USBRequestTargetTypeEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "USB_REQUEST_TARGET_DEVICE", USB_REQUEST_TARGET_DEVICE);
    SetEnumProperty(env, object, "USB_REQUEST_TARGET_INTERFACE", USB_REQUEST_TARGET_INTERFACE);
    SetEnumProperty(env, object, "USB_REQUEST_TARGET_ENDPOINT", USB_REQUEST_TARGET_ENDPOINT);
    SetEnumProperty(env, object, "USB_REQUEST_TARGET_OTHER", USB_REQUEST_TARGET_OTHER);
    return object;
}

static napi_value USBControlRequestTypeEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "USB_REQUEST_TYPE_STANDARD", USB_REQUEST_TYPE_STANDARD);
    SetEnumProperty(env, object, "USB_REQUEST_TYPE_CLASS", USB_REQUEST_TYPE_CLASS);
    SetEnumProperty(env, object, "USB_REQUEST_TYPE_VENDOR", USB_REQUEST_TYPE_VENDOR);
    return object;
}

static napi_value USBRequestDirectionEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "USB_REQUEST_DIR_TO_DEVICE", USB_REQUEST_DIR_TO_DEVICE);
    SetEnumProperty(env, object, "USB_REQUEST_DIR_FROM_DEVICE", USB_REQUEST_DIR_FROM_DEVICE);
    return object;
}

static napi_value FunctionTypeEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "ACM", ACM);
    SetEnumProperty(env, object, "ECM", ECM);
    SetEnumProperty(env, object, "HDC", HDC);
    SetEnumProperty(env, object, "MTP", MTP);
    SetEnumProperty(env, object, "PTP", PTP);
    SetEnumProperty(env, object, "RNDIS", RNDIS);
    SetEnumProperty(env, object, "MIDI", MIDI);
    SetEnumProperty(env, object, "AUDIO_SOURCE", AUDIO_SOURCE);
    SetEnumProperty(env, object, "NCM", NCM);
    SetEnumProperty(env, object, "STORAGE", STORAGE);
    return object;
}

static napi_value DeclareEnum(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        /* Declare Enum PowerRoleType */
        DECLARE_NAPI_STATIC_PROPERTY("NONE", ToInt32Value(env, NONE)),
        DECLARE_NAPI_STATIC_PROPERTY("SOURCE", ToInt32Value(env, SOURCE)),
        DECLARE_NAPI_STATIC_PROPERTY("SINK", ToInt32Value(env, SINK)),
        DECLARE_NAPI_STATIC_PROPERTY("PowerRoleType", PowerRoleTypeEnum(env)),

        /* Declare Enum DataRoleType */
        DECLARE_NAPI_STATIC_PROPERTY("HOST", ToInt32Value(env, HOST)),
        DECLARE_NAPI_STATIC_PROPERTY("DEVICE", ToInt32Value(env, DEVICE)),
        DECLARE_NAPI_STATIC_PROPERTY("DataRoleType", DataRoleTypeEnum(env)),

        /* Declare Enum PortModeType */
        DECLARE_NAPI_STATIC_PROPERTY("UFP", ToInt32Value(env, UFP)),
        DECLARE_NAPI_STATIC_PROPERTY("DFP", ToInt32Value(env, DFP)),
        DECLARE_NAPI_STATIC_PROPERTY("DRP", ToInt32Value(env, DRP)),
        DECLARE_NAPI_STATIC_PROPERTY("NUM_MODES", ToInt32Value(env, NUM_MODES)),
        DECLARE_NAPI_STATIC_PROPERTY("PortModeType", PortModeTypeEnum(env)),

        /* Declare Enum USBRequestTargetType */
        DECLARE_NAPI_STATIC_PROPERTY("USB_REQUEST_TARGET_DEVICE", ToInt32Value(env, USB_REQUEST_TARGET_DEVICE)),
        DECLARE_NAPI_STATIC_PROPERTY("USB_REQUEST_TARGET_INTERFACE", ToInt32Value(env, USB_REQUEST_TARGET_INTERFACE)),
        DECLARE_NAPI_STATIC_PROPERTY("USB_REQUEST_TARGET_ENDPOINT", ToInt32Value(env, USB_REQUEST_TARGET_ENDPOINT)),
        DECLARE_NAPI_STATIC_PROPERTY("USB_REQUEST_TARGET_OTHER", ToInt32Value(env, USB_REQUEST_TARGET_OTHER)),
        DECLARE_NAPI_STATIC_PROPERTY("USBRequestTargetType", USBRequestTargetTypeEnum(env)),

        /* Declare Enum USBControlRequestType */
        DECLARE_NAPI_STATIC_PROPERTY("USB_REQUEST_TYPE_STANDARD", ToInt32Value(env, USB_REQUEST_TYPE_STANDARD)),
        DECLARE_NAPI_STATIC_PROPERTY("USB_REQUEST_TYPE_CLASS", ToInt32Value(env, USB_REQUEST_TYPE_CLASS)),
        DECLARE_NAPI_STATIC_PROPERTY("USB_REQUEST_TYPE_VENDOR", ToInt32Value(env, USB_REQUEST_TYPE_VENDOR)),
        DECLARE_NAPI_STATIC_PROPERTY("USBControlRequestType", USBControlRequestTypeEnum(env)),

        /* Declare Enum USBRequestDirection */
        DECLARE_NAPI_STATIC_PROPERTY("USB_REQUEST_DIR_TO_DEVICE", ToInt32Value(env, USB_REQUEST_DIR_TO_DEVICE)),
        DECLARE_NAPI_STATIC_PROPERTY("USB_REQUEST_DIR_FROM_DEVICE", ToInt32Value(env, USB_REQUEST_DIR_FROM_DEVICE)),
        DECLARE_NAPI_STATIC_PROPERTY("USBRequestDirection", USBRequestDirectionEnum(env)),

        /* Declare Enum FunctionType */
        DECLARE_NAPI_STATIC_PROPERTY("ACM", ToInt32Value(env, ACM)),
        DECLARE_NAPI_STATIC_PROPERTY("ECM", ToInt32Value(env, ECM)),
        DECLARE_NAPI_STATIC_PROPERTY("HDC", ToInt32Value(env, HDC)),
        DECLARE_NAPI_STATIC_PROPERTY("MTP", ToInt32Value(env, MTP)),
        DECLARE_NAPI_STATIC_PROPERTY("PTP", ToInt32Value(env, PTP)),
        DECLARE_NAPI_STATIC_PROPERTY("RNDIS", ToInt32Value(env, RNDIS)),
        DECLARE_NAPI_STATIC_PROPERTY("MIDI", ToInt32Value(env, MIDI)),
        DECLARE_NAPI_STATIC_PROPERTY("AUDIO_SOURCE", ToInt32Value(env, AUDIO_SOURCE)),
        DECLARE_NAPI_STATIC_PROPERTY("NCM", ToInt32Value(env, NCM)),
        DECLARE_NAPI_STATIC_PROPERTY("STORAGE", ToInt32Value(env, STORAGE)),
        DECLARE_NAPI_STATIC_PROPERTY("FunctionType", FunctionTypeEnum(env)),
        DECLARE_NAPI_STATIC_PROPERTY("UsbTransferFlags", NapiCreateFlagsEnum(env)),
        DECLARE_NAPI_STATIC_PROPERTY("UsbEndpointTransferType", NapiCreateTypeEnum(env)),
        DECLARE_NAPI_STATIC_PROPERTY("UsbTransferStatus", NapiCreateStatusEnum(env)),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

EXTERN_C_START
/*
 * function for module exports
 */
napi_value UsbInit(napi_env env, napi_value exports)
{
    USB_HILOGD(MODULE_USB_NAPI, "enter");

    napi_property_descriptor desc[] = {
        /* usb core */
        DECLARE_NAPI_FUNCTION("getDevices", CoreGetDevices),
        DECLARE_NAPI_FUNCTION("connectDevice", CoreConnectDevice),
        DECLARE_NAPI_FUNCTION("hasRight", CoreHasRight),
        DECLARE_NAPI_FUNCTION("requestRight", CoreRequestRight),
        DECLARE_NAPI_FUNCTION("usbFunctionsFromString", CoreUsbFunctionsFromString),
        DECLARE_NAPI_FUNCTION("getFunctionsFromString", CoreUsbFunctionsFromString),
        DECLARE_NAPI_FUNCTION("usbFunctionsToString", CoreUsbFunctionsToString),
        DECLARE_NAPI_FUNCTION("getStringFromFunctions", CoreUsbFunctionsToString),
        DECLARE_NAPI_FUNCTION("setCurrentFunctions", CoreSetCurrentFunctions),
        DECLARE_NAPI_FUNCTION("setDeviceFunctions", CoreSetCurrentFunctions),
        DECLARE_NAPI_FUNCTION("getCurrentFunctions", CoreGetCurrentFunctions),
        DECLARE_NAPI_FUNCTION("getDeviceFunctions", CoreGetCurrentFunctions),
        DECLARE_NAPI_FUNCTION("getPorts", CoreGetPorts),
        DECLARE_NAPI_FUNCTION("getPortList", CoreGetPorts),

        /* usb port */
        DECLARE_NAPI_FUNCTION("getSupportedModes", PortGetSupportedModes),
        DECLARE_NAPI_FUNCTION("getPortSupportModes", PortGetSupportedModes),
        DECLARE_NAPI_FUNCTION("setPortRoles", PortSetPortRole),
        DECLARE_NAPI_FUNCTION("setPortRoleTypes", PortSetPortRole),

        /* usb device pipe */
        DECLARE_NAPI_FUNCTION("claimInterface", PipeClaimInterface),
        DECLARE_NAPI_FUNCTION("releaseInterface", PipeReleaseInterface),
        DECLARE_NAPI_FUNCTION("bulkTransfer", PipeBulkTransfer),
        DECLARE_NAPI_FUNCTION("controlTransfer", PipeControlTransfer),
        DECLARE_NAPI_FUNCTION("usbControlTransfer", PipeUsbControlTransfer),
        DECLARE_NAPI_FUNCTION("setInterface", PipeSetInterface),
        DECLARE_NAPI_FUNCTION("setConfiguration", PipeSetConfiguration),
        DECLARE_NAPI_FUNCTION("getRawDescriptor", PipeGetRawDescriptors),
        DECLARE_NAPI_FUNCTION("getFileDescriptor", PipeGetFileDescriptor),
        DECLARE_NAPI_FUNCTION("closePipe", PipeClose),
        DECLARE_NAPI_FUNCTION("resetUsbDevice", PipeResetDevice),
        DECLARE_NAPI_FUNCTION("usbCancelTransfer", UsbCancelTransfer),
        DECLARE_NAPI_FUNCTION("usbSubmitTransfer", UsbSubmitTransfer),

        /* fort test get usb service version */
        DECLARE_NAPI_FUNCTION("getVersion", GetVersion),
        DECLARE_NAPI_FUNCTION("addRight", DeviceAddRight),
        DECLARE_NAPI_FUNCTION("addDeviceAccessRight", DeviceAddAccessRight),
        DECLARE_NAPI_FUNCTION("removeRight", DeviceRemoveRight),
        DECLARE_NAPI_FUNCTION("getAccessoryList", DeviceGetAccessoryList),
        DECLARE_NAPI_FUNCTION("openAccessory", DeviceOpenAccessory),
        DECLARE_NAPI_FUNCTION("closeAccessory", DeviceCloseAccessory),
        DECLARE_NAPI_FUNCTION("addAccessoryRight", DeviceAddAccessoryRight),
        DECLARE_NAPI_FUNCTION("hasAccessoryRight", DeviceHasAccessoryRight),
        DECLARE_NAPI_FUNCTION("requestAccessoryRight", DeviceRequestAccessoryRight),
        DECLARE_NAPI_FUNCTION("cancelAccessoryRight", DeviceCancelAccessoryRight),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));

    DeclareEnum(env, exports);

    USB_HILOGD(MODULE_USB_NAPI, "return");

    return exports;
}
EXTERN_C_END
