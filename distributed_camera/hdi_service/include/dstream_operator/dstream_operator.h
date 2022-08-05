/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_CAMERA_STREAM_OPERATOR_H
#define DISTRIBUTED_CAMERA_STREAM_OPERATOR_H

#include <map>
#include <set>
#include <vector>

#include "constants.h"
#include "dcamera.h"
#include "dcamera_stream.h"
#include "dmetadata_processor.h"

#include "json/json.h"
#include "v1_0/istream_operator.h"
#include "v1_0/types.h"

namespace OHOS {
namespace DistributedHardware {
using namespace std;
using namespace OHOS::HDI::Camera::V1_0;
class DCameraProvider;
class DStreamOperator : public IStreamOperator {
public:
    explicit DStreamOperator(std::shared_ptr<DMetadataProcessor> &dMetadataProcessor);
    DStreamOperator() = default;
    virtual ~DStreamOperator() = default;
    DStreamOperator(const DStreamOperator &other) = delete;
    DStreamOperator(DStreamOperator &&other) = delete;
    DStreamOperator& operator=(const DStreamOperator &other) = delete;
    DStreamOperator& operator=(DStreamOperator &&other) = delete;

public:
    int32_t IsStreamsSupported(OperationMode mode, const std::vector<uint8_t> &modeSetting,
        const std::vector<StreamInfo> &infos, StreamSupportType &type) override;
    int32_t CreateStreams(const std::vector<StreamInfo> &streamInfos) override;
    int32_t ReleaseStreams(const std::vector<int32_t> &streamIds) override;
    int32_t CommitStreams(OperationMode mode, const std::vector<uint8_t> &modeSetting) override;
    int32_t GetStreamAttributes(std::vector<StreamAttribute> &attributes) override;
    int32_t AttachBufferQueue(int32_t streamId, const sptr<BufferProducerSequenceable> &bufferProducer) override;
    int32_t DetachBufferQueue(int32_t streamId) override;
    int32_t Capture(int32_t captureId, const CaptureInfo &info, bool isStreaming) override;
    int32_t CancelCapture(int32_t captureId) override;
    int32_t ChangeToOfflineStream(const std::vector<int32_t> &streamIds,
        const sptr<IStreamOperatorCallback> &callbackObj, sptr<IOfflineStreamOperator> &offlineOperator) override;

    DCamRetCode InitOutputConfigurations(const DHBase &dhBase, const std::string &abilityInfo);
    DCamRetCode AcquireBuffer(int streamId, DCameraBuffer &buffer);
    DCamRetCode ShutterBuffer(int streamId, const DCameraBuffer &buffer);
    DCamRetCode SetCallBack(OHOS::sptr<IStreamOperatorCallback> const &callback);
    DCamRetCode SetDeviceCallback(function<void(ErrorType, int)> &errorCbk,
        function<void(uint64_t, std::shared_ptr<OHOS::Camera::CameraMetadata>)> &resultCbk);
    void Release();
    std::vector<int> GetStreamIds();

private:
    bool IsCapturing();
    void SetCapturing(bool isCapturing);
    DCamRetCode NegotiateSuitableCaptureInfo(const CaptureInfo& srcCaptureInfo, bool isStreaming);
    void ChooseSuitableFormat(std::vector<std::shared_ptr<DCStreamInfo>> &streamInfo,
        std::shared_ptr<DCCaptureInfo> &captureInfo);
    void ChooseSuitableResolution(std::vector<std::shared_ptr<DCStreamInfo>> &streamInfo,
        std::shared_ptr<DCCaptureInfo> &captureInfo);
    void ChooseSuitableDataSpace(std::vector<std::shared_ptr<DCStreamInfo>> &streamInfo,
        std::shared_ptr<DCCaptureInfo> &captureInfo);
    void ChooseSuitableEncodeType(std::vector<std::shared_ptr<DCStreamInfo>> &streamInfo,
        std::shared_ptr<DCCaptureInfo> &captureInfo);
    void ConvertStreamInfo(const StreamInfo &srcInfo, std::shared_ptr<DCStreamInfo> &dstInfo);
    DCEncodeType ConvertDCEncodeType(std::string &srcEncodeType);
    std::shared_ptr<DCCaptureInfo> BuildSuitableCaptureInfo(const CaptureInfo& srcCaptureInfo,
        std::vector<std::shared_ptr<DCStreamInfo>> &srcStreamInfo);
    void SnapShotStreamOnCaptureEnded(int32_t captureId, int streamId);
    bool HasContinuousCaptureInfo(int captureId);
    void ExtractStreamInfo(DCStreamInfo &dstStreamInfo, const std::shared_ptr<DCStreamInfo> &srcStreamInfo);
    void ExtractCaptureInfo(std::vector<DCCaptureInfo> &captureInfos);
    void ExtractCameraAttr(Json::Value &rootValue, std::set<int> &allFormats, std::vector<int> &photoFormats);
    DCamRetCode GetInputCaptureInfo(const CaptureInfo& srcCaptureInfo, bool isStreaming,
        std::shared_ptr<DCCaptureInfo>& inputCaptureInfo);

private:
    std::shared_ptr<DMetadataProcessor> dMetadataProcessor_;
    OHOS::sptr<IStreamOperatorCallback> dcStreamOperatorCallback_;
    function<void(ErrorType, int)> errorCallback_;

    DHBase dhBase_;
    std::vector<DCEncodeType> dcSupportedCodecType_;
    std::map<DCSceneType, std::vector<int>> dcSupportedFormatMap_;
    std::map<int, std::vector<DCResolution>> dcSupportedResolutionMap_;

    std::map<int, std::shared_ptr<DCameraStream>> halStreamMap_;
    std::map<int, std::shared_ptr<DCStreamInfo>> dcStreamInfoMap_;
    std::map<int, std::shared_ptr<CaptureInfo>> halCaptureInfoMap_;
    std::vector<std::shared_ptr<DCCaptureInfo>> cachedDCaptureInfoList_;
    std::map<int, bool> enableShutterCbkMap_;
    std::map<pair<int, int>, int> acceptedBufferNum_;

    std::mutex requestLock_;
    bool isCapturing_ = false;
    std::mutex isCapturingLock_;
    OperationMode currentOperMode_ = OperationMode::NORMAL;
    std::shared_ptr<OHOS::Camera::CameraMetadata> latestStreamSetting_;
};
} // namespace DistributedHardware
} // namespace OHOS
#endif // DISTRIBUTED_CAMERA_STREAM_OPERATOR_H