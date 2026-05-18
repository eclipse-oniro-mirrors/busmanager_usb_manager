/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef USB_HOST_MANAGER_H
#define USB_HOST_MANAGER_H

#include <map>
#include <string>
#include <vector>

#include "system_ability.h"
#include "usb_device.h"
#include "usb_right_manager.h"
#include "serial_manager.h"
#include "usb_interface_type.h"
#include "v1_2/iusb_interface.h"
#include "iremote_object.h"
#include "system_ability_load_callback_stub.h"
#ifdef USB_MANAGER_PASS_THROUGH
#include "mem_mgr_proxy.h"
#include "mem_mgr_client.h"
#include "v2_0/iusb_host_interface.h"
#include "v2_0/iusb_device_interface.h"
#include "system_ability_definition.h"
#include "usb_manager_subscriber.h"
#include "usb_bulkcallback_impl.h"
#include "usb_transfer_callback_impl.h"
#endif // USB_MANAGER_PASS_THROUGH

namespace OHOS {
namespace USB {
struct DeviceClassUsage {
    uint8_t usage;
    std::string description;

    DeviceClassUsage(int8_t uage, std::string des)
        : usage(uage), description(des) {};
};

typedef std::map<std::string, UsbDevice *> MAP_STR_DEVICE;
class UsbHostManager {
public:
    explicit UsbHostManager(SystemAbility *systemAbility);
    ~UsbHostManager();
#ifdef USB_MANAGER_PASS_THROUGH
    bool InitUsbHostInterface();
    void Stop();
    int32_t BindUsbdSubscriber(const sptr<HDI::Usb::V2_0::IUsbdSubscriber> &subscriber);
    int32_t UnbindUsbdSubscriber(const sptr<HDI::Usb::V2_0::IUsbdSubscriber> &subscriber);
#endif // USB_MANAGER_PASS_THROUGH
    bool GetTargetDevice(uint8_t busNum, uint8_t devAddr, UsbDevice &dev);
    bool GetEndpointFromId(UsbDevice dev, int32_t endpointId, USBEndpoint &endpoint);
    bool GetProductName(const std::string &deviceName, std::string &productName);
    bool DelDevice(uint8_t busNum, uint8_t devNum);
    bool AddDevice(UsbDevice *dev);
    bool Dump(int fd, const std::string &args);
    void ExecuteStrategy();
    void SetSerialManager(std::shared_ptr<SERIAL::SerialManager> serialManager);

    int32_t OpenDevice(uint8_t busNum, uint8_t devAddr);
    int32_t Close(uint8_t busNum, uint8_t devAddr);
    int32_t ResetDevice(uint8_t busNum, uint8_t devAddr);
    int32_t ClaimInterface(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid, uint8_t force);
    int32_t SetInterface(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid, uint8_t altIndex);
    int32_t ReleaseInterface(uint8_t busNum, uint8_t devAddr, uint8_t interface);
    int32_t SetActiveConfig(uint8_t busNum, uint8_t devAddr, uint8_t configIndex);
    int32_t ManageGlobalInterface(bool disable);
    int32_t ManageDevice(int32_t vendorId, int32_t productId, bool disable);
    int32_t ManageDevicePolicy(std::vector<UsbDeviceId> &trustList);
    int32_t ManageInterfaceType(const std::vector<UsbDeviceType> &disableType, bool disable);
    int32_t ManageUsbType(const std::vector<UsbDeviceType> &disableType, bool disable);
    int32_t ManageUsbSerialDevice(bool disable);
    int32_t UsbAttachKernelDriver(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid);
    int32_t UsbDetachKernelDriver(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid);
    int32_t ClearHalt(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId, uint8_t endpointId);
    int32_t UsbDeviceAuthorize(uint8_t busNum, uint8_t devAddr, bool authorized, const std::string &operationType);
    int32_t UsbInterfaceAuthorize(
        const HDI::Usb::V1_0::UsbDev &dev, uint8_t configId, uint8_t interfaceId, bool authorized);

    int32_t GetDevices(std::vector<UsbDevice> &deviceList);
    int32_t GetDeviceInfo(uint8_t busNum, uint8_t devAddr, UsbDevice &dev);
    int32_t GetDeviceInfoDescriptor(
        const HDI::Usb::V1_0::UsbDev &uDev, std::vector<uint8_t> &descriptor, UsbDevice &dev);
    int32_t GetConfigDescriptor(UsbDevice &dev, std::vector<uint8_t> &descriptor);
    int32_t GetActiveConfig(uint8_t busNum, uint8_t devAddr, uint8_t &configIndex);
    int32_t GetRawDescriptor(uint8_t busNum, uint8_t devAddr, std::vector<uint8_t> &bufferData);
    int32_t GetFileDescriptor(uint8_t busNum, uint8_t devAddr, int32_t &fd);
    int32_t GetDeviceSpeed(uint8_t busNum, uint8_t devAddr, uint8_t &speed);
    int32_t GetInterfaceActiveStatus(uint8_t busNum, uint8_t devAddr, uint8_t interfaceid, bool &unactivated);

    int32_t BulkTransferRead(const HDI::Usb::V1_0::UsbDev &dev, const HDI::Usb::V1_0::UsbPipe &pipe,
        std::vector<uint8_t> &bufferData, int32_t timeout);
    int32_t BulkTransferReadwithLength(const HDI::Usb::V1_0::UsbDev &dev, const HDI::Usb::V1_0::UsbPipe &pipe,
        int32_t length, std::vector<uint8_t> &bufferData, int32_t timeOut);
    int32_t BulkTransferWrite(const HDI::Usb::V1_0::UsbDev &dev, const HDI::Usb::V1_0::UsbPipe &pipe,
        const std::vector<uint8_t>& bufferData, int32_t timeout);
    int32_t ControlTransfer(const HDI::Usb::V1_0::UsbDev &dev, const HDI::Usb::V1_0::UsbCtrlTransfer &ctrl,
        std::vector<uint8_t> &bufferData);
    int32_t UsbControlTransfer(const HDI::Usb::V1_0::UsbDev &dev,
        const HDI::Usb::V1_2::UsbCtrlTransferParams &ctrlParams, std::vector<uint8_t> &bufferData);
    int32_t RequestQueue(const HDI::Usb::V1_0::UsbDev &dev, const HDI::Usb::V1_0::UsbPipe &pipe,
        const std::vector<uint8_t> &clientData, const std::vector<uint8_t> &bufferData);
    int32_t RequestWait(const HDI::Usb::V1_0::UsbDev &dev, int32_t timeOut, std::vector<uint8_t> &clientData,
        std::vector<uint8_t> &bufferData);
    int32_t RequestCancel(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId, uint8_t endpointId);
    int32_t UsbCancelTransfer(const HDI::Usb::V1_0::UsbDev &devInfo, const int32_t &endpoint);
    int32_t UsbSubmitTransfer(const HDI::Usb::V1_0::UsbDev &devInfo, HDI::Usb::V1_2::USBTransferInfo &info,
        const sptr<IRemoteObject> &cb, sptr<Ashmem> &ashmem);
    int32_t RegBulkCallback(const HDI::Usb::V1_0::UsbDev &devInfo, const HDI::Usb::V1_0::UsbPipe &pipe,
        const sptr<IRemoteObject> &cb);
    int32_t UnRegBulkCallback(const HDI::Usb::V1_0::UsbDev &devInfo, const HDI::Usb::V1_0::UsbPipe &pipe);
    int32_t BulkRead(
        const HDI::Usb::V1_0::UsbDev &devInfo, const HDI::Usb::V1_0::UsbPipe &pipe, sptr<Ashmem> &ashmem);
    int32_t BulkWrite(
        const HDI::Usb::V1_0::UsbDev &devInfo, const HDI::Usb::V1_0::UsbPipe &pipe, sptr<Ashmem> &ashmem);
    int32_t BulkCancel(const HDI::Usb::V1_0::UsbDev &devInfo, const HDI::Usb::V1_0::UsbPipe &pipe);

private:
    bool PublishCommonEvent(const std::string &event, UsbDevice &dev);
    void ReportHostPlugSysEvent(const std::string &event, const UsbDevice &dev);
    std::string ConcatenateToDescription(const UsbDeviceType &interfaceType, const std::string& str);
    int32_t GetDeviceDescription(int32_t baseClass, std::string &description, uint8_t &usage);
    int32_t GetInterfaceDescription(const UsbDevice &dev, std::string &description, int32_t &baseClass);
    std::string GetInterfaceUsageDescription(const UsbDeviceType &interfaceType);
    int32_t FillDevStrings(UsbDevice &dev);
    std::string GetDevStringValFromIdx(uint8_t busNum, uint8_t devAddr, uint8_t idx);
    bool IsEdmEnabled();
    int32_t ExecuteManageDevicePolicy(std::vector<UsbDeviceId> &trustList);
    int32_t ExecuteManageInterfaceType(const std::vector<UsbDeviceType> &disableType, bool disable);
    void ExecuteManageUsbType(const std::vector<UsbDeviceType> &disableType, bool disable);
    void ManageUsbTypeDeviceImpl(const std::vector<UsbDeviceType> &types, bool disable);
    void ManageUsbTypeInterfaceImpl(const std::vector<UsbDeviceType> &types, bool disable);
    bool IsUsbInterfaceTypeMatched(const std::vector<UsbDeviceType> &types, const UsbInterface intf);
    int32_t GetEdmPolicy(bool &IsGlobalDisabled, std::vector<UsbDeviceType> &disableType,
        std::vector<UsbDeviceId> &trustUsbDeviceIds);
    int32_t GetUsbPolicy(bool &IsGlobalDisabled, std::vector<UsbDeviceType> &disableType,
        std::vector<UsbDeviceId> &trustUsbDeviceIds);
    int32_t GetEdmTypePolicy(sptr<IRemoteObject> remote, std::vector<UsbDeviceType> &disableType);
    void ReadTypePolicyFromParcel(MessageParcel &reply, int size, std::vector<UsbDeviceType> &disableType);
    int32_t GetEdmGlobalPolicy(sptr<IRemoteObject> remote, bool &IsGlobalDisabled);
    int32_t GetEdmStroageTypePolicy(sptr<IRemoteObject> remote, std::vector<UsbDeviceType> &disableType);
    int32_t GetEdmTrustListPolicy(sptr<IRemoteObject> remote, std::vector<UsbDeviceId> &trustUsbDeviceIds);
    int32_t ManageInterface(const HDI::Usb::V1_0::UsbDev &dev, uint8_t interfaceId, bool disable);
    void FindMatchingTypes(const std::unordered_map<InterfaceType, std::vector<int32_t>> &map, bool isDev,
        std::vector<InterfaceType> &matchingTypes, const std::vector<UsbDeviceType> &disableType);
    void ExecuteManageDeviceType(const std::vector<UsbDeviceType> &disableType, bool disable,
        const std::unordered_map<InterfaceType, std::vector<int32_t>> &map, bool isDev);
    int32_t ManageGlobalInterfaceImpl(bool disable);
    int32_t ManageDeviceImpl(int32_t vendorId, int32_t productId, bool disable);
    int32_t ManageInterfaceTypeImpl(InterfaceType interfaceType, bool disable);
    int32_t ManageDeviceTypeImpl(InterfaceType interfaceType, bool disable);
    void AddUsbSerialDevice(UsbDevice &dev);
    bool IsUsbSerialDevice(UsbDevice &dev);
    bool IsUsbSerialDisable();
    void ReportManageDeviceInfo(const std::string &operationType, UsbDevice* device,
        const UsbInterface* interface, bool isInterfaceType);
    int32_t CheckDevPathIsExist(uint8_t busNum, uint8_t devAddr);
    void LoadEdmService();
    MAP_STR_DEVICE devices_;
    SystemAbility *systemAbility_;
    bool isPermTypePolicy_ = false;
    std::mutex mutex_;
    std::shared_mutex devicesMutex_;
    std::shared_ptr<UsbRightManager> usbRightManager_;
    std::shared_ptr<SERIAL::SerialManager> usbSerialManager_ = nullptr;
    std::vector<HDI::Usb::V1_0::UsbDev> serialDevices_;
    sptr<HDI::Usb::V1_0::IUsbdBulkCallback> hdiCb_ = nullptr;
    std::mutex hdiCbMutex_;
    std::mutex transferMutex_;
    class UsbSubmitTransferDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        UsbSubmitTransferDeathRecipient(const HDI::Usb::V1_0::UsbDev &devInfo, const int32_t endpoint,
            UsbHostManager *service, const sptr<IRemoteObject> cb)
            : devInfo_(devInfo), endpoint_(endpoint), service_(service), cb_(cb) {};
        ~UsbSubmitTransferDeathRecipient() {};
        void OnRemoteDied(const wptr<IRemoteObject> &object) override;
    private:
        const HDI::Usb::V1_0::UsbDev devInfo_;
        const int32_t endpoint_;
        UsbHostManager *service_;
        const sptr<IRemoteObject> cb_;
    };
    class UsbEdmLoadCallback : public SystemAbilityLoadCallbackStub {
    public:
        void OnLoadSystemAbilitySuccess(int32_t systemAbilityId, const sptr<IRemoteObject>& remoteObject) override;
        void OnLoadSystemAbilityFail(int32_t systemAbilityId) override;
        UsbEdmLoadCallback(UsbHostManager *usbHostManager) : usbHostManager_(usbHostManager) {};
        ~UsbEdmLoadCallback() {};
    private:
        UsbHostManager *usbHostManager_;
    };

#ifdef USB_MANAGER_PASS_THROUGH
    sptr<HDI::Usb::V2_0::IUsbDeviceInterface> usbDeviceInterface_ = nullptr;
    sptr<HDI::Usb::V2_0::IUsbHostInterface> usbHostInterface_ = nullptr;
    sptr<HDI::Usb::V2_0::IUsbdBulkCallback> usbHostHdiCb_ = nullptr;
    sptr<UsbManagerSubscriber> usbManagerSubscriber_;
#else
    sptr<HDI::Usb::V1_2::IUsbInterface> usbd_ = nullptr;
#endif // USB_MANAGER_PASS_THROUGH
};
} // namespace USB
} // namespace OHOS

#endif // USB_HOST_MANAGER_H
