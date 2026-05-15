
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

#include "usbmgr_parseconfigdescriptors_fuzzer.h"
#include "usb_service.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "usb_descriptor_parser.h"
#include "usb_config.h"
#include "usb_errors.h"

namespace OHOS {
constexpr size_t THRESHOLD = 10;
constexpr uint32_t CURSOR_INIT = 18;
const std::u16string USB_INTERFACE_TOKEN = u"OHOS.USB.IUsbServer";

namespace USB {
bool UsbMgrParseConfigDescriptorsFuzzTest(const uint8_t* rawData, size_t size)
{
    if (rawData == nullptr || size < CURSOR_INIT + sizeof(UsbdDescriptorHeader)) {
        return false;
    }

    std::vector<uint8_t> descriptor;
    *(rawData + CURSOR_INIT + 1) %= 6;
    descriptor = std::vector<uint8_t>(rawData, rawData + size);
    std::vector<USBConfig> configs;
    if (offset < descriptor.size()) {
        UsbDescriptorParser::ParseConfigDescriptors(descriptor, CURSOR_INIT, configs);
    }

    return true;
}
} // namespace USB
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < OHOS::THRESHOLD) {
        return 0;
    }
    
    /* Run your code on data */
    OHOS::USB::UsbMgrParseConfigDescriptorsFuzzTest(data, size);
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
        .processName = "usb_descriptor_parser_fuzztest",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&info);
    SetSelfTokenID(tokenId | SYSTEM_APP_MASK);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    return 0;
}