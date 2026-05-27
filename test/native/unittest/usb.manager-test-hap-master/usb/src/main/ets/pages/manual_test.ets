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

import router from '@ohos.router';
import usbManager from '@ohos.usbManager';
import promptAction from '@ohos.promptAction';


enum PowerRoleType {
  NONE = 0,
  SOURCE = 1,
  SINK = 2
}

enum DataRoleType {
  NONE = 0,
  HOST = 1,
  DEVICE = 2
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
struct Results {
  @State ports: Array<usbManager.USBPort> = []
  @State functions: string = "none"
  @State selectedPowerRole: PowerRoleType = PowerRoleType.NONE
  @State selectedDataRole: DataRoleType = DataRoleType.NONE
  @State dataRoleText: string = '无'
  @State dataRoleIndex: number = 0
  @State selectedFunction: FunctionType = FunctionType.NONE
  @State functionText: string = '无'

  onPageShow(): void {
    this.fetchPortData()
  }

  fetchPortData() {
    try {
      this.ports = usbManager.getPortList();
      this.functions = this.getCurrentFunctions(this.getDeviceFunctions())
      console.log("获取端口数据成功");
    } catch (error) {
      console.log("获取 USB 端口数据时出错: ", error);
    }
  }

  getModeType(mode: usbManager.PortModeType): string {
    switch (mode) {
      case 0: return '无';
      case 1: return '上行端口';
      case 2: return '下行端口';
      case 3: return '动态重新配置端口';
      default: return '未知';
    }
  }

  getCurrentMode(mode: number): string {
    switch (mode) {
      case 0: return '无';
      case 1: return 'DFP (下行端口)';
      case 2: return 'UFP (上行端口)';
      case 3: return '独立端口';
      default: return '未知';
    }
  }

  getCurrentPowerRole(role: number): string {
    switch (role) {
      case 0: return '无';
      case 1: return '自我供电';
      case 2: return '外部供电';
      default: return '未知';
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

  getCurrentDataRole(role: number): string {
    switch (role) {
      case 0: return '无';
      case 1: return '主机';
      case 2: return '设备';
      default: return '未知';
    }
  }


  async setPortRoleTypes(portId: number, powerRole: PowerRoleType, dataRole: DataRoleType): Promise<void> {
    try {
      await usbManager.setPortRoleTypes(portId, powerRole, dataRole);
      console.log(`设置端口角色成功: 端口ID = ${portId}, 电源角色 = ${powerRole}, 数据角色 = ${dataRole}`);
      this.fetchPortData(); // 更新端口数据
    } catch (error) {
      promptAction.showToast({ message: `${error}` })
      console.log("设置端口角色时出错: ", error);
    }
  }

  getDeviceFunctions(): FunctionType {
    try {
      const functions = usbManager.getDeviceFunctions();
      console.log(`获取当前USB功能成功: ${this.getCurrentFunctions(functions)}`);
      return functions;
    } catch (error) {
      return FunctionType.NONE
      console.log("获取当前USB功能时出错: ", error);
    }
  }

  async setDeviceFunctions(funcs: FunctionType): Promise<void> {
    try {
      await usbManager.setDeviceFunctions(funcs);
      this.fetchPortData();
      console.log(`设置当前USB功能成功: ${funcs}`);
      promptAction.showToast({message:`设置当前USB功能为 ${this.getCurrentFunctions(funcs)} 成功`})
    } catch (error) {
      console.log("设置当前USB功能时出错: ", error);
    }
  }

  build() {
    Column() {
      Image($r('app.media.back'))
        .width(23)
        .height(23)
        .onClick(() => router.back())
        .alignSelf(ItemAlign.Start)

      Text('USB 端口信息')
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .textAlign(TextAlign.Center)
        .fontColor('#ff000000')
        .margin({ bottom: '20px' })

      Button('获取端口信息')
        .onClick(() => {
          this.fetchPortData();
        })
        .margin({ bottom: 20 })


      ForEach(this.ports, (port: usbManager.USBPort) => {
        Column() {
          Row() {
            Text('端口 ID:')
              .fontSize(18)
              .fontWeight(FontWeight.Bold)
              .margin({ right: '15px' })
            Text(`${port.id}`)
              .fontSize(18)
          }
          .margin({ bottom: '10px' })

          Row() {
            Text('支持的模式:')
              .fontSize(18)
              .fontWeight(FontWeight.Bold)
              .margin({ right: '15px' })
            Text(`${this.getModeType(port.supportedModes)}`)
              .fontSize(18)
          }
          .margin({ bottom: '20px' })

          Row() {
            Text('当前模式:')
              .fontSize(18)
              .fontWeight(FontWeight.Bold)
              .margin({ right: '15px' })
            Text(`${this.getCurrentMode(port.status.currentMode)}`)
              .fontSize(18)
          }
          .margin({ bottom: '20px' })

          Row() {
            Text('当前电源角色:')
              .fontSize(18)
              .fontWeight(FontWeight.Bold)
              .margin({ right: '15px' })
            Text(`${this.getCurrentPowerRole(port.status.currentPowerRole)}`)
              .fontSize(18)
          }
          .margin({ bottom: '20px' })

          Row() {
            Text('当前数据角色:')
              .fontSize(18)
              .fontWeight(FontWeight.Bold)
              .margin({ right: '15px' })
            Text(`${this.getCurrentDataRole(port.status.currentDataRole)}`)
              .fontSize(18)
          }
          .margin({ bottom: '20px' })

          Row() {
            Text('当前功能:')
              .fontSize(18)
              .fontWeight(FontWeight.Bold)
              .margin({ right: '15px' })
            Text(`${this.functions}`)
              .fontSize(18)
          }
          .margin({ bottom: '20px' })
          .padding({ top: '10px', bottom: '10px', left: '10px', right: '10px' })
          .backgroundColor('#ffffffff')
          .borderRadius('10px')
          .margin({ bottom: '60px' })
        }
      })

      Column() {

        Row() {
          Text('选择数据角色:')
            .fontSize(18)
            .fontWeight(FontWeight.Normal)
            .margin({ right: '10px' })
          Select([
            { value: '无', icon: "/common/public_icon.svg" },
            { value: '主机', icon: "/common/public_icon.svg" },
            { value: '设备', icon: "/common/public_icon.svg" }
          ])
            .selected(this.dataRoleIndex)
            .value(this.dataRoleText)
            .font({ size: 16, weight: 500 })
            .fontColor('#182431')
            .selectedOptionFont({ size: 16, weight: 400 })
            .optionFont({ size: 16, weight: 400 })
            .space(8)
            .arrowPosition(ArrowPosition.END)
            .menuAlign(MenuAlignType.START, { dx: 0, dy: 0 })
            .onSelect((index: number, text?: string | undefined) => {
              console.info('Select DataRole:' + index)
              this.dataRoleIndex = index;
              if (text) {
                this.dataRoleText = text;
                this.selectedDataRole = index as DataRoleType;
                if (index == DataRoleType.HOST){
                  this.selectedPowerRole = PowerRoleType.SOURCE
                }else if(index == DataRoleType.DEVICE){
                  this.selectedPowerRole = PowerRoleType.SINK
                }else{
                  promptAction.showToast({message:`Cannot select None`})
                }
              }
            })
        }
        .margin({ bottom: '15px' })

        Button('设置端口角色')
          .onClick(() => {
            this.setPortRoleTypes(
              this.ports[0].id,
              this.selectedPowerRole,
              this.selectedDataRole
            );
          })
          .margin({ bottom: '25px' })


        Row() {
          Text('选择设备功能:')
            .fontSize(18)
            .fontWeight(FontWeight.Normal)
            .margin({ right: '10px' })
          Select([
            { value: '无', icon: "/common/public_icon.svg" },
            { value: 'ACM', icon: "/common/public_icon.svg" },
            { value: 'ECM', icon: "/common/public_icon.svg" },
            { value: 'HDC', icon: "/common/public_icon.svg" },
            { value: 'MTP', icon: "/common/public_icon.svg" },
            { value: 'PTP', icon: "/common/public_icon.svg" },
            { value: 'RNDIS', icon: "/common/public_icon.svg" },
            { value: 'MIDI', icon: "/common/public_icon.svg" },
            { value: 'AUDIO_SOURCE', icon: "/common/public_icon.svg" },
            { value: 'NCM', icon: "/common/public_icon.svg" }
          ])
            .selected(0)
            .value(this.functionText)
            .font({ size: 16, weight: 500 })
            .fontColor('#182431')
            .selectedOptionFont({ size: 16, weight: 400 })
            .optionFont({ size: 16, weight: 400 })
            .space(8)
            .arrowPosition(ArrowPosition.END)
            .menuAlign(MenuAlignType.START, { dx: 0, dy: 0 })
            .onSelect((index: number, text?: string | undefined) => {
              console.info('Select Function:' + index)
              if (index === 0){
                this.selectedFunction = 0
              }else{
                this.selectedFunction = 1 << (index - 1);
              }
              if (text) {
                this.functionText = text;
              }
            })
        }
        .margin({ bottom: '15px' })

        Button('设置设备功能')
          .onClick(() => {
            this.setDeviceFunctions(this.selectedFunction);
          })
          .margin({ bottom: '25px' })
      }
    }
    .width('100%')
    .height('100%')
    .padding({ top: '20px', bottom: '20px', left: '10px', right: '10px' })
    .backgroundColor('#ffffffff')
  }
}