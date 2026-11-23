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

#include "usbmgropendevicewithmonitor_fuzzer.h"

#include "usb_srv_client.h"
#include "usb_errors.h"
#include "usb_device.h"
#include "usb_device_pipe.h"

namespace OHOS {
const uint32_t OFFSET = 4;
constexpr size_t THRESHOLD = 4;
namespace USB {
    bool UsbMgrOpenDeviceWithMonitorFuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < OFFSET) {
            USB_HILOGE(MODULE_USB_SERVICE, "data size is insufficient!");
            return false;
        }
        auto &usbSrvClient = UsbSrvClient::GetInstance();

        UsbDevice fuzzDevice;
        fuzzDevice.SetBusNum(data[0]);
        fuzzDevice.SetDevAddr(data[1]);
        USBDevicePipe fuzzPipe;
        usbSrvClient.OpenDeviceWithMonitor(fuzzDevice, fuzzPipe);

        std::vector<UsbDevice> devList;
        auto ret = usbSrvClient.GetDevices(devList);
        if (ret != UEC_OK || devList.empty()) {
            USB_HILOGE(MODULE_USB_SERVICE, "get devices failed ret=%{public}d", ret);
            return true;
        }
        UsbDevice device = devList.front();
        usbSrvClient.RequestRight(device.GetName());

        USBDevicePipe pipe;
        ret = usbSrvClient.OpenDeviceWithMonitor(device, pipe);
        if (ret == UEC_OK) {
            USBDevicePipe pipe2;
            usbSrvClient.OpenDeviceWithMonitor(device, pipe2);
            usbSrvClient.Close(pipe);
        }

        if (size >= OFFSET + sizeof(uint8_t) * 2) {
            UsbDevice mixDevice = device;
            mixDevice.SetBusNum(data[OFFSET]);
            mixDevice.SetDevAddr(data[OFFSET + 1]);
            USBDevicePipe mixPipe;
            usbSrvClient.OpenDeviceWithMonitor(mixDevice, mixPipe);
        }

        USBDevicePipe closePipe;
        closePipe.SetBusNum(device.GetBusNum());
        closePipe.SetDevAddr(device.GetDevAddr());
        usbSrvClient.Close(closePipe);
        return true;
    }
} // namespace USB
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < OHOS::THRESHOLD) {
        return 0;
    }
    OHOS::USB::UsbMgrOpenDeviceWithMonitorFuzzTest(data, size);
    return 0;
}
