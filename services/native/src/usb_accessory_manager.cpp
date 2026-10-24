/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024. All rights reserved.
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
#include "usb_accessory_manager.h"

#include <iostream>
#include <functional>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <hdf_base.h>

#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "hisysevent.h"
#include "usb_errors.h"
#include "usb_srv_support.h"
#include "usbd_type.h"
#include "cJSON.h"
using namespace OHOS::AAFwk;
using namespace OHOS::EventFwk;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HDI::Usb::V1_0;
using namespace OHOS::HDI::Usb::V1_2;

namespace OHOS {
namespace USB {
constexpr int32_t ACCESSORY_INFO_SIZE = 5;
constexpr uint32_t ACCESSORY_EXTRA_INDEX = 5;
constexpr uint32_t FUN_ACCESSORY = 1 << 11;
constexpr int32_t NUM_OF_SERAIL_BIT = 16;
constexpr uint32_t DELAY_ACC_INTERVAL = 10 * 1000;
constexpr uint32_t ANTI_SHAKE_INTERVAL = 1 * 1000;
constexpr int32_t ACCESSORY_IS_BUSY = -16;
const int INDEX_FIRST = 0;
const int INDEX_SECOND = 1;
const int INDEX_THIRD = 2;
const int INDEX_FORTH = 3;
const int INDEX_FIFTH = 4;
const uint32_t OFFSET2 = 2;
const uint32_t OFFSET4 = 4;
const uint32_t OFFSET6 = 6;
const std::string BASE_64_CHARS =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";
const char *ACCESSORY_DRIVER_PATH = "/dev/usb_accessory";

// LCOV_EXCL_START
UsbAccessoryManager::UsbAccessoryManager()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager::Init start");
#ifndef USB_MANAGER_V2_0
    usbdImpl_ = OHOS::HDI::Usb::V1_2::IUsbInterface::Get();
    if (usbdImpl_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbDeviceManager::Get inteface failed");
    }
#endif // USB_MANAGER_V2_0
}

#ifdef USB_MANAGER_V2_0
bool UsbAccessoryManager::InitUsbAccessoryInterface()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbAccessoryManager::InitUsbAccessoryInterface start");
    usbDeviceInterface_ = HDI::Usb::V2_0::IUsbDeviceInterface::Get();
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager:: Get inteface failed");
        return false;
    }
    return true;
}
#endif // USB_MANAGER_V2_0

void UsbAccessoryManager::GetAccessoryInfo(std::vector<std::string> &accessorys)
{
#ifdef USB_MANAGER_V2_0
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager usbDeviceInterface_ is nullptr.");
        return;
    }
    usbDeviceInterface_->GetAccessoryInfo(accessorys);
#else
    if (usbdImpl_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager usbdImpl_ is nullptr.");
        return;
    }
    usbdImpl_->GetAccessoryInfo(accessorys);
#endif // USB_MANAGER_V2_0
}

int32_t UsbAccessoryManager::SetCurrentFunctions(int32_t funcs)
{
#ifdef USB_MANAGER_V2_0
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager usbDeviceInterface_ is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbDeviceInterface_->SetCurrentFunctions(funcs);
#else
    if (usbdImpl_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager usbdImpl_ is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbdImpl_->SetCurrentFunctions(funcs);
#endif // USB_MANAGER_V2_0
}

int32_t UsbAccessoryManager::GetCurrentFunctions(int32_t &funcs)
{
#ifdef USB_MANAGER_V2_0
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager usbDeviceInterface_ is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbDeviceInterface_->GetCurrentFunctions(funcs);
#else
    if (usbdImpl_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager usbdImpl_ is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    return usbdImpl_->GetCurrentFunctions(funcs);
#endif // USB_MANAGER_V2_0
}

int32_t UsbAccessoryManager::SetUsbd(const sptr<OHOS::HDI::Usb::V1_2::IUsbInterface> usbd)
{
    if (usbd == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager usbd is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    usbdImpl_ = usbd;
    return UEC_OK;
}

void UsbAccessoryManager::GetAccessoryList(const std::string &bundleName,
    std::vector<USBAccessory> &accessoryList)
{
    std::lock_guard<std::mutex> guard(mutexHandleEvent_);
    if (accStatus_ == ACC_START) {
        USBAccessory access = this->accessory;
        access.SetSerialNumber(bundleName + this->accessory.GetSerialNumber());
        accessoryList.push_back(access);
    }
    USB_HILOGW(MODULE_USB_SERVICE, "%{public}s: status is not ACC_START", __func__);
    return;
}

int32_t UsbAccessoryManager::OpenAccessory(int32_t &fd)
{
    int32_t ret = UEC_INTERFACE_INVALID_VALUE;
#ifdef USB_MANAGER_V2_0
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager usbDeviceInterface_ is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    ret = usbDeviceInterface_->OpenAccessory(fd);
#else
    if (usbdImpl_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager usbdImpl_ is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    ret = usbdImpl_->OpenAccessory(fd);
#endif // USB_MANAGER_V2_0
    if (ret == UEC_OK) {
        std::lock_guard<std::mutex> guard(accFdMutex_);
        accFd_ = fd;
        return ret;
    } else if (ret == ACCESSORY_IS_BUSY) {
        return UEC_SERVICE_ACCESSORY_REOPEN;
    }
    return UEC_SERVICE_ACCESSORY_OPEN_NATIVE_NODE_FAILED;
}

int32_t UsbAccessoryManager::CloseAccessory(int32_t fd)
{
    int32_t ret = UEC_INTERFACE_INVALID_VALUE;
#ifdef USB_MANAGER_V2_0
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager usbDeviceInterface_ is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t newFd = open(ACCESSORY_DRIVER_PATH, O_RDWR);
    fdsan_exchange_owner_tag(newFd, 0, fdsan_create_owner_tag(FDSAN_OWNER_TYPE_FILE, LOG_DOMAIN));
    ret = usbDeviceInterface_->CloseAccessory(newFd);
    fdsan_close_with_tag(newFd, fdsan_create_owner_tag(FDSAN_OWNER_TYPE_FILE, LOG_DOMAIN));
#else
    if (usbdImpl_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager usbdImpl_ is nullptr.");
        return UEC_SERVICE_INVALID_VALUE;
    }
    int32_t localAccFd = 0;
    {
        std::lock_guard<std::mutex> guard(accFdMutex_);
        localAccFd = accFd_;
    }
    ret = usbdImpl_->CloseAccessory(localAccFd);
#endif // USB_MANAGER_V2_0
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "%{public}s, close ret: %{public}d, fd: %{public}d.", __func__, ret, fd);
        return ret;
    }
    {
        std::lock_guard<std::mutex> guard(accFdMutex_);
        accFd_ = 0;
    }
    return ret;
}

int32_t UsbAccessoryManager::ProcessAccessoryStart(int32_t curFunc, int32_t curAccStatus)
{
    uint32_t curFuncUint = static_cast<uint32_t>(curFunc);
    if ((curFuncUint & FUN_ACCESSORY) != 0 && accStatus_ != ACC_START) {
        this->accStatus_ = ACC_START;
        std::vector<std::string> accessorys;
        GetAccessoryInfo(accessorys);
        this->accessory.SetAccessory(accessorys);
        std::string hashSerial = SerialValueHash(this->accessory.GetSerialNumber());
        this->accessory.SetSerialNumber(hashSerial);
        Want want;
        want.SetAction(CommonEventSupport::COMMON_EVENT_USB_ACCESSORY_ATTACHED);
        CommonEventData data(want);
        data.SetData(this->accessory.GetJsonString().c_str());
        CommonEventPublishInfo publishInfo;
        USB_HILOGI(MODULE_USB_SERVICE, "send accessory attached broadcast device:%{public}s",
            this->accessory.GetJsonString().c_str());
        return CommonEventManager::PublishCommonEvent(data, publishInfo);
    } else if ((curFuncUint & FUN_ACCESSORY) == 0 && curAccStatus == ACC_CONFIGURING) {
        int32_t ret = SetCurrentFunctions(FUN_ACCESSORY);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_SERVICE, "curFunc %{public}d curAccStatus:%{public}u, set func ret: %{public}d",
                curFuncUint, curAccStatus, ret);
            return ret;
        }
        lastDeviceFunc_ = static_cast<int32_t>(curFuncUint);
        auto task = [&]() {
            this->accStatus_ = ACC_STOP;
            int32_t ret = SetCurrentFunctions(this->lastDeviceFunc_);
            if (ret != UEC_OK) {
                USB_HILOGW(MODULE_USB_SERVICE, "set old func: %{public}d ret: %{public}d", this->lastDeviceFunc_, ret);
            }
            return;
        };
        accDelayTimerId_ = UsbTimerWrapper::GetInstance()->Register(task, DELAY_ACC_INTERVAL, true);
        this->accStatus_ = ACC_CONFIGURING;
    } else {
        USB_HILOGD(MODULE_USB_SERVICE, "curFunc %{public}u curAccStatus:%{public}d not necessary",
            curFuncUint, curAccStatus);
    }
    return UEC_OK;
}

int32_t UsbAccessoryManager::ProcessAccessoryStop(int32_t curFunc, int32_t curAccStatus)
{
    uint32_t curFuncUint = static_cast<uint32_t>(curFunc);
    if ((curFuncUint & FUN_ACCESSORY) != 0 && accStatus_ == ACC_START) {
        accStatus_ = ACC_STOP;
        int32_t ret = SetCurrentFunctions(lastDeviceFunc_);
        if (ret != UEC_OK) {
            USB_HILOGW(MODULE_USB_SERVICE, "setFunc %{public}d curAccStatus:%{public}d, set func ret: %{public}d",
                lastDeviceFunc_, curAccStatus, ret);
        }
        curDeviceFunc_ = lastDeviceFunc_;
        Want want;
        want.SetAction(CommonEventSupport::COMMON_EVENT_USB_ACCESSORY_DETACHED);
        CommonEventData data(want);
        data.SetData(this->accessory.GetJsonString().c_str());
        CommonEventPublishInfo publishInfo;
        USB_HILOGI(MODULE_USB_SERVICE, "send accessory detached broadcast device:%{public}s",
            this->accessory.GetJsonString().c_str());
        return CommonEventManager::PublishCommonEvent(data, publishInfo);
    } else {
        USB_HILOGD(MODULE_USB_SERVICE, "curFunc %{public}u curAccStatus:%{public}d not necessary",
            curFuncUint, curAccStatus);
    }
    return UEC_OK;
}

int32_t UsbAccessoryManager::ProcessAccessorySend()
{
    this->accStatus_ = ACC_SEND;
    std::vector<std::string> accessorys;
    GetAccessoryInfo(accessorys);
    this->accessory.SetAccessory(accessorys);
    std::string extraInfo;
    if (accessorys.size() > ACCESSORY_INFO_SIZE &&
        ACCESSORY_EXTRA_INDEX < accessorys.size() && !accessorys[ACCESSORY_EXTRA_INDEX].empty()) {
        if (base64Map_.empty()) {
            InitBase64Map();
        }
        std::string extraEcode = accessorys[ACCESSORY_EXTRA_INDEX];
        USB_HILOGE(MODULE_USB_SERVICE, "extraEcode, length: %{public}zu, extraData: %{public}s",
            extraEcode.length(), extraEcode.c_str());
        std::vector<uint8_t> extraData = Base64Decode(extraEcode);
        cJSON *root = cJSON_CreateArray();
        for (uint8_t value : extraData) {
            cJSON_AddItemToArray(root, cJSON_CreateNumber(value));
        }
        char *pExtraJson = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        if (!pExtraJson) {
            USB_HILOGE(MODULE_USB_SERVICE, "print json error.");
            return UEC_SERVICE_INVALID_VALUE;
        }
        extraInfo = pExtraJson;
        cJSON_free(pExtraJson);
        pExtraJson = nullptr;
    }

    USBAccessory extraAcces;
    extraAcces.SetDescription(extraInfo);
    Want want;
    want.SetAction(CommonEventSupport::COMMON_EVENT_USB_ACCESSORY_ATTACHED);
    CommonEventData data(want);
    data.SetData(extraAcces.GetJsonString().c_str());
    CommonEventPublishInfo publishInfo;
    USB_HILOGI(MODULE_USB_SERVICE, "send accessory attached broadcast device:%{public}s",
        extraAcces.GetJsonString().c_str());
    CommonEventManager::PublishCommonEvent(data, publishInfo);
    return UEC_OK;
}

void UsbAccessoryManager::HandleEvent(int32_t status, bool delayProcess)
{
#ifdef USB_MANAGER_V2_0
    if (usbDeviceInterface_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager::usbDeviceInterface_ is nullptr");
        return;
    }
#else
    if (usbdImpl_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "UsbAccessoryManager::usbdImpl_ is nullptr");
        return;
    }
#endif // USB_MANAGER_V2_0
    std::lock_guard<std::mutex> guard(mutexHandleEvent_);
    eventStatus_ = status;
    if (delayProcess) {
        UsbTimerWrapper::GetInstance()->Unregister(antiShakeDelayTimerId_);
    }
    if ((status == ACT_UPDEVICE || status == ACT_DOWNDEVICE) && delayProcess) {
        auto task = [&]() {
            this->HandleEvent(this->eventStatus_, false);
            return;
        };
        antiShakeDelayTimerId_ = UsbTimerWrapper::GetInstance()->Register(task, ANTI_SHAKE_INTERVAL, true);
        return;
    }
    int32_t curAccStatus = ACC_NONE;
    switch (status) {
        case ACT_UPDEVICE: {
            if (accStatus_ == ACC_CONFIGURING) {
                curAccStatus = ACC_START;
            }
            break;
        }
        case ACT_ACCESSORYUP: {
            curAccStatus = ACC_CONFIGURING;
            break;
        }
        case ACT_DOWNDEVICE:
        case ACT_ACCESSORYDOWN: {
            curAccStatus = ACC_STOP;
            break;
        }
        case ACT_ACCESSORYSEND: {
            curAccStatus = ACC_SEND;
            break;
        }
        default:
            USB_HILOGE(MODULE_USB_SERVICE, "invalid status %{public}d", status);
    }
    ProcessHandle(curAccStatus);
}

void UsbAccessoryManager::ProcessHandle(int32_t curAccStatus)
{
    int32_t ret = UEC_INTERFACE_INVALID_VALUE;
    if ((curAccStatus == ACC_CONFIGURING || curAccStatus == ACC_START)) {
        UsbTimerWrapper::GetInstance()->Unregister(accDelayTimerId_);
        int32_t curFunc = 0;
        ret = GetCurrentFunctions(curFunc);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_SERVICE, "GetCurrentFunctions ret: %{public}d", ret);
            return;
        }
        this->curDeviceFunc_ = curFunc;
        ret = ProcessAccessoryStart(curFunc, curAccStatus);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_SERVICE, "ProcessAccessoryStart ret: %{public}d", ret);
            return;
        }
    } else if (curAccStatus == ACC_STOP && accStatus_ != ACC_CONFIGURING) {
        ret = ProcessAccessoryStop(curDeviceFunc_, curAccStatus);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_SERVICE, "ProcessAccessoryStop ret: %{public}d", ret);
            return;
        }
    } else if (curAccStatus == ACC_SEND) {
        ProcessAccessorySend();
    }
    return;
}

std::string UsbAccessoryManager::SerialValueHash(const std::string&serialValue)
{
    uint64_t timestamp = 0;
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    timestamp = static_cast<uint64_t>(millis);

    std::hash<std::string> string_hash;
    std::hash<uint64_t> int_hash;

    uint32_t hashValue = (string_hash(serialValue) ^ (int_hash(timestamp) << 1));

    std::stringstream ss;
    ss << std::hex << std::setw(NUM_OF_SERAIL_BIT) << std::setfill('0') << hashValue;
    return ss.str();
}

void UsbAccessoryManager::InitBase64Map()
{
    for (size_t i = 0; i < BASE_64_CHARS.size(); ++i) {
        base64Map_[BASE_64_CHARS[i]] = static_cast<int>(i);
    }
}

bool IsBase64(unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::vector<uint8_t> UsbAccessoryManager::Base64Decode(const std::string& basicString)
{
    std::string decoded_data;
    uint32_t i = 0;
    int index = 0;
    int len = static_cast<int>(basicString.size());
    unsigned char charArray3[INDEX_FORTH];
    unsigned char charArray4[INDEX_FIFTH];

    while (len-- && (basicString[index] != '=') && IsBase64(basicString[index])) {
        charArray4[i++] = basicString[index];
        index++;
        if (i == sizeof(charArray4)) {
            for (i = 0; i < sizeof(charArray4); i++) {
                charArray4[i] = BASE_64_CHARS.find(charArray4[i]);
            }
            charArray3[INDEX_FIRST] = (charArray4[INDEX_FIRST] << OFFSET2) +
                ((charArray4[INDEX_SECOND] & 0x30) >> OFFSET4);
            charArray3[INDEX_SECOND] = ((charArray4[INDEX_SECOND] & 0xf) << OFFSET4) +
                ((charArray4[INDEX_THIRD] & 0x3c) >> OFFSET2);
            charArray3[INDEX_THIRD] = ((charArray4[INDEX_THIRD] & 0x3) << OFFSET6) + charArray4[INDEX_FORTH];
            for (i = 0; i < sizeof(charArray3); i++) {
                decoded_data += charArray3[i];
            }
            i = 0;
        }
    }

    if (i) {
        uint32_t j = 0;
        for (j = i; j < sizeof(charArray4); j++) {
            charArray4[j] = 0;
        }
        for (j = 0; j < sizeof(charArray4); j++) {
            charArray4[j] = BASE_64_CHARS.find(charArray4[j]);
        }
        charArray3[INDEX_FIRST] = (charArray4[INDEX_FIRST] << OFFSET2) +
            ((charArray4[INDEX_SECOND] & 0x30) >> OFFSET4);
        charArray3[INDEX_SECOND] = ((charArray4[INDEX_SECOND] & 0xf) << OFFSET4) +
            ((charArray4[INDEX_THIRD] & 0x3c) >> OFFSET2);
        charArray3[INDEX_THIRD] = ((charArray4[INDEX_THIRD] & 0x3) << OFFSET6) + charArray4[INDEX_FORTH];
        for (j = 0; j < i - 1; j++) {
            decoded_data += charArray3[j];
        }
    }

    std::vector<uint8_t> ret;
    for (char c : decoded_data) {
        ret.push_back(static_cast<uint8_t>(c));
    }

    return ret;
}

bool UsbAccessoryManager::compare(const std::string &s1, const std::string &s2)
{
    if (s1.empty())
        return (s2.empty());
    return s1 == s2;
}

int32_t UsbAccessoryManager::GetAccessorySerialNumber(const USBAccessory &access,
    const std::string &bundleName, std::string &serialValue)
{
    USB_HILOGD(MODULE_USB_SERVICE, "%{public}s, bundleName: %{public}s, serial: %{public}s",
        __func__, bundleName.c_str(), this->accessory.GetSerialNumber().c_str());
    std::lock_guard<std::mutex> guard(mutexHandleEvent_);
    if (accStatus_ != ACC_START) {
        USB_HILOGE(MODULE_USB_SERVICE, "invalid status %{public}d", accStatus_);
        return UEC_SERVICE_ACCESSORY_NOT_MATCH;
    } else if (compare(this->accessory.GetManufacturer(), access.GetManufacturer()) &&
        (compare(this->accessory.GetProduct(), access.GetProduct())) &&
        compare(this->accessory.GetDescription(), access.GetDescription()) &&
        compare(this->accessory.GetManufacturer(), access.GetManufacturer()) &&
        compare(this->accessory.GetVersion(), access.GetVersion()) &&
        (compare(this->accessory.GetSerialNumber(), access.GetSerialNumber()) ||
        compare(bundleName + this->accessory.GetSerialNumber(), access.GetSerialNumber()))) {
        serialValue = access.GetManufacturer() + access.GetProduct() + access.GetVersion() + access.GetSerialNumber();
        return UEC_OK;
    }
    return UEC_SERVICE_ACCESSORY_NOT_MATCH;
}
// LCOV_EXCL_STOP

} // USB
} // OHOS
