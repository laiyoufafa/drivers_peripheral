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

#include <hdf_base.h>
#include <hdf_log.h>
#include <osal_mem.h>
#include <securec.h>
#include "v1_0/ihuks.h"

#include "hks_core_service.h"
#include "huks_hdi.h"
#include "ihuks_types.h"

#define HDF_LOG_TAG    huks_service

struct HuksService {
    struct IHuks interface;
};

static int32_t HuksModuleInit(struct IHuks *self)
{
    return HksCoreModuleInit();
}

static int32_t HuksGenerateKey(struct IHuks *self, const struct HuksBlob* keyAlias, const struct HuksParamSet* paramSet,
     const struct HuksBlob* keyIn, struct HuksBlob* encKeyOut)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_GENERATEKEY(keyAlias, paramSet, keyIn, encKeyOut, ret, HksCoreGenerateKey)
    return ret;
}

static int32_t HuksImportKey(struct IHuks *self, const struct HuksBlob* keyAlias, const struct HuksBlob* key,
     const struct HuksParamSet* paramSet, struct HuksBlob* encKeyOut)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_IMPORTKEY(keyAlias, key, paramSet, encKeyOut, ret, HksCoreImportKey)
    return ret;
}

static int32_t HuksImportWrappedKey(struct IHuks *self, const struct HuksBlob* wrappingKeyAlias,
     const struct HuksBlob* wrappingEncKey, const struct HuksBlob* wrappedKeyData, const struct HuksParamSet* paramSet, struct HuksBlob* encKeyOut)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_IMPORTWRAPPEDKEY(wrappingKeyAlias, wrappingEncKey, wrappedKeyData, paramSet, encKeyOut, ret,
        HksCoreImportWrappedKey)
    return ret;
}

static int32_t HuksExportPublicKey(struct IHuks *self, const struct HuksBlob* encKey,
     const struct HuksParamSet* paramSet, struct HuksBlob* keyOut)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_EXPORTPUBLICKEY(encKey, paramSet, keyOut, ret, HksCoreExportPublicKey)
    return ret;
}

static int32_t HuksInit(struct IHuks *self, const struct HuksBlob* encKey, const struct HuksParamSet* paramSet,
     struct HuksBlob* handle, struct HuksBlob* token)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_INIT(encKey, paramSet, handle, token, ret, HksCoreInit)
    return ret;
}

static int32_t HuksUpdate(struct IHuks *self, const struct HuksBlob* handle, const struct HuksParamSet* paramSet,
     const struct HuksBlob* inData, struct HuksBlob* outData)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_UPDATE(handle, paramSet, inData, outData, ret, HksCoreUpdate)
    return ret;
}

static int32_t HuksFinish(struct IHuks *self, const struct HuksBlob* handle, const struct HuksParamSet* paramSet,
     const struct HuksBlob* inData, struct HuksBlob* outData)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_FINISH(handle, paramSet, inData, outData, ret, HksCoreFinish)
    return ret;
}

static int32_t HuksAbort(struct IHuks *self, const struct HuksBlob* handle, const struct HuksParamSet* paramSet)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_ABORT(handle, paramSet, ret, HksCoreAbort)
    return ret;
}

static int32_t HuksCheckKeyValidity(struct IHuks *self, const struct HuksParamSet* paramSet,
     const struct HuksBlob* encKey)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_CHECKKEYVALIDITY(paramSet, encKey, ret, HksCoreGetKeyProperties)
    return ret;
}

static int32_t HuksAttestKey(struct IHuks *self, const struct HuksBlob* encKey, const struct HuksParamSet* paramSet,
     struct HuksBlob* certChain)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_ATTESTKEY(encKey, paramSet, certChain, ret, HksCoreAttestKey)
    return ret;
}

static int32_t HuksGenerateRandom(struct IHuks *self, const struct HuksParamSet* paramSet, struct HuksBlob* random)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_GENERATERANDOM(paramSet, random, ret, HksCoreGenerateRandom)
    return ret;
}

static int32_t HuksSign(struct IHuks *self, const struct HuksBlob* encKey, const struct HuksParamSet* paramSet,
     const struct HuksBlob* srcData, struct HuksBlob* signature)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_SIGN(encKey, paramSet, srcData, signature, ret, HksCoreSign)
    return ret;
}

static int32_t HuksVerify(struct IHuks *self, const struct HuksBlob* encKey, const struct HuksParamSet* paramSet,
     const struct HuksBlob* srcData, const struct HuksBlob* signature)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_VERIFY(encKey, paramSet, srcData, signature, ret, HksCoreVerify)
    return ret;
}

static int32_t HuksEncrypt(struct IHuks *self, const struct HuksBlob* encKey, const struct HuksParamSet* paramSet,
     const struct HuksBlob* plainText, struct HuksBlob* cipherText)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_ENCRYPT(encKey, paramSet, plainText, cipherText, ret, HksCoreEncrypt)
    return ret;
}

static int32_t HuksDecrypt(struct IHuks *self, const struct HuksBlob* encKey, const struct HuksParamSet* paramSet,
     const struct HuksBlob* cipherText, struct HuksBlob* plainText)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_DECRYPT(encKey, paramSet, cipherText, plainText, ret, HksCoreDecrypt)
    return ret;
}

static int32_t HuksAgreeKey(struct IHuks *self, const struct HuksParamSet* paramSet,
     const struct HuksBlob* encPrivateKey, const struct HuksBlob* peerPublicKey, struct HuksBlob* agreedKey)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_AGREEKEY(paramSet, encPrivateKey, peerPublicKey, agreedKey, ret, HksCoreAgreeKey)
    return ret;
}

static int32_t HuksDeriveKey(struct IHuks *self, const struct HuksParamSet* paramSet, const struct HuksBlob* encKdfKey,
     struct HuksBlob* derivedKey)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_DERIVEKEY(paramSet, encKdfKey, derivedKey, ret, HksCoreDeriveKey)
    return ret;
}

static int32_t HuksMac(struct IHuks *self, const struct HuksBlob* encKey, const struct HuksParamSet* paramSet,
     const struct HuksBlob* srcData, struct HuksBlob* mac)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_MAC(encKey, paramSet, srcData, mac, ret, HksCoreMac)
    return ret;
}

static int32_t HuksUpgradeKey(struct IHuks *self, const struct HuksBlob* encOldKey, const struct HuksParamSet* paramSet,
     struct HuksBlob* encNewKey)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_UPGRADEKEY(encOldKey, paramSet, encNewKey, ret, HksCoreUpgradeKey)
    return ret;
}

static int32_t HuksExportChipsetPlatformPublicKey(struct IHuks *self, const struct HuksBlob* salt,
     enum HuksChipsetPlatformDecryptScene scene, struct HuksBlob* publicKey)
{
    int32_t ret = HDF_FAILURE;
    HDI_CONVERTER_FUNC_EXPORTCHIPSETPLATFORMPUBLICKEY(salt, scene, publicKey, ret,
        HksCoreExportChipsetPlatformPublicKey)
    return ret;
}

static int32_t HuksGetVersion(struct IHuks *self, uint32_t* majorVer, uint32_t* minorVer)
{
    *majorVer = IHUKS_MAJOR_VERSION;
    *minorVer = IHUKS_MINOR_VERSION;
    return HDF_SUCCESS;
}

struct IHuks *HuksImplGetInstance(void)
{
    struct HuksService *service = (struct HuksService *)OsalMemCalloc(sizeof(struct HuksService));
    if (service == NULL) {
        HDF_LOGE("%{public}s: malloc HuksService obj failed!", __func__);
        return NULL;
    }

    service->interface.ModuleInit = HuksModuleInit;
    service->interface.GenerateKey = HuksGenerateKey;
    service->interface.ImportKey = HuksImportKey;
    service->interface.ImportWrappedKey = HuksImportWrappedKey;
    service->interface.ExportPublicKey = HuksExportPublicKey;
    service->interface.Init = HuksInit;
    service->interface.Update = HuksUpdate;
    service->interface.Finish = HuksFinish;
    service->interface.Abort = HuksAbort;
    service->interface.CheckKeyValidity = HuksCheckKeyValidity;
    service->interface.AttestKey = HuksAttestKey;
    service->interface.GenerateRandom = HuksGenerateRandom;
    service->interface.Sign = HuksSign;
    service->interface.Verify = HuksVerify;
    service->interface.Encrypt = HuksEncrypt;
    service->interface.Decrypt = HuksDecrypt;
    service->interface.AgreeKey = HuksAgreeKey;
    service->interface.DeriveKey = HuksDeriveKey;
    service->interface.Mac = HuksMac;
    service->interface.UpgradeKey = HuksUpgradeKey;
    service->interface.ExportChipsetPlatformPublicKey = HuksExportChipsetPlatformPublicKey;
    service->interface.GetVersion = HuksGetVersion;
    return &service->interface;
}

void HuksImplRelease(struct IHuks *instance)
{
    if (instance == NULL) {
        return;
    }
    OsalMemFree(instance);
}
