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

#include "usb_claim_exclusive_test.h"

#include "hilog_wrapper.h"
#include "usb_common_test.h"
#include "usb_errors.h"

using namespace testing::ext;
using namespace OHOS::USB::Common;

namespace OHOS {
namespace USB {

void UsbClaimExclusiveTest::SetUpTestCase()
{
    UsbClaimTestBase::SetUpTestCase();
}

void UsbClaimExclusiveTest::TearDownTestCase()
{
    UsbClaimTestBase::TearDownTestCase();
}

/**
 * @tc.name: ClaimExclusive001
 * @tc.desc: Test ClaimInterfaceExclusive when the interface is idle
 * @tc.desc: Positive test: app A exclusive claims an idle interface
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive001, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    SwitchToApp(CLAIM_APP_A);
    auto &client = UsbSrvClient::GetInstance();
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimExclusive002
 * @tc.desc: Test ClaimInterfaceExclusive between two different applications
 * @tc.desc: Negative test: app B is rejected with UEC_INTERFACE_BUSY after app A
 *           exclusive claimed the same interface
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive002, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimExclusive003
 * @tc.desc: Test repeated ClaimInterfaceExclusive by the same application
 * @tc.desc: Positive test: app A can exclusive claim the same interface twice
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive003, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimExclusive004
 * @tc.desc: Test normal claim after an exclusive claim by another application
 * @tc.desc: Positive test: normal claim is never blocked by an exclusive claim
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive004, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimExclusive005
 * @tc.desc: Test normal claim after an exclusive claim by the same application
 * @tc.desc: Positive test: the exclusive owner can also normal claim the interface
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive005, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimExclusive006
 * @tc.desc: Test ClaimInterfaceExclusive after a normal claim by another application
 * @tc.desc: Negative test: exclusive claim is rejected when normal claim exists
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive006, TestSize.Level1)
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
 * @tc.name: ClaimExclusive007
 * @tc.desc: Test ClaimInterfaceExclusive after normal claims by two applications
 * @tc.desc: Negative test: exclusive claim is rejected when normal claim flag exists
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive007, TestSize.Level1)
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
 * @tc.name: ClaimExclusive008
 * @tc.desc: Test ClaimInterfaceExclusive after the normal claim is released
 * @tc.desc: Positive test: exclusive claim succeeds once normal claim is cleared
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive008, TestSize.Level1)
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
 * @tc.name: ClaimExclusive009
 * @tc.desc: Test that normal claim behavior keeps unchanged for multiple applications
 * @tc.desc: Positive test: regression, alternate normal claims all succeed
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive009, TestSize.Level1)
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
}

/**
 * @tc.name: ClaimExclusive011
 * @tc.desc: Test ClaimInterfaceExclusive without a conflict callback
 * @tc.desc: Positive test: null callback is accepted (degraded mode)
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive011, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    SwitchToApp(CLAIM_APP_A);
    auto &client = UsbSrvClient::GetInstance();
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimExclusive013
 * @tc.desc: Test ClaimInterfaceExclusive without device access right
 * @tc.desc: Negative test: a hap token without usb right is rejected
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive013, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    Security::AccessToken::AccessTokenID hapToken = UsbCommonTest::AllocHapTest();
    ASSERT_NE(0U, hapToken);
    UsbCommonTest::SetSelfToken(hapToken);
    auto &client = UsbSrvClient::GetInstance();
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED, ret);
    SwitchToApp(CLAIM_APP_A);
}

/**
 * @tc.name: ClaimExclusive014
 * @tc.desc: Test ClaimInterfaceExclusive with an invalid device address
 * @tc.desc: Negative test: device permission check fails for a non-existent device
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimExclusiveTest, ClaimExclusive014, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    SwitchToApp(CLAIM_APP_A);
    USBDevicePipe invalidPipe(UINT8_MAX, UINT8_MAX);
    auto &client = UsbSrvClient::GetInstance();
    auto ret = client.ClaimInterfaceExclusive(invalidPipe, iface_, true, nullptr);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED, ret);
}
} // namespace USB
} // namespace OHOS
