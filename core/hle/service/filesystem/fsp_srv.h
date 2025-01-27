// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#pragma once

#include <memory>
#include "core/hle/service/service.h"

namespace Core {
class Reporter;
}

namespace FileSys {
class ContentProvider;
class FileSystemBackend;
} // namespace FileSys

namespace Service::FileSystem {

enum class AccessLogVersion : u32 {
    V7_0_0 = 2,

    Latest = V7_0_0,
};

enum class AccessLogMode : u32 {
    None,
    Log,
    SdCard,
};

class FSP_SRV final : public ServiceFramework<FSP_SRV> {
public:
    explicit FSP_SRV();
    ~FSP_SRV() override;

private:
    void SetCurrentProcess(Kernel::HLERequestContext& ctx);
    void OpenFileSystemWithPatch(Kernel::HLERequestContext& ctx);
    void OpenSdCardFileSystem(Kernel::HLERequestContext& ctx);
    void CreateSaveDataFileSystem(Kernel::HLERequestContext& ctx);
    void OpenSaveDataFileSystem(Kernel::HLERequestContext& ctx);
    void OpenReadOnlySaveDataFileSystem(Kernel::HLERequestContext& ctx);
    void OpenSaveDataInfoReaderBySaveDataSpaceId(Kernel::HLERequestContext& ctx);
    void WriteSaveDataFileSystemExtraDataBySaveDataAttribute(Kernel::HLERequestContext& ctx);
    void ReadSaveDataFileSystemExtraDataWithMaskBySaveDataAttribute(Kernel::HLERequestContext& ctx);
    void OpenDataStorageByCurrentProcess(Kernel::HLERequestContext& ctx);
    void OpenDataStorageByDataId(Kernel::HLERequestContext& ctx);
    void OpenPatchDataStorageByCurrentProcess(Kernel::HLERequestContext& ctx);
    void OpenDataStorageWithProgramIndex(Kernel::HLERequestContext& ctx);
    void DisableAutoSaveDataCreation(Kernel::HLERequestContext& ctx);
    void SetGlobalAccessLogMode(Kernel::HLERequestContext& ctx);
    void GetGlobalAccessLogMode(Kernel::HLERequestContext& ctx);
    void OutputAccessLogToSdCard(Kernel::HLERequestContext& ctx);
    void GetProgramIndexForAccessLog(Kernel::HLERequestContext& ctx);
    void OpenMultiCommitManager(Kernel::HLERequestContext& ctx);

    FileSys::VirtualFile romfs;
    u64 current_process_id = 0;
    u32 access_log_program_index = 0;
    AccessLogMode access_log_mode = AccessLogMode::None;
};

} // namespace Service::FileSystem
