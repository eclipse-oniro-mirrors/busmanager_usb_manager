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

#include "usb_claim_release_test.h"

#include <vector>

#include "hilog_wrapper.h"
#include "usb_errors.h"

using namespace testing::ext;

namespace OHOS {
namespace USB {
namespace {
constexpr int32_t TRANSFER_BUFFER_SIZE = 8;
constexpr int32_t TRANSFER_TIMEOUT_MS = 300;
}

void UsbClaimReleaseTest::SetUpTestCase()
{
    UsbClaimTestBase::SetUpTestCase();
}

void UsbClaimReleaseTest::TearDownTestCase()
{
    UsbClaimTestBase::TearDownTestCase();
}

/**
 * @tc.name: ClaimRelease001
 * @tc.desc: Test ReleaseInterface by a non-owner while the interface is
 *           exclusively claimed without normal claim
 * @tc.desc: Negative test: rejected with UEC_INTERFACE_INVALID_OPERATION
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease001, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_INTERFACE_INVALID_OPERATION, ret);
}

/**
 * @tc.name: ClaimRelease002
 * @tc.desc: Test ReleaseInterface by the exclusive owner without normal claim
 * @tc.desc: Positive test: release succeeds
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease002, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimRelease003
 * @tc.desc: Test ReleaseInterface by the exclusive owner while a normal claim
 *           exists
 * @tc.desc: Positive test: release succeeds and the normal claim flag is
 *           retained, so a following exclusive claim is still busy
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease003, TestSize.Level1)
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
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimRelease004
 * @tc.desc: Test ReleaseInterface by a normal claimer while the interface is
 *           still exclusively claimed
 * @tc.desc: Positive test: release succeeds, the normal claim flag is cleared
 *           and transfers of the normal claimer are blocked again
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease004, TestSize.Level1)
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
    std::vector<uint8_t> buffer(TRANSFER_BUFFER_SIZE, 0);
    ret = client.BulkTransfer(pipe_, endpoint_, buffer, TRANSFER_TIMEOUT_MS);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, ret);
}

/**
 * @tc.name: ClaimRelease005
 * @tc.desc: Test ReleaseInterface with only a normal claim
 * @tc.desc: Positive test: release succeeds
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease005, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimRelease006
 * @tc.desc: Test ReleaseInterface on an idle interface
 * @tc.desc: Positive test: the request is not blocked by the claim state
 *           machine and is forwarded to the HDI layer, where releasing an
 *           interface that was never claimed fails at the driver level
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease006, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    SwitchToApp(CLAIM_APP_A);
    auto &client = UsbSrvClient::GetInstance();
    auto ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_NE(UEC_OK, ret);
    EXPECT_NE(UEC_INTERFACE_INVALID_OPERATION, ret);
    EXPECT_NE(UEC_SERVICE_PERMISSION_DENIED, ret);
}

/**
 * @tc.name: ClaimRelease007
 * @tc.desc: Test chained release: normal claimer first, then exclusive owner
 * @tc.desc: Positive test: both releases succeed and the interface returns to
 *           idle, so app C can exclusive claim afterwards
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease007, TestSize.Level1)
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
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimRelease008
 * @tc.desc: Test chained release: exclusive owner first, then normal claimer
 * @tc.desc: Positive test: both releases succeed and the interface returns to
 *           idle, so app C can exclusive claim afterwards
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease008, TestSize.Level1)
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
    SwitchToApp(CLAIM_APP_B);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimRelease009
 * @tc.desc: Test exclusive claim after the previous exclusive claim is released
 * @tc.desc: Positive test: release clears the exclusive record
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease009, TestSize.Level1)
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
 * @tc.name: ClaimRelease010
 * @tc.desc: Test Close called by the exclusive owner
 * @tc.desc: Positive test: Close clears the caller exclusive record, so app B
 *           can exclusive claim afterwards
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease010, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_TRUE(client.Close(pipe_));
    SwitchToApp(CLAIM_APP_B);
    // Close released the HDI device handle, reopen it before claiming again
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimRelease011
 * @tc.desc: Test Close called by another application
 * @tc.desc: Negative test: Close does not clear the exclusive record of other
 *           applications
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease011, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    EXPECT_TRUE(client.Close(pipe_));
    // Close released the HDI device handle, reopen it before claiming again
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimRelease012
 * @tc.desc: Test Close called by the exclusive owner while a normal claim exists
 * @tc.desc: Positive test: Close clears only the caller exclusive record and
 *           the normal claim flag is retained
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease012, TestSize.Level1)
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
    SwitchToApp(CLAIM_APP_B);
    // Close released the HDI device handle, reopen it before claiming again
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimRelease013
 * @tc.desc: Test exclusive claim cleanup after the owner closes the device
 * @tc.desc: Positive test: app A exclusive claims, closes the device, then
 *           reopens and exclusive claims the same interface again; the
 *           record was cleared by Close, so the reclaim succeeds and other
 *           apps are not blocked anymore
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease013, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_TRUE(client.Close(pipe_));

    // the previous exclusive owner reopens and reclaims the same interface
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
    // cleanup the reclaim for the next case
    SwitchToApp(CLAIM_APP_A);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimRelease014
 * @tc.desc: Test transfer unblocking after the owner closes the device
 * @tc.desc: Positive test: while A holds the exclusive claim the transfer of
 *           app B is blocked; after A closes the device (clearing the
 *           exclusive record) the transfer of app B is no longer blocked
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimReleaseTest, ClaimRelease014, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    std::vector<uint8_t> buffer(TRANSFER_BUFFER_SIZE, 0);
    ret = client.BulkTransfer(pipe_, endpoint_, buffer, TRANSFER_TIMEOUT_MS);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, ret);

    // the owner closes the device, clearing the exclusive record
    SwitchToApp(CLAIM_APP_A);
    EXPECT_TRUE(client.Close(pipe_));

    // app B reopens and its transfer passes the (now empty) claim check
    SwitchToApp(CLAIM_APP_B);
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.BulkTransfer(pipe_, endpoint_, buffer, TRANSFER_TIMEOUT_MS);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, ret);
}
} // namespace USB
} // namespace OHOS
