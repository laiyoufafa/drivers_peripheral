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
#include "audio_hdi_fuzzer_common.h"
#include "audiocreatecapturedesc_fuzzer.h"

using namespace OHOS::Audio;
namespace OHOS {
namespace Audio {
bool AudioCreatecaptureDescFuzzTest(const uint8_t *data, size_t size)
{
    bool result = false;
    TestAudioManager *manager = nullptr;
    int32_t ret = GetManager(manager);
    if (ret < 0 || manager == nullptr) {
        return false;
    }
    struct AudioAdapter *adapter = nullptr;
    struct AudioPort *capturePort = nullptr;
    ret = GetLoadAdapter(manager, &adapter, capturePort);
    if (ret < 0 || adapter == nullptr) {
        return false;
    }
    struct AudioSampleAttributes attrs = {};
    InitAttrs(attrs);
    struct AudioCapture *capture = nullptr;
    struct AudioDeviceDescriptor devDescFuzz {
        .portId = *(uint32_t *)data,
        .pins = *(AudioPortPin *)data,
        .desc = (char *)data,
    };
    ret = adapter->CreateCapture(adapter, &devDescFuzz, &attrs, &capture);
    if (ret == HDF_SUCCESS) {
        adapter->DestroyCapture(adapter, capture);
        result = true;
    }

    manager->UnloadAdapter(manager, adapter);
    return result;
}
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Audio::AudioCreatecaptureDescFuzzTest(data, size);
    return 0;
}