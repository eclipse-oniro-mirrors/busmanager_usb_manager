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

#include "usb_claim_hotplug_test.h"

#include <functional>

#include "hilog_wrapper.h"
#include "usb_errors.h"

using namespace testing::ext;

namespace OHOS {
namespace USB {
namespace {
std::function<void(uint8_t, uint8_t, uint8_t)> MakeHotplugNotifyCallback()
{
    return [](uint8_t busNum, uint8_t devAddr, uint8_t interfaceId) {
        USB_HILOGI(MODULE_USB_SERVICE, "notify received bus=%{public}hhu dev=%{public}hhu if=%{public}hhu",
            busNum, devAddr, interfaceId);
    };
}
} // namespace

void UsbClaimHotplugTest::SetUpTestCase()
{
    UsbClaimTestBase::SetUpTestCase();
}

void UsbClaimHotplugTest::TearDownTestCase()
{
    EnsureDevicePresent();
    UsbClaimTestBase::TearDownTestCase();
}

void UsbClaimHotplugTest::SetUp()
{
    ASSERT_TRUE(EnsureDevicePresent());
    CleanClaimState();
}

void UsbClaimHotplugTest::TearDown()
{
    if (EnsureDevicePresent()) {
        CleanClaimState();
    }
}

/*
 * The unplug/plug cycle is implemented by the base fixture (ManageGlobalInterface
 * based, see UsbClaimTestBase::SimulateUnplug/SimulatePlug). This suite only
 * combines it with claim state assertions.
 */
bool UsbClaimHotplugTest::EnsureDevicePresent()
{
    if (WaitForDevicePresent(true, 0)) {
        return RefreshTestDevice();
    }
    return SimulatePlug();
}

/**
 * @tc.name: ClaimHotplug001
 * @tc.desc: Test claim state cleanup when the device is unplugged
 * @tc.desc: Positive test: the exclusive claim and the normal claim flag are
 *           cleared on device removal, so the previous exclusive owner can
 *           claim again after the device is plugged back
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimHotplugTest, ClaimHotplug001, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);

    ASSERT_TRUE(SimulateUnplug());
    ASSERT_TRUE(SimulatePlug());

    SwitchToApp(CLAIM_APP_A);
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimHotplug002
 * @tc.desc: Test exclusive claim while the device is unplugged
 * @tc.desc: Negative test: the claim fails because the device is gone
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimHotplugTest, ClaimHotplug002, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);

    ASSERT_TRUE(SimulateUnplug());

    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_NE(UEC_OK, ret);
}

/**
 * @tc.name: ClaimHotplug003
 * @tc.desc: Test a normal claim after device unplug and plug
 * @tc.desc: Positive test: the claim state of the removed device does not
 *           affect a fresh normal claim after the device is back
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimHotplugTest, ClaimHotplug003, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, MakeHotplugNotifyCallback());
    EXPECT_EQ(UEC_OK, ret);

    ASSERT_TRUE(SimulateUnplug());
    ASSERT_TRUE(SimulatePlug());

    SwitchToApp(CLAIM_APP_B);
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ClaimHotplug004
 * @tc.desc: Test that a stale exclusive claim does not survive device unplug
 * @tc.desc: Positive test: another application can exclusive claim the
 *           interface after the device is plugged back
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimHotplugTest, ClaimHotplug004, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);

    ASSERT_TRUE(SimulateUnplug());
    ASSERT_TRUE(SimulatePlug());

    SwitchToApp(CLAIM_APP_B);
    ret = client.OpenDevice(device_, pipe_);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}
} // namespace USB
} // namespace OHOS
