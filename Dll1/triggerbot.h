#pragma once
#include <Windows.h>
#include <chrono>
#include <thread>
#include <atomic>

#include "settings.h"
#include "types.h"
#include "utils.h"
#include "aimbot.h"
#include "offsets.h"
#include "entity_reader.h"
#include "entity_utils.h"

inline std::atomic<bool> g_triggerbot_running{ false };

static inline void triggerbot_tick() {
    // 1. Tapis menu & suis utama
    if (!g_settings.master_switch || !g_settings.triggerbot_enabled || g_settings.menu_open) {
        return;
    }

    // 2. Semak syarat Always Active atau Hotkey
    if (!g_settings.triggerbot_always_on) {
        int key = g_settings.key_triggerbot ? g_settings.key_triggerbot : 'X';
        if (!(GetAsyncKeyState(key) & 0x8000)) {
            return;
        }
    }

    AimbotFrame frame = g_aimbot_data.snapshot();
    if (frame.local_pawn == 0 || is_holding_non_gun(frame.local_weapon_def_index)) {
        return;
    }

    // 3. Tapis mod Scoped Only (cth: AWP / Scout)
    if (g_settings.triggerbot_scoped_only && !frame.is_scoped) {
        return;
    }

    // 4. Baca ID entiti di bawah crosshair
    int crosshair_id = read_mem<int>(frame.local_pawn + g_offsets.C_CSPlayerPawn.m_iIDEntIndex);
    if (crosshair_id <= 0) return;

    uintptr_t client_base = Offsets::get_client_base();
    if (!client_base) return;

    uintptr_t entity_list = read_mem<uintptr_t>(client_base + g_offsets.client.dwEntityList);
    if (!entity_list) return;

    // 5. Dapatkan pointer entiti musuh menggunakan resolve_handle
    uintptr_t target_pawn = EntityList::resolve_handle(entity_list, static_cast<uint32_t>(crosshair_id));
    if (!target_pawn || target_pawn == frame.local_pawn) return;

    // 6. Semak kesihatan & pasukan sasaran
    int target_health = read_mem<int>(target_pawn + g_offsets.C_BaseEntity.m_iHealth);
    int target_team = read_mem<int>(target_pawn + g_offsets.C_BaseEntity.m_iTeamNum);

    if (target_health <= 0 || target_health > 100) return;
    if (target_team == frame.local_team) return;

    // 7. Kelewatan reaksi (Legit Delay)
    float delay = g_settings.triggerbot_delay;
    if (delay > 0.0f) {
        Sleep(static_cast<DWORD>(delay));
    }

    // 8. Tembak
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    Sleep(20);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

    // Cooldown mengelakkan tembakan bertindih
    Sleep(120);
}

static inline void triggerbot_thread_func() {
    while (g_triggerbot_running.load(std::memory_order_relaxed)) {
        triggerbot_tick();
        Sleep(2);
    }
}

inline void start_triggerbot_thread() {
    static bool started = false;
    if (!started) {
        g_triggerbot_running.store(true, std::memory_order_relaxed);
        std::thread(triggerbot_thread_func).detach();
        started = true;
    }
}

inline void stop_triggerbot_thread() {
    g_triggerbot_running.store(false, std::memory_order_relaxed);
}