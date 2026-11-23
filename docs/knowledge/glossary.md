# 术语表

> 路径相对仓根；数值与枚举位均经代码核实。

## USB 管理器（usb_manager / USB Manager）
USB 子系统的系统服务组件（@ohos/usb_manager，bundle.json:12），面向应用提供 USB 主机/设备/端口/串口/配件统一管理与权限管控。区别于 external_device_manager（外设驱动框架、厂商 DDK）：本组件是"系统侧 USB 管理与授权面"，DDK 应用不经本组件访问设备。

## 系统服务 4201（SA 4201 / usb_service）
本组件 SA 编号（USB_SYSTEM_ABILITY_ID）与进程名。init 按 `bootevent.wms.fullscreen.ready=true` 条件按需拉起（services/usb_service.cfg:5-6），UID usb、APL system_basic、SELinux u:r:usb_service:s0；产物 libusbservice.z.so（sa_profile/4201.json:5-6）。服务类 `UsbService`（services/native/src/usb_service.cpp:99）。

## 主机模式 / 设备模式 / 端口（Host / Device(gadget) / Port）
三个正交业务域，各由一个管理器承担：UsbHostManager（管理外部 USB 设备）、UsbDeviceManager（本机作为 USB 设备的功能位图）、UsbPortManager（物理端口的电源/数据角色）。分别受 USB_MANAGER_FEATURE_HOST/DEVICE/PORT 编译开关控制（usbmgr.gni:22-24）。

## 直通模式（pass-through / USB_MANAGER_PASS_THROUGH）
usbmgr.gni:25 构建开关（默认 true）。true：服务按域分别使用 HDI 2.0 接口——IUsbHostInterface（服务名 "usb_host_interface_service"，usb_host_manager.cpp:126）、IUsbDeviceInterface、IUsbPortInterface；false：使用统一 V1_2 IUsbInterface（HAL 模式）。订阅者也随之成对切换（UsbManagerSubscriber vs UsbServiceSubscriber）。AGENTS.md "HDI/pass-through/HAL" 路由词条即指此。

## deviceName（两种格式，易混）
对外 IPC 键（HasRight/RequestRight/OpenDevice 系列）：`"busNum-devAddr"`（如 "1-2"）。权限库内键（deviceVidPidMap_ 的 value）：`"VID-PID-Serial"`（usb_service.cpp:1321-1327 组装）。`UsbService::GetDeviceVidPidSerialNumber`（usb_service.cpp:2481）做前者到后者的转换；串口用 serialVidPidMap_ 按 portId → (deviceName, VID-PID-Serial) 双值（usb_service.cpp:4109-4124）。授权必须以 VID-PID-Serial 为准（跨插拔稳定）。

## USB 设备对象（UsbDevice）
主机侧在位设备的完整模型：设备描述符字段 + 配置/接口/端点树 + 字符串（厂商/产品/序列号）+ JSON 序列化（getJsonString，用于公共事件 payload）。定义于 interfaces/innerkits/native/include/usb_device.h；服务侧实例表在 `UsbHostManager::devices_`（usb_host_manager.h:230，map\<name, UsbDevice*\>）。带授权状态机 NEW_ARRIVED→ENABLED/DISABLED（NEW_ARRIVED 置位 usb_host_manager.cpp:1497，ENABLED 判定与置位 usb_host_manager.cpp:1517-1519，DISABLED 由 ExecuteStrategy 决定）。

## 设备访问权限（right / usbRight.db）
应用对某设备（VID-PID-Serial）的使用授权。持久化于 /data/service/el1/public/usb_service/right/ 下 usbRight.db 的 usbRightInfoTable（usb_right_database.h:33-48）：uid/installTime/updateTime/requestTime/validPeriod/deviceName/bundleName/tokenId 九列。有效期 validPeriod 上限 USB_RIGHT_VALID_PERIOD_SET（usb_right_db_helper.h:35），一般授权近似永久；installTime/updateTime 用于识别应用重装/更新后使旧记录失效（CleanUpRightAppReinstalled，usb_right_manager.cpp:864）。系统应用/SA 与 console 调用旁路（usb_right_manager.cpp:177/653）。

## 授权弹窗（sysDialog/common）
设备/串口/配件未授权使用时的用户确认对话框。服务侧经 `UsbRightManager::ConnectAbility` 连接 com.ohos.sceneboard 的 systemdialog（回退 com.ohos.systemui.dialog，usb_right_manager.cpp:206-216），以 cmdCode=1 发送参数 JSON（bundleName/abilityName("com.usb.right"/"UsbServiceExtAbility")/parameters + `ability.want.params.uiExtensionType = "sysDialog/common"`，usb_right_manager.cpp:1079-1095）；UI 实体在 frameworks/dialog/dialog_ui/usb_right_dialog。同时只允许一个弹窗（dialogRunning_），等待用信号量 waitDialogDisappear_。

## 接口独占声明（ClaimInterfaceExclusive）
主机接口的两种占用形态：普通 ClaimInterface（可共存）与独占 ClaimInterfaceExclusive（仅一个 tokenId 持有，其余应用传输被拒）。状态机在 `UsbService::claimStateMap_`（usb_service.h:359-375，ClaimInterfaceState{exclusiveOwner, normalClaimed, notifyCallback, claimRecipient}）；键 "busNum-devAddr-interfaceId"（MakeClaimKey，usb_service.cpp:934）。独占者可注册 onConflict 回调（NotifyExclusiveOwner，usb_service.cpp:914-932）。

## 设备功能位图（gadget functions / FUNCTION_*）
本机作为 USB 设备的功能开关集合（usb_srv_support.h:52-63）：ACM=1、ECM=1<<1、HDC=1<<2、MTP=1<<3、PTP=1<<4、RNDIS=1<<5、NCM=1<<8、STORAGE=1<<9、DEVMODE_AUTH=1<<12。可设集合由 `functionSettable_` 白名单限定（usb_device_manager.h:85-87）；字符串↔位图转换 ConvertFromString/ConvertToString（usb_device_manager.cpp:301/338）。当前值缓存 currentFunctions_（默认 FUNCTION_HDC，usb_device_manager.h:89）。

## 功能选择弹窗（UsbFunctionSwitchWindow）
设备连接 PC 后的"仅充电/传输文件/传输照片"选择框。单例（usb_function_switch_window.h:53），连接 com.usb.right 的 UsbFunctionSwitchExtAbility（usb_function_switch_window.h:89-90）；与授权弹窗同 hap。开关 `usb_manager_feature_pop_up_func_switch_model`（usbmgr.gni:21）控制其行为。

## 端口角色（power role / data role）
USB-C 端口的电源角色（POWER_ROLE_SOURCE/SINK）与数据角色（DATA_ROLE_HOST/DEVICE）（usb_srv_support.h:34-50）。`UsbPortManager::SetPortRole`（usb_port_manager.cpp:185）切换；角色变化由订阅者 PortChangedEvent 发布 COMMON_EVENT_USB_PORT_CHANGED（携带 JSON）。**易混**：mode（PORT_MODE_DEVICE/HOST）与角色是两个维度；supportedModes 表示端口硬件能力。反向充电判定 IsReverseCharge（usb_port_manager.cpp:167）用于抑制弹窗。

## 串口（serial / portId / seriald）
USB 串口（CDC-ACM/tty）访问域：端口发现（SerialGetPortList）、打开/关闭、同步读写、属性（波特率/数据位/停止位/校验）。底层经 HDI `ISerialInterface`（服务名 "serial_interface_service"，usb_service.cpp:3564）。portId 是串口端口标识（区别于设备 busNum-devAddr）；权限键复用 VID-PID-Serial（serialVidPidMap_）。每端口一次读写触发安全审计上报（ReportSerialOperationSecurityInfo，serial_manager.cpp:462）。

## USB 配件（accessory / AOA）
Android Open Accessory 模式设备：主机侧以配件身份通信。`UsbAccessoryManager`（services/native/src/usb_accessory_manager.cpp）管理配件列表/启动停止（ProcessAccessoryStart/Stop（usb_accessory_manager.cpp:221/264））与 fd 打开；授权键 SerialValueHash 哈希后的序列号（usb_accessory_manager.cpp:416）。

## EDM 策略（ExecuteStrategy）
企业设备管理对 USB 的管控执行框架：设备插入时 `UsbHostManager::ExecuteStrategy`（usb_host_manager.cpp:239）向 EDM SA 1601 读取策略——全局禁用(1043)、禁用设备类型(1026/1059/1166)、信任列表(1044)（usb_host_manager.cpp:107-111）——并对新设备去使能（UsbDeviceAuthorize，usb_host_manager.cpp:1869）。设备状态 DISABLED 时不广播、不可见。

## 错误码体系（UEC_* / UsbRightErrCode / NAPI 错误码）
服务与 inner_kit 返回 `UsbErrCode`（utils/native/include/usb_errors.h:37-108）：UEC_INTERFACE_* 与 UEC_SERVICE_* 两段相差 USB_OFFSET=100；串口专用 UEC_SERIAL_*；权限子系统另有负值 UsbRightErrCode（usb_errors.h:110-119）。NAPI 层错误码（usb_napi_errors.h、serial_napi_errors.h）独立映射。三套不可互换。

## 观测事件域（HiSysEvent domain USB）
9 个事件（hisysevent.yaml:16-86）：PLUG_IN_OUT_HOST_MODE/PLUG_IN_OUT_DEVICE_MODE（插拔行为）、FUNCTION_CHANGED、PORT_ROLE_CHANGED、TRANSFER_FAULT（传输故障，含端点级字段）、SERIAL_OPERATION（串口操作行为）、OPERATION_FAULT（通用操作故障）、DEVICE_SECURITY_POLICY（受管设备安全审计，SECURITY 级）。SN 只报末 5 位。

## 设备连接监听（IUsbConnectionCallback）
系统应用（如设置）注册的"应用-设备连接关系"监听（usb_service.cpp:615-698 注册/注销）。服务维护 deviceConnectionMap_（key "busNum-devAddr" → 设备 + apps{uid→bundleName}，usb_service.h:400），OpenDevice/Close/拔出时增删并回调 OnDeviceConnected/Disconnected；新注册监听者会收到存量连接回放（usb_service.cpp:653-670）。

## 按需服务与自卸载（on-demand / UnLoadSelf）
SA 4201 按需拉起；每次设备事件后调用 `UnLoadSelf(UNLOAD_SA_DELAY)`（usb_service.cpp:3519）：若 hasActiveDevice 或 gadget 连接（IsNotNeedUnload，usb_service.cpp:3313-3326）则取消卸载并 SetCritical；否则 30 秒后 UnloadSystemAbility（usb_service.cpp:3330-3377）。UNLOAD_SA_IMMEDIATELY 用于立即回收（如 usbd 死亡，usb_service.cpp:3388）。

## HDI 服务（usbd / seriald）
usbd：drivers_interface_usb 提供的总线服务，本服务全部设备操作/传输的执行者；死亡即触发本 SA 卸载重建（UsbdDeathRecipient，usb_service.cpp:3380）。seriald：串口 HDI 服务（ISerialInterface）。两者代理版本由 sa_profile/4201.json:11 锁定。
