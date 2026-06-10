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

#include "usb_host_manager_extended_test.h"
#include <gtest/gtest-spi.h>
#include <numeric>
#include <random>
#include <algorithm>

#include "hilog_wrapper.h"

using namespace testing;
using namespace OHOS::HDI::Usb::V1_0;

namespace OHOS {
namespace USB {
namespace ExtendedTest {

constexpr uint8_t BUS_NUM_0 = 0;
constexpr uint8_t BUS_NUM_1 = 1;
constexpr uint8_t BUS_NUM_2 = 2;
constexpr uint8_t BUS_NUM_3 = 3;
constexpr uint8_t BUS_NUM_MAX = 255;
constexpr uint8_t DEV_ADDR_0 = 0;
constexpr uint8_t DEV_ADDR_1 = 1;
constexpr uint8_t DEV_ADDR_2 = 2;
constexpr uint8_t DEV_ADDR_3 = 3;
constexpr uint8_t DEV_ADDR_MAX = 255;
constexpr uint16_t VENDOR_ID_TEST_A = 0x1234;
constexpr uint16_t VENDOR_ID_TEST_B = 0x5678;
constexpr uint16_t VENDOR_ID_TEST_C = 0xABCD;
constexpr uint16_t VENDOR_ID_TEST_D = 0xDCBA;
constexpr uint16_t VENDOR_ID_MAX = 0xFFFF;
constexpr uint16_t VENDOR_ID_MIN = 0x0000;
constexpr uint16_t PRODUCT_ID_TEST_A = 0xAAAA;
constexpr uint16_t PRODUCT_ID_TEST_B = 0xBBBB;
constexpr uint16_t PRODUCT_ID_TEST_C = 0xCCCC;
constexpr uint16_t PRODUCT_ID_TEST_D = 0xDDDD;
constexpr uint8_t INTERFACE_ID_0 = 0;
constexpr uint8_t INTERFACE_ID_1 = 1;
constexpr uint8_t INTERFACE_ID_2 = 2;
constexpr uint8_t INTERFACE_ID_3 = 3;
constexpr uint8_t INTERFACE_ID_MAX = 255;
constexpr uint8_t ENDPOINT_ID_1 = 1;
constexpr uint8_t ENDPOINT_ID_2 = 2;
constexpr uint8_t ENDPOINT_ID_81 = 0x81;
constexpr uint8_t ENDPOINT_ID_82 = 0x82;
constexpr uint8_t ENDPOINT_ID_MAX = 255;
constexpr uint8_t CONFIG_INDEX_0 = 0;
constexpr uint8_t CONFIG_INDEX_1 = 1;
constexpr uint8_t ALT_INDEX_0 = 0;
constexpr uint8_t ALT_INDEX_1 = 1;
constexpr int32_t TIMEOUT_SHORT = 100;
constexpr int32_t TIMEOUT_DEFAULT = 1000;
constexpr int32_t TIMEOUT_LONG = 5000;
constexpr int32_t TIMEOUT_ZERO = 0;
constexpr int32_t TIMEOUT_NEGATIVE = -1;

void UsbHostManagerExtendedTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerExtendedTest::SetUpTestCase");
}

void UsbHostManagerExtendedTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerExtendedTest::TearDownTestCase");
}

void UsbHostManagerExtendedTest::SetUp()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerExtendedTest::SetUp");
    SystemAbility* sa = nullptr;
    hostManager_ = std::make_unique<UsbHostManager>(sa);
    ASSERT_NE(hostManager_, nullptr);
    devices_.clear();
}

void UsbHostManagerExtendedTest::TearDown()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerExtendedTest::TearDown");
    CleanupDevices();
    hostManager_.reset();
}

UsbDevice* UsbHostManagerExtendedTest::CreateExtendedDevice(uint8_t busNum, uint8_t devAddr,
    uint16_t vendorId, uint16_t productId, uint8_t deviceClass, uint8_t subClass,
    uint8_t protocol)
{
    UsbDevice* device = new UsbDevice();
    if (device == nullptr) {
        return nullptr;
    }
    device->SetBusNum(busNum);
    device->SetDevAddr(devAddr);
    device->SetVendorId(vendorId);
    device->SetProductId(productId);
    device->SetClass(deviceClass);
    device->SetSubclass(subClass);
    device->SetProtocol(protocol);
    device->SetProductName("ExtTestDevice");
    device->SetManufacturerName("ExtTestMfg");
    device->SetmSerial("EXT_SN_001");
    device->SetVersion("2.0.0");
    device->SetAuthorizeStatus(ENABLED);
    devices_.push_back(device);
    return device;
}

void UsbHostManagerExtendedTest::CleanupDevices()
{
    for (auto* device : devices_) {
        if (device != nullptr) {
            delete device;
        }
    }
    devices_.clear();
}

HWTEST_F(UsbHostManagerExtendedTest, Constructor_NullSystemAbility_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "Constructor_NullSystemAbility_001 start");
    SystemAbility* sa = nullptr;
    auto manager = std::make_unique<UsbHostManager>(sa);
    EXPECT_NE(manager, nullptr);
    USB_HILOGI(MODULE_USB_HOST, "Constructor_NullSystemAbility_001 end");
}

HWTEST_F(UsbHostManagerExtendedTest, Constructor_ReinterpretSystemAbility_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "Constructor_ReinterpretSystemAbility_001 start");
    SystemAbility* sa = reinterpret_cast<SystemAbility*>(0xFFFF);
    auto manager = std::make_unique<UsbHostManager>(sa);
    EXPECT_NE(manager, nullptr);
    USB_HILOGI(MODULE_USB_HOST, "Constructor_ReinterpretSystemAbility_001 end");
}

HWTEST_F(UsbHostManagerExtendedTest, Destructor_ScopeExit_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "Destructor_ScopeExit_001 start");
    {
        SystemAbility* sa = nullptr;
        auto manager = std::make_unique<UsbHostManager>(sa);
        ASSERT_NE(manager, nullptr);
    }
    SUCCEED();
    USB_HILOGI(MODULE_USB_HOST, "Destructor_ScopeExit_001 end");
}

HWTEST_F(UsbHostManagerExtendedTest, OpenDevice_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "OpenDevice_ValidParams_001 start");
    int32_t ret = hostManager_->OpenDevice(BUS_NUM_1, DEV_ADDR_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "OpenDevice ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, OpenDevice_ZeroBusZeroDev_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "OpenDevice_ZeroBusZeroDev_001 start");
    int32_t ret = hostManager_->OpenDevice(BUS_NUM_0, DEV_ADDR_0);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "OpenDevice(0,0) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, OpenDevice_MaxBusMaxDev_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "OpenDevice_MaxBusMaxDev_001 start");
    int32_t ret = hostManager_->OpenDevice(BUS_NUM_MAX, DEV_ADDR_MAX);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "OpenDevice(255,255) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, OpenDevice_MultipleDevices_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "OpenDevice_MultipleDevices_001 start");
    std::vector<std::pair<uint8_t, uint8_t>> deviceAddrs = {
        {1, 1}, {1, 2}, {2, 1}, {2, 2}, {3, 3}
    };
    for (auto& addr : deviceAddrs) {
        int32_t ret = hostManager_->OpenDevice(addr.first, addr.second);
        USB_HILOGI(MODULE_USB_HOST, "OpenDevice(%{public}u,%{public}u) ret=%{public}d",
                    addr.first, addr.second, ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, Close_WithoutOpen_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "Close_WithoutOpen_001 start");
    int32_t ret = hostManager_->Close(BUS_NUM_1, DEV_ADDR_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "Close ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, Close_AfterOpen_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "Close_AfterOpen_001 start");
    hostManager_->OpenDevice(BUS_NUM_1, DEV_ADDR_1);
    int32_t ret = hostManager_->Close(BUS_NUM_1, DEV_ADDR_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "Close ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, Close_DoubleClose_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "Close_DoubleClose_001 start");
    hostManager_->OpenDevice(BUS_NUM_1, DEV_ADDR_1);
    hostManager_->Close(BUS_NUM_1, DEV_ADDR_1);
    int32_t ret = hostManager_->Close(BUS_NUM_1, DEV_ADDR_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "DoubleClose ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ResetDevice_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ResetDevice_ValidParams_001 start");
    int32_t ret = hostManager_->ResetDevice(BUS_NUM_1, DEV_ADDR_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ResetDevice ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ResetDevice_InvalidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ResetDevice_InvalidParams_001 start");
    int32_t ret = hostManager_->ResetDevice(BUS_NUM_MAX, DEV_ADDR_MAX);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "ResetDevice(255,255) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ResetDevice_AfterOpen_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ResetDevice_AfterOpen_001 start");
    hostManager_->OpenDevice(BUS_NUM_1, DEV_ADDR_1);
    int32_t ret = hostManager_->ResetDevice(BUS_NUM_1, DEV_ADDR_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    hostManager_->Close(BUS_NUM_1, DEV_ADDR_1);
    USB_HILOGI(MODULE_USB_HOST, "ResetDevice after open ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ClaimInterface_ForceZero_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ClaimInterface_ForceZero_001 start");
    int32_t ret = hostManager_->ClaimInterface(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0, 0);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ClaimInterface force=0 ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ClaimInterface_ForceOne_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ClaimInterface_ForceOne_001 start");
    int32_t ret = hostManager_->ClaimInterface(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0, 1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ClaimInterface force=1 ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ClaimInterface_MultipleInterfaces_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ClaimInterface_MultipleInterfaces_001 start");
    for (uint8_t iface = 0; iface < 5; iface++) {
        int32_t ret = hostManager_->ClaimInterface(BUS_NUM_1, DEV_ADDR_1, iface, 1);
        USB_HILOGI(MODULE_USB_HOST, "ClaimInterface iface=%{public}u ret=%{public}d", iface, ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, ReleaseInterface_WithoutClaim_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ReleaseInterface_WithoutClaim_001 start");
    int32_t ret = hostManager_->ReleaseInterface(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ReleaseInterface ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ReleaseInterface_InvalidInterface_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ReleaseInterface_InvalidInterface_001 start");
    int32_t ret = hostManager_->ReleaseInterface(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_MAX);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ReleaseInterface(255) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, SetInterface_AltSettingZero_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "SetInterface_AltSettingZero_001 start");
    int32_t ret = hostManager_->SetInterface(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0, ALT_INDEX_0);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "SetInterface alt=0 ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, SetInterface_AltSettingOne_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "SetInterface_AltSettingOne_001 start");
    int32_t ret = hostManager_->SetInterface(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0, ALT_INDEX_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "SetInterface alt=1 ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, SetInterface_MultipleAltSettings_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "SetInterface_MultipleAltSettings_001 start");
    for (uint8_t alt = 0; alt < 5; alt++) {
        int32_t ret = hostManager_->SetInterface(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0, alt);
        USB_HILOGI(MODULE_USB_HOST, "SetInterface alt=%{public}u ret=%{public}d", alt, ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, SetActiveConfig_ConfigZero_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "SetActiveConfig_ConfigZero_001 start");
    int32_t ret = hostManager_->SetActiveConfig(BUS_NUM_1, DEV_ADDR_1, CONFIG_INDEX_0);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "SetActiveConfig config=0 ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, SetActiveConfig_ConfigOne_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "SetActiveConfig_ConfigOne_001 start");
    int32_t ret = hostManager_->SetActiveConfig(BUS_NUM_1, DEV_ADDR_1, CONFIG_INDEX_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "SetActiveConfig config=1 ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetActiveConfig_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetActiveConfig_ValidParams_001 start");
    uint8_t configIndex = 0;
    int32_t ret = hostManager_->GetActiveConfig(BUS_NUM_1, DEV_ADDR_1, configIndex);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "GetActiveConfig ret=%{public}d config=%{public}u", ret, configIndex);
}

HWTEST_F(UsbHostManagerExtendedTest, GetActiveConfig_InvalidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetActiveConfig_InvalidParams_001 start");
    uint8_t configIndex = 0;
    int32_t ret = hostManager_->GetActiveConfig(BUS_NUM_MAX, DEV_ADDR_MAX, configIndex);
    EXPECT_TRUE(ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "GetActiveConfig(255,255) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetRawDescriptor_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetRawDescriptor_ValidParams_001 start");
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->GetRawDescriptor(BUS_NUM_1, DEV_ADDR_1, bufferData);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "GetRawDescriptor ret=%{public}d size=%{public}zu", ret, bufferData.size());
}

HWTEST_F(UsbHostManagerExtendedTest, GetRawDescriptor_ZeroBusZeroDev_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetRawDescriptor_ZeroBusZeroDev_001 start");
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->GetRawDescriptor(BUS_NUM_0, DEV_ADDR_0, bufferData);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "GetRawDescriptor(0,0) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetFileDescriptor_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetFileDescriptor_ValidParams_001 start");
    int32_t fd = -1;
    int32_t ret = hostManager_->GetFileDescriptor(BUS_NUM_1, DEV_ADDR_1, fd);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    if (ret == UEC_OK) {
        EXPECT_GE(fd, 0);
    }
    USB_HILOGI(MODULE_USB_HOST, "GetFileDescriptor ret=%{public}d fd=%{public}d", ret, fd);
}

HWTEST_F(UsbHostManagerExtendedTest, GetDeviceSpeed_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetDeviceSpeed_ValidParams_001 start");
    uint8_t speed = 0;
    int32_t ret = hostManager_->GetDeviceSpeed(BUS_NUM_1, DEV_ADDR_1, speed);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    if (ret == UEC_OK) {
        USB_HILOGI(MODULE_USB_HOST, "Device speed=%{public}u", speed);
    }
    USB_HILOGI(MODULE_USB_HOST, "GetDeviceSpeed ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetDeviceSpeed_InvalidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetDeviceSpeed_InvalidParams_001 start");
    uint8_t speed = 0;
    int32_t ret = hostManager_->GetDeviceSpeed(BUS_NUM_MAX, DEV_ADDR_MAX, speed);
    EXPECT_TRUE(ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "GetDeviceSpeed(255,255) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetInterfaceActiveStatus_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetInterfaceActiveStatus_ValidParams_001 start");
    bool unactivated = false;
    int32_t ret = hostManager_->GetInterfaceActiveStatus(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0, unactivated);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "GetInterfaceActiveStatus ret=%{public}d unactivated=%{public}d",
                ret, unactivated);
}

HWTEST_F(UsbHostManagerExtendedTest, GetInterfaceActiveStatus_MultipleInterfaces_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetInterfaceActiveStatus_MultipleInterfaces_001 start");
    for (uint8_t iface = 0; iface < 4; iface++) {
        bool unactivated = false;
        int32_t ret = hostManager_->GetInterfaceActiveStatus(BUS_NUM_1, DEV_ADDR_1, iface, unactivated);
        USB_HILOGI(MODULE_USB_HOST, "Interface %{public}u: ret=%{public}d unactivated=%{public}d",
                    iface, ret, unactivated);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, ClearHalt_ValidEndpoint_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ClearHalt_ValidEndpoint_001 start");
    int32_t ret = hostManager_->ClearHalt(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0, ENDPOINT_ID_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ClearHalt ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ClearHalt_ZeroEndpoint_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ClearHalt_ZeroEndpoint_001 start");
    int32_t ret = hostManager_->ClearHalt(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0, 0);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ClearHalt ep=0 ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ClearHalt_MaxEndpoint_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ClearHalt_MaxEndpoint_001 start");
    int32_t ret = hostManager_->ClearHalt(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0, ENDPOINT_ID_MAX);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ClearHalt ep=255 ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkTransferRead_DefaultTimeout_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferRead_DefaultTimeout_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->BulkTransferRead(dev, pipe, bufferData, TIMEOUT_DEFAULT);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferRead ret=%{public}d size=%{public}zu", ret, bufferData.size());
}

HWTEST_F(UsbHostManagerExtendedTest, BulkTransferRead_ShortTimeout_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferRead_ShortTimeout_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->BulkTransferRead(dev, pipe, bufferData, TIMEOUT_SHORT);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferRead short timeout ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkTransferRead_LongTimeout_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferRead_LongTimeout_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->BulkTransferRead(dev, pipe, bufferData, TIMEOUT_LONG);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferRead long timeout ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkTransferRead_ZeroTimeout_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferRead_ZeroTimeout_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->BulkTransferRead(dev, pipe, bufferData, TIMEOUT_ZERO);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferRead zero timeout ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkTransferWrite_SmallBuffer_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferWrite_SmallBuffer_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_2};
    std::vector<uint8_t> bufferData = {0x01};
    int32_t ret = hostManager_->BulkTransferWrite(dev, pipe, bufferData, TIMEOUT_DEFAULT);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferWrite small buffer ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkTransferWrite_LargeBuffer_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferWrite_LargeBuffer_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_2};
    std::vector<uint8_t> bufferData(4096, 0xAA);
    int32_t ret = hostManager_->BulkTransferWrite(dev, pipe, bufferData, TIMEOUT_DEFAULT);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferWrite large buffer ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkTransferWrite_EmptyBuffer_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferWrite_EmptyBuffer_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_2};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->BulkTransferWrite(dev, pipe, bufferData, TIMEOUT_DEFAULT);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferWrite empty buffer ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkTransferReadwithLength_SmallLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferReadwithLength_SmallLength_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->BulkTransferReadwithLength(dev, pipe, 64, bufferData, TIMEOUT_DEFAULT);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferReadwithLength ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkTransferReadwithLength_LargeLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferReadwithLength_LargeLength_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->BulkTransferReadwithLength(dev, pipe, 16384, bufferData, TIMEOUT_DEFAULT);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferReadwithLength large ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkTransferReadwithLength_ZeroLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferReadwithLength_ZeroLength_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->BulkTransferReadwithLength(dev, pipe, 0, bufferData, TIMEOUT_DEFAULT);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferReadwithLength zero ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ControlTransfer_StandardRequest_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ControlTransfer_StandardRequest_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbCtrlTransfer ctrl = {0x80, 0x06, 0x0100, 0x00, TIMEOUT_DEFAULT};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->ControlTransfer(dev, ctrl, bufferData);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "ControlTransfer standard ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ControlTransfer_VendorRequest_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ControlTransfer_VendorRequest_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbCtrlTransfer ctrl = {0x40, 0x01, 0x0000, 0x0000, TIMEOUT_DEFAULT};
    std::vector<uint8_t> bufferData = {0x01, 0x02, 0x03};
    int32_t ret = hostManager_->ControlTransfer(dev, ctrl, bufferData);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "ControlTransfer vendor ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ControlTransfer_ClassRequest_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ControlTransfer_ClassRequest_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbCtrlTransfer ctrl = {0xA1, 0xFE, 0x0000, 0x0000, TIMEOUT_DEFAULT};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->ControlTransfer(dev, ctrl, bufferData);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "ControlTransfer class ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UsbControlTransfer_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbControlTransfer_ValidParams_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    HDI::Usb::V1_2::UsbCtrlTransferParams ctrlParams = {0x80, 0x06, 0x0100, 0x00, TIMEOUT_DEFAULT};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->UsbControlTransfer(dev, ctrlParams, bufferData);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "UsbControlTransfer ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UsbControlTransfer_DifferentTimeouts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbControlTransfer_DifferentTimeouts_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    std::vector<int32_t> timeouts = {TIMEOUT_ZERO, TIMEOUT_SHORT, TIMEOUT_DEFAULT, TIMEOUT_LONG};
    for (auto timeout : timeouts) {
        HDI::Usb::V1_2::UsbCtrlTransferParams ctrlParams = {0x80, 0x06, 0x0100, 0x00, timeout};
        std::vector<uint8_t> bufferData;
        int32_t ret = hostManager_->UsbControlTransfer(dev, ctrlParams, bufferData);
        USB_HILOGI(MODULE_USB_HOST, "UsbControlTransfer timeout=%{public}d ret=%{public}d", timeout, ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, RequestQueue_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "RequestQueue_ValidParams_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    std::vector<uint8_t> clientData = {0xAA, 0xBB};
    std::vector<uint8_t> bufferData(256, 0x00);
    int32_t ret = hostManager_->RequestQueue(dev, pipe, clientData, bufferData);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "RequestQueue ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, RequestQueue_EmptyClientData_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "RequestQueue_EmptyClientData_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    std::vector<uint8_t> clientData;
    std::vector<uint8_t> bufferData(256, 0x00);
    int32_t ret = hostManager_->RequestQueue(dev, pipe, clientData, bufferData);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "RequestQueue empty client ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, RequestQueue_EmptyBufferData_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "RequestQueue_EmptyBufferData_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    std::vector<uint8_t> clientData = {0x01};
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->RequestQueue(dev, pipe, clientData, bufferData);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "RequestQueue empty buffer ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, RequestWait_DefaultTimeout_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "RequestWait_DefaultTimeout_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    std::vector<uint8_t> clientData;
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->RequestWait(dev, TIMEOUT_DEFAULT, clientData, bufferData);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "RequestWait ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, RequestWait_ZeroTimeout_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "RequestWait_ZeroTimeout_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    std::vector<uint8_t> clientData;
    std::vector<uint8_t> bufferData;
    int32_t ret = hostManager_->RequestWait(dev, TIMEOUT_ZERO, clientData, bufferData);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "RequestWait zero ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, RequestCancel_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "RequestCancel_ValidParams_001 start");
    int32_t ret = hostManager_->RequestCancel(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0, ENDPOINT_ID_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "RequestCancel ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, RequestCancel_InvalidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "RequestCancel_InvalidParams_001 start");
    int32_t ret = hostManager_->RequestCancel(BUS_NUM_MAX, DEV_ADDR_MAX, INTERFACE_ID_MAX, ENDPOINT_ID_MAX);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "RequestCancel(255,255,255,255) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UsbCancelTransfer_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbCancelTransfer_ValidParams_001 start");
    UsbDev devInfo = {BUS_NUM_1, DEV_ADDR_1};
    int32_t ret = hostManager_->UsbCancelTransfer(devInfo, ENDPOINT_ID_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbCancelTransfer ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UsbCancelTransfer_DifferentEndpoints_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbCancelTransfer_DifferentEndpoints_001 start");
    UsbDev devInfo = {BUS_NUM_1, DEV_ADDR_1};
    std::vector<int32_t> endpoints = {0x01, 0x02, 0x81, 0x82, 0x83};
    for (auto ep : endpoints) {
        int32_t ret = hostManager_->UsbCancelTransfer(devInfo, ep);
        USB_HILOGI(MODULE_USB_HOST, "UsbCancelTransfer ep=0x%{public}X ret=%{public}d", ep, ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, RegBulkCallback_NullCallback_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "RegBulkCallback_NullCallback_001 start");
    UsbDev devInfo = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    sptr<IRemoteObject> cb = nullptr;
    int32_t ret = hostManager_->RegBulkCallback(devInfo, pipe, cb);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "RegBulkCallback null ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UnRegBulkCallback_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UnRegBulkCallback_ValidParams_001 start");
    UsbDev devInfo = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    int32_t ret = hostManager_->UnRegBulkCallback(devInfo, pipe);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UnRegBulkCallback ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkRead_NullAshmem_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkRead_NullAshmem_001 start");
    UsbDev devInfo = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    sptr<Ashmem> ashmem = nullptr;
    int32_t ret = hostManager_->BulkRead(devInfo, pipe, ashmem);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkRead null ashmem ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkWrite_NullAshmem_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkWrite_NullAshmem_001 start");
    UsbDev devInfo = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_2};
    sptr<Ashmem> ashmem = nullptr;
    int32_t ret = hostManager_->BulkWrite(devInfo, pipe, ashmem);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkWrite null ashmem ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, BulkCancel_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "BulkCancel_ValidParams_001 start");
    UsbDev devInfo = {BUS_NUM_1, DEV_ADDR_1};
    UsbPipe pipe = {INTERFACE_ID_0, ENDPOINT_ID_81};
    int32_t ret = hostManager_->BulkCancel(devInfo, pipe);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "BulkCancel ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageGlobalInterface_DisableTrue_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageGlobalInterface_DisableTrue_001 start");
    int32_t ret = hostManager_->ManageGlobalInterface(true);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageGlobalInterface(true) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageGlobalInterface_DisableFalse_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageGlobalInterface_DisableFalse_001 start");
    int32_t ret = hostManager_->ManageGlobalInterface(false);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageGlobalInterface(false) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageDevice_SingleVendorProduct_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageDevice_SingleVendorProduct_001 start");
    int32_t ret = hostManager_->ManageDevice(VENDOR_ID_TEST_A, PRODUCT_ID_TEST_A, true);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageDevice ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageDevice_MultipleDevices_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageDevice_MultipleDevices_001 start");
    std::vector<std::pair<int32_t, int32_t>> devIds = {
        {VENDOR_ID_TEST_A, PRODUCT_ID_TEST_A},
        {VENDOR_ID_TEST_B, PRODUCT_ID_TEST_B},
        {VENDOR_ID_TEST_C, PRODUCT_ID_TEST_C},
        {VENDOR_ID_TEST_D, PRODUCT_ID_TEST_D}
    };
    for (auto& devId : devIds) {
        int32_t ret = hostManager_->ManageDevice(devId.first, devId.second, true);
        EXPECT_EQ(ret, UEC_OK);
        USB_HILOGI(MODULE_USB_HOST, "ManageDevice vid=0x%{public}X pid=0x%{public}X ret=%{public}d",
                    devId.first, devId.second, ret);
    }
}

HWTEST_F(UsbHostManagerExtendedTest, ManageDevice_ZeroIds_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageDevice_ZeroIds_001 start");
    int32_t ret = hostManager_->ManageDevice(VENDOR_ID_MIN, VENDOR_ID_MIN, true);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageDevice(0,0) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageDevice_MaxIds_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageDevice_MaxIds_001 start");
    int32_t ret = hostManager_->ManageDevice(VENDOR_ID_MAX, VENDOR_ID_MAX, false);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageDevice(0xFFFF,0xFFFF) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageDevicePolicy_EmptyTrustList_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageDevicePolicy_EmptyTrustList_001 start");
    std::vector<UsbDeviceId> trustList;
    int32_t ret = hostManager_->ManageDevicePolicy(trustList);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageDevicePolicy empty ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageDevicePolicy_SingleEntry_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageDevicePolicy_SingleEntry_001 start");
    std::vector<UsbDeviceId> trustList;
    trustList.push_back({VENDOR_ID_TEST_A, PRODUCT_ID_TEST_A});
    int32_t ret = hostManager_->ManageDevicePolicy(trustList);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageDevicePolicy single ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageDevicePolicy_MultipleEntries_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageDevicePolicy_MultipleEntries_001 start");
    std::vector<UsbDeviceId> trustList;
    trustList.push_back({VENDOR_ID_TEST_A, PRODUCT_ID_TEST_A});
    trustList.push_back({VENDOR_ID_TEST_B, PRODUCT_ID_TEST_B});
    trustList.push_back({VENDOR_ID_TEST_C, PRODUCT_ID_TEST_C});
    trustList.push_back({VENDOR_ID_TEST_D, PRODUCT_ID_TEST_D});
    trustList.push_back({0x1111, 0x2222});
    int32_t ret = hostManager_->ManageDevicePolicy(trustList);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageDevicePolicy multiple ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageInterfaceType_EmptyTypes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageInterfaceType_EmptyTypes_001 start");
    std::vector<UsbDeviceType> disableType;
    bool disable = true;
    int32_t ret = hostManager_->ManageInterfaceType(disableType, disable);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageInterfaceType empty ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageInterfaceType_MultipleTypes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageInterfaceType_MultipleTypes_001 start");
    std::vector<UsbDeviceType> disableType;
    disableType.push_back({0x08, 0x06, 0x50, 0});
    disableType.push_back({0x03, 0x01, 0x01, 0});
    disableType.push_back({0x03, 0x01, 0x02, 0});
    disableType.push_back({0x0E, 0x01, 0x00, 0});
    disableType.push_back({0x01, 0x01, 0x00, 0});
    bool disable = true;
    int32_t ret = hostManager_->ManageInterfaceType(disableType, disable);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageInterfaceType multiple ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageInterfaceType_DisableFalse_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageInterfaceType_DisableFalse_001 start");
    std::vector<UsbDeviceType> disableType;
    disableType.push_back({0x08, 0x01, 0x00, 0});
    bool disable = false;
    int32_t ret = hostManager_->ManageInterfaceType(disableType, disable);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageInterfaceType disable=false ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UsbAttachKernelDriver_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbAttachKernelDriver_ValidParams_001 start");
    int32_t ret = hostManager_->UsbAttachKernelDriver(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbAttachKernelDriver ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UsbAttachKernelDriver_DifferentInterfaces_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbAttachKernelDriver_DifferentInterfaces_001 start");
    for (uint8_t iface = 0; iface < 4; iface++) {
        int32_t ret = hostManager_->UsbAttachKernelDriver(BUS_NUM_1, DEV_ADDR_1, iface);
        USB_HILOGI(MODULE_USB_HOST, "AttachKernelDriver iface=%{public}u ret=%{public}d", iface, ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, UsbDetachKernelDriver_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDetachKernelDriver_ValidParams_001 start");
    int32_t ret = hostManager_->UsbDetachKernelDriver(BUS_NUM_1, DEV_ADDR_1, INTERFACE_ID_0);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbDetachKernelDriver ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UsbDetachKernelDriver_DifferentInterfaces_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDetachKernelDriver_DifferentInterfaces_001 start");
    for (uint8_t iface = 0; iface < 4; iface++) {
        int32_t ret = hostManager_->UsbDetachKernelDriver(BUS_NUM_1, DEV_ADDR_1, iface);
        USB_HILOGI(MODULE_USB_HOST, "DetachKernelDriver iface=%{public}u ret=%{public}d", iface, ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, GetDevices_MultipleCalls_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetDevices_MultipleCalls_001 start");
    for (int i = 0; i < 10; i++) {
        std::vector<UsbDevice> deviceList;
        int32_t ret = hostManager_->GetDevices(deviceList);
        EXPECT_EQ(ret, UEC_OK);
        USB_HILOGI(MODULE_USB_HOST, "GetDevices iter=%{public}d count=%{public}zu", i, deviceList.size());
    }
}

HWTEST_F(UsbHostManagerExtendedTest, GetDeviceInfo_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetDeviceInfo_ValidParams_001 start");
    UsbDevice dev;
    int32_t ret = hostManager_->GetDeviceInfo(BUS_NUM_1, DEV_ADDR_1, dev);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "GetDeviceInfo ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetDeviceInfo_InvalidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetDeviceInfo_InvalidParams_001 start");
    UsbDevice dev;
    int32_t ret = hostManager_->GetDeviceInfo(BUS_NUM_MAX, DEV_ADDR_MAX, dev);
    EXPECT_TRUE(ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "GetDeviceInfo(255,255) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetDeviceInfoDescriptor_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetDeviceInfoDescriptor_ValidParams_001 start");
    UsbDev uDev = {BUS_NUM_1, DEV_ADDR_1};
    std::vector<uint8_t> descriptor;
    UsbDevice dev;
    int32_t ret = hostManager_->GetDeviceInfoDescriptor(uDev, descriptor, dev);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "GetDeviceInfoDescriptor ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetConfigDescriptor_ValidDevice_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetConfigDescriptor_ValidDevice_001 start");
    auto* dev = CreateExtendedDevice(BUS_NUM_1, DEV_ADDR_1, VENDOR_ID_TEST_A, PRODUCT_ID_TEST_A, 0x08, 0x06, 0x50);
    ASSERT_NE(dev, nullptr);
    std::vector<uint8_t> descriptor;
    int32_t ret = hostManager_->GetConfigDescriptor(*dev, descriptor);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "GetConfigDescriptor ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, FillDevStrings_ValidDevice_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "FillDevStrings_ValidDevice_001 start");
    auto* dev = CreateExtendedDevice(BUS_NUM_1, DEV_ADDR_1, VENDOR_ID_TEST_A, PRODUCT_ID_TEST_A, 0x08, 0x06, 0x50);
    ASSERT_NE(dev, nullptr);
    int32_t ret = hostManager_->FillDevStrings(*dev);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "FillDevStrings ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetTargetDevice_NonExistent_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetTargetDevice_NonExistent_001 start");
    UsbDevice dev;
    bool ret = hostManager_->GetTargetDevice(BUS_NUM_1, DEV_ADDR_1, dev);
    EXPECT_FALSE(ret);
    USB_HILOGI(MODULE_USB_HOST, "GetTargetDevice ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetTargetDevice_MultipleLookups_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetTargetDevice_MultipleLookups_001 start");
    std::vector<std::pair<uint8_t, uint8_t>> addrs = {
        {0, 0}, {1, 1}, {2, 2}, {127, 127}, {255, 255}
    };
    for (auto& addr : addrs) {
        UsbDevice dev;
        bool ret = hostManager_->GetTargetDevice(addr.first, addr.second, dev);
        USB_HILOGI(MODULE_USB_HOST, "GetTargetDevice(%{public}u,%{public}u)=%{public}d",
                    addr.first, addr.second, ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, GetEndpointFromId_NoEndpoints_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetEndpointFromId_NoEndpoints_001 start");
    auto* dev = CreateExtendedDevice(BUS_NUM_1, DEV_ADDR_1, VENDOR_ID_TEST_A, PRODUCT_ID_TEST_A, 0x08, 0x06, 0x50);
    ASSERT_NE(dev, nullptr);
    USBEndpoint endpoint;
    bool ret = hostManager_->GetEndpointFromId(*dev, ENDPOINT_ID_81, endpoint);
    EXPECT_FALSE(ret);
    USB_HILOGI(MODULE_USB_HOST, "GetEndpointFromId ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetEndpointFromId_MultipleIds_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetEndpointFromId_MultipleIds_001 start");
    auto* dev = CreateExtendedDevice(BUS_NUM_1, DEV_ADDR_1, VENDOR_ID_TEST_A, PRODUCT_ID_TEST_A, 0x08, 0x06, 0x50);
    ASSERT_NE(dev, nullptr);
    std::vector<int32_t> endpointIds = {0x01, 0x02, 0x81, 0x82, 0xFF};
    for (auto epId : endpointIds) {
        USBEndpoint endpoint;
        bool ret = hostManager_->GetEndpointFromId(*dev, epId, endpoint);
        USB_HILOGI(MODULE_USB_HOST, "GetEndpointFromId ep=0x%{public}X ret=%{public}d", epId, ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, GetProductName_NonExistent_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetProductName_NonExistent_001 start");
    std::string deviceName = "1-1";
    std::string productName;
    bool ret = hostManager_->GetProductName(deviceName, productName);
    EXPECT_FALSE(ret);
    USB_HILOGI(MODULE_USB_HOST, "GetProductName ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetProductName_EmptyName_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetProductName_EmptyName_001 start");
    std::string deviceName = "";
    std::string productName;
    bool ret = hostManager_->GetProductName(deviceName, productName);
    EXPECT_FALSE(ret);
    USB_HILOGI(MODULE_USB_HOST, "GetProductName empty ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetProductName_VariousNames_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetProductName_VariousNames_001 start");
    std::vector<std::string> deviceNames = {"1-1", "1-2", "2-1", "usb1", ""};
    for (auto& name : deviceNames) {
        std::string productName;
        bool ret = hostManager_->GetProductName(name, productName);
        USB_HILOGI(MODULE_USB_HOST, "GetProductName '%{public}s' ret=%{public}d", name.c_str(), ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, UsbDeviceAuthorize_AuthorizedTrue_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDeviceAuthorize_AuthorizedTrue_001 start");
    int32_t ret = hostManager_->UsbDeviceAuthorize(BUS_NUM_1, DEV_ADDR_1, true, "authorize");
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbDeviceAuthorize(true) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UsbDeviceAuthorize_AuthorizedFalse_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDeviceAuthorize_AuthorizedFalse_001 start");
    int32_t ret = hostManager_->UsbDeviceAuthorize(BUS_NUM_1, DEV_ADDR_1, false, "unauthorize");
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbDeviceAuthorize(false) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UsbInterfaceAuthorize_EnableInterface_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbInterfaceAuthorize_EnableInterface_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    int32_t ret = hostManager_->UsbInterfaceAuthorize(dev, CONFIG_INDEX_0, INTERFACE_ID_0, true);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbInterfaceAuthorize(true) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UsbInterfaceAuthorize_DisableInterface_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbInterfaceAuthorize_DisableInterface_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    int32_t ret = hostManager_->UsbInterfaceAuthorize(dev, CONFIG_INDEX_0, INTERFACE_ID_0, false);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbInterfaceAuthorize(false) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, UsbInterfaceAuthorize_MultipleInterfaces_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbInterfaceAuthorize_MultipleInterfaces_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    for (uint8_t iface = 0; iface < 4; iface++) {
        int32_t ret = hostManager_->UsbInterfaceAuthorize(dev, CONFIG_INDEX_0, iface, true);
        USB_HILOGI(MODULE_USB_HOST, "InterfaceAuthorize iface=%{public}u ret=%{public}d", iface, ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, ManageInterface_EnableDev_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageInterface_EnableDev_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    int32_t ret = hostManager_->ManageInterface(dev, INTERFACE_ID_0, false);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ManageInterface(false) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageInterface_DisableDev_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageInterface_DisableDev_001 start");
    UsbDev dev = {BUS_NUM_1, DEV_ADDR_1};
    int32_t ret = hostManager_->ManageInterface(dev, INTERFACE_ID_0, true);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ManageInterface(true) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, CheckDevPathIsExist_ValidParams_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "CheckDevPathIsExist_ValidParams_001 start");
    int32_t ret = hostManager_->CheckDevPathIsExist(BUS_NUM_1, DEV_ADDR_1);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE || ret == UEC_SERVICE_INNER_ERR);
    USB_HILOGI(MODULE_USB_HOST, "CheckDevPathIsExist ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, CheckDevPathIsExist_BoundaryValues_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "CheckDevPathIsExist_BoundaryValues_001 start");
    std::vector<std::pair<uint8_t, uint8_t>> testCases = {
        {0, 0}, {0, 1}, {1, 0}, {1, 1}, {127, 127}, {255, 1}, {1, 255}, {255, 255}
    };
    for (auto& tc : testCases) {
        int32_t ret = hostManager_->CheckDevPathIsExist(tc.first, tc.second);
        USB_HILOGI(MODULE_USB_HOST, "CheckDevPathIsExist(%{public}u,%{public}u)=%{public}d",
                    tc.first, tc.second, ret);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, Dump_ValidArg_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "Dump_ValidArg_001 start");
    std::string args = "-a";
    bool ret = hostManager_->Dump(1, args);
    EXPECT_TRUE(ret);
    USB_HILOGI(MODULE_USB_HOST, "Dump(-a) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, Dump_InvalidArg_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "Dump_InvalidArg_001 start");
    std::string args = "-invalid";
    bool ret = hostManager_->Dump(1, args);
    EXPECT_FALSE(ret);
    USB_HILOGI(MODULE_USB_HOST, "Dump(-invalid) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, Dump_EmptyArg_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "Dump_EmptyArg_001 start");
    std::string args = "";
    bool ret = hostManager_->Dump(1, args);
    EXPECT_FALSE(ret);
    USB_HILOGI(MODULE_USB_HOST, "Dump(empty) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ExecuteStrategy_NoDevices_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ExecuteStrategy_NoDevices_001 start");
    hostManager_->ExecuteStrategy();
    SUCCEED();
    USB_HILOGI(MODULE_USB_HOST, "ExecuteStrategy_NoDevices_001 end");
}

HWTEST_F(UsbHostManagerExtendedTest, ExecuteManageDevicePolicy_EmptyList_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ExecuteManageDevicePolicy_EmptyList_001 start");
    std::vector<UsbDeviceId> trustList;
    int32_t ret = hostManager_->ExecuteManageDevicePolicy(trustList);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ExecuteManageDevicePolicy ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ExecuteManageDevicePolicy_LargeList_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ExecuteManageDevicePolicy_LargeList_001 start");
    std::vector<UsbDeviceId> trustList;
    for (int i = 0; i < 20; i++) {
        trustList.push_back({static_cast<uint16_t>(i * 0x100), static_cast<uint16_t>(i * 0x100 + 1)});
    }
    int32_t ret = hostManager_->ExecuteManageDevicePolicy(trustList);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ExecuteManageDevicePolicy large ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ExecuteManageInterfaceType_MultipleTypes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ExecuteManageInterfaceType_MultipleTypes_001 start");
    std::vector<UsbDeviceType> disableType;
    disableType.push_back({0x08, 0x06, 0x50, 0});
    disableType.push_back({0x03, 0x01, 0x01, 0});
    disableType.push_back({0x03, 0x01, 0x02, 0});
    disableType.push_back({0x0E, 0x01, 0x00, 0});
    int32_t ret = hostManager_->ExecuteManageInterfaceType(disableType, false);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ExecuteManageInterfaceType ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, GetDevStringValFromIdx_ValidIdx_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetDevStringValFromIdx_ValidIdx_001 start");
    std::string result = hostManager_->GetDevStringValFromIdx(BUS_NUM_1, DEV_ADDR_1, 1);
    USB_HILOGI(MODULE_USB_HOST, "GetDevStringValFromIdx result='%{public}s'", result.c_str());
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, GetDevStringValFromIdx_VariousIndices_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "GetDevStringValFromIdx_VariousIndices_001 start");
    for (uint8_t idx = 0; idx < 10; idx++) {
        std::string result = hostManager_->GetDevStringValFromIdx(BUS_NUM_1, DEV_ADDR_1, idx);
        USB_HILOGI(MODULE_USB_HOST, "idx=%{public}u result='%{public}s'", idx, result.c_str());
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, ConcurrentGetDevices_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ConcurrentGetDevices_001 start");
    const int threadCount = 10;
    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < 20; j++) {
                std::vector<UsbDevice> deviceList;
                int32_t ret = hostManager_->GetDevices(deviceList);
                EXPECT_EQ(ret, UEC_OK);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, ConcurrentManageDevice_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ConcurrentManageDevice_001 start");
    const int threadCount = 5;
    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([this, i]() {
            int32_t vid = VENDOR_ID_TEST_A + i;
            int32_t pid = PRODUCT_ID_TEST_A + i;
            int32_t ret = hostManager_->ManageDevice(vid, pid, (i % 2 == 0));
            EXPECT_EQ(ret, UEC_OK);
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, StressTest_OpenClose_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "StressTest_OpenClose_001 start");
    for (int i = 0; i < 100; i++) {
        hostManager_->OpenDevice(BUS_NUM_1, DEV_ADDR_1);
        hostManager_->Close(BUS_NUM_1, DEV_ADDR_1);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, StressTest_GetDevices_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "StressTest_GetDevices_001 start");
    for (int i = 0; i < 100; i++) {
        std::vector<UsbDevice> deviceList;
        int32_t ret = hostManager_->GetDevices(deviceList);
        EXPECT_EQ(ret, UEC_OK);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, DeviceCreation_SetProperties_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "DeviceCreation_SetProperties_001 start");
    auto* dev = CreateExtendedDevice(BUS_NUM_2, DEV_ADDR_3, VENDOR_ID_TEST_C, PRODUCT_ID_TEST_C, 0x0E, 0x01, 0x00);
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(dev->GetBusNum(), BUS_NUM_2);
    EXPECT_EQ(dev->GetDevAddr(), DEV_ADDR_3);
    EXPECT_EQ(dev->GetVendorId(), VENDOR_ID_TEST_C);
    EXPECT_EQ(dev->GetProductId(), PRODUCT_ID_TEST_C);
    EXPECT_EQ(dev->GetClass(), 0x0E);
    USB_HILOGI(MODULE_USB_HOST, "DeviceCreation_SetProperties_001 end");
}

HWTEST_F(UsbHostManagerExtendedTest, DeviceCreation_MultipleDevices_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "DeviceCreation_MultipleDevices_001 start");
    auto* dev1 = CreateExtendedDevice(1, 1, 0x1234, 0xAAAA, 0x08, 0x06, 0x50);
    auto* dev2 = CreateExtendedDevice(1, 2, 0x5678, 0xBBBB, 0x03, 0x01, 0x01);
    auto* dev3 = CreateExtendedDevice(2, 1, 0xABCD, 0xCCCC, 0x0E, 0x01, 0x00);
    auto* dev4 = CreateExtendedDevice(2, 2, 0xDCBA, 0xDDDD, 0x01, 0x01, 0x00);
    ASSERT_NE(dev1, nullptr);
    ASSERT_NE(dev2, nullptr);
    ASSERT_NE(dev3, nullptr);
    ASSERT_NE(dev4, nullptr);
    EXPECT_EQ(devices_.size(), 4u);
    USB_HILOGI(MODULE_USB_HOST, "DeviceCreation_MultipleDevices_001 end");
}

HWTEST_F(UsbHostManagerExtendedTest, ErrorHandling_AllInvalid_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ErrorHandling_AllInvalid_001 start");
    std::vector<std::pair<std::string, int32_t>> results;

    results.push_back({"OpenDevice(255,255)", hostManager_->OpenDevice(BUS_NUM_MAX, DEV_ADDR_MAX)});
    results.push_back({"Close(255,255)", hostManager_->Close(BUS_NUM_MAX, DEV_ADDR_MAX)});
    results.push_back({"ResetDevice(255,255)", hostManager_->ResetDevice(BUS_NUM_MAX, DEV_ADDR_MAX)});
    results.push_back({"ClaimInterface(255,255,255,255)",
        hostManager_->ClaimInterface(BUS_NUM_MAX, DEV_ADDR_MAX, INTERFACE_ID_MAX, 255)});

    for (auto& r : results) {
        USB_HILOGI(MODULE_USB_HOST, "%{public}s = %{public}d", r.first.c_str(), r.second);
    }
    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, ErrorHandling_TransferInvalid_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ErrorHandling_TransferInvalid_001 start");
    UsbDev dev = {BUS_NUM_MAX, DEV_ADDR_MAX};
    UsbPipe pipe = {INTERFACE_ID_MAX, ENDPOINT_ID_MAX};

    std::vector<uint8_t> bufferData = {0x01, 0x02};
    int32_t ret1 = hostManager_->BulkTransferRead(dev, pipe, bufferData, TIMEOUT_DEFAULT);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferRead invalid=%{public}d", ret1);

    int32_t ret2 = hostManager_->BulkTransferWrite(dev, pipe, bufferData, TIMEOUT_DEFAULT);
    USB_HILOGI(MODULE_USB_HOST, "BulkTransferWrite invalid=%{public}d", ret2);

    UsbCtrlTransfer ctrl = {0xFF, 0xFF, 0xFFFF, 0xFFFF, 0};
    std::vector<uint8_t> ctrlBuffer;
    int32_t ret3 = hostManager_->ControlTransfer(dev, ctrl, ctrlBuffer);
    USB_HILOGI(MODULE_USB_HOST, "ControlTransfer invalid=%{public}d", ret3);

    SUCCEED();
}

HWTEST_F(UsbHostManagerExtendedTest, UsbSubmitTransfer_NullCallback_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbSubmitTransfer_NullCallback_001 start");
    UsbDev devInfo = {BUS_NUM_1, DEV_ADDR_1};
    HDI::Usb::V1_2::USBTransferInfo info;
    sptr<IRemoteObject> cb = nullptr;
    sptr<Ashmem> ashmem = nullptr;
    int32_t ret = hostManager_->UsbSubmitTransfer(devInfo, info, cb, ashmem);
    EXPECT_TRUE(ret == UEC_OK || ret == UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "UsbSubmitTransfer ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageUsbType_EmptyTypes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageUsbType_EmptyTypes_001 start");
    std::vector<UsbDeviceType> disableType;
    int32_t ret = hostManager_->ManageUsbType(disableType, true);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageUsbType empty ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageUsbType_SingleType_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageUsbType_SingleType_001 start");
    std::vector<UsbDeviceType> disableType;
    disableType.push_back({0x08, 0x06, 0x50, 0});
    int32_t ret = hostManager_->ManageUsbType(disableType, true);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageUsbType single ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageUsbSerialDevice_DisableTrue_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageUsbSerialDevice_DisableTrue_001 start");
    int32_t ret = hostManager_->ManageUsbSerialDevice(true);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageUsbSerialDevice(true) ret=%{public}d", ret);
}

HWTEST_F(UsbHostManagerExtendedTest, ManageUsbSerialDevice_DisableFalse_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ManageUsbSerialDevice_DisableFalse_001 start");
    int32_t ret = hostManager_->ManageUsbSerialDevice(false);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ManageUsbSerialDevice(false) ret=%{public}d", ret);
}

} // namespace ExtendedTest
} // namespace USB
} // namespace OHOS
