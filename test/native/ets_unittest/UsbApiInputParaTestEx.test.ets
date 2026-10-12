/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import usbMgr from '@ohos.usbManager';
import { BusinessError } from '@ohos.base';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect, TestType, Size, Level } from '@ohos/hypium';

const TAG = "[UsbApiInputParaTestEx]";

let USBAccessoryFeature: usbMgr.USBAccessory = {
  manufacturer: 'testManufacturer',
  product: 'testProduct',
  description: 'testDescription',
  version: '1.0',
  serialNumber: 'testSerial',
};

export default function UsbApiInputParaTestEx() {
  describe('UsbApiInputParaTestEx', () => {

    beforeAll(async () => {
      console.log(TAG, '*************Usb Unit UsbApiInputParaTestEx Begin*************');
    })

    beforeEach(() => {
      console.info(TAG, 'beforeEach: *************Usb Unit Test Case Ex*************');
    })

    afterEach(() => {
      console.info(TAG, 'afterEach: *************Usb Unit Test Case Ex*************');
    })

    afterAll(() => {
      console.log(TAG, '*************Usb Unit UsbApiInputParaTestEx End*************');
    })

    /**
     * @tc.name   testRemoveRightNoString
     * @tc.number SUB_USB_HostManager_InputParaEx_0100
     * @tc.desc   removeRight input parameter ""
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRemoveRightNoString', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testRemoveRightNoString begin');
      try {
        let ret = usbMgr.removeRight("");
        console.info(TAG, 'usb testRemoveRightNoString ret: ' + ret);
        expect(ret).assertFalse();
        done();
      } catch (err) {
        console.info(TAG, 'usb testRemoveRightNoString fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testRemoveRightNull
     * @tc.number SUB_USB_HostManager_InputParaEx_0200
     * @tc.desc   removeRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRemoveRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testRemoveRightNull begin');
      try {
        let ret = usbMgr.removeRight(null);
        console.info(TAG, 'usb testRemoveRightNull ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testRemoveRightNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testRemoveRightUndefined
     * @tc.number SUB_USB_HostManager_InputParaEx_0300
     * @tc.desc   removeRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRemoveRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testRemoveRightUndefined begin');
      try {
        let ret = usbMgr.removeRight(undefined);
        console.info(TAG, 'usb testRemoveRightUndefined ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testRemoveRightUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testAddDeviceAccessRightNull
     * @tc.number SUB_USB_HostManager_InputParaEx_0400
     * @tc.desc   addDeviceAccessRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAddDeviceAccessRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testAddDeviceAccessRightNull begin');
      try {
        let ret = usbMgr.addDeviceAccessRight(null, null);
        console.info(TAG, 'usb testAddDeviceAccessRightNull ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testAddDeviceAccessRightNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testAddDeviceAccessRightUndefined
     * @tc.number SUB_USB_HostManager_InputParaEx_0500
     * @tc.desc   addDeviceAccessRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAddDeviceAccessRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testAddDeviceAccessRightUndefined begin');
      try {
        let ret = usbMgr.addDeviceAccessRight(undefined, undefined);
        console.info(TAG, 'usb testAddDeviceAccessRightUndefined ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testAddDeviceAccessRightUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testAddDeviceAccessRightEmpty
     * @tc.number SUB_USB_HostManager_InputParaEx_0600
     * @tc.desc   addDeviceAccessRight input parameter ""
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAddDeviceAccessRightEmpty', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testAddDeviceAccessRightEmpty begin');
      try {
        let ret = usbMgr.addDeviceAccessRight("", "");
        console.info(TAG, 'usb testAddDeviceAccessRightEmpty ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testAddDeviceAccessRightEmpty fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testAddAccessoryRightNull
     * @tc.number SUB_USB_HostManager_InputParaEx_0700
     * @tc.desc   addAccessoryRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAddAccessoryRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testAddAccessoryRightNull begin');
      try {
        usbMgr.addAccessoryRight(null, null);
        done();
      } catch (err) {
        console.info(TAG, 'usb testAddAccessoryRightNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testAddAccessoryRightUndefined
     * @tc.number SUB_USB_HostManager_InputParaEx_0800
     * @tc.desc   addAccessoryRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAddAccessoryRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testAddAccessoryRightUndefined begin');
      try {
        usbMgr.addAccessoryRight(undefined, undefined);
        done();
      } catch (err) {
        console.info(TAG, 'usb testAddAccessoryRightUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testGetAccessoryList001
     * @tc.number SUB_USB_HostManager_InputParaEx_0900
     * @tc.desc   getAccessoryList basic call test
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetAccessoryList001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetAccessoryList001 begin');
      try {
        let ret = usbMgr.getAccessoryList();
        console.info(TAG, 'usb testGetAccessoryList001 ret: ' + JSON.stringify(ret));
        expect(Array.isArray(ret)).assertTrue();
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetAccessoryList001 fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(14400001);
        done();
      }
    })

    /**
     * @tc.name   testClaimInterfaceExclusiveNull
     * @tc.number SUB_USB_HostManager_InputParaEx_1000
     * @tc.desc   claimInterfaceExclusive input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testClaimInterfaceExclusiveNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testClaimInterfaceExclusiveNull begin');
      try {
        usbMgr.claimInterfaceExclusive(null, null, false);
        done();
      } catch (err) {
        console.info(TAG, 'usb testClaimInterfaceExclusiveNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testClaimInterfaceExclusiveUndefined
     * @tc.number SUB_USB_HostManager_InputParaEx_1100
     * @tc.desc   claimInterfaceExclusive input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testClaimInterfaceExclusiveUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testClaimInterfaceExclusiveUndefined begin');
      try {
        usbMgr.claimInterfaceExclusive(undefined, undefined, false);
        done();
      } catch (err) {
        console.info(TAG, 'usb testClaimInterfaceExclusiveUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testGetFunctionsFromStringNull
     * @tc.number SUB_USB_HostManager_InputParaEx_1200
     * @tc.desc   getFunctionsFromString input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetFunctionsFromStringNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetFunctionsFromStringNull begin');
      try {
        let ret = usbMgr.getFunctionsFromString(null);
        console.info(TAG, 'usb testGetFunctionsFromStringNull ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetFunctionsFromStringNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testGetFunctionsFromStringUndefined
     * @tc.number SUB_USB_HostManager_InputParaEx_1300
     * @tc.desc   getFunctionsFromString input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetFunctionsFromStringUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetFunctionsFromStringUndefined begin');
      try {
        let ret = usbMgr.getFunctionsFromString(undefined);
        console.info(TAG, 'usb testGetFunctionsFromStringUndefined ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetFunctionsFromStringUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testGetFunctionsFromStringEmpty
     * @tc.number SUB_USB_HostManager_InputParaEx_1400
     * @tc.desc   getFunctionsFromString input parameter ""
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetFunctionsFromStringEmpty', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetFunctionsFromStringEmpty begin');
      try {
        let ret = usbMgr.getFunctionsFromString("");
        console.info(TAG, 'usb testGetFunctionsFromStringEmpty ret: ' + ret);
        expect(ret).assertEqual(0);
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetFunctionsFromStringEmpty fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testGetStringFromFunctionsNull
     * @tc.number SUB_USB_HostManager_InputParaEx_1500
     * @tc.desc   getStringFromFunctions input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetStringFromFunctionsNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetStringFromFunctionsNull begin');
      try {
        let ret = usbMgr.getStringFromFunctions(null);
        console.info(TAG, 'usb testGetStringFromFunctionsNull ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetStringFromFunctionsNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testGetStringFromFunctionsUndefined
     * @tc.number SUB_USB_HostManager_InputParaEx_1600
     * @tc.desc   getStringFromFunctions input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetStringFromFunctionsUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetStringFromFunctionsUndefined begin');
      try {
        let ret = usbMgr.getStringFromFunctions(undefined);
        console.info(TAG, 'usb testGetStringFromFunctionsUndefined ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetStringFromFunctionsUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testGetStringFromFunctionsNegative
     * @tc.number SUB_USB_HostManager_InputParaEx_1700
     * @tc.desc   getStringFromFunctions input parameter -1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetStringFromFunctionsNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetStringFromFunctionsNegative begin');
      try {
        let ret = usbMgr.getStringFromFunctions(-1);
        console.info(TAG, 'usb testGetStringFromFunctionsNegative ret: ' + ret);
        expect(ret).assertEqual("none");
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetStringFromFunctionsNegative fail:' + JSON.stringify(err));
        done();
      }
    })

    /**
     * @tc.name   testGetStringFromFunctionsZero
     * @tc.number SUB_USB_HostManager_InputParaEx_1800
     * @tc.desc   getStringFromFunctions input parameter 0
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetStringFromFunctionsZero', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetStringFromFunctionsZero begin');
      try {
        let ret = usbMgr.getStringFromFunctions(0);
        console.info(TAG, 'usb testGetStringFromFunctionsZero ret: ' + ret);
        expect(ret).assertEqual("none");
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetStringFromFunctionsZero fail:' + JSON.stringify(err));
        done();
      }
    })

    /**
     * @tc.name   testGetDeviceFunctions001
     * @tc.number SUB_USB_HostManager_InputParaEx_1900
     * @tc.desc   getDeviceFunctions basic call test
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetDeviceFunctions001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetDeviceFunctions001 begin');
      try {
        let ret = usbMgr.getDeviceFunctions();
        console.info(TAG, 'usb testGetDeviceFunctions001 ret: ' + ret);
        expect(ret !== null).assertTrue();
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetDeviceFunctions001 fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(202);
        done();
      }
    })

    /**
     * @tc.name   testGetPortList001
     * @tc.number SUB_USB_HostManager_InputParaEx_2000
     * @tc.desc   getPortList basic call test
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetPortList001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetPortList001 begin');
      try {
        let ret = usbMgr.getPortList();
        console.info(TAG, 'usb testGetPortList001 ret: ' + JSON.stringify(ret));
        expect(Array.isArray(ret)).assertTrue();
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetPortList001 fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(202);
        done();
      }
    })

    /**
     * @tc.name   testGetPortSupportModesNull
     * @tc.number SUB_USB_HostManager_InputParaEx_2100
     * @tc.desc   getPortSupportModes input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetPortSupportModesNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetPortSupportModesNull begin');
      try {
        let ret = usbMgr.getPortSupportModes(null);
        console.info(TAG, 'usb testGetPortSupportModesNull ret: ' + JSON.stringify(ret));
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetPortSupportModesNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testGetPortSupportModesUndefined
     * @tc.number SUB_USB_HostManager_InputParaEx_2200
     * @tc.desc   getPortSupportModes input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetPortSupportModesUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetPortSupportModesUndefined begin');
      try {
        let ret = usbMgr.getPortSupportModes(undefined);
        console.info(TAG, 'usb testGetPortSupportModesUndefined ret: ' + JSON.stringify(ret));
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetPortSupportModesUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testGetPortSupportModesNegative
     * @tc.number SUB_USB_HostManager_InputParaEx_2300
     * @tc.desc   getPortSupportModes input parameter -1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetPortSupportModesNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetPortSupportModesNegative begin');
      try {
        let ret = usbMgr.getPortSupportModes(-1);
        console.info(TAG, 'usb testGetPortSupportModesNegative ret: ' + JSON.stringify(ret));
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetPortSupportModesNegative fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(202);
        done();
      }
    })

    /**
     * @tc.name   testSetPortRoleTypesNull
     * @tc.number SUB_USB_HostManager_InputParaEx_2400
     * @tc.desc   setPortRoleTypes input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetPortRoleTypesNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSetPortRoleTypesNull begin');
      try {
        await usbMgr.setPortRoleTypes(null, null, null);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSetPortRoleTypesNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSetPortRoleTypesUndefined
     * @tc.number SUB_USB_HostManager_InputParaEx_2500
     * @tc.desc   setPortRoleTypes input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetPortRoleTypesUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSetPortRoleTypesUndefined begin');
      try {
        await usbMgr.setPortRoleTypes(undefined, undefined, undefined);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSetPortRoleTypesUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSetPortRoleTypesNegative
     * @tc.number SUB_USB_HostManager_InputParaEx_2600
     * @tc.desc   setPortRoleTypes input parameter negative values
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetPortRoleTypesNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSetPortRoleTypesNegative begin');
      try {
        await usbMgr.setPortRoleTypes(-1, -1, -1);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSetPortRoleTypesNegative fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(202);
        done();
      }
    })

    /**
     * @tc.name   testSetDeviceFunctionsNull
     * @tc.number SUB_USB_HostManager_InputParaEx_2700
     * @tc.desc   setDeviceFunctions input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetDeviceFunctionsNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSetDeviceFunctionsNull begin');
      try {
        await usbMgr.setDeviceFunctions(null);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSetDeviceFunctionsNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSetDeviceFunctionsUndefined
     * @tc.number SUB_USB_HostManager_InputParaEx_2800
     * @tc.desc   setDeviceFunctions input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetDeviceFunctionsUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSetDeviceFunctionsUndefined begin');
      try {
        await usbMgr.setDeviceFunctions(undefined);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSetDeviceFunctionsUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSetDeviceFunctionsNegative
     * @tc.number SUB_USB_HostManager_InputParaEx_2900
     * @tc.desc   setDeviceFunctions input parameter -1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetDeviceFunctionsNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSetDeviceFunctionsNegative begin');
      try {
        await usbMgr.setDeviceFunctions(-1);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSetDeviceFunctionsNegative fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(202);
        done();
      }
    })

    /**
     * @tc.name   testGetDevices001
     * @tc.number SUB_USB_HostManager_InputParaEx_3000
     * @tc.desc   getDevices basic call test
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetDevices001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetDevices001 begin');
      try {
        let ret = usbMgr.getDevices();
        console.info(TAG, 'usb testGetDevices001 ret length: ' + ret.length);
        expect(Array.isArray(ret)).assertTrue();
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetDevices001 fail:' + JSON.stringify(err));
        expect(err).assertInstanceOf('Error');
        done();
      }
    })

    /**
     * @tc.name   testHasAccessoryRightValid
     * @tc.number SUB_USB_HostManager_InputParaEx_3100
     * @tc.desc   hasAccessoryRight input parameter USBAccessoryFeature
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testHasAccessoryRightValid', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testHasAccessoryRightValid begin');
      try {
        let ret = usbMgr.hasAccessoryRight(USBAccessoryFeature);
        console.info(TAG, 'usb testHasAccessoryRightValid ret: ' + ret);
        expect(typeof ret === 'boolean').assertTrue();
        done();
      } catch (err) {
        console.info(TAG, 'usb testHasAccessoryRightValid fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(14400001);
        done();
      }
    })

    /**
     * @tc.name   testRequestAccessoryRightValid
     * @tc.number SUB_USB_HostManager_InputParaEx_3200
     * @tc.desc   requestAccessoryRight input parameter USBAccessoryFeature
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRequestAccessoryRightValid', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testRequestAccessoryRightValid begin');
      try {
        let ret = await usbMgr.requestAccessoryRight(USBAccessoryFeature);
        console.info(TAG, 'usb testRequestAccessoryRightValid ret: ' + ret);
        expect(typeof ret === 'boolean').assertTrue();
        done();
      } catch (err) {
        console.info(TAG, 'usb testRequestAccessoryRightValid fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(14400001);
        done();
      }
    })

    /**
     * @tc.name   testCancelAccessoryRightValid
     * @tc.number SUB_USB_HostManager_InputParaEx_3300
     * @tc.desc   cancelAccessoryRight input parameter USBAccessoryFeature
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testCancelAccessoryRightValid', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testCancelAccessoryRightValid begin');
      try {
        usbMgr.cancelAccessoryRight(USBAccessoryFeature);
        console.info(TAG, 'usb testCancelAccessoryRightValid success');
        done();
      } catch (err) {
        console.info(TAG, 'usb testCancelAccessoryRightValid fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(14400001);
        done();
      }
    })

    /**
     * @tc.name   testOpenAccessoryValid
     * @tc.number SUB_USB_HostManager_InputParaEx_3400
     * @tc.desc   openAccessory input parameter USBAccessoryFeature
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testOpenAccessoryValid', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testOpenAccessoryValid begin');
      try {
        let ret = usbMgr.openAccessory(USBAccessoryFeature);
        console.info(TAG, 'usb testOpenAccessoryValid ret: ' + JSON.stringify(ret));
        done();
      } catch (err) {
        console.info(TAG, 'usb testOpenAccessoryValid fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(14400001);
        done();
      }
    })

    /**
     * @tc.name   testCloseAccessoryValid
     * @tc.number SUB_USB_HostManager_InputParaEx_3500
     * @tc.desc   closeAccessory input parameter USBAccessoryHandle
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testCloseAccessoryValid', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testCloseAccessoryValid begin');
      let accessoryHandle: usbMgr.USBAccessoryHandle = {
        accessoryFd: -1,
      };
      try {
        usbMgr.closeAccessory(accessoryHandle);
        console.info(TAG, 'usb testCloseAccessoryValid success');
        done();
      } catch (err) {
        console.info(TAG, 'usb testCloseAccessoryValid fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(14400001);
        done();
      }
    })
  })
}
