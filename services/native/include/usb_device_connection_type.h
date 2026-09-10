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

#ifndef USB_DEVICE_CONNECTION_TYPE_H
#define USB_DEVICE_CONNECTION_TYPE_H

#include <map>
#include <string>

#include "iremote_object.h"
#include "usb_device.h"
#include "iusb_connection_callback.h"

namespace OHOS {
namespace USB {

enum UsbDeviceConnectionType : int32_t {
    USB_DEVICE_CONNECTION_CONNECT = 0,
    USB_DEVICE_CONNECTION_DISCONNECT = 1,
};

struct UsbDeviceConnectionInfo {
    UsbDeviceConnectionType type = USB_DEVICE_CONNECTION_CONNECT;
    UsbDevice device;
    std::string tokenId;
    std::string bundleName;
};

struct DeviceListenerEntry {
    sptr<IUsbConnectionCallback> listener;
    sptr<IRemoteObject::DeathRecipient> deathRecipient;
};

struct AppConnectionInfo {
    std::string bundleName;
};

struct DeviceConnectionRecord {
    UsbDevice device;
    std::map<std::string, AppConnectionInfo> apps;
};

} // namespace USB
} // namespace OHOS

#endif // USB_DEVICE_CONNECTION_TYPE_H