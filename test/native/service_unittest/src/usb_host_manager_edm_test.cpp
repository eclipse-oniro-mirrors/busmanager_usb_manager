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
#include <thread>
#include <chrono>

#include "usb_host_manager.h"
#include "usb_device.h"
#include "usb_errors.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;

constexpr uint8_t TEST_BUS_NUM_1 = 1;
constexpr uint8_t TEST_BUS_NUM_2 = 2;
constexpr uint8_t TEST_DEV_ADDR_1 = 1;
constexpr uint8_t TEST_INTERFACE_ID_0 = 0;
constexpr uint8_t TEST_ENDPOINT_ADDR_1 = 1;

class UsbHostManagerEdmTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::unique_ptr<UsbHostManager> usbHostManager_;
};

void UsbHostManagerEdmTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerEdmTest::SetUpTestCase enter");
}

void UsbHostManagerEdmTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerEdmTest::TearDownTestCase enter");
}

void UsbHostManagerEdmTest::SetUp()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerEdmTest::SetUp enter");
    SystemAbility* systemAbility = nullptr;
    usbHostManager_ = std::make_unique<UsbHostManager>(systemAbility);
    ASSERT_NE(usbHostManager_, nullptr);
}

void UsbHostManagerEdmTest::TearDown()
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManagerEdmTest::TearDown enter");
    usbHostManager_.reset();
}

/**
 * @tc.name: UsbHostManager_EDM_ManageGlobalInterface_Enable
 * @tc.desc: Test EDM ManageGlobalInterface with disable=false (enable)
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ManageGlobalInterface_Enable, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageGlobalInterface_Enable start");

    bool disable = false;

    int32_t ret = usbHostManager_->ManageGlobalInterface(disable);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageGlobalInterface_Enable ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_EDM_ManageGlobalInterface_Disable
 * @tc.desc: Test EDM ManageGlobalInterface with disable=true (disable)
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ManageGlobalInterface_Disable, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageGlobalInterface_Disable start");

    bool disable = true;

    int32_t ret = usbHostManager_->ManageGlobalInterface(disable);

    EXPECT_EQ(ret, UEC_OK);

    bool enableAgain = false;
    int32_t ret2 = usbHostManager_->ManageGlobalInterface(enableAgain);
    EXPECT_EQ(ret2, UEC_OK);

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageGlobalInterface_Disable ret=%{public}d ret2=%{public}d",
                ret, ret2);
}

/**
 * @tc.name: UsbHostManager_EDM_ManageDevice_EnableDevice
 * @tc.desc: Test EDM ManageDevice to enable specific device
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ManageDevice_EnableDevice, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevice_EnableDevice start");

    int32_t vendorId = 0x1234;
    int32_t productId = 0x5678;
    bool disable = false;

    int32_t ret = usbHostManager_->ManageDevice(vendorId, productId, disable);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevice_EnableDevice ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_EDM_ManageDevice_DisableDevice
 * @tc.desc: Test EDM ManageDevice to disable specific device
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ManageDevice_DisableDevice, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevice_DisableDevice start");

    int32_t vendorId = 0xABCD;
    int32_t productId = 0xEF01;
    bool disable = true;

    int32_t ret = usbHostManager_->ManageDevice(vendorId, productId, disable);

    EXPECT_EQ(ret, UEC_OK);

    bool enableAgain = false;
    int32_t ret2 = usbHostManager_->ManageDevice(vendorId, productId, enableAgain);
    EXPECT_EQ(ret2, UEC_OK);

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevice_DisableDevice ret=%{public}d ret2=%{public}d",
                ret, ret2);
}

/**
 * @tc.name: UsbHostManager_EDM_ManageDevice_MultipleDevices
 * @tc.desc: Test EDM ManageDevice with multiple devices
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ManageDevice_MultipleDevices, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevice_MultipleDevices start");

    std::vector<std::pair<int32_t, int32_t>> devices = {
        {0x1234, 0x5678},
        {0xABCD, 0xEF01},
        {0x9999, 0x8888},
        {0x1111, 0x2222}
    };

    for (const auto& device : devices) {
        bool disable = true;
        int32_t ret = usbHostManager_->ManageDevice(device.first, device.second, disable);
        EXPECT_EQ(ret, UEC_OK);
        USB_HILOGI(MODULE_USB_HOST, "ManageDevice vid=%{public}x pid=%{public}x ret=%{public}d",
                    device.first, device.second, ret);
    }

    for (const auto& device : devices) {
        bool enable = false;
        int32_t ret = usbHostManager_->ManageDevice(device.first, device.second, enable);
        EXPECT_EQ(ret, UEC_OK);
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevice_MultipleDevices end");
}

/**
 * @tc.name: UsbHostManager_EDM_ManageDevicePolicy_EmptyTrustList
 * @tc.desc: Test EDM ManageDevicePolicy with empty trust list
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ManageDevicePolicy_EmptyTrustList, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevicePolicy_EmptyTrustList start");

    std::vector<UsbDeviceId> trustList;

    int32_t ret = usbHostManager_->ManageDevicePolicy(trustList);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevicePolicy_EmptyTrustList ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_EDM_ManageDevicePolicy_SingleDevice
 * @tc.desc: Test EDM ManageDevicePolicy with single device in trust list
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ManageDevicePolicy_SingleDevice, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevicePolicy_SingleDevice start");

    std::vector<UsbDeviceId> trustList;
    UsbDeviceId devId = {0x1234, 0x5678};
    trustList.push_back(devId);

    int32_t ret = usbHostManager_->ManageDevicePolicy(trustList);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevicePolicy_SingleDevice ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_EDM_ManageDevicePolicy_MultipleDevices
 * @tc.desc: Test EDM ManageDevicePolicy with multiple devices in trust list
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ManageDevicePolicy_MultipleDevices, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevicePolicy_MultipleDevices start");

    std::vector<UsbDeviceId> trustList;
    trustList.push_back({0x1234, 0x5678});
    trustList.push_back({0xABCD, 0xEF01});
    trustList.push_back({0x9999, 0x8888});
    trustList.push_back({0x1111, 0x2222});
    trustList.push_back({0x3333, 0x4444});

    int32_t ret = usbHostManager_->ManageDevicePolicy(trustList);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageDevicePolicy_MultipleDevices ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_EDM_ManageInterfaceType_SingleType
 * @tc.desc: Test EDM ManageInterfaceType with single interface type
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ManageInterfaceType_SingleType, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageInterfaceType_SingleType start");

    std::vector<UsbDeviceType> disableType;
    UsbDeviceType type = {0x08, 0x01, 0x00, 0};
    disableType.push_back(type);

    bool disable = true;

    int32_t ret = usbHostManager_->ManageInterfaceType(disableType, disable);

    EXPECT_EQ(ret, UEC_OK);

    bool enableAgain = false;
    int32_t ret2 = usbHostManager_->ManageInterfaceType(disableType, enableAgain);
    EXPECT_EQ(ret2, UEC_OK);

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageInterfaceType_SingleType ret=%{public}d ret2=%{public}d",
                ret, ret2);
}

/**
 * @tc.name: UsbHostManager_EDM_ManageInterfaceType_MultipleTypes
 * @tc.desc: Test EDM ManageInterfaceType with multiple interface types
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ManageInterfaceType_MultipleTypes, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageInterfaceType_MultipleTypes start");

    std::vector<UsbDeviceType> disableType;
    disableType.push_back({0x08, 0x01, 0x00, 0});  // Mass Storage
    disableType.push_back({0x03, 0x01, 0x01, 0});  // Keyboard
    disableType.push_back({0x03, 0x01, 0x02, 0});  // Mouse
    disableType.push_back({0x0E, 0x01, 0x00, 0});  // Video

    bool disable = true;

    int32_t ret = usbHostManager_->ManageInterfaceType(disableType, disable);

    EXPECT_EQ(ret, UEC_OK);

    bool enableAgain = false;
    int32_t ret2 = usbHostManager_->ManageInterfaceType(disableType, enableAgain);
    EXPECT_EQ(ret2, UEC_OK);

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageInterfaceType_MultipleTypes ret=%{public}d ret2=%{public}d",
                ret, ret2);
}

/**
 * @tc.name: UsbHostManager_EDM_ManageInterfaceType_UnknownType
 * @tc.desc: Test EDM ManageInterfaceType with unknown interface type
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ManageInterfaceType_UnknownType, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageInterfaceType_UnknownType start");

    std::vector<UsbDeviceType> disableType;
    UsbDeviceType type = {0xFF, 0xFF, 0xFF, 0};
    disableType.push_back(type);

    bool disable = true;

    int32_t ret = usbHostManager_->ManageInterfaceType(disableType, disable);

    EXPECT_EQ(ret, UEC_OK);

    bool enableAgain = false;
    int32_t ret2 = usbHostManager_->ManageInterfaceType(disableType, enableAgain);
    EXPECT_EQ(ret2, UEC_OK);

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ManageInterfaceType_UnknownType ret=%{public}d ret2=%{public}d",
                ret, ret2);
}

/**
 * @tc.name: UsbHostManager_EDM_PolicyToggle_RapidSwitch
 * @tc.desc: Test rapid switching between enable/disable policies
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_PolicyToggle_RapidSwitch, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_PolicyToggle_RapidSwitch start");

    bool disable = true;
    for (int i = 0; i < 10; i++) {
        int32_t ret = usbHostManager_->ManageGlobalInterface(disable);
        EXPECT_EQ(ret, UEC_OK);

        disable = !disable;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_PolicyToggle_RapidSwitch end");
}

/**
 * @tc.name: UsbHostManager_EDM_CompositePolicy_DeviceAndInterface
 * @tc.desc: Test composite EDM policy with both device and interface restrictions
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_CompositePolicy_DeviceAndInterface, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_CompositePolicy_DeviceAndInterface start");

    std::vector<UsbDeviceId> trustList;
    trustList.push_back({0x1234, 0x5678});

    std::vector<UsbDeviceType> disableType;
    disableType.push_back({0x03, 0x01, 0x01, 0});

    int32_t ret1 = usbHostManager_->ManageDevicePolicy(trustList);
    EXPECT_EQ(ret1, UEC_OK);

    int32_t ret2 = usbHostManager_->ManageInterfaceType(disableType, true);
    EXPECT_EQ(ret2, UEC_OK);

    int32_t ret3 = usbHostManager_->ManageInterfaceType(disableType, false);
    EXPECT_EQ(ret3, UEC_OK);

    USB_HILOGI(MODULE_USB_HOST, "ret1=%{public}d ret2=%{public}d ret3=%{public}d",
                ret1, ret2, ret3);
}

/**
 * @tc.name: UsbHostManager_EDM_LargeTrustList
 * @tc.desc: Test EDM ManageDevicePolicy with large trust list
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_LargeTrustList, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_LargeTrustList start");

    std::vector<UsbDeviceId> trustList;

    for (int i = 0; i < 500; i++) {
        UsbDeviceId devId = {0x1000 + (i % 0xFFFF), 0x2000 + (i % 0xFFFF)};
        trustList.push_back(devId);
    }

    int32_t ret = usbHostManager_->ManageDevicePolicy(trustList);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_LargeTrustList ret=%{public}d size=%{public}zu",
                ret, trustList.size());
}

/**
 * @tc.name: UsbHostManager_EDM_ZeroVidPid
 * @tc.desc: Test EDM ManageDevice with zero VID and PID
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_ZeroVidPid, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ZeroVidPid start");

    int32_t vendorId = 0;
    int32_t productId = 0;
    bool disable = true;

    int32_t ret = usbHostManager_->ManageDevice(vendorId, productId, disable);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_ZeroVidPid ret=%{public}d", ret);
}

/**
 * @tc.name: UsbHostManager_EDM_MaxVidPid
 * @tc.desc: Test EDM ManageDevice with maximum VID and PID
 * @tc.type: FUNC
 */
HWTEST_F(UsbHostManagerEdmTest, UsbHostManager_EDM_MaxVidPid, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_MaxVidPid start");

    int32_t vendorId = 0xFFFF;
    int32_t productId = 0xFFFF;
    bool disable = false;

    int32_t ret = usbHostManager_->ManageDevice(vendorId, productId, disable);

    EXPECT_EQ(ret, UEC_OK);
    USB_HILOGI(MODULE_USB_HOST, "UsbHostManager_EDM_MaxVidPid ret=%{public}d", ret);
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS