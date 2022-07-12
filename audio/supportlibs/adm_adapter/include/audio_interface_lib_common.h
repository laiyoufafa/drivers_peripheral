/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef AUDIO_INTERFACE_LIB_COMMON_H
#define AUDIO_INTERFACE_LIB_COMMON_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "securec.h"
#include "audio_internal.h"
#include "osal_mem.h"
#include "hdf_io_service_if.h"
#include "hdf_sbuf.h"

#define SERVIC_NAME_MAX_LEN             32
#define AUDIO_MIN_DEVICENUM             1

#define AUDIODRV_CTL_ELEM_IFACE_MIXER 2 /* virtual mixer control */

#define AUDIO_WAIT_DELAY        (10 * 1000) // 10ms
#define AUDIO_CAP_WAIT_DELAY    (5 * 1000)  // 5ms


#define AUDIODRV_CTL_ACODEC_ENABLE  1
#define AUDIODRV_CTL_ACODEC_DISABLE 0
#define AUDIODRV_CTL_INTERNAL_ACODEC_ENABLE 1
#define AUDIODRV_CTL_EXTERN_ACODEC_ENABLE   2

#define AUDIODRV_CTL_INTER_CARD_STR "hdf_audio_codec_primary_dev0"
#define AUDIODRV_CTL_EXTN_CARD_STR  "hdf_audio_codec_primary_dev11"

enum AudioCriBuffStatus {
    CIR_BUFF_NORMAL    = -1,
    CIR_BUFF_FULL      = -2,
    CIR_BUFF_EMPTY     = -3,
};

struct AudioCtlElemId {
    const char *cardServiceName;
    const char *itemName; /* ASCII name of item */
    int32_t iface;
};

struct AudioCtlElemValue {
    struct AudioCtlElemId id;
    int32_t value[2];
};

struct AudioCtrlElemInfo {
    struct AudioCtlElemId id;
    uint32_t count; /* count of values */
    int32_t type;   /* R: value type - AUDIODRV_CTL_ELEM_IFACE_MIXER_* */
    int32_t min;    /* R: minimum value */
    int32_t max;    /* R: maximum value */
};

struct HdfIoService *HdfIoServiceBindName(const char *serviceName);
void AudioBufReplyRecycle(struct HdfSBuf *sBuf, struct HdfSBuf *reply);
int32_t AudioServiceDispatch(struct HdfIoService *service, int cmdId,
    struct HdfSBuf *sBuf, struct HdfSBuf *reply);
struct HdfSBuf *AudioObtainHdfSBuf(void);
int32_t AudioCtlGetVolThresholdRead(struct HdfSBuf *reply, struct AudioCtrlElemInfo *volThreshold);

#endif /* AUDIO_INTERFACE_LIB_COMMON_H */
