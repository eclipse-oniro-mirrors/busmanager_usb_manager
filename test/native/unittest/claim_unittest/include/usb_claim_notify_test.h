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

#ifndef USB_CLAIM_NOTIFY_TEST_H
#define USB_CLAIM_NOTIFY_TEST_H

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>

#include "accesstoken_kit.h"

#include "usb_claim_test_common.h"

namespace OHOS {
namespace USB {
/*
 * Conflict notification tests.
 *
 * The notification receiving cases impersonate a normal hap application: the
 * device access right is granted through the system management API
 * (AddAccessRight, called with the management token) instead of RequestRight,
 * so no permission dialog is popped up, and the callback registered with the
 * exclusive claim runs as a normal application identity. The conflict
 * counterpart is a separate probe process (/data/claim_notify_probe, "normal"
 * mode: only normal claims), because in-process token switching does not
 * change the IPC caller identity of an established session.
 *
 * PRECONDITION (read before running): the probe binary must be deployed at
 * /data/claim_notify_probe and selinux should be permissive (setenforce 0)
 * for the notification delivery cases.
 */
class UsbClaimNotifyTest : public UsbClaimTestBase {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    static std::function<void(uint8_t, uint8_t, uint8_t)> MakeHapNotifyCallback();
    static bool WaitForHapNotify(int32_t expectedCount, int32_t timeoutMs);
    static void GetHapNotifyParams(uint8_t &busNum, uint8_t &devAddr, uint8_t &interfaceId);
    static void ResetHapNotifyState();

    // hap token impersonating a normal application (notification receiver)
    static Security::AccessToken::AccessTokenID hapToken_;
};
} // namespace USB
} // namespace OHOS
#endif // USB_CLAIM_NOTIFY_TEST_H
