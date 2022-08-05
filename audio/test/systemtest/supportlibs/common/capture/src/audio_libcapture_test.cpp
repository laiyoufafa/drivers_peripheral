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

#include "audio_lib_common.h"
#include "audio_libcapture_test.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Audio;

namespace {
const string BIND_CONTROL = "control";
const string BIND_CAPTURE = "capture";
const string BIND_NAME_ERROR = "rendeo";

class AudioLibCaptureTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static struct DevHandle *(*BindServiceCaptureSo)(const char *serverName);
    static int32_t (*InterfaceLibOutputCapture)(struct DevHandle *handle, int cmdId,
        struct AudioHwCaptureParam *handleData);
    static int32_t (*InterfaceLibCtlCapture)(struct DevHandle *handle, int cmdId,
        struct AudioHwCaptureParam *handleData);
    static void (*CloseServiceCaptureSo)(struct DevHandle *handle);
    static void *ptrHandle;
    int32_t BindServiceAndHwCapture(struct AudioHwCapture *&hwCapture, const std::string BindName,
        const std::string adapterNameCase, struct DevHandle *&handle) const;
};

struct DevHandle *(*AudioLibCaptureTest::BindServiceCaptureSo)(const char *serverName) = nullptr;
int32_t (*AudioLibCaptureTest::InterfaceLibOutputCapture)(struct DevHandle *handle, int cmdId,
    struct AudioHwCaptureParam *handleData) = nullptr;
int32_t (*AudioLibCaptureTest::InterfaceLibCtlCapture)(struct DevHandle *handle, int cmdId,
    struct AudioHwCaptureParam *handleData) = nullptr;
void (*AudioLibCaptureTest::CloseServiceCaptureSo)(struct DevHandle *handle) = nullptr;
void *AudioLibCaptureTest::ptrHandle = nullptr;

void AudioLibCaptureTest::SetUpTestCase(void)
{
    char resolvedPath[] = HDF_LIBRARY_FULL_PATH("libhdi_audio_interface_lib_capture");
    ptrHandle = dlopen(resolvedPath, RTLD_LAZY);
    if (ptrHandle == nullptr) {
        return;
    }
    BindServiceCaptureSo = (struct DevHandle* (*)(const char *serverName))dlsym(ptrHandle, "AudioBindServiceCapture");
    InterfaceLibOutputCapture = (int32_t (*)(struct DevHandle *handle, int cmdId,
        struct AudioHwCaptureParam *handleData))dlsym(ptrHandle, "AudioInterfaceLibOutputCapture");
    InterfaceLibCtlCapture = (int32_t (*)(struct DevHandle *handle, int cmdId,
        struct AudioHwCaptureParam *handleData))dlsym(ptrHandle, "AudioInterfaceLibCtlCapture");
    CloseServiceCaptureSo = (void (*)(struct DevHandle *handle))dlsym(ptrHandle, "AudioCloseServiceCapture");
    if (BindServiceCaptureSo == nullptr || CloseServiceCaptureSo == nullptr ||
        InterfaceLibCtlCapture == nullptr || InterfaceLibOutputCapture == nullptr) {
        dlclose(ptrHandle);
        return;
    }
}

void AudioLibCaptureTest::TearDownTestCase(void)
{
    if (BindServiceCaptureSo != nullptr) {
        BindServiceCaptureSo = nullptr;
    }
    if (CloseServiceCaptureSo != nullptr) {
        CloseServiceCaptureSo = nullptr;
    }
    if (InterfaceLibCtlCapture != nullptr) {
        InterfaceLibCtlCapture = nullptr;
    }
    if (InterfaceLibOutputCapture != nullptr) {
        InterfaceLibOutputCapture = nullptr;
    }
    if (ptrHandle != nullptr) {
        dlclose(ptrHandle);
        ptrHandle = nullptr;
    }
}

void AudioLibCaptureTest::SetUp(void) {}

void AudioLibCaptureTest::TearDown(void) {}

int32_t AudioLibCaptureTest::BindServiceAndHwCapture(struct AudioHwCapture *&hwCapture,
    const std::string BindName, const std::string adapterNameCase, struct DevHandle *&handle) const
{
    if (BindServiceCaptureSo == nullptr) {
        return HDF_FAILURE;
    }
    int32_t ret = HDF_FAILURE;
    handle = BindServiceCaptureSo(BindName.c_str());
    if (handle == nullptr) {
        return HDF_FAILURE;
    }
    hwCapture = (struct AudioHwCapture *)calloc(1, sizeof(*hwCapture));
    if (hwCapture == nullptr) {
        CloseServiceCaptureSo(handle);
        return HDF_FAILURE;
    }
    ret = InitHwCapture(hwCapture, adapterNameCase);
    if (ret != HDF_SUCCESS) {
        free(hwCapture);
        hwCapture = nullptr;
        CloseServiceCaptureSo(handle);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

/**
* @tc.name  Test AudioBindServiceCapture API via legal input
* @tc.number   SUB_Audio_InterfaceLibBindServiceCapture_001
* @tc.desc  Test AudioBindServiceCapture interface,return 0 is call successfully
* @tc.author: wangkang
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibBindServiceCapture_001, TestSize.Level1)
{
    struct DevHandle *handle = nullptr;
    ASSERT_TRUE((BindServiceCaptureSo != nullptr && CloseServiceCaptureSo != nullptr));
    handle = BindServiceCaptureSo(BIND_CONTROL.c_str());
    EXPECT_NE(nullptr, handle);
    CloseServiceCaptureSo(handle);
}
/**
* @tc.name  Test AudioBindServiceCapture API via invalid input.
* @tc.number  SUB_Audio_InterfaceLibBindServiceCapture_002
* @tc.desc  test Binding failed Service which invalid service Name is rendeo.
* @tc.author: wangkang
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibBindServicecapture_002, TestSize.Level1)
{
    struct DevHandle *handle = nullptr;
    ASSERT_TRUE((BindServiceCaptureSo != nullptr && CloseServiceCaptureSo != nullptr));
    handle = BindServiceCaptureSo(BIND_NAME_ERROR.c_str());
    if (handle != nullptr) {
        CloseServiceCaptureSo(handle);
        ASSERT_EQ(nullptr, handle);
    }
    EXPECT_EQ(nullptr, handle);
}
/**
* @tc.name  Test AudioBindServiceCapture API via setting the incoming parameter handle is empty.
* @tc.number   SUB_Audio_InterfaceLibBindServiceCapture_003
* @tc.desc  test Binding failed Service, nullptr pointer passed in.
* @tc.author: wangkang
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibBindServiceCapture_003, TestSize.Level1)
{
    struct DevHandle *handle = {};
    char *bindNameNull = nullptr;
    ASSERT_TRUE((BindServiceCaptureSo != nullptr && CloseServiceCaptureSo != nullptr));
    handle = BindServiceCaptureSo(bindNameNull);
    if (handle != nullptr) {
        CloseServiceCaptureSo(handle);
        ASSERT_EQ(nullptr, handle);
    }
    EXPECT_EQ(nullptr, handle);
}
/**
* @tc.name  test BindServiceCaptureSo API via name lens is 25.
* @tc.number  SUB_Audio_InterfaceLibBindServiceCapture_004
* @tc.desc  test Binding failed Service, Log printing 'service name not support!' and 'Failed to get service!'.
* @tc.author: wangkang
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibBindServiceCapture_004, TestSize.Level1)
{
    struct DevHandle *handle = nullptr;
    ASSERT_TRUE((BindServiceCaptureSo != nullptr && CloseServiceCaptureSo != nullptr));
    string bindNameOverLen = "capturecaptureededededede";
    handle = BindServiceCaptureSo(bindNameOverLen.c_str());
    if (handle != nullptr) {
        CloseServiceCaptureSo(handle);
        ASSERT_EQ(nullptr, handle);
    }
    EXPECT_EQ(nullptr, handle);
}
/**
* @tc.name  test BindServiceCaptureSo API via name lens is 26.
* @tc.number  SUB_Audio_InterfaceLibBindServiceCapture_005
* @tc.desc  test Binding failed Service, Log printing 'Failed to snprintf_s'.
* @tc.author: wangkang
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibBindServiceCapture_005, TestSize.Level1)
{
    struct DevHandle *handle = nullptr;
    string bindNameOverLen = "capturecaptureedededededer";
    ASSERT_TRUE((BindServiceCaptureSo != nullptr && CloseServiceCaptureSo != nullptr));
    handle = BindServiceCaptureSo(bindNameOverLen.c_str());
    if (handle != nullptr) {
        CloseServiceCaptureSo(handle);
        ASSERT_EQ(nullptr, handle);
    }
    EXPECT_EQ(nullptr, handle);
}
/**
* @tc.name  test InterfaceLibCtlCapture API via writing mute value that include 1 and 0 and reading mute value.
* @tc.number  SUB_Audio_InterfaceLibCtlCapture_MuteWrite_Read_001
* @tc.desc  test InterfaceLibCtlCapture ,cmdId is AUDIODRV_CTL_IOCTL_MUTE_WRITE_CAPTURE and
*    AUDIODRV_CTL_IOCTL_MUTE_READ_CAPTURE.
* @tc.author: wangkang
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibCtlCapture_MuteWrite_Read_001, TestSize.Level1)
{
    int32_t ret = HDF_FAILURE;
    bool muteValue = 1;
    bool wishValue = 0;
    bool expectedValue = 1;
    struct DevHandle *handle = nullptr;
    struct AudioHwCapture *hwCapture = nullptr;

    ASSERT_TRUE((InterfaceLibCtlCapture != nullptr && CloseServiceCaptureSo != nullptr));
    ret = BindServiceAndHwCapture(hwCapture, BIND_CONTROL.c_str(), ADAPTER_NAME, handle);
    ASSERT_EQ(HDF_SUCCESS, ret);

    hwCapture->captureParam.captureMode.ctlParam.mute = 0;
    ret = InterfaceLibCtlCapture(handle, AUDIODRV_CTL_IOCTL_MUTE_WRITE_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibCtlCapture(handle, AUDIODRV_CTL_IOCTL_MUTE_READ_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    muteValue = hwCapture->captureParam.captureMode.ctlParam.mute;
    EXPECT_EQ(wishValue, muteValue);
    hwCapture->captureParam.captureMode.ctlParam.mute = 1;
    ret = InterfaceLibCtlCapture(handle, AUDIODRV_CTL_IOCTL_MUTE_WRITE_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibCtlCapture(handle, AUDIODRV_CTL_IOCTL_MUTE_READ_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    muteValue = hwCapture->captureParam.captureMode.ctlParam.mute;
    EXPECT_EQ(expectedValue, muteValue);
    free(hwCapture);
    hwCapture = nullptr;
    CloseServiceCaptureSo(handle);
}
/**
* @tc.name  test InterfaceLibCtlCapture API via writing mute value that is 2 and read mute value.
* @tc.number  SUB_Audio_InterfaceLibCtlCapture_MuteWrite_Read_002
* @tc.desc  test InterfaceLibCtlCapture ,cmdId is AUDIODRV_CTL_IOCTL_MUTE_WRITE_CAPTURE and
*    AUDIODRV_CTL_IOCTL_MUTE_READ_CAPTURE.
* @tc.author: wangkang
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibCtlCapture_MuteWrite_Read_002, TestSize.Level1)
{
    int32_t ret = HDF_FAILURE;
    bool muteValue = 0;
    bool wishValue = 0;
    bool expectedValue = 1;
    struct DevHandle *handle = nullptr;
    struct AudioHwCapture *hwCapture = nullptr;
    ASSERT_TRUE((InterfaceLibCtlCapture != nullptr && CloseServiceCaptureSo != nullptr));
    ret = BindServiceAndHwCapture(hwCapture, BIND_CONTROL.c_str(), ADAPTER_NAME, handle);
    ASSERT_EQ(HDF_SUCCESS, ret);

    hwCapture->captureParam.captureMode.ctlParam.mute = 2;
    ret = InterfaceLibCtlCapture(handle, AUDIODRV_CTL_IOCTL_MUTE_WRITE_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibCtlCapture(handle, AUDIODRV_CTL_IOCTL_MUTE_READ_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    muteValue = hwCapture->captureParam.captureMode.ctlParam.mute;
    EXPECT_EQ(expectedValue, muteValue);
    hwCapture->captureParam.captureMode.ctlParam.mute = 0;
    ret = InterfaceLibCtlCapture(handle, AUDIODRV_CTL_IOCTL_MUTE_WRITE_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibCtlCapture(handle, AUDIODRV_CTL_IOCTL_MUTE_READ_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    muteValue = hwCapture->captureParam.captureMode.ctlParam.mute;
    EXPECT_EQ(wishValue, muteValue);
    free(hwCapture);
    hwCapture = nullptr;
    CloseServiceCaptureSo(handle);
}
/**
* @tc.name  test InterfaceLibCtlCapture API via inputting cmdid invalid.
* @tc.number  SUB_Audio_InterfaceLibCtlCapture_Abnormal_001
* @tc.desc  test InterfaceLibCtlCapture, cmdId is invalid eg 30,so return -1.
* @tc.author: liutian
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibCtlCapture_Abnormal_001, TestSize.Level1)
{
    int32_t ret = HDF_FAILURE;
    struct DevHandle *handle = nullptr;
    struct AudioHwCapture *hwCapture = nullptr;
    ASSERT_TRUE((InterfaceLibCtlCapture != nullptr && CloseServiceCaptureSo != nullptr));
    ret = BindServiceAndHwCapture(hwCapture, BIND_CONTROL.c_str(), ADAPTER_NAME, handle);
    ASSERT_EQ(HDF_SUCCESS, ret);

    ret = InterfaceLibCtlCapture(handle, 30, &hwCapture->captureParam);
    if (ret == 0) {
        CloseServiceCaptureSo(handle);
        free(hwCapture);
        hwCapture = nullptr;
        ASSERT_EQ(HDF_FAILURE, ret);
    }
    EXPECT_EQ(HDF_FAILURE, ret);
    CloseServiceCaptureSo(handle);
    free(hwCapture);
    hwCapture = nullptr;
}
/**
* @tc.name  test InterfaceLibCtlCapture API via inputting handleData invalid.
* @tc.number  SUB_Audio_InterfaceLibCtlCapture_Abnormal_002
* @tc.desc  test InterfaceLibCtlCapture, handleData is invalid,so return -1.
* @tc.author: liutian
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibCtlCapture_Abnormal_002, TestSize.Level1)
{
    int32_t ret = HDF_FAILURE;
    struct DevHandle *handle = nullptr;
    struct AudioHwCaptureParam *handleData = nullptr;
    ASSERT_TRUE((InterfaceLibCtlCapture != nullptr && CloseServiceCaptureSo != nullptr &&
        CloseServiceCaptureSo != nullptr));
    handle = BindServiceCaptureSo(BIND_CONTROL.c_str());
    ASSERT_NE(nullptr, handle);
    ret = InterfaceLibCtlCapture(handle, AUDIO_DRV_PCM_IOCTL_READ, handleData);
    EXPECT_EQ(HDF_FAILURE, ret);
    CloseServiceCaptureSo(handle);
}
/**
* @tc.name  test InterfaceLibOutputCapture API via cmdid is AUDIO_DRV_PCM_IOCTL_HW_PARAMS.
* @tc.number  SUB_Audio_InterfaceLibOutputCapture_HwParams_001
* @tc.desc  Test AudioOutputcapture interface,return 0 is call successfully.
* @tc.author: liutian
*/
HWTEST_F(AudioLibCaptureTest,  SUB_Audio_InterfaceLibOutputCapture_HwParams_001, TestSize.Level1)
{
    int32_t ret = HDF_FAILURE;
    struct DevHandle *handle = nullptr;
    struct AudioHwCapture *hwCapture = nullptr;
    ASSERT_TRUE((InterfaceLibOutputCapture != nullptr && CloseServiceCaptureSo != nullptr));
    ret = BindServiceAndHwCapture(hwCapture, BIND_CAPTURE.c_str(), ADAPTER_NAME, handle);
    ASSERT_EQ(HDF_SUCCESS, ret);

    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_OPEN, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_HW_PARAMS, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_CLOSE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    CloseServiceCaptureSo(handle);
    free(hwCapture);
    hwCapture = nullptr;
}
/**
* @tc.name  test InterfaceLibOutputCapture API via cmdid is AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE.
* @tc.number  SUB_Audio_InterfaceLib_OutputCapture_Prepare_001
* @tc.desc  test InterfaceLibOutputCapture,cmdId is AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE.
* @tc.author: liutian
*/
HWTEST_F(AudioLibCaptureTest,  SUB_Audio_InterfaceLibOutputCapture_Prepare_001, TestSize.Level1)
{
    int32_t ret = HDF_FAILURE;
    struct AudioHwCapture *hwCapture = nullptr;
    struct DevHandle *handle = nullptr;
    ASSERT_TRUE((InterfaceLibOutputCapture != nullptr && CloseServiceCaptureSo != nullptr));
    ret = BindServiceAndHwCapture(hwCapture, BIND_CAPTURE.c_str(), ADAPTER_NAME, handle);
    ASSERT_EQ(HDF_SUCCESS, ret);

    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_OPEN, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_HW_PARAMS, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_CLOSE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    CloseServiceCaptureSo(handle);
    free(hwCapture);
    hwCapture = nullptr;
}
/**
* @tc.name  test InterfaceLibOutputCapture API via cmdid is AUDIO_DRV_PCM_IOCTRL_START_CAPTURE.
* @tc.number   SUB_Audio_InterfaceLib_OutputCapture_Start_001
* @tc.desc  test InterfaceLibOutputCapture,cmdId is AUDIO_DRV_PCM_IOCTRL_START_CAPTURE.
* @tc.author: liutian
*/
HWTEST_F(AudioLibCaptureTest,  SUB_Audio_InterfaceLibOutputCapture_Start_001, TestSize.Level1)
{
    struct DevHandle *handle = nullptr;
    struct AudioHwCapture *hwCapture = nullptr;
    int32_t ret = HDF_FAILURE;
    ASSERT_TRUE((InterfaceLibOutputCapture != nullptr && CloseServiceCaptureSo != nullptr));
    ret = BindServiceAndHwCapture(hwCapture, BIND_CAPTURE.c_str(), ADAPTER_NAME, handle);
    ASSERT_EQ(HDF_SUCCESS, ret);

    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_OPEN, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_HW_PARAMS, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_START_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_STOP_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_CLOSE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    CloseServiceCaptureSo(handle);
    free(hwCapture);
    hwCapture = nullptr;
}
/**
* @tc.name  Test AudioOutputcapture API via cmdid is AUDIO_DRV_PCM_IOCTL_READ.
            and AUDIO_DRV_PCM_IOCTRL_STOP_CAPTURE.
* @tc.number   SUB_Audio_InterfaceLibOutputCapture_Read_Stop_001
* @tc.desc  test InterfaceLibOutputCapture,cmdId is AUDIO_DRV_PCM_IOCTL_READ and AUDIO_DRV_PCM_IOCTRL_STOP_CAPTURE.
* @tc.author: liutian
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibOutputCapture_Read_Stop_001, TestSize.Level1)
{
    int32_t ret = HDF_FAILURE;
    struct DevHandle *handle = nullptr;
    struct AudioHwCapture *hwCapture = nullptr;
    ASSERT_TRUE((InterfaceLibOutputCapture != nullptr && CloseServiceCaptureSo != nullptr));
    ret = BindServiceAndHwCapture(hwCapture, BIND_CAPTURE.c_str(), ADAPTER_NAME, handle);
    ASSERT_EQ(HDF_SUCCESS, ret);

    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_OPEN, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_HW_PARAMS, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_START_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    hwCapture->captureParam.frameCaptureMode.buffer = (char *)calloc(1, 16384);
    if (hwCapture->captureParam.frameCaptureMode.buffer == nullptr) {
        CloseServiceCaptureSo(handle);
        free(hwCapture);
        hwCapture = nullptr;
        ASSERT_NE(nullptr, hwCapture);
    }
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_READ, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_STOP_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_CLOSE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    CloseServiceCaptureSo(handle);
    free(hwCapture->captureParam.frameCaptureMode.buffer);
    hwCapture->captureParam.frameCaptureMode.buffer = nullptr;
    free(hwCapture);
    hwCapture = nullptr;
}
/**
* @tc.name  Test AudioOutputcapture API data flow and control flow are serial.
* @tc.number  SUB_Audio_InterfaceLibOutputCapture_001
* @tc.desc  test Audio lib Interface CtlCapture and OutputCapture,Data stream and control stream send successful.
* @tc.author: liutian
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibOutputCapture_001, TestSize.Level1)
{
    struct DevHandle *handle1 = nullptr;
    struct AudioHwCapture *hwCapture = nullptr;
    ASSERT_TRUE((InterfaceLibOutputCapture != nullptr && InterfaceLibCtlCapture != nullptr &&
        CloseServiceCaptureSo != nullptr));
    int32_t ret = BindServiceAndHwCapture(hwCapture, BIND_CONTROL.c_str(), ADAPTER_NAME, handle1);
    ASSERT_EQ(HDF_SUCCESS, ret);
    struct DevHandle *handle2 = BindServiceCaptureSo(BIND_CAPTURE.c_str());
    if (handle2 == nullptr) {
        CloseServiceCaptureSo(handle1);
        free(hwCapture);
        ASSERT_NE(nullptr, handle2);
    }
    if (hwCapture != nullptr) {
        hwCapture->captureParam.captureMode.ctlParam.mute = 0;
        ret = InterfaceLibCtlCapture(handle1, AUDIODRV_CTL_IOCTL_MUTE_WRITE_CAPTURE, &hwCapture->captureParam);
        EXPECT_EQ(HDF_SUCCESS, ret);
        ret = InterfaceLibCtlCapture(handle1, AUDIODRV_CTL_IOCTL_MUTE_READ_CAPTURE, &hwCapture->captureParam);
        EXPECT_EQ(HDF_SUCCESS, ret);
        ret = InterfaceLibOutputCapture(handle2, AUDIO_DRV_PCM_IOCTRL_CAPTURE_OPEN, &hwCapture->captureParam);
        EXPECT_EQ(HDF_SUCCESS, ret);
        ret = InterfaceLibOutputCapture(handle2, AUDIO_DRV_PCM_IOCTL_HW_PARAMS, &hwCapture->captureParam);
        EXPECT_EQ(HDF_SUCCESS, ret);
        ret = InterfaceLibOutputCapture(handle2, AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE, &hwCapture->captureParam);
        EXPECT_EQ(HDF_SUCCESS, ret);
        ret = InterfaceLibOutputCapture(handle2, AUDIO_DRV_PCM_IOCTRL_START_CAPTURE, &hwCapture->captureParam);
        EXPECT_EQ(HDF_SUCCESS, ret);
        hwCapture->captureParam.frameCaptureMode.buffer = (char *)calloc(1, 16384);
        if (hwCapture->captureParam.frameCaptureMode.buffer == nullptr) {
            CloseServiceCaptureSo(handle1);
            CloseServiceCaptureSo(handle2);
            free(hwCapture);
            hwCapture = nullptr;
            ASSERT_NE(nullptr, hwCapture);
        }
        ret = InterfaceLibOutputCapture(handle2, AUDIO_DRV_PCM_IOCTL_READ, &hwCapture->captureParam);
        EXPECT_EQ(HDF_SUCCESS, ret);
        ret = InterfaceLibOutputCapture(handle2, AUDIO_DRV_PCM_IOCTRL_STOP_CAPTURE, &hwCapture->captureParam);
        EXPECT_EQ(HDF_SUCCESS, ret);
        ret = InterfaceLibOutputCapture(handle2, AUDIO_DRV_PCM_IOCTRL_CAPTURE_CLOSE, &hwCapture->captureParam);
        EXPECT_EQ(HDF_SUCCESS, ret);
        if (hwCapture->captureParam.frameCaptureMode.buffer != nullptr) {
            free(hwCapture->captureParam.frameCaptureMode.buffer);
        }
        free(hwCapture);
    }
    CloseServiceCaptureSo(handle1);
    CloseServiceCaptureSo(handle2);
}
/**
* @tc.name  test InterfaceLibOutputCapture API via pause.
* @tc.number  SUB_Audio_InterfaceLibctlcapture_Pause_001
* @tc.desc  test InterfaceLibOutputCapture,cmdId is AUDIODRV_CTL_IOCTL_PAUSE_WRITE.
* @tc.author: wangkang
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibOutputCapture_Pause_001, TestSize.Level1)
{
    int32_t ret = HDF_FAILURE;
    struct DevHandle *handle = {};
    struct AudioHwCapture *hwCapture = nullptr;
    ASSERT_TRUE((InterfaceLibOutputCapture != nullptr && CloseServiceCaptureSo != nullptr));
    ret = BindServiceAndHwCapture(hwCapture, BIND_CAPTURE.c_str(), ADAPTER_NAME, handle);
    ASSERT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_OPEN, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_HW_PARAMS, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_START_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    hwCapture->captureParam.captureMode.ctlParam.pause = 1;
    ret = InterfaceLibOutputCapture(handle, AUDIODRV_CTL_IOCTL_PAUSE_WRITE_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_STOP_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_CLOSE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    CloseServiceCaptureSo(handle);
    free(hwCapture);
    hwCapture = nullptr;
}
/**
* @tc.name  test InterfaceLibOutputCapture API via resume.
* @tc.number  SUB_Audio_InterfaceLibctlcapture_Resume_001
* @tc.desc  test InterfaceLibOutputCapture,cmdId is AUDIODRV_CTL_IOCTL_PAUSE_WRITE.
* @tc.author: wangkang
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibOutputCapture_Resume_001, TestSize.Level1)
{
    int32_t ret = HDF_FAILURE;
    struct AudioHwCapture *hwCapture = nullptr;
    struct DevHandle *handle = {};
    ASSERT_TRUE((InterfaceLibOutputCapture != nullptr && CloseServiceCaptureSo != nullptr));
    ret = BindServiceAndHwCapture(hwCapture, BIND_CAPTURE.c_str(), ADAPTER_NAME, handle);
    ASSERT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_OPEN, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_HW_PARAMS, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_START_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    hwCapture->captureParam.captureMode.ctlParam.pause = 0;
    ret = InterfaceLibOutputCapture(handle, AUDIODRV_CTL_IOCTL_PAUSE_WRITE_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_STOP_CAPTURE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTRL_CAPTURE_CLOSE, &hwCapture->captureParam);
    EXPECT_EQ(HDF_SUCCESS, ret);
    CloseServiceCaptureSo(handle);
    free(hwCapture);
    hwCapture = nullptr;
}
/**
* @tc.name  Test InterfaceLibOutputCapture API via setting the cmdId(30) is invalid
* @tc.number  SUB_Audio_InterfaceLibOutputCapture_Abnormal_001
* @tc.desc  test OutputCapture interface, return -1 if the cmdId is invalid.
* @tc.author: liutian
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibOutputCapture_Abnormal_001, TestSize.Level1)
{
    int32_t ret = HDF_FAILURE;
    struct DevHandle *handle = nullptr;
    struct AudioHwCapture *hwCapture = nullptr;
    ASSERT_TRUE((InterfaceLibOutputCapture != nullptr && CloseServiceCaptureSo != nullptr));
    ret = BindServiceAndHwCapture(hwCapture, BIND_CAPTURE.c_str(), ADAPTER_NAME, handle);
    ASSERT_EQ(HDF_SUCCESS, ret);

    ret = InterfaceLibOutputCapture(handle, 30, &hwCapture->captureParam);
    EXPECT_EQ(HDF_FAILURE, ret);
    CloseServiceCaptureSo(handle);
    free(hwCapture);
    hwCapture = nullptr;
}
/**
* @tc.name  Test Outputcapture API via setting the incoming parameter handleData is empty
* @tc.number  SUB_Audio_InterfaceLibOutputCapture_Abnormal_002
* @tc.desc   Test Outputcapture interface, return -1 if the incoming parameter handleData is empty.
* @tc.author: liutian
*/
HWTEST_F(AudioLibCaptureTest, SUB_Audio_InterfaceLibOutputCapture_Abnormal_002, TestSize.Level1)
{
    int32_t ret = HDF_FAILURE;
    struct DevHandle *handle = nullptr;
    struct AudioHwCaptureParam *handleData = nullptr;
    struct AudioHwCapture *hwCapture = nullptr;
    ASSERT_TRUE((InterfaceLibOutputCapture != nullptr && CloseServiceCaptureSo != nullptr));
    ret = BindServiceAndHwCapture(hwCapture, BIND_CAPTURE.c_str(), ADAPTER_NAME, handle);
    ASSERT_EQ(HDF_SUCCESS, ret);

    ret = InterfaceLibOutputCapture(handle, AUDIO_DRV_PCM_IOCTL_READ, handleData);
    EXPECT_EQ(HDF_FAILURE, ret);
    CloseServiceCaptureSo(handle);
    free(hwCapture);
    hwCapture = nullptr;
}
}