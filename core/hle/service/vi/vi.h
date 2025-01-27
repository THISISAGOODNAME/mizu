// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#pragma once

#include "common/common_types.h"

namespace Kernel {
class HLERequestContext;
}

namespace Service::NVFlinger {
class NVFlinger;
}

namespace Service::SM {
class ServiceManager;
}

namespace Service::VI {

enum class DisplayResolution : u32 {
    DockedWidth = 1920,
    DockedHeight = 1080,
    UndockedWidth = 1280,
    UndockedHeight = 720,
};

/// Permission level for a particular VI service instance
enum class Permission {
    User,
    System,
    Manager,
};

/// A policy type that may be requested via GetDisplayService and
/// GetDisplayServiceWithProxyNameExchange
enum class Policy {
    User,
    Compositor,
};

namespace detail {
void GetDisplayServiceImpl(Kernel::HLERequestContext& ctx, Permission permission);
} // namespace detail

/// Registers all VI services with specified service manager.
void InstallInterfaces();

} // namespace Service::VI
