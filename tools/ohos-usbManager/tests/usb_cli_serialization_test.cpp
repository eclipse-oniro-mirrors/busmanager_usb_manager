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
#include "cJSON.h"
#include "usb_cli_serialization.h"
#include "usb_device.h"
#include "usb_accessory.h"
#include "usb_serial_type.h"

using namespace OHOS::USB;
using namespace testing::ext;

class UsbCliSerializationTest : public testing::Test {};

HWTEST_F(UsbCliSerializationTest, SerializeDevice_PublicApiFields, TestSize.Level1)
{
    std::vector<USBConfig> configs;
    UsbDevice device("test_device", "test_mfg", "test_product", "1.0",
        2, 1, 0x1234, 0x5678, 0, 0, 0, configs);
    device.SetmSerial("SN12345");

    cJSON *json = SerializeDevice(device);
    ASSERT_NE(json, nullptr);

    cJSON *item = cJSON_GetObjectItem(json, "busNum");
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->valueint, 1);

    item = cJSON_GetObjectItem(json, "devAddress");
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->valueint, 2);

    item = cJSON_GetObjectItem(json, "serial");
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(item->valuestring, "SN12345");

    item = cJSON_GetObjectItem(json, "name");
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(item->valuestring, "test_device");

    item = cJSON_GetObjectItem(json, "manufacturerName");
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(item->valuestring, "test_mfg");

    item = cJSON_GetObjectItem(json, "productName");
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(item->valuestring, "test_product");

    item = cJSON_GetObjectItem(json, "vendorId");
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->valueint, 0x1234);

    item = cJSON_GetObjectItem(json, "productId");
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->valueint, 0x5678);

    item = cJSON_GetObjectItem(json, "clazz");
    ASSERT_NE(item, nullptr);

    item = cJSON_GetObjectItem(json, "subClass");
    ASSERT_NE(item, nullptr);

    item = cJSON_GetObjectItem(json, "protocol");
    ASSERT_NE(item, nullptr);

    item = cJSON_GetObjectItem(json, "configs");
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(cJSON_IsArray(item));

    cJSON_Delete(json);
}

HWTEST_F(UsbCliSerializationTest, SerializeDevice_NoInternalFields, TestSize.Level1)
{
    std::vector<USBConfig> configs;
    UsbDevice device("name", "mfg", "prod", "1.0", 1, 1, 0, 0, 0, 0, 0, configs);

    cJSON *json = SerializeDevice(device);
    ASSERT_NE(json, nullptr);

    EXPECT_EQ(cJSON_GetObjectItem(json, "devAddr"), nullptr);
    EXPECT_EQ(cJSON_GetObjectItem(json, "baseClass"), nullptr);
    EXPECT_EQ(cJSON_GetObjectItem(json, "iManufacturer"), nullptr);
    EXPECT_EQ(cJSON_GetObjectItem(json, "iProduct"), nullptr);
    EXPECT_EQ(cJSON_GetObjectItem(json, "iSerialNumber"), nullptr);
    EXPECT_EQ(cJSON_GetObjectItem(json, "bMaxPacketSize0"), nullptr);
    EXPECT_EQ(cJSON_GetObjectItem(json, "bcdUSB"), nullptr);
    EXPECT_EQ(cJSON_GetObjectItem(json, "bcdDevice"), nullptr);
    EXPECT_EQ(cJSON_GetObjectItem(json, "authorizeStatus"), nullptr);

    cJSON_Delete(json);
}

HWTEST_F(UsbCliSerializationTest, SerializeAccessory_AllFields, TestSize.Level1)
{
    USBAccessory accessory("mfg", "prod", "desc", "1.0", "SN001");

    cJSON *json = SerializeAccessory(accessory);
    ASSERT_NE(json, nullptr);

    cJSON *item = cJSON_GetObjectItem(json, "manufacturer");
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(item->valuestring, "mfg");

    item = cJSON_GetObjectItem(json, "product");
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(item->valuestring, "prod");

    item = cJSON_GetObjectItem(json, "description");
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(item->valuestring, "desc");

    item = cJSON_GetObjectItem(json, "version");
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(item->valuestring, "1.0");

    item = cJSON_GetObjectItem(json, "serialNumber");
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(item->valuestring, "SN001");

    cJSON_Delete(json);
}

HWTEST_F(UsbCliSerializationTest, SerializeSerialPort_PublicApiFields, TestSize.Level1)
{
    UsbSerialPort port;
    port.portId_ = 3;
    port.busNum_ = 5;
    port.devAddr_ = 7;
    port.vid_ = 0x1234;
    port.pid_ = 0x5678;
    port.serialNum_ = "SN001";

    cJSON *json = SerializeSerialPort(port);
    ASSERT_NE(json, nullptr);

    cJSON *item = cJSON_GetObjectItem(json, "portId");
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->valueint, 3);

    item = cJSON_GetObjectItem(json, "busNum");
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->valueint, 5);

    item = cJSON_GetObjectItem(json, "devAddr");
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->valueint, 7);

    item = cJSON_GetObjectItem(json, "vid");
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->valueint, 0x1234);

    item = cJSON_GetObjectItem(json, "pid");
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->valueint, 0x5678);

    item = cJSON_GetObjectItem(json, "serialNum");
    ASSERT_NE(item, nullptr);
    EXPECT_STREQ(item->valuestring, "SN001");

    cJSON_Delete(json);
}

HWTEST_F(UsbCliSerializationTest, FormatSuccess_EmptyList, TestSize.Level1)
{
    std::vector<cJSON*> items;
    std::string result = FormatSuccess(items);

    cJSON *root = cJSON_Parse(result.c_str());
    ASSERT_NE(root, nullptr);

    cJSON *number = cJSON_GetObjectItem(root, "number");
    ASSERT_NE(number, nullptr);
    EXPECT_EQ(number->valueint, 0);

    cJSON *list = cJSON_GetObjectItem(root, "list");
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(cJSON_GetArraySize(list), 0);

    cJSON_Delete(root);
}

HWTEST_F(UsbCliSerializationTest, FormatSuccess_WithItems, TestSize.Level1)
{
    USBAccessory acc1("mfg1", "prod1", "desc1", "1.0", "SN1");
    USBAccessory acc2("mfg2", "prod2", "desc2", "2.0", "SN2");

    std::vector<cJSON*> items;
    items.push_back(SerializeAccessory(acc1));
    items.push_back(SerializeAccessory(acc2));

    std::string result = FormatSuccess(items);

    cJSON *root = cJSON_Parse(result.c_str());
    ASSERT_NE(root, nullptr);

    cJSON *number = cJSON_GetObjectItem(root, "number");
    ASSERT_NE(number, nullptr);
    EXPECT_EQ(number->valueint, 2);

    cJSON *list = cJSON_GetObjectItem(root, "list");
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(cJSON_GetArraySize(list), 2);

    cJSON_Delete(root);
    for (auto *item : items) {
        cJSON_Delete(item);
    }
}

HWTEST_F(UsbCliSerializationTest, FormatError_Structure, TestSize.Level1)
{
    std::string result = FormatError(42, "test error");

    cJSON *root = cJSON_Parse(result.c_str());
    ASSERT_NE(root, nullptr);

    cJSON *code = cJSON_GetObjectItem(root, "code");
    ASSERT_NE(code, nullptr);
    EXPECT_EQ(code->valueint, 42);

    cJSON *message = cJSON_GetObjectItem(root, "message");
    ASSERT_NE(message, nullptr);
    EXPECT_STREQ(message->valuestring, "test error");

    cJSON_Delete(root);
}
