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

#include "codec_jpeg_callback.h"
#include <hdf_base.h>

#define HDF_LOG_TAG    codec_image_callback_service

CodecJpegCallbackService::CodecJpegCallbackService(std::shared_ptr<JpegDecoder> decoder)
{
    decoder_ = decoder;
}

int32_t CodecJpegCallbackService::OnImageEvent(int32_t error)
{
    decoder_->OnEvent(error);
    return HDF_SUCCESS;
}
