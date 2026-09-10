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

static constexpr OH_UsbManager_ErrorCode ERR_SUCCESS = OH_USBMANAGER_SUCCESS;
static constexpr OH_UsbManager_ErrorCode ERR_PERM = OH_USBMANAGER_ERROR_PERMISSION_DENIED;
static constexpr OH_UsbManager_ErrorCode ERR_SERVICE = OH_USBMANAGER_ERROR_SERVICE_EXCEPTION;

static bool GetFirstDevice(OH_UsbManager_UsbDevice &outDevice)
{
    OH_UsbManager_UsbDevice *devices = nullptr;
    uint32_t count = 0;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(&devices, &count);
    if (ret != ERR_SUCCESS || devices == nullptr || count == 0) {
        OH_UsbManager_FreeUsbDeviceList(devices, count);
        return false;
    }
    outDevice = devices[0];
    OH_UsbManager_FreeUsbDeviceList(devices, count);
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
    OH_UsbManager_UsbDevice device = {};
    if (!GetFirstDevice(device)) {
        UsbCommonTest::GrantPermissionSysNative();
        UsbCommonTest::DeleteAllocHapToken(hapTokenId);
        GTEST_SKIP() << "No USB device, skip";
    }
    OH_UsbManager_UsbPipe pipe = {};
    OH_UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
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
    OH_UsbManager_UsbDevice device = {};
    if (!GetFirstDevice(device)) {
        UsbCommonTest::GrantPermissionSysNative();
        UsbCommonTest::DeleteAllocHapToken(hapTokenId);
        GTEST_SKIP() << "No USB device, skip";
    }
    OH_UsbManager_UsbPipe pipe = {};
    pipe.busNum = device.busNum;
    pipe.devAddress = device.devAddress;
    int32_t fd = -1;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_GetFileDescriptor(&pipe, &fd);
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
    OH_UsbManager_UsbDevice device = {};
    if (!GetFirstDevice(device)) {
        UsbCommonTest::GrantPermissionSysNative();
        UsbCommonTest::DeleteAllocHapToken(hapTokenId);
        GTEST_SKIP() << "No USB device, skip";
    }
    OH_UsbManager_UsbPipe pipe = {};
    pipe.busNum = device.busNum;
    pipe.devAddress = device.devAddress;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_ClosePipe(&pipe);
    EXPECT_EQ(ret, ERR_SERVICE);
    UsbCommonTest::GrantPermissionSysNative();
    UsbCommonTest::DeleteAllocHapToken(hapTokenId);
}

HWTEST_F(OHUsbManagerDeviceTest, ConnectDeviceSysPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConnectDeviceSysPermission001 start");
    OH_UsbManager_UsbDevice device = {};
    if (!GetFirstDevice(device)) {
        GTEST_SKIP() << "No USB device, skip";
    }
    OH_UsbManager_UsbPipe pipe = {};
    OH_UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    EXPECT_EQ(ret, ERR_SUCCESS);
    OH_UsbManager_ClosePipe(&pipe);
}

HWTEST_F(OHUsbManagerDeviceTest, GetFileDescriptorSysPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "GetFileDescriptorSysPermission001 start");
    OH_UsbManager_UsbDevice device = {};
    if (!GetFirstDevice(device)) {
        GTEST_SKIP() << "No USB device, skip";
    }
    OH_UsbManager_UsbPipe pipe = {};
    OH_UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    ASSERT_EQ(ret, ERR_SUCCESS);
    int32_t fd = -1;
    OH_UsbManager_ErrorCode fdRet = OH_UsbManager_GetFileDescriptor(&pipe, &fd);
    EXPECT_EQ(fdRet, ERR_SUCCESS);
    EXPECT_GE(fd, 0);
    OH_UsbManager_ClosePipe(&pipe);
}

HWTEST_F(OHUsbManagerDeviceTest, ClosePipeSysPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ClosePipeSysPermission001 start");
    OH_UsbManager_UsbDevice device = {};
    if (!GetFirstDevice(device)) {
        GTEST_SKIP() << "No USB device, skip";
    }
    OH_UsbManager_UsbPipe pipe = {};
    OH_UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    ASSERT_EQ(ret, ERR_SUCCESS);
    OH_UsbManager_ErrorCode closeRet = OH_UsbManager_ClosePipe(&pipe);
    EXPECT_EQ(closeRet, ERR_SUCCESS);
}

HWTEST_F(OHUsbManagerDeviceTest, HasPermissionSysPermission001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "HasPermissionSysPermission001 start");
    OH_UsbManager_UsbDevice device = {};
    if (!GetFirstDevice(device)) {
        GTEST_SKIP() << "No USB device, skip";
    }
    bool result = false;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_HasPermission(device.name, &result);
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
    OH_UsbManager_UsbDevice device = {};
    if (!GetFirstDevice(device)) {
        UsbCommonTest::GrantPermissionSysNative();
        UsbCommonTest::DeleteAllocHapToken(hapTokenId);
        GTEST_SKIP() << "No USB device, skip";
    }
    bool result = true;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_HasPermission(device.name, &result);
    EXPECT_EQ(ret, ERR_SUCCESS);
    EXPECT_FALSE(result);
    UsbCommonTest::GrantPermissionSysNative();
    UsbCommonTest::DeleteAllocHapToken(hapTokenId);
}

} // namespace CapiTest
} // namespace USB
} // namespace OHOS
