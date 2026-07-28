/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include <regex>
#include "usb_port_manager.h"
#include <unistd.h>
#include "hisysevent.h"
#include "usb_errors.h"
#include "usb_srv_support.h"
#include "if_system_ability_manager.h"
#include "usb_connection_notifier.h"
#include "system_ability_definition.h"
#include "iproxy_broker.h"
#include "iservice_registry.h"

using namespace OHOS::HiviewDFX;
using namespace OHOS::HDI::Usb::V1_0;

namespace OHOS {
namespace USB {
#ifndef USB_MANAGER_V2_0
constexpr int32_t SUPPORTED_MODES = 3;
#endif
constexpr int32_t PARAM_COUNT_TWO = 2;
constexpr int32_t PARAM_COUNT_THR = 3;
constexpr int32_t DEFAULT_ROLE_HOST = 1;
#ifdef USB_MANAGER_HIDUMPER_SET
constexpr int32_t DEFAULT_ROLE_DEVICE = 2;
constexpr int32_t MAX_INPUT_STRING_NUMBER_SIZE = 9;
#endif // USB_MANAGER_HIDUMPER_SET
constexpr uint32_t CMD_INDEX = 1;
constexpr uint32_t PARAM_INDEX = 2;
constexpr int32_t HOST_MODE = 2;
constexpr int32_t WAIT_DELAY_US = 20000;
UsbPortManager::UsbPortManager()
{
    USB_HILOGI(MODULE_USB_PORT, "UsbPortManager::Init start");
#ifndef USB_MANAGER_V2_0
    GetIUsbInterface();
#endif // USB_MANAGER_V2_0
}

UsbPortManager::~UsbPortManager()
{
    USB_HILOGI(MODULE_USB_PORT, "UsbPortManager::unInit start");
}

void UsbPortManager::Init()
{
    USB_HILOGI(MODULE_USB_PORT, "UsbPortManager::QueryPort start");
    int32_t ret = QueryPort();
    if (ret) {
        USB_HILOGE(MODULE_USB_PORT, "UsbPortManager::QueryPort false");
    }
    AddSupportedMode();
}

void UsbPortManager::AddSupportedMode()
{
    USB_HILOGI(MODULE_USB_PORT, "%{public}s:: Enter", __func__);
    supportedModeMap_.clear();
    for (const auto& [portId, port] : portMap_) {
        USB_HILOGI(MODULE_USB_PORT, "%{public}s:: portId is %{public}d, supportedModes is %{public}d",
            __func__, port.id, port.supportedModes);
        supportedModeMap_[portId] = port.supportedModes;
    }
    USB_HILOGI(MODULE_USB_PORT, "%{public}s:: successed", __func__);
}

#ifdef USB_MANAGER_V2_0
bool UsbPortManager::InitUsbPortInterface()
{
    USB_HILOGI(MODULE_USB_PORT, "InitUsbPortInterface in");
    for (int32_t i = 0; i < PARAM_COUNT_THR; i++) {
        usbPortInterface_ = HDI::Usb::V2_0::IUsbPortInterface::Get();
        if (usbPortInterface_ == nullptr) {
            USB_HILOGE(MODULE_USB_PORT, "GetIUsbInterface get usbPortInterface_ is nullptr");
            usleep(WAIT_DELAY_US);
        } else {
            break;
        }
    }
    if (usbPortInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "InitUsbPortInterface get usbPortInterface_ is nullptr");
        return false;
    }
    usbManagerSubscriber_ = new (std::nothrow) UsbManagerSubscriber();
    if (usbManagerSubscriber_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "Init failed");
        return false;
    }
    recipient_ = new UsbdPortDeathRecipient();
    sptr<IRemoteObject> remote = OHOS::HDI::hdi_objcast<HDI::Usb::V2_0::IUsbPortInterface>(usbPortInterface_);
    if (!remote->AddDeathRecipient(recipient_)) {
        USB_HILOGE(MODULE_USB_PORT, "add DeathRecipient failed");
        return false;
    }

    ErrCode ret = usbPortInterface_->BindUsbdPortSubscriber(usbManagerSubscriber_);
    USB_HILOGI(MODULE_USB_PORT, "entry InitUsbPortInterface ret: %{public}d", ret);
    return SUCCEEDED(ret);
}

void UsbPortManager::UsbdPortDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "get samgr failed");
        return;
    }

    auto ret = samgrProxy->UnloadSystemAbility(USB_SYSTEM_ABILITY_ID);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_PORT, "unload failed");
    }
}

void UsbPortManager::Stop()
{
    if (usbPortInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "UsbPortManager::usbPortInterface_ is nullptr");
        return;
    }
    sptr<IRemoteObject> remote = OHOS::HDI::hdi_objcast<HDI::Usb::V2_0::IUsbPortInterface>(usbPortInterface_);
    remote->RemoveDeathRecipient(recipient_);
    recipient_.clear();
    usbPortInterface_->UnbindUsbdPortSubscriber(usbManagerSubscriber_);
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 0, USB_SYSTEM_ABILITY_ID);
}

int32_t UsbPortManager::BindUsbdSubscriber(const sptr<HDI::Usb::V2_0::IUsbdSubscriber> &subscriber)
{
    if (usbPortInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "UsbPortManager::BulkCancel usbPortInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbPortInterface_->BindUsbdPortSubscriber(subscriber);
}

int32_t UsbPortManager::UnbindUsbdSubscriber(const sptr<HDI::Usb::V2_0::IUsbdSubscriber> &subscriber)
{
    if (usbPortInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "UsbPortManager::BulkCancel usbPortInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbPortInterface_->UnbindUsbdPortSubscriber(subscriber);
}
#endif // USB_MANAGER_V2_0

bool UsbPortManager::IsReverseCharge()
{
    int32_t powerRole_ = 0;
    int32_t dataRole_ = 0;
    auto it = portMap_.find(CMD_INDEX);
    if (it == portMap_.end()) {
        USB_HILOGE(MODULE_USB_PORT, "Port not found");
        return false;
    }
    powerRole_ = it->second.usbPortStatus.currentPowerRole;
    dataRole_ = it->second.usbPortStatus.currentDataRole;
    if (powerRole_ == DEFAULT_ROLE_HOST) {
        return true;
    }
    return false;
}

int32_t UsbPortManager::SetPortRole(int32_t portId, int32_t powerRole, int32_t dataRole)
{
    USB_HILOGI(MODULE_USB_PORT, "UsbPortManager SetPortRole enter");
    std::lock_guard<std::mutex> guard(setPortRoleMutex_);
#ifdef USB_MANAGER_V2_0
    if (usbPortInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "UsbPortManager::SetPortRole usbPortInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbPortInterface_->SetPortRole(portId, powerRole, dataRole);
    if (ret != UEC_OK) {
        USB_HILOGI(MODULE_USB_PORT, "setportrole failed");
        return ret;
    }
    if (IsReverseCharge()) {
        UsbConnectionNotifier::GetInstance()->SendNotification(USB_FUNC_REVERSE_CHARGE);
    } else {
        UsbConnectionNotifier::GetInstance()->SendNotification(USB_FUNC_CHARGE);
    }
    return UEC_OK;
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "UsbPortManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t ret = usbd_->SetPortRole(portId, powerRole, dataRole);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_PORT, "setportrole failed");
    }
    return ret;
#endif // USB_MANAGER_V2_0
}

void UsbPortManager::GetIUsbInterface()
{
    if (usbd_ == nullptr) {
        for (int32_t i = 0; i < PARAM_COUNT_THR; i++) {
            usbd_ = IUsbInterface::Get();
            USB_HILOGI(MODULE_USB_PORT, "%{public}s:Get usbd_", __func__);
            if (usbd_ == nullptr) {
                USB_HILOGE(MODULE_USB_PORT, "Get iUsbInteface failed");
                usleep(WAIT_DELAY_US);
            } else {
                break;
            }
        }
    }
}

int32_t UsbPortManager::SetUsbd(const sptr<IUsbInterface> &usbd)
{
    if (usbd == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "UsbPortManager usbd is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    usbd_ = usbd;
    USB_HILOGI(MODULE_USB_PORT, "%{public}s:usbd_ = usbd", __func__);
    return UEC_OK;
}

int32_t UsbPortManager::GetPorts(std::vector<UsbPort> &ports)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (portMap_.size() > 0) {
        for (auto it = portMap_.begin(); it != portMap_.end(); ++it) {
            USB_HILOGI(MODULE_USB_PORT, "%{public}s: portId is %{public}d, supportedModes is %{public}d",
                __func__, it->second.id, it->second.supportedModes);
            ports.push_back(it->second);
        }
        USB_HILOGI(MODULE_USB_PORT, "UsbPortManager::GetPorts success");
        return UEC_OK;
    } else {
        int32_t ret = QueryPort();
        if (ret == UEC_OK) {
            for (auto it = portMap_.begin(); it != portMap_.end(); ++it) {
                ports.push_back(it->second);
            }
            USB_HILOGI(MODULE_USB_PORT, "UsbPortManager::QueryPort and GetPorts success");
            return ret;
        }
    }
    USB_HILOGE(MODULE_USB_PORT, "UsbPortManager::GetPorts false");
    return UEC_SERVICE_INVALID_VALUE;
}

int32_t UsbPortManager::GetSupportedModes(int32_t portId, int32_t &supportedModes)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = supportedModeMap_.find(portId);
    if (it != supportedModeMap_.end()) {
        supportedModes = it->second;
        USB_HILOGI(MODULE_USB_PORT, "UsbPortManager::GetSupportedModes port=%{public}d modes=%{public}d",
                   portId, supportedModes);
        return UEC_OK;
    }
    USB_HILOGE(MODULE_USB_PORT, "UsbPortManagerSupportedModes port %{public}d not found", portId);
    return UEC_SERVICE_INVALID_VALUE;
}

int32_t UsbPortManager::QueryPort()
{
#ifdef USB_MANAGER_V2_0
    if (usbPortInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "UsbPortManager::QueryPort usbPortInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    std::vector<HDI::Usb::V2_0::UsbPort> portList;
    int32_t ret = usbPortInterface_->QueryPorts(portList);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_PORT, "%{public}s QueryPorts failed", __func__);
        return ret;
    }

    for (const auto& it : portList) {
        AddPortInfo(it.id, it.supportedModes,
            it.usbPortStatus.currentMode, it.usbPortStatus.currentDataRole, it.usbPortStatus.currentPowerRole);
    }
#else
    GetIUsbInterface();
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "UsbPortManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    int32_t portId = 0;
    int32_t powerRole = 0;
    int32_t dataRole = 0;
    int32_t mode = 0;
    int32_t ret = usbd_->QueryPort(portId, powerRole, dataRole, mode);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_PORT, "%{public}s QueryPort failed", __func__);
        return ret;
    }

    AddPortInfo(portId, SUPPORTED_MODES, mode, dataRole, powerRole);
#endif // USB_MANAGER_V2_0
    return ret;
}

void UsbPortManager::UpdatePort(int32_t portId, int32_t powerRole, int32_t dataRole, int32_t mode)
{
    USB_HILOGI(MODULE_USB_PORT, "UsbPortManager::updatePort run");
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = portMap_.find(portId);
    if (it != portMap_.end()) {
        if (it->second.id == portId) {
            ReportPortRoleChangeSysEvent(it->second.usbPortStatus.currentPowerRole, powerRole,
                it->second.usbPortStatus.currentDataRole, dataRole);
            it->second.usbPortStatus.currentPowerRole = powerRole;
            it->second.usbPortStatus.currentDataRole = dataRole;
            if (it->second.usbPortStatus.currentDataRole == UsbSrvSupport::DATA_ROLE_HOST) {
                it->second.usbPortStatus.currentMode = UsbSrvSupport::PORT_MODE_HOST;
            } else if (it->second.usbPortStatus.currentDataRole == UsbSrvSupport::DATA_ROLE_DEVICE) {
                it->second.usbPortStatus.currentMode = UsbSrvSupport::PORT_MODE_DEVICE;
            }
            USB_HILOGI(MODULE_USB_PORT, "UsbPortManager::updatePort seccess");
            return;
        }
    }
    USB_HILOGE(MODULE_USB_PORT, "updatePort false");
}

void UsbPortManager::UpdatePort(int32_t portId, int32_t powerRole, int32_t dataRole,
    int32_t mode, int32_t supportedModes)
{
    USB_HILOGI(MODULE_USB_PORT, "UsbPortManager::updatePort run");
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = portMap_.find(portId);
    if (it != portMap_.end()) {
        if (it->second.id == portId) {
            ReportPortRoleChangeSysEvent(it->second.usbPortStatus.currentPowerRole, powerRole,
                it->second.usbPortStatus.currentDataRole, dataRole);
            it->second.usbPortStatus.currentPowerRole = powerRole;
            it->second.usbPortStatus.currentDataRole = dataRole;
            if (it->second.usbPortStatus.currentDataRole == UsbSrvSupport::DATA_ROLE_HOST) {
                it->second.usbPortStatus.currentMode = UsbSrvSupport::PORT_MODE_HOST;
            } else if (it->second.usbPortStatus.currentDataRole == UsbSrvSupport::DATA_ROLE_DEVICE) {
                it->second.usbPortStatus.currentMode = UsbSrvSupport::PORT_MODE_DEVICE;
            }
            USB_HILOGI(MODULE_USB_PORT, "UsbPortManager::updatePort seccess");
            return;
        }
    }
    USB_HILOGE(MODULE_USB_PORT, "updatePort false");
}

void UsbPortManager::AddPortInfo(int32_t portId, int32_t supportedModes,
    int32_t currentMode, int32_t currentDataRole, int32_t currentPowerRole)
{
    UsbPort usbPort;
    usbPort.id = portId;
    usbPort.supportedModes = supportedModes;
    usbPort.usbPortStatus.currentMode = currentMode;
    usbPort.usbPortStatus.currentDataRole = currentDataRole;
    usbPort.usbPortStatus.currentPowerRole = currentPowerRole;
    AddPort(usbPort);
}

void UsbPortManager::AddPort(UsbPort &port)
{
    USB_HILOGI(MODULE_USB_PORT, "addPort run, portId is %{public}d, supportedModes is %{public}d",
        port.id, port.supportedModes);

    auto res = portMap_.insert(std::map<int32_t, UsbPort>::value_type(port.id, port));
    if (!res.second) {
        USB_HILOGW(MODULE_USB_PORT, "addPort port id duplicated");
        return;
    }
    USB_HILOGI(MODULE_USB_PORT, "addPort successed");
}

void UsbPortManager::RemovePort(int32_t portId)
{
    USB_HILOGI(MODULE_USB_PORT, "removePort run");
    std::lock_guard<std::mutex> lock(mutex_);
    size_t num = portMap_.erase(portId);
    if (num == 0) {
        USB_HILOGW(MODULE_USB_PORT, "removePort false");
        return;
    }
    USB_HILOGI(MODULE_USB_PORT, "removePort seccess");
}

void UsbPortManager::GetPortsInfo(int32_t fd)
{
    std::vector<UsbPort> usbports;
    int32_t ret = GetPorts(usbports);
    if (ret != UEC_OK) {
        dprintf(fd, "%s:GetPorts failed\n", __func__);
        return;
    }

    if (usbports[0].usbPortStatus.currentMode == HOST_MODE) {
        dprintf(fd, "get current port %d: host\n", usbports[0].usbPortStatus.currentMode);
    } else {
        dprintf(fd, "get current port %d: device\n", usbports[0].usbPortStatus.currentMode);
    }
}

void UsbPortManager::GetDumpHelp(int32_t fd)
{
    dprintf(fd, "=========== dump the all device port ===========\n");
    dprintf(fd, "usb_port -a: Query All Port List\n");
    dprintf(fd, "usb_port -p Q: Query Port\n");
#ifdef USB_MANAGER_HIDUMPER_SET
    dprintf(fd, "usb_port -p 1: Switch to host\n");
    dprintf(fd, "usb_port -p 2: Switch to device\n");
#endif // USB_MANAGER_HIDUMPER_SET
    dprintf(fd, "------------------------------------------------\n");
}

void UsbPortManager::DumpGetSupportPort(int32_t fd)
{
    dprintf(fd, "Usb Port device status:\n");
    std::vector<UsbPort> ports;
    if (GetPorts(ports) != UEC_OK) {
        dprintf(fd, "UsbPortManager::GetPorts failed, port is empty\n");
        return;
    }

    if (ports.size() == 0) {
        dprintf(fd, "all port list is empty\n");
        return;
    }
    for (size_t i = 0; i < ports.size(); ++i) {
        dprintf(fd, "id: %d | supportedModes: %d | currentMode: %d | currentPowerRole: %d | currentDataRole: %d\n",
            ports[i].id, ports[i].supportedModes, ports[i].usbPortStatus.currentMode,
            ports[i].usbPortStatus.currentPowerRole, ports[i].usbPortStatus.currentDataRole);
    }
}

void UsbPortManager::DumpSetPortRoles(int32_t fd, const std::string &args)
{
#ifdef USB_MANAGER_V2_0
    if (usbPortInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "UsbPortManager::DumpSetPortRoles usbPortInterface_ is nullptr");
        return;
    }
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_PORT, "UsbPortManager::DumpSetPortRoles usbd_ is nullptr");
        return;
    }
#endif // USB_MANAGER_V2_0
    if (args.compare("Q") == 0) {
        GetPortsInfo(fd);
        return;
    }
#ifdef USB_MANAGER_HIDUMPER_SET
    if (!std::regex_match(args, std::regex("^[0-9]+$"))) {
        dprintf(fd, "Invalid input, please enter a valid integer\n");
        GetDumpHelp(fd);
        return;
    }
    if (args.size() > MAX_INPUT_STRING_NUMBER_SIZE) {
        dprintf(fd, "Invalid input, value out of range\n");
        return;
    }
    int32_t mode = stoi(args);
    switch (mode) {
        case DEFAULT_ROLE_HOST:
            SetPortRole(
                UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
            GetPortsInfo(fd);
            break;
        case DEFAULT_ROLE_DEVICE:
            SetPortRole(
                UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
            GetPortsInfo(fd);
            break;
        default:
            dprintf(fd, "port param error, please enter again\n");
            GetDumpHelp(fd);
    }
#else
    dprintf(fd, "port param error, please enter again\n");
    GetDumpHelp(fd);
#endif // USB_MANAGER_HIDUMPER_SET
}

void UsbPortManager::Dump(int32_t fd, const std::vector<std::string> &args)
{
    if (args.size() < PARAM_COUNT_TWO || args.size() > PARAM_COUNT_THR) {
        GetDumpHelp(fd);
        return;
    }

    if (args[CMD_INDEX] == "-a" && args.size() == PARAM_COUNT_TWO) {
        DumpGetSupportPort(fd);
    } else if (args[CMD_INDEX] == "-p" && args.size() == PARAM_COUNT_THR) {
        DumpSetPortRoles(fd, args[PARAM_INDEX]);
    } else {
        dprintf(fd, "port param error, please enter again\n");
        GetDumpHelp(fd);
    }
}

void UsbPortManager::ReportPortRoleChangeSysEvent(
    int32_t currentPowerRole, int32_t updatePowerRole, int32_t currentDataRole, int32_t updateDataRole)
{
    USB_HILOGI(MODULE_USB_PORT, "Port switch points are as follows:");
    HiSysEventWrite(HiSysEvent::Domain::USB, "PORT_ROLE_CHANGED",
        HiSysEvent::EventType::BEHAVIOR, "CURRENT_POWERROLE",
        currentPowerRole, "UPDATE_POWERROLE", updatePowerRole, "CURRENT_DATAROLE", currentDataRole, "UPDATE_DATAROLE",
        updateDataRole);
}
} // namespace USB
} // namespace OHOS
