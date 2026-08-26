#pragma once
#include <Windows.h>
#include <atomic>
#include <cstdio>
#include <thread>
#include <chrono>
#include <cmath>
#include <cfloat>
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

inline std::thread g_aimbot_thread;
inline std::atomic<bool> g_aimbot_running{ false };

inline float aim_error_x = 0.0f;
inline float aim_error_y = 0.0f;

inline bool  g_trigger_waiting = false;
inline bool  g_trigger_held = false;
inline float g_trigger_delay_end = 0.0f;
inline float g_trigger_release_time = 0.0f;

static inline bool is_holding_non_gun(uint16_t w_id) {
    if (w_id == 0) return true;                               
    if (w_id == 31) return true;                               
    if (w_id == 41 || w_id == 42 || w_id == 59 || w_id == 524) return true; 
    if (w_id >= 43 && w_id <= 48) return true;                
    if (w_id == 49) return true;                              
    if (w_id >= 500 && w_id <= 530) return true;              
    return false;
}

static inline bool check_target_visible(const Vec3& eye_pos, const Vec3& target_pos, const AimbotTarget& target, int local_player_index) {
    if (g_bvh.valid()) {
        const auto trace = g_bvh.trace_ray(eye_pos, target_pos);
        return (!trace.hit || trace.fraction > 0.97f);
    }

    if (local_player_index >= 0 && local_player_index < 64) {
        return (target.bSpottedByMask & (1ULL << local_player_index)) != 0;
    }

    return true;
}

struct TriggerbotResult {
    bool found = false;
    float distance_sq = FLT_MAX;
    Vec3  hit_pos{};
};

static inline TriggerbotResult triggerbot_trace_crosshair(
    const Vec3& eye_pos,
    const AimAngles& view_angles,
    const AimbotFrame& frame)
{
    TriggerbotResult result{};

    float pitch_rad = view_angles.pitch * DEG2RAD;
    float yaw_rad = view_angles.yaw * DEG2RAD;

    Vec3 forward;
    forward.x = cosf(pitch_rad) * cosf(yaw_rad);
    forward.y = cosf(pitch_rad) * sinf(yaw_rad);
    forward.z = -sinf(pitch_rad);
    forward = forward.normalized();

    constexpr auto MAX_RANGE = 8192.0f;

    struct BoneInfo {
        Vec3 AimbotFrame::Target::* bone_ptr;
        float radius;
    };

    BoneInfo hitboxes_to_check[] = {
        { &AimbotFrame::Target::head_pos,   4.5f },
        { &AimbotFrame::Target::neck_pos,   4.0f },
        { &AimbotFrame::Target::chest_pos,  6.5f },
        { &AimbotFrame::Target::pelvis_pos, 5.5f },
    };
    int num_hitboxes = 4;

    for (int i = 1; i < 64; i++)
    {
        const auto& t = frame.targets[i];
        if (!t.valid || t.health <= 0) continue;
        if (t.team == frame.local_team && g_settings.aimbot_team_check) continue;

        for (int hb_idx = 0; hb_idx < num_hitboxes; hb_idx++)
        {
            const auto& current_hb_info = hitboxes_to_check[hb_idx];
            Vec3 bone_center = t.*(current_hb_info.bone_ptr);
            float radius = current_hb_info.radius;

            Vec3 oc = eye_pos - bone_center;
            float a = forward.dot(forward);
            float b = 2.0f * oc.dot(forward);
            float c = oc.dot(oc) - radius * radius;
            float discriminant = b * b - 4.0f * a * c;

            if (discriminant < 0.0f) continue;

            float sqrt_d = std::sqrtf(discriminant);
            float t_val = (-b - sqrt_d) / (2.0f * a);
            if (t_val < 0.0f) t_val = (-b + sqrt_d) / (2.0f * a);
            if (t_val < 0.0f || t_val > MAX_RANGE) continue;

            Vec3 hit_pos = eye_pos + forward * t_val;
            float dist_sq = (hit_pos - eye_pos).length_sqr();
            if (dist_sq >= result.distance_sq) continue;

            bool visible = check_target_visible(eye_pos, bone_center, t, frame.local_player_index);

            if (visible)
            {
                result.distance_sq = dist_sq;
                result.hit_pos = hit_pos;
                result.found = true;
            }
        }
    }
    return result;
}

static inline float get_time_seconds()
{
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float>(now - start).count();
}

static inline void triggerbot_tick()
{

    if (!g_settings.master_switch || !g_settings.triggerbot_enabled || g_settings.menu_open)
    {
        g_trigger_waiting = false;
        g_trigger_held = false;
        return;
    }

    float now = get_time_seconds();

    if (g_trigger_held)
    {
        if (now >= g_trigger_release_time)
        {
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            g_trigger_held = false;
        }
        return;
    }

    AimbotFrame frame = g_aimbot_data.snapshot();

    if (frame.local_pawn == 0 || frame.screen_w == 0 || !frame.camera_valid) return;

    // Sekat tembakan jika sedang pegang pisau/bom
    if (is_holding_non_gun(frame.local_weapon_def_index)) {
        g_trigger_waiting = false;
        return;
    }

    bool key_down = (g_settings.key_triggerbot > 0) && (GetAsyncKeyState(g_settings.key_triggerbot) & 0x8000);
    if (!key_down && !g_settings.triggerbot_always_on)
    {
        g_trigger_waiting = false;
        return;
    }

    if (g_settings.triggerbot_scoped_only && !frame.is_scoped)
    {
        auto w = frame.local_weapon_def_index;
        if (w == 9 || w == 40)
        {
            g_trigger_waiting = false;
            return;
        }
    }

    Vec3 eye_pos = frame.eye_origin;
    AimAngles view_angles{ frame.view_angles.x, frame.view_angles.y };

    TriggerbotResult trace_result = triggerbot_trace_crosshair(eye_pos, view_angles, frame);

    if (!trace_result.found)
    {
        g_trigger_waiting = false;
        return;
    }

    if (g_settings.triggerbot_delay <= 0.0f)
    {
        g_trigger_waiting = false;
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        return;
    }

    if (!g_trigger_waiting)
    {
        g_trigger_waiting = true;
        g_trigger_delay_end = now + (g_settings.triggerbot_delay * 0.001f);
        return;
    }

    if (now < g_trigger_delay_end)
        return;

    g_trigger_waiting = false;
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    g_trigger_held = true;

    float hold_ms = 40.0f + static_cast<float>(rand() % 30);
    g_trigger_release_time = now + (hold_ms * 0.001f);
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

    if (frame.local_pawn == 0 || frame.screen_w == 0 || !frame.camera_valid) return;

    if (is_holding_non_gun(frame.local_weapon_def_index)) {
        aim_error_x = aim_error_y = 0.0f;
        return;
    }

    Vec3 eye_pos = frame.eye_origin;

    AimAngles view_angles{};
    view_angles.pitch = frame.view_angles.x;
    view_angles.yaw = frame.view_angles.y;

    static const Vec3 AimbotFrame::Target::* bone_list[] = {
        &AimbotFrame::Target::head_pos,
        &AimbotFrame::Target::neck_pos,
        &AimbotFrame::Target::chest_pos,
        &AimbotFrame::Target::pelvis_pos,
    };

    int bone_idx = std::clamp(g_settings.aimbot_bone, 0, 3);
    auto fov_limit = static_cast<float>(g_settings.aimbot_fov);

    float best_fov = fov_limit;
    bool  found = false;
    Vec3  best_aim_point{};

    for (int i = 1; i < 64; i++)
    {
        if (!g_aimbot_running.load(std::memory_order_relaxed)) return;

        const auto& t = frame.targets[i];
        if (!t.valid || t.health <= 0) continue;
        if (t.team == frame.local_team && g_settings.aimbot_team_check) continue;

        Vec3 bone_pos = t.*(bone_list[bone_idx]);
        AimAngles desired = calculate_angle(eye_pos, bone_pos);
        float fov = get_fov_between(view_angles, desired);

        if (fov > best_fov) continue;

        if (g_settings.aimbot_visible_check) {
            bool is_visible = check_target_visible(eye_pos, bone_pos, t, frame.local_player_index);
            if (!is_visible) continue;
        }

        best_fov = fov;
        best_aim_point = bone_pos;
        found = true;
    }

    if (found)
    {
        AimAngles desired = calculate_angle(eye_pos, best_aim_point);

        float delta_pitch = desired.pitch - view_angles.pitch;
        float delta_yaw = normalize_yaw(desired.yaw - view_angles.yaw);

        float smooth = (g_settings.aimbot_smooth > 1.0f) ? g_settings.aimbot_smooth : 1.0f;
        delta_pitch /= smooth;
        delta_yaw /= smooth;
        float sens = (g_settings.aimbot_sensitivity > 0.01f) ? g_settings.aimbot_sensitivity : 1.0f;
        float m_yaw = 0.022f;
        float move_x = -delta_yaw / m_yaw;
        float move_y = delta_pitch / m_yaw;

        aim_error_x += move_x;
        aim_error_y += move_y;

        int dx = static_cast<int>(aim_error_x);
        int dy = static_cast<int>(aim_error_y);

        aim_error_x -= static_cast<float>(dx);
        aim_error_y -= static_cast<float>(dy);

        if (dx != 0 || dy != 0)
        {
            mouse_event(MOUSEEVENTF_MOVE, (DWORD)dx, (DWORD)dy, 0, 0);
        }
    }
    else
    {
        aim_error_x = aim_error_y = 0.0f;
    }
}

static inline void aimbot_thread_func()
{
    static double target_tick_fps = 128.0;

    while (g_aimbot_running.load(std::memory_order_relaxed))
    {
        auto tick_start = std::chrono::high_resolution_clock::now();

        HWND fg = GetForegroundWindow();
        if (fg) {
            char title[128] = { 0 };
            GetWindowTextA(fg, title, sizeof(title));
            if (strstr(title, "Counter-Strike 2")) {
                triggerbot_tick();
                aimbot_tick();
            }
        }

        auto tick_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = tick_end - tick_start;
        double frame_time = 1000.0 / target_tick_fps;

        if (elapsed.count() < frame_time) {
            std::this_thread::sleep_for(
                std::chrono::duration<double, std::milli>(frame_time - elapsed.count())
            );
        }
    }

    if (g_trigger_held) {
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        g_trigger_held = false;
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

inline void stop_aimbot_thread()
{
    if (!g_aimbot_running.load())
        return;

    g_aimbot_running.store(false);

    if (g_aimbot_thread.joinable())
    {
        g_aimbot_thread.join();
    }
}