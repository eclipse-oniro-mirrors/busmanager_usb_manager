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

import usbMgr from '@ohos.usbManager';
import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it, Level, Size, TestType } from '@ohos/hypium';

const TAG = "[UsbApiInputParaTest]";

let USBDeviceFeature: usbMgr.USBDevice;
let USBDeviceFeature1: usbMgr.USBDevice;
let USBDeviceFeature2: usbMgr.USBDevice;
let USBDeviceFeature3: usbMgr.USBDevice;
let USBDeviceFeature4: usbMgr.USBDevice;
let USBConfigurationFeature: usbMgr.USBConfiguration;
let USBConfigurationFeature1: usbMgr.USBConfiguration;
let USBConfigurationFeature2: usbMgr.USBConfiguration;
let USBConfigurationFeature3: usbMgr.USBConfiguration;
let USBInterfaceFeature: usbMgr.USBInterface;
let USBInterfaceFeature1: usbMgr.USBInterface;
let USBInterfaceFeature2: usbMgr.USBInterface;
let USBEndpointFeature: usbMgr.USBEndpoint;
let USBEndpointFeature1: usbMgr.USBEndpoint;

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

USBConfigurationFeature = {
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
  direction: usbMgr.USBRequestDirection.USB_REQUEST_DIR_TO_DEVICE,
  number: 0,
  type: 0,
  interfaceId: 0,
}

USBEndpointFeature1 = {
  address: 2,
  attributes: 3,
  interval: 0,
  maxPacketSize: 0,
  direction: usbMgr.USBRequestDirection.USB_REQUEST_DIR_FROM_DEVICE,
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

USBConfigurationFeature1 = {
  id: 2,
  attributes: 3,
  maxPower: 0,
  name: '',
  isRemoteWakeup: false,
  isSelfPowered: false,
  interfaces: [USBInterfaceFeature],
}

USBConfigurationFeature2 = {
  id: 2,
  attributes: 3,
  maxPower: 0,
  name: '',
  isRemoteWakeup: false,
  isSelfPowered: false,
  interfaces: [USBInterfaceFeature1],
}

USBConfigurationFeature3 = {
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
  configs: [USBConfigurationFeature],
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
  configs: [USBConfigurationFeature1],
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
  configs: [USBConfigurationFeature2],
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
  configs: [USBConfigurationFeature3],
}

export default function UsbApiInputParaTest() {
  describe('UsbApiInputParaTest', () => {

    beforeAll(async () => {
      console.info(`${TAG} beforeAll start`);
    })

    beforeEach(() => {
      console.info(`${TAG} beforeEach`);
    })

    afterEach(() => {
      console.info(`${TAG} afterEach`);
    })

    afterAll(() => {
      console.info(`${TAG} afterAll`);
    })

    /**
     * @tc.name   testHasRightNoString
     * @tc.number SUB_USB_HostManager_InputPara_0100
     * @tc.desc   hasRight input parameter ""
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testHasRightNoString', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testHasRightNoString start`);
      let ret = usbMgr.hasRight("");
      console.info(`${TAG} testHasRightNoString ret: ${ret}`);
      expect(ret).assertFalse();
    })

    /**
     * @tc.name   testHasRightNull
     * @tc.number SUB_USB_HostManager_InputPara_0200
     * @tc.desc   hasRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testHasRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testHasRightNull start`);
      try {
        let ret = usbMgr.hasRight(null);
        console.info(`${TAG} testHasRightNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testHasRightNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testHasRightUndefined
     * @tc.number SUB_USB_HostManager_InputPara_0300
     * @tc.desc   hasRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testHasRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testHasRightUndefined start`);
      try {
        let ret = usbMgr.hasRight(undefined);
        console.info(`${TAG} testHasRightUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testHasRightUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testRequestRightNoString
     * @tc.number SUB_USB_HostManager_InputPara_0400
     * @tc.desc   requestRight input parameter ""
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRequestRightNoString', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      try {
        console.info(`${TAG} testRequestRightNoString start`);
        let ret = await usbMgr.requestRight("");
        console.info(`${TAG} testRequestRightNoString ret: ${ret}`);
        expect(ret).assertFalse();
        done();
      } catch (err) {
        done(err);
      }
    })

    /**
     * @tc.name   testRequestRightNull
     * @tc.number SUB_USB_HostManager_InputPara_0500
     * @tc.desc   requestRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRequestRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      try {
        console.info(`${TAG} testRequestRightNull start`);
        let ret = await usbMgr.requestRight(null);
        console.info(`${TAG} testRequestRightNull ret: ${ret}`);
        expect().assertFail();
        done();
      } catch (err) {
        console.info(`${TAG} testRequestRightNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testRequestRightUndefined
     * @tc.number SUB_USB_HostManager_InputPara_0600
     * @tc.desc   requestRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRequestRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      try {
        console.info(`${TAG} testRequestRightUndefined start`);
        let ret = await usbMgr.requestRight(undefined);
        console.info(`${TAG} testRequestRightUndefined ret: ${ret}`);
        expect().assertFail();
        done();
      } catch (err) {
        console.info(`${TAG} testRequestRightUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testClosePipeNull
     * @tc.number SUB_USB_HostManager_InputPara_0700
     * @tc.desc   closePipe input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testClosePipeNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testClosePipeNull start`);
      try {
        let ret = usbMgr.closePipe(null);
        console.info(`${TAG} testClosePipeNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testClosePipeNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testClosePipeUndefined
     * @tc.number SUB_USB_HostManager_InputPara_0800
     * @tc.desc   closePipe input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testClosePipeUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testClosePipeUndefined start`);
      try {
        let ret = usbMgr.closePipe(undefined);
        console.info(`${TAG} testClosePipeUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testClosePipeUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testGetRawDescriptorNull
     * @tc.number SUB_USB_HostManager_InputPara_0900
     * @tc.desc   getRawDescriptor input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetRawDescriptorNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testGetRawDescriptorNull start`);
      try {
        let ret = usbMgr.getRawDescriptor(null);
        console.info(`${TAG} testGetRawDescriptorNull ret: ${JSON.stringify(ret)}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testGetRawDescriptorNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testGetRawDescriptorUndefined
     * @tc.number SUB_USB_HostManager_InputPara_1000
     * @tc.desc   getRawDescriptor input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetRawDescriptorUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testGetRawDescriptorUndefined start`);
      try {
        let ret = usbMgr.getRawDescriptor(undefined);
        console.info(`${TAG} testGetRawDescriptorUndefined ret: ${JSON.stringify(ret)}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testGetRawDescriptorUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testGetFileDescriptorNull
     * @tc.number SUB_USB_HostManager_InputPara_1100
     * @tc.desc   getFileDescriptor input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetFileDescriptorNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testGetFileDescriptorNull start`);
      try {
        let ret = usbMgr.getFileDescriptor(null);
        console.info(`${TAG} testGetFileDescriptorNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testGetFileDescriptorNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testGetFileDescriptorUndefined
     * @tc.number SUB_USB_HostManager_InputPara_1200
     * @tc.desc   getFileDescriptor input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetFileDescriptorUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testGetFileDescriptorUndefined start`);
      try {
        let ret = usbMgr.getFileDescriptor(undefined);
        console.info(`${TAG} testGetFileDescriptorUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testGetFileDescriptorUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testConnectDeviceNull
     * @tc.number SUB_USB_HostManager_InputPara_1300
     * @tc.desc   connectDevice input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testConnectDeviceNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testConnectDeviceNull start`);
      try {
        let ret = usbMgr.connectDevice(null);
        console.info(`${TAG} testConnectDeviceNull ret: ${JSON.stringify(ret)}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testConnectDeviceNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testConnectDeviceUndefined
     * @tc.number SUB_USB_HostManager_InputPara_1400
     * @tc.desc   connectDevice input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testConnectDeviceUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testConnectDeviceUndefined start`);
      try {
        let ret = usbMgr.connectDevice(undefined);
        console.info(`${TAG} testConnectDeviceUndefined ret: ${JSON.stringify(ret)}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testConnectDeviceUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testClaimInterfaceNull
     * @tc.number SUB_USB_HostManager_InputPara_1500
     * @tc.desc   claimInterface input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testClaimInterfaceNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testClaimInterfaceNull start`);
      try {
        let ret = usbMgr.claimInterface(null, null, false);
        console.info(`${TAG} testClaimInterfaceNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testClaimInterfaceNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testClaimInterfaceUndefined
     * @tc.number SUB_USB_HostManager_InputPara_1600
     * @tc.desc   claimInterface input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testClaimInterfaceUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testClaimInterfaceUndefined start`);
      try {
        let ret = usbMgr.claimInterface(undefined, undefined, false);
        console.info(`${TAG} testClaimInterfaceUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testClaimInterfaceUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testReleaseInterfaceNull
     * @tc.number SUB_USB_HostManager_InputPara_1700
     * @tc.desc   releaseInterface input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testReleaseInterfaceNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testReleaseInterfaceNull start`);
      try {
        let ret = usbMgr.releaseInterface(null, null);
        console.info(`${TAG} testReleaseInterfaceNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testReleaseInterfaceNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testReleaseInterfaceUndefined
     * @tc.number SUB_USB_HostManager_InputPara_1800
     * @tc.desc   releaseInterface input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testReleaseInterfaceUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testReleaseInterfaceUndefined start`);
      try {
        let ret = usbMgr.releaseInterface(undefined, undefined);
        console.info(`${TAG} testReleaseInterfaceUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testReleaseInterfaceUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testSetConfigurationNull
     * @tc.number SUB_USB_HostManager_InputPara_1900
     * @tc.desc   setConfiguration input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetConfigurationNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testSetConfigurationNull start`);
      try {
        let ret = usbMgr.setConfiguration(null, null);
        console.info(`${TAG} testSetConfigurationNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testSetConfigurationNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testSetConfigurationUndefined
     * @tc.number SUB_USB_HostManager_InputPara_2000
     * @tc.desc   setConfiguration input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetConfigurationUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testSetConfigurationUndefined start`);
      try {
        let ret = usbMgr.setConfiguration(undefined, undefined);
        console.info(`${TAG} testSetConfigurationUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testSetConfigurationUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testSetInterfaceNull
     * @tc.number SUB_USB_HostManager_InputPara_2100
     * @tc.desc   setInterface input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetInterfaceNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testSetInterfaceNull start`);
      try {
        let ret = usbMgr.setInterface(null, null);
        console.info(`${TAG} testSetInterfaceNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testSetInterfaceNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testSetInterfaceUndefined
     * @tc.number SUB_USB_HostManager_InputPara_2200
     * @tc.desc   setInterface input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetInterfaceUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testSetInterfaceUndefined start`);
      try {
        let ret = usbMgr.setInterface(undefined, undefined);
        console.info(`${TAG} testSetInterfaceUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testSetInterfaceUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testControlTransferNull
     * @tc.number SUB_USB_HostManager_InputPara_2300
     * @tc.desc   controlTransfer input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testControlTransferNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testControlTransferNull start`);
      try {
        let ret = usbMgr.controlTransfer(null, null, null);
        console.info(`${TAG} testControlTransferNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testControlTransferNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testControlTransferUndefined
     * @tc.number SUB_USB_HostManager_InputPara_2400
     * @tc.desc   controlTransfer input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testControlTransferUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testControlTransferUndefined start`);
      try {
        let ret = usbMgr.controlTransfer(undefined, undefined, undefined);
        console.info(`${TAG} testControlTransferUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testControlTransferUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testBulkTransferNull
     * @tc.number SUB_USB_HostManager_InputPara_2500
     * @tc.desc   bulkTransfer input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testBulkTransferNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testBulkTransferNull start`);
      try {
        let ret = usbMgr.bulkTransfer(null, null, null);
        console.info(`${TAG} testBulkTransferNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testBulkTransferNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testBulkTransferUndefined
     * @tc.number SUB_USB_HostManager_InputPara_2600
     * @tc.desc   bulkTransfer input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testBulkTransferUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testBulkTransferUndefined start`);
      try {
        let ret = usbMgr.bulkTransfer(undefined, undefined, undefined);
        console.info(`${TAG} testBulkTransferUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testBulkTransferUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbControlTransferNull
     * @tc.number SUB_USB_HostManager_InputPara_2700
     * @tc.desc   usbControlTransfer input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbControlTransferNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbControlTransferNull start`);
      try {
        let ret = usbMgr.usbControlTransfer(null, null, null);
        console.info(`${TAG} testUsbControlTransferNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testUsbControlTransferNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbControlTransferUndefined
     * @tc.number SUB_USB_HostManager_InputPara_2800
     * @tc.desc   usbControlTransfer input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbControlTransferUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbControlTransferUndefined start`);
      try {
        let ret = usbMgr.usbControlTransfer(undefined, undefined, undefined);
        console.info(`${TAG} testUsbControlTransferUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testUsbControlTransferUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testHasAccessoryRightNull
     * @tc.number SUB_USB_HostManager_InputPara_2900
     * @tc.desc   hasAccessoryRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testHasAccessoryRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testHasAccessoryRightNull start`);
      try {
        let ret = usbMgr.hasAccessoryRight(null);
        console.info(`${TAG} testHasAccessoryRightNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testHasAccessoryRightNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testHasAccessoryRightUndefined
     * @tc.number SUB_USB_HostManager_InputPara_3000
     * @tc.desc   hasAccessoryRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testHasAccessoryRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testHasAccessoryRightUndefined start`);
      try {
        let ret = usbMgr.hasAccessoryRight(undefined);
        console.info(`${TAG} testHasAccessoryRightUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testHasAccessoryRightUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testRequestAccessoryRightNull
     * @tc.number SUB_USB_HostManager_InputPara_3100
     * @tc.desc   requestAccessoryRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRequestAccessoryRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testRequestAccessoryRightNull start`);
      try {
        let ret = usbMgr.requestAccessoryRight(null);
        console.info(`${TAG} testRequestAccessoryRightNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testRequestAccessoryRightNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testRequestAccessoryRightUndefined
     * @tc.number SUB_USB_HostManager_InputPara_3200
     * @tc.desc   requestAccessoryRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testRequestAccessoryRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testRequestAccessoryRightUndefined start`);
      try {
        let ret = usbMgr.requestAccessoryRight(undefined);
        console.info(`${TAG} testRequestAccessoryRightUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testRequestAccessoryRightUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testCancelAccessoryRightNull
     * @tc.number SUB_USB_HostManager_InputPara_3300
     * @tc.desc   cancelAccessoryRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testCancelAccessoryRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testCancelAccessoryRightNull start`);
      try {
        usbMgr.cancelAccessoryRight(null);
      } catch (err) {
        console.info(`${TAG} testCancelAccessoryRightNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testCancelAccessoryRightUndefined
     * @tc.number SUB_USB_HostManager_InputPara_3400
     * @tc.desc   cancelAccessoryRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testCancelAccessoryRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testCancelAccessoryRightUndefined start`);
      try {
        usbMgr.cancelAccessoryRight(undefined);
      } catch (err) {
        console.info(`${TAG} testCancelAccessoryRightUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testOpenAccessoryNull
     * @tc.number SUB_USB_HostManager_InputPara_3500
     * @tc.desc   openAccessory input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testOpenAccessoryNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testOpenAccessoryNull start`);
      try {
        let ret = usbMgr.openAccessory(null);
        console.info(`${TAG} testOpenAccessoryNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testOpenAccessoryNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testOpenAccessoryUndefined
     * @tc.number SUB_USB_HostManager_InputPara_3600
     * @tc.desc   openAccessory input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testOpenAccessoryUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testOpenAccessoryUndefined start`);
      try {
        let ret = usbMgr.openAccessory(undefined);
        console.info(`${TAG} testOpenAccessoryUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testOpenAccessoryUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testCloseAccessoryNull
     * @tc.number SUB_USB_HostManager_InputPara_3700
     * @tc.desc   closeAccessory input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testCloseAccessoryNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testCloseAccessoryNull start`);
      try {
        usbMgr.closeAccessory(null);
      } catch (err) {
        console.info(`${TAG} testCloseAccessoryNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testCloseAccessoryUndefined
     * @tc.number SUB_USB_HostManager_InputPara_3800
     * @tc.desc   closeAccessory input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testCloseAccessoryUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testCloseAccessoryUndefined start`);
      try {
        usbMgr.closeAccessory(undefined);
      } catch (err) {
        console.info(`${TAG} testCloseAccessoryUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbSubmitTransferNull
     * @tc.number SUB_USB_HostManager_InputPara_3900
     * @tc.desc   usbSubmitTransfer input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbSubmitTransferNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbSubmitTransferNull start`);
      try {
        usbMgr.usbSubmitTransfer(null);
      } catch (err) {
        console.info(`${TAG} testUsbSubmitTransferNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbSubmitTransferUndefined
     * @tc.number SUB_USB_HostManager_InputPara_4000
     * @tc.desc   usbSubmitTransfer input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbSubmitTransferUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbSubmitTransferUndefined start`);
      try {
        usbMgr.usbSubmitTransfer(undefined);
      } catch (err) {
        console.info(`${TAG} testUsbSubmitTransferUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbCancelTransferNull
     * @tc.number SUB_USB_HostManager_InputPara_4100
     * @tc.desc   usbCancelTransfer input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbCancelTransferNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbCancelTransferNull start`);
      try {
        usbMgr.usbCancelTransfer(null);
      } catch (err) {
        console.info(`${TAG} testUsbCancelTransferNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbCancelTransferUndefined
     * @tc.number SUB_USB_HostManager_InputPara_4200
     * @tc.desc   usbCancelTransfer input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbCancelTransferUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbCancelTransferUndefined start`);
      try {
        usbMgr.usbCancelTransfer(undefined);
      } catch (err) {
        console.info(`${TAG} testUsbCancelTransferUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testResetUsbDeviceNull
     * @tc.number SUB_USB_HostManager_InputPara_4300
     * @tc.desc   resetUsbDevice input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testResetUsbDeviceNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testResetUsbDeviceNull start`);
      try {
        let ret = usbMgr.resetUsbDevice(null);
        console.info(`${TAG} testResetUsbDeviceNull ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testResetUsbDeviceNull catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testResetUsbDeviceUndefined
     * @tc.number SUB_USB_HostManager_InputPara_4400
     * @tc.desc   resetUsbDevice input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testResetUsbDeviceUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testResetUsbDeviceUndefined start`);
      try {
        let ret = usbMgr.resetUsbDevice(undefined);
        console.info(`${TAG} testResetUsbDeviceUndefined ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testResetUsbDeviceUndefined catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testConnectDeviceInputPara
     * @tc.number SUB_USB_HostManager_InputPara_4500
     * @tc.desc   connectDevice input parameter USBDeviceFeature
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testConnectDeviceInputPara', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testConnectDeviceInputPara start`);
      try {
        console.info(`${TAG} testConnectDeviceInputPara USBDeviceFeature: ${JSON.stringify(USBDeviceFeature)}`);
        let ret = usbMgr.connectDevice(USBDeviceFeature);
        console.info(`${TAG} testConnectDeviceInputPara ret: ${JSON.stringify(ret)}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testConnectDeviceInputPara catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(14400001);
      }
    })

    /**
     * @tc.name   testConnectDeviceInputPara001
     * @tc.number SUB_USB_HostManager_InputPara_4600
     * @tc.desc   connectDevice input parameter USBDeviceFeature1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testConnectDeviceInputPara001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testConnectDeviceInputPara001 start`);
      try {
        console.info(`${TAG} testConnectDeviceInputPara001 USBDeviceFeature1: ${JSON.stringify(USBDeviceFeature1)}`);
        let ret = usbMgr.connectDevice(USBDeviceFeature1);
        console.info(`${TAG} testConnectDeviceInputPara001 ret: ${JSON.stringify(ret)}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testConnectDeviceInputPara001 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(14400001);
      }
    })

    /**
     * @tc.name   testConnectDeviceInputPara002
     * @tc.number SUB_USB_HostManager_InputPara_4700
     * @tc.desc   connectDevice input parameter USBDeviceFeature2
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testConnectDeviceInputPara002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testConnectDeviceInputPara002 start`);
      try {
        console.info(`${TAG} testConnectDeviceInputPara002 USBDeviceFeature2: ${JSON.stringify(USBDeviceFeature2)}`);
        let ret = usbMgr.connectDevice(USBDeviceFeature2);
        console.info(`${TAG} testConnectDeviceInputPara002 ret: ${JSON.stringify(ret)}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testConnectDeviceInputPara002 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(14400001);
      }
    })

    /**
     * @tc.name   testConnectDeviceInputPara003
     * @tc.number SUB_USB_HostManager_InputPara_4800
     * @tc.desc   connectDevice input parameter USBDeviceFeature3
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testConnectDeviceInputPara003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testConnectDeviceInputPara003 start`);
      try {
        console.info(`${TAG} testConnectDeviceInputPara003 USBDeviceFeature3: ${JSON.stringify(USBDeviceFeature3)}`);
        let ret = usbMgr.connectDevice(USBDeviceFeature3);
        console.info(`${TAG} testConnectDeviceInputPara003 ret: ${JSON.stringify(ret)}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testConnectDeviceInputPara003 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(14400001);
      }
    })

    /**
     * @tc.name   testConnectDeviceInputPara004
     * @tc.number SUB_USB_HostManager_InputPara_4900
     * @tc.desc   connectDevice input parameter USBDeviceFeature4
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testConnectDeviceInputPara004', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testConnectDeviceInputPara004 start`);
      try {
        console.info(`${TAG} testConnectDeviceInputPara004 USBDeviceFeature4: ${JSON.stringify(USBDeviceFeature4)}`);
        let ret = usbMgr.connectDevice(USBDeviceFeature4);
        console.info(`${TAG} testConnectDeviceInputPara004 ret: ${JSON.stringify(ret)}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testConnectDeviceInputPara004 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(14400001);
      }
    })

    /**
     * @tc.name   testClaimInterfaceInputPara
     * @tc.number SUB_USB_HostManager_InputPara_5000
     * @tc.desc   claimInterface input parameter USBInterfaceFeature
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testClaimInterfaceInputPara', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testClaimInterfaceInputPara start`);
      try {
        console.info(`${TAG} testClaimInterfaceInputPara USBInterfaceFeature: ${JSON.stringify(USBInterfaceFeature)}`);
        let ret = usbMgr.claimInterface(null, USBInterfaceFeature);
        console.info(`${TAG} testClaimInterfaceInputPara ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testClaimInterfaceInputPara catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testClaimInterfaceInputPara001
     * @tc.number SUB_USB_HostManager_InputPara_5100
     * @tc.desc   claimInterface input parameter USBInterfaceFeature1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testClaimInterfaceInputPara001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testClaimInterfaceInputPara001 start`);
      try {
        console.info(
          `${TAG} testClaimInterfaceInputPara001 USBInterfaceFeature1: ${JSON.stringify(USBInterfaceFeature1)}`);
        let ret = usbMgr.claimInterface(null, USBInterfaceFeature1);
        console.info(`${TAG} testClaimInterfaceInputPara001 ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testClaimInterfaceInputPara001 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testClaimInterfaceInputPara002
     * @tc.number SUB_USB_HostManager_InputPara_5200
     * @tc.desc   claimInterface input parameter USBInterfaceFeature2
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testClaimInterfaceInputPara002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testClaimInterfaceInputPara002 start`);
      try {
        console.info(
          `${TAG} testClaimInterfaceInputPara002 USBInterfaceFeature2: ${JSON.stringify(USBInterfaceFeature2)}`);
        let ret = usbMgr.claimInterface(null, USBInterfaceFeature2);
        console.info(`${TAG} testClaimInterfaceInputPara002 ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testClaimInterfaceInputPara002 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testReleaseInterfaceInputPara
     * @tc.number SUB_USB_HostManager_InputPara_5300
     * @tc.desc   releaseInterface input parameter USBInterfaceFeature
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testReleaseInterfaceInputPara', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testReleaseInterfaceInputPara start`);
      try {
        console.info(
          `${TAG} testReleaseInterfaceInputPara USBInterfaceFeature: ${JSON.stringify(USBInterfaceFeature)}`);
        let ret = usbMgr.releaseInterface(null, USBInterfaceFeature);
        console.info(`${TAG} testReleaseInterfaceInputPara ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testReleaseInterfaceInputPara catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testReleaseInterfaceInputPara001
     * @tc.number SUB_USB_HostManager_InputPara_5400
     * @tc.desc   releaseInterface input parameter USBInterfaceFeature1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testReleaseInterfaceInputPara001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testReleaseInterfaceInputPara001 start`);
      try {
        console.info(
          `${TAG} testReleaseInterfaceInputPara001 USBInterfaceFeature1: ${JSON.stringify(USBInterfaceFeature1)}`);
        let ret = usbMgr.releaseInterface(null, USBInterfaceFeature1);
        console.info(`${TAG} testReleaseInterfaceInputPara001 ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testReleaseInterfaceInputPara001 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testReleaseInterfaceInputPara002
     * @tc.number SUB_USB_HostManager_InputPara_5500
     * @tc.desc   releaseInterface input parameter USBInterfaceFeature2
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testReleaseInterfaceInputPara002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testReleaseInterfaceInputPara002 start`);
      try {
        console.info(
          `${TAG} testReleaseInterfaceInputPara002 USBInterfaceFeature2: ${JSON.stringify(USBInterfaceFeature2)}`);
        let ret = usbMgr.releaseInterface(null, USBInterfaceFeature2);
        console.info(`${TAG} testReleaseInterfaceInputPara002 ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testReleaseInterfaceInputPara002 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testSetConfigurationInputPara
     * @tc.number SUB_USB_HostManager_InputPara_5600
     * @tc.desc   setConfiguration input parameter USBConfigurationFeature
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetConfigurationInputPara', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testSetConfigurationInputPara start`);
      try {
        console.info(
          `${TAG} testSetConfigurationInputPara USBConfigurationFeature: ${JSON.stringify(USBConfigurationFeature)}`);
        let ret = usbMgr.setConfiguration(null, USBConfigurationFeature);
        console.info(`${TAG} testSetConfigurationInputPara ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testSetConfigurationInputPara catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testSetConfigurationInputPara001
     * @tc.number SUB_USB_HostManager_InputPara_5700
     * @tc.desc   setConfiguration input parameter USBConfigurationFeature1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetConfigurationInputPara001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testSetConfigurationInputPara001 start`);
      try {
        console.info(`${TAG} testSetConfigurationInputPara001 USBConfigurationFeature1: ${JSON.stringify(
          USBConfigurationFeature1)}`);
        let ret = usbMgr.setConfiguration(null, USBConfigurationFeature1);
        console.info(`${TAG} testSetConfigurationInputPara001 ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testSetConfigurationInputPara001 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testSetConfigurationInputPara002
     * @tc.number SUB_USB_HostManager_InputPara_5800
     * @tc.desc   setConfiguration input parameter USBConfigurationFeature2
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetConfigurationInputPara002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testSetConfigurationInputPara002 start`);
      try {
        console.info(`${TAG} testSetConfigurationInputPara002 USBConfigurationFeature2: ${JSON.stringify(
          USBConfigurationFeature2)}`);
        let ret = usbMgr.setConfiguration(null, USBConfigurationFeature2);
        console.info(`${TAG} testSetConfigurationInputPara002 ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testSetConfigurationInputPara002 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testSetConfigurationInputPara003
     * @tc.number SUB_USB_HostManager_InputPara_5900
     * @tc.desc   setConfiguration input parameter USBConfigurationFeature3
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetConfigurationInputPara003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testSetConfigurationInputPara003 start`);
      try {
        console.info(`${TAG} testSetConfigurationInputPara003 USBConfigurationFeature3: ${JSON.stringify(
          USBConfigurationFeature3)}`);
        let ret = usbMgr.setConfiguration(null, USBConfigurationFeature3);
        console.info(`${TAG} testSetConfigurationInputPara003 ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testSetConfigurationInputPara003 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testSetInterfaceInputPara
     * @tc.number SUB_USB_HostManager_InputPara_6000
     * @tc.desc   setInterface input parameter USBInterfaceFeature
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetInterfaceInputPara', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testSetInterfaceInputPara start`);
      try {
        console.info(`${TAG} testSetInterfaceInputPara USBInterfaceFeature: ${JSON.stringify(USBInterfaceFeature)}`);
        let ret = usbMgr.setInterface(null, USBInterfaceFeature);
        console.info(`${TAG} testSetInterfaceInputPara ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testSetInterfaceInputPara catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testSetInterfaceInputPara001
     * @tc.number SUB_USB_HostManager_InputPara_6100
     * @tc.desc   setInterface input parameter USBInterfaceFeature1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetInterfaceInputPara001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testSetInterfaceInputPara001 start`);
      try {
        console.info(
          `${TAG} testSetInterfaceInputPara001 USBInterfaceFeature1: ${JSON.stringify(USBInterfaceFeature1)}`);
        let ret = usbMgr.setInterface(null, USBInterfaceFeature1);
        console.info(`${TAG} testSetInterfaceInputPara001 ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testSetInterfaceInputPara001 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testSetInterfaceInputPara002
     * @tc.number SUB_USB_HostManager_InputPara_6200
     * @tc.desc   setInterface input parameter USBInterfaceFeature2
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSetInterfaceInputPara002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testSetInterfaceInputPara002 start`);
      try {
        console.info(
          `${TAG} testSetInterfaceInputPara002 USBInterfaceFeature2: ${JSON.stringify(USBInterfaceFeature2)}`);
        let ret = usbMgr.setInterface(null, USBInterfaceFeature2);
        console.info(`${TAG} testSetInterfaceInputPara002 ret: ${ret}`);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testSetInterfaceInputPara002 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbTransferInputPara
     * @tc.number SUB_USB_HostManager_InputPara_6300
     * @tc.desc   usbControlTransfer input parameter USBInterfaceFeature2
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbTransferInputPara', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbTransferInputPara start`);
      try {
        let pip: usbMgr.USBDevicePipe = {
          devAddress: 1,
          busNum: 2,
        };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: usbMgr.UsbTransferFlags.USB_TRANSFER_SHORT_NOT_OK,
          endpoint: 1,
          type: usbMgr.UsbEndpointTransferType.TRANSFER_TYPE_BULK,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 0,
        };
        usbMgr.usbSubmitTransfer(transferParams);
        usbMgr.usbCancelTransfer(transferParams);
      } catch (err) {
        console.info(`${TAG} testUsbTransferInputPara catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(14400001);
      }
    })

    /**
     * @tc.name   testUsbTransferInputPara001
     * @tc.number SUB_USB_HostManager_InputPara_6400
     * @tc.desc   usbSubmitTransfer/usbCancelTransfer input parameter enum
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbTransferInputPara001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbTransferInputPara001 start`);
      try {
        let pip: usbMgr.USBDevicePipe = {
          devAddress: 1,
          busNum: 2,
        };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: usbMgr.UsbTransferFlags.USB_TRANSFER_FREE_BUFFER,
          endpoint: 1,
          type: usbMgr.UsbEndpointTransferType.TRANSFER_TYPE_INTERRUPT,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 1,
        };
        usbMgr.usbSubmitTransfer(transferParams);
        usbMgr.usbCancelTransfer(transferParams);
      } catch (err) {
        console.info(`${TAG} testUsbTransferInputPara001 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(14400001);
      }
    })

    /**
     * @tc.name   testUsbTransferInputPara002
     * @tc.number SUB_USB_HostManager_InputPara_6500
     * @tc.desc   usbSubmitTransfer/usbCancelTransfer input parameter enum
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbTransferInputPara002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbTransferInputPara002 start`);
      try {
        let pip: usbMgr.USBDevicePipe = {
          devAddress: 1,
          busNum: 2,
        };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: usbMgr.UsbTransferFlags.USB_TRANSFER_ADD_ZERO_PACKET,
          endpoint: 1,
          type: usbMgr.UsbEndpointTransferType.TRANSFER_TYPE_ISOCHRONOUS,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 2,
        };
        usbMgr.usbSubmitTransfer(transferParams);
        usbMgr.usbCancelTransfer(transferParams);
      } catch (err) {
        console.info(`${TAG} testUsbTransferInputPara002 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(14400001);
      }
    })

    /**
     * @tc.name   testUsbTransferInputPara003
     * @tc.number SUB_USB_HostManager_InputPara_6600
     * @tc.desc   usbSubmitTransfer/usbCancelTransfer input parameter enum
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbTransferInputPara003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbTransferInputPara003 start`);
      try {
        let pip: usbMgr.USBDevicePipe = {
          devAddress: 1,
          busNum: 2,
        };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: usbMgr.UsbTransferFlags.USB_TRANSFER_FREE_TRANSFER,
          endpoint: 1,
          type: usbMgr.UsbEndpointTransferType.TRANSFER_TYPE_BULK,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 1,
        };
        usbMgr.usbSubmitTransfer(transferParams);
        usbMgr.usbCancelTransfer(transferParams);
      } catch (err) {
        console.info(`${TAG} testUsbTransferInputPara003 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(14400001);
      }
    })

    /**
     * @tc.name   testUsbSubmitTransferParamEx001
     * @tc.number SUB_USB_HostManager_InputPara_6700
     * @tc.desc   usbSubmitTransfer with invalid flags=-1 should throw 401
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbSubmitTransferParamEx001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbSubmitTransferParamEx001 start`);
      try {
        let pip: usbMgr.USBDevicePipe = { devAddress: 1, busNum: 2 };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: -1,
          endpoint: 1,
          type: usbMgr.UsbEndpointTransferType.TRANSFER_TYPE_BULK,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 0,
        };
        usbMgr.usbSubmitTransfer(transferParams);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testUsbSubmitTransferParamEx001 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbSubmitTransferParamEx002
     * @tc.number SUB_USB_HostManager_InputPara_6800
     * @tc.desc   usbSubmitTransfer with invalid flags=4 (out of range) should throw 401
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbSubmitTransferParamEx002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbSubmitTransferParamEx002 start`);
      try {
        let pip: usbMgr.USBDevicePipe = { devAddress: 1, busNum: 2 };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: 4,
          endpoint: 1,
          type: usbMgr.UsbEndpointTransferType.TRANSFER_TYPE_BULK,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 0,
        };
        usbMgr.usbSubmitTransfer(transferParams);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testUsbSubmitTransferParamEx002 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbSubmitTransferParamEx003
     * @tc.number SUB_USB_HostManager_InputPara_6900
     * @tc.desc   usbSubmitTransfer with invalid type=0 should throw 401
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbSubmitTransferParamEx003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbSubmitTransferParamEx003 start`);
      try {
        let pip: usbMgr.USBDevicePipe = { devAddress: 1, busNum: 2 };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: usbMgr.UsbTransferFlags.USB_TRANSFER_SHORT_NOT_OK,
          endpoint: 1,
          type: 0,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 0,
        };
        usbMgr.usbSubmitTransfer(transferParams);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testUsbSubmitTransferParamEx003 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbSubmitTransferParamEx004
     * @tc.number SUB_USB_HostManager_InputPara_7000
     * @tc.desc   usbSubmitTransfer with invalid type=4 (out of range) should throw 401
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbSubmitTransferParamEx004', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbSubmitTransferParamEx004 start`);
      try {
        let pip: usbMgr.USBDevicePipe = { devAddress: 1, busNum: 2 };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: usbMgr.UsbTransferFlags.USB_TRANSFER_SHORT_NOT_OK,
          endpoint: 1,
          type: 4,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 0,
        };
        usbMgr.usbSubmitTransfer(transferParams);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testUsbSubmitTransferParamEx004 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbCancelTransferParamEx001
     * @tc.number SUB_USB_HostManager_InputPara_7100
     * @tc.desc   usbCancelTransfer with invalid flags=-1 should throw 401
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbCancelTransferParamEx001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbCancelTransferParamEx001 start`);
      try {
        let pip: usbMgr.USBDevicePipe = { devAddress: 1, busNum: 2 };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: -1,
          endpoint: 1,
          type: usbMgr.UsbEndpointTransferType.TRANSFER_TYPE_BULK,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 0,
        };
        usbMgr.usbCancelTransfer(transferParams);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testUsbCancelTransferParamEx001 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbCancelTransferParamEx002
     * @tc.number SUB_USB_HostManager_InputPara_7200
     * @tc.desc   usbCancelTransfer with invalid flags=4 (out of range) should throw 401
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbCancelTransferParamEx002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbCancelTransferParamEx002 start`);
      try {
        let pip: usbMgr.USBDevicePipe = { devAddress: 1, busNum: 2 };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: 4,
          endpoint: 1,
          type: usbMgr.UsbEndpointTransferType.TRANSFER_TYPE_BULK,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 0,
        };
        usbMgr.usbCancelTransfer(transferParams);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testUsbCancelTransferParamEx002 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbCancelTransferParamEx003
     * @tc.number SUB_USB_HostManager_InputPara_7300
     * @tc.desc   usbCancelTransfer with invalid type=0 should throw 401
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbCancelTransferParamEx003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbCancelTransferParamEx003 start`);
      try {
        let pip: usbMgr.USBDevicePipe = { devAddress: 1, busNum: 2 };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: usbMgr.UsbTransferFlags.USB_TRANSFER_SHORT_NOT_OK,
          endpoint: 1,
          type: 0,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 0,
        };
        usbMgr.usbCancelTransfer(transferParams);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testUsbCancelTransferParamEx003 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })

    /**
     * @tc.name   testUsbCancelTransferParamEx004
     * @tc.number SUB_USB_HostManager_InputPara_7400
     * @tc.desc   usbCancelTransfer with invalid type=4 (out of range) should throw 401
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbCancelTransferParamEx004', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testUsbCancelTransferParamEx004 start`);
      try {
        let pip: usbMgr.USBDevicePipe = { devAddress: 1, busNum: 2 };
        let transferParams: usbMgr.UsbDataTransferParams = {
          devPipe: pip,
          flags: usbMgr.UsbTransferFlags.USB_TRANSFER_SHORT_NOT_OK,
          endpoint: 1,
          type: 4,
          timeout: 2000,
          length: 10,
          callback: () => {
          },
          userData: new Uint8Array(10),
          buffer: new Uint8Array(10),
          isoPacketCount: 0,
        };
        usbMgr.usbCancelTransfer(transferParams);
        expect().assertFail();
      } catch (err) {
        console.info(`${TAG} testUsbCancelTransferParamEx004 catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(401);
      }
    })
  })
}
