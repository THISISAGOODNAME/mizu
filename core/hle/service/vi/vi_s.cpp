// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#include "common/logging/log.h"
#include "core/hle/service/vi/vi.h"
#include "core/hle/service/vi/vi_s.h"

namespace Service::VI {

VI_S::VI_S()
    : ServiceFramework{"vi:s"} {
    static const FunctionInfo functions[] = {
        {1, &VI_S::GetDisplayService, "GetDisplayService"},
        {3, nullptr, "GetDisplayServiceWithProxyNameExchange"},
    };
    RegisterHandlers(functions);
}

VI_S::~VI_S() = default;

void VI_S::GetDisplayService(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_VI, "called");

    detail::GetDisplayServiceImpl(ctx, Permission::System);
}

} // namespace Service::VI
