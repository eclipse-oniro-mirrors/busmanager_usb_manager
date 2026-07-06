/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "ohos.usbManager.impl.hpp"
#include "ashmem.h"
#include "event_handler.h"
#include "hilog_wrapper.h"
#include "ohos.usbManager.proj.hpp"
#include "stdexcept"
#include "taihe/runtime.hpp"
#include "usb_api_metrics.h"
#include "usb_errors.h"
#include "usb_srv_client.h"
#include "v1_2/usb_types.h"
#include "ohos.usbManager.SubmitTransferCallback.ani.1.hpp"

using namespace taihe;
using namespace ohos::usbManager;
using OHOS::Ashmem;
using OHOS::sptr;
using OHOS::USB::MODULE_USB_NAPI;
using OHOS::USB::USB_MGR_LABEL;

static int32_t g_accFd = 0;
const int32_t IO_ERROR = -1;
const int32_t INVALID_PARAM = -2;
const int32_t NO_DEVICE = -4;
const int32_t NOT_FOUND = -5;
const int32_t ERROR_BUSY = -6;
const int32_t NO_MEM = -11;

const int32_t ERROR = -1;
const int32_t HDF_DEV_ERR_NO_DEVICE = -202;
const int32_t USB_DEVICE_PIPE_CHECK_ERROR = 14400013;

static OHOS::USB::UsbSrvClient &g_usbClient = OHOS::USB::UsbSrvClient::GetInstance();

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

enum UsbJsErrCode : int32_t {
    OHEC_COMMON_PERMISSION_NOT_ALLOWED = 201,
    OHEC_COMMON_NORMAL_APP_NOT_ALLOWED = 202,
    OHEC_COMMON_PARAM_ERROR = 401,
    CAPABILITY_NOT_SUPPORT = 801,

    UEC_COMMON_BASE = 14400000,
    UEC_COMMON_HAS_NO_RIGHT = UEC_COMMON_BASE + 1,
    UEC_COMMON_HDC_NOT_ALLOWED = UEC_COMMON_BASE + 2,
    UEC_COMMON_PORTROLE_SWITCH_NOT_ALLOWED = UEC_COMMON_BASE + 3,
    UEC_COMMON_SERVICE_EXCEPTION = UEC_COMMON_BASE + 4,
    UEC_COMMON_RIGHT_DATABASE_ERROR = UEC_COMMON_BASE + 5,
    UEC_COMMON_FUNCTION_NOT_SUPPORT = UEC_COMMON_BASE + 6,

    UEC_ACCESSORY_BASE = UEC_COMMON_BASE + 1000,
    UEC_ACCESSORY_NOT_MATCH = UEC_ACCESSORY_BASE + 1,
    UEC_ACCESSORY_OPEN_FAILED = UEC_ACCESSORY_BASE + 2,
    UEC_ACCESSORY_CAN_NOT_REOPEN = UEC_ACCESSORY_BASE + 3,

    USB_SUBMIT_TRANSFER_RESOURCE_BUSY_ERROR = 14400007,
    USB_SUBMIT_TRANSFER_NO_DEVICE_ERROR = 14400008,
    USB_SUBMIT_TRANSFER_NO_MEM_ERROR = 14400009,
    USB_SUBMIT_TRANSFER_OTHER_ERROR = 14400010,
    USB_SUBMIT_TRANSFER_NOT_FOUND_ERROR = 14400011,
    USB_SUBMIT_TRANSFER_IO_ERROR = 14400012,
};

const std::map<int32_t, std::string_view> ERRCODE_MSG_MAP = {
    {OHEC_COMMON_PERMISSION_NOT_ALLOWED,
     "BusinessError 201:Permission verification failed. "
        "The application does not have the permission required to call the API."},
    {OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "BusinessError 202:Permission denied. Normal application uses system api."},
    {OHEC_COMMON_PARAM_ERROR, "BusinessError 401:Parameter error."},
    {CAPABILITY_NOT_SUPPORT, "BusinessError 801:Capability not supported."},
    {UEC_COMMON_HAS_NO_RIGHT, "BusinessError 14400001:Permission denied."},
    {UEC_COMMON_HDC_NOT_ALLOWED, "BusinessError 14400002:Permission denied. The HDC is disabled by the system."},
    {UEC_COMMON_PORTROLE_SWITCH_NOT_ALLOWED,
     "BusinessError 14400003:Unsupported operation.The current device does not support port role switching."},
    {UEC_COMMON_SERVICE_EXCEPTION,
     "BusinessError 14400004:Service exception. Possible causes:No accessory is plugged in."},
    {UEC_COMMON_RIGHT_DATABASE_ERROR, "BusinessError 14400005:Database operation exception."},
    {UEC_COMMON_FUNCTION_NOT_SUPPORT, "BusinessError 14400006:Unsupported operation. The function is not supported."},
    {UEC_ACCESSORY_NOT_MATCH, "BusinessError 14401001:The target USBAccessory not matched."},
    {UEC_ACCESSORY_OPEN_FAILED, "BusinessError 14401002:Failed to open the native accessory node."},
    {UEC_ACCESSORY_CAN_NOT_REOPEN, "BusinessError 14401003:Cannot reopen the accessory."},

    {USB_SUBMIT_TRANSFER_RESOURCE_BUSY_ERROR, "BusinessError 14400007:Resource busy."},
    {USB_SUBMIT_TRANSFER_NO_DEVICE_ERROR, "BusinessError 14400008:No such device (it may have been disconnected)."},
    {USB_SUBMIT_TRANSFER_NO_MEM_ERROR, "BusinessError 14400009:Insufficient memory."},
    {USB_SUBMIT_TRANSFER_OTHER_ERROR, "BusinessError 14400010:Other USB error."},
    {USB_SUBMIT_TRANSFER_NOT_FOUND_ERROR,
     "BusinessError 14400011:The transfer is not in progress, or is already complete or cancelled."},
    {USB_SUBMIT_TRANSFER_IO_ERROR, "BusinessError 14400012:Transmission I/O error."},
};

static std::string_view GetErrMsgByErrCode(int32_t errCode)
{
    auto obj = ERRCODE_MSG_MAP.find(errCode);
    if (obj == ERRCODE_MSG_MAP.end()) {
        return "";
    }
    return obj->second;
}

static void ThrowBusinessError(int32_t errCode, const std::string &errMsg)
{
    set_business_error(errCode, std::string(GetErrMsgByErrCode(errCode)) + errMsg);
}

namespace {

static ohos::usbManager::USBEndpoint ParseToUSBEndpoint(const OHOS::USB::USBEndpoint &usbEndpoint)
{
    auto directionKey = usbEndpoint.GetDirection() == OHOS::USB::USB_ENDPOINT_DIR_OUT ?
        ohos::usbManager::USBRequestDirection::key_t::USB_REQUEST_DIR_TO_DEVICE :
        ohos::usbManager::USBRequestDirection::key_t::USB_REQUEST_DIR_FROM_DEVICE;
    return {
        .address = usbEndpoint.GetAddress(),
        .attributes = usbEndpoint.GetAttributes(),
        .interval = usbEndpoint.GetInterval(),
        .maxPacketSize = usbEndpoint.GetMaxPacketSize(),
        .direction = ohos::usbManager::USBRequestDirection(directionKey),
        .endpointAddr = usbEndpoint.GetEndpointNumber(),
        .type = usbEndpoint.GetType(),
        .interfaceId = usbEndpoint.GetInterfaceId()
    };
}

static ohos::usbManager::USBInterface ParseToUSBInterface(OHOS::USB::UsbInterface &usbInterface)
{
    std::vector<ohos::usbManager::USBEndpoint> endpoints;
    for (const auto &endpoint : usbInterface.GetEndpoints()) {
        endpoints.push_back(ParseToUSBEndpoint(endpoint));
    }
    return {
        .id = usbInterface.GetId(),
        .protocol = usbInterface.GetProtocol(),
        .clazz = usbInterface.GetClass(),
        .subClass = usbInterface.GetSubClass(),
        .alternateSetting = usbInterface.GetAlternateSetting(),
        .name = usbInterface.GetName(),
        .endpoints = array<ohos::usbManager::USBEndpoint>(endpoints)
    };
}

static ohos::usbManager::USBConfiguration ParseToUSBConfiguration(OHOS::USB::USBConfig &usbConfig)
{
    std::vector<ohos::usbManager::USBInterface> interfaces;
    for (auto &interface : usbConfig.GetInterfaces()) {
        interfaces.push_back(ParseToUSBInterface(interface));
    }
    return {
        .id = usbConfig.GetId(),
        .attributes = usbConfig.GetAttributes(),
        .maxPower = usbConfig.GetMaxPower(),
        .name = usbConfig.GetName(),
        .isRemoteWakeup = usbConfig.IsRemoteWakeup(),
        .isSelfPowered = usbConfig.IsSelfPowered(),
        .interfaces = array<ohos::usbManager::USBInterface>(interfaces)
    };
}

static ohos::usbManager::USBDevice ParseToUSBDevice(OHOS::USB::UsbDevice &usbDevice)
{
    std::vector<ohos::usbManager::USBConfiguration> configs;
    for (auto &config : usbDevice.GetConfigs()) {
        configs.push_back(ParseToUSBConfiguration(config));
    }
    return {
        .busNum = usbDevice.GetBusNum(),
        .devAddress = usbDevice.GetDevAddr(),
        .serial = usbDevice.GetmSerial(),
        .name = usbDevice.GetName(),
        .manufacturerName = usbDevice.GetManufacturerName(),
        .productName = usbDevice.GetProductName(),
        .version = usbDevice.GetVersion(),
        .vendorId = usbDevice.GetVendorId(),
        .productId = usbDevice.GetProductId(),
        .clazz = usbDevice.GetClass(),
        .subClass = usbDevice.GetSubclass(),
        .protocol = usbDevice.GetProtocol(),
        .configs = array<ohos::usbManager::USBConfiguration>(configs)
    };
}

std::map<OHOS::USB::UsbErrCode, std::string> errorTable = {
    {OHOS::USB::UsbErrCode::UEC_SERIAL_OTHER_ERROR,               "Connecting to the service failed."},
    {OHOS::USB::UsbErrCode::UEC_INTERFACE_PERMISSION_DENIED,      "Permission is denied"             },
    {OHOS::USB::UsbErrCode::UEC_SERVICE_PERMISSION_DENIED_SYSAPI, "System permission is denied"      },
    {OHOS::USB::UsbErrCode::UEC_INTERFACE_INVALID_VALUE,          "Invalid input parameter."         }
};

static OHOS::USB::USBEndpoint ConvertToUSBEndpoint(const ohos::usbManager::USBEndpoint &usbEndpoint)
{
    OHOS::USB::USBEndpoint point;
    point.SetAddr(usbEndpoint.address);
    point.SetAttr(usbEndpoint.attributes);
    point.SetInterval(usbEndpoint.interval);
    point.SetMaxPacketSize(usbEndpoint.maxPacketSize);
    point.SetInterfaceId(usbEndpoint.interfaceId);
    return point;
}

static OHOS::USB::UsbInterface ConvertToUsbInterface(ohos::usbManager::USBInterface const &usbInterface)
{
    std::vector<OHOS::USB::USBEndpoint> endpts;
    for (const auto &endpoint : usbInterface.endpoints) {
        endpts.push_back(ConvertToUSBEndpoint(endpoint));
    }
    OHOS::USB::UsbInterface interface;
    interface.SetId(usbInterface.id);
    interface.SetProtocol(usbInterface.protocol);
    interface.SetClass(usbInterface.clazz);
    interface.SetSubClass(usbInterface.subClass);
    interface.SetAlternateSetting(usbInterface.alternateSetting);
    interface.SetName(usbInterface.name.c_str());
    interface.SetEndpoints(endpts);
    return interface;
}

static OHOS::USB::USBDevicePipe ConvertUSBDevicePipe(ohos::usbManager::USBDevicePipe const &usbDevice)
{
    OHOS::USB::USBDevicePipe device;
    device.SetBusNum(static_cast<uint8_t>(usbDevice.busNum));
    device.SetDevAddr(static_cast<uint8_t>(usbDevice.devAddress));
    return device;
}

static OHOS::USB::USBConfig ConvertToUSBConfig(ohos::usbManager::USBConfiguration const &usbConfiguration)
{
    std::vector<OHOS::USB::UsbInterface> interfaces;
    for (auto &interface : usbConfiguration.interfaces) {
        interfaces.push_back(ConvertToUsbInterface(interface));
    }
    OHOS::USB::USBConfig usbConfig;
    usbConfig.SetId(usbConfiguration.id);
    usbConfig.SetAttribute(usbConfiguration.attributes);
    usbConfig.SetMaxPower(usbConfiguration.maxPower);
    usbConfig.SetName(usbConfiguration.name.c_str());
    usbConfig.SetInterfaces(interfaces);
    return usbConfig;
}

static OHOS::USB::USBAccessory ConvertUsbDevice(ohos::usbManager::USBAccessory const &usbDevice)
{
    OHOS::USB::USBAccessory device;
    device.SetManufacturer(usbDevice.manufacturer.c_str());
    device.SetProduct(usbDevice.product.c_str());
    device.SetDescription(usbDevice.description.c_str());
    device.SetVersion(usbDevice.version.c_str());
    device.SetSerialNumber(usbDevice.serialNumber.c_str());
    return device;
}

static OHOS::USB::UsbDevice ConvertToUsbDevice(ohos::usbManager::USBDevice const &usbDevice)
{
    std::vector<OHOS::USB::USBConfig> configs;
    for (auto &config : usbDevice.configs) {
        configs.push_back(ConvertToUSBConfig(config));
    }
    OHOS::USB::UsbDevice device;
    device.SetConfigs(configs);
    device.SetDevAddr(usbDevice.devAddress);
    device.SetBusNum(usbDevice.busNum);
    device.SetName(usbDevice.name.c_str());
    device.SetManufacturerName(usbDevice.manufacturerName.c_str());
    device.SetProductName(usbDevice.productName.c_str());
    device.SetVersion(usbDevice.version.c_str());
    device.SetVendorId(usbDevice.vendorId);
    device.SetProductId(usbDevice.productId);
    device.SetClass(usbDevice.clazz);
    device.SetSubclass(usbDevice.subClass);
    device.SetProtocol(usbDevice.protocol);
    return device;
}

static ohos::usbManager::USBPortStatus ConvertUSBPortStatus(OHOS::USB::UsbPortStatus const &usbDevice)
{
    ohos::usbManager::USBPortStatus device;
    device.currentDataRole = usbDevice.currentDataRole;
    device.currentMode = usbDevice.currentMode;
    device.currentPowerRole = usbDevice.currentPowerRole;
    return device;
}

static ohos::usbManager::USBPort ConvertUSBPort(OHOS::USB::UsbPort const &usbDevice)
{
    using PortModeType = ohos::usbManager::PortModeType;
    using key_t = PortModeType::key_t;
    key_t modeKey = key_t::NONE;
    if (usbDevice.supportedModes >= 0 && usbDevice.supportedModes < static_cast<int32_t>(key_t::NUM_MODES)) {
        modeKey = static_cast<key_t>(usbDevice.supportedModes);
    }
    return {
        .id = usbDevice.id,
        .supportedModes = PortModeType(modeKey),
        .status = ConvertUSBPortStatus(usbDevice.usbPortStatus)
    };
}

USBDevicePipe connectDevice(USBDevice const &device)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.ConnectDevice");
    OHOS::USB::USBDevicePipe pipe;
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return {};
    }
    OHOS::USB::UsbDevice usbDev = ConvertToUsbDevice(device);
    int32_t ret = g_usbClient.OpenDevice(usbDev, pipe);
    if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED || ret == OHOS::USB::UEC_INTERFACE_PERMISSION_DENIED) {
        metrics.SetErrorCode(UEC_COMMON_HAS_NO_RIGHT);
        ThrowBusinessError(UEC_COMMON_HAS_NO_RIGHT, "need call requestRight to get the permission");
        USB_HILOGE(MODULE_USB_NAPI, "Connect Device failed, return code:%{public}d", ret);
        return {};
    }
    if (ret == HDF_DEV_ERR_NO_DEVICE) {
        metrics.SetErrorCode(USB_SUBMIT_TRANSFER_IO_ERROR);
        ThrowBusinessError(USB_SUBMIT_TRANSFER_IO_ERROR, "");
        return {};
    }
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGE(MODULE_USB_NAPI, "Connect Device failed, return code:%{public}d", ret);
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION, "");
        return {};
    }
    return {
        .busNum = pipe.GetBusNum(),
        .devAddress = pipe.GetDevAddr()
    };
}

static void ParseEndpointObj(const ohos::usbManager::USBEndpoint endpoint, OHOS::USB::USBEndpoint &ep)
{
    ep = OHOS::USB::USBEndpoint(endpoint.address, endpoint.attributes, endpoint.interval, endpoint.maxPacketSize);
    ep.SetInterfaceId(endpoint.interfaceId);
}

array<ohos::usbManager::USBDevice> getDevices()
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.GetDevices");
    std::vector<ohos::usbManager::USBDevice> res;
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return array<ohos::usbManager::USBDevice>(res);
    }
    std::vector<OHOS::USB::UsbDevice> deviceList;
    auto ret = g_usbClient.GetDevices(deviceList);
    if (ret != 0) {
        USB_HILOGE(MODULE_USB_NAPI, "GetDevices failed, return code:%{public}d", ret);
        return array<ohos::usbManager::USBDevice>(res);
    }
    for (auto &usbDevice : deviceList) {
        res.push_back(ParseToUSBDevice(usbDevice));
    }
    return array<ohos::usbManager::USBDevice>(res);
}

bool hasRight(string_view deviceName)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.HasRight");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return false;
    }
    bool result = g_usbClient.HasRight(std::string(deviceName));
    return result;
}

bool requestRightSync(string_view deviceName)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.RequestRightSync");
    bool bRet = false;
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return bRet;
    }
    int32_t ret = g_usbClient.RequestRight(std::string(deviceName));
    if (ret == OHOS::USB::UEC_OK) {
        bRet = true;
    }
    return bRet;
}

bool removeRight(string_view deviceName)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.RemoveRight");
    bool bResult = false;
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return bResult;
    }
    int32_t ret = g_usbClient.RemoveRight(std::string(deviceName));
    if (ret == OHOS::USB::UEC_OK) {
        bResult = true;
    }
    return bResult;
}

bool addDeviceAccessRight(string_view tokenId, string_view deviceName)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.AddAccessRight");
    bool bResult = false;
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return bResult;
    }
    int32_t ret = g_usbClient.AddAccessRight(std::string(tokenId), std::string(deviceName));
    if (ret == OHOS::USB::UEC_OK) {
        bResult = true;
    } else if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI) {
        metrics.SetErrorCode(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "Normal app not allowed");
        return bResult;
    } else if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED) {
        metrics.SetErrorCode(OHEC_COMMON_PERMISSION_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_PERMISSION_NOT_ALLOWED, "Permission not allowed");
        return bResult;
    } else {
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        USB_HILOGE(MODULE_USB_NAPI, "AddAccessRight ret = %{public}d", ret);
    }
    return bResult;
}

int32_t getFunctionsFromString(string_view funcs)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.GetFunctionsFromString");
    if (!HasFeature(FEATURE_DEVICE)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return CAPABILITY_NOT_SUPPORT;
    }
    int32_t numFuncs = g_usbClient.UsbFunctionsFromString(std::string(funcs));
    if (numFuncs == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI) {
        metrics.SetErrorCode(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
        return OHEC_COMMON_NORMAL_APP_NOT_ALLOWED;
    }
    if (numFuncs == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED) {
        metrics.SetErrorCode(OHEC_COMMON_PERMISSION_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
        return OHEC_COMMON_PERMISSION_NOT_ALLOWED;
    }
    return static_cast<int32_t>(numFuncs);
}

string getStringFromFunctions(int32_t funcs)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.GetStringFromFunctions");
    if (!HasFeature(FEATURE_DEVICE)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return "";
    }
    std::string strFuncs = g_usbClient.UsbFunctionsToString(funcs);
    if (strFuncs == OHOS::USB::PERMISSION_DENIED_SYSAPI) {
        metrics.SetErrorCode(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
        return "";
    }
    if (strFuncs == OHOS::USB::SYS_APP_PERMISSION_DENIED_SYSAPI) {
        metrics.SetErrorCode(OHEC_COMMON_PERMISSION_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
        return "";
    }
    return strFuncs;
}

void setDeviceFunctionsSync(int32_t funcs)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.SetDeviceFunctionsSync");
    if (!HasFeature(FEATURE_DEVICE)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return;
    }
    int32_t ret = g_usbClient.SetCurrentFunctions(funcs);
    if (ret == OHOS::USB::UEC_OK) {
        return;
    } else if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI) {
        metrics.SetErrorCode(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
        return;
    } else if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED) {
        metrics.SetErrorCode(OHEC_COMMON_PERMISSION_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
        return;
    } else if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_CHECK_HDC) {
        metrics.SetErrorCode(UEC_COMMON_HDC_NOT_ALLOWED);
        ThrowBusinessError(UEC_COMMON_HDC_NOT_ALLOWED, "");
        return;
    } else {
        USB_HILOGE(MODULE_USB_NAPI, "setDeviceFunctionsSync failed, return code:%{public}d", ret);
        metrics.SetErrorCode(UEC_COMMON_FUNCTION_NOT_SUPPORT);
        ThrowBusinessError(UEC_COMMON_FUNCTION_NOT_SUPPORT, "");
        return;
    }
}

int32_t getDeviceFunctions()
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.GetDeviceFunctions");
    int32_t cfuncs = 0;
    if (!HasFeature(FEATURE_DEVICE)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return cfuncs;
    }
    int32_t ret = g_usbClient.GetCurrentFunctions(cfuncs);
    if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI) {
        metrics.SetErrorCode(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "Normal app not allowed");
        return cfuncs;
    }
    if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED) {
        metrics.SetErrorCode(OHEC_COMMON_PERMISSION_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_PERMISSION_NOT_ALLOWED, "Permission not allowed");
        return cfuncs;
    }
    if (ret != OHOS::USB::UEC_OK) {
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        USB_HILOGE(MODULE_USB_NAPI, "GetCurrentFunctions ret = %{public}d", ret);
        ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION, "");
    }
    return cfuncs;
}

array<USBPort> getPortList()
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.GetPortList");
    if (!HasFeature(FEATURE_PORT)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return {};
    }
    std::vector<OHOS::USB::UsbPort> ports;
    int32_t ret = g_usbClient.GetPorts(ports);
    if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI) {
        metrics.SetErrorCode(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
        return {};
    }
    if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED) {
        metrics.SetErrorCode(OHEC_COMMON_PERMISSION_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
        return {};
    }
    if (ret != OHOS::USB::UEC_OK) {
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        USB_HILOGE(MODULE_USB_NAPI, "GetPorts ret = %{public}d", ret);
        ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION, "");
        return {};
    }
    if (ports.empty()) {
        USB_HILOGE(MODULE_USB_NAPI, "ports empty");
        return {};
    }
    std::vector<::ohos::usbManager::USBPort> convertedPorts;
    convertedPorts.reserve(ports.size());
    for (const auto &port : ports) {
        convertedPorts.push_back(ConvertUSBPort(port));
    }
    return ::taihe::array<::ohos::usbManager::USBPort>(
        taihe::array_view<::ohos::usbManager::USBPort>(convertedPorts.data(), convertedPorts.size()));
}

PortModeType getPortSupportModes(int32_t portId)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.GetPortSupportModes");
    int32_t result = 0;
    if (!HasFeature(FEATURE_PORT)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return {PortModeType::key_t(result)};
    }
    if (portId < 0) {
        metrics.SetErrorCode(OHEC_COMMON_PARAM_ERROR);
        ThrowBusinessError(OHEC_COMMON_PARAM_ERROR, "");
        return {PortModeType::key_t(result)};
    }
    int32_t ret = g_usbClient.GetSupportedModes(portId, result);
    if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI) {
        metrics.SetErrorCode(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
        return {PortModeType::key_t(result)};
    }
    if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED) {
        metrics.SetErrorCode(OHEC_COMMON_PERMISSION_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
        return {PortModeType::key_t(result)};
    }
    if (ret != OHOS::USB::UEC_OK) {
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        USB_HILOGE(MODULE_USB_NAPI, "GetSupportedModes ret = %{public}d", ret);
    }
    return {PortModeType::key_t(result)};
}

void setPortRoleTypesSync(int32_t portId, PowerRoleType powerRole, DataRoleType dataRole)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.SetPortRoleTypesSync");
    if (!HasFeature(FEATURE_PORT)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return;
    }
    if (portId < 0 || powerRole < 0 || dataRole < 0) {
        metrics.SetErrorCode(OHEC_COMMON_PARAM_ERROR);
        ThrowBusinessError(OHEC_COMMON_PARAM_ERROR, "");
        return;
    }
    int ret = g_usbClient.SetPortRole(portId, powerRole, dataRole);
    if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI) {
        metrics.SetErrorCode(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
        return;
    }
    if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED) {
        metrics.SetErrorCode(OHEC_COMMON_PERMISSION_NOT_ALLOWED);
        ThrowBusinessError(OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
        return;
    }
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGD(MODULE_USB_NAPI, "SetPortRole ret: %{public}d", ret);
        metrics.SetErrorCode(UEC_COMMON_PORTROLE_SWITCH_NOT_ALLOWED);
        ThrowBusinessError(UEC_COMMON_PORTROLE_SWITCH_NOT_ALLOWED, "");
        return;
    }
}

void addAccessoryRight(int32_t tokenId, ohos::usbManager::USBAccessory const &accessory)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.AddAccessoryRight");
    if (!HasFeature(FEATURE_DEVICE)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return;
    }
    int32_t ret = g_usbClient.AddAccessoryRight(tokenId, ConvertUsbDevice(accessory));
    switch (ret) {
        case OHOS::USB::UEC_OK:
            return;
        case OHOS::USB::UEC_SERVICE_GET_TOKEN_INFO_FAILED:
            metrics.SetErrorCode(OHEC_COMMON_PARAM_ERROR);
            ThrowBusinessError(OHEC_COMMON_PARAM_ERROR, "");
            break;
        case OHOS::USB::UEC_SERVICE_ACCESSORY_NOT_MATCH:
            metrics.SetErrorCode(OHEC_COMMON_PARAM_ERROR);
            ThrowBusinessError(OHEC_COMMON_PARAM_ERROR, "Get accessory through getAccessoryList");
            break;
        case OHOS::USB::UEC_SERVICE_DATABASE_OPERATION_FAILED:
            metrics.SetErrorCode(UEC_COMMON_RIGHT_DATABASE_ERROR);
            ThrowBusinessError(UEC_COMMON_RIGHT_DATABASE_ERROR, "Database request operation exception");
            break;
        case OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI:
            metrics.SetErrorCode(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED);
            ThrowBusinessError(OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "");
            break;
        case OHOS::USB::UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED:
            metrics.SetErrorCode(OHEC_COMMON_PERMISSION_NOT_ALLOWED);
            ThrowBusinessError(OHEC_COMMON_PERMISSION_NOT_ALLOWED, "");
            break;
        default:
            metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
            ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION, "Service exception");
            break;
    }
    USB_HILOGD(MODULE_USB_NAPI, "Device call AddAccessoryRight ret: %{public}d", ret);
}

int32_t claimInterface(USBDevicePipe const &pipe, USBInterface const &iface, optional_view<bool> force)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.ClaimInterface");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return ERROR;
    }
    OHOS::USB::USBDevicePipe internalPipe = ConvertUSBDevicePipe(pipe);
    bool forceClaim = force.has_value() ? *force : false;
    int ret = internalPipe.ClaimInterface(ConvertToUsbInterface(iface), forceClaim);
    USB_HILOGD(MODULE_USB_NAPI, "pipe call claimInterface ret: %{public}d", ret);
    return ret;
}

int32_t releaseInterface(USBDevicePipe const &pipe, USBInterface const &iface)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.ReleaseInterface");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return ERROR;
    }
    OHOS::USB::USBDevicePipe internalPipe = ConvertUSBDevicePipe(pipe);
    int ret = internalPipe.ReleaseInterface(ConvertToUsbInterface(iface));
    USB_HILOGD(MODULE_USB_NAPI, "pipe call releaseInterface ret: %{public}d", ret);
    return ret;
}

int32_t setConfiguration(USBDevicePipe const &pipe, USBConfiguration const &config)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.SetConfiguration");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return ERROR;
    }
    OHOS::USB::USBDevicePipe internalPipe = ConvertUSBDevicePipe(pipe);
    return g_usbClient.SetConfiguration(internalPipe, ConvertToUSBConfig(config));
}

int32_t setInterface(USBDevicePipe const &pipe, USBInterface const &iface)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.SetInterface");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return ERROR;
    }
    OHOS::USB::USBDevicePipe internalPipe = ConvertUSBDevicePipe(pipe);
    return g_usbClient.SetInterface(internalPipe, ConvertToUsbInterface(iface));
}

array<uint8_t> getRawDescriptor(USBDevicePipe const &pipe)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.GetRawDescriptor");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return {};
    }
    std::vector<uint8_t> bufferData;
    OHOS::USB::USBDevicePipe internalPipe = ConvertUSBDevicePipe(pipe);
    int ret = g_usbClient.GetRawDescriptors(internalPipe, bufferData);
    if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED) {
        metrics.SetErrorCode(UEC_COMMON_HAS_NO_RIGHT);
        ThrowBusinessError(UEC_COMMON_HAS_NO_RIGHT, "need call requestRight to get the permission");
        USB_HILOGE(MODULE_USB_NAPI, "Connect Device failed, return code:%{public}d", ret);
        return {};
    }
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGE(MODULE_USB_NAPI, "getRawDescriptor failed:%{public}d", ret);
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION, "");
        return {};
    }
    return ::taihe::array<uint8_t>(bufferData);
}

int32_t getFileDescriptor(USBDevicePipe const &pipe)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.GetFileDescriptor");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return ERROR;
    }
    int32_t fd = ERROR;
    OHOS::USB::USBDevicePipe internalPipe = ConvertUSBDevicePipe(pipe);
    int32_t ret = g_usbClient.GetFileDescriptor(internalPipe, fd);
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGE(MODULE_USB_NAPI, "getFileDescriptor failed:%{public}d", ret);
    }
    return static_cast<int32_t>(fd);
}

int32_t usbControlTransferSync(
    USBDevicePipe const &pipe, USBDeviceRequestParams const &requestparam, optional_view<int32_t> timeout)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.UsbControlTransferSync");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return ERROR;
    }
    OHOS::USB::USBDevicePipe nativePipe;
    nativePipe.SetBusNum(static_cast<uint8_t>(pipe.busNum));
    nativePipe.SetDevAddr(static_cast<uint8_t>(pipe.devAddress));
    int32_t timeOut = 0;
    if (timeout.has_value()) {
        timeOut = int(timeout.value());
    }
    const OHOS::HDI::Usb::V1_2::UsbCtrlTransferParams tctrl = {requestparam.bmRequestType, requestparam.bRequest,
        requestparam.wValue, requestparam.wIndex, requestparam.wLength, timeOut};

    ani_ref bufferRef;
    ani_env *env = ::taihe::get_env();
    ani_object array_obj = reinterpret_cast<ani_object>(requestparam.data);
    if (env == nullprt) {
        USB_HILOGE(MODULE_USB_NAPI, "get_env failed,env is nullptr");
        return ERROR;
    }

    if (ANI_OK != env->Object_GetFieldByName_Ref(array_obj, "buffer", &bufferRef)) {
        USB_HILOGE(MODULE_USB_NAPI, "Object_GetFieldByName_Ref failed.");
        return ERROR;
    }
    void *data;
    size_t size;
    if (ANI_OK != env->ArrayBuffer_GetInfo(static_cast<ani_arraybuffer>(bufferRef), &data, &size)) {
        USB_HILOGE(MODULE_USB_NAPI,  "ArrayBuffer_GetInfo failed.");
        return ERROR;
    }
    int32_t ret;
    std::vector<uint8_t> bufferData(static_cast<uint8_t *>(data), static_cast<uint8_t *>(data) + size);
    ret = nativePipe.UsbControlTransfer(tctrl, bufferData);
    if ((uint32_t(requestparam.bmRequestType) & OHOS::USB::USB_ENDPOINT_DIR_MASK) == OHOS::USB::USB_ENDPOINT_DIR_IN) {
        ret = memcpy_s(data, size, bufferData.data(), bufferData.size());
    }
    if (ret != OHOS::USB::UEC_OK) {
        return ERROR;
    }
    return bufferData.size();
}

int32_t bulkTransferSync(::ohos::usbManager::USBDevicePipe const &pipe, ::ohos::usbManager::USBEndpoint const &endpoint,
    uintptr_t buffer, optional_view<int32_t> timeout)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.BulkTransferSync");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return ERROR;
    }
    ani_ref bufferRef;
    ani_env *env = ::taihe::get_env();
    ani_object array_obj = reinterpret_cast<ani_object>(buffer);
    if (env == nullprt) {
        USB_HILOGE(MODULE_USB_NAPI, "get_env failed,env is nullptr");
        return ERROR;
    }

    if (ANI_OK != env->Object_GetFieldByName_Ref(array_obj, "buffer", &bufferRef)) {
        USB_HILOGE(MODULE_USB_NAPI,   "Object_GetFieldByName_Ref failed.");
        return ERROR;
    }
    void *data;
    size_t size;
    if (ANI_OK != env->ArrayBuffer_GetInfo(static_cast<ani_arraybuffer>(bufferRef), &data, &size)) {
        USB_HILOGE(MODULE_USB_NAPI,   "ArrayBuffer_GetInfo failed.");
        return ERROR;
    }
    OHOS::USB::USBEndpoint ep;
    ParseEndpointObj(endpoint, ep);
    std::vector<uint8_t> bufferData(static_cast<uint8_t *>(data), static_cast<uint8_t *>(data) + size);
    OHOS::USB::USBDevicePipe nativePipe;
    nativePipe.SetBusNum(static_cast<uint8_t>(pipe.busNum));
    nativePipe.SetDevAddr(static_cast<uint8_t>(pipe.devAddress));
    int32_t timeOut = 0;
    if (timeout.has_value()) {
        timeOut = timeout.value();
    }
    int32_t ret = nativePipe.BulkTransfer(ep, bufferData, timeOut);
    if (ep.GetDirection() == OHOS::USB::USB_ENDPOINT_DIR_IN) {
        if (bufferData.size() > size) {
            USB_HILOGE(MODULE_USB_NAPI, "src buffer size larger than dst.");
            return ERROR;
        }
        ret = memcpy_s(data, size, bufferData.data(), bufferData.size());
    }
    if (ret == OHOS::USB::UEC_OK) {
        return bufferData.size();
    } else {
        return ERROR;
    }
}

int32_t closePipe(USBDevicePipe const &pipe)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.ClosePipe");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return ERROR;
    }
    OHOS::USB::USBDevicePipe nativePipe;
    nativePipe.SetBusNum(static_cast<uint8_t>(pipe.busNum));
    nativePipe.SetDevAddr(static_cast<uint8_t>(pipe.devAddress));
    return nativePipe.Close();
}

static OHOS::USB::USBAccessory taihe2Native(ohos::usbManager::USBAccessory accessory)
{
    OHOS::USB::USBAccessory nativeAccessory = {accessory.manufacturer.c_str(), accessory.product.c_str(),
        accessory.description.c_str(), accessory.version.c_str(), accessory.serialNumber.c_str()};
    return nativeAccessory;
}

static ohos::usbManager::USBAccessory native2Taihe(OHOS::USB::USBAccessory accessory)
{
    ohos::usbManager::USBAccessory taiheAccessory = {accessory.GetManufacturer(), accessory.GetProduct(),
        accessory.GetDescription(), accessory.GetVersion(), accessory.GetSerialNumber()};
    return taiheAccessory;
}

bool hasAccessoryRight(ohos::usbManager::USBAccessory const &accessory)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.HasAccessoryRight");
    bool result = false;
    if (!HasFeature(FEATURE_DEVICE)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return result;
    }
    int32_t ret = g_usbClient.HasAccessoryRight(taihe2Native(accessory), result);
    if (ret == OHOS::USB::UEC_OK) {
        return result;
    } else if (ret == OHOS::USB::UEC_SERVICE_ACCESSORY_NOT_MATCH) {
        metrics.SetErrorCode(UEC_ACCESSORY_NOT_MATCH);
        ThrowBusinessError(UEC_ACCESSORY_NOT_MATCH, "Get accessory through getAccessoryList");
        return result;
    } else if (ret == OHOS::USB::UEC_SERVICE_DATABASE_OPERATION_FAILED) {
        metrics.SetErrorCode(UEC_COMMON_RIGHT_DATABASE_ERROR);
        ThrowBusinessError(UEC_COMMON_RIGHT_DATABASE_ERROR, "Database request operation exception");
        return result;
    } else {
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION, "Service exception");
    }
    return result;
}

bool requestAccessoryRightSync(ohos::usbManager::USBAccessory const &accessory)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.RequestAccessoryRightSync");
    bool result = false;
    if (!HasFeature(FEATURE_DEVICE)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return result;
    }
    int32_t ret = g_usbClient.RequestAccessoryRight(taihe2Native(accessory), result);
    if (ret == OHOS::USB::UEC_OK) {
        return result;
    } else if (ret == OHOS::USB::UEC_SERVICE_ACCESSORY_NOT_MATCH) {
        metrics.SetErrorCode(UEC_ACCESSORY_NOT_MATCH);
        ThrowBusinessError(UEC_ACCESSORY_NOT_MATCH, "");
        return result;
    } else if (ret == OHOS::USB::UEC_SERVICE_DATABASE_OPERATION_FAILED) {
        metrics.SetErrorCode(UEC_COMMON_RIGHT_DATABASE_ERROR);
        ThrowBusinessError(UEC_COMMON_RIGHT_DATABASE_ERROR, "");
        return result;
    } else {
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION, "");
    }
    return result;
}

void cancelAccessoryRight(ohos::usbManager::USBAccessory const &accessory)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.CancelAccessoryRight");
    if (!HasFeature(FEATURE_DEVICE)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return;
    }
    if (g_accFd != 0) {
        g_usbClient.CloseAccessory(g_accFd);
        close(g_accFd);
        g_accFd = 0;
    }
    int32_t ret = g_usbClient.CancelAccessoryRight(taihe2Native(accessory));
    if (ret == OHOS::USB::UEC_OK) {
        return;
    } else if (ret == OHOS::USB::UEC_SERVICE_ACCESSORY_NOT_MATCH) {
        metrics.SetErrorCode(UEC_ACCESSORY_NOT_MATCH);
        ThrowBusinessError(UEC_ACCESSORY_NOT_MATCH, "Get accessory through getAccessoryList");
        return;
    } else if (ret == OHOS::USB::UEC_SERVICE_DATABASE_OPERATION_FAILED) {
        metrics.SetErrorCode(UEC_COMMON_RIGHT_DATABASE_ERROR);
        ThrowBusinessError(UEC_COMMON_RIGHT_DATABASE_ERROR, "Database request operation exception");
        return;
    } else {
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION, "Service exception");
        return;
    }
}

array<ohos::usbManager::USBAccessory> getAccessoryList()
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.GetAccessoryList");
    if (!HasFeature(FEATURE_DEVICE)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return {};
    }
    std::vector<OHOS::USB::USBAccessory> accessoryList;
    std::vector<ohos::usbManager::USBAccessory> results;
    int32_t ret = g_usbClient.GetAccessoryList(accessoryList);
    if (ret == OHOS::USB::UEC_OK) {
        for (const auto &item : accessoryList) {
            results.push_back(native2Taihe(item));
        }
    } else {
        results.clear();
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION, "Service exception");
    }
    return array<ohos::usbManager::USBAccessory>(results);
}

USBAccessoryHandle openAccessory(ohos::usbManager::USBAccessory const &accessory)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.OpenAccessory");
    USBAccessoryHandle handler;
    if (!HasFeature(FEATURE_DEVICE)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return handler;
    }
    int32_t fd = -1;
    int32_t ret = g_usbClient.OpenAccessory(taihe2Native(accessory), fd);
    if (ret == OHOS::USB::UEC_OK) {
        g_accFd = fd;
        handler.accessoryFd = fd;
    } else if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED || ret == OHOS::USB::UEC_INTERFACE_PERMISSION_DENIED) {
        metrics.SetErrorCode(UEC_COMMON_HAS_NO_RIGHT);
        ThrowBusinessError(UEC_COMMON_HAS_NO_RIGHT, "");
    } else if (ret == OHOS::USB::UEC_SERVICE_ACCESSORY_NOT_MATCH) {
        metrics.SetErrorCode(UEC_ACCESSORY_NOT_MATCH);
        ThrowBusinessError(UEC_ACCESSORY_NOT_MATCH, "");
    } else if (ret == OHOS::USB::UEC_SERVICE_ACCESSORY_OPEN_NATIVE_NODE_FAILED) {
        metrics.SetErrorCode(UEC_ACCESSORY_OPEN_FAILED);
        ThrowBusinessError(UEC_ACCESSORY_OPEN_FAILED, "");
    } else if (ret == OHOS::USB::UEC_SERVICE_ACCESSORY_REOPEN) {
        metrics.SetErrorCode(UEC_ACCESSORY_CAN_NOT_REOPEN);
        ThrowBusinessError(UEC_ACCESSORY_CAN_NOT_REOPEN, "");
    } else {
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION, "");
    }
    return handler;
}

void closeAccessory(USBAccessoryHandle const &accessoryHandle)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.Static.CloseAccessory");
    if (!HasFeature(FEATURE_DEVICE)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return;
    }
    auto accessoryFd = accessoryHandle.accessoryFd;
    if (accessoryFd != g_accFd && g_accFd != 0) {
        metrics.SetErrorCode(OHEC_COMMON_PARAM_ERROR);
        ThrowBusinessError(OHEC_COMMON_PARAM_ERROR, "Parameter accessoryHandle error, need openAccessory first.");
        return;
    }
    close(accessoryFd);
    accessoryFd = 0;
    int32_t ret = g_usbClient.CloseAccessory(g_accFd);
    g_accFd = 0;
    if (ret != OHOS::USB::UEC_OK) {
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION, "Service exception");
    }
    return;
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
        case OHOS::USB::UEC_SERVICE_PERMISSION_DENIED:
            return UEC_COMMON_HAS_NO_RIGHT;
        default:
            return USB_SUBMIT_TRANSFER_OTHER_ERROR;
    }
}

struct USBTransferAsyncContext {
    ani_vm *vm;
    OHOS::USB::USBDevicePipe pipe;
    int32_t endpoint;
    int32_t flags;
    int32_t type;
    int32_t status;
    int32_t timeOut = 0;
    int32_t length;
    int32_t actualLength;
    size_t bufferLength = 0;
    sptr<Ashmem> ashmem = nullptr;
    uint8_t *userData;
    uint8_t *buffer;
    uint32_t numIsoPackets;
    std::string name = "CreateAshmem";
    ani_object callbackRef;
};

static bool CreateAndWriteAshmem(USBTransferAsyncContext *asyncContext, OHOS::HDI::Usb::V1_2::USBTransferInfo &obj)
{
    asyncContext->ashmem = Ashmem::CreateAshmem(asyncContext->name.c_str(), asyncContext->length);
    if (asyncContext->ashmem == nullptr) {
        return false;
    }
    uint8_t endpointId = static_cast<uint8_t>(asyncContext->endpoint) & OHOS::USB::USB_ENDPOINT_DIR_MASK;
    if (endpointId == OHOS::USB::USB_ENDPOINT_DIR_OUT) {
        std::vector<uint8_t> bufferData(asyncContext->buffer, asyncContext->buffer + asyncContext->bufferLength);
        obj.length = static_cast<int32_t>(bufferData.size());
        asyncContext->ashmem->MapReadAndWriteAshmem();
        if (!asyncContext->ashmem->WriteToAshmem(asyncContext->buffer, bufferData.size(), 0)) {
            asyncContext->ashmem->CloseAshmem();
            return false;
        }
    }
    return true;
}

static void ReadDataToBuffer(USBTransferAsyncContext *asyncContext, const OHOS::USB::TransferCallbackInfo &info)
{
    uint8_t endpointId = static_cast<uint8_t>(asyncContext->endpoint) & OHOS::USB::USB_ENDPOINT_DIR_MASK;
    if (endpointId == OHOS::USB::USB_ENDPOINT_DIR_IN) {
        asyncContext->ashmem->MapReadAndWriteAshmem();
        auto ashmemBuffer = asyncContext->ashmem->ReadFromAshmem(info.actualLength, 0);
        if (ashmemBuffer == nullptr) {
            asyncContext->ashmem->UnmapAshmem();
            asyncContext->ashmem->CloseAshmem();
            return;
        }
        int32_t ret = memcpy_s(asyncContext->buffer, asyncContext->bufferLength, ashmemBuffer, info.actualLength);
        if (ret != EOK) {
            USB_HILOGE(MODULE_USB_NAPI, "memcpy_s fatal failed error: %{public}d", ret);
        }
    }
    asyncContext->ashmem->UnmapAshmem();
    asyncContext->ashmem->CloseAshmem();
}

static ani_object GetDefaultBusinessError(ani_env *env)
{
    static const char *businessErrorName = "@ohos.base.BusinessError";
    ani_class cls;
    auto status = env->FindClass(businessErrorName, &cls);
    if (ANI_OK != status) {
        USB_HILOGE(MODULE_USB_NAPI, "Not found class '%{public}s' errcode %{public}d.", businessErrorName, status);
        return nullptr;
    }
    ani_method ctor;
    status = env->Class_FindMethod(cls, "<ctor>", ":", &ctor);
    if (ANI_OK != status) {
        USB_HILOGE(MODULE_USB_NAPI, "Not found ctor of '%{public}s' errcode %{public}d.", businessErrorName, status);
        return nullptr;
    }
    ani_object businessErrorObject;
    status = env->Object_New(cls, ctor, &businessErrorObject);
    if (ANI_OK != status) {
        USB_HILOGE(MODULE_USB_NAPI, "Can not create business error errcode %{public}d.", status);
        return nullptr;
    }
    return businessErrorObject;
}

static thread_local std::shared_ptr<OHOS::AppExecFwk::EventHandler> mainHandler = nullptr;

static bool SendEventToMainThread(const std::function<void()> func)
{
    if (func == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "func is nullptr!");
        return false;
    }

    if (!mainHandler) {
        auto runner = OHOS::AppExecFwk::EventRunner::GetMainEventRunner();
        if (!runner) {
            USB_HILOGE(MODULE_USB_NAPI, "get main event runner failed!");
            return false;
        }
        mainHandler = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
    }
    mainHandler->PostTask(func, "", 0, OHOS::AppExecFwk::EventQueue::Priority::HIGH, {});
    return true;
}

static void DeleteCallback(USBTransferAsyncContext* context)
{
    if (context == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "%{public}s: context is nullptr", __func__);
        return;
    }
    ani_env* env = ::taihe::get_env();
    if (env == nullptr || context->callbackRef == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "%{public}s: env/callbackRef is nullptr", __func__);
        delete context;
        return;
    }
    if (env->GlobalReference_Delete(context->callbackRef) != ANI_OK) {
        USB_HILOGE(MODULE_USB_NAPI, "%{public}s: delete context.callbackRef is failed", __func__);
    }
    delete context;
}

static constexpr int32_t LOCAL_SCOPE_SIZE = 16;
static void AniCallBack(USBTransferAsyncContext *asyncContext, const OHOS::USB::TransferCallbackInfo &info,
    const std::vector<OHOS::HDI::Usb::V1_2::UsbIsoPacketDescriptor> &isoInfo)
{
    USB_HILOGI(MODULE_USB_NAPI, "AniCallBack enter.");
    using ::ohos::usbManager::UsbTransferStatus;
    ReadDataToBuffer(asyncContext, info);
    int32_t actualLength = info.actualLength;
    UsbTransferStatus status((UsbTransferStatus::key_t)info.status);
    std::vector<UsbIsoPacketDescriptor> isoPacketDesc;
    for (const auto &isoInfoItem : isoInfo) {
        USB_HILOGE(MODULE_USB_NAPI, "AniCallBack has iso info.");
        UsbIsoPacketDescriptor descriptor = {
            isoInfoItem.isoActualLength, isoInfoItem.isoLength, {(UsbTransferStatus::key_t)isoInfoItem.isoStatus}};
        isoPacketDesc.push_back(descriptor);
    }
    ::taihe::array<UsbIsoPacketDescriptor> isoPacketDescs(isoPacketDesc);
    ::ohos::usbManager::SubmitTransferCallback callbackParam = {
        std::move(actualLength), std::move(status), std::move(isoPacketDescs)};
    auto task = [asyncContext, callbackParam]() {
        ani_env *env = nullptr;
        ani_options aniArgs {0, nullptr};
        if (ANI_ERROR == asyncContext->vm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &env)) {
            if (ANI_OK != asyncContext->vm->GetEnv(ANI_VERSION_1, &env)) {
                USB_HILOGI(MODULE_USB_NAPI, "GetEnv failed.");
                DeleteCallback(asyncContext);
                return;
            }
        }
        if (ANI_OK != env->CreateLocalScope(LOCAL_SCOPE_SIZE)) {
            USB_HILOGI(MODULE_USB_NAPI, "CreateLocalScope failed.");
            DeleteCallback(asyncContext);
            return;
        }
        auto businessError = GetDefaultBusinessError(env);
        auto param = ::taihe::into_ani<::ohos::usbManager::SubmitTransferCallback>(env, callbackParam);
        auto callbackFunc = asyncContext->callbackRef;
        ani_ref ani_argv[] = {businessError, param};
        ani_ref ani_result;
        ani_class cls;
        if (ANI_OK != env->FindClass("std.core.Function2", &cls)) {
            USB_HILOGE(MODULE_USB_NAPI, "%{public}s: FindClass failed.", __func__);
            DeleteCallback(asyncContext);
            env->DestroyLocalScope();
            return;
        }
        ani_boolean result;
        ani_status ret = env->Object_InstanceOf(callbackFunc, cls, &result);
        if (!result || ret != ANI_OK) {
            USB_HILOGE(MODULE_USB_NAPI, "%{public}s: callbackFunc is not instance Of Function2.", __func__);
            DeleteCallback(asyncContext);
            env->DestroyLocalScope();
            return;
        }
        auto errCode = env->FunctionalObject_Call(static_cast<ani_fn_object>(callbackFunc), 2, ani_argv, &ani_result);
        if (errCode != ANI_OK) {
            USB_HILOGE(MODULE_USB_NAPI, "AniCallBack FunctionalObject_Call returned %{public}d.", errCode);
        }
        DeleteCallback(asyncContext);
        env->DestroyLocalScope();
    };
    if (!SendEventToMainThread(task)) {
        USB_HILOGI(MODULE_USB_NAPI, "SendEventToMainThread failed.");
        DeleteCallback(asyncContext);
    }
}

USBTransferAsyncContext* CreateTransferContext(const UsbDataTransferParams& transfer)
{
    auto context = new (std::nothrow) USBTransferAsyncContext();
    if (!context) {
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return nullptr;
    }
    context->pipe.SetBusNum(static_cast<uint8_t>(transfer.devPipe.busNum));
    context->pipe.SetDevAddr(static_cast<uint8_t>(transfer.devPipe.devAddress));
    context->flags = transfer.flags;
    context->endpoint = transfer.endpoint;
    context->type = transfer.type;
    context->timeOut = transfer.timeout;
    context->length = transfer.length;
    context->numIsoPackets = transfer.isoPacketCount;
    return context;
}

bool ExtractBufferData(USBTransferAsyncContext* context, ani_object array_obj)
{
    ani_ref buffer;
    ani_env* env = ::taihe::get_env();
    ani_vm* vm = nullptr;
    if (env == nullptr || context->callbackRef == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "%{public}s: env/callbackRef is nullptr", __func__);
        delete context;
        return;
    }

    if (env->GetVM(&vm) != ANI_OK ||
        env->Object_GetFieldByName_Ref(array_obj, "buffer", &buffer) != ANI_OK) {
        return false;
    }
    void* data;
    size_t size;
    if (env->ArrayBuffer_GetInfo(static_cast<ani_arraybuffer>(buffer), &data, &size) != ANI_OK) {
        return false;
    }
    context->vm = vm;
    context->buffer = static_cast<uint8_t*>(data);
    context->bufferLength = size;
    return true;
}

bool SetupCallback(USBTransferAsyncContext* context, ani_object callbackObj)
{
    ani_ref callback;
    ani_env* env = ::taihe::get_env();
    if (env == nullptr || context->callbackRef == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "%{public}s: env/callbackRef is nullptr", __func__);
        delete context;
        return;
    }

    if (env->GlobalReference_Create(callbackObj, &callback) == ANI_OK) {
        context->callbackRef = reinterpret_cast<ani_object>(callback);
        return true;
    } else {
        return false;
    }
}

OHOS::HDI::Usb::V1_2::USBTransferInfo PrepareTransferInfo(
    const UsbDataTransferParams& transfer, USBTransferAsyncContext* context)
{
    OHOS::HDI::Usb::V1_2::USBTransferInfo obj;
    obj.endpoint = transfer.endpoint;
    obj.type = transfer.type;
    obj.timeOut = transfer.timeout;
    obj.length = transfer.length;
    obj.numIsoPackets = transfer.isoPacketCount;
    obj.userData = reinterpret_cast<std::uintptr_t>(context);
    return obj;
}

static void TransferCompleteCallback(
    const OHOS::USB::TransferCallbackInfo& info,
    const std::vector<OHOS::HDI::Usb::V1_2::UsbIsoPacketDescriptor>& isoInfo,
    uint64_t userData)
{
    USBTransferAsyncContext* context = reinterpret_cast<USBTransferAsyncContext*>(userData);
    AniCallBack(context, info, isoInfo);
}

void usbSubmitTransfer(UsbDataTransferParams const &transfer)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.SubmitTransfer");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return;
    }
    USB_HILOGD(MODULE_USB_NAPI, "SubmitTransfer enter");
    auto context = CreateTransferContext(transfer);
    if (!context) {
        return;
    }
    if (!ExtractBufferData(context, reinterpret_cast<ani_object>(transfer.buffer)) ||
        !SetupCallback(context, reinterpret_cast<ani_object>(transfer.callback))) {
        delete context;
        return;
    }
    auto transferInfo = PrepareTransferInfo(transfer, context);
    if (!CreateAndWriteAshmem(context, transferInfo)) {
        DeleteCallback(context);
        return;
    }
    USB_HILOGD(MODULE_USB_NAPI, "CreateAndWriteAshmem OK.");
    int32_t ret = context->pipe.UsbSubmitTransfer(transferInfo, TransferCompleteCallback, context->ashmem);
    USB_HILOGD(MODULE_USB_NAPI, "usbSubmitTransfer ret: %{public}d", ret);
    if (ret != OHOS::USB::UEC_OK) {
        context->ashmem->CloseAshmem();
        DeleteCallback(context);
        metrics.SetErrorCode(UsbSubmitTransferErrorCode(ret));
        ThrowBusinessError(UsbSubmitTransferErrorCode(ret), "");
    }
    USB_HILOGD(MODULE_USB_NAPI, "usbSubmitTransfer fin.");
}

void usbCancelTransfer(UsbDataTransferParams const &transfer)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.UsbCancelTransfer");
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "");
        return;
    }
    OHOS::USB::USBDevicePipe nativePipe;
    nativePipe.SetBusNum(static_cast<uint8_t>(transfer.devPipe.busNum));
    nativePipe.SetDevAddr(static_cast<uint8_t>(transfer.devPipe.devAddress));
    int32_t endpoint = transfer.endpoint;
    int32_t ret = nativePipe.UsbCancelTransfer(endpoint);
    if (ret != OHOS::USB::UEC_OK) {
        metrics.SetErrorCode(UsbSubmitTransferErrorCode(ret));
        ThrowBusinessError(UsbSubmitTransferErrorCode(ret), "");
    }
}

bool resetUsbDevice(::ohos::usbManager::USBDevicePipe const& pipe)
{
    OHOS::USB::UsbApiMetrics metrics("BasicServicesKit.UsbManager.ResetUsbDevice");
    bool result = false;
    if (!HasFeature(FEATURE_HOST)) {
        metrics.SetErrorCode(CAPABILITY_NOT_SUPPORT);
        ThrowBusinessError(CAPABILITY_NOT_SUPPORT, "Capability not supported.");
        return result;
    }
    OHOS::USB::USBDevicePipe pipe_tmp = ConvertUSBDevicePipe(pipe);
    int32_t ret = g_usbClient.ResetDevice(pipe_tmp);
    if (ret == OHOS::USB::UEC_OK) {
        result = true;
    } else if (ret == HDF_DEV_ERR_NO_DEVICE || ret == OHOS::USB::UEC_INTERFACE_NAME_NOT_FOUND) {
        metrics.SetErrorCode(USB_SUBMIT_TRANSFER_NO_DEVICE_ERROR);
        ThrowBusinessError(USB_SUBMIT_TRANSFER_NO_DEVICE_ERROR,
            "BusinessError 14400008:No such device (it may have been disconnected).");
        return result;
    } else if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED) {
        metrics.SetErrorCode(UEC_COMMON_HAS_NO_RIGHT);
        ThrowBusinessError(UEC_COMMON_HAS_NO_RIGHT, "No permission.");
        return result;
    } else if (ret == OHOS::USB::USB_RIGHT_FAILURE) {
        metrics.SetErrorCode(USB_DEVICE_PIPE_CHECK_ERROR);
        ThrowBusinessError(USB_DEVICE_PIPE_CHECK_ERROR, "Check devicePipe failed.");
        return result;
    } else if (ret == OHOS::USB::UEC_SERVICE_INVALID_VALUE) {
        metrics.SetErrorCode(UEC_COMMON_SERVICE_EXCEPTION);
        ThrowBusinessError(UEC_COMMON_SERVICE_EXCEPTION,
            "BusinessError 14400004:Service exception. Possible causes:No accessory is plugged in.");
        return result;
    } else {
        metrics.SetErrorCode(USB_SUBMIT_TRANSFER_OTHER_ERROR);
        ThrowBusinessError(USB_SUBMIT_TRANSFER_OTHER_ERROR, "BusinessError 14400010:Other USB error.");
    }
    return result;
}
} // namespace

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_getDevices(getDevices);
TH_EXPORT_CPP_API_connectDevice(connectDevice);
TH_EXPORT_CPP_API_hasRight(hasRight);
TH_EXPORT_CPP_API_requestRightSync(requestRightSync);
TH_EXPORT_CPP_API_removeRight(removeRight);
TH_EXPORT_CPP_API_addDeviceAccessRight(addDeviceAccessRight);
TH_EXPORT_CPP_API_getFunctionsFromString(getFunctionsFromString);
TH_EXPORT_CPP_API_getStringFromFunctions(getStringFromFunctions);
TH_EXPORT_CPP_API_setDeviceFunctionsSync(setDeviceFunctionsSync);
TH_EXPORT_CPP_API_getDeviceFunctions(getDeviceFunctions);
TH_EXPORT_CPP_API_getPortList(getPortList);
TH_EXPORT_CPP_API_getPortSupportModes(getPortSupportModes);
TH_EXPORT_CPP_API_setPortRoleTypesSync(setPortRoleTypesSync);
TH_EXPORT_CPP_API_addAccessoryRight(addAccessoryRight);
TH_EXPORT_CPP_API_claimInterface(claimInterface);
TH_EXPORT_CPP_API_releaseInterface(releaseInterface);
TH_EXPORT_CPP_API_setConfiguration(setConfiguration);
TH_EXPORT_CPP_API_setInterface(setInterface);
TH_EXPORT_CPP_API_getRawDescriptor(getRawDescriptor);
TH_EXPORT_CPP_API_getFileDescriptor(getFileDescriptor);
TH_EXPORT_CPP_API_usbControlTransferSync(usbControlTransferSync);
TH_EXPORT_CPP_API_bulkTransferSync(bulkTransferSync);
TH_EXPORT_CPP_API_closePipe(closePipe);
TH_EXPORT_CPP_API_hasAccessoryRight(hasAccessoryRight);
TH_EXPORT_CPP_API_requestAccessoryRightSync(requestAccessoryRightSync);
TH_EXPORT_CPP_API_cancelAccessoryRight(cancelAccessoryRight);
TH_EXPORT_CPP_API_getAccessoryList(getAccessoryList);
TH_EXPORT_CPP_API_openAccessory(openAccessory);
TH_EXPORT_CPP_API_closeAccessory(closeAccessory);
TH_EXPORT_CPP_API_usbSubmitTransfer(usbSubmitTransfer);
TH_EXPORT_CPP_API_usbCancelTransfer(usbCancelTransfer);
TH_EXPORT_CPP_API_resetUsbDevice(resetUsbDevice);
// NOLINTEND
