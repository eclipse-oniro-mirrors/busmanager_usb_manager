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

#ifndef USB_DEVICE_MANAGER_TEST_H
#define USB_DEVICE_MANAGER_TEST_H

#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "usb_device_manager.h"
#include "usb_errors.h"
#include "usb_srv_support.h"

namespace OHOS {
namespace USB {

class UsbDeviceManagerTest : public testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
    std::shared_ptr<UsbDeviceManager> deviceManager_;
};

class UsbDeviceManagerFunctionTest : public testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
    std::shared_ptr<UsbDeviceManager> deviceManager_;
};

class UsbDeviceManagerConvertTest : public testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
};

class UsbDeviceManagerEventTest : public testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
    std::shared_ptr<UsbDeviceManager> deviceManager_;
};

class UsbDeviceManagerDumpTest : public testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
    std::shared_ptr<UsbDeviceManager> deviceManager_;
};

} // namespace USB
} // namespace OHOS

#endif // USB_DEVICE_MANAGER_TEST_H
