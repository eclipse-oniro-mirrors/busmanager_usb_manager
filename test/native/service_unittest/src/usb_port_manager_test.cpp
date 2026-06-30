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
#include <vector>
#include <mutex>

#include "usb_port_manager.h"
#include "usb_port.h"
#include "usb_errors.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

using namespace testing;

constexpr int32_t TEST_PORT_ID_1 = 0;
constexpr int32_t TEST_PORT_ID_2 = 1;
constexpr int32_t TEST_PORT_ID_INVALID = -1;
constexpr int32_t TEST_POWER_ROLE_NONE = 0;
constexpr int32_t TEST_POWER_ROLE_SOURCE = 1;
constexpr int32_t TEST_POWER_ROLE_SINK = 2;
constexpr int32_t TEST_DATA_ROLE_NONE = 0;
constexpr int32_t TEST_DATA_ROLE_HOST = 1;
constexpr int32_t TEST_DATA_ROLE_DEVICE = 2;
constexpr int32_t TEST_MODE_NONE = 0;
constexpr int32_t TEST_MODE_DFP = 1;
constexpr int32_t TEST_MODE_UFP = 2;
constexpr int32_t TEST_MODE_DRD = 3;
constexpr int32_t TEST_SUPPORT_MODES = 15;

class UsbPortManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::unique_ptr<UsbPortManager> usbPortManager_;
};

void UsbPortManagerTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerTest::SetUpTestCase enter");
}

void UsbPortManagerTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerTest::TearDownTestCase enter");
}

void UsbPortManagerTest::SetUp()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerTest::SetUp enter");
    usbPortManager_ = std::make_unique<UsbPortManager>();
    ASSERT_NE(usbPortManager_, nullptr);
    usbPortManager_->Init();
}

void UsbPortManagerTest::TearDown()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManagerTest::TearDown enter");
    usbPortManager_.reset();
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_Constructor_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_Constructor_001 enter");
    auto portManager = std::make_unique<UsbPortManager>();
    ASSERT_NE(portManager, nullptr);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_GetPorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_GetPorts_001 enter");
    std::vector<UsbPort> ports;
    int32_t ret = usbPortManager_->GetPorts(ports);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_AddPort_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_AddPort_001 enter");
    UsbPort port;
    port.id = TEST_PORT_ID_1;
    port.powerRole = TEST_POWER_ROLE_SINK;
    port.dataRole = TEST_DATA_ROLE_DEVICE;
    port.mode = TEST_MODE_UFP;
    usbPortManager_->AddPort(port);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_RemovePort_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_RemovePort_001 enter");
    usbPortManager_->RemovePort(TEST_PORT_ID_1);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_UpdatePort_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_UpdatePort_001 enter");
    usbPortManager_->UpdatePort(TEST_PORT_ID_1, TEST_POWER_ROLE_SINK, TEST_DATA_ROLE_DEVICE, TEST_MODE_UFP);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_UpdatePort_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_UpdatePort_002 enter");
    usbPortManager_->UpdatePort(TEST_PORT_ID_1, TEST_POWER_ROLE_SINK, TEST_DATA_ROLE_DEVICE,
        TEST_MODE_UFP, TEST_SUPPORT_MODES);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_GetSupportedModes_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_GetSupportedModes_001 enter");
    int32_t supportedModes = 0;
    int32_t ret = usbPortManager_->GetSupportedModes(TEST_PORT_ID_1, supportedModes);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_GetSupportedModes_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_GetSupportedModes_002 enter");
    int32_t supportedModes = 0;
    int32_t ret = usbPortManager_->GetSupportedModes(TEST_PORT_ID_INVALID, supportedModes);
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_QueryPort_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_QueryPort_001 enter");
    int32_t ret = usbPortManager_->QueryPort();
    ASSERT_GE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_MultiplePortOperations_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_MultiplePortOperations_001 enter");
    UsbPort port1, port2;
    port1.id = TEST_PORT_ID_1;
    port1.powerRole = TEST_POWER_ROLE_SOURCE;
    port1.dataRole = TEST_DATA_ROLE_HOST;
    port1.mode = TEST_MODE_DFP;

    port2.id = TEST_PORT_ID_2;
    port2.powerRole = TEST_POWER_ROLE_SINK;
    port2.dataRole = TEST_DATA_ROLE_DEVICE;
    port2.mode = TEST_MODE_UFP;

    usbPortManager_->AddPort(port1);
    usbPortManager_->AddPort(port2);

    usbPortManager_->UpdatePort(TEST_PORT_ID_1, TEST_POWER_ROLE_SINK, TEST_DATA_ROLE_DEVICE, TEST_MODE_UFP);
    usbPortManager_->UpdatePort(TEST_PORT_ID_2, TEST_POWER_ROLE_SOURCE, TEST_DATA_ROLE_HOST, TEST_MODE_DFP);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_ThreadSafety_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_ThreadSafety_001 enter");
    const int32_t numThreads = 10;
    std::vector<std::thread> threads;

    for (int32_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i]() {
            UsbPort port;
            port.id = i;
            port.powerRole = TEST_POWER_ROLE_SINK;
            port.dataRole = TEST_DATA_ROLE_DEVICE;
            port.mode = TEST_MODE_UFP;
            usbPortManager_->AddPort(port);
            usbPortManager_->UpdatePort(i, TEST_POWER_ROLE_SOURCE, TEST_DATA_ROLE_HOST, TEST_MODE_DFP);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_EdgeCase_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_EdgeCase_001 enter");
    UsbPort port;
    port.id = TEST_PORT_ID_1;
    port.powerRole = TEST_POWER_ROLE_NONE;
    port.dataRole = TEST_DATA_ROLE_NONE;
    port.mode = TEST_MODE_NONE;
    usbPortManager_->AddPort(port);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_EdgeCase_002, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_EdgeCase_002 enter");
    UsbPort port;
    port.id = TEST_PORT_ID_1;
    port.powerRole = TEST_POWER_ROLE_SOURCE;
    port.dataRole = TEST_DATA_ROLE_DEVICE;
    port.mode = TEST_MODE_DRD;
    usbPortManager_->AddPort(port);
}

HWTEST_F(UsbPortManagerTest, UsbPortManager_InvalidPortId_001, TestSize.Level2)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbPortManager_InvalidPortId_001 enter");
    usbPortManager_->RemovePort(TEST_PORT_ID_INVALID);
}

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS