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

#include "usb_settings_datashare.h"

#include "hilog_wrapper.h"
#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "iservice_registry.h"
#include "uri.h"
#include "singleton.h"

namespace OHOS {
namespace USB {
namespace {
constexpr const char *SETTINGS_DATASHARE_URI =
    "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
const std::string SETTINGS_DATASHARE_EXT_URI = "datashare:///com.ohos.settingsdata.DataAbility";
constexpr const char *SETTINGS_DATA_COLUMN_KEYWORD = "KEYWORD";
constexpr const char *SETTINGS_DATA_COLUMN_VALUE = "VALUE";
}
 
UsbSettingDataShare::UsbSettingDataShare()
{
    datashareHelper_ = CreateDataShareHelper(USB_SYSTEM_ABILITY_ID);
}
 
UsbSettingDataShare::~UsbSettingDataShare()
{
    if (datashareHelper_ != nullptr) {
        USB_HILOGI(MODULE_USB_UTILS, "datashareHelper_ is here Release!");
        datashareHelper_->Release();
    }
}

std::shared_ptr<DataShare::DataShareHelper> UsbSettingDataShare::CreateDataShareHelper(int systemAbilityId)
{
    USB_HILOGI(MODULE_USB_UTILS, "CreateDataShareHelper start!");
    sptr<ISystemAbilityManager> saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        USB_HILOGE(MODULE_USB_UTILS, "%{public}s: GetSystemAbilityManager failed!", __func__);
        return nullptr;
    }
    sptr<IRemoteObject> remote = saManager->GetSystemAbility(systemAbilityId);
    if (remote == nullptr) {
        USB_HILOGE(MODULE_USB_UTILS, "%{public}s: GetSystemAbility Service Failed!", __func__);
        return nullptr;
    }
    USB_HILOGI(MODULE_USB_UTILS, "%{public}s: systemAbilityId = %{public}d", __func__, systemAbilityId);
    auto [ret, helper] = DataShare::DataShareHelper::Create(remote, SETTINGS_DATASHARE_URI, SETTINGS_DATASHARE_EXT_URI);
    if (ret == DataShare::E_OK) {
        return helper;
    } else if (ret == DataShare::E_DATA_SHARE_NOT_READY) {
        USB_HILOGE(MODULE_USB_UTILS, "%{public}s: data is not ready!", __func__);
        return nullptr;
    } else {
        USB_HILOGE(MODULE_USB_UTILS, "%{public}s: create datashare failed, ret = %{public}d.", __func__, ret);
        return nullptr;
    }
}
 
bool UsbSettingDataShare::Query(Uri &uri, const std::string &key, std::string &value)
{
    USB_HILOGI(MODULE_USB_UTILS, "start Query key = %{public}s", key.c_str());
    if (datashareHelper_ == nullptr) {
        USB_HILOGE(MODULE_USB_UTILS, "query error, datashareHelper_ is nullptr");
        return false;
    }
 
    std::vector<std::string> columns;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SETTINGS_DATA_COLUMN_KEYWORD, key);
    auto result = datashareHelper_->Query(uri, predicates, columns);
    if (result == nullptr) {
        USB_HILOGE(MODULE_USB_UTILS, "query error, result is nullptr");
        return false;
    }
 
    if (result->GoToFirstRow() != DataShare::E_OK) {
        USB_HILOGE(MODULE_USB_UTILS, "query error, go to first row error");
        result->Close();
        return true;
    }
 
    int columnIndex = 0;
    result->GetColumnIndex(SETTINGS_DATA_COLUMN_VALUE, columnIndex);
    result->GetString(columnIndex, value);
    result->Close();
    USB_HILOGI(MODULE_USB_UTILS, "SettingUtils: query success");
    return true;
}

bool UsbSettingDataShare::OobeQuery(Uri &uri, const std::string &key, std::string &value)
{
    USB_HILOGI(MODULE_USB_UTILS, "start Query key = %{public}s", key.c_str());
    if (datashareHelper_ == nullptr) {
        USB_HILOGE(MODULE_USB_UTILS, "query error, datashareHelper_ is nullptr");
        return false;
    }
 
    std::vector<std::string> columns;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SETTINGS_DATA_COLUMN_KEYWORD, key);
    auto result = datashareHelper_->Query(uri, predicates, columns);
    if (result == nullptr) {
        USB_HILOGE(MODULE_USB_UTILS, "query error, result is nullptr");
        return false;
    }
 
    if (result->GoToFirstRow() != DataShare::E_OK) {
        USB_HILOGE(MODULE_USB_UTILS, "query error, go to first row error");
        result->Close();
        return true;
    }
 
    int columnIndex = 0;
    result->GetColumnIndex(SETTINGS_DATA_COLUMN_VALUE, columnIndex);
    result->GetString(columnIndex, value);
    result->Close();
    USB_HILOGI(MODULE_USB_UTILS, "SettingUtils: query success");
    return true;
}

bool UsbSettingDataShare::Insert(Uri uri, const std::string &key, std::string &value)
{
    USB_HILOGI(MODULE_USB_UTILS, "start Insert key = %{public}s", key.c_str());
    if (datashareHelper_ == nullptr) {
        USB_HILOGE(MODULE_USB_UTILS, "query error, datashareHelper_ is nullptr");
        return false;
    }
    DataShare::DataShareValuesBucket valuesBucket;
    DataShare::DataShareValueObject keyObj(key);
    DataShare::DataShareValueObject valueObj(value);
    valuesBucket.Put(SETTINGS_DATA_COLUMN_KEYWORD, keyObj);
    valuesBucket.Put(SETTINGS_DATA_COLUMN_VALUE, valueObj);
    int32_t result = datashareHelper_->Insert(uri, valuesBucket);
    if (result == RDB_INVALID_VALUE) {
        datashareHelper_->Release();
        return false;
    }
    USB_HILOGI(MODULE_USB_UTILS, "UsbSettingDataShare: insert success");
    datashareHelper_->NotifyChange(uri);
    datashareHelper_->Release();
    return true;
}

bool UsbSettingDataShare::Update(Uri uri, const std::string &key, std::string &value)
{
    if (datashareHelper_ == nullptr) {
        USB_HILOGE(MODULE_USB_UTILS, "query error, datashareHelper_ is nullptr");
        return false;
    }
    USB_HILOGI(MODULE_USB_UTILS, "start Update key = %{public}s", key.c_str());
    std::string queryValue = "";
    if (!Query(uri, key, queryValue)) {
        USB_HILOGI(MODULE_USB_UTILS, "%{public}s is cannot Query!", key.c_str());
        return Insert(uri, key, value);
    }
    DataShare::DataShareValuesBucket valuesBucket;
    DataShare::DataShareValueObject valueObj(value);
    valuesBucket.Put(SETTINGS_DATA_COLUMN_VALUE, valueObj);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SETTINGS_DATA_COLUMN_KEYWORD, key);
    int32_t result = datashareHelper_->Update(uri, predicates, valuesBucket);
    if (result == RDB_INVALID_VALUE) {
        USB_HILOGE(MODULE_USB_UTILS, "fail to update %{public}s !", key.c_str());
        datashareHelper_->Release();
        return false;
    }
    USB_HILOGI(MODULE_USB_UTILS, "UsbSettingDataShare: update success");
    datashareHelper_->NotifyChange(uri);
    datashareHelper_->Release();
    return true;
}
} // namespace USB
} // namespace OHOS
