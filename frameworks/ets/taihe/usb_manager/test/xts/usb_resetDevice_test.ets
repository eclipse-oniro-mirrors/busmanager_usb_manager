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
    it("test_resetUsbDevice001", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, ' resetUsbDevice begin');

        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          if (devicesList.length == 0) {
            hilog.info(domain, tag, 'device list is empt');
          }

          usbManager.requestRight(devicesList[0].name);
          let devicePipe: usbManager.USBDevicePipe = usbManager.connectDevice(devicesList[0]);
          let ret: boolean = usbManager.resetUsbDevice(devicePipe);
          hilog.info(domain, tag, `resetUsbDevice ret =${ret}`);
          expect(ret == true).assertTrue();
          let ret0: int = usbManager.closePipe(devicePipe);
        } catch (err) {
          hilog.info(domain, tag, '%{public}s', 'test_resetUsbDevice001');
        }
        hilog.info(domain, tag, 'usb test_resetUsbDevice end');
        done();
    })
  })
}