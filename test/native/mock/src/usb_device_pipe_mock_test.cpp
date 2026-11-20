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

#include "usb_device_pipe_mock_test.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#include "delayed_sp_singleton.h"
#include "file_ex.h"
#include "hilog_wrapper.h"
#include "iusb_srv.h"
#include "securec.h"
#include "usb_common.h"
#include "usb_common_test.h"
#include "usb_descriptor_parser.h"
#include "usb_errors.h"
#include "usb_port_manager.h"
#include "usb_service.h"
#include "usb_srv_client.h"
#include "usbd_bulkcallback_impl.h"

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
constexpr int32_t CTL_VALUE = 0x100;
constexpr int32_t TRANSFER_TIME_OUT = 500;
constexpr int32_t TRANSFER_TIME = -5;
constexpr int32_t TRANSFER_LEN_LONG = 16;
constexpr int32_t TRANSFER_LEN_SHORT = 8;
constexpr int32_t USB_REQUEST_TARGET_INTERFACE = 1;
constexpr int32_t USB_REQUEST_TARGET_ENDPOINT = 2;
constexpr uint8_t USB_SERVICE_CDC_REQ_SET_LINE_CODING = 0x20;
constexpr uint8_t USB_SERVICE_CDC_REQ_SET_CONTROL_LINE_STATE = 0x22;
constexpr uint8_t USB_SERVICE_REQ_GET_CONFIGURATION = 0x08;
constexpr uint8_t USB_SERVICE_REQ_GET_DESCRIPTOR = 0x06;
constexpr uint8_t USB_SERVICE_REQ_GET_INTERFACE = 0x0A;
constexpr uint8_t USB_SERVICE_REQ_SYNCH_FRAME = 0x0C;
constexpr uint8_t USB_SERVICE_REQ_GET_STATUS = 0x00;
constexpr uint8_t USB_SERVICE_TYPE_CLASS = 0x20;

sptr<MockUsbImpl> UsbDevicePipeMockTest::mockUsbImpl_ = nullptr;
sptr<UsbService> UsbDevicePipeMockTest::usbSrv_ = nullptr;
UsbDev UsbDevicePipeMockTest::dev_ = {BUS_NUM_OK, DEV_ADDR_OK};
UsbDevice UsbDevicePipeMockTest::device_ {};

void UsbDevicePipeMockTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest SetUpTestCase");
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
    device_ = MockUsbImpl::FindDeviceInfo(devList);

    EXPECT_CALL(*mockUsbImpl_, OpenDevice(testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->OpenDevice(dev_.busNum, dev_.devAddr);
    EXPECT_EQ(0, ret);
}

void UsbDevicePipeMockTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest TearDownTestCase");
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

void UsbDevicePipeMockTest::SetUp(void) {}

void UsbDevicePipeMockTest::TearDown(void) {}
/**
 * @tc.name: getDevices001
 * @tc.desc: Test functions to getDevices(std::vector<UsbDevice> &deviceList);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, getDevices001, TestSize.Level1)
{
    vector<UsbDevice> devList;
    auto ret = usbSrv_->GetDevices(devList);
    EXPECT_EQ(0, ret);
    EXPECT_FALSE(devList.empty()) << "devList NULL";
    UsbDevice device = MockUsbImpl::FindDeviceInfo(devList);
}
/**
 * @tc.name: UsbOpenDevice001
 * @tc.desc: Test functions of OpenDevice
 * @tc.desc: Positive：code run normal,return 0
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbOpenDevice001, TestSize.Level1)
{
    EXPECT_CALL(*mockUsbImpl_, CloseDevice(testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->Close(dev_.busNum, dev_.devAddr);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, OpenDevice(testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->OpenDevice(dev_.busNum, dev_.devAddr);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbControlTransferRead001
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead001, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_IN, USB_SERVICE_REQ_GET_CONFIGURATION, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(TRANSFER_LEN_SHORT);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead001 ControlTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbControlTransferRead002
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead002, TestSize.Level1)
{
    dev_.busNum = BUS_NUM_INVALID;
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_IN, USB_SERVICE_REQ_GET_CONFIGURATION, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(TRANSFER_LEN_SHORT);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead002 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbControlTransferRead003
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead003, TestSize.Level1)
{
    dev_.devAddr = DEV_ADDR_INVALID;
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_IN, USB_SERVICE_REQ_GET_CONFIGURATION, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(TRANSFER_LEN_SHORT);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead003 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbControlTransferRead004
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead004, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN, USB_SERVICE_REQ_GET_DESCRIPTOR, CTL_VALUE, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(TRANSFER_LEN_SHORT);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead004 ControlTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbControlTransferRead005
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead005, TestSize.Level1)
{
    dev_.busNum = BUS_NUM_INVALID;
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN, USB_SERVICE_REQ_GET_DESCRIPTOR, CTL_VALUE, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(TRANSFER_LEN_SHORT);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead005 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbControlTransferRead006
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead006, TestSize.Level1)
{
    dev_.devAddr = DEV_ADDR_INVALID;
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN, USB_SERVICE_REQ_GET_DESCRIPTOR, CTL_VALUE, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(TRANSFER_LEN_SHORT);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead006 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbControlTransferRead007
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead007, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_INTERFACE, USB_SERVICE_REQ_GET_INTERFACE, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead007 ControlTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbControlTransferRead008
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead008, TestSize.Level1)
{
    dev_.busNum = BUS_NUM_INVALID;
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_INTERFACE, USB_SERVICE_REQ_GET_INTERFACE, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead008 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbControlTransferRead009
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead009, TestSize.Level1)
{
    dev_.devAddr = DEV_ADDR_INVALID;
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_INTERFACE, USB_SERVICE_REQ_GET_INTERFACE, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead009 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbControlTransferRead010
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead010, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_IN, USB_SERVICE_REQ_GET_STATUS, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead010 ControlTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbControlTransferRead011
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead011, TestSize.Level1)
{
    dev_.busNum = BUS_NUM_INVALID;
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_IN, USB_SERVICE_REQ_GET_STATUS, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead011 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbControlTransferRead012
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead012, TestSize.Level1)
{
    dev_.devAddr = DEV_ADDR_INVALID;
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_IN, USB_SERVICE_REQ_GET_STATUS, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead012 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbControlTransferRead0013
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead0013, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_INTERFACE, USB_SERVICE_REQ_GET_STATUS, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead0013 ControlTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbControlTransferRead0014
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead0014, TestSize.Level1)
{
    dev_.busNum = BUS_NUM_INVALID;
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_INTERFACE, USB_SERVICE_REQ_GET_STATUS, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead0014 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbControlTransferRead0015
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead0015, TestSize.Level1)
{
    dev_.devAddr = DEV_ADDR_INVALID;
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_INTERFACE, USB_SERVICE_REQ_GET_STATUS, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead0015 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbControlTransferRead016
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead016, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_ENDPOINT, USB_SERVICE_REQ_GET_STATUS, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(TRANSFER_LEN_LONG);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead016 ControlTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbControlTransferRead017
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead017, TestSize.Level1)
{
    dev_.busNum = BUS_NUM_INVALID;
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_ENDPOINT, USB_SERVICE_REQ_GET_STATUS, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(TRANSFER_LEN_LONG);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead017 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbControlTransferRead018
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead018, TestSize.Level1)
{
    dev_.devAddr = DEV_ADDR_INVALID;
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_ENDPOINT, USB_SERVICE_REQ_GET_STATUS, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(TRANSFER_LEN_LONG);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead018 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbControlTransferRead019
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead019, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_ENDPOINT, USB_SERVICE_REQ_SYNCH_FRAME, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead019 ControlTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbControlTransferRead020
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead020, TestSize.Level1)
{
    dev_.busNum = BUS_NUM_INVALID;
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_ENDPOINT, USB_SERVICE_REQ_SYNCH_FRAME, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead020 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbControlTransferRead021
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferRead021, TestSize.Level1)
{
    dev_.devAddr = DEV_ADDR_INVALID;
    struct UsbCtrlTransfer ctrlData = {
        USB_ENDPOINT_DIR_IN | USB_REQUEST_TARGET_ENDPOINT, USB_SERVICE_REQ_SYNCH_FRAME, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferRead021 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbControlTransferWrite001
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferWrite001, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_OUT | USB_SERVICE_TYPE_CLASS | USB_REQUEST_TARGET_INTERFACE,
        USB_SERVICE_CDC_REQ_SET_LINE_CODING, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferWrite(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferWrite001 ControlTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbControlTransferWrite002
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferWrite002, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_OUT | USB_SERVICE_TYPE_CLASS | USB_REQUEST_TARGET_INTERFACE,
        USB_SERVICE_CDC_REQ_SET_LINE_CODING, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, ControlTransferWrite(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferWrite002 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbControlTransferWrite003
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferWrite003, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_OUT | USB_SERVICE_TYPE_CLASS | USB_REQUEST_TARGET_INTERFACE,
        USB_SERVICE_CDC_REQ_SET_LINE_CODING, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferWrite(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    dev_.devAddr = DEV_ADDR_INVALID;
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferWrite003 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbControlTransferWrite004
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferWrite004, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_OUT | USB_SERVICE_TYPE_CLASS | USB_REQUEST_TARGET_INTERFACE,
        USB_SERVICE_CDC_REQ_SET_CONTROL_LINE_STATE, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferWrite(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferWrite002 ControlTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbControlTransferWrite005
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferWrite005, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_OUT | USB_SERVICE_TYPE_CLASS | USB_REQUEST_TARGET_INTERFACE,
        USB_SERVICE_CDC_REQ_SET_CONTROL_LINE_STATE, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    EXPECT_CALL(*mockUsbImpl_, ControlTransferWrite(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    dev_.busNum = BUS_NUM_INVALID;
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferWrite006 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbControlTransferWrite006
 * @tc.desc: Test functions to ControlTransfer
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbControlTransferWrite006, TestSize.Level1)
{
    struct UsbCtrlTransfer ctrlData = {USB_ENDPOINT_DIR_OUT | USB_SERVICE_TYPE_CLASS | USB_REQUEST_TARGET_INTERFACE,
        USB_SERVICE_CDC_REQ_SET_CONTROL_LINE_STATE, 0, 0, TRANSFER_TIME_OUT};
    std::vector<uint8_t> ctrlBuffer(BUFFER_SIZE);
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, ControlTransferWrite(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ControlTransfer(dev_, ctrlData, ctrlBuffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbControlTransferWrite006 ControlTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbClaimInterface001
 * @tc.desc: Test functions to ClaimInterface(const UsbInterface &interface, bool force);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbClaimInterface001, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().front();
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbClaimInterface001 ClaimInterface=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbClaimInterface002
 * @tc.desc: Test functions to ClaimInterface(const UsbInterface &interface, bool force);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbClaimInterface002, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().front();
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbClaimInterface002 ClaimInterface=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbClaimInterface003
 * @tc.desc: Test functions to ClaimInterface(const UsbInterface &interface, bool force);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbClaimInterface003, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbClaimInterface003 ClaimInterface=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbClaimInterface004
 * @tc.desc: Test functions to ClaimInterface(const UsbInterface &interface, bool force);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbClaimInterface004, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, false);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbClaimInterface004 ClaimInterface=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbClaimInterface005
 * @tc.desc: Test functions to ClaimInterface(const UsbInterface &interface, bool force);
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbClaimInterface005, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().front();
    uint8_t interfaceId = interface.GetId();
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbClaimInterface005 ClaimInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbClaimInterface006
 * @tc.desc: Test functions to ClaimInterface(const UsbInterface &interface, bool force);
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbClaimInterface006, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().front();
    uint8_t interfaceId = interface.GetId();
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbClaimInterface006 ClaimInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);
    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbClaimInterface007
 * @tc.desc: Test functions to ClaimInterface(const UsbInterface &interface, bool force);
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbClaimInterface007, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbClaimInterface007 ClaimInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbClaimInterface008
 * @tc.desc: Test functions to ClaimInterface(const UsbInterface &interface, bool force);
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbClaimInterface008, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbClaimInterface008 ClaimInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbReleaseInterface001
 * @tc.desc: Test functions to ReleaseInterface(const UsbInterface &interface);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbReleaseInterface001, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(0);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ReleaseInterface(testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ReleaseInterface(dev_.busNum, dev_.devAddr, interfaceId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbReleaseInterface001 ReleaseInterface=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbReleaseInterface002
 * @tc.desc: Test functions to ReleaseInterface(const UsbInterface &interface);
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbReleaseInterface002, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(0);
    uint8_t interfaceId = interface.GetId();
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, ReleaseInterface(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ReleaseInterface(dev_.busNum, dev_.devAddr, interfaceId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbReleaseInterface002 ReleaseInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbReleaseInterface003
 * @tc.desc: Test functions to ReleaseInterface(const UsbInterface &interface);
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbReleaseInterface003, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(0);
    uint8_t interfaceId = interface.GetId();
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, ReleaseInterface(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ReleaseInterface(dev_.busNum, dev_.devAddr, interfaceId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbReleaseInterface003 ReleaseInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbReleaseInterface004
 * @tc.desc: Test functions to ReleaseInterface(const UsbInterface &interface);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbReleaseInterface004, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, ReleaseInterface(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->ReleaseInterface(dev_.busNum, dev_.devAddr, interfaceId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbReleaseInterface004 ReleaseInterface=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbReleaseInterface005
 * @tc.desc: Test functions to ReleaseInterface(const UsbInterface &interface);
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbReleaseInterface005, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, ReleaseInterface(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ReleaseInterface(dev_.busNum, dev_.devAddr, interfaceId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbReleaseInterface005 ReleaseInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbReleaseInterface006
 * @tc.desc: Test functions to ReleaseInterface(const UsbInterface &interface);
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbReleaseInterface006, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, ReleaseInterface(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->ReleaseInterface(dev_.busNum, dev_.devAddr, interfaceId);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbReleaseInterface006 ReleaseInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbBulkTransfer001
 * @tc.desc: Test functions to BulkTransfer(const USBEndpoint &endpoint, uint8_t *buffer, uint32_t &length, int32_t
 * timeout);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbBulkTransfer001, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    USBEndpoint point = interface.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> bulkBuffer = {'b', 'u', 'l', 'k', ' ', 'r', 'e', 'a', 'd', '0', '0', '1'};
    EXPECT_CALL(*mockUsbImpl_, BulkTransferRead(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(0));
    ret = usbSrv_->BulkTransferRead(dev_, pipe, bulkBuffer, TRANSFER_TIME_OUT);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbBulkTransfer001 BulkTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbBulkTransfer002
 * @tc.desc: Test functions to BulkTransfer(const USBEndpoint &endpoint, uint8_t *buffer, uint32_t &length, int32_t
 * timeout);
 * @tc.desc: @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbBulkTransfer002, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();

    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    std::vector<uint8_t> bulkBuffer = {'b', 'u', 'l', 'k', ' ', 'r', 'e', 'a', 'd', '0', '0', '2'};
    USBEndpoint point = interface.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, BulkTransferRead(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(0));
    ret = usbSrv_->BulkTransferRead(dev_, pipe, bulkBuffer, TRANSFER_TIME);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbBulkTransfer002 BulkTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbBulkTransfer003
 * @tc.desc: Test functions to BulkTransfer(const USBEndpoint &endpoint, uint8_t *buffer, uint32_t &length, int32_t
 * timeout);
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbBulkTransfer003, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    dev_.busNum = BUS_NUM_INVALID;
    USBEndpoint point = interface.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> bulkBuffer = {'b', 'u', 'l', 'k', ' ', 'r', 'e', 'a', 'd', '0', '0', '3'};
    EXPECT_CALL(*mockUsbImpl_, BulkTransferRead(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkTransferRead(dev_, pipe, bulkBuffer, TRANSFER_TIME_OUT);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbBulkTransfer003 BulkTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbBulkTransfer004
 * @tc.desc: Test functions to BulkTransfer(const USBEndpoint &endpoint, uint8_t *buffer, uint32_t &length, int32_t
 * timeout);
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbBulkTransfer004, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    dev_.devAddr = DEV_ADDR_INVALID;
    USBEndpoint point = interface.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> bulkBuffer = {'b', 'u', 'l', 'k', ' ', 'r', 'e', 'a', 'd', '0', '0', '4'};
    EXPECT_CALL(*mockUsbImpl_, BulkTransferRead(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkTransferRead(dev_, pipe, bulkBuffer, TRANSFER_TIME_OUT);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbBulkTransfer004 BulkTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbBulkTransfer005
 * @tc.desc: Test functions to BulkTransfer(const USBEndpoint &endpoint, uint8_t *buffer, uint32_t &length, int32_t
 * timeout);
 * @tc.desc: Negative test: parameters exception, point error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbBulkTransfer005, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    USBEndpoint point = interface.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    point.SetInterfaceId(BUFFER_SIZE);
    std::vector<uint8_t> bulkBuffer = {'b', 'u', 'l', 'k', ' ', 'r', 'e', 'a', 'd', '0', '0', '5'};
    EXPECT_CALL(*mockUsbImpl_, BulkTransferRead(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkTransferRead(dev_, pipe, bulkBuffer, TRANSFER_TIME_OUT);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbBulkTransfer005 BulkTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: UsbBulkTransfer006
 * @tc.desc: Test functions to BulkTransfer(const USBEndpoint &endpoint, uint8_t *buffer, uint32_t &length, int32_t
 * timeout);
 * @tc.desc: Negative test: parameters exception, point SetAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbBulkTransfer006, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    USBEndpoint point = interface.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    point.SetAddr(BUFFER_SIZE);
    std::vector<uint8_t> bulkBuffer = {'b', 'u', 'l', 'k', ' ', 'r', 'e', 'a', 'd', '0', '0', '6'};
    EXPECT_CALL(*mockUsbImpl_, BulkTransferRead(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkTransferRead(dev_, pipe, bulkBuffer, TRANSFER_TIME_OUT);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbBulkTransfer006 BulkTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: UsbBulkTransfer007
 * @tc.desc: Test functions to BulkTransfer(const USBEndpoint &endpoint, uint8_t *buffer, uint32_t &length, int32_t
 * timeout);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbBulkTransfer007, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    USBEndpoint point = interface.GetEndpoints().at(1);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> bulkBuffer = {'b', 'u', 'l', 'k', ' ', 'r', 'e', 'a', 'd', '0', '0', '7'};
    EXPECT_CALL(*mockUsbImpl_, BulkTransferWrite(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(0));
    ret = usbSrv_->BulkTransferWrite(dev_, pipe, bulkBuffer, TRANSFER_TIME_OUT);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbBulkTransfer007 BulkTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UsbBulkTransfer008
 * @tc.desc: Test functions to BulkTransfer(const USBEndpoint &endpoint, uint8_t *buffer, uint32_t &length, int32_t
 * timeout);
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbBulkTransfer008, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    dev_.devAddr = DEV_ADDR_INVALID;
    USBEndpoint point = interface.GetEndpoints().at(1);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> bulkBuffer = {'b', 'u', 'l', 'k', ' ', 'r', 'e', 'a', 'd', '0', '0', '8'};
    EXPECT_CALL(*mockUsbImpl_, BulkTransferWrite(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkTransferWrite(dev_, pipe, bulkBuffer, TRANSFER_TIME_OUT);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbBulkTransfer008 BulkTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: UsbBulkTransfer009
 * @tc.desc: Test functions to BulkTransfer(const USBEndpoint &endpoint, uint8_t *buffer, uint32_t &length, int32_t
 * timeout);
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbBulkTransfer009, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    dev_.busNum = BUS_NUM_INVALID;
    USBEndpoint point = interface.GetEndpoints().at(1);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> bulkBuffer = {'b', 'u', 'l', 'k', ' ', 'r', 'e', 'a', 'd', '0', '0', '9'};
    EXPECT_CALL(*mockUsbImpl_, BulkTransferWrite(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkTransferWrite(dev_, pipe, bulkBuffer, TRANSFER_TIME_OUT);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbBulkTransfer009 BulkTransfer=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UsbBulkTransfer010
 * @tc.desc: Test functions to BulkTransfer(const USBEndpoint &endpoint, uint8_t *buffer, uint32_t &length, int32_t
 * timeout);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, UsbBulkTransfer010, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    USBEndpoint point = interface.GetEndpoints().at(1);
    const UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    std::vector<uint8_t> bulkBuffer = {'b', 'u', 'l', 'k', ' ', 'r', 'e', 'a', 'd', '0', '1', '0'};
    EXPECT_CALL(*mockUsbImpl_, BulkTransferWrite(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(0));
    ret = usbSrv_->BulkTransferWrite(dev_, pipe, bulkBuffer, TRANSFER_TIME);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::UsbBulkTransfer010 BulkTransfer=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: SetConfiguration001
 * @tc.desc: Test functions to  SetConfiguration(const USBConfig &config);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, SetConfiguration001, TestSize.Level1)
{
    USBConfig config = device_.GetConfigs().front();
    uint8_t configIndex = config.GetiConfiguration();
    EXPECT_CALL(*mockUsbImpl_, SetConfig(testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->SetActiveConfig(dev_.busNum, dev_.devAddr, configIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::SetConfiguration001 SetConfiguration=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: SetConfiguration002
 * @tc.desc: Test functions to  SetConfiguration(const USBConfig &config);
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, SetConfiguration002, TestSize.Level1)
{
    USBConfig config = device_.GetConfigs().front();
    uint8_t configIndex = config.GetiConfiguration();
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, SetConfig(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->SetActiveConfig(dev_.busNum, dev_.devAddr, configIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::SetConfiguration002 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: SetConfiguration003
 * @tc.desc: Test functions to  SetConfiguration(const USBConfig &config);
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, SetConfiguration003, TestSize.Level1)
{
    USBConfig config = device_.GetConfigs().front();
    uint8_t configIndex = config.GetiConfiguration();
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, SetConfig(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->SetActiveConfig(dev_.busNum, dev_.devAddr, configIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::SetConfiguration003 SetConfiguration=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: Close001
 * @tc.desc: Test functions to  Close();
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, Close001, TestSize.Level1)
{
    EXPECT_CALL(*mockUsbImpl_, CloseDevice(testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->Close(dev_.busNum, dev_.devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::Close001 close=%{public}d", ret);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, OpenDevice(testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->OpenDevice(dev_.busNum, dev_.devAddr);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: Close002
 * @tc.desc: Test functions to  Close();
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, Close002, TestSize.Level1)
{
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, CloseDevice(testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->Close(dev_.busNum, dev_.devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::Close002 close=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
    EXPECT_CALL(*mockUsbImpl_, OpenDevice(testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->OpenDevice(dev_.busNum, dev_.devAddr);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: Close003
 * @tc.desc: Test functions to  Close();
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, Close003, TestSize.Level1)
{
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, CloseDevice(testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->Close(dev_.busNum, dev_.devAddr);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::Close003 ret=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
    EXPECT_CALL(*mockUsbImpl_, OpenDevice(testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->OpenDevice(dev_.busNum, dev_.devAddr);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: SetInterface001
 * @tc.desc: Test functions to SetInterface(const UsbInterface &interface);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, SetInterface001, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(0);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    uint8_t altIndex = interface.GetAlternateSetting();
    EXPECT_CALL(*mockUsbImpl_, SetInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->SetInterface(dev_.busNum, dev_.devAddr, interfaceId, altIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::SetInterface001 SetInterface=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: SetInterface002
 * @tc.desc: Test functions to SetInterface(const UsbInterface &interface);
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, SetInterface002, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(0);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    dev_.busNum = BUS_NUM_INVALID;
    uint8_t altIndex = interface.GetAlternateSetting();
    EXPECT_CALL(*mockUsbImpl_, SetInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->SetInterface(dev_.busNum, dev_.devAddr, interfaceId, altIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::SetInterface002 SetInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: SetInterface003
 * @tc.desc: Test functions to SetInterface(const UsbInterface &interface);
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, SetInterface003, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().front().GetInterfaces().at(0);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    dev_.devAddr = DEV_ADDR_INVALID;
    uint8_t altIndex = interface.GetAlternateSetting();
    EXPECT_CALL(*mockUsbImpl_, SetInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->SetInterface(dev_.busNum, dev_.devAddr, interfaceId, altIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::SetInterface003 SetInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: SetInterface004
 * @tc.desc: Test functions to  SetInterface(const UsbInterface &interface);
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, SetInterface004, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().at(0).GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    uint8_t altIndex = interface.GetAlternateSetting();
    EXPECT_CALL(*mockUsbImpl_, SetInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->SetInterface(dev_.busNum, dev_.devAddr, interfaceId, altIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::SetInterface004 SetInterface=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: SetInterface005
 * @tc.desc: Test functions to  SetInterface(const UsbInterface &interface);
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, SetInterface005, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().at(0).GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();

    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    dev_.busNum = BUS_NUM_INVALID;
    uint8_t altIndex = interface.GetAlternateSetting();
    EXPECT_CALL(*mockUsbImpl_, SetInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->SetInterface(dev_.busNum, dev_.devAddr, interfaceId, altIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::SetInterface005 SetInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: SetInterface006
 * @tc.desc: Test functions to  SetInterface(const UsbInterface &interface);
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, SetInterface006, TestSize.Level1)
{
    UsbInterface interface = device_.GetConfigs().at(0).GetInterfaces().at(1);
    uint8_t interfaceId = interface.GetId();
    EXPECT_CALL(*mockUsbImpl_, ClaimInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->ClaimInterface(dev_.busNum, dev_.devAddr, interfaceId, true);
    EXPECT_EQ(0, ret);

    dev_.devAddr = DEV_ADDR_INVALID;
    uint8_t altIndex = interface.GetAlternateSetting();
    EXPECT_CALL(*mockUsbImpl_, SetInterface(testing::_, testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->SetInterface(dev_.busNum, dev_.devAddr, interfaceId, altIndex);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::SetInterface006 SetInterface=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: GetRawDescriptors001
 * @tc.desc: Test functions to GetRawDescriptors
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, GetRawDescriptors001, TestSize.Level1)
{
    std::vector<uint8_t> buffer;
    auto ret = usbSrv_->GetRawDescriptor(dev_.busNum, dev_.devAddr, buffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::GetRawDescriptors001 GetRawDescriptors=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: GetRawDescriptors002
 * @tc.desc: Test functions to GetRawDescriptors
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, GetRawDescriptors002, TestSize.Level1)
{
    std::vector<uint8_t> buffer;
    dev_.busNum = BUS_NUM_INVALID;
    auto ret = usbSrv_->GetRawDescriptor(dev_.busNum, dev_.devAddr, buffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::GetRawDescriptors002 GetRawDescriptors=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: GetRawDescriptors003
 * @tc.desc: Test functions to GetRawDescriptors
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, GetRawDescriptors003, TestSize.Level1)
{
    std::vector<uint8_t> buffer;
    dev_.devAddr = DEV_ADDR_INVALID;
    auto ret = usbSrv_->GetRawDescriptor(dev_.busNum, dev_.devAddr, buffer);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::GetRawDescriptors003 GetRawDescriptors=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: GetFileDescriptors001
 * @tc.desc: Test functions to GetRawDescriptors
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, GetFileDescriptors001, TestSize.Level1)
{
    std::vector<uint8_t> buffer;
    auto ret = usbSrv_->GetRawDescriptor(dev_.busNum, dev_.devAddr, buffer);
    EXPECT_EQ(0, ret);

    int32_t fd = 0;
    EXPECT_CALL(*mockUsbImpl_, GetFileDescriptor(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->GetFileDescriptor(dev_.busNum, dev_.devAddr, fd);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::GetFileDescriptors001 GetFileDescriptor=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: GetFileDescriptors002
 * @tc.desc: Test functions to GetRawDescriptors
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, GetFileDescriptors002, TestSize.Level1)
{
    std::vector<uint8_t> buffer;
    auto ret = usbSrv_->GetRawDescriptor(dev_.busNum, dev_.devAddr, buffer);
    EXPECT_EQ(0, ret);

    dev_.busNum = BUS_NUM_INVALID;
    int32_t fd = 0;
    EXPECT_CALL(*mockUsbImpl_, GetFileDescriptor(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->GetFileDescriptor(dev_.busNum, dev_.devAddr, fd);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::GetFileDescriptors002 GetFileDescriptor=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: GetFileDescriptors003
 * @tc.desc: Test functions to GetRawDescriptors
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbDevicePipeMockTest, GetFileDescriptors003, TestSize.Level1)
{
    std::vector<uint8_t> buffer;
    auto ret = usbSrv_->GetRawDescriptor(dev_.busNum, dev_.devAddr, buffer);
    EXPECT_EQ(0, ret);

    dev_.devAddr = DEV_ADDR_INVALID;
    int32_t fd = 0;
    EXPECT_CALL(*mockUsbImpl_, GetFileDescriptor(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->GetFileDescriptor(dev_.busNum, dev_.devAddr, fd);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDevicePipeMockTest::GetFileDescriptors003 GetFileDescriptor=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}
} // namespace USB
} // namespace OHOS
