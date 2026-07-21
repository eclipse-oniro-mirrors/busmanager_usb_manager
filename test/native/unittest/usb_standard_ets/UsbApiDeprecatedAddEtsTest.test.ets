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

import usb from '@ohos.usb';
import { BusinessError } from '@ohos.base'
import Buffer from '@ohos.buffer'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect, TestType, Size, Level } from '@ohos/hypium';

const TAG = "[UsbApiDeprecatedAddEtsTest]";

let USBDeviceFeature:usb.USBDevice;
let USBDeviceFeature1:usb.USBDevice;
let USBDeviceFeature2:usb.USBDevice;
let USBDeviceFeature3:usb.USBDevice;
let USBDeviceFeature4:usb.USBDevice;
let USBConfigFeature:usb.USBConfig;
let USBConfigFeature1:usb.USBConfig;
let USBConfigFeature2:usb.USBConfig;
let USBConfigFeature3:usb.USBConfig;
let USBInterfaceFeature:usb.USBInterface;
let USBInterfaceFeature1:usb.USBInterface;
let USBInterfaceFeature2:usb.USBInterface;
let USBEndpointFeature:usb.USBEndpoint;
let USBEndpointFeature1:usb.USBEndpoint;

USBDeviceFeature = {
  busNum: 2,
  devAddress: 3,
  serial: '',
  name: '',
  manufacturerName: '',
  productName: '',
  version: '',
  vendorId: 0,
  productId: 0,
  clazz: 0,
  subClass: 0,
  protocol: 0,
  configs: [],
}

USBConfigFeature = {
  id: 2,
  attributes: 3,
  maxPower: 0,
  name: '',
  isRemoteWakeup: true,
  isSelfPowered: true,
  interfaces: [],
}

USBInterfaceFeature = {
  id: 2,
  protocol: 3,
  clazz: 0,
  subClass: 0,
  alternateSetting: 0,
  name: '',
  endpoints: [],
}

USBEndpointFeature = {
  address: 2,
  attributes: 3,
  interval: 0,
  maxPacketSize: 0,
  direction: usb.USBRequestDirection.USB_REQUEST_DIR_TO_DEVICE,
  number: 0,
  type: 0,
  interfaceId: 0,
}

USBEndpointFeature1 = {
  address: 2,
  attributes: 3,
  interval: 0,
  maxPacketSize: 0,
  direction: usb.USBRequestDirection.USB_REQUEST_DIR_FROM_DEVICE,
  number: 0,
  type: 0,
  interfaceId: 0,
}

USBInterfaceFeature1 = {
  id: 2,
  protocol: 3,
  clazz: 0,
  subClass: 0,
  alternateSetting: 0,
  name: '',
  endpoints: [USBEndpointFeature],
}

USBInterfaceFeature2 = {
  id: 2,
  protocol: 3,
  clazz: 0,
  subClass: 0,
  alternateSetting: 0,
  name: '',
  endpoints: [USBEndpointFeature1],
}

USBConfigFeature1 = {
  id: 2,
  attributes: 3,
  maxPower: 0,
  name: '',
  isRemoteWakeup: false,
  isSelfPowered: false,
  interfaces: [USBInterfaceFeature],
}

USBConfigFeature2 = {
  id: 2,
  attributes: 3,
  maxPower: 0,
  name: '',
  isRemoteWakeup: false,
  isSelfPowered: false,
  interfaces: [USBInterfaceFeature1],
}

USBConfigFeature3 = {
  id: 2,
  attributes: 3,
  maxPower: 0,
  name: '',
  isRemoteWakeup: false,
  isSelfPowered: false,
  interfaces: [USBInterfaceFeature2],
}

USBDeviceFeature1 = {
  busNum: 2,
  devAddress: 3,
  serial: '',
  name: '',
  manufacturerName: '',
  productName: '',
  version: '',
  vendorId: 0,
  productId: 0,
  clazz: 0,
  subClass: 0,
  protocol: 0,
  configs: [USBConfigFeature],
}

USBDeviceFeature2 = {
  busNum: 2,
  devAddress: 3,
  serial: '',
  name: '',
  manufacturerName: '',
  productName: '',
  version: '',
  vendorId: 0,
  productId: 0,
  clazz: 0,
  subClass: 0,
  protocol: 0,
  configs: [USBConfigFeature1],
}

USBDeviceFeature3 = {
  busNum: 2,
  devAddress: 3,
  serial: '',
  name: '',
  manufacturerName: '',
  productName: '',
  version: '',
  vendorId: 0,
  productId: 0,
  clazz: 0,
  subClass: 0,
  protocol: 0,
  configs: [USBConfigFeature2],
}

USBDeviceFeature4 = {
  busNum: 2,
  devAddress: 3,
  serial: '',
  name: '',
  manufacturerName: '',
  productName: '',
  version: '',
  vendorId: 0,
  productId: 0,
  clazz: 0,
  subClass: 0,
  protocol: 0,
  configs: [USBConfigFeature3],
}

function generateLargeData(sizeInBytes: number): string {
  const char = 'A';
  const charSizeInBytes = Buffer.byteLength(char);
  const repeatTimes = Math.floor(sizeInBytes / charSizeInBytes);
  return char.repeat(repeatTimes);
}

export default function UsbApiDeprecatedAddEtsTest() {
  describe('UsbApiDeprecatedAddEtsTest', () => {
    
    beforeAll(async () => {
      console.log(TAG, '*************Usb Unit UsbApiDeprecatedAddEtsTest Begin*************');
    })

    beforeEach(() => {
      console.info(TAG, 'beforeEach: *************Usb Unit Test  Case*************');
    })

    afterEach(() => {
      console.info(TAG, 'afterEach: *************Usb Unit Test  Case*************');
    })

    afterAll(() => {
      console.log(TAG, '*************Usb Unit UsbApiDeprecatedEtsTest End*************');
    })
    

    /**
     * @tc.name   testHasRightNoString
     * @tc.number SUB_USB_HostManager_UsbFunc_0100
     * @tc.desc   hasRight input parameter ""
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testHasRightNoString', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testHasRightNoString begin');
      let ret = usb.hasRight("");
      console.info(TAG, 'usb testHasRightNoString ret: ' + ret);
      expect(ret).assertFalse();
      done();
    })

    /**
     * @tc.name   testHasRightMaxString
     * @tc.number SUB_USB_HostManager_UsbFunc_4500
     * @tc.desc   hasRight input parameter MaxString
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testHasRightMaxString', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testHasRightMaxString begin');
      let largeData = generateLargeData(200 * 1024 - 4);
      console.info(TAG, 'usb testHasRightMaxString largeData: ' + Buffer.byteLength(largeData));
      let ret = usb.hasRight(largeData);
      console.info(TAG, 'usb testHasRightMaxString ret: ' + ret);
      expect(ret).assertFalse();
      done();
    })

    /**
     * @tc.name   testHasRightMaxxString
     * @tc.number SUB_USB_HostManager_UsbFunc_4600
     * @tc.desc   hasRight input parameter MaxString+1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testHasRightMaxxString', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testHasRightMaxxString begin');
      let largeData = generateLargeData(201 * 1024 - 4);
      console.info(TAG, 'usb testHasRightMaxxString largeData: ' + Buffer.byteLength(largeData));
      let ret = usb.hasRight(largeData);
      console.info(TAG, 'usb testHasRightMaxxString ret: ' + ret);
      expect(ret).assertFalse();
      done();
    })

    /**
     * @tc.name   testHasRightNull
     * @tc.number SUB_USB_HostManager_UsbFunc_0200
     * @tc.desc   hasRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testHasRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testHasRightNull begin');
      try {
        let ret = usb.hasRight(null);
        console.info(TAG, 'usb testHasRightNull ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testHasRightNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testHasRightUndefined
     * @tc.number SUB_USB_HostManager_UsbFunc_0300
     * @tc.desc   hasRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testHasRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testHasRightUndefined begin');
      try {
        let ret = usb.hasRight(undefined);
        console.info(TAG, 'usb testHasRightUndefined ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testHasRightUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testRequestRightNoString
     * @tc.number SUB_USB_HostManager_UsbFunc_0400
     * @tc.desc   requestRight input parameter ""
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRequestRightNoString', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testRequestRightNoString begin');
      let ret = await usb.requestRight("");
      console.info(TAG, 'usb testRequestRightNoString ret: ' + ret);
      expect(ret).assertFalse();
      done();
    })

    /**
     * @tc.name   testRequestRightMaxString
     * @tc.number SUB_USB_HostManager_UsbFunc_4700
     * @tc.desc   requestRight input parameter MaxString
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRequestRightMaxString', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testRequestRightMaxString begin');
      let largeData = generateLargeData(200 * 1024 - 4);
      console.info(TAG, 'usb testRequestRightMaxString largeData: ' + Buffer.byteLength(largeData));
      let ret = await usb.requestRight(largeData);
      console.info(TAG, 'usb testRequestRightMaxString ret: ' + ret);
      expect(ret).assertFalse();
      done();
    })

    /**
     * @tc.name   testRequestRightMaxxString
     * @tc.number SUB_USB_HostManager_UsbFunc_4800
     * @tc.desc   requestRight input parameter MaxString+1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRequestRightMaxxString', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testRequestRightMaxxString begin');
      let largeData = generateLargeData(201 * 1024 - 4);
      console.info(TAG, 'usb testRequestRightMaxxString largeData: ' + Buffer.byteLength(largeData));
      let ret = await usb.requestRight(largeData);
      console.info(TAG, 'usb testRequestRightMaxxString ret: ' + ret);
      expect(ret).assertFalse();
      done();
    })

    /**
     * @tc.name   testRequestRightNull
     * @tc.number SUB_USB_HostManager_UsbFunc_0500
     * @tc.desc   requestRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRequestRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testRequestRightNull begin');
      try {
        let ret = await usb.requestRight(null);
        console.info(TAG, 'usb testRequestRightNull ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testRequestRightNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testRequestRightUndefined
     * @tc.number SUB_USB_HostManager_UsbFunc_0600
     * @tc.desc   requestRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRequestRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testRequestRightUndefined begin');
      try {
        let ret = await usb.requestRight(undefined);
        console.info(TAG, 'usb testRequestRightUndefined ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testRequestRightUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })
})
}