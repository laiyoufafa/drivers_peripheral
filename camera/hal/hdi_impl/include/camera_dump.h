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

#ifndef HDI_CAMERA_DUMP_H
#define HDI_CAMERA_DUMP_H

#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "camera_metadata_operator.h"
#include "camera_metadata_info.h"
#include "devhost_dump_reg.h"
#include "hdf_sbuf.h"
#include "ibuffer.h"

namespace OHOS::Camera {
enum DumpType {
    MedataType,
    BufferType
};

class CameraDumper {
public:
    ~CameraDumper ();
    bool DumpBuffer(const std::shared_ptr<IBuffer>& buffer);
    bool DumpMetadata(const std::shared_ptr<CameraMetadata>& metadata, std::string tag = "camera");
    void ShowDumpMenu(struct HdfSBuf *reply);
    void CameraHostDumpProcess(struct HdfSBuf *data, struct HdfSBuf *reply);
    static CameraDumper& GetInstance()
    {
        static CameraDumper instance_;
        return instance_;
    }

private:
    CameraDumper() {}
    bool IsDumpOpened(DumpType type);
    uint64_t GetCurrentLocalTimeStamp();
    void UpdateDumpMode(DumpType type, bool isDump, struct HdfSBuf *reply);
    bool SaveDataToFile(const char *fileName, const void *data, uint32_t size);
    void CheckDiskInfo();
    void ThreadWorkFun();
    void StartCheckDiskInfo();
    void StopCheckDiskInfo();

private:
    std::mutex dumpStateLock_;
    std::condition_variable cv_;
    std::mutex terminateLock_;
    bool terminate_ = true;
    std::unique_ptr<std::thread> handleThread_ = nullptr;
};

int32_t CameraDumpEvent(struct HdfSBuf *data, struct HdfSBuf *reply);

} // OHOS::Camera

#endif // HDI_CAMERA_DUMP_H
