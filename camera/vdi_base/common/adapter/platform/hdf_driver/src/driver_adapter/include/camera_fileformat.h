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

#ifndef CAMERA_FILEFORMAT_H
#define CAMERA_FILEFORMAT_H

#include "camera_common.h"

namespace OHOS::Camera {
class CameraFileFormat {
public:
    CameraFileFormat();
    ~CameraFileFormat();
    RetCode CameraOpenDevice(struct CameraFeature feature);
    RetCode CameraCloseDevice(struct CameraFeature feature);

    RetCode CameraSetFormat(struct CameraFeature feature, CameraCtrl &ctrl);
    RetCode CameraGetFormat(struct CameraFeature feature, CameraCtrl &ctrl);
    RetCode CameraSetCrop(struct CameraFeature feature, CameraCtrl &ctrl);
    RetCode CameraGetCrop(struct CameraFeature feature, CameraCtrl &ctrl);
    RetCode CameraSetFPS(struct CameraFeature feature, CameraCtrl &ctrl);
    RetCode CameraGetFPS(struct CameraFeature feature, CameraCtrl &ctrl);
    RetCode CameraGetFmtDescs(struct CameraFeature feature, std::vector<CameraCtrl> &fmtDesc);

private:
    RetCode CameraSearchFormat(struct CameraFeature feature, std::vector<CameraCtrl> &fmtDesc);
    int EnumFmtDesc(struct CameraFeature feature, struct CameraFmtDesc &enumFmtDesc);
    int EnumFrmsize(struct CameraFeature feature, struct CameraFrmSizeDesc &frmSize);
    int Enumfrmivale(struct CameraFeature feature, struct CameraFrmRatioDesc &fraMival);
};
} // namespace OHOS::Camera
#endif // CAMERA_FILEFORMAT_H