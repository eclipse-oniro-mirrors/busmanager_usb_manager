# USB 设备会话管理与异常退出清理设计

> 仓库:usb_manager(SA 4201)
> 关联特性:应用连接 USB 设备状态监听 InnerAPI(RegisterConnectionListener)
> 状态:设计稿 v6 —— **死亡锚点方案**(binder 死亡通知)+ 条件注入:HAP 调用方注入锚点、native SA 传空免 SELinux transfer 策略;含基线适配修复(GetCallingAppUid 一致性)

## 1. 背景与目标

应用经 `UsbSrvClient::OpenDevice` 打开设备后,HDI 层(usbd/libusb_adapter)**按 Open/Close 调用配对维护引用计数**(count++,减到 0 才真正 `libusb_close`)。当前生命周期依赖应用主动 Close,存在四类失配场景:

| 场景 | 后果 |
|---|---|
| 应用 crash / 被杀,未调 Close | HDI 计数永不递减,设备句柄/fd 卡开直到拔出 |
| 应用 crash 后独占接口未释放 | 独占卡死,其他应用无法申请 |
| 应用 crash 后 | 监听方看到陈旧 CONNECT,收不到 DISCONNECT |
| usb_service 按需卸载重启(HAL 模式) | 无主占用无人认领,新应用 open 失败 |

**核心诉求**:HAP 调用方不 Close 直接退出时,连接状态(含独占)**实时**回收。

## 2. 双监听机制

| 监听 | 数据结构 | 职责 |
|---|---|---|
| **监听①:连接广播** | `deviceConnectionMap_["bus-dev"] → apps: map<uid, bundleName>` | 对外广播"哪个应用连着哪个设备" |
| **监听②:会话表 + 死亡锚点** | `deviceClientMap_[(busNum,devAddr,tokenId)] → DeviceClientInfo{deviceRemote, deviceRecipient, tokenId, uid}` | 资源真值;锚点 `DeviceDeathMonitor`(客户端桩,系统侧自动注入)+ `DeviceDeathRecipient`(服务端通知,身份固化)感知持有者消亡 |

两张表在三个终点事件(正常 Close / 应用死亡 / 设备拔出)同步迁移;清理统一收敛到 `ReleaseDeviceClient`。与串口(SerialDeathRecipient)、Accessory、Claim 的既有死亡清理机制同构。

## 3. 检测机制选型(决策记录)

| 维度 | **锚点(binder 死亡通知)—— 最终采用** | A:AMS 观察者(否决) | B:5s 巡检(否决) |
|---|---|---|---|
| 检测延迟 | **毫秒级(内核直推)** | 毫秒级(AMS 中转) | ≤5s |
| HAP 覆盖 | **~100%**(UsbSrvClient 系统侧自动注入,应用零改造) | ~100% | ~100% |
| native SA 覆盖 | **✓(顺带覆盖)** | ✗(AMS 只广播 HAP) | ✓ |
| 事件可靠性 | **内核保证,免疫丢失** | ✗ oneway 拥塞/AMS 重启会漏 | ✓ 自包含 |
| 外部依赖 | **零** | ability_runtime:app_manager | 零 |
| SELinux | usb 侧**一条** transfer 规则(见 §6) | 预期零 | usb 侧一条 /proc 规则 |
| IDL 变更 | **要**(3 参,已合入,评审中) | 零 | 零 |
| 功耗 | 零 | 零 | 周期唤醒(否决原因) |

**决策理由**:客户端系统侧注入使覆盖自动 ~100%、内核直推保证可靠性与实时性、SA 顺带覆盖、零外部依赖;代价为 IDL 加参(已完成)与一条 usb 自管策略(评估接受)。AMS 否决:外部依赖 + SA 盲区 + 事件丢失无兜底;巡检否决:功耗。
**修正记录**:早期评估"锚点依赖应用配合改造"仅适用于应用自行传对象的形态;按本仓库既有模式(客户端注入),HAP 自动全覆盖。

## 4. 最终实现要点与基线适配

1. **锚点注册链(条件注入)**:UsbSrvClient 按自身 token 类型判定——HAP(TOKEN_HAP)注入锚点桩,native/shell 传空(parcel 不携带 binder 对象,免除 transfer 策略);服务端 deviceRemote 非空 → 创建 DeviceDeathRecipient 并注册,为空 → 跳过锚点、open 正常执行;HDI open 失败 → **RemoveDeathRecipient 回滚**(防僵尸通知);成功 → 会话表入账(同 key 重开先注销旧锚点)→ 连接广播
2. **统一清理链 `ReleaseDeviceClient`**(死亡/兜底共用,幂等查表):锁内查 key 快照 uid → 定向清独占 claim(显式 token)→ HDI Close(引用计数 -1,多应用安全)→ 清会话表+注销锚点 → `ReleaseDeviceConnectionByUid` 清连接记录并广播 DISCONNECT
3. **死亡回调 `OnRemoteDied`**:GetGlobalInstance 判空 → `ReleaseDeviceClient(固化身份)`,不经过依赖 IPC 上下文的公共 Close
4. **拔出闭环**:DelDevice → RemoveAllDeviceConnections(逐 uid 广播)→ RemoveAllDeviceClients(清会话+注销锚点,不关 HDI/不广播)→ RemoveAllClaimByDevice → Close(幂等 no-op)
5. **基线适配修复(v5)**:上游已将全仓 `GetCallingUid` 统一迁移为 `GetCallingAppUid`(连接表 apps 键源);OpenDevice 会话身份捕获同步改为 `GetCallingAppUid`,**否则死亡清理按 uid 匹配连接记录会落空**(DISCONNECT 丢失)。本修复已随本设计提交

## 5. 目标行为数据流程图

### 5.1 Open(双监听建立)

```
应用调 UsbSrvClient::OpenDevice(device, pipe)   ← ArkTS 签名未变,应用零改动
 └ proxy_->OpenDevice(bus, dev, deviceRemote)    ← 锚点由系统侧 UsbSrvClient 自动注入
 UsbService::OpenDevice(busNum, devAddr, deviceRemote)
 ① 权限检查(IPC 上下文正常)
 ② 固化身份:tokenId/uid(GetCallingAppUid,与连接表键源一致)
 ③ new DeviceDeathRecipient(bus,dev,tokenId) → AddDeathRecipient    【监听②:死亡锚点】
 ④ usbHostManager_->OpenDevice → HDI 引用计数 +1
    └ 失败 → RemoveDeathRecipient 回滚 → return
 ⑤ 锁内入表(同 key 重开先注销旧锚点)                                【监听②:会话表】
 ⑥ AddDeviceConnection → 锁外 oneway 广播 CONNECT                    【监听①】
```

### 5.2 Close(双监听拆除)

```
Close(busNum, devAddr)
 ① 权限检查 + RemoveExclusiveClaimByDevice(调用者 token)
 ② usbHostManager_->Close → HDI 引用计数 -1
 ③ 成功:RemoveDeviceClientInfo(清会话表+注销锚点)+ RemoveDeviceConnection(广播 DISCONNECT)
 ④ 失败:仅日志,条目保留(可重试;应用后续死亡由锚点兜底)
```

### 5.3 应用异常退出未 Close(内核直推)

```
应用进程消亡(锚点 DeviceDeathMonitor 随进程死亡)
 └ binder 驱动 → DeviceDeathRecipient::OnRemoteDied(binder 线程,毫秒级)
    ├ GetGlobalInstance();null → return
    └ ReleaseDeviceClient(固化 tokenId,不依赖 IPC 上下文)
       定向清独占 → HDI Close(计数-1)→ 清会话表+注销锚点
       → ReleaseDeviceConnectionByUid → 广播 DISCONNECT
 → 监听方毫秒级收到 OnDeviceDisconnected,与正常 Close 表现一致
```

### 5.4 设备拔出(双表同步)

```
usbd → UsbServiceSubscriber::DeviceEvent{ACT_DEVDOWN} → DelDevice
 ① RemoveAllDeviceConnections → 逐 uid 广播 DISCONNECT      【监听①】
 ② RemoveAllDeviceClients → 清该设备全部会话+注销锚点       【监听②】(不关 HDI/不广播)
 ③ RemoveAllClaimByDevice → ④ Close(HDI CloseDevice,表内查无均幂等 no-op ✓)
```

## 6. SELinux 权限说明(条件注入后)

| 调用方 | 锚点/对象传递 | 策略状态 |
|---|---|---|
| 普通 HAP(normal_hap / debug_hap) | ✓ 注入锚点 | **规则现成**:`allow normal_hap usb_service:binder { transfer }`(usb_service.te)/ debug 同条 |
| native SA(TOKEN_NATIVE) | **✗ 传空** | **零策略**:parcel 无对象即无 transfer 检查,凭既有 `binder call` 即通 |
| 系统 HAP(system_basic/core_hap) | ✓ 注入锚点 | **缺 2 条**——仅当系统应用也需要死亡监听时补(usb 组件 te,带属性一条/域);亦可确认无此消费方后不补 |
| 监听消费方 resource_schedule_service | cb 对象 | **已全通**:`resource_schedule_service usb_service:binder { call transfer }` + 回调方向 `usb_service → rss: call` 均已存在(带 avc 修复注释) |
| 应用侧 | — | 零改动、零概念:SELinux 是系统镜像策略,应用无感;判定在系统镜像内 UsbSrvClient,对调用方零 API |

验证方法:permissive 下全量回归(HAP openDevice、SA openDevice、监听注册),收集 `hilog | grep avc` 确认零缺失后切 enforcing 复验。

## 7. 独占(ClaimInterfaceExclusive)与监听联动场景

| # | 场景 | 无清理链 | 有清理链 |
|---|---|---|---|
| 1 | 独占者 crash | exclusiveOwner 卡死为死者 token,他人**永远申请不到**;监听方却看到其已掉线——观测自相矛盾 | 独占释放与 DISCONNECT 广播**同刻发生**(锚点毫秒级),观测一致 |
| 2 | A 死后 B 抢独占 | B 申请失败时长 = 检测延迟 | 锚点毫秒释放,B 基本无感 |
| 3 | A 独占 X 死亡,B 用同设备 Y 接口 | 整表连坐会误伤 | 按 (bus,dev,token) 定向清,B 不受影响 |
| 4 | 接口独占+普通 claim 并存 | 可能连清普通持有 | 按 owner 匹配,normalClaimed 降级语义不动 |
| 5 | 既有事实 | — | 独占已有实时死亡保护(cb 参数既有设计,ClaimDeathRecipient);清理链中的 claim 清理是收敛/兜底,二者幂等共存 |
| 6 | 拔出 | — | RemoveAllClaimByDevice 与逐应用 DISCONNECT 同步(已有逻辑) |

## 8. 设计决策与待确认项

1. **同 key 重复 open 配对语义**:HDI 计数按调用配对 vs 会话表按 key 唯一,open×2 + close×1 + 死亡兜底×1 恰平衡;极端场景计数残留 +1,记录为已知边界
2. **直通模式 SA 重启即进程级释放**;HAL 模式 usbd 独立进程持有计数,遗留本期不处理
3. 设备策略禁用路径的服务端强制关设备不自动清会话表,保留至应用死亡/拔出;建议后续补挂 `RemoveAllDeviceClients`
4. **策略评审**:§6 的 transfer 规则需走 sepolicy 评审流程(usb 侧一条,带 hap 属性);IDL 3 参变更已合入,公共接口评审随 PR 流程

## 9. 边界与已知限制

- native/SA 调用者不产生连接广播(GetCallingInfo 跳过 native token),会话表照记、锚点照常覆盖其死亡(HDI 资源可回收)
- 检测依赖 usb_service 存活:SA 卸载期间应用死亡无人接收;重启后内存表丢失,直通模式无残留、HAL 模式计数遗留
- 同源遗留(另行整改):串口 `CancelSerialRight` 在死亡回调中依赖调用者身份,右记录清理实际不生效

## 10. 测试要点

1. 正常 Open/Close:双表清理、锚点注销、广播各一次
2. crash(唯一持有者):**毫秒级** OnRemoteDied → 清理链;监听方 DISCONNECT 与 Close 表现一致;HDI 计数归零
3. crash(双应用同设备):只清死者,第二应用读写不受影响
4. **独占场景**:独占者 crash → 独占释放与 DISCONNECT 同刻;独占+普通 claim 并存降级
5. 重复 open 同 key:旧锚点注销、表项覆盖;OpenDevice 失败无残留锚点
6. 拔出:先广播后清表,无重复 DISCONNECT
7. uid 一致性(本版关键回归):`GetCallingAppUid` 作为会话 uid 与连接表键一致,crash 后 DISCONNECT 必达
8. **SA 调用 openDevice(条件注入回归)**:open 成功且 parcel 不携带锚点对象,零 transfer 相关 avc;该会话无死亡清理(设计语义)
9. SELinux:permissive 收集 avc → 按需合规则 → enforcing 全量回归
10. mock 回归:`usb_unittest_test` / `service_unittest`(native 测试进程自动走传空分支,用例无需改动)
