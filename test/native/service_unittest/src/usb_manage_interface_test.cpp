/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "usb_manage_interface_test.h"

#include <sys/time.h>

#include <iostream>
#include <vector>

#include "delayed_sp_singleton.h"
#include "hilog_wrapper.h"
#include "if_system_ability_manager.h"
#include "system_ability_definition.h"
#include "usb_common_test.h"
#include "usb_srv_client.h"
#include "usb_errors.h"
#include "usb_interface_type.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::USB;
using namespace std;
using namespace OHOS::HDI::Usb::V1_0;
using namespace OHOS::USB::Common;

namespace OHOS {
namespace USB {
namespace ManageInterface {

void UsbManageInterfaceTest::SetUpTestCase(void)
{
    UsbCommonTest::GrantPermissionSysNative();
    std::cout << "please connect device, press enter to continue" << std::endl;
    int32_t c;
    while ((c = getchar()) != '\n' && c != EOF) {
        ;
    }
    USB_HILOGI(MODULE_USB_SERVICE, "Start UsbManageInterfaceTest");
}

void UsbManageInterfaceTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "End UsbManageInterfaceTest");
}

void UsbManageInterfaceTest::SetUp(void) {}

void UsbManageInterfaceTest::TearDown(void) {}

/**
 * @tc.name: ManageGlobalInterface001
 * @tc.desc: Test functions to ManageGlobalInterface(bool disable);
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageGlobalInterface001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageGlobalInterface001 : ManageGlobalInterface");
    auto &client = UsbSrvClient::GetInstance();
    auto ret = client.ManageGlobalInterface(true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageGlobalInterface001 : ManageGlobalInterface");
}

/**
 * @tc.name: ManageGlobalInterface002
 * @tc.desc: Test functions to ManageGlobalInterface(bool disable);
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageGlobalInterface002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageGlobalInterface002 : ManageGlobalInterface");
    UsbCommonTest::GrantPermissionNormalNative();
    auto &client = UsbSrvClient::GetInstance();
    auto ret = client.ManageGlobalInterface(true);
    ASSERT_NE(ret, 0);
    UsbCommonTest::GrantPermissionSysNative();
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageGlobalInterface002 : ManageGlobalInterface");
}

/**
 * @tc.name: ManageGlobalInterface003
 * @tc.desc: Test functions to ManageGlobalInterface(bool disable);
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageGlobalInterface003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageGlobalInterface003 : ManageGlobalInterface");
    auto &client = UsbSrvClient::GetInstance();
    auto ret = client.ManageGlobalInterface(false);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageGlobalInterface003 : ManageGlobalInterface");
}

/**
 * @tc.name: ManageDevice001
 * @tc.desc: Test functions to ManageDevice(int32_t vendorId, int32_t productId, bool disable);
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageDevice001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice001 : ManageDevice");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    int32_t vendorId = 0;
    int32_t productId = 0;
    if (devi.size() > 0) {
        vendorId = devi.at(0).GetVendorId();
        productId = devi.at(0).GetProductId();
    }
    ret = client.ManageDevice(vendorId, productId, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice001 : ManageDevice");
}

/**
 * @tc.name: ManageDevice002
 * @tc.desc: Test functions to ManageDevice(int32_t vendorId, int32_t productId, bool disable);
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageDevice002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageDevice002 : ManageDevice");
    UsbCommonTest::GrantPermissionNormalNative();
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    int32_t vendorId = 0;
    int32_t productId = 0;
    if (devi.size() > 0) {
        vendorId = devi.at(0).GetVendorId();
        productId = devi.at(0).GetProductId();
    }
    ret = client.ManageDevice(vendorId, productId, true);
    ASSERT_NE(ret, 0);
    UsbCommonTest::GrantPermissionSysNative();
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageDevice002 : ManageDevice");
}

/**
 * @tc.name: ManageDevice003
 * @tc.desc: Test functions to ManageDevice(int32_t vendorId, int32_t productId, bool disable);
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageDevice003, TestSize.Level1)
{
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    int32_t vendorId = 0;
    int32_t productId = 0;
    if (devi.size() > 0) {
        vendorId = devi.at(0).GetVendorId();
        productId = devi.at(0).GetProductId();
    }
    ret = client.ManageDevice(vendorId, productId, false);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(UsbManageInterfaceTest, ManageInterfaceType001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType001 : ManageInterfaceType");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 3;
    usbDeviceType.subClass = 1;
    usbDeviceType.protocol = 2;
    usbDeviceType.isDeviceType = 0;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageInterfaceType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType001 : ManageInterfaceType");
}

HWTEST_F(UsbManageInterfaceTest, ManageInterfaceType002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType002 : ManageInterfaceType");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 8;
    usbDeviceType.subClass = 6;
    usbDeviceType.protocol = 80;
    usbDeviceType.isDeviceType = 0;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageInterfaceType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType002 : ManageInterfaceType");
}

HWTEST_F(UsbManageInterfaceTest, ManageInterfaceType003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType003 : ManageInterfaceType");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 3;
    usbDeviceType.subClass = 1;
    usbDeviceType.protocol = 2;
    usbDeviceType.isDeviceType = 0;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageInterfaceType(disableType, false);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType003 : ManageInterfaceType");
}

HWTEST_F(UsbManageInterfaceTest, ManageInterfaceType004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType004 : ManageInterfaceType");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 8;
    usbDeviceType.subClass = 6;
    usbDeviceType.protocol = 80;
    usbDeviceType.isDeviceType = 0;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageInterfaceType(disableType, false);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType004 : ManageInterfaceType");
}


HWTEST_F(UsbManageInterfaceTest, ManageInterfaceType005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageInterfaceType005 : ManageDevice");
    UsbCommonTest::GrantPermissionNormalNative();
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 8;
    usbDeviceType.subClass = 6;
    usbDeviceType.protocol = 80;
    usbDeviceType.isDeviceType = 0;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageInterfaceType(disableType, false);
    ASSERT_NE(ret, 0);
    UsbCommonTest::GrantPermissionSysNative();
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageInterfaceType005 : ManageDevice");
}

/**
 * @tc.name: ManageUsbType001
 * @tc.desc: Test functions to ManageUsbType – disable HID interface type
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType001");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 3;
    usbDeviceType.subClass = 1;
    usbDeviceType.protocol = 2;
    usbDeviceType.isDeviceType = false;
    usbDeviceType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType001");
}

/**
 * @tc.name: ManageUsbType002
 * @tc.desc: Test functions to ManageUsbType – disable Storage interface type
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType002");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 8;
    usbDeviceType.subClass = 6;
    usbDeviceType.protocol = 80;
    usbDeviceType.isDeviceType = false;
    usbDeviceType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType002");
}

/**
 * @tc.name: ManageUsbType003
 * @tc.desc: Test functions to ManageUsbType – enable HID interface type
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType003");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 3;
    usbDeviceType.subClass = 1;
    usbDeviceType.protocol = 2;
    usbDeviceType.isDeviceType = false;
    usbDeviceType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, false);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType003");
}

/**
 * @tc.name: ManageUsbType004
 * @tc.desc: Test functions to ManageUsbType – enable Storage interface type
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType004");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 8;
    usbDeviceType.subClass = 6;
    usbDeviceType.protocol = 80;
    usbDeviceType.isDeviceType = false;
    usbDeviceType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, false);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType004");
}

/**
 * @tc.name: ManageUsbType005
 * @tc.desc: Test functions to ManageUsbType – permission denied for normal app
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType005, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType005");
    UsbCommonTest::GrantPermissionNormalNative();
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 8;
    usbDeviceType.subClass = 6;
    usbDeviceType.protocol = 80;
    usbDeviceType.isDeviceType = false;
    usbDeviceType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, true);
    ASSERT_NE(ret, 0);
    UsbCommonTest::GrantPermissionSysNative();
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType005");
}

/**
 * @tc.name: ManageUsbType006
 * @tc.desc: Test functions to ManageUsbType – device level disable with isDeviceType=true
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType006, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType006");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 0;
    usbDeviceType.subClass = 0;
    usbDeviceType.protocol = 0;
    usbDeviceType.isDeviceType = true;
    usbDeviceType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType006");
}

/**
 * @tc.name: ManageUsbType007
 * @tc.desc: Test functions to ManageUsbType – all match mode with isDeviceTypeAllMatch=true
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType007, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType007");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 3;
    usbDeviceType.subClass = -1;
    usbDeviceType.protocol = -1;
    usbDeviceType.isDeviceType = false;
    usbDeviceType.isDeviceTypeAllMatch = true;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType007");
}

/**
 * @tc.name: ManageUsbType008
 * @tc.desc: Test functions to ManageUsbType – wildcard subClass=-1
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType008, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType008");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 3;
    usbDeviceType.subClass = -1;
    usbDeviceType.protocol = 2;
    usbDeviceType.isDeviceType = false;
    usbDeviceType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType008");
}

/**
 * @tc.name: ManageUsbType009
 * @tc.desc: Test functions to ManageUsbType – wildcard protocol=-1
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType009, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType009");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 8;
    usbDeviceType.subClass = 6;
    usbDeviceType.protocol = -1;
    usbDeviceType.isDeviceType = false;
    usbDeviceType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType009");
}

/**
 * @tc.name: ManageUsbType010
 * @tc.desc: Test functions to ManageUsbType – batch disable multiple types
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType010, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType010");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType hidType;
    hidType.baseClass = 3;
    hidType.subClass = 1;
    hidType.protocol = 2;
    hidType.isDeviceType = false;
    hidType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(hidType);
    UsbDeviceType storageType;
    storageType.baseClass = 8;
    storageType.subClass = 6;
    storageType.protocol = 80;
    storageType.isDeviceType = false;
    storageType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(storageType);
    UsbDeviceType audioType;
    audioType.baseClass = 1;
    audioType.subClass = 1;
    audioType.protocol = 0;
    audioType.isDeviceType = false;
    audioType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(audioType);
    ret = client.ManageUsbType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType010");
}

/**
 * @tc.name: ManageUsbType011
 * @tc.desc: Test functions to ManageUsbType – empty vector
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType011, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType011");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    ret = client.ManageUsbType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType011");
}

/**
 * @tc.name: ManageUsbType012
 * @tc.desc: Test functions to ManageUsbType – all zero fields
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType012, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType012");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 0;
    usbDeviceType.subClass = 0;
    usbDeviceType.protocol = 0;
    usbDeviceType.isDeviceType = false;
    usbDeviceType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType012");
}

/**
 * @tc.name: ManageUsbType013
 * @tc.desc: Test functions to ManageUsbType – extreme large values
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType013, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType013");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = INT32_MAX;
    usbDeviceType.subClass = INT32_MAX;
    usbDeviceType.protocol = INT32_MAX;
    usbDeviceType.isDeviceType = false;
    usbDeviceType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType013");
}

/**
 * @tc.name: ManageUsbType014
 * @tc.desc: Test functions to ManageUsbType – wildcard both subClass and protocol
 * @tc.type: FUNC
 */
HWTEST_F(UsbManageInterfaceTest, ManageUsbType014, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ManageUsbType014");
    auto &client = UsbSrvClient::GetInstance();
    vector<UsbDevice> devi;
    auto ret = client.GetDevices(devi);
    ASSERT_EQ(ret, 0);
    vector<UsbDeviceType> disableType;
    UsbDeviceType usbDeviceType;
    usbDeviceType.baseClass = 3;
    usbDeviceType.subClass = -1;
    usbDeviceType.protocol = -1;
    usbDeviceType.isDeviceType = false;
    usbDeviceType.isDeviceTypeAllMatch = false;
    disableType.emplace_back(usbDeviceType);
    ret = client.ManageUsbType(disableType, true);
    ASSERT_EQ(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ManageUsbType014");
}
} // ManagerInterface
} // USB
} // OHOS
