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
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "usb_config.h"
#include "usb_descriptor_parser.h"
#include "usb_device.h"
#include "usb_endpoint.h"
#include "usb_errors.h"
#include "usb_interface.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;
using namespace testing::ext;

constexpr uint8_t DEVICE_DESC_LENGTH = 18;
constexpr uint8_t CONFIG_DESC_LENGTH = 9;
constexpr uint8_t INTERFACE_DESC_LENGTH = 9;
constexpr uint8_t ENDPOINT_DESC_LENGTH = 7;
constexpr uint8_t AUDIO_ENDPOINT_DESC_LENGTH = 9;

constexpr uint8_t DESC_TYPE_DEVICE = 1;
constexpr uint8_t DESC_TYPE_CONFIG = 2;
constexpr uint8_t DESC_TYPE_STRING = 3;
constexpr uint8_t DESC_TYPE_INTERFACE = 4;
constexpr uint8_t DESC_TYPE_ENDPOINT = 5;
constexpr uint8_t DESC_TYPE_DEVICE_QUALIFIER = 6;
constexpr uint8_t DESC_TYPE_OTHER_SPEED = 7;
constexpr uint8_t DESC_TYPE_INTERFACE_POWER = 8;
constexpr uint8_t DESC_TYPE_OTG = 9;
constexpr uint8_t DESC_TYPE_DEBUG = 10;
constexpr uint8_t DESC_TYPE_INTERFACE_ASSOC = 11;
constexpr uint8_t DESC_TYPE_HID = 0x21;
constexpr uint8_t DESC_TYPE_CS_INTERFACE = 0x24;
constexpr uint8_t DESC_TYPE_CS_ENDPOINT = 0x25;
constexpr uint8_t DESC_TYPE_UNKNOWN_MAX = 0xFF;
constexpr uint32_t ZERO_OFFSET = 0;
constexpr uint32_t BITS_PER_BYTE = 8;
constexpr uint32_t BYTE_MASK = 0xFF;

constexpr uint16_t TEST_BCD_USB = 0x0200;
constexpr uint16_t TEST_BCD_DEVICE = 0x0100;
constexpr uint16_t TEST_VID = 0x1234;
constexpr uint16_t TEST_PID = 0x5678;
constexpr uint16_t TEST_VID_MAX = 0xFFFF;
constexpr uint16_t TEST_PID_MAX = 0xFFFF;
constexpr uint8_t TEST_DEVICE_CLASS = 0x09;
constexpr uint8_t TEST_DEVICE_SUBCLASS = 0x02;
constexpr uint8_t TEST_DEVICE_PROTOCOL = 0x01;
constexpr uint8_t TEST_MAX_PACKET_SIZE0 = 0x40;
constexpr uint8_t TEST_I_MANUFACTURER = 0x01;
constexpr uint8_t TEST_I_PRODUCT = 0x02;
constexpr uint8_t TEST_I_SERIAL = 0x03;
constexpr uint8_t TEST_NUM_CONFIGS = 0x01;
constexpr uint8_t TEST_NUM_CONFIGS_MULTI = 0x03;

constexpr uint16_t TEST_CONFIG_TOTAL_LENGTH = 0x0020;
constexpr uint8_t TEST_CONFIG_NUM_INTERFACES = 0x01;
constexpr uint8_t TEST_CONFIG_VALUE = 0x01;
constexpr uint8_t TEST_CONFIG_VALUE_SECOND = 0x02;
constexpr uint8_t TEST_CONFIG_VALUE_THIRD = 0x03;
constexpr uint8_t TEST_CONFIG_INDEX = 0x04;
constexpr uint8_t TEST_CONFIG_ATTRIBUTES = 0xE0;
constexpr uint8_t TEST_CONFIG_ATTRIBUTES_BUS = 0x80;
constexpr uint8_t TEST_CONFIG_MAX_POWER = 0xFA;
constexpr uint8_t TEST_CONFIG_MAX_POWER_ZERO = 0x00;

constexpr uint8_t TEST_INTERFACE_NUMBER = 0x00;
constexpr uint8_t TEST_INTERFACE_NUMBER_SECOND = 0x01;
constexpr uint8_t TEST_INTERFACE_NUMBER_THIRD = 0x02;
constexpr uint8_t TEST_ALTERNATE_SETTING = 0x00;
constexpr uint8_t TEST_ALTERNATE_SETTING_ALT = 0x01;
constexpr uint8_t TEST_NUM_ENDPOINTS = 0x02;
constexpr uint8_t TEST_INTERFACE_CLASS = 0x08;
constexpr uint8_t TEST_INTERFACE_SUBCLASS = 0x06;
constexpr uint8_t TEST_INTERFACE_PROTOCOL = 0x50;
constexpr uint8_t TEST_I_INTERFACE = 0x05;

constexpr uint8_t TEST_EP_ADDR_IN = 0x81;
constexpr uint8_t TEST_EP_ADDR_OUT = 0x02;
constexpr uint8_t TEST_EP_ADDR_IN_SECOND = 0x83;
constexpr uint8_t TEST_EP_ATTR_CONTROL = 0x00;
constexpr uint8_t TEST_EP_ATTR_ISOC = 0x01;
constexpr uint8_t TEST_EP_ATTR_BULK = 0x02;
constexpr uint8_t TEST_EP_ATTR_INTERRUPT = 0x03;
constexpr uint16_t TEST_EP_MAX_PACKET_SIZE = 0x0200;
constexpr uint16_t TEST_EP_MAX_PACKET_SIZE_SMALL = 0x0040;
constexpr uint8_t TEST_EP_INTERVAL = 0x0A;

constexpr uint8_t PAD_BYTE = 0x00;
constexpr size_t LARGE_BUFFER_SIZE = 4096;
constexpr size_t STRESS_LOOP_COUNT = 64;
constexpr size_t MANY_ENDPOINT_COUNT = 15;
constexpr size_t MANY_INTERFACE_COUNT = 8;
constexpr size_t MANY_CONFIG_COUNT = 5;
constexpr uint32_t EXPECTED_SINGLE = 1;
constexpr uint32_t EXPECTED_NONE = 0;

struct TestDeviceDesc {
    uint8_t bLength = DEVICE_DESC_LENGTH;
    uint8_t bDescriptorType = DESC_TYPE_DEVICE;
    uint16_t bcdUsb = TEST_BCD_USB;
    uint8_t deviceClass = TEST_DEVICE_CLASS;
    uint8_t deviceSubClass = TEST_DEVICE_SUBCLASS;
    uint8_t deviceProtocol = TEST_DEVICE_PROTOCOL;
    uint8_t maxPacketSize0 = TEST_MAX_PACKET_SIZE0;
    uint16_t idVendor = TEST_VID;
    uint16_t idProduct = TEST_PID;
    uint16_t bcdDevice = TEST_BCD_DEVICE;
    uint8_t iManufacturer = TEST_I_MANUFACTURER;
    uint8_t iProduct = TEST_I_PRODUCT;
    uint8_t iSerialNumber = TEST_I_SERIAL;
    uint8_t numConfigurations = TEST_NUM_CONFIGS;
};

struct TestConfigDesc {
    uint8_t bLength = CONFIG_DESC_LENGTH;
    uint8_t bDescriptorType = DESC_TYPE_CONFIG;
    uint16_t wTotalLength = TEST_CONFIG_TOTAL_LENGTH;
    uint8_t numInterfaces = TEST_CONFIG_NUM_INTERFACES;
    uint8_t configurationValue = TEST_CONFIG_VALUE;
    uint8_t iConfiguration = TEST_CONFIG_INDEX;
    uint8_t bmAttributes = TEST_CONFIG_ATTRIBUTES;
    uint8_t maxPower = TEST_CONFIG_MAX_POWER;
};

struct TestInterfaceDesc {
    uint8_t bLength = INTERFACE_DESC_LENGTH;
    uint8_t bDescriptorType = DESC_TYPE_INTERFACE;
    uint8_t interfaceNumber = TEST_INTERFACE_NUMBER;
    uint8_t alternateSetting = TEST_ALTERNATE_SETTING;
    uint8_t numEndpoints = TEST_NUM_ENDPOINTS;
    uint8_t interfaceClass = TEST_INTERFACE_CLASS;
    uint8_t interfaceSubClass = TEST_INTERFACE_SUBCLASS;
    uint8_t interfaceProtocol = TEST_INTERFACE_PROTOCOL;
    uint8_t iInterface = TEST_I_INTERFACE;
};

struct TestEndpointDesc {
    uint8_t bLength = ENDPOINT_DESC_LENGTH;
    uint8_t bDescriptorType = DESC_TYPE_ENDPOINT;
    uint8_t endpointAddress = TEST_EP_ADDR_IN;
    uint8_t bmAttributes = TEST_EP_ATTR_BULK;
    uint16_t wMaxPacketSize = TEST_EP_MAX_PACKET_SIZE;
    uint8_t interval = TEST_EP_INTERVAL;
};

class UsbDescriptorParserExtendedTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static void AppendUint8(std::vector<uint8_t> &buffer, uint8_t value);
    static void AppendUint16(std::vector<uint8_t> &buffer, uint16_t value);
    static std::vector<uint8_t> BuildDeviceDescriptor(const TestDeviceDesc &desc);
    static void AppendConfigDescriptor(std::vector<uint8_t> &buffer, const TestConfigDesc &desc);
    static void AppendInterfaceDescriptor(std::vector<uint8_t> &buffer, const TestInterfaceDesc &desc);
    static void AppendEndpointDescriptor(std::vector<uint8_t> &buffer, const TestEndpointDesc &desc);
    static void AppendUnknownDescriptor(std::vector<uint8_t> &buffer, uint8_t length, uint8_t type);
    static std::vector<uint8_t> BuildSimpleTopology();
};

void UsbDescriptorParserExtendedTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserExtendedTest::SetUpTestCase enter");
}

void UsbDescriptorParserExtendedTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserExtendedTest::TearDownTestCase enter");
}

void UsbDescriptorParserExtendedTest::SetUp()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserExtendedTest::SetUp enter");
}

void UsbDescriptorParserExtendedTest::TearDown()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserExtendedTest::TearDown enter");
}

void UsbDescriptorParserExtendedTest::AppendUint8(std::vector<uint8_t> &buffer, uint8_t value)
{
    buffer.push_back(value);
}

void UsbDescriptorParserExtendedTest::AppendUint16(std::vector<uint8_t> &buffer, uint16_t value)
{
    buffer.push_back(static_cast<uint8_t>(value & BYTE_MASK));
    buffer.push_back(static_cast<uint8_t>((value >> BITS_PER_BYTE) & BYTE_MASK));
}

std::vector<uint8_t> UsbDescriptorParserExtendedTest::BuildDeviceDescriptor(const TestDeviceDesc &desc)
{
    std::vector<uint8_t> buffer;
    AppendUint8(buffer, desc.bLength);
    AppendUint8(buffer, desc.bDescriptorType);
    AppendUint16(buffer, desc.bcdUsb);
    AppendUint8(buffer, desc.deviceClass);
    AppendUint8(buffer, desc.deviceSubClass);
    AppendUint8(buffer, desc.deviceProtocol);
    AppendUint8(buffer, desc.maxPacketSize0);
    AppendUint16(buffer, desc.idVendor);
    AppendUint16(buffer, desc.idProduct);
    AppendUint16(buffer, desc.bcdDevice);
    AppendUint8(buffer, desc.iManufacturer);
    AppendUint8(buffer, desc.iProduct);
    AppendUint8(buffer, desc.iSerialNumber);
    AppendUint8(buffer, desc.numConfigurations);
    return buffer;
}

void UsbDescriptorParserExtendedTest::AppendConfigDescriptor(std::vector<uint8_t> &buffer, const TestConfigDesc &desc)
{
    AppendUint8(buffer, desc.bLength);
    AppendUint8(buffer, desc.bDescriptorType);
    AppendUint16(buffer, desc.wTotalLength);
    AppendUint8(buffer, desc.numInterfaces);
    AppendUint8(buffer, desc.configurationValue);
    AppendUint8(buffer, desc.iConfiguration);
    AppendUint8(buffer, desc.bmAttributes);
    AppendUint8(buffer, desc.maxPower);
}

void UsbDescriptorParserExtendedTest::AppendInterfaceDescriptor(std::vector<uint8_t> &buffer,
    const TestInterfaceDesc &desc)
{
    AppendUint8(buffer, desc.bLength);
    AppendUint8(buffer, desc.bDescriptorType);
    AppendUint8(buffer, desc.interfaceNumber);
    AppendUint8(buffer, desc.alternateSetting);
    AppendUint8(buffer, desc.numEndpoints);
    AppendUint8(buffer, desc.interfaceClass);
    AppendUint8(buffer, desc.interfaceSubClass);
    AppendUint8(buffer, desc.interfaceProtocol);
    AppendUint8(buffer, desc.iInterface);
}

void UsbDescriptorParserExtendedTest::AppendEndpointDescriptor(std::vector<uint8_t> &buffer,
    const TestEndpointDesc &desc)
{
    AppendUint8(buffer, desc.bLength);
    AppendUint8(buffer, desc.bDescriptorType);
    AppendUint8(buffer, desc.endpointAddress);
    AppendUint8(buffer, desc.bmAttributes);
    AppendUint16(buffer, desc.wMaxPacketSize);
    AppendUint8(buffer, desc.interval);
    for (uint8_t i = ENDPOINT_DESC_LENGTH; i < desc.bLength; ++i) {
        AppendUint8(buffer, PAD_BYTE);
    }
}

void UsbDescriptorParserExtendedTest::AppendUnknownDescriptor(std::vector<uint8_t> &buffer, uint8_t length,
    uint8_t type)
{
    AppendUint8(buffer, length);
    AppendUint8(buffer, type);
    for (uint8_t i = 2; i < length; ++i) {
        AppendUint8(buffer, PAD_BYTE);
    }
}

std::vector<uint8_t> UsbDescriptorParserExtendedTest::BuildSimpleTopology()
{
    std::vector<uint8_t> buffer;
    TestConfigDesc config;
    AppendConfigDescriptor(buffer, config);
    TestInterfaceDesc interface;
    AppendInterfaceDescriptor(buffer, interface);
    TestEndpointDesc endpointIn;
    endpointIn.endpointAddress = TEST_EP_ADDR_IN;
    AppendEndpointDescriptor(buffer, endpointIn);
    TestEndpointDesc endpointOut;
    endpointOut.endpointAddress = TEST_EP_ADDR_OUT;
    AppendEndpointDescriptor(buffer, endpointOut);
    return buffer;
}

/**
 * @tc.name: ParseDeviceDescriptor_NullBuffer_001
 * @tc.desc: ParseDeviceDescriptor returns error when buffer pointer is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_NullBuffer_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_NullBuffer_001 start");
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(nullptr, DEVICE_DESC_LENGTH, device);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_NullBuffer_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_ZeroLength_001
 * @tc.desc: ParseDeviceDescriptor returns error when length is zero
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_ZeroLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ZeroLength_001 start");
    TestDeviceDesc desc;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(), 0, device);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ZeroLength_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_ShortLength_001
 * @tc.desc: ParseDeviceDescriptor rejects a length smaller than the device descriptor size
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_ShortLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ShortLength_001 start");
    TestDeviceDesc desc;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(), DEVICE_DESC_LENGTH - 1, device);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ShortLength_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_ShortLength_002
 * @tc.desc: ParseDeviceDescriptor rejects every length below the descriptor size
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_ShortLength_002, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ShortLength_002 start");
    TestDeviceDesc desc;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    for (uint32_t length = 1; length < DEVICE_DESC_LENGTH; ++length) {
        UsbDevice device;
        int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(), length, device);
        EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE) << "length=" << length;
    }
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ShortLength_002 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_ExactLength_001
 * @tc.desc: ParseDeviceDescriptor accepts a buffer whose length equals the descriptor size
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_ExactLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ExactLength_001 start");
    TestDeviceDesc desc;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(), DEVICE_DESC_LENGTH, device);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ExactLength_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_LongerLength_001
 * @tc.desc: ParseDeviceDescriptor accepts a buffer longer than the descriptor size
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_LongerLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_LongerLength_001 start");
    TestDeviceDesc desc;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    AppendConfigDescriptor(buffer, TestConfigDesc());
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_LongerLength_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_LargeBuffer_001
 * @tc.desc: ParseDeviceDescriptor handles a large buffer with a valid leading descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_LargeBuffer_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_LargeBuffer_001 start");
    TestDeviceDesc desc;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    buffer.resize(LARGE_BUFFER_SIZE, PAD_BYTE);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_EQ(device.GetVendorId(), static_cast<int32_t>(TEST_VID));
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_LargeBuffer_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_WrongBLength_001
 * @tc.desc: ParseDeviceDescriptor rejects a descriptor whose bLength is too small
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_WrongBLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_WrongBLength_001 start");
    TestDeviceDesc desc;
    desc.bLength = DEVICE_DESC_LENGTH - 1;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_WrongBLength_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_WrongBLength_002
 * @tc.desc: ParseDeviceDescriptor rejects a descriptor whose bLength is too large
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_WrongBLength_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_WrongBLength_002 start");
    TestDeviceDesc desc;
    desc.bLength = DESC_TYPE_UNKNOWN_MAX;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_WrongBLength_002 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_WrongBLength_003
 * @tc.desc: ParseDeviceDescriptor rejects a zero bLength descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_WrongBLength_003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_WrongBLength_003 start");
    TestDeviceDesc desc;
    desc.bLength = 0;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_WrongBLength_003 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_UnexpectedType_001
 * @tc.desc: ParseDeviceDescriptor only validates bLength so a wrong type still parses
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_UnexpectedType_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_UnexpectedType_001 start");
    TestDeviceDesc desc;
    desc.bDescriptorType = DESC_TYPE_CONFIG;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_EQ(device.GetProductId(), static_cast<int32_t>(TEST_PID));
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_UnexpectedType_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_VendorAndProduct_001
 * @tc.desc: ParseDeviceDescriptor maps idVendor and idProduct to the device model
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_VendorAndProduct_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_VendorAndProduct_001 start");
    TestDeviceDesc desc;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);
    EXPECT_EQ(device.GetVendorId(), static_cast<int32_t>(TEST_VID));
    EXPECT_EQ(device.GetProductId(), static_cast<int32_t>(TEST_PID));
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_VendorAndProduct_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_VendorAndProduct_002
 * @tc.desc: ParseDeviceDescriptor maps the maximum vendor and product identifiers
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_VendorAndProduct_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_VendorAndProduct_002 start");
    TestDeviceDesc desc;
    desc.idVendor = TEST_VID_MAX;
    desc.idProduct = TEST_PID_MAX;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);
    EXPECT_EQ(device.GetVendorId(), static_cast<int32_t>(TEST_VID_MAX));
    EXPECT_EQ(device.GetProductId(), static_cast<int32_t>(TEST_PID_MAX));
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_VendorAndProduct_002 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_ClassTriple_001
 * @tc.desc: ParseDeviceDescriptor maps class, subclass and protocol fields
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_ClassTriple_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ClassTriple_001 start");
    TestDeviceDesc desc;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);
    EXPECT_EQ(device.GetClass(), static_cast<int32_t>(TEST_DEVICE_CLASS));
    EXPECT_EQ(device.GetSubclass(), static_cast<int32_t>(TEST_DEVICE_SUBCLASS));
    EXPECT_EQ(device.GetProtocol(), static_cast<int32_t>(TEST_DEVICE_PROTOCOL));
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ClassTriple_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_ClassTriple_002
 * @tc.desc: ParseDeviceDescriptor maps every possible device class value
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_ClassTriple_002, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ClassTriple_002 start");
    std::vector<uint8_t> classCodes = {0x00, 0x01, 0x02, 0x03, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0E, 0xEF, 0xFF};
    for (uint8_t code : classCodes) {
        TestDeviceDesc desc;
        desc.deviceClass = code;
        std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
        UsbDevice device;
        int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
            static_cast<uint32_t>(buffer.size()), device);
        ASSERT_EQ(ret, UEC_OK);
        EXPECT_EQ(device.GetClass(), static_cast<int32_t>(code));
    }
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ClassTriple_002 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_ConfigCount_001
 * @tc.desc: ParseDeviceDescriptor maps bNumConfigurations to the descriptor config count
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_ConfigCount_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ConfigCount_001 start");
    TestDeviceDesc desc;
    desc.numConfigurations = TEST_NUM_CONFIGS_MULTI;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);
    EXPECT_EQ(device.GetDescConfigCount(), TEST_NUM_CONFIGS_MULTI);
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ConfigCount_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_PacketSizeAndBcd_001
 * @tc.desc: ParseDeviceDescriptor maps bMaxPacketSize0, bcdUSB and bcdDevice
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_PacketSizeAndBcd_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_PacketSizeAndBcd_001 start");
    TestDeviceDesc desc;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);
    EXPECT_EQ(device.GetbMaxPacketSize0(), TEST_MAX_PACKET_SIZE0);
    EXPECT_EQ(device.GetbcdUSB(), TEST_BCD_USB);
    EXPECT_EQ(device.GetbcdDevice(), TEST_BCD_DEVICE);
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_PacketSizeAndBcd_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_StringIndexes_001
 * @tc.desc: ParseDeviceDescriptor maps manufacturer, product and serial string indexes
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_StringIndexes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_StringIndexes_001 start");
    TestDeviceDesc desc;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);
    EXPECT_EQ(device.GetiManufacturer(), TEST_I_MANUFACTURER);
    EXPECT_EQ(device.GetiProduct(), TEST_I_PRODUCT);
    EXPECT_EQ(device.GetiSerialNumber(), TEST_I_SERIAL);
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_StringIndexes_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_Repeated_001
 * @tc.desc: ParseDeviceDescriptor can be invoked repeatedly with a stable result
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_Repeated_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_Repeated_001 start");
    TestDeviceDesc desc;
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(desc);
    for (size_t i = 0; i < STRESS_LOOP_COUNT; ++i) {
        UsbDevice device;
        int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
            static_cast<uint32_t>(buffer.size()), device);
        ASSERT_EQ(ret, UEC_OK);
        EXPECT_EQ(device.GetVendorId(), static_cast<int32_t>(TEST_VID));
    }
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_Repeated_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_ReuseDevice_001
 * @tc.desc: ParseDeviceDescriptor overwrites the fields of a reused device object
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_ReuseDevice_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ReuseDevice_001 start");
    UsbDevice device;
    TestDeviceDesc first;
    std::vector<uint8_t> firstBuffer = BuildDeviceDescriptor(first);
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(firstBuffer.data(),
        static_cast<uint32_t>(firstBuffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);

    TestDeviceDesc second;
    second.idVendor = TEST_VID_MAX;
    second.idProduct = TEST_PID_MAX;
    std::vector<uint8_t> secondBuffer = BuildDeviceDescriptor(second);
    ret = UsbDescriptorParser::ParseDeviceDescriptor(secondBuffer.data(),
        static_cast<uint32_t>(secondBuffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);
    EXPECT_EQ(device.GetVendorId(), static_cast<int32_t>(TEST_VID_MAX));
    EXPECT_EQ(device.GetProductId(), static_cast<int32_t>(TEST_PID_MAX));
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ReuseDevice_001 end");
}

/**
 * @tc.name: ParseDeviceDescriptor_ReuseDevice_001
 * @tc.desc: ParseDeviceDescriptor overwrites the fields of a reused device object
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseDeviceDescriptor_ReuseDevice_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ReuseDevice_001 start");
    UsbDevice device;
    TestDeviceDesc first;
    std::vector<uint8_t> firstBuffer = BuildDeviceDescriptor(first);
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(firstBuffer.data(),
        static_cast<uint32_t>(firstBuffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);

    TestDeviceDesc second;
    second.idVendor = TEST_VID_MAX;
    second.idProduct = TEST_PID_MAX;
    std::vector<uint8_t> secondBuffer = BuildDeviceDescriptor(second);
    ret = UsbDescriptorParser::ParseDeviceDescriptor(secondBuffer.data(),
        static_cast<uint32_t>(secondBuffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);
    EXPECT_EQ(device.GetVendorId(), static_cast<int32_t>(TEST_VID_MAX));
    EXPECT_EQ(device.GetProductId(), static_cast<int32_t>(TEST_PID_MAX));
    USB_HILOGI(MODULE_USB_HOST, "ParseDeviceDescriptor_ReuseDevice_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EmptyBuffer_001
 * @tc.desc: ParseConfigDescriptors succeeds and adds nothing for an empty buffer
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EmptyBuffer_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EmptyBuffer_001 start");
    std::vector<uint8_t> buffer;
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_TRUE(configs.empty());
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EmptyBuffer_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_HeaderOnly_001
 * @tc.desc: ParseConfigDescriptors stops when the remaining data is shorter than a header
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_HeaderOnly_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_HeaderOnly_001 start");
    std::vector<uint8_t> buffer = {CONFIG_DESC_LENGTH};
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_TRUE(configs.empty());
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_HeaderOnly_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_OffsetSkipsDeviceDesc_001
 * @tc.desc: ParseConfigDescriptors parses configs located after a device descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_OffsetSkipsDeviceDesc_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_OffsetSkipsDeviceDesc_001 start");
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(TestDeviceDesc());
    std::vector<uint8_t> topology = BuildSimpleTopology();
    buffer.insert(buffer.end(), topology.begin(), topology.end());

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, DEVICE_DESC_LENGTH, configs);
    EXPECT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetInterfaceCount(), EXPECTED_SINGLE);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_OffsetSkipsDeviceDesc_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_SingleConfig_001
 * @tc.desc: ParseConfigDescriptors adds a single configuration descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_SingleConfig_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_SingleConfig_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetId(), static_cast<int32_t>(TEST_CONFIG_VALUE));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_SingleConfig_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_ConfigFields_001
 * @tc.desc: ParseConfigDescriptors maps attributes, max power and configuration index
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_ConfigFields_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ConfigFields_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetAttributes(), static_cast<uint32_t>(TEST_CONFIG_ATTRIBUTES));
    EXPECT_EQ(configs.front().GetMaxPower(), static_cast<int32_t>(TEST_CONFIG_MAX_POWER));
    EXPECT_EQ(configs.front().GetiConfiguration(), TEST_CONFIG_INDEX);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ConfigFields_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_ConfigFields_002
 * @tc.desc: ParseConfigDescriptors maps bus powered attributes and a zero max power
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_ConfigFields_002, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ConfigFields_002 start");
    TestConfigDesc desc;
    desc.bmAttributes = TEST_CONFIG_ATTRIBUTES_BUS;
    desc.maxPower = TEST_CONFIG_MAX_POWER_ZERO;
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, desc);
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetAttributes(), static_cast<uint32_t>(TEST_CONFIG_ATTRIBUTES_BUS));
    EXPECT_EQ(configs.front().GetMaxPower(), 0);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ConfigFields_002 end");
}

/**
 * @tc.name: ParseConfigDescriptors_ConfigBadLength_001
 * @tc.desc: ParseConfigDescriptors rejects a config descriptor with a wrong bLength
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_ConfigBadLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ConfigBadLength_001 start");
    TestConfigDesc desc;
    desc.bLength = CONFIG_DESC_LENGTH + 1;
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, desc);
    buffer.push_back(PAD_BYTE);
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    EXPECT_TRUE(configs.empty());
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ConfigBadLength_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_ConfigTruncated_001
 * @tc.desc: ParseConfigDescriptors stops when a config descriptor exceeds the buffer
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_ConfigTruncated_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ConfigTruncated_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    buffer.pop_back();
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_TRUE(configs.empty());
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ConfigTruncated_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_ZeroBLength_001
 * @tc.desc: ParseConfigDescriptors stops on a descriptor whose bLength is below the header size
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_ZeroBLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ZeroBLength_001 start");
    std::vector<uint8_t> buffer = {0x00, DESC_TYPE_CONFIG, PAD_BYTE, PAD_BYTE};
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_TRUE(configs.empty());
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ZeroBLength_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_MultipleConfigs_001
 * @tc.desc: ParseConfigDescriptors adds each configuration descriptor in order
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_MultipleConfigs_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_MultipleConfigs_001 start");
    std::vector<uint8_t> buffer;
    TestConfigDesc first;
    first.configurationValue = TEST_CONFIG_VALUE;
    AppendConfigDescriptor(buffer, first);
    TestConfigDesc second;
    second.configurationValue = TEST_CONFIG_VALUE_SECOND;
    AppendConfigDescriptor(buffer, second);
    TestConfigDesc third;
    third.configurationValue = TEST_CONFIG_VALUE_THIRD;
    AppendConfigDescriptor(buffer, third);

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(TEST_NUM_CONFIGS_MULTI));
    EXPECT_EQ(configs[0].GetId(), static_cast<int32_t>(TEST_CONFIG_VALUE));
    EXPECT_EQ(configs[1].GetId(), static_cast<int32_t>(TEST_CONFIG_VALUE_SECOND));
    EXPECT_EQ(configs[2].GetId(), static_cast<int32_t>(TEST_CONFIG_VALUE_THIRD));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_MultipleConfigs_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_MultipleConfigs_002
 * @tc.desc: ParseConfigDescriptors appends to a non empty config vector
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_MultipleConfigs_002, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_MultipleConfigs_002 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    std::vector<USBConfig> configs;
    configs.emplace_back(USBConfig());
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    EXPECT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE) + 1);
    EXPECT_EQ(configs.back().GetId(), static_cast<int32_t>(TEST_CONFIG_VALUE));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_MultipleConfigs_002 end");
}

/**
 * @tc.name: ParseConfigDescriptors_InterfaceWithoutConfig_001
 * @tc.desc: ParseConfigDescriptors rejects an interface descriptor without a config
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_InterfaceWithoutConfig_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceWithoutConfig_001 start");
    std::vector<uint8_t> buffer;
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    EXPECT_TRUE(configs.empty());
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceWithoutConfig_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_InterfaceBadLength_001
 * @tc.desc: ParseConfigDescriptors rejects an interface descriptor with a wrong bLength
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_InterfaceBadLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceBadLength_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    TestInterfaceDesc desc;
    desc.bLength = INTERFACE_DESC_LENGTH + 1;
    AppendInterfaceDescriptor(buffer, desc);
    buffer.push_back(PAD_BYTE);
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    EXPECT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceBadLength_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_InterfaceFields_001
 * @tc.desc: ParseConfigDescriptors maps all interface descriptor fields
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_InterfaceFields_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceFields_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    ASSERT_EQ(configs.front().GetInterfaceCount(), EXPECTED_SINGLE);
    UsbInterface &interface = configs.front().GetInterfaces().front();
    EXPECT_EQ(interface.GetId(), static_cast<int32_t>(TEST_INTERFACE_NUMBER));
    EXPECT_EQ(interface.GetClass(), static_cast<int32_t>(TEST_INTERFACE_CLASS));
    EXPECT_EQ(interface.GetSubClass(), static_cast<int32_t>(TEST_INTERFACE_SUBCLASS));
    EXPECT_EQ(interface.GetProtocol(), static_cast<int32_t>(TEST_INTERFACE_PROTOCOL));
    EXPECT_EQ(interface.GetAlternateSetting(), static_cast<int32_t>(TEST_ALTERNATE_SETTING));
    EXPECT_EQ(interface.GetiInterface(), TEST_I_INTERFACE);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceFields_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_InterfaceAltSetting_001
 * @tc.desc: ParseConfigDescriptors keeps alternate settings as separate interfaces
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_InterfaceAltSetting_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceAltSetting_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    TestInterfaceDesc primary;
    AppendInterfaceDescriptor(buffer, primary);
    TestInterfaceDesc alternate;
    alternate.alternateSetting = TEST_ALTERNATE_SETTING_ALT;
    AppendInterfaceDescriptor(buffer, alternate);

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    ASSERT_EQ(configs.front().GetInterfaceCount(), EXPECTED_SINGLE + 1);
    EXPECT_EQ(configs.front().GetInterfaces()[0].GetAlternateSetting(),
        static_cast<int32_t>(TEST_ALTERNATE_SETTING));
    EXPECT_EQ(configs.front().GetInterfaces()[1].GetAlternateSetting(),
        static_cast<int32_t>(TEST_ALTERNATE_SETTING_ALT));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceAltSetting_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_InterfaceGoesToLastConfig_001
 * @tc.desc: ParseConfigDescriptors attaches interfaces to the most recent config
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_InterfaceGoesToLastConfig_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceGoesToLastConfig_001 start");
    std::vector<uint8_t> buffer;
    TestConfigDesc first;
    AppendConfigDescriptor(buffer, first);
    TestConfigDesc second;
    second.configurationValue = TEST_CONFIG_VALUE_SECOND;
    AppendConfigDescriptor(buffer, second);
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE) + 1);
    EXPECT_EQ(configs[0].GetInterfaceCount(), EXPECTED_NONE);
    EXPECT_EQ(configs[1].GetInterfaceCount(), EXPECTED_SINGLE);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceGoesToLastConfig_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointWithoutConfig_001
 * @tc.desc: ParseConfigDescriptors rejects an endpoint descriptor without a config
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointWithoutConfig_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointWithoutConfig_001 start");
    std::vector<uint8_t> buffer;
    AppendEndpointDescriptor(buffer, TestEndpointDesc());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    EXPECT_TRUE(configs.empty());
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointWithoutConfig_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointWithoutInterface_001
 * @tc.desc: ParseConfigDescriptors rejects an endpoint descriptor without an interface
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointWithoutInterface_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointWithoutInterface_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    AppendEndpointDescriptor(buffer, TestEndpointDesc());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetInterfaceCount(), EXPECTED_NONE);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointWithoutInterface_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointBadLength_001
 * @tc.desc: ParseConfigDescriptors rejects an endpoint descriptor with an unsupported bLength
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointBadLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointBadLength_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());
    TestEndpointDesc desc;
    desc.bLength = ENDPOINT_DESC_LENGTH + 1;
    AppendEndpointDescriptor(buffer, desc);
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetInterfaces().front().GetEndpointCount(), 0);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointBadLength_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointNormalLength_001
 * @tc.desc: ParseConfigDescriptors accepts a seven byte endpoint descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointNormalLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointNormalLength_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());
    AppendEndpointDescriptor(buffer, TestEndpointDesc());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetInterfaces().front().GetEndpointCount(),
        static_cast<int32_t>(EXPECTED_SINGLE));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointNormalLength_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointAudioLength_001
 * @tc.desc: ParseConfigDescriptors accepts a nine byte audio endpoint descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointAudioLength_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointAudioLength_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());
    TestEndpointDesc desc;
    desc.bLength = AUDIO_ENDPOINT_DESC_LENGTH;
    desc.bmAttributes = TEST_EP_ATTR_ISOC;
    AppendEndpointDescriptor(buffer, desc);
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    ASSERT_EQ(configs.front().GetInterfaces().front().GetEndpointCount(),
        static_cast<int32_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetInterfaces().front().GetEndpoints().front().GetType(),
        static_cast<uint32_t>(USB_ENDPOINT_XFER_ISOC));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointAudioLength_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointFields_001
 * @tc.desc: ParseConfigDescriptors maps address, attributes, interval and packet size
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointFields_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointFields_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());
    AppendEndpointDescriptor(buffer, TestEndpointDesc());

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    ASSERT_EQ(configs.front().GetInterfaces().front().GetEndpointCount(),
        static_cast<int32_t>(EXPECTED_SINGLE));
    USBEndpoint &endpoint = configs.front().GetInterfaces().front().GetEndpoints().front();
    EXPECT_EQ(endpoint.GetAddress(), static_cast<uint32_t>(TEST_EP_ADDR_IN));
    EXPECT_EQ(endpoint.GetAttributes(), static_cast<uint32_t>(TEST_EP_ATTR_BULK));
    EXPECT_EQ(endpoint.GetInterval(), static_cast<int32_t>(TEST_EP_INTERVAL));
    EXPECT_EQ(endpoint.GetMaxPacketSize(), static_cast<int32_t>(TEST_EP_MAX_PACKET_SIZE));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointFields_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointDirection_001
 * @tc.desc: ParseConfigDescriptors preserves the endpoint direction bit
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointDirection_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointDirection_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());
    TestEndpointDesc endpointIn;
    endpointIn.endpointAddress = TEST_EP_ADDR_IN;
    AppendEndpointDescriptor(buffer, endpointIn);
    TestEndpointDesc endpointOut;
    endpointOut.endpointAddress = TEST_EP_ADDR_OUT;
    AppendEndpointDescriptor(buffer, endpointOut);

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    std::vector<USBEndpoint> &endpoints = configs.front().GetInterfaces().front().GetEndpoints();
    ASSERT_EQ(endpoints.size(), static_cast<size_t>(EXPECTED_SINGLE) + 1);
    EXPECT_EQ(endpoints[0].GetDirection(), USB_ENDPOINT_DIR_IN);
    EXPECT_EQ(endpoints[1].GetDirection(), USB_ENDPOINT_DIR_OUT);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointDirection_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointTypes_001
 * @tc.desc: ParseConfigDescriptors maps every standard endpoint transfer type
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointTypes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointTypes_001 start");
    std::vector<uint8_t> attributes = {TEST_EP_ATTR_CONTROL, TEST_EP_ATTR_ISOC,
        TEST_EP_ATTR_BULK, TEST_EP_ATTR_INTERRUPT};
    std::vector<int32_t> expectedTypes = {USB_ENDPOINT_XFER_CONTROL, USB_ENDPOINT_XFER_ISOC,
        USB_ENDPOINT_XFER_BULK, USB_ENDPOINT_XFER_INT};

    for (size_t i = 0; i < attributes.size(); ++i) {
        std::vector<uint8_t> buffer;
        AppendConfigDescriptor(buffer, TestConfigDesc());
        AppendInterfaceDescriptor(buffer, TestInterfaceDesc());
        TestEndpointDesc desc;
        desc.bmAttributes = attributes[i];
        AppendEndpointDescriptor(buffer, desc);

        std::vector<USBConfig> configs;
        int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
        ASSERT_EQ(ret, UEC_OK);
        ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
        ASSERT_EQ(configs.front().GetInterfaces().front().GetEndpointCount(),
            static_cast<int32_t>(EXPECTED_SINGLE));
        EXPECT_EQ(configs.front().GetInterfaces().front().GetEndpoints().front().GetType(),
            static_cast<uint32_t>(expectedTypes[i]));
    }
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointTypes_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointInterfaceId_001
 * @tc.desc: ParseConfigDescriptors links endpoints to the owning interface id
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointInterfaceId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointInterfaceId_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    TestInterfaceDesc interface;
    interface.interfaceNumber = TEST_INTERFACE_NUMBER_THIRD;
    AppendInterfaceDescriptor(buffer, interface);
    AppendEndpointDescriptor(buffer, TestEndpointDesc());

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    ASSERT_EQ(configs.front().GetInterfaces().front().GetEndpointCount(),
        static_cast<int32_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetInterfaces().front().GetEndpoints().front().GetInterfaceId(),
        static_cast<int8_t>(TEST_INTERFACE_NUMBER_THIRD));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointInterfaceId_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointGoesToLastInterface_001
 * @tc.desc: ParseConfigDescriptors attaches endpoints to the most recent interface
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointGoesToLastInterface_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointGoesToLastInterface_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    TestInterfaceDesc first;
    first.interfaceNumber = TEST_INTERFACE_NUMBER;
    AppendInterfaceDescriptor(buffer, first);
    TestInterfaceDesc second;
    second.interfaceNumber = TEST_INTERFACE_NUMBER_SECOND;
    AppendInterfaceDescriptor(buffer, second);
    AppendEndpointDescriptor(buffer, TestEndpointDesc());

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    ASSERT_EQ(configs.front().GetInterfaceCount(), EXPECTED_SINGLE + 1);
    EXPECT_EQ(configs.front().GetInterfaces()[0].GetEndpointCount(), 0);
    EXPECT_EQ(configs.front().GetInterfaces()[1].GetEndpointCount(),
        static_cast<int32_t>(EXPECTED_SINGLE));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointGoesToLastInterface_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_UnknownDescriptor_001
 * @tc.desc: ParseConfigDescriptors skips an unrecognized descriptor type
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_UnknownDescriptor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_UnknownDescriptor_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    AppendUnknownDescriptor(buffer, INTERFACE_DESC_LENGTH, DESC_TYPE_HID);
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetInterfaceCount(), EXPECTED_SINGLE);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_UnknownDescriptor_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_UnknownDescriptor_002
 * @tc.desc: ParseConfigDescriptors skips all reserved and class specific descriptor types
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_UnknownDescriptor_002, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_UnknownDescriptor_002 start");
    std::vector<uint8_t> types = {DESC_TYPE_STRING, DESC_TYPE_DEVICE, DESC_TYPE_DEVICE_QUALIFIER,
        DESC_TYPE_OTHER_SPEED, DESC_TYPE_INTERFACE_POWER, DESC_TYPE_OTG, DESC_TYPE_DEBUG,
        DESC_TYPE_INTERFACE_ASSOC, DESC_TYPE_CS_INTERFACE, DESC_TYPE_CS_ENDPOINT, DESC_TYPE_UNKNOWN_MAX};

    for (uint8_t type : types) {
        std::vector<uint8_t> buffer;
        AppendConfigDescriptor(buffer, TestConfigDesc());
        AppendUnknownDescriptor(buffer, INTERFACE_DESC_LENGTH, type);

        std::vector<USBConfig> configs;
        int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
        EXPECT_EQ(ret, UEC_OK) << "type=" << static_cast<int32_t>(type);
        EXPECT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    }
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_UnknownDescriptor_002 end");
}

/**
 * @tc.name: ParseConfigDescriptors_UnknownBetweenEndpoints_001
 * @tc.desc: ParseConfigDescriptors keeps parsing endpoints after an unknown descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_UnknownBetweenEndpoints_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_UnknownBetweenEndpoints_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());
    TestEndpointDesc endpointIn;
    endpointIn.endpointAddress = TEST_EP_ADDR_IN;
    AppendEndpointDescriptor(buffer, endpointIn);
    AppendUnknownDescriptor(buffer, ENDPOINT_DESC_LENGTH, DESC_TYPE_CS_ENDPOINT);
    TestEndpointDesc endpointSecond;
    endpointSecond.endpointAddress = TEST_EP_ADDR_IN_SECOND;
    AppendEndpointDescriptor(buffer, endpointSecond);

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetInterfaces().front().GetEndpointCount(),
        static_cast<int32_t>(EXPECTED_SINGLE) + 1);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_UnknownBetweenEndpoints_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_ManyEndpoints_001
 * @tc.desc: ParseConfigDescriptors parses an interface carrying many endpoints
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_ManyEndpoints_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ManyEndpoints_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());
    for (size_t i = 0; i < MANY_ENDPOINT_COUNT; ++i) {
        TestEndpointDesc desc;
        desc.endpointAddress = static_cast<uint8_t>(i + 1);
        desc.wMaxPacketSize = TEST_EP_MAX_PACKET_SIZE_SMALL;
        AppendEndpointDescriptor(buffer, desc);
    }

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetInterfaces().front().GetEndpoints().size(), MANY_ENDPOINT_COUNT);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ManyEndpoints_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_ManyInterfaces_001
 * @tc.desc: ParseConfigDescriptors parses a config carrying many interfaces
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_ManyInterfaces_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ManyInterfaces_001 start");
    std::vector<uint8_t> buffer;
    TestConfigDesc config;
    config.numInterfaces = static_cast<uint8_t>(MANY_INTERFACE_COUNT);
    AppendConfigDescriptor(buffer, config);
    for (size_t i = 0; i < MANY_INTERFACE_COUNT; ++i) {
        TestInterfaceDesc interface;
        interface.interfaceNumber = static_cast<uint8_t>(i);
        interface.numEndpoints = 1;
        AppendInterfaceDescriptor(buffer, interface);
        TestEndpointDesc endpoint;
        endpoint.endpointAddress = static_cast<uint8_t>(i + 1);
        AppendEndpointDescriptor(buffer, endpoint);
    }

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    ASSERT_EQ(configs.front().GetInterfaces().size(), MANY_INTERFACE_COUNT);
    for (size_t i = 0; i < MANY_INTERFACE_COUNT; ++i) {
        EXPECT_EQ(configs.front().GetInterfaces()[i].GetId(), static_cast<int32_t>(i));
        EXPECT_EQ(configs.front().GetInterfaces()[i].GetEndpointCount(),
            static_cast<int32_t>(EXPECTED_SINGLE));
    }
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ManyInterfaces_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_ManyConfigs_001
 * @tc.desc: ParseConfigDescriptors parses a full multi configuration topology
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_ManyConfigs_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ManyConfigs_001 start");
    std::vector<uint8_t> buffer;
    for (size_t i = 0; i < MANY_CONFIG_COUNT; ++i) {
        TestConfigDesc config;
        config.configurationValue = static_cast<uint8_t>(i + 1);
        AppendConfigDescriptor(buffer, config);
        TestInterfaceDesc interface;
        interface.interfaceNumber = static_cast<uint8_t>(i);
        interface.numEndpoints = 1;
        AppendInterfaceDescriptor(buffer, interface);
        TestEndpointDesc endpoint;
        endpoint.endpointAddress = static_cast<uint8_t>(i + 1);
        AppendEndpointDescriptor(buffer, endpoint);
    }

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), MANY_CONFIG_COUNT);
    for (size_t i = 0; i < MANY_CONFIG_COUNT; ++i) {
        EXPECT_EQ(configs[i].GetId(), static_cast<int32_t>(i + 1));
        ASSERT_EQ(configs[i].GetInterfaceCount(), EXPECTED_SINGLE);
        EXPECT_EQ(configs[i].GetInterfaces().front().GetEndpointCount(),
            static_cast<int32_t>(EXPECTED_SINGLE));
    }
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ManyConfigs_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_TrailingGarbage_001
 * @tc.desc: ParseConfigDescriptors keeps parsed data when trailing bytes are incomplete
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_TrailingGarbage_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_TrailingGarbage_001 start");
    std::vector<uint8_t> buffer = BuildSimpleTopology();
    buffer.push_back(DESC_TYPE_UNKNOWN_MAX);
    buffer.push_back(DESC_TYPE_ENDPOINT);

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetInterfaces().front().GetEndpointCount(),
        static_cast<int32_t>(EXPECTED_SINGLE) + 1);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_TrailingGarbage_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_LargePaddedBuffer_001
 * @tc.desc: ParseConfigDescriptors parses a topology followed by a large zero padding
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_LargePaddedBuffer_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_LargePaddedBuffer_001 start");
    std::vector<uint8_t> buffer = BuildSimpleTopology();
    buffer.resize(LARGE_BUFFER_SIZE, PAD_BYTE);

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    EXPECT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    EXPECT_EQ(configs.front().GetInterfaces().front().GetEndpointCount(),
        static_cast<int32_t>(EXPECTED_SINGLE) + 1);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_LargePaddedBuffer_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_OffsetInsideDescriptor_001
 * @tc.desc: ParseConfigDescriptors treats a misaligned offset as unrecognized data
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_OffsetInsideDescriptor_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_OffsetInsideDescriptor_001 start");
    std::vector<uint8_t> buffer = BuildSimpleTopology();
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, CONFIG_DESC_LENGTH, configs);
    EXPECT_EQ(ret, UEC_SERVICE_INVALID_VALUE);
    EXPECT_TRUE(configs.empty());
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_OffsetInsideDescriptor_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_Repeated_001
 * @tc.desc: ParseConfigDescriptors is stable when invoked repeatedly on the same buffer
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_Repeated_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_Repeated_001 start");
    std::vector<uint8_t> buffer = BuildSimpleTopology();
    for (size_t i = 0; i < STRESS_LOOP_COUNT; ++i) {
        std::vector<USBConfig> configs;
        int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
        ASSERT_EQ(ret, UEC_OK);
        ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
        EXPECT_EQ(configs.front().GetInterfaces().front().GetEndpointCount(),
            static_cast<int32_t>(EXPECTED_SINGLE) + 1);
    }
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_Repeated_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_FullTopologyThenDevice_001
 * @tc.desc: A device descriptor and its config topology can be parsed together
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_FullTopologyThenDevice_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_FullTopologyThenDevice_001 start");
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(TestDeviceDesc());
    std::vector<uint8_t> topology = BuildSimpleTopology();
    buffer.insert(buffer.end(), topology.begin(), topology.end());

    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);

    std::vector<USBConfig> configs;
    ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, DEVICE_DESC_LENGTH, configs);
    ASSERT_EQ(ret, UEC_OK);
    device.SetConfigs(configs);
    EXPECT_EQ(device.GetConfigCount(), static_cast<int32_t>(EXPECTED_SINGLE));
    EXPECT_EQ(device.GetDescConfigCount(), TEST_NUM_CONFIGS);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_FullTopologyThenDevice_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_ConfigAccessByIndex_001
 * @tc.desc: A parsed config can be retrieved from the device by index
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_ConfigAccessByIndex_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ConfigAccessByIndex_001 start");
    std::vector<uint8_t> buffer = BuildSimpleTopology();
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);

    UsbDevice device;
    device.SetConfigs(configs);
    USBConfig config;
    ret = device.GetConfig(0, config);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_EQ(config.GetId(), static_cast<int32_t>(TEST_CONFIG_VALUE));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_ConfigAccessByIndex_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_InterfaceAccessByIndex_001
 * @tc.desc: A parsed interface can be retrieved from the config by index
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_InterfaceAccessByIndex_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceAccessByIndex_001 start");
    std::vector<uint8_t> buffer = BuildSimpleTopology();
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));

    UsbInterface interface;
    bool found = configs.front().GetInterface(0, interface);
    EXPECT_TRUE(found);
    EXPECT_EQ(interface.GetId(), static_cast<int32_t>(TEST_INTERFACE_NUMBER));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_InterfaceAccessByIndex_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointAccessByIndex_001
 * @tc.desc: A parsed endpoint can be retrieved from the interface by index
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointAccessByIndex_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointAccessByIndex_001 start");
    std::vector<uint8_t> buffer = BuildSimpleTopology();
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));

    UsbInterface &interface = configs.front().GetInterfaces().front();
    auto endpoint = interface.GetEndpoint(0);
    ASSERT_TRUE(endpoint.has_value());
    EXPECT_EQ(endpoint->GetAddress(), static_cast<uint32_t>(TEST_EP_ADDR_IN));
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointAccessByIndex_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointNumberMask_001
 * @tc.desc: A parsed endpoint exposes the masked endpoint number
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointNumberMask_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointNumberMask_001 start");
    std::vector<uint8_t> buffer;
    AppendConfigDescriptor(buffer, TestConfigDesc());
    AppendInterfaceDescriptor(buffer, TestInterfaceDesc());
    TestEndpointDesc desc;
    desc.endpointAddress = TEST_EP_ADDR_IN_SECOND;
    AppendEndpointDescriptor(buffer, desc);

    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));
    USBEndpoint &endpoint = configs.front().GetInterfaces().front().GetEndpoints().front();
    EXPECT_EQ(endpoint.GetNumber(), static_cast<uint8_t>(TEST_EP_ADDR_IN_SECOND & USB_ENDPOINT_NUMBER_MASK));
    EXPECT_EQ(endpoint.GetEndpointNumber(), TEST_EP_ADDR_IN_SECOND & USB_ENDPOINT_NUMBER_MASK);
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointNumberMask_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_EndpointToString_001
 * @tc.desc: A parsed endpoint can be serialized to a descriptive string
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_EndpointToString_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointToString_001 start");
    std::vector<uint8_t> buffer = BuildSimpleTopology();
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(buffer, ZERO_OFFSET, configs);
    ASSERT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(EXPECTED_SINGLE));

    std::string text = configs.front().GetInterfaces().front().GetEndpoints().front().ToString();
    EXPECT_FALSE(text.empty());
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_EndpointToString_001 end");
}

/**
 * @tc.name: ParseConfigDescriptors_DeviceToString_001
 * @tc.desc: A device filled from parsed descriptors can be serialized to a string
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserExtendedTest, ParseConfigDescriptors_DeviceToString_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_DeviceToString_001 start");
    std::vector<uint8_t> buffer = BuildDeviceDescriptor(TestDeviceDesc());
    UsbDevice device;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buffer.data(),
        static_cast<uint32_t>(buffer.size()), device);
    ASSERT_EQ(ret, UEC_OK);
    EXPECT_FALSE(device.ToString().empty());
    USB_HILOGI(MODULE_USB_HOST, "ParseConfigDescriptors_DeviceToString_001 end");
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS
