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

#include "alsa_lib_capture.h"

#define HDF_LOG_TAG HDF_AUDIO_HAL_LIB

#define AUDIO_TIMESTAMP_FREQ    8   /* Hz */
#define AUDIO_SAMPLE_FREQ       48000
#define AUDIO_PERIOD            (AUDIO_SAMPLE_FREQ / AUDIO_TIMESTAMP_FREQ)
#define AUDIO_PCM_WAIT          100
#define AUDIO_RESUME_POLL       (10 * AUDIO_PCM_WAIT) // 1s

static unsigned int g_bufferTime = 500000; /* ring buffer length in us */
static unsigned int g_periodTime = 100000; /* period time in us */
static snd_pcm_sframes_t g_bufferSize = 0;
static snd_pcm_sframes_t g_periodSize = 0;
static int g_resample       = 1;    /* enable alsa-lib resampling */
static int g_periodEvent    = 0;    /* produce poll event after each period */
static int g_canPause       = 0;    /* 0 Hardware doesn't support pause, 1 Hardware supports pause */

static int32_t AudioSetMixerCapVolume(snd_mixer_elem_t *pcmElemen, long vol)
{
    int32_t ret;

    if (pcmElemen == NULL) {
        LOG_FUN_ERR("parameter is NULL!");
        return HDF_FAILURE;
    }

    /* Judge whether it is mono or stereo */
    ret = snd_mixer_selem_is_capture_mono(pcmElemen);
    if (ret == 1) { // mono
        ret = snd_mixer_selem_set_capture_volume(pcmElemen,
                                                 SND_MIXER_SCHN_MONO, vol);
        if (ret < 0) {
            LOG_FUN_ERR("Failed to set volume: %s\n", snd_strerror(ret));
            return HDF_FAILURE;
        }
    } else { // ret == 0: is not mono. (stereo)
        ret = snd_mixer_selem_set_capture_volume_all(pcmElemen, vol);
        if (ret < 0) {
            LOG_FUN_ERR("Failed to set all channel volume: %s\n",
                        snd_strerror(ret));
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t AudioCaptureSetPauseState(snd_pcm_t *pcm, int32_t pause)
{
    int32_t ret;

    if (pcm == NULL) {
        LOG_FUN_ERR("Param is NULL!");
        return HDF_FAILURE;
    }

    if (pause == AUDIO_ALSALIB_IOCTRL_RESUME) {
        ret = snd_pcm_resume(pcm);
        if (ret == -EAGAIN) {
            /* wait until suspend flag is released */
            poll(NULL, 0, AUDIO_RESUME_POLL);
        }
        if (ret < 0) {
            ret = snd_pcm_prepare(pcm);
            if (ret < 0) {
                LOG_FUN_ERR("Resume prepare failed: %{public}s",
                            snd_strerror(ret));
                return HDF_FAILURE;
            }
        }
    } else if (pause == AUDIO_ALSALIB_IOCTRL_PAUSE) {
        ret = snd_pcm_pause(pcm, pause);
        if (ret < 0) {
            LOG_FUN_ERR("Pause fail: %{public}s", snd_strerror(ret));
            return HDF_FAILURE;
        }
    } else {
        /* Nothing to do! */
    }

    return HDF_SUCCESS;
}

int32_t AudioCtlCaptureSetPauseStu(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    int32_t pause;
    struct AudioCardInfo *cardIns;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("Param is NULL!");
        return HDF_FAILURE;
    }

    if (g_canPause == 0) {
        /* The hardware does not support pause/resume,
         * so a success message is returned.
         * The software processing scheme is implemented
         * in AudioCaptureReadFrame interface.
         */
        return HDF_SUCCESS;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns or capturePcmHandle is NULL!");
        return HDF_FAILURE;
    }

    pause = handleData->captureMode.ctlParam.pause ?
            AUDIO_ALSALIB_IOCTRL_PAUSE : AUDIO_ALSALIB_IOCTRL_RESUME;
    ret = AudioCaptureSetPauseState(cardIns->capturePcmHandle, pause);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("set pause error!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioCtlCaptureGetVolume(const struct DevHandleCapture *handle,
    int cmdId, struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    long  volEverage;
    long  volLeft = 0;
    long  volRight = 0;
    struct AudioCardInfo *cardIns;
    char *adapterName = NULL;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("AudioCtlRenderSetVolume parameter is NULL!");
        return HDF_FAILURE;
    }

    adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("Unable to obtain correct sound card information!");
        return HDF_FAILURE;
    }

    if (strncmp(adapterName, USB, strlen(USB)) == 0) {
        handleData->captureMode.ctlParam.volume = MAX_VOLUME;
        return HDF_SUCCESS;
    }

    ret = snd_mixer_handle_events(cardIns->mixer);
    if (ret < 0) {
        LOG_FUN_ERR("snd_mixer_handle_events fail: %s\n", snd_strerror(ret));
        return HDF_FAILURE;
    }

    /* Read the two channel volume */
    ret = snd_mixer_selem_get_capture_volume(cardIns->ctrlLeftVolume,
        SND_MIXER_SCHN_FRONT_LEFT, &volLeft);
    if (ret < 0) {
        LOG_FUN_ERR("Get left channel volume fail: %s\n", snd_strerror(ret));
    }
    ret = snd_mixer_selem_get_capture_volume(cardIns->ctrlLeftVolume,
        SND_MIXER_SCHN_FRONT_RIGHT, &volRight);
    if (ret < 0) {
        LOG_FUN_ERR("Get right channel volume fail: %s\n", snd_strerror(ret));
    }
    volEverage = (volLeft + volRight) >> 1;
    handleData->captureMode.ctlParam.volume = (float)(volEverage);

    return HDF_SUCCESS;
}

int32_t AudioCtlCaptureSetVolume(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    int32_t vol;
    struct AudioCardInfo *cardIns;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("parameter is NULL!");
        return HDF_FAILURE;
    }

    vol = (int32_t)handleData->captureMode.ctlParam.volume;
    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns is NULL!");
        return HDF_FAILURE;
    }

    if (strncmp(adapterName, USB, strlen(USB)) == 0) {
        /* The external Settings. */
        return HDF_SUCCESS;
    }

    ret = AudioSetMixerCapVolume(cardIns->ctrlLeftVolume, vol);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("AudioSetMixerVolume left fail!");
        return ret;
    }

    ret = AudioSetMixerCapVolume(cardIns->ctrlRightVolume, vol);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("AudioSetMixerVolume right fail!");
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t AudioCtlCaptureSetMuteStu(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    bool muteState;
    struct AudioCardInfo *cardIns;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns is NULL!");
        return HDF_FAILURE;
    }

    muteState = (bool)cardIns->captureMuteValue;
    if (muteState == false) {
        ret = AudioMixerSetCtrlMode(cardIns, adapterName,
                                    "Digital Capture mute",
                                    SND_CAP_MIC_PATH,
                                    SND_OUT_CARD_MIC_OFF);
        if (ret != HDF_SUCCESS) {
            LOG_FUN_ERR("AudioMixerSetCtrlMode failed!");
            return ret;
        }
    } else {
        ret = AudioMixerSetCtrlMode(cardIns, adapterName,
                                    "Digital Capture mute",
                                    SND_CAP_MIC_PATH,
                                    SND_OUT_CARD_MAIN_MIC);
        if (ret != HDF_SUCCESS) {
            LOG_FUN_ERR("AudioMixerSetCtrlMode failed!");
            return ret;
        }
    }
    cardIns->captureMuteValue = (int32_t)handleData->captureMode.ctlParam.mute;

    return HDF_SUCCESS;
}

int32_t AudioCtlCaptureGetMuteStu(const struct DevHandleCapture *handle,
    int cmdId, struct AudioHwCaptureParam *handleData)
{
    struct AudioCardInfo *cardIns;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns is NULL!");
        return HDF_FAILURE;
    }

    if (strncmp(adapterName, USB, strlen(USB)) == 0) {
        /* The external Settings. */
        return HDF_SUCCESS;
    }
    handleData->captureMode.ctlParam.mute = (bool)cardIns->captureMuteValue;

    return HDF_SUCCESS;
}

int32_t AudioCtlCaptureSetGainStu(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioCtlCaptureGetGainStu(const struct DevHandleCapture *handle,
    int cmdId, struct AudioHwCaptureParam *handleData)
{
    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioCtlCaptureSceneSelect(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    int32_t deviceNum;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    deviceNum = handleData->captureMode.hwInfo.pathSelect.deviceInfo.deviceNum;
    if (deviceNum < AUDIO_MIN_CARD_NUM) {
        LOG_FUN_ERR("Not find device!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioCtlCaptureGetGainThreshold(const struct DevHandleCapture *handle,
    int cmdId, struct AudioHwCaptureParam *handleData)
{
    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioCtlCaptureGetVolThreshold(const struct DevHandleCapture *handle,
    int cmdId, struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    struct AudioCardInfo *cardIns;
    long volMax = MAX_VOLUME;
    long volMin = MIN_VOLUME;

    if (handleData == NULL) {
        LOG_FUN_ERR("Param is NULL!");
        (void)DestroyCardList();
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns is NULL!");
        (void)DestroyCardList();
        return HDF_FAILURE;
    }

    if (strncmp(adapterName, USB, strlen(USB)) == 0) {
        handleData->captureMode.ctlParam.volThreshold.volMax = MAX_VOLUME;
        handleData->captureMode.ctlParam.volThreshold.volMin = MIN_VOLUME;
        return HDF_SUCCESS;
    }

    ret = snd_mixer_selem_get_capture_volume_range(cardIns->ctrlLeftVolume,
                                                   &volMin, &volMax);
    if (ret < 0) {
        LOG_FUN_ERR("Get capture volume range fail: %{public}s.", snd_strerror(ret));
        (void)CloseMixerHandle(cardIns->mixer);
        CheckCardStatus(cardIns);
        (void)DestroyCardList();
        return HDF_FAILURE;
    }
    handleData->captureMode.ctlParam.volThreshold.volMax = (int)volMax;
    handleData->captureMode.ctlParam.volThreshold.volMin = (int)volMin;

    return HDF_SUCCESS;
}

int32_t AudioCtlCaptureSetAcodecMode(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioInterfaceLibCtlCapture(const struct DevHandleCapture *handle,
    int cmdId, struct AudioHwCaptureParam *handleData)
{
    int32_t ret;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    switch (cmdId) {
        /* setPara: */
        case AUDIODRV_CTL_IOCTL_ELEM_WRITE_CAPTURE:
            ret = AudioCtlCaptureSetVolume(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_MUTE_WRITE_CAPTURE:
            ret = AudioCtlCaptureSetMuteStu(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_MUTE_READ_CAPTURE:
            ret = AudioCtlCaptureGetMuteStu(handle, cmdId, handleData);
            break;
        /* getPara: */
        case AUDIODRV_CTL_IOCTL_ELEM_READ_CAPTURE:
            ret = AudioCtlCaptureGetVolThreshold(handle, cmdId, handleData);
            ret = AudioCtlCaptureGetVolume(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_GAIN_WRITE_CAPTURE:
            ret = AudioCtlCaptureSetGainStu(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_GAIN_READ_CAPTURE:
            ret = AudioCtlCaptureGetGainStu(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_SCENESELECT_CAPTURE:
            ret = AudioCtlCaptureSceneSelect(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_GAINTHRESHOLD_CAPTURE:
            ret = AudioCtlCaptureGetGainThreshold(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_ACODEC_CHANGE_IN_CAPTURE:
            ret = AudioCtlCaptureSetAcodecMode(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_ACODEC_CHANGE_OUT_CAPTURE:
            ret = AudioCtlCaptureSetAcodecMode(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_VOL_THRESHOLD_CAPTURE:
            ret = AudioCtlCaptureGetVolThreshold(handle, cmdId, handleData);
            break;
        default:
            LOG_FUN_ERR("Ctl Mode not support!");
            ret = HDF_FAILURE;
            break;
    }

    return ret;
}

static int32_t GetCapHwParams(struct AudioCardInfo *cardIns, const struct AudioHwCaptureParam *handleData)
{
    if (cardIns == NULL || handleData == NULL) {
        LOG_FUN_ERR("Parameter is NULL!");
        return HDF_FAILURE;
    }

    cardIns->hwCaptureParams.streamType = AUDIO_CAPTURE_STREAM;
    cardIns->hwCaptureParams.channels = handleData->frameCaptureMode.attrs.channelCount;
    cardIns->hwCaptureParams.rate = handleData->frameCaptureMode.attrs.sampleRate;
    cardIns->hwCaptureParams.periodSize = handleData->frameCaptureMode.periodSize;
    cardIns->hwCaptureParams.periodCount = handleData->frameCaptureMode.periodCount;
    cardIns->hwCaptureParams.format = handleData->frameCaptureMode.attrs.format;
    cardIns->hwCaptureParams.period = handleData->frameCaptureMode.attrs.period;
    cardIns->hwCaptureParams.frameSize = handleData->frameCaptureMode.attrs.frameSize;
    cardIns->hwCaptureParams.isBigEndian = handleData->frameCaptureMode.attrs.isBigEndian;
    cardIns->hwCaptureParams.isSignedData = handleData->frameCaptureMode.attrs.isSignedData;
    cardIns->hwCaptureParams.startThreshold = handleData->frameCaptureMode.attrs.startThreshold;
    cardIns->hwCaptureParams.stopThreshold = handleData->frameCaptureMode.attrs.stopThreshold;
    cardIns->hwCaptureParams.silenceThreshold = handleData->frameCaptureMode.attrs.silenceThreshold;

    return HDF_SUCCESS;
}

static int32_t SetHWParamsSub(snd_pcm_t *handle, snd_pcm_hw_params_t *params,
    struct AudioPcmHwParams hwCapParams, snd_pcm_access_t access)
{
    int32_t ret;

    if (handle == NULL || params == NULL) {
        LOG_FUN_ERR("SetHWParamsSub parameter is null!");
        return HDF_FAILURE;
    }

    ret = snd_pcm_hw_params_set_rate_resample(handle, params, g_resample);
    if (ret < 0) {
        LOG_FUN_ERR("Resampling setup failed for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    /* set the interleaved read/write format */
    ret = snd_pcm_hw_params_set_access(handle, params, access);
    if (ret < 0) {
        LOG_FUN_ERR("Access type not available for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    /* set the sample format */
    ret = snd_pcm_hw_params_set_format(handle, params, hwCapParams.format);
    if (ret < 0) {
        LOG_FUN_ERR("Sample format not available for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    /* set the count of channels */
    ret = snd_pcm_hw_params_set_channels(handle, params, hwCapParams.channels);
    if (ret < 0) {
        LOG_FUN_ERR("Channels count (%{public}u) not available for capture: %{public}s",
                    hwCapParams.channels, snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t SetHWRate(snd_pcm_t *handle, snd_pcm_hw_params_t *params, uint32_t *rate)
{
    int32_t ret;
    uint32_t rRate;

    if (handle == NULL || params == NULL || rate == NULL) {
        LOG_FUN_ERR("SetHWParams parameter is null!");
        return HDF_FAILURE;
    }

    /* set the stream rate */
    rRate = *rate;
    ret = snd_pcm_hw_params_set_rate_near(handle, params, &rRate, 0);
    if (ret < 0) {
        LOG_FUN_ERR("Rate %{public}uHz not available for capture: %{public}s",
                    *rate, snd_strerror(ret));
        return HDF_FAILURE;
    }

    if (rRate != *rate) {
        ret = snd_pcm_hw_params_set_rate_near(handle, params, &rRate, 0);
        if (ret < 0) {
            LOG_FUN_ERR("Rate %{public}uHz not available for capture: %{public}s",
                        *rate, snd_strerror(ret));
            return HDF_FAILURE;
        }
    }
    /* Update to hardware supported rate */
    *rate = rRate;

    g_canPause = snd_pcm_hw_params_can_pause(params);

    return HDF_SUCCESS;
}

static int32_t SetHWBuffer(snd_pcm_t *handle, snd_pcm_hw_params_t *params)
{
    int32_t ret;
    int32_t dir = 0; /* dir Value range (-1,0,1) */
    snd_pcm_uframes_t size = 0;

    if (handle == NULL || params == NULL) {
        LOG_FUN_ERR("SetHWParams parameter is null!");
        return HDF_FAILURE;
    }

    ret = snd_pcm_hw_params_set_buffer_time_near(handle, params, &g_bufferTime, &dir);
    if (ret < 0) {
        LOG_FUN_ERR("Unable to set buffer time %u for capture: %{public}s",
                    g_bufferTime, snd_strerror(ret));
        return HDF_FAILURE;
    }

    ret = snd_pcm_hw_params_get_buffer_size(params, &size);
    if (ret < 0) {
        LOG_FUN_ERR("Unable to get buffer size for capture: %{public}s",
                    snd_strerror(ret));
        return HDF_FAILURE;
    }
    g_bufferSize = size;

    return HDF_SUCCESS;
}

static int32_t SetHWPeriod(snd_pcm_t *handle, snd_pcm_hw_params_t *params)
{
    int32_t ret;
    int32_t dir = 0; /* dir Value range (-1,0,1) */
    snd_pcm_uframes_t size = 0;

    if (handle == NULL || params == NULL) {
        LOG_FUN_ERR("SetHWParams parameter is null!");
        return HDF_FAILURE;
    }

    ret = snd_pcm_hw_params_set_period_time_near(handle, params, &g_periodTime, &dir);
    if (ret < 0) {
        LOG_FUN_ERR("Unable to set period time %{public}u for capture: %{public}s",
                    g_periodTime, snd_strerror(ret));
        return HDF_FAILURE;
    }

    ret = snd_pcm_hw_params_get_period_size(params, &size, &dir);
    if (ret < 0) {
        LOG_FUN_ERR("Unable to get period size for capture: %{public}s",
                    snd_strerror(ret));
        return HDF_FAILURE;
    }
    g_periodSize = size;

    return HDF_SUCCESS;
}

static int32_t SetHWParams(snd_pcm_t *handle, snd_pcm_hw_params_t *params,
    struct AudioPcmHwParams hwCapParams, snd_pcm_access_t access)
{
    int32_t ret;

    if (handle == NULL || params == NULL) {
        LOG_FUN_ERR("SetHWParams parameter is null!");
        return HDF_FAILURE;
    }

    ret = snd_pcm_hw_params_any(handle, params); // choose all parameters
    if (ret < 0) {
        LOG_FUN_ERR("Broken configuration for capture: no configurations available: %{public}s.",
                    snd_strerror(ret));
        return HDF_FAILURE;
    }

    ret = SetHWParamsSub(handle, params, hwCapParams, access);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("SetHWParamsSub failed!");
        return ret;
    }

    ret = SetHWRate(handle, params, &(hwCapParams.rate));
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("SetHWRate failed!");
        return ret;
    }

    ret = SetHWBuffer(handle, params);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("SetHWBuffer failed!");
        return ret;
    }

    ret = SetHWPeriod(handle, params);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("SetHWPeriod failed!");
        return ret;
    }

    /* write the parameters to device. */
    ret = snd_pcm_hw_params(handle, params);
    if (ret < 0) {
        LOG_FUN_ERR("Unable to set hw params for capture: %{public}s",
                    snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t SetSWParams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams)
{
    int32_t ret;
    if (handle == NULL || swparams == NULL) {
        LOG_FUN_ERR("SetHWParams parameter is null!");
        return HDF_FAILURE;
    }

    /* get the current swparams */
    ret = snd_pcm_sw_params_current(handle, swparams);
    if (ret < 0) {
        LOG_FUN_ERR("Unable to determine current swparams for capture: %{public}s.", snd_strerror(ret));
        return HDF_FAILURE;
    }

    /* start the transfer when the buffer is almost full: */
    /* (buffer_size / avail_min) * avail_min */
    ret = snd_pcm_sw_params_set_start_threshold(handle, swparams,
                                                (g_bufferSize / g_periodSize) * g_periodSize);
    if (ret < 0) {
        LOG_FUN_ERR("Unable to set start threshold mode for capture: %{public}s.", snd_strerror(ret));
        return HDF_FAILURE;
    }

    /* allow the transfer when at least period_size samples can be processed */
    /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
    ret = snd_pcm_sw_params_set_avail_min(handle, swparams, g_periodEvent ? g_bufferSize : g_periodSize);
    if (ret < 0) {
        LOG_FUN_ERR("Unable to set avail min for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    /* enable period events when requested */
    if (g_periodEvent) {
        ret = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
        if (ret < 0) {
            LOG_FUN_ERR("Unable to set period event: %{public}s", snd_strerror(ret));
            return HDF_FAILURE;
        }
    }

    /* write the parameters to the capture device */
    ret = snd_pcm_sw_params(handle, swparams);
    if (ret < 0) {
        LOG_FUN_ERR("Unable to set sw params for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioOutputCaptureHwParams(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    struct AudioCardInfo *cardIns;
    snd_pcm_hw_params_t *hwParams = NULL;
    snd_pcm_sw_params_t *swParams = NULL;

    if (handleData == NULL) {
        LOG_FUN_ERR("The parameter is empty");
        (void)DestroyCardList();
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns is NULL!");
        (void)DestroyCardList();
        return HDF_FAILURE;
    }

    ret = GetCapHwParams(cardIns, handleData);
    if (ret < 0) {
        LOG_FUN_ERR("GetCapHwParams error.");
        (void)CloseMixerHandle(cardIns->mixer);
        CheckCardStatus(cardIns);
        (void)DestroyCardList();
        return HDF_FAILURE;
    }

    snd_pcm_hw_params_alloca(&hwParams);
    snd_pcm_sw_params_alloca(&swParams);
    ret = SetHWParams(cardIns->capturePcmHandle, hwParams,
                      cardIns->hwCaptureParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) {
        LOG_FUN_ERR("Setting of hwparams failed: %{public}s", snd_strerror(ret));
        (void)CloseMixerHandle(cardIns->mixer);
        CheckCardStatus(cardIns);
        (void)DestroyCardList();
        return HDF_FAILURE;
    }
    ret = SetSWParams(cardIns->capturePcmHandle, swParams);
    if (ret < 0) {
        LOG_FUN_ERR("Setting of swparams failed: %{public}s", snd_strerror(ret));
        (void)CloseMixerHandle(cardIns->mixer);
        CheckCardStatus(cardIns);
        (void)DestroyCardList();
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t InitMixerCtrlCapVolumeRange(const char *adapterName,
    struct AudioCardInfo *cardIns)
{
    int32_t ret;
    long volMin = MIN_VOLUME;
    long volMax = MAX_VOLUME;

    if (cardIns == NULL || adapterName == NULL) {
        LOG_FUN_ERR("The parameter is NULL");
        return HDF_FAILURE;
    }

    if (strncmp(adapterName, USB, strlen(USB)) == 0) {
        /* The external Settings. */
        return HDF_SUCCESS;
    }

    if (cardIns->ctrlLeftVolume != NULL) {
        ret = snd_mixer_selem_get_capture_volume_range(cardIns->ctrlLeftVolume, &volMin, &volMax);
        if (ret < 0) {
            LOG_FUN_ERR("Failed to get capture volume range");
        }
        ret = snd_mixer_selem_set_capture_volume_range(cardIns->ctrlLeftVolume, MIN_VOLUME, MAX_VOLUME);
        if (ret < 0) {
            LOG_FUN_ERR("Failed to set capture volume range");
            return HDF_FAILURE;
        }
    }

    if (cardIns->ctrlRightVolume != NULL) {
        ret = snd_mixer_selem_get_capture_volume_range(cardIns->ctrlRightVolume, &volMin, &volMax);
        if (ret < 0) {
            LOG_FUN_ERR("Failed to get capture volume range");
        }
        ret = snd_mixer_selem_set_capture_volume_range(cardIns->ctrlRightVolume, MIN_VOLUME, MAX_VOLUME);
        if (ret < 0) {
            LOG_FUN_ERR("Failed to set capture volume range");
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t InitMixerCtlElement(const char *adapterName,
    struct AudioCardInfo *cardIns, snd_mixer_t *mixer)
{
    int32_t ret;

    if (adapterName == NULL || cardIns == NULL || mixer == NULL) {
        LOG_FUN_ERR("The parameter is empty.");
        return HDF_FAILURE;
    }

    snd_mixer_elem_t *pcmElement = snd_mixer_first_elem(mixer);
    if (strncmp(adapterName, PRIMARY, strlen(PRIMARY)) == 0) {
        ret = GetPriMixerCtlElement(cardIns, pcmElement);
        if (ret < 0) {
            LOG_FUN_ERR("Capture GetPriMixerCtlElement failed.");
            return HDF_FAILURE;
        }
    } else if (strncmp(adapterName, USB, strlen(USB)) == 0) {
        cardIns->ctrlLeftVolume = AudioUsbFindElement(mixer);
    } else {
        LOG_FUN_ERR("The selected sound card not supported, please check!");
        return HDF_FAILURE;
    }

    ret = InitMixerCtrlCapVolumeRange(adapterName, cardIns);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("InitMixerCtrlCapVolumeRange fail!");
        return ret;
    }

    ret = AudioMixerSetCtrlMode(cardIns, adapterName,
                                "Capture MIC Path",
                                SND_CAP_MIC_PATH,
                                SND_OUT_CARD_MAIN_MIC);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("AudioMixerSetCtrlMode failed!");
        return ret;
    }

    return HDF_SUCCESS;
}

/*
 * brief: Opens a capture PCM
 * param mode Open mode (see #SND_PCM_NONBLOCK, #SND_PCM_ASYNC)
 */
int32_t AudioOutputCaptureOpen(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    struct AudioCardInfo *cardIns;

    if (handleData == NULL) {
        LOG_FUN_ERR("Function parameter is NULL!");
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = AudioGetCardInfo(adapterName, SND_PCM_STREAM_CAPTURE);
    if (cardIns == NULL) {
        LOG_FUN_ERR("AudioRenderGetCardIns failed.");
        return HDF_FAILURE;
    }

    if (cardIns->capturePcmHandle != NULL) {
        LOG_FUN_ERR("Resource busy!!");
        return HDF_ERR_DEVICE_BUSY;
    }
    ret = snd_pcm_open(&cardIns->capturePcmHandle, cardIns->devName,
        SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (ret < 0) {
        LOG_FUN_ERR("Capture open device error: %s", snd_strerror(ret));
        CheckCardStatus(cardIns);
        (void)DestroyCardList();
        return HDF_FAILURE;
    }

    InitSound(&cardIns->mixer, cardIns->ctrlName);
    ret = InitMixerCtlElement(adapterName, cardIns, cardIns->mixer);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("capture InitMixerCtlElement failed!");
        (void)CloseMixerHandle(cardIns->mixer);
        CheckCardStatus(cardIns);
        (void)DestroyCardList();
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t AudioCaptureResetParams(snd_pcm_t *handle,
    struct AudioPcmHwParams audioHwParams, snd_pcm_access_t access)
{
    int32_t ret;
    snd_pcm_hw_params_t *hwParams = NULL;
    snd_pcm_sw_params_t *swParams = NULL;

    if (handle == NULL) {
        LOG_FUN_ERR("handle is NULL!");
        return HDF_FAILURE;
    }

    snd_pcm_hw_params_alloca(&hwParams);
    snd_pcm_sw_params_alloca(&swParams);
    ret = SetHWParams(handle, hwParams, audioHwParams, access);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("Setting of hwparams failed: %{public}s",
                    snd_strerror(ret));
        return HDF_FAILURE;
    }

    ret = SetSWParams(handle, swParams);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("Setting of swparams failed: %{public}s",
                    snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t CaptureDataCopy(struct AudioHwCaptureParam *handleData,
    char *buffer, uint64_t frames)
{
    int32_t ret;
    uint32_t channels;
    uint32_t format;
    uint64_t recvDataSize;

    if (handleData == NULL || buffer == NULL || frames == 0) {
        LOG_FUN_ERR("Param is NULL!");
        return HDF_FAILURE;
    }

    if (g_canPause == 0) { /* Hardware does not support pause, enable soft solution */
        if (handleData->captureMode.ctlParam.pause) {
            LOG_FUN_ERR("Currently in pause, please check!");
            return HDF_FAILURE;
        }
    }

    if (handleData->frameCaptureMode.buffer == NULL) {
        LOG_FUN_ERR("frameCaptureMode.buffer is NULL!");
        return HDF_FAILURE;
    }
    channels = handleData->frameCaptureMode.attrs.channelCount;
    format = (uint32_t)handleData->frameCaptureMode.attrs.format;
    recvDataSize = (uint64_t)(frames * channels * format);
    ret = memcpy_s(handleData->frameCaptureMode.buffer, CAPTURE_FRAME_DATA, buffer, recvDataSize);
    if (ret != EOK) {
        LOG_FUN_ERR("memcpy frame data failed!");
        return HDF_FAILURE;
    }
    handleData->frameCaptureMode.bufferSize = recvDataSize;
    handleData->frameCaptureMode.bufferFrameSize = frames;

    return HDF_SUCCESS;
}

static int32_t AudioCaptureReadFrame(struct AudioHwCaptureParam *handleData,
    struct AudioCardInfo *cardIns, snd_pcm_uframes_t bufferSize, snd_pcm_uframes_t periodSize)
{
    int32_t ret;
    long frames;
    char *buffer = NULL;

    if (handleData == NULL || cardIns == NULL) {
        LOG_FUN_ERR("Param is NULL!");
        return HDF_FAILURE;
    }
    buffer = calloc(1, bufferSize);
    if (buffer == NULL) {
        LOG_FUN_ERR("Failed to Calloc buffer");
        return HDF_FAILURE;
    }

    ret = snd_pcm_wait(cardIns->capturePcmHandle, -1);
    if (ret < 0) {
        LOG_FUN_ERR("snd_pcm_wait failed: %{public}s.", snd_strerror(ret));
        AudioMemFree((void **)&buffer);
        return HDF_FAILURE;
    }
    frames = snd_pcm_readi(cardIns->capturePcmHandle, buffer, periodSize);
    if (frames < 0) {
        frames = snd_pcm_recover(cardIns->capturePcmHandle, frames, 0);
    }
    if (frames < 0) {
        LOG_FUN_ERR("snd_pcm_writei fail: %{public}s.", snd_strerror(frames));
        AudioMemFree((void **)&buffer);
        return HDF_FAILURE;
    }

    ret = CaptureDataCopy(handleData, buffer, (uint64_t)frames);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("Failed to copy data. It may be paused. Check the status!");
        AudioMemFree((void **)&buffer);
        return HDF_FAILURE;
    }
    AudioMemFree((void **)&buffer);

    return HDF_SUCCESS;
}

int32_t AudioOutputCaptureRead(const struct DevHandleCapture *handle,
    int cmdId, struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    struct AudioCardInfo *cardIns;
    snd_pcm_uframes_t bufferSize = 0;
    snd_pcm_uframes_t periodSize = 0;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("Param is NULL!");
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns is NULL!");
        return HDF_FAILURE;
    }

    ret = snd_pcm_get_params(cardIns->capturePcmHandle, &bufferSize, &periodSize);
    if (ret < 0) {
        LOG_FUN_ERR("Get capture params error: %{public}s.", snd_strerror(ret));
        return HDF_FAILURE;
    }

    if (!cardIns->captureMmapFlag) {
        ret = AudioCaptureResetParams(cardIns->capturePcmHandle,
                                      cardIns->hwCaptureParams,
                                      SND_PCM_ACCESS_RW_INTERLEAVED);
        if (ret != HDF_SUCCESS) {
            LOG_FUN_ERR("AudioSetParamsMmap failed!");
            return ret;
        }
        ret = snd_pcm_start(cardIns->capturePcmHandle);
        if (ret < 0) {
            LOG_FUN_ERR("snd_pcm_start fail. %{public}s", snd_strerror(ret));
            return HDF_FAILURE;
        }
        cardIns->captureMmapFlag = true;
    }

    ret = AudioCaptureReadFrame(handleData, cardIns, bufferSize, periodSize);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("AudioOutputCaptureRead failed");
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t AudioOutputCaptureStart(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("Param is NULL!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioOutputCapturePrepare(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    struct AudioCardInfo *cardIns;

    if (handleData == NULL) {
        LOG_FUN_ERR("Param is NULL!");
        (void)DestroyCardList();
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns is NULL!");
        (void)DestroyCardList();
        return HDF_FAILURE;
    }

    ret = snd_pcm_prepare(cardIns->capturePcmHandle);
    if (ret < 0) {
        LOG_FUN_ERR("snd_pcm_prepare fail! %{public}s.", snd_strerror(ret));
        (void)CloseMixerHandle(cardIns->mixer);
        CheckCardStatus(cardIns);
        (void)DestroyCardList();
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioOutputCaptureClose(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    struct AudioCardInfo *cardIns;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("Parameter is NULL!");
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns is NULL!");
        return HDF_FAILURE;
    }

    if (cardIns->capturePcmHandle != NULL) {
        ret = snd_pcm_close(cardIns->capturePcmHandle);
        if (ret < 0) {
            LOG_FUN_ERR("snd_pcm_close fail: %{public}s", snd_strerror(ret));
        }
        cardIns->capturePcmHandle = NULL;
    }

    if (cardIns->cardStatus > 0) {
        cardIns->cardStatus -= 1;
    }
    if (cardIns->cardStatus == 0) {
        if (cardIns->mixer != NULL) {
            ret = snd_mixer_close(cardIns->mixer);
            if (ret < 0) {
                LOG_FUN_ERR("snd_mixer_close fail: %{public}s", snd_strerror(ret));
            }
            cardIns->mixer = NULL;
        }
        (void)memset_s(cardIns->cardName, MAX_CARD_NAME_LEN + 1, 0, MAX_CARD_NAME_LEN + 1);
        ret = DestroyCardList();
        if (ret != HDF_SUCCESS) {
            LOG_FUN_ERR("DestroyCardList failed: %{public}d.", ret);
            return ret;
        }
    }

    return HDF_SUCCESS;
}

// Stop capture first, and then close the resource
int32_t AudioOutputCaptureStop(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    struct AudioCardInfo *cardIns;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("Param is NULL!");
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns is NULL!");
        return HDF_FAILURE;
    }

    /* pass the remaining samples, otherwise they're dropped in close */
    ret = snd_pcm_drop(cardIns->capturePcmHandle);
    if (ret < 0) {
        LOG_FUN_ERR("snd_pcm_drain failed: %{public}s.", snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t UpdateSetParams(struct AudioCardInfo *cardIns)
{
    int32_t ret;

    if (cardIns == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    cardIns->captureMmapFlag = false;
    ret = AudioCaptureResetParams(cardIns->capturePcmHandle,
        cardIns->hwCaptureParams, SND_PCM_ACCESS_MMAP_INTERLEAVED);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("AudioSetParamsMmap failed!");
        return ret;
    }

    ret = snd_pcm_start(cardIns->capturePcmHandle);
    if (ret < 0) {
        LOG_FUN_ERR("snd_pcm_start fail. %{public}s.", snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t MmapDescWriteBufferCapture(const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    char *mmapAddr;
    uint32_t frameSize;
    snd_pcm_sframes_t xfer;
    snd_pcm_uframes_t size;
    struct AudioCardInfo *cardIns;

    if (handleData == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns is NULL!");
        return HDF_FAILURE;
    }

    ret = UpdateSetParams(cardIns);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("Update set params failed!");
        return HDF_FAILURE;
    }

    mmapAddr = (char *)handleData->frameCaptureMode.mmapBufDesc.memoryAddress;
    size = (snd_pcm_sframes_t)handleData->frameCaptureMode.mmapBufDesc.totalBufferFrames;
    frameSize = handleData->frameCaptureMode.attrs.channelCount * handleData->frameCaptureMode.attrs.format;
    while (size > 0) {
        xfer = snd_pcm_mmap_readi(cardIns->capturePcmHandle, mmapAddr, size);
        if (xfer < 0) {
            if (xfer == -EAGAIN) {
                snd_pcm_wait(cardIns->capturePcmHandle, AUDIO_PCM_WAIT);
                continue;
            }
            LOG_FUN_ERR("snd_pcm_mmap_readi: %{public}s", snd_strerror(xfer));
            return HDF_FAILURE;
        }

        if (xfer > 0) {
            mmapAddr += xfer * frameSize;
            size -= xfer;
            cardIns->capMmapFrames += xfer;
        }
    }

    return HDF_SUCCESS;
}

int32_t AudioOutputCaptureReqMmapBuffer(const struct DevHandleCapture *handle,
    int cmdId, const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    ret = MmapDescWriteBufferCapture(handleData);
    if (ret != HDF_SUCCESS) {
        LOG_FUN_ERR("Capture mmap write buffer failed!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t AudioOutputCaptureGetMmapPosition(const struct DevHandleCapture *handle,
    int cmdId, struct AudioHwCaptureParam *handleData)
{
    struct AudioCardInfo *cardIns;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("param is NULL!");
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    cardIns = GetCardIns(adapterName);
    if (cardIns == NULL) {
        LOG_FUN_ERR("cardIns is NULL!");
        return HDF_FAILURE;
    }
    handleData->frameCaptureMode.frames = cardIns->capMmapFrames;

    return HDF_SUCCESS;
}

struct DevHandleCapture *AudioBindServiceCaptureObject(struct DevHandleCapture * const handle,
    const char *name)
{
    int32_t ret;
    char *serviceName;
    struct HdfIoService *service;

    if (handle == NULL || name == NULL) {
        LOG_FUN_ERR("service name or handle is NULL!");
        return NULL;
    }

    serviceName = (char *)calloc(1, NAME_LEN);
    if (serviceName == NULL) {
        LOG_FUN_ERR("Failed to OsalMemCalloc serviceName");
        AudioMemFree((void **)&handle);
        return NULL;
    }

    ret = snprintf_s(serviceName, NAME_LEN - 1, SERVIC_NAME_MAX_LEN + 1, "hdf_audio_%s", name);
    if (ret < 0) {
        LOG_FUN_ERR("Failed to snprintf_s, err = %{public}d.", ret);
        AudioMemFree((void **)&serviceName);
        AudioMemFree((void **)&handle);
        return NULL;
    }

    service = HdfIoServiceBindName(serviceName);
    if (service == NULL) {
        LOG_FUN_ERR("Failed to get service!");
        AudioMemFree((void **)&serviceName);
        AudioMemFree((void **)&handle);
        return NULL;
    }
    AudioMemFree((void **)&serviceName);
    handle->object = service;

    return handle->object;
}

/* CreatCapture for Bind handle */
struct DevHandleCapture *AudioBindServiceCapture(const char *name)
{
    struct DevHandleCapture *handle = NULL;
    struct DevHandleCapture *object = NULL;

    if (name == NULL) {
        LOG_FUN_ERR("service name NULL!");
        return NULL;
    }

    handle = (struct DevHandleCapture *)calloc(1, sizeof(struct DevHandleCapture));
    if (handle == NULL) {
        LOG_FUN_ERR("Failed to OsalMemCalloc handle");
        return NULL;
    }

    object = AudioBindServiceCaptureObject(handle, name);
    if (object == NULL) {
        LOG_FUN_ERR("handle->object is NULL!");
        return NULL;
    }
    handle->object = object;
    LOG_PARA_INFO("BIND SERVICE SUCCESS!");

    return handle;
}

void AudioCloseServiceCapture(const struct DevHandleCapture *handle)
{
    if (handle != NULL) {
        if (handle->object == NULL) {
            LOG_FUN_ERR("Capture handle or handle->object is NULL");
        }
        AudioMemFree((void **)&handle);
    }
}

int32_t AudioInterfaceLibOutputCapture(const struct DevHandleCapture *handle,
    int cmdId, struct AudioHwCaptureParam *handleData)
{
    int32_t ret;

    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("Parameter is NULL!");
        return HDF_FAILURE;
    }

    switch (cmdId) {
        case AUDIO_DRV_PCM_IOCTL_HW_PARAMS:
            ret = AudioOutputCaptureHwParams(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTL_READ:
            ret = AudioOutputCaptureRead(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_START_CAPTURE:
            ret = AudioOutputCaptureStart(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE:
            ret = AudioOutputCapturePrepare(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_CAPTURE_CLOSE:
            ret = AudioOutputCaptureClose(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_CAPTURE_OPEN:
            ret = AudioOutputCaptureOpen(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_STOP_CAPTURE:
            ret = AudioOutputCaptureStop(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_PAUSE_WRITE_CAPTURE:
            ret = AudioCtlCaptureSetPauseStu(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTL_MMAP_BUFFER_CAPTURE:
            ret = AudioOutputCaptureReqMmapBuffer(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTL_MMAP_POSITION_CAPTURE:
            ret = AudioOutputCaptureGetMmapPosition(handle, cmdId, handleData);
            break;
        default:
            LOG_FUN_ERR("Output Mode not support!");
            ret = HDF_FAILURE;
            break;
    }

    return ret;
}

int32_t AudioInterfaceLibModeCapture(const struct DevHandleCapture *handle,
    struct AudioHwCaptureParam *handleData, int cmdId)
{
    LOG_FUN_INFO();
    if (handle == NULL || handleData == NULL) {
        LOG_FUN_ERR("paras is NULL!");
        return HDF_FAILURE;
    }

    switch (cmdId) {
        case AUDIO_DRV_PCM_IOCTL_HW_PARAMS:
        case AUDIO_DRV_PCM_IOCTL_READ:
        case AUDIO_DRV_PCM_IOCTRL_START_CAPTURE:
        case AUDIO_DRV_PCM_IOCTRL_STOP_CAPTURE:
        case AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE:
        case AUDIODRV_CTL_IOCTL_PAUSE_WRITE_CAPTURE:
        case AUDIO_DRV_PCM_IOCTL_MMAP_BUFFER_CAPTURE:
        case AUDIO_DRV_PCM_IOCTL_MMAP_POSITION_CAPTURE:
        case AUDIO_DRV_PCM_IOCTRL_CAPTURE_OPEN:
        case AUDIO_DRV_PCM_IOCTRL_CAPTURE_CLOSE:
            return (AudioInterfaceLibOutputCapture(handle, cmdId, handleData));
        case AUDIODRV_CTL_IOCTL_ELEM_WRITE_CAPTURE:
        case AUDIODRV_CTL_IOCTL_ELEM_READ_CAPTURE:
        case AUDIODRV_CTL_IOCTL_MUTE_WRITE_CAPTURE:
        case AUDIODRV_CTL_IOCTL_MUTE_READ_CAPTURE:
        case AUDIODRV_CTL_IOCTL_GAIN_WRITE_CAPTURE:
        case AUDIODRV_CTL_IOCTL_GAIN_READ_CAPTURE:
        case AUDIODRV_CTL_IOCTL_SCENESELECT_CAPTURE:
        case AUDIODRV_CTL_IOCTL_GAINTHRESHOLD_CAPTURE:
        case AUDIODRV_CTL_IOCTL_ACODEC_CHANGE_IN_CAPTURE:
        case AUDIODRV_CTL_IOCTL_ACODEC_CHANGE_OUT_CAPTURE:
        case AUDIODRV_CTL_IOCTL_VOL_THRESHOLD_CAPTURE:
            return (AudioInterfaceLibCtlCapture(handle, cmdId, handleData));
        default:
            LOG_FUN_ERR("Mode Error!");
            break;
    }

    return HDF_ERR_NOT_SUPPORT;
}