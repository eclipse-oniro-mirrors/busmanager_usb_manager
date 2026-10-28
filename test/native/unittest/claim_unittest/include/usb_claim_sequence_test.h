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

#ifndef USB_CLAIM_SEQUENCE_TEST_H
#define USB_CLAIM_SEQUENCE_TEST_H

#include "usb_claim_test_common.h"

namespace OHOS {
namespace USB {

/*
 * Fixture for claim state-machine sequence tests.
 *
 * While the other claim suites focus on one API at a time (exclusive claim,
 * release, transfer, notify, death or hotplug), this suite composes multiple
 * operations into longer sequences and verifies the resulting claim state
 * after every step. Each step asserts an intermediate expectation so a
 * failure points at the exact transition that broke, and every case ends by
 * walking the interface back to the idle baseline through CleanClaimState.
 *
 * The semantics exercised here are documented inline per case; in short the
 * service keeps two orthogonal flags per interface:
 *   - an exclusive owner token (recorded per calling token id), and
 *   - a shared normal claim flag (no owner recorded).
 * Exclusive claims are rejected with UEC_INTERFACE_BUSY whenever another app
 * holds the exclusive claim or when the normal claim flag is set. Normal
 * claims are never blocked. ReleaseInterface clears only the caller owned
 * state, and Close clears only the caller exclusive record while dropping the
 * HDI device handle.
 */
class UsbClaimSequenceTest : public UsbClaimTestBase {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
};

} // namespace USB
} // namespace OHOS
#endif // USB_CLAIM_SEQUENCE_TEST_H