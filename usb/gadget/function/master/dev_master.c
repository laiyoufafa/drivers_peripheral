/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "default_config.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_device_object.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "securec.h"
#include "usbfn_device.h"
#include "usbfn_interface.h"
#include "usbfn_request.h"

#define HDF_LOG_TAG dev_master

enum DevMasterCmd {
    DEV_MASTER_INIT = 0x1,
    DEV_MASTER_RELEASE,
};

struct DevMasterMgr {
    struct IDeviceIoService service;
    struct UsbFnDescriptorData descData;
    struct HdfDeviceObject *device;
    const char *udcName;
};

struct UsbFnDevice *g_fnDev = NULL;
static int32_t MasterReleaseFuncDevice()
{
    int32_t ret;
    if (g_fnDev == NULL) {
        HDF_LOGE("%{public}s: fnDev is null", __func__);
        return HDF_FAILURE;
    }
    ret = UsbFnRemoveDevice(g_fnDev);
    if (ret == HDF_SUCCESS) {
        g_fnDev = NULL;
    } else {
        HDF_LOGE("%{public}s: remove usb function device failed", __func__);
    }

    return ret;
}

static int32_t UsbFnRegistUsbfnDevice(struct HdfDeviceIoClient *client, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    struct HdfDeviceObject *device = client->device;
    struct DevMasterMgr *devMgr = NULL;
    struct UsbFnDevice *fnDev = NULL;
    uint8_t value;
    struct DeviceResourceIface *iface = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);

    if (iface == NULL || iface->GetUint32 == NULL || device == NULL) {
        HDF_LOGE("%{public}s: iface is invalid", __func__);
        return HDF_FAILURE;
    }
    devMgr = (struct DevMasterMgr *)device->service;
    if (HdfSbufReadUint8(data, &value) != true) {
        HDF_LOGE("%{public}s: read sbuf failed", __func__);
        return HDF_FAILURE;
    }
    HDF_LOGD("%{public}s: data=%{public}d, descriptor in %{public}s", __func__, value,
        (devMgr->descData.type == USBFN_DESC_DATA_TYPE_DESC ? "code" : "hcs"));
    devMgr->descData.functionMask = value;
    fnDev = (struct UsbFnDevice *)UsbFnCreateDevice(devMgr->udcName, &devMgr->descData);
    if (fnDev == NULL) {
        HDF_LOGE("%{public}s: create usb function device failed", __func__);
        if (!HdfSbufWriteInt8(reply, 0)) {
            HDF_LOGE("%{public}s: fn_master sbuf write error", __func__);
        }
        return HDF_FAILURE;
    }
    g_fnDev = fnDev;
    if (!HdfSbufWriteInt8(reply, value)) {
        HDF_LOGE("%{public}s: fn_master sbuf write error", __func__);
        return HDF_FAILURE;
    }
    HDF_LOGD("%{public}s: create device done, reply: %{public}d", __func__, value);
    return HDF_SUCCESS;
}

static int32_t MasterDispatch(
    struct HdfDeviceIoClient *client, int32_t cmdId, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    if (client == NULL) {
        HDF_LOGE("%{public}s: client is NULL", __func__);
        return HDF_FAILURE;
    }

    if (HdfDeviceObjectCheckInterfaceDesc(client->device, data) == false) {
        HDF_LOGE("%{public}s: check interface desc fail", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    switch (cmdId) {
        case DEV_MASTER_INIT:
            ret = UsbFnRegistUsbfnDevice(client, data, reply);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: create usbfn device failed", __func__);
            }
            break;
        case DEV_MASTER_RELEASE:
            ret = MasterReleaseFuncDevice();
            break;
        default:
            ret = HDF_ERR_INVALID_OBJECT;
            HDF_LOGE("%{public}s: unknown cmd id %{public}d", __func__, cmdId);
            break;
    }
    return ret;
}

/* HdfDriverEntry implementations */
static int32_t MasterDriverBind(struct HdfDeviceObject *device)
{
    struct DevMasterMgr *devMgr = NULL;
    if (device == NULL) {
        HDF_LOGE("%{public}s: device is null\n", __func__);
        return HDF_FAILURE;
    }
    devMgr = (struct DevMasterMgr *)OsalMemCalloc(sizeof(*devMgr));
    if (devMgr == NULL) {
        HDF_LOGE("%{public}s: usbfn Alloc usb devMgr failed\n", __func__);
        return HDF_FAILURE;
    }

    if (HdfDeviceObjectSetInterfaceDesc(device, "hdf.usb.usbfn") != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: Set Desc fail!\n", __func__);
        OsalMemFree(devMgr);
        return HDF_FAILURE;
    }

    devMgr->device = device;
    device->service = &(devMgr->service);
    devMgr->device->service->Dispatch = MasterDispatch;
    return HDF_SUCCESS;
}

static int32_t MasterDriverInit(struct HdfDeviceObject *device)
{
    struct DevMasterMgr *devMgr = NULL;
    uint8_t useHcs;

    struct DeviceResourceIface *iface = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (iface == NULL || iface->GetUint32 == NULL || device == NULL) {
        HDF_LOGE("%{public}s: iface is invalid\n", __func__);
        return HDF_FAILURE;
    }
    devMgr = (struct DevMasterMgr *)device->service;
    if (iface->GetString(device->property, "udc_name", &devMgr->udcName, UDC_NAME) != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: read udc_name failed, use default: %{public}s", __func__, UDC_NAME);
    }
    HDF_LOGI("%{public}s: udcName=%{public}s", __func__, devMgr->udcName);
    if (iface->GetUint8(device->property, "use_hcs", &useHcs, 0) != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: read use_hcs fail, use default", __func__);
    }
    HDF_LOGI("%{public}s: use descriptor in %{public}s", __func__, (useHcs == 1) ? "hcs" : "code");
    /* force use descripto in hcs, refer to sample for use descriptor in code */
    devMgr->descData.type = USBFN_DESC_DATA_TYPE_PROP;
    devMgr->descData.property = device->property;
    return HDF_SUCCESS;
}

static void MasterDriverRelease(struct HdfDeviceObject *device)
{
    struct DevMasterMgr *devMgr = NULL;
    devMgr = (struct DevMasterMgr *)device->service;
    if (devMgr == NULL) {
        HDF_LOGE("%{public}s: descriptor is NULL", __func__);
        return;
    }
    OsalMemFree(devMgr);
}

struct HdfDriverEntry g_masterDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "usbfn_master",
    .Bind = MasterDriverBind,
    .Init = MasterDriverInit,
    .Release = MasterDriverRelease,
};

HDF_INIT(g_masterDriverEntry);
