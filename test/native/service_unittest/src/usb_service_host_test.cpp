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

#include "usb_service_host_test.h"
#include <sys/time.h>
#include <iostream>
#include <vector>
#include "delayed_sp_singleton.h"
#include "hilog_wrapper.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "usb_common_test.h"
#include "usb_srv_client.h"
#include "usb_errors.h"
#include "usb_interface_type.h"
#include "usb_service.h"
#include "usb_host_manager.h"

using namespace testing::ext;
using namespace OHOS::USB;
using namespace OHOS;
using namespace std;
using namespace OHOS::USB::Common;

namespace OHOS {
namespace USB {
namespace Service {

void UsbServiceHostTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Start UsbServiceHostTest");
}

void UsbServiceHostTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "End UsbServiceHostTest");
}

void UsbServiceHostTest::SetUp(void) {}

void UsbServiceHostTest::TearDown(void) {}

#ifdef USB_MANAGER_FEATURE_HOST

HWTEST_F(UsbServiceHostTest, OpenDevice001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenDevice001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(busNum, devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::OpenDevice001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenDevice001");
}

HWTEST_F(UsbServiceHostTest, OpenDevice002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenDevice002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(busNum, devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::OpenDevice002 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenDevice002");
}

HWTEST_F(UsbServiceHostTest, OpenDevice003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : OpenDevice003");
    uint8_t busNum = 255;
    uint8_t devAddr = 255;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.OpenDevice(busNum, devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::OpenDevice003 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : OpenDevice003");
}

HWTEST_F(UsbServiceHostTest, Close001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Close001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.Close(busNum, devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::Close001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Close001");
}

HWTEST_F(UsbServiceHostTest, Close002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Close002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.Close(busNum, devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::Close002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Close002");
}

HWTEST_F(UsbServiceHostTest, Close003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Close003");
    uint8_t busNum = 255;
    uint8_t devAddr = 255;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.Close(busNum, devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::Close003 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Close003");
}

HWTEST_F(UsbServiceHostTest, GetDevices001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDevices001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetDevices001 ret=%{public}d", ret);
    ASSERT_EQ(UEC_OK, ret);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDevices001");
}

HWTEST_F(UsbServiceHostTest, GetDevices002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDevices002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetDevices002 ret=%{public}d, size=%{public}zu",
        ret, deviceList.size());
    ASSERT_EQ(UEC_OK, ret);
    ASSERT_GE(deviceList.size(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDevices002");
}

HWTEST_F(UsbServiceHostTest, GetDeviceInfo001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDeviceInfo001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    auto &instance = UsbSrvClient::GetInstance();
    UsbDevice dev;
    int32_t ret = instance.GetDeviceInfo(busNum, devAddr, dev);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetDeviceInfo001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDeviceInfo001");
}

HWTEST_F(UsbServiceHostTest, GetDeviceInfo002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDeviceInfo002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    auto &instance = UsbSrvClient::GetInstance();
    UsbDevice dev;
    int32_t ret = instance.GetDeviceInfo(busNum, devAddr, dev);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetDeviceInfo002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDeviceInfo002");
}

HWTEST_F(UsbServiceHostTest, GetDeviceInfo003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDeviceInfo003");
    uint8_t busNum = 255;
    uint8_t devAddr = 255;
    auto &instance = UsbSrvClient::GetInstance();
    UsbDevice dev;
    int32_t ret = instance.GetDeviceInfo(busNum, devAddr, dev);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetDeviceInfo003 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDeviceInfo003");
}

HWTEST_F(UsbServiceHostTest, ClaimInterface001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClaimInterface001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t interface = 0;
    uint8_t force = 1;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ClaimInterface(busNum, devAddr, interface, force);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ClaimInterface001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClaimInterface001");
}

HWTEST_F(UsbServiceHostTest, ClaimInterface002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClaimInterface002");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t interface = 255;
    uint8_t force = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ClaimInterface(busNum, devAddr, interface, force);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ClaimInterface002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClaimInterface002");
}

HWTEST_F(UsbServiceHostTest, ReleaseInterface001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ReleaseInterface001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t interface = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ReleaseInterface(busNum, devAddr, interface);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ReleaseInterface001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_INTERFACE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ReleaseInterface001");
}

HWTEST_F(UsbServiceHostTest, ReleaseInterface002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ReleaseInterface002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    uint8_t interface = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ReleaseInterface(busNum, devAddr, interface);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ReleaseInterface002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ReleaseInterface002");
}

HWTEST_F(UsbServiceHostTest, SetInterface001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetInterface001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t interfaceid = 0;
    uint8_t altIndex = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetInterface(busNum, devAddr, interfaceid, altIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::SetInterface001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetInterface001");
}

HWTEST_F(UsbServiceHostTest, SetInterface002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetInterface002");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t interfaceid = 255;
    uint8_t altIndex = 255;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetInterface(busNum, devAddr, interfaceid, altIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::SetInterface002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetInterface002");
}

HWTEST_F(UsbServiceHostTest, SetActiveConfig001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetActiveConfig001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t configIndex = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetActiveConfig(busNum, devAddr, configIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::SetActiveConfig001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetActiveConfig001");
}

HWTEST_F(UsbServiceHostTest, SetActiveConfig002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : SetActiveConfig002");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t configIndex = 255;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.SetActiveConfig(busNum, devAddr, configIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::SetActiveConfig002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : SetActiveConfig002");
}

HWTEST_F(UsbServiceHostTest, GetActiveConfig001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetActiveConfig001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t configIndex = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.GetActiveConfig(busNum, devAddr, configIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetActiveConfig001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetActiveConfig001");
}

HWTEST_F(UsbServiceHostTest, GetActiveConfig002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetActiveConfig002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    uint8_t configIndex = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.GetActiveConfig(busNum, devAddr, configIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetActiveConfig002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetActiveConfig002");
}

HWTEST_F(UsbServiceHostTest, UsbAttachKernelDriver001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbAttachKernelDriver001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t interfaceid = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.UsbAttachKernelDriver(busNum, devAddr, interfaceid);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::UsbAttachKernelDriver001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbAttachKernelDriver001");
}

HWTEST_F(UsbServiceHostTest, UsbDetachKernelDriver001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : UsbDetachKernelDriver001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t interfaceid = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.UsbDetachKernelDriver(busNum, devAddr, interfaceid);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::UsbDetachKernelDriver001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : UsbDetachKernelDriver001");
}

HWTEST_F(UsbServiceHostTest, ClearHalt001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClearHalt001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t interfaceId = 0;
    uint8_t endpointId = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ClearHalt(busNum, devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ClearHalt001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClearHalt001");
}

HWTEST_F(UsbServiceHostTest, ClearHalt002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClearHalt002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    uint8_t interfaceId = 0;
    uint8_t endpointId = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ClearHalt(busNum, devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ClearHalt002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClearHalt002");
}

HWTEST_F(UsbServiceHostTest, ClearHalt003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ClearHalt003");
    uint8_t busNum = 255;
    uint8_t devAddr = 255;
    uint8_t interfaceId = 255;
    uint8_t endpointId = 255;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ClearHalt(busNum, devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ClearHalt003 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ClearHalt003");
}

HWTEST_F(UsbServiceHostTest, GetRawDescriptor001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetRawDescriptor001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    std::vector<uint8_t> bufferData;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.GetRawDescriptor(busNum, devAddr, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetRawDescriptor001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetRawDescriptor001");
}

HWTEST_F(UsbServiceHostTest, GetRawDescriptor002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetRawDescriptor002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    std::vector<uint8_t> bufferData;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.GetRawDescriptor(busNum, devAddr, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetRawDescriptor002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetRawDescriptor002");
}

HWTEST_F(UsbServiceHostTest, GetFileDescriptor001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetFileDescriptor001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    int32_t fd = -1;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.GetFileDescriptor(busNum, devAddr, fd);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetFileDescriptor001 ret=%{public}d, fd=%{public}d", ret, fd);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetFileDescriptor001");
}

HWTEST_F(UsbServiceHostTest, GetFileDescriptor002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetFileDescriptor002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    int32_t fd = -1;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.GetFileDescriptor(busNum, devAddr, fd);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetFileDescriptor002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetFileDescriptor002");
}

HWTEST_F(UsbServiceHostTest, GetDeviceSpeed001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDeviceSpeed001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t speed = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.GetDeviceSpeed(busNum, devAddr, speed);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetDeviceSpeed001 ret=%{public}d, speed=%{public}d",
        ret, speed);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDeviceSpeed001");
}

HWTEST_F(UsbServiceHostTest, GetDeviceSpeed002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDeviceSpeed002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    uint8_t speed = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.GetDeviceSpeed(busNum, devAddr, speed);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetDeviceSpeed002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDeviceSpeed002");
}

HWTEST_F(UsbServiceHostTest, GetInterfaceActiveStatus001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetInterfaceActiveStatus001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    uint8_t interfaceid = 0;
    bool unactivated = false;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.GetInterfaceActiveStatus(busNum, devAddr, interfaceid, unactivated);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetInterfaceActiveStatus001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetInterfaceActiveStatus001");
}

HWTEST_F(UsbServiceHostTest, GetInterfaceActiveStatus002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetInterfaceActiveStatus002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    uint8_t interfaceid = 0;
    bool unactivated = false;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.GetInterfaceActiveStatus(busNum, devAddr, interfaceid, unactivated);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetInterfaceActiveStatus002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetInterfaceActiveStatus002");
}

HWTEST_F(UsbServiceHostTest, GetDeviceProductName001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDeviceProductName001");
    std::string deviceName = "1-2";
    std::string productName;
    auto &instance = UsbSrvClient::GetInstance();
    bool ret = instance.GetDeviceProductName(deviceName, productName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetDeviceProductName001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == true || ret == false);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDeviceProductName001");
}

HWTEST_F(UsbServiceHostTest, GetDeviceProductName002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetDeviceProductName002");
    std::string deviceName = "";
    std::string productName;
    auto &instance = UsbSrvClient::GetInstance();
    bool ret = instance.GetDeviceProductName(deviceName, productName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetDeviceProductName002 ret=%{public}d", ret);
    ASSERT_TRUE(ret == false);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetDeviceProductName002");
}

HWTEST_F(UsbServiceHostTest, GetConfigDescriptor001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GetConfigDescriptor001");
    UsbDevice dev;
    std::vector<uint8_t> descriptor;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.GetConfigDescriptor(dev, descriptor);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GetConfigDescriptor001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GetConfigDescriptor001");
}

HWTEST_F(UsbServiceHostTest, HasRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasRight001");
    std::string deviceName = "1-2";
    auto &instance = UsbSrvClient::GetInstance();
    bool ret = instance.HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::HasRight001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == true || ret == false);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasRight001");
}

HWTEST_F(UsbServiceHostTest, HasRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasRight002");
    std::string deviceName = "";
    auto &instance = UsbSrvClient::GetInstance();
    bool ret = instance.HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::HasRight002 ret=%{public}d", ret);
    ASSERT_TRUE(ret == false);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasRight002");
}

HWTEST_F(UsbServiceHostTest, HasRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HasRight003");
    std::string deviceName = "255-255";
    auto &instance = UsbSrvClient::GetInstance();
    bool ret = instance.HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::HasRight003 ret=%{public}d", ret);
    ASSERT_TRUE(ret == false);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HasRight003");
}

HWTEST_F(UsbServiceHostTest, RequestRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestRight001");
    std::string deviceName = "1-2";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RequestRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RequestRight001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestRight001");
}

HWTEST_F(UsbServiceHostTest, RequestRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestRight002");
    std::string deviceName = "";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RequestRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RequestRight002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestRight002");
}

HWTEST_F(UsbServiceHostTest, RequestRight003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestRight003");
    std::string deviceName = "255-255";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RequestRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RequestRight003 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestRight003");
}

HWTEST_F(UsbServiceHostTest, RemoveRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RemoveRight001");
    std::string deviceName = "1-2";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RemoveRight001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RemoveRight001");
}

HWTEST_F(UsbServiceHostTest, RemoveRight002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RemoveRight002");
    std::string deviceName = "";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RemoveRight002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RemoveRight002");
}

HWTEST_F(UsbServiceHostTest, ResetDevice001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ResetDevice001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ResetDevice(busNum, devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ResetDevice001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_INTERFACE_NAME_NOT_FOUND);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ResetDevice001");
}

HWTEST_F(UsbServiceHostTest, ResetDevice002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ResetDevice002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ResetDevice(busNum, devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ResetDevice002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ResetDevice002");
}

#endif // USB_MANAGER_FEATURE_HOST

HWTEST_F(UsbServiceHostTest, ResetDevice001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ResetDevice001");
    uint8_t busNum = 1;
    uint8_t devAddr = 2;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ResetDevice(busNum, devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ResetDevice001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED || ret == UEC_INTERFACE_NAME_NOT_FOUND);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ResetDevice001");
}

HWTEST_F(UsbServiceHostTest, ResetDevice002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ResetDevice002");
    uint8_t busNum = 0;
    uint8_t devAddr = 0;
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.ResetDevice(busNum, devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ResetDevice002 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ResetDevice002");
}

HWTEST_F(UsbServiceHostTest, BulkTransfer001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransfer001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::BulkTransfer001 ret=%{public}d", ret);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        ASSERT_TRUE(dev.GetBusNum() >= 0);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransfer001");
}

HWTEST_F(UsbServiceHostTest, BulkTransfer002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransfer002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::BulkTransfer002 ret=%{public}d, size=%{public}zu",
        ret, deviceList.size());
    ASSERT_EQ(UEC_OK, ret);
    ASSERT_GE(deviceList.size(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransfer002");
}

HWTEST_F(UsbServiceHostTest, BulkTransfer003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : BulkTransfer003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::BulkTransfer003 devices=%{public}zu", deviceList.size());
    ASSERT_EQ(UEC_OK, ret);
    for (size_t i = 0; i < deviceList.size(); i++) {
        USB_HILOGI(MODULE_USB_SERVICE, "device[%{public}zu] bus=%{public}d addr=%{public}d",
            i, deviceList[i].GetBusNum(), deviceList[i].GetDevAddr());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : BulkTransfer003");
}

HWTEST_F(UsbServiceHostTest, DeviceInfo001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfo001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        uint8_t busNum = dev.GetBusNum();
        uint8_t devAddr = dev.GetDevAddr();
        UsbDevice devInfo;
        ret = instance.GetDeviceInfo(busNum, devAddr, devInfo);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfo001 ret=%{public}d", ret);
        ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfo001");
}

HWTEST_F(UsbServiceHostTest, DeviceInfo002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfo002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        uint8_t busNum = dev.GetBusNum();
        uint8_t devAddr = dev.GetDevAddr();
        UsbDevice devInfo;
        ret = instance.GetDeviceInfo(busNum, devAddr, devInfo);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfo002 ret=%{public}d", ret);
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfo002");
}

HWTEST_F(UsbServiceHostTest, DeviceInfo003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfo003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() == 0) {
        uint8_t busNum = 1;
        uint8_t devAddr = 2;
        UsbDevice devInfo;
        ret = instance.GetDeviceInfo(busNum, devAddr, devInfo);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfo003 ret=%{public}d", ret);
        ASSERT_TRUE(ret != UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfo003");
}

HWTEST_F(UsbServiceHostTest, DeviceInfoDescriptor001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfoDescriptor001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        HDI::Usb::V1_0::UsbDev uDev = {dev.GetBusNum(), dev.GetDevAddr()};
        std::vector<uint8_t> descriptor;
        ret = instance.GetDeviceInfoDescriptor(uDev, descriptor, dev);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfoDescriptor001 ret=%{public}d", ret);
        ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfoDescriptor001");
}

HWTEST_F(UsbServiceHostTest, DeviceInfoDescriptor002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfoDescriptor002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        HDI::Usb::V1_0::UsbDev uDev = {dev.GetBusNum(), dev.GetDevAddr()};
        std::vector<uint8_t> descriptor;
        ret = instance.GetDeviceInfoDescriptor(uDev, descriptor, dev);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfoDescriptor002 ret=%{public}d, size=%{public}zu",
            ret, descriptor.size());
        ASSERT_GE(ret, UEC_OK);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfoDescriptor002");
}

HWTEST_F(UsbServiceHostTest, DeviceInfoDescriptor003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfoDescriptor003");
    auto &instance = UsbSrvClient::GetInstance();
    HDI::Usb::V1_0::UsbDev uDev = {0, 0};
    std::vector<uint8_t> descriptor;
    UsbDevice dev;
    int32_t ret = instance.GetDeviceInfoDescriptor(uDev, descriptor, dev);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfoDescriptor003 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfoDescriptor003");
}

HWTEST_F(UsbServiceHostTest, Permission001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Permission001");
    std::string deviceName = "0-0";
    auto &instance = UsbSrvClient::GetInstance();
    bool ret = instance.HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::Permission001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == false);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Permission001");
}

HWTEST_F(UsbServiceHostTest, Permission002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Permission002");
    std::string deviceName = "1-1";
    auto &instance = UsbSrvClient::GetInstance();
    bool ret = instance.HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::Permission002 ret=%{public}d", ret);
    ASSERT_TRUE(ret == false);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Permission002");
}

HWTEST_F(UsbServiceHostTest, Permission003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : Permission003");
    std::string deviceName = "test-device-name";
    auto &instance = UsbSrvClient::GetInstance();
    bool ret = instance.HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::Permission003 ret=%{public}d", ret);
    ASSERT_TRUE(ret == false);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : Permission003");
}

HWTEST_F(UsbServiceHostTest, RequestRight004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestRight004");
    std::string deviceName = "test-device-123";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RequestRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RequestRight004 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestRight004");
}

HWTEST_F(UsbServiceHostTest, RequestRight005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RequestRight005");
    std::string deviceName = "0-1";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RequestRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RequestRight005 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RequestRight005");
}

HWTEST_F(UsbServiceHostTest, RemoveDeviceRight001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RemoveDeviceRight001");
    std::string deviceName = "test-device";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RemoveDeviceRight001 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RemoveDeviceRight001");
}

HWTEST_F(UsbServiceHostTest, ManageGlobalInterface001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageGlobalInterface001");
    auto &instance = UsbSrvClient::GetInstance();
    bool disable = true;
    int32_t ret = instance.ManageGlobalInterface(disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageGlobalInterface001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageGlobalInterface001");
}

HWTEST_F(UsbServiceHostTest, ManageGlobalInterface002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageGlobalInterface002");
    auto &instance = UsbSrvClient::GetInstance();
    bool disable = false;
    int32_t ret = instance.ManageGlobalInterface(disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageGlobalInterface002 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageGlobalInterface002");
}

HWTEST_F(UsbServiceHostTest, ManageDevice001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice001");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t vendorId = 0x1234;
    int32_t productId = 0x5678;
    bool disable = true;
    int32_t ret = instance.ManageDevice(vendorId, productId, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageDevice001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice001");
}

HWTEST_F(UsbServiceHostTest, ManageDevice002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice002");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t vendorId = 0x0000;
    int32_t productId = 0x0000;
    bool disable = false;
    int32_t ret = instance.ManageDevice(vendorId, productId, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageDevice002 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice002");
}

HWTEST_F(UsbServiceHostTest, ManageDevice003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice003");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t vendorId = -1;
    int32_t productId = -1;
    bool disable = true;
    int32_t ret = instance.ManageDevice(vendorId, productId, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageDevice003 ret=%{public}d", ret);
    ASSERT_TRUE(ret != UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice003");
}

HWTEST_F(UsbServiceHostTest, ManageDevicePolicy001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevicePolicy001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceIdInfo> trustList;
    UsbDeviceIdInfo info;
    info.productId = 0x1234;
    info.vendorId = 0x5678;
    trustList.push_back(info);
    int32_t ret = instance.ManageDevicePolicy(trustList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageDevicePolicy001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevicePolicy001");
}

HWTEST_F(UsbServiceHostTest, ManageDevicePolicy002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevicePolicy002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceIdInfo> trustList;
    int32_t ret = instance.ManageDevicePolicy(trustList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageDevicePolicy002 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevicePolicy002");
}

HWTEST_F(UsbServiceHostTest, ManageInterfaceType001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    UsbDeviceTypeInfo info;
    info.baseClass = 0xFF;
    info.subClass = 0xFF;
    info.protocol = 0xFF;
    info.isDeviceType = true;
    devTypeInfo.push_back(info);
    bool disable = true;
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageInterfaceType001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType001");
}

HWTEST_F(UsbServiceHostTest, ManageInterfaceType002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    bool disable = false;
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageInterfaceType002 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType002");
}

HWTEST_F(UsbServiceHostTest, ManageInterfaceType003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    UsbDeviceTypeInfo info;
    info.baseClass = 0;
    info.subClass = 0;
    info.protocol = 0;
    info.isDeviceType = false;
    devTypeInfo.push_back(info);
    bool disable = true;
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageInterfaceType003 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType003");
}

HWTEST_F(UsbServiceHostTest, ManageInterfaceType003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    UsbDeviceTypeInfo info;
    info.baseClass = 0;
    info.subClass = 0;
    info.protocol = 0;
    info.isDeviceType = false;
    devTypeInfo.push_back(info);
    bool disable = true;
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageInterfaceType003 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType003");
}

HWTEST_F(UsbServiceHostTest, ManageInterfaceType004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    UsbDeviceTypeInfo info1;
    info1.baseClass = 0x08;
    info1.subClass = 0x06;
    info1.protocol = 0x50;
    info1.isDeviceType = true;
    devTypeInfo.push_back(info1);
    UsbDeviceTypeInfo info2;
    info2.baseClass = 0x09;
    info2.subClass = 0x00;
    info2.protocol = 0x00;
    info2.isDeviceType = false;
    devTypeInfo.push_back(info2);
    bool disable = true;
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageInterfaceType004 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType004");
}

HWTEST_F(UsbServiceHostTest, ManageInterfaceType005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    UsbDeviceTypeInfo info;
    info.baseClass = 0xFF;
    info.subClass = 0xFF;
    info.protocol = 0xFF;
    info.isDeviceType = false;
    devTypeInfo.push_back(info);
    bool disable = false;
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageInterfaceType005 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType005");
}

HWTEST_F(UsbServiceHostTest, ManageInterfaceType006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType006");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceTypeInfo> devTypeInfo;
    UsbDeviceTypeInfo info;
    info.baseClass = 0xE0;
    info.subClass = 0x01;
    info.protocol = 0x01;
    info.isDeviceType = true;
    devTypeInfo.push_back(info);
    bool disable = true;
    int32_t ret = instance.ManageInterfaceType(devTypeInfo, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageInterfaceType006 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType006");
}

HWTEST_F(UsbServiceHostTest, ManageDevicePolicy003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevicePolicy003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceIdInfo> trustList;
    UsbDeviceIdInfo info1;
    info1.productId = 0x1000;
    info1.vendorId = 0x2000;
    trustList.push_back(info1);
    UsbDeviceIdInfo info2;
    info2.productId = 0x3000;
    info2.vendorId = 0x4000;
    trustList.push_back(info2);
    int32_t ret = instance.ManageDevicePolicy(trustList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageDevicePolicy003 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevicePolicy003");
}

HWTEST_F(UsbServiceHostTest, ManageDevice004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice004");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t vendorId = 0xFFFF;
    int32_t productId = 0xFFFF;
    bool disable = true;
    int32_t ret = instance.ManageDevice(vendorId, productId, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageDevice004 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice004");
}

HWTEST_F(UsbServiceHostTest, ManageDevice005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice005");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t vendorId = 0x1234;
    int32_t productId = 0x5678;
    bool disable = false;
    int32_t ret = instance.ManageDevice(vendorId, productId, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageDevice005 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice005");
}

HWTEST_F(UsbServiceHostTest, ManageDevice006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice006");
    auto &instance = UsbSrvClient::GetInstance();
    int32_t vendorId = 0x0001;
    int32_t productId = 0x0002;
    bool disable = true;
    int32_t ret = instance.ManageDevice(vendorId, productId, disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::ManageDevice006 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice006");
}

HWTEST_F(UsbServiceHostTest, GlobalInterface001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GlobalInterface001");
    auto &instance = UsbSrvClient::GetInstance();
    bool disable = true;
    int32_t ret = instance.ManageGlobalInterface(disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GlobalInterface001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GlobalInterface001");
}

HWTEST_F(UsbServiceHostTest, GlobalInterface002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : GlobalInterface002");
    auto &instance = UsbSrvClient::GetInstance();
    bool disable = false;
    int32_t ret = instance.ManageGlobalInterface(disable);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::GlobalInterface002 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_OPERATION);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : GlobalInterface002");
}

HWTEST_F(UsbServiceHostTest, RightManagement001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RightManagement001");
    std::string deviceName = "1-2";
    auto &instance = UsbSrvClient::GetInstance();
    bool ret = instance.HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RightManagement001 ret=%{public}d", ret);
    ASSERT_TRUE(ret == true || ret == false);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RightManagement001");
}

HWTEST_F(UsbServiceHostTest, RightManagement002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RightManagement002");
    std::string deviceName = "2-1";
    auto &instance = UsbSrvClient::GetInstance();
    bool ret = instance.HasRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RightManagement002 ret=%{public}d", ret);
    ASSERT_TRUE(ret == true || ret == false);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RightManagement002");
}

HWTEST_F(UsbServiceHostTest, RightManagement003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RightManagement003");
    std::string deviceName = "10-20";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RequestRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RightManagement003 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RightManagement003");
}

HWTEST_F(UsbServiceHostTest, RightManagement004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RightManagement004");
    std::string deviceName = "5-10";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RequestRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RightManagement004 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RightManagement004");
}

HWTEST_F(UsbServiceHostTest, RightManagement005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RightManagement005");
    std::string deviceName = "3-4";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RightManagement005 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RightManagement005");
}

HWTEST_F(UsbServiceHostTest, RightManagement006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : RightManagement006");
    std::string deviceName = "6-7";
    auto &instance = UsbSrvClient::GetInstance();
    int32_t ret = instance.RemoveRight(deviceName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::RightManagement006 ret=%{public}d", ret);
    ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : RightManagement006");
}

HWTEST_F(UsbServiceHostTest, DeviceSearch001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceSearch001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceSearch001 devices=%{public}zu", deviceList.size());
    ASSERT_EQ(UEC_OK, ret);
    for (size_t i = 0; i < deviceList.size(); i++) {
        USB_HILOGI(MODULE_USB_SERVICE, "Device[%{public}zu] vid=%{public}d pid=%{public}d",
            i, deviceList[i].GetVendorId(), deviceList[i].GetProductId());
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceSearch001");
}

HWTEST_F(UsbServiceHostTest, DeviceSearch002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceSearch002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 1) {
        ASSERT_GE(deviceList.size(), 2);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceSearch002 found=%{public}d", deviceList.size() > 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceSearch002");
}

HWTEST_F(UsbServiceHostTest, DeviceSearch003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceSearch003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    int emptyCount = 0;
    for (size_t i = 0; i < deviceList.size(); i++) {
        if (deviceList[i].GetVendorId() == 0 && deviceList[i].GetProductId() == 0) {
            emptyCount++;
        }
    }
    ASSERT_GE(deviceList.size(), emptyCount);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceSearch003");
}

HWTEST_F(UsbServiceHostTest, DeviceInfoExt001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfoExt001");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        uint8_t busNum = dev.GetBusNum();
        uint8_t devAddr = dev.GetDevAddr();
        uint8_t configIndex = 0;
        ret = instance.GetActiveConfig(busNum, devAddr, configIndex);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfoExt001 ret=%{public}d", ret);
        ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfoExt001");
}

HWTEST_F(UsbServiceHostTest, DeviceInfoExt002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfoExt002");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        uint8_t busNum = dev.GetBusNum();
        uint8_t devAddr = dev.GetDevAddr();
        uint8_t speed = 0;
        ret = instance.GetDeviceSpeed(busNum, devAddr, speed);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfoExt002 ret=%{public}d, speed=%{public}d",
            ret, speed);
        ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfoExt002");
}

HWTEST_F(UsbServiceHostTest, DeviceInfoExt003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfoExt003");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        uint8_t busNum = dev.GetBusNum();
        uint8_t devAddr = dev.GetDevAddr();
        std::string productName;
        std::string name = std::to_string(busNum) + "-" + std::to_string(devAddr);
        bool result = instance.GetDeviceProductName(name, productName);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfoExt003 result=%{public}d", result);
        ASSERT_TRUE(result == true || result == false);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfoExt003");
}

HWTEST_F(UsbServiceHostTest, DeviceInfoExt004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfoExt004");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        uint8_t busNum = dev.GetBusNum();
        uint8_t devAddr = dev.GetDevAddr();
        std::vector<uint8_t> bufferData;
        ret = instance.GetRawDescriptor(busNum, devAddr, bufferData);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfoExt004 ret=%{public}d, size=%{public}zu",
            ret, bufferData.size());
        ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfoExt004");
}

HWTEST_F(UsbServiceHostTest, DeviceInfoExt005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfoExt005");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        uint8_t busNum = dev.GetBusNum();
        uint8_t devAddr = dev.GetDevAddr();
        int32_t fd = -1;
        ret = instance.GetFileDescriptor(busNum, devAddr, fd);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfoExt005 ret=%{public}d, fd=%{public}d",
            ret, fd);
        ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_PERMISSION_DENIED);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfoExt005");
}

HWTEST_F(UsbServiceHostTest, DeviceInfoExt006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfoExt006");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        uint8_t busNum = dev.GetBusNum();
        uint8_t devAddr = dev.GetDevAddr();
        uint8_t interfaceid = 0;
        bool unactivated = false;
        ret = instance.GetInterfaceActiveStatus(busNum, devAddr, interfaceid, unactivated);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfoExt006 ret=%{public}d, unactivated=%{public}d",
            ret, unactivated);
        ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfoExt006");
}

HWTEST_F(UsbServiceHostTest, DeviceInfoExt007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfoExt007");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        UsbDevice configDev;
        ret = instance.GetConfigDescriptor(dev, std::vector<uint8_t>());
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfoExt007 ret=%{public}d", ret);
        ASSERT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfoExt007");
}

HWTEST_F(UsbServiceHostTest, DeviceInfoExt008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : DeviceInfoExt008");
    auto &instance = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = instance.GetDevices(deviceList);
    ASSERT_EQ(UEC_OK, ret);
    if (deviceList.size() > 0) {
        UsbDevice &dev = deviceList[0];
        std::string name = std::to_string(dev.GetBusNum()) + "-" + std::to_string(dev.GetDevAddr());
        std::string productName;
        bool result = instance.GetDeviceProductName(name, productName);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceHostTest::DeviceInfoExt008 result=%{public}d, name=%{public}s",
            result, productName.c_str());
        ASSERT_TRUE(result == true || result == false);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : DeviceInfoExt008");
}

} // namespace Service
} // namespace USB
} // namespace OHOS