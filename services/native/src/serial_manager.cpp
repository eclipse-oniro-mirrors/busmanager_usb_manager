/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http:www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <regex>
#include <unistd.h>
#include "hisysevent.h"
#include "usb_errors.h"
#include "usb_security_report.h"
#include "securec.h"
#include "serial_manager.h"
#include "hilog_wrapper.h"
#include "tokenid_kit.h"
#include "accesstoken_kit.h"

#define DUMP_PARAMS_NUM_2 2

using namespace OHOS::HiviewDFX;
using OHOS::USB::USB_MGR_LABEL;
using OHOS::USB::MODULE_USB_SERIAL;
using OHOS::USB::UEC_OK;

namespace OHOS {
namespace SERIAL {
constexpr int32_t PARAM_COUNT_ONE = 1;
constexpr int32_t ERR_CODE_IOEXCEPTION = -1;
constexpr int32_t ERR_CODE_DEVICENOTOPEN = -6;
constexpr int32_t ERR_CODE_TIMEOUT = -7;
constexpr int32_t ERR_CODE_ERROR_OVERFLOW = -8;
constexpr int32_t ERR_CODE_ERROR_DEVICE_BUSY = -16;

SerialManager::SerialManager()
{
    if (serial_ == nullptr) {
        serial_ = OHOS::HDI::Usb::Serial::V1_0::ISerialInterface::Get("serial_interface_service", true);
        if (serial_ == nullptr) {
            USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: serial_ is nullptr", __func__);
        }
    }

    usbRightManager_ = std::make_shared<USB::UsbRightManager>();
}

SerialManager::~SerialManager()
{
    USB_HILOGI(MODULE_USB_SERIAL, "%{public}s: start", __func__);
}

inline int32_t ErrorCodeWrap(int32_t errorCode)
{
    if (errorCode == ERR_CODE_IOEXCEPTION || errorCode == ERR_CODE_ERROR_OVERFLOW) {
        return USB::UEC_SERIAL_IO_EXCEPTION;
    } else if (errorCode == ERR_CODE_DEVICENOTOPEN) {
        return USB::UEC_SERIAL_DEVICENOTOPEN;
    } else if (errorCode == ERR_CODE_TIMEOUT) {
        return USB::UEC_INTERFACE_TIMED_OUT;
    } else if (errorCode == ERR_CODE_ERROR_DEVICE_BUSY) {
        return USB::UEC_SERIAL_PORT_REPEAT_OPEN;
    } else {
        return USB::UEC_SERIAL_OTHER_ERROR;
    }
}

int32_t SerialManager::CheckPortAndTokenId(int32_t portId)
{
    if (serial_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: serial_ is nullptr", __func__);
        return OHOS::USB::UEC_SERVICE_INVALID_VALUE;
    }

    if (!CheckTokenIdValidity(portId)) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: Application ID mismatch", __func__);
        return OHOS::USB::UEC_SERIAL_PORT_NOT_OPEN;
    }

    return UEC_OK;
}

int32_t SerialManager::SerialOpen(int32_t portId)
{
    USB_HILOGI(MODULE_USB_SERIAL, "%{public}s: start", __func__);
    if (serial_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: serial_ is nullptr", __func__);
        return OHOS::USB::UEC_SERVICE_INVALID_VALUE;
    }

    std::lock_guard<std::mutex> guard(portTokenMapMutex_);
    if (portTokenMap_.find(portId) != portTokenMap_.end()) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: The port has been opened", __func__);
        return OHOS::USB::UEC_SERIAL_PORT_REPEAT_OPEN;
    }

    uint64_t timeMs = USB::UsbSecurityReport::GetCurrentTime();
    int32_t ret = serial_->SerialOpen(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: SerialOpen failed ret = %{public}d", __func__, ret);
        return ErrorCodeWrap(ret);
    }
    ReportSerialOperationSecurityInfo(portId, SERIAL_OPEN, timeMs);

    portTokenMap_[portId] = IPCSkeleton::GetCallingTokenID();
    return ret;
}

int32_t SerialManager::SerialClose(int32_t portId)
{
    USB_HILOGI(MODULE_USB_SERIAL, "%{public}s: start", __func__);

    int32_t ret = CheckPortAndTokenId(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: CheckPortAndTokenId failed", __func__);
        return ret;
    }

    uint64_t timeMs = USB::UsbSecurityReport::GetCurrentTime();
    ret = serial_->SerialClose(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: SerialClose failed ret = %{public}d", __func__, ret);
        return ErrorCodeWrap(ret);
    }
    ReportSerialOperationSecurityInfo(portId, SERIAL_CLOSE, timeMs);

    ResetFirstRead(portId);
    ResetFirstWrite(portId);
    std::lock_guard<std::mutex> guard(portTokenMapMutex_);
    portTokenMap_.erase(portId);
    return ret;
}

int32_t SerialManager::SerialRead(int32_t portId, std::vector<uint8_t> &data, uint32_t size,
    uint32_t &actualSize, uint32_t timeout)
{
    USB_HILOGI(MODULE_USB_SERIAL, "%{public}s: start", __func__);
    int32_t ret = CheckPortAndTokenId(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: CheckPortAndTokenId failed", __func__);
        return ret;
    }

    uint64_t timeMs = USB::UsbSecurityReport::GetCurrentTime();
    ret = serial_->SerialRead(portId, data, size, timeout);
    if (ret < UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: SerialRead failed ret = %{public}d", __func__, ret);
        return ErrorCodeWrap(ret);
    }
    if (QueryAndRecordFirstRead(portId)) {
        ReportSerialOperationSecurityInfo(portId, SERIAL_READ, timeMs);
    }
    actualSize = static_cast<uint32_t>(ret);
    return UEC_OK;
}

int32_t SerialManager::SerialWrite(int32_t portId, const std::vector<uint8_t>& data, uint32_t size,
    uint32_t &actualSize, uint32_t timeout)
{
    USB_HILOGI(MODULE_USB_SERIAL, "%{public}s: start", __func__);
    int32_t ret = CheckPortAndTokenId(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: CheckPortAndTokenId failed", __func__);
        return ret;
    }

    uint64_t timeMs = USB::UsbSecurityReport::GetCurrentTime();
    ret = serial_->SerialWrite(portId, data, size, timeout);
    if (ret < UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: SerialWrite failed ret = %{public}d", __func__, ret);
        return ErrorCodeWrap(ret);
    }
    if (QueryAndRecordFirstWrite(portId)) {
        ReportSerialOperationSecurityInfo(portId, SERIAL_WRITE, timeMs);
    }
    actualSize = static_cast<uint32_t>(ret);
    return UEC_OK;
}

int32_t SerialManager::SerialGetAttribute(int32_t portId, OHOS::HDI::Usb::Serial::V1_0::SerialAttribute& attribute)
{
    USB_HILOGI(MODULE_USB_SERIAL, "%{public}s: start", __func__);
    int32_t ret = CheckPortAndTokenId(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: CheckPortAndTokenId failed", __func__);
        return ret;
    }

    ret = serial_->SerialGetAttribute(portId, attribute);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: SerialGetAttribute failed ret = %{public}d", __func__, ret);
        return ErrorCodeWrap(ret);
    }

    return ret;
}

int32_t SerialManager::SerialSetAttribute(int32_t portId,
    const OHOS::HDI::Usb::Serial::V1_0::SerialAttribute& attribute)
{
    USB_HILOGI(MODULE_USB_SERIAL, "%{public}s: start", __func__);
    int32_t ret = CheckPortAndTokenId(portId);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: CheckPortAndTokenId failed", __func__);
        return ret;
    }

    ret = serial_->SerialSetAttribute(portId, attribute);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: SerialSetAttribute failed ret = %{public}d", __func__, ret);
        return ErrorCodeWrap(ret);
    }

    return ret;
}

int32_t SerialManager::SerialGetPortList(std::vector<OHOS::HDI::Usb::Serial::V1_0::SerialPort>& serialPortList)
{
    USB_HILOGI(MODULE_USB_SERIAL, "%{public}s: start", __func__);

    if (serial_ == nullptr) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: serial_ is nullptr", __func__);
        return OHOS::USB::UEC_SERVICE_INVALID_VALUE;
    }

    int32_t ret = serial_->SerialGetPortList(serialPortList);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: SerialGetPortList failed ret = %{public}d", __func__, ret);
        return ret;
    }
    
    UpdateSerialPortMap(serialPortList);
    return ret;
}

void SerialManager::UpdateSerialPortMap(std::vector<OHOS::HDI::Usb::Serial::V1_0::SerialPort>& serialPortList)
{
    std::lock_guard<std::mutex> guard(serialPortMapMutex_);
    for (auto& it : serialPortList) {
        serialPortMap_[it.portId] = it;
    }
}

bool SerialManager::IsPortIdExist(int32_t portId)
{
    std::lock_guard<std::mutex> guard(serialPortMapMutex_);
    if (serialPortMap_.find(portId) == serialPortMap_.end()) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: port %{public}d not exist", __func__, portId);
        return false;
    }

    return true;
}

bool SerialManager::CheckTokenIdValidity(int32_t portId)
{
    std::lock_guard<std::mutex> guard(portTokenMapMutex_);
    auto it = portTokenMap_.find(portId);
    if (it == portTokenMap_.end()) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: The port is not open", __func__);
        return false;
    }
    if (IPCSkeleton::GetCallingTokenID() != it->second) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: The tokenId corresponding to the port is incorrect", __func__);
        return false;
    }

    return true;
}

void SerialManager::FreeTokenId(int32_t portId, uint32_t tokenId)
{
    USB_HILOGI(MODULE_USB_SERIAL, "%{public}s: start", __func__);
    std::lock_guard<std::mutex> guard(portTokenMapMutex_);
    if ((portTokenMap_.find(portId) == portTokenMap_.end()) || (portTokenMap_[portId] != tokenId)) {
        USB_HILOGE(MODULE_USB_SERIAL, "%{public}s: portid not exist or tokenId failed", __func__);
        return;
    }

    if (serial_ != nullptr) {
        serial_->SerialClose(portId);
    }
    
    portTokenMap_.erase(portId);
}

void SerialManager::SerialPortListDump(int32_t fd, const std::vector<std::string>& args)
{
    if (args.size() != PARAM_COUNT_ONE) {
        dprintf(fd, "port param error: exactly %d parameters are required\n", PARAM_COUNT_ONE);
        ListGetDumpHelp(fd);
        return;
    }
    
    if (args[0] == "-l") {
        std::vector<OHOS::HDI::Usb::Serial::V1_0::SerialPort> serialPortList;
        SerialGetPortList(serialPortList);
        dprintf(fd, "=========== dump the serial port list ===========\n");
        for (size_t i = 0; i < serialPortList.size(); ++i) {
            char portName[20];
            snprintf_s(portName, sizeof(portName), sizeof(portName)-1, "ttyUSB%zu", i);
            dprintf(fd, "%s\n", portName);
        }
        dprintf(fd, "------------------------------------------------\n");
    } else {
        dprintf(fd, "port param error: invalid command '%s'\n", args[0].c_str());
        ListGetDumpHelp(fd);
    }
}

bool SerialManager::CheckDataAndProcessPortId(int32_t fd, const std::vector<std::string>& args,
    OHOS::HDI::Usb::Serial::V1_0::SerialAttribute attribute)
{
    if (!args.empty() && !std::regex_match(args[1], std::regex("^[0-9]+$"))) {
        dprintf(fd, "Invalid input, please enter a valid integer\n");
        return false;
    }
    int32_t portId = std::stoi(args[1]);
    int32_t ret = SerialGetAttribute(portId, attribute);
    if (ret != UEC_OK) {
        ret = SerialOpen(portId);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_SERIAL, "SerialOpen failed");
            dprintf(fd, "Port %zu: port does not exist\n", portId);
            return false;
        }
        ret = SerialGetAttribute(portId, attribute);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_SERIAL, "SerialGetAttribute failed");
        }
        SerialClose(portId);
    }
    dprintf(fd, "Port %zu: baudrate: %zu stopBits:%zu parity:%zu dataBits:%zu\n",
        portId, attribute.baudrate, attribute.stopBits, attribute.parity, attribute.dataBits);
    return true;
}

void SerialManager::SerialGetAttributeDump(int32_t fd, const std::vector<std::string>& args)
{
    size_t size = args.size();
    OHOS::HDI::Usb::Serial::V1_0::SerialAttribute attribute;
    if (size == 1) {
        std::vector<OHOS::HDI::Usb::Serial::V1_0::SerialPort> serialPortList;
        int32_t ret = SerialGetPortList(serialPortList);
        if (ret != UEC_OK) {
            USB_HILOGE(MODULE_USB_SERIAL, "SerialGetPortList failed");
            return;
        }
        size_t portCount = serialPortList.size();
        for (size_t i = 0; i < portCount; ++i) {
            if (i > INT32_MAX) {
                USB_HILOGE(MODULE_USB_SERIAL, "SerialGetPortList port index out of range");
                break;
            }
            int32_t portId = static_cast<int32_t>(serialPortList[i].portId);
            ret = SerialGetAttribute(portId, attribute);
            if (ret != UEC_OK) {
                SerialOpen(portId);
                SerialGetAttribute(portId, attribute);
                SerialClose(portId);
            }
            dprintf(fd, "Port %zu: baudrate: %zu stopBits:%zu parity:%zu dataBits:%zu\n",
                i, attribute.baudrate, attribute.stopBits, attribute.parity, attribute.dataBits);
        }
    } else if (size == DUMP_PARAMS_NUM_2) {
        if (!CheckDataAndProcessPortId(fd, args, attribute)) {
            USB_HILOGE(MODULE_USB_SERIAL, "CheckDataAndProcessPortId failed");
            return;
        }
    } else {
        ListGetDumpHelp(fd);
    }
}

void SerialManager::ListGetDumpHelp(int32_t fd)
{
    dprintf(fd, "=========== dump the serial port information ===========\n");
    dprintf(fd, "serail -l: Query All Serial Port List\n");
    dprintf(fd, "serial -h: Serial port help\n");
    dprintf(fd, "serial -g : Gets the properties of all ports\n");
    dprintf(fd, "serial \"-g [port number]\": Gets the properties of the specified port\n");
    dprintf(fd, "------------------------------------------------\n");
}

bool SerialManager::GetSerialPort(int32_t portId, OHOS::HDI::Usb::Serial::V1_0::SerialPort& serialPort)
{
    if (serialPortMap_.find(portId) == serialPortMap_.end()) {
        USB_HILOGI(MODULE_USB_SERIAL, "serialPort not found");
        return false;
    }

    serialPort = serialPortMap_[portId];
    return true;
}

bool SerialManager::QueryAndRecordFirstRead(int32_t portId)
{
    std::lock_guard lock(readStatusMutex_);
    return portsHasBeenRead_.insert(portId).second; // return true if not read before
}

bool SerialManager::QueryAndRecordFirstWrite(int32_t portId)
{
    std::lock_guard lock(writeStatusMutex_);
    return portsHasBeenWritten_.insert(portId).second; // return true if not written before
}

void SerialManager::ResetFirstRead(int32_t portId)
{
    std::lock_guard lock(readStatusMutex_);
    portsHasBeenRead_.erase(portId);
}

void SerialManager::ResetFirstWrite(int32_t portId)
{
    std::lock_guard lock(writeStatusMutex_);
    portsHasBeenWritten_.erase(portId);
}

void SerialManager::ReportSerialOperationSecurityInfo(int32_t portId, std::string operationType, uint64_t time)
{
    USB_HILOGI(MODULE_USB_SERIAL, "%{public}s enter: operation=%{public}s", __func__, operationType.c_str());
    nlohmann::json operationJson {
        {"happenTime", time},
        {"portId", portId},
        {"operationType", operationType},
        {"uid", IPCSkeleton::GetCallingUid()},
    };
    USB::UsbSecurityReport::ReportSecurityInfo(USB_SERIAL_EVENT_ID, std::string(SERIAL_VERSION), operationJson);
}

}  //namespace SERIAL
}  //namespace OHOS
