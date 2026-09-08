/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef USB_NAPI_ERRORS_H
#define USB_NAPI_ERRORS_H

#include <map>
#include <string_view>
#include "napi/native_api.h"

namespace OHOS {
namespace USB {
enum UsbJsErrCode : int32_t {
    OHEC_COMMON_PERMISSION_NOT_ALLOWED = 201,
    OHEC_COMMON_NORMAL_APP_NOT_ALLOWED = 202,
    OHEC_COMMON_PARAM_ERROR = 401,
    CAPABILITY_NOT_SUPPORT = 801,

    UEC_COMMON_BASE = 14400000,
    UEC_COMMON_HAS_NO_RIGHT = UEC_COMMON_BASE + 1,
    UEC_COMMON_HDC_NOT_ALLOWED = UEC_COMMON_BASE + 2,
    UEC_COMMON_PORTROLE_SWITCH_NOT_ALLOWED = UEC_COMMON_BASE + 3,
    UEC_COMMON_SERVICE_EXCEPTION = UEC_COMMON_BASE + 4,
    UEC_COMMON_RIGHT_DATABASE_ERROR = UEC_COMMON_BASE + 5,
    UEC_COMMON_FUNCTION_NOT_SUPPORT = UEC_COMMON_BASE + 6,

    UEC_ACCESSORY_BASE = UEC_COMMON_BASE + 1000,
    UEC_ACCESSORY_NOT_MATCH = UEC_ACCESSORY_BASE + 1,
    UEC_ACCESSORY_OPEN_FAILED = UEC_ACCESSORY_BASE + 2,
    UEC_ACCESSORY_CAN_NOT_REOPEN = UEC_ACCESSORY_BASE + 3,

    USB_SUBMIT_TRANSFER_RESOURCE_BUSY_ERROR = 14400007,
    USB_SUBMIT_TRANSFER_NO_DEVICE_ERROR = 14400008,
    USB_SUBMIT_TRANSFER_NO_MEM_ERROR =  14400009,
    USB_SUBMIT_TRANSFER_OTHER_ERROR = 14400010,
    USB_SUBMIT_TRANSFER_NOT_FOUND_ERROR = 14400011,
    USB_SUBMIT_TRANSFER_IO_ERROR = 14400012,
    USB_DEVICE_PIPE_CHECK_ERROR = 14400013,
    USB_INTERFACE_BUSY_ERROR = 14400014,
};

const std::map<int32_t, std::string_view> ERRCODE_MSG_MAP = {
    {OHEC_COMMON_PERMISSION_NOT_ALLOWED,
        "BusinessError 201:Permission verification failed. "
        "The application does not have the permission required to call the API."},
    {OHEC_COMMON_NORMAL_APP_NOT_ALLOWED, "BusinessError 202:Permission denied. Normal application uses system api."   },
    {OHEC_COMMON_PARAM_ERROR,       "BusinessError 401:Parameter error."                                         },
    {CAPABILITY_NOT_SUPPORT, "BusinessError 801:Capability not supported."},
    {UEC_COMMON_HAS_NO_RIGHT, "BusinessError 14400001:Permission denied."                                  },
    {UEC_COMMON_HDC_NOT_ALLOWED,    "BusinessError 14400002:Permission denied. The HDC is disabled by the system."},
    {UEC_COMMON_PORTROLE_SWITCH_NOT_ALLOWED,
     "BusinessError 14400003:Unsupported operation.The current device does not support port role switching."    },
    {UEC_COMMON_SERVICE_EXCEPTION,
     "BusinessError 14400004:Service exception. Possible causes:No accessory is plugged in."},
    {UEC_COMMON_RIGHT_DATABASE_ERROR,    "BusinessError 14400005:Database operation exception."},
    {UEC_COMMON_FUNCTION_NOT_SUPPORT,
        "BusinessError 14400006:Unsupported operation. The function is not supported."},
    {UEC_ACCESSORY_NOT_MATCH,    "BusinessError 14401001:The target USBAccessory not matched."},
    {UEC_ACCESSORY_OPEN_FAILED, "BusinessError 14401002:Failed to open the native accessory node."},
    {UEC_ACCESSORY_CAN_NOT_REOPEN,    "BusinessError 14401003:Cannot reopen the accessory."},

    {USB_SUBMIT_TRANSFER_RESOURCE_BUSY_ERROR, "BusinessError 14400007:Resource busy. Possible causes:"
                                              "1. The transfer has already been submitted."
                                              "2. The interface is claimed by another program or driver."},
    {USB_SUBMIT_TRANSFER_NO_DEVICE_ERROR, "BusinessError 14400008:No such device (it may have been disconnected)."},
    {USB_SUBMIT_TRANSFER_NO_MEM_ERROR,
        "BusinessError 14400009:Insufficient memory. Possible causes: 1. Malloc memory failed."},
    {USB_SUBMIT_TRANSFER_OTHER_ERROR, "BusinessError 14400010:Other USB error."},
    {USB_SUBMIT_TRANSFER_NOT_FOUND_ERROR,
        "BusinessError 14400011:The transfer is not in progress, or is already complete or cancelled."},
    {USB_SUBMIT_TRANSFER_IO_ERROR, "BusinessError 14400012:Transmission I/O error."},
    {USB_DEVICE_PIPE_CHECK_ERROR, "BusinessError 14400013:The USBDevicePipe validity check failed. Possible causes:"
                                  "1.The input parameters fail the validation check."
                                  "2.The call chain used to obtain the input parameters is not reasonable."},
    {USB_INTERFACE_BUSY_ERROR, "BusinessError 14400014:Interface busy. The interface is claimed by another program."},
};

void ThrowBusinessError(const napi_env &env, int32_t errCode, const std::string &errMsg);
napi_value CreateBusinessError(const napi_env &env, int32_t errCode, const std::string &errMsg);

#define USB_ASSERT_BASE(env, assertion, errCode, errMsg, retVal) \
    do {                                                         \
        if (!(assertion)) {                                      \
            USB_HILOGE(MODULE_USB_UTILS, #errMsg);                 \
            ThrowBusinessError((env), errCode, errMsg);          \
            return retVal;                                       \
        }                                                        \
    } while (0)

#define NOTHING
#define USB_ASSERT(env, assertion, errCode, errMsg) USB_ASSERT_BASE(env, assertion, errCode, errMsg, nullptr)
#define USB_ASSERT_RETURN_VOID(env, assertion, errCode, errMsg) \
    USB_ASSERT_BASE(env, assertion, errCode, errMsg, NOTHING)
#define USB_ASSERT_RETURN_FALSE(env, assertion, errCode, errMsg) USB_ASSERT_BASE(env, assertion, errCode, errMsg, false)
#define USB_ASSERT_RETURN_UNDEF(env, assertion, errCode, errMsg) \
    do {                                                         \
        napi_value obj = nullptr;                                \
        napi_get_undefined(env, &obj);                           \
        USB_ASSERT_BASE(env, assertion, errCode, errMsg, obj);   \
    } while (0)

#define NAPI_CHECK(env, theCall, loginfo)                                     \
    do {                                                                      \
        if ((theCall) != napi_ok) {                                           \
            USB_HILOGE(MODULE_USB_UTILS, "%{public}s " #loginfo " ", __func__); \
            napi_value obj = nullptr;                                         \
            napi_get_undefined(env, &obj);                                    \
            return obj;                                                       \
        }                                                                     \
    } while (0)

#define NAPI_CHECK_BASE(theCall, loginfo, retVal)                             \
    do {                                                                      \
        if ((theCall) != napi_ok) {                                           \
            USB_HILOGE(MODULE_USB_UTILS, "%{public}s " #loginfo " ", __func__); \
            return retVal;                                                    \
        }                                                                     \
    } while (0)

#define NAPI_CHECK_RETURN_VOID(theCall, loginfo)  NAPI_CHECK_BASE(theCall, loginfo, NOTHING)
#define NAPI_CHECK_RETURN_FALSE(theCall, loginfo) NAPI_CHECK_BASE(theCall, loginfo, false)
} // namespace USB
} // namespace OHOS
#endif // USB_NAPI_ERRORS_H
