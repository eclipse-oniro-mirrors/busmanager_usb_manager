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

#include <cstdlib>
#include <gtest/gtest.h>
#include <memory>
#include <new>

#include "hilog_wrapper.h"
#include "ipc_object_stub.h"
#include "usb_config.h"
#include "usb_device.h"
#include "usb_host_manager.h"
#include "usb_interface.h"
#include "usb_server_proxy.h"
#include "usb_service.h"
#include "usb_srv_client.h"

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

class MockUsbDeviceInterface : public OHOS::HDI::Usb::V2_0::IUsbDeviceInterface {
public:
    struct DeviceAuthorizeCall {
        uint8_t busNum;
        uint8_t devAddr;
        bool authorized;
    };
    struct InterfaceAuthorizeCall {
        uint8_t busNum;
        uint8_t devAddr;
        uint8_t configId;
        uint8_t interfaceId;
        bool authorized;
    };

    int32_t GetCurrentFunctions(int32_t &funcs) override
    {
        return 0;
    }
    int32_t SetCurrentFunctions(int32_t funcs) override
    {
        return 0;
    }
    int32_t GetAccessoryInfo(std::vector<std::string> &accessoryInfo) override
    {
        return 0;
    }
    int32_t OpenAccessory(int &fd) override
    {
        return 0;
    }
    int32_t CloseAccessory(int fd) override
    {
        return 0;
    }
    int32_t BindUsbdDeviceSubscriber(const sptr<OHOS::HDI::Usb::V2_0::IUsbdSubscriber> &subscriber) override
    {
        return 0;
    }
    int32_t UnbindUsbdDeviceSubscriber(const sptr<OHOS::HDI::Usb::V2_0::IUsbdSubscriber> &subscriber) override
    {
        return 0;
    }
    int32_t UsbDeviceAuthorize(uint8_t busNum, uint8_t devAddr, bool authorized) override
    {
        deviceCalls.push_back({busNum, devAddr, authorized});
        return deviceRet;
    }
    int32_t UsbInterfaceAuthorize(const OHOS::HDI::Usb::V2_0::UsbDev &dev, uint8_t configId, uint8_t interfaceId,
                                  bool authorized) override
    {
        interfaceCalls.push_back({dev.busNum, dev.devAddr, configId, interfaceId, authorized});
        return interfaceRet;
    }

    std::vector<DeviceAuthorizeCall> deviceCalls;
    std::vector<InterfaceAuthorizeCall> interfaceCalls;
    int32_t deviceRet = 0;
    int32_t interfaceRet = 0;
};

class MockDeviceControlRemote : public IPCObjectStub {
public:
    MockDeviceControlRemote() : IPCObjectStub(u"MockDeviceControlRemote") {}
    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        lastCode = code;
        UsbDevice *device = UsbDevice::Unmarshalling(data);
        if (device != nullptr) {
            lastSerial = device->GetmSerial();
            delete device;
        }
        lastAuthorizeStatus = data.ReadInt32();
        lastAttached = data.ReadBool();
        sendCount++;
        return sendRet;
    }

    uint32_t lastCode = 0;
    std::string lastSerial;
    int32_t lastAuthorizeStatus = -1;
    bool lastAttached = false;
    int32_t sendRet = 0;
    int32_t sendCount = 0;
};

class MockUsbServerRemote : public IPCObjectStub {
public:
    MockUsbServerRemote() : IPCObjectStub(u"MockUsbServerRemote") {}
    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        if (code == static_cast<uint32_t>(IUsbServerIpcCode::COMMAND_GET_ALL_DEVICES)) {
            reply.WriteInt32(UEC_OK);
            reply.WriteInt32(static_cast<int32_t>(deviceInfos.size()));
            for (auto &info : deviceInfos) {
                if (UsbDeviceInfoBlockMarshalling(reply, info) != 0) {
                    return -1;
                }
            }
        } else if (code == static_cast<uint32_t>(IUsbServerIpcCode::COMMAND_USB_DEVICE_AUTHORIZE) ||
                   code == static_cast<uint32_t>(IUsbServerIpcCode::COMMAND_USB_INTERFACE_AUTHORIZE)) {
            reply.WriteInt32(authorizeRet);
        } else {
            reply.WriteInt32(UEC_OK);
        }
        return 0;
    }

    std::vector<UsbDeviceInfo> deviceInfos;
    int32_t authorizeRet = UEC_OK;
};

UsbDevice *CreateServiceTestDevice(uint8_t busNum, uint8_t devAddr, int32_t deviceClass = 0x08)
{
    auto device = new (std::nothrow) UsbDevice();
    if (device == nullptr) {
        return nullptr;
    }
    device->SetBusNum(busNum);
    device->SetDevAddr(devAddr);
    device->SetVendorId(0x1234);
    device->SetProductId(0x5678);
    device->SetClass(deviceClass);
    device->SetSubclass(0x01);
    device->SetProtocol(0x00);
    device->SetProductName("ServiceTestDevice");
    device->SetManufacturerName("ServiceTestManufacturer");
    device->SetmSerial("SERVICE-SN");
    device->SetVersion("1.0.0");
    device->SetAuthorizeStatus(ENABLED);
    return device;
}

UsbDevice *CreateCompositeTestDevice(uint8_t busNum, uint8_t devAddr, bool interfaceEnabled)
{
    UsbDevice *device = CreateServiceTestDevice(busNum, devAddr);
    if (device == nullptr) {
        return nullptr;
    }
    UsbInterface interface;
    interface.SetId(0);
    interface.SetClass(0x08);
    interface.SetSubClass(0x06);
    interface.SetProtocol(0x50);
    interface.SetAlternateSetting(0);
    interface.SetAuthorizeStatus(interfaceEnabled);
    USBConfig config;
    config.SetId(1);
    config.SetInterfaces({interface});
    device->SetConfigs({config});
    return device;
}

class UsbDynamicControlServiceTest : public testing::Test {
public:
    void SetUp() override
    {
        UsbHostManager::SetEdmEnabledForTest(0);
        UsbService::SetCallerValidForTest(1);
        service_ = new (std::nothrow) UsbService();
        ASSERT_NE(service_, nullptr);
        mock_ = new (std::nothrow) MockUsbDeviceInterface();
        ASSERT_NE(mock_, nullptr);
        service_->usbHostManager_->usbDeviceInterface_ = mock_;
    }

    void TearDown() override
    {
        service_ = nullptr;
        UsbService::SetCallerValidForTest(-1);
        UsbHostManager::SetEdmEnabledForTest(-1);
    }

    sptr<UsbService> service_;
    sptr<MockUsbDeviceInterface> mock_;
};

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
 *   existing enums UEC_INTERFACE_INVALID_VALUE / UEC_INTERFACE_NAME_NOT_FOUND.
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

    usbHostManager_->InitDynamicControlEnabled(true);
    EXPECT_TRUE(usbHostManager_->IsDynamicControlEnabled());

    // repeated set keeps the same state (idempotent)
    usbHostManager_->InitDynamicControlEnabled(true);
    EXPECT_TRUE(usbHostManager_->IsDynamicControlEnabled());

    usbHostManager_->InitDynamicControlEnabled(false);
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
    // failure paths use the closest existing framework error codes rather than UEC_OK
    EXPECT_NE(UEC_INTERFACE_INVALID_VALUE, UEC_OK);
    EXPECT_NE(UEC_INTERFACE_NAME_NOT_FOUND, UEC_OK);
    EXPECT_NE(UEC_INTERFACE_INVALID_OPERATION, UEC_OK);
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
    usbHostManager_->InitDynamicControlEnabled(true);
    UsbDevice *device = CreateTestDevice(DYN_TEST_BUS_NUM_1, DYN_TEST_DEV_ADDR_1, ENABLED);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(usbHostManager_->AddDevice(device));

    // simulate the state written by the dynamic control strategy on a disabled device
    device->SetAuthorizeStatus(DISABLED);

    std::vector<UsbDeviceInfo> allDevices;
    EXPECT_EQ(usbHostManager_->GetAllDevices(allDevices), UEC_OK);
    EXPECT_EQ(allDevices.size(), 1);
    EXPECT_FALSE(allDevices[0].authorized);
    EXPECT_EQ(allDevices[0].deviceId, "1-1");

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
    std::vector<UsbDeviceInfo> allDevices;
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
    usbHostManager_->InitDynamicControlEnabled(true);
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
    usbHostManager_->InitDynamicControlEnabled(true);
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
    EXPECT_TRUE(IsDynamicControlExcludedDevice(BASE_CLASS_HUB));
    EXPECT_FALSE(IsDynamicControlExcludedDevice(0x08));
    EXPECT_FALSE(IsDynamicControlExcludedDevice(0x00));
}

/**
 * @tc.name: UsbDynamicControl_RootHubDetection
 * @tc.desc: only the root hub (hub class at address 1) is excluded from inner API control
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_RootHubDetection, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_RootHubDetection start");
    UsbDevice *rootHub = CreateTestDevice(DYN_TEST_BUS_NUM_1, 1, NEW_ARRIVED);
    ASSERT_NE(rootHub, nullptr);
    rootHub->SetClass(BASE_CLASS_HUB);
    EXPECT_TRUE(IsRootHub(*rootHub));

    UsbDevice *nonRootHub = CreateTestDevice(DYN_TEST_BUS_NUM_1, 2, NEW_ARRIVED);
    ASSERT_NE(nonRootHub, nullptr);
    nonRootHub->SetClass(BASE_CLASS_HUB);
    EXPECT_FALSE(IsRootHub(*nonRootHub));

    UsbDevice *normalDevice = CreateTestDevice(DYN_TEST_BUS_NUM_1, 3, NEW_ARRIVED);
    ASSERT_NE(normalDevice, nullptr);
    EXPECT_FALSE(IsRootHub(*normalDevice));

    delete rootHub;
    delete nonRootHub;
    delete normalDevice;
}

/**
 * @tc.name: UsbDynamicControl_ModeEnterDisablesAllNonHub
 * @tc.desc: entering the mode disables every non-HUB device and skips HUB
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_ModeEnterDisablesAllNonHub, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_ModeEnterDisablesAllNonHub start");
    sptr<MockUsbDeviceInterface> mock = new (std::nothrow) MockUsbDeviceInterface();
    ASSERT_NE(mock, nullptr);
    usbHostManager_->usbDeviceInterface_ = mock;

    UsbDevice *device1 = CreateTestDevice(DYN_TEST_BUS_NUM_1, 2, ENABLED);
    UsbDevice *device2 = CreateTestDevice(DYN_TEST_BUS_NUM_1, 3, ENABLED);
    UsbDevice *hub = CreateTestDevice(DYN_TEST_BUS_NUM_1, 4, ENABLED);
    ASSERT_NE(device1, nullptr);
    ASSERT_NE(device2, nullptr);
    ASSERT_NE(hub, nullptr);
    hub->SetClass(BASE_CLASS_HUB);
    ASSERT_TRUE(usbHostManager_->AddDevice(device1));
    ASSERT_TRUE(usbHostManager_->AddDevice(device2));
    ASSERT_TRUE(usbHostManager_->AddDevice(hub));
    mock->deviceCalls.clear();

    usbHostManager_->InitDynamicControlEnabled(true);
    usbHostManager_->SetAllNonHubDevicesAuthorized(false);

    ASSERT_EQ(mock->deviceCalls.size(), 2);
    for (const auto &call : mock->deviceCalls) {
        EXPECT_FALSE(call.authorized);
        EXPECT_NE(call.devAddr, 4);
    }
}

/**
 * @tc.name: UsbDynamicControl_ModeExitEnablesAllNonHub
 * @tc.desc: exiting the mode enables every non-HUB device and skips HUB
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_ModeExitEnablesAllNonHub, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_ModeExitEnablesAllNonHub start");
    sptr<MockUsbDeviceInterface> mock = new (std::nothrow) MockUsbDeviceInterface();
    ASSERT_NE(mock, nullptr);
    usbHostManager_->usbDeviceInterface_ = mock;
    UsbDevice *device = CreateTestDevice(DYN_TEST_BUS_NUM_1, 2, ENABLED);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(usbHostManager_->AddDevice(device));
    usbHostManager_->InitDynamicControlEnabled(true);
    usbHostManager_->SetAllNonHubDevicesAuthorized(false);
    mock->deviceCalls.clear();

    usbHostManager_->InitDynamicControlEnabled(false);
    usbHostManager_->SetAllNonHubDevicesAuthorized(true);

    ASSERT_EQ(mock->deviceCalls.size(), 1);
    EXPECT_TRUE(mock->deviceCalls[0].authorized);
    EXPECT_EQ(mock->deviceCalls[0].devAddr, 2);
}

/**
 * @tc.name: UsbDynamicControl_NewNonHubDefaultDisabled
 * @tc.desc: a NEW_ARRIVED non-HUB device is disabled and query reports authorized=false
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_NewNonHubDefaultDisabled, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_NewNonHubDefaultDisabled start");
    sptr<MockUsbDeviceInterface> mock = new (std::nothrow) MockUsbDeviceInterface();
    ASSERT_NE(mock, nullptr);
    usbHostManager_->usbDeviceInterface_ = mock;
    usbHostManager_->InitDynamicControlEnabled(true);

    UsbDevice *device = CreateTestDevice(DYN_TEST_BUS_NUM_1, 2, NEW_ARRIVED);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(usbHostManager_->AddDevice(device));
    ASSERT_FALSE(mock->deviceCalls.empty());
    EXPECT_FALSE(mock->deviceCalls.back().authorized);

    std::vector<UsbDeviceInfo> infos;
    ASSERT_EQ(usbHostManager_->GetAllDevices(infos), UEC_OK);
    ASSERT_EQ(infos.size(), 1);
    EXPECT_FALSE(infos[0].authorized);
    EXPECT_FALSE(infos[0].isHub);
}

/**
 * @tc.name: UsbDynamicControl_NewHubNotDisabled
 * @tc.desc: a NEW_ARRIVED HUB is not disabled by the dynamic mode
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_NewHubNotDisabled, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_NewHubNotDisabled start");
    sptr<MockUsbDeviceInterface> mock = new (std::nothrow) MockUsbDeviceInterface();
    ASSERT_NE(mock, nullptr);
    usbHostManager_->usbDeviceInterface_ = mock;
    usbHostManager_->InitDynamicControlEnabled(true);

    UsbDevice *hub = CreateTestDevice(DYN_TEST_BUS_NUM_1, 2, NEW_ARRIVED);
    ASSERT_NE(hub, nullptr);
    hub->SetClass(BASE_CLASS_HUB);
    ASSERT_TRUE(usbHostManager_->AddDevice(hub));
    EXPECT_TRUE(mock->deviceCalls.empty());

    std::vector<UsbDeviceInfo> infos;
    ASSERT_EQ(usbHostManager_->GetAllDevices(infos), UEC_OK);
    ASSERT_EQ(infos.size(), 1);
    EXPECT_TRUE(infos[0].authorized);
    EXPECT_TRUE(infos[0].isHub);
}

/**
 * @tc.name: UsbDynamicControl_InterfaceBottomLayerError
 * @tc.desc: host manager returns the bottom-layer interface authorize error unchanged
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_InterfaceBottomLayerError, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_InterfaceBottomLayerError start");
    sptr<MockUsbDeviceInterface> mock = new (std::nothrow) MockUsbDeviceInterface();
    ASSERT_NE(mock, nullptr);
    usbHostManager_->usbDeviceInterface_ = mock;
    mock->interfaceRet = UEC_INTERFACE_INVALID_OPERATION;

    OHOS::HDI::Usb::V1_0::UsbDev dev = {DYN_TEST_BUS_NUM_1, 2};
    int32_t ret = usbHostManager_->UsbInterfaceAuthorize(dev, 0, 0, false);

    EXPECT_EQ(ret, UEC_INTERFACE_INVALID_OPERATION);
    ASSERT_EQ(mock->interfaceCalls.size(), 1);
    EXPECT_FALSE(mock->interfaceCalls[0].authorized);
}


/**
 * @tc.name: UsbDynamicControlService_QueryAndPermission
 * @tc.desc: GetAllDevices permission/host-null/success paths and active device query
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_QueryAndPermission, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_QueryAndPermission start");
    std::vector<UsbDeviceInfo> infos;
    UsbService::SetCallerValidForTest(0);
    EXPECT_EQ(service_->GetAllDevices(infos), UEC_SERVICE_INVALID_OPERATION);

    UsbService::SetCallerValidForTest(1);
    auto host = service_->usbHostManager_;
    service_->usbHostManager_ = nullptr;
    EXPECT_EQ(service_->GetAllDevices(infos), UEC_SERVICE_INVALID_VALUE);
    service_->usbHostManager_ = host;
    EXPECT_EQ(service_->GetAllDevices(infos), UEC_OK);
    EXPECT_TRUE(infos.empty());

    UsbDevice *device = CreateServiceTestDevice(1, 2);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(service_->usbHostManager_->AddDevice(device));
    EXPECT_TRUE(service_->usbHostManager_->hasActiveDevice());
    EXPECT_EQ(service_->GetAllDevices(infos), UEC_OK);
    ASSERT_EQ(infos.size(), 1);
    EXPECT_EQ(infos[0].deviceId, "1-2");
    EXPECT_TRUE(infos[0].authorized);
    EXPECT_FALSE(infos[0].isHub);
}

/**
 * @tc.name: UsbDynamicControlService_ParamChanged
 * @tc.desc: dynamic control parameter callback null/on/off/host-null branches
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_ParamChanged, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_ParamChanged start");
    service_->OnDynamicControlParamChanged(nullptr, nullptr);
    service_->OnDynamicControlParamChanged("persist.edm.usb_dynamic_control_enable", "1");
    EXPECT_TRUE(service_->usbHostManager_->IsDynamicControlEnabled());
    service_->OnDynamicControlParamChanged("persist.edm.usb_dynamic_control_enable", "0");
    EXPECT_FALSE(service_->usbHostManager_->IsDynamicControlEnabled());

    auto host = service_->usbHostManager_;
    service_->usbHostManager_ = nullptr;
    service_->OnDynamicControlParamChanged("persist.edm.usb_dynamic_control_enable", "1");
    service_->usbHostManager_ = host;
}

/**
 * @tc.name: UsbDynamicControlService_DeviceAuthorize
 * @tc.desc: device authorize permission/range/absent/root-hub/non-root-hub/success/failure/host-null
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_DeviceAuthorize, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_DeviceAuthorize start");
    UsbService::SetCallerValidForTest(0);
    EXPECT_EQ(service_->UsbDeviceAuthorize(1, 2, true), UEC_SERVICE_INVALID_OPERATION);
    UsbService::SetCallerValidForTest(1);
    EXPECT_EQ(service_->UsbDeviceAuthorize(0, 2, true), UEC_INTERFACE_INVALID_VALUE);
    EXPECT_EQ(service_->UsbDeviceAuthorize(1, 2, true), UEC_INTERFACE_NAME_NOT_FOUND);

    UsbDevice *device = CreateServiceTestDevice(1, 2);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(service_->usbHostManager_->AddDevice(device));
    EXPECT_EQ(service_->UsbDeviceAuthorize(1, 2, true), UEC_OK);
    EXPECT_EQ(service_->UsbDeviceAuthorize(1, 2, false), UEC_OK);

    UsbDevice *rootHub = CreateServiceTestDevice(1, 1, BASE_CLASS_HUB);
    ASSERT_NE(rootHub, nullptr);
    ASSERT_TRUE(service_->usbHostManager_->AddDevice(rootHub));
    EXPECT_EQ(service_->UsbDeviceAuthorize(1, 1, false), UEC_INTERFACE_INVALID_OPERATION);

    UsbDevice *nonRootHub = CreateServiceTestDevice(1, 3, BASE_CLASS_HUB);
    ASSERT_NE(nonRootHub, nullptr);
    ASSERT_TRUE(service_->usbHostManager_->AddDevice(nonRootHub));
    EXPECT_EQ(service_->UsbDeviceAuthorize(1, 3, false), UEC_OK);

    mock_->deviceRet = UEC_INTERFACE_INVALID_OPERATION;
    EXPECT_EQ(service_->UsbDeviceAuthorize(1, 2, true), UEC_INTERFACE_INVALID_OPERATION);

    auto host = service_->usbHostManager_;
    service_->usbHostManager_ = nullptr;
    EXPECT_EQ(service_->UsbDeviceAuthorize(1, 2, true), UEC_SERVICE_INVALID_VALUE);
    service_->usbHostManager_ = host;
}

/**
 * @tc.name: UsbDynamicControlService_InterfaceAuthorize
 * @tc.desc: interface authorize permission/range/absent/root-hub/success/failure/host-null
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_InterfaceAuthorize, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_InterfaceAuthorize start");
    UsbService::SetCallerValidForTest(0);
    EXPECT_EQ(service_->UsbInterfaceAuthorize(1, 2, 0, 0, true), UEC_SERVICE_INVALID_OPERATION);
    UsbService::SetCallerValidForTest(1);
    EXPECT_EQ(service_->UsbInterfaceAuthorize(1, 2, 0, 0, true), UEC_INTERFACE_NAME_NOT_FOUND);
    EXPECT_EQ(service_->UsbInterfaceAuthorize(0, 2, 0, 0, true), UEC_INTERFACE_INVALID_VALUE);

    UsbDevice *device = CreateServiceTestDevice(1, 2);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(service_->usbHostManager_->AddDevice(device));
    EXPECT_EQ(service_->UsbInterfaceAuthorize(1, 2, 0, 0, true), UEC_OK);
    mock_->interfaceRet = UEC_INTERFACE_INVALID_OPERATION;
    EXPECT_EQ(service_->UsbInterfaceAuthorize(1, 2, 0, 0, false), UEC_INTERFACE_INVALID_OPERATION);

    UsbDevice *rootHub = CreateServiceTestDevice(1, 1, BASE_CLASS_HUB);
    ASSERT_NE(rootHub, nullptr);
    ASSERT_TRUE(service_->usbHostManager_->AddDevice(rootHub));
    EXPECT_EQ(service_->UsbInterfaceAuthorize(1, 1, 0, 0, false), UEC_INTERFACE_INVALID_OPERATION);

    auto host = service_->usbHostManager_;
    service_->usbHostManager_ = nullptr;
    EXPECT_EQ(service_->UsbInterfaceAuthorize(1, 2, 0, 0, true), UEC_SERVICE_INVALID_VALUE);
    service_->usbHostManager_ = host;
}

/**
 * @tc.name: UsbDynamicControlService_Callbacks
 * @tc.desc: callback permission/null/duplicate/unregister/notify payload/error/death/destructor paths
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_Callbacks, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_Callbacks start");
    UsbService::SetCallerValidForTest(0);
    EXPECT_EQ(service_->RegDeviceControlCallback(nullptr), UEC_SERVICE_INVALID_OPERATION);
    UsbService::SetCallerValidForTest(1);
    EXPECT_EQ(service_->RegDeviceControlCallback(nullptr), UEC_INTERFACE_INVALID_VALUE);

    sptr<MockDeviceControlRemote> remote = new (std::nothrow) MockDeviceControlRemote();
    ASSERT_NE(remote, nullptr);
    EXPECT_EQ(service_->RegDeviceControlCallback(remote), UEC_OK);
    EXPECT_EQ(service_->RegDeviceControlCallback(remote), UEC_OK);

    UsbDevice *device = CreateServiceTestDevice(1, 2);
    ASSERT_NE(device, nullptr);
    device->SetAuthorizeStatus(DISABLED);
    service_->NotifyDeviceControlEvent(*device, true);
    EXPECT_EQ(remote->sendCount, 1);
    EXPECT_EQ(remote->lastCode, static_cast<uint32_t>(DEVICE_CONTROL_EVENT_CODE));
    EXPECT_EQ(remote->lastAuthorizeStatus, static_cast<int32_t>(DISABLED));
    EXPECT_TRUE(remote->lastAttached);
    delete device;

    EXPECT_EQ(service_->UnRegDeviceControlCallback(nullptr), UEC_INTERFACE_INVALID_VALUE);
    EXPECT_EQ(service_->UnRegDeviceControlCallback(remote), UEC_OK);
    UsbDevice *device2 = CreateServiceTestDevice(1, 4);
    ASSERT_NE(device2, nullptr);
    service_->NotifyDeviceControlEvent(*device2, false);
    EXPECT_EQ(remote->sendCount, 1);
    delete device2;
    EXPECT_EQ(service_->UnRegDeviceControlCallback(remote), UEC_OK);

    EXPECT_EQ(service_->RegDeviceControlCallback(remote), UEC_OK);
    remote->sendRet = UEC_INTERFACE_INVALID_OPERATION;
    UsbDevice *device3 = CreateServiceTestDevice(1, 5);
    ASSERT_NE(device3, nullptr);
    service_->NotifyDeviceControlEvent(*device3, true);
    EXPECT_EQ(remote->sendCount, 2);
    delete device3;

    sptr<UsbService::DeviceControlDeathRecipient> recipient =
        new (std::nothrow) UsbService::DeviceControlDeathRecipient(service_.GetRefPtr(), remote.GetRefPtr());
    ASSERT_NE(recipient, nullptr);
    wptr<IRemoteObject> remoteWptr = remote;
    recipient->OnRemoteDied(remoteWptr);
    sptr<UsbService::DeviceControlDeathRecipient> nullRecipient =
        new (std::nothrow) UsbService::DeviceControlDeathRecipient(nullptr, remote.GetRefPtr());
    ASSERT_NE(nullRecipient, nullptr);
    nullRecipient->OnRemoteDied(remoteWptr);
    EXPECT_EQ(service_->UnRegDeviceControlCallback(remote), UEC_OK);
}

/**
 * @tc.name: UsbDynamicControlService_EdmModeOverride
 * @tc.desc: enterprise device management mode can be forced on/off for unit tests
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_EdmModeOverride, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_EdmModeOverride start");
    UsbHostManager::SetEdmEnabledForTest(1);
    EXPECT_TRUE(service_->usbHostManager_->IsEdmEnabled());
    UsbHostManager::SetEdmEnabledForTest(0);
    EXPECT_FALSE(service_->usbHostManager_->IsEdmEnabled());
    UsbHostManager::SetEdmEnabledForTest(-1);
    (void)service_->usbHostManager_->IsEdmEnabled();
}

/**
 * @tc.name: UsbDynamicControl_DeviceTypeMatrix
 * @tc.desc: mode enter skips HUB and disables storage/HID/serial/other classes
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_DeviceTypeMatrix, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_DeviceTypeMatrix start");
    sptr<MockUsbDeviceInterface> mock = new (std::nothrow) MockUsbDeviceInterface();
    ASSERT_NE(mock, nullptr);
    usbHostManager_->usbDeviceInterface_ = mock;
    UsbHostManager::SetEdmEnabledForTest(0);
    const int32_t classes[] = {0x08, 0x03, 0x09, 0x00, 0xEF};
    constexpr uint8_t hubDevAddr = 4;
    for (uint8_t index = 0; index < sizeof(classes) / sizeof(classes[0]); ++index) {
        UsbDevice *device = CreateTestDevice(DYN_TEST_BUS_NUM_1, static_cast<uint8_t>(index + 2), ENABLED);
        ASSERT_NE(device, nullptr);
        device->SetClass(classes[index]);
        ASSERT_TRUE(usbHostManager_->AddDevice(device));
    }
    mock->deviceCalls.clear();
    usbHostManager_->InitDynamicControlEnabled(true);
    usbHostManager_->SetAllNonHubDevicesAuthorized(false);
    EXPECT_EQ(mock->deviceCalls.size(), 4);
    for (const auto &call : mock->deviceCalls) {
        EXPECT_NE(call.devAddr, hubDevAddr);
        EXPECT_FALSE(call.authorized);
    }
    UsbHostManager::SetEdmEnabledForTest(-1);
}

/**
 * @tc.name: UsbDynamicControl_RealUsbSmoke
 * @tc.desc: optional real-USB smoke test; skipped unless RM006_REAL_USB_TEST=1
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_RealUsbSmoke, TestSize.Level1)
{
    const char *enabled = getenv("RM006_REAL_USB_TEST");
    if (enabled == nullptr || std::string(enabled) != "1") {
        GTEST_SKIP() << "set RM006_REAL_USB_TEST=1 and connect USB hardware to run";
    }
    std::vector<UsbDevice> devices;
    EXPECT_EQ(usbHostManager_->GetDevices(devices), UEC_OK);
}

/**
 * @tc.name: UsbDynamicControl_InitAndSkipBranches
 * @tc.desc: init path, empty active device, disable/enable skip branches
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_InitAndSkipBranches, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_InitAndSkipBranches start");
    sptr<MockUsbDeviceInterface> mock = new (std::nothrow) MockUsbDeviceInterface();
    ASSERT_NE(mock, nullptr);
    usbHostManager_->usbDeviceInterface_ = mock;
    UsbHostManager::SetEdmEnabledForTest(0);
    EXPECT_FALSE(usbHostManager_->hasActiveDevice());

    UsbDevice *enabledDevice = CreateTestDevice(DYN_TEST_BUS_NUM_1, 2, ENABLED);
    UsbDevice *disabledDevice = CreateTestDevice(DYN_TEST_BUS_NUM_1, 3, ENABLED);
    ASSERT_NE(enabledDevice, nullptr);
    ASSERT_NE(disabledDevice, nullptr);
    ASSERT_TRUE(usbHostManager_->AddDevice(enabledDevice));
    ASSERT_TRUE(usbHostManager_->AddDevice(disabledDevice));
    disabledDevice->SetAuthorizeStatus(DISABLED);
    EXPECT_TRUE(usbHostManager_->hasActiveDevice());

    mock->deviceCalls.clear();
    usbHostManager_->InitDynamicControlEnabled(true);
    EXPECT_TRUE(usbHostManager_->IsDynamicControlEnabled());
    // setting the flag alone does not touch existing devices
    EXPECT_TRUE(mock->deviceCalls.empty());
    usbHostManager_->SetAllNonHubDevicesAuthorized(false);
    EXPECT_EQ(mock->deviceCalls.size(), 1);
    usbHostManager_->InitDynamicControlEnabled(false);
    EXPECT_FALSE(usbHostManager_->IsDynamicControlEnabled());

    enabledDevice->SetAuthorizeStatus(DISABLED);
    disabledDevice->SetAuthorizeStatus(ENABLED);
    mock->deviceCalls.clear();
    usbHostManager_->SetAllNonHubDevicesAuthorized(false);
    EXPECT_EQ(mock->deviceCalls.size(), 1);

    enabledDevice->SetAuthorizeStatus(DISABLED);
    disabledDevice->SetAuthorizeStatus(ENABLED);
    mock->deviceCalls.clear();
    usbHostManager_->SetAllNonHubDevicesAuthorized(true);
    EXPECT_EQ(mock->deviceCalls.size(), 1);
    UsbHostManager::SetEdmEnabledForTest(-1);
}

/**
 * @tc.name: UsbDynamicControlService_DestructorCleanup
 * @tc.desc: service destructor clears an active callback subscription
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_DestructorCleanup, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_DestructorCleanup start");
    sptr<MockDeviceControlRemote> remote = new (std::nothrow) MockDeviceControlRemote();
    ASSERT_NE(remote, nullptr);
    EXPECT_EQ(service_->RegDeviceControlCallback(remote), UEC_OK);
    service_ = nullptr;
    SUCCEED();
}

/**
 * @tc.name: UsbDynamicControlService_UnRegPermission
 * @tc.desc: unregister permission failure branch
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_UnRegPermission, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_UnRegPermission start");
    UsbService::SetCallerValidForTest(0);
    EXPECT_EQ(service_->UnRegDeviceControlCallback(nullptr), UEC_SERVICE_INVALID_OPERATION);
}

/**
 * @tc.name: UsbDynamicControl_QueryCompositeAndHub
 * @tc.desc: composite interface info, HUB flag, and removed-device filtering
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_QueryCompositeAndHub, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_QueryCompositeAndHub start");
    UsbDevice *composite = CreateCompositeTestDevice(DYN_TEST_BUS_NUM_1, 2, false);
    UsbDevice *hub = CreateTestDevice(DYN_TEST_BUS_NUM_1, 3, ENABLED);
    ASSERT_NE(composite, nullptr);
    ASSERT_NE(hub, nullptr);
    hub->SetClass(BASE_CLASS_HUB);
    ASSERT_TRUE(usbHostManager_->AddDevice(composite));
    ASSERT_TRUE(usbHostManager_->AddDevice(hub));

    std::vector<UsbDeviceInfo> infos;
    ASSERT_EQ(usbHostManager_->GetAllDevices(infos), UEC_OK);
    ASSERT_EQ(infos.size(), 2);
    for (const auto &info : infos) {
        if (info.devAddr == 2) {
            EXPECT_FALSE(info.isHub);
            ASSERT_EQ(info.interfaces.size(), 1);
            EXPECT_EQ(info.interfaces[0].interfaceClass, 0x08);
            EXPECT_FALSE(info.interfaces[0].authorized);
        }
        if (info.devAddr == 3) {
            EXPECT_TRUE(info.isHub);
        }
    }
    ASSERT_TRUE(usbHostManager_->DelDevice(DYN_TEST_BUS_NUM_1, 2));
    infos.clear();
    ASSERT_EQ(usbHostManager_->GetAllDevices(infos), UEC_OK);
    ASSERT_EQ(infos.size(), 1);
    EXPECT_EQ(infos[0].devAddr, 3);
}

/**
 * @tc.name: UsbDynamicControl_ModeExitIdempotentAndEdmDisabled
 * @tc.desc: exit is idempotent and enables a device that was disabled before mode entry
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_ModeExitIdempotentAndEdmDisabled, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_ModeExitIdempotentAndEdmDisabled start");
    sptr<MockUsbDeviceInterface> mock = new (std::nothrow) MockUsbDeviceInterface();
    ASSERT_NE(mock, nullptr);
    usbHostManager_->usbDeviceInterface_ = mock;
    UsbHostManager::SetEdmEnabledForTest(0);
    UsbDevice *device = CreateTestDevice(DYN_TEST_BUS_NUM_1, 2, ENABLED);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(usbHostManager_->AddDevice(device));
    device->SetAuthorizeStatus(DISABLED);
    mock->deviceCalls.clear();

    usbHostManager_->InitDynamicControlEnabled(true);
    usbHostManager_->SetAllNonHubDevicesAuthorized(false);
    EXPECT_TRUE(mock->deviceCalls.empty());
    usbHostManager_->InitDynamicControlEnabled(false);
    usbHostManager_->SetAllNonHubDevicesAuthorized(true);
    ASSERT_EQ(mock->deviceCalls.size(), 1);
    EXPECT_TRUE(mock->deviceCalls[0].authorized);
    mock->deviceCalls.clear();
    usbHostManager_->SetAllNonHubDevicesAuthorized(true);
    EXPECT_TRUE(mock->deviceCalls.empty());
    UsbHostManager::SetEdmEnabledForTest(-1);
}

/**
 * @tc.name: UsbDynamicControlService_AuthorizeThenQuery
 * @tc.desc: service authorize result is reflected by GetAllDevices (EDM replay path)
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_AuthorizeThenQuery, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_AuthorizeThenQuery start");
    UsbDevice *device = CreateServiceTestDevice(1, 2);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(service_->usbHostManager_->AddDevice(device));
    service_->usbHostManager_->InitDynamicControlEnabled(true);
    EXPECT_EQ(service_->UsbDeviceAuthorize(1, 2, true), UEC_OK);

    std::vector<UsbDeviceInfo> infos;
    ASSERT_EQ(service_->GetAllDevices(infos), UEC_OK);
    ASSERT_EQ(infos.size(), 1);
    EXPECT_TRUE(infos[0].authorized);
}

/**
 * @tc.name: UsbDynamicControlService_ClientPath
 * @tc.desc: UsbSrvClient GetAllDevices/device/interface authorize wrappers
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_ClientPath, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_ClientPath start");
    sptr<MockUsbServerRemote> remote = new (std::nothrow) MockUsbServerRemote();
    ASSERT_NE(remote, nullptr);
    UsbDeviceInfo info;
    info.deviceId = "1-2";
    info.busNum = 1;
    info.devAddr = 2;
    info.vendorId = 0x1234;
    info.productId = 0x5678;
    info.deviceClass = 0x08;
    info.isHub = false;
    info.authorized = false;
    remote->deviceInfos.push_back(info);

    UsbSrvClient &client = UsbSrvClient::GetInstance();
    client.proxy_ = new UsbServerProxy(remote);
    ASSERT_NE(client.proxy_, nullptr);

    std::vector<UsbDeviceInfo> infos;
    EXPECT_EQ(client.GetAllDevices(infos), UEC_OK);
    ASSERT_EQ(infos.size(), 1);
    EXPECT_EQ(infos[0].deviceId, "1-2");
    EXPECT_FALSE(infos[0].authorized);

    EXPECT_EQ(client.UsbDeviceAuthorize(1, 2, false), UEC_OK);
    EXPECT_EQ(client.UsbInterfaceAuthorize(1, 2, 0, 0, true), UEC_OK);
    remote->authorizeRet = UEC_INTERFACE_INVALID_OPERATION;
    EXPECT_EQ(client.UsbDeviceAuthorize(1, 2, false), UEC_INTERFACE_INVALID_OPERATION);
    client.proxy_ = nullptr;
}

/**
 * @tc.name: UsbDynamicControl_ModeOffNoDisable
 * @tc.desc: mode off does not disable an attached device through the dynamic branch
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlTest, UsbDynamicControl_ModeOffNoDisable, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbDynamicControl_ModeOffNoDisable start");
    sptr<MockUsbDeviceInterface> mock = new (std::nothrow) MockUsbDeviceInterface();
    ASSERT_NE(mock, nullptr);
    usbHostManager_->usbDeviceInterface_ = mock;
    UsbHostManager::SetEdmEnabledForTest(0);
    UsbDevice *device = CreateTestDevice(DYN_TEST_BUS_NUM_1, 2, ENABLED);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(usbHostManager_->AddDevice(device));
    mock->deviceCalls.clear();
    usbHostManager_->ExecuteStrategy();
    EXPECT_TRUE(mock->deviceCalls.empty());
    UsbHostManager::SetEdmEnabledForTest(-1);
}

/**
 * @tc.name: UsbDynamicControlService_AuthorizeIdempotent
 * @tc.desc: repeated device authorize with the same state does not issue a second HDI call
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_AuthorizeIdempotent, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_AuthorizeIdempotent start");
    UsbDevice *device = CreateServiceTestDevice(1, 2);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(service_->usbHostManager_->AddDevice(device));
    EXPECT_EQ(service_->UsbDeviceAuthorize(1, 2, false), UEC_OK);
    mock_->deviceCalls.clear();
    EXPECT_EQ(service_->UsbDeviceAuthorize(1, 2, false), UEC_OK);
    EXPECT_TRUE(mock_->deviceCalls.empty());
}

/**
 * @tc.name: UsbDynamicControlService_InterfaceInvalidPassthrough
 * @tc.desc: invalid interface arguments are passed to the bottom layer, not pre-rejected
 * @tc.type: FUNC
 */
HWTEST_F(UsbDynamicControlServiceTest, UsbDynamicControlService_InterfaceInvalidPassthrough, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDynamicControlService_InterfaceInvalidPassthrough start");
    UsbDevice *device = CreateServiceTestDevice(1, 2);
    ASSERT_NE(device, nullptr);
    ASSERT_TRUE(service_->usbHostManager_->AddDevice(device));
    mock_->interfaceRet = UEC_INTERFACE_INVALID_VALUE;
    EXPECT_EQ(service_->UsbInterfaceAuthorize(1, 2, 0xFF, 0xFF, true), UEC_INTERFACE_INVALID_VALUE);
    ASSERT_EQ(mock_->interfaceCalls.size(), 1);
    EXPECT_EQ(mock_->interfaceCalls[0].configId, 0xFF);
    EXPECT_EQ(mock_->interfaceCalls[0].interfaceId, 0xFF);
}

} // namespace USB
} // namespace OHOS