# USB 设备会话管理与异常退出清理设计

> 仓库:usb_manager(SA 4201)
> 关联特性:应用连接 USB 设备状态监听 InnerAPI(RegisterConnectionListener)
> 状态:设计稿 v3 —— 检测机制选型:AMS 实时监听(主) + 异步巡检(兜底)双通道

## 1. 背景与目标

应用经 `UsbSrvClient::OpenDevice` 打开设备后,HDI 层(usbd/libusb_adapter)**按 Open/Close 调用配对维护引用计数**(count++,减到 0 才真正 `libusb_close`)。当前生命周期依赖应用主动 Close,存在四类失配场景:

| 场景 | 后果 |
|---|---|
| 应用 crash / 被杀,未调 Close | HDI 计数永不递减,设备句柄/fd 卡开直到拔出 |
| 应用 crash 后独占接口未释放 | 独占卡死,其他应用无法申请 |
| 应用 crash 后 | 监听方看到陈旧 CONNECT,收不到 DISCONNECT |
| usb_service 按需卸载重启(HAL 模式) | 无主占用无人认领,新应用 open 失败 |

**核心诉求**:HAP 调用方不 Close 直接退出时,连接状态(含独占)**实时**回收。
**约束**:OpenDevice 调用方应用众多,IDL 加参(锚点方案)会给所有调用方应用域带来 SELinux 策略,故不可行。

## 2. 双监听机制

| 监听 | 数据结构 | 职责 |
|---|---|---|
| **监听①:连接广播** | `deviceConnectionMap_["bus-dev"] → apps: map<uid, bundleName>` | 对外广播"哪个应用连着哪个设备" |
| **监听②:会话表 + 双通道死亡检测** | `deviceClientMap_[(busNum,devAddr,tokenId)] → {tokenId, uid, pid}` | 资源真值:谁开着设备;死亡检测后回收 |

两张表在三个终点事件(正常 Close / 应用死亡 / 设备拔出)同步迁移;清理统一收敛到 `ReleaseDeviceClient`。

## 3. 检测机制选型对比

| 维度 | 锚点(binder 死亡通知,已废弃) | **A:AMS 实时监听(主通道)** | **B:5s 异步巡检(兜底通道)** |
|---|---|---|---|
| 原理 | 客户端传锚点桩,服务端挂 DeathRecipient | `RegisterApplicationStateObserver` → AMS 在应用进程死亡时主动推 `OnProcessDied{pid,uid,bundleName}` | 定时器每 5s 无条件"点名":`stat("/proc/<pid>")` 判活 |
| 检测延迟 | 毫秒级 | **毫秒级(AMS 主动推)** | ≤5s(平均 2.5s) |
| IDL 变更 | OpenDevice 加 IRemoteObject 参数 ❌ | **零变更** | **零变更** |
| HAP 调用方死亡 | ✓ | ✓ 实时 | ✓ ≤5s |
| **native SA 调用方死亡** | ✓ | **✗ AMS 只广播 HAP 进程** | ✓ /proc 对任意进程有效 |
| 事件丢失/注册失败(AMS 重启、oneway 拥塞) | — | **✗ 漏一次即永久残留,无兜底** | ✓ 自包含,下轮补上 |
| SA 卸载窗口内死亡 | 盲 | 盲 | 盲(三案等价,已知边界) |
| PID 复用 | 驱动保证 | 事件自带 pid+uid+bundleName,天然免疫 | 靠 st_uid 属主校验防护 |
| 冻结应用 | 不误伤 | STATE_TERMINATED 才推,不误伤 | /proc 仍在,不误伤 |
| SELinux | 所有调用方应用域策略 ❌ | 预期零新增(系统域通用策略大概率覆盖,真机 avc 验证) | usb 自加一条 proc getattr 规则 |
| 新增依赖 | — | ability_runtime(app_manager),Apache 2.0 | 零 |

**结论:双通道混合** —— A 满足"实时 DISCONNECT"核心诉求;B 保留为安全网,专治 A 的三个盲区(事件丢失、AMS 注册失败窗口、native SA 调用者)。两通道命中同一会话时经 `ReleaseDeviceClient` 幂等查表自然去重。
说明:SA 盲区不伤害监听诉求——native 调用者本就不进连接广播(`GetCallingInfo` 跳过),其死亡只影响 HDI 资源回收,由 B 覆盖。

## 4. 目标行为数据流程图

### 4.1 Open(双监听建立)

```
OpenDevice(busNum, devAddr)   ← 签名未变,调用方零改动
 ① 权限检查(IPC 上下文正常)
 ② 固化身份:tokenId/uid/pid(入口取,pid 为死亡检测锚)
 ③ usbHostManager_->OpenDevice → HDI 计数 +1(失败直接 return,无残留)
 ④ 锁内入表 deviceClientMap_[key]={tokenId, uid, pid}     【监听②】
 ⑤ StartDeviceClientSweepTimer(表非空时启动巡检)          【监听②:B 通道】
 ⑥ AddDeviceConnection → 锁外 oneway 广播 CONNECT          【监听①】
```

### 4.2 Close(双监听拆除)

```
Close(busNum, devAddr)
 ① 权限检查 + RemoveExclusiveClaimByDevice(调用者 token,IPC 上下文正常)
 ② usbHostManager_->Close → HDI 计数 -1
 ③ 成功:RemoveDeviceClientInfo(清表,表空停巡检)+ RemoveDeviceConnection(广播 DISCONNECT)
 ④ 失败:仅日志,条目保留(可重试;应用后续死亡由双通道兜底)
```

### 4.3 应用异常退出未 Close(双通道检测,同一收敛点)

```
应用 crash / kill -9 / 被查杀
 ├─ A 通道(毫秒级):AMS → OnProcessDied{pid,uid,bundleName}
 │    → 按 pid 匹配会话表 → ReleaseDeviceClient
 └─ B 通道(≤5s):巡检节拍 stat /proc/<pid>(ENOENT=死;属主≠uid=死;其他错误=存活保守)
      → 死键快照 → 锁外 ReleaseDeviceClient
 ReleaseDeviceClient(幂等,先到者清理,后到者 no-op):
   定向清独占 claim → HDI Close(计数-1,多应用安全)→ 清会话表 → 广播 DISCONNECT
 → 监听方实时(A)/最迟 5s(B)收到 OnDeviceDisconnected,与正常 Close 表现一致
```

### 4.4 设备拔出(双表同步)

```
usbd → UsbServiceSubscriber::DeviceEvent{ACT_DEVDOWN} → DelDevice
 ① RemoveAllDeviceConnections → 逐 uid 广播 DISCONNECT      【监听①】
 ② RemoveAllDeviceClients → 清该设备全部会话,表空停巡检   【监听②】
 ③ RemoveAllClaimByDevice(独占随设备清理)
 ④ Close → HDI CloseDevice;后续按 key 查表均幂等 no-op ✓
```

## 5. SELinux 权限说明

| 通道 | 是否需要 | 内容 |
|---|---|---|
| **B(巡检)** | 需要,**仅一条,usb 组件自管** | `stat("/proc/<pid>")` → `allow usb_service proc:dir { search getattr };`(usb_service.te;类型名以 avc 日志为准)。**失效安全**:EACCES → 按存活处理,功能静默失效而非事故 |
| **A(AMS)** | 预期零新增 | 两个方向 binder:①usb_service→AMS 注册 ②AMS→usb_service 回调。memmgr/资源调度等系统 SA 已在用同类观察者,通用策略大概率覆盖;若有 avc 拒绝,同样仅动 usb 自己的 .te(每方向一条)。另有 AccessToken 层注册校验,系统 SA 预期通过 |
| 外部调用方应用 | **零改动、零策略** | 应用是"被探测"客体,不执行任何新操作;IDL 未变 |

验证方法:Enforcing 真机 kill -9 持设备应用 → 看 hilog 清理日志;无则 `hilog | grep avc` 按拒绝日志补规则。可先 permissive 验逻辑再 enforcing 验策略。

## 6. 独占(ClaimInterfaceExclusive)与监听联动场景

| # | 场景 | 无清理链 | 有清理链 |
|---|---|---|---|
| 1 | 独占者 crash | exclusiveOwner 卡死为死者 token,他人**永远申请不到**;监听方却看到其已掉线——观测自相矛盾 | 独占释放与 DISCONNECT 广播**同刻发生**,观测一致 |
| 2 | A 死后 B 抢独占 | B 申请失败时长 = 检测延迟 | A 通道毫秒释放,B 基本无感(独占场景是实时性的最大收益点) |
| 3 | A 独占 X 死亡,B 用同设备 Y 接口 | 整表连坐会误伤 | 按 (bus,dev,token) 定向清,B 不受影响 |
| 4 | 接口独占+普通 claim 并存 | 可能连清普通持有 | 按 owner 匹配,normalClaimed 降级语义不动 |
| 5 | 既有事实 | — | 独占已有实时死亡保护(cb 参数既有设计,ClaimDeathRecipient);清理链中的 claim 清理是收敛/兜底,二者幂等共存 |
| 6 | 拔出 | — | RemoveAllClaimByDevice 与逐应用 DISCONNECT 同步(已有逻辑) |

## 7. 设计决策与待确认项

1. **双通道收敛**:A/B 均收敛到 `ReleaseDeviceClient`(幂等查表),天然去重,任一通道单独失效不致功能全灭
2. **巡检周期 5s**(`DEVICE_CLIENT_SWEEP_INTERVAL`):A 通道落地后可放宽(如 30s)降频;现阶段 A 未落地前保持 5s
3. **同 key 重复 open 配对语义**:HDI 计数按调用配对 vs 会话表按 key 唯一,open×2 + close×1 + 死亡兜底×1 恰平衡;open×2 + close×1 后死亡的极端场景计数残留 +1,记录为已知边界
4. **定时器生命周期 = 会话表生命周期**:首个 open 启动、表空即停(锁内复核防误停);不阻止 SA 按需卸载
5. **直通模式 SA 重启即进程级释放**;HAL 模式 usbd 独立进程持有计数,遗留本期不处理
6. **A 通道落地清单**:observer 类 + BUILD.gn 加 ability_runtime 依赖(Apache 2.0,过依赖评审)+ Init 注册重试(仿 COMMEVENT_REGISTER_RETRY_TIMES)+ 卸载反注册;清理/广播逻辑零改动

## 8. 边界与已知限制

- native/SA 调用者不产生连接广播(GetCallingInfo 跳过 native token),会话表照记、B 通道照常回收其 HDI 资源
- 检测依赖 usb_service 存活:SA 睡眠/卸载期间应用死亡,回收延后至 SA 被唤醒(设备插拔/新调用);唤醒后内存表丢失,直通模式无残留、HAL 模式计数遗留
- 设备策略禁用路径的服务端强制关设备不自动清会话表,保留至应用死亡/拔出;建议后续补挂
- 巡检线程 stat 在表锁内执行(系统调用微秒级);表规模极大可改锁内快照锁外判活
- 同源遗留(另行整改):串口 `CancelSerialRight` 在死亡回调中依赖调用者身份,右记录清理实际不生效

## 9. 测试要点

1. 正常 Open/Close:双表清理、广播各一次、表空巡检停
2. crash 唯一持有者:回收与 DISCONNECT 时延(A 通道毫秒级 / B 通道 ≤5s),HDI 计数归零
3. crash 双应用同设备:只清死者,第二应用读写不受影响
4. **独占场景**:独占者 crash → 独占释放与 DISCONNECT 同刻;窗口期内他应用抢独占的行为;独占+普通 claim 并存降级
5. PID 复用:crash 后快速起同 uid 新进程,旧会话不误判存活(B 通道属主校验)
6. 重复 open 同 key / OpenDevice 失败无残留
7. 拔出:先广播后清表,无重复 DISCONNECT
8. SELinux:Enforcing 真机全量用例无 avc 拒绝
9. mock 回归:`usb_unittest_test` / `service_unittest`
