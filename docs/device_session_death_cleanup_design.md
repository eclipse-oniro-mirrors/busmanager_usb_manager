# USB 设备会话管理与异常退出清理设计

> 仓库:usb_manager(SA 4201)
> 关联特性:应用连接 USB 设备状态监听 InnerAPI(RegisterConnectionListener)
> 状态:设计稿(基于本地 WIP 未提交修改,行号以工作区当前代码为准;代码级修改方案另行提供)

## 1. 背景与目标

应用经 `UsbSrvClient::OpenDevice` 打开设备后,HDI 层(usbd/libusb_adapter)**按 Open/Close 调用配对维护引用计数**(count++,减到 0 才真正 `libusb_close`,见 drivers_peripheral/usb `libusb_adapter.cpp:342-348/386-409`)。当前生命周期依赖应用主动 Close,存在四类失配场景:

| 场景 | 后果 |
|---|---|
| 应用 crash / 被杀,未调 Close | HDI 计数永不递减,设备句柄/fd 卡开直到拔出 |
| usb_service 按需卸载重启(HAL 模式) | 无主占用无人认领,新应用 open 失败 |
| binder 死亡通知延迟/丢失(极端) | 无兜底 |
| 应用 crash 后 | 监听方看到陈旧 CONNECT,收不到 DISCONNECT |

目标:**Open/Close/异常退出/设备拔出**四条路径全部与 HDI 引用计数严格配对;双监听(连接广播 + 死亡锚点)状态与资源真值不脱节。

## 2. 双监听机制

| 监听 | 数据结构 | 职责 |
|---|---|---|
| **监听①:连接广播** | `deviceConnectionMap_["bus-dev"] → apps: map<uid, bundleName>`(usb_service.h:388) | 对外广播"哪个应用连着哪个设备",喂给 `RegisterConnectionListener` 的系统应用 |
| **监听②:死亡锚点会话表** | `deviceClientMap_[(busNum,devAddr,tokenId)] → DeviceClientInfo`(usb_service.h:379-385,WIP 新增) | 资源真值:谁开着设备;锚点 `DeviceDeathMonitor`(客户端桩)+ `DeviceDeathRecipient`(服务端死亡通知)负责感知持有者消亡 |

两张表在三个终点事件(正常 Close / 应用死亡 / 设备拔出)同步迁移。

## 3. 现状问题分析(基于本地 WIP)

### 3.1 编译级问题(当前 HOST 配置下不可编译)

1. `usb_service.cpp:562`:`uin  t32_t tokenId` 笔误,且与 :553 重复声明
2. `RemoveDeviceClientInfo` 声明(.h:338)与实现(.cpp:1784-1798)均被注释,但 :563、:3106 两处仍在调用
3. `usb_service.h` 缺 `#include <tuple>`(:384 的 map key 用了 `std::tuple`,目前靠传递包含侥幸)
4. mock 测试 7 处 2 参 `OpenDevice` 调用点断裂(usb_request_mock_test.cpp:87;usb_device_pipe_mock_test.cpp:111/166/1283/1303/1323;usb_bulkcallback_mock_test.cpp:119)

### 3.2 设计级问题:死亡回调不能走公共 Close

WIP 的 `DeviceDeathRecipient::OnRemoteDied`(:3100-3109)调用 `service->Close(busNum_, devAddr_)`。binder 死亡回调线程**没有 IPC 调用上下文**,`GetCallingTokenID()` 返回 usb_service 自身身份而非死亡应用:

| Close 内部步骤 | 死亡线程中的行为 |
|---|---|
| `CheckDevicePermission`(:549→HasRight→GetCallingInfo) | 查错误身份的授权 → 大概率拒绝直接 return(HDI 计数永不递减);即便 SA 旁路通过,后续步骤全部作用于错误身份 |
| `RemoveExclusiveClaimByDevice`(wrongToken) | 空转,死者独占状态泄漏 |
| `RemoveDeviceClientInfo`(wrongToken) | 空转(冗余,OnRemoteDied 已用正确 token 清过) |
| `RemoveDeviceConnection`→`GetCallingInfo` 失败(:761-763) | 提前 return:连接记录残留、**监听方收不到 DISCONNECT** |

仓库惯例佐证:没有任何既有死亡回调调用带 IPC 权限检查的公共方法——`ClaimDeathRecipient`(:3143,教科书:身份固化+锁内决策锁外执行+manager 直调)、`UsbSubmitTransferDeathRecipient`(usb_host_manager.cpp:203,直调 manager 层 Close)、串口 `FreeTokenId`(校验归属→HDI close→清簿记)。

### 3.3 关键机制事实

- **HDI 引用计数按"调用次数"配对**,不区分应用身份。多应用同设备时一方 Close 只递减计数,不会误伤他人(减到 0 才真关)。因此**应用层无需多持有者扫描**;代价是 Open/Close 必须严格 1:1 配对
- 串口先例顺序(`SerialManager::FreeTokenId`,serial_manager.cpp:305-322):先锁内校验归属并 erase → 锁外 HDI close → 再清服务层簿记
- 防误删先例:`portTokenMap_[portId] == tokenId`(serial_manager.cpp:310)、`exclusiveOwner == tokenId_`(usb_service.cpp:3156)

## 4. 目标行为数据流程图

### 4.1 Open 之后(双监听建立)

```
应用调 UsbSrvClient::OpenDevice(device, pipe)
 └ proxy_->OpenDevice(bus, dev, deviceRemote)      [usb_srv_client.cpp:139]
    deviceRemote = DeviceDeathMonitor 进程内单例(:44 构造)——随应用进程生灭
 UsbService::OpenDevice                                        [usb_service.cpp:505]
  ① CheckDevicePermission(IPC 上下文正常)
  ② tokenId/callingUid = GetCallingTokenID/GetCallingUid(入口取)
     key = (busNum, devAddr, tokenId)
  ③ new DeviceDeathRecipient(bus,dev,tokenId) → AddDeathRecipient(:525)
     【监听②:死亡锚点注册——持有者进程消亡时 binder 驱动回调】
  ④ usbHostManager_->OpenDevice → HDI 引用计数 +1
     └ 失败 → RemoveDeathRecipient 回滚(:533)→ return
  ⑤ 锁内:同 key 先注销旧锚点 → deviceClientMap_[key] = {remote, recipient, tokenId, uid}
     【监听②:会话表入账——资源真值】
  ⑥ AddDeviceConnection(:720)
     GetCallingUid+GetCallingInfo → deviceConnectionMap_["bus-dev"].apps[uid]=bundleName
     【监听①:连接广播——锁外 oneway OnDeviceConnected(device, uid, bundleName)】
```

### 4.2 Close 之后(双监听拆除)

```
UsbService::Close                                              [usb_service.cpp:547]
 ① CheckDevicePermission(IPC 上下文正常)
 ② tokenId = GetCallingTokenID(:553) → RemoveExclusiveClaimByDevice(:554)
 ③ usbHostManager_->Close → HDI 引用计数 -1
 ④ 成功:
    RemoveDeviceClientInfo(bus,dev,tokenId)   【监听②:清会话表+注销锚点】(清理置于 HDI close 成功之后)
    RemoveDeviceConnection(bus,dev)           【监听①:清 apps[uid] + 广播 DISCONNECT】
 ⑤ 失败:仅日志,两表条目保留(可重试;应用后续 crash 由死亡兜底链清理)
```

### 4.3 应用异常退出未 Close(crash / kill -9)

```
应用进程消亡(锚点 DeviceDeathMonitor 随进程死亡)
 └ binder 驱动 → DeviceDeathRecipient::OnRemoteDied [binder 线程,异步]
     ├ GetGlobalInstance();null → return(判空)
     └ ReleaseDeviceClient(busNum_, devAddr_, tokenId_)   ← 全程只用固化身份,不依赖 IPC 上下文
        ① 锁内 find(key=(bus,dev,tokenId));查无 → 幂等 return;查有 → 快照 uid
        ② RemoveExclusiveClaimByDevice(bus,dev,tokenId)    定向清死者独占 claim
        ③ usbHostManager_->Close → HDI 引用计数 -1(多应用同设备安全:减到 0 才真关)
        ④ RemoveDeviceClientInfo → 清 deviceClientMap_ + 注销锚点
        ⑤ ReleaseDeviceConnectionByUid(bus,dev,uid)
           锁内 find(禁 operator[])→ 快照{device, bundleName 存量}
           → erase apps[uid];apps 空则 erase 整条
           → 锁外 NotifyDeviceConnection(DISCONNECT)
              → 监听方收到 OnDeviceDisconnected —— 与正常 Close 表现完全一致
```

### 4.4 设备拔出(双表同步)

```
usbd(HDI 回调)→ UsbServiceSubscriber::DeviceEvent{busNum,devAddr,ACT_DEVDOWN}
 → UsbService::DeviceEvent → DelDevice                          [usb_service.cpp:1297]
 ① RemoveAllDeviceConnections → 逐 uid 广播 DISCONNECT          【监听①】(已有)
 ② RemoveAllDeviceClients(新增)                                 【监听②】
    锁内拷贝整条 value(sptr 保活)+ erase → 锁外逐条 RemoveDeathRecipient
    (不调 HDI Close:设备已移除;不广播:①已完成)
 ③ RemoveAllClaimByDevice
 ④ Close(...) → HDI CloseDevice
    └ RemoveDeviceClientInfo(HDI native token) → key 查无 → 幂等 no-op ✓
```

## 5. 新方案相对原 WIP 方案的优点

| # | 维度 | 原 WIP 现状 | 新方案 |
|---|---|---|---|
| 1 | 可编译性 | `uin t32_t` 笔误、调用被注释函数、缺 `<tuple>`、mock 7 处断裂 | 笔误/注释函数/头文件/mock 调用点全部修复 |
| 2 | 死亡链路有效性 | `OnRemoteDied→Close()`:死亡线程权限/身份失效,大概率整链空转 | `ReleaseDeviceClient` 全程固化身份,与仓库惯例(Claim/Serial/Accessory)同构 |
| 3 | HDI 计数配对 | Close 被拒 → 计数永不递减 → 设备永久卡开 | 可靠递减;Close 清理移到成功后(失败不孤儿);拔出闭环(RemoveAllDeviceClients) |
| 4 | 独占 claim 清理 | 死亡路径用错 token 空转,claim 状态泄漏 | 显式传 tokenId 定向清理(已验证 :916-935 无 IPC 依赖) |
| 5 | 监听①广播一致性 | crash 后连接记录残留直到拔出,监听方收不到 DISCONNECT | open 时固化 uid,`ReleaseDeviceConnectionByUid` 用存量数据清理+广播,与正常 Close 一致 |
| 6 | 幂等与竞态 | 重复死亡通知/拔出后残余通知行为不定 | key 查表幂等;重开注销旧锚点;插表前移收窄死亡竞态窗口 |
| 7 | 内存/UB 安全 | — | null 检查;find 禁 operator[](防幽灵条目);value 拷贝防悬垂;窄化转换修正 |
| 8 | 评审/维护成本 | 自创调用公共 Close 的新模式 | 与三套已出货死亡清理同构,命名结构 100% 沿用 WIP(deviceClientMap_/DeviceClientInfo/RemoveDeviceClientInfo) |

## 6. 设计决策与待确认项

1. **同 key 重复 open 的配对语义**:HDI 计数按调用配对 vs 会话表按 key 唯一,存在计数泄漏边界(open 两次计数 +2,表仅 1 条,一次 Close + 死亡兜底只递减 2 次,恰平衡;但 open 后仅 Close 一次再死亡的场景计数残留 +1);建议评估"同 key 已存在时返回 UEC_OK 但不再调 HDI Open"(保持 1:1)或记录为已知边界
2. **Sweep/ForceRecover 兜底**:直通模式下 SA 重启即进程级释放(libusb 在本进程);HAL 模式(usbd 独立进程持有计数)才存在无主占用,需要时再加"open 报占用 → 强关重试",本期不做
3. **IDL 兼容评审**:OpenDevice 加参属 innerkit 公共接口变更,需走兼容性评审;`[in]IRemoteObject` 同文件 :60-61 有先例,codegen 无兼容问题
4. `device_death_monitor.h` 为 untracked 文件,提交时需一并 add(无需改 BUILD.gn,先例 serial_death_monitor.h 同样仅头文件包含)

## 7. 边界与已知限制

- native/SA 调用者 GetCallingInfo 失败,不产生连接广播(监听①不可见),会话表(监听②)照常记录
- 设备策略禁用路径的服务端强制关设备不自动清会话表,记录保留至应用死亡/拔出;建议后续补挂 `RemoveAllDeviceClients`
- **同源遗留问题(另行整改)**:串口 `CancelSerialRight`(:3653-3662)在死亡回调中同样依赖调用者身份,右记录清理实际不生效——与本问题同根因
- `SerialDeathRecipient`/`ClaimDeathRecipient` 的 `GetGlobalInstance()` 未判空是既有隐患,新代码已判空、不照抄

## 8. 测试要点

1. 正常 Open/Close:双表清理、锚点注销、CONNECT/DISCONNECT 各一次
2. crash(唯一持有者):秒级回收;监听方收到 DISCONNECT,与 Close 表现一致;HDI 计数归零
3. crash(双应用同设备):只清死者,第二应用读写不受影响,其 Close 后计数才归零
4. kill usb_service 后重启(HAL 模式):新应用 open → 验证计数配对是否残留(直通模式预期无残留)
5. OpenDevice 失败:无僵尸锚点
6. 重复 open 同 key:旧锚点注销、表项覆盖、计数配对观察
7. 拔出:先逐 uid 广播后注销锚点,无重复 DISCONNECT;DelDevice 后表为空
8. 监听方 crash:注册自动摘除,其余监听方不受影响
9. mock 回归:`usb_unittest_test` / `service_unittest`(调用点补参后)
