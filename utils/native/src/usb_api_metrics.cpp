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
#include "usb_errors.h"

#define TIME_1000 1000
#define UEC_MAX 31400007

namespace OHOS {
namespace USB {

UsbApiMetrics::UsbApiMetrics(std::string name)
: metricsName(name)
{
    errorCode = 0;
    gettimeofday(&startTime, nullptr);
}

UsbApiMetrics::~UsbApiMetrics()
{
#ifdef USB_MANAGER_METRICS_ENABLE
    std::string boolMetricsName = metricsName + ".Boolean";
    std::string enumMetricsName = metricsName + ".Enum";
    std::string timeMetricsName = metricsName + ".Time";
    if (errorCode == UEC_OK) {
        HISTOGRAM_BOOLEAN(boolMetricsName.c_str(), 1);
    } else {
        HISTOGRAM_BOOLEAN(boolMetricsName.c_str(), 0);
    }
    struct timeval endTime;
    gettimeofday(&endTime, nullptr);
    int32_t runTime = (int32_t)((endTime.tv_sec - startTime.tv_sec) * TIME_1000 +
        (endTime.tv_usec - startTime.tv_usec) / TIME_1000);
    HISTOGRAM_TIMES(timeMetricsName.c_str(), runTime);
    HISTOGRAM_ENUMERATION(enumMetricsName.c_str(), errorCode, UEC_MAX);
#endif
}

void UsbApiMetrics::SetErrorCode(int32_t error)
{
    errorCode = error;
}
}
}