# USB 设备会话管理与异常退出清理设计

> 仓库:usb_manager(SA 4201)
> 关联特性:应用连接 USB 设备状态监听 InnerAPI(RegisterConnectionListener)
> 状态:设计稿 v4 —— 采用纯 A 通道(AMS 应用状态观察者);B 巡检因功耗顾虑移除;锚点方案经准确再评估后列为演进选项

## 1. 背景与目标

应用经 `UsbSrvClient::OpenDevice` 打开设备后,HDI 层(usbd/libusb_adapter)**按 Open/Close 调用配对维护引用计数**(count++,减到 0 才真正 `libusb_close`)。当前生命周期依赖应用主动 Close,存在四类失配场景:

| 场景 | 后果 |
|---|---|
| 应用 crash / 被杀,未调 Close | HDI 计数永不递减,设备句柄/fd 卡开直到拔出 |
| 应用 crash 后独占接口未释放 | 独占卡死,其他应用无法申请 |
| 应用 crash 后 | 监听方看到陈旧 CONNECT,收不到 DISCONNECT |
| usb_service 按需卸载重启(HAL 模式) | 无主占用无人认领,新应用 open 失败 |

**核心诉求**:HAP 调用方不 Close 直接退出时,连接状态(含独占)**实时**回收。
**约束**:OpenDevice 调用方应用众多,IDL 加参(锚点方案)的 SELinux 策略评估与公共接口评审流程代价高。

## 2. 双监听机制

| 监听 | 数据结构 | 职责 |
|---|---|---|
| **监听①:连接广播** | `deviceConnectionMap_["bus-dev"] → apps: map<uid, bundleName>` | 对外广播"哪个应用连着哪个设备",喂给 `RegisterConnectionListener` 的系统应用 |
| **监听②:会话表 + AMS 观察者** | `deviceClientMap_[(busNum,devAddr,tokenId)] → {tokenId, uid, pid}` + `UsbAppDeathObserver` | 资源真值:谁开着设备;AMS 推送应用进程死亡事件后回收 |

两张表在三个终点事件(正常 Close / 应用死亡 / 设备拔出)同步迁移;清理统一收敛到 `ReleaseDeviceClient`。

## 3. 检测机制选型对比(修正版)

| 维度 | 锚点(binder 死亡通知) | **A:AMS 实时监听(现行)** | B:5s 异步巡检(已移除) |
|---|---|---|---|
| 原理 | 客户端传锚点桩,服务端挂 DeathRecipient,内核直推 | `RegisterApplicationStateObserver` → AMS 在应用进程死亡时主动推 `OnProcessDied{pid,uid,bundleName}` | 定时器周期 `stat("/proc/<pid>")` 判活 |
| 检测延迟 | 毫秒级(内核直推) | **毫秒级**(AMS 推送) | ≤5s |
| **HAP 覆盖** | **~100%(客户端 UsbSrvClient 系统侧自动注入,应用零改造)** | **~100%(系统广播,零配合)** | ~100% |
| native SA 覆盖 | ✓(顺带) | ✗ | ✓ |
| 事件丢失/注册失败 | 内核保证,免疫 | ✗ AMS 重启窗口/oneway 拥塞会漏,无兜底 | ✓ 自包含 |
| SA 卸载窗口内死亡 | 盲 | 盲 | 盲(三案等价) |
| SELinux | **usb 侧一条规则**:`allow {hap域属性} usb_service:binder transfer`(评估/评审流程) | **预期零新增**(系统域通用策略大概率覆盖,真机 avc 验证) | usb 侧一条 /proc getattr 规则(随 B 移除已不需要) |
| IDL 变更 | **要**(公共接口评审) | 零 | 零 |
| 新增依赖 | 零 | ability_runtime:app_manager | 零 |
| 功耗 | 零 | 零 | 每会话周期 stat(已移除的原因) |

**决策**:纯 A 落地。理由:HAP 诉求下覆盖与实时性双达标、调用方与 IDL 零变更、预期零 SELinux;B 因"仅表非空时段运行"的周期唤醒仍引入额外功耗顾虑而移除(接受 SA 死亡无检测的取舍);锚点因 IDL/策略评审流程未复活,列为后续 SA 覆盖的演进选项(见 §4)。

**修正说明**:早期评估曾认为"锚点方案存量应用不配合则监听不到"——该说法仅适用于应用自行传对象的形态;按本仓库既有模式(锚点由 UsbSrvClient 系统侧自动注入,同 serial/accessory),HAP 覆盖为自动 ~100%,应用零改造。锚点被否的真实代价是 **IDL 公共接口变更评审 + binder transfer 策略评估流程**,而非覆盖度。

## 4. 异步锚点可行性评估(演进储备)

若后续需要覆盖 native SA 调用方死亡,锚点方案技术上完全可行,评估结论如下:

| 评估项 | 结论 |
|---|---|
| 不影响 OpenDevice 原有功能 | ✓ 可行——锚点注册与 open 主流程(权限→HDI 打开→记账→广播)完全解耦,**失败仅降级**(该会话无死亡清理),open 照常成功 |
| "异步"的必要性 | **无性能收益**:`AddDeathRecipient` 是本地记账(µs 级,无 IPC);异步反而引入**注册窗口竞态**(open 成功→异步注册完成前 crash→漏清理,需 AMS/巡检兜底)。保障"不影响基本功能"靠的是失败仅降级原则,同步实现更简 |
| SA 是否顺带覆盖 | SA 走同一 UsbSrvClient 会自动传锚点 → SA 死亡被清理是额外正确行为;若坚持不监测,服务端按 GetCallingTokenID 识别 native token 主动忽略 |
| 复活成本 | 回滚 IDL 三参数 + 客户端注入 + usb 侧一条 transfer 规则(评审)+ 复用现有 ReleaseDeviceClient 清理链(零改动) |

**演进方向**:长期可形成"锚点管 SA(内核直推)+ AMS 管 HAP(系统广播)"双通道,均收敛到 ReleaseDeviceClient 幂等清理点,天然去重。

## 5. 目标行为数据流程图

### 5.1 Open(双监听建立)

```
应用调 UsbSrvClient::OpenDevice(device, pipe)   ← 签名未变,调用方零改动
 UsbService::OpenDevice(busNum, devAddr)
  ① 权限检查(IPC 上下文正常)
  ② 固化身份:tokenId/uid/pid(入口取,pid 为死亡匹配锚)
  ③ usbHostManager_->OpenDevice → HDI 引用计数 +1(失败直接 return,无残留)
  ④ 锁内入表 deviceClientMap_[key]={tokenId, uid, pid}          【监听②】
  ⑤ EnsureAppDeathObserver(幂等;首个 open 注册 AMS 观察者,
     失败下次 open 自愈重试——应用在调 open 说明 AMS 必然就绪)  【监听②:A 通道】
  ⑥ AddDeviceConnection → 锁外 oneway 广播 CONNECT               【监听①】
```

### 5.2 Close(双监听拆除)

```
Close(busNum, devAddr)
 ① 权限检查 + RemoveExclusiveClaimByDevice(调用者 token,IPC 上下文正常)
 ② usbHostManager_->Close → HDI 引用计数 -1
 ③ 成功:RemoveDeviceClientInfo(清表)+ RemoveDeviceConnection(广播 DISCONNECT)
 ④ 失败:仅日志,条目保留(可重试;应用后续死亡由 A 通道兜底)
```

### 5.3 应用异常退出未 Close(AMS 实时检测)

```
HAP 进程 crash / kill -9 / 被查杀
 └ AMS → UsbAppDeathObserver::OnProcessDied{pid, uid, bundleName}(毫秒级,binder 回调线程)
    ├ GetGlobalInstance();null → return
    └ ReleaseDeviceClientByPid(pid, uid)
       ① 锁内按 (pid && uid) 快照命中会话(双重匹配防 PID 复用误判)
       ② 锁外逐个 ReleaseDeviceClient ← 与 Close 失败兜底共用同一收敛点
          定向清独占 claim → HDI Close(计数-1,多应用安全)
          → 清会话表 → ReleaseDeviceConnectionByUid → 广播 DISCONNECT
 → 监听方毫秒级收到 OnDeviceDisconnected,与正常 Close 表现一致
```

### 5.4 设备拔出(双表同步)

```
usbd → UsbServiceSubscriber::DeviceEvent{ACT_DEVDOWN} → DelDevice
 ① RemoveAllDeviceConnections → 逐 uid 广播 DISCONNECT      【监听①】
 ② RemoveAllDeviceClients → 清该设备全部会话条目           【监听②】
 ③ RemoveAllClaimByDevice(独占随设备清理)
 ④ Close → HDI CloseDevice;后续按 key 查表均幂等 no-op ✓
 SA 卸载:UnLoadSelf 任务先 UnregisterAppDeathObserver 再 UnloadSystemAbility
```

## 6. SELinux 权限说明

| 通道 | 结论 |
|---|---|
| **A(现行)** | **预期零新增**:两个方向 binder(usb_service→AMS 注册、AMS→usb_service 回调)大概率被系统域通用策略覆盖(memmgr/资源调度等系统 SA 已在用同类观察者)。若有 avc 拒绝,仅动 usb 自己的 .te,每方向一条。另有 AccessToken 层注册校验,系统 SA 预期通过 |
| B(已移除) | 原 `allow usb_service proc:dir { search getattr }` 随方案移除**不再需要** |
| 锚点(若复活) | usb 侧一条 `allow {hap域属性} usb_service:binder transfer`(带属性覆盖全部 HAP,非逐应用);应用侧**无"加 SELinux 权限"概念**——SELinux 是系统镜像策略,应用零感知;上层仅系统镜像内 UsbSrvClient 注入,应用零改造 |
| 外部调用方应用 | **零改动、零策略**(现行方案) |

验证方法:Enforcing 真机 kill -9 持设备应用 → 看 `OnProcessDied`/`ReleaseDeviceClientByPid` 清理日志;无则 `hilog | grep avc` 按拒绝日志补规则;可先 permissive 验逻辑。

## 7. 独占(ClaimInterfaceExclusive)与监听联动场景

| # | 场景 | 无清理链 | 有清理链 |
|---|---|---|---|
| 1 | 独占者 crash | exclusiveOwner 卡死为死者 token,他人**永远申请不到**;监听方却看到其已掉线——观测自相矛盾 | 独占释放与 DISCONNECT 广播**同刻发生**,观测一致 |
| 2 | A 死后 B 抢独占 | B 申请失败时长 = 检测延迟 | A 通道毫秒释放,B 基本无感(独占场景是实时性的最大收益点) |
| 3 | A 独占 X 死亡,B 用同设备 Y 接口 | 整表连坐会误伤 | 按 (bus,dev,token) 定向清,B 不受影响 |
| 4 | 接口独占+普通 claim 并存 | 可能连清普通持有 | 按 owner 匹配,normalClaimed 降级语义不动 |
| 5 | 既有事实 | — | 独占已有实时死亡保护(cb 参数既有设计,ClaimDeathRecipient);清理链中的 claim 清理是收敛/兜底,二者幂等共存 |
| 6 | 拔出 | — | RemoveAllClaimByDevice 与逐应用 DISCONNECT 同步(已有逻辑) |

## 8. 设计决策与待确认项

1. **纯 A 决策**:HAP 诉求全覆盖;B 移除(功耗);SA 死亡无检测为**已接受取舍**(native 调用者本就不进连接广播,监听无感知差异,仅 HDI 资源回收无兜底)
2. **A 通道失效模式**:AMS 事件丢失/注册失败窗口 → 该会话无死亡清理,残留至拔出——无兜底通道,已知取舍(若不可接受,演进为 A+锚点双通道,见 §4)
3. **注册自愈**:首个 OpenDevice 成功时注册(应用在调 open ⇒ AMS 必然就绪),失败下次 open 重试;幂等防重;SA 卸载前反注册
4. **同 key 重复 open 配对语义**:HDI 计数按调用配对 vs 会话表按 key 唯一,open×2 + close×1 + 死亡兜底×1 恰平衡;极端场景计数残留 +1,记录为已知边界
5. **直通模式 SA 重启即进程级释放**;HAL 模式 usbd 独立进程持有计数,遗留本期不处理
6. **AMS API 版本适配点**(集中在 `usb_app_death_observer.h/.cpp` + 2 处注册调用):①观察者头文件路径/名称 ②基类形态(是否需同时继承 Stub)③`AppStateData` vs `ProcessData` ④`DelayedSingleton<AppManager>` 单例形式 ⑤OnProcessDied 在新版本可能被 OnProcessStateChanged 取代——首次编译报错必然只落在这两个新文件,按目标 SDK 校准即可
7. 设备策略禁用路径的服务端强制关设备不自动清会话表,保留至应用死亡/拔出;建议后续补挂

## 9. 边界与已知限制

- native/SA 调用者不产生连接广播(GetCallingInfo 跳过 native token),会话表照记;其死亡**无检测**(纯 A 盲区,已接受)
- AMS 观察者依赖链:AMS 重启窗口内的应用死亡事件丢失 → 会话残留至拔出(无兜底)
- 检测依赖 usb_service 存活:SA 卸载期间应用死亡,无人接收;重启后内存表丢失,直通模式无残留、HAL 模式计数遗留
- OnProcessDied 是全局广播:系统内所有应用死亡都会回调,查表 miss 即返回,开销可忽略
- 同源遗留(另行整改):串口 `CancelSerialRight` 在死亡回调中依赖调用者身份,右记录清理实际不生效

## 10. 测试要点

1. 正常 Open/Close:双表清理、广播各一次;首个 open 后 hilog 出现 `EnsureAppDeathObserver register success`
2. crash(唯一持有者):**毫秒级**出现 `OnProcessDied → ReleaseDeviceClientByPid`;监听方收到 DISCONNECT 与 Close 表现一致;HDI 计数归零
3. crash(双应用同设备):只清死者,第二应用读写不受影响
4. **独占场景**:独占者 crash → 独占释放与 DISCONNECT 同刻;窗口期抢独占行为;独占+普通 claim 并存降级
5. PID 复用:同 uid 新进程复用旧 pid → (pid&&uid) 双重匹配不误判(注意:同 uid 重建进程场景观察行为)
6. 重复 open 同 key / OpenDevice 失败无残留
7. 拔出:先广播后清表,无重复 DISCONNECT
8. SA 卸载:UnLoadSelf 前观察者反注册日志
9. SELinux:Enforcing 真机全量用例无 avc 拒绝
10. mock 回归:`usb_unittest_test` / `service_unittest`
