/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "usb_api_metrics.h"
#include <vector>

#define TIME_1000 1000
#define ERROR_CODE_BOUNDARY 29

namespace OHOS {
namespace USB {
enum MetricsErrCode : int32_t {
    METRICS_UEC_OK = 0,
    METRICS_OHEC_COMMON_PERMISSION_NOT_ALLOWED = 201,
    METRICS_OHEC_COMMON_NORMAL_APP_NOT_ALLOWED = 202,
    METRICS_OHEC_COMMON_PARAM_ERROR = 401,
    METRICS_CAPABILITY_NOT_SUPPORT = 801,
    METRICS_UEC_COMMON_HAS_NO_RIGHT = 14400001,
    METRICS_UEC_COMMON_HDC_NOT_ALLOWED = 14400002,
    METRICS_UEC_COMMON_PORTROLE_SWITCH_NOT_ALLOWED = 14400003,
    METRICS_UEC_COMMON_SERVICE_EXCEPTION = 14400004,
    METRICS_UEC_COMMON_RIGHT_DATABASE_ERROR = 14400005,
    METRICS_UEC_COMMON_FUNCTION_NOT_SUPPORT = 14400006,
    METRICS_USB_SUBMIT_TRANSFER_RESOURCE_BUSY_ERROR = 14400007,
    METRICS_USB_SUBMIT_TRANSFER_NO_DEVICE_ERROR = 14400008,
    METRICS_USB_SUBMIT_TRANSFER_NO_MEM_ERROR =  14400009,
    METRICS_USB_SUBMIT_TRANSFER_OTHER_ERROR = 14400010,
    METRICS_USB_SUBMIT_TRANSFER_NOT_FOUND_ERROR = 14400011,
    METRICS_USB_SUBMIT_TRANSFER_IO_ERROR = 14400012,
    METRICS_USB_DEVICE_PIPE_CHECK_ERROR = 14400013,
    METRICS_UEC_ACCESSORY_BASE = 14401000,
    METRICS_UEC_ACCESSORY_NOT_MATCH = 14401001,
    METRICS_UEC_ACCESSORY_OPEN_FAILED = 14401002,
    METRICS_UEC_ACCESSORY_CAN_NOT_REOPEN = 14401003,
    METRICS_SERIAL_SERVICE_ABNORMAL = 31400001,
    METRICS_SERIAL_INTERFACE_PERMISSION_DENIED = 31400002,
    METRICS_SERIAL_PORT_NOT_EXIST = 31400003,
    METRICS_SERIAL_PORT_OCCUPIED = 31400004,
    METRICS_SERIAL_PORT_NOT_OPEN = 31400005,
    METRICS_SERIAL_TIMED_OUT = 31400006,
    METRICS_SERIAL_IO_EXCEPTION = 31400007,
    METRICS_OTHER_ERROR = 99999999,
};

std::vector<int32_t> g_errorVector = {
    METRICS_UEC_OK,
    METRICS_OHEC_COMMON_PERMISSION_NOT_ALLOWED,
    METRICS_OHEC_COMMON_NORMAL_APP_NOT_ALLOWED,
    METRICS_OHEC_COMMON_PARAM_ERROR,
    METRICS_CAPABILITY_NOT_SUPPORT,
    METRICS_UEC_COMMON_HAS_NO_RIGHT,
    METRICS_UEC_COMMON_HDC_NOT_ALLOWED,
    METRICS_UEC_COMMON_PORTROLE_SWITCH_NOT_ALLOWED,
    METRICS_UEC_COMMON_SERVICE_EXCEPTION,
    METRICS_UEC_COMMON_RIGHT_DATABASE_ERROR,
    METRICS_UEC_COMMON_FUNCTION_NOT_SUPPORT,
    METRICS_USB_SUBMIT_TRANSFER_RESOURCE_BUSY_ERROR,
    METRICS_USB_SUBMIT_TRANSFER_NO_DEVICE_ERROR,
    METRICS_USB_SUBMIT_TRANSFER_NO_MEM_ERROR,
    METRICS_USB_SUBMIT_TRANSFER_OTHER_ERROR,
    METRICS_USB_SUBMIT_TRANSFER_NOT_FOUND_ERROR,
    METRICS_USB_SUBMIT_TRANSFER_IO_ERROR,
    METRICS_USB_DEVICE_PIPE_CHECK_ERROR,
    METRICS_UEC_ACCESSORY_BASE,
    METRICS_UEC_ACCESSORY_NOT_MATCH,
    METRICS_UEC_ACCESSORY_OPEN_FAILED,
    METRICS_UEC_ACCESSORY_CAN_NOT_REOPEN,
    METRICS_SERIAL_SERVICE_ABNORMAL,
    METRICS_SERIAL_INTERFACE_PERMISSION_DENIED,
    METRICS_SERIAL_PORT_NOT_EXIST,
    METRICS_SERIAL_PORT_OCCUPIED,
    METRICS_SERIAL_PORT_NOT_OPEN,
    METRICS_SERIAL_TIMED_OUT,
    METRICS_SERIAL_IO_EXCEPTION,
    METRICS_OTHER_ERROR
};

UsbApiMetrics::UsbApiMetrics(std::string name)
    : metricsName_(name)
{
    errorCode_ = 0;
    gettimeofday(&startTime_, nullptr);
}

UsbApiMetrics::~UsbApiMetrics()
{
#ifdef USB_MANAGER_METRICS_ENABLE
    std::string boolMetricsName = metricsName_ + ".Boolean";
    std::string enumMetricsName = metricsName_ + ".Enum";
    std::string timeMetricsName = metricsName_ + ".Time";
    if (errorCode_ == METRICS_UEC_OK) {
        HISTOGRAM_BOOLEAN(boolMetricsName.c_str(), 1);
    } else {
        HISTOGRAM_BOOLEAN(boolMetricsName.c_str(), 0);
    }
    struct timeval endTime;
    gettimeofday(&endTime, nullptr);
    int32_t runTime = (int32_t)((endTime.tv_sec - startTime_.tv_sec) * TIME_1000 +
        (endTime.tv_usec - startTime_.tv_usec) / TIME_1000);
    HISTOGRAM_TIMES(timeMetricsName.c_str(), runTime);
    HISTOGRAM_ENUMERATION(enumMetricsName.c_str(), errorCode_, ERROR_CODE_BOUNDARY);
#endif
}

void UsbApiMetrics::SetErrorCode(int32_t error)
{
    for (int i = 0; i < ERROR_CODE_BOUNDARY; i++) {
        if (error == g_errorVector[i]) {
            errorCode_ = i;
            return;
        }
    }
    errorCode_ = ERROR_CODE_BOUNDARY;
}
}
}