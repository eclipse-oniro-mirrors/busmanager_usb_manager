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

#ifndef USB_CLI_SERIALIZATION_H
#define USB_CLI_SERIALIZATION_H

#include <string>
#include <vector>
#include "cJSON.h"
#include "usb_device.h"
#include "usb_accessory.h"
#include "usb_serial_type.h"

namespace OHOS {
namespace USB {

cJSON* SerializeDevice(UsbDevice &device);
cJSON* SerializeAccessory(const USBAccessory &accessory);
cJSON* SerializeSerialPort(const UsbSerialPort &port);

std::string FormatSuccess(const std::vector<cJSON*> &items);
std::string FormatError(int32_t code, const std::string &message);

}
}

#endif
