/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <unistd.h>

#include <sys/time.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <future>
#include <functional>

#include "hilog_wrapper.h"
#include "securec.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_common.h"
#include "napi_util.h"
#include "serial_async_context.h"
#include "serial_napi_errors.h"
#include "usb_api_metrics.h"
#include "usb_errors.h"

#include "usb_srv_client.h"
#include "usb_serial_type.h"

using namespace OHOS;
using namespace OHOS::USB;

const int32_t ARGC_1 = 1;
const int32_t ARGC_2 = 2;
const int32_t ARGC_3 = 3;

static UsbSrvClient &g_usbClient = UsbSrvClient::GetInstance();

static int32_t ErrorCodeConversion(int32_t value)
{
    static const std::map<int32_t, int32_t> errorMap = {
        {UEC_SERVICE_PERMISSION_DENIED_SYSAPI, SERIAL_SYSAPI_PERMISSION_DENIED},
        {UEC_SERVICE_PERMISSION_DENIED, SERIAL_INTERFACE_PERMISSION_DENIED},
        {UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED, SERIAL_SYSAPI_NOPERMISSION_CALL},
        {UEC_SERIAL_PORT_REPEAT_OPEN, SERIAL_PORT_OCCUPIED},
        {UEC_SERIAL_PORT_REPEAT_CLOSE, SERIAL_PORT_OCCUPIED},
        {UEC_SERIAL_PORT_OCCUPIED, SERIAL_PORT_OCCUPIED},
        {UEC_SERIAL_DEVICENOTOPEN, SERIAL_PORT_NOT_OPEN},
        {UEC_SERIAL_PORT_NOT_OPEN, SERIAL_PORT_NOT_OPEN},
        {UEC_INTERFACE_TIMED_OUT, SERIAL_TIMED_OUT},
        {UEC_SERIAL_IO_EXCEPTION, SERIAL_IO_EXCEPTION},
        {UEC_SERIAL_PORT_NOT_EXIST, SERIAL_PORT_NOT_EXIST},
        {UEC_SERIAL_DATEBASE_ERROR, UEC_COMMON_RIGHT_DATABASE_ERROR}
    };
    auto it = errorMap.find(value);
    if (it != errorMap.end()) {
        return it->second;
    }
    return SERIAL_SERVICE_ABNORMAL;
}

static napi_value SerialGetPortListNapi(napi_env env, napi_callback_info info)
{
    UsbApiMetrics metrics("BasicServicesKit.SerialManager.GetPortList");
    USB_HILOGI(MODULE_USB_NAPI, "SerialGetPortListNapi start");
    std::vector<OHOS::USB::UsbSerialPort> portIds;
    int32_t ret = g_usbClient.SerialGetPortList(portIds);
    if (ret < 0) {
        metrics.SetErrorCode(ret);
        USB_HILOGE(MODULE_USB_NAPI, "SerialGetPortList failed");
    }
    napi_value result = nullptr;
    napi_create_array(env, &result);
    if (ret < 0) {
        return result;
    }
    for (uint32_t i = 0; i < portIds.size(); ++i) {
        napi_value portObj;
        napi_create_object(env, &portObj);
        NapiUtil::SetValueInt32(env, "portId", portIds[i].portId_, portObj);
        std::string deviceName = std::to_string(portIds[i].busNum_) + "-" +
            std::to_string(portIds[i].devAddr_);
        NapiUtil::SetValueUtf8String(env, "deviceName", deviceName, portObj);
        USB_HILOGI(MODULE_USB_NAPI, "portId: %{public}d", portIds[i].portId_);
        USB_HILOGI(MODULE_USB_NAPI, "deviceName: %{public}s", deviceName.c_str());
        napi_set_element(env, result, i, portObj);
    }
    return result;
}

static napi_value SerialGetAttributeNapi(napi_env env, napi_callback_info info)
{
    UsbApiMetrics metrics("BasicServicesKit.SerialManager.GetAttribute");
    USB_HILOGI(MODULE_USB_NAPI, "SerialGetAttributeNapi start");
    size_t argc = ARGC_1;
    napi_value argv[ARGC_1] = {nullptr};
    if (!CheckNapiResult(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr),
        "Get call back info failed")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        CheckAndThrowOnError(env, false, SYSPARAM_INVALID_INPUT, "get arguments failed.");
        return nullptr;
    }
    if (!CheckAndThrowOnError(env, (argc == ARGC_1), SYSPARAM_INVALID_INPUT, "The function takes 1 arguments.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    napi_value portId = argv[0];
    napi_valuetype type;
    napi_typeof(env, portId, &type);
    if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT, "The type of arg0 must be int32_t.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    int32_t portIdValue = -1;
    napi_get_value_int32(env, portId, &portIdValue);
    if (!CheckAndThrowOnError(env, (portIdValue != -1), SYSPARAM_INVALID_INPUT, "Failed to get portId.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    UsbSerialAttr serialAttribute;
    int32_t ret = g_usbClient.SerialGetAttribute(portIdValue, serialAttribute);
    if (!CheckAndThrowOnError(env, (ret == 0), ErrorCodeConversion(ret), "Failed to get attribute.")) {
        metrics.SetErrorCode(ErrorCodeConversion(ret));
        return nullptr;
    }
    napi_value result = nullptr;
    napi_create_object(env, &result);
    NapiUtil::SetValueUint32(env, "baudRate", serialAttribute.baudRate_, result);
    NapiUtil::SetValueUint32(env, "dataBits", serialAttribute.dataBits_, result);
    NapiUtil::SetValueUint32(env, "parity", serialAttribute.parity_, result);
    NapiUtil::SetValueUint32(env, "stopBits", serialAttribute.stopBits_, result);
    return result;
}

static bool ParseSetAttributeInterfaceParams(napi_env env, napi_callback_info info,
    int32_t& portIdValue, UsbSerialAttr& serialAttribute)
{
    USB_HILOGI(MODULE_USB_NAPI, "ParseSetAttributeInterfaceParams start");
    size_t argc = ARGC_2;
    napi_value argv[ARGC_2] = { nullptr };
    if (!CheckNapiResult(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr),
        "Get call back info failed")) {
        CheckAndThrowOnError(env, false, SYSPARAM_INVALID_INPUT, "get arguments failed.");
        return false;
    }
    if (!CheckAndThrowOnError(env, (argc == ARGC_2), SYSPARAM_INVALID_INPUT, "The function takes 2 arguments.")) {
        return false;
    }
    napi_value portId = argv[0];
    napi_valuetype type;
    napi_typeof(env, portId, &type);
    if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT, "The type of arg0 must be int32_t.")) {
        return false;
    }
    napi_get_value_int32(env, portId, &portIdValue);
    if (!CheckAndThrowOnError(env, (portIdValue != -1), SYSPARAM_INVALID_INPUT, "Failed to get portId.")) {
        return false;
    }
    napi_value obj = argv[1];
    napi_typeof(env, obj, &type);
    if (!CheckAndThrowOnError(env, type == napi_object, SYSPARAM_INVALID_INPUT,
        "The type of arg1 must be SerialAttribute.")) {
        return false;
    }
    bool result = NapiUtil::JsObjectToUint32(env, obj, "baudRate", serialAttribute.baudRate_) &&
        NapiUtil::JsObjectToUint8(env, obj, "dataBits", serialAttribute.dataBits_) &&
        NapiUtil::JsObjectToUint8(env, obj, "parity", serialAttribute.parity_) &&
        NapiUtil::JsObjectToUint8(env, obj, "stopBits", serialAttribute.stopBits_);
    if (!CheckAndThrowOnError(env, result == true, SYSPARAM_INVALID_INPUT,
        "Parse attribute failed.")) {
        return false;
    }
    return true;
}

static napi_value SerialSetAttributeNapi(napi_env env, napi_callback_info info)
{
    UsbApiMetrics metrics("BasicServicesKit.SerialManager.SetAttribute");
    USB_HILOGI(MODULE_USB_NAPI, "SerialSetAttributeNapi start");
    UsbSerialAttr serialAttribute;
    int32_t portIdValue = -1;
    if (!ParseSetAttributeInterfaceParams(env, info, portIdValue, serialAttribute)) {
        return nullptr;
    }
    USB_HILOGI(MODULE_USB_NAPI, "SetAttributeNapi portIdValue: %{public}d", portIdValue);
    int ret = g_usbClient.SerialSetAttribute(portIdValue, serialAttribute);
    if (!CheckAndThrowOnError(env, (ret == 0), ErrorCodeConversion(ret), "Failed to set attribute.")) {
        metrics.SetErrorCode(ErrorCodeConversion(ret));
        return nullptr;
    }
    return nullptr;
}

static bool ParseWriteInterfaceParams(napi_env env, napi_callback_info info,
    int32_t& portIdValue, napi_value* buffer, uint32_t& timeoutValue)
{
    USB_HILOGI(MODULE_USB_NAPI, "ParseWriteInterfaceParams start");
    size_t argc = ARGC_3;
    napi_value argv[ARGC_3] = {nullptr};
    if (!CheckNapiResult(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr),
        "Get call back info failed")) {
        CheckAndThrowOnError(env, false, SYSPARAM_INVALID_INPUT, "get arguments failed.");
        return false;
    }
    if (!CheckAndThrowOnError(env, (argc == ARGC_2 || argc == ARGC_3), SYSPARAM_INVALID_INPUT,
        "The function takes 2 or 3 arguments.")) {
        return false;
    }
    napi_value portId = argv[0];
    napi_valuetype type;
    napi_typeof(env, portId, &type);
    if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT,
        "The type of portId must be int32_t.")) {
        return false;
    }
    napi_get_value_int32(env, portId, &portIdValue);
    if (!CheckAndThrowOnError(env, (portIdValue != -1), SYSPARAM_INVALID_INPUT, "Failed to get portId.")) {
        return false;
    }
    *buffer = argv[1];
    if (argc == ARGC_3) {
        napi_value timeout = argv[2];
        napi_typeof(env, timeout, &type);
        if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT,
            "The type of timeout must be uint32_t.")) {
            return false;
        }
        napi_get_value_uint32(env, timeout, &timeoutValue);
    }
    return true;
}

static napi_value SerialWriteSyncNapi(napi_env env, napi_callback_info info)
{
    UsbApiMetrics metrics("BasicServicesKit.SerialManager.WriteSync");
    USB_HILOGI(MODULE_USB_NAPI, "SerialWriteSyncNapi start");
    int32_t portIdValue = -1;
    napi_value buffer;
    uint32_t timeoutValue = 0;
    if (!ParseWriteInterfaceParams(env, info, portIdValue, &buffer, timeoutValue)) {
        return nullptr;
    }
    napi_typedarray_type arrayType;
    size_t bufferLength;
    void* bufferValue = nullptr;
    napi_get_typedarray_info(env, buffer, &arrayType, &bufferLength, &bufferValue, nullptr, nullptr);
    if (!CheckAndThrowOnError(env, arrayType == napi_uint8_array, SYSPARAM_INVALID_INPUT,
        "The type of buffer must be an array of uint8_t.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    std::vector<uint8_t> bufferVector(static_cast<uint8_t*>(bufferValue),
        static_cast<uint8_t*>(bufferValue) + bufferLength);

    uint32_t actualSize = 0;
    int32_t ret = g_usbClient.SerialWrite(portIdValue, bufferVector, bufferLength, actualSize, timeoutValue);
    if (!CheckAndThrowOnError(env, (ret == 0), ErrorCodeConversion(ret), "SerialWrite Failed.")) {
        metrics.SetErrorCode(ErrorCodeConversion(ret));
        return nullptr;
    }

    napi_value result = nullptr;
    napi_create_int32(env, actualSize, &result);
    return result;
}

static auto g_serialWriteExecute = [](napi_env env, void* data) {
    SerialWriteAsyncContext *context = static_cast<SerialWriteAsyncContext *>(data);
    context->metrics = new UsbApiMetrics("BasicServicesKit.SerialManager.Write");
    if (context->contextErrno) {
        USB_HILOGE(MODULE_USB_NAPI, "ExecuteCallback failed, reason: napi_get_reference_value");
        return;
    }

    void* bufferValue = context->pData;
    if (context->size == 0) {
        USB_HILOGE(MODULE_USB_NAPI, "no valid data to write");
        return;
    }

    std::vector<uint8_t> bufferVector(static_cast<uint8_t*>(bufferValue),
        static_cast<uint8_t*>(bufferValue) + context->size);

    uint32_t actualSize = 0;
    int32_t ret = g_usbClient.SerialWrite(context->portId, bufferVector, context->size, actualSize, context->timeout);
    if (ret != 0) {
        context->contextErrno = ErrorCodeConversion(ret);
        context->metrics->SetErrorCode(ret);
    }

    context->ret = actualSize;
};

static auto g_serialWriteComplete = [](napi_env env, napi_status status, void *data) {
    SerialWriteAsyncContext *context = static_cast<SerialWriteAsyncContext *>(data);
    napi_value result = nullptr;
    if (context->contextErrno) {
        napi_create_int32(env, context->contextErrno, &result);
        napi_reject_deferred(env, context->deferred, result);
    } else {
        napi_create_int32(env, context->ret, &result);
        napi_resolve_deferred(env, context->deferred, result);
    }
    napi_delete_async_work(env, context->work);
    if (context->bufferRef) {
        napi_delete_reference(env, context->bufferRef);
    }
    delete context->metrics;
    delete context;
};

static napi_value SerialWriteNapi(napi_env env, napi_callback_info info)
{
    USB_HILOGI(MODULE_USB_NAPI, "start write Napi");
    int32_t portIdValue = -1;
    napi_value buffer;
    uint32_t timeoutValue = 0;
    if (!ParseWriteInterfaceParams(env, info, portIdValue, &buffer, timeoutValue)) {
        return nullptr;
    }
    napi_typedarray_type arrayType;
    size_t bufferLength;
    void* bufferValue = nullptr;
    napi_get_typedarray_info(env, buffer, &arrayType, &bufferLength, &bufferValue, nullptr, nullptr);
    if (!CheckAndThrowOnError(env, arrayType == napi_uint8_array, SYSPARAM_INVALID_INPUT,
        "The type of buffer must be an array of uint8_t.")) {
        return nullptr;
    }
    SerialWriteAsyncContext *asyncContext = new (std::nothrow) SerialWriteAsyncContext;
    if (asyncContext == nullptr) {
        USB_HILOGE(MODULE_USB_NAPI, "new SerialWriteAsyncContext failed!");
        CheckAndThrowOnError(env, false, SYSPARAM_INVALID_INPUT, "asyncContext is null");
        return nullptr;
    }
    napi_value promise;
    if (napi_create_promise(env, &asyncContext->deferred, &promise)) {
        USB_HILOGE(MODULE_USB_NAPI, "create promise failed!");
        CheckAndThrowOnError(env, false, SERIAL_SERVICE_ABNORMAL, "promise is null");
        delete asyncContext;
        asyncContext = nullptr;
        return nullptr;
    }
    asyncContext->portId = portIdValue;
    asyncContext->timeout = timeoutValue;
    asyncContext->contextErrno = 0;
    asyncContext->size = bufferLength;
    asyncContext->pData = bufferValue;
    napi_ref bufferRef = nullptr;
    napi_status refStatus = napi_create_reference(env, buffer, 1, &bufferRef);
    if (refStatus != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "create buffer reference failed");
        delete asyncContext;
        return nullptr;
    }
    asyncContext->bufferRef = bufferRef;
    napi_value resourceName;
    napi_create_string_utf8(env, "SerialWrite", NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(env, nullptr, resourceName, g_serialWriteExecute, g_serialWriteComplete, asyncContext,
        &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);
    return promise;
}

static bool ParseReadInterfaceParams(napi_env env, napi_callback_info info, int32_t& portIdValue,
    napi_value* buffer, uint32_t& timeoutValue)
{
    size_t argc = ARGC_3;
    napi_value argv[ARGC_3] = {nullptr};
    if (!CheckNapiResult(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr),
        "Get call back info failed")) {
        CheckAndThrowOnError(env, false, SYSPARAM_INVALID_INPUT, "get arguments failed.");
        return false;
    }
    if (!CheckAndThrowOnError(env, (argc == ARGC_2 || argc == ARGC_3), SYSPARAM_INVALID_INPUT,
        "The function takes 2 or 3 arguments.")) {
        return false;
    }
    napi_value portId = argv[0];
    napi_valuetype type;
    napi_typeof(env, portId, &type);
    if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT,
        "The type of portId must be int32_t.")) {
        return false;
    }
    napi_get_value_int32(env, portId, &portIdValue);
    if (!CheckAndThrowOnError(env, (portIdValue != -1), SYSPARAM_INVALID_INPUT, "Failed to get portId.")) {
        return false;
    }
    *buffer = argv[1];
    if (argc == ARGC_3) {
        napi_value timeout = argv[2];
        napi_typeof(env, timeout, &type);
        if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT,
            "The type of timeout must be uint32_t.")) {
            return false;
        }
        napi_get_value_uint32(env, timeout, &timeoutValue);
    }
    return true;
}

static napi_value SerialReadSyncNapi(napi_env env, napi_callback_info info)
{
    UsbApiMetrics metrics("BasicServicesKit.SerialManager.ReadSync");
    USB_HILOGI(MODULE_USB_NAPI, "SerialReadSyncNapi start");
    int32_t portIdValue = -1;
    napi_value buffer;
    uint32_t timeoutValue = 0;
    if (!ParseReadInterfaceParams(env, info, portIdValue, &buffer, timeoutValue)) {
        return nullptr;
    }
    napi_typedarray_type arrayType;
    size_t bufferLength;
    void* bufferValue = nullptr;
    napi_get_typedarray_info(env, buffer, &arrayType, &bufferLength, &bufferValue, nullptr, nullptr);
    if (!CheckAndThrowOnError(env, arrayType == napi_uint8_array, SYSPARAM_INVALID_INPUT,
        "The type of buffer must be an array of uint8_t.")) {
        return nullptr;
    }

    std::vector<uint8_t> bufferData;
    uint32_t actualSize = 0;
    int32_t ret = g_usbClient.SerialRead(portIdValue, bufferData, bufferLength,
        actualSize, timeoutValue);
    if (!CheckAndThrowOnError(env, (ret == 0), ErrorCodeConversion(ret), "SerialReadSync Failed.")) {
        return nullptr;
    }
    ret = memcpy_s(bufferValue, bufferLength, bufferData.data(), bufferData.size());
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_NAPI,
            "serial read sync, memcpy_s failed ret: %{public}d", ret);
    }
    napi_value result = nullptr;
    napi_create_int32(env, actualSize, &result);
    return result;
}

static auto g_serialReadExecute = [](napi_env env, void* data) {
    SerialReadAsyncContext *context = static_cast<SerialReadAsyncContext *>(data);
    context->metrics = new UsbApiMetrics("BasicServicesKit.SerialManager.Read");
    uint32_t actualSize = 0;
    std::vector<uint8_t> bufferData;
    int32_t ret = g_usbClient.SerialRead(context->portId, bufferData,
        context->size, actualSize, context->timeout);
    if (ret != 0) {
        context->contextErrno = ErrorCodeConversion(ret);
        context->metrics->SetErrorCode(ret);
    }
    ret = memcpy_s(context->pData, context->size, bufferData.data(), bufferData.size());
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_NAPI, "memcpy_s failed ret: %{public}d", ret);
    }
    context->ret = actualSize;
};

static auto g_serialReadComplete = [](napi_env env, napi_status status, void* data) {
    SerialReadAsyncContext *context = static_cast<SerialReadAsyncContext *>(data);
    napi_value result = nullptr;
    if (context->contextErrno) {
        napi_create_int32(env, context->contextErrno, &result);
        napi_reject_deferred(env, context->deferred, result);
    } else {
        napi_create_int32(env, context->ret, &result);
        napi_resolve_deferred(env, context->deferred, result);
    }
    napi_delete_async_work(env, context->work);
    delete context->metrics;
    delete context;
};

static napi_value SerialReadNapi(napi_env env, napi_callback_info info)
{
    USB_HILOGI(MODULE_USB_NAPI, "SerialReadNapi start");
    int32_t portIdValue = -1;
    napi_value buffer;
    uint32_t timeoutValue = 0;
    if (!ParseReadInterfaceParams(env, info, portIdValue, &buffer, timeoutValue)) {
        return nullptr;
    }
    napi_typedarray_type arrayType;
    size_t bufferLength;
    void* bufferValue = nullptr;
    napi_get_typedarray_info(env, buffer, &arrayType, &bufferLength, &bufferValue, nullptr, nullptr);
    if (!CheckAndThrowOnError(env, arrayType == napi_uint8_array, SYSPARAM_INVALID_INPUT,
        "The type of buffer must be an array of uint8_t.")) {
        return nullptr;
    }
    SerialReadAsyncContext *asyncContext = new (std::nothrow) SerialReadAsyncContext;
    if (!CheckAndThrowOnError(env, asyncContext != nullptr, SYSPARAM_INVALID_INPUT, "Failed to create promise.")) {
        return nullptr;
    }
    napi_value promise;
    if (napi_create_promise(env, &asyncContext->deferred, &promise)) {
        USB_HILOGE(MODULE_USB_NAPI, "create promise failed!");
        CheckAndThrowOnError(env, false, SERIAL_SERVICE_ABNORMAL, "promise is null");
        delete asyncContext;
        asyncContext = nullptr;
        return nullptr;
    }
    asyncContext->portId = portIdValue;
    asyncContext->timeout = timeoutValue;
    asyncContext->contextErrno = 0;
    asyncContext->size = bufferLength;
    asyncContext->pData = bufferValue;
    napi_value resourceName;
    napi_create_string_utf8(env, "SerialRead", NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(env, nullptr, resourceName, g_serialReadExecute, g_serialReadComplete, asyncContext,
        &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);
    return promise;
}

static napi_value SerialOpenNapi(napi_env env, napi_callback_info info)
{
    UsbApiMetrics metrics("BasicServicesKit.SerialManager.Open");
    USB_HILOGI(MODULE_USB_NAPI, "serialOpenNapi start");
    size_t argc = ARGC_1;
    napi_value argv[ARGC_1] = {nullptr};
    if (!CheckNapiResult(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr),
        "Get call back info failed")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        CheckAndThrowOnError(env, false, SYSPARAM_INVALID_INPUT, "get arguments failed.");
        return nullptr;
    }
    if (!CheckAndThrowOnError(env, (argc == ARGC_1), SYSPARAM_INVALID_INPUT, "The function takes 1 arguments.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    napi_value portId = argv[0];
    napi_valuetype type;
    napi_typeof(env, portId, &type);
    if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT,
        "The type of portId must be int32_t.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    int32_t portIdValue = -1;
    napi_get_value_int32(env, portId, &portIdValue);
    if (!CheckAndThrowOnError(env, (portIdValue != -1), SYSPARAM_INVALID_INPUT, "Failed to get portId.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    USB_HILOGE(MODULE_USB_NAPI, "portIdValue: %{public}d", portIdValue);
    int ret = g_usbClient.SerialOpen(portIdValue);
    if (!CheckAndThrowOnError(env, ret == 0, ErrorCodeConversion(ret), "SerialOpen failed.")) {
        metrics.SetErrorCode(ErrorCodeConversion(ret));
        return nullptr;
    }

    return nullptr;
}

static napi_value SerialCloseNapi(napi_env env, napi_callback_info info)
{
    UsbApiMetrics metrics("BasicServicesKit.SerialManager.Close");
    USB_HILOGI(MODULE_USB_NAPI, "SerialCloseNapi start");
    size_t argc = ARGC_1;
    napi_value argv[ARGC_1] = {nullptr};
    if (!CheckNapiResult(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr),
        "Get call back info failed")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        CheckAndThrowOnError(env, false, SYSPARAM_INVALID_INPUT, "get arguments failed.");
        return nullptr;
    }
    if (!CheckAndThrowOnError(env, (argc == ARGC_1), SYSPARAM_INVALID_INPUT, "The function takes 1 arguments.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    napi_value portId = argv[0];
    napi_valuetype type;
    napi_typeof(env, portId, &type);
    if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT,
        "The type of portId must be int32_t.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    int32_t portIdValue = -1;
    napi_get_value_int32(env, portId, &portIdValue);
    if (!CheckAndThrowOnError(env, (portIdValue != -1), SYSPARAM_INVALID_INPUT, "Failed to get portId.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    
    int ret = g_usbClient.SerialClose(portIdValue);
    if (!CheckAndThrowOnError(env, ret == 0, ErrorCodeConversion(ret), "SerialClose failed.")) {
        metrics.SetErrorCode(ErrorCodeConversion(ret));
        return nullptr;
    }
    return nullptr;
}

static napi_value SerialHasRightNapi(napi_env env, napi_callback_info info)
{
    UsbApiMetrics metrics("BasicServicesKit.SerialManager.HasSerialRight");
    USB_HILOGI(MODULE_USB_NAPI, "SerialHasRightNapi start");
    size_t argc = ARGC_1;
    napi_value argv[ARGC_1] = {nullptr};
    if (!CheckNapiResult(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr),
        "Get call back info failed")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        CheckAndThrowOnError(env, false, SYSPARAM_INVALID_INPUT, "get arguments failed.");
        return nullptr;
    }
    if (!CheckAndThrowOnError(env, (argc == ARGC_1), SYSPARAM_INVALID_INPUT, "The function takes 1 arguments.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    napi_value portId = argv[0];
    napi_valuetype type;
    napi_typeof(env, portId, &type);
    if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT,
        "The type of portId must be int32_t.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    int32_t portIdValue = -1;
    napi_get_value_int32(env, portId, &portIdValue);
    if (!CheckAndThrowOnError(env, (portIdValue != -1), SYSPARAM_INVALID_INPUT, "Failed to get portId.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    napi_value result = nullptr;
    bool hasRight = false;
    int32_t ret = g_usbClient.HasSerialRight(portIdValue, hasRight);
    if (!CheckAndThrowOnError(env, (ret == 0), ErrorCodeConversion(ret), "SerialHasRight failed.")) {
        metrics.SetErrorCode(ErrorCodeConversion(ret));
        return nullptr;
    }
    napi_get_boolean(env, hasRight, &result);
    return result;
}

static napi_value CancelSerialRightNapi(napi_env env, napi_callback_info info)
{
    UsbApiMetrics metrics("BasicServicesKit.SerialManager.CancelSerialRight");
    USB_HILOGI(MODULE_USB_NAPI, "CancelSerialRightNapi start");
    size_t argc = ARGC_1;
    napi_value argv[ARGC_1] = {nullptr};
    if (!CheckNapiResult(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr),
        "Get call back info failed")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        CheckAndThrowOnError(env, false, SYSPARAM_INVALID_INPUT, "get arguments failed.");
        return nullptr;
    }
    if (!CheckAndThrowOnError(env, (argc == ARGC_1), SYSPARAM_INVALID_INPUT, "The function takes 1 arguments.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    napi_value portId = argv[0];
    napi_valuetype type;
    napi_typeof(env, portId, &type);
    if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT,
        "The type of portId must be int32_t.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    int32_t portIdValue = -1;
    napi_get_value_int32(env, portId, &portIdValue);
    if (!CheckAndThrowOnError(env, (portIdValue != -1), SYSPARAM_INVALID_INPUT, "Failed to get portId.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    int32_t ret = g_usbClient.CancelSerialRight(portIdValue);
    if (!CheckAndThrowOnError(env, ret == 0, ErrorCodeConversion(ret), "SerialRemoveRight failed.")) {
        metrics.SetErrorCode(ErrorCodeConversion(ret));
        return nullptr;
    }
    return nullptr;
}

static napi_value SerialAddRightNapi(napi_env env, napi_callback_info info)
{
    UsbApiMetrics metrics("BasicServicesKit.SerialManager.AddSerialRight");
    USB_HILOGI(MODULE_USB_NAPI, "SerialAddRightNapi start");
    size_t argc = ARGC_2;
    napi_value argv[ARGC_2] = {nullptr};
    if (!CheckNapiResult(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr),
        "Get call back info failed")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        CheckAndThrowOnError(env, false, SYSPARAM_INVALID_INPUT, "can not get arguments.");
        return nullptr;
    }
    if (!CheckAndThrowOnError(env, (argc == ARGC_2), SYSPARAM_INVALID_INPUT, "The function takes 2 arguments.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    napi_value tokenId = argv[0];
    napi_valuetype type;
    napi_typeof(env, tokenId, &type);
    if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT,
        "The type of tokenId must be int32_t.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    int32_t tokenIdValue = 0;
    napi_get_value_int32(env, tokenId, &tokenIdValue);
    if (!CheckAndThrowOnError(env, (tokenIdValue != 0), SYSPARAM_INVALID_INPUT, "Failed to get tokenId.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    napi_value portId = argv[1];
    napi_typeof(env, portId, &type);
    if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT,
        "The type of portId must be int32_t.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    int32_t portIdValue = -1;
    napi_get_value_int32(env, portId, &portIdValue);
    if (!CheckAndThrowOnError(env, (portIdValue != -1), SYSPARAM_INVALID_INPUT, "Failed to get portId.")) {
        metrics.SetErrorCode(SYSPARAM_INVALID_INPUT);
        return nullptr;
    }
    int32_t ret = g_usbClient.AddSerialRight(tokenIdValue, portIdValue);
    if (!CheckAndThrowOnError(env, ret == 0, ErrorCodeConversion(ret), "SerialAddRight failed.")) {
        metrics.SetErrorCode(ErrorCodeConversion(ret));
        return nullptr;
    }
    return nullptr;
}

static auto g_serialRequestRightExecute = [](napi_env env, void* data) {
    SerialRequestRightAsyncContext *asyncContext = static_cast<SerialRequestRightAsyncContext *>(data);
    asyncContext->metrics = new UsbApiMetrics("BasicServicesKit.SerialManager.RequestSerialRight");
    int32_t ret = g_usbClient.RequestSerialRight(asyncContext->portIdValue, asyncContext->hasRight);
    asyncContext->contextErrno = 0;
    if (ret != 0) {
        USB_HILOGE(MODULE_USB_NAPI, "request right has error");
        asyncContext->contextErrno = ErrorCodeConversion(ret);
        asyncContext->metrics->SetErrorCode(ret);
    }
};

static auto g_serialRequestRightComplete = [](napi_env env, napi_status status, void* data) {
    SerialRequestRightAsyncContext *asyncContext = static_cast<SerialRequestRightAsyncContext *>(data);
    napi_value result = nullptr;
    if (asyncContext->contextErrno) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to request serial right");
        napi_create_int32(env, asyncContext->contextErrno, &result);
        napi_reject_deferred(env, asyncContext->deferred, result);
    } else {
        napi_get_boolean(env, asyncContext->hasRight, &result);
        napi_resolve_deferred(env, asyncContext->deferred, result);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext->metrics;
    delete asyncContext;
};

static napi_value SerialRequestRightNapi(napi_env env, napi_callback_info info)
{
    size_t argc = ARGC_1;
    napi_value argv[ARGC_1] = {nullptr};
    if (!CheckNapiResult(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr),
        "Get call back info failed")) {
        CheckAndThrowOnError(env, false, SYSPARAM_INVALID_INPUT, "can not get arguments.");
        return nullptr;
    }
    if (!CheckAndThrowOnError(env, (argc == ARGC_1), SYSPARAM_INVALID_INPUT, "The function takes 1 arguments.")) {
        return nullptr;
    }
    napi_value portId = argv[0];
    napi_valuetype type;
    napi_typeof(env, portId, &type);
    if (!CheckAndThrowOnError(env, type == napi_number, SYSPARAM_INVALID_INPUT,
        "The type of portId must be int32_t.")) {
        return nullptr;
    }
    int32_t portIdValue = -1;
    napi_get_value_int32(env, portId, &portIdValue);
    if (!CheckAndThrowOnError(env, (portIdValue != -1), SYSPARAM_INVALID_INPUT, "Failed to get portId.")) {
        return nullptr;
    }
    SerialRequestRightAsyncContext* asyncContext = new (std::nothrow) SerialRequestRightAsyncContext;
    if (!CheckAndThrowOnError(env, asyncContext != nullptr, SYSPARAM_INVALID_INPUT, "Failed to create promise.")) {
        return nullptr;
    }
    asyncContext->portIdValue = portIdValue;
    asyncContext->contextErrno = 0;
    asyncContext->hasRight = false;
    napi_value result = nullptr;
    if (napi_create_promise(env, &asyncContext->deferred, &result)) {
        USB_HILOGE(MODULE_USB_NAPI, "create promise failed!");
        CheckAndThrowOnError(env, false, SERIAL_SERVICE_ABNORMAL, "promise is null");
        delete asyncContext;
        asyncContext = nullptr;
        return nullptr;
    }
    napi_value resourceName;
    napi_create_string_utf8(env, "SerialRequestRight", NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(env, nullptr, resourceName, g_serialRequestRightExecute,
        g_serialRequestRightComplete, static_cast<void*>(asyncContext), &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);
    return result;
}

static void SetEnumProperty(napi_env env, napi_value object, const std::string &name, int32_t value)
{
    if (name.empty()) {
        USB_HILOGE(MODULE_USB_NAPI, "Property name cannot be an empty string");
        return;
    }

    napi_value tempValue = nullptr;
    napi_status status = napi_create_int32(env, value, &tempValue);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create int32 value for enum %{public}s", name.c_str());
        return;
    }
    status = napi_set_named_property(env, object, name.c_str(), tempValue);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to set property %{public}s", name.c_str());
        return;
    }
}

static napi_value NapiCreateStopBitsTypeEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "STOPBIT_1", STOPBIT_1);
    SetEnumProperty(env, object, "STOPBIT_2", STOPBIT_2);
    return object;
}

static napi_value NapiCreateParityTypeEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "PARITY_NONE", PARITY_NONE);
    SetEnumProperty(env, object, "PARITY_ODD", PARITY_ODD);
    SetEnumProperty(env, object, "PARITY_EVEN", PARITY_EVEN);
    SetEnumProperty(env, object, "PARITY_MARK", PARITY_MARK);
    SetEnumProperty(env, object, "PARITY_SPACE", PARITY_SPACE);
    return object;
}

static napi_value NapiCreateDataBitsTypeEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "DATABIT_8", DATABIT_8);
    SetEnumProperty(env, object, "DATABIT_7", DATABIT_7);
    SetEnumProperty(env, object, "DATABIT_6", DATABIT_6);
    SetEnumProperty(env, object, "DATABIT_5", DATABIT_5);
    return object;
}

static napi_value NapiCreateBaudRatesTypeEnum(napi_env env)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        USB_HILOGE(MODULE_USB_NAPI, "Failed to create object");
        return nullptr;
    }
    SetEnumProperty(env, object, "BAUDRATE_50", BAUDRATE_50);
    SetEnumProperty(env, object, "BAUDRATE_75", BAUDRATE_75);
    SetEnumProperty(env, object, "BAUDRATE_110", BAUDRATE_110);
    SetEnumProperty(env, object, "BAUDRATE_134", BAUDRATE_134);
    SetEnumProperty(env, object, "BAUDRATE_150", BAUDRATE_150);
    SetEnumProperty(env, object, "BAUDRATE_200", BAUDRATE_200);
    SetEnumProperty(env, object, "BAUDRATE_300", BAUDRATE_300);
    SetEnumProperty(env, object, "BAUDRATE_600", BAUDRATE_600);
    SetEnumProperty(env, object, "BAUDRATE_1200", BAUDRATE_1200);
    SetEnumProperty(env, object, "BAUDRATE_1800", BAUDRATE_1800);
    SetEnumProperty(env, object, "BAUDRATE_2400", BAUDRATE_2400);
    SetEnumProperty(env, object, "BAUDRATE_4800", BAUDRATE_4800);
    SetEnumProperty(env, object, "BAUDRATE_9600", BAUDRATE_9600);
    SetEnumProperty(env, object, "BAUDRATE_19200", BAUDRATE_19200);
    SetEnumProperty(env, object, "BAUDRATE_38400", BAUDRATE_38400);
    SetEnumProperty(env, object, "BAUDRATE_57600", BAUDRATE_57600);
    SetEnumProperty(env, object, "BAUDRATE_115200", BAUDRATE_115200);
    SetEnumProperty(env, object, "BAUDRATE_230400", BAUDRATE_230400);
    SetEnumProperty(env, object, "BAUDRATE_460800", BAUDRATE_460800);
    SetEnumProperty(env, object, "BAUDRATE_500000", BAUDRATE_500000);
    SetEnumProperty(env, object, "BAUDRATE_576000", BAUDRATE_576000);
    SetEnumProperty(env, object, "BAUDRATE_921600", BAUDRATE_921600);
    SetEnumProperty(env, object, "BAUDRATE_1000000", BAUDRATE_1000000);
    SetEnumProperty(env, object, "BAUDRATE_1152000", BAUDRATE_1152000);
    SetEnumProperty(env, object, "BAUDRATE_1500000", BAUDRATE_1500000);
    SetEnumProperty(env, object, "BAUDRATE_2000000", BAUDRATE_2000000);
    SetEnumProperty(env, object, "BAUDRATE_2500000", BAUDRATE_2500000);
    SetEnumProperty(env, object, "BAUDRATE_3000000", BAUDRATE_3000000);
    SetEnumProperty(env, object, "BAUDRATE_3500000", BAUDRATE_3500000);
    SetEnumProperty(env, object, "BAUDRATE_4000000", BAUDRATE_4000000);
    return object;
}

static napi_value DeclareEnum(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("StopBits", NapiCreateStopBitsTypeEnum(env)),
        DECLARE_NAPI_STATIC_PROPERTY("Parity", NapiCreateParityTypeEnum(env)),
        DECLARE_NAPI_STATIC_PROPERTY("DataBits", NapiCreateDataBitsTypeEnum(env)),
        DECLARE_NAPI_STATIC_PROPERTY("BaudRates", NapiCreateBaudRatesTypeEnum(env)),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

EXTERN_C_START
/*
 * function for module exports
 */
napi_value SerialInit(napi_env env, napi_value exports)
{
    USB_HILOGD(MODULE_USB_NAPI, "enter");
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("getPortList", SerialGetPortListNapi),
        DECLARE_NAPI_FUNCTION("open", SerialOpenNapi),
        DECLARE_NAPI_FUNCTION("close", SerialCloseNapi),
        DECLARE_NAPI_FUNCTION("read", SerialReadNapi),
        DECLARE_NAPI_FUNCTION("readSync", SerialReadSyncNapi),
        DECLARE_NAPI_FUNCTION("write", SerialWriteNapi),
        DECLARE_NAPI_FUNCTION("writeSync", SerialWriteSyncNapi),
        DECLARE_NAPI_FUNCTION("getAttribute", SerialGetAttributeNapi),
        DECLARE_NAPI_FUNCTION("setAttribute", SerialSetAttributeNapi),
        DECLARE_NAPI_FUNCTION("hasSerialRight", SerialHasRightNapi),
        DECLARE_NAPI_FUNCTION("requestSerialRight", SerialRequestRightNapi),
        DECLARE_NAPI_FUNCTION("addSerialRight", SerialAddRightNapi),
        DECLARE_NAPI_FUNCTION("cancelSerialRight", CancelSerialRightNapi),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    DeclareEnum(env, exports);
    USB_HILOGD(MODULE_USB_NAPI, "return");
    return exports;
}
EXTERN_C_END
