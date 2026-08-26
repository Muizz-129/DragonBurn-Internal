#pragma once
#include <Windows.h>
#include <string>
#include <chrono>
#include <thread>


#include "../imgui/imgui.h"
#pragma comment(lib, "winmm.lib")

inline std::string get_dll_directory() {
    char path[MAX_PATH] = { 0 };
    HMODULE hm = NULL;

    
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&get_dll_directory, &hm)) {
        GetModuleFileNameA(hm, path, sizeof(path));
        std::string s(path);
        size_t pos = s.find_last_of("\\/");
        if (pos != std::string::npos) {
            return s.substr(0, pos + 1);
        }
    }
    return "";
}

inline void limit_frame(std::chrono::high_resolution_clock::time_point frame_start,
    double target_fps) {
    using namespace std::chrono;

    static bool timer_initialized = false;
    if (!timer_initialized) {
        timeBeginPeriod(1);
        timer_initialized = true;
    }

    long long target_ns = static_cast<long long>(1'000'000'000.0 / target_fps);
    nanoseconds target(target_ns);

    nanoseconds elapsed = high_resolution_clock::now() - frame_start;
    nanoseconds remaining = target - elapsed;

    if (remaining > milliseconds(2)) {
        nanoseconds sleep_time = remaining - microseconds(1500);
        std::this_thread::sleep_for(sleep_time);
    }

    while (high_resolution_clock::now() - frame_start < target) {
        _mm_pause();
    }
}

inline ImU32 apply_opacity(ImU32 col, float opacity) {
    if (opacity >= 1.0f) return col;
    ImU32 a = (ImU32)(((col >> 24) & 0xFF) * opacity);
    return (col & 0x00FFFFFF) | (a << 24);
}

inline const char* vk_name(int vk) {
    switch (vk) {
    case VK_LBUTTON: return "Mouse1";
    case VK_RBUTTON: return "Mouse2";
    case VK_MBUTTON: return "Mouse3";
    case VK_XBUTTON1: return "Mouse4";
    case VK_XBUTTON2: return "Mouse5";
    case VK_LSHIFT: return "LShift";
    case VK_RSHIFT: return "RShift";
    case VK_LCONTROL: return "LCtrl";
    case VK_RCONTROL: return "RCtrl";
    case VK_LMENU: return "LAlt";
    case VK_RMENU: return "RAlt";
    case VK_CAPITAL: return "CapsLock";
    case VK_TAB: return "Tab";
    case VK_SPACE: return "Space";
    case VK_F1: return "F1";
    case VK_F2: return "F2";
    case VK_F3: return "F3";
    case VK_F4: return "F4";
    case VK_F5: return "F5";
    case VK_F6: return "F6";
    case VK_F7: return "F7";
    case VK_F8: return "F8";
    case VK_F9: return "F9";
    case VK_F10: return "F10";
    case VK_F11: return "F11";
    case VK_F12: return "F12";
    case VK_INSERT: return "Insert";
    case VK_DELETE: return "Delete";
    case VK_HOME: return "Home";
    case VK_END: return "End";
    case VK_PRIOR: return "PageUp";
    case VK_NEXT: return "PageDown";
    case VK_ESCAPE: return "Escape";
    default: {
        static char buf[32];
        UINT scan = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        if (GetKeyNameTextA(scan << 16, buf, 32)) return buf;
        snprintf(buf, 32, "Key 0x%02X", vk);
        return buf;
    }
    }
}

inline int scan_any_key(bool allow_mouse1 = true) {
    static bool prev_state[256] = {};

    // Check Escape first
    bool esc_down = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    if (esc_down && !prev_state[VK_ESCAPE]) {
        // Update all states before returning
        for (int k = 1; k < 256; ++k)
            prev_state[k] = (GetAsyncKeyState(k) & 0x8000) != 0;
        return -1;
    }

    // Check extended modifier keys FIRST (left/right variants)
    // so they aren't overshadowed by generic VK_SHIFT/VK_CONTROL/VK_MENU
    static const int extended_keys[] = {
        VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU,
        VK_LWIN, VK_RWIN
    };
    for (int k : extended_keys) {
        bool is_down = (GetAsyncKeyState(k) & 0x8000) != 0;
        bool was_down = prev_state[k];
        if (is_down && !was_down) {
            // Update all prev_state before returning
            for (int j = 1; j < 256; ++j)
                prev_state[j] = (GetAsyncKeyState(j) & 0x8000) != 0;
            return k;
        }
        prev_state[k] = is_down;
    }

    int result = 0;

    for (int k = 1; k < 256; ++k)
    {
        bool is_down = (GetAsyncKeyState(k) & 0x8000) != 0;
        bool was_down = prev_state[k];

        if (k == VK_ESCAPE) {
            prev_state[k] = is_down;
            continue;
        }

        if (!allow_mouse1 && k == VK_LBUTTON) {
            prev_state[k] = is_down;
            continue;
        }

        // Skip extended keys we already checked above
        if (k == VK_LSHIFT || k == VK_RSHIFT || k == VK_LCONTROL ||
            k == VK_RCONTROL || k == VK_LMENU || k == VK_RMENU ||
            k == VK_LWIN || k == VK_RWIN) {
            prev_state[k] = is_down;
            continue;
        }

        // Detect fresh press (edge: not down before, down now)
        if (is_down && !was_down && result == 0) {
            result = k;
        }

        prev_state[k] = is_down;
    }

    return result;
}