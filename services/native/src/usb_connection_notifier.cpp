/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
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
#include "usb_connection_notifier.h"

#include <parameters.h>
#include <param_wrapper.h>
#include "hilog_wrapper.h"
#include "want_agent_info.h"
#include "want_agent_helper.h"
#include "notification_normal_content.h"
#include "resource_manager.h"
#include "locale_config.h"
#include "locale_info.h"
#include "string_wrapper.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "image_source.h"
#include "usb_errors.h"
#include "bundle_mgr_interface.h"
#include "os_account_manager.h"
#include "usb_trace.h"

#define DEFAULT_PARAM_VALUE "charge,mtp,ptp"
using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;

static const std::string NOTIFICATION_NAME = "usb_service";
static const std::string NOTIFICATION_NAME_ONE = "usb_service1";
static int32_t g_uid = 1018;
constexpr int32_t USER_ID = 100;
static const std::string BUNDLE_NAME = "com.usb.right";
static const std::string SET_BUNDLE_NAME = "com.usb.right";
static const std::string SETTING_BUNDLE_NAME = "com.ohos.settings";
static const std::string SETTING_BUNDLE_NAME_ABILITY = "com.ohos.settings.MainAbility";
static const std::string SETTING_BUNDLE_NAME_URL = "developer_options_settings";
static const std::string BUNDLE_NAME_KEY = "EntryAbility";
static const std::string URI = "";
static const std::string HAP_PATH = "system/app/com.usb.right/usb_right_dialog.hap";
static const std::string ICON_NAME = "icon";
constexpr std::int32_t BUNDLE_MGR_SERVICE_SA_ID = 401;
constexpr std::int32_t NOTIFICATION_ID_ZERO = 0;
constexpr std::int32_t NOTIFICATION_ID_ONE = 1;
constexpr std::uint32_t NOTIFICATION_STATUS_CLOSE_SOUND = 1 << 0;
constexpr std::uint32_t NOTIFICATION_STATUS_CLOSE_LOCKSCREEN = 1 << 1;
constexpr std::uint32_t NOTIFICATION_STATUS_CLOSE_BANNER = 1 << 2;
constexpr std::uint32_t NOTIFICATION_STATUS_CLOSE_LIGHT_SCREEN = 1 << 3;
constexpr std::uint32_t NOTIFICATION_STATUS_CLOSE_VIBRATION = 1 << 4;

enum SUPPORTED_FUNC : int32_t {
    SUPPORTED_FUNC_NONE = -1,
    SUPPORTED_FUNC_CHARGE = 0,
    SUPPORTED_FUNC_MTP = 8,
    SUPPORTED_FUNC_PTP = 16,
    SUPPORTED_FUNC_REVERSECHARGE = 1 << 30,
};

namespace OHOS {
namespace USB {
std::mutex UsbConnectionNotifier::insMutex_;
std::shared_ptr<UsbConnectionNotifier> UsbConnectionNotifier::instance_;

std::shared_ptr<UsbConnectionNotifier> UsbConnectionNotifier::GetInstance()
{
    std::lock_guard<std::mutex> guard(insMutex_);
    if (instance_ == nullptr) {
        USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: reset to new instance", __func__);
        instance_.reset(new UsbConnectionNotifier());
    }
    return instance_;
}

// LCOV_EXCL_START
sptr<IBundleMgr> UsbConnectionNotifier::GetBundleMgr(void)
{
    USB_TRACE;
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s get systemAbilityManager failed", __func__);
        return nullptr;
    }
    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SA_ID);
    if (remoteObject == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s get remoteObject failed", __func__);
        return nullptr;
    }
    sptr<IBundleMgr> bundleMgr = iface_cast<IBundleMgr>(remoteObject);
    if (bundleMgr == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s get bundleMgr failed", __func__);
        return nullptr;
    }
    return bundleMgr;
}

void UsbConnectionNotifier::GetSetUid()
{
    USB_TRACE;
    int32_t userId = -1;
    int32_t ret = OHOS::AccountSA::OsAccountManager::GetOsAccountLocalIdFromProcess(userId);
    if (ret != 0) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s GetOsAccountLocalIdFromProcess ret:%{public}d", __func__, ret);
        return;
    }
    sptr<AppExecFwk::IBundleMgr> bundleMgr = GetBundleMgr();
    if (bundleMgr == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s GetBundleMgr failed", __func__);
        return;
    }
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s GetUidByBundleName", __func__);
    g_uid = bundleMgr->GetUidByBundleName(SET_BUNDLE_NAME, USER_ID);
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s userId %{public}d, g_uid %{public}d", __func__, userId, g_uid);
}

UsbConnectionNotifier::UsbConnectionNotifier()
{
    USB_TRACE;
    GetHapIcon();

    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: end", __func__);
}

UsbConnectionNotifier::~UsbConnectionNotifier() {}

void UsbConnectionNotifier::GetHapIcon()
{
    USB_TRACE;
    std::shared_ptr<Global::Resource::ResourceManager> resourceManager(Global::Resource::CreateResourceManager(false));
    if (resourceManager == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "resourceManager is null");
        return;
    }
    if (!resourceManager->AddResource(HAP_PATH.c_str())) {
        USB_HILOGE(MODULE_USB_SERVICE, "AddResource failed");
        return;
    }
    size_t len = 0;
    std::unique_ptr<uint8_t[]> data;
    resourceManager->GetMediaDataByName(ICON_NAME.c_str(), len, data);
    Media::SourceOptions opts;
    uint32_t errorCode = 0;
    std::unique_ptr<Media::ImageSource> imageSource =
        Media::ImageSource::CreateImageSource(data.get(), len, opts, errorCode);
    Media::DecodeOptions decodeOpts;
    decodeOpts.desiredPixelFormat = Media::PixelFormat::BGRA_8888;
    if (imageSource) {
        auto pixelMapPtr = imageSource->CreatePixelMap(decodeOpts, errorCode);
        icon_ = std::shared_ptr<Media::PixelMap>(pixelMapPtr.release());
    }
    if (errorCode != 0 || !icon_) {
        USB_HILOGE(MODULE_USB_SERVICE, "Get icon failed");
    }
    return;
}

void UsbConnectionNotifier::GetHapString()
{
    USB_TRACE;
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s", __func__);
    std::shared_ptr<Global::Resource::ResourceManager> resourceManager(Global::Resource::CreateResourceManager(false));
    if (resourceManager == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s resourceManager is null", __func__);
        return;
    }
    std::unique_ptr<Global::Resource::ResConfig> resConfig(Global::Resource::CreateResConfig());
    if (resConfig == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s resConfig is null", __func__);
        return;
    }
    std::map<std::string, std::string> configs;
    OHOS::Global::I18n::LocaleInfo locale(Global::I18n::LocaleConfig::GetEffectiveLanguage(), configs);
    resConfig->SetLocaleInfo(locale.GetLanguage().c_str(), locale.GetScript().c_str(), locale.GetRegion().c_str());
    resourceManager->UpdateResConfig(*resConfig);
    if (!resourceManager->AddResource(HAP_PATH.c_str())) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s AddResource failed", __func__);
        return;
    }
    for (auto it : notifierMap) {
        std::string outValue;
        resourceManager->GetStringByName(it.first.c_str(), outValue);
        notifierMap[it.first] = outValue;
    }
    return;
}

void UsbConnectionNotifier::SetWantAgent(OHOS::Notification::NotificationRequest &request)
{
    USB_TRACE;
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s", __func__);
    auto want = std::make_shared<AAFwk::Want>();
    want->SetElementName(BUNDLE_NAME, BUNDLE_NAME_KEY);
    want->SetUri(URI);
    want->SetParam("usbSupportedFunctions", usbSupportedFunctions_);
    std::vector<std::shared_ptr<AAFwk::Want>> wants;
    wants.push_back(want);

    std::vector<AbilityRuntime::WantAgent::WantAgentConstant::Flags> flags;
    flags.push_back(AbilityRuntime::WantAgent::WantAgentConstant::Flags::CONSTANT_FLAG);

    AbilityRuntime::WantAgent::WantAgentInfo wantAgentInfo(
        0, AbilityRuntime::WantAgent::WantAgentConstant::OperationType::START_ABILITY, flags, wants, nullptr);
    auto wantAgent = AbilityRuntime::WantAgent::WantAgentHelper::GetWantAgent(wantAgentInfo);
    request.SetWantAgent(wantAgent);
}

void UsbConnectionNotifier::SetWantAgentHdc(OHOS::Notification::NotificationRequest &request)
{
    USB_TRACE;
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s", __func__);
    auto want = std::make_shared<AAFwk::Want>();
    want->SetElementName(SETTING_BUNDLE_NAME, SETTING_BUNDLE_NAME_ABILITY);
    want->SetUri(SETTING_BUNDLE_NAME_URL);
    std::vector<std::shared_ptr<AAFwk::Want>> wants;
    wants.push_back(want);

    std::vector<AbilityRuntime::WantAgent::WantAgentConstant::Flags> flags;
    flags.push_back(AbilityRuntime::WantAgent::WantAgentConstant::Flags::CONSTANT_FLAG);

    AbilityRuntime::WantAgent::WantAgentInfo wantAgentInfo(
        0, AbilityRuntime::WantAgent::WantAgentConstant::OperationType::START_ABILITY, flags, wants, nullptr);
    auto wantAgent = AbilityRuntime::WantAgent::WantAgentHelper::GetWantAgent(wantAgentInfo);
    request.SetWantAgent(wantAgent);
}

void UsbConnectionNotifier::SendNotification(std::string func)
{
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: func %{public}s", __func__, func.c_str());
    std::lock_guard<std::mutex> guard(functionMutex_);

    usbSupportedFunctions_ = GetSupportedFunctions();
    HITRACE_METER_FMT(HITRACE_TAG_HDF, "%s func %s usbSupportedFunctions_ %d", __func__, func.c_str(),
        usbSupportedFunctions_);

    if (usbSupportedFunctions_ <= 0) {
        return;
    }

    uint32_t funcBit = 0;
    if (func == USB_FUNC_CHARGE) {
        funcBit = SUPPORTED_FUNC_MTP | SUPPORTED_FUNC_PTP;
    } else if (func == USB_FUNC_MTP) {
        funcBit = SUPPORTED_FUNC_MTP;
    } else if (func == USB_FUNC_PTP) {
        funcBit = SUPPORTED_FUNC_PTP;
    } else if (func == USB_FUNC_REVERSE_CHARGE) {
        funcBit = SUPPORTED_FUNC_REVERSECHARGE;
    }
    if ((usbSupportedFunctions_ & funcBit) == 0) {
        USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: func %{public}s is not supported, skip notification",
            __func__, func.c_str());
        return;
    }

    lastFunc_ = func;
    GetHapString();
    std::shared_ptr<OHOS::Notification::NotificationNormalContent> normalContent =
        std::make_shared<OHOS::Notification::NotificationNormalContent>();
    normalContent->SetTitle(notifierMap[func]);
    normalContent->SetText(notifierMap[USB_FUNC_MORE]);
    std::shared_ptr<OHOS::Notification::NotificationContent> content =
        std::make_shared<OHOS::Notification::NotificationContent>(normalContent);

    request_.SetNotificationId(NOTIFICATION_ID_ZERO);
    request_.SetContent(content);
    request_.SetCreatorUid(g_uid);
    request_.SetUnremovable(true);
    request_.SetRemoveAllowed(true);
    request_.SetTapDismissed(false);
    request_.SetGroupName(NOTIFICATION_NAME);
    if (icon_.has_value()) {
        request_.SetLittleIcon(icon_.value());
    }
    SetWantAgent(request_);
    StartTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_HDF, "Helper::PublishNotification");
    int32_t result = OHOS::Notification::NotificationHelper::PublishNotification(request_);
    FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_HDF);
    USB_HILOGI(MODULE_USB_SERVICE, "PublishNotification result : %{public}d", result);
}

void UsbConnectionNotifier::CancelNotification(bool isHost)
{
    USB_TRACE;
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s", __func__);
    std::lock_guard<std::mutex> guard(functionMutex_);
    if ((isHost && lastFunc_ != USB_FUNC_REVERSE_CHARGE) ||
        (!isHost && lastFunc_ == USB_FUNC_REVERSE_CHARGE)) {
        USB_HILOGW(MODULE_USB_SERVICE, "%{public}s: isHost cancel: %{public}d, lastFunc %{public}s", __func__,
            isHost, lastFunc_.c_str());
        return;
    }
    lastFunc_ = "";
    int32_t notificationId = request_.GetNotificationId();
    int32_t result = OHOS::Notification::NotificationHelper::CancelNotification(notificationId);
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: notificationId %{public}d, result %{public}d", __func__,
        notificationId, result);
}

int32_t UsbConnectionNotifier::GetSupportedFunctions()
{
    USB_TRACE;
    std::string supportedFuncStr = "";
    (void)OHOS::system::GetStringParameter("const.usb_manager.supported_functions",
        supportedFuncStr, DEFAULT_PARAM_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: supportedFuncStr %{public}s", __func__, supportedFuncStr.c_str());

    if (supportedFuncStr.find("none") != std::string::npos) {
        return SUPPORTED_FUNC_NONE;
    }
    uint32_t mtp = supportedFuncStr.find("mtp") != std::string::npos ? SUPPORTED_FUNC_MTP : 0;
    uint32_t ptp = supportedFuncStr.find("ptp") != std::string::npos ? SUPPORTED_FUNC_PTP : 0;
    uint32_t reverseCharge =
        supportedFuncStr.find("reverseCharge") != std::string::npos ? SUPPORTED_FUNC_REVERSECHARGE : 0;
    bool hasCharge = (mtp != 0) || (ptp != 0);
    uint32_t charge = hasCharge ? SUPPORTED_FUNC_CHARGE : 0;
    if (mtp == 0 && ptp == 0 && reverseCharge == 0) {
        return SUPPORTED_FUNC_NONE;
    }
    return static_cast<int32_t>(charge | mtp | ptp | reverseCharge);
}

void UsbConnectionNotifier::SendHdcNotification()
{
    USB_TRACE;
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s", __func__);
    std::lock_guard<std::mutex> guard(hdcMutex_);

    GetHapString();
    std::shared_ptr<OHOS::Notification::NotificationNormalContent> normalContent =
            std::make_shared<OHOS::Notification::NotificationNormalContent>();
    normalContent->SetTitle(notifierMap[USB_HDC_NOTIFIER_TITLE]);
    normalContent->SetText(notifierMap[USB_HDC_NOTIFIER_CONTENT]);
    std::shared_ptr<OHOS::Notification::NotificationContent> content =
            std::make_shared<OHOS::Notification::NotificationContent>(normalContent);

    requestHdc_.SetNotificationId(NOTIFICATION_ID_ONE);
    requestHdc_.SetContent(content);
    requestHdc_.SetCreatorUid(g_uid);
    requestHdc_.SetUnremovable(true);
    requestHdc_.SetRemoveAllowed(true);
    requestHdc_.SetTapDismissed(false);
    requestHdc_.SetGroupName(NOTIFICATION_NAME_ONE);
    requestHdc_.SetSlotType(OHOS::Notification::NotificationConstant::SlotType::SERVICE_REMINDER);
    requestHdc_.SetNotificationControlFlags(NOTIFICATION_STATUS_CLOSE_SOUND |
                                            NOTIFICATION_STATUS_CLOSE_LOCKSCREEN |
                                            NOTIFICATION_STATUS_CLOSE_BANNER |
                                            NOTIFICATION_STATUS_CLOSE_LIGHT_SCREEN |
                                            NOTIFICATION_STATUS_CLOSE_VIBRATION);
    if (icon_.has_value()) {
        requestHdc_.SetLittleIcon(icon_.value());
    }
    SetWantAgentHdc(requestHdc_);
    StartTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_HDF, "Helper::PublishNotification");
    int32_t result = OHOS::Notification::NotificationHelper::PublishNotification(requestHdc_);
    FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_HDF);
    USB_HILOGI(MODULE_USB_SERVICE, "PublishNotification result : %{public}d", result);
}

void UsbConnectionNotifier::CancelHdcNotification()
{
    USB_TRACE;
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s", __func__);
    std::lock_guard<std::mutex> guard(hdcMutex_);
    int32_t notificationId = requestHdc_.GetNotificationId();
    int32_t result = OHOS::Notification::NotificationHelper::CancelNotification(notificationId);
    USB_HILOGI(MODULE_USB_SERVICE, "%{public}s: notificationId %{public}d, result %{public}d", __func__,
               notificationId, result);
}
// LCOV_EXCL_STOP

} // namespace USB
} // namespace OHOS