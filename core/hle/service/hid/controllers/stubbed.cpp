// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#include <cstring>
#include <ctime>
#include "common/common_types.h"
#include "core/hle/service/hid/controllers/stubbed.h"

namespace Service::HID {

Controller_Stubbed::Controller_Stubbed() : ControllerLockedBase{} {}
Controller_Stubbed::~Controller_Stubbed() = default;

void Controller_Stubbed::OnInit() {}

void Controller_Stubbed::OnRelease() {}

void Controller_Stubbed::OnUpdate(u8* data,
                                  std::size_t size) {
    if (!smart_update) {
        return;
    }

    CommonHeader header{};
    header.timestamp = static_cast<s64_le>(::clock());
    header.total_entry_count = 17;
    header.entry_count = 0;
    header.last_entry_index = 0;

    std::memcpy(data + common_offset, &header, sizeof(CommonHeader));
}

void Controller_Stubbed::OnLoadInputDevices() {}

void Controller_Stubbed::SetCommonHeaderOffset(std::size_t off) {
    common_offset = off;
    smart_update = true;
}
} // namespace Service::HID
