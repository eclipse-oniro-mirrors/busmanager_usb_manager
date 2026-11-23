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

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "hilog_wrapper.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "usb_endpoint.h"
#include "usb_errors.h"
#include "usb_host_manager.h"
#include "usb_right_manager.h"
#include "usb_service.h"
#include "usbd_bulk_callback.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::USB;

namespace OHOS {
namespace USB {
namespace {
constexpr uint8_t TEST_BUS_NUM = 1;
constexpr uint8_t TEST_DEV_ADDR = 2;
constexpr uint8_t TEST_INTERFACE_ID = 0;
constexpr uint32_t TEST_BULK_IN_EP_ADDR = 0x81;
constexpr uint32_t TEST_BULK_EP_ATTRIBUTES = 0x02;
constexpr uint32_t TEST_BULK_EP_MAX_PACKET = 512;
constexpr int32_t TEST_MEM_SIZE = 1024;
constexpr int32_t INVALID_MEM_SIZE = 0;
// fd 0/1/2 are stdin/stdout/stderr; the test needs a descriptor whose index is >= 3.
constexpr int32_t MIN_TEST_FD = 3;
constexpr int32_t TEST_HAP_USER_ID = 1;
constexpr int32_t TEST_HAP_API_VERSION = 18;
constexpr int32_t TEST_HAP_INST_INDEX = 0;
constexpr char TEST_HAP_BUNDLE_NAME[] = "com.example.bulksysapitest";
constexpr char TEST_NATIVE_PERM_PROCESS[] = "usb_bulk_sysapi_perm_test";
constexpr char TEST_NATIVE_NO_PERM_PROCESS[] = "usb_bulk_sysapi_noperm_test";
constexpr char MANAGE_USB_CONFIG_PERM[] = "ohos.permission.MANAGE_USB_CONFIG";
constexpr uint8_t TEST_BULK_IN_ENDPOINT = 0x81;
constexpr uint8_t TEST_BULK_OUT_ENDPOINT = 0x02;
constexpr uint8_t TEST_INTERFACE_ID_ALT = 1;
constexpr uint8_t TEST_DEV_ADDR_ALT = 10;
constexpr uint8_t TEST_BUS_NUM_ALT = 11;

USBEndpoint MakeBulkEndpoint()
{
    USBEndpoint endpoint(TEST_BULK_IN_EP_ADDR, TEST_BULK_EP_ATTRIBUTES, 0, TEST_BULK_EP_MAX_PACKET);
    endpoint.SetInterfaceId(TEST_INTERFACE_ID);
    return endpoint;
}

// Create a pipe read fd greater than stderr so that fd > 0, which is required by the
// parameter check of BulkRead and BulkWrite.
int32_t CreatePipeReadFd()
{
    int32_t fds[2] = {-1, -1};
    if (pipe(fds) != 0) {
        return -1;
    }
    (void)close(fds[1]);
    int32_t fd = fds[0];
    if (fd < MIN_TEST_FD) {
        int32_t dupFd = fcntl(fd, F_DUPFD, MIN_TEST_FD);
        (void)close(fd);
        fd = dupFd;
    }
    return fd;
}

bool IsFdClosed(int32_t fd)
{
    errno = 0;
    return (fcntl(fd, F_GETFD) == -1) && (errno == EBADF);
}

class BulkCallbackRecorder : public UsbdBulkCallBack {
public:
    int32_t OnBulkWriteCallback(int32_t status, int32_t actLength) override
    {
        (void)status;
        (void)actLength;
        return UEC_OK;
    }

    int32_t OnBulkReadCallback(int32_t status, int32_t actLength) override
    {
        (void)status;
        (void)actLength;
        return UEC_OK;
    }
};

// Switch the process to a system_basic native token, optionally holding MANAGE_USB_CONFIG.
void SetNativeToken(bool withManagePerm)
{
    const char *perms[] = {MANAGE_USB_CONFIG_PERM};
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = withManagePerm ? 1 : 0,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = withManagePerm ? perms : nullptr,
        .acls = nullptr,
        .processName = withManagePerm ? TEST_NATIVE_PERM_PROCESS : TEST_NATIVE_NO_PERM_PROCESS,
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    (void)SetSelfTokenID(tokenId);
    (void)OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

// Allocate an APL_NORMAL, non-system app HAP token and return the full 64-bit token id.
Security::AccessToken::AccessTokenIDEx AllocNormalHapToken()
{
    Security::AccessToken::HapInfoParams hapInfo = {
        .userID = TEST_HAP_USER_ID,
        .bundleName = TEST_HAP_BUNDLE_NAME,
        .instIndex = TEST_HAP_INST_INDEX,
        .appIDDesc = "usb bulk sysapi test",
        .apiVersion = TEST_HAP_API_VERSION,
        .isSystemApp = false,
    };
    Security::AccessToken::HapPolicyParams hapPolicy = {
        .apl = Security::AccessToken::APL_NORMAL,
        .domain = "test.domain.usb",
    };
    return Security::AccessToken::AccessTokenKit::AllocHapToken(hapInfo, hapPolicy);
}

// Scope guard for a normal HAP token: switches to an APL_NORMAL, non-system app HAP token
// inside the scope, and restores the system_basic native token and deletes the HAP token on
// exit so that cases do not affect each other.
class ScopedNormalHapToken {
public:
    ScopedNormalHapToken()
    {
        tokenIdEx_ = AllocNormalHapToken();
        if (IsValid()) {
            (void)SetSelfTokenID(tokenIdEx_.tokenIDEx);
        }
    }

    ~ScopedNormalHapToken()
    {
        SetNativeToken(false);
        if (IsValid()) {
            (void)Security::AccessToken::AccessTokenKit::DeleteToken(tokenIdEx_.tokenIdExStruct.tokenID);
        }
    }

    ScopedNormalHapToken(const ScopedNormalHapToken &) = delete;
    ScopedNormalHapToken &operator=(const ScopedNormalHapToken &) = delete;

    bool IsValid() const
    {
        return tokenIdEx_.tokenIdExStruct.tokenID != 0;
    }

private:
    Security::AccessToken::AccessTokenIDEx tokenIdEx_ = {0};
};
} // namespace

class UsbBulkSysApiPermissionTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() override
    {
        SetNativeToken(false);
#ifdef USB_MANAGER_FEATURE_HOST
        hostManager_ = std::make_unique<UsbHostManager>(nullptr);
#endif // USB_MANAGER_FEATURE_HOST
    }

    void TearDown() override
    {
#ifdef USB_MANAGER_FEATURE_HOST
        hostManager_.reset();
#endif // USB_MANAGER_FEATURE_HOST
        SetNativeToken(false);
    }

    // Process-wide UsbService instance created by usb_service.cpp during static init.
    // It is used directly instead of building a second SystemAbility instance with the same
    // ID, and it must never be destroyed (teardown goes through the SystemAbility unload path
    // and crashes the process).
    static UsbService *SharedService()
    {
        static sptr<UsbService> service = UsbService::GetGlobalInstance();
        return service.GetRefPtr();
    }

    // The key format must match UsbHostManager::GetBulkCallbackKey: bus-devAddr-intfId-endpointId.
    static std::string MakeCallbackKey(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId, uint8_t endpointId)
    {
        return std::to_string(busNum) + "-" + std::to_string(devAddr) + "-" +
            std::to_string(interfaceId) + "-" + std::to_string(endpointId);
    }

    // Cleanup cases only need the map entries: RemoveBulkCallbackByDevice does not touch
    // the HDI callback when usbHostInterface_ (or usbd_) is null, which is the UT case.
    void SeedCallbackEntry(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId, uint8_t endpointId)
    {
        std::string key = MakeCallbackKey(busNum, devAddr, interfaceId, endpointId);
#ifdef USB_MANAGER_PASS_THROUGH
        UsbHostManager::UsbHostHdiBulkCallbackInfo info;
        hostManager_->usbHostHdiCbMap_[key] = info;
#else
        UsbHostManager::HdiBulkCallbackInfo info;
        hostManager_->hdiCbMap_[key] = info;
#endif // USB_MANAGER_PASS_THROUGH
    }

    size_t GetSeededCallbackCount() const
    {
#ifdef USB_MANAGER_PASS_THROUGH
        return hostManager_->usbHostHdiCbMap_.size();
#else
        return hostManager_->hdiCbMap_.size();
#endif // USB_MANAGER_PASS_THROUGH
    }

    bool HasSeededCallback(uint8_t busNum, uint8_t devAddr, uint8_t interfaceId, uint8_t endpointId) const
    {
        std::string key = MakeCallbackKey(busNum, devAddr, interfaceId, endpointId);
#ifdef USB_MANAGER_PASS_THROUGH
        return hostManager_->usbHostHdiCbMap_.find(key) != hostManager_->usbHostHdiCbMap_.end();
#else
        return hostManager_->hdiCbMap_.find(key) != hostManager_->hdiCbMap_.end();
#endif // USB_MANAGER_PASS_THROUGH
    }

    std::unique_ptr<UsbHostManager> hostManager_;
};

/**
 * @tc.name: CheckSysApiPermission_NormalHapDenied_001
 * @tc.desc: Normal HAP app (APL_NORMAL, non-system app) calls CheckSysApiPermission and is
 *           rejected by the IsSystemAppOrSa check, returning UEC_SERVICE_PERMISSION_DENIED_SYSAPI.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, CheckSysApiPermission_NormalHapDenied_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "CheckSysApiPermission_NormalHapDenied_001 start");
    ScopedNormalHapToken hapToken;
    if (!hapToken.IsValid()) {
        GTEST_SKIP() << "AllocHapToken failed, skip";
    }
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->CheckSysApiPermission(), UEC_SERVICE_PERMISSION_DENIED_SYSAPI);
    USB_HILOGI(MODULE_USB_SERVICE, "CheckSysApiPermission_NormalHapDenied_001 end");
}

/**
 * @tc.name: CheckSysApiPermission_NativeNoPermDenied_001
 * @tc.desc: system_basic native process (token type NATIVE) lacks MANAGE_USB_CONFIG, so
 *           VerifyPermission fails; a native token is not a HAP token, so GetHapApiVersion
 *           falls back to API 18 and UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED is returned.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, CheckSysApiPermission_NativeNoPermDenied_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "CheckSysApiPermission_NativeNoPermDenied_001 start");
    SetNativeToken(false);
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->CheckSysApiPermission(), UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED);
    USB_HILOGI(MODULE_USB_SERVICE, "CheckSysApiPermission_NativeNoPermDenied_001 end");
}

/**
 * @tc.name: CheckSysApiPermission_NativeWithPermAllowed_001
 * @tc.desc: system_basic native process holding MANAGE_USB_CONFIG passes CheckSysApiPermission
 *           with UEC_OK, serving as the baseline of the cases that expect the gate to pass.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, CheckSysApiPermission_NativeWithPermAllowed_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "CheckSysApiPermission_NativeWithPermAllowed_001 start");
    SetNativeToken(true);
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->CheckSysApiPermission(), UEC_OK);
    USB_HILOGI(MODULE_USB_SERVICE, "CheckSysApiPermission_NativeWithPermAllowed_001 end");
}

/**
 * @tc.name: IsSystemAppOrSa_NormalHapFalse_001
 * @tc.desc: A normal HAP token is neither a system app nor a native token, so IsSystemAppOrSa
 *           returns false, which is the basis for rejecting normal apps on bulk interfaces.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, IsSystemAppOrSa_NormalHapFalse_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "IsSystemAppOrSa_NormalHapFalse_001 start");
    ScopedNormalHapToken hapToken;
    if (!hapToken.IsValid()) {
        GTEST_SKIP() << "AllocHapToken failed, skip";
    }
    UsbRightManager rightManager;
    EXPECT_FALSE(rightManager.IsSystemAppOrSa());
    USB_HILOGI(MODULE_USB_SERVICE, "IsSystemAppOrSa_NormalHapFalse_001 end");
}

/**
 * @tc.name: RegBulkCallback_NormalHapDenied_001
 * @tc.desc: A normal HAP calls RegBulkCallback; the permission check runs before the null-cb
 *           check, so it returns UEC_SERVICE_PERMISSION_DENIED_SYSAPI even when cb is null.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, RegBulkCallback_NormalHapDenied_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "RegBulkCallback_NormalHapDenied_001 start");
    ScopedNormalHapToken hapToken;
    if (!hapToken.IsValid()) {
        GTEST_SKIP() << "AllocHapToken failed, skip";
    }
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    sptr<IRemoteObject> cb;
    EXPECT_EQ(service->RegBulkCallback(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint(), cb),
        UEC_SERVICE_PERMISSION_DENIED_SYSAPI);
    USB_HILOGI(MODULE_USB_SERVICE, "RegBulkCallback_NormalHapDenied_001 end");
}

/**
 * @tc.name: UnRegBulkCallback_NormalHapDenied_001
 * @tc.desc: A normal HAP calling UnRegBulkCallback is rejected by the system SA permission
 *           check and cannot unregister another app's bulk callback.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, UnRegBulkCallback_NormalHapDenied_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UnRegBulkCallback_NormalHapDenied_001 start");
    ScopedNormalHapToken hapToken;
    if (!hapToken.IsValid()) {
        GTEST_SKIP() << "AllocHapToken failed, skip";
    }
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->UnRegBulkCallback(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint()),
        UEC_SERVICE_PERMISSION_DENIED_SYSAPI);
    USB_HILOGI(MODULE_USB_SERVICE, "UnRegBulkCallback_NormalHapDenied_001 end");
}

/**
 * @tc.name: BulkRead_NormalHapDeniedAndFdClosed_001
 * @tc.desc: A normal HAP calling BulkRead is rejected with UEC_SERVICE_PERMISSION_DENIED_SYSAPI,
 *           and the rejection path must close the caller-provided fd to avoid leaking it.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, BulkRead_NormalHapDeniedAndFdClosed_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "BulkRead_NormalHapDeniedAndFdClosed_001 start");
    ScopedNormalHapToken hapToken;
    if (!hapToken.IsValid()) {
        GTEST_SKIP() << "AllocHapToken failed, skip";
    }
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    int32_t fd = CreatePipeReadFd();
    ASSERT_GT(fd, 0);
    EXPECT_EQ(service->BulkRead(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint(), fd, TEST_MEM_SIZE),
        UEC_SERVICE_PERMISSION_DENIED_SYSAPI);
    EXPECT_TRUE(IsFdClosed(fd));
    USB_HILOGI(MODULE_USB_SERVICE, "BulkRead_NormalHapDeniedAndFdClosed_001 end");
}

/**
 * @tc.name: BulkWrite_NormalHapDeniedAndFdClosed_001
 * @tc.desc: A normal HAP calling BulkWrite is rejected with UEC_SERVICE_PERMISSION_DENIED_SYSAPI,
 *           and the rejection path must close the caller-provided fd.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, BulkWrite_NormalHapDeniedAndFdClosed_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "BulkWrite_NormalHapDeniedAndFdClosed_001 start");
    ScopedNormalHapToken hapToken;
    if (!hapToken.IsValid()) {
        GTEST_SKIP() << "AllocHapToken failed, skip";
    }
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    int32_t fd = CreatePipeReadFd();
    ASSERT_GT(fd, 0);
    EXPECT_EQ(service->BulkWrite(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint(), fd, TEST_MEM_SIZE),
        UEC_SERVICE_PERMISSION_DENIED_SYSAPI);
    EXPECT_TRUE(IsFdClosed(fd));
    USB_HILOGI(MODULE_USB_SERVICE, "BulkWrite_NormalHapDeniedAndFdClosed_001 end");
}

/**
 * @tc.name: BulkCancel_NormalHapDenied_001
 * @tc.desc: A normal HAP calling BulkCancel is rejected and cannot cancel another app's
 *           bulk transfer.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, BulkCancel_NormalHapDenied_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "BulkCancel_NormalHapDenied_001 start");
    ScopedNormalHapToken hapToken;
    if (!hapToken.IsValid()) {
        GTEST_SKIP() << "AllocHapToken failed, skip";
    }
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->BulkCancel(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint()),
        UEC_SERVICE_PERMISSION_DENIED_SYSAPI);
    USB_HILOGI(MODULE_USB_SERVICE, "BulkCancel_NormalHapDenied_001 end");
}

/**
 * @tc.name: BulkRead_NativeNoPermDeniedAndFdClosed_001
 * @tc.desc: A system native process without MANAGE_USB_CONFIG gets
 *           UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED from BulkRead, distinguishing it from
 *           the normal HAP error code; the fd is also verified to be closed.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, BulkRead_NativeNoPermDeniedAndFdClosed_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "BulkRead_NativeNoPermDeniedAndFdClosed_001 start");
    SetNativeToken(false);
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    int32_t fd = CreatePipeReadFd();
    ASSERT_GT(fd, 0);
    EXPECT_EQ(service->BulkRead(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint(), fd, TEST_MEM_SIZE),
        UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED);
    EXPECT_TRUE(IsFdClosed(fd));
    USB_HILOGI(MODULE_USB_SERVICE, "BulkRead_NativeNoPermDeniedAndFdClosed_001 end");
}

/**
 * @tc.name: RegBulkCallback_NativeNoPermDenied_001
 * @tc.desc: A system native process without MANAGE_USB_CONFIG gets
 *           UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED from RegBulkCallback; the permission
 *           check runs before callback registration.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, RegBulkCallback_NativeNoPermDenied_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "RegBulkCallback_NativeNoPermDenied_001 start");
    SetNativeToken(false);
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    sptr<IRemoteObject> cb(new BulkCallbackRecorder());
    EXPECT_EQ(service->RegBulkCallback(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint(), cb),
        UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED);
    USB_HILOGI(MODULE_USB_SERVICE, "RegBulkCallback_NativeNoPermDenied_001 end");
}

/**
 * @tc.name: BulkRead_NativeWithPermInvalidParam_001
 * @tc.desc: After the permission check passes, BulkRead continues with parameter validation;
 *           an invalid memSize returns UEC_SERVICE_INVALID_VALUE and the invalid-parameter
 *           path also closes the fd.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, BulkRead_NativeWithPermInvalidParam_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "BulkRead_NativeWithPermInvalidParam_001 start");
    SetNativeToken(true);
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    int32_t fd = CreatePipeReadFd();
    ASSERT_GT(fd, 0);
    EXPECT_EQ(service->BulkRead(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint(), fd, INVALID_MEM_SIZE),
        UEC_SERVICE_INVALID_VALUE);
    EXPECT_TRUE(IsFdClosed(fd));
    USB_HILOGI(MODULE_USB_SERVICE, "BulkRead_NativeWithPermInvalidParam_001 end");
}

#ifdef USB_MANAGER_PASS_THROUGH
/**
 * @tc.name: BulkRead_NativeWithPermDeviceRightDenied_001
 * @tc.desc: The permission check passes but no device access record exists in the UT
 *           environment, so BulkRead returns UEC_SERVICE_PERMISSION_DENIED at the device-right
 *           check and closes the fd, showing that device-right checks still apply after the
 *           system permission passes. This case relies on usbHostManager_ being created and is
 *           only valid in pass-through mode.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, BulkRead_NativeWithPermDeviceRightDenied_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "BulkRead_NativeWithPermDeviceRightDenied_001 start");
    SetNativeToken(true);
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    int32_t fd = CreatePipeReadFd();
    ASSERT_GT(fd, 0);
    EXPECT_EQ(service->BulkRead(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint(), fd, TEST_MEM_SIZE),
        UEC_SERVICE_PERMISSION_DENIED);
    EXPECT_TRUE(IsFdClosed(fd));
    USB_HILOGI(MODULE_USB_SERVICE, "BulkRead_NativeWithPermDeviceRightDenied_001 end");
}
#endif // USB_MANAGER_PASS_THROUGH

/**
 * @tc.name: RegBulkCallback_NativeWithPermDeviceRightDenied_001
 * @tc.desc: After the permission check passes, RegBulkCallback enters the device-right check;
 *           with no device access record in the UT environment it returns
 *           UEC_SERVICE_PERMISSION_DENIED instead of a system permission error.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, RegBulkCallback_NativeWithPermDeviceRightDenied_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "RegBulkCallback_NativeWithPermDeviceRightDenied_001 start");
    SetNativeToken(true);
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    sptr<IRemoteObject> cb(new BulkCallbackRecorder());
    EXPECT_EQ(service->RegBulkCallback(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint(), cb),
        UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_SERVICE, "RegBulkCallback_NativeWithPermDeviceRightDenied_001 end");
}

/**
 * @tc.name: UnRegBulkCallback_NativeWithPermDeviceRightDenied_001
 * @tc.desc: After the permission check passes, UnRegBulkCallback continues with the device-right
 *           check and returns UEC_SERVICE_PERMISSION_DENIED when no access record exists.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, UnRegBulkCallback_NativeWithPermDeviceRightDenied_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UnRegBulkCallback_NativeWithPermDeviceRightDenied_001 start");
    SetNativeToken(true);
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->UnRegBulkCallback(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint()),
        UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_SERVICE, "UnRegBulkCallback_NativeWithPermDeviceRightDenied_001 end");
}

/**
 * @tc.name: BulkCancel_NativeWithPermDeviceRightDenied_001
 * @tc.desc: After the permission check passes, BulkCancel continues with the device-right check
 *           and returns UEC_SERVICE_PERMISSION_DENIED when no access record exists.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, BulkCancel_NativeWithPermDeviceRightDenied_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "BulkCancel_NativeWithPermDeviceRightDenied_001 start");
    SetNativeToken(true);
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->BulkCancel(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint()),
        UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_SERVICE, "BulkCancel_NativeWithPermDeviceRightDenied_001 end");
}

#ifdef USB_MANAGER_FEATURE_HOST
/**
 * @tc.name: RemoveBulkCallbackByDevice_MultiEndpointCleanup_001
 * @tc.desc: RemoveBulkCallbackByDevice must erase every callback of one device, including
 *           entries of different interfaces and endpoints.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, RemoveBulkCallbackByDevice_MultiEndpointCleanup_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "RemoveBulkCallbackByDevice_MultiEndpointCleanup_001 start");
    SeedCallbackEntry(TEST_BUS_NUM, TEST_DEV_ADDR, TEST_INTERFACE_ID, TEST_BULK_IN_ENDPOINT);
    SeedCallbackEntry(TEST_BUS_NUM, TEST_DEV_ADDR, TEST_INTERFACE_ID, TEST_BULK_OUT_ENDPOINT);
    SeedCallbackEntry(TEST_BUS_NUM, TEST_DEV_ADDR, TEST_INTERFACE_ID_ALT, TEST_BULK_IN_ENDPOINT);
    ASSERT_EQ(GetSeededCallbackCount(), 3U);

    hostManager_->RemoveBulkCallbackByDevice(TEST_BUS_NUM, TEST_DEV_ADDR);

    EXPECT_EQ(GetSeededCallbackCount(), 0U);
    USB_HILOGI(MODULE_USB_SERVICE, "RemoveBulkCallbackByDevice_MultiEndpointCleanup_001 end");
}

/**
 * @tc.name: RemoveBulkCallbackByDevice_PrefixIsolation_001
 * @tc.desc: Removing the callbacks of device (1, 1) must not erase (1, 10), (11, 1) or (2, 1),
 *           whose keys share a textual prefix with the "1-1-" key prefix of the target device.
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkSysApiPermissionTest, RemoveBulkCallbackByDevice_PrefixIsolation_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "RemoveBulkCallbackByDevice_PrefixIsolation_001 start");
    SeedCallbackEntry(1, 1, TEST_INTERFACE_ID, TEST_BULK_IN_ENDPOINT);
    SeedCallbackEntry(1, TEST_DEV_ADDR_ALT, TEST_INTERFACE_ID, TEST_BULK_IN_ENDPOINT);
    SeedCallbackEntry(TEST_BUS_NUM_ALT, 1, TEST_INTERFACE_ID, TEST_BULK_IN_ENDPOINT);
    SeedCallbackEntry(2, 1, TEST_INTERFACE_ID, TEST_BULK_OUT_ENDPOINT);
    ASSERT_EQ(GetSeededCallbackCount(), 4U);

    hostManager_->RemoveBulkCallbackByDevice(1, 1);

    EXPECT_EQ(GetSeededCallbackCount(), 3U);
    EXPECT_FALSE(HasSeededCallback(1, 1, TEST_INTERFACE_ID, TEST_BULK_IN_ENDPOINT));
    EXPECT_TRUE(HasSeededCallback(1, TEST_DEV_ADDR_ALT, TEST_INTERFACE_ID, TEST_BULK_IN_ENDPOINT));
    EXPECT_TRUE(HasSeededCallback(TEST_BUS_NUM_ALT, 1, TEST_INTERFACE_ID, TEST_BULK_IN_ENDPOINT));
    EXPECT_TRUE(HasSeededCallback(2, 1, TEST_INTERFACE_ID, TEST_BULK_OUT_ENDPOINT));
    USB_HILOGI(MODULE_USB_SERVICE, "RemoveBulkCallbackByDevice_PrefixIsolation_001 end");
}
#endif // USB_MANAGER_FEATURE_HOST
} // namespace USB
} // namespace OHOS
