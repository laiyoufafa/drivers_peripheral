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

#include <hdf_base.h>
#include <hdf_device_desc.h>
#include <hdf_sbuf_ipc.h>
#include "v1_0/pin_auth_interface_stub.h"
#include "pin_auth.h"
#include "iam_logger.h"
#include "iam_ptr.h"

#define LOG_LABEL OHOS::UserIAM::Common::LABEL_PIN_AUTH_HDI

using namespace OHOS::HDI::PinAuth::V1_0;

struct HdfPinAuthInterfaceHost {
    struct IDeviceIoService ioService;
    OHOS::sptr<OHOS::IRemoteObject> stub;
};

static int32_t PinAuthInterfaceDriverDispatch(struct HdfDeviceIoClient *client, int cmdId, struct HdfSBuf *data,
    struct HdfSBuf *reply)
{
    IAM_LOGI("start");
    auto *hdfPinAuthInterfaceHost = CONTAINER_OF(client->device->service,
        struct HdfPinAuthInterfaceHost, ioService);

    OHOS::MessageParcel *dataParcel = nullptr;
    OHOS::MessageParcel *replyParcel = nullptr;
    OHOS::MessageOption option;

    if (SbufToParcel(data, &dataParcel) != HDF_SUCCESS) {
        IAM_LOGE("%{public}s:invalid data sbuf object to dispatch", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (SbufToParcel(reply, &replyParcel) != HDF_SUCCESS) {
        IAM_LOGE("%{public}s:invalid reply sbuf object to dispatch", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    return hdfPinAuthInterfaceHost->stub->SendRequest(cmdId, *dataParcel, *replyParcel, option);
}

static int HdfPinAuthInterfaceDriverInit(struct HdfDeviceObject *deviceObject)
{
    IAM_LOGI("start");
    std::shared_ptr<OHOS::UserIAM::PinAuth::PinAuth> pinHdi =
        OHOS::UserIAM::Common::MakeShared<OHOS::UserIAM::PinAuth::PinAuth>();
    constexpr uint32_t SUCCESS = 0;
    if (pinHdi == nullptr || pinHdi->Init() != SUCCESS) {
        IAM_LOGE("Pin hal init failed");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int HdfPinAuthInterfaceDriverBind(struct HdfDeviceObject *deviceObject)
{
    IAM_LOGI("start");
    auto *hdfPinAuthInterfaceHost = new (std::nothrow) HdfPinAuthInterfaceHost;
    if (hdfPinAuthInterfaceHost == nullptr) {
        IAM_LOGE("%{public}s: failed to create create HdfPinAuthInterfaceHost object", __func__);
        return HDF_FAILURE;
    }

    hdfPinAuthInterfaceHost->ioService.Dispatch = PinAuthInterfaceDriverDispatch;
    hdfPinAuthInterfaceHost->ioService.Open = NULL;
    hdfPinAuthInterfaceHost->ioService.Release = NULL;

    auto serviceImpl = IPinAuthInterface::Get(true);
    if (serviceImpl == nullptr) {
        IAM_LOGE("%{public}s: failed to get of implement service", __func__);
        return HDF_FAILURE;
    }

    hdfPinAuthInterfaceHost->stub = OHOS::HDI::ObjectCollector::GetInstance().GetOrNewObject(serviceImpl,
        IPinAuthInterface::GetDescriptor());
    if (hdfPinAuthInterfaceHost->stub == nullptr) {
        IAM_LOGE("%{public}s: failed to get stub object", __func__);
        return HDF_FAILURE;
    }

    deviceObject->service = &hdfPinAuthInterfaceHost->ioService;
    IAM_LOGI("success");
    return HDF_SUCCESS;
}

static void HdfPinAuthInterfaceDriverRelease(struct HdfDeviceObject *deviceObject)
{
    IAM_LOGI("start");
    auto *hdfPinAuthInterfaceHost = CONTAINER_OF(deviceObject->service,
        struct HdfPinAuthInterfaceHost, ioService);
    delete hdfPinAuthInterfaceHost;
    IAM_LOGI("success");
}

static struct HdfDriverEntry g_pinAuthInterfaceDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "pinauth_interface_service",
    .Bind = HdfPinAuthInterfaceDriverBind,
    .Init = HdfPinAuthInterfaceDriverInit,
    .Release = HdfPinAuthInterfaceDriverRelease,
};

#ifndef __cplusplus
extern "C" {
#endif
HDF_INIT(g_pinAuthInterfaceDriverEntry);
#ifndef __cplusplus
}
#endif