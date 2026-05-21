/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "usb_function_switch_window.h"

#include <parameters.h>
#include <param_wrapper.h>
#include <semaphore.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include "cJSON.h"

#include "ability_manager_client.h"
#include "bundle_mgr_client.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "usb_settings_datashare.h"
#include "os_account_manager.h"
#include "usb_errors.h"
#include "uri.h"

#define DEFAULT_PARAM_VALUE "charge,mtp,ptp"
using namespace OHOS::AppExecFwk;
using namespace OHOS::EventFwk;

namespace OHOS {
namespace USB {
constexpr int32_t INVALID_USERID = -1;
constexpr int32_t MESSAGE_PARCEL_KEY_SIZE = 3;
constexpr int32_t MAX_RETRY_TIMES = 30;
constexpr int32_t RETRY_INTERVAL_SECONDS = 1;
constexpr uint32_t DELAY_CHECK_DIALOG = 1;

class FuncSwitchSubscriber : public CommonEventSubscriber {
public:
    explicit FuncSwitchSubscriber(const CommonEventSubscribeInfo &sp) : CommonEventSubscriber(sp) {}

    void OnReceiveEvent(const CommonEventData &data) override
    {
        auto &want = data.GetWant();
        std::string wantAction = want.GetAction();
        if (wantAction == CommonEventSupport::COMMON_EVENT_BUNDLE_SCAN_FINISHED) {
            UsbFunctionSwitchWindow::GetInstance()->CheckDialogInstallStatus();
        }
    }
};

std::shared_ptr<UsbFunctionSwitchWindow> UsbFunctionSwitchWindow::instance_;
std::mutex UsbFunctionSwitchWindow::insMutex_;

std::shared_ptr<UsbFunctionSwitchWindow> UsbFunctionSwitchWindow::GetInstance()
{
    std::lock_guard<std::mutex> guard(insMutex_);
    if (instance_ == nullptr) {
        USB_HILOGI(MODULE_USB_DEVICE, "reset to new instance");
        instance_.reset(new UsbFunctionSwitchWindow());
    }
    return instance_;
}

UsbFunctionSwitchWindow::UsbFunctionSwitchWindow() {}

UsbFunctionSwitchWindow::~UsbFunctionSwitchWindow()
{
    if (windowAction_ == UsbFunctionSwitchWindowAction::FUNCTION_SWITCH_WINDOW_ACTION_SHOW) {
        (void)UnShowFunctionSwitchWindow();
    }
}

void UsbFunctionSwitchWindow::SubscribeCommonEvent()
{
    USB_HILOGI(MODULE_USB_DEVICE, "subscriber bms scan finished.");
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_BUNDLE_SCAN_FINISHED);
    CommonEventSubscribeInfo subscriberInfo(matchingSkills);
    std::shared_ptr<FuncSwitchSubscriber> subscriber = std::make_shared<FuncSwitchSubscriber>(subscriberInfo);
    bool ret = CommonEventManager::SubscribeCommonEvent(subscriber);
    if (!ret) {
        USB_HILOGW(MODULE_USB_DEVICE, "subscriber event failed.");
    }
}

int32_t UsbFunctionSwitchWindow::Init()
{
    USB_HILOGI(MODULE_USB_DEVICE, "init: window action=%{public}d,%{public}d", windowAction_, isDialogInstalled_);
    if (isDialogInstalled_) {
        return UEC_OK;
    }

    checkDialogTimer_.Unregister(checkDialogTimerId_);
    checkDialogTimer_.Shutdown();
    // async check dialog install status
    auto task = [this]() {
        bool checkRet = CheckDialogInstallStatus();
        if (!checkRet) {
            SubscribeCommonEvent();
        }
        checkDialogTimer_.Unregister(checkDialogTimerId_);
        checkDialogTimer_.Shutdown(false);
        USB_HILOGI(MODULE_USB_DEVICE, "dialog check end");
    };
    auto ret = checkDialogTimer_.Setup();
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "set up timer failed %{public}u", ret);
        // fall back to sync
        bool checkRet = CheckDialogInstallStatus();
        if (!checkRet) {
            SubscribeCommonEvent();
        }
        return isDialogInstalled_ ? UEC_OK : ret;
    }
    checkDialogTimerId_ = checkDialogTimer_.Register(task, DELAY_CHECK_DIALOG, true);
    return UEC_OK;
}

bool UsbFunctionSwitchWindow::PopUpFunctionSwitchWindow()
{
    USB_HILOGI(MODULE_USB_DEVICE, "pop up function switch window");
    bool isPromptEnabled = OHOS::system::GetBoolParameter("persist.usb.setting.gadget_conn_prompt", true);
    if (!isPromptEnabled) {
        USB_HILOGE(MODULE_USB_DEVICE, "gadget_conn_prompt is false");
        return false;
    }
    bool isTempDisablePrompt = OHOS::system::GetBoolParameter("usb.setting.gadget_conn_prompt", true);
    if (!isTempDisablePrompt) {
        USB_HILOGE(MODULE_USB_DEVICE, "temporarily close the pop up window");
        if (!OHOS::system::SetParameter("usb.setting.gadget_conn_prompt", "true")) {
            USB_HILOGE(MODULE_USB_DEVICE, "set parameter failed");
        }
        return false;
    }
    int32_t supportedFuncs = GetSupportedFunctions();
    USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: supportedFuncs %{public}d", __func__, supportedFuncs);
    if (supportedFuncs < 0) {
        USB_HILOGE(MODULE_USB_DEVICE, "no supported functions: %{public}d", supportedFuncs);
        return false;
    }

    std::lock_guard<std::mutex> guard(opMutex_);
    if (windowAction_ == UsbFunctionSwitchWindowAction::FUNCTION_SWITCH_WINDOW_ACTION_FORBID) {
        USB_HILOGI(MODULE_USB_DEVICE, "forbid: pop up function switch window");
        return false;
    }
    windowAction_ = UsbFunctionSwitchWindowAction::FUNCTION_SWITCH_WINDOW_ACTION_SHOW;
 
    isPromptEnabled = OHOS::system::GetBoolParameter("bootevent.boot.completed", false);
    if (!isPromptEnabled) {
        USB_HILOGE(MODULE_USB_DEVICE, "boot.completed is false!");
        int ret = WatchParameter("bootevent.boot.completed", BootCompletedEventCallback, this);
        if (ret != 0) {
            USB_HILOGI(MODULE_USB_DEVICE, "watchParameter is failed!");
        }
        return false;
    }
    if (ShouldRejectShowWindow()) {
        USB_HILOGE(MODULE_USB_DEVICE, "OOBE is not ready!");
    }
    return ShowFunctionSwitchWindow();
}

bool UsbFunctionSwitchWindow::DismissFunctionSwitchWindow()
{
    USB_HILOGI(MODULE_USB_DEVICE, "dismiss function switch window");
    std::lock_guard<std::mutex> guard(opMutex_);
    if (windowAction_ == UsbFunctionSwitchWindowAction::FUNCTION_SWITCH_WINDOW_ACTION_FORBID) {
        USB_HILOGI(MODULE_USB_DEVICE, "forbid: dismiss function switch window");
        return false;
    }
    windowAction_ = UsbFunctionSwitchWindowAction::FUNCTION_SWITCH_WINDOW_ACTION_DISMISS;
    return UnShowFunctionSwitchWindow();
}

int32_t UsbFunctionSwitchWindow::GetSupportedFunctions()
{
    std::string supportedFuncStr = "";
    (void)OHOS::system::GetStringParameter("const.usb_manager.supported_functions",
        supportedFuncStr, DEFAULT_PARAM_VALUE);
    USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: supportedFuncStr %{public}s", __func__, supportedFuncStr.c_str());

    if (supportedFuncStr.find("none") != std::string::npos) {
        return SUPPORTED_FUNC_NONE;
    }
    uint32_t mtp = supportedFuncStr.find("mtp") != std::string::npos ? SUPPORTED_FUNC_MTP : 0;
    uint32_t ptp = supportedFuncStr.find("ptp") != std::string::npos ? SUPPORTED_FUNC_PTP : 0;

    return static_cast<int32_t>(mtp | ptp);
}

void UsbFunctionSwitchWindow::UsbFuncAbilityConn::OnAbilityConnectDone(const AppExecFwk::ElementName &element,
    const sptr<IRemoteObject> &remoteObject, int32_t resultCode)
{
    USB_HILOGI(MODULE_USB_DEVICE, "OnAbilityConnectDone");
    if (remoteObject == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "remoteObject is nullptr");
        return;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (isAbortDialog_) {
        USB_HILOGI(MODULE_USB_DEVICE, "abort function switch window");
        const uint32_t cmdCode = 3;
        int32_t ret = remoteObject_->SendRequest(cmdCode, data, reply, option);
        if (ret != ERR_OK) {
            USB_HILOGE(MODULE_USB_DEVICE, "abort dialog failed: %{public}d", ret);
        }
        return;
    }
    data.WriteInt32(MESSAGE_PARCEL_KEY_SIZE);
    data.WriteString16(u"bundleName");
    data.WriteString16(u"com.usb.right");
    data.WriteString16(u"abilityName");
    data.WriteString16(u"UsbFunctionSwitchExtAbility");
    data.WriteString16(u"parameters");
    std::string paramStr;
    PrepareJson(paramStr);
    if (paramStr.size() == 0) {
        USB_HILOGE(MODULE_USB_DEVICE, "Print paramJson error");
        return;
    }
    data.WriteString16(Str8ToStr16(paramStr));

    const uint32_t cmdCode = 1;
    int32_t ret = remoteObject->SendRequest(cmdCode, data, reply, option);
    if (ret != ERR_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "send request failed: %{public}d", ret);
        return;
    }
    if (!reply.ReadInt32(ret) || ret != ERR_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "show dialog failed: %{public}d", ret);
        return;
    }
    std::lock_guard<std::mutex> guard(remoteMutex_);
    remoteObject_ = remoteObject;
    return;
}

void UsbFunctionSwitchWindow::UsbFuncAbilityConn::PrepareJson(std::string &jsonStr)
{
    cJSON* paramJson = cJSON_CreateObject();
    int32_t supportedFuncs = GetSupportedFunctions();
    USB_HILOGI(MODULE_USB_DEVICE, "%{public}s: supportedFuncs %{public}d", __func__, supportedFuncs);
    cJSON_AddStringToObject(paramJson, "supportedFuncs", std::to_string(supportedFuncs).c_str());
    std::string uiExtensionTypeStr = "sysDialog/common";
    cJSON_AddStringToObject(paramJson, "ability.want.params.uiExtensionType", uiExtensionTypeStr.c_str());
    char *pParamJson = cJSON_PrintUnformatted(paramJson);
    cJSON_Delete(paramJson);
    paramJson = nullptr;
    if (!pParamJson) {
        jsonStr = "";
        return;
    }
    jsonStr = std::string(pParamJson);
    cJSON_free(pParamJson);
    pParamJson = NULL;
    return;
}

void UsbFunctionSwitchWindow::UsbFuncAbilityConn::OnAbilityDisconnectDone(
    const AppExecFwk::ElementName& element, int resultCode)
{
    USB_HILOGI(MODULE_USB_DEVICE, "OnAbilityDisconnectDone");
    std::lock_guard<std::mutex> guard(remoteMutex_);
    remoteObject_ = nullptr;
    USB_HILOGI(MODULE_USB_DEVICE, "UsbFuncAbilityConn remoteObject_ freed");
    return;
}

void UsbFunctionSwitchWindow::UsbFuncAbilityConn::CloseDialog()
{
    USB_HILOGI(MODULE_USB_DEVICE, "UsbFuncAbilityConn CloseDialog enter");
    isAbortDialog_ = true;
    std::lock_guard<std::mutex> guard(remoteMutex_);
    if (remoteObject_ == nullptr) {
        USB_HILOGW(MODULE_USB_DEVICE, "CloseDialog: disconnected");
        return;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    const uint32_t cmdCode = 3;
    int32_t ret = remoteObject_->SendRequest(cmdCode, data, reply, option);
    int32_t replyCode = -1;
    bool success = false;
    if (ret == ERR_OK) {
        success = reply.ReadInt32(replyCode);
    }
    USB_HILOGI(MODULE_USB_DEVICE, "CloseDialog: ret=%{public}d, %{public}d, %{public}d", ret, success, replyCode);
}

void UsbFunctionSwitchWindow::UsbFuncAbilityConn::ReopenDialog()
{
    USB_HILOGI(MODULE_USB_DEVICE, "UsbFuncAbilityConn ReopenDialog enter");
    isAbortDialog_ = false;
}

bool UsbFunctionSwitchWindow::ShowFunctionSwitchWindow()
{
    USB_HILOGI(MODULE_USB_DEVICE, "show function switch window right now, installed: %{public}d", isDialogInstalled_);
    if (!isDialogInstalled_) {
        return false;
    }

    if (usbFuncAbilityConn == nullptr) {
        usbFuncAbilityConn = sptr<UsbFuncAbilityConn>(new (std::nothrow) UsbFuncAbilityConn());
    }
    usbFuncAbilityConn->ReopenDialog();

    auto abilityManager = AAFwk::AbilityManagerClient::GetInstance();
    if (abilityManager == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "AbilityManagerClient is nullptr");
        return false;
    }

    AAFwk::Want want;
    want.SetElementName("com.ohos.sceneboard", "com.ohos.sceneboard.systemdialog");
    auto ret = abilityManager->ConnectAbility(want, usbFuncAbilityConn, INVALID_USERID);
    if (ret != ERR_OK) {
        want.SetElementName("com.ohos.systemui", "com.ohos.systemui.dialog");
        ret = abilityManager->ConnectAbility(want, usbFuncAbilityConn, INVALID_USERID);
        if (ret != ERR_OK) {
            USB_HILOGE(MODULE_USB_DEVICE, "ConnectServiceExtensionAbility systemui failed, ret: %{public}d", ret);
            usbFuncAbilityConn = nullptr;
            return false;
        }
    }
    USB_HILOGI(MODULE_USB_DEVICE, "StartAbility success, ret: %{public}d", ret);
    return true;
}

bool UsbFunctionSwitchWindow::UnShowFunctionSwitchWindow()
{
    if (usbFuncAbilityConn == nullptr) {
        return true;
    }

    auto abmc = AAFwk::AbilityManagerClient::GetInstance();
    if (abmc == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "GetInstance failed");
        return false;
    }
    USB_HILOGI(MODULE_USB_DEVICE, "unshow function switch window");
    usbFuncAbilityConn->CloseDialog();

    auto ret = abmc->DisconnectAbility(usbFuncAbilityConn);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_DEVICE, "DisconnectAbility failed %{public}d", ret);
        return false;
    }
    USB_HILOGD(MODULE_USB_DEVICE, "unshow function switch window success");
    return true;
}

bool UsbFunctionSwitchWindow::CheckDialogInstallStatus()
{
    AppExecFwk::BundleInfo info;
    AppExecFwk::BundleMgrClient bmc;
    int32_t retryTimes = 0;
    while (retryTimes < MAX_RETRY_TIMES) {
        isDialogInstalled_ = bmc.GetBundleInfo(functionSwitchBundleName_,
            AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, info, AppExecFwk::Constants::ALL_USERID);
        USB_HILOGI(MODULE_USB_DEVICE, "check dialog, times=%{public}d,res=%{public}d", retryTimes, isDialogInstalled_);
        if (!isDialogInstalled_) {
            retryTimes++;
            sleep(RETRY_INTERVAL_SECONDS);
            continue;
        }

        if (windowAction_ == UsbFunctionSwitchWindowAction::FUNCTION_SWITCH_WINDOW_ACTION_SHOW) {
            if (OHOS::system::GetBoolParameter("bootevent.boot.completed", false)) {
                ShowFunctionSwitchWindow();
            }
        }
        return true;
    }
    USB_HILOGE(MODULE_USB_DEVICE, "dialog is not installed");
    return false;
}

void UsbFunctionSwitchWindow::BootCompletedEventCallback(const char *key, const char *value, void *context)
{
    USB_HILOGI(MODULE_USB_DEVICE, "testParameterChange key: %{public}s, value: %{public}s!", key, value);
    if (!OHOS::system::GetBoolParameter("bootevent.boot.completed", false)) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: boot.completed is false!", __func__);
        return;
    }
    if (context == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: context is null!", __func__);
        return;
    }
    auto eventSwitchWindow = reinterpret_cast<UsbFunctionSwitchWindow*>(context);
    if (eventSwitchWindow == nullptr) {
        USB_HILOGE(MODULE_USB_DEVICE, "get eventSwitchWindow is null!");
        return;
    }

    if (eventSwitchWindow->ShouldRejectShowWindow()) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: OOBE is not ready!", __func__);
    }
    bool ret = eventSwitchWindow->ShowWindowIfConnected();
    if (!ret) {
        USB_HILOGE(MODULE_USB_DEVICE, "watchParameter to ShowWindowIfConnected is failed!");
    }
}

bool UsbFunctionSwitchWindow::ShowWindowIfConnected()
{
    std::lock_guard<std::mutex> guard(opMutex_);
    if (windowAction_ == UsbFunctionSwitchWindowAction::FUNCTION_SWITCH_WINDOW_ACTION_SHOW) {
        if (!ShowFunctionSwitchWindow()) {
            USB_HILOGE(MODULE_USB_DEVICE, "%{public}s:ShowFunctionSwitchWindow is failed!", __func__);
            return false;
        }
        return true;
    }
    USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: windowAction_ is not FUNCTION_SWITCH_WINDOW_ACTION_SHOW", __func__);
    return false;
}

bool UsbFunctionSwitchWindow::ShouldRejectShowWindow()
{
    auto datashareHelper = std::make_shared<UsbSettingDataShare>();
    std::string device_provisioned {"0"};
    OHOS::Uri uri(
        "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true&key=device_provisioned");
    bool resp = datashareHelper->OobeQuery(uri, "device_provisioned", device_provisioned);
    if (resp && device_provisioned != "1") {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: device_provisioned is = 0", __func__);
        return true;
    }
 
    std::string user_setup_complete {"0"};
    std::vector<int> activedOsAccountIds;
    OHOS::AccountSA::OsAccountManager::QueryActiveOsAccountIds(activedOsAccountIds);
    if (activedOsAccountIds.empty()) {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: activedOsAccountIds is empty", __func__);
        return true;
    }
    int userId = activedOsAccountIds[0];
    OHOS::Uri uri_setup(
        "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_SECURE_"
        + std::to_string(userId) + "?Proxy=true&key=user_setup_complete");
    bool resp_userSetup = datashareHelper->OobeQuery(uri_setup, "user_setup_complete", user_setup_complete);
    if (resp_userSetup && user_setup_complete != "1") {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: user_setup_complete is = 0", __func__);
        return true;
    }
 
    std::string is_ota_finished {"0"};
    OHOS::Uri uri_ota(
        "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_SECURE_"
        + std::to_string(userId) + "?Proxy=true&key=is_ota_finished");
    bool resp_ota = datashareHelper->OobeQuery(uri_ota, "is_ota_finished", is_ota_finished);
    if (resp_ota && is_ota_finished == "0") {
        USB_HILOGE(MODULE_USB_DEVICE, "%{public}s: is_ota_finished is = 0", __func__);
        return true;
    }
    return false;
}
} // namespace USB
} // namespace OHOS
