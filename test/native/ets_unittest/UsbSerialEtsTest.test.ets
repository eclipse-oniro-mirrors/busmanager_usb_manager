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

import serialManager from '@ohos.usbManager.serial';
import { BusinessError } from '@ohos.base';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect, TestType, Size, Level } from '@ohos/hypium';

const TAG = "[UsbSerialEtsTest]";

export default function UsbSerialEtsTest() {
  describe('UsbSerialEtsTest', () => {

    beforeAll(async () => {
      console.log(TAG, '*************Usb Unit UsbSerialEtsTest Begin*************');
    })

    beforeEach(() => {
      console.info(TAG, 'beforeEach: *************Usb Unit Test Case*************');
    })

    afterEach(() => {
      console.info(TAG, 'afterEach: *************Usb Unit Test Case*************');
    })

    afterAll(() => {
      console.log(TAG, '*************Usb Unit UsbSerialEtsTest End*************');
    })

    /**
     * @tc.name   testSerialGetPortList001
     * @tc.number SUB_USB_SerialManager_InputPara_0100
     * @tc.desc   serial getPortList basic call test
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialGetPortList001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialGetPortList001 begin');
      try {
        let portList = serialManager.getPortList();
        console.info(TAG, 'usb testSerialGetPortList001 portList: ' + JSON.stringify(portList));
        expect(Array.isArray(portList)).assertTrue();
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialGetPortList001 fail:' + JSON.stringify(err));
        done();
      }
    })

    /**
     * @tc.name   testSerialHasSerialRightNull
     * @tc.number SUB_USB_SerialManager_InputPara_0200
     * @tc.desc   hasSerialRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialHasSerialRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialHasSerialRightNull begin');
      try {
        let ret = serialManager.hasSerialRight(null);
        console.info(TAG, 'usb testSerialHasSerialRightNull ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialHasSerialRightNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialHasSerialRightUndefined
     * @tc.number SUB_USB_SerialManager_InputPara_0300
     * @tc.desc   hasSerialRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialHasSerialRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialHasSerialRightUndefined begin');
      try {
        let ret = serialManager.hasSerialRight(undefined);
        console.info(TAG, 'usb testSerialHasSerialRightUndefined ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialHasSerialRightUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialHasSerialRightNegative
     * @tc.number SUB_USB_SerialManager_InputPara_0400
     * @tc.desc   hasSerialRight input parameter -1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialHasSerialRightNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialHasSerialRightNegative begin');
      try {
        let ret = serialManager.hasSerialRight(-1);
        console.info(TAG, 'usb testSerialHasSerialRightNegative ret: ' + ret);
        expect(ret).assertFalse();
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialHasSerialRightNegative fail:' + JSON.stringify(err));
        done();
      }
    })

    /**
     * @tc.name   testSerialRequestSerialRightNull
     * @tc.number SUB_USB_SerialManager_InputPara_0500
     * @tc.desc   requestSerialRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialRequestSerialRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialRequestSerialRightNull begin');
      try {
        let ret = await serialManager.requestSerialRight(null);
        console.info(TAG, 'usb testSerialRequestSerialRightNull ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialRequestSerialRightNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialRequestSerialRightUndefined
     * @tc.number SUB_USB_SerialManager_InputPara_0600
     * @tc.desc   requestSerialRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialRequestSerialRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialRequestSerialRightUndefined begin');
      try {
        let ret = await serialManager.requestSerialRight(undefined);
        console.info(TAG, 'usb testSerialRequestSerialRightUndefined ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialRequestSerialRightUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialRequestSerialRightNegative
     * @tc.number SUB_USB_SerialManager_InputPara_0700
     * @tc.desc   requestSerialRight input parameter -1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialRequestSerialRightNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialRequestSerialRightNegative begin');
      try {
        let ret = await serialManager.requestSerialRight(-1);
        console.info(TAG, 'usb testSerialRequestSerialRightNegative ret: ' + ret);
        expect(ret).assertFalse();
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialRequestSerialRightNegative fail:' + JSON.stringify(err));
        done();
      }
    })

    /**
     * @tc.name   testSerialAddSerialRightNull
     * @tc.number SUB_USB_SerialManager_InputPara_0800
     * @tc.desc   addSerialRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialAddSerialRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialAddSerialRightNull begin');
      try {
        serialManager.addSerialRight(null, null);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialAddSerialRightNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialAddSerialRightUndefined
     * @tc.number SUB_USB_SerialManager_InputPara_0900
     * @tc.desc   addSerialRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialAddSerialRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialAddSerialRightUndefined begin');
      try {
        serialManager.addSerialRight(undefined, undefined);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialAddSerialRightUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialCancelSerialRightNull
     * @tc.number SUB_USB_SerialManager_InputPara_1000
     * @tc.desc   cancelSerialRight input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialCancelSerialRightNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialCancelSerialRightNull begin');
      try {
        serialManager.cancelSerialRight(null);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialCancelSerialRightNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialCancelSerialRightUndefined
     * @tc.number SUB_USB_SerialManager_InputPara_1100
     * @tc.desc   cancelSerialRight input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialCancelSerialRightUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialCancelSerialRightUndefined begin');
      try {
        serialManager.cancelSerialRight(undefined);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialCancelSerialRightUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialOpenNull
     * @tc.number SUB_USB_SerialManager_InputPara_1200
     * @tc.desc   open input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialOpenNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialOpenNull begin');
      try {
        serialManager.open(null);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialOpenNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialOpenUndefined
     * @tc.number SUB_USB_SerialManager_InputPara_1300
     * @tc.desc   open input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialOpenUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialOpenUndefined begin');
      try {
        serialManager.open(undefined);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialOpenUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialOpenNegative
     * @tc.number SUB_USB_SerialManager_InputPara_1400
     * @tc.desc   open input parameter -1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialOpenNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialOpenNegative begin');
      try {
        serialManager.open(-1);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialOpenNegative fail:' + JSON.stringify(err));
        done();
      }
    })

    /**
     * @tc.name   testSerialCloseNull
     * @tc.number SUB_USB_SerialManager_InputPara_1500
     * @tc.desc   close input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialCloseNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialCloseNull begin');
      try {
        serialManager.close(null);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialCloseNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialCloseUndefined
     * @tc.number SUB_USB_SerialManager_InputPara_1600
     * @tc.desc   close input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialCloseUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialCloseUndefined begin');
      try {
        serialManager.close(undefined);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialCloseUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialCloseNegative
     * @tc.number SUB_USB_SerialManager_InputPara_1700
     * @tc.desc   close input parameter -1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialCloseNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialCloseNegative begin');
      try {
        serialManager.close(-1);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialCloseNegative fail:' + JSON.stringify(err));
        done();
      }
    })

    /**
     * @tc.name   testSerialGetAttributeNull
     * @tc.number SUB_USB_SerialManager_InputPara_1800
     * @tc.desc   getAttribute input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialGetAttributeNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialGetAttributeNull begin');
      try {
        let ret = serialManager.getAttribute(null);
        console.info(TAG, 'usb testSerialGetAttributeNull ret: ' + JSON.stringify(ret));
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialGetAttributeNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialGetAttributeUndefined
     * @tc.number SUB_USB_SerialManager_InputPara_1900
     * @tc.desc   getAttribute input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialGetAttributeUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialGetAttributeUndefined begin');
      try {
        let ret = serialManager.getAttribute(undefined);
        console.info(TAG, 'usb testSerialGetAttributeUndefined ret: ' + JSON.stringify(ret));
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialGetAttributeUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialGetAttributeNegative
     * @tc.number SUB_USB_SerialManager_InputPara_2000
     * @tc.desc   getAttribute input parameter -1
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialGetAttributeNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialGetAttributeNegative begin');
      try {
        let ret = serialManager.getAttribute(-1);
        console.info(TAG, 'usb testSerialGetAttributeNegative ret: ' + JSON.stringify(ret));
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialGetAttributeNegative fail:' + JSON.stringify(err));
        done();
      }
    })

    /**
     * @tc.name   testSerialSetAttributeNull
     * @tc.number SUB_USB_SerialManager_InputPara_2100
     * @tc.desc   setAttribute input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialSetAttributeNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialSetAttributeNull begin');
      try {
        serialManager.setAttribute(null, null);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialSetAttributeNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialSetAttributeUndefined
     * @tc.number SUB_USB_SerialManager_InputPara_2200
     * @tc.desc   setAttribute input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialSetAttributeUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialSetAttributeUndefined begin');
      try {
        serialManager.setAttribute(undefined, undefined);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialSetAttributeUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialSetAttributeNegative
     * @tc.number SUB_USB_SerialManager_InputPara_2300
     * @tc.desc   setAttribute input parameter -1 and valid attribute
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialSetAttributeNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialSetAttributeNegative begin');
      let attribute: serialManager.SerialAttribute = {
        baudRate: serialManager.BaudRates.BAUDRATE_9600,
        dataBits: serialManager.DataBits.DATABIT_8,
        parity: serialManager.Parity.PARITY_NONE,
        stopBits: serialManager.StopBits.STOPBIT_1,
      };
      try {
        serialManager.setAttribute(-1, attribute);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialSetAttributeNegative fail:' + JSON.stringify(err));
        done();
      }
    })

    /**
     * @tc.name   testSerialReadNull
     * @tc.number SUB_USB_SerialManager_InputPara_2400
     * @tc.desc   read input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialReadNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialReadNull begin');
      try {
        let ret = await serialManager.read(null, null);
        console.info(TAG, 'usb testSerialReadNull ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialReadNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialReadUndefined
     * @tc.number SUB_USB_SerialManager_InputPara_2500
     * @tc.desc   read input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialReadUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialReadUndefined begin');
      try {
        let ret = await serialManager.read(undefined, undefined);
        console.info(TAG, 'usb testSerialReadUndefined ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialReadUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialReadNegative
     * @tc.number SUB_USB_SerialManager_InputPara_2600
     * @tc.desc   read input parameter -1 and valid buffer
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialReadNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialReadNegative begin');
      let buffer = new Uint8Array(1024);
      try {
        let ret = await serialManager.read(-1, buffer);
        console.info(TAG, 'usb testSerialReadNegative ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialReadNegative fail:' + JSON.stringify(err));
        done();
      }
    })

    /**
     * @tc.name   testSerialWriteNull
     * @tc.number SUB_USB_SerialManager_InputPara_2700
     * @tc.desc   write input parameter null
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialWriteNull', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialWriteNull begin');
      try {
        let ret = await serialManager.write(null, null);
        console.info(TAG, 'usb testSerialWriteNull ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialWriteNull fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialWriteUndefined
     * @tc.number SUB_USB_SerialManager_InputPara_2800
     * @tc.desc   write input parameter undefined
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialWriteUndefined', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialWriteUndefined begin');
      try {
        let ret = await serialManager.write(undefined, undefined);
        console.info(TAG, 'usb testSerialWriteUndefined ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialWriteUndefined fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.name   testSerialWriteNegative
     * @tc.number SUB_USB_SerialManager_InputPara_2900
     * @tc.desc   write input parameter -1 and valid buffer
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialWriteNegative', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialWriteNegative begin');
      let buffer = new Uint8Array([0x41, 0x42, 0x43]);
      try {
        let ret = await serialManager.write(-1, buffer);
        console.info(TAG, 'usb testSerialWriteNegative ret: ' + ret);
        done();
      } catch (err) {
        console.info(TAG, 'usb testSerialWriteNegative fail:' + JSON.stringify(err));
        done();
      }
    })

    /**
     * @tc.name   testSerialEnumBaudRates001
     * @tc.number SUB_USB_SerialManager_Enum_0100
     * @tc.desc   BaudRates enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialEnumBaudRates001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialEnumBaudRates001 begin');
      expect(serialManager.BaudRates.BAUDRATE_50).assertEqual(50);
      expect(serialManager.BaudRates.BAUDRATE_300).assertEqual(300);
      expect(serialManager.BaudRates.BAUDRATE_9600).assertEqual(9600);
      expect(serialManager.BaudRates.BAUDRATE_115200).assertEqual(115200);
      expect(serialManager.BaudRates.BAUDRATE_4000000).assertEqual(4000000);
      done();
    })

    /**
     * @tc.name   testSerialEnumDataBits001
     * @tc.number SUB_USB_SerialManager_Enum_0200
     * @tc.desc   DataBits enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialEnumDataBits001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialEnumDataBits001 begin');
      expect(serialManager.DataBits.DATABIT_8).assertEqual(8);
      expect(serialManager.DataBits.DATABIT_7).assertEqual(7);
      expect(serialManager.DataBits.DATABIT_6).assertEqual(6);
      expect(serialManager.DataBits.DATABIT_5).assertEqual(5);
      done();
    })

    /**
     * @tc.name   testSerialEnumParity001
     * @tc.number SUB_USB_SerialManager_Enum_0300
     * @tc.desc   Parity enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialEnumParity001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialEnumParity001 begin');
      expect(serialManager.Parity.PARITY_NONE).assertEqual(0);
      expect(serialManager.Parity.PARITY_ODD).assertEqual(1);
      expect(serialManager.Parity.PARITY_EVEN).assertEqual(2);
      expect(serialManager.Parity.PARITY_MARK).assertEqual(3);
      expect(serialManager.Parity.PARITY_SPACE).assertEqual(4);
      done();
    })

    /**
     * @tc.name   testSerialEnumStopBits001
     * @tc.number SUB_USB_SerialManager_Enum_0400
     * @tc.desc   StopBits enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testSerialEnumStopBits001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testSerialEnumStopBits001 begin');
      expect(serialManager.StopBits.STOPBIT_1).assertEqual(0);
      expect(serialManager.StopBits.STOPBIT_2).assertEqual(1);
      done();
    })
  })
}
