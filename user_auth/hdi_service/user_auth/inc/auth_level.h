/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef USERIAM_TRUST_LEVEL_H
#define USERIAM_TRUST_LEVEL_H

#include <stdint.h>

#include "defines.h"
#include "coauth.h"

#ifdef __cplusplus
extern "C" {
#endif

ResultCode SingleAuthTrustLevel(uint32_t userId, uint32_t authType, uint32_t *atl);
uint32_t GetAtl(uint32_t acl, uint32_t asl);
ResultCode QueryScheduleAtl(const CoAuthSchedule *coAuthSchedule, uint32_t acl, uint32_t *atl);

#ifdef __cplusplus
}
#endif

#endif // USERIAM_TRUST_LEVEL_H