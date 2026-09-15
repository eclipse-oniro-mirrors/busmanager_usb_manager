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

#ifndef USB_PORT_MANAGER_EXTENDED_TEST_H
#define USB_PORT_MANAGER_EXTENDED_TEST_H

#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "usb_errors.h"
#include "usb_port.h"
#include "usb_port_manager.h"

namespace OHOS {
namespace USB {
namespace ExtendedTest {

class UsbPortManagerExtendedTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    UsbPort BuildPort(int32_t portId, int32_t supportedModes, int32_t currentMode,
        int32_t powerRole, int32_t dataRole);
    void AddDefaultPorts();
    int32_t FindPortIndex(const std::vector<UsbPort> &ports, int32_t portId);
    void ConcurrentAddPortsWorker(int32_t threadIndex);
    void ConcurrentGetPortsWorker(std::atomic<int32_t> &okCount);
    void ConcurrentAddAndRemoveWorker(int32_t threadIndex);

    std::unique_ptr<UsbPortManager> portManager_;
    std::mutex extendedMutex_;
};

} // namespace ExtendedTest
} // namespace USB
} // namespace OHOS

#endif // USB_PORT_MANAGER_EXTENDED_TEST_H
