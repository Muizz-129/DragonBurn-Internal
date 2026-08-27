#pragma once
#include <Windows.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>

#include "types.h"
#include "settings.h"
#include "utils.h"
#include "bvh.h"
#include "aimbot_math.h"

struct AimbotTarget {
    bool valid = false;
    int health = 0;
    int team = 0;
    uint64_t bSpottedByMask = 0;
    Vec3 head_pos{};
    Vec3 neck_pos{};
    Vec3 chest_pos{};
    Vec3 pelvis_pos{};
};

struct AimbotFrame {
    using Target = AimbotTarget;
    uintptr_t local_pawn = 0;
    int local_team = 0;
    int local_player_index = -1;
    int screen_w = 0;
    int screen_h = 0;
    bool camera_valid = false;
    float camera_fov = 90.0f;
    Vec3 eye_origin{};
    Vec3 view_angles{};
    bool is_scoped = false;
    uint16_t local_weapon_def_index = 0;
    Target targets[64]{};
};

class AimbotSharedData {
public:
    void publish(const AimbotFrame& frame) {
        int write_idx = m_read_index.load(std::memory_order_relaxed) ^ 1;
        m_frames[write_idx] = frame;
        m_read_index.store(write_idx, std::memory_order_release);
    }

    AimbotFrame snapshot() const {
        int idx = m_read_index.load(std::memory_order_acquire);
        return m_frames[idx];
    }

private:
    AimbotFrame m_frames[2]{};
    std::atomic<int> m_read_index{ 0 };
};

inline AimbotSharedData g_aimbot_data;
inline std::atomic<bool> g_aimbot_running{ false };

inline float aim_error_x = 0.0f;
inline float aim_error_y = 0.0f;

static inline bool is_holding_non_gun(uint16_t w_id) {
    if (w_id == 0) return false;
    if (w_id == 31 || w_id == 41 || w_id == 42 || w_id == 59 || w_id == 524) return true; // Knife/Zeus
    if (w_id >= 43 && w_id <= 48) return true; // Grenades
    if (w_id == 49) return true;               // C4
    if (w_id >= 500 && w_id <= 530) return true; // Custom Knives
    return false;
}

static inline bool check_target_visible(const Vec3& eye_pos, const Vec3& target_pos, const AimbotTarget& target, int local_player_index) {
    if (g_bvh.valid()) {
        const auto trace = g_bvh.trace_ray(eye_pos, target_pos);
        return (!trace.hit || trace.fraction > 0.97f);
    }

    if (local_player_index >= 0 && local_player_index < 64 && target.bSpottedByMask != 0) {
        return (target.bSpottedByMask & (1ULL << local_player_index)) != 0;
    }

    return true;
}

static inline void aimbot_tick() {
    if (!g_settings.master_switch || !g_settings.aimbot_enabled || g_settings.menu_open) {
        aim_error_x = aim_error_y = 0.0f;
        return;
    }

    int key = g_settings.key_aimbot ? g_settings.key_aimbot : VK_XBUTTON1;
    if (!(GetAsyncKeyState(key) & 0x8000)) {
        aim_error_x = aim_error_y = 0.0f;
        return;
    }

    AimbotFrame frame = g_aimbot_data.snapshot();
    if (frame.local_pawn == 0 || frame.screen_w == 0) return;
    if (is_holding_non_gun(frame.local_weapon_def_index)) return;

    Vec3 eye_pos = (frame.camera_valid && frame.eye_origin.length_sqr() > 1.0f)
        ? frame.eye_origin
        : Vec3{ frame.eye_origin.x, frame.eye_origin.y, frame.eye_origin.z + 64.0f };

    AimAngles view_angles{ frame.view_angles.x, frame.view_angles.y };

    static const Vec3 AimbotFrame::Target::* bone_list[] = {
        &AimbotFrame::Target::head_pos,
        &AimbotFrame::Target::neck_pos,
        &AimbotFrame::Target::chest_pos,
        &AimbotFrame::Target::pelvis_pos,
    };

    int bone_idx = std::clamp(g_settings.aimbot_bone, 0, 3);
    float best_fov = static_cast<float>(g_settings.aimbot_fov);
    bool found = false;
    Vec3 best_aim_point{};

    for (int i = 1; i < 64; i++) {
        const auto& t = frame.targets[i];
        if (!t.valid || t.health <= 0) continue;
        if (t.team == frame.local_team && g_settings.aimbot_team_check) continue;

        Vec3 bone_pos = t.*(bone_list[bone_idx]);
        if (bone_pos.length_sqr() < 1.0f) bone_pos = t.chest_pos;
        if (bone_pos.length_sqr() < 1.0f) continue;

        AimAngles desired = calculate_angle(eye_pos, bone_pos);
        float fov = get_fov_between(view_angles, desired);

        if (fov > best_fov) continue;

        if (g_settings.aimbot_visible_check && !check_target_visible(eye_pos, bone_pos, t, frame.local_player_index)) {
            continue;
        }

        best_fov = fov;
        best_aim_point = bone_pos;
        found = true;
    }

    if (found) {
        AimAngles desired = calculate_angle(eye_pos, best_aim_point);

        float delta_pitch = desired.pitch - view_angles.pitch;
        float delta_yaw = normalize_yaw(desired.yaw - view_angles.yaw);

        float smooth = (g_settings.aimbot_smooth >= 1.0f) ? g_settings.aimbot_smooth : 1.0f;
        delta_pitch /= smooth;
        delta_yaw /= smooth;

        float sens = (g_settings.aimbot_sensitivity > 0.01f) ? g_settings.aimbot_sensitivity : 1.0f;
        constexpr float m_yaw = 0.022f;

        float move_x = -delta_yaw / (m_yaw * sens);
        float move_y = delta_pitch / (m_yaw * sens);

        aim_error_x += move_x;
        aim_error_y += move_y;

        int dx = static_cast<int>(aim_error_x);
        int dy = static_cast<int>(aim_error_y);

        aim_error_x -= static_cast<float>(dx);
        aim_error_y -= static_cast<float>(dy);

        if (dx != 0 || dy != 0) {
            INPUT input = { 0 };
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = dx;
            input.mi.dy = dy;
            SendInput(1, &input, sizeof(INPUT));
        }
    }
    else {
        aim_error_x = aim_error_y = 0.0f;
    }
}

static inline void aimbot_thread_func() {
    constexpr double target_tick_fps = 128.0;
    constexpr double frame_time = 1000.0 / target_tick_fps;

    while (g_aimbot_running.load(std::memory_order_relaxed)) {
        auto tick_start = std::chrono::high_resolution_clock::now();

        aimbot_tick();

        auto tick_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = tick_end - tick_start;

        if (elapsed.count() < frame_time) {
            std::this_thread::sleep_for(
                std::chrono::duration<double, std::milli>(frame_time - elapsed.count())
            );
        }
    }
}

inline void start_aimbot_thread() {
    static bool started = false;
    if (!started) {
        g_aimbot_running.store(true, std::memory_order_relaxed);
        std::thread(aimbot_thread_func).detach();
        started = true;
    }
}

inline void stop_aimbot_thread() {
    g_aimbot_running.store(false, std::memory_order_relaxed);
}