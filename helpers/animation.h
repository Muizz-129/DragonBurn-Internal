#pragma once

#include "../imgui/imgui_internal.h"

namespace anim
{
    inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

    inline ImVec4 lerp_color(ImVec4 a, ImVec4 b, float t)
    {
        return ImVec4(lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t), lerp(a.w, b.w, t));
    }

    inline void ease(float& value, float target, float dt, float speed = 16.f)
    {
        const float step = speed * dt;
        if (value < target)
            value = ImMin(value + step, target);
        else if (value > target)
            value = ImMax(value - step, target);
    }

    inline float smoothstep(float t)
    {
        t = ImClamp(t, 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }

    inline float fixed_speed(float speed)
    {
        return speed / ImMax(ImGui::GetIO().Framerate, 1.f);
    }

    inline void static_ease(float& value, float target, float speed)
    {
        const float step = fixed_speed(speed);
        if (value < target)
            value = ImMin(value + step, target);
        else if (value > target)
            value = ImMax(value - step, target);
    }

    inline void dynamic_ease(float& value, float target, float speed)
    {
        value = ImLerp(value, target, fixed_speed(speed));
    }

    inline void dynamic_ease(ImVec4& value, const ImVec4& target, float speed)
    {
        value = ImLerp(value, target, fixed_speed(speed));
    }
}
