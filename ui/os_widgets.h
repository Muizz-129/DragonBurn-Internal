#pragma once
#include <functional>
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

#include "../helpers/fonts.h"
#include "../helpers/icons.h"
#include "../theme/colors.h"
#include "../theme/layout.h"
#include "../ui/draw/fx.h"

#include <map>
#include <cmath>

namespace os
{
    // ── Animation state (per-ID, from widgets.cpp style) ──
    static std::map<ImGuiID, float> g_wid_hover;
    static std::map<ImGuiID, float> g_wid_active;
    static std::map<ImGuiID, float> g_wid_pulse;

    namespace detail {
        inline void ease(float& v, float target, float dt, float speed = 14.f)
        {
            float step = speed * dt;
            if (v < target) v = ImMin(v + step, target);
            else if (v > target) v = ImMax(v - step, target);
        }

        inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

        inline ImVec4 lerp_col(ImVec4 a, ImVec4 b, float t)
        {
            return { lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t), lerp(a.w, b.w, t) };
        }

        // ── 3-dot listening animation ──
        inline void draw_listening_dots(ImDrawList* dl, ImVec2 center, ImVec4 color)
        {
            int count = ((int)(ImGui::GetTime() * 4.f) % 3) + 1;
            float spacing = 5.f;
            float radius = 1.6f;
            float start_x = center.x - spacing;
            for (int i = 0; i < 3; ++i)
            {
                float a = (i < count) ? 1.f : 0.22f;
                dl->AddCircleFilled({ start_x + (float)i * spacing, center.y }, radius,
                    ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, a)));
            }
        }

        inline float widget_center_y(ImFont* font, float font_size, const ImRect& row)
        {
            return row.Min.y + row.GetHeight() * 0.5f;
        }
    }

    // ── Gradient section header with accent dot ──
    inline void gradient_text(const char* text)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImFont* font = fonts::bold_14();
        if (!font) return;
        const float size = 17.f;
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.f, text);

        // Accent dot before header
        dl->AddCircleFilled({ pos.x + 4.f, pos.y + ts.y * 0.5f }, 3.f,
            ImGui::GetColorU32(colors::Accent));
        // Subtle glow ring around dot
        dl->AddCircleFilled({ pos.x + 4.f, pos.y + ts.y * 0.5f }, 6.f,
            ImGui::GetColorU32(ImVec4(colors::Accent.x, colors::Accent.y, colors::Accent.z, 0.15f)));

        // Shift text right past dot
        ImVec2 text_pos = { pos.x + 14.f, pos.y };
        fx::gradient_text(dl, font, size, text_pos, text,
            ImVec4(colors::Accent.x, colors::Accent.y, colors::Accent.z, 0.9f),
            ImVec4(colors::AccentDark.x, colors::AccentDark.y, colors::AccentDark.z, 0.9f));

        // Thin separator line under header
        dl->AddRectFilled(
            { pos.x + 14.f, pos.y + ts.y + 10.f },
            { pos.x + ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x - 4.f, pos.y + ts.y + 11.f },
            ImGui::GetColorU32(ImVec4(colors::Border.x, colors::Border.y, colors::Border.z, 0.25f)));

        ImGui::Dummy({ 0.f, ts.y + 14.f });
    }

    // ── Animated toggle switch with glow ──
    inline bool switch_button(const char* str_id, bool* v)
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float height = 24.f;
        float width = height * 1.7f;
        float radius = height / 2.f - 2.f;

        ImGui::InvisibleButton(str_id, ImVec2(width, height));
        ImGuiID id = ImGui::GetID(str_id);
        const bool clicked = ImGui::IsItemClicked();
        if (clicked) *v = !(*v);

        bool hovered = ImGui::IsItemHovered();
        float dt = ImGui::GetIO().DeltaTime;

        float t = *v ? 1.f : 0.f;
        float anim_speed = 0.12f;
        ImGuiContext& g = *GImGui;
        if (g.LastActiveId == id)
        {
            float T_Anim = ImSaturate(g.LastActiveIdTimer / anim_speed);
            t = *v ? T_Anim : 1.f - T_Anim;
        }

        detail::ease(g_wid_hover[id], hovered ? 1.f : 0.f, dt, 16.f);
        detail::ease(g_wid_active[id], *v ? 1.f : 0.f, dt, 18.f);

        float ho = g_wid_hover[id];

        // Outer glow ring when active or hovered
        float glow = t * 0.20f + ho * 0.10f;
        if (glow > 0.01f)
        {
            ImRect glow_r({ p.x - 2.f, p.y - 2.f }, { p.x + width + 2.f, p.y + height + 2.f });
            fx::glow_rect(dl, glow_r, colors::Accent, glow * 1.2f, height * 0.5f);
        }

        // Track background — gradient from accent to dimmer when on
        ImVec4 track_off = colors::WidgetBg;
        ImVec4 track_on = colors::Accent;
        ImVec4 track_col = detail::lerp_col(track_off, track_on, t);
        // Hover brightens the track slightly
        track_col.x += ho * 0.04f; track_col.y += ho * 0.04f; track_col.z += ho * 0.04f;

        dl->AddRectFilled(
            { p.x, p.y + height * 0.30f },
            { p.x + width, p.y + height * 0.70f },
            ImGui::GetColorU32(track_col), height * 0.5f);

        // Knob drop shadow
        float knob_x = p.x + radius + t * (width - radius * 2.f);
        float knob_y = p.y + radius + 1.5f;
        dl->AddCircleFilled({ knob_x, knob_y }, radius + 3.f,
            ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.25f + ho * 0.10f)), 360);

        // Knob body
        ImVec4 knob_off = { 0.65f, 0.65f, 0.68f, 1.f };
        ImVec4 knob_on  = { 1.f, 1.f, 1.f, 1.f };
        ImVec4 knob_col = detail::lerp_col(knob_off, knob_on, t);
        // Hover lift
        float hl = 1.f + ho * 0.04f;
        dl->AddCircleFilled({ knob_x, knob_y }, radius * hl,
            ImGui::GetColorU32(knob_col), 360);

        // Inner glow ring on knob when active
        if (t > 0.01f)
        {
            float ring_a = t * 0.25f;
            dl->AddCircle({ knob_x, knob_y }, radius * 0.75f,
                ImGui::GetColorU32(ImVec4(colors::Accent.x, colors::Accent.y, colors::Accent.z, ring_a * 0.5f)),
                360, 1.2f);
        }

        return clicked;
    }

    // ── Layout helpers ──
    inline void align_right(float content_width)
    {
        float col_width = ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x;
        float pos_x = ImGui::GetColumnOffset() + col_width - content_width;
        ImGui::SetCursorPosX(pos_x);
    }

    inline void put_switch(const char* label, float cursor_x, float content_width, bool* v,
        bool color_editor = false, const char* color_label = nullptr, float col[4] = nullptr)
    {
        ImGui::PushID(label);
        float cur_x = ImGui::GetCursorPosX();
        float cur_y = ImGui::GetCursorPosY();
        ImGui::SetCursorPosX(cur_x + cursor_x);
        ImGui::TextDisabled(label);
        ImGui::SameLine();
        ImGui::SetCursorPosY(cur_y - 2);
        if (color_editor && col) {
            align_right(content_width + ImGui::GetFrameHeight() + 7);
            // Styled color edit button
            ImGui::ColorEdit4(color_label, col,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
            ImGui::SameLine();
        } else {
            align_right(content_width);
        }
        switch_button("", v);
        ImGui::PopID();
    }

    // ── Custom gradient slider with glow handle ──
    inline void put_slider_float(const char* label, float cursor_x, float* v,
        float p_min, float p_max, const char* format = "%.0f")
    {
        ImGui::PushID(label);
        float cur_x = ImGui::GetCursorPosX();
        float col_w = ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x;
        float slider_w = col_w - cursor_x - 15;

        // ── Label + value text line ──
        ImGui::SetCursorPosX(cur_x + cursor_x);
        ImGui::TextDisabled(label);

        // Value (bold accent, right-aligned)
        char val_buf[64];
        snprintf(val_buf, sizeof(val_buf), format, *v);
        if (ImFont* bold_font = fonts::bold()) {
            ImVec2 vs = bold_font->CalcTextSizeA(15.f, FLT_MAX, 0.f, val_buf);
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetColumnOffset() + col_w - vs.x - 15);
            ImGui::TextColored(colors::Accent, "%s", val_buf);
        }

        // ── Custom slider row (advances to next line automatically) ──
        ImVec2 ss = ImGui::GetCursorScreenPos(); // screen position of slider row
        const float row_h = 24.f;
        const ImRect bb(ss, { ss.x + slider_w, ss.y + row_h });

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImGuiID id = ImGui::GetID(label);
        ImGui::InvisibleButton("##bar", bb.GetSize());
        bool hovered = ImGui::IsItemHovered();
        bool held   = ImGui::IsItemActive();
        if (held && ImGui::GetIO().MouseDelta.x != 0.f) {
            float pct = ImClamp((ImGui::GetIO().MousePos.x - bb.Min.x) / bb.GetWidth(), 0.f, 1.f);
            *v = p_min + pct * (p_max - p_min);
        }

        float dt = ImGui::GetIO().DeltaTime;
        detail::ease(g_wid_hover[id], (hovered || held) ? 1.f : 0.f, dt, 14.f);
        if (held) g_wid_pulse[id] = 1.f;
        detail::ease(g_wid_pulse[id], 0.f, dt, 8.f);

        float pct = ImClamp((*v - p_min) / (p_max - p_min), 0.f, 1.f);
        float ho = g_wid_hover[id];
        float pu = g_wid_pulse[id];

        // Track vertical center
        float track_y = bb.GetCenter().y;
        float fill_h = 5.f;
        float track_h = 2.f;
        float fill_end = bb.Min.x + bb.GetWidth() * pct;

        // Track bg
        dl->AddRectFilled(
            { bb.Min.x, track_y - track_h * 0.5f },
            { bb.Max.x, track_y + track_h * 0.5f },
            ImGui::GetColorU32(colors::Border), track_h * 0.5f);

        // Fill + glow
        if (pct > 0.001f) {
            ImRect fill_r(bb.Min.x, track_y - fill_h * 0.5f, fill_end, track_y + fill_h * 0.5f);
            fx::glow_rect(dl, fill_r, colors::Accent, pu * 0.35f + ho * 0.20f + 0.15f, fill_h * 0.5f);
            dl->AddRectFilled(fill_r.Min, fill_r.Max,
                ImGui::GetColorU32(colors::Accent), fill_h * 0.5f);
        }

        // Handle dot (skip only at exact 0; draw at 1)
        if (pct > 0.001f) {
            float dot_r = fill_h * 0.55f + ho * 0.8f;
            dl->AddCircleFilled({ fill_end, track_y }, dot_r + 2.f,
                IM_COL32(0,0,0,60));
            dl->AddCircleFilled({ fill_end, track_y }, dot_r + 4.f,
                ImGui::GetColorU32(ImVec4(colors::Accent.x, colors::Accent.y, colors::Accent.z, 0.10f * (1.f + ho))));
            dl->AddCircleFilled({ fill_end, track_y }, dot_r,
                ImGui::GetColorU32(colors::AccentHover));
            dl->AddCircleFilled({ fill_end, track_y }, dot_r * 0.45f,
                IM_COL32(255,255,255,90));
        }

        ImGui::PopID();
    }

    // ── Color edit with styled frame ──
    inline void put_color_edit(const char* label, const char* color_id, float cursor_x,
        float content_width, float col[4])
    {
        ImGui::PushID(label);
        float cur_x = ImGui::GetCursorPosX();
        ImGui::SetCursorPosX(cur_x + cursor_x);
        ImGui::TextDisabled(label);
        ImGui::SameLine();
        align_right(content_width + ImGui::GetFrameHeight() + 8);
        ImGui::ColorEdit4(color_id, col,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
        ImGui::PopID();
    }

    // ── Keybind button with listening animation ──
    inline void hotkey_button(const char* label, const char* btn_id, int* key,
        bool* waiting, std::function<void()> start_capture)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);
        ImGui::TextDisabled(label);
        ImGui::SameLine();
        align_right(70.f);

        // Key capture logic
        if (*waiting)
        {
            extern int scan_any_key(bool allow_mouse1);
            int p = scan_any_key(false);
            if (p > 0) { *key = p; *waiting = false; }
            else if (p == -1) *waiting = false;
        }

        // Button background
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float kb_w = 70.f;
        float kb_h = 22.f;
        ImRect kb_rect(p, { p.x + kb_w, p.y + kb_h });

        ImGui::InvisibleButton(btn_id, { kb_w, kb_h });
        bool hovered = ImGui::IsItemHovered();
        bool clicked = ImGui::IsItemClicked();
        if (clicked && start_capture) start_capture();

        ImGuiID id = ImGui::GetID(btn_id);
        float dt = ImGui::GetIO().DeltaTime;
        detail::ease(g_wid_hover[id], hovered ? 1.f : 0.f, dt, 14.f);
        float active_for = *waiting ? 1.f : 0.f;
        detail::ease(g_wid_active[id], active_for, dt, 16.f);
        float ho = g_wid_hover[id];
        float cap = g_wid_active[id];

        // Background
        ImVec4 bg = detail::lerp_col(colors::KeybindBg, colors::KeybindHover, ho);
        if (cap > 0.01f)
            bg = detail::lerp_col(bg, colors::KeybindCapture, cap);
        dl->AddRectFilled(kb_rect.Min, kb_rect.Max,
            ImGui::GetColorU32(bg), 5.f);

        // Border — accent when capturing
        ImVec4 border_col = detail::lerp_col(
            { colors::Border.x, colors::Border.y, colors::Border.z, 0.25f },
            colors::Accent, cap);
        dl->AddRect(kb_rect.Min, kb_rect.Max,
            ImGui::GetColorU32(border_col), 5.f);

        // Keyboard icon on left
        if (ImFont* icon_font = fonts::icon())
        {
            float icon_size = 11.f;
            ImVec2 icon_pos = { kb_rect.Min.x + 6.f, kb_rect.GetCenter().y - icon_size * 0.5f + 1.f };
            ImU32 icon_col = cap > 0.01f
                ? IM_COL32(255, 255, 255, 220)
                : ImGui::GetColorU32(colors::KeybindMuted);
            dl->AddText(icon_font, icon_size, icon_pos, icon_col, icons::keyboard);
        }

        // Center content: 3-dot animation or key name
        if (*waiting && cap > 0.01f)
        {
            detail::draw_listening_dots(dl, { kb_rect.GetCenter().x, kb_rect.GetCenter().y + 1.f }, colors::KeybindText);
        }
        else
        {
            char klabel[48];
            snprintf(klabel, sizeof(klabel), "%s", *key > 0 ? vk_name(*key) : "None");

            ImFont* font = fonts::regular();
            float fs = 12.f;
            if (font)
            {
                ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.f, klabel);
                ImVec2 text_pos = { kb_rect.Min.x + (kb_w - ts.x) * 0.5f, kb_rect.GetCenter().y - ts.y * 0.5f };
                // Slightly shift text right if icon is drawn
                if (text_pos.x < kb_rect.Min.x + 18.f)
                    text_pos.x = kb_rect.Min.x + 18.f;
                ImU32 text_col = ImGui::GetColorU32(*key > 0 ? colors::KeybindText : colors::KeybindMuted);
                dl->AddText(font, fs, { text_pos.x + 1.f, text_pos.y + 1.f },
                    ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.25f)), klabel);
                dl->AddText(font, fs, text_pos, text_col, klabel);
            }
            else
            {
                ImGui::SetCursorScreenPos({ p.x + 4.f, p.y + 3.f });
                ImGui::Text("%s", klabel);
            }
        }
    }
}
