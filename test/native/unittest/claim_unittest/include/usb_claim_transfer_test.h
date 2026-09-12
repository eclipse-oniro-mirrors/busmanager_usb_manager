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

#ifndef USB_CLAIM_TRANSFER_TEST_H
#define USB_CLAIM_TRANSFER_TEST_H

#include "usb_claim_test_common.h"

namespace OHOS {
namespace USB {
class UsbClaimTransferTest : public UsbClaimTestBase {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    /*
     * The transfer cases are direction aware: the read variants run only
     * when the interface descriptor exposes an IN endpoint and the write
     * variants run only when it exposes an OUT endpoint. If a direction is
     * missing the corresponding variant is skipped and treated as success.
     */
    static USBEndpoint inEndpoint_;
    static USBEndpoint outEndpoint_;
    static bool hasInEndpoint_;
    static bool hasOutEndpoint_;
};
} // namespace USB
} // namespace OHOS
#endif // USB_CLAIM_TRANSFER_TEST_H
