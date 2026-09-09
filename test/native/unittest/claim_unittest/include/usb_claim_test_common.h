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

#ifndef USB_CLAIM_TEST_COMMON_H
#define USB_CLAIM_TEST_COMMON_H

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

#include "usb_device.h"
#include "usb_device_pipe.h"
#include "usb_endpoint.h"
#include "usb_interface.h"
#include "usb_srv_client.h"

namespace OHOS {
namespace USB {

enum ClaimTestApp {
    CLAIM_APP_A = 0,
    CLAIM_APP_B = 1,
    CLAIM_APP_C = 2,
    CLAIM_APP_SYS = 3,
};

/*
 * Base fixture for exclusive claim device tests.
 *
 * The tests run on a real device and talk to the real usb_service system
 * ability through UsbSrvClient IPC. Different applications are impersonated
 * by switching the self token id of the test process between several native
 * tokens (tokenA/tokenB/tokenC). Native tokens are treated as system side
 * callers by the service permission check, while the exclusive claim state
 * machine keys on the calling token id, so every token behaves as a separate
 * application. tokenSys carries ohos.permission.MANAGE_USB_CONFIG and is used
 * for management APIs (ManageGlobalInterface) that require the permission.
 *
 * The test device is the first non-hub device reported by GetDevices. It must
 * expose at least one interface with at least one endpoint. Claims use
 * force=true because the kernel driver (for example usbhid) usually holds the
 * interface of an off-the-shelf device.
 */
class UsbClaimTestBase : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

    static void SwitchToApp(ClaimTestApp app);
    static void CleanClaimState();
    static bool RefreshTestDevice();
    static bool WaitForDevicePresent(bool present, int32_t timeoutMs);
    static bool SimulateUnplug();
    static bool SimulatePlug();
    static bool IsKernelDriverBound();

protected:
    static bool PickTestDevice();

    static uint64_t tokenA_;
    static uint64_t tokenB_;
    static uint64_t tokenC_;
    static uint64_t tokenSys_;
    static uint8_t busNum_;
    static uint8_t devAddr_;
    static uint8_t ifaceId_;
    static std::string devName_;
    static UsbDevice device_;
    static USBDevicePipe pipe_;
    static UsbInterface iface_;
    static USBEndpoint endpoint_;
    static bool deviceReady_;
};
} // namespace USB
} // namespace OHOS
#endif // USB_CLAIM_TEST_COMMON_H
