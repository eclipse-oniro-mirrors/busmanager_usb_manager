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
#include <memory>

#include "hilog_wrapper.h"
#include "usb_config.h"
#include "usb_device.h"
#include "usb_host_manager.h"
#include "usb_interface.h"

using namespace testing::ext;
using namespace OHOS::USB;

namespace OHOS {
namespace USB {

constexpr uint8_t TEST_BUS_NUM = 1;
constexpr uint8_t TEST_DEV_ADDR = 2;
constexpr uint8_t TEST_CONFIG_INDEX = 0;

class UsbHostManagerActiveInterfacesTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: UsbHostManager_GetActiveInterfacesJson_001
 * @tc.desc: Normal path: GetActiveConfig fails (usbd_==nullptr) so fallback to cached
 *           configs[0]; 1 config with 2 interfaces -> JSON array with correct field mapping
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerActiveInterfacesTest, UsbHostManager_GetActiveInterfacesJson_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_001 start");

    UsbInterface intf1;
    intf1.SetId(0);
    intf1.SetClass(0x03);
    intf1.SetSubClass(0x01);
    intf1.SetProtocol(0x01);
    intf1.SetAlternateSetting(0);

    UsbInterface intf2;
    intf2.SetId(1);
    intf2.SetClass(0x08);
    intf2.SetSubClass(0x06);
    intf2.SetProtocol(0x50);
    intf2.SetAlternateSetting(1);

    USBConfig config;
    config.SetId(TEST_CONFIG_INDEX);
    config.SetInterfaces({intf1, intf2});

    UsbDevice device;
    device.SetBusNum(TEST_BUS_NUM);
    device.SetDevAddr(TEST_DEV_ADDR);
    device.SetConfigs({config});

    std::unique_ptr<UsbHostManager> usbHostManager = std::make_unique<UsbHostManager>(nullptr);

    nlohmann::json interfacesJson;
    interfacesJson.push_back({
        {"stale", "data"}
    });

    usbHostManager->GetActiveInterfacesJson(&device, interfacesJson);

    EXPECT_EQ(interfacesJson.size(), 2);
    if (interfacesJson.size() >= 2) {
        EXPECT_EQ(interfacesJson[0]["id"], 0);
        EXPECT_EQ(interfacesJson[0]["class"], 0x03);
        EXPECT_EQ(interfacesJson[0]["subclass"], 0x01);
        EXPECT_EQ(interfacesJson[0]["protocol"], 0x01);
        EXPECT_EQ(interfacesJson[0]["altSetting"], 0);
        EXPECT_EQ(interfacesJson[1]["id"], 1);
        EXPECT_EQ(interfacesJson[1]["class"], 0x08);
        EXPECT_EQ(interfacesJson[1]["subclass"], 0x06);
        EXPECT_EQ(interfacesJson[1]["protocol"], 0x50);
        EXPECT_EQ(interfacesJson[1]["altSetting"], 1);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_001 size=%{public}zu", interfacesJson.size());
}

/**
 * @tc.name: UsbHostManager_GetActiveInterfacesJson_002
 * @tc.desc: Multi-config device: fallback selection targets configs[0] (index 0),
 *           NOT configs[1], verifying the index-1 adjustment logic
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerActiveInterfacesTest, UsbHostManager_GetActiveInterfacesJson_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_002 start");

    UsbInterface intf0;
    intf0.SetId(0);
    USBConfig config0;
    config0.SetId(0);
    config0.SetName("Cfg0");
    config0.SetInterfaces({intf0});

    UsbInterface marker;
    marker.SetId(7);
    marker.SetClass(0x09);
    USBConfig config1;
    config1.SetId(1);
    config1.SetName("Cfg1");
    config1.SetInterfaces({marker});

    UsbDevice device;
    device.SetBusNum(TEST_BUS_NUM);
    device.SetDevAddr(TEST_DEV_ADDR);
    device.SetConfigs({config0, config1});

    std::unique_ptr<UsbHostManager> usbHostManager = std::make_unique<UsbHostManager>(nullptr);

    nlohmann::json interfacesJson;
    usbHostManager->GetActiveInterfacesJson(&device, interfacesJson);

    EXPECT_EQ(interfacesJson.size(), 1);
    if (interfacesJson.size() >= 1) {
        EXPECT_EQ(interfacesJson[0]["id"], 0);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_002 size=%{public}zu", interfacesJson.size());
}

/**
 * @tc.name: UsbHostManager_GetActiveInterfacesJson_003
 * @tc.desc: Out-of-range: device has no configs -> index 0 >= size 0 -> output cleared
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerActiveInterfacesTest, UsbHostManager_GetActiveInterfacesJson_003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_003 start");

    UsbDevice device;
    device.SetBusNum(TEST_BUS_NUM);
    device.SetDevAddr(TEST_DEV_ADDR);

    std::unique_ptr<UsbHostManager> usbHostManager = std::make_unique<UsbHostManager>(nullptr);

    nlohmann::json interfacesJson;
    interfacesJson.push_back({
        {"pre", "existing"}
    });

    usbHostManager->GetActiveInterfacesJson(&device, interfacesJson);

    EXPECT_TRUE(interfacesJson.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_003 size=%{public}zu", interfacesJson.size());
}

/**
 * @tc.name: UsbHostManager_GetActiveInterfacesJson_004
 * @tc.desc: Empty-interface config: config exists but has 0 interfaces ->
 *           result is a JSON array (not null) with size 0
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerActiveInterfacesTest, UsbHostManager_GetActiveInterfacesJson_004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_004 start");

    USBConfig config;
    config.SetId(0);
    config.SetInterfaces({});

    UsbDevice device;
    device.SetBusNum(TEST_BUS_NUM);
    device.SetDevAddr(TEST_DEV_ADDR);
    device.SetConfigs({config});

    std::unique_ptr<UsbHostManager> usbHostManager = std::make_unique<UsbHostManager>(nullptr);

    nlohmann::json interfacesJson = nlohmann::json::array();
    usbHostManager->GetActiveInterfacesJson(&device, interfacesJson);

    EXPECT_TRUE(interfacesJson.is_array());
    EXPECT_EQ(interfacesJson.size(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_004 size=%{public}zu", interfacesJson.size());
}
} // namespace USB
} // namespace OHOS