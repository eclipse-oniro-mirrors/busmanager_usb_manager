# 编码铁律

> 依据本仓现行代码与 AGENTS.md 约束整理，路径相对仓根。规则均给出代码依据。

## 特性开关与条件编译

- **禁止**假设特性默认开启。新增能力若属主机/设备/端口/串口域，**采用**对应宏包裹并在 usbmgr.gni `declare_args` 登记（usbmgr.gni:20-29），同步在 interfaces/innerkits/IUsbServer.idl 方法上加 [macrodef]、在 services/BUILD.gn 传入 defines。AGENTS.md 明言 "Always check feature flags"。
- **禁止**在 pass-through 分支新增 HDI 调用而不写非直通分支（或反之）。每个 HDI 调用点都要 `#ifdef USB_MANAGER_PASS_THROUGH` / `#else` 成对（模式见 usb_host_manager.cpp:317-334）。改动后两种开关组合都必须可编译。
- **禁止**硬编码 HDI 代理版本；`min_hdi_proxy_version`（sa_profile/4201.json:11）与依赖库（libusb_proxy_2.0/libusb_proxy_1.1）只能随开关整体切换（services/BUILD.gn 条件依赖）。

## 接口兼容（最高优先级）

- **禁止**修改 public API 签名而不做兼容评审：interfaces/innerkits/native/include/（inner_kits 发布面，bundle.json:92-104）与 interfaces/kits/js/napi/src/。NAPI 与 C++ API 必须同步更新（AGENTS.md Generated-code boundaries）。
- **禁止**重排或复用 `UsbInterfaceCode` 0-67（services/zidl/include/usb_service_ipc_interface_code.h:23-90）；新方法一律追加在 INVALID 之前且需确认 IDL 同步生成。
- **禁止**手改 IDL 生成物（IUsbServer.idl 生成的 stub/proxy、回调 stub）。
- **禁止**变更 hisysevent.yaml 中 9 个事件的字段名/类型/等级（域 USB，hisysevent.yaml:16-86）——下游 hiview 规则消费该 schema；变更需与 DFX 团队对齐（AGENTS.md DFX 约束）。

## 错误处理

- **禁止**自造返回码，**采用** `UsbErrCode`（utils/native/include/usb_errors.h:37-108）：UEC_OK=0；UEC_INTERFACE_* 与 UEC_SERVICE_* 两段相隔 `USB_OFFSET=100`（usb_errors.h:33-35）；串口/权限各有独立枚举（UEC_SERIAL_*、UsbRightErrCode（usb_errors.h:110-119））。**禁止**在既有枚举值之前插入新值（全部后值漂移）；新增一律加段尾。
- **禁止**混淆三套错误语义：IPC 层 int32 返回（HDF/UEC 直接透传）、业务出参（如 hasRight/actualSize）、NAPI 错误码（usb_napi_errors.h、serial_napi_errors.h）。系统 API 权限拒绝要区分 API 版本分档：`apiVersion < API_VERSION_ID_18` 返回 UEC_SERVICE_PERMISSION_DENIED_SYSAPI，否则 UEC_SERVICE_PERMISSION_DENIED_SYSAPI_FAILED（usb_service.cpp:3104-3121/84）。
- **禁止**吞掉传输失败现场：传输类方法失败必须调用 `UsbReportSysEvent::ReportTransferFaultSysEvent`（usb_service.cpp:1776/1790 等；实现见 services/native/src/usb_report_sys_event.cpp）；串口失败必须 `ReportUsbSerialOperationFaultSysEvent`（usb_service.cpp:3599 等）。
- 传输参数前置校验：fd>0、memSize>0 且 < MEMSIZE_MAX=512MB（usb_service.cpp:78/2101）；串口读 ≤64KB、写 ≤200KB 且 size≤data.size()（usb_service.cpp:72-73/3679/3705-3712）；越界先 `::close(fd)` 再返回 UEC_SERVICE_INVALID_VALUE / UEC_SERIAL_IO_EXCEPTION。
- 用户可见操作失败埋点用 `ReportUsbOperationFaultSysEvent`（FUNCTION_CHANGED/SetPortRole 等路径，usb_service.cpp:2559/3044）。

## 权限与安全

- **禁止**移除或绕过入口校验：系统 API 先 `CheckSysApiPermission`（usb_service.cpp:3104）；设备访问先 `CheckDevicePermission`（usb_service.cpp:2304）→ HasRight（usb_service.cpp:2322：IsSystemAppOrSa 旁路 → GetCallingInfo → UsbRightDbHelper 查库）；Manage* 策略入口另加 `IsCallerValid`（root 或 EDM_UID=3057，usb_service.cpp:88-89/3542-3551）。
- **禁止**在日志与埋点输出敏感值：`busDev` 用 `%{private}s`（usb_right_manager.cpp:225），序列号只取末 5 位（usb_host_manager.cpp:63 `LAST_FIVE`、ReportHostPlugSysEvent（usb_host_manager.cpp:1640））；AGENTS.md 明令 USB_HILOGE 不得记录序列号/用户 token/鉴权质询值。
- **禁止**在权限库记录 console 调用（USB_RIGHT_USERID_CONSOLE 旁路仅限既有逻辑，usb_right_manager.cpp:177-180）；改动 usb_right_manager.cpp / usb_right_db_helper.cpp 必须安全评审（AGENTS.md Do-not-change）。
- **禁止**变更权限库 schema 不带迁移方案：usbRightInfoTable 九列 + tokenId 列的 v2 升级路径见 usb_right_database.h:37-49；路径/数据/请求时间/有效期字段语义是防重放设计（installTime/updateTime/requestTime/validPeriod，usb_right_db_helper.h:37-44）。

## 并发模型

- **禁止**跨域持锁调用 IPC：各域状态仅在本域锁内改写；`UsbHostManager::AddDevice` 演示了正确姿势——unique_lock 修改 devices_ 后主动 unlock 再执行 ExecuteStrategy/广播（usb_host_manager.cpp:1484-1505 注释明示 "DONT hold unique_lock here"）。
- **禁止**并发弹窗：`GetUserAgreementByDiag` 以 dialogRunning_ 互斥保证系统同时只有一个授权弹窗（usb_right_manager.cpp:244-245）；弹窗等待用 `sem_t waitDialogDisappear_`（usb_right_manager.cpp:282/289），OnAbilityDisconnectDone 里 sem_post（usb_right_manager.cpp:1128）。
- devices_ 读多写少用 `std::shared_mutex`（usb_host_manager.h:237）；客户端 proxy_ 同理（usb_srv_client.h:168）。权限 DB 单写者（usb_right_db_helper.h:130）。
- 跨进程会话必须挂 DeathRecipient：串口（usb_service.cpp:3613）、附件（usb_service.cpp:2745）、claim 独占（usb_service.cpp:1031）、连接监听（usb_service.cpp:638）、异步传输（usb_host_manager.cpp:192-204）、SA 客户端死亡重连（usb_srv_client.h:153）。

## 日志规范

- **统一采用** `USB_HILOGF/E/W/I/D(module, ...)`（utils/native/include/hilog_wrapper.h:91-100），module 取 `UsbMgrSubModule` 8 值（hilog_wrapper.h:52-62，域 0xD002A00）；tag 体系见 hilog_wrapper.h:79-88。**禁止**printf/dprintf 于业务逻辑（dump 帮助除外，usb_service.cpp:3288-3309）。

## 命名与文件

- C++ 类/方法 PascalCase；成员 snake_case_ 后缀下划线；文件小写；命名空间 `OHOS::USB`（串口为 `OHOS::SERIAL`，serial_manager.h:30-31 例外）；许可头 Apache 2.0 不得删（AGENTS.md Conventions）。
- 消息/JSON 键与外部约定（cJSON 组装的事件 JSON、弹窗参数键）属于接口契约，改键等于改接口。

## 构建与测试

- **禁止**在 BUILD.gn 引入 bundle.json 未声明的组件依赖（services/BUILD.gn 30+ 依赖需 license 审查，AGENTS.md Third-party constraints）。
- **禁止**未经确认改 MEMSIZE_MAX、串口收发上限、EDM 超时（EDM_SA_MAX_TIME_OUT=5000，usb_host_manager.cpp:101）等行为数值（AGENTS.md Ask before）。
- 单测镜像源目录放 test/native/（unittest/service_unittest/serial/、fuzztest、benchmarktest、js_unittest、ets_unittest，test/native/ 下 8 个目录）；C++ 用 gtest HWTEST_F，ArkTS 用 Hypium（AGENTS.md Testing）。改动权限流必须过 service_unittest，NAPI 改动须同时构建 usb_info/usbmanager_middle/serial_middle 三源文件对应模块。
- 提交前按 AGENTS.md Formatting 约定执行格式化（仓根配置文件，Webkit 基底 120 列）；本仓 Windows 环境无 OpenHarmony 工具链时须明示"构建/测试未执行"。
