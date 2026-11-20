/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "usb_request_mock_test.h"

#include <csignal>
#include <cstring>
#include <iostream>
#include <vector>

#include "iusb_srv.h"
#include "usb_common.h"
#include "usb_common_test.h"
#include "usb_impl_mock.h"
#include "usb_request.h"
#include "usb_service.h"
#include "usb_srv_client.h"

using namespace OHOS;
using namespace OHOS::USB;
using namespace OHOS::USB::Common;
using namespace std;
using namespace testing::ext;
using ::testing::Eq;
using ::testing::Exactly;
using ::testing::Ge;
using ::testing::Le;
using ::testing::Ne;
using ::testing::Return;

namespace OHOS {
namespace USB {
constexpr int32_t BUFFER_SIZE = 255;
constexpr int32_t REQUEST_TIME_OUT = 5000;
sptr<MockUsbImpl> UsbRequestMockTest::mockUsbImpl_ = nullptr;
sptr<UsbService> UsbRequestMockTest::usbSrv_ = nullptr;
UsbDev UsbRequestMockTest::dev_ = {BUS_NUM_OK, DEV_ADDR_OK};
UsbInterface UsbRequestMockTest::interface_ {};

void UsbRequestMockTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest SetUpTestCase");
    UsbCommonTest::SetTestCaseHapApply();

    usbSrv_ = DelayedSpSingleton<UsbService>::GetInstance();
    EXPECT_NE(usbSrv_, nullptr);
    mockUsbImpl_ = DelayedSpSingleton<MockUsbImpl>::GetInstance();
    EXPECT_NE(mockUsbImpl_, nullptr);

    usbSrv_->SetUsbd(mockUsbImpl_);

    sptr<UsbServiceSubscriber> iSubscriber = new UsbServiceSubscriber();
    EXPECT_NE(iSubscriber, nullptr);
    mockUsbImpl_->BindUsbdSubscriber(iSubscriber);

    EXPECT_CALL(*mockUsbImpl_, OpenDevice(testing::_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockUsbImpl_, CloseDevice(testing::_)).WillRepeatedly(Return(0));
    USBDeviceInfo info = {ACT_DEVUP, BUS_NUM_OK, DEV_ADDR_OK};
    auto ret = mockUsbImpl_->SubscriberDeviceEvent(info);
    EXPECT_EQ(0, ret);

    ret = mockUsbImpl_->SetPortRole(DEFAULT_PORT_ID, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    EXPECT_EQ(0, ret);
    if (ret != 0) {
        exit(0);
    }

    vector<UsbDevice> devList;
    ret = usbSrv_->GetDevices(devList);
    EXPECT_EQ(0, ret);
    EXPECT_FALSE(devList.empty()) << "devList NULL";
    UsbDevice device = MockUsbImpl::FindDeviceInfo(devList);

    EXPECT_CALL(*mockUsbImpl_, OpenDevice(testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->OpenDevice(dev_.busNum, dev_.devAddr);
    EXPECT_EQ(0, ret);

    interface_ = device.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface_.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);
}

void UsbRequestMockTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest TearDownTestCase");
    EXPECT_CALL(*mockUsbImpl_, CloseDevice(testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->Close(dev_.busNum, dev_.devAddr);
    EXPECT_EQ(0, ret);

    USBDeviceInfo info = {ACT_DEVDOWN, BUS_NUM_OK, DEV_ADDR_OK};
    ret = mockUsbImpl_->SubscriberDeviceEvent(info);
    EXPECT_EQ(0, ret);

    mockUsbImpl_->UnbindUsbdSubscriber(nullptr);
    sptr<IUsbInterface> usbd = IUsbInterface::Get();
    usbSrv_->SetUsbd(usbd);

    mockUsbImpl_ = nullptr;
    usbSrv_ = nullptr;
    DelayedSpSingleton<UsbService>::DestroyInstance();
    DelayedSpSingleton<MockUsbImpl>::DestroyInstance();
}

void UsbRequestMockTest::SetUp(void) {}

void UsbRequestMockTest::TearDown(void) {}
/**
 * @tc.name: UsbrequestQueue001
 * @tc.desc: Test functions of RequestQueue(UsbRequest &request, uint8_t *buffer, uint32_t length)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbrequestQueue001, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().at(0);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'q', 'u', 'e', 'u', 'e', ' ', 'r', 'e', 'a', 'd', '0', '0', '1'};
    std::vector<uint8_t> bufferData(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest::UsbrequestQueue001 queue=%{public}d ", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbrequestQueue002
 * @tc.desc: Test functions of RequestQueue(UsbRequest &request, uint8_t *buffer, uint32_t length)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbrequestQueue002, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().at(1);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'q', 'u', 'e', 'u', 'e', ' ', 'w', 'r', 'i', 't', 'e', '0', '0', '1'};
    std::vector<uint8_t> dataBuffer = {'r', 'e', 'q', 'u', 'e', 's', 't', ' ', 't', 'e', 's', 't'};
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, dataBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:UsbrequestQueue002 queue=%{public}d ", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbrequestQueue003
 * @tc.desc: Test functions of RequestQueue(UsbRequest &request, uint8_t *buffer, uint32_t length)
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbrequestQueue003, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().at(0);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'q', 'u', 'e', 'u', 'e', ' ', 'r', 'e', 'a', 'd', '0', '0', '3'};
    std::vector<uint8_t> bufferData(BUFFER_SIZE);
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest::UsbrequestQueue003 queue=%{public}d ", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbrequestQueue004
 * @tc.desc: Test functions of RequestQueue(UsbRequest &request, uint8_t *buffer, uint32_t length)
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbrequestQueue004, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().at(0);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'q', 'u', 'e', 'u', 'e', ' ', 'r', 'e', 'a', 'd', '0', '0', '4'};
    std::vector<uint8_t> bufferData(BUFFER_SIZE);
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest::UsbrequestQueue004 queue=%{public}d ", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbrequestQueue005
 * @tc.desc: Test functions of RequestQueue(UsbRequest &request, uint8_t *buffer, uint32_t length)
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbrequestQueue005, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().at(1);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'q', 'u', 'e', 'u', 'e', ' ', 'w', 'r', 'i', 't', 'e', '0', '0', '5'};
    std::vector<uint8_t> dataBuffer = {'r', 'e', 'q', 'u', 'e', 's', 't', ' ', 't', 'e', 's', 't'};
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, dataBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest::UsbrequestQueue005 queue=%{public}d ", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbrequestQueue006
 * @tc.desc: Test functions of RequestQueue(UsbRequest &request, uint8_t *buffer, uint32_t length)
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbrequestQueue006, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().at(1);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'q', 'u', 'e', 'u', 'e', ' ', 'w', 'r', 'i', 't', 'e', '0', '0', '6'};
    std::vector<uint8_t> dataBuffer = {'r', 'e', 'q', 'u', 'e', 's', 't', ' ', 't', 'e', 's', 't'};
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, dataBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest::UsbrequestQueue006 queue=%{public}d ", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: RequestCancel001
 * @tc.desc: Test functions of RequestFree(UsbRequest &request);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, RequestCancel001, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(0);
    uint8_t endpointId = point.GetAddress();
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:RequestCancel001 free=%{public}d ", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: RequestCancel002
 * @tc.desc: Test functions of RequestFree(UsbRequest &request);
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, RequestCancel002, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(0);
    uint8_t endpointId = point.GetAddress();
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:RequestCancel002 free=%{public}d ", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: RequestCancel003
 * @tc.desc: Test functions of RequestFree(UsbRequest &request);
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, RequestCancel003, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(0);
    uint8_t endpointId = point.GetAddress();
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:RequestCancel003 free=%{public}d ", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: RequestCancel004
 * @tc.desc: Test functions of RequestFree(UsbRequest &request);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, RequestCancel004, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(1);
    uint8_t endpointId = point.GetAddress();
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:RequestCancel004 free=%{public}d ", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: RequestCancel005
 * @tc.desc: Test functions of RequestFree(UsbRequest &request);
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, RequestCancel005, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(1);
    uint8_t endpointId = point.GetAddress();
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:RequestCancel005 free=%{public}d ", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: RequestCancel006
 * @tc.desc: Test functions of RequestFree(UsbRequest &request);
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, RequestCancel006, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(1);
    uint8_t endpointId = point.GetAddress();
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:RequestCancel006 free=%{public}d ", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: Usbrequestabort001
 * @tc.desc: Test functions of RequestAbort(request)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, Usbrequestabort001, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(0);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'h', 'e', 'l', 'l', 'o', ' ', 'a', 'b', 'o', 'r', 't', '0', '0', '1'};
    std::vector<uint8_t> dataBuffer = {'a', 'b', 'o', 'r', 't', '0', '0', '1'};
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, dataBuffer);
    EXPECT_EQ(0, ret);

    uint8_t endpointId = point.GetAddress();
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:Usbrequestabort001 Abort=%{public}d ", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: Usbrequestabort002
 * @tc.desc: Test functions of RequestAbort(request)
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, Usbrequestabort002, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(0);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'a', 'b', 'o', 'r', 't', '0', '0', '2'};
    std::vector<uint8_t> bufferData(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, bufferData);
    EXPECT_EQ(0, ret);

    uint8_t endpointId = point.GetAddress();
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:Usbrequestabort002 Abort=%{public}d ", ret);
    EXPECT_NE(ret, 0);
    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: Usbrequestabort003
 * @tc.desc: Test functions of RequestAbort(request)
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, Usbrequestabort003, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(0);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'a', 'b', 'o', 'r', 't', '0', '0', '3'};
    std::vector<uint8_t> bufferData(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, bufferData);
    EXPECT_EQ(0, ret);

    uint8_t endpointId = point.GetAddress();
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:Usbrequestabort003 Abort=%{public}d ", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: Usbrequestabort004
 * @tc.desc: Test functions of RequestAbort(request)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, Usbrequestabort004, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(1);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'h', 'e', 'l', 'l', 'o', ' ', 'a', 'b', 'o', 'r', 't', '0', '0', '4'};
    std::vector<uint8_t> dataBuffer = {'a', 'b', 'o', 'r', 't', '0', '0', '4'};
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, dataBuffer);
    EXPECT_EQ(0, ret);

    uint8_t endpointId = point.GetAddress();
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:Usbrequestabort004 abort=%{public}d ", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: Usbrequestabort005
 * @tc.desc: Test functions of RequestAbort(request)
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, Usbrequestabort005, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(1);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'h', 'e', 'l', 'l', 'o', ' ', 'a', 'b', 'o', 'r', 't', '0', '0', '5'};
    std::vector<uint8_t> dataBuffer = {'a', 'b', 'o', 'r', 't', '0', '0', '5'};
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, dataBuffer);
    EXPECT_EQ(0, ret);

    uint8_t endpointId = point.GetAddress();
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:Usbrequestabort005 Abort=%{public}d ", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: Usbrequestabort006
 * @tc.desc: Test functions of RequestAbort(request)
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, Usbrequestabort006, TestSize.Level1)
{
    uint8_t interfaceId = interface_.GetId();
    USBEndpoint point = interface_.GetEndpoints().at(1);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'h', 'e', 'l', 'l', 'o', ' ', 'a', 'b', 'o', 'r', 't', '0', '0', '6'};
    std::vector<uint8_t> dataBuffer = {'a', 'b', 'o', 'r', 't', '0', '0', '6'};
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, dataBuffer);
    EXPECT_EQ(0, ret);

    uint8_t endpointId = point.GetAddress();
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestCancel(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->RequestCancel(dev_.busNum, dev_.devAddr, interfaceId, endpointId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest:Usbrequestabort006 Abort=%{public}d ", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbRequestWait001
 * @tc.desc: Test functions of UsbRequestWait(USBDevicePipe &pip, int64_t timeout, UsbRequest &req)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbRequestWait001, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().front();
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'w', 'a', 'i', 't', '0', '0', '1'};
    std::vector<uint8_t> bufferData(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, bufferData);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, RequestWait(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->RequestWait(dev_, REQUEST_TIME_OUT, cmdData, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest::UsbRequestWait001 UsbRequestWait=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbRequestWait002
 * @tc.desc: Test functions of UsbRequestWait(USBDevicePipe &pip, int64_t timeout, UsbRequest &req)
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbRequestWait002, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().front();
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'w', 'a', 'i', 't', '0', '0', '2'};
    std::vector<uint8_t> bufferData(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, bufferData);
    EXPECT_EQ(0, ret);

    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestWait(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->RequestWait(dev_, REQUEST_TIME_OUT, cmdData, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest::UsbRequestWait002 UsbRequestWait=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbRequestWait003
 * @tc.desc: Test functions of UsbRequestWait(USBDevicePipe &pip, int64_t timeout, UsbRequest &req)
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbRequestWait003, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().front();
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'w', 'a', 'i', 't', '0', '0', '3'};
    std::vector<uint8_t> bufferData(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, bufferData);
    EXPECT_EQ(0, ret);

    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestWait(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->RequestWait(dev_, REQUEST_TIME_OUT, cmdData, bufferData);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest::UsbRequestWait003 UsbRequestWait=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbRequestWait004
 * @tc.desc: Test functions of UsbRequestWait(USBDevicePipe &pip, int64_t timeout, UsbRequest &req)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbRequestWait004, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().at(1);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'w', 'a', 'i', 't', '0', '0', '4'};
    std::vector<uint8_t> dataBuffer = {'w', 'a', 'i', 't', ' ', 't', 'e', 's', 't', '0', '0', '4'};
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, dataBuffer);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, RequestWait(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->RequestWait(dev_, REQUEST_TIME_OUT, cmdData, dataBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest::UsbRequestWait004 UsbRequestWait=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbRequestWait005
 * @tc.desc: Test functions of UsbRequestWait(USBDevicePipe &pip, int64_t timeout, UsbRequest &req)
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbRequestWait005, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().at(1);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'w', 'a', 'i', 't', '0', '0', '5'};
    std::vector<uint8_t> dataBuffer = {'w', 'a', 'i', 't', ' ', 't', 'e', 's', 't', '0', '0', '5'};
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, dataBuffer);
    EXPECT_EQ(0, ret);

    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestWait(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->RequestWait(dev_, REQUEST_TIME_OUT, cmdData, dataBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest::UsbRequestWait005 UsbRequestWait=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbRequestWait006
 * @tc.desc: Test functions of UsbRequestWait(USBDevicePipe &pip, int64_t timeout, UsbRequest &req)
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbRequestMockTest, UsbRequestWait006, TestSize.Level1)
{
    USBEndpoint point = interface_.GetEndpoints().at(1);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> cmdData = {'w', 'a', 'i', 't', '0', '0', '6'};
    std::vector<uint8_t> dataBuffer = {'w', 'a', 'i', 't', ' ', 't', 'e', 's', 't', '0', '0', '6'};
    EXPECT_CALL(*mockUsbImpl_, RequestQueue(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RequestQueue(dev_, pipe, cmdData, dataBuffer);
    EXPECT_EQ(0, ret);

    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RequestWait(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->RequestWait(dev_, REQUEST_TIME_OUT, cmdData, dataBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbRequestMockTest::UsbRequestWait006 UsbRequestWait=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}
} // namespace USB
} // namespace OHOS
