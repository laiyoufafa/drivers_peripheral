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

#include <gtest/gtest.h>
#include <climits>
#include "osal_mem.h"
#include "v1_0/iaudio_capture.h"
#include "v1_0/iaudio_manager.h"

using namespace std;
using namespace testing::ext;
namespace {
static const uint32_t MAX_AUDIO_ADAPTER_NUM = 5;
const int DEFAULT_BUFFER_SIZE = 16384;
const float HALF_OF_MAX_VOLUME = 0.5;
const int TEST_SAMPLE_RATE_MASK_48000 = 48000;
const int TEST_CHANNEL_COUNT = 2;

class AudioUtCaptureTest : public testing::Test {
public:
    struct IAudioManager *manager_ = nullptr;;
    struct IAudioAdapter *adapter_ = nullptr;
    struct IAudioCapture *capture_ = nullptr;
    uint32_t captureId_ = 0;
    char *devDescriptorName_ = nullptr;
    struct AudioAdapterDescriptor *adapterDescs_ = nullptr;
    virtual void SetUp();
    virtual void TearDown();
    uint64_t GetCaptureBufferSize();
    void InitCaptureDevDesc(struct AudioDeviceDescriptor &devDesc);
    void InitCaptureAttrs(struct AudioSampleAttributes &attrs);
    void FreeAdapterElements(struct AudioAdapterDescriptor *dataBlock, bool freeSelf);
    void ReleaseAllAdapterDescs(struct AudioAdapterDescriptor **descs, uint32_t descsLen);
};

/* common method for capture ceate */
uint64_t AudioUtCaptureTest::GetCaptureBufferSize()
{
    int32_t ret = HDF_SUCCESS;
    uint64_t frameSize = 0;
    uint64_t frameCount = 0;
    uint64_t bufferSize = 0;
    
    if (capture_ == nullptr) {
        return DEFAULT_BUFFER_SIZE;
    }

    ret = capture_->GetFrameSize(capture_, &frameSize);
    if (ret != HDF_SUCCESS) {
        return DEFAULT_BUFFER_SIZE;
    }

    ret = capture_->GetFrameCount(capture_, &frameCount);
    if (ret != HDF_SUCCESS) {
        return DEFAULT_BUFFER_SIZE;
    }

    bufferSize = frameCount * frameSize;
    if (bufferSize == 0) {
        bufferSize = DEFAULT_BUFFER_SIZE;
    }

    return bufferSize;
}

void AudioUtCaptureTest::InitCaptureDevDesc(struct AudioDeviceDescriptor &devDesc)
{
    devDesc.pins = (enum AudioPortPin)PIN_IN_MIC;
    devDescriptorName_ = strdup("cardname");
    devDesc.desc = devDescriptorName_;

    ASSERT_NE(adapterDescs_, nullptr);
    ASSERT_NE(adapterDescs_->ports, nullptr);
    for (uint32_t index = 0; index < adapterDescs_->portsLen; index++) {
        if (adapterDescs_->ports[index].dir == PORT_IN) {
            devDesc.portId = adapterDescs_->ports[index].portId;
            return;
        }
    }
}

void AudioUtCaptureTest::InitCaptureAttrs(struct AudioSampleAttributes &attrs)
{
    attrs.format = AUDIO_FORMAT_TYPE_PCM_16_BIT;
    attrs.channelCount = TEST_CHANNEL_COUNT;
    attrs.sampleRate = TEST_SAMPLE_RATE_MASK_48000;
    attrs.interleaved = 0;
    attrs.type = AUDIO_IN_MEDIA;
    attrs.period = 4 * 1024;
    attrs.frameSize = 16 * 2 / 8;
    attrs.isBigEndian = false;
    attrs.isSignedData = true;
    attrs.startThreshold = 4 * 1024 / (AUDIO_FORMAT_TYPE_PCM_16_BIT * attrs.channelCount / 8);
    attrs.stopThreshold = INT_MAX;
    attrs.silenceThreshold = 1024 * 16;
}

void AudioUtCaptureTest::FreeAdapterElements(struct AudioAdapterDescriptor *dataBlock, bool freeSelf)
{
    if (dataBlock == nullptr) {
        return;
    }

    if (dataBlock->adapterName != nullptr) {
        OsalMemFree(dataBlock->adapterName);
        dataBlock->adapterName = nullptr;
    }

    if (dataBlock->ports != nullptr) {
        OsalMemFree(dataBlock->ports);
    }

    if (freeSelf) {
        OsalMemFree(dataBlock);
    }
}

void AudioUtCaptureTest::ReleaseAllAdapterDescs(struct AudioAdapterDescriptor **descs, uint32_t descsLen)
{
    if ((descsLen > 0) && (descs != nullptr) && ((*descs) != nullptr)) {
        for (uint32_t i = 0; i < descsLen; i++) {
            FreeAdapterElements(&(*descs)[i], false);
        }
        OsalMemFree(*descs);
        *descs = nullptr;
    }
}

void AudioUtCaptureTest::SetUp()
{
    uint32_t size = MAX_AUDIO_ADAPTER_NUM;
    struct AudioDeviceDescriptor devDesc = {};
    struct AudioSampleAttributes attrs = {};

    manager_ = IAudioManagerGet(false);
    ASSERT_NE(manager_, nullptr);

    adapterDescs_ = (struct AudioAdapterDescriptor *)OsalMemCalloc(
        sizeof(struct AudioAdapterDescriptor) * (MAX_AUDIO_ADAPTER_NUM));
    ASSERT_NE(adapterDescs_, nullptr);

    EXPECT_EQ(HDF_SUCCESS, manager_->GetAllAdapters(manager_, adapterDescs_, &size));
    if (size > MAX_AUDIO_ADAPTER_NUM) {
        ReleaseAllAdapterDescs(&adapterDescs_, MAX_AUDIO_ADAPTER_NUM);
        ASSERT_LT(size, MAX_AUDIO_ADAPTER_NUM);
    }

    EXPECT_EQ(HDF_SUCCESS, manager_->LoadAdapter(manager_, &adapterDescs_[0], &adapter_));
    if (adapter_ == nullptr) {
        ReleaseAllAdapterDescs(&adapterDescs_, MAX_AUDIO_ADAPTER_NUM);
        EXPECT_NE(adapter_, nullptr);
    }

    InitCaptureDevDesc(devDesc);
    InitCaptureAttrs(attrs);
    EXPECT_EQ(HDF_SUCCESS, adapter_->CreateCapture(adapter_, &devDesc, &attrs, &capture_, &captureId_));
    if (capture_ == nullptr) {
        (void)manager_->UnloadAdapter(manager_, adapterDescs_[0].adapterName);
        ReleaseAllAdapterDescs(&adapterDescs_, MAX_AUDIO_ADAPTER_NUM);
    }
    ASSERT_NE(capture_, nullptr);
}

void AudioUtCaptureTest::TearDown()
{
    ASSERT_NE(devDescriptorName_, nullptr);
    free(devDescriptorName_);    

    ASSERT_NE(capture_, nullptr);
    EXPECT_EQ(HDF_SUCCESS, adapter_->DestroyCapture(adapter_, captureId_));

    ASSERT_NE(manager_, nullptr);
    EXPECT_EQ(HDF_SUCCESS, manager_->UnloadAdapter(manager_, adapterDescs_[0].adapterName));
    ReleaseAllAdapterDescs(&adapterDescs_, MAX_AUDIO_ADAPTER_NUM);

    IAudioManagerRelease(manager_, false);
}

/* capture frame cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureFrame001, TestSize.Level1)
{
    uint32_t frameLen = (uint64_t)GetCaptureBufferSize();
    uint64_t requestBytes = frameLen;
    ASSERT_NE(capture_->CaptureFrame, nullptr);

    int32_t ret = capture_->Start(capture_);
    EXPECT_EQ(ret, HDF_SUCCESS);

    int8_t *frame = (int8_t *)calloc(1, frameLen);
    EXPECT_NE(nullptr, frame);

    ret = capture_->CaptureFrame(capture_, frame, &frameLen, &requestBytes);
    EXPECT_EQ(ret, HDF_SUCCESS);
    capture_->Stop(capture_);

    if (frame != nullptr) {
        free(frame);
        frame = nullptr;
    }
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureFrameExceptions001, TestSize.Level1)
{
    uint32_t frameLen = (uint64_t)GetCaptureBufferSize();
    uint64_t requestBytes = frameLen;
    ASSERT_NE(capture_->CaptureFrame, nullptr);

    int32_t ret = capture_->Start(capture_);
    EXPECT_EQ(ret, HDF_SUCCESS);

    int8_t *frame = (int8_t *)calloc(1, frameLen);
    EXPECT_NE(nullptr, frame);

    EXPECT_NE(HDF_SUCCESS, capture_->CaptureFrame(nullptr, nullptr, nullptr, nullptr));
    EXPECT_NE(HDF_SUCCESS, capture_->CaptureFrame(capture_, frame, nullptr, nullptr));
    EXPECT_NE(HDF_SUCCESS, capture_->CaptureFrame(capture_, frame, &frameLen, nullptr));
    EXPECT_NE(HDF_SUCCESS, capture_->CaptureFrame(nullptr, frame, &frameLen, &requestBytes));
}

/* capture getposition cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioGetCapturePosition001, TestSize.Level1)
{
    uint64_t frames;
    struct AudioTimeStamp time;
    uint32_t frameLen = (uint64_t)GetCaptureBufferSize();
    uint64_t requestBytes = frameLen;
    ASSERT_NE(capture_->CaptureFrame, nullptr);
    ASSERT_NE(capture_->GetCapturePosition, nullptr);

    int32_t ret = capture_->Start(capture_);
    EXPECT_EQ(ret, HDF_SUCCESS);

    int8_t *frame = (int8_t *)calloc(1, frameLen);
    EXPECT_NE(nullptr, frame);

    ret = capture_->CaptureFrame(capture_, frame, &frameLen, &requestBytes);
    EXPECT_EQ(ret, HDF_SUCCESS);

    ret = capture_->GetCapturePosition(capture_, &frames, &time);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);

    capture_->Stop(capture_);
    if (frame != nullptr) {
        free(frame);
        frame = nullptr;
    }
}

HWTEST_F(AudioUtCaptureTest, HdfAudioGetCapturePositionExceptions001, TestSize.Level1)
{
    int32_t ret;
    uint64_t frames;
    struct AudioTimeStamp time;
    ASSERT_NE(capture_->GetCapturePosition, nullptr);

    ret = capture_->GetCapturePosition(capture_, &frames, &time);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);

    ret = capture_->GetCapturePosition(capture_, nullptr, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);

    ret = capture_->GetCapturePosition(capture_, &frames, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/**
 * @brief from here starts the control tests
 * 
 */

/* capture start cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureStart001, TestSize.Level1)
{
    ASSERT_NE(capture_->Start, nullptr);

    int32_t ret = capture_->Start(capture_);
    EXPECT_EQ(ret, HDF_SUCCESS);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureException001, TestSize.Level1)
{
    ASSERT_NE(capture_->Start, nullptr);

    int32_t ret = capture_->Start(nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture stop cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureStop001, TestSize.Level1)
{
    ASSERT_NE(capture_->Start, nullptr);
    ASSERT_NE(capture_->Stop, nullptr);

    int32_t ret = capture_->Start(capture_);
    EXPECT_EQ(ret, HDF_SUCCESS);

    ret = capture_->Stop(capture_);
    EXPECT_EQ(ret, HDF_SUCCESS);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureStopException001, TestSize.Level1)
{
    ASSERT_NE(capture_->Stop, nullptr);

    int32_t ret = capture_->Stop(capture_);
    EXPECT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureStopException002, TestSize.Level1)
{
    ASSERT_NE(capture_->Stop, nullptr);

    int32_t ret = capture_->Stop(nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture pause cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCapturePause001, TestSize.Level1)
{
    ASSERT_NE(capture_->Pause, nullptr);
    ASSERT_NE(capture_->Start, nullptr);

    int32_t ret = capture_->Start(capture_);
    EXPECT_EQ(ret, HDF_SUCCESS);

    ret = capture_->Pause(capture_);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT || HDF_ERR_INVALID_PARAM);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCapturePauseException001, TestSize.Level1)
{
    ASSERT_NE(capture_->Pause, nullptr);

    int32_t ret = capture_->Pause(nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCapturePauseException002, TestSize.Level1)
{
    ASSERT_NE(capture_->Pause, nullptr);
    ASSERT_NE(capture_->Start, nullptr);

    int32_t ret = capture_->Start(capture_);
    EXPECT_EQ(ret, HDF_SUCCESS);

    ret = capture_->Pause(capture_);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);

    ret = capture_->Pause(capture_);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);
}

/* capture resume cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureResume001, TestSize.Level1)
{
    ASSERT_NE(capture_->Pause, nullptr);
    ASSERT_NE(capture_->Resume, nullptr);
    ASSERT_NE(capture_->Start, nullptr);
    ASSERT_NE(capture_->Stop, nullptr);

    int32_t ret = capture_->Start(capture_);
    ASSERT_EQ(ret, HDF_SUCCESS);

    ret = capture_->Pause(capture_);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);

    ret = capture_->Resume(capture_);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);

    ret = capture_->Stop(capture_);
    ASSERT_EQ(ret, HDF_SUCCESS);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureResumeException001, TestSize.Level1)
{
    ASSERT_NE(capture_->Resume, nullptr);

    int32_t ret = capture_->Resume(capture_);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureResumeException002, TestSize.Level1)
{
    ASSERT_NE(capture_->Resume, nullptr);

    int32_t ret = capture_->Resume(nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture flush cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureFlush001, TestSize.Level1)
{
    ASSERT_NE(capture_->Flush, nullptr);

    int32_t ret = capture_->Flush(capture_);
    EXPECT_EQ(HDF_ERR_NOT_SUPPORT, ret);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureFlushException001, TestSize.Level1)
{
    ASSERT_NE(capture_->Flush, nullptr);

    int32_t ret = capture_->Flush(nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture TurnStandbyMode cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureTurnStandbyMode001, TestSize.Level1)
{
    ASSERT_NE(capture_->TurnStandbyMode, nullptr);
    ASSERT_NE(capture_->Start, nullptr);

    int32_t ret = capture_->Start(capture_);
    EXPECT_EQ(ret, HDF_SUCCESS);

    ret = capture_->TurnStandbyMode(capture_);
    EXPECT_EQ(ret, HDF_SUCCESS);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureTurnStandbyModeException001, TestSize.Level1)
{
    ASSERT_NE(capture_->TurnStandbyMode, nullptr);

    int32_t ret = capture_->TurnStandbyMode(nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture AudioDevDump cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureAudioDevDump001, TestSize.Level1)
{
    ASSERT_NE(capture_->AudioDevDump, nullptr);

    int32_t range = 4;
    char pathBuf[] = "./CaptureDump.log";

    FILE *file = fopen(pathBuf, "wb+");
    ASSERT_NE(nullptr, file);
    int fd = fileno(file);
    if (fd == -1) {
        fclose(file);
        ASSERT_NE(fd, -1);
    }

    int32_t ret = capture_->AudioDevDump(capture_, range, fd);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);
    fclose(file);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureAudioDevDumpExption001, TestSize.Level1)
{
    ASSERT_NE(capture_->AudioDevDump, nullptr);
    int32_t range = 4;

    int32_t ret = capture_->AudioDevDump(nullptr, range, -1);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/**
 * @brief here starts the volume test cases
 * 
 */
/* capture SetMute cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureSetMute001, TestSize.Level1)
{
    bool isSupport = false;
    ASSERT_NE(capture_->SetMute, nullptr);
    ASSERT_NE(capture_->GetMute, nullptr);

    int32_t ret = capture_->SetMute(capture_, isSupport);
    if (ret == HDF_SUCCESS) {
        ret = capture_->GetMute(capture_, &isSupport);
        ASSERT_EQ(isSupport, false);
    } else if (ret == HDF_ERR_NOT_SUPPORT) {
        ASSERT_TRUE(true);
    } else {
        ASSERT_TRUE(false);
    }

    isSupport = true;
    ret = capture_->SetMute(capture_, isSupport);
    if (ret == HDF_SUCCESS) {
        ret = capture_->GetMute(capture_, &isSupport);
        ASSERT_EQ(isSupport, true);
    } else if (ret == HDF_ERR_NOT_SUPPORT) {
        ASSERT_TRUE(true);
    } else {
        ASSERT_TRUE(false);
    }
}

// set twice
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureSetMuteException001, TestSize.Level1)
{
    bool isSupport = true;
    ASSERT_NE(capture_->SetMute, nullptr);
    ASSERT_NE(capture_->GetMute, nullptr);

    int32_t ret = capture_->SetMute(capture_, isSupport);
    if (ret == HDF_SUCCESS) {
        ret = capture_->GetMute(capture_, &isSupport);
        ASSERT_EQ(isSupport, true);
    } else if (ret == HDF_ERR_NOT_SUPPORT) {
        ASSERT_TRUE(true);
    } else {
        ASSERT_TRUE(false);
    }

    ret = capture_->SetMute(capture_, isSupport);
    if (ret == HDF_SUCCESS) {
        ret = capture_->GetMute(capture_, &isSupport);
        ASSERT_EQ(isSupport, true);
    } else if (ret == HDF_ERR_NOT_SUPPORT) {
        ASSERT_TRUE(true);
    } else {
        ASSERT_TRUE(false);
    }
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureSetMuteException002, TestSize.Level1)
{
    EXPECT_NE(capture_->SetMute, nullptr);

    int32_t ret = capture_->SetMute(nullptr, true);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture GetMute cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetMute001, TestSize.Level1)
{
    bool isSupport = true;
    EXPECT_NE(capture_->GetMute, nullptr);

    int32_t ret = capture_->GetMute(capture_, &isSupport);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetMuteException001, TestSize.Level1)
{
    EXPECT_NE(capture_->GetMute, nullptr);

    int32_t ret = capture_->GetMute(nullptr, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);

    ret = capture_->GetMute(capture_, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture SetVolume cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureSetVolume001, TestSize.Level1)
{
    float volume = 0.0;
    EXPECT_NE(capture_->SetVolume, nullptr);
    EXPECT_NE(capture_->GetVolume, nullptr);

    int32_t ret = capture_->SetVolume(capture_, HALF_OF_MAX_VOLUME);
    if (ret == HDF_SUCCESS) {
        ret = capture_->GetVolume(capture_, &volume);
        ASSERT_EQ(volume, HALF_OF_MAX_VOLUME);
    } else if (ret == HDF_ERR_NOT_SUPPORT) {
        ASSERT_TRUE(true);
    } else {
        ASSERT_TRUE(false);
    }
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureSetVolumeException001, TestSize.Level1)
{
    float exceptionVolume = 2.0;
    EXPECT_NE(capture_->SetVolume, nullptr);

    int32_t ret = capture_->SetVolume(capture_, exceptionVolume);
    EXPECT_NE(ret, HDF_SUCCESS);

    exceptionVolume = -3.0;
    ret = capture_->SetVolume(capture_, exceptionVolume);
    EXPECT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureSetVolumeException002, TestSize.Level1)
{
    EXPECT_NE(capture_->SetVolume, nullptr);

    int32_t ret = capture_->SetVolume(nullptr, HALF_OF_MAX_VOLUME);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture GetVolume cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetVolume001, TestSize.Level1)
{
    float volume = 0.0;
    EXPECT_NE(capture_->GetVolume, nullptr);

    int32_t ret = capture_->GetVolume(capture_, &volume);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetVolumeException001, TestSize.Level1)
{
    float volume = 0.0;
    EXPECT_NE(capture_->GetVolume, nullptr);

    int32_t ret = capture_->GetVolume(nullptr, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);

    ret = capture_->GetVolume(capture_, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);

    ret = capture_->GetVolume(nullptr, &volume);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture GetGainThreshold cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetGainThreshold001, TestSize.Level1)
{
    float bottom = 0;
    float top = 0;
    EXPECT_NE(capture_->GetGainThreshold, nullptr);

    int32_t ret = capture_->GetGainThreshold(capture_, &bottom, &top);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetGainThresholdException002, TestSize.Level1)
{
    float bottom = 0;
    float top = 0;
    EXPECT_NE(capture_->GetGainThreshold, nullptr);

    int32_t ret = capture_->GetGainThreshold(nullptr, &bottom, &top);
    EXPECT_NE(ret, HDF_SUCCESS);

    ret = capture_->GetGainThreshold(nullptr, nullptr, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);

    ret = capture_->GetGainThreshold(capture_, nullptr, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/**
 * @brief here starts the attributes cases
 * 
 */
/* capture GetSampleAttributes cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetSampleAttributes001, TestSize.Level1)
{
    struct AudioSampleAttributes attrs = {};
    EXPECT_NE(capture_->GetSampleAttributes, nullptr);

    int32_t ret = capture_->GetSampleAttributes(capture_, &attrs);
    EXPECT_EQ(ret, HDF_SUCCESS);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetSampleAttributesException001, TestSize.Level1)
{
    struct AudioSampleAttributes attrs = {};
    EXPECT_NE(capture_->GetSampleAttributes, nullptr);

    int32_t ret = capture_->GetSampleAttributes(nullptr, &attrs);
    EXPECT_NE(ret, HDF_SUCCESS);

    ret = capture_->GetSampleAttributes(capture_, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureSetSampleAttributesException001, TestSize.Level1)
{
    struct AudioSampleAttributes attrs = {
        .format = AUDIO_FORMAT_TYPE_PCM_16_BIT,
        .channelCount = TEST_CHANNEL_COUNT,
        .sampleRate = TEST_SAMPLE_RATE_MASK_48000,
    };
    EXPECT_NE(capture_->SetSampleAttributes, nullptr);

    int32_t ret = capture_->SetSampleAttributes(capture_, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);

    ret = capture_->SetSampleAttributes(nullptr, &attrs);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture GetCurrentChannelId cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetCurrentChannelId001, TestSize.Level1)
{
    int32_t ret = HDF_SUCCESS;
    uint32_t channelId = 0;
    EXPECT_NE(capture_->GetCurrentChannelId, nullptr);

    ret = capture_->GetCurrentChannelId(capture_, &channelId);
    EXPECT_EQ(ret, HDF_SUCCESS);
    EXPECT_EQ(TEST_CHANNEL_COUNT, channelId);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetCurrentChannelIdException001, TestSize.Level1)
{
    int32_t ret = HDF_SUCCESS;
    uint32_t channelId = 0;
    EXPECT_NE(capture_->GetCurrentChannelId, nullptr);

    ret = capture_->GetCurrentChannelId(capture_, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);

    ret = capture_->GetCurrentChannelId(nullptr, &channelId);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture SetExtraParams cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureSetExtraParams001, TestSize.Level1)
{
    EXPECT_NE(capture_->SetExtraParams, nullptr);
    EXPECT_NE(capture_->GetExtraParams, nullptr);

    char kvList[] = "attr-route=1;attr-format=32;attr-channels=2;attr-frame-count=82;attr-sampling-rate=48000";
    char keyValueListReply[256] = {};
    uint32_t listLenth = 256;
    size_t index = 1;

    int32_t ret = capture_->SetExtraParams(capture_, kvList);
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_NOT_SUPPORT);
    
    ret = capture_->GetExtraParams(capture_, keyValueListReply, listLenth);
    // the vendor can not supply this method，one it not supply returns HDF_ERR_INVALID_PARAM
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_INVALID_PARAM);

    std::string strGetValue = keyValueListReply;
    size_t indexAttr = strGetValue.find("attr-frame-count");
    size_t indexFlag = strGetValue.rfind(";");

    if (indexAttr != string::npos && indexFlag != string::npos) {
        strGetValue.replace(indexAttr, indexFlag - indexAttr + index, "");
    }

}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureSetExtraParamsException001, TestSize.Level1)
{
    ASSERT_NE(capture_->SetExtraParams, nullptr);

    int32_t ret = capture_->SetExtraParams(nullptr, nullptr);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/* capture GetExtraParams cases */
HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetExtraParams001, TestSize.Level1)
{
    ASSERT_NE(capture_->GetExtraParams, nullptr);
    char keyValueListReply[256] = {};
    uint32_t listLenth = 256;

    int32_t ret = capture_->GetExtraParams(capture_, keyValueListReply, listLenth);
    // the vendor can not supply this method，one it not supply returns HDF_ERR_INVALID_PARAM
    ASSERT_TRUE(ret == HDF_SUCCESS || ret == HDF_ERR_INVALID_PARAM);
}

HWTEST_F(AudioUtCaptureTest, HdfAudioCaptureGetExtraParamsException001, TestSize.Level1)
{
    ASSERT_NE(capture_->GetExtraParams, nullptr);
    char keyValueListReply[256] = {};
    uint32_t listLenth = 256;

    int32_t ret = capture_->GetExtraParams(nullptr, keyValueListReply, listLenth);
    EXPECT_NE(ret, HDF_SUCCESS);

    ret = capture_->GetExtraParams(capture_, nullptr, listLenth);
    EXPECT_NE(ret, HDF_SUCCESS);
}

/**
 * @brief here starts the attributes cases
 * 
 */

} // end of name space