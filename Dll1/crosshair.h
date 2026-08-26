#pragma once
#include "../imgui/imgui.h"
#include <cmath>
#include <algorithm>
#include "types.h"

class Crosshair {
public:
    struct Config {
        bool enabled;
        int shape; // 0=cross, 1=T-shape, 2=circle, 3=dot_only, 4=cross+circle
        float size, gap, thickness;
        ImU32 color;
        bool outline;
        float outline_thickness;
        ImU32 outline_color;
        bool dot;
        float dot_size;
    };

    void draw(ImDrawList* d, int sw, int sh, const Config& c) {
        if (!c.enabled) return;
        // Integer division ensures we are locked to the exact pixel grid
        int cx = sw / 2;
        int cy = sh / 2;
        draw_at(d, cx, cy, c);
    }

    void draw_preview(ImDrawList* d, float cx, float cy, const Config& c) {
        draw_at(d, (int)std::floor(cx), (int)std::floor(cy), c);
    }

private:
    void draw_at(ImDrawList* d, int cx, int cy, const Config& c) {
        switch (c.shape) {
        case 0: draw_cross(d, cx, cy, c, false); break;
        case 1: draw_cross(d, cx, cy, c, true); break;
        case 2: draw_circle(d, cx, cy, c); break;
        case 3: /* Handled entirely by dot rendering */ break;
        case 4:
            draw_cross(d, cx, cy, c, false);
            draw_circle(d, cx, cy, c);
            break;
        }

        if (c.shape == 3 || c.dot) {
            render_dot(d, cx, cy, c);
        }
    }

    static void draw_cross(ImDrawList* d, int cx, int cy, const Config& c, bool t_shape) {
        // Round strictly to integers to prevent ImGui sub-pixel snapping asymmetry
        int t = std::max(1, (int)std::round(c.thickness));
        int g = (int)std::round(c.gap);
        int l = (int)std::round(c.size);
        int o = c.outline ? std::max(1, (int)std::round(c.outline_thickness)) : 0;

        // Calculate thickness distribution (keeps odd/even thickness perfectly centered)
        int tl = t / 2;
        int tr = t - tl;

        // X coordinates for horizontal arms
        int r_x1 = cx + tr + g;
        int r_x2 = r_x1 + l;
        int l_x2 = cx - tl - g;
        int l_x1 = l_x2 - l;

        // Y coordinates for horizontal arms (thickness)
        int h_y1 = cy - tl;
        int h_y2 = cy + tr;

        // Y coordinates for vertical arms
        int b_y1 = cy + tr + g;
        int b_y2 = b_y1 + l;
        int t_y2 = cy - tl - g;
        int t_y1 = t_y2 - l;

        // X coordinates for vertical arms (thickness)
        int v_x1 = cx - tl;
        int v_x2 = cx + tr;

        // Helper to draw strict integer rectangles
        auto draw_rect = [&](int x1, int y1, int x2, int y2, ImU32 col) {
            if (x1 > x2) std::swap(x1, x2);
            if (y1 > y2) std::swap(y1, y2);
            d->AddRectFilled(ImVec2((float)x1, (float)y1), ImVec2((float)x2, (float)y2), col);
            };

        // Draw Outlines
        if (c.outline) {
            ImU32 oc = c.outline_color;
            draw_rect(r_x1 - o, h_y1 - o, r_x2 + o, h_y2 + o, oc); // Right
            draw_rect(l_x1 - o, h_y1 - o, l_x2 + o, h_y2 + o, oc); // Left
            draw_rect(v_x1 - o, b_y1 - o, v_x2 + o, b_y2 + o, oc); // Bottom
            if (!t_shape)
                draw_rect(v_x1 - o, t_y1 - o, v_x2 + o, t_y2 + o, oc); // Top
        }

        // Draw Inner Lines
        ImU32 col = c.color;
        draw_rect(r_x1, h_y1, r_x2, h_y2, col);
        draw_rect(l_x1, h_y1, l_x2, h_y2, col);
        draw_rect(v_x1, b_y1, v_x2, b_y2, col);
        if (!t_shape)
            draw_rect(v_x1, t_y1, v_x2, t_y2, col);
    }

    static void render_dot(ImDrawList* d, int cx, int cy, const Config& c) {
        float radius = (c.shape == 3) ? c.size : c.dot_size;

        // If radius is small, circles turn into diamonds. We force a crisp square block instead.
        if (radius <= 1.5f) {
            int ds = std::max(1, (int)std::round(radius * 2.0f)); // total diameter
            int dl = ds / 2;
            int dr = ds - dl;

            int d_x1 = cx - dl;
            int d_x2 = cx + dr;
            int d_y1 = cy - dl;
            int d_y2 = cy + dr;

            int o = c.outline ? std::max(1, (int)std::round(c.outline_thickness)) : 0;

            if (c.outline) {
                d->AddRectFilled(ImVec2((float)(d_x1 - o), (float)(d_y1 - o)),
                    ImVec2((float)(d_x2 + o), (float)(d_y2 + o)), c.outline_color);
            }
            d->AddRectFilled(ImVec2((float)d_x1, (float)d_y1), ImVec2((float)d_x2, (float)d_y2), c.color);
        }
        else {
            // Safe to draw real high-poly circles for larger dots
            float t = std::max(1.0f, std::round(c.thickness));
            float offset = (std::fmod(t, 2.0f) != 0.0f) ? 0.5f : 0.0f;
            ImVec2 center((float)cx + offset, (float)cy + offset);

            if (c.outline) {
                d->AddCircleFilled(center, radius + c.outline_thickness, c.outline_color, 32);
            }
            d->AddCircleFilled(center, radius, c.color, 32);
        }
    }

    static void draw_circle(ImDrawList* d, int cx, int cy, const Config& c) {
        float radius = c.gap + c.size * 0.5f;
        float t = std::max(1.0f, std::round(c.thickness));

        // For sub-pixel precision: Odd thickness lines belong precisely on pixel centers (0.5)
        // Even thickness lines belong precisely on pixel edges (0.0)
        float offset = (std::fmod(t, 2.0f) != 0.0f) ? 0.5f : 0.0f;
        ImVec2 center((float)cx + offset, (float)cy + offset);

        if (c.outline) {
            d->AddCircle(center, radius, c.outline_color, 48, c.thickness + c.outline_thickness * 2.0f);
        }
        d->AddCircle(center, radius, c.color, 48, c.thickness);
    }
};

inline Crosshair g_crosshair;