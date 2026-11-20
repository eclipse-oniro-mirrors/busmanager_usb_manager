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

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "hilog_wrapper.h"
#include "usb_common_test.h"
#include "usb_endpoint.h"
#include "usb_errors.h"
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
    if (fd <= 2) {
        int32_t dupFd = fcntl(fd, F_DUPFD, 3);
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

// Scope guard for a normal HAP token: switches to an APL_NORMAL, non-system app HAP token
// inside the scope, and restores the system_basic native token and deletes the HAP token on
// exit so that cases do not affect each other.
class ScopedNormalHapToken {
public:
    ScopedNormalHapToken() : tokenId_(Common::UsbCommonTest::AllocHapTest())
    {
        if (tokenId_ != 0) {
            Common::UsbCommonTest::SetSelfToken(tokenId_);
        }
    }

    ~ScopedNormalHapToken()
    {
        Common::UsbCommonTest::GrantSysNoPermissionNative();
        if (tokenId_ != 0) {
            Common::UsbCommonTest::DeleteAllocHapToken(tokenId_);
        }
    }

    ScopedNormalHapToken(const ScopedNormalHapToken &) = delete;
    ScopedNormalHapToken &operator=(const ScopedNormalHapToken &) = delete;

    bool IsValid() const
    {
        return tokenId_ != 0;
    }

private:
    Security::AccessToken::AccessTokenID tokenId_ = 0;
};
} // namespace

class UsbBulkSysApiPermissionTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() override
    {
        Common::UsbCommonTest::GrantSysNoPermissionNative();
    }

    void TearDown() override
    {
        Common::UsbCommonTest::GrantSysNoPermissionNative();
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
    Common::UsbCommonTest::GrantSysNoPermissionNative();
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
    Common::UsbCommonTest::GrantPermissionSysNative();
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
    Common::UsbCommonTest::GrantSysNoPermissionNative();
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
    Common::UsbCommonTest::GrantSysNoPermissionNative();
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
    Common::UsbCommonTest::GrantPermissionSysNative();
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
    Common::UsbCommonTest::GrantPermissionSysNative();
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
    Common::UsbCommonTest::GrantPermissionSysNative();
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
    Common::UsbCommonTest::GrantPermissionSysNative();
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
    Common::UsbCommonTest::GrantPermissionSysNative();
    UsbService *service = SharedService();
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->BulkCancel(TEST_BUS_NUM, TEST_DEV_ADDR, MakeBulkEndpoint()),
        UEC_SERVICE_PERMISSION_DENIED);
    USB_HILOGI(MODULE_USB_SERVICE, "BulkCancel_NativeWithPermDeviceRightDenied_001 end");
}
} // namespace USB
} // namespace OHOS
