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
#include <cstddef>
#include <vector>
#include <cstdint>

#include "usb_descriptor_parser_tdd_test.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

namespace {

constexpr uint32_t DEVICE_DESCRIPTOR_SIZE = sizeof(UsbdDeviceDescriptor);
constexpr uint32_t CONFIG_DESCRIPTOR_SIZE = sizeof(UsbdConfigDescriptor);
constexpr uint32_t INTERFACE_DESCRIPTOR_SIZE = sizeof(UsbdInterfaceDescriptor);
constexpr uint32_t ENDPOINT_DESCRIPTOR_SIZE = sizeof(UsbdEndpointDescriptor);
constexpr uint32_t CURSOR_INIT = DEVICE_DESCRIPTOR_SIZE;

constexpr uint8_t DESCRIPTOR_TYPE_DEVICE = 1;
constexpr uint8_t DESCRIPTOR_TYPE_CONFIG = 2;
constexpr uint8_t DESCRIPTOR_TYPE_INTERFACE = 4;
constexpr uint8_t DESCRIPTOR_TYPE_ENDPOINT = 5;

constexpr uint16_t BCD_USB_2_1 = 0x0210;
constexpr uint8_t DEVICE_CLASS_PER_INTERFACE = 0x00;
constexpr uint8_t DEVICE_SUBCLASS_NONE = 0x00;
constexpr uint8_t DEVICE_PROTOCOL_NONE = 0x00;
constexpr uint8_t EP0_MAX_PACKET_SIZE_64 = 64;
constexpr uint16_t TEST_VENDOR_ID = 0x18D1;
constexpr uint16_t TEST_PRODUCT_ID = 0x4E11;
constexpr uint16_t BCD_DEVICE_1_0 = 0x0100;
constexpr uint8_t I_MANUFACTURER_INDEX = 1;
constexpr uint8_t I_PRODUCT_INDEX = 2;
constexpr uint8_t I_SERIAL_NUMBER_INDEX = 3;
constexpr uint8_t NUM_CONFIGURATIONS_ONE = 1;

constexpr uint16_t W_TOTAL_LENGTH_18 = 0x0012;
constexpr uint8_t NUM_INTERFACES_ONE = 1;
constexpr uint8_t CONFIGURATION_VALUE_ONE = 1;
constexpr uint8_t I_CONFIGURATION_NONE = 0;
constexpr uint8_t BM_ATTRIBUTES_SELF_POWERED = 0x80;
constexpr uint8_t MAX_POWER_100MA = 50;

constexpr uint8_t INTERFACE_NUMBER_ZERO = 0;
constexpr uint8_t ALTERNATE_SETTING_ZERO = 0;
constexpr uint8_t NUM_ENDPOINTS_ONE = 1;
constexpr uint8_t INTERFACE_CLASS_MASS_STORAGE = 0x08;
constexpr uint8_t INTERFACE_SUBCLASS_SCSI = 0x06;
constexpr uint8_t INTERFACE_PROTOCOL_BULK_ONLY = 0x50;
constexpr uint8_t I_INTERFACE_NONE = 0;

constexpr uint8_t ENDPOINT_ADDR_IN_EP1 = 0x81;
constexpr uint8_t ENDPOINT_ATTR_BULK = 0x02;
constexpr uint16_t W_MAX_PACKET_SIZE_512 = 0x0200;
constexpr uint8_t ENDPOINT_INTERVAL_ZERO = 0;

constexpr uint16_t CUSTOM_VENDOR_ID = 0x1234;
constexpr uint16_t CUSTOM_PRODUCT_ID = 0x5678;
constexpr uint8_t DEVICE_CLASS_HUB = 0x09;
constexpr uint8_t NUM_CONFIGURATIONS_THREE = 3;
constexpr uint8_t NUM_CONFIGURATIONS_MAX = 0xFF;
constexpr uint8_t NUM_CONFIGURATIONS_ZERO = 0;

constexpr uint8_t INVALID_BLENGTH_EIGHT = 8;
constexpr uint8_t INVALID_BLENGTH_SIX = 6;
constexpr uint8_t AUDIO_ENDPOINT_BLENGTH_NINE = 9;
constexpr uint8_t INVALID_BLENGTH_ONE = 1;
constexpr uint8_t UNKNOWN_DESC_TYPE = 3;
constexpr uint8_t UNKNOWN_DESC_LENGTH = 3;
constexpr uint8_t CONFIGURATION_VALUE_TWO = 2;
constexpr uint8_t INTERFACE_NUMBER_ONE = 1;
constexpr uint8_t ENDPOINT_ADDR_OUT_EP2 = 0x02;

constexpr uint32_t DESCRIPTOR_BUFFER_SIZE_100 = 100;
constexpr uint32_t OFFSET_OUT_OF_RANGE_200 = 200;
constexpr uint32_t LARGER_BUFFER_SIZE_100 = 100;
constexpr uint32_t EXTRA_BYTE_COUNT = 1;
constexpr uint32_t AUDIO_ENDPOINT_EXTRA_BYTES = 2;

std::vector<uint8_t> MakeValidDeviceDescriptor()
{
    UsbdDeviceDescriptor desc = {};
    desc.bLength = DEVICE_DESCRIPTOR_SIZE;
    desc.bDescriptorType = DESCRIPTOR_TYPE_DEVICE;
    desc.bcdUSB = BCD_USB_2_1;
    desc.bDeviceClass = DEVICE_CLASS_PER_INTERFACE;
    desc.bDeviceSubClass = DEVICE_SUBCLASS_NONE;
    desc.bDeviceProtocol = DEVICE_PROTOCOL_NONE;
    desc.bMaxPacketSize0 = EP0_MAX_PACKET_SIZE_64;
    desc.idVendor = TEST_VENDOR_ID;
    desc.idProduct = TEST_PRODUCT_ID;
    desc.bcdDevice = BCD_DEVICE_1_0;
    desc.iManufacturer = I_MANUFACTURER_INDEX;
    desc.iProduct = I_PRODUCT_INDEX;
    desc.iSerialNumber = I_SERIAL_NUMBER_INDEX;
    desc.bNumConfigurations = NUM_CONFIGURATIONS_ONE;
    const uint8_t *begin = reinterpret_cast<const uint8_t *>(&desc);
    return std::vector<uint8_t>(begin, begin + sizeof(desc));
}

std::vector<uint8_t> MakeValidConfigDescriptor()
{
    UsbdConfigDescriptor desc = {};
    desc.bLength = CONFIG_DESCRIPTOR_SIZE;
    desc.bDescriptorType = DESCRIPTOR_TYPE_CONFIG;
    desc.wTotalLength = W_TOTAL_LENGTH_18;
    desc.bNumInterfaces = NUM_INTERFACES_ONE;
    desc.bConfigurationValue = CONFIGURATION_VALUE_ONE;
    desc.iConfiguration = I_CONFIGURATION_NONE;
    desc.bmAttributes = BM_ATTRIBUTES_SELF_POWERED;
    desc.bMaxPower = MAX_POWER_100MA;
    const uint8_t *begin = reinterpret_cast<const uint8_t *>(&desc);
    return std::vector<uint8_t>(begin, begin + sizeof(desc));
}

std::vector<uint8_t> MakeValidInterfaceDescriptor()
{
    UsbdInterfaceDescriptor desc = {};
    desc.bLength = INTERFACE_DESCRIPTOR_SIZE;
    desc.bDescriptorType = DESCRIPTOR_TYPE_INTERFACE;
    desc.bInterfaceNumber = INTERFACE_NUMBER_ZERO;
    desc.bAlternateSetting = ALTERNATE_SETTING_ZERO;
    desc.bNumEndpoints = NUM_ENDPOINTS_ONE;
    desc.bInterfaceClass = INTERFACE_CLASS_MASS_STORAGE;
    desc.bInterfaceSubClass = INTERFACE_SUBCLASS_SCSI;
    desc.bInterfaceProtocol = INTERFACE_PROTOCOL_BULK_ONLY;
    desc.iInterface = I_INTERFACE_NONE;
    const uint8_t *begin = reinterpret_cast<const uint8_t *>(&desc);
    return std::vector<uint8_t>(begin, begin + sizeof(desc));
}

std::vector<uint8_t> MakeValidEndpointDescriptor()
{
    UsbdEndpointDescriptor desc = {};
    desc.bLength = ENDPOINT_DESCRIPTOR_SIZE;
    desc.bDescriptorType = DESCRIPTOR_TYPE_ENDPOINT;
    desc.bEndpointAddress = ENDPOINT_ADDR_IN_EP1;
    desc.bmAttributes = ENDPOINT_ATTR_BULK;
    desc.wMaxPacketSize = W_MAX_PACKET_SIZE_512;
    desc.bInterval = ENDPOINT_INTERVAL_ZERO;
    const uint8_t *begin = reinterpret_cast<const uint8_t *>(&desc);
    return std::vector<uint8_t>(begin, begin + sizeof(desc));
}

std::vector<uint8_t> MakeFullDescriptorBlob()
{
    std::vector<uint8_t> blob;
    auto dev = MakeValidDeviceDescriptor();
    auto cfg = MakeValidConfigDescriptor();
    auto intf = MakeValidInterfaceDescriptor();
    auto ep = MakeValidEndpointDescriptor();
    blob.insert(blob.end(), dev.begin(), dev.end());
    blob.insert(blob.end(), cfg.begin(), cfg.end());
    blob.insert(blob.end(), intf.begin(), intf.end());
    blob.insert(blob.end(), ep.begin(), ep.end());
    return blob;
}

} // namespace

class UsbDescriptorParserTddTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserTddTest::SetUpTestCase enter");
    }
    static void TearDownTestCase(void)
    {
        USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserTddTest::TearDownTestCase enter");
    }
    void SetUp(void)
    {
        USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserTddTest::SetUp enter");
    }
    void TearDown(void)
    {
        USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserTddTest::TearDown enter");
    }
};

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_NullBuffer_ReturnsInvalidValue, TestSize.Level1)
{
    UsbDevice dev;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(nullptr, DEVICE_DESCRIPTOR_SIZE, dev);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ZeroLength_ReturnsInvalidValue, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), 0, dev);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_NullAndZero_ReturnsInvalidValue, TestSize.Level1)
{
    UsbDevice dev;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(nullptr, 0, dev);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ShortBuffer_ReturnsInvalidValue, TestSize.Level1)
{
    std::vector<uint8_t> buf(DEVICE_DESCRIPTOR_SIZE - 1, 0);
    UsbDevice dev;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_BufferSize17_ReturnsInvalidValue, TestSize.Level1)
{
    std::vector<uint8_t> buf(DEVICE_DESCRIPTOR_SIZE - 1, 0);
    UsbDevice dev;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_WrongBLength_ReturnsInvalidValue, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    auto *desc = reinterpret_cast<UsbdDeviceDescriptor *>(buf.data());
    desc->bLength = DEVICE_DESCRIPTOR_SIZE - 1;
    UsbDevice dev;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_BLengthZero_ReturnsInvalidValue, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    auto *desc = reinterpret_cast<UsbdDeviceDescriptor *>(buf.data());
    desc->bLength = 0;
    UsbDevice dev;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_BLengthTooLarge_ReturnsInvalidValue, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    auto *desc = reinterpret_cast<UsbdDeviceDescriptor *>(buf.data());
    desc->bLength = DEVICE_DESCRIPTOR_SIZE + 1;
    UsbDevice dev;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_ReturnsOk, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsVendorId, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetVendorId(), TEST_VENDOR_ID);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsProductId, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetProductId(), TEST_PRODUCT_ID);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsClass, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetClass(), DEVICE_CLASS_PER_INTERFACE);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsSubclass, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetSubclass(), DEVICE_SUBCLASS_NONE);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsProtocol, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetProtocol(), DEVICE_PROTOCOL_NONE);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsDescConfigCount, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetDescConfigCount(), NUM_CONFIGURATIONS_ONE);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsbMaxPacketSize0, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetbMaxPacketSize0(), EP0_MAX_PACKET_SIZE_64);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsbcdUSB, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetbcdUSB(), BCD_USB_2_1);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsbcdDevice, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetbcdDevice(), BCD_DEVICE_1_0);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsiManufacturer, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetiManufacturer(), I_MANUFACTURER_INDEX);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsiProduct, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetiProduct(), I_PRODUCT_INDEX);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ValidBuffer_SetsiSerialNumber, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetiSerialNumber(), I_SERIAL_NUMBER_INDEX);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_CustomVendorId_SetsCorrectly, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    auto *desc = reinterpret_cast<UsbdDeviceDescriptor *>(buf.data());
    desc->idVendor = CUSTOM_VENDOR_ID;
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetVendorId(), CUSTOM_VENDOR_ID);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_CustomProductId_SetsCorrectly, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    auto *desc = reinterpret_cast<UsbdDeviceDescriptor *>(buf.data());
    desc->idProduct = CUSTOM_PRODUCT_ID;
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetProductId(), CUSTOM_PRODUCT_ID);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ClassHub_SetsCorrectly, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    auto *desc = reinterpret_cast<UsbdDeviceDescriptor *>(buf.data());
    desc->bDeviceClass = DEVICE_CLASS_HUB;
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetClass(), DEVICE_CLASS_HUB);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_MultipleConfigs_SetsConfigCount, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    auto *desc = reinterpret_cast<UsbdDeviceDescriptor *>(buf.data());
    desc->bNumConfigurations = NUM_CONFIGURATIONS_THREE;
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetDescConfigCount(), NUM_CONFIGURATIONS_THREE);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_MaxConfigs_SetsConfigCount, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    auto *desc = reinterpret_cast<UsbdDeviceDescriptor *>(buf.data());
    desc->bNumConfigurations = NUM_CONFIGURATIONS_MAX;
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetDescConfigCount(), NUM_CONFIGURATIONS_MAX);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ZeroConfigs_SetsConfigCount, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    auto *desc = reinterpret_cast<UsbdDeviceDescriptor *>(buf.data());
    desc->bNumConfigurations = NUM_CONFIGURATIONS_ZERO;
    UsbDevice dev;
    UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(dev.GetDescConfigCount(), NUM_CONFIGURATIONS_ZERO);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_ExactSize_ReturnsOk, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    UsbDevice dev;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), DEVICE_DESCRIPTOR_SIZE, dev);
    EXPECT_EQ(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseDeviceDescriptor_LargerBuffer_ReturnsOk, TestSize.Level1)
{
    auto buf = MakeValidDeviceDescriptor();
    buf.resize(LARGER_BUFFER_SIZE_100);
    UsbDevice dev;
    int32_t ret = UsbDescriptorParser::ParseDeviceDescriptor(buf.data(), buf.size(), dev);
    EXPECT_EQ(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_EmptyVector_ReturnsOk, TestSize.Level1)
{
    std::vector<uint8_t> descriptor;
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(descriptor, CURSOR_INIT, configs);
    EXPECT_EQ(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_OffsetOutOfRange_ReturnsOk, TestSize.Level1)
{
    std::vector<uint8_t> descriptor(DESCRIPTOR_BUFFER_SIZE_100, 0);
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(descriptor, OFFSET_OUT_OF_RANGE_200, configs);
    EXPECT_EQ(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_OffsetEqualsLength_ReturnsOk, TestSize.Level1)
{
    std::vector<uint8_t> descriptor(DESCRIPTOR_BUFFER_SIZE_100, 0);
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(descriptor, DESCRIPTOR_BUFFER_SIZE_100, configs);
    EXPECT_EQ(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_RemainingBytesLessThanHeader_NoError, TestSize.Level1)
{
    std::vector<uint8_t> descriptor = MakeFullDescriptorBlob();
    descriptor.resize(descriptor.size() + EXTRA_BYTE_COUNT);
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(descriptor, CURSOR_INIT, configs);
    EXPECT_EQ(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidConfig_AddsConfig, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_EQ(configs.size(), static_cast<size_t>(NUM_INTERFACES_ONE));
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidConfig_SetsId, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    ASSERT_EQ(configs.size(), static_cast<size_t>(NUM_INTERFACES_ONE));
    EXPECT_EQ(configs[0].GetId(), CONFIGURATION_VALUE_ONE);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidConfig_SetsAttribute, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    ASSERT_EQ(configs.size(), static_cast<size_t>(NUM_INTERFACES_ONE));
    EXPECT_EQ(configs[0].GetAttributes(), static_cast<uint32_t>(BM_ATTRIBUTES_SELF_POWERED));
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidConfig_SetsMaxPower, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    ASSERT_EQ(configs.size(), static_cast<size_t>(NUM_INTERFACES_ONE));
    EXPECT_EQ(configs[0].GetMaxPower(), MAX_POWER_100MA * 2);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidConfig_SetsiConfiguration, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    ASSERT_EQ(configs.size(), static_cast<size_t>(NUM_INTERFACES_ONE));
    EXPECT_EQ(configs[0].GetiConfiguration(), I_CONFIGURATION_NONE);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidInterface_AddsInterface, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    ASSERT_EQ(configs.size(), static_cast<size_t>(NUM_INTERFACES_ONE));
    EXPECT_EQ(configs[0].GetInterfaceCount(), static_cast<size_t>(NUM_INTERFACES_ONE));
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidInterface_SetsId, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    ASSERT_EQ(configs.size(), static_cast<size_t>(NUM_INTERFACES_ONE));
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    EXPECT_EQ(intf.GetId(), INTERFACE_NUMBER_ZERO);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidInterface_SetsClass, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    EXPECT_EQ(intf.GetClass(), INTERFACE_CLASS_MASS_STORAGE);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidInterface_SetsSubClass, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    EXPECT_EQ(intf.GetSubClass(), INTERFACE_SUBCLASS_SCSI);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidInterface_SetsProtocol, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    EXPECT_EQ(intf.GetProtocol(), INTERFACE_PROTOCOL_BULK_ONLY);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidInterface_SetsAlternateSetting, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    EXPECT_EQ(intf.GetAlternateSetting(), ALTERNATE_SETTING_ZERO);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidEndpoint_AddsEndpoint, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    EXPECT_EQ(intf.GetEndpointCount(), NUM_ENDPOINTS_ONE);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidEndpoint_SetsAddress, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    auto ep = intf.GetEndpoint(0);
    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ(ep->GetAddress(), static_cast<uint32_t>(ENDPOINT_ADDR_IN_EP1));
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidEndpoint_SetsAttributes, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    auto ep = intf.GetEndpoint(0);
    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ(ep->GetAttributes(), static_cast<uint32_t>(ENDPOINT_ATTR_BULK));
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidEndpoint_SetsInterval, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    auto ep = intf.GetEndpoint(0);
    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ(ep->GetInterval(), ENDPOINT_INTERVAL_ZERO);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidEndpoint_SetsMaxPacketSize, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    auto ep = intf.GetEndpoint(0);
    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ(ep->GetMaxPacketSize(), W_MAX_PACKET_SIZE_512);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ValidEndpoint_SetsInterfaceId, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    auto ep = intf.GetEndpoint(0);
    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ(ep->GetInterfaceId(), INTERFACE_NUMBER_ZERO);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_InterfaceBeforeConfig_ReturnsError, TestSize.Level1)
{
    std::vector<uint8_t> blob;
    auto intf = MakeValidInterfaceDescriptor();
    blob.insert(blob.end(), intf.begin(), intf.end());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, 0, configs);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_EndpointBeforeConfig_ReturnsError, TestSize.Level1)
{
    std::vector<uint8_t> blob;
    auto ep = MakeValidEndpointDescriptor();
    blob.insert(blob.end(), ep.begin(), ep.end());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, 0, configs);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_EndpointBeforeInterface_ReturnsError, TestSize.Level1)
{
    std::vector<uint8_t> blob;
    auto cfg = MakeValidConfigDescriptor();
    auto ep = MakeValidEndpointDescriptor();
    blob.insert(blob.end(), cfg.begin(), cfg.end());
    blob.insert(blob.end(), ep.begin(), ep.end());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, 0, configs);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_InvalidConfigBLength_ReturnsError, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    auto *cfgDesc = reinterpret_cast<UsbdConfigDescriptor *>(blob.data() + CURSOR_INIT);
    cfgDesc->bLength = INVALID_BLENGTH_EIGHT;
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_InvalidInterfaceBLength_ReturnsError, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    auto *intfDesc = reinterpret_cast<UsbdInterfaceDescriptor *>(
        blob.data() + CURSOR_INIT + CONFIG_DESCRIPTOR_SIZE);
    intfDesc->bLength = INVALID_BLENGTH_EIGHT;
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_InvalidEndpointBLength_ReturnsError, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    auto *epDesc = reinterpret_cast<UsbdEndpointDescriptor *>(
        blob.data() + CURSOR_INIT + CONFIG_DESCRIPTOR_SIZE + INTERFACE_DESCRIPTOR_SIZE);
    epDesc->bLength = INVALID_BLENGTH_SIX;
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_AudioEndpointBLength9_ReturnsOk, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    auto *epDesc = reinterpret_cast<UsbdEndpointDescriptor *>(
        blob.data() + CURSOR_INIT + CONFIG_DESCRIPTOR_SIZE + INTERFACE_DESCRIPTOR_SIZE);
    epDesc->bLength = AUDIO_ENDPOINT_BLENGTH_NINE;
    blob.resize(blob.size() + AUDIO_ENDPOINT_EXTRA_BYTES);
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    EXPECT_EQ(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_UnknownDescriptorType_SkipsAndContinues, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<uint8_t> unknownDesc = {UNKNOWN_DESC_LENGTH, UNKNOWN_DESC_TYPE};
    blob.insert(blob.begin() + CURSOR_INIT + CONFIG_DESCRIPTOR_SIZE,
        unknownDesc.begin(), unknownDesc.end());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    EXPECT_EQ(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_MultipleConfigs_ReturnsAll, TestSize.Level1)
{
    std::vector<uint8_t> blob;
    auto dev = MakeValidDeviceDescriptor();
    auto cfg1 = MakeValidConfigDescriptor();
    auto cfg2 = MakeValidConfigDescriptor();
    auto *cfg2Desc = reinterpret_cast<UsbdConfigDescriptor *>(cfg2.data());
    cfg2Desc->bConfigurationValue = CONFIGURATION_VALUE_TWO;
    blob.insert(blob.end(), dev.begin(), dev.end());
    blob.insert(blob.end(), cfg1.begin(), cfg1.end());
    blob.insert(blob.end(), cfg2.begin(), cfg2.end());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_EQ(configs.size(), static_cast<size_t>(CONFIGURATION_VALUE_TWO));
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_MultipleInterfaces_ReturnsAll, TestSize.Level1)
{
    std::vector<uint8_t> blob;
    auto dev = MakeValidDeviceDescriptor();
    auto cfg = MakeValidConfigDescriptor();
    auto intf1 = MakeValidInterfaceDescriptor();
    auto intf2 = MakeValidInterfaceDescriptor();
    auto *intf2Desc = reinterpret_cast<UsbdInterfaceDescriptor *>(intf2.data());
    intf2Desc->bInterfaceNumber = INTERFACE_NUMBER_ONE;
    blob.insert(blob.end(), dev.begin(), dev.end());
    blob.insert(blob.end(), cfg.begin(), cfg.end());
    blob.insert(blob.end(), intf1.begin(), intf1.end());
    blob.insert(blob.end(), intf2.begin(), intf2.end());
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    ASSERT_EQ(configs.size(), static_cast<size_t>(NUM_INTERFACES_ONE));
    EXPECT_EQ(configs[0].GetInterfaceCount(), static_cast<size_t>(CONFIGURATION_VALUE_TWO));
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_MultipleEndpoints_ReturnsAll, TestSize.Level1)
{
    std::vector<uint8_t> blob;
    auto dev = MakeValidDeviceDescriptor();
    auto cfg = MakeValidConfigDescriptor();
    auto intf = MakeValidInterfaceDescriptor();
    auto ep1 = MakeValidEndpointDescriptor();
    auto ep2 = MakeValidEndpointDescriptor();
    auto *ep2Desc = reinterpret_cast<UsbdEndpointDescriptor *>(ep2.data());
    ep2Desc->bEndpointAddress = ENDPOINT_ADDR_OUT_EP2;
    blob.insert(blob.end(), dev.begin(), dev.end());
    blob.insert(blob.end(), cfg.begin(), cfg.end());
    blob.insert(blob.end(), intf.begin(), intf.end());
    blob.insert(blob.end(), ep1.begin(), ep1.end());
    blob.insert(blob.end(), ep2.begin(), ep2.end());
    std::vector<USBConfig> configs;
    UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    ASSERT_EQ(configs.size(), static_cast<size_t>(NUM_INTERFACES_ONE));
    UsbInterface result;
    configs[0].GetInterface(0, result);
    EXPECT_EQ(result.GetEndpointCount(), CONFIGURATION_VALUE_TWO);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_FullBlob_AllFieldsCorrect, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    EXPECT_EQ(ret, UEC_OK);
    ASSERT_EQ(configs.size(), static_cast<size_t>(NUM_INTERFACES_ONE));
    EXPECT_EQ(configs[0].GetId(), CONFIGURATION_VALUE_ONE);
    EXPECT_EQ(configs[0].GetAttributes(), static_cast<uint32_t>(BM_ATTRIBUTES_SELF_POWERED));
    EXPECT_EQ(configs[0].GetMaxPower(), MAX_POWER_100MA * 2);
    EXPECT_EQ(configs[0].GetInterfaceCount(), static_cast<size_t>(NUM_INTERFACES_ONE));
    UsbInterface intf;
    configs[0].GetInterface(0, intf);
    EXPECT_EQ(intf.GetId(), INTERFACE_NUMBER_ZERO);
    EXPECT_EQ(intf.GetClass(), INTERFACE_CLASS_MASS_STORAGE);
    EXPECT_EQ(intf.GetSubClass(), INTERFACE_SUBCLASS_SCSI);
    EXPECT_EQ(intf.GetProtocol(), INTERFACE_PROTOCOL_BULK_ONLY);
    EXPECT_EQ(intf.GetEndpointCount(), NUM_ENDPOINTS_ONE);
    auto ep = intf.GetEndpoint(0);
    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ(ep->GetAddress(), static_cast<uint32_t>(ENDPOINT_ADDR_IN_EP1));
    EXPECT_EQ(ep->GetAttributes(), static_cast<uint32_t>(ENDPOINT_ATTR_BULK));
    EXPECT_EQ(ep->GetMaxPacketSize(), W_MAX_PACKET_SIZE_512);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_BLengthTooSmall_Breaks, TestSize.Level1)
{
    std::vector<uint8_t> blob = MakeFullDescriptorBlob();
    auto *cfgDesc = reinterpret_cast<UsbdConfigDescriptor *>(blob.data() + CURSOR_INIT);
    cfgDesc->bLength = INVALID_BLENGTH_ONE;
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_EQ(configs.size(), static_cast<size_t>(NUM_CONFIGURATIONS_ZERO));
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_BLengthZero_Breaks, TestSize.Level1)
{
    std::vector<uint8_t> blob = MakeFullDescriptorBlob();
    auto *cfgDesc = reinterpret_cast<UsbdConfigDescriptor *>(blob.data() + CURSOR_INIT);
    cfgDesc->bLength = NUM_CONFIGURATIONS_ZERO;
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_EQ(configs.size(), static_cast<size_t>(NUM_CONFIGURATIONS_ZERO));
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ZeroOffsetWithFullBlob_ReturnsError, TestSize.Level1)
{
    auto blob = MakeFullDescriptorBlob();
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, 0, configs);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbDescriptorParserTddTest, ParseConfigDescriptors_ConfigOnlyNoInterface_ReturnsOk, TestSize.Level1)
{
    std::vector<uint8_t> blob;
    auto dev = MakeValidDeviceDescriptor();
    auto cfg = MakeValidConfigDescriptor();
    blob.insert(blob.end(), dev.begin(), dev.end());
    blob.insert(blob.end(), cfg.begin(), cfg.end());
    std::vector<USBConfig> configs;
    int32_t ret = UsbDescriptorParser::ParseConfigDescriptors(blob, CURSOR_INIT, configs);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_EQ(configs.size(), static_cast<size_t>(NUM_INTERFACES_ONE));
    EXPECT_EQ(configs[0].GetInterfaceCount(), static_cast<size_t>(NUM_CONFIGURATIONS_ZERO));
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS
