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

#include "usbmgrclaiminterfaceexclusive_fuzzer.h"

#include <functional>

#include "usb_srv_client.h"
#include "usb_errors.h"

namespace OHOS {
const uint32_t OFFSET = 4;
constexpr size_t THRESHOLD = 10;
namespace USB {
    bool UsbMgrClaimInterfaceExclusiveFuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < sizeof(USBDevicePipe) || size < OFFSET + sizeof(UsbInterface)) {
            USB_HILOGE(MODULE_USB_SERVICE, "data size is insufficient!");
            return false;
        }
        auto &usbSrvClient = UsbSrvClient::GetInstance();

        USBDevicePipe pipe;
        auto force = static_cast<bool>(data[0] & 0x1);
        std::function<void(uint8_t, uint8_t, uint8_t)> callback =
            [](uint8_t busNum, uint8_t devAddr, uint8_t interfaceId) {
                USB_HILOGD(MODULE_USB_SERVICE, "exclusive claim conflict bus=%{public}hhu dev=%{public}hhu "
                    "if=%{public}hhu", busNum, devAddr, interfaceId);
            };
        uint8_t *pipeBytes = const_cast<uint8_t *>(data);
        const uint8_t *ifaceBytes = data + OFFSET;
        int32_t ret = usbSrvClient.ClaimInterfaceExclusive(
            reinterpret_cast<USBDevicePipe &>(*pipeBytes),
            reinterpret_cast<const UsbInterface &>(*ifaceBytes),
            force, callback);
        if (ret == UEC_OK) {
            return false;
        }
        return true;
    }
} // USB
} // OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < OHOS::THRESHOLD) {
        return 0;
    }
    /* Run your code on data */
    OHOS::USB::UsbMgrClaimInterfaceExclusiveFuzzTest(data, size);
    return 0;
}
