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

function devAccessoryFunc() {
  if (accList.length > 0) {
    hilog.info(domain, tag, "This USB device is accessory function");
    return true;
  }
  hilog.info(domain, tag, "This USB device is accessory function");
  return false;
}

let domain: number = 0x0000;
let tag: string = 'testTag';

const PARAM_NULLSTRING: string = "";

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
      // await Utils.msSleep(2000)
      // abilityDelegator.executeShellCommand("aa start -a EntryAbility -b com.example.helloworld")
      // await Utils.msSleep(2000)
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
    /**
     * @tc.number   : SUB_USB_HostManager_JS_Func_0500
     * @tc.name     : testGetDevices001
     * @tc.desc     : Positive test: Get device list
     * @tc.desc     : 【C-ALL-HARDWARE-0502】必须支持连接标准 USB 外围设备
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testGetDevices001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb testGetDevices001 begin');
        let getDeviceList: Array<usbManager.USBDevice> = usbManager.getDevices();
        hilog.info(domain, tag, 'usb case getDevices ret length: ' + getDeviceList.length);
        expect(getDeviceList.length > 0).assertTrue();
        done();
      })


    it("testGetPortListNormalApp002", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_getPortList_001');
        try {
          let portList = usbManager.getPortList();
          hilog.info(domain, tag, 'abilityTest_getPortList_001 success: %{public}s', JSON.stringify(portList));
          expect(Array.isArray(portList)).assertTrue();
          expect(portList.length > 0).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'usb testGetPortListNormalApp001 catch err code: ', err.code, ', message: ',
            err.message);
          expect(err.code).assertEqual(202);
        }
        done();
      })

    it("abilityTest_getRawDescriptorRight_003", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB getRawDescriptor begin');
        try {
          hilog.info(domain, tag, 'USB getRawDescriptor begin 1');
          let hasRight = usbManager.requestRight(devices.name);
          hilog.info(domain, tag, 'USB getRawDescriptor begin 2');
          hilog.info(domain, tag, 'USB getRawDescriptor begin 2' + devices.name);
          if (!hasRight) {
            hilog.info(domain, tag, 'requestRight true');
          }
          gPipe = usbManager.connectDevice(devices);
          let devs = usbManager.getRawDescriptor(gPipe);
          hilog.info(domain, tag, `getRawDescriptor busNum = ${gPipe.busNum}`);
          hilog.info(domain, tag, `getRawDescriptor evAddress = ${gPipe.devAddress}`);
          hilog.info(domain, tag, `usb getRawDescriptor success : ret :` + devs);
          expect(devs instanceof Uint8Array).assertTrue();
          expect(devs.length > 0).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'usb testGetPortListNormalApp001 catch err code: ', err.code, ', message: ',
            err.message);
        }
        done();
      })

    it("abilityTest_getPortSupportModes_004", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_getPortSupportModes_004');
        try {
          hilog.info(domain, tag, 'api enter');
          let pid = 1;
          let devs = usbManager.getPortSupportModes(pid);
          hilog.info(domain, tag, `usb abilityTest_getPortSupportModes_004 success : ret :` + devs);
        } catch (err) {
          hilog.info(domain, tag, 'abilityTest_getPortSupportModes_004 catch err code:%{public}s', JSON.stringify(err));
        }
        done();
      })

    it("abilityTest_claimInterface_005", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB abilityTest_claimInterface_006 begin');
        try {
          usbManager.requestRight(devices.name);
          let devicePipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
          let interfaces: usbManager.USBInterface = devices.configs[0].interfaces[0];


          hilog.info(domain, tag, `abilityTest_claimInterface_006 busNum = ${devicePipe.busNum}`);
          hilog.info(domain, tag, `abilityTest_claimInterface_006 busNum = ${devicePipe.devAddress}`);
          hilog.info(domain, tag, 'abilityTest_claimInterface_006 interfaces = %{public}s', JSON.stringify(interfaces));
          let ret: int = usbManager.claimInterface(devicePipe, interfaces, true);
          usbManager.releaseInterface(devicePipe, interfaces);
          hilog.info(domain, tag, `claimInterface = ${ret}`);
          expect(ret === 0).assertTrue();
        } catch (err) {
          hilog.info(domain, tag, 'abilityTest_claimInterface_004 catch err code:%{public}s', JSON.stringify(err));
        }
        done();
      })

    it("abilityTest_releaseInterface_006", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_releaseInterface_007');
        try {
          usbManager.requestRight(devices.name);
          let devicePipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
          let interfaces: usbManager.USBInterface = devices.configs[0].interfaces[0];

          hilog.info(domain, tag, `abilityTest_releaseInterface_007 busNum = ${devicePipe.busNum}`);
          hilog.info(domain, tag, `abilityTest_releaseInterface_007 busNum = ${devicePipe.devAddress}`);
          hilog.info(domain, tag, 'abilityTest_releaseInterface_007 interfaces = %{public}s',
            JSON.stringify(interfaces));

          usbManager.claimInterface(devicePipe, interfaces);
          let ret = usbManager.releaseInterface(devicePipe, interfaces);
          hilog.info(domain, tag, `releaseInterface = ${ret}`);
          expect(ret === 0).assertTrue();
        } catch (err) {
          hilog.info(domain, tag, 'abilityTest_releaseInterface_005 catch err code:%{public}s', JSON.stringify(err));
        }
        done();
      })

    it("abilityTest_setInterface_007", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_setInterface_008');
        try {
          usbManager.requestRight(devices.name);
          let devicePipe = usbManager.connectDevice(devices);
          let interfaces = devices.configs[0].interfaces[0];
          hilog.info(domain, tag, `abilityTest_setInterface_008 busNum = ${devicePipe.busNum}`);
          hilog.info(domain, tag, `abilityTest_setInterface_008 busNum = ${devicePipe.devAddress}`);
          hilog.info(domain, tag, 'abilityTest_setInterface_008 interfaces = %{public}s',
            JSON.stringify(interfaces));
          usbManager.claimInterface(devicePipe, interfaces);
          let ret = usbManager.setInterface(devicePipe, interfaces);
          usbManager.releaseInterface(devicePipe, interfaces);
          hilog.info(domain, tag, `releaseInterface = ${ret}`);
          expect(ret === 0).assertTrue();
        } catch (err) {
          hilog.info(domain, tag, 'abilityTest_setInterface_006 catch err code:%{public}s', JSON.stringify(err));
        }
        done();
      })

    it("abilityTest_setConfiguration_008", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_setConfiguration_009');
        try {
          usbManager.requestRight(devices.name);
          let devicePipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
          let config: usbManager.USBConfiguration = devices.configs[0];
          hilog.info(domain, tag, 'abilityTest_setConfiguration_009 interfaces = %{public}s',
            JSON.stringify(config));
          let ret: int = usbManager.setConfiguration(devicePipe, config);
          hilog.info(domain, tag, `setConfiguration = ${ret}`);
          expect(ret === 0).assertTrue();
        } catch (err) {
          hilog.info(domain, tag, 'abilityTest_setConfiguration_007 catch err code:%{public}s', JSON.stringify(err));
        }
        done();
      })

    it("abilityTest_getFileDescriptor_009", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_getFileDescriptor_0010');
        try {
          usbManager.requestRight(devices.name);
          let isHasRight = usbManager.hasRight(devices.name);
          gPipe = usbManager.connectDevice(devices);
          let devs: int = usbManager.getFileDescriptor(gPipe);
          hilog.info(domain, tag, `usb abilityTest_getFileDescriptor_0010 success ret :` + devs);
          expect(devs >= 0).assertTrue();
        } catch (err) {
          hilog.info(domain, tag, 'abilityTest_getFileDescriptor_008 catch err code:%{public}s', JSON.stringify(err));
        }
        done();
      })

    it("test_hasRight_0010", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB test_hasRight_13 begin');
        try {
          let isHasRight = usbManager.hasRight(devices.name);
          hilog.info(domain, tag, 'usb case hasRight ret : ' + isHasRight);
          expect(isHasRight).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'usb test_hasRight_13 catch err code: ', err.code, ', message: ',
            err.message);
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("test_requestRight_0011", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'test_requestRight_14');
        try {
          let isHasRight = await usbManager.requestRight(devices.name);
          hilog.info(domain, tag, 'usb case test_requestRight_14 ret : ' + isHasRight);
          expect(isHasRight).assertTrue();
        } catch (err) {
          hilog.info(domain, tag, 'test_requestRight_14 catch err code: ' + JSON.stringify(err));
          expect(err !== null).assertFalse();
        }
        done();
      })

    it("test_hasRight_0012", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB test_hasRight_15 begin');
        try {
          for (let i = 0; i < gDeviceList.length; i++) {
            let deviceName = gDeviceList[i].name;
            deviceName = deviceName + "123";
            let isHasRight = usbManager.hasRight(deviceName);
            hilog.info(domain, tag, 'usb [', deviceName, '] hasRight ret : ' + isHasRight);
            expect(isHasRight).assertFalse();
          }
        } catch (err) {
          hilog.info(domain, tag, 'test_hasRight_15 catch err : ' + JSON.stringify(err));
          expect(err !== null).assertFalse();
        }
        done();
      })

    it("test_removeRight0013", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'test_removeRight17');
        try {
          let remRight = usbManager.removeRight(devices.name);
          hilog.info(domain, tag, 'usb case test_removeRight17 ret : ' + remRight);
          expect(remRight).assertTrue();
        } catch (err) {
          hilog.info(domain, tag, 'test_removeRight17 catch err code: ' + JSON.stringify(err));
          expect(err !== null).assertFalse();
        }
        done();
      })

    it("test_connectDevice0014", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB test_connectDevice18 begin');
        try {
          let deviceName = devices.name;
          let isHasRight: boolean = await usbManager.requestRight(deviceName);
          if (isHasRight) {
            hilog.info(domain, tag, 'requestRight is true');
          }
          gPipe = usbManager.connectDevice(devices);
          hilog.info(domain, tag, 'test_connectDevice18 success  gPipe.busNum : ' + gPipe.busNum);
          hilog.info(domain, tag, 'test_connectDevice18 success gPipe.devAddress : ' + gPipe.devAddress);
          expect(gPipe.busNum > 0).assertTrue();
          expect(gPipe.devAddress > 0).assertTrue();

          expect(gPipe.busNum < 256).assertTrue();
          expect(gPipe.devAddress < 128).assertTrue();
        } catch (err) {
          hilog.info(domain, tag, 'test_connectDevice18 catch err code: ' + JSON.stringify(err));
          expect(err !== null).assertFalse();
        }
        done();
      })

    it("tes_addDeviceAccessRight0015", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB tes_addDeviceAccessRight19 begin');
        try {
          let isHasRight = await usbManager.requestRight(devices.name);
          let bundleFlags = bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION;
          let data = bundleManager.getBundleInfoForSelfSync(bundleFlags);
          hilog.info(domain, tag, 'getBundleInfoForSelfSync: ' + JSON.stringify(data));
          let tokenId = data.appInfo.accessTokenId;

          hilog.info(domain, tag, 'accessTokenId: ' + JSON.stringify(tokenId));
          let ret = usbManager.addDeviceAccessRight(JSON.stringify(tokenId), devices.name);
          if (ret) {
            hilog.info(domain, tag, 'tes_addDeviceAccessRight19 success:' + ret);
          } else {
            hilog.info(domain, tag, 'tes_addDeviceAccessRight19 failed:' + ret);
          }
          expect(ret).assertTrue();
        } catch (err) {
          hilog.info(domain, tag, 'tes_addDeviceAccessRight19 catch err code: ' + JSON.stringify(err));
          expect(err !== null).assertFalse();
        }
        done();
      })


    it("test_getFunctionsFromString0016", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB test_getFunctionsFromString20 begin');
        try {
          let ret = usbManager.getFunctionsFromString('acm');
          if (ret) {
            hilog.info(domain, tag, 'test_getFunctionsFromString20 success:' + ret);
          } else {
            hilog.info(domain, tag, 'test_getFunctionsFromString20 failed:' + ret);
          }
          expect(ret === 1).assertTrue();
        } catch (err) {
          hilog.info(domain, tag, 'test_getFunctionsFromString20 catch err code: ' + JSON.stringify(err));
        }
        done();
      })


    it("test_getStringFromFunctions0017", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'test_getStringFromFunctions21');
        try {
          let ret = usbManager.getStringFromFunctions(usbManager.FunctionType.ACM);
          if (ret) {
            hilog.info(domain, tag, 'getStringFromFunctions success:' + ret);
          } else {
            hilog.info(domain, tag, 'getStringFromFunctions failed:' + ret);
          }
          expect(ret).assertEqual('acm');
        } catch (err) {
          hilog.info(domain, tag, 'test_getStringFromFunctions catch err code: ' + JSON.stringify(err));
          expect(err !== null).assertFalse();
        }
        done();
      })


    it("test_getDeviceFunctions0018", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB test_getDeviceFunctions23 begin');
        try {
          let ret = usbManager.getDeviceFunctions();
        } catch (err) {
          hilog.info(domain, tag, 'test_getDeviceFunctions catch err code: ' + JSON.stringify(err));
          expect(err !== null).assertFalse();
        }
        done();
    })


    /**
     * @tc.number   : SUB_USB_HostManager_JS_Func_0700
     * @tc.name     : testGetRawDescriptor001
     * @tc.desc     : Positive test: Get the original USB descriptor
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testGetRawDescriptor0019', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, 'usb testGetRawDescriptor001 begin');

      getPipe('testGetRawDescriptor001');
      let descriptor = usbManager.getRawDescriptor(gPipe);
      hilog.info(domain, tag, 'usb case testGetRawDescriptor001 ret: ' + JSON.stringify(descriptor));
      expect(descriptor.length).assertLarger(0);
      toClosePipe('testGetRawDescriptor001');
    })

    it("test_setDeviceFunctions20", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb testsetDeviceFunctionsSync001 begin');
        try {

          let ret = usbManager.setDeviceFunctions(usbManager.FunctionType.ACM);
          if (ret) {
            hilog.info(domain, tag, 'setDeviceFunctions success:' + ret);
          } else {
            hilog.info(domain, tag, 'setDeviceFunctions failed:' + ret);
          }

        } catch (err) {
          hilog.info(domain, tag, 'test_setDeviceFunctions catch err code: ' + JSON.stringify(err));
          expect(err !== null).assertFalse();
        }
        hilog.info(domain, tag, 'usb test_setDeviceFunctions end');
        done();
      })
  })
}