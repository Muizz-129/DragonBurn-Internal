#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <cstdint>
#include <thread>
#include <atomic>
#include <algorithm>
#include "patterns.h"
#include "settings.h"

#pragma comment(lib, "winmm.lib")

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
            else if (remaining > 1.5) {
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

    // Synchronization status with Aimbot
    std::atomic<bool> m_aimbot_locked{ false };
    std::atomic<uint16_t> m_weapon_id{ 0 };
    std::atomic<float> m_sensitivity{ 1.0f };

    // Recoil angle offset value to be read by the Aimbot (degrees)
    std::atomic<float> m_recoil_pitch{ 0.0f };
    std::atomic<float> m_recoil_yaw{ 0.0f };
    std::atomic<int> m_current_bullet{ 0 };

    float m_recoil_scale_x = 1.0f;
    float m_recoil_scale_y = 1.0f;

    void compensation_sequence(uint16_t weapon_def_index, float sensitivity) {
        const auto* pattern = get_weapon_pattern(weapon_def_index);
        if (!pattern || pattern->empty()) return;

        double begin_time = m_timer.get_time_ms();
        double accumulated_time = 0.0;
        float sum_x = 0.0f;
        float sum_y = 0.0f;
        float accumulated_angle_pitch = 0.0f;
        float accumulated_angle_yaw = 0.0f;
        float sens = (sensitivity > 0.01f) ? sensitivity : 1.0f;

        for (size_t i = 0; i < pattern->size(); ++i) {
            if (g_settings.menu_open || !(GetAsyncKeyState(VK_LBUTTON) & 0x8000) || !m_running.load(std::memory_order_relaxed)) {
                break;
            }

            // Store the actual bullet index: 0, 1, 2, 3...
            m_current_bullet.store(static_cast<int>(i), std::memory_order_relaxed);

            const auto& point = (*pattern)[i];
            float delay = (point.delay > 0.0f) ? point.delay : 99.0f;

            // Bullet Phase 0: Initial pause only, with no mouse movement
            if (i == 0 || (point.dx == 0.0f && point.dy == 0.0f)) {
                accumulated_time += delay;
                m_timer.sleep_until(accumulated_time, begin_time);
                continue;
            }

            // Bullet Phase 1, 2, 3... : Initial recoil push
            float total_dx = (point.dx * m_recoil_scale_x) / sens;
            float total_dy = (-point.dy * m_recoil_scale_y) / sens;

            double bullet_start_time = m_timer.get_time_ms();
            float moved_x = 0.0f;
            float moved_y = 0.0f;

            while (true) {
                if (g_settings.menu_open || !(GetAsyncKeyState(VK_LBUTTON) & 0x8000) || !m_running.load(std::memory_order_relaxed)) {
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
                Sleep(1);
            }

            accumulated_time += delay;
            m_timer.sleep_until(accumulated_time, begin_time);
        }

        // Reset to 0 when the player releases the Mouse 1 button
        m_current_bullet.store(0, std::memory_order_relaxed);
        m_recoil_pitch.store(0.0f, std::memory_order_relaxed);
        m_recoil_yaw.store(0.0f, std::memory_order_relaxed);

        while ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && m_running.load(std::memory_order_relaxed)) {
            Sleep(1);
        }
    }

    void thread_loop() {
        while (m_running.load(std::memory_order_relaxed)) {
            if (g_settings.menu_open || !g_settings.master_switch) {
                Sleep(10);
                continue;
            }
            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                uint16_t weapon_id = m_weapon_id.load(std::memory_order_relaxed);
                float sens = m_sensitivity.load(std::memory_order_relaxed);

                if (weapon_id != 0) {
                    compensation_sequence(weapon_id, sens);
                }
            }
            Sleep(1);
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

    // Communication interface with the Aimbot
    void set_aimbot_locked(bool locked) {
        m_aimbot_locked.store(locked, std::memory_order_relaxed);
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