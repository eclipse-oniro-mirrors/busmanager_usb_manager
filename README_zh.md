# USB服务组件<a name="ZH-CN_TOPIC_0000001124094823"></a>

-   [简介](#section11660541593)
-   [目录](#section19472752217)
-   [接口](#section19472752218)
-   [开发示例](#section19472752219)
-   [相关仓](#section63151229062)

## 简介<a name="section11660541593"></a>
USB服务框架如下图所示：

**图 1**  USB服务框架架图<a name="fig15658513184019"></a>
![](figures/usb-manager-architecture_zh.png "USB服务框架架图")

架构及功能介绍：
1. USB API：提供USB的基础API，主要包含查询USB设备的列表、设备插拔通知、USB HOST/DEVICE 功能切换、批量数据传输、控制命令传输、USB设备打开的权限控制及USB device模式下的function功能切换等。
2. USB Service：主要实现HAL层数据接收、解析、分发，前后台的策略管控，对该设备USB的管理，USB权限管控等。
3. USB HAL层：提供给用户态可直接调用的驱动能力接口，按照功能分类三大类：DDK初始化类、对interface对象操作类、对request对象操作类，可以提供DDK初始化、interface绑定和释放，打开和关闭操作，request的申请和释放，同步和异步传输等。HAL层代码位于[drivers\_peripheral](https://gitcode.com/openharmony/drivers_peripheral/blob/master/README_zh.md)

### 串口能力说明<a name="section15658513184020"></a>

本仓 `services/native/src/serial_manager.cpp` 提供 USB 转串口的基础通信能力：端口列表查询、打开与关闭、同步读写，以及波特率、数据位、停止位、校验位等串口属性的读写；端口归属按 tokenId 校验，串口操作会上报审计事件。该模块在检测到 USB 转串口设备时启用。

板载串口、更多串口类型，以及更完整的串口信号管理（RTS/CTS、DTR/DSR、flush/drain/断帧）由独立的串口管理组件提供，详见 [serial_manager](https://gitcode.com/openharmony-sig/busmanager_serial_manager)。

| 能力维度 | 本仓 serial 模块 | serial_manager |
| --- | --- | --- |
| 设备范围 | USB 转串口（`ttyUSB`） | 板载串口与 USB 转串口 |
| 对外接口 | usb_manager 的 USB 串口 API | `@ohos.busManager.serial` |
| 通信功能 | 同步/异步读写、属性配置 | 异步回调收发、信号与缓冲控制 |
| 授权模型 | 按 tokenId 校验端口归属 | 应用级授权，授权结果持久化 |

两者最终操作同一批 USB 转串口设备，但端口标识方式不同：本仓使用 `portId`（对应 USB 设备的 bus/dev、VID/PID、序列号），serial_manager 使用设备名。同一端口请勿同时由两个服务打开；新业务建议优先使用 serial_manager，存量 USB 业务可继续使用本仓能力。

## 目录<a name="section19472752217"></a>
```
base/usb/usb_manager
├── interfaces                  # 接口层
│   ├── innerkits                  # 内部接口
│   └── kits                       # 外部接口
├── sa_profile                  # SA配置文件
└── services                    # 服务层
│   ├── native                     # native层
│   └── zidl                       # zidl接口层
└── test                        # 测试用例
└── utils                       # 工具和通用层
```
## 接口<a name="section19472752218"></a>
### [外部接口](https://gitcode.com/openharmony/interface_sdk-js/blob/master/api/@ohos.usbManager.d.ts)

### 内部接口
#### Host 功能接口

| **内部接口申明** | **接口描述** |
| --- | --- |
| int32_t OpenDevice(const UsbDevice &device, USBDevicePipe &pip); | 打开设备，建立连接 |
| bool Close(const USBDevicePipe &pip); | 关闭设备，释放与设备相关的所有系统资源 |
| int32_t GetDevices(std::vector &deviceList); | 获取设备列表 |
| int32_t SetConfiguration(USBDevicePipe &pip, const USBConfig &config); | 设置设备当前使用的配置，通过配置值进行指定 |
| int32_t ClaimInterface(USBDevicePipe &pip, const UsbInterface &interface, bool force); | 打开接口，并申明独占接口，必须在数据传输前执行 |
| int32_t ReleaseInterface(USBDevicePipe &pip, const UsbInterface &interface); | 关闭接口，释放接口的占用，在停止数据传输后执行 |
| int32_t SetInterface(USBDevicePipe &pipe, const UsbInterface &interface); | 设置指定接口的备选设置，用于在具有相同ID但不同备用设置的两个接口之间进行选择 |
| int32_t BulkTransfer(USBDevicePipe &pip, const USBEndpoint &endpoint, std::vector<uint8_t> &vdata, int32_t timeout); | 在给定端点上执行批量数据读写， 返回读取的数据和长度 |
| int32_t ControlTransfer(USBDevicePipe &pip, const UsbCtrlTransfer &ctrl, std::vector<uint8_t> &vdata); | 对此设备执行端点零的控制事务，传输方向由请求类型决定。 如果requestType＆ USB_ENDPOINT_DIR_MASK是USB_DIR_OUT ，则传输是写入，如果是USB_DIR_IN ，则传输是读取。 |
| int32_t RequestInitialize(UsbRequest &request); | 初始化请求 |
| int32_t RequestQueue(UsbRequest &request); | 进行异步数据发送或者接收请求，数据传输方向由端点方向决定 |
| int32_t PipeRequestWait(USBDevicePipe &pip, int64_t timeout, UsbRequest &req); | 等待RequestQueue异步请求的操作结果 |
| int32_t RequestAbort(UsbRequest &request); | 取消待处理的数据请求 |
| int32_t RequestFree(UsbRequest &request); | 释放数据请求 |
| bool HasRight(std::string deviceName); | 判断设备是否有权限 |
| int32_t RequestRight(std::string deviceName); | 给指定设备申请权限 |

#### Device 功能接口
| **内部接口申明** | **接口描述** |
| --- | --- |
| int32_t GetCurrentFunctions(int32_t &funcs); | 获取从设备支持的功能列表（按位域表示） |
| int32_t SetCurrentFunctions(int32_t funcs); | 设置从设备支持的功能列表（按位域表示） |
| int32_t UsbFunctionsFromString(std::string funcs); | 将给定的功能列表描述字符串转换为功能列表的数字组合掩码 |
| std::string UsbFunctionsToString(int32_t funcs); | 将给定的功能列表的数字组合掩码转换为功能列表描述字符串 |

#### Port 功能接口
| **内部接口申明** | **接口描述** |
| --- | --- |
| int32_t GetPorts(std::vector &usbPorts); | 获取port列表 |
| int32_t SetPortRole(int32_t portId, int32_t powerRole, int32_t dataRole); | 设置端口工作模式 |
| int32_t GetSupportedModes(int32_t portId, int32_t &supportedModes); | 获取指定的端口支持的模式列表的组合掩码 |

## 开发示例<a name="section19472752219"></a>
主要接口调用示例如下，详细业务流程代码可以参考测试用例代码 [js_unittest](test/native/js_unittest)
#### Host功能开发
Usb设备作为host设备连接device设备进行数据传输。
1. 获取设备列表
```JS
// 导入usb接口api包
import usb from '@ohos.usb';
// 获取设备列表
var deviceList = usb.getDevices();
```
2. 获取设备操作权限
```JS
// device name
var deviceName = deviceList[0].name;
// 申请操作指定的device的操作权限
usb.requestRight(deviceName).then(hasRight => {
	console.info("usb device request right result: " + hasRight);
}).catch(error => {
	console.info("usb device request right failed : " + error);
});
```
3. 打开USB设备
```JS
// 打开设备，获取数据传输通道
var pipe = usb.connectDevice(deviceList[0]);
// 打开对应接口
usb.claimInterface(pipe , interface, true); // interface为device中需要操作的interface，选取合适的interface进行对应操作
```
4. 数据传输
```JS
// 读取数据，在device信息中选取对应数据接收的endpoint来做数据传（endpoint.direction == 0x80）；dataUint8Array要发送的数据
usb.bulkTransfer(pipe, inEndpoint, dataUint8Array, 15000).then(dataLength => {
if (dataLength >= 0) {
	console.info("usb readData result Length : " + dataLength);
	var resultStr = this.ab2str(dataUint8Array); // uint8数据转string
	console.info("usb readData buffer : " + resultStr);
} else {
	console.info("usb readData failed : " + dataLength);
}
}).catch(error => {
console.info("usb readData error : " + JSON.stringify(error));
});
// 发送数据，在device信息中选取对应数据发送的endpoint来做数据传（endpoint.direction == 0）
usb.bulkTransfer(this.pip, this.outEndpoint, dataUint8Array, 15000).then(dataLength => {
	if (dataLength >= 0) {
		console.info("usb writeData result write length : " + dataLength);
	} else {
		console.info("writeData failed");
	}
}).catch(error => {
	console.info("usb writeData error : " + JSON.stringify(error));
});
```
5. 释放接口、关闭设备
```JS
usb.releaseInterface(pipe, interface);
usb.closePipe(pipe);
```
#### Device功能开发
Usb设备作为device设备，设置ACM、ECM、HDC等功能。
1. 设置USB function功能
```JS
usb.setCurrentFunctions(funType).then(data => {
	console.info("usb setCurrentFunctions : " + data);
}).catch(error => {
	console.info("usb setCurrentFunctions error : " + error);
});
```

## 相关仓<a name="section63151229062"></a>

[serial\_manager](https://gitcode.com/openharmony-sig/busmanager_serial_manager)（通用串口管理组件）

[驱动子系统](https://gitcode.com/openharmony/docs/blob/master/zh-cn/readme/%E9%A9%B1%E5%8A%A8%E5%AD%90%E7%B3%BB%E7%BB%9F.md)

[drivers\_peripheral](https://gitcode.com/openharmony/drivers_peripheral/blob/master/README_zh.md)

[drivers\_framework](https://gitcode.com/openharmony/drivers_framework/blob/master/README_zh.md)

[drivers\_adapter](https://gitcode.com/openharmony/drivers_adapter/blob/master/README_zh.md)

[drivers\_adapter\_khdf\_linux](https://gitcode.com/openharmony/drivers_adapter_khdf_linux/blob/master/README_zh.md)
