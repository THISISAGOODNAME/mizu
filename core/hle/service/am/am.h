// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// Adapted by Kent Hall for mizu on Horizon Linux.

#pragma once

#include <chrono>
#include <memory>
#include <queue>

#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

namespace Kernel {
class KernelCore;
class KTransferMemory;
} // namespace Kernel

namespace Service::NVFlinger {
class NVFlinger;
}

namespace Service::AM {

enum SystemLanguage {
    Japanese = 0,
    English = 1, // en-US
    French = 2,
    German = 3,
    Italian = 4,
    Spanish = 5,
    Chinese = 6,
    Korean = 7,
    Dutch = 8,
    Portuguese = 9,
    Russian = 10,
    Taiwanese = 11,
    BritishEnglish = 12, // en-GB
    CanadianFrench = 13,
    LatinAmericanSpanish = 14, // es-419
    // 4.0.0+
    SimplifiedChinese = 15,
    TraditionalChinese = 16,
};

class AppletMessageQueue {
public:
    enum class AppletMessage : u32 {
        NoMessage = 0,
        ExitRequested = 4,
        FocusStateChanged = 15,
        OperationModeChanged = 30,
        PerformanceModeChanged = 31,
    };

    explicit AppletMessageQueue();
    ~AppletMessageQueue();

    int GetMessageReceiveEvent() const;
    int GetOperationModeChangedEvent() const;
    void PushMessage(AppletMessage msg);
    AppletMessage PopMessage();
    std::size_t GetMessageCount() const;
    void RequestExit();
    void FocusStateChanged();
    void OperationModeChanged();

private:
    int on_new_message;
    int on_operation_mode_changed;

    std::queue<AppletMessage> messages;
};

using AppletMessageQueueMap =
	std::unordered_map<::pid_t, std::pair<size_t,
	                   std::shared_ptr<Shared<AppletMessageQueue>>>>;

class IWindowController final : public ServiceFramework<IWindowController> {
public:
    explicit IWindowController();
    ~IWindowController() override;

private:
    void GetAppletResourceUserId(Kernel::HLERequestContext& ctx);
    void AcquireForegroundRights(Kernel::HLERequestContext& ctx);
};

class IAudioController final : public ServiceFramework<IAudioController> {
public:
    explicit IAudioController();
    ~IAudioController() override;

private:
    void SetExpectedMasterVolume(Kernel::HLERequestContext& ctx);
    void GetMainAppletExpectedMasterVolume(Kernel::HLERequestContext& ctx);
    void GetLibraryAppletExpectedMasterVolume(Kernel::HLERequestContext& ctx);
    void ChangeMainAppletMasterVolume(Kernel::HLERequestContext& ctx);
    void SetTransparentAudioRate(Kernel::HLERequestContext& ctx);

    static constexpr float min_allowed_volume = 0.0f;
    static constexpr float max_allowed_volume = 1.0f;

    float main_applet_volume{0.25f};
    float library_applet_volume{max_allowed_volume};
    float transparent_volume_rate{min_allowed_volume};

    // Volume transition fade time in nanoseconds.
    // e.g. If the main applet volume was 0% and was changed to 50%
    //      with a fade of 50ns, then over the course of 50ns,
    //      the volume will gradually fade up to 50%
    std::chrono::nanoseconds fade_time_ns{0};
};

class IDisplayController final : public ServiceFramework<IDisplayController> {
public:
    explicit IDisplayController();
    ~IDisplayController() override;
};

class IDebugFunctions final : public ServiceFramework<IDebugFunctions> {
public:
    explicit IDebugFunctions();
    ~IDebugFunctions() override;
};

class ISelfController final : public ServiceFramework<ISelfController> {
public:
    explicit ISelfController();
    ~ISelfController() override;

private:
    void Exit(Kernel::HLERequestContext& ctx);
    void LockExit(Kernel::HLERequestContext& ctx);
    void UnlockExit(Kernel::HLERequestContext& ctx);
    void EnterFatalSection(Kernel::HLERequestContext& ctx);
    void LeaveFatalSection(Kernel::HLERequestContext& ctx);
    void GetLibraryAppletLaunchableEvent(Kernel::HLERequestContext& ctx);
    void SetScreenShotPermission(Kernel::HLERequestContext& ctx);
    void SetOperationModeChangedNotification(Kernel::HLERequestContext& ctx);
    void SetPerformanceModeChangedNotification(Kernel::HLERequestContext& ctx);
    void SetFocusHandlingMode(Kernel::HLERequestContext& ctx);
    void SetRestartMessageEnabled(Kernel::HLERequestContext& ctx);
    void SetOutOfFocusSuspendingEnabled(Kernel::HLERequestContext& ctx);
    void SetAlbumImageOrientation(Kernel::HLERequestContext& ctx);
    void CreateManagedDisplayLayer(Kernel::HLERequestContext& ctx);
    void CreateManagedDisplaySeparableLayer(Kernel::HLERequestContext& ctx);
    void SetHandlesRequestToDisplay(Kernel::HLERequestContext& ctx);
    void SetIdleTimeDetectionExtension(Kernel::HLERequestContext& ctx);
    void GetIdleTimeDetectionExtension(Kernel::HLERequestContext& ctx);
    void SetAutoSleepDisabled(Kernel::HLERequestContext& ctx);
    void IsAutoSleepDisabled(Kernel::HLERequestContext& ctx);
    void GetAccumulatedSuspendedTickValue(Kernel::HLERequestContext& ctx);
    void GetAccumulatedSuspendedTickChangedEvent(Kernel::HLERequestContext& ctx);
    void SetAlbumImageTakenNotificationEnabled(Kernel::HLERequestContext& ctx);

    enum class ScreenshotPermission : u32 {
        Inherit = 0,
        Enable = 1,
        Disable = 2,
    };

    int launchable_event;
    int accumulated_suspended_tick_changed_event;

    u32 idle_time_detection_extension = 0;
    u64 num_fatal_sections_entered = 0;
    bool is_auto_sleep_disabled = false;
    ScreenshotPermission screenshot_permission = ScreenshotPermission::Inherit;
};

class ICommonStateGetter final : public ServiceFramework<ICommonStateGetter> {
public:
    explicit ICommonStateGetter(std::shared_ptr<Shared<AppletMessageQueue>> msg_queue_);
    ~ICommonStateGetter() override;

private:
    enum class FocusState : u8 {
        InFocus = 1,
        NotInFocus = 2,
    };

    enum class OperationMode : u8 {
        Handheld = 0,
        Docked = 1,
    };

    void GetEventHandle(Kernel::HLERequestContext& ctx);
    void ReceiveMessage(Kernel::HLERequestContext& ctx);
    void GetCurrentFocusState(Kernel::HLERequestContext& ctx);
    void GetDefaultDisplayResolutionChangeEvent(Kernel::HLERequestContext& ctx);
    void GetOperationMode(Kernel::HLERequestContext& ctx);
    void GetPerformanceMode(Kernel::HLERequestContext& ctx);
    void GetBootMode(Kernel::HLERequestContext& ctx);
    void IsVrModeEnabled(Kernel::HLERequestContext& ctx);
    void SetVrModeEnabled(Kernel::HLERequestContext& ctx);
    void SetLcdBacklighOffEnabled(Kernel::HLERequestContext& ctx);
    void BeginVrModeEx(Kernel::HLERequestContext& ctx);
    void EndVrModeEx(Kernel::HLERequestContext& ctx);
    void GetDefaultDisplayResolution(Kernel::HLERequestContext& ctx);
    void SetCpuBoostMode(Kernel::HLERequestContext& ctx);
    void SetRequestExitToLibraryAppletAtExecuteNextProgramEnabled(Kernel::HLERequestContext& ctx);

    std::shared_ptr<Shared<AppletMessageQueue>> msg_queue;
    bool vr_mode_state{};
};

class IStorageImpl {
public:
    virtual ~IStorageImpl();
    virtual std::vector<u8>& GetData() = 0;
    virtual const std::vector<u8>& GetData() const = 0;
    virtual std::size_t GetSize() const = 0;
};

class IStorage final : public ServiceFramework<IStorage> {
public:
    explicit IStorage(std::vector<u8>&& buffer);
    ~IStorage() override;

    std::vector<u8>& GetData() {
        return impl->GetData();
    }

    const std::vector<u8>& GetData() const {
        return impl->GetData();
    }

    std::size_t GetSize() const {
        return impl->GetSize();
    }

private:
    void Register();
    void Open(Kernel::HLERequestContext& ctx);

    std::shared_ptr<IStorageImpl> impl;
};

class IStorageAccessor final : public ServiceFramework<IStorageAccessor> {
public:
    explicit IStorageAccessor(IStorage& backing_);
    ~IStorageAccessor() override;

private:
    void GetSize(Kernel::HLERequestContext& ctx);
    void Write(Kernel::HLERequestContext& ctx);
    void Read(Kernel::HLERequestContext& ctx);

    IStorage& backing;
};

class ILibraryAppletCreator final : public ServiceFramework<ILibraryAppletCreator> {
public:
    explicit ILibraryAppletCreator();
    ~ILibraryAppletCreator() override;

private:
    void CreateLibraryApplet(Kernel::HLERequestContext& ctx);
    void CreateStorage(Kernel::HLERequestContext& ctx);
    void CreateTransferMemoryStorage(Kernel::HLERequestContext& ctx);
    void CreateHandleStorage(Kernel::HLERequestContext& ctx);
};

class IApplicationFunctions final : public ServiceFramework<IApplicationFunctions> {
public:
    explicit IApplicationFunctions();
    ~IApplicationFunctions() override;

private:
    void PopLaunchParameter(Kernel::HLERequestContext& ctx);
    void CreateApplicationAndRequestToStartForQuest(Kernel::HLERequestContext& ctx);
    void EnsureSaveData(Kernel::HLERequestContext& ctx);
    void SetTerminateResult(Kernel::HLERequestContext& ctx);
    void GetDisplayVersion(Kernel::HLERequestContext& ctx);
    void GetDesiredLanguage(Kernel::HLERequestContext& ctx);
    void IsGamePlayRecordingSupported(Kernel::HLERequestContext& ctx);
    void InitializeGamePlayRecording(Kernel::HLERequestContext& ctx);
    void SetGamePlayRecordingState(Kernel::HLERequestContext& ctx);
    void NotifyRunning(Kernel::HLERequestContext& ctx);
    void GetPseudoDeviceId(Kernel::HLERequestContext& ctx);
    void ExtendSaveData(Kernel::HLERequestContext& ctx);
    void GetSaveDataSize(Kernel::HLERequestContext& ctx);
    void BeginBlockingHomeButtonShortAndLongPressed(Kernel::HLERequestContext& ctx);
    void EndBlockingHomeButtonShortAndLongPressed(Kernel::HLERequestContext& ctx);
    void BeginBlockingHomeButton(Kernel::HLERequestContext& ctx);
    void EndBlockingHomeButton(Kernel::HLERequestContext& ctx);
    void EnableApplicationCrashReport(Kernel::HLERequestContext& ctx);
    void InitializeApplicationCopyrightFrameBuffer(Kernel::HLERequestContext& ctx);
    void SetApplicationCopyrightImage(Kernel::HLERequestContext& ctx);
    void SetApplicationCopyrightVisibility(Kernel::HLERequestContext& ctx);
    void QueryApplicationPlayStatistics(Kernel::HLERequestContext& ctx);
    void QueryApplicationPlayStatisticsByUid(Kernel::HLERequestContext& ctx);
    void ExecuteProgram(Kernel::HLERequestContext& ctx);
    void ClearUserChannel(Kernel::HLERequestContext& ctx);
    void UnpopToUserChannel(Kernel::HLERequestContext& ctx);
    void GetPreviousProgramIndex(Kernel::HLERequestContext& ctx);
    void GetGpuErrorDetectedSystemEvent(Kernel::HLERequestContext& ctx);
    void GetFriendInvitationStorageChannelEvent(Kernel::HLERequestContext& ctx);
    void TryPopFromFriendInvitationStorageChannel(Kernel::HLERequestContext& ctx);
    void GetNotificationStorageChannelEvent(Kernel::HLERequestContext& ctx);
    void GetHealthWarningDisappearedSystemEvent(Kernel::HLERequestContext& ctx);

    bool launch_popped_application_specific = false;
    bool launch_popped_account_preselect = false;
    s32 previous_program_index{-1};
    int gpu_error_detected_event;
    int friend_invitation_storage_channel_event;
    int notification_storage_channel_event;
    int health_warning_disappeared_system_event;
};

class IHomeMenuFunctions final : public ServiceFramework<IHomeMenuFunctions> {
public:
    explicit IHomeMenuFunctions();
    ~IHomeMenuFunctions() override;

private:
    void RequestToGetForeground(Kernel::HLERequestContext& ctx);
    void GetPopFromGeneralChannelEvent(Kernel::HLERequestContext& ctx);

    int pop_from_general_channel_event;
};

class IGlobalStateController final : public ServiceFramework<IGlobalStateController> {
public:
    explicit IGlobalStateController();
    ~IGlobalStateController() override;
};

class IApplicationCreator final : public ServiceFramework<IApplicationCreator> {
public:
    explicit IApplicationCreator();
    ~IApplicationCreator() override;
};

class IProcessWindingController final : public ServiceFramework<IProcessWindingController> {
public:
    explicit IProcessWindingController();
    ~IProcessWindingController() override;
};

/// Registers all AM services with the service manager.
void InstallInterfaces();

} // namespace Service::AM
