/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "cstdio"
#include "fcntl.h"
#include "parameter.h"
#include "unistd.h"
#include "usb_common_test.h"
#include "usb_ffs.h"
#include "usb_device.h"
#include <bitset>
#include <cerrno>
#include <iostream>
#include <signal.h>
#include <string>

using namespace std;
using namespace OHOS::USB::Common;

static uv_loop_t *g_loop = NULL;

namespace OHOS {
static constexpr int CONFIG_COUNT2 = 2;
static constexpr int CONFIG_COUNT3 = 3;
static constexpr int CONFIG_COUNT5 = 5;

void FillUsbV2Head(UsbFunctionfsDescV2 &descUsbFfs)
{
    descUsbFfs.head.magic = LONG_LE(FUNCTIONFS_DESCRIPTORS_MAGIC_V2);
    descUsbFfs.head.length = LONG_LE(sizeof(descUsbFfs));
    descUsbFfs.head.flags = FUNCTIONFS_HAS_FS_DESC | FUNCTIONFS_HAS_HS_DESC | FUNCTIONFS_HAS_SS_DESC |
        FUNCTIONFS_HAS_MS_OS_DESC | FUNCTIONFS_ALL_CTRL_RECIP;
    descUsbFfs.config1Count = CONFIG_COUNT3;
    descUsbFfs.config2Count = CONFIG_COUNT3;
    descUsbFfs.config3Count = CONFIG_COUNT5;
    descUsbFfs.configWosCount = CONFIG_COUNT2;
    descUsbFfs.config1Desc = config1;
    descUsbFfs.config2Desc = config2;
    descUsbFfs.config3Desc = config3;
    descUsbFfs.wosHead = g_wosHead;
    descUsbFfs.wosDesc = g_wosDesc;
    descUsbFfs.osPropHead = g_osPropHead;
    descUsbFfs.osPropValues = g_osPropValues;
}

static void Main()
{
    struct UsbFunctionfsDescV2 descUsbFfs = {};
    FillUsbV2Head(descUsbFfs);
    printf("sizeof descUsbFfs = %zu\n", sizeof(descUsbFfs));
    string basePath = "/dev/usb-ffs/devtest";
    string ep0Path = basePath + "/ep0";
    string ep1Path = basePath + "/ep1";
    string ep2Path = basePath + "/ep2";
    cout << ep0Path << endl;

    int ret = 0;
    int ep1 = -1;
    int ep2 = -1;
    UsbDevice usbDevice;
    uv_loop_t *loop = NULL;

    int controlEp = open(ep0Path.c_str(), O_RDWR);
    if (controlEp < 0) {
        cout << "open ep0 failed: " << errno << endl;
        return;
    }
    cout << "open ep0 success: " << controlEp << endl;

    if (write(controlEp, &descUsbFfs, sizeof(descUsbFfs)) < 0) {
        cout << "write ffs configs failed failed: " << errno << endl;
        goto CLOSE_EP0;
    }
    cout << "write ffs configs ok" << endl;

    if (write(controlEp, &USB_FFS_VALUE, sizeof(USB_FFS_VALUE)) < 0) {
        cout << "write USB_FFS_VALUE failed: " << errno << endl;
        goto CLOSE_EP0;
    }
    cout << "write USB_FFS_VALUE ok" << endl;

    ep1 = open(ep1Path.c_str(), O_RDWR);
    if (ep1 < 0) {
        cout << "open ep1 failed: " << errno << endl;
        goto CLOSE_EP0;
    }
    cout << "open ep1 success: " << ep1 << endl;

    ep2 = open(ep2Path.c_str(), O_RDWR);
    if (ep2 < 0) {
        cout << "open ep2 failed: " << errno << endl;
        goto CLOSE_EP1;
    }
    cout << "open ep2 success: " << ep2 << endl;

    ret = SetParameter("sys.usb.ffs.devtest.add", "1");
    cout << "SetParameter sys.usb.ffs.devtest.add to 1 return:" << ret << endl;

    usbDevice = UsbDevice(controlEp, ep1, ep2);

    loop = uv_default_loop();
    usbDevice.StartRead(loop);
    cout << "start uv_run" << endl;
    g_loop = loop;
    uv_run(loop, UV_RUN_DEFAULT);
    cout << "end uv_run" << endl;
    uv_loop_close(loop);
    cout << "end uv_loop_close" << endl;

    // CLOSE_EP2:
    close(ep2);
CLOSE_EP1:
    close(ep1);
CLOSE_EP0:
    close(controlEp);
    SetParameter("sys.usb.config", "none");
    sleep(3);
    SetParameter("sys.usb.config", "hdc");
    cout << "exit" << endl;
}

} // namespace OHOS

static void PrintHelp()
{
    printf("device test\n");
}

static void Get_CtrlC_handler(int sig)
{
    cout << "Ctrl-C detected" << endl;
    if (g_loop) {
        cout << "uv_stop start" << endl;
        uv_stop(g_loop);
        g_loop = NULL;
        cout << "uv_stop end" << endl;
    } else {
        cout << "g_loop is null" << endl;
    }
}

int32_t main(int32_t argc, char *argv[])
{
    signal(SIGINT, Get_CtrlC_handler);
    UsbCommonTest::GrantPermissionSysNative();
    PrintHelp();
    OHOS::Main();
    return 0;
}
