/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ohusb_manager.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "hilog_wrapper.h"
#include "usb_config.h"
#include "usb_device.h"
#include "usb_device_pipe.h"
#include "usb_endpoint.h"
#include "usb_errors.h"
#include "usb_interface.h"
#include "usb_srv_client.h"

using namespace OHOS::USB;

namespace {
constexpr const char *EMPTY_STRING = "";

UsbManager_ErrorCode ConvertErrCode(int32_t cppRet)
{
    if (cppRet == OHOS::USB::UEC_OK) {
        return USB_MANAGER_SUCCESS;
    }
    if (cppRet == OHOS::USB::UEC_INTERFACE_NO_MEMORY ||
        cppRet == OHOS::USB::UEC_SERVICE_NO_MEMORY) {
        return USB_MANAGER_ERROR_NO_MEMORY;
    }
    if (cppRet == OHOS::USB::UEC_INTERFACE_PERMISSION_DENIED ||
        cppRet == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED) {
        return USB_MANAGER_ERROR_PERMISSION_DENIED;
    }
    return USB_MANAGER_ERROR_SERVICE_EXCEPTION;
}

char *StrdupSafe(const char *src)
{
    if (src == nullptr) {
        return const_cast<char *>(EMPTY_STRING);
    }
    char *dst = strdup(src);
    return (dst != nullptr) ? dst : const_cast<char *>(EMPTY_STRING);
}

char *StrdupFromStd(const std::string &src)
{
    return StrdupSafe(src.c_str());
}

UsbManager_ErrorCode CopyEndpoints(UsbInterface &cppIface, UsbManager_Interface *out)
{
    auto &cppEps = cppIface.GetEndpoints();
    uint32_t eCount = static_cast<uint32_t>(cppEps.size());
    if (eCount == 0) {
        out->endpoints = nullptr;
        out->endpointCount = 0;
        return USB_MANAGER_SUCCESS;
    }

    UsbManager_Endpoint *eps = static_cast<UsbManager_Endpoint *>(calloc(eCount, sizeof(UsbManager_Endpoint)));
    if (eps == nullptr) {
        return USB_MANAGER_ERROR_NO_MEMORY;
    }

    for (uint32_t e = 0; e < eCount; ++e) {
        const USBEndpoint &cppEp = cppEps[e];
        eps[e].address = cppEp.GetAddress();
        eps[e].attributes = cppEp.GetAttributes();
        eps[e].interval = cppEp.GetInterval();
        eps[e].maxPacketSize = cppEp.GetMaxPacketSize();
    }
    out->endpoints = eps;
    out->endpointCount = eCount;
    return USB_MANAGER_SUCCESS;
}

void FreeInterfaces(UsbManager_Interface *ifaces, uint32_t count)
{
    if (ifaces == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (ifaces[i].name != nullptr && ifaces[i].name != EMPTY_STRING) {
            free(const_cast<char *>(ifaces[i].name));
        }
        free(ifaces[i].endpoints);
    }
    free(ifaces);
}

UsbManager_ErrorCode CopyInterfaces(USBConfig &cppCfg, UsbManager_Config *out)
{
    auto &cppIfaces = cppCfg.GetInterfaces();
    uint32_t iCount = static_cast<uint32_t>(cppIfaces.size());
    if (iCount == 0) {
        out->interfaces = nullptr;
        out->interfaceCount = 0;
        return USB_MANAGER_SUCCESS;
    }

    UsbManager_Interface *ifaces = static_cast<UsbManager_Interface *>(calloc(iCount, sizeof(UsbManager_Interface)));
    if (ifaces == nullptr) {
        return USB_MANAGER_ERROR_NO_MEMORY;
    }

    for (uint32_t i = 0; i < iCount; ++i) {
        UsbInterface &cppIface = cppIfaces[i];
        ifaces[i].name = StrdupFromStd(cppIface.GetName());
        ifaces[i].id = cppIface.GetId();
        ifaces[i].clazz = cppIface.GetClass();
        ifaces[i].subClass = cppIface.GetSubClass();
        ifaces[i].alternateSetting = cppIface.GetAlternateSetting();
        ifaces[i].protocol = cppIface.GetProtocol();

        UsbManager_ErrorCode epRet = CopyEndpoints(cppIface, &ifaces[i]);
        if (epRet != USB_MANAGER_SUCCESS) {
            FreeInterfaces(ifaces, i + 1);
            return epRet;
        }
    }
    out->interfaces = ifaces;
    out->interfaceCount = iCount;
    return USB_MANAGER_SUCCESS;
}

void FreeConfigsArray(UsbManager_Config *cfgs, uint32_t count)
{
    if (cfgs == nullptr) {
        return;
    }
    for (uint32_t c = 0; c < count; ++c) {
        if (cfgs[c].name != nullptr && cfgs[c].name != EMPTY_STRING) {
            free(const_cast<char *>(cfgs[c].name));
        }
        FreeInterfaces(cfgs[c].interfaces, cfgs[c].interfaceCount);
    }
    free(cfgs);
}

UsbManager_ErrorCode CopyConfigs(OHOS::USB::UsbDevice &cppDev, UsbManager_Device *out)
{
    auto &cppConfigs = cppDev.GetConfigs();
    uint32_t cCount = static_cast<uint32_t>(cppConfigs.size());
    if (cCount == 0) {
        out->configs = nullptr;
        out->configCount = 0;
        return USB_MANAGER_SUCCESS;
    }

    UsbManager_Config *cfgs = static_cast<UsbManager_Config *>(calloc(cCount, sizeof(UsbManager_Config)));
    if (cfgs == nullptr) {
        return USB_MANAGER_ERROR_NO_MEMORY;
    }

    for (uint32_t c = 0; c < cCount; ++c) {
        USBConfig &cppCfg = cppConfigs[c];
        cfgs[c].name = StrdupFromStd(cppCfg.GetName());
        cfgs[c].id = cppCfg.GetId();
        cfgs[c].attributes = cppCfg.GetAttributes();
        cfgs[c].maxPower = cppCfg.GetMaxPower();

        UsbManager_ErrorCode ifaceRet = CopyInterfaces(cppCfg, &cfgs[c]);
        if (ifaceRet != USB_MANAGER_SUCCESS) {
            FreeConfigsArray(cfgs, c + 1);
            return ifaceRet;
        }
    }

    out->configs = cfgs;
    out->configCount = cCount;
    return USB_MANAGER_SUCCESS;
}
}

#ifdef __cplusplus
extern "C" {
#endif

UsbManager_ErrorCode OH_UsbManager_GetUsbDeviceList(UsbManager_Device **devices, uint32_t *deviceCount)
{
    if (devices == nullptr || deviceCount == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "devices or deviceCount is null");
        return USB_MANAGER_ERROR_INVALID_PARAMETER;
    }
    *devices = nullptr;
    *deviceCount = 0;

    std::vector<OHOS::USB::UsbDevice> deviceList;
    int32_t ret = OHOS::USB::UsbSrvClient::GetInstance().GetDevices(deviceList);
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "GetDevices failed, ret=%{public}d", ret);
        return ConvertErrCode(ret);
    }
    if (deviceList.empty()) {
        USB_HILOGI(MODULE_USB_INNERKIT, "No USB devices found");
        return USB_MANAGER_SUCCESS;
    }

    uint32_t count = static_cast<uint32_t>(deviceList.size());
    UsbManager_Device *arr = static_cast<UsbManager_Device *>(calloc(count, sizeof(UsbManager_Device)));
    if (arr == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "calloc failed for device array");
        return USB_MANAGER_ERROR_NO_MEMORY;
    }

    for (uint32_t i = 0; i < count; ++i) {
        OHOS::USB::UsbDevice &cppDev = deviceList[i];
        arr[i].busNum = cppDev.GetBusNum();
        arr[i].devAddress = cppDev.GetDevAddr();
        arr[i].serial = StrdupFromStd(cppDev.GetmSerial());
        arr[i].name = StrdupFromStd(cppDev.GetName());
        arr[i].manufacturerName = StrdupFromStd(cppDev.GetManufacturerName());
        arr[i].productName = StrdupFromStd(cppDev.GetProductName());
        arr[i].version = StrdupFromStd(cppDev.GetVersion());
        arr[i].vendorId = cppDev.GetVendorId();
        arr[i].productId = cppDev.GetProductId();
        arr[i].clazz = cppDev.GetClass();
        arr[i].subClass = cppDev.GetSubclass();
        arr[i].protocol = cppDev.GetProtocol();

        UsbManager_ErrorCode cfgRet = CopyConfigs(cppDev, &arr[i]);
        if (cfgRet != USB_MANAGER_SUCCESS) {
            USB_HILOGE(MODULE_USB_INNERKIT, "CopyConfigs failed for device %{public}d", i);
            OH_UsbManager_FreeDeviceList(arr, i + 1);
            return cfgRet;
        }
    }

    *devices = arr;
    *deviceCount = count;
    return USB_MANAGER_SUCCESS;
}

void OH_UsbManager_FreeDeviceList(UsbManager_Device *devices, uint32_t deviceCount)
{
    if (devices == nullptr || deviceCount == 0) {
        return;
    }
    for (uint32_t i = 0; i < deviceCount; ++i) {
        if (devices[i].serial != nullptr && devices[i].serial != EMPTY_STRING) {
            free(const_cast<char *>(devices[i].serial));
        }
        if (devices[i].name != nullptr && devices[i].name != EMPTY_STRING) {
            free(const_cast<char *>(devices[i].name));
        }
        if (devices[i].manufacturerName != nullptr && devices[i].manufacturerName != EMPTY_STRING) {
            free(const_cast<char *>(devices[i].manufacturerName));
        }
        if (devices[i].productName != nullptr && devices[i].productName != EMPTY_STRING) {
            free(const_cast<char *>(devices[i].productName));
        }
        if (devices[i].version != nullptr && devices[i].version != EMPTY_STRING) {
            free(const_cast<char *>(devices[i].version));
        }
        FreeConfigsArray(devices[i].configs, devices[i].configCount);
    }
    free(devices);
}

UsbManager_ErrorCode OH_UsbManager_ConnectDevice(const UsbManager_Device *device, UsbManager_DevicePipe *pipe)
{
    if (device == nullptr || pipe == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "device or pipe is null");
        return USB_MANAGER_ERROR_INVALID_PARAMETER;
    }

    OHOS::USB::UsbDevice cppDevice;
    cppDevice.SetBusNum(device->busNum);
    cppDevice.SetDevAddr(device->devAddress);

    OHOS::USB::USBDevicePipe cppPipe;
    int32_t ret = OHOS::USB::UsbSrvClient::GetInstance().OpenDevice(cppDevice, cppPipe);
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "OpenDevice failed, ret=%{public}d", ret);
        return ConvertErrCode(ret);
    }

    pipe->busNum = cppPipe.GetBusNum();
    pipe->devAddress = cppPipe.GetDevAddr();
    return USB_MANAGER_SUCCESS;
}

UsbManager_ErrorCode OH_UsbManager_HasPermission(const char *deviceName, bool *result)
{
    if (deviceName == nullptr || result == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "deviceName or result is null");
        return USB_MANAGER_ERROR_INVALID_PARAMETER;
    }
    *result = false;
    bool hasRight = false;
    int32_t ret = OHOS::USB::UsbSrvClient::GetInstance().HasRightEx(std::string(deviceName), hasRight);
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "HasRight service exception, ret=%{public}d", ret);
        return USB_MANAGER_ERROR_SERVICE_EXCEPTION;
    }
    *result = hasRight;
    return USB_MANAGER_SUCCESS;
}

UsbManager_ErrorCode OH_UsbManager_RequestPermission(const char *deviceName,
    UsbManager_PermissionCallback callback, void *userData)
{
    if (deviceName == nullptr || callback == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "deviceName or callback is null");
        return USB_MANAGER_ERROR_INVALID_PARAMETER;
    }
    std::string name(deviceName);
    std::thread([callback, userData, name = std::move(name)]() {
        int32_t ret = OHOS::USB::UsbSrvClient::GetInstance().RequestRight(name);
        UsbManager_ErrorCode errCode;
        bool result = false;
        if (ret == OHOS::USB::UEC_OK) {
            errCode = USB_MANAGER_SUCCESS;
            result = true;
        } else if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED) {
            errCode = USB_MANAGER_SUCCESS;
            result = false;
        } else {
            USB_HILOGE(MODULE_USB_INNERKIT, "RequestRight failed, ret=%{public}d", ret);
            errCode = USB_MANAGER_ERROR_SERVICE_EXCEPTION;
            result = false;
        }
        callback(errCode, result, userData);
    }).detach();
    return USB_MANAGER_SUCCESS;
}

UsbManager_ErrorCode OH_UsbManager_GetFileDescriptor(const UsbManager_DevicePipe *pipe, int32_t *fd)
{
    if (pipe == nullptr || fd == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "pipe or fd is null");
        return USB_MANAGER_ERROR_INVALID_PARAMETER;
    }

    OHOS::USB::USBDevicePipe cppPipe(pipe->busNum, pipe->devAddress);
    int32_t cppFd = -1;
    int32_t ret = OHOS::USB::UsbSrvClient::GetInstance().GetFileDescriptor(cppPipe, cppFd);
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "GetFileDescriptor failed, ret=%{public}d", ret);
        return ConvertErrCode(ret);
    }
    *fd = cppFd;
    return USB_MANAGER_SUCCESS;
}

UsbManager_ErrorCode OH_UsbManager_ClosePipe(const UsbManager_DevicePipe *pipe)
{
    if (pipe == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "pipe is null");
        return USB_MANAGER_ERROR_INVALID_PARAMETER;
    }

    OHOS::USB::USBDevicePipe cppPipe(pipe->busNum, pipe->devAddress);
    int32_t ret = OHOS::USB::UsbSrvClient::GetInstance().CloseEx(cppPipe);
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "Close pipe service exception, ret=%{public}d", ret);
        return USB_MANAGER_ERROR_SERVICE_EXCEPTION;
    }
    return USB_MANAGER_SUCCESS;
}

#ifdef __cplusplus
}
#endif
