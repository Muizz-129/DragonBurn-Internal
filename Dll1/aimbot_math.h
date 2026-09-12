#pragma once
#include "types.h"
#include <cmath>
#include <algorithm>

constexpr float M_PI_F = 3.14159265358979323846f;
constexpr float RAD2DEG = 180.0f / M_PI_F;
constexpr float DEG2RAD = M_PI_F / 180.0f;

struct AimAngles {
    float pitch = 0.0f;
    float yaw = 0.0f;
};

inline float clampf(float v, float lo, float hi)
{
    return std::clamp(v, lo, hi);
}

inline float normalize_yaw(float yaw)
{
    yaw = fmodf(yaw, 360.0f);
    if (yaw > 180.0f)  yaw -= 360.0f;
    if (yaw < -180.0f) yaw += 360.0f;
    return yaw;
}

inline float normalize_pitch(float pitch)
{
    return clampf(pitch, -89.0f, 89.0f);
}

// Tukar sudut Euler kepada vektor unit hadapan untuk penapis kon pantas
inline Vec3 angle_to_forward(const AimAngles& angles)
{
    float pitch_rad = angles.pitch * DEG2RAD;
    float yaw_rad = angles.yaw * DEG2RAD;
    float cp = cosf(pitch_rad);
    return Vec3{
        cp * cosf(yaw_rad),
        cp * sinf(yaw_rad),
        -sinf(pitch_rad)
    };
}

inline AimAngles calculate_angle(const Vec3& src, const Vec3& dst)
{
    float dx = dst.x - src.x;
    float dy = dst.y - src.y;
    float dz = dst.z - src.z;

    float dist_xy = sqrtf(dx * dx + dy * dy);

    AimAngles angles;
    angles.pitch = normalize_pitch(atan2f(-dz, dist_xy) * RAD2DEG);
    angles.yaw = normalize_yaw(atan2f(dy, dx) * RAD2DEG);
    return angles;
}

inline float get_fov_between(const AimAngles& view, const AimAngles& target)
{
    float dp = view.pitch - target.pitch;
    float dy = normalize_yaw(view.yaw - target.yaw);
    return sqrtf(dp * dp + dy * dy);
}