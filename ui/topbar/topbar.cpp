#include "topbar.h"

#include "../../helpers/animation.h"
#include "../../helpers/fonts.h"
#include "../../helpers/icons.h"
#include "../../theme/colors.h"
#include "../../theme/layout.h"
#include "../../ui/draw/fx.h"

#include "../../imgui/imgui.h"

#include <map>
#include <cmath>

namespace topbar
{
    const char* combat_tab_name(combat_tab tab)
    {
        switch (tab)
        {
        case combat_tab::aimbot:     return "Aimbot";
        case combat_tab::triggerbot: return "Triggerbot";
        case combat_tab::recoil:     return "Recoil";
        default:                     return "";
        }
    }

    namespace
    {
        struct TabAnim
        {
            float pill_alpha = 0.f;
            ImVec4 text = colors::TabTextIdle;
        };

        std::map<ImGuiID, TabAnim> g_tab_anims;

        float calc_tab_width(const char* label)
        {
            ImFont* font = fonts::regular_14();
            if (!font) return 80.f;
            const ImVec2 ts = font->CalcTextSizeA(15.f, FLT_MAX, 0.f, label);
            return ts.x + 32.f;
        }

        float measure_tabs_strip_width()
        {
            float w = 0.f;
            for (int i = 0; i < (int)combat_tab::count; ++i)
                w += calc_tab_width(combat_tab_name((combat_tab)i));
            return w;
        }

        void draw_combat_tabs(ImDrawList* dl, const ImRect& strip, state& st)
        {
            const float strip_h = strip.GetHeight();
            const float pill_round = 5.f;

            // Tab strip background
            dl->AddRectFilled(strip.Min, strip.Max,
                ImGui::GetColorU32(colors::TabStripBg), pill_round);

            // Very subtle top highlight
            dl->AddRectFilled(
                { strip.Min.x, strip.Min.y },
                { strip.Max.x, strip.Min.y + 1.f },
                ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.025f)),
                pill_round);

            ImFont* label_font = fonts::regular_14();
            const float label_size = 15.f;

            float x = strip.Min.x;
            for (int i = 0; i < (int)combat_tab::count; ++i)
            {
                const combat_tab item = (combat_tab)i;
                const bool selected = (st.selected == item);
                const char* label = combat_tab_name(item);

                const float tab_w = calc_tab_width(label);
                const ImRect tab_bb({ x, strip.Min.y }, { x + tab_w, strip.Max.y });

                ImGuiID id = ImGui::GetID(label);
                ImGui::ItemSize(tab_bb);
                ImGui::ItemAdd(tab_bb, id);

                bool hovered = false, held = false;
                const bool pressed = ImGui::ButtonBehavior(tab_bb, id, &hovered, &held);

                TabAnim& anim = g_tab_anims[id];
                const float dt = ImGui::GetIO().DeltaTime;
                anim::static_ease(anim.pill_alpha, selected ? 1.f : 0.f, 8.f);
                anim::dynamic_ease(anim.text, selected ? colors::TabTextActive : colors::TabTextIdle, 22.f);

                // Active pill background
                if (anim.pill_alpha > 0.01f)
                {
                    const float pad = 3.f;
                    const ImRect pill_rect(
                        tab_bb.Min + ImVec2(pad, pad),
                        tab_bb.Max - ImVec2(pad, pad));

                    // Subtle accent glow behind pill
                    fx::glow_rect(dl, pill_rect, colors::AccentDim,
                        anim.pill_alpha * 0.2f, pill_round);

                    dl->AddRectFilled(pill_rect.Min, pill_rect.Max,
                        ImGui::GetColorU32(ImVec4(
                            colors::TabPill.x, colors::TabPill.y,
                            colors::TabPill.z,
                            colors::TabPill.w * anim.pill_alpha)),
                        pill_round);
                }

                // Hover highlight (non-active tabs)
                if (!selected && hovered)
                {
                    const float pad = 3.f;
                    dl->AddRectFilled(
                        tab_bb.Min + ImVec2(pad, pad),
                        tab_bb.Max - ImVec2(pad, pad),
                        ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.04f)),
                        pill_round);
                }

                // Label
                const ImU32 text_col = ImGui::GetColorU32(anim.text);
                if (label_font)
                {
                    const ImVec2 ts = label_font->CalcTextSizeA(label_size, FLT_MAX, 0.f, label);
                    dl->AddText(label_font, label_size,
                        fx::snap_pos({
                            tab_bb.GetCenter().x - ts.x * 0.5f,
                            tab_bb.GetCenter().y - ts.y * 0.5f }),
                        text_col, label);
                }

                if (hovered) st.blocks_drag = true;
                if (pressed) st.selected = item;

                x += tab_w;
            }
        }
    }

    float draw_header(state& header_state)
    {
        header_state.blocks_drag = false;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const float content_w = ImGui::GetContentRegionAvail().x;
        const float row_h = layout::combat_tabs_h;

        // Align tabs to left with some padding
        const float pad_x = 0.f;
        const float x = origin.x + pad_x;
        const float tabs_w = measure_tabs_strip_width();
        const ImRect tab_strip({ x, origin.y }, { x + tabs_w, origin.y + row_h });
        draw_combat_tabs(dl, tab_strip, header_state);

        const float total_h = row_h + layout::header_gap;
        ImGui::Dummy({ content_w, total_h });
        return total_h;
    }
}
