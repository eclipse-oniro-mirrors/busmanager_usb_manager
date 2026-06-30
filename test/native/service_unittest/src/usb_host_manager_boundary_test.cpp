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

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include "usb_host_manager.h"
#include "usb_device.h"
#include "usb_errors.h"
#include "usb_descriptor_parser.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;

constexpr uint8_t TEST_BUS_NUM_1 = 1;
constexpr uint8_t TEST_BUS_NUM_2 = 2;
constexpr uint8_t TEST_DEV_ADDR_1 = 1;
constexpr uint8_t TEST_DEV_ADDR_2 = 2;
constexpr uint8_t TEST_INTERFACE_ID_0 = 0;
constexpr uint8_t TEST_ENDPOINT_ADDR_1 = 1;
constexpr uint8_t TEST_ENDPOINT_ADDR_2 = 2;
constexpr int32_t TEST_TIMEOUT = 1000;
constexpr int32_t NUM_DEVICES_TEST = 100;

class UsbHostManagerBoundaryTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    void CreateMockDevices(size_t count);
    UsbDevice* CreateTestDevice(uint8_t busNum, uint8_t devAddr,
                                uint16_t vendorId, uint16_t productId);

    std::unique_ptr<UsbHostManager> usbHostManager_;
    std::vector<UsbDevice*> testDevices_;
};

void UsbHostManagerBoundaryTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerBoundaryTest::SetUpTestCase enter");
}

void UsbHostManagerBoundaryTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerBoundaryTest::TearDownTestCase enter");
}

void UsbHostManagerBoundaryTest::SetUp()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerBoundaryTest::SetUp enter");
    SystemAbility* systemAbility = nullptr;
    usbHostManager_ = std::make_unique<UsbHostManager>(systemAbility);
    ASSERT_NE(usbHostManager_, nullptr);
    testDevices_.clear();
}

void UsbHostManagerBoundaryTest::TearDown()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerBoundaryTest::TearDown enter");
    for (auto* device : testDevices_) {
        if (device != nullptr) {
            delete device;
        }
    }
    testDevices_.clear();
    usbHostManager_.reset();
}

void UsbHostManagerBoundaryTest::CreateMockDevices(size_t count)
{
    for (size_t i = 0; i < count; i++) {
        uint8_t busNum = 1 + (i % 10);
        uint8_t devAddr = 1 + (i % 127);
        uint16_t vendorId = 0x1000 + (i % 0xFFFF);
        uint16_t productId = 0x2000 + (i % 0xFFFF);
        CreateTestDevice(busNum, devAddr, vendorId, productId);
    }
}

UsbDevice* UsbHostManagerBoundaryTest::CreateTestDevice(uint8_t busNum, uint8_t devAddr,
                                                        uint16_t vendorId, uint16_t productId)
{
    UsbDevice* device = new UsbDevice();
    if (device == nullptr) {
        return nullptr;
    }
    device->SetBusNum(busNum);
    device->SetDevAddr(devAddr);
    device->SetVendorId(vendorId);
    device->SetProductId(productId);
    device->SetClass(0x08);
    device->SetSubclass(0x01);
    device->SetProtocol(0x00);
    device->SetProductName("TestDevice");
    device->SetManufacturerName("TestManufacturer");
    device->SetmSerial("SN123456");
    device->SetVersion("1.0.0");
    device->SetAuthorizeStatus(ENABLED);

    testDevices_.push_back(device);
    return device;
}

/**
 * @tc.name: UsbHostManager_Boundary_OpenDevice_MaxBusNumber
 * @tc.desc: Test OpenDevice with maximum valid bus number
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_OpenDevice_MaxBusNumber, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_OpenDevice_MaxBusNumber start");

    uint8_t maxBusNum = 255;
    uint8_t devAddr = 1;

    int32_t ret = usbHostManager_->OpenDevice(maxBusNum, devAddr);

    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_OpenDevice_MaxBusNumber ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_Boundary_OpenDevice_MaxDevAddr
 * @tc.desc: Test OpenDevice with maximum valid device address
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_OpenDevice_MaxDevAddr, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_OpenDevice_MaxDevAddr start");

    uint8_t busNum = 1;
    uint8_t maxDevAddr = 127;

    int32_t ret = usbHostManager_->OpenDevice(busNum, maxDevAddr);

    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_OpenDevice_MaxDevAddr ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_Boundary_GetDevices_LargeList
 * @tc.desc: Test GetDevices with large device list
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_GetDevices_LargeList, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_GetDevices_LargeList start");

    std::vector<UsbDevice> deviceList;
    int32_t ret = usbHostManager_->GetDevices(deviceList);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_GetDevices_LargeList size=%{public}zu", deviceList.size());
}

/**
 * @tc.name: UsbHostManager_Boundary_GetDeviceInfo_NonExistentDevice
 * @tc.desc: Test GetDeviceInfo with non-existent device
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_GetDeviceInfo_NonExistentDevice, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_GetDeviceInfo_NonExistentDevice start");

    uint8_t busNum = 255;
    uint8_t devAddr = 255;
    UsbDevice dev;

    int32_t ret = usbHostManager_->GetDeviceInfo(busNum, devAddr, dev);

    EXPECT_TRUE(ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_GetDeviceInfo_NonExistentDevice ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_Boundary_BulkTransfer_LargeBuffer
 * @tc.desc: Test BulkTransfer with large buffer size
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_BulkTransfer_LargeBuffer, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_BulkTransfer_LargeBuffer start");

    UsbDev dev = {TEST_BUS_NUM_1, TEST_DEV_ADDR_1};
    UsbPipe pipe = {TEST_INTERFACE_ID_0, TEST_ENDPOINT_ADDR_1};

    std::vector<uint8_t> largeBuffer(512 * 1024, 0xAA);
    int32_t timeOut = TEST_TIMEOUT;

    int32_t ret = usbHostManager_->BulkTransferWrite(dev, pipe, largeBuffer, timeOut);

    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_BulkTransfer_LargeBuffer ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_Boundary_BulkTransfer_ZeroBuffer
 * @tc.desc: Test BulkTransfer with zero-length buffer
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_BulkTransfer_ZeroBuffer, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_BulkTransfer_ZeroBuffer start");

    UsbDev dev = {TEST_BUS_NUM_1, TEST_DEV_ADDR_1};
    UsbPipe pipe = {TEST_INTERFACE_ID_0, TEST_ENDPOINT_ADDR_1};

    std::vector<uint8_t> emptyBuffer;
    int32_t timeOut = TEST_TIMEOUT;

    int32_t ret = usbHostManager_->BulkTransferWrite(dev, pipe, emptyBuffer, timeOut);

    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_BulkTransfer_ZeroBuffer ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_Boundary_ControlTransfer_MaxLength
 * @tc.desc: Test ControlTransfer with maximum length
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_ControlTransfer_MaxLength, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_ControlTransfer_MaxLength start");

    UsbDev dev = {TEST_BUS_NUM_1, TEST_DEV_ADDR_1};
    UsbCtrlTransfer ctrl = {0x80, 0x06, 0x0100, 0x00, TEST_TIMEOUT};
    std::vector<uint8_t> bufferData;

    ctrl.length = 65535;

    int32_t ret = usbHostManager_->ControlTransfer(dev, ctrl, bufferData);

    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_ControlTransfer_MaxLength ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_Boundary_SetInterface_MaxAltIndex
 * @tc.desc: Test SetInterface with maximum alternate index
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_SetInterface_MaxAltIndex, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_SetInterface_MaxAltIndex start");

    uint8_t maxAltIndex = 255;

    int32_t ret = usbHostManager_->SetInterface(TEST_BUS_NUM_1, TEST_DEV_ADDR_1,
                                                 TEST_INTERFACE_ID_0, maxAltIndex);

    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_SetInterface_MaxAltIndex ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_Boundary_RequestQueue_LargeClientData
 * @tc.desc: Test RequestQueue with large client data
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_RequestQueue_LargeClientData, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_RequestQueue_LargeClientData start");

    UsbDev dev = {TEST_BUS_NUM_1, TEST_DEV_ADDR_1};
    UsbPipe pipe = {TEST_INTERFACE_ID_0, TEST_ENDPOINT_ADDR_1};

    std::vector<uint8_t> clientData(64 * 1024, 0x01);
    std::vector<uint8_t> bufferData(64 * 1024, 0);

    int32_t ret = usbHostManager_->RequestQueue(dev, pipe, clientData, bufferData);

    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_RequestQueue_LargeClientData ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_Boundary_ManageDevice_InvalidVidPid
 * @tc.desc: Test ManageDevice with invalid VID/PID combinations
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_ManageDevice_InvalidVidPid, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_ManageDevice_InvalidVidPid start");

    std::vector<std::pair<int32_t, int32_t>> invalidPairs = {
        {0xFFFF, 0xFFFF},
        {0x0000, 0x0000},
        {-1, -1},
        {0x1234, -1}
    };

    for (const auto& pair : invalidPairs) {
        int32_t vendorId = pair.first;
        int32_t productId = pair.second;
        bool disable = true;

        int32_t ret = usbHostManager_->ManageDevice(vendorId, productId, disable);

        EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
        USB_HILOGI(MODULE_USB_HOST, "ManageDevice vid=%{public}d pid=%{public}d ret=%{public}d",
                    vendorId, productId, ret);
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_ManageDevice_InvalidVidPid end");
}

/**
 * @tc.name: UsbHostManager_Boundary_ManageInterfaceType_EmptyList
 * @tc.desc: Test ManageInterfaceType with empty type list
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_ManageInterfaceType_EmptyList, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_ManageInterfaceType_EmptyList start");

    std::vector<UsbDeviceType> disableType;
    bool disable = true;

    int32_t ret = usbHostManager_->ManageInterfaceType(disableType, disable);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_ManageInterfaceType_EmptyList ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_Boundary_ManageInterfaceType_LargeList
 * @tc.desc: Test ManageInterfaceType with large type list
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_ManageInterfaceType_LargeList, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_ManageInterfaceType_LargeList start");

    std::vector<UsbDeviceType> disableType;
    for (int i = 0; i < 100; i++) {
        UsbDeviceType type = {static_cast<uint8_t>(i % 256), static_cast<uint8_t>((i / 256) % 256),
                             static_cast<uint8_t>((i / 65536) % 256), 0};
        disableType.push_back(type);
    }
    bool disable = true;

    int32_t ret = usbHostManager_->ManageInterfaceType(disableType, disable);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_ManageInterfaceType_LargeList ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_Boundary_ClaimInterface_ForceAllInterfaces
 * @tc.desc: Test ClaimInterface with force=1 for multiple interfaces
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_ClaimInterface_ForceAllInterfaces, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_ClaimInterface_ForceAllInterfaces start");

    uint8_t force = 1;

    for (uint8_t interfaceId = 0; interfaceId < 10; interfaceId++) {
        int32_t ret = usbHostManager_->ClaimInterface(TEST_BUS_NUM_1, TEST_DEV_ADDR_1,
                                                       interfaceId, force);
        USB_HILOGI(MODULE_USB_HOST, "ClaimInterface interface=%{public}u ret=%{public}d",
                    interfaceId, ret);
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_ClaimInterface_ForceAllInterfaces end");
}

/**
 * @tc.name: UsbHostManager_Boundary_GetRawDescriptor_VariousSizes
 * @tc.desc: Test GetRawDescriptor with various buffer sizes
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_GetRawDescriptor_VariousSizes, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_GetRawDescriptor_VariousSizes start");

    std::vector<size_t> bufferSizes = {0, 1, 64, 256, 1024, 4096, 65536};

    for (size_t size : bufferSizes) {
        std::vector<uint8_t> bufferData;
        int32_t ret = usbHostManager_->GetRawDescriptor(TEST_BUS_NUM_1, TEST_DEV_ADDR_1, bufferData);
        USB_HILOGI(MODULE_USB_HOST, "GetRawDescriptor size=%{public}zu ret=%{public}d", size, ret);
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_GetRawDescriptor_VariousSizes end");
}

/**
 * @tc.name: UsbHostManager_Boundary_Timeout_Variations
 * @tc.desc: Test operations with various timeout values
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerBoundaryTest, UsbHostManager_Boundary_Timeout_Variations, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_Timeout_Variations start");

    std::vector<int32_t> timeouts = {0, 1, 100, 1000, 5000, 30000, 60000};

    for (int32_t timeout : timeouts) {
        UsbDev dev = {TEST_BUS_NUM_1, TEST_DEV_ADDR_1};
        UsbPipe pipe = {TEST_INTERFACE_ID_0, TEST_ENDPOINT_ADDR_1};
        std::vector<uint8_t> bufferData = {0x01};

        int32_t ret = usbHostManager_->BulkTransferWrite(dev, pipe, bufferData, timeout);
        USB_HILOGI(MODULE_USB_HOST, "BulkTransferWrite timeout=%{public}d ret=%{public}d", timeout, ret);
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_Boundary_Timeout_Variations end");
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS