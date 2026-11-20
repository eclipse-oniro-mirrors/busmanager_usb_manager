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

#include "usb_bulkcallback_mock_test.h"

#include <iostream>
#include <sys/time.h>
#include <vector>

#include "ashmem.h"
#include "delayed_sp_singleton.h"
#include "hilog_wrapper.h"
#include "securec.h"
#include "usb_callback_test.h"
#include "usb_common_test.h"
#include "usb_device_pipe.h"
#include "usb_errors.h"
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
constexpr uint32_t ASHMEM_MAX_SIZE = 1024;
constexpr uint32_t MEM_DATA = 1024 * 1024;
sptr<MockUsbImpl> UsbBulkcallbackMockTest::mockUsbImpl_ = nullptr;
sptr<UsbService> UsbBulkcallbackMockTest::usbSrv_ = nullptr;
UsbDev UsbBulkcallbackMockTest::dev_ = {BUS_NUM_OK, DEV_ADDR_OK};
UsbInterface UsbBulkcallbackMockTest::interface_ {};

int32_t UsbBulkcallbackMockTest::InitAshmemOne(sptr<Ashmem> &asmptr, int32_t asmSize, uint8_t flg)
{
    asmptr = Ashmem::CreateAshmem("ttashmem000", asmSize);
    if (asmptr == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "InitAshmemOne CreateAshmem failed");
        return UEC_SERVICE_NO_MEMORY;
    }

    asmptr->MapReadAndWriteAshmem();

    if (flg == 0) {
        uint8_t tData[ASHMEM_MAX_SIZE];
        int32_t offset = 0;
        int32_t tLen = 0;

        int32_t retSafe = memset_s(tData, sizeof(tData), 'Y', ASHMEM_MAX_SIZE);
        if (retSafe != EOK) {
            USB_HILOGE(MODULE_USB_SERVICE, "InitAshmemOne memset_s failed");
            return UEC_SERVICE_NO_MEMORY;
        }
        while (offset < asmSize) {
            tLen = (asmSize - offset) < ASHMEM_MAX_SIZE ? (asmSize - offset) : ASHMEM_MAX_SIZE;
            asmptr->WriteToAshmem(tData, tLen, offset);
            offset += tLen;
        }
    }

    return 0;
}

void UsbBulkcallbackMockTest::SetUpTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest SetUpTestCase");
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

void UsbBulkcallbackMockTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest TearDownTestCase");
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

void UsbBulkcallbackMockTest::SetUp(void) {}

void UsbBulkcallbackMockTest::TearDown(void) {}

/**
 * @tc.name: RegBulkCallback001
 * @tc.desc: Test functions to int32_t RegBulkCallback(const UsbDev &devInfo, const UsbPipe &pipe, const
 * sptr<IRemoteObject> &cb)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, RegBulkCallback001, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::RegBulkCallback001 RegBulkCallback=%{public}d", ret);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: RegBulkCallback002
 * @tc.desc: Test functions to int32_t RegBulkCallback(const UsbDev &devInfo, const UsbPipe &pipe, const
 * sptr<IRemoteObject> &cb)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, RegBulkCallback002, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().at(1);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::RegBulkCallback002 RegBulkCallback=%{public}d", ret);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: RegBulkCallback003
 * @tc.desc: Test functions to int32_t RegBulkCallback(const UsbDev &devInfo, const UsbPipe &pipe, const
 * sptr<IRemoteObject> &cb)
 * @tc.desc: Negative test: parameters exception, busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, RegBulkCallback003, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().at(1);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::RegBulkCallback003 RegBulkCallback=%{public}d", ret);
    EXPECT_NE(ret, 0);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: RegBulkCallback004
 * @tc.desc: Test functions to int32_t RegBulkCallback(const UsbDev &devInfo, const UsbPipe &pipe, const
 * sptr<IRemoteObject> &cb)
 * @tc.desc: Negative test: parameters exception, devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, RegBulkCallback004, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().at(1);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::RegBulkCallback004 RegBulkCallback=%{public}d", ret);
    EXPECT_NE(ret, 0);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}

/**
 * @tc.name: BulkRead001
 * @tc.desc: Test functions to int32_t BulkRead(const UsbDev &devInfo, const UsbPipe &pipe, sptr<Ashmem> &ashmem)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, BulkRead001, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    sptr<Ashmem> ashmem;
    uint8_t flg = 0;
    int32_t asmSize = MEM_DATA;
    ret = UsbBulkcallbackMockTest::UsbBulkcallbackMockTest::InitAshmemOne(ashmem, asmSize, flg);
    EXPECT_CALL(*mockUsbImpl_, BulkRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkRead(dev_, pipe, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkRead001 BulkRead=%{public}d", ret);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: BulkRead002
 * @tc.desc: Test functions to int32_t BulkRead(const UsbDev &devInfo, const UsbPipe &pipe, sptr<Ashmem> &ashmem)
 * @tc.desc: Negative test: parameters exception,point error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, BulkRead002, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().at(1);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    sptr<Ashmem> ashmem;
    uint8_t flg = 0;
    int32_t asmSize = MEM_DATA;
    ret = UsbBulkcallbackMockTest::UsbBulkcallbackMockTest::InitAshmemOne(ashmem, asmSize, flg);
    EXPECT_CALL(*mockUsbImpl_, BulkRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkRead(dev_, pipe, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkRead002 BulkRead=%{public}d", ret);
    EXPECT_NE(ret, 0);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: BulkRead003
 * @tc.desc: Test functions to int32_t BulkRead(const UsbDev &devInfo, const UsbPipe &pipe, sptr<Ashmem> &ashmem)
 * @tc.desc: Negative test: parameters exception,busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, BulkRead003, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    sptr<Ashmem> ashmem;
    uint8_t flg = 0;
    int32_t asmSize = MEM_DATA;
    ret = UsbBulkcallbackMockTest::InitAshmemOne(ashmem, asmSize, flg);
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, BulkRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkRead(dev_, pipe, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkRead003 BulkRead=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: BulkRead004
 * @tc.desc: Test functions to int32_t BulkRead(const UsbDev &devInfo, const UsbPipe &pipe, sptr<Ashmem> &ashmem)
 * @tc.desc: Negative test: parameters exception,devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, BulkRead004, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    sptr<Ashmem> ashmem;
    uint8_t flg = 0;
    int32_t asmSize = MEM_DATA;
    ret = UsbBulkcallbackMockTest::InitAshmemOne(ashmem, asmSize, flg);
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, BulkRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkRead(dev_, pipe, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkRead004 BulkRead=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: BulkWrite001
 * @tc.desc: Test functions to int32_t BulkWrite(const UsbDev &devInfo, const UsbPipe &pipe, sptr<Ashmem> &ashmem)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, BulkWrite001, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().at(1);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    sptr<Ashmem> ashmem;
    uint8_t flg = 0;
    int32_t asmSize = MEM_DATA;
    ret = UsbBulkcallbackMockTest::InitAshmemOne(ashmem, asmSize, flg);
    EXPECT_CALL(*mockUsbImpl_, BulkWrite(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkWrite(dev_, pipe, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkWrite001 BulkWrite=%{public}d", ret);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: BulkWrite002
 * @tc.desc: Test functions to int32_t BulkWrite(const UsbDev &devInfo, const UsbPipe &pipe, sptr<Ashmem> &ashmem)
 * @tc.desc: Negative test: parameters exception,point error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, BulkWrite002, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    sptr<Ashmem> ashmem;
    uint8_t flg = 0;
    int32_t asmSize = MEM_DATA;
    ret = UsbBulkcallbackMockTest::InitAshmemOne(ashmem, asmSize, flg);
    EXPECT_CALL(*mockUsbImpl_, BulkWrite(testing::_, testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkWrite(dev_, pipe, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkWrite002 BulkWrite=%{public}d", ret);
    EXPECT_NE(ret, 0);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: BulkWrite003
 * @tc.desc: Test functions to int32_t BulkWrite(const UsbDev &devInfo, const UsbPipe &pipe, sptr<Ashmem> &ashmem)
 * @tc.desc: Negative test: parameters exception,busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, BulkWrite003, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().at(1);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    sptr<Ashmem> ashmem;
    uint8_t flg = 0;
    int32_t asmSize = MEM_DATA;
    ret = UsbBulkcallbackMockTest::InitAshmemOne(ashmem, asmSize, flg);
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, BulkWrite(testing::_, testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkWrite(dev_, pipe, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkWrite003 BulkWrite=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: BulkWrite004
 * @tc.desc: Test functions to int32_t BulkWrite(const UsbDev &devInfo, const UsbPipe &pipe, sptr<Ashmem> &ashmem)
 * @tc.desc: Negative test: parameters exception,devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, BulkWrite004, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().at(1);
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    sptr<Ashmem> ashmem;
    uint8_t flg = 0;
    int32_t asmSize = MEM_DATA;
    ret = UsbBulkcallbackMockTest::InitAshmemOne(ashmem, asmSize, flg);
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, BulkWrite(testing::_, testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkWrite(dev_, pipe, ashmem);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkWrite004 BulkWrite=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: BulkCancel001
 * @tc.desc: Test functions to int32_t BulkCancel(const UsbDev &devInfo, const UsbPipe &pipe)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, BulkCancel001, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    sptr<Ashmem> ashmem;
    uint8_t flg = 0;
    int32_t asmSize = MEM_DATA;
    ret = UsbBulkcallbackMockTest::InitAshmemOne(ashmem, asmSize, flg);
    EXPECT_CALL(*mockUsbImpl_, BulkRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkRead(dev_, pipe, ashmem);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkCancel001 BulkCancel=%{public}d", ret);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkCancel001 BulkCancel=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: BulkCancel002
 * @tc.desc: Test functions to int32_t BulkCancel(const UsbDev &devInfo, const UsbPipe &pipe)
 * @tc.desc: Negative test: parameters exception,busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, BulkCancel002, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    sptr<Ashmem> ashmem;
    uint8_t flg = 0;
    int32_t asmSize = MEM_DATA;
    ret = UsbBulkcallbackMockTest::InitAshmemOne(ashmem, asmSize, flg);
    EXPECT_CALL(*mockUsbImpl_, BulkRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkRead(dev_, pipe, ashmem);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);

    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkCancel002 BulkCancel=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkCancel002 BulkCancel=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: BulkCancel003
 * @tc.desc: Test functions to int32_t BulkCancel(const UsbDev &devInfo, const UsbPipe &pipe)
 * @tc.desc: Negative test: parameters exception,devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, BulkCancel003, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    sptr<Ashmem> ashmem;
    uint8_t flg = 0;
    int32_t asmSize = MEM_DATA;
    ret = UsbBulkcallbackMockTest::InitAshmemOne(ashmem, asmSize, flg);
    EXPECT_CALL(*mockUsbImpl_, BulkRead(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkRead(dev_, pipe, ashmem);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    EXPECT_EQ(0, ret);

    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkCancel002 BulkCancel=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
    EXPECT_CALL(*mockUsbImpl_, BulkCancel(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->BulkCancel(dev_, pipe);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::BulkCancel003 BulkCancel=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UnRegBulkCallback001
 * @tc.desc: Test functions to int32_t UnRegBulkCallback(const UsbDev &devInfo, const UsbPipe &pipe)
 * @tc.desc: Positive test: parameters correctly
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, UnRegBulkCallback001, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_EQ(0, ret);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(0));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::UnRegBulkCallback001 UnRegBulkCallback=%{public}d", ret);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name: UnRegBulkCallback002
 * @tc.desc: Test functions to int32_t UnRegBulkCallback(const UsbDev &devInfo, const UsbPipe &pipe)
 * @tc.desc: Negative test: parameters exception,busNum error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, UnRegBulkCallback002, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    dev_.busNum = BUS_NUM_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_NE(ret, 0);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::UnRegBulkCallback002 UnRegBulkCallback=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.busNum = BUS_NUM_OK;
}

/**
 * @tc.name: UnRegBulkCallback003
 * @tc.desc: Test functions to int32_t UnRegBulkCallback(const UsbDev &devInfo, const UsbPipe &pipe)
 * @tc.desc: Negative test: parameters exception,devAddr error
 * @tc.type: FUNC
 */
HWTEST_F(UsbBulkcallbackMockTest, UnRegBulkCallback003, TestSize.Level1)
{
    sptr<UsbCallbackTest> cb = new UsbCallbackTest();
    EXPECT_NE(cb, nullptr);
    USBEndpoint point = interface_.GetEndpoints().front();
    UsbPipe pipe = {point.GetInterfaceId(), point.GetAddress()};
    dev_.devAddr = DEV_ADDR_INVALID;
    EXPECT_CALL(*mockUsbImpl_, RegBulkCallback(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(RETVAL_INVALID));
    auto ret = usbSrv_->RegBulkCallback(dev_, pipe, cb);
    EXPECT_NE(ret, 0);

    EXPECT_CALL(*mockUsbImpl_, UnRegBulkCallback(testing::_, testing::_)).WillRepeatedly(Return(RETVAL_INVALID));
    ret = usbSrv_->UnRegBulkCallback(dev_, pipe);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbBulkcallbackMockTest::UnRegBulkCallback003 UnRegBulkCallback=%{public}d", ret);
    EXPECT_NE(ret, 0);

    dev_.devAddr = DEV_ADDR_OK;
}
} // namespace USB
} // namespace OHOS
