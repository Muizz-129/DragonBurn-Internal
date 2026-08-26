#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "../imgui/imgui.h"
#include <algorithm>
#include "types.h"
#include "settings.h"
#include "chams_renderer.h"
#include "box_renderer.h"
#include "../helpers/fonts.h"

class VisibleESP {
public:
    BoxRenderer box_renderer;

    void draw_player(ImDrawList* draw, const PlayerVisuals& p, int local_team,
        int sw, int sh, int idx, bool is_scoped) {
        if (!p.valid || !g_settings.esp_enabled || !g_settings.master_switch) return;
        bool enemy = (p.team != local_team);
        if (!enemy && !g_settings.draw_teammates) {
            if (p.has_c4 && g_settings.c4_esp_enabled) {
                draw_c4_only_label(draw, p);
            }
            return;
        }

        float avg_depth = 0.0f;
        int   depth_cnt = 0;
        static constexpr int CENTER_BONES[] = {
            BONE_HEAD, BONE_NECK, BONE_SPINE1, BONE_SPINE2, BONE_PELVIS
        };
        for (int b : CENTER_BONES) {
            if (p.visible[b]) { avg_depth += p.depths[b]; depth_cnt++; }
        }
        float opacity = 1.0f;
        if (depth_cnt > 0) opacity = esp_depth_opacity(avg_depth / depth_cnt);

        if (g_settings.esp_use_theme) {
            EspTheme::apply(enemy);
            g_settings.healthbar_solid_color = true;
        }

        ColorSet c = enemy ? get_enemy_colors(opacity) : get_team_colors(opacity);

        if (g_settings.chams_enabled) {
            ChamsStyle style = static_cast<ChamsStyle>(g_settings.chams_style);
            float depth_scale = g_settings.depth_scale * (is_scoped ? 2.0f : 1.0f);
            ChamsRenderer::draw_chams(draw, p, c, style, depth_scale);
        }

        ImFont* font = fonts::regular() ? fonts::regular() : ImGui::GetFont();

        if (g_settings.draw_box || g_settings.draw_healthbar ||
            g_settings.draw_health_text || g_settings.draw_name ||
            g_settings.draw_weapon) {
            box_renderer.draw_box_hp_name(draw, p, c, idx, is_scoped,
                font,
                g_settings.esp_font_atlas_size,
                opacity);
        }
    }

private:
    void draw_c4_only_label(ImDrawList* draw, const PlayerVisuals& p) {
        float top = 1e9f;
        int cnt = 0;
        for (int b : BOX_HEIGHT_BONES) {
            if (!p.visible[b]) continue;
            top = std::min(top, p.screens[b].y);
            cnt++;
        }
        if (cnt < 3) return;

        float avg_depth = 0;
        int depth_cnt = 0;
        for (int b : {BONE_HEAD, BONE_NECK, BONE_SPINE1, BONE_PELVIS}) {
            if (!p.visible[b]) continue;
            avg_depth += p.depths[b];
            depth_cnt++;
        }
        if (depth_cnt == 0) return;
        avg_depth /= depth_cnt;

        ImFont* font = fonts::regular() ? fonts::regular() : ImGui::GetFont();
        if (!font) return;

        float fs = g_settings.esp_font_atlas_size * 0.7f;
        ImU32 col = IM_COL32(255, 165, 0, 230);
        const char* label = "C4";

        float head_extra = g_settings.head_radius * g_settings.depth_scale / avg_depth;
        top -= (head_extra + g_settings.box_padding_y * g_settings.depth_scale / avg_depth);
        top -= fs - 2.f;

        ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, label);
        float center_x = 0;
        int cx_cnt = 0;
        for (int b : {BONE_HEAD, BONE_NECK, BONE_SPINE1, BONE_PELVIS}) {
            if (!p.visible[b]) continue;
            center_x += p.screens[b].x;
            cx_cnt++;
        }
        if (cx_cnt == 0) return;
        center_x /= cx_cnt;

        float lx = floorf(center_x - ts.x * 0.5f);
        float ly = floorf(top);
        ImU32 shadow = IM_COL32(0, 0, 0, 200);
        draw->AddText(font, fs, { lx - 1, ly - 1 }, shadow, label);
        draw->AddText(font, fs, { lx + 1, ly - 1 }, shadow, label);
        draw->AddText(font, fs, { lx - 1, ly + 1 }, shadow, label);
        draw->AddText(font, fs, { lx + 1, ly + 1 }, shadow, label);
        draw->AddText(font, fs, { lx, ly }, col, label);
    }
};

inline VisibleESP g_esp;