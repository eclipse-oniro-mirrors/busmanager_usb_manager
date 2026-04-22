/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "usb_descriptor_parser.h"
#include "hilog_wrapper.h"
#include "message_parcel.h"
#include "securec.h"
#include "usb_config.h"
#include "usb_endpoint.h"
#include "usb_errors.h"
#include "usb_interface.h"
#include "usbd_type.h"

static constexpr uint8_t NORMAL_ENDPOINT_DESCRIPTOR = 7;
static constexpr uint8_t AUDIO_ENDPOINT_DESCRIPTOR = 9;
namespace OHOS {
namespace USB {
enum class DescriptorType {
    DESCRIPTOR_TYPE_DEVICE = 1,
    DESCRIPTOR_TYPE_CONFIG = 2,
    DESCRIPTOR_TYPE_INTERFACE = 4,
    DESCRIPTOR_TYPE_ENDPOINT = 5
};

UsbDescriptorParser::UsbDescriptorParser() {}

UsbDescriptorParser::~UsbDescriptorParser() {}

int32_t UsbDescriptorParser::ParseDeviceDescriptor(const uint8_t *buffer, uint32_t length, UsbDevice &dev)
{
    if (buffer == nullptr || length == 0) {
        USB_HILOGE(MODULE_USB_HOST, "buffer is null or length is zero");
        return UEC_SERVICE_INVALID_VALUE;
    }

    USB_HILOGD(MODULE_USB_HOST, "parse begin length=%{public}u", length);
    uint32_t deviceDescriptorSize = sizeof(UsbdDeviceDescriptor);
    if (length < deviceDescriptorSize) {
        USB_HILOGE(MODULE_USB_HOST, "buffer size error");
        return UEC_SERVICE_INVALID_VALUE;
    }

    UsbdDeviceDescriptor deviceDescriptor = *(reinterpret_cast<const UsbdDeviceDescriptor *>(buffer));
    if (deviceDescriptor.bLength != deviceDescriptorSize) {
        USB_HILOGE(MODULE_USB_HOST, "UsbdDeviceDescriptor size error");
        return UEC_SERVICE_INVALID_VALUE;
    }

    dev.SetVendorId(deviceDescriptor.idVendor);
    dev.SetProductId(deviceDescriptor.idProduct);
    dev.SetClass(deviceDescriptor.bDeviceClass);
    dev.SetSubclass(deviceDescriptor.bDeviceSubClass);
    dev.SetProtocol(deviceDescriptor.bDeviceProtocol);
    dev.SetDescConfigCount(deviceDescriptor.bNumConfigurations);

    dev.SetbMaxPacketSize0(deviceDescriptor.bMaxPacketSize0);
    dev.SetbcdDevice(deviceDescriptor.bcdDevice);
    dev.SetbcdUSB(deviceDescriptor.bcdUSB);
    dev.SetiManufacturer(deviceDescriptor.iManufacturer);
    dev.SetiProduct(deviceDescriptor.iProduct);
    dev.SetiSerialNumber(deviceDescriptor.iSerialNumber);
    return UEC_OK;
}

static int32_t AddConfig(std::vector<USBConfig> &configs, const UsbdConfigDescriptor *configDescriptor)
{
    if (configDescriptor == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "configDescriptor is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }

    USBConfig config;
    config.SetId(configDescriptor->bConfigurationValue);
    config.SetAttribute(configDescriptor->bmAttributes);
    config.SetMaxPower(configDescriptor->bMaxPower);
    config.SetiConfiguration(configDescriptor->iConfiguration);
    configs.emplace_back(config);
    USB_HILOGD(MODULE_USB_HOST, "add config, interfaces=%{public}u", configDescriptor->bNumInterfaces);
    return UEC_OK;
}

static int32_t AddInterface(std::vector<USBConfig> &configs, const UsbdInterfaceDescriptor *interfaceDescriptor)
{
    if (interfaceDescriptor == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "interfaceDescriptor is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    if (configs.empty()) {
        USB_HILOGE(MODULE_USB_HOST, "config descriptor not found");
        return UEC_SERVICE_INVALID_VALUE;
    }

    UsbInterface interface;
    interface.SetId(interfaceDescriptor->bInterfaceNumber);
    interface.SetProtocol(interfaceDescriptor->bInterfaceProtocol);
    interface.SetAlternateSetting(interfaceDescriptor->bAlternateSetting);
    interface.SetClass(interfaceDescriptor->bInterfaceClass);
    interface.SetSubClass(interfaceDescriptor->bInterfaceSubClass);
    interface.SetiInterface(interfaceDescriptor->iInterface);
    configs.back().GetInterfaces().emplace_back(interface);
    USB_HILOGD(MODULE_USB_HOST, "add interface, endpoints=%{public}u", interfaceDescriptor->bNumEndpoints);
    return UEC_OK;
}

static int32_t AddEndpoint(std::vector<USBConfig> &configs, const UsbdEndpointDescriptor *endpointDescriptor)
{
    if (endpointDescriptor == nullptr) {
        USB_HILOGE(MODULE_USB_HOST, "endpointDescriptor is nullptr");
        return UEC_SERVICE_INVALID_VALUE;
    }
    if (configs.empty() || configs.back().GetInterfaces().empty()) {
        USB_HILOGE(MODULE_USB_HOST, "interface descriptor not found");
        return UEC_SERVICE_INVALID_VALUE;
    }

    USBEndpoint endpoint;
    endpoint.SetAddr(endpointDescriptor->bEndpointAddress);
    endpoint.SetAttr(endpointDescriptor->bmAttributes);
    endpoint.SetInterval(endpointDescriptor->bInterval);
    endpoint.SetMaxPacketSize(endpointDescriptor->wMaxPacketSize);
    endpoint.SetInterfaceId(configs.back().GetInterfaces().back().GetId());
    configs.back().GetInterfaces().back().GetEndpoints().emplace_back(endpoint);
    USB_HILOGD(MODULE_USB_HOST, "add endpoint, address=%{public}u", endpointDescriptor->bEndpointAddress);
    return UEC_OK;
}

int32_t UsbDescriptorParser::ParseConfigDescriptors(std::vector<uint8_t> &descriptor, uint32_t offset,
    std::vector<USBConfig> &configs)
{
    uint8_t *buffer = descriptor.data();
    uint32_t length = descriptor.size();
    uint32_t cursor = offset;
    int32_t ret = UEC_OK;

    while (cursor < length) {
        if ((length - cursor) < sizeof(UsbdDescriptorHeader)) {
            USB_HILOGW(MODULE_USB_HOST, "invalid desc data, length=%{public}u, cursor=%{public}u", length, cursor);
            break;
        }
        UsbdDescriptorHeader descriptorHeader = *(reinterpret_cast<const UsbdDescriptorHeader *>(buffer + cursor));
        if (descriptorHeader.bLength < sizeof(UsbdDescriptorHeader) || descriptorHeader.bLength > (length - cursor)) {
            USB_HILOGW(MODULE_USB_HOST, "invalid data length, bLen=%{public}u, length=%{public}u, cursor=%{public}u",
                descriptorHeader.bLength, length, cursor);
            break;
        }
        switch (descriptorHeader.bDescriptorType) {
            case static_cast<uint8_t>(DescriptorType::DESCRIPTOR_TYPE_CONFIG):
                if (descriptorHeader.bLength != sizeof(UsbdConfigDescriptor)) {
                    USB_HILOGE(MODULE_USB_HOST, "invalid config, length=%{public}u", descriptorHeader.bLength);
                    return UEC_SERVICE_INVALID_VALUE;
                }
                ret = AddConfig(configs, reinterpret_cast<const UsbdConfigDescriptor *>(buffer + cursor));
                break;
            case static_cast<uint8_t>(DescriptorType::DESCRIPTOR_TYPE_INTERFACE):
                if (descriptorHeader.bLength != sizeof(UsbdInterfaceDescriptor)) {
                    USB_HILOGE(MODULE_USB_HOST, "invalid interface, length=%{public}u", descriptorHeader.bLength);
                    return UEC_SERVICE_INVALID_VALUE;
                }
                ret = AddInterface(configs, reinterpret_cast<const UsbdInterfaceDescriptor *>(buffer + cursor));
                break;
            case static_cast<uint8_t>(DescriptorType::DESCRIPTOR_TYPE_ENDPOINT):
                if (descriptorHeader.bLength != NORMAL_ENDPOINT_DESCRIPTOR
                    && descriptorHeader.bLength != AUDIO_ENDPOINT_DESCRIPTOR) {
                    USB_HILOGE(MODULE_USB_HOST, "invalid endpoint, length=%{public}u", descriptorHeader.bLength);
                    return UEC_SERVICE_INVALID_VALUE;
                }
                ret = AddEndpoint(configs, reinterpret_cast<const UsbdEndpointDescriptor *>(buffer + cursor));
                break;
            default:
                USB_HILOGW(MODULE_USB_HOST, "unrecognized type=%{public}d", descriptorHeader.bDescriptorType);
                break;
        }
        if (ret != UEC_OK) {
            return ret;
        }
        cursor += descriptorHeader.bLength;
    }
    return ret;
}
} // namespace USB
} // namespace OHOS
