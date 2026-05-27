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

import { describe, it, expect, TestType, Size, Level, beforeAll } from "../../../hypium/index";
import hilog from '@ohos.hilog'
import { BusinessError } from '@ohos.base';
import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';
import common from '@ohos.app.ability.common';
import UIAbility from '@ohos.app.ability.UIAbility';
import Want from '@ohos.app.ability.Want';
import Utils from './Util.test';
import usbManager from '@ohos.usbManager';
import bundleManager from '@ohos.bundle.bundleManager';

let gDeviceList: Array<Readonly<usbManager.USBDevice>> = usbManager.getDevices();
let devices: usbManager.USBDevice = gDeviceList[0];
let gPipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
let isDeviceConnected: boolean;
let tmpPipe: usbManager.USBDevicePipe;
let iface: usbManager.USBInterface;
let config: usbManager.USBConfiguration;
let accessory: usbManager.USBAccessory;
let powerRole: usbManager.PowerRoleType;
let dataRole: usbManager.DataRoleType;
let param: usbManager.USBDeviceRequestParams;
let isDevAccessoryFunc: boolean;
let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
let accessInfo: usbManager.USBAccessory;
let accessInfoTest: usbManager.USBAccessory = {
  manufacturer: 'manufacturer',
  product: 'product',
  description: 'description',
  version: 'version',
  serialNumber: 'serialNumber'
}

let domain: number = 0x0000;
let tag: string = 'testTag';
const PARAM_NULLSTRING: string = "";

function devAccessoryFunc() {
  if (accList.length > 0) {
    hilog.info(domain, tag, "This USB device is accessory function");
    return true;
  }
  hilog.info(domain, tag, "This USB device is accessory function");
  return false;
}

function getTransfersParam(gPipe: usbManager.USBDevicePipe, flagsValue: usbManager.UsbTransferFlags,
  endpointValue: int, typeValue: usbManager.UsbEndpointTransferType, timeOutValue: int) {
  let transferParams: usbManager.UsbDataTransferParams = {
    devPipe: gPipe,
    flags: flagsValue,
    endpoint: endpointValue,
    type: typeValue,
    timeout: timeOutValue,
    length: 1000,
    callback: () => {
    },
    userData: new Uint8Array(1000),
    buffer: new Uint8Array(1000),
    isoPacketCount: 0,
  };
  return transferParams;
}

function deviceConnected() {
  if (gDeviceList.length > 0) {
    hilog.info(domain, tag, "Test USB device is connected");
    return true;
  }
  hilog.info(domain, tag, "Test USB device is not connected");
  return false;
}

function getPipe(testCaseName: string) {
  gPipe = usbManager.connectDevice(devices);
  hilog.info(domain, tag, `usb ${testCaseName} connectDevice getPipe ret: ${JSON.stringify(gPipe)}`);
  expect(gPipe !== null).assertTrue();
}

function toClosePipe(testCaseName: string) {
  let isPipClose = usbManager.closePipe(gPipe);
  console.info(domain, tag, `usb ${testCaseName} closePipe getPipe ret: ${isPipClose}`);
  expect(isPipClose).assertEqual(0);
}


export default function abilityTest() {

  describe("abilityTest", (): void => {
    hilog.info(domain, tag, '%{public}s', 'describe start');
    beforeAll(() => {
      hilog.info(domain, tag, '%{public}s', 'beforeAll start');
      let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator = abilityDelegatorRegistry.getAbilityDelegator()
      abilityDelegator.addAbilityMonitor({
        abilityName: "EntryAbility",
        moduleName: "entry",
        onAbilityCreate: (abilitys: UIAbility): void => {
        },
      }, (err: BusinessError | null): void => {
        if (err != null) {
          hilog.info(domain, tag, '%{public}s', '-----' + err.code);
        }
        hilog.info(domain, tag, '%{public}s', 'BusinessError  end');
      });
      await Utils.msSleep(2000)
      abilityDelegator.executeShellCommand("aa start -a EntryAbility -b com.example.helloworld")
      await Utils.msSleep(2000)
      hilog.info(domain, tag, '%{public}s', 'beforeAll end');


      gDeviceList = usbManager.getDevices();
      hilog.info(domain, tag, '获取设备信息: %{public}s', JSON.stringify(gDeviceList));

      isDeviceConnected = deviceConnected();
      if (isDeviceConnected) {
        devices = gDeviceList[0];
        let hasRight = usbManager.hasRight(devices.name);
        if (!hasRight) {
          hilog.info(domain, tag, `beforeAll: usb requestRight start`);
        }
      }

      isDevAccessoryFunc = devAccessoryFunc();
      if (isDevAccessoryFunc) {
        accList = usbManager.getAccessoryList();
        accessInfo = accList[0];
        hilog.info(domain, tag, 'beforeAll ret : ', JSON.stringify(accList));
      }
    })
    it("testUsbSubmitTransfer001", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer003 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbSubmitTransfer001 No device is connected');
          expect(isDeviceConnected).assertFalse();
          done()
        }
        let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
        if (devicesList.length == 0) {
          hilog.info(domain, tag, 'device list is empty');
        }

        let devices: usbManager.USBDevice = devicesList[0];
        usbManager.requestRight(devices.name);
        let gPipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
        let tmpInterface: usbManager.USBInterface = devices.configs[0].interfaces[0];
        let flags: usbManager.UsbTransferFlags = usbManager.UsbTransferFlags.USB_TRANSFER_FREE_BUFFER;
        let type: usbManager.UsbEndpointTransferType = usbManager.UsbEndpointTransferType.TRANSFER_TYPE_INTERRUPT;
        let transferParams: usbManager.UsbDataTransferParams = getTransfersParam(gPipe, flags, 129, type, 2000);
        usbManager.claimInterface(gPipe, tmpInterface, true);
        try {
          transferParams.callback = (err: BusinessError | null, callBackData: usbManager.SubmitTransferCallback | undefined) => {
            if (typeof callBackData != 'undefined') {
              hilog.info(domain, tag, 'callBackData = ' + JSON.stringify(callBackData));
              hilog.info(domain, tag,
                'testUsbSubmitTransfer003 transfer success, result = ' + transferParams.buffer.toString());
              hilog.info(domain, tag, 'status = ' + callBackData?.status);
              expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_TIMED_OUT).assertEqual(true);
            }
            usbManager.closePipe(gPipe);
            hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer003 end');
            done()
          }
          usbManager.usbSubmitTransfer(transferParams);
        } catch (error) {
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, 'interrupt error:', err.code);
          usbManager.closePipe(gPipe);
          done()
        }
      })

    /**
     * @tc.number   : SUB_USB_Host_JS_usbCancelTransfer_Func_0100
     * @tc.name     : testUsbCancelTransfer001
     * @tc.desc     : interrupt transfer cancel successfully.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it("testUsbCancelTransfer001", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbCancelTransfer004 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbCancelTransfer004 No device is connected');
          expect(isDeviceConnected).assertFalse();
          done()
        }
        let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
        if (devicesList.length == 0) {
          hilog.info(domain, tag, 'device list is empty');
        }
        let devices: usbManager.USBDevice = devicesList[0];
        usbManager.requestRight(devices.name);
        let gPipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
        let endpoint = devices.configs[0].interfaces[0]?.endpoints.find((value) => {
          return value.direction == usbManager.USBRequestDirection.USB_REQUEST_DIR_TO_DEVICE; // && value.type === 2
        })
        let flags: usbManager.UsbTransferFlags = usbManager.UsbTransferFlags.USB_TRANSFER_FREE_BUFFER;
        let type: usbManager.UsbEndpointTransferType = usbManager.UsbEndpointTransferType.TRANSFER_TYPE_INTERRUPT;
        let transferParams: usbManager.UsbDataTransferParams = getTransfersParam(gPipe, flags, 129, type, 2000);
        // let err = 0;
        try {
          // transferParams.endpoint = endpoint?.address as number;
          // transferParams.callback = (err: BusinessError, callBackData: usbManager.SubmitTransferCallback) => {
          //   hilog.info(domain, tag, 'callBackData ==============' + JSON.stringify(callBackData));
          // }
          usbManager.usbSubmitTransfer(transferParams);
          usbManager.usbCancelTransfer(transferParams);
          hilog.info(domain, tag, 'USB transfer request submitted.');
        } catch (error: BusinessError) {
          hilog.info(domain, tag, 'testUsbCancelTransfer004 catch err code================: ' + JSON.stringify(error));
        }
        done();
      })

    /**
     * @tc.number   : SUB_USB_Host_JS_usbCancelTransfer_ErrCode_0100
     * @tc.name     : testUsbCancelTransfer002
     * @tc.desc     : interrupt transfer cancel failed.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it("testUsbCancelTransfer001", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbCancelTransfer005 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbCancelTransfer004 No device is connected');
          expect(isDeviceConnected).assertFalse();
          done()
        }
        let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
        if (devicesList.length == 0) {
          hilog.info(domain, tag, 'device list is empty');
        }
        let devices: usbManager.USBDevice = devicesList[0];
        usbManager.requestRight(devices.name);
        let gPipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
        let endpoint = devices.configs[0].interfaces[0]?.endpoints.find((value) => {
          return value.direction === usbManager.USBRequestDirection.USB_REQUEST_DIR_TO_DEVICE;//  && value.type === 2
        })
        let flags: usbManager.UsbTransferFlags = usbManager.UsbTransferFlags.USB_TRANSFER_FREE_BUFFER;
        let type: usbManager.UsbEndpointTransferType = usbManager.UsbEndpointTransferType.TRANSFER_TYPE_INTERRUPT;
        let transferParams: usbManager.UsbDataTransferParams = getTransfersParam(gPipe, flags, 129, type, 2000);
        // let err = 0;
        try {
          usbManager.usbCancelTransfer(transferParams);
          expect().assertFail();
          hilog.info(domain, tag, 'USB transfer request submitted.');
        } catch (error: BusinessError) {
          hilog.info(domain, tag, 'interrupt cancel failed:' + error.code);
          expect(error.code).assertEqual(14400011);
        }
        done();
      })

    it("test_usbControlTransfer01", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb usbControlTransfer begin');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb not connected');
        }
        let param: usbManager.USBDeviceRequestParams = {
          bmRequestType: 0x80,
          bRequest: 0x06,
          wValue: (0x01 << 8) | 0x00,
          wIndex: 0x00,
          wLength: 18,
          data: new Uint8Array(18),
        };
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          if (devicesList.length == 0) {
            hilog.info(domain, tag, 'device list is empty');
          }

          usbManager.requestRight(devicesList[0].name);
          hilog.info(domain, tag, `usbControlTransfer name = ${devicesList[0].name}`);
          let devicePipe: usbManager.USBDevicePipe = usbManager.connectDevice(devicesList[0]);
          let timeout: int | undefined = 30;

          hilog.info(domain, tag, `usbControlTransfer device.name = ${devicesList[0].name}`);
          let ret = usbManager.usbControlTransfer(devicePipe, param, timeout).then((ret: int) => {
            hilog.info(domain, tag, `usbControlTransfer size= ${ret}`);
            expect(ret == 18).assertEqual(true);
          })
          usbManager.closePipe(devicePipe);
        } catch (error: BusinessError) {
          hilog.info(0, 'testTag ui', `closeAccessory error `)
          hilog.info(domain, tag, 'usbControlTransfer end');
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, `usbControlTransferlnn0 err.code= ${err.code}`);
        }
        done();
      })

    it("test_bulkTransfer02", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb test_bulkTransfer02 begin');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb not connected');
        }

        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          if (devicesList.length == 0) {
            hilog.info(domain, tag, 'device list is empty');
          }

          let device: usbManager.USBDevice = devicesList[0];
          usbManager.requestRight(device.name);

          let devicePipe: usbManager.USBDevicePipe = usbManager.connectDevice(device);
          hilog.info(domain, tag, `usbManager.connectDevice(device) = ${devicePipe.busNum}`);
          hilog.info(domain, tag, `usbManager.connectDevice(device) = ${devicePipe.devAddress}`);
          let interfaces: usbManager.USBInterface = device.configs[0].interfaces[0];
          let endpoint: usbManager.USBEndpoint =
            device.configs[0].interfaces[0].endpoints[0]; //read 0x81 光驱  ; 1 U盘 write
          hilog.info(domain, tag, `usbManager.USBEndpoint = ${endpoint}`);
          let ret: int = usbManager.claimInterface(devicePipe, interfaces, true);
          hilog.info(domain, tag, `bulkTransfer begin  = ${ret}`);
          if (ret == 0) {
            hilog.info(domain, tag, 'USB claim sucess');
            let bufferTransfer = new Uint8Array(128);

            hilog.info(domain, tag, `test_bulkTransfer name = ${devicesList[0].name}`);
            hilog.info(domain, tag, `test_bulkTransfer buffer = ${bufferTransfer}`);
            let timeout: int | undefined = 8000;

            usbManager.bulkTransfer(devicePipe, endpoint, bufferTransfer, timeout).then((retVal: int) => {
              hilog.info(domain, tag, `test_bulkTransfer02 retVal = ${retVal}`);
              expect(retVal == 128).assertTrue();
            });
          }
          hilog.info(domain, tag, `bulkTransfer end `);
          usbManager.closePipe(devicePipe);
        } catch (error: BusinessError) {
          hilog.info(domain, tag, `bulkTransfer error `)
          hilog.info(domain, tag, `bulkTransfer err.code= ${error.code}`);
        }
        done();
      })
  })
}