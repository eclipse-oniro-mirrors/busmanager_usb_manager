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

#include "usb_port_manager_extended_test.h"

#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "hilog_wrapper.h"
#include "usb_srv_support.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace USB {
namespace ExtendedTest {
namespace {
constexpr int32_t PORT_ID_1 = 1;
constexpr int32_t PORT_ID_2 = 2;
constexpr int32_t PORT_ID_3 = 3;
constexpr int32_t PORT_ID_10 = 10;
constexpr int32_t INVALID_PORT_ID = 100;
constexpr int32_t NEGATIVE_PORT_ID = -1;
constexpr int32_t SUPPORTED_MODES_ALL = 3;
constexpr int32_t SUPPORTED_MODES_HOST_ONLY = 2;
constexpr int32_t SUPPORTED_MODES_DEVICE_ONLY = 1;
constexpr int32_t INVALID_ROLE = 5;
constexpr int32_t STRESS_LOOP_COUNT = 100;
constexpr int32_t BULK_PORT_COUNT = 50;
constexpr int32_t THREAD_COUNT = 8;
constexpr int32_t THREAD_ITERATION_COUNT = 20;
constexpr size_t SIZE_ZERO = 0;
constexpr size_t SIZE_ONE = 1;
constexpr size_t SIZE_TWO = 2;
constexpr size_t SIZE_THREE = 3;
} // namespace

void UsbPortManagerExtendedTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_PORT, "Start UsbPortManagerExtendedTest");
}

void UsbPortManagerExtendedTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_PORT, "End UsbPortManagerExtendedTest");
}

void UsbPortManagerExtendedTest::SetUp(void)
{
    portManager_ = std::make_unique<UsbPortManager>();
    ASSERT_NE(portManager_, nullptr);
}

void UsbPortManagerExtendedTest::TearDown(void)
{
    portManager_.reset();
}

UsbPort UsbPortManagerExtendedTest::BuildPort(int32_t portId, int32_t supportedModes, int32_t currentMode,
    int32_t powerRole, int32_t dataRole)
{
    UsbPort port;
    port.id = portId;
    port.supportedModes = supportedModes;
    port.usbPortStatus.currentMode = currentMode;
    port.usbPortStatus.currentPowerRole = powerRole;
    port.usbPortStatus.currentDataRole = dataRole;
    return port;
}

void UsbPortManagerExtendedTest::AddDefaultPorts()
{
    UsbPort port1 = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    UsbPort port2 = BuildPort(PORT_ID_2, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port1);
    portManager_->AddPort(port2);
}

int32_t UsbPortManagerExtendedTest::FindPortIndex(const std::vector<UsbPort> &ports, int32_t portId)
{
    for (size_t i = 0; i < ports.size(); i++) {
        if (ports[i].id == portId) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

void UsbPortManagerExtendedTest::ConcurrentAddPortsWorker(int32_t threadIndex)
{
    for (int32_t i = 0; i < THREAD_ITERATION_COUNT; i++) {
        int32_t portId = threadIndex * THREAD_ITERATION_COUNT + i + 1;
        UsbPort port = BuildPort(portId, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        std::lock_guard<std::mutex> lock(extendedMutex_);
        portManager_->AddPort(port);
    }
}

void UsbPortManagerExtendedTest::ConcurrentGetPortsWorker(std::atomic<int32_t> &okCount)
{
    for (int32_t i = 0; i < THREAD_ITERATION_COUNT; i++) {
        std::vector<UsbPort> ports;
        if (portManager_->GetPorts(ports) == UEC_OK && ports.size() == SIZE_TWO) {
            okCount++;
        }
    }
}

void UsbPortManagerExtendedTest::ConcurrentAddAndRemoveWorker(int32_t threadIndex)
{
    for (int32_t i = 0; i < THREAD_ITERATION_COUNT; i++) {
        int32_t portId = threadIndex * THREAD_ITERATION_COUNT + i + 1;
        UsbPort port = BuildPort(portId, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        std::lock_guard<std::mutex> lock(extendedMutex_);
        portManager_->AddPort(port);
        portManager_->RemovePort(portId);
    }
}

HWTEST_F(UsbPortManagerExtendedTest, Constructor_Default_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Constructor_Default_001");
    auto manager = std::make_unique<UsbPortManager>();
    ASSERT_NE(manager, nullptr);
    std::vector<UsbPort> ports;
    int32_t ret = manager->GetPorts(ports);
    EXPECT_NE(ret, UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ZERO);
}

HWTEST_F(UsbPortManagerExtendedTest, Constructor_MultipleInstances_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Constructor_MultipleInstances_001");
    auto managerA = std::make_unique<UsbPortManager>();
    auto managerB = std::make_unique<UsbPortManager>();
    ASSERT_NE(managerA, nullptr);
    ASSERT_NE(managerB, nullptr);
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    managerA->AddPort(port);
    std::vector<UsbPort> portsA;
    std::vector<UsbPort> portsB;
    EXPECT_EQ(managerA->GetPorts(portsA), UEC_OK);
    EXPECT_NE(managerB->GetPorts(portsB), UEC_OK);
    EXPECT_EQ(portsA.size(), SIZE_ONE);
    EXPECT_EQ(portsB.size(), SIZE_ZERO);
}

HWTEST_F(UsbPortManagerExtendedTest, Destructor_ScopeExit_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Destructor_ScopeExit_001");
    {
        auto manager = std::make_unique<UsbPortManager>();
        ASSERT_NE(manager, nullptr);
        UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        manager->AddPort(port);
    }
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Destructor_Reset_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Destructor_Reset_001");
    auto manager = std::make_unique<UsbPortManager>();
    ASSERT_NE(manager, nullptr);
    manager.reset();
    EXPECT_EQ(manager, nullptr);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_SinglePort_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_SinglePort_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ONE);
    EXPECT_NE(FindPortIndex(ports, PORT_ID_1), -1);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_MultiplePorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_MultiplePorts_001");
    AddDefaultPorts();
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_TWO);
    EXPECT_NE(FindPortIndex(ports, PORT_ID_1), -1);
    EXPECT_NE(FindPortIndex(ports, PORT_ID_2), -1);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_DuplicateId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_DuplicateId_001");
    UsbPort port1 = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    UsbPort port2 = BuildPort(PORT_ID_1, SUPPORTED_MODES_HOST_ONLY,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port1);
    portManager_->AddPort(port2);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ONE);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_DuplicateIdKeepsOriginal_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_DuplicateIdKeepsOriginal_001");
    UsbPort port1 = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    UsbPort port2 = BuildPort(PORT_ID_1, SUPPORTED_MODES_HOST_ONLY,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port1);
    portManager_->AddPort(port2);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    int32_t index = FindPortIndex(ports, PORT_ID_1);
    ASSERT_NE(index, -1);
    EXPECT_EQ(ports[static_cast<size_t>(index)].supportedModes, SUPPORTED_MODES_ALL);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_DEVICE);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_ZeroId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_ZeroId_001");
    UsbPort port = BuildPort(0, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ONE);
    EXPECT_EQ(ports[0].id, 0);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_NegativeId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_NegativeId_001");
    UsbPort port = BuildPort(NEGATIVE_PORT_ID, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ONE);
    EXPECT_EQ(ports[0].id, NEGATIVE_PORT_ID);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_MaxInt32Id_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_MaxInt32Id_001");
    UsbPort port = BuildPort(INT32_MAX, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ONE);
    EXPECT_EQ(ports[0].id, INT32_MAX);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_MinInt32Id_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_MinInt32Id_001");
    UsbPort port = BuildPort(INT32_MIN, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ONE);
    EXPECT_EQ(ports[0].id, INT32_MIN);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_BulkPorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_BulkPorts_001");
    for (int32_t i = 1; i <= BULK_PORT_COUNT; i++) {
        UsbPort port = BuildPort(i, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        portManager_->AddPort(port);
    }
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(static_cast<int32_t>(ports.size()), BULK_PORT_COUNT);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_AfterRemove_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_AfterRemove_001");
    UsbPort port1 = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port1);
    portManager_->RemovePort(PORT_ID_1);
    UsbPort port2 = BuildPort(PORT_ID_1, SUPPORTED_MODES_HOST_ONLY,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port2);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ONE);
    EXPECT_EQ(ports[0].supportedModes, SUPPORTED_MODES_HOST_ONLY);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_OrderedByKey_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_OrderedByKey_001");
    UsbPort port3 = BuildPort(PORT_ID_3, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    UsbPort port1 = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    UsbPort port2 = BuildPort(PORT_ID_2, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port3);
    portManager_->AddPort(port1);
    portManager_->AddPort(port2);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    ASSERT_EQ(ports.size(), SIZE_THREE);
    EXPECT_EQ(ports[0].id, PORT_ID_1);
    EXPECT_EQ(ports[1].id, PORT_ID_2);
    EXPECT_EQ(ports[2].id, PORT_ID_3);
}

HWTEST_F(UsbPortManagerExtendedTest, AddPort_StatusFieldsPreserved_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : AddPort_StatusFieldsPreserved_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_HOST_ONLY,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    ASSERT_EQ(ports.size(), SIZE_ONE);
    EXPECT_EQ(ports[0].id, PORT_ID_1);
    EXPECT_EQ(ports[0].supportedModes, SUPPORTED_MODES_HOST_ONLY);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
    EXPECT_EQ(ports[0].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_SOURCE);
    EXPECT_EQ(ports[0].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_HOST);
}

HWTEST_F(UsbPortManagerExtendedTest, RemovePort_Existing_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : RemovePort_Existing_001");
    AddDefaultPorts();
    portManager_->RemovePort(PORT_ID_1);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ONE);
    EXPECT_EQ(FindPortIndex(ports, PORT_ID_1), -1);
    EXPECT_NE(FindPortIndex(ports, PORT_ID_2), -1);
}

HWTEST_F(UsbPortManagerExtendedTest, RemovePort_NonExistent_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : RemovePort_NonExistent_001");
    AddDefaultPorts();
    portManager_->RemovePort(INVALID_PORT_ID);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_TWO);
}

HWTEST_F(UsbPortManagerExtendedTest, RemovePort_DoubleRemove_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : RemovePort_DoubleRemove_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->RemovePort(PORT_ID_1);
    portManager_->RemovePort(PORT_ID_1);
    std::vector<UsbPort> ports;
    EXPECT_NE(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ZERO);
}

HWTEST_F(UsbPortManagerExtendedTest, RemovePort_AllPorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : RemovePort_AllPorts_001");
    AddDefaultPorts();
    portManager_->RemovePort(PORT_ID_1);
    portManager_->RemovePort(PORT_ID_2);
    std::vector<UsbPort> ports;
    EXPECT_NE(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ZERO);
}

HWTEST_F(UsbPortManagerExtendedTest, RemovePort_ZeroId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : RemovePort_ZeroId_001");
    UsbPort port = BuildPort(0, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->RemovePort(0);
    std::vector<UsbPort> ports;
    EXPECT_NE(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ZERO);
}

HWTEST_F(UsbPortManagerExtendedTest, RemovePort_NegativeId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : RemovePort_NegativeId_001");
    UsbPort port = BuildPort(NEGATIVE_PORT_ID, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->RemovePort(NEGATIVE_PORT_ID);
    std::vector<UsbPort> ports;
    EXPECT_NE(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ZERO);
}

HWTEST_F(UsbPortManagerExtendedTest, RemovePort_MaxInt32Id_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : RemovePort_MaxInt32Id_001");
    UsbPort port = BuildPort(INT32_MAX, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->RemovePort(INT32_MAX);
    std::vector<UsbPort> ports;
    EXPECT_NE(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ZERO);
}

HWTEST_F(UsbPortManagerExtendedTest, RemovePort_DoesNotAffectOthers_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : RemovePort_DoesNotAffectOthers_001");
    AddDefaultPorts();
    portManager_->RemovePort(PORT_ID_1);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    ASSERT_EQ(ports.size(), SIZE_ONE);
    EXPECT_EQ(ports[0].id, PORT_ID_2);
    EXPECT_EQ(ports[0].supportedModes, SUPPORTED_MODES_ALL);
    EXPECT_EQ(ports[0].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_HOST);
}

HWTEST_F(UsbPortManagerExtendedTest, RemovePort_Bulk_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : RemovePort_Bulk_001");
    for (int32_t i = 1; i <= BULK_PORT_COUNT; i++) {
        UsbPort port = BuildPort(i, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        portManager_->AddPort(port);
    }
    for (int32_t i = 1; i <= BULK_PORT_COUNT; i++) {
        portManager_->RemovePort(i);
    }
    std::vector<UsbPort> ports;
    EXPECT_NE(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ZERO);
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_Empty_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_Empty_001");
    std::vector<UsbPort> ports;
    int32_t ret = portManager_->GetPorts(ports);
    EXPECT_NE(ret, UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ZERO);
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_SinglePort_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_SinglePort_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ONE);
    EXPECT_EQ(ports[0].id, PORT_ID_1);
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_MultiplePorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_MultiplePorts_001");
    AddDefaultPorts();
    UsbPort port3 = BuildPort(PORT_ID_3, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port3);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_THREE);
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_AppendsToVector_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_AppendsToVector_001");
    AddDefaultPorts();
    std::vector<UsbPort> ports;
    UsbPort placeholder = BuildPort(INVALID_PORT_ID, 0,
        UsbSrvSupport::PORT_MODE_NONE, UsbSrvSupport::POWER_ROLE_NONE, UsbSrvSupport::DTA_ROLE_NONE);
    ports.push_back(placeholder);
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_THREE);
    EXPECT_EQ(ports[0].id, INVALID_PORT_ID);
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_OrderedById_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_OrderedById_001");
    for (int32_t i = BULK_PORT_COUNT; i >= 1; i--) {
        UsbPort port = BuildPort(i, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        portManager_->AddPort(port);
    }
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    ASSERT_EQ(static_cast<int32_t>(ports.size()), BULK_PORT_COUNT);
    for (size_t i = 1; i < ports.size(); i++) {
        EXPECT_LT(ports[i - 1].id, ports[i].id);
    }
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_MultipleCalls_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_MultipleCalls_001");
    AddDefaultPorts();
    for (int32_t i = 0; i < 5; i++) {
        std::vector<UsbPort> ports;
        ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
        EXPECT_EQ(ports.size(), SIZE_TWO);
    }
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_AfterRemove_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_AfterRemove_001");
    AddDefaultPorts();
    portManager_->RemovePort(PORT_ID_2);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_ONE);
    EXPECT_EQ(ports[0].id, PORT_ID_1);
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_FieldsMatch_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_FieldsMatch_001");
    UsbPort port = BuildPort(PORT_ID_2, SUPPORTED_MODES_DEVICE_ONLY,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].id, PORT_ID_2);
    EXPECT_EQ(ports[0].supportedModes, SUPPORTED_MODES_DEVICE_ONLY);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_DEVICE);
    EXPECT_EQ(ports[0].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_SINK);
    EXPECT_EQ(ports[0].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_DEVICE);
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_DoesNotExposeInternalState_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_DoesNotExposeInternalState_001");
    AddDefaultPorts();
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    ports.clear();
    ports.push_back(BuildPort(INVALID_PORT_ID, 0,
        UsbSrvSupport::PORT_MODE_NONE, UsbSrvSupport::POWER_ROLE_NONE, UsbSrvSupport::DTA_ROLE_NONE));
    std::vector<UsbPort> portsAgain;
    ASSERT_EQ(portManager_->GetPorts(portsAgain), UEC_OK);
    EXPECT_EQ(portsAgain.size(), SIZE_TWO);
    EXPECT_EQ(FindPortIndex(portsAgain, INVALID_PORT_ID), -1);
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_BulkPorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_BulkPorts_001");
    for (int32_t i = 1; i <= BULK_PORT_COUNT; i++) {
        UsbPort port = BuildPort(i, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        portManager_->AddPort(port);
    }
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(static_cast<int32_t>(ports.size()), BULK_PORT_COUNT);
    for (int32_t i = 0; i < BULK_PORT_COUNT; i++) {
        EXPECT_EQ(ports[static_cast<size_t>(i)].id, i + 1);
    }
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_AfterDuplicateAdd_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_AfterDuplicateAdd_001");
    for (int32_t round = 0; round < 3; round++) {
        UsbPort port1 = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        UsbPort port2 = BuildPort(PORT_ID_2, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
        portManager_->AddPort(port1);
        portManager_->AddPort(port2);
    }
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_TWO);
}

HWTEST_F(UsbPortManagerExtendedTest, GetPorts_Stress_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetPorts_Stress_001");
    for (int32_t i = 0; i < STRESS_LOOP_COUNT; i++) {
        UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        portManager_->AddPort(port);
        std::vector<UsbPort> ports;
        ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
        EXPECT_EQ(ports.size(), SIZE_ONE);
        portManager_->RemovePort(PORT_ID_1);
    }
}

HWTEST_F(UsbPortManagerExtendedTest, GetSupportedModes_WithoutInit_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetSupportedModes_WithoutInit_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    int32_t supportedModes = 0;
    int32_t ret = portManager_->GetSupportedModes(PORT_ID_1, supportedModes);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, GetSupportedModes_AfterInit_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetSupportedModes_AfterInit_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->Init();
    int32_t supportedModes = 0;
    int32_t ret = portManager_->GetSupportedModes(PORT_ID_1, supportedModes);
    EXPECT_EQ(ret, UEC_OK);
    EXPECT_EQ(supportedModes, SUPPORTED_MODES_ALL);
}

HWTEST_F(UsbPortManagerExtendedTest, GetSupportedModes_NonExistentId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetSupportedModes_NonExistentId_001");
    AddDefaultPorts();
    portManager_->Init();
    int32_t supportedModes = 0;
    EXPECT_NE(portManager_->GetSupportedModes(INVALID_PORT_ID, supportedModes), UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, GetSupportedModes_ZeroId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetSupportedModes_ZeroId_001");
    UsbPort port = BuildPort(0, SUPPORTED_MODES_HOST_ONLY,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port);
    portManager_->Init();
    int32_t supportedModes = 0;
    ASSERT_EQ(portManager_->GetSupportedModes(0, supportedModes), UEC_OK);
    EXPECT_EQ(supportedModes, SUPPORTED_MODES_HOST_ONLY);
}

HWTEST_F(UsbPortManagerExtendedTest, GetSupportedModes_NegativeId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetSupportedModes_NegativeId_001");
    UsbPort port = BuildPort(NEGATIVE_PORT_ID, SUPPORTED_MODES_DEVICE_ONLY,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->Init();
    int32_t supportedModes = 0;
    ASSERT_EQ(portManager_->GetSupportedModes(NEGATIVE_PORT_ID, supportedModes), UEC_OK);
    EXPECT_EQ(supportedModes, SUPPORTED_MODES_DEVICE_ONLY);
}

HWTEST_F(UsbPortManagerExtendedTest, GetSupportedModes_MaxInt32Id_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetSupportedModes_MaxInt32Id_001");
    UsbPort port = BuildPort(INT32_MAX, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->Init();
    int32_t supportedModes = 0;
    EXPECT_EQ(portManager_->GetSupportedModes(INT32_MAX, supportedModes), UEC_OK);
    EXPECT_EQ(supportedModes, SUPPORTED_MODES_ALL);
}

HWTEST_F(UsbPortManagerExtendedTest, GetSupportedModes_MultiplePorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetSupportedModes_MultiplePorts_001");
    UsbPort port1 = BuildPort(PORT_ID_1, SUPPORTED_MODES_HOST_ONLY,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    UsbPort port2 = BuildPort(PORT_ID_2, SUPPORTED_MODES_DEVICE_ONLY,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port1);
    portManager_->AddPort(port2);
    portManager_->Init();
    int32_t modes1 = 0;
    int32_t modes2 = 0;
    ASSERT_EQ(portManager_->GetSupportedModes(PORT_ID_1, modes1), UEC_OK);
    ASSERT_EQ(portManager_->GetSupportedModes(PORT_ID_2, modes2), UEC_OK);
    EXPECT_EQ(modes1, SUPPORTED_MODES_HOST_ONLY);
    EXPECT_EQ(modes2, SUPPORTED_MODES_DEVICE_ONLY);
}

HWTEST_F(UsbPortManagerExtendedTest, Init_NoHdi_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Init_NoHdi_001");
    portManager_->Init();
    int32_t supportedModes = 0;
    EXPECT_NE(portManager_->GetSupportedModes(PORT_ID_1, supportedModes), UEC_OK);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Init_MultipleCalls_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Init_MultipleCalls_001");
    AddDefaultPorts();
    for (int32_t i = 0; i < 3; i++) {
        portManager_->Init();
    }
    int32_t supportedModes = 0;
    ASSERT_EQ(portManager_->GetSupportedModes(PORT_ID_1, supportedModes), UEC_OK);
    EXPECT_EQ(supportedModes, SUPPORTED_MODES_ALL);
}

HWTEST_F(UsbPortManagerExtendedTest, Init_AfterAddPort_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Init_AfterAddPort_001");
    for (int32_t i = 1; i <= BULK_PORT_COUNT; i++) {
        UsbPort port = BuildPort(i, i % SUPPORTED_MODES_ALL + 1,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        portManager_->AddPort(port);
    }
    portManager_->Init();
    for (int32_t i = 1; i <= BULK_PORT_COUNT; i++) {
        int32_t supportedModes = 0;
        ASSERT_EQ(portManager_->GetSupportedModes(i, supportedModes), UEC_OK);
        EXPECT_EQ(supportedModes, i % SUPPORTED_MODES_ALL + 1);
    }
}

HWTEST_F(UsbPortManagerExtendedTest, Init_ResyncsAfterRemove_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Init_ResyncsAfterRemove_001");
    AddDefaultPorts();
    portManager_->Init();
    int32_t supportedModes = 0;
    ASSERT_EQ(portManager_->GetSupportedModes(PORT_ID_1, supportedModes), UEC_OK);
    portManager_->RemovePort(PORT_ID_1);
    portManager_->Init();
    EXPECT_NE(portManager_->GetSupportedModes(PORT_ID_1, supportedModes), UEC_OK);
    EXPECT_EQ(portManager_->GetSupportedModes(PORT_ID_2, supportedModes), UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, GetSupportedModes_ValueMatches_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetSupportedModes_ValueMatches_001");
    UsbPort port = BuildPort(PORT_ID_10, SUPPORTED_MODES_HOST_ONLY,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port);
    portManager_->Init();
    int32_t supportedModes = 0;
    ASSERT_EQ(portManager_->GetSupportedModes(PORT_ID_10, supportedModes), UEC_OK);
    EXPECT_EQ(supportedModes, SUPPORTED_MODES_HOST_ONLY);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    int32_t index = FindPortIndex(ports, PORT_ID_10);
    ASSERT_NE(index, -1);
    EXPECT_EQ(ports[static_cast<size_t>(index)].supportedModes, supportedModes);
}

HWTEST_F(UsbPortManagerExtendedTest, QueryPort_NoHdi_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : QueryPort_NoHdi_001");
    int32_t ret = portManager_->QueryPort();
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, QueryPort_MultipleCalls_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : QueryPort_MultipleCalls_001");
    for (int32_t i = 0; i < 5; i++) {
        EXPECT_NE(portManager_->QueryPort(), UEC_OK);
    }
}

HWTEST_F(UsbPortManagerExtendedTest, QueryPort_DoesNotAffectAddedPorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : QueryPort_DoesNotAffectAddedPorts_001");
    AddDefaultPorts();
    EXPECT_NE(portManager_->QueryPort(), UEC_OK);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_TWO);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_HostRole_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_HostRole_001");
    AddDefaultPorts();
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    int32_t index = FindPortIndex(ports, PORT_ID_1);
    ASSERT_NE(index, -1);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_SOURCE);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_DeviceRole_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_DeviceRole_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port);
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SINK,
        UsbSrvSupport::DATA_ROLE_DEVICE, UsbSrvSupport::PORT_MODE_DEVICE);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_SINK);
    EXPECT_EQ(ports[0].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_DEVICE);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_DEVICE);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_NoneRole_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_NoneRole_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port);
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_NONE,
        UsbSrvSupport::DTA_ROLE_NONE, UsbSrvSupport::PORT_MODE_NONE);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_NONE);
    EXPECT_EQ(ports[0].usbPortStatus.currentDataRole, UsbSrvSupport::DTA_ROLE_NONE);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_PowerRoleSource_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_PowerRoleSource_001");
    AddDefaultPorts();
    portManager_->UpdatePort(PORT_ID_2, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    int32_t index = FindPortIndex(ports, PORT_ID_2);
    ASSERT_NE(index, -1);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_SOURCE);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_PowerRoleSink_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_PowerRoleSink_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port);
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SINK,
        UsbSrvSupport::DATA_ROLE_DEVICE, UsbSrvSupport::PORT_MODE_DEVICE);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_SINK);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_NonExistent_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_NonExistent_001");
    AddDefaultPorts();
    portManager_->UpdatePort(INVALID_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_TWO);
    EXPECT_EQ(FindPortIndex(ports, INVALID_PORT_ID), -1);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_ModeParamIgnored_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_ModeParamIgnored_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_DEVICE);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_RepeatedUpdate_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_RepeatedUpdate_001");
    AddDefaultPorts();
    for (int32_t i = 0; i < 10; i++) {
        portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
            UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    }
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    int32_t index = FindPortIndex(ports, PORT_ID_1);
    ASSERT_NE(index, -1);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_SwitchHostToDevice_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_SwitchHostToDevice_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port);
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SINK,
        UsbSrvSupport::DATA_ROLE_DEVICE, UsbSrvSupport::PORT_MODE_DEVICE);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_DEVICE);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_DEVICE);
    EXPECT_EQ(ports[0].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_SINK);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_SwitchDeviceToHost_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_SwitchDeviceToHost_001");
    AddDefaultPorts();
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    int32_t index = FindPortIndex(ports, PORT_ID_1);
    ASSERT_NE(index, -1);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_SOURCE);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_DoesNotAffectOthers_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_DoesNotAffectOthers_001");
    AddDefaultPorts();
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    int32_t index2 = FindPortIndex(ports, PORT_ID_2);
    ASSERT_NE(index2, -1);
    EXPECT_EQ(ports[static_cast<size_t>(index2)].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_EQ(ports[static_cast<size_t>(index2)].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
    EXPECT_EQ(ports[static_cast<size_t>(index2)].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_SOURCE);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePort_MultiplePorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePort_MultiplePorts_001");
    AddDefaultPorts();
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    portManager_->UpdatePort(PORT_ID_2, UsbSrvSupport::POWER_ROLE_SINK,
        UsbSrvSupport::DATA_ROLE_DEVICE, UsbSrvSupport::PORT_MODE_DEVICE);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    ASSERT_EQ(ports.size(), SIZE_TWO);
    EXPECT_EQ(ports[0].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
    EXPECT_EQ(ports[1].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_DEVICE);
    EXPECT_EQ(ports[1].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_DEVICE);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePortWithModes_HostRole_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePortWithModes_HostRole_001");
    AddDefaultPorts();
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST, SUPPORTED_MODES_ALL);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    int32_t index = FindPortIndex(ports, PORT_ID_1);
    ASSERT_NE(index, -1);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_SOURCE);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePortWithModes_DeviceRole_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePortWithModes_DeviceRole_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port);
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SINK,
        UsbSrvSupport::DATA_ROLE_DEVICE, UsbSrvSupport::PORT_MODE_DEVICE, SUPPORTED_MODES_ALL);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_SINK);
    EXPECT_EQ(ports[0].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_DEVICE);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_DEVICE);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePortWithModes_NoneRole_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePortWithModes_NoneRole_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_NONE,
        UsbSrvSupport::DTA_ROLE_NONE, UsbSrvSupport::PORT_MODE_NONE, SUPPORTED_MODES_ALL);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].usbPortStatus.currentPowerRole, UsbSrvSupport::POWER_ROLE_NONE);
    EXPECT_EQ(ports[0].usbPortStatus.currentDataRole, UsbSrvSupport::DTA_ROLE_NONE);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_DEVICE);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePortWithModes_NonExistent_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePortWithModes_NonExistent_001");
    AddDefaultPorts();
    portManager_->UpdatePort(INVALID_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST, SUPPORTED_MODES_ALL);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_TWO);
    EXPECT_EQ(FindPortIndex(ports, INVALID_PORT_ID), -1);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePortWithModes_SupportedModesParamIgnored_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePortWithModes_SupportedModesParamIgnored_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST, SUPPORTED_MODES_DEVICE_ONLY);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].supportedModes, SUPPORTED_MODES_ALL);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePortWithModes_ModeParamIgnored_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePortWithModes_ModeParamIgnored_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, INVALID_ROLE, SUPPORTED_MODES_ALL);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePortWithModes_RepeatedUpdate_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePortWithModes_RepeatedUpdate_001");
    AddDefaultPorts();
    for (int32_t i = 0; i < 10; i++) {
        portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SINK,
            UsbSrvSupport::DATA_ROLE_DEVICE, UsbSrvSupport::PORT_MODE_DEVICE, SUPPORTED_MODES_ALL);
    }
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    int32_t index = FindPortIndex(ports, PORT_ID_1);
    ASSERT_NE(index, -1);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_DEVICE);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_DEVICE);
}

HWTEST_F(UsbPortManagerExtendedTest, UpdatePortWithModes_SwitchRoles_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : UpdatePortWithModes_SwitchRoles_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    for (int32_t i = 0; i < 5; i++) {
        portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
            UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST, SUPPORTED_MODES_ALL);
        portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SINK,
            UsbSrvSupport::DATA_ROLE_DEVICE, UsbSrvSupport::PORT_MODE_DEVICE, SUPPORTED_MODES_ALL);
    }
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].usbPortStatus.currentDataRole, UsbSrvSupport::DATA_ROLE_DEVICE);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_DEVICE);
}

HWTEST_F(UsbPortManagerExtendedTest, SetUsbd_Nullptr_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetUsbd_Nullptr_001");
    sptr<OHOS::HDI::Usb::V1_0::IUsbInterface> usbd = nullptr;
    int32_t ret = portManager_->SetUsbd(usbd);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, SetPortRole_NoHdi_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetPortRole_NoHdi_001");
    int32_t ret = portManager_->SetPortRole(PORT_ID_1,
        UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, SetPortRole_SourceHost_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetPortRole_SourceHost_001");
    int32_t ret = portManager_->SetPortRole(PORT_ID_1,
        UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, SetPortRole_SinkDevice_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetPortRole_SinkDevice_001");
    int32_t ret = portManager_->SetPortRole(PORT_ID_1,
        UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, SetPortRole_SourceDevice_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetPortRole_SourceDevice_001");
    int32_t ret = portManager_->SetPortRole(PORT_ID_1,
        UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_DEVICE);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, SetPortRole_SinkHost_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetPortRole_SinkHost_001");
    int32_t ret = portManager_->SetPortRole(PORT_ID_1,
        UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, SetPortRole_InvalidPowerRole_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetPortRole_InvalidPowerRole_001");
    int32_t ret = portManager_->SetPortRole(PORT_ID_1, INVALID_ROLE, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, SetPortRole_InvalidDataRole_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetPortRole_InvalidDataRole_001");
    int32_t ret = portManager_->SetPortRole(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE, INVALID_ROLE);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, SetPortRole_NegativePortId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetPortRole_NegativePortId_001");
    int32_t ret = portManager_->SetPortRole(NEGATIVE_PORT_ID,
        UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, SetPortRole_LargePortId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetPortRole_LargePortId_001");
    int32_t ret = portManager_->SetPortRole(INVALID_PORT_ID,
        UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, SetPortRole_ZeroRoles_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetPortRole_ZeroRoles_001");
    int32_t ret = portManager_->SetPortRole(PORT_ID_1,
        UsbSrvSupport::POWER_ROLE_NONE, UsbSrvSupport::DTA_ROLE_NONE);
    EXPECT_NE(ret, UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, SetPortRole_Stress_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : SetPortRole_Stress_001");
    for (int32_t i = 0; i < STRESS_LOOP_COUNT; i++) {
        int32_t ret = portManager_->SetPortRole(PORT_ID_1,
            UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
        EXPECT_NE(ret, UEC_OK);
    }
}

HWTEST_F(UsbPortManagerExtendedTest, GetDumpHelp_StdoutFd_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetDumpHelp_StdoutFd_001");
    portManager_->GetDumpHelp(STDOUT_FILENO);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, GetDumpHelp_NegativeFd_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : GetDumpHelp_NegativeFd_001");
    portManager_->GetDumpHelp(-1);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_EmptyArgs_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_EmptyArgs_001");
    std::vector<std::string> args;
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_OneArg_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_OneArg_001");
    std::vector<std::string> args = {"usb_port"};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_FourArgs_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_FourArgs_001");
    std::vector<std::string> args = {"usb_port", "-a", "x", "y"};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_AllPortsFlag_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_AllPortsFlag_001");
    AddDefaultPorts();
    std::vector<std::string> args = {"usb_port", "-a"};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_AllPortsFlagEmpty_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_AllPortsFlagEmpty_001");
    std::vector<std::string> args = {"usb_port", "-a"};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_AllPortsFlagWithExtraArg_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_AllPortsFlagWithExtraArg_001");
    AddDefaultPorts();
    std::vector<std::string> args = {"usb_port", "-a", "x"};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_QueryPortQ_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_QueryPortQ_001");
    AddDefaultPorts();
    std::vector<std::string> args = {"usb_port", "-p", "Q"};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_QueryPortOne_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_QueryPortOne_001");
    AddDefaultPorts();
    std::vector<std::string> args = {"usb_port", "-p", "1"};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_QueryPortTwo_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_QueryPortTwo_001");
    AddDefaultPorts();
    std::vector<std::string> args = {"usb_port", "-p", "2"};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_InvalidFlag_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_InvalidFlag_001");
    std::vector<std::string> args = {"usb_port", "-x"};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_PortFlagWithoutParam_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_PortFlagWithoutParam_001");
    std::vector<std::string> args = {"usb_port", "-p"};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_DevNullFd_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_DevNullFd_001");
    int32_t fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        fd = STDOUT_FILENO;
    }
    AddDefaultPorts();
    std::vector<std::string> args = {"usb_port", "-a"};
    portManager_->Dump(fd, args);
    portManager_->GetDumpHelp(fd);
    if (fd != STDOUT_FILENO) {
        close(fd);
    }
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_AfterAddPort_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_AfterAddPort_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port);
    std::vector<std::string> args = {"usb_port", "-a"};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_LongArg_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_LongArg_001");
    std::string longArg(256, '9');
    std::vector<std::string> args = {"usb_port", "-p", longArg};
    portManager_->Dump(STDOUT_FILENO, args);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Dump_Stress_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Dump_Stress_001");
    AddDefaultPorts();
    std::vector<std::string> helpArgs;
    std::vector<std::string> listArgs = {"usb_port", "-a"};
    for (int32_t i = 0; i < STRESS_LOOP_COUNT; i++) {
        portManager_->GetDumpHelp(STDOUT_FILENO);
        portManager_->Dump(STDOUT_FILENO, helpArgs);
        portManager_->Dump(STDOUT_FILENO, listArgs);
    }
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Flow_AddUpdateGetRemove_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Flow_AddUpdateGetRemove_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    ASSERT_EQ(ports.size(), SIZE_ONE);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
    portManager_->RemovePort(PORT_ID_1);
    std::vector<UsbPort> portsAfter;
    EXPECT_NE(portManager_->GetPorts(portsAfter), UEC_OK);
    EXPECT_EQ(portsAfter.size(), SIZE_ZERO);
}

HWTEST_F(UsbPortManagerExtendedTest, Flow_HostDeviceSwitch_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Flow_HostDeviceSwitch_001");
    AddDefaultPorts();
    for (int32_t i = 0; i < 10; i++) {
        portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
            UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
        std::vector<UsbPort> hostPorts;
        ASSERT_EQ(portManager_->GetPorts(hostPorts), UEC_OK);
        int32_t hostIndex = FindPortIndex(hostPorts, PORT_ID_1);
        ASSERT_NE(hostIndex, -1);
        EXPECT_EQ(hostPorts[static_cast<size_t>(hostIndex)].usbPortStatus.currentMode,
            UsbSrvSupport::PORT_MODE_HOST);
        portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SINK,
            UsbSrvSupport::DATA_ROLE_DEVICE, UsbSrvSupport::PORT_MODE_DEVICE);
        std::vector<UsbPort> devicePorts;
        ASSERT_EQ(portManager_->GetPorts(devicePorts), UEC_OK);
        int32_t deviceIndex = FindPortIndex(devicePorts, PORT_ID_1);
        ASSERT_NE(deviceIndex, -1);
        EXPECT_EQ(devicePorts[static_cast<size_t>(deviceIndex)].usbPortStatus.currentMode,
            UsbSrvSupport::PORT_MODE_DEVICE);
    }
}

HWTEST_F(UsbPortManagerExtendedTest, Flow_InitThenUpdateThenGet_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Flow_InitThenUpdateThenGet_001");
    AddDefaultPorts();
    portManager_->Init();
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    int32_t supportedModes = 0;
    ASSERT_EQ(portManager_->GetSupportedModes(PORT_ID_1, supportedModes), UEC_OK);
    EXPECT_EQ(supportedModes, SUPPORTED_MODES_ALL);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    int32_t index = FindPortIndex(ports, PORT_ID_1);
    ASSERT_NE(index, -1);
    EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
}

HWTEST_F(UsbPortManagerExtendedTest, Flow_RemoveAndReadd_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Flow_RemoveAndReadd_001");
    UsbPort port1 = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port1);
    portManager_->RemovePort(PORT_ID_1);
    UsbPort port2 = BuildPort(PORT_ID_1, SUPPORTED_MODES_HOST_ONLY,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    portManager_->AddPort(port2);
    portManager_->Init();
    int32_t supportedModes = 0;
    ASSERT_EQ(portManager_->GetSupportedModes(PORT_ID_1, supportedModes), UEC_OK);
    EXPECT_EQ(supportedModes, SUPPORTED_MODES_HOST_ONLY);
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports[0].usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
}

HWTEST_F(UsbPortManagerExtendedTest, Flow_AllPortsLifecycle_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Flow_AllPortsLifecycle_001");
    for (int32_t i = 1; i <= BULK_PORT_COUNT; i++) {
        UsbPort port = BuildPort(i, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        portManager_->AddPort(port);
    }
    for (int32_t i = 1; i <= BULK_PORT_COUNT; i++) {
        portManager_->UpdatePort(i, UsbSrvSupport::POWER_ROLE_SOURCE,
            UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    }
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(static_cast<int32_t>(ports.size()), BULK_PORT_COUNT);
    for (const auto &portItem : ports) {
        EXPECT_EQ(portItem.usbPortStatus.currentMode, UsbSrvSupport::PORT_MODE_HOST);
    }
    for (int32_t i = 1; i <= BULK_PORT_COUNT; i++) {
        portManager_->RemovePort(i);
    }
    std::vector<UsbPort> portsAfter;
    EXPECT_NE(portManager_->GetPorts(portsAfter), UEC_OK);
    EXPECT_EQ(portsAfter.size(), SIZE_ZERO);
}

HWTEST_F(UsbPortManagerExtendedTest, Flow_UpdateThenModesUnchanged_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Flow_UpdateThenModesUnchanged_001");
    UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_HOST_ONLY,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
    portManager_->AddPort(port);
    portManager_->Init();
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    int32_t supportedModes = 0;
    ASSERT_EQ(portManager_->GetSupportedModes(PORT_ID_1, supportedModes), UEC_OK);
    EXPECT_EQ(supportedModes, SUPPORTED_MODES_HOST_ONLY);
}

HWTEST_F(UsbPortManagerExtendedTest, Flow_MixedPortIds_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Flow_MixedPortIds_001");
    portManager_->AddPort(BuildPort(0, SUPPORTED_MODES_DEVICE_ONLY,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE));
    portManager_->AddPort(BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE));
    portManager_->AddPort(BuildPort(INT32_MAX, SUPPORTED_MODES_HOST_ONLY,
        UsbSrvSupport::PORT_MODE_HOST, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST));
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(ports.size(), SIZE_THREE);
    EXPECT_EQ(ports[0].id, 0);
    EXPECT_EQ(ports[1].id, PORT_ID_1);
    EXPECT_EQ(ports[2].id, INT32_MAX);
}

HWTEST_F(UsbPortManagerExtendedTest, Concurrent_AddPorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Concurrent_AddPorts_001");
    std::vector<std::thread> threads;
    for (int32_t t = 0; t < THREAD_COUNT; t++) {
        threads.emplace_back(&UsbPortManagerExtendedTest::ConcurrentAddPortsWorker, this, t);
    }
    for (auto &threadItem : threads) {
        threadItem.join();
    }
    std::vector<UsbPort> ports;
    ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
    EXPECT_EQ(static_cast<int32_t>(ports.size()), THREAD_COUNT * THREAD_ITERATION_COUNT);
}

HWTEST_F(UsbPortManagerExtendedTest, Concurrent_GetPorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Concurrent_GetPorts_001");
    AddDefaultPorts();
    std::atomic<int32_t> okCount {0};
    std::vector<std::thread> threads;
    for (int32_t t = 0; t < THREAD_COUNT; t++) {
        threads.emplace_back(&UsbPortManagerExtendedTest::ConcurrentGetPortsWorker, this, std::ref(okCount));
    }
    for (auto &threadItem : threads) {
        threadItem.join();
    }
    EXPECT_EQ(okCount.load(), THREAD_COUNT * THREAD_ITERATION_COUNT);
}

HWTEST_F(UsbPortManagerExtendedTest, Concurrent_AddAndRemove_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Concurrent_AddAndRemove_001");
    std::vector<std::thread> threads;
    for (int32_t t = 0; t < THREAD_COUNT; t++) {
        threads.emplace_back(&UsbPortManagerExtendedTest::ConcurrentAddAndRemoveWorker, this, t);
    }
    for (auto &threadItem : threads) {
        threadItem.join();
    }
    std::vector<UsbPort> ports;
    portManager_->GetPorts(ports);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, StressTest_AddRemoveCycle_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : StressTest_AddRemoveCycle_001");
    for (int32_t i = 0; i < STRESS_LOOP_COUNT; i++) {
        UsbPort port = BuildPort(PORT_ID_1, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        portManager_->AddPort(port);
        std::vector<UsbPort> ports;
        ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
        EXPECT_EQ(ports.size(), SIZE_ONE);
        portManager_->RemovePort(PORT_ID_1);
    }
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, StressTest_BulkPorts_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : StressTest_BulkPorts_001");
    for (int32_t round = 0; round < 5; round++) {
        for (int32_t i = 1; i <= BULK_PORT_COUNT; i++) {
            UsbPort port = BuildPort(i, SUPPORTED_MODES_ALL,
                UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK,
                UsbSrvSupport::DATA_ROLE_DEVICE);
            portManager_->AddPort(port);
        }
        std::vector<UsbPort> ports;
        ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
        EXPECT_EQ(static_cast<int32_t>(ports.size()), BULK_PORT_COUNT);
        for (int32_t i = 1; i <= BULK_PORT_COUNT; i++) {
            portManager_->RemovePort(i);
        }
    }
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Boundary_PortIds_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Boundary_PortIds_001");
    const std::vector<int32_t> boundaryIds = {0, -1, 1, INVALID_PORT_ID, INT32_MAX, INT32_MIN};
    for (int32_t id : boundaryIds) {
        UsbPort port = BuildPort(id, SUPPORTED_MODES_ALL,
            UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SINK, UsbSrvSupport::DATA_ROLE_DEVICE);
        portManager_->AddPort(port);
        std::vector<UsbPort> ports;
        ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
        EXPECT_NE(FindPortIndex(ports, id), -1);
        portManager_->RemovePort(id);
    }
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, Boundary_RoleValues_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : Boundary_RoleValues_001");
    AddDefaultPorts();
    const std::vector<int32_t> roleValues = {0, 1, 2, -1, INVALID_ROLE};
    for (int32_t role : roleValues) {
        portManager_->UpdatePort(PORT_ID_1, role, role, role);
        std::vector<UsbPort> ports;
        ASSERT_EQ(portManager_->GetPorts(ports), UEC_OK);
        int32_t index = FindPortIndex(ports, PORT_ID_1);
        ASSERT_NE(index, -1);
        EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentPowerRole, role);
        EXPECT_EQ(ports[static_cast<size_t>(index)].usbPortStatus.currentDataRole, role);
    }
}

HWTEST_F(UsbPortManagerExtendedTest, ErrorHandling_AllOpsOnEmptyManager_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : ErrorHandling_AllOpsOnEmptyManager_001");
    std::vector<UsbPort> ports;
    EXPECT_NE(portManager_->GetPorts(ports), UEC_OK);
    int32_t supportedModes = 0;
    EXPECT_NE(portManager_->GetSupportedModes(PORT_ID_1, supportedModes), UEC_OK);
    EXPECT_NE(portManager_->QueryPort(), UEC_OK);
    EXPECT_NE(portManager_->SetPortRole(PORT_ID_1,
        UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST), UEC_OK);
    portManager_->RemovePort(PORT_ID_1);
    portManager_->UpdatePort(PORT_ID_1, UsbSrvSupport::POWER_ROLE_SOURCE,
        UsbSrvSupport::DATA_ROLE_HOST, UsbSrvSupport::PORT_MODE_HOST);
    SUCCEED();
}

HWTEST_F(UsbPortManagerExtendedTest, ErrorHandling_InvalidRolesNoHdi_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : ErrorHandling_InvalidRolesNoHdi_001");
    EXPECT_NE(portManager_->SetPortRole(PORT_ID_1, INVALID_ROLE, INVALID_ROLE), UEC_OK);
    EXPECT_NE(portManager_->SetPortRole(PORT_ID_1, -1, -1), UEC_OK);
    EXPECT_NE(portManager_->SetPortRole(INVALID_PORT_ID, INVALID_ROLE, INVALID_ROLE), UEC_OK);
    EXPECT_NE(portManager_->SetPortRole(NEGATIVE_PORT_ID,
        UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST), UEC_OK);
}

HWTEST_F(UsbPortManagerExtendedTest, ErrorHandling_DumpInvalidArgs_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_PORT, "Case Start : ErrorHandling_DumpInvalidArgs_001");
    const std::vector<std::vector<std::string>> argSets = {
        {},
        {"usb_port"},
        {"usb_port", "-x"},
        {"usb_port", "-a", "extra"},
        {"usb_port", "-p"},
        {"usb_port", "-p", "abc"},
        {"usb_port", "-p", "-1"},
        {"usb_port", "-p", "99999999999999999999"},
        {"usb_port", "-p", "Q", "extra"},
    };
    for (const auto &args : argSets) {
        portManager_->Dump(STDOUT_FILENO, args);
    }
    portManager_->GetDumpHelp(-1);
    SUCCEED();
}

} // namespace ExtendedTest
} // namespace USB
} // namespace OHOS
