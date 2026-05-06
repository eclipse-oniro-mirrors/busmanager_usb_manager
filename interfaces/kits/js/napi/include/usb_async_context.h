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
#ifndef USB_ASYNC_CONTEXT_H
#define USB_ASYNC_CONTEXT_H

#include <chrono>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "usb_device_pipe.h"
#include "usb_endpoint.h"
#include "usb_request.h"
#include "usb_accessory.h"
#include "usb_api_metrics.h"

namespace OHOS {
namespace USB {
const int32_t NONE = 0;
const int32_t SOURCE = 1;
const int32_t SINK = 2;

const int32_t HOST = 1;
const int32_t DEVICE = 2;

const int32_t UFP = 1;
const int32_t DFP = 2;
const int32_t DRP = 3;
const int32_t NUM_MODES = 4;

const int32_t USB_REQUEST_TARGET_DEVICE = 0;
const int32_t USB_REQUEST_TARGET_INTERFACE = 1;
const int32_t USB_REQUEST_TARGET_ENDPOINT = 2;
const int32_t USB_REQUEST_TARGET_OTHER = 3;

const int32_t USB_REQUEST_TYPE_STANDARD = 0;
const int32_t USB_REQUEST_TYPE_CLASS = 1;
const int32_t USB_REQUEST_TYPE_VENDOR = 2;

const int32_t USB_REQUEST_DIR_TO_DEVICE = 0;
const int32_t USB_REQUEST_DIR_FROM_DEVICE = 0x80;

const int32_t ACM = 1;
const int32_t ECM = 1 << 1;
const int32_t HDC = 1 << 2;
const int32_t MTP = 1 << 3;
const int32_t PTP = 1 << 4;
const int32_t RNDIS = 1 << 5;
const int32_t MIDI = 1 << 6;
const int32_t AUDIO_SOURCE = 1 << 7;
const int32_t NCM = 1 << 8;
const int32_t STORAGE = 1 << 9;

struct USBAsyncContext {
    napi_env env;
    napi_async_work work;

    napi_deferred deferred;
    napi_status status;
    UsbApiMetrics* metrics = nullptr;
};

struct USBRightAsyncContext : USBAsyncContext {
    std::string deviceName;
    bool hasRight = false;
};

struct USBAccessoryRightAsyncContext : USBAsyncContext {
    USBAccessory accessory;
    bool hasRight = false;
    int32_t errCode;
};

struct USBFunctionAsyncContext : USBAsyncContext {
    int32_t functions;
    int32_t errCode;
    std::string metricsName;
};

struct USBPortRoleAsyncContext : USBAsyncContext {
    int32_t portId;
    int32_t powerRole;
    int32_t dataRole;
    int32_t errCode;
    std::string metricsName;
};

struct USBControlTransferAsyncContext : USBAsyncContext {
    USBDevicePipe pipe;
    int32_t request;
    int32_t target;
    uint32_t reqType;
    int32_t directon;
    int32_t value;
    int32_t index;
    uint8_t *buffer;
    uint32_t bufferLength;
    uint32_t dataSize;
    int32_t timeOut = 0;
};

struct USBDeviceControlTransferAsyncContext : USBAsyncContext {
    USBDevicePipe pipe;
    uint32_t reqType;
    int32_t request;
    int32_t value;
    int32_t index;
    int32_t length;
    uint8_t *buffer;
    uint32_t bufferLength;
    uint32_t dataSize;
    int32_t timeOut = 0;
};

struct USBBulkTransferAsyncContext : USBAsyncContext {
    uint8_t *buffer;
    uint32_t bufferLength;
    uint32_t dataSize;
    int32_t timeOut = 0;
    USBDevicePipe pipe;
    USBEndpoint endpoint;
};

struct AsyncCallBackContext {
    napi_env env{nullptr};
    napi_deferred deferred{nullptr};
    napi_ref callbackRef{nullptr};
    
    int32_t actualLength;
    int32_t status;

    std::vector<HDI::Usb::V1_2::UsbIsoPacketDescriptor> isoInfo;
};

struct TimesUse {
    std::chrono::steady_clock::time_point beginTime;
    std::chrono::steady_clock::time_point endTime;
};

struct USBTransferAsyncContext : USBAsyncContext {
    USBDevicePipe pipe;
    int32_t endpoint;
    int32_t flags;
    int32_t type;
    int32_t status;
    int32_t timeOut = 0;
    int32_t length;
    int32_t actualLength;
    size_t bufferLength = 0;
    sptr<Ashmem> ashmem = nullptr;
    uint8_t *userData;
    uint8_t *buffer;
    uint32_t numIsoPackets;
    std::string name = "CreateAshmem";
    napi_ref callbackRef{nullptr};
};


} // namespace USB
} // namespace OHOS
#endif // USB_ASYNC_CONTEXT_H
