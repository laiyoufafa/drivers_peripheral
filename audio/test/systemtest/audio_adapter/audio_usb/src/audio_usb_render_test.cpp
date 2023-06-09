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

/**
 * @addtogroup Audio
 * @{
 *
 * @brief Test audio-related APIs, including custom data types and functions for loading drivers,
 * accessing a driver adapter.
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file audio_hdi_common.h
 *
 * @brief Declares APIs for operations related to the audio adapter.
 *
 * @since 1.0
 * @version 1.0
 */

#include "audio_hdi_common.h"
#include "audio_usb_render_test.h"

using namespace std;
using namespace testing::ext;
using namespace HMOS::Audio;

namespace {
const string ADAPTER_NAME_USB = "usb";

class AudioUsbRenderTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static TestAudioManager *(*GetAudioManager)();
    static void *handleSo;
#ifdef AUDIO_MPI_SO
    static int32_t (*SdkInit)();
    static void (*SdkExit)();
    static void *sdkSo;
#endif
static int32_t GetManager(struct PrepareAudioPara& audiopara);
};

using THREAD_FUNC = void *(*)(void *);

TestAudioManager *(*AudioUsbRenderTest::GetAudioManager)() = nullptr;
void *AudioUsbRenderTest::handleSo = nullptr;
#ifdef AUDIO_MPI_SO
    int32_t (*AudioUsbRenderTest::SdkInit)() = nullptr;
    void (*AudioUsbRenderTest::SdkExit)() = nullptr;
    void *AudioUsbRenderTest::sdkSo = nullptr;
#endif

void AudioUsbRenderTest::SetUpTestCase(void)
{
#ifdef AUDIO_MPI_SO
    char sdkResolvedPath[] = HDF_LIBRARY_FULL_PATH("libhdi_audio_interface_lib_render");
    sdkSo = dlopen(sdkResolvedPath, RTLD_LAZY);
    if (sdkSo == nullptr) {
        return;
    }
    SdkInit = (int32_t (*)())(dlsym(sdkSo, "MpiSdkInit"));
    if (SdkInit == nullptr) {
        return;
    }
    SdkExit = (void (*)())(dlsym(sdkSo, "MpiSdkExit"));
    if (SdkExit == nullptr) {
        return;
    }
    SdkInit();
#endif
    char absPath[PATH_MAX] = {0};
    if (realpath(RESOLVED_PATH.c_str(), absPath) == nullptr) {
        return;
    }
    handleSo = dlopen(absPath, RTLD_LAZY);
    if (handleSo == nullptr) {
        return;
    }
    GetAudioManager = (TestAudioManager *(*)())(dlsym(handleSo, FUNCTION_NAME.c_str()));
    if (GetAudioManager == nullptr) {
        return;
    }
}

void AudioUsbRenderTest::TearDownTestCase(void)
{
#ifdef AUDIO_MPI_SO
    SdkExit();
    if (sdkSo != nullptr) {
        dlclose(sdkSo);
        sdkSo = nullptr;
    }
    if (SdkInit != nullptr) {
        SdkInit = nullptr;
    }
    if (SdkExit != nullptr) {
        SdkExit = nullptr;
    }
#endif
    if (handleSo != nullptr) {
        dlclose(handleSo);
        handleSo = nullptr;
    }
    if (GetAudioManager != nullptr) {
        GetAudioManager = nullptr;
    }
}

void AudioUsbRenderTest::SetUp(void) {}

void AudioUsbRenderTest::TearDown(void) {}

int32_t AudioUsbRenderTest::GetManager(struct PrepareAudioPara& audiopara)
{
    auto *inst = (AudioUsbRenderTest *)audiopara.self;
    if (inst != nullptr && inst->GetAudioManager != nullptr) {
        audiopara.manager = inst->GetAudioManager();
    }
    if (audiopara.manager == nullptr) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}
/**
* @tc.name  Test AudioRenderGetMute API via legal input.
* @tc.number  SUB_Audio_HDI_AudioRenderGetMute_0001
* @tc.desc  Test AudioRenderGetMute interface , return 0 if the audiocapture gets mute successfully.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderGetMute_0001, TestSize.Level1)
{
    int32_t ret = -1;
    bool muteTrue = true;
    bool muteFalse = false;
    bool defaultmute = false;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->volume.GetMute(render, &muteFalse);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(muteFalse, defaultmute);

    ret = render->volume.SetMute(render, muteTrue);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->volume.GetMute(render, &muteTrue);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_TRUE(muteTrue);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}

/**
* @tc.name  Test AudioRenderSetMute API via legal input.
* @tc.number  SUB_Audio_HDI_AudioRenderSetMute_0001
* @tc.desc  Test AudioRenderSetMute interface , return 0 if the audiorender object sets mute successfully.
* @tc.author:ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderSetMute_0001, TestSize.Level1)
{
    int32_t ret = -1;
    bool muteFalse = false;
    bool muteTrue = true;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->volume.SetMute(render, muteFalse);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetMute(render, &muteFalse);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(false, muteFalse);

    ret = render->volume.SetMute(render, muteTrue);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetMute(render, &muteTrue);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(true, muteTrue);

    muteTrue = false;
    ret = render->volume.SetMute(render, muteTrue);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_FALSE(muteTrue);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name  Test AudioRenderSetVolume API via legal input.
* @tc.number  SUB_Audio_HDI_AudioRenderSetVolume_0001
* @tc.desc  Test AudioRenderSetVolume interface , return 0 if the audiocapture sets volume successfully.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderSetVolume_0001, TestSize.Level1)
{
    int32_t ret = -1;
    float volumeInit = 0.20;
    float volumeInitExpc = 0.20;
    float volumeLow = 0.10;
    float volumeLowExpc = 0.10;
    float volumeMid = 0.50;
    float volumeMidExpc = 0.50;
    float volumeHigh = 0.80;
    float volumeHighExpc = 0.80;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->volume.SetVolume(render, volumeInit);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetVolume(render, &volumeInit);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(volumeInitExpc, volumeInit);
    ret = render->volume.SetVolume(render, volumeLow);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetVolume(render, &volumeLow);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(volumeLowExpc, volumeLow);
    ret = render->volume.SetVolume(render, volumeMid);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetVolume(render, &volumeMid);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(volumeMidExpc, volumeMid);
    ret = render->volume.SetVolume(render, volumeHigh);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetVolume(render, &volumeHigh);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(volumeHighExpc, volumeHigh);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name  Test AudioRenderGetVolume API via legal input.
* @tc.number  SUB_Audio_HDI_AudioRenderGetVolume_0001
* @tc.desc  Test AudioRenderGetVolume interface , return 0 if the audiocapture is get successful.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderGetVolume_0001, TestSize.Level1)
{
    int32_t ret = -1;
    float volume = 0.30;
    float volumeDefault = 0.30;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->volume.SetVolume(render, volume);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetVolume(render, &volume);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(volumeDefault, volume);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}

/**
* @tc.name  Test AudioRenderSetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_AudioRenderSetSampleAttributes_0001
* @tc.desc  Test AudioRenderSetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT;
*    attrs.sampleRate = AUDIO_SAMPLE_RATE_MASK_8000;
*    attrs.channelCount = 1;
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderSetSampleAttributes_0001, TestSize.Level1)
{
    int32_t ret = -1;
    uint32_t ret1 = 1;
    uint32_t ret2 = 8000;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_16_BIT, 1, 8000);

    ret = render->attr.SetSampleAttributes(render, &attrs);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->attr.GetSampleAttributes(render, &attrsValue);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_16_BIT, attrsValue.format);
    EXPECT_EQ(ret2, attrsValue.sampleRate);
    EXPECT_EQ(ret1, attrsValue.channelCount);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name  Test AudioRenderGetSampleAttributes API via legal input.
* @tc.number  SUB_Audio_HDI_AudioRenderGetSampleAttributes_0001
* @tc.desc  Test AudioRenderGetSampleAttributes ,the setting parameters are as follows.
*    attrs.type = AUDIO_IN_MEDIA;
*    attrs.format = AUDIO_FORMAT_PCM_16_BIT;
*    attrs.sampleRate = 8000;
*    attrs.channelCount = 1;
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderGetSampleAttributes_0001, TestSize.Level1)
{
    int32_t ret = -1;
    uint32_t ret1 = 8000;
    uint32_t ret2 = 1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, 1, 8000);
    ret = AudioRenderSetGetSampleAttributes(attrs, attrsValue, render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    EXPECT_EQ(AUDIO_IN_MEDIA, attrsValue.type);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(ret1, attrsValue.sampleRate);
    EXPECT_EQ(ret2, attrsValue.channelCount);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test AudioRenderStart API via  legal input
    * @tc.number  SUB_Audio_HDI_RenderStart_0001
    * @tc.desc  Test AudioRenderStart interface,return 0 if the audiorender object is created successfully.
    * @tc.author: wangqian
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderStart_0001, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    ASSERT_NE(nullptr, GetAudioManager);
    manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Start((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name Test AudioRenderStart API via the interface is called twice in a row
* @tc.number  SUB_Audio_HDI_RenderStart_0003
* @tc.desc  Test AudioRenderStart interface,return -1 the second time if the RenderStart is called twice
* @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderStart_0003, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Start((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Start((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_ERR_AO_BUSY, ret);

    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name Test AudioRenderStop API via legal input
* @tc.number  SUB_Audio_HDI_RenderStop_0001
* @tc.desc  test AudioRenderStop interface. return 0 if the rendering is successfully stopped.
* @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderStop_0001, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Start((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name Test AudioRenderStop API via the render does not start and stop only
* @tc.number  SUB_Audio_HDI_RenderStop_0002
* @tc.desc  test AudioRenderStop interface. return -4 if the render does not start and stop only
* @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderStop_0002, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_ERR_NOT_SUPPORT, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name Test RenderStop API via the interface is called twice in a row
* @tc.number  SUB_Audio_HDI_RenderStop_0003
* @tc.desc  Test RenderStop interface,return -4 the second time if the RenderStop is called twice
* @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderStop_0003, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Start((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_ERR_NOT_SUPPORT, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderPause API via legal input
    * @tc.number  SUB_Audio_HDI_RenderPause_001
    * @tc.desc  test HDI RenderPause interface，return 0 if the render is paused after start
    * @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderPause_0001, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Pause((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name Test AudioRenderPause API via the interface is called twice in a row
* @tc.number  SUB_Audio_HDI_RenderPause_0002
* @tc.desc  Test AudioRenderPause interface, return -1 the second time if RenderPause is called twice
* @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderPause_0002, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    TestAudioManager* manager = {};

    manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Pause((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Pause((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_ERR_NOT_SUPPORT, ret);

    render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name Test AudioRenderPause API via the render is paused after created.
* @tc.number  SUB_Audio_HDI_RenderPause_0003
* @tc.desc  Test AudioRenderPause interface,return -1 if the render is paused after created.
* @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderPause_0003, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Pause((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_ERR_INTERNAL, ret);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name Test AudioRenderPause API via the render is paused after stoped.
* @tc.number  SUB_Audio_HDI_RenderPause_0005
* @tc.desc  Test AudioRenderPause interface, return -1 the render is paused after stoped.
* @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderPause_0005, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    TestAudioManager* manager = {};

    manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Stop((AudioHandle)render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Pause((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_ERR_INTERNAL, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderResume API via the render is resumed after started
    * @tc.number  SUB_Audio_HDI_RenderResume_0001
    * @tc.desc  test HDI RenderResume interface,return -1 if the render is resumed after started
    * @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderResume_0001, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    TestAudioManager* manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Resume((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_ERR_NOT_SUPPORT, ret);

    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderResume API via the render is resumed after stopped
    * @tc.number  SUB_Audio_HDI_RenderResume_0002
    * @tc.desc  test HDI RenderResume interface,return -1 if the render is resumed after stopped
    * @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderResume_0002, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    TestAudioManager* manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Resume((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_ERR_NOT_SUPPORT, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderResume API via legal input
    * @tc.number  SUB_Audio_HDI_RenderResume_0003
    * @tc.desc  Test AudioRenderResume interface,return 0 if the render is resumed after paused
    * @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderResume_0003, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    TestAudioManager* manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Pause((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Resume((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderResume API via the interface is called twice in a row
    * @tc.number  SUB_Audio_HDI_RenderResume_0004
    * @tc.desc  Test RenderResume interface,return -1 the second time if the RenderResume is called twice
    * @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderResume_0004, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    TestAudioManager* manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Pause((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Resume((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Resume((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_ERR_NOT_SUPPORT, ret);

    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderResume API via the render Continue to start after resume
    * @tc.number  SUB_Audio_HDI_RenderResume_0005
    * @tc.desc  test HDI RenderResume interface,return -1 if the render Continue to start after resume
    * @tc.author: Xuhuandi
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderResume_0005, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    TestAudioManager* manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Pause((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Resume((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Start((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_ERR_AO_BUSY, ret);

    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name  Test AudioRenderFrame API via legal input
* @tc.number  SUB_Audio_HDI_AudioRenderFrame_0001
* @tc.desc  test AudioRenderFrame interface,Returns 0 if the data is written successfully
* @tc.author: liweiming
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderFrame_0001, TestSize.Level1)
{
    int32_t ret = -1;
    uint64_t requestBytes = 0;
    uint64_t replyBytes = 0;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    char *frame = nullptr;
    ASSERT_NE(GetAudioManager, nullptr);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Start((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = RenderFramePrepare(AUDIO_FILE, frame, requestBytes);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->RenderFrame(render, frame, requestBytes, &replyBytes);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
    if (frame != nullptr) {
        free(frame);
        frame = nullptr;
    }
}
/**
* @tc.name  Test AudioRenderFrame API without calling interface renderstart
* @tc.number  SUB_Audio_HDI_AudioRenderFrame_0005
* @tc.desc  Test AudioRenderFrame interface,Returns -1 if without calling interface renderstart
* @tc.author: liweiming
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderFrame_0005, TestSize.Level1)
{
    int32_t ret = -1;
    uint64_t replyBytes = 0;
    uint64_t requestBytes = 0;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    char *frame = nullptr;

    ASSERT_NE(GetAudioManager, nullptr);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = RenderFramePrepare(AUDIO_FILE, frame, requestBytes);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->RenderFrame(render, frame, requestBytes, &replyBytes);
    EXPECT_EQ(AUDIO_HAL_ERR_INVALID_PARAM, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
    if (frame != nullptr) {
        free(frame);
        frame = nullptr;
    }
}
/**
    * @tc.name  Test SetChannelMode API via setting channel mode to different enumeration values
    * @tc.number  SUB_Audio_HDI_AudioRenderSetChannelMode_0001
    * @tc.desc  Test SetChannelMode interface,return 0 if set channel mode to different enumeration values
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderSetChannelMode_0001, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter =nullptr;
    struct AudioRender *render = nullptr;
    AudioChannelMode mode = AUDIO_CHANNEL_NORMAL;
    AudioChannelMode modeOne = AUDIO_CHANNEL_BOTH_LEFT;
    AudioChannelMode modeSec = AUDIO_CHANNEL_BOTH_RIGHT;
    AudioChannelMode modeTrd = AUDIO_CHANNEL_EXCHANGE;
    ASSERT_NE(GetAudioManager, nullptr);
    manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->SetChannelMode(render, mode);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetChannelMode(render, &mode);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_CHANNEL_NORMAL, mode);
    ret = render->SetChannelMode(render, modeOne);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetChannelMode(render, &modeOne);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_CHANNEL_BOTH_LEFT, modeOne);
    ret = render->SetChannelMode(render, modeSec);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetChannelMode(render, &modeSec);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_CHANNEL_BOTH_RIGHT, modeSec);
    ret = render->SetChannelMode(render, modeTrd);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetChannelMode(render, &modeTrd);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_CHANNEL_EXCHANGE, modeTrd);
    render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test SetChannelMode API via setting channel mode to different values
    * @tc.number  SUB_Audio_HDI_AudioRenderSetChannelMode_0002
    * @tc.desc  Test SetChannelMode interface,return 0 if set channel mode to different values
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderSetChannelMode_0002, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter =nullptr;
    struct AudioRender *render = nullptr;
    AudioChannelMode mode = AUDIO_CHANNEL_MIX;
    AudioChannelMode modeOne = AUDIO_CHANNEL_LEFT_MUTE;
    AudioChannelMode modeSec = AUDIO_CHANNEL_RIGHT_MUTE;
    AudioChannelMode modeTrd = AUDIO_CHANNEL_BOTH_MUTE;
    ASSERT_NE(GetAudioManager, nullptr);
    manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->SetChannelMode(render, mode);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetChannelMode(render, &mode);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_CHANNEL_MIX, mode);
    ret = render->SetChannelMode(render, modeOne);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetChannelMode(render, &modeOne);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_CHANNEL_LEFT_MUTE, modeOne);
    ret = render->SetChannelMode(render, modeSec);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetChannelMode(render, &modeSec);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_CHANNEL_RIGHT_MUTE, modeSec);
    ret = render->SetChannelMode(render, modeTrd);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetChannelMode(render, &modeTrd);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_CHANNEL_BOTH_MUTE, modeTrd);
    render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test SetChannelMode API via setting channel mode after render object is created
    * @tc.number  SUB_Audio_HDI_AudioRenderSetChannelMode_0003
    * @tc.desc  Test SetChannelMode interface,return 0 if set channel mode after render object is created
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderSetChannelMode_0003, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter =nullptr;
    struct AudioRender *render = nullptr;
    AudioChannelMode mode = AUDIO_CHANNEL_NORMAL;
    ASSERT_NE(GetAudioManager, nullptr);
    manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->SetChannelMode(render, mode);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetChannelMode(render, &mode);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_CHANNEL_NORMAL, mode);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test GetChannelMode API via getting the channel mode after setting
    * @tc.number  SUB_Audio_HDI_AudioRenderGetChannelMode_0001
    * @tc.desc  Test GetChannelMode interface,return 0 if getting the channel mode after setting
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderGetChannelMode_0001, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter =nullptr;
    struct AudioRender *render = nullptr;
    AudioChannelMode mode = AUDIO_CHANNEL_NORMAL;
    ASSERT_NE(GetAudioManager, nullptr);
    manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->GetChannelMode(render, &mode);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->SetChannelMode(render, mode);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetChannelMode(render, &mode);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_CHANNEL_NORMAL, mode);

    render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test GetChannelMode API via getting the channel mode after the render object is created
    * @tc.number  SUB_Audio_HDI_AudioRenderGetChannelMode_0003
    * @tc.desc  Test GetChannelMode interface,return 0 if getting the channel mode after the object is created
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderGetChannelMode_0003, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    TestAudioManager* manager = {};
    AudioChannelMode mode = AUDIO_CHANNEL_NORMAL;
    ASSERT_NE(GetAudioManager, nullptr);
    manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetChannelMode(render, &mode);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_CHANNEL_NORMAL, mode);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test SetRenderSpeed API via legal
    * @tc.number  SUB_Audio_HDI_AudioRenderSetRenderSpeed_0001
    * @tc.desc  Test SetRenderSpeed interface,return -2 if setting RenderSpeed
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderSetRenderSpeed_0001, TestSize.Level1)
{
    int32_t ret = -1;
    float speed = 100;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter =nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(GetAudioManager, nullptr);
    manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->SetRenderSpeed(render, speed);
    EXPECT_EQ(AUDIO_HAL_ERR_NOT_SUPPORT, ret);

    render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test GetRenderSpeed API via legal
    * @tc.number  SUB_Audio_HDI_AudioRenderGetRenderSpeed_0001
    * @tc.desc  Test GetRenderSpeed interface,return -2 if getting RenderSpeed
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderGetRenderSpeed_0001, TestSize.Level1)
{
    int32_t ret = -1;
    float speed = 0;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter =nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(GetAudioManager, nullptr);
    manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->GetRenderSpeed(render, &speed);
    EXPECT_EQ(AUDIO_HAL_ERR_NOT_SUPPORT, ret);

    render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name  Test RenderGetLatency API via legal
* @tc.number  SUB_Audio_HDI_RenderGetLatency_0001
* @tc.desc  test RenderGetLatency interface, return 0 if GetLatency successful
* @tc.author: wangkang
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderGetLatency_0001, TestSize.Level1)
{
    int32_t ret = -1;
    uint32_t latencyTime = 0;
    uint32_t expectLatency = 0;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(GetAudioManager, nullptr);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->GetLatency(render, &latencyTime);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_LT(expectLatency, latencyTime);

    ret = render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderFlush API via legal input Verify that the data in the buffer is flushed after stop
    * @tc.number  SUB_Audio_HDI_RenderFlush_0001
    * @tc.desc  Test RenderFlush interface,return -2 if the data in the buffer is flushed successfully after stop
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderFlush_0001, TestSize.Level1)
{
    int32_t ret = -1;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    manager = GetAudioManager();
    ASSERT_NE(nullptr, GetAudioManager);
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Flush((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_ERR_NOT_SUPPORT, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderGetFrameSize API via legal input
    * @tc.number  SUB_Audio_HDI_RenderGetFrameSize_0001
    * @tc.desc  Test RenderGetFrameSize interface,return 0 if the FrameSize was obtained successfully
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetFrameSize_0001, TestSize.Level1)
{
    int32_t ret = -1;
    uint64_t size = 0;
    uint64_t zero = 0;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->attr.GetFrameSize(render, &size);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_GT(size, zero);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderGetFrameSize API via define format to different values
    * @tc.number  SUB_Audio_HDI_RenderGetFrameSize_0004
    * @tc.desc  Test RenderGetFrameSize interface,return 0 if get framesize define format as different values
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetFrameSize_0004, TestSize.Level1)
{
    int32_t ret = -1;
    uint64_t size = 0;
    uint64_t channelCountExp = 2;
    uint32_t sampleRateExp = 48000;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, 2, 48000);

    ret = AudioRenderSetGetSampleAttributes(attrs, attrsValue, render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(sampleRateExp, attrsValue.sampleRate);
    EXPECT_EQ(channelCountExp, attrsValue.channelCount);

    ret = render->attr.GetFrameSize(render, &size);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_GT(size, INITIAL_VALUE);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderGetFrameCount API via legal
    * @tc.number  SUB_Audio_HDI_RenderGetFrameCount_0001
    * @tc.desc  Test RenderGetFrameCount interface, return 0 if the FrameSize was obtained successfully
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetFrameCount_0001, TestSize.Level1)
{
    int32_t ret = -1;
    uint64_t count = 0;
    uint64_t zero = 0;
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter =nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->attr.GetFrameCount(render, &count);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_GT(count, zero);

    render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderGetFrameCount API via define channelCount to different value
    * @tc.number  SUB_Audio_HDI_RenderGetFrameCount_0004
    * @tc.desc  Test RenderGetFrameCount interface,return 0 if get framesize define channelCount as different values
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetFrameCount_0004, TestSize.Level1)
{
    int32_t ret = -1;
    uint64_t count = 0;
    uint64_t channelCountExp = 2;
    uint32_t sampleRateExp = 8000;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, 2, 8000);

    ret = AudioRenderSetGetSampleAttributes(attrs, attrsValue, render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(AUDIO_FORMAT_PCM_24_BIT, attrsValue.format);
    EXPECT_EQ(sampleRateExp, attrsValue.sampleRate);
    EXPECT_EQ(channelCountExp, attrsValue.channelCount);

    ret = AudioRenderStartAndOneFrame(render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->attr.GetFrameCount(render, &count);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_GT(count, INITIAL_VALUE);

    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderGetCurrentChannelId API via legal input
    * @tc.number  SUB_Audio_HDI_RenderGetCurrentChannelId_0001
    * @tc.desc  Test RenderGetCurrentChannelId, return 0 if the default CurrentChannelId is obtained successfully
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetCurrentChannelId_0001, TestSize.Level1)
{
    int32_t ret = -1;
    uint32_t channelId = 0;
    uint32_t channelIdValue = CHANNELCOUNT;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->attr.GetCurrentChannelId(render, &channelId);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(channelIdValue, channelId);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test GetCurrentChannelId API via get channelId to 1 and set channelCount to 1
    * @tc.number  SUB_Audio_HDI_RenderGetCurrentChannelId_0003
    * @tc.desc  Test GetCurrentChannelId interface,return 0 if get channelId to 1 and set channelCount to 1
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetCurrentChannelId_0002, TestSize.Level1)
{
    int32_t ret = -1;
    uint32_t channelId = 0;
    uint32_t channelIdExp = 1;
    uint32_t channelCountExp = 1;
    struct AudioSampleAttributes attrs = {};
    struct AudioSampleAttributes attrsValue = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    InitAttrsUpdate(attrs, AUDIO_FORMAT_PCM_24_BIT, 1, 32000);

    ret = AudioRenderSetGetSampleAttributes(attrs, attrsValue, render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(channelCountExp, attrs.channelCount);

    ret = render->attr.GetCurrentChannelId(render, &channelId);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(channelIdExp, channelId);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderGetCurrentChannelId API via CurrentChannelId is obtained after created
    * @tc.number  SUB_Audio_HDI_RenderGetCurrentChannelId_0003
    * @tc.desc  Test RenderGetCurrentChannelId interface, return 0 if CurrentChannelId is obtained after created
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetCurrentChannelId_0003, TestSize.Level1)
{
    int32_t ret = -1;
    uint32_t channelId = 0;
    uint32_t channelIdExp = 2;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->attr.GetCurrentChannelId(render, &channelId);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(channelIdExp, channelId);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name   Test AudioRenderCheckSceneCapability API and check scene's capability
* @tc.number  SUB_Audio_HDI_RenderCheckSceneCapability_0001
* @tc.desc  Test AudioRenderCheckSceneCapability interface,return 0 if check scene's capability successful.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderCheckSceneCapability_0001, TestSize.Level1)
{
    int32_t ret = -1;
    bool supported = false;
    struct AudioSceneDescriptor scenes = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    scenes.scene.id = 0;
    scenes.desc.pins = PIN_OUT_SPEAKER;
    ret = render->scene.CheckSceneCapability(render, &scenes, &supported);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_TRUE(supported);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name   Test checking scene's capability where the scene is not configed in the josn.
* @tc.number  SUB_Audio_HDI_RenderCheckSceneCapability_0002
* @tc.desc  Test RenderCheckSceneCapability interface,return -1 if the scene is not configed in the josn.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderCheckSceneCapability_0002, TestSize.Level1)
{
    int32_t ret = -1;
    bool supported = true;
    struct AudioSceneDescriptor scenes = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    scenes.scene.id = 5;
    scenes.desc.pins = PIN_OUT_SPEAKER;
    ret = render->scene.CheckSceneCapability(render, &scenes, &supported);
    EXPECT_EQ(AUDIO_HAL_ERR_INTERNAL, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name  Test AudioRenderSelectScene API via legal input
* @tc.number  SUB_Audio_HDI_AudioRenderSelectScene_0001
* @tc.desc  Test AudioRenderSelectScene interface,return 0 if select Render's scene successful.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderSelectScene_0001, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioSceneDescriptor scenes = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    scenes.scene.id = 0;
    scenes.desc.pins = PIN_OUT_SPEAKER;

    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->scene.SelectScene(render, &scenes);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = AudioRenderStartAndOneFrame(render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name  Test AudioRenderSelectScene API after Render start.
* @tc.number  SUB_Audio_HDI_AudioRenderSelectScene_0002
* @tc.desc  Test AudioRenderSelectScene, return 0 if select Render's scene successful after Render start.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderSelectScene_0002, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioSceneDescriptor scenes = {};
    TestAudioManager* manager = {};
    struct AudioAdapter *adapter =nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    scenes.scene.id = 0;
    scenes.desc.pins = PIN_OUT_SPEAKER;
    ret = render->scene.SelectScene(render, &scenes);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name  Test AudioRenderSelectScene API where the parameter handle is empty.
* @tc.number  SUB_Audio_HDI_AudioRenderSelectScene_0003
* @tc.desc  Test AudioRenderSelectScene, return -1 if the parameter handle is empty.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderSelectScene_0003, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioSceneDescriptor scenes = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    struct AudioRender *renderNull = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    scenes.scene.id = 0;
    scenes.desc.pins = PIN_IN_MIC;
    ret = render->scene.SelectScene(renderNull, &scenes);
    EXPECT_EQ(AUDIO_HAL_ERR_INVALID_PARAM, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name  Test AudioRenderSelectScene API where the scene is not configed in the josn.
* @tc.number  SUB_Audio_HDI_AudioRenderSelectScene_0005
* @tc.desc  Test AudioRenderSelectScene, return -1 if the scene is not configed in the josn.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderSelectScene_0005, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioSceneDescriptor scenes = {};
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    scenes.scene.id = 5;
    scenes.desc.pins = PIN_OUT_HDMI;
    ret = render->scene.SelectScene(render, &scenes);
    EXPECT_EQ(AUDIO_HAL_ERR_INTERNAL, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderGetGainThreshold API via legal input
    * @tc.number  SUB_Audio_HDI_RenderGetGainThreshold_0001
    * @tc.desc  Test RenderGetGainThreshold interface,return 0 if the GetGainThreshold is obtained successfully
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetGainThreshold_0001, TestSize.Level1)
{
    int32_t ret = -1;
    float min = 0;
    float max = 0;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->volume.GetGainThreshold((AudioHandle)render, &min, &max);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(min, GAIN_MIN);
    EXPECT_EQ(max, GAIN_MAX);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderSetGain API via legal input
    * @tc.number  SUB_Audio_HDI_RenderSetGain_0001
    * @tc.desc  Test RenderSetGain interface,return 0 if Set gain to normal value, maximum or minimum and get success
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderSetGain_0001, TestSize.Level1)
{
    int32_t ret = -1;
    float min = 0;
    float max = 0;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetGainThreshold((AudioHandle)render, &min, &max);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    float gain = 10.8;
    float gainMax = max;
    float gainMin = min;
    float gainExpc = 10;
    float gainMaxExpc = max;
    float gainMinExpc = min;
    ret = render->volume.SetGain(render, gain);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetGain(render, &gain);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(gainExpc, gain);

    ret = render->volume.SetGain(render, gainMax);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetGain(render, &gainMax);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(gainMaxExpc, gainMax);

    ret = render->volume.SetGain(render, gainMin);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetGain(render, &gainMin);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(gainMinExpc, gainMin);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderGetGain API via legal input
    * @tc.number  SUB_Audio_HDI_RenderGetGain_0001
    * @tc.desc  Test RenderGetGain interface,return 0 if the RenderGetGain was obtained successfully
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetGain_0001, TestSize.Level1)
{
    int32_t ret = -1;
    float min = 0;
    float max = 0;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetGainThreshold((AudioHandle)render, &min, &max);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    float gain = min+1;
    float gainValue = min+1;
    ret = render->volume.SetGain(render, gain);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->volume.GetGain(render, &gain);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(gainValue, gain);

    render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name  Test GetRenderPosition API via legal input
* @tc.number  SUB_Audio_HDI_AudioRenderGetRenderPosition_0001
* @tc.desc  Test GetRenderPosition interface,Returns 0 if get RenderPosition during playing.
* @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderGetRenderPosition_0001, TestSize.Level1)
{
    int32_t ret = -1;
    uint64_t frames = 0;
    int64_t timeExp = 0;
    struct AudioTimeStamp time = {.tvSec = 0, .tvNSec = 0};
    struct PrepareAudioPara audiopara = {
        .portType = PORT_OUT, .adapterName = ADAPTER_NAME_USB.c_str(), .self = this, .pins = PIN_OUT_SPEAKER,
        .path = AUDIO_FILE.c_str()
    };
    ASSERT_NE(GetAudioManager, nullptr);
    audiopara.manager = GetAudioManager();
    ASSERT_NE(audiopara.manager, nullptr);

    ret = pthread_create(&audiopara.tids, NULL, (THREAD_FUNC)PlayAudioFile, &audiopara);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    sleep(1);
    if (audiopara.render != nullptr) {
        ret = audiopara.render->GetRenderPosition(audiopara.render, &frames, &time);
        EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
        EXPECT_GT((time.tvSec) * SECTONSEC + (time.tvNSec), timeExp);
        EXPECT_GT(frames, INITIAL_VALUE);
    }

    ret = ThreadRelease(audiopara);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
}
/**
* @tc.name  Test AudioRenderGetRenderPosition API via get RenderPosition after the audio file is Paused and resumed
* @tc.number  SUB_Audio_HDI_AudioRenderGetRenderPosition_0002
* @tc.desc   Test GetRenderPosition interface,Returns 0 if get RenderPosition after Pause and resume during playing
* @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderGetRenderPosition_0002, TestSize.Level1)
{
    int32_t ret = -1;
    int64_t timeExp = 0;
    uint64_t frames = 0;
    struct AudioTimeStamp time = {.tvSec = 0, .tvNSec = 0};
    struct PrepareAudioPara audiopara = {
        .portType = PORT_OUT, .adapterName = ADAPTER_NAME_USB.c_str(), .self = this, .pins = PIN_OUT_SPEAKER,
        .path = AUDIO_FILE.c_str()
    };
    ASSERT_NE(GetAudioManager, nullptr);
    audiopara.manager = GetAudioManager();
    ASSERT_NE(audiopara.manager, nullptr);

    ret = pthread_create(&audiopara.tids, NULL, (THREAD_FUNC)PlayAudioFile, &audiopara);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    sleep(1);
    if (audiopara.render != nullptr) {
        FrameStatus(0);
        usleep(1000);
        ret = audiopara.render->control.Pause((AudioHandle)(audiopara.render));
        EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
        ret = audiopara.render->GetRenderPosition(audiopara.render, &frames, &time);
        EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
        EXPECT_GT((time.tvSec) * SECTONSEC + (time.tvNSec), timeExp);
        EXPECT_GT(frames, INITIAL_VALUE);
        usleep(1000);
        ret = audiopara.render->control.Resume((AudioHandle)(audiopara.render));
        EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
        FrameStatus(1);
        ret = audiopara.render->GetRenderPosition(audiopara.render, &frames, &time);
        EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
        EXPECT_GT((time.tvSec) * SECTONSEC + (time.tvNSec), timeExp);
        EXPECT_GT(frames, INITIAL_VALUE);
    }

    ret = ThreadRelease(audiopara);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
}
/**
* @tc.name  Test GetRenderPosition API via get RenderPosition after the audio file is stopped
* @tc.number  SUB_Audio_HDI_AudioRenderGetRenderPosition_0003
* @tc.desc  Test GetRenderPosition interface,Returns 0 if get RenderPosition after stop
* @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderGetRenderPosition_0003, TestSize.Level1)
{
    int32_t ret = -1;
    int64_t timeExp = 0;
    uint64_t frames = 0;
    struct AudioAdapter *adapter =nullptr;
    struct AudioRender *render = nullptr;
    struct AudioTimeStamp time = {.tvSec = 0, .tvNSec = 0};
    ASSERT_NE(GetAudioManager, nullptr);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->GetRenderPosition(render, &frames, &time);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_GT((time.tvSec) * SECTONSEC + (time.tvNSec), timeExp);
    EXPECT_GT(frames, INITIAL_VALUE);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderSetExtraParams API via setting ExtraParams during playback
    * @tc.number  SUB_Audio_HDI_RenderSetExtraParams_0001
    * @tc.desc  Test RenderSetExtraParams interface,return 0 if the ExtraParams is set during playback
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderSetExtraParams_0001, TestSize.Level1)
{
    int32_t ret = -1;
    char keyValueList[] = "attr-route=1;attr-format=32;attr-channels=2;attr-frame-count=82;attr-sampling-rate=48000";
    char keyValueListExp[] = "attr-route=1;attr-format=32;attr-channels=2;attr-sampling-rate=48000";
    size_t index = 1;
    char keyValueListValue[256] = {};
    int32_t listLenth = 256;
    struct PrepareAudioPara audiopara = {
        .portType = PORT_OUT, .adapterName = ADAPTER_NAME_USB.c_str(), .self = this, .pins = PIN_OUT_SPEAKER,
        .path = AUDIO_FILE.c_str()
    };
    ASSERT_NE(nullptr, GetAudioManager);
    audiopara.manager = GetAudioManager();
    ASSERT_NE(nullptr, audiopara.manager);

    ret = pthread_create(&audiopara.tids, NULL, (THREAD_FUNC)PlayAudioFile, &audiopara);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    sleep(1);
    if (audiopara.render != nullptr) {
        ret = audiopara.render->attr.SetExtraParams((AudioHandle)audiopara.render, keyValueList);
        EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
        ret = audiopara.render->attr.GetExtraParams((AudioHandle)audiopara.render, keyValueListValue, listLenth);
        EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
        string strGetValue = keyValueListValue;
        size_t indexAttr = strGetValue.find("attr-frame-count");
        size_t indexFlag = strGetValue.rfind(";");
        if (indexAttr != string::npos && indexFlag != string::npos) {
            strGetValue.replace(indexAttr, indexFlag - indexAttr + index, "");
        }
        EXPECT_STREQ(keyValueListExp, strGetValue.c_str());
    }

    ret = ThreadRelease(audiopara);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
}
/**
    * @tc.name  Test RenderSetExtraParams API via setting some parameters after playing
    * @tc.number  SUB_Audio_HDI_RenderSetExtraParams_0002
    * @tc.desc  Test RenderSetExtraParams interface,return 0 if some parameters is set after playing
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderSetExtraParams_0002, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = {};
    struct AudioRender *render = nullptr;
    char keyValueListOne[] = "attr-frame-count=1024;";
    char keyValueListOneExp[] = "attr-route=0;attr-format=16;attr-channels=2;attr-frame-count=1024;\
attr-sampling-rate=48000";
    char keyValueListTwo[] = "attr-format=16;attr-frame-count=1024;";
    char keyValueListTwoExp[] = "attr-route=0;attr-format=16;attr-channels=2;attr-frame-count=1024;\
attr-sampling-rate=48000";
    char keyValueListThr[] = "attr-route=1;attr-channels=1;attr-frame-count=1024;";
    char keyValueListThrExp[] = "attr-route=1;attr-format=16;attr-channels=1;attr-frame-count=1024;\
attr-sampling-rate=48000";
    char keyValueListFour[] = "attr-format=32;attr-channels=2;attr-frame-count=4096;attr-sampling-rate=48000";
    char keyValueListFourExp[] = "attr-route=1;attr-format=32;attr-channels=2;attr-frame-count=4096;\
attr-sampling-rate=48000";
    char keyValueListValueOne[256] = {};
    char keyValueListValueTwo[256] = {};
    char keyValueListValueThr[256] = {};
    char keyValueListValueFour[256] = {};
    int32_t listLenth = 256;
    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->attr.SetExtraParams((AudioHandle)render, keyValueListOne);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->attr.GetExtraParams((AudioHandle)render, keyValueListValueOne, listLenth);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_STREQ(keyValueListOneExp, keyValueListValueOne);
    ret = render->attr.SetExtraParams((AudioHandle)render, keyValueListTwo);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->attr.GetExtraParams((AudioHandle)render, keyValueListValueTwo, listLenth);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_STREQ(keyValueListTwoExp, keyValueListValueTwo);
    ret = render->attr.SetExtraParams((AudioHandle)render, keyValueListThr);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->attr.GetExtraParams((AudioHandle)render, keyValueListValueThr, listLenth);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_STREQ(keyValueListThrExp, keyValueListValueThr);
    ret = render->attr.SetExtraParams((AudioHandle)render, keyValueListFour);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->attr.GetExtraParams((AudioHandle)render, keyValueListValueFour, listLenth);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_STREQ(keyValueListFourExp, keyValueListValueFour);
    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
    * @tc.name  Test RenderGetExtraParams API via legal input
    * @tc.number  SUB_Audio_HDI_RenderGetExtraParams_0001
    * @tc.desc  Test RenderGetExtraParams interface,return 0 if the RenderGetExtraParams was obtained successfully
    * @tc.author: tiansuli
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetExtraParams_0001, TestSize.Level1)
{
    int32_t ret = -1;
    uint64_t count = 0;
    struct AudioAdapter *adapter = {};
    struct AudioRender *render = nullptr;
    struct AudioSampleAttributes attrsValue = {};
    char keyValueList[] = "attr-format=24;attr-frame-count=4096;";
    char keyValueListExp[] = "attr-route=0;attr-format=24;attr-channels=2;attr-frame-count=4096;\
attr-sampling-rate=48000";
    char keyValueListValue[256] = {};
    int32_t listLenth = 256;
    int32_t formatExp = 3;
    uint32_t sampleRateExp = 48000;
    uint32_t channelCountExp = 2;
    uint32_t frameCountExp = 4096;

    ASSERT_NE(nullptr, GetAudioManager);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->attr.SetExtraParams((AudioHandle)render, keyValueList);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = render->attr.GetExtraParams((AudioHandle)render, keyValueListValue, listLenth);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_STREQ(keyValueListExp, keyValueListValue);

    ret = render->attr.GetSampleAttributes(render, &attrsValue);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(formatExp, attrsValue.format);
    EXPECT_EQ(sampleRateExp, attrsValue.sampleRate);
    EXPECT_EQ(channelCountExp, attrsValue.channelCount);
    ret = render->attr.GetFrameCount(render, &count);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ(count, frameCountExp);

    ret = render->control.Stop((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}

/**
* @tc.name  Test AudioRenderTurnStandbyMode API via
* @tc.number  SUB_Audio_HDI_AudioRenderTurnStandbyMode_0001
* @tc.desc  Test AudioRenderTurnStandbyMode interface,return 0 if the interface use correctly.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderTurnStandbyMode_0001, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    ASSERT_NE(GetAudioManager, nullptr);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateStartRender(manager, &render, &adapter, ADAPTER_NAME_USB);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->control.TurnStandbyMode((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    sleep(3);

    ret = render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}

/**
* @tc.name  Test AudioRenderAudioDevDump API via
* @tc.number  SUB_Audio_HDI_AudioRenderAudioDevDump_0001
* @tc.desc  Test AudioRenderAudioDevDump interface,return 0 if the interface use correctly.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderAudioDevDump_0001, TestSize.Level1)
{
    int32_t ret = -1;
    char pathBuf[] = "./DevDump.log";
    FILE *fp = fopen(pathBuf, "wb+");
    ASSERT_NE(nullptr, fp);
    int fd = fileno(fp);
    if (fd == -1) {
        fclose(fp);
        ASSERT_NE(fd, -1);
    }
    struct PrepareAudioPara audiopara = {
        .portType = PORT_OUT, .adapterName = ADAPTER_NAME_USB.c_str(), .self = this, .pins = PIN_OUT_SPEAKER,
        .path = AUDIO_FILE.c_str()
    };
    ret = GetManager(audiopara);
    if (ret < 0) {
        fclose(fp);
        ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    }
    ret = pthread_create(&audiopara.tids, NULL, (THREAD_FUNC)PlayAudioFile, &audiopara);
    if (ret < 0) {
        fclose(fp);
        ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    }
    sleep(1);
    ret = audiopara.render->control.Pause((AudioHandle)audiopara.render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    sleep(1);
    ret = audiopara.render->control.Resume((AudioHandle)audiopara.render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = audiopara.render->control.AudioDevDump((AudioHandle)audiopara.render, RANGE, fd);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    fclose(fp);
    ret = ThreadRelease(audiopara);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
}

/**
* @tc.name  Test ReqMmapBuffer API via legal input
* @tc.number  SUB_Audio_HDI_RenderReqMmapBuffer_0001
* @tc.desc  Test ReqMmapBuffer interface,return 0 if call ReqMmapBuffer interface sucessfully
* @tc.author: liweiming
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderReqMmapBuffer_0001, TestSize.Level1)
{
    int32_t ret = -1;
    bool isRender = true;
    int32_t reqSize = 0;
    struct AudioMmapBufferDescripter desc = {};
    struct AudioRender *render = nullptr;
    struct AudioAdapter *adapter = nullptr;
    struct AudioSampleAttributes attrs = {};
    ASSERT_NE(GetAudioManager, nullptr);
    FILE *fp = fopen(LOW_LATENCY_AUDIO_FILE.c_str(), "rb+");
    ASSERT_NE(fp, nullptr);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    if (ret < 0 || render == nullptr) {
        fclose(fp);
        ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
        ASSERT_EQ(nullptr, render);
    }
    InitAttrs(attrs);
    attrs.startThreshold = 0;
    ret = render->attr.SetSampleAttributes(render, &attrs);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = InitMmapDesc(fp, desc, reqSize, isRender);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret =  render->control.Start((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret =  render->attr.ReqMmapBuffer((AudioHandle)render, reqSize, &desc);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    fclose(fp);
    if (ret == 0) {
        munmap(desc.memoryAddress, reqSize);
    }
    render->control.Stop((AudioHandle)render);
    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}
/**
* @tc.name  Test GetMmapPosition API via Getting position is normal in Before playing and Playing.
* @tc.number  SUB_Audio_HDI_RenderGetMmapPosition_0001
* @tc.desc  Test GetMmapPosition interface,return 0 if Getting position successfully.
* @tc.author: zhouyongxiao
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetMmapPosition_0001, TestSize.Level1)
{
    uint64_t frames = 0;
    uint64_t framesRendering = 0;
    uint64_t framesexpRender = 0;
    int64_t timeExp = 0;
    struct PrepareAudioPara audiopara = {
        .portType = PORT_OUT, .adapterName = ADAPTER_NAME_USB.c_str(), .pins = PIN_OUT_SPEAKER,
        .path = LOW_LATENCY_AUDIO_FILE.c_str()
    };
    ASSERT_NE(GetAudioManager, nullptr);
    audiopara.manager = GetAudioManager();
    ASSERT_NE(audiopara.manager, nullptr);
    int32_t ret = AudioCreateRender(audiopara.manager, audiopara.pins, audiopara.adapterName, &audiopara.adapter,
                            &audiopara.render);
    if (ret < 0 || audiopara.render == nullptr) {
        ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
        ASSERT_EQ(nullptr, audiopara.render);
    }
    InitAttrs(audiopara.attrs);
    audiopara.attrs.startThreshold = 0;
    ret = audiopara.render->attr.SetSampleAttributes(audiopara.render, &(audiopara.attrs));
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = audiopara.render->attr.GetMmapPosition(audiopara.render, &frames, &(audiopara.time));
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_EQ((audiopara.time.tvSec) * SECTONSEC + (audiopara.time.tvNSec), timeExp);
    EXPECT_EQ(frames, INITIAL_VALUE);

    ret = pthread_create(&audiopara.tids, NULL, (THREAD_FUNC)PlayMapAudioFile, &audiopara);
    if (ret != 0) {
        audiopara.adapter->DestroyRender(audiopara.adapter, audiopara.render);
        audiopara.manager->UnloadAdapter(audiopara.manager, audiopara.adapter);
        ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    }
    sleep(1);
    ret = audiopara.render->attr.GetMmapPosition(audiopara.render, &framesRendering, &(audiopara.time));
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_GT((audiopara.time.tvSec) * SECTONSEC + (audiopara.time.tvNSec), timeExp);
    EXPECT_GT(framesRendering, INITIAL_VALUE);
    int64_t timeExprendering = (audiopara.time.tvSec) * SECTONSEC + (audiopara.time.tvNSec);
    void *result = nullptr;
    pthread_join(audiopara.tids, &result);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, (intptr_t)result);
    ret = audiopara.render->attr.GetMmapPosition(audiopara.render, &framesexpRender, &(audiopara.time));
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_GE((audiopara.time.tvSec) * SECTONSEC + (audiopara.time.tvNSec), timeExprendering);
    EXPECT_GE(framesexpRender, framesRendering);
    audiopara.render->control.Stop((AudioHandle)audiopara.render);
    audiopara.adapter->DestroyRender(audiopara.adapter, audiopara.render);
    audiopara.manager->UnloadAdapter(audiopara.manager, audiopara.adapter);
}
/**
* @tc.name  Test GetMmapPosition API via SetSampleAttributes and Getting position is normal.
* @tc.number  SUB_Audio_HDI_RenderGetMmapPosition_0002
* @tc.desc  Test GetMmapPosition interface,return 0 if Getting position successfully.
* @tc.author: zhouyongxiao
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_RenderGetMmapPosition_0002, TestSize.Level1)
{
    int32_t ret = -1;
    uint64_t frames = 0;
    int64_t timeExp = 0;
    struct PrepareAudioPara audiopara = {
        .portType = PORT_OUT, .adapterName = ADAPTER_NAME_USB.c_str(), .self = this, .pins = PIN_OUT_SPEAKER,
        .path = LOW_LATENCY_AUDIO_FILE.c_str()
    };
    ASSERT_NE(GetAudioManager, nullptr);
    audiopara.manager = GetAudioManager();
    ASSERT_NE(audiopara.manager, nullptr);
    ret = AudioCreateRender(audiopara.manager, audiopara.pins, audiopara.adapterName, &audiopara.adapter,
                            &audiopara.render);
    if (ret < 0 || audiopara.render == nullptr) {
        ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
        ASSERT_EQ(nullptr, audiopara.render);
    }
    InitAttrs(audiopara.attrs);
    audiopara.attrs.format = AUDIO_FORMAT_PCM_24_BIT;
    audiopara.attrs.channelCount = 1;
    ret = audiopara.render->attr.SetSampleAttributes(audiopara.render, &(audiopara.attrs));
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = pthread_create(&audiopara.tids, NULL, (THREAD_FUNC)PlayMapAudioFile, &audiopara);
    if (ret != 0) {
        audiopara.adapter->DestroyRender(audiopara.adapter, audiopara.render);
        audiopara.manager->UnloadAdapter(audiopara.manager, audiopara.adapter);
        ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    }

    void *result = nullptr;
    pthread_join(audiopara.tids, &result);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, (intptr_t)result);
    ret = audiopara.render->attr.GetMmapPosition(audiopara.render, &frames, &(audiopara.time));
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    EXPECT_GT((audiopara.time.tvSec) * SECTONSEC + (audiopara.time.tvNSec), timeExp);
    EXPECT_GT(frames, INITIAL_VALUE);
    audiopara.render->control.Stop((AudioHandle)audiopara.render);
    audiopara.adapter->DestroyRender(audiopara.adapter, audiopara.render);
    audiopara.manager->UnloadAdapter(audiopara.manager, audiopara.adapter);
}

#ifdef AUDIO_ADM_SO
/**
* @tc.name  Test AudioRenderTurnStandbyMode API via input "AUDIO_FLUSH_COMPLETED"
* @tc.number  SUB_Audio_HDI_AudioRenderRegCallback_0001
* @tc.desc  Test AudioRenderTurnStandbyMode interface,return 0 if the interface use correctly.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderRegCallback_0001, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;

    ASSERT_NE(GetAudioManager, nullptr);
    TestAudioManager* manager = GetAudioManager();
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->RegCallback(render, AudioRenderCallback, nullptr);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->control.Flush((AudioHandle)render);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = CheckFlushValue();
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
}

/**
* @tc.name  Test AudioRenderRegCallback API via input "AUDIO_NONBLOCK_WRITE_COMPELETED"
* @tc.number  SUB_Audio_HDI_AudioRenderRegCallback_0002
* @tc.desc  Test AudioRenderRegCallback interface,return 0 if the interface use correctly.
* @tc.author: ZHANGHAILIN
*/
HWTEST_F(AudioUsbRenderTest, SUB_Audio_HDI_AudioRenderRegCallback_0002, TestSize.Level1)
{
    int32_t ret = -1;
    struct AudioAdapter *adapter = nullptr;
    struct AudioRender *render = nullptr;
    struct AudioSampleAttributes attrs;
    struct AudioHeadInfo headInfo;
    char absPath[PATH_MAX] = {0};
    realpath(AUDIO_FILE.c_str(), absPath);
    ASSERT_NE(realpath(AUDIO_FILE.c_str(), absPath), nullptr);

    FILE *file = fopen(absPath, "rb");
    ASSERT_NE(file, nullptr);
    ASSERT_NE(GetAudioManager, nullptr);
    TestAudioManager* manager = GetAudioManager();
    ret = WavHeadAnalysis(headInfo, file, attrs);
    if (ret < 0) {
        fclose(file);
        ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    }
    ret = AudioCreateRender(manager, PIN_OUT_SPEAKER, ADAPTER_NAME_USB, &adapter, &render);
    ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = render->RegCallback(render, AudioRenderCallback, nullptr);
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    ret = FrameStart(headInfo, render, file, attrs);
    if (ret < 0) {
        adapter->DestroyRender(adapter, render);
        manager->UnloadAdapter(manager, adapter);
        fclose(file);
        ASSERT_EQ(AUDIO_HAL_SUCCESS, ret);
    }

    ret = CheckWriteCompleteValue();
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);
    ret = CheckRenderFullValue();
    EXPECT_EQ(AUDIO_HAL_SUCCESS, ret);

    adapter->DestroyRender(adapter, render);
    manager->UnloadAdapter(manager, adapter);
    fclose(file);
}
#endif
}
