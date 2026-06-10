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
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

#include "usb_device.h"
#include "usb_config.h"
#include "usb_interface.h"
#include "usb_endpoint.h"
#include "usb_port.h"
#include "usb_accessory.h"
#include "usb_errors.h"
#include "usb_srv_support.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace DeviceModelTest {

class UsbDeviceModelTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void UsbDeviceModelTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceModelTest::SetUpTestCase");
}

void UsbDeviceModelTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDeviceModelTest::TearDownTestCase");
}

void UsbDeviceModelTest::SetUp(void) {}

void UsbDeviceModelTest::TearDown(void) {}

HWTEST_F(UsbDeviceModelTest, UsbDevice_DefaultConstructor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_DefaultConstructor_001 start");
    UsbDevice device;
    EXPECT_EQ(device.GetBusNum(), 0);
    EXPECT_EQ(device.GetDevAddr(), 0);
    EXPECT_EQ(device.GetVendorId(), 0);
    EXPECT_EQ(device.GetProductId(), 0);
    EXPECT_EQ(device.GetClass(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_DefaultConstructor_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetBusNum_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetBusNum_001 start");
    UsbDevice device;
    device.SetBusNum(1);
    EXPECT_EQ(device.GetBusNum(), 1);
    device.SetBusNum(255);
    EXPECT_EQ(device.GetBusNum(), 255);
    device.SetBusNum(0);
    EXPECT_EQ(device.GetBusNum(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetBusNum_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetDevAddr_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetDevAddr_001 start");
    UsbDevice device;
    device.SetDevAddr(1);
    EXPECT_EQ(device.GetDevAddr(), 1);
    device.SetDevAddr(127);
    EXPECT_EQ(device.GetDevAddr(), 127);
    device.SetDevAddr(0);
    EXPECT_EQ(device.GetDevAddr(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetDevAddr_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetVendorId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetVendorId_001 start");
    UsbDevice device;
    device.SetVendorId(0x1234);
    EXPECT_EQ(device.GetVendorId(), 0x1234);
    device.SetVendorId(0xFFFF);
    EXPECT_EQ(device.GetVendorId(), 0xFFFF);
    device.SetVendorId(0x0000);
    EXPECT_EQ(device.GetVendorId(), 0x0000);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetVendorId_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetProductId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetProductId_001 start");
    UsbDevice device;
    device.SetProductId(0x5678);
    EXPECT_EQ(device.GetProductId(), 0x5678);
    device.SetProductId(0xFFFF);
    EXPECT_EQ(device.GetProductId(), 0xFFFF);
    device.SetProductId(0x0000);
    EXPECT_EQ(device.GetProductId(), 0x0000);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetProductId_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetClass_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetClass_001 start");
    UsbDevice device;
    device.SetClass(0x08);
    EXPECT_EQ(device.GetClass(), 0x08);
    device.SetClass(0x03);
    EXPECT_EQ(device.GetClass(), 0x03);
    device.SetClass(0xFF);
    EXPECT_EQ(device.GetClass(), 0xFF);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetClass_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetSubclass_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetSubclass_001 start");
    UsbDevice device;
    device.SetSubclass(0x06);
    EXPECT_EQ(device.GetSubclass(), 0x06);
    device.SetSubclass(0x01);
    EXPECT_EQ(device.GetSubclass(), 0x01);
    device.SetSubclass(0xFF);
    EXPECT_EQ(device.GetSubclass(), 0xFF);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetSubclass_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetProtocol_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetProtocol_001 start");
    UsbDevice device;
    device.SetProtocol(0x50);
    EXPECT_EQ(device.GetProtocol(), 0x50);
    device.SetProtocol(0x00);
    EXPECT_EQ(device.GetProtocol(), 0x00);
    device.SetProtocol(0xFF);
    EXPECT_EQ(device.GetProtocol(), 0xFF);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetProtocol_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetProductName_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetProductName_001 start");
    UsbDevice device;
    device.SetProductName("TestUSBDevice");
    EXPECT_EQ(device.GetName(), "TestUSBDevice");
    device.SetProductName("");
    EXPECT_EQ(device.GetName(), "");
    device.SetProductName("中文设备名");
    EXPECT_EQ(device.GetName(), "中文设备名");
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetProductName_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetManufacturerName_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetManufacturerName_001 start");
    UsbDevice device;
    device.SetManufacturerName("TestMfg");
    EXPECT_EQ(device.GetManufacturerName(), "TestMfg");
    device.SetManufacturerName("");
    EXPECT_EQ(device.GetManufacturerName(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetManufacturerName_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetSerial_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetSerial_001 start");
    UsbDevice device;
    device.SetmSerial("SN12345678");
    EXPECT_EQ(device.GetmSerial(), "SN12345678");
    device.SetmSerial("");
    EXPECT_EQ(device.GetmSerial(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetSerial_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetVersion_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetVersion_001 start");
    UsbDevice device;
    device.SetVersion("1.0.0");
    EXPECT_EQ(device.GetVersion(), "1.0.0");
    device.SetVersion("2.0.0");
    EXPECT_EQ(device.GetVersion(), "2.0.0");
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetVersion_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetAuthorizeStatus_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetAuthorizeStatus_001 start");
    UsbDevice device;
    device.SetAuthorizeStatus(ENABLED);
    EXPECT_EQ(device.GetAuthorizeStatus(), ENABLED);
    device.SetAuthorizeStatus(DISABLED);
    EXPECT_EQ(device.GetAuthorizeStatus(), DISABLED);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetAuthorizeStatus_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_SetConfigs_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetConfigs_001 start");
    UsbDevice device;
    std::vector<USBConfig> configs;
    USBConfig config;
    config.SetId(1);
    config.SetAttributes(0x80);
    configs.push_back(config);
    device.SetConfigs(configs);
    EXPECT_EQ(device.GetConfigCount(), 1u);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetConfigs_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_MultipleConfigs_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_MultipleConfigs_001 start");
    UsbDevice device;
    std::vector<USBConfig> configs;
    for (int i = 0; i < 5; i++) {
        USBConfig config;
        config.SetId(i);
        config.SetAttributes(0x80);
        configs.push_back(config);
    }
    device.SetConfigs(configs);
    EXPECT_EQ(device.GetConfigCount(), 5u);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_MultipleConfigs_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_CopyConstructor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_CopyConstructor_001 start");
    UsbDevice device1;
    device1.SetBusNum(1);
    device1.SetDevAddr(2);
    device1.SetVendorId(0x1234);
    device1.SetProductId(0x5678);
    device1.SetClass(0x08);
    device1.SetProductName("TestDevice");

    UsbDevice device2(device1);
    EXPECT_EQ(device2.GetBusNum(), 1);
    EXPECT_EQ(device2.GetDevAddr(), 2);
    EXPECT_EQ(device2.GetVendorId(), 0x1234);
    EXPECT_EQ(device2.GetProductId(), 0x5678);
    EXPECT_EQ(device2.GetClass(), 0x08);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_CopyConstructor_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_AssignmentOperator_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_AssignmentOperator_001 start");
    UsbDevice device1;
    device1.SetBusNum(3);
    device1.SetDevAddr(4);
    device1.SetVendorId(0xABCD);
    device1.SetProductId(0xEF01);

    UsbDevice device2;
    device2 = device1;
    EXPECT_EQ(device2.GetBusNum(), 3);
    EXPECT_EQ(device2.GetDevAddr(), 4);
    EXPECT_EQ(device2.GetVendorId(), 0xABCD);
    EXPECT_EQ(device2.GetProductId(), 0xEF01);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_AssignmentOperator_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_FullPropertySet_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_FullPropertySet_001 start");
    UsbDevice device;
    device.SetBusNum(1);
    device.SetDevAddr(5);
    device.SetVendorId(0x12D1);
    device.SetProductId(0x107E);
    device.SetClass(0x00);
    device.SetSubclass(0x00);
    device.SetProtocol(0x00);
    device.SetProductName("Huawei Phone");
    device.SetManufacturerName("Huawei");
    device.SetmSerial("HW12345678");
    device.SetVersion("3.0.0");
    device.SetAuthorizeStatus(ENABLED);

    EXPECT_EQ(device.GetBusNum(), 1);
    EXPECT_EQ(device.GetDevAddr(), 5);
    EXPECT_EQ(device.GetVendorId(), 0x12D1);
    EXPECT_EQ(device.GetProductId(), 0x107E);
    EXPECT_EQ(device.GetClass(), 0x00);
    EXPECT_EQ(device.GetSubclass(), 0x00);
    EXPECT_EQ(device.GetProtocol(), 0x00);
    EXPECT_EQ(device.GetName(), "Huawei Phone");
    EXPECT_EQ(device.GetManufacturerName(), "Huawei");
    EXPECT_EQ(device.GetmSerial(), "HW12345678");
    EXPECT_EQ(device.GetVersion(), "3.0.0");
    EXPECT_EQ(device.GetAuthorizeStatus(), ENABLED);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_FullPropertySet_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBConfig_DefaultConstructor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_DefaultConstructor_001 start");
    USBConfig config;
    EXPECT_EQ(config.GetId(), 0);
    EXPECT_EQ(config.GetAttributes(), 0);
    EXPECT_EQ(config.GetMaxPower(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_DefaultConstructor_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBConfig_SetId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_SetId_001 start");
    USBConfig config;
    config.SetId(1);
    EXPECT_EQ(config.GetId(), 1);
    config.SetId(255);
    EXPECT_EQ(config.GetId(), 255);
    config.SetId(0);
    EXPECT_EQ(config.GetId(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_SetId_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBConfig_SetAttributes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_SetAttributes_001 start");
    USBConfig config;
    config.SetAttributes(0x80);
    EXPECT_EQ(config.GetAttributes(), 0x80);
    config.SetAttributes(0xC0);
    EXPECT_EQ(config.GetAttributes(), 0xC0);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_SetAttributes_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBConfig_SetMaxPower_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_SetMaxPower_001 start");
    USBConfig config;
    config.SetMaxPower(50);
    EXPECT_EQ(config.GetMaxPower(), 50);
    config.SetMaxPower(500);
    EXPECT_EQ(config.GetMaxPower(), 500);
    config.SetMaxPower(0);
    EXPECT_EQ(config.GetMaxPower(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_SetMaxPower_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBConfig_SetName_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_SetName_001 start");
    USBConfig config;
    config.SetName("Config1");
    EXPECT_EQ(config.GetName(), "Config1");
    config.SetName("");
    EXPECT_EQ(config.GetName(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_SetName_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBConfig_SetInterfaces_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_SetInterfaces_001 start");
    USBConfig config;
    std::vector<UsbInterface> interfaces;
    UsbInterface iface;
    iface.SetId(0);
    iface.SetClass(0x08);
    interfaces.push_back(iface);
    config.SetInterfaces(interfaces);
    EXPECT_EQ(config.GetInterfaceCount(), 1u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_SetInterfaces_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBConfig_MultipleInterfaces_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_MultipleInterfaces_001 start");
    USBConfig config;
    std::vector<UsbInterface> interfaces;
    for (int i = 0; i < 4; i++) {
        UsbInterface iface;
        iface.SetId(i);
        iface.SetClass(0x08);
        interfaces.push_back(iface);
    }
    config.SetInterfaces(interfaces);
    EXPECT_EQ(config.GetInterfaceCount(), 4u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_MultipleInterfaces_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBConfig_FullPropertySet_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_FullPropertySet_001 start");
    USBConfig config;
    config.SetId(1);
    config.SetAttributes(0x80);
    config.SetMaxPower(250);
    config.SetName("FullConfig");
    config.SetIsSelfPowered(true);
    config.SetIsRemoteWakeup(true);

    EXPECT_EQ(config.GetId(), 1);
    EXPECT_EQ(config.GetAttributes(), 0x80);
    EXPECT_EQ(config.GetMaxPower(), 250);
    EXPECT_EQ(config.GetName(), "FullConfig");
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_FullPropertySet_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbInterface_DefaultConstructor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_DefaultConstructor_001 start");
    UsbInterface iface;
    EXPECT_EQ(iface.GetId(), 0);
    EXPECT_EQ(iface.GetClass(), 0);
    EXPECT_EQ(iface.GetSubClass(), 0);
    EXPECT_EQ(iface.GetProtocol(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_DefaultConstructor_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbInterface_SetId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetId_001 start");
    UsbInterface iface;
    iface.SetId(0);
    EXPECT_EQ(iface.GetId(), 0);
    iface.SetId(1);
    EXPECT_EQ(iface.GetId(), 1);
    iface.SetId(255);
    EXPECT_EQ(iface.GetId(), 255);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetId_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbInterface_SetClass_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetClass_001 start");
    UsbInterface iface;
    iface.SetClass(0x08);
    EXPECT_EQ(iface.GetClass(), 0x08);
    iface.SetClass(0x03);
    EXPECT_EQ(iface.GetClass(), 0x03);
    iface.SetClass(0xFF);
    EXPECT_EQ(iface.GetClass(), 0xFF);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetClass_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbInterface_SetSubClass_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetSubClass_001 start");
    UsbInterface iface;
    iface.SetSubClass(0x06);
    EXPECT_EQ(iface.GetSubClass(), 0x06);
    iface.SetSubClass(0x01);
    EXPECT_EQ(iface.GetSubClass(), 0x01);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetSubClass_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbInterface_SetProtocol_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetProtocol_001 start");
    UsbInterface iface;
    iface.SetProtocol(0x50);
    EXPECT_EQ(iface.GetProtocol(), 0x50);
    iface.SetProtocol(0x00);
    EXPECT_EQ(iface.GetProtocol(), 0x00);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetProtocol_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbInterface_SetName_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetName_001 start");
    UsbInterface iface;
    iface.SetName("MassStorage");
    EXPECT_EQ(iface.GetName(), "MassStorage");
    iface.SetName("");
    EXPECT_EQ(iface.GetName(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetName_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbInterface_SetAlternateSetting_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetAlternateSetting_001 start");
    UsbInterface iface;
    iface.SetAlternateSetting(0);
    EXPECT_EQ(iface.GetAlternateSetting(), 0);
    iface.SetAlternateSetting(1);
    EXPECT_EQ(iface.GetAlternateSetting(), 1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetAlternateSetting_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbInterface_SetEndpoints_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetEndpoints_001 start");
    UsbInterface iface;
    std::vector<USBEndpoint> endpoints;
    USBEndpoint ep;
    ep.SetAddress(0x81);
    endpoints.push_back(ep);
    iface.SetEndpoints(endpoints);
    EXPECT_EQ(iface.GetEndpointCount(), 1u);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_SetEndpoints_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbInterface_FullPropertySet_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_FullPropertySet_001 start");
    UsbInterface iface;
    iface.SetId(2);
    iface.SetClass(0x08);
    iface.SetSubClass(0x06);
    iface.SetProtocol(0x50);
    iface.SetName("SCSI_Bulk");
    iface.SetAlternateSetting(0);

    EXPECT_EQ(iface.GetId(), 2);
    EXPECT_EQ(iface.GetClass(), 0x08);
    EXPECT_EQ(iface.GetSubClass(), 0x06);
    EXPECT_EQ(iface.GetProtocol(), 0x50);
    EXPECT_EQ(iface.GetName(), "SCSI_Bulk");
    EXPECT_EQ(iface.GetAlternateSetting(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_FullPropertySet_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBEndpoint_DefaultConstructor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_DefaultConstructor_001 start");
    USBEndpoint endpoint;
    EXPECT_EQ(endpoint.GetAddress(), 0);
    EXPECT_EQ(endpoint.GetInterfaceId(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_DefaultConstructor_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBEndpoint_SetAddress_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetAddress_001 start");
    USBEndpoint endpoint;
    endpoint.SetAddress(0x81);
    EXPECT_EQ(endpoint.GetAddress(), 0x81);
    endpoint.SetAddress(0x02);
    EXPECT_EQ(endpoint.GetAddress(), 0x02);
    endpoint.SetAddress(0x00);
    EXPECT_EQ(endpoint.GetAddress(), 0x00);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetAddress_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBEndpoint_SetInterfaceId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetInterfaceId_001 start");
    USBEndpoint endpoint;
    endpoint.SetInterfaceId(0);
    EXPECT_EQ(endpoint.GetInterfaceId(), 0);
    endpoint.SetInterfaceId(1);
    EXPECT_EQ(endpoint.GetInterfaceId(), 1);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetInterfaceId_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBEndpoint_SetDirection_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetDirection_001 start");
    USBEndpoint endpoint;
    endpoint.SetDirection(USB_ENDPOINT_DIR_IN);
    EXPECT_EQ(endpoint.GetDirection(), USB_ENDPOINT_DIR_IN);
    endpoint.SetDirection(USB_ENDPOINT_DIR_OUT);
    EXPECT_EQ(endpoint.GetDirection(), USB_ENDPOINT_DIR_OUT);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetDirection_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBEndpoint_SetType_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetType_001 start");
    USBEndpoint endpoint;
    endpoint.SetType(USB_ENDPOINT_XFER_CONTROL);
    EXPECT_EQ(endpoint.GetType(), USB_ENDPOINT_XFER_CONTROL);
    endpoint.SetType(USB_ENDPOINT_XFER_ISOC);
    EXPECT_EQ(endpoint.GetType(), USB_ENDPOINT_XFER_ISOC);
    endpoint.SetType(USB_ENDPOINT_XFER_BULK);
    EXPECT_EQ(endpoint.GetType(), USB_ENDPOINT_XFER_BULK);
    endpoint.SetType(USB_ENDPOINT_XFER_INT);
    EXPECT_EQ(endpoint.GetType(), USB_ENDPOINT_XFER_INT);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetType_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBEndpoint_SetMaxPacketSize_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetMaxPacketSize_001 start");
    USBEndpoint endpoint;
    endpoint.SetMaxPacketSize(64);
    EXPECT_EQ(endpoint.GetMaxPacketSize(), 64);
    endpoint.SetMaxPacketSize(512);
    EXPECT_EQ(endpoint.GetMaxPacketSize(), 512);
    endpoint.SetMaxPacketSize(1024);
    EXPECT_EQ(endpoint.GetMaxPacketSize(), 1024);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetMaxPacketSize_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBEndpoint_SetInterval_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetInterval_001 start");
    USBEndpoint endpoint;
    endpoint.SetInterval(1);
    EXPECT_EQ(endpoint.GetInterval(), 1);
    endpoint.SetInterval(10);
    EXPECT_EQ(endpoint.GetInterval(), 10);
    endpoint.SetInterval(255);
    EXPECT_EQ(endpoint.GetInterval(), 255);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_SetInterval_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBEndpoint_FullPropertySet_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_FullPropertySet_001 start");
    USBEndpoint endpoint;
    endpoint.SetAddress(0x81);
    endpoint.SetInterfaceId(0);
    endpoint.SetDirection(USB_ENDPOINT_DIR_IN);
    endpoint.SetType(USB_ENDPOINT_XFER_BULK);
    endpoint.SetMaxPacketSize(512);
    endpoint.SetInterval(0);

    EXPECT_EQ(endpoint.GetAddress(), 0x81);
    EXPECT_EQ(endpoint.GetInterfaceId(), 0);
    EXPECT_EQ(endpoint.GetDirection(), USB_ENDPOINT_DIR_IN);
    EXPECT_EQ(endpoint.GetType(), USB_ENDPOINT_XFER_BULK);
    EXPECT_EQ(endpoint.GetMaxPacketSize(), 512);
    EXPECT_EQ(endpoint.GetInterval(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_FullPropertySet_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbPort_DefaultConstructor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_DefaultConstructor_001 start");
    UsbPort port;
    EXPECT_EQ(port.GetId(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_DefaultConstructor_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbPort_SetId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_SetId_001 start");
    UsbPort port;
    port.SetId(0);
    EXPECT_EQ(port.GetId(), 0);
    port.SetId(1);
    EXPECT_EQ(port.GetId(), 1);
    port.SetId(255);
    EXPECT_EQ(port.GetId(), 255);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_SetId_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbPort_SetSupportedModes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_SetSupportedModes_001 start");
    UsbPort port;
    port.SetSupportedModes(0);
    EXPECT_EQ(port.GetSupportedModes(), 0);
    port.SetSupportedModes(1);
    EXPECT_EQ(port.GetSupportedModes(), 1);
    port.SetSupportedModes(3);
    EXPECT_EQ(port.GetSupportedModes(), 3);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_SetSupportedModes_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBAccessory_DefaultConstructor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_DefaultConstructor_001 start");
    USBAccessory accessory;
    EXPECT_EQ(accessory.GetManufacturer(), "");
    EXPECT_EQ(accessory.GetModel(), "");
    EXPECT_EQ(accessory.GetDescription(), "");
    EXPECT_EQ(accessory.GetVersion(), "");
    EXPECT_EQ(accessory.GetUri(), "");
    EXPECT_EQ(accessory.GetSerial(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_DefaultConstructor_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBAccessory_SetManufacturer_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetManufacturer_001 start");
    USBAccessory accessory;
    accessory.SetManufacturer("TestMfg");
    EXPECT_EQ(accessory.GetManufacturer(), "TestMfg");
    accessory.SetManufacturer("");
    EXPECT_EQ(accessory.GetManufacturer(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetManufacturer_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBAccessory_SetModel_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetModel_001 start");
    USBAccessory accessory;
    accessory.SetModel("TestModel");
    EXPECT_EQ(accessory.GetModel(), "TestModel");
    accessory.SetModel("");
    EXPECT_EQ(accessory.GetModel(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetModel_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBAccessory_SetDescription_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetDescription_001 start");
    USBAccessory accessory;
    accessory.SetDescription("Test Description");
    EXPECT_EQ(accessory.GetDescription(), "Test Description");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetDescription_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBAccessory_SetVersion_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetVersion_001 start");
    USBAccessory accessory;
    accessory.SetVersion("1.0");
    EXPECT_EQ(accessory.GetVersion(), "1.0");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetVersion_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBAccessory_SetUri_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetUri_001 start");
    USBAccessory accessory;
    accessory.SetUri("http://test.uri");
    EXPECT_EQ(accessory.GetUri(), "http://test.uri");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetUri_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBAccessory_SetSerial_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetSerial_001 start");
    USBAccessory accessory;
    accessory.SetSerial("ACC12345");
    EXPECT_EQ(accessory.GetSerial(), "ACC12345");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetSerial_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBAccessory_FullPropertySet_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_FullPropertySet_001 start");
    USBAccessory accessory;
    accessory.SetManufacturer("TestMfg");
    accessory.SetModel("TestModel");
    accessory.SetDescription("Test Desc");
    accessory.SetVersion("2.0");
    accessory.SetUri("http://example.com");
    accessory.SetSerial("SN00001");

    EXPECT_EQ(accessory.GetManufacturer(), "TestMfg");
    EXPECT_EQ(accessory.GetModel(), "TestModel");
    EXPECT_EQ(accessory.GetDescription(), "Test Desc");
    EXPECT_EQ(accessory.GetVersion(), "2.0");
    EXPECT_EQ(accessory.GetUri(), "http://example.com");
    EXPECT_EQ(accessory.GetSerial(), "SN00001");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_FullPropertySet_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbSrvSupport_FunctionConstants_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSrvSupport_FunctionConstants_001 start");
    EXPECT_NE(UsbSrvSupport::FUNCTION_NONE, 0);
    EXPECT_NE(UsbSrvSupport::FUNCTION_ACM, 0);
    EXPECT_NE(UsbSrvSupport::FUNCTION_ECM, 0);
    EXPECT_NE(UsbSrvSupport::FUNCTION_HDC, 0);
    EXPECT_NE(UsbSrvSupport::FUNCTION_MTP, 0);
    EXPECT_NE(UsbSrvSupport::FUNCTION_PTP, 0);
    EXPECT_NE(UsbSrvSupport::FUNCTION_RNDIS, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbSrvSupport_FunctionConstants_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_HierarchyConstruction_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_HierarchyConstruction_001 start");
    USBEndpoint ep1;
    ep1.SetAddress(0x81);
    ep1.SetInterfaceId(0);
    ep1.SetDirection(USB_ENDPOINT_DIR_IN);
    ep1.SetType(USB_ENDPOINT_XFER_BULK);
    ep1.SetMaxPacketSize(512);

    USBEndpoint ep2;
    ep2.SetAddress(0x02);
    ep2.SetInterfaceId(0);
    ep2.SetDirection(USB_ENDPOINT_DIR_OUT);
    ep2.SetType(USB_ENDPOINT_XFER_BULK);
    ep2.SetMaxPacketSize(512);

    std::vector<USBEndpoint> endpoints;
    endpoints.push_back(ep1);
    endpoints.push_back(ep2);

    UsbInterface iface;
    iface.SetId(0);
    iface.SetClass(0x08);
    iface.SetSubClass(0x06);
    iface.SetProtocol(0x50);
    iface.SetEndpoints(endpoints);
    EXPECT_EQ(iface.GetEndpointCount(), 2u);

    std::vector<UsbInterface> interfaces;
    interfaces.push_back(iface);

    USBConfig config;
    config.SetId(1);
    config.SetAttributes(0x80);
    config.SetMaxPower(250);
    config.SetInterfaces(interfaces);
    EXPECT_EQ(config.GetInterfaceCount(), 1u);

    std::vector<USBConfig> configs;
    configs.push_back(config);

    UsbDevice device;
    device.SetBusNum(1);
    device.SetDevAddr(1);
    device.SetVendorId(0x1234);
    device.SetProductId(0x5678);
    device.SetConfigs(configs);
    EXPECT_EQ(device.GetConfigCount(), 1u);

    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_HierarchyConstruction_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_MultipleHierarchyConstruction_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_MultipleHierarchyConstruction_001 start");
    USBEndpoint epIn;
    epIn.SetAddress(0x83);
    epIn.SetDirection(USB_ENDPOINT_DIR_IN);
    epIn.SetType(USB_ENDPOINT_XFER_INT);
    epIn.SetMaxPacketSize(8);

    USBEndpoint epOut;
    epOut.SetAddress(0x04);
    epOut.SetDirection(USB_ENDPOINT_DIR_OUT);
    epOut.SetType(USB_ENDPOINT_XFER_INT);
    epOut.SetMaxPacketSize(8);

    std::vector<USBEndpoint> eps;
    eps.push_back(epIn);
    eps.push_back(epOut);

    UsbInterface iface1;
    iface1.SetId(0);
    iface1.SetClass(0x03);
    iface1.SetSubClass(0x01);
    iface1.SetProtocol(0x01);
    iface1.SetEndpoints(eps);

    UsbInterface iface2;
    iface2.SetId(1);
    iface2.SetClass(0x03);
    iface2.SetSubClass(0x01);
    iface2.SetProtocol(0x02);
    iface2.SetEndpoints(eps);

    std::vector<UsbInterface> ifaces;
    ifaces.push_back(iface1);
    ifaces.push_back(iface2);

    USBConfig config;
    config.SetId(1);
    config.SetInterfaces(ifaces);

    std::vector<USBConfig> configs;
    configs.push_back(config);

    UsbDevice device;
    device.SetBusNum(2);
    device.SetDevAddr(3);
    device.SetVendorId(0x5678);
    device.SetProductId(0xAAAA);
    device.SetClass(0x03);
    device.SetConfigs(configs);

    EXPECT_EQ(device.GetConfigCount(), 1u);
    EXPECT_EQ(device.GetClass(), 0x03);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_MultipleHierarchyConstruction_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_DeviceClassMapping_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_DeviceClassMapping_001 start");
    std::vector<uint8_t> deviceClasses = {
        0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
        0x12, 0xDC, 0xE0, 0xEF, 0xFE, 0xFF
    };
    for (auto cls : deviceClasses) {
        UsbDevice device;
        device.SetClass(cls);
        EXPECT_EQ(device.GetClass(), cls);
        USB_HILOGI(MODULE_USB_SERVICE, "Device class 0x%{public}02X verified", cls);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_DeviceClassMapping_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbDevice_VariousVendorProducts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_VariousVendorProducts_001 start");
    std::vector<std::pair<uint16_t, uint16_t>> vendorProducts = {
        {0x1234, 0x5678}, {0x12D1, 0x107E}, {0x04E8, 0x6860},
        {0x046D, 0xC52B}, {0x05AC, 0x12AB}, {0x045E, 0x00DB},
        {0x0000, 0x0000}, {0xFFFF, 0xFFFF}
    };
    for (auto& vp : vendorProducts) {
        UsbDevice device;
        device.SetVendorId(vp.first);
        device.SetProductId(vp.second);
        EXPECT_EQ(device.GetVendorId(), vp.first);
        EXPECT_EQ(device.GetProductId(), vp.second);
        USB_HILOGI(MODULE_USB_SERVICE, "VID=0x%{public}04X PID=0x%{public}04X",
                    vp.first, vp.second);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_VariousVendorProducts_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBEndpoint_DirectionTypes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_DirectionTypes_001 start");
    USBEndpoint epIn;
    epIn.SetAddress(0x81);
    epIn.SetDirection(USB_ENDPOINT_DIR_IN);
    EXPECT_EQ(epIn.GetDirection(), USB_ENDPOINT_DIR_IN);

    USBEndpoint epOut;
    epOut.SetAddress(0x01);
    epOut.SetDirection(USB_ENDPOINT_DIR_OUT);
    EXPECT_EQ(epOut.GetDirection(), USB_ENDPOINT_DIR_OUT);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_DirectionTypes_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBEndpoint_TransferTypes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_TransferTypes_001 start");
    std::vector<uint8_t> transferTypes = {
        USB_ENDPOINT_XFER_CONTROL,
        USB_ENDPOINT_XFER_ISOC,
        USB_ENDPOINT_XFER_BULK,
        USB_ENDPOINT_XFER_INT
    };
    for (auto type : transferTypes) {
        USBEndpoint ep;
        ep.SetType(type);
        EXPECT_EQ(ep.GetType(), type);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_TransferTypes_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbPort_SupportedModes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_SupportedModes_001 start");
    UsbPort port;
    port.SetSupportedModes(0);
    EXPECT_EQ(port.GetSupportedModes(), 0);
    port.SetSupportedModes(1);
    EXPECT_EQ(port.GetSupportedModes(), 1);
    port.SetSupportedModes(2);
    EXPECT_EQ(port.GetSupportedModes(), 2);
    port.SetSupportedModes(3);
    EXPECT_EQ(port.GetSupportedModes(), 3);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_SupportedModes_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBAccessory_CopyConstructor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_CopyConstructor_001 start");
    USBAccessory acc1;
    acc1.SetManufacturer("Mfg1");
    acc1.SetModel("Model1");
    acc1.SetDescription("Desc1");
    acc1.SetVersion("1.0");
    acc1.SetUri("http://test.com");
    acc1.SetSerial("SN001");

    USBAccessory acc2(acc1);
    EXPECT_EQ(acc2.GetManufacturer(), "Mfg1");
    EXPECT_EQ(acc2.GetModel(), "Model1");
    EXPECT_EQ(acc2.GetDescription(), "Desc1");
    EXPECT_EQ(acc2.GetVersion(), "1.0");
    EXPECT_EQ(acc2.GetUri(), "http://test.com");
    EXPECT_EQ(acc2.GetSerial(), "SN001");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_CopyConstructor_001 end");
}

HWTEST_F(UsbDeviceModelTest, USBAccessory_AssignmentOperator_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_AssignmentOperator_001 start");
    USBAccessory acc1;
    acc1.SetManufacturer("Mfg2");
    acc1.SetModel("Model2");

    USBAccessory acc2;
    acc2 = acc1;
    EXPECT_EQ(acc2.GetManufacturer(), "Mfg2");
    EXPECT_EQ(acc2.GetModel(), "Model2");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_AssignmentOperator_001 end");
}

HWTEST_F(UsbDeviceModelTest, UsbErrCode_Values_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbErrCode_Values_001 start");
    EXPECT_EQ(UEC_OK, 0);
    EXPECT_NE(UEC_SERVICE_INVALID_VALUE, 0);
    EXPECT_NE(UEC_SERVICE_INNER_ERR, 0);
    EXPECT_LT(UEC_SERVICE_INVALID_VALUE, 0);
    EXPECT_LT(UEC_SERVICE_INNER_ERR, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbErrCode_Values_001 end");
}

} // namespace DeviceModelTest
} // namespace USB
} // namespace OHOS
