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

#ifndef USB_CLAIM_FORCE_TEST_H
#define USB_CLAIM_FORCE_TEST_H

#include "usb_claim_test_common.h"

namespace OHOS {
namespace USB {
class UsbClaimForceTest : public UsbClaimTestBase {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

    /*
     * The force cases need an interface that is held by a kernel driver,
     * which is typical for HID devices (mouse/keyboard, interface class 3
     * bound to usbhid). The helper scans every attached device for such an
     * interface and updates the fixture interface/endpoint members.
     */
    static bool PickHidInterface();

    friend bool PickHidInterfaceWithRetry();
};
} // namespace USB
} // namespace OHOS
#endif // USB_CLAIM_FORCE_TEST_H
