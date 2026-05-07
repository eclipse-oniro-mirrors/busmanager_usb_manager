/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

import UIExtensionAbility from '@ohos.app.ability.UIExtensionAbility';
import UIExtensionContentSession from '@ohos.app.ability.UIExtensionContentSession';
import rpc from '@ohos.rpc';
import {terminateDialog} from '../util/DialogUtil';

let TAG: string = 'UsbService';

const BG_COLOR = '#00000000';
const COLOR_MODE_NOT_SET = -1;

export default class UsbDialogAbility extends UIExtensionAbility {
  /**
   * Lifecycle function, called back when a service extension is started for initialization.
   */
  onSessionCreate(want, session): void {
    console.log(TAG + 'UsbDialogAbility onSessionCreate');
    console.log(JSON.stringify(want));

    globalThis.extensionContext = this.context;
    globalThis.want = want;
    globalThis.windowNum = 0;

    try {
      AppStorage.setOrCreate('session', session);
      let page = want?.parameters?.['portName'] !== undefined ? 'pages/SerialDialog' : 'pages/UsbDialog';
      (session as UIExtensionContentSession)?.loadContent(page);
      (session as UIExtensionContentSession)?.setWindowBackgroundColor(BG_COLOR);
      this.context?.getApplicationContext()?.setColorMode(COLOR_MODE_NOT_SET);
    } catch (error) {
      console.error(TAG + 'UsbFunctionSwitchAbility onSessionCreate error: ' + error?.code);
      terminateDialog();
    }
  }

  onSessionDestroy(session): void {
    console.log(TAG + 'UsbDialogAbility onSessionDestroy');
  }
};

