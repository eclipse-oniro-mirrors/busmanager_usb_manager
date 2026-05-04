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
#ifndef USB_API_METRICS_H
#define USB_API_METRICS_H

#ifdef USB_MANAGER_METRICS_ENABLE
#include "histogram_plugin_macros.h"
#endif
#include <string>
#include <sys/time.h>

namespace OHOS {
namespace USB {

class UsbApiMetrics {
public:
    UsbApiMetrics(std::string name);
    ~UsbApiMetrics();
    void SetErrorCode(int32_t error);
private:
    std::string metricsName;
    struct timeval startTime;
    int32_t errorCode;
};
}
}

#endif