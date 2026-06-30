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

#ifndef USBMGR_USB_SERVICE_H
#define USBMGR_USB_SERVICE_H

#include <map>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>

#include "delayed_sp_singleton.h"
#include "iremote_object.h"
#include "iusb_srv.h"
#include "usb_interface_type.h"
#include "system_ability.h"
#include "system_ability_status_change_stub.h"
#include "timer.h"
#include "usb_device_manager.h"
#include "usb_accessory_manager.h"
#include "usb_host_manager.h"
#include "usb_port_manager.h"
#include "usb_right_manager.h"
#include "usb_server_stub.h"
#include "usb_service_subscriber.h"
#include "usbd_type.h"
#include "usb_serial_type.h"
#include "v1_2/iusb_interface.h"
#include "v1_0/iusbd_bulk_callback.h"
#include "v1_0/iusbd_subscriber.h"
#include "v1_1/usb_types.h"
#include "serial_manager.h"
#include "v1_2/usb_types.h"
#include "usbd_bulkcallback_impl.h"
#include "usb_bulk_trans_data.h"
#ifdef USB_MANAGER_PASS_THROUGH
#include "v2_0/iusb_host_interface.h"
#endif // USB_MANAGER_PASS_THROUGH

namespace OHOS {
namespace USB {
const std::string USB_HOST = "usb_host";
const std::string USB_DEVICE = "usb_device";
const std::string USB_PORT = "usb_port";
const std::string USB_HELP = "-h";
const std::string USB_LIST = "-l";
const std::string USB_GETT = "-g";
const int32_t ERRCODE_NEGATIVE_ONE = -1;
const int32_t ERRCODE_NEGATIVE_TWO = -2;
const int32_t ERRCODE_NEGATIVE_FOUR = -4;
const int32_t ERRCODE_NEGATIVE_ELEVEN = -11;
const int32_t ERRCODE_NEGATIVE_TWELVE = -12;
const int32_t IO_ERROR = -1;
const int32_t INVALID_PARAM = -2;
const int32_t NO_DEVICE = -4;
const int32_t NO_MEM = -11;
const int32_t NOT_SUPPORT = -12;
class UsbService : public SystemAbility, public UsbServerStub {
    DECLARE_SYSTEM_ABILITY(UsbService)
    DECLARE_DELAYED_SP_SINGLETON(UsbService);

public:
    enum UnLoadSaType { UNLOAD_SA_DELAY, UNLOAD_SA_IMMEDIATELY };
    void OnStart() override;
    void OnStop() override;
    int Dump(int fd, const std::vector<std::u16string> &args) override;

    bool IsServiceReady() const
    {
        return ready_;
    }
    static sptr<UsbService> GetGlobalInstance();
#ifndef USB_MANAGER_PASS_THROUGH
    int32_t SetUsbd(const sptr<HDI::Usb::V1_2::IUsbInterface> &usbd);
#endif // USB_MANAGER_PASS_THROUGH
    int32_t CheckSysApiPermission();
    int32_t GetHapApiVersion();
    void UnLoadSelf(UnLoadSaType type);
    int32_t DeviceEvent(const HDI::Usb::V1_0::USBDeviceInfo &info);
#ifdef USB_MANAGER_FEATURE_HOST
    int32_t OpenDevice(uint8_t busNum, uint8_t devAddr) override;
    int32_t Close(uint8_t busNum, uint8_t devAddr) override;
    int32_t ResetDevice(uint8_t busNum, uint8_t devAddr) override;
    int32_t ClaimInterface(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid, uint8_t force) override;
    int32_t SetInterface(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid, uint8_t altIndex) override;
    int32_t ReleaseInterface(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid) override;
    int32_t SetActiveConfig(uint8_t busNum, uint8_t devAddr, uint8_t configIndex) override;
    int32_t ManageGlobalInterface(bool disable) override;
    int32_t ManageDevice(int32_t vendorId, int32_t productId, bool disable) override;
    int32_t ManageDevicePolicy(const std::vector<UsbDeviceIdInfo> &whilteList) override;
    int32_t ManageInterfaceType(const std::vector<UsbDeviceTypeInfo> &devTypeInfo, bool disable) override;
    int32_t ManageUsbType(const std::vector<UsbDeviceTypeInfo> &devTypeInfo, bool disable) override;
    int32_t ManageUsbSerialDevice(bool disable);
    int32_t UsbAttachKernelDriver(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid) override;
    int32_t UsbDetachKernelDriver(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid) override;
    int32_t ClearHalt(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId, uint8_t endpointId) override;

    bool AddDevice(uint8_t busNum, uint8_t devAddr);
    bool DelDevice(uint8_t busNum, uint8_t devAddr);
    int32_t GetDevices(std::vector<UsbDevice> &deviceList) override;
    int32_t GetDeviceInfo(uint8_t busNum, uint8_t devAddr, UsbDevice &dev);
    int32_t GetDeviceInfoDescriptor(
        const HDI::Usb::V1_0::UsbDev &uDev, std::vector<uint8_t> &descriptor, UsbDevice &dev);
    int32_t GetConfigDescriptor(UsbDevice &dev, std::vector<uint8_t> &descriptor);
    int32_t GetActiveConfig(uint8_t busNum, uint8_t devAddr, uint8_t &configIndex) override;
    int32_t GetRawDescriptor(uint8_t busNum, uint8_t devAddr, std::vector<uint8_t> &bufferData) override;
    int32_t GetFileDescriptor(uint8_t busNum, uint8_t devAddr, int32_t &fd) override;
    int32_t GetDeviceSpeed(uint8_t busNum, uint8_t devAddr, uint8_t &speed) override;
    int32_t GetInterfaceActiveStatus(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid, bool &unactivated) override;
    bool GetDeviceProductName(const std::string &deviceName, std::string &productName);

    int32_t BulkTransferRead(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep,
        UsbBulkTransData &bufferData, int32_t timeOut) override;
    int32_t BulkTransferReadwithLength(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep,
        int32_t length, UsbBulkTransData &bufferData, int32_t timeOut) override;
    int32_t BulkTransferWrite(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep,
        const UsbBulkTransData &bufferData, int32_t timeOut) override;
    int32_t ControlTransfer(uint8_t busNum, uint8_t devAddr, const UsbCtlSetUp& ctrlParams,
        std::vector<uint8_t> &bufferData) override;
    int32_t UsbControlTransfer(uint8_t busNum, uint8_t devAddr,
        const UsbCtlSetUp& ctrlParams, std::vector<uint8_t> &bufferData) override;
    int32_t RequestQueue(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep,
        const std::vector<uint8_t> &clientData, const std::vector<uint8_t> &bufferData) override;
    int32_t RequestWait(uint8_t busNum, uint8_t devAddr, int32_t timeOut, std::vector<uint8_t> &clientData,
        std::vector<uint8_t> &bufferData) override;
    int32_t RequestCancel(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid, uint8_t endpointId) override;
    int32_t UsbCancelTransfer(uint8_t busNum, uint8_t devAddr, int32_t endpoint) override;
    int32_t UsbSubmitTransfer(uint8_t busNum, uint8_t devAddr, const UsbTransInfo &param,
        const sptr<IRemoteObject> &cb, int32_t fd, int32_t memSize) override;
    int32_t RegBulkCallback(uint8_t busNum, uint8_t devAddr, const USBEndpoint& ep,
            const sptr<IRemoteObject> &cb) override;
    int32_t UnRegBulkCallback(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep) override;
    int32_t BulkRead(
        uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep, int32_t fd, int32_t memSize) override;
    int32_t BulkWrite(
        uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep, int32_t fd, int32_t memSize) override;
    int32_t BulkCancel(uint8_t busNum, uint8_t devAddr, const USBEndpoint &ep) override;

    bool CheckDevicePermission(uint8_t busNum, uint8_t devAddr);
    bool HasRight(const std::string &deviceName);
    int32_t HasRight(const std::string &deviceName, bool &hasRight) override;
    int32_t RequestRight(const std::string &deviceName) override;
    int32_t RemoveRight(const std::string &deviceName) override;
    int32_t AddAccessRight(const std::string &tokenId, const std::string &deviceName) override;
#endif // USB_MANAGER_FEATURE_HOST
#ifdef USB_MANAGER_FEATURE_DEVICE
    int32_t GetCurrentFunctions(int32_t &funcs) override;
    int32_t SetCurrentFunctions(int32_t funcs) override;
    int32_t UsbFunctionsFromString(const std::string &funcs, int32_t &funcResult) override;
    int32_t UsbFunctionsToString(int32_t funcs, std::string& funcResult) override;
    void UpdateDeviceState(int32_t status);
    void SetPhyConnect(bool phyConnect);
    int32_t UserChangeProcess();
    int32_t GetAccessoryList(std::vector<USBAccessory> &accessList, const bool isCliTool = false) override;
    int32_t OpenAccessory(const USBAccessory &access, int32_t &fd) override;
    int32_t CloseAccessory(int32_t fd) override;
    int32_t AddAccessoryRight(const uint32_t tokenId, const USBAccessory &access) override;
    int32_t HasAccessoryRight(const USBAccessory &access, bool &result) override;
    int32_t RequestAccessoryRight(const USBAccessory &access, bool &result) override;
    int32_t CancelAccessoryRight(const USBAccessory &access) override;
    bool InitSettingsDataHdcStatus();
    bool SetSettingsDataHdcStatus(int32_t func_uint);
#endif // USB_MANAGER_FEATURE_DEVICE
#ifdef USB_MANAGER_FEATURE_PORT
    int32_t GetPorts(std::vector<UsbPort> &ports) override;
    int32_t GetSupportedModes(int32_t portId, int32_t &result) override;
    int32_t SetPortRole(int32_t portId, int32_t powerRole, int32_t dataRole) override;
    void UpdateUsbPort(int32_t portId, int32_t powerRole, int32_t dataRole, int32_t mode);
    void UpdateUsbPort(int32_t portId, int32_t powerRole, int32_t dataRole, int32_t mode, int32_t supportedModes);
#endif // USB_MANAGER_FEATURE_PORT

    int32_t SerialOpen(int32_t portId, const sptr<IRemoteObject> &serialRemote) override;
    int32_t SerialClose(int32_t portId) override;
    int32_t SerialRead(int32_t portId, std::vector<uint8_t>& data, uint32_t size,
        uint32_t &actualSize, uint32_t timeout) override;
    int32_t SerialWrite(int32_t portId, const std::vector<uint8_t>& data, uint32_t size,
        uint32_t &actualSize, uint32_t timeout) override;
    int32_t SerialGetAttribute(int32_t portId, UsbSerialAttr& attribute) override;
    int32_t SerialSetAttribute(int32_t portId, const UsbSerialAttr& attribute) override;
    int32_t SerialGetPortList(std::vector<UsbSerialPort>& serialPortList) override;
    int32_t HasSerialRight(int32_t portId, bool &hasRight) override;
    int32_t AddSerialRight(uint32_t tokenId, int32_t portId) override;
    int32_t CancelSerialRight(int32_t portId) override;
    int32_t RequestSerialRight(int32_t portId, bool &hasRight) override;

private:
    void SerialAttributeChange(const UsbSerialAttr &serialAttr,
        OHOS::HDI::Usb::Serial::V1_0::SerialAttribute& attribute);
    void SerialAttributeChange(UsbSerialAttr &serialAttr,
        OHOS::HDI::Usb::Serial::V1_0::SerialAttribute& attribute);
    void SerialPortChange(std::vector<UsbSerialPort> &serialInfoList,
        std::vector<OHOS::HDI::Usb::Serial::V1_0::SerialPort>& serialPortLis);
#ifdef USB_MANAGER_PASS_THROUGH
    class SystemAbilityStatusChangeListener : public SystemAbilityStatusChangeStub {
    public:
        explicit SystemAbilityStatusChangeListener(sptr<UsbManagerSubscriber> usbManagerSubscriber);
        ~SystemAbilityStatusChangeListener() = default;
        void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
        void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

    private:
        sptr<UsbManagerSubscriber> usbManagerSubscriber_;
    };
#else
    class SystemAbilityStatusChangeListener : public SystemAbilityStatusChangeStub {
    public:
        explicit SystemAbilityStatusChangeListener(sptr<UsbServiceSubscriber> usbdSubscriber);
        ~SystemAbilityStatusChangeListener() = default;
        void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
        void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

    private:
        sptr<UsbServiceSubscriber> usbdSubscriber_;
    };
#endif // USB_MANAGER_PASS_THROUGH

    class UsbdDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        void OnRemoteDied(const wptr<IRemoteObject> &object) override;
    };

    class SerialDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        SerialDeathRecipient(UsbService *service, int32_t portId, uint32_t tokenId)
            : service_(service), portId_(portId), tokenId_(tokenId){};
        ~SerialDeathRecipient() {};
        void OnRemoteDied(const wptr<IRemoteObject> &object) override;
    private:
        UsbService *service_;
        int32_t portId_;
        uint32_t tokenId_;
    };

private:
    bool Init();
    bool InitUsbd();
    bool IsCommonEventServiceAbilityExist();
    bool GetBundleName(std::string &bundleName);
    bool IsNotNeedUnload();
    void WaitUsbdService();
    int32_t PreCallFunction();
    int32_t InitUsbRight();
    bool IsCallerValid();
    void DumpHelp(int32_t fd);
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    bool InitSerial();
    int32_t GetDeviceVidPidSerialNumber(int32_t portId, std::string& deviceName, std::string& strDesc);
    void UpdateDeviceVidPidMap(std::vector<OHOS::HDI::Usb::Serial::V1_0::SerialPort>& serialPortList);
    int DoDump(int fd, const std::vector<std::string> &argList);
    void FreeTokenId(int32_t portId, uint32_t tokenId);
    int32_t ValidateUsbSerialManagerAndPort(int32_t portId);
    int32_t CheckDbAbility(int32_t portId);
    void GetTransferTypeString(const UsbTransInfo &transInfo, USBEndpoint &ep, std::string &transType);
    void ReportUsbOperationFaultSysEvent(const std::string &operationType, int32_t failReason,
        const std::string &failDescription);
    void ReportUsbSerialOperationSysEvent(int32_t portId, const std::string &operationType);
    void ReportUsbSerialOperationFaultSysEvent(int32_t portId, const std::string &operationType, int32_t failReason,
        const std::string &failDescription);
#ifdef USB_MANAGER_FEATURE_HOST
    void UsbCtrlTransferChange(HDI::Usb::V1_0::UsbCtrlTransfer &param, const UsbCtlSetUp &ctlSetup);
    void UsbCtrlTransferChange(HDI::Usb::V1_2::UsbCtrlTransferParams &param, const UsbCtlSetUp &ctlSetup);
    void UsbDeviceIdChange(const std::vector<UsbDeviceIdInfo> &deviceIdInfoList,
        std::vector<UsbDeviceId> &deviceIdList);
    void UsbDeviceTypeChange(std::vector<UsbDeviceType> &disableType,
        const std::vector<UsbDeviceTypeInfo> &deviceTypes);
    void UsbTransInfoChange(HDI::Usb::V1_2::USBTransferInfo &info, const UsbTransInfo &param);
    std::string GetDeviceVidPidSerialNumber(const std::string &deviceName);
    int32_t GetDeviceVidPidSerialNumber(const std::string &deviceName, std::string& strDesc);
#endif // USB_MANAGER_FEATURE_HOST
#if defined(USB_MANAGER_FEATURE_HOST) || defined(USB_MANAGER_FEATURE_DEVICE)
    bool GetCallingInfo(std::string &bundleName, std::string &tokenId, int32_t &userId);
#endif // USB_MANAGER_FEATURE_HOST || USB_MANAGER_FEATURE_DEVICE
    bool ready_ = false;
    int32_t commEventRetryTimes_ = 0;
    std::mutex mutex_;
    std::mutex serialManagerMutex_;
    std::mutex serialPidVidMapMutex_;
    std::mutex unloadSelfTimerMutex_;
#ifdef USB_MANAGER_FEATURE_HOST
    std::shared_ptr<UsbHostManager> usbHostManager_;
#endif // USB_MANAGER_FEATURE_HOST
#ifdef USB_MANAGER_FEATURE_DEVICE
    std::shared_ptr<UsbDeviceManager> usbDeviceManager_;
    std::shared_ptr<UsbAccessoryManager> usbAccessoryManager_;
#endif // USB_MANAGER_FEATURE_DEVICE
#ifdef USB_MANAGER_FEATURE_PORT
    std::shared_ptr<UsbPortManager> usbPortManager_;
#endif // USB_MANAGER_FEATURE_PORT
    std::shared_ptr<UsbRightManager> usbRightManager_;
    sptr<UsbServiceSubscriber> usbdSubscriber_;
#ifdef USB_MANAGER_PASS_THROUGH
    sptr<UsbManagerSubscriber> usbManagerSubscriber_;
#endif // USB_MANAGER_PASS_THROUGH
    std::shared_ptr<SERIAL::SerialManager> usbSerialManager_;
    sptr<HDI::Usb::V1_2::IUsbInterface> usbd_ = nullptr;
    std::map<std::string, std::string> deviceVidPidMap_;
    std::map<int32_t, std::pair<std::string, std::string>> serialVidPidMap_;
    sptr<OHOS::HDI::Usb::Serial::V1_0::ISerialInterface> seriald_ = nullptr;
    Utils::Timer unloadSelfTimer_ {"unLoadTimer"};
    uint32_t unloadSelfTimerId_ {UINT32_MAX};
    sptr<IRemoteObject::DeathRecipient> recipient_;
};
} // namespace USB
} // namespace OHOS
#endif // USBMGR_USB_SERVICE_H
