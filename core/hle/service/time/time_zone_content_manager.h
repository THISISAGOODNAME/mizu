// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#pragma once

#include <string>
#include <vector>

#include "core/hle/service/time/time_zone_manager.h"

namespace Service::Time {
class TimeManager;
}

namespace Service::Time::TimeZone {

class TimeZoneContentManager final {
public:
    explicit TimeZoneContentManager(TimeManager& time_manager);

    const TimeZoneManager& GetTimeZoneManager() const {
        return time_zone_manager;
    }

    ResultCode LoadTimeZoneRule(TimeZoneRule& rules, const std::string& location_name) const;

private:
    bool IsLocationNameValid(const std::string& location_name) const;
    ResultCode GetTimeZoneInfoFile(const std::string& location_name,
                                   FileSys::VirtualFile& vfs_file) const;

    ;
    TimeZoneManager time_zone_manager;
    const std::vector<std::string> location_name_cache;
};

} // namespace Service::Time::TimeZone
