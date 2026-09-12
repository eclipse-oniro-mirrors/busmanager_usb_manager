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

#include "usb_claim_force_test.h"

#include <unistd.h>
#include <vector>

#include "hilog_wrapper.h"
#include "usb_errors.h"

using namespace testing::ext;

namespace OHOS {
namespace USB {
namespace {
constexpr int32_t HID_INTERFACE_CLASS = 3;
constexpr int32_t HUB_DEVICE_CLASS = 9;
constexpr int32_t HID_RETRY_INTERVAL_MS = 200;
constexpr int32_t HID_RETRY_TIMEOUT_MS = 5000;

/*
 * After the unplug/plug cycle the HID device is re-enumerated and the usbhid
 * driver rebinds asynchronously, so locating the HID interface needs retries.
 */
bool PickHidInterfaceWithRetry()
{
    int32_t waitedMs = 0;
    while (true) {
        if (UsbClaimForceTest::PickHidInterface()) {
            return true;
        }
        if (waitedMs >= HID_RETRY_TIMEOUT_MS) {
            return false;
        }
        usleep(HID_RETRY_INTERVAL_MS * 1000);
        waitedMs += HID_RETRY_INTERVAL_MS;
    }
}
}

void UsbClaimForceTest::SetUpTestCase()
{
    UsbClaimTestBase::SetUpTestCase();
    ASSERT_TRUE(PickHidInterface());
    ASSERT_TRUE(IsKernelDriverBound()) << "kernel driver is not bound to the HID interface";
}

void UsbClaimForceTest::TearDownTestCase()
{
    UsbClaimTestBase::TearDownTestCase();
}

/*
 * Every force case runs one unplug/plug cycle first, so the HID kernel
 * driver (usbhid) is freshly bound to the interface and a non-forced claim
 * must be rejected by the HDI layer.
 */
void UsbClaimForceTest::SetUp()
{
    ASSERT_TRUE(SimulateUnplug());
    ASSERT_TRUE(SimulatePlug());
    ASSERT_TRUE(PickHidInterfaceWithRetry());
    ASSERT_TRUE(IsKernelDriverBound());
    CleanClaimState();
}

void UsbClaimForceTest::TearDown()
{
    CleanClaimState();
}

bool UsbClaimForceTest::PickHidInterface()
{
    /*
     * Scan every attached non-hub device for a HID interface (mouse/keyboard,
     * interface class 3 bound to usbhid) and re-target the fixture members at
     * it, independent of which device the base fixture picked.
     */
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    if (client.GetDevices(deviceList) != UEC_OK) {
        return false;
    }
    for (auto &dev : deviceList) {
        if (dev.GetClass() == HUB_DEVICE_CLASS) {
            continue;
        }
        for (auto &config : dev.GetConfigs()) {
            for (auto &interface : config.GetInterfaces()) {
                if (interface.GetClass() != HID_INTERFACE_CLASS) {
                    continue;
                }
                if (interface.GetEndpoints().empty()) {
                    continue;
                }
                device_ = dev;
                busNum_ = dev.GetBusNum();
                devAddr_ = dev.GetDevAddr();
                devName_ = dev.GetName();
                ifaceId_ = static_cast<uint8_t>(interface.GetId());
                iface_ = interface;
                endpoint_ = interface.GetEndpoints().at(0);
                pipe_.SetBusNum(busNum_);
                pipe_.SetDevAddr(devAddr_);
                USB_HILOGI(MODULE_USB_SERVICE, "HID interface picked bus=%{public}hhu dev=%{public}hhu if=%{public}hhu",
                    busNum_, devAddr_, ifaceId_);
                return true;
            }
        }
    }
    USB_HILOGE(MODULE_USB_SERVICE, "no HID interface found on any attached device");
    return false;
}

/**
 * @tc.name: ClaimForce001
 * @tc.desc: Test the force parameter forwarding of ClaimInterfaceExclusive
 * @tc.desc: Positive/negative test: after a unplug/plug cycle the kernel
 *           driver holds the HID interface, so force=0 is rejected by the HDI
 *           layer, force=1 succeeds (force is forwarded), and a later
 *           exclusive claim of app B is still rejected at the service layer
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimForceTest, ClaimForce001, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ASSERT_TRUE(IsKernelDriverBound());
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, false, nullptr);
    EXPECT_NE(UEC_OK, ret);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, false, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
}

/**
 * @tc.name: ClaimForce002
 * @tc.desc: Test that no state is recorded when the underlying HDI claim fails
 * @tc.desc: Negative test: after a unplug/plug cycle the kernel driver holds
 *           the HID interface, so the force=0 exclusive claim of app A fails
 *           at the HDI layer and records nothing, which must not block a
 *           later exclusive claim of app B
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimForceTest, ClaimForce002, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ASSERT_TRUE(IsKernelDriverBound());
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, false, nullptr);
    EXPECT_NE(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
}
} // namespace USB
} // namespace OHOS
