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

#include "usb_claim_test_common.h"

#include <dirent.h>
#include <fstream>
#include <unistd.h>

#include "hilog_wrapper.h"
#include "usb_errors.h"

namespace OHOS {
namespace USB {
namespace {
constexpr int32_t DEVICE_WAIT_INTERVAL_MS = 100;
constexpr int32_t MS_TO_US = 1000;
constexpr int32_t HUB_DEVICE_CLASS = 9;
constexpr int32_t HOTPLUG_WAIT_TIMEOUT_MS = 10000;
const char *APP_A_PROCESS_NAME = "usb_claim_test_app_a";
const char *APP_B_PROCESS_NAME = "usb_claim_test_app_b";
const char *APP_C_PROCESS_NAME = "usb_claim_test_app_c";
const char *APP_SYS_PROCESS_NAME = "usb_claim_test_sys";
const char *USB_SYSFS_ROOT = "/sys/bus/usb/devices/";

uint64_t AllocNativeToken(const char *processName)
{
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 0,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = nullptr,
        .acls = nullptr,
        .processName = processName,
        .aplStr = "normal",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    if (tokenId == 0) {
        USB_HILOGE(MODULE_USB_SERVICE, "AllocNativeToken failed for %{public}s", processName);
    }
    return tokenId;
}

/*
 * Token used for management APIs (ManageGlobalInterface) which require
 * ohos.permission.MANAGE_USB_CONFIG in addition to a root native caller.
 */
uint64_t AllocManageNativeToken()
{
    const char *permsInfo[] = {"ohos.permission.MANAGE_USB_CONFIG"};
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = permsInfo,
        .acls = nullptr,
        .processName = APP_SYS_PROCESS_NAME,
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    if (tokenId == 0) {
        USB_HILOGE(MODULE_USB_SERVICE, "AllocManageNativeToken failed");
    }
    return tokenId;
}

bool ReadSysfsNumber(const std::string &path, int32_t &value)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    file >> value;
    return file.good() || file.eof();
}

/*
 * The HDI device name is "bus-devAddr" while the sysfs directory name is
 * port based ("bus-port"), so the sysfs directory has to be located by
 * comparing busnum/devnum attributes.
 */
bool FindSysfsDeviceName(uint8_t busNum, uint8_t devAddr, std::string &sysfsName)
{
    std::string root = USB_SYSFS_ROOT;
    DIR *dir = opendir(root.c_str());
    if (dir == nullptr) {
        return false;
    }
    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == ".." || name.rfind("usb", 0) == 0) {
            continue;
        }
        int32_t busNumFile = -1;
        int32_t devNumFile = -1;
        if (!ReadSysfsNumber(root + name + "/busnum", busNumFile) ||
            !ReadSysfsNumber(root + name + "/devnum", devNumFile)) {
            continue;
        }
        if (busNumFile == static_cast<int32_t>(busNum) && devNumFile == static_cast<int32_t>(devAddr)) {
            sysfsName = name;
            closedir(dir);
            return true;
        }
    }
    closedir(dir);
    return false;
}
} // namespace

uint64_t UsbClaimTestBase::tokenA_ = 0;
uint64_t UsbClaimTestBase::tokenB_ = 0;
uint64_t UsbClaimTestBase::tokenC_ = 0;
uint64_t UsbClaimTestBase::tokenSys_ = 0;
uint8_t UsbClaimTestBase::busNum_ = 0;
uint8_t UsbClaimTestBase::devAddr_ = 0;
uint8_t UsbClaimTestBase::ifaceId_ = 0;
std::string UsbClaimTestBase::devName_ = "";
UsbDevice UsbClaimTestBase::device_ {};
USBDevicePipe UsbClaimTestBase::pipe_ {};
UsbInterface UsbClaimTestBase::iface_ {};
USBEndpoint UsbClaimTestBase::endpoint_ {};
bool UsbClaimTestBase::deviceReady_ = false;

void UsbClaimTestBase::SetUpTestCase()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbClaimTestBase SetUpTestCase");
    tokenA_ = AllocNativeToken(APP_A_PROCESS_NAME);
    tokenB_ = AllocNativeToken(APP_B_PROCESS_NAME);
    tokenC_ = AllocNativeToken(APP_C_PROCESS_NAME);
    tokenSys_ = AllocManageNativeToken();
    ASSERT_NE(0U, tokenA_);
    ASSERT_NE(0U, tokenB_);
    ASSERT_NE(0U, tokenC_);
    ASSERT_NE(0U, tokenSys_);
    SwitchToApp(CLAIM_APP_A);

    deviceReady_ = PickTestDevice();
    ASSERT_TRUE(deviceReady_);
    USB_HILOGI(MODULE_USB_SERVICE, "test device bus=%{public}hhu dev=%{public}hhu name=%{public}s if=%{public}hhu",
        busNum_, devAddr_, devName_.c_str(), ifaceId_);

    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.OpenDevice(device_, pipe_);
    ASSERT_EQ(UEC_OK, ret);
}

void UsbClaimTestBase::TearDownTestCase()
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbClaimTestBase TearDownTestCase");
    if (!deviceReady_) {
        return;
    }
    CleanClaimState();
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    client.UsbAttachKernelDriver(pipe_, iface_);
    client.Close(pipe_);
}

void UsbClaimTestBase::SetUp()
{
    CleanClaimState();
}

void UsbClaimTestBase::TearDown()
{
    CleanClaimState();
}

void UsbClaimTestBase::SwitchToApp(ClaimTestApp app)
{
    uint64_t token = tokenA_;
    if (app == CLAIM_APP_B) {
        token = tokenB_;
    } else if (app == CLAIM_APP_C) {
        token = tokenC_;
    } else if (app == CLAIM_APP_SYS) {
        token = tokenSys_;
    }
    SetSelfTokenID(token);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

/*
 * Bring the claim state of the test interface back to a clean baseline:
 * no exclusive claim, no normal claim flag, interface not claimed at HDI
 * level and the kernel driver re-attached. ReleaseInterface is invoked
 * under every identity because the normal claim flag does not record its
 * owner and only the exclusive owner can clear the exclusive record.
 */
void UsbClaimTestBase::CleanClaimState()
{
    if (!deviceReady_) {
        return;
    }
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    // OpenDevice is reference counted at the HDI layer, so this also recovers
    // the handle after a previous test case closed the device.
    client.OpenDevice(device_, pipe_);
    client.ReleaseInterface(pipe_, iface_);
    SwitchToApp(CLAIM_APP_B);
    client.ReleaseInterface(pipe_, iface_);
    SwitchToApp(CLAIM_APP_C);
    client.ReleaseInterface(pipe_, iface_);
    SwitchToApp(CLAIM_APP_A);
    client.UsbAttachKernelDriver(pipe_, iface_);
}

bool UsbClaimTestBase::PickTestDevice()
{
    std::vector<UsbDevice> deviceList;
    auto &client = UsbSrvClient::GetInstance();
    if (client.GetDevices(deviceList) != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "GetDevices failed");
        return false;
    }
    for (auto &dev : deviceList) {
        if (dev.GetClass() == HUB_DEVICE_CLASS) {
            continue;
        }
        if (dev.GetConfigs().empty()) {
            continue;
        }
        USBConfig config = dev.GetConfigs().at(0);
        if (config.GetInterfaces().empty()) {
            continue;
        }
        UsbInterface interface = config.GetInterfaces().at(0);
        if (interface.GetEndpoints().empty()) {
            continue;
        }
        device_ = dev;
        busNum_ = dev.GetBusNum();
        devAddr_ = dev.GetDevAddr();
        devName_ = dev.GetName();
        ifaceId_ = static_cast<uint8_t>(interface.GetId());
        iface_ = interface;
        endpoint_ = interface.GetEndpoints().at(0);
        pipe_.SetBusNum(busNum_);
        pipe_.SetDevAddr(devAddr_);
        return true;
    }
    USB_HILOGE(MODULE_USB_SERVICE, "no usable non-hub device found");
    return false;
}

bool UsbClaimTestBase::RefreshTestDevice()
{
    if (!PickTestDevice()) {
        return false;
    }
    deviceReady_ = true;
    USB_HILOGI(MODULE_USB_SERVICE, "refresh device bus=%{public}hhu dev=%{public}hhu", busNum_, devAddr_);
    return true;
}

bool UsbClaimTestBase::WaitForDevicePresent(bool present, int32_t timeoutMs)
{
    auto &client = UsbSrvClient::GetInstance();
    int32_t waitedMs = 0;
    while (true) {
        std::vector<UsbDevice> deviceList;
        bool found = false;
        if (client.GetDevices(deviceList) == UEC_OK) {
            for (auto &dev : deviceList) {
                if (dev.GetClass() != HUB_DEVICE_CLASS) {
                    found = true;
                    break;
                }
            }
        }
        if (found == present) {
            return true;
        }
        if (waitedMs >= timeoutMs) {
            return false;
        }
        usleep(DEVICE_WAIT_INTERVAL_MS * MS_TO_US);
        waitedMs += DEVICE_WAIT_INTERVAL_MS;
    }
}

bool UsbClaimTestBase::IsKernelDriverBound()
{
    std::string sysfsName;
    if (!FindSysfsDeviceName(busNum_, devAddr_, sysfsName)) {
        USB_HILOGE(MODULE_USB_SERVICE, "sysfs node not found bus=%{public}hhu dev=%{public}hhu", busNum_, devAddr_);
        return false;
    }
    std::string path = std::string(USB_SYSFS_ROOT) + sysfsName + "/" + sysfsName + ":1." + std::to_string(ifaceId_)
        + "/driver";
    return access(path.c_str(), F_OK) == 0;
}

/*
 * Hotplug is simulated through ManageGlobalInterface. Disabling deauthorizes
 * the root hubs at the HDI level, which disconnects every downstream device
 * and generates real device-leave events, so the service side DelDevice
 * cleanup (claim record removal) runs. Enabling re-authorizes the hubs and
 * the devices come back re-enumerated (possibly with a new device address),
 * so the test device must be refreshed after the plug. The API requires a
 * root native caller holding ohos.permission.MANAGE_USB_CONFIG, which the
 * dedicated system token of the test process satisfies.
 */
bool UsbClaimTestBase::SimulateUnplug()
{
    SwitchToApp(CLAIM_APP_SYS);
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.ManageGlobalInterface(true);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "ManageGlobalInterface disable failed ret=%{public}d", ret);
        return false;
    }
    return WaitForDevicePresent(false, HOTPLUG_WAIT_TIMEOUT_MS);
}

bool UsbClaimTestBase::SimulatePlug()
{
    SwitchToApp(CLAIM_APP_SYS);
    auto &client = UsbSrvClient::GetInstance();
    int32_t ret = client.ManageGlobalInterface(false);
    if (ret != UEC_OK) {
        USB_HILOGE(MODULE_USB_SERVICE, "ManageGlobalInterface enable failed ret=%{public}d", ret);
        return false;
    }
    if (!WaitForDevicePresent(true, HOTPLUG_WAIT_TIMEOUT_MS)) {
        return false;
    }
    return RefreshTestDevice();
}
} // namespace USB
} // namespace OHOS
