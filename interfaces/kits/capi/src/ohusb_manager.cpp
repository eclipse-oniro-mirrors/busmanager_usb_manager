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

OH_UsbManager_ErrorCode ConvertErrCode(int32_t cppRet)
{
    if (cppRet == OHOS::USB::UEC_OK) {
        return OH_USBMANAGER_SUCCESS;
    }
    if (cppRet == OHOS::USB::UEC_INTERFACE_NO_MEMORY ||
        cppRet == OHOS::USB::UEC_SERVICE_NO_MEMORY) {
        return OH_USBMANAGER_ERROR_NO_MEMORY;
    }
    if (cppRet == OHOS::USB::UEC_INTERFACE_PERMISSION_DENIED ||
        cppRet == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED) {
        return OH_USBMANAGER_ERROR_PERMISSION_DENIED;
    }
    return OH_USBMANAGER_ERROR_SERVICE_EXCEPTION;
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

OH_UsbManager_ErrorCode CopyEndpoints(UsbInterface &cppIface, OH_UsbManager_UsbInterface *out)
{
    auto &cppEps = cppIface.GetEndpoints();
    uint32_t eCount = static_cast<uint32_t>(cppEps.size());
    if (eCount == 0) {
        out->endpoints = nullptr;
        out->endpointCount = 0;
        return OH_USBMANAGER_SUCCESS;
    }

    OH_UsbManager_UsbEndpoint *eps =
        static_cast<OH_UsbManager_UsbEndpoint *>(calloc(eCount, sizeof(OH_UsbManager_UsbEndpoint)));
    if (eps == nullptr) {
        return OH_USBMANAGER_ERROR_NO_MEMORY;
    }

    for (uint32_t e = 0; e < eCount; ++e) {
        const USBEndpoint &cppEp = cppEps[e];
        eps[e].address = static_cast<uint8_t>(cppEp.GetAddress());
        eps[e].attributes = static_cast<uint8_t>(cppEp.GetAttributes());
        eps[e].interval = static_cast<uint8_t>(cppEp.GetInterval());
        eps[e].maxPacketSize = static_cast<uint16_t>(cppEp.GetMaxPacketSize());
        eps[e].direction = static_cast<OH_UsbManager_RequestDirection>(cppEp.GetDirection());
        eps[e].number = cppEp.GetNumber();
        eps[e].type = static_cast<uint8_t>(cppEp.GetType());
        eps[e].interfaceId = static_cast<uint8_t>(cppEp.GetInterfaceId());
    }
    out->endpoints = eps;
    out->endpointCount = eCount;
    return OH_USBMANAGER_SUCCESS;
}

void FreeInterfaces(OH_UsbManager_UsbInterface *ifaces, uint32_t count)
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

OH_UsbManager_ErrorCode CopyInterfaces(USBConfig &cppCfg, OH_UsbManager_UsbConfig *out)
{
    auto &cppIfaces = cppCfg.GetInterfaces();
    uint32_t iCount = static_cast<uint32_t>(cppIfaces.size());
    if (iCount == 0) {
        out->interfaces = nullptr;
        out->interfaceCount = 0;
        return OH_USBMANAGER_SUCCESS;
    }

    OH_UsbManager_UsbInterface *ifaces =
        static_cast<OH_UsbManager_UsbInterface *>(calloc(iCount, sizeof(OH_UsbManager_UsbInterface)));
    if (ifaces == nullptr) {
        return OH_USBMANAGER_ERROR_NO_MEMORY;
    }

    for (uint32_t i = 0; i < iCount; ++i) {
        UsbInterface &cppIface = cppIfaces[i];
        ifaces[i].id = static_cast<uint8_t>(cppIface.GetId());
        ifaces[i].protocol = static_cast<uint8_t>(cppIface.GetProtocol());
        ifaces[i].clazz = static_cast<uint8_t>(cppIface.GetClass());
        ifaces[i].subClass = static_cast<uint8_t>(cppIface.GetSubClass());
        ifaces[i].alternateSetting = static_cast<uint8_t>(cppIface.GetAlternateSetting());
        ifaces[i].name = StrdupFromStd(cppIface.GetName());

        OH_UsbManager_ErrorCode epRet = CopyEndpoints(cppIface, &ifaces[i]);
        if (epRet != OH_USBMANAGER_SUCCESS) {
            FreeInterfaces(ifaces, i + 1);
            return epRet;
        }
    }
    out->interfaces = ifaces;
    out->interfaceCount = iCount;
    return OH_USBMANAGER_SUCCESS;
}

void FreeConfigsArray(OH_UsbManager_UsbConfig *cfgs, uint32_t count)
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

OH_UsbManager_ErrorCode CopyConfigs(OHOS::USB::UsbDevice &cppDev, OH_UsbManager_UsbDevice *out)
{
    auto &cppConfigs = cppDev.GetConfigs();
    uint32_t cCount = static_cast<uint32_t>(cppConfigs.size());
    if (cCount == 0) {
        out->configs = nullptr;
        out->configCount = 0;
        return OH_USBMANAGER_SUCCESS;
    }

    OH_UsbManager_UsbConfig *cfgs =
        static_cast<OH_UsbManager_UsbConfig *>(calloc(cCount, sizeof(OH_UsbManager_UsbConfig)));
    if (cfgs == nullptr) {
        return OH_USBMANAGER_ERROR_NO_MEMORY;
    }

    for (uint32_t c = 0; c < cCount; ++c) {
        USBConfig &cppCfg = cppConfigs[c];
        cfgs[c].id = static_cast<uint8_t>(cppCfg.GetId());
        cfgs[c].attributes = static_cast<uint8_t>(cppCfg.GetAttributes());
        cfgs[c].maxPower = static_cast<uint8_t>(cppCfg.GetMaxPower());
        cfgs[c].name = StrdupFromStd(cppCfg.GetName());
        cfgs[c].isRemoteWakeup = cppCfg.IsRemoteWakeup();
        cfgs[c].isSelfPowered = cppCfg.IsSelfPowered();

        OH_UsbManager_ErrorCode ifaceRet = CopyInterfaces(cppCfg, &cfgs[c]);
        if (ifaceRet != OH_USBMANAGER_SUCCESS) {
            FreeConfigsArray(cfgs, c + 1);
            return ifaceRet;
        }
    }

    out->configs = cfgs;
    out->configCount = cCount;
    return OH_USBMANAGER_SUCCESS;
}
}

#ifdef __cplusplus
extern "C" {
#endif

OH_UsbManager_ErrorCode OH_UsbManager_GetUsbDeviceList(OH_UsbManager_UsbDevice **devices, uint32_t *deviceCount)
{
    if (devices == nullptr || deviceCount == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "devices or deviceCount is null");
        return OH_USBMANAGER_ERROR_INVALID_PARAMETER;
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
        return OH_USBMANAGER_SUCCESS;
    }

    uint32_t count = static_cast<uint32_t>(deviceList.size());
    OH_UsbManager_UsbDevice *arr =
        static_cast<OH_UsbManager_UsbDevice *>(calloc(count, sizeof(OH_UsbManager_UsbDevice)));
    if (arr == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "calloc failed for device array");
        return OH_USBMANAGER_ERROR_NO_MEMORY;
    }

    for (uint32_t i = 0; i < count; ++i) {
        OHOS::USB::UsbDevice &cppDev = deviceList[i];
        arr[i].busNum = cppDev.GetBusNum();
        arr[i].devAddress = cppDev.GetDevAddr();
        arr[i].name = StrdupFromStd(cppDev.GetName());
        arr[i].manufacturerName = StrdupFromStd(cppDev.GetManufacturerName());
        arr[i].productName = StrdupFromStd(cppDev.GetProductName());
        arr[i].version = StrdupFromStd(cppDev.GetVersion());
        arr[i].vendorId = static_cast<uint16_t>(cppDev.GetVendorId());
        arr[i].productId = static_cast<uint16_t>(cppDev.GetProductId());
        arr[i].clazz = static_cast<uint8_t>(cppDev.GetClass());
        arr[i].subClass = static_cast<uint8_t>(cppDev.GetSubclass());
        arr[i].protocol = static_cast<uint8_t>(cppDev.GetProtocol());

        OH_UsbManager_ErrorCode cfgRet = CopyConfigs(cppDev, &arr[i]);
        if (cfgRet != OH_USBMANAGER_SUCCESS) {
            USB_HILOGE(MODULE_USB_INNERKIT, "CopyConfigs failed for device %{public}d", i);
            OH_UsbManager_FreeUsbDeviceList(arr, i + 1);
            return cfgRet;
        }
    }

    *devices = arr;
    *deviceCount = count;
    return OH_USBMANAGER_SUCCESS;
}

void OH_UsbManager_FreeUsbDeviceList(OH_UsbManager_UsbDevice *devices, uint32_t deviceCount)
{
    if (devices == nullptr || deviceCount == 0) {
        return;
    }
    for (uint32_t i = 0; i < deviceCount; ++i) {
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

OH_UsbManager_ErrorCode OH_UsbManager_ConnectDevice(const OH_UsbManager_UsbDevice *device,
    OH_UsbManager_UsbPipe *pipe)
{
    if (device == nullptr || pipe == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "device or pipe is null");
        return OH_USBMANAGER_ERROR_INVALID_PARAMETER;
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
    return OH_USBMANAGER_SUCCESS;
}

OH_UsbManager_ErrorCode OH_UsbManager_HasPermission(const char *deviceName, bool *result)
{
    if (deviceName == nullptr || result == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "deviceName or result is null");
        return OH_USBMANAGER_ERROR_INVALID_PARAMETER;
    }
    *result = false;
    bool hasRight = false;
    int32_t ret = OHOS::USB::UsbSrvClient::GetInstance().HasRightEx(std::string(deviceName), hasRight);
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "HasRight service exception, ret=%{public}d", ret);
        return OH_USBMANAGER_ERROR_SERVICE_EXCEPTION;
    }
    *result = hasRight;
    return OH_USBMANAGER_SUCCESS;
}

OH_UsbManager_ErrorCode OH_UsbManager_RequestPermission(const char *deviceName,
    OH_UsbManager_PermissionCallback callback, void *userContext)
{
    if (deviceName == nullptr || callback == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "deviceName or callback is null");
        return OH_USBMANAGER_ERROR_INVALID_PARAMETER;
    }
    std::string name(deviceName);
    std::thread([callback, userContext, name = std::move(name)]() {
        int32_t ret = OHOS::USB::UsbSrvClient::GetInstance().RequestRight(name);
        OH_UsbManager_ErrorCode errCode;
        bool result = false;
        if (ret == OHOS::USB::UEC_OK) {
            errCode = OH_USBMANAGER_SUCCESS;
            result = true;
        } else if (ret == OHOS::USB::UEC_SERVICE_PERMISSION_DENIED) {
            errCode = OH_USBMANAGER_SUCCESS;
            result = false;
        } else {
            USB_HILOGE(MODULE_USB_INNERKIT, "RequestRight failed, ret=%{public}d", ret);
            errCode = OH_USBMANAGER_ERROR_SERVICE_EXCEPTION;
            result = false;
        }
        callback(errCode, result, userContext);
    }).detach();
    return OH_USBMANAGER_SUCCESS;
}

OH_UsbManager_ErrorCode OH_UsbManager_GetFileDescriptor(const OH_UsbManager_UsbPipe *pipe, int32_t *fd)
{
    if (pipe == nullptr || fd == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "pipe or fd is null");
        return OH_USBMANAGER_ERROR_INVALID_PARAMETER;
    }

    OHOS::USB::USBDevicePipe cppPipe(pipe->busNum, pipe->devAddress);
    int32_t cppFd = -1;
    int32_t ret = OHOS::USB::UsbSrvClient::GetInstance().GetFileDescriptor(cppPipe, cppFd);
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "GetFileDescriptor failed, ret=%{public}d", ret);
        return ConvertErrCode(ret);
    }
    *fd = cppFd;
    return OH_USBMANAGER_SUCCESS;
}

OH_UsbManager_ErrorCode OH_UsbManager_ClosePipe(const OH_UsbManager_UsbPipe *pipe)
{
    if (pipe == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "pipe is null");
        return OH_USBMANAGER_ERROR_INVALID_PARAMETER;
    }

    OHOS::USB::USBDevicePipe cppPipe(pipe->busNum, pipe->devAddress);
    int32_t ret = OHOS::USB::UsbSrvClient::GetInstance().CloseEx(cppPipe);
    if (ret != OHOS::USB::UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "Close pipe service exception, ret=%{public}d", ret);
        return ConvertErrCode(ret);
    }
    return OH_USBMANAGER_SUCCESS;
}

#ifdef __cplusplus
}
#endif