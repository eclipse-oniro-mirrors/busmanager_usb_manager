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

#ifndef CLAIM_EXCLUSIVE_CALLBACK_H
#define CLAIM_EXCLUSIVE_CALLBACK_H

#include <functional>
#include <map>
#include <mutex>
#include <string>

#include "claim_exclusive_callback_stub.h"

namespace OHOS::USB {
class ClaimExclusiveCallback : public ClaimExclusiveCallbackStub {
public:
    ClaimExclusiveCallback() = default;
    ~ClaimExclusiveCallback() override = default;

    void AddListener(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId,
        std::function<void(uint8_t, uint8_t, uint8_t)> listener);
    void RemoveListener(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId);
    void RemoveListenersByDevice(uint8_t busNum, uint8_t devAddr);

    ErrCode onConflict(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId) override;

private:
    std::string MakeKey(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId) const;

    std::mutex mutex_;
    std::map<std::string, std::function<void(uint8_t, uint8_t, uint8_t)>> listeners_;
};
} // namespace OHOS::USB
#endif // CLAIM_EXCLUSIVE_CALLBACK_H