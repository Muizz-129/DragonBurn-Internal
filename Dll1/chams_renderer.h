#pragma once
#include "../imgui/imgui.h"
#include <cmath>
#include <algorithm>
#include "types.h"
#include "settings.h"
#include "utils.h"

// Styles: 0=Filled, 1=Wireframe, 2=Glow, 3=Skeleton
enum class ChamsStyle { FILLED, WIREFRAME, GLOW, SKELETON };

struct ColorSet {
    ImU32 fill, outline, glow, wire, head_fill;
};

inline ColorSet get_enemy_colors(float opacity = 1.0f) {
    ImU32 fill = apply_opacity(float4_to_col(g_settings.enemy_fill), opacity);
    ImU32 outline = apply_opacity(float4_to_col(g_settings.enemy_outline), opacity);
    ImU32 glow = apply_opacity(float4_to_col(g_settings.enemy_glow), opacity);
    return {
        fill, outline, glow, outline,
        apply_opacity((fill & 0x00FFFFFF) | 0x60000000, opacity),
    };
}

inline ColorSet get_team_colors(float opacity = 1.0f) {
    ImU32 fill = apply_opacity(float4_to_col(g_settings.team_fill), opacity);
    ImU32 outline = apply_opacity(float4_to_col(g_settings.team_outline), opacity);
    ImU32 glow = apply_opacity(float4_to_col(g_settings.team_glow), opacity);
    return {
        fill, outline, glow, outline,
        apply_opacity((fill & 0x00FFFFFF) | 0x60000000, opacity),
    };
}

struct LimbQuad {
    ImVec2 p[4];
    bool valid;
};

class ChamsRenderer {
public:
    static LimbQuad build_limb_quad(const PlayerVisuals& p, const LimbDef& limb,
        float depth_scale, float extra = 0) {
        LimbQuad q{};
        q.valid = false;
        if (!p.visible[limb.bone_a] || !p.visible[limb.bone_b]) return q;

        ImVec2 a = p.screens[limb.bone_a], b = p.screens[limb.bone_b];
        float da = p.depths[limb.bone_a], db = p.depths[limb.bone_b];

        float sc = g_settings.body_width_scale;
        float wa = (limb.width_a * sc + extra) * depth_scale / da;
        float wb = (limb.width_b * sc + extra) * depth_scale / db;

        float dx = b.x - a.x, dy = b.y - a.y;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 0.5f) return q;

        float nx = -dy / len, ny = dx / len;
        q.p[0] = { a.x + nx * wa, a.y + ny * wa };
        q.p[1] = { a.x - nx * wa, a.y - ny * wa };
        q.p[2] = { b.x - nx * wb, b.y - ny * wb };
        q.p[3] = { b.x + nx * wb, b.y + ny * wb };
        q.valid = true;
        return q;
    }

    // ===== BODY STYLES (OPTIMIZED QUAD RENDERING) =====

    static void draw_body_filled(ImDrawList* d, const PlayerVisuals& p,
        const ColorSet& c, bool shade, float depth_scale) {
        for (int i = 0; i < BODY_LIMB_COUNT; i++) {
            LimbQuad q = build_limb_quad(p, BODY_LIMBS[i], depth_scale);
            if (!q.valid) continue;

            ImU32 limb_fill = c.fill;
            if (g_settings.vis_check_skeleton) {
                bool occ = p.bone_occluded[BODY_LIMBS[i].bone_a] || p.bone_occluded[BODY_LIMBS[i].bone_b];
                limb_fill = occ ? float4_to_col(g_settings.vis_color_occluded)
                    : float4_to_col(g_settings.vis_color_visible);
            }
            d->AddQuadFilled(q.p[0], q.p[1], q.p[2], q.p[3], limb_fill);
        }
    }

    static void draw_body_outline(ImDrawList* d, const PlayerVisuals& p,
        const ColorSet& c, float depth_scale) {
        for (int i = 0; i < BODY_LIMB_COUNT; i++) {
            LimbQuad q = build_limb_quad(p, BODY_LIMBS[i], depth_scale);
            if (!q.valid) continue;

            ImU32 limb_out = c.outline;
            if (g_settings.vis_check_skeleton) {
                bool occ = p.bone_occluded[BODY_LIMBS[i].bone_a] || p.bone_occluded[BODY_LIMBS[i].bone_b];
                limb_out = occ ? float4_to_col(g_settings.vis_color_occluded)
                    : float4_to_col(g_settings.vis_color_visible);
            }
            d->AddPolyline(q.p, 4, limb_out, ImDrawFlags_Closed, 1.2f);
        }
    }

    static void draw_body_wireframe(ImDrawList* d, const PlayerVisuals& p,
        const ColorSet& c, float depth_scale) {
        float sc = g_settings.body_width_scale;
        for (int i = 0; i < BODY_LIMB_COUNT; i++) {
            const auto& limb = BODY_LIMBS[i];
            if (!p.visible[limb.bone_a] || !p.visible[limb.bone_b]) continue;
            ImVec2 a = p.screens[limb.bone_a], b = p.screens[limb.bone_b];
            float da = p.depths[limb.bone_a], db = p.depths[limb.bone_b];
            float wa = limb.width_a * sc * depth_scale / da;
            float wb = limb.width_b * sc * depth_scale / db;
            float dx = b.x - a.x, dy = b.y - a.y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len < 1.0f) continue;
            ImU32 limb_wire = c.wire;
            ImU32 limb_fill = c.fill;
            if (g_settings.vis_check_skeleton) {
                bool occ = p.bone_occluded[limb.bone_a] || p.bone_occluded[limb.bone_b];
                limb_wire = occ ? float4_to_col(g_settings.vis_color_occluded)
                    : float4_to_col(g_settings.vis_color_visible);
                limb_fill = limb_wire;
            }
            float nx = -dy / len, ny = dx / len;
            ImVec2 p0 = { a.x + nx * wa, a.y + ny * wa };
            ImVec2 p1 = { a.x - nx * wa, a.y - ny * wa };
            ImVec2 p2 = { b.x - nx * wb, b.y - ny * wb };
            ImVec2 p3 = { b.x + nx * wb, b.y + ny * wb };
            d->AddQuadFilled(p0, p1, p2, p3, (limb_fill & 0x00FFFFFF) | 0x10000000);
            d->AddLine(p0, p3, limb_wire, 1.3f);
            d->AddLine(p1, p2, limb_wire, 1.3f);
            d->AddLine(p0, p1, limb_wire, 1.0f);
            d->AddLine(p2, p3, limb_wire, 1.0f);
            d->AddLine(p0, p2, limb_wire, 0.5f);
            d->AddLine(p1, p3, limb_wire, 0.5f);
            if (len > 20.0f) {
                float mw = (wa + wb) * 0.5f;
                ImVec2 mid_a = { (a.x + b.x) * 0.5f + nx * mw, (a.y + b.y) * 0.5f + ny * mw };
                ImVec2 mid_b = { (a.x + b.x) * 0.5f - nx * mw, (a.y + b.y) * 0.5f - ny * mw };
                d->AddLine(mid_a, mid_b, limb_wire, 0.6f);
            }
        }
    }

    static void draw_body_glow(ImDrawList* d, const PlayerVisuals& p,
        const ColorSet& c, float depth_scale, float expand) {
        for (int i = 0; i < BODY_LIMB_COUNT; i++) {
            LimbQuad q = build_limb_quad(p, BODY_LIMBS[i], depth_scale, expand);
            if (!q.valid) continue;

            ImU32 limb_glow = c.glow;
            if (g_settings.vis_check_skeleton) {
                bool occ = p.bone_occluded[BODY_LIMBS[i].bone_a] || p.bone_occluded[BODY_LIMBS[i].bone_b];
                limb_glow = occ ? float4_to_col(g_settings.vis_color_occluded)
                    : float4_to_col(g_settings.vis_color_visible);
            }
            d->AddQuadFilled(q.p[0], q.p[1], q.p[2], q.p[3], limb_glow);
        }
    }

    // ===== HEAD =====

    static void draw_head(ImDrawList* d, const PlayerVisuals& p,
        const ColorSet& c, bool glow, float depth_scale) {
        if (!p.visible[BONE_HEAD] || !p.visible[BONE_NECK]) return;
        ImU32 h_fill = c.head_fill, h_out = c.outline, h_glow = c.glow;
        if (g_settings.vis_check_skeleton) {
            bool occ = p.bone_occluded[BONE_HEAD];
            h_fill = occ ? float4_to_col(g_settings.vis_color_occluded)
                : float4_to_col(g_settings.vis_color_visible);
            h_out = h_fill;
            h_glow = h_fill;
        }
        float r = std::clamp(g_settings.head_radius * depth_scale / p.depths[BONE_HEAD], 2.0f, 80.0f);
        ImVec2 head = p.screens[BONE_HEAD];
        ImVec2 neck = p.screens[BONE_NECK];

        float neck_w_top = r * 0.5f, neck_w_bot = r * 0.7f;
        float dx = neck.x - head.x, dy = neck.y - head.y;
        float len = sqrtf(dx * dx + dy * dy);
        if (len > 1.0f) {
            float nx = -dy / len, ny = dx / len;
            ImVec2 nt[4] = {
                {head.x + nx * neck_w_top, head.y + ny * neck_w_top},
                {head.x - nx * neck_w_top, head.y - ny * neck_w_top},
                {neck.x - nx * neck_w_bot, neck.y - ny * neck_w_bot},
                {neck.x + nx * neck_w_bot, neck.y + ny * neck_w_bot},
            };
            if (glow) {
                float expand = g_settings.glow_expand_inner;
                ImVec2 ntg[4] = {
                    {head.x + nx * (neck_w_top + expand), head.y + ny * (neck_w_top + expand)},
                    {head.x - nx * (neck_w_top + expand), head.y - ny * (neck_w_top + expand)},
                    {neck.x - nx * (neck_w_bot + expand), neck.y - ny * (neck_w_bot + expand)},
                    {neck.x + nx * (neck_w_bot + expand), neck.y + ny * (neck_w_bot + expand)},
                };
                d->AddQuadFilled(ntg[0], ntg[1], ntg[2], ntg[3], h_glow);
            }
            d->AddQuadFilled(nt[0], nt[1], nt[2], nt[3], h_fill);
            d->AddLine(nt[0], nt[3], h_out, 1.0f);
            d->AddLine(nt[1], nt[2], h_out, 1.0f);
        }
        if (glow) {
            d->AddCircleFilled(head, r + g_settings.glow_expand_outer, h_glow, 16);
            d->AddCircleFilled(head, r + g_settings.glow_expand_inner, h_glow, 16);
        }
        d->AddCircleFilled(head, r, h_fill, 16);
        d->AddCircle(head, r, h_out, 16, 1.2f);
    }

    static void draw_head_wire(ImDrawList* d, const PlayerVisuals& p,
        const ColorSet& c, float depth_scale) {
        if (!p.visible[BONE_HEAD] || !p.visible[BONE_NECK]) return;
        ImU32 h_wire = c.wire;
        if (g_settings.vis_check_skeleton) {
            bool occ = p.bone_occluded[BONE_HEAD];
            h_wire = occ ? float4_to_col(g_settings.vis_color_occluded)
                : float4_to_col(g_settings.vis_color_visible);
        }
        float r = std::clamp(g_settings.head_radius * depth_scale / p.depths[BONE_HEAD],
            2.0f, 80.0f);
        ImVec2 head = p.screens[BONE_HEAD];
        ImVec2 neck = p.screens[BONE_NECK];
        d->AddCircleFilled(head, r, (h_wire & 0x00FFFFFF) | 0x10000000, 16);
        d->AddCircle(head, r, h_wire, 16, 1.3f);
        d->AddLine({ head.x - r, head.y }, { head.x + r, head.y }, h_wire, 0.5f);
        d->AddLine({ head.x, head.y - r * 1.15f }, { head.x, head.y + r * 1.15f }, h_wire, 0.5f);
        d->AddLine(head, neck, h_wire, 1.0f);
    }

    // ===== SKELETON =====

    static void draw_skeleton_style(ImDrawList* d, const PlayerVisuals& p, const ColorSet& c, float depth_scale) {
        ImU32 core_col = (c.wire & 0x00FFFFFF) | 0xFF000000;
        ImU32 inner_glow = (c.wire & 0x00FFFFFF) | 0x80000000;
        ImU32 outer_glow = (c.wire & 0x00FFFFFF) | 0x20000000;

        for (const auto& [f, t] : SKELETON_CONNECTIONS) {
            if (p.visible[f] && p.visible[t]) {
                float avg_depth = (p.depths[f] + p.depths[t]) * 0.5f;
                float base_thick = std::clamp(500.0f / avg_depth, 1.0f, 4.0f);

                ImVec2 p1 = p.screens[f];
                ImVec2 p2 = p.screens[t];

                d->AddLine(p1, p2, outer_glow, base_thick * 4.0f);
                d->AddLine(p1, p2, inner_glow, base_thick * 2.0f);
                d->AddLine(p1, p2, core_col, base_thick);

                d->AddCircleFilled(p1, base_thick * 1.5f, core_col);
                d->AddCircleFilled(p2, base_thick * 1.5f, core_col);
            }
        }
    }

    static void draw_skeleton_vis(ImDrawList* d, const PlayerVisuals& p, const ColorSet& c, float depth_scale) {
        ImU32 vis_col = float4_to_col(g_settings.vis_color_visible);
        ImU32 occ_col = float4_to_col(g_settings.vis_color_occluded);

        for (const auto& [f, t] : SKELETON_CONNECTIONS) {
            if (p.visible[f] && p.visible[t]) {
                bool occ = p.bone_occluded[f] || p.bone_occluded[t];
                ImU32 base_color = occ ? occ_col : vis_col;

                ImU32 core_col = (base_color & 0x00FFFFFF) | 0xFF000000;
                ImU32 inner_glow = (base_color & 0x00FFFFFF) | 0x80000000;
                ImU32 outer_glow = (base_color & 0x00FFFFFF) | 0x20000000;

                float avg_depth = (p.depths[f] + p.depths[t]) * 0.5f;
                float base_thick = std::clamp(500.0f / avg_depth, 1.0f, 4.0f);

                ImVec2 p1 = p.screens[f];
                ImVec2 p2 = p.screens[t];

                d->AddLine(p1, p2, outer_glow, base_thick * 4.0f);
                d->AddLine(p1, p2, inner_glow, base_thick * 2.0f);
                d->AddLine(p1, p2, core_col, base_thick);

                d->AddCircleFilled(p1, base_thick * 1.5f, core_col);
                d->AddCircleFilled(p2, base_thick * 1.5f, core_col);
            }
        }
    }

    static void draw_head_skeleton(ImDrawList* d, const PlayerVisuals& p, const ColorSet& c, float depth_scale) {
        if (!p.visible[BONE_HEAD]) return;
        float r = std::clamp(g_settings.head_radius * depth_scale / p.depths[BONE_HEAD], 2.0f, 80.0f);
        ImVec2 head = p.screens[BONE_HEAD];

        ImU32 core_col = (c.wire & 0x00FFFFFF) | 0xFF000000;
        ImU32 inner_glow = (c.wire & 0x00FFFFFF) | 0x80000000;
        ImU32 outer_glow = (c.wire & 0x00FFFFFF) | 0x20000000;

        d->AddCircle(head, r + 2.0f, outer_glow, 16, 3.0f);
        d->AddCircle(head, r + 0.5f, inner_glow, 16, 2.0f);
        d->AddCircle(head, r, core_col, 16, 1.2f);
    }

    static void draw_head_skeleton_vis(ImDrawList* d, const PlayerVisuals& p, float depth_scale) {
        if (!p.visible[BONE_HEAD]) return;
        float r = std::clamp(g_settings.head_radius * depth_scale / p.depths[BONE_HEAD], 2.0f, 80.0f);
        ImVec2 head = p.screens[BONE_HEAD];

        ImU32 base_color = p.bone_occluded[BONE_HEAD] ? float4_to_col(g_settings.vis_color_occluded) : float4_to_col(g_settings.vis_color_visible);

        ImU32 core_col = (base_color & 0x00FFFFFF) | 0xFF000000;
        ImU32 inner_glow = (base_color & 0x00FFFFFF) | 0x80000000;
        ImU32 outer_glow = (base_color & 0x00FFFFFF) | 0x20000000;

        d->AddCircle(head, r + 2.0f, outer_glow, 16, 3.0f);
        d->AddCircle(head, r + 0.5f, inner_glow, 16, 2.0f);
        d->AddCircle(head, r, core_col, 16, 1.2f);
    }

    // ===== MAIN DISPATCH =====

    static void draw_chams(ImDrawList* d, const PlayerVisuals& p,
        const ColorSet& c, ChamsStyle style, float depth_scale) {
        switch (style) {
        case ChamsStyle::FILLED:
            draw_body_filled(d, p, c, false, depth_scale);
            draw_body_outline(d, p, c, depth_scale);
            if (g_settings.draw_head) draw_head(d, p, c, false, depth_scale);
            break;
        case ChamsStyle::WIREFRAME:
            draw_body_wireframe(d, p, c, depth_scale);
            if (g_settings.draw_head) draw_head_wire(d, p, c, depth_scale);
            break;
        case ChamsStyle::GLOW:
            draw_body_glow(d, p, c, depth_scale, g_settings.glow_expand_outer);
            draw_body_glow(d, p, c, depth_scale, g_settings.glow_expand_inner);
            draw_body_filled(d, p, c, false, depth_scale);
            draw_body_outline(d, p, c, depth_scale);
            if (g_settings.draw_head) draw_head(d, p, c, true, depth_scale);
            break;
        case ChamsStyle::SKELETON:
            if (g_settings.vis_check_skeleton)
                draw_skeleton_vis(d, p, c, depth_scale);
            else
                draw_skeleton_style(d, p, c, depth_scale);

            if (g_settings.draw_head) {
                if (g_settings.vis_check_skeleton)
                    draw_head_skeleton_vis(d, p, depth_scale);
                else
                    draw_head_skeleton(d, p, c, depth_scale);
            }
            break;
        }
    }
};