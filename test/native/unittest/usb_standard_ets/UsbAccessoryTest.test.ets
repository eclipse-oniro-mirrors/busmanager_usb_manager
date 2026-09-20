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
import { Driver, MatchPattern, ON } from '@ohos.UiTest';
import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it, Level, Size, TestType } from '@ohos/hypium';

const TAG: string = "[UsbAccessoryTest]";
const SKIP_TAG: string = "[SKIP]";

let accessoryList: Array<usbMgr.USBAccessory>;
let accessory: usbMgr.USBAccessory;
let hasAccessory: boolean;
let fakeAccessory: usbMgr.USBAccessory = {
  manufacturer: 'manufacturer',
  product: 'product',
  description: 'description',
  version: 'version',
  serialNumber: 'serialNumber'
}

function isAccessoryAvailable(): boolean {
  if (accessoryList.length > 0) {
    console.info(`${TAG} USB accessory is available`);
    return true;
  }
  console.info(`${TAG} USB accessory is not available`);
  return false;
}

async function clickAllowButton(): Promise<void> {
  console.info(`${TAG} clickAllowButton`);
  try {
    let driver: Driver = Driver.create();
    await driver.delayMs(1000);
    let buttons = await driver.findComponents(ON.text('允许', MatchPattern.EQUALS));
    if (!buttons || buttons.length === 0) {
      buttons = await driver.findComponents(ON.text('Allow', MatchPattern.EQUALS));
    }
    if (buttons && buttons.length > 0) {
      for (let i = 0; i < buttons.length; i++) {
        await buttons[i].click();
      }
    }
  } catch (error) {
    console.error(`${TAG} clickAllowButton catch err: ${JSON.stringify(error)}`);
  }
}

async function clickRejectButton(): Promise<void> {
  console.info(`${TAG} clickRejectButton`);
  try {
    let driver: Driver = Driver.create();
    await driver.delayMs(1000);
    let buttons = await driver.findComponents(ON.text('不允许', MatchPattern.EQUALS));
    if (!buttons || buttons.length === 0) {
      buttons = await driver.findComponents(ON.text('Disallow', MatchPattern.EQUALS));
    }
    if (buttons && buttons.length > 0) {
      await buttons[0].click();
    }
  } catch (error) {
    console.error(`${TAG} clickRejectButton catch err: ${JSON.stringify(error)}`);
  }
}

async function requestAccessoryRight(accessory: usbMgr.USBAccessory): Promise<void> {
  console.info(`${TAG} requestAccessoryRight start`);
  const rightPromise = usbMgr.requestAccessoryRight(accessory);
  await clickAllowButton();
  try {
    let hasRight = await rightPromise;
    console.info(`${TAG} requestAccessoryRight success, hasRight: ${hasRight}`);
  } catch (error) {
    console.error(`${TAG} requestAccessoryRight fail: ${JSON.stringify(error)}`);
  }
}

export default function UsbAccessoryTest() {
  describe("UsbAccessoryTest", () => {

    beforeAll(() => {
      console.info(`${TAG} beforeAll start`);
      try {
        accessoryList = usbMgr.getAccessoryList();
        if (accessoryList.length > 0) {
          accessory = accessoryList[0];
        }
        console.info(`${TAG} accessoryList: ${JSON.stringify(accessoryList)}`);
      } catch (error) {
        console.error(`${TAG} beforeAll getAccessoryList fail: ${JSON.stringify(error)}`);
      }
      hasAccessory = isAccessoryAvailable();
    })

    afterAll(() => {
      console.info(`${TAG} afterAll`);
    })

    beforeEach(() => {
      console.info(`${TAG} beforeEach`);
    })

    afterEach(() => {
      console.info(`${TAG} afterEach`);
    })

    /**
     * @tc.name   testAccessoryGetListVerify
     * @tc.number SUB_USB_DeviceManager_Accessory_0100
     * @tc.desc   getAccessoryList should return 1 accessory with string fields
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryGetListVerify', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testAccessoryGetListVerify start`);
      if (!hasAccessory) {
        console.info(`${TAG} ${SKIP_TAG} testAccessoryGetListVerify, skip`);
        return;
      }
      accessoryList = usbMgr.getAccessoryList();
      console.info(`${TAG} accessoryList: ${JSON.stringify(accessoryList)}`);
      expect(accessoryList.length).assertEqual(1);
      expect(typeof (accessory.manufacturer)).assertEqual('string');
      expect(typeof (accessory.product)).assertEqual('string');
      expect(typeof (accessory.description)).assertEqual('string');
      expect(typeof (accessory.version)).assertEqual('string');
      expect(typeof (accessory.serialNumber)).assertEqual('string');
    })

    /**
     * @tc.name   testAccessoryGetListEmpty
     * @tc.number SUB_USB_DeviceManager_Accessory_RetValue_0400
     * @tc.desc   getAccessoryList should return empty list when no accessory
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryGetListEmpty', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testAccessoryGetListEmpty start`);
      if (hasAccessory) {
        console.info(`${TAG} ${SKIP_TAG} testAccessoryGetListEmpty, accessory exists, skip`);
        return;
      }
      let res = usbMgr.getAccessoryList();
      console.info(`${TAG} accessoryList length: ${res.length}`);
      expect(res.length).assertEqual(0);
    })

    /**
     * @tc.name   testAccessoryRequestRightInvalidParam
     * @tc.number SUB_USB_DeviceManager_Accessory_0200
     * @tc.desc   requestAccessoryRight with fake accessory should throw 14401001
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryRequestRightInvalidParam', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3,
      async (done: Function) => {
        console.info(`${TAG} testAccessoryRequestRightInvalidParam start`);
        if (!hasAccessory) {
          console.info(`${TAG} ${SKIP_TAG} testAccessoryRequestRightInvalidParam, skip`);
          done();
          return;
        }
        try {
          await usbMgr.requestAccessoryRight(fakeAccessory);
          expect().assertFail();
          done();
        } catch (err) {
          console.error(`${TAG} testAccessoryRequestRightInvalidParam catch err: ${JSON.stringify(err)}`);
          expect(err.code).assertEqual(14401001);
          done();
        }
      })

    /**
     * @tc.name   testAccessoryRequestRightAllow
     * @tc.number SUB_USB_DeviceManager_Accessory_0300
     * @tc.desc   requestAccessoryRight with real accessory and click allow should return true
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryRequestRightAllow', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(`${TAG} testAccessoryRequestRightAllow start`);
      if (!hasAccessory) {
        console.info(`${TAG} ${SKIP_TAG} testAccessoryRequestRightAllow, skip`);
        done();
        return;
      }
      try {
        const rightPromise = usbMgr.requestAccessoryRight(accessory);
        await clickAllowButton();
        let hasRight = await rightPromise;
        console.info(`${TAG} requestAccessoryRight result: ${hasRight}`);
        expect(hasRight).assertTrue();
        usbMgr.cancelAccessoryRight(accessory);
        done();
      } catch (err) {
        console.error(`${TAG} testAccessoryRequestRightAllow catch err: ${JSON.stringify(err)}`);
        expect().assertFail();
        done(err);
      }
    })

    /**
     * @tc.name   testAccessoryRequestRightReject
     * @tc.number SUB_USB_DeviceManager_ReValue_Accessory_0100
     * @tc.desc   requestAccessoryRight with real accessory and click reject should return false
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryRequestRightReject', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3,
      async (done: Function) => {
        console.info(`${TAG} testAccessoryRequestRightReject start`);
        if (!hasAccessory) {
          console.info(`${TAG} ${SKIP_TAG} testAccessoryRequestRightReject, skip`);
          done();
          return;
        }
        try {
          const rightPromise = usbMgr.requestAccessoryRight(accessory);
          await clickRejectButton();
          let hasRight = await rightPromise;
          console.info(`${TAG} requestAccessoryRight result: ${hasRight}`);
          expect(hasRight).assertFalse();
          done();
        } catch (err) {
          console.error(`${TAG} testAccessoryRequestRightReject catch err: ${JSON.stringify(err)}`);
          expect().assertFail();
          done(err);
        }
      })

    /**
     * @tc.name   testAccessoryHasRightInvalidParam
     * @tc.number SUB_USB_DeviceManager_Accessory_0400
     * @tc.desc   hasAccessoryRight with fake accessory should throw 14401001
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryHasRightInvalidParam', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testAccessoryHasRightInvalidParam start`);
      if (!hasAccessory) {
        console.info(`${TAG} ${SKIP_TAG} testAccessoryHasRightInvalidParam, skip`);
        return;
      }
      try {
        usbMgr.hasAccessoryRight(fakeAccessory);
        expect().assertFail();
      } catch (error) {
        console.error(`${TAG} testAccessoryHasRightInvalidParam catch err: ${JSON.stringify(error)}`);
        expect(error.code).assertEqual(14401001);
      }
    })

    /**
     * @tc.name   testAccessoryHasRightAfterGrant
     * @tc.number SUB_USB_DeviceManager_Accessory_0600
     * @tc.desc   hasAccessoryRight should return true after granting permission
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryHasRightAfterGrant', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3,
      async (done: Function) => {
        console.info(`${TAG} testAccessoryHasRightAfterGrant start`);
        if (!hasAccessory) {
          console.info(`${TAG} ${SKIP_TAG} testAccessoryHasRightAfterGrant, skip`);
          done();
          return;
        }
        try {
          await requestAccessoryRight(accessory);
          let ret = usbMgr.hasAccessoryRight(accessory);
          console.info(`${TAG} hasAccessoryRight after grant: ${ret}`);
          expect(ret).assertTrue();
          usbMgr.cancelAccessoryRight(accessory);
          done();
        } catch (err) {
          console.error(`${TAG} testAccessoryHasRightAfterGrant catch err: ${JSON.stringify(err)}`);
          expect().assertFail();
          done(err);
        }
      })

    /**
     * @tc.name   testAccessoryHasRightAfterCancel
     * @tc.number SUB_USB_DeviceManager_Accessory_0601
     * @tc.desc   hasAccessoryRight should return false after canceling permission
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryHasRightAfterCancel', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3,
      async (done: Function) => {
        console.info(`${TAG} testAccessoryHasRightAfterCancel start`);
        if (!hasAccessory) {
          console.info(`${TAG} ${SKIP_TAG} testAccessoryHasRightAfterCancel, skip`);
          done();
          return;
        }
        try {
          await requestAccessoryRight(accessory);
          usbMgr.cancelAccessoryRight(accessory);
          let ret = usbMgr.hasAccessoryRight(accessory);
          console.info(`${TAG} hasAccessoryRight after cancel: ${ret}`);
          expect(ret).assertFalse();
          done();
        } catch (err) {
          console.error(`${TAG} testAccessoryHasRightAfterCancel catch err: ${JSON.stringify(err)}`);
          expect().assertFail();
          done(err);
        }
      })

    /**
     * @tc.name   testAccessoryOpenInvalidParam
     * @tc.number SUB_USB_DeviceManager_Accessory_0900
     * @tc.desc   openAccessory with fake accessory should throw 14401001
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryOpenInvalidParam', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testAccessoryOpenInvalidParam start`);
      if (!hasAccessory) {
        console.info(`${TAG} ${SKIP_TAG} testAccessoryOpenInvalidParam, skip`);
        return;
      }
      try {
        usbMgr.openAccessory(fakeAccessory);
        expect().assertFail();
      } catch (error) {
        console.error(`${TAG} testAccessoryOpenInvalidParam catch err: ${JSON.stringify(error)}`);
        expect(error.code).assertEqual(14401001);
      }
    })

    /**
     * @tc.name   testAccessoryOpenSuccess
     * @tc.number SUB_USB_DeviceManager_Accessory_1000
     * @tc.desc   openAccessory with permission should return handle with fd > 0
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryOpenSuccess', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(`${TAG} testAccessoryOpenSuccess start`);
      if (!hasAccessory) {
        console.info(`${TAG} ${SKIP_TAG} testAccessoryOpenSuccess, skip`);
        done();
        return;
      }
      try {
        await requestAccessoryRight(accessory);
        let accHandle = usbMgr.openAccessory(accessory);
        console.info(`${TAG} accHandle: ${JSON.stringify(accHandle)}`);
        let accFd = accHandle.accessoryFd;
        console.info(`${TAG} accessoryFd: ${accFd}`);
        expect(typeof (accFd)).assertEqual("number");
        expect(accFd).assertLarger(0);
        usbMgr.closeAccessory(accHandle!);
        usbMgr.cancelAccessoryRight(accessory);
        done();
      } catch (err) {
        console.error(`${TAG} testAccessoryOpenSuccess catch err: ${JSON.stringify(err)}`);
        expect().assertFail();
        done(err);
      }
    })

    /**
     * @tc.name   testAccessoryOpenReopen
     * @tc.number SUB_USB_DeviceManager_Accessory_1100
     * @tc.desc   openAccessory twice should throw 14401003
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryOpenReopen', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(`${TAG} testAccessoryOpenReopen start`);
      if (!hasAccessory) {
        console.info(`${TAG} ${SKIP_TAG} testAccessoryOpenReopen, skip`);
        done();
        return;
      }
      let accHandle: usbMgr.USBAccessoryHandle | null = null;
      try {
        await requestAccessoryRight(accessory);
        accHandle = usbMgr.openAccessory(accessory);
        usbMgr.openAccessory(accessory);
        expect().assertFail();
        done();
      } catch (err) {
        console.error(`${TAG} testAccessoryOpenReopen catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(14401003);
        done();
      } finally {
        if (accHandle) {
          try {
            usbMgr.closeAccessory(accHandle!);
          } catch (e) {
            console.error(`${TAG} testAccessoryOpenReopen closeAccessory fail: ${JSON.stringify(e)}`);
          }
        }
        usbMgr.cancelAccessoryRight(accessory);
      }
    })

    /**
     * @tc.name   testAccessoryOpenNoPermission
     * @tc.number SUB_USB_DeviceManager_Accessory_1200
     * @tc.desc   openAccessory without permission should throw 14400001
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryOpenNoPermission', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(`${TAG} testAccessoryOpenNoPermission start`);
      if (!hasAccessory) {
        console.info(`${TAG} ${SKIP_TAG} testAccessoryOpenNoPermission, skip`);
        done();
        return;
      }
      try {
        usbMgr.cancelAccessoryRight(accessory);
        usbMgr.openAccessory(accessory);
        expect().assertFail();
        done();
      } catch (err) {
        console.error(`${TAG} testAccessoryOpenNoPermission catch err: ${JSON.stringify(err)}`);
        expect(err.code).assertEqual(14400001);
        done();
      } finally {
        if (!usbMgr.hasAccessoryRight(accessory)) {
          console.info(`${TAG} testAccessoryOpenNoPermission: restoring permission`);
          await requestAccessoryRight(accessory);
        }
      }
    })

    /**
     * @tc.name   testAccessoryCloseSuccess
     * @tc.number SUB_USB_DeviceManager_Accessory_1300
     * @tc.desc   closeAccessory with valid handle should succeed without error
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryCloseSuccess', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: Function) => {
      console.info(`${TAG} testAccessoryCloseSuccess start`);
      if (!hasAccessory) {
        console.info(`${TAG} ${SKIP_TAG} testAccessoryCloseSuccess, skip`);
        done();
        return;
      }
      let accHandle: usbMgr.USBAccessoryHandle;
      try {
        await requestAccessoryRight(accessory);
        accHandle = usbMgr.openAccessory(accessory);
        usbMgr.closeAccessory(accHandle!);
        console.info(`${TAG} closeAccessory success`);
        done();
      } catch (err) {
        console.error(`${TAG} testAccessoryCloseSuccess catch err: ${JSON.stringify(err)}`);
        expect().assertFail();
        done(err);
      } finally {
        usbMgr.cancelAccessoryRight(accessory);
      }
    })

    /**
     * @tc.name   testAccessoryCloseInvalidFd
     * @tc.number SUB_USB_DeviceManager_Accessory_1400
     * @tc.desc   closeAccessory with fd=0 should throw 14400004
     * @tc.type   FUNCTION
     * @tc.size   MEDIUMTEST
     * @tc.level  LEVEL3
     */
    it('testAccessoryCloseInvalidFd', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, () => {
      console.info(`${TAG} testAccessoryCloseInvalidFd start`);
      if (!hasAccessory) {
        console.info(`${TAG} ${SKIP_TAG} testAccessoryCloseInvalidFd, skip`);
        return;
      }
      let accHandle: usbMgr.USBAccessoryHandle = { accessoryFd: 0 };
      try {
        usbMgr.closeAccessory(accHandle!);
        expect().assertFail();
      } catch (error) {
        console.error(`${TAG} testAccessoryCloseInvalidFd catch err: ${JSON.stringify(error)}`);
        expect(error.code).assertEqual(14400004);
      }
    })

  })
}
