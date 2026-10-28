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

#ifndef USB_DESCRIPTOR_PARSER_TDD_TEST_H
#define USB_DESCRIPTOR_PARSER_TDD_TEST_H

#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <cstdint>

#include "usb_descriptor_parser.h"
#include "usb_device.h"
#include "usb_errors.h"
#include "usbd_type.h"

namespace OHOS {
namespace USB {
namespace ServiceTest {

class UsbDescriptorParserTddTest : public testing::Test {
protected:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

} // namespace ServiceTest
} // namespace USB
} // namespace OHOS

#endif // USB_DESCRIPTOR_PARSER_TDD_TEST_H
