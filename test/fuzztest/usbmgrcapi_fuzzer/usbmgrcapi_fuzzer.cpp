/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "usbmgrcapi_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ohusb_manager.h"

namespace OHOS {
namespace USB {

constexpr size_t THRESHOLD = 4;
constexpr size_t MAX_DEVICE_NAME_LEN = 64;
constexpr uint32_t PERMISSION_REQUEST_LIMIT = 64;

/*
 * The first byte selects an operation and the remaining bytes are used as the
 * fuzzed payload. Each case covers either the normal path or the null/invalid
 * parameter path of one C API, giving 12 entry points in total that exercise all
 * seven interfaces exported by libohusb_manager.so.
 */
enum class FuncId : uint8_t {
    GET_DEVICE_LIST = 0,           // OH_UsbManager_GetUsbDeviceList normal path + FreeUsbDeviceList
    GET_DEVICE_LIST_NULL,          // OH_UsbManager_GetUsbDeviceList null parameter paths
    CONNECT_DEVICE,                // OH_UsbManager_ConnectDevice normal path
    CONNECT_DEVICE_NULL,           // OH_UsbManager_ConnectDevice null parameter paths
    HAS_PERMISSION,                // OH_UsbManager_HasPermission with a fuzzed device name
    HAS_PERMISSION_NULL,           // OH_UsbManager_HasPermission null parameter paths
    REQUEST_PERMISSION,            // OH_UsbManager_RequestPermission normal path (rate-limited)
    REQUEST_PERMISSION_NULL,       // OH_UsbManager_RequestPermission null parameter paths
    GET_FILE_DESCRIPTOR,           // OH_UsbManager_GetFileDescriptor with a fuzzed pipe
    GET_FILE_DESCRIPTOR_NULL,      // OH_UsbManager_GetFileDescriptor null parameter paths
    CLOSE_PIPE,                    // OH_UsbManager_ClosePipe with a fuzzed pipe
    CLOSE_PIPE_NULL,               // OH_UsbManager_ClosePipe null parameter path
    FUNC_ID_COUNT,
};

// Builds a NUL-terminated device name from fuzzed bytes, bounded by outSize.
void BuildDeviceName(const uint8_t *data, size_t size, char *out, size_t outSize)
{
    if (out == nullptr || outSize == 0) {
        return;
    }
    size_t len = (size < outSize - 1) ? size : (outSize - 1);
    for (size_t i = 0; i < len; ++i) {
        out[i] = static_cast<char>(data[i]);
    }
    out[len] = '\0';
}

// Zero-fills then interprets fuzzed bytes as an OH_UsbManager_UsbPipe.
OH_UsbManager_UsbPipe BuildPipe(const uint8_t *data, size_t size)
{
    OH_UsbManager_UsbPipe pipe;
    memset(&pipe, 0, sizeof(pipe));
    size_t copyLen = (size < sizeof(pipe)) ? size : sizeof(pipe);
    if (copyLen > 0) {
        memcpy(&pipe, data, copyLen);
    }
    return pipe;
}

// Zero-fills then interprets fuzzed bytes as an OH_UsbManager_UsbDevice. Only busNum
// and devAddress are meaningful to OH_UsbManager_ConnectDevice.
OH_UsbManager_UsbDevice BuildDevice(const uint8_t *data, size_t size)
{
    OH_UsbManager_UsbDevice device;
    memset(&device, 0, sizeof(device));
    if (size >= 1) {
        device.busNum = data[0];
    }
    if (size >= 2) {
        device.devAddress = data[1];
    }
    return device;
}

// No-op callback for OH_UsbManager_RequestPermission. Unnamed parameters avoid
// unused-parameter warnings.
void PermissionCallback(OH_UsbManager_ErrorCode /* errorCode */, bool /* result */, void * /* userContext */)
{
}

// Caps the total number of detached threads spawned by OH_UsbManager_RequestPermission
// to avoid exhausting thread resources across the whole fuzz run.
static uint32_t g_permRequestCount = 0;

// Normal path: enumerate devices then free the returned list, covering the deep copy
// and release logic inside libohusb_manager.
bool TestGetDeviceList()
{
    OH_UsbManager_UsbDevice *devices = nullptr;
    uint32_t deviceCount = 0;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(&devices, &deviceCount);
    if (ret == OH_USBMANAGER_SUCCESS && devices != nullptr && deviceCount > 0) {
        OH_UsbManager_FreeUsbDeviceList(devices, deviceCount);
    }
    return true;
}

// Invalid parameter paths: a null devices or deviceCount must yield
// OH_USBMANAGER_ERROR_INVALID_PARAMETER.
bool TestGetDeviceListNull()
{
    uint32_t deviceCount = 0;
    OH_UsbManager_UsbDevice *devices = nullptr;

    (void)OH_UsbManager_GetUsbDeviceList(nullptr, nullptr);
    (void)OH_UsbManager_GetUsbDeviceList(nullptr, &deviceCount);
    (void)OH_UsbManager_GetUsbDeviceList(&devices, nullptr);
    return true;
}

// Normal path: connect using fuzzed busNum/devAddress, then fetch the fd and close
// the pipe when the connection succeeds.
bool TestConnectDevice(const uint8_t *data, size_t size)
{
    OH_UsbManager_UsbDevice device = BuildDevice(data, size);
    OH_UsbManager_UsbPipe pipe = {};
    OH_UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    if (ret == OH_USBMANAGER_SUCCESS) {
        int32_t fd = -1;
        (void)OH_UsbManager_GetFileDescriptor(&pipe, &fd);
        (void)OH_UsbManager_ClosePipe(&pipe);
    }
    return true;
}

// Invalid parameter paths: a null device or pipe must yield
// OH_USBMANAGER_ERROR_INVALID_PARAMETER.
bool TestConnectDeviceNull()
{
    OH_UsbManager_UsbPipe pipe = {};
    OH_UsbManager_UsbDevice device = {};
    (void)OH_UsbManager_ConnectDevice(nullptr, &pipe);
    (void)OH_UsbManager_ConnectDevice(&device, nullptr);
    (void)OH_UsbManager_ConnectDevice(nullptr, nullptr);
    return true;
}

// Normal path: query permission with a fuzzed device name; the result pointer is
// always valid.
bool TestHasPermission(const uint8_t *data, size_t size)
{
    char deviceName[MAX_DEVICE_NAME_LEN] = { 0 };
    BuildDeviceName(data, size, deviceName, sizeof(deviceName));
    bool result = false;
    (void)OH_UsbManager_HasPermission(deviceName, &result);
    return true;
}

// Invalid parameter paths: a null deviceName or result must yield
// OH_USBMANAGER_ERROR_INVALID_PARAMETER.
bool TestHasPermissionNull()
{
    bool result = false;
    (void)OH_UsbManager_HasPermission(nullptr, &result);
    (void)OH_UsbManager_HasPermission("1-1", nullptr);
    (void)OH_UsbManager_HasPermission(nullptr, nullptr);
    return true;
}

// Normal path: request device permission asynchronously, rate-limited to bound the
// number of detached worker threads.
bool TestRequestPermission(const uint8_t *data, size_t size)
{
    if (g_permRequestCount >= PERMISSION_REQUEST_LIMIT) {
        return true;
    }
    ++g_permRequestCount;
    char deviceName[MAX_DEVICE_NAME_LEN] = { 0 };
    BuildDeviceName(data, size, deviceName, sizeof(deviceName));
    (void)OH_UsbManager_RequestPermission(deviceName, PermissionCallback, nullptr);
    return true;
}

// Invalid parameter paths: a null deviceName or callback must yield
// OH_USBMANAGER_ERROR_INVALID_PARAMETER.
bool TestRequestPermissionNull()
{
    (void)OH_UsbManager_RequestPermission(nullptr, PermissionCallback, nullptr);
    (void)OH_UsbManager_RequestPermission("1-1", nullptr, nullptr);
    (void)OH_UsbManager_RequestPermission(nullptr, nullptr, nullptr);
    return true;
}

// Normal path: get a file descriptor using a fuzzed pipe.
bool TestGetFileDescriptor(const uint8_t *data, size_t size)
{
    OH_UsbManager_UsbPipe pipe = BuildPipe(data, size);
    int32_t fd = -1;
    (void)OH_UsbManager_GetFileDescriptor(&pipe, &fd);
    return true;
}

// Invalid parameter paths: a null pipe or fd must yield
// OH_USBMANAGER_ERROR_INVALID_PARAMETER.
bool TestGetFileDescriptorNull()
{
    int32_t fd = -1;
    OH_UsbManager_UsbPipe pipe = {};
    (void)OH_UsbManager_GetFileDescriptor(nullptr, &fd);
    (void)OH_UsbManager_GetFileDescriptor(&pipe, nullptr);
    (void)OH_UsbManager_GetFileDescriptor(nullptr, nullptr);
    return true;
}

// Normal path: close a fuzzed pipe.
bool TestClosePipe(const uint8_t *data, size_t size)
{
    OH_UsbManager_UsbPipe pipe = BuildPipe(data, size);
    (void)OH_UsbManager_ClosePipe(&pipe);
    return true;
}

// Invalid parameter path: a null pipe must yield OH_USBMANAGER_ERROR_INVALID_PARAMETER.
bool TestClosePipeNull()
{
    (void)OH_UsbManager_ClosePipe(nullptr);
    return true;
}

// C API fuzz entry: dispatch on the first byte to one of the 12 cases above.
bool UsbMgrCapiFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < 1) {
        return false;
    }
    const uint8_t *payload = data + 1;
    size_t payloadSize = size - 1;
    FuncId funcId = static_cast<FuncId>(data[0] % static_cast<uint8_t>(FuncId::FUNC_ID_COUNT));
    switch (funcId) {
        case FuncId::GET_DEVICE_LIST:
            return TestGetDeviceList();
        case FuncId::GET_DEVICE_LIST_NULL:
            return TestGetDeviceListNull();
        case FuncId::CONNECT_DEVICE:
            return TestConnectDevice(payload, payloadSize);
        case FuncId::CONNECT_DEVICE_NULL:
            return TestConnectDeviceNull();
        case FuncId::HAS_PERMISSION:
            return TestHasPermission(payload, payloadSize);
        case FuncId::HAS_PERMISSION_NULL:
            return TestHasPermissionNull();
        case FuncId::REQUEST_PERMISSION:
            return TestRequestPermission(payload, payloadSize);
        case FuncId::REQUEST_PERMISSION_NULL:
            return TestRequestPermissionNull();
        case FuncId::GET_FILE_DESCRIPTOR:
            return TestGetFileDescriptor(payload, payloadSize);
        case FuncId::GET_FILE_DESCRIPTOR_NULL:
            return TestGetFileDescriptorNull();
        case FuncId::CLOSE_PIPE:
            return TestClosePipe(payload, payloadSize);
        case FuncId::CLOSE_PIPE_NULL:
            return TestClosePipeNull();
        default:
            return false;
    }
}

} // namespace USB
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < OHOS::USB::THRESHOLD) {
        return 0;
    }
    (void)OHOS::USB::UsbMgrCapiFuzzTest(data, size);
    return 0;
}