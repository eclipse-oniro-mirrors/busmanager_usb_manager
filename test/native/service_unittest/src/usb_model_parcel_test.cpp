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

/*
 * UsbModelParcelTest
 *
 * This test suite covers the Parcel (marshalling / unmarshalling) and cJSON
 * serialization paths of the USB inner-kit model classes:
 *
 *   - UsbDevice
 *   - USBConfig
 *   - UsbInterface
 *   - USBEndpoint
 *   - UsbPort / UsbPortStatus
 *   - USBAccessory
 *
 * The focus is on binary round-trips through OHOS::Parcel and JSON round-trips
 * through cJSON, plus boundary conditions (empty parcels, truncated parcels,
 * element-count caps such as USB_CONFIG_MAX_NUM / USB_INTERFACE_MAX_NUM /
 * USB_ENDPOINT_MAX_NUM, and getter semantics for the endpoint address masks).
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cJSON.h"
#include "hilog_wrapper.h"
#include "usb_accessory.h"
#include "usb_config.h"
#include "usb_device.h"
#include "usb_endpoint.h"
#include "usb_errors.h"
#include "usb_interface.h"
#include "usb_port.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::USB;

namespace OHOS {
namespace USB {
namespace ServiceTest {
namespace {
// Count caps enforced by the model marshalling code.
constexpr uint32_t USB_MODEL_ENDPOINT_MAX_NUM = 128;
constexpr uint32_t USB_MODEL_INTERFACE_MAX_NUM = 128;
constexpr uint32_t USB_MODEL_CONFIG_MAX_NUM = 128;

// Endpoint address / attribute bit masks defined in usb_common.h.
constexpr uint32_t MODEL_XFERTYPE_MASK = 0x03;
constexpr uint32_t MODEL_NUMBER_MASK = 0x0f;
constexpr uint32_t MODEL_DIR_MASK = 0x80;

// Common control-transfer endpoint addresses.
constexpr uint32_t MODEL_EP_ADDR_IN = 0x81;
constexpr uint32_t MODEL_EP_ADDR_OUT = 0x01;
constexpr uint32_t MODEL_EP_ADDR_BULK_IN = 0x82;
constexpr uint32_t MODEL_EP_ADDR_BULK_OUT = 0x02;

// USBConfig attribute flags.
constexpr uint32_t MODEL_CFG_SELF_POWERED = 0x80;
constexpr uint32_t MODEL_CFG_REMOTE_WAKEUP = 0x20;
constexpr uint32_t MODEL_CFG_BOTH = MODEL_CFG_SELF_POWERED | MODEL_CFG_REMOTE_WAKEUP;
constexpr uint32_t MODEL_CFG_NONE = 0;

constexpr uint8_t MODEL_INTERFACE_ID_MAX = 255;
constexpr uint8_t MODEL_I_INTERFACE_UNSET = 255;
constexpr uint8_t MODEL_I_CONFIG_UNSET = 255;

constexpr const char *MODEL_NAME = "usb_model_parcel_test";
constexpr const char *MODEL_MFG = "OpenHarmony Device Co., Ltd";
constexpr const char *MODEL_PRODUCT = "OpenHarmony USB Bridge";
constexpr const char *MODEL_VERSION = "2.1.0";
constexpr const char *MODEL_SERIAL = "SN-2048-1128";

// Fixture values consumed by the MakeModel* helpers below.
constexpr uint32_t MODEL_EP_ATTR_BULK = 2;
constexpr uint32_t MODEL_EP_INTERVAL_DEFAULT = 0;
constexpr uint32_t MODEL_EP_MAX_PACKET_SIZE = 64;
constexpr uint8_t MODEL_EP_INTERFACE_ID = 3;
constexpr int32_t MODEL_IFACE_CLASS_MASS_STORAGE = 8;
constexpr int32_t MODEL_IFACE_SUBCLASS_SCSI = 6;
constexpr uint32_t MODEL_CONFIG_ID = 1;
constexpr uint32_t MODEL_CONFIG_MAX_POWER = 250;
constexpr uint8_t MODEL_DEV_ADDR = 2;
constexpr uint8_t MODEL_BUS_NUM = 1;
constexpr int32_t MODEL_VENDOR_ID = 0x12D1;
constexpr int32_t MODEL_PRODUCT_ID = 0x0001;
constexpr uint8_t MODEL_I_MANUFACTURER = 1;
constexpr uint8_t MODEL_I_PRODUCT = 2;
constexpr uint8_t MODEL_I_SERIAL_NUMBER = 3;
constexpr uint8_t MODEL_B_MAX_PACKET_SIZE0 = 64;
} // namespace

class UsbModelParcelTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void UsbModelParcelTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbModelParcelTest::SetUpTestCase enter");
}

void UsbModelParcelTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbModelParcelTest::TearDownTestCase enter");
}

void UsbModelParcelTest::SetUp(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbModelParcelTest::SetUp enter");
}

void UsbModelParcelTest::TearDown(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbModelParcelTest::TearDown enter");
}

/*
 * Allocates a canonical USBEndpoint used by most round-trip tests below.
 * The endpoint is bulk IN on interface 3 with a 64-byte max packet.
 */
USBEndpoint MakeModelEndpoint(void)
{
    USBEndpoint ep(MODEL_EP_ADDR_BULK_IN, MODEL_EP_ATTR_BULK, MODEL_EP_INTERVAL_DEFAULT,
        MODEL_EP_MAX_PACKET_SIZE);
    ep.SetInterfaceId(MODEL_EP_INTERFACE_ID);
    return ep;
}

/*
 * Builds a UsbInterface carrying the given endpoint vector. The interface
 * uses realistic class/subclass values so later JSON assertions are stable.
 */
UsbInterface MakeModelInterface(const std::vector<USBEndpoint> &endpoints)
{
    UsbInterface intf(0, 0, MODEL_IFACE_CLASS_MASS_STORAGE, MODEL_IFACE_SUBCLASS_SCSI, 0, "Mass Storage",
        endpoints);
    intf.SetiInterface(1);
    return intf;
}

/*
 * Builds a USBConfig holding the given interfaces. Attributes default to
 * self-powered so IsSelfPowered() returns true.
 */
USBConfig MakeModelConfig(const std::vector<UsbInterface> &interfaces)
{
    USBConfig config(MODEL_CONFIG_ID, MODEL_CFG_SELF_POWERED, "Config 1", MODEL_CONFIG_MAX_POWER, interfaces);
    config.SetiConfiguration(0);
    return config;
}

/*
 * Builds a UsbDevice with the given configs and a deterministic set of
 * descriptor-level fields (bcdUSB, bcdDevice, iManufacturer, ...).
 */
UsbDevice MakeModelDevice(const std::vector<USBConfig> &configs)
{
    UsbDevice device(MODEL_NAME, MODEL_MFG, MODEL_PRODUCT, MODEL_VERSION,
        MODEL_DEV_ADDR, MODEL_BUS_NUM, MODEL_VENDOR_ID, MODEL_PRODUCT_ID, 0, 0, 0, configs);
    device.SetmSerial(MODEL_SERIAL);
    device.SetiManufacturer(MODEL_I_MANUFACTURER);
    device.SetiProduct(MODEL_I_PRODUCT);
    device.SetiSerialNumber(MODEL_I_SERIAL_NUMBER);
    device.SetbMaxPacketSize0(MODEL_B_MAX_PACKET_SIZE0);
    device.SetbcdUSB(0x0200);
    device.SetbcdDevice(0x0100);
    device.SetDescConfigCount(1);
    device.SetAuthorizeStatus(ENABLED);
    return device;
}

/*
 * Marshals a USBEndpoint into a Parcel and unmarshals a new object from it.
 * Returns nullptr only if the write itself fails.
 */
std::unique_ptr<USBEndpoint> RoundTripModelEndpoint(const USBEndpoint &ep)
{
    Parcel parcel;
    if (!ep.Marshalling(parcel)) {
        return nullptr;
    }
    return std::unique_ptr<USBEndpoint>(USBEndpoint::Unmarshalling(parcel));
}

/*
 * Marshals a UsbInterface into a Parcel and unmarshals a new object from it.
 */
std::unique_ptr<UsbInterface> RoundTripModelInterface(const UsbInterface &intf)
{
    Parcel parcel;
    if (!intf.Marshalling(parcel)) {
        return nullptr;
    }
    return std::unique_ptr<UsbInterface>(UsbInterface::Unmarshalling(parcel));
}

/*
 * Marshals a USBConfig into a Parcel and unmarshals a new object from it.
 */
std::unique_ptr<USBConfig> RoundTripModelConfig(const USBConfig &config)
{
    Parcel parcel;
    if (!config.Marshalling(parcel)) {
        return nullptr;
    }
    return std::unique_ptr<USBConfig>(USBConfig::Unmarshalling(parcel));
}

/*
 * Marshals a UsbDevice into a Parcel and unmarshals a new object from it.
 */
std::unique_ptr<UsbDevice> RoundTripModelDevice(const UsbDevice &device)
{
    Parcel parcel;
    if (!device.Marshalling(parcel)) {
        return nullptr;
    }
    return std::unique_ptr<UsbDevice>(UsbDevice::Unmarshalling(parcel));
}

/*
 * Marshals a UsbPort into a Parcel and unmarshals a new object from it.
 */
std::unique_ptr<UsbPort> RoundTripModelPort(const UsbPort &port)
{
    Parcel parcel;
    if (!port.Marshalling(parcel)) {
        return nullptr;
    }
    return std::unique_ptr<UsbPort>(UsbPort::Unmarshalling(parcel));
}

/*
 * Marshals a USBAccessory into a Parcel and unmarshals a new object from it.
 */
std::unique_ptr<USBAccessory> RoundTripModelAccessory(const USBAccessory &accessory)
{
    Parcel parcel;
    if (!accessory.Marshalling(parcel)) {
        return nullptr;
    }
    return std::unique_ptr<USBAccessory>(USBAccessory::Unmarshalling(parcel));
}

/*
 * Parses a JSON string with cJSON and returns a parsed root object, or
 * nullptr when the input is not valid JSON. The caller owns the result and
 * must release it through cJSON_Delete().
 */
cJSON *ParseModelJson(const std::string &jsonText)
{
    return cJSON_Parse(jsonText.c_str());
}

/*
 * Serializes the device through getJsonString() and reconstructs a UsbDevice
 * from the produced JSON, mimicking the driver event broadcast path.
 */
UsbDevice ReconstructModelDeviceFromJson(const UsbDevice &device)
{
    std::string jsonText = device.getJsonString();
    cJSON *json = ParseModelJson(jsonText);
    if (json == nullptr) {
        return UsbDevice();
    }
    UsbDevice reconstructed(json);
    cJSON_Delete(json);
    return reconstructed;
}

/*
 * Serializes the config through getJsonString() and reconstructs a USBConfig
 * from the produced JSON.
 */
USBConfig ReconstructModelConfigFromJson(const USBConfig &config)
{
    std::string jsonText = config.getJsonString();
    cJSON *json = ParseModelJson(jsonText);
    if (json == nullptr) {
        return USBConfig();
    }
    USBConfig reconstructed(json);
    cJSON_Delete(json);
    return reconstructed;
}

/*
 * Serializes the interface through getJsonString() and reconstructs a
 * UsbInterface from the produced JSON.
 */
UsbInterface ReconstructModelInterfaceFromJson(const UsbInterface &intf)
{
    std::string jsonText = intf.getJsonString();
    cJSON *json = ParseModelJson(jsonText);
    if (json == nullptr) {
        return UsbInterface();
    }
    UsbInterface reconstructed(json);
    cJSON_Delete(json);
    return reconstructed;
}

/*
 * Serializes the endpoint through getJsonString() and reconstructs a
 * USBEndpoint from the produced JSON.
 */
USBEndpoint ReconstructModelEndpointFromJson(const USBEndpoint &ep)
{
    std::string jsonText = ep.getJsonString();
    cJSON *json = ParseModelJson(jsonText);
    if (json == nullptr) {
        return USBEndpoint();
    }
    USBEndpoint reconstructed(json);
    cJSON_Delete(json);
    return reconstructed;
}

/*
 * Serializes the accessory through GetJsonString() and reconstructs a
 * USBAccessory from the produced JSON.
 */
USBAccessory ReconstructModelAccessoryFromJson(const USBAccessory &accessory)
{
    std::string jsonText = accessory.GetJsonString();
    cJSON *json = ParseModelJson(jsonText);
    if (json == nullptr) {
        return USBAccessory();
    }
    USBAccessory reconstructed(json);
    cJSON_Delete(json);
    return reconstructed;
}

/*
 * Verifies the five string fields of two accessories are equal.
 */
bool ExpectModelAccessoryEqual(const USBAccessory &lhs, const USBAccessory &rhs)
{
    EXPECT_EQ(lhs.GetManufacturer(), rhs.GetManufacturer());
    EXPECT_EQ(lhs.GetProduct(), rhs.GetProduct());
    EXPECT_EQ(lhs.GetDescription(), rhs.GetDescription());
    EXPECT_EQ(lhs.GetVersion(), rhs.GetVersion());
    EXPECT_EQ(lhs.GetSerialNumber(), rhs.GetSerialNumber());
    return true;
}

/*
 * The USBConfig serializes id/attributes/maxPower/name and, when present,
 * the interface array. All fields must survive the round-trip.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_ParcelRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_ParcelRoundTrip_001 enter");
    USBConfig config = MakeModelConfig({});
    config.SetMaxPower(120);
    std::unique_ptr<USBConfig> out = RoundTripModelConfig(config);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetId(), 1);
    EXPECT_EQ(out->GetAttributes(), MODEL_CFG_SELF_POWERED);
    EXPECT_EQ(out->GetMaxPower(), 240);
    EXPECT_EQ(out->GetName(), "Config 1");
    EXPECT_EQ(out->GetInterfaceCount(), 0u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_ParcelRoundTrip_001 end");
}

/*
 * A USBConfig with no interfaces: the empty interface count must be written
 * and read back as zero.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_ParcelEmptyInterfaces_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_ParcelEmptyInterfaces_001 enter");
    USBConfig config(7, 0, "Empty", 100, {});
    std::unique_ptr<USBConfig> out = RoundTripModelConfig(config);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetId(), 7);
    EXPECT_EQ(out->GetName(), "Empty");
    EXPECT_EQ(out->GetInterfaceCount(), 0u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_ParcelEmptyInterfaces_001 end");
}

/*
 * The iConfiguration index is a descriptor-level field and is serialized into
 * the parcel, so it must be preserved across the round-trip.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_ParcelIConfiguration_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_ParcelIConfiguration_001 enter");
    USBConfig config(3, 0, "Cfg", 200, {});
    config.SetiConfiguration(5);
    std::unique_ptr<USBConfig> out = RoundTripModelConfig(config);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetiConfiguration(), 5u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_ParcelIConfiguration_001 end");
}

/*
 * With both bits set, both predicate getters must be true, and the flags must
 * survive a parcel round-trip.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_AttributeBoth_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_AttributeBoth_001 enter");
    USBConfig config(1, MODEL_CFG_BOTH, "Plain", 100, {});
    EXPECT_TRUE(config.IsSelfPowered());
    EXPECT_TRUE(config.IsRemoteWakeup());
    std::unique_ptr<USBConfig> out = RoundTripModelConfig(config);
    ASSERT_NE(out, nullptr);
    EXPECT_TRUE(out->IsSelfPowered());
    EXPECT_TRUE(out->IsRemoteWakeup());
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_AttributeBoth_001 end");
}

/*
 * GetMaxPower() reports twice the stored value, following the USB 2.x power
 * unit convention (2mA per unit).
 */
HWTEST_F(UsbModelParcelTest, USBConfig_MaxPowerDouble_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_MaxPowerDouble_001 enter");
    USBConfig config(1, 0, "Power", 0, {});
    config.SetMaxPower(50);
    EXPECT_EQ(config.GetMaxPower(), 100);
    config.SetMaxPower(0);
    EXPECT_EQ(config.GetMaxPower(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_MaxPowerDouble_001 end");
}

/*
 * GetInterface() must return false for any out-of-range index.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_GetInterfaceOutOfRange_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_GetInterfaceOutOfRange_001 enter");
    std::vector<UsbInterface> interfaces;
    interfaces.push_back(MakeModelInterface({}));
    USBConfig config(1, 0, "Cfg", 100, interfaces);
    UsbInterface found;
    EXPECT_FALSE(config.GetInterface(1, found));
    EXPECT_FALSE(config.GetInterface(100, found));
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_GetInterfaceOutOfRange_001 end");
}

/*
 * A config holding two interfaces must expose both through GetInterface(),
 * and both must survive a parcel round-trip inside the config.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_MultipleInterfacesParcel_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_MultipleInterfacesParcel_001 enter");
    std::vector<UsbInterface> interfaces;
    UsbInterface hcd(0, 0, 9, 0, 0, "Hub", {});
    UsbInterface msc(1, 0x50, 8, 6, 0, "Mass", {});
    interfaces.push_back(hcd);
    interfaces.push_back(msc);
    USBConfig config(1, 0, "Cfg", 100, interfaces);
    std::unique_ptr<USBConfig> out = RoundTripModelConfig(config);
    ASSERT_NE(out, nullptr);
    ASSERT_EQ(out->GetInterfaceCount(), 2u);
    UsbInterface first;
    UsbInterface second;
    ASSERT_TRUE(out->GetInterface(0, first));
    ASSERT_TRUE(out->GetInterface(1, second));
    EXPECT_EQ(first.GetId(), 0);
    EXPECT_EQ(first.GetName(), "Hub");
    EXPECT_EQ(second.GetId(), 1);
    EXPECT_EQ(second.GetName(), "Mass");
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_MultipleInterfacesParcel_001 end");
}

/*
 * A JSON object without the interfaces array is tolerated: the reconstructed
 * config keeps scalar fields and carries zero interfaces.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_JsonMissingInterfaces_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonMissingInterfaces_001 enter");
    cJSON *json = cJSON_Parse("{\"id\":9,\"attributes\":0,\"maxPower\":500,\"name\":\"NoIf\"}");
    ASSERT_NE(json, nullptr);
    USBConfig config(json);
    cJSON_Delete(json);
    EXPECT_EQ(config.GetId(), 9);
    EXPECT_EQ(config.GetMaxPower(), 1000);
    EXPECT_EQ(config.GetName(), "NoIf");
    EXPECT_EQ(config.GetInterfaceCount(), 0u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonMissingInterfaces_001 end");
}

/*
 * A JSON object with non-numeric scalar fields must fall back to defaults: the
 * int getter returns zero and the string getter returns an empty string.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_JsonWrongFieldTypes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonWrongFieldTypes_001 enter");
    cJSON *json = cJSON_Parse("{\"id\":\"abc\",\"attributes\":true,\"name\":123}");
    ASSERT_NE(json, nullptr);
    USBConfig config(json);
    cJSON_Delete(json);
    EXPECT_EQ(config.GetId(), 0);
    EXPECT_EQ(config.GetAttributes(), 0u);
    EXPECT_EQ(config.GetName(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonWrongFieldTypes_001 end");
}

/*
 * A null JSON pointer is accepted: every field falls back to its default.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_JsonNull_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonNull_001 enter");
    USBConfig config(nullptr);
    EXPECT_EQ(config.GetId(), 0);
    EXPECT_EQ(config.GetAttributes(), 0u);
    EXPECT_EQ(config.GetMaxPower(), 0);
    EXPECT_EQ(config.GetName(), "");
    EXPECT_EQ(config.GetInterfaceCount(), 0u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonNull_001 end");
}

/*
 * getJsonString() -> cJSON_Parse() -> USBConfig(cJSON*) must reproduce all
 * scalar fields and the interface list.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_JsonRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonRoundTrip_001 enter");
    std::vector<UsbInterface> interfaces;
    interfaces.push_back(MakeModelInterface({}));
    USBConfig config(4, MODEL_CFG_BOTH, "RoundTrip", 250, interfaces);
    USBConfig reconstructed = ReconstructModelConfigFromJson(config);
    EXPECT_EQ(reconstructed.GetId(), 4);
    EXPECT_EQ(reconstructed.GetAttributes(), MODEL_CFG_BOTH);
    EXPECT_EQ(reconstructed.GetMaxPower(), 500);
    EXPECT_EQ(reconstructed.GetName(), "RoundTrip");
    EXPECT_EQ(reconstructed.GetInterfaceCount(), 1u);
    EXPECT_TRUE(reconstructed.IsSelfPowered());
    EXPECT_TRUE(reconstructed.IsRemoteWakeup());
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonRoundTrip_001 end");
}

/*
 * A config nested inside a device must survive the device getJsonString()
 * round-trip including its scalar attributes.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_JsonNestedInDevice_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonNestedInDevice_001 enter");
    USBConfig config = MakeModelConfig({});
    config.SetMaxPower(180);
    UsbDevice device = MakeModelDevice({config});
    UsbDevice reconstructed = ReconstructModelDeviceFromJson(device);
    ASSERT_EQ(reconstructed.GetConfigCount(), 1);
    USBConfig nested;
    ASSERT_EQ(reconstructed.GetConfig(0, nested), ERR_OK);
    EXPECT_EQ(nested.GetId(), 1);
    EXPECT_EQ(nested.GetAttributes(), MODEL_CFG_SELF_POWERED);
    EXPECT_EQ(nested.GetMaxPower(), 360);
    EXPECT_EQ(nested.GetName(), "Config 1");
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonNestedInDevice_001 end");
}

/*
 * Interfaces nested two levels deep (device -> config -> interface) must keep
 * their id and name through the JSON round-trip.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_JsonDeepNestedInterface_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonDeepNestedInterface_001 enter");
    USBConfig config = MakeModelConfig({MakeModelInterface({})});
    config.SetId(11);
    UsbDevice device = MakeModelDevice({config});
    UsbDevice reconstructed = ReconstructModelDeviceFromJson(device);
    ASSERT_EQ(reconstructed.GetConfigCount(), 1);
    USBConfig nested;
    ASSERT_EQ(reconstructed.GetConfig(0, nested), ERR_OK);
    ASSERT_EQ(nested.GetInterfaceCount(), 1u);
    UsbInterface intf;
    ASSERT_TRUE(nested.GetInterface(0, intf));
    EXPECT_EQ(intf.GetId(), 0);
    EXPECT_EQ(intf.GetName(), "Mass Storage");
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_JsonDeepNestedInterface_001 end");
}

/*
 * ToString() must mention the config name and id so logs stay greppable.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_ToString_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_ToString_001 enter");
    USBConfig config = MakeModelConfig({});
    std::string text = config.ToString();
    EXPECT_NE(text.find("USBConfig["), std::string::npos);
    EXPECT_NE(text.find("name=Config 1"), std::string::npos);
    EXPECT_NE(text.find("id=1"), std::string::npos);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_ToString_001 end");
}

/*
 * A USBConfig with exactly USB_INTERFACE_MAX_NUM interfaces marshals and
 * unmarshals without losing any interface.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_MaxInterfaceCount_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_MaxInterfaceCount_001 enter");
    std::vector<UsbInterface> interfaces;
    for (uint32_t i = 0; i < USB_MODEL_INTERFACE_MAX_NUM; ++i) {
        interfaces.push_back(MakeModelInterface({}));
    }
    USBConfig config = MakeModelConfig(interfaces);
    std::unique_ptr<USBConfig> out = RoundTripModelConfig(config);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetInterfaceCount(), USB_MODEL_INTERFACE_MAX_NUM);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_MaxInterfaceCount_001 end");
}

/*
 * A USBConfig carrying more interfaces than the cap still writes the true
 * count into the parcel, but only the first cap of interfaces is serialized.
 * Unmarshalling therefore restores exactly the capped number.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_OverCapInterfaceCount_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_OverCapInterfaceCount_001 enter");
    std::vector<UsbInterface> interfaces;
    for (uint32_t i = 0; i < USB_MODEL_INTERFACE_MAX_NUM + 10; ++i) {
        interfaces.push_back(MakeModelInterface({}));
    }
    USBConfig config = MakeModelConfig(interfaces);
    EXPECT_EQ(config.GetInterfaceCount(), USB_MODEL_INTERFACE_MAX_NUM + 10);
    std::unique_ptr<USBConfig> out = RoundTripModelConfig(config);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetInterfaceCount(), USB_MODEL_INTERFACE_MAX_NUM);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_OverCapInterfaceCount_001 end");
}

/*
 * The default-constructed USBConfig carries a negative id and power value,
 * signalling "not set".
 */
HWTEST_F(UsbModelParcelTest, USBConfig_DefaultCtor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_DefaultCtor_001 enter");
    USBConfig config;
    EXPECT_EQ(config.GetId(), INVALID_USB_INT_VALUE);
    EXPECT_EQ(config.GetMaxPower(), INVALID_USB_INT_VALUE * 2);
    EXPECT_EQ(config.GetName(), "");
    EXPECT_EQ(config.GetInterfaceCount(), 0u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_DefaultCtor_001 end");
}

/*
 * An empty parcel yields a valid USBConfig object holding default scalar
 * values and no interfaces.
 */
HWTEST_F(UsbModelParcelTest, USBConfig_UnmarshalEmptyParcel_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_UnmarshalEmptyParcel_001 enter");
    Parcel parcel;
    std::unique_ptr<USBConfig> out(USBConfig::Unmarshalling(parcel));
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetId(), 0);
    EXPECT_EQ(out->GetName(), "");
    EXPECT_EQ(out->GetInterfaceCount(), 0u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBConfig_UnmarshalEmptyParcel_001 end");
}

/*
 * The USBEndpoint serializes address, attributes, interval, maxPacketSize and
 * interfaceId. All five fields must survive the round-trip.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_ParcelRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelRoundTrip_001 enter");
    USBEndpoint ep = MakeModelEndpoint();
    std::unique_ptr<USBEndpoint> out = RoundTripModelEndpoint(ep);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetAddress(), MODEL_EP_ADDR_BULK_IN);
    EXPECT_EQ(out->GetAttributes(), 2u);
    EXPECT_EQ(out->GetInterval(), 0);
    EXPECT_EQ(out->GetMaxPacketSize(), 64);
    EXPECT_EQ(static_cast<uint8_t>(out->GetInterfaceId()), 3u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelRoundTrip_001 end");
}

/*
 * A default-constructed endpoint keeps its defaults through the parcel
 * round-trip: zero addresses and negative interval / packet size.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_ParcelDefaultRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelDefaultRoundTrip_001 enter");
    USBEndpoint ep;
    std::unique_ptr<USBEndpoint> out = RoundTripModelEndpoint(ep);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetAddress(), 0u);
    EXPECT_EQ(out->GetAttributes(), 0u);
    EXPECT_EQ(out->GetInterval(), INVALID_USB_INT_VALUE);
    EXPECT_EQ(out->GetMaxPacketSize(), INVALID_USB_INT_VALUE);
    EXPECT_EQ(out->GetInterfaceId(), -1);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelDefaultRoundTrip_001 end");
}

/*
 * Non-zero interval and max packet size values must survive the round-trip.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_ParcelValues_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelValues_001 enter");
    USBEndpoint ep(0x0A, 3, 255, 1024);
    std::unique_ptr<USBEndpoint> out = RoundTripModelEndpoint(ep);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetAddress(), 0x0Au);
    EXPECT_EQ(out->GetAttributes(), 3u);
    EXPECT_EQ(out->GetInterval(), 255);
    EXPECT_EQ(out->GetMaxPacketSize(), 1024);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelValues_001 end");
}

/*
 * A negative interval encoded into the parcel must come back unchanged.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_ParcelNegativeInterval_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelNegativeInterval_001 enter");
    USBEndpoint ep(0x01, 0, -1, 64);
    std::unique_ptr<USBEndpoint> out = RoundTripModelEndpoint(ep);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetInterval(), -1);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelNegativeInterval_001 end");
}

/*
 * SetInterfaceId() accepts the full uint8_t range; both extremes must survive
 * the parcel round-trip.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_ParcelInterfaceIdBoundaries_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelInterfaceIdBoundaries_001 enter");
    USBEndpoint zero(0x01, 0, 0, 0);
    zero.SetInterfaceId(0);
    std::unique_ptr<USBEndpoint> outZero = RoundTripModelEndpoint(zero);
    ASSERT_NE(outZero, nullptr);
    EXPECT_EQ(static_cast<uint8_t>(outZero->GetInterfaceId()), 0u);

    USBEndpoint max(0x01, 0, 0, 0);
    max.SetInterfaceId(MODEL_INTERFACE_ID_MAX);
    std::unique_ptr<USBEndpoint> outMax = RoundTripModelEndpoint(max);
    ASSERT_NE(outMax, nullptr);
    EXPECT_EQ(static_cast<uint8_t>(outMax->GetInterfaceId()), MODEL_INTERFACE_ID_MAX);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelInterfaceIdBoundaries_001 end");
}

/*
 * GetType() must extract only the two low bits of the attributes field.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_GetTypeMask_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_GetTypeMask_001 enter");
    USBEndpoint control(0x01, 0, 0, 0);
    EXPECT_EQ(control.GetType(), 0u);
    USBEndpoint isoc(0x01, 1, 0, 0);
    EXPECT_EQ(isoc.GetType(), 1u);
    USBEndpoint bulk(0x01, 2, 0, 0);
    EXPECT_EQ(bulk.GetType(), 2u);
    USBEndpoint interrupt(0x01, 3, 0, 0);
    EXPECT_EQ(interrupt.GetType(), 3u);
    USBEndpoint noisy(0x01, 0xFFFFFFFF, 0, 0);
    EXPECT_EQ(noisy.GetType(), MODEL_XFERTYPE_MASK);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_GetTypeMask_001 end");
}

/*
 * The transfer type derived from the attributes field must survive a parcel
 * round-trip.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_ParcelTypePreserved_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelTypePreserved_001 enter");
    USBEndpoint ep(0x81, 2, 0, 64);
    std::unique_ptr<USBEndpoint> out = RoundTripModelEndpoint(ep);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetType(), 2u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ParcelTypePreserved_001 end");
}

/*
 * GetEndpointNumber() must extract only the low nibble of the address.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_GetEndpointNumberMask_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_GetEndpointNumberMask_001 enter");
    USBEndpoint ep(0x8F, 0, 0, 0);
    EXPECT_EQ(ep.GetEndpointNumber(), MODEL_NUMBER_MASK);
    EXPECT_EQ(ep.GetNumber(), static_cast<uint8_t>(0x0F));
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_GetEndpointNumberMask_001 end");
}

/*
 * The json constructor must read address/attributes/interval/maxPacketSize
 * and interfaceId from a well-formed endpoint object.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_JsonCtor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_JsonCtor_001 enter");
    cJSON *json = cJSON_Parse("{\"address\":130,\"attributes\":2,\"interval\":0,"
        "\"maxPacketSize\":64,\"interfaceId\":7}");
    ASSERT_NE(json, nullptr);
    USBEndpoint ep(json);
    cJSON_Delete(json);
    EXPECT_EQ(ep.GetAddress(), 0x82u);
    EXPECT_EQ(ep.GetAttributes(), 2u);
    EXPECT_EQ(ep.GetInterval(), 0);
    EXPECT_EQ(ep.GetMaxPacketSize(), 64);
    EXPECT_EQ(static_cast<uint8_t>(ep.GetInterfaceId()), 7u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_JsonCtor_001 end");
}

/*
 * The json constructor tolerates missing fields and returns zero defaults for
 * number fields and empty strings for string fields.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_JsonCtorEmptyObject_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_JsonCtorEmptyObject_001 enter");
    cJSON *json = cJSON_Parse("{}");
    ASSERT_NE(json, nullptr);
    USBEndpoint ep(json);
    cJSON_Delete(json);
    EXPECT_EQ(ep.GetAddress(), 0u);
    EXPECT_EQ(ep.GetAttributes(), 0u);
    EXPECT_EQ(ep.GetInterval(), 0);
    EXPECT_EQ(ep.GetMaxPacketSize(), 0);
    EXPECT_EQ(static_cast<uint8_t>(ep.GetInterfaceId()), 0u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_JsonCtorEmptyObject_001 end");
}

/*
 * The json constructor with a null pointer must not crash and must default
 * every field.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_JsonCtorNull_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_JsonCtorNull_001 enter");
    USBEndpoint ep(nullptr);
    EXPECT_EQ(ep.GetAddress(), 0u);
    EXPECT_EQ(ep.GetAttributes(), 0u);
    EXPECT_EQ(ep.GetInterval(), 0);
    EXPECT_EQ(ep.GetMaxPacketSize(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_JsonCtorNull_001 end");
}

/*
 * getJsonString() -> cJSON_Parse() -> USBEndpoint(cJSON*) reproduces every
 * serialized field, including the interfaceId.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_JsonRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_JsonRoundTrip_001 enter");
    USBEndpoint ep = MakeModelEndpoint();
    USBEndpoint reconstructed = ReconstructModelEndpointFromJson(ep);
    EXPECT_EQ(reconstructed.GetAddress(), MODEL_EP_ADDR_BULK_IN);
    EXPECT_EQ(reconstructed.GetAttributes(), 2u);
    EXPECT_EQ(reconstructed.GetInterval(), 0);
    EXPECT_EQ(reconstructed.GetMaxPacketSize(), 64);
    EXPECT_EQ(static_cast<uint8_t>(reconstructed.GetInterfaceId()), 3u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_JsonRoundTrip_001 end");
}

/*
 * The json string of an endpoint must include direction, number and type
 * helper fields for consumers of the generic USB device broadcast.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_JsonStringKeys_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_JsonStringKeys_001 enter");
    USBEndpoint ep(MODEL_EP_ADDR_BULK_IN, 2, 0, 64);
    std::string text = ep.getJsonString();
    ASSERT_NE(text.find("\"direction\""), std::string::npos);
    ASSERT_NE(text.find("\"number\""), std::string::npos);
    ASSERT_NE(text.find("\"type\""), std::string::npos);
    ASSERT_NE(text.find("\"interfaceId\""), std::string::npos);
    cJSON *json = ParseModelJson(text);
    ASSERT_NE(json, nullptr);
    cJSON *direction = cJSON_GetObjectItem(json, "direction");
    ASSERT_NE(direction, nullptr);
    EXPECT_EQ(direction->valueint, static_cast<int>(MODEL_DIR_MASK));
    cJSON_Delete(json);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_JsonStringKeys_001 end");
}

/*
 * ToString() outputs the address, direction and type in a stable format.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_ToString_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ToString_001 enter");
    USBEndpoint ep(MODEL_EP_ADDR_BULK_IN, 2, 0, 64);
    std::string text = ep.ToString();
    EXPECT_NE(text.find("USBEndpoint:[Address:130"), std::string::npos);
    EXPECT_NE(text.find("Type:2"), std::string::npos);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_ToString_001 end");
}

/*
 * An empty parcel yields a valid USBEndpoint object holding zero defaults
 * rather than crashing.
 */
HWTEST_F(UsbModelParcelTest, USBEndpoint_UnmarshalEmptyParcel_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_UnmarshalEmptyParcel_001 enter");
    Parcel parcel;
    std::unique_ptr<USBEndpoint> out(USBEndpoint::Unmarshalling(parcel));
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetAddress(), 0u);
    EXPECT_EQ(out->GetAttributes(), 0u);
    USB_HILOGI(MODULE_USB_SERVICE, "USBEndpoint_UnmarshalEmptyParcel_001 end");
}

/*
 * The UsbInterface serializes id, protocol, class, subclass, alternateSetting,
 * name, iInterface and the endpoint array. All must survive the round-trip.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_ParcelRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_ParcelRoundTrip_001 enter");
    UsbInterface intf(2, 0, 8, 6, 1, "Storage", {});
    intf.SetiInterface(3);
    std::unique_ptr<UsbInterface> out = RoundTripModelInterface(intf);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetId(), 2);
    EXPECT_EQ(out->GetProtocol(), 0);
    EXPECT_EQ(out->GetClass(), 8);
    EXPECT_EQ(out->GetSubClass(), 6);
    EXPECT_EQ(out->GetAlternateSetting(), 1);
    EXPECT_EQ(out->GetName(), "Storage");
    EXPECT_EQ(out->GetiInterface(), 3u);
    EXPECT_EQ(out->GetEndpointCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_ParcelRoundTrip_001 end");
}

/*
 * A default-constructed interface keeps its sentinel defaults through the
 * parcel round-trip.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_ParcelDefaultRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_ParcelDefaultRoundTrip_001 enter");
    UsbInterface intf;
    std::unique_ptr<UsbInterface> out = RoundTripModelInterface(intf);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetId(), INT32_MAX);
    EXPECT_EQ(out->GetName(), "");
    EXPECT_EQ(out->GetEndpointCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_ParcelDefaultRoundTrip_001 end");
}

/*
 * Endpoints contained in an interface keep every endpoint field through the
 * nested interface round-trip.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_ParcelEndpoints_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_ParcelEndpoints_001 enter");
    std::vector<USBEndpoint> endpoints;
    endpoints.push_back(MakeModelEndpoint());
    endpoints.push_back(USBEndpoint(0x01, 2, 0, 8));
    UsbInterface intf(0, 0, 8, 6, 0, "Bulk", endpoints);
    std::unique_ptr<UsbInterface> out = RoundTripModelInterface(intf);
    ASSERT_NE(out, nullptr);
    ASSERT_EQ(out->GetEndpointCount(), 2);
    std::optional<USBEndpoint> first = out->GetEndpoint(0);
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->GetAddress(), MODEL_EP_ADDR_BULK_IN);
    EXPECT_EQ(static_cast<uint8_t>(first->GetInterfaceId()), 3u);
    std::optional<USBEndpoint> second = out->GetEndpoint(1);
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->GetAddress(), 0x01u);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_ParcelEndpoints_001 end");
}

/*
 * GetEndpoint() must return nullopt for indexes equal to or beyond the
 * endpoint count.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_GetEndpointOutOfRange_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_GetEndpointOutOfRange_001 enter");
    UsbInterface intf = MakeModelInterface({MakeModelEndpoint()});
    EXPECT_FALSE(intf.GetEndpoint(1).has_value());
    EXPECT_FALSE(intf.GetEndpoint(100).has_value());
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_GetEndpointOutOfRange_001 end");
}

/*
 * The json constructor must restore id/protocol/class/subclass/alternateSetting
 * and the endpoint list.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_JsonCtor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_JsonCtor_001 enter");
    cJSON *json = cJSON_Parse("{\"id\":5,\"protocol\":0,\"clazz\":8,\"subClass\":6,"
        "\"alternateSetting\":2,\"name\":\"Cdc\"}");
    ASSERT_NE(json, nullptr);
    UsbInterface intf(json);
    cJSON_Delete(json);
    EXPECT_EQ(intf.GetId(), 5);
    EXPECT_EQ(intf.GetProtocol(), 0);
    EXPECT_EQ(intf.GetClass(), 8);
    EXPECT_EQ(intf.GetSubClass(), 6);
    EXPECT_EQ(intf.GetAlternateSetting(), 2);
    EXPECT_EQ(intf.GetName(), "Cdc");
    EXPECT_EQ(intf.GetEndpointCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_JsonCtor_001 end");
}

/*
 * As in the other model classes, the json constructor must tolerate a missing
 * endpoints array and produce zero endpoints.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_JsonCtorMissingEndpoints_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_JsonCtorMissingEndpoints_001 enter");
    cJSON *json = cJSON_Parse("{\"id\":9,\"name\":\"Raw\"}");
    ASSERT_NE(json, nullptr);
    UsbInterface intf(json);
    cJSON_Delete(json);
    EXPECT_EQ(intf.GetId(), 9);
    EXPECT_EQ(intf.GetName(), "Raw");
    EXPECT_EQ(intf.GetEndpointCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_JsonCtorMissingEndpoints_001 end");
}

/*
 * The json constructor with a null pointer defaults every field to zero/empty.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_JsonCtorNull_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_JsonCtorNull_001 enter");
    UsbInterface intf(nullptr);
    EXPECT_EQ(intf.GetId(), 0);
    EXPECT_EQ(intf.GetName(), "");
    EXPECT_EQ(intf.GetEndpointCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_JsonCtorNull_001 end");
}

/*
 * getJsonString() -> cJSON_Parse() -> UsbInterface(cJSON*) reproduces all
 * scalar fields and the endpoint list.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_JsonRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_JsonRoundTrip_001 enter");
    UsbInterface intf = MakeModelInterface({MakeModelEndpoint()});
    UsbInterface reconstructed = ReconstructModelInterfaceFromJson(intf);
    EXPECT_EQ(reconstructed.GetId(), 0);
    EXPECT_EQ(reconstructed.GetName(), "Mass Storage");
    EXPECT_EQ(reconstructed.GetClass(), 8);
    EXPECT_EQ(reconstructed.GetSubClass(), 6);
    EXPECT_EQ(reconstructed.GetEndpointCount(), 1);
    std::optional<USBEndpoint> ep = reconstructed.GetEndpoint(0);
    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ(ep->GetAddress(), MODEL_EP_ADDR_BULK_IN);
    EXPECT_EQ(static_cast<uint8_t>(ep->GetInterfaceId()), 3u);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_JsonRoundTrip_001 end");
}

/*
 * The iInterface descriptor index is not part of the JSON payload, so the
 * reconstructed interface falls back to its sentinel UINT8_MAX value.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_JsonRoundTripIIinterface_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_JsonRoundTripIIinterface_001 enter");
    UsbInterface intf = MakeModelInterface({});
    intf.SetiInterface(2);
    UsbInterface reconstructed = ReconstructModelInterfaceFromJson(intf);
    EXPECT_EQ(reconstructed.GetiInterface(), MODEL_I_INTERFACE_UNSET);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_JsonRoundTripIIinterface_001 end");
}

/*
 * An interface with exactly USB_ENDPOINT_MAX_NUM endpoints marshals and
 * unmarshals without losing any endpoint.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_MaxEndpointCount_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_MaxEndpointCount_001 enter");
    std::vector<USBEndpoint> endpoints;
    for (uint32_t i = 0; i < USB_MODEL_ENDPOINT_MAX_NUM; ++i) {
        endpoints.push_back(USBEndpoint(i, 2, 0, 64));
    }
    UsbInterface intf = MakeModelInterface(endpoints);
    std::unique_ptr<UsbInterface> out = RoundTripModelInterface(intf);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetEndpointCount(), static_cast<int32_t>(USB_MODEL_ENDPOINT_MAX_NUM));
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_MaxEndpointCount_001 end");
}

/*
 * An interface with more than the endpoint cap restores only the capped
 * number of endpoints, mirroring the USBConfig behavior.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_OverCapEndpointCount_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_OverCapEndpointCount_001 enter");
    std::vector<USBEndpoint> endpoints;
    for (uint32_t i = 0; i < USB_MODEL_ENDPOINT_MAX_NUM + 10; ++i) {
        endpoints.push_back(USBEndpoint(i, 2, 0, 64));
    }
    UsbInterface intf = MakeModelInterface(endpoints);
    EXPECT_EQ(intf.GetEndpointCount(), static_cast<int32_t>(USB_MODEL_ENDPOINT_MAX_NUM + 10));
    std::unique_ptr<UsbInterface> out = RoundTripModelInterface(intf);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetEndpointCount(), static_cast<int32_t>(USB_MODEL_ENDPOINT_MAX_NUM));
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_OverCapEndpointCount_001 end");
}

/*
 * ToString() mentions the interface name and class so dump logs stay useful.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_ToString_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_ToString_001 enter");
    UsbInterface intf = MakeModelInterface({});
    std::string text = intf.ToString();
    EXPECT_NE(text.find("UsbInterface["), std::string::npos);
    EXPECT_NE(text.find("name_=Mass Storage"), std::string::npos);
    EXPECT_NE(text.find("klass_=8"), std::string::npos);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_ToString_001 end");
}

/*
 * An empty parcel yields a valid UsbInterface object with zero values and no
 * endpoints instead of crashing.
 */
HWTEST_F(UsbModelParcelTest, UsbInterface_UnmarshalEmptyParcel_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_UnmarshalEmptyParcel_001 enter");
    Parcel parcel;
    std::unique_ptr<UsbInterface> out(UsbInterface::Unmarshalling(parcel));
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetId(), 0);
    EXPECT_EQ(out->GetName(), "");
    EXPECT_EQ(out->GetEndpointCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbInterface_UnmarshalEmptyParcel_001 end");
}

/*
 * Every scalar field of UsbDevice must survive the parcel round-trip,
 * including the descriptor-level fields that are not part of the JSON path.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_ParcelRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelRoundTrip_001 enter");
    UsbDevice device = MakeModelDevice({});
    std::unique_ptr<UsbDevice> out = RoundTripModelDevice(device);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetName(), MODEL_NAME);
    EXPECT_EQ(out->GetManufacturerName(), MODEL_MFG);
    EXPECT_EQ(out->GetProductName(), MODEL_PRODUCT);
    EXPECT_EQ(out->GetVersion(), MODEL_VERSION);
    EXPECT_EQ(out->GetmSerial(), MODEL_SERIAL);
    EXPECT_EQ(out->GetBusNum(), 1u);
    EXPECT_EQ(out->GetDevAddr(), 2u);
    EXPECT_EQ(out->GetVendorId(), MODEL_VENDOR_ID);
    EXPECT_EQ(out->GetProductId(), MODEL_PRODUCT_ID);
    EXPECT_EQ(out->GetClass(), 0);
    EXPECT_EQ(out->GetSubclass(), 0);
    EXPECT_EQ(out->GetProtocol(), 0);
    EXPECT_EQ(out->GetiManufacturer(), 1u);
    EXPECT_EQ(out->GetiProduct(), 2u);
    EXPECT_EQ(out->GetiSerialNumber(), 3u);
    EXPECT_EQ(out->GetbMaxPacketSize0(), 64u);
    EXPECT_EQ(out->GetbcdUSB(), 0x0200u);
    EXPECT_EQ(out->GetbcdDevice(), 0x0100u);
    EXPECT_EQ(out->GetConfigCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelRoundTrip_001 end");
}

/*
 * The authorize status is not marshalled into the parcel, so a round-trip
 * keeps the default ENABLED value rather than the caller-side value.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_ParcelAuthorizeNotSerialized_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelAuthorizeNotSerialized_001 enter");
    UsbDevice device = MakeModelDevice({});
    device.SetAuthorizeStatus(DISABLED);
    std::unique_ptr<UsbDevice> out = RoundTripModelDevice(device);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(device.GetAuthorizeStatus(), DISABLED);
    EXPECT_EQ(out->GetAuthorizeStatus(), ENABLED);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelAuthorizeNotSerialized_001 end");
}

/*
 * GetConfig() must return ERR_INVALID_VALUE for out-of-range indexes.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_GetConfigOutOfRange_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_GetConfigOutOfRange_001 enter");
    UsbDevice device = MakeModelDevice({});
    USBConfig config;
    EXPECT_EQ(device.GetConfig(0, config), ERR_INVALID_VALUE);
    EXPECT_EQ(device.GetConfig(100, config), ERR_INVALID_VALUE);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_GetConfigOutOfRange_001 end");
}

/*
 * A device with several configs must expose them all after the parcel
 * round-trip, including nested interfaces and endpoints.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_ParcelNestedConfigs_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelNestedConfigs_001 enter");
    std::vector<UsbInterface> interfaces;
    interfaces.push_back(MakeModelInterface({MakeModelEndpoint()}));
    USBConfig first = MakeModelConfig(interfaces);
    USBConfig second = MakeModelConfig({});
    second.SetId(2);
    std::vector<USBConfig> configs;
    configs.push_back(first);
    configs.push_back(second);
    UsbDevice device = MakeModelDevice(configs);
    std::unique_ptr<UsbDevice> out = RoundTripModelDevice(device);
    ASSERT_NE(out, nullptr);
    ASSERT_EQ(out->GetConfigCount(), 2);
    USBConfig restored;
    ASSERT_EQ(out->GetConfig(0, restored), ERR_OK);
    EXPECT_EQ(restored.GetId(), 1);
    ASSERT_EQ(restored.GetInterfaceCount(), 1u);
    UsbInterface restoredIf;
    ASSERT_TRUE(restored.GetInterface(0, restoredIf));
    EXPECT_EQ(restoredIf.GetEndpointCount(), 1);
    ASSERT_EQ(out->GetConfig(1, restored), ERR_OK);
    EXPECT_EQ(restored.GetId(), 2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelNestedConfigs_001 end");
}

/*
 * A device with no configs marshals an empty config array and restores it as
 * zero configs.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_ParcelNoConfigs_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelNoConfigs_001 enter");
    UsbDevice device = MakeModelDevice({});
    std::unique_ptr<UsbDevice> out = RoundTripModelDevice(device);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetConfigCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelNoConfigs_001 end");
}

/*
 * Empty string fields must not break the parcel round-trip.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_ParcelEmptyStrings_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelEmptyStrings_001 enter");
    UsbDevice device("", "", "", "", 0, 0, 0, 0, 0, 0, 0, {});
    std::unique_ptr<UsbDevice> out = RoundTripModelDevice(device);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetName(), "");
    EXPECT_EQ(out->GetManufacturerName(), "");
    EXPECT_EQ(out->GetProductName(), "");
    EXPECT_EQ(out->GetVersion(), "");
    EXPECT_EQ(out->GetmSerial(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelEmptyStrings_001 end");
}

/*
 * A long ASCII name string must survive the parcel round-trip byte for byte.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_ParcelLongString_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelLongString_001 enter");
    std::string longName;
    const std::string chunk = "usb-parcel-round-trip-";
    for (int32_t i = 0; i < 200; ++i) {
        longName.append(chunk);
    }
    UsbDevice device(longName, MODEL_MFG, MODEL_PRODUCT, MODEL_VERSION,
        1, 1, 0, 0, 0, 0, 0, {});
    std::unique_ptr<UsbDevice> out = RoundTripModelDevice(device);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetName(), longName);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ParcelLongString_001 end");
}

/*
 * A device with more than the config cap keeps only the first capped configs
 * after unmarshalling, consistent with the marshalling-side cutoff.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_OverCapConfigCount_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_OverCapConfigCount_001 enter");
    std::vector<USBConfig> configs;
    for (uint32_t i = 0; i < USB_MODEL_CONFIG_MAX_NUM + 10; ++i) {
        USBConfig config = MakeModelConfig({});
        config.SetId(static_cast<int32_t>(i));
        configs.push_back(config);
    }
    UsbDevice device = MakeModelDevice(configs);
    EXPECT_EQ(device.GetConfigCount(), static_cast<int32_t>(USB_MODEL_CONFIG_MAX_NUM + 10));
    std::unique_ptr<UsbDevice> out = RoundTripModelDevice(device);
    ASSERT_NE(out, nullptr);
    ASSERT_EQ(out->GetConfigCount(), static_cast<int32_t>(USB_MODEL_CONFIG_MAX_NUM));
    USBConfig first;
    ASSERT_EQ(out->GetConfig(0, first), ERR_OK);
    EXPECT_EQ(first.GetId(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_OverCapConfigCount_001 end");
}

/*
 * The json constructor must restore every scalar field emitted by
 * getJsonString(), and drop the internal descriptor-level fields.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_JsonCtor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_JsonCtor_001 enter");
    cJSON *json = cJSON_Parse("{\"busNum\":1,\"devAddress\":2,\"name\":\"demo\","
        "\"manufacturerName\":\"mfg\",\"productName\":\"prod\",\"version\":\"9.9\","
        "\"vendorId\":4096,\"productId\":257,\"clazz\":3,\"subClass\":1,\"protocol\":2}");
    ASSERT_NE(json, nullptr);
    UsbDevice device(json);
    cJSON_Delete(json);
    EXPECT_EQ(device.GetBusNum(), 1u);
    EXPECT_EQ(device.GetDevAddr(), 2u);
    EXPECT_EQ(device.GetName(), "demo");
    EXPECT_EQ(device.GetManufacturerName(), "mfg");
    EXPECT_EQ(device.GetProductName(), "prod");
    EXPECT_EQ(device.GetVersion(), "9.9");
    EXPECT_EQ(device.GetVendorId(), 4096);
    EXPECT_EQ(device.GetProductId(), 257);
    EXPECT_EQ(device.GetClass(), 3);
    EXPECT_EQ(device.GetSubclass(), 1);
    EXPECT_EQ(device.GetProtocol(), 2);
    EXPECT_EQ(device.GetConfigCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_JsonCtor_001 end");
}

/*
 * getJsonString() -> cJSON_Parse() -> UsbDevice(cJSON*) reproduces all of the
 * public fields, including the nested config/interface/endpoint hierarchy.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_JsonRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_JsonRoundTrip_001 enter");
    USBConfig config = MakeModelConfig({MakeModelInterface({MakeModelEndpoint()})});
    UsbDevice device = MakeModelDevice({config});
    UsbDevice reconstructed = ReconstructModelDeviceFromJson(device);
    EXPECT_EQ(reconstructed.GetName(), MODEL_NAME);
    EXPECT_EQ(reconstructed.GetManufacturerName(), MODEL_MFG);
    EXPECT_EQ(reconstructed.GetProductName(), MODEL_PRODUCT);
    EXPECT_EQ(reconstructed.GetVersion(), MODEL_VERSION);
    EXPECT_EQ(reconstructed.GetBusNum(), 1u);
    EXPECT_EQ(reconstructed.GetDevAddr(), 2u);
    EXPECT_EQ(reconstructed.GetVendorId(), MODEL_VENDOR_ID);
    EXPECT_EQ(reconstructed.GetProductId(), MODEL_PRODUCT_ID);
    ASSERT_EQ(reconstructed.GetConfigCount(), 1);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_JsonRoundTrip_001 end");
}

/*
 * Descriptor-level fields are intentionally absent from the JSON payload and
 * therefore fall back to their defaults after a JSON round-trip.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_JsonRoundTripDropsDescriptorFields_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_JsonRoundTripDropsDescriptorFields_001 enter");
    UsbDevice device = MakeModelDevice({});
    UsbDevice reconstructed = ReconstructModelDeviceFromJson(device);
    EXPECT_EQ(reconstructed.GetiManufacturer(), MODEL_I_INTERFACE_UNSET);
    EXPECT_EQ(reconstructed.GetiProduct(), MODEL_I_INTERFACE_UNSET);
    EXPECT_EQ(reconstructed.GetiSerialNumber(), MODEL_I_INTERFACE_UNSET);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_JsonRoundTripDropsDescriptorFields_001 end");
}

/*
 * A device JSON without a configs array is tolerated and produces no configs.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_JsonMissingConfigs_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_JsonMissingConfigs_001 enter");
    cJSON *json = cJSON_Parse("{\"name\":\"single\"}");
    ASSERT_NE(json, nullptr);
    UsbDevice device(json);
    cJSON_Delete(json);
    EXPECT_EQ(device.GetName(), "single");
    EXPECT_EQ(device.GetConfigCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_JsonMissingConfigs_001 end");
}

/*
 * The json constructor with a null pointer defaults every scalar field.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_JsonCtorNull_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_JsonCtorNull_001 enter");
    UsbDevice device(nullptr);
    EXPECT_EQ(device.GetName(), "");
    EXPECT_EQ(device.GetVendorId(), 0);
    EXPECT_EQ(device.GetConfigCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_JsonCtorNull_001 end");
}

/*
 * GetIntValue() returns the stored integer when present, and zero when the
 * key is missing, wrong-typed, or the object is null.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_GetIntValue_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_GetIntValue_001 enter");
    cJSON *json = cJSON_Parse("{\"present\":42,\"asString\":\"42\"}");
    ASSERT_NE(json, nullptr);
    EXPECT_EQ(UsbDevice::GetIntValue(json, "present"), 42);
    EXPECT_EQ(UsbDevice::GetIntValue(json, "asString"), 0);
    EXPECT_EQ(UsbDevice::GetIntValue(json, "missing"), 0);
    EXPECT_EQ(UsbDevice::GetIntValue(nullptr, "present"), 0);
    cJSON_Delete(json);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_GetIntValue_001 end");
}

/*
 * GetStringValue() returns the stored string when present, and an empty
 * string when the key is missing, wrong-typed, or the object is null.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_GetStringValue_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_GetStringValue_001 enter");
    cJSON *json = cJSON_Parse("{\"present\":\"ok\",\"asNumber\":7}");
    ASSERT_NE(json, nullptr);
    EXPECT_EQ(UsbDevice::GetStringValue(json, "present"), "ok");
    EXPECT_EQ(UsbDevice::GetStringValue(json, "asNumber"), "");
    EXPECT_EQ(UsbDevice::GetStringValue(json, "missing"), "");
    EXPECT_EQ(UsbDevice::GetStringValue(nullptr, "present"), "");
    cJSON_Delete(json);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_GetStringValue_001 end");
}

/*
 * Setting a config vector through SetConfigs() must be visible through
 * GetConfigs() and GetConfigCount().
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_SetConfigs_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetConfigs_001 enter");
    std::vector<USBConfig> configs;
    configs.push_back(MakeModelConfig({}));
    configs.push_back(MakeModelConfig({}));
    UsbDevice device;
    device.SetConfigs(configs);
    EXPECT_EQ(device.GetConfigCount(), 2);
    EXPECT_EQ(device.GetConfigs().size(), 2u);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_SetConfigs_001 end");
}

/*
 * ToString() must include the name and the descriptor triple so logs can be
 * correlated with a specific device.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_ToString_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ToString_001 enter");
    UsbDevice device = MakeModelDevice({});
    std::string text = device.ToString();
    EXPECT_NE(text.find("UsbDevice["), std::string::npos);
    EXPECT_NE(text.find("name_=usb_model_parcel_test"), std::string::npos);
    EXPECT_NE(text.find("busNum_=1"), std::string::npos);
    EXPECT_NE(text.find("vendorId_="), std::string::npos);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_ToString_001 end");
}

/*
 * An empty parcel yields a valid UsbDevice with default values rather than
 * crashing: numeric reads fail soft to zero and string reads to empty.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_UnmarshalEmptyParcel_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_UnmarshalEmptyParcel_001 enter");
    Parcel parcel;
    std::unique_ptr<UsbDevice> out(UsbDevice::Unmarshalling(parcel));
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetName(), "");
    EXPECT_EQ(out->GetBusNum(), 0u);
    EXPECT_EQ(out->GetVendorId(), 0);
    EXPECT_EQ(out->GetConfigCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_UnmarshalEmptyParcel_001 end");
}

/*
 * A truncated parcel (only the first field written) must still yield a valid
 * UsbDevice; the remaining reads fail soft and the config list stays empty.
 */
HWTEST_F(UsbModelParcelTest, UsbDevice_UnmarshalTruncatedParcel_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_UnmarshalTruncatedParcel_001 enter");
    Parcel parcel;
    ASSERT_TRUE(parcel.WriteUint8(9));
    std::unique_ptr<UsbDevice> out(UsbDevice::Unmarshalling(parcel));
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetBusNum(), 9u);
    EXPECT_EQ(out->GetDevAddr(), 0u);
    EXPECT_EQ(out->GetName(), "");
    EXPECT_EQ(out->GetConfigCount(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevice_UnmarshalTruncatedParcel_001 end");
}

/*
 * The UsbPort struct must round-trip all five integer fields: id, supported
 * modes and the three port status values.
 */
HWTEST_F(UsbModelParcelTest, UsbPort_ParcelRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_ParcelRoundTrip_001 enter");
    UsbPort port;
    port.id = 2;
    port.supportedModes = 3;
    port.usbPortStatus.currentMode = 2;
    port.usbPortStatus.currentPowerRole = 1;
    port.usbPortStatus.currentDataRole = 2;
    std::unique_ptr<UsbPort> out = RoundTripModelPort(port);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->id, 2);
    EXPECT_EQ(out->supportedModes, 3);
    EXPECT_EQ(out->usbPortStatus.currentMode, 2);
    EXPECT_EQ(out->usbPortStatus.currentPowerRole, 1);
    EXPECT_EQ(out->usbPortStatus.currentDataRole, 2);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_ParcelRoundTrip_001 end");
}

/*
 * A port with zero/negative values must survive the round-trip unchanged.
 */
HWTEST_F(UsbModelParcelTest, UsbPort_ParcelExtremes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_ParcelExtremes_001 enter");
    UsbPort port;
    port.id = -1;
    port.supportedModes = 0;
    port.usbPortStatus.currentMode = -1;
    port.usbPortStatus.currentPowerRole = 7;
    port.usbPortStatus.currentDataRole = -7;
    std::unique_ptr<UsbPort> out = RoundTripModelPort(port);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->id, -1);
    EXPECT_EQ(out->supportedModes, 0);
    EXPECT_EQ(out->usbPortStatus.currentMode, -1);
    EXPECT_EQ(out->usbPortStatus.currentPowerRole, 7);
    EXPECT_EQ(out->usbPortStatus.currentDataRole, -7);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_ParcelExtremes_001 end");
}

/*
 * An empty parcel yields a valid UsbPort struct with all-zero fields.
 */
HWTEST_F(UsbModelParcelTest, UsbPort_UnmarshalEmptyParcel_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_UnmarshalEmptyParcel_001 enter");
    Parcel parcel;
    std::unique_ptr<UsbPort> out(UsbPort::Unmarshalling(parcel));
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->id, 0);
    EXPECT_EQ(out->supportedModes, 0);
    EXPECT_EQ(out->usbPortStatus.currentMode, 0);
    EXPECT_EQ(out->usbPortStatus.currentPowerRole, 0);
    EXPECT_EQ(out->usbPortStatus.currentDataRole, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPort_UnmarshalEmptyParcel_001 end");
}

/*
 * The USBAccessory must round-trip all five string fields through the parcel.
 */
HWTEST_F(UsbModelParcelTest, USBAccessory_ParcelRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_ParcelRoundTrip_001 enter");
    USBAccessory accessory("mfg", "prod", "desc", "1.0", "SN001");
    std::unique_ptr<USBAccessory> out = RoundTripModelAccessory(accessory);
    ASSERT_NE(out, nullptr);
    ExpectModelAccessoryEqual(accessory, *out);
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_ParcelRoundTrip_001 end");
}

/*
 * A default-constructed accessory keeps empty strings through the parcel
 * round-trip.
 */
HWTEST_F(UsbModelParcelTest, USBAccessory_ParcelDefault_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_ParcelDefault_001 enter");
    USBAccessory accessory;
    std::unique_ptr<USBAccessory> out = RoundTripModelAccessory(accessory);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetManufacturer(), "");
    EXPECT_EQ(out->GetProduct(), "");
    EXPECT_EQ(out->GetDescription(), "");
    EXPECT_EQ(out->GetVersion(), "");
    EXPECT_EQ(out->GetSerialNumber(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_ParcelDefault_001 end");
}

/*
 * The json constructor must restore every string field of an accessory.
 */
HWTEST_F(UsbModelParcelTest, USBAccessory_JsonCtor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_JsonCtor_001 enter");
    cJSON *json = cJSON_Parse("{\"manufacturer\":\"acc-mfg\",\"product\":\"acc-prod\","
        "\"description\":\"acc-desc\",\"version\":\"3.0\",\"serialNumber\":\"acc-sn\"}");
    ASSERT_NE(json, nullptr);
    USBAccessory accessory(json);
    cJSON_Delete(json);
    EXPECT_EQ(accessory.GetManufacturer(), "acc-mfg");
    EXPECT_EQ(accessory.GetProduct(), "acc-prod");
    EXPECT_EQ(accessory.GetDescription(), "acc-desc");
    EXPECT_EQ(accessory.GetVersion(), "3.0");
    EXPECT_EQ(accessory.GetSerialNumber(), "acc-sn");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_JsonCtor_001 end");
}

/*
 * The json constructor with a null pointer keeps all fields empty.
 */
HWTEST_F(UsbModelParcelTest, USBAccessory_JsonCtorNull_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_JsonCtorNull_001 enter");
    USBAccessory accessory(nullptr);
    EXPECT_EQ(accessory.GetManufacturer(), "");
    EXPECT_EQ(accessory.GetProduct(), "");
    EXPECT_EQ(accessory.GetDescription(), "");
    EXPECT_EQ(accessory.GetVersion(), "");
    EXPECT_EQ(accessory.GetSerialNumber(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_JsonCtorNull_001 end");
}

/*
 * GetJsonString() -> cJSON_Parse() -> USBAccessory(cJSON*) reproduces every
 * field.
 */
HWTEST_F(UsbModelParcelTest, USBAccessory_JsonRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_JsonRoundTrip_001 enter");
    USBAccessory accessory("mfg", "prod", "desc", "1.0", "SN001");
    USBAccessory reconstructed = ReconstructModelAccessoryFromJson(accessory);
    ExpectModelAccessoryEqual(accessory, reconstructed);
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_JsonRoundTrip_001 end");
}

/*
 * SetAccessory() fills all five fields from a five-element vector.
 */
HWTEST_F(UsbModelParcelTest, USBAccessory_SetAccessory_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetAccessory_001 enter");
    std::vector<std::string> values = {"m1", "p1", "d1", "v1", "s1"};
    USBAccessory accessory;
    accessory.SetAccessory(values);
    EXPECT_EQ(accessory.GetManufacturer(), "m1");
    EXPECT_EQ(accessory.GetProduct(), "p1");
    EXPECT_EQ(accessory.GetDescription(), "d1");
    EXPECT_EQ(accessory.GetVersion(), "v1");
    EXPECT_EQ(accessory.GetSerialNumber(), "s1");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetAccessory_001 end");
}

/*
 * A vector with fewer than ACC_SIZE elements is rejected and leaves the
 * accessory untouched.
 */
HWTEST_F(UsbModelParcelTest, USBAccessory_SetAccessoryTooShort_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetAccessoryTooShort_001 enter");
    std::vector<std::string> values = {"m1", "p1", "d1", "v1"};
    USBAccessory accessory;
    accessory.SetManufacturer("keep");
    accessory.SetAccessory(values);
    EXPECT_EQ(accessory.GetManufacturer(), "keep");
    EXPECT_EQ(accessory.GetSerialNumber(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_SetAccessoryTooShort_001 end");
}

/*
 * operator== must return true only when every field matches.
 */
HWTEST_F(UsbModelParcelTest, USBAccessory_Equality_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_Equality_001 enter");
    USBAccessory lhs("m", "p", "d", "v", "s");
    USBAccessory rhs("m", "p", "d", "v", "s");
    EXPECT_TRUE(lhs == rhs);
    rhs.SetSerialNumber("other");
    EXPECT_FALSE(lhs == rhs);
    USBAccessory empty;
    EXPECT_FALSE(lhs == empty);
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_Equality_001 end");
}

/*
 * ToString() must mention the manufacturer and serial number.
 */
HWTEST_F(UsbModelParcelTest, USBAccessory_ToString_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_ToString_001 enter");
    USBAccessory accessory("mfg", "prod", "desc", "1.0", "SN001");
    std::string text = accessory.ToString();
    EXPECT_NE(text.find("USBAccessory["), std::string::npos);
    EXPECT_NE(text.find("manufacturer=mfg"), std::string::npos);
    EXPECT_NE(text.find("serialNumber=SN001"), std::string::npos);
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_ToString_001 end");
}

/*
 * An empty parcel yields a valid USBAccessory with all-empty strings.
 */
HWTEST_F(UsbModelParcelTest, USBAccessory_UnmarshalEmptyParcel_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_UnmarshalEmptyParcel_001 enter");
    Parcel parcel;
    std::unique_ptr<USBAccessory> out(USBAccessory::Unmarshalling(parcel));
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->GetManufacturer(), "");
    EXPECT_EQ(out->GetSerialNumber(), "");
    USB_HILOGI(MODULE_USB_SERVICE, "USBAccessory_UnmarshalEmptyParcel_001 end");
}

/*
 * A full stack (endpoint -> interface -> config -> device) must survive both
 * the parcel and the JSON round-trip with identical nesting depths.
 */
HWTEST_F(UsbModelParcelTest, Model_FullStackParcelRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Model_FullStackParcelRoundTrip_001 enter");
    std::vector<USBEndpoint> endpoints;
    endpoints.push_back(MakeModelEndpoint());
    endpoints.push_back(USBEndpoint(0x01, 2, 0, 8));
    std::vector<UsbInterface> interfaces;
    interfaces.push_back(MakeModelInterface(endpoints));
    interfaces.push_back(MakeModelInterface({}));
    std::vector<USBConfig> configs;
    configs.push_back(MakeModelConfig(interfaces));
    configs.push_back(MakeModelConfig({}));
    UsbDevice device = MakeModelDevice(configs);
    std::unique_ptr<UsbDevice> out = RoundTripModelDevice(device);
    ASSERT_NE(out, nullptr);
    ASSERT_EQ(out->GetConfigCount(), 2);
    USBConfig first;
    ASSERT_EQ(out->GetConfig(0, first), ERR_OK);
    ASSERT_EQ(first.GetInterfaceCount(), 2u);
    UsbInterface bulkIf;
    ASSERT_TRUE(first.GetInterface(0, bulkIf));
    EXPECT_EQ(bulkIf.GetEndpointCount(), 2);
    EXPECT_EQ(out->GetName(), MODEL_NAME);
    USB_HILOGI(MODULE_USB_SERVICE, "Model_FullStackParcelRoundTrip_001 end");
}

/*
 * The same full stack must also survive the JSON path used by the device
 * broadcast events.
 */
HWTEST_F(UsbModelParcelTest, Model_FullStackJsonRoundTrip_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Model_FullStackJsonRoundTrip_001 enter");
    std::vector<USBEndpoint> endpoints;
    endpoints.push_back(MakeModelEndpoint());
    std::vector<UsbInterface> interfaces;
    interfaces.push_back(MakeModelInterface(endpoints));
    std::vector<USBConfig> configs;
    configs.push_back(MakeModelConfig(interfaces));
    UsbDevice device = MakeModelDevice(configs);
    UsbDevice reconstructed = ReconstructModelDeviceFromJson(device);
    ASSERT_EQ(reconstructed.GetConfigCount(), 1);
    USBConfig first;
    ASSERT_EQ(reconstructed.GetConfig(0, first), ERR_OK);
    ASSERT_EQ(first.GetInterfaceCount(), 1u);
    UsbInterface bulkIf;
    ASSERT_TRUE(first.GetInterface(0, bulkIf));
    ASSERT_EQ(bulkIf.GetEndpointCount(), 1);
    std::optional<USBEndpoint> ep = bulkIf.GetEndpoint(0);
    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ(ep->GetAddress(), MODEL_EP_ADDR_BULK_IN);
    USB_HILOGI(MODULE_USB_SERVICE, "Model_FullStackJsonRoundTrip_001 end");
}

/*
 * A stress round-trip (100 parcel + JSON cycles on the same hierarchy) must
 * not drift or crash.
 */
HWTEST_F(UsbModelParcelTest, Model_StressRoundTrip_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Model_StressRoundTrip_001 enter");
    std::vector<USBEndpoint> endpoints;
    endpoints.push_back(MakeModelEndpoint());
    std::vector<UsbInterface> interfaces;
    interfaces.push_back(MakeModelInterface(endpoints));
    std::vector<USBConfig> configs;
    configs.push_back(MakeModelConfig(interfaces));
    UsbDevice device = MakeModelDevice(configs);
    const int32_t loop = 100;
    for (int32_t i = 0; i < loop; ++i) {
        std::unique_ptr<UsbDevice> out = RoundTripModelDevice(device);
        ASSERT_NE(out, nullptr);
        ASSERT_EQ(out->GetConfigCount(), 1);
        USBConfig first;
        ASSERT_EQ(out->GetConfig(0, first), ERR_OK);
        ASSERT_EQ(first.GetInterfaceCount(), 1u);
        UsbInterface bulkIf;
        ASSERT_TRUE(first.GetInterface(0, bulkIf));
        EXPECT_EQ(bulkIf.GetEndpointCount(), 1);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Model_StressRoundTrip_001 end");
}

/*
 * A single endpoint address with the IN bit plus upper bits must keep its
 * derived direction and number consistent for the whole address space
 * representative values.
 */
HWTEST_F(UsbModelParcelTest, Model_EndpointAddressDerivation_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Model_EndpointAddressDerivation_001 enter");
    const uint32_t addresses[] = {0x00, 0x01, 0x02, 0x0F, 0x80, 0x81, 0x8F, 0x82};
    for (uint32_t address : addresses) {
        USBEndpoint ep(address, 2, 0, 64);
        EXPECT_EQ(ep.GetEndpointNumber(), address & MODEL_NUMBER_MASK);
        EXPECT_EQ(ep.GetDirection(), address & MODEL_DIR_MASK);
        EXPECT_EQ(ep.GetNumber(), static_cast<uint8_t>(address & MODEL_NUMBER_MASK));
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Model_EndpointAddressDerivation_001 end");
}

/*
 * The transfer type extraction must be consistent for every type constant
 * defined in usb_common.h.
 */
HWTEST_F(UsbModelParcelTest, Model_EndpointTypeFromConstants_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Model_EndpointTypeFromConstants_001 enter");
    EXPECT_EQ(USBEndpoint(0x01, USB_ENDPOINT_XFER_CONTROL, 0, 0).GetType(), 0u);
    EXPECT_EQ(USBEndpoint(0x01, USB_ENDPOINT_XFER_ISOC, 0, 0).GetType(), 1u);
    EXPECT_EQ(USBEndpoint(0x01, USB_ENDPOINT_XFER_BULK, 0, 0).GetType(), 2u);
    EXPECT_EQ(USBEndpoint(0x01, USB_ENDPOINT_XFER_INT, 0, 0).GetType(), 3u);
    USB_HILOGI(MODULE_USB_SERVICE, "Model_EndpointTypeFromConstants_001 end");
}

/*
 * Marshalling a freshly default-constructed hierarchy must succeed, proving
 * the write path tolerates uninitialized-like defaults.
 */
HWTEST_F(UsbModelParcelTest, Model_MarshalDefaultHierarchy_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Model_MarshalDefaultHierarchy_001 enter");
    USBEndpoint endpoint;
    UsbInterface intf;
    USBConfig config;
    UsbDevice device;
    Parcel parcel;
    EXPECT_TRUE(endpoint.Marshalling(parcel));
    EXPECT_TRUE(intf.Marshalling(parcel));
    EXPECT_TRUE(config.Marshalling(parcel));
    EXPECT_TRUE(device.Marshalling(parcel));
    USB_HILOGI(MODULE_USB_SERVICE, "Model_MarshalDefaultHierarchy_001 end");
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS