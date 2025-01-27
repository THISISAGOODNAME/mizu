// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <linux/binfmts.h>
#include <linux/horizon.h>
#include "common/concepts.h"
#include "common/fs/path_util.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "common/fs/fs.h"
#include "common/thread.h"
#include "core/core.h"
#include "core/file_sys/vfs_concat.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/glue/glue_manager.h"
#include "core/loader/deconstructed_rom_directory.h"
/* #include "core/loader/elf.h" */
/* #include "core/loader/kip.h" */
/* #include "core/loader/nax.h" */
#include "core/loader/nca.h"
#include "core/loader/nro.h"
#include "core/loader/nso.h"
#include "core/loader/nsp.h"
/* #include "core/loader/xci.h" */

namespace Loader {

namespace {

template <Common::DerivedFrom<AppLoader> T>
std::optional<FileType> IdentifyFileLoader(FileSys::VirtualFile file) {
    const auto file_type = T::IdentifyType(file);
    if (file_type != FileType::Error) {
        return file_type;
    }
    return std::nullopt;
}

} // namespace

FileType IdentifyFile(FileSys::VirtualFile file) {
    if (const auto romdir_type = IdentifyFileLoader<AppLoader_DeconstructedRomDirectory>(file)) {
        return *romdir_type;
    } else if (const auto nso_type = IdentifyFileLoader<AppLoader_NSO>(file)) {
        return *nso_type;
    } else if (const auto nro_type = IdentifyFileLoader<AppLoader_NRO>(file)) {
        return *nro_type;
    } else if (const auto nca_type = IdentifyFileLoader<AppLoader_NCA>(file)) {
        return *nca_type;
    /* } else if (const auto xci_type = IdentifyFileLoader<AppLoader_XCI>(file)) { */
    /*     return *xci_type; */
    /* } else if (const auto nax_type = IdentifyFileLoader<AppLoader_NAX>(file)) { */
    /*     return *nax_type; */
    } else if (const auto nsp_type = IdentifyFileLoader<AppLoader_NSP>(file)) {
        return *nsp_type;
    /* } else if (const auto kip_type = IdentifyFileLoader<AppLoader_KIP>(file)) { */
    /*     return *kip_type; */
    } else {
        return FileType::Unknown;
    }
}

FileType GuessFromFilename(const std::string& name) {
    if (name == "main")
        return FileType::DeconstructedRomDirectory;
    if (name == "00")
        return FileType::NCA;

    const std::string extension =
        Common::ToLower(std::string(Common::FS::GetExtensionFromFilename(name)));

    if (extension == "nro")
        return FileType::NRO;
    if (extension == "nso")
        return FileType::NSO;
    if (extension == "nca")
        return FileType::NCA;
    if (extension == "xci")
        return FileType::XCI;
    if (extension == "nsp")
        return FileType::NSP;
    if (extension == "kip")
        return FileType::KIP;

    return FileType::Unknown;
}

std::string GetFileTypeString(FileType type) {
    switch (type) {
    case FileType::NRO:
        return "NRO";
    case FileType::NSO:
        return "NSO";
    case FileType::NCA:
        return "NCA";
    case FileType::XCI:
        return "XCI";
    case FileType::NAX:
        return "NAX";
    case FileType::NSP:
        return "NSP";
    case FileType::KIP:
        return "KIP";
    case FileType::DeconstructedRomDirectory:
        return "Directory";
    case FileType::Error:
    case FileType::Unknown:
        break;
    }

    return "unknown";
}

constexpr std::array<const char*, 66> RESULT_MESSAGES{
    "The operation completed successfully.",
    "The loader requested to load is already loaded.",
    "The operation is not implemented.",
    "The loader is not initialized properly.",
    "The NPDM file has a bad header.",
    "The NPDM has a bad ACID header.",
    "The NPDM has a bad ACI header,",
    "The NPDM file has a bad file access control.",
    "The NPDM has a bad file access header.",
    "The NPDM has bad kernel capability descriptors.",
    "The PFS/HFS partition has a bad header.",
    "The PFS/HFS partition has incorrect size as determined by the header.",
    "The NCA file has a bad header.",
    "The general keyfile could not be found.",
    "The NCA Header key could not be found.",
    "The NCA Header key is incorrect or the header is invalid.",
    "Support for NCA2-type NCAs is not implemented.",
    "Support for NCA0-type NCAs is not implemented.",
    "The titlekey for this Rights ID could not be found.",
    "The titlekek for this crypto revision could not be found.",
    "The Rights ID in the header is invalid.",
    "The key area key for this application type and crypto revision could not be found.",
    "The key area key is incorrect or the section header is invalid.",
    "The titlekey and/or titlekek is incorrect or the section header is invalid.",
    "The XCI file is missing a Program-type NCA.",
    "The NCA file is not an application.",
    "The ExeFS partition could not be found.",
    "The XCI file has a bad header.",
    "The XCI file is missing a partition.",
    "The file could not be found or does not exist.",
    "The game is missing a program metadata file (main.npdm).",
    "The game uses the currently-unimplemented 32-bit architecture.",
    "Unable to completely parse the kernel metadata when loading the emulated process",
    "The RomFS could not be found.",
    "The ELF file has incorrect size as determined by the header.",
    "There was a general error loading the NRO into emulated memory.",
    "There was a general error loading the NSO into emulated memory.",
    "There is no icon available.",
    "There is no control data available.",
    "The NAX file has a bad header.",
    "The NAX file has incorrect size as determined by the header.",
    "The HMAC to generated the NAX decryption keys failed.",
    "The HMAC to validate the NAX decryption keys failed.",
    "The NAX key derivation failed.",
    "The NAX file cannot be interpreted as an NCA file.",
    "The NAX file has an incorrect path.",
    "The SD seed could not be found or derived.",
    "The SD KEK Source could not be found.",
    "The AES KEK Generation Source could not be found.",
    "The AES Key Generation Source could not be found.",
    "The SD Save Key Source could not be found.",
    "The SD NCA Key Source could not be found.",
    "The NSP file is missing a Program-type NCA.",
    "The BKTR-type NCA has a bad BKTR header.",
    "The BKTR Subsection entry is not located immediately after the Relocation entry.",
    "The BKTR Subsection entry is not at the end of the media block.",
    "The BKTR-type NCA has a bad Relocation block.",
    "The BKTR-type NCA has a bad Subsection block.",
    "The BKTR-type NCA has a bad Relocation bucket.",
    "The BKTR-type NCA has a bad Subsection bucket.",
    "The BKTR-type NCA is missing the base RomFS.",
    "The NSP or XCI does not contain an update in addition to the base game.",
    "The KIP file has a bad header.",
    "The KIP BLZ decompression of the section failed unexpectedly.",
    "The INI file has a bad header.",
    "The INI file contains more than the maximum allowable number of KIP files.",
};

std::string GetResultStatusString(ResultStatus status) {
    return RESULT_MESSAGES.at(static_cast<std::size_t>(status));
}

std::ostream& operator<<(std::ostream& os, ResultStatus status) {
    os << RESULT_MESSAGES.at(static_cast<std::size_t>(status));
    return os;
}

AppLoader::AppLoader(FileSys::VirtualFile file_) : file(std::move(file_)) {}
AppLoader::~AppLoader() = default;

/**
 * Get a loader for a file with a specific type
 * @param file   The file to retrieve the loader for
 * @param type   The file type
 * @param program_index Specifies the index within the container of the program to launch.
 * @return std::unique_ptr<AppLoader> a pointer to a loader object;  nullptr for unsupported type
 */
static std::unique_ptr<AppLoader> GetFileLoader(FileSys::VirtualFile file,
                                                FileType type, u64 program_id,
                                                std::size_t program_index) {
    switch (type) {
    // NX NSO file format.
    case FileType::NSO:
        return std::make_unique<AppLoader_NSO>(std::move(file));

    // NX NRO file format.
    case FileType::NRO:
        return std::make_unique<AppLoader_NRO>(std::move(file));

    // NX NCA (Nintendo Content Archive) file format.
    case FileType::NCA:
        return std::make_unique<AppLoader_NCA>(std::move(file));

#if 0 // mizu TEMP
    // NX XCI (nX Card Image) file format.
    case FileType::XCI:
        return std::make_unique<AppLoader_XCI>(std::move(file), system.GetFileSystemController(),
                                               system.GetContentProvider(), program_id,
                                               program_index);

    // NX NAX (NintendoAesXts) file format.
    case FileType::NAX:
        return std::make_unique<AppLoader_NAX>(std::move(file));
#endif

    // NX NSP (Nintendo Submission Package) file format
    case FileType::NSP:
        return std::make_unique<AppLoader_NSP>(std::move(file), program_id, program_index);

#if 0 // mizu TEMP
    // NX KIP (Kernel Internal Process) file format
    case FileType::KIP:
        return std::make_unique<AppLoader_KIP>(std::move(file));
#endif

    // NX deconstructed ROM directory.
    case FileType::DeconstructedRomDirectory:
        return std::make_unique<AppLoader_DeconstructedRomDirectory>(std::move(file));

    default:
        return nullptr;
    }
}

std::unique_ptr<AppLoader> GetLoader(FileSys::VirtualFile file,
                                     u64 program_id, std::size_t program_index) {
    FileType type = IdentifyFile(file);
    const FileType filename_type = GuessFromFilename(file->GetName());

    // Special case: 00 is either a NCA or NAX.
    if (type != filename_type && !(file->GetName() == "00" && type == FileType::NAX)) {
        LOG_WARNING(Loader, "File {} has a different type than its extension.", file->GetName());
        if (FileType::Unknown == type) {
            type = filename_type;
        }
    }

    LOG_DEBUG(Loader, "Loading file {} as {}...", file->GetName(), GetFileTypeString(type));

    return GetFileLoader(std::move(file), type, program_id, program_index);
}

[[ noreturn ]] void RunForever() {
    Common::SetCurrentThreadName("mizu:Loader");

    // open the message queue for receiving requests
    mq_attr attr = { 0 };
    attr.mq_maxmsg = 10; // default per mq_overview(7)
    attr.mq_msgsize = PATH_MAX;
    mqd_t mqd = ::mq_open("/mizu_loader", O_RDONLY | O_CLOEXEC | O_CREAT,
                          0666, &attr);
    if (mqd == -1) {
        ::perror("mq_open failed");
        ::exit(1);
    }

    // cleanup on exit
    ::atexit([](){ ::mq_unlink("/mizu_loader"); });

    // for some reason the mq_open mode doesn't seem to work really, do this to
    // be sure
    ::chmod("/dev/mqueue/mizu_loader", 0666);

    char path[PATH_MAX];
    for (;;) {
        ::pid_t pid;

        // reap any exited children and cleanup state
        while ((pid = ::waitpid(-1, NULL, WNOHANG)) > 0)
            Service::SharedWriter(Service::filesystem_controller)->UnregisterRomFS(pid);

        // block until received load request
        ssize_t r = ::mq_receive(mqd, path, PATH_MAX, NULL);
        if (r == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                ::perror("mq_receive failed");
                ::mq_close(mqd);
                ::exit(1);
            }
        }
        path[r] = 0;

        FileSys::VirtualFile file = nullptr;

        // To account for split 00+01+etc files.
        std::string dir_name;
        std::string filename;
        Common::SplitPath(path, &dir_name, &filename, nullptr);

        {
            auto vfs = Service::SharedWriter(Service::filesystem);
            if (filename == "00") {
                const auto dir = vfs->OpenDirectory(dir_name, FileSys::Mode::Read);
                std::vector<FileSys::VirtualFile> concat;

                for (u32 i = 0; i < 0x10; ++i) {
                    const auto file_name = fmt::format("{:02X}", i);
                    auto next = dir->GetFile(file_name);

                    if (next != nullptr) {
                        concat.push_back(std::move(next));
                    } else {
                        next = dir->GetFile(file_name);

                        if (next == nullptr) {
                            break;
                        }

                        concat.push_back(std::move(next));
                    }
                }

                if (concat.empty()) {
                    file = nullptr;
                } else {
                    file = FileSys::ConcatenatedVfsFile::MakeConcatenatedFile(std::move(concat),
                                                                              dir->GetName());
                }
            } else if (Common::FS::IsDir(path)) {
                file = vfs->OpenFile(path + std::string("/main"), FileSys::Mode::Read);
            } else {
                file = vfs->OpenFile(path, FileSys::Mode::Read);
            }
        }

        if (!file) {
            continue;
        }

        auto app_loader = GetLoader(file, 0, 0);

        if (!app_loader) {
            LOG_CRITICAL(Core, "Failed to obtain loader for '{}'!", path);
            continue;
        }

        // open temporary file
        int fd = ::open(std::filesystem::temp_directory_path().string().c_str(),
                        O_TMPFILE | O_WRONLY, S_IRWXU);
        if (fd == -1) {
            LOG_CRITICAL(Core, "open O_TMPFILE failed: {}!", ::strerror(errno));
            continue;
        }

        // synchronization pipe
        int pipefd[2];
        if (::pipe(pipefd) == -1) {
            LOG_CRITICAL(Core, "pipe2 failed: {}", ::strerror(errno));
            ::close(fd);
            continue;
        }

        static char whocares;

        // fork/exec the process
        pid = ::fork();
        if (pid == -1) {
            LOG_CRITICAL(Core, "fork failed: {}", ::strerror(errno));
            ::close(fd);
            ::close(pipefd[0]);
            ::close(pipefd[1]);
            continue;
        }
        if (pid == 0) {
            // open temp executable O_PATH for execveat
            int pfd = ::open(("/proc/self/fd/" + std::to_string(fd)).c_str(),
                             O_PATH | O_CLOEXEC);
            if (pfd == -1) {
                ::perror("mizu loader child: open failed");
                ::_exit(1);
            }
            ::close(fd);

            // set process name by app title
            std::string title;
            char *procname;
            if (app_loader->ReadTitle(title) == ResultStatus::Success)
                procname = const_cast<char *>(title.c_str());
            else
                procname = basename(path);

            // wait for parent to write temporary and setup fs controller
            ::close(pipefd[1]);
            ssize_t r = ::read(pipefd[0], &whocares, 1);
            ::close(pipefd[0]);
            if (r < 1) {
                if (r == -1)
                    perror("mizu loader child: read from pipe failed");
                ::_exit(1);
            }

            // exec the temporary
            char * const av[] = { procname, NULL }, * const ev[] = { NULL };
            ::syscall(__NR_horizon_execveat, pfd, std::string().c_str(), av, ev, AT_EMPTY_PATH);
            perror("mizu loader child: horizon_execveat failed");
            ::_exit(1);
        }
        ::close(pipefd[0]);

        // load data
        std::vector<Kernel::CodeSet> codesets;
        const auto [load_result, load_parameters] = app_loader->Load(pid, codesets);
        if (load_result != ResultStatus::Success) {
            LOG_CRITICAL(Core, "Failed to load ROM at '{}' (Error {})!", path, load_result);
            ::close(fd);
            ::close(pipefd[1]);
            continue;
        }

        u64 title_id = 0;
        app_loader->ReadProgramId(title_id); // read in Title ID if there

        std::vector<u8> nacp_data;
        FileSys::NACP nacp;
        if (app_loader->ReadControlData(nacp) == Loader::ResultStatus::Success) {
            nacp_data = nacp.GetRawBytes();
        } else {
            nacp_data.resize(sizeof(FileSys::RawNACP));
        }

        Service::Glue::ApplicationLaunchProperty launch{};
        launch.title_id = title_id;

        FileSys::PatchManager pm{launch.title_id};
        launch.version = pm.GetGameVersion().value_or(0);

        auto GetStorageIdForFrontendSlot =
            [](std::optional<FileSys::ContentProviderUnionSlot> slot) {
            if (!slot.has_value()) {
                return FileSys::StorageId::None;
            }

            switch (*slot) {
            case FileSys::ContentProviderUnionSlot::UserNAND:
                return FileSys::StorageId::NandUser;
            case FileSys::ContentProviderUnionSlot::SysNAND:
                return FileSys::StorageId::NandSystem;
            case FileSys::ContentProviderUnionSlot::SDMC:
                return FileSys::StorageId::SdCard;
            case FileSys::ContentProviderUnionSlot::FrontendManual:
                return FileSys::StorageId::Host;
            default:
                return FileSys::StorageId::None;
            }
        };

        // TODO(DarkLordZach): When FSController/Game Card Support is added, if
        // current_process_game_card use correct StorageId
        launch.base_game_storage_id = GetStorageIdForFrontendSlot(Service::SharedReader(Service::content_provider)->
            GetSlotForEntry(launch.title_id, FileSys::ContentRecordType::Program));
        launch.update_storage_id = GetStorageIdForFrontendSlot(Service::SharedReader(Service::content_provider)->
            GetSlotForEntry(FileSys::GetUpdateTitleID(launch.title_id), FileSys::ContentRecordType::Program));

        Service::SharedWriter(Service::arp_manager)->Register(launch.title_id, launch, std::move(nacp_data));

        // write out data to temporary file
        ssize_t w;
        size_t total = 0;
        bool fail = false;
        auto metadata = app_loader->LoadedMetadata();
        horizon_hdr hdr = {
            .magic = HORIZON_MAGIC,
            .title_id = title_id,
            .ideal_core = metadata.GetMainThreadCore(),
            .is_64bit = metadata.Is64BitProgram(),
            .address_space_type = static_cast<u8>(metadata.GetAddressSpaceType()),
            .system_resource_size = metadata.GetSystemResourceSize(),
            .main_thread_priority = load_parameters->main_thread_priority,
            .num_codesets = static_cast<u32>(codesets.size()),
        };
        static_assert(sizeof(hdr) <= BINPRM_BUF_SIZE);
        if ((w = ::write(fd, &hdr, sizeof(hdr))) != sizeof(hdr)) {
            LOG_CRITICAL(Core, "Failed to write temporary file: {}", w == -1 ? ::strerror(errno) : "N/A");
            ::close(fd);
            ::close(pipefd[1]);
            continue;
        }
        total += w;
        for (const auto& codeset : codesets) {
            if ((w = ::write(fd, &codeset.hdr, sizeof(codeset.hdr))) != sizeof(codeset.hdr)) {
                fail = true;
                break;
            }
            total += w;
        }
        if (fail) {
            LOG_CRITICAL(Core, "Failed to write temporary file: {}", w == -1 ? ::strerror(errno) : "N/A");
            ::close(fd);
            ::close(pipefd[1]);
            continue;
        }
        if (lseek(fd, PageAlignSize(total), SEEK_SET) == -1) {
            LOG_CRITICAL(Core, "lseek failed: {}", ::strerror(errno));
            ::close(fd);
            ::close(pipefd[1]);
            continue;
        }
        for (const auto& codeset : codesets) {
            w = ::write(fd, codeset.GetMemory().data(), codeset.GetMemory().size());
            if (w != codeset.GetMemory().size()) {
                fail = true;
                break;
            }
        }
        ::close(fd);
        if (fail) {
            LOG_CRITICAL(Core, "Failed to write temporary file: {}", w == -1 ? ::strerror(errno) : "N/A");
            ::close(pipefd[1]);
            continue;
        }

        // notify the child to exec
        w = ::write(pipefd[1], &whocares, 1);
        ::close(pipefd[1]);
        if (w == -1) {
            LOG_CRITICAL(Core, "write pipe failed: {}", ::strerror(errno));
        }
    }
}

} // namespace Loader
