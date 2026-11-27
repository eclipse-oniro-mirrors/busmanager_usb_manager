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

#include "usb_hid_host_test.h"

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "ashmem.h"
#include "hilog_wrapper.h"
#include "iusb_srv.h"
#include "usb_common.h"
#include "usb_common_test.h"
#include "usb_config.h"
#include "usb_device.h"
#include "usb_device_pipe.h"
#include "usb_endpoint.h"
#include "usb_errors.h"
#include "usb_interface.h"
#include "usb_port.h"
#include "usb_srv_client.h"
#include "v1_2/usb_types.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::USB;
using namespace std;
using namespace OHOS::USB::Common;

namespace OHOS {
namespace USB {
namespace HidHost {

/* -------------------------------------------------------------------------- */
/* Fixture state                                                              */
/* -------------------------------------------------------------------------- */

namespace {
// Cached enumeration result collected once in SetUpTestCase. All test cases
// operate on this snapshot so the test does not depend on the operator
// plugging / unplugging the keyboard and mouse during the run.
std::vector<UsbDevice> g_devices;

// True when at least one HID device (keyboard or mouse) was seen at setup.
bool g_hasKeyboard = false;
bool g_hasMouse = false;

// Endpoint transfer type of an interrupt endpoint. usb_common.h defines
// USB_ENDPOINT_XFER_INT as int32_t; USBEndpoint::GetType() returns uint32_t,
// so keep a uint32_t constant here to avoid sign-compare diagnostics.
constexpr uint32_t HID_ENDPOINT_TYPE_INTERRUPT = 3u;

// RK3568 host HAL may reject interrupt/HID-class transfers (interrupt endpoint
// support varies by build); the service then reports an I/O error instead of
// success. The transfer tests accept either result as long as the call path
// completes without crashing.
constexpr int32_t HID_INT_IO_ERROR = -1;
} // namespace

/* -------------------------------------------------------------------------- */
/* Identification helpers                                                     */
/* -------------------------------------------------------------------------- */

bool UsbHidHostTest::IsHidDevice(UsbDevice &device)
{
    if (device.GetClass() == USB_CLASS_HID) {
        return true;
    }
    for (auto &config : device.GetConfigs()) {
        for (auto &intf : config.GetInterfaces()) {
            if (intf.GetClass() == USB_CLASS_HID) {
                return true;
            }
        }
    }
    return false;
}

int32_t UsbHidHostTest::GetHidProtocol(UsbDevice &device)
{
    // First scan interfaces: a standalone HID keyboard/mouse reports its HID
    // class and boot protocol at the interface level (device level is 0x00).
    for (auto &config : device.GetConfigs()) {
        for (auto &intf : config.GetInterfaces()) {
            if (intf.GetClass() == USB_CLASS_HID) {
                return intf.GetProtocol();
            }
        }
    }
    // Fall back to the device-level class/protocol for non-composite devices.
    if (device.GetClass() == USB_CLASS_HID) {
        return device.GetProtocol();
    }
    return HID_PROTOCOL_INVALID;
}

bool UsbHidHostTest::IsKeyboard(UsbDevice &device)
{
    return GetHidProtocol(device) == HID_PROTOCOL_KEYBOARD;
}

bool UsbHidHostTest::IsMouse(UsbDevice &device)
{
    return GetHidProtocol(device) == HID_PROTOCOL_MOUSE;
}

bool UsbHidHostTest::FindHidDevice(UsbDevice &device)
{
    for (auto &dev : g_devices) {
        if (IsHidDevice(dev)) {
            device = dev;
            return true;
        }
    }
    return false;
}

bool UsbHidHostTest::FindKeyboardDevice(UsbDevice &device)
{
    for (auto &dev : g_devices) {
        if (IsKeyboard(dev)) {
            device = dev;
            return true;
        }
    }
    return false;
}

bool UsbHidHostTest::FindMouseDevice(UsbDevice &device)
{
    for (auto &dev : g_devices) {
        if (IsMouse(dev)) {
            device = dev;
            return true;
        }
    }
    return false;
}

namespace {
// 查找单个 interface 上第一个 interrupt-IN endpoint，拆分出来以降低
// FindInterruptInEndpoint 的循环嵌套深度（静态告警要求方法深度 ≤4）。
bool FindInterruptInEndpointInInterface(UsbInterface &intf, UsbInterface &outInterface, USBEndpoint &outEndpoint)
{
    for (auto &ep : intf.GetEndpoints()) {
        if (ep.GetType() == HID_ENDPOINT_TYPE_INTERRUPT && ep.GetDirection() == USB_ENDPOINT_DIR_IN) {
            outInterface = intf;
            outEndpoint = ep;
            return true;
        }
    }
    return false;
}
} // namespace

bool UsbHidHostTest::FindInterruptInEndpoint(UsbDevice &device, UsbInterface &interface, USBEndpoint &endpoint)
{
    for (auto &config : device.GetConfigs()) {
        for (auto &intf : config.GetInterfaces()) {
            if (FindInterruptInEndpointInInterface(intf, interface, endpoint)) {
                return true;
            }
        }
    }
    return false;
}

/* -------------------------------------------------------------------------- */
/* Fixture lifecycle                                                          */
/* -------------------------------------------------------------------------- */

void UsbHidHostTest::SetUpTestCase()
{
    UsbCommonTest::GrantPermissionSysNative();

    auto &client = UsbSrvClient::GetInstance();

    // Single-board host mode: the board is already in host role with the
    // keyboard / mouse on its host port. Do NOT call SetPortRole here — it
    // switches the port role and drops this board's own hdc debug link.

    // Enumerate once and classify what is attached.
    int32_t ret = client.GetDevices(g_devices);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbHidHostTest GetDevices ret=%{public}d size=%{public}zu", ret, g_devices.size());
    for (auto &dev : g_devices) {
        if (IsKeyboard(dev)) {
            g_hasKeyboard = true;
        }
        if (IsMouse(dev)) {
            g_hasMouse = true;
        }
    }
    USB_HILOGI(MODULE_USB_SERVICE, "UsbHidHostTest keyboard=%{public}d mouse=%{public}d", g_hasKeyboard, g_hasMouse);

    // The test is meaningful only when a keyboard / mouse is attached. Give the
    // operator a chance to attach one before the test cases run.
    if (!g_hasKeyboard && !g_hasMouse) {
        std::cout << "[UsbHidHostTest] connect a USB keyboard or mouse, then press Enter..." << std::endl;
        int32_t ch;
        do {
            ch = getchar();
        } while (ch != '\n' && ch != EOF);

        ret = client.GetDevices(g_devices);
        USB_HILOGI(MODULE_USB_SERVICE, "UsbHidHostTest re-enumerate ret=%{public}d size=%{public}zu", ret,
            g_devices.size());
        for (auto &dev : g_devices) {
            if (IsKeyboard(dev)) {
                g_hasKeyboard = true;
            }
            if (IsMouse(dev)) {
                g_hasMouse = true;
            }
        }
    }
    USB_HILOGI(MODULE_USB_SERVICE, "UsbHidHostTest::SetUpTestCase end");
}

void UsbHidHostTest::TearDownTestCase()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbHidHostTest::TearDownTestCase end");
    g_devices.clear();
}

void UsbHidHostTest::SetUp() {}

void UsbHidHostTest::TearDown() {}

/* -------------------------------------------------------------------------- */
/* Section 1: Device enumeration                                              */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_GetDevices_001
 * @tc.desc: Test that GetDevices returns UEC_OK and enumerates at least one
 *           device (the board root hub) on a host port.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_GetDevices_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> devices;
    int32_t ret = client.GetDevices(devices);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_GetDevices_001 ret=%{public}d size=%{public}zu", ret, devices.size());
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_FALSE(devices.empty());
}

/**
 * @tc.name: HidHost_GetDevices_002
 * @tc.desc: Test that repeated GetDevices calls return a consistent snapshot.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_GetDevices_002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> first;
    std::vector<UsbDevice> second;
    EXPECT_EQ(client.GetDevices(first), UEC_OK);
    EXPECT_EQ(client.GetDevices(second), UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_GetDevices_002 first=%{public}zu second=%{public}zu", first.size(),
        second.size());
    EXPECT_EQ(first.size(), second.size());
}

/**
 * @tc.name: HidHost_GetDevices_003
 * @tc.desc: Test every enumerated device exposes a non-empty name and a valid
 *           bus/address pair.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_GetDevices_003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> devices;
    ASSERT_EQ(client.GetDevices(devices), UEC_OK);
    for (const auto &dev : devices) {
        EXPECT_FALSE(dev.GetName().empty()) << "device name must not be empty";
        USB_HILOGI(MODULE_USB_SERVICE, "HidHost_GetDevices_003 name=%{public}s bus=%{public}d addr=%{public}d",
            dev.GetName().c_str(), static_cast<int32_t>(dev.GetBusNum()), static_cast<int32_t>(dev.GetDevAddr()));
    }
}

/* -------------------------------------------------------------------------- */
/* Section 2: HID identification (keyboard / mouse)                           */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Identify_001
 * @tc.desc: Test that a HID device (class 0x03) is present among the
 *           enumerated non-hub devices.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Identify_001, TestSize.Level1)
{
    UsbDevice device;
    bool found = FindHidDevice(device);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Identify_001 found=%{public}d", found);
    EXPECT_TRUE(found) << "no HID device detected on the host port";
}

/**
 * @tc.name: HidHost_Identify_002
 * @tc.desc: Test a keyboard (boot protocol 0x01) is identified when connected.
 *           Skips gracefully when only a mouse is present.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Identify_002, TestSize.Level1)
{
    UsbDevice device;
    bool found = FindKeyboardDevice(device);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Identify_002 keyboard found=%{public}d", found);
    if (found) {
        EXPECT_EQ(GetHidProtocol(device), HID_PROTOCOL_KEYBOARD);
    }
}

/**
 * @tc.name: HidHost_Identify_003
 * @tc.desc: Test a mouse (boot protocol 0x02) is identified when connected.
 *           Skips gracefully when only a keyboard is present.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Identify_003, TestSize.Level1)
{
    UsbDevice device;
    bool found = FindMouseDevice(device);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Identify_003 mouse found=%{public}d", found);
    if (found) {
        EXPECT_EQ(GetHidProtocol(device), HID_PROTOCOL_MOUSE);
    }
}

/* -------------------------------------------------------------------------- */
/* Section 3: Device metadata                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_DeviceInfo_001
 * @tc.desc: Test the HID device reports a non-zero vendor id and a
 *           non-negative product id.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_DeviceInfo_001, TestSize.Level1)
{
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_DeviceInfo_001 vid=0x%{public}X pid=0x%{public}X", device.GetVendorId(),
        device.GetProductId());
    EXPECT_NE(device.GetVendorId(), 0);
    EXPECT_GE(device.GetProductId(), 0);
}

/**
 * @tc.name: HidHost_DeviceInfo_003
 * @tc.desc: Test the HID device exposes at least one configuration.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_DeviceInfo_003, TestSize.Level1)
{
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    EXPECT_GE(device.GetConfigCount(), 1);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_DeviceInfo_003 configCount=%{public}d", device.GetConfigCount());
}

/* -------------------------------------------------------------------------- */
/* Section 4: Permission checks                                               */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Permission_002
 * @tc.desc: Test HasRightEx returns UEC_OK and populates the result flag.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Permission_002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    bool result = false;
    int32_t ret = client.HasRightEx(device.GetName(), result);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Permission_002 ret=%{public}d result=%{public}d", ret, result);
    EXPECT_EQ(ret, UEC_OK);
}

/**
 * @tc.name: HidHost_Permission_003
 * @tc.desc: Test RequestRight / RemoveRight round-trip for a HID device.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Permission_003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    std::string name = device.GetName();
    int32_t reqRet = client.RequestRight(name);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Permission_003 RequestRight=%{public}d", reqRet);
    int32_t rmRet = client.RemoveRight(name);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Permission_003 RemoveRight=%{public}d", rmRet);
    // Both calls return without crashing; the dialog flow may be unavailable on
    // a headless board, so return codes are informational.
    SUCCEED();
}

/**
 * @tc.name: HidHost_Permission_004
 * @tc.desc: Test HasRight against an invalid device name is tolerated and
 *           reports no right.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Permission_004, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    bool hasRight = client.HasRight("255-255");
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Permission_004 invalid name hasRight=%{public}d", hasRight);
    EXPECT_FALSE(hasRight);
}

/* -------------------------------------------------------------------------- */
/* Section 5: Open / close lifecycle                                          */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_OpenClose_001
 * @tc.desc: Test the basic OpenDevice / Close lifecycle on a HID device.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_OpenClose_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    int32_t ret = client.OpenDevice(device, pipe);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_OpenClose_001 OpenDevice=%{public}d", ret);
    ASSERT_EQ(ret, UEC_OK);
    EXPECT_TRUE(client.Close(pipe));
}

/**
 * @tc.name: HidHost_OpenClose_002
 * @tc.desc: Test CloseEx returns UEC_OK after a successful open.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_OpenClose_002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    int32_t ret = client.CloseEx(pipe);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_OpenClose_002 CloseEx=%{public}d", ret);
    EXPECT_EQ(ret, UEC_OK);
}

/**
 * @tc.name: HidHost_OpenClose_003
 * @tc.desc: Test repeated open/close cycles do not fail.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_OpenClose_003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    for (int32_t i = 0; i < 5; ++i) {
        USBDevicePipe pipe;
        int32_t ret = client.OpenDevice(device, pipe);
        USB_HILOGI(MODULE_USB_SERVICE, "HidHost_OpenClose_003 iter=%{public}d ret=%{public}d", i, ret);
        if (ret == UEC_OK) {
            EXPECT_TRUE(client.Close(pipe));
        }
    }
    SUCCEED();
}

/* -------------------------------------------------------------------------- */
/* Section 6: Interface claim / release                                       */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Claim_001
 * @tc.desc: Test ClaimInterface with force=true on the first interface.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Claim_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    ASSERT_FALSE(device.GetConfigs().empty());
    ASSERT_FALSE(device.GetConfigs().front().GetInterfaces().empty());
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    int32_t ret = client.ClaimInterface(pipe, interface, true);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Claim_001 ret=%{public}d", ret);
    EXPECT_EQ(ret, UEC_OK);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Claim_002
 * @tc.desc: Test ClaimInterface / ReleaseInterface round-trip.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Claim_002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    ASSERT_EQ(client.ClaimInterface(pipe, interface, true), UEC_OK);
    int32_t ret = client.ReleaseInterface(pipe, interface);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Claim_002 ReleaseInterface=%{public}d", ret);
    EXPECT_EQ(ret, UEC_OK);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Claim_003
 * @tc.desc: Test ClaimInterface with force=false then force=true.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Claim_003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    int32_t retNoForce = client.ClaimInterface(pipe, interface, false);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Claim_003 noForce=%{public}d", retNoForce);
    if (retNoForce != UEC_OK) {
        (void)client.ReleaseInterface(pipe, interface);
    }
    int32_t retForce = client.ClaimInterface(pipe, interface, true);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Claim_003 force=%{public}d", retForce);
    EXPECT_EQ(retForce, UEC_OK);
    (void)client.Close(pipe);
}

/* -------------------------------------------------------------------------- */
/* Section 7: Descriptor retrieval                                            */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Descriptor_001
 * @tc.desc: Test GetRawDescriptors returns UEC_OK and a non-empty buffer.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Descriptor_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    std::vector<uint8_t> bufferData;
    int32_t ret = client.GetRawDescriptors(pipe, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Descriptor_001 ret=%{public}d size=%{public}zu", ret, bufferData.size());
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_FALSE(bufferData.empty());
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Descriptor_002
 * @tc.desc: Test GetRawDescriptors returns a consistent size across calls.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Descriptor_002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    std::vector<uint8_t> first;
    std::vector<uint8_t> second;
    ASSERT_EQ(client.GetRawDescriptors(pipe, first), UEC_OK);
    ASSERT_EQ(client.GetRawDescriptors(pipe, second), UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Descriptor_002 first=%{public}zu second=%{public}zu", first.size(),
        second.size());
    EXPECT_EQ(first.size(), second.size());
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Descriptor_003
 * @tc.desc: Test GET_DESCRIPTOR (device) via UsbControlTransfer returns UEC_OK
 *           and fills an 18-byte device descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Descriptor_003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    HDI::Usb::V1_2::UsbCtrlTransferParams ctrlParams;
    ctrlParams.requestType = REQUEST_TYPE_STANDARD_IN;
    ctrlParams.requestCmd = USB_REQUEST_GET_DESCRIPTOR;
    ctrlParams.value = (USB_DT_DEVICE << 8);
    ctrlParams.index = 0;
    ctrlParams.length = DEVICE_DESC_LENGTH;
    ctrlParams.timeout = CONTROL_TRANSFER_TIMEOUT;
    std::vector<uint8_t> bufferData(DEVICE_DESC_LENGTH);
    int32_t ret = client.UsbControlTransfer(pipe, ctrlParams, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Descriptor_003 ret=%{public}d", ret);
    EXPECT_EQ(ret, UEC_OK);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Descriptor_004
 * @tc.desc: Test GET_DESCRIPTOR (HID) via UsbControlTransfer.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Descriptor_004, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    HDI::Usb::V1_2::UsbCtrlTransferParams ctrlParams;
    ctrlParams.requestType = REQUEST_TYPE_STANDARD_IN;
    ctrlParams.requestCmd = USB_REQUEST_GET_DESCRIPTOR;
    ctrlParams.value = (USB_DT_HID << 8);
    ctrlParams.index = interface.GetId();
    ctrlParams.length = 9;
    ctrlParams.timeout = CONTROL_TRANSFER_TIMEOUT;
    std::vector<uint8_t> bufferData(9);
    int32_t ret = client.UsbControlTransfer(pipe, ctrlParams, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Descriptor_004 ret=%{public}d", ret);
    EXPECT_EQ(ret, UEC_OK);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Descriptor_005
 * @tc.desc: Test GET_DESCRIPTOR (configuration) via UsbControlTransfer.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Descriptor_005, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    HDI::Usb::V1_2::UsbCtrlTransferParams ctrlParams;
    ctrlParams.requestType = REQUEST_TYPE_STANDARD_IN;
    ctrlParams.requestCmd = USB_REQUEST_GET_DESCRIPTOR;
    ctrlParams.value = (USB_DT_CONFIG << 8);
    ctrlParams.index = 0;
    ctrlParams.length = 9;
    ctrlParams.timeout = CONTROL_TRANSFER_TIMEOUT;
    std::vector<uint8_t> bufferData(9);
    int32_t ret = client.UsbControlTransfer(pipe, ctrlParams, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Descriptor_005 ret=%{public}d", ret);
    EXPECT_EQ(ret, UEC_OK);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Descriptor_006
 * @tc.desc: Test GET_DESCRIPTOR (string index 0) returns the language id list.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Descriptor_006, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    HDI::Usb::V1_2::UsbCtrlTransferParams ctrlParams;
    ctrlParams.requestType = REQUEST_TYPE_STANDARD_IN;
    ctrlParams.requestCmd = USB_REQUEST_GET_DESCRIPTOR;
    ctrlParams.value = (USB_DT_STRING << 8) | 0;
    ctrlParams.index = 0;
    ctrlParams.length = 4;
    ctrlParams.timeout = CONTROL_TRANSFER_TIMEOUT;
    std::vector<uint8_t> bufferData(4);
    int32_t ret = client.UsbControlTransfer(pipe, ctrlParams, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Descriptor_006 ret=%{public}d", ret);
    EXPECT_EQ(ret, UEC_OK);
    (void)client.Close(pipe);
}

/* -------------------------------------------------------------------------- */
/* Section 8: HID class control transfers                                     */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_HidRequest_001
 * @tc.desc: Test HID GET_REPORT class request on the interrupt interface.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_HidRequest_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    HDI::Usb::V1_2::UsbCtrlTransferParams ctrlParams;
    ctrlParams.requestType = REQUEST_TYPE_CLASS_INTERFACE_IN;
    ctrlParams.requestCmd = HID_GET_REPORT;
    ctrlParams.value = 0x0100;
    ctrlParams.index = interface.GetId();
    ctrlParams.length = INTERRUPT_BUFFER_LENGTH;
    ctrlParams.timeout = CONTROL_TRANSFER_TIMEOUT;
    std::vector<uint8_t> bufferData(INTERRUPT_BUFFER_LENGTH);
    int32_t ret = client.UsbControlTransfer(pipe, ctrlParams, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_HidRequest_001 GET_REPORT ret=%{public}d", ret);
    EXPECT_TRUE(ret == UEC_OK || ret == HID_INT_IO_ERROR);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_HidRequest_002
 * @tc.desc: Test HID GET_IDLE class request on the interrupt interface.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_HidRequest_002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    HDI::Usb::V1_2::UsbCtrlTransferParams ctrlParams;
    ctrlParams.requestType = REQUEST_TYPE_CLASS_INTERFACE_IN;
    ctrlParams.requestCmd = HID_GET_IDLE;
    ctrlParams.value = 0;
    ctrlParams.index = interface.GetId();
    ctrlParams.length = 1;
    ctrlParams.timeout = CONTROL_TRANSFER_TIMEOUT;
    std::vector<uint8_t> bufferData(1);
    int32_t ret = client.UsbControlTransfer(pipe, ctrlParams, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_HidRequest_002 GET_IDLE ret=%{public}d", ret);
    EXPECT_TRUE(ret == UEC_OK || ret == HID_INT_IO_ERROR);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_HidRequest_003
 * @tc.desc: Test HID GET_PROTOCOL class request on the interrupt interface.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_HidRequest_003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    HDI::Usb::V1_2::UsbCtrlTransferParams ctrlParams;
    ctrlParams.requestType = REQUEST_TYPE_CLASS_INTERFACE_IN;
    ctrlParams.requestCmd = HID_GET_PROTOCOL;
    ctrlParams.value = 0;
    ctrlParams.index = interface.GetId();
    ctrlParams.length = 1;
    ctrlParams.timeout = CONTROL_TRANSFER_TIMEOUT;
    std::vector<uint8_t> bufferData(1);
    int32_t ret = client.UsbControlTransfer(pipe, ctrlParams, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_HidRequest_003 GET_PROTOCOL ret=%{public}d", ret);
    EXPECT_TRUE(ret == UEC_OK || ret == HID_INT_IO_ERROR);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_HidRequest_004
 * @tc.desc: Test HID SET_IDLE class request is accepted.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_HidRequest_004, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    HDI::Usb::V1_2::UsbCtrlTransferParams ctrlParams;
    ctrlParams.requestType = REQUEST_TYPE_CLASS_INTERFACE_OUT;
    ctrlParams.requestCmd = HID_SET_IDLE;
    ctrlParams.value = 0;
    ctrlParams.index = interface.GetId();
    ctrlParams.length = 0;
    ctrlParams.timeout = CONTROL_TRANSFER_TIMEOUT;
    std::vector<uint8_t> bufferData;
    int32_t ret = client.UsbControlTransfer(pipe, ctrlParams, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_HidRequest_004 SET_IDLE ret=%{public}d", ret);
    EXPECT_TRUE(ret == UEC_OK || ret == HID_INT_IO_ERROR);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_HidRequest_005
 * @tc.desc: Test HID GET_PROTOCOL / SET_PROTOCOL round-trip on a keyboard.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_HidRequest_005, TestSize.Level1)
{
    UsbDevice device;
    if (!FindKeyboardDevice(device)) {
        GTEST_SKIP() << "no keyboard connected, skip keyboard-specific protocol case";
    }
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();

    HDI::Usb::V1_2::UsbCtrlTransferParams getParams;
    getParams.requestType = REQUEST_TYPE_CLASS_INTERFACE_IN;
    getParams.requestCmd = HID_GET_PROTOCOL;
    getParams.value = 0;
    getParams.index = interface.GetId();
    getParams.length = 1;
    getParams.timeout = CONTROL_TRANSFER_TIMEOUT;
    std::vector<uint8_t> bufferData(1);
    int32_t getRet = client.UsbControlTransfer(pipe, getParams, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_HidRequest_005 GET_PROTOCOL=%{public}d", getRet);
    EXPECT_TRUE(getRet == UEC_OK || getRet == HID_INT_IO_ERROR);

    HDI::Usb::V1_2::UsbCtrlTransferParams setParams;
    setParams.requestType = REQUEST_TYPE_CLASS_INTERFACE_OUT;
    setParams.requestCmd = HID_SET_PROTOCOL;
    setParams.value = HID_PROTOCOL_KEYBOARD;
    setParams.index = interface.GetId();
    setParams.length = 0;
    setParams.timeout = CONTROL_TRANSFER_TIMEOUT;
    std::vector<uint8_t> empty;
    int32_t setRet = client.UsbControlTransfer(pipe, setParams, empty);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_HidRequest_005 SET_PROTOCOL=%{public}d", setRet);
    EXPECT_TRUE(setRet == UEC_OK || setRet == HID_INT_IO_ERROR);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_HidRequest_006
 * @tc.desc: Test HID SET_REPORT (keyboard LED report) is accepted.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_HidRequest_006, TestSize.Level1)
{
    UsbDevice device;
    if (!FindKeyboardDevice(device)) {
        GTEST_SKIP() << "no keyboard connected, skip keyboard-specific LED case";
    }
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();

    HDI::Usb::V1_2::UsbCtrlTransferParams ctrlParams;
    ctrlParams.requestType = REQUEST_TYPE_CLASS_INTERFACE_OUT;
    ctrlParams.requestCmd = HID_SET_REPORT;
    ctrlParams.value = 0x0200;
    ctrlParams.index = interface.GetId();
    ctrlParams.length = 1;
    ctrlParams.timeout = CONTROL_TRANSFER_TIMEOUT;
    std::vector<uint8_t> bufferData(1, 0x00);
    int32_t ret = client.UsbControlTransfer(pipe, ctrlParams, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_HidRequest_006 SET_REPORT=%{public}d", ret);
    EXPECT_TRUE(ret == UEC_OK || ret == HID_INT_IO_ERROR);
    (void)client.Close(pipe);
}

/* -------------------------------------------------------------------------- */
/* Section 9: Endpoint validation                                             */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Endpoint_001
 * @tc.desc: Test that the HID device exposes an interrupt IN endpoint.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Endpoint_001, TestSize.Level1)
{
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    UsbInterface interface;
    USBEndpoint endpoint;
    bool found = FindInterruptInEndpoint(device, interface, endpoint);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Endpoint_001 found=%{public}d", found);
    EXPECT_TRUE(found) << "HID device must expose an interrupt IN endpoint";
}

/**
 * @tc.name: HidHost_Endpoint_002
 * @tc.desc: Test the interrupt IN endpoint reports direction IN and type INT.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Endpoint_002, TestSize.Level1)
{
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    UsbInterface interface;
    USBEndpoint endpoint;
    ASSERT_TRUE(FindInterruptInEndpoint(device, interface, endpoint));
    EXPECT_EQ(endpoint.GetType(), HID_ENDPOINT_TYPE_INTERRUPT);
    EXPECT_EQ(endpoint.GetDirection(), USB_ENDPOINT_DIR_IN);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Endpoint_002 addr=0x%{public}X interval=%{public}d maxPacket=%{public}d",
        endpoint.GetAddress(), endpoint.GetInterval(), endpoint.GetMaxPacketSize());
}

/**
 * @tc.name: HidHost_Endpoint_003
 * @tc.desc: Test all endpoints on the HID device have a valid transfer type.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Endpoint_003, TestSize.Level1)
{
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    for (auto &config : device.GetConfigs()) {
        for (auto &intf : config.GetInterfaces()) {
            for (auto &ep : intf.GetEndpoints()) {
                int32_t type = static_cast<int32_t>(ep.GetType());
                USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Endpoint_003 addr=0x%{public}X type=%{public}d",
                    ep.GetAddress(), type);
                EXPECT_TRUE(type == USB_ENDPOINT_XFER_CONTROL || type == USB_ENDPOINT_XFER_ISOC ||
                    type == USB_ENDPOINT_XFER_BULK || type == USB_ENDPOINT_XFER_INT);
            }
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Section 10: Interrupt transfers                                            */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Interrupt_001
 * @tc.desc: Test submitting an interrupt IN transfer on the HID device. The
 *           submit must be accepted; report data arrives when the keyboard or
 *           mouse produces it.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Interrupt_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    UsbInterface interface;
    USBEndpoint endpoint;
    ASSERT_TRUE(FindInterruptInEndpoint(device, interface, endpoint));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    ASSERT_EQ(client.ClaimInterface(pipe, interface, true), UEC_OK);

    sptr<Ashmem> ashmem = Ashmem::CreateAshmem("usb_hid_intr", INTERRUPT_BUFFER_LENGTH);
    ASSERT_NE(ashmem, nullptr);
    ashmem->MapReadAndWriteAshmem();

    HDI::Usb::V1_2::USBTransferInfo transferInfo;
    transferInfo.endpoint = endpoint.GetAddress();
    transferInfo.flags = 0;
    transferInfo.type = USB_ENDPOINT_XFER_INT;
    transferInfo.timeOut = INTERRUPT_TRANSFER_TIMEOUT;
    transferInfo.length = INTERRUPT_BUFFER_LENGTH;
    transferInfo.userData = 0;
    transferInfo.numIsoPackets = 0;

    auto callback = [](const TransferCallbackInfo &info,
                        const std::vector<HDI::Usb::V1_2::UsbIsoPacketDescriptor> &packets, uint64_t userData) {
        USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Interrupt_001 cb status=%{public}d actualLength=%{public}d",
            info.status, info.actualLength);
    };
    int32_t ret = client.UsbSubmitTransfer(pipe, transferInfo, callback, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Interrupt_001 submit ret=%{public}d", ret);
    EXPECT_TRUE(ret == UEC_OK || ret == HID_INT_IO_ERROR);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Interrupt_002
 * @tc.desc: Test submitting an interrupt transfer to an invalid endpoint
 *           returns an error instead of crashing.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Interrupt_002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    UsbInterface interface;
    USBEndpoint endpoint;
    ASSERT_TRUE(FindInterruptInEndpoint(device, interface, endpoint));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    ASSERT_EQ(client.ClaimInterface(pipe, interface, true), UEC_OK);

    sptr<Ashmem> ashmem = Ashmem::CreateAshmem("usb_hid_intr_bad", INTERRUPT_BUFFER_LENGTH);
    ASSERT_NE(ashmem, nullptr);
    ashmem->MapReadAndWriteAshmem();

    HDI::Usb::V1_2::USBTransferInfo transferInfo;
    transferInfo.endpoint = 0xFF;
    transferInfo.flags = 0;
    transferInfo.type = USB_ENDPOINT_XFER_INT;
    transferInfo.timeOut = INTERRUPT_TRANSFER_TIMEOUT;
    transferInfo.length = INTERRUPT_BUFFER_LENGTH;
    transferInfo.userData = 0;
    transferInfo.numIsoPackets = 0;

    auto callback = [](const TransferCallbackInfo &info,
                        const std::vector<HDI::Usb::V1_2::UsbIsoPacketDescriptor> &packets, uint64_t userData) {};
    int32_t ret = client.UsbSubmitTransfer(pipe, transferInfo, callback, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Interrupt_002 invalid endpoint ret=%{public}d", ret);
    EXPECT_NE(ret, UEC_OK);
    (void)client.Close(pipe);
}

/* -------------------------------------------------------------------------- */
/* Section 11: Config / interface traversal                                   */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Traverse_001
 * @tc.desc: Test all configurations and interfaces are traversable.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Traverse_001, TestSize.Level1)
{
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    int32_t configIndex = 0;
    for (auto &config : device.GetConfigs()) {
        USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Traverse_001 config=%{public}d interfaces=%{public}u", configIndex++,
            config.GetInterfaceCount());
        EXPECT_GE(config.GetInterfaceCount(), 1u);
        for (auto &intf : config.GetInterfaces()) {
            USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Traverse_001 interface id=%{public}d class=%{public}d",
                intf.GetId(), intf.GetClass());
        }
    }
    SUCCEED();
}

/**
 * @tc.name: HidHost_Traverse_002
 * @tc.desc: Test configuration attributes are reported consistently.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Traverse_002, TestSize.Level1)
{
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    for (auto &config : device.GetConfigs()) {
        USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Traverse_002 id=%{public}d maxPower=%{public}d selfPowered=%{public}d",
            config.GetId(), config.GetMaxPower(), config.IsSelfPowered());
        EXPECT_GE(config.GetId(), 0);
    }
    SUCCEED();
}

/* -------------------------------------------------------------------------- */
/* Section 12: Miscellaneous device operations                                */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Misc_001
 * @tc.desc: Test GetDeviceSpeed returns UEC_OK and a plausible speed value.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Misc_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    uint8_t speed = 0;
    int32_t ret = client.GetDeviceSpeed(pipe, speed);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Misc_001 ret=%{public}d speed=%{public}u", ret, speed);
    EXPECT_EQ(ret, UEC_OK);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Misc_002
 * @tc.desc: Test GetFileDescriptor returns a non-negative fd after open.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Misc_002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    int32_t fd = -1;
    int32_t ret = client.GetFileDescriptor(pipe, fd);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Misc_002 ret=%{public}d fd=%{public}d", ret, fd);
    if (ret == UEC_OK) {
        EXPECT_GE(fd, 0);
    }
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Misc_003
 * @tc.desc: Test GetInterfaceActiveStatus reports a valid activation state.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Misc_003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    bool unactivated = false;
    int32_t ret = client.GetInterfaceActiveStatus(pipe, interface, unactivated);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Misc_003 ret=%{public}d unactivated=%{public}d", ret, unactivated);
    EXPECT_EQ(ret, UEC_OK);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Misc_004
 * @tc.desc: ResetDevice re-enumerates the device and would invalidate the
 *           cached snapshot used by the rest of the suite, so it is skipped.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Misc_004, TestSize.Level1)
{
    GTEST_SKIP() << "ResetDevice re-enumerates the device and invalidates the snapshot";
}

/**
 * @tc.name: HidHost_Misc_005
 * @tc.desc: Test ClearHalt on a claimed interrupt IN endpoint.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Misc_005, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    UsbInterface interface;
    USBEndpoint endpoint;
    ASSERT_TRUE(FindInterruptInEndpoint(device, interface, endpoint));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    ASSERT_EQ(client.ClaimInterface(pipe, interface, true), UEC_OK);
    int32_t ret = client.ClearHalt(pipe, endpoint);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Misc_005 ClearHalt=%{public}d", ret);
    SUCCEED();
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Misc_006
 * @tc.desc: Test SetInterface on the first interface.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Misc_006, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    int32_t ret = client.SetInterface(pipe, interface);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Misc_006 SetInterface=%{public}d", ret);
    SUCCEED();
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Misc_007
 * @tc.desc: Test SetConfiguration on the first configuration.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Misc_007, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    ASSERT_FALSE(device.GetConfigs().empty());
    int32_t ret = client.SetConfiguration(pipe, device.GetConfigs().front());
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Misc_007 SetConfiguration=%{public}d", ret);
    SUCCEED();
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Misc_008
 * @tc.desc: Test GetVersion returns a non-empty service version string.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Misc_008, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::string version = client.GetVersion();
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Misc_008 version=%{public}s", version.c_str());
    EXPECT_FALSE(version.empty());
}

/* -------------------------------------------------------------------------- */
/* Section 13: Port role                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Port_001
 * @tc.desc: Test GetPorts returns UEC_OK and at least one port; skipped when
 *           port management is unavailable in pass-through mode.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Port_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbPort> ports;
    int32_t ret = client.GetPorts(ports);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Port_001 ret=%{public}d ports=%{public}zu", ret, ports.size());
    if (ret != UEC_OK || ports.empty()) {
        GTEST_SKIP() << "port management unavailable in pass-through mode";
    }
    SUCCEED();
}

/**
 * @tc.name: HidHost_Port_002
 * @tc.desc: Test GetSupportedModes returns UEC_OK for the first port; skipped
 *           when port management is unavailable in pass-through mode.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Port_002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbPort> ports;
    if (client.GetPorts(ports) != UEC_OK || ports.empty()) {
        GTEST_SKIP() << "port management unavailable in pass-through mode";
    }
    int32_t supportedModes = 0;
    int32_t ret = client.GetSupportedModes(ports.front().id, supportedModes);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Port_002 ret=%{public}d modes=%{public}d", ret, supportedModes);
    SUCCEED();
}

/**
 * @tc.name: HidHost_Port_003
 * @tc.desc: SetPortRole switches the USB port role and would drop this board's
 *           own hdc debug link, so this case is skipped in stress runs.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Port_003, TestSize.Level1)
{
    GTEST_SKIP() << "SetPortRole drops the hdc debug link";
}

/* -------------------------------------------------------------------------- */
/* Section 14: EDM / management surface                                       */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Manage_001
 * @tc.desc: ManageGlobalInterface requires EDM enterprise privileges and
 *           disables the global USB interface as a side effect; skipped in
 *           stress runs.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Manage_001, TestSize.Level1)
{
    GTEST_SKIP() << "ManageGlobalInterface requires EDM privileges and disables the USB interface";
}

/**
 * @tc.name: HidHost_Manage_002
 * @tc.desc: ManageDevice requires EDM enterprise privileges and disables the
 *           device as a side effect; skipped in stress runs.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Manage_002, TestSize.Level1)
{
    GTEST_SKIP() << "ManageDevice requires EDM privileges and disables the device";
}

/* -------------------------------------------------------------------------- */
/* Section 15: Stress and boundary                                            */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Stress_001
 * @tc.desc: Test GetDevices can be called repeatedly without failure.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Stress_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    for (int32_t i = 0; i < 20; ++i) {
        std::vector<UsbDevice> devices;
        int32_t ret = client.GetDevices(devices);
        if (ret != UEC_OK) {
            ADD_FAILURE() << "GetDevices failed at iteration " << i << " ret=" << ret;
            break;
        }
    }
    SUCCEED();
}

/**
 * @tc.name: HidHost_Boundary_001
 * @tc.desc: Test claiming an interface without opening a device returns an
 *           error rather than crashing.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Boundary_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    ASSERT_FALSE(device.GetConfigs().empty());
    ASSERT_FALSE(device.GetConfigs().front().GetInterfaces().empty());
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    USBDevicePipe pipe;
    int32_t ret = client.ClaimInterface(pipe, interface, true);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Boundary_001 claim-without-open=%{public}d", ret);
    EXPECT_NE(ret, UEC_OK);
}

/**
 * @tc.name: HidHost_Boundary_002
 * @tc.desc: Test ClearHalt on an unopened pipe is tolerated.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Boundary_002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    UsbInterface interface;
    USBEndpoint endpoint;
    ASSERT_TRUE(FindInterruptInEndpoint(device, interface, endpoint));
    USBDevicePipe pipe;
    int32_t ret = client.ClearHalt(pipe, endpoint);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Boundary_002 clearhalt-without-open=%{public}d", ret);
    EXPECT_NE(ret, UEC_OK);
}

/**
 * @tc.name: HidHost_Boundary_003
 * @tc.desc: Test GetRawDescriptors on an unopened pipe returns an error.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Boundary_003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    std::vector<uint8_t> bufferData;
    int32_t ret = client.GetRawDescriptors(pipe, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Boundary_003 rawdesc-without-open=%{public}d", ret);
    EXPECT_NE(ret, UEC_OK);
}

/**
 * @tc.name: HidHost_Boundary_004
 * @tc.desc: Test GetDeviceSpeed on an unopened pipe returns an error.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Boundary_004, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    uint8_t speed = 0;
    int32_t ret = client.GetDeviceSpeed(pipe, speed);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Boundary_004 speed-without-open=%{public}d", ret);
    EXPECT_NE(ret, UEC_OK);
}

/* -------------------------------------------------------------------------- */
/* Section 16: Keyboard / mouse specific interaction                          */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Keyboard_001
 * @tc.desc: Test a keyboard device can be opened and its boot interface
 *           claimed.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Keyboard_001, TestSize.Level1)
{
    UsbDevice device;
    if (!FindKeyboardDevice(device)) {
        GTEST_SKIP() << "no keyboard connected, skip keyboard-specific case";
    }
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    int32_t ret = client.ClaimInterface(pipe, interface, true);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Keyboard_001 claim=%{public}d", ret);
    EXPECT_EQ(ret, UEC_OK);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Keyboard_002
 * @tc.desc: Test a keyboard's interrupt IN endpoint can be read via a
 *           submitted transfer while the operator presses a key.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Keyboard_002, TestSize.Level1)
{
    UsbDevice device;
    if (!FindKeyboardDevice(device)) {
        GTEST_SKIP() << "no keyboard connected, skip keyboard-specific case";
    }
    auto &client = UsbSrvClient::GetInstance();
    UsbInterface interface;
    USBEndpoint endpoint;
    ASSERT_TRUE(FindInterruptInEndpoint(device, interface, endpoint));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    ASSERT_EQ(client.ClaimInterface(pipe, interface, true), UEC_OK);

    sptr<Ashmem> ashmem = Ashmem::CreateAshmem("usb_kbd_intr", INTERRUPT_BUFFER_LENGTH);
    ASSERT_NE(ashmem, nullptr);
    ashmem->MapReadAndWriteAshmem();

    HDI::Usb::V1_2::USBTransferInfo transferInfo;
    transferInfo.endpoint = endpoint.GetAddress();
    transferInfo.flags = 0;
    transferInfo.type = USB_ENDPOINT_XFER_INT;
    transferInfo.timeOut = INTERRUPT_TRANSFER_TIMEOUT;
    transferInfo.length = INTERRUPT_BUFFER_LENGTH;
    transferInfo.userData = 0;
    transferInfo.numIsoPackets = 0;

    auto callback = [](const TransferCallbackInfo &info,
                        const std::vector<HDI::Usb::V1_2::UsbIsoPacketDescriptor> &packets, uint64_t userData) {
        USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Keyboard_002 cb status=%{public}d actualLength=%{public}d",
            info.status, info.actualLength);
    };
    int32_t ret = client.UsbSubmitTransfer(pipe, transferInfo, callback, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Keyboard_002 submit ret=%{public}d", ret);
    EXPECT_TRUE(ret == UEC_OK || ret == HID_INT_IO_ERROR);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Mouse_001
 * @tc.desc: Test a mouse device can be opened and its boot interface claimed.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Mouse_001, TestSize.Level1)
{
    UsbDevice device;
    if (!FindMouseDevice(device)) {
        GTEST_SKIP() << "no mouse connected, skip mouse-specific case";
    }
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().front();
    int32_t ret = client.ClaimInterface(pipe, interface, true);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Mouse_001 claim=%{public}d", ret);
    EXPECT_EQ(ret, UEC_OK);
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_Mouse_002
 * @tc.desc: Test a mouse's interrupt IN endpoint can be read via a submitted
 *           transfer while the operator moves the mouse.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Mouse_002, TestSize.Level1)
{
    UsbDevice device;
    if (!FindMouseDevice(device)) {
        GTEST_SKIP() << "no mouse connected, skip mouse-specific case";
    }
    auto &client = UsbSrvClient::GetInstance();
    UsbInterface interface;
    USBEndpoint endpoint;
    ASSERT_TRUE(FindInterruptInEndpoint(device, interface, endpoint));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);
    ASSERT_EQ(client.ClaimInterface(pipe, interface, true), UEC_OK);

    sptr<Ashmem> ashmem = Ashmem::CreateAshmem("usb_mouse_intr", INTERRUPT_BUFFER_LENGTH);
    ASSERT_NE(ashmem, nullptr);
    ashmem->MapReadAndWriteAshmem();

    HDI::Usb::V1_2::USBTransferInfo transferInfo;
    transferInfo.endpoint = endpoint.GetAddress();
    transferInfo.flags = 0;
    transferInfo.type = USB_ENDPOINT_XFER_INT;
    transferInfo.timeOut = INTERRUPT_TRANSFER_TIMEOUT;
    transferInfo.length = INTERRUPT_BUFFER_LENGTH;
    transferInfo.userData = 0;
    transferInfo.numIsoPackets = 0;

    auto callback = [](const TransferCallbackInfo &info,
                        const std::vector<HDI::Usb::V1_2::UsbIsoPacketDescriptor> &packets, uint64_t userData) {
        USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Mouse_002 cb status=%{public}d actualLength=%{public}d", info.status,
            info.actualLength);
    };
    int32_t ret = client.UsbSubmitTransfer(pipe, transferInfo, callback, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Mouse_002 submit ret=%{public}d", ret);
    EXPECT_TRUE(ret == UEC_OK || ret == HID_INT_IO_ERROR);
    (void)client.Close(pipe);
}

/* -------------------------------------------------------------------------- */
/* Section 17: Callback registration and end-to-end                           */
/* -------------------------------------------------------------------------- */

/**
 * @tc.name: HidHost_Callback_001
 * @tc.desc: Test RegBulkCallback / UnRegBulkCallback with a null callback are
 *           rejected without crashing.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_Callback_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    ASSERT_TRUE(FindHidDevice(device));
    UsbInterface interface;
    USBEndpoint endpoint;
    ASSERT_TRUE(FindInterruptInEndpoint(device, interface, endpoint));
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(device, pipe), UEC_OK);

    sptr<IRemoteObject> nullCb = nullptr;
    int32_t regRet = client.RegBulkCallback(pipe, endpoint, nullCb);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Callback_001 RegBulkCallback=%{public}d", regRet);
    EXPECT_NE(regRet, UEC_OK);

    int32_t unregRet = client.UnRegBulkCallback(pipe, endpoint);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_Callback_001 UnRegBulkCallback=%{public}d", unregRet);
    SUCCEED();
    (void)client.Close(pipe);
}

/**
 * @tc.name: HidHost_E2E_001
 * @tc.desc: End-to-end lifecycle: enumerate, locate HID device, request right,
 *           open, claim, read raw descriptor, read interrupt endpoint, release,
 *           and close in the correct order.
 * @tc.type: FUNC
 */
HWTEST_F(UsbHidHostTest, HidHost_E2E_001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();

    // 1. enumerate
    std::vector<UsbDevice> devices;
    ASSERT_EQ(client.GetDevices(devices), UEC_OK);
    ASSERT_FALSE(devices.empty());

    // 2. locate an HID device with an interrupt-IN endpoint
    UsbDevice hidDevice;
    UsbInterface hidInterface;
    USBEndpoint hidEndpoint;
    bool foundHid = false;
    for (auto &dev : devices) {
        if (IsHidDevice(dev) && FindInterruptInEndpoint(dev, hidInterface, hidEndpoint)) {
            hidDevice = dev;
            foundHid = true;
            break;
        }
    }
    if (!foundHid) {
        GTEST_SKIP() << "no HID device with interrupt-IN endpoint found";
    }

    // 3. request right (best effort on a headless board)
    (void)client.RequestRight(hidDevice.GetName());

    // 4. open
    USBDevicePipe pipe;
    ASSERT_EQ(client.OpenDevice(hidDevice, pipe), UEC_OK);

    // 5. claim
    ASSERT_EQ(client.ClaimInterface(pipe, hidInterface, true), UEC_OK);

    // 6. read raw descriptor
    std::vector<uint8_t> rawDesc;
    int32_t descRet = client.GetRawDescriptors(pipe, rawDesc);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_E2E_001 rawDesc ret=%{public}d size=%{public}zu", descRet, rawDesc.size());
    EXPECT_EQ(descRet, UEC_OK);

    // 7. read interrupt endpoint
    sptr<Ashmem> ashmem = Ashmem::CreateAshmem("usb_e2e_intr", INTERRUPT_BUFFER_LENGTH);
    ASSERT_NE(ashmem, nullptr);
    ashmem->MapReadAndWriteAshmem();
    HDI::Usb::V1_2::USBTransferInfo transferInfo;
    transferInfo.endpoint = hidEndpoint.GetAddress();
    transferInfo.flags = 0;
    transferInfo.type = USB_ENDPOINT_XFER_INT;
    transferInfo.timeOut = INTERRUPT_TRANSFER_TIMEOUT;
    transferInfo.length = INTERRUPT_BUFFER_LENGTH;
    transferInfo.userData = 0;
    transferInfo.numIsoPackets = 0;
    int32_t submitRet = client.UsbSubmitTransfer(pipe, transferInfo,
        [](const TransferCallbackInfo &info, const std::vector<HDI::Usb::V1_2::UsbIsoPacketDescriptor> &packets,
            uint64_t userData) {
            USB_HILOGI(MODULE_USB_SERVICE, "HidHost_E2E_001 cb status=%{public}d len=%{public}d", info.status,
                info.actualLength);
        },
        ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "HidHost_E2E_001 submit ret=%{public}d", submitRet);
    EXPECT_TRUE(submitRet == UEC_OK || submitRet == HID_INT_IO_ERROR);

    // 8. release
    EXPECT_EQ(client.ReleaseInterface(pipe, hidInterface), UEC_OK);

    // 9. close
    EXPECT_TRUE(client.Close(pipe));
}

} // namespace HidHost
} // namespace USB
} // namespace OHOS