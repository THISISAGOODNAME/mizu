// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#pragma once

#include <vector>
#include "core/hle/service/service.h"

namespace AudioCore {
class AudioOut;
}

namespace Core {
class System;
}

namespace Kernel {
class HLERequestContext;
}

namespace Service::Audio {

class IAudioOut;

class AudOutU final : public ServiceFramework<AudOutU> {
public:
    explicit AudOutU();
    ~AudOutU() override;

private:
    void ListAudioOutsImpl(Kernel::HLERequestContext& ctx);
    void OpenAudioOutImpl(Kernel::HLERequestContext& ctx);

    std::vector<std::shared_ptr<IAudioOut>> audio_out_interfaces;
    std::unique_ptr<AudioCore::AudioOut> audio_core;
};

} // namespace Service::Audio
