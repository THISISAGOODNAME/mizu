// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#pragma once

#include <memory>
#include "common/common_types.h"
#include "core/file_sys/vfs.h"
#include "core/loader/loader.h"

namespace FileSys {
class ContentProvider;
class NACP;
class NSP;
} // namespace FileSys

namespace Service::FileSystem {
class FileSystemController;
}

namespace Loader {

class AppLoader_NCA;

/// Loads an XCI file
class AppLoader_NSP final : public AppLoader {
public:
    explicit AppLoader_NSP(FileSys::VirtualFile file_, u64 program_id,
                           std::size_t program_index);
    ~AppLoader_NSP() override;

    /**
     * Identifies whether or not the given file is an NSP file.
     *
     * @param nsp_file The file to identify.
     *
     * @return FileType::NSP, or FileType::Error if the file is not an NSP.
     */
    static FileType IdentifyType(const FileSys::VirtualFile& nsp_file);

    FileType GetFileType() const override {
        return IdentifyType(file);
    }

    LoadResult Load(::pid_t pid, std::vector<Kernel::CodeSet>& codesets) override;

    ResultStatus ReadRomFS(FileSys::VirtualFile& out_file) override;
    u64 ReadRomFSIVFCOffset() const override;
    ResultStatus ReadUpdateRaw(FileSys::VirtualFile& out_file) override;
    ResultStatus ReadProgramId(u64& out_program_id) override;
    ResultStatus ReadProgramIds(std::vector<u64>& out_program_ids) override;
    ResultStatus ReadIcon(std::vector<u8>& buffer) override;
    ResultStatus ReadTitle(std::string& title) override;
    ResultStatus ReadControlData(FileSys::NACP& nacp) override;
    ResultStatus ReadManualRomFS(FileSys::VirtualFile& out_file) override;

    ResultStatus ReadBanner(std::vector<u8>& buffer) override;
    ResultStatus ReadLogo(std::vector<u8>& buffer) override;

    FileSys::ProgramMetadata LoadedMetadata() const override {
        return secondary_loader->LoadedMetadata();
    }

private:
    std::unique_ptr<FileSys::NSP> nsp;
    std::unique_ptr<AppLoader> secondary_loader;

    FileSys::VirtualFile icon_file;
    std::unique_ptr<FileSys::NACP> nacp_file;
};

} // namespace Loader
