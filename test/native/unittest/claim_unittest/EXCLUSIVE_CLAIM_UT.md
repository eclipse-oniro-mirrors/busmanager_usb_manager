# USB Manager Exclusive Claim UT 测试说明

> **仓库归属**：本套件位于 `busmanager_usb_manager_9`（最新工作仓库），目录 `test/native/unittest/claim_unittest/`。历史仓库 `busmanager_usb_manager_6` 中仅保留部分文件（exclusive/release/common/probe），**全部 8 套件以本仓库为准**。

## 1. 测试目的

针对 USB Manager 新增的接口独占（Exclusive Claim）机制进行设备级功能验证。该机制在完全兼容现有 `ClaimInterface`（普通 claim）行为的基础上，新增 `ClaimInterfaceExclusive`（独占 claim）接口，实现接口占用状态跟踪、传输隔离与冲突异步通知。

UT 覆盖以下核心规则（对应需求文档 `USB_MANAGER_CLAIM_FORCE_REQUIREMENT.md` 设计原则 1/2/3/4/5/6/7/8/10）：

1. **Exclusive claim 互斥**：不同应用的独占 claim 互斥，后来者返回 `UEC_INTERFACE_BUSY`
2. **Normal claim 不被阻断**：无论接口是否被独占，普通 claim 始终透传 HDI 成功
3. **Normal claim 阻断 exclusive claim**：有普通 claim 时独占 claim 返回 `UEC_INTERFACE_BUSY`
4. **同应用可重复独占 claim**：独占者重复 claim 成功（幂等，透传 HDI）
5. **传输条件校验**：仅当接口仅有独占 claim（无 normal claim）且调用方非独占者时，传输返回 `UEC_SERVICE_IO_EXCEPTION`；有 normal claim 时不校验
6. **异步通知触发**：其他应用 normal claim 已被独占的接口时，服务端触发 `CLAIM_CONFLICT_NOTIFY` 通知路径（fire-and-forget，不影响 normal claim 结果）
7. **ReleaseInterface 条件调用**（按更新后的语义矩阵）：

   | 接口状态 | 调用方 | 返回 | HDI Release |
   |---|---|---|---|
   | 无 claim | 任意 | 成功（透传 HDI 结果） | 调用 |
   | 仅 normal | 任意 | 成功 | 调用 |
   | 仅 exclusive(A) | A 独占者 | 成功 | 调用 |
   | 仅 exclusive(A) | B 他人 | `UEC_INTERFACE_INVALID_OPERATION` | 不调用 |
   | excl(A)+normal | A 独占者 | 成功 | 不调用（normal 保留） |
   | excl(A)+normal | B 他人 | 成功 | 不调用（normalClaimed 清除） |

8. **Close 清理调用方独占记录**：不影响其他应用的占用状态
9. **设备拔出清理**：拔出时清除该设备所有独占记录和 normal claim 标志，重插后接口恢复空闲
10. **权限校验**：无设备访问权限的调用被 `UEC_SERVICE_PERMISSION_DENIED` 拒绝
11. **回归**：普通 claim / 空闲接口传输等现有行为不变

## 2. 验证方案

### 2.1 总体架构

真机设备级测试（rk3568 + OpenHarmony），测试进程通过 `UsbSrvClient` IPC 调用真实运行的 `usb_service` 系统服务（SA 4201），HDI 层走真实 libusb 直通实现。测试设备为接入的任意非 hub USB 设备（如鼠标），取其配置 0 的接口 0 和端点 0 作为被测对象。

```
测试进程 (gtest 二进制)
  ├── UsbSrvClient ──IPC──> usb_service (SA 4201) ──HDI──> libusb 直通 ──> 真实 USB 设备
  └── 身份切换: SetSelfTokenID + ReloadNativeTokenInfo
```

### 2.2 仿冒不同应用的机制

测试进程内分配 4 个 native token 模拟 4 个不同应用：

| token | 权限 | 角色 |
|---|---|---|
| tokenA（`usb_claim_test_app_a`） | native（系统侧放行） | 应用 A：独占者 |
| tokenB（`usb_claim_test_app_b`） | native | 应用 B：普通/竞争者 |
| tokenC（`usb_claim_test_app_c`） | native | 应用 C：第三方 |
| tokenSys（`usb_claim_test_sys`，system_basic + `MANAGE_USB_CONFIG`） | native + 管理权限 | 系统管理调用（热插拔模拟） |

`SwitchToApp()` 切换 self token 后直调客户端接口，服务侧 `IPCSkeleton::GetCallingTokenID()` 取到不同 token，claim 状态机按 token 区分应用身份。native token 同时满足 `IsSystemAppOrSa()` 判定，绕过设备授权弹窗，保证权限检查通过而 claim 逻辑正常生效。

### 2.3 设备拔插模拟

通过 `UsbSrvClient::ManageGlobalInterface(true/false)` 实现（tokenSys 调用）：

- **拔出**：`ManageGlobalInterface(true)` → HDI deauthorize 全部 root hub → 下游设备物理断开 → libusb 热插拔回调上报 DEVICE_LEFT → 服务侧 `DelDevice` → `RemoveAllClaimByDevice` 清理 claim 记录
- **插入**：`ManageGlobalInterface(false)` → root hub 重新 authorize → 设备重枚举（devAddr 可能变化）→ 上报 DEVICE_ARRIVED → `AddDevice`

设备重枚举后 devAddr 会变，用例中调用 `RefreshTestDevice()` 重新选取测试设备。

### 2.4 用例隔离

- 每个用例 `SetUp`/`TearDown` 调用 `CleanClaimState()`：以 A/B/C 三个身份各调一次 `ReleaseInterface`（normal claim 标志不记录占用者，需全身份清理），再 `UsbAttachKernelDriver` 恢复驱动绑定
- `CleanClaimState()` 中先补一次 `OpenDevice`：HDI 层设备句柄引用计数，用于恢复前序用例 `Close` 掉的句柄
- claim 全部使用 `force=true`：市售设备接口通常被内核驱动（usbhid 等）占用，非 force claim 会失败

### 2.5 传输结果断言口径

真实设备的端点类型不可控（如鼠标仅有中断 IN 端点），放行的传输断言为 `ret != UEC_SERVICE_IO_EXCEPTION`：只要不是独占阻断错误码，即证明请求通过了 claim 校验并到达 HDI 层（驱动层超时/类型不符等失败不影响断言语义）。被阻断的传输严格断言 `== UEC_SERVICE_IO_EXCEPTION` 且 HDI 零调用。

### 2.6 构建与部署

| 项 | 内容 |
|---|---|
| 测试目录 | `test/native/unittest/claim_unittest/`（挂入 `test/native/BUILD.gn` 的 `usb_unittest_test` 分组，仅 `usb_manager_feature_host` 下编译） |
| 构建目标 | `test_claim_exclusive` / `test_claim_notify` / `test_claim_release` / `test_claim_transfer` / `test_claim_hotplug` / `test_claim_force` / `test_claim_multi_device` / `test_claim_death` + 辅助可执行 `claim_notify_probe` |
| 远程编译 | SSH 至 `/srv/workspace/openharmony_master_default_20260826175846_huawei_3189f875a/code`，`gn gen out/rk3568 && ninja tests/unittest/usb_manager/usb_unittest_test/test_claim_*` |
| 设备部署 | hdc 推送 5 个测试二进制至 `/data/`，`libusbservice.z.so`（含新实现）推送至 `/system/lib/` 后重启 usb_service |
| 执行 | `hdc shell "/data/test_claim_<suite> --gtest_brief=1"`（root 权限） |

## 3. 用例实现

### 3.1 文件清单

```
test/native/unittest/claim_unittest/
├── BUILD.gn                              # 8 个 ohos_unittest 目标 + 辅助可执行 claim_notify_probe
├── include/
│   ├── usb_claim_test_common.h           # 公共 fixture 基类 UsbClaimTestBase
│   ├── usb_claim_exclusive_test.h
│   ├── usb_claim_notify_test.h
│   ├── usb_claim_release_test.h
│   ├── usb_claim_transfer_test.h
│   ├── usb_claim_hotplug_test.h
│   ├── usb_claim_force_test.h
│   ├── usb_claim_multi_device_test.h
│   └── usb_claim_death_test.h
└── src/
    ├── usb_claim_test_common.cpp         # token 分配/身份切换/设备选取/清理/拔插模拟
    ├── usb_claim_exclusive_test.cpp      # 12 例 — claim 互斥规则
    ├── usb_claim_notify_test.cpp         # 7 例（hap 身份通知接收 + 到达断言）
    ├── usb_claim_release_test.cpp        # 14 例（Release 条件调用 + Close 清理）
    ├── usb_claim_transfer_test.cpp       # 15 例（方向自适应 + Submit/Control）
    ├── usb_claim_hotplug_test.cpp        # 4 例（ManageGlobalInterface 拔插）
    ├── usb_claim_force_test.cpp          # 2 例（HID 设备 force 透传）
    ├── usb_claim_multi_device_test.cpp   # 7 例（双设备 + UsbSubmitTransfer 成功）
    ├── usb_claim_death_test.cpp          # 2 例（死亡自动释放三段式）
    └── claim_notify_probe.cpp            # 辅助可执行：hold/normal 模式
```

公共设施（`usb_claim_test_common.cpp`）：

- `AllocNativeToken/AllocManageNativeToken`：native token 分配（后者带 `MANAGE_USB_CONFIG`）
- `SwitchToApp(app)`：切换 4 个身份之一
- `PickTestDevice/RefreshTestDevice`：从 `GetDevices` 选取首个非 hub、有配置/接口/端点的设备
- `WaitForDevicePresent(present, timeout)`：轮询服务设备列表等待拔插生效
- `SimulateUnplug/SimulatePlug`：`ManageGlobalInterface` 驱动的拔插模拟（公共能力，hotplug/force 套件复用）
- `IsKernelDriverBound()`：sysfs 判断接口驱动绑定（按 busnum/devnum 匹配 sysfs 目录，因 HDI 设备名 bus-devAddr 与 sysfs 目录名 bus-port 不一致）
- `CleanClaimState()`：全身份 release + 重挂内核驱动

### 3.2 test_claim_exclusive（12 例）— claim 互斥规则

| 用例 | 操作序列 | 断言 |
|---|---|---|
| ClaimExclusive001 | 空闲接口，A 独占 claim | `UEC_OK` |
| ClaimExclusive002 | A 独占后 B 独占同接口 | B 得 `UEC_INTERFACE_BUSY` |
| ClaimExclusive003 | A 重复独占 claim | 两次均 `UEC_OK`（幂等） |
| ClaimExclusive004 | A 独占后 B normal claim | `UEC_OK`（normal 不被阻断） |
| ClaimExclusive005 | A 独占后 A normal claim | `UEC_OK` |
| ClaimExclusive006 | A normal 后 B 独占 | `UEC_INTERFACE_BUSY`（normal 阻断 exclusive） |
| ClaimExclusive007 | A、B 均 normal 后 C 独占 | `UEC_INTERFACE_BUSY` |
| ClaimExclusive008 | A normal → A release → B 独占 | `UEC_OK`（normal 已清除） |
| ClaimExclusive009 | 回归：A/B 交替 normal claim ×4 | 全部 `UEC_OK`（现有行为不变） |
| ClaimExclusive011 | 独占 cb=nullptr | `UEC_OK`（降级：无通知无死亡监听） |
| ClaimExclusive013 | 无权限 hap token 独占 claim | `UEC_SERVICE_PERMISSION_DENIED` |
| ClaimExclusive014 | 无效 bus/dev 独占 claim | `UEC_SERVICE_PERMISSION_DENIED`（设备不存在，权限检查失败） |

### 3.3 test_claim_notify（7 例）— 异步通知（hap 普通应用身份 + 通知到达断言）

**设计**：通知接收方采用 **hap token 模拟普通应用**——设备权限经系统管理 API 授权（SYS token 调 `AddAccessRight`，不拉起授权弹窗），回调以普通应用身份注册；冲突方为独立 probe 进程（`normal` 模式：仅 normal claim）。**通知到达自动化断言**：回调触发次数、busNum/devAddr/interfaceId 参数全匹配。

**执行前置条件**：probe 部署于 `/data/claim_notify_probe`；SELinux 需 Permissive（`setenforce 0`，Enforcing 下服务端向测试进程域发通知被 binder call 策略拒绝，见遗留问题 #1）。

| 用例 | 操作序列 | 断言 |
|---|---|---|
| ClaimNotify001 | hapA 独占(带回调) → probe 进程 normal claim | hapA 回调收到 1 次通知，bus/dev/ifid 全匹配 |
| ClaimNotify002 | hapA 独占(带回调) → hapA 自己 normal claim | claim `UEC_OK`；无通知（同应用不通知自身） |
| ClaimNotify003 | 无独占，B normal claim | claim `UEC_OK`；无通知 |
| ClaimNotify004 | hapA 独占 → release → B normal | 全部 `UEC_OK`；无通知 |
| ClaimNotify005 | hapA 独占 → probe normal → probe 再 normal | 两次通知均到达（计数=2） |
| ClaimNotify006 | hapA 独占(回调1) → 重复独占(回调2) → probe normal | 更新后的回调收到通知 |
| ClaimNotify007 | hapA 独占(带回调) → probe normal | probe claim 同步返回 `UEC_OK`（fire-and-forget 不影响结果）；通知随后到达 |

### 3.4 test_claim_release（14 例）— Release 条件调用 + Close

| 用例 | 操作序列 | 断言 |
|---|---|---|
| ClaimRelease001 | A 独占 → B release | B 得 `UEC_INTERFACE_INVALID_OPERATION`（他人独占且无 normal） |
| ClaimRelease002 | A 独占 → A release | `UEC_OK`（独占者，无 normal → 调 HDI） |
| ClaimRelease003 | A 独占 + B normal → A release → B 独占 | A release `UEC_OK`；B 独占仍 `UEC_INTERFACE_BUSY`（normal 保留） |
| ClaimRelease004 | A 独占 + B normal → B release → B BulkTransfer | release `UEC_OK`；传输 `UEC_SERVICE_IO_EXCEPTION`（normalClaimed 已清，回到仅独占态） |
| ClaimRelease005 | A normal → A release | `UEC_OK`（仅 normal → 调 HDI） |
| ClaimRelease006 | 空闲接口 A release | 非 `UEC_OK` 且非阻断/权限错误（透传 HDI，驱动层释放未 claim 的接口失败） |
| ClaimRelease007 | A 独占 + B normal → B release → A release → C 独占 | 两步 release 均 `UEC_OK`；C 独占 `UEC_OK`（链式全清） |
| ClaimRelease008 | A 独占 + B normal → A release → B release → C 独占 | 同上（先独占后 normal 的清理链） |
| ClaimRelease009 | A 独占 → A release → B 独占 | B `UEC_OK`（独占记录已清） |
| ClaimRelease010 | A 独占 → A Close → B（重开设备后）独占 | Close 成功；B 独占 `UEC_OK`（Close 清调用方独占记录） |
| ClaimRelease011 | A 独占 → B Close → B（重开设备后）独占 | Close 成功；B 独占仍 `UEC_INTERFACE_BUSY`（Close 不清他人记录） |
| ClaimRelease012 | A 独占 + B normal → A Close → B 独占 | B 独占 `UEC_INTERFACE_BUSY`（Close 后 normal 保留） |
| ClaimRelease013 | A 独占 → A Close → A 重开设备独占 → B 独占 | A 重独占 `UEC_OK`（Close 清理后独占者自身可重新独占）；B 独占 `UEC_INTERFACE_BUSY`（新的独占生效） |
| ClaimRelease014 | A 独占 → B BulkTransfer 被阻断 → A Close → B 重开设备 BulkTransfer | 阻断期 `UEC_SERVICE_IO_EXCEPTION`；Close 后非 IO_EXCEPTION（独占记录清除，传输恢复放行） |

### 3.5 test_claim_transfer（15 例）— 传输校验（方向自适应 + Submit/Control）

**方向自适应机制**：`SetUpTestCase` 扫描被测接口的端点描述符，识别 IN/OUT 端点。每个 bulk 用例内部按方向条件执行——有 IN 端点才执行读变体（`BulkTransfer` IN → `BulkTransferReadwithLength`），有 OUT 端点才执行写变体（`BulkTransferWrite`）；描述符中缺失的方向直接记成功跳过（`SUCCEED()`）。当前测试设备（HP 鼠标）仅有中断 IN 端点，实际执行读变体。

**UsbSubmitTransfer**：使用接口实际暴露的端点（IN 优先，OUT 兜底），transfer type 取自端点描述符（bulk=2/interrupt=3），ashmem 传递缓冲区。**ControlTransfer**：标准 USB 协议请求 GET_DESCRIPTOR(device)（bmRequestType=0x80, bRequest=0x06, wValue=0x0100, length=18）走端点 0（不隶属任何接口），不受独占校验影响，断言严格 `UEC_OK`（标准请求真实设备必成功）。

| 用例 | 操作序列 | 断言 |
|---|---|---|
| ClaimTransfer001 | A 独占 → A BulkTransfer（读/写变体） | 非 `UEC_SERVICE_IO_EXCEPTION`（独占者放行） |
| ClaimTransfer002 | A 独占 → B BulkTransfer（读/写变体） | `UEC_SERVICE_IO_EXCEPTION`（非独占者阻断） |
| ClaimTransfer003 | A 独占 → B normal → B BulkTransfer | 非 `UEC_SERVICE_IO_EXCEPTION`（有 normal 不校验） |
| ClaimTransfer004 | A 独占 → B normal → C BulkTransfer | 非 `UEC_SERVICE_IO_EXCEPTION`（对任意应用放行） |
| ClaimTransfer005 | A normal（无独占） → B BulkTransfer | 非 `UEC_SERVICE_IO_EXCEPTION`（无独占不校验） |
| ClaimTransfer006 | 空闲接口 B BulkTransfer | 非 `UEC_SERVICE_IO_EXCEPTION`（回归：现有行为不变） |
| ClaimTransfer007 | A 独占 → A release → B BulkTransfer | 非 `UEC_SERVICE_IO_EXCEPTION`（独占清除后恢复） |
| ClaimTransfer008 | A 独占 → B normal → B release → B BulkTransfer | `UEC_SERVICE_IO_EXCEPTION`（normal 清除后回到仅独占态） |
| ClaimTransfer009 | A 独占 → B 连续两次 BulkTransfer | 两次均 `UEC_SERVICE_IO_EXCEPTION`（阻断决策稳定） |
| ClaimTransfer010 | A 独占 → C BulkTransfer | `UEC_SERVICE_IO_EXCEPTION`（第三方同样阻断） |
| ClaimTransfer011 | A 独占 → A UsbSubmitTransfer | 非 `UEC_SERVICE_IO_EXCEPTION`（独占者放行） |
| ClaimTransfer012 | A 独占 → B UsbSubmitTransfer | `UEC_SERVICE_IO_EXCEPTION`（非独占者阻断） |
| ClaimTransfer013 | A 独占 → B normal → B UsbSubmitTransfer | 非 `UEC_SERVICE_IO_EXCEPTION`（有 normal 不校验） |
| ClaimTransfer014 | A 独占 → B ControlTransfer（GET_DESCRIPTOR 设备描述符） | `UEC_OK`（标准请求成功；端点 0 不隶属接口不受独占校验） |
| ClaimTransfer015 | A 独占 → B UsbControlTransfer（v1.2，GET_DESCRIPTOR） | `UEC_OK`（标准请求成功） |

### 3.6 test_claim_hotplug（4 例）— 设备拔插

| 用例 | 操作序列 | 断言 |
|---|---|---|
| ClaimHotplug001 | A 独占 + B normal → 拔出 → 插入 → A 重开设备独占 | 拔插均生效；A 独占 `UEC_OK`（claim 记录全清） |
| ClaimHotplug002 | A 独占 → 拔出 → B 独占（旧 bus/dev） | B 非 `UEC_OK`（设备不存在，claim 失败） |
| ClaimHotplug003 | A 独占(带回调) → 拔出 → 插入 → B 重开设备 normal claim | B `UEC_OK`（旧 claim 状态不影响新 claim） |
| ClaimHotplug004 | A 独占 → 拔出 → 插入 → B 重开设备独占 | B `UEC_OK`（陈旧独占记录已被清理） |

### 3.7 test_claim_force（2 例）— force 参数透传 + HDI 失败不记状态

**前置条件**：被测接口必须是鼠标/键盘类 HID 设备（接口 class=3，usbhid 内核驱动绑定）。`PickHidInterface()` 从测试设备选取 HID 接口；**每个用例 SetUp 执行一次完整拔插**（`ManageGlobalInterface` deauthorize/re-authorize root hub），确保 usbhid 驱动重新绑定接口——此时 force=0 的 claim 必被 HDI 层拒绝（libusb `LIBUSB_ERROR_BUSY`）。

| 用例 | 操作序列 | 断言 |
|---|---|---|
| ClaimForce001 | 拔插一次 → A force=0 独占 → A force=1 独占 → B force=0 独占 | force=0 失败（驱动占用，HDI 层拒绝）；force=1 成功（force 参数透传到 HDI）；B 仍 `UEC_INTERFACE_BUSY`（服务层阻断先于 HDI 调用） |
| ClaimForce002 | 拔插一次 → A force=0 独占（失败） → B force=1 独占 | A 失败；B `UEC_OK`（HDI 失败不记录服务层状态） |

### 3.8 test_claim_multi_device（7 例）— 双设备隔离 + 真实读写

**执行前置条件（重要）**：需接入**两个及以上**非 hub USB 设备，且其中一个设备具备 bulk IN + bulk OUT 端点的接口（USB 串口适配器如 PL2303/CP210x 即可，当前环境为 PL2303 串口线 + HP 鼠标）。`SetUpTestCase` 中 `PickSerialDevice()`（选 bulk 双向设备）+ `PickSecondDevice()`（选另一设备）任一失败即 `GTEST_SKIP()` 跳过整个套件（单设备环境安全运行，6 例全部 SKIP，已实测验证）。文件头注释中亦有说明。

| 用例 | 操作序列 | 断言 |
|---|---|---|
| ClaimMultiDev001 | A normal claim 串口接口 → bulk OUT 写 | 写 `UEC_OK`（真实写成功，严格断言） |
| ClaimMultiDev002 | A normal claim 串口接口 → bulk IN 读 | 非 `UEC_OK` 且非 `UEC_SERVICE_IO_EXCEPTION`（请求到达设备；串口 IN 端点无回传数据，驱动层返回超时错误） |
| ClaimMultiDev003 | A normal claim → 先写后读完整回环 | 写 `UEC_OK`；读非 `UEC_OK` 且非 IO_EXCEPTION（双向通路打通） |
| ClaimMultiDev004 | A normal claim 串口接口 → UsbSubmitTransfer（bulk OUT 异步提交） | `UEC_OK`（异步提交被 HDI 层接受，严格断言） |
| ClaimMultiDev005 | A 独占串口接口 → B bulk 写/读 | 两方向均 `UEC_SERVICE_IO_EXCEPTION`（写方向同样受独占校验） |
| ClaimMultiDev006 | A 独占设备1接口 + B 独占设备2接口 → B 在设备2 读写 | 两 claim 均 `UEC_OK`；写 `UEC_OK`；读非 `UEC_OK` 且非 IO_EXCEPTION（设备间独占状态隔离） |
| ClaimMultiDev007 | A 独占设备1 → B 在设备2 normal claim/release/独占/写 | 设备2 全链路正常；写 `UEC_OK`（设备1 的独占不影响设备2） |

**断言口径说明**：bulk OUT 写方向断言严格 `UEC_OK`（串口设备真实写入成功）；bulk IN 读方向因串口适配器无主动上行数据，驱动层返回超时错误，断言为"非 `UEC_OK` 且非 `UEC_SERVICE_IO_EXCEPTION`"（证明请求通过独占校验并到达设备，而非被服务层阻断）。

### 3.9 test_claim_death（2 例）— 死亡自动释放（独立文件）

**执行前置条件（重要）**：辅助可执行 `claim_notify_probe`（本目录 `src/claim_notify_probe.cpp` 构建，随套件一起编译）须部署到 `/data/claim_notify_probe`。**该套件与其他套件不在同一文件/进程**：probe 作为 stage1 子进程以 `hold` 模式持独占 claim（不自我冲突 claim、无清理路径，仅等被杀），独占记录只能靠服务端 DeathRecipient 清理；随后本测试进程（stage2，全新进程）对同一接口重新独占 claim，成功即证明死亡自动释放生效。SELinux 需为 Permissive（`setenforce 0`，见遗留问题 #1）。

| 用例 | 操作序列 | 断言 |
|---|---|---|
| ClaimDeath001 | fork probe(hold) 独占 claim → stage1 存活期间本进程独占 claim 同接口 | `UEC_INTERFACE_BUSY`（独占互斥先验证生效） |
| （接上） | SIGKILL stage1（无清理）→ 本进程再独占 claim | `UEC_OK`（DeathRecipient 清理了死亡进程的独占记录） |
| ClaimDeath002 | fork probe(hold) 独占 claim → stage1 存活期间 B 应用 normal claim | `UEC_OK`（normal claim 不被独占阻断） |
| （接上） | SIGKILL stage1 → B release 自身 normal claim → B 独占 claim | release `UEC_OK`；独占 `UEC_OK`（无残留独占记录；先清 normal 再独占符合"normal 阻断 exclusive"语义） |

**probe hold 模式说明**：`claim_notify_probe <bus> <dev> <iface> hold`——独占 claim 成功后死循环挂起直到被外部 kill。默认模式（无 hold 参数）用于通知链路手动验证（自冲突 claim + 等通知 + 清理退出）。


## 4. 测试结果

设备：rk3568 / OpenHarmony 7.0.0.40（32 位 ARM）；PL2303 串口 + HP 鼠标双设备环境（150100424a5444345209d93cbe28b900 等）

**ArkTS 接口签名对齐说明**：ArkTS 层最终签名为 `claimInterfaceExclusive(pipe, iface, force?, onConflict?: Callback<InterfaceConflictCallback>): void`——`onConflict` 为 `Callback<T>` 单参数回调（无 err），数据结构 `InterfaceConflictCallback{busNum, devAddr, interfaceId}`，`claimInterfaceWithForce` 已废弃删除。C++ Client（`UsbSrvClient::ClaimInterfaceExclusive(pipe, interface, force, callback)`）与 NAPI 层（threadsafefunction 单参数调用）均与该签名对齐；UT 直调 C++ Client 接口签名未变，**用例代码无需修改**，已在 150100424a5444345209d93cbe28b900（OH 7.0.0.40 + PL2303 串口 b1d6 + HP 鼠标 b3d3 双设备）全量复验 **63/63 通过**（multi_device 7 例与 force 2 例在双设备齐备后由 SKIP 转为 PASSED）

**ArkTS 异常化修改说明**：ArkTS 接口由返回错误码改为**抛出 BusinessError 异常**（void 函数），NAPI 层 `PipeClaimInterfaceExclusive` 将服务端返回值映射为异常：`UEC_SERVICE_PERMISSION_DENIED`→BusinessError 14400001（无权限）、`UEC_INTERFACE_BUSY`→14400007（接口被独占）、`UEC_SERVICE_INVALID_VALUE`→14400004（服务异常）、其他→14400010（驱动异常），与 d.ts `@throws` 声明（中英文）一致。**异常转换仅发生在 NAPI 层**，C++ Client 仍返回 int32_t 错误码，UT 直调 C++ Client 契约不变，**用例无需修改**，已在同设备复验 63/63 通过

| 套件 | 用例数 | 结果 |
|---|---|---|
| test_claim_exclusive | 12 | 12 PASSED |
| test_claim_notify | 7 | 7 PASSED（hap 身份通知断言，需 Permissive） |
| test_claim_release | 14 | 14 PASSED |
| test_claim_transfer | 15 | 15 PASSED |
| test_claim_hotplug | 4 | 4 PASSED |
| test_claim_force | 2 | 2 PASSED |
| test_claim_multi_device | 7 | 7 PASSED（单设备环境 7 SKIP） |
| test_claim_death | 2 | 2 PASSED |
| **合计** | **63** | **63 PASSED / 0 FAILED** |

## 5. 遗留问题

| # | 问题 | 说明 | 建议 |
|---|---|---|---|
| 1 | ~~异步通知接收链路未自动化验证~~（**已自动化 + 根因已定位：SELinux 缺策略**） | `test_claim_notify` 已实现通知接收自动化断言（hap token 回调 + 独立 probe 进程触发，Permissive 下通知到达且参数匹配）。Enforcing 下被拒根因（kmsg avc 实锤）：`avc: denied { call } for comm="usb_service" scontext=u:r:usb_service:s0 tcontext=u:r:su:s0 tclass=binder permissive=0`——usb_service 域缺向客户端域 binder call 权限；另有配套 `{ transfer }`。注意测试进程跑在 su 域（SetSelfToken 不改 SELinux 域），真实 hap 应用域预期不受限 | SELinux 策略补充：`allow usb_service <client_domain>:binder { call transfer }`；UT 执行通知类验证时临时 `setenforce 0`；真实 hap 应用行为待 ArkTS demo 最终确认 |
| 2 | ~~死亡自动释放用例搁置~~（已解决） | `test_claim_death`（独立文件）已实现完整三段式验证：stage1 子进程（`claim_notify_probe` hold 模式）独占 claim → 存活期间他人独占 claim 被 `UEC_INTERFACE_BUSY` 拒绝（互斥先验证）→ SIGKILL（无清理路径）→ 新进程独占 claim 成功。死亡自动释放实测生效。**实现要点**：probe 需 hold 模式长持独占（默认模式会在数秒内自行释放退出，导致时序上测不到互斥）；native token 由进程名确定性生成，同名进程会被服务端视为同一应用走幂等分支，故 stage1/stage2 必须不同进程名 | 需求 TC32"幂等 claim 后死亡不重复释放"未单独断言（HDI 次数真机不可见，见 #5），可在 mock 环境补充 |
| 3 | ~~写方向传输变体未实际执行~~（已解决） | 已接入 PL2303 串口设备（bulk IN 0x83 + bulk OUT 0x02），`test_claim_multi_device` 套件真实执行读/写/回环与写方向阻断用例；transfer 套件方向自适应机制在接入双向端点设备后也自动执行写变体 | — |
| 4 | **多接口设备的接口隔离用例仍受限** | 双设备隔离已由 `test_claim_multi_device` 覆盖（ClaimMultiDev005/006）；但"A 独占 if0 + B 独占 if1 同设备接口隔离"仍需单设备多接口（当前两个设备均只有接口 0） | 如后续接入多接口设备（如 CDC 复合设备），在 multi_device 套件追加同设备双接口用例 |
| 5 | **HDI 调用次数无法在真机断言** | 原方案设计的 gmock `Times(n)` 精确断言 HDI 调用次数仅适用于 mock 框架（本地 service_unittest）；真机方案通过返回值 + 后续状态观测间接验证（如 ClaimRelease004 通过 release 后传输被阻断证明 normalClaimed 已清除） | 如需精确次数断言，可在 mock 测试环境（`test/native/mock/`）补充对应 mock 用例 |
| 6 | **设备环境依赖** | 测试需真机 + 已插入的非 hub USB 设备 + root 权限（hdc shell）；无设备时 SetUpTestCase 直接失败。`test_claim_hotplug` 与 `test_claim_force` 会真实断开/恢复所有 USB 外设（含调试用的鼠标键盘），`test_claim_force` 还要求接入 HID 类设备（鼠标/键盘，全设备扫描 + 拔插后重试选取）；`test_claim_multi_device` 需双设备（单设备自动 SKIP）；`test_claim_death` 需 `/data/claim_notify_probe` 已部署且建议 SELinux Permissive | 纳入设备实验室定时执行，避免在依赖 USB 外设的人工调试环境运行 hotplug/force 套件 |
| 7 | **远端 usb_service.cpp 与本地有一处无关差异** | 远程仓库 `usb_service.cpp:2728` 附近 `hasActiveDevices` 用 `usbHostManager_->hasActiveDevice()`，本地用 `GetDevices` 判空（远程版本已编译验证通过） | 后续 fetch 时注意该文件合并冲突 |

## 6. 环境恢复

- 设备原 `libusbservice.z.so` 备份于 `/data/local/tmp/usb_backup/libusbservice.z.so.bak`，如需回退：hdc 推回 `/system/lib/` 并 kill usb_service
- 测试二进制位于设备 `/data/test_claim_*`，可随时删除
