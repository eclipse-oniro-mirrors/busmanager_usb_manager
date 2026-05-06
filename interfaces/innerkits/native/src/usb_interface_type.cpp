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

#include "usb_interface_type.h"

namespace OHOS {
namespace USB {
bool UsbDeviceType::Marshalling(MessageParcel &parcel) const
{
    WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Int32, parcel, baseClass);
    WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Int32, parcel, subClass);
    WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Int32, parcel, protocol);
    WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Bool, parcel, isDeviceType);
    WRITE_PARCEL_AND_RETURN_FALSE_WHEN_FAIL(Bool, parcel, isDeviceTypeAllMatch);
    return true;
}

bool UsbDeviceType::Unmarshalling(MessageParcel &parcel, UsbDeviceType &usbDeviceType)
{
    return usbDeviceType.ReadFromParcel(parcel);
}

bool UsbDeviceType::ReadFromParcel(MessageParcel &parcel)
{
    baseClass = parcel.ReadInt32();
    subClass = parcel.ReadInt32();
    protocol = parcel.ReadInt32();
    isDeviceType = parcel.ReadBool();
    isDeviceTypeAllMatch = parcel.ReadBool();
    return true;
}
} // namespace USB
} // namespace OHOS