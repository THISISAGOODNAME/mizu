// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#pragma once

#include <memory>
#include "common/common_types.h"
#include "core/file_sys/vfs.h"
#include "core/hle/result.h"

namespace Loader {
class AppLoader;
} // namespace Loader

namespace Service::FileSystem {
class FileSystemController;
}

namespace FileSys {

class ContentProvider;
class NCA;

enum class ContentRecordType : u8;

enum class StorageId : u8 {
    None = 0,
    Host = 1,
    GameCard = 2,
    NandSystem = 3,
    NandUser = 4,
    SdCard = 5,
};

/// File system interface to the RomFS archive
class RomFSFactory {
public:
    explicit RomFSFactory(Loader::AppLoader& app_loader);
    ~RomFSFactory();

    void SetPackedUpdate(VirtualFile update_raw_file);
    [[nodiscard]] ResultVal<VirtualFile> OpenCurrentProcess(u64 current_process_title_id) const;
    [[nodiscard]] ResultVal<VirtualFile> OpenPatchedRomFS(u64 title_id,
                                                          ContentRecordType type) const;
    [[nodiscard]] ResultVal<VirtualFile> OpenPatchedRomFSWithProgramIndex(
        u64 title_id, u8 program_index, ContentRecordType type) const;
    [[nodiscard]] ResultVal<VirtualFile> Open(u64 title_id, StorageId storage,
                                              ContentRecordType type) const;

private:
    [[nodiscard]] std::shared_ptr<NCA> GetEntry(u64 title_id, StorageId storage,
                                                ContentRecordType type) const;

    VirtualFile file;
    VirtualFile update_raw;
    bool updatable;
    u64 ivfc_offset;
};

} // namespace FileSys
