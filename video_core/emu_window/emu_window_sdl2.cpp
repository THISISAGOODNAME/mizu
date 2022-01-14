// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <SDL.h>

#include "common/logging/log.h"
#include "common/scm_rev.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/perf_stats.h"
#include "input_common/keyboard.h"
#include "input_common/main.h"
#include "input_common/mouse/mouse_input.h"
#include "input_common/sdl/sdl.h"
#include "video_core/emu_window/emu_window_sdl2.h"

EmuWindow_SDL2::EmuWindow_SDL2(Tegra::GPU& gpu_)
    : gpu{gpu_} {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        LOG_CRITICAL(Frontend, "Failed to initialize SDL2: %s", SDL_GetError());
    }
    input_subsystem.Initialize();
    SDL_SetMainReady();
    event_thread = std::jthread([this](std::stop_token stop_token) {
        while (!stop_token.stop_requested()) {
            WaitEvent();
        }
    });
}

EmuWindow_SDL2::~EmuWindow_SDL2() {
    input_subsystem.Shutdown();
    SDL_Quit();
}

void EmuWindow_SDL2::OnMouseMotion(s32 x, s32 y) {
    TouchMoved((unsigned)std::max(x, 0), (unsigned)std::max(y, 0), 0);

    input_subsystem.GetMouse()->MouseMove(x, y, 0, 0);
}

MouseInput::MouseButton EmuWindow_SDL2::SDLButtonToMouseButton(u32 button) const {
    switch (button) {
    case SDL_BUTTON_LEFT:
        return MouseInput::MouseButton::Left;
    case SDL_BUTTON_RIGHT:
        return MouseInput::MouseButton::Right;
    case SDL_BUTTON_MIDDLE:
        return MouseInput::MouseButton::Wheel;
    case SDL_BUTTON_X1:
        return MouseInput::MouseButton::Backward;
    case SDL_BUTTON_X2:
        return MouseInput::MouseButton::Forward;
    default:
        return MouseInput::MouseButton::Undefined;
    }
}

void EmuWindow_SDL2::OnMouseButton(u32 button, u8 state, s32 x, s32 y) {
    const auto mouse_button = SDLButtonToMouseButton(button);
    if (button == SDL_BUTTON_LEFT) {
        if (state == SDL_PRESSED) {
            TouchPressed((unsigned)std::max(x, 0), (unsigned)std::max(y, 0), 0);
        } else {
            TouchReleased(0);
        }
    } else {
        if (state == SDL_PRESSED) {
            input_subsystem.GetMouse()->PressButton(x, y, mouse_button);
        } else {
            input_subsystem.GetMouse()->ReleaseButton(mouse_button);
        }
    }
}

std::pair<unsigned, unsigned> EmuWindow_SDL2::TouchToPixelPos(float touch_x, float touch_y) const {
    int w, h;
    SDL_GetWindowSize(render_window, &w, &h);

    touch_x *= w;
    touch_y *= h;

    return {static_cast<unsigned>(std::max(std::round(touch_x), 0.0f)),
            static_cast<unsigned>(std::max(std::round(touch_y), 0.0f))};
}

void EmuWindow_SDL2::OnFingerDown(float x, float y) {
    // TODO(NeatNit): keep track of multitouch using the fingerID and a dictionary of some kind
    // This isn't critical because the best we can do when we have that is to average them, like the
    // 3DS does

    const auto [px, py] = TouchToPixelPos(x, y);
    TouchPressed(px, py, 0);
}

void EmuWindow_SDL2::OnFingerMotion(float x, float y) {
    const auto [px, py] = TouchToPixelPos(x, y);
    TouchMoved(px, py, 0);
}

void EmuWindow_SDL2::OnFingerUp() {
    TouchReleased(0);
}

void EmuWindow_SDL2::OnKeyEvent(int key, u8 state) {
    if (state == SDL_PRESSED) {
        input_subsystem.GetKeyboard()->PressKey(key);
    } else if (state == SDL_RELEASED) {
        input_subsystem.GetKeyboard()->ReleaseKey(key);
    }
}

bool EmuWindow_SDL2::IsShown() const {
    return is_shown;
}

void EmuWindow_SDL2::OnResize() {
    int width, height;
    SDL_GetWindowSize(render_window, &width, &height);
    UpdateCurrentFramebufferLayout(width, height);
}

void EmuWindow_SDL2::ShowCursor(bool show_cursor) {
    SDL_ShowCursor(show_cursor ? SDL_ENABLE : SDL_DISABLE);
}

void EmuWindow_SDL2::Fullscreen() {
    switch (Settings::values.fullscreen_mode.GetValue()) {
    case Settings::FullscreenMode::Exclusive:
        // Set window size to render size before entering fullscreen -- SDL does not resize to
        // display dimensions in this mode.
        // TODO: Multiply the window size by resolution_factor (for both docked modes)
        if (Settings::values.use_docked_mode) {
            SDL_SetWindowSize(render_window, Layout::ScreenDocked::Width,
                              Layout::ScreenDocked::Height);
        }

        if (SDL_SetWindowFullscreen(render_window, SDL_WINDOW_FULLSCREEN) == 0) {
            return;
        }

        LOG_ERROR(Frontend, "Fullscreening failed: {}", SDL_GetError());
        LOG_INFO(Frontend, "Attempting to use borderless fullscreen...");
        [[fallthrough]];
    case Settings::FullscreenMode::Borderless:
        if (SDL_SetWindowFullscreen(render_window, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0) {
            return;
        }

        LOG_ERROR(Frontend, "Borderless fullscreening failed: {}", SDL_GetError());
        [[fallthrough]];
    default:
        // Fallback algorithm: Maximise window.
        // Works on all systems (unless something is seriously wrong), so no fallback for this one.
        LOG_INFO(Frontend, "Falling back on a maximised window...");
        SDL_MaximizeWindow(render_window);
        break;
    }
}

void EmuWindow_SDL2::WaitEvent() {
    // Called on main thread
    SDL_Event event;

    if (!SDL_WaitEvent(&event)) {
        LOG_CRITICAL(Frontend, "SDL_WaitEvent failed: {}", SDL_GetError());
        return;
    }

    switch (event.type) {
    case SDL_WINDOWEVENT:
        switch (event.window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_MAXIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
            OnResize();
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
        case SDL_WINDOWEVENT_EXPOSED:
            is_shown = event.window.event == SDL_WINDOWEVENT_EXPOSED;
            OnResize();
            break;
        case SDL_WINDOWEVENT_CLOSE:
            LOG_INFO(Frontend, "window close requested, not yet implemented");
            break;
        }
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        OnKeyEvent(static_cast<int>(event.key.keysym.scancode), event.key.state);
        break;
    case SDL_MOUSEMOTION:
        // ignore if it came from touch
        if (event.button.which != SDL_TOUCH_MOUSEID)
            OnMouseMotion(event.motion.x, event.motion.y);
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        // ignore if it came from touch
        if (event.button.which != SDL_TOUCH_MOUSEID) {
            OnMouseButton(event.button.button, event.button.state, event.button.x, event.button.y);
        }
        break;
    case SDL_FINGERDOWN:
        OnFingerDown(event.tfinger.x, event.tfinger.y);
        break;
    case SDL_FINGERMOTION:
        OnFingerMotion(event.tfinger.x, event.tfinger.y);
        break;
    case SDL_FINGERUP:
        OnFingerUp();
        break;
    case SDL_QUIT:
        LOG_INFO(Frontend, "SDL quit requested, not yet implemented");
        break;
    default:
        break;
    }

    const u32 current_time = SDL_GetTicks();
    if (current_time > last_time + 2000) {
        const auto results = gpu.GetPerfStats().GetAndResetStats(Service::GetGlobalTimeUs());
        const auto title =
            fmt::format("Horizon renderer | FPS: {:.0f} ({:.0f}%)", results.average_game_fps,
                        results.emulation_speed * 100.0);
        SDL_SetWindowTitle(render_window, title.c_str());
        last_time = current_time;
    }
}

// Credits to Samantas5855 and others for this function.
void EmuWindow_SDL2::SetWindowIcon() {
    /* SDL_RWops* const yuzu_icon_stream = SDL_RWFromConstMem((void*)yuzu_icon, yuzu_icon_size); */
    /* if (yuzu_icon_stream == nullptr) { */
    /*     LOG_WARNING(Frontend, "Failed to create yuzu icon stream."); */
    /*     return; */
    /* } */
    /* SDL_Surface* const window_icon = SDL_LoadBMP_RW(yuzu_icon_stream, 1); */
    /* if (window_icon == nullptr) { */
    /*     LOG_WARNING(Frontend, "Failed to read BMP from stream."); */
    /*     return; */
    /* } */
    /* // The icon is attached to the window pointer */
    /* SDL_SetWindowIcon(render_window, window_icon); */
    /* SDL_FreeSurface(window_icon); */
    LOG_INFO(Frontend, "mizu TODO SetWindowIcon");
}

void EmuWindow_SDL2::OnMinimalClientAreaChangeRequest(std::pair<u32, u32> minimal_size) {
    SDL_SetWindowMinimumSize(render_window, minimal_size.first, minimal_size.second);
}