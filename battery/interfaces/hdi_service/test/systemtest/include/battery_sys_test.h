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

#ifndef BATTERY_SYS_TEST_H
#define BATTERY_SYS_TEST_H

#include <gtest/gtest.h>

namespace OHOS {
namespace HDI {
namespace Battery {
namespace V1_2 {
class BatterySysTest : public testing::Test {
public:
    static constexpr uint32_t RED_LIGHT = (255 << 16);
    static constexpr uint32_t GREEN_LIGHT = (255 << 8);
    static constexpr uint32_t YELLOW_LIGHT = (255 << 16) | (255 << 8);
    static constexpr uint32_t LIGHT_OFF = 0;
};
} // namespace V1_2
} // namespace Battery
} // namespace HDI
} // namespace OHOS
#endif // Battery_SYS_TEST_H