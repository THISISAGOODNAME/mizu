// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#include "core/hle/service/am/tcap.h"

namespace Service::AM {

TCAP::TCAP() : ServiceFramework{"tcap"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetContinuousHighSkinTemperatureEvent"},
        {1, nullptr, "SetOperationMode"},
        {2, nullptr, "LoadAndApplySettings"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

TCAP::~TCAP() = default;

} // namespace Service::AM
