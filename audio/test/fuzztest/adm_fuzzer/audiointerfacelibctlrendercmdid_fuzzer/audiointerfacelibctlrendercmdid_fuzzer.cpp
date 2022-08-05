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

#include "audiointerfacelibctlrendercmdid_fuzzer.h"
#include "audio_adm_fuzzer_common.h"

using namespace OHOS::Audio;
namespace OHOS {
namespace Audio {
    bool AudioInterfacelibctlrenderCmdidFuzzTest(const uint8_t *data, size_t size)
    {
        bool result = false;
        char resolvedPath[] = HDF_LIBRARY_FULL_PATH("libhdi_audio_interface_lib_render");
        void *PtrHandle = dlopen(resolvedPath, RTLD_LAZY);
        if (PtrHandle == nullptr) {
            HDF_LOGE("%{public}s: dlopen failed \n", __func__);
            return false;
        }
        struct DevHandle *(*BindServiceRender)(const char *) = nullptr;
        int32_t (*InterfaceLibCtlRender)(struct DevHandle *, int32_t, struct AudioHwRenderParam *) = nullptr;
        BindServiceRender = (struct DevHandle *(*)(const char *))dlsym(PtrHandle, "AudioBindServiceRender");
        if (BindServiceRender == nullptr) {
            HDF_LOGE("%{public}s: dlsym AudioBindServiceRender failed \n", __func__);
            dlclose(PtrHandle);
            return false;
        }
        struct DevHandle *handle = BindServiceRender(BIND_CONTROL.c_str());
        if (handle == nullptr) {
            HDF_LOGE("%{public}s: BindServiceRender failed \n", __func__);
            dlclose(PtrHandle);
            return false;
        }
        InterfaceLibCtlRender = (int32_t (*)(struct DevHandle *, int,
            struct AudioHwRenderParam *))dlsym(PtrHandle, "AudioInterfaceLibCtlRender");
        if (InterfaceLibCtlRender == nullptr) {
            HDF_LOGE("%{public}s: dlsym AudioInterfaceLibCtlRender failed \n", __func__);
            dlclose(PtrHandle);
            return false;
        }
        struct AudioHwRender *hwRender = nullptr;
        int32_t ret = BindServiceAndHwRender(hwRender);
        hwRender->renderParam.renderMode.hwInfo.card = AUDIO_SERVICE_IN;
        int32_t cmdId = *(int32_t *)(data);
        ret = InterfaceLibCtlRender(handle, cmdId, &hwRender->renderParam);
        if (ret == HDF_SUCCESS) {
            result = true;
        }
        free(hwRender);
        void (*CloseService)(const struct DevHandle *) = nullptr;
        CloseService = (void (*)(const struct DevHandle *))dlsym(PtrHandle, "AudioCloseServiceRender");
        if (CloseService == nullptr) {
            HDF_LOGE("%{public}s:dlsym AudioCloseServiceRender failed \n", __func__);
            dlclose(PtrHandle);
            return false;
        }
        CloseService(handle);
        dlclose(PtrHandle);
        return result;
    }
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Audio::AudioInterfacelibctlrenderCmdidFuzzTest(data, size);
    return 0;
}