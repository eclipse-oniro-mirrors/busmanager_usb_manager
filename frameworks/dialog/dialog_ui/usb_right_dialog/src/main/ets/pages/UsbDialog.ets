/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

import { AlertDialog } from '@ohos.arkui.advanced.Dialog'
import usbMgr from '@ohos.usbManager'
import { terminateDialog } from '../util/DialogUtil';
import serialManager from '@ohos.usbManager.serial'

@Entry
@Component
struct IndexHapComponent {
  confirmDialogController: CustomDialogController = new CustomDialogController({
    builder: AlertDialog({
      primaryTitle: $r('app.string.usb_right_title'),
      content: $r('app.string.usb_right_content', globalThis.want?.parameters?.['appName'], globalThis.want?.parameters?.['productName']),
      primaryButton: {
        value: $r('app.string.notAllowButton'),
        action: () => {
          console.info('USBRight not allowed');
          this.closeDialog()
        },
      },
      secondaryButton: {
        value: $r('app.string.allowButton'),
        action: () => {
          try {
            if (globalThis.want.parameters['accessory'] === undefined) {
              if (globalThis.want.parameters['portId'] === undefined) {
                console.info('USBRight allowed');
                let res = usbMgr.addDeviceAccessRight(globalThis.want.parameters['tokenId'],
                  globalThis.want.parameters['deviceName'])
                console.info(`USBRight addDeviceAccessRight result:${res}`);
              } else {
                console.info('SERIALRight allowed');
                serialManager.addSerialRight(Number(globalThis.want.parameters['tokenId']),
                  Number(globalThis.want.parameters['portId']));
              }
            } else {
              let accessoryInfo: usbMgr.USBAccessory = JSON.parse(globalThis.want.parameters['accessory']);
              console.info('USB accessory right allowed: ' + globalThis.want.parameters['accessory']);
              usbMgr.addAccessoryRight(parseInt(globalThis.want.parameters['tokenId']), accessoryInfo)
            }
          } catch {
            console.error('USBRight ConfirmDialog onClick error!');
          } finally {
            this.closeDialog()
          }
        }
      },
    }),
    autoCancel: false,
    cancel: () => {
      console.info('USBRight dialog canceled !');
      this.closeDialog()
    }
  })

  closeDialog() {
    console.info('USBRight close dialog');
    terminateDialog();
  }

  aboutToAppear(): void {
    console.info('USBRight show dialog', JSON.stringify(globalThis.want));
    setTimeout(() => {
        this.confirmDialogController.open();
    });
  }

  build() {
  }
}