/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "sensor_callback_vdi.h"

namespace OHOS {
namespace HDI {
namespace Sensor {
namespace V1_0 {

int32_t SensorCallbackVdi::OnDataEventVdi(const HdfSensorEventsVdi& eventVdi)
{
    struct HdfSensorEvents event;

    if (sensorCallback_ == nullptr) {
        HDF_LOGE("%{public}s sensorCallback_ is NULL", __func__);
        return HDF_FAILURE;
    }

    event.sensorId = eventVdi.sensorId;
    event.version = eventVdi.version;
    event.timestamp = eventVdi.timestamp;
    event.option = eventVdi.option;
    event.mode = eventVdi.mode;
    event.data = eventVdi.data;
    event.dataLen = eventVdi.dataLen;

    int32_t ret = sensorCallback_->OnDataEvent(event);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%{public}s OnDataEvent failed, error code is %{public}d", __func__, ret);
    }

    return ret;
}

sptr<IRemoteObject> SensorCallbackVdi::HandleCallbackDeath()
{
    sptr<IRemoteObject> remote = OHOS::HDI::hdi_objcast<ISensorCallback>(sensorCallback_);

    return remote;
}
} // V1_0
} // Sensor
} // HDI
} // OHOS