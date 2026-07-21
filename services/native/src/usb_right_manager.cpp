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

#include "usb_right_manager.h"

#include <algorithm>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex>

#include "ability_manager_client.h"
#include "accesstoken_kit.h"
#include "bundle_mgr_interface.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "os_account_manager.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "tokenid_kit.h"
#include "usb_errors.h"
#include "usb_right_db_helper.h"
#include "usb_napi_errors.h"
#include "usb_srv_support.h"
#include "usb_service.h"
#include "parameters.h"

using namespace OHOS::AppExecFwk;
using namespace OHOS::EventFwk;
using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace USB {

constexpr int32_t PARAM_BUF_LEN = 128;
constexpr int32_t USB_RIGHT_USERID_INVALID = -1;
constexpr int32_t USB_RIGHT_USERID_DEFAULT = 100;
constexpr int32_t USB_RIGHT_USERID_CONSOLE = 0;
constexpr int32_t DECIMAL_BASE = 10;
constexpr int32_t MAX_RETRY_TIMES = 30;
constexpr int32_t RETRY_INTERVAL_SECONDS = 1;
constexpr int32_t MESSAGE_PARCEL_KEY_SIZE = 3;
const std::string USB_MANAGE_ACCESS_USB_DEVICE = "ohos.permission.MANAGE_USB_CONFIG";
const std::string DEVELOPERMODE_STATE = "const.security.developermode.state";
const std::string DEFAULT_SERIAL_BUNDLE_NAME = "com.example.serial";
const std::string DEFAULT_SERIAL_DEVICE_NAME = "1-1";
enum UsbRightTightUpChoose : uint32_t {
    TIGHT_UP_USB_RIGHT_RECORD_NONE = 0,
    TIGHT_UP_USB_RIGHT_RECORD_APP_UNINSTALLED = 1 << 0,
    TIGHT_UP_USB_RIGHT_RECORD_USER_DELETED = 1 << 1,
    TIGHT_UP_USB_RIGHT_RECORD_EXPIRED = 1 << 2,
    TIGHT_UP_USB_RIGHT_RECORD_APP_REINSTALLED = 1 << 3,
};

constexpr uint32_t TIGHT_UP_USB_RIGHT_RECORD_ALL =
    (TIGHT_UP_USB_RIGHT_RECORD_APP_UNINSTALLED | TIGHT_UP_USB_RIGHT_RECORD_USER_DELETED |
        TIGHT_UP_USB_RIGHT_RECORD_EXPIRED | TIGHT_UP_USB_RIGHT_RECORD_APP_REINSTALLED);

sem_t UsbRightManager::waitDialogDisappear_ {0};
std::mutex UsbRightManager::usbDialogParamsMutex_;
std::map<std::string, std::string> UsbRightManager::usbDialogParams_ = {};

class RightSubscriber : public CommonEventSubscriber {
public:
    explicit RightSubscriber(const CommonEventSubscribeInfo &sp) : CommonEventSubscriber(sp) {}

    void OnReceiveEvent(const CommonEventData &data) override
    {
        auto &want = data.GetWant();
        std::string wantAction = want.GetAction();

        USB_HILOGI(MODULE_USB_HOST, "%{public}s wantAction %{public}s", __func__, wantAction.c_str());
        if (wantAction == CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED ||
            wantAction == CommonEventSupport::COMMON_EVENT_BUNDLE_REMOVED ||
            wantAction == CommonEventSupport::COMMON_EVENT_PACKAGE_FULLY_REMOVED) {
            int32_t uid = want.GetParams().GetIntParam("userId", USB_RIGHT_USERID_DEFAULT);
            std::string bundleName = want.GetBundle();
            int32_t ret = UsbRightManager::CleanUpRightAppUninstalled(uid, bundleName);
            USB_HILOGD(MODULE_USB_HOST,
                "recv event uninstall: event=%{public}s bunndleName=%{public}s uid=%{public}d, delete_ret=%{public}d",
                wantAction.c_str(), bundleName.c_str(), uid, ret);
        } else if (wantAction == CommonEventSupport::COMMON_EVENT_UID_REMOVED ||
            wantAction == CommonEventSupport::COMMON_EVENT_USER_REMOVED) {
            int32_t totalUsers = 0;
            int32_t deleteUsers = 0;
            int32_t ret = UsbRightManager::CleanUpRightUserDeleted(totalUsers, deleteUsers);
            USB_HILOGD(MODULE_USB_HOST,
                "recv event user delete: event=%{public}s, delete detail[%{public}d/%{public}d]: %{public}d",
                wantAction.c_str(), deleteUsers, totalUsers, ret);
        } else if (wantAction == CommonEventSupport::COMMON_EVENT_USER_STOPPED) {
            int32_t uid = data.GetCode();
            int32_t ret = UsbRightManager::CleanUpRightUserStopped(uid);
            USB_HILOGD(MODULE_USB_HOST, "on user %{public}d stopped, ret=%{public}d", uid, ret);
#ifdef USB_MANAGER_FEATURE_DEVICE
        } else if (wantAction == CommonEventSupport::COMMON_EVENT_DATA_SHARE_READY) {
            USB_HILOGI(MODULE_USB_HOST, "%{public}s: COMMON_EVENT_DATA_SHARE_READY action is start!", __func__);
            auto usbService = UsbService::GetGlobalInstance();
            if (!usbService->InitSettingsDataHdcStatus()) {
                USB_HILOGE(MODULE_USB_HOST, "%{public}s: function is get failed!", __func__);
            }
        } else if (wantAction == CommonEventSupport::COMMON_EVENT_POWER_CONNECTED ||
                   wantAction == CommonEventSupport::COMMON_EVENT_POWER_DISCONNECTED) {
            USB_HILOGI(MODULE_USB_HOST, "%{public}s: COMMON_EVENT_POWER_CONNECTED action is start!", __func__);
            auto usbService = UsbService::GetGlobalInstance();
            if (usbService != nullptr) {
                usbService->SetPhyConnect(wantAction == CommonEventSupport::COMMON_EVENT_POWER_CONNECTED);
            }
#endif // USB_MANAGER_FEATURE_DEVICE
        } else if (wantAction == CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
#ifdef USB_MANAGER_FEATURE_DEVICE
            USB_HILOGI(MODULE_USB_HOST, "recv user switched.");
            auto usbService = UsbService::GetGlobalInstance();
            if (usbService != nullptr) {
                usbService->UserChangeProcess();
            }
#endif // USB_MANAGER_FEATURE_DEVICE
        }
    }
};

int32_t UsbRightManager::Init()
{
    USB_HILOGI(MODULE_USB_HOST, "subscriber app/bundle remove event and uid/user remove event");
    MatchingSkills matchingSkills;
    /* subscribe app/bundle remove event, need permission: ohos.permission.LISTEN_BUNDLE_CHANGE */
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_BUNDLE_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_PACKAGE_FULLY_REMOVED);
    /* subscribe uid/user remove event */
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_UID_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_STOPPED);

#ifdef USB_MANAGER_FEATURE_DEVICE
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_DATA_SHARE_READY);
    /* subscribe Physic plug/unplug */
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_POWER_CONNECTED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_POWER_DISCONNECTED);
#endif // USB_MANAGER_FEATURE_DEVICE

    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_SWITCHED);
    CommonEventSubscribeInfo subscriberInfo(matchingSkills);
    std::shared_ptr<RightSubscriber> subscriber = std::make_shared<RightSubscriber>(subscriberInfo);
    int32_t retryTimes = 0;
    while (retryTimes < MAX_RETRY_TIMES) {
        retryTimes++;
        bool ret = CommonEventManager::SubscribeCommonEvent(subscriber);
        if (!ret) {
            USB_HILOGW(MODULE_USB_HOST, "subscriber event for right manager failed: %{public}d", ret);
            sleep(RETRY_INTERVAL_SECONDS);
            continue;
        }
        return UEC_OK;
    }
    return UEC_SERVICE_INNER_ERR;
}

bool UsbRightManager::HasRight(const std::string &deviceName, const std::string &bundleName,
    const std::string &tokenId, const int32_t &userId)
{
    USB_HILOGI(MODULE_USB_HOST, "HasRight: uid=%{public}d app=%{public}s",
        userId, bundleName.c_str());
    if (userId == USB_RIGHT_USERID_CONSOLE) {
        USB_HILOGW(MODULE_USB_HOST, "console called, bypass");
        return true;
    }
    uint64_t nowTime = GetCurrentTimestamp();
    (void)TidyUpRight(TIGHT_UP_USB_RIGHT_RECORD_EXPIRED);
    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    // no record or expired record: expired true, has right false, add right next time
    // valid record: expired false, has right true, no need add right
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "helper is nullptr, false");
        return false;
    }
    return !helper->IsRecordExpired(userId, deviceName, bundleName, tokenId, nowTime);
}

int32_t UsbRightManager::ConnectAbility(const int32_t userId)
{
    USB_HILOGI(MODULE_USB_HOST, "%{public}s: uid %{public}d", __func__, userId);
    if (usbAbilityConn_ == nullptr) {
        USB_HILOGI(MODULE_USB_HOST, "new UsbAbilityConn");
        usbAbilityConn_ = sptr<UsbAbilityConn>(new (std::nothrow) UsbAbilityConn());
    }

    auto abmc = AAFwk::AbilityManagerClient::GetInstance();
    if (abmc == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "GetInstance failed");
        return USB_RIGHT_FAILURE;
    }

    AAFwk::Want want;
    want.SetElementName("com.ohos.sceneboard", "com.ohos.sceneboard.systemdialog");
    int32_t ret = abmc->ConnectAbility(want, usbAbilityConn_, userId);
    if (ret != ERR_OK) {
        want.SetElementName("com.ohos.systemui", "com.ohos.systemui.dialog");
        int32_t ret = abmc->ConnectAbility(want, usbAbilityConn_, userId);
        if (ret != ERR_OK) {
            USB_HILOGE(MODULE_USB_HOST, "ConnectServiceExtensionAbility systemui failed, ret: %{public}d", ret);
            usbAbilityConn_ = nullptr;
            return ret;
        }
    }
    return USB_RIGHT_OK;
}

#ifdef USB_MANAGER_FEATURE_HOST
int32_t UsbRightManager::RequestRight(const std::string &busDev, const std::string &deviceName,
    const std::string &bundleName, const std::string &tokenId, const int32_t &userId)
{
    USB_HILOGD(MODULE_USB_HOST, "RequestRight: busdev=%{private}s app=%{public}s", busDev.c_str(),
        bundleName.c_str());
    if (HasRight(deviceName, bundleName, tokenId, userId)) {
        USB_HILOGW(MODULE_USB_HOST, "device has Right ");
        return UEC_OK;
    }
    if (!GetUserAgreementByDiag(busDev, deviceName, bundleName, tokenId, userId)) {
        USB_HILOGW(MODULE_USB_HOST, "user don't agree");
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    return UEC_OK;
}

bool UsbRightManager::GetUserAgreementByDiag(const std::string &busDev, const std::string &deviceName,
    const std::string &bundleName, const std::string &tokenId, const int32_t &userId)
{
#ifdef USB_RIGHT_TEST
    return true;
#endif
    /* There can only be one dialog at a time */
    std::lock_guard<std::mutex> guard(dialogRunning_);
    if (!ShowUsbDialog(busDev, deviceName, bundleName, tokenId, userId)) {
        USB_HILOGE(MODULE_USB_HOST, "ShowUsbDialog failed");
        return false;
    }

    return HasRight(deviceName, bundleName, tokenId, userId);
}

bool UsbRightManager::ShowUsbDialog(
    const std::string &busDev, const std::string &deviceName, const std::string &bundleName,
    const std::string &tokenId, const int32_t userId)
{
    USB_HILOGI(MODULE_USB_HOST,
        "%{public}s deviceName %{public}s bundleName %{public}s tokenId %{public}s uid %{public}d",
        __func__, deviceName.c_str(), bundleName.c_str(), tokenId.c_str(), userId);

    std::string appName;
    if (!GetAppName(bundleName, appName)) {
        appName = bundleName;
    }

    std::string productName;
    if (!GetProductName(busDev, productName)) {
        productName = busDev;
    }
    
    {
        std::lock_guard<std::mutex> lock(usbDialogParamsMutex_);
        usbDialogParams_.clear();
        usbDialogParams_["bundleName"] = bundleName;
        usbDialogParams_["deviceName"] = busDev;
        usbDialogParams_["tokenId"] = tokenId;
        usbDialogParams_["appName"] = appName;
        usbDialogParams_["productName"] = productName;
    }

    sem_init(&waitDialogDisappear_, 1, 0);
    int32_t ret = ConnectAbility(userId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "connectAbility failed %{public}d", ret);
        return false;
    }
    /* Waiting for the user to click */
    sem_wait(&waitDialogDisappear_);
    USB_HILOGI(MODULE_USB_HOST, "%{public}s success", __func__);
    return true;
}

bool UsbRightManager::GetProductName(const std::string &devName, std::string &productName)
{
    auto usbService = UsbService::GetGlobalInstance();
    if (usbService == nullptr) {
        return false;
    }
    return usbService->GetDeviceProductName(devName, productName);
}
#endif // USB_MANAGER_FEATURE_HOST

int32_t UsbRightManager::RequestRight(const USBAccessory &access, const std::string &seriaValue,
    const std::string &bundleName, const std::string &tokenId, const int32_t &userId, bool &result)
{
    USB_HILOGD(MODULE_USB_HOST, "RequestAccessoryRight:  seriaValue=%{public}s app=%{public}s",
        seriaValue.c_str(), bundleName.c_str());
    if (HasRight(seriaValue, bundleName, tokenId, userId)) {
        USB_HILOGW(MODULE_USB_HOST, "device has Right ");
        result = true;
        return UEC_OK;
    }
    if (!GetUserAgreementByDiag(access, seriaValue, bundleName, tokenId, userId)) {
        USB_HILOGW(MODULE_USB_HOST, "user don't agree");
        result = false;
        return UEC_OK;
    }
    result = true;
    return UEC_OK;
}

bool IsWithinUint64Range(const std::string &numberStr)
{
    if (numberStr.empty()) {
        USB_HILOGE(MODULE_USB_HOST, "numberStr is empty");
        return false;
    }
    errno = 0;
    uint64_t number = 0;
    number = std::strtoull(numberStr.c_str(), nullptr, DECIMAL_BASE);
    if (errno == ERANGE) {
        USB_HILOGE(MODULE_USB_HOST, "number is out of uint64_t range");
        return false;
    }
    return true;
}

int32_t UsbRightManager::RequestRight(const int32_t portId, const SerialDeviceIdentity &serialDeviceIdentity,
    const std::string &bundleName, const std::string &tokenId, const int32_t &userId)
{
    USB_HILOGI(MODULE_USB_HOST, "RequestSerialRight: serialValue=%{public}s app=%{public}s",
        serialDeviceIdentity.deviceName.c_str(), bundleName.c_str());
    if (HasRight(serialDeviceIdentity.deviceName, bundleName, tokenId, userId)) {
        USB_HILOGW(MODULE_USB_HOST, "device has Right ");
        return UEC_OK;
    }
    if (!GetUserAgreementByDiag(portId, serialDeviceIdentity, bundleName, tokenId, userId)) {
        USB_HILOGW(MODULE_USB_HOST, "user don't agree");
        return UEC_SERVICE_PERMISSION_DENIED;
    }
    return UEC_OK;
}

bool UsbRightManager::AddDeviceRight(const std::string &deviceName, const std::string &tokenIdStr)
{
    if (!IsAllDigits(tokenIdStr)) {
        USB_HILOGE(MODULE_USB_HOST, "tokenIdStr invalid");
        return false;
    }
    /* already checked system app/hap when call */
    if (!IsWithinUint64Range(tokenIdStr)) {
        USB_HILOGE(MODULE_USB_HOST, "tokenIdStr is out of uint64_t range");
        return false;
    }
    HapTokenInfo hapTokenInfoRes;
    int32_t ret = AccessTokenKit::GetHapTokenInfo((AccessTokenID) std::stoul(tokenIdStr), hapTokenInfoRes);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetHapTokenInfo failed:ret:%{public}d", ret);
        return false;
    }
    int32_t uid = hapTokenInfoRes.userID;
    if (uid == USB_RIGHT_USERID_CONSOLE) {
        USB_HILOGE(MODULE_USB_HOST, "console called, bypass");
        return true;
    }
    uint64_t installTime = GetCurrentTimestamp();
    uint64_t updateTime = GetCurrentTimestamp();
    if (!GetBundleInstallAndUpdateTime(uid, hapTokenInfoRes.bundleName, installTime, updateTime)) {
        USB_HILOGE(MODULE_USB_HOST, "get app install time and update time failed: %{public}d", uid);
    }
    struct UsbRightAppInfo info;
    info.uid = uid;
    info.installTime = installTime;
    info.updateTime = updateTime;
    info.requestTime = GetCurrentTimestamp();
    info.validPeriod = USB_RIGHT_VALID_PERIOD_SET;

    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "helper is nullptr, false");
        return false;
    }
    ret = helper->AddOrUpdateRightRecord(uid, deviceName, hapTokenInfoRes.bundleName, tokenIdStr, info);
    if (ret < 0) {
        USB_HILOGE(MODULE_USB_HOST, "add or update failed: %{public}s/%{public}d, ret=%{public}d",
            deviceName.c_str(), uid, ret);
        return false;
    }
    return true;
}

bool UsbRightManager::AddDeviceRight(const std::string &deviceName, const std::string &bundleName,
    const std::string &tokenId, const int32_t &userId)
{
    /* already checked system app/hap when call */
    if (userId == USB_RIGHT_USERID_CONSOLE) {
        USB_HILOGE(MODULE_USB_HOST, "console called, bypass");
        return true;
    }
    uint64_t installTime = GetCurrentTimestamp();
    uint64_t updateTime = GetCurrentTimestamp();
    if (!GetBundleInstallAndUpdateTime(userId, bundleName, installTime, updateTime)) {
        USB_HILOGE(MODULE_USB_HOST, "get app install time and update time failed: %{public}s/%{public}d",
            bundleName.c_str(), userId);
    }
    struct UsbRightAppInfo info;
    info.uid = userId;
    info.installTime = installTime;
    info.updateTime = updateTime;
    info.requestTime = GetCurrentTimestamp();
    info.validPeriod = USB_RIGHT_VALID_PERIOD_SET;

    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "helper is nullptr, false");
        return false;
    }
    int32_t ret = helper->AddOrUpdateRightRecord(userId, deviceName, bundleName, tokenId, info);
    if (ret < 0) {
        USB_HILOGE(MODULE_USB_HOST, "add or update failed: %{public}s/%{public}s/%{public}d, ret=%{public}d",
            deviceName.c_str(), bundleName.c_str(), userId, ret);
        return false;
    }
    return true;
}

bool UsbRightManager::RemoveDeviceRight(const std::string &deviceName, const std::string &bundleName,
    const std::string &tokenId, const int32_t &userId)
{
    if (userId == USB_RIGHT_USERID_CONSOLE) {
        USB_HILOGW(MODULE_USB_HOST, "console called, bypass");
        return true;
    }
    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "helper is nullptr, false");
        return false;
    }
    int32_t ret = helper->DeleteRightRecord(userId, deviceName, bundleName, tokenId);
    if (ret < 0) {
        USB_HILOGE(MODULE_USB_HOST, "delete failed: %{public}s/%{public}s/%{public}d", deviceName.c_str(),
            bundleName.c_str(), userId);
        return false;
    }
    return true;
}

int32_t UsbRightManager::CancelDeviceRight(const std::string &deviceName, const std::string &bundleName,
    const std::string &tokenId, const int32_t &userId)
{
    if (userId == USB_RIGHT_USERID_CONSOLE) {
        USB_HILOGW(MODULE_USB_HOST, "console called, bypass");
        return UEC_SERVICE_INVALID_VALUE;
    }
    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "helper is nullptr, false");
        return UEC_SERVICE_INNER_ERR;
    }
    int32_t ret = helper->DeleteRightRecord(userId, deviceName, bundleName, tokenId);
    if (ret < 0) {
        USB_HILOGW(MODULE_USB_HOST, "delete failed: %{public}s/%{public}s/%{public}d", deviceName.c_str(),
            bundleName.c_str(), userId);
        return UEC_OK;
    }
    return UEC_OK;
}

bool UsbRightManager::RemoveDeviceAllRight(const std::string &deviceName)
{
    USB_HILOGD(MODULE_USB_HOST, "device %{private}s detached, process right", deviceName.c_str());
    CleanUpRightTemporaryExpired(deviceName);
    TidyUpRight(TIGHT_UP_USB_RIGHT_RECORD_ALL);
    UnShowUsbDialog();
    return true;
}

bool UsbRightManager::GetAccessoryName(const USBAccessory &access, std::string &accessName)
{
    accessName = access.GetProduct();
    return true;
}

bool UsbRightManager::ShowUsbDialog(const USBAccessory &access, const std::string &seriaValue,
    const std::string &bundleName, const std::string &tokenId)
{
    USB_HILOGI(MODULE_USB_HOST, "%{public}s seriaValue %{public}s bundleName %{public}s tokenId %{public}s",
               __func__, seriaValue.c_str(), bundleName.c_str(), tokenId.c_str());

    std::string appName;
    if (!GetAppName(bundleName, appName)) {
        appName = bundleName;
    }

    std::string accessoryName;
    if (!GetAccessoryName(access, accessoryName)) {
        accessoryName = seriaValue;
    }

    {
        std::lock_guard <std::mutex> lock(usbDialogParamsMutex_);
        usbDialogParams_.clear();
        usbDialogParams_["bundleName"] = bundleName;
        usbDialogParams_["deviceName"] = seriaValue;
        usbDialogParams_["tokenId"] = tokenId;
        usbDialogParams_["appName"] = appName;
        usbDialogParams_["productName"] = accessoryName;
        usbDialogParams_["accessory"] = access.GetJsonString();
    }
    
    sem_init(&waitDialogDisappear_, 1, 0);
    int32_t ret = ConnectAbility(USB_RIGHT_USERID_INVALID);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "connectAbility failed %{public}d", ret);
        return false;
    }
    /* Waiting for the user to click */
    sem_wait(&waitDialogDisappear_);
    USB_HILOGI(MODULE_USB_HOST, "%{public}s success", __func__);
    return true;
}

bool UsbRightManager::ShowSerialDialog(const int32_t portId, const uint32_t tokenId, const std::string &bundleName,
    const std::string &busDev)
{
    USB_HILOGI(MODULE_USB_HOST,
               "%{public}s portId %{public}d tokenId %{public}d bundleName %{public}s busDev %{public}s",
               __func__, portId, tokenId, bundleName.c_str(), busDev.c_str());

    std::string appName;
    if (!GetAppName(bundleName, appName)) {
        appName = bundleName;
    }

    int32_t castId = static_cast<int32_t>(tokenId);
    if (castId < 0) {
        USB_HILOGE(MODULE_USB_HOST, "tokenId cast failed %{public}d", castId);
        return false;
    }

    {
        std::lock_guard <std::mutex> lock(usbDialogParamsMutex_);
        usbDialogParams_.clear();
        usbDialogParams_["portId"] = std::to_string(portId);
        usbDialogParams_["tokenId"] = std::to_string(castId);
        usbDialogParams_["bundleName"] = DEFAULT_SERIAL_BUNDLE_NAME;
        usbDialogParams_["deviceName"] = DEFAULT_SERIAL_DEVICE_NAME;
        usbDialogParams_["appName"] = appName;
        usbDialogParams_["productName"] = "COM" + std::to_string(portId);
    }

    sem_init(&waitDialogDisappear_, 1, 0);
    int32_t ret = ConnectAbility(USB_RIGHT_USERID_INVALID);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "connectAbility failed %{public}d", ret);
        return false;
    }
    /* Waiting for the user to click */
    sem_wait(&waitDialogDisappear_);
    USB_HILOGI(MODULE_USB_HOST, "sem_wait done");
    return true;
}

bool UsbRightManager::GetUserAgreementByDiag(const USBAccessory &access, const std::string &seriaValue,
    const std::string &bundleName, const std::string &tokenId, const int32_t &userId)
{
    /* There can only be one dialog at a time */
    std::lock_guard<std::mutex> guard(dialogRunning_);
    if (!ShowUsbDialog(access, seriaValue, bundleName, tokenId)) {
        USB_HILOGE(MODULE_USB_HOST, "ShowUsbDialog failed");
        return false;
    }

    return HasRight(seriaValue, bundleName, tokenId, userId);
}

bool UsbRightManager::GetUserAgreementByDiag(const int32_t portId, const SerialDeviceIdentity &serialDeviceIdentity,
    const std::string &bundleName, const std::string &tokenId, const int32_t &userId)
{
    USB_HILOGI(MODULE_USB_HOST, "GetUserAgreementByDiag start");
    /* There can only be one dialog at a time */
    std::lock_guard<std::mutex> guard(dialogRunning_);
    if (!std::regex_match(tokenId, std::regex("^[0-9]+$"))) {
        USB_HILOGE(MODULE_USB_HOST, "Invalid tokenId");
        return false;
    }
    uint32_t mTokenId = static_cast<uint32_t>(std::stoul(tokenId));
    if (!ShowSerialDialog(portId, mTokenId, bundleName, serialDeviceIdentity.busDev)) {
        USB_HILOGE(MODULE_USB_HOST, "ShowSerialDialog failed");
        return false;
    }

    return HasRight(serialDeviceIdentity.deviceName, bundleName, tokenId, userId);
}

sptr<IBundleMgr> UsbRightManager::GetBundleMgr()
{
    auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sam == nullptr) {
        USB_HILOGW(MODULE_USB_HOST, "GetSystemAbilityManager return nullptr");
        return nullptr;
    }
    auto bundleMgrSa = sam->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleMgrSa == nullptr) {
        USB_HILOGW(MODULE_USB_HOST, "GetSystemAbility return nullptr");
        return nullptr;
    }
    auto bundleMgr = iface_cast<IBundleMgr>(bundleMgrSa);
    if (bundleMgr == nullptr) {
        USB_HILOGW(MODULE_USB_HOST, "iface_cast return nullptr");
    }
    return bundleMgr;
}

sptr<IBundleResource> UsbRightManager::GetBundleResMgr()
{
    auto bundleMgr = GetBundleMgr();
    if (bundleMgr == nullptr) {
        return nullptr;
    }
    return bundleMgr->GetBundleResourceProxy();
}

bool UsbRightManager::GetAppName(const std::string &bundleName, std::string &appName)
{
    auto resMgr = GetBundleResMgr();
    if (resMgr == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "GetAppName: get res mgr failed");
        return false;
    }

    BundleResourceInfo info;
    int32_t ret = resMgr->GetBundleResourceInfo(bundleName, (uint32_t)ResourceFlag::GET_RESOURCE_INFO_WITH_LABEL, info);
    if (ret != ERR_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetAppName: get res info failed: %{public}d", ret);
        return false;
    }
    appName = info.label;
    return true;
}

bool UsbRightManager::IsSystemAppOrSa()
{
    uint64_t tokenid = IPCSkeleton::GetCallingFullTokenID();
    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(tokenid);
    if (isSystemApp) {
        return true;
    }

    AccessTokenID accessTokenId = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(accessTokenId);
    if (tokenType == TOKEN_NATIVE) {
        return true;
    }

    USB_HILOGW(MODULE_USB_HOST, "neither system app nor sa");
    return false;
}

bool UsbRightManager::VerifyPermission()
{
    AccessTokenID tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t ret = AccessTokenKit::VerifyAccessToken(tokenId, USB_MANAGE_ACCESS_USB_DEVICE);
    if (ret == PermissionState::PERMISSION_DENIED) {
        USB_HILOGW(MODULE_USB_HOST, "no permission");
        return false;
    }
    return true;
}

bool UsbRightManager::IsAppInstalled(int32_t uid, const std::string &bundleName)
{
    auto bundleMgr = GetBundleMgr();
    if (bundleMgr == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "BundleMgr is nullptr, return false");
        return false;
    }
    ApplicationInfo appInfo;
    if (!bundleMgr->GetApplicationInfo(bundleName, GET_BASIC_APPLICATION_INFO, uid, appInfo)) {
        USB_HILOGE(MODULE_USB_HOST, "BundleMgr GetApplicationInfo failed");
        return false;
    }
    return true;
}

bool UsbRightManager::GetBundleInstallAndUpdateTime(
    int32_t uid, const std::string &bundleName, uint64_t &installTime, uint64_t &updateTime)
{
    BundleInfo bundleInfo;
    auto bundleMgr = GetBundleMgr();
    if (bundleMgr == nullptr) {
        USB_HILOGW(MODULE_USB_HOST, "BundleMgr is nullptr, return false");
        return false;
    }
    if (!bundleMgr->GetBundleInfo(bundleName, BundleFlag::GET_BUNDLE_DEFAULT, bundleInfo, uid)) {
        USB_HILOGW(MODULE_USB_HOST, "BundleMgr GetBundleInfo(uid) failed");
        return false;
    }
    installTime = static_cast<uint64_t>(bundleInfo.installTime);
    updateTime = static_cast<uint64_t>(bundleInfo.updateTime);
    return true;
}

uint64_t UsbRightManager::GetCurrentTimestamp()
{
    int64_t time =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return static_cast<uint64_t>(time);
}

void UsbRightManager::GetCurrentUserId(int32_t &uid)
{
    int32_t ret = AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(IPCSkeleton::GetCallingUid(), uid);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "GetOsAccountLocalIdFromUid failed: %{public}d, set to defult", ret);
        uid = USB_RIGHT_USERID_DEFAULT; /* default user id */
    }
    USB_HILOGD(MODULE_USB_HOST, "usb get userid success: %{public}d, uid: %{public}d", ret, uid);
}

int32_t UsbRightManager::IsOsAccountExists(int32_t id, bool &isAccountExists)
{
    int32_t ret = AccountSA::OsAccountManager::IsOsAccountExists(id, isAccountExists);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, " api IsOsAccountExists failed: ret=%{public}d id=%{public}d", ret, id);
        return USB_RIGHT_FAILURE;
    }
    return USB_RIGHT_OK;
}

int32_t UsbRightManager::HasSetFuncRight(int32_t functions)
{
    if (!(static_cast<uint32_t>(functions) & UsbSrvSupport::FUNCTION_HDC)) {
        return UEC_OK;
    }
    USB_HILOGI(MODULE_USB_HOST, "Set up function permission validation");
    char paramValue[PARAM_BUF_LEN] = { 0 };
    int32_t ret = GetParameter("persist.hdc.control", "true", paramValue, sizeof(paramValue));
    if (ret < 0) {
        USB_HILOGW(MODULE_USB_HOST, "GetParameter fail");
    }
    ret = strcmp(paramValue, "true");
    if (ret != 0) {
        USB_HILOGE(MODULE_USB_HOST, "HDC setup failed");
        return UEC_SERVICE_PERMISSION_CHECK_HDC;
    }
    if (!OHOS::system::GetBoolParameter(DEVELOPERMODE_STATE, false)) {
        USB_HILOGE(MODULE_USB_HOST, "Developer mode unabled, FUNCTION_HDC cannot be set");
        return UEC_SERVICE_PERMISSION_CHECK_HDC;
    }
    return UEC_OK;
}

int32_t UsbRightManager::CleanUpRightExpired(std::vector<std::string> &devices)
{
    USB_HILOGD(MODULE_USB_HOST, "clean up expired right: size=%{public}zu", devices.size());
    size_t len = devices.size();
    int32_t ret = USB_RIGHT_OK;
    for (size_t i = 0; i < len; i++) {
        std::string dev = devices.at(i);
        ret = CleanUpRightTemporaryExpired(dev);
        if (ret != USB_RIGHT_OK) {
            USB_HILOGE(MODULE_USB_HOST,
                "failed(%{public}zu/%{public}zu): delete temporary expiried record, dev=%{private}s", i, len,
                dev.c_str());
            continue;
        }
    }
    int32_t uid = USB_RIGHT_USERID_INVALID;
    GetCurrentUserId(uid);
    ret = CleanUpRightNormalExpired(uid);
    if (ret != USB_RIGHT_OK) {
        USB_HILOGE(MODULE_USB_HOST, "delete expired record with uid(%{public}d) failed: %{public}d", uid, ret);
    }
    return ret;
}

int32_t UsbRightManager::CleanUpRightAppUninstalled(int32_t uid, int32_t &totalApps, int32_t &deleteApps)
{
    std::vector<std::string> apps;
    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "helper is nullptr, false");
        return false;
    }
    int32_t ret = helper->QueryRightRecordApps(uid, apps);
    if (ret <= 0) {
        /* error or empty record */
        return USB_RIGHT_NOP;
    }
    totalApps = static_cast<int32_t>(apps.size());
    deleteApps = 0;
    for (int32_t i = 0; i < totalApps; i++) {
        std::string app = apps.at(i);
        if (!IsAppInstalled(uid, app)) {
            ret = helper->DeleteAppRightRecord(uid, app);
            if (ret != USB_RIGHT_OK) {
                USB_HILOGW(MODULE_USB_HOST, "clean failed: app=%{public}s, ret=%{public}d", app.c_str(), ret);
                continue;
            }
            deleteApps++;
        }
    }
    USB_HILOGD(MODULE_USB_HOST, "clean uninstall app record[%{public}d/%{public}d]: uid=%{public}d", deleteApps,
        totalApps, uid);
    return ret;
}

int32_t UsbRightManager::CleanUpRightAppUninstalled(int32_t uid, const std::string &bundleName)
{
    std::vector<std::string> apps;
    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "helper is nullptr, false");
        return false;
    }
    int32_t ret = helper->QueryRightRecordApps(uid, apps);
    if (ret <= 0) {
        /* error or empty record */
        return USB_RIGHT_NOP;
    }
    int32_t index = 0;
    if (!StringVectorFound(apps, bundleName, index)) {
        /* app not in record, ignore */
        return USB_RIGHT_NOP;
    }
    ret = helper->DeleteAppRightRecord(uid, apps.at(index));
    USB_HILOGD(MODULE_USB_HOST, "clean[%{public}d/%{public}zu]: uid=%{public}d, app=%{public}s, ret=%{public}d",
        index, apps.size(), uid, bundleName.c_str(), ret);
    return ret;
}

void UsbRightManager::StringVectorSortAndUniq(std::vector<std::string> &strings)
{
    sort(strings.begin(), strings.end());
    auto last = unique(strings.begin(), strings.end());
    strings.erase(last, strings.end());
}

bool UsbRightManager::StringVectorFound(
    const std::vector<std::string> &strings, const std::string &value, int32_t &index)
{
    size_t len = strings.size();
    for (size_t i = 0; i < len; i++) {
        if (value == strings.at(i)) {
            index = static_cast<int32_t>(i);
            return true;
        }
    }
    return false;
}

int32_t UsbRightManager::CleanUpRightAppReinstalled(int32_t uid, uint32_t &totalApps, uint32_t &deleteApps)
{
    std::vector<std::string> apps;
    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "helper is nullptr, false");
        return false;
    }
    int32_t ret = helper->QueryRightRecordApps(uid, apps);
    if (ret <= 0) {
        USB_HILOGE(MODULE_USB_HOST, "query apps failed or empty: %{public}d", ret);
        return USB_RIGHT_NOP;
    }
    StringVectorSortAndUniq(apps);
    deleteApps = 0;
    totalApps = apps.size();
    std::vector<std::string> deleteBundleNames;
    for (size_t i = 0; i < apps.size(); i++) {
        std::string bundleName = apps.at(i);
        std::vector<struct UsbRightAppInfo> infos;
        ret = helper->QueryAppRightRecord(uid, bundleName, infos);
        if (ret < 0) {
            USB_HILOGE(MODULE_USB_HOST, "query app info %{public}s failed: %{public}d", bundleName.c_str(), ret);
            return USB_RIGHT_FAILURE;
        }
        uint64_t installTime = 0;
        uint64_t updateTime = 0;
        if (!GetBundleInstallAndUpdateTime(uid, bundleName, installTime, updateTime)) {
            USB_HILOGE(MODULE_USB_HOST, "get app install time and update time failed: app=%{public}s uid=%{public}d",
                bundleName.c_str(), uid);
            return USB_RIGHT_FAILURE;
        }
        for (size_t j = 0; j < infos.size(); j++) {
            struct UsbRightAppInfo info = infos.at(j);
            if (info.installTime != installTime) {
                deleteBundleNames.push_back(bundleName);
                break;
            }
        }
    }
    StringVectorSortAndUniq(deleteBundleNames);
    ret = helper->DeleteAppsRightRecord(uid, deleteBundleNames);
    if (ret != USB_RIGHT_OK) {
        USB_HILOGE(MODULE_USB_HOST, "delete apps failed: %{public}d", ret);
    } else {
        deleteApps = deleteBundleNames.size();
    }
    return ret;
}

int32_t UsbRightManager::CleanUpRightUserDeleted(int32_t &totalUsers, int32_t &deleteUsers)
{
    std::vector<std::string> rightRecordUids;
    bool isAccountExists = false;
    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "helper is nullptr, false");
        return false;
    }
    int32_t ret = helper->QueryRightRecordUids(rightRecordUids);
    if (ret <= 0) {
        USB_HILOGE(MODULE_USB_HOST, "query apps failed or empty: %{public}d", ret);
        return USB_RIGHT_NOP;
    }
    size_t len = rightRecordUids.size();
    deleteUsers = 0;
    for (size_t i = 0; i < len; i++) {
        int32_t uid = 0;
        if (!StrToInt(rightRecordUids.at(i), uid)) {
            USB_HILOGE(MODULE_USB_HOST, "convert failed: %{public}s", rightRecordUids.at(i).c_str());
            continue;
        }
        ret = IsOsAccountExists(uid, isAccountExists);
        if (ret != USB_RIGHT_OK) {
            USB_HILOGE(MODULE_USB_HOST, "call IsOsAccountExists failed: %{public}d", ret);
            continue;
        }
        if (!isAccountExists) {
            ret = helper->DeleteUidRightRecord(uid);
            USB_HILOGE(MODULE_USB_HOST, "detecte delete uid=%{public}d: %{public}d", uid, ret);
            deleteUsers++;
        }
        USB_HILOGD(MODULE_USB_HOST, "uid exist, ignore: %{public}d", uid);
    }
    totalUsers = static_cast<int32_t>(rightRecordUids.size());
    return USB_RIGHT_OK;
}

int32_t UsbRightManager::CleanUpRightUserStopped(int32_t uid)
{
    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "CleanUpRightUserStopped %{public}d: helper is null", uid);
        return false;
    }

    return helper->DeleteUidRightRecord(uid);
}

int32_t UsbRightManager::CleanUpRightTemporaryExpired(const std::string &deviceName)
{
    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    if (helper == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "helper is nullptr, false");
        return false;
    }
    int32_t ret = helper->DeleteValidPeriodRightRecord(USB_RIGHT_VALID_PERIOD_MIN, deviceName);
    if (ret != USB_RIGHT_OK) {
        USB_HILOGE(MODULE_USB_HOST, "failed: delete temporary expiried record: dev=%{private}s", deviceName.c_str());
    }
    return ret;
}

int32_t UsbRightManager::CleanUpRightNormalExpired(int32_t uid)
{
    uint64_t nowTime = GetCurrentTimestamp();
    std::shared_ptr<UsbRightDbHelper> helper = UsbRightDbHelper::GetInstance();
    int32_t ret = helper->DeleteNormalExpiredRightRecord(uid, nowTime);
    if (ret != USB_RIGHT_OK) {
        USB_HILOGD(MODULE_USB_HOST, "failed: clean up expired record at %{public}" PRIu64 "", nowTime);
    }
    return ret;
}

int32_t UsbRightManager::TidyUpRight(uint32_t choose)
{
    if (choose == TIGHT_UP_USB_RIGHT_RECORD_NONE) {
        /* ignore */
        return USB_RIGHT_NOP;
    }
    if ((choose | TIGHT_UP_USB_RIGHT_RECORD_ALL) != TIGHT_UP_USB_RIGHT_RECORD_ALL) {
        USB_HILOGE(MODULE_USB_HOST, "choose invalid");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t uid = USB_RIGHT_USERID_INVALID;
    GetCurrentUserId(uid);
    if (uid == USB_RIGHT_USERID_CONSOLE) {
        USB_HILOGE(MODULE_USB_HOST, "console called, bypass");
        return true;
    }
    int32_t ret = 0;
    if ((choose & TIGHT_UP_USB_RIGHT_RECORD_APP_UNINSTALLED) != 0) {
        int32_t totalUninstalledApps = 0;
        int32_t deleteUninstalledApps = 0;
        ret = CleanUpRightAppUninstalled(uid, totalUninstalledApps, deleteUninstalledApps);
        USB_HILOGD(MODULE_USB_HOST, "delete app uninstalled record[%{public}d/%{public}d]: %{public}d",
            deleteUninstalledApps, totalUninstalledApps, ret);
    }
    if ((choose & TIGHT_UP_USB_RIGHT_RECORD_USER_DELETED) != 0) {
        int32_t totalUsers = 0;
        int32_t deleteUsers = 0;
        ret = CleanUpRightUserDeleted(totalUsers, deleteUsers);
        USB_HILOGD(MODULE_USB_HOST, "delete user deleted record[%{public}d/%{public}d]: %{public}d", deleteUsers,
            totalUsers, ret);
    }
    if ((choose & TIGHT_UP_USB_RIGHT_RECORD_EXPIRED) != 0) {
        ret = CleanUpRightNormalExpired(uid);
        USB_HILOGD(MODULE_USB_HOST, "delete expired record: %{public}d", ret);
    }
    if ((choose & TIGHT_UP_USB_RIGHT_RECORD_APP_REINSTALLED) != 0) {
        uint32_t totalReinstalledApps = 0;
        uint32_t deleteReinstalledApps = 0;
        ret = CleanUpRightAppReinstalled(uid, totalReinstalledApps, deleteReinstalledApps);
        USB_HILOGD(MODULE_USB_HOST, "delete app reinstalled record[%{public}u/%{public}u]: %{public}d",
            deleteReinstalledApps, totalReinstalledApps, ret);
    }
    return ret;
}

bool UsbRightManager::IsAllDigits(const std::string &bundleName)
{
    size_t len = bundleName.length();
    for (size_t i = 0; i < len; i++) {
        if (!isdigit(bundleName[i])) {
            return false;
        }
    }
    return true;
}

bool UsbRightManager::UnShowUsbDialog()
{
    if (usbAbilityConn_ == nullptr) {
        return true;
    }

    auto abmc = AAFwk::AbilityManagerClient::GetInstance();
    if (abmc == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "GetInstance failed");
        return false;
    }
    USB_HILOGI(MODULE_USB_HOST, "unshow usb dialog window");
    usbAbilityConn_->CloseDialog();

    int32_t ret = abmc->DisconnectAbility(usbAbilityConn_);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_HOST, "DisconnectAbility failed %{public}d", ret);
        return false;
    }
    USB_HILOGD(MODULE_USB_HOST, "unshow usb dialog window success");
    return true;
}

void UsbRightManager::UsbAbilityConn::OnAbilityConnectDone(const AppExecFwk::ElementName &element,
                                                           const sptr<IRemoteObject> &remoteObject, int32_t resultCode)
{
    USB_HILOGI(MODULE_USB_HOST, "%{public}s UsbAbilityConn", __func__);
    if (remoteObject == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "remoteObject is nullptr");
        return;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(MESSAGE_PARCEL_KEY_SIZE);
    data.WriteString16(u"bundleName");
    data.WriteString16(u"com.usb.right");
    data.WriteString16(u"abilityName");
    data.WriteString16(u"UsbServiceExtAbility");
    data.WriteString16(u"parameters");
    cJSON* paramJson = cJSON_CreateObject();

    {
        std::lock_guard <std::mutex> lock(usbDialogParamsMutex_);
        for (auto it : usbDialogParams_) {
            cJSON_AddStringToObject(paramJson, it.first.c_str(), it.second.c_str());
        }
    }

    std::string uiExtensionTypeStr = "sysDialog/common";
    cJSON_AddStringToObject(paramJson, "ability.want.params.uiExtensionType", uiExtensionTypeStr.c_str());

    char *pParamJson = cJSON_PrintUnformatted(paramJson);
    cJSON_Delete(paramJson);
    paramJson = nullptr;
    if (!pParamJson) {
        USB_HILOGE(MODULE_USB_HOST, "Print paramJson error");
        return;
    }
    std::string paramStr(pParamJson);
    data.WriteString16(Str8ToStr16(paramStr));
    cJSON_free(pParamJson);
    pParamJson = NULL;

    const uint32_t cmdCode = 1;
    int32_t ret = remoteObject->SendRequest(cmdCode, data, reply, option);
    USB_HILOGI(MODULE_USB_HOST, "%{public}s ret %{public}d", __func__, ret);
    if (ret != ERR_OK) {
        USB_HILOGE(MODULE_USB_HOST, "send request failed: %{public}d", ret);
        return;
    }
    if (!reply.ReadInt32(ret) || ret != ERR_OK) {
        USB_HILOGE(MODULE_USB_HOST, "show dialog failed: %{public}d", ret);
        return;
    }
    std::lock_guard<std::mutex> guard(remoteMutex_);
    remoteObject_ = remoteObject;
}

void UsbRightManager::UsbAbilityConn::OnAbilityDisconnectDone(const AppExecFwk::ElementName &element,
                                                              int32_t resultCode)
{
    USB_HILOGI(MODULE_USB_HOST, "disconnect done");
    sem_post(&waitDialogDisappear_);
    std::lock_guard<std::mutex> guard(remoteMutex_);
    remoteObject_ = nullptr;
}

void UsbRightManager::UsbAbilityConn::CloseDialog()
{
    std::lock_guard<std::mutex> guard(remoteMutex_);
    if (remoteObject_ == nullptr) {
        USB_HILOGW(MODULE_USB_HOST, "CloseDialog: disconnected");
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
    USB_HILOGI(MODULE_USB_HOST, "CloseDialog: ret=%{public}d, %{public}d, %{public}d", ret, success, replyCode);
}
} // namespace USB
} // namespace OHOS
