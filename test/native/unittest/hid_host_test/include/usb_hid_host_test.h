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

#ifndef USB_HID_HOST_TEST_H
#define USB_HID_HOST_TEST_H

#include <gtest/gtest.h>

#include "usb_device.h"
#include "usb_device_pipe.h"
#include "usb_endpoint.h"
#include "usb_interface.h"

namespace OHOS {
namespace USB {
namespace HidHost {

/*
 * USB HID (Human Interface Device) host test.
 *
 * Target environment:
 *   - One RK3568 board running in USB host mode (a single board is sufficient).
 *   - A USB keyboard and/or a USB mouse plugged into the board's host port.
 *
 * The test exercises the full host-side lifecycle against a real HID device:
 * enumeration, HID class/protocol identification (keyboard vs. mouse),
 * permission checks, open/close lifecycle, interface claim/release, descriptor
 * retrieval, HID class control transfers and interrupt-IN transfers.
 *
 * HID identification note: a standalone keyboard/mouse reports bDeviceClass =
 * 0x00 at the device level. The HID class (0x03) and the boot protocol
 * (keyboard = 0x01, mouse = 0x02) are reported at the interface level, so the
 * identification helpers scan interfaces first and fall back to the device
 * level.
 */

/*
 * USB class codes used to recognize keyboard / mouse / hub.
 */
static constexpr int32_t USB_CLASS_HID = 0x03;

/*
 * HID boot protocol identifiers.
 */
static constexpr int32_t HID_PROTOCOL_KEYBOARD = 0x01;
static constexpr int32_t HID_PROTOCOL_MOUSE = 0x02;
static constexpr int32_t HID_PROTOCOL_INVALID = -1;

/*
 * USB standard request bmRequestType / bRequest constants.
 */
static constexpr int32_t REQUEST_TYPE_STANDARD_IN = 0x80;
static constexpr int32_t REQUEST_TYPE_CLASS_INTERFACE_IN = 0xA1;
static constexpr int32_t REQUEST_TYPE_CLASS_INTERFACE_OUT = 0x21;
static constexpr int32_t USB_REQUEST_GET_DESCRIPTOR = 0x06;

/*
 * USB descriptor type constants.
 */
static constexpr int32_t USB_DT_DEVICE = 0x01;
static constexpr int32_t USB_DT_CONFIG = 0x02;
static constexpr int32_t USB_DT_STRING = 0x03;
static constexpr int32_t USB_DT_HID = 0x21;

/*
 * HID class specific request codes.
 */
static constexpr int32_t HID_GET_REPORT = 0x01;
static constexpr int32_t HID_GET_IDLE = 0x02;
static constexpr int32_t HID_GET_PROTOCOL = 0x03;
static constexpr int32_t HID_SET_REPORT = 0x09;
static constexpr int32_t HID_SET_IDLE = 0x0A;
static constexpr int32_t HID_SET_PROTOCOL = 0x0B;

/*
 * Test configuration constants.
 */
static constexpr int32_t DEVICE_DESC_LENGTH = 18;
static constexpr int32_t INTERRUPT_BUFFER_LENGTH = 8;
static constexpr int32_t CONTROL_TRANSFER_TIMEOUT = 2000;
static constexpr int32_t INTERRUPT_TRANSFER_TIMEOUT = 2000;

/*
 * The test fixture. It caches the enumeration result once in SetUpTestCase so
 * every test case can look the keyboard / mouse up without touching the
 * hardware repeatedly.
 *
 * The traversal helpers take a non-const UsbDevice& on purpose: the model's
 * GetConfigs() / GetInterfaces() / GetEndpoints() accessors are non-const.
 */
class UsbHidHostTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    /*
     * Return the HID boot protocol of the first HID interface (keyboard = 1,
     * mouse = 2), falling back to the device-level protocol. Returns
     * HID_PROTOCOL_INVALID when the device is not a boot HID device.
     */
    static int32_t GetHidProtocol(UsbDevice &device);

    static bool IsHidDevice(UsbDevice &device);
    static bool IsKeyboard(UsbDevice &device);
    static bool IsMouse(UsbDevice &device);
    static bool FindHidDevice(UsbDevice &device);
    static bool FindKeyboardDevice(UsbDevice &device);
    static bool FindMouseDevice(UsbDevice &device);

    /*
     * Locate the first interrupt-IN endpoint on the device and return the
     * owning interface and endpoint via output parameters.
     */
    static bool FindInterruptInEndpoint(UsbDevice &device, UsbInterface &interface, USBEndpoint &endpoint);
};

} // namespace HidHost
} // namespace USB
} // namespace OHOS
#endif // USB_HID_HOST_TEST_H