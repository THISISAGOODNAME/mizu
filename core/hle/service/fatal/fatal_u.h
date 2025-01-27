// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#pragma once

#include "core/hle/service/fatal/fatal.h"

namespace Service::Fatal {

class Fatal_U final : public Module::Interface {
public:
    explicit Fatal_U(std::shared_ptr<Module> module_, Core::System& system_);
    ~Fatal_U() override;
};

} // namespace Service::Fatal
