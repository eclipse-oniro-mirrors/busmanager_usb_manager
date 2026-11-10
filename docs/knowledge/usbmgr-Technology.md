# 技术术语知识

usb_manager 模块核心技术框架和机制的知识定义，涵盖价值认知、原理、使用方式和代码关联。路径相对仓根。

## SA 4201 服务与 IPC 机制（UsbService）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | USB 设备表、权限记录、功能状态都是全局独占资源；多应用并发访问需要集中裁决；服务必须按需驻留以节省内存 |
| 技术目标 | 单一系统服务承载全部 USB 管理接口，宏裁剪出产品形态，空闲自回收 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `UsbService : SystemAbility(USB_SYSTEM_ABILITY_ID, true) + UsbServerStub`，DelayedSpSingleton 全局单例（usb_service.cpp:87-91），MakeAndRegisterAbility 注册。OnStart（usb_service.cpp:272-351）：CES 就绪重试 → WaitUsbdService（最多 60 秒，usb_service.cpp:252-268）→ 串口条件初始化（SysCap 参数 + WatchParameter usb_serial_disable（usb_service.cpp:288-306））→ port/device/right 域 Init → SubscribeSystemAbility(自身) → AddSystemAbilityListener(MEMORY_MANAGER_SA_ID) → Publish → UnLoadSelf(DELAY)。方法实现模式：权限校验 → 委派域管理器 → 返回 UEC_*。OnStop（usb_service.cpp:445-482）解绑订阅者。自卸载：设备事件后必调 UnLoadSelf（usb_service.cpp:3170），IsNotNeedUnload（活跃设备/gadget 连接，usb_service.cpp:2964-2977）为真则取消并 SetCritical，否则 30 秒定时 UnloadSystemAbility（usb_service.cpp:2981-3028）；usbd 死亡或 IMMEDIATELY 立即卸载（usb_service.cpp:3031-3044）。Dump 支持 usb_host/usb_device/usb_port/-h/-l/-g（usb_service.cpp:2850-2960） |
| 关键流程 | 客户端 `UsbSrvClient::Connect`（interfaces/innerkits/native/src/usb_srv_client.cpp，声明 usb_srv_client.h:157）：GetSystemAbility(4201) → iface_cast\<IUsbServer\> + DeathRecipient；死亡后 ResetProxy 重连 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 68 个 IPC 码（services/zidl/include/usb_service_ipc_interface_code.h:23-90）：权限 4 + 设备 25 + 传输 14 + gadget 8 + port 3 + 串口 11 + 配件 7 + 监听 2（按宏分组见 IUsbServer.idl:29-99）。C++ 客户端统一走 `UsbSrvClient`（usb_srv_client.h:52-136），应用侧为 NAPI 三模块与 CAPI |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbService`（services/native/src/usb_service.cpp）、`UsbSrvClient`、`UsbServerStub`（IDL 生成） |
| 核心成员 | 各域 shared_ptr 管理器 + 六类 mutex + 多个 map 会话表（usb_service.h:356-400） |
| 核心方法 | `OnStart`:272、`OnStop`:445、`UnLoadSelf`:2981、`IsNotNeedUnload`:2964、`CheckSysApiPermission`:2755、`GetCallingInfo`:2736、`Dump`:2903 |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | on-demand 拉起条件 bootevent.wms.fullscreen.ready（services/usb_service.cfg:6）；min_hdi_proxy_version 双代理版本（sa_profile/4201.json:11） |
| 并发约束 | dump 与串口懒建共用 serialManagerMutex_（usb_service.cpp:2915/3461）；UnloadSelf 由 unloadSelfTimerMutex_ 串行（usb_service.cpp:3014） |
| 安全约束 | 所有入口权限校验不可省（CheckSysApiPermission/CheckDevicePermission/IsCallerValid 三档） |

## HDI 双模接入与事件订阅机制（pass-through / HAL）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | HDI USB 接口存在 1.x 整体式（IUsbInterface）与 2.0 按域拆分（Host/Device/Port）两代，产品按形态取其一 |
| 技术目标 | 一套服务代码同时支持两种 HDI 交互层，事件订阅与死亡处理成对完备 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 编译期二选一：`USB_MANAGER_PASS_THROUGH`（usbmgr.gni:25）→ UsbHostManager::InitUsbHostInterface 取 IUsbHostInterface（服务名 usb_host_interface_service）+ IUsbDeviceInterface + IUsbPortInterface 并 BindUsbdHostSubscriber（usb_host_manager.cpp:150-171，device/port 域同构见 usb_device_manager.cpp:76 与 usb_port_manager.cpp:88）；非直通 → UsbService::InitUsbd 统一取 V1_2 IUsbInterface + UsbServiceSubscriber + 死亡监听（usb_service.cpp:402-441）。全部触达 HDI 的方法双分支实现（如 OpenDevice（usb_host_manager.cpp:277-294）） |
| 关键流程 | 事件上行：IUsbdSubscriber::DeviceEvent/PortChangedEvent → 本仓两个订阅者（usb_manager_subscriber.cpp:35/79、usb_service_subscriber.cpp:35/78）→ UsbService::DeviceEvent/UpdateUsbPort。下行解绑：SystemAbilityStatusChangeListener::OnRemoveSystemAbility 在自身 SA 移除时逐域 Unbind（usb_service.cpp:177-204/224-237） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 内部机制。域管理器构造时即取 HDI 代理（UsbHostManager 构造 usb_host_manager.cpp:129-138，非直通取 V1_2）；pass-through 下各域持独立接口指针（usb_host_manager.h:211-215） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbManagerSubscriber`/`UsbServiceSubscriber`、三个域管理器 |
| 核心方法 | `InitUsbHostInterface`（usb_host_manager.cpp:150）、`InitUsbd`（usb_service.cpp:375/402）、`PortChangedEvent`（usb_manager_subscriber.cpp:35）、`Stop`（各域解绑 + MemMgr NotifyProcessStatus） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | pass-through 下无 usbd 整体死亡监听（按域接口各自存在）；非直通下 usbd 死亡即卸载自身（usb_service.cpp:3031-3044） |
| 并发约束 | 订阅者绑定发生在启动单线程；OnRemoveSystemAbility 是唯一的显式解绑点（SA 停止时） |
| 安全约束 | 服务名常量 "usb_host_interface_service"（usb_host_manager.cpp:127）与 sa_profile 版本锁定是部署契约 |

## 主机热插拔与设备事件广播机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 应用需要感知设备插拔与产品名；插入瞬间必须完成描述符级信息采集并执行管控策略后才可见 |
| 技术目标 | 一次 DEVUP 内完成"建对象→策略裁决→广播→通知"的原子流水线 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `UsbService::DeviceEvent`（usb_service.cpp:3147-3183）按 status 分派：gadget 域事件转 UpdateDeviceState；主机域 DEVUP → `AddDevice`（usb_service.cpp:1239：GetDeviceInfo 读设备/配置描述符建 UsbDevice → deviceVidPidMap_ 记 VID-PID-Serial → UsbHostManager::AddDevice）。AddDevice（usb_host_manager.cpp:1253-1302）：置 NEW_ARRIVED → 解锁后 `ExecuteStrategy`（EDM 裁决）→ 非禁用置 ENABLED 并 `PublishCommonEvent`（COMMON_EVENT_USB_DEVICE_ATTACHED，payload 为设备 JSON，usb_host_manager.cpp:1316-1343）；DelDevice（usb_host_manager.cpp:1215-1251）对 ENABLED 设备发 DETACHED。广播附带产品逻辑：Hub 用系统订阅者类型；非音频设备插拔联动反向充电通知（BatterySrvClient 查充电态，usb_host_manager.cpp:1325-1337） |
| 关键流程 | 设备字符串补全 `FillDevStrings`（usb_host_manager.cpp:1452，读 string descriptor 处理 BOM/UTF-16，常量 usb_host_manager.cpp:63-64/96-99）；串口域旁路：DEVUP 时 CheckForTtyUSB（usb_service.cpp:3125-3144，探测 /sys/bus/usb-serial/devices 与 ttyACM* 前缀）懒建 SerialManager |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 对外：GetDevices（列表拷贝，usb_host_manager.cpp:498）、GetDeviceInfoDescriptor/GetRawDescriptor；事件面：COMMON_EVENT_USB_DEVICE_ATTACHED/DETACHED（应用侧 CES 订阅） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbHostManager`、`UsbService` |
| 核心成员 | devices_（shared_mutex）、deviceVidPidMap_（usb_service.h:390） |
| 核心方法 | `DeviceEvent`（usb_service.cpp:3147）、`AddDevice/DelDevice`（usb_service.cpp:1239/1273 + usb_host_manager.cpp:1253/1215）、`PublishCommonEvent`（usb_host_manager.cpp:1316）、`ReportHostPlugSysEvent`（usb_host_manager.cpp:1417）、`FillDevStrings`（usb_host_manager.cpp:1452） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 设备可见性以 ENABLED 为准（EDM 禁用不发广播，usb_host_manager.cpp:1292-1293）；重复 DEVUP 替换旧对象 |
| 并发约束 | AddDevice 明示不持锁执行策略（usb_host_manager.cpp:1282 注释）；广播在 shared_lock 下二次确认设备仍在（usb_host_manager.cpp:1285-1291） |
| 安全约束 | SN_NUM 埋点只取末 5 位（usb_host_manager.cpp:63/:1417）；PLUG_IN_OUT_HOST_MODE 为 CRITICAL BEHAVIOR 事件 |

## 设备访问权限与用户授权弹窗机制（UsbRightManager）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | USB 设备直通应用会带来数据外泄面，需要用户显式授权且授权在设备重插后仍有效、应用重装后失效 |
| 技术目标 | "库内记录 + 弹窗补授 + 生命周期自动清理"三段式，系统应用零打扰 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 判定：HasRight（usb_right_manager.cpp:172）= console/系统应用旁路 + `IsRecordExpired`（时间戳 + install/update 比对，usb_right_db_helper.h:64）。补授：RequestRight（usb_right_manager.cpp:222）→ `GetUserAgreementByDiag`（usb_right_manager.cpp:238，dialogRunning_ 单弹窗互斥）→ `ShowUsbDialog`（usb_right_manager.cpp:254：GetAppName/GetProductName 组参数 → ConnectAbility 连 sceneboard systemdialog（usb_right_manager.cpp:192-219，回退 systemui）→ sem_wait）→ `UsbAbilityConn::OnAbilityConnectDone`（usb_right_manager.cpp:1067）以 cmdCode=1 发送参数 JSON（uiExtensionType=sysDialog/common）→ 用户允许则弹窗进程调 NAPI addDeviceAccessRight → AddDeviceRight（usb_right_manager.cpp:355）落库 → OnAbilityDisconnectDone sem_post（usb_right_manager.cpp:1128）。清理：Init 订阅 8 类公共事件（usb_right_manager.cpp:138-155）；CleanUpRightExpired（启动全量 usb_right_manager.cpp:765）+ 应用卸载（usb_right_manager.cpp:789/820）+ 重装失效（usb_right_manager.cpp:864）+ 用户删除/停止（usb_right_manager.cpp:914/952）+ 设备拔出临时清理（usb_right_manager.cpp:963） |
| 关键流程 | 串口/配件复用同一库与弹窗通道：ShowSerialDialog（usb_right_manager.cpp:534）、ShowUsbDialog(accessory)（usb_right_manager.cpp:495）；deviceName 语义见术语表（VID-PID-Serial） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | HasRight/RequestRight/RemoveRight/AddAccessRight（应用侧 NAPI：hasRight/requestRight/removeRight/addDeviceAccessRight，usb_info.cpp:3190/3191/3228/3227）；系统侧 addDeviceAccessRight 需要 CheckSysApiPermission（usb_service.cpp:2080-2084） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbRightManager`、`UsbRightDbHelper`、`UsbRightDataBase`、`UsbAbilityConn` |
| 核心成员 | waitDialogDisappear_（sem_t）、usbDialogParams_（静态参数表）、databaseMutex_ |
| 核心方法 | `RequestRight`（usb_right_manager.cpp:222/304/339 三重载）、`ShowUsbDialog`（usb_right_manager.cpp:254）、`ConnectAbility`（usb_right_manager.cpp:192）、`OnAbilityConnectDone`（usb_right_manager.cpp:1067）、`AddDeviceRight`（usb_right_manager.cpp:355/403）、`CleanUpRight*`（usb_right_manager.cpp:765-1010）、`HasSetFuncRight`（usb_right_manager.cpp:742，gadget 设置权限） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 弹窗依赖 sceneboard/systemui 就绪（SA 启动 condition 即为此）；USB_RIGHT_TEST 宏为测试旁路（usb_right_manager.cpp:241-243、usb_service.cpp:2820） |
| 并发约束 | dialogRunning_ 全局单弹窗；sem 等待会阻塞调用线程（IPC 线程内等待用户操作） |
| 安全约束 | 该机制是 AGENTS.md 声明的 security-critical 面；改库结构必须迁移方案；installTime/updateTime 防重放语义不可破坏 |

## EDM 管控策略执行机制（ExecuteStrategy）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 企业管理要求按策略禁 USB：全局禁用、按类型禁用、仅信任列表可用；策略须在每次插入时即时生效 |
| 技术目标 | 插入即裁决：策略读取→设备/接口去使能→结果埋点，全程无人工 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 触发点：设备 AddDevice 后与 EDM SA 上线回调（UsbEdmLoadCallback::OnLoadSystemAbilitySuccess，usb_host_manager.cpp:213-218）→ `ExecuteStrategy`（usb_host_manager.cpp:226-275）：IsEdmEnabled（usb_host_manager.cpp:1579）→ `GetUsbPolicy`（usb_host_manager.cpp:1766，含 GetEdmGlobalPolicy usb_host_manager.cpp:1885 / GetEdmStroageTypePolicy usb_host_manager.cpp:1912 / GetEdmPermTypePolicy usb_host_manager.cpp:1830 / GetEdmTrustListPolicy usb_host_manager.cpp:1946，经 ohos.edm.IEnterpriseDeviceMgr 远程调用，策略码 1043/1026/1059/1166/1044 见 usb_host_manager.cpp:107-111）→ 依序执行 ManageGlobalInterfaceImpl（usb_host_manager.cpp:2049）/ ManageUsbSerialDevice（EDM 参数禁串口时）/ ManageUsbType 或 ExecuteManageInterfaceType（usb_host_manager.cpp:1684）/ ExecuteManageDevicePolicy（usb_host_manager.cpp:1657）。设备级使能控制 `UsbDeviceAuthorize`（usb_host_manager.cpp:1586，写 /sys authorize 节点），接口级 `UsbInterfaceAuthorize`/ManageInterface（usb_host_manager.cpp:1638/:1984）。信任列表上限 TRUSTLIST_POLICY_MAX_DEVICES=1000（usb_host_manager.cpp:112） |
| 关键流程 | EDM SA 未起：LoadSystemAbility 异步拉起（usb_host_manager.cpp:1748-1765）+ 超时 EDM_SA_MAX_TIME_OUT=5000（usb_host_manager.cpp:101）；策略超时/失败保留 NEW_ARRIVED 不放行 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 应用面：ManageGlobalInterface/ManageDevice/ManageDevicePolicy/ManageInterfaceType/ManageUsbType（usb_service.cpp:1088-1169，全部 IsCallerValid（root/EDM 3057）+ PreCallFunction 前置）；变化埋点 ReportManageDeviceInfo（usb_host_manager.cpp:2305） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbHostManager`、`UsbEdmLoadCallback` |
| 核心成员 | isPermTypePolicy_（策略类型分档标志，usb_host_manager.h:179） |
| 核心方法 | `ExecuteStrategy`:226、`GetUsbPolicy`:1766、`LoadEdmService`:1748、`UsbDeviceAuthorize`:1586、`ExecuteManageDevicePolicy`:1657、`ManageGlobalInterfaceImpl`:2049、`ManageDeviceTypeImpl`:2164 |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | EDM SA 1601 存在与否决定执行与否；策略执行期间设备处于 NEW_ARRIVED（不可用） |
| 并发约束 | 策略查询与执行不持 devices_ 写锁（usb_host_manager.cpp:1282 注释明示，防止与策略 IPC 互锁） |
| 安全约束 | 只允许 root/EDM UID（usb_service.cpp:84-85/3186-3195）；策略读写面即 DEVICE_SECURITY_POLICY 埋点（usb_security_report.cpp） |

## 主机数据传输机制（同步 bulk/control 与异步 ashmem）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 既有小数据同步收发（读卡器/简单外设），也有大吞吐（文件传输）场景；大块数据走 IPC 拷贝不可接受 |
| 技术目标 | 同步接口简单直接；异步接口以 ashmem 零拷贝 + 回调；传输前权限/独占/参数三重校验 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 同步：BulkTransferRead/Write（usb_service.cpp:1448/1521）与 ControlTransfer/UsbControlTransfer（V1_0/V1_2 参数结构体差异 usb_service.cpp:1556/1613，转换函数 usb_service.cpp:1588/1600）——权限（CheckDevicePermission）→ 独占（CheckExclusiveTransfer）→ 委派 UsbHostManager → 失败必埋 TRANSFER_FAULT。请求队列：RequestQueue/Wait/Cancel（usb_service.cpp:1645-1701）。异步：UsbSubmitTransfer（usb_service.cpp:1775）：fd/memSize 校验（<512MB）→ Ashmem 封装 → USBTransferInfo（endpoint/flags/type/timeout/numIsoPackets，usb_service.cpp:1746）→ HDI；BulkRead/Write（ashmem 版，usb_service.cpp:1860/1897）。回调：应用回调对象直接送 HDI（RegBulkCallback usb_service.cpp:1704）；调用方死亡由 UsbSubmitTransferDeathRecipient Cancel+Close（usb_host_manager.cpp:203-211） |
| 关键流程 | 端点解析：GetEndpointFromId（usb_host_manager.cpp:1180）用于埋点归因；GetTransferTypeString（usb_service.cpp:1759）把 type 1/2/3 映射 Isochronous/Bulk/Interrupt ±Read/Write |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | NAPI：bulkTransfer/controlTransfer/usbControlTransfer/usbSubmitTransfer/usbCancelTransfer（usb_info.cpp:3213-3223）；同步请求式 RequestQueue 族；底层 fd 经 GetFileDescriptor 供高级用法（usb_service.cpp:1384） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbService` + `UsbHostManager`（HDI 直转层）、`UsbTransferCallbackImpl`/`UsbdBulkCallbackImpl`（回调桥接）、`UsbReportSysEvent` |
| 核心方法 | `BulkTransferRead/Write`（usb_service.cpp:1448/1521）、`ControlTransfer`（usb_service.cpp:1556）、`UsbSubmitTransfer`（usb_service.cpp:1775）、`RegBulkCallback`（usb_service.cpp:1704）、`BulkRead/BulkWrite`（usb_service.cpp:1860/1897）、HDI 直转层 usb_host_manager.cpp:751-1165 |
| 核心成员 | MEMSIZE_MAX=512MB（usb_service.cpp:74）、ISO/BULK/INTP type 常量（usb_service.cpp:71-73） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 传输必须设备已打开（OpenDevice 建权）且接口未被他人独占；超时参数勿设 0（无限，AGENTS.md 硬件警告） |
| 并发约束 | transferMutex_（usb_host_manager.h:187）串行化部分传输路径；回调死亡监听独立于 claim 监听 |
| 安全约束 | 每次传输都重验权限（无信任缓存）；故障埋点含接口/端点/VID/PID 字段（TRANSFER_FAULT schema） |

## 设备侧功能状态机与弹窗通知机制（UsbDeviceManager）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | gadget 连接 PC 后功能选择（仅充电/MTP/PTP）需要用户介入；HDC 调试状态需在设置可见；插拔抖动会重复触发 |
| 技术目标 | 功能位图单点管理；连接/断开经延迟任务消抖；UI（弹窗/通知/设置数据）三面同步 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 写入：`SetCurrentFunctions`（usb_device_manager.cpp:201/233 双模）幂等短路 + HDI 设置 + `UpdateFunctions`（usb_device_manager.cpp:358）。事件：`HandleEvent`（usb_device_manager.cpp:416/468）以 UsbTimerWrapper 延迟任务处理连接/断开（断开自动回退 MTP/PTP/DEVMODE_AUTH→STORAGE，usb_device_manager.cpp:441-451；phyConnect 时断开延迟加倍 usb_device_manager.cpp:455-460）；`UserChangeProcess`（usb_device_manager.cpp:521）切户复位。三面同步：ProcessFuncChange（usb_device_manager.cpp:576）→ `UsbFunctionSwitchWindow::PopUpFunctionSwitchWindow`（usb_function_switch_window.cpp:130，连接 com.usb.right 的 UsbFunctionSwitchExtAbility）+ `BroadcastFuncChange`（usb_device_manager.cpp:547 公共事件）+ `UsbConnectionNotifier`（功能通知/HDC 通知，usb_connection_notifier.h:43-46）；HDC 位写设置数据 HDC_STATUS（usb_service.cpp:2607-2639） |
| 关键流程 | 弹窗安装检测 CheckDialogInstallStatus（usb_function_switch_window.cpp:370）与 boot 完成回调（usb_function_switch_window.cpp:396）；反向充电期间 ShouldRejectShowWindow（usb_function_switch_window.cpp:436）抑制 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | GetCurrentFunctions/SetCurrentFunctions/UsbFunctionsFromString/ToString（usb_service.cpp:2201-2293；字符串↔位图 usb_device_manager.cpp:301/338） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbDeviceManager`、`UsbFunctionSwitchWindow`、`UsbConnectionNotifier`、`UsbTimerWrapper`（usb_timer_wraper.cpp） |
| 核心成员 | currentFunctions_/connected_/phyConnect_/delayDisconnTimerId_（usb_device_manager.h:89-99） |
| 核心方法 | `SetCurrentFunctions`:201/233、`HandleEvent`:416/468、`UserChangeProcess`:521、`ProcessFuncChange`:576、`PopUpFunctionSwitchWindow`:130、`SetSettingsDataHdcStatus`（usb_service.cpp:2607） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 仅 functionSettable_ 位可设（HDC/ACM/ECM/MTP/PTP/RNDIS/NCM/STORAGE，usb_device_manager.h:85-87）；弹窗 hap 未装则只通知 |
| 并发约束 | functionMutex_ 串行设置；HandleEvent 全程 deviceMutex_（定时任务内再次加锁） |
| 安全约束 | SetCurrentFunctions 需 CheckSysApiPermission + HasSetFuncRight（EDM 允许，usb_right_manager.cpp:742） |

## 端口角色管理机制（UsbPortManager）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | DRP 平台端口角色决定本机做主机还是设备；角色变化必须同步给系统与应用 |
| 技术目标 | 端口表维护、角色切换入口、角色变化事件化 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 初始化 `Init`/`QueryPort`（usb_port_manager.cpp:60/284）拉取端口建 portMap_/supportedModeMap_；订阅者 PortChangedEvent（usb_manager_subscriber.cpp:35）→ `UpdatePort`（usb_port_manager.cpp:325/348，五元组版本含 supportedModes）→ 发布 COMMON_EVENT_USB_PORT_CHANGED（JSON payload）+ PORT_ROLE_CHANGED 埋点（usb_port_manager.cpp:523）。切换 `SetPortRole`（usb_port_manager.cpp:185）经系统 API 权限（usb_service.cpp:2685-2708）转 HDI，不支持时映射 UEC_SERVICE_NOT_SUPPORT_SWITCH_PORT |
| 关键流程 | 死亡监听 UsbdPortDeathRecipient（usb_port_manager.cpp:121）；IsReverseCharge（usb_port_manager.cpp:167）读充电状态供功能域抑制弹窗（usb_service.cpp:2325-2327） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | GetPorts/GetSupportedModes/SetPortRole（usb_service.cpp:2644-2708）；NAPI getPorts/getPortSupportModes/setPortRoles（usb_info.cpp:3200-3206） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbPortManager`、`UsbPort` |
| 核心成员 | portMap_/supportedModeMap_ + mutex_/setPortRoleMutex_ |
| 核心方法 | `Init`:60、`QueryPort`:284、`SetPortRole`:185、`UpdatePort`:325/348、`ReportPortRoleChangeSysEvent`:523、`IsReverseCharge`:167 |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 角色切换可能断开在连设备（AGENTS.md 硬件警告，真机验证须硬件团队确认） |
| 并发约束 | setPortRoleMutex_ 防并发切换 |
| 安全约束 | 仅系统 API 权限可查询与切换 |

## 串口访问管理机制（SerialManager）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 串口设备（ttyUSB/ttyACM）传统上直接读设备节点，无权限、无审计；应用需要一个受控入口 |
| 技术目标 | 端口级权限（复用授权弹窗）+ HDI 化读写 + 首次操作安全审计 + 死亡自动回收 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 懒初始化：SA OnStart 按 SysCap 参数（usb_service.cpp:288）与 DEVUP 时 CheckForTtyUSB（usb_service.cpp:3125）创建 SerialManager；SerialGetPortList（usb_service.cpp:3458）亦兜底懒建并 UpdateDeviceVidPidMap（usb_service.cpp:3747，portId→(deviceName,VID-PID-Serial)）。会话：SerialOpen（usb_service.cpp:3231）→ 权限（RequestSerialRight usb_service.cpp:3503 走弹窗）→ SerialManager::SerialOpen（serial_manager.cpp:91，portTokenMap_ 记 tokenId + 重复打开拒绝）→ serialClientMap_ 挂死亡监听。读写：上限 64KB/200KB（usb_service.cpp:68-69）；首次读写触发 `ReportSerialOperationSecurityInfo`（serial_manager.cpp:462，security_guard 审计）与 SERIAL_OPERATION 埋点（usb_service.cpp:3790）。回收：SerialDeathRecipient（usb_service.cpp:3047-3054）= FreeTokenId + CancelSerialRight + 关端口 |
| 关键流程 | EDM 参数禁用：WatchParameter persist.edm.usb_serial_disable → ManageUsbSerialDevice（usb_service.cpp:293-302/:1171；判定 IsUsbSerialDisable usb_host_manager.cpp:2260） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | SerialOpen/Close/Read/Write/GetAttribute/SetAttribute/GetPortList + HasSerialRight/RequestSerialRight/AddSerialRight/CancelSerialRight（usb_service.cpp:3231-3724）；NAPI serial 模块（serial_info.cpp:963-975） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `SERIAL::SerialManager`、`UsbService::SerialDeathRecipient` |
| 核心成员 | serialPortMap_/portTokenMap_ + 三锁（serial_manager.h:74-84）、portsHasBeenRead_/Written_ |
| 核心方法 | `SerialOpen`（serial_manager.cpp:91）、`CheckPortAndTokenId`（serial_manager.cpp:76）、`FreeTokenId`（serial_manager.cpp:305）、`ReportSerialOperationSecurityInfo`（serial_manager.cpp:462）、`ValidateUsbSerialManagerAndPort`（usb_service.cpp:3214）、`Dump` 三命令（serial_manager.cpp:324-424） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 依赖 seriald（serial_interface_service）与 SysCap Serial；无串口硬件时 SerialGetPortList 返回空成功 |
| 并发约束 | 端口单会话；读写状态锁分离（readStatusMutex_/writeStatusMutex_） |
| 安全约束 | 首次读写审计 + SERIAL_OPERATION 行为事件（含 bundle/VID/PID/属性）；每次操作失败均 OPERATION_FAULT |

## DFX 埋点与安全审计机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | USB 故障（传输失败、串口异常、功能切换失败）需要端点级现场；企业场景需要受管设备与串口操作的安全留痕 |
| 技术目标 | 统一 USB 域事件 schema + security_guard 审计双通道，敏感值脱敏 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | HiSysEvent 域 USB 9 事件（hisysevent.yaml:16-86）：传输故障经 `UsbReportSysEvent::ReportTransferFaultSysEvent`（services/native/src/usb_report_sys_event.cpp，含 CheckAttribute 变体补端点属性）；插拔/功能/端口行为事件在各自管理器内直写（usb_host_manager.cpp:1417、usb_device_manager.cpp:823/831、usb_port_manager.cpp:523）；通用操作故障 OPERATION_FAULT（usb_service.cpp:3805-3825）；串口行为 SERIAL_OPERATION（usb_service.cpp:3766-3801）。安全审计：串口首次读写 ReportSerialOperationSecurityInfo（serial_manager.cpp:462，事件 id USB_SERIAL_EVENT_ID serial_manager.h:32）；受管设备 DEVICE_SECURITY_POLICY 经 usb_security_report.cpp。HITRACE：SerialRead/Write 打点（usb_service.cpp:3316/3342） |
| 关键流程 | 脱敏：SN 末 5 位（usb_host_manager.cpp:63/:1417）；埋点全部 USB_HILOG 记录且不落敏感明文 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | `UsbReportSysEvent` 静态类（usb_report_sys_event.h:31-37）；新增/改事件必须同步 hisysevent.yaml 并与 DFX 对齐（AGENTS.md 约束） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbReportSysEvent`、`UsbSecurityReport`（usb_security_report.cpp） |
| 核心方法 | `ReportTransferFaultSysEvent`/`CheckAttributeReportTransferFaultSysEvent`（usb_report_sys_event.cpp）、`ReportHostPlugSysEvent`（usb_host_manager.cpp:1417）、`ReportUsbOperationFaultSysEvent`（usb_service.cpp:3805）、`ReportUsbSerialOperationSysEvent`（usb_service.cpp:3766） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 事件 schema（字段名/类型/等级）为公共契约不可擅改；BEHAVIOR/FAULT/SECURITY 三类语义不可混用 |
| 并发约束 | 埋点在业务线程同步写，无额外锁 |
| 安全约束 | 不得新增明文敏感字段（SN 全量、token、认证挑战）；USB_HILOG 体系不可绕过（hilog_wrapper.h:91-100） |
