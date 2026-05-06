/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#ifndef SERIAL_ASYNC_CONTEXT_H
#define SERIAL_ASYNC_CONTEXT_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "usb_api_metrics.h"

namespace OHOS {
namespace USB {
const int32_t BAUDRATE_50 = 50;
const int32_t BAUDRATE_75 = 75;
const int32_t BAUDRATE_110 = 110;
const int32_t BAUDRATE_134 = 134;
const int32_t BAUDRATE_150 = 150;
const int32_t BAUDRATE_200 = 200;
const int32_t BAUDRATE_300 = 300;
const int32_t BAUDRATE_600 = 600;
const int32_t BAUDRATE_1200 = 1200;
const int32_t BAUDRATE_1800 = 1800;
const int32_t BAUDRATE_2400 = 2400;
const int32_t BAUDRATE_4800 = 4800;
const int32_t BAUDRATE_9600 = 9600;
const int32_t BAUDRATE_19200 = 19200;
const int32_t BAUDRATE_38400 = 38400;
const int32_t BAUDRATE_57600 = 57600;
const int32_t BAUDRATE_115200 = 115200;
const int32_t BAUDRATE_230400 = 230400;
const int32_t BAUDRATE_460800 = 460800;
const int32_t BAUDRATE_500000 = 500000;
const int32_t BAUDRATE_576000 = 576000;
const int32_t BAUDRATE_921600 = 921600;
const int32_t BAUDRATE_1000000 = 1000000;
const int32_t BAUDRATE_1152000 = 1152000;
const int32_t BAUDRATE_1500000 = 1500000;
const int32_t BAUDRATE_2000000 = 2000000;
const int32_t BAUDRATE_2500000 = 2500000;
const int32_t BAUDRATE_3000000 = 3000000;
const int32_t BAUDRATE_3500000 = 3500000;
const int32_t BAUDRATE_4000000 = 4000000;

const int32_t DATABIT_8 = 8;
const int32_t DATABIT_7 = 7;
const int32_t DATABIT_6 = 6;
const int32_t DATABIT_5 = 5;

const int32_t PARITY_NONE = 0;
const int32_t PARITY_ODD = 1;
const int32_t PARITY_EVEN = 2;
const int32_t PARITY_MARK = 3;
const int32_t PARITY_SPACE = 4;

const int32_t STOPBIT_1 = 0;
const int32_t STOPBIT_2 = 1;

typedef struct SerialWriteAsyncContext {
    int contextErrno;
    int ret;
    napi_deferred deferred;
    napi_async_work work;
    int32_t portId;
    uint32_t size;
    uint32_t timeout;
    void* pData;
    napi_ref bufferRef;
    UsbApiMetrics* metrics = nullptr;
} SerialWriteAsyncContext;

typedef struct SerialReadAsyncContext {
    int contextErrno;
    int ret;
    napi_deferred deferred;
    napi_async_work work;
    int32_t portId;
    uint32_t size;
    uint32_t timeout;
    void* pData;
    UsbApiMetrics* metrics = nullptr;
} SerialReadAsyncContext;

typedef struct SerialRequestRightAsyncContext {
    int contextErrno;
    bool hasRight;
    napi_async_work work;
    napi_deferred deferred;
    int32_t portIdValue;
    UsbApiMetrics* metrics = nullptr;
} SerialRequestRightAsyncContext;
} // namespace USB
} // namespace OHOS
#endif // SERIAL_ASYNC_CONTEXT_H