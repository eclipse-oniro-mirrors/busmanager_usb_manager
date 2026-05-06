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

#ifndef USB_INTERFACE_TYPE_H
#define USB_INTERFACE_TYPE_H
#include "usb_common.h"
#include "message_parcel.h"

namespace OHOS {
namespace USB {
enum InterfaceType {
    TYPE_INTERFACE,
    TYPE_STORAGE,
    TYPE_AUDIO,
    TYPE_HID,
    TYPE_PHYSICAL,
    TYPE_IMAGE,
    TYPE_PRINTER,
    TYPE_COMMUNICATION,
    TYPE_FULL_SPEED_HUB,
    TYPE_FULL_SPEED_HUB_S,
    TYPE_FULL_SPEED_HUB_M,
    TYPE_CDC_DATA,
    TYPE_SMART_CARD,
    TYPE_CONTENT_SECURTIY,
    TYPE_VIDEO,
    TYPE_PERSONAL_HEALTHCARE,
    TYPE_AVCONTROL,
    TYPE_AVV_STREAMING,
    TYPE_AVA_STREAMING,
    TYPE_BILLBOARD,
    TYPE_TYPEC_BRIDGE,
    TYPE_BDP,
    TYPE_MCTP_MANA_1,
    TYPE_MCTP_MANA_2,
    TYPE_MCTP_HOST_1,
    TYPE_MCTP_HOST_2,
    TYPE_I3C,
    TYPE_DIAGNOSTIC_1,
    TYPE_DIAGNOSTIC_2,
    TYPE_DIAGNOSTIC_3,
    TYPE_DIAGNOSTIC_4,
    TYPE_DIAGNOSTIC_5,
    TYPE_DIAGNOSTIC_6,
    TYPE_DIAGNOSTIC_7,
    TYPE_DIAGNOSTIC_8,
    TYPE_DIAGNOSTIC_9,
    TYPE_DIAGNOSTIC_10,
    TYPE_DIAGNOSTIC_11,
    TYPE_DIAGNOSTIC_12,
    TYPE_DIAGNOSTIC_13,
    TYPE_DIAGNOSTIC_14,
    TYPE_WIRLESS_CONTROLLER_1,
    TYPE_WIRLESS_CONTROLLER_2,
    TYPE_WIRLESS_CONTROLLER_3,
    TYPE_WIRLESS_CONTROLLER_4,
    TYPE_WIRLESS_CONTROLLER_5,
    TYPE_WIRLESS_CONTROLLER_6,
    TYPE_WIRLESS_CONTROLLER_7,
    TYPE_MISCELLANEOUS_1,
    TYPE_MISCELLANEOUS_2,
    TYPE_MISCELLANEOUS_3,
    TYPE_MISCELLANEOUS_4,
    TYPE_MISCELLANEOUS_5,
    TYPE_MISCELLANEOUS_6,
    TYPE_MISCELLANEOUS_7,
    TYPE_MISCELLANEOUS_8,
    TYPE_MISCELLANEOUS_9,
    TYPE_MISCELLANEOUS_10,
    TYPE_MISCELLANEOUS_11,
    TYPE_MISCELLANEOUS_12,
    TYPE_MISCELLANEOUS_13,
    TYPE_MISCELLANEOUS_14,
    TYPE_MISCELLANEOUS_15,
    TYPE_MISCELLANEOUS_16,
    TYPE_MISCELLANEOUS_17,
    TYPE_MISCELLANEOUS_18,
    TYPE_MISCELLANEOUS_19,
    TYPE_APPLICATION_SPE_1,
    TYPE_APPLICATION_SPE_2,
    TYPE_APPLICATION_SPE_3,
    TYPE_APPLICATION_SPE_4,
    TYPE_VENDOR_SPECIFIC,
};

struct UsbDeviceType {
    int32_t baseClass;
    int32_t subClass;
    int32_t protocol;
    bool isDeviceType;
    bool isDeviceTypeAllMatch = false;
    bool operator<(const UsbDeviceType &other) const
    {
        if (baseClass != other.baseClass) {
            return baseClass < other.baseClass;
        } else if (subClass != other.subClass) {
            return subClass < other.subClass;
        } else if (protocol != other.protocol) {
            return protocol < other.protocol;
        } else if (isDeviceType != other.isDeviceType) {
            return isDeviceType < other.isDeviceType;
        } else {
            return isDeviceTypeAllMatch < other.isDeviceTypeAllMatch;
        }
    }
    bool operator == (const UsbDeviceType &other) const
    {
        return (baseClass == other.baseClass) && (subClass == other.subClass) && (protocol == other.protocol) &&
            (isDeviceType == other.isDeviceType) && (isDeviceTypeAllMatch == other.isDeviceTypeAllMatch);
    }
    UsbDeviceType (int32_t deviceBaseClass, int32_t sub, int32_t prot, bool deviceType)
        : baseClass(deviceBaseClass), subClass(sub), protocol(prot), isDeviceType(deviceType),
          isDeviceTypeAllMatch(false) {};
    UsbDeviceType (): baseClass(0), subClass(0), protocol(0), isDeviceType(0), isDeviceTypeAllMatch(false) {};
    bool Marshalling(MessageParcel &parcel) const;
    static bool Unmarshalling(MessageParcel &parcel, UsbDeviceType &usbDeviceType);
    bool ReadFromParcel(MessageParcel &parcel);
};

struct UsbDeviceId {
    int32_t productId;
    int32_t vendorId;
};

const std::unordered_map<InterfaceType, std::vector<int32_t>> d_typeMap = {
    {InterfaceType::TYPE_INTERFACE,             {0, 0, 0}},
    {InterfaceType::TYPE_COMMUNICATION,         {2, -1, -1}},
    {InterfaceType::TYPE_FULL_SPEED_HUB,        {9, 0, 0}},
    {InterfaceType::TYPE_FULL_SPEED_HUB_S,      {9, 0, 1}},
    {InterfaceType::TYPE_FULL_SPEED_HUB_M,      {9, 0, 2}},
    {InterfaceType::TYPE_BILLBOARD,             {17, 0, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_1,          {220, 1, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_2,          {220, 2, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_3,          {220, 2, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_4,          {220, 3, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_5,          {220, 3, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_6,          {220, 4, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_7,          {220, 4, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_8,          {220, 5, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_9,          {220, 5, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_10,         {220, 6, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_11,         {220, 6, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_12,         {220, 7, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_13,         {220, 7, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_14,         {220, 8, 0}},
    {InterfaceType::TYPE_MISCELLANEOUS_1,       {239, 1, 1}},
    {InterfaceType::TYPE_MISCELLANEOUS_2,       {239, 1, 2}},
    {InterfaceType::TYPE_MISCELLANEOUS_3,       {239, 2, 1}},
    {InterfaceType::TYPE_MISCELLANEOUS_4,       {239, 2, 2}},
    {InterfaceType::TYPE_VENDOR_SPECIFIC,       {255, -1, -1}}
};

const std::unordered_map<InterfaceType, std::vector<int32_t>> g_typeMap = {
    {InterfaceType::TYPE_AUDIO,                 {1, -1, -1}},
    {InterfaceType::TYPE_COMMUNICATION,         {2, -1, -1}},
    {InterfaceType::TYPE_HID,                   {3, -1, -1}},
    {InterfaceType::TYPE_PHYSICAL,              {5, -1, -1}},
    {InterfaceType::TYPE_IMAGE,                 {6, 1, 1}},
    {InterfaceType::TYPE_PRINTER,               {7, -1, -1}},
    {InterfaceType::TYPE_STORAGE,               {8, -1, -1}},
    {InterfaceType::TYPE_CDC_DATA,              {10, -1, -1}},
    {InterfaceType::TYPE_SMART_CARD,            {11, -1, -1}},
    {InterfaceType::TYPE_CONTENT_SECURTIY,      {13, 0, 0}},
    {InterfaceType::TYPE_VIDEO,                 {14, -1, -1}},
    {InterfaceType::TYPE_PERSONAL_HEALTHCARE,   {15, -1, -1}},
    {InterfaceType::TYPE_AVCONTROL,             {16, 1, 0}},
    {InterfaceType::TYPE_AVV_STREAMING,         {16, 2, 0}},
    {InterfaceType::TYPE_AVA_STREAMING,         {16, 3, 0}},
    {InterfaceType::TYPE_TYPEC_BRIDGE,          {18, 0, 0}},
    {InterfaceType::TYPE_BDP,                   {19, 0, 0}},
    {InterfaceType::TYPE_MCTP_MANA_1,           {20, 0, 1}},
    {InterfaceType::TYPE_MCTP_MANA_2,           {20, 0, 2}},
    {InterfaceType::TYPE_MCTP_HOST_1,           {20, 1, 1}},
    {InterfaceType::TYPE_MCTP_HOST_2,           {20, 1, 2}},
    {InterfaceType::TYPE_I3C,                   {60, 0, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_1,          {220, 1, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_2,          {220, 2, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_3,          {220, 2, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_4,          {220, 3, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_5,          {220, 3, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_6,          {220, 4, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_7,          {220, 4, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_8,          {220, 5, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_9,          {220, 5, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_10,         {220, 6, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_11,         {220, 6, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_12,         {220, 7, 0}},
    {InterfaceType::TYPE_DIAGNOSTIC_13,         {220, 7, 1}},
    {InterfaceType::TYPE_DIAGNOSTIC_14,         {220, 8, 0}},
    {InterfaceType::TYPE_WIRLESS_CONTROLLER_1,  {224, 1, 1}},
    {InterfaceType::TYPE_WIRLESS_CONTROLLER_2,  {224, 1, 2}},
    {InterfaceType::TYPE_WIRLESS_CONTROLLER_3,  {224, 1, 3}},
    {InterfaceType::TYPE_WIRLESS_CONTROLLER_4,  {224, 1, 4}},
    {InterfaceType::TYPE_WIRLESS_CONTROLLER_5,  {224, 2, 1}},
    {InterfaceType::TYPE_WIRLESS_CONTROLLER_6,  {224, 2, 2}},
    {InterfaceType::TYPE_WIRLESS_CONTROLLER_7,  {224, 2, 3}},
    {InterfaceType::TYPE_MISCELLANEOUS_1,       {239, 1, 1}},
    {InterfaceType::TYPE_MISCELLANEOUS_2,       {239, 1, 2}},
    {InterfaceType::TYPE_MISCELLANEOUS_5,       {239, 3, 1}},
    {InterfaceType::TYPE_MISCELLANEOUS_6,       {239, 4, 1}},
    {InterfaceType::TYPE_MISCELLANEOUS_7,       {239, 4, 2}},
    {InterfaceType::TYPE_MISCELLANEOUS_8,       {239, 4, 3}},
    {InterfaceType::TYPE_MISCELLANEOUS_9,       {239, 4, 4}},
    {InterfaceType::TYPE_MISCELLANEOUS_10,      {239, 4, 5}},
    {InterfaceType::TYPE_MISCELLANEOUS_11,      {239, 4, 6}},
    {InterfaceType::TYPE_MISCELLANEOUS_12,      {239, 4, 7}},
    {InterfaceType::TYPE_MISCELLANEOUS_13,      {239, 5, 0}},
    {InterfaceType::TYPE_MISCELLANEOUS_14,      {239, 5, 1}},
    {InterfaceType::TYPE_MISCELLANEOUS_15,      {239, 5, 2}},
    {InterfaceType::TYPE_MISCELLANEOUS_16,      {239, 6, 1}},
    {InterfaceType::TYPE_MISCELLANEOUS_17,      {239, 6, 2}},
    {InterfaceType::TYPE_MISCELLANEOUS_18,      {239, 7, 1}},
    {InterfaceType::TYPE_MISCELLANEOUS_19,      {239, 7, 2}},
    {InterfaceType::TYPE_APPLICATION_SPE_1,     {254, 1, 1}},
    {InterfaceType::TYPE_APPLICATION_SPE_2,     {254, 2, 0}},
    {InterfaceType::TYPE_APPLICATION_SPE_3,     {254, 3, 0}},
    {InterfaceType::TYPE_APPLICATION_SPE_4,     {254, 3, 1}},
    {InterfaceType::TYPE_VENDOR_SPECIFIC,       {255, -1, -1}}
};
} // namespace USB
} // namespace OHOS
#endif // USB_INTERFACE_TYPE_H
