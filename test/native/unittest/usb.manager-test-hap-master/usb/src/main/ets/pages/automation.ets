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

import usb from '@ohos.usbManager';
import promptAction from '@ohos.promptAction';
import router from '@ohos.router';
import usbManager from '@ohos.enterprise.usbManager';
import buffer from '@ohos.buffer';

interface TestResult {
  name: string;
  result: string;
}

interface DeviceDescriptor {
  length: number;
  descriptorType: number;
  usbVersion: number;
  deviceClass: number;
  deviceSubClass: number;
  deviceProtocol: number;
  maxPacketSize: number;
  vendorId: number;
  productId: number;
  deviceVersion: number;
  manufacturer: number;
  product: number;
  serialNumber: number;
  numConfigurations: number;
}

enum FunctionType {
  NONE = 0,
  ACM = 1,
  ECM = 2,
  HDC = 4,
  MTP = 8,
  PTP = 16,
  RNDIS = 32,
  MIDI = 64,
  AUDIO_SOURCE = 128,
  NCM = 256
}

@Entry
@Component
struct Automation {
  @State testResults: Array<TestResult> = [];
  @State testInProgress: boolean = false;
  @State devices: Array<usb.USBDevice> = [];
  @State selectedDevice: usb.USBDevice | null = null;
  @State showDeviceSelection: boolean = true;
  @State end: boolean = false;
  @State progress: number = 0;

  private bulkResult = new Uint8Array([85,83,66,83,36,0,149,227,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0])
  private testPipe: usb.USBDevicePipe | null = null;

  aboutToAppear() {
    this.refreshDevices();
  }

  refreshDevices() {
    this.devices = usb.getDevices();
    if (this.devices.length === 0) {
      promptAction.showToast({ message: '没有找到USB设备' });
    }
  }

  reset(){
    this.testResults = [];
    this.testInProgress = false
    this.devices = []
    this.selectedDevice = null
    this.end = false
    this.progress = 0
    this.testPipe = null
  }

  addTestResult(name: string, result: string) {
    this.testResults.push({ name, result });
  }

  private sendData : Uint8Array = new Uint8Array([
    0x12, // Operation Code (INQUIRY)
    0x00, // EVPD (Vital Product Data)
    0x00, // Page Code
    0x00, // Allocation Length (high byte)
    0x24, // Allocation Length (low byte)
    0x00  // Control
  ]);

  compareUint8Arrays(arr1: Uint8Array, arr2: Uint8Array): boolean {
  if (arr1.length !== arr2.length) {
    return false;
  }

  for (let i = 0; i < arr1.length; i++) {
    if (arr1[i] !== arr2[i]) {
      return false;
    }
  }

  return true;
}


  async runTests() {
    this.testInProgress = true;
    this.testResults = [];
    this.progress = 0;

    if (!this.selectedDevice) {
      this.addTestResult('初始化失败', '没有选择USB设备');
      this.testInProgress = false;
      return;
    }

    this.addTestResult('初始化成功', `USB为${this.selectedDevice.productName}`);
    this.updateProgress()
    // await this.testRemoveAndAddRight();
    await this.testConnectAndClaimInterface();
    this.updateProgress()
    await this.testSetConfigurationAndInterface();
    this.updateProgress()
    await this.testGetDescriptors();
    this.updateProgress()
    await this.testDeviceInfo();
    this.updateProgress()
    await this.testControlTransfers();
    this.updateProgress()
    // await this.testBulkTransfers();
    // this.updateProgress()
    await this.testReleaseInterface()
    this.updateProgress()
    await this.testSetDeviceFunctions()
    this.updateProgress()


    this.testInProgress = false;
    this.end = true
  }

  updateProgress(){
    const total = 8
    this.progress = this.progress + 100/total
  }


  async testRemoveAndAddRight() {
    try {
      // const added = usb.addRight('com.example.usbtest', this.selectedDevice.name);
      // const added = usb.requestRight(this.selectedDevice.name);
      // this.addTestResult('Add Right', added ? 'Success' : 'Failed');
      if(this.selectedDevice == null) return
      const removed = usb.removeRight(this.selectedDevice.name);
      this.addTestResult('权限移除测试', removed ? '成功' : '失败');
    } catch (error) {
      this.addTestResult('增加/移除权限测试', `失败，失败原因: ${error.message}`);
    }
  }

  async testConnectAndClaimInterface() {
    try {
      this.testPipe = await usb.connectDevice(this.selectedDevice);
      this.addTestResult('设备连接测试', `成功`);

      if(this.selectedDevice == null) return

      if (this.selectedDevice.configs[0] && this.selectedDevice.configs[0].interfaces[0]) {
        const result = usb.claimInterface(this.testPipe, this.selectedDevice.configs[0].interfaces[0], true);
        this.addTestResult('注册通信接口测试', result === 0 ? '成功' : `失败: ${result}`);
      } else {
        this.addTestResult('注册通信接口测试', '无法获取通讯接口');
      }
    } catch (error) {
      this.addTestResult('设备连接/通讯接口测试', `失败，失败原因: ${error.message}`);
    }
  }

  async testSetConfigurationAndInterface() {
    if(this.selectedDevice == null) return

    if (!this.testPipe || !this.selectedDevice.configs[0]) return;

    try {
      const configResult = usb.setConfiguration(this.testPipe, this.selectedDevice.configs[0]);
      this.addTestResult('设置设备配置测试', configResult === 0 ? '成功' : `失败: ${configResult}`);

      if (this.selectedDevice.configs[0].interfaces[0]) {
        const interfaceResult = usb.setInterface(this.testPipe, this.selectedDevice.configs[0].interfaces[0]);
        this.addTestResult('设置设备接口测试', interfaceResult === 0 ? '成功' : `失败: ${interfaceResult}`);
      }
    } catch (error) {
      this.addTestResult('Set Config/Interface', `Error: ${error.message}`);
    }
  }

  parseUsbDescriptor(rawDescriptor: Uint8Array): string {
    // Here we parse the raw descriptor and extract meaningful information
    // This is a simplified example and may need to be extended for full parsing
    const view = new DataView(rawDescriptor.buffer);

    const deviceDescriptor: DeviceDescriptor = {
      length: view.getUint8(0),
      descriptorType: view.getUint8(1),
      usbVersion: view.getUint16(2, true),
      deviceClass: view.getUint8(4),
      deviceSubClass: view.getUint8(5),
      deviceProtocol: view.getUint8(6),
      maxPacketSize: view.getUint8(7),
      vendorId: view.getUint16(8, true),
      productId: view.getUint16(10, true),
      deviceVersion: view.getUint16(12, true),
      manufacturer: view.getUint8(14),
      product: view.getUint8(15),
      serialNumber: view.getUint8(16),
      numConfigurations: view.getUint8(17),
    };

    return `Length: ${deviceDescriptor.length}
Descriptor Type: ${deviceDescriptor.descriptorType}
USB Version: ${deviceDescriptor.usbVersion}
Device Class: ${deviceDescriptor.deviceClass}
Device SubClass: ${deviceDescriptor.deviceSubClass}
Device Protocol: ${deviceDescriptor.deviceProtocol}
Max Packet Size: ${deviceDescriptor.maxPacketSize}
Vendor ID: ${deviceDescriptor.vendorId}
Product ID: ${deviceDescriptor.productId}
Device Version: ${deviceDescriptor.deviceVersion}
Manufacturer Index: ${deviceDescriptor.manufacturer}
Product Index: ${deviceDescriptor.product}
Serial Number Index: ${deviceDescriptor.serialNumber}
Configuration num: ${deviceDescriptor.numConfigurations}
    `
  }

  async testGetDescriptors() {
    if (!this.testPipe) return;

    try {
      const rawDescriptor = usb.getRawDescriptor(this.testPipe);
      this.addTestResult('原始USB描述符获取测试', `成功: ${rawDescriptor.byteLength} bytes`);
      this.addTestResult('原始USB描述符', `${rawDescriptor}`);
      const parsedDescriptor = this.parseUsbDescriptor(rawDescriptor);
      this.addTestResult('Device Descriptor 解析', `${parsedDescriptor}`);
      // const fileDescriptor = usb.getFileDescriptor(this.testPipe);
      // if (fileDescriptor == -1){
      //   this.addTestResult('获取文件描述符测试', `失败, 文件描述符为 ${fileDescriptor}`);
      // }else{
      //   this.addTestResult('获取文件描述符测试', `成功, 文件描述符为 ${fileDescriptor}`);
      // }
    } catch (error) {
      this.addTestResult('获取文件描述符测试', `失败，失败原因: ${error.message}`);
    }
  }

  async testDeviceInfo() {
    if (this.selectedDevice == null) return;

    try {
      const deviceInfo = `设备名称: ${this.selectedDevice.name}
制造商: ${this.selectedDevice.manufacturerName}
产品名: ${this.selectedDevice.productName}
设备版本: ${this.selectedDevice.version}
总线号: ${this.selectedDevice.busNum}
设备地址: ${this.selectedDevice.devAddress}
供应商ID: ${this.selectedDevice.vendorId}
产品ID: ${this.selectedDevice.productId}
类: ${this.selectedDevice.clazz}
子类: ${this.selectedDevice.subClass}
协议: ${this.selectedDevice.protocol}`;
      this.addTestResult('API设备信息获取', deviceInfo);
    } catch (error) {
      this.addTestResult('设备信息获取失败', `失败原因: ${error.message}`);
    }
  }

  async testControlTransfers() {
    if(this.selectedDevice == null) return;
    if (!this.testPipe || !this.selectedDevice.configs[0] || !this.selectedDevice.configs[0].interfaces[0]) return;

    try {
      let controlParam: usb.USBControlParams = {
        request: 6, // GET_DESCRIPTOR
        reqType: 0x80, // Standard request
        target: 0, // Device
        value: (1 << 8), // Descriptor type (1 for device descriptor) in the high byte
        index: 0, // Index (0 for device descriptor)
        data: new Uint8Array(18) // Buffer to receive 18 bytes of the device descriptor
      };

      usb.controlTransfer(this.testPipe, controlParam).then((ret: number) => {
        console.log(`controlTransfer = ${ret}`);
        this.addTestResult('控制传输测试(读取Descriptor)', `成功: ${ret} bytes`);
        this.addTestResult('控制传输读取数据', `${controlParam.data}`);
        const newParsedDescriptor = this.parseUsbDescriptor(controlParam.data);
        this.addTestResult('控制传输读取数据解析', `${newParsedDescriptor}`);
      })
    } catch (error) {
      this.addTestResult('传输测试', `失败，失败原因: ${error.message}`);
    }
  }



  async performWriteBulkTransfer(pipe:usb.USBDevicePipe, endpoint: usb.USBEndpoint) {
  try {

    // Perform the bulk transfer to read data from the USB disk
    const bytesRead: number = await usb.bulkTransfer(pipe, endpoint, this.sendData);

    if (bytesRead !== -1) {
      this.addTestResult('批量传输测试写入', `成功: ${bytesRead} bytes, 写入data为 SCSI INQUIRY command \n ${this.sendData}`);
      console.log(`Write ${bytesRead} bytes successfully.`);
    } else {
      console.log("Bulk transfer failed.");
    }
  } catch (error) {
      console.error("An unexpected error occurred:", error);
    }
  }


  async performReadBulkTransfer(pipe:usb.USBDevicePipe, endpoint: usb.USBEndpoint) {
  try {
    const buffer = new Uint8Array(36);

    const bytesWritten : number = await usb.bulkTransfer(pipe, endpoint, buffer);

    if (bytesWritten !== -1) {
      console.log(`Read ${bytesWritten} bytes successfully, ${buffer}`);
      let flag = this.compareUint8Arrays(buffer, this.bulkResult)
      if (flag){
        this.addTestResult('批量传输测试读出', `成功: ${bytesWritten} bytes，读出数据与预期一致`);
      }else{
        this.addTestResult('批量传输测试读出', `失败: ${bytesWritten} bytes，读出数据与预期不匹配. 读出结果：${buffer}, 预期结果: ${this.bulkResult}`);
      }
      console.log(`读出数据为： ${buffer}`)
    } else {
      console.log("Bulk transfer failed.");
    }

  } catch (error) {
      console.error("An unexpected error occurred:", error);
  }
}

  async testBulkTransfers() {
    if (this.selectedDevice == null) return;
    if (!this.testPipe || !this.selectedDevice.configs[0] || !this.selectedDevice.configs[0].interfaces[0]) return;
    if (this.testPipe == null) return;

    try {
      if (this.selectedDevice.configs[0].interfaces[0].endpoints[0]) {
        this.performWriteBulkTransfer(this.testPipe, this.selectedDevice.configs[0].interfaces[0].endpoints[1]);
        // this.performReadBulkTransfer(this.testPipe, this.selectedDevice.configs[0].interfaces[0].endpoints[1]);
        this.performReadBulkTransfer(this.testPipe, this.selectedDevice.configs[0].interfaces[0].endpoints[0]);
        this.performReadBulkTransfer(this.testPipe, this.selectedDevice.configs[0].interfaces[0].endpoints[0]);
        // this.performReadBulkTransfer(this.testPipe, this.selectedDevice.configs[0].interfaces[0].endpoints[0]);
        // this.performReadBulkTransfer(this.testPipe, this.selectedDevice.configs[0].interfaces[0].endpoints[0]);
      }
    } catch (error) {
      this.addTestResult('传输测试', `失败，失败原因: ${error.message}`);
    }
  }

  async testReleaseInterface(){
    if (this.testPipe == null) return;
    // if (!this.testPipe || !this.selectedDevice.configs[0] || !this.selectedDevice.configs[0].interfaces[0]) return;
    if (this.selectedDevice == null) return;

    try{
      let result : number = usb.releaseInterface(this.testPipe, this.selectedDevice.configs[0].interfaces[0])
      if (result == 0){
        this.addTestResult('接口释放测试', `成功`);
      }else{
        this.addTestResult('接口释放测试', `失败，失败返回值为${result}`);
      }
    } catch (error ){
      this.addTestResult('接口释放测试', `失败，失败原因: ${error.message}`);
    }

  }

  getCurrentFunctions(store: FunctionType): string{
    switch(store){
      case 0: return "None";
      case 1: return "ACM";
      case 2: return "ECM";
      case 4: return "HDC";
      case 8: return "MTP";
      case 16: return "PTP";
      case 32: return "RNDIS";
      case 64: return "MIDI";
      case 128: return "AUDIO_SOURCE";
      case 256: return "NCM";
      default: return "未知";
    }
  }

  async testSetDeviceFunctions() {
    if (!this.selectedDevice) {
      this.addTestResult('设置设备功能测试', '未选择设备');
      return;
    }

    try {
      // Define the sequence of functions to test
      const functionSequence: FunctionType[] = [
        FunctionType.NONE,
        FunctionType.ACM,
        FunctionType.ECM,
        FunctionType.MTP,
        FunctionType.PTP,
        FunctionType.RNDIS,
        FunctionType.HDC,
      ];

      for (const func of functionSequence) {
        await usb.setDeviceFunctions(func);
        const currentFunctions = usb.getDeviceFunctions();

        if (currentFunctions === func) {
          this.addTestResult(`设置设备功能为 ${this.getCurrentFunctions(func)}`, '成功');
        } else {
          this.addTestResult(`设置设备功能为 ${this.getCurrentFunctions(func)}`, '失败');
        }
      }
    } catch (error) {
      this.addTestResult('设置设备功能测试', `失败，失败原因: ${error.message}`);
    }
  }

  // async testFunctionConversions() {
  //   try {
  //     const funcNum = usb.usbFunctionsFromString('mtp');
  //     this.addTestResult('Functions From String', `Success: ${funcNum}`);
  //
  //     const funcString = usb.usbFunctionsToString(funcNum);
  //     this.addTestResult('Functions To String', `Success: ${funcString}`);
  //   } catch (error) {
  //     this.addTestResult('Function Conversions', `Error: ${error.message}`);
  //   }
  // }
  //
  // async testCurrentFunctions() {
  //   try {
  //     const currentFunctions = usb.getCurrentFunctions();
  //     this.addTestResult('Get Current Functions', `Success: ${currentFunctions}`);
  //
  //     await usb.setCurrentFunctions(usb.FunctionType.FUNCTION_MTP);
  //     this.addTestResult('Set Current Functions', 'Success');
  //   } catch (error) {
  //     this.addTestResult('Current Functions', `Error: ${error.message}`);
  //   }
  // }
  //
  // async testPortOperations() {
  //   try {
  //     const ports = usb.getPorts();
  //     this.addTestResult('Get Ports', `Success: ${ports.length} ports`);
  //
  //     if (ports.length > 0) {
  //       const supportedModes = usb.getSupportedModes(ports[0].id);
  //       this.addTestResult('Get Supported Modes', `Success: ${supportedModes}`);
  //
  //       await usb.setPortRoles(ports[0].id, usb.PowerRoleType.SOURCE, usb.DataRoleType.HOST);
  //       this.addTestResult('Set Port Roles', 'Success');
  //     }
  //   } catch (error) {
  //     this.addTestResult('Port Operations', `Error: ${error.message}`);
  //   }
  // }

  build() {
    Column() {
        Image($r('app.media.back'))
          .width(18)
          .height(18)
          .onClick(() => {
            this.reset()
            this.showDeviceSelection = true
            router.back()
          })
          .alignSelf(ItemAlign.Start)


        Text('USB自动化接口测试')
          .fontSize(24)
          .fontWeight(FontWeight.Bold)
          .textAlign(TextAlign.Center)
          .margin({ bottom: 20 })

      if (this.showDeviceSelection) {
        Button('刷新设备列表')
          .onClick(() => this.refreshDevices())
          .margin({ bottom: 40 })

        if(this.devices.length !== 0){
          Text('选择一个设备进行测试:')
            .fontSize(18)
            .fontWeight(FontWeight.Bold)
            .margin({ bottom: 10 })
        }

        List() {
          ForEach(this.devices, (device: usb.USBDevice) => {
            ListItem() {
              Button(){
                Column() {
                  Text(`设备名称: ${device.productName}`)
                    .fontSize(16)
                    .padding(5);
                  Text(`供应商ID: ${device.vendorId}`)
                    .fontSize(16)
                    .padding(5);
                  Text(`产品ID: ${device.productId}`)
                    .fontSize(16)
                    .padding(5);
                }
                .alignItems(HorizontalAlign.Center)
                .onClick(() => {
                  this.selectedDevice = device;
                  this.showDeviceSelection = false;
                  promptAction.showToast({ message: `已选择设备: ${device.productName}` });
                })
                .padding(10)
                .margin({ bottom: 10 });
              }
              .backgroundColor(this.selectedDevice === device ? Color.Gray : Color.White)
            }
          })
        }
        .width('100%')
        .height('40%')
        .margin({ bottom: 20 })
      } else {
        Button('返回设备选择')
          .onClick(() => {
            this.reset()
            this.showDeviceSelection = true;
            })
          .margin({ bottom: 20 })
      }

      if (!this.showDeviceSelection && !this.end && !this.testInProgress) {
        Button('开始测试')
          .onClick(() => this.runTests())
          .enabled(!this.testInProgress && !!this.selectedDevice)
          .margin({ bottom: 20 })

      }

      if(!this.showDeviceSelection && this.testInProgress){
        Progress({
          value: this.progress,
        }).margin({ bottom: 20 });
      }


      List() {
        ForEach(this.testResults, (result: TestResult) => {
          ListItem() {
            Row() {
              Text(result.name)
                .width('50%')
              Text(result.result)
                .width('50%')
            }
            .padding(10)
            .backgroundColor(Color.White)
          }
        })
      }
      .width('100%')
      .height('80%')
    }
    .width('100%')
    .height('100%')
    .padding(20)
  }
}