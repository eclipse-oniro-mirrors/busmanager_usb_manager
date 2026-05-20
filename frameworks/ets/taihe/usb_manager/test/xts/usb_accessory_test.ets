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
    })

    it("abilityTest_addAccessoryRight_001", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL1,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, 'USB abilityTest_addAccessoryRight_0012 begin');
        try {
          let accList: usbManager.USBAccessory[] = usbManager.getAccessoryList();
          let bundleFlags = bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION;
          let data = bundleManager.getBundleInfoForSelfSync(bundleFlags);
          hilog.info(domain, tag, 'getBundleInfoForSelfSync: ' + JSON.stringify(data));
          let tokenId :int= Math.floor(data.appInfo.accessTokenId) as int;
          usbManager.addAccessoryRight(tokenId, accList[0])
          hilog.info(0, 'testTag ui', `addAccessoryRight success`)
        } catch (error) {
          hilog.info(0, 'testTag ui', `addAccessoryRight error `)
        }
        done();
      })

    /**
     * @tc.number     : SUB_USB_DeviceManager_JS_Accessory_0400
     * @tc.name       : testHasAccessoryRight001
     * @tc.desc       : erify hasAccessoryRight no param result
     * @tc.size       : MediumTest
     * @tc.type       : Function
     * @tc.level      : Level 3
     */
    it("testHasAccessoryRight001", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testHasAccessoryRight001---------------------------');
      try {
        accList = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = usbManager.hasAccessoryRight(accessInfo);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'usb testHasAccessoryRight001 catch err code: ', err.code, ', message: ',
          err.message);
        expect(err.code).assertEqual(14400005);
      }
    })

    /**
     * @tc.number     : SUB_USB_DeviceManager_JS_Accessory_0300
     * @tc.name       : testRequestAccessoryRight002
     * @tc.desc       : verify requestAccessoryRight result
     * @tc.size       : MediumTest
     * @tc.type       : Function
     * @tc.level      : Level 3
     */
    it("testRequestAccessoryRight002", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async () => {
      hilog.info(domain, tag, '----------------------testRequestAccessoryRight002---------------------------');
      try {
        accList = usbManager.getAccessoryList();
        accessInfo = accList[0];
        let ret = await usbManager.requestAccessoryRight(accessInfo);
        hilog.info(domain, tag, 'requestAccessoryRight ret ', ret);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'testRequestAccessoryRight002 err : ', err);
      }
    })

    /**
     * @tc.number     : SUB_USB_DeviceManager_JS_Accessory_0100
     * @tc.name       : testGetAccessoryList001
     * @tc.desc       : verify getAccessoryList result
     * @tc.size       : MediumTest
     * @tc.type       : Function
     * @tc.level      : Level 3
     */
    it("testGetAccessoryList001", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testGetAccessoryList001---------------------------');
      if (!isDevAccessoryFunc) {
        expect(isDevAccessoryFunc).assertFalse();
      }
      try {
        accList = usbManager.getAccessoryList();
        hilog.info(domain, tag, 'testGetAccessoryList001 ret : ', JSON.stringify(accList));
        expect(accList != null).assertEqual(true);
        expect(accList.length == 1).assertEqual(true);
        expect(typeof (accessInfo.manufacturer)).assertEqual('string');
        expect(typeof (accessInfo.product)).assertEqual('string');
        expect(typeof (accessInfo.description)).assertEqual('string');
        expect(typeof (accessInfo.version)).assertEqual('string');
        expect(typeof (accessInfo.serialNumber)).assertEqual('string');
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'testGetAccessoryList001 err : ', err);
        expect(err.code).assertEqual(14400005);
      }
    })

    /**
     * @tc.number     : SUB_USB_DeviceManager_JS_Accessory_1000
     * @tc.name       : testOpenAccessory002
     * @tc.desc       : verify openAccessory result
     * @tc.size       : MediumTest
     * @tc.type       : Function
     * @tc.level      : Level 3
     */
    it("testOpenAccessory002", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3,
      async (done: () => void): Promise<void> => {
        hilog.info(domain, tag, '----------------------testOpenAccessory002---------------------------');
        try {
          accList = usbManager.getAccessoryList();
          accessInfo = accList[0];
          await usbManager.requestAccessoryRight(accessInfo);
          let accHandle: usbManager.USBAccessoryHandle = usbManager.openAccessory(accessInfo);
          usbManager.closeAccessory(accHandle);
          usbManager.cancelAccessoryRight(accessInfo);
          done();
        } catch (error) {
          const err = error as BusinessError;
          hilog.info(domain, tag, 'testOpenAccessory002 err : ', err);
        }
      })

    /**
     * @tc.number     : SUB_USB_DeviceManager_JS_Accessory_1400
     * @tc.name       : testCloseAccessory002
     * @tc.desc       : verify openAccessory no param result
     * @tc.size       : MediumTest
     * @tc.type       : Function
     * @tc.level      : Level 3
     */
    it("testCloseAccessory002", TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      hilog.info(domain, tag, '----------------------testCloseAccessory002---------------------------');
      let accHandle: usbManager.USBAccessoryHandle = {
        accessoryFd: 2
      };
      try {
        usbManager.closeAccessory(accHandle);
      } catch (error) {
        const err = error as BusinessError;
        hilog.info(domain, tag, 'testCloseAccessory002 err : ', err);
        expect(err.code).assertEqual(401);
      }
    })
  })
}