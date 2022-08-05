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

#include "thermal_dfx.h"
#include <cerrno>
#include <cstdio>
#include <deque>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <hdf_log.h>
#include <hdf_base.h>

#include "directory_ex.h"
#include "parameters.h"
#include "securec.h"
#include "thermal_log.h"
#include "thermal_zone_manager.h"
#include "zlib.h"

namespace OHOS {
namespace HDI {
namespace Thermal {
namespace V1_0 {
namespace {
constexpr uint8_t LOG_INDEX_LEN = 4;
constexpr int32_t MSEC_TO_SEC = 1000;
constexpr int32_t MAX_FILE_NUM = 10;
constexpr int32_t MAX_FILE_SIZE = 10 * 1024 *1024;
constexpr int32_t DEFAULT_INTERVAL_MS = 5000;
constexpr int32_t MAX_TIME_LEN = 20;
constexpr int32_t MAX_BUFF_SIZE = 128;
constexpr int32_t TIME_FORMAT_1 = 1;
constexpr int32_t TIME_FORMAT_2 = 2;
constexpr int32_t COMPRESS_READ_BUF_SIZE = 4096;
const std::string TIMESTAMP_TITLE = "timestamp";
uint32_t g_currentLogIndex = 0;
int32_t g_timerInterval = -1;
bool g_firstCreate = true;
std::deque<std::string> g_saveLogFile;
std::string g_logTime = "";
XMLTracingInfo g_xmlTraceInfo;
}

static std::string GetCurrentTime(const int32_t format)
{
    struct tm* pTime;
    char strTime[MAX_TIME_LEN] = {0};
    time_t t;
    if (time(&t) == -1) {
        THERMAL_HILOGW(COMP_HDI, "call time failed");
        return "";
    }

    pTime = localtime(&t);
    if (format == TIME_FORMAT_1) {
        if (strftime(strTime, sizeof(strTime), "%Y%m%d-%H%M%S", pTime) == 0U) {
            THERMAL_HILOGW(COMP_HDI, "call strfime failed");
            return "";
        }
    } else if (format == TIME_FORMAT_2) {
        if (strftime(strTime, sizeof(strTime), "%Y-%m-%d %H:%M:%S", pTime) == 0U) {
            THERMAL_HILOGW(COMP_HDI, "call strfime failed");
            return "";
        }
    } else {
        THERMAL_HILOGW(COMP_HDI, "invalid format value");
        return "";
    }
    return strTime;
}

std::string ThermalDfx::GetFileNameIndex(const uint32_t index)
{
    char res[LOG_INDEX_LEN];
    (void)snprintf_s(res, sizeof(res), sizeof(res) - 1, "%03d", index % MAX_FILE_NUM);
    std::string fileNameIndex(res);
    return fileNameIndex;
}

void ThermalDfx::UpdateInterval()
{
    std::string paramInterval = OHOS::system::GetParameter("persist.thermal.log.interval", "5000");
    int32_t interval = std::stoi(paramInterval.c_str());
    THERMAL_HILOGD(COMP_HDI, "interval = %{public}d", interval);
    if (interval == 0) {
        interval = std::stoi(g_xmlTraceInfo.interval.c_str());
    }

    if (interval == 0) {
        interval = DEFAULT_INTERVAL_MS;
    }

    g_timerInterval = interval;
}

std::string ThermalDfx::CanonicalizeSpecPath(const char* src)
{
    if (src == nullptr || strlen(src) >= PATH_MAX) {
        fprintf(stderr, "Error: CanonicalizeSpecPath %s failed", src);
        return "";
    }
    char resolvedPath[PATH_MAX] = { 0 };
    if (access(src, F_OK) == 0) {
        if (realpath(src, resolvedPath) == nullptr) {
            fprintf(stderr, "Error: realpath %s failed", src);
            return "";
        }
    } else {
        std::string fileName(src);
        if (fileName.find("..") == std::string::npos) {
            if (snprintf_s(resolvedPath, PATH_MAX, sizeof(resolvedPath) - 1, src) == -1) {
                fprintf(stderr, "Error: sprintf_s %s failed", src);
                return "";
            }
        } else {
            fprintf(stderr, "Error: find .. %s failed", src);
            return "";
        }
    }

    std::string res(resolvedPath);
    return res;
}

bool ThermalDfx::Compress(const std::string& dataFile, const std::string& destFile)
{
    std::string resolvedPath = CanonicalizeSpecPath(dataFile.c_str());
    FILE* fp = fopen(resolvedPath.c_str(), "rb");
    if (fp == nullptr) {
        THERMAL_HILOGE(COMP_HDI, "Fail to open data file %{public}s", dataFile.c_str());
        perror("Fail to fopen(rb)");
        return false;
    }

    std::unique_ptr<gzFile_s, decltype(&gzclose)> fgz(gzopen(destFile.c_str(), "wb"), gzclose);
    if (fgz == nullptr) {
        THERMAL_HILOGE(COMP_HDI, "Fail to call gzopen(%{public}s)", destFile.c_str());
        fclose(fp);
        return false;
    }

    std::vector<char> buf(COMPRESS_READ_BUF_SIZE);
    size_t len = 0;
    while ((len = fread(buf.data(), sizeof(uint8_t), buf.size(), fp))) {
        if (gzwrite(fgz.get(), buf.data(), len) == 0) {
            THERMAL_HILOGE(COMP_HDI, "Fail to call gzwrite for %{public}zu bytes", len);
            fclose(fp);
            return false;
        }
    }
    if (!feof(fp)) {
        if (ferror(fp) != 0) {
            THERMAL_HILOGE(COMP_HDI, "ferror return err");
            fclose(fp);
            return false;
        }
    }
    if (fclose(fp) < 0) {
        return false;
    }
    return true;
}

void ThermalDfx::CompressFile()
{
    unsigned long size;
    std::string unCompressFile = g_xmlTraceInfo.outpath + "/" + "thermal." + GetFileNameIndex(g_currentLogIndex) +
        "." + g_logTime;

    FILE* fp = fopen(unCompressFile.c_str(), "rb");
    if (fp == nullptr) {
        THERMAL_HILOGE(COMP_HDI, "open uncompressfile failed");
        return;
    }

    if (fseek(fp, SEEK_SET, SEEK_END) != 0) {
        THERMAL_HILOGE(COMP_HDI, "fseek() failed");
        return;
    }

    size = ftell(fp);
    if (size < MAX_FILE_SIZE) {
        if (fclose(fp) < 0) {
            THERMAL_HILOGW(COMP_HDI, "fclose() failed");
        }
        THERMAL_HILOGD(COMP_HDI, "file is not enough for compress");
        return;
    }
    if (fclose(fp) < 0) {
        THERMAL_HILOGW(COMP_HDI, "fclose() failed");
    }

    std::string compressFile = g_xmlTraceInfo.outpath + "/" + "thermal." + GetFileNameIndex(g_currentLogIndex) +
        "." + g_logTime + ".gz";
    if (!Compress(unCompressFile, compressFile)) {
        THERMAL_HILOGE(COMP_HDI, "CompressFile fail");
        return;
    }

    if (remove(unCompressFile.c_str()) != 0) {
        THERMAL_HILOGW(COMP_HDI, "failed to remove file %{public}s", unCompressFile.c_str());
    }

    if (g_saveLogFile.size() >= MAX_FILE_NUM) {
        if (remove(g_saveLogFile.front().c_str()) != 0) {
            THERMAL_HILOGW(COMP_HDI, "failed to remove file %{public}s", compressFile.c_str());
        }
        g_saveLogFile.pop_front();
    }
    g_saveLogFile.push_back(compressFile);
    g_currentLogIndex++;
    g_logTime = GetCurrentTime(TIME_FORMAT_1);
}

int32_t ThermalDfx::ParseValue(const std::string& path, std::string& value)
{
    char bufType[MAX_BUFF_SIZE] = {0};
    std::unique_ptr<ThermalZoneManager> thermalZoneMgr = std::make_unique<ThermalZoneManager>();
    if (thermalZoneMgr == nullptr) {
        THERMAL_HILOGE(COMP_HDI, "thermalZoneMgr is nullptr");
        return -1;
    }

    int32_t ret = thermalZoneMgr->ReadThermalSysfsToBuff(path.c_str(), bufType, sizeof(bufType));
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    value = bufType;
    return HDF_SUCCESS;
}

bool ThermalDfx::PrepareWriteDfxLog()
{
    if (g_xmlTraceInfo.outpath == "") {
        THERMAL_HILOGW(COMP_HDI, "parse thermal_hdi_config.xml outpath fail");
        return false;
    }

    std::string paramEnable = OHOS::system::GetParameter("persist.thermal.log.enable", "true");
    if (paramEnable == "false") {
        THERMAL_HILOGD(COMP_HDI, "param does not start recording");
        return false;
    }

    return true;
}

void ThermalDfx::CreateLogFile()
{
    if (!PrepareWriteDfxLog()) {
        THERMAL_HILOGD(COMP_HDI, "prepare write dfx log failed");
        return;
    }
    if (g_firstCreate) {
        g_currentLogIndex = 0;
        g_logTime = GetCurrentTime(TIME_FORMAT_1);
        g_firstCreate = false;
    }
    std::string logFile = g_xmlTraceInfo.outpath + "/" + "thermal." + GetFileNameIndex(g_currentLogIndex) +
        "." + g_logTime;
    if (access(g_xmlTraceInfo.outpath.c_str(), 0) == -1) {
        auto ret = ForceCreateDirectory(g_xmlTraceInfo.outpath.c_str());
        if (!ret) {
            THERMAL_HILOGE(COMP_HDI, "create output dir failed");
            return;
        }
    }

    bool isEmpty = false;
    std::ifstream fin(logFile);
    std::fstream file;
    file.open(logFile, std::ios::in);
    if (file.eof() || !fin) {
        isEmpty = true;
    }
    file.close();

    ProcessLogInfo(logFile, isEmpty);

    return;
}

void ThermalDfx::ProcessLogInfo(std::string& logFile, bool isEmpty)
{
    std::string width = OHOS::system::GetParameter("persist.thermal.log.width", "20");
    std::string interval = OHOS::system::GetParameter("persist.thermal.log.interval", "5000");
    uint32_t paramWidth = std::stoi(width.c_str());

    std::string currentTime = GetCurrentTime(TIME_FORMAT_2);
    std::ofstream wStream(logFile, std::ios::app);
    if (wStream.is_open()) {
        if (isEmpty) {
            wStream << TIMESTAMP_TITLE;
            for (uint32_t i = 0; i < paramWidth; ++i) {
                wStream << " ";
            }

            std::vector<DfxTraceInfo> logInfo = ThermalHdfConfig::GetInsance().GetTracingInfo();
            for (auto info : logInfo) {
                if (info.title.find("/")) {
                    std::string titlePath = info.title;
                    ParseValue(titlePath, info.title);
                }
                wStream << info.title;
                if (info.value == logInfo.back().value &&
                    info.title == logInfo.back().title) {
                    break;
                }
                for (uint32_t i = 0; i < paramWidth; ++i) {
                    wStream << " ";
                }
            }
            wStream << "\n";

            WriteToFile(wStream, currentTime, paramWidth);
            wStream.close();
            return;
        }

        WriteToFile(wStream, currentTime, paramWidth);
        wStream.close();
    }
}

void ThermalDfx::WriteToFile(std::ofstream& wStream, std::string& currentTime, uint32_t paramWidth)
{
    wStream << currentTime;
    for (uint32_t i = 0; i < paramWidth + TIMESTAMP_TITLE.length() - currentTime.length(); ++i) {
        wStream << " ";
    }
    std::vector<DfxTraceInfo> logInfo = ThermalHdfConfig::GetInsance().GetTracingInfo();
    for (auto info : logInfo) {
        std::string valuePath = info.value;
        ParseValue(valuePath, info.value);
        wStream << info.value;
        if (info.value == logInfo.back().value &&
            info.title == logInfo.back().title) {
            break;
        }

        if (info.title.find("/")) {
            std::string titlePath = info.title;
            ParseValue(titlePath, info.title);
        }
        for (uint32_t i = 0; i < paramWidth + info.title.length() - info.value.length(); ++i) {
            wStream << " ";
        }
    }
    wStream << "\n";
}

void ThermalDfx::GetTraceInfo()
{
    XMLTracingInfo info = ThermalHdfConfig::GetInsance().GetXmlTraceInfo();
    THERMAL_HILOGD(COMP_HDI, "info.interval = %{public}s, info.outpath = %{private}s",
        info.interval.c_str(), info.outpath.c_str());

    g_xmlTraceInfo.interval = info.interval;
    g_xmlTraceInfo.outpath = info.outpath;

    std::vector<DfxTraceInfo> logInfo = ThermalHdfConfig::GetInsance().GetTracingInfo();
    for (auto info : logInfo) {
        THERMAL_HILOGD(COMP_HDI, "info.title = %{public}s, info.value = %{public}s",
            info.title.c_str(), info.value.c_str());
    }

    CreateLogFile();
}

int32_t ThermalDfx::LoopingThreadEntry()
{
    while (true) {
        UpdateInterval();
        int32_t timeout = g_timerInterval;
        THERMAL_HILOGD(COMP_HDI, "timeout = %{public}d", timeout);
        std::this_thread::sleep_for(std::chrono::seconds(timeout / MSEC_TO_SEC));
        GetTraceInfo();
        CompressFile();
    }
}

void ThermalDfx::StartThread()
{
    std::make_unique<std::thread>(&ThermalDfx::LoopingThreadEntry, this)->detach();
}

int32_t ThermalDfx::Init()
{
    StartThread();
    return HDF_SUCCESS;
}
} // V1_0
} // Thermal
} // HDI
} // OHOS