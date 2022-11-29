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

#include "thermal_interface_impl.h"

#include <thread>
#include <memory>
#include <hdf_base.h>
#include "thermal_hdf_config.h"
#include "thermal_hdf_timer.h"
#include "thermal_simulation_node.h"
#include "thermal_device_mitigation.h"
#include "thermal_zone_manager.h"
#include "thermal_log.h"

namespace OHOS {
namespace HDI {
namespace Thermal {
namespace V1_0 {
namespace {
const std::string HDI_XML_NAME = HDF_ETC_DIR "/thermal_config/hdf/thermal_hdi_config.xml";
const std::string HDI_XML_CUST_NAME = HDF_ETC_DIR "/thermal_config/hdf/cust/thermal_hdi_config.xml";
bool g_isHdiStart = false;
}
static sptr<IThermalCallback> theramalCb_ = nullptr;
static std::shared_ptr<HdfThermalCallbackInfo> callbackInfo_ = nullptr;
static std::shared_ptr<ThermalHdfTimer> hdfTimer_ = nullptr;
static std::shared_ptr<ThermalSimulationNode> simulation_ = nullptr;
static std::shared_ptr<ThermalDeviceMitigation> mitigation_ = nullptr;
static std::shared_ptr<ThermalZoneManager> thermalZoneMgr_ = nullptr;

extern "C" IThermalInterface *ThermalInterfaceImplGetInstance(void)
{
    return new (std::nothrow) ThermalInterfaceImpl();
}

ThermalInterfaceImpl::ThermalInterfaceImpl()
{
    Init();
}

int32_t ThermalInterfaceImpl::Init()
{
    int32_t ret = ThermalHdfConfig::GetInsance().ThermalHDIConfigInit(HDI_XML_CUST_NAME);
    if (ret != HDF_SUCCESS) {
        THERMAL_HILOGI(COMP_HDI, "init thermal hdi common XML");
        ret = ThermalHdfConfig::GetInsance().ThermalHDIConfigInit(HDI_XML_NAME);
        if (ret != HDF_SUCCESS) {
            THERMAL_HILOGE(COMP_HDI, "failed to init XML, ret: %{public}d", ret);
            return HDF_FAILURE;
        }
    }

    if (simulation_ == nullptr) {
        simulation_ = std::make_shared<ThermalSimulationNode>();
    }

    if (thermalZoneMgr_ == nullptr) {
        thermalZoneMgr_ = std::make_shared<ThermalZoneManager>();
    }

    if (mitigation_ == nullptr) {
        mitigation_ = std::make_shared<ThermalDeviceMitigation>();
    }

    if (hdfTimer_ == nullptr) {
        hdfTimer_ = std::make_shared<ThermalHdfTimer>(simulation_, thermalZoneMgr_);
        hdfTimer_->SetSimluationFlag();
    }

    if (hdfTimer_->GetSimluationFlag()) {
        ret = simulation_->NodeInit();
        if (ret != HDF_SUCCESS) {
            return HDF_FAILURE;
        }
    }

    thermalZoneMgr_->CalculateMaxCd();
    thermalZoneMgr_->SetMultiples();
    ret = thermalZoneMgr_->ParseThermalZoneInfo();
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    hdfTimer_->DumpSensorConfigInfo();
    mitigation_->SetFlag(static_cast<bool>(hdfTimer_->GetSimluationFlag()));
    return HDF_SUCCESS;
}

int32_t ThermalInterfaceImpl::SetCpuFreq(int32_t freq)
{
    if (mitigation_ != nullptr) {
        int32_t ret = mitigation_->CpuRequest(freq);
        if (ret != HDF_SUCCESS) {
            THERMAL_HILOGE(COMP_HDI, "failed to set freq %{public}d", ret);
            return ret;
        }
    }
    return HDF_SUCCESS;
}

int32_t ThermalInterfaceImpl::SetGpuFreq(int32_t freq)
{
    if (mitigation_ != nullptr) {
        int32_t ret = mitigation_->GpuRequest(freq);
        if (ret != HDF_SUCCESS) {
            THERMAL_HILOGE(COMP_HDI, "failed to set freq %{public}d", ret);
            return ret;
        }
    }
    return HDF_SUCCESS;
}

int32_t ThermalInterfaceImpl::SetBatteryCurrent(int32_t current)
{
    if (mitigation_ != nullptr) {
        int32_t ret = mitigation_->ChargerRequest(current);
        if (ret != HDF_SUCCESS) {
            THERMAL_HILOGE(COMP_HDI, "failed to set current %{public}d", ret);
            return ret;
        }
    }
    return HDF_SUCCESS;
}

int32_t ThermalInterfaceImpl::GetThermalZoneInfo(HdfThermalCallbackInfo& event)
{
    if (thermalZoneMgr_ != nullptr) {
        thermalZoneMgr_->ParseThermalZoneInfo();
        event.info = thermalZoneMgr_->tzInfoAcaualEvent_.info;
    }
    return HDF_SUCCESS;
}

int32_t ThermalInterfaceImpl::Register(const sptr<IThermalCallback>& callbackObj)
{
    int32_t ret;
    theramalCb_ = callbackObj;
    if (hdfTimer_ == nullptr) return HDF_FAILURE;
    hdfTimer_->SetThermalEventCb(theramalCb_);

    if (!g_isHdiStart) {
        ret = hdfTimer_->Init();
        if (ret != HDF_SUCCESS) {
            return ret;
        }
        g_isHdiStart = true;
    }
    return HDF_SUCCESS;
}

int32_t ThermalInterfaceImpl::Unregister()
{
    theramalCb_ = nullptr;
    return HDF_SUCCESS;
}
} // V1_0
} // Thermal
} // HDI
} // OHOS
