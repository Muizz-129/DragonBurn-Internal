#pragma once

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_internal.h"

namespace fx
{
    // ── Core effects ──
    void glow_rect(ImDrawList* dl, ImRect rect, ImVec4 color, float strength, float rounding);
    void glow_rect_inner(ImDrawList* dl, ImRect rect, ImVec4 color, float strength, float rounding);
    void glow_circle(ImDrawList* dl, ImVec2 center, float radius, ImVec4 color, float strength);
    void soft_shadow(ImDrawList* dl, ImRect rect, float rounding, float alpha = 0.35f);

    // ── Gradients ──
    void gradient_text(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, const char* text, ImVec4 from, ImVec4 to);
    void gradient_rect_v(ImDrawList* dl, ImRect rect, ImVec4 top, ImVec4 bot, float rounding = 0.f);
    void gradient_rect_h(ImDrawList* dl, ImRect rect, ImVec4 left, ImVec4 right, float rounding = 0.f);

    // ── Decorative ──
    void accent_stripe(ImDrawList* dl, float x, float y, float h, float w, ImVec4 top, ImVec4 bot);
    void bordered_rect(ImDrawList* dl, ImRect rect, ImU32 fill, ImU32 border, float rounding, float border_w = 1.f);
    void dot_grid(ImDrawList* dl, ImRect rect, float spacing, ImU32 color, float radius = 0.5f);

    // ── Text / Icon ──
    ImVec2 snap_pos(ImVec2 pos);
    void icon_text(ImDrawList* dl, ImFont* font, float size, ImVec2 center, const char* text, ImU32 col, float rotation_deg = 0.f);
}
