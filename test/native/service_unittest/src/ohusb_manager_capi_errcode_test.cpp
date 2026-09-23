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

#include "ohusb_manager_capi_errcode_test.h"

// Include the CAPI implementation directly to reach the file-local error conversion helpers.
// The pragma suppresses -Wheader-hygiene for the global using-directive inside the cpp.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wheader-hygiene"
#include "ohusb_manager.cpp"
#pragma clang diagnostic pop

#include "hilog_wrapper.h"

using namespace testing::ext;
using namespace OHOS::USB;

namespace OHOS {
namespace USB {
namespace CapiTest {

void OHUsbManagerErrCodeTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "OHUsbManagerErrCodeTest::SetUpTestCase");
}

void OHUsbManagerErrCodeTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "OHUsbManagerErrCodeTest::TearDownTestCase");
}

void OHUsbManagerErrCodeTest::SetUp(void) {}

void OHUsbManagerErrCodeTest::TearDown(void) {}

static constexpr OH_UsbManager_ErrorCode ERR_SUCCESS = OH_USBMANAGER_SUCCESS;
static constexpr OH_UsbManager_ErrorCode ERR_PERM = OH_USBMANAGER_ERROR_PERMISSION_DENIED;
static constexpr OH_UsbManager_ErrorCode ERR_SERVICE = OH_USBMANAGER_ERROR_SERVICE_EXCEPTION;
static constexpr OH_UsbManager_ErrorCode ERR_NO_DEVICE = OH_USBMANAGER_ERROR_NO_DEVICE;
static constexpr OH_UsbManager_ErrorCode ERR_NO_MEMORY = OH_USBMANAGER_ERROR_NO_MEMORY;
static constexpr OH_UsbManager_ErrorCode ERR_IO = OH_USBMANAGER_ERROR_IO_ERROR;

// Driver-layer HDF status code (hdf_base.h HDF_ERR_TIMEOUT) that the CAPI deliberately does not
// map; used to verify the fallback to SERVICE_EXCEPTION.
constexpr int32_t HDF_ERR_TIMEOUT_CODE = -7;

/**
 * @tc.name: ConvertErrCodeForConnect001
 * @tc.desc: Test ConnectDevice error conversion for driver-layer HDF codes.
 * @tc.type: FUNC
 */
HWTEST_F(OHUsbManagerErrCodeTest, ConvertErrCodeForConnect001, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConvertErrCodeForConnect001 start");
    EXPECT_EQ(ConvertErrCodeForConnect(HDF_FAILURE_CODE), ERR_IO);
    EXPECT_EQ(ConvertErrCodeForConnect(HDF_ERR_IO_CODE), ERR_IO);
    EXPECT_EQ(ConvertErrCodeForConnect(HDF_DEV_ERR_NO_DEVICE_CODE), ERR_IO);
}

/**
 * @tc.name: ConvertErrCodeForConnect002
 * @tc.desc: Test ConnectDevice error conversion for unmapped codes falling back to ConvertErrCode.
 * @tc.type: FUNC
 */
HWTEST_F(OHUsbManagerErrCodeTest, ConvertErrCodeForConnect002, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConvertErrCodeForConnect002 start");
    EXPECT_EQ(ConvertErrCodeForConnect(UEC_OK), ERR_SUCCESS);
    EXPECT_EQ(ConvertErrCodeForConnect(UEC_SERVICE_PERMISSION_DENIED), ERR_PERM);
    EXPECT_EQ(ConvertErrCodeForConnect(UEC_INTERFACE_PERMISSION_DENIED), ERR_PERM);
    EXPECT_EQ(ConvertErrCodeForConnect(UEC_SERVICE_NO_MEMORY), ERR_NO_MEMORY);
    EXPECT_EQ(ConvertErrCodeForConnect(UEC_INTERFACE_NO_MEMORY), ERR_NO_MEMORY);
    EXPECT_EQ(ConvertErrCodeForConnect(UEC_SERVICE_INVALID_VALUE), ERR_SERVICE);
    EXPECT_EQ(ConvertErrCodeForConnect(HDF_ERR_TIMEOUT_CODE), ERR_SERVICE);
}

/**
 * @tc.name: ConvertErrCodeForGetFd001
 * @tc.desc: Test GetFileDescriptor error conversion for driver-layer HDF codes.
 * @tc.type: FUNC
 */
HWTEST_F(OHUsbManagerErrCodeTest, ConvertErrCodeForGetFd001, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConvertErrCodeForGetFd001 start");
    EXPECT_EQ(ConvertErrCodeForGetFd(HDF_DEV_ERR_NO_DEVICE_CODE), ERR_NO_DEVICE);
    EXPECT_EQ(ConvertErrCodeForGetFd(HDF_FAILURE_CODE), ERR_NO_DEVICE);
    EXPECT_EQ(ConvertErrCodeForGetFd(HDF_ERR_IO_CODE), ERR_SERVICE);
}

/**
 * @tc.name: ConvertErrCodeForGetFd002
 * @tc.desc: Test GetFileDescriptor error conversion for unmapped codes falling back to ConvertErrCode.
 * @tc.type: FUNC
 */
HWTEST_F(OHUsbManagerErrCodeTest, ConvertErrCodeForGetFd002, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConvertErrCodeForGetFd002 start");
    EXPECT_EQ(ConvertErrCodeForGetFd(UEC_OK), ERR_SUCCESS);
    EXPECT_EQ(ConvertErrCodeForGetFd(UEC_SERVICE_PERMISSION_DENIED), ERR_PERM);
    EXPECT_EQ(ConvertErrCodeForGetFd(UEC_INTERFACE_PERMISSION_DENIED), ERR_PERM);
    EXPECT_EQ(ConvertErrCodeForGetFd(UEC_SERVICE_NO_MEMORY), ERR_NO_MEMORY);
    EXPECT_EQ(ConvertErrCodeForGetFd(UEC_INTERFACE_NO_MEMORY), ERR_NO_MEMORY);
    EXPECT_EQ(ConvertErrCodeForGetFd(UEC_SERVICE_INVALID_VALUE), ERR_SERVICE);
    EXPECT_EQ(ConvertErrCodeForGetFd(HDF_ERR_TIMEOUT_CODE), ERR_SERVICE);
}

/**
 * @tc.name: ConvertErrCode001
 * @tc.desc: Test the base UEC error code conversion.
 * @tc.type: FUNC
 */
HWTEST_F(OHUsbManagerErrCodeTest, ConvertErrCode001, TestSize.Level0)
{
    USB_HILOGI(MODULE_USB_INNERKIT, "ConvertErrCode001 start");
    EXPECT_EQ(ConvertErrCode(UEC_OK), ERR_SUCCESS);
    EXPECT_EQ(ConvertErrCode(UEC_SERVICE_PERMISSION_DENIED), ERR_PERM);
    EXPECT_EQ(ConvertErrCode(UEC_INTERFACE_PERMISSION_DENIED), ERR_PERM);
    EXPECT_EQ(ConvertErrCode(UEC_SERVICE_NO_MEMORY), ERR_NO_MEMORY);
    EXPECT_EQ(ConvertErrCode(UEC_INTERFACE_NO_MEMORY), ERR_NO_MEMORY);
    EXPECT_EQ(ConvertErrCode(UEC_SERVICE_INVALID_VALUE), ERR_SERVICE);
    EXPECT_EQ(ConvertErrCode(UEC_INTERFACE_NO_INIT), ERR_SERVICE);
    EXPECT_EQ(ConvertErrCode(HDF_FAILURE_CODE), ERR_SERVICE);
}
} // CapiTest
} // USB
} // OHOS
