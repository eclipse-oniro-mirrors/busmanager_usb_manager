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

#ifndef USB_CLAIM_DEATH_TEST_H
#define USB_CLAIM_DEATH_TEST_H

#include "usb_claim_test_common.h"

namespace OHOS {
namespace USB {
/*
 * Death auto release tests.
 *
 * PRECONDITION (read before running): the claim_notify_probe binary must be
 * deployed at /data/claim_notify_probe (see the claim_unittest documentation;
 * it is the standalone exclusive-claim holder built from this directory).
 *
 * The first stage holds an exclusive claim inside a child process and exits
 * WITHOUT any cleanup. The second stage (a new process) exclusive claims the
 * same interface again: success proves the service released the claim record
 * when the holder process died. Run order matters: run the stage binary
 * "claim_death_stage1" first, then this suite.
 */
class UsbClaimDeathTest : public UsbClaimTestBase {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
};
} // namespace USB
} // namespace OHOS
#endif // USB_CLAIM_DEATH_TEST_H
