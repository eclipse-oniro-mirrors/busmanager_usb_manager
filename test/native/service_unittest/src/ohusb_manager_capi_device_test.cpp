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

#include "ohusb_manager.h"
#include "hilog_wrapper.h"
#include "ohusb_manager_capi_device_test.h"
#include "usb_common_test.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::USB::Common;
using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace USB {
namespace CapiTest {

static constexpr UsbManager_ErrorCode ERR_SUCCESS = USB_MANAGER_SUCCESS;
static constexpr UsbManager_ErrorCode ERR_PERM = USB_MANAGER_ERROR_PERMISSION_DENIED;
static constexpr UsbManager_ErrorCode ERR_SERVICE = USB_MANAGER_ERROR_SERVICE_EXCEPTION;

static bool GetFirstDevice(UsbManager_Device &outDevice)
{
    UsbManager_Device *devices = nullptr;
    uint32_t count = 0;
    UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(&devices, &count);
    if (ret != ERR_SUCCESS || devices == nullptr || count == 0) {
        OH_UsbManager_FreeDeviceList(devices, count);
        return false;
    }
    outDevice = devices[0];
    OH_UsbManager_FreeDeviceList(devices, count);
    return true;
}

void OHUsbManagerDeviceTest::SetUpTestCase(void)
{
    UsbCommonTest::GrantPermissionSysNative();
    USB_HILOGI(MODULE_USB_INNERKIT, "OHUsbManagerDeviceTest::SetUpTestCase");
}

void OHUsbManagerDeviceTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "OHUsbManagerDeviceTest::TearDownTestCase");
}

void OHUsbManagerDeviceTest::SetUp(void) {}

void OHUsbManagerDeviceTest::TearDown(void) {}

HWTEST_F(OHUsbManagerDeviceTest, ConnectDeviceNoPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConnectDeviceNoPermission001 start");
    AccessTokenID hapTokenId = UsbCommonTest::AllocHapTest();
    if (hapTokenId == 0) {
        GTEST_SKIP() << "AllocHapToken failed, skip";
    }
    UsbCommonTest::SetSelfToken(hapTokenId);
    UsbManager_Device device = {};
    if (!GetFirstDevice(device)) {
        UsbCommonTest::GrantPermissionSysNative();
        UsbCommonTest::DeleteAllocHapToken(hapTokenId);
        GTEST_SKIP() << "No USB device, skip";
    }
    UsbManager_DevicePipe pipe = {};
    UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    EXPECT_EQ(ret, ERR_PERM);
    UsbCommonTest::GrantPermissionSysNative();
    UsbCommonTest::DeleteAllocHapToken(hapTokenId);
}

HWTEST_F(OHUsbManagerDeviceTest, GetFileDescriptorNoPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "GetFileDescriptorNoPermission001 start");
    AccessTokenID hapTokenId = UsbCommonTest::AllocHapTest();
    if (hapTokenId == 0) {
        GTEST_SKIP() << "AllocHapToken failed, skip";
    }
    UsbCommonTest::SetSelfToken(hapTokenId);
    UsbManager_Device device = {};
    if (!GetFirstDevice(device)) {
        UsbCommonTest::GrantPermissionSysNative();
        UsbCommonTest::DeleteAllocHapToken(hapTokenId);
        GTEST_SKIP() << "No USB device, skip";
    }
    UsbManager_DevicePipe pipe = {};
    pipe.busNum = device.busNum;
    pipe.devAddress = device.devAddress;
    int32_t fd = -1;
    UsbManager_ErrorCode ret = OH_UsbManager_GetFileDescriptor(&pipe, &fd);
    EXPECT_EQ(ret, ERR_PERM);
    UsbCommonTest::GrantPermissionSysNative();
    UsbCommonTest::DeleteAllocHapToken(hapTokenId);
}

HWTEST_F(OHUsbManagerDeviceTest, ClosePipeNoPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ClosePipeNoPermission001 start");
    AccessTokenID hapTokenId = UsbCommonTest::AllocHapTest();
    if (hapTokenId == 0) {
        GTEST_SKIP() << "AllocHapToken failed, skip";
    }
    UsbCommonTest::SetSelfToken(hapTokenId);
    UsbManager_Device device = {};
    if (!GetFirstDevice(device)) {
        UsbCommonTest::GrantPermissionSysNative();
        UsbCommonTest::DeleteAllocHapToken(hapTokenId);
        GTEST_SKIP() << "No USB device, skip";
    }
    UsbManager_DevicePipe pipe = {};
    pipe.busNum = device.busNum;
    pipe.devAddress = device.devAddress;
    UsbManager_ErrorCode ret = OH_UsbManager_ClosePipe(&pipe);
    EXPECT_EQ(ret, ERR_SERVICE);
    UsbCommonTest::GrantPermissionSysNative();
    UsbCommonTest::DeleteAllocHapToken(hapTokenId);
}

HWTEST_F(OHUsbManagerDeviceTest, ConnectDeviceSysPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConnectDeviceSysPermission001 start");
    UsbManager_Device device = {};
    if (!GetFirstDevice(device)) {
        GTEST_SKIP() << "No USB device, skip";
    }
    UsbManager_DevicePipe pipe = {};
    UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    EXPECT_EQ(ret, ERR_SUCCESS);
    OH_UsbManager_ClosePipe(&pipe);
}

HWTEST_F(OHUsbManagerDeviceTest, GetFileDescriptorSysPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "GetFileDescriptorSysPermission001 start");
    UsbManager_Device device = {};
    if (!GetFirstDevice(device)) {
        GTEST_SKIP() << "No USB device, skip";
    }
    UsbManager_DevicePipe pipe = {};
    UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    ASSERT_EQ(ret, ERR_SUCCESS);
    int32_t fd = -1;
    UsbManager_ErrorCode fdRet = OH_UsbManager_GetFileDescriptor(&pipe, &fd);
    EXPECT_EQ(fdRet, ERR_SUCCESS);
    EXPECT_GE(fd, 0);
    OH_UsbManager_ClosePipe(&pipe);
}

HWTEST_F(OHUsbManagerDeviceTest, ClosePipeSysPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ClosePipeSysPermission001 start");
    UsbManager_Device device = {};
    if (!GetFirstDevice(device)) {
        GTEST_SKIP() << "No USB device, skip";
    }
    UsbManager_DevicePipe pipe = {};
    UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    ASSERT_EQ(ret, ERR_SUCCESS);
    UsbManager_ErrorCode closeRet = OH_UsbManager_ClosePipe(&pipe);
    EXPECT_EQ(closeRet, ERR_SUCCESS);
}

HWTEST_F(OHUsbManagerDeviceTest, HasPermissionSysPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "HasPermissionSysPermission001 start");
    UsbManager_Device device = {};
    if (!GetFirstDevice(device)) {
        GTEST_SKIP() << "No USB device, skip";
    }
    bool result = false;
    UsbManager_ErrorCode ret = OH_UsbManager_HasPermission(device.name, &result);
    EXPECT_EQ(ret, ERR_SUCCESS);
    EXPECT_TRUE(result);
}

HWTEST_F(OHUsbManagerDeviceTest, HasPermissionNoPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "HasPermissionNoPermission001 start");
    AccessTokenID hapTokenId = UsbCommonTest::AllocHapTest();
    if (hapTokenId == 0) {
        GTEST_SKIP() << "AllocHapToken failed, skip";
    }
    UsbCommonTest::SetSelfToken(hapTokenId);
    UsbManager_Device device = {};
    if (!GetFirstDevice(device)) {
        UsbCommonTest::GrantPermissionSysNative();
        UsbCommonTest::DeleteAllocHapToken(hapTokenId);
        GTEST_SKIP() << "No USB device, skip";
    }
    bool result = true;
    UsbManager_ErrorCode ret = OH_UsbManager_HasPermission(device.name, &result);
    EXPECT_EQ(ret, ERR_SUCCESS);
    EXPECT_FALSE(result);
    UsbCommonTest::GrantPermissionSysNative();
    UsbCommonTest::DeleteAllocHapToken(hapTokenId);
}

} // namespace CapiTest
} // namespace USB
} // namespace OHOS
