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

#include "usb_srv_client.h"
#include "datetime_ex.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "usb_common.h"
#include "usb_device.h"
#include "usb_errors.h"
#include "timer.h"
#include "v1_2/iusb_interface.h"
#include "usbd_callback_server.h"
#include "usb_bulk_trans_data.h"
using namespace OHOS::HDI::Usb::V1_2;
namespace OHOS {
namespace USB {
constexpr uint32_t WAIT_SERVICE_LOAD = 500;
constexpr uint32_t MAX_WAIT_LOAD_SA_SECONDS = 4;
#ifdef USB_MANAGER_FEATURE_HOST
constexpr int32_t PARAM_ERROR = 401;
#endif // USB_MANAGER_FEATURE_HOST
[[ maybe_unused ]] constexpr int32_t CAPABILITY_NOT_SUPPORT = 801;
UsbSrvClient::UsbSrvClient()
{
    Connect();
    serialRemote = new SerialDeathMonitor();
}
UsbSrvClient::~UsbSrvClient()
{
    USB_HILOGE(MODULE_USB_INNERKIT, "~UsbSrvClient!");
}

UsbSrvClient& UsbSrvClient::GetInstance()
{
    static UsbSrvClient instance;
    return instance;
}

int32_t UsbSrvClient::Connect(bool force)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (proxy_ != nullptr) {
        return UEC_OK;
    }
    return ConnectUnLocked(force);
}

int32_t UsbSrvClient::ConnectUnLocked(bool force)
{
    sptr<ISystemAbilityManager> sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "fail to get SystemAbilityManager");
        return UEC_INTERFACE_GET_SYSTEM_ABILITY_MANAGER_FAILED;
    }
    sptr<IRemoteObject> remoteObject = nullptr;
    if (force) {
        USB_HILOGE(MODULE_USB_INNERKIT, "force load start");
        remoteObject = sm->LoadSystemAbility(USB_SYSTEM_ABILITY_ID, MAX_WAIT_LOAD_SA_SECONDS);
    } else {
        remoteObject = sm->CheckSystemAbility(USB_SYSTEM_ABILITY_ID);
    }
    if (remoteObject == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "GetSystemAbility failed. force:%{public}d", force);
        return UEC_INTERFACE_GET_USB_SERVICE_FAILED;
    }
    proxy_ = iface_cast<IUsbServer>(remoteObject);
    USB_HILOGI(MODULE_USB_INNERKIT, "Connect UsbService ok.");
    sptr<IRemoteObject> deathObject = proxy_->AsObject();
    if (deathObject == nullptr) {
        USB_HILOGI(MODULE_USB_INNERKIT, "deathObject is null.");
        return UEC_INTERFACE_DEAD_OBJECT;
    }
    deathRecipient_  = new UsbSrvDeathRecipient();
    deathObject->AddDeathRecipient(deathRecipient_);
    return UEC_OK;
}

void UsbSrvClient::ResetProxy(const wptr<IRemoteObject> &remote)
{
    std::lock_guard<std::mutex> lock(mutex_);
    RETURN_IF(proxy_ == nullptr);
    auto serviceRemote = proxy_->AsObject();
    if ((serviceRemote != nullptr) && (serviceRemote == remote.promote())) {
        serviceRemote->RemoveDeathRecipient(deathRecipient_);
        proxy_ = nullptr;

        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_SERVICE_LOAD));
        ConnectUnLocked();
    } else {
        USB_HILOGW(MODULE_USB_INNERKIT, "serviceRemote is null or serviceRemote != promote");
    }
}

void UsbSrvClient::UsbSrvDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    if (remote == nullptr) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvDeathRecipient::OnRemoteDied failed, remote is nullptr.");
        return;
    }
    USB_HILOGI(MODULE_USB_INNERKIT, "UsbSrvDeathRecipient::Recv death notice.");
    UsbSrvClient::GetInstance().ResetProxy(remote);
}

#ifdef USB_MANAGER_FEATURE_HOST
int32_t UsbSrvClient::OpenDevice(const UsbDevice &device, USBDevicePipe &pipe)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling OpenDevice Start!");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->OpenDevice(device.GetBusNum(), device.GetDevAddr());
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "OpenDevice failed with ret = %{public}d !", ret);
        return ret;
    }

    pipe.SetBusNum(device.GetBusNum());
    pipe.SetDevAddr(device.GetDevAddr());
    return UEC_OK;
}

int32_t UsbSrvClient::ResetDevice(USBDevicePipe &pipe)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling ResetDevice Start!");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->ResetDevice(pipe.GetBusNum(), pipe.GetDevAddr());
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "ResetDevice failed with ret = %{public}d !", ret);
        return ret;
    }

    return UEC_OK;
}

bool UsbSrvClient::HasRight(std::string deviceName)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling HasRight Start!");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, false);
    bool hasRight = false;
    proxy_->HasRight(deviceName, hasRight);
    return hasRight;
}

int32_t UsbSrvClient::RequestRight(std::string deviceName)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->RequestRight(deviceName);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "Calling RequestRight failed with ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::RemoveRight(std::string deviceName)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->RemoveRight(deviceName);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "Calling RemoveRight failed with ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::GetDevices(std::vector<UsbDevice> &deviceList)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->GetDevices(deviceList);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "GetDevices failed ret = %{public}d!", ret);
        return ret;
    }
    USB_HILOGI(MODULE_USB_INNERKIT, "GetDevices deviceList size = %{public}zu!", deviceList.size());
    return ret;
}

int32_t UsbSrvClient::ClaimInterface(USBDevicePipe &pipe, const UsbInterface &interface, bool force)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->ClaimInterface(pipe.GetBusNum(), pipe.GetDevAddr(), interface.GetId(), force);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::UsbAttachKernelDriver(USBDevicePipe &pipe, const UsbInterface &interface)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->UsbAttachKernelDriver(pipe.GetBusNum(), pipe.GetDevAddr(), interface.GetId());
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbAttachKernelDriver failed width ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::UsbDetachKernelDriver(USBDevicePipe &pipe, const UsbInterface &interface)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->UsbDetachKernelDriver(pipe.GetBusNum(), pipe.GetDevAddr(), interface.GetId());
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbDetachKernelDriver failed width ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::ReleaseInterface(USBDevicePipe &pipe, const UsbInterface &interface)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->ReleaseInterface(pipe.GetBusNum(), pipe.GetDevAddr(), interface.GetId());
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::BulkTransfer(
    USBDevicePipe &pipe, const USBEndpoint &endpoint, std::vector<uint8_t> &bufferData, int32_t timeOut)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t ret = UEC_INTERFACE_INVALID_VALUE;
    if (USB_ENDPOINT_DIR_IN == endpoint.GetDirection()) {
        int32_t length = static_cast<int32_t>(bufferData.size());
        bufferData.clear();
        UsbBulkTransData bulkData;
        ret = proxy_->BulkTransferReadwithLength(pipe.GetBusNum(), pipe.GetDevAddr(),
            endpoint, length, bulkData, timeOut);
        bufferData.swap(bulkData.data_);
    } else if (USB_ENDPOINT_DIR_OUT == endpoint.GetDirection()) {
        UsbBulkTransData bulkData (bufferData);
        ret = proxy_->BulkTransferWrite(pipe.GetBusNum(), pipe.GetDevAddr(), endpoint, bulkData, timeOut);
    }
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

void UsbSrvClient::UsbCtrlTransferChange(const HDI::Usb::V1_0::UsbCtrlTransfer &param, UsbCtlSetUp &ctlSetup)
{
    ctlSetup.reqType = param.requestType;
    ctlSetup.reqCmd = param.requestCmd;
    ctlSetup.value = param.value;
    ctlSetup.index = param.index;
    ctlSetup.timeout = param.timeout;
    return;
}

int32_t UsbSrvClient::ControlTransfer(
    USBDevicePipe &pipe, const UsbCtrlTransfer &ctrl, std::vector<uint8_t> &bufferData)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    UsbCtlSetUp ctlSetup;
    UsbCtrlTransferChange(ctrl, ctlSetup);
    if ((static_cast<uint32_t>(ctlSetup.reqType) & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_IN) {
        bufferData.clear();
    }
    int32_t ret = proxy_->ControlTransfer(pipe.GetBusNum(), pipe.GetDevAddr(), ctlSetup, bufferData);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }

    return ret;
}

void UsbSrvClient::UsbCtrlTransferChange(const HDI::Usb::V1_2::UsbCtrlTransferParams &param, UsbCtlSetUp &ctlSetup)
{
    ctlSetup.reqType = param.requestType;
    ctlSetup.reqCmd = param.requestCmd;
    ctlSetup.value = param.value;
    ctlSetup.index = param.index;
    ctlSetup.length = param.length;
    ctlSetup.timeout = param.timeout;
    return;
}

int32_t UsbSrvClient::UsbControlTransfer(USBDevicePipe &pipe, const HDI::Usb::V1_2::UsbCtrlTransferParams &ctrlParams,
    std::vector<uint8_t> &bufferData)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    UsbCtlSetUp ctlSetup;
    UsbCtrlTransferChange(ctrlParams, ctlSetup);
    if ((static_cast<uint32_t>(ctlSetup.reqType) & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_IN) {
        bufferData.clear();
    }
    int32_t ret = proxy_->UsbControlTransfer(pipe.GetBusNum(), pipe.GetDevAddr(), ctlSetup, bufferData);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }

    return ret;
}

int32_t UsbSrvClient::SetConfiguration(USBDevicePipe &pipe, const USBConfig &config)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->SetActiveConfig(pipe.GetBusNum(), pipe.GetDevAddr(), config.GetId());
    return ret;
}

int32_t UsbSrvClient::SetInterface(USBDevicePipe &pipe, const UsbInterface &interface)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    return proxy_->SetInterface(
        pipe.GetBusNum(), pipe.GetDevAddr(), interface.GetId(), interface.GetAlternateSetting());
}

int32_t UsbSrvClient::GetRawDescriptors(USBDevicePipe &pipe, std::vector<uint8_t> &bufferData)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->GetRawDescriptor(pipe.GetBusNum(), pipe.GetDevAddr(), bufferData);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::GetFileDescriptor(USBDevicePipe &pipe, int32_t &fd)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->GetFileDescriptor(pipe.GetBusNum(), pipe.GetDevAddr(), fd);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed ret = %{public}d!", ret);
    }
    return ret;
}

bool UsbSrvClient::Close(const USBDevicePipe &pipe)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, false);
    int32_t ret = proxy_->Close(pipe.GetBusNum(), pipe.GetDevAddr());
    return (ret == UEC_OK);
}

int32_t UsbSrvClient::PipeRequestWait(USBDevicePipe &pipe, int64_t timeOut, UsbRequest &req)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    std::vector<uint8_t> clientData;
    std::vector<uint8_t> bufferData;
    int32_t ret = proxy_->RequestWait(pipe.GetBusNum(), pipe.GetDevAddr(), timeOut, clientData, bufferData);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d.", ret);
        return ret;
    }

    req.SetPipe(pipe);
    req.SetClientData(clientData);
    req.SetReqData(bufferData);
    return ret;
}

int32_t UsbSrvClient::RequestInitialize(UsbRequest &request)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    const USBDevicePipe &pipe = request.GetPipe();
    const USBEndpoint &endpoint = request.GetEndpoint();
    return proxy_->ClaimInterface(pipe.GetBusNum(), pipe.GetDevAddr(), endpoint.GetInterfaceId(), CLAIM_FORCE_1);
}

int32_t UsbSrvClient::RequestFree(UsbRequest &request)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    const USBDevicePipe &pipe = request.GetPipe();
    const USBEndpoint &ep = request.GetEndpoint();
    return proxy_->RequestCancel(pipe.GetBusNum(), pipe.GetDevAddr(), ep.GetInterfaceId(), ep.GetAddress());
}

int32_t UsbSrvClient::RequestAbort(UsbRequest &request)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    const USBDevicePipe &pipe = request.GetPipe();
    const USBEndpoint &ep = request.GetEndpoint();
    return proxy_->RequestCancel(pipe.GetBusNum(), pipe.GetDevAddr(), ep.GetInterfaceId(), ep.GetAddress());
}

int32_t UsbSrvClient::RequestQueue(UsbRequest &request)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    const USBDevicePipe &pipe = request.GetPipe();
    const USBEndpoint &ep = request.GetEndpoint();
    return proxy_->RequestQueue(pipe.GetBusNum(), pipe.GetDevAddr(), ep, request.GetClientData(), request.GetReqData());
}

int32_t UsbSrvClient::UsbCancelTransfer(USBDevicePipe &pipe, int32_t &endpoint)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->UsbCancelTransfer(pipe.GetBusNum(), pipe.GetDevAddr(), endpoint);
    if (ret!= UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbCancelTransfer failed with ret = %{public}d!", ret);
    }
    return ret;
}

void UsbSrvClient::UsbTransInfoChange(const HDI::Usb::V1_2::USBTransferInfo &param, UsbTransInfo &info)
{
    info.endpoint = param.endpoint;
    info.flags = param.flags;
    info.type = param.type;
    info.timeOut = param.timeOut;
    info.length = param.length;
    info.userData = param.userData;
    info.numIsoPackets = param.numIsoPackets;
    return;
}

int32_t UsbSrvClient::UsbSubmitTransfer(USBDevicePipe &pipe, HDI::Usb::V1_2::USBTransferInfo &info,
    const TransferCallback &cb, sptr<Ashmem> &ashmem)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    if (cb == nullptr) {
        return PARAM_ERROR;
    }
    sptr<UsbdCallBackServer> callBackService = new UsbdCallBackServer(cb);
    UsbTransInfo param;
    UsbTransInfoChange(info, param);
    int32_t fd = ashmem->GetAshmemFd();
    int32_t memSize = ashmem->GetAshmemSize();
    int32_t ret = proxy_->UsbSubmitTransfer(pipe.GetBusNum(), pipe.GetDevAddr(), param, callBackService, fd, memSize);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSubmitTransfer failed with ret = %{public}d", ret);
    }
    return ret;
}

int32_t UsbSrvClient::RegBulkCallback(USBDevicePipe &pipe, const USBEndpoint &endpoint, const sptr<IRemoteObject> &cb)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->RegBulkCallback(pipe.GetBusNum(), pipe.GetDevAddr(), endpoint, cb);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::UnRegBulkCallback(USBDevicePipe &pipe, const USBEndpoint &endpoint)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->UnRegBulkCallback(pipe.GetBusNum(), pipe.GetDevAddr(), endpoint);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::BulkRead(USBDevicePipe &pipe, const USBEndpoint &endpoint, sptr<Ashmem> &ashmem)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t fd = ashmem->GetAshmemFd();
    int32_t memSize = ashmem->GetAshmemSize();
    int32_t ret = proxy_->BulkRead(pipe.GetBusNum(), pipe.GetDevAddr(), endpoint, fd, memSize);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::BulkWrite(USBDevicePipe &pipe, const USBEndpoint &endpoint, sptr<Ashmem> &ashmem)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t fd = ashmem->GetAshmemFd();
    int32_t memSize = ashmem->GetAshmemSize();
    int32_t ret = proxy_->BulkWrite(pipe.GetBusNum(), pipe.GetDevAddr(), endpoint, fd, memSize);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::BulkCancel(USBDevicePipe &pipe, const USBEndpoint &endpoint)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->BulkCancel(pipe.GetBusNum(), pipe.GetDevAddr(), endpoint);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::AddAccessRight(const std::string &tokenId, const std::string &deviceName)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling AddAccessRight");
    int32_t ret = proxy_->AddAccessRight(tokenId, deviceName);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::ManageGlobalInterface(bool disable)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->ManageGlobalInterface(disable);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::ManageDevice(int32_t vendorId, int32_t productId, bool disable)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->ManageDevice(vendorId, productId, disable);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

void UsbSrvClient::UsbDeviceIdChange(const std::vector<UsbDeviceId> &deviceIdList,
    std::vector<UsbDeviceIdInfo> &deviceIdInfoList)
{
    for (auto &deviceId : deviceIdList) {
        UsbDeviceIdInfo info;
        info.productId = deviceId.productId;
        info.vendorId = deviceId.vendorId;
        deviceIdInfoList.emplace_back(info);
    }
    return;
}

int32_t UsbSrvClient::ManageDevicePolicy(std::vector<UsbDeviceId> &trustList)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    std::vector<UsbDeviceIdInfo> deviceIdInfoList{};
    UsbDeviceIdChange(trustList, deviceIdInfoList);
    int32_t ret = proxy_->ManageDevicePolicy(deviceIdInfoList);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

void UsbSrvClient::UsbDeviceTypeChange(const std::vector<UsbDeviceType> &disableType,
    std::vector<UsbDeviceTypeInfo> &deviceTypes)
{
    for (size_t i = 0; i < disableType.size(); i++) {
        UsbDeviceTypeInfo info;
        info.baseClass = disableType[i].baseClass;
        info.subClass = disableType[i].subClass;
        info.protocol = disableType[i].protocol;
        info.isDeviceType = disableType[i].isDeviceType;
        deviceTypes.push_back(info);
    }
    return;
}

int32_t UsbSrvClient::ManageInterfaceType(const std::vector<UsbDeviceType> &disableType, bool disable)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    std::vector<UsbDeviceTypeInfo> disableDevType;
    UsbDeviceTypeChange(disableType, disableDevType);
    int32_t ret = proxy_->ManageInterfaceType(disableDevType, disable);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed width ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::ClearHalt(USBDevicePipe &pipe, const USBEndpoint &ep)
{
    RETURN_IF_WITH_RET(proxy_ == nullptr, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->ClearHalt(pipe.GetBusNum(), pipe.GetDevAddr(), ep.GetInterfaceId(), ep.GetAddress());
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "ClearHalt failed ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::GetDeviceSpeed(USBDevicePipe &pipe, uint8_t &speed)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->GetDeviceSpeed(pipe.GetBusNum(), pipe.GetDevAddr(), speed);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed ret = %{public}d!", ret);
    }
    USB_HILOGE(MODULE_USB_INNERKIT, "GetDeviceSpeed speed = %{public}u!", speed);
    return ret;
}

int32_t UsbSrvClient::GetInterfaceActiveStatus(USBDevicePipe &pipe, const UsbInterface &interface, bool &unactivated)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->GetInterfaceActiveStatus(pipe.GetBusNum(), pipe.GetDevAddr(), interface.GetId(), unactivated);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed ret = %{public}d!", ret);
    }
    return ret;
}
#else
int32_t UsbSrvClient::OpenDevice(const UsbDevice &device, USBDevicePipe &pipe)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::ResetDevice(USBDevicePipe &pipe)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

bool UsbSrvClient::HasRight(std::string deviceName)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return false;
}

int32_t UsbSrvClient::RequestRight(std::string deviceName)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::RemoveRight(std::string deviceName)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::GetDevices(std::vector<UsbDevice> &deviceList)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::ClaimInterface(USBDevicePipe &pipe, const UsbInterface &interface, bool force)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::UsbAttachKernelDriver(USBDevicePipe &pipe, const UsbInterface &interface)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::UsbDetachKernelDriver(USBDevicePipe &pipe, const UsbInterface &interface)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::ReleaseInterface(USBDevicePipe &pipe, const UsbInterface &interface)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::BulkTransfer(
    USBDevicePipe &pipe, const USBEndpoint &endpoint, std::vector<uint8_t> &bufferData, int32_t timeOut)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::ControlTransfer(
    USBDevicePipe &pipe, const UsbCtrlTransfer &ctrl, std::vector<uint8_t> &bufferData)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::UsbControlTransfer(USBDevicePipe &pipe, const HDI::Usb::V1_2::UsbCtrlTransferParams &ctrlParams,
    std::vector<uint8_t> &bufferData)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::SetConfiguration(USBDevicePipe &pipe, const USBConfig &config)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::SetInterface(USBDevicePipe &pipe, const UsbInterface &interface)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::GetRawDescriptors(USBDevicePipe &pipe, std::vector<uint8_t> &bufferData)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::GetFileDescriptor(USBDevicePipe &pipe, int32_t &fd)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

bool UsbSrvClient::Close(const USBDevicePipe &pipe)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return false;
}

int32_t UsbSrvClient::PipeRequestWait(USBDevicePipe &pipe, int64_t timeOut, UsbRequest &req)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::RequestInitialize(UsbRequest &request)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::RequestFree(UsbRequest &request)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::RequestAbort(UsbRequest &request)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::RequestQueue(UsbRequest &request)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::UsbCancelTransfer(USBDevicePipe &pipe, int32_t &endpoint)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::UsbSubmitTransfer(USBDevicePipe &pipe, HDI::Usb::V1_2::USBTransferInfo &info,
    const TransferCallback &cb, sptr<Ashmem> &ashmem)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::RegBulkCallback(USBDevicePipe &pipe, const USBEndpoint &endpoint, const sptr<IRemoteObject> &cb)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::UnRegBulkCallback(USBDevicePipe &pipe, const USBEndpoint &endpoint)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::BulkRead(USBDevicePipe &pipe, const USBEndpoint &endpoint, sptr<Ashmem> &ashmem)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::BulkWrite(USBDevicePipe &pipe, const USBEndpoint &endpoint, sptr<Ashmem> &ashmem)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::BulkCancel(USBDevicePipe &pipe, const USBEndpoint &endpoint)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}


int32_t UsbSrvClient::AddAccessRight(const std::string &tokenId, const std::string &deviceName)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::ManageGlobalInterface(bool disable)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::ManageDevice(int32_t vendorId, int32_t productId, bool disable)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::ManageInterfaceType(const std::vector<UsbDeviceType> &disableType, bool disable)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::ClearHalt(USBDevicePipe &pipe, const USBEndpoint &ep)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::GetDeviceSpeed(USBDevicePipe &pipe, uint8_t &speed)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::GetInterfaceActiveStatus(USBDevicePipe &pipe, const UsbInterface &interface, bool &unactivated)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}
#endif // USB_MANAGER_FEATURE_HOST

#ifdef USB_MANAGER_FEATURE_DEVICE
int32_t UsbSrvClient::GetCurrentFunctions(int32_t &funcs)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling GetCurrentFunctions!");
    RETURN_IF_WITH_RET(Connect(true) != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->GetCurrentFunctions(funcs);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed ret = %{public}d!", ret);
        return ret;
    }
    USB_HILOGI(MODULE_USB_INNERKIT, "GetCurrentFunctions funcs = %{public}d!", funcs);
    return ret;
}

int32_t UsbSrvClient::SetCurrentFunctions(int32_t funcs)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "SetCurrentFunctions funcs = %{public}d!", funcs);
    RETURN_IF_WITH_RET(Connect(true) != UEC_OK, false);
    int32_t ret = proxy_->SetCurrentFunctions(funcs);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed ret = %{public}d!", ret);
        return ret;
    }
    USB_HILOGI(MODULE_USB_INNERKIT, " Calling SetCurrentFunctions Success!");
    return ret;
}

int32_t UsbSrvClient::UsbFunctionsFromString(std::string_view funcs)
{
    RETURN_IF_WITH_RET(Connect(true) != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t funcResult = 0;
    std::string funcsStr(funcs);
    int32_t ret = proxy_->UsbFunctionsFromString(funcsStr, funcResult);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbFunctionsFromString failed ret = %{public}d!", ret);
        return ret;
    }
    USB_HILOGI(MODULE_USB_INNERKIT, " Calling UsbFunctionsFromString Success!");
    return funcResult;
}

std::string UsbSrvClient::UsbFunctionsToString(int32_t funcs)
{
    std::string result;
    RETURN_IF_WITH_RET(Connect(true) != UEC_OK, result);
    int32_t ret = proxy_->UsbFunctionsToString(funcs, result);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbFunctionsToString failed ret = %{public}d!", ret);
        return "";
    }
    USB_HILOGI(MODULE_USB_INNERKIT, " Calling UsbFunctionsToString Success!");
    return result;
}

int32_t UsbSrvClient::GetAccessoryList(std::vector<USBAccessory> &accessList)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->GetAccessoryList(accessList);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "GetAccessoryList failed ret = %{public}d!", ret);
        return ret;
    }
    USB_HILOGI(MODULE_USB_INNERKIT, "GetAccessoryList accessList size = %{public}zu!", accessList.size());
    return ret;
}

int32_t UsbSrvClient::OpenAccessory(const USBAccessory &access, int32_t &fd)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->OpenAccessory(access, fd);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "OpenAccessory ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::AddAccessoryRight(const uint32_t tokenId, const USBAccessory &access)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->AddAccessoryRight(tokenId, access);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "AddAccessoryRight ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::HasAccessoryRight(const USBAccessory &access, bool &result)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    return proxy_->HasAccessoryRight(access, result);
}

int32_t UsbSrvClient::RequestAccessoryRight(const USBAccessory &access, bool &result)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->RequestAccessoryRight(access, result);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "RequestAccessoryRight ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::CancelAccessoryRight(const USBAccessory &access)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->CancelAccessoryRight(access);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "CancelAccessoryRight ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::CloseAccessory(const int32_t fd)
{
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->CloseAccessory(fd);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "CloseAccessory ret = %{public}d!", ret);
    }
    return ret;
}
#else
int32_t UsbSrvClient::GetCurrentFunctions(int32_t &funcs)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::SetCurrentFunctions(int32_t funcs)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::UsbFunctionsFromString(std::string_view funcs)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

std::string UsbSrvClient::UsbFunctionsToString(int32_t funcs)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return "";
}

int32_t UsbSrvClient::GetAccessoryList(std::vector<USBAccessory> &accessList)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::OpenAccessory(const USBAccessory &access, int32_t &fd)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::AddAccessoryRight(const uint32_t tokenId, const USBAccessory &access)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::HasAccessoryRight(const USBAccessory &access, bool &result)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::RequestAccessoryRight(const USBAccessory &access, bool &result)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::CancelAccessoryRight(const USBAccessory &access)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::CloseAccessory(const int32_t fd)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}
#endif // USB_MANAGER_FEATURE_DEVICE

#ifdef USB_MANAGER_FEATURE_PORT
int32_t UsbSrvClient::GetPorts(std::vector<UsbPort> &usbports)
{
    RETURN_IF_WITH_RET(Connect(true) != UEC_OK, UEC_INTERFACE_NO_INIT);
    USB_HILOGI(MODULE_USB_INNERKIT, " Calling GetPorts");
    int32_t ret = proxy_->GetPorts(usbports);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::GetSupportedModes(int32_t portId, int32_t &result)
{
    RETURN_IF_WITH_RET(Connect(true) != UEC_OK, UEC_INTERFACE_NO_INIT);
    USB_HILOGI(MODULE_USB_INNERKIT, " Calling GetSupportedModes");
    int32_t ret = proxy_->GetSupportedModes(portId, result);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::SetPortRole(int32_t portId, int32_t powerRole, int32_t dataRole)
{
    RETURN_IF_WITH_RET(Connect(true) != UEC_OK, UEC_INTERFACE_NO_INIT);
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling SetPortRole");
    int32_t ret = proxy_->SetPortRole(portId, powerRole, dataRole);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "failed ret = %{public}d!", ret);
    }
    return ret;
}
#else
int32_t UsbSrvClient::GetPorts(std::vector<UsbPort> &usbports)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::GetSupportedModes(int32_t portId, int32_t &result)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}

int32_t UsbSrvClient::SetPortRole(int32_t portId, int32_t powerRole, int32_t dataRole)
{
    USB_HILOGW(MODULE_USB_INNERKIT, "%{public}s: Capability not supported.", __FUNCTION__);
    return CAPABILITY_NOT_SUPPORT;
}
#endif // USB_MANAGER_FEATURE_PORT

int32_t UsbSrvClient::SerialOpen(int32_t portId)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling SerialOpen");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->SerialOpen(portId, serialRemote);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvClient::SerialOpen failed ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::SerialClose(int32_t portId)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling SerialClose");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->SerialClose(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvClient::SerialClose failed ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::SerialRead(int32_t portId, std::vector<uint8_t> &data,
    uint32_t bufferSize, uint32_t &actualSize, uint32_t timeout)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling SerialRead");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->SerialRead(portId, data, bufferSize, actualSize, timeout);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvClient::SerialRead failed ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::SerialWrite(int32_t portId, const std::vector<uint8_t>& data,
    uint32_t bufferSize, uint32_t &actualSize, uint32_t timeout)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling SerialWrite");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->SerialWrite(portId, data, bufferSize, actualSize, timeout);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvClient::SerialWrite failed ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::SerialGetAttribute(int32_t portId, UsbSerialAttr& attribute)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling SerialGetAttribute");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->SerialGetAttribute(portId, attribute);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvClient::SerialGetAttribute failed ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::SerialSetAttribute(int32_t portId,
    const UsbSerialAttr& attribute)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling SerialSetAttribute");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->SerialSetAttribute(portId, attribute);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvClient::SerialSetAttribute failed ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::SerialGetPortList(
    std::vector<UsbSerialPort>& serialPortList)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling SerialGetPortList");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->SerialGetPortList(serialPortList);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvClient::SerialGetPortList failed ret = %{public}d!", ret);
    }
    return ret;
}

int32_t UsbSrvClient::HasSerialRight(int32_t portId, bool &hasRight)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling HasSerialRight");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->HasSerialRight(portId, hasRight);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvClient::HasSerialRight failed ret = %{public}d!", ret);
        return ret;
    }
    return UEC_OK;
}

int32_t UsbSrvClient::CancelSerialRight(int32_t portId)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling CancelSerialRight");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->CancelSerialRight(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvClient::CancelSerialRight failed ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::RequestSerialRight(int32_t portId, bool &hasRight)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling RequestSerialRight");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->RequestSerialRight(portId, hasRight);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvClient::RequestSerialRight failed ret = %{public}d !", ret);
    }
    return ret;
}

int32_t UsbSrvClient::AddSerialRight(uint32_t tokenId, int32_t portId)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "Calling AddSerialRight");
    RETURN_IF_WITH_RET(Connect() != UEC_OK, UEC_INTERFACE_NO_INIT);
    int32_t ret = proxy_->AddSerialRight(tokenId, portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_INNERKIT, "UsbSrvClient::AddSerialRight failed ret = %{public}d!", ret);
    }
    return ret;
}
} // namespace USB
} // namespace OHOS
