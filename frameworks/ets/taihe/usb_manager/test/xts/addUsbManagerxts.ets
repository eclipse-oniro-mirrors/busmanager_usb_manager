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

let isDeviceConnected: boolean;
let tmpPipe: usbManager.USBDevicePipe;
let iface: usbManager.USBInterface;
let config: usbManager.USBConfiguration;
let accessory: usbManager.USBAccessory;
let powerRole: usbManager.PowerRoleType;
let dataRole: usbManager.DataRoleType;
let param: usbManager.USBDeviceRequestParams;
let isDevAccessoryFunc: boolean;
let accessInfo: usbManager.USBAccessory;
let accessInfoTest: usbManager.USBAccessory = {
  manufacturer: 'manufacturer',
  product: 'product',
  description: 'description',
  version: 'version',
  serialNumber: 'serialNumber'
}

function devAccessoryFunc() {
  let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
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
  let gDeviceList: Array<Readonly<usbManager.USBDevice>> = usbManager.getDevices();
  if (gDeviceList.length > 0) {
    hilog.info(domain, tag, "Test USB device is connected");
    return true;
  }
  hilog.info(domain, tag, "Test USB device is not connected");
  return false;
}

function getPipe(testCaseName: string) {
  if (isDeviceConnected) {
    let gDeviceList: Array<Readonly<usbManager.USBDevice>> = usbManager.getDevices();
    let devices: usbManager.USBDevice = gDeviceList[0];
    let gPipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
    gPipe = usbManager.connectDevice(devices);
    hilog.info(domain, tag, `usb ${testCaseName} connectDevice getPipe ret: ${JSON.stringify(gPipe)}`);
    expect(gPipe !== null).assertTrue();
  }
  hilog.info(domain, tag, "connectDevice error");
}

function toClosePipe(testCaseName: string) {
  let gDeviceList: Array<Readonly<usbManager.USBDevice>> = usbManager.getDevices();
  let devices: usbManager.USBDevice = gDeviceList[0];
  let gPipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
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


      let gDeviceList = usbManager.getDevices();
      hilog.info(domain, tag, '获取设备信息: %{public}s', JSON.stringify(gDeviceList));

      isDeviceConnected = deviceConnected();

      if (isDeviceConnected) {
        let gDeviceList: Array<Readonly<usbManager.USBDevice>> = usbManager.getDevices();
        let devices: usbManager.USBDevice = gDeviceList[0];
        let hasRight = usbManager.hasRight(devices.name);
        if (!hasRight) {
          hilog.info(domain, tag, `beforeAll: usb requestRight start`);
        }
      }

      isDevAccessoryFunc = devAccessoryFunc();
      if (isDevAccessoryFunc) {
        let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
        accessInfo = accList[0];
        hilog.info(domain, tag, 'beforeAll ret : ', JSON.stringify(accList));
      }
    })
    it("testHasAccessoryRight001", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testHasAccessoryRight001---------------------------');
      if (!isDevAccessoryFunc) {
        expect(isDevAccessoryFunc).assertFalse();
        return
      }
      try {
        let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = usbManager.hasAccessoryRight(accessInfoTest);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'usb testHasAccessoryRight001 catch err code: ', err.code, ', message: ',
          err.message);
        expect(err.code).assertEqual(14400005);
        expect(err.code != 401).assertEqual(true);
      }
    })

    it("abilityTest_addAccessoryRight_002", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB abilityTest_addAccessoryRight_002 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let accList: usbManager.USBAccessory[] = usbManager.getAccessoryList()
          let bundleFlags = bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION;
          let data = bundleManager.getBundleInfoForSelfSync(bundleFlags);
          hilog.info(domain, tag, 'getBundleInfoForSelfSync: ' + JSON.stringify(data));
          let tokenId = Math.floor(data.appInfo.accessTokenId) as int;
          usbManager.addAccessoryRight(tokenId, accList[0])
          hilog.info(0, 'testTag ui', `addAccessoryRight success`)
        } catch (error) {
          hilog.info(0, 'testTag ui', `addAccessoryRight error `)
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
    it('testGetRawDescriptor003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, 'usb testGetRawDescriptor003 begin');
      if (!isDeviceConnected) {
        expect(isDeviceConnected).assertFalse();
      }
      let gDeviceList: Array<Readonly<usbManager.USBDevice>> = usbManager.getDevices();
      let devices: usbManager.USBDevice = gDeviceList[0];
      let gPipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
      getPipe('testGetRawDescriptor001');
      let descriptor = usbManager.getRawDescriptor(gPipe);
      hilog.info(domain, tag, 'usb case testGetRawDescriptor001 ret: ' + JSON.stringify(descriptor));
      expect(descriptor.length).assertLarger(0);
      toClosePipe('testGetRawDescriptor001');
    })

    it("USBRequestTargetType004", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL0, async (done: () => void): Promise<void> => {
      hilog.info(domain, tag, '%{public}s', '----------------------USBRequestTargetType004---------------------------');
      hilog.info(domain, tag, `USB_REQUEST_DIR_TO_DEVICE: ${usbManager.USBRequestTargetType.USB_REQUEST_TARGET_DEVICE}`);
      hilog.info(domain, tag, `USB_REQUEST_DIR_TO_DEVICE: ${usbManager.USBRequestTargetType.USB_REQUEST_TARGET_INTERFACE}`);
      hilog.info(domain, tag, `USB_REQUEST_DIR_TO_DEVICE: ${usbManager.USBRequestTargetType.USB_REQUEST_TARGET_ENDPOINT}`);
      hilog.info(domain, tag, `USB_REQUEST_DIR_TO_DEVICE: ${usbManager.USBRequestTargetType.USB_REQUEST_TARGET_OTHER}`);
      done()
      hilog.info(domain, tag, '%{public}s', 'USBRequestTargetType001 end');
    })

    it("testUsbSubmitTransfer005", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer005 enter');
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
            hilog.info(domain, tag, 'callBackData = ' + JSON.stringify(callBackData));
            hilog.info(domain, tag,
              'testUsbSubmitTransfer003 transfer success, result = ' + transferParams.buffer.toString());
            hilog.info(domain, tag, 'status = ' + callBackData?.status);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_COMPLETED).assertEqual(false);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_ERROR).assertEqual(false);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_TIMED_OUT).assertEqual(true);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_CANCELED).assertEqual(false);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_STALL).assertEqual(false);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_NO_DEVICE).assertEqual(false);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_OVERFLOW).assertEqual(false);
            const descs = callBackData?.isoPacketDescs;
            if (descs && descs.length) {
              for (let j = 0; j < descs.length; j++) {
                const i = descs[j];
                hilog.info(domain, tag, 'isoPacketDescs length = ' + i.length);
                hilog.info(domain, tag, 'isoPacketDescs actualLength = ' + i.actualLength);
                hilog.info(domain, tag, 'isoPacketDescs status = ' + i.status);
              }
            }
            usbManager.closePipe(gPipe);
            hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer003 end');
            expect(flags!= usbManager.UsbTransferFlags.USB_TRANSFER_SHORT_NOT_OK).assertEqual(true);
            expect(flags!= usbManager.UsbTransferFlags.USB_TRANSFER_FREE_TRANSFER).assertEqual(true);
            expect(flags!= usbManager.UsbTransferFlags.USB_TRANSFER_ADD_ZERO_PACKET).assertEqual(true);
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

    it("test_usbControlTransfer06", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb test_usbControlTransfer06 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
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

          hilog.info(domain, tag, `usbControlTransfer device.name = ${devicesList[0].name}`);
          let ret = usbManager.usbControlTransfer(devicePipe, param).then((ret: int) => {
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

    it("test_bulkTransfer07", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb test_bulkTransfer07 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
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

            usbManager.bulkTransfer(devicePipe, endpoint, bufferTransfer).then((retVal: int) => {
              hilog.info(domain, tag, `test_bulkTransfer02 retVal = ${retVal}`);
              expect(retVal == 128).assertTrue();
            });
          }
          let ret0 = usbManager.releaseInterface(devicePipe, interfaces);
          console.log(`releaseInterface = ${ret0}`);
          hilog.info(domain, tag, `bulkTransfer end `);
          usbManager.closePipe(devicePipe);
        } catch (error: BusinessError) {
          hilog.info(domain, tag, `bulkTransfer error `)
          hilog.info(domain, tag, `bulkTransfer err.code= ${error.code}`);
        }
        done();
      })

    it("test_hasRight_008", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB test_hasRight_008 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          let isHasRight = usbManager.hasRight(devices.name);
          hilog.info(domain, tag, 'usb case hasRight ret : ' + isHasRight);
          expect(isHasRight).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'usb test_hasRight_13 catch err code: ', err.code, ', message: ',
            err.message);
          expect(err.code).assertEqual(401);
        }
        done();
      })


    it("test_connectDevice009", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB test_connectDevice009 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          let deviceName = devices.name;
          let isHasRight: boolean = await usbManager.requestRight(deviceName);
          if (isHasRight) {
            hilog.info(domain, tag, 'requestRight is true');
          }
          let gPipe = usbManager.connectDevice(devices);
          hilog.info(domain, tag, 'test_connectDevice18 success  gPipe.busNum : ' + gPipe.busNum);
          hilog.info(domain, tag, 'test_connectDevice18 success gPipe.devAddress : ' + gPipe.devAddress);
          expect(gPipe.busNum > 0).assertTrue();
          expect(gPipe.devAddress > 0).assertTrue();

          expect(gPipe.busNum < 256).assertTrue();
          expect(gPipe.devAddress < 128).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          expect(err.code).assertEqual(401);
        }
        done();
      })

    it("test_connectDevice010", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB test_connectDevice010 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          let deviceName = devices.name;
          let isHasRight: boolean = await usbManager.requestRight(deviceName);
          if (isHasRight) {
            hilog.info(domain, tag, 'requestRight is true');
          }
          let gPipe = usbManager.connectDevice(devices);
          hilog.info(domain, tag, 'test_connectDevice18 success  gPipe.busNum : ' + gPipe.busNum);
          hilog.info(domain, tag, 'test_connectDevice18 success gPipe.devAddress : ' + gPipe.devAddress);
          expect(gPipe.busNum > 0).assertTrue();
          expect(gPipe.devAddress > 0).assertTrue();

          expect(gPipe.busNum < 256).assertTrue();
          expect(gPipe.devAddress < 128).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("tes_addDeviceAccessRight011", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB tes_addDeviceAccessRight011 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
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
        } catch (error) {
          const err = error as BusinessError;
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("tes_addDeviceAccessRight012", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB tes_addDeviceAccessRight012 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
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
        } catch (error) {
          const err = error as BusinessError;
          expect(err.code).assertEqual(401);
        }
        done();
      })

    it("test_removeRight013", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'test_removeRight013');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          let remRight = usbManager.removeRight(devices.name);
          hilog.info(domain, tag, 'usb case test_removeRight17 ret : ' + remRight);
          expect(remRight).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          expect(err.code).assertEqual(401);
        }
        done();
      })

    it("test_removeRight014", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'test_removeRight014');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          let remRight = usbManager.removeRight(devices.name);
          hilog.info(domain, tag, 'usb case test_removeRight17 ret : ' + remRight);
          expect(remRight).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("abilityTest_claimInterface_015", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB abilityTest_claimInterface_015 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          usbManager.requestRight(devices.name);
          let devicePipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
          let interfaces: usbManager.USBInterface = devices.configs[0].interfaces[0];

          hilog.info(domain, tag, `abilityTest_claimInterface_006 busNum = ${devicePipe.busNum}`);
          hilog.info(domain, tag, `abilityTest_claimInterface_006 busNum = ${devicePipe.devAddress}`);
          hilog.info(domain, tag, 'abilityTest_claimInterface_006 interfaces = %{public}s', JSON.stringify(interfaces));
          let ret: int = usbManager.claimInterface(devicePipe, interfaces, false);
          usbManager.releaseInterface(devicePipe, interfaces);
          hilog.info(domain, tag, `claimInterface = ${ret}`);
          expect(ret === 0).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'abilityTest_claimInterface_004 catch err code:%{public}s', JSON.stringify(err));
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("abilityTest_claimInterface_016", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB abilityTest_claimInterface_016 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          usbManager.requestRight(devices.name);
          let devicePipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
          let interfaces: usbManager.USBInterface = devices.configs[0].interfaces[0];

          hilog.info(domain, tag, `abilityTest_claimInterface_006 busNum = ${devicePipe.busNum}`);
          hilog.info(domain, tag, `abilityTest_claimInterface_006 busNum = ${devicePipe.devAddress}`);
          hilog.info(domain, tag, 'abilityTest_claimInterface_006 interfaces = %{public}s', JSON.stringify(interfaces));
          let ret: int = usbManager.claimInterface(devicePipe, interfaces, false);
          usbManager.releaseInterface(devicePipe, interfaces);
          hilog.info(domain, tag, `claimInterface = ${ret}`);
          expect(ret === 0).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'abilityTest_claimInterface_004 catch err code:%{public}s', JSON.stringify(err));
          expect(err.code).assertEqual(401);
        }
        done();
      })

    it("abilityTest_releaseInterface_017", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_releaseInterface_017');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
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
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'abilityTest_releaseInterface_005 catch err code:%{public}s', JSON.stringify(err));
          expect(err.code).assertEqual(401);
        }
        done();
      })

    it("abilityTest_releaseInterface_018", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_releaseInterface_018');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
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
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'abilityTest_releaseInterface_005 catch err code:%{public}s', JSON.stringify(err));
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("abilityTest_setConfiguration_019", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_setConfiguration_019');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          usbManager.requestRight(devices.name);
          let devicePipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
          let config: usbManager.USBConfiguration = devices.configs[0];
          hilog.info(domain, tag, 'abilityTest_setConfiguration_009 interfaces = %{public}s',
            JSON.stringify(config));
          let ret: int = usbManager.setConfiguration(devicePipe, config);
          hilog.info(domain, tag, `setConfiguration = ${ret}`);
          expect(ret === 0).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'abilityTest_setConfiguration_007 catch err code:%{public}s', JSON.stringify(err));
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("abilityTest_setConfiguration_020", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_setConfiguration_020');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          usbManager.requestRight(devices.name);
          let devicePipe: usbManager.USBDevicePipe = usbManager.connectDevice(devices);
          let config: usbManager.USBConfiguration = devices.configs[0];
          hilog.info(domain, tag, 'abilityTest_setConfiguration_009 interfaces = %{public}s',
            JSON.stringify(config));
          let ret: int = usbManager.setConfiguration(devicePipe, config);
          hilog.info(domain, tag, `setConfiguration = ${ret}`);
          expect(ret === 0).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'abilityTest_setConfiguration_007 catch err code:%{public}s', JSON.stringify(err));
          expect(err.code).assertEqual(401);
        }
        done();
      })

    it("abilityTest_setInterface_021", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_setInterface_021');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
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
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'abilityTest_setInterface_006 catch err code:%{public}s', JSON.stringify(err));
          expect(err.code).assertEqual(401);
        }
        done();
      })

    it("abilityTest_setInterface_022", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_setInterface_022');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
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
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'abilityTest_setInterface_006 catch err code:%{public}s', JSON.stringify(err));
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("abilityTest_getRawDescriptorRight_023", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB abilityTest_getRawDescriptorRight_023 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          hilog.info(domain, tag, 'USB getRawDescriptor begin 1');
          let hasRight = usbManager.requestRight(devices.name);
          hilog.info(domain, tag, 'USB getRawDescriptor begin 2');
          hilog.info(domain, tag, 'USB getRawDescriptor begin 2' + devices.name);
          if (!hasRight) {
            hilog.info(domain, tag, 'requestRight true');
          }
          let gPipe = usbManager.connectDevice(devices);
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
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("abilityTest_getRawDescriptorRight_024", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB abilityTest_getRawDescriptorRight_024 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          hilog.info(domain, tag, 'USB getRawDescriptor begin 1');
          let hasRight = usbManager.requestRight(devices.name);
          hilog.info(domain, tag, 'USB getRawDescriptor begin 2');
          hilog.info(domain, tag, 'USB getRawDescriptor begin 2' + devices.name);
          if (!hasRight) {
            hilog.info(domain, tag, 'requestRight true');
          }
          let gPipe = usbManager.connectDevice(devices);
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
          expect(err.code).assertEqual(401);
        }
        done();
      })

    it("abilityTest_getFileDescriptor_025", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_getFileDescriptor_025');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          usbManager.requestRight(devices.name);
          let isHasRight = usbManager.hasRight(devices.name);
          let gPipe = usbManager.connectDevice(devices);
          let devs: int = usbManager.getFileDescriptor(gPipe);
          hilog.info(domain, tag, `usb abilityTest_getFileDescriptor_0010 success ret :` + devs);
          expect(devs >= 0).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'abilityTest_getFileDescriptor_008 catch err code:%{public}s', JSON.stringify(err));
          expect(err.code).assertEqual(401);
        }
        done();
      })

    it("abilityTest_getFileDescriptor_026", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'abilityTest_getFileDescriptor_026');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          usbManager.requestRight(devices.name);
          let isHasRight = usbManager.hasRight(devices.name);
          let gPipe = usbManager.connectDevice(devices);
          let devs: int = usbManager.getFileDescriptor(gPipe);
          hilog.info(domain, tag, `usb abilityTest_getFileDescriptor_0010 success ret :` + devs);
          expect(devs >= 0).assertTrue();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'abilityTest_getFileDescriptor_008 catch err code:%{public}s', JSON.stringify(err));
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("test_usbControlTransfer27", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb test_usbControlTransfer27 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
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
          expect(err.code).assertEqual(401);
        }
        done();
      })

    it("test_usbControlTransfer28", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb test_usbControlTransfer28 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
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
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("test_bulkTransfer029", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb test_bulkTransfer029 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
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
              hilog.info(domain, tag, `test_bulkTransfer029 retVal = ${retVal}`);
              expect(retVal == 128).assertTrue();
            });
          }
          let ret0 = usbManager.releaseInterface(devicePipe, interfaces);
          console.log(`releaseInterface = ${ret0}`);
          hilog.info(domain, tag, `bulkTransfer end `);
          usbManager.closePipe(devicePipe);
        } catch (error: BusinessError) {
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, `bulkTransfer error `)
          hilog.info(domain, tag, `bulkTransfer err.code= ${err.code}`);
          expect(err.code).assertEqual(801);
        }
        done();
      })

    it("test_bulkTransfer030", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb test_bulkTransfer030 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
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
              hilog.info(domain, tag, `test_bulkTransfer030 retVal = ${retVal}`);
              expect(retVal == 128).assertTrue();
            });
          }
          let ret0 = usbManager.releaseInterface(devicePipe, interfaces);
          console.log(`releaseInterface = ${ret0}`);
          hilog.info(domain, tag, `bulkTransfer end `);
          usbManager.closePipe(devicePipe);
        } catch (error: BusinessError) {
          hilog.info(domain, tag, `bulkTransfer error `)
          hilog.info(domain, tag, `bulkTransfer err.code= ${error.code}`);
          expect(error.code).assertEqual(401);
        }
        done();
      })

    it("testHasAccessoryRight031", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testHasAccessoryRight031---------------------------');
      if (!isDevAccessoryFunc) {
        expect(isDevAccessoryFunc).assertFalse();
        return
      }
      try {
        let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = usbManager.hasAccessoryRight(accessInfoTest);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'usb testHasAccessoryRight031 catch err code: ', err.code, ', message: ',
          err.message);
        expect(err.code).assertEqual(801);
      }
    })

    it("testHasAccessoryRight032", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testHasAccessoryRight032---------------------------');
      if (!isDevAccessoryFunc) {
        expect(isDevAccessoryFunc).assertFalse();
        return
      }
      try {
        let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = usbManager.hasAccessoryRight(accessInfoTest);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'usb testHasAccessoryRight032 catch err code: ', err.code, ', message: ',
          err.message);
        expect(err.code).assertEqual(14400004);
      }
    })

    it("testHasAccessoryRight033", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testHasAccessoryRight033---------------------------');
      if (!isDevAccessoryFunc) {
        expect(isDevAccessoryFunc).assertFalse();
        return
      }
      try {
        let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = usbManager.hasAccessoryRight(accessInfoTest);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'usb testHasAccessoryRight033 catch err code: ', err.code, ', message: ',
          err.message);
        expect(err.code).assertEqual(14400001);
      }
    })

    it("testRequestAccessoryRight034", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testRequestAccessoryRight034---------------------------');
      if (!isDevAccessoryFunc) {
        expect(isDevAccessoryFunc).assertFalse();
        return
      }
      try {
        let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = await usbManager.requestAccessoryRight(accessInfo);
      } catch (error) {
        const err = error as BusinessError;
        expect(err.code).assertEqual(401);
      }
    })

    it("testRequestAccessoryRight035", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testRequestAccessoryRight035---------------------------');
      if (!isDevAccessoryFunc) {
        hilog.info(domain, tag, 'getAccessoryList is empty : ', );
        return;
      }
      try {
        let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = await usbManager.requestAccessoryRight(accessInfo);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'testRequestAccessoryRight035 err : ', err);
        expect(err.code).assertEqual(801);
      }
    })

    it("testRequestAccessoryRight036", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testRequestAccessoryRight036---------------------------');
      if (!isDevAccessoryFunc) {
        hilog.info(domain, tag, 'getAccessoryList is empty : ', );
        return;
      }
      try {
        let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = await usbManager.requestAccessoryRight(accessInfo);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'testRequestAccessoryRight036 err : ', err);
        expect(err.code).assertEqual(14400004);
      }
    })

    it("testRequestAccessoryRight037", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testRequestAccessoryRight037---------------------------');
      if (!isDevAccessoryFunc) {
        hilog.info(domain, tag, 'getAccessoryList is empty : ', );
        return;
      }
      try {
        let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = await usbManager.requestAccessoryRight(accessInfo);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'testRequestAccessoryRight037 err : ', err);
        expect(err.code).assertEqual(14400005);
      }
    })

    it("testRequestAccessoryRight038", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testRequestAccessoryRight038---------------------------');
      if (!isDevAccessoryFunc) {
        hilog.info(domain, tag, 'getAccessoryList is empty : ', );
        return;
      }
      try {
        let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = await usbManager.requestAccessoryRight(accessInfo);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'testRequestAccessoryRight038 err : ', err);
        expect(err.code).assertEqual(14400001);
      }
    })

    it("testOpenAccessory039", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
        hilog.info(domain, tag, '----------------------testOpenAccessory039---------------------------');
        if (!isDevAccessoryFunc) {
          hilog.info(domain, tag, 'getAccessoryList is empty : ', );
          return;
        }
        try {
          let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
          accessInfo = accList[0];
          await usbManager.requestAccessoryRight(accessInfo);
          let accHandle: usbManager.USBAccessoryHandle = usbManager.openAccessory(accessInfo);
          usbManager.closeAccessory(accHandle);
          usbManager.cancelAccessoryRight(accessInfo);
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'testOpenAccessory039 err : ', err);
          expect(err.code).assertEqual(801);
        }
      })

    it("testOpenAccessory040", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3,() => {
        hilog.info(domain, tag, '----------------------testOpenAccessory040---------------------------');
        if (!isDevAccessoryFunc) {
          hilog.info(domain, tag, 'getAccessoryList is empty : ', );
          return;
        }
        try {
          let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
          accessInfo = accList[0];
          await usbManager.requestAccessoryRight(accessInfo);
          let accHandle: usbManager.USBAccessoryHandle = usbManager.openAccessory(accessInfo);
          usbManager.closeAccessory(accHandle);
          usbManager.cancelAccessoryRight(accessInfo);
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'testOpenAccessory040 err : ', err);
          expect(err.code).assertEqual(401);
        }
      })

    it("testOpenAccessory041", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
        hilog.info(domain, tag, '----------------------testOpenAccessory041---------------------------');
        if (!isDevAccessoryFunc) {
          hilog.info(domain, tag, 'getAccessoryList is empty : ', );
          return;
        }
        try {
          let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
          accessInfo = accList[0];
          await usbManager.requestAccessoryRight(accessInfo);
          let accHandle: usbManager.USBAccessoryHandle = usbManager.openAccessory(accessInfo);
          usbManager.closeAccessory(accHandle);
          usbManager.cancelAccessoryRight(accessInfo);
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'testOpenAccessory041 err : ', err);
          expect(err.code).assertEqual(14400001);
        }
      })

    it("testOpenAccessory042", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
        hilog.info(domain, tag, '----------------------testOpenAccessory042---------------------------');
        if (!isDevAccessoryFunc) {
          hilog.info(domain, tag, 'getAccessoryList is empty : ', );
          return;
        }
        try {
          let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
          accessInfo = accList[0];
          await usbManager.requestAccessoryRight(accessInfo);
          let accHandle: usbManager.USBAccessoryHandle = usbManager.openAccessory(accessInfo);
          usbManager.closeAccessory(accHandle);
          usbManager.cancelAccessoryRight(accessInfo);
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'testOpenAccessory042 err : ', err);
          expect(err.code).assertEqual(14400004);
        }
      })

    it("testOpenAccessory043", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
        hilog.info(domain, tag, '----------------------testOpenAccessory043---------------------------');
        if (!isDevAccessoryFunc) {
          hilog.info(domain, tag, 'getAccessoryList is empty : ', );
          return;
        }
        try {
          let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
          accessInfo = accList[0];
          await usbManager.requestAccessoryRight(accessInfo);
          let accHandle: usbManager.USBAccessoryHandle = usbManager.openAccessory(accessInfo);
          usbManager.closeAccessory(accHandle);
          usbManager.cancelAccessoryRight(accessInfo);
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'testOpenAccessory043 err : ', err);
          expect(err.code).assertEqual(14401001);
        }
      })

    it("testOpenAccessory044", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
        hilog.info(domain, tag, '----------------------testOpenAccessory044---------------------------');
        if (!isDevAccessoryFunc) {
          hilog.info(domain, tag, 'getAccessoryList is empty : ', );
          return;
        }
        try {
          let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
          accessInfo = accList[0];
          await usbManager.requestAccessoryRight(accessInfo);
          let accHandle: usbManager.USBAccessoryHandle = usbManager.openAccessory(accessInfo);
          usbManager.closeAccessory(accHandle);
          usbManager.cancelAccessoryRight(accessInfo);
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'testOpenAccessory044 err : ', err);
          expect(err.code).assertEqual(14401002);
        }
      })

    it("testOpenAccessory045", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
        hilog.info(domain, tag, '----------------------testOpenAccessory045---------------------------');
        if (!isDevAccessoryFunc) {
          hilog.info(domain, tag, 'getAccessoryList is empty : ', );
          return;
        }
        try {
          let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
          accessInfo = accList[0];
          await usbManager.requestAccessoryRight(accessInfo);
          let accHandle: usbManager.USBAccessoryHandle = usbManager.openAccessory(accessInfo);
          usbManager.closeAccessory(accHandle);
          usbManager.cancelAccessoryRight(accessInfo);
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'testOpenAccessory045 err : ', err);
          expect(err.code).assertEqual(14401003);
        }
      })

    it("abilityTest_addAccessoryRight_046", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1, () => {
        hilog.info(domain, tag, 'USB abilityTest_addAccessoryRight_046 begin');
        if (!isDevAccessoryFunc) {
          hilog.info(domain, tag, 'getAccessoryList is empty : ', );
          return;
        }
        try {
          let accList: usbManager.USBAccessory[] = usbManager.getAccessoryList();
          let bundleFlags = bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION;
          let data = bundleManager.getBundleInfoForSelfSync(bundleFlags);
          hilog.info(domain, tag, 'getBundleInfoForSelfSync: ' + JSON.stringify(data));
          let tokenId = Math.floor(data.appInfo.accessTokenId) as int;
          usbManager.addAccessoryRight(tokenId, accList[0])
          hilog.info(0, 'testTag ui', `addAccessoryRight success`)
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(0, 'testTag ui', `addAccessoryRight error `)
          expect(err.code).assertEqual(401);
        }
      })

    it("abilityTest_addAccessoryRight_047", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1, () => {
        hilog.info(domain, tag, 'USB abilityTest_addAccessoryRight_047 begin');
        if (!isDevAccessoryFunc) {
          hilog.info(domain, tag, 'getAccessoryList is empty : ', );
          return;
        }
        try {
          let accList: usbManager.USBAccessory[] = usbManager.getAccessoryList();
          let bundleFlags = bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION;
          let data = bundleManager.getBundleInfoForSelfSync(bundleFlags);
          hilog.info(domain, tag, 'getBundleInfoForSelfSync: ' + JSON.stringify(data));
          let tokenId = Math.floor(data.appInfo.accessTokenId) as int;
          usbManager.addAccessoryRight(tokenId, accList[0])
          hilog.info(0, 'testTag ui', `addAccessoryRight success`)
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(0, 'testTag ui', `addAccessoryRight error `)
          expect(err.code).assertEqual(801);
        }
      })

    it("testOpenAccessory048", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
        hilog.info(domain, tag, '----------------------testOpenAccessory048---------------------------');
        if (!isDevAccessoryFunc) {
          hilog.info(domain, tag, 'getAccessoryList is empty : ', );
          return;
        }
        try {
          let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
          accessInfo = accList[0];
          await usbManager.requestAccessoryRight(accessInfo);
          let accHandle: usbManager.USBAccessoryHandle = usbManager.openAccessory(accessInfo);
          usbManager.closeAccessory(accHandle);
          usbManager.cancelAccessoryRight(accessInfo);
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'testOpenAccessory048 err : ', err);
          expect(err.code).assertEqual(801);
        }
      })

    it("testOpenAccessory049", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
        hilog.info(domain, tag, '----------------------testOpenAccessory049---------------------------');
        if (!isDevAccessoryFunc) {
          hilog.info(domain, tag, 'getAccessoryList is empty : ', );
          return;
        }
        try {
          let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
          accessInfo = accList[0];
          await usbManager.requestAccessoryRight(accessInfo);
          let accHandle: usbManager.USBAccessoryHandle = usbManager.openAccessory(accessInfo);
          usbManager.closeAccessory(accHandle);
          usbManager.cancelAccessoryRight(accessInfo);
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'testOpenAccessory049 err : ', err);
          expect(err.code).assertEqual(401);
        }
      })

    it("testCloseAccessory050", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testCloseAccessory050---------------------------');
      if (!isDevAccessoryFunc) {
        hilog.info(domain, tag, 'getAccessoryList is empty : ', );
        return;
      }
      let accHandle: usbManager.USBAccessoryHandle = {
        accessoryFd: 0
      };
      try {
        usbManager.closeAccessory(accHandle);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'testCloseAccessory050 err : ', err);
        expect(err.code).assertEqual(401);
      }
    })

    it("testCloseAccessory051", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testCloseAccessory051---------------------------');
      if (!isDevAccessoryFunc) {
        hilog.info(domain, tag, 'getAccessoryList is empty : ', );
        return;
      }
      let accHandle: usbManager.USBAccessoryHandle = {
        accessoryFd: 0
      };
      try {
        usbManager.closeAccessory(accHandle);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'testCloseAccessory051 err : ', err);
        expect(err.code).assertEqual(801);
      }
    })

    it("testCloseAccessory052", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testCloseAccessory052---------------------------');
      if (!isDevAccessoryFunc) {
        hilog.info(domain, tag, 'getAccessoryList is empty : ', );
        return;
      }
      let accHandle: usbManager.USBAccessoryHandle = {
        accessoryFd: 0
      };
      try {
        usbManager.closeAccessory(accHandle);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'testCloseAccessory052 err : ', err);
        expect(err.code).assertEqual(14400004);
      }
    })

    it("testUsbSubmitTransfer053", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer053 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbSubmitTransfer053 No device is connected');
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
            hilog.info(domain, tag, 'callBackData = ' + JSON.stringify(callBackData));
            hilog.info(domain, tag,
              'testUsbSubmitTransfer053 transfer success, result = ' + transferParams.buffer.toString());
            hilog.info(domain, tag, 'status = ' + callBackData?.status);
            hilog.info(domain, tag, 'length = ' + callBackData?.actualLength);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_TIMED_OUT).assertEqual(true);
            usbManager.closePipe(gPipe);
            hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer053 end');
            done()
          }
          usbManager.usbSubmitTransfer(transferParams);
        } catch (error) {
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, 'interrupt error:', err.code);
          usbManager.closePipe(gPipe);
          expect(err.code).assertEqual(801);
          done()
        }
      })

    it("testUsbCancelTransfer054", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbCancelTransfer054 enter');
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
          return value.direction === 0 && value.type === 2
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
          hilog.info(domain, tag, 'testUsbCancelTransfer054 catch err code================: ' + JSON.stringify(error));
          expect(error.code).assertEqual(801);
        }
        done();
      })

    it("test_connectDevice055", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB test_connectDevice055 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let devicesList: Array<usbManager.USBDevice> = usbManager.getDevices();
          let devices: usbManager.USBDevice = devicesList[0];
          let deviceName = devices.name;
          let isHasRight: boolean = await usbManager.requestRight(deviceName);
          if (isHasRight) {
            hilog.info(domain, tag, 'requestRight is true');
          }
          let gPipe = usbManager.connectDevice(devices);
          hilog.info(domain, tag, 'test_connectDevice18 success  gPipe.busNum : ' + gPipe.busNum);
          hilog.info(domain, tag, 'test_connectDevice18 success gPipe.devAddress : ' + gPipe.devAddress);
          expect(gPipe.busNum > 0).assertTrue();
          expect(gPipe.devAddress > 0).assertTrue();

          expect(gPipe.busNum < 256).assertTrue();
          expect(gPipe.devAddress < 128).assertTrue();
        } catch (error: BusinessError) {
          expect(error.code).assertEqual(14400001);
        }
        done();
      })

    it("testHasAccessoryRight056", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testHasAccessoryRight056---------------------------');
      try {
        let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = usbManager.hasAccessoryRight(accessInfoTest);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'usb testHasAccessoryRight056 catch err code: ', err.code, ', message: ',
          err.message);
        expect(err.code).assertEqual(14401001);
      }
    })

    it("testOpenAccessory057", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, '----------------------testOpenAccessory057---------------------------');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        try {
          let accList: Array<usbManager.USBAccessory> = usbManager.getAccessoryList();
          accessInfo = accList[0];
          await usbManager.requestAccessoryRight(accessInfo);
          let accHandle: usbManager.USBAccessoryHandle = usbManager.openAccessory(accessInfo);
          usbManager.closeAccessory(accHandle);
          usbManager.cancelAccessoryRight(accessInfo);
          done();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'testOpenAccessory057 err : ', err);
          expect(err.code).assertEqual(14400005);
        }
      })

    it("testUsbSubmitTransfer058", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer058 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbSubmitTransfer058 No device is connected');
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
            hilog.info(domain, tag, 'callBackData = ' + JSON.stringify(callBackData));
            hilog.info(domain, tag,
              'testUsbSubmitTransfer058 transfer success, result = ' + transferParams.buffer.toString());
            hilog.info(domain, tag, 'status = ' + callBackData?.status);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_TIMED_OUT).assertEqual(true);
            usbManager.closePipe(gPipe);
            hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer058 end');
            done()
          }
          usbManager.usbSubmitTransfer(transferParams);
        } catch (error) {
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, 'interrupt error:', err.code);
          usbManager.closePipe(gPipe);
          expect(err.code).assertEqual(14400001);
          done()
        }
      })

    it("testUsbSubmitTransfer059", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer059 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbSubmitTransfer059 No device is connected');
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
            hilog.info(domain, tag, 'callBackData = ' + JSON.stringify(callBackData));
            hilog.info(domain, tag,
              'testUsbSubmitTransfer059 transfer success, result = ' + transferParams.buffer.toString());
            hilog.info(domain, tag, 'status = ' + callBackData?.status);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_TIMED_OUT).assertEqual(true);
            usbManager.closePipe(gPipe);
            hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer059 end');
            done()
          }
          usbManager.usbSubmitTransfer(transferParams);
        } catch (error) {
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, 'interrupt error:', err.code);
          usbManager.closePipe(gPipe);
          expect(err.code).assertEqual(14400007);
          done()
        }
      })


    it("testUsbSubmitTransfer060", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer060 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbSubmitTransfer060 No device is connected');
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
          transferParams.callback = (err: BusinessError | null, callBackData: usbManager.SubmitTransferCallback | undefined)=> {
            hilog.info(domain, tag, 'callBackData = ' + JSON.stringify(callBackData));
            hilog.info(domain, tag,
              'testUsbSubmitTransfer060 transfer success, result = ' + transferParams.buffer.toString());
            hilog.info(domain, tag, 'status = ' + callBackData?.status);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_TIMED_OUT).assertEqual(true);
            usbManager.closePipe(gPipe);
            hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer060 end');
            done()
          }
          usbManager.usbSubmitTransfer(transferParams);
        } catch (error) {
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, 'interrupt error:', err.code);
          usbManager.closePipe(gPipe);
          expect(err.code).assertEqual(14400008);
          done()
        }
      })


    it("testUsbSubmitTransfer061", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer061 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbSubmitTransfer061 No device is connected');
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
            hilog.info(domain, tag, 'callBackData = ' + JSON.stringify(callBackData));
            hilog.info(domain, tag,
              'testUsbSubmitTransfer061 transfer success, result = ' + transferParams.buffer.toString());
            hilog.info(domain, tag, 'status = ' + callBackData?.status);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_TIMED_OUT).assertEqual(true);
            usbManager.closePipe(gPipe);
            hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer061 end');
            done()
          }
          usbManager.usbSubmitTransfer(transferParams);
        } catch (error) {
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, 'interrupt error:', err.code);
          usbManager.closePipe(gPipe);
          expect(err.code).assertEqual(14400009);
          done()
        }
      })

    it("testUsbSubmitTransfer062", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer062 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbSubmitTransfer062 No device is connected');
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
          transferParams.callback = (err: BusinessError | null, callBackData: usbManager.SubmitTransferCallback | undefined)=> {
            hilog.info(domain, tag, 'callBackData = ' + JSON.stringify(callBackData));
            hilog.info(domain, tag,
              'testUsbSubmitTransfer062 transfer success, result = ' + transferParams.buffer.toString());
            hilog.info(domain, tag, 'status = ' + callBackData?.status);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_TIMED_OUT).assertEqual(true);
            usbManager.closePipe(gPipe);
            hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer062 end');
            done()
          }
          usbManager.usbSubmitTransfer(transferParams);
        } catch (error) {
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, 'interrupt error:', err.code);
          usbManager.closePipe(gPipe);
          expect(err.code).assertEqual(14400012);
          done()
        }
      })

    it("testUsbCancelTransfer063", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbCancelTransfer063 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbCancelTransfer063 No device is connected');
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
          return value.direction === 0 && value.type === 2
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
          hilog.info(domain, tag, 'testUsbCancelTransfer063 catch err code================: ' + JSON.stringify(error));
          expect(error.code).assertEqual(14400001);
        }
        done();
      })

    it("testUsbCancelTransfer064", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbCancelTransfer064 enter');
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
          return value.direction === 0 && value.type === 2
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
          hilog.info(domain, tag, 'testUsbCancelTransfer064 catch err code================: ' + JSON.stringify(error));
          expect(error.code).assertEqual(14400008);
        }
        done();
      })

    it("testUsbCancelTransfer065", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbCancelTransfer065 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbCancelTransfer065 No device is connected');
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
          return value.direction === 0 && value.type === 2
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
          hilog.info(domain, tag, 'testUsbCancelTransfer065 catch err code================: ' + JSON.stringify(error));
          expect(error.code).assertEqual(14400010);
        }
        done();
      })


    it("testUsbSubmitTransfer066", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer066 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbSubmitTransfer066 No device is connected');
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
        let type: usbManager.UsbEndpointTransferType = usbManager.UsbEndpointTransferType.TRANSFER_TYPE_BULK;
        let transferParams: usbManager.UsbDataTransferParams = getTransfersParam(gPipe, flags, 129, type, 2000);
        usbManager.claimInterface(gPipe, tmpInterface, true);
        try {
          transferParams.callback = (err: BusinessError | null, callBackData: usbManager.SubmitTransferCallback | undefined)=> {
            hilog.info(domain, tag, 'callBackData = ' + JSON.stringify(callBackData));
            hilog.info(domain, tag,
              'testUsbSubmitTransfer066 transfer success, result = ' + transferParams.buffer.toString());
            hilog.info(domain, tag, 'status = ' + callBackData?.status);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_TIMED_OUT).assertEqual(true);
            usbManager.closePipe(gPipe);
            hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer066 end');
            done()
          }
          usbManager.usbSubmitTransfer(transferParams);
        } catch (error) {
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, 'interrupt error:', err.code);
          usbManager.closePipe(gPipe);
          expect(err.code).assertEqual(14400012);
          done()
        }
      })

    it("testUsbSubmitTransfer067", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer067 enter');
        if (!isDeviceConnected) {
          hilog.info(domain, tag, 'usb testUsbSubmitTransfer067 No device is connected');
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
        let type: usbManager.UsbEndpointTransferType = usbManager.UsbEndpointTransferType.TRANSFER_TYPE_ISOCHRONOUS;
        let transferParams: usbManager.UsbDataTransferParams = getTransfersParam(gPipe, flags, 129, type, 2000);
        usbManager.claimInterface(gPipe, tmpInterface, true);
        try {
          transferParams.callback = (err: BusinessError | null, callBackData: usbManager.SubmitTransferCallback | undefined)=> {
            hilog.info(domain, tag, 'callBackData = ' + JSON.stringify(callBackData));
            hilog.info(domain, tag,
              'testUsbSubmitTransfer067 transfer success, result = ' + transferParams.buffer.toString());
            hilog.info(domain, tag, 'status = ' + callBackData?.status);
            expect(callBackData?.status == usbManager.UsbTransferStatus.TRANSFER_TIMED_OUT).assertEqual(true);
            usbManager.closePipe(gPipe);
            hilog.info(domain, tag, 'interrupt testUsbSubmitTransfer067 end');
            done()
          }
          usbManager.usbSubmitTransfer(transferParams);
        } catch (error) {
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, 'interrupt error:', err.code);
          usbManager.closePipe(gPipe);
          expect(err.code).assertEqual(14400012);
          done()
        }
      })



    it("test_usbControlTransfer68", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb test_usbControlTransfer68 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        let param: usbManager.USBDeviceRequestParams = {
          bmRequestType: 0x80,
          bRequest: usbManager.USBControlRequestType.USB_REQUEST_TYPE_STANDARD,
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
          let ret = usbManager.usbControlTransfer(devicePipe, param).then((ret: int) => {
            hilog.info(domain, tag, `usbControlTransfer size= ${ret}`);
            expect(ret == 18).assertEqual(true);
          })
          usbManager.closePipe(devicePipe);
        } catch (error: BusinessError) {
          hilog.info(0, 'testTag ui', `closeAccessory error `)
          hilog.info(domain, tag, 'usbControlTransfer end');
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, `test_usbControlTransfer68 err.code= ${err.code}`);
        }
        done();
      })

    it("test_usbControlTransfer69", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb test_usbControlTransfer69 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        let param: usbManager.USBDeviceRequestParams = {
          bmRequestType: 0x80,
          bRequest: usbManager.USBControlRequestType.USB_REQUEST_TYPE_CLASS,
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
          let ret = usbManager.usbControlTransfer(devicePipe, param).then((ret: int) => {
            hilog.info(domain, tag, `usbControlTransfer size= ${ret}`);
            expect(ret == 18).assertEqual(true);
          })
          usbManager.closePipe(devicePipe);
        } catch (error: BusinessError) {
          hilog.info(0, 'testTag ui', `closeAccessory error `)
          hilog.info(domain, tag, 'usbControlTransfer end');
          let err: BusinessError = error as BusinessError;
          hilog.info(domain, tag, `test_usbControlTransfer69 err.code= ${err.code}`);
        }
        done();
      })

    it("test_usbControlTransfer70", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'usb test_usbControlTransfer70 begin');
        if (!isDeviceConnected) {
          expect(isDeviceConnected).assertFalse();
          done()
        }
        let param: usbManager.USBDeviceRequestParams = {
          bmRequestType: 0x80,
          bRequest: usbManager.USBControlRequestType.USB_REQUEST_TYPE_VENDOR,
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
          let ret = usbManager.usbControlTransfer(devicePipe, param).then((ret: int) => {
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
  })
}