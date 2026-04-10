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

#ifndef USB_FUNCTION_MANAGER_H
#define USB_FUNCTION_MANAGER_H

#include <map>
#include <string>
#include <vector>
#include "timer.h"

#include "usb_common.h"
#include "usb_function_switch_window.h"
#include "usb_srv_support.h"
#include "v1_0/iusb_interface.h"
#include "v1_0/iusbd_subscriber.h"
#include "usb_timer_wraper.h"
#ifdef USB_MANAGER_V2_0
#include "usb_manager_subscriber.h"
#include "v2_0/iusb_device_interface.h"
#include "v2_1/iusb_device_interface.h"
#include "mem_mgr_proxy.h"
#include "mem_mgr_client.h"
#include "system_ability_definition.h"
#endif // USB_MANAGER_V2_0

#define USB_FUNCTION_HDC     (1 << 2)
#define USB_FUNCTION_MTP     (1 << 3)
#define USB_FUNCTION_PTP     (1 << 4)
#define USB_FUNCTION_STORAGE     (1 << 9)
#define USB_FUNCTION_DEVMODE_AUTH     (1 << 12)
namespace OHOS {
namespace USB {
class UsbDeviceManager {
public:
#ifdef USB_MANAGER_V2_0
    bool InitUsbDeviceInterface();
    void Stop();
    int32_t BindUsbdSubscriber(const sptr<HDI::Usb::V2_0::IUsbdSubscriber> &subscriber);
    int32_t UnbindUsbdSubscriber(const sptr<HDI::Usb::V2_0::IUsbdSubscriber> &subscriber);
    void ProcessCustomControlRequestUevent(int32_t status);
#endif // USB_MANAGER_V2_0
    UsbDeviceManager();
    int32_t Init();
    void SetPhyConnectState(bool phyConnect);
    static bool IsSettableFunctions(int32_t funcs);

    int32_t SetUsbd(const sptr<HDI::Usb::V1_0::IUsbInterface> &usbd);
    static uint32_t ConvertFromString(std::string_view funcs);
    static std::string ConvertToString(uint32_t func);
    void UpdateFunctions(int32_t func);
    int32_t GetCurrentFunctions();
    void HandleEvent(int32_t status);
    void GetDumpHelp(int32_t fd);
    void Dump(int32_t fd, const std::vector<std::string> &args);
    bool IsGadgetConnected(void);
    int32_t UserChangeProcess();
    void SetChargeFlag(bool isReverseCharge);
    int32_t GetCurrentFunctions(int32_t& funcs);
    int32_t SetCurrentFunctions(int32_t funcs);
private:
    void ProcessFunctionSwitchWindow(bool connected);
    void ProcessFunctionNotifier(bool connected, int32_t func);
    void DumpGetSupportFunc(int32_t fd);
    void DumpSetFunc(int32_t fd, const std::string &args);
    void ReportUsbOperationFaultSysEvent(const std::string &operationType, int32_t failReason,
        const std::string &failDescription);
    void ReportFuncChangeSysEvent(int32_t currentFunctions, int32_t updateFunctions);
    void ReportDevicePlugSysEvent(int32_t currentFunctions, bool connected);
    void ProcessFuncChange(bool connected, int32_t currentFunc);
    void BroadcastFuncChange(bool connected, int32_t currentFunc);
    void ProcessStatus(int32_t status, bool &curConnect);
    static constexpr uint32_t functionSettable_ = UsbSrvSupport::FUNCTION_HDC | UsbSrvSupport::FUNCTION_ACM |
        UsbSrvSupport::FUNCTION_ECM | UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_PTP |
        UsbSrvSupport::FUNCTION_RNDIS | UsbSrvSupport::FUNCTION_NCM | UsbSrvSupport::FUNCTION_STORAGE;
    static const std::map<std::string_view, uint32_t> FUNCTION_MAPPING_N2C;
    int32_t currentFunctions_ {UsbSrvSupport::FUNCTION_HDC};
    bool connected_ {false};
    bool isReverseCharge_ {false};
    bool hasHdcNotifier_ {false};
    bool gadgetConnected_ {false};
    bool isDisableDialog_ {false};
    sptr<HDI::Usb::V1_0::IUsbInterface> usbd_ = nullptr;
    uint32_t delayDisconnTimerId_ {UINT32_MAX};
    std::mutex functionMutex_;
    bool phyConnect_ {false};
#ifdef USB_MANAGER_V2_0
    int32_t GetCustomControlRequestData(int32_t id, std::vector<uint8_t> &data);

    sptr<HDI::Usb::V2_0::IUsbDeviceInterface> usbDeviceInterface_ = nullptr;
    sptr<HDI::Usb::V2_1::IUsbDeviceInterface> usbDeviceInterfaceV2_1_ = nullptr;
    sptr<UsbManagerSubscriber> usbManagerSubscriber_;
#endif // USB_MANAGER_V2_0
};
} // namespace USB
} // namespace OHOS

#endif // USB_FUNCTION_MANAGER_H
