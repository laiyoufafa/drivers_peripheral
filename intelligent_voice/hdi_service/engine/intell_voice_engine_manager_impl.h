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

#ifndef HDI_DEVICE_INTELL_VOICE_MANAGER_IMPL_H
#define HDI_DEVICE_INTELL_VOICE_MANAGER_IMPL_H

#include <map>
#include <mutex>
#include <string>

#include "v1_0/intell_voice_engine_types.h"
#include "v1_0/iintell_voice_engine_manager.h"
#include "i_engine.h"

namespace OHOS {
namespace IntelligentVoice {
namespace Engine {
using OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager;
using OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineAdapter;
using OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterType;
using OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterDescriptor;

using GetEngineManagerHalInstFunc = IEngineManager *(*)();

struct IntellVoiceEngineManagerPriv {
    void *handle { nullptr };
    GetEngineManagerHalInstFunc getEngineManagerHalInst { nullptr };
};

class IntellVoiceEngineManagerImpl : public IIntellVoiceEngineManager {
public:
    IntellVoiceEngineManagerImpl();
    ~IntellVoiceEngineManagerImpl();

    int32_t GetAdapterDescriptors(std::vector<IntellVoiceEngineAdapterDescriptor>& descs) override;
    int32_t CreateAdapter(const IntellVoiceEngineAdapterDescriptor& descriptor,
        sptr<IIntellVoiceEngineAdapter>& adapter) override;
    int32_t ReleaseAdapter(const IntellVoiceEngineAdapterDescriptor& descriptor) override;

private:
    int32_t LoadVendorLib();
    void UnloadVendorLib();

private:
    std::map<IntellVoiceEngineAdapterType, sptr<IIntellVoiceEngineAdapter>> adapters_;
    std::mutex mutex_ {};
    IntellVoiceEngineManagerPriv engineManagerPriv_;
    IEngineManager *inst_ = nullptr;
};
}
}
}
#endif