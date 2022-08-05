/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DOUBLE_PREVIEW_H
#define DOUBLE_PREVIEW_H

#include "test_display.h"
#define PREVIEW_WIDTH 640
#define PREVIEW_HEIGHT 480
#define CAMERA_CAPTURE_WIDTH 640
#define CAMERA_CAPTURE_HEIGHT 480
#define CAMERA_VIDEO_WIDTH 640
#define CAMERA_VIDEO_HEIGHT 480

enum {
    streamId_preview_double = 1004, // double preview streamID
    captureId_preview_double = 2004, // double preview captureId
};

class DoublePreviewTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    void CreateStream(int streamId, OHOS::Camera::StreamIntent intent);
    void CommitStream();
    void SetStreamInfo(std::shared_ptr<OHOS::Camera::StreamInfo> &streamInfo,
        const std::shared_ptr<StreamCustomer> &streamCustomer,
        const int streamId, const OHOS::Camera::StreamIntent intent);
    void StartCapture(int streamId, int captureId, bool shutterCallback, bool isStreaming);
    void StopStream(std::vector<int> &captureIds, std::vector<int> &streamIds);

    OHOS::Camera::CamRetCode result_;
    std::shared_ptr<TestDisplay> display_ = nullptr;
    std::shared_ptr<StreamCustomer> streamCustomerPreview_ = nullptr;
    std::shared_ptr<StreamCustomer> streamCustomerPreviewDouble_ = nullptr;
    std::vector<std::shared_ptr<OHOS::Camera::StreamInfo>> streamInfos_;
    std::shared_ptr<OHOS::Camera::CaptureInfo> captureInfo_ = nullptr;
    std::vector<int> captureIds_;
    std::vector<int> streamIds_;
};
#endif /* DOUBLE_PREVIEW_H */