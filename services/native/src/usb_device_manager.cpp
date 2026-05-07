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

#include <regex>
#include "usb_device_manager.h"
#include <hdf_base.h>
#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "usb_connection_notifier.h"
#include "hisysevent.h"
#include "usb_errors.h"
#include "usb_srv_support.h"
#include "usbd_type.h"

using namespace OHOS::AAFwk;
using namespace OHOS::EventFwk;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HDI::Usb::V1_0;

namespace OHOS {
namespace USB {
constexpr int32_t PARAM_COUNT_TWO = 2;
constexpr int32_t PARAM_COUNT_THR = 3;
constexpr int32_t DECIMAL_BASE = 10;
constexpr uint32_t CMD_INDEX = 1;
constexpr uint32_t PARAM_INDEX = 2;
constexpr uint32_t DELAY_CONNECT_INTERVAL = 1000;
constexpr uint32_t DELAY_DISCONN_INTERVAL = 1400;
const std::map<std::string_view, uint32_t> UsbDeviceManager::FUNCTION_MAPPING_N2C = {
    {UsbSrvSupport::FUNCTION_NAME_NONE, UsbSrvSupport::FUNCTION_NONE},
    {UsbSrvSupport::FUNCTION_NAME_ACM, UsbSrvSupport::FUNCTION_ACM},
    {UsbSrvSupport::FUNCTION_NAME_ECM, UsbSrvSupport::FUNCTION_ECM},
    {UsbSrvSupport::FUNCTION_NAME_HDC, UsbSrvSupport::FUNCTION_HDC},
    {UsbSrvSupport::FUNCTION_NAME_MTP, UsbSrvSupport::FUNCTION_MTP},
    {UsbSrvSupport::FUNCTION_NAME_PTP, UsbSrvSupport::FUNCTION_PTP},
    {UsbSrvSupport::FUNCTION_NAME_RNDIS, UsbSrvSupport::FUNCTION_RNDIS},
    {UsbSrvSupport::FUNCTION_NAME_NCM, UsbSrvSupport::FUNCTION_NCM},
    {UsbSrvSupport::FUNCTION_NAME_STORAGE, UsbSrvSupport::FUNCTION_STORAGE},
    {UsbSrvSupport::FUNCTION_NAME_DEVMODE_AUTH, UsbSrvSupport::FUNCTION_DEVMODE_AUTH},
};

UsbDeviceManager::UsbDeviceManager()
{
    USB_HILOGI(MODULE_USB_DEVICE, "UsbDeviceManager::Init start");
#ifndef USB_MANAGER_V2_0
    if (usbd_ == nullptr) {
        usbd_ = IUsbInterface::Get();
        USB_HILOGI(MODULE_USB_DEVICE, "%{public}s:%{public}d usbd_ == nullptr: %{public}d",
            __func__, __LINE__, usbd_ == nullptr);
    } else {
        USB_HILOGW(MODULE_USB_DEVICE, "%{public}s:usbd_ != nullptr", __func__);
    }
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::Get inteface failed");
    }
#endif // USB_MANAGER_V2_0
}

#ifdef USB_MANAGER_V2_0
bool UsbDeviceManager::InitUsbDeviceInterface()
{
    USB_HILOGI(MODULE_USB_DEVICE, "InitUsbDeviceInterface in");
    usbDeviceInterface_ = HDI::Usb::V2_0::IUsbDeviceInterface::Get();
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "InitUsbDeviceInterface get usbDeviceInterface_ is nullptr");
        return false;
    }

    usbManagerSubscriber_ = new (std::nothrow) UsbManagerSubscriber();
    if (usbManagerSubscriber_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "Init failed");
        return false;
    }

    ErrCode ret = usbDeviceInterface_->BindUsbdDeviceSubscriber(usbManagerSubscriber_);
    USB_HILOGI(MODULE_USB_DEVICE, "entry InitUsbDeviceInterface ret: %{public}d", ret);
    return SUCCEEDED(ret);
}

void UsbDeviceManager::Stop()
{
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::usbDeviceInterface_ is nullptr");
        return;
    }
    usbDeviceInterface_->UnbindUsbdDeviceSubscriber(usbManagerSubscriber_);
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 0, USB_SYSTEM_ABILITY_ID);
}

int32_t UsbDeviceManager::BindUsbdSubscriber(const sptr<HDI::Usb::V2_0::IUsbdSubscriber> &subscriber)
{
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::BulkCancel usbDeviceInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbDeviceInterface_->BindUsbdDeviceSubscriber(subscriber);
}

int32_t UsbDeviceManager::UnbindUsbdSubscriber(const sptr<HDI::Usb::V2_0::IUsbdSubscriber> &subscriber)
{
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::BulkCancel usbDeviceInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbDeviceInterface_->UnbindUsbdDeviceSubscriber(subscriber);
}

void UsbDeviceManager::ProcessCustomControlRequestUevent(int32_t status)
{
    USB_HILOGD(MODULE_USB_DEVICE, "%{public}s, recv status: %{public}d", __func__, status);
    if (status != ACT_CUSTOMCONTROLREQUEST) {
        return;
    }
    std::vector<uint8_t> ueventData;
    int32_t ret = GetCustomControlRequestData(status, ueventData);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "get uevent dqata failed, ret: %{public}d", ret);
        return;
    }
    cJSON *root = cJSON_CreateArray();
    for (uint8_t value : ueventData) {
        cJSON_AddItemToArray(root, cJSON_CreateNumber(value));
    }
    char *pUeventDataStr = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!pUeventDataStr) {
        USB_HILOGE(MODULE_USB_SERVICE, "print json error.");
        return;
    }

    Want want;
    want.SetAction(CommonEventSupport::COMMON_EVENT_USB_CONTROL_DATA);
    CommonEventData data(want);
    data.SetData(pUeventDataStr);
    CommonEventPublishInfo publishInfo;
    USB_HILOGI(MODULE_USB_SERVICE, "send COMMON_EVENT_USB_CONTROL_DATA broadcast.");
    CommonEventManager::PublishCommonEvent(data, publishInfo);
    USB_HILOGD(MODULE_USB_SERVICE, "send COMMON_EVENT_USB_CONTROL_DATA broadcast device:%{public}s",
        pUeventDataStr);
    cJSON_free(pUeventDataStr);
    pUeventDataStr = nullptr;
}

int32_t UsbDeviceManager::GetCustomControlRequestData(int32_t id, std::vector<uint8_t> &data)
{
    if (usbDeviceInterfaceV2_1_ == nullptr) {
        usbDeviceInterfaceV2_1_ = HDI::Usb::V2_1::IUsbDeviceInterface::CastFrom(usbDeviceInterface_);
    }

    if (usbDeviceInterfaceV2_1_ != nullptr) {
        return usbDeviceInterfaceV2_1_->GetControlTransferData(id, data);
    }
    return UEC_SERVICE_INVALID_OPERATION;
}
#endif

int32_t UsbDeviceManager::GetCurrentFunctions(int32_t& funcs)
{
#ifdef USB_MANAGER_V2_0
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::usbDeviceInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    std::lock_guard<std::mutex> guard(functionMutex_);
    return usbDeviceInterface_->GetCurrentFunctions(funcs);
#else
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    std::lock_guard<std::mutex> guard(functionMutex_);
    return usbd_->GetCurrentFunctions(funcs);
#endif // USB_MANAGER_V2_0
}

#ifdef USB_MANAGER_V2_0
int32_t UsbDeviceManager::SetCurrentFunctions(int32_t funcs)
{
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::usbDeviceInterface_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t lastFunc;
    std::lock_guard<std::mutex> guard(functionMutex_);
    if (usbDeviceInterface_->GetCurrentFunctions(lastFunc) != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::get current functions fail");
        ReportUsbOperationFaultSysEvent("FUNCTION_CHANGED", UEC_SERVICE_INVALID_VALUE, "DevIntfGetFunc is failed");
        return UEC_SERVICE_INVALID_VALUE;
    }
    if (lastFunc == funcs) {
        USB_HILOGI(MODULE_USB_DEVICE, "UsbDeviceManager::no change in functionality");
        return UEC_OK;
    }
    int32_t ret = usbDeviceInterface_->SetCurrentFunctions(funcs);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::usbDeviceInterface_ set function error");
        ReportUsbOperationFaultSysEvent("FUNCTION_CHANGED", ret, "DevIntfSetFunc is failed");
        if (ret == HDF_ERR_NOT_SUPPORT) {
            return UEC_SERVICE_FUNCTION_NOT_SUPPORT;
        }
        return ret;
    }
    UpdateFunctions(funcs);
    return UEC_OK;
}
#else
int32_t UsbDeviceManager::SetCurrentFunctions(int32_t funcs)
{
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::usbd_ is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t lastFunc;
    std::lock_guard<std::mutex> guard(functionMutex_);
    if (usbd_->GetCurrentFunctions(lastFunc) != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::get current functions fail");
        ReportUsbOperationFaultSysEvent("FUNCTION_CHANGED", UEC_SERVICE_INVALID_VALUE, "UsbdfGetFunc is failed");
        return UEC_SERVICE_INVALID_VALUE;
    }
    if (lastFunc == funcs) {
        USB_HILOGI(MODULE_USB_DEVICE, "UsbDeviceManager::no change in functionality");
        return UEC_OK;
    }
    int32_t ret = usbd_->SetCurrentFunctions(funcs);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::usbd_ set function error");
        ReportUsbOperationFaultSysEvent("FUNCTION_CHANGED", UEC_SERVICE_INVALID_VALUE, "UsbdSetFunc is failed");
        if (ret == HDF_ERR_NOT_SUPPORT) {
            return UEC_SERVICE_FUNCTION_NOT_SUPPORT;
        }
        return ret;
    }
    UpdateFunctions(funcs);
    return UEC_OK;
}
#endif // USB_MANAGER_V2_0

int32_t UsbDeviceManager::Init()
{
    std::shared_ptr<UsbFunctionSwitchWindow> window_ = UsbFunctionSwitchWindow::GetInstance();
    if (window_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "get usb function switch window failed");
        return UEC_SERVICE_INNER_ERR;
    }

    int32_t ret = window_->Init();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "Init usb function switch window failed");
    }
    return ret;
}

int32_t UsbDeviceManager::SetUsbd(const sptr<IUsbInterface> &usbd)
{
    if (usbd == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager usbd is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    if (usbd_ == nullptr) {
        usbd_ = usbd;
    } else {
        USB_HILOGW(MODULE_USB_DEVICE, "%{public}s:usbd_ != nullptr", __func__);
    }
    return UEC_OK;
}

bool UsbDeviceManager::IsSettableFunctions(int32_t funcs)
{
    return static_cast<uint32_t>(funcs) == UsbSrvSupport::FUNCTION_NONE ||
        ((~functionSettable_ & static_cast<uint32_t>(funcs)) == 0);
}

uint32_t UsbDeviceManager::ConvertFromString(std::string_view strFun)
{
    if (strFun == UsbSrvSupport::FUNCTION_NAME_NONE) {
        return UsbSrvSupport::FUNCTION_NONE;
    }

    size_t len = strFun.length();
    if (len == 0) {
        return UEC_SERVICE_INVALID_VALUE;
    }

    std::vector<std::string_view> vModeStr;
    size_t pos = 0;
    while (pos < len) {
        size_t findPos = strFun.find(",", pos);
        if (findPos == strFun.npos) {
            vModeStr.push_back(strFun.substr(pos, len - pos));
            break;
        }
        vModeStr.push_back(strFun.substr(pos, findPos - pos));
        pos = findPos + 1;
    }

    uint32_t ret = 0;
    for (auto &&item : vModeStr) {
        auto it = FUNCTION_MAPPING_N2C.find(item);
        if (it != FUNCTION_MAPPING_N2C.end()) {
            ret |= it->second;
        } else {
            USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::ConvertFromString Invalid argument of usb function");
            return UEC_SERVICE_INVALID_VALUE;
        }
    }

    return ret;
}

std::string UsbDeviceManager::ConvertToString(uint32_t function)
{
    std::string stream;
    if (function <= UsbSrvSupport::FUNCTION_NONE || function > functionSettable_) {
        return std::string {UsbSrvSupport::FUNCTION_NAME_NONE};
    }
    bool flag = false;
    for (auto it = FUNCTION_MAPPING_N2C.begin(); it != FUNCTION_MAPPING_N2C.end(); ++it) {
        if ((function & it->second) != 0) {
            if (flag) {
                stream += ",";
            }
            stream += std::string {it->first};
            flag = true;
        }
    }
    USB_HILOGI(MODULE_USB_DEVICE, "UsbDeviceManager::ConvertToString success");
    return stream;
}

void UsbDeviceManager::UpdateFunctions(int32_t func)
{
    USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: func %{public}d, currentFunctions_ %{public}d", __func__, func,
        currentFunctions_);
    if (func == currentFunctions_) {
        return;
    }
    ReportFuncChangeSysEvent(currentFunctions_, func);
    currentFunctions_ = func;
    if (!connected_ && gadgetConnected_) {
        USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: connect %{public}d, gadget connect %{public}d",
            __func__, connected_, gadgetConnected_);
    } else {
        BroadcastFuncChange(connected_, currentFunctions_);
    }
    ProcessFunctionNotifier(connected_, func);
}

int32_t UsbDeviceManager::GetCurrentFunctions()
{
    return currentFunctions_;
}

void UsbDeviceManager::ProcessStatus(int32_t status, bool &curConnect)
{
    switch (status) {
        case ACT_UPDEVICE:
            curConnect = true;
            gadgetConnected_ = true;
            break;
        case ACT_DOWNDEVICE:
            curConnect = false;
            gadgetConnected_ = false;
            break;
        case ACT_ACCESSORYUP:
        case ACT_ACCESSORYDOWN:
        case ACT_ACCESSORYSEND:
        case ACT_CUSTOMCONTROLREQUEST: {
            isDisableDialog_ = true;
            USB_HILOGI(MODULE_USB_DEVICE, "disable dialog success");
            ProcessFunctionSwitchWindow(false);
            return;
        }
        default:
            USB_HILOGE(MODULE_USB_DEVICE, "invalid status %{public}d", status);
            return;
    }
}

void UsbDeviceManager::SetPhyConnectState(bool phyConnect)
{
    phyConnect_ = phyConnect;
}

#ifdef USB_MANAGER_V2_0
void UsbDeviceManager::HandleEvent(int32_t status)
{
    if (usbDeviceInterface_ == nullptr) {
        return;
    }
    bool curConnect = false;
    ProcessStatus(status, curConnect);
    UsbTimerWrapper::GetInstance()->Unregister(delayDisconnTimerId_);
    if (curConnect && (connected_ != curConnect)) {
        auto task = [&]() {
            USB_HILOGI(MODULE_USB_DEVICE, "execute connect task:%{public}d", currentFunctions_);
            connected_ = true;
            GetCurrentFunctions(currentFunctions_);
            ProcessFuncChange(connected_, currentFunctions_);
        };
        delayDisconnTimerId_ = UsbTimerWrapper::GetInstance()->Register(task, DELAY_CONNECT_INTERVAL, true);
        USB_HILOGI(MODULE_USB_DEVICE, "register a connect task, id %{public}u", delayDisconnTimerId_);
    } else if (!curConnect && (connected_ != curConnect)) {
        auto task = [&]() {
            USB_HILOGI(MODULE_USB_DEVICE, "execute disconnect task:%{public}d", currentFunctions_);
            connected_ = false;
            isDisableDialog_ = false;
            if ((static_cast<uint32_t>(currentFunctions_) & USB_FUNCTION_MTP) != 0 ||
                (static_cast<uint32_t>(currentFunctions_) & USB_FUNCTION_PTP) != 0) {
                currentFunctions_ = static_cast<uint32_t>(currentFunctions_) &
                    (~USB_FUNCTION_MTP) & (~USB_FUNCTION_PTP);
                USB_HILOGI(MODULE_USB_DEVICE, "usb function reset %{public}d", currentFunctions_);
                currentFunctions_ = currentFunctions_ == 0 ? USB_FUNCTION_STORAGE : currentFunctions_;
                usbDeviceInterface_->SetCurrentFunctions(currentFunctions_);
            } else if ((static_cast<uint32_t>(currentFunctions_) & USB_FUNCTION_DEVMODE_AUTH) != 0) {
                currentFunctions_ = USB_FUNCTION_STORAGE;
                usbDeviceInterface_->SetCurrentFunctions(currentFunctions_);
            }
            ProcessFuncChange(connected_, currentFunctions_);
            return;
        };
        int32_t delayTime = DELAY_DISCONN_INTERVAL;
        if (phyConnect_) {
            delayTime += DELAY_DISCONN_INTERVAL;
            USB_HILOGI(MODULE_USB_DEVICE, "Physical is plug");
        }
        delayDisconnTimerId_ = UsbTimerWrapper::GetInstance()->Register(task, delayTime, true);
        USB_HILOGI(MODULE_USB_DEVICE, "register a disconnect task, id %{public}u, delaytime: %{public}d",
            delayDisconnTimerId_, delayTime);
    } else {
        USB_HILOGI(MODULE_USB_DEVICE, "else info cur status %{public}d, bconnected: %{public}d", status, connected_);
    }
}
#else
void UsbDeviceManager::HandleEvent(int32_t status)
{
    if (usbd_ == nullptr) {
        return;
    }
    bool curConnect = false;
    ProcessStatus(status, curConnect);
    UsbTimerWrapper::GetInstance()->Unregister(delayDisconnTimerId_);
    if (curConnect && (connected_ != curConnect)) {
        auto task = [&]() {
            USB_HILOGI(MODULE_USB_DEVICE, "execute connect task:%{public}d", currentFunctions_);
            connected_ = true;
            GetCurrentFunctions(currentFunctions_);
            ProcessFuncChange(connected_, currentFunctions_);
        };
        delayDisconnTimerId_ = UsbTimerWrapper::GetInstance()->Register(task, DELAY_CONNECT_INTERVAL, true);
        USB_HILOGI(MODULE_USB_DEVICE, "register a connect task, id %{public}u", delayDisconnTimerId_,);
    } else if (!curConnect && (connected_ != curConnect)) {
        auto task = [&]() {
            USB_HILOGI(MODULE_USB_DEVICE, "execute disconnect task:%{public}d", currentFunctions_);
            connected_ = false;
            isDisableDialog_ = false;
            if ((static_cast<uint32_t>(currentFunctions_) & USB_FUNCTION_MTP) != 0 ||
                (static_cast<uint32_t>(currentFunctions_) & USB_FUNCTION_PTP) != 0) {
                currentFunctions_ = static_cast<uint32_t>(currentFunctions_) &
                    (~USB_FUNCTION_MTP) & (~USB_FUNCTION_PTP);
                USB_HILOGI(MODULE_USB_DEVICE, "usb function reset %{public}d", currentFunctions_);
                currentFunctions_ = currentFunctions_ == 0 ? USB_FUNCTION_STORAGE : currentFunctions_;
                usbd_->SetCurrentFunctions(currentFunctions_);
            } else if ((static_cast<uint32_t>(currentFunctions_) & USB_FUNCTION_DEVMODE_AUTH) != 0) {
                currentFunctions_ = USB_FUNCTION_STORAGE;
                usbd_->SetCurrentFunctions(currentFunctions_);
            }
            ProcessFuncChange(connected_, currentFunctions_);
            return;
        };
        int32_t delayTime = DELAY_DISCONN_INTERVAL;
        if (phyConnect_) {
            delayTime += DELAY_DISCONN_INTERVAL;
            USB_HILOGI(MODULE_USB_DEVICE, "Physical is plug");
        }
        delayDisconnTimerId_ = UsbTimerWrapper::GetInstance()->Register(task, delayTime, true);
        USB_HILOGI(MODULE_USB_DEVICE, "register a disconnect task, id %{public}u, delaytime: %{public}d",
            delayDisconnTimerId_, delayTime);
    } else {
        USB_HILOGI(MODULE_USB_DEVICE, "else info cur status %{public}d, bconnected: %{public}d", status, connected_);
    }
}
#endif // USB_MANAGER_V2_0

int32_t UsbDeviceManager::UserChangeProcess()
{
    USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: in", __func__);
    int32_t ret = HDF_FAILURE;
    if ((static_cast<uint32_t>(currentFunctions_) & USB_FUNCTION_MTP) != 0 ||
        (static_cast<uint32_t>(currentFunctions_) & USB_FUNCTION_PTP) != 0) {
        uint32_t func = static_cast<uint32_t>(currentFunctions_) & (~USB_FUNCTION_MTP) & (~USB_FUNCTION_PTP);
        func = func == 0 ? USB_FUNCTION_STORAGE : func;
        USB_HILOGI(MODULE_USB_DEVICE, "usb function reset %{public}d", func);
        int32_t funcs = static_cast<int32_t>(func);
#ifdef USB_MANAGER_V2_0
        ret = usbDeviceInterface_->SetCurrentFunctions(funcs);
#else
        ret = usbd_->SetCurrentFunctions(funcs);
#endif // USB_MANAGER_V2_0
        if (ret == ERR_OK && funcs != currentFunctions_) {
            currentFunctions_ = funcs;
            ReportFuncChangeSysEvent(currentFunctions_, funcs);
            BroadcastFuncChange(connected_, currentFunctions_);
        }
    }
    ProcessFunctionNotifier(connected_, currentFunctions_);
    return ret;
}

void UsbDeviceManager::BroadcastFuncChange(bool connected, int32_t currentFunc)
{
    USB_HILOGI(MODULE_USB_DEVICE, "Current Connect %{public}d,bconnected: %{public}d", connected, currentFunc);
    CommonEventManager::RemoveStickyCommonEvent(CommonEventSupport::COMMON_EVENT_USB_STATE);
    Want want;
    want.SetAction(CommonEventSupport::COMMON_EVENT_USB_STATE);
 
    want.SetParam(std::string {UsbSrvSupport::CONNECTED}, connected);
    uint32_t remainderFunc = static_cast<uint32_t>(currentFunc);
    // start from bit 1
    uint32_t bit = 1;
    while (remainderFunc != 0) {
        if (remainderFunc & bit) {
            want.SetParam(ConvertToString(bit), true);
            // set current bit to zero
            remainderFunc &= ~bit;
        }
        // 1 means to next bit
        bit = bit << 1;
    }
    CommonEventData data(want);
    CommonEventPublishInfo publishInfo;
    publishInfo.SetSticky(true);
    USB_HILOGI(MODULE_USB_DEVICE, "send COMMON_EVENT_USB_STATE broadcast connected:%{public}d, "
        "currentFunctions:%{public}d", connected, currentFunc);
    CommonEventManager::PublishCommonEvent(data, publishInfo);
    ReportDevicePlugSysEvent(currentFunc, connected);
}

void UsbDeviceManager::ProcessFuncChange(bool connected, int32_t currentFunc)
{
    USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: connected %{public}d, currentFunc %{public}d, isDisableDialog "
                                   "%{public}d", __func__, connected, currentFunc, isDisableDialog_);
    BroadcastFuncChange(connected, currentFunc);
    if (!isDisableDialog_) {
        ProcessFunctionSwitchWindow(connected);
    }
    ProcessFunctionNotifier(connected, currentFunc);
}

void UsbDeviceManager::SetChargeFlag(bool isReverseCharge)
{
    isReverseCharge_ = isReverseCharge;
}

void UsbDeviceManager::ProcessFunctionNotifier(bool connected, int32_t func)
{
    USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: connected %{public}d, func %{public}d, hasHdcNotifier_ %{public}d",
               __func__, connected, func, hasHdcNotifier_);
    uint32_t func_uint = static_cast<uint32_t>(func);
    if (connected) {
        if (func_uint & USB_FUNCTION_MTP) {
            UsbConnectionNotifier::GetInstance()->SendNotification(USB_FUNC_MTP);
        } else if (func_uint & USB_FUNCTION_PTP) {
            UsbConnectionNotifier::GetInstance()->SendNotification(USB_FUNC_PTP);
        } else if (isReverseCharge_) {
            UsbConnectionNotifier::GetInstance()->SendNotification(USB_FUNC_REVERSE_CHARGE);
        } else {
            UsbConnectionNotifier::GetInstance()->SendNotification(USB_FUNC_CHARGE);
        }

        if ((func_uint & USB_FUNCTION_HDC) && !hasHdcNotifier_) {
            UsbConnectionNotifier::GetInstance()->SendHdcNotification();
            hasHdcNotifier_ = true;
        }
        if (!(func_uint & USB_FUNCTION_HDC) && hasHdcNotifier_) {
            UsbConnectionNotifier::GetInstance()->CancelHdcNotification();
            hasHdcNotifier_ = false;
        }
    } else {
        UsbConnectionNotifier::GetInstance()->CancelNotification(false);
        if (hasHdcNotifier_) {
            UsbConnectionNotifier::GetInstance()->CancelHdcNotification();
            hasHdcNotifier_ = false;
        }
    }
}

void UsbDeviceManager::ProcessFunctionSwitchWindow(bool connected)
{
    USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: connected %{public}d", __func__, connected);
    std::shared_ptr<UsbFunctionSwitchWindow> window_ = UsbFunctionSwitchWindow::GetInstance();
    if (window_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "show window: get usb function switch window failed");
        return;
    }

    if (connected) {
        USB_HILOGD(MODULE_USB_DEVICE, "start pop up usb service switch window");
        if (!window_->PopUpFunctionSwitchWindow()) {
            USB_HILOGE(MODULE_USB_DEVICE, "start pop up usb service switch window failed");
        }
    } else {
        USB_HILOGD(MODULE_USB_DEVICE, "start dismiss usb service switch window");
        if (!window_->DismissFunctionSwitchWindow()) {
            USB_HILOGE(MODULE_USB_DEVICE, "start dismiss usb service switch window failed");
        }
    }
}

void UsbDeviceManager::GetDumpHelp(int32_t fd)
{
    dprintf(fd, "========= dump the all device function =========\n");
    dprintf(fd, "usb_device -a:    Query all function\n");
    dprintf(fd, "usb_device -f Q:  Query Current function\n");
#ifdef USB_MANAGER_HIDUMPER_SET
    dprintf(fd, "usb_device -f 0:  Switch to function:none\n");
    dprintf(fd, "usb_device -f 1:  Switch to function:acm\n");
    dprintf(fd, "usb_device -f 2:  Switch to function:ecm\n");
    dprintf(fd, "usb_device -f 3:  Switch to function:acm&ecm\n");
    dprintf(fd, "usb_device -f 4:  Switch to function:hdc\n");
    dprintf(fd, "usb_device -f 5:  Switch to function:acm&hdc\n");
    dprintf(fd, "usb_device -f 6:  Switch to function:ecm&hdc\n");
    dprintf(fd, "usb_device -f 32: Switch to function:rndis\n");
    dprintf(fd, "usb_device -f 512:Switch to function:storage\n");
    dprintf(fd, "usb_device -f 36: Switch to function:rndis&hdc\n");
    dprintf(fd, "usb_device -f 516:Switch to function:storage&hdc\n");
#endif // USB_MANAGER_HIDUMPER_SET
    dprintf(fd, "------------------------------------------------\n");
}

void UsbDeviceManager::DumpGetSupportFunc(int32_t fd)
{
    dprintf(fd, "Usb Device function list info:\n");
    dprintf(fd, "current function: %s\n", ConvertToString(currentFunctions_).c_str());
    dprintf(fd, "supported functions list: %s\n", ConvertToString(functionSettable_).c_str());
}

bool StringToInteger(const std::string &str, int32_t &result)
{
    char *endptr = nullptr;
    errno = 0;
    int64_t number = std::strtol(str.c_str(), &endptr, DECIMAL_BASE);
    if (errno != 0 || number < INT32_MIN || number > INT32_MAX) {
        USB_HILOGE(MODULE_USB_DEVICE, "number is out of range");
        return false;
    }
    if (endptr == nullptr || endptr == str.c_str() || *endptr != '\0') {
        USB_HILOGE(MODULE_USB_DEVICE, "conversion failed");
        return false;
    }
    result = static_cast<int32_t>(number);
    return true;
}

#ifdef USB_MANAGER_V2_0
void UsbDeviceManager::DumpSetFunc(int32_t fd, const std::string &args)
{
    int32_t currentFunction;
    int32_t ret;
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::DumpSetFunc usbDeviceInterface_ is nullptr");
        return;
    }
    if (args.compare("Q") == 0) {
        ret = usbDeviceInterface_->GetCurrentFunctions(currentFunction);
        if (ret != UEC_OK) {
            dprintf(fd, "GetCurrentFunctions failed: %d\n", ret);
            return;
        }
        dprintf(fd, "current function: %s\n", ConvertToString(currentFunction).c_str());
        return;
    }
#ifdef USB_MANAGER_HIDUMPER_SET
    if (!std::regex_match(args, std::regex("^[0-9]+$"))) {
        dprintf(fd, "Invalid input, please enter a valid integer\n");
        GetDumpHelp(fd);
        return;
    }
    int32_t mode;
    if (!StringToInteger(args, mode)) {
        dprintf(fd, "Invalid input, the number is out of range\n");
        GetDumpHelp(fd);
        return;
    }
    ret = usbDeviceInterface_->SetCurrentFunctions(mode);
    if (ret != UEC_OK) {
        dprintf(fd, "SetCurrentFunctions failed");
        return;
    }
    ret = usbDeviceInterface_->GetCurrentFunctions(currentFunction);
    if (ret != UEC_OK) {
        dprintf(fd, "GetCurrentFunctions failed: %d\n", ret);
        return;
    }

    dprintf(fd, "current function: %s\n", ConvertToString(currentFunction).c_str());
#else
    dprintf(fd, "Invalid input, please enter a valid argument\n");
    GetDumpHelp(fd);
#endif // USB_MANAGER_HIDUMPER_SET
}
#else
void UsbDeviceManager::DumpSetFunc(int32_t fd, const std::string &args)
{
    int32_t currentFunction;
    int32_t ret;
    if (usbd_ == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "UsbDeviceManager::DumpSetFunc usbd_ is nullptr");
        return;
    }
    if (args.compare("Q") == 0) {
        ret = usbd_->GetCurrentFunctions(currentFunction);
        if (ret != UEC_OK) {
            dprintf(fd, "GetCurrentFunctions failed: %d\n", ret);
            return;
        }
        dprintf(fd, "current function: %s\n", ConvertToString(currentFunction).c_str());
        return;
    }
#ifdef USB_MANAGER_HIDUMPER_SET
    if (!std::regex_match(args, std::regex("^[0-9]+$"))) {
        dprintf(fd, "Invalid input, please enter a valid integer\n");
        GetDumpHelp(fd);
        return;
    }
    int32_t mode;
    if (!StringToInteger(args, mode)) {
        dprintf(fd, "Invalid input, the number is out of range\n");
        GetDumpHelp(fd);
        return;
    }
    ret = usbd_->SetCurrentFunctions(mode);
    if (ret != UEC_OK) {
        dprintf(fd, "SetCurrentFunctions failed");
        return;
    }
    ret = usbd_->GetCurrentFunctions(currentFunction);
    if (ret != UEC_OK) {
        dprintf(fd, "GetCurrentFunctions failed: %d\n", ret);
        return;
    }
    dprintf(fd, "current function: %s\n", ConvertToString(currentFunction).c_str());
#else
    dprintf(fd, "Invalid input, please enter a valid argument\n");
    GetDumpHelp(fd);
#endif // USB_MANAGER_HIDUMPER_SET
}
#endif // USB_MANAGER_V2_0

void UsbDeviceManager::Dump(int32_t fd, const std::vector<std::string> &args)
{
    if (args.size() < PARAM_COUNT_TWO || args.size() > PARAM_COUNT_THR) {
        GetDumpHelp(fd);
        return;
    }

    if (args[CMD_INDEX] == "-a" && args.size() == PARAM_COUNT_TWO) {
        DumpGetSupportFunc(fd);
    } else if (args[CMD_INDEX] == "-f" && args.size() == PARAM_COUNT_THR) {
        DumpSetFunc(fd, args[PARAM_INDEX]);
    } else {
        dprintf(fd, "func param error, please enter again\n");
        GetDumpHelp(fd);
    }
}

void UsbDeviceManager::ReportUsbOperationFaultSysEvent(const std::string &operationType, int32_t failReason,
    const std::string &failDescription)
{
    HiSysEventWrite(HiSysEvent::Domain::USB, "OPERATION_FAULT",
        HiSysEvent::EventType::FAULT, "OPERATION_TYPE_NAME", operationType,
        "FAIL_REASON", failReason,
        "FAIL_DESCRIPTION", failDescription);
}

void UsbDeviceManager::ReportFuncChangeSysEvent(int32_t currentFunctions, int32_t updateFunctions)
{
    USB_HILOGI(MODULE_USB_DEVICE, "Device function Indicates the switch point information:");
    HiSysEventWrite(HiSysEvent::Domain::USB, "FUNCTION_CHANGED",
        HiSysEvent::EventType::BEHAVIOR, "CURRENT_FUNCTION",
        currentFunctions_, "UPDATE_FUNCTION", updateFunctions);
}

void UsbDeviceManager::ReportDevicePlugSysEvent(int32_t currentFunctions, bool connected)
{
    USB_HILOGI(MODULE_USB_DEVICE, "Device mode Indicates the insertion and removal information:");
    HiSysEventWrite(HiSysEvent::Domain::USB, "PLUG_IN_OUT_DEVICE_MODE",
        HiSysEvent::EventType::BEHAVIOR, "CURRENT_FUNCTIONS",
        currentFunctions, "CONNECTED", connected);
}
bool UsbDeviceManager::IsGadgetConnected(void)
{
    return gadgetConnected_;
}
} // namespace USB
} // namespace OHOS
