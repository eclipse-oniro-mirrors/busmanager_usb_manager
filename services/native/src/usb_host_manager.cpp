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
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <string>
#include <set>
#include <thread>
#include <ipc_skeleton.h>
#include <iconv.h>

#include "usb_host_manager.h"
#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "hilog_wrapper.h"
#include "hisysevent.h"
#include "cJSON.h"
#include "usb_serial_reader.h"
#include "usb_device.h"
#include "usb_config.h"
#include "usb_interface.h"
#include "usb_errors.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "parameters.h"
#include "usbd_bulkcallback_impl.h"
#include "usb_descriptor_parser.h"
#include "usbd_transfer_callback_impl.h"
#include "usb_napi_errors.h"
#include "accesstoken_kit.h"
#include "usb_connection_notifier.h"
#include "securec.h"

using namespace OHOS::AAFwk;
using namespace OHOS::EventFwk;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HDI::Usb::V1_2;

namespace OHOS {
namespace USB {
constexpr int32_t CLASS_PRINT_LENGTH = 2;
constexpr int32_t USAGE_IN_INTERFACE_CLASS = 0;
constexpr uint8_t DES_USAGE_IN_INTERFACE = 0x02;
constexpr int32_t BCD_HEX_DIGITS = 4;
constexpr int LAST_FIVE = 5;
constexpr int BOM_BYTE_COUNT = 2;
constexpr int INVALID_RET = -1;
constexpr int BYTES_PER_UTF8_CHAR = 4;
std::map<int32_t, DeviceClassUsage> deviceUsageMap = {
    {0x00, {DeviceClassUsage(2, "Use class information in the Interface Descriptors")}},
    {0x01, {DeviceClassUsage(2, "Audio")}},
    {0x02, {DeviceClassUsage(3, "Communications and CDC Control")}},
    {0x03, {DeviceClassUsage(2, "HID(Human Interface Device)")}},
    {0x05, {DeviceClassUsage(2, "Physical")}},
    {0x06, {DeviceClassUsage(2, "Image")}},
    {0x07, {DeviceClassUsage(2, "Printer")}},
    {0x08, {DeviceClassUsage(2, "Mass Storage")}},
    {0x09, {DeviceClassUsage(1, "Hub")}},
    {0x0a, {DeviceClassUsage(2, "CDC-Data")}},
    {0x0b, {DeviceClassUsage(2, "Smart Card")}},
    {0x0d, {DeviceClassUsage(2, "Content Security")}},
    {0x0e, {DeviceClassUsage(2, "Video")}},
    {0x0f, {DeviceClassUsage(2, "Personal Healthcare")}},
    {0x10, {DeviceClassUsage(2, "Audio/Video Device")}},
    {0x11, {DeviceClassUsage(1, "Billboard Device Class")}},
    {0x12, {DeviceClassUsage(2, "USB Type-C Bridge Class")}}
};

std::map<UsbDeviceType, std::string> interfaceUsageMap = {
    {{UsbDeviceType(0x03, 0x01, 0x01, 0)}, "KeyBoard"},
    {{UsbDeviceType(0x03, 0x01, 0x02, 0)}, "Mouse/Table/Touch screen"},
};
constexpr uint32_t CURSOR_INIT = 18;
constexpr int32_t DESCRIPTOR_TYPE_STRING = 3;
constexpr int32_t DESCRIPTOR_VALUE_START_OFFSET = 2;
constexpr int32_t HALF = 2;
constexpr uint32_t MANAGE_INTERFACE_INTERVAL = 50;
constexpr uint32_t EDM_SA_MAX_TIME_OUT = 5000;
constexpr uint32_t EDM_SYSTEM_ABILITY_ID = 1601;
const std::u16string DESCRIPTOR = u"ohos.edm.IEnterpriseDeviceMgr";
constexpr uint32_t WITHOUT_USERID = 0;
constexpr uint32_t WITHOUT_ADMIN = 1;
constexpr uint32_t EMD_MASK_CODE = 20;
constexpr uint32_t DISABLE_USB = 1043;
constexpr uint32_t ALLOWED_USB_DEVICES = 1044;
constexpr uint32_t USB_STORAGE_DEVICE_ACCESS_POLICY = 1026;
constexpr uint32_t USB_DEVICE_ACCESS_POLICY = 1059;
constexpr uint32_t USB_TYPE_DISALLOW_POLICY = 1166;
constexpr int32_t TRUSTLIST_POLICY_MAX_DEVICES = 1000;
constexpr uint32_t EDM_SA_TIME_OUT_CODE = 9200007;
constexpr int32_t BASECLASS_INDEX = 0;
constexpr int32_t SUBCLASS_INDEX = 1;
constexpr int32_t PROTOCAL_INDEX = 2;
constexpr int32_t STORAGE_BASE_CLASS = 8;
constexpr int32_t GET_EDM_STORAGE_DISABLE_TYPE = 2;
constexpr int32_t RANDOM_VALUE_INDICATE = -1;
constexpr int32_t BASE_CLASS_AUDIO = 0x01;
constexpr int32_t BASE_CLASS_HUB = 0x09;
constexpr int32_t RETRY_NUM = 10;
constexpr uint32_t RETRY_INTERVAL = 100;
constexpr uint32_t USB_PATH_LENGTH = 64;
constexpr const char* USB_DEV_FS_PATH = "/dev/bus/usb";
#ifdef USB_MANAGER_PASS_THROUGH
const std::string SERVICE_NAME = "usb_host_interface_service";
#endif // USB_MANAGER_PASS_THROUGH
UsbHostManager::UsbHostManager(SystemAbility *systemAbility)
{
    systemAbility_ = systemAbility;
    usbRightManager_ = std::make_shared<UsbRightManager>();
#ifndef USB_MANAGER_PASS_THROUGH
    usbd_ = OHOS::HDI::Usb::V1_2::IUsbInterface::Get();
    USB_HILOGI(MODULE_USB_HOST, "%{public}s:%{public}d usbd_ == nullptr: %{public}d",
        __func__, __LINE__, usbd_ == nullptr);
#endif // USB_MANAGER_PASS_THROUGH
}

UsbHostManager::~UsbHostManager()
{
    std::unique_lock lock(devicesMutex_);
    for (auto &pair : devices_) {
        delete pair.second;
    }
    devices_.clear();
}

#ifdef USB_MANAGER_PASS_THROUGH
bool UsbHostManager::InitUsbHostInterface()
{
    USB_HILOGI(MODULE_USB_HOST, "InitUsbHostInterface in");
    usbDeviceInterface_ = HDI::Usb::V2_0::IUsbDeviceInterface::Get();
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "InitUsbDeviceInterface get usbDeviceInterface_ is nullptr");
        return false;
    }
    usbHostInterface_ = HDI::Usb::V2_0::IUsbHostInterface::Get(SERVICE_NAME, true);
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "InitUsbHostInterface get usbHostInterface_ is nullptr");
        return false;
    }
    usbManagerSubscriber_ = new (std::nothrow) UsbManagerSubscriber();
    if (usbManagerSubscriber_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "usbManagerSubscriber_ is nullptr");
        return false;
    }
    ErrCode ret = usbHostInterface_->BindUsbdHostSubscriber(usbManagerSubscriber_);
    USB_HILOGI(MODULE_USB_HOST, "entry InitUsbHostInterface ret: %{public}d", ret);
    return SUCCEEDED(ret);
}

void UsbHostManager::Stop()
{
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbHostInterface_ is nullptr");
        return;
    }
    usbHostInterface_->UnbindUsbdHostSubscriber(usbManagerSubscriber_);
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 0, USB_SYSTEM_ABILITY_ID);
}

int32_t UsbHostManager::BindUsbdSubscriber(const sptr<HDI::Usb::V2_0::IUsbdSubscriber> &subscriber)
{
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::BindUsbdSubscriber usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostInterface_->BindUsbdHostSubscriber(subscriber);
}

int32_t UsbHostManager::UnbindUsbdSubscriber(const sptr<HDI::Usb::V2_0::IUsbdSubscriber> &subscriber)
{
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::UnbindUsbdSubscriber usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostInterface_->UnbindUsbdHostSubscriber(subscriber);
}
#endif // USB_MANAGER_PASS_THROUGH

// LCOV_EXCL_START
void UsbHostManager::UsbSubmitTransferDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager UsbSubmitTransferDeathRecipient enter");
    int32_t ret = service_->UsbCancelTransfer(devInfo_, endpoint_);
    if (ret == UEC_OK) {
        USB_HILOGI(MODULE_USB_HOST, "UsbHostManager OnRemoteDied Close.");
        service_->Close(devInfo_.busNum, devInfo_.devAddr);
    }
}

void UsbHostManager::UsbEdmLoadCallback::OnLoadSystemAbilitySuccess(
    int32_t systemAbilityId, const sptr<IRemoteObject>& remoteObject)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager Load SA success, systemAbilityId = [%{public}d]", systemAbilityId);
    usbHostManager_ -> ExecuteStrategy();
}

void UsbHostManager::UsbEdmLoadCallback::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager Load SA failed, systemAbilityId = [%{public}d]", systemAbilityId);
}
// LCOV_EXCL_STOP

void UsbHostManager::ExecuteStrategy()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager::ExecuteStrategy start");
    if (!IsEdmEnabled()) {
        USB_HILOGE(MODULE_USB_HOST, "edm is not activate, skip");
        return;
    }
    bool isGlobalDisabled = false;
    std::vector<UsbDeviceType> disableType{};
    std::vector<UsbDeviceId> trustUsbDeviceIds{};

    int32_t ret = GetUsbPolicy(isGlobalDisabled, disableType, trustUsbDeviceIds);
    if (ret == UEC_SERVICE_EDM_SA_TIME_OUT_FAILED || ret == UEC_SERVICE_PREPARE_EDM_SA_FAILED) {
        USB_HILOGE(MODULE_USB_HOST, "EDM sa time out or prepare failed, ret = %{public}d", ret);
        return;
    }

    if (isGlobalDisabled) {
        ret = ManageGlobalInterfaceImpl(isGlobalDisabled);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ManageGlobalInterface failed");
        }
        return;
    }

    if (IsUsbSerialDisable()) {
        ret = ManageUsbSerialDevice(true);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ManageUsbSerialDevice failed");
        }
    }

    if (!disableType.empty()) {
        ret = isPermTypePolicy_? ManageUsbType(disableType, true) : ExecuteManageInterfaceType(disableType, true);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ExecuteManageInterfaceType failed");
        }
        return;
    }

    if (trustUsbDeviceIds.empty()) {
        USB_HILOGI(MODULE_USB_HOST, "trustUsbDeviceIds is empty, no devices disable");
        return;
    }
    ret = ExecuteManageDevicePolicy(trustUsbDeviceIds);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "ExecuteManageDevicePolicy failed");
    }
    return;
}

int32_t UsbHostManager::OpenDevice(uint8_t busNum, uint8_t devAddr)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::OpenDevice usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->OpenDevice(dev);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->OpenDevice(dev);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::Close(uint8_t busNum, uint8_t devAddr)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::Close usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->CloseDevice(dev);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->CloseDevice(dev);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::ResetDevice(uint8_t busNum, uint8_t devAddr)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::ResetDevice usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->ResetDevice(dev);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->ResetDevice(dev);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::ClaimInterface(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid, uint8_t force)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::ClaimInterface usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->ClaimInterface(dev, interfaceid, force);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->ClaimInterface(dev, interfaceid, force);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::SetInterface(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid, uint8_t altIndex)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::SetInterface usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->SetInterface(dev, interfaceid, altIndex);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->SetInterface(dev, interfaceid, altIndex);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::ReleaseInterface(uint8_t busNum, uint8_t devAddr, uint8_t interface)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::ReleaseInterface usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->ReleaseInterface(dev, interface);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->ReleaseInterface(dev, interface);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::SetActiveConfig(uint8_t busNum, uint8_t devAddr, uint8_t configIndex)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::SetActiveConfig usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->SetConfig(dev, configIndex);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->SetConfig(dev, configIndex);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::ManageGlobalInterface(bool disable)
{
    return ManageGlobalInterfaceImpl(disable);
}

int32_t UsbHostManager::ManageDevice(int32_t vendorId, int32_t productId, bool disable)
{
    std::shared_lock lock(devicesMutex_);
    return ManageDeviceImpl(vendorId, productId, disable);
}

int32_t UsbHostManager::ManageDevicePolicy(std::vector<UsbDeviceId> &trustList)
{
    return ExecuteManageDevicePolicy(trustList);
}

int32_t UsbHostManager::ManageInterfaceType(const std::vector<UsbDeviceType> &disableType, bool disable)
{
    return ExecuteManageInterfaceType(disableType, disable);
}

int32_t UsbHostManager::ManageUsbType(const std::vector<UsbDeviceType> &disableType, bool disable)
{
    ExecuteManageUsbType(disableType, disable);
    return UEC_OK;
}

int32_t UsbHostManager::UsbAttachKernelDriver(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::UsbAttachKernelDriver usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->ManageInterface(dev, interfaceid, false);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    return usbd_->ManageInterface(dev, interfaceid, false);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::UsbDetachKernelDriver(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::UsbDetachKernelDriver usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->ManageInterface(dev, interfaceid, true);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    return usbd_->ManageInterface(dev, interfaceid, true);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::ClearHalt(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId, uint8_t endpointId)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::ClearHalt usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    const HDI::Usb::V2_0::UsbPipe pipe = {interfaceId, endpointId};
    return usbHostInterface_->ClearHalt(dev, pipe);
#else
    const UsbDev dev = {busNum, devAddr};
    const UsbPipe pipe = {interfaceId, endpointId};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "ClearHalt: usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->ClearHalt(dev, pipe);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::GetDevices(std::vector<UsbDevice> &deviceList)
{
    std::shared_lock lock(devicesMutex_);
    USB_HILOGI(MODULE_USB_HOST, "list size %{public}zu", devices_.size());
    bool isSystemAppOrSa = usbRightManager_->IsSystemAppOrSa();
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if ((it->second->GetClass() == BASE_CLASS_HUB && !isSystemAppOrSa) ||
            it->second->GetAuthorizeStatus() != ENABLED) {
            continue;
        }
        auto dev = UsbDevice(*it->second);
        if (!(isSystemAppOrSa)) {
            dev.SetmSerial("");
        }
        deviceList.push_back(dev);
    }
    return UEC_OK;
}

int32_t UsbHostManager::CheckDevPathIsExist(uint8_t busNum, uint8_t devAddr)
{
    char path[USB_PATH_LENGTH] = {"\0"};
    int32_t ret = sprintf_s(path, sizeof(path), "%s/%03u/%03u", USB_DEV_FS_PATH, busNum, devAddr);
    if (ret < UEC_OK) {
        USB_HILOGW(MODULE_USB_HOST, "check dev path, sprintf_s failed, ret: %{public}d, path, %{public}s. ",
            ret, path);
        return UEC_SERVICE_INVALID_VALUE;
    }

    ret = access(path, F_OK);
    if (ret != UEC_OK) {
        USB_HILOGW(MODULE_USB_HOST, "check dev path, path not exist, ret: %{public}d, path, %{public}s. ",
            ret, path);
        return UEC_SERVICE_INNER_ERR;
    }

    ret = access(path, R_OK | W_OK);
    if (ret != UEC_OK) {
        USB_HILOGW(MODULE_USB_HOST, "has no read or write permission, ret: %{public}d, path, %{public}s. ",
            ret, path);
        return UEC_SERVICE_INNER_ERR;
    }
    return UEC_OK;
}

int32_t UsbHostManager::GetDeviceInfo(uint8_t busNum, uint8_t devAddr, UsbDevice &dev)
{
    const UsbDev uDev = {busNum, devAddr};
    std::vector<uint8_t> descriptor;

    int32_t res = UEC_OK;
    int32_t ret = UEC_OK;
    for (int32_t i = 0; i < RETRY_NUM; i++) {
        ret = CheckDevPathIsExist(busNum, devAddr);
        if (ret == UEC_OK) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_INTERVAL));
    }
    ret = OpenDevice(busNum, devAddr);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetDeviceInfo OpenDevice failed ret=%{public}d", ret);
        return ret;
    }
    ret = GetDeviceInfoDescriptor(uDev, descriptor, dev);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetDeviceInfoDescriptor ret=%{public}d", ret);
        res = Close(busNum, devAddr);
        if (res != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "GetDeviceInfo CloseDevice failed res=%{public}d", res);
            return res;
        }
        return ret;
    }
    res = GetConfigDescriptor(dev, descriptor);
    if (res != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetConfigDescriptor ret=%{public}d", ret);
    }
    ret = Close(busNum, devAddr);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetDeviceInfo CloseDevice failed ret=%{public}d", ret);
        return ret;
    }
    return res;
}

int32_t UsbHostManager::GetDeviceInfoDescriptor(const HDI::Usb::V1_0::UsbDev &uDev, std::vector<uint8_t> &descriptor,
    UsbDevice &dev)
{
    int32_t ret = GetRawDescriptor(uDev.busNum, uDev.devAddr, descriptor);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetRawDescriptor failed ret=%{public}d busNum:%{public}d devAddr:%{public}d",
            ret, uDev.busNum, uDev.devAddr);
        return ret;
    }
    uint8_t *buffer = descriptor.data();
    uint32_t length = descriptor.size();
    if ((!buffer) || (length == 0)) {
        USB_HILOGE(MODULE_USB_HOST, "GetRawDescriptor failed len=%{public}d busNum:%{public}d devAddr:%{public}d",
            length, uDev.busNum, uDev.devAddr);
        return UEC_SERVICE_INVALID_VALUE;
    }
    dev.SetBusNum(uDev.busNum);
    dev.SetDevAddr(uDev.devAddr);
    dev.SetName(std::to_string(uDev.busNum) + "-" + std::to_string(uDev.devAddr));

    ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer, length, dev);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "ParseDeviceDescriptor failed ret=%{public}d", ret);
        return ret;
    }
    return ret;
}

int32_t UsbHostManager::GetConfigDescriptor(UsbDevice &dev, std::vector<uint8_t> &descriptor)
{
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(descriptor, CURSOR_INIT, configs);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "ParseConfigDescriptors failed ret=%{public}d", ret);
        return ret;
    }
    dev.SetConfigs(configs);
    ret = FillDevStrings(dev);
    USB_HILOGI(MODULE_USB_HOST, "FillDevStrings ret=%{public}d", ret);
    return ret;
}

int32_t UsbHostManager::GetActiveConfig(uint8_t busNum, uint8_t devAddr, uint8_t &configIndex)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->GetConfig(dev, configIndex);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    return usbd_->GetConfig(dev, configIndex);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::GetRawDescriptor(uint8_t busNum, uint8_t devAddr, std::vector<uint8_t> &bufferData)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::GetRawDescriptor usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->GetRawDescriptor(dev, bufferData);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->GetRawDescriptor(dev, bufferData);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::GetFileDescriptor(uint8_t busNum, uint8_t devAddr, int32_t &fd)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::GetFileDescriptor usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->GetDeviceFileDescriptor(dev, fd);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->GetDeviceFileDescriptor(dev, fd);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::GetDeviceSpeed(uint8_t busNum, uint8_t devAddr, uint8_t &speed)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::GetDeviceSpeed usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    return usbHostInterface_->GetDeviceSpeed(dev, speed);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->GetDeviceSpeed(dev, speed);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::GetInterfaceActiveStatus(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid,
    bool &unactivated)
{
#ifdef USB_MANAGER_PASS_THROUGH
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::GetInterfaceActiveStatus usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostInterface_->GetInterfaceActiveStatus(dev, interfaceid, unactivated);
#else
    const UsbDev dev = {busNum, devAddr};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->GetInterfaceActiveStatus(dev, interfaceid, unactivated);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::BulkTransferRead(const HDI::Usb::V1_0::UsbDev &dev, const HDI::Usb::V1_0::UsbPipe &pipe,
    std::vector<uint8_t> &bufferData, int32_t timeOut)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::BulkTransferRead usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(dev);
    const HDI::Usb::V2_0::UsbPipe &usbPipe_ = reinterpret_cast<const HDI::Usb::V2_0::UsbPipe &>(pipe);
    return usbHostInterface_->BulkTransferRead(usbDev_, usbPipe_, timeOut, bufferData);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    return usbd_->BulkTransferRead(dev, pipe, timeOut, bufferData);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::BulkTransferReadwithLength(const HDI::Usb::V1_0::UsbDev &dev,
    const HDI::Usb::V1_0::UsbPipe &pipe, int32_t length, std::vector<uint8_t> &bufferData, int32_t timeOut)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::BulkTransferReadwithLength usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(dev);
    const HDI::Usb::V2_0::UsbPipe &usbPipe_ = reinterpret_cast<const HDI::Usb::V2_0::UsbPipe &>(pipe);
    return usbHostInterface_->BulkTransferReadwithLength(usbDev_, usbPipe_, timeOut, length, bufferData);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->BulkTransferReadwithLength(dev, pipe, timeOut, length, bufferData);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::BulkTransferWrite(const HDI::Usb::V1_0::UsbDev &dev, const HDI::Usb::V1_0::UsbPipe &pipe,
    const std::vector<uint8_t> &bufferData, int32_t timeOut)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::BulkTransferWrite usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(dev);
    const HDI::Usb::V2_0::UsbPipe &usbPipe_ = reinterpret_cast<const HDI::Usb::V2_0::UsbPipe &>(pipe);
    return usbHostInterface_->BulkTransferWrite(usbDev_, usbPipe_, timeOut, bufferData);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->BulkTransferWrite(dev, pipe, timeOut, bufferData);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::ControlTransfer(const HDI::Usb::V1_0::UsbDev &dev, const HDI::Usb::V1_0::UsbCtrlTransfer &ctrl,
    std::vector<uint8_t> &bufferData)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::ControlTransfer usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(dev);
    const HDI::Usb::V2_0::UsbCtrlTransfer &usbCtrl_ = reinterpret_cast<const HDI::Usb::V2_0::UsbCtrlTransfer &>(ctrl);
    int32_t ret = UEC_SERVICE_INNER_ERR;
    std::lock_guard<std::mutex> guard(transferMutex_);
    if (((uint32_t)ctrl.requestType & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_OUT) {
        ret = usbHostInterface_->ControlTransferWrite(usbDev_, usbCtrl_, bufferData);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ControlTransfer ControlTransferWrite error ret:%{public}d", ret);
        }
    } else {
        bufferData.clear();
        ret = usbHostInterface_->ControlTransferRead(usbDev_, usbCtrl_, bufferData);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ControlTransfer ControlTransferRead error ret:%{public}d", ret);
        }
    }
    return ret;
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = UEC_SERVICE_INNER_ERR;
    std::lock_guard<std::mutex> guard(transferMutex_);
    if (((uint32_t)ctrl.requestType & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_OUT) {
        ret = usbd_->ControlTransferWrite(dev, ctrl, bufferData);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ControlTransferWrite error ret:%{public}d", ret);
        }
    } else {
        bufferData.clear();
        ret = usbd_->ControlTransferRead(dev, ctrl, bufferData);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ControlTransferRead error ret:%{public}d", ret);
        }
    }
    return ret;
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::UsbControlTransfer(const HDI::Usb::V1_0::UsbDev &dev,
    const HDI::Usb::V1_2::UsbCtrlTransferParams &ctrlParams, std::vector<uint8_t> &bufferData)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::UsbControlTransfer usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(dev);
    const HDI::Usb::V2_0::UsbCtrlTransferParams &usbCtrlParams_ =
        reinterpret_cast<const HDI::Usb::V2_0::UsbCtrlTransferParams &>(ctrlParams);
    int32_t ret = UEC_SERVICE_INNER_ERR;
    HDI::Usb::V2_0::UsbCtrlTransfer ctrl = {
        ctrlParams.requestType, ctrlParams.requestCmd, ctrlParams.value, ctrlParams.index, ctrlParams.timeout};

    std::lock_guard<std::mutex> guard(transferMutex_);
    if (((uint32_t)ctrlParams.requestType & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_OUT) {
        ret = usbHostInterface_->ControlTransferWrite(usbDev_, ctrl, bufferData);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ControlTransferWrite UsbControlTransfer error ret:%{public}d", ret);
        }
    } else {
        bufferData.clear();
        ret = usbHostInterface_->ControlTransferReadwithLength(usbDev_, usbCtrlParams_, bufferData);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ControlTransferWritewithLength error ret:%{public}d", ret);
        }
    }
    return ret;
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = UEC_SERVICE_INNER_ERR;
    UsbCtrlTransfer ctrl = {
        ctrlParams.requestType, ctrlParams.requestCmd, ctrlParams.value, ctrlParams.index, ctrlParams.timeout};

    std::lock_guard<std::mutex> guard(transferMutex_);
    if (((uint32_t)ctrlParams.requestType & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_OUT) {
        ret = usbd_->ControlTransferWrite(dev, ctrl, bufferData);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ControlTransferWrite error ret:%{public}d", ret);
        }
    } else {
        bufferData.clear();
        ret = usbd_->ControlTransferReadwithLength(dev, ctrlParams, bufferData);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ControlTransferWritewithLength error ret:%{public}d", ret);
        }
    }
    return ret;
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::RequestQueue(const HDI::Usb::V1_0::UsbDev &dev, const HDI::Usb::V1_0::UsbPipe &pipe,
    const std::vector<uint8_t> &clientData, const std::vector<uint8_t> &bufferData)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::RequestQueue usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(dev);
    const HDI::Usb::V2_0::UsbPipe &usbPipe_ = reinterpret_cast<const HDI::Usb::V2_0::UsbPipe &>(pipe);
    return usbHostInterface_->RequestQueue(usbDev_, usbPipe_, clientData, bufferData);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->RequestQueue(dev, pipe, clientData, bufferData);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::RequestWait(const HDI::Usb::V1_0::UsbDev &dev, int32_t timeOut,
    std::vector<uint8_t> &clientData, std::vector<uint8_t> &bufferData)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::RequestWait usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(dev);
    return usbHostInterface_->RequestWait(usbDev_, clientData, bufferData, timeOut);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->RequestWait(dev, clientData, bufferData, timeOut);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::RequestCancel(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId, uint8_t endpointId)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::RequestCancel usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev dev = {busNum, devAddr};
    const HDI::Usb::V2_0::UsbPipe pipe = {interfaceId, endpointId};
    return usbHostInterface_->RequestCancel(dev, pipe);
#else
    const UsbDev dev = {busNum, devAddr};
    const UsbPipe pipe = {interfaceId, endpointId};
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    return usbd_->RequestCancel(dev, pipe);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::UsbCancelTransfer(const HDI::Usb::V1_0::UsbDev &devInfo, const int32_t &endpoint)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::UsbCancelTransfer usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(devInfo);
    return usbHostInterface_->UsbCancelTransfer(usbDev_, endpoint);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbd_->UsbCancelTransfer(devInfo, endpoint);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "UsbCancelTransfer error ret:%{public}d", ret);
        return ret;
    }
    return ret;
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::UsbSubmitTransfer(const HDI::Usb::V1_0::UsbDev &devInfo, HDI::Usb::V1_2::USBTransferInfo &info,
    const sptr<IRemoteObject> &cb, sptr<Ashmem> &ashmem)
{
    int32_t ret = UEC_SERVICE_INVALID_VALUE;
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::UsbSubmitTransfer usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    sptr<UsbHostManager::UsbSubmitTransferDeathRecipient> submitRecipient =
        new UsbSubmitTransferDeathRecipient(devInfo, info.endpoint, this, cb);
    if (!cb->AddDeathRecipient(submitRecipient)) {
        USB_HILOGE(MODULE_USB_HOST, "add DeathRecipient failed");
        return UEC_SERVICE_INVALID_VALUE;
    }
    sptr<UsbTransferCallbackImpl> callbackImpl = new UsbTransferCallbackImpl(cb);
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(devInfo);
    const HDI::Usb::V2_0::USBTransferInfo &usbInfo = reinterpret_cast<const HDI::Usb::V2_0::USBTransferInfo &>(info);
    ret = usbHostInterface_->UsbSubmitTransfer(usbDev_, usbInfo, callbackImpl, ashmem);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::UsbSubmitTransfer usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    sptr<UsbHostManager::UsbSubmitTransferDeathRecipient> submitRecipient =
        new UsbSubmitTransferDeathRecipient(devInfo, info.endpoint, this, cb);
    if (!cb->AddDeathRecipient(submitRecipient)) {
        USB_HILOGE(MODULE_USB_HOST, "add DeathRecipient failed");
        return UEC_SERVICE_INVALID_VALUE;
    }
    sptr<UsbdTransferCallbackImpl> callbackImpl = new UsbdTransferCallbackImpl(cb);
    ret = usbd_->UsbSubmitTransfer(devInfo, info, callbackImpl, ashmem);
#endif // USB_MANAGER_PASS_THROUGH
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager UsbSubmitTransfer error ret:%{public}d", ret);
        cb->RemoveDeathRecipient(submitRecipient);
        submitRecipient.clear();
        return ret;
    }
    return ret;
}

int32_t UsbHostManager::RegBulkCallback(const HDI::Usb::V1_0::UsbDev &devInfo, const HDI::Usb::V1_0::UsbPipe &pipe,
    const sptr<IRemoteObject> &cb)
{
    if (cb == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "cb is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    std::lock_guard<std::mutex> guard(hdiCbMutex_);
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostHdiCb_ == nullptr) {
        usbHostHdiCb_ = new UsbBulkCallbackImpl(cb);
    }
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::RegBulkCallback usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(devInfo);
    const HDI::Usb::V2_0::UsbPipe &usbPipe_ = reinterpret_cast<const HDI::Usb::V2_0::UsbPipe &>(pipe);
    return usbHostInterface_->RegBulkCallback(usbDev_, usbPipe_, usbHostHdiCb_);
#else
    if (hdiCb_ == nullptr) {
        hdiCb_ = new UsbdBulkCallbackImpl(cb);
    }
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    return usbd_->RegBulkCallback(devInfo, pipe, hdiCb_);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::UnRegBulkCallback(const HDI::Usb::V1_0::UsbDev &devInfo, const HDI::Usb::V1_0::UsbPipe &pipe)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::UnRegBulkCallback usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    std::lock_guard<std::mutex> guard(hdiCbMutex_);
    usbHostHdiCb_ = nullptr;
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(devInfo);
    const HDI::Usb::V2_0::UsbPipe &usbPipe_ = reinterpret_cast<const HDI::Usb::V2_0::UsbPipe &>(pipe);
    return usbHostInterface_->UnRegBulkCallback(usbDev_, usbPipe_);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    std::lock_guard<std::mutex> guard(hdiCbMutex_);
    hdiCb_ = nullptr;
    return usbd_->UnRegBulkCallback(devInfo, pipe);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::BulkRead(const HDI::Usb::V1_0::UsbDev &devInfo, const HDI::Usb::V1_0::UsbPipe &pipe,
    sptr<Ashmem> &ashmem)
{
    if (ashmem == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "BulkRead error ashmem");
        return UEC_SERVICE_INVALID_VALUE;
    }
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::BulkRead usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(devInfo);
    const HDI::Usb::V2_0::UsbPipe &usbPipe_ = reinterpret_cast<const HDI::Usb::V2_0::UsbPipe &>(pipe);
    return usbHostInterface_->BulkRead(usbDev_, usbPipe_, ashmem);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    return usbd_->BulkRead(devInfo, pipe, ashmem);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::BulkWrite(const HDI::Usb::V1_0::UsbDev &devInfo, const HDI::Usb::V1_0::UsbPipe &pipe,
    sptr<Ashmem> &ashmem)
{
    if (ashmem == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "BulkWrite error ashmem");
        return UEC_SERVICE_INVALID_VALUE;
    }
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::BulkWrite usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(devInfo);
    const HDI::Usb::V2_0::UsbPipe &usbPipe_ = reinterpret_cast<const HDI::Usb::V2_0::UsbPipe &>(pipe);
    return usbHostInterface_->BulkWrite(usbDev_, usbPipe_, ashmem);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    return usbd_->BulkWrite(devInfo, pipe, ashmem);
#endif // USB_MANAGER_PASS_THROUGH
}

int32_t UsbHostManager::BulkCancel(const HDI::Usb::V1_0::UsbDev &devInfo, const HDI::Usb::V1_0::UsbPipe &pipe)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::BulkCancel usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(devInfo);
    const HDI::Usb::V2_0::UsbPipe &usbPipe_ = reinterpret_cast<const HDI::Usb::V2_0::UsbPipe &>(pipe);
    return usbHostInterface_->BulkCancel(usbDev_, usbPipe_);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    return usbd_->BulkCancel(devInfo, pipe);
#endif // USB_MANAGER_PASS_THROUGH
}

bool UsbHostManager::GetTargetDevice(uint8_t busNum, uint8_t devAddr, UsbDevice &dev)
{
    std::shared_lock lock(devicesMutex_);
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if ((it->second->GetBusNum() == busNum) && (it->second->GetDevAddr() == devAddr)) {
            dev = *it->second;
            return true;
        }
    }
    USB_HILOGE(MODULE_USB_HOST, "UsbHostManager: target device not found");
    return false;
}

bool UsbHostManager::GetEndpointFromId(UsbDevice dev, int32_t endpointId, USBEndpoint &endpoint)
{
    // get USBEndpoint based on endpoint address(id); return false if not found
    for (auto &config : dev.GetConfigs()) {
        for (auto &interface : config.GetInterfaces()) {
            auto &eps = interface.GetEndpoints();
            auto it = std::find_if(eps.begin(), eps.end(),
                [endpointId](auto &ep) { return ep.GetAddress() == endpointId; });
            if (it != eps.end()) {
                endpoint = *it;
                return true;
            }
        }
    }
    USB_HILOGE(MODULE_USB_HOST, "invalid endpoint id %{public}d", endpointId);
    return false;
}

bool UsbHostManager::GetProductName(const std::string &deviceName, std::string &productName)
{
    std::shared_lock lock(devicesMutex_);
    auto iter = devices_.find(deviceName);
    if (iter == devices_.end()) {
        return false;
    }

    UsbDevice *dev = iter->second;
    if (dev == nullptr) {
        return false;
    }

    productName = dev->GetProductName();
    return true;
}

bool UsbHostManager::DelDevice(uint8_t busNum, uint8_t devNum)
{
    std::string name = std::to_string(busNum) + "-" + std::to_string(devNum);
    std::unique_lock lock(devicesMutex_);
    MAP_STR_DEVICE::iterator iter = devices_.find(name);
    if (iter == devices_.end()) {
        USB_HILOGF(MODULE_USB_HOST, "name:%{public}s bus:%{public}hhu dev:%{public}hhu not exist", name.c_str(), busNum,
            devNum);
        return false;
    }
    UsbDevice *devOld = iter->second;
    if (devOld == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "invalid device");
        return false;
    }

    if (devOld->GetAuthorizeStatus() == ENABLED) {
        // if enabled, then broadcast common event; o.w. dev is already unseen
        auto isSuccess = PublishCommonEvent(CommonEventSupport::COMMON_EVENT_USB_DEVICE_DETACHED, *devOld);
        if (!isSuccess) {
            USB_HILOGW(MODULE_USB_HOST, "send device attached broadcast failed");
        }
    }

    for (auto it = serialDevices_.begin(); it != serialDevices_.end(); ++it) {
        if ((it->busNum == devOld->GetBusNum()) && (it->devAddr == devOld->GetDevAddr())) {
            serialDevices_.erase(it);
            break;
        }
    }
    delete devOld;
    devices_.erase(iter);
    USB_HILOGI(MODULE_USB_HOST,
        "device:%{public}s bus:%{public}hhu dev:%{public}hhu erase, cur device size: %{public}zu",
        name.c_str(), busNum, devNum, devices_.size());
    return true;
}

bool UsbHostManager::AddDevice(UsbDevice *dev)
{
    if (dev == nullptr) {
        USB_HILOGF(MODULE_USB_HOST, "device is NULL");
        return false;
    }

    uint8_t busNum = dev->GetBusNum();
    uint8_t devNum = dev->GetDevAddr();
    std::string name = std::to_string(busNum) + "-" + std::to_string(devNum);
    std::unique_lock lock(devicesMutex_);
    MAP_STR_DEVICE::iterator iter = devices_.find(name);
    if (iter != devices_.end()) {
        USB_HILOGF(MODULE_USB_HOST, "device:%{public}s bus:%{public}hhu dev:%{public}hhu already exist", name.c_str(),
            busNum, devNum);
        UsbDevice *devOld = iter->second;
        if (devOld != nullptr && devOld != dev) {
            delete devOld;
        }
        devices_.erase(iter);
    }
    devices_.insert(std::pair<std::string, UsbDevice *>(name, dev));
    dev->SetAuthorizeStatus(NEW_ARRIVED);   // will be updated in ExecuteStrategy
    USB_HILOGI(MODULE_USB_HOST,
        "device:%{public}s bus:%{public}hhu dev:%{public}hhu insert, cur device size: %{public}zu",
        name.c_str(), busNum, devNum, devices_.size());
    AddUsbSerialDevice(*dev);
    lock.unlock();

    // DONT hold unique_lock here: ExecuteStratgy quiries policy (requires the same lock with policy execution in MDM)
    ExecuteStrategy();

    std::shared_lock sharedLock(devicesMutex_);
    iter = devices_.find(name);
    if (iter == devices_.end()) {
        USB_HILOGW(MODULE_USB_HOST, "%{public}s: device removed before publish common event", __func__);
        return false;
    }
    dev = iter->second;
    if (dev->GetAuthorizeStatus() == DISABLED) {
        USB_HILOGI(MODULE_USB_HOST, "device is disallowed by EDM, skip common event broadcast");
    } else {
        dev->SetAuthorizeStatus(ENABLED);
        auto isSuccess = PublishCommonEvent(CommonEventSupport::COMMON_EVENT_USB_DEVICE_ATTACHED, *dev);
        if (!isSuccess) {
            USB_HILOGW(MODULE_USB_HOST, "send device attached broadcast failed");
        }
    }
    return true;
}

static bool IsAudioDevice(UsbDevice &dev)
{
    for (auto &config : dev.GetConfigs()) {
        for (auto &intf : config.GetInterfaces()) {
            if (intf.GetClass() == BASE_CLASS_AUDIO) {
                return true;
            }
        }
    }
    return false;
}

bool UsbHostManager::PublishCommonEvent(const std::string &event, UsbDevice &dev)
{
    Want want;
    want.SetAction(event);
    CommonEventData data(want);
    data.SetData(dev.getJsonString().c_str());
    CommonEventPublishInfo publishInfo;
    if (dev.GetClass() == BASE_CLASS_HUB) {
        publishInfo.SetSubscriberType(SubscriberType::SYSTEM_SUBSCRIBER_TYPE);
    } else if (!IsAudioDevice(dev)) {
        if (event == CommonEventSupport::COMMON_EVENT_USB_DEVICE_DETACHED) {
            UsbConnectionNotifier::GetInstance()->CancelNotification(true);
        } else {
            UsbConnectionNotifier::GetInstance()->SendNotification(USB_FUNC_REVERSE_CHARGE);
        }
    }
    USB_HILOGI(MODULE_USB_HOST, "send %{public}s broadcast device:%{public}s", event.c_str(),
        dev.getJsonString().c_str());
    ReportHostPlugSysEvent(event, dev);
    return CommonEventManager::PublishCommonEvent(data, publishInfo);
}

bool UsbHostManager::Dump(int fd, const std::string &args)
{
    if (args.compare("-a") != 0) {
        dprintf(fd, "args is not -a\n");
        return false;
    }

    dprintf(fd, "Usb Host all device list info:\n");
    std::shared_lock lock(devicesMutex_);
    for (const auto &item : devices_) {
        dprintf(fd, "usb host list info: %s\n", item.second->getJsonString().c_str());
    }
    return true;
}

int32_t UsbHostManager::GetDeviceDescription(int32_t baseClass, std::string &description, uint8_t &usage)
{
    auto iter = deviceUsageMap.find(baseClass);
    if (iter != deviceUsageMap.end()) {
        description = iter->second.description;
        usage = iter->second.usage;
    } else {
        description = "NA";
        usage = 1;
    }
    return UEC_OK;
}


std::string UsbHostManager::ConcatenateToDescription(const UsbDeviceType &interfaceType, const std::string& str)
{
    std::stringstream ss;
    ss << std::setw(CLASS_PRINT_LENGTH) << std::setfill('0') << std::hex << interfaceType.baseClass << "_";
    ss << std::setw(CLASS_PRINT_LENGTH) << std::setfill('0') << std::hex << interfaceType.subClass << "_";
    ss << std::setw(CLASS_PRINT_LENGTH) << std::setfill('0') << std::hex << interfaceType.protocol << ",";
    ss << str;
    return ss.str();
}

std::string UsbHostManager::GetInterfaceUsageDescription(const UsbDeviceType &interfaceType)
{
    std::string infUsageDes = "NA";
    auto infUsageIter = interfaceUsageMap.find(interfaceType);
    if (infUsageIter != interfaceUsageMap.end()) {
        return infUsageIter->second;
    }
    return infUsageDes;
}

int32_t UsbHostManager::GetInterfaceDescription(const UsbDevice &dev, std::string &description, int32_t &baseClass)
{
    std::set<UsbDeviceType> useInterfaceType;
    for (int32_t i = 0; i < dev.GetConfigCount(); i++) {
        USBConfig config;
        dev.GetConfig(i, config);
        for (uint32_t j = 0; j < config.GetInterfaceCount(); j++) {
            if (i != 0 || j != 0) {
                description += ";";
            }
            UsbInterface interface;
            config.GetInterface(j, interface);
            baseClass = interface.GetClass();
            UsbDeviceType interfaceType = {interface.GetClass(),
                interface.GetSubClass(), interface.GetProtocol(), 0};
                useInterfaceType.insert(interfaceType);
                std::string infUsageDes = GetInterfaceUsageDescription(interfaceType);
                description += ConcatenateToDescription(interfaceType, infUsageDes);
        }
    }
    return UEC_OK;
}

void UsbHostManager::ReportHostPlugSysEvent(const std::string &event, const UsbDevice &dev)
{
    std::string deviceUsageDes;
    uint8_t deviceUsage = 0;
    GetDeviceDescription(dev.GetClass(), deviceUsageDes, deviceUsage);
    std::string extUsageDes;
    int32_t intfBaseClass = 0;
    if (deviceUsage & DES_USAGE_IN_INTERFACE) {
        GetInterfaceDescription(dev, extUsageDes, intfBaseClass);
    }

    if (dev.GetClass() == USAGE_IN_INTERFACE_CLASS) {
        GetDeviceDescription(intfBaseClass, deviceUsageDes, deviceUsage);
    }
    std::string snNum;
    if (dev.GetmSerial().length() > LAST_FIVE) {
        snNum = dev.GetmSerial().substr(dev.GetmSerial().length() - LAST_FIVE);
    }
    USB_HILOGI(MODULE_USB_HOST, "Host mode Indicates the insertion and removal information");
    HiSysEventWrite(HiSysEvent::Domain::USB, "PLUG_IN_OUT_HOST_MODE", HiSysEvent::EventType::BEHAVIOR,
        "DEVICE_NAME", dev.GetProductName(), "DEVICE_PROTOCOL", dev.GetProtocol(),
        "DEVICE_SUBCLASS", dev.GetSubclass(), "DEVICE_CLASS", dev.GetClass(),
        "DEVICE_CLASS_DESCRIPTION", deviceUsageDes, "INTERFACE_CLASS_DESCRIPTION", extUsageDes,
        "VENDOR_ID", dev.GetVendorId(), "PRODUCT_ID", dev.GetProductId(),
        "VERSION", dev.GetVersion(), "EVENT_NAME", event, "SN_NUM", snNum,
        "MANUFACTURER_NAME", dev.GetManufacturerName());
}

static std::string BcdToString(uint16_t bcdUsb)
{
    std::ostringstream oss;
    oss << std::hex << std::uppercase <<std::setw(BCD_HEX_DIGITS) << std::setfill('0') << bcdUsb;
    return oss.str();
}

int32_t UsbHostManager::FillDevStrings(UsbDevice &dev)
{
    uint8_t busNum;
    uint8_t devAddr;

    busNum = dev.GetBusNum();
    devAddr = dev.GetDevAddr();
    uint16_t bcdUsb = dev.GetbcdUSB();
    dev.SetVersion(BcdToString(bcdUsb));
    dev.SetManufacturerName(GetDevStringValFromIdx(busNum, devAddr, dev.GetiManufacturer()));
    dev.SetProductName(GetDevStringValFromIdx(busNum, devAddr, dev.GetiProduct()));
    dev.SetmSerial(GetDevStringValFromIdx(busNum, devAddr, dev.GetiSerialNumber()));
    USB_HILOGI(MODULE_USB_HOST,
        "iSerial:%{public}d Manufactur:%{public}s product:%{public}s "
        "version:%{public}s",
        dev.GetiSerialNumber(), dev.GetManufacturerName().c_str(), dev.GetProductName().c_str(),
        dev.GetVersion().c_str());

    std::vector<USBConfig> configs;
    configs = dev.GetConfigs();
    for (auto it = configs.begin(); it != configs.end(); ++it) {
        it->SetName(GetDevStringValFromIdx(busNum, devAddr, it->GetiConfiguration()));
        USB_HILOGI(MODULE_USB_HOST, "Config:%{public}d %{public}s", it->GetiConfiguration(), it->GetName().c_str());
        std::vector<UsbInterface> interfaces = it->GetInterfaces();
        for (auto itIF = interfaces.begin(); itIF != interfaces.end(); ++itIF) {
            itIF->SetName(GetDevStringValFromIdx(busNum, devAddr, itIF->GetiInterface()));
            USB_HILOGI(MODULE_USB_HOST, "interface:%{public}hhu %{public}s", itIF->GetiInterface(),
                itIF->GetName().c_str());
        }
        it->SetInterfaces(interfaces);
    }
    dev.SetConfigs(configs);

    return UEC_OK;
}

static std::string Utf16leToUtf8(char *utf16leBytes, size_t length)
{
    if (utf16leBytes == nullptr || length % HALF) {
        USB_HILOGE(MODULE_USB_HOST, "Utf16leToUtf8: invalid length: %{public}zu", length);
        return " ";
    }
    size_t charCount = length / HALF;
    // check and skip BOM
    if (charCount > 0 && utf16leBytes[0] == 0xFF && utf16leBytes[1] == 0xFE) {
        charCount--;
        utf16leBytes += BOM_BYTE_COUNT;
        length -= BOM_BYTE_COUNT;
    }
    if (charCount == 0) {
        USB_HILOGE(MODULE_USB_HOST, "empty string");
        return " ";
    }
    iconv_t cd = iconv_open("UTF-8", "UTF-16LE");
    if (cd == reinterpret_cast<iconv_t>(INVALID_RET)) {
        USB_HILOGE(MODULE_USB_HOST, "iconv_open failed");
        return " ";
    }
    std::vector<char> outbuf(charCount * BYTES_PER_UTF8_CHAR + 1, 0);
    char *outptr = outbuf.data();
    size_t outBytesLeft = outbuf.size();
    size_t inBytesLeft = length;
    size_t result = iconv(cd, &utf16leBytes, &inBytesLeft, &outptr, &outBytesLeft);
    iconv_close(cd);
    if (result == static_cast<size_t>(INVALID_RET)) {
        USB_HILOGE(MODULE_USB_HOST, "iconv failed: %{public}zu", result);
        return " ";
    }
    std::string str = std::string(outbuf.data(), outptr - outbuf.data());
    auto iter = str.find('\0');
    if (iter != std::string::npos) {
        str = str.substr(0, iter);
    }
    return str;
}

std::string UsbHostManager::GetDevStringValFromIdx(uint8_t busNum, uint8_t devAddr, uint8_t idx)
{
    const UsbDev dev = {busNum, devAddr};
    std::vector<uint8_t> strV;
    std::string strDesc = " ";

    if (idx == 0) {
        return strDesc;
    }

#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbHostInterface_ is nullptr");
        return strDesc;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(dev);
    int32_t ret = usbHostInterface_->GetStringDescriptor(usbDev_, idx, strV);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::usbd_ is nullptr");
        return strDesc;
    }
    int32_t ret = usbd_->GetStringDescriptor(dev, idx, strV);
#endif // USB_MANAGER_PASS_THROUGH
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "get string[%{public}hhu] failed ret:%{public}d", idx, ret);
        return strDesc;
    }
    size_t length = strV.size();
    if ((length < DESCRIPTOR_VALUE_START_OFFSET) || (strV[1] != DESCRIPTOR_TYPE_STRING)) {
        USB_HILOGI(MODULE_USB_HOST, "type or length error, len:%{public}zu", length);
        return strDesc;
    }

    char *tbuf = new (std::nothrow) char[length - DESCRIPTOR_VALUE_START_OFFSET]();
    if (tbuf == nullptr) {
        USB_HILOGI(MODULE_USB_HOST, "new failed");
        return strDesc;
    }

    for (uint32_t i = 0; i < length - DESCRIPTOR_VALUE_START_OFFSET; ++i) {
        tbuf[i] = strV[i + DESCRIPTOR_VALUE_START_OFFSET];
    }

    strDesc = Utf16leToUtf8(tbuf, length - DESCRIPTOR_VALUE_START_OFFSET);
    USB_HILOGI(MODULE_USB_HOST, "getString idx: %{public}d length:%{public}zu, str: %{public}s",
        idx, strDesc.length(), strDesc.c_str());
    delete[] tbuf;
    return strDesc;
}

bool UsbHostManager::IsEdmEnabled()
{
    std::string edmParaValue = OHOS::system::GetParameter("persist.edm.enterprise_config_enable", "false");
    USB_HILOGI(MODULE_USB_HOST, "edmParaValue enterprise_config_enable value is %{public}s", edmParaValue.c_str());
    return edmParaValue == "true";
}

int32_t UsbHostManager::UsbDeviceAuthorize(
    uint8_t busNum, uint8_t devAddr, bool authorized, const std::string &operationType)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDeviceAuthorize: set authorized=%{public}d, operationType=%{public}s",
        int(authorized), operationType.c_str());
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "usbDeviceInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    std::string name = std::to_string(busNum) + "-" + std::to_string(devAddr);
    auto iterDev = devices_.find(name);
    if (iterDev == devices_.end()) {
        USB_HILOGE(MODULE_USB_HOST, "UsbDeviceAuthorize: dev %{public}s not found", name.c_str());
        return UEC_SERVICE_INVALID_VALUE;
    }
    auto authorizeStatus = iterDev->second->GetAuthorizeStatus();
    if ((authorized && authorizeStatus != DISABLED) || (!authorized && authorizeStatus == DISABLED)) {
        USB_HILOGI(MODULE_USB_HOST, "no need to change dev %{public}s authorize state", name.c_str());
        return UEC_OK;
    }

    USB_HILOGI(MODULE_USB_HOST, "set dev %{public}s authorized state=%{public}d",
        name.c_str(), int(authorized));
    int32_t ret = usbDeviceInterface_->UsbDeviceAuthorize(busNum, devAddr, authorized);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "UsbDeviceAuthorize: failed to (un)authorize dev %{public}s", name.c_str());
        return ret;
    }

    if (!authorized) {
        ReportManageDeviceInfo(operationType, iterDev->second, nullptr, false);
    }
    if (authorizeStatus != NEW_ARRIVED) { // skip for newly arrived device here (send in AddDevice if not disabled)
        auto eventType = authorized? CommonEventSupport::COMMON_EVENT_USB_DEVICE_ATTACHED :
            CommonEventSupport::COMMON_EVENT_USB_DEVICE_DETACHED;
        auto isSuccess = PublishCommonEvent(eventType, *iterDev->second);
        if (!isSuccess) {
            USB_HILOGW(MODULE_USB_HOST, "send device attached/detached broadcast failed");
        }
    }
    iterDev->second->SetAuthorizeStatus(authorized? ENABLED : DISABLED); // authorized==true -> ENABLED
    std::this_thread::sleep_for(std::chrono::milliseconds(MANAGE_INTERFACE_INTERVAL));
    return UEC_OK;
}

int32_t UsbHostManager::UsbInterfaceAuthorize(
    const HDI::Usb::V1_0::UsbDev &dev, uint8_t configId, uint8_t interfaceId, bool authorized)
{
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "usbDeviceInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev usbDev_ = {dev.busNum, dev.devAddr};
    auto ret = usbDeviceInterface_->UsbInterfaceAuthorize(usbDev_, configId, interfaceId, authorized);
    USB_HILOGI(MODULE_USB_HOST, "usbInterfaceAuthorize: authorized=%{public}d; ret=%{public}d",
        int(authorized), ret);
    if (ret == UEC_OK && authorized) {
        ret = ManageInterface(dev, interfaceId, !authorized);
        USB_HILOGI(MODULE_USB_HOST, "usbInterfaceAuthorize: ManageInterface ret=%{public}d", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(MANAGE_INTERFACE_INTERVAL));
    return ret;
}

int32_t UsbHostManager::ExecuteManageDevicePolicy(std::vector<UsbDeviceId> &trustList)
{
    int32_t ret = UEC_OK;
    USB_HILOGI(MODULE_USB_HOST, "list size %{public}zu", devices_.size());
    std::shared_lock lock(devicesMutex_);
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        bool inTrustList = false;
        for (auto dev : trustList) {
            if (it->second->GetProductId() == dev.productId && it->second->GetVendorId() == dev.vendorId) {
                inTrustList = true;
                break;
            }
        }
        if (inTrustList || trustList.empty()) {
            ret = ManageDeviceImpl(it->second->GetVendorId(), it->second->GetProductId(), false);
        } else {
            ret = ManageDeviceImpl(it->second->GetVendorId(), it->second->GetProductId(), true);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(MANAGE_INTERFACE_INTERVAL));
    }
    if (ret != UEC_OK) {
        USB_HILOGI(MODULE_USB_HOST, "ManageDevice failed");
        return UEC_SERVICE_EXECUTE_POLICY_FAILED;
    }
    return UEC_OK;
}

int32_t UsbHostManager::ExecuteManageInterfaceType(const std::vector<UsbDeviceType> &disableType, bool disable)
{
    std::shared_lock lock(devicesMutex_);
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        UsbDev dev = {it->second->GetBusNum(), it->second->GetDevAddr()};
        int32_t ret = OpenDevice(dev.busNum, dev.devAddr);
        if (ret != UEC_OK) {
            USB_HILOGW(MODULE_USB_HOST, "ExecuteManageInterfaceType open fail ret = %{public}d", ret);
        }
    }
    ExecuteManageDeviceType(disableType, disable, d_typeMap, true);
    ExecuteManageDeviceType(disableType, disable, g_typeMap, false);
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        UsbDev dev = {it->second->GetBusNum(), it->second->GetDevAddr()};
        int32_t ret = Close(dev.busNum, dev.devAddr);
        if (ret != UEC_OK) {
            USB_HILOGW(MODULE_USB_HOST, "ExecuteManageInterfaceType close fail ret = %{public}d", ret);
        }
    }
    return UEC_OK;
}

int32_t UsbHostManager::GetEdmPolicy(bool &IsGlobalDisabled, std::vector<UsbDeviceType> &disableType,
    std::vector<UsbDeviceId> &trustUsbDeviceIds)
{
    sptr<ISystemAbilityManager> sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "fail to get SystemAbilityManager");
        return UEC_SERVICE_GET_SYSTEM_ABILITY_MANAGER_FAILED;
    }
    sptr<IRemoteObject> remote = sm->CheckSystemAbility(EDM_SYSTEM_ABILITY_ID);
    if (remote == nullptr) {
        LoadEdmService();
        USB_HILOGE(MODULE_USB_HOST, "Get Edm SystemAbility failed.");
        return UEC_SERVICE_GET_EDM_SERVICE_FAILED;
    }
    int32_t ret = GetEdmGlobalPolicy(remote, IsGlobalDisabled);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetEdmGlobalPolicy failed.");
        return ret;
    }
    ret = GetEdmStroageTypePolicy(remote, disableType);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetEdmStroageTypePolicy failed.");
        return ret;
    }
    ret = GetEdmTypePolicy(remote, disableType);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetEdmTypePolicy failed.");
        return ret;
    }
    ret = GetEdmPermTypePolicy(remote, disableType);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetEdmTypePolicy failed.");
        return ret;
    }
    ret = GetEdmTrustListPolicy(remote, trustUsbDeviceIds);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetEdmTrustListPolicy failed.");
        return ret;
    }
    return UEC_OK;
}

void UsbHostManager::LoadEdmService()
{
    USB_HILOGI(MODULE_USB_HOST,
        "%{public}s enter, systemAbilityId = [%{public}d] loading", __func__, EDM_SYSTEM_ABILITY_ID);
    sptr<ISystemAbilityManager> sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "%{public}s: get system ability manager failed", __func__);
        return;
    }
    auto usbEdmLoadCallback = sptr<UsbEdmLoadCallback>(new UsbEdmLoadCallback(this));
    int32_t ret = sm -> LoadSystemAbility(EDM_SYSTEM_ABILITY_ID, usbEdmLoadCallback);
    if (ret != ERR_OK) {
        USB_HILOGE(MODULE_USB_HOST,
            "%{public}s: failed to load system ability , SA Id = [%{public}d], ret = [%{public}d]",
            __func__, EDM_SYSTEM_ABILITY_ID, ret);
    }
}

int32_t UsbHostManager::GetUsbPolicy(bool &IsGlobalDisabled, std::vector<UsbDeviceType> &disableType,
    std::vector<UsbDeviceId> &trustUsbDeviceIds)
{
    auto startTime = std::chrono::steady_clock::now();
    bool isPolicyObtained = false;
    while (!isPolicyObtained) {
        int32_t ret = GetEdmPolicy(IsGlobalDisabled, disableType, trustUsbDeviceIds);
        if (ret == UEC_OK) {
            USB_HILOGI(MODULE_USB_HOST, "GetUsbPolicy succeed");
            break;
        } else if (ret == EDM_SA_TIME_OUT_CODE) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
            if (elapsedTime >= EDM_SA_MAX_TIME_OUT) {
                USB_HILOGE(MODULE_USB_HOST, "Time out, exit loop");
                return UEC_SERVICE_EDM_SA_TIME_OUT_FAILED;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(MANAGE_INTERFACE_INTERVAL));
        } else {
            USB_HILOGE(MODULE_USB_HOST, "EDM sa failed");
            return UEC_SERVICE_PREPARE_EDM_SA_FAILED;
        }
    }
    return UEC_OK;
}

int32_t UsbHostManager::GetEdmTypePolicy(sptr<IRemoteObject> remote, std::vector<UsbDeviceType> &disableType)
{
    if (remote == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "Remote is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInterfaceToken(DESCRIPTOR);
    data.WriteInt32(WITHOUT_USERID);
    data.WriteString("");
    data.WriteInt32(WITHOUT_ADMIN);

    // query permissive type policy
    uint32_t funcCode = (1 << EMD_MASK_CODE) | USB_DEVICE_ACCESS_POLICY;
    int32_t sendRet = remote->SendRequest(funcCode, data, reply, option);
    int32_t ret = ERR_INVALID_VALUE;
    bool isSuccess = reply.ReadInt32(ret) && (ret == ERR_OK);
    if (!isSuccess || (sendRet != UEC_OK)) {
        USB_HILOGE(MODULE_USB_HOST, "%{public}s failed. sendRet =  %{public}d, ret = %{public}d",
            __func__, sendRet, ret);
        return UEC_SERVICE_EDM_SEND_REQUEST_FAILED;
    }
    int32_t size = reply.ReadInt32();
    if (size < 0 || static_cast<uint32_t>(size) > TRUSTLIST_POLICY_MAX_DEVICES) {
        USB_HILOGE(MODULE_USB_HOST, "EdmTypeList size=[%{public}d] is invalid", size);
        return UEC_SERVICE_EDM_DEVICE_SIZE_EXCEED;
    }

    isPermTypePolicy_ = false;
    USB_HILOGI(MODULE_USB_HOST, "%{public}s return size:%{public}d", __func__, size);
    ReadTypePolicyFromParcel(reply, size, disableType);
    return UEC_OK;
}

int32_t UsbHostManager::GetEdmPermTypePolicy(sptr<IRemoteObject> remote, std::vector<UsbDeviceType> &disableType)
{
    if (remote == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "Remote is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInterfaceToken(DESCRIPTOR);
    data.WriteInt32(WITHOUT_USERID);
    data.WriteString("");
    data.WriteInt32(WITHOUT_ADMIN);

    // query permissive type policy
    uint32_t funcCode = (1 << EMD_MASK_CODE) | USB_TYPE_DISALLOW_POLICY;
    int32_t sendRet = remote->SendRequest(funcCode, data, reply, option);
    int32_t ret = ERR_INVALID_VALUE;
    bool isSuccess = reply.ReadInt32(ret) && (ret == ERR_OK);
    if (!isSuccess || (sendRet != UEC_OK)) {
        USB_HILOGE(MODULE_USB_HOST, "%{public}s failed. sendRet =  %{public}d, ret = %{public}d",
            __func__, sendRet, ret);
        return UEC_SERVICE_EDM_SEND_REQUEST_FAILED;
    }
    int32_t size = reply.ReadInt32();
    if (size <= 0 || static_cast<uint32_t>(size) > TRUSTLIST_POLICY_MAX_DEVICES) {
        USB_HILOGE(MODULE_USB_HOST, "EdmTypeList size=[%{public}d] is invalid", size);
        return UEC_SERVICE_EDM_DEVICE_SIZE_EXCEED;
    }

    isPermTypePolicy_ = true;
    USB_HILOGI(MODULE_USB_HOST, "%{public}s return size:%{public}d", __func__, size);
    ReadTypePolicyFromParcel(reply, size, disableType);
    return UEC_OK;
}

void UsbHostManager::ReadTypePolicyFromParcel(MessageParcel &reply, int size, std::vector<UsbDeviceType> &disableType)
{
    for (int32_t i = 0; i < size; i++) {
        UsbDeviceType usbDeviceType;
        usbDeviceType.baseClass = reply.ReadInt32();
        usbDeviceType.subClass = reply.ReadInt32();
        usbDeviceType.protocol = reply.ReadInt32();
        usbDeviceType.isDeviceType = reply.ReadBool();
        usbDeviceType.isDeviceTypeAllMatch = reply.ReadBool();
        disableType.emplace_back(usbDeviceType);
    }
}

int32_t UsbHostManager::GetEdmGlobalPolicy(sptr<IRemoteObject> remote, bool &IsGlobalDisabled)
{
    if (remote == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "Remote is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInterfaceToken(DESCRIPTOR);
    data.WriteInt32(WITHOUT_USERID);
    data.WriteString("");
    data.WriteInt32(WITHOUT_ADMIN);
    uint32_t funcCode = (1 << EMD_MASK_CODE) | DISABLE_USB;
    int32_t sendRet = remote->SendRequest(funcCode, data, reply, option);
    int32_t ret = ERR_INVALID_VALUE;
    bool isSuccess = reply.ReadInt32(ret) && (ret == UEC_OK);
    if (!isSuccess || (sendRet != UEC_OK)) {
        USB_HILOGE(MODULE_USB_HOST, "GetGlobalPolicy failed. sendRet =  %{public}d, ret = %{public}d",
            sendRet, ret);
        return UEC_SERVICE_EDM_SEND_REQUEST_FAILED;
    }

    reply.ReadBool(IsGlobalDisabled);
    return UEC_OK;
}

int32_t UsbHostManager::GetEdmStroageTypePolicy(sptr<IRemoteObject> remote, std::vector<UsbDeviceType> &disableType)
{
    if (remote == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "Remote is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t stroageDisableType = 0;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInterfaceToken(DESCRIPTOR);
    data.WriteInt32(WITHOUT_USERID);
    data.WriteString("");
    data.WriteInt32(WITHOUT_ADMIN);
    uint32_t funcCode = (1 << EMD_MASK_CODE) | USB_STORAGE_DEVICE_ACCESS_POLICY;
    int32_t sendRet = remote->SendRequest(funcCode, data, reply, option);
    int32_t ret = ERR_INVALID_VALUE;
    bool isSuccess = reply.ReadInt32(ret) && (ret == ERR_OK);
    if (!isSuccess || (sendRet != UEC_OK)) {
        USB_HILOGE(MODULE_USB_HOST, "GetEdmStroageTypePolicy failed. sendRet =  %{public}d, ret = %{public}d",
            sendRet, ret);
        return UEC_SERVICE_EDM_SEND_REQUEST_FAILED;
    }

    reply.ReadInt32(stroageDisableType);
    if (stroageDisableType == GET_EDM_STORAGE_DISABLE_TYPE) {
        UsbDeviceType usbDeviceType;
        usbDeviceType.baseClass = STORAGE_BASE_CLASS;
        usbDeviceType.isDeviceType = 0;
        disableType.emplace_back(usbDeviceType);
    }
    return UEC_OK;
}

int32_t UsbHostManager::GetEdmTrustListPolicy(sptr<IRemoteObject> remote, std::vector<UsbDeviceId> &trustUsbDeviceIds)
{
    if (remote == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "Remote is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInterfaceToken(DESCRIPTOR);
    data.WriteInt32(WITHOUT_USERID);
    data.WriteString("");
    data.WriteInt32(WITHOUT_ADMIN);
    uint32_t funcCode = (1 << EMD_MASK_CODE) | ALLOWED_USB_DEVICES;
    int32_t sendRet = remote->SendRequest(funcCode, data, reply, option);
    int32_t ret = ERR_INVALID_VALUE;
    bool IsSuccess = reply.ReadInt32(ret) && (ret == ERR_OK);
    if (!IsSuccess || (sendRet != UEC_OK)) {
        USB_HILOGE(MODULE_USB_HOST, "GetEdmTrustListPolicy failed. sendRet =  %{public}d, ret = %{public}d",
            sendRet, ret);
        return UEC_SERVICE_EDM_SEND_REQUEST_FAILED;
    }

    int32_t size = reply.ReadInt32();
    if (size < 0 || static_cast<uint32_t>(size) > TRUSTLIST_POLICY_MAX_DEVICES) {
        USB_HILOGE(MODULE_USB_HOST, "EdmTypeList size=[%{public}d] is invalid", size);
        return UEC_SERVICE_EDM_DEVICE_SIZE_EXCEED;
    }
    USB_HILOGI(MODULE_USB_HOST, "GetEdmTrustListPolicy return size:%{public}d", size);
    for (int32_t i = 0; i < size; i++) {
        UsbDeviceId usbDeviceId;
        usbDeviceId.vendorId = reply.ReadInt32();
        usbDeviceId.productId = reply.ReadInt32();
        trustUsbDeviceIds.emplace_back(usbDeviceId);
    }
    return UEC_OK;
}

int32_t UsbHostManager::ManageInterface(const HDI::Usb::V1_0::UsbDev &dev, uint8_t interfaceId, bool disable)
{
#ifdef USB_MANAGER_PASS_THROUGH
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::ManageInterface usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    const HDI::Usb::V2_0::UsbDev &usbDev_ = reinterpret_cast<const HDI::Usb::V2_0::UsbDev &>(dev);
    return usbHostInterface_->ManageInterface(usbDev_, interfaceId, disable);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbd_->ManageInterface(dev, interfaceId, disable);
#endif // USB_MANAGER_PASS_THROUGH
}

void UsbHostManager::ExecuteManageDeviceType(const std::vector<UsbDeviceType> &disableType, bool disable,
    const std::unordered_map<InterfaceType, std::vector<int32_t>> &map, bool isDev)
{
    std::vector<InterfaceType> interfaceTypes;
    FindMatchingTypes(map, isDev, interfaceTypes, disableType);

    for (auto& [interfaceTypeValues, typeValues] : map) {
        bool canFind = false;
        for (auto disallowedValues : interfaceTypes) {
            if (interfaceTypeValues == disallowedValues) {
                canFind = true;
                break;
            }
        }
        bool execDisable = canFind? disable : !disable;
        if (isDev) {
            ManageDeviceTypeImpl(interfaceTypeValues, execDisable);
        } else {
            ManageInterfaceTypeImpl(interfaceTypeValues, execDisable);
        }
    }
}

void UsbHostManager::FindMatchingTypes(const std::unordered_map<InterfaceType, std::vector<int32_t>> &map,
    bool isDev, std::vector<InterfaceType> &matchingTypes, const std::vector<UsbDeviceType> &disableType)
{
    for (const auto &dev : disableType) {
        bool isMatch = false;
        if (dev.isDeviceType != isDev) {
            continue;
        }
        for (auto& [interfaceTypeValues, typeValues] : map) {
            if ((typeValues[BASECLASS_INDEX] == dev.baseClass) &&
                (typeValues[SUBCLASS_INDEX] == -1 || typeValues[SUBCLASS_INDEX] == dev.subClass) &&
                (typeValues[PROTOCAL_INDEX] == -1 || typeValues[PROTOCAL_INDEX] == dev.protocol)) {
                    isMatch = true;
                    matchingTypes.emplace_back(interfaceTypeValues);
                    break;
            }
        }
        if (!isMatch) {
            USB_HILOGE(MODULE_USB_HOST, "is not in the type list, %{public}d, %{public}d, %{public}d",
                dev.baseClass, dev.subClass, dev.protocol);
        }
    }
}

int32_t UsbHostManager::ManageGlobalInterfaceImpl(bool disable)
{
    USB_HILOGI(MODULE_USB_HOST, "list size %{public}zu", devices_.size());
    std::shared_lock lock(devicesMutex_);
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if ((disable && it->second->GetClass() != BASE_CLASS_HUB) ||
            (IsUsbSerialDisable() && IsUsbSerialDevice(*it->second))) {
            continue;
        }
        UsbDev dev = {it->second->GetBusNum(), it->second->GetDevAddr()};
        int32_t ret = UsbDeviceAuthorize(dev.busNum, dev.devAddr, !disable, "GlobalType");
        USB_HILOGI(MODULE_USB_HOST, "UsbDeviceAuthorize ret = %{public}d", ret);
        if (!disable) {
            ret = OpenDevice(dev.busNum, dev.devAddr);
            if (ret != UEC_OK) {
                USB_HILOGW(MODULE_USB_HOST, "ManageGlobalInterfaceImpl open fail ret = %{public}d", ret);
                continue;
            }
            // global authorization need to enable all interfaces
            uint8_t configIndex = 0;
            if (GetActiveConfig(dev.busNum, dev.devAddr, configIndex) || (configIndex < 1)) {
                USB_HILOGW(MODULE_USB_HOST, "get device active config failed.");
                (void)Close(dev.busNum, dev.devAddr);
                continue;
            }
            uint8_t index = static_cast<uint8_t>(configIndex) - 1;
            if (index >= it->second->GetConfigs().size()) {
                USB_HILOGW(MODULE_USB_HOST, "get device config info failed.");
                (void)Close(dev.busNum, dev.devAddr);
                continue;
            }
            for (auto &interface : it->second->GetConfigs()[index].GetInterfaces()) {
                UsbInterfaceAuthorize(dev, it->second->GetConfigs()[index].GetId(), interface.GetId(), !disable);
                interface.SetAuthorizeStatus(!disable);
            }
            if (Close(dev.busNum, dev.devAddr) != UEC_OK) {
                USB_HILOGW(MODULE_USB_HOST, "ManageGlobalInterfaceImpl CloseDevice fail");
            }
        }
    }
    return UEC_OK;
}

int32_t UsbHostManager::ManageDeviceImpl(int32_t vendorId, int32_t productId, bool disable)
{
    USB_HILOGI(MODULE_USB_HOST, "list size %{public}zu, vId: %{public}d, pId: %{public}d, b: %{public}d",
        devices_.size(), vendorId, productId, disable);
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if (it->second->GetClass() == BASE_CLASS_HUB) {
            continue;
        }
        if ((it->second->GetVendorId() == vendorId) && (it->second->GetProductId() == productId)) {
            int32_t ret = OpenDevice(it->second->GetBusNum(), it->second->GetDevAddr());
            if (ret != UEC_OK) {
                USB_HILOGW(MODULE_USB_HOST, "ManageDeviceImpl open fail ret = %{public}d", ret);
                return ret;
            }
            ret = UsbDeviceAuthorize(it->second->GetBusNum(), it->second->GetDevAddr(), !disable, "DeviceType");
            USB_HILOGI(MODULE_USB_HOST, "UsbDeviceAuthorize ret = %{public}d", ret);
            if (Close(it->second->GetBusNum(), it->second->GetDevAddr()) != UEC_OK) {
                USB_HILOGW(MODULE_USB_HOST, "ManageDeviceImpl Close fail");
            }
        }
    }
    return UEC_OK;
}

int32_t UsbHostManager::ManageInterfaceTypeImpl(InterfaceType interfaceType, bool disable)
{
    auto iterInterface = g_typeMap.find(interfaceType);
    if (iterInterface == g_typeMap.end()) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::not find interface type");
        return UEC_SERVICE_INVALID_VALUE;
    }
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if (it->second->GetClass() == BASE_CLASS_HUB || it->second->GetAuthorizeStatus() == DISABLED) {
            continue;
        }
        UsbDev dev = {it->second->GetBusNum(), it->second->GetDevAddr()};
        uint8_t configIndex = 0;
        if (GetActiveConfig(dev.busNum, dev.devAddr, configIndex)) {
            USB_HILOGW(MODULE_USB_HOST, "get device active config failed.");
            continue;
        }
        uint8_t index = static_cast<uint8_t>(configIndex) - 1;
        if (index >= it->second->GetConfigs().size()) {
            USB_HILOGW(MODULE_USB_HOST, "get device config info failed.");
            continue;
        }
        for (auto &interface : it->second->GetConfigs()[index].GetInterfaces()) {
            int32_t ret = RANDOM_VALUE_INDICATE;
            bool needReport = true;
            if (interface.GetAuthorizeStatus() == !disable) {
                needReport = false;
            }
            // 0 indicate base class, 1 indicate subclass, 2 indicate protocal. -1 indicate any value.
            if ((interface.GetClass() == iterInterface->second[BASECLASS_INDEX]) &&
                (interface.GetSubClass() == iterInterface->second[SUBCLASS_INDEX] ||
                iterInterface->second[SUBCLASS_INDEX] == RANDOM_VALUE_INDICATE) &&
                (interface.GetProtocol() == iterInterface->second[PROTOCAL_INDEX] ||
                iterInterface->second[PROTOCAL_INDEX] == RANDOM_VALUE_INDICATE)) {
                USB_HILOGI(MODULE_USB_HOST, "size %{public}zu, interfaceType: %{public}d, disable: %{public}d",
                    devices_.size(), static_cast<int32_t>(interfaceType), disable);
                ret = UsbInterfaceAuthorize(dev, it->second->GetConfigs()[index].GetId(), interface.GetId(), !disable);
                interface.SetAuthorizeStatus(disable ? DISABLED : ENABLED);
                USB_HILOGI(MODULE_USB_HOST, "UsbInterfaceAuthorize ret = %{public}d", ret);
            }
            if (disable && needReport && ret == UEC_OK) {
                ReportManageDeviceInfo("InterfaceType", it->second, &interface, true);
            }
        }
    }
    return UEC_OK;
}

int32_t UsbHostManager::ManageDeviceTypeImpl(InterfaceType interfaceType, bool disable)
{
    auto iterInterface = d_typeMap.find(interfaceType);
    int32_t ret;
    if (iterInterface == d_typeMap.end()) {
        USB_HILOGE(MODULE_USB_HOST, "UsbHostManager::not find interface type");
        return UEC_SERVICE_INVALID_VALUE;
    }
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if (IsUsbSerialDisable() && IsUsbSerialDevice(*it->second)) {
            continue;   // managed by usb serial policy
        }
        if ((it->second->GetClass() == iterInterface->second[BASECLASS_INDEX]) &&
            (it->second->GetSubclass() == iterInterface->second[SUBCLASS_INDEX] ||
            iterInterface->second[SUBCLASS_INDEX] == RANDOM_VALUE_INDICATE) &&
            (it->second->GetProtocol() == iterInterface->second[PROTOCAL_INDEX] ||
            iterInterface->second[PROTOCAL_INDEX] == RANDOM_VALUE_INDICATE)) {
            USB_HILOGI(MODULE_USB_HOST, "list size %{public}zu, interfaceType: %{public}d, disable: %{public}d",
                devices_.size(), static_cast<int32_t>(interfaceType), disable);
            ret = OpenDevice(it->second->GetBusNum(), it->second->GetDevAddr());
            if (ret != UEC_OK) {
                USB_HILOGW(MODULE_USB_HOST, "ManageDeviceTypeImpl open fail ret = %{public}d", ret);
                continue;
            }
            ret = UsbDeviceAuthorize(it->second->GetBusNum(), it->second->GetDevAddr(), !disable, "InterfaceType");
            USB_HILOGI(MODULE_USB_HOST, "UsbDeviceAuthorize ret = %{public}d", ret);
            if (Close(it->second->GetBusNum(), it->second->GetDevAddr()) != UEC_OK) {
                USB_HILOGW(MODULE_USB_HOST, "ManageDeviceTypeImpl CloseDevice fail");
            }
        }
    }
    return UEC_OK;
}

void UsbHostManager::SetSerialManager(std::shared_ptr<SERIAL::SerialManager> serialManager)
{
    USB_HILOGI(MODULE_USB_HOST, "%{public}s: enter", __func__);
    if (serialManager != nullptr) {
        USB_HILOGI(MODULE_USB_HOST, "%{public}s: update serial manager", __func__);
        usbSerialManager_ = serialManager;
    }
}

void UsbHostManager::AddUsbSerialDevice(UsbDevice &dev)
{
    USB_HILOGI(MODULE_USB_HOST, "%{public}s: enter", __func__);
    if (usbSerialManager_ == nullptr) {
        USB_HILOGW(MODULE_USB_HOST, "%{public}s: serial not init", __func__);
        return;
    }
    std::vector<OHOS::HDI::Usb::Serial::V1_0::SerialPort> serialList;
    if (usbSerialManager_->SerialGetPortList(serialList) != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "%{public}s: failed to get serial devices", __func__);
        return;
    }
    for (auto &port : serialList) {
        if (port.deviceInfo.busNum == dev.GetBusNum() && port.deviceInfo.devAddr == dev.GetDevAddr()) {
            auto it = std::find_if(serialDevices_.begin(), serialDevices_.end(), [dev](auto &devIt) {
                return dev.GetBusNum() == devIt.busNum && dev.GetDevAddr() == devIt.devAddr;
            });
            if (it == serialDevices_.end()) {
                HDI::Usb::V1_0::UsbDev usbDev = {dev.GetBusNum(), dev.GetDevAddr()};
                serialDevices_.emplace_back(usbDev);
                USB_HILOGI(MODULE_USB_HOST, "%{public}s: add usb serial device to vector", __func__);
            } else {
                USB_HILOGW(MODULE_USB_HOST, "%{public}s: usb serial device already added", __func__);
            }
            return; // {busNum, devAddr} already matched
        }
    }
}

bool UsbHostManager::IsUsbSerialDevice(UsbDevice &dev)
{
    for (auto it = serialDevices_.begin(); it != serialDevices_.end(); ++it) {
        if ((it->busNum == dev.GetBusNum()) && (it->devAddr == dev.GetDevAddr())) {
            return true;
        }
    }
    return false;
}

int32_t UsbHostManager::ManageUsbSerialDevice(bool disable)
{
    USB_HILOGI(MODULE_USB_HOST, "%{public}s: enter", __func__);
    std::shared_lock lock(devicesMutex_);
    for (auto &dev : serialDevices_) {
        (void)UsbDeviceAuthorize(dev.busNum, dev.devAddr, !disable, "UsbSerialType");
    }
    if (!disable) {
        lock.unlock();
        ExecuteStrategy();
    }
    return UEC_OK;
}

bool UsbHostManager::IsUsbSerialDisable()
{
    // activate EDM && system parameter == "1"
    std::string isSerialDisable = OHOS::system::GetParameter("persist.edm.usb_serial_disable", "0");
    return IsEdmEnabled() && (isSerialDisable == "1");
}

void UsbHostManager::ReportManageDeviceInfo(const std::string &operationType, UsbDevice* device,
                                            const UsbInterface* interface, bool isInterfaceType)
{
    USB_HILOGI(MODULE_USB_HOST, "ReportManageDeviceInfo");
    int32_t vid = device->GetVendorId();
    int32_t pid = device->GetProductId();
    int32_t baseClass = RANDOM_VALUE_INDICATE;
    int32_t subClass = RANDOM_VALUE_INDICATE;
    int32_t protocol = RANDOM_VALUE_INDICATE;
    if (isInterfaceType) {
        baseClass = interface->GetClass();
        subClass = interface->GetSubClass();
        protocol = interface->GetProtocol();
    } else {
        baseClass = device->GetClass();
        subClass = device->GetSubclass();
        protocol = device->GetProtocol();
    }
    HiSysEventWrite(HiSysEvent::Domain::USB, "DEVICE_SECURITY_POLICY",
        HiSysEvent::EventType::SECURITY, "SECURITY_POLICY_TYPE", operationType,
        "VID", vid,
        "PID", pid,
        "CLASS", baseClass,
        "SUBCLASS", subClass,
        "PROTOCOL", protocol);
}

void UsbHostManager::ExecuteManageUsbType(const std::vector<UsbDeviceType> &disableType, bool disable)
{
    std::vector<UsbDeviceType> deviceTypes;
    std::vector<UsbDeviceType> interfaceTypes;
    for (const auto &type : disableType) {
        if (type.isDeviceTypeAllMatch) {
            deviceTypes.emplace_back(type);
            interfaceTypes.emplace_back(type);
        } else if (type.isDeviceType) {
            deviceTypes.emplace_back(type);
        } else {
            interfaceTypes.emplace_back(type);
        }
    }

    std::shared_lock lock(devicesMutex_);
    ManageUsbTypeDeviceImpl(deviceTypes, disable);
    ManageUsbTypeInterfaceImpl(interfaceTypes, disable);
}

void UsbHostManager::ManageUsbTypeDeviceImpl(const std::vector<UsbDeviceType> &types, bool disable)
{
    USB_HILOGI(MODULE_USB_HOST, "%{public}s enter", __func__);
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if (IsUsbSerialDisable() && IsUsbSerialDevice(*it->second)) {
            continue;
        }
        bool notMatched = true;
        for (const auto &type : types) {
            if ((type.baseClass == it->second->GetClass()) &&
                (type.subClass == RANDOM_VALUE_INDICATE || type.subClass == it->second->GetSubclass()) &&
                (type.protocol == RANDOM_VALUE_INDICATE || type.protocol == it->second->GetProtocol())) {
                auto ret = UsbDeviceAuthorize(it->second->GetBusNum(), it->second->GetDevAddr(), !disable, "UsbType");
                USB_HILOGI(MODULE_USB_HOST, "UsbDeviceAuthorize ret = %{public}d", ret);
                notMatched = false;
                break;
            }
        }
        if (notMatched) {
            // not matched => authorized = !!disable
            (void)UsbDeviceAuthorize(it->second->GetBusNum(), it->second->GetDevAddr(), disable, "UsbType");
        }
    }
}

void UsbHostManager::ManageUsbTypeInterfaceImpl(const std::vector<UsbDeviceType> &types, bool disable)
{
    USB_HILOGI(MODULE_USB_HOST, "%{public}s enter", __func__);
    int32_t ret = UEC_OK;

    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if (it->second->GetClass() == BASE_CLASS_HUB || it->second->GetAuthorizeStatus() == DISABLED) {
            continue;
        }

        UsbDev dev = {it->second->GetBusNum(), it->second->GetDevAddr()};
        ret = OpenDevice(dev.busNum, dev.devAddr);
        if (ret != UEC_OK) {
            USB_HILOGW(MODULE_USB_HOST, "ManageUsbType open fail ret = %{public}d", ret);
        }
        uint8_t configIndex = 0;
        if (GetActiveConfig(dev.busNum, dev.devAddr, configIndex)) {
            USB_HILOGW(MODULE_USB_HOST, "get device active config failed.");
            continue;
        }
        uint8_t index = static_cast<uint8_t>(configIndex) - 1;
        if (index >= it->second->GetConfigs().size()) {
            USB_HILOGW(MODULE_USB_HOST, "get device config info failed.");
            continue;
        }

        for (auto &interface : it->second->GetConfigs()[index].GetInterfaces()) {
            if (IsUsbInterfaceTypeMatched(types, interface)) {
                ret = UsbInterfaceAuthorize(dev, it->second->GetConfigs()[index].GetId(), interface.GetId(), !disable);
                interface.SetAuthorizeStatus(disable ? DISABLED : ENABLED);
                USB_HILOGI(MODULE_USB_HOST, "UsbInterfaceAuthorize ret = %{public}d", ret);
            } else {
                // not matched => authorized = !!disable
                UsbInterfaceAuthorize(dev, it->second->GetConfigs()[index].GetId(), interface.GetId(), disable);
                interface.SetAuthorizeStatus(disable ? ENABLED : DISABLED);
                continue;
            }
            if (disable && ret == UEC_OK) {
                ReportManageDeviceInfo("UsbType", it->second, &interface, true);
            }
        }

        (void)Close(dev.busNum, dev.devAddr);
    }
}

bool UsbHostManager::IsUsbInterfaceTypeMatched(const std::vector<UsbDeviceType> &types, const UsbInterface intf)
{
    for (const auto &type : types) {
        if ((type.baseClass == intf.GetClass()) &&
            (type.subClass == RANDOM_VALUE_INDICATE || type.subClass == intf.GetSubClass()) &&
            (type.protocol == RANDOM_VALUE_INDICATE || type.protocol == intf.GetProtocol())) {
            return true;
        }
    }
    return false;
}
} // namespace USB
} // namespace OHOS
