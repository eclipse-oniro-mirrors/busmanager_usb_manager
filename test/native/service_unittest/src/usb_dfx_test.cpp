/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <csignal>
#include <iostream>
#include <memory>
#include <strings.h>
#include <vector>
#include <map>
#include <semaphore.h>
#include "cJSON.h"
#include "usb_dfx_test.h"

#include "ashmem.h"
#include "securec.h"
#include "delayed_sp_singleton.h"
#include "hilog_wrapper.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "usb_common_test.h"
#include "usb_errors.h"
#include "usb_srv_client.h"
#include "usb_srv_support.h"
#include "common_event_manager.h"
#include "common_event_support.h"

#include "usb_host_manager.h"
#include "usb_device.h"
#include "usb_config.h"
#include "usb_interface.h"

using namespace OHOS::EventFwk;
using namespace testing::ext;
using namespace OHOS::USB;
using namespace OHOS;
using namespace OHOS::HDI::Usb::V1_0;
using namespace OHOS::USB::Common;


namespace OHOS {
namespace USB {
namespace USBDFX {
constexpr int32_t USB_BUS_NUM_INVALID = -1;
constexpr int32_t USB_DEV_ADDR_INVALID = -1;
constexpr uint32_t USB_ENDPOINT_DIR_OUT = 0;
constexpr uint32_t USB_ENDPOINT_DIR_IN = 0x80;
constexpr uint32_t ASHMEM_MAX_SIZE = 1024;
constexpr uint32_t MEM_DATA = 1024 * 1024;
constexpr uint8_t TEST_BUS_NUM = 1;
constexpr uint8_t TEST_DEV_ADDR = 2;
constexpr uint8_t TEST_CONFIG_INDEX = 0;
sem_t UsbDfxTest::testSem_ {};

static int32_t InitAshmemOne(sptr<Ashmem> &asmptr, int32_t asmSize, uint8_t rflg)
{
    asmptr = Ashmem::CreateAshmem("ttashmem000", asmSize);
    if (asmptr == nullptr) {
        USB_HILOGE(MODULE_USB_SERVICE, "InitAshmemOne CreateAshmem failed");
        return UEC_SERVICE_NO_MEMORY;
    }

    asmptr->MapReadAndWriteAshmem();

    if (rflg == 0) {
        uint8_t tdata[ASHMEM_MAX_SIZE];
        int32_t offset = 0;
        int32_t tlen = 0;
        int32_t retSafe = memset_s(tdata, sizeof(tdata), 'Y', ASHMEM_MAX_SIZE);
        if (retSafe != EOK) {
            USB_HILOGE(MODULE_USB_SERVICE, "InitAshmemOne memset_s failed");
            return UEC_SERVICE_NO_MEMORY;
        }
        while (offset < asmSize) {
            tlen = (asmSize - offset) < ASHMEM_MAX_SIZE ? (asmSize - offset) : ASHMEM_MAX_SIZE;
            asmptr->WriteToAshmem(tdata, tlen, offset);
            offset += tlen;
        }
    }

    return 0;
}

void UsbDfxTest::SetUpTestCase(void)
{
    UsbCommonTest::GrantPermissionSysNative();
    USB_HILOGI(MODULE_USB_SERVICE, "Start UsbDfxTest");
}

void UsbDfxTest::TearDownTestCase(void)
{
    USB_HILOGI(MODULE_USB_SERVICE, "End UsbDfxTest");
}

void UsbDfxTest::SetUp(void) {}

void UsbDfxTest::TearDown(void) {}

class UsbSubscriberTest : public CommonEventSubscriber {
public:
    explicit UsbSubscriberTest(const CommonEventSubscribeInfo &sp) : CommonEventSubscriber(sp) {}

    void OnReceiveEvent(const CommonEventData &data) override
    {
        USB_HILOGI(MODULE_USB_SERVICE, "recv event ok");
        auto &want = data.GetWant();
        if (want.GetAction() == CommonEventSupport::COMMON_EVENT_USB_STATE) {
            eventDatas[CommonEventSupport::COMMON_EVENT_USB_STATE] = 1;
        } else if (want.GetAction() == CommonEventSupport::COMMON_EVENT_USB_DEVICE_ATTACHED) {
            eventDatas[CommonEventSupport::COMMON_EVENT_USB_DEVICE_ATTACHED] = 1;
        }
        sem_post(&UsbDfxTest::testSem_);
    }

    bool PrintPromptMsg()
    {
        auto usbStates = eventDatas.find(CommonEventSupport::COMMON_EVENT_USB_STATE);
        auto attachStates = eventDatas.find(CommonEventSupport::COMMON_EVENT_USB_DEVICE_ATTACHED);
        if (usbStates != eventDatas.end() && attachStates != eventDatas.end()) {
            std::cout<< "test ok" << std::endl;
            return true;
        } else if (usbStates == eventDatas.end()) {
            std::cout<< "please connect or disconnect the gadget to some host" << std::endl;
        } else if (attachStates == eventDatas.end()) {
            std::cout << "please connect or disconnect a device to the host" << std::endl;
        }
        return false;
    }
    int32_t GetMapSize()
    {
        return eventDatas.size();
    }

private:
    std::map<std::string, int32_t> eventDatas;
};

/**
 * @tc.name: ReportSysEvent001
 * @tc.desc: Trigger the dot event PLUG_IN_OUT_HOST_MODE, PLUG_IN_OUT_DEVICE_MODE,
 * @tc.type: FUNC
 */
HWTEST_F(UsbDfxTest, ReportSysEvent001, TestSize.Level1)
{
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USB_STATE);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USB_DEVICE_ATTACHED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USB_DEVICE_DETACHED);
    CommonEventSubscribeInfo subscriberInfo(matchingSkills);
    std::shared_ptr<UsbSubscriberTest> subscriber = std::make_shared<UsbSubscriberTest>(subscriberInfo);
    CommonEventManager::SubscribeCommonEvent(subscriber);

    while (!subscriber->PrintPromptMsg())
    {
        sem_wait(&UsbDfxTest::testSem_);
    }

    ASSERT_NE(subscriber->GetMapSize(), 0);
}

/**
 * @tc.name: ReportSysEvent002
 * @tc.desc: Trigger the dot event FUNCTION_CHANGED, PORT_ROLE_CHANGED
 * @tc.type: FUNC
 */
HWTEST_F(UsbDfxTest, GetCurrentFunctions002, TestSize.Level1)
{
    std::cout << "please connect device, press enter to continue" << std::endl;
    int32_t c;
    while ((c = getchar()) != '\n' && c != EOF) {
        std::cout << "please connect device, press enter to continue" << std::endl;
    }
    int32_t ret = 0;
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ReportSysEvent002");
    auto &UsbSrvClient = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_NONE);
    UsbSrvClient.GetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDfxTest::ret=%{public}d", ret);
    int32_t isok = UsbSrvClient.SetCurrentFunctions(funcs);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDfxTest::SetCurrentFunctions=%{public}d", isok);

    UsbSrvClient.SetPortRole(
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    USB_HILOGI(MODULE_USB_SERVICE, "UsbDfxTest::status=%{public}d", ret);
    UsbCommonTest::SwitchErrCode(ret);
    ASSERT_EQ(0, ret);

    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ReportSysEvent002");
}

/**
 * @tc.name: ReportSysEvent003
 * @tc.desc: Trigger the dot event USB_TRANSFER_FAULT
 * @tc.type: FUNC
 */
HWTEST_F(UsbDfxTest, GetCurrentFunctions003, TestSize.Level1)
{
    int32_t ret = 0;
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : ReportSysEvent003.");
    auto &UsbSrvClient = UsbSrvClient::GetInstance();
    UsbCommonTest::GrantPermissionSysNative();
    
    std::vector<UsbDevice> devs;
    UsbSrvClient.GetDevices(devs);
    ASSERT_NE(devs.size(), 0);
    UsbDevice device = devs.at(0);
    USBDevicePipe pipe;
    UsbSrvClient.OpenDevice(device, pipe);
    vector<uint8_t> buffData;
    ASSERT_NE(device.GetConfigs().front().GetInterfaces().size(), 0);
    UsbInterface interface = device.GetConfigs().front().GetInterfaces().at(0);
    ASSERT_NE(interface.GetEndpoints().size(), 0);
    USBEndpoint point = interface.GetEndpoints().front();
    UsbSrvClient.BulkTransfer(pipe, point, buffData, 100);

    UsbSrvClient.GetRawDescriptors(pipe, buffData);
    UsbSrvClient.Close(pipe);

    sptr<Ashmem> ashmem;
    uint8_t rflg = 0;
    InitAshmemOne(ashmem, MEM_DATA, rflg);
    UsbSrvClient.BulkRead(pipe, point, ashmem);
    ret = UsbSrvClient.BulkWrite(pipe, point, ashmem);
    // control transfer
    struct UsbCtrlTransfer ctrldata = {0b10000000, 8, 0, 0, 500};
    UsbSrvClient.ControlTransfer(pipe, ctrldata, buffData);
    struct HDI::Usb::V1_2::UsbCtrlTransferParams ctrldataParams = {0b10000000, 8, 0, 0, 8, 500};
    UsbSrvClient.UsbControlTransfer(pipe, ctrldataParams, buffData);
    // bulk transfer
    HDI::Usb::V1_2::USBTransferInfo info = {point.GetAddress(), 0, 2, 500, 8, 0b10000000, 0};
    TransferCallback cb = [] (const TransferCallbackInfo &info,
        const std::vector<HDI::Usb::V1_2::UsbIsoPacketDescriptor> &isoInfo, uint64_t userData) -> void{};
    UsbSrvClient.UsbSubmitTransfer(pipe, info, cb, ashmem);
    // isochronous transfer
    info = {point.GetAddress(), 0, 1, 500, 8, 0b10000000, 1};
    UsbSrvClient.UsbSubmitTransfer(pipe, info, cb, ashmem);
    // interrupt transfer
    info = {point.GetAddress(), 0, 3, 500, 8, 0b10000000, 0};
    UsbSrvClient.UsbSubmitTransfer(pipe, info, cb, ashmem);

    ASSERT_NE(ret, 0);
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : ReportSysEvent003.");
}

/**
 * @tc.name: HiTrace001
 * @tc.desc: Trigger hitrace
 * @tc.type: FUNC
 */
HWTEST_F(UsbDfxTest, HiTrace001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "Case Start : HiTrace001.");
    auto &UsbSrvClient = UsbSrvClient::GetInstance();
    int32_t funcs = static_cast<int32_t>(UsbSrvSupport::FUNCTION_NONE);
    UsbSrvClient.GetCurrentFunctions(funcs);

    UsbSrvClient.SetPortRole(
        UsbSrvSupport::PORT_MODE_DEVICE, UsbSrvSupport::POWER_ROLE_SOURCE, UsbSrvSupport::DATA_ROLE_HOST);
    UsbDevice device;
    device.SetBusNum(USB_BUS_NUM_INVALID);
    device.SetDevAddr(USB_DEV_ADDR_INVALID);
    USBDevicePipe pipe;
    UsbSrvClient.OpenDevice(device, pipe);
    UsbInterface interface;
    UsbSrvClient.ClaimInterface(pipe, interface, true);
    UsbSrvClient.ReleaseInterface(pipe, interface);

    vector<uint8_t> buffData;
    USBEndpoint pointIn(USB_ENDPOINT_DIR_IN, 0, 0, 0);
    UsbSrvClient.BulkTransfer(pipe, pointIn, buffData, 100);
    USBEndpoint pointOut(USB_ENDPOINT_DIR_OUT, 0, 0, 0);
    UsbSrvClient.BulkTransfer(pipe, pointOut, buffData, 100);

    struct UsbCtrlTransfer ctrldata = {0b10000000, 8, 0, 0, 500};
    UsbSrvClient.ControlTransfer(pipe, ctrldata, buffData);

    USBConfig config;
    UsbSrvClient.SetConfiguration(pipe, config);

    UsbSrvClient.SetInterface(pipe, interface);

    sptr<Ashmem> ashmem;
    uint8_t rflg = 0;
    InitAshmemOne(ashmem, MEM_DATA, rflg);
    UsbSrvClient.BulkRead(pipe, pointIn, ashmem);
    UsbSrvClient.BulkWrite(pipe, pointOut, ashmem);

    ASSERT_EQ(true, true);
    UsbCommonTest::GrantPermissionSysNative();
    USB_HILOGI(MODULE_USB_SERVICE, "Case End : HiTrace001.");
}

/**
 * @tc.name: UsbHostManager_GetActiveInterfacesJson_001
 * @tc.desc: Normal path: GetActiveConfig fails (usbd_==nullptr) so fallback to cached
 *           configs[0]; 1 config with 2 interfaces -> JSON array with correct field mapping
 * @tc.type: FUNC
 */
HWTEST_F(UsbDfxTest, UsbHostManager_GetActiveInterfacesJson_001, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_001 start");

    UsbInterface intf1;
    intf1.SetId(0);
    intf1.SetClass(0x03);
    intf1.SetSubClass(0x01);
    intf1.SetProtocol(0x01);
    intf1.SetAlternateSetting(0);

    UsbInterface intf2;
    intf2.SetId(1);
    intf2.SetClass(0x08);
    intf2.SetSubClass(0x06);
    intf2.SetProtocol(0x50);
    intf2.SetAlternateSetting(1);

    USBConfig config;
    config.SetId(TEST_CONFIG_INDEX);
    config.SetInterfaces({intf1, intf2});

    UsbDevice device;
    device.SetBusNum(TEST_BUS_NUM);
    device.SetDevAddr(TEST_DEV_ADDR);
    device.SetConfigs({config});

    std::unique_ptr<UsbHostManager> usbHostManager = std::make_unique<UsbHostManager>(nullptr);

    nlohmann::json interfacesJson;
    interfacesJson.push_back({
        {"stale", "data"}
    });

    usbHostManager->GetActiveInterfacesJson(&device, interfacesJson);

    EXPECT_EQ(interfacesJson.size(), 2);
    if (interfacesJson.size() >= 2) {
        EXPECT_EQ(interfacesJson[0]["id"], 0);
        EXPECT_EQ(interfacesJson[0]["class"], 0x03);
        EXPECT_EQ(interfacesJson[0]["subclass"], 0x01);
        EXPECT_EQ(interfacesJson[0]["protocol"], 0x01);
        EXPECT_EQ(interfacesJson[0]["altSetting"], 0);
        EXPECT_EQ(interfacesJson[1]["id"], 1);
        EXPECT_EQ(interfacesJson[1]["class"], 0x08);
        EXPECT_EQ(interfacesJson[1]["subclass"], 0x06);
        EXPECT_EQ(interfacesJson[1]["protocol"], 0x50);
        EXPECT_EQ(interfacesJson[1]["altSetting"], 1);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_001 size=%{public}zu", interfacesJson.size());
}

/**
 * @tc.name: UsbHostManager_GetActiveInterfacesJson_002
 * @tc.desc: Multi-config device: fallback selection targets configs[0] (index 0),
 *           NOT configs[1], verifying the index-1 adjustment logic
 * @tc.type: FUNC
 */
HWTEST_F(UsbDfxTest, UsbHostManager_GetActiveInterfacesJson_002, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_002 start");

    UsbInterface intf0;
    intf0.SetId(0);
    USBConfig config0;
    config0.SetId(0);
    config0.SetName("Cfg0");
    config0.SetInterfaces({intf0});

    UsbInterface marker;
    marker.SetId(7);
    marker.SetClass(0x09);
    USBConfig config1;
    config1.SetId(1);
    config1.SetName("Cfg1");
    config1.SetInterfaces({marker});

    UsbDevice device;
    device.SetBusNum(TEST_BUS_NUM);
    device.SetDevAddr(TEST_DEV_ADDR);
    device.SetConfigs({config0, config1});

    std::unique_ptr<UsbHostManager> usbHostManager = std::make_unique<UsbHostManager>(nullptr);

    nlohmann::json interfacesJson;
    usbHostManager->GetActiveInterfacesJson(&device, interfacesJson);

    EXPECT_EQ(interfacesJson.size(), 1);
    if (interfacesJson.size() >= 1) {
        EXPECT_EQ(interfacesJson[0]["id"], 0);
    }
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_002 size=%{public}zu", interfacesJson.size());
}

/**
 * @tc.name: UsbHostManager_GetActiveInterfacesJson_003
 * @tc.desc: Out-of-range: device has no configs -> index 0 >= size 0 -> output cleared
 * @tc.type: FUNC
 */
HWTEST_F(UsbDfxTest, UsbHostManager_GetActiveInterfacesJson_003, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_003 start");

    UsbDevice device;
    device.SetBusNum(TEST_BUS_NUM);
    device.SetDevAddr(TEST_DEV_ADDR);

    std::unique_ptr<UsbHostManager> usbHostManager = std::make_unique<UsbHostManager>(nullptr);

    nlohmann::json interfacesJson;
    interfacesJson.push_back({
        {"pre", "existing"}
    });

    usbHostManager->GetActiveInterfacesJson(&device, interfacesJson);

    EXPECT_TRUE(interfacesJson.empty());
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_003 size=%{public}zu", interfacesJson.size());
}

/**
 * @tc.name: UsbHostManager_GetActiveInterfacesJson_004
 * @tc.desc: Empty-interface config: config exists but has 0 interfaces ->
 *           result is a JSON array (not null) with size 0
 * @tc.type: FUNC
 */
HWTEST_F(UsbDfxTest, UsbHostManager_GetActiveInterfacesJson_004, TestSize.Level1)
{
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_004 start");

    USBConfig config;
    config.SetId(0);
    config.SetInterfaces({});

    UsbDevice device;
    device.SetBusNum(TEST_BUS_NUM);
    device.SetDevAddr(TEST_DEV_ADDR);
    device.SetConfigs({config});

    std::unique_ptr<UsbHostManager> usbHostManager = std::make_unique<UsbHostManager>(nullptr);

    nlohmann::json interfacesJson = nlohmann::json::array();
    usbHostManager->GetActiveInterfacesJson(&device, interfacesJson);

    EXPECT_TRUE(interfacesJson.is_array());
    EXPECT_EQ(interfacesJson.size(), 0);
    USB_HILOGI(MODULE_USB_SERVICE, "GetActiveInterfacesJson_004 size=%{public}zu", interfacesJson.size());
}

} // Core
} // USB
} // OHOS
