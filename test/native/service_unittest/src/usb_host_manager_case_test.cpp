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
#include <new>

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
constexpr uint8_t DYN_TEST_BUS_NUM_1 = 1;
constexpr uint8_t DYN_TEST_DEV_ADDR_1 = 1;
constexpr uint8_t DYN_TEST_BUS_NUM_2 = 2;
constexpr uint8_t DYN_TEST_DEV_ADDR_2 = 2;

/**
 * @brief Unit tests for USB dynamic control mode (requirement 20260813045248).
 *
 * Covered acceptance criteria:
 * - AC-1.2: dynamic control mode keeps the legacy behaviour when it is disabled.
 * - AC-3.1: full device query returns devices that the existing query filters out.
 * - AC-2.1 / AC-2.2: attach and detach events are delivered to the subscribed sink.
 * - Error code contract: parameter out of range / target device not attached reuse the
 *   existing enums UEC_INTERFACE_INVALID_VALUE / UEC_INTERFACE_INVALID_OPERATION.
 */
class UsbDynamicControlTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    UsbDevice *CreateTestDevice(uint8_t busNum, uint8_t devAddr, AuthorizeStatus status);

    std::unique_ptr<UsbHostManager> usbHostManager_ = nullptr;
};

void UsbDynamicControlTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControlTest::SetUpTestCase enter");
}

void UsbDynamicControlTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControlTest::TearDownTestCase enter");
}

void UsbDynamicControlTest::SetUp()
{
    SystemAbility *systemAbility = nullptr;
    usbHostManager_ = std::make_unique<UsbHostManager>(systemAbility);
    ASSERT_NE(usbHostManager_, nullptr);
}

void UsbDynamicControlTest::TearDown()
{
    // UsbHostManager owns the devices added through AddDevice() and releases them in its destructor
    usbHostManager_.reset();
}

UsbDevice *UsbDynamicControlTest::CreateTestDevice(uint8_t busNum, uint8_t devAddr, AuthorizeStatus status)
{
    auto device = new (std::nothrow) UsbDevice();
    if (device == nullptr) {
        return nullptr;
    }
    device->SetBusNum(busNum);
    device->SetDevAddr(devAddr);
    device->SetVendorId(0x1234);
    device->SetProductId(0x5678);
    device->SetClass(0x08);
    device->SetSubclass(0x01);
    device->SetProtocol(0x00);
    device->SetProductName("DynamicControlTestDevice");
    device->SetManufacturerName("TestManufacturer");
    device->SetmSerial("SN20260813045248");
    device->SetVersion("1.0.0");
    device->SetAuthorizeStatus(status);
    return device;
}

/**
 * @tc.name: UsbDynamicControl_ModeSwitch_DefaultOffAndToggle
 * @tc.desc: mode is disabled by default and can be toggled (AC-1.2 baseline, task 1)
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_ModeSwitch_DefaultOffAndToggle, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_ModeSwitch_DefaultOffAndToggle start");
    EXPECT_FALSE(usbHostManager_->IsDynamicControlEnabled());

    usbHostManager_->SetDynamicControlEnabled(true);
    EXPECT_TRUE(usbHostManager_->IsDynamicControlEnabled());

    // repeated set keeps the same state (idempotent)
    usbHostManager_->SetDynamicControlEnabled(true);
    EXPECT_TRUE(usbHostManager_->IsDynamicControlEnabled());

    usbHostManager_->SetDynamicControlEnabled(false);
    EXPECT_FALSE(usbHostManager_->IsDynamicControlEnabled());
}

/**
 * @tc.name: UsbDynamicControl_ErrorCodeContract
 * @tc.desc: failure codes reuse existing enums (parameter out of range / device not attached)
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_ErrorCodeContract, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_ErrorCodeContract start");
    EXPECT_EQ(DYNAMIC_CONTROL_OK, 0);
    EXPECT_EQ(DYNAMIC_CONTROL_INVALID_PARAM, UEC_INTERFACE_INVALID_VALUE);
    EXPECT_EQ(DYNAMIC_CONTROL_DEVICE_NOT_ATTACHED, UEC_INTERFACE_INVALID_OPERATION);
    // parameter name is a single point of truth, pending COND-02 contract freeze
    EXPECT_STREQ(DYNAMIC_CONTROL_MODE_PARAM, "persist.edm.usb_dynamic_control_enable");
}

/**
 * @tc.name: UsbDynamicControl_FullQuery_ContainsDisabledDevice
 * @tc.desc: full query returns disabled devices while the legacy query keeps filtering them (AC-3.1)
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_FullQuery_ContainsDisabledDevice, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_FullQuery_ContainsDisabledDevice start");
    // enable dynamic control so the strategy entry takes the dynamic-control branch and the test
    // does not depend on the host EDM policy state
    usbHostManager_->SetDynamicControlEnabled(true);
    UsbDevice *device = CreateTestDevice(DYN_TEST_BUS_NUM_1, DYN_TEST_DEV_ADDR_1, ENABLED);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(usbHostManager_->AddDevice(device));

    // simulate the state written by the dynamic control strategy on a disabled device
    device->SetAuthorizeStatus(DISABLED);

    std::vector<UsbDevice> allDevices;
    EXPECT_EQ(usbHostManager_->GetAllDevices(allDevices), UEC_OK);
    EXPECT_EQ(allDevices.size(), 1);

    std::vector<UsbDevice> enabledDevices;
    EXPECT_EQ(usbHostManager_->GetDevices(enabledDevices), UEC_OK);
    EXPECT_TRUE(enabledDevices.empty());
}

/**
 * @tc.name: UsbDynamicControl_EmptyManager_QueryReturnsEmpty
 * @tc.desc: full query on an empty manager returns OK and an empty list (boundary)
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_EmptyManager_QueryReturnsEmpty, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_EmptyManager_QueryReturnsEmpty start");
    std::vector<UsbDevice> allDevices;
    EXPECT_EQ(usbHostManager_->GetAllDevices(allDevices), UEC_OK);
    EXPECT_TRUE(allDevices.empty());
}

/**
 * @tc.name: UsbDynamicControl_EventSink_AttachAndDetach
 * @tc.desc: registered sink receives one attach event and one detach event (AC-2.1, AC-2.2)
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_EventSink_AttachAndDetach, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_EventSink_AttachAndDetach start");
    usbHostManager_->SetDynamicControlEnabled(true);
    int32_t eventCount = 0;
    bool lastAttached = false;
    AuthorizeStatus lastStatus = NEW_ARRIVED;
    uint8_t lastBusNum = 0;
    uint8_t lastDevAddr = 0;

    usbHostManager_->SetDeviceControlEventSink(
        [&eventCount, &lastAttached, &lastStatus, &lastBusNum, &lastDevAddr](const UsbDevice &device, bool attached) {
            eventCount++;
            lastAttached = attached;
            UsbDevice snapshot(device);
            lastStatus = snapshot.GetAuthorizeStatus();
            lastBusNum = snapshot.GetBusNum();
            lastDevAddr = snapshot.GetDevAddr();
        });

    UsbDevice *device = CreateTestDevice(DYN_TEST_BUS_NUM_2, DYN_TEST_DEV_ADDR_2, NEW_ARRIVED);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(usbHostManager_->AddDevice(device));

    EXPECT_EQ(eventCount, 1);
    EXPECT_TRUE(lastAttached);
    EXPECT_EQ(lastBusNum, DYN_TEST_BUS_NUM_2);
    EXPECT_EQ(lastDevAddr, DYN_TEST_DEV_ADDR_2);
    EXPECT_EQ(lastStatus, ENABLED);

    EXPECT_TRUE(usbHostManager_->DelDevice(DYN_TEST_BUS_NUM_2, DYN_TEST_DEV_ADDR_2));
    EXPECT_EQ(eventCount, 2);
    EXPECT_FALSE(lastAttached);
    EXPECT_EQ(lastBusNum, DYN_TEST_BUS_NUM_2);
    EXPECT_EQ(lastDevAddr, DYN_TEST_DEV_ADDR_2);
}

/**
 * @tc.name: UsbDynamicControl_EventSink_NotSet
 * @tc.desc: attach and detach work when no subscriber is registered (boundary)
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_EventSink_NotSet, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_EventSink_NotSet start");
    usbHostManager_->SetDynamicControlEnabled(true);
    UsbDevice *device = CreateTestDevice(DYN_TEST_BUS_NUM_1, DYN_TEST_DEV_ADDR_2, NEW_ARRIVED);
    ASSERT_NE(device, nullptr);

    EXPECT_TRUE(usbHostManager_->AddDevice(device));
    EXPECT_TRUE(usbHostManager_->DelDevice(DYN_TEST_BUS_NUM_1, DYN_TEST_DEV_ADDR_2));
}

/**
 * @tc.name: UsbDynamicControl_Detach_NeverAttachedDevice
 * @tc.desc: detaching an unknown device fails without raising events (boundary)
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_Detach_NeverAttachedDevice, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_Detach_NeverAttachedDevice start");
    int32_t eventCount = 0;
    usbHostManager_->SetDeviceControlEventSink([&eventCount](const UsbDevice &device, bool attached) { eventCount++; });

    EXPECT_FALSE(usbHostManager_->DelDevice(DYN_TEST_BUS_NUM_1, DYN_TEST_DEV_ADDR_1));
    EXPECT_EQ(eventCount, 0);
}

/**
 * @tc.name: UsbDynamicControl_HubExcluded
 * @tc.desc: HUB class is excluded from dynamic default-disable; other classes are not
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_HubExcluded, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_HubExcluded start");
    EXPECT_TRUE(IsDynamicControlExcludedDevice(USB_BASE_CLASS_HUB));
    EXPECT_FALSE(IsDynamicControlExcludedDevice(0x08));
    EXPECT_FALSE(IsDynamicControlExcludedDevice(0x00));
}

/**
 * @tc.name: UsbDynamicControl_DeviceStatusSerialization
 * @tc.desc: authorization state survives Marshalling/Unmarshalling so GetAllDevices can report DISABLED
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_DeviceStatusSerialization, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_DeviceStatusSerialization start");
    UsbDevice *device = CreateTestDevice(DYN_TEST_BUS_NUM_1, DYN_TEST_DEV_ADDR_1, DISABLED);
    ASSERT_NE(device, nullptr);
    Parcel parcel;
    ASSERT_TRUE(device->Marshalling(parcel));
    parcel.RewindRead(0);
    UsbDevice *out = UsbDevice::Unmarshalling(parcel);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetAuthorizeStatus(), DISABLED);
    delete out;
    delete device;
}

/**
 * @tc.name: UsbDynamicControl_InterfaceStatusSerialization
 * @tc.desc: interface authorization state survives Marshalling/Unmarshalling
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_InterfaceStatusSerialization, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_InterfaceStatusSerialization start");
    UsbInterface intf;
    intf.SetAuthorizeStatus(false);
    Parcel parcel;
    ASSERT_TRUE(intf.Marshalling(parcel));
    parcel.RewindRead(0);
    UsbInterface *out = UsbInterface::Unmarshalling(parcel);
    ASSERT_NE(out, nullptr);
    EXPECT_FALSE(out->GetAuthorizeStatus());
    delete out;
}

} // namespace USB
} // namespace OHOS