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

#include <sys/types.h>
#include <unistd.h>

#include <map>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "hilog_wrapper.h"
#include "ipc_skeleton.h"
#include "nativetoken_kit.h"
#include "os_account_manager.h"
#include "serial_manager.h"
#include "token_setproc.h"
#include "usb_connection_callback_stub.h"
#include "usb_device_connection_type.h"
#include "usb_errors.h"
#include "usb_right_manager.h"
#include "usb_service.h"

using namespace testing::ext;
using namespace OHOS::USB;

namespace OHOS {
namespace USB {
namespace {
constexpr int32_t TEST_HAP_USER_ID = 100;
constexpr int32_t TEST_HAP_API_VERSION = 12;
constexpr int32_t EXPECTED_RIGHT_DEFAULT_USERID = 100;
const char *TEST_HAP_BUNDLE_NAME = "com.example.usbappuidtest";
const char *TEST_NATIVE_PROCESS_NAME = "usb_calling_appuid_test";
constexpr uint8_t TEST_BUS_NUM = 1;
constexpr uint8_t TEST_DEV_ADDR = 2;

void SetTestNativeToken()
{
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 0,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = nullptr,
        .acls = nullptr,
        .processName = TEST_NATIVE_PROCESS_NAME,
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

void SetTestHapToken()
{
    Security::AccessToken::HapInfoParams hapInfo = {
        .userID = TEST_HAP_USER_ID,
        .bundleName = TEST_HAP_BUNDLE_NAME,
        .instIndex = 0,
        .appIDDesc = "usb calling appuid test",
        .apiVersion = TEST_HAP_API_VERSION,
        .isSystemApp = false,
    };
    Security::AccessToken::HapPolicyParams hapPolicy = {
        .apl = Security::AccessToken::APL_NORMAL,
        .domain = "test.domain.usb",
    };
    Security::AccessToken::AccessTokenIDEx tokenIdEx =
        Security::AccessToken::AccessTokenKit::AllocHapToken(hapInfo, hapPolicy);
    SetSelfTokenID(tokenIdEx.tokenIDEx);
}

class RecordingConnectionListener : public UsbConnectionCallbackStub {
public:
    RecordingConnectionListener() = default;
    ~RecordingConnectionListener() override = default;

    int32_t OnDeviceConnected(const UsbDevice &device, int32_t uid, const std::string &bundleName) override
    {
        gotConnected = true;
        lastUid = uid;
        lastBundleName = bundleName;
        return UEC_OK;
    }

    int32_t OnDeviceDisconnected(const UsbDevice &device, int32_t uid, const std::string &bundleName) override
    {
        gotDisconnected = true;
        lastUid = uid;
        lastBundleName = bundleName;
        return UEC_OK;
    }

    bool gotConnected = false;
    bool gotDisconnected = false;
    int32_t lastUid = -1;
    std::string lastBundleName;
};
} // namespace

class UsbCallingAppUidTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() override
    {
        SetTestNativeToken();
    }
    void TearDown() override {}

    // 进程内共享的 UsbService 实例：仅构造、不析构。usb_service.cpp 的全局
    // g_serviceInstance 已在静态初始化时向 LocalAbilityManager 注册同 ID 能力，
    // 局部实例析构会走 SystemAbility 卸载路径导致进程崩溃，故此处刻意不释放。
    static UsbService *SharedService()
    {
        static UsbService *service = new UsbService();
        return service;
    }
};

/**
 * @tc.name: GetCallingAppUid_EquivalenceWithGetCallingUid_001
 * @tc.desc: 迁移不变式：当前 IPC 版本 GetCallingAppUid 与 GetCallingUid 返回一致，
 *           且进程内调用等于本进程 uid。IPC 层后续分化语义时该用例报警提示复核迁移点。
 * @tc.type: FUNC
 */
HWTEST_F(UsbCallingAppUidTest, GetCallingAppUid_EquivalenceWithGetCallingUid_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "GetCallingAppUid_EquivalenceWithGetCallingUid_001 start");
    pid_t appUid = IPCSkeleton::GetCallingAppUid();
    pid_t rawUid = IPCSkeleton::GetCallingUid();
    EXPECT_EQ(appUid, rawUid);
    EXPECT_EQ(appUid, static_cast<pid_t>(getuid()));
    USB_HILOGI(MODULE_USB_SERVICE, "appUid=%{public}d rawUid=%{public}d", appUid, rawUid);
}

/**
 * @tc.name: UsbRightManager_GetCurrentUserId_001
 * @tc.desc: GetCurrentUserId 以 GetCallingAppUid 经账号服务推导 userId，
 *           结果与直接以 GetCallingAppUid 调用 GetOsAccountLocalIdFromUid 的结果一致，
 *           账号服务失败时回落 USB_RIGHT_USERID_DEFAULT(100)。
 * @tc.type: FUNC
 */
HWTEST_F(UsbCallingAppUidTest, UsbRightManager_GetCurrentUserId_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRightManager_GetCurrentUserId_001 start");
    UsbRightManager rightManager;
    int32_t userId = -1;
    rightManager.GetCurrentUserId(userId);

    int32_t oracle = -1;
    int32_t ret = AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(IPCSkeleton::GetCallingAppUid(), oracle);
    int32_t expected = (ret == 0) ? oracle : EXPECTED_RIGHT_DEFAULT_USERID;
    EXPECT_EQ(userId, expected);
    EXPECT_GE(userId, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "ret=%{public}d userId=%{public}d", ret, userId);
}

/**
 * @tc.name: SerialManager_ReportSerialOperationSecurityInfo_001
 * @tc.desc: 串口操作审计使用 GetCallingAppUid 记录 uid，直接调用完成无崩溃（事件经 hisysevent 上报）。
 * @tc.type: FUNC
 */
HWTEST_F(UsbCallingAppUidTest, SerialManager_ReportSerialOperationSecurityInfo_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERIAL, "SerialManager_ReportSerialOperationSecurityInfo_001 start");
    SERIAL::SerialManager serialManager;
    serialManager.ReportSerialOperationSecurityInfo(1, std::string("open"), 1700000000000ULL);
    EXPECT_EQ(IPCSkeleton::GetCallingAppUid(), IPCSkeleton::GetCallingUid());
    USB_HILOGI(MODULE_USB_SERIAL, "SerialManager_ReportSerialOperationSecurityInfo_001 end");
}

/**
 * @tc.name: UsbService_IsCallerValid_001
 * @tc.desc: root 进程持 native token 时 IsCallerValid 通过（ROOT_UID 命中）。
 * @tc.type: FUNC
 */
HWTEST_F(UsbCallingAppUidTest, UsbService_IsCallerValid_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService_IsCallerValid_001 start");
    SetTestNativeToken();
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    EXPECT_TRUE(service->IsCallerValid());
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService_IsCallerValid_001 end");
}

/**
 * @tc.name: UsbService_IsCallerValid_002
 * @tc.desc: HAP token 调用方不是 native token，IsCallerValid 拒绝，无提权窗口。
 * @tc.type: FUNC
 */
HWTEST_F(UsbCallingAppUidTest, UsbService_IsCallerValid_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService_IsCallerValid_002 start");
    SetTestHapToken();
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    EXPECT_FALSE(service->IsCallerValid());
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService_IsCallerValid_002 end");
}

/**
 * @tc.name: UsbService_GetBundleName_001
 * @tc.desc: GetBundleName 以 GetCallingAppUid 走 BMS 查询，root(uid 0) 非应用 uid，
 *           预期 GetNameForUid 查询失败返回 false，且流程不崩溃。
 * @tc.type: FUNC
 */
HWTEST_F(UsbCallingAppUidTest, UsbService_GetBundleName_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService_GetBundleName_001 start");
    SetTestNativeToken();
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    std::string bundleName;
    bool ret = service->GetBundleName(bundleName);
    USB_HILOGI(MODULE_USB_SERVICE, "GetBundleName ret=%{public}d name=%{public}s", ret, bundleName.c_str());
    EXPECT_FALSE(ret);
    EXPECT_TRUE(bundleName.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService_GetBundleName_001 end");
}

#ifdef USB_MANAGER_FEATURE_HOST
/**
 * @tc.name: UsbService_RemoveDeviceConnection_KeyConsistency_001
 * @tc.desc: RemoveDeviceConnection 与 AddDeviceConnection 必须同用 GetCallingAppUid 派生键。
 *           按 GetCallingAppUid 注入连接记录后调用 Remove，记录被删除且回调收到的
 *           uid 与写入键一致；若 Add/Remove 键来源不一致（迁移破坏）该用例失败。
 * @tc.type: FUNC
 */
HWTEST_F(UsbCallingAppUidTest, UsbService_RemoveDeviceConnection_KeyConsistency_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService_RemoveDeviceConnection_KeyConsistency_001 start");
    SetTestHapToken();
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);

    sptr<RecordingConnectionListener> listener(new RecordingConnectionListener());
    ASSERT_NE(listener, nullptr);
    DeviceListenerEntry entry;
    entry.listener = listener;
    service->deviceListeners_.push_back(entry);

    int32_t appUid = IPCSkeleton::GetCallingAppUid();
    std::string key = service->GetDeviceKey(TEST_BUS_NUM, TEST_DEV_ADDR);
    DeviceConnectionRecord record;
    record.apps[appUid] = TEST_HAP_BUNDLE_NAME;
    service->deviceConnectionMap_[key] = record;

    service->RemoveDeviceConnection(TEST_BUS_NUM, TEST_DEV_ADDR);

    EXPECT_EQ(service->deviceConnectionMap_.count(key), 0U);
    EXPECT_TRUE(listener->gotDisconnected);
    EXPECT_EQ(listener->lastUid, appUid);
    EXPECT_EQ(listener->lastBundleName, std::string(TEST_HAP_BUNDLE_NAME));
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService_RemoveDeviceConnection_KeyConsistency_001 end");
}

/**
 * @tc.name: UsbService_AddDeviceConnection_NullHostManager_001
 * @tc.desc: usbHostManager_ 为空时 AddDeviceConnection 防御返回，不写记录不崩溃。
 * @tc.type: FUNC
 */
HWTEST_F(UsbCallingAppUidTest, UsbService_AddDeviceConnection_NullHostManager_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService_AddDeviceConnection_NullHostManager_001 start");
    SetTestHapToken();
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    service->deviceConnectionMap_.clear();
    service->usbHostManager_ = nullptr;
    service->AddDeviceConnection(TEST_BUS_NUM, TEST_DEV_ADDR);
    EXPECT_TRUE(service->deviceConnectionMap_.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "UsbService_AddDeviceConnection_NullHostManager_001 end");
}
#endif // USB_MANAGER_FEATURE_HOST
} // namespace USB
} // namespace OHOS
