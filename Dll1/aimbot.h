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
#include "aimbot_math.h"
#include "rcs.h"

struct AimbotTarget {
    bool valid = false;
    bool is_visible = false; // Continuously reading the status from the non-raytraced render loop
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
    if (w_id == 41 || w_id == 42 || w_id == 59 || w_id == 524) return true;
    if (w_id >= 43 && w_id <= 48) return true;
    if (w_id == 49) return true;
    if (w_id >= 500 && w_id <= 530) return true;
    return false;
}

static inline void aimbot_tick() {
    static int locked_target_idx = -1;

    if (!g_settings.master_switch || !g_settings.aimbot_enabled || g_settings.menu_open) {
        aim_error_x = aim_error_y = 0.0f;
        locked_target_idx = -1;
        g_rcs.set_aimbot_locked(false);
        g_rcs.set_target_visible(false);
        return;
    }

    int key = g_settings.key_aimbot ? g_settings.key_aimbot : VK_XBUTTON1;
    if (!(GetAsyncKeyState(key) & 0x8000)) {
        aim_error_x = aim_error_y = 0.0f;
        locked_target_idx = -1;
        g_rcs.set_aimbot_locked(false);
        g_rcs.set_target_visible(false);
        return;
    }

    AimbotFrame frame = g_aimbot_data.snapshot();
    if (frame.local_pawn == 0 || frame.screen_w == 0 || is_holding_non_gun(frame.local_weapon_def_index)) {
        aim_error_x = aim_error_y = 0.0f;
        locked_target_idx = -1;
        g_rcs.set_aimbot_locked(false);
        g_rcs.set_target_visible(false);
        return;
    }

    float sens = (g_settings.aimbot_sensitivity > 0.01f) ? g_settings.aimbot_sensitivity : 1.0f;
    g_rcs.update_weapon_state(frame.local_weapon_def_index, sens);

    Vec3 eye_pos = (frame.camera_valid && frame.eye_origin.length_sqr() > 1.0f)
        ? frame.eye_origin
        : Vec3{ frame.eye_origin.x, frame.eye_origin.y, frame.eye_origin.z + 64.0f };

    AimAngles view_angles{ frame.view_angles.x, frame.view_angles.y };
    float max_fov = (g_settings.aimbot_fov > 0.1f) ? static_cast<float>(g_settings.aimbot_fov) : 5.0f;

    Vec3 forward_dir = angle_to_forward(view_angles);
    float min_cos = cosf(max_fov * DEG2RAD);
    float min_cos_sq = min_cos * min_cos;

    float best_score = 999999.0f;
    float target_distance = 0.0f;
    bool found = false;
    Vec3 best_aim_point{};
    int best_candidate_idx = -1;

    struct BoneCandidate {
        int flag;
        Vec3 pos;
    };

    // 1. Review existing targets
    if (locked_target_idx >= 1 && locked_target_idx < 64) {
        const auto& lt = frame.targets[locked_target_idx];
        if (lt.valid && lt.health > 0 && lt.health <= 100 && (!g_settings.aimbot_team_check || lt.team != frame.local_team)) {
            const BoneCandidate candidate_bones[] = {
                { BONE_FLAG_HEAD,   lt.head_pos },
                { BONE_FLAG_NECK,   lt.neck_pos },
                { BONE_FLAG_CHEST,  lt.chest_pos },
                { BONE_FLAG_PELVIS, lt.pelvis_pos }
            };
            for (const auto& b : candidate_bones) {
                if (!(g_settings.aimbot_target_bones & b.flag)) continue;

                Vec3 diff = b.pos - eye_pos;
                float dist_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                if (dist_sq < 1.0f) continue;

                float dot = diff.x * forward_dir.x + diff.y * forward_dir.y + diff.z * forward_dir.z;
                if (dot <= 0.0f || (dot * dot < min_cos_sq * dist_sq)) continue;

                AimAngles desired = calculate_angle(eye_pos, b.pos);
                float fov = get_fov_between(view_angles, desired);
                if (fov <= max_fov) {
                    if (!g_settings.aimbot_visible_check || lt.is_visible) {
                        best_aim_point = b.pos;
                        target_distance = sqrtf(dist_sq);
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found) {
            locked_target_idx = -1;
        }
    }

    // 2. Look for a new target if none is locked on
    if (!found) {
        for (int i = 1; i < 64; i++) {
            const auto& t = frame.targets[i];
            if (!t.valid || t.health <= 0 || t.health > 100) continue;
            if (g_settings.aimbot_team_check && t.team == frame.local_team) continue;
            if (g_settings.aimbot_visible_check && !t.is_visible) continue;

            const BoneCandidate candidate_bones[] = {
                { BONE_FLAG_HEAD,   t.head_pos },
                { BONE_FLAG_NECK,   t.neck_pos },
                { BONE_FLAG_CHEST,  t.chest_pos },
                { BONE_FLAG_PELVIS, t.pelvis_pos }
            };

            for (const auto& b : candidate_bones) {
                if (!(g_settings.aimbot_target_bones & b.flag)) continue;

                Vec3 diff = b.pos - eye_pos;
                float dist_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                if (dist_sq < 1.0f) continue;

                float dot = diff.x * forward_dir.x + diff.y * forward_dir.y + diff.z * forward_dir.z;
                if (dot <= 0.0f || (dot * dot < min_cos_sq * dist_sq)) continue;

                AimAngles desired = calculate_angle(eye_pos, b.pos);
                float fov = get_fov_between(view_angles, desired);
                if (fov > max_fov) continue;

                float dist = sqrtf(dist_sq);
                float score = fov * 0.75f + (dist / 100.0f) * 0.25f;

                if (score < best_score) {
                    best_score = score;
                    best_aim_point = b.pos;
                    target_distance = dist;
                    best_candidate_idx = i;
                    found = true;
                }
            }
        }

        if (found) {
            locked_target_idx = best_candidate_idx;
        }
    }

    // 3. Directly write angles to the dwViewAngles memory (Lag-free & Crash-free)
    if (found) {
        g_rcs.set_aimbot_locked(true);
        g_rcs.set_target_visible(true);

        AimAngles desired = calculate_angle(eye_pos, best_aim_point);
        int current_bullet = g_rcs.get_current_bullet();

        if (current_bullet > 0) {
            desired.pitch += g_rcs.get_recoil_pitch();
            desired.yaw -= g_rcs.get_recoil_yaw();
        }

        // KEY 1: Pitch lock to prevent exceeding engine limits (-89 to +89)
        desired.pitch = std::clamp(desired.pitch, -89.0f, 89.0f);
        desired.yaw = normalize_yaw(desired.yaw);

        float delta_pitch = desired.pitch - view_angles.pitch;
        float delta_yaw = normalize_yaw(desired.yaw - view_angles.yaw);

        // KEY 2: Avoid a crash if a calculation results in NaN or infinity
        if (std::isnan(delta_pitch) || std::isnan(delta_yaw) ||
            std::isinf(delta_pitch) || std::isinf(delta_yaw)) {
            aim_error_x = 0.0f;
            aim_error_y = 0.0f;
            return;
        }

        float base_smooth = (g_settings.aimbot_smooth >= 1.0f) ? g_settings.aimbot_smooth : 1.0f;
        float dist_scale = std::clamp(target_distance / 800.0f, 0.45f, 2.0f);
        float dynamic_smooth = std::max(1.0f, base_smooth * dist_scale);

        delta_pitch /= dynamic_smooth;
        delta_yaw /= dynamic_smooth;

        constexpr float m_yaw = 0.022f;
        float move_x = -delta_yaw / (m_yaw * sens);
        float move_y = delta_pitch / (m_yaw * sens);

        aim_error_x += move_x;
        aim_error_y += move_y;

        // Restore the value if the accumulated error becomes corrupted
        if (std::isnan(aim_error_x) || std::isnan(aim_error_y)) {
            aim_error_x = 0.0f;
            aim_error_y = 0.0f;
            return;
        }

        int dx = static_cast<int>(aim_error_x);
        int dy = static_cast<int>(aim_error_y);

        aim_error_x -= static_cast<float>(dx);
        aim_error_y -= static_cast<float>(dy);

        // KEY 3: Limit maximum displacement per frame (Avoid lagging / input choke)
        // A maximum of 35 pixels per tick is fast enough and does not overburden Windows messages
        dx = std::clamp(dx, -35, 35);
        dy = std::clamp(dy, -35, 35);

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
        locked_target_idx = -1;
        g_rcs.set_aimbot_locked(false);
        g_rcs.set_target_visible(false);
        aim_error_x = 0.0f;
        aim_error_y = 0.0f;
    }
}

static inline void aimbot_thread_func() {
    constexpr double target_tick_fps = 144.0;
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
        else {
            // KEY: Must sleep for at least 1ms so the CPU doesn't get pegged at 100% usage
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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