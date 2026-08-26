#pragma once
#include "types.h"
#include <cmath>


constexpr float M_PI_F = 3.14159265358979323846f;
constexpr float RAD2DEG = 180.0f / M_PI_F;
constexpr float DEG2RAD = M_PI_F / 180.0f;

struct AimAngles {
    float pitch;  // x
    float yaw;    // y
};

inline AimAngles calculate_angle(const Vec3& src, const Vec3& dst)
{
    float dx = dst.x - src.x;
    float dy = dst.y - src.y;
    float dz = dst.z - src.z;

    float dist_xy = sqrtf(dx * dx + dy * dy);

    AimAngles angles;
    angles.pitch = atan2f(-dz, dist_xy) * RAD2DEG;
    angles.yaw = atan2f(dy, dx) * RAD2DEG;
    return angles;
}

inline float normalize_yaw(float yaw)
{
    while (yaw > 180.0f)  yaw -= 360.0f;
    while (yaw < -180.0f) yaw += 360.0f;
    return yaw;
}

inline float get_fov_between(const AimAngles& view, const AimAngles& target)
{
    float dp = view.pitch - target.pitch;
    float dy = normalize_yaw(view.yaw - target.yaw);
    return sqrtf(dp * dp + dy * dy);
}