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

#include <dlfcn.h>
#include <limits.h>
#include <pthread.h>
#include <securec.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "hdf_base.h"
#include "inttypes.h"
#include "osal_mem.h"
#include "v1_0/audio_manager.h"

#define AUDIO_FUNC_LOGE(fmt, arg...)                                                     \
    do {                                                                                 \
        printf("%s: [%s]: [%d]:[ERROR]:" fmt "\n", __FILE__, __func__, __LINE__, ##arg); \
    } while (0)

#define MAX_AUDIO_ADAPTER_DESC          5
#define BUFFER_LEN                      256
#define ID_RIFF                         0x46464952
#define ID_WAVE                         0x45564157
#define ID_FMT                          0x20746d66
#define ID_DATA                         0x61746164
#define MOVE_LEFT_NUM                   8
#define AUDIO_SAMPLE_RATE_8K            8000
#define AUDIO_CHANNELCOUNT              2
#define AUDIO_SAMPLE_RATE_48K           48000
#define PATH_LEN                        256
#define DEEP_BUFFER_RENDER_PERIOD_SIZE  4096
#define DEEP_BUFFER_RENDER_PERIOD_COUNT 8
#define INT_32_MAX                      0x7fffffff
#define EXT_PARAMS_MAXLEN               107
#define SERVICE_NAME                    "idl_audio_service"

enum AudioPCMBit {
    PCM_8_BIT = 8,   /* 8-bit PCM */
    PCM_16_BIT = 16, /* 16-bit PCM */
    PCM_24_BIT = 24, /* 24-bit PCM */
    PCM_32_BIT = 32, /* 32-bit PCM */
};

enum RenderSoundCardMode {
    PRIMARY = 1,
    PRIMARY_EXT = 2,
    AUDIO_USB = 3,
    AUDIO_A2DP = 4,
};

enum RenderLoadingMode {
    DIRECT = 1,
    SERVICE = 2,
};

struct AudioHeadInfo {
    uint32_t testFileRiffId;
    uint32_t testFileRiffSize;
    uint32_t testFileFmt;
    uint32_t audioFileFmtId;
    uint32_t audioFileFmtSize;
    uint16_t audioFileFormat;
    uint16_t audioChannelNum;
    uint32_t audioSampleRate;
    uint32_t audioByteRate;
    uint16_t audioBlockAlign;
    uint16_t audioBitsPerSample;
    uint32_t dataId;
    uint32_t dataSize;
};

struct StrPara {
    struct AudioRender *render;
    FILE *file;
    struct AudioSampleAttributes attrs;
    uint64_t *replyBytes;
    char *frame;
    int32_t bufferSize;
};

struct AudioRender *g_render = NULL;
struct AudioAdapter *g_adapter = NULL;
static struct AudioManager *g_audioManager = NULL;
struct AudioDeviceDescriptor g_devDesc;
struct AudioSampleAttributes g_attrs;
struct AudioPort g_audioPort;
struct AudioHeadInfo g_wavHeadInfo;
static struct StrPara g_str;
void (*g_AudioManagerRelease)(struct AudioManager *) = NULL;
void (*g_AudioAdapterRelease)(struct AudioAdapter *) = NULL;
void (*g_AudioRenderRelease)(struct AudioRender *) = NULL;

pthread_t g_tids;
char *g_frame = NULL;
void *g_handle;
FILE *g_file;

char g_path[256];
char g_adapterName[PATH_LEN] = {0};
static int32_t g_closeEnd = 0;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_functionCond = PTHREAD_COND_INITIALIZER;
int g_waitSleep = 0;

enum RenderMenuId {
    RENDER_START = 1,
    RENDER_STOP,
    RENDER_RESUME,
    RENDER_PAUSE,
    SET_RENDER_VOLUME,
    SET_RENDER_GAIN,
    SET_RENDER_MUTE,
    SET_RENDER_ATTRIBUTES,
    SET_RENDER_SLECET_SCENE,
    GET_RENDER_EXT_PARAMS,
    GET_RENDER_POSITION,
};

enum RenderInputType {
    INPUT_INT = 0,
    INPUT_FLOAT,
    INPUT_UINT32,
};

typedef int32_t (*AudioRenderOperation)(struct AudioRender **);

struct ProcessRenderMenuSwitchList {
    enum RenderMenuId cmd;
    AudioRenderOperation operation;
};

void CleanStdin(void)
{
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

int32_t CheckInputName(int type, void *val)
{
    int ret;
    int inputInt = 0;
    float inputFloat = 0.0;
    uint32_t inputUint = 0;
    if (val == NULL) {
        return HDF_FAILURE;
    }
    printf("\n");
    switch (type) {
        case INPUT_INT:
            ret = scanf_s("%d", &inputInt);
            if (inputInt < 0 || inputInt > GET_RENDER_POSITION + 1) {
                AUDIO_FUNC_LOGE("Input failure");
                return HDF_FAILURE;
            }
            *(int *)val = inputInt;
            break;
        case INPUT_FLOAT:
            ret = scanf_s("%f", &inputFloat);
            *(float *)val = inputFloat;
            break;
        case INPUT_UINT32:
            ret = scanf_s("%u", &inputUint);
            if (inputUint > 0xFFFFFFFF || inputUint < 0) {
                return HDF_FAILURE;
            }
            *(uint32_t *)val = inputUint;
            break;
        default:
            ret = EOF;
            break;
    }
    if (ret == 0) {
        CleanStdin();
    } else if (ret == EOF) {
        AUDIO_FUNC_LOGE("Input failure occurs!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

void SystemInputFail(void)
{
    printf("please ENTER to go on...");
    while (getchar() != '\n') {
        continue;
    }
}

int32_t InitAttrs(struct AudioSampleAttributes *attrs)
{
    if (attrs == NULL) {
        return HDF_FAILURE;
    }
    /* Initialization of audio parameters for playback */
    attrs->format = AUDIO_FORMAT_PCM_16_BIT;
    attrs->channelCount = AUDIO_CHANNELCOUNT;
    attrs->sampleRate = AUDIO_SAMPLE_RATE_48K;
    attrs->interleaved = 0;
    attrs->type = AUDIO_IN_MEDIA;
    attrs->period = DEEP_BUFFER_RENDER_PERIOD_SIZE;
    attrs->frameSize = PCM_16_BIT * attrs->channelCount / PCM_8_BIT;
    attrs->isBigEndian = false;
    attrs->isSignedData = true;
    attrs->startThreshold = DEEP_BUFFER_RENDER_PERIOD_SIZE / (attrs->frameSize);
    attrs->stopThreshold = INT_32_MAX;
    attrs->silenceThreshold = 0;
    return HDF_SUCCESS;
}

int32_t InitDevDesc(struct AudioDeviceDescriptor *devDesc, uint32_t portId)
{
    if (devDesc == NULL) {
        return HDF_FAILURE;
    }
    /* Initialization of audio parameters for playback */
    devDesc->portId = portId;
    devDesc->pins = PIN_OUT_SPEAKER;
    devDesc->desc = strdup("cardname");
    return HDF_SUCCESS;
}

uint32_t StringToInt(const char *flag)
{
    if (flag == NULL) {
        return 0;
    }
    uint32_t temp = flag[0];
    for (int32_t i = strlen(flag) - 1; i >= 0; i--) {
        temp <<= MOVE_LEFT_NUM;
        temp += flag[i];
    }
    return temp;
}
int32_t WavHeadAnalysis(FILE *file, struct AudioSampleAttributes *attrs)
{
    if (file == NULL || attrs == NULL) {
        return HDF_FAILURE;
    }
    uint32_t ret;
    const char *audioRiffIdParam = "RIFF";
    const char *audioFileFmtParam = "WAVE";
    const char *aduioDataIdParam = "data";
    ret = fread(&g_wavHeadInfo, sizeof(g_wavHeadInfo), 1, file);
    if (ret != 1) {
        return HDF_FAILURE;
    }
    uint32_t audioRiffId = StringToInt(audioRiffIdParam);
    uint32_t audioFileFmt = StringToInt(audioFileFmtParam);
    uint32_t aduioDataId = StringToInt(aduioDataIdParam);
    if (g_wavHeadInfo.testFileRiffId != audioRiffId || g_wavHeadInfo.testFileFmt != audioFileFmt ||
        g_wavHeadInfo.dataId != aduioDataId) {
        return HDF_FAILURE;
    }
    attrs->channelCount = g_wavHeadInfo.audioChannelNum;
    attrs->sampleRate = g_wavHeadInfo.audioSampleRate;
    switch (g_wavHeadInfo.audioBitsPerSample) {
        case PCM_8_BIT: {
            attrs->format = AUDIO_FORMAT_PCM_8_BIT;
            break;
        }
        case PCM_16_BIT: {
            attrs->format = AUDIO_FORMAT_PCM_16_BIT;
            break;
        }
        case PCM_24_BIT: {
            attrs->format = AUDIO_FORMAT_PCM_24_BIT;
            break;
        }
        case PCM_32_BIT: {
            attrs->format = AUDIO_FORMAT_PCM_32_BIT;
            break;
        }
        default:
            return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t SwitchAdapter(struct AudioAdapterDescriptor *descs, const char *adapterNameCase,
    enum AudioPortDirection portFlag, struct AudioPort *renderPort, int32_t size)
{
    struct AudioAdapterDescriptor *desc = NULL;
    if (descs == NULL || adapterNameCase == NULL || renderPort == NULL) {
        return HDF_FAILURE;
    }
    uint32_t port;
    for (int32_t index = 0; index < size; index++) {
        desc = &descs[index];
        if (desc == NULL) {
            continue;
        }
        if (desc->adapterName == NULL) {
            return HDF_FAILURE;
        }
        if (strcmp((const char *)desc->adapterName, adapterNameCase)) {
            continue;
        }
        for (port = 0; port < desc->portsLen; port++) {
            // Only find out the port of out in the sound card
            if (desc->ports[port].dir == portFlag) {
                *renderPort = desc->ports[port];
                return index;
            }
        }
    }
    return HDF_ERR_NOT_SUPPORT;
}

void StreamClose(int32_t sig)
{
    /* allow the stream to be closed gracefully */
    (void)signal(sig, SIG_IGN);
    g_closeEnd = 1;
}

uint32_t PcmFormatToBits(enum AudioFormat formatBit)
{
    switch (formatBit) {
        case AUDIO_FORMAT_PCM_16_BIT:
            return PCM_16_BIT;
        case AUDIO_FORMAT_PCM_8_BIT:
            return PCM_8_BIT;
        default:
            return PCM_16_BIT;
    };
}

uint32_t PcmFramesToBytes(const struct AudioSampleAttributes attrs)
{
    return DEEP_BUFFER_RENDER_PERIOD_SIZE * attrs.channelCount * (PcmFormatToBits(attrs.format) >> 3);
}

int32_t StopAudioFiles(struct AudioRender **renderS)
{
    if (renderS == NULL) {
        return HDF_FAILURE;
    }
    if (g_waitSleep) {
        pthread_mutex_lock(&g_mutex);
        g_waitSleep = 0;
        pthread_cond_signal(&g_functionCond);
        pthread_mutex_unlock(&g_mutex);
    }
    if (!g_closeEnd) {
        g_closeEnd = true;
        usleep(100000); // sleep 100000us
    }
    struct AudioRender *render = *renderS;
    if (render == NULL) {
        AUDIO_FUNC_LOGE("render is null");
        return HDF_FAILURE;
    }
    int32_t ret = render->Stop((void *)render);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Stop Render!");
    }
    if (g_adapter == NULL || g_adapter->DestroyRender == NULL) {
        return HDF_FAILURE;
    }
    ret = g_adapter->DestroyRender(g_adapter);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Destroy Render!");
    }
    g_AudioRenderRelease(render);
    *renderS = NULL;
    g_render = NULL;
    if (g_frame != NULL) {
        OsalMemFree(g_frame);
        g_frame = NULL;
    }
    if (g_file != NULL) {
        fclose(g_file);
        g_file = NULL;
    }
    printf("Stop Successful\n");
    return ret;
}

int32_t FrameStartMmap(const struct StrPara *param)
{
    if (param == NULL) {
        return HDF_FAILURE;
    }
    const struct StrPara *strParam = param;
    struct AudioRender *render = strParam->render;
    struct AudioMmapBufferDescripter mmapDesc;
    (void)signal(SIGINT, StreamClose);
    // get file length
    char pathBuf[PATH_MAX] = {'\0'};
    if (realpath(g_path, pathBuf) == NULL) {
        return HDF_FAILURE;
    }
    // get fileSize
    FILE *fp = fopen(pathBuf, "rb+");
    if (fp == NULL) {
        printf("Open file failed!\n");
        return HDF_FAILURE;
    }
    int32_t ret = fseek(fp, 0, SEEK_END);
    if (ret != 0) {
        fclose(fp);
        return HDF_FAILURE;
    }
    int32_t reqSize = ftell(fp);
    if (reqSize < 0) {
        fclose(fp);
        return HDF_FAILURE;
    }
    (void)fclose(fp);
    // Init param
    mmapDesc.memoryFd = 0; // default 0
    mmapDesc.filePath = strdup(pathBuf);
    mmapDesc.isShareable = 1;                                        // 1:Shareable ,0:Don't share
    mmapDesc.transferFrameSize = DEEP_BUFFER_RENDER_PERIOD_SIZE / 4; // One frame size 4 bit
    mmapDesc.offset = sizeof(g_wavHeadInfo);
    // start
    if (render == NULL || render->ReqMmapBuffer == NULL) {
        return HDF_FAILURE;
    }
    ret = render->ReqMmapBuffer(render, reqSize, &mmapDesc);
    if (ret < 0 || reqSize <= 0) {
        printf("Request map fail,please check.\n");
        return HDF_FAILURE;
    }
    if (g_render != NULL) {
        ret = StopAudioFiles(&render);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("StopAudioFiles File!");
        }
    }
    return HDF_SUCCESS;
}

int32_t FrameStart(const struct StrPara *param)
{
    if (param == NULL) {
        return HDF_FAILURE;
    }
    const struct StrPara *strParam = param;
    struct AudioRender *render = strParam->render;
    char *frame = strParam->frame;
    int32_t bufferSize = strParam->bufferSize;
    int32_t ret;
    int32_t readSize;
    int32_t remainingDataSize = g_wavHeadInfo.testFileRiffSize;
    uint32_t numRead;
    (void)signal(SIGINT, StreamClose);
    uint64_t replyBytes;
    if (g_file == NULL) {
        return HDF_FAILURE;
    }
    if (render == NULL || render->RenderFrame == NULL || frame == NULL) {
        return HDF_FAILURE;
    }
    do {
        readSize = (remainingDataSize > bufferSize) ? bufferSize : remainingDataSize;
        numRead = fread(frame, 1, readSize, g_file);
        if (numRead > 0) {
            ret = render->RenderFrame(render, (int8_t *)frame, numRead, &replyBytes);
            if (ret == HDF_ERR_INVALID_OBJECT) {
                AUDIO_FUNC_LOGE("Render already stop!");
                break;
            }
            remainingDataSize -= numRead;
        }
        while (g_waitSleep) {
            printf("music pause now.\n");
            pthread_cond_wait(&g_functionCond, &g_mutex);
            printf("music resume now.\n");
        }
    } while (!g_closeEnd && numRead > 0 && remainingDataSize > 0);
    if (!g_closeEnd) {
        printf("\nPlay complete, please select input again\n");
        (void)StopAudioFiles(&render);
    }
    return HDF_SUCCESS;
}

void FileClose(FILE **file)
{
    if ((file != NULL) && ((*file) != NULL)) {
        fclose(*file);
        *file = NULL;
    }
    return;
}

int32_t InitPlayingAudioParam(struct AudioRender *render)
{
    if (render == NULL) {
        return HDF_FAILURE;
    }
    uint32_t bufferSize = PcmFramesToBytes(g_attrs);
    g_frame = (char *)OsalMemCalloc(bufferSize);
    if (g_frame == NULL) {
        return HDF_FAILURE;
    }
    (void)memset_s(&g_str, sizeof(struct StrPara), 0, sizeof(struct StrPara));
    g_str.render = render;
    g_str.bufferSize = bufferSize;
    g_str.frame = g_frame;
    return HDF_SUCCESS;
}

void PrintPlayMode(void)
{
    printf(" ============= Play Render Mode ==========\n");
    printf("| 1. Render non-mmap                     |\n");
    printf("| 2. Render mmap                         |\n");
    printf(" ======================================== \n");
}

int32_t SelectPlayMode(int32_t *palyModeFlag)
{
    if (palyModeFlag == NULL) {
        AUDIO_FUNC_LOGE("palyModeFlag is null");
        return HDF_FAILURE;
    }
    system("clear");
    int choice = 0;
    PrintPlayMode();
    printf("Please enter your choice:");
    int32_t ret = CheckInputName(INPUT_INT, (void *)&choice);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("CheckInputName Fail");
        return HDF_FAILURE;
    } else {
        *palyModeFlag = choice;
    }
    return HDF_SUCCESS;
}

int32_t StartPlayThread(int32_t palyModeFlag)
{
    pthread_attr_t tidsAttr;
    pthread_attr_init(&tidsAttr);
    pthread_attr_setdetachstate(&tidsAttr, PTHREAD_CREATE_DETACHED);
    switch (palyModeFlag) {
        case 1: // 1. Stander Loading
            if (pthread_create(&g_tids, &tidsAttr, (void *)(&FrameStart), &g_str) != 0) {
                AUDIO_FUNC_LOGE("Create Thread Fail");
                return HDF_FAILURE;
            }
            break;
        case 2: // 2. Low latency Loading
            if (pthread_create(&g_tids, &tidsAttr, (void *)(&FrameStartMmap), &g_str) != 0) {
                AUDIO_FUNC_LOGE("Create Thread Fail");
                return HDF_FAILURE;
            }
            break;
        default:
            printf("Input error,Switched to non-mmap Mode for you,");
            SystemInputFail();
            if (pthread_create(&g_tids, &tidsAttr, (void *)(&FrameStart), &g_str) != 0) {
                AUDIO_FUNC_LOGE("Create Thread Fail");
                return HDF_FAILURE;
            }
            break;
    }
    return HDF_SUCCESS;
}

int32_t PlayingAudioInitFile(void)
{
    if (g_file != NULL) {
        AUDIO_FUNC_LOGE("the music is playing,please stop first");
        return HDF_FAILURE;
    }
    g_closeEnd = false;
    char pathBuf[PATH_MAX] = {'\0'};
    if (realpath(g_path, pathBuf) == NULL) {
        return HDF_FAILURE;
    }
    g_file = fopen(pathBuf, "rb");
    if (g_file == NULL) {
        printf("failed to open '%s'\n", g_path);
        return HDF_FAILURE;
    }
    if (WavHeadAnalysis(g_file, &g_attrs) < 0) {
        AUDIO_FUNC_LOGE("Frame test is Fail");
        FileClose(&g_file);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t PlayingAudioInitRender(struct AudioRender **renderTemp)
{
    if (renderTemp == NULL) {
        AUDIO_FUNC_LOGE("render is null");
        return HDF_FAILURE;
    }
    struct AudioRender *render = NULL;
    int32_t ret = g_adapter->CreateRender(g_adapter, &g_devDesc, &g_attrs, &render);
    if (render == NULL || ret < 0 || render->RenderFrame == NULL) {
        AUDIO_FUNC_LOGE("AudioDeviceCreateRender failed or RenderFrame is null");
        return HDF_FAILURE;
    }
    // Playing audio files
    if (render->Start((void *)render)) {
        AUDIO_FUNC_LOGE("Start Bind Fail!");
        g_adapter->DestroyRender(g_adapter);
        g_AudioRenderRelease(render);
        return HDF_FAILURE;
    }
    if (InitPlayingAudioParam(render) < 0) {
        g_adapter->DestroyRender(g_adapter);
        g_AudioRenderRelease(render);
        return HDF_FAILURE;
    }
    *renderTemp = render;
    return HDF_SUCCESS;
}

int32_t PlayingAudioFiles(struct AudioRender **renderS)
{
    if (renderS == NULL || g_adapter == NULL || g_adapter->CreateRender == NULL) {
        return HDF_FAILURE;
    }
    if (PlayingAudioInitFile() < 0) {
        AUDIO_FUNC_LOGE("PlayingAudioInitFile Fail");
        return HDF_FAILURE;
    }
    int32_t palyModeFlag = 0;
    if (SelectPlayMode(&palyModeFlag) < 0) {
        AUDIO_FUNC_LOGE("SelectPlayMode Fail");
        FileClose(&g_file);
        return HDF_FAILURE;
    }
    struct AudioRender *render = NULL;
    if (PlayingAudioInitRender(&render) < 0) {
        AUDIO_FUNC_LOGE("PlayingAudioInitRender fail");
        FileClose(&g_file);
        return HDF_FAILURE;
    }
    if (StartPlayThread(palyModeFlag) < 0) {
        FileClose(&g_file);
        if (g_adapter != NULL && g_adapter->DestroyRender != NULL) {
            g_adapter->DestroyRender(g_adapter);
        }
        g_AudioRenderRelease(render);
        return HDF_FAILURE;
    }

    *renderS = render;
    printf("Start Successful,Music is playing\n");
    return HDF_SUCCESS;
}

void PrintMenu0(void)
{
    printf(" ============= Play Render Sound Card Mode ==========\n");
    printf("| 1. Render Primary                                 |\n");
    printf("| 2. Render Primary_Ext                             |\n");
    printf(" =================================================== \n");
}

void PrintMenu1(void)
{
    printf(" ============== Play Render Loading Mode ===========\n");
    printf("| 1. Render Direct Loading                         |\n");
    printf("| 2. Render Service Loading                        |\n");
    printf("| Note: switching is not supported in the MPI's    |\n");
    printf("|       version.                                   |\n");
    printf(" ================================================== \n");
}

int32_t SwitchInternalOrExternal(char *adapterNameCase, int32_t nameLen)
{
    system("clear");
    int choice = 0;
    PrintMenu0();
    printf("Please enter your choice:");
    int32_t ret = CheckInputName(INPUT_INT, (void *)&choice);
    if (ret < 0) {
        return HDF_FAILURE;
    }
    switch (choice) {
        case PRIMARY:
            snprintf_s(adapterNameCase, nameLen, nameLen - 1, "%s", "primary");
            break;
        case PRIMARY_EXT:
            snprintf_s(adapterNameCase, nameLen, nameLen - 1, "%s", "primary_ext");
            break;
        default:
            printf("Input error,Switched to Acodec in for you,");
            SystemInputFail();
            snprintf_s(adapterNameCase, nameLen, nameLen - 1, "%s", "primary");
            break;
    }
    return HDF_SUCCESS;
}

int32_t SelectLoadingMode(char *resolvedPath, int32_t pathLen)
{
    system("clear");
    int choice = 0;
    char *soPathHdi = NULL;
    char *soPathProxy = NULL;
    soPathHdi = HDF_LIBRARY_FULL_PATH("libhdi_audio_passthrough");
#ifdef __aarch64__
    soPathProxy = "/system/lib64/libaudio_proxy_1.0.z.so";
#else
    soPathProxy = "/system/lib/libaudio_proxy_1.0.z.so";
#endif

    PrintMenu1();
    printf("Please enter your choice:");
    int32_t ret = CheckInputName(INPUT_INT, (void *)&choice);
    if (ret < 0) {
        return HDF_FAILURE;
    }

    switch (choice) {
        case DIRECT:
            if (snprintf_s(resolvedPath, pathLen, pathLen - 1, "%s", soPathHdi) < 0) {
                return HDF_FAILURE;
            }
            break;
        case SERVICE:
            if (snprintf_s(resolvedPath, pathLen, pathLen - 1, "%s", soPathProxy) < 0) {
                return HDF_FAILURE;
            }
            break;
        default:
            printf("Input error,Switched to direct loading in for you,");
            SystemInputFail();
            if (snprintf_s(resolvedPath, pathLen, pathLen - 1, "%s", soPathHdi) < 0) {
                return HDF_FAILURE;
            }
            break;
    }

    return HDF_SUCCESS;
}

void AudioAdapterDescriptorFree(struct AudioAdapterDescriptor *dataBlock, bool freeSelf)
{
    if (dataBlock == NULL) {
        return;
    }

    if (dataBlock->adapterName != NULL) {
        OsalMemFree(dataBlock->adapterName);
        dataBlock->adapterName = NULL;
    }

    if (dataBlock->ports != NULL) {
        OsalMemFree(dataBlock->ports);
    }

    if (freeSelf) {
        OsalMemFree(dataBlock);
    }
}

void ReleaseAdapterDescs(struct AudioAdapterDescriptor **descs, uint32_t descsLen)
{
    if (descsLen > 0 && descs != NULL && (*descs) != NULL) {
        for (uint32_t i = 0; i < descsLen; i++) {
            AudioAdapterDescriptorFree(&(*descs)[i], false);
        }
        OsalMemFree(*descs);
        *descs = NULL;
    }
}

int32_t GetManagerAndLoadAdapter(const char *adapterNameCase, struct AudioPort *renderPort)
{
    if (adapterNameCase == NULL || renderPort == NULL) {
        AUDIO_FUNC_LOGE("The Parameter is NULL");
        return HDF_FAILURE;
    }

    struct AudioManager *(*getAudioManager)(const char *) = NULL;
    getAudioManager = (struct AudioManager *(*)(const char *))(dlsym(g_handle, "AudioManagerGetInstance"));
    if (getAudioManager == NULL) {
        return HDF_FAILURE;
    }
    struct AudioManager *audioManagerIns = getAudioManager(SERVICE_NAME);
    if (audioManagerIns == NULL) {
        AUDIO_FUNC_LOGE("Get audio Manager Fail");
        return HDF_FAILURE;
    }
    g_audioManager = audioManagerIns;
    struct AudioAdapterDescriptor *descs = (struct AudioAdapterDescriptor *)OsalMemCalloc(
        sizeof(struct AudioAdapterDescriptor) * (MAX_AUDIO_ADAPTER_DESC));
    if (descs == NULL) {
        AUDIO_FUNC_LOGE("OsalMemCalloc for descs failed");
        return HDF_FAILURE;
    }
    uint32_t adapterNum = MAX_AUDIO_ADAPTER_DESC;
    int32_t ret = audioManagerIns->GetAllAdapters(audioManagerIns, descs, &adapterNum);
    if (ret < 0 || adapterNum == 0) {
        AUDIO_FUNC_LOGE("Get All Adapters Fail");
        ReleaseAdapterDescs(&descs, MAX_AUDIO_ADAPTER_DESC);
        return HDF_ERR_NOT_SUPPORT;
    }
    // Get qualified sound card and port
    enum AudioPortDirection port = PORT_OUT; // Set port information
    int32_t index = SwitchAdapter(descs, adapterNameCase, port, renderPort, adapterNum);
    if (index < 0) {
        AUDIO_FUNC_LOGE("Not Switch Adapter Fail");
        ReleaseAdapterDescs(&descs, MAX_AUDIO_ADAPTER_DESC);
        return HDF_ERR_NOT_SUPPORT;
    }
    if (audioManagerIns->LoadAdapter(audioManagerIns, &descs[index], &g_adapter)) {
        AUDIO_FUNC_LOGE("Load Adapter Fail");
        ReleaseAdapterDescs(&descs, MAX_AUDIO_ADAPTER_DESC);
        return HDF_ERR_NOT_SUPPORT;
    }
    ReleaseAdapterDescs(&descs, MAX_AUDIO_ADAPTER_DESC);
    if (g_adapter == NULL) {
        AUDIO_FUNC_LOGE("load audio device failed");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t InitRenderParam(const char *adapterNameCase, uint32_t portId)
{
    if (adapterNameCase == NULL) {
        AUDIO_FUNC_LOGE("The Parameter is NULL");
        return HDF_FAILURE;
    }

    if (g_adapter == NULL) {
        AUDIO_FUNC_LOGE("g_adapter is NULL");
        return HDF_FAILURE;
    }

    // Initialization port information, can fill through mode and other parameters
    (void)g_adapter->InitAllPorts(g_adapter);
    // User needs to set
    if (InitAttrs(&g_attrs) < 0) {
        AUDIO_FUNC_LOGE("InitAttrs failed");
        return HDF_FAILURE;
    }
    // Specify a hardware device
    if (InitDevDesc(&g_devDesc, portId) < 0) {
        AUDIO_FUNC_LOGE("InitDevDesc failed");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t RenderGetAdapterAndInitEnvParams(const char *adapterNameCase)
{
    struct AudioPort renderPort;
    int32_t ret = GetManagerAndLoadAdapter(adapterNameCase, &renderPort);
    if (ret < 0) {
        return ret;
    }
    if (InitRenderParam(adapterNameCase, renderPort.portId) < 0) {
        g_audioManager->UnloadAdapter(g_audioManager, adapterNameCase);
        g_AudioAdapterRelease(g_adapter);
        g_adapter = NULL;
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t InitReleaseFun(void)
{
    g_AudioManagerRelease = (void (*)(struct AudioManager *))(dlsym(g_handle, "AudioManagerRelease"));
    if (g_AudioManagerRelease == NULL) {
        AUDIO_FUNC_LOGE("get AudioManagerRelease fun ptr failed");
        return HDF_FAILURE;
    }
    g_AudioAdapterRelease = (void (*)(struct AudioAdapter *))(dlsym(g_handle, "AudioAdapterRelease"));
    if (g_AudioAdapterRelease == NULL) {
        AUDIO_FUNC_LOGE("get AudioAdapterRelease fun ptr failed");
        return HDF_FAILURE;
    }
    g_AudioRenderRelease = (void (*)(struct AudioRender *))(dlsym(g_handle, "AudioRenderRelease"));
    if (g_AudioRenderRelease == NULL) {
        AUDIO_FUNC_LOGE("get AudioRenderRelease fun ptr failed");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}
int32_t InitParam(void)
{
    /* Internal and external switch,begin */
    if (SwitchInternalOrExternal(g_adapterName, PATH_LEN) < 0) {
        return HDF_FAILURE;
    }
    char resolvedPath[PATH_LEN] = {0}; // Select loading mode,begin
    if (SelectLoadingMode(resolvedPath, PATH_LEN) < 0) {
        return HDF_FAILURE;
    }
    /* Select loading mode,end */
    g_audioPort.dir = PORT_OUT;
    g_audioPort.portId = 0;
    g_audioPort.portName = "AOP";
    g_handle = dlopen(resolvedPath, 1);
    if (g_handle == NULL) {
        AUDIO_FUNC_LOGE("Open so Fail, reason:%s", dlerror());
        return HDF_FAILURE;
    }
    if (InitReleaseFun() < 0) {
        AUDIO_FUNC_LOGE("InitReleaseFun Fail");
        dlclose(g_handle);
        return HDF_FAILURE;
    }
    if (RenderGetAdapterAndInitEnvParams(g_adapterName) < 0) {
        AUDIO_FUNC_LOGE("GetProxyManagerFunc Fail");
        if (g_audioManager != NULL) {
            g_AudioManagerRelease(g_audioManager);
            g_audioManager = NULL;
        }
        dlclose(g_handle);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t SetRenderMute(struct AudioRender **render)
{
    (void)render;
    int32_t val;
    bool isMute = false;
    int32_t ret;
    if (g_render == NULL || g_render->GetMute == NULL) {
        return HDF_FAILURE;
    }
    ret = g_render->GetMute((void *)g_render, &isMute);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("The current mute state was not obtained!");
    }
    printf("Now %s ,Do you need to set mute status(1/0):", isMute ? "mute" : "not mute");
    ret = CheckInputName(INPUT_INT, (void *)&val);
    if (ret < 0) {
        return HDF_FAILURE;
    }
    if (isMute != 0 && isMute != 1) {
        AUDIO_FUNC_LOGE("Invalid value!");
        SystemInputFail();
        return HDF_FAILURE;
    }
    if (g_render == NULL || g_render->SetMute == NULL) {
        AUDIO_FUNC_LOGE("Music already stop!");
        SystemInputFail();
        return HDF_FAILURE;
    }
    if (val == 1) {
        ret = g_render->SetMute((void *)g_render, !isMute);
    }
    return ret;
}

int32_t SetRenderVolume(struct AudioRender **render)
{
    (void)render;
    int32_t ret;
    float val = 0.0;
    if (g_render == NULL || g_render->GetVolume == NULL) {
        return HDF_FAILURE;
    }
    ret = g_render->GetVolume((void *)g_render, &val);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Get current volume failed!");
        SystemInputFail();
        return ret;
    }
    printf("Now the volume is %f ,Please enter the volume value you want to set (0.0-1.0):", val);
    ret = CheckInputName(INPUT_FLOAT, (void *)&val);
    if (ret < 0) {
        return HDF_FAILURE;
    }
    if (val < 0.0 || val > 1.0) {
        AUDIO_FUNC_LOGE("Invalid volume value!");
        SystemInputFail();
        return HDF_FAILURE;
    }
    if (g_render == NULL || g_render->SetVolume == NULL) {
        AUDIO_FUNC_LOGE("Music already stop!");
        SystemInputFail();
        return HDF_FAILURE;
    }
    ret = g_render->SetVolume((void *)g_render, val);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("set volume fail!");
        SystemInputFail();
    }
    return ret;
}

int32_t GetRenderGain(struct AudioRender **render)
{
    (void)render;
    int32_t ret;
    float val = 1.0;
    if (g_render == NULL || g_render->GetGain == NULL) {
        return HDF_FAILURE;
    }
    ret = g_render->GetGain((void *)g_render, &val);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Get current gain failed!");
        SystemInputFail();
        return HDF_FAILURE;
    }
    printf("Now the gain is %f,", val);
    SystemInputFail();
    return HDF_SUCCESS;
}

int32_t SetRenderPause(struct AudioRender **render)
{
    (void)render;
    if (g_waitSleep) {
        AUDIO_FUNC_LOGE("Already pause,not need pause again!");
        SystemInputFail();
        return HDF_FAILURE;
    }
    int32_t ret;
    if (g_render == NULL || g_render->Pause == NULL) {
        return HDF_FAILURE;
    }
    ret = g_render->Pause((void *)g_render);
    if (ret != 0) {
        return HDF_FAILURE;
    }
    printf("Pause success!\n");
    g_waitSleep = 1;
    return HDF_SUCCESS;
}

int32_t SetRenderResume(struct AudioRender **render)
{
    (void)render;
    if (!g_waitSleep) {
        AUDIO_FUNC_LOGE("Now is Playing,not need resume!");
        SystemInputFail();
        return HDF_FAILURE;
    }
    int32_t ret;
    if (g_render == NULL || g_render->Resume == NULL) {
        return HDF_FAILURE;
    }
    ret = g_render->Resume((void *)g_render);
    if (ret != 0) {
        return HDF_FAILURE;
    }
    printf("resume success!\n");
    pthread_mutex_lock(&g_mutex);
    g_waitSleep = 0;
    pthread_cond_signal(&g_functionCond);
    pthread_mutex_unlock(&g_mutex);
    return HDF_SUCCESS;
}
void PrintAttributesFromat(void)
{
    printf(" ============= Render Sample Attributes Fromat =============== \n");
    printf("| 1. Render AUDIO_FORMAT_PCM_8_BIT                            |\n");
    printf("| 2. Render AUDIO_FORMAT_PCM_16_BIT                           |\n");
    printf("| 3. Render AUDIO_FORMAT_PCM_24_BIT                           |\n");
    printf("| 4. Render AUDIO_FORMAT_PCM_32_BIT                           |\n");
    printf(" ============================================================= \n");
}
int32_t SelectAttributesFomat(uint32_t *fomat)
{
    if (fomat == NULL) {
        AUDIO_FUNC_LOGE("fomat is null!");
        return HDF_FAILURE;
    }
    PrintAttributesFromat();
    printf("Please select audio format,If not selected, the default is 16bit:");
    int32_t ret;
    int val = 0;
    ret = CheckInputName(INPUT_INT, (void *)&val);
    if (ret < 0) {
        return HDF_FAILURE;
    }
    switch (val) {
        case AUDIO_FORMAT_PCM_8_BIT:
            *fomat = AUDIO_FORMAT_PCM_8_BIT;
            break;
        case AUDIO_FORMAT_PCM_16_BIT:
            *fomat = AUDIO_FORMAT_PCM_16_BIT;
            break;
        case AUDIO_FORMAT_PCM_24_BIT:
            *fomat = AUDIO_FORMAT_PCM_24_BIT;
            break;
        case AUDIO_FORMAT_PCM_32_BIT:
            *fomat = AUDIO_FORMAT_PCM_32_BIT;
            break;
        default:
            *fomat = AUDIO_FORMAT_PCM_16_BIT;
            break;
    }
    return HDF_SUCCESS;
}

int32_t SetRenderAttributes(struct AudioRender **render)
{
    (void)render;
    struct AudioSampleAttributes attrs;
    if (g_render == NULL || g_render->GetSampleAttributes == NULL) {
        AUDIO_FUNC_LOGE("The pointer is null!");
        return HDF_FAILURE;
    }
    int32_t ret = g_render->GetSampleAttributes((void *)g_render, &attrs);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("GetRenderAttributes failed!");
    } else {
        printf("Current sample attributes:\n");
        printf("audioType is %u\nfomat is %u\nsampleRate is %u\nchannalCount is"
               "%u\nperiod is %u\nframesize is %u\nbigEndian is %u\nSignedData is %u\n",
            attrs.type, attrs.format, attrs.sampleRate, attrs.channelCount, attrs.period, attrs.frameSize,
            attrs.isBigEndian, attrs.isSignedData);
    }
    printf("Set Sample Attributes,");
    SystemInputFail();
    system("clear");
    printf("The sample attributes you want to set,Step by step, please.\n");
    ret = SelectAttributesFomat((uint32_t *)(&attrs.format));
    if (ret < 0) {
        AUDIO_FUNC_LOGE("SetRenderAttributes format failed!");
        return HDF_FAILURE;
    }
    printf("\nPlease input sample rate(48000,44100,32000...):");
    ret = CheckInputName(INPUT_UINT32, (void *)(&attrs.sampleRate));
    if (ret < 0) {
        return HDF_FAILURE;
    }
    printf("\nPlease input bigEndian(false=0/true=1):");
    ret = CheckInputName(INPUT_UINT32, (void *)(&attrs.isBigEndian));
    if (ret < 0) {
        return HDF_FAILURE;
    }
    if (g_render == NULL || g_render->SetSampleAttributes == NULL) {
        AUDIO_FUNC_LOGE("Music already complete,Please replay and set the attrbutes!");
        SystemInputFail();
        return HDF_FAILURE;
    }
    ret = g_render->SetSampleAttributes((void *)g_render, &attrs);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Set render attributes failed!");
        SystemInputFail();
    }
    return ret;
}

int32_t SelectRenderScene(struct AudioRender **render)
{
    (void)render;
    system("clear");
    int32_t ret;
    int val = 0;
    struct AudioSceneDescriptor scene;
    printf(" =================== Select Scene ===================== \n");
    printf("0 is Speaker.                                          |\n");
    printf("1 is HeadPhones.                                       |\n");
    printf(" ====================================================== \n");
    printf("Please input your choice:\n");
    ret = CheckInputName(INPUT_INT, (void *)&val);
    if (ret < 0 || (val != 0 && val != 1)) {
        AUDIO_FUNC_LOGE("Invalid value!");
        SystemInputFail();
        return HDF_FAILURE;
    }
    if (val == 1) {
        scene.scene.id = 0;
        scene.desc.pins = PIN_OUT_HEADSET;
    } else {
        scene.scene.id = 0;
        scene.desc.pins = PIN_OUT_SPEAKER;
    }
    scene.desc.desc = "mic";
    if (g_render == NULL) {
        AUDIO_FUNC_LOGE("Music already stop,");
        SystemInputFail();
        return HDF_FAILURE;
    }
    if (g_render->SelectScene == NULL) {
        return HDF_FAILURE;
    }
    ret = g_render->SelectScene((void *)g_render, &scene);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Select scene fail\n");
    }
    return ret;
}

int32_t GetExtParams(struct AudioRender **render)
{
    (void)render;
    char keyValueList[BUFFER_LEN] = {0};
    int32_t ret;
    if (g_render == NULL || g_render->GetExtraParams == NULL) {
        return HDF_FAILURE;
    }
    ret = g_render->GetExtraParams((void *)g_render, keyValueList, EXT_PARAMS_MAXLEN);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Get EXT params failed!");
        SystemInputFail();
        return HDF_FAILURE;
    }
    printf("keyValueList = %s\n", keyValueList);
    return HDF_SUCCESS;
}

int32_t GetRenderMmapPosition(struct AudioRender **render)
{
    (void)render;
    int32_t ret;
    if (g_render == NULL || g_render->GetMmapPosition == NULL) {
        return HDF_FAILURE;
    }
    uint64_t frames = 0;
    struct AudioTimeStamp time;
    time.tvNSec = 0;
    time.tvSec = 0;
    ret = g_render->GetMmapPosition((void *)g_render, &frames, &time);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Get current Mmap frames Position failed!");
        SystemInputFail();
        return HDF_FAILURE;
    }
    printf("Now the Position is %" PRIu64 "\n", frames);
    return HDF_SUCCESS;
}

void PrintMenu2(void)
{
    printf(" ================== Play Render Menu ================== \n");
    printf("| 1. Render Start                                      |\n");
    printf("| 2. Render Stop                                       |\n");
    printf("| 3. Render Resume                                     |\n");
    printf("| 4. Render Pause                                      |\n");
    printf("| 5. Render SetVolume                                  |\n");
    printf("| 6. Render GetGain                                    |\n");
    printf("| 7. Render SetMute                                    |\n");
    printf("| 8. Render SetAttributes                              |\n");
    printf("| 9. Render SelectScene                                |\n");
    printf("| 10. Render getEXtParams                              |\n");
    printf("| 11. Render getMmapPosition                           |\n");
    printf("| 12.Exit                                              |\n");
    printf(" ====================================================== \n");
}

static struct ProcessRenderMenuSwitchList g_processRenderMenuSwitchList[] = {
    {RENDER_START, PlayingAudioFiles},
    {RENDER_STOP, StopAudioFiles},
    {RENDER_RESUME, SetRenderResume},
    {RENDER_PAUSE, SetRenderPause},
    {SET_RENDER_VOLUME, SetRenderVolume},
    {SET_RENDER_GAIN, GetRenderGain},
    {SET_RENDER_MUTE, SetRenderMute},
    {SET_RENDER_ATTRIBUTES, SetRenderAttributes},
    {SET_RENDER_SLECET_SCENE, SelectRenderScene},
    {GET_RENDER_EXT_PARAMS, GetExtParams},
    {GET_RENDER_POSITION, GetRenderMmapPosition},
};

void ProcessMenu(int32_t choice)
{
    int32_t i;
    if (choice == GET_RENDER_POSITION + 1) {
        AUDIO_FUNC_LOGE("Exit from application program!");
        return;
    }
    if (g_render == NULL && choice != 1) {
        AUDIO_FUNC_LOGE("This render already release!");
        SystemInputFail();
        return;
    }
    for (i = RENDER_START; i <= GET_RENDER_POSITION; ++i) {
        if ((choice == (int32_t)g_processRenderMenuSwitchList[i - 1].cmd) &&
            (g_processRenderMenuSwitchList[i - 1].operation != NULL)) {
            g_processRenderMenuSwitchList[i - 1].operation(&g_render);
        }
    }
}

void Choice(void)
{
    int32_t choice = 0;
    int ret;
    while (choice < GET_RENDER_POSITION + 1 && choice >= 0) {
        system("clear");
        PrintMenu2();
        printf("your choice is:\n");
        ret = CheckInputName(INPUT_INT, (void *)&choice);
        if (ret < 0) {
            continue;
        }
        if (choice < RENDER_START || choice > GET_RENDER_POSITION + 1) {
            AUDIO_FUNC_LOGE("You input is wrong!");
            choice = 0;
            SystemInputFail();
            continue;
        }
        ProcessMenu(choice);
    }
}

int32_t main(int32_t argc, char const *argv[])
{
    if (argc < 2 || argv == NULL || argv[0] == NULL) { // The parameter number is not greater than 2
        printf("usage:[1]sample [2]/data/test.wav\n");
        return 0;
    }
    if (argv[1] == NULL || strlen(argv[1]) == 0) {
        return HDF_FAILURE;
    }
    int32_t ret = 0;
    ret = strncpy_s(g_path, PATH_LEN - 1, argv[1], strlen(argv[1]) + 1);
    if (ret != 0) {
        return HDF_FAILURE;
    }

    char pathBuf[PATH_MAX] = {'\0'};
    if (realpath(g_path, pathBuf) == NULL) {
        return HDF_FAILURE;
    }

    FILE *file = fopen(pathBuf, "rb");
    if (file == NULL) {
        printf("Failed to open '%s',Please enter the correct file name \n", g_path);
        return HDF_FAILURE;
    }
    (void)fclose(file);
    if (InitParam()) { // init
        AUDIO_FUNC_LOGE("InitParam Fail!");
        return HDF_FAILURE;
    }
    Choice();
    if (g_render != NULL && g_adapter != NULL) {
        StopAudioFiles(&g_render);
    }
    if (g_audioManager != NULL && g_audioManager->UnloadAdapter != NULL) {
        g_audioManager->UnloadAdapter(g_audioManager, g_adapterName);
        g_AudioAdapterRelease(g_adapter);
        g_adapter = NULL;
        g_AudioManagerRelease(g_audioManager);
        g_audioManager = NULL;
    }
    dlclose(g_handle);
    return 0;
}