// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#pragma once

#include <memory>
#include <string>

#include "audio_core/sink_stream.h"
#include "common/common_types.h"

namespace AudioCore {

constexpr char auto_device_name[] = "auto";

/**
 * This class is an interface for an audio sink. An audio sink accepts samples in stereo signed
 * PCM16 format to be output. Sinks *do not* handle resampling and expect the correct sample rate.
 * They are dumb outputs.
 */
class Sink {
public:
    virtual ~Sink() = default;
    virtual SinkStream& AcquireSinkStream(u32 sample_rate, u32 num_channels,
                                          const std::string& name) = 0;
};

using SinkPtr = std::unique_ptr<Sink>;

} // namespace AudioCore
