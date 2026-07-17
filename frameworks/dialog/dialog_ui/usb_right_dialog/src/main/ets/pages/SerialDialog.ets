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

import { AlertDialog } from '@ohos.arkui.advanced.Dialog'
import { terminateDialog } from '../util/DialogUtil';
import serial from '@ohos.busManager.serial'

@Entry
@Component
struct SerialDialogComponent {
  confirmDialogController: CustomDialogController = new CustomDialogController({
    builder: AlertDialog({
      primaryTitle: $r('app.string.serial_right_title'),
      content: $r('app.string.usb_right_content', globalThis.want?.parameters?.['appName'],
        globalThis.want?.parameters?.['portName']),
      primaryButton: {
        value: $r('app.string.notAllowButton'),
        action: () => {
          console.info('SerialRight not allowed');
          this.closeDialog()
        },
      },
      secondaryButton: {
        value: $r('app.string.allowButton'),
        action: () => {
          try {
            let tokenId: string = globalThis.want.parameters['tokenId'];
            let deviceId: string = globalThis.want.parameters['deviceId'];
            console.info('SerialRight allowed');
            serial.addPortAuthorization(tokenId, deviceId).then(() => {
              console.info('SerialRight addPortAuthorization success');
            }).catch((err: Error) => {
              console.error('SerialRight addPortAuthorization failed: ' + JSON.stringify(err));
            });
          } catch (err) {
            console.error('SerialRight ConfirmDialog onClick error: ' + JSON.stringify(err));
          } finally {
            this.closeDialog()
          }
        }
      },
    }),
    autoCancel: false,
    cancel: () => {
      console.info('SerialRight dialog canceled');
      this.closeDialog()
    }
  })

  closeDialog() {
    console.info('SerialRight close dialog');
    terminateDialog();
  }

  aboutToAppear(): void {
    console.info('SerialRight show dialog', JSON.stringify(globalThis.want));
    setTimeout(() => {
        this.confirmDialogController.open();
    });
  }

  build() {
  }
}
