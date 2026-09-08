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

#include <atomic>
#include <csignal>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <vector>

#include "accesstoken_kit.h"
#include "hilog_wrapper.h"
#include "ipc_skeleton.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "usb_device.h"
#include "usb_device_pipe.h"
#include "usb_errors.h"
#include "usb_srv_client.h"

using namespace OHOS;
using namespace OHOS::USB;

namespace {
constexpr int32_t PROBE_MODE_ARG_INDEX = 4;
constexpr int32_t MIN_ARG_COUNT = 2;
constexpr int32_t PROBE_EXIT_FAIL = 2;
std::mutex g_mutex;
std::condition_variable g_cv;
std::atomic<int32_t> g_notifyCount(0);
std::atomic<bool> g_holdMode(true);
uint8_t g_bus = 0;
uint8_t g_dev = 0;
uint8_t g_if = 0;

void AllocToken(const char *name)
{
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 0,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = nullptr,
        .acls = nullptr,
        .processName = name,
        .aplStr = "normal",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

void HoldSignalHandler(int)
{
    g_holdMode.store(false);
}

bool FindTestDevice(UsbSrvClient &client, uint8_t busNum, uint8_t devAddr, UsbDevice &device)
{
    std::vector<UsbDevice> deviceList;
    if (client.GetDevices(deviceList) != UEC_OK || deviceList.empty()) {
        printf("get devices failed\n");
        return false;
    }
    for (auto &dev : deviceList) {
        if (dev.GetBusNum() == busNum && dev.GetDevAddr() == devAddr) {
            device = dev;
            return true;
        }
    }
    printf("device %u-%u not found\n", busNum, devAddr);
    return false;
}

bool FindTestInterface(UsbDevice &device, uint8_t ifaceId, UsbInterface &iface)
{
    for (auto &config : device.GetConfigs()) {
        for (auto &interface : config.GetInterfaces()) {
            if (static_cast<uint8_t>(interface.GetId()) == ifaceId) {
                iface = interface;
                return true;
            }
        }
    }
    printf("interface %u not found\n", ifaceId);
    return false;
}

void RunHoldMode()
{
    signal(SIGTERM, HoldSignalHandler);
    signal(SIGINT, HoldSignalHandler);
    printf("hold mode: keeping exclusive claim until killed\n");
    while (g_holdMode.load()) {
        pause();
    }
}

int RunSelfTest(UsbSrvClient &client, USBDevicePipe &pipe, UsbInterface &iface,
    uint8_t busNum, uint8_t devAddr, uint8_t ifaceId)
{
    sleep(1);
    AllocToken("claim_notify_probe_other");
    int32_t ret = client.ClaimInterface(pipe, iface, true);
    printf("other app normal claim ret=%d\n", ret);

    std::unique_lock<std::mutex> lock(g_mutex);
    bool notified = g_cv.wait_for(lock, std::chrono::seconds(5), []() { return g_notifyCount > 0; });
    printf("notify result: %s, count=%d\n", notified ? "RECEIVED" : "TIMEOUT", g_notifyCount.load());
    if (notified) {
        printf("params: bus=%u dev=%u if=%u (expect %u/%u/%u)\n", g_bus, g_dev, g_if, busNum, devAddr, ifaceId);
    }

    AllocToken("claim_notify_probe");
    client.ReleaseInterface(pipe, iface);
    client.UsbAttachKernelDriver(pipe, iface);
    AllocToken("claim_notify_probe_other");
    client.ReleaseInterface(pipe, iface);
    AllocToken("claim_notify_probe");
    client.Close(pipe);
    return notified ? 0 : PROBE_EXIT_FAIL;
}
} // namespace

int main(int argc, char *argv[])
{
    if (argc < MIN_ARG_COUNT) {
        printf("usage: %s <busNum> <devAddr> <ifaceId>\n", argv[0]);
        return 1;
    }
    uint8_t busNum = static_cast<uint8_t>(atoi(argv[1]));
    uint8_t devAddr = static_cast<uint8_t>(atoi(argv[2]));
    uint8_t ifaceId = static_cast<uint8_t>(atoi(argv[3]));

    AllocToken("claim_notify_probe");
    auto &client = UsbSrvClient::GetInstance();

    UsbDevice device;
    if (!FindTestDevice(client, busNum, devAddr, device)) {
        return 1;
    }
    USBDevicePipe pipe;
    if (client.OpenDevice(device, pipe) != UEC_OK) {
        printf("open device failed\n");
        return 1;
    }
    UsbInterface iface;
    if (!FindTestInterface(device, ifaceId, iface)) {
        return 1;
    }

    auto callback = [](uint8_t busNumCb, uint8_t devAddrCb, uint8_t interfaceIdCb) {
        printf(">>> NOTIFY RECEIVED bus=%u dev=%u if=%u\n", busNumCb, devAddrCb, interfaceIdCb);
        USB_HILOGI(MODULE_USB_SERVICE, ">>> NOTIFY RECEIVED bus=%{public}hhu dev=%{public}hhu if=%{public}hhu",
            busNumCb, devAddrCb, interfaceIdCb);
        std::lock_guard<std::mutex> lock(g_mutex);
        g_notifyCount++;
        g_bus = busNumCb;
        g_dev = devAddrCb;
        g_if = interfaceIdCb;
        g_cv.notify_all();
    };

    if (argc > PROBE_MODE_ARG_INDEX && strcmp(argv[PROBE_MODE_ARG_INDEX], "normal") == 0) {
        int32_t ret = client.ClaimInterface(pipe, iface, true);
        printf("normal mode claim ret=%d\n", ret);
        return ret == UEC_OK ? 0 : PROBE_EXIT_FAIL;
    }

    int32_t ret = client.ClaimInterfaceExclusive(pipe, iface, true, callback);
    printf("exclusive claim ret=%d\n", ret);
    if (ret != UEC_OK) {
        return 1;
    }

    std::thread ipcThread([]() { OHOS::IPCSkeleton::JoinWorkThread(); });
    ipcThread.detach();
    printf("ipc work loop started, waiting for conflict claim...\n");

    if (argc > PROBE_MODE_ARG_INDEX && strcmp(argv[PROBE_MODE_ARG_INDEX], "hold") == 0) {
        RunHoldMode();
        return 0;
    }
    return RunSelfTest(client, pipe, iface, busNum, devAddr, ifaceId);
}
