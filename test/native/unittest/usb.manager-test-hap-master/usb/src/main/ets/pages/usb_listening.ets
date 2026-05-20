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

import promptAction from '@ohos.promptAction';
import commonEvent from '@ohos.commonEventManager';
import router from '@ohos.router';

// Define interfaces and types
interface Endpoint {
  address: number;
  attributes: number;
  interval: number;
  maxPacketSize: number;
  direction: number;
  number: number;
  type: number;
  interfaceId: number;
}

interface UsbInterface {
  id: number;
  protocol: number;
  clazz: number;
  subClass: number;
  alternateSetting: number;
  name: string;
  endpoints: Endpoint[];
}

interface UsbConfig {
  id: number;
  attributes: number;
  maxPower: number;
  name: string;
  isRemoteWakeup: boolean;
  isSelfPowered: boolean;
  interfaces: UsbInterface[];
}

interface UsbDeviceEventData {
  busNum: number;
  devAddress: number;
  serial: string;
  name: string;
  manufacturerName: string;
  productName: string;
  version: string;
  vendorId: number;
  productId: number;
  clazz: number;
  subClass: number;
  protocol: number;
  configs: UsbConfig[];
}

interface USBDeviceInfo {
  productName: string;
  manufacturerName: string;
}

interface SubscriberInfo {
  events: string[];
}

@Entry
@Component
struct UsbEventPage {
  @State deviceInfo: USBDeviceInfo | null = null;
  @State isListening: boolean = false;
  @State eventData: string | null = null;  // Used to store event data string
  @State usbDeviceCount: number = 0;  // Count of connected USB devices
  private usbSubscriber: commonEvent.CommonEventSubscriber | null = null;

  aboutToAppear() {
  }

  aboutToDisappear() {
    this.stopUsbListening();
  }

  toggleUsbEvents() {
    if (this.isListening) {
      this.stopUsbListening();
      this.usbDeviceCount = 0
      this.eventData = null
      this.deviceInfo = null
    } else {
      this.startUsbListening();
    }
    this.isListening = !this.isListening;
  }

  startUsbListening() {
    const subscriberInfo: SubscriberInfo = {
      events: [
        'usual.event.hardware.usb.action.USB_DEVICE_ATTACHED',
        'usual.event.hardware.usb.action.USB_DEVICE_DETACHED'
      ]
    };

    commonEvent.createSubscriber(subscriberInfo, (err, subscriber) => {
      if (err) {
        promptAction.showToast({ message: `创建订阅者失败: ${err.message}` });
        return;
      }

      this.usbSubscriber = subscriber;

      commonEvent.subscribe(this.usbSubscriber, (err, event) => {
        if (err) {
          promptAction.showToast({ message: `订阅事件失败: ${err.message}` });
          return;
        }

        if (event.event === 'usual.event.hardware.usb.action.USB_DEVICE_ATTACHED') {
          this.handleUsbDeviceAdd(event);
        } else if (event.event === 'usual.event.hardware.usb.action.USB_DEVICE_DETACHED') {
          this.handleUsbDeviceRemove(event);
        }
      });
    });
  }

  handleUsbDeviceAdd(event: commonEvent.CommonEventData) {
    if (event.data) {
      try {
        const deviceData: UsbDeviceEventData = JSON.parse(event.data);
        this.deviceInfo = {
          productName: deviceData.productName,
          manufacturerName: deviceData.manufacturerName
        };
        this.eventData = this.formatEventData(deviceData);
        this.usbDeviceCount++;  // Increment device count
        promptAction.showToast({ message: `设备已插入: ${deviceData.productName}` });
      } catch (error) {
        this.eventData = '解析设备数据失败';
        promptAction.showToast({ message: '解析设备数据失败' });
      }
    } else {
      this.eventData = '事件数据为空';
      promptAction.showToast({ message: '事件数据为空' });
    }
  }

  handleUsbDeviceRemove(event: commonEvent.CommonEventData) {
    if (this.deviceInfo !== null) {
      promptAction.showToast({ message: `设备已拔出: ${this.deviceInfo.productName}` });
      this.deviceInfo = null;
      if (this.usbDeviceCount > 0) {
        this.usbDeviceCount--;  // Decrement device count
      }
    }
  }

  stopUsbListening() {
    if (this.usbSubscriber !== null) {
      commonEvent.unsubscribe(this.usbSubscriber, (err) => {
        if (err) {
          promptAction.showToast({ message: `取消订阅失败: ${err.message}` });
        }
      });
      this.usbSubscriber = null;
    }
  }

  formatEventData(deviceData: UsbDeviceEventData): string {
    let formattedData = `
      设备名称: ${deviceData.name}\n
      产品名称: ${deviceData.productName}\n
      制造商名称: ${deviceData.manufacturerName}\n
      版本: ${deviceData.version}\n
      供应商ID: ${deviceData.vendorId}\n
      产品ID: ${deviceData.productId}\n
      类别: ${deviceData.clazz}\n
      子类别: ${deviceData.subClass}\n
      协议: ${deviceData.protocol}\n
      总线编号: ${deviceData.busNum}\n
      设备地址: ${deviceData.devAddress}\n
      配置数量: ${deviceData.configs.length}
    `;

    deviceData.configs.forEach((config, configIndex) => {
      formattedData += `\n\n**配置 ${configIndex + 1}:**\n`;
      formattedData += `
        配置ID: ${config.id}\n
        配置名称: ${config.name}\n
        属性: ${config.attributes}\n
        最大功率: ${config.maxPower}\n
        远程唤醒: ${config.isRemoteWakeup}\n
        自供电: ${config.isSelfPowered}\n
        接口数量: ${config.interfaces.length}
      `;

      config.interfaces.forEach((usbInterface, interfaceIndex) => {
        formattedData += `\n  **接口 ${interfaceIndex + 1}:**\n`;
        formattedData += `
          接口ID: ${usbInterface.id}\n
          接口名称: ${usbInterface.name}\n
          类别: ${usbInterface.clazz}\n
          子类别: ${usbInterface.subClass}\n
          协议: ${usbInterface.protocol}\n
          备用设置: ${usbInterface.alternateSetting}\n
          端点数量: ${usbInterface.endpoints.length}
        `;

        usbInterface.endpoints.forEach((endpoint, endpointIndex) => {
          formattedData += `\n    **端点 ${endpointIndex + 1}:**\n`;
          formattedData += `
            地址: ${endpoint.address}\n
            属性: ${endpoint.attributes}\n
            间隔: ${endpoint.interval}\n
            最大包尺寸: ${endpoint.maxPacketSize}\n
            方向: ${endpoint.direction}\n
            编号: ${endpoint.number}\n
            类型: ${endpoint.type}
          `;
        });
      });
    });

    return formattedData;
  }

  build() {
    Column() {
      Image($r('app.media.back'))
        .width(18)
        .height(18)
        .onClick(() => router.back())
        .alignSelf(ItemAlign.Start)

      Text('USB 插拔事件通知')
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .margin({ bottom: 20 })

      Button(this.isListening ? '停止监听 USB 事件' : '开始监听 USB 事件')
        .fontSize(20)
        .onClick(() => this.toggleUsbEvents())
        .margin({ bottom: 20 })

      Text(`连接的 USB 设备数量: ${this.usbDeviceCount}`)
        .fontSize(15)
        .margin({ bottom: 20 })

      if (this.deviceInfo && this.isListening) {
        Column() {
          Text(`产品名称: ${this.deviceInfo.productName}`)
            .fontSize(18)
            .margin({ bottom: 10 })
          Text(`制造商名称: ${this.deviceInfo.manufacturerName}`)
            .fontSize(18)
            .margin({ bottom: 10 })
        }
        .padding(10)
        .backgroundColor(Color.White)
        .borderRadius(10)
      } else {
        Text('没有设备连接')
          .fontSize(20)
      }

      if (this.eventData && this.deviceInfo && this.isListening) {
        // Adding a scrollable area for event data
        Scroll() {
          Text(`${this.eventData}`)
            .fontSize(16)
            .padding(10)
            .backgroundColor(Color.White)
            .borderRadius(10)
        }
        .width('100%')
        .height(500)
        .margin({ top: 20 })
        .backgroundColor(Color.White)
      }
    }
    .width('100%')
    .height('100%')
    .padding(20)
    .backgroundColor(Color.White)
  }
}