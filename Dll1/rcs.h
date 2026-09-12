#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <cstdint>
#include <thread>
#include <atomic>
#include <chrono>
#include "patterns.h"
#include "settings.h"

#pragma comment(lib, "winmm.lib")

// 1. Semakan kursor game (elak mouse tertarik masa buka Buy Menu / ESC)
inline bool is_game_cursor_visible() {
    static auto last_check = std::chrono::steady_clock::now();
    static bool cached_state = false;

    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_check).count() > 50) {
        CURSORINFO ci = { sizeof(CURSORINFO) };
        if (GetCursorInfo(&ci)) {
            cached_state = (ci.flags & CURSOR_SHOWING) != 0;
        }
        last_check = now;
    }
    return cached_state;
}

// 2. Tapis HANYA senjata automatik (Abaikan Smoke, Flash, Bom C4, Pisau & Sniper)
inline bool is_sprayable_weapon(uint16_t id) {
    switch (id) {
    case 7:  // AK-47
    case 8:  // AUG
    case 10: // FAMAS
    case 13: // Galil AR
    case 16: // M4A4
    case 39: // SG 553
    case 60: // M4A1-S
    case 17: // MAC-10
    case 19: // P90
    case 23: // MP5-SD
    case 24: // UMP-45
    case 26: // PP-Bizon
    case 33: // MP7
    case 34: // MP9
    case 14: // M249
    case 28: // Negev
    case 63: // CZ75-Auto
        return true;
    default:
        return false;
    }
}

class HighPrecisionTimer {
private:
    LARGE_INTEGER m_freq{};

public:
    HighPrecisionTimer() {
        QueryPerformanceFrequency(&m_freq);
        timeBeginPeriod(1);
    }

    ~HighPrecisionTimer() {
        timeEndPeriod(1);
    }

    double get_time_ms() const {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return (static_cast<double>(counter.QuadPart) / static_cast<double>(m_freq.QuadPart)) * 1000.0;
    }

    void sleep_until(double target_ms, double begin_ms) const {
        double target_abs = begin_ms + target_ms;

        while (true) {
            double current = get_time_ms();
            double remaining = target_abs - current;

            if (remaining <= 0.0) break;

            if (remaining > 15.0) {
                Sleep(static_cast<DWORD>(remaining - 10.0));
            }
            else if (remaining > 2.0) {
                Sleep(1);
            }
            else {
                YieldProcessor();
            }
        }
    }
};

class RecoilControlSystem {
private:
    HighPrecisionTimer m_timer;
    std::atomic<bool> m_running{ false };
    std::thread m_thread;

    std::atomic<bool> m_aimbot_locked{ false };
    std::atomic<bool> m_target_visible{ false }; // Status musuh nampak
    std::atomic<uint16_t> m_weapon_id{ 0 };
    std::atomic<float> m_sensitivity{ 1.0f };

    std::atomic<float> m_recoil_pitch{ 0.0f };
    std::atomic<float> m_recoil_yaw{ 0.0f };
    std::atomic<int> m_current_bullet{ 0 };

    float m_recoil_scale_x = 1.0f;
    float m_recoil_scale_y = 1.0f;

    void compensation_sequence(uint16_t weapon_def_index, float sensitivity) {
        try {
            const auto* pattern = get_weapon_pattern(weapon_def_index);
            if (!pattern || pattern->empty()) return;

            double begin_time = m_timer.get_time_ms();
            double accumulated_time = 0.0;
            float sum_x = 0.0f;
            float sum_y = 0.0f;
            float accumulated_angle_pitch = 0.0f;
            float accumulated_angle_yaw = 0.0f;
            float sens = (sensitivity > 0.01f) ? sensitivity : 1.0f;

            size_t max_bullets = pattern->size();

            for (size_t i = 0; i < max_bullets; ++i) {
                // Henti serta-merta jika menu terbuka, kursor aktif, Mouse1 dilepas, atau musuh hilang
                if (g_settings.menu_open || is_game_cursor_visible() ||
                    !(GetAsyncKeyState(VK_LBUTTON) & 0x8000) ||
                    !m_running.load(std::memory_order_relaxed) ||
                    !m_target_visible.load(std::memory_order_relaxed)) {
                    break;
                }

                m_current_bullet.store(static_cast<int>(i), std::memory_order_relaxed);

                const auto& point = (*pattern)[i];
                float delay = (point.delay > 0.0f) ? point.delay : 99.0f;

                if (i == 0 || (point.dx == 0.0f && point.dy == 0.0f)) {
                    accumulated_time += delay;
                    m_timer.sleep_until(accumulated_time, begin_time);
                    continue;
                }

                float total_dx = (point.dx * m_recoil_scale_x) / sens;
                float total_dy = (-point.dy * m_recoil_scale_y) / sens;

                double bullet_start_time = m_timer.get_time_ms();
                float moved_x = 0.0f;
                float moved_y = 0.0f;

                while (true) {
                    // 1. Henti HANYA jika menu buka, kursor aktif, atau Mouse 1 dilepas
                    if (g_settings.menu_open || is_game_cursor_visible() ||
                        !(GetAsyncKeyState(VK_LBUTTON) & 0x8000) ||
                        !m_running.load(std::memory_order_relaxed)) {
                        break;
                    }

                    double now = m_timer.get_time_ms();
                    double elapsed = now - bullet_start_time;
                    float progress = static_cast<float>(elapsed / delay);
                    if (progress > 1.0f) progress = 1.0f;

                    float target_now_x = total_dx * progress;
                    float target_now_y = total_dy * progress;

                    float delta_x = target_now_x - moved_x;
                    float delta_y = target_now_y - moved_y;

                    sum_x += delta_x;
                    sum_y += delta_y;

                    // 2. Simpan nilai sudut untuk dibaca oleh aimbot.h bila ada musuh
                    accumulated_angle_pitch += delta_y * (0.022f * sens);
                    accumulated_angle_yaw += delta_x * (0.022f * sens);

                    m_recoil_pitch.store(accumulated_angle_pitch, std::memory_order_relaxed);
                    m_recoil_yaw.store(accumulated_angle_yaw, std::memory_order_relaxed);

                    moved_x = target_now_x;
                    moved_y = target_now_y;

                    int move_x = static_cast<int>(sum_x);
                    int move_y = static_cast<int>(sum_y);

                    sum_x -= static_cast<float>(move_x);
                    sum_y -= static_cast<float>(move_y);

                    // 3. JIKA TIADA MUSUH (SPRAY DINDING KOSONG):
                    // Tarik tetikus terus guna SendInput di sini
                    if (!m_aimbot_locked.load(std::memory_order_relaxed)) {
                        if (move_x != 0 || move_y != 0) {
                            INPUT input = { 0 };
                            input.type = INPUT_MOUSE;
                            input.mi.dwFlags = MOUSEEVENTF_MOVE;
                            input.mi.dx = move_x;
                            input.mi.dy = move_y;
                            SendInput(1, &input, sizeof(INPUT));
                        }
                    }

                    if (progress >= 1.0f) break;
                    Sleep(2); // Sleep 2ms beri rehat pada CPU supaya FPS tak drop
                }

                accumulated_time += delay;
                m_timer.sleep_until(accumulated_time, begin_time);
            }
        }
        catch (...) {
        }

        m_current_bullet.store(0, std::memory_order_relaxed);
        m_recoil_pitch.store(0.0f, std::memory_order_relaxed);
        m_recoil_yaw.store(0.0f, std::memory_order_relaxed);

        // Jimatkan CPU: Sleep(10) semasa menanti Mouse1 dilepaskan (bukan Sleep(1))
        while ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && m_running.load(std::memory_order_relaxed)) {
            Sleep(10);
        }
    }

    void thread_loop() {
        while (m_running.load(std::memory_order_relaxed)) {
            try {
                if (g_settings.menu_open || !g_settings.master_switch || is_game_cursor_visible()) {
                    Sleep(20);
                    continue;
                }

                // HANYA jalan jika Mouse 1 ditekan DAN musuh sedang nampak
                if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && m_target_visible.load(std::memory_order_relaxed)) {
                    uint16_t weapon_id = m_weapon_id.load(std::memory_order_relaxed);
                    float sens = m_sensitivity.load(std::memory_order_relaxed);

                    // Tapis: Senjata automatik sahaja
                    if (is_sprayable_weapon(weapon_id)) {
                        compensation_sequence(weapon_id, sens);
                    }
                }
            }
            catch (...) {
            }
            // Jimatkan 75% kitaran CPU thread semasa idle: 4ms sleep memberi tindak balas segera tanpa choke CPU
            Sleep(4);
        }
    }

public:
    void start() {
        if (!m_running.load()) {
            m_running.store(true);
            m_thread = std::thread(&RecoilControlSystem::thread_loop, this);
            m_thread.detach();
        }
    }

    int get_current_bullet() const {
        return m_current_bullet.load(std::memory_order_relaxed);
    }

    void stop() {
        m_running.store(false);
    }

    void set_aimbot_locked(bool locked) {
        m_aimbot_locked.store(locked, std::memory_order_relaxed);
    }

    void set_target_visible(bool visible) {
        m_target_visible.store(visible, std::memory_order_relaxed);
    }

    void update_weapon_state(uint16_t weapon_id, float sens) {
        m_weapon_id.store(weapon_id, std::memory_order_relaxed);
        m_sensitivity.store(sens, std::memory_order_relaxed);
    }

    float get_recoil_pitch() const {
        return m_recoil_pitch.load(std::memory_order_relaxed);
    }

    float get_recoil_yaw() const {
        return m_recoil_yaw.load(std::memory_order_relaxed);
    }

    void set_scales(float scale_x, float scale_y) {
        m_recoil_scale_x = scale_x;
        m_recoil_scale_y = scale_y;
    }
};

inline RecoilControlSystem g_rcs;