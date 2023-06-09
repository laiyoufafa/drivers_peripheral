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

#include "codeccreatecomponent_fuzzer.h"
#include "codec_callback_type_stub.h"
#include "codec_component_type.h"
#include "codec_component_manager.h"

#include <osal_mem.h>
#include <hdf_log.h>

namespace OHOS {
namespace Codec {
    bool CodecCreateComponent(const uint8_t* data, size_t size)
    {
        bool result = false;
        struct CodecComponentManager *manager = nullptr;
        struct CodecComponentType *component = nullptr;
        CodecCallbackType* callback = CodecCallbackTypeStubGetInstance();

        manager = GetCodecComponentManager();
        if (manager == nullptr) {
            HDF_LOGE("%{public}s: GetCodecComponentManager failed\n", __func__);
            return false;
        }

        int32_t ret = manager->CreateComponent(&component, (char*)data, (void *)data, sizeof((int32_t)data), callback);
        if (ret == HDF_SUCCESS) {
            HDF_LOGI("%{public}s: CreateComponent succeed\n", __func__);
            result = true;
        }

        ret = manager->DestoryComponent(component);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%{public}s: DestoryComponent failed\n", __func__);
            return false;
        }
        CodecComponentManagerRelease();

        return result;
    }
} // namespace codec
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::Codec::CodecCreateComponent(data, size);
    return 0;
}