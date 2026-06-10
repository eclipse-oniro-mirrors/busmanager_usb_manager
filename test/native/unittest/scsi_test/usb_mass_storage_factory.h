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

#ifndef USB_MASS_STORAGE_FACTORY_H
#define USB_MASS_STORAGE_FACTORY_H

#include <memory>
#include "usb_mass_storage_interface.h"

namespace OHOS {
namespace USB {

class UsbMassStorageFactory {
public:
    static std::unique_ptr<IUsbMassStorageInterface> CreateInstance();
};

} // namespace USB
} // namespace OHOS

#endif // USB_MASS_STORAGE_FACTORY_H