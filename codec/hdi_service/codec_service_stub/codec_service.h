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

#ifndef CODEC_SERVICE_H
#define CODEC_SERVICE_H

#include "hdf_base.h"
#include "codec_type.h"
#include "codec_instance.h"

#ifdef __cplusplus
extern "C"
{
#endif

int32_t CodecInit();
int32_t CodecDeinit();
int32_t CodecEnumerateCapbility(uint32_t index, CodecCapbility *cap);
int32_t CodecGetCapbility(AvCodecMime mime, CodecType type, uint32_t flags, CodecCapbility *cap);
int32_t CodecCreate(const char* name, const Param *attr, int len, CODEC_HANDLETYPE *handle);
int32_t CodecDestroy(CODEC_HANDLETYPE handle);
int32_t CodecSetPortMode(CODEC_HANDLETYPE handle, DirectionType type, BufferMode mode);
int32_t CodecSetParameter(CODEC_HANDLETYPE handle, const Param *params, int paramCnt);
int32_t CodecGetParameter(CODEC_HANDLETYPE handle, Param *params, int paramCnt);
int32_t CodecStart(CODEC_HANDLETYPE handle);
int32_t CodecStop(CODEC_HANDLETYPE handle);
int32_t CodecFlush(CODEC_HANDLETYPE handle, DirectionType directType);
int32_t CodecQueueInput(CODEC_HANDLETYPE handle, const InputInfo *inputData, uint32_t timeoutMs);
int32_t CodecDequeInput(CODEC_HANDLETYPE handle, uint32_t timeoutMs, InputInfo *inputData);
int32_t CodecQueueOutput(CODEC_HANDLETYPE handle, OutputInfo *outInfo, uint32_t timeoutMs, int releaseFenceFd);
int32_t CodecDequeueOutput(CODEC_HANDLETYPE handle, uint32_t timeoutMs, int *acquireFd, OutputInfo *outInfo);
int32_t CodecSetCallback(CODEC_HANDLETYPE handle, const CodecCallback *cb, UINTPTR instance);

#ifdef __cplusplus
}
#endif
#endif  // CODEC_SERVICE_H