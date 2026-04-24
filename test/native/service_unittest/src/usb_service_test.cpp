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

#include "usb_service_test.h"
#include <csignal>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>

#include "hilog_wrapper.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "usb_common_test.h"
#include "usb_errors.h"
#include "usb_srv_client.h"
#include "usb_srv_support.h"
#include "usb_right_manager.h"
#include "usb_host_manager.h"
#include "usb_device_manager.h"
#include "usb_port_manager.h"
#include "usb_accessory_manager.h"

using namespace testing::ext;
using namespace OHOS::USB;
using namespace OHOS;
using namespace std;
using namespace OHOS::USB::Common;

namespace OHOS {
namespace USB {
namespace Service {

constexpr uint8_t TEST_BUS_NUM_1 = 1;
constexpr uint8_t TEST_BUS_NUM_2 = 2;
constexpr uint8_t TEST_BUS_NUM_255 = 255;
constexpr uint8_t TEST_DEV_ADDR_1 = 1;
constexpr uint8_t TEST_DEV_ADDR_2 = 2;
constexpr uint8_t TEST_DEV_ADDR_127 = 127;
constexpr uint8_t TEST_DEV_ADDR_255 = 255;
constexpr uint8_t TEST_INTERFACE_ID_0 = 0;
constexpr uint8_t TEST_INTERFACE_ID_1 = 1;
constexpr uint8_t TEST_INTERFACE_ID_2 = 2;
constexpr uint8_t TEST_INTERFACE_ID_3 = 3;
constexpr uint8_t TEST_INTERFACE_ID_4 = 4;
constexpr uint8_t TEST_ALT_INDEX_0 = 0;
constexpr uint8_t TEST_ALT_INDEX_1 = 1;
constexpr uint8_t TEST_CONFIG_INDEX_0 = 0;
constexpr uint8_t TEST_CONFIG_INDEX_1 = 1;
constexpr uint8_t TEST_ENDPOINT_ADDR_1 = 1;
constexpr uint8_t TEST_ENDPOINT_ADDR_2 = 2;
constexpr uint8_t TEST_ENDPOINT_ADDR_81 = 0x81;
constexpr uint8_t TEST_ENDPOINT_ADDR_2 = 0x02;
constexpr int32_t TEST_TIMEOUT_100 = 100;
constexpr int32_t TEST_TIMEOUT_1000 = 1000;
constexpr int32_t TEST_TIMEOUT_5000 = 5000;
constexpr int32_t TEST_VENDOR_ID_0X1234 = 0x1234;
constexpr int32_t TEST_VENDOR_ID_0X5678 = 0x5678;
constexpr int32_t TEST_PRODUCT_ID_0XABCD = 0xABCD;
constexpr int32_t TEST_PRODUCT_ID_0XDCBA = 0xDCBA;
constexpr int32_t TEST_PORT_ID_0 = 0;
constexpr int32_t TEST_PORT_ID_1 = 1;
constexpr int32_t TEST_PORT_ID_2 = 2;
constexpr int32_t TEST_POWER_ROLE_SOURCE = 1;
constexpr int32_t TEST_POWER_ROLE_SINK = 2;
constexpr int32_t TEST_POWER_ROLE_NONE = 0;
constexpr int32_t TEST_DATA_ROLE_HOST = 1;
constexpr int32_t TEST_DATA_ROLE_DEVICE = 2;
constexpr int32_t TEST_DATA_ROLE_NONE = 0;
constexpr int32_t TEST_MODE_DFP = 1;
constexpr int32_t TEST_MODE_UFP = 2;
constexpr int32_t TEST_MODE_DRD = 3;
constexpr int32_t TEST_MODE_NONE = 0;
constexpr uint32_t TEST_BAUD_RATE_1200 = 1200;
constexpr uint32_t TEST_BAUD_RATE_2400 = 2400;
constexpr uint32_t TEST_BAUD_RATE_4800 = 4800;
constexpr uint32_t TEST_BAUD_RATE_9600 = 9600;
constexpr uint32_t TEST_BAUD_RATE_14400 = 14400;
constexpr uint32_t TEST_BAUD_RATE_19200 = 19200;
constexpr uint32_t TEST_BAUD_RATE_28800 = 28800;
constexpr uint32_t TEST_BAUD_RATE_38400 = 38400;
constexpr uint32_t TEST_BAUD_RATE_57600 = 57600;
constexpr uint32_t TEST_BAUD_RATE_115200 = 115200;
constexpr uint32_t TEST_BAUD_RATE_230400 = 230400;
constexpr uint32_t TEST_BAUD_RATE_460800 = 460800;
constexpr uint32_t TEST_BAUD_RATE_921600 = 921600;
constexpr uint32_t TEST_BAUD_RATE_1843200 = 1843200;
constexpr uint8_t TEST_DATA_BITS_5 = 5;
constexpr uint8_t TEST_DATA_BITS_6 = 6;
constexpr uint8_t TEST_DATA_BITS_7 = 7;
constexpr uint8_t TEST_DATA_BITS_8 = 8;
constexpr uint8_t TEST_STOP_BITS_1 = 1;
constexpr uint8_t TEST_STOP_BITS_1_5 = 15;
constexpr uint8_t TEST_STOP_BITS_2 = 2;
constexpr uint8_t TEST_STOP_BITS_2 = 2;
constexpr uint8_t TEST_PARITY_NONE = 0;
constexpr uint8_t TEST_PARITY_ODD = 1;
constexpr uint8_t TEST_PARITY_EVEN = 2;
constexpr uint8_t TEST_PARITY_MARK = 3;
constexpr uint8_t TEST_PARITY_SPACE = 4;
constexpr uint8_t TEST_ENDPOINT_ADDR_1 = 1;
constexpr uint8_t TEST_ENDPOINT_ADDR_2 = 2;
constexpr uint8_t TEST_ENDPOINT_ADDR_81 = 0x81;
constexpr uint8_t TEST_ENDPOINT_ADDR_82 = 0x02;
constexpr uint8_t TEST_FD_VALUE = 10;
constexpr uint32_t TEST_SIZE_64 = 64;
constexpr uint32_t TEST_SIZE_256 = 256;
constexpr uint32_t TEST_SIZE_512 = 512;
constexpr uint32_t TEST_SIZE_1024 = 1024;
constexpr uint32_t TEST_SIZE_4096 = 4096;
constexpr uint32_t TEST_FD_VALUE = 10;

void UsbServiceTest::SetUpTestCase(void)
{
    UsbCommonTest::GrantPermissionSysNative();
    USB_HILOGI(MODULE_USB_SERVICE, "Start UsbServiceTest");
}

void UsbServiceTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "End UsbServiceTest");
}

void UsbServiceTest::SetUp(void) {}

void UsbServiceTest::TearDown(void) {}

#ifdef USB_MANAGER_FEATURE_HOST

HWTEST_F(UsbServiceTest, OpenDevice001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenDevice001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenDevice001");
}

HWTEST_F(UsbServiceTest, OpenDevice002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenDevice002");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_2, TEST_DEV_ADDR_2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_2, TEST_DEV_ADDR_2);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenDevice002");
}

HWTEST_F(UsbServiceTest, OpenDevice003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenDevice003");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_255, TEST_DEV_ADDR_255);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenDevice003");
}

HWTEST_F(UsbServiceTest, Close001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Close001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Close001");
}

HWTEST_F(UsbServiceTest, Close002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Close002");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Close002");
}

HWTEST_F(UsbServiceTest, ResetDevice001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ResetDevice001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ResetDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ResetDevice001");
}

HWTEST_F(UsbServiceTest, ClaimInterface001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClaimInterface001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ClaimInterface(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_0, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClaimInterface001");
}

HWTEST_F(UsbServiceTest, ClaimInterface002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClaimInterface002");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ClaimInterface(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_1, 1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClaimInterface002");
}

HWTEST_F(UsbServiceTest, ClaimInterface003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClaimInterface003");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ClaimInterface(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_2, 1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClaimInterface003");
}

HWTEST_F(UsbServiceTest, ReleaseInterface001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ReleaseInterface001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ClaimInterface(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_0, 0);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ReleaseInterface(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ReleaseInterface001");
}

HWTEST_F(UsbServiceTest, SetInterface001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetInterface001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ClaimInterface(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_0, 0);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.SetInterface(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_0, TEST_ALT_INDEX_1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetInterface001");
}

HWTEST_F(UsbServiceTest, SetActiveConfig001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetActiveConfig001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.SetActiveConfig(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_CONFIG_INDEX_1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetActiveConfig001");
}

HWTEST_F(UsbServiceTest, GetActiveConfig001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetActiveConfig001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    uint8_t configIndex = 0;
    ret = instance.GetActiveConfig(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, configIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, configIndex=%{public}u", ret, configIndex);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetActiveConfig001");
}

HWTEST_F(UsbServiceTest, GetDevices001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDevices001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, deviceCount=%{public}zu", ret, deviceList.size());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDevices001");
}

HWTEST_F(UsbServiceTest, GetDevices002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDevices002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    for (const auto &device : deviceList) {
        USB_HILOGI(MODULE_USB_SERVICE, "Device: busNum=%{public}u, devAddr=%{public}u",
            device.GetBusNum(), device.GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDevices002");
}

HWTEST_F(UsbServiceTest, GetRawDescriptor001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetRawDescriptor001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    std::vector<uint8_t> bufferData;
    ret = instance.GetRawDescriptor(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, bufferSize=%{public}zu", ret, bufferData.size());
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetRawDescriptor001");
}

HWTEST_F(UsbServiceTest, GetFileDescriptor001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetFileDescriptor001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    int32_t fd = -1;
    ret = instance.GetFileDescriptor(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, fd);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, fd=%{public}d", ret, fd);
    ASSERT_GE(ret, UEC_OK);
    ASSERT_GE(fd, 0);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetFileDescriptor001");
}

HWTEST_F(UsbServiceTest, GetDeviceSpeed001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDeviceSpeed001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        uint8_t speed = 0;
        ret = instance.GetDeviceSpeed(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), speed);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, speed=%{public}u", ret, speed);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDeviceSpeed001");
}

HWTEST_F(UsbServiceTest, GetInterfaceActiveStatus001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetInterfaceActiveStatus001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    bool unactivated = false;
    ret = instance.GetInterfaceActiveStatus(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_0, unactivated);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, unactivated=%{public}d", ret, unactivated);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetInterfaceActiveStatus001");
}

HWTEST_F(UsbServiceTest, ManageGlobalInterface001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageGlobalInterface001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ManageGlobalInterface(false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageGlobalInterface001");
}

HWTEST_F(UsbServiceTest, ManageGlobalInterface002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageGlobalInterface002");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ManageGlobalInterface(true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageGlobalInterface002");
}

HWTEST_F(UsbServiceTest, ManageDevice001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ManageDevice(TEST_VENDOR_ID_0X1234, TEST_PRODUCT_ID_0XABCD, false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice001");
}

HWTEST_F(UsbServiceTest, ManageDevice002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice002");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ManageDevice(TEST_VENDOR_ID_0X5678, TEST_PRODUCT_ID_0XDCBA, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice002");
}

HWTEST_F(UsbServiceTest, ManageDevicePolicy001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevicePolicy001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceIdInfo> trustList;
    UsbDeviceIdInfo devInfo;
    devInfo.vendorId = TEST_VENDOR_ID_0X1234;
    devInfo.productId = TEST_PRODUCT_ID_0XABCD;
    trustList.push_back(devInfo);
    int32_t ret = instance.ManageDevicePolicy(trustList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevicePolicy001");
}

HWTEST_F(UsbServiceTest, ManageDevicePolicy002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevicePolicy002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceIdInfo> trustList;
    UsbDeviceIdInfo devInfo1;
    devInfo1.vendorId = TEST_VENDOR_ID_0X1234;
    devInfo1.productId = TEST_PRODUCT_ID_0XABCD;
    trustList.push_back(devInfo1);
    UsbDeviceIdInfo devInfo2;
    devInfo2.vendorId = TEST_VENDOR_ID_0X5678;
    devInfo2.productId = TEST_PRODUCT_ID_0XDCBA;
    trustList.push_back(devInfo2);
    int32_t ret = instance.ManageDevicePolicy(trustList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevicePolicy002");
}

HWTEST_F(UsbServiceTest, ManageInterfaceType001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    UsbDeviceTypeInfo typeInfo;
    typeInfo.baseClass = 0xFF;
    typeInfo.subClass = 0x01;
    typeInfo.protocol = 0x01;
    typeInfo.isDeviceType = true;
    devTypeInfo.push_back(typeInfo);
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType001");
}

HWTEST_F(UsbServiceTest, ManageInterfaceType002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    UsbDeviceTypeInfo typeInfo;
    typeInfo.baseClass = 0x03;
    typeInfo.subClass = 0x01;
    typeInfo.protocol = 0x01;
    typeInfo.isDeviceType = false;
    devTypeInfo.push_back(typeInfo);
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType002");
}

HWTEST_F(UsbServiceTest, UsbAttachKernelDriver001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbAttachKernelDriver001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.UsbAttachKernelDriver(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbAttachKernelDriver001");
}

HWTEST_F(UsbServiceTest, UsbDetachKernelDriver001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDetachKernelDriver001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.UsbDetachKernelDriver(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbDetachKernelDriver001");
}

HWTEST_F(UsbServiceTest, ClearHalt001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClearHalt001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ClearHalt(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_0, TEST_ENDPOINT_ADDR_2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClearHalt001");
}

HWTEST_F(UsbServiceTest, RequestRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestRight001");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = std::to_string(TEST_BUS_NUM_1) + "-" + std::to_string(TEST_DEV_ADDR_1);
    int32_t ret = instance.RequestRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestRight001");
}

HWTEST_F(UsbServiceTest, RemoveRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RemoveRight001");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = std::to_string(TEST_BUS_NUM_1) + "-" + std::to_string(TEST_DEV_ADDR_1);
    int32_t ret = instance.RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RemoveRight001");
}

HWTEST_F(UsbServiceTest, BulkTransferRead001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransferRead001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        UsbBulkTransData bufferData;
        ret = instance.BulkTransferRead(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, bufferData, TEST_TIMEOUT_1000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransferRead001");
}

HWTEST_F(UsbServiceTest, BulkTransferWrite001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransferWrite001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_2);
        UsbBulkTransData bufferData;
        bufferData.data_ = std::vector<uint8_t>(TEST_SIZE_64, 0x55);
        ret = instance.BulkTransferWrite(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, bufferData, TEST_TIMEOUT_1000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransferWrite001");
}

HWTEST_F(UsbServiceTest, ControlTransfer001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ControlTransfer001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        UsbCtlSetUp ctrlParams;
        ctrlParams.reqType = 0x80;
        ctrlParams.reqCmd = 0x06;
        ctrlParams.value = 0x0100;
        ctrlParams.index = 0x0000;
        ctrlParams.length = 18;
        ctrlParams.timeout = TEST_TIMEOUT_1000;
        std::vector<uint8_t> bufferData;
        ret = instance.ControlTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ctrlParams, bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, bufferSize=%{public}zu",
            ret, bufferData.size());
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ControlTransfer001");
}

HWTEST_F(UsbServiceTest, UsbControlTransfer001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbControlTransfer001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        UsbCtlSetUp ctrlParams;
        ctrlParams.reqType = 0x21;
        ctrlParams.reqCmd = 0x09;
        ctrlParams.value = 0x0000;
        ctrlParams.index = 0x0000;
        ctrlParams.length = 0;
        ctrlParams.timeout = TEST_TIMEOUT_1000;
        std::vector<uint8_t> bufferData;
        ret = instance.UsbControlTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ctrlParams, bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbControlTransfer001");
}

HWTEST_F(UsbServiceTest, RequestQueue001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestQueue001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        std::vector<uint8_t> clientData = {0x01, 0x02, 0x03};
        std::vector<uint8_t> bufferData = std::vector<uint8_t>(TEST_SIZE_256, 0);
        ret = instance.RequestQueue(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, clientData, bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestQueue001");
}

HWTEST_F(UsbServiceTest, RequestCancel001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestCancel001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.RequestCancel(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_INTERFACE_ID_0, TEST_ENDPOINT_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestCancel001");
}

HWTEST_F(UsbServiceTest, UsbCancelTransfer001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbCancelTransfer001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.UsbCancelTransfer(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, TEST_ENDPOINT_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    instance.Close(TEST_BUS_NUM_1, TEST_DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbCancelTransfer001");
}

#endif

#ifdef USB_MANAGER_FEATURE_DEVICE

HWTEST_F(UsbServiceTest, GetCurrentFunctions001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetCurrentFunctions001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = 0;
    int32_t ret = instance.GetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcs=%{public}d", ret, funcs);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetCurrentFunctions001");
}

HWTEST_F(UsbServiceTest, SetCurrentFunctions001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_ACM);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions001");
}

HWTEST_F(UsbServiceTest, SetCurrentFunctions002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions002");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_ECM);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions002");
}

HWTEST_F(UsbServiceTest, SetCurrentFunctions003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions003");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_HDC);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions003");
}

HWTEST_F(UsbServiceTest, SetCurrentFunctions004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions004");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_MTP);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions004");
}

HWTEST_F(UsbServiceTest, SetCurrentFunctions005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions005");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_PTP);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions005");
}

HWTEST_F(UsbServiceTest, SetCurrentFunctions006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions006");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_RNDIS);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions006");
}

HWTEST_F(UsbServiceTest, SetCurrentFunctions007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions007");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions007");
}

HWTEST_F(UsbServiceTest, SetCurrentFunctions008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions008");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_NONE);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions008");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString001");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "acm";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString001");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString002");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "ecm";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString002");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString003");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "hdc";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString003");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString004");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "mtp";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString004");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString005");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "ptp";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString005");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString006");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "rndis";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString006");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString007");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "none";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString007");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString008");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "acm,ecm";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString008");
}

HWTEST_F(UsbServiceTest, UsbFunctionsToString001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsToString001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_ACM);
    std::string funcResult;
    int32_t ret = instance.UsbFunctionsToString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}s", ret, funcResult.c_str());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsToString001");
}

HWTEST_F(UsbServiceTest, UsbFunctionsToString002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsToString002");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_ECM);
    std::string funcResult;
    int32_t ret = instance.UsbFunctionsToString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}s", ret, funcResult.c_str());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsToString002");
}

HWTEST_F(UsbServiceTest, UsbFunctionsToString003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsToString003");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_HDC);
    std::string funcResult;
    int32_t ret = instance.UsbFunctionsToString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}s", ret, funcResult.c_str());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsToString003");
}

HWTEST_F(UsbServiceTest, UsbFunctionsToString004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsToString004");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_MTP);
    std::string funcResult;
    int32_t ret = instance.UsbFunctionsToString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}s", ret, funcResult.c_str());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsToString004");
}

HWTEST_F(UsbServiceTest, UsbFunctionsToString005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsToString005");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_PTP);
    std::string funcResult;
    int32_t ret = instance.UsbFunctionsToString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}s", ret, funcResult.c_str());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsToString005");
}

HWTEST_F(UsbServiceTest, UsbFunctionsToString006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsToString006");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_RNDIS);
    std::string funcResult;
    int32_t ret = instance.UsbFunctionsToString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}s", ret, funcResult.c_str());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsToString006");
}

HWTEST_F(UsbServiceTest, UsbFunctionsToString007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsToString007");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_NONE);
    std::string funcResult;
    int32_t ret = instance.UsbFunctionsToString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}s", ret, funcResult.c_str());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsToString007");
}

HWTEST_F(UsbServiceTest, GetAccessoryList001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetAccessoryList001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<USBAccessory> accessList;
    int32_t ret = instance.GetAccessoryList(accessList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, count=%{public}zu", ret, accessList.size());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetAccessoryList001");
}

HWTEST_F(UsbServiceTest, GetAccessoryList002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetAccessoryList002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<USBAccessory> accessList;
    int32_t ret = instance.GetAccessoryList(accessList);
    ASSERT_EQ(0, ret);
    for (const auto &accessory : accessList) {
        USB_HILOGI(MODULE_USB_SERVICE, "Accessory: manufacturer=%{public}s, model=%{public}s",
            accessory.GetManufacturer().c_str(), accessory.GetModel().c_str());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetAccessoryList002");
}

HWTEST_F(UsbServiceTest, OpenAccessory001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenAccessory001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<USBAccessory> accessList;
    int32_t ret = instance.GetAccessoryList(accessList);
    ASSERT_EQ(0, ret);
    if (!accessList.empty()) {
        int32_t fd = -1;
        ret = instance.OpenAccessory(accessList[0], fd);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, fd=%{public}d", ret, fd);
        ASSERT_GE(ret, UEC_OK);
        if (fd >= 0) {
            instance.CloseAccessory(fd);
        }
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenAccessory001");
}

HWTEST_F(UsbServiceTest, CloseAccessory001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : CloseAccessory001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.CloseAccessory(TEST_FD_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : CloseAccessory001");
}

HWTEST_F(UsbServiceTest, AddAccessoryRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : AddAccessoryRight001");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer");
    accessory.SetModel("Test Model");
    accessory.SetDescription("Test Description");
    accessory.SetVersion("1.0");
    accessory.SetUri("http://test.uri");
    accessory.SetSerial("12345");
    uint32_t tokenId = 12345678;
    int32_t ret = instance.AddAccessoryRight(tokenId, accessory);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : AddAccessoryRight001");
}

HWTEST_F(UsbServiceTest, HasAccessoryRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasAccessoryRight001");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer");
    accessory.SetModel("Test Model");
    accessory.SetDescription("Test Description");
    accessory.SetVersion("1.0");
    accessory.SetUri("http://test.uri");
    accessory.SetSerial("12345");
    bool result = false;
    int32_t ret = instance.HasAccessoryRight(accessory, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, result);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasAccessoryRight001");
}

HWTEST_F(UsbServiceTest, RequestAccessoryRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestAccessoryRight001");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer");
    accessory.SetModel("Test Model");
    accessory.SetDescription("Test Description");
    accessory.SetVersion("1.0");
    accessory.SetUri("http://test.uri");
    accessory.SetSerial("12345");
    bool result = false;
    int32_t ret = instance.RequestAccessoryRight(accessory, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, result);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestAccessoryRight001");
}

HWTEST_F(UsbServiceTest, CancelAccessoryRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : CancelAccessoryRight001");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer");
    accessory.SetModel("Test Model");
    accessory.SetDescription("Test Description");
    accessory.SetVersion("1.0");
    accessory.SetUri("http://test.uri");
    accessory.SetSerial("12345");
    int32_t ret = instance.CancelAccessoryRight(accessory);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : CancelAccessoryRight001");
}

#endif

#ifdef USB_MANAGER_FEATURE_PORT

HWTEST_F(UsbServiceTest, GetPorts001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetPorts001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbPort> ports;
    int32_t ret = instance.GetPorts(ports);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, portCount=%{public}zu", ret, ports.size());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetPorts001");
}

HWTEST_F(UsbServiceTest, GetPorts002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetPorts002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbPort> ports;
    int32_t ret = instance.GetPorts(ports);
    ASSERT_EQ(0, ret);
    for (const auto &port : ports) {
        USB_HILOGI(MODULE_USB_SERVICE, "Port: id=%{public}d, supportedModes=%{public}d",
            port.GetId(), port.GetSupportedModes());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetPorts002");
}

HWTEST_F(UsbServiceTest, GetSupportedModes001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModes001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t supportedModes = 0;
    int32_t ret = instance.GetSupportedModes(TEST_PORT_ID_0, supportedModes);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, supportedModes=%{public}d", ret, supportedModes);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetSupportedModes001");
}

HWTEST_F(UsbServiceTest, GetSupportedModes002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModes002");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t supportedModes = 0;
    int32_t ret = instance.GetSupportedModes(TEST_PORT_ID_1, supportedModes);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, supportedModes=%{public}d", ret, supportedModes);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetSupportedModes002");
}

HWTEST_F(UsbServiceTest, SetPortRole001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetPortRole(TEST_PORT_ID_0, TEST_POWER_ROLE_SOURCE, TEST_DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetPortRole001");
}

HWTEST_F(UsbServiceTest, SetPortRole002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole002");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetPortRole(TEST_PORT_ID_0, TEST_POWER_ROLE_SINK, TEST_DATA_ROLE_DEVICE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetPortRole002");
}

HWTEST_F(UsbServiceTest, SetPortRole003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole003");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetPortRole(TEST_PORT_ID_1, TEST_POWER_ROLE_NONE, TEST_DATA_ROLE_NONE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetPortRole003");
}

HWTEST_F(UsbServiceTest, SetPortRole004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole004");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetPortRole(TEST_PORT_ID_2, TEST_POWER_ROLE_SOURCE, TEST_DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetPortRole004");
}

HWTEST_F(UsbServiceTest, SetPortRole005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole005");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetPortRole(TEST_PORT_ID_0, TEST_POWER_ROLE_SINK, TEST_DATA_ROLE_DEVICE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetPortRole005");
}

#endif

HWTEST_F(UsbServiceTest, SerialGetPortList001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetPortList001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, portCount=%{public}zu", ret, serialPortList.size());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetPortList001");
}

HWTEST_F(UsbServiceTest, SerialGetPortList002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetPortList002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    for (const auto &port : serialPortList) {
        USB_HILOGI(MODULE_USB_SERVICE, "SerialPort: portId=%{public}d, busNum=%{public}u, devAddr=%{public}u",
            port.GetPortId(), port.GetBusNum(), port.GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetPortList002");
}

HWTEST_F(UsbServiceTest, SerialOpen001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialOpen001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialOpen001");
}

HWTEST_F(UsbServiceTest, SerialClose001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialClose001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialClose(serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialClose001");
}

HWTEST_F(UsbServiceTest, SerialGetAttribute001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetAttribute001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        ret = instance.SerialGetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, baudRate=%{public}u", ret, attribute.baudRate_);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetAttribute001");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = TEST_BAUD_RATE_115200;
        attribute.dataBits_ = TEST_DATA_BITS_8;
        attribute.stopBits_ = TEST_STOP_BITS_1;
        attribute.parity_ = TEST_PARITY_NONE;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute001");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = TEST_BAUD_RATE_9600;
        attribute.dataBits_ = TEST_DATA_BITS_8;
        attribute.stopBits_ = TEST_STOP_BITS_2;
        attribute.parity_ = TEST_PARITY_ODD;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute002");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = TEST_BAUD_RATE_19200;
        attribute.dataBits_ = TEST_DATA_BITS_7;
        attribute.stopBits_ = TEST_STOP_BITS_1;
        attribute.parity_ = TEST_PARITY_EVEN;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute003");
}

HWTEST_F(UsbServiceTest, SerialRead001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialRead001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data;
        uint32_t actualSize = 0;
        ret = instance.SerialRead(serialPortList[0].GetPortId(), data, TEST_SIZE_256, actualSize, TEST_TIMEOUT_1000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialRead001");
}

HWTEST_F(UsbServiceTest, SerialWrite001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialWrite001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64};
        uint32_t actualSize = 0;
        ret = instance.SerialWrite(serialPortList[0].GetPortId(), data, data.size(), actualSize, TEST_TIMEOUT_1000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialWrite001");
}

HWTEST_F(UsbServiceTest, SerialRead002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialRead002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data;
        uint32_t actualSize = 0;
        ret = instance.SerialRead(serialPortList[0].GetPortId(), data, TEST_SIZE_512, actualSize, TEST_TIMEOUT_5000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialRead002");
}

HWTEST_F(UsbServiceTest, SerialWrite002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialWrite002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data(TEST_SIZE_512, 0x55);
        uint32_t actualSize = 0;
        ret = instance.SerialWrite(serialPortList[0].GetPortId(), data, data.size(), actualSize, TEST_TIMEOUT_5000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialWrite002");
}

HWTEST_F(UsbServiceTest, SerialRead003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialRead003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data;
        uint32_t actualSize = 0;
        ret = instance.SerialRead(serialPortList[0].GetPortId(), data, TEST_SIZE_1024, actualSize, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialRead003");
}

HWTEST_F(UsbServiceTest, SerialWrite003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialWrite003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data(TEST_SIZE_1024, 0xAA);
        uint32_t actualSize = 0;
        ret = instance.SerialWrite(serialPortList[0].GetPortId(), data, data.size(), actualSize, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialWrite003");
}

HWTEST_F(UsbServiceTest, SerialRead004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialRead004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data;
        uint32_t actualSize = 0;
        ret = instance.SerialRead(serialPortList[0].GetPortId(), data, TEST_SIZE_64, actualSize, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialRead004");
}

HWTEST_F(UsbServiceTest, SerialWrite004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialWrite004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data(TEST_SIZE_64, 0x00);
        uint32_t actualSize = 0;
        ret = instance.SerialWrite(serialPortList[0].GetPortId(), data, data.size(), actualSize, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialWrite004");
}

HWTEST_F(UsbServiceTest, SerialRead005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialRead005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data;
        uint32_t actualSize = 0;
        ret = instance.SerialRead(serialPortList[0].GetPortId(), data, TEST_SIZE_4096, actualSize, TEST_TIMEOUT_5000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialRead005");
}

HWTEST_F(UsbServiceTest, SerialWrite005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialWrite005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data(TEST_SIZE_4096, 0xFF);
        uint32_t actualSize = 0;
        ret = instance.SerialWrite(serialPortList[0].GetPortId(), data, data.size(), actualSize, TEST_TIMEOUT_5000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialWrite005");
}

HWTEST_F(UsbServiceTest, RequestSerialRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestSerialRight001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        bool hasRight = false;
        ret = instance.RequestSerialRight(serialPortList[0].GetPortId(), hasRight);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, hasRight);
        ASSERT_EQ(0, ret);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestSerialRight001");
}

HWTEST_F(UsbServiceTest, HasSerialRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasSerialRight001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        bool hasRight = false;
        ret = instance.HasSerialRight(serialPortList[0].GetPortId(), hasRight);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, hasRight);
        ASSERT_EQ(0, ret);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasSerialRight001");
}

HWTEST_F(UsbServiceTest, AddSerialRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : AddSerialRight001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        uint32_t tokenId = 12345678;
        ret = instance.AddSerialRight(tokenId, serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : AddSerialRight001");
}

HWTEST_F(UsbServiceTest, CancelSerialRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : CancelSerialRight001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.CancelSerialRight(serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : CancelSerialRight001");
}

#ifdef USB_MANAGER_FEATURE_HOST

HWTEST_F(UsbServiceTest, BulkTransferReadwithLength001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransferReadwithLength001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        UsbBulkTransData bufferData;
        ret = instance.BulkTransferReadwithLength(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, TEST_SIZE_256, bufferData, TEST_TIMEOUT_1000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransferReadwithLength001");
}

HWTEST_F(UsbServiceTest, BulkTransferReadwithLength002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransferReadwithLength002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        UsbBulkTransData bufferData;
        ret = instance.BulkTransferReadwithLength(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, TEST_SIZE_512, bufferData, TEST_TIMEOUT_5000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransferReadwithLength002");
}

HWTEST_F(UsbServiceTest, BulkTransferReadwithLength003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransferReadwithLength003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        UsbBulkTransData bufferData;
        ret = instance.BulkTransferReadwithLength(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, TEST_SIZE_1024, bufferData, TEST_TIMEOUT_5000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransferReadwithLength003");
}

HWTEST_F(UsbServiceTest, UsbSubmitTransfer001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbSubmitTransfer001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        UsbTransInfo transInfo;
        transInfo.endpoint = TEST_ENDPOINT_ADDR_81;
        transInfo.type = 2;
        transInfo.flags = 0;
        transInfo.timeOut = TEST_TIMEOUT_1000;
        transInfo.length = TEST_SIZE_256;
        transInfo.userData = 0;
        transInfo.numIsoPackets = 0;
        sptr<IRemoteObject> cb = nullptr;
        int fd = -1;
        int memSize = 512;
        ret = instance.UsbSubmitTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            transInfo, cb, fd, memSize);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbSubmitTransfer001");
}

HWTEST_F(UsbServiceTest, UsbSubmitTransfer002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbSubmitTransfer002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        UsbTransInfo transInfo;
        transInfo.endpoint = TEST_ENDPOINT_ADDR_2;
        transInfo.type = 2;
        transInfo.flags = 0;
        transInfo.timeOut = TEST_TIMEOUT_1000;
        transInfo.length = TEST_SIZE_512;
        transInfo.userData = 0;
        transInfo.numIsoPackets = 0;
        sptr<IRemoteObject> cb = nullptr;
        int fd = -1;
        int memSize = 1024;
        ret = instance.UsbSubmitTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            transInfo, cb, fd, memSize);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbSubmitTransfer002");
}

HWTEST_F(UsbServiceTest, UsbSubmitTransfer003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbSubmitTransfer003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        UsbTransInfo transInfo;
        transInfo.endpoint = TEST_ENDPOINT_ADDR_81;
        transInfo.type = 3;
        transInfo.flags = 0;
        transInfo.timeOut = TEST_TIMEOUT_5000;
        transInfo.length = TEST_SIZE_64;
        transInfo.userData = 0;
        transInfo.numIsoPackets = 0;
        sptr<IRemoteObject> cb = nullptr;
        int fd = -1;
        int memSize = 256;
        ret = instance.UsbSubmitTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            transInfo, cb, fd, memSize);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbSubmitTransfer003");
}

HWTEST_F(UsbServiceTest, RegBulkCallback001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RegBulkCallback001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        sptr<IRemoteObject> cb = nullptr;
        ret = instance.RegBulkCallback(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep, cb);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RegBulkCallback001");
}

HWTEST_F(UsbServiceTest, UnRegBulkCallback001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UnRegBulkCallback001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        ret = instance.UnRegBulkCallback(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UnRegBulkCallback001");
}

HWTEST_F(UsbServiceTest, BulkRead001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkRead001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        int fd = -1;
        int memSize = 512;
        ret = instance.BulkRead(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep, fd, memSize);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkRead001");
}

HWTEST_F(UsbServiceTest, BulkWrite002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkWrite002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_2);
        int fd = -1;
        int memSize = 1024;
        ret = instance.BulkWrite(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep, fd, memSize);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkWrite002");
}

HWTEST_F(UsbServiceTest, BulkCancel001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkCancel001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        ret = instance.BulkCancel(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkCancel001");
}

HWTEST_F(UsbServiceTest, BulkCancel002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkCancel002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_1);
        ep.SetAddress(TEST_ENDPOINT_ADDR_2);
        ret = instance.BulkCancel(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkCancel002");
}

HWTEST_F(UsbServiceTest, HasRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasRight001");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = std::to_string(TEST_BUS_NUM_1) + "-" + std::to_string(TEST_DEV_ADDR_1);
    bool hasRight = false;
    int32_t ret = instance.HasRight(deviceName, hasRight);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, hasRight);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasRight001");
}

HWTEST_F(UsbServiceTest, HasRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasRight002");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = std::to_string(TEST_BUS_NUM_2) + "-" + std::to_string(TEST_DEV_ADDR_2);
    bool hasRight = false;
    int32_t ret = instance.HasRight(deviceName, hasRight);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, hasRight);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasRight002");
}

HWTEST_F(UsbServiceTest, AddAccessRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : AddAccessRight001");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = std::to_string(TEST_BUS_NUM_1) + "-" + std::to_string(TEST_DEV_ADDR_1);
    std::string tokenId = "12345678";
    int32_t ret = instance.AddAccessRight(tokenId, deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : AddAccessRight001");
}

HWTEST_F(UsbServiceTest, AddAccessRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : AddAccessRight002");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = std::to_string(TEST_BUS_NUM_2) + "-" + std::to_string(TEST_DEV_ADDR_2);
    std::string tokenId = "87654321";
    int32_t ret = instance.AddAccessRight(tokenId, deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : AddAccessRight002");
}

#endif

#ifdef USB_MANAGER_FEATURE_HOST

HWTEST_F(UsbServiceTest, OpenDevice004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenDevice004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenDevice004");
}

HWTEST_F(UsbServiceTest, Close003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Close003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Close003");
}

HWTEST_F(UsbServiceTest, ResetDevice002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ResetDevice002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ResetDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ResetDevice002");
}

HWTEST_F(UsbServiceTest, ClaimInterface004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClaimInterface004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ClaimInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_INTERFACE_ID_0, 0);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClaimInterface004");
}

HWTEST_F(UsbServiceTest, ReleaseInterface002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ReleaseInterface002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ClaimInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_INTERFACE_ID_0, 0);
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ReleaseInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_INTERFACE_ID_0);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ReleaseInterface002");
}

HWTEST_F(UsbServiceTest, SetInterface002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetInterface002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ClaimInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_INTERFACE_ID_0, 0);
        ASSERT_GE(ret, UEC_OK);
        ret = instance.SetInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_0, TEST_ALT_INDEX_0);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetInterface002");
}

HWTEST_F(UsbServiceTest, SetActiveConfig002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetActiveConfig002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.SetActiveConfig(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_CONFIG_INDEX_0);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetActiveConfig002");
}

HWTEST_F(UsbServiceTest, GetActiveConfig002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetActiveConfig002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        uint8_t configIndex = 0;
        ret = instance.GetActiveConfig(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), configIndex);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, configIndex=%{public}u", ret, configIndex);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetActiveConfig002");
}

HWTEST_F(UsbServiceTest, GetRawDescriptor002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetRawDescriptor002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> bufferData;
        ret = instance.GetRawDescriptor(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, bufferSize=%{public}zu",
            ret, bufferData.size());
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetRawDescriptor002");
}

HWTEST_F(UsbServiceTest, GetFileDescriptor002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetFileDescriptor002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        int32_t fd = -1;
        ret = instance.GetFileDescriptor(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), fd);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, fd=%{public}d", ret, fd);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetFileDescriptor002");
}

HWTEST_F(UsbServiceTest, GetDeviceSpeed002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDeviceSpeed002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (deviceList.size() > 1) {
        ret = instance.OpenDevice(deviceList[1].GetBusNum(), deviceList[1].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        uint8_t speed = 0;
        ret = instance.GetDeviceSpeed(deviceList[1].GetBusNum(), deviceList[1].GetDevAddr(), speed);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, speed=%{public}u", ret, speed);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[1].GetBusNum(), deviceList[1].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDeviceSpeed002");
}

HWTEST_F(UsbServiceTest, GetInterfaceActiveStatus002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetInterfaceActiveStatus002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        bool unactivated = false;
        ret = instance.GetInterfaceActiveStatus(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_1, unactivated);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, unactivated=%{public}d", ret, unactivated);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetInterfaceActiveStatus002");
}

HWTEST_F(UsbServiceTest, ManageGlobalInterface003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageGlobalInterface003");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ManageGlobalInterface(false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ManageGlobalInterface(true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageGlobalInterface003");
}

HWTEST_F(UsbServiceTest, ManageDevice003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice003");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ManageDevice(TEST_VENDOR_ID_0X1234, TEST_PRODUCT_ID_0XABCD, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ManageDevice(TEST_VENDOR_ID_0X1234, TEST_PRODUCT_ID_0XABCD, false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice003");
}

HWTEST_F(UsbServiceTest, ManageDevicePolicy003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevicePolicy003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceIdInfo> trustList;
    UsbDeviceIdInfo devInfo1;
    devInfo1.vendorId = 0x1234;
    devInfo1.productId = 0xABCD;
    trustList.push_back(devInfo1);
    UsbDeviceIdInfo devInfo2;
    devInfo2.vendorId = 0x5678;
    devInfo2.productId = 0xDCBA;
    trustList.push_back(devInfo2);
    UsbDeviceIdInfo devInfo3;
    devInfo3.vendorId = 0xAAAA;
    devInfo3.productId = 0xBBBB;
    trustList.push_back(devInfo3);
    int32_t ret = instance.ManageDevicePolicy(trustList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevicePolicy003");
}

HWTEST_F(UsbServiceTest, ManageInterfaceType003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    UsbDeviceTypeInfo typeInfo1;
    typeInfo1.baseClass = 0x03;
    typeInfo1.subClass = 0x01;
    typeInfo1.protocol = 0x01;
    typeInfo1.isDeviceType = false;
    devTypeInfo.push_back(typeInfo1);
    UsbDeviceTypeInfo typeInfo2;
    typeInfo2.baseClass = 0x08;
    typeInfo2.subClass = 0x06;
    typeInfo2.protocol = 0x50;
    typeInfo2.isDeviceType = false;
    devTypeInfo.push_back(typeInfo2);
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType003");
}

HWTEST_F(UsbServiceTest, UsbAttachKernelDriver002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbAttachKernelDriver002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.UsbAttachKernelDriver(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_1);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbAttachKernelDriver002");
}

HWTEST_F(UsbServiceTest, UsbDetachKernelDriver002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDetachKernelDriver002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.UsbDetachKernelDriver(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_1);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbDetachKernelDriver002");
}

HWTEST_F(UsbServiceTest, ClearHalt002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClearHalt002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ClearHalt(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_1, TEST_ENDPOINT_ADDR_81);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClearHalt002");
}

HWTEST_F(UsbServiceTest, RequestRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestRight002");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = std::to_string(TEST_BUS_NUM_2) + "-" + std::to_string(TEST_DEV_ADDR_2);
    int32_t ret = instance.RequestRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestRight002");
}

HWTEST_F(UsbServiceTest, RemoveRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RemoveRight002");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = std::to_string(TEST_BUS_NUM_2) + "-" + std::to_string(TEST_DEV_ADDR_2);
    int32_t ret = instance.RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RemoveRight002");
}

HWTEST_F(UsbServiceTest, BulkTransferRead002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransferRead002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        UsbBulkTransData bufferData;
        ret = instance.BulkTransferRead(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, bufferData, TEST_TIMEOUT_5000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransferRead002");
}

HWTEST_F(UsbServiceTest, BulkTransferWrite002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransferWrite002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_2);
        UsbBulkTransData bufferData;
        bufferData.data_ = std::vector<uint8_t>(TEST_SIZE_256, 0xAA);
        ret = instance.BulkTransferWrite(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, bufferData, TEST_TIMEOUT_5000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransferWrite002");
}

HWTEST_F(UsbServiceTest, ControlTransfer002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ControlTransfer002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        UsbCtlSetUp ctrlParams;
        ctrlParams.reqType = 0x21;
        ctrlParams.reqCmd = 0x01;
        ctrlParams.value = 0x0000;
        ctrlParams.index = 0x0000;
        ctrlParams.length = 0;
        ctrlParams.timeout = TEST_TIMEOUT_1000;
        std::vector<uint8_t> bufferData;
        ret = instance.ControlTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ctrlParams, bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ControlTransfer002");
}

HWTEST_F(UsbServiceTest, UsbControlTransfer002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbControlTransfer002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        UsbCtlSetUp ctrlParams;
        ctrlParams.reqType = 0xA1;
        ctrlParams.reqCmd = 0x01;
        ctrlParams.value = 0x0000;
        ctrlParams.index = 0x0000;
        ctrlParams.length = 64;
        ctrlParams.timeout = TEST_TIMEOUT_1000;
        std::vector<uint8_t> bufferData;
        ret = instance.UsbControlTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ctrlParams, bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbControlTransfer002");
}

HWTEST_F(UsbServiceTest, RequestQueue002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestQueue002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_2);
        std::vector<uint8_t> clientData = {0x04, 0x05, 0x06};
        std::vector<uint8_t> bufferData = std::vector<uint8_t>(TEST_SIZE_512, 0);
        ret = instance.RequestQueue(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, clientData, bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestQueue002");
}

HWTEST_F(UsbServiceTest, RequestCancel002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestCancel002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.RequestCancel(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_1, TEST_ENDPOINT_ADDR_2);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestCancel002");
}

HWTEST_F(UsbServiceTest, UsbCancelTransfer002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbCancelTransfer002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.UsbCancelTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_ENDPOINT_ADDR_2);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbCancelTransfer002");
}

#endif

HWTEST_F(UsbServiceTest, ManageDevice004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice004");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ManageDevice(0xFFFF, 0xFFFF, false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice004");
}

HWTEST_F(UsbServiceTest, ManageDevicePolicy004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevicePolicy004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceIdInfo> trustList;
    UsbDeviceIdInfo devInfo1;
    devInfo1.vendorId = 0x0000;
    devInfo1.productId = 0x0000;
    trustList.push_back(devInfo1);
    int32_t ret = instance.ManageDevicePolicy(trustList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevicePolicy004");
}

HWTEST_F(UsbServiceTest, ManageInterfaceType004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    UsbDeviceTypeInfo typeInfo;
    typeInfo.baseClass = 0xFF;
    typeInfo.subClass = 0xFF;
    typeInfo.protocol = 0xFF;
    typeInfo.isDeviceType = true;
    devTypeInfo.push_back(typeInfo);
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType004");
}

HWTEST_F(UsbServiceTest, ManageInterfaceType005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType005");
}

#ifdef USB_MANAGER_FEATURE_HOST

HWTEST_F(UsbServiceTest, OpenDevice005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenDevice005");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(0, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenDevice005");
}

HWTEST_F(UsbServiceTest, OpenDevice006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenDevice006");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(0xFF, 0xFF);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenDevice006");
}

HWTEST_F(UsbServiceTest, Close004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Close004");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.Close(0, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Close004");
}

HWTEST_F(UsbServiceTest, Close005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Close005");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.Close(0xFF, 0xFF);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Close005");
}

HWTEST_F(UsbServiceTest, ResetDevice003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ResetDevice003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ResetDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ResetDevice003");
}

HWTEST_F(UsbServiceTest, ClaimInterface005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClaimInterface005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ClaimInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_INTERFACE_ID_2, 1);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClaimInterface005");
}

HWTEST_F(UsbServiceTest, ClaimInterface006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClaimInterface006");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ClaimInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_INTERFACE_ID_3, 0);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClaimInterface006");
}

HWTEST_F(UsbServiceTest, ReleaseInterface003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ReleaseInterface003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ClaimInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_INTERFACE_ID_1, 1);
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ReleaseInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_INTERFACE_ID_1);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ReleaseInterface003");
}

HWTEST_F(UsbServiceTest, SetInterface003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetInterface003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ClaimInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_INTERFACE_ID_1, 1);
        ASSERT_GE(ret, UEC_OK);
        ret = instance.SetInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_1, TEST_ALT_INDEX_0);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetInterface003");
}

HWTEST_F(UsbServiceTest, SetActiveConfig003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetActiveConfig003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.SetActiveConfig(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_CONFIG_INDEX_0);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetActiveConfig003");
}

HWTEST_F(UsbServiceTest, GetActiveConfig003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetActiveConfig003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (deviceList.size() >= 1) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        uint8_t configIndex = 0;
        ret = instance.GetActiveConfig(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), configIndex);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, configIndex=%{public}u", ret, configIndex);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetActiveConfig003");
}

HWTEST_F(UsbServiceTest, GetRawDescriptor003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetRawDescriptor003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (deviceList.size() >= 1) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> bufferData;
        ret = instance.GetRawDescriptor(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetRawDescriptor003");
}

HWTEST_F(UsbServiceTest, GetFileDescriptor003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetFileDescriptor003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (deviceList.size() >= 1) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        int32_t fd = -1;
        ret = instance.GetFileDescriptor(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), fd);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetFileDescriptor003");
}

HWTEST_F(UsbServiceTest, GetDeviceSpeed003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDeviceSpeed003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (deviceList.size() >= 2) {
        ret = instance.OpenDevice(deviceList[1].GetBusNum(), deviceList[1].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        uint8_t speed = 0;
        ret = instance.GetDeviceSpeed(deviceList[1].GetBusNum(), deviceList[1].GetDevAddr(), speed);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[1].GetBusNum(), deviceList[1].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDeviceSpeed003");
}

HWTEST_F(UsbServiceTest, GetInterfaceActiveStatus003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetInterfaceActiveStatus003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        bool unactivated = false;
        ret = instance.GetInterfaceActiveStatus(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_2, unactivated);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetInterfaceActiveStatus003");
}

HWTEST_F(UsbServiceTest, ManageGlobalInterface004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageGlobalInterface004");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret1 = instance.ManageGlobalInterface(false);
    int32_t ret2 = instance.ManageGlobalInterface(true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret1=%{public}d, ret2=%{public}d", ret1, ret2);
    ASSERT_GE(ret1, UEC_OK);
    ASSERT_GE(ret2, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageGlobalInterface004");
}

HWTEST_F(UsbServiceTest, ManageDevice005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice005");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ManageDevice(0x1234, 0xABCD, false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ManageDevice(0x1234, 0xABCD, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice005");
}

HWTEST_F(UsbServiceTest, UsbAttachKernelDriver003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbAttachKernelDriver003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.UsbAttachKernelDriver(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_1);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbAttachKernelDriver003");
}

HWTEST_F(UsbServiceTest, UsbDetachKernelDriver003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDetachKernelDriver003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.UsbDetachKernelDriver(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_1);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbDetachKernelDriver003");
}

HWTEST_F(UsbServiceTest, ClearHalt003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClearHalt003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ClearHalt(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_1, TEST_ENDPOINT_ADDR_2);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClearHalt003");
}

HWTEST_F(UsbServiceTest, BulkTransferRead003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransferRead003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        UsbBulkTransData bufferData;
        ret = instance.BulkTransferRead(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, bufferData, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransferRead003");
}

HWTEST_F(UsbServiceTest, BulkTransferWrite003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransferWrite003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_2);
        UsbBulkTransData bufferData;
        bufferData.data_ = std::vector<uint8_t>(TEST_SIZE_1024, 0x77);
        ret = instance.BulkTransferWrite(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, bufferData, TEST_TIMEOUT_5000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransferWrite003");
}

HWTEST_F(UsbServiceTest, ControlTransfer003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ControlTransfer003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        UsbCtlSetUp ctrlParams;
        ctrlParams.reqType = 0x00;
        ctrlParams.reqCmd = 0x09;
        ctrlParams.value = 0x0001;
        ctrlParams.index = 0x0000;
        ctrlParams.length = 0;
        ctrlParams.timeout = TEST_TIMEOUT_100;
        std::vector<uint8_t> bufferData;
        ret = instance.ControlTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ctrlParams, bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ControlTransfer003");
}

HWTEST_F(UsbServiceTest, UsbControlTransfer003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbControlTransfer003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        UsbCtlSetUp ctrlParams;
        ctrlParams.reqType = 0xA1;
        ctrlParams.reqCmd = 0x03;
        ctrlParams.value = 0x0000;
        ctrlParams.index = 0x0000;
        ctrlParams.length = 18;
        ctrlParams.timeout = TEST_TIMEOUT_100;
        std::vector<uint8_t> bufferData;
        ret = instance.UsbControlTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ctrlParams, bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbControlTransfer003");
}

HWTEST_F(UsbServiceTest, RequestQueue003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestQueue003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_1);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        std::vector<uint8_t> clientData = {0x07, 0x08, 0x09};
        std::vector<uint8_t> bufferData = std::vector<uint8_t>(TEST_SIZE_1024, 0);
        ret = instance.RequestQueue(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, clientData, bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestQueue003");
}

HWTEST_F(UsbServiceTest, RequestWait001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestWait001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        std::vector<uint8_t> clientData = {0x01, 0x02, 0x03};
        ret = instance.RequestQueue(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, clientData, clientData);
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> bufferData;
        ret = instance.RequestWait(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_TIMEOUT_1000, clientData, bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestWait001");
}

HWTEST_F(UsbServiceTest, RequestCancel003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestCancel003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.RequestCancel(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            TEST_INTERFACE_ID_2, TEST_ENDPOINT_ADDR_1);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestCancel003");
}

HWTEST_F(UsbServiceTest, UsbCancelTransfer003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbCancelTransfer003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.UsbCancelTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_ENDPOINT_ADDR_81);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbCancelTransfer003");
}

HWTEST_F(UsbServiceTest, RegBulkCallback002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RegBulkCallback002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_1);
        ep.SetAddress(TEST_ENDPOINT_ADDR_2);
        sptr<IRemoteObject> cb = nullptr;
        ret = instance.RegBulkCallback(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep, cb);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RegBulkCallback002");
}

HWTEST_F(UsbServiceTest, UnRegBulkCallback002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UnRegBulkCallback002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_1);
        ep.SetAddress(TEST_ENDPOINT_ADDR_2);
        ret = instance.UnRegBulkCallback(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UnRegBulkCallback002");
}

HWTEST_F(UsbServiceTest, BulkRead003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkRead003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_1);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        int fd = -1;
        int memSize = 1024;
        ret = instance.BulkRead(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep, fd, memSize);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkRead003");
}

HWTEST_F(UsbServiceTest, BulkWrite003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkWrite003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_1);
        ep.SetAddress(TEST_ENDPOINT_ADDR_2);
        int fd = -1;
        int memSize = 2048;
        ret = instance.BulkWrite(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep, fd, memSize);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkWrite003");
}

HWTEST_F(UsbServiceTest, BulkCancel003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkCancel003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_1);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        ret = instance.BulkCancel(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkCancel003");
}

HWTEST_F(UsbServiceTest, BulkCancel004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkCancel004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_2);
        ep.SetAddress(TEST_ENDPOINT_ADDR_2);
        ret = instance.BulkCancel(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkCancel004");
}

HWTEST_F(UsbServiceTest, RequestRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestRight003");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = "1-1";
    int32_t ret = instance.RequestRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestRight003");
}

HWTEST_F(UsbServiceTest, RemoveRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RemoveRight003");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = "2-2";
    int32_t ret = instance.RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RemoveRight003");
}

HWTEST_F(UsbServiceTest, HasRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasRight003");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = "1-1";
    bool hasRight = false;
    int32_t ret = instance.HasRight(deviceName, hasRight);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, hasRight);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasRight003");
}

HWTEST_F(UsbServiceTest, HasRight004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasRight004");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = "255-255";
    bool hasRight = false;
    int32_t ret = instance.HasRight(deviceName, hasRight);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasRight004");
}

HWTEST_F(UsbServiceTest, AddAccessRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : AddAccessRight003");
    auto &instance = UsbSrvClient::GetInstance();
    std::string deviceName = "1-1";
    std::string tokenId = "99999999";
    int32_t ret = instance.AddAccessRight(tokenId, deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : AddAccessRight003");
}

HWTEST_F(UsbServiceTest, GetDevices003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDevices003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, count=%{public}zu", ret, deviceList.size());
    ASSERT_EQ(0, ret);
    for (size_t i = 0; i < deviceList.size() && i < 5; i++) {
        USB_HILOGI(MODULE_USB_SERVICE, "Device[%zu]: busNum=%{public}u, devAddr=%{public}u",
            i, deviceList[i].GetBusNum(), deviceList[i].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDevices003");
}

#endif

#ifdef USB_MANAGER_FEATURE_PORT

HWTEST_F(UsbServiceTest, GetSupportedModes003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModes003");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t supportedModes = 0;
    int32_t ret = instance.GetSupportedModes(TEST_PORT_ID_1, supportedModes);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, supportedModes=%{public}d", ret, supportedModes);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetSupportedModes003");
}

HWTEST_F(UsbServiceTest, GetSupportedModes004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetSupportedModes004");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t supportedModes = 0;
    int32_t ret = instance.GetSupportedModes(255, supportedModes);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetSupportedModes004");
}

HWTEST_F(UsbServiceTest, SetPortRole006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole006");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetPortRole(TEST_PORT_ID_0, TEST_POWER_ROLE_SOURCE, TEST_DATA_ROLE_DEVICE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetPortRole006");
}

HWTEST_F(UsbServiceTest, SetPortRole007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole007");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetPortRole(TEST_PORT_ID_1, TEST_POWER_ROLE_SINK, TEST_DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetPortRole007");
}

HWTEST_F(UsbServiceTest, SetPortRole008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetPortRole008");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetPortRole(255, TEST_POWER_ROLE_NONE, TEST_DATA_ROLE_NONE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetPortRole008");
}

HWTEST_F(UsbServiceTest, GetPorts003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetPorts003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbPort> ports;
    int32_t ret = instance.GetPorts(ports);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, portCount=%{public}zu", ret, ports.size());
    ASSERT_EQ(0, ret);
    for (size_t i = 0; i < ports.size() && i < 3; i++) {
        USB_HILOGI(MODULE_USB_SERVICE, "Port[%zu]: id=%{public}d, powerRole=%{public}d, dataRole=%{public}d",
            i, ports[i].GetId(), ports[i].GetPowerRole(), ports[i].GetDataRole());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetPorts003");
}

#endif

HWTEST_F(UsbServiceTest, SerialSetAttribute004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 230400;
        attribute.dataBits_ = 8;
        attribute.stopBits_ = 1;
        attribute.parity_ = 0;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute004");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 460800;
        attribute.dataBits_ = 8;
        attribute.stopBits_ = 1;
        attribute.parity_ = 0;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute005");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute006");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 57600;
        attribute.dataBits_ = 8;
        attribute.stopBits_ = 2;
        attribute.parity_ = 0;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute006");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute007");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 115200;
        attribute.dataBits_ = 7;
        attribute.stopBits_ = 1;
        attribute.parity_ = 1;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute007");
}

HWTEST_F(UsbServiceTest, SerialRead006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialRead006");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data;
        uint32_t actualSize = 0;
        ret = instance.SerialRead(serialPortList[0].GetPortId(), data, 128, actualSize, TEST_TIMEOUT_5000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialRead006");
}

HWTEST_F(UsbServiceTest, SerialWrite006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialWrite006");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data(128, 0xBB);
        uint32_t actualSize = 0;
        ret = instance.SerialWrite(serialPortList[0].GetPortId(), data, data.size(), actualSize, TEST_TIMEOUT_5000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialWrite006");
}

HWTEST_F(UsbServiceTest, SerialRead007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialRead007");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data;
        uint32_t actualSize = 0;
        ret = instance.SerialRead(serialPortList[0].GetPortId(), data, 2048, actualSize, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialRead007");
}

HWTEST_F(UsbServiceTest, SerialWrite007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialWrite007");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data(2048, 0xCC);
        uint32_t actualSize = 0;
        ret = instance.SerialWrite(serialPortList[0].GetPortId(), data, data.size(), actualSize, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, actualSize=%{public}u", ret, actualSize);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialWrite007");
}

HWTEST_F(UsbServiceTest, SerialGetAttribute002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetAttribute002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        ret = instance.SerialGetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, baudRate=%{public}u", ret, attribute.baudRate_);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetAttribute002");
}

HWTEST_F(UsbServiceTest, SerialGetAttribute003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetAttribute003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        UsbSerialAttr attribute;
        ret = instance.SerialGetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, dataBits=%{public}u", ret, attribute.dataBits_);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetAttribute003");
}

HWTEST_F(UsbServiceTest, SerialGetAttribute004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetAttribute004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        UsbSerialAttr attribute;
        ret = instance.SerialGetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, stopBits=%{public}u", ret, attribute.stopBits_);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetAttribute004");
}

HWTEST_F(UsbServiceTest, SerialGetAttribute005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetAttribute005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        UsbSerialAttr attribute;
        ret = instance.SerialGetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, parity=%{public}u", ret, attribute.parity_);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetAttribute005");
}

HWTEST_F(UsbServiceTest, RequestSerialRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestSerialRight002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        bool hasRight = false;
        ret = instance.RequestSerialRight(serialPortList[0].GetPortId(), hasRight);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, hasRight);
        ASSERT_EQ(0, ret);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestSerialRight002");
}

HWTEST_F(UsbServiceTest, RequestSerialRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestSerialRight003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        bool hasRight = false;
        ret = instance.RequestSerialRight(serialPortList[0].GetPortId(), hasRight);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_EQ(0, ret);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestSerialRight003");
}

HWTEST_F(UsbServiceTest, HasSerialRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasSerialRight002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        bool hasRight = false;
        ret = instance.HasSerialRight(serialPortList[0].GetPortId(), hasRight);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, hasRight);
        ASSERT_EQ(0, ret);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasSerialRight002");
}

HWTEST_F(UsbServiceTest, HasSerialRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasSerialRight003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        bool hasRight = false;
        ret = instance.HasSerialRight(serialPortList[0].GetPortId(), hasRight);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_EQ(0, ret);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasSerialRight003");
}

HWTEST_F(UsbServiceTest, AddSerialRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : AddSerialRight002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        uint32_t tokenId = 98765432;
        ret = instance.AddSerialRight(tokenId, serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : AddSerialRight002");
}

HWTEST_F(UsbServiceTest, AddSerialRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : AddSerialRight003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        uint32_t tokenId = 11111111;
        ret = instance.AddSerialRight(tokenId, serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : AddSerialRight003");
}

HWTEST_F(UsbServiceTest, CancelSerialRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : CancelSerialRight002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.CancelSerialRight(serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : CancelSerialRight002");
}

HWTEST_F(UsbServiceTest, CancelSerialRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : CancelSerialRight003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.size() >= 2) {
        ret = instance.SerialOpen(serialPortList[1].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.CancelSerialRight(serialPortList[1].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[1].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : CancelSerialRight003");
}

HWTEST_F(UsbServiceTest, SerialOpen002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialOpen002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (serialPortList.size() >= 2) {
        ret = instance.SerialOpen(serialPortList[1].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[1].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialOpen002");
}

HWTEST_F(UsbServiceTest, SerialClose002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialClose002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (serialPortList.size() >= 2) {
        ret = instance.SerialOpen(serialPortList[1].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.SerialClose(serialPortList[1].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialClose002");
}

#ifdef USB_MANAGER_FEATURE_DEVICE

HWTEST_F(UsbServiceTest, SetCurrentFunctions009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions009");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_MTP);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions009");
}

HWTEST_F(UsbServiceTest, SetCurrentFunctions010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions010");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_PTP | UsbSrvSupport::FUNCTION_RNDIS);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions010");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString009");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "acm,mtp";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString009");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString010");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "ptp,rndis";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString010");
}

HWTEST_F(UsbServiceTest, UsbFunctionsToString008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsToString008");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_HDC);
    std::string funcResult;
    int32_t ret = instance.UsbFunctionsToString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}s", ret, funcResult.c_str());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsToString008");
}

HWTEST_F(UsbServiceTest, UsbFunctionsToString009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsToString009");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_ECM | UsbSrvSupport::FUNCTION_MTP);
    std::string funcResult;
    int32_t ret = instance.UsbFunctionsToString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}s", ret, funcResult.c_str());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsToString009");
}

HWTEST_F(UsbServiceTest, OpenAccessory002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenAccessory002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<USBAccessory> accessList;
    int32_t ret = instance.GetAccessoryList(accessList);
    ASSERT_EQ(0, ret);
    if (accessList.size() >= 2) {
        int32_t fd = -1;
        ret = instance.OpenAccessory(accessList[1], fd);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, fd=%{public}d", ret, fd);
        ASSERT_GE(ret, UEC_OK);
        if (fd >= 0) {
            instance.CloseAccessory(fd);
        }
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenAccessory002");
}

HWTEST_F(UsbServiceTest, CloseAccessory002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : CloseAccessory002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<USBAccessory> accessList;
    int32_t ret = instance.GetAccessoryList(accessList);
    ASSERT_EQ(0, ret);
    if (!accessList.empty()) {
        int32_t fd = 100;
        ret = instance.CloseAccessory(fd);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : CloseAccessory002");
}

HWTEST_F(UsbServiceTest, AddAccessoryRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : AddAccessoryRight002");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer2");
    accessory.SetModel("Test Model2");
    accessory.SetDescription("Test Description2");
    accessory.SetVersion("2.0");
    accessory.SetUri("http://test2.uri");
    accessory.SetSerial("67890");
    uint32_t tokenId = 23456789;
    int32_t ret = instance.AddAccessoryRight(tokenId, accessory);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : AddAccessoryRight002");
}

HWTEST_F(UsbServiceTest, HasAccessoryRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasAccessoryRight002");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer2");
    accessory.SetModel("Test Model2");
    accessory.SetDescription("Test Description2");
    accessory.SetVersion("2.0");
    accessory.SetUri("http://test2.uri");
    accessory.SetSerial("67890");
    bool result = false;
    int32_t ret = instance.HasAccessoryRight(accessory, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, result);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasAccessoryRight002");
}

HWTEST_F(UsbServiceTest, RequestAccessoryRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestAccessoryRight002");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer2");
    accessory.SetModel("Test Model2");
    accessory.SetDescription("Test Description2");
    accessory.SetVersion("2.0");
    accessory.SetUri("http://test2.uri");
    accessory.SetSerial("67890");
    bool result = false;
    int32_t ret = instance.RequestAccessoryRight(accessory, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, result);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestAccessoryRight002");
}

HWTEST_F(UsbServiceTest, CancelAccessoryRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : CancelAccessoryRight002");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer2");
    accessory.SetModel("Test Model2");
    accessory.SetDescription("Test Description2");
    accessory.SetVersion("2.0");
    accessory.SetUri("http://test2.uri");
    accessory.SetSerial("67890");
    int32_t ret = instance.CancelAccessoryRight(accessory);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : CancelAccessoryRight002");
}

HWTEST_F(UsbServiceTest, GetAccessoryList003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetAccessoryList003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<USBAccessory> accessList;
    int32_t ret = instance.GetAccessoryList(accessList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, count=%{public}zu", ret, accessList.size());
    ASSERT_EQ(0, ret);
    for (size_t i = 0; i < accessList.size() && i < 3; i++) {
        USB_HILOGI(MODULE_USB_SERVICE, "Accessory[%zu]: manufacturer=%{public}s",
            i, accessList[i].GetManufacturer().c_str());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetAccessoryList003");
}

#endif

HWTEST_F(UsbServiceTest, SerialGetPortList003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetPortList003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, portCount=%{public}zu", ret, serialPortList.size());
    ASSERT_EQ(0, ret);
    for (size_t i = 0; i < serialPortList.size() && i < 3; i++) {
        USB_HILOGI(MODULE_USB_SERVICE, "SerialPort[%zu]: portId=%{public}d", i, serialPortList[i].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetPortList003");
}

HWTEST_F(UsbServiceTest, SerialGetPortList004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetPortList004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    for (size_t i = 0; i < serialPortList.size(); i++) {
        USB_HILOGI(MODULE_USB_SERVICE, "SerialPort[%zu]: vid=%{public}u, pid=%{public}u",
            i, serialPortList[i].GetVid(), serialPortList[i].GetPid());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetPortList004");
}

HWTEST_F(UsbServiceTest, SerialClose003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialClose003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialClose(serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        ret = instance.SerialClose(serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialClose003");
}

HWTEST_F(UsbServiceTest, ManageGlobalInterface005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageGlobalInterface005");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ManageGlobalInterface(true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageGlobalInterface005");
}

#ifdef USB_MANAGER_FEATURE_HOST

HWTEST_F(UsbServiceTest, BulkTransferReadwithLength004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransferReadwithLength004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_1);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        UsbBulkTransData bufferData;
        ret = instance.BulkTransferReadwithLength(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            ep, TEST_SIZE_1024, bufferData, TEST_TIMEOUT_1000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransferReadwithLength004");
}

HWTEST_F(UsbServiceTest, UsbSubmitTransfer004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbSubmitTransfer004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        UsbTransInfo transInfo;
        transInfo.endpoint = TEST_ENDPOINT_ADDR_2;
        transInfo.type = 1;
        transInfo.flags = 0;
        transInfo.timeOut = TEST_TIMEOUT_5000;
        transInfo.length = TEST_SIZE_64;
        transInfo.userData = 0;
        transInfo.numIsoPackets = 0;
        sptr<IRemoteObject> cb = nullptr;
        int fd = -1;
        int memSize = 256;
        ret = instance.UsbSubmitTransfer(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(),
            transInfo, cb, fd, memSize);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbSubmitTransfer004");
}

HWTEST_F(UsbServiceTest, RegBulkCallback003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RegBulkCallback003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_2);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        sptr<IRemoteObject> cb = nullptr;
        ret = instance.RegBulkCallback(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep, cb);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RegBulkCallback003");
}

HWTEST_F(UsbServiceTest, UnRegBulkCallback003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UnRegBulkCallback003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.ClaimInterface(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), TEST_INTERFACE_ID_0, 0);
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_0);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        ret = instance.UnRegBulkCallback(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UnRegBulkCallback003");
}

HWTEST_F(UsbServiceTest, BulkRead004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkRead004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_2);
        ep.SetAddress(TEST_ENDPOINT_ADDR_81);
        int fd = -1;
        int memSize = 2048;
        ret = instance.BulkRead(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep, fd, memSize);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkRead004");
}

HWTEST_F(UsbServiceTest, BulkWrite004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkWrite004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(0, ret);
    if (!deviceList.empty()) {
        ret = instance.OpenDevice(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
        ASSERT_GE(ret, UEC_OK);
        USBEndpoint ep;
        ep.SetInterfaceId(TEST_INTERFACE_ID_2);
        ep.SetAddress(TEST_ENDPOINT_ADDR_2);
        int fd = -1;
        int memSize = 4096;
        ret = instance.BulkWrite(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr(), ep, fd, memSize);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.Close(deviceList[0].GetBusNum(), deviceList[0].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkWrite004");
}

HWTEST_F(UsbServiceTest, ManageInterfaceType006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType006");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    UsbDeviceTypeInfo typeInfo1;
    typeInfo1.baseClass = 0x01;
    typeInfo1.subClass = 0x01;
    typeInfo1.protocol = 0x01;
    typeInfo1.isDeviceType = true;
    devTypeInfo.push_back(typeInfo1);
    UsbDeviceTypeInfo typeInfo2;
    typeInfo2.baseClass = 0x02;
    typeInfo2.subClass = 0x02;
    typeInfo2.protocol = 0x02;
    typeInfo2.isDeviceType = true;
    devTypeInfo.push_back(typeInfo2);
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType006");
}

HWTEST_F(UsbServiceTest, ManageInterfaceType007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType007");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    ret = instance.ManageInterfaceType(devTypeInfo, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType007");
}

#endif

HWTEST_F(UsbServiceTest, SerialRead008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialRead008");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data;
        uint32_t actualSize = 0;
        ret = instance.SerialRead(serialPortList[0].GetPortId(), data, 256, actualSize, TEST_TIMEOUT_10000);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialRead008");
}

HWTEST_F(UsbServiceTest, SerialRead009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialRead009");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data;
        uint32_t actualSize = 0;
        ret = instance.SerialRead(serialPortList[0].GetPortId(), data, 512, actualSize, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialRead009");
}

HWTEST_F(UsbServiceTest, SerialWrite008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialWrite008");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data(256, 0xDD);
        uint32_t actualSize = 0;
        ret = instance.SerialWrite(serialPortList[0].GetPortId(), data, data.size(), actualSize, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialWrite008");
}

HWTEST_F(UsbServiceTest, SerialWrite009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialWrite009");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data(512, 0xEE);
        uint32_t actualSize = 0;
        ret = instance.SerialWrite(serialPortList[0].GetPortId(), data, data.size(), actualSize, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialWrite009");
}

HWTEST_F(UsbServiceTest, SerialRead010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialRead010");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data;
        uint32_t actualSize = 0;
        ret = instance.SerialRead(serialPortList[0].GetPortId(), data, 1024, actualSize, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialRead010");
}

HWTEST_F(UsbServiceTest, SerialWrite010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialWrite010");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        std::vector<uint8_t> data(1024, 0xCC);
        uint32_t actualSize = 0;
        ret = instance.SerialWrite(serialPortList[0].GetPortId(), data, data.size(), actualSize, TEST_TIMEOUT_100);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialWrite010");
}

HWTEST_F(UsbServiceTest, SerialGetAttribute006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetAttribute006");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        UsbSerialAttr attribute;
        ret = instance.SerialGetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetAttribute006");
}

HWTEST_F(UsbServiceTest, SerialGetAttribute007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetAttribute007");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        UsbSerialAttr attribute;
        ret = instance.SerialGetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, ret");
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetAttribute007");
}

HWTEST_F(UsbServiceTest, SerialGetAttribute008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetAttribute008");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        UsbSerialAttr attribute;
        ret = instance.SerialGetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetAttribute008");
}

HWTEST_F(UsbServiceTest, SerialGetAttribute009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetAttribute009");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        UsbSerialAttr attribute;
        ret = instance.SerialGetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetAttribute009");
}

HWTEST_F(UsbServiceTest, SerialGetAttribute010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetAttribute010");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        UsbSerialAttr attribute;
        ret = instance.SerialGetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetAttribute010");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute009");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 1200;
        attribute.dataBits_ = 5;
        attribute.stopBits_ = 2;
        attribute.parity_ = 0;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute009");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute010");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 2400;
        attribute.dataBits_ = 6;
        attribute.stopBits_ = 2;
        attribute.parity_ = 0;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute010");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute011, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute011");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 4800;
        attribute.dataBits_ = 6;
        attribute.stopBits_ = 2;
        attribute.parity_ = 0;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute011");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute012, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute012");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 57600;
        attribute.dataBits_ = 8;
        attribute.stopBits_ = 2;
        attribute.parity_ = 0;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute012");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute013, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute013");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 115200;
        attribute.dataBits_ = 7;
        attribute.stopBits_ = 1;
        attribute.parity = 1;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute013");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute014, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute014");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 230400;
        attribute.dataBits_ = 8;
        attribute.stopBits_ = 1;
        attribute.parity_ = 0;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute014");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute015, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute015");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 460800;
        attribute.dataBits_ = 8;
        attribute.stopBits_ = 2;
        attribute.parity_ = 0;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute015");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute016, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute016");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 921600;
        attribute.dataBits_ = 8;
        attribute.stopBits_ = 1;
        attribute.parity_ = 0;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute016");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute017, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute017");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 115200;
        attribute.dataBits_ = 7;
        attribute.stopBits_ = 2;
        attribute.parity_ = 0;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute017");
}

HWTEST_F(UsbServiceTest, SerialSetAttribute018, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialSetAttribute018");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        UsbSerialAttr attribute;
        attribute.baudRate_ = 230400;
        attribute.dataBits_ = 7;
        attribute.stopBits_ = 1;
        attribute.parity_ = 1;
        ret = instance.SerialSetAttribute(serialPortList[0].GetPortId(), attribute);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialSetAttribute018");
}

HWTEST_F(UsbServiceTest, RequestSerialRight004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestSerialRight004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        bool hasRight = false;
        ret = instance.RequestSerialRight(serialPortList[0].GetPortId(), hasRight);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_EQ(0, ret);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestSerialRight004");
}

HWTEST_F(UsbServiceTest, RequestSerialRight005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestSerialRight005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        bool hasRight = false;
        ret = instance.RequestSerialRight(serialPortList[0].GetPortId(), hasRight);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_EQ(0, ret);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestSerialRight005");
}

HWTEST_F(UsbServiceTest, HasSerialRight004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasSerialRight004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        bool hasRight = false;
        ret = instance.HasSerialRight(serialPortList[0].GetPortId(), hasRight);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_EQ(0, ret);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasSerialRight004");
}

HWTEST_F(UsbServiceTest, HasSerialRight005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasSerialRight005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        bool hasRight = false;
        ret = instance.HasSerialRight(serialPortList[0].GetPortId(), hasRight);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_EQ(0, ret);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasSerialRight005");
}

HWTEST_F(UsbServiceTest, AddSerialRight004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : AddSerialRight004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        uint32_t tokenId = 654321;
        ret = instance.AddSerialRight(tokenId, serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : AddSerialRight004");
}

HWTEST_F(UsbServiceTest, AddSerialRight005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : AddSerialRight005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        uint32_t tokenId = 543210987;
        ret = instance.AddSerialRight(tokenId, serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : AddSerialRight005");
}

HWTEST_F(UsbServiceTest, CancelSerialRight004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : CancelSerialRight004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.CancelSerialRight(serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[0].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : CancelSerialRight004");
}

HWTEST_F(UsbServiceTest, CancelSerialRight005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : CancelSerialRight005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (serialPortList.size() >= 2) {
        ret = instance.SerialOpen(serialPortList[1].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.CancelSerialRight(serialPortList[1].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[1].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : CancelSerialRight005");
}

HWTEST_F(UsbServiceTest, SerialOpen003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialOpen003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (serialPortList.size() >= 2) {
        ret = instance.SerialOpen(serialPortList[1].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        instance.SerialClose(serialPortList[1].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialOpen003");
}

HWTEST_F(UsbServiceTest, SerialClose004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialClose004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (serialPortList.size() >= 2) {
        ret = instance.SerialOpen(serialPortList[1].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.SerialClose(serialPortList[1].GetPortId());
        ret = instance.SerialClose(serialPortList[1].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialClose004");
}

HWTEST_F(UsbServiceTest, SerialClose005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialClose005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.SerialClose(serialPortList[0].GetPortId());
        ret = instance.SerialClose(serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialClose005");
}

HWTEST_F(UsbServiceTest, SerialGetPortList005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetPortList005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, portCount=%{public}zu", ret, serialPortList.size());
    ASSERT_EQ(0, ret);
    for (size_t i = 0; i < serialPortList.size() && i < 3; i++) {
        USB_HILOGI(MODULE_USB_SERVICE, "SerialPort[%zu]: portId=%{public}d",
            i, serialPortList[i].GetPortId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetPortList005");
}

HWTEST_F(UsbServiceTest, SerialGetPortList006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialGetPortList006");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, portCount=%{public}zu", ret, serialPortList.size());
    ASSERT_EQ(0, ret);
    for (size_t i = 0; i < serialPortList.size(); i++) {
        USB_HILOGI(MODULE_USB_SERVICE, "SerialPort[%zu]: vid=%{public}u, pid=%{public}u",
            i, serialPortList[i].GetVid(), serialPortList[i].GetPid());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialGetPortList006");
}

HWTEST_F(UsbServiceTest, SerialClose003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialClose003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (!serialPortList.empty()) {
        ret = instance.SerialOpen(serialPortList[0].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.SerialClose(serialPortList[0].GetPortId());
        ret = instance.SerialClose(serialPortList[0].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialClose003");
}

HWTEST_F(UsbServiceTest, SerialClose004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SerialClose004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPortList;
    int32_t ret = instance.SerialGetPortList(serialPortList);
    ASSERT_EQ(0, ret);
    if (serialPortList.size() >= 2) {
        ret = instance.SerialOpen(serialPortList[1].GetPortId());
        ASSERT_GE(ret, UEC_OK);
        ret = instance.SerialClose(serialPortList[1].GetPortId());
        ret = instance.SerialClose(serialPortList[1].GetPortId());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SerialClose004");
}

#ifdef USB_MANAGER_FEATURE_DEVICE

HWTEST_F(UsbServiceTest, SetCurrentFunctions011, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions011");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(
        UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_HDC);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions011");
}

HWTEST_F(UsbServiceTest, SetCurrentFunctions012, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetCurrentFunctions012");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_PTP | UsbSrvSupport::FUNCTION_RNDIS);
    int32_t ret = instance.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetCurrentFunctions012");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString011, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString011");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "acm,mtp,hdc";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString011");
}

HWTEST_F(UsbServiceTest, UsbFunctionsFromString012, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsFromString012");
    auto &instance = UsbSrvClient::GetInstance();
    std::string funcs = "ptp,rndis";
    int32_t funcResult = 0;
    int32_t ret = instance.UsbFunctionsFromString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}d", ret, funcResult);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsFromString012");
}

HWTEST_F(UsbServiceTest, UsbFunctionsToString010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsToString010");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(
        UsbSrvSupport::FUNCTION_ACM | UsbSrvSupport::FUNCTION_ECM | UsbSrvSupport::FUNCTION_HDC);
    std::string funcResult;
    int32_t ret = instance.UsbFunctionsToString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}s", ret, funcResult.c_str());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsToString010");
}

HWTEST_F(UsbServiceTest, UsbFunctionsToString011, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbFunctionsToString011");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_MTP | UsbSrvSupport::FUNCTION_PTP);
    std::string funcResult;
    int32_t ret = instance.UsbFunctionsToString(funcs, funcResult);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, funcResult=%{public}s", ret, funcResult.c_str());
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbFunctionsToString011");
}

HWTEST_F(UsbServiceTest, OpenAccessory002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenAccessory002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<USBAccessory> accessList;
    int32_t ret = instance.GetAccessoryList(accessList);
    ASSERT_EQ(0, ret);
    if (accessList.size() >= 2) {
        int32_t fd = -1;
        ret = instance.OpenAccessory(accessList[1], fd);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, fd=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
        if (fd >= 0) {
            instance.CloseAccessory(fd);
        }
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenAccessory002");
}

HWTEST_F(UsbServiceTest, CloseAccessory002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : CloseAccessory002");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.CloseAccessory(100);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : CloseAccessory002");
}

HWTEST_F(UsbServiceTest, AddAccessoryRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : AddAccessoryRight003");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer3");
    accessory.SetModel("Test Model3");
    accessory.SetDescription("Test Description3");
    accessory.SetVersion("3.0");
    accessory.SetUri("http://test3.uri");
    accessory.SetSerial("ABC90");
    uint32_t tokenId = 345678901;
    int32_t ret = instance.AddAccessoryRight(tokenId, accessory);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : AddAccessoryRight003");
}

HWTEST_F(UsbServiceTest, HasAccessoryRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasAccessoryRight003");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer3");
    accessory.SetModel("Test Model3");
    accessory.SetDescription("Test Description3");
    accessory.SetVersion("3.0");
    accessory.SetUri("http://test3.uri");
    accessory.SetSerial("ABC90");
    bool result = false;
    int32_t ret = instance.HasAccessoryRight(accessory, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, result);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasAccessoryRight003");
}

HWTEST_F(UsbServiceTest, RequestAccessoryRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestAccessoryRight003");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer3");
    accessory.SetModel("Test Model3");
    accessory.SetDescription("Test Description3");
    accessory.SetVersion("3.0");
    accessory.SetUri("http://test3.uri");
    accessory.SetSerial("ABC90");
    bool result = false;
    int32_t ret = instance.RequestAccessoryRight(accessory, result);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, hasRight=%{public}d", ret, result);
    ASSERT_EQ(0, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestAccessoryRight003");
}

HWTEST_F(UsbServiceTest, CancelAccessoryRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : CancelAccessoryRight003");
    auto &instance = UsbSrvClient::GetInstance();
    USBAccessory accessory;
    accessory.SetManufacturer("Test Manufacturer3");
    accessory.SetModel("Test Model3");
    accessory.SetDescription("Test Description3");
    accessory.SetVersion("3.0");
    accessory.SetUri("http://test3.uri");
    accessory.SetSerial("ABC90");
    int32_t ret = instance.CancelAccessoryRight(accessory);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d", ret);
    ASSERT_GE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : CancelAccessoryRight003");
}

HWTEST_F(UsbServiceTest, GetAccessoryList004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetAccessoryList004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<USBAccessory> accessList;
    int32_t ret = instance.GetAccessoryList(accessList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTest::ret=%{public}d, count=%{public}zu", ret, accessList.size());
    ASSERT_EQ(0, ret);
    for (size_t i = 0; i < accessList.size() && i < 2; i++) {
        USB_HILOGI(MODULE_USB_SERVICE, "Accessory[%zu]: manufacturer=%{public}s",
            i, accessList[i].GetManufacturer().c_str());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetAccessoryList004");
}

#endif

} // namespace Service
} // namespace USB
} // namespace OHOS
