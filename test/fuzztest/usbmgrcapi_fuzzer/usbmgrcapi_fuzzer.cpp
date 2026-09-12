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

#include <cstdint>
#include <cstdlib>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>

#include "ohusb_manager.h"

namespace OHOS {
namespace USB {
namespace {
constexpr size_t THRESHOLD = 12;
constexpr size_t MAX_STRING_LEN = 64;
constexpr uint8_t FUNC_COUNT = 7;

void FuzzPermissionCallback(OH_UsbManager_ErrorCode errorCode, bool result, void *userData)
{
    (void)errorCode;
    (void)result;
    (void)userData;
}

OH_UsbManager_UsbDevice MakeFuzzDevice(FuzzedDataProvider &fdp)
{
    OH_UsbManager_UsbDevice dev {};
    dev.busNum = fdp.ConsumeIntegral<uint8_t>();
    dev.devAddress = fdp.ConsumeIntegral<uint8_t>();
    dev.vendorId = fdp.ConsumeIntegral<uint16_t>();
    dev.productId = fdp.ConsumeIntegral<uint16_t>();
    dev.clazz = fdp.ConsumeIntegral<uint8_t>();
    dev.subClass = fdp.ConsumeIntegral<uint8_t>();
    dev.protocol = fdp.ConsumeIntegral<uint8_t>();
    return dev;
}

OH_UsbManager_UsbPipe MakeFuzzPipe(FuzzedDataProvider &fdp)
{
    OH_UsbManager_UsbPipe pipe {};
    pipe.busNum = fdp.ConsumeIntegral<uint8_t>();
    pipe.devAddress = fdp.ConsumeIntegral<uint8_t>();
    return pipe;
}

void RunGetDeviceList(FuzzedDataProvider &fdp)
{
    OH_UsbManager_UsbDevice *devices = nullptr;
    uint32_t deviceCount = 0;
    if (fdp.ConsumeBool()) {
        (void)OH_UsbManager_GetUsbDeviceList(nullptr, &deviceCount);
        (void)OH_UsbManager_GetUsbDeviceList(&devices, nullptr);
    }
    OH_UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(&devices, &deviceCount);
    if (ret == OH_USBMANAGER_SUCCESS) {
        OH_UsbManager_FreeUsbDeviceList(devices, deviceCount);
    } else {
        OH_UsbManager_FreeUsbDeviceList(devices, 0);
    }
}

void RunFreeDeviceList(FuzzedDataProvider &fdp)
{
    uint32_t count = fdp.ConsumeIntegralInRange<uint32_t>(0, 8);
    OH_UsbManager_UsbDevice *arr = static_cast<OH_UsbManager_UsbDevice *>(
        calloc(count, sizeof(OH_UsbManager_UsbDevice)));
    if (arr == nullptr) {
        OH_UsbManager_FreeUsbDeviceList(nullptr, 0);
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        arr[i].name = nullptr;
        arr[i].manufacturerName = nullptr;
        arr[i].productName = nullptr;
        arr[i].version = nullptr;
        arr[i].configs = nullptr;
        arr[i].configCount = 0;
    }
    OH_UsbManager_FreeUsbDeviceList(arr, count);
    OH_UsbManager_FreeUsbDeviceList(nullptr, count);
}

void RunConnectDevice(FuzzedDataProvider &fdp)
{
    OH_UsbManager_UsbDevice dev = MakeFuzzDevice(fdp);
    OH_UsbManager_UsbPipe pipe {};
    if (fdp.ConsumeBool()) {
        (void)OH_UsbManager_ConnectDevice(nullptr, &pipe);
    }
    if (fdp.ConsumeBool()) {
        (void)OH_UsbManager_ConnectDevice(&dev, nullptr);
    }
    (void)OH_UsbManager_ConnectDevice(&dev, &pipe);
}

void RunHasPermission(FuzzedDataProvider &fdp)
{
    std::string name = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    bool result = false;
    if (fdp.ConsumeBool()) {
        (void)OH_UsbManager_HasPermission(nullptr, &result);
    }
    if (fdp.ConsumeBool()) {
        (void)OH_UsbManager_HasPermission(name.c_str(), nullptr);
    }
    (void)OH_UsbManager_HasPermission(name.c_str(), &result);
}

void RunRequestPermission(FuzzedDataProvider &fdp)
{
    std::string name = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    if (fdp.ConsumeBool()) {
        (void)OH_UsbManager_RequestPermission(nullptr, FuzzPermissionCallback, nullptr);
    }
    if (fdp.ConsumeBool()) {
        (void)OH_UsbManager_RequestPermission(name.c_str(), nullptr, nullptr);
    }
    (void)OH_UsbManager_RequestPermission(name.c_str(), FuzzPermissionCallback, nullptr);
}

void RunGetFileDescriptor(FuzzedDataProvider &fdp)
{
    OH_UsbManager_UsbPipe pipe = MakeFuzzPipe(fdp);
    int32_t fd = -1;
    if (fdp.ConsumeBool()) {
        (void)OH_UsbManager_GetFileDescriptor(nullptr, &fd);
    }
    if (fdp.ConsumeBool()) {
        (void)OH_UsbManager_GetFileDescriptor(&pipe, nullptr);
    }
    (void)OH_UsbManager_GetFileDescriptor(&pipe, &fd);
}

void RunClosePipe(FuzzedDataProvider &fdp)
{
    OH_UsbManager_UsbPipe pipe = MakeFuzzPipe(fdp);
    if (fdp.ConsumeBool()) {
        OH_UsbManager_ClosePipe(nullptr);
    }
    OH_UsbManager_ClosePipe(&pipe);
}
} // namespace

bool UsbMgrCapiFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < THRESHOLD) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    uint8_t selector = fdp.ConsumeIntegral<uint8_t>() % FUNC_COUNT;
    switch (selector) {
        case 0:
            RunGetDeviceList(fdp);
            break;
        case 1:
            RunFreeDeviceList(fdp);
            break;
        case 2:
            RunConnectDevice(fdp);
            break;
        case 3:
            RunHasPermission(fdp);
            break;
        case 4:
            RunRequestPermission(fdp);
            break;
        case 5:
            RunGetFileDescriptor(fdp);
            break;
        case 6:
            RunClosePipe(fdp);
            break;
        default:
            return false;
    }
    return true;
}
} // namespace USB
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::USB::UsbMgrCapiFuzzTest(data, size);
    return 0;
}
