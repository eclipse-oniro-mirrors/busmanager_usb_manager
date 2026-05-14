/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef USB_SERVICE_IPC_INTERFACE_CODE_H
#define USB_SERVICE_IPC_INTERFACE_CODE_H

/* SAID: 4201 */
namespace OHOS {
namespace USB {
    enum class UsbInterfaceCode {
        USB_FUN_HAS_RIGHT = 0,
        USB_FUN_REQUEST_RIGHT,
        USB_FUN_REMOVE_RIGHT,
        USB_FUN_OPEN_DEVICE,
        USB_FUN_RESET_DEVICE,
        USB_FUN_GET_DEVICE,
        USB_FUN_GET_DEVICES,
        USB_FUN_GET_CURRENT_FUNCTIONS,
        USB_FUN_SET_CURRENT_FUNCTIONS,
        USB_FUN_USB_FUNCTIONS_FROM_STRING,
        USB_FUN_USB_FUNCTIONS_TO_STRING,
        USB_FUN_CLAIM_INTERFACE,
        USB_FUN_RELEASE_INTERFACE,
        USB_FUN_BULK_TRANSFER_READ,
        USB_FUN_BULK_TRANSFER_WRITE,
        USB_FUN_CONTROL_TRANSFER,
        USB_FUN_USB_CONTROL_TRANSFER,
        USB_FUN_SET_ACTIVE_CONFIG,
        USB_FUN_GET_ACTIVE_CONFIG,
        USB_FUN_SET_INTERFACE,
        USB_FUN_GET_PORTS,
        USB_FUN_GET_SUPPORTED_MODES,
        USB_FUN_SET_PORT_ROLE,
        USB_FUN_REQUEST_QUEUE,
        USB_FUN_REQUEST_WAIT,
        USB_FUN_REQUEST_CANCEL,
        USB_FUN_GET_DESCRIPTOR,
        USB_FUN_GET_FILEDESCRIPTOR,
        USB_FUN_CLOSE_DEVICE,
        USB_FUN_SUBMIT_TRANSFER,
        USB_FUN_CANCEL_TRANSFER,
        USB_FUN_BULK_AYSNC_READ,
        USB_FUN_BULK_AYSNC_WRITE,
        USB_FUN_BULK_AYSNC_CANCEL,
        USB_FUN_REG_BULK_CALLBACK,
        USB_FUN_UNREG_BULK_CALLBACK,
        USB_FUN_ADD_RIGHT,
        USB_FUN_DISABLE_GLOBAL_INTERFACE,
        USB_FUN_DISABLE_DEVICE,
        USB_FUN_DISABLE_INTERFACE_TYPE,
        USB_FUN_MANAGE_USB_TYPE,
        USB_FUN_CLEAR_HALT,
        USB_FUN_GET_DEVICE_SPEED,
        USB_FUN_GET_DRIVER_ACTIVE_STATUS,
		USB_FUN_ADD_ACCESS_RIGHT,
        USB_FUN_BULK_TRANSFER_READ_WITH_LENGTH,
        USB_FUN_ATTACH_KERNEL_DRIVER,
        USB_FUN_DETACH_KERNEL_DRIVER,
        USB_FUN_GET_ACCESSORY_LIST,
        USB_FUN_OPEN_ACCESSORY,
        USB_FUN_CLOSE_ACCESSORY,
        USB_FUN_HAS_ACCESSORY_RIGHT,
        USB_FUN_REQUEST_ACCESSORY_RIGHT,
        USB_FUN_REMOVE_ACCESSORY_RIGHT,
        USB_FUN_ADD_ACCESSORY_RIGHT,
        USB_FUN_SERIAL_OPEN,
        USB_FUN_SERIAL_CLOSE,
        USB_FUN_SERIAL_READ,
        USB_FUN_SERIAL_WRITE,
        USB_FUN_SERIAL_GET_ATTRIBUTE,
        USB_FUN_SERIAL_SET_ATTRIBUTE,
        USB_FUN_SERIAL_GET_PORTLIST,
        USB_FUN_HAS_SERIAL_RIGHT,
        USB_FUN_ADD_SERIAL_RIGHT,
        USB_FUN_CANCEL_SERIAL_RIGHT,
        USB_FUN_REQUEST_SERIAL_RIGHT
    };
} // namespace USB
} // namespace OHOS

#endif // USB_SERVICE_IPC_INTERFACE_CODE_H
