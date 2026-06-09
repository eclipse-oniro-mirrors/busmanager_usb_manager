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

#include "usb_descriptor_parser.h"
#include "usb_device.h"
#include "usb_errors.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;

class UsbDescriptorParserTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::unique_ptr<UsbDescriptorParser> usbDescriptorParser_;
};

void UsbDescriptorParserTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserTest::SetUpTestCase enter");
}

void UsbDescriptorParserTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserTest::TearDownTestCase enter");
}

void UsbDescriptorParserTest::SetUp()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserTest::SetUp enter");
    usbDescriptorParser_ = std::make_unique<UsbDescriptorParser>();
    ASSERT_NE(usbDescriptorParser_, nullptr);
}

void UsbDescriptorParserTest::TearDown()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParserTest::TearDown enter");
    usbDescriptorParser_.reset();
}

/**
 * @tc.name: UsbDescriptorParser_EmptyDescriptor
 * @tc.desc: Test parsing empty descriptor buffer
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_EmptyDescriptor, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_EmptyDescriptor start");

    std::vector<uint8_t> emptyBuffer;
    UsbDevice device;

    int32_t ret = usbDescriptorParser_->ParseDeviceDescriptor(emptyBuffer, device);

    EXPECT_NE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_EmptyDescriptor ret=%{public}d", ret);
}

/**
 * @tc.name: UsbDescriptorParser_InvalidDescriptorLength
 * @tc.desc: Test parsing descriptor with invalid length
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_InvalidDescriptorLength, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_InvalidDescriptorLength start");

    std::vector<uint8_t> shortBuffer = {0x12};
    UsbDevice device;

    int32_t ret = usbDescriptorParser_->ParseDeviceDescriptor(shortBuffer, device);

    EXPECT_NE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_InvalidDescriptorLength ret=%{public}d", ret);
}

/**
 * @tc.name: UsbDescriptorParser_InvalidDescriptorType
 * @tc.desc: Test parsing descriptor with invalid type
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_InvalidDescriptorType, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_InvalidDescriptorType start");

    std::vector<uint8_t> buffer(18, 0xFF);
    buffer[0] = 18;
    buffer[1] = 0xFF;

    UsbDevice device;

    int32_t ret = usbDescriptorParser_->ParseDeviceDescriptor(buffer, device);

    EXPECT_NE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_InvalidDescriptorType ret=%{public}d", ret);
}

/**
 * @tc.name: UsbDescriptorParser_MinimalValidDescriptor
 * @tc.desc: Test parsing minimal valid device descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_MinimalValidDescriptor, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_MinimalValidDescriptor start");

    std::vector<uint8_t> buffer = {
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x34, 0x12, 0x78, 0x56, 0x00, 0x00, 0x01, 0x02,
        0x03, 0x01
    };

    UsbDevice device;

    int32_t ret = usbDescriptorParser_->ParseDeviceDescriptor(buffer, device);

    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_MinimalValidDescriptor ret=%{public}d", ret);
}

/**
 * @tc.name: UsbDescriptorParser_ConfigurationDescriptor
 * @tc.desc: Test parsing configuration descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_ConfigurationDescriptor, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_ConfigurationDescriptor start");

    std::vector<uint8_t> buffer = {
        0x09, 0x02, 0x1E, 0x00, 0x01, 0x00, 0x00, 0xE0, 0x00
    };

    UsbConfig config;

    int32_t ret = usbDescriptorParser_->ParseConfigDescriptor(buffer, config);

    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_ConfigurationDescriptor ret=%{public}d", ret);
}

/**
 * @tc.name: UsbDescriptorParser_InterfaceDescriptor
 * @tc.desc: Test parsing interface descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_InterfaceDescriptor, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_InterfaceDescriptor start");

    std::vector<uint8_t> buffer = {
        0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00
    };

    UsbInterface interface;

    int32_t ret = usbDescriptorParser_->ParseInterfaceDescriptor(buffer, interface);

    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_InterfaceDescriptor ret=%{public}d", ret);
}

/**
 * @tc.name: UsbDescriptorParser_EndpointDescriptor
 * @tc.desc: Test parsing endpoint descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_EndpointDescriptor, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_EndpointDescriptor start");

    std::vector<uint8_t> buffer = {
        0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00
    };

    UsbEndpoint endpoint;

    int32_t ret = usbDescriptorParser_->ParseEndpointDescriptor(buffer, endpoint);

    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_EndpointDescriptor ret=%{public}d", ret);
}

/**
 * @tc.name: UsbDescriptorParser_LargeDescriptorBuffer
 * @tc.desc: Test parsing large descriptor buffer
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_LargeDescriptorBuffer, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_LargeDescriptorBuffer start");

    std::vector<uint8_t> largeBuffer(4096, 0);
    largeBuffer[0] = 18;
    largeBuffer[1] = 0x01;

    UsbDevice device;

    int32_t ret = usbDescriptorParser_->ParseDeviceDescriptor(largeBuffer, device);

    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_LargeDescriptorBuffer ret=%{public}d", ret);
}

/**
 * @tc.name: UsbDescriptorParser_AllInterfaceClasses
 * @tc.desc: Test parsing descriptors for all standard interface classes
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_AllInterfaceClasses, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_AllInterfaceClasses start");

    std::vector<uint8_t> interfaceClasses = {0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07,
                                               0x08, 0x09, 0x0A, 0x0B, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12};

    for (uint8_t classCode : interfaceClasses) {
        std::vector<uint8_t> buffer = {
            0x09, 0x04, 0x00, 0x00, 0x01, classCode, 0x01, 0x00, 0x00
        };

        UsbInterface interface;
        int32_t ret = usbDescriptorParser_->ParseInterfaceDescriptor(buffer, interface);
        USB_HILOGI(MODULE_USB_HOST, "ParseInterface class=0x%{public}X ret=%{public}d", classCode, ret);
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_AllInterfaceClasses end");
}

/**
 * @tc.name: UsbDescriptorParser_EndpointTypes
 * @tc.desc: Test parsing descriptors for different endpoint types
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_EndpointTypes, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_EndpointTypes start");

    std::vector<std::pair<uint8_t, uint8_t>> endpointTypes = {
        {0x01, 0x02},  // Control
        {0x02, 0x02},  // Bulk
        {0x03, 0x01},  // Interrupt
        {0x03, 0x02}   // Isochronous
    };

    for (const auto& epType : endpointTypes) {
        std::vector<uint8_t> buffer = {
            0x07, 0x05, 0x81, (epType.first << 5) | epType.second, 0x40, 0x00, 0x00
        };

        UsbEndpoint endpoint;
        int32_t ret = usbDescriptorParser_->ParseEndpointDescriptor(buffer, endpoint);
        USB_HILOGI(MODULE_USB_HOST, "ParseEndpoint type=%{public}u ret=%{public}d",
                    epType.first, ret);
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_EndpointTypes end");
}

/**
 * @tc.name: UsbDescriptorParser_MultipleConfigurations
 * @tc.desc: Test parsing device with multiple configurations
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_MultipleConfigurations, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_MultipleConfigurations start");

    std::vector<uint8_t> buffer = {
        0x12, 0x01, 0x00, 0x02, 0xFF, 0xFF, 0x02, 0x40,
        0x34, 0x12, 0x78, 0x56, 0x00, 0x00, 0x01, 0x02,
        0x03, 0x01,
        0x09, 0x02, 0x1E, 0x00, 0x01, 0x00, 0x00, 0xE0, 0x00,
        0x09, 0x02, 0x1E, 0x00, 0x01, 0x00, 0x00, 0xE0, 0x00
    };

    UsbDevice device;

    int32_t ret = usbDescriptorParser_->ParseDeviceDescriptor(buffer, device);

    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_MultipleConfigurations ret=%{public}d", ret);
}

/**
 * @tc.name: UsbDescriptorParser_MalformedDescriptor
 * @tc.desc: Test parsing malformed descriptor with inconsistent lengths
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_MalformedDescriptor, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_MalformedDescriptor start");

    std::vector<uint8_t> malformedBuffer = {
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x34, 0x12, 0x78, 0x56, 0x00, 0x00, 0x01, 0x02,
        0x03, 0x01
    };

    malformedBuffer[0] = 0xFF;

    UsbDevice device;

    int32_t ret = usbDescriptorParser_->ParseDeviceDescriptor(malformedBuffer, device);

    EXPECT_NE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_MalformedDescriptor ret=%{public}d", ret);
}

/**
 * @tc.name: UsbDescriptorParser_StringDescriptor
 * @tc.desc: Test parsing string descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_StringDescriptor, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_StringDescriptor start");

    std::vector<uint8_t> buffer = {
        0x04, 0x03, 0x09, 0x04
    };

    std::string stringData;

    int32_t ret = usbDescriptorParser_->ParseStringDescriptor(buffer, stringData);

    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_StringDescriptor ret=%{public}d", ret);
}

/**
 * @tc.name: UsbDescriptorParser_TruncatedDescriptor
 * @tc.desc: Test parsing truncated descriptor
 * @tc.type: FUNC
 */
HWTEST_F(UsbDescriptorParserTest, UsbDescriptorParser_TruncatedDescriptor, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_TruncatedDescriptor start");

    std::vector<uint8_t> truncatedBuffer = {
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00
    };

    UsbDevice device;

    int32_t ret = usbDescriptorParser_->ParseDeviceDescriptor(truncatedBuffer, device);

    EXPECT_NE(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbDescriptorParser_TruncatedDescriptor ret=%{public}d", ret);
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS