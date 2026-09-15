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

#include <cstdio>
#include <iostream>
#include <string>

#include "usb_srv_client.h"
#include "usb_connection_callback_stub.h"
#include "usb_common_test.h"
#include "usb_errors.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::USB;
using namespace OHOS::USB::Common;

static UsbSrvClient &g_usbClient = UsbSrvClient::GetInstance();

static void DumpDevice(const UsbDevice &device)
{
    printf("    [device] name=%s\n", device.GetName().c_str());
    printf("             manufacturer=%s product=%s\n",
        device.GetManufacturerName().c_str(), device.GetProductName().c_str());
    printf("             bus=%u dev=%u vid=0x%04x pid=0x%04x class=0x%x subclass=0x%x proto=0x%x configs=%d\n",
        device.GetBusNum(), device.GetDevAddr(), device.GetVendorId(), device.GetProductId(),
        device.GetClass(), device.GetSubclass(), device.GetProtocol(), device.GetConfigCount());
}

class UsbListenerCallback : public UsbConnectionCallbackStub {
public:
    UsbListenerCallback() = default;
    ~UsbListenerCallback() override = default;

    int32_t OnDeviceConnected(const UsbDevice &device, int32_t uid, const std::string &bundleName) override
    {
        printf("\n[<<< listener callback >>>] type=CONNECT\n");
        DumpDevice(device);
        printf("  [app] uid=%d bundleName=%s\n\n", uid, bundleName.c_str());
        fflush(stdout);
        return 0;
    }

    int32_t OnDeviceDisconnected(const UsbDevice &device, int32_t uid, const std::string &bundleName) override
    {
        printf("\n[<<< listener callback >>>] type=DISCONNECT\n");
        DumpDevice(device);
        printf("  [app] uid=%d bundleName=%s\n\n", uid, bundleName.c_str());
        fflush(stdout);
        return 0;
    }
};

static void PrintHelp()
{
    printf("================================================================\n");
    printf("  USB device connection listener test\n");
    printf("  (register/unregister the device connection InnerAPI)\n");
    printf("----------------------------------------------------------------\n");
    printf("  1 : register device listener (immediate replay of connected)\n");
    printf("  2 : unregister device listener\n");
    printf("  0 / q : quit\n");
    printf("----------------------------------------------------------------\n");
    printf("  After registering, connect/close/unplug a USB device\n");
    printf("  via usbManager#connectDevice / closePipe to see events here.\n");
    printf("================================================================\n");
}

int main(int argc, char *argv[])
{
    // Grant system_basic + ohos.permission.MANAGE_USB_CONFIG so the
    // server-side RegisterConnectionListener permission check passes.
    UsbCommonTest::GrantPermissionSysNative();

    sptr<IUsbConnectionCallback> listener = new (std::nothrow) UsbListenerCallback();
    if (listener == nullptr) {
        printf("[fail] new UsbListenerCallback failed\n");
        return -1;
    }

    string line;
    PrintHelp();
    while (true) {
        printf("input> ");
        fflush(stdout);
        if (!std::getline(std::cin, line)) {
            break; // EOF
        }

        if (line.empty()) {
            continue;
        }

        if (line == "1") {
            int32_t ret = g_usbClient.RegisterConnectionListener(listener);
            if (ret == UEC_OK) {
                printf("[ok] register listener success, waiting events...\n");
            } else {
                printf("[fail] register listener failed, ret=%d\n", ret);
            }
        } else if (line == "2") {
            int32_t ret = g_usbClient.UnRegisterConnectionListener(listener);
            if (ret == UEC_OK) {
                printf("[ok] unregister listener success\n");
            } else {
                printf("[fail] unregister listener failed, ret=%d\n", ret);
            }
        } else if (line == "0" || line == "q" || line == "Q") {
            break;
        } else {
            printf("[warn] unknown input '%s'\n", line.c_str());
        }
    }

    g_usbClient.UnRegisterConnectionListener(listener);
    printf("bye.\n");
    return 0;
}