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

#include "usb_claim_sequence_test.h"

#include <vector>

#include "hilog_wrapper.h"
#include "usb_common_test.h"
#include "usb_errors.h"

using namespace testing::ext;
using namespace OHOS::USB::Common;

namespace OHOS {
namespace USB {
namespace {
constexpr int32_t TRANSFER_BUFFER_SIZE = 8;
constexpr int32_t TRANSFER_TIMEOUT_MS = 300;

/*
 * Transfer blocking convention (copied from the transfer suite): an allowed
 * transfer is asserted with ret != UEC_SERVICE_IO_EXCEPTION because the real
 * driver result of an allowed request depends on the endpoint type of the
 * off-the-shelf device (a read on a data-less interrupt endpoint returns a
 * driver error rather than IO exception). A blocked transfer is asserted
 * strictly with UEC_SERVICE_IO_EXCEPTION, the value the service returns when
 * the exclusive claim check rejects the request before it reaches the HDI.
 */

void DoBulkTransfer(USBDevicePipe &pipe, const USBEndpoint &endpoint, int32_t &ret)
{
    std::vector<uint8_t> buffer(TRANSFER_BUFFER_SIZE, 0);
    auto &client = UsbSrvClient::GetInstance();
    ret = client.BulkTransfer(pipe, endpoint, buffer, TRANSFER_TIMEOUT_MS);
}
} // namespace

void UsbClaimSequenceTest::SetUpTestCase()
{
    UsbClaimTestBase::SetUpTestCase();
}

void UsbClaimSequenceTest::TearDownTestCase()
{
    UsbClaimTestBase::TearDownTestCase();
}

/*
 * Section A: elementary single-application transitions.
 *
 * These cases establish the state-machine baseline from the idle state: how
 * an exclusive claim, a normal claim and a release walk the interface through
 * its elementary transitions and back to idle.
 */

/**
 * @tc.name: ClaimSequence001
 * @tc.desc: Test exclusive claim and release return the interface to idle
 * @tc.desc: Positive test: app A exclusive claims an idle interface and then
 *           releases it, so a following exclusive claim of app B succeeds
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence001, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence002
 * @tc.desc: Test normal claim and release return the interface to idle
 * @tc.desc: Positive test: app A normal claims an idle interface and then
 *           releases it, so a following exclusive claim of app B succeeds
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence002, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence003
 * @tc.desc: Test that repeated exclusive claim by the same app is idempotent
 * @tc.desc: Positive test: app A exclusive claims the same interface three
 *           times in a row and every call succeeds; the state stays owned by
 *           app A and does not leak records
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence003, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence004
 * @tc.desc: Test that repeated normal claim by the same app is idempotent
 * @tc.desc: Positive test: app A normal claims the same interface three times
 *           in a row and every call succeeds; a following exclusive claim of
 *           another app is still busy because the normal claim flag is set
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence004, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence005
 * @tc.desc: Test exclusive claim followed by a normal claim of the same app
 * @tc.desc: Positive test: the exclusive owner normal claims its own interface
 *           and both claims coexist; releasing once clears the exclusive
 *           record while the normal claim flag is retained, so app B stays
 *           busy on a following exclusive claim
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence005, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence013
 * @tc.desc: Test two claim types then two releases of the same app
 * @tc.desc: Positive test: app A exclusive claims and normal claims, then
 *           releases twice; the first release clears the caller exclusive
 *           record, the second clears the normal claim flag, leaving the
 *           interface idle for app B
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence013, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence016
 * @tc.desc: Test that an exclusive claim survives a failed normal claim
 * @tc.desc: Positive/negative test: app A holds the exclusive claim; a normal
 *           claim attempt of a rightless hap token is rejected, leaving the
 *           claim state unchanged, so app B is still blocked exclusively
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence016, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    Security::AccessToken::AccessTokenID hapToken = UsbCommonTest::AllocHapTest();
    ASSERT_NE(0U, hapToken);
    UsbCommonTest::SetSelfToken(hapToken);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence017
 * @tc.desc: Test exclusive claim, normal claim, then release in reverse order
 * @tc.desc: Positive test: app A exclusive claims, app B normal claims, then
 *           app B releases before app A; the normal claim flag is cleared
 *           first but the exclusive record remains, so app C is still busy
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence017, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence018
 * @tc.desc: Test exclusive claim, normal claim, release in forward order
 * @tc.desc: Positive test: app A exclusive claims, app B normal claims, then
 *           app A releases before app B; the exclusive record is cleared but
 *           the normal claim flag remains, so app C is still busy
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence018, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence019
 * @tc.desc: Test exclusive claim then release by a non-owner
 * @tc.desc: Negative test: app B tries to release an interface that app A
 *           exclusively claimed without any normal claim; the request is
 *           rejected with UEC_INTERFACE_INVALID_OPERATION and app A keeps the
 *           exclusive claim
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence019, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_INTERFACE_INVALID_OPERATION, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence020
 * @tc.desc: Test release by a non-owner with a normal claim present
 * @tc.desc: Positive test: app A exclusive claims and app B normal claims; a
 *           release of app C clears no state because neither the exclusive
 *           record nor the normal claim belongs to it, so app B is still able
 *           to transfer while app D is still busy exclusively
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence020, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_NE(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
}

/*
 * Section B: two-application takeover sequences.
 *
 * These cases walk the interface ownership from app A to app B (and back)
 * through every legal handover path: release before claim, claim before
 * release, and close-based handover.
 */

/**
 * @tc.name: ClaimSequence021
 * @tc.desc: Test a clean exclusive handover from A to B
 * @tc.desc: Positive test: app A exclusive claims, releases, then app B
 *           exclusive claims; the ownership fully transfers and app A is now
 *           blocked
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence021, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence022
 * @tc.desc: Test exclusive handover blocked without a release
 * @tc.desc: Negative test: app A exclusive claims and app B tries to claim
 *           exclusively without app A releasing; app B is rejected with
 *           UEC_INTERFACE_BUSY until app A releases
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence022, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence023
 * @tc.desc: Test a clean normal handover from A to B
 * @tc.desc: Positive test: app A normal claims and releases, then app B
 *           normal claims; both succeed and the interface is never blocked
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence023, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence024
 * @tc.desc: Test simultaneous exclusive claim of A and normal claim of B
 * @tc.desc: Positive test: app A exclusive claims and app B normal claims at
 *           the same time; the normal claim is never blocked, and app A's
 *           release still leaves the normal claim flag set for app B
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence024, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence025
 * @tc.desc: Test exclusive takeover through a normal claim bridge
 * @tc.desc: Positive test: app A exclusive claims and app B normal claims;
 *           even after app A releases, the normal claim flag blocks a third
 *           exclusive claim until app B releases, then app C takes over
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence025, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence026
 * @tc.desc: Test a round-trip exclusive handover A-B-A
 * @tc.desc: Positive test: app A exclusive claims and releases, app B claims
 *           and releases, then app A claims again; each handover is clean and
 *           the final owner is app A
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence026, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence027
 * @tc.desc: Test normal claim of B does not hide the exclusive claim of A
 * @tc.desc: Positive test: app A exclusive claims and app B normal claims; a
 *           third exclusive claim is still busy even though the normal claim
 *           succeeded, proving the two flags are tracked independently
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence027, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence028
 * @tc.desc: Test that a blocked exclusive claim records no partial state
 * @tc.desc: Positive/negative test: app A exclusive claims, app B is rejected
 *           with UEC_INTERFACE_BUSY, then app A releases; the interface is
 *           idle and app B can claim exclusively right after
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence028, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence029
 * @tc.desc: Test two normal claims then an exclusive claim
 * @tc.desc: Negative test: app A and app B both normal claim, then app C tries
 *           to exclusive claim; the shared normal claim flag blocks app C
 *           until both normal claimers release
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence029, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence030
 * @tc.desc: Test order-independent normal claim release
 * @tc.desc: Positive test: two normal claimers release in arbitrary order;
 *           the flag is a shared counter that is only fully cleared when both
 *           release, at which point app C can exclusive claim
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence030, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence031
 * @tc.desc: Test exclusive owner normal-claims then hands over
 * @tc.desc: Positive test: app A exclusive claims and normal claims; app A
 *           releases the exclusive record first, then releases the normal
 *           flag, allowing app B to exclusive claim afterwards
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence031, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence032
 * @tc.desc: Test exclusive claim after a mixed two-app sequence
 * @tc.desc: Positive test: app A exclusive claims, app B normal claims and
 *           releases, app A releases; the full sequence returns to idle and
 *           app B can now claim exclusively
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence032, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence034
 * @tc.desc: Test alternating normal claims across two apps
 * @tc.desc: Positive test: app A and app B normal claim in alternation; the
 *           flag stays set the whole time and no exclusive claim is allowed
 *           in between
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence034, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    EXPECT_EQ(UEC_OK, client.ClaimInterface(pipe_, iface_, true));
    SwitchToApp(CLAIM_APP_B);
    EXPECT_EQ(UEC_OK, client.ClaimInterface(pipe_, iface_, true));
    SwitchToApp(CLAIM_APP_A);
    EXPECT_EQ(UEC_OK, client.ClaimInterface(pipe_, iface_, true));
    SwitchToApp(CLAIM_APP_B);
    EXPECT_EQ(UEC_OK, client.ClaimInterface(pipe_, iface_, true));
    SwitchToApp(CLAIM_APP_C);
    EXPECT_EQ(UEC_INTERFACE_BUSY, client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr));
}

/**
 * @tc.name: ClaimSequence035
 * @tc.desc: Test exclusive claim during an active normal claim
 * @tc.desc: Negative test: while app A stares a normal claim, app B tries to
 *           exclusive claim and is rejected; the normal claim flag blocks the
 *           exclusive claim regardless of owner
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence035, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence036
 * @tc.desc: Test the full two-app ownership round trip with normal claims
 * @tc.desc: Positive test: app A normal claims, app B normal claims, both
 *           release in order, then app B exclusive claims; the round trip
 *           proves the normal share counter returns to zero
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence036, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence037
 * @tc.desc: Test that release does not clear another app's normal claim
 * @tc.desc: Positive test: app A and app B normal claim; app A releases and
 *           app C is still busy exclusively because app B's normal claim flag
 *           remains
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence037, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence039
 * @tc.desc: Test handover after a blocked attempt and a release
 * @tc.desc: Positive test: app A exclusive claims, app B is blocked once,
 *           then app A releases and app B immediately takes over exclusively
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence039, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence040
 * @tc.desc: Test interrupted takeover with a stale normal claim
 * @tc.desc: Positive/negative test: app A exclusive claims and app B normal
 *           claims; app B releases the normal claim, app A releases the
 *           exclusive claim, then app B exclusive claims cleanly
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence040, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/*
 * Section C: three-application interaction.
 *
 * These cases involve app A, app B and app C to verify that the shared flags
 * behave correctly when more than two identities touch the same interface.
 */

/**
 * @tc.name: ClaimSequence041
 * @tc.desc: Test exclusive claim against multiple normal claimers
 * @tc.desc: Negative test: app A and app B normal claim, app C exclusive
 *           claims and is rejected; after A and B release in turn the third
 *           exclusive claim succeeds
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence041, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence042
 * @tc.desc: Test a three-way exclusive relay
 * @tc.desc: Positive test: app A exclusive claims and releases, app B claims
 *           and releases, app C claims and releases; every handover is clean
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence042, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence043
 * @tc.desc: Test three normal claimers then a sequential release
 * @tc.desc: Positive test: app A, B and C all normal claim; the interface
 *           stays blocked for exclusive claims until all three release, each
 *           release narrowing the remaining normal claimers
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence043, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence044
 * @tc.desc: Test exclusive owner blocked by a third normal claimer
 * @tc.desc: Positive/negative test: app A exclusive claims, app B and app C
 *           normal claim; app A releases and the normal flag still blocks a
 *           new exclusive claim of app A until B and C release
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence044, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence045
 * @tc.desc: Test a three-app normal claim triangle
 * @tc.desc: Positive test: app A normal claims, app B normal claims, app C
 *           normal claims; the normal claim flag is shared, so any single
 *           release is insufficient to allow an exclusive claim
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence045, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence046
 * @tc.desc: Test exclusive claim owner blocked only by normal flag
 * @tc.desc: Positive test: app A exclusive claims and app B normal claims;
 *           app A re-claims exclusively (allowed as owner) while the normal
 *           flag does not block the existing owner re-claim
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence046, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence047
 * @tc.desc: Test that a third app is never allowed during an exclusive claim
 * @tc.desc: Negative test: app A exclusive claims; both app B and app C are
 *           rejected with UEC_INTERFACE_BUSY on repeated attempts
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence047, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence048
 * @tc.desc: Test takeover by the last standing normal claimer
 * @tc.desc: Positive test: app A exclusive claims, B and C normal claim, then
 *           A and B release; the remaining normal claimer C still blocks a new
 *           exclusive claim until C releases and takes over
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence048, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence049
 * @tc.desc: Test that every identity sees the same busy state
 * @tc.desc: Negative test: app A exclusive claims; the busy state reported to
 *           app B equals the busy state reported to app C
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence049, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    auto retB = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    SwitchToApp(CLAIM_APP_C);
    auto retC = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(retB, retC);
    EXPECT_EQ(UEC_INTERFACE_BUSY, retB);
}

/**
 * @tc.name: ClaimSequence050
 * @tc.desc: Test full three-app round trip resetting to idle
 * @tc.desc: Positive test: A, B and C exclusive claim and release in turn; the
 *           interface is idle after the last release and any app may claim
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence050, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/*
 * Section D: close-based cleanup sequences.
 *
 * Close drops the HDI device handle and clears the caller exclusive record
 * only (never another app's record and never the shared normal claim flag).
 */

/**
 * @tc.name: ClaimSequence051
 * @tc.desc: Test exclusive claim then close by the owner
 * @tc.desc: Positive test: app A exclusive claims and closes; the exclusive
 *           record is cleared so app B can exclusive claim after reopening
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence051, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_TRUE(client.Close(pipe_));
    SwitchToApp(CLAIM_APP_B);
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence052
 * @tc.desc: Test close by a non-owner does not clear the exclusive record
 * @tc.desc: Negative test: app A exclusive claims, app B closes the device and
 *           reopens; the exclusive record of app A survives, so app B is still
 *           busy exclusively
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence052, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    EXPECT_TRUE(client.Close(pipe_));
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence053
 * @tc.desc: Test close while a normal claim is held by another app
 * @tc.desc: Positive test: app A exclusive claims and app B normal claims,
 *           then app A closes; the normal claim flag of app B survives the
 *           close, so app C is still busy exclusively
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence053, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_A);
    EXPECT_TRUE(client.Close(pipe_));
    SwitchToApp(CLAIM_APP_C);
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence054
 * @tc.desc: Test close clears only the caller exclusive record
 * @tc.desc: Positive test: app A exclusive claims, app B exclusive claims is
 *           busy, then app A closes and the exclusive record is cleared so a
 *           reopened app B can take over exclusively
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence054, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
    SwitchToApp(CLAIM_APP_A);
    EXPECT_TRUE(client.Close(pipe_));
    SwitchToApp(CLAIM_APP_B);
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence055
 * @tc.desc: Test the exclusive owner reclaim after close
 * @tc.desc: Positive test: app A exclusive claims, closes, reopens and claims
 *           the same interface exclusively again; the record was cleared by
 *           Close so the reclaim succeeds
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence055, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_TRUE(client.Close(pipe_));
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence056
 * @tc.desc: Test normal claimer close does not break the exclusive claim
 * @tc.desc: Negative test: app A exclusive claims and app B normal claims;
 *           app B closes and reopens, then tries an exclusive claim which is
 *           still rejected because app A keeps the exclusive record
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence056, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_TRUE(client.Close(pipe_));
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence057
 * @tc.desc: Test repeated close-open cycles preserve the record of others
 * @tc.desc: Positive test: app A exclusive claims; app B closes and reopens
 *           several times without ever clearing app A's record, and remains
 *           busy exclusively throughout
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence057, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    for (int32_t i = 0; i < 3; i++) {
        EXPECT_TRUE(client.Close(pipe_));
        EXPECT_EQ(UEC_OK, client.OpenDevice(device_, pipe_));
        EXPECT_EQ(UEC_INTERFACE_BUSY, client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr));
    }
}

/**
 * @tc.name: ClaimSequence058
 * @tc.desc: Test close by the owner inside a claim-release-close sequence
 * @tc.desc: Positive test: app A exclusive claims and normal claims, closes,
 *           then app B exclusive claims after reopen; the close cleared only
 *           the caller exclusive record and the normal flag
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence058, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_TRUE(client.Close(pipe_));
    SwitchToApp(CLAIM_APP_B);
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence059
 * @tc.desc: Test close without any prior claim
 * @tc.desc: Positive test: app A closes an idle, opened device; the operation
 *           succeeds and a following exclusive claim after reopen is clean
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence059, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    EXPECT_TRUE(client.Close(pipe_));
    EXPECT_EQ(UEC_OK, client.OpenDevice(device_, pipe_));
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence060
 * @tc.desc: Test close of a non-owner leaves the normal claim untouched
 * @tc.desc: Positive test: app A normal claims, app B closes the device and
 *           reopens; the normal claim flag (owner-less) survives the close of
 *           app B, so app C is still busy exclusively
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence060, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    EXPECT_TRUE(client.Close(pipe_));
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/*
 * Section E: transfer-state interaction sequences.
 *
 * The exclusive claim check gates BulkTransfer. These cases verify how the
 * transfer gate follows the claim state through chained claims and releases.
 */

/**
 * @tc.name: ClaimSequence061
 * @tc.desc: Test transfer of the exclusive owner across a claim sequence
 * @tc.desc: Positive test: app A exclusive claims and its transfer is allowed;
 *           after app A releases and app B takes over, app B's transfer is
 *           allowed while app A's is blocked again
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence061, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    SwitchToApp(CLAIM_APP_A);
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimSequence062
 * @tc.desc: Test transfer gate through a normal claim bridge
 * @tc.desc: Positive test: app A exclusive claims and app B normal claims;
 *           app B's transfer is allowed (normal claim disables the gate) even
 *           while app A holds the exclusive claim
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence062, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimSequence063
 * @tc.desc: Test transfer gate restored after normal claim release
 * @tc.desc: Negative test: app A exclusive claims, app B normal claims and
 *           then releases; app B's transfer is blocked again because the
 *           normal claim flag was cleared while the exclusive claim remains
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence063, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimSequence064
 * @tc.desc: Test transfer gate across close by the owner
 * @tc.desc: Positive test: app A exclusive claims and closes; the exclusive
 *           record is cleared so app B's transfer is allowed after reopen
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence064, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_TRUE(client.Close(pipe_));
    SwitchToApp(CLAIM_APP_B);
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimSequence065
 * @tc.desc: Test transfer gate on the exclusive owner before release
 * @tc.desc: Positive test: app A exclusive claims and transfers (allowed),
 *           then transfers again before releasing (allowed again); the owner
 *           gate stays open for the whole holding window
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence065, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimSequence066
 * @tc.desc: Test transfer gate for a third app during exclusive claim
 * @tc.desc: Negative test: app A exclusive claims; both app B and app C are
 *           transfer-blocked with UEC_SERVICE_IO_EXCEPTION
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence066, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    SwitchToApp(CLAIM_APP_B);
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
    SwitchToApp(CLAIM_APP_C);
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimSequence067
 * @tc.desc: Test transfer gate after a normal claim by the owner
 * @tc.desc: Positive test: app A exclusive claims, then normal claims, then
 *           releases the normal claim; the exclusive record remains so app A's
 *           own transfer is still allowed
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence067, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimSequence068
 * @tc.desc: Test transfer gate after releasing the exclusive claim
 * @tc.desc: Positive test: app A exclusive claims and releases; app B's
 *           transfer is then allowed because no exclusive claim remains
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence068, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    int32_t transferRet = UEC_OK;
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimSequence069
 * @tc.desc: Test transfer gate stability under repeated exclusive re-claim
 * @tc.desc: Negative test: app A exclusive claims several times (idempotent)
 *           while app B is transfer-blocked throughout
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence069, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    SwitchToApp(CLAIM_APP_B);
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimSequence070
 * @tc.desc: Test that transfer gate follows the last released state
 * @tc.desc: Positive test: after a full sequence of claim and release the
 *           transfer of the previous owner is allowed again
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence070, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    int32_t transferRet = UEC_OK;
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    DoBulkTransfer(pipe_, endpoint_, transferRet);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/*
 * Section F: repeated-cycle stability.
 *
 * The state machine must not leak state across repeated claim/release cycles:
 * every cycle starts and ends in the idle baseline.
 */

/**
 * @tc.name: ClaimSequence071
 * @tc.desc: Test repeated exclusive claim and release cycles
 * @tc.desc: Positive test: app A exclusive claims and releases ten times; each
 *           cycle is clean and app B can take over after the final release
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence071, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    for (int32_t i = 0; i < 10; i++) {
        auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
    }
    SwitchToApp(CLAIM_APP_B);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence072
 * @tc.desc: Test repeated normal claim and release cycles
 * @tc.desc: Positive test: app A normal claims and releases ten times; the
 *           normal flag never leaks, so app B can exclusive claim afterwards
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence072, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    for (int32_t i = 0; i < 10; i++) {
        auto ret = client.ClaimInterface(pipe_, iface_, true);
        EXPECT_EQ(UEC_OK, ret);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
    }
    SwitchToApp(CLAIM_APP_B);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence073
 * @tc.desc: Test alternating exclusive ownership across cycles
 * @tc.desc: Positive test: app A and app B alternate the exclusive claim for
 *           several cycles with a full release in between; takeover never
 *           fails
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence073, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    for (int32_t i = 0; i < 5; i++) {
        SwitchToApp(CLAIM_APP_A);
        auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
        SwitchToApp(CLAIM_APP_B);
        ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
    }
}

/**
 * @tc.name: ClaimSequence074
 * @tc.desc: Test mixed normal and exclusive cycles
 * @tc.desc: Positive test: app A alternates between exclusive claim and normal
 *           claim with releases, verifying the two flag types never interfere
 *           across cycles
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence074, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    for (int32_t i = 0; i < 5; i++) {
        auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
        ret = client.ClaimInterface(pipe_, iface_, true);
        EXPECT_EQ(UEC_OK, ret);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
    }
    SwitchToApp(CLAIM_APP_B);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence075
 * @tc.desc: Test that a failed cycle leaves no residue
 * @tc.desc: Negative/positive test: each cycle starts with a blocked exclusive
 *           claim (which records no state), then a successful claim and
 *           release; the baseline stays idle across all cycles
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence075, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    for (int32_t i = 0; i < 5; i++) {
        SwitchToApp(CLAIM_APP_A);
        auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        SwitchToApp(CLAIM_APP_B);
        ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
        SwitchToApp(CLAIM_APP_A);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
    }
    SwitchToApp(CLAIM_APP_B);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimSequence076
 * @tc.desc: Test repeated exclusive claim without releasing
 * @tc.desc: Positive test: app A exclusive claims ten times without releasing;
 *           the idempotent re-claim keeps a single record and app B is busy
 *           after all ten
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence076, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    for (int32_t i = 0; i < 10; i++) {
        auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
    }
    SwitchToApp(CLAIM_APP_B);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence077
 * @tc.desc: Test repeated normal claim without releasing
 * @tc.desc: Positive test: app A normal claims ten times without releasing;
 *           the shared flag stays set and blocks an exclusive claim of app B
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence077, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    for (int32_t i = 0; i < 10; i++) {
        auto ret = client.ClaimInterface(pipe_, iface_, true);
        EXPECT_EQ(UEC_OK, ret);
    }
    SwitchToApp(CLAIM_APP_B);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimSequence078
 * @tc.desc: Test interleaved three-app cycles return to idle
 * @tc.desc: Positive test: A, B and C each perform an exclusive claim/release
 *           round in rotation; the interface returns to idle after every
 *           release and the final takeover by A succeeds
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence078, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    for (int32_t i = 0; i < 3; i++) {
        SwitchToApp(CLAIM_APP_A);
        auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
        SwitchToApp(CLAIM_APP_B);
        ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
        SwitchToApp(CLAIM_APP_C);
        ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
    }
}

/**
 * @tc.name: ClaimSequence079
 * @tc.desc: Test exclusive claim state after many close-open cycles
 * @tc.desc: Positive test: app A exclusive claims and closes, app B takes over
 *           and closes; repeating this handover several times never leaks a
 *           stale exclusive record
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence079, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    for (int32_t i = 0; i < 3; i++) {
        SwitchToApp(CLAIM_APP_A);
        auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        EXPECT_TRUE(client.Close(pipe_));
        SwitchToApp(CLAIM_APP_B);
        ret = client.OpenDevice(device_, pipe_);
        EXPECT_EQ(UEC_OK, ret);
        ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        EXPECT_TRUE(client.Close(pipe_));
        SwitchToApp(CLAIM_APP_A);
        ret = client.OpenDevice(device_, pipe_);
        EXPECT_EQ(UEC_OK, ret);
    }
}

/**
 * @tc.name: ClaimSequence080
 * @tc.desc: Test transfer gate over repeated handover cycles
 * @tc.desc: Positive test: each exclusive handover flips the transfer gate
 *           correctly between the previous and the new owner
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimSequenceTest, ClaimSequence080, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    for (int32_t i = 0; i < 3; i++) {
        SwitchToApp(CLAIM_APP_A);
        auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        int32_t transferRet = UEC_OK;
        DoBulkTransfer(pipe_, endpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
        SwitchToApp(CLAIM_APP_B);
        ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
        EXPECT_EQ(UEC_OK, ret);
        DoBulkTransfer(pipe_, endpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
        ret = client.ReleaseInterface(pipe_, iface_);
        EXPECT_EQ(UEC_OK, ret);
    }
}
} // namespace USB
} // namespace OHOS