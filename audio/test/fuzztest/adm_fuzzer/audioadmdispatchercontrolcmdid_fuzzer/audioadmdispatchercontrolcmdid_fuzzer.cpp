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
#include "audioadmdispatchercontrolcmdid_fuzzer.h"
#include "audio_hdi_common.h"
#include "audio_adm_common.h"

using namespace HMOS::Audio;
namespace OHOS {
namespace Audio {
    bool AudioAdmDispatcherControlCmdidFuzzTest(const uint8_t *data, size_t size)
    {
        bool result = false;
        struct HdfIoService *service = nullptr;
        struct HdfSBuf *writeBuf = nullptr;
        struct HdfSBuf *writeReply = nullptr;

        struct AudioCtlElemValue writeElemValue = {
            .id.cardServiceName = CARD_SEVICE_NAME.c_str(),
            .id.iface = AUDIODRV_CTL_ELEM_IFACE_GAIN,
            .id.itemName = "Mic Left Gain",
            .value[0] = 5,
        };

        service = HdfIoServiceBind(HDF_CONTROL_SERVICE.c_str());
        if (service == nullptr || service->dispatcher == nullptr) {
            return false;
        }

        writeBuf = HdfSbufObtainDefaultSize();
        if (writeBuf == nullptr) {
            HdfIoServiceRecycle(service);
            return false;
        }
        int32_t ret = WriteEleValueToBuf(writeBuf, writeElemValue);
        if (ret < 0) {
            return false;
        }
        int32_t cmdId = *(int32_t *)(data);
        ret = service->dispatcher->Dispatch(&service->object, cmdId, writeBuf, writeReply);
        if (ret == HDF_SUCCESS) {
            result = true;
        }
        HdfSbufRecycle(writeBuf);
        HdfIoServiceRecycle(service);
        return result;
    }
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Audio::AudioAdmDispatcherControlCmdidFuzzTest(data, size);
    return 0;
}