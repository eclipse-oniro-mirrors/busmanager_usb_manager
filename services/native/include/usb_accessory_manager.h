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

#ifndef USB_ACCESSORY_MANAGER_H
#define USB_ACCESSORY_MANAGER_H
#include <map>
#include <string>
#include <vector>
#include "timer.h"

#include "usb_accessory.h"
#include "usb_common.h"
#include "usb_srv_support.h"
#include "v1_2/iusb_interface.h"
#include "v1_0/iusb_interface.h"
#include "v1_0/iusbd_subscriber.h"
#include "delayed_sp_singleton.h"
#include "usb_right_manager.h"
#include "usb_timer_wraper.h"
#ifdef USB_MANAGER_V2_0
#include "v2_0/iusb_device_interface.h"
#endif // USB_MANAGER_V2_0
namespace OHOS {
namespace USB {

enum ACCESSORYMODE : int32_t {
    ACC_NONE,
    ACC_CONFIGURING,
    ACC_START,
    ACC_STOP,
    ACC_SEND,
};

class UsbAccessoryManager {
public:
    UsbAccessoryManager();
    void HandleEvent(int32_t status, bool delayProcess = true);
    int32_t SetUsbd(const sptr<OHOS::HDI::Usb::V1_2::IUsbInterface> usbd);
    void GetAccessoryList(const std::string &bundleName, std::vector<USBAccessory> &accessoryList);
    int32_t GetAccessorySerialNumber(const USBAccessory &access, const std::string &bundleName,
        std::string &serialValue);
    int32_t OpenAccessory(int32_t &fd);
    int32_t CloseAccessory(int32_t fd);
#ifdef USB_MANAGER_V2_0
    bool InitUsbAccessoryInterface();
#endif // USB_MANAGER_V2_0
private:
    void GetAccessoryInfo(std::vector<std::string> &accessorys);
    int32_t SetCurrentFunctions(int32_t funcs);
    int32_t GetCurrentFunctions(int32_t &funcs);
    void ProcessHandle(int32_t curAccStatus);
    int32_t ProcessAccessoryStart(int32_t curFunc, int32_t curAccStatus);
    int32_t ProcessAccessoryStop(int32_t curFunc, int32_t curAccStatus);
    int32_t ProcessAccessorySend();
    void InitBase64Map();
    std::vector<uint8_t> Base64Decode(const std::string& encoded_string);
    std::string SerialValueHash(const std::string &serialValue);
    bool compare(const std::string &s1, const std::string &s2);
    USBAccessory accessory;
    int32_t accStatus_ {ACC_NONE};
    int32_t eventStatus_ = 0;
    int32_t curDeviceFunc_ = {UsbSrvSupport::FUNCTION_NONE};
    int32_t lastDeviceFunc_ = {UsbSrvSupport::FUNCTION_NONE};
    int32_t accFd_ = {0};
    uint32_t accDelayTimerId_ {UINT32_MAX};
    uint32_t antiShakeDelayTimerId_ {UINT32_MAX};
    sptr<HDI::Usb::V1_2::IUsbInterface> usbdImpl_ = nullptr;
    std::map<char, int> base64Map_;
    std::mutex mutexHandleEvent_;
    std::mutex accFdMutex_;
#ifdef USB_MANAGER_V2_0
    sptr<HDI::Usb::V2_0::IUsbDeviceInterface> usbDeviceInterface_ = nullptr;
#endif // USB_MANAGER_V2_0
};

} // USB
} // OHOS
#endif // USB_ACCESSORY_MANAGER_H
