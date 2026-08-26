#include "sidebar.h"

#include "../../helpers/animation.h"
#include "../../helpers/fonts.h"
#include "../../theme/colors.h"
#include "../../theme/layout.h"
#include "../../ui/draw/fx.h"

#include "../../imgui/imgui.h"

#include <map>

namespace sidebar
{
    const char* tab_name(tab t)
    {
        switch (t)
        {
        case tab::combat:   return "Combat";
        case tab::visuals:  return "Visuals";
        case tab::misc:     return "Misc";
        case tab::settings: return "Settings";
        default:            return "";
        }
    }

    namespace
    {
        struct TabAnim
        {
            float hover    = 0.f;
            float active   = 0.f;
            float bar_grow = 0.f;
        };

        std::map<ImGuiID, TabAnim> g_tab_anims;

        void draw_brand(ImDrawList* dl, float x, float y)
        {
            const float sz = layout::sidebar_brand_size;
            const ImRect box({ x, y }, { x + sz, y + sz });

            // Icon box with accent glow
            fx::glow_rect(dl, box, colors::AccentDim, 0.35f, layout::round);
            dl->AddRectFilled(box.Min, box.Max,
                ImGui::GetColorU32(colors::Surface), layout::round);
            dl->AddRect(box.Min, box.Max,
                ImGui::GetColorU32(ImVec4(colors::Accent.x, colors::Accent.y, colors::Accent.z, 0.15f)),
                layout::round);

            // Brand letter
            if (ImFont* brand = fonts::brand_18())
            {
                const float font_sz = 22.f;
                const char* letter = "C";
                const ImVec2 ts = brand->CalcTextSizeA(font_sz, FLT_MAX, 0.f, letter);
                dl->AddText(brand, font_sz,
                    fx::snap_pos({ box.GetCenter().x - ts.x * 0.5f,
                                   box.GetCenter().y - ts.y * 0.5f + 1.f }),
                    ImGui::GetColorU32(colors::Accent), letter);
            }

            // Brand name - gradient text
            if (ImFont* title = fonts::brand_18())
            {
                const float label_sz = layout::sidebar_brand_text_size;
                const ImVec2 ts = title->CalcTextSizeA(label_sz, FLT_MAX, 0.f, "ESP");
                fx::gradient_text(dl, title, label_sz,
                    fx::snap_pos({ x + sz + 10.f, y + (sz - ts.y) * 0.5f }),
                    "ESP", colors::AccentDark, colors::AccentHover);
            }
        }

        void draw_tab_row(ImDrawList* dl, float x, float& y, float w, tab item, state& st)
        {
            const bool selected = (st.selected == item);
            const float row_h = layout::sidebar_tab_h;
            const ImRect row({ x, y }, { x + w, y + row_h });
            ImGuiID id = ImGui::GetID(tab_name(item));

            ImGui::ItemSize(row);
            ImGui::ItemAdd(row, id);

            bool hovered = false, held = false;
            const bool pressed = ImGui::ButtonBehavior(row, id, &hovered, &held);

            TabAnim& a = g_tab_anims[id];
            const float dt = ImGui::GetIO().DeltaTime;
            anim::ease(a.hover, hovered ? 1.f : 0.f, dt, 14.f);
            anim::ease(a.active, selected ? 1.f : 0.f, dt, 18.f);
            anim::ease(a.bar_grow, (selected || hovered) ? 1.f : 0.f, dt, 16.f);

            // Hover / active background
            const float bg_alpha = a.active * 0.5f + a.hover * 0.15f;
            if (bg_alpha > 0.01f)
            {
                const ImRect bg_rect(
                    row.Min + ImVec2(6.f, 2.f),
                    row.Max - ImVec2(6.f, 2.f));
                dl->AddRectFilled(bg_rect.Min, bg_rect.Max,
                    ImGui::GetColorU32(ImVec4(
                        colors::TabActive.x, colors::TabActive.y,
                        colors::TabActive.z, bg_alpha)),
                    layout::round);
            }

            // Accent indicator bar (active tab, animated from center)
            if (a.bar_grow > 0.01f)
            {
                const float bar_h = 16.f;
                const float bar_final_w = layout::sidebar_accent_w;
                const float bar_w = bar_final_w * anim::smoothstep(a.bar_grow);
                const float bar_cy = row.GetCenter().y;
                const float bar_x = row.Min.x + layout::sidebar_accent_bar_x;
                fx::accent_stripe(dl,
                    bar_x, bar_cy - bar_h * 0.5f,
                    bar_h, bar_w,
                    colors::Accent, colors::AccentDark);
            }

            // Tab label
            const ImVec4 label_col = anim::lerp_color(
                colors::TabTextIdle, colors::TabTextActive,
                a.active + a.hover * 0.3f);

            if (ImFont* font = fonts::regular_14())
            {
                const float font_sz = 15.f;
                const char* label = tab_name(item);
                const ImVec2 ts = font->CalcTextSizeA(font_sz, FLT_MAX, 0.f, label);
                dl->AddText(font, font_sz,
                    fx::snap_pos({ x + 16.f, row.GetCenter().y - ts.y * 0.5f }),
                    ImGui::GetColorU32(label_col), label);
            }

            if (hovered) st.blocks_drag = true;
            if (pressed) st.selected = item;

            y += row_h + layout::sidebar_tab_gap;
        }
    }

    void draw(const ImRect& bounds, state& sidebar_state)
    {
        sidebar_state.blocks_drag = false;

        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Sidebar base with subtle vertical gradient
        dl->AddRectFilled(bounds.Min, bounds.Max,
            ImGui::GetColorU32(colors::SidebarBg), layout::round,
            ImDrawFlags_RoundCornersLeft);

        fx::gradient_rect_v(dl, bounds,
            ImVec4(colors::SidebarBg.x, colors::SidebarBg.y,
                   colors::SidebarBg.z, 0.f),
            ImVec4(colors::SidebarBg.x + 0.015f, colors::SidebarBg.y + 0.015f,
                   colors::SidebarBg.z + 0.02f, colors::SidebarBg.w * 0.5f),
            layout::round);

        // Accent edge stripe (left side)
        fx::accent_stripe(dl,
            bounds.Min.x, bounds.Min.y + 8.f,
            bounds.GetHeight() - 16.f, layout::sidebar_accent_w,
            colors::SideAccentTop, colors::SideAccentBot);

        const float pad = layout::sidebar_inner_pad;
        const float inner_w = bounds.GetWidth() - pad * 2.f;
        float y = bounds.Min.y + pad;

        const float x = bounds.Min.x + pad;
        draw_brand(dl, x, y);
        y += 50.f;

        // Separator line
        dl->AddRectFilled(
            { bounds.Min.x + 12.f, y - 4.f },
            { bounds.Max.x - 12.f, y - 3.f },
            ImGui::GetColorU32(ImVec4(
                colors::Border.x, colors::Border.y,
                colors::Border.z, 0.35f)));

        for (int i = 0; i < (int)tab::count; ++i)
            draw_tab_row(dl, x, y, inner_w, (tab)i, sidebar_state);
    }
}
