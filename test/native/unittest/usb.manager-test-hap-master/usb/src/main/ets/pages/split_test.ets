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
struct UsbTestPage {
  @State devices: Array<usb.USBDevice> = [];
  @State selectedDevice: usb.USBDevice | null = null;
  @State devicePipe: usb.USBDevicePipe | null = null;
  @State output: string = '';
  @State expandedInterface: number | null = null; // Track which interface is expanded

  aboutToAppear() {
    this.refreshDevices();
  }

  refreshDevices() {
    let devicesList: Array<usb.USBDevice> = usb.getDevices();
    if (devicesList.length == 0) {
      console.warn("no device is connected");
      promptAction.showToast({ message: '没有找到USB设备' });
      this.devices = [];
    } else {
      this.devices = devicesList;
    }
  }

  connectDevice(device: usb.USBDevice) {
    try {
      let pipe = usb.connectDevice(device);
      this.devicePipe = pipe;
      this.output = `已连接到设备: ${device.name}`;
    } catch (error) {
      this.output = `连接设备失败: ${error.message}`;
    }
  }

  claimInterface(pipe: usb.USBDevicePipe, iface: usb.USBInterface) {
    try {
      let result = usb.claimInterface(pipe, iface, true);
      if (result < 0) {
        this.output = `通信接口注册失败(claimInterface): ${result}`;
      } else {
        this.output = `通信接口已注册(claimInterface): ${result}`;
      }

    } catch (error) {
      this.output = `通信接口注册失败(claimInterface): ${error.message}`;
    }
  }

  releaseInterface(pipe: usb.USBDevicePipe, iface: usb.USBInterface) {
    try {
      let result = usb.releaseInterface(pipe, iface);
      this.output = `通信接口释放成功(releaseInterface): ${result}`;
    } catch (error) {
      this.output = `通信接口释放失败(releaseInterface): ${error.message}`;
    }
  }

  setInterface(pipe: usb.USBDevicePipe, iface: usb.USBInterface) {
    try {
      let result = usb.setInterface(pipe, iface);
      this.output = `设置设备接口成功(setInterface): ${result}`;
    } catch (error) {
      this.output = `设置设备接口失败(setInterface): ${error.message}`;
    }
  }

  closePipe(pipe: usb.USBDevicePipe) {
    try {
      let result = usb.closePipe(pipe);
      this.output = `关闭设备消息控制通道成功(closePipe): ${result}`;
      this.devicePipe = null;
    } catch (error) {
      this.output = `关闭设备消息控制通道失败(closePipe): ${error.message}`;
    }
  }

  setConfiguration(pipe: usb.USBDevicePipe, config: usb.USBConfiguration) {
    try {
      let result = usb.setConfiguration(pipe, config);
      this.output = `设置设备配置成功(setConfiguration): ${result}`;
    } catch (error) {
      this.output = `设置设备配置失败(setConfiguration): ${error.message}`;
    }
  }

  getInterfaces(): Array<usb.USBInterface> {
    if (this.selectedDevice && this.selectedDevice.configs.length > 0) {
      return this.selectedDevice.configs[0].interfaces; // Assuming the first config
    }
    return [];
  }

  toggleInterface(index: number) {
    if (this.expandedInterface === index) {
      this.expandedInterface = null;
    } else {
      this.expandedInterface = index;
    }
  }

  build() {
    Column() {
      Image($r('app.media.back'))
        .width(18)
        .height(18)
        .onClick(() => router.back())
        .alignSelf(ItemAlign.Start)

      Text('USB接口分离测试')
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .margin({ bottom: 20 })

      if (!this.selectedDevice) {
        Button('刷新设备列表')
          .onClick(() => {
            this.refreshDevices();
          })
          .margin({ bottom: 20 })
          .width('100%')

        Scroll() {
          List() {
            ForEach(this.devices, (device: usb.USBDevice, index) => {
              ListItem() {
                Row() {
                  Column() {
                    Text(`设备名称: ${device.name}`)
                    Text(`制造商: ${device.manufacturerName}`)
                    Text(`产品名: ${device.productName}`)
                    Text(`版本: ${device.version}`)
                  }
                  .layoutWeight(1)

                  Radio({ value: index.toString(), group: 'devices' })
                    .onChange((isChecked: boolean) => {
                      if (isChecked) {
                        this.selectedDevice = device;
                        promptAction.showToast({ message: `已选择设备: ${device.productName}` });
                      }
                    })
                }
                .width('100%')
                .padding(10)
                .backgroundColor(this.selectedDevice === device ? Color.Gray : Color.White)
              }
            })
          }
          .width('100%')
          .height('50%')
        }
      } else {
        Scroll() {
          Column() {

            Text(`设备名称: ${this.selectedDevice?.name}`)
              .fontSize(20)
              .fontWeight(FontWeight.Bold)
              .margin({ top: 20, bottom: 10 })

            Text(`制造商: ${this.selectedDevice?.manufacturerName}`)
              .fontSize(20)
              .fontWeight(FontWeight.Bold)
              .margin({ bottom: 20 })

            Button('连接设备')
              .onClick(() => {
                if (this.selectedDevice) {
                  this.connectDevice(this.selectedDevice);
                }
              })
              .margin({ bottom: 10 })
              .width('100%')

            if (this.devicePipe) {
              ForEach(this.getInterfaces(), (iface: usb.USBInterface, index) => {
                Column() {
                  Button(`Interface ${index + 1}`)
                    .onClick(() => {
                      this.toggleInterface(index);
                    })
                    .margin({ bottom: 10 })
                    .width('100%')

                  if (this.expandedInterface === index) {
                    Column() {
                      Button(`注册通信接口`)
                        .onClick(() => {
                          if (this.devicePipe) {
                            this.claimInterface(this.devicePipe, iface);
                          }
                        })
                        .margin({ bottom: 10 })
                        .width('100%')
                        .backgroundColor(Color.Grey) // Different color for sub-buttons

                      Button(`设置设备接口`)
                        .onClick(() => {
                          if (this.devicePipe) {
                            this.setInterface(this.devicePipe, iface);
                          }
                        })
                        .margin({ bottom: 10 })
                        .width('100%')
                        .backgroundColor(Color.Grey) // Different color for sub-buttons

                      Button(`释放通信接口`)
                        .onClick(() => {
                          if (this.devicePipe) {
                            this.releaseInterface(this.devicePipe, iface);
                          }
                        })
                        .margin({ bottom: 10 })
                        .width('100%')
                        .backgroundColor(Color.Grey) // Different color for sub-buttons

                    }
                    .margin({ left: 20 })
                  }
                }
                .width('100%')
              })

              ForEach(this.selectedDevice?.configs, (config: usb.USBConfiguration, index) => {
                Column() {
                  Button(`设置设备配置`)
                    .onClick(() => {
                      if (this.devicePipe) {
                        this.setConfiguration(this.devicePipe, config);
                      }
                    })
                    .margin({ bottom: 10 })
                    .width('100%')
                }
                .width('100%')
              })

              Button('关闭设备消息控制通道')
                .onClick(() => {
                  if (this.devicePipe) {
                    this.closePipe(this.devicePipe);
                  }
                })
                .margin({ bottom: 10 })
                .width('100%')
            }
          }
          .width('100%')
        }
      }

      Text(this.output)
        .fontSize(18)
        .margin({ top: 20 })
    }
    .width('100%')
    .height('100%')
    .padding(20)
  }
}