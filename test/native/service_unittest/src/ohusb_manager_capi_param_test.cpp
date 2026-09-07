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

#include <string>

#include "ohusb_manager.h"
#include "hilog_wrapper.h"
#include "ohusb_manager_capi_param_test.h"
#include "usb_common_test.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::USB::Common;

namespace OHOS {
namespace USB {
namespace CapiTest {

void OHUsbManagerParamTest::SetUpTestCase(void)
{
    UsbCommonTest::GrantPermissionSysNative();
    USB_HILOGI(MODULE_USB_INNERKIT, "OHUsbManagerParamTest::SetUpTestCase");
}

void OHUsbManagerParamTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "OHUsbManagerParamTest::TearDownTestCase");
}

void OHUsbManagerParamTest::SetUp(void) {}

void OHUsbManagerParamTest::TearDown(void) {}

static constexpr UsbManager_ErrorCode ERR_INVALID = USB_MANAGER_ERROR_INVALID_PARAMETER;
static constexpr UsbManager_ErrorCode ERR_SERVICE = USB_MANAGER_ERROR_SERVICE_EXCEPTION;
static constexpr UsbManager_ErrorCode ERR_PERM = USB_MANAGER_ERROR_PERMISSION_DENIED;
static constexpr UsbManager_ErrorCode ERR_SUCCESS = USB_MANAGER_SUCCESS;

HWTEST_F(OHUsbManagerParamTest, GetUsbDeviceListNullDevices001, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "GetUsbDeviceListNullDevices001 start");
    uint32_t count = 0;
    UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(nullptr, &count);
    EXPECT_EQ(ret, ERR_INVALID);
}

HWTEST_F(OHUsbManagerParamTest, GetUsbDeviceListNullCount002, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "GetUsbDeviceListNullCount002 start");
    UsbManager_Device *devices = nullptr;
    UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(&devices, nullptr);
    EXPECT_EQ(ret, ERR_INVALID);
}

HWTEST_F(OHUsbManagerParamTest, FreeDeviceListNullDevices001, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "FreeDeviceListNullDevices001 start");
    OH_UsbManager_FreeDeviceList(nullptr, 1);
    SUCCEED();
}

HWTEST_F(OHUsbManagerParamTest, FreeDeviceListZeroCount002, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "FreeDeviceListZeroCount002 start");
    UsbManager_Device dev = {};
    OH_UsbManager_FreeDeviceList(&dev, 0);
    SUCCEED();
}

HWTEST_F(OHUsbManagerParamTest, ConnectDeviceNullDevice001, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConnectDeviceNullDevice001 start");
    UsbManager_DevicePipe pipe = {};
    UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(nullptr, &pipe);
    EXPECT_EQ(ret, ERR_INVALID);
}

HWTEST_F(OHUsbManagerParamTest, ConnectDeviceNullPipe002, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConnectDeviceNullPipe002 start");
    UsbManager_Device device = {};
    UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, nullptr);
    EXPECT_EQ(ret, ERR_INVALID);
}

HWTEST_F(OHUsbManagerParamTest, HasPermissionNullDeviceName001, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "HasPermissionNullDeviceName001 start");
    bool result = false;
    UsbManager_ErrorCode ret = OH_UsbManager_HasPermission(nullptr, &result);
    EXPECT_EQ(ret, ERR_INVALID);
}

HWTEST_F(OHUsbManagerParamTest, HasPermissionNullResult002, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "HasPermissionNullResult002 start");
    UsbManager_ErrorCode ret = OH_UsbManager_HasPermission("/dev/bus/usb/001/002", nullptr);
    EXPECT_EQ(ret, ERR_INVALID);
}

HWTEST_F(OHUsbManagerParamTest, RequestPermissionNullDeviceName001, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "RequestPermissionNullDeviceName001 start");
    UsbManager_ErrorCode ret = OH_UsbManager_RequestPermission(nullptr, nullptr, nullptr);
    EXPECT_EQ(ret, ERR_INVALID);
}

HWTEST_F(OHUsbManagerParamTest, RequestPermissionNullCallback002, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "RequestPermissionNullCallback002 start");
    UsbManager_ErrorCode ret = OH_UsbManager_RequestPermission("/dev/bus/usb/001/002", nullptr, nullptr);
    EXPECT_EQ(ret, ERR_INVALID);
}

HWTEST_F(OHUsbManagerParamTest, GetFileDescriptorNullPipe001, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "GetFileDescriptorNullPipe001 start");
    int32_t fd = -1;
    UsbManager_ErrorCode ret = OH_UsbManager_GetFileDescriptor(nullptr, &fd);
    EXPECT_EQ(ret, ERR_INVALID);
}

HWTEST_F(OHUsbManagerParamTest, GetFileDescriptorNullFd002, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "GetFileDescriptorNullFd002 start");
    UsbManager_DevicePipe pipe = {};
    UsbManager_ErrorCode ret = OH_UsbManager_GetFileDescriptor(&pipe, nullptr);
    EXPECT_EQ(ret, ERR_INVALID);
}

HWTEST_F(OHUsbManagerParamTest, ClosePipeNullPipe001, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ClosePipeNullPipe001 start");
    UsbManager_ErrorCode ret = OH_UsbManager_ClosePipe(nullptr);
    EXPECT_EQ(ret, ERR_INVALID);
}

HWTEST_F(OHUsbManagerParamTest, HasPermissionInvalidDeviceName001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "HasPermissionInvalidDeviceName001 start");
    bool result = true;
    UsbManager_ErrorCode ret = OH_UsbManager_HasPermission("usb1-1", &result);
    EXPECT_EQ(ret, ERR_SUCCESS);
    EXPECT_FALSE(result);
}

HWTEST_F(OHUsbManagerParamTest, ConnectDeviceInvalidBusNum001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConnectDeviceInvalidBusNum001 start");
    UsbManager_Device device = {};
    device.busNum = 255;
    device.devAddress = 1;
    UsbManager_DevicePipe pipe = {};
    UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    EXPECT_EQ(ret, ERR_PERM);
}

HWTEST_F(OHUsbManagerParamTest, GetFileDescriptorInvalidBusNum001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "GetFileDescriptorInvalidBusNum001 start");
    UsbManager_DevicePipe pipe = {};
    pipe.busNum = 255;
    pipe.devAddress = 255;
    int32_t fd = -1;
    UsbManager_ErrorCode ret = OH_UsbManager_GetFileDescriptor(&pipe, &fd);
    EXPECT_EQ(ret, ERR_PERM);
}

HWTEST_F(OHUsbManagerParamTest, ClosePipeInvalidBusNum001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ClosePipeInvalidBusNum001 start");
    UsbManager_DevicePipe pipe = {};
    pipe.busNum = 255;
    pipe.devAddress = 255;
    UsbManager_ErrorCode ret = OH_UsbManager_ClosePipe(&pipe);
    EXPECT_EQ(ret, ERR_SERVICE);
}

HWTEST_F(OHUsbManagerParamTest, HasPermissionNormalToken001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "HasPermissionNormalToken001 start");
    UsbCommonTest::GrantPermissionNormalNative();
    bool result = true;
    UsbManager_ErrorCode ret = OH_UsbManager_HasPermission("/dev/bus/usb/001/002", &result);
    EXPECT_EQ(ret, ERR_SUCCESS);
    EXPECT_FALSE(result);
    UsbCommonTest::GrantPermissionSysNative();
}

HWTEST_F(OHUsbManagerParamTest, HasPermissionSysToken001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "HasPermissionSysToken001 start");
    UsbCommonTest::GrantPermissionSysNative();
    bool result = false;
    UsbManager_ErrorCode ret = OH_UsbManager_HasPermission("/dev/bus/usb/001/002", &result);
    EXPECT_EQ(ret, ERR_SUCCESS);
}

} // namespace CapiTest
} // namespace USB
} // namespace OHOS
