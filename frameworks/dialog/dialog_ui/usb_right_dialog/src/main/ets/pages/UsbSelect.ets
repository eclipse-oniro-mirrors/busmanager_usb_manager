/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { usbServiceSwitch } from '../util/UsbServiceSwitch';
import { getUserAuth } from '../util/UserAuth';
import deviceInfo from '@ohos.deviceInfo';
import common from '@ohos.app.ability.common';
import commonEventManager from '@ohos.commonEventManager';
import usbManagerService from '@ohos.usbManager';

let USBSERVICESWITCH_ID_CHARGE_ONLY: number = 0;
let USBSERVICESWITCH_ID_XFER_HDC: number = 4;
let USBSERVICESWITCH_ID_XFER_FILE: number = 8;
let USBSERVICESWITCH_ID_XFER_PIC: number = 16;
let USBSERVICESWITCH_ID_REVERSE_CHARGE: number = 1 << 30;
let DEBOUNCE_TIME: number = 500;
export const USB_DIALOG_OPEN = 'true';
export const USB_DIALOG_CLOSE = 'false';

class Contact {
  public name: ResourceStr = '';
  public des: ResourceStr = '';
  public usbFunction: number;
  public checked: boolean;

  constructor(name: ResourceStr = '', des: ResourceStr = '', usbFunction: number, checked: boolean) {
    this.name = name;
    this.des = des;
    this.usbFunction = usbFunction;
    this.checked = checked;
  }
}

@Entry
@Component
struct SimpleContacts {
  @State mtpTip: ResourceStr = $r('app.string.USB_func_mtp_tip');
  @State mtpChecked: boolean = false;
  @State ptpChecked: boolean = false;
  @State chargeChecked: boolean = false;
  @State reverseChargeChecked: boolean = false;
  @State setFuncFinished: boolean = true;
  @State setPortRoleFinished: boolean = true;
  @State isSetFuncHere: boolean = false;
  @State ignoreDetachEvent: boolean = false;
  private curFunc: number = USBSERVICESWITCH_ID_CHARGE_ONLY;
  @State index: number = USBSERVICESWITCH_ID_CHARGE_ONLY;
  private contacts: Array<Contact> = []
  private timeout: number | undefined = 0;
  private setPortRoleTimeout: number | undefined = 0;
  private usbPortList: Array<usbManagerService.USBPort> = [];

  onPageHide() {
    console.info('IndexComponent onPageHide');
    usbServiceSwitch.setUSBChargeDialog(USB_DIALOG_OPEN);
    this.isSetFuncHere = false;
    if (!this.setFuncFinished) {
        usbServiceSwitch.serviceChoose(this.index);
    }
    if (!this.setPortRoleFinished) {
        if (this.usbPortList?.length > 0 &&
              this.usbPortList[0].status.currentPowerRole !== usbManagerService.PortModeType.UFP) {
            usbServiceSwitch.setPortRole(this.usbPortList[0].id, usbManagerService.PortModeType.UFP,
                usbManagerService.PortModeType.UFP);
        }
    }
    let context = getContext(this) as common.ServiceExtensionContext;
    context?.terminateSelf();
  }

  async aboutToAppear() {
    usbServiceSwitch.setUSBChargeDialog(USB_DIALOG_CLOSE);
    if (deviceInfo.deviceType === 'tablet') {
      this.mtpTip = $r('app.string.USB_func_mtp_tip2', $r('app.string.USB_my_tablet'));
    }
    this.initSelectedOption();

    console.info('globalThis.want');
    let usbSupportedFunctions = USBSERVICESWITCH_ID_XFER_FILE | USBSERVICESWITCH_ID_XFER_PIC;
    if (globalThis.want?.parameters?.usbSupportedFunctions !== undefined) {
      usbSupportedFunctions = globalThis.want?.parameters?.usbSupportedFunctions;
    }
    if (usbSupportedFunctions & USBSERVICESWITCH_ID_XFER_FILE) {
      this.contacts.push(new Contact($r('app.string.USB_func_mtp'), this.mtpTip, USBSERVICESWITCH_ID_XFER_FILE,
        this.mtpChecked));
    }
    if (usbSupportedFunctions & USBSERVICESWITCH_ID_XFER_PIC) {
      this.contacts.push(new Contact($r('app.string.USB_func_ptp'), $r('app.string.USB_func_ptp_tip'),
        USBSERVICESWITCH_ID_XFER_PIC, this.ptpChecked));
    }
    if (usbSupportedFunctions & USBSERVICESWITCH_ID_REVERSE_CHARGE) {
      this.contacts.push(new Contact($r('app.string.USB_func_reverse_charge'), $r('app.string.USB_charge_connected_device'),
        USBSERVICESWITCH_ID_REVERSE_CHARGE, this.reverseChargeChecked));
    }
    this.contacts.push(new Contact($r('app.string.USB_func_charge'), '',
      USBSERVICESWITCH_ID_CHARGE_ONLY, this.chargeChecked));

    const eventsToSubscribe = [
      commonEventManager.Support.COMMON_EVENT_USB_STATE,
      commonEventManager.Support.COMMON_EVENT_USB_DEVICE_ATTACHED,
      commonEventManager.Support.COMMON_EVENT_USB_DEVICE_DETACHED
    ];

    commonEventManager.createSubscriber({ events: eventsToSubscribe }, (err, subscriber) => {
      if (err) {
        console.error('createSubscriber failed');
        return;
      }

      commonEventManager.subscribe(subscriber, (err, data) => {
        if (err) {
          console.error('subscribe failed');
          return;
        }

        console.info('common event received');
        this.usbPortList = usbServiceSwitch.getPorts()
        if (data.event === commonEventManager.Support.COMMON_EVENT_USB_STATE) {
          if (data.parameters?.connected === false && !this.ignoreDetachEvent) {
            if (this.isSetFuncHere) {
              console.info('return because isSetFuncHere is ' + this.isSetFuncHere);
              this.isSetFuncHere = false;
              return;
            }
            console.info('connected false and terminateSelf');
            usbServiceSwitch.setUSBChargeDialog(USB_DIALOG_OPEN);
            let context = getContext(this) as common.ServiceExtensionContext;
            context?.terminateSelf();
          } else {
            if (data.parameters?.mtp !== undefined && data.parameters?.mtp === true) {
              this.index = USBSERVICESWITCH_ID_XFER_FILE;
            } else if (data.parameters?.ptp !== undefined && data.parameters?.ptp === true) {
              this.index = USBSERVICESWITCH_ID_XFER_PIC;
            } else if (this.usbPortList?.length > 0 &&
                         this.usbPortList[0].status.currentPowerRole == usbManagerService.PortModeType.UFP) {
              this.index = USBSERVICESWITCH_ID_REVERSE_CHARGE;
            } else {
              this.index = USBSERVICESWITCH_ID_CHARGE_ONLY;
            }
          }
          return;
        }
        if (data.event === commonEventManager.Support.COMMON_EVENT_USB_DEVICE_DETACHED) {
          if (this.ignoreDetachEvent) {
            return;
          }
          console.info('connected false and terminateSelf');
          usbServiceSwitch.setUSBChargeDialog(USB_DIALOG_OPEN);
          let context = getContext(this) as common.ServiceExtensionContext;
          context?.terminateSelf();
          return;
        }
        if (data.event === commonEventManager.Support.COMMON_EVENT_USB_DEVICE_ATTACHED) {
          if (this.usbPortList?.length > 0 &&
                this.usbPortList[0].status.currentPowerRole == usbManagerService.PortModeType.UFP) {
            this.index = USBSERVICESWITCH_ID_REVERSE_CHARGE;
          } else {
            this.index = USBSERVICESWITCH_ID_CHARGE_ONLY;
          }
        }
      })
    });
  }

  private initSelectedOption() {
    this.usbPortList = usbServiceSwitch.getPorts()
    this.curFunc = usbServiceSwitch.getCurrentFunctions();
    if (this.usbPortList?.length > 0 &&
          this.usbPortList[0].status.currentPowerRole == usbManagerService.PortModeType.UFP) {
      this.reverseChargeChecked = true;
      this.index = USBSERVICESWITCH_ID_REVERSE_CHARGE;
    } else if (this.curFunc & USBSERVICESWITCH_ID_XFER_FILE) {
      this.mtpChecked = true;
      this.index = USBSERVICESWITCH_ID_XFER_FILE;
    } else if (this.curFunc & USBSERVICESWITCH_ID_XFER_PIC) {
      this.ptpChecked = true;
      this.index = USBSERVICESWITCH_ID_XFER_PIC;
    } else {
      this.chargeChecked = true;
      this.index = USBSERVICESWITCH_ID_CHARGE_ONLY;
    }
  }

  @Builder
  itemHead(text: ResourceStr = '') {
    Text(text)
      .fontSize(19)
      .width('100%')
      .padding(16)
      .fontWeight(FontWeight.Bolder)
      .margin({
        top: 0,
        left: 0,
        bottom: 15,
        right: 0
      })
  }

  @Builder
  itemFoot(num: number) {
  }

  @Builder
  overBuilder(index: number) {

  }

  build() {
    List({ space: 10 }) {
      ListItemGroup({ header: this.itemHead($r('app.string.USB_hint')), footer: this.itemFoot(3), space: 12 }) {
        ForEach(this.contacts, (item: Contact, index: number) => {
          ListItem() {
            Button({ type: ButtonType.Normal, stateEffect: true }) {
              Row() {
                Row() {
                  Column() {
                    Text(item.name)
                      .fontSize(16)
                      .width('100%')
                      .padding({
                        top: 1,
                        left: 0,
                        bottom: 1,
                        right: 0
                      })
                      .margin(0)
                      .fontWeight(FontWeight.Medium)
                    if (item.des != '') {
                      Text(item.des)
                        .fontSize(14)
                        .width('100%')
                        .margin(0)
                        .padding({
                          top: 1,
                          left: 0,
                          bottom: 1,
                          right: 0
                        })
                        .fontColor('#626262')
                    }
                  }
                  .width('92.5%')
                  .justifyContent(FlexAlign.Start)
                  .padding({
                    top: 5,
                    left: 0,
                    bottom: 5,
                    right: 0
                  })

                  Column() {
                    if (this.index === item.usbFunction) {
                      Radio({ value: 'item.name', group: 'radioGroup', indicatorType: RadioIndicatorType.TICK })
                        .radioStyle({
                          checkedBackgroundColor: Color.Blue
                        })
                        .height(20)
                        .width(20)
                        .margin(0)
                        .padding(0)
                        .checked(this.index === item.usbFunction)
                        .alignSelf(ItemAlign.End)
                    } else {
                      Column()
                        .height(20)
                        .width(20)
                        .alignSelf(ItemAlign.End)
                    }
                  }
                  .justifyContent(FlexAlign.End)
                  .width('7.5%')
                }
                .width('100%')
              }
              .padding(12)
              .width('100%')
            }
            .borderRadius(20)
            .backgroundColor($r('sys.color.comp_background_list_card'))
            .width('100%')
            .onClick((e) => {
              this.index = item.usbFunction;
              console.log('choose=' + item.usbFunction);
              getUserAuth(item.name, 
                () => {
                  if (item.usbFunction == USBSERVICESWITCH_ID_REVERSE_CHARGE) {
                    this.ignoreDetachEvent = true;
                    this.setPortRoleToHost();
                    return;
                  }
                  this.setUsbFunction(item.usbFunction);
                  this.index = item.usbFunction;
                },
                () => {
                  this.initSelectedOption();
              });
            })
          }
          .margin({
            top: 0,
            left: 16,
            bottom: 0,
            right: 16
          })
        }, (item: Contact) => JSON.stringify(item))
      }
    }
    .height('100%')
    .width('100%')
    .backgroundColor($r('sys.color.comp_background_gray'))
    .alignListItem(ListItemAlign.Center)
    .expandSafeArea([SafeAreaType.SYSTEM], [SafeAreaEdge.TOP, SafeAreaEdge.BOTTOM])
  }

  setUsbFunction(choose: number) {
    console.log('choose is ' + choose)
    this.setPortRoleToDevice();
    if (this.timeout) {
      clearTimeout(this.timeout);
    }
    this.timeout = setTimeout(() => {
      console.log('serviceChoose start')
      this.isSetFuncHere = true;
      usbServiceSwitch.serviceChoose(choose);
      console.log('serviceChoose end')
      this.setFuncFinished = true;
    }, DEBOUNCE_TIME)
    this.setFuncFinished = false;
  }

  setPortRoleToHost() {
    console.log('setPortRole')
    if (this.setPortRoleTimeout) {
      clearTimeout(this.setPortRoleTimeout);
    }
    this.setPortRoleTimeout = setTimeout(() => {
      console.log('setPortRole start')
      this.usbPortList = usbServiceSwitch.getPorts();
      if (this.usbPortList?.length > 0 &&
            this.usbPortList[0].status.currentDataRole == usbManagerService.PortModeType.DFP) {
        usbServiceSwitch.setPortRole(this.usbPortList[0].id, usbManagerService.PortModeType.UFP,
            usbManagerService.PortModeType.UFP);
      }
      console.log('setPortRoleToHost end')
      this.setPortRoleFinished = true;
    }, DEBOUNCE_TIME)
    this.setPortRoleFinished = false;
  }

  setPortRoleToDevice() {
    console.log('setPortRoleToDevice')
    if (this.setPortRoleTimeout) {
      clearTimeout(this.setPortRoleTimeout);
    }
    this.setPortRoleTimeout = setTimeout(() => {
      console.log('setPortRole start')
      this.usbPortList = usbServiceSwitch.getPorts();
      if (this.usbPortList?.length > 0 &&
            this.usbPortList[0].status.currentDataRole == usbManagerService.PortModeType.UFP) {
        usbServiceSwitch.setPortRole(this.usbPortList[0].id, usbManagerService.PortModeType.DFP,
            usbManagerService.PortModeType.DFP);
      }
      console.log('setPortRoleToDevice end')
    }, DEBOUNCE_TIME)
  }
}
