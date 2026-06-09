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

#include "usb_service.h"

#include <cstdio>
#include <iostream>
#include <ipc_skeleton.h>
#include <sstream>
#include <string>
#include <filesystem>
#include <unistd.h>
#include <unordered_map>
#include "parameters.h"
#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"
#include "common_timer_errors.h"
#include "file_ex.h"
#include "if_system_ability_manager.h"
#include "iproxy_broker.h"
#include "iservice_registry.h"
#include "iusb_srv.h"
#include "securec.h"
#include "system_ability_definition.h"
#include "usb_common.h"
#include "usb_descriptor_parser.h"
#include "usb_errors.h"
#include "usb_napi_errors.h"
#include "usb_port_manager.h"
#include "usb_right_manager.h"
#include "usb_right_db_helper.h"
#include "usb_report_sys_event.h"
#include "usb_settings_datashare.h"
#include "tokenid_kit.h"
#include "accesstoken_kit.h"
#include "mem_mgr_proxy.h"
#include "mem_mgr_client.h"
#include "uri.h"
#include "usb_function_switch_window.h"
#include "usbd_transfer_callback_impl.h"
#include "hitrace_meter.h"
#include "hisysevent.h"

using OHOS::sptr;
using OHOS::HiviewDFX::HiSysEvent;
using namespace OHOS::HDI::Usb::V1_2;
using namespace OHOS::Security::AccessToken;
namespace OHOS {
namespace USB {
namespace {
constexpr int32_t COMMEVENT_REGISTER_RETRY_TIMES = 10;
constexpr int32_t COMMEVENT_REGISTER_WAIT_DELAY_US = 20000;
constexpr int32_t SERVICE_STARTUP_MAX_TIME = 30;
constexpr uint32_t UNLOAD_SA_TIMER_INTERVAL = 30 * 1000;
constexpr uint32_t SERIALREAD_SIZE_MAX = 64 * 1024;
constexpr uint32_t SERIALWRITE_SIZE_MAX = 200 * 1024;
#ifdef USB_MANAGER_FEATURE_HOST
constexpr int32_t ISO_TRANSFER_TYPE = 1;
constexpr int32_t BULK_TRANSFER_TYPE = 2;
constexpr int32_t INTP_TRANSFER_TYPE = 3;
constexpr uint32_t MEMSIZE_MAX = 512 * 1024 * 1024;
constexpr uint32_t ARGLIST_SIZE_MIN = 2;
#endif // USB_MANAGER_FEATURE_HOST
#if defined(USB_MANAGER_FEATURE_HOST) || defined(USB_MANAGER_FEATURE_DEVICE)
constexpr int32_t USB_RIGHT_USERID_INVALID = -1;
#endif // USB_MANAGER_FEATURE_HOST || USB_MANAGER_FEATURE_DEVICE
constexpr int32_t API_VERSION_ID_18 = 18;
static const std::filesystem::path TTYUSB_PATH = "/sys/bus/usb-serial/devices";
constexpr const pid_t ROOT_UID = 0;
constexpr const pid_t EDM_UID = 3057;
} // namespace
auto g_serviceInstance = DelayedSpSingleton<UsbService>::GetInstance();
const bool G_REGISTER_RESULT =
    SystemAbility::MakeAndRegisterAbility(DelayedSpSingleton<UsbService>::GetInstance().GetRefPtr());

UsbService::UsbService() : SystemAbility(USB_SYSTEM_ABILITY_ID, true)
{
    usbRightManager_ = std::make_shared<UsbRightManager>();
#ifdef USB_MANAGER_PASS_THROUGH

#ifdef USB_MANAGER_FEATURE_HOST
    usbHostManager_ = std::make_shared<UsbHostManager>(nullptr);
#endif // USB_MANAGER_FEATURE_HOST
#ifdef USB_MANAGER_FEATURE_PORT
    usbPortManager_ = std::make_shared<UsbPortManager>();
#endif // USB_MANAGER_FEATURE_PORT
#ifdef USB_MANAGER_FEATURE_DEVICE
    usbDeviceManager_ = std::make_shared<UsbDeviceManager>();
    usbAccessoryManager_ = std::make_shared<UsbAccessoryManager>();
#endif // USB_MANAGER_FEATURE_DEVICE

#else
    if (usbd_ == nullptr) {
        usbd_ = OHOS::HDI::Usb::V1_2::IUsbInterface::Get();
        USB_HILOGI(MODULE_USB_SERVICE, "%{public}s:%{public}d usbd_ == nullptr: %{public}d",
            __func__, __LINE__, usbd_ == nullptr);
    } else {
        USB_HILOGW(MODULE_USB_SERVICE, "%{public}s:usbd_ != nullptr", __func__);
    }
#endif // USB_MANAGER_PASS_THROUGH
}

UsbService::~UsbService() {}

// LCOV_EXCL_START
#ifndef USB_MANAGER_PASS_THROUGH
int32_t UsbService::SetUsbd(const sptr<OHOS::HDI::Usb::V1_2::IUsbInterface> &usbd)
{
    if (usbd == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbService usbd is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    if (usbd_ == nullptr) {
        USB_HILOGI(MODULE_USB_SERVICE, "%{public}s:usbd_ = usbd", __func__);
        usbd_ = usbd;
    } else {
        USB_HILOGW(MODULE_USB_SERVICE, "%{public}s:usbd_ != nullptr", __func__);
    }
#ifdef USB_MANAGER_FEATURE_PORT
    if (usbPortManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "invalid usbPortManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    usbPortManager_->SetUsbd(usbd);
#endif // USB_MANAGER_FEATURE_PORT
#ifdef USB_MANAGER_FEATURE_DEVICE
    if (usbDeviceManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "invalid usbDeviceManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    usbDeviceManager_->SetUsbd(usbd);

    if (usbAccessoryManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "invalid usbAccessoryManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    usbAccessoryManager_->SetUsbd(usbd);
#endif // USB_MANAGER_FEATURE_DEVICE
    return UEC_OK;
}
#endif // USB_MANAGER_PASS_THROUGH
// LCOV_EXCL_STOP

#ifdef USB_MANAGER_PASS_THROUGH
// LCOV_EXCL_START
UsbService::SystemAbilityStatusChangeListener::SystemAbilityStatusChangeListener(
    sptr<UsbManagerSubscriber> usbManagerSubscriber)
    : usbManagerSubscriber_(usbManagerSubscriber)
{
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::SystemAbilityStatusChangeListener::OnAddSystemAbility(
    int32_t systemAbilityId, const std::string &deviceId)
{
    USB_HILOGI(MODULE_USB_SERVICE, "OnAddSystemAbility ID = %{public}d", systemAbilityId);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::SystemAbilityStatusChangeListener::OnRemoveSystemAbility(
    int32_t systemAbilityId, const std::string &deviceId)
{
    USB_HILOGI(MODULE_USB_SERVICE, "OnRemoveSystemAbility ID = %{public}d", systemAbilityId);
    if (systemAbilityId == USB_SYSTEM_ABILITY_ID) {
#ifdef USB_MANAGER_FEATURE_HOST
        sptr<OHOS::HDI::Usb::V2_0::IUsbHostInterface> usbHostInterface_ =
            OHOS::HDI::Usb::V2_0::IUsbHostInterface::Get("usb_host_interface_service", true);
        if (usbHostInterface_ != nullptr) {
            usbHostInterface_->UnbindUsbdHostSubscriber(usbManagerSubscriber_);
        }
#endif // USB_MANAGER_FEATURE_HOST
#ifdef USB_MANAGER_FEATURE_DEVICE
        sptr<OHOS::HDI::Usb::V2_0::IUsbDeviceInterface> usbDeviceInterface_ =
            OHOS::HDI::Usb::V2_0::IUsbDeviceInterface::Get();
        if (usbDeviceInterface_ != nullptr) {
            usbDeviceInterface_->UnbindUsbdDeviceSubscriber(usbManagerSubscriber_);
        }
#endif // USB_MANAGER_FEATURE_DEVICE
#ifdef USB_MANAGER_FEATURE_PORT
        sptr<OHOS::HDI::Usb::V2_0::IUsbPortInterface> usbPortInterface_ =
            OHOS::HDI::Usb::V2_0::IUsbPortInterface::Get();
        if (usbPortInterface_ != nullptr) {
            usbPortInterface_->UnbindUsbdPortSubscriber(usbManagerSubscriber_);
        }
#endif // USB_MANAGER_FEATURE_PORT
    }
}
// LCOV_EXCL_STOP
#else
// LCOV_EXCL_START
UsbService::SystemAbilityStatusChangeListener::SystemAbilityStatusChangeListener(
    sptr<UsbServiceSubscriber> usbdSubscriber)
    : usbdSubscriber_(usbdSubscriber)
{
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::SystemAbilityStatusChangeListener::OnAddSystemAbility(
    int32_t systemAbilityId, const std::string &deviceId)
{
    USB_HILOGI(MODULE_USB_SERVICE, "OnAddSystemAbility ID = %{public}d", systemAbilityId);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::SystemAbilityStatusChangeListener::OnRemoveSystemAbility(
    int32_t systemAbilityId, const std::string &deviceId)
{
    USB_HILOGI(MODULE_USB_SERVICE, "OnRemoveSystemAbility ID = %{public}d", systemAbilityId);
    if (systemAbilityId == USB_SYSTEM_ABILITY_ID) {
        sptr<OHOS::HDI::Usb::V1_2::IUsbInterface> usbd_ = OHOS::HDI::Usb::V1_2::IUsbInterface::Get();
        USB_HILOGI(MODULE_USB_SERVICE, "%{public}s:%{public}d usbd_ == nullptr: %{public}d",
            __func__, __LINE__, usbd_ == nullptr);
        if (usbd_ != nullptr) {
            usbd_->UnbindUsbdSubscriber(usbdSubscriber_);
        }
    }
}
// LCOV_EXCL_STOP
#endif // USB_MANAGER_PASS_THROUGH

// LCOV_EXCL_START
void UsbService::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    USB_HILOGI(MODULE_USB_SERVICE, "OnAddSystemAbility systemAbilityId:%{public}d", systemAbilityId);
    if (systemAbilityId == MEMORY_MANAGER_SA_ID) {
        Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 1, USB_SYSTEM_ABILITY_ID);
        Memory::MemMgrClient::GetInstance().SetCritical(getpid(), true, USB_SYSTEM_ABILITY_ID);
    }
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::WaitUsbdService()
{
    // wait for the usbd service to start and bind usb service and usbd service
    int32_t retryTimes = 0;
    while (retryTimes < SERVICE_STARTUP_MAX_TIME) {
        if (InitUsbd()) {
            break;
        }
        sleep(1);
        retryTimes++;

        if (retryTimes == SERVICE_STARTUP_MAX_TIME) {
            USB_HILOGE(MODULE_USB_SERVICE, "OnStart call initUsbd failed");
            return;
        }
    }
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::OnStart()
{
    USB_HILOGI(MODULE_USB_SERVICE, "usb_service OnStart enter");
    if (ready_) {
        USB_HILOGE(MODULE_USB_SERVICE, "OnStart is ready, nothing to do");
        return;
    }

    if (!(Init())) {
        USB_HILOGE(MODULE_USB_SERVICE, "OnStart call init fail");
        return;
    }

    WaitUsbdService();

#ifdef USB_MANAGER_FEATURE_HOST
    if (OHOS::system::GetBoolParameter("const.SystemCapability.USB.USBManager.Serial", false)) {
        std::unique_lock lock(serialManagerMutex_);
        usbSerialManager_ = std::make_shared<SERIAL::SerialManager>();
        usbHostManager_->SetSerialManager(usbSerialManager_);
    }
    auto watchRet = WatchParameter("persist.edm.usb_serial_disable",
        [](const char *key, const char *value, void *context) {
            if (key == nullptr || value == nullptr) {
                USB_HILOGE(MODULE_USB_SERVICE, "param null pointer");
                return;
            }
            USB_HILOGI(MODULE_USB_SERVICE, "watch serial param: get %{public}s=%{public}s", key, value);
            bool disable = (std::string(value) == "1");
            g_serviceInstance->ManageUsbSerialDevice(disable);
        }, this);
    if (watchRet != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "failed to watch usb_serial_disable parameter");
    }
#endif // USB_MANAGER_FEATURE_HOST
#ifdef USB_MANAGER_FEATURE_PORT
    if (usbPortManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "invalid usbPortManager_");
        return;
    }
    usbPortManager_->Init();
#endif // USB_MANAGER_FEATURE_PORT
#ifdef USB_MANAGER_FEATURE_DEVICE
    if (usbDeviceManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "invalid usbDeviceManager_");
        return;
    }
    (void)usbDeviceManager_->Init();
#endif // USB_MANAGER_FEATURE_DEVICE
    (void)InitUsbRight();
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
#ifdef USB_MANAGER_PASS_THROUGH
    sptr<ISystemAbilityStatusChange> status =
        new (std::nothrow) SystemAbilityStatusChangeListener(usbManagerSubscriber_);
#else
    sptr<ISystemAbilityStatusChange> status = new (std::nothrow) SystemAbilityStatusChangeListener(usbdSubscriber_);
#endif // USB_MANAGER_PASS_THROUGH
    if (samgrProxy == nullptr || status == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "samgrProxy or SystemAbilityStatusChangeListener is nullptr");
        return;
    }
    int32_t ret = samgrProxy->SubscribeSystemAbility(USB_SYSTEM_ABILITY_ID, status);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "SubscribeSystemAbility failed. ret = %{public}d", ret);
        return;
    }
    AddSystemAbilityListener(MEMORY_MANAGER_SA_ID);
    if (!Publish(g_serviceInstance)) {
        USB_HILOGE(MODULE_USB_SERVICE, "OnStart register to system ability manager failed.");
        return;
    }
    ready_ = true;
    UnLoadSelf(UsbService::UnLoadSaType::UNLOAD_SA_DELAY);
#ifdef USB_MANAGER_FEATURE_DEVICE
    if (!InitSettingsDataHdcStatus()) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbService::OnStart update HDC_STATUS failed!");
    }
#endif // USB_MANAGER_FEATURE_DEVICE
    USB_HILOGE(MODULE_USB_SERVICE, "OnStart and add system ability success");
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
bool UsbService::Init()
{
    USB_HILOGI(MODULE_USB_SERVICE, "usb_service Init enter");

    while (commEventRetryTimes_ <= COMMEVENT_REGISTER_RETRY_TIMES) {
        if (!IsCommonEventServiceAbilityExist()) {
            ++commEventRetryTimes_;
            usleep(COMMEVENT_REGISTER_WAIT_DELAY_US);
        } else {
            commEventRetryTimes_ = 0;
            break;
        }
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Init success");
    return true;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
#ifdef USB_MANAGER_PASS_THROUGH
bool UsbService::InitUsbd()
{
#ifdef USB_MANAGER_FEATURE_DEVICE
    if (usbAccessoryManager_ == nullptr || !usbAccessoryManager_->InitUsbAccessoryInterface()) {
        USB_HILOGE(MODULE_USB_SERVICE, "usbAccessoryManager_ is nullptr or InitUsbAccessoryInterface failed");
        return false;
    }
    if (usbDeviceManager_ == nullptr || !usbDeviceManager_->InitUsbDeviceInterface()) {
        USB_HILOGE(MODULE_USB_SERVICE, "usbDeviceManager_ is nullptr or InitUsbDeviceInterface failed");
        return false;
    }
#endif // USB_MANAGER_FEATURE_DEVICE
#ifdef USB_MANAGER_FEATURE_PORT
    if (usbPortManager_ == nullptr || !usbPortManager_->InitUsbPortInterface()) {
        USB_HILOGE(MODULE_USB_SERVICE, "usbPortManager_ is nullptr or InitUsbPortInterface failed");
        return false;
    }
#endif // USB_MANAGER_FEATURE_PORT
#ifdef USB_MANAGER_FEATURE_HOST
    if (usbHostManager_ == nullptr || !usbHostManager_->InitUsbHostInterface()) {
        USB_HILOGE(MODULE_USB_SERVICE, "usbHostManager_ is nullptr or InitUsbHostInterface failed");
        return false;
    }
#endif // USB_MANAGER_FEATURE_HOST
    return true;
}
#else
bool UsbService::InitUsbd()
{
    if (usbd_ == nullptr) {
        usbd_ = OHOS::HDI::Usb::V1_2::IUsbInterface::Get();
        USB_HILOGI(MODULE_USB_SERVICE, "%{public}s:%{public}d usbd_ == nullptr: %{public}d",
            __func__, __LINE__, usbd_ == nullptr);
    } else {
        USB_HILOGW(MODULE_USB_SERVICE, "%{public}s:usbd_ != nullptr", __func__);
    }
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, " get usbd_ is nullptr");
        return false;
    }
    usbdSubscriber_ = new (std::nothrow) UsbServiceSubscriber();
    if (usbdSubscriber_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "Init failed");
        return false;
    }
    recipient_ = new UsbdDeathRecipient();
    sptr<IRemoteObject> remote = OHOS::HDI::hdi_objcast<HDI::Usb::V1_0::IUsbInterface>(usbd_);
    if (!remote->AddDeathRecipient(recipient_)) {
        USB_HILOGE(MODULE_USB_SERVICE, "add DeathRecipient failed");
        return false;
    }

#ifdef USB_MANAGER_FEATURE_HOST
    usbHostManager_ = std::make_shared<UsbHostManager>(nullptr);
#endif // USB_MANAGER_FEATURE_HOST
#ifdef USB_MANAGER_FEATURE_PORT
    usbPortManager_ = std::make_shared<UsbPortManager>();
#endif // USB_MANAGER_FEATURE_PORT
#ifdef USB_MANAGER_FEATURE_DEVICE
    usbDeviceManager_ = std::make_shared<UsbDeviceManager>();
    usbAccessoryManager_ = std::make_shared<UsbAccessoryManager>();
#endif // USB_MANAGER_FEATURE_DEVICE
    ErrCode ret = usbd_->BindUsbdSubscriber(usbdSubscriber_);
    USB_HILOGI(MODULE_USB_SERVICE, "entry InitUsbd ret: %{public}d", ret);
    return ret == UEC_OK;
}
#endif // USB_MANAGER_PASS_THROUGH
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::OnStop()
{
    USB_HILOGI(MODULE_USB_SERVICE, "entry stop service %{public}d", ready_);
    if (!ready_) {
        return;
    }
    ready_ = false;
#ifdef USB_MANAGER_PASS_THROUGH
#ifdef USB_MANAGER_FEATURE_HOST
    if (usbHostManager_ == nullptr) {
        return;
    }
    usbHostManager_->Stop();
#endif // USB_MANAGER_FEATURE_HOST
#ifdef USB_MANAGER_FEATURE_DEVICE
    if (usbDeviceManager_ == nullptr) {
        return;
    }
    usbDeviceManager_->Stop();
#endif // USB_MANAGER_FEATURE_DEVICE
#ifdef USB_MANAGER_FEATURE_PORT
    if (usbPortManager_ == nullptr) {
        return;
    }
    usbPortManager_->Stop();
#endif // USB_MANAGER_FEATURE_PORT
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbService::usbd_ is nullptr");
        return;
    }
    sptr<IRemoteObject> remote = OHOS::HDI::hdi_objcast<HDI::Usb::V1_0::IUsbInterface>(usbd_);
    remote->RemoveDeathRecipient(recipient_);
    recipient_.clear();
    usbd_->UnbindUsbdSubscriber(usbdSubscriber_);
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 0, USB_SYSTEM_ABILITY_ID);
#endif // USB_MANAGER_PASS_THROUGH
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
bool UsbService::IsCommonEventServiceAbilityExist()
{
    sptr<ISystemAbilityManager> sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE,
            "IsCommonEventServiceAbilityExist Get ISystemAbilityManager "
            "failed, no SystemAbilityManager");
        return false;
    }
    sptr<IRemoteObject> remote = sm->CheckSystemAbility(COMMON_EVENT_SERVICE_ID);
    if (!remote) {
        USB_HILOGE(MODULE_USB_SERVICE, "No CesServiceAbility");
        return false;
    }
    return true;
}
// LCOV_EXCL_STOP

#ifdef USB_MANAGER_FEATURE_HOST
int32_t UsbService::OpenDevice(uint8_t busNum, uint8_t devAddr, const sptr<IRemoteObject> &deviceRemote)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        ReportUsbOperationFaultSysEvent("OpenDevice", UEC_SERVICE_PERMISSION_DENIED, "CheckDevicePermission failed");
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    deviceRecipient_ = new DeviceDeathRecipient(busNum, devAddr);
    if (deviceRecipient_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::OpenDevice deviceRecipient_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    if (!deviceRemote->AddDeathRecipient(deviceRecipient_)) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::OpenDevice add DeathRecipient failed");
        return UEC_SERVICE_INVALID_VALUE;
    }

    int32_t ret = usbHostManager_->OpenDevice(busNum, devAddr);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "OpenDevice failed ret:%{public}d", ret);
        deviceRemote->RemoveDeathRecipient(deviceRecipient_);
        deviceRecipient_ = nullptr;
    }

    return ret;
    // LCOV_EXCL_STOP
}

int32_t UsbService::Close(uint8_t busNum, uint8_t devAddr, const sptr<IRemoteObject> &deviceRemote)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbHostManager_->Close(busNum, devAddr);
    if (ret == UEC_OK) {
        deviceRemote->RemoveDeathRecipient(deviceRecipient_);
        deviceRecipient_ = nullptr;
    }
    return ret;
    // LCOV_EXCL_STOP
}

// LCOV_EXCL_START
int32_t UsbService::ResetDevice(uint8_t busNum, uint8_t devAddr)
{
    std::string name = std::to_string(busNum) + "-" + std::to_string(devAddr);
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if (deviceVidPidMap_.find(name) == deviceVidPidMap_.end()) {
            USB_HILOGE(MODULE_USB_HOST, "Device not found: %{public}s", name.c_str());
            return UEC_INTERFACE_NAME_NOT_FOUND;
        }
    }
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        ReportUsbOperationFaultSysEvent("ResetDevice", UEC_SERVICE_PERMISSION_DENIED, "CheckDevicePermission failed");
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbHostManager_->ResetDevice(busNum, devAddr);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "ResetDevice failed ret:%{public}d", ret);
    }

    return ret;
}
// LCOV_EXCL_STOP

int32_t UsbService::ClaimInterface(uint8_t busNum, uint8_t devAddr, uint8_t interface, uint8_t force)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbHostManager_->ClaimInterface(busNum, devAddr, interface, force);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "claim interface false.");
    }
    return ret;
    // LCOV_EXCL_STOP
}

int32_t UsbService::SetInterface(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid, uint8_t altIndex)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostManager_->SetInterface(busNum, devAddr, interfaceid, altIndex);
    // LCOV_EXCL_STOP
}

int32_t UsbService::ReleaseInterface(uint8_t busNum, uint8_t devAddr, uint8_t interface)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbHostManager_->ReleaseInterface(busNum, devAddr, interface);
    if (ret == HDF_DEV_ERR_OP) {
        USB_HILOGE(MODULE_USB_HOST, "ReleaseInterface failed.");
        return UEC_INTERFACE_INVALID_OPERATION;
    }
    return ret;
    // LCOV_EXCL_STOP
}

int32_t UsbService::SetActiveConfig(uint8_t busNum, uint8_t devAddr, uint8_t configIndex)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostManager_->SetActiveConfig(busNum, devAddr, configIndex);
    // LCOV_EXCL_STOP
}

int32_t UsbService::ManageGlobalInterface(bool disable)
{
    if (!IsCallerValid()) {
        USB_HILOGE(MODULE_USB_HOST, "not root or edm process.");
        return UEC_SERVICE_INVALID_OPERATION;
    }
    if (PreCallFunction() != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "PreCallFunction failed");
        return UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED;
    }
    return usbHostManager_->ManageGlobalInterface(disable);
}

int32_t UsbService::ManageDevice(int32_t vendorId, int32_t productId, bool disable)
{
    if (!IsCallerValid()) {
        USB_HILOGE(MODULE_USB_HOST, "not root or edm process.");
        return UEC_SERVICE_INVALID_OPERATION;
    }
    if (PreCallFunction() != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "PreCallFunction failed");
        return UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED;
    }
    return usbHostManager_->ManageDevice(vendorId, productId, disable);
}

void UsbService::UsbDeviceIdChange(const std::vector<UsbDeviceIdInfo> &deviceIdInfoList,
    std::vector<UsbDeviceId> &deviceIdList)
{
    for (auto &deviceInfo : deviceIdInfoList) {
        UsbDeviceId devId;
        devId.productId = deviceInfo.productId;
        devId.vendorId = deviceInfo.vendorId;
        deviceIdList.emplace_back(devId);
    }
    return;
}

int32_t UsbService::ManageDevicePolicy(const std::vector<UsbDeviceIdInfo> &trustList)
{
    if (!IsCallerValid()) {
        USB_HILOGE(MODULE_USB_HOST, "not root or edm process.");
        return UEC_SERVICE_INVALID_OPERATION;
    }
    if (PreCallFunction() != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "PreCallFunction failed");
        return UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED;
    }
    std::vector<UsbDeviceId> devIdList;
    UsbDeviceIdChange(trustList, devIdList);
    return usbHostManager_->ManageDevicePolicy(devIdList);
}

int32_t UsbService::ManageInterfaceType(const std::vector<UsbDeviceTypeInfo> &devTypeInfo, bool disable)
{
    if (!IsCallerValid()) {
        USB_HILOGE(MODULE_USB_HOST, "not root or edm process.");
        return UEC_SERVICE_INVALID_OPERATION;
    }
    if (PreCallFunction() != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "PreCallFunction failed");
        return UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED;
    }
    std::vector<UsbDeviceType> disableType;
    UsbDeviceTypeChange(disableType, devTypeInfo);
    return usbHostManager_->ManageInterfaceType(disableType, disable);
}

int32_t UsbService::ManageUsbType(const std::vector<UsbDeviceTypeInfo> &devTypeInfo, bool disable)
{
    if (!IsCallerValid()) {
        USB_HILOGE(MODULE_USB_HOST, "not root or edm process.");
        return UEC_SERVICE_INVALID_OPERATION;
    }
    if (PreCallFunction() != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "PreCallFunction failed");
        return UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED;
    }
    std::vector<UsbDeviceType> disableType;
    UsbDeviceTypeChange(disableType, devTypeInfo);
    return usbHostManager_->ManageUsbType(disableType, disable);
}

int32_t UsbService::ManageUsbSerialDevice(bool disable)
{
    if (usbSerialManager_ == nullptr || usbHostManager_ == nullptr) {
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostManager_->ManageUsbSerialDevice(disable);
}

// LCOV_EXCL_START
int32_t UsbService::UsbAttachKernelDriver(uint8_t busNum, uint8_t devAddr, uint8_t interface)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    int32_t ret = usbHostManager_->UsbAttachKernelDriver(busNum, devAddr, interface);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "UsbAttachKernelDriver ManageInterface false.");
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::UsbDetachKernelDriver(uint8_t busNum, uint8_t devAddr, uint8_t interface)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    int32_t ret = usbHostManager_->UsbDetachKernelDriver(busNum, devAddr, interface);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "UsbDetachKernelDriver ManageInterface false.");
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::ClearHalt(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId, uint8_t endpointId)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbHostManager_->ClearHalt(busNum, devAddr, interfaceId, endpointId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "ClearHalt error ret:%{public}d", ret);
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
bool UsbService::AddDevice(uint8_t busNum, uint8_t devAddr)
{
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return false;
    }
    UsbDevice *devInfo = new (std::nothrow) UsbDevice();
    if (devInfo == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "new failed");
        return false;
    }

    int32_t ret = GetDeviceInfo(busNum, devAddr, *devInfo);
    USB_HILOGI(MODULE_USB_HOST, "GetDeviceInfo ret=%{public}d", ret);
    if (ret != UEC_OK) {
        delete devInfo;
        return false;
    }

    std::string name = std::to_string(busNum) + "-" + std::to_string(devAddr);
    std::string uniqueName = std::to_string(devInfo->GetVendorId()) + "-" + std::to_string(devInfo->GetProductId()) +
        "-" + devInfo->GetmSerial();
    USB_HILOGI(MODULE_USB_HOST, "map+: %{public}s", name.c_str());
    {
        std::lock_guard<std::mutex> guard(mutex_);
        deviceVidPidMap_.insert(std::pair<std::string, std::string>(name, uniqueName));
    }

    usbHostManager_->AddDevice(devInfo);
    return true;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
bool UsbService::DelDevice(uint8_t busNum, uint8_t devAddr)
{
    USB_HILOGI(MODULE_USB_HOST, "entry");
    int32_t ret = Close(busNum, devAddr);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "Close device failed width ret = %{public}d", ret);
    }

    if (usbHostManager_ == nullptr || usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "invalid usbHostManager_ or usbRightManager_");
        return false;
    }

    std::string name = std::to_string(busNum) + "-" + std::to_string(devAddr);
    if (!usbRightManager_->RemoveDeviceAllRight(GetDeviceVidPidSerialNumber(name))) {
        USB_HILOGW(MODULE_USB_HOST, "remove right failed: %{public}s", name.c_str());
    }

    {
        std::lock_guard<std::mutex> guard(mutex_);
        for (auto it = deviceVidPidMap_.begin(); it != deviceVidPidMap_.end(); ++it) {
            if (it->first == name) {
                deviceVidPidMap_.erase(it);
                break;
            }
        }
    }

    return usbHostManager_->DelDevice(busNum, devAddr);
}
// LCOV_EXCL_STOP

int32_t UsbService::GetDevices(std::vector<UsbDevice> &deviceList)
{
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostManager_->GetDevices(deviceList);
}

// LCOV_EXCL_START
int32_t UsbService::GetDeviceInfo(uint8_t busNum, uint8_t devAddr, UsbDevice &dev)
{
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostManager_->GetDeviceInfo(busNum, devAddr, dev);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::GetDeviceInfoDescriptor(const UsbDev &uDev, std::vector<uint8_t> &descriptor, UsbDevice &dev)
{
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostManager_->GetDeviceInfoDescriptor(uDev, descriptor, dev);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::GetConfigDescriptor(UsbDevice &dev, std::vector<uint8_t> &descriptor)
{
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostManager_->GetConfigDescriptor(dev, descriptor);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::GetActiveConfig(uint8_t busNum, uint8_t devAddr, uint8_t &configIndex)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostManager_->GetActiveConfig(busNum, devAddr, configIndex);
}
// LCOV_EXCL_STOP

int32_t UsbService::GetRawDescriptor(uint8_t busNum, uint8_t devAddr, std::vector<uint8_t> &bufferData)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        ReportUsbOperationFaultSysEvent("GetRawDescriptor", UEC_SERVICE_PERMISSION_DENIED,
            "CheckDevicePermission failed");
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbHostManager_->GetRawDescriptor(busNum, devAddr, bufferData);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "error ret:%{public}d", ret);
    }
    return ret;
    // LCOV_EXCL_STOP
}

int32_t UsbService::GetFileDescriptor(uint8_t busNum, uint8_t devAddr, int32_t &fd)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbHostManager_->GetFileDescriptor(busNum, devAddr, fd);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "get fd error ret:%{public}d", ret);
    }
    return ret;
    // LCOV_EXCL_STOP
}

// LCOV_EXCL_START
int32_t UsbService::GetDeviceSpeed(uint8_t busNum, uint8_t devAddr, uint8_t &speed)
{
    if (PreCallFunction() != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "PreCallFunction failed");
        return UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED;
    }

    int32_t ret = usbHostManager_->GetDeviceSpeed(busNum, devAddr, speed);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "error ret:%{public}d", ret);
    }
    USB_HILOGE(MODULE_USB_HOST, "GetDeviceSpeedImpl:%{public}u", speed);
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::GetInterfaceActiveStatus(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid, bool &unactivated)
{
    if (PreCallFunction() != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "PreCallFunction failed");
        return UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED;
    }

    int32_t ret = usbHostManager_->GetInterfaceActiveStatus(busNum, devAddr, interfaceid, unactivated);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "error ret:%{public}d", ret);
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
bool UsbService::GetDeviceProductName(const std::string &deviceName, std::string &productName)
{
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "usbHostManager_ is nullptr");
        return false;
    }
    return usbHostManager_->GetProductName(deviceName, productName);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::BulkTransferRead(
    uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep, UsbBulkTransData &bufferData, int32_t timeOut)
{
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    HDI::Usb::V1_0::UsbDev devInfo = {busNum, devAddr};
    UsbPipe pipe = {ep.GetInterfaceId(), ep.GetAddress()};
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::ReportTransferFaultSysEvent("BulkRead", usbDev, pipe,
                UEC_SERVICE_PERMISSION_DENIED, "CheckDevicePermission failed");
        }
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    int32_t ret = usbHostManager_->BulkTransferRead(devInfo, pipe, bufferData.data_, timeOut);
    if (ret != UEC_OK) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::CheckAttributeReportTransferFaultSysEvent("BulkRead", usbDev, pipe, ep,
                ret, "BulkTransferReadFail");
        }
        USB_HILOGE(MODULE_USB_HOST, "BulkTransferRead error ret:%{public}d", ret);
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::BulkTransferReadwithLength(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep,
    int32_t length, UsbBulkTransData &bufferData, int32_t timeOut)
{
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    HDI::Usb::V1_0::UsbDev devInfo = {busNum, devAddr};
    UsbPipe pipe = {ep.GetInterfaceId(), ep.GetAddress()};
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::ReportTransferFaultSysEvent("BulkRead", usbDev, pipe,
                UEC_SERVICE_PERMISSION_DENIED, "CheckDevicePermission failed");
        }
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    int32_t ret = usbHostManager_->BulkTransferReadwithLength(devInfo, pipe, length, bufferData.data_, timeOut);
    if (ret != UEC_OK) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::CheckAttributeReportTransferFaultSysEvent("BulkRead", usbDev, pipe, ep,
                ret, "BulkTransferReadFail");
        }
        USB_HILOGE(MODULE_USB_HOST, "BulkTransferReadWithLength error ret:%{public}d", ret);
    }
    return ret;
}
// LCOV_EXCL_STOP

int32_t UsbService::BulkTransferWrite(
    uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep, const UsbBulkTransData &bufferData, int32_t timeOut)
{
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    HDI::Usb::V1_0::UsbDev dev = {busNum, devAddr};
    UsbPipe pipe = {ep.GetInterfaceId(), ep.GetAddress()};
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::ReportTransferFaultSysEvent("BulkWrite", usbDev, pipe,
                UEC_SERVICE_PERMISSION_DENIED, "CheckDevicePermission failed");
        }
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    int32_t ret = usbHostManager_->BulkTransferWrite(dev, pipe, bufferData.data_, timeOut);
    if (ret != UEC_OK) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::CheckAttributeReportTransferFaultSysEvent("BulkWrite", usbDev, pipe, ep,
                ret, "BulkTransferWriteFail");
        }
        USB_HILOGE(MODULE_USB_HOST, "BulkTransferWrite error ret:%{public}d", ret);
    }
    return ret;
}

int32_t UsbService::ControlTransfer(uint8_t busNum, uint8_t devAddr,
    const UsbCtlSetUp& ctrlParams, std::vector<uint8_t> &bufferData)
{
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    HDI::Usb::V1_0::UsbDev dev = {busNum, devAddr};
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::ReportTransferFaultSysEvent("ControlTransfer", usbDev, {0, 0},
                UEC_SERVICE_PERMISSION_DENIED, "CheckDevicePermission failed");
        }
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    HDI::Usb::V1_0::UsbCtrlTransfer ctrl;
    UsbCtrlTransferChange(ctrl, ctrlParams);
    int32_t ret = usbHostManager_->ControlTransfer(dev, ctrl, bufferData);
    if (ret != UEC_OK) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::ReportTransferFaultSysEvent("ControlTransfer", usbDev, {0, 0},
                ret, "ControlTransferFail");
        }
        USB_HILOGE(MODULE_USB_HOST, "ControlTransfer error ret:%{public}d", ret);
    }
    return ret;
}

// LCOV_EXCL_START
void UsbService::UsbCtrlTransferChange(HDI::Usb::V1_0::UsbCtrlTransfer &param, const UsbCtlSetUp &ctlSetup)
{
    param.requestType = ctlSetup.reqType;
    param.requestCmd = ctlSetup.reqCmd ;
    param.value = ctlSetup.value;
    param.index = ctlSetup.index;
    param.timeout = ctlSetup.timeout;
    return;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::UsbCtrlTransferChange(HDI::Usb::V1_2::UsbCtrlTransferParams &param, const UsbCtlSetUp &ctlSetup)
{
    param.requestType = ctlSetup.reqType;
    param.requestCmd = ctlSetup.reqCmd ;
    param.value = ctlSetup.value;
    param.index = ctlSetup.index;
    param.length = ctlSetup.length;
    param.timeout = ctlSetup.timeout;
    return;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::UsbControlTransfer(
    uint8_t busNum, uint8_t devAddr, const UsbCtlSetUp& ctrlParams, std::vector<uint8_t> &bufferData)
{
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    HDI::Usb::V1_0::UsbDev dev = {busNum, devAddr};
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::ReportTransferFaultSysEvent("ControlTransfer", usbDev, {0, 0},
                UEC_SERVICE_PERMISSION_DENIED, "CheckDevicePermission failed");
        }
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    HDI::Usb::V1_2::UsbCtrlTransferParams ctlSetUp;
    UsbCtrlTransferChange(ctlSetUp, ctrlParams);
    int32_t ret = usbHostManager_->UsbControlTransfer(dev, ctlSetUp, bufferData);
    if (ret != UEC_OK) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::ReportTransferFaultSysEvent("ControlTransfer", usbDev, {0, 0},
                ret, "UsbControlTransferFail");
        }
        USB_HILOGE(MODULE_USB_HOST, "UsbControlTransfer error ret:%{public}d", ret);
    }
    return ret;
}
// LCOV_EXCL_STOP

int32_t UsbService::RequestQueue(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep,
    const std::vector<uint8_t> &clientData, const std::vector<uint8_t> &bufferData)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    HDI::Usb::V1_0::UsbDev dev = {busNum, devAddr};
    UsbPipe pipe = {ep.GetInterfaceId(), ep.GetAddress()};
    int32_t ret = usbHostManager_->RequestQueue(dev, pipe, clientData, bufferData);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "error ret:%{public}d", ret);
    }
    return ret;
    // LCOV_EXCL_STOP
}

// LCOV_EXCL_START
int32_t UsbService::RequestWait(uint8_t busNum, uint8_t devAddr, int32_t timeOut,
    std::vector<uint8_t> &clientData, std::vector<uint8_t> &bufferData)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    HDI::Usb::V1_0::UsbDev dev = {busNum, devAddr};
    int32_t ret = usbHostManager_->RequestWait(dev, timeOut, clientData, bufferData);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "error ret:%{public}d", ret);
    }
    return ret;
}
// LCOV_EXCL_STOP

int32_t UsbService::RequestCancel(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId, uint8_t endpointId)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbHostManager_->RequestCancel(busNum, devAddr, interfaceId, endpointId);
    // LCOV_EXCL_STOP
}

// LCOV_EXCL_START
int32_t UsbService::RegBulkCallback(uint8_t busNum, uint8_t devAddr,
    const USBEndpoint &ep, const sptr<IRemoteObject> &cb)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    HDI::Usb::V1_0::UsbDev devInfo = {busNum, devAddr};
    UsbPipe pipe = {ep.GetInterfaceId(), ep.GetAddress()};
    int32_t ret = usbHostManager_->RegBulkCallback(devInfo, pipe, cb);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "RegBulkCallback error ret:%{public}d", ret);
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::UsbCancelTransfer(uint8_t busNum, uint8_t devAddr, int32_t endpoint)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbService UsbCancelTransfer enter");
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    HDI::Usb::V1_0::UsbDev devInfo = {busNum, devAddr};
    int32_t ret = usbHostManager_->UsbCancelTransfer(devInfo, endpoint);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "UsbCancelTransfer error ret:%{public}d", ret);
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::UsbTransInfoChange(HDI::Usb::V1_2::USBTransferInfo &info, const UsbTransInfo &param)
{
    info.endpoint = param.endpoint;
    info.flags = param.flags;
    info.type = param.type;
    info.timeOut = param.timeOut;
    info.length = param.length;
    info.userData = param.userData;
    info.numIsoPackets = param.numIsoPackets;
    return;
}
// LCOV_EXCL_STOP

void UsbService::GetTransferTypeString(const UsbTransInfo &transInfo, USBEndpoint &ep, std::string &transType)
{
    switch (transInfo.type) {
        case (ISO_TRANSFER_TYPE): transType = "IsochronousTransfer"; break;
        case (BULK_TRANSFER_TYPE): transType = "BulkTransfer"; break;
        case (INTP_TRANSFER_TYPE): transType = "InterruptTransfer"; break;
        default: transType = "Unknown";
    }
    if (ep.GetDirection() == USB_ENDPOINT_DIR_IN) {
        transType += "Read";
    } else if (ep.GetDirection() == USB_ENDPOINT_DIR_OUT) {
        transType += "Write";
    }
}

// LCOV_EXCL_START
int32_t UsbService::UsbSubmitTransfer(uint8_t busNum, uint8_t devAddr, const UsbTransInfo &param,
    const sptr<IRemoteObject> &cb, int32_t fd, int32_t memSize)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbService UsbSubmitTransfer enter");
    if (cb == nullptr || fd <= 0 || memSize <= 0 || memSize >= MEMSIZE_MAX) {
        ::close(fd);
        USB_HILOGE(MODULE_USB_HOST, "invalid param, fd=[%{public}d],memSize=[%{public}d]", fd, memSize);
        return UEC_SERVICE_INVALID_VALUE;
    }
    HDI::Usb::V1_2::USBTransferInfo info;
    UsbTransInfoChange(info, param);
    sptr<Ashmem> ashmem = new (std::nothrow) Ashmem(fd, memSize);
    if (ashmem == nullptr) {
        ::close(fd);
        USB_HILOGE(MODULE_USB_HOST, "UsbService UsbSubmitTransfer error ashmem");
        return UEC_SERVICE_INVALID_VALUE;
    }
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    HDI::Usb::V1_0::UsbDev devInfo = {busNum, devAddr};
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        UsbDevice usbDev;
        USBEndpoint ep;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev) &&
            usbHostManager_->GetEndpointFromId(usbDev, param.endpoint, ep)) {
            std::string transferType;
            GetTransferTypeString(param, ep, transferType);
            UsbReportSysEvent::ReportTransferFaultSysEvent(transferType.c_str(), usbDev,
                {ep.GetInterfaceId(), param.endpoint}, UEC_SERVICE_PERMISSION_DENIED, "CheckDevicePermission failed");
        }
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    int32_t ret = usbHostManager_->UsbSubmitTransfer(devInfo, info, cb, ashmem);
    if (ret != UEC_OK) {
        UsbDevice usbDev;
        USBEndpoint ep;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev) &&
            usbHostManager_->GetEndpointFromId(usbDev, param.endpoint, ep)) {
            std::string transferType;
            GetTransferTypeString(param, ep, transferType);
            UsbReportSysEvent::ReportTransferFaultSysEvent(transferType.c_str(), usbDev,
                {ep.GetInterfaceId(), param.endpoint}, ret, "UsbSubmitTransferFail");
        }
        USB_HILOGE(MODULE_USB_HOST, "UsbSubmitTransfer error ret:%{public}d", ret);
    }
    return ret;
}
// LCOV_EXCL_STOP

int32_t UsbService::UnRegBulkCallback(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    HDI::Usb::V1_0::UsbDev devInfo = {busNum, devAddr};
    UsbPipe pipe = {ep.GetInterfaceId(), ep.GetAddress()};
    int32_t ret = usbHostManager_->UnRegBulkCallback(devInfo, pipe);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "UnRegBulkCallback error ret:%{public}d", ret);
    }
    return ret;
    // LCOV_EXCL_STOP
}

int32_t UsbService::BulkRead(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep, int32_t fd, int32_t memSize)
{
    if (usbHostManager_ == nullptr || fd <= 0 || memSize <= 0) {
        ::close(fd);
        USB_HILOGE(MODULE_USB_HOST, "invalid param, fd=[%{public}d],memSize=[%{public}d]", fd, memSize);
        return UEC_SERVICE_INVALID_VALUE;
    }

    HDI::Usb::V1_0::UsbDev devInfo = {busNum, devAddr};
    UsbPipe pipe = {ep.GetInterfaceId(), ep.GetAddress()};
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        ::close(fd);
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::ReportTransferFaultSysEvent("BulkRead", usbDev, pipe,
                UEC_SERVICE_PERMISSION_DENIED, "CheckDevicePermission failed");
        }
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    sptr<Ashmem> ashmem = new (std::nothrow) Ashmem(fd, memSize);
    if (ashmem == nullptr) {
        ::close(fd);
        USB_HILOGE(MODULE_USB_HOST, "UsbService BulkRead error ashmem");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbHostManager_->BulkRead(devInfo, pipe, ashmem);
    if (ret != UEC_OK) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::CheckAttributeReportTransferFaultSysEvent("BulkRead", usbDev, pipe, ep,
                ret, "BulkReadFail");
        }
        USB_HILOGE(MODULE_USB_HOST, "BulkRead error ret:%{public}d", ret);
    }
    return ret;
}

int32_t UsbService::BulkWrite(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep, int32_t fd, int32_t memSize)
{
    if (usbHostManager_ == nullptr || fd <= 0 || memSize <= 0) {
        ::close(fd);
        USB_HILOGE(MODULE_USB_HOST, "invalid param, fd=[%{public}d],memSize=[%{public}d]", fd, memSize);
        return UEC_SERVICE_INVALID_VALUE;
    }

    HDI::Usb::V1_0::UsbDev devInfo = {busNum, devAddr};
    UsbPipe pipe = {ep.GetInterfaceId(), ep.GetAddress()};
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        ::close(fd);
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::ReportTransferFaultSysEvent("BulkWrite", usbDev, pipe,
                UEC_SERVICE_PERMISSION_DENIED, "CheckDevicePermission failed");
        }
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    sptr<Ashmem> ashmem = new (std::nothrow) Ashmem(fd, memSize);
    if (ashmem == nullptr) {
        ::close(fd);
        USB_HILOGE(MODULE_USB_HOST, "UsbService BulkWrite error ashmem");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbHostManager_->BulkWrite(devInfo, pipe, ashmem);
    if (ret != UEC_OK) {
        UsbDevice usbDev;
        if (usbHostManager_->GetTargetDevice(busNum, devAddr, usbDev)) {
            UsbReportSysEvent::CheckAttributeReportTransferFaultSysEvent("BulkWrite", usbDev, pipe, ep,
                ret, "BulkWriteFail");
        }
        USB_HILOGE(MODULE_USB_HOST, "BulkWrite error ret:%{public}d", ret);
    }
    return ret;
}

int32_t UsbService::BulkCancel(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep)
{
    if (!UsbService::CheckDevicePermission(busNum, devAddr)) {
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    // LCOV_EXCL_START
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "UsbService::usbHostManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    HDI::Usb::V1_0::UsbDev devInfo = {busNum, devAddr};
    UsbPipe pipe = {ep.GetInterfaceId(), ep.GetAddress()};
    int32_t ret = usbHostManager_->BulkCancel(devInfo, pipe);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "BulkCancel error ret:%{public}d", ret);
    }
    return ret;
    // LCOV_EXCL_STOP
}

bool UsbService::CheckDevicePermission(uint8_t busNum, uint8_t devAddr)
{
    std::string name = std::to_string(busNum) + "-" + std::to_string(devAddr);
    if (!UsbService::HasRight(name)) {
        USB_HILOGE(MODULE_USB_HOST, "No permission");
        return false;
    }
    return true;
}

// LCOV_EXCL_START
int32_t UsbService::HasRight(const std::string &deviceName, bool &hasRight)
{
    hasRight = this->HasRight(deviceName);
    return UEC_OK;
}
// LCOV_EXCL_STOP

bool UsbService::HasRight(const std::string &deviceName)
{
    USB_HILOGI(MODULE_USB_HOST, "calling usbRightManager HasRight");
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "invalid usbRightManager_");
        return false;
    }

    std::string deviceVidPidSerialNum = "";
    if (GetDeviceVidPidSerialNumber(deviceName, deviceVidPidSerialNum) != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "can not find deviceName.");
        return false;
    }

    // LCOV_EXCL_START
    if (usbRightManager_->IsSystemAppOrSa()) {
        USB_HILOGW(MODULE_USB_HOST, "system app, bypass: dev=%{public}s", deviceName.c_str());
        return true;
    }

    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_HOST, "HasRight GetCallingInfo false");
        return false;
    }

    USB_HILOGI(MODULE_USB_HOST, "bundle=%{public}s, device=%{public}s", bundleName.c_str(), deviceName.c_str());
    return usbRightManager_->HasRight(deviceVidPidSerialNum, bundleName, tokenId, userId);
    // LCOV_EXCL_STOP
}

int32_t UsbService::RequestRight(const std::string &deviceName)
{
    USB_HILOGI(MODULE_USB_HOST, "calling usbRightManager RequestRight");
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "invalid usbRightManager_");
        return UEC_SERVICE_INNER_ERR;
    }
    std::string deviceVidPidSerialNum = "";
    int32_t ret = GetDeviceVidPidSerialNumber(deviceName, deviceVidPidSerialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "can not find deviceName.");
        return ret;
    }
    // LCOV_EXCL_START
    if (usbRightManager_->IsSystemAppOrSa()) {
        USB_HILOGW(MODULE_USB_HOST, "system app, bypass: dev=%{public}s", deviceName.c_str());
        return UEC_OK;
    }
    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_HOST, "GetCallingInfo false");
        return false;
    }

    USB_HILOGI(MODULE_USB_HOST, "bundle=%{public}s, device=%{public}s", bundleName.c_str(), deviceName.c_str());
    return usbRightManager_->RequestRight(deviceName, deviceVidPidSerialNum, bundleName, tokenId, userId);
    // LCOV_EXCL_STOP
}

int32_t UsbService::RemoveRight(const std::string &deviceName)
{
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "invalid usbRightManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    std::string deviceVidPidSerialNum = "";
    int32_t ret = GetDeviceVidPidSerialNumber(deviceName, deviceVidPidSerialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "can not find deviceName.");
        return ret;
    }
    if (usbRightManager_->IsSystemAppOrSa()) {
        USB_HILOGW(MODULE_USB_HOST, "system app, bypass: dev=%{public}s", deviceName.c_str());
        return UEC_OK;
    }
    // LCOV_EXCL_START
    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_HOST, "GetCallingInfo false");
        return false;
    }

    if (!usbRightManager_->RemoveDeviceRight(deviceVidPidSerialNum, bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_SERVICE, "RemoveDeviceRight failed");
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    USB_HILOGI(MODULE_USB_HOST, "RemoveRight done");
    return UEC_OK;
    // LCOV_EXCL_STOP
}


// LCOV_EXCL_START
int32_t UsbService::AddAccessRight(const std::string &tokenId, const std::string &deviceName)
{
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "invalid usbRightManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = CheckSysApiPermission();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "%{public}s: CheckSysApiPermission failed ret = %{public}d", __func__, ret);
        return ret;
    }
    std::string deviceVidPidSerialNum = "";
    ret = GetDeviceVidPidSerialNumber(deviceName, deviceVidPidSerialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "can not find deviceName.");
        return ret;
    }
    USB_HILOGI(MODULE_USB_HOST, "AddRight deviceName = %{public}s", deviceName.c_str());
    if (!usbRightManager_->AddDeviceRight(deviceVidPidSerialNum, tokenId)) {
        USB_HILOGE(MODULE_USB_HOST, "AddDeviceRight failed");
        return UEC_SERVICE_INNER_ERR;
    }
    USB_HILOGI(MODULE_USB_HOST, "AddRight done");
    return UEC_OK;
}
// LCOV_EXCL_STOP

void UsbService::UsbDeviceTypeChange(std::vector<UsbDeviceType> &disableType,
    const std::vector<UsbDeviceTypeInfo> &deviceTypes)
{
    for (size_t i = 0; i < deviceTypes.size(); i++) {
        UsbDeviceType info;
        info.baseClass = deviceTypes[i].baseClass;
        info.subClass = deviceTypes[i].subClass;
        info.protocol = deviceTypes[i].protocol;
        info.isDeviceType = deviceTypes[i].isDeviceType;
        info.isDeviceTypeAllMatch = deviceTypes[i].isDeviceTypeAllMatch;
        disableType.push_back(info);
    }
    return;
}

// LCOV_EXCL_START
std::string UsbService::GetDeviceVidPidSerialNumber(const std::string &deviceName)
{
    std::string strDesc = "test";
    std::lock_guard<std::mutex> guard(mutex_);
    for (auto it = deviceVidPidMap_.begin(); it != deviceVidPidMap_.end(); ++it) {
        USB_HILOGI(MODULE_USB_HOST, " it->first = %{public}s", it->first.c_str());
        if (it->first == deviceName) {
            strDesc = it->second;
            break;
        }
    }
    return strDesc;
}
// LCOV_EXCL_STOP

int32_t UsbService::GetDeviceVidPidSerialNumber(const std::string &deviceName, std::string& strDesc)
{
    int32_t isMatched = UEC_INTERFACE_INVALID_VALUE;
    std::lock_guard<std::mutex> guard(mutex_);
    for (auto it = deviceVidPidMap_.begin(); it != deviceVidPidMap_.end(); ++it) {
        USB_HILOGI(MODULE_USB_HOST, " it->first = %{public}s", it->first.c_str());
        if (it->first == deviceName) {
            strDesc = it->second;
            isMatched = UEC_OK;
            break;
        }
    }
    return isMatched;
}

// LCOV_EXCL_START
int32_t UsbService::PreCallFunction()
{
#ifdef USB_MANAGER_PASS_THROUGH
    sptr<HDI::Usb::V2_0::IUsbHostInterface> usbHostInterface_ =
        HDI::Usb::V2_0::IUsbHostInterface::Get("usb_host_interface_service", true);
    if (usbHostInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "usbHostInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
#else
    if (usbd_ == nullptr) {
        usbd_ = OHOS::HDI::Usb::V1_2::IUsbInterface::Get();
        USB_HILOGI(MODULE_USB_HOST, "%{public}s:%{public}d usbd_ == nullptr: %{public}d",
            __func__, __LINE__, usbd_ == nullptr);
    } else {
        USB_HILOGW(MODULE_USB_HOST, "%{public}s:usbd_ != nullptr", __func__);
    }
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
#endif // USB_MANAGER_PASS_THROUGH
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "invalid usbRightManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = CheckSysApiPermission();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "%{public}s: CheckSysApiPermission failed ret = %{public}d", __func__, ret);
        return ret;
    }
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "invalid usbHostManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }

    return UEC_OK;
}
// LCOV_EXCL_STOP
#endif // USB_MANAGER_FEATURE_HOST

#ifdef USB_MANAGER_FEATURE_DEVICE
void UsbService::SetPhyConnect(bool phyConnect)
{
    if (usbDeviceManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbService::usbDeviceManager_ is nullptr");
        return;
    }
    usbDeviceManager_->SetPhyConnectState(phyConnect);
    USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: physic connect state is %{public}d", __func__, phyConnect);
    return;
}

int32_t UsbService::GetCurrentFunctions(int32_t &functions)
{
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbRightManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = CheckSysApiPermission();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: CheckSysApiPermission failed ret = %{public}d", __func__, ret);
        ReportUsbOperationFaultSysEvent("FUNCTION_CHANGED", ret, "CheckSysApiPermission failed");
        return ret;
    }
    // LCOV_EXCL_START
    if (usbDeviceManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbService::usbDeviceManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbDeviceManager_->GetCurrentFunctions(functions);
    // LCOV_EXCL_STOP
}

int32_t UsbService::SetCurrentFunctions(int32_t functions)
{
    USB_HILOGI(MODULE_USB_DEVICE, "func = %{public}d", functions);
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbRightManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = CheckSysApiPermission();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: CheckSysApiPermission failed ret = %{public}d", __func__, ret);
        ReportUsbOperationFaultSysEvent("FUNCTION_CHANGED", ret, "CheckSysApiPermission failed");
        return ret;
    }
    ret = usbRightManager_->HasSetFuncRight(functions);
    if (ret != 0) {
        USB_HILOGE(MODULE_USB_DEVICE, "HasSetFuncRight fail");
        ReportUsbOperationFaultSysEvent("FUNCTION_CHANGED", ret, "HasSetFuncRight fail");
        return ret;
    }
    if (usbDeviceManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbService::usbDeviceManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    if (!SetSettingsDataHdcStatus(functions)) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: SetHdcStatus failed, function is: %{public}d", __func__, functions);
    }
    return usbDeviceManager_->SetCurrentFunctions(functions);
}

int32_t UsbService::UsbFunctionsFromString(const std::string &funcs, int32_t &funcResult)
{
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbRightManager_");
        funcResult = UEC_SERVICE_INVALID_VALUE;
        return UEC_OK;
    }
    int32_t ret = CheckSysApiPermission();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: CheckSysApiPermission failed ret = %{public}d", __func__, ret);
        funcResult = ret;
        return UEC_OK;
    }
    USB_HILOGI(MODULE_USB_DEVICE, "calling UsbFunctionsFromString");
    funcResult = static_cast<int32_t>(UsbDeviceManager::ConvertFromString(funcs));
    return UEC_OK;
}

int32_t UsbService::UsbFunctionsToString(int32_t funcs, std::string& funcResult)
{
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbRightManager_");
        funcResult = "";
        return UEC_OK;
    }
    if (!usbRightManager_->IsSystemAppOrSa()) {
        funcResult = PERMISSION_DENIED_SYSAPI;
        return UEC_OK;
    }
    if (!usbRightManager_->VerifyPermission()) {
        int32_t apiVersion = GetHapApiVersion();
        if (apiVersion < API_VERSION_ID_18) {
            funcResult = PERMISSION_DENIED_SYSAPI;
            return UEC_OK;
        }
        funcResult = SYS_APP_PERMISSION_DENIED_SYSAPI;
        return UEC_OK;
    }

    USB_HILOGI(MODULE_USB_DEVICE, "calling UsbFunctionsToString");
    funcResult = UsbDeviceManager::ConvertToString(funcs);
    return UEC_OK;
}

// LCOV_EXCL_START
void UsbService::UpdateDeviceState(int32_t status)
{
    if (usbDeviceManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbDeviceManager_");
        return;
    }

    usbDeviceManager_->HandleEvent(status);
    usbDeviceManager_->ProcessCustomControlRequestUevent(status);
    if (usbAccessoryManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbAccessoryManager_");
        return;
    }
    usbAccessoryManager_->HandleEvent(status);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::UserChangeProcess()
{
    if (usbDeviceManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "usbDeviceManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
#ifdef USB_MANAGER_FEATURE_PORT
    if (usbPortManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "usbPortManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    bool res = usbPortManager_->IsReverseCharge();
    usbDeviceManager_->SetChargeFlag(res);
#endif // USB_MANAGER_FEATURE_PORT
    return usbDeviceManager_->UserChangeProcess();
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::GetAccessoryList(std::vector<USBAccessory> &accessList)
{
    if (usbAccessoryManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbAccessoryManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_DEVICE, "GetCallingInfo false");
        ReportUsbOperationFaultSysEvent("GetAccessoryList ", UEC_SERVICE_INNER_ERR, "GetCallingInfo false");
        return UEC_SERVICE_INNER_ERR;
    }

    usbAccessoryManager_->GetAccessoryList(bundleName, accessList);
    USB_HILOGD(MODULE_USB_DEVICE, "get accessory list size %{public}zu", accessList.size());
    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::OpenAccessory(const USBAccessory &access, int32_t &fd, const sptr<IRemoteObject> &accessoryRemote)
{
    if (usbAccessoryManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbAccessoryManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_DEVICE, "GetCallingInfo false");
        ReportUsbOperationFaultSysEvent("OpenAccessory", UEC_SERVICE_GET_TOKEN_INFO_FAILED, "GetCallingInfo false");
        return UEC_SERVICE_GET_TOKEN_INFO_FAILED;
    }

    std::string serialNum = "";
    int32_t ret = usbAccessoryManager_->GetAccessorySerialNumber(access, bundleName, serialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "can not find accessory.");
        ReportUsbOperationFaultSysEvent("OpenAccessory", ret, "can not find accessory");
        return ret;
    }

    bool result = false;
    ret = UsbService::HasAccessoryRight(access, result);
    if (ret != UEC_OK || !result) {
        USB_HILOGE(MODULE_USB_DEVICE, "No permission");
        ReportUsbOperationFaultSysEvent("OpenAccessory", UEC_SERVICE_PERMISSION_DENIED, "No permission");
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    accessoryRecipient_ = new AccessoryDeathRecipient(fd);
    if (accessoryRecipient_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbService::OpenAccessory accessoryRecipient_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    if (!accessoryRemote->AddDeathRecipient(accessoryRecipient_)) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbService::OpenAccessory add DeathRecipient failed");
        CloseAccessory(fd);
        return UEC_SERVICE_INVALID_VALUE;
    }

    ret = usbAccessoryManager_->OpenAccessory(fd);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "error ret:%{public}d", ret);
        accessoryRemote->RemoveDeathRecipient(accessoryRecipient_);
        accessoryRecipient_ = nullptr;
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::CloseAccessory(int32_t fd, const sptr<IRemoteObject> &accessoryRemote)
{
    if (usbAccessoryManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbAccessoryManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbAccessoryManager_->CloseAccessory(fd);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "error ret:%{public}d", ret);
    } else {
        accessoryRemote->RemoveDeathRecipient(accessoryRecipient_);
        accessoryRecipient_ = nullptr;
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::AddAccessoryRight(const uint32_t tokenId, const USBAccessory &access)
{
    if (usbRightManager_ == nullptr || usbAccessoryManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbRightManager_ or usbAccessoryManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = CheckSysApiPermission();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: CheckSysApiPermission failed ret = %{public}d", __func__, ret);
        return ret;
    }

    HapTokenInfo hapTokenInfoRes;
    ret = AccessTokenKit::GetHapTokenInfo((AccessTokenID) tokenId, hapTokenInfoRes);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "GetHapTokenInfo failed:ret:%{public}d", ret);
        return UEC_SERVICE_GET_TOKEN_INFO_FAILED;
    }

    std::string serialNum = "";
    ret = usbAccessoryManager_->GetAccessorySerialNumber(access, hapTokenInfoRes.bundleName, serialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "can not find accessory.");
        return ret;
    }

    USB_HILOGI(MODULE_USB_DEVICE, "Add accessory Right, deviceName = %{public}s", serialNum.c_str());
    if (!usbRightManager_->AddDeviceRight(serialNum, std::to_string(tokenId))) {
        USB_HILOGE(MODULE_USB_DEVICE, "AddDeviceRight failed");
        return UEC_SERVICE_DATABASE_OPERATION_FAILED;
    }
    USB_HILOGI(MODULE_USB_DEVICE, "AddAccessoryRight done");
    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::HasAccessoryRight(const USBAccessory &access, bool &result)
{
    USB_HILOGI(MODULE_USB_DEVICE, "calling HasAccessoryRight");
    if (usbRightManager_ == nullptr || usbAccessoryManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbRightManager_ or usbAccessoryManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }

    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_DEVICE, "HasRight GetCallingInfo false");
        return UEC_SERVICE_GET_TOKEN_INFO_FAILED;
    }

    std::string serialNum = "";
    int32_t ret = usbAccessoryManager_->GetAccessorySerialNumber(access, bundleName, serialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "can not find accessory.");
        return ret;
    }

    USB_HILOGI(MODULE_USB_DEVICE, "bundle=%{public}s, serialNum=%{public}s", bundleName.c_str(), serialNum.c_str());
    result = usbRightManager_->HasRight(serialNum, bundleName, tokenId, userId);

    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::RequestAccessoryRight(const USBAccessory &access, bool &result)
{
    USB_HILOGI(MODULE_USB_DEVICE, "calling usbRightManager RequestAccessoryRight");
    if (usbRightManager_ == nullptr || usbAccessoryManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbRightManager_ or usbAccessoryManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }

    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_DEVICE, "GetCallingInfo false");
        return UEC_SERVICE_GET_TOKEN_INFO_FAILED;
    }

    std::string serialNum = "";
    int32_t ret = usbAccessoryManager_->GetAccessorySerialNumber(access, bundleName, serialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "can not find accessory.");
        return ret;
    }

    USB_HILOGI(MODULE_USB_DEVICE, "bundle=%{public}s, device=%{public}s", bundleName.c_str(), serialNum.c_str());
    return usbRightManager_->RequestRight(access, serialNum, bundleName, tokenId, userId, result);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::CancelAccessoryRight(const USBAccessory &access)
{
    USB_HILOGI(MODULE_USB_DEVICE, "calling CancelAccessoryRight");
    if (usbRightManager_ == nullptr || usbAccessoryManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "invalid usbRightManager_ or usbAccessoryManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }

    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_DEVICE, "GetCallingInfo false");
        return UEC_SERVICE_GET_TOKEN_INFO_FAILED;
    }

    std::string serialNum = "";
    int32_t ret = usbAccessoryManager_->GetAccessorySerialNumber(access, bundleName, serialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "can not find accessory.");
        return ret;
    }

    if (usbRightManager_->CancelDeviceRight(serialNum, bundleName, tokenId, userId) != UEC_OK) {
        USB_HILOGI(MODULE_USB_DEVICE, "CancelAccessoryRight failed");
        return UEC_SERVICE_DATABASE_OPERATION_FAILED;
    }

    USB_HILOGI(MODULE_USB_DEVICE, "CancelAccessoryRight done");
    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
bool UsbService::InitSettingsDataHdcStatus()
{
    auto datashareHelper = std::make_shared<UsbSettingDataShare>();
    if (datashareHelper->CreateDataShareHelper(USB_SYSTEM_ABILITY_ID) == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: datashare is not ready", __func__);
        return false;
    }
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: invalid usbRightManager_", __func__);
        return false;
    }
    if (usbDeviceManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: UsbService::usbDeviceManager_ is nullptr", __func__);
        return false;
    }
    
    int32_t func = 0;
    if (usbDeviceManager_->GetCurrentFunctions(func) != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: function is get failed!", __func__);
    } else if (!SetSettingsDataHdcStatus(func)) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: HDC_STATUS is set failed, func is: %{public}d", __func__, func);
    }
    return true;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
bool UsbService::SetSettingsDataHdcStatus(int32_t func)
{
    uint32_t func_uint = static_cast<uint32_t>(func);
    auto datashareHelper = std::make_shared<UsbSettingDataShare>();
    std::string hdcStatus {"false"};
    OHOS::Uri uri(
        "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true&key=HDC_STATUS");
    if (func_uint & USB_FUNCTION_HDC) {
        USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: func is = %{public}d (USB_FUNCTION_HDC)", __func__, func_uint);
        if (datashareHelper->Query(uri, "HDC_STATUS", hdcStatus) && hdcStatus == "true") {
            USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: HDC_STATUS is already true!", __func__);
            return true;
        }
        hdcStatus = "true";
        if (!datashareHelper->Update(uri, "HDC_STATUS", hdcStatus)) {
            USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: HDC_STATUS is update failed!", __func__);
            return false;
        }
        return true;
    } else {
        USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: func is = %{public}d", __func__, func_uint);
        if (datashareHelper->Query(uri, "HDC_STATUS", hdcStatus) && hdcStatus == "false") {
            USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: HDC_STATUS is already false!", __func__);
            return false;
        }
        hdcStatus = "false";
        if (!datashareHelper->Update(uri, "HDC_STATUS", hdcStatus)) {
            USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: HDC_STATUS is update failed!", __func__);
            return false;
        }
        return true;
    }
}
// LCOV_EXCL_STOP
#endif // USB_MANAGER_FEATURE_DEVICE

#ifdef USB_MANAGER_FEATURE_PORT
int32_t UsbService::GetPorts(std::vector<UsbPort> &ports)
{
    USB_HILOGI(MODULE_USB_PORT, "calling usbPortManager getPorts");
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "invalid usbRightManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = CheckSysApiPermission();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_PORT, "%{public}s: CheckSysApiPermission failed ret = %{public}d", __func__, ret);
        ReportUsbOperationFaultSysEvent("GetPorts", ret, "CheckSysApiPermission failed");
        return ret;
    }
    if (usbPortManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "invalid usbPortManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbPortManager_->GetPorts(ports);
}

// LCOV_EXCL_START
int32_t UsbService::GetSupportedModes(int32_t portId, int32_t &supportedModes)
{
    USB_HILOGI(MODULE_USB_PORT, "calling usbPortManager getSupportedModes");
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "invalid usbRightManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = CheckSysApiPermission();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_PORT, "%{public}s: CheckSysApiPermission failed ret = %{public}d", __func__, ret);
        return ret;
    }
    if (usbPortManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "invalid usbPortManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbPortManager_->GetSupportedModes(portId, supportedModes);
}
// LCOV_EXCL_STOP

int32_t UsbService::SetPortRole(int32_t portId, int32_t powerRole, int32_t dataRole)
{
    USB_HILOGI(MODULE_USB_PORT, "calling usbPortManager SetPortRole");
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "invalid usbRightManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = CheckSysApiPermission();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_PORT, "%{public}s: CheckSysApiPermission failed ret = %{public}d", __func__, ret);
        ReportUsbOperationFaultSysEvent("SetPortRole", ret, "CheckSysApiPermission failed");
        return ret;
    }
    if (usbPortManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "UsbService::usbPortManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    ret = usbPortManager_->SetPortRole(portId, powerRole, dataRole);
    if (ret == HDF_ERR_NOT_SUPPORT) {
        USB_HILOGE(MODULE_USB_PORT, "SetPortRole not support");
        return UEC_SERVICE_NOT_SUPPORT_SWITCH_PORT;
    }
    return ret;
}

// LCOV_EXCL_START
void UsbService::UpdateUsbPort(int32_t portId, int32_t powerRole, int32_t dataRole, int32_t mode)
{
    if (usbPortManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "invalid usbPortManager_");
        return;
    }

    usbPortManager_->UpdatePort(portId, powerRole, dataRole, mode);
}

void UsbService::UpdateUsbPort(int32_t portId, int32_t powerRole,
    int32_t dataRole, int32_t mode, int32_t supportedModes)
{
    if (usbPortManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "invalid usbPortManager_");
        return;
    }

    usbPortManager_->UpdatePort(portId, powerRole, dataRole, mode, supportedModes);
}
// LCOV_EXCL_STOP
#endif // USB_MANAGER_FEATURE_PORT

#if defined(USB_MANAGER_FEATURE_HOST) || defined(USB_MANAGER_FEATURE_DEVICE)
// LCOV_EXCL_START
bool UsbService::GetCallingInfo(std::string &bundleName, std::string &tokenId, int32_t &userId)
{
    OHOS::Security::AccessToken::AccessTokenID token = IPCSkeleton::GetCallingTokenID();
    OHOS::Security::AccessToken::HapTokenInfo hapTokenInfoRes;
    int32_t ret = OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenInfo(token, hapTokenInfoRes);
    if (ret != ERR_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "failed, ret: %{public}d, app: %{public}s",
            ret, bundleName.c_str());
        return false;
    }
    bundleName = hapTokenInfoRes.bundleName;
    tokenId = std::to_string((uint32_t)token);
    userId = hapTokenInfoRes.userID;
    USB_HILOGD(MODULE_USB_SERVICE, "ret: %{public}d, app: %{public}s", ret, bundleName.c_str());
    return true;
}
// LCOV_EXCL_STOP
#endif // USB_MANAGER_FEATURE_HOST || USB_MANAGER_FEATURE_DEVICE

int32_t UsbService::CheckSysApiPermission()
{
    if (!usbRightManager_->IsSystemAppOrSa()) {
        return UEC_SERVICE_PERMISSION_DENIED_SYSAPI;
    }

    // LCOV_EXCL_START
    if (!usbRightManager_->VerifyPermission()) {
        int32_t apiVersion = GetHapApiVersion();
        if (apiVersion < API_VERSION_ID_18) {
            return UEC_SERVICE_PERMISSION_DENIED_SYSAPI;
        }
        return UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED;
    }

    return UEC_OK;
    // LCOV_EXCL_STOP
}

// LCOV_EXCL_START
int32_t UsbService::GetHapApiVersion()
{
    OHOS::Security::AccessToken::AccessTokenID token = IPCSkeleton::GetCallingTokenID();
    OHOS::Security::AccessToken::HapTokenInfo hapTokenInfoRes;
    int32_t ret = OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenInfo(token, hapTokenInfoRes);
    if (ret != ERR_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "GetHapTokenInfo failed, ret: %{public}d", ret);
        return API_VERSION_ID_18;
    }
    int32_t hapApiVersion = hapTokenInfoRes.apiVersion;
    USB_HILOGD(MODULE_USB_SERVICE, "API check hapApiVersion = %{public}d", hapApiVersion);

    return hapApiVersion;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::InitUsbRight()
{
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "invalid usbRightManager_");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbRightManager_->Init();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "Init usb right manager failed: %{public}d", ret);
        return ret;
    }
    std::vector<std::string> devices;
    std::lock_guard<std::mutex> guard(mutex_);
    for (auto it = deviceVidPidMap_.begin(); it != deviceVidPidMap_.end(); ++it) {
        devices.push_back(it->second);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "clean: %{public}zu/%{public}zu", devices.size(), deviceVidPidMap_.size());
    ret = usbRightManager_->CleanUpRightExpired(devices);
    if (ret != USB_RIGHT_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "clean expired usb right failed: %{public}d", ret);
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
bool UsbService::GetBundleName(std::string &bundleName)
{
#ifdef USB_RIGHT_TEST
    bundleName = "com.usb.right";
    return true;
#endif // USB_RIGHT_TEST
    pid_t uid = GetCallingUid();
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        return false;
    }
    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == nullptr) {
        return false;
    }

    sptr<AppExecFwk::IBundleMgr> bundleMgr(new AppExecFwk::BundleMgrProxy(remoteObject));
    if (bundleMgr == nullptr) {
        return false;
    }

    ErrCode ret = bundleMgr->GetNameForUid(uid, bundleName);
    if (ret != ERR_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "failed to obtain bundleName");
        return false;
    }
    return true;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int UsbService::DoDump(int fd, const std::vector<std::string> &argList)
{
#ifdef USB_MANAGER_FEATURE_HOST
    if (argList[0] == USB_HOST) {
        if (usbHostManager_ == nullptr) {
            USB_HILOGE(MODULE_USB_SERVICE, "usbHostManager_ is nullptr");
            return UEC_SERVICE_INVALID_VALUE;
        }
        if (argList.size() < ARGLIST_SIZE_MIN) {
            USB_HILOGE(MODULE_USB_SERVICE, "argList size is less than 2");
            return UEC_SERVICE_INVALID_VALUE;
        }
        usbHostManager_->Dump(fd, argList[1]);
        return UEC_OK;
    }
#endif // USB_MANAGER_FEATURE_HOST
#ifdef USB_MANAGER_FEATURE_DEVICE
    if (argList[0] == USB_DEVICE) {
        if (usbDeviceManager_ == nullptr) {
            USB_HILOGE(MODULE_USB_SERVICE, "usbDeviceManager_ is nullptr");
            return UEC_SERVICE_INVALID_VALUE;
        }
        usbDeviceManager_->Dump(fd, argList);
        return UEC_OK;
    }
#endif // USB_MANAGER_FEATURE_DEVICE
#ifdef USB_MANAGER_FEATURE_PORT
    if (argList[0] == USB_PORT) {
        if (usbPortManager_ == nullptr) {
            USB_HILOGE(MODULE_USB_SERVICE, "usbPortManager_ is nullptr");
            return UEC_SERVICE_INVALID_VALUE;
        }
        usbPortManager_->Dump(fd, argList);
        return UEC_OK;
    }
#endif // USB_MANAGER_FEATURE_PORT
    if (argList[0] == USB_HELP) {
        DumpHelp(fd);
        usbSerialManager_->ListGetDumpHelp(fd);
    } else if (argList[0] == USB_LIST) {
        usbSerialManager_->SerialPortListDump(fd, argList);
    } else if (argList[0] == USB_GETT) {
        usbSerialManager_->SerialGetAttributeDump(fd, argList);
    } else {
        dprintf(fd, "Usb Dump service:invalid parameter.\n");
        DumpHelp(fd);
        usbSerialManager_->ListGetDumpHelp(fd);
    }
    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int UsbService::Dump(int fd, const std::vector<std::u16string> &args)
{
    if (fd < 0) {
        USB_HILOGE(MODULE_USB_SERVICE, "fd is invalid fd:%{public}d", fd);
        return UEC_SERVICE_INVALID_VALUE;
    }

    std::vector<std::string> argList;
    std::transform(args.begin(), args.end(), std::back_inserter(argList), [](const std::u16string &arg) {
        return Str16ToStr8(arg);
    });

    std::unique_lock lock(serialManagerMutex_);
    if (usbSerialManager_ == nullptr) {
        USB_HILOGI(MODULE_USB_SERVICE, "usbSerialManager_ is nullptr");
        usbSerialManager_ = std::make_shared<SERIAL::SerialManager>();
        if (usbSerialManager_ == nullptr) {
            USB_HILOGE(MODULE_USB_SERVICE, "usbSerialManager_ is still nullptr");
            return UEC_SERVICE_INVALID_VALUE;
        }
#ifdef USB_MANAGER_FEATURE_HOST
        usbHostManager_->SetSerialManager(usbSerialManager_);
#endif // USB_MANAGER_FEATURE_HOST
    }
    lock.unlock();
    if (argList.empty()) {
        USB_HILOGE(MODULE_USB_SERVICE, "argList is empty");
        DumpHelp(fd);
        usbSerialManager_->ListGetDumpHelp(fd);
        return UEC_SERVICE_INVALID_VALUE;
    }
    return DoDump(fd, argList);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::DumpHelp(int32_t fd)
{
    dprintf(fd, "Refer to the following usage:\n");
    dprintf(fd, "-h: dump help\n");
    dprintf(fd, "============= dump the all device ==============\n");
    dprintf(fd, "usb_host -a: dump the all device list info\n");
    dprintf(fd, "------------------------------------------------\n");
#ifdef USB_MANAGER_FEATURE_DEVICE
    if (usbDeviceManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "usbDeviceManager_ is nullptr");
        return;
    }
    usbDeviceManager_->GetDumpHelp(fd);
#endif // USB_MANAGER_FEATURE_DEVICE
#ifdef USB_MANAGER_FEATURE_PORT
    if (usbPortManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "usbPortManager_ is nullptr");
        return;
    }
    usbPortManager_->GetDumpHelp(fd);
#endif // USB_MANAGER_FEATURE_PORT
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
bool UsbService::IsNotNeedUnload()
{
    bool hasActiveDevices = false;
    bool isGadgetConnected = false;
#ifdef USB_MANAGER_FEATURE_HOST
    std::vector<UsbDevice> devices;
    (void)usbHostManager_->GetDevices(devices);
    hasActiveDevices = !devices.empty();
#endif // USB_MANAGER_FEATURE_HOST
#ifdef USB_MANAGER_FEATURE_DEVICE
    isGadgetConnected = usbDeviceManager_->IsGadgetConnected();
#endif // USB_MANAGER_FEATURE_DEVICE
    USB_HILOGI(MODULE_USB_SERVICE, "hasActiveDevices=[%{public}d],isGadgetConnected=[%{public}d]",
        hasActiveDevices, isGadgetConnected);
    return hasActiveDevices || isGadgetConnected;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::UnLoadSelf(UnLoadSaType type)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UnLoadSelf enter,type=[%{public}d],unloadSelfTimerId_=[%{public}u]",
        type, unloadSelfTimerId_);

    auto task = []() {
        USB_HILOGI(MODULE_USB_SERVICE, "unload usb_service task start.");
        auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgrProxy == nullptr) {
            USB_HILOGE(MODULE_USB_SERVICE, "get samgr failed");
            return;
        }

        if (samgrProxy->UnloadSystemAbility(USB_SYSTEM_ABILITY_ID) != UEC_OK) {
            USB_HILOGE(MODULE_USB_SERVICE, "unload failed");
        }
    };
    if (type == UNLOAD_SA_IMMEDIATELY) {
        task();
        return;
    }
#ifdef USB_MANAGER_FEATURE_HOST
    if (usbHostManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "invalid usbHostManager_");
        return;
    }
#endif // USB_MANAGER_FEATURE_HOST
#ifdef USB_MANAGER_FEATURE_DEVICE
    if (usbDeviceManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "invalid usbDeviceManager_");
        return;
    }
#endif // USB_MANAGER_FEATURE_DEVICE
    std::lock_guard<std::mutex>  guard(unloadSelfTimerMutex_);
    unloadSelfTimer_.Unregister(unloadSelfTimerId_);
    unloadSelfTimer_.Shutdown(false);

    if (IsNotNeedUnload()) {  // delay unload conditions
        USB_HILOGW(MODULE_USB_SERVICE, "not need unload");
        Memory::MemMgrClient::GetInstance().SetCritical(getpid(), true, USB_SYSTEM_ABILITY_ID);
        return;
    } else {
        Memory::MemMgrClient::GetInstance().SetCritical(getpid(), false, USB_SYSTEM_ABILITY_ID);
    }

    if (auto ret = unloadSelfTimer_.Setup(); ret != Utils::TIMER_ERR_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "set up timer failed %{public}u", ret);
        return;
    }
    unloadSelfTimerId_ = unloadSelfTimer_.Register(task, UNLOAD_SA_TIMER_INTERVAL, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UnLoadSelf exit,unloadSelfTimerId_=[%{public}u]", unloadSelfTimerId_);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::UsbdDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "get samgr failed");
        return;
    }

    auto ret = samgrProxy->UnloadSystemAbility(USB_SYSTEM_ABILITY_ID);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "unload failed");
    }
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::SerialDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService SerialDeathRecipient enter");
    service_->FreeTokenId(this->portId_, this->tokenId_);
    service_->CancelSerialRight(this->portId_);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
#ifdef USB_MANAGER_FEATURE_HOST
void UsbService::DeviceDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService DeviceDeathRecipient enter");
    sptr<UsbService> service = UsbService::GetGlobalInstance();
    service->Close(this->busNum_, this->devAddr_);
}
#endif
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::AccessoryDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService AccessoryDeathRecipient enter");
    sptr<UsbService> service = UsbService::GetGlobalInstance();
    service->CloseAccessory(this->fd_);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::FreeTokenId(int32_t portId, uint32_t tokenId)
{
    usbSerialManager_->FreeTokenId(portId, tokenId);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
sptr<UsbService> UsbService::GetGlobalInstance()
{
    return g_serviceInstance;
}
// LCOV_EXCL_STOP

bool CheckForTtyUSB()
{
    USB_HILOGI(MODULE_USB_SERVICE, "CheckForTtyUSB");
    if (!exists(TTYUSB_PATH)) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: The path does not exist", __func__);
        return false;
    }
    for (const auto& entry : std::filesystem::directory_iterator(TTYUSB_PATH)) {
        if (entry.is_directory()) {
            return true;
        }
    }
    USB_HILOGI(MODULE_USB_SERVICE, "can't find ttyUSB");
    return false;
}

// LCOV_EXCL_START
int32_t UsbService::DeviceEvent(const HDI::Usb::V1_0::USBDeviceInfo &info)
{
    int32_t status = info.status;
#ifdef USB_MANAGER_FEATURE_DEVICE
    if (status == ACT_UPDEVICE || status == ACT_DOWNDEVICE ||
        status == ACT_ACCESSORYUP || status == ACT_ACCESSORYDOWN || status == ACT_ACCESSORYSEND ||
        status == ACT_CUSTOMCONTROLREQUEST) {
        USB_HILOGI(MODULE_USB_SERVICE, "device: usb, deviceAction is %{public}d", status);
        g_serviceInstance->UpdateDeviceState(status);
        g_serviceInstance->UnLoadSelf(UsbService::UnLoadSaType::UNLOAD_SA_DELAY);
        return UEC_OK;
    }
#endif // USB_MANAGER_FEATURE_DEVICE
#ifdef USB_MANAGER_FEATURE_HOST
    int32_t busNum = info.busNum;
    int32_t devAddr = info.devNum;
    if (status == ACT_DEVUP) {
        USB_HILOGI(MODULE_USB_SERVICE, "host: usb attached");
        g_serviceInstance->AddDevice(busNum, devAddr);
    } else {
        USB_HILOGI(MODULE_USB_SERVICE, "host: usb detached");
        g_serviceInstance->DelDevice(busNum, devAddr);
    }
    g_serviceInstance->UnLoadSelf(UsbService::UnLoadSaType::UNLOAD_SA_DELAY);
#endif // USB_MANAGER_FEATURE_HOST
    if (status == ACT_DEVUP) {
        std::unique_lock lock(serialManagerMutex_);
        if (usbSerialManager_ == nullptr && CheckForTtyUSB()) {
            USB_HILOGI(MODULE_USB_SERVICE, "try to start serial");
            usbSerialManager_ = std::make_shared<SERIAL::SerialManager>();
#ifdef USB_MANAGER_FEATURE_HOST
            usbHostManager_->SetSerialManager(usbSerialManager_);
#endif // USB_MANAGER_FEATURE_HOST
        }
    }
    return UEC_OK;
}
// LCOV_EXCL_STOP

bool UsbService::IsCallerValid()
{
    OHOS::Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    auto callerTokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenType(callerToken);
    if (callerTokenType == OHOS::Security::AccessToken::TypeATokenTypeEnum::TOKEN_NATIVE) {
        pid_t callerUid = IPCSkeleton::GetCallingUid();
        return callerUid == ROOT_UID || callerUid == EDM_UID;
    }
    return false;
}

// LCOV_EXCL_START
bool UsbService::InitSerial()
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    if (seriald_ == nullptr) {
        seriald_ = OHOS::HDI::Usb::Serial::V1_0::ISerialInterface::Get("serial_interface_service", true);
        if (seriald_ == nullptr) {
            USB_HILOGE(MODULE_USB_SERVICE, "UsbService::InitSerial seriald_ is null");
            return false;
        }
    }

    return true;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::ValidateUsbSerialManagerAndPort(int32_t portId)
{
    if (usbSerialManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: usbSerialManager_ is nullptr", __func__);
        return UEC_SERVICE_INVALID_VALUE;
    }
    
    if (!usbSerialManager_->IsPortIdExist(portId)) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: %{public}d port not exist", __func__, portId);
        return UEC_SERIAL_PORT_NOT_EXIST;
    }
    
    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::SerialOpen(int32_t portId, const sptr<IRemoteObject> &serialRemote)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    int32_t ret = ValidateUsbSerialManagerAndPort(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: ValidateUsbSerialManagerAndPort failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialOpen", ret, "ValidateUsbSerialManagerAndPort failed");
        return ret;
    }

    bool hasRight = false;
    HasSerialRight(portId, hasRight);
    if (!hasRight) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: There are no permissions", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialOpen", UEC_SERVICE_PERMISSION_DENIED,
            "There are no permissions");
        return UEC_SERVICE_PERMISSION_DENIED;
    }

    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    sptr<UsbService::SerialDeathRecipient> serialRecipient_ = new SerialDeathRecipient(this, portId, tokenId);
    if (serialRecipient_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: serialRecipient_ is nullptr", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialOpen", UEC_SERVICE_INVALID_VALUE,
            "serialRecipient_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    if (!serialRemote->AddDeathRecipient(serialRecipient_)) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: SerialOpen add DeathRecipient failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialOpen", UEC_SERVICE_INVALID_VALUE,
            "SerialOpen add DeathRecipient failed");
        return UEC_SERVICE_INVALID_VALUE;
    }

    ret = usbSerialManager_->SerialOpen(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: SerialOpen failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialOpen", ret, "SerialOpen failed");
        return ret;
    }

    ReportUsbSerialOperationSysEvent(portId, "SerialOpen");
    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::SerialClose(int32_t portId)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    int32_t ret = ValidateUsbSerialManagerAndPort(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: ValidateUsbSerialManagerAndPort failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialClose", ret, "ValidateUsbSerialManagerAndPort failed");
        return ret;
    }

    ReportUsbSerialOperationSysEvent(portId, "SerialClose");
    ret = usbSerialManager_->SerialClose(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: SerialClose failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialClose", ret, "SerialClose failed");
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::SerialRead(int32_t portId, std::vector<uint8_t>& data, uint32_t size,
    uint32_t &actualSize, uint32_t timeout)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    HITRACE_METER_NAME(HITRACE_TAG_USB, "SerialRead");
    if (size > SERIALREAD_SIZE_MAX || data.size() > SERIALREAD_SIZE_MAX) {
        USB_HILOGE(MODULE_USB_SERVICE, "size is more than 64kb");
        return UEC_SERIAL_IO_EXCEPTION;
    }
    int32_t ret = ValidateUsbSerialManagerAndPort(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: ValidateUsbSerialManagerAndPort failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialRead", ret, "ValidateUsbSerialManagerAndPort failed");
        return ret;
    }

    ret = usbSerialManager_->SerialRead(portId, data, size, actualSize, timeout);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: SerialRead failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialRead", ret, "SerialRead failed");
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::SerialWrite(int32_t portId, const std::vector<uint8_t>& data, uint32_t size,
    uint32_t &actualSize, uint32_t timeout)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    HITRACE_METER_NAME(HITRACE_TAG_USB, "SerialWrite");
    if (size > SERIALWRITE_SIZE_MAX || data.size() > SERIALWRITE_SIZE_MAX) {
        USB_HILOGE(MODULE_USB_SERVICE, "size is more than 200kb");
        return UEC_SERIAL_IO_EXCEPTION;
    }
    int32_t ret = ValidateUsbSerialManagerAndPort(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: ValidateUsbSerialManagerAndPort failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialWrite", ret, "ValidateUsbSerialManagerAndPort failed");
        return ret;
    }

    ret = usbSerialManager_->SerialWrite(portId, data, size, actualSize, timeout);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: SerialWrite failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialWrite", ret, "SerialWrite failed");
    }
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::SerialAttributeChange(const UsbSerialAttr &serialAttr,
    OHOS::HDI::Usb::Serial::V1_0::SerialAttribute& attribute)
{
    attribute.baudrate = serialAttr.baudRate_;
    attribute.stopBits = serialAttr.stopBits_;
    attribute.parity = serialAttr.parity_;
    attribute.dataBits = serialAttr.dataBits_;
    return;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::SerialAttributeChange(UsbSerialAttr &serialAttr,
    OHOS::HDI::Usb::Serial::V1_0::SerialAttribute& attribute)
{
    serialAttr.baudRate_ = attribute.baudrate;
    serialAttr.stopBits_ = attribute.stopBits;
    serialAttr.parity_ = attribute.parity;
    serialAttr.dataBits_ = attribute.dataBits;
    return;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::SerialGetAttribute(int32_t portId, UsbSerialAttr& attributeInfo)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    int32_t ret = ValidateUsbSerialManagerAndPort(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: ValidateUsbSerialManagerAndPort failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialGetAttribute", ret,
            "ValidateUsbSerialManagerAndPort failed");
        return ret;
    }
    OHOS::HDI::Usb::Serial::V1_0::SerialAttribute serialAttr;
    ret = usbSerialManager_->SerialGetAttribute(portId, serialAttr);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: SerialGetAttribute failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialGetAttribute", ret, "SerialGetAttribute failed");
    }
    SerialAttributeChange(attributeInfo, serialAttr);
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::SerialSetAttribute(int32_t portId,
    const UsbSerialAttr& attributeInfo)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    int32_t ret = ValidateUsbSerialManagerAndPort(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: ValidateUsbSerialManagerAndPort failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialSetAttribute", ret,
            "ValidateUsbSerialManagerAndPort failed");
        return ret;
    }
    OHOS::HDI::Usb::Serial::V1_0::SerialAttribute serialAttr;
    SerialAttributeChange(attributeInfo, serialAttr);
    ret = usbSerialManager_->SerialSetAttribute(portId, serialAttr);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: SerialSetAttribute failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "SerialSetAttribute", ret, "SerialSetAttribute failed");
        return ret;
    }

    ReportUsbSerialOperationSysEvent(portId, "SerialSetAttribute");
    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::SerialPortChange(std::vector<UsbSerialPort> &serialInfoList,
    std::vector<OHOS::HDI::Usb::Serial::V1_0::SerialPort>& serialPortList)
{
    for (size_t i = 0; i < serialPortList.size(); i++) {
        UsbSerialPort info;
        info.portId_ = serialPortList[i].portId;
        info.busNum_ = serialPortList[i].deviceInfo.busNum;
        info.devAddr_ = serialPortList[i].deviceInfo.devAddr;
        info.vid_ = serialPortList[i].deviceInfo.vid;
        info.pid_ = serialPortList[i].deviceInfo.pid;
        info.serialNum_ = serialPortList[i].deviceInfo.serialNum;
        serialInfoList.push_back(info);
    }
    return;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::SerialGetPortList(std::vector<UsbSerialPort>& serialInfoList)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    std::unique_lock lock(serialManagerMutex_);
    if (usbSerialManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: usbSerialManager_ is nullptr", __func__);
        if (CheckForTtyUSB()) {
            USB_HILOGI(MODULE_USB_SERVICE, "try to start serial");
            usbSerialManager_ = std::make_shared<SERIAL::SerialManager>();
#ifdef USB_MANAGER_FEATURE_HOST
            usbHostManager_->SetSerialManager(usbSerialManager_);
#endif // USB_MANAGER_FEATURE_HOST
        } else {
            return UEC_OK;
        }
    }
    lock.unlock();
    std::vector<OHOS::HDI::Usb::Serial::V1_0::SerialPort> serialPortList;
    int32_t ret = usbSerialManager_->SerialGetPortList(serialPortList);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: SerialGetPortList failed", __func__);
        return ret;
    }
    SerialPortChange(serialInfoList, serialPortList);
    UpdateDeviceVidPidMap(serialPortList);
    return ret;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::CheckDbAbility(int32_t portId)
{
    std::shared_ptr<UsbRightDbHelper> helper = OHOS::USB::UsbRightDbHelper::GetInstance();
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: get dbHelper failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "RequestSerialRight", UEC_SERIAL_DATEBASE_ERROR,
            "get dbHelper failed");
        return UEC_SERIAL_DATEBASE_ERROR;
    } else {
        return UEC_OK;
    }
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::RequestSerialRight(int32_t portId, bool &hasRight)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    hasRight = false;
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: usbRightManager_ is nullptr", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "RequestSerialRight", UEC_SERVICE_INVALID_VALUE,
            "usbRightManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = ValidateUsbSerialManagerAndPort(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: ValidateUsbSerialManagerAndPort failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "RequestSerialRight", ret, "invalid portId");
        return ret;
    }
    if ((ret = CheckDbAbility(portId)) != UEC_OK) {
        return ret;
    }

    std::string deviceName;
    std::string deviceVidPidSerialNum;

    ret = GetDeviceVidPidSerialNumber(portId, deviceName, deviceVidPidSerialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: can not find deviceName.", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "RequestSerialRight", ret, "can not find deviceName.");
        return ret;
    }

    if (usbRightManager_->IsSystemAppOrSa()) {
        USB_HILOGW(MODULE_USB_SERVICE, "system app, bypass: dev=%{public}s", deviceName.c_str());
        hasRight = true;
        return UEC_OK;
    }

#if defined(USB_MANAGER_FEATURE_HOST) || defined(USB_MANAGER_FEATURE_DEVICE)
    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: GetCallingInfo false", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "RequestSerialRight", UEC_SERVICE_INVALID_VALUE,
            "GetCallingInfo false");
        return UEC_SERVICE_INVALID_VALUE;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "bundle=%{public}s, device=%{public}s, tokenId=%{public}s",
        bundleName.c_str(), deviceName.c_str(), tokenId.c_str());
    
    SerialDeviceIdentity serialDeviceIdentity = { deviceName, deviceVidPidSerialNum };
    if (usbRightManager_->RequestRight(portId, serialDeviceIdentity, bundleName, tokenId, userId) != UEC_OK) {
        USB_HILOGW(MODULE_USB_SERVICE, "%{public}s:user don't agree", __func__);
        return UEC_OK;
    }
#endif // USB_MANAGER_FEATURE_HOST || USB_MANAGER_FEATURE_DEVICE

    hasRight = true;
    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::CancelSerialRight(int32_t portId)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: usbRightManager_ is nullptr", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "CancelSerialRight", UEC_SERVICE_INVALID_VALUE,
            "usbRightManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    int32_t ret = ValidateUsbSerialManagerAndPort(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: ValidateUsbSerialManagerAndPort failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "CancelSerialRight", ret, "validate portId");
        return ret;
    }
    if ((ret = CheckDbAbility(portId)) != UEC_OK) {
        return ret;
    }

    std::string deviceName;
    std::string deviceVidPidSerialNum;

    ret = GetDeviceVidPidSerialNumber(portId, deviceName, deviceVidPidSerialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: can not find deviceName.", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "CancelSerialRight", ret, "can not find deviceName.");
        return ret;
    }

    if (usbRightManager_->IsSystemAppOrSa()) {
        USB_HILOGW(MODULE_USB_SERVICE, "system app, bypass: dev=%{public}s", deviceName.c_str());
        return UEC_OK;
    }

#if defined(USB_MANAGER_FEATURE_HOST) || defined(USB_MANAGER_FEATURE_DEVICE)
    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: GetCallingInfo false", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "CancelSerialRight", UEC_SERVICE_INVALID_VALUE,
            "GetCallingInfo false");
        return UEC_SERVICE_INVALID_VALUE;
    }

    if (!usbRightManager_->RemoveDeviceRight(deviceVidPidSerialNum, bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_SERVICE, "RemoveDeviceRight failed");
        ReportUsbSerialOperationFaultSysEvent(portId, "CancelSerialRight", UEC_SERVICE_PERMISSION_DENIED,
            "RemoveDeviceRight failed");
        return UEC_SERVICE_PERMISSION_DENIED;
    }
#endif // USB_MANAGER_FEATURE_HOST || USB_MANAGER_FEATURE_DEVICE

    usbSerialManager_->SerialClose(portId);
    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::HasSerialRight(int32_t portId, bool &hasRight)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    hasRight = false;
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: usbRightManager_ is nullptr", __func__);
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = ValidateUsbSerialManagerAndPort(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: ValidateUsbSerialManagerAndPort failed", __func__);
        return ret;
    }
    if ((ret = CheckDbAbility(portId)) != UEC_OK) {
        return ret;
    }

    std::string deviceName;
    std::string deviceVidPidSerialNum;

    ret = GetDeviceVidPidSerialNumber(portId, deviceName, deviceVidPidSerialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: can not find deviceName.", __func__);
        return ret;
    }

    if (usbRightManager_->IsSystemAppOrSa()) {
        USB_HILOGW(MODULE_USB_SERVICE, "system app, bypass: dev=%{public}s ", deviceName.c_str());
        hasRight = true;
        return UEC_OK;
    }

#if defined(USB_MANAGER_FEATURE_HOST) || defined(USB_MANAGER_FEATURE_DEVICE)
    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: HasRight GetCallingInfo false", __func__);
        return UEC_SERVICE_INVALID_VALUE;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "bundle=%{public}s, device=%{public}s",
        bundleName.c_str(), deviceName.c_str());
    if (usbRightManager_->HasRight(deviceVidPidSerialNum, bundleName, tokenId, userId)) {
        hasRight = true;
        return UEC_OK;
    }
#endif // USB_MANAGER_FEATURE_HOST || USB_MANAGER_FEATURE_DEVICE

    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::AddSerialRight(uint32_t tokenId, int32_t portId)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    if (usbRightManager_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: usbRightManager_ is nullptr", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "AddSerialRight", UEC_SERVICE_INVALID_VALUE,
            "usbRightManager_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    int32_t ret = CheckSysApiPermission();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE,
            "%{public}s: SerialCheckSysApiPermission failed ret = %{public}d", __func__, ret);
        ReportUsbSerialOperationFaultSysEvent(portId, "AddSerialRight", ret, "SerialCheckSysApiPermission failed");
        return ret;
    }
    if ((ret = CheckDbAbility(portId)) != UEC_OK) {
        return ret;
    }
    std::string deviceName;
    std::string deviceVidPidSerialNum;
    ret = ValidateUsbSerialManagerAndPort(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: ValidateUsbSerialManagerAndPort failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "AddSerialRight", ret, "ValidateUsbSerialManagerAndPort failed");
        return ret;
    }
    ret = GetDeviceVidPidSerialNumber(portId, deviceName, deviceVidPidSerialNum);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: can not find deviceName.", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "AddSerialRight", ret, "can not find deviceName.");
        return ret;
    }

    if (!usbRightManager_->AddDeviceRight(deviceVidPidSerialNum, std::to_string(tokenId).c_str())) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: AddDeviceRight failed", __func__);
        ReportUsbSerialOperationFaultSysEvent(portId, "AddSerialRight", UEC_SERIAL_DATEBASE_ERROR,
            "AddDeviceRight failed");
        return UEC_SERIAL_DATEBASE_ERROR;
    }

    USB_HILOGI(MODULE_USB_SERVICE, "AddRight done");
    return UEC_OK;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
int32_t UsbService::GetDeviceVidPidSerialNumber(int32_t portId, std::string& deviceName, std::string& strDesc)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    int32_t isMatched = UEC_INTERFACE_INVALID_VALUE;
    std::lock_guard<std::mutex> guard(serialPidVidMapMutex_);
    for (auto it = serialVidPidMap_.begin(); it != serialVidPidMap_.end(); ++it) {
        USB_HILOGI(MODULE_USB_SERVICE, " it->first = %{public}d", it->first);
        if (it->first == portId) {
            deviceName = it->second.first;
            strDesc = it->second.second;
            isMatched = UEC_OK;
            break;
        }
    }
    return isMatched;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::UpdateDeviceVidPidMap(std::vector<OHOS::HDI::Usb::Serial::V1_0::SerialPort>& serialPortList)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: Start", __func__);
    std::lock_guard<std::mutex> guard(serialPidVidMapMutex_);
    serialVidPidMap_.clear();
    for (auto &ele: serialPortList) {
        std::string busNum = std::to_string(ele.deviceInfo.busNum);
        std::string devAddr = std::to_string(ele.deviceInfo.devAddr);
        std::string vid = std::to_string(ele.deviceInfo.vid);
        std::string pid = std::to_string(ele.deviceInfo.pid);
        std::string deviceName = busNum + "-" + devAddr;
        std::string deviceVidPidSerialNum = vid + "-" + pid + "-" + ele.deviceInfo.serialNum;
        serialVidPidMap_.insert({ele.portId, {deviceName, deviceVidPidSerialNum}});
    }
    USB_HILOGI(MODULE_USB_SERVICE, "deviceVidPidMap size=%{public}zu", serialVidPidMap_.size());
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::ReportUsbSerialOperationSysEvent(int32_t portId, const std::string &operationType)
{
    USB_HILOGI(MODULE_USB_SERVICE, "the behavior of the usb serial");
    OHOS::HDI::Usb::Serial::V1_0::SerialPort serialPort;
    if (!usbSerialManager_->GetSerialPort(portId, serialPort)) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: GetSerialPort failed", __func__);
        return;
    }

    OHOS::HDI::Usb::Serial::V1_0::SerialAttribute attribute;
    if (usbSerialManager_->SerialGetAttribute(portId, attribute) != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: SerialGetAttribute failed", __func__);
        return;
    }

#if defined(USB_MANAGER_FEATURE_HOST) || defined(USB_MANAGER_FEATURE_DEVICE)
    std::string bundleName;
    std::string tokenId;
    int32_t userId = USB_RIGHT_USERID_INVALID;
    if (!GetCallingInfo(bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s: GetCallingInfo failed", __func__);
        return;
    }

    HiSysEventWrite(HiSysEvent::Domain::USB, "SERIAL_OPERATION",
        HiSysEvent::EventType::BEHAVIOR, "OPERATION_TYPE_NAME", operationType,
        "CLIENT_NAME", bundleName,
        "PORT_VID", serialPort.deviceInfo.vid,
        "PORT_PID", serialPort.deviceInfo.pid,
        "PORT_ID", serialPort.deviceInfo.serialNum,
        "ATTRIBUTE_BAUD_RATE", attribute.baudrate,
        "ATTRIBUTE_STOP_BIT", attribute.stopBits,
        "ATTRIBUTE_PARITY_CHECK", attribute.parity,
        "ATTRIBUTE_DATA_BIT", attribute.dataBits);
#endif // USB_MANAGER_FEATURE_HOST || USB_MANAGER_FEATURE_DEVICE
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::ReportUsbOperationFaultSysEvent(const std::string &operationType, int32_t failReason,
    const std::string &failDescription)
{
    USB_HILOGI(MODULE_USB_SERVICE, "report operation fault");
    HiSysEventWrite(HiSysEvent::Domain::USB, "OPERATION_FAULT",
        HiSysEvent::EventType::FAULT, "OPERATION_TYPE_NAME", operationType,
        "FAIL_REASON", failReason,
        "FAIL_DESCRIPTION", failDescription);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void UsbService::ReportUsbSerialOperationFaultSysEvent(int32_t portId, const std::string &operationType,
    int32_t failReason, const std::string &failDescription)
{
    USB_HILOGI(MODULE_USB_SERVICE, "serial port operation fault");
    HiSysEventWrite(HiSysEvent::Domain::USB, "OPERATION_FAULT",
        HiSysEvent::EventType::FAULT, "OPERATION_TYPE_NAME", operationType,
        "FAIL_REASON", failReason,
        "FAIL_DESCRIPTION", failDescription);
}
// LCOV_EXCL_STOP
} // namespace USB
} // namespace OHOS
