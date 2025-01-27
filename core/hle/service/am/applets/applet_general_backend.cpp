// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#include <string_view>

#include "common/assert.h"
#include "common/hex_util.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/frontend/applets/general_frontend.h"
#include "core/hle/result.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/am/applets/applet_general_backend.h"
#include "core/reporter.h"

namespace Service::AM::Applets {

constexpr ResultCode ERROR_INVALID_PIN{ErrorModule::PCTL, 221};

static void LogCurrentStorage(AppletDataBroker& broker, std::string_view prefix) {
    std::shared_ptr<IStorage> storage = broker.PopNormalDataToApplet();
    for (; storage != nullptr; storage = broker.PopNormalDataToApplet()) {
        const auto data = storage->GetData();
        LOG_INFO(Service_AM,
                 "called (STUBBED), during {} received normal data with size={:08X}, data={}",
                 prefix, data.size(), Common::HexToString(data));
    }

    storage = broker.PopInteractiveDataToApplet();
    for (; storage != nullptr; storage = broker.PopInteractiveDataToApplet()) {
        const auto data = storage->GetData();
        LOG_INFO(Service_AM,
                 "called (STUBBED), during {} received interactive data with size={:08X}, data={}",
                 prefix, data.size(), Common::HexToString(data));
    }
}

Auth::Auth(LibraryAppletMode applet_mode_,
           Core::Frontend::ParentalControlsApplet& frontend_)
    : Applet{applet_mode_}, frontend{frontend_} {}

Auth::~Auth() = default;

void Auth::Initialize() {
    Applet::Initialize();
    complete = false;

    const auto storage = broker.PopNormalDataToApplet();
    ASSERT(storage != nullptr);
    const auto data = storage->GetData();
    ASSERT(data.size() >= 0xC);

    struct Arg {
        INSERT_PADDING_BYTES(4);
        AuthAppletType type;
        u8 arg0;
        u8 arg1;
        u8 arg2;
        INSERT_PADDING_BYTES(1);
    };
    static_assert(sizeof(Arg) == 0xC, "Arg (AuthApplet) has incorrect size.");

    Arg arg{};
    std::memcpy(&arg, data.data(), sizeof(Arg));

    type = arg.type;
    arg0 = arg.arg0;
    arg1 = arg.arg1;
    arg2 = arg.arg2;
}

bool Auth::TransactionComplete() const {
    return complete;
}

ResultCode Auth::GetStatus() const {
    return successful ? ResultSuccess : ERROR_INVALID_PIN;
}

void Auth::ExecuteInteractive() {
    UNREACHABLE_MSG("Unexpected interactive applet data.");
}

void Auth::Execute() {
    if (complete) {
        return;
    }

    const auto unimplemented_log = [this] {
        UNIMPLEMENTED_MSG("Unimplemented Auth applet type for type={:08X}, arg0={:02X}, "
                          "arg1={:02X}, arg2={:02X}",
                          type, arg0, arg1, arg2);
    };

    switch (type) {
    case AuthAppletType::ShowParentalAuthentication: {
        const auto callback = [this](bool is_successful) { AuthFinished(is_successful); };

        if (arg0 == 1 && arg1 == 0 && arg2 == 1) {
            // ShowAuthenticatorForConfiguration
            frontend.VerifyPINForSettings(callback);
        } else if (arg1 == 0 && arg2 == 0) {
            // ShowParentalAuthentication(bool)
            frontend.VerifyPIN(callback, static_cast<bool>(arg0));
        } else {
            unimplemented_log();
        }
        break;
    }
    case AuthAppletType::RegisterParentalPasscode: {
        const auto callback = [this] { AuthFinished(true); };

        if (arg0 == 0 && arg1 == 0 && arg2 == 0) {
            // RegisterParentalPasscode
            frontend.RegisterPIN(callback);
        } else {
            unimplemented_log();
        }
        break;
    }
    case AuthAppletType::ChangeParentalPasscode: {
        const auto callback = [this] { AuthFinished(true); };

        if (arg0 == 0 && arg1 == 0 && arg2 == 0) {
            // ChangeParentalPasscode
            frontend.ChangePIN(callback);
        } else {
            unimplemented_log();
        }
        break;
    }
    default:
        unimplemented_log();
    }
}

void Auth::AuthFinished(bool is_successful) {
    successful = is_successful;

    struct Return {
        ResultCode result_code;
    };
    static_assert(sizeof(Return) == 0x4, "Return (AuthApplet) has incorrect size.");

    Return return_{GetStatus()};

    std::vector<u8> out(sizeof(Return));
    std::memcpy(out.data(), &return_, sizeof(Return));

    broker.PushNormalDataFromApplet(std::make_shared<IStorage>(std::move(out)));
    broker.SignalStateChanged();
}

PhotoViewer::PhotoViewer(LibraryAppletMode applet_mode_,
                         const Core::Frontend::PhotoViewerApplet& frontend_)
    : Applet{applet_mode_}, frontend{frontend_} {}

PhotoViewer::~PhotoViewer() = default;

void PhotoViewer::Initialize() {
    Applet::Initialize();
    complete = false;

    const auto storage = broker.PopNormalDataToApplet();
    ASSERT(storage != nullptr);
    const auto data = storage->GetData();
    ASSERT(!data.empty());
    mode = static_cast<PhotoViewerAppletMode>(data[0]);
}

bool PhotoViewer::TransactionComplete() const {
    return complete;
}

ResultCode PhotoViewer::GetStatus() const {
    return ResultSuccess;
}

void PhotoViewer::ExecuteInteractive() {
    UNREACHABLE_MSG("Unexpected interactive applet data.");
}

void PhotoViewer::Execute() {
    if (complete)
        return;

    const auto callback = [this] { ViewFinished(); };
    switch (mode) {
    case PhotoViewerAppletMode::CurrentApp:
        frontend.ShowPhotosForApplication(GetTitleID(), callback);
        break;
    case PhotoViewerAppletMode::AllApps:
        frontend.ShowAllPhotos(callback);
        break;
    default:
        UNIMPLEMENTED_MSG("Unimplemented PhotoViewer applet mode={:02X}!", mode);
    }
}

void PhotoViewer::ViewFinished() {
    broker.PushNormalDataFromApplet(std::make_shared<IStorage>(std::vector<u8>{}));
    broker.SignalStateChanged();
}

StubApplet::StubApplet(AppletId id_, LibraryAppletMode applet_mode_)
    : Applet{applet_mode_}, id{id_} {}

StubApplet::~StubApplet() = default;

void StubApplet::Initialize() {
    LOG_WARNING(Service_AM, "called (STUBBED)");
    Applet::Initialize();

    const auto data = broker.PeekDataToAppletForDebug();
    reporter.SaveUnimplementedAppletReport(
        static_cast<u32>(id), common_args.arguments_version, common_args.library_version,
        common_args.theme_color, common_args.play_startup_sound, common_args.system_tick,
        data.normal, data.interactive);

    LogCurrentStorage(broker, "Initialize");
}

bool StubApplet::TransactionComplete() const {
    LOG_WARNING(Service_AM, "called (STUBBED)");
    return true;
}

ResultCode StubApplet::GetStatus() const {
    LOG_WARNING(Service_AM, "called (STUBBED)");
    return ResultSuccess;
}

void StubApplet::ExecuteInteractive() {
    LOG_WARNING(Service_AM, "called (STUBBED)");
    LogCurrentStorage(broker, "ExecuteInteractive");

    broker.PushNormalDataFromApplet(std::make_shared<IStorage>(std::vector<u8>(0x1000)));
    broker.PushInteractiveDataFromApplet(
        std::make_shared<IStorage>(std::vector<u8>(0x1000)));
    broker.SignalStateChanged();
}

void StubApplet::Execute() {
    LOG_WARNING(Service_AM, "called (STUBBED)");
    LogCurrentStorage(broker, "Execute");

    broker.PushNormalDataFromApplet(std::make_shared<IStorage>(std::vector<u8>(0x1000)));
    broker.PushInteractiveDataFromApplet(
        std::make_shared<IStorage>(std::vector<u8>(0x1000)));
    broker.SignalStateChanged();
}

} // namespace Service::AM::Applets
