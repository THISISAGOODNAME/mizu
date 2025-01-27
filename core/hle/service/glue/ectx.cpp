// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#include "core/hle/service/glue/ectx.h"

namespace Service::Glue {

ECTX_AW::ECTX_AW() : ServiceFramework{"ectx:aw"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "CreateContextRegistrar"},
        {1, nullptr, "CommitContext"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

ECTX_AW::~ECTX_AW() = default;

} // namespace Service::Glue
