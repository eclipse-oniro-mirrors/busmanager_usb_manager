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

import usb from "@ohos.usbManager";
import promptAction from '@ohos.promptAction';
import router from '@ohos.router';

@Entry
@Component
struct UsbInfoPage {
  @State devices: Array<usb.USBDevice> = [];
  @State selectedDevice: usb.USBDevice | null = null;
  @State deviceRights: Map<string, boolean> = new Map();

  aboutToAppear() {
    this.refreshDevices();
  }

  refreshDevices() {
    let devicesList: Array<usb.USBDevice> = usb.getDevices();
    if (devicesList.length == 0) {
      console.warn("no device is connected")
      promptAction.showToast({ message: '没有找到USB设备' })
      this.devices = []
    } else {
      this.devices = devicesList
      this.updateDeviceRights();
    }
  }

  updateDeviceRights() {
    this.deviceRights.clear();
    this.devices.forEach(device => {
      let right: boolean = usb.hasRight(device.name)
      this.deviceRights.set(device.name, right);
    });
  }

  removeRight(deviceName: string): boolean {
    try {
      usb.removeRight(deviceName);
      return true;
    } catch (error) {
      promptAction.showToast({ message: `权限移除失败: ${error.message}` });
      return false;
    }
  }

  build() {
    Column() {
      Image($r('app.media.back'))
        .width(18)
        .height(18)
        .onClick(() => router.back())
        .alignSelf(ItemAlign.Start)

      Text('USB设备信息')
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .margin({ bottom: 20 })

      Button('刷新设备列表')
        .onClick(() => {
          this.refreshDevices();
        })
        .margin({ bottom: 20 })

      Scroll() {
        Column() {
          ForEach(this.devices, (device: usb.USBDevice, index) => {
            ListItem() {
              Button() {
                Column() {
                  Text(`设备名称: ${device.name}`)
                  Text(`制造商: ${device.manufacturerName}`)
                  Text(`产品名: ${device.productName}`)
                  Text(`版本: ${device.version}`)
                  Text(`总线号: ${device.busNum}`)
                  Text(`设备地址: ${device.devAddress}`)
                  Text(`序列号: ${device.serial}`)
                  Text(`厂商ID: ${device.vendorId}`)
                  Text(`产品ID: ${device.productId}`)
                  Text(`类: ${device.clazz}`)
                  Text(`子类: ${device.subClass}`)
                  Text(`协议: ${device.protocol}`)
                  Text(`是否有权限访问: ${this.deviceRights.get(device.name) ? '是' : '否'}`)
                }
                .padding(10)
              }
              .backgroundColor(this.selectedDevice === device ? Color.Gray : Color.White)
              .borderRadius(0) // Make the corners square
              .onClick(() => {
                this.selectedDevice = device;
                promptAction.showToast({ message: `Selected device: ${device.productName}`, bottom:"120vp" })
              })
              .padding(10)
            }
            .width('100%')
          })
        }
        .width('100%')
      }
      .width('100%')
      .height('70%')

      if (this.selectedDevice) {
        Column() {
          Text('选中设备操作')
            .fontSize(20)
            .fontWeight(FontWeight.Bold)
            .margin({ top: 20, bottom: 20 })

          Row() {
            Button('请求权限', { type: ButtonType.Capsule, stateEffect: true })
              .onClick(() => {
                if (this.selectedDevice == null) return
                usb.requestRight(this.selectedDevice.name)
                  .then((hasRight) => {
                    if (hasRight) {
                      promptAction.showToast({ message: '权限请求成功', bottom:"120vp" });
                      this.updateDeviceRights();
                    } else {
                      promptAction.showToast({ message: '权限被拒绝', bottom:"120vp"  });
                    }
                  })
                  .catch((error: Error) => {
                    promptAction.showToast({ message: `权限请求失败: ${error.message}`, bottom:"120vp"  });
                  });
              })
              .margin({ right: 20 })

            Button('移除权限', { type: ButtonType.Capsule, stateEffect: false })
              .onClick(() => {
                if (this.selectedDevice == null) return
                let result = this.removeRight(this.selectedDevice.name);
                if (result) {
                  promptAction.showToast({ message: '权限移除成功', bottom:"120vp"  });
                  this.updateDeviceRights();
                }
              })
              .margin({ left: 20 })
          }
          .justifyContent(FlexAlign.Center)
          .width('100%')
          .margin({ bottom: 10 })
        }
        .width('100%')
      }
    }
    .width('100%')
    .height('100%')
    .padding(20)
  }
}