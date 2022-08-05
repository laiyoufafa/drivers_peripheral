/*
 * Copyright (c) 2022 Shenzhen Kaihong DID Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "codec_service.h"
#include <errno.h>
#include <fcntl.h>
#include <securec.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include "ashmem_wrapper.h"
#ifndef CODEC_HAL_PASSTHROUGH
#include "codec_config_parser.h"
#endif
#include "hdf_log.h"
#include "osal_mem.h"

#define HDF_LOG_TAG codec_hdi_service
#define VIDEO_HARDWARE_ENCODER_INDEX 0
#define VIDEO_HARDWARE_DECODER_INDEX 1
#define AUDIO_HARDWARE_ENCODER_INDEX 4
#define AUDIO_HARDWARE_DECODER_INDEX 5

struct CodecInstance *g_codecInstance = NULL;
#ifdef CODEC_HAL_PASSTHROUGH
const CodecCallback *g_codecCallback = NULL;
UINTPTR g_userData;
#endif

static int32_t DefaultCbOnEvent(UINTPTR userData, EventType event, uint32_t length, int32_t eventData[])
{
#ifdef CODEC_HAL_PASSTHROUGH
    g_codecCallback->OnEvent(g_userData, event, length, eventData);
#endif
    return HDF_SUCCESS;
}

static int32_t DefaultCbInputBufferAvailable(UINTPTR userData, CodecBuffer *inBuf, int32_t *acquireFd)
{
    if (inBuf == NULL || inBuf->bufferCnt == 0) {
        HDF_LOGE("%{public}s: inBuf Nullpoint or buf not assigned", __func__);
        return HDF_FAILURE;
    }
    CodecBuffer *inputInfo = GetInputInfo(g_codecInstance, inBuf->bufferId);
    if (inputInfo == NULL || inputInfo->bufferCnt == 0) {
        HDF_LOGE("%{public}s: inputInfo Nullpoint or buf not assigned", __func__);
        return HDF_FAILURE;
    }
    inputInfo->buffer[0].buf = (intptr_t)GetFdById(g_codecInstance, inBuf->bufferId);
    inputInfo->buffer[0].type = BUFFER_TYPE_FD;
    inputInfo->buffer[0].length = 0;
    g_codecInstance->bufferManagerWrapper->PutUsedInputDataBuffer(g_codecInstance->bufferManagerWrapper, inputInfo);
#ifdef CODEC_HAL_PASSTHROUGH
    g_codecCallback->InputBufferAvailable(g_userData, inBuf, acquireFd);
#endif
    return HDF_SUCCESS;
}

static int32_t DefaultCbOutputBufferAvailable(UINTPTR userData, CodecBuffer *outBuf, int32_t *acquireFd)
{
    if (outBuf == NULL || outBuf->bufferCnt == 0) {
        HDF_LOGE("%{public}s: outBuf Nullpoint or buf not assigned", __func__);
        return HDF_FAILURE;
    }
    struct BufferManagerWrapper *bmWrapper = g_codecInstance->bufferManagerWrapper;
    CodecBuffer *outputInfo = GetOutputInfo(g_codecInstance, outBuf->bufferId);
    if (outputInfo == NULL || outputInfo->bufferCnt == 0) {
        HDF_LOGE("%{public}s: outputInfo Nullpoint or buf not assigned", __func__);
        return HDF_FAILURE;
    }
    CopyCodecBuffer(outputInfo, outBuf);
    outputInfo->buffer[0].buf = (intptr_t)GetFdById(g_codecInstance, outBuf->bufferId);
    outputInfo->buffer[0].type = BUFFER_TYPE_FD;
    bmWrapper->PutOutputDataBuffer(bmWrapper, outputInfo);
#ifdef CODEC_HAL_PASSTHROUGH
    g_codecCallback->OutputBufferAvailable(g_userData, outputInfo, acquireFd);
#endif
    // get a new OutputBuffer
    CodecBuffer *output = NULL;
    while (output == NULL && g_codecInstance->codecStatus == CODEC_STATUS_STARTED) {
        output = bmWrapper->GetUsedOutputDataBuffer(bmWrapper, QUEUE_TIME_OUT);
    }
    outBuf->buffer[0].type = BUFFER_TYPE_VIRTUAL;
    outBuf->bufferId = output->bufferId;
    outBuf->buffer[0].buf = (intptr_t)GetOutputShm(g_codecInstance, output->bufferId)->virAddr;

    return HDF_SUCCESS;
}

int32_t CodecInit()
{
    g_codecInstance = GetCodecInstance();
    if (g_codecInstance == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance is NULL!", __func__);
        return HDF_FAILURE;
    }
    int32_t ret = InitCodecInstance(g_codecInstance);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: InitCodecInstance failed!", __func__);
        return ret;
    }

    if (g_codecInstance->codecOemIface == NULL) {
        HDF_LOGE("%{public}s: oemIface is NULL!", __func__);
        return HDF_FAILURE;
    }
    return g_codecInstance->codecOemIface->CodecInit();
}

int32_t CodecDeinit()
{
    if (g_codecInstance == NULL || g_codecInstance->codecOemIface == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance or oemIface is NULL!", __func__);
        return HDF_FAILURE;
    }
    g_codecInstance->codecOemIface->CodecDeinit();
    int32_t ret = DestroyCodecInstance(g_codecInstance);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: CodecDeinit failed!", __func__);
        return ret;
    }
    g_codecInstance = NULL;
    return HDF_SUCCESS;
}

int32_t CodecEnumerateCapability(uint32_t index, CodecCapability *cap)
{
#ifndef CODEC_HAL_PASSTHROUGH
    int32_t loopIndex;
    uint32_t cursor = index;
    CodecCapablityGroup *group = NULL;
    if (cursor + 1 < cursor) {
        HDF_LOGE("%{public}s: the index out of bounds!", __func__);
        return HDF_FAILURE;
    }
    for (loopIndex = 0; loopIndex < CODEC_CAPABLITY_GROUP_NUM; loopIndex++) {
        group = GetCapablityGroup(loopIndex);
        if (group == NULL) {
            continue;
        }
        if (cursor + 1 <= (uint32_t)group->num) {
            *cap = group->capablitis[cursor];
            return HDF_SUCCESS;
        } else {
            cursor -= group->num;
        }
    }
#endif
    return HDF_FAILURE;
}

int32_t CodecGetCapability(AvCodecMime mime, CodecType type, uint32_t flags, CodecCapability *cap)
{
#ifndef CODEC_HAL_PASSTHROUGH
    int32_t groupIndex;
    int32_t capIndex;
    CodecCapablityGroup *group = NULL;
    CodecCapability *capItem;
    bool inputHardwareFlag = flags == 0;

    for (groupIndex = 0; groupIndex < CODEC_CAPABLITY_GROUP_NUM; groupIndex++) {
        group = GetCapablityGroup(groupIndex);
        if (group == NULL) {
            continue;
        }
        bool curHardwareFlag = (groupIndex == VIDEO_HARDWARE_ENCODER_INDEX)
            || (groupIndex == VIDEO_HARDWARE_DECODER_INDEX) || (groupIndex == AUDIO_HARDWARE_ENCODER_INDEX)
            || (groupIndex == AUDIO_HARDWARE_DECODER_INDEX);
        if (inputHardwareFlag != curHardwareFlag) {
            continue;
        }
        for (capIndex = 0; capIndex < group->num; capIndex++) {
            capItem = &group->capablitis[capIndex];
            if (mime == capItem->mime && type == capItem->type) {
                *cap = group->capablitis[capIndex];
                return HDF_SUCCESS;
            }
        }
    }
#endif
    return HDF_FAILURE;
}

int32_t CodecCreate(const char* name, CODEC_HANDLETYPE *handle)
{
    if (g_codecInstance == NULL || g_codecInstance->codecOemIface == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance or oemIface is NULL!", __func__);
        return HDF_FAILURE;
    }
    int32_t ret = g_codecInstance->codecOemIface->CodecCreate(name, handle);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    g_codecInstance->handle = *handle;
    HDF_LOGI("%{public}s codec created", __func__);
    return HDF_SUCCESS;
}

int32_t CodecDestroy(CODEC_HANDLETYPE handle)
{
    if (g_codecInstance == NULL || g_codecInstance->codecOemIface == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance or oemIface is NULL!", __func__);
        return HDF_FAILURE;
    }
    return g_codecInstance->codecOemIface->CodecDestroy(handle);
}

int32_t CodecSetPortMode(CODEC_HANDLETYPE handle, DirectionType direct, AllocateBufferMode mode, BufferType type)
{
    return HDF_SUCCESS;
}

int32_t CodecGetPortMode(CODEC_HANDLETYPE handle, DirectionType direct, AllocateBufferMode *mode, BufferType *type)
{
    return HDF_SUCCESS;
}

int32_t CodecSetParameter(CODEC_HANDLETYPE handle, const Param *params, int32_t paramCnt)
{
    if (g_codecInstance == NULL || g_codecInstance->codecOemIface == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance or oemIface is NULL!", __func__);
        return HDF_FAILURE;
    }
    if (params == NULL || paramCnt <= 0) {
        HDF_LOGE("%{public}s: params empty!", __func__);
        return HDF_FAILURE;
    }
    for (int32_t i = 0; i < paramCnt; i++) {
        if (params[i].key == KEY_CODEC_TYPE) {
            int32_t codecType = 0;
            memcpy_s(&codecType, sizeof(codecType), params[i].val, params[i].size);
            g_codecInstance->codecType = codecType;
        }
    }
    return g_codecInstance->codecOemIface->CodecSetParameter(handle, params, paramCnt);
}

int32_t CodecGetParameter(CODEC_HANDLETYPE handle, Param *params, int32_t paramCnt)
{
    if (g_codecInstance == NULL || g_codecInstance->codecOemIface == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance or oemIface is NULL!", __func__);
        return HDF_FAILURE;
    }
    if (params == NULL || paramCnt <= 0) {
        HDF_LOGE("%{public}s: params empty!", __func__);
        return HDF_FAILURE;
    }
    return g_codecInstance->codecOemIface->CodecGetParameter(handle, params, paramCnt);
}

int32_t CodecStart(CODEC_HANDLETYPE handle)
{
    if (g_codecInstance == NULL || g_codecInstance->codecOemIface == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance or oemIface is NULL!", __func__);
        return HDF_FAILURE;
    }
    if (!g_codecInstance->hasCallback) {
        g_codecInstance->defaultCb.OnEvent = DefaultCbOnEvent;
        g_codecInstance->defaultCb.InputBufferAvailable = DefaultCbInputBufferAvailable;
        g_codecInstance->defaultCb.OutputBufferAvailable = DefaultCbOutputBufferAvailable;
        g_codecInstance->codecOemIface->CodecSetCallback(handle, &(g_codecInstance->defaultCb), 0);
    }
    return RunCodecInstance(g_codecInstance);
}

int32_t CodecStop(CODEC_HANDLETYPE handle)
{
    if (g_codecInstance == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance is NULL!", __func__);
        return HDF_FAILURE;
    }
    return StopCodecInstance(g_codecInstance);
}

int32_t CodecFlush(CODEC_HANDLETYPE handle, DirectionType directType)
{
    if (g_codecInstance == NULL || g_codecInstance->codecOemIface == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance or oemIface is NULL!", __func__);
        return HDF_FAILURE;
    }
    return g_codecInstance->codecOemIface->CodecFlush(handle, directType);
}

int32_t CodecQueueInput(CODEC_HANDLETYPE handle, const CodecBuffer *inputData, uint32_t timeoutMs, int releaseFenceFd)
{
    if (g_codecInstance == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance is NULL!", __func__);
        return HDF_FAILURE;
    }

    if (g_codecInstance->codecStatus == CODEC_STATUS_IDLE) {
        uint32_t i;
        if (g_codecInstance->codecType == VIDEO_DECODER || g_codecInstance->codecType == AUDIO_DECODER ||
            g_codecInstance->codecType == VIDEO_ENCODER || g_codecInstance->codecType == AUDIO_ENCODER) {
            for (i = 0; i < inputData->bufferCnt; i++) {
                AddInputShm(g_codecInstance, &inputData->buffer[i], inputData->bufferId);
            }
        } else {
            HDF_LOGE("%{public}s: codecType invalid, queue input buffer failed!", __func__);
            return HDF_FAILURE;
        }
        CodecBuffer *dup = DupCodecBuffer(inputData);
        AddInputInfo(g_codecInstance, dup);
        g_codecInstance->bufferManagerWrapper->PutUsedInputDataBuffer(g_codecInstance->bufferManagerWrapper, dup);
        return HDF_SUCCESS;
    } else if (g_codecInstance->codecStatus == CODEC_STATUS_STARTED) {
        CodecBuffer *info = GetInputInfo(g_codecInstance, inputData->bufferId);
        CopyCodecBuffer(info, inputData);
        g_codecInstance->bufferManagerWrapper->PutInputDataBuffer(g_codecInstance->bufferManagerWrapper, info);
        return HDF_SUCCESS;
    }
    return HDF_SUCCESS;
}

int32_t CodecDequeueInput(CODEC_HANDLETYPE handle, uint32_t timeoutMs, int32_t *acquireFd, CodecBuffer *inputData)
{
    if (g_codecInstance == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance is NULL!", __func__);
        return HDF_FAILURE;
    }

    CodecBuffer *info = g_codecInstance->bufferManagerWrapper->GetUsedInputDataBuffer(
        g_codecInstance->bufferManagerWrapper, QUEUE_TIME_OUT);
    if (info != NULL) {
        *acquireFd = -1;
        CopyCodecBuffer(inputData, info);
    } else {
        return HDF_ERR_TIMEOUT;
    }
    
    return HDF_SUCCESS;
}

int32_t CodecQueueOutput(CODEC_HANDLETYPE handle, CodecBuffer *outInfo, uint32_t timeoutMs, int releaseFenceFd)
{
    if (g_codecInstance == NULL || g_codecInstance->bufferManagerWrapper == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance or buffermanager is NULL!", __func__);
        return HDF_FAILURE;
    }

    if (g_codecInstance->codecStatus == CODEC_STATUS_IDLE) {
        uint32_t i;
        if (g_codecInstance->codecType == VIDEO_DECODER || g_codecInstance->codecType == AUDIO_DECODER ||
            g_codecInstance->codecType == VIDEO_ENCODER || g_codecInstance->codecType == AUDIO_ENCODER) {
            for (i = 0; i < outInfo->bufferCnt; i++) {
                AddOutputShm(g_codecInstance, &outInfo->buffer[i], outInfo->bufferId);
            }
        } else {
            HDF_LOGE("%{public}s: codecType invalid, queue output buffer failed!", __func__);
            return HDF_FAILURE;
        }
        CodecBuffer *dup = DupCodecBuffer(outInfo);
        AddOutputInfo(g_codecInstance, dup);
        g_codecInstance->bufferManagerWrapper->PutUsedOutputDataBuffer(g_codecInstance->bufferManagerWrapper, dup);
        return HDF_SUCCESS;
    } else if (g_codecInstance->codecStatus == CODEC_STATUS_STARTED) {
        CodecBuffer *info = GetOutputInfo(g_codecInstance, outInfo->bufferId);
        CopyCodecBuffer(info, outInfo);
        info->buffer[0].length = 0;
        g_codecInstance->bufferManagerWrapper->PutUsedOutputDataBuffer(g_codecInstance->bufferManagerWrapper, info);
        return HDF_SUCCESS;
    } else if (g_codecInstance->codecStatus == CODEC_STATUS_STOPED) {
        CodecBuffer *dup = DupCodecBuffer(outInfo);
        g_codecInstance->bufferManagerWrapper->PutOutputDataBuffer(g_codecInstance->bufferManagerWrapper, dup);
    }
    return HDF_SUCCESS;
}

int32_t CodecDequeueOutput(CODEC_HANDLETYPE handle, uint32_t timeoutMs, int32_t *acquireFd, CodecBuffer *outInfo)
{
    if (g_codecInstance == NULL || g_codecInstance->bufferManagerWrapper == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance or buffermanager is NULL!", __func__);
        return HDF_FAILURE;
    }
    
    CodecBuffer *info = g_codecInstance->bufferManagerWrapper->GetOutputDataBuffer(
        g_codecInstance->bufferManagerWrapper, QUEUE_TIME_OUT);
    if (info != NULL) {
        *acquireFd = -1;
        CopyCodecBuffer(outInfo, info);
    } else {
        return HDF_ERR_TIMEOUT;
    }
    
    return HDF_SUCCESS;
}

int32_t CodecSetCallback(CODEC_HANDLETYPE handle, const CodecCallback *cb, UINTPTR instance)
{
    if (g_codecInstance == NULL || g_codecInstance->codecOemIface == NULL) {
        HDF_LOGE("%{public}s: g_codecInstance or oemIface is NULL!", __func__);
        return HDF_FAILURE;
    }
#ifndef CODEC_HAL_PASSTHROUGH
    int32_t ret = g_codecInstance->codecOemIface->CodecSetCallback(handle, cb, instance);
#else
    g_codecInstance->defaultCb.OnEvent = DefaultCbOnEvent;
    g_codecInstance->defaultCb.InputBufferAvailable = DefaultCbInputBufferAvailable;
    g_codecInstance->defaultCb.OutputBufferAvailable = DefaultCbOutputBufferAvailable;
    int32_t ret = g_codecInstance->codecOemIface->CodecSetCallback(handle, &(g_codecInstance->defaultCb), 0);
    g_codecCallback = cb;
    g_userData = instance;
#endif
    if (ret == HDF_SUCCESS) {
        g_codecInstance->hasCallback = true;
    }
    return ret;
}