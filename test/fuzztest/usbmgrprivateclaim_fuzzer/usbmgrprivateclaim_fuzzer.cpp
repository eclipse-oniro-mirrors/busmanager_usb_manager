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

#include "usbmgrprivateclaim_fuzzer.h"

#include "accesstoken_kit.h"
#include "claim_exclusive_callback.h"
#include "nativetoken_kit.h"
#include "securec.h"
#include "token_setproc.h"
#include "usb_errors.h"
#include "usb_service.h"

namespace OHOS {
namespace USB {
    const size_t OFFSET_TOKEN_ID = 4;
    const size_t OFFSET_EXISTING_OWNER = 8;
    const uint8_t RELEASE_SCENARIO_EMPTY = 0;
    const uint8_t RELEASE_SCENARIO_OWNER_WITH_NORMAL = 1;
    const uint8_t RELEASE_SCENARIO_OWNER_ONLY = 2;
    const uint8_t RELEASE_SCENARIO_OTHER_WITH_NORMAL = 3;
    const uint8_t RELEASE_SCENARIO_OTHER_ONLY = 4;
    const uint8_t RELEASE_SCENARIO_NO_OWNER_WITH_NORMAL = 5;

    void PrepareClaimFuzzEnv(UsbService *service, uint8_t busNum, uint8_t devAddr)
    {
        std::string name = std::to_string(busNum) + "-" + std::to_string(devAddr);
        std::string uniqueName = std::to_string(busNum) + "+" + std::to_string(devAddr);
        service->deviceVidPidMap_[name] = uniqueName;
    }

    void ClearClaimState(UsbService *service, const std::string &key)
    {
        std::lock_guard<std::mutex> guard(service->claimMutex_);
        service->claimStateMap_.erase(key);
    }

    void RegisterCallbackForTest(UsbService *service, const std::string &key,
        uint8_t busNum, uint8_t devAddr, uint8_t interface, uint32_t tokenId)
    {
        sptr<ClaimExclusiveCallback> cb = new ClaimExclusiveCallback();
        service->RegisterExclusiveCallback(key, busNum, devAddr, interface, tokenId, cb, false);
    }

    void RegisterExclusiveCallbackFuzzTest(UsbService *service, uint8_t busNum, uint8_t devAddr,
        uint8_t interface, uint32_t tokenId, const std::string &key, uint8_t flags)
    {
        bool isRepeat = (flags & 0x01) != 0;

        ClearClaimState(service, key);

        service->RegisterExclusiveCallback(key, busNum, devAddr, interface, tokenId, nullptr, isRepeat);

        sptr<ClaimExclusiveCallback> cb = new ClaimExclusiveCallback();
        service->RegisterExclusiveCallback(key, busNum, devAddr, interface, tokenId, cb, isRepeat);
    }

    void NotifyExclusiveOwnerFuzzTest(UsbService *service, uint8_t busNum, uint8_t devAddr,
        uint8_t interface, uint32_t tokenId, const std::string &key, uint8_t flags, uint32_t existingOwner)
    {
        bool prePopulate = (flags & 0x01) != 0;
        bool setCallback = (flags & 0x02) != 0;
        bool setNormalClaimed = (flags & 0x04) != 0;

        ClearClaimState(service, key);

        if (prePopulate) {
            std::lock_guard<std::mutex> guard(service->claimMutex_);
            auto &state = service->claimStateMap_[key];
            state.exclusiveOwner = existingOwner;
            state.normalClaimed = setNormalClaimed;
            if (setCallback) {
                state.notifyCallback = new ClaimExclusiveCallback();
            }
        }

        service->NotifyExclusiveOwner(busNum, devAddr, interface, tokenId);
    }

    void CheckExclusiveTransferFuzzTest(UsbService *service, uint8_t busNum, uint8_t devAddr,
        uint8_t interface, const std::string &key, uint8_t flags, uint32_t existingOwner)
    {
        bool prePopulate = (flags & 0x01) != 0;
        bool setNormalClaimed = (flags & 0x02) != 0;

        ClearClaimState(service, key);

        if (prePopulate) {
            std::lock_guard<std::mutex> guard(service->claimMutex_);
            auto &state = service->claimStateMap_[key];
            state.exclusiveOwner = existingOwner;
            state.normalClaimed = setNormalClaimed;
        }

        service->CheckExclusiveTransfer(busNum, devAddr, interface);
    }

    void RemoveExclusiveClaimByDeviceFuzzTest(UsbService *service, uint8_t busNum, uint8_t devAddr,
        uint32_t tokenId, const std::string &key, uint8_t flags)
    {
        bool setExclusive = (flags & 0x01) != 0;
        bool setNormalClaimed = (flags & 0x02) != 0;

        ClearClaimState(service, key);

        if (setExclusive) {
            RegisterCallbackForTest(service, key, busNum, devAddr, 0, tokenId);
            std::lock_guard<std::mutex> guard(service->claimMutex_);
            auto &state = service->claimStateMap_[key];
            state.exclusiveOwner = tokenId;
            state.normalClaimed = setNormalClaimed;
        }

        service->RemoveExclusiveClaimByDevice(busNum, devAddr, tokenId);
    }

    void RemoveAllClaimByDeviceFuzzTest(UsbService *service, uint8_t busNum, uint8_t devAddr,
        const std::string &key, uint8_t flags)
    {
        bool setExclusive = (flags & 0x01) != 0;

        ClearClaimState(service, key);

        if (setExclusive) {
            RegisterCallbackForTest(service, key, busNum, devAddr, 0, 0xDEAD);
            std::lock_guard<std::mutex> guard(service->claimMutex_);
            auto &state = service->claimStateMap_[key];
            state.exclusiveOwner = 0xDEAD;
        }

        service->RemoveAllClaimByDevice(busNum, devAddr);
    }

    void ClaimInterfaceExclusiveFuzzTest(UsbService *service, uint8_t busNum, uint8_t devAddr,
        uint8_t interface, uint8_t force, const std::string &key, uint8_t flags, uint32_t existingOwner)
    {
        bool prePopulate = (flags & 0x01) != 0;
        bool setNormalClaimed = (flags & 0x02) != 0;
        bool testNullHostManager = (flags & 0x08) != 0;

        ClearClaimState(service, key);

        if (prePopulate) {
            std::lock_guard<std::mutex> guard(service->claimMutex_);
            auto &state = service->claimStateMap_[key];
            state.exclusiveOwner = existingOwner;
            state.normalClaimed = setNormalClaimed;
        }

        sptr<ClaimExclusiveCallback> cb = new ClaimExclusiveCallback();

        if (testNullHostManager) {
            auto saved = service->usbHostManager_;
            service->usbHostManager_ = nullptr;
            service->ClaimInterfaceExclusive(busNum, devAddr, interface, force, cb);
            service->usbHostManager_ = saved;
        } else {
            service->ClaimInterfaceExclusive(busNum, devAddr, interface, force, cb);
        }
    }

    void PrepareReleaseScenario(UsbService *service, const std::string &key,
        uint8_t busNum, uint8_t devAddr, uint8_t interface, uint32_t tokenId,
        uint8_t scenario, uint32_t otherOwner)
    {
        if (scenario == RELEASE_SCENARIO_EMPTY) {
            return;
        }
        bool callerIsOwner = (scenario == RELEASE_SCENARIO_OWNER_WITH_NORMAL ||
            scenario == RELEASE_SCENARIO_OWNER_ONLY);
        bool otherIsOwner = (scenario == RELEASE_SCENARIO_OTHER_WITH_NORMAL ||
            scenario == RELEASE_SCENARIO_OTHER_ONLY);
        if (callerIsOwner) {
            RegisterCallbackForTest(service, key, busNum, devAddr, interface, tokenId);
        }
        std::lock_guard<std::mutex> guard(service->claimMutex_);
        auto &state = service->claimStateMap_[key];
        if (callerIsOwner) {
            state.exclusiveOwner = tokenId;
            state.normalClaimed = (scenario == RELEASE_SCENARIO_OWNER_WITH_NORMAL);
        } else if (otherIsOwner) {
            state.exclusiveOwner = otherOwner;
            state.normalClaimed = (scenario == RELEASE_SCENARIO_OTHER_WITH_NORMAL);
        } else if (scenario == RELEASE_SCENARIO_NO_OWNER_WITH_NORMAL) {
            state.normalClaimed = true;
        }
    }

    void ReleaseClaimStateFuzzTest(UsbService *service, uint8_t busNum, uint8_t devAddr,
        uint8_t interface, uint32_t tokenId, const std::string &key, uint8_t scenario,
        uint32_t existingOwner)
    {
        ClearClaimState(service, key);

        uint32_t otherOwner = (existingOwner == 0 || existingOwner == tokenId)
            ? (tokenId == 0xFFFFFFFF ? 1 : tokenId + 1)
            : existingOwner;
        bool releaseToHdi = false;

        PrepareReleaseScenario(service, key, busNum, devAddr, interface, tokenId, scenario, otherOwner);

        service->ReleaseClaimState(busNum, devAddr, interface, tokenId, releaseToHdi);
    }

    bool UsbMgrPrivateClaimFuzzTest(const uint8_t *rawData, size_t size)
    {
        constexpr size_t minSize = 13;
        if (rawData == nullptr || size < minSize) {
            return false;
        }

        auto service = UsbService::GetGlobalInstance();
        if (service == nullptr) {
            return false;
        }

        uint8_t busNum = rawData[0];
        uint8_t devAddr = rawData[1];
        uint8_t interface = rawData[2];
        uint8_t flags = rawData[3];
        uint32_t tokenId = 0;
        uint32_t existingOwner = 0;
        errno_t ret = memcpy_s(&tokenId, sizeof(tokenId), rawData + OFFSET_TOKEN_ID, sizeof(uint32_t));
        if (ret != EOK) {
            return false;
        }
        ret = memcpy_s(&existingOwner, sizeof(existingOwner), rawData + OFFSET_EXISTING_OWNER, sizeof(uint32_t));
        if (ret != EOK) {
            return false;
        }
        uint8_t releaseScenario = rawData[12] & 0x07;

        PrepareClaimFuzzEnv(service, busNum, devAddr);
        std::string key = service->MakeClaimKey(busNum, devAddr, interface);

        RegisterExclusiveCallbackFuzzTest(service, busNum, devAddr, interface, tokenId, key, flags);

        NotifyExclusiveOwnerFuzzTest(service, busNum, devAddr, interface, tokenId, key, flags, existingOwner);

        CheckExclusiveTransferFuzzTest(service, busNum, devAddr, interface, key, flags, existingOwner);

        RemoveExclusiveClaimByDeviceFuzzTest(service, busNum, devAddr, tokenId, key, flags);

        RemoveAllClaimByDeviceFuzzTest(service, busNum, devAddr, key, flags);

        uint8_t force = (flags & 0x04) != 0 ? 1 : 0;
        ClaimInterfaceExclusiveFuzzTest(service, busNum, devAddr, interface, force, key, flags, existingOwner);

        ReleaseClaimStateFuzzTest(service, busNum, devAddr, interface, tokenId, key, releaseScenario, existingOwner);

        ClearClaimState(service, key);
        std::string devName = std::to_string(busNum) + "-" + std::to_string(devAddr);
        service->deviceVidPidMap_.erase(devName);

        return true;
    }
} // USB
} // OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::USB::UsbMgrPrivateClaimFuzzTest(data, size);
    return 0;
}

static const uint64_t SYSTEM_APP_MASK = (static_cast<uint64_t>(1) << 32);

extern "C" int LLVMFuzzerInitialize(int *argc, char **argv)
{
    constexpr int permissionNum = 1;
    const char *perms[permissionNum] = {"ohos.permission.MANAGE_USB_CONFIG"};
    NativeTokenInfoParams info = {
        .dcapsNum = 0,
        .permsNum = permissionNum,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "usb_private_claim_fuzztest",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&info);
    SetSelfTokenID(tokenId | SYSTEM_APP_MASK);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    return 0;
}
