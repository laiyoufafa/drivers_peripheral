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

#include "audiounloadadapteradapter_fuzzer.h"
#include "audio_hdi_fuzzer_common.h"
using namespace OHOS::Audio;
namespace OHOS {
namespace Audio {
bool AudioUnloadadapterAdapterFuzzTest(const uint8_t *data, size_t size)
{
    TestAudioManager *manager = nullptr;
    int32_t ret = GetManager(manager);
    if (ret < 0 || manager == nullptr) {
        return false;
    }

    struct AudioAdapter *adapterFuzz = (struct AudioAdapter *)data;
    manager->UnloadAdapter(manager, adapterFuzz);
    return true;
}
}
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Audio::AudioUnloadadapterAdapterFuzzTest(data, size);
    return 0;
}