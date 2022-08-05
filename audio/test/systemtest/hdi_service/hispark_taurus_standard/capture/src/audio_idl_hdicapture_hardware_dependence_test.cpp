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

#include "hdf_remote_adapter_if.h"
#include "hdi_service_common.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Audio;

namespace {
class AudioIdlHdiCaptureHardwareDependenceTest : public testing::Test {
public:
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    struct AudioAdapter *adapter = nullptr;
    struct AudioCapture *capture = nullptr;
    static TestAudioManager *(*GetAudioManager)(const char *);
    static TestAudioManager *manager;
    static void *handle;
    static void (*AudioManagerRelease)(struct AudioManager *);
    static void (*AudioAdapterRelease)(struct AudioAdapter *);
    static void (*AudioCaptureRelease)(struct AudioCapture *);
    void ReleaseCaptureSource(void);
};

using THREAD_FUNC = void *(*)(void *);

TestAudioManager *(*AudioIdlHdiCaptureHardwareDependenceTest::GetAudioManager)(const char *) = nullptr;
TestAudioManager *AudioIdlHdiCaptureHardwareDependenceTest::manager = nullptr;
void *AudioIdlHdiCaptureHardwareDependenceTest::handle = nullptr;
void (*AudioIdlHdiCaptureHardwareDependenceTest::AudioManagerRelease)(struct AudioManager *) = nullptr;
void (*AudioIdlHdiCaptureHardwareDependenceTest::AudioAdapterRelease)(struct AudioAdapter *) = nullptr;
void (*AudioIdlHdiCaptureHardwareDependenceTest::AudioCaptureRelease)(struct AudioCapture *) = nullptr;

void AudioIdlHdiCaptureHardwareDependenceTest::SetUpTestCase(void)
{
    char absPath[PATH_MAX] = {0};
    char *path = realpath(RESOLVED_PATH.c_str(), absPath);
    ASSERT_NE(nullptr, path);
    handle = dlopen(absPath, RTLD_LAZY);
    ASSERT_NE(nullptr, handle);
    GetAudioManager = (TestAudioManager *(*)(const char *))(dlsym(handle, FUNCTION_NAME.c_str()));
    ASSERT_NE(nullptr, GetAudioManager);
    (void)HdfRemoteGetCallingPid();
    manager = GetAudioManager(IDL_SERVER_NAME.c_str());
    ASSERT_NE(nullptr, manager);
    AudioManagerRelease = (void (*)(struct AudioManager *))(dlsym(handle, "AudioManagerRelease"));
    ASSERT_NE(nullptr, AudioManagerRelease);
    AudioAdapterRelease = (void (*)(struct AudioAdapter *))(dlsym(handle, "AudioAdapterRelease"));
    ASSERT_NE(nullptr, AudioAdapterRelease);
    AudioCaptureRelease = (void (*)(struct AudioCapture *))(dlsym(handle, "AudioCaptureRelease"));
    ASSERT_NE(nullptr, AudioCaptureRelease);
}

void AudioIdlHdiCaptureHardwareDependenceTest::TearDownTestCase(void)
{
    if (AudioManagerRelease !=nullptr) {
        AudioManagerRelease(manager);
        manager = nullptr;
    }
    if (GetAudioManager != nullptr) {
        GetAudioManager = nullptr;
    }
    if (handle != nullptr) {
        dlclose(handle);
        handle = nullptr;
    }
}

void AudioIdlHdiCaptureHardwareDependenceTest::SetUp(void)
{
    int32_t ret;
    ASSERT_NE(nullptr, manager);
    ret = AudioCreateCapture(manager, PIN_IN_MIC, ADAPTER_NAME, &adapter, &capture);
    ASSERT_EQ(HDF_SUCCESS, ret);
}

void AudioIdlHdiCaptureHardwareDependenceTest::TearDown(void)
{
    ReleaseCaptureSource();
}

void AudioIdlHdiCaptureHardwareDependenceTest::ReleaseCaptureSource(void)
{
    if (capture != nullptr && AudioCaptureRelease != nullptr) {
        adapter->DestroyCapture(adapter);
        AudioCaptureRelease(capture);
        capture = nullptr;
    }
    if (adapter != nullptr && AudioAdapterRelease != nullptr) {
        manager->UnloadAdapter(manager, ADAPTER_NAME.c_str());
        AudioAdapterRelease(adapter);
        adapter = nullptr;
    }
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_001
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT;
*    attrs.sampleRate = SAMPLE_RATE_8000;
*    attrs.channelCount = 1;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_001, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_8000);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_8000, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_002
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_24_BIT;
*    attrs.sampleRate = SAMPLE_RATE_11025;
*    attrs.channelCount = 2;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_002, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_11025);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_11025, attrsValue.sampleRate);
    EXPECT_EQ(DOUBLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_003
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT;
*    attrs.sampleRate = SAMPLE_RATE_22050;
*    attrs.channelCount = 1;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_003, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_22050);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_22050, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_004
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_24_BIT;
*    attrs.sampleRate = SAMPLE_RATE_32000;
*    attrs.channelCount = 2;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_004, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_32000);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_32000, attrsValue.sampleRate);
    EXPECT_EQ(DOUBLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_005
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT;
*    attrs.sampleRate = SAMPLE_RATE_44100;
*    attrs.channelCount = 1;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_005, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_44100);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_44100, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_006
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_COMMUNICATION;
*    attrs.format = AUDIO_FORMAT_PCM_24_BIT;
*    attrs.sampleRate = SAMPLE_RATE_48000;
*    attrs.channelCount = 2;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_006, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_48000);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_48000, attrsValue.sampleRate);
    EXPECT_EQ(DOUBLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_008
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT;
*    attrs.sampleRate = SAMPLE_RATE_12000;
*    attrs.channelCount = 1;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_008, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_12000);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_12000, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_009
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_24_BIT;
*    attrs.sampleRate = SAMPLE_RATE_16000;
*    attrs.channelCount = 1;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_009, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_16000);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_16000, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_010
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT;
*    attrs.sampleRate = SAMPLE_RATE_24000;
*    attrs.channelCount = 2;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_010, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_24000);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_24000, attrsValue.sampleRate);
    EXPECT_EQ(DOUBLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_011
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT;
*    attrs.sampleRate = SAMPLE_RATE_64000;
*    attrs.channelCount = 1;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_011, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_64000);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_64000, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_012
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_24_BIT;
*    attrs.sampleRate = SAMPLE_RATE_96000;
*    attrs.channelCount = 1;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_012, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_96000);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_96000, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via illegal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_013
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16;
*    attrs.sampleRate = 0xFFFFFFFFu;
*    attrs.channelCount = 2;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_013, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, DOUBLE_CHANNEL_COUNT, 0xFFFFFFFFu);
    ret = capture->SetSampleAttributes(capture, &attrs);
    EXPECT_EQ(HDF_ERR_NOT_SUPPORT, ret);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via illegal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_014
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_8/32_BIT/AAC_MAIN;
*    attrs.sampleRate = SAMPLE_RATE_8000/SAMPLE_RATE_11025/SAMPLE_RATE_22050;
*    attrs.channelCount = 1/2;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_014, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs1 = {};
    struct AudioSampleAttributes attrs2 = {};
    struct AudioSampleAttributes attrs3 = {};
    ASSERT_NE(nullptr, capture);

    InitAttrsUpdate(attrs1, AUDIO_FORMAT_PCM_8_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_8000);
    ret = capture->SetSampleAttributes(capture, &attrs1);
    EXPECT_EQ(HDF_FAILURE, ret);

    InitAttrsUpdate(attrs2, AUDIO_FORMAT_PCM_32_BIT, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_11025);
    ret = capture->SetSampleAttributes(capture, &attrs2);
#ifdef ALSA_LIB_MODE
    EXPECT_EQ(HDF_SUCCESS, ret);
#else
    EXPECT_EQ(HDF_FAILURE, ret);
#endif
    InitAttrsUpdate(attrs3, AUDIO_FORMAT_AAC_MAIN, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_22050);
    ret = capture->SetSampleAttributes(capture, &attrs3);
    EXPECT_EQ(HDF_FAILURE, ret);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via illegal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_015
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_AAC_LC/LD/ELD;
*    attrs.sampleRate = SAMPLE_RATE_32000/SAMPLE_RATE_44100/SAMPLE_RATE_48000;
*    attrs.channelCount = 1/2;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_015, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs1 = {};
    struct AudioSampleAttributes attrs2 = {};
    struct AudioSampleAttributes attrs3 = {};
    ASSERT_NE(nullptr, capture);

    InitAttrsUpdate(attrs1, AUDIO_FORMAT_AAC_LC, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_32000);
    ret = capture->SetSampleAttributes(capture, &attrs1);
    EXPECT_EQ(HDF_FAILURE, ret);

    InitAttrsUpdate(attrs2, AUDIO_FORMAT_AAC_LD, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_44100);
    ret = capture->SetSampleAttributes(capture, &attrs2);
    EXPECT_EQ(HDF_FAILURE, ret);

    InitAttrsUpdate(attrs3, AUDIO_FORMAT_AAC_ELD, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_48000);
    ret = capture->SetSampleAttributes(capture, &attrs3);
    EXPECT_EQ(HDF_FAILURE, ret);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via illegal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_016
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_AAC_HE_V1/V2
*    attrs.sampleRate = SAMPLE_RATE_8000/SAMPLE_RATE_44100;
*    attrs.channelCount = 1/2;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_016, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs1 = {};
    struct AudioSampleAttributes attrs2 = {};
    ASSERT_NE(nullptr, capture);

    InitAttrsUpdate(attrs1, AUDIO_FORMAT_AAC_HE_V1, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_8000);
    ret = capture->SetSampleAttributes(capture, &attrs1);
    EXPECT_EQ(HDF_FAILURE, ret);

    InitAttrsUpdate(attrs2, AUDIO_FORMAT_AAC_HE_V2, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_44100);
    ret = capture->SetSampleAttributes(capture, &attrs2);
    EXPECT_EQ(HDF_FAILURE, ret);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via illegal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_017
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT
*    attrs.sampleRate = SAMPLE_RATE_8000;
*    attrs.channelCount = 5;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_017, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    ASSERT_NE(nullptr, capture);
    uint32_t channelCount = 5;
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, channelCount, SAMPLE_RATE_8000);
    ret = capture->SetSampleAttributes(capture, &attrs);
    EXPECT_EQ(HDF_FAILURE, ret);
}
#ifndef ALSA_LIB_MODE
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via illegal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_018
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT
*    attrs.sampleRate = SAMPLE_RATE_8000;
*    attrs.channelCount = 2;
*    silenceThreshold = 32*1024;
* @tc.author: ZENG LIFENG
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_018, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    ASSERT_NE(nullptr, capture);
    uint32_t silenceThreshold = 32*1024;
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_8000, silenceThreshold);
    ret = capture->SetSampleAttributes(capture, &attrs);
    EXPECT_EQ(HDF_FAILURE, ret);
}
/**
* @tc.name  Test AudioCaptureSetSampleAttributes API via illegal input.
* @tc.number  SUB_Audio_HDI_CaptureSetSampleAttributes_019
* @tc.desc  Test AudioCaptureSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT
*    attrs.sampleRate = SAMPLE_RATE_8000;
*    attrs.channelCount = 2;
*    silenceThreshold = 2*1024;
* @tc.author: ZENG LIFENG
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureSetSampleAttributes_019, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    ASSERT_NE(nullptr, capture);
    uint32_t silenceThreshold = 2*1024;
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, 2, SAMPLE_RATE_8000, silenceThreshold);
    ret = capture->SetSampleAttributes(capture, &attrs);
    EXPECT_EQ(HDF_FAILURE, ret);
}
#endif
/**
* @tc.name  Test AudioCaptureGetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_CaptureGetSampleAttributes_001
* @tc.desc  Test AudioCaptureGetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT;
*    attrs.sampleRate = SAMPLE_RATE_8000;
*    attrs.channelCount = 1;
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetSampleAttributes_001, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    ret = capture->GetSampleAttributes(capture, &attrsValue);
    EXPECT_EQ(HDF_SUCCESS, ret);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_32000);
    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_32000, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);
}
/**
* @tc.name  Test CaptureGetFrameSize API via define format to different values
* @tc.number  SUB_Audio_HDI_CaptureGetFrameSize_004
* @tc.desc  Test CaptureGetFrameSize interface,return 0 if get framesize define format as different values
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetFrameSize_004, TestSize.Level1)
{
    int32_t ret;
    uint64_t size = 0;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_48000);

    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_48000, attrsValue.sampleRate);
    EXPECT_EQ(DOUBLE_CHANNEL_COUNT, attrsValue.channelCount);

    ret = capture->GetFrameSize(capture, &size);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT(size, INITIAL_VALUE);
}
/**
* @tc.name  Test CaptureGetFrameSize API via define sampleRate to different values
* @tc.number  SUB_Audio_HDI_CaptureGetFrameSize_005
* @tc.desc  Test CaptureGetFrameSize interface,return 0 if get framesize define sampleRate as different values
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetFrameSize_005, TestSize.Level1)
{
    int32_t ret;
    uint64_t size = 0;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_48000);

    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_48000, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);

    ret = capture->GetFrameSize(capture, &size);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT(size, INITIAL_VALUE);
}
/**
* @tc.name  Test CaptureGetFrameSize API via define channelCount to different values
* @tc.number  SUB_Audio_HDI_CaptureGetFrameSize_006
* @tc.desc  Test CaptureGetFrameSize interface,return 0 if get framesize define channelCount as different values
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetFrameSize_006, TestSize.Level1)
{
    int32_t ret;
    uint64_t size = 0;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_44100);

    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_44100, attrsValue.sampleRate);
    EXPECT_EQ(DOUBLE_CHANNEL_COUNT, attrsValue.channelCount);

    ret = capture->GetFrameSize(capture, &size);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT(size, INITIAL_VALUE);
}
/**
* @tc.name  Test CaptureGetFrameSize API via define sampleRate to different value
* @tc.number  SUB_Audio_HDI_CaptureGetFrameSize_007
* @tc.desc  Test CaptureGetFrameSize interface,return 0 if get framesize define sampleRate as different values
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetFrameSize_007, TestSize.Level1)
{
    int32_t ret;
    uint64_t size = 0;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_48000);

    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_48000, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);

    ret = capture->GetFrameSize(capture, &size);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT(size, INITIAL_VALUE);
}
/**
* @tc.name  Test CaptureGetFrameCount API via define channelCount to different value
* @tc.number  SUB_Audio_HDI_CaptureGetFrameCount_005
* @tc.desc  Test CaptureGetFrameCount interface,return 0 if get framesize define channelCount as different values
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetFrameCount_005, TestSize.Level1)
{
    int32_t ret;
    uint64_t count = 0;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_8000);

    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_8000, attrsValue.sampleRate);
    EXPECT_EQ(DOUBLE_CHANNEL_COUNT, attrsValue.channelCount);

    ret = AudioCaptureStartAndOneFrame(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetFrameCount(capture, &count);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT(count, INITIAL_VALUE);

    ret = capture->Stop(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
}
/**
* @tc.name  Test CaptureGetFrameCount API via define format to different value
* @tc.number  SUB_Audio_HDI_CaptureGetFrameCount_006
* @tc.desc  Test CaptureGetFrameCount interface,return 0 if get framesize define format as different values
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetFrameCount_006, TestSize.Level1)
{
    int32_t ret;
    uint64_t count = 0;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, DOUBLE_CHANNEL_COUNT, SAMPLE_RATE_8000);

    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_8000, attrsValue.sampleRate);
    EXPECT_EQ(DOUBLE_CHANNEL_COUNT, attrsValue.channelCount);

    ret = AudioCaptureStartAndOneFrame(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetFrameCount(capture, &count);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT(count, INITIAL_VALUE);

    ret = capture->Stop(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
}
/**
* @tc.name  Test CaptureGetFrameCount API via define channelCount to different value
* @tc.number  SUB_Audio_HDI_CaptureGetFrameCount_007
* @tc.desc  Test CaptureGetFrameCount interface,return 0 if get framesize define channelCount to different values
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetFrameCount_007, TestSize.Level1)
{
    int32_t ret;
    uint64_t count = 0;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_44100);

    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_44100, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);

    ret = AudioCaptureStartAndOneFrame(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetFrameCount(capture, &count);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT(count, INITIAL_VALUE);

    ret = capture->Stop(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
}
/**
* @tc.name  Test CaptureGetFrameCount API via define format to different value
* @tc.number  SUB_Audio_HDI_CaptureGetFrameCount_008
* @tc.desc  Test CaptureGetFrameCount interface,return 0 if get framesize define format as different values
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetFrameCount_008, TestSize.Level1)
{
    int32_t ret;
    uint64_t count = 0;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_32000);

    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_32000, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);

    ret = AudioCaptureStartAndOneFrame(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetFrameCount(capture, &count);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT(count, INITIAL_VALUE);

    ret = capture->Stop(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
}
/**
* @tc.name  Test GetCurrentChannelId API via getting channelId to 1 and set channelCount to 1
* @tc.number  SUB_Audio_HDI_CaptureGetCurrentChannelId_002
* @tc.desc  Test GetCurrentChannelId interface,return 0 if get channelId to 1 and set channelCount to 1
* @tc.author: ZengLifeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetCurrentChannelId_002, TestSize.Level1)
{
    int32_t ret;
    uint32_t channelId = 0;
    uint32_t channelIdExp = 1;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, capture);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, SINGLE_CHANNEL_COUNT, SAMPLE_RATE_48000);

    ret = AudioCaptureSetGetSampleAttributes(attrs, attrsValue, capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);

    ret = capture->GetCurrentChannelId(capture, &channelId);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(channelIdExp, channelId);
}
/**
* @tc.name  Test GetMmapPosition API via SetSampleAttributes and Getting position is normal.
* @tc.number  SUB_Audio_HDI_CaptureGetMmapPosition_002
* @tc.desc  Test GetMmapPosition interface,return 0 if Getting position successfully.
* @tc.author: ZengLiFeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetMmapPosition_002, TestSize.Level1)
{
    int32_t ret;
    uint64_t frames = 0;
    int64_t timeExp = 0;
    ASSERT_NE(nullptr, capture);
    struct PrepareAudioPara audiopara = {
        .capture = capture, .portType = PORT_IN, .adapterName = ADAPTER_NAME.c_str(), .pins = PIN_IN_MIC,
        .path = AUDIO_LOW_LATENCY_CAPTURE_FILE.c_str()
    };

    InitAttrs(audiopara.attrs);
    audiopara.attrs.format = AUDIO_FORMAT_PCM_24_BIT;
    audiopara.attrs.channelCount = 1;
    ret = audiopara.capture->SetSampleAttributes(audiopara.capture, &(audiopara.attrs));
    EXPECT_EQ(HDF_SUCCESS, ret);

    ret = pthread_create(&audiopara.tids, NULL, (THREAD_FUNC)RecordMapAudio, &audiopara);
    ASSERT_EQ(HDF_SUCCESS, ret);

    void *result = nullptr;
    pthread_join(audiopara.tids, &result);
    EXPECT_EQ(HDF_SUCCESS, (intptr_t)result);

    ret = audiopara.capture->GetMmapPosition(audiopara.capture, &frames, &(audiopara.time));
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT((audiopara.time.tvSec) * SECTONSEC + (audiopara.time.tvNSec), timeExp);
    EXPECT_GT(frames, INITIAL_VALUE);

    audiopara.capture->Stop(audiopara.capture);
}
/*
* @tc.name  Test GetCapturePosition API via define format to AUDIO_FORMAT_PCM_16_BIT
* @tc.number  SUB_Audio_HDI_CaptureGetCapturePosition_009
* @tc.desc  Test GetCapturePosition interface,return 0 if get framesize define format to AUDIO_FORMAT_PCM_16_BIT
* @tc.author: ZengLiFeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetCapturePosition_009, TestSize.Level1)
{
    int32_t ret;
    uint64_t frames = 0;
    int64_t timeExp = 0;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    struct AudioTimeStamp time = {.tvSec = 0, .tvNSec = 0};
    ASSERT_NE(nullptr, capture);
    InitAttrs(attrs);
    attrs.type = AUDIO_IN_MEDIA;
    attrs.interleaved = false;
    attrs.format = AUDIO_FORMAT_PCM_16_BIT;
    attrs.sampleRate = SAMPLE_RATE_48000;
    attrs.channelCount = 2;
    ret = capture->SetSampleAttributes(capture, &attrs);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetSampleAttributes(capture, &attrsValue);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(SAMPLE_RATE_48000, attrsValue.sampleRate);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(DOUBLE_CHANNEL_COUNT, attrsValue.channelCount);

    ret = AudioCaptureStartAndOneFrame(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetCapturePosition(capture, &frames, &time);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT((time.tvSec) * SECTONSEC + (time.tvNSec), timeExp);
    EXPECT_GT(frames, INITIAL_VALUE);
    capture->Stop(capture);
}
/**
* @tc.name  Test GetCapturePosition API via define format to AUDIO_FORMAT_PCM_24_BIT
* @tc.number  SUB_Audio_HDI_CaptureGetCapturePosition_010
* @tc.desc  Test GetCapturePosition interface,return 0 if get framesize define format to AUDIO_FORMAT_PCM_24_BIT
* @tc.author: ZengLiFeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetCapturePosition_010, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    uint64_t frames = 0;
    int64_t timeExp = 0;
    struct AudioTimeStamp time = {.tvSec = 0, .tvNSec = 0};
    ASSERT_NE(nullptr, capture);
    InitAttrs(attrs);
    attrs.type = AUDIO_IN_MEDIA;
    attrs.interleaved = false;
    attrs.format = AUDIO_FORMAT_PCM_24_BIT;
    attrs.sampleRate = SAMPLE_RATE_48000;
    attrs.channelCount = 2;
    ret = capture->SetSampleAttributes(capture, &attrs);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetSampleAttributes(capture, &attrsValue);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(DOUBLE_CHANNEL_COUNT, attrsValue.channelCount);
    EXPECT_EQ(SAMPLE_RATE_48000, attrsValue.sampleRate);

    ret = AudioCaptureStartAndOneFrame(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetCapturePosition(capture, &frames, &time);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT((time.tvSec) * SECTONSEC + (time.tvNSec), timeExp);
    EXPECT_GT(frames, INITIAL_VALUE);
    capture->Stop(capture);
}
/**
* @tc.name  Test GetCapturePosition API via define sampleRate and channelCount to different value
* @tc.number  SUB_Audio_HDI_CaptureGetCapturePosition_011
* @tc.desc  Test GetCapturePosition interface,return 0 if get framesize define channelCount  as different values
* @tc.author: ZengLiFeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetCapturePosition_011, TestSize.Level1)
{
    int32_t ret;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    uint64_t frames = 0;
    int64_t timeExp = 0;
    struct AudioTimeStamp time = {.tvSec = 0, .tvNSec = 0};
    ASSERT_NE(nullptr, capture);
    InitAttrs(attrs);
    attrs.type = AUDIO_IN_MEDIA;
    attrs.interleaved = false;
    attrs.format = AUDIO_FORMAT_PCM_16_BIT;
    attrs.sampleRate = SAMPLE_RATE_48000;
    attrs.channelCount = 1;
    ret = capture->SetSampleAttributes(capture, &attrs);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetSampleAttributes(capture, &attrsValue);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_48000, attrsValue.sampleRate);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);

    ret = AudioCaptureStartAndOneFrame(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetCapturePosition(capture, &frames, &time);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT((time.tvSec) * SECTONSEC + (time.tvNSec), timeExp);
    EXPECT_GT(frames, INITIAL_VALUE);
    capture->Stop(capture);
}
/**
* @tc.name  Test GetCapturePosition API via define sampleRate and channelCount to 1
* @tc.number  SUB_Audio_HDI_CaptureGetCapturePosition_012
* @tc.desc  Test GetCapturePosition interface,return 0 if get framesize define channelCount to 1
* @tc.author: ZengLiFeng
*/
HWTEST_F(AudioIdlHdiCaptureHardwareDependenceTest, SUB_Audio_HDI_CaptureGetCapturePosition_012, TestSize.Level1)
{
    int32_t ret;
    uint64_t frames = 0;
    int64_t timeExp = 0;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    struct AudioTimeStamp time = {.tvSec = 0, .tvNSec = 0};
    ASSERT_NE(nullptr, capture);
    InitAttrs(attrs);
    attrs.type = AUDIO_IN_MEDIA;
    attrs.interleaved = false;
    attrs.format = AUDIO_FORMAT_PCM_24_BIT;
    attrs.sampleRate = SAMPLE_RATE_48000;
    attrs.channelCount = 1;
    ret = capture->SetSampleAttributes(capture, &attrs);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetSampleAttributes(capture, &attrsValue);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_EQ(SINGLE_CHANNEL_COUNT, attrsValue.channelCount);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(SAMPLE_RATE_48000, attrsValue.sampleRate);

    ret = AudioCaptureStartAndOneFrame(capture);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = capture->GetCapturePosition(capture, &frames, &time);
    EXPECT_EQ(HDF_SUCCESS, ret);
    EXPECT_GT((time.tvSec) * SECTONSEC + (time.tvNSec), timeExp);
    EXPECT_GT(frames, INITIAL_VALUE);
    capture->Stop(capture);
}
}
