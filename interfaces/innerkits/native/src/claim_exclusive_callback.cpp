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

#include "claim_exclusive_callback.h"

#include "usb_common.h"

namespace OHOS::USB {
std::string ClaimExclusiveCallback::MakeKey(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId) const
{
    return std::to_string(busNum) + "-" + std::to_string(devAddr) + "-" + std::to_string(interfaceId);
}

void ClaimExclusiveCallback::AddListener(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId,
    std::function<void(uint8_t, uint8_t, uint8_t)> listener)
{
    if (listener == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> guard(mutex_);
    listeners_[MakeKey(busNum, devAddr, interfaceId)] = std::move(listener);
}

void ClaimExclusiveCallback::RemoveListener(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId)
{
    std::lock_guard<std::mutex> guard(mutex_);
    listeners_.erase(MakeKey(busNum, devAddr, interfaceId));
}

void ClaimExclusiveCallback::RemoveListenersByDevice(uint8_t busNum, uint8_t devAddr)
{
    std::string prefix = std::to_string(busNum) + "-" + std::to_string(devAddr) + "-";
    std::lock_guard<std::mutex> guard(mutex_);
    for (auto it = listeners_.begin(); it != listeners_.end();) {
        if (it->first.compare(0, prefix.size(), prefix) == 0) {
            it = listeners_.erase(it);
        } else {
            ++it;
        }
    }
}

ErrCode ClaimExclusiveCallback::onConflict(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId)
{
    std::function<void(uint8_t, uint8_t, uint8_t)> listener = nullptr;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        auto it = listeners_.find(MakeKey(busNum, devAddr, interfaceId));
        if (it != listeners_.end()) {
            listener = it->second;
        }
    }
    if (listener != nullptr) {
        listener(busNum, devAddr, interfaceId);
    } else {
        USB_HILOGW(MODULE_USB_INNERKIT,
            "ClaimExclusiveCallback onConflict no listener bus=%{public}hhu dev=%{public}hhu if=%{public}hhu",
            busNum, devAddr, interfaceId);
    }
    return 0;
}
} // namespace OHOS::USB