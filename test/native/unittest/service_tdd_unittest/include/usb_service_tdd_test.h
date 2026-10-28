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

#ifndef USB_SERVICE_TDD_TEST_H
#define USB_SERVICE_TDD_TEST_H

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include "usb_device.h"
#include "usb_device_pipe.h"
#include "usb_endpoint.h"
#include "usb_interface.h"

namespace OHOS {
namespace USB {
namespace Service {

/*
 * TDD fixture for UsbService (services/native/src/usb_service.cpp).
 *
 * Tests talk to the real usb_service system ability through UsbSrvClient IPC
 * and specify the externally observable behavior of the service:
 * permission gating, device right lookup, claim state machine, transfer
 * parameter validation, serial/port/device-function input contracts.
 *
 * Caller identities are impersonated by switching the self token id of the
 * test process between native tokens:
 *   - sys caller   : system_basic native token holding MANAGE_USB_CONFIG
 *   - normal caller: normal native token without any permission
 *   - sysNoPerm    : system_basic native token without permissions
 *   - appA/appB    : two distinct normal native tokens, used as two separate
 *                    applications for the exclusive claim state machine which
 *                    keys on the calling token id.
 *
 * Device dependent cases use the first non-hub device reported by GetDevices
 * and are skipped when no device is attached, so the suite also runs on
 * hardware-less CI.
 */
class UsbServiceTddTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

protected:
    static bool PickTestDevice();
    static void CleanClaimState();
    static void SwitchToSysCaller();
    static void SwitchToNormalCaller();
    static void SwitchToSysCallerNoPerm();
    static void SwitchToApp(int32_t appIndex);

    static uint64_t tokenA_;
    static uint64_t tokenB_;
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

} // Service
} // USB
} // OHOS

#endif // USB_SERVICE_TDD_TEST_H
