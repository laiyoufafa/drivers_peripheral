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

#include "adaptor_time_test.h"

#include "adaptor_time.h"

namespace OHOS {
namespace UserIam {
namespace UserAuth {
using namespace testing;
using namespace testing::ext;

void AdaptorTimeTest::SetUpTestCase()
{
}

void AdaptorTimeTest::TearDownTestCase()
{
}

void AdaptorTimeTest::SetUp()
{
}

void AdaptorTimeTest::TearDown()
{
}

/**
 * @tc.name: Get Time API test
 * @tc.desc: verify GetRtcTime and GetSystemTime
 * @tc.type: FUNC
 * @tc.require: #I64XCB
 */
HWTEST_F(AdaptorTimeTest, Get_Time_test, TestSize.Level0)
{
    uint64_t result = GetRtcTime();
    EXPECT_NE(result, 0);
    result = GetSystemTime();
    EXPECT_NE(result, 0);
}
} // namespace UserAuth
} // namespace UserIam
} // namespace OHOS
