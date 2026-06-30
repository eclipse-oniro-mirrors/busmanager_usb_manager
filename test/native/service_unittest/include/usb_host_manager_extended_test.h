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

#ifndef USB_HOST_MANAGER_EXTENDED_TEST_H
#define USB_HOST_MANAGER_EXTENDED_TEST_H

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "usb_host_manager.h"
#include "usb_device.h"
#include "usb_errors.h"

namespace OHOS {
namespace USB {
namespace ExtendedTest {

class UsbHostManagerExtendedTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    UsbDevice* CreateExtendedDevice(uint8_t busNum, uint8_t devAddr,
                                    uint16_t vendorId, uint16_t productId,
                                    uint8_t deviceClass, uint8_t subClass,
                                    uint8_t protocol);
    void CleanupDevices();

    std::unique_ptr<UsbHostManager> hostManager_;
    std::vector<UsbDevice*> devices_;
    std::mutex extendedMutex_;
};

} // namespace ExtendedTest
} // namespace USB
} // namespace OHOS

#endif // USB_HOST_MANAGER_EXTENDED_TEST_H
