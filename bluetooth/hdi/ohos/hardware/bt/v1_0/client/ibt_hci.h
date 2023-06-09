/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_HARDWARE_BT_V1_0_IBTHCI_CLIENT_H
#define OHOS_HARDWARE_BT_V1_0_IBTHCI_CLIENT_H


#include <cstdint>
#include <string>
#include <hdf_log.h>
#include <iservmgr_hdi.h>
#include "ohos/hardware/bt/v1_0/types.h"
#include "ohos/hardware/bt/v1_0/ibt_hci_callbacks.h"

namespace ohos {
namespace hardware {
namespace bt {
namespace v1_0 {
using namespace OHOS;

enum class HciCmd {
    CMD_INIT,
    CMD_SEND_HCI_PACKET,
    CMD_CLOSE,
};

class IBtHci : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.hardware.bt.v1_0.IBtHci");

    virtual ~IBtHci() = default;

    static sptr<IBtHci> Get();

    virtual int32_t Init(const sptr<IBtHciCallbacks> &callbacks) = 0;

    virtual int32_t SendHciPacket(BtType type, const std::vector<uint8_t> &data) = 0;

    virtual int32_t Close() = 0;
};
}  // namespace v1_0
}  // namespace bt
}  // namespace hardware
}  // namespace ohos

#endif  // OHOS_HARDWARE_BT_V1_0_IBTHCI_CLIENT_H
