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

#include "usb_claim_notify_test.h"

#include <chrono>
#include <cstdio>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "hilog_wrapper.h"
#include "ipc_skeleton.h"
#include "usb_common_test.h"
#include "usb_errors.h"

using namespace testing::ext;
using namespace OHOS::USB::Common;

namespace OHOS {
namespace USB {
namespace {
constexpr int32_t NOTIFY_WAIT_TIMEOUT_MS = 8000;
constexpr const char *PROBE_PATH = "/data/claim_notify_probe";

std::mutex g_hapNotifyMutex;
std::condition_variable g_hapNotifyCv;
int32_t g_hapNotifyCount = 0;
uint8_t g_hapNotifyBus = 0;
uint8_t g_hapNotifyDev = 0;
uint8_t g_hapNotifyIf = 0;

/*
 * The conflict counterpart runs in a separate process with its own native
 * token: the probe in "normal" mode only normal claims the interface, which
 * is never blocked and triggers the conflict notification of the holder.
 */
bool RunProbeNormalClaim(uint8_t busNum, uint8_t devAddr, uint8_t ifaceId, int32_t &probeRet)
{
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        char bus[8] = {0};
        char dev[8] = {0};
        char iface[8] = {0};
        (void)snprintf(bus, sizeof(bus), "%u", busNum);
        (void)snprintf(dev, sizeof(dev), "%u", devAddr);
        (void)snprintf(iface, sizeof(iface), "%u", ifaceId);
        (void)execl(PROBE_PATH, PROBE_PATH, bus, dev, iface, "normal", nullptr);
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) != pid) {
        return false;
    }
    if (!WIFEXITED(status)) {
        return false;
    }
    probeRet = WEXITSTATUS(status);
    return true;
}
} // namespace

Security::AccessToken::AccessTokenID UsbClaimNotifyTest::hapToken_ = 0;

std::function<void(uint8_t, uint8_t, uint8_t)> UsbClaimNotifyTest::MakeHapNotifyCallback()
{
    return [](uint8_t busNum, uint8_t devAddr, uint8_t interfaceId) {
        USB_HILOGI(MODULE_USB_SERVICE, "notify received bus=%{public}hhu dev=%{public}hhu if=%{public}hhu",
            busNum, devAddr, interfaceId);
        std::lock_guard<std::mutex> lock(g_hapNotifyMutex);
        g_hapNotifyCount++;
        g_hapNotifyBus = busNum;
        g_hapNotifyDev = devAddr;
        g_hapNotifyIf = interfaceId;
        g_hapNotifyCv.notify_all();
    };
}

bool UsbClaimNotifyTest::WaitForHapNotify(int32_t expectedCount, int32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(g_hapNotifyMutex);
    g_hapNotifyCv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
        [expectedCount]() { return g_hapNotifyCount >= expectedCount; });
    return g_hapNotifyCount >= expectedCount;
}

void UsbClaimNotifyTest::GetHapNotifyParams(uint8_t &busNum, uint8_t &devAddr, uint8_t &interfaceId)
{
    std::lock_guard<std::mutex> lock(g_hapNotifyMutex);
    busNum = g_hapNotifyBus;
    devAddr = g_hapNotifyDev;
    interfaceId = g_hapNotifyIf;
}

void UsbClaimNotifyTest::ResetHapNotifyState()
{
    std::lock_guard<std::mutex> lock(g_hapNotifyMutex);
    g_hapNotifyCount = 0;
    g_hapNotifyBus = 0;
    g_hapNotifyDev = 0;
    g_hapNotifyIf = 0;
}

namespace {
/*
 * The test process needs an IPC work loop so that the asynchronous
 * CLAIM_CONFLICT_NOTIFY requests sent by the service can be dispatched to
 * the ClaimExclusiveMonitor stub created inside UsbSrvClient.
 */
void StartIpcWorkLoop()
{
    static bool ipcLoopStarted = false;
    if (ipcLoopStarted) {
        return;
    }
    ipcLoopStarted = true;
    std::thread ipcThread([]() { OHOS::IPCSkeleton::JoinWorkThread(); });
    ipcThread.detach();
    USB_HILOGI(MODULE_USB_SERVICE, "ipc work loop thread started");
}
} // namespace

void UsbClaimNotifyTest::SetUpTestCase()
{
    UsbClaimTestBase::SetUpTestCase();
    StartIpcWorkLoop();

    // hap token impersonating a normal application (the notification
    // receiver); its device access right is granted through the system
    // management API so no permission dialog is popped up
    hapToken_ = UsbCommonTest::AllocHapTest();
    ASSERT_NE(0U, hapToken_);
    SwitchToApp(CLAIM_APP_SYS);
    auto &client = UsbSrvClient::GetInstance();
    auto ret = client.AddAccessRight(std::to_string(hapToken_), device_.GetName());
    EXPECT_EQ(UEC_OK, ret);
}

void UsbClaimNotifyTest::TearDownTestCase()
{
    UsbClaimTestBase::TearDownTestCase();
}

/**
 * @tc.name: ClaimNotify001
 * @tc.desc: Test the conflict notification delivery to a normal application
 * @tc.desc: Positive test: the hap application exclusive claims with a
 *           callback, a separate process normal claims the same interface,
 *           and the callback is invoked with matching parameters
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimNotifyTest, ClaimNotify001, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ASSERT_TRUE(access(PROBE_PATH, X_OK) == 0)
        << "probe binary /data/claim_notify_probe missing, deploy it before running this suite";
    ResetHapNotifyState();

    auto &client = UsbSrvClient::GetInstance();
    UsbCommonTest::SetSelfToken(hapToken_);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, MakeHapNotifyCallback());
    EXPECT_EQ(UEC_OK, ret);

    // the probe process normal claims the same interface as another app
    int32_t probeRet = -1;
    ASSERT_TRUE(RunProbeNormalClaim(busNum_, devAddr_, ifaceId_, probeRet));
    EXPECT_EQ(0, probeRet);

    // the notification must have been delivered to the hap callback
    ASSERT_TRUE(WaitForHapNotify(1, NOTIFY_WAIT_TIMEOUT_MS));
    uint8_t bus = 0;
    uint8_t dev = 0;
    uint8_t iface = 0;
    GetHapNotifyParams(bus, dev, iface);
    EXPECT_EQ(busNum_, bus);
    EXPECT_EQ(devAddr_, dev);
    EXPECT_EQ(ifaceId_, iface);

    UsbCommonTest::SetSelfToken(hapToken_);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimNotify002
 * @tc.desc: Test that no notification is delivered for a same-process claim
 * @tc.desc: Positive test: the hap owner normal claims its own interface
 *           (the established session keeps the same caller identity), no
 *           notification is expected for the same application
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimNotifyTest, ClaimNotify002, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ResetHapNotifyState();

    auto &client = UsbSrvClient::GetInstance();
    UsbCommonTest::SetSelfToken(hapToken_);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, MakeHapNotifyCallback());
    EXPECT_EQ(UEC_OK, ret);
    // same-app normal claim: no notification for the owner itself
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    usleep(500 * 1000);
    {
        std::lock_guard<std::mutex> lock(g_hapNotifyMutex);
        EXPECT_EQ(0, g_hapNotifyCount);
    }

    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimNotify003
 * @tc.desc: Test a normal claim without any exclusive claim
 * @tc.desc: Positive test: the claim succeeds and no notification is needed
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimNotifyTest, ClaimNotify003, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ResetHapNotifyState();

    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_B);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    usleep(500 * 1000);
    {
        std::lock_guard<std::mutex> lock(g_hapNotifyMutex);
        EXPECT_EQ(0, g_hapNotifyCount);
    }
}

/**
 * @tc.name: ClaimNotify004
 * @tc.desc: Test a normal claim after the exclusive claim is released
 * @tc.desc: Positive test: the claim succeeds and no notification is sent
 *           because the exclusive claim was already released
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimNotifyTest, ClaimNotify004, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ResetHapNotifyState();

    auto &client = UsbSrvClient::GetInstance();
    UsbCommonTest::SetSelfToken(hapToken_);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, MakeHapNotifyCallback());
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);

    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    usleep(500 * 1000);
    {
        std::lock_guard<std::mutex> lock(g_hapNotifyMutex);
        EXPECT_EQ(0, g_hapNotifyCount);
    }
}

/**
 * @tc.name: ClaimNotify005
 * @tc.desc: Test repeated conflict notification after normal claim release
 * @tc.desc: Positive test: two probe conflict claims trigger two
 *           notifications for the hap owner
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimNotifyTest, ClaimNotify005, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ASSERT_TRUE(access(PROBE_PATH, X_OK) == 0)
        << "probe binary /data/claim_notify_probe missing, deploy it before running this suite";
    ResetHapNotifyState();

    auto &client = UsbSrvClient::GetInstance();
    UsbCommonTest::SetSelfToken(hapToken_);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, MakeHapNotifyCallback());
    EXPECT_EQ(UEC_OK, ret);

    int32_t probeRet = -1;
    ASSERT_TRUE(RunProbeNormalClaim(busNum_, devAddr_, ifaceId_, probeRet));
    EXPECT_EQ(0, probeRet);
    ASSERT_TRUE(WaitForHapNotify(1, NOTIFY_WAIT_TIMEOUT_MS));
    ASSERT_TRUE(RunProbeNormalClaim(busNum_, devAddr_, ifaceId_, probeRet));
    EXPECT_EQ(0, probeRet);
    ASSERT_TRUE(WaitForHapNotify(2, NOTIFY_WAIT_TIMEOUT_MS));

    UsbCommonTest::SetSelfToken(hapToken_);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimNotify006
 * @tc.desc: Test the callback update path on repeated exclusive claim
 * @tc.desc: Positive test: the hap owner re-claims with a new callback and
 *           the notification triggered afterwards is received by the
 *           updated callback
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimNotifyTest, ClaimNotify006, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ASSERT_TRUE(access(PROBE_PATH, X_OK) == 0)
        << "probe binary /data/claim_notify_probe missing, deploy it before running this suite";
    ResetHapNotifyState();

    auto &client = UsbSrvClient::GetInstance();
    UsbCommonTest::SetSelfToken(hapToken_);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, MakeHapNotifyCallback());
    EXPECT_EQ(UEC_OK, ret);
    // repeated exclusive claim updates the callback registration
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, MakeHapNotifyCallback());
    EXPECT_EQ(UEC_OK, ret);

    int32_t probeRet = -1;
    ASSERT_TRUE(RunProbeNormalClaim(busNum_, devAddr_, ifaceId_, probeRet));
    EXPECT_EQ(0, probeRet);
    ASSERT_TRUE(WaitForHapNotify(1, NOTIFY_WAIT_TIMEOUT_MS));

    UsbCommonTest::SetSelfToken(hapToken_);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimNotify007
 * @tc.desc: Test that the notification is fire-and-forget
 * @tc.desc: Positive test: the normal claim result of the probe process is
 *           not affected by the asynchronous notification
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimNotifyTest, ClaimNotify007, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ASSERT_TRUE(access(PROBE_PATH, X_OK) == 0)
        << "probe binary /data/claim_notify_probe missing, deploy it before running this suite";
    ResetHapNotifyState();

    auto &client = UsbSrvClient::GetInstance();
    UsbCommonTest::SetSelfToken(hapToken_);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, MakeHapNotifyCallback());
    EXPECT_EQ(UEC_OK, ret);

    // the probe normal claim succeeds synchronously, the notification
    // arrives asynchronously afterwards
    int32_t probeRet = -1;
    ASSERT_TRUE(RunProbeNormalClaim(busNum_, devAddr_, ifaceId_, probeRet));
    EXPECT_EQ(0, probeRet);
    ASSERT_TRUE(WaitForHapNotify(1, NOTIFY_WAIT_TIMEOUT_MS));

    UsbCommonTest::SetSelfToken(hapToken_);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
}
} // namespace USB
} // namespace OHOS
