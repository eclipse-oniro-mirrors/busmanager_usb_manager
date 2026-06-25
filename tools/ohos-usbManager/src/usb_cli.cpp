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
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "usb_srv_client.h"
#include "usb_errors.h"
#include "usb_cli_serialization.h"

#define ARGC_TWO 2

using namespace OHOS::USB;

static const char *TOOL_VERSION = "1.0.0";
static const char *TOOL_NAME = "ohos-usbManager";

static const char *CMD_GET_DEVICE_LIST = "get-device-list";
static const char *CMD_GET_ACCESSORY_LIST = "get-accessory-list";
static const char *CMD_GET_USBSERIAL_LIST = "get-usbserial-list";

static void PrintMainHelp()
{
    std::cout << "Usage: " << TOOL_NAME << " [--help] [--version] <subcommand>" << std::endl;
    std::cout << std::endl;
    std::cout << "Parameters:" << std::endl;
    std::cout << "  --help              Show this help message" << std::endl;
    std::cout << "  --version           Show version information" << std::endl;
    std::cout << "  get-device-list     List all attached USB peripheral devices" << std::endl;
    std::cout << "  get-accessory-list  List all connected USB accessories" << std::endl;
    std::cout << "  get-usbserial-list  List all available USB virtual serial ports" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << TOOL_NAME << " get-device-list" << std::endl;
    std::cout << "  " << TOOL_NAME << " get-accessory-list" << std::endl;
    std::cout << "  " << TOOL_NAME << " get-usbserial-list" << std::endl;
}

static void PrintDeviceListHelp()
{
    std::cout << "Usage: " << TOOL_NAME << " get-device-list [--help] [--version]" << std::endl;
    std::cout << std::endl;
    std::cout << "Parameters:" << std::endl;
    std::cout << "  --help      Show this help message" << std::endl;
    std::cout << "  --version   Show version information" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << TOOL_NAME << " get-device-list" << std::endl;
}

static void PrintAccessoryListHelp()
{
    std::cout << "Usage: " << TOOL_NAME << " get-accessory-list [--help] [--version]" << std::endl;
    std::cout << std::endl;
    std::cout << "Parameters:" << std::endl;
    std::cout << "  --help      Show this help message" << std::endl;
    std::cout << "  --version   Show version information" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << TOOL_NAME << " get-accessory-list" << std::endl;
}

static void PrintSerialListHelp()
{
    std::cout << "Usage: " << TOOL_NAME << " get-usbserial-list [--help] [--version]" << std::endl;
    std::cout << std::endl;
    std::cout << "Parameters:" << std::endl;
    std::cout << "  --help      Show this help message" << std::endl;
    std::cout << "  --version   Show version information" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << TOOL_NAME << " get-usbserial-list" << std::endl;
}

static void PrintVersion()
{
    std::cout << TOOL_NAME << " version " << TOOL_VERSION << std::endl;
}

static bool HasFlag(int argc, char *argv[], const char *flag)
{
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], flag) == 0) {
            return true;
        }
    }
    return false;
}

static int HandleGetDeviceList()
{
    UsbSrvClient &client = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = client.GetDevices(deviceList);
    if (ret != 0) {
        std::cout << FormatError(ret, "Query failed") << std::endl;
        return 1;
    }

    std::vector<cJSON*> items;
    items.reserve(deviceList.size());
    for (auto &device : deviceList) {
        cJSON *item = SerializeDevice(device);
        items.push_back(item);
    }
    std::cout << FormatSuccess(items) << std::endl;
    return 0;
}

static int HandleGetAccessoryList()
{
    UsbSrvClient &client = UsbSrvClient::GetInstance();
    std::vector<USBAccessory> accessList;
    int32_t ret = client.GetAccessoryList(accessList);
    if (ret != 0) {
        std::cout << FormatError(ret, "Query failed") << std::endl;
        return 1;
    }

    std::vector<cJSON*> items;
    items.reserve(accessList.size());
    for (auto &accessory : accessList) {
        cJSON *item = SerializeAccessory(accessory);
        items.push_back(item);
    }
    std::cout << FormatSuccess(items) << std::endl;
    return 0;
}

static int HandleGetSerialList()
{
    UsbSrvClient &client = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = client.SerialGetPortList(serialPortList);
    if (ret != 0) {
        std::cout << FormatError(ret, "Query failed") << std::endl;
        return 1;
    }

    std::vector<cJSON*> items;
    items.reserve(serialPortList.size());
    for (auto &port : serialPortList) {
        cJSON *item = SerializeSerialPort(port);
        items.push_back(item);
    }
    std::cout << FormatSuccess(items) << std::endl;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < ARGC_TWO) {
        std::cout << FormatError(1, "Missing subcommand. Please specify a subcommand to execute.") << std::endl;
        return 1;
    }
    if (HasFlag(argc, argv, "--help")) {
        PrintMainHelp();
        return 0;
    }
    if (HasFlag(argc, argv, "--version")) {
        PrintVersion();
        return 0;
    }

    const char *subcommand = argv[1];

    if (strcmp(subcommand, CMD_GET_DEVICE_LIST) == 0) {
        if (HasFlag(argc, argv, "--help")) {
            PrintDeviceListHelp();
            return 0;
        }
        if (HasFlag(argc, argv, "--version")) {
            PrintVersion();
            return 0;
        }
        return HandleGetDeviceList();
    }
    if (strcmp(subcommand, CMD_GET_ACCESSORY_LIST) == 0) {
        if (HasFlag(argc, argv, "--help")) {
            PrintAccessoryListHelp();
            return 0;
        }
        if (HasFlag(argc, argv, "--version")) {
            PrintVersion();
            return 0;
        }
        return HandleGetAccessoryList();
    }
    if (strcmp(subcommand, CMD_GET_USBSERIAL_LIST) == 0) {
        if (HasFlag(argc, argv, "--help")) {
            PrintSerialListHelp();
            return 0;
        }
        if (HasFlag(argc, argv, "--version")) {
            PrintVersion();
            return 0;
        }
        return HandleGetSerialList();
    }

    std::string errMsg = "Unknown subcommand: ";
    errMsg += subcommand;
    std::cout << FormatError(ARGC_TWO, errMsg) << std::endl;
    return 1;
}
