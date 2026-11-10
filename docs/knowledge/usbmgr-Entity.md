# 业务实体知识

usb_manager 模块核心业务实体的完整知识定义，涵盖静态概念、动态运行、规则约束与代码映射。路径相对仓根。

## USB 设备（UsbDevice）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | USB 设备（UsbDevice） |
| 实体定义 | 一个插在主机侧的 USB 外设的完整画像：描述符级身份与能力（配置/接口/端点树）+ 人类可读字符串 + 授权状态。它是主机域一切操作（打开/传输/声明）的目标对象 |
| 核心特征 | 字段组见 interfaces/innerkits/native/include/usb_device.h（busNum/devAddr/厂商/产品/序列号/配置列表，getJsonString 序列化）；服务侧两份索引：`UsbHostManager::devices_`（"busNum-devAddr"→UsbDevice*，usb_host_manager.h:177）与 `UsbService::deviceVidPidMap_`（同键→"VID-PID-Serial"，usb_service.h:390）；授权状态 NEW_ARRIVED/ENABLED/DISABLED（usb_host_manager.cpp:1275/1292-1295 置位） |
| 类型/分类 | 设备级 class（bDeviceClass），Hub(0x09) 与音频(0x01) 有专门处理（usb_host_manager.cpp:120-121/1304-1314） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 创建：DEVUP 事件 → `UsbService::AddDevice`（usb_service.cpp:1239）读描述符建对象并写 deviceVidPidMap_（usb_service.cpp:1258-1264）→ `UsbHostManager::AddDevice` 入 devices_（usb_host_manager.cpp:1253）。查询：GetDevices 拷贝输出（usb_host_manager.cpp:498）；GetDeviceInfo/GetTargetDevice 就地查找。移除：DEVDOWN → `UsbService::DelDevice`（usb_service.cpp:1273，清连接/claim/权限临时记录/deviceVidPidMap_）→ `UsbHostManager::DelDevice`（usb_host_manager.cpp:1215，ENABLED 才发 DETACHED 广播） |
| 状态流转 | 插入：NEW_ARRIVED → ExecuteStrategy 依据 EDM 策略 → DISABLED（不发广播，usb_host_manager.cpp:1292-1293）或 ENABLED（发 COMMON_EVENT_USB_DEVICE_ATTACHED，usb_host_manager.cpp:1296）。拔出：直接销毁；若 ENABLED 先发 COMMON_EVENT_USB_DEVICE_DETACHED（usb_host_manager.cpp:1231-1237）。重复 DEVUP 替换旧对象（usb_host_manager.cpp:1265-1273） |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 名称键即 "busNum-devAddr"（设备级 GetDeviceKey，usb_service.cpp:689-692）；重复插入去重并释放旧对象；Enable 状态决定可见性与广播；EDM 禁用设备跳过一切对外呈现 |
| 并发约束 | devices_ 由 devicesMutex_（shared_mutex，usb_host_manager.h:181）保护：写 unique_lock（usb_host_manager.cpp:1263）、读 shared_lock（usb_host_manager.cpp:1285/1353）；deviceVidPidMap_ 由 UsbService::mutex_ 保护（usb_service.cpp:1262-1265/2135） |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | UsbHostManager.devices_ 持有裸指针（析构统一 delete，usb_host_manager.cpp:140-147） |
| 下游影响 | 权限记录以其 VID-PID-Serial 为键；连接记录/claim 状态以其 busNum-devAddr 为键；插拔公共事件 payload 为其 JSON |
| 平级关联 | UsbEndpoint/UsbInterface/USBConfig 组成其配置树（interfaces/innerkits/native/include/usb_config.h、usb_interface.h）；描述符解析由 usb_descriptor_parser.cpp 与 `FillDevStrings`（usb_host_manager.cpp:1452）补全字符串 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbDevice`（interfaces/innerkits/native/include/usb_device.h）、`UsbHostManager`（services/native/src/usb_host_manager.cpp） |
| 核心方法 | `UsbService::AddDevice/DelDevice`（usb_service.cpp:1239/1273）、`UsbHostManager::AddDevice/DelDevice`（usb_host_manager.cpp:1253/1215）、`GetDeviceInfo`（usb_host_manager.cpp:549）、`FillDevStrings`（usb_host_manager.cpp:1452） |

## 设备访问权限记录（UsbRightAppInfo / usbRight.db）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 设备访问权限记录（UsbRightAppInfo） |
| 实体定义 | 一个 (用户 uid, 应用 bundleName+tokenId, 设备 VID-PID-Serial) 三元组上的授权事实，含时间戳与有效期，是 HasRight 判定的唯一依据 |
| 核心特征 | 结构体 UsbRightAppInfo{primaryKeyId, uid, installTime, updateTime, requestTime, validPeriod}（usb_right_db_helper.h:37-44）；持久化表 usbRightInfoTable（usb_right_database.h:39-48），库 /data/service/el1/public/usb_service/usbRight.db（usb_right_database.h:33） |
| 类型/分类 | 记录不分临时/永久字段，靠 validPeriod 数值区分（弹窗拒绝的临时授权不落库；CleanUpRightTemporaryExpired 按设备清理，usb_right_manager.cpp:963） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 产生：用户在授权弹窗点允许 → 弹窗侧 NAPI `addDeviceAccessRight`（usb_info.cpp:3227）→ IPC USB_FUN_ADD_RIGHT → `UsbService::AddAccessRight`（usb_service.cpp:2074，CheckSysApiPermission 前置）→ `UsbRightManager::AddDeviceRight`（usb_right_manager.cpp:355）→ `AddOrUpdateRightRecord` 落库。消费：每次设备操作 `HasRight` → `UsbRightDbHelper::IsRecordExpired`（usb_right_db_helper.h:64）判定。消亡：应用卸载（CleanUpRightAppUninstalled（usb_right_manager.cpp:789/820））、用户删除（CleanUpRightUserDeleted（usb_right_manager.cpp:914））、设备拔出临时清理（usb_right_manager.cpp:963）、重装失效（CleanUpRightAppReinstalled（usb_right_manager.cpp:864）） |
| 状态流转 | 无独立状态机；记录有效性由"当前时间 vs requestTime+validPeriod"及 installTime/updateTime 与应用实际安装/更新时间比对决定（重装即失效）。DB 版本 v2（tokenId 列经 OnUpgrade 补列，usb_right_database.h:37/49） |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | console（USB_RIGHT_USERID_CONSOLE）与系统应用/SA 旁路不入库（usb_right_manager.cpp:177-180/653）；有效写入前取应用真实 install/updateTime 防篡改（GetBundleInstallAndUpdateTime（usb_right_manager.cpp:697））；改表结构必须带迁移（AGENTS.md Do-not-change） |
| 并发约束 | 全部 DB 操作经 `UsbRightDbHelper::databaseMutex_`（usb_right_db_helper.h:130）串行 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | UsbRightManager（域单写者）；键依赖 UsbDevice 的 VID-PID-Serial |
| 下游影响 | 决定 OpenDevice/传输/串口/配件全链放行或 UEC_SERVICE_PERMISSION_DENIED |
| 平级关联 | 公共事件订阅（RightSubscriber，usb_right_manager.cpp:138-155）驱动记录清理；弹窗会话产出本记录 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbRightManager`、`UsbRightDbHelper`（usb_right_db_helper.cpp）、`UsbRightDataBase`（usb_right_database.cpp） |
| 核心方法 | `HasRight`（usb_right_manager.cpp:172）、`AddDeviceRight`（usb_right_manager.cpp:355/403）、`RemoveDeviceRight/RemoveDeviceAllRight`（usb_right_manager.cpp:438/480）、`IsRecordExpired/AddOrUpdateRightRecord/DeleteRightRecord`（usb_right_db_helper.h:64-95）、`CleanUpRightExpired`（usb_right_manager.cpp:765） |

## 接口独占状态（ClaimInterfaceState）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 接口声明状态（ClaimInterfaceState） |
| 实体定义 | 一个设备接口（"busNum-devAddr-interfaceId"）上的占用关系：谁独占（exclusiveOwner tokenId）、是否有普通声明（normalClaimed）、冲突通知回调与死亡监听。实现"一个应用独占接口、他人传输被拒"的语义 |
| 核心特征 | 结构体（usb_service.h:332-337）；容器 `claimStateMap_`（usb_service.h:348，map\<string, ClaimInterfaceState\>）；键 MakeClaimKey（usb_service.cpp:871-874） |
| 类型/分类 | 两形态：normalClaimed=true（普通声明，可多个应用）与 exclusiveOwner≠0（独占） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 建立：ClaimInterface → HDI 成功后 `NotifyExclusiveOwner` 置 normalClaimed 并向既有独占者回调 onConflict（usb_service.cpp:846-869）；ClaimInterfaceExclusive（usb_service.cpp:929）→ 校验无人独占/无普通声明（UEC_INTERFACE_BUSY 拒绝）→ HDI claim → `RegisterExclusiveCallback`（usb_service.cpp:962）登记 owner+回调+ClaimDeathRecipient。校验：每次传输前 `CheckExclusiveTransfer`（usb_service.cpp:876-890）非 owner 返回 UEC_SERVICE_IO_EXCEPTION。释放：ReleaseInterface → `ReleaseClaimState`（usb_service.cpp:1001-1041）——owner 释放且仍有普通声明时跳过 HDI 释放；普通声明者释放只清标志。死亡：ClaimDeathRecipient（usb_service.cpp:3086-3114）清 owner，无普通声明则补 HDI ReleaseInterface。拔出/Close：RemoveAllClaimByDevice/RemoveExclusiveClaimByDevice（usb_service.cpp:892-927） |
| 状态流转 | 空 → 普通声明（normalClaimed=true）→ 独占叠加（owner 置位）→ owner 释放（保留 normalClaimed）→ 全部释放 → 删除条目；独占者重复声明允许更新回调（isRepeat，usb_service.cpp:940-952） |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 独占期间他人任何该接口传输被拒（BulkRead/Write/SubmitTransfer 前置 CheckExclusiveTransfer，usb_service.cpp:1466/1503/1539/1815）；独占与普通声明互斥建立 |
| 并发约束 | claimMutex_ 全程持有（usb_service.h:347）；回调触发在锁外（usb_service.cpp:866-868） |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | 附着于 UsbDevice 的接口维度 |
| 下游影响 | 传输域四类入口的放行判定；IClaimExclusiveCallback（interfaces/innerkits/IClaimExclusiveCallback.idl）向应用通知冲突 |
| 平级关联 | HDI ClaimInterface/ReleaseInterface 是其物理层对应 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbService`（状态容器与四个 DeathRecipient 之一） |
| 核心方法 | `ClaimInterfaceExclusive`（usb_service.cpp:929）、`RegisterExclusiveCallback`（usb_service.cpp:962）、`ReleaseClaimState`（usb_service.cpp:1001）、`CheckExclusiveTransfer`（usb_service.cpp:876）、`NotifyExclusiveOwner`（usb_service.cpp:851）、`RemoveAllClaimByDevice`（usb_service.cpp:913） |

## 应用-设备连接关系（deviceConnectionMap_ 与监听者）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 连接记录（DeviceConnectionRecord）与连接监听（deviceListeners_） |
| 实体定义 | 哪些应用打开了哪个设备的登记表，与系统应用注册的"连接变化观察者"集合；两者配合向设置等系统应用呈现应用-设备占用关系 |
| 核心特征 | deviceConnectionMap_：map\<"busNum-devAddr" → {UsbDevice device, map\<uid→bundleName\> apps}\>（usb_service.h:366）；deviceListeners_：list\<DeviceListenerEntry{listener, deathRecipient}\>（usb_service.h:364，类型见 usb_device_connection_type.h） |
| 类型/分类 | 单设备可多应用并存；监听者按 AsObject 去重 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 登记：OpenDevice 成功 → `AddDeviceConnection`（usb_service.cpp:696：GetCallingInfo 取 uid/bundleName）→ `NotifyDeviceConnection` 广播 CONNECT。撤销：Close 成功 → `RemoveDeviceConnection`（usb_service.cpp:730，按 uid 清，空则删键）；设备拔出 → `RemoveAllDeviceConnections`（usb_service.cpp:774）对每个 app 发 DISCONNECT。监听：RegisterConnectionListener（usb_service.cpp:552，CheckSysApiPermission 前置 + 回放存量 usb_service.cpp:590-607）；UnRegisterConnectionListener（usb_service.cpp:612）；监听者死亡由 DeviceListenerDeathRecipient 清理（usb_service.cpp:638-663） |
| 状态流转 | 记录生命周期 = OpenDevice→Close 区间；监听者生命周期 = 注册→注销/进程死亡 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 仅系统 API 权限者可注册监听（UEC 门槛 usb_service.cpp:558）；同一监听对象重复注册返回 UEC_SERVICE_ALREADY_EXISTS（usb_service.cpp:569-571） |
| 并发约束 | deviceConnectionMutex_ / deviceListenerMutex_ 各自独立（usb_service.h:365-367）；通知在锁外逐个回调（usb_service.cpp:669-685） |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | 依赖 OpenDevice/Close 的调用边界 |
| 下游影响 | IUsbConnectionCallback::OnDeviceConnected/Disconnected（interfaces/innerkits/IUsbConnectionCallback.idl） |
| 平级关联 | UsbDevice 提供设备快照；GetCallingInfo 取应用身份 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbService`、`IUsbConnectionCallback` |
| 核心方法 | `AddDeviceConnection`（usb_service.cpp:696）、`RemoveDeviceConnection`（usb_service.cpp:730）、`RemoveAllDeviceConnections`（usb_service.cpp:774）、`NotifyDeviceConnection`（usb_service.cpp:667）、`RegisterConnectionListener`（usb_service.cpp:552）、`UnRegisterConnectionListener`（usb_service.cpp:612） |

## 串口会话（serialClientMap_ / UsbSerialPort）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 串口端口（UsbSerialPort）与串口打开会话（serialClientMap_） |
| 实体定义 | 串口端口是 ttyUSB/ttyACM 类设备的端口抽象（portId/busNum/devAddr/VID/PID/serialNum，interfaces/innerkits/native/include/usb_serial_type.h）；会话是应用 SerialOpen 之后建立的 (portId → 回调对象+死亡监听) 占用关系 |
| 核心特征 | `serialClientMap_`：map\<portId → {serialRemote, serialRecipient}\>（usb_service.h:395-399）；SerialManager 内部另有 portTokenMap_（portId→tokenId，serial_manager.h:74）与 serialPortMap_（serial_manager.h:75） |
| 类型/分类 | 每端口单会话：重复 Open 报 UEC_SERIAL_PORT_REPEAT_OPEN（serial_manager.cpp:91-125 CheckPortAndTokenId） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 建立：SerialGetPortList（usb_service.cpp:3458，按需懒建 SerialManager 并 UpdateDeviceVidPidMap）→ RequestSerialRight 授权 → `SerialOpen`（usb_service.cpp:3231：校验端口/权限 → 挂 SerialDeathRecipient → SerialManager::SerialOpen（serial_manager.cpp:91，portTokenMap_ 登记）→ serialClientMap_ 登记）。使用：SerialRead/Write/Get/SetAttribute（usb_service.cpp:3312-3436，前置 ValidateUsbSerialManagerAndPort（usb_service.cpp:3214））。撤销：SerialClose（usb_service.cpp:3283）/客户端死亡（SerialDeathRecipient（usb_service.cpp:3047-3054）：FreeTokenId + CancelSerialRight 自动撤销权限并关端口） |
| 状态流转 | 端口未打开 → 打开（记录 tokenId）→ 首次读/写触发审计（portsHasBeenRead_/Written_，serial_manager.h:81-82）→ 关闭（FreeTokenId 清 map）；进程死亡等价关闭且权限同步删除 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 读写上限 64KB/200KB（usb_service.cpp:68-69）；权限键为端口对应 VID-PID-Serial（serialVidPidMap_）；死亡回收是全链（tokenId + 权限 + 端口） |
| 并发约束 | serialClientMutex_（usb_service.h:400）、serialPidVidMapMutex_（usb_service.h:360）、SerialManager 内 serialPortMapMutex_/portTokenMapMutex_（serial_manager.h:79-80） |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | NAPI serial 模块（serial_info.cpp:963-975）持有会话 |
| 下游影响 | HDI ISerialInterface 执行实际读写；SERIAL_OPERATION/OPERATION_FAULT 埋点 |
| 平级关联 | 权限域复用 usbRight.db；设备拔出（DEVDOWN）触发端口表更新 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `SERIAL::SerialManager`（services/native/src/serial_manager.cpp）、`UsbService::SerialDeathRecipient` |
| 核心方法 | `SerialOpen/Close/Read/Write`（usb_service.cpp:3231/3283/3312/3338）、`SerialManager::SerialOpen`（serial_manager.cpp:91）、`FreeTokenId`（serial_manager.cpp:305）、`UpdateDeviceVidPidMap`（usb_service.cpp:3747）、`CheckForTtyUSB`（usb_service.cpp:3125） |

## USB 配件会话（USBAccessory / accessoryClientMap_）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | USB 配件（USBAccessory）及其打开会话 |
| 实体定义 | AOA 配件对象（manufacturer/model/version/serial 等，interfaces/innerkits/native/include/usb_accessory.h）与 app 打开后获得的 fd 会话（accessoryClientMap_：map\<(fd,tokenId) → {remote, recipient, tokenId}\>，usb_service.h:372-378） |
| 核心特征 | 配件身份经 `SerialValueHash`（usb_accessory_manager.cpp:416）归一为权限键；fd 是应用直通内核节点的句柄 |
| 类型/分类 | 配件生命周期状态由 usbd 上报（ACT_ACCESSORYUP/DOWN/SEND，usb_service.cpp:3152-3153） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 启动：HandleEvent → ProcessAccessoryStart（usb_accessory_manager.cpp:221 设置功能 + GetAccessoryInfo 收集）。使用：GetAccessoryList（usb_service.cpp:2333）→ RequestAccessoryRight（usb_service.cpp:2517）→ OpenAccessory（usb_service.cpp:2358：HasAccessoryRight → usbAccessoryManager_->OpenAccessory(fd)（usb_accessory_manager.cpp:160 打开 /dev 节点）→ AccessoryDeathRecipient 挂钩 → accessoryClientMap_ 登记）。撤销：CloseAccessory（usb_service.cpp:2414）/客户端死亡（AccessoryDeathRecipient（usb_service.cpp:3058-3067）：移除登记 + 关 fd） |
| 状态流转 | 配件未启动 → 启动（可被 ProcessAccessoryStop（usb_accessory_manager.cpp:264）停止）→ 应用获得 fd 会话 → 关闭；会话键含 tokenId 故同一 fd 不同应用隔离 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 权限与设备授权同库同键语义（HasRight 复用，usb_service.cpp:2510）；OpenAccessory 必须已有权限（usb_service.cpp:2382-2387） |
| 并发约束 | accessoryClientMutex_（usb_service.h:378） |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | NAPI usb 模块配件接口（usb_info.cpp:3229-3235） |
| 下游影响 | /dev 节点 fd 直通应用；ACT_ACCESSORYSEND 经 ProcessAccessorySend 转发数据（usb_accessory_manager.cpp:290） |
| 平级关联 | 权限域、gadget 功能域（配件启动依赖功能切换） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbAccessoryManager`（usb_accessory_manager.cpp）、`UsbService::AccessoryDeathRecipient` |
| 核心方法 | `GetAccessoryList`（usb_service.cpp:2333）、`OpenAccessory/CloseAccessory`（usb_service.cpp:2358/2414）、`HandleEvent`（usb_accessory_manager.cpp:334）、`ProcessAccessoryStart/Stop`（usb_accessory_manager.cpp:221/264）、`GetAccessorySerialNumber`（usb_accessory_manager.cpp:507） |

## 设备侧功能状态（currentFunctions_）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 设备功能位图（currentFunctions_） |
| 实体定义 | 本机作为 USB 设备当前对外呈现的功能集合（hdc/mtp/ptp/storage 等 FUNCTION_* 位组合），是 gadget 域的核心可变状态 |
| 核心特征 | 缓存于 `UsbDeviceManager::currentFunctions_`（usb_device_manager.h:89，默认 FUNCTION_HDC）；真值在 HDI gadget 侧，Init/连接时 GetCurrentFunctions 校准（usb_device_manager.cpp:430/482）；settable 白名单 functionSettable_（usb_device_manager.h:85-87） |
| 类型/分类 | 位掩码；字符串形态经 ConvertToString/ConvertFromString（usb_device_manager.cpp:301/338）互转（逗号分隔如 "mtp,ptp"） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 设置：SetCurrentFunctions（usb_service.cpp:2222 → usb_device_manager.cpp:201/233 双模实现：读旧值→幂等短路→HDI 设置→UpdateFunctions）。事件驱动：gadget 连接/断开事件 → HandleEvent（usb_device_manager.cpp:416/468）延迟定时任务中改写（断开自动回退 MTP/PTP/DEVMODE_AUTH → STORAGE，usb_device_manager.cpp:441-451）。用户切换：UserChangeProcess（usb_device_manager.cpp:521）在切户时复位 MTP/PTP。观察：ProcessFuncChange（usb_device_manager.cpp:576）→ 功能弹窗 + BroadcastFuncChange 公共事件 + 通知 |
| 状态流转 | 初值 FUNCTION_HDC → SetCurrentFunctions/事件/切户改写 → 全部以 HDI GetCurrentFunctions 为校准基准；HDC 位同步到设置数据 HDC_STATUS（usb_service.cpp:2607-2639） |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 只有 functionSettable_ 内位可设（IsSettableFunctions，usb_device_manager.cpp:295-299）；设置前有系统 API 权限 + HasSetFuncRight 两道校验（usb_service.cpp:2229-2240，权限实现在 usb_right_manager.cpp:742）；FUNCTION_CHANGED 埋点记录新旧值（usb_device_manager.cpp:823） |
| 并发约束 | functionMutex_ 保护设置临界区（usb_device_manager.cpp:209/241），deviceMutex_ 保护事件状态（usb_device_manager.cpp:418） |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | HDI IUsbDeviceInterface/V1_2 IUsbInterface 执行真值 |
| 下游影响 | 功能弹窗（UsbFunctionSwitchWindow）、连接通知（UsbConnectionNotifier）、COMMON_EVENT_USB_FUNCTION 系公共事件、设置 HDC_STATUS |
| 平级关联 | 反向充电状态（IsReverseCharge）抑制弹窗（usb_service.cpp:2325-2327） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbDeviceManager`（usb_device_manager.cpp） |
| 核心方法 | `SetCurrentFunctions`（usb_device_manager.cpp:201/233）、`GetCurrentFunctions`（usb_device_manager.cpp:181/377）、`HandleEvent`（usb_device_manager.cpp:416/468）、`UserChangeProcess`（usb_device_manager.cpp:521）、`ProcessFuncChange/BroadcastFuncChange`（usb_device_manager.cpp:576/547）、`IsSettableFunctions`（usb_device_manager.cpp:295） |

## USB 端口（UsbPort）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | USB 端口（UsbPort） |
| 实体定义 | 一个物理 USB-C 端口的角色与模式画像：id、当前 powerRole/dataRole/mode、硬件 supportedModes（interfaces/innerkits/native/include/usb_port.h） |
| 核心特征 | 容器 `UsbPortManager::portMap_`（map\<portId, UsbPort\>）与 supportedModeMap_（usb_port_manager.h:75-76）；初始化经 QueryPort 拉取（usb_port_manager.cpp:284） |
| 类型/分类 | 支持模式 PORT_MODE_NONE/DEVICE/HOST（usb_srv_support.h:46-50）；角色枚举 PowerRole/DataRole（usb_srv_support.h:34-44） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 查询：GetPorts（usb_port_manager.cpp:245）/GetSupportedModes（usb_port_manager.cpp:270）。切换：SetPortRole（usb_port_manager.cpp:185，系统 API 权限在 usb_service.cpp:2685 前置）→ HDI。事件：PortChangedEvent（usb_manager_subscriber.cpp:35）→ UpdatePort（usb_port_manager.cpp:325/348 双签名）→ 发布 COMMON_EVENT_USB_PORT_CHANGED（JSON：portId/powerRole/dataRole/mode/supportedModes）。Init 时 QueryPort + AddSupportedMode 构建初始表（usb_port_manager.cpp:60-86） |
| 状态流转 | 表内容随 HDI 事件更新；SetPortRole 结果异步体现在后续 PortChangedEvent；不支持切换返回 UEC_SERVICE_NOT_SUPPORT_SWITCH_PORT（usb_service.cpp:2703-2706） |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 端口角色切换影响在连设备（AGENTS.md 硬件警告：不得在真机随意测试）；反向充电期间 IsReverseCharge 供功能域抑制弹窗（usb_port_manager.cpp:167-183） |
| 并发约束 | mutex_ 保护 portMap_（usb_port_manager.h:73），setPortRoleMutex_ 串行化切换（usb_port_manager.h:74） |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | HDI IUsbPortInterface/V1_2 IUsbInterface::QueryPort |
| 下游影响 | 角色决定主机/设备域谁生效（DRP 平台）；PORT_ROLE_CHANGED 埋点（usb_port_manager.cpp:523） |
| 平级关联 | UsbDeviceManager 用其充电状态；订阅者事件为唯一刷新源 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `UsbPortManager`（usb_port_manager.cpp）、`UsbPort` |
| 核心方法 | `Init`（usb_port_manager.cpp:60）、`QueryPort`（usb_port_manager.cpp:284）、`SetPortRole`（usb_port_manager.cpp:185）、`UpdatePort`（usb_port_manager.cpp:325/348）、`AddPortInfo/AddPort/RemovePort`（usb_port_manager.cpp:372/384/397）、`IsReverseCharge`（usb_port_manager.cpp:167） |
