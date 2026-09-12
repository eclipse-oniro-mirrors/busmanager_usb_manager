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

#include "usb_claim_death_test.h"

#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

#include "hilog_wrapper.h"
#include "usb_errors.h"

using namespace testing::ext;

namespace OHOS {
namespace USB {
namespace {
constexpr int32_t STAGE_CLAIM_WAIT_MS = 3000;
constexpr const char *PROBE_PATH = "/data/claim_notify_probe";

/*
 * Stage1 is the standalone probe binary: it exclusive claims the interface
 * and keeps holding it until it gets killed (before its cleanup path runs),
 * so the claim record can only be cleared by the service side DeathRecipient.
 */
pid_t StartStage1Holder(uint8_t busNum, uint8_t devAddr, uint8_t ifaceId)
{
    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid == 0) {
        char bus[8] = {0};
        char dev[8] = {0};
        char iface[8] = {0};
        (void)snprintf(bus, sizeof(bus), "%u", busNum);
        (void)snprintf(dev, sizeof(dev), "%u", devAddr);
        (void)snprintf(iface, sizeof(iface), "%u", ifaceId);
        // "hold": the probe keeps the exclusive claim until killed, no self
        // conflict claim and no cleanup
        (void)execl(PROBE_PATH, PROBE_PATH, bus, dev, iface, "hold", nullptr);
        _exit(127);
    }
    return pid;
}

/*
 * Give the holder process time to finish the exclusive claim before killing
 * it. The kill happens before the probe's wait-and-cleanup path runs.
 */
bool KillStage1Holder(pid_t pid)
{
    usleep(STAGE_CLAIM_WAIT_MS * 1000);
    if (kill(pid, SIGKILL) != 0) {
        USB_HILOGE(MODULE_USB_SERVICE, "kill stage1 pid=%{public}d failed", pid);
        return false;
    }
    int status = 0;
    waitpid(pid, &status, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "stage1 pid=%{public}d killed", pid);
    return true;
}
} // namespace

void UsbClaimDeathTest::SetUpTestCase()
{
    UsbClaimTestBase::SetUpTestCase();
}

void UsbClaimDeathTest::TearDownTestCase()
{
    UsbClaimTestBase::TearDownTestCase();
}

/**
 * @tc.name: ClaimDeath001
 * @tc.desc: Test that the exclusive claim is auto released when the holder
 *           process dies abnormally
 * @tc.desc: Positive/negative test: stage1 (child process) exclusive claims
 *           the interface, then the same claim from this process is rejected
 *           with UEC_INTERFACE_BUSY while stage1 is alive; after stage1 is
 *           SIGKILLed without cleanup the same claim succeeds, proving the
 *           service cleared the claim record via the DeathRecipient
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimDeathTest, ClaimDeath001, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ASSERT_TRUE(access(PROBE_PATH, X_OK) == 0)
        << "stage1 binary /data/claim_notify_probe missing, deploy it before running this suite";

    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);

    // stage1: holder process claims the interface exclusively
    pid_t holder = StartStage1Holder(busNum_, devAddr_, ifaceId_);
    ASSERT_GT(holder, 0);

    // while stage1 is alive the same exclusive claim from this process
    // (another app) must be rejected
    usleep(STAGE_CLAIM_WAIT_MS * 1000);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);

    // stage1 dies without any cleanup
    ASSERT_TRUE(KillStage1Holder(holder));

    // stage2: this process claims the same interface exclusively again
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimDeath002
 * @tc.desc: Test that normal claim is also possible after the exclusive
 *           holder died abnormally
 * @tc.desc: Positive test: while stage1 holds the exclusive claim a normal
 *           claim from another app succeeds (never blocked); after stage1
 *           dies without cleanup the exclusive claim of that app succeeds,
 *           proving no stale exclusive record is left behind
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimDeathTest, ClaimDeath002, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ASSERT_TRUE(access(PROBE_PATH, X_OK) == 0)
        << "stage1 binary /data/claim_notify_probe missing, deploy it before running this suite";

    auto &client = UsbSrvClient::GetInstance();

    pid_t holder = StartStage1Holder(busNum_, devAddr_, ifaceId_);
    ASSERT_GT(holder, 0);

    // while stage1 is alive a normal claim from another app is not blocked
    SwitchToApp(CLAIM_APP_B);
    usleep(STAGE_CLAIM_WAIT_MS * 1000);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);

    // stage1 dies without any cleanup
    ASSERT_TRUE(KillStage1Holder(holder));

    // release the normal claim first (a normal claim blocks exclusive
    // claims by design), then take over the interface exclusively
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}
} // namespace USB
} // namespace OHOS
