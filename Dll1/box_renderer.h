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
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cctype>
#include "types.h"
#include "settings.h"
#include "utils.h"
#include "chams_renderer.h"
#include "weapon_icons.h"

enum class BoxStyle { CORNERS, FULL, DASHED };
enum class NamePosition { TOP = 0, BOTTOM };

class BoxRenderer {
public:
    static constexpr float PLAYER_ASPECT = 0.48f;

    void draw_box_hp_name(ImDrawList* d, const PlayerVisuals& p,
        const ColorSet& c, int idx, bool is_scoped,
        ImFont* font, float font_size,
        float opacity = 1.0f) {
        float raw_top = 1e9f, raw_bot = -1e9f;
        float center_x_sum = 0;
        int center_cnt = 0;
        float avg_depth = 0;
        int depth_cnt = 0;

        for (int b : BOX_HEIGHT_BONES) {
            if (!p.visible[b]) continue;
            raw_top = (std::min)(raw_top, p.screens[b].y);
            raw_bot = (std::max)(raw_bot, p.screens[b].y);
            avg_depth += p.depths[b];
            depth_cnt++;
        }
        if (depth_cnt < 3) return;
        avg_depth /= depth_cnt;

        static constexpr int CENTER_BONES[] = {
            BONE_HEAD, BONE_NECK, BONE_SPINE1, BONE_SPINE2, BONE_PELVIS
        };
        for (int b : CENTER_BONES) {
            if (!p.visible[b]) continue;
            center_x_sum += p.screens[b].x;
            center_cnt++;
        }
        if (center_cnt == 0) return;
        float center_x = center_x_sum / center_cnt;

        float ds = g_settings.depth_scale;
        if (is_scoped) ds *= 2.0f;

        float head_extra = g_settings.head_radius * ds / avg_depth;
        float pad_y = g_settings.box_padding_y * ds / avg_depth;
        float pad_x = g_settings.box_padding_x * ds / avg_depth;

        raw_top -= (head_extra + pad_y);
        raw_bot += pad_y * 0.5f;

        float height = raw_bot - raw_top;
        if (height < 4.0f) return;

        float width = height * PLAYER_ASPECT + pad_x * 2.0f;

        float x0 = center_x - width * 0.5f;
        float x1 = center_x + width * 0.5f;
        float y0 = raw_top;
        float y1 = raw_bot;

        float box_thick = g_settings.box_thickness;

        if (g_settings.draw_box)
            draw_box(d, x0, y0, x1, y1, box_thick, c, opacity);

        if (g_settings.draw_healthbar)
            draw_healthbar(d, x0, y0, x1, y1, p.health, font,
                g_settings.hp_font_size, opacity);

        if (g_settings.draw_health_text && p.health < 100 && font)
        {
            float bw = 3;
            float bx = x0 - (g_settings.draw_healthbar ? bw + 4 : 2);
            float bh = y1 - y0;
            float hp = std::clamp(p.health / 100.0f, 0.0f, 1.0f);
            float filled = bh * hp;

            char txt[8];
            snprintf(txt, sizeof(txt), "%d", p.health);
            ImVec2 ts = font->CalcTextSizeA(g_settings.hp_font_size, FLT_MAX, 0.0f, txt);

            float tx = floorf(bx - ts.x - 3);
            float ty = floorf((y0 + y1) * 0.5f - ts.y * 0.5f);

            ImU32 outline_col = apply_opacity(float4_to_col(g_settings.hp_text_shadow_color), opacity);
            ImU32 text_col = apply_opacity(float4_to_col(g_settings.hp_text_color), opacity);

            if (g_settings.hp_text_shadow) {
                d->AddText(font, g_settings.hp_font_size, { tx - 1, ty }, outline_col, txt);
                d->AddText(font, g_settings.hp_font_size, { tx + 1, ty }, outline_col, txt);
                d->AddText(font, g_settings.hp_font_size, { tx, ty - 1 }, outline_col, txt);
                d->AddText(font, g_settings.hp_font_size, { tx, ty + 1 }, outline_col, txt);
            }
            d->AddText(font, g_settings.hp_font_size, { tx, ty }, text_col, txt);
        }

        if (g_settings.draw_name && p.name[0] && font)
            draw_name(d, x0, y0, x1, y1, p.name, font, avg_depth, opacity);

        if (g_settings.draw_weapon && (p.weapon[0] || p.weapon_def_index) && font)
            draw_weapon(d, x0, y0, x1, y1, p, font, avg_depth, opacity);

        float label_y = y0;

        if (g_settings.show_is_scoped && p.is_scoped && font) {
            float scoped_font_size = std::roundf(g_settings.esp_font_atlas_size * 0.75f);
            ImU32 scoped_col = apply_opacity(IM_COL32(131, 137, 150, 255), opacity);
            ImU32 scoped_shadow = apply_opacity(IM_COL32(0, 0, 0, 200), opacity);
            ImVec2 spos(x0 - 1.f, label_y - scoped_font_size - 2.f);
            d->AddText(font, scoped_font_size, { spos.x - 1.f, spos.y - 1.f }, scoped_shadow, "s");
            d->AddText(font, scoped_font_size, { spos.x + 1.f, spos.y - 1.f }, scoped_shadow, "s");
            d->AddText(font, scoped_font_size, { spos.x - 1.f, spos.y + 1.f }, scoped_shadow, "s");
            d->AddText(font, scoped_font_size, { spos.x + 1.f, spos.y + 1.f }, scoped_shadow, "s");
            d->AddText(font, scoped_font_size, spos, scoped_col, "s");
            label_y -= scoped_font_size + 2.f;
        }

        if (g_settings.show_flashed_esp && p.flash_duration > 0.f && font) {
            const char* flash_text = "FLASHED";
            float flash_fs = std::roundf(g_settings.esp_font_atlas_size * 0.85f);
            ImVec2 flash_ts = font->CalcTextSizeA(flash_fs, FLT_MAX, 0.f, flash_text);
            ImU32 flash_col = apply_opacity(IM_COL32(255, 230, 80, 255), opacity);
            ImU32 flash_shadow = apply_opacity(IM_COL32(0, 0, 0, 230), opacity);
            ImVec2 fpos(x0 + (x1 - x0) * 0.5f - flash_ts.x * 0.5f,
                label_y - flash_fs - 4.f);

            ImVec2 bg_min(fpos.x - 3.f, fpos.y - 2.f);
            ImVec2 bg_max(fpos.x + flash_ts.x + 3.f, fpos.y + flash_ts.y + 2.f);
            d->AddRectFilled(bg_min, bg_max, apply_opacity(IM_COL32(0, 0, 0, 160), opacity), 2.f);

            d->AddText(font, flash_fs, { fpos.x - 1.f, fpos.y - 1.f }, flash_shadow, flash_text);
            d->AddText(font, flash_fs, { fpos.x + 1.f, fpos.y - 1.f }, flash_shadow, flash_text);
            d->AddText(font, flash_fs, { fpos.x - 1.f, fpos.y + 1.f }, flash_shadow, flash_text);
            d->AddText(font, flash_fs, { fpos.x + 1.f, fpos.y + 1.f }, flash_shadow, flash_text);
            d->AddText(font, flash_fs, fpos, flash_col, flash_text);
            label_y -= flash_fs + 6.f;
        }

        if (p.has_c4 && g_settings.c4_esp_enabled && font) {
            float c4_fs = std::roundf(g_settings.esp_font_atlas_size * 0.75f);
            ImU32 c4_col = apply_opacity(IM_COL32(255, 165, 0, 255), opacity);
            ImU32 c4_shadow = apply_opacity(IM_COL32(0, 0, 0, 230), opacity);
            ImVec2 cpos(x0 - 1.f, label_y - c4_fs - 2.f);
            d->AddText(font, c4_fs, { cpos.x - 1.f, cpos.y - 1.f }, c4_shadow, "C4");
            d->AddText(font, c4_fs, { cpos.x + 1.f, cpos.y - 1.f }, c4_shadow, "C4");
            d->AddText(font, c4_fs, { cpos.x - 1.f, cpos.y + 1.f }, c4_shadow, "C4");
            d->AddText(font, c4_fs, { cpos.x + 1.f, cpos.y + 1.f }, c4_shadow, "C4");
            d->AddText(font, c4_fs, { cpos.x, cpos.y }, c4_col, "C4");
        }
    }

private:
    void draw_box(ImDrawList* d, float x0, float y0, float x1, float y1,
        float box_thick, const ColorSet& c, float opacity) {
        BoxStyle style = static_cast<BoxStyle>(g_settings.box_style);

        x0 = floorf(x0); y0 = floorf(y0);
        x1 = floorf(x1); y1 = floorf(y1);

        switch (style) {
        case BoxStyle::CORNERS: {
            float w = x1 - x0, h = y1 - y0;
            float raw_corner = (std::min)(w, h) * g_settings.box_corner_pct;
            float max_corner = (std::min)(w, h) * 0.45f;
            float corner = std::clamp(raw_corner, 2.0f, (std::max)(max_corner, 2.0f));

            ImU32 bg = apply_opacity(IM_COL32(0, 0, 0, 70), opacity);

            auto draw_corner = [&](ImVec2 tip, ImVec2 h_end, ImVec2 v_end) {
                ImVec2 bg_pts[3] = { v_end, tip, h_end };
                d->AddPolyline(bg_pts, 3, bg, ImDrawFlags_None, box_thick + 2.0f);
                ImVec2 fg_pts[3] = { v_end, tip, h_end };
                d->AddPolyline(fg_pts, 3, c.outline, ImDrawFlags_None, box_thick);
                };

            draw_corner({ x0, y0 }, { x0 + corner, y0 }, { x0, y0 + corner });
            draw_corner({ x1, y0 }, { x1 - corner, y0 }, { x1, y0 + corner });
            draw_corner({ x0, y1 }, { x0 + corner, y1 }, { x0, y1 - corner });
            draw_corner({ x1, y1 }, { x1 - corner, y1 }, { x1, y1 - corner });
            break;
        }
        case BoxStyle::FULL: {
            ImU32 bg = apply_opacity(IM_COL32(0, 0, 0, 70), opacity);
            d->AddRect({ x0, y0 }, { x1, y1 }, bg, 0, 0, box_thick + 2.0f);
            d->AddRect({ x0, y0 }, { x1, y1 }, c.outline, 0, 0, box_thick);
            break;
        }
        case BoxStyle::DASHED: {
            float dash = 8.0f, gap = 5.0f;
            auto dashed_line = [&](ImVec2 a, ImVec2 b, ImU32 col, float thick) {
                float dx = b.x - a.x, dy = b.y - a.y;
                float len = sqrtf(dx * dx + dy * dy);
                if (len < 1) return;
                float nx = dx / len, ny = dy / len;
                float pos = 0;
                while (pos < len) {
                    float end = (std::min)(pos + dash, len);
                    d->AddLine({ a.x + nx * pos, a.y + ny * pos },
                        { a.x + nx * end, a.y + ny * end }, col, thick);
                    pos = end + gap;
                }
                };
            ImU32 bg = apply_opacity(IM_COL32(0, 0, 0, 50), opacity);
            dashed_line({ x0, y0 }, { x1, y0 }, bg, box_thick + 2);
            dashed_line({ x1, y0 }, { x1, y1 }, bg, box_thick + 2);
            dashed_line({ x1, y1 }, { x0, y1 }, bg, box_thick + 2);
            dashed_line({ x0, y1 }, { x0, y0 }, bg, box_thick + 2);
            dashed_line({ x0, y0 }, { x1, y0 }, c.outline, box_thick);
            dashed_line({ x1, y0 }, { x1, y1 }, c.outline, box_thick);
            dashed_line({ x1, y1 }, { x0, y1 }, c.outline, box_thick);
            dashed_line({ x0, y1 }, { x0, y0 }, c.outline, box_thick);
            break;
        }
        }
    }

    void draw_healthbar(ImDrawList* d, float x0, float y0, float x1, float y1,
        int health, ImFont* font, float hp_font_size,
        float opacity)
    {
        float bw = 3, bx = x0 - bw - 4, bh = y1 - y0;
        float hp = std::clamp(health / 100.0f, 0.0f, 1.0f);
        float filled = bh * hp;

        d->AddRectFilled({ bx - 1, y0 - 1 }, { bx + bw + 1, y1 + 1 }, apply_opacity(IM_COL32(0, 0, 0, 60), opacity));
        d->AddRectFilled({ bx, y0 }, { bx + bw, y1 }, apply_opacity(IM_COL32(20, 20, 20, 100), opacity));

        if (g_settings.healthbar_solid_color) {
            ImU32 bar_col = apply_opacity(float4_to_col(g_settings.healthbar_color), opacity);
            d->AddRectFilled({ bx, y0 + (bh - filled) }, { bx + bw, y1 }, bar_col);
        }
        else {
            uint8_t r = (uint8_t)(255 * (1.0f - hp));
            uint8_t g = (uint8_t)(255 * hp);
            d->AddRectFilled({ bx, y0 + (bh - filled) }, { bx + bw, y1 },
                apply_opacity(IM_COL32(r, g, 0, 230), opacity));
        }
    }

    void draw_name(ImDrawList* d, float x0, float y0, float x1, float y1,
        const char* name, ImFont* font, float avg_depth,
        float opacity)
    {
        float name_fs = g_settings.name_font_size;
        ImVec2 ts = font->CalcTextSizeA(name_fs, FLT_MAX, 0.0f, name);

        NamePosition pos = static_cast<NamePosition>(g_settings.name_position);

        float base_gap = 3.0f;
        float nx = 0.0f;
        float ny = 0.0f;

        switch (pos)
        {
        case NamePosition::TOP:
            nx = (x0 + x1) * 0.5f - ts.x * 0.5f;
            ny = y0 - ts.y - base_gap;
            break;

        case NamePosition::BOTTOM:
            nx = (x0 + x1) * 0.5f - ts.x * 0.5f;
            ny = y1 + base_gap;
            break;
        }

        nx += g_settings.name_offset_x;
        ny += g_settings.name_offset_y;

        nx = floorf(nx);
        ny = floorf(ny);

        ImU32 text_col = apply_opacity(float4_to_col(g_settings.name_color), opacity);
        ImU32 outline_col = apply_opacity(float4_to_col(g_settings.name_shadow_color), opacity);

        if (g_settings.name_shadow)
        {
            d->AddText(font, name_fs, { nx - 1, ny }, outline_col, name);
            d->AddText(font, name_fs, { nx + 1, ny }, outline_col, name);
            d->AddText(font, name_fs, { nx, ny - 1 }, outline_col, name);
            d->AddText(font, name_fs, { nx, ny + 1 }, outline_col, name);
        }

        d->AddText(font, name_fs, { nx, ny }, text_col, name);
    }

    void draw_weapon(ImDrawList* d, float x0, float y0, float x1, float y1,
        const PlayerVisuals& p, ImFont* font, float avg_depth,
        float opacity)
    {
        float raw_factor = g_settings.depth_scale / (std::max)(avg_depth, 1.0f);
        raw_factor = std::clamp(raw_factor, 0.1f, 3.0f);

        float dropoff = g_settings.weapon_distance_dropoff;
        float scale = 1.0f + (raw_factor - 1.0f) * dropoff;
        scale = std::clamp(scale, 0.6f, 1.8f);

        float wep_fs = g_settings.weapon_font_size * scale;
        float gap = 2.0f;
        float wy = y1 + gap;

        if (g_settings.draw_name && g_settings.name_position == 1)
            wy += g_settings.name_font_size + 2.0f;

        float center_x = (x0 + x1) * 0.5f;

        bool show_icon = g_settings.weapon_show_icon && p.weapon_def_index > 0 && p.weapon_def_index != 49;
        bool show_text = g_settings.weapon_show_text && p.weapon[0] && p.weapon_def_index != 49;
        if (!show_icon && !show_text) return;

        char wep_lower[80];
        {
            const char* src = p.weapon;
            int i = 0;
            for (; src[i] && i < 63; i++)
                wep_lower[i] = (char)tolower((unsigned char)src[i]);
            wep_lower[i] = 0;
        }

        float icon_w = 0, icon_h = 0;
        float text_w = 0;
        float spacing = 3.0f * scale;

        ImTextureID icon_tex = nullptr;

        if (show_icon) {
            icon_tex = g_weapon_icons.get_icon(p.weapon_def_index);
            if (icon_tex) {
                icon_h = wep_fs + 2.0f * scale;
                float aspect = g_weapon_icons.get_icon_aspect(p.weapon_def_index);
                icon_w = icon_h * aspect;
            }
            else {
                show_icon = false;
            }
        }

        if (show_text) {
            ImVec2 ts = font->CalcTextSizeA(wep_fs, FLT_MAX, 0.0f, wep_lower);
            text_w = ts.x;
        }

        float total_width = icon_w + (show_icon && show_text ? spacing : 0) + text_w;
        float draw_x = center_x - total_width * 0.5f;

        ImU32 outline_col = apply_opacity(float4_to_col(g_settings.weapon_shadow_color), opacity);
        ImU32 text_col = apply_opacity(float4_to_col(g_settings.weapon_color), opacity);
        ImU32 icon_col = apply_opacity(float4_to_col(g_settings.weapon_icon_color), opacity);

        if (show_icon && icon_tex) {
            ImVec2 icon_min = { floorf(draw_x), floorf(wy) };
            ImVec2 icon_max = { floorf(draw_x + icon_w), floorf(wy + icon_h) };

            if (g_settings.weapon_shadow) {
                ImU32 icon_shadow = (outline_col & 0xFF000000) | 0x00000000;
                d->AddImage(icon_tex,
                    { icon_min.x + 1, icon_min.y + 1 },
                    { icon_max.x + 1, icon_max.y + 1 },
                    { 0,0 }, { 1,1 }, icon_shadow);
            }

            d->AddImage(icon_tex, icon_min, icon_max, { 0,0 }, { 1,1 }, icon_col);
            draw_x += icon_w + spacing;
        }

        if (show_text) {
            float text_y = wy;
            if (show_icon)
                text_y = wy + (icon_h - font->CalcTextSizeA(wep_fs, FLT_MAX, 0.0f, wep_lower).y) * 0.5f;

            float tx = floorf(draw_x);
            float ty = floorf(text_y);

            if (g_settings.weapon_shadow) {
                d->AddText(font, wep_fs, { tx - 1, ty }, outline_col, wep_lower);
                d->AddText(font, wep_fs, { tx + 1, ty }, outline_col, wep_lower);
                d->AddText(font, wep_fs, { tx, ty - 1 }, outline_col, wep_lower);
                d->AddText(font, wep_fs, { tx, ty + 1 }, outline_col, wep_lower);
            }

            d->AddText(font, wep_fs, { tx, ty }, text_col, wep_lower);
        }
    }
};