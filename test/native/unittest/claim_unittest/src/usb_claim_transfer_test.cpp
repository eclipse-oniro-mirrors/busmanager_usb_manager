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

#include "usb_claim_transfer_test.h"

#include <vector>

#include "ashmem.h"
#include "hilog_wrapper.h"
#include "iusb_srv.h"
#include "usb_errors.h"
#include "v1_2/usb_types.h"

using namespace testing::ext;
using OHOS::HDI::Usb::V1_2::USBTransferInfo;

namespace OHOS {
namespace USB {
namespace {
constexpr int32_t TRANSFER_BUFFER_SIZE = 8;
constexpr int32_t TRANSFER_TIMEOUT_MS = 300;
constexpr int32_t DEVICE_DESC_BUFFER_SIZE = 18;
constexpr uint8_t USB_SERVICE_REQ_GET_DESCRIPTOR = 0x06;
// 0x80 (IN) | 0x00 (recipient: device): standard bmRequestType for GET_DESCRIPTOR(device)
constexpr uint32_t CTRL_REQ_TYPE_DEVICE_IN = 0x80;
constexpr uint32_t DEVICE_DESC_VALUE = 0x100; // descriptor type 1 (device), index 0

/*
 * The transfer result of an allowed transfer depends on the endpoint type of
 * the test device (an off-the-shelf device usually exposes an interrupt IN
 * endpoint only), so an allowed transfer is asserted with ret not equal to
 * UEC_SERVICE_IO_EXCEPTION: any other value proves that the request passed
 * the exclusive claim check and reached the HDI layer. A blocked transfer is
 * asserted strictly with UEC_SERVICE_IO_EXCEPTION.
 */

void DoBulkRead(USBDevicePipe &pipe, const USBEndpoint &endpoint, int32_t &ret)
{
    std::vector<uint8_t> buffer(TRANSFER_BUFFER_SIZE, 0);
    auto &client = UsbSrvClient::GetInstance();
    ret = client.BulkTransfer(pipe, endpoint, buffer, TRANSFER_TIMEOUT_MS);
}

void DoBulkWrite(USBDevicePipe &pipe, const USBEndpoint &endpoint, int32_t &ret)
{
    std::vector<uint8_t> buffer(TRANSFER_BUFFER_SIZE, 0);
    auto &client = UsbSrvClient::GetInstance();
    ret = client.BulkTransfer(pipe, endpoint, buffer, TRANSFER_TIMEOUT_MS);
}

void DoControlRead(USBDevicePipe &pipe, int32_t &ret)
{
    // standard GET_DESCRIPTOR(device) request over endpoint 0
    std::vector<uint8_t> buffer(DEVICE_DESC_BUFFER_SIZE, 0);
    HDI::Usb::V1_0::UsbCtrlTransfer ctrl = {CTRL_REQ_TYPE_DEVICE_IN,
        USB_SERVICE_REQ_GET_DESCRIPTOR, DEVICE_DESC_VALUE, 0, TRANSFER_TIMEOUT_MS};
    auto &client = UsbSrvClient::GetInstance();
    ret = client.ControlTransfer(pipe, ctrl, buffer);
}

void DoUsbControlRead(USBDevicePipe &pipe, int32_t &ret)
{
    // standard GET_DESCRIPTOR(device) request over endpoint 0 (v1.2 path)
    std::vector<uint8_t> buffer(DEVICE_DESC_BUFFER_SIZE, 0);
    HDI::Usb::V1_2::UsbCtrlTransferParams ctrl = {CTRL_REQ_TYPE_DEVICE_IN,
        USB_SERVICE_REQ_GET_DESCRIPTOR, DEVICE_DESC_VALUE, 0, DEVICE_DESC_BUFFER_SIZE, TRANSFER_TIMEOUT_MS};
    auto &client = UsbSrvClient::GetInstance();
    ret = client.UsbControlTransfer(pipe, ctrl, buffer);
}

/*
 * UsbSubmitTransfer works on the given endpoint. The transfer type is taken
 * from the endpoint descriptor so the request matches a bulk/interrupt
 * device.
 */
void DoSubmitTransfer(USBDevicePipe &pipe, const USBEndpoint &endpoint, int32_t &ret)
{
    auto &client = UsbSrvClient::GetInstance();
    sptr<Ashmem> ashmem = Ashmem::CreateAshmem("usb_claim_transfer", TRANSFER_BUFFER_SIZE);
    if (ashmem == nullptr) {
        ret = UEC_SERVICE_INVALID_VALUE;
        return;
    }
    ashmem->MapReadAndWriteAshmem();
    USBTransferInfo transferInfo;
    transferInfo.endpoint = static_cast<int32_t>(endpoint.GetAddress());
    transferInfo.flags = 0;
    transferInfo.type = static_cast<int32_t>(endpoint.GetType());
    transferInfo.timeOut = TRANSFER_TIMEOUT_MS;
    transferInfo.length = TRANSFER_BUFFER_SIZE;
    transferInfo.userData = 0;
    transferInfo.numIsoPackets = 0;
    auto callback = [](const TransferCallbackInfo &info,
        const std::vector<HDI::Usb::V1_2::UsbIsoPacketDescriptor> &packets, uint64_t userData) {};
    ret = client.UsbSubmitTransfer(pipe, transferInfo, callback, ashmem);
}
} // namespace

USBEndpoint UsbClaimTransferTest::inEndpoint_ {};
USBEndpoint UsbClaimTransferTest::outEndpoint_ {};
bool UsbClaimTransferTest::hasInEndpoint_ = false;
bool UsbClaimTransferTest::hasOutEndpoint_ = false;

void UsbClaimTransferTest::SetUpTestCase()
{
    UsbClaimTestBase::SetUpTestCase();
    for (auto &endpoint : iface_.GetEndpoints()) {
        if (endpoint.GetDirection() == USB_ENDPOINT_DIR_IN && !hasInEndpoint_) {
            inEndpoint_ = endpoint;
            hasInEndpoint_ = true;
        } else if (endpoint.GetDirection() == USB_ENDPOINT_DIR_OUT && !hasOutEndpoint_) {
            outEndpoint_ = endpoint;
            hasOutEndpoint_ = true;
        }
    }
    USB_HILOGI(MODULE_USB_SERVICE, "endpoints in=%{public}d out=%{public}d",
        static_cast<int32_t>(hasInEndpoint_), static_cast<int32_t>(hasOutEndpoint_));
}

void UsbClaimTransferTest::TearDownTestCase()
{
    UsbClaimTestBase::TearDownTestCase();
}

/**
 * @tc.name: ClaimTransfer001
 * @tc.desc: Test transfer of the exclusive owner
 * @tc.desc: Positive test: the exclusive owner passes the transfer check on
 *           both directions offered by the device
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer001, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    if (hasInEndpoint_) {
        DoBulkRead(pipe_, inEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no IN endpoint, skip read variant";
    }
    if (hasOutEndpoint_) {
        DoBulkWrite(pipe_, outEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no OUT endpoint, skip write variant";
    }
}

/**
 * @tc.name: ClaimTransfer002
 * @tc.desc: Test transfer of another application while the interface is
 *           exclusively claimed only
 * @tc.desc: Negative test: blocked with UEC_SERVICE_IO_EXCEPTION on both
 *           directions offered by the device
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer002, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    int32_t transferRet = UEC_OK;
    if (hasInEndpoint_) {
        DoBulkRead(pipe_, inEndpoint_, transferRet);
        EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no IN endpoint, skip read variant";
    }
    if (hasOutEndpoint_) {
        DoBulkWrite(pipe_, outEndpoint_, transferRet);
        EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no OUT endpoint, skip write variant";
    }
}

/**
 * @tc.name: ClaimTransfer003
 * @tc.desc: Test transfer of the normal claimer after a normal claim
 * @tc.desc: Positive test: a normal claim disables the transfer check
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer003, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    if (hasInEndpoint_) {
        DoBulkRead(pipe_, inEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no IN endpoint, skip read variant";
    }
    if (hasOutEndpoint_) {
        DoBulkWrite(pipe_, outEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no OUT endpoint, skip write variant";
    }
}

/**
 * @tc.name: ClaimTransfer004
 * @tc.desc: Test transfer of a third application while a normal claim exists
 * @tc.desc: Positive test: a normal claim disables the transfer check for
 *           every application
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer004, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    int32_t transferRet = UEC_OK;
    if (hasInEndpoint_) {
        DoBulkRead(pipe_, inEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no IN endpoint, skip read variant";
    }
    if (hasOutEndpoint_) {
        DoBulkWrite(pipe_, outEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no OUT endpoint, skip write variant";
    }
}

/**
 * @tc.name: ClaimTransfer005
 * @tc.desc: Test transfer while only a normal claim exists
 * @tc.desc: Positive test: no exclusive claim means no transfer check
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer005, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    int32_t transferRet = UEC_OK;
    if (hasInEndpoint_) {
        DoBulkRead(pipe_, inEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no IN endpoint, skip read variant";
    }
    if (hasOutEndpoint_) {
        DoBulkWrite(pipe_, outEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no OUT endpoint, skip write variant";
    }
}

/**
 * @tc.name: ClaimTransfer006
 * @tc.desc: Test transfer on an idle interface
 * @tc.desc: Positive test: regression, existing behavior is unchanged
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer006, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    SwitchToApp(CLAIM_APP_B);
    int32_t transferRet = UEC_OK;
    if (hasInEndpoint_) {
        DoBulkRead(pipe_, inEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no IN endpoint, skip read variant";
    }
    if (hasOutEndpoint_) {
        DoBulkWrite(pipe_, outEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no OUT endpoint, skip write variant";
    }
}

/**
 * @tc.name: ClaimTransfer007
 * @tc.desc: Test transfer after the exclusive claim is released
 * @tc.desc: Positive test: transfer check is disabled again after release
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer007, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    int32_t transferRet = UEC_OK;
    if (hasInEndpoint_) {
        DoBulkRead(pipe_, inEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no IN endpoint, skip read variant";
    }
    if (hasOutEndpoint_) {
        DoBulkWrite(pipe_, outEndpoint_, transferRet);
        EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no OUT endpoint, skip write variant";
    }
}

/**
 * @tc.name: ClaimTransfer008
 * @tc.desc: Test transfer after the normal claimer released while the
 *           exclusive claim remains
 * @tc.desc: Negative test: transfer is blocked again because only the
 *           exclusive claim remains
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer008, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    ret = client.ReleaseInterface(pipe_, iface_);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    if (hasInEndpoint_) {
        DoBulkRead(pipe_, inEndpoint_, transferRet);
        EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no IN endpoint, skip read variant";
    }
    if (hasOutEndpoint_) {
        DoBulkWrite(pipe_, outEndpoint_, transferRet);
        EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no OUT endpoint, skip write variant";
    }
}

/**
 * @tc.name: ClaimTransfer009
 * @tc.desc: Test repeated transfers of a blocked application
 * @tc.desc: Negative test: the block decision is stable across transfers
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer009, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    int32_t transferRet = UEC_OK;
    if (hasInEndpoint_) {
        DoBulkRead(pipe_, inEndpoint_, transferRet);
        EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
        DoBulkRead(pipe_, inEndpoint_, transferRet);
        EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no IN endpoint, skip read variant";
    }
    if (hasOutEndpoint_) {
        DoBulkWrite(pipe_, outEndpoint_, transferRet);
        EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
        DoBulkWrite(pipe_, outEndpoint_, transferRet);
        EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no OUT endpoint, skip write variant";
    }
}

/**
 * @tc.name: ClaimTransfer010
 * @tc.desc: Test transfer of a third application while the interface is
 *           exclusively claimed only
 * @tc.desc: Negative test: blocked with UEC_SERVICE_IO_EXCEPTION
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer010, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_C);
    int32_t transferRet = UEC_OK;
    if (hasInEndpoint_) {
        DoBulkRead(pipe_, inEndpoint_, transferRet);
        EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no IN endpoint, skip read variant";
    }
    if (hasOutEndpoint_) {
        DoBulkWrite(pipe_, outEndpoint_, transferRet);
        EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
    } else {
        SUCCEED() << "no OUT endpoint, skip write variant";
    }
}

/**
 * @tc.name: ClaimTransfer011
 * @tc.desc: Test UsbSubmitTransfer of the exclusive owner
 * @tc.desc: Positive test: the exclusive owner passes the transfer check of
 *           UsbSubmitTransfer
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer011, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ASSERT_TRUE(hasInEndpoint_ || hasOutEndpoint_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    USBEndpoint submitEp = hasInEndpoint_ ? inEndpoint_ : outEndpoint_;
    DoSubmitTransfer(pipe_, submitEp, transferRet);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimTransfer012
 * @tc.desc: Test UsbSubmitTransfer of another application while the
 *           interface is exclusively claimed only
 * @tc.desc: Negative test: blocked with UEC_SERVICE_IO_EXCEPTION
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer012, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ASSERT_TRUE(hasInEndpoint_ || hasOutEndpoint_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    int32_t transferRet = UEC_OK;
    USBEndpoint submitEp = hasInEndpoint_ ? inEndpoint_ : outEndpoint_;
    DoSubmitTransfer(pipe_, submitEp, transferRet);
    EXPECT_EQ(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimTransfer013
 * @tc.desc: Test UsbSubmitTransfer while a normal claim exists
 * @tc.desc: Positive test: a normal claim disables the transfer check of
 *           UsbSubmitTransfer
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer013, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    ASSERT_TRUE(hasInEndpoint_ || hasOutEndpoint_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    ret = client.ClaimInterface(pipe_, iface_, true);
    EXPECT_EQ(UEC_OK, ret);
    int32_t transferRet = UEC_OK;
    USBEndpoint submitEp = hasInEndpoint_ ? inEndpoint_ : outEndpoint_;
    DoSubmitTransfer(pipe_, submitEp, transferRet);
    EXPECT_NE(UEC_SERVICE_IO_EXCEPTION, transferRet);
}

/**
 * @tc.name: ClaimTransfer014
 * @tc.desc: Test ControlTransfer while the interface is exclusively claimed
 * @tc.desc: Positive test: a standard GET_DESCRIPTOR(device) control request
 *           over endpoint 0 succeeds even for a blocked application, because
 *           endpoint 0 belongs to no interface and is not subject to the
 *           exclusive claim check
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer014, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    int32_t transferRet = UEC_OK;
    DoControlRead(pipe_, transferRet);
    EXPECT_EQ(UEC_OK, transferRet);
}

/**
 * @tc.name: ClaimTransfer015
 * @tc.desc: Test UsbControlTransfer while the interface is exclusively claimed
 * @tc.desc: Positive test: the v1.2 control transfer path with a standard
 *           GET_DESCRIPTOR(device) request succeeds as well
 * @tc.type: FUNC
 */
HWTEST_F(UsbClaimTransferTest, ClaimTransfer015, TestSize.Level1)
{
    ASSERT_TRUE(deviceReady_);
    auto &client = UsbSrvClient::GetInstance();
    SwitchToApp(CLAIM_APP_A);
    auto ret = client.ClaimInterfaceExclusive(pipe_, iface_, true, nullptr);
    EXPECT_EQ(UEC_OK, ret);
    SwitchToApp(CLAIM_APP_B);
    int32_t transferRet = UEC_OK;
    DoUsbControlRead(pipe_, transferRet);
    EXPECT_EQ(UEC_OK, transferRet);
}
} // namespace USB
} // namespace OHOS
