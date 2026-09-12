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

#ifndef USB_CLAIM_MULTI_DEVICE_TEST_H
#define USB_CLAIM_MULTI_DEVICE_TEST_H

#include "usb_claim_test_common.h"

namespace OHOS {
namespace USB {
/*
 * Multi-device exclusive claim tests.
 *
 * PRECONDITION (read before running): at least two non-hub USB devices must
 * be attached, one of which exposes an interface with both a bulk IN and a
 * bulk OUT endpoint (a USB serial adapter like PL2303/CP210x is typical).
 * With a single device the whole suite is skipped.
 *
 * The first bulk-capable device (deviceB_, the serial adapter) is used for
 * the real read/write transfer cases and the interface isolation cases
 * (two interfaces), while another device (deviceA_) is used for the
 * cross-device isolation cases.
 */
class UsbClaimMultiDeviceTest : public UsbClaimTestBase {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

    static bool PickSerialDevice();
    static bool PickSecondDevice();

    static UsbDevice deviceA_; // first non-hub device, cross-device isolation
    static UsbDevice deviceB_; // bulk IN+OUT device, read/write and isolation
    static USBDevicePipe pipeA_;
    static USBDevicePipe pipeB_;
    static UsbInterface ifaceA_;
    static UsbInterface ifaceB_;     // interface with bulk IN+OUT endpoints
    static USBEndpoint bulkInEp_;
    static USBEndpoint bulkOutEp_;
    static uint8_t ifaceIdA_;
    static uint8_t ifaceIdB_;
    static bool multiDeviceReady_;
};
} // namespace USB
} // namespace OHOS
#endif // USB_CLAIM_MULTI_DEVICE_TEST_H
