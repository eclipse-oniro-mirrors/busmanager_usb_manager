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
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect, TestType, Size, Level } from '@ohos/hypium';

const TAG = "[UsbEnumEtsTest]";

export default function UsbEnumEtsTest() {
  describe('UsbEnumEtsTest', () => {

    beforeAll(async () => {
      console.log(TAG, '*************Usb Unit UsbEnumEtsTest Begin*************');
    })

    beforeEach(() => {
      console.info(TAG, 'beforeEach: *************Usb Unit Test Case*************');
    })

    afterEach(() => {
      console.info(TAG, 'afterEach: *************Usb Unit Test Case*************');
    })

    afterAll(() => {
      console.log(TAG, '*************Usb Unit UsbEnumEtsTest End*************');
    })

    /**
     * @tc.name   testPowerRoleTypeEnum
     * @tc.number SUB_USB_Enum_PowerRoleType_0100
     * @tc.desc   PowerRoleType enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testPowerRoleTypeEnum', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testPowerRoleTypeEnum begin');
      expect(usbMgr.PowerRoleType.NONE).assertEqual(0);
      expect(usbMgr.PowerRoleType.SOURCE).assertEqual(1);
      expect(usbMgr.PowerRoleType.SINK).assertEqual(2);
      done();
    })

    /**
     * @tc.name   testDataRoleTypeEnum
     * @tc.number SUB_USB_Enum_DataRoleType_0100
     * @tc.desc   DataRoleType enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testDataRoleTypeEnum', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testDataRoleTypeEnum begin');
      expect(usbMgr.DataRoleType.NONE).assertEqual(0);
      expect(usbMgr.DataRoleType.HOST).assertEqual(1);
      expect(usbMgr.DataRoleType.DEVICE).assertEqual(2);
      done();
    })

    /**
     * @tc.name   testPortModeTypeEnum
     * @tc.number SUB_USB_Enum_PortModeType_0100
     * @tc.desc   PortModeType enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testPortModeTypeEnum', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testPortModeTypeEnum begin');
      expect(usbMgr.PortModeType.NONE).assertEqual(0);
      expect(usbMgr.PortModeType.UFP).assertEqual(1);
      expect(usbMgr.PortModeType.DFP).assertEqual(2);
      expect(usbMgr.PortModeType.DRP).assertEqual(3);
      expect(usbMgr.PortModeType.NUM_MODES).assertEqual(4);
      done();
    })

    /**
     * @tc.name   testUSBRequestTargetTypeEnum
     * @tc.number SUB_USB_Enum_USBRequestTargetType_0100
     * @tc.desc   USBRequestTargetType enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUSBRequestTargetTypeEnum', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testUSBRequestTargetTypeEnum begin');
      expect(usbMgr.USBRequestTargetType.USB_REQUEST_TARGET_DEVICE).assertEqual(0);
      expect(usbMgr.USBRequestTargetType.USB_REQUEST_TARGET_INTERFACE).assertEqual(1);
      expect(usbMgr.USBRequestTargetType.USB_REQUEST_TARGET_ENDPOINT).assertEqual(2);
      expect(usbMgr.USBRequestTargetType.USB_REQUEST_TARGET_OTHER).assertEqual(3);
      done();
    })

    /**
     * @tc.name   testUSBControlRequestTypeEnum
     * @tc.number SUB_USB_Enum_USBControlRequestType_0100
     * @tc.desc   USBControlRequestType enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUSBControlRequestTypeEnum', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testUSBControlRequestTypeEnum begin');
      expect(usbMgr.USBControlRequestType.USB_REQUEST_TYPE_STANDARD).assertEqual(0);
      expect(usbMgr.USBControlRequestType.USB_REQUEST_TYPE_CLASS).assertEqual(1);
      expect(usbMgr.USBControlRequestType.USB_REQUEST_TYPE_VENDOR).assertEqual(2);
      done();
    })

    /**
     * @tc.name   testUSBRequestDirectionEnum
     * @tc.number SUB_USB_Enum_USBRequestDirection_0100
     * @tc.desc   USBRequestDirection enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUSBRequestDirectionEnum', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testUSBRequestDirectionEnum begin');
      expect(usbMgr.USBRequestDirection.USB_REQUEST_DIR_TO_DEVICE).assertEqual(0);
      expect(usbMgr.USBRequestDirection.USB_REQUEST_DIR_FROM_DEVICE).assertEqual(0x80);
      done();
    })

    /**
     * @tc.name   testFunctionTypeEnum
     * @tc.number SUB_USB_Enum_FunctionType_0100
     * @tc.desc   FunctionType enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testFunctionTypeEnum', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testFunctionTypeEnum begin');
      expect(usbMgr.FunctionType.NONE).assertEqual(0);
      expect(usbMgr.FunctionType.ACM).assertEqual(1);
      expect(usbMgr.FunctionType.ECM).assertEqual(2);
      expect(usbMgr.FunctionType.HDC).assertEqual(4);
      expect(usbMgr.FunctionType.MTP).assertEqual(8);
      expect(usbMgr.FunctionType.PTP).assertEqual(16);
      expect(usbMgr.FunctionType.RNDIS).assertEqual(32);
      expect(usbMgr.FunctionType.MIDI).assertEqual(64);
      expect(usbMgr.FunctionType.AUDIO_SOURCE).assertEqual(128);
      expect(usbMgr.FunctionType.NCM).assertEqual(256);
      done();
    })

    /**
     * @tc.name   testUsbTransferFlagsEnum
     * @tc.number SUB_USB_Enum_UsbTransferFlags_0100
     * @tc.desc   UsbTransferFlags enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbTransferFlagsEnum', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testUsbTransferFlagsEnum begin');
      expect(usbMgr.UsbTransferFlags.USB_TRANSFER_SHORT_NOT_OK).assertEqual(0);
      expect(usbMgr.UsbTransferFlags.USB_TRANSFER_FREE_BUFFER).assertEqual(1);
      expect(usbMgr.UsbTransferFlags.USB_TRANSFER_FREE_TRANSFER).assertEqual(2);
      expect(usbMgr.UsbTransferFlags.USB_TRANSFER_ADD_ZERO_PACKET).assertEqual(3);
      done();
    })

    /**
     * @tc.name   testUsbTransferStatusEnum
     * @tc.number SUB_USB_Enum_UsbTransferStatus_0100
     * @tc.desc   UsbTransferStatus enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbTransferStatusEnum', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testUsbTransferStatusEnum begin');
      expect(usbMgr.UsbTransferStatus.TRANSFER_COMPLETED).assertEqual(0);
      expect(usbMgr.UsbTransferStatus.TRANSFER_ERROR).assertEqual(1);
      expect(usbMgr.UsbTransferStatus.TRANSFER_TIMED_OUT).assertEqual(2);
      expect(usbMgr.UsbTransferStatus.TRANSFER_CANCELED).assertEqual(3);
      expect(usbMgr.UsbTransferStatus.TRANSFER_STALL).assertEqual(4);
      expect(usbMgr.UsbTransferStatus.TRANSFER_NO_DEVICE).assertEqual(5);
      expect(usbMgr.UsbTransferStatus.TRANSFER_OVERFLOW).assertEqual(6);
      done();
    })

    /**
     * @tc.name   testUsbEndpointTransferTypeEnum
     * @tc.number SUB_USB_Enum_UsbEndpointTransferType_0100
     * @tc.desc   UsbEndpointTransferType enum value verification
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testUsbEndpointTransferTypeEnum', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testUsbEndpointTransferTypeEnum begin');
      expect(usbMgr.UsbEndpointTransferType.TRANSFER_TYPE_ISOCHRONOUS).assertEqual(0x1);
      expect(usbMgr.UsbEndpointTransferType.TRANSFER_TYPE_BULK).assertEqual(0x2);
      expect(usbMgr.UsbEndpointTransferType.TRANSFER_TYPE_INTERRUPT).assertEqual(0x3);
      done();
    })

    /**
     * @tc.name   testGetFunctionsAndStringConversion
     * @tc.number SUB_USB_Enum_FunctionConversion_0100
     * @tc.desc   getFunctionsFromString and getStringFromFunctions round-trip conversion
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetFunctionsAndStringConversion', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetFunctionsAndStringConversion begin');
      try {
        let funcs = usbMgr.FunctionType.HDC;
        let strFunc = usbMgr.getStringFromFunctions(funcs);
        console.info(TAG, 'usb testGetFunctionsAndStringConversion strFunc: ' + strFunc);
        expect(strFunc).assertEqual('hdc');
        let backFuncs = usbMgr.getFunctionsFromString(strFunc);
        console.info(TAG, 'usb testGetFunctionsAndStringConversion backFuncs: ' + backFuncs);
        expect(backFuncs).assertEqual(funcs);
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetFunctionsAndStringConversion fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(202);
        done();
      }
    })

    /**
     * @tc.name   testGetFunctionsAndStringConversionNone
     * @tc.number SUB_USB_Enum_FunctionConversion_0200
     * @tc.desc   getFunctionsFromString and getStringFromFunctions for NONE
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testGetFunctionsAndStringConversionNone', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(TAG, 'usb testGetFunctionsAndStringConversionNone begin');
      try {
        let funcs = usbMgr.FunctionType.NONE;
        let strFunc = usbMgr.getStringFromFunctions(funcs);
        console.info(TAG, 'usb testGetFunctionsAndStringConversionNone strFunc: ' + strFunc);
        expect(strFunc).assertEqual('none');
        let backFuncs = usbMgr.getFunctionsFromString(strFunc);
        console.info(TAG, 'usb testGetFunctionsAndStringConversionNone backFuncs: ' + backFuncs);
        expect(backFuncs).assertEqual(funcs);
        done();
      } catch (err) {
        console.info(TAG, 'usb testGetFunctionsAndStringConversionNone fail:' + JSON.stringify(err));
        expect(err.code).assertEqual(202);
        done();
      }
    })
  })
}
