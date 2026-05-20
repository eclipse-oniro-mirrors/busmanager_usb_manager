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

import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect, TestType, Size, Level } from "../../../hypium/index";
import hilog from '@ohos.hilog'
import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';
import common from '@ohos.app.ability.common';
import UIAbility from '@ohos.app.ability.UIAbility';
import { BusinessError } from '@ohos.base';
import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';
import Want from '@ohos.app.ability.Want';
import Utils from './Util.test';
import buffer from '@ohos.buffer';
import serialManager from '@ohos.usbManager.serial';

let domain: number = 0x0000;
const TAG: string = "[SerialTest]";
let portIds: int[] = [];

function checkDevice(): boolean {
  let haveDevice: boolean = false;
  try {
    const portList = serialManager.getPortList();
    if (Array.isArray(portList)) {
      portList.forEach((port) => {
        portIds.push(port.portId);
        haveDevice = true;
      });
    } else {
      hilog.error(domain, TAG, '%{public}s', `Failed! getPortList, did not return an array: ${typeof portList}`);
      haveDevice = false;
    }
  } catch (error : BusinessError) {
    hilog.error(domain, TAG, '%{public}s', `Failed! getPortList, error code: ${error}`);
    haveDevice = false;
  }
  return haveDevice;
}

export default function SerialTest() {
  describe('SerialTest', () => {
    let openPortId: int = -1;
    beforeAll(() => {
      try {
        if (checkDevice()) {
          openPortId = portIds[0];
          let hasRight: boolean = serialManager.hasSerialRight(openPortId);
          if (!hasRight) {
            hilog.info(domain, TAG, 'beforeEach: requestSerialRight start');
            let futureRes: Promise<boolean> = serialManager.requestSerialRight(openPortId);
            try {
              let hasRight: boolean = await futureRes;
              hilog.info(domain, TAG, 'getPermission isAgree: ', hasRight);
            } catch (err : BusinessError) {
              hilog.error(domain, TAG, 'getPermission catch err name: ', err.name, ', message: ', err.message);
            }
          }
        } else {
          hilog.error(domain, TAG, 'no device');
        }
      } catch(error : BusinessError){
        hilog.info(domain, TAG, 'beforeAll error: ', JSON.stringify(error));
      }
    })
    afterAll(async () => {
      hilog.info(domain, TAG, 'afterAll');
      if (serialManager.hasSerialRight(openPortId)){
        serialManager.close(openPortId);
        serialManager.cancelSerialRight(openPortId);
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_setAttribute_Func_0100
     * @tc.name     : testSetAttribute001
     * @tc.desc     : setAttribute successfully.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 0
     */
    it('testSetAttribute001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL0, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testSetAttribute001 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let attribute: serialManager.SerialAttribute = {
          baudRate: serialManager.BaudRates.BAUDRATE_9600,
          dataBits: serialManager.DataBits.DATABIT_8,
          parity: serialManager.Parity.PARITY_NONE,
          stopBits: serialManager.StopBits.STOPBIT_1
        };
        serialManager.open(openPortId);
        serialManager.setAttribute(openPortId, attribute);
        expect(serialManager.getAttribute(openPortId).dataBits).assertEqual(serialManager.DataBits.DATABIT_8);
        serialManager.close(openPortId);
        done();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testSetAttribute001 catch err code: ', error.code, ', message: ', error.message);
        expect().assertFail();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_setAttribute_ErrCode_0100
     * @tc.name     : testSetAttribute002
     * @tc.desc     : not openPortId to setAttribute.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testSetAttribute002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testSetAttribute002 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let attribute: serialManager.SerialAttribute = {
          baudRate: serialManager.BaudRates.BAUDRATE_9600,
          dataBits: serialManager.DataBits.DATABIT_8,
          parity: serialManager.Parity.PARITY_NONE,
          stopBits: serialManager.StopBits.STOPBIT_1
        };
        serialManager.setAttribute(openPortId, attribute);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testSetAttribute002 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(31400005);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_setAttribute_ErrCode_0200
     * @tc.name     : testSetAttribute003
     * @tc.desc     : invalid PortId to setAttribute.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testSetAttribute003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testSetAttribute003 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let attribute: serialManager.SerialAttribute = {
          baudRate: serialManager.BaudRates.BAUDRATE_9600,
          dataBits: serialManager.DataBits.DATABIT_8,
          parity: serialManager.Parity.PARITY_NONE,
          stopBits: serialManager.StopBits.STOPBIT_1
        };
        let invalidInput: int = -1;
        serialManager.setAttribute(invalidInput, attribute);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testSetAttribute003 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_setAttribute_ErrCode_0300
     * @tc.name     : testSetAttribute004
     * @tc.desc     : not exist PortId to setAttribute.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testSetAttribute004', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testSetAttribute004 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let attribute: serialManager.SerialAttribute = {
          baudRate: serialManager.BaudRates.BAUDRATE_9600,
          dataBits: serialManager.DataBits.DATABIT_8,
          parity: serialManager.Parity.PARITY_NONE,
          stopBits: serialManager.StopBits.STOPBIT_1
        };
        let invalidInput: int = 100;
        serialManager.setAttribute(invalidInput, attribute);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testSetAttribute004 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(31400003);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_getAttribute_Func_0100
     * @tc.name     : testGetAttribute001
     * @tc.desc     : getAttribute successfully.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 0
     */
    it('testGetAttribute001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL0, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testGetAttribute001 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let attribute: serialManager.SerialAttribute = {
          baudRate: serialManager.BaudRates.BAUDRATE_9600,
          dataBits: serialManager.DataBits.DATABIT_8,
          parity: serialManager.Parity.PARITY_NONE,
          stopBits: serialManager.StopBits.STOPBIT_2
        };
        serialManager.open(openPortId);
        serialManager.setAttribute(openPortId, attribute);
        expect(serialManager.getAttribute(openPortId).stopBits).assertEqual(serialManager.StopBits.STOPBIT_2);
        serialManager.close(openPortId);
        done();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testGetAttribute001 catch err code: ', error.code, ', message: ', error.message);
        expect().assertFail();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_getAttribute_ErrCode_0100
     * @tc.name     : testGetAttribute002
     * @tc.desc     : not openPortId to getAttribute.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testGetAttribute002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testGetAttribute002 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        serialManager.getAttribute(openPortId);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testGetAttribute002 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(31400005);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_getAttribute_ErrCode_0200
     * @tc.name     : testGetAttribute003
     * @tc.desc     : invalid openPortId to getAttribute.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testGetAttribute003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testGetAttribute003 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let invalidInput: int = -1;
        serialManager.getAttribute(invalidInput);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testGetAttribute003 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_getAttribute_ErrCode_0300
     * @tc.name     : testGetAttribute004
     * @tc.desc     : not exist portId to getAttribute.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testGetAttribute004', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testGetAttribute004 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let invalidInput: int = 100;
        serialManager.getAttribute(invalidInput);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testGetAttribute004 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(31400003);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_read_ErrCode_0100
     * @tc.name     : testRead001
     * @tc.desc     : read timeout.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testRead001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testRead001 start');
        await Utils.msSleep(1000);
        serialManager.open(openPortId);
        await Utils.msSleep(1000);
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }

        //没有写数据，必定会超时
        let readBuffer: Uint8Array = new Uint8Array(8192);
        await serialManager.read(openPortId, readBuffer, 10);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testRead001 catch err name: ', error);
        expect(error.code).assertEqual(31400006);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_readSync_ErrCode_0100
     * @tc.name     : testReadSync001
     * @tc.desc     : read timeout.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testReadSync001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testReadSync001 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let readSyncBuffer: Uint8Array = new Uint8Array(8192);
        let length: int = serialManager.readSync(openPortId, readSyncBuffer, 10);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testReadSync001 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(31400006);
        serialManager.close(openPortId);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_writeSync_Func_0100
     * @tc.name     : testWriteSync001
     * @tc.desc     : writeSync successfully.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 0
     */
    it('testWriteSync001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL0, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testWriteSync001 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let writeSyncBuffer: Uint8Array = new Uint8Array(100).fill(1);
        serialManager.open(openPortId);
        let length: int = serialManager.writeSync(openPortId, writeSyncBuffer, 500);
        expect(length >= 0).assertTrue();
        serialManager.close(openPortId);
        done();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testWriteSync001 catch err code: ', error.code, ', message: ', error.message);
        expect().assertFail();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_writeSync_ErrCode_0100
     * @tc.name     : testWriteSync002
     * @tc.desc     : not openPortId to writeSync.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testWriteSync002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testWriteSync002 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let writeSyncBuffer: Uint8Array = new Uint8Array(100).fill(1);
        serialManager.writeSync(openPortId, writeSyncBuffer, 500);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testWriteSync002 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(31400005);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_writeSync_ErrCode_0200
     * @tc.name     : testWriteSync003
     * @tc.desc     : invalidPortId to writeSync.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testWriteSync003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testWriteSync003 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let invalidInput: int = -1;
        let writeSyncBuffer: Uint8Array = new Uint8Array(100).fill(1);
        serialManager.writeSync(invalidInput, writeSyncBuffer, 500);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testWriteSync003 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_writeSync_ErrCode_0300
     * @tc.name     : testWriteSync004
     * @tc.desc     : not exist PortId to writeSync.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testWriteSync004', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testWriteSync004 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let invalidInput: int = 100;
        let writeSyncBuffer: Uint8Array = new Uint8Array(100).fill(1);
        serialManager.writeSync(invalidInput, writeSyncBuffer, 500);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testWriteSync004 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(31400003);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_write_Func_0100
     * @tc.name     : testWrite001
     * @tc.desc     : write successfully.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 0
     */
    it('testWrite001', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL0, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testWrite001 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let writeBuffer: Uint8Array = new Uint8Array(100).fill(1);
        serialManager.open(openPortId);
        let length = await serialManager.write(openPortId, writeBuffer, 500);
        expect(length >= 0).assertTrue();
        serialManager.close(openPortId);
        done();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testWrite001 catch err name: ', error.name, ', message: ', error.message);
        expect().assertFail();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_write_ErrCode_0100
     * @tc.name     : testWrite002
     * @tc.desc     : invalid PortId to write.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testWrite002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try{
        hilog.info(domain, TAG, 'testWrite002 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let writeBuffer: Uint8Array = new Uint8Array(100).fill(1);
        let invalidInput: int = -1;
        await serialManager.write(invalidInput, writeBuffer, 500);
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testWrite002 catch err name: ', error.name, ', message: ', error.message);
        expect(error.code).assertEqual(401);
        done();
      };
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_write_ErrCode_0200
     * @tc.name     : testWrite003
     * @tc.desc     : not openPortId to write.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testWrite003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try{
        hilog.info(domain, TAG, 'testWrite003 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let writeBuffer: Uint8Array = new Uint8Array(100).fill(1);
        await serialManager.write(openPortId, writeBuffer, 500);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testWrite003 catch err name: ', error);
        expect(error.code).assertEqual(31400005);
        done();
      };
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_write_ErrCode_0300
     * @tc.name     : testWrite004
     * @tc.desc     : not exist PortId to write.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testWrite004', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try{
        hilog.info(domain, TAG, 'testWrite004 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let invalidport: int = 100;
        let writeBuffer: Uint8Array = new Uint8Array(100).fill(1);
        await serialManager.write(invalidport, writeBuffer, 500);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testWrite004 catch err name: ', error);
        expect(error.code).assertEqual(31400003);
        done();
      };
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_readSync_Func_0100
     * @tc.name     : testReadSync002
     * @tc.desc     : readsync successfully.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 0
     */
    it('testReadSync002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL0, async (done: () => void): Promise<void> => {
      await Utils.msSleep(1000);
      try {
        hilog.info(domain, TAG, 'testReadSync002 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        serialManager.open(openPortId);

        // serialManager.open(portIds[1]);
        // let writeSyncBuffer: Uint8Array = new Uint8Array(10).fill(1);
        // await serialManager.write(portIds[1], writeSyncBuffer, 200);
        // serialManager.close(portIds[1]);

        let readSyncBuffer: Uint8Array = new Uint8Array(8192);
        let length: int = serialManager.readSync(openPortId, readSyncBuffer, 1000);
        expect(length >= 0).assertTrue();
        serialManager.close(openPortId);
        done();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testReadSync002 catch err code: ', error.code, ', message: ', error.message);
        expect().assertFail();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_readSync_ErrCode_0200
     * @tc.name     : testReadSync003
     * @tc.desc     : port not open.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testReadSync003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      await Utils.msSleep(1000);
      try {
        hilog.info(domain, TAG, 'testReadSync003 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let readSyncBuffer: Uint8Array = new Uint8Array(8192);
        serialManager.readSync(openPortId, readSyncBuffer, 500);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testReadSync003 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(31400005);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_readSync_ErrCode_0300
     * @tc.name     : testReadSync004
     * @tc.desc     : invalidport to open.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testReadSync004', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testReadSync004 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let invalid: int = -1;
        let readSyncBuffer: Uint8Array = new Uint8Array(8192);
        let length: int = serialManager.readSync(invalid, readSyncBuffer, 500);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testReadSync004 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_readSync_ErrCode_0400
     * @tc.name     : testReadSync005
     * @tc.desc     : invalidport to open.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testReadSync005', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testReadSync005 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let invalid: int = 100;
        let readSyncBuffer: Uint8Array = new Uint8Array(8192);
        let length: int = serialManager.readSync(invalid, readSyncBuffer, 500);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testReadSync005 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(31400003);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_read_Func_0100
     * @tc.name     : testRead002
     * @tc.desc     : read successfully.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 0
     */
    it('testRead002', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL0, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testRead002 start');
        await Utils.msSleep(3000);
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }

        // serialManager.open(portIds[1]);
        // let writeSyncBuffer: Uint8Array = new Uint8Array(10).fill(1);
        // await serialManager.write(portIds[1], writeSyncBuffer, 200);
        // serialManager.close(portIds[1]);

        let readBuffer: Uint8Array = new Uint8Array(8192);
        serialManager.open(openPortId);
        let length = await serialManager.read(openPortId, readBuffer, 1000);
        expect(length >= 0).assertTrue();
        serialManager.close(openPortId);
        done();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testRead002 catch err name: ', error.name, ', message: ', error.message);
        expect().assertFail();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_read_ErrCode_0200
     * @tc.name     : testRead003
     * @tc.desc     : port not open.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testRead003', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      await Utils.msSleep(1000);
      try {
        hilog.info(domain, TAG, 'testRead003 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let readBuffer: Uint8Array = new Uint8Array(8192);
        await serialManager.read(openPortId, readBuffer, 1000);
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testRead003 catch err name: ', error);
        expect(error.code).assertEqual(31400005);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_read_ErrCode_0300
     * @tc.name     : testRead004
     * @tc.desc     : invalidPort to read.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testRead004', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testRead004 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let readBuffer: Uint8Array = new Uint8Array(8192);
        let invalid: int = -1;
        await serialManager.read(invalid, readBuffer, 10);
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testRead004 catch err code: ', error.code, ', message: ', error.message);
        expect(error.code).assertEqual(401);
        done();
      }
    })

    /**
     * @tc.number   : SUB_USB_HostMgr_Serial_JS_read_ErrCode_0400
     * @tc.name     : testRead005
     * @tc.desc     : not exist to read.
     * @tc.size     : MediumTest
     * @tc.type     : Function
     * @tc.level    : Level 3
     */
    it('testRead005', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async (done: () => void): Promise<void> => {
      try {
        hilog.info(domain, TAG, 'testRead005 start');
        if (checkDevice() === false) {
          done();
          return Promise.resolve();
        }
        let readBuffer: Uint8Array = new Uint8Array(8192);
        let invalid: int = 100;
        await serialManager.read(invalid, readBuffer, 1000)
        expect().assertFail();
      } catch (error : BusinessError) {
        hilog.error(domain, TAG, 'testRead005 catch err name: ', error);
        expect(error.code).assertEqual(31400003);
        done();
      }
    })
  })
}
