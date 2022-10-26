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

#include "usbd_function.h"

#include "devmgr_hdi.h"
#include "hdf_log.h"
#include "hdf_remote_service.h"
#include "hdf_sbuf.h"
#include "iservmgr_hdi.h"
#include "message_option.h"
#include "message_parcel.h"
#include "osal_time.h"
#include "parameter.h"
#include "securec.h"
#include "string_ex.h"
#include "usbd_type.h"

namespace OHOS {
namespace HDI {
namespace Usb {
namespace V1_0 {
uint32_t UsbdFunction::currentFuncs_ = USB_FUNCTION_HDC;

using OHOS::HDI::ServiceManager::V1_0::IServiceManager;

int32_t UsbdFunction::SendCmdToService(const char *name, int32_t cmd, unsigned char funcMask)
{
    auto servMgr = IServiceManager::Get();
    if (servMgr == nullptr) {
        HDF_LOGE("%{public}s: get IServiceManager failed", __func__);
        return HDF_FAILURE;
    }

    sptr<IRemoteObject> remote = servMgr->GetService(name);
    if (remote == nullptr) {
        HDF_LOGE("%{public}s: get remote object failed: %{public}s", __func__, name);
        return HDF_FAILURE;
    }

    OHOS::MessageParcel data;
    OHOS::MessageParcel reply;
    OHOS::MessageOption option;

    if (!data.WriteInterfaceToken(Str8ToStr16(HDF_USB_USBFN_DESC))) {
        HDF_LOGE("%{public}s: WriteInterfaceToken failed", __func__);
        return HDF_FAILURE;
    }

    if (!data.WriteUint8(funcMask)) {
        HDF_LOGE("%{public}s: WriteInt8 failed: %{public}d", __func__, funcMask);
        return HDF_FAILURE;
    }

    int32_t ret = remote->SendRequest(cmd, data, reply, option);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: send request to %{public}s failed, ret=%{public}d", __func__, name, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

int32_t UsbdFunction::RemoveHdc()
{
    int32_t status = SetParameter(SYS_USB_CONFIG, HDC_CONFIG_OFF);
    if (status != 0) {
        HDF_LOGE("%{public}s:remove hdc config error = %{public}d", __func__, status);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbdFunction::AddHdc()
{
    int32_t status = SetParameter(SYS_USB_CONFIG, HDC_CONFIG_ON);
    if (status != 0) {
        HDF_LOGE("%{public}s:add hdc config error = %{public}d", __func__, status);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbdFunction::SetFunctionToRndis()
{
    int32_t status = SetParameter(SYS_USB_CONFIG, HDC_CONFIG_RNDIS);
    if (status != 0) {
        HDF_LOGE("%{public}s:add rndis config error = %{public}d", __func__, status);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbdFunction::SetFunctionToStorage()
{
    int32_t status = SetParameter(SYS_USB_CONFIG, HDC_CONFIG_STORAGE);
    if (status != 0) {
        HDF_LOGE("%{public}s:add storage config error = %{public}d", __func__, status);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbdFunction::SetFunctionToRndisHdc()
{
    int32_t status = SetParameter(SYS_USB_CONFIG, HDC_CONFIG_RNDIS_HDC);
    if (status != 0) {
        HDF_LOGE("%{public}s:add rndis hdc config error = %{public}d", __func__, status);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbdFunction::SetFunctionToStorageHdc()
{
    int32_t status = SetParameter(SYS_USB_CONFIG, HDC_CONFIG_STORAGE_HDC);
    if (status != 0) {
        HDF_LOGE("%{public}s:add storage hdc config error = %{public}d", __func__, status);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbdFunction::SetFunctionToNone()
{
    UsbdFunction::SendCmdToService(ACM_SERVICE_NAME, ACM_RELEASE, USB_FUNCTION_ACM);
    UsbdFunction::SendCmdToService(ECM_SERVICE_NAME, ECM_RELEASE, USB_FUNCTION_ECM);
    UsbdFunction::SendCmdToService(MTP_SERVICE_NAME, MTP_RELEASE, USB_FUNCTION_MTP);
    UsbdFunction::SendCmdToService(PTP_SERVICE_NAME, PTP_RELEASE, USB_FUNCTION_PTP);
    UsbdFunction::SendCmdToService(DEV_SERVICE_NAME, FUNCTION_DEL, USB_DDK_FUNCTION_SUPPORT);
    int32_t ret = RemoveHdc();
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: RemoveHdc error, ret = %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    currentFuncs_ = USB_FUNCTION_NONE;
    return HDF_SUCCESS;
}

int32_t UsbdFunction::SetDDKFunction(uint32_t funcs)
{
    HDF_LOGD("%{public}s: SetDDKFunction funcs=%{public}d", __func__, funcs);
    uint32_t ddkFuns = static_cast<uint32_t>(funcs) & USB_DDK_FUNCTION_SUPPORT;
    if (ddkFuns == 0) {
        HDF_LOGE("%{public}s: not use ddkfunction", __func__);
        return HDF_SUCCESS;
    }
    if (UsbdFunction::SendCmdToService(DEV_SERVICE_NAME, FUNCTION_ADD, ddkFuns)) {
        HDF_LOGE("%{public}s: create dev error: %{public}d", __func__, ddkFuns);
        return HDF_FAILURE;
    }
    if ((ddkFuns & USB_FUNCTION_ACM) && UsbdFunction::SendCmdToService(ACM_SERVICE_NAME, ACM_INIT, USB_FUNCTION_ACM)) {
        HDF_LOGE("%{public}s: acm init error", __func__);
        return HDF_FAILURE;
    }
    if ((ddkFuns & USB_FUNCTION_ECM) && UsbdFunction::SendCmdToService(ECM_SERVICE_NAME, ECM_INIT, USB_FUNCTION_ECM)) {
        HDF_LOGE("%{public}s: ecm init error", __func__);
        return HDF_FAILURE;
    }
    if ((ddkFuns & USB_FUNCTION_MTP) && UsbdFunction::SendCmdToService(MTP_SERVICE_NAME, MTP_INIT, USB_FUNCTION_MTP)) {
        HDF_LOGE("%{public}s: mtp init error", __func__);
        return HDF_FAILURE;
    }
    if ((ddkFuns & USB_FUNCTION_PTP) && UsbdFunction::SendCmdToService(PTP_SERVICE_NAME, PTP_INIT, USB_FUNCTION_PTP)) {
        HDF_LOGE("%{public}s: ptp init error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbdFunction::UsbdSetFunction(uint32_t funcs)
{
    HDF_LOGI("%{public}s: UsbdSetFunction funcs=%{public}d", __func__, funcs);
    if ((funcs | USB_FUNCTION_SUPPORT) != USB_FUNCTION_SUPPORT) {
        HDF_LOGE("%{public}s: funcs invalid", __func__);
        return HDF_FAILURE;
    }

    uint32_t kfuns = static_cast<uint32_t>(funcs) & (~USB_DDK_FUNCTION_SUPPORT);
    if (UsbdFunction::SetFunctionToNone()) {
        HDF_LOGW("%{public}s: setFunctionToNone error", __func__);
    }

    if (UsbdFunction::SetDDKFunction(funcs)) {
        HDF_LOGE("%{public}s:SetDDKFunction error", __func__);
        return HDF_FAILURE;
    }

    switch (kfuns) {
        case USB_FUNCTION_HDC:
            if (UsbdFunction::AddHdc()) {
                HDF_LOGE("%{public}s: AddHdc error", __func__);
                return HDF_FAILURE;
            }
            break;
        case USB_FUNCTION_RNDIS:
            if (UsbdFunction::SetFunctionToRndis()) {
                HDF_LOGE("%{public}s: SetFunctionToRndis error", __func__);
                return HDF_FAILURE;
            }
            break;
        case USB_FUNCTION_STORAGE:
            if (UsbdFunction::SetFunctionToStorage()) {
                HDF_LOGE("%{public}s: SetFunctionToStorage error", __func__);
                return HDF_FAILURE;
            }
            break;
        case USB_FUNCTION_RNDIS | USB_FUNCTION_HDC:
            if (UsbdFunction::SetFunctionToRndisHdc()) {
                HDF_LOGE("%{public}s: SetFunctionToRndisHdc error", __func__);
                return HDF_FAILURE;
            }
            break;
        case USB_FUNCTION_STORAGE | USB_FUNCTION_HDC:
            if (UsbdFunction::SetFunctionToStorageHdc()) {
                HDF_LOGE("%{public}s: SetFunctionToStorageHdc error", __func__);
                return HDF_FAILURE;
            }
            break;
        default:
            break;
    }
    currentFuncs_ = funcs;
    return HDF_SUCCESS;
}

int32_t UsbdFunction::UsbdGetFunction(void)
{
    return currentFuncs_;
}
} // namespace V1_0
} // namespace Usb
} // namespace HDI
} // namespace OHOS
