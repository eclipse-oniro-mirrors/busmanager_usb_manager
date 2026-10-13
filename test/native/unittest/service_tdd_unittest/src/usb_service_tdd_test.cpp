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

#include "usb_service_tdd_test.h"

#include <unistd.h>

#include <cstdint>
#include <string>
#include <vector>

#include "accesstoken_kit.h"
#include "hilog_wrapper.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "usb_common_test.h"
#include "usb_connection_callback_stub.h"
#include "usb_errors.h"
#include "usb_srv_client.h"

using namespace testing::ext;
using namespace OHOS::USB::Common;

namespace OHOS {
namespace USB {
namespace Service {
namespace {
constexpr uint8_t TEST_BUS_NUM_INVALID = 254;
constexpr uint8_t TEST_DEV_ADDR_INVALID = 254;
constexpr uint8_t TEST_INTERFACE_ID_INVALID = 0xFF;
constexpr uint8_t TEST_ENDPOINT_ADDR = 0x81;
constexpr int32_t TEST_TIMEOUT_MS = 1000;
constexpr int32_t TEST_PORT_ID_INVALID = 999;
constexpr int32_t TEST_SERIAL_PORT_ID_INVALID = -1;
constexpr int32_t TEST_TOKEN_ID_NUM = 1;
constexpr int32_t TEST_VENDOR_ID = 0x1234;
constexpr int32_t TEST_PRODUCT_ID = 0xABCD;
constexpr uint32_t TEST_SERIAL_READ_MAX = 64 * 1024;
constexpr uint32_t TEST_SERIAL_WRITE_MAX = 200 * 1024;
constexpr int32_t TEST_HUB_DEVICE_CLASS = 9;
constexpr int32_t TDD_APP_A = 0;
constexpr int32_t TDD_APP_B = 1;
const char *TDD_APP_A_PROCESS_NAME = "usb_service_tdd_app_a";
const char *TDD_APP_B_PROCESS_NAME = "usb_service_tdd_app_b";
const char *TDD_UNKNOWN_DEVICE = "254-254";

uint64_t AllocTddNativeToken(const char *processName, const char *apl)
{
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 0,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = nullptr,
        .acls = nullptr,
        .processName = processName,
        .aplStr = apl,
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    if (tokenId == 0) {
        USB_HILOGE(MODULE_USB_SERVICE, "AllocTddNativeToken failed for %{public}s", processName);
    }
    return tokenId;
}

class UsbTddConnectionCallback : public UsbConnectionCallbackStub {
public:
    UsbTddConnectionCallback() = default;
    ~UsbTddConnectionCallback() override = default;

    int32_t OnDeviceConnected(const UsbDevice &device, int32_t uid, const std::string &bundleName) override
    {
        USB_HILOGI(MODULE_USB_SERVICE, "Tdd listener OnDeviceConnected uid=%{public}d", uid);
        return UEC_OK;
    }

    int32_t OnDeviceDisconnected(const UsbDevice &device, int32_t uid, const std::string &bundleName) override
    {
        USB_HILOGI(MODULE_USB_SERVICE, "Tdd listener OnDeviceDisconnected uid=%{public}d", uid);
        return UEC_OK;
    }
};
} // namespace

uint64_t UsbServiceTddTest::tokenA_ = 0;
uint64_t UsbServiceTddTest::tokenB_ = 0;
uint8_t UsbServiceTddTest::busNum_ = 0;
uint8_t UsbServiceTddTest::devAddr_ = 0;
uint8_t UsbServiceTddTest::ifaceId_ = 0;
std::string UsbServiceTddTest::devName_ = "";
UsbDevice UsbServiceTddTest::device_ {};
USBDevicePipe UsbServiceTddTest::pipe_ {};
UsbInterface UsbServiceTddTest::iface_ {};
USBEndpoint UsbServiceTddTest::endpoint_ {};
bool UsbServiceTddTest::deviceReady_ = false;

void UsbServiceTddTest::SetUpTestCase()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTddTest SetUpTestCase");
    SwitchToSysCaller();
    tokenA_ = AllocTddNativeToken(TDD_APP_A_PROCESS_NAME, "normal");
    tokenB_ = AllocTddNativeToken(TDD_APP_B_PROCESS_NAME, "normal");
    deviceReady_ = PickTestDevice();
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTddTest deviceReady=%{public}d", deviceReady_);
}

void UsbServiceTddTest::TearDownTestCase()
{
    SwitchToSysCaller();
    if (deviceReady_) {
        CleanClaimState();
    }
    USB_HILOGI(MODULE_USB_SERVICE, "UsbServiceTddTest TearDownTestCase");
}

void UsbServiceTddTest::SetUp()
{
    SwitchToSysCaller();
    if (deviceReady_) {
        CleanClaimState();
    }
}

void UsbServiceTddTest::TearDown()
{
    SwitchToSysCaller();
    if (deviceReady_) {
        CleanClaimState();
    }
}

void UsbServiceTddTest::SwitchToSysCaller()
{
    UsbCommonTest::GrantPermissionSysNative();
}

void UsbServiceTddTest::SwitchToNormalCaller()
{
    UsbCommonTest::GrantPermissionNormalNative();
}

void UsbServiceTddTest::SwitchToSysCallerNoPerm()
{
    UsbCommonTest::GrantSysNoPermissionNative();
}

void UsbServiceTddTest::SwitchToApp(int32_t appIndex)
{
    uint64_t token = (appIndex == TDD_APP_B) ? tokenB_ : tokenA_;
    SetSelfTokenID(token);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

bool UsbServiceTddTest::PickTestDevice()
{
    std::vector<UsbDevice> deviceList;
    auto &client = UsbSrvClient::GetInstance();
    if (client.GetDevices(deviceList) != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "PickTestDevice GetDevices failed");
        return false;
    }
    for (auto &dev : deviceList) {
        if (dev.GetClass() == TEST_HUB_DEVICE_CLASS) {
            continue;
        }
        if (dev.GetConfigs().empty()) {
            continue;
        }
        USBConfig config = dev.GetConfigs().at(0);
        if (config.GetInterfaces().empty()) {
            continue;
        }
        UsbInterface interface = config.GetInterfaces().at(0);
        if (interface.GetEndpoints().empty()) {
            continue;
        }
        device_ = dev;
        busNum_ = dev.GetBusNum();
        devAddr_ = dev.GetDevAddr();
        devName_ = dev.GetName();
        ifaceId_ = static_cast<uint8_t>(interface.GetId());
        iface_ = interface;
        endpoint_ = interface.GetEndpoints().at(0);
        pipe_.SetBusNum(busNum_);
        pipe_.SetDevAddr(devAddr_);
        return true;
    }
    USB_HILOGI(MODULE_USB_SERVICE, "PickTestDevice no usable non-hub device");
    return false;
}

/*
 * Bring the claim state of the test interface back to a clean baseline:
 * no exclusive claim record, no normal claim flag, interface released at HDI
 * level and the kernel driver re-attached. ReleaseInterface runs under both
 * identities because the normal claim flag does not record its owner and
 * only the exclusive owner can clear the exclusive record.
 */
void UsbServiceTddTest::CleanClaimState()
{
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(TDD_APP_A);
    client.OpenDevice(device_, pipe_);
    client.ReleaseInterface(pipe_, iface_);
    SwitchToApp(TDD_APP_B);
    client.ReleaseInterface(pipe_, iface_);
    SwitchToApp(TDD_APP_A);
    client.UsbAttachKernelDriver(pipe_, iface_);
}

/* ------------------------------------------------------------------------ *
 * Section A: Service availability
 * ------------------------------------------------------------------------ */

/**
 * @tc.name: ServiceAvailable001
 * @tc.desc: Test GetDevices as a system caller holding MANAGE_USB_CONFIG
 * @tc.desc: Positive test: the service answers with UEC_OK
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ServiceAvailable001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> deviceList;
    int32_t ret = client.GetDevices(deviceList);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: ServiceAvailable002
 * @tc.desc: Test GetVersion reports the negotiated service version string
 * @tc.desc: Positive test: version equals "001.001.025"
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ServiceAvailable002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    EXPECT_EQ(std::string("001.001.025"), client.GetVersion());
}

/**
 * @tc.name: ServiceAvailable003
 * @tc.desc: Test two consecutive GetDevices calls return a consistent device count
 * @tc.desc: Positive test: repeated enumeration is stable
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ServiceAvailable003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbDevice> firstList;
    std::vector<UsbDevice> secondList;
    ASSERT_EQ(UEC_OK, client.GetDevices(firstList));
    ASSERT_EQ(UEC_OK, client.GetDevices(secondList));
    EXPECT_EQ(firstList.size(), secondList.size());
}

/* ------------------------------------------------------------------------ *
 * Section B: SysAPI permission gating (UsbService::CheckSysApiPermission)
 * ------------------------------------------------------------------------ */

#ifdef USB_MANAGER_FEATURE_DEVICE
/**
 * @tc.name: SysApiPermission001
 * @tc.desc: Test GetCurrentFunctions called by a normal native token without permission
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED_SYSAPI
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SysApiPermission001, TestSize.Level1)
{
    SwitchToNormalCaller();
    auto &client = UsbSrvClient::GetInstance();
    int32_t funcs = -1;
    int32_t ret = client.GetCurrentFunctions(funcs);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED_SYSAPI, ret);
}

/**
 * @tc.name: SysApiPermission002
 * @tc.desc: Test GetCurrentFunctions called by a system_basic token without MANAGE_USB_CONFIG
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SysApiPermission002, TestSize.Level1)
{
    SwitchToSysCallerNoPerm();
    auto &client = UsbSrvClient::GetInstance();
    int32_t funcs = -1;
    int32_t ret = client.GetCurrentFunctions(funcs);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED, ret);
}

/**
 * @tc.name: SysApiPermission003
 * @tc.desc: Test UsbFunctionsToString called by a normal native token
 * @tc.desc: Negative test: returns the PERMISSION_DENIED_SYSAPI marker string
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SysApiPermission003, TestSize.Level1)
{
    SwitchToNormalCaller();
    auto &client = UsbSrvClient::GetInstance();
    std::string funcResult = client.UsbFunctionsToString(0);
    EXPECT_EQ(PERMISSION_DENIED_SYSAPI, funcResult);
}

/**
 * @tc.name: SysApiPermission004
 * @tc.desc: Test UsbFunctionsToString called by a system_basic token without the permission
 * @tc.desc: Negative test: returns the SYS_APP_PERMISSION_DENIED_SYSAPI marker string
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SysApiPermission004, TestSize.Level1)
{
    SwitchToSysCallerNoPerm();
    auto &client = UsbSrvClient::GetInstance();
    std::string funcResult = client.UsbFunctionsToString(0);
    EXPECT_EQ(SYS_APP_PERMISSION_DENIED_SYSAPI, funcResult);
}
#endif // USB_MANAGER_FEATURE_DEVICE

#ifdef USB_MANAGER_FEATURE_PORT
/**
 * @tc.name: SysApiPermission005
 * @tc.desc: Test GetPorts called by a normal native token without permission
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED_SYSAPI
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SysApiPermission005, TestSize.Level1)
{
    SwitchToNormalCaller();
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbPort> ports;
    int32_t ret = client.GetPorts(ports);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED_SYSAPI, ret);
}
#endif // USB_MANAGER_FEATURE_PORT

#ifdef USB_MANAGER_FEATURE_HOST
/**
 * @tc.name: SysApiPermission006
 * @tc.desc: Test AddAccessRight called by a normal native token without permission
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED_SYSAPI
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SysApiPermission006, TestSize.Level1)
{
    SwitchToNormalCaller();
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.AddAccessRight(std::to_string(TEST_TOKEN_ID_NUM), TDD_UNKNOWN_DEVICE);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED_SYSAPI, ret);
}

/**
 * @tc.name: SysApiPermission007
 * @tc.desc: Test AddAccessRight called by a system_basic token without MANAGE_USB_CONFIG
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SysApiPermission007, TestSize.Level1)
{
    SwitchToSysCallerNoPerm();
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.AddAccessRight(std::to_string(TEST_TOKEN_ID_NUM), TDD_UNKNOWN_DEVICE);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED, ret);
}
#endif // USB_MANAGER_FEATURE_HOST

/* ------------------------------------------------------------------------ *
 * Section C: Device right lookup for unknown devices
 * (UsbService::HasRight / RequestRight / RemoveRight / AddAccessRight)
 * ------------------------------------------------------------------------ */

#ifdef USB_MANAGER_FEATURE_HOST
/**
 * @tc.name: DeviceRight001
 * @tc.desc: Test HasRightEx with a device name that was never attached
 * @tc.desc: Negative test: call succeeds but reports hasRight=false because the
 *           device is missing from the vid/pid map before the system app bypass
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, DeviceRight001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    bool hasRight = true;
    int32_t ret = client.HasRightEx(TDD_UNKNOWN_DEVICE, hasRight);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_FALSE(hasRight);
}

/**
 * @tc.name: DeviceRight002
 * @tc.desc: Test HasRightEx with an empty device name
 * @tc.desc: Negative test: reports hasRight=false without failing the call
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, DeviceRight002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    bool hasRight = true;
    int32_t ret = client.HasRightEx("", hasRight);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_FALSE(hasRight);
}

/**
 * @tc.name: DeviceRight003
 * @tc.desc: Test RequestRight with a device name that was never attached
 * @tc.desc: Negative test: rejected with UEC_INTERFACE_INVALID_VALUE
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, DeviceRight003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.RequestRight(TDD_UNKNOWN_DEVICE);
    EXPECT_EQ(UEC_INTERFACE_INVALID_VALUE, ret);
}

/**
 * @tc.name: DeviceRight004
 * @tc.desc: Test RequestRight with an empty device name
 * @tc.desc: Negative test: rejected with UEC_INTERFACE_INVALID_VALUE
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, DeviceRight004, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.RequestRight("");
    EXPECT_EQ(UEC_INTERFACE_INVALID_VALUE, ret);
}

/**
 * @tc.name: DeviceRight005
 * @tc.desc: Test RemoveRight with a device name that was never attached
 * @tc.desc: Negative test: rejected with UEC_INTERFACE_INVALID_VALUE
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, DeviceRight005, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.RemoveRight(TDD_UNKNOWN_DEVICE);
    EXPECT_EQ(UEC_INTERFACE_INVALID_VALUE, ret);
}

/**
 * @tc.name: DeviceRight006
 * @tc.desc: Test RemoveRight with an empty device name
 * @tc.desc: Negative test: rejected with UEC_INTERFACE_INVALID_VALUE
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, DeviceRight006, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.RemoveRight("");
    EXPECT_EQ(UEC_INTERFACE_INVALID_VALUE, ret);
}

/**
 * @tc.name: DeviceRight007
 * @tc.desc: Test AddAccessRight with an authorized system caller but an unknown device
 * @tc.desc: Negative test: rejected with UEC_INTERFACE_INVALID_VALUE after the
 *           permission check passes
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, DeviceRight007, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.AddAccessRight(std::to_string(TEST_TOKEN_ID_NUM), TDD_UNKNOWN_DEVICE);
    EXPECT_EQ(UEC_INTERFACE_INVALID_VALUE, ret);
}

/**
 * @tc.name: DeviceRight008
 * @tc.desc: Test HasRightEx for an attached device called by a system native token
 * @tc.desc: Positive test: the system app bypass grants hasRight=true
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, DeviceRight008, TestSize.Level1)
{
    if (!deviceReady_) {
        GTEST_SKIP() << "no USB device attached, skip";
    }
    auto &client = UsbSrvClient::GetInstance();
    bool hasRight = false;
    int32_t ret = client.HasRightEx(devName_, hasRight);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_TRUE(hasRight);
}
#endif // USB_MANAGER_FEATURE_HOST

/* ------------------------------------------------------------------------ *
 * Section D: Open / close / reset on invalid device addresses
 * (UsbService::OpenDevice / Close / ResetDevice)
 * ------------------------------------------------------------------------ */

#ifdef USB_MANAGER_FEATURE_HOST
/**
 * @tc.name: OpenClose001
 * @tc.desc: Test OpenDevice with a bus/dev address that was never attached
 * @tc.desc: Negative test: permission lookup fails first with
 *           UEC_SERVICE_PERMISSION_DENIED
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, OpenClose001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbDevice device;
    device.SetBusNum(TEST_BUS_NUM_INVALID);
    device.SetDevAddr(TEST_DEV_ADDR_INVALID);
    USBDevicePipe pipe;
    int32_t ret = client.OpenDevice(device, pipe);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED, ret);
}

/**
 * @tc.name: OpenClose002
 * @tc.desc: Test CloseEx on a pipe that was never opened
 * @tc.desc: Negative test: permission lookup fails first with
 *           UEC_SERVICE_PERMISSION_DENIED
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, OpenClose002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    pipe.SetBusNum(TEST_BUS_NUM_INVALID);
    pipe.SetDevAddr(TEST_DEV_ADDR_INVALID);
    int32_t ret = client.CloseEx(pipe);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED, ret);
}

/**
 * @tc.name: OpenClose003
 * @tc.desc: Test ResetDevice on a device missing from the vid/pid map
 * @tc.desc: Negative test: rejected with UEC_INTERFACE_NAME_NOT_FOUND before
 *           the permission check
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, OpenClose003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    pipe.SetBusNum(TEST_BUS_NUM_INVALID);
    pipe.SetDevAddr(TEST_DEV_ADDR_INVALID);
    int32_t ret = client.ResetDevice(pipe);
    EXPECT_EQ(UEC_INTERFACE_NAME_NOT_FOUND, ret);
}

/**
 * @tc.name: OpenClose004
 * @tc.desc: Test open and close cycle on an attached device
 * @tc.desc: Positive test: OpenDevice succeeds and Close returns true
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, OpenClose004, TestSize.Level1)
{
    if (!deviceReady_) {
        GTEST_SKIP() << "no USB device attached, skip";
    }
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    int32_t ret = client.OpenDevice(device_, pipe);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_TRUE(client.Close(pipe));
}

/**
 * @tc.name: OpenClose005
 * @tc.desc: Test GetDeviceSpeed on an opened pipe
 * @tc.desc: Positive test: the service reports UEC_OK
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, OpenClose005, TestSize.Level1)
{
    if (!deviceReady_) {
        GTEST_SKIP() << "no USB device attached, skip";
    }
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipe));
    uint8_t speed = 0;
    int32_t ret = client.GetDeviceSpeed(pipe, speed);
    EXPECT_EQ(UEC_OK, ret);
    client.Close(pipe);
}

/**
 * @tc.name: OpenClose006
 * @tc.desc: Test GetFileDescriptor on an opened pipe
 * @tc.desc: Positive test: the service reports UEC_OK with a positive fd
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, OpenClose006, TestSize.Level1)
{
    if (!deviceReady_) {
        GTEST_SKIP() << "no USB device attached, skip";
    }
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipe));
    int32_t fd = -1;
    int32_t ret = client.GetFileDescriptor(pipe, fd);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_GT(fd, 0);
    client.Close(pipe);
}
#endif // USB_MANAGER_FEATURE_HOST

/* ------------------------------------------------------------------------ *
 * Section E: Claim interface state machine
 * (UsbService::ClaimInterfaceExclusive / ReleaseClaimState / CheckExclusiveTransfer)
 * ------------------------------------------------------------------------ */

#ifdef USB_MANAGER_FEATURE_HOST
/**
 * @tc.name: ClaimExclusive001
 * @tc.desc: Test exclusive claim followed by release by the same application
 * @tc.desc: Positive test: both operations return UEC_OK
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ClaimExclusive001, TestSize.Level1)
{
    if (!deviceReady_) {
        GTEST_SKIP() << "no USB device attached, skip";
    }
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(TDD_APP_A);
    USBDevicePipe pipe;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipe));
    int32_t ret = client.ClaimInterfaceExclusive(pipe, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe, iface_);
    EXPECT_EQ(UEC_OK, ret);
    client.Close(pipe);
}

/**
 * @tc.name: ClaimExclusive002
 * @tc.desc: Test exclusive claim of an interface already exclusively claimed by another app
 * @tc.desc: Negative test: the second application gets UEC_INTERFACE_BUSY
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ClaimExclusive002, TestSize.Level1)
{
    if (!deviceReady_) {
        GTEST_SKIP() << "no USB device attached, skip";
    }
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(TDD_APP_A);
    USBDevicePipe pipeA;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipeA));
    ASSERT_EQ(UEC_OK, client.ClaimInterfaceExclusive(pipeA, iface_, true, nullptr));
    SwitchToApp(TDD_APP_B);
    USBDevicePipe pipeB;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipeB));
    int32_t ret = client.ClaimInterfaceExclusive(pipeB, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
    client.Close(pipeB);
    SwitchToApp(TDD_APP_A);
    client.ReleaseInterface(pipeA, iface_);
    client.Close(pipeA);
}

/**
 * @tc.name: ClaimExclusive003
 * @tc.desc: Test repeated exclusive claim by the same application
 * @tc.desc: Positive test: the repeat claim returns UEC_OK
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ClaimExclusive003, TestSize.Level1)
{
    if (!deviceReady_) {
        GTEST_SKIP() << "no USB device attached, skip";
    }
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(TDD_APP_A);
    USBDevicePipe pipe;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipe));
    ASSERT_EQ(UEC_OK, client.ClaimInterfaceExclusive(pipe, iface_, true, nullptr));
    int32_t ret = client.ClaimInterfaceExclusive(pipe, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe, iface_);
    EXPECT_EQ(UEC_OK, ret);
    client.Close(pipe);
}

/**
 * @tc.name: ClaimExclusive004
 * @tc.desc: Test normal claim coexisting with an exclusive claim by another app
 * @tc.desc: Positive test: normal claim succeeds, and each application can
 *           release its own claim with UEC_OK
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ClaimExclusive004, TestSize.Level1)
{
    if (!deviceReady_) {
        GTEST_SKIP() << "no USB device attached, skip";
    }
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(TDD_APP_A);
    USBDevicePipe pipeA;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipeA));
    ASSERT_EQ(UEC_OK, client.ClaimInterfaceExclusive(pipeA, iface_, true, nullptr));
    SwitchToApp(TDD_APP_B);
    USBDevicePipe pipeB;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipeB));
    int32_t ret = client.ClaimInterface(pipeB, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipeB, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(TDD_APP_A);
    ret = client.ReleaseInterface(pipeA, iface_);
    EXPECT_EQ(UEC_OK, ret);
    client.Close(pipeB);
    client.Close(pipeA);
}

/**
 * @tc.name: ClaimExclusive005
 * @tc.desc: Test bulk transfer on an interface exclusively claimed by another app
 * @tc.desc: Negative test: CheckExclusiveTransfer denies with UEC_SERVICE_IO_EXCEPTION
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ClaimExclusive005, TestSize.Level1)
{
    if (!deviceReady_) {
        GTEST_SKIP() << "no USB device attached, skip";
    }
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(TDD_APP_A);
    USBDevicePipe pipeA;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipeA));
    ASSERT_EQ(UEC_OK, client.ClaimInterfaceExclusive(pipeA, iface_, true, nullptr));
    SwitchToApp(TDD_APP_B);
    USBDevicePipe pipeB;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipeB));
    std::vector<uint8_t> bufferData(TEST_SERIAL_READ_MAX, 0);
    int32_t ret = client.BulkTransfer(pipeB, endpoint_, bufferData, TEST_TIMEOUT_MS);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, ret);
    client.Close(pipeB);
    SwitchToApp(TDD_APP_A);
    client.ReleaseInterface(pipeA, iface_);
    client.Close(pipeA);
}

/**
 * @tc.name: ClaimExclusive006
 * @tc.desc: Test exclusive claim of an interface already normally claimed by another app
 * @tc.desc: Negative test: the exclusive claim gets UEC_INTERFACE_BUSY
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ClaimExclusive006, TestSize.Level1)
{
    if (!deviceReady_) {
        GTEST_SKIP() << "no USB device attached, skip";
    }
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(TDD_APP_A);
    USBDevicePipe pipeA;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipeA));
    ASSERT_EQ(UEC_OK, client.ClaimInterface(pipeA, iface_, true));
    SwitchToApp(TDD_APP_B);
    USBDevicePipe pipeB;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipeB));
    int32_t ret = client.ClaimInterfaceExclusive(pipeB, iface_, true, nullptr);
    EXPECT_EQ(UEC_INTERFACE_BUSY, ret);
    client.Close(pipeB);
    SwitchToApp(TDD_APP_A);
    client.ReleaseInterface(pipeA, iface_);
    client.Close(pipeA);
}

/**
 * @tc.name: ClaimExclusive007
 * @tc.desc: Test ClaimInterface with an interface id that does not exist on the device
 * @tc.desc: Negative test: the HDI layer rejects the claim and the service
 *           forwards the failure
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ClaimExclusive007, TestSize.Level1)
{
    if (!deviceReady_) {
        GTEST_SKIP() << "no USB device attached, skip";
    }
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    ASSERT_EQ(UEC_OK, client.OpenDevice(device_, pipe));
    UsbInterface badIface = iface_;
    badIface.SetId(TEST_INTERFACE_ID_INVALID);
    int32_t ret = client.ClaimInterface(pipe, badIface, true);
    EXPECT_NE(UEC_OK, ret);
    client.Close(pipe);
}
#endif // USB_MANAGER_FEATURE_HOST

/* ------------------------------------------------------------------------ *
 * Section F: Transfer parameter validation on unknown devices
 * (UsbService::BulkTransferRead / ControlTransfer / GetRawDescriptor ...)
 * ------------------------------------------------------------------------ */

#ifdef USB_MANAGER_FEATURE_HOST
/**
 * @tc.name: TransferValidation001
 * @tc.desc: Test GetRawDescriptors for a device missing from the vid/pid map
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, TransferValidation001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    pipe.SetBusNum(TEST_BUS_NUM_INVALID);
    pipe.SetDevAddr(TEST_DEV_ADDR_INVALID);
    std::vector<uint8_t> bufferData;
    int32_t ret = client.GetRawDescriptors(pipe, bufferData);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED, ret);
}

/**
 * @tc.name: TransferValidation002
 * @tc.desc: Test GetFileDescriptor for a device missing from the vid/pid map
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, TransferValidation002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    pipe.SetBusNum(TEST_BUS_NUM_INVALID);
    pipe.SetDevAddr(TEST_DEV_ADDR_INVALID);
    int32_t fd = -1;
    int32_t ret = client.GetFileDescriptor(pipe, fd);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED, ret);
}

/**
 * @tc.name: TransferValidation003
 * @tc.desc: Test GetDeviceSpeed for a device that cannot be opened
 * @tc.desc: Negative test: the HDI layer fails the request
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, TransferValidation003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    pipe.SetBusNum(TEST_BUS_NUM_INVALID);
    pipe.SetDevAddr(TEST_DEV_ADDR_INVALID);
    uint8_t speed = 0;
    int32_t ret = client.GetDeviceSpeed(pipe, speed);
    EXPECT_NE(UEC_OK, ret);
}

/**
 * @tc.name: TransferValidation004
 * @tc.desc: Test GetInterfaceActiveStatus for a device that cannot be opened
 * @tc.desc: Negative test: the HDI layer fails the request
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, TransferValidation004, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    pipe.SetBusNum(TEST_BUS_NUM_INVALID);
    pipe.SetDevAddr(TEST_DEV_ADDR_INVALID);
    bool unactivated = false;
    int32_t ret = client.GetInterfaceActiveStatus(pipe, iface_, unactivated);
    EXPECT_NE(UEC_OK, ret);
}

/**
 * @tc.name: TransferValidation005
 * @tc.desc: Test ControlTransfer for a device missing from the vid/pid map
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, TransferValidation005, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    pipe.SetBusNum(TEST_BUS_NUM_INVALID);
    pipe.SetDevAddr(TEST_DEV_ADDR_INVALID);
    HDI::Usb::V1_0::UsbCtrlTransfer ctrl {};
    std::vector<uint8_t> bufferData;
    int32_t ret = client.ControlTransfer(pipe, ctrl, bufferData);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED, ret);
}

/**
 * @tc.name: TransferValidation006
 * @tc.desc: Test BulkTransfer for a device missing from the vid/pid map
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, TransferValidation006, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    pipe.SetBusNum(TEST_BUS_NUM_INVALID);
    pipe.SetDevAddr(TEST_DEV_ADDR_INVALID);
    std::vector<uint8_t> bufferData(TEST_SERIAL_READ_MAX, 0);
    int32_t ret = client.BulkTransfer(pipe, endpoint_, bufferData, TEST_TIMEOUT_MS);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED, ret);
}

/**
 * @tc.name: TransferValidation007
 * @tc.desc: Test UsbCancelTransfer for a device missing from the vid/pid map
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, TransferValidation007, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    pipe.SetBusNum(TEST_BUS_NUM_INVALID);
    pipe.SetDevAddr(TEST_DEV_ADDR_INVALID);
    int32_t endpoint = TEST_ENDPOINT_ADDR;
    int32_t ret = client.UsbCancelTransfer(pipe, endpoint);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED, ret);
}

/**
 * @tc.name: TransferValidation008
 * @tc.desc: Test ClearHalt for a device missing from the vid/pid map
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, TransferValidation008, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    USBDevicePipe pipe;
    pipe.SetBusNum(TEST_BUS_NUM_INVALID);
    pipe.SetDevAddr(TEST_DEV_ADDR_INVALID);
    int32_t ret = client.ClearHalt(pipe, endpoint_);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED, ret);
}
#endif // USB_MANAGER_FEATURE_HOST

/* ------------------------------------------------------------------------ *
 * Section G: EDM manage APIs caller validation
 * (UsbService::IsCallerValid / PreCallFunction)
 * ------------------------------------------------------------------------ */

#ifdef USB_MANAGER_FEATURE_HOST
/**
 * @tc.name: ManageApi001
 * @tc.desc: Test ManageGlobalInterface called by a normal native token without permission
 * @tc.desc: Negative test: the pre-call permission gate rejects with
 *           UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED and no HDI state changes
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ManageApi001, TestSize.Level1)
{
    if (getuid() != 0) {
        GTEST_SKIP() << "test process is not root, IsCallerValid cannot pass, skip";
    }
    SwitchToNormalCaller();
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.ManageGlobalInterface(true);
    EXPECT_EQ(UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED, ret);
}

/**
 * @tc.name: ManageApi002
 * @tc.desc: Test ManageDevice called by a normal native token without permission
 * @tc.desc: Negative test: the pre-call permission gate rejects with
 *           UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ManageApi002, TestSize.Level1)
{
    if (getuid() != 0) {
        GTEST_SKIP() << "test process is not root, IsCallerValid cannot pass, skip";
    }
    SwitchToNormalCaller();
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.ManageDevice(TEST_VENDOR_ID, TEST_PRODUCT_ID, true);
    EXPECT_EQ(UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED, ret);
}

/**
 * @tc.name: ManageApi003
 * @tc.desc: Test ManageDevicePolicy with an empty trust list from a normal caller
 * @tc.desc: Negative test: the pre-call permission gate rejects before the
 *           list is processed
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ManageApi003, TestSize.Level1)
{
    if (getuid() != 0) {
        GTEST_SKIP() << "test process is not root, IsCallerValid cannot pass, skip";
    }
    SwitchToNormalCaller();
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceId> trustList;
    int32_t ret = client.ManageDevicePolicy(trustList);
    EXPECT_EQ(UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED, ret);
}

/**
 * @tc.name: ManageApi004
 * @tc.desc: Test ManageInterfaceType with an empty type list from a normal caller
 * @tc.desc: Negative test: the pre-call permission gate rejects before the
 *           list is processed
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ManageApi004, TestSize.Level1)
{
    if (getuid() != 0) {
        GTEST_SKIP() << "test process is not root, IsCallerValid cannot pass, skip";
    }
    SwitchToNormalCaller();
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceType> disableType;
    int32_t ret = client.ManageInterfaceType(disableType, true);
    EXPECT_EQ(UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED, ret);
}

/**
 * @tc.name: ManageApi005
 * @tc.desc: Test ManageUsbType with an empty type list from a normal caller
 * @tc.desc: Negative test: the pre-call permission gate rejects before the
 *           list is processed
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, ManageApi005, TestSize.Level1)
{
    if (getuid() != 0) {
        GTEST_SKIP() << "test process is not root, IsCallerValid cannot pass, skip";
    }
    SwitchToNormalCaller();
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbDeviceType> disableType;
    int32_t ret = client.ManageUsbType(disableType, true);
    EXPECT_EQ(UEC_SERVICE_PRE_MANAGE_INTERFACE_FAILED, ret);
}
#endif // USB_MANAGER_FEATURE_HOST

/* ------------------------------------------------------------------------ *
 * Section H: Serial port input contracts
 * (UsbService::SerialRead / SerialWrite / ValidateUsbSerialManagerAndPort)
 * ------------------------------------------------------------------------ */

/**
 * @tc.name: SerialValidation001
 * @tc.desc: Test SerialRead with a requested size above the 64KB read limit
 * @tc.desc: Negative test: rejected with UEC_SERIAL_IO_EXCEPTION before the
 *           port is validated
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SerialValidation001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<uint8_t> data(TEST_SERIAL_READ_MAX, 0);
    uint32_t actualSize = 0;
    int32_t ret = client.SerialRead(TEST_SERIAL_PORT_ID_INVALID, data, TEST_SERIAL_READ_MAX + 1, actualSize,
        TEST_TIMEOUT_MS);
    EXPECT_EQ(UEC_SERIAL_IO_EXCEPTION, ret);
}

/**
 * @tc.name: SerialValidation002
 * @tc.desc: Test SerialRead with a receive buffer above the 64KB read limit
 * @tc.desc: Negative test: rejected with UEC_SERIAL_IO_EXCEPTION before the
 *           port is validated
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SerialValidation002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<uint8_t> data(TEST_SERIAL_READ_MAX + 1, 0);
    uint32_t actualSize = 0;
    int32_t ret = client.SerialRead(TEST_SERIAL_PORT_ID_INVALID, data, 1, actualSize, TEST_TIMEOUT_MS);
    EXPECT_EQ(UEC_SERIAL_IO_EXCEPTION, ret);
}

/**
 * @tc.name: SerialValidation003
 * @tc.desc: Test SerialWrite with a buffer above the 200KB write limit
 * @tc.desc: Negative test: rejected with UEC_SERIAL_IO_EXCEPTION before the
 *           port is validated
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SerialValidation003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<uint8_t> data(TEST_SERIAL_WRITE_MAX + 1, 0);
    uint32_t actualSize = 0;
    int32_t ret = client.SerialWrite(TEST_SERIAL_PORT_ID_INVALID, data, TEST_SERIAL_WRITE_MAX + 1, actualSize,
        TEST_TIMEOUT_MS);
    EXPECT_EQ(UEC_SERIAL_IO_EXCEPTION, ret);
}

/**
 * @tc.name: SerialValidation004
 * @tc.desc: Test SerialWrite with a size larger than the supplied data buffer
 * @tc.desc: Negative test: rejected with UEC_SERVICE_INVALID_VALUE
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SerialValidation004, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<uint8_t> data(16, 0);
    uint32_t actualSize = 0;
    int32_t ret = client.SerialWrite(TEST_SERIAL_PORT_ID_INVALID, data, 32, actualSize, TEST_TIMEOUT_MS);
    EXPECT_EQ(UEC_SERVICE_INVALID_VALUE, ret);
}

/**
 * @tc.name: SerialValidation005
 * @tc.desc: Test SerialOpen with a port id that does not exist
 * @tc.desc: Negative test: rejected with UEC_SERIAL_PORT_NOT_EXIST, or
 *           UEC_SERVICE_INVALID_VALUE when the serial manager is not started
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SerialValidation005, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.SerialOpen(TEST_SERIAL_PORT_ID_INVALID);
    EXPECT_TRUE(ret == UEC_SERIAL_PORT_NOT_EXIST || ret == UEC_SERVICE_INVALID_VALUE);
}

/**
 * @tc.name: SerialValidation006
 * @tc.desc: Test SerialClose with a port id that does not exist
 * @tc.desc: Negative test: rejected with UEC_SERIAL_PORT_NOT_EXIST, or
 *           UEC_SERVICE_INVALID_VALUE when the serial manager is not started
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SerialValidation006, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.SerialClose(TEST_SERIAL_PORT_ID_INVALID);
    EXPECT_TRUE(ret == UEC_SERIAL_PORT_NOT_EXIST || ret == UEC_SERVICE_INVALID_VALUE);
}

/**
 * @tc.name: SerialValidation007
 * @tc.desc: Test SerialGetAttribute with a port id that does not exist
 * @tc.desc: Negative test: rejected with UEC_SERIAL_PORT_NOT_EXIST, or
 *           UEC_SERVICE_INVALID_VALUE when the serial manager is not started
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SerialValidation007, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbSerialAttr attribute;
    int32_t ret = client.SerialGetAttribute(TEST_SERIAL_PORT_ID_INVALID, attribute);
    EXPECT_TRUE(ret == UEC_SERIAL_PORT_NOT_EXIST || ret == UEC_SERVICE_INVALID_VALUE);
}

/**
 * @tc.name: SerialValidation008
 * @tc.desc: Test SerialSetAttribute with a port id that does not exist
 * @tc.desc: Negative test: rejected with UEC_SERIAL_PORT_NOT_EXIST, or
 *           UEC_SERVICE_INVALID_VALUE when the serial manager is not started
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SerialValidation008, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    UsbSerialAttr attribute;
    attribute.baudRate_ = 115200;
    int32_t ret = client.SerialSetAttribute(TEST_SERIAL_PORT_ID_INVALID, attribute);
    EXPECT_TRUE(ret == UEC_SERIAL_PORT_NOT_EXIST || ret == UEC_SERVICE_INVALID_VALUE);
}

/**
 * @tc.name: SerialValidation009
 * @tc.desc: Test SerialGetPortList on a device without serial hardware
 * @tc.desc: Positive test: the call always succeeds with an empty list when no
 *           serial port exists
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SerialValidation009, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbSerialPort> serialPorts;
    int32_t ret = client.SerialGetPortList(serialPorts);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: SerialValidation010
 * @tc.desc: Test HasSerialRight with a port id that does not exist
 * @tc.desc: Negative test: rejected with the invalid-port error and hasRight
 *           stays false
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, SerialValidation010, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    bool hasRight = true;
    int32_t ret = client.HasSerialRight(TEST_SERIAL_PORT_ID_INVALID, hasRight);
    EXPECT_TRUE(ret == UEC_SERIAL_PORT_NOT_EXIST || ret == UEC_SERVICE_INVALID_VALUE);
    EXPECT_FALSE(hasRight);
}

/* ------------------------------------------------------------------------ *
 * Section I: Port APIs (UsbService::GetPorts / GetSupportedModes / SetPortRole)
 * ------------------------------------------------------------------------ */

#ifdef USB_MANAGER_FEATURE_PORT
/**
 * @tc.name: PortApi001
 * @tc.desc: Test GetPorts as an authorized system caller
 * @tc.desc: Positive test: the call succeeds and reports valid port ids
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, PortApi001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::vector<UsbPort> ports;
    int32_t ret = client.GetPorts(ports);
    EXPECT_EQ(UEC_OK, ret);
    for (auto &port : ports) {
        EXPECT_GE(port.id, 0);
    }
}

/**
 * @tc.name: PortApi002
 * @tc.desc: Test GetSupportedModes with a port id that does not exist
 * @tc.desc: Negative test: the HDI layer fails the request
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, PortApi002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t supportedModes = -1;
    int32_t ret = client.GetSupportedModes(TEST_PORT_ID_INVALID, supportedModes);
    EXPECT_NE(UEC_OK, ret);
}

/**
 * @tc.name: PortApi003
 * @tc.desc: Test SetPortRole with a port id that does not exist
 * @tc.desc: Negative test: the request must not report success for an unknown
 *           port
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, PortApi003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.SetPortRole(TEST_PORT_ID_INVALID, 0, 0);
    EXPECT_NE(UEC_OK, ret);
}

/**
 * @tc.name: PortApi004
 * @tc.desc: Test SetPortRole called by a normal native token without permission
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED_SYSAPI
 *           before any port state is touched
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, PortApi004, TestSize.Level1)
{
    SwitchToNormalCaller();
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.SetPortRole(TEST_PORT_ID_INVALID, 0, 0);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED_SYSAPI, ret);
}
#endif // USB_MANAGER_FEATURE_PORT

/* ------------------------------------------------------------------------ *
 * Section J: Device function string contracts
 * (UsbDeviceManager::ConvertFromString / ConvertToString via UsbService)
 * ------------------------------------------------------------------------ */

#ifdef USB_MANAGER_FEATURE_DEVICE
/**
 * @tc.name: Functions001
 * @tc.desc: Test GetCurrentFunctions as an authorized system caller
 * @tc.desc: Positive test: the current function value is reported
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, Functions001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t funcs = -1;
    int32_t ret = client.GetCurrentFunctions(funcs);
    EXPECT_EQ(UEC_OK, ret);
    EXPECT_GE(funcs, 0);
}

/**
 * @tc.name: Functions002
 * @tc.desc: Test UsbFunctionsFromString with the "none" function name
 * @tc.desc: Positive test: "none" converts to FUNCTION_NONE (0)
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, Functions002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.UsbFunctionsFromString("none");
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: Functions003
 * @tc.desc: Test UsbFunctionsFromString with an unknown function name
 * @tc.desc: Negative test: conversion fails with UEC_SERVICE_INVALID_VALUE
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, Functions003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.UsbFunctionsFromString("tdd_bogus_function");
    EXPECT_EQ(UEC_SERVICE_INVALID_VALUE, ret);
}

/**
 * @tc.name: Functions004
 * @tc.desc: Test UsbFunctionsToString with the FUNCTION_NONE value
 * @tc.desc: Positive test: 0 converts back to the "none" function name
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, Functions004, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    std::string funcResult = client.UsbFunctionsToString(0);
    EXPECT_EQ(std::string("none"), funcResult);
}

/**
 * @tc.name: Functions005
 * @tc.desc: Test SetCurrentFunctions with the current function value
 * @tc.desc: Positive test: re-applying the current value is idempotent
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, Functions005, TestSize.Level2)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t funcs = 0;
    ASSERT_EQ(UEC_OK, client.GetCurrentFunctions(funcs));
    int32_t ret = client.SetCurrentFunctions(funcs);
    EXPECT_EQ(UEC_OK, ret);
}

/**
 * @tc.name: Functions006
 * @tc.desc: Test SetCurrentFunctions called by a normal native token without permission
 * @tc.desc: Negative test: rejected with UEC_SERVICE_PERMISSION_DENIED_SYSAPI
 *           before any function state is touched
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, Functions006, TestSize.Level1)
{
    SwitchToNormalCaller();
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.SetCurrentFunctions(0);
    EXPECT_EQ(UEC_SERVICE_PERMISSION_DENIED_SYSAPI, ret);
}
#endif // USB_MANAGER_FEATURE_DEVICE

/* ------------------------------------------------------------------------ *
 * Section K: Device connection listener registration
 * (UsbService::RegisterConnectionListener / UnRegisterConnectionListener)
 * ------------------------------------------------------------------------ */

#ifdef USB_MANAGER_FEATURE_HOST
/**
 * @tc.name: Listener001
 * @tc.desc: Test RegisterConnectionListener with a null callback
 * @tc.desc: Negative test: rejected with UEC_INTERFACE_INVALID_VALUE
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, Listener001, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    sptr<IUsbConnectionCallback> cb = nullptr;
    int32_t ret = client.RegisterConnectionListener(cb);
    EXPECT_EQ(UEC_INTERFACE_INVALID_VALUE, ret);
}

/**
 * @tc.name: Listener002
 * @tc.desc: Test UnRegisterConnectionListener with a null callback
 * @tc.desc: Negative test: rejected with UEC_INTERFACE_INVALID_VALUE
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, Listener002, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    sptr<IUsbConnectionCallback> cb = nullptr;
    int32_t ret = client.UnRegisterConnectionListener(cb);
    EXPECT_EQ(UEC_INTERFACE_INVALID_VALUE, ret);
}

/**
 * @tc.name: Listener003
 * @tc.desc: Test RegisterConnectionListener with a valid callback
 * @tc.desc: Positive test: registration returns UEC_OK
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, Listener003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    sptr<IUsbConnectionCallback> cb = new (std::nothrow) UsbTddConnectionCallback();
    ASSERT_NE(nullptr, cb);
    int32_t ret = client.RegisterConnectionListener(cb);
    EXPECT_EQ(UEC_OK, ret);
    client.UnRegisterConnectionListener(cb);
}

/**
 * @tc.name: Listener004
 * @tc.desc: Test registering the same callback object twice
 * @tc.desc: Negative test: the duplicate registration is rejected with
 *           UEC_SERVICE_ALREADY_EXISTS
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, Listener004, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    sptr<IUsbConnectionCallback> cb = new (std::nothrow) UsbTddConnectionCallback();
    ASSERT_NE(nullptr, cb);
    ASSERT_EQ(UEC_OK, client.RegisterConnectionListener(cb));
    int32_t ret = client.RegisterConnectionListener(cb);
    EXPECT_EQ(UEC_SERVICE_ALREADY_EXISTS, ret);
    client.UnRegisterConnectionListener(cb);
}

/**
 * @tc.name: Listener005
 * @tc.desc: Test unregistering a callback twice
 * @tc.desc: Positive test: the first unregister returns UEC_OK and the second
 *           one is tolerated with UEC_OK as well
 * @tc.type: FUNC
 */
HWTEST_F(UsbServiceTddTest, Listener005, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    sptr<IUsbConnectionCallback> cb = new (std::nothrow) UsbTddConnectionCallback();
    ASSERT_NE(nullptr, cb);
    ASSERT_EQ(UEC_OK, client.RegisterConnectionListener(cb));
    int32_t ret = client.UnRegisterConnectionListener(cb);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.UnRegisterConnectionListener(cb);
    EXPECT_EQ(UEC_OK, ret);
}
#endif // USB_MANAGER_FEATURE_HOST
} // namespace Service
} // namespace USB
} // namespace OHOS
