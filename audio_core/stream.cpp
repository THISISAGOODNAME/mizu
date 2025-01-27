// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#include <algorithm>
#include <cmath>

#include "audio_core/sink.h"
#include "audio_core/sink_details.h"
#include "audio_core/sink_stream.h"
#include "audio_core/stream.h"
#include "core/hle/service/kernel_helpers.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/settings.h"

namespace AudioCore {

constexpr std::size_t MaxAudioBufferCount{32};

u32 Stream::GetNumChannels() const {
    switch (format) {
    case Format::Mono16:
        return 1;
    case Format::Stereo16:
        return 2;
    case Format::Multi51Channel16:
        return 6;
    }
    UNIMPLEMENTED_MSG("Unimplemented format={}", static_cast<u32>(format));
    return {};
}

Stream::Stream(u32 sample_rate_, Format format_,
               ReleaseCallback&& release_callback_, SinkStream& sink_stream_, std::string&& name_)
    : sample_rate{sample_rate_}, format{format_}, release_callback{std::move(release_callback_)},
      sink_stream{sink_stream_}, name{std::move(name_)} {
    release_event = Service::KernelHelpers::CreateTimerEvent(
        name,
        this,
        [](::sigval sigev_value) {
            auto stream = static_cast<Stream *>(sigev_value.sival_ptr);
            stream->ReleaseActiveBuffer();
        });
}

Stream::~Stream() {
    ASSERT(stop_source.request_stop());
    {
        std::unique_lock lock{mutex};
        while (!is_done)
            done_cv.wait(lock);
    }
    Service::KernelHelpers::CloseTimerEvent(release_event);
}

void Stream::Play() {
    std::unique_lock lock{mutex};
    state = State::Playing;
    PlayNextBuffer();
}

void Stream::Stop() {
    std::unique_lock lock{mutex};
    state = State::Stopped;
    UNIMPLEMENTED();
}

bool Stream::Flush() {
    const bool had_buffers = !queued_buffers.empty();
    while (!queued_buffers.empty()) {
        queued_buffers.pop();
    }
    return had_buffers;
}

void Stream::SetVolume(float volume) {
    game_volume = volume;
}

Stream::State Stream::GetState() const {
    return state;
}

std::chrono::nanoseconds Stream::GetBufferReleaseNS(const Buffer& buffer) const {
    const std::size_t num_samples{buffer.GetSamples().size() / GetNumChannels()};
    return std::chrono::nanoseconds((static_cast<u64>(num_samples) * 1000000000ULL) / sample_rate);
}

static void VolumeAdjustSamples(std::vector<s16>& samples, float game_volume) {
    const float volume{std::clamp(Settings::Volume() - (1.0f - game_volume), 0.0f, 1.0f)};

    if (volume == 1.0f) {
        return;
    }

    // Implementation of a volume slider with a dynamic range of 60 dB
    const float volume_scale_factor = volume == 0 ? 0 : std::exp(6.90775f * volume) * 0.001f;
    for (auto& sample : samples) {
        sample = static_cast<s16>(sample * volume_scale_factor);
    }
}

void Stream::PlayNextBuffer() {
    is_done = true;

    if (stop_source.stop_requested()) {
        done_cv.notify_all();
        return;
    }

    if (!IsPlaying(true)) {
        // Ensure we are in playing state before playing the next buffer
        sink_stream.Flush();
        done_cv.notify_all();
        return;
    }

    if (active_buffer) {
        // Do not queue a new buffer if we are already playing a buffer
        done_cv.notify_all();
        return;
    }

    if (queued_buffers.empty()) {
        // No queued buffers - we are effectively paused
        sink_stream.Flush();
        done_cv.notify_all();
        return;
    }

    is_done = false;

    active_buffer = queued_buffers.front();
    queued_buffers.pop();

    auto& samples = active_buffer->GetSamples();

    VolumeAdjustSamples(samples, game_volume);

    sink_stream.EnqueueSamples(GetNumChannels(), samples);
    played_samples += samples.size();

    const auto buffer_release_ns = GetBufferReleaseNS(*active_buffer);

    Service::KernelHelpers::ScheduleTimerEvent(buffer_release_ns, release_event);
}

void Stream::ReleaseActiveBuffer() {
    std::unique_lock lock{mutex};
    ASSERT(active_buffer);
    released_buffers.push(std::move(active_buffer));
    release_callback();
    PlayNextBuffer();
}

bool Stream::QueueBuffer(BufferPtr&& buffer) {
    std::unique_lock lock{mutex};
    if (queued_buffers.size() < MaxAudioBufferCount) {
        queued_buffers.push(std::move(buffer));
        PlayNextBuffer();
        return true;
    }
    return false;
}

bool Stream::ContainsBuffer([[maybe_unused]] Buffer::Tag tag) const {
    UNIMPLEMENTED();
    return {};
}

std::vector<Buffer::Tag> Stream::GetTagsAndReleaseBuffers(std::size_t max_count) {
    std::vector<Buffer::Tag> tags;
    for (std::size_t count = 0; count < max_count && !released_buffers.empty(); ++count) {
        if (released_buffers.front()) {
            tags.push_back(released_buffers.front()->GetTag());
        } else {
            ASSERT_MSG(false, "Invalid tag in released_buffers!");
        }
        released_buffers.pop();
    }
    return tags;
}

std::vector<Buffer::Tag> Stream::GetTagsAndReleaseBuffers() {
    std::vector<Buffer::Tag> tags;
    tags.reserve(released_buffers.size());
    while (!released_buffers.empty()) {
        if (released_buffers.front()) {
            tags.push_back(released_buffers.front()->GetTag());
        } else {
            ASSERT_MSG(false, "Invalid tag in released_buffers!");
        }
        released_buffers.pop();
    }
    return tags;
}

} // namespace AudioCore
