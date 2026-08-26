#include "widgets.h"

#include "../../imgui/imgui_internal.h"

#include "../../helpers/animation.h"
#include "../../helpers/fonts.h"
#include "../../helpers/icons.h"
#include "../../theme/colors.h"
#include "../../theme/layout.h"
#include "../../ui/draw/fx.h"

#include <cstdio>
#include <cmath>
#include <cctype>
#include <string>
#include <Windows.h>

namespace widgets
{
    std::map<ImGuiID, WidgetAnim> g_anims;

    namespace
    {
        struct PanelState
        {
            bool open = true;
            float anim = 1.f;
            ImVec2 origin = {};
            float anim_h = 0.f;
            float width = 0.f;
            bool body_open = false;
            bool auto_height = false;
            float actual_body_h = 0.f;
        };

        struct ExternalPanel
        {
            bool open = false;
            float alpha = 0.f;
            float opened_at = -1.f;
            ImRect anchor;
        };

        static std::map<std::string, PanelState> g_panels;
        static std::map<std::string, ExternalPanel> g_popups;
        static const char* g_active_panel = nullptr;

        ImRect accessory_rect(const ImRect& row_bb, int slot, float slot_w)
        {
            const float right = row_bb.Max.x;
            float slot_right = right;
            for (int i = 0; i < slot; ++i)
                slot_right -= layout::cog_w + layout::cog_gap;
            const float slot_left = slot_right - slot_w;
            const float cy = row_bb.GetCenter().y;
            return ImRect({ slot_left, cy - layout::cog_w * 0.5f },
                          { slot_right, cy + layout::cog_w * 0.5f });
        }

        static float row_center_y(const ImRect& row)
        {
            return row.Min.y + row.GetHeight() * 0.5f;
        }

        static float widget_center_y(const ImRect& row, ImFont* font, float font_size)
        {
            const float cy = row_center_y(row) + layout::row_widget_y_nudge;
            if (!font) return cy;
            const float cap_h = font->Ascent * (font_size / font->FontSize);
            const ImVec2 ts = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, "Ag");
            const float bbox_excess = ImMax(0.f, ts.y - cap_h);
            return cy + bbox_excess * 0.5f;
        }

        static ImVec2 icon_pos_in_rect(ImFont* font, float size, const ImRect& rect, const char* icon, float y_nudge = 0.f)
        {
            if (!font || !icon) return rect.GetCenter();
            const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.f, icon);
            return fx::snap_pos({
                rect.Min.x + (rect.GetWidth() - ts.x) * 0.5f,
                rect.Min.y + (rect.GetHeight() - ts.y) * 0.5f + y_nudge });
        }

        static ImVec2 centered_text_pos(ImFont* font, float size, const ImRect& row, const char* text, float x)
        {
            const float cy = row_center_y(row);
            if (!font || !text) return fx::snap_pos({ x, cy - size * 0.5f });
            const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.f, text);
            return fx::snap_pos({ x, cy - ts.y * 0.5f });
        }

        static ImRect right_checkbox_rect(const ImRect& row, float right_limit, float inset, ImFont* font, float font_size)
        {
            const float box = layout::checkbox_size;
            const float cy = widget_center_y(row, font, font_size);
            const float right = right_limit - inset;
            return ImRect({ right - box, cy - box * 0.5f }, { right, cy + box * 0.5f });
        }

        static void close_other_popups(const char* keep)
        {
            for (auto& [id, state] : g_popups)
            {
                if (id != keep)
                    state.open = false;
            }
        }

        static ImVec2 dropdown_pos(const ImRect& anchor)
        {
            return { anchor.Min.x, anchor.Max.y + 4.f };
        }

        static bool begin_dropdown_popup(const char* id, const ImRect& anchor, float width, float height)
        {
            ExternalPanel& state = g_popups[id];
            state.alpha = anim::lerp(state.alpha, state.open ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 16.f);
            if (state.alpha < 0.01f && !state.open)
                return false;

            const ImVec2 pos = dropdown_pos(anchor);
            ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize({ width, height }, ImGuiCond_Always);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, layout::round);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 4.f, 4.f });
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, state.alpha);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.f, 0.f, 0.f, 0.f });

            char win_id[128];
            snprintf(win_id, sizeof(win_id), "##dropdown_%s", id);

            if (!ImGui::Begin(win_id, nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav))
            {
                ImGui::End();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar(4);
                return false;
            }

            const ImRect panel = ImGui::GetCurrentWindow()->Rect();
            ImDrawList* wdl = ImGui::GetWindowDrawList();
            wdl->AddRectFilled(panel.Min, panel.Max,
                ImGui::GetColorU32(colors::DropdownBg), layout::round);
            // Border
            wdl->AddRect(panel.Min, panel.Max,
                ImGui::GetColorU32(ImVec4(colors::Border.x, colors::Border.y, colors::Border.z, 0.5f)),
                layout::round);

            if (state.alpha > 0.5f && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && ImGui::GetTime() - state.opened_at > 0.05f)
            {
                const ImVec2 mouse = ImGui::GetIO().MousePos;
                if (!panel.Contains(mouse) && !anchor.Contains(mouse))
                    state.open = false;
            }

            return true;
        }

        static bool begin_popup(const char* id, const ImRect& anchor, ImVec2 size, bool auto_height = false)
        {
            ExternalPanel& state = g_popups[id];
            state.alpha = anim::lerp(state.alpha, state.open ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 14.f);
            if (state.alpha < 0.01f && !state.open) return false;

            ImGui::SetNextWindowPos({ anchor.Min.x, anchor.Min.y }, ImGuiCond_Always);
            if (auto_height)
                ImGui::SetNextWindowSize({ size.x, 0.f }, ImGuiCond_Always);
            else
                ImGui::SetNextWindowSize(size, ImGuiCond_Always);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, layout::round);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 12.f, 12.f });
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, state.alpha);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.f, 0.f, 0.f, 0.f });

            char win_id[128];
            snprintf(win_id, sizeof(win_id), "##popup_%s", id);

            ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav;
            if (auto_height) flags |= ImGuiWindowFlags_AlwaysAutoResize;

            if (!ImGui::Begin(win_id, nullptr, flags))
            {
                ImGui::End();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar(3);
                return false;
            }

            const ImRect panel = ImGui::GetCurrentWindow()->Rect();
            ImDrawList* wdl = ImGui::GetWindowDrawList();
            wdl->AddRectFilled(panel.Min, panel.Max,
                ImGui::GetColorU32(colors::PopupBg), layout::round);
            wdl->AddRect(panel.Min, panel.Max,
                ImGui::GetColorU32(ImVec4(colors::Border.x, colors::Border.y, colors::Border.z, 0.4f)),
                layout::round);

            if (state.alpha > 0.5f && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && ImGui::GetTime() - state.opened_at > 0.05f)
            {
                const ImVec2 mouse = ImGui::GetIO().MousePos;
                if (!panel.Contains(mouse) && !anchor.Contains(mouse))
                    state.open = false;
            }

            return true;
        }

        static void end_popup()
        {
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(3);
        }

        static void end_dropdown_popup()
        {
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(4);
        }

        static float dropdown_popup_height(int count)
        {
            return 8.f + (float)count * layout::dropdown_item_h;
        }

        // ── Checkmark ──
        static void draw_checkmark(ImDrawList* dl, ImRect box, float active)
        {
            if (active <= 0.05f) return;
            const float sz = box.GetWidth() * 0.48f;
            const ImVec2 pos = {
                box.Min.x + (box.GetWidth() - sz) * 0.5f,
                box.Min.y + (box.GetHeight() - sz) * 0.5f };
            ImGui::RenderCheckMark(dl, pos,
                ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, active)), sz);
        }

        static void draw_listening_dots(ImDrawList* dl, const ImRect& area, ImVec4 color)
        {
            const int count = ((int)(ImGui::GetTime() * 4.f) % 3) + 1;
            const ImVec2 center = area.GetCenter();
            const float spacing = 5.f;
            const float radius = 1.6f;
            const float start_x = center.x - spacing;
            for (int i = 0; i < 3; ++i)
            {
                const float alpha = (i < count) ? 1.f : 0.22f;
                dl->AddCircleFilled(
                    { start_x + (float)i * spacing, center.y }, radius,
                    ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, alpha)));
            }
        }

        // ── Enhanced checkbox ──
        static void draw_checkbox_box(ImDrawList* dl, const ImRect& box_rect, WidgetAnim& anim, bool checked, bool pressed)
        {
            const float r = layout::checkbox_round;
            if (pressed) anim.pulse = 1.f;

            const float dt = ImGui::GetIO().DeltaTime;
            anim::ease(anim.active, checked ? 1.f : 0.f, dt, 16.f);
            anim::ease(anim.pulse, 0.f, dt, 8.f);

            // Glow when active
            const float glow = anim.pulse * 0.4f + anim.active * 0.25f;
            if (glow > 0.02f)
                fx::glow_rect(dl, box_rect, colors::Accent, glow, r);

            // Background fill (animated between off/on)
            const ImVec4 fill_off = colors::WidgetBg;
            const ImVec4 fill_on = colors::Accent;
            const ImVec4 fill = anim::lerp_color(fill_off, fill_on, anim.active);

            dl->AddRectFilled(box_rect.Min, box_rect.Max,
                ImGui::GetColorU32(fill), r);

            // Border
            const ImVec4 border_col = anim::lerp_color(
                colors::Border, ImVec4(colors::Accent.x, colors::Accent.y, colors::Accent.z, 0.6f),
                anim.active);
            dl->AddRect(box_rect.Min, box_rect.Max,
                ImGui::GetColorU32(border_col), r);

            // Checkmark (animated scale)
            draw_checkmark(dl, box_rect, anim.active);
        }

        // ── Key name and mode helpers ──

        static const char* key_name(int key)
        {
            static char buf[32];
            if (key <= 0) return "None";
            switch (key)
            {
            case VK_LBUTTON: return "M1";
            case VK_RBUTTON: return "M2";
            case VK_MBUTTON: return "M3";
            case VK_XBUTTON1: return "M4";
            case VK_XBUTTON2: return "M5";
            case VK_LSHIFT: return "LShift";
            case VK_RSHIFT: return "RShift";
            case VK_SHIFT: return "Shift";
            case VK_LCONTROL: return "LCtrl";
            case VK_RCONTROL: return "RCtrl";
            case VK_CONTROL: return "Ctrl";
            case VK_LMENU: return "LAlt";
            case VK_RMENU: return "RAlt";
            case VK_MENU: return "Alt";
            case VK_SPACE: return "Space";
            case VK_TAB: return "Tab";
            case VK_ESCAPE: return "Esc";
            case VK_INSERT: return "Ins";
            case VK_DELETE: return "Del";
            case VK_HOME: return "Home";
            case VK_END: return "End";
            case VK_PRIOR: return "PgUp";
            case VK_NEXT: return "PgDn";
            default:
                if (key >= '0' && key <= '9') { buf[0] = (char)key; buf[1] = 0; return buf; }
                if (key >= 'A' && key <= 'Z') { buf[0] = (char)key; buf[1] = 0; return buf; }
                buf[0] = 0;
                if (GetKeyNameTextA(MapVirtualKeyA(key, MAPVK_VK_TO_VSC) << 16, buf, 32) > 0)
                    return buf;
                return "Key";
            }
        }

        static void key_name_upper(int key, char* out, size_t out_size)
        {
            const char* name = key_name(key);
            if (out_size == 0) return;
            size_t i = 0;
            for (; name[i] && i + 1 < out_size; ++i)
                out[i] = (char)toupper((unsigned char)name[i]);
            out[i] = 0;
        }

        static bool keybind_mode_item(const char* label, bool selected, bool hovered_row)
        {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            const ImVec2 pos = window->DC.CursorPos;
            const float item_w = ImGui::GetContentRegionAvail().x;
            const float item_h = layout::dropdown_item_h;
            const ImRect item_bb(pos, pos + ImVec2(item_w, item_h));
            ImGuiID id = window->GetID(label);
            ImGui::ItemSize(item_bb);
            if (!ImGui::ItemAdd(item_bb, id)) return false;

            bool hovered = false, held = false;
            const bool pressed = ImGui::ButtonBehavior(item_bb, id, &hovered, &held);

            WidgetAnim& anim = g_anims[id];
            anim::ease(anim.hover, hovered ? 1.f : 0.f, ImGui::GetIO().DeltaTime, 18.f);

            if (anim.hover > 0.01f || hovered_row)
            {
                window->DrawList->AddRectFilled(item_bb.Min, item_bb.Max,
                    ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.03f + anim.hover * 0.04f)), 0.f);
            }

            if (selected || anim.hover > 0.01f)
            {
                const float bar_x = item_bb.Min.x + 4.f;
                window->DrawList->AddRectFilled(
                    { bar_x, item_bb.Min.y + 6.f },
                    { bar_x + layout::accent_bar_w, item_bb.Max.y - 6.f },
                    ImGui::GetColorU32(colors::Accent), 0.f);
            }

            ImFont* font = fonts::regular();
            const float font_size = 14.f;
            const ImVec4 text_col = selected || anim.hover > 0.01f
                ? colors::Text : ImVec4(0.7f, 0.7f, 0.75f, 1.f);
            if (font)
            {
                window->DrawList->AddText(font, font_size,
                    centered_text_pos(font, font_size, item_bb, label, item_bb.Min.x + 14.f),
                    ImGui::GetColorU32(text_col), label);
            }
            return pressed;
        }

        static bool begin_keybind_dropdown(const char* id, const ImRect& container, float width, float height)
        {
            ExternalPanel& state = g_popups[id];
            state.alpha = anim::lerp(state.alpha, state.open ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 16.f);
            if (state.alpha < 0.01f && !state.open) return false;

            const ImVec2 pos = { container.Max.x - width, container.Max.y + 4.f };
            ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize({ width, height }, ImGuiCond_Always);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, layout::round);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, state.alpha);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.f, 0.f, 0.f, 0.f });

            char win_id[128];
            snprintf(win_id, sizeof(win_id), "##kbdrop_%s", id);

            if (!ImGui::Begin(win_id, nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav))
            {
                ImGui::End();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar(3);
                return false;
            }

            const ImRect panel = ImGui::GetCurrentWindow()->Rect();
            ImDrawList* wdl = ImGui::GetWindowDrawList();
            wdl->AddRectFilled(panel.Min, panel.Max,
                ImGui::GetColorU32(colors::PanelHeader), layout::round);
            wdl->AddRect(panel.Min, panel.Max,
                ImGui::GetColorU32(ImVec4(colors::Border.x, colors::Border.y, colors::Border.z, 0.4f)),
                layout::round);

            if (state.alpha > 0.5f && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && ImGui::GetTime() - state.opened_at > 0.05f)
            {
                const ImVec2 mouse = ImGui::GetIO().MousePos;
                if (!panel.Contains(mouse) && !container.Contains(mouse))
                    state.open = false;
            }

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 4.f });
            return true;
        }
    }

    // ════════════════════════════════════════════════════════════════
    //  PANELS
    // ════════════════════════════════════════════════════════════════

    bool begin_panel(const char* title, float width, int rows, bool has_slider, bool default_open, int keybind_rows)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImVec2 origin = window->DC.CursorPos;
        ImDrawList* draw = window->DrawList;

        if (g_panels.find(title) == g_panels.end())
            g_panels[title] = { default_open, default_open ? 1.f : 0.f };
        PanelState& state = g_panels[title];

        state.auto_height = (rows < 0);
        if (state.auto_height) {
            rows = 20;
            state.actual_body_h = 100.f;
        }

        const float body_h = layout::panel_body_height(rows, has_slider, keybind_rows);
        const float dt = ImGui::GetIO().DeltaTime;
        state.anim += ((state.open ? 1.f : 0.f) - state.anim) * ImMin(1.f, dt * 22.f);
        const float anim_body = state.auto_height
            ? state.actual_body_h * anim::smoothstep(state.anim)
            : body_h * anim::smoothstep(state.anim);
        const float anim_h = layout::panel_header_h + anim_body;

        state.origin = origin;
        state.anim_h = anim_h;
        state.width = width;
        state.body_open = false;
        g_active_panel = title;

        const ImVec2 panel_max(origin.x + width, origin.y + anim_h);
        const bool collapsed = state.anim < 0.02f;

        // Panel body background
        draw->AddRectFilled(origin, panel_max,
            ImGui::GetColorU32(colors::PanelBg), layout::panel_round);

        // Panel border
        draw->AddRect(origin, panel_max,
            ImGui::GetColorU32(ImVec4(colors::Border.x, colors::Border.y, colors::Border.z, 0.3f)),
            layout::panel_round);

        // Panel header
        const ImRect header(origin, { panel_max.x, origin.y + layout::panel_header_h });
        draw->AddRectFilled(header.Min, header.Max,
            ImGui::GetColorU32(colors::PanelHeader),
            layout::panel_round,
            collapsed ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersTop);

        // Subtle header gradient
        draw->AddRectFilledMultiColor(header.Min, header.Max,
            ImGui::GetColorU32(colors::GradTop),
            ImGui::GetColorU32(colors::GradTop),
            ImGui::GetColorU32(colors::GradBot),
            ImGui::GetColorU32(colors::GradBot));

        // Header bottom separator
        draw->AddRectFilled(
            { header.Min.x, header.Max.y - 1.f },
            { header.Max.x, header.Max.y },
            ImGui::GetColorU32(ImVec4(colors::Border.x, colors::Border.y, colors::Border.z, 0.15f)));

        // Interaction
        ImGuiID header_id = window->GetID(title);
        ImGui::ItemSize({ width, anim_h });
        ImGui::SetCursorScreenPos(origin);
        ImGui::ItemAdd(header, header_id);

        bool hovered = false, held = false;
        if (ImGui::ButtonBehavior(header, header_id, &hovered, &held))
            state.open = !state.open;

        WidgetAnim& h_anim = g_anims[header_id];
        anim::ease(h_anim.hover, hovered ? 1.f : 0.f, dt, 14.f);

        // Title text
        if (fonts::bold_14())
        {
            draw->AddText(fonts::bold_14(), 15.f,
                centered_text_pos(fonts::bold_14(), 15.f, header, title, origin.x + layout::panel_pad_x),
                ImGui::GetColorU32(anim::lerp_color(colors::TextDim, colors::Text, h_anim.hover * 0.3f + 0.7f)),
                title);
        }

        // Chevron icon
        if (ImFont* icon_font = fonts::icon_12())
        {
            const char* chevron = state.open ? icons::arrow_up : icons::arrow_down;
            const ImVec2 ts = icon_font->CalcTextSizeA(12.f, FLT_MAX, 0.f, chevron);
            const float chevron_x = origin.x + width - layout::panel_pad_x - ts.x;
            draw->AddText(icon_font, 12.f,
                fx::snap_pos({ chevron_x, row_center_y(header) - ts.y * 0.5f }),
                ImGui::GetColorU32(colors::AccentDim), chevron);
        }

        if (state.anim < 0.92f)
        {
            ImGui::SetCursorScreenPos({ origin.x, origin.y + anim_h });
            return false;
        }

        // Begin panel body child
        ImGui::SetCursorScreenPos({ origin.x, origin.y + layout::panel_header_h });
        ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0, 0, 0, 0 });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { layout::panel_pad_x, layout::panel_pad_y });
        ImGui::BeginChild(title, { width, body_h }, false, ImGuiWindowFlags_AlwaysUseWindowPadding);
        ImGui::BeginGroup();
        state.body_open = true;
        return true;
    }

    void end_panel()
    {
        if (g_active_panel && g_panels.count(g_active_panel))
        {
            PanelState& state = g_panels[g_active_panel];
            if (state.body_open)
            {
                ImGui::EndGroup();
                float child_content_end_y = 0.f;
                if (state.auto_height)
                    child_content_end_y = ImGui::GetCurrentWindow()->DC.CursorPos.y;

                ImGui::EndChild();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();

                if (state.auto_height && child_content_end_y > 0.f)
                {
                    ImGuiWindow* parent = ImGui::GetCurrentWindow();
                    const float child_origin_y = state.origin.y + layout::panel_header_h;
                    float actual_body = child_content_end_y - child_origin_y + layout::panel_pad_y;
                    if (actual_body < layout::row_h)
                        actual_body = layout::row_h;

                    state.actual_body_h = actual_body;
                    const float new_anim_h = layout::panel_header_h + actual_body;

                    ImDrawList* dl = parent->DrawList;
                    const ImVec2 bmin(state.origin.x, child_origin_y);
                    const ImVec2 bmax(state.origin.x + state.width, state.origin.y + new_anim_h);
                    dl->AddRectFilled(bmin, bmax,
                        ImGui::GetColorU32(colors::PanelBg),
                        layout::panel_round, ImDrawFlags_RoundCornersBottom);
                    dl->AddRect(bmin, bmax,
                        ImGui::GetColorU32(ImVec4(colors::Border.x, colors::Border.y, colors::Border.z, 0.3f)),
                        layout::panel_round, ImDrawFlags_RoundCornersBottom);

                    state.anim_h = new_anim_h;
                }
            }

            ImGui::SetCursorScreenPos({ state.origin.x, state.origin.y + state.anim_h });

            if (state.auto_height)
            {
                ImGuiWindow* parent = ImGui::GetCurrentWindow();
                const float panel_bottom = state.origin.y + state.anim_h;
                if (parent->DC.CursorMaxPos.y > panel_bottom)
                    parent->DC.CursorMaxPos.y = panel_bottom;
            }

            g_active_panel = nullptr;
            return;
        }

        ImGui::EndGroup();
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    // ════════════════════════════════════════════════════════════════
    //  CHECKBOX
    // ════════════════════════════════════════════════════════════════

    bool checkbox(const char* label, bool* value, bool last_element, float right_inset)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiID id = window->GetID(label);
        const ImVec2 pos = window->DC.CursorPos;
        const float width = ImGui::GetContentRegionAvail().x;
        const ImRect bb(pos, { pos.x + width, pos.y + layout::row_h });

        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered = false, held = false;
        const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
        if (pressed) *value = !*value;

        WidgetAnim& anim = g_anims[id];
        anim::ease(anim.hover, hovered ? 1.f : 0.f, ImGui::GetIO().DeltaTime, 12.f);

        ImDrawList* draw = window->DrawList;
        ImFont* font = fonts::regular();
        const float font_size = font ? 15.f : ImGui::GetFontSize();
        const ImRect box_rect = right_checkbox_rect(bb, bb.Max.x - right_inset, 0.f, font, font_size);
        draw_checkbox_box(draw, box_rect, anim, *value, pressed);

        // Label text
        const float text_max_x = box_rect.Min.x - layout::checkbox_text_gap;
        const ImVec4 text_col = anim::lerp_color(colors::TextDim, colors::Text, anim.active + anim.hover * 0.2f);
        if (font)
        {
            draw->AddText(font, font_size,
                centered_text_pos(font, font_size, bb, label, pos.x),
                ImGui::GetColorU32(text_col), label, nullptr, text_max_x - pos.x);
        }

        (void)last_element;
        return pressed;
    }

    // ════════════════════════════════════════════════════════════════
    //  CHECKBOX + KEYBIND ROW
    // ════════════════════════════════════════════════════════════════

    bool checkbox_keybind_row(const char* label, bool* value, const char* id, int* key, int* mode, bool last_element)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        const ImVec2 pos = window->DC.CursorPos;
        const float width = ImGui::GetContentRegionAvail().x;
        const ImRect row(pos, { pos.x + width, pos.y + layout::row_h });

        ImGuiID row_id = window->GetID(label);
        ImGui::ItemSize(row);
        if (!ImGui::ItemAdd(row, row_id)) return false;

        ImDrawList* draw = window->DrawList;
        ImFont* font = fonts::regular();
        const float font_size = font ? 15.f : ImGui::GetFontSize();
        const float cy = widget_center_y(row, font, font_size);
        const float kb_w = layout::keybind_w;
        const float kb_h = layout::keybind_h;
        const float pad = layout::keybind_pad;
        const float kb_gap = 6.f;

        const ImRect box_rect = right_checkbox_rect(row, row.Max.x, 0.f, font, font_size);

        // Keybind rect
        const float kb_left = box_rect.Min.x - kb_gap - kb_w;
        const ImRect kb({ kb_left, cy - kb_h * 0.5f },
                        { kb_left + kb_w, cy + kb_h * 0.5f });
        const ImRect cb_hit({ pos.x, row.Min.y }, { kb.Min.x - 4.f, row.Max.y });

        // Checkbox
        ImGuiID cb_id = window->GetID((std::string(label) + "_cb").c_str());
        bool cb_hovered = false, cb_held = false;
        const bool cb_pressed = ImGui::ButtonBehavior(cb_hit, cb_id, &cb_hovered, &cb_held);
        if (cb_pressed && value) *value = !*value;

        WidgetAnim& cb_anim = g_anims[cb_id];
        const bool checked = value && *value;
        anim::ease(cb_anim.hover, cb_hovered ? 1.f : 0.f, ImGui::GetIO().DeltaTime, 12.f);
        draw_checkbox_box(draw, box_rect, cb_anim, checked, cb_pressed);

        // Label
        if (font)
        {
            const ImVec4 text_col = anim::lerp_color(colors::TextDim, colors::Text,
                cb_anim.active + cb_anim.hover * 0.2f);
            draw->AddText(font, font_size,
                centered_text_pos(font, font_size, row, label, pos.x),
                ImGui::GetColorU32(text_col), label, nullptr,
                kb.Min.x - layout::checkbox_text_gap - pos.x);
        }

        // Keybind inner elements
        const float icon_box_sz = 20.f;
        const float icon_size = 12.f;
        const ImRect icon_box(
            { kb.Min.x + pad, cy - icon_box_sz * 0.5f },
            { kb.Min.x + pad + icon_box_sz, cy + icon_box_sz * 0.5f });
        const ImRect cog_box(
            { kb.Max.x - pad - icon_box_sz, cy - icon_box_sz * 0.5f },
            { kb.Max.x - pad, cy + icon_box_sz * 0.5f });
        const ImRect text_box({ icon_box.Max.x + 2.f, kb.Min.y },
                              { cog_box.Min.x - 2.f, kb.Max.y });

        char mode_popup_id[64];
        snprintf(mode_popup_id, sizeof(mode_popup_id), "kbmode_%s", id);

        ImGuiID kb_id = window->GetID(id);
        static std::map<ImGuiID, bool> capturing;
        static std::map<ImGuiID, float> capture_timer;
        bool& active_capture = capturing[kb_id];
        float& timer = capture_timer[kb_id];

        ImGuiID icon_id = window->GetID((std::string(id) + "_icon").c_str());
        ImGui::SetNextItemAllowOverlap();
        ImGui::ItemAdd(icon_box, icon_id);
        bool icon_hovered = false, icon_held = false;
        const bool icon_pressed = ImGui::ButtonBehavior(icon_box, icon_id, &icon_hovered, &icon_held);
        if (icon_pressed)
        {
            active_capture = true;
            timer = (float)ImGui::GetTime();
            close_other_popups("none");
            g_popups[mode_popup_id].open = false;
        }

        if (active_capture && ImGui::GetTime() - timer > 0.15)
        {
            static const int extended_keys[] = {
                VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU, VK_LWIN, VK_RWIN
            };
            bool found = false;
            for (int k : extended_keys)
            {
                if (GetAsyncKeyState(k) & 0x8000) { *key = k; active_capture = false; found = true; break; }
            }
            if (!found)
            {
                for (int i = 1; i < 256; ++i)
                {
                    if (i == VK_LBUTTON || i == VK_RBUTTON || i == VK_MBUTTON) continue;
                    if (i == VK_LSHIFT || i == VK_RSHIFT || i == VK_LCONTROL || i == VK_RCONTROL ||
                        i == VK_LMENU || i == VK_RMENU || i == VK_LWIN || i == VK_RWIN) continue;
                    if (GetAsyncKeyState(i) & 0x8000) { *key = i; active_capture = false; break; }
                }
            }
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { *key = 0; active_capture = false; }
        }

        ImGuiID cog_id = window->GetID((std::string(id) + "_cog").c_str());
        ImGui::SetNextItemAllowOverlap();
        ImGui::ItemAdd(cog_box, cog_id);
        bool cog_hovered = false, cog_held = false;
        const bool cog_pressed = !active_capture && ImGui::ButtonBehavior(cog_box, cog_id, &cog_hovered, &cog_held);

        if (cog_pressed)
        {
            close_other_popups(mode_popup_id);
            ExternalPanel& popup = g_popups[mode_popup_id];
            popup.open = true;
            popup.opened_at = (float)ImGui::GetTime();
            popup.anchor = cog_box;
        }

        WidgetAnim& icon_anim = g_anims[icon_id];
        WidgetAnim& cog_anim = g_anims[cog_id];
        anim::ease(icon_anim.hover, icon_hovered && !active_capture ? 1.f : 0.f, ImGui::GetIO().DeltaTime, 14.f);
        anim::ease(cog_anim.hover, cog_hovered ? 1.f : 0.f, ImGui::GetIO().DeltaTime, 10.f);
        cog_anim.slide = anim::lerp(cog_anim.slide, cog_hovered ? 90.f : 0.f, ImGui::GetIO().DeltaTime * 8.f);

        // Keybind background
        draw->AddRectFilled(kb.Min, kb.Max,
            ImGui::GetColorU32(colors::KeybindBg), layout::keybind_round);
        draw->AddRect(kb.Min, kb.Max,
            ImGui::GetColorU32(ImVec4(colors::Border.x, colors::Border.y, colors::Border.z, 0.25f)),
            layout::keybind_round);

        // Keybind icon box
        const ImVec4 icon_bg = active_capture
            ? colors::KeybindCapture
            : anim::lerp_color(colors::WidgetBg, colors::KeybindHover, icon_anim.hover);
        draw->AddRectFilled(icon_box.Min, icon_box.Max,
            ImGui::GetColorU32(icon_bg), layout::keybind_round);

        if (ImFont* icon_font = fonts::icon())
        {
            const ImU32 icon_col = ImGui::GetColorU32(colors::KeybindText);
            draw->AddText(icon_font, icon_size,
                icon_pos_in_rect(icon_font, icon_size, icon_box, icons::keyboard, 1.f),
                icon_col, icons::keyboard);
        }

        // Key label
        static char center_buf[32];
        if (active_capture)
        {
            draw_listening_dots(draw, text_box, colors::KeybindText);
        }
        else
        {
            const char* center_label = "None";
            ImVec4 center_col = colors::KeybindMuted;
            if (*key > 0)
            {
                key_name_upper(*key, center_buf, sizeof(center_buf));
                center_label = center_buf;
                center_col = colors::KeybindText;
            }
            if (font)
            {
                const ImVec2 ts = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, center_label);
                draw->AddText(font, font_size,
                    fx::snap_pos({ text_box.GetCenter().x - ts.x * 0.5f,
                                   text_box.GetCenter().y - ts.y * 0.5f }),
                    ImGui::GetColorU32(center_col), center_label);
            }
        }

        // Cog icon
        if (ImFont* icon_font = fonts::icon())
        {
            const ImU32 cog_col = ImGui::GetColorU32(
                anim::lerp_color(colors::KeybindMuted, colors::KeybindText, cog_anim.hover));
            const ImVec2 cog_center = { cog_box.GetCenter().x, cog_box.GetCenter().y + 1.f };
            fx::icon_text(draw, icon_font, icon_size, cog_center, icons::cog, cog_col, cog_anim.slide);
        }

        // Mode dropdown
        static const char* mode_items[] = { "Hold", "Toggle" };
        static const int mode_values[] = { 1, 0 };
        const float popup_h = dropdown_popup_height(IM_ARRAYSIZE(mode_items));
        if (begin_keybind_dropdown(mode_popup_id, kb, layout::keybind_dropdown_w, popup_h))
        {
            for (int i = 0; i < IM_ARRAYSIZE(mode_items); ++i)
            {
                if (keybind_mode_item(mode_items[i], *mode == mode_values[i], false))
                {
                    *mode = mode_values[i];
                    g_popups[mode_popup_id].open = false;
                }
            }
            ImGui::PopStyleVar();
            end_popup();
        }

        (void)last_element;
        return cb_pressed || icon_pressed || cog_pressed;
    }

    // ════════════════════════════════════════════════════════════════
    //  SLIDER
    // ════════════════════════════════════════════════════════════════

    bool slider_float(const char* label, float* value, float min, float max, const char* format, bool last_element)
    {
        (void)last_element;
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImGuiID id = window->GetID(label);

        const ImVec2 pos = window->DC.CursorPos;
        const float width = ImGui::GetContentRegionAvail().x;
        const ImRect bb(pos, { pos.x + width, pos.y + layout::slider_row_h });

        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id)) return false;

        char val_buf[64];
        snprintf(val_buf, sizeof(val_buf), format, *value);

        ImDrawList* draw = window->DrawList;
        ImFont* font = fonts::regular();
        ImFont* bold_font = fonts::bold();
        const float font_size = font ? 15.f : ImGui::GetFontSize();
        const ImVec2 label_size = font
            ? font->CalcTextSizeA(font_size, FLT_MAX, 0.f, label)
            : ImGui::CalcTextSize(label);
        const ImVec2 val_size = bold_font
            ? bold_font->CalcTextSizeA(font_size, FLT_MAX, 0.f, val_buf)
            : ImVec2(0, 0);

        const float axis_y = widget_center_y(bb, font, font_size);

        // Label
        if (font)
        {
            draw->AddText(font, font_size,
                centered_text_pos(font, font_size, bb, label, pos.x),
                ImGui::GetColorU32(colors::Text), label);
        }

        // Value (bold accent)
        const float val_x = pos.x + label_size.x + layout::slider_label_gap;
        if (bold_font)
        {
            draw->AddText(bold_font, font_size,
                centered_text_pos(bold_font, font_size, bb, val_buf, val_x),
                ImGui::GetColorU32(colors::Accent), val_buf);
        }
        else if (font)
        {
            draw->AddText(font, font_size,
                centered_text_pos(font, font_size, bb, val_buf, val_x),
                ImGui::GetColorU32(colors::Accent), val_buf);
        }

        // Slider track
        const float slider_left = val_x + val_size.x + layout::slider_value_gap;
        const float slider_right = pos.x + width;
        const float slider_w = ImMax(slider_right - slider_left, layout::slider_min_w);
        const float fill_top = axis_y - layout::slider_fill_h * 0.5f;
        const float fill_bottom = axis_y + layout::slider_fill_h * 0.5f;
        const ImRect track({ slider_left, axis_y - layout::slider_track_h * 0.5f },
                           { slider_left + slider_w, axis_y + layout::slider_track_h * 0.5f });
        const ImRect hit({ slider_left, bb.Min.y }, { slider_left + slider_w, bb.Max.y });

        bool hovered = false, held = false;
        ImGui::ButtonBehavior(hit, id, &hovered, &held);
        if (held && slider_w > 0.f && ImGui::GetIO().MouseDelta.x != 0.f)
        {
            const float pct = ImClamp((ImGui::GetIO().MousePos.x - slider_left) / slider_w, 0.f, 1.f);
            *value = min + pct * (max - min);
        }

        WidgetAnim& anim = g_anims[id];
        anim::ease(anim.hover, (hovered || held) ? 1.f : 0.f, ImGui::GetIO().DeltaTime, 12.f);
        if (held && anim.pulse < 0.5f) anim.pulse = 1.f;
        anim::ease(anim.pulse, 0.f, ImGui::GetIO().DeltaTime, 8.f);

        const float pct = ImClamp((*value - min) / (max - min), 0.f, 1.f);
        if (anim.slide < 0.f) anim.slide = pct;
        const float lerp_speed = 1.f - expf(-20.f * ImGui::GetIO().DeltaTime);
        anim.slide += (pct - anim.slide) * lerp_speed;

        // Track background
        draw->AddRectFilled(track.Min, track.Max,
            ImGui::GetColorU32(colors::Border), layout::slider_track_h * 0.5f);

        // Fill
        if (anim.slide > 0.001f)
        {
            const float fill_end = slider_left + slider_w * anim.slide;
            const ImRect fill_rect({ slider_left, fill_top }, { fill_end, fill_bottom });

            // Glow on fill
            const float glow = anim.pulse * 0.35f + anim.hover * 0.2f + 0.12f;
            fx::glow_rect(draw, fill_rect, colors::Accent, glow, layout::slider_fill_h * 0.5f);

            // Fill bar
            draw->AddRectFilled(fill_rect.Min, fill_rect.Max,
                ImGui::GetColorU32(colors::Accent), layout::slider_fill_h * 0.5f);

            // Handle dot at fill tip
            const float dot_r = layout::slider_fill_h * 0.55f;
            draw->AddCircleFilled({ fill_end, axis_y }, dot_r + 2.f,
                ImGui::GetColorU32(ImVec4(colors::Accent.x, colors::Accent.y, colors::Accent.z, 0.25f)));
            draw->AddCircleFilled({ fill_end, axis_y }, dot_r,
                ImGui::GetColorU32(colors::AccentHover));
            draw->AddCircleFilled({ fill_end, axis_y }, dot_r * 0.5f,
                ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.3f)));
        }

        return hovered || held;
    }

    // ════════════════════════════════════════════════════════════════
    //  COMBO
    // ════════════════════════════════════════════════════════════════

    bool combo(const char* label, int* index, const char* const items[], int count, bool last_element)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiID id = window->GetID(label);
        const ImVec2 pos = window->DC.CursorPos;
        const float width = ImGui::GetContentRegionAvail().x;
        const ImRect bb(pos, { pos.x + width, pos.y + layout::row_h });

        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id)) return false;

        const char* preview = (*index >= 0 && *index < count) ? items[*index] : "None";
        ImFont* font = fonts::regular();
        const float font_size = font ? 15.f : ImGui::GetFontSize();
        const bool hidden_label = label[0] == '#';
        const ImVec2 label_size = (!hidden_label && font)
            ? font->CalcTextSizeA(font_size, FLT_MAX, 0.f, label)
            : ImVec2(0.f, 0.f);
        const ImVec2 preview_size = font
            ? font->CalcTextSizeA(font_size, FLT_MAX, 0.f, preview)
            : ImGui::CalcTextSize(preview);

        const float label_reserve = hidden_label ? 0.f : label_size.x + 12.f;
        const float avail_box_w = width - label_reserve;
        const float box_w = hidden_label
            ? ImClamp(avail_box_w, 96.f, layout::combo_max_w)
            : ImClamp(ImMax(preview_size.x + 32.f, 110.f), 110.f, ImMin(avail_box_w, layout::combo_max_w));
        const float box_h = layout::combo_h;
        const float box_cy = widget_center_y(bb, font, font_size);
        const ImVec2 box_min = { bb.Max.x - box_w, box_cy - box_h * 0.5f };
        const ImRect box_rect(box_min, box_min + ImVec2(box_w, box_h));

        bool hovered = false, held = false;
        const bool pressed = ImGui::ButtonBehavior(box_rect, id, &hovered, &held);

        char popup_id[64];
        snprintf(popup_id, sizeof(popup_id), "combo_%s", label);
        if (pressed)
        {
            close_other_popups(popup_id);
            g_popups[popup_id].open = !g_popups[popup_id].open;
            g_popups[popup_id].opened_at = (float)ImGui::GetTime();
            g_popups[popup_id].anchor = box_rect;
        }
        const bool is_open = g_popups[popup_id].open || g_popups[popup_id].alpha > 0.01f;

        WidgetAnim& anim = g_anims[id];
        anim::ease(anim.active, is_open ? 1.f : 0.f, ImGui::GetIO().DeltaTime, 15.f);

        ImDrawList* draw = window->DrawList;

        // Label
        if (font && !hidden_label)
        {
            draw->AddText(font, font_size,
                centered_text_pos(font, font_size, bb, label, pos.x),
                ImGui::GetColorU32(colors::Text), label);
        }

        // Combo box
        const ImVec4 box_bg = anim::lerp_color(colors::WidgetBg, colors::WidgetHover, anim.active * 0.5f);
        draw->AddRectFilled(box_rect.Min, box_rect.Max,
            ImGui::GetColorU32(box_bg), layout::combo_round);
        draw->AddRect(box_rect.Min, box_rect.Max,
            ImGui::GetColorU32(ImVec4(colors::Border.x, colors::Border.y, colors::Border.z, 0.35f)),
            layout::combo_round);

        // Preview text
        if (font)
        {
            const ImRect text_clip(
                { box_rect.Min.x + 10.f, box_rect.Min.y },
                { box_rect.Max.x - 24.f, box_rect.Max.y });
            draw->PushClipRect(text_clip.Min, text_clip.Max, true);
            draw->AddText(font, font_size,
                centered_text_pos(font, font_size, box_rect, preview, box_rect.Min.x + 10.f),
                ImGui::GetColorU32(colors::Text), preview);
            draw->PopClipRect();
        }

        // Chevron
        if (ImFont* icon_font = fonts::icon_12())
        {
            const char* chevron = is_open ? icons::arrow_up : icons::arrow_down;
            const ImVec2 ts = icon_font->CalcTextSizeA(12.f, FLT_MAX, 0.f, chevron);
            draw->AddText(icon_font, 12.f,
                fx::snap_pos({
                    box_rect.Max.x - 10.f - ts.x,
                    box_cy - ts.y * 0.5f }),
                ImGui::GetColorU32(ImVec4(colors::Accent.x, colors::Accent.y, colors::Accent.z, 0.6f)),
                chevron);
        }

        // Dropdown popup
        const float popup_h = dropdown_popup_height(count);
        if (begin_dropdown_popup(popup_id, box_rect, box_w, popup_h))
        {
            for (int i = 0; i < count; ++i)
            {
                if (combo_selectable_item(items[i], *index == i))
                {
                    *index = i;
                    g_popups[popup_id].open = false;
                }
            }
            end_dropdown_popup();
        }

        (void)last_element;
        (void)hovered;
        (void)held;
        return pressed;
    }

    // ════════════════════════════════════════════════════════════════
    //  KEYBIND SETTER
    // ════════════════════════════════════════════════════════════════

    bool keybind_setter(const char* id, int* key, int* mode)
    {
        static bool dummy = true;
        return checkbox_keybind_row("##kb", &dummy, id, key, mode, true);
    }

    // ════════════════════════════════════════════════════════════════
    //  SETTINGS COG
    // ════════════════════════════════════════════════════════════════

    bool settings_cog(const char* id, int slot, bool active)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        char widget_id[128];
        snprintf(widget_id, sizeof(widget_id), "##cog_%s", id);
        ImGuiID wid = window->GetID(widget_id);

        ImRect row_bb(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImRect bb = accessory_rect(row_bb, slot, layout::cog_w);
        ImGui::ItemAdd(bb, wid);

        bool hovered = false, held = false;
        const bool clicked = ImGui::ButtonBehavior(bb, wid, &hovered, &held);

        char popup_id[64];
        snprintf(popup_id, sizeof(popup_id), "settings_%s", id);
        if (clicked)
        {
            close_other_popups(popup_id);
            g_popups[popup_id].open = true;
            g_popups[popup_id].anchor = bb;
        }
        const bool is_open = g_popups[popup_id].open || g_popups[popup_id].alpha > 0.01f;

        WidgetAnim& anim = g_anims[wid];
        anim::ease(anim.active, active ? 1.f : 0.f, ImGui::GetIO().DeltaTime, 14.f);
        anim::ease(anim.hover, (hovered || is_open) ? 1.f : 0.f, ImGui::GetIO().DeltaTime, 12.f);

        if (active && anim.active > 0.05f)
            fx::glow_rect(window->DrawList, bb, colors::Accent, anim.active * 0.28f, layout::round);

        const ImVec4 btn_bg = anim::lerp_color(
            colors::WidgetBg, colors::Accent, anim.active * 0.85f + anim.hover * 0.08f);
        window->DrawList->AddRectFilled(bb.Min, bb.Max,
            ImGui::GetColorU32(btn_bg), layout::round);

        if (ImFont* icon_font = fonts::icon_12())
        {
            const char* glyph = icons::cog;
            const ImVec2 ts = icon_font->CalcTextSizeA(12.f, FLT_MAX, 0.f, glyph);
            window->DrawList->AddText(icon_font, 12.f,
                { bb.GetCenter().x - ts.x * 0.5f, bb.GetCenter().y - ts.y * 0.5f },
                ImGui::GetColorU32(anim::lerp_color(
                    colors::TextDim, ImVec4(1.f, 1.f, 1.f, 1.f), anim.active + anim.hover * 0.35f)),
                glyph);
        }

        if (begin_popup(popup_id, bb, { layout::external_popup_w, 0.f }, true))
            return true;

        return false;
    }

    void end_settings_popup()
    {
        end_popup();
    }

    // ════════════════════════════════════════════════════════════════
    //  SELECTABLE ITEMS
    // ════════════════════════════════════════════════════════════════

    bool combo_selectable_item(const char* label, bool selected)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImVec2 pos = window->DC.CursorPos;
        const float item_w = ImGui::GetContentRegionAvail().x;
        const float item_h = layout::dropdown_item_h;
        const ImRect item_bb(pos, pos + ImVec2(item_w, item_h));
        ImGuiID id = window->GetID(label);
        ImGui::ItemSize(item_bb);
        if (!ImGui::ItemAdd(item_bb, id)) return false;

        bool hovered = false, held = false;
        const bool pressed = ImGui::ButtonBehavior(item_bb, id, &hovered, &held);

        WidgetAnim& anim = g_anims[id];
        anim::ease(anim.hover, hovered ? 1.f : 0.f, ImGui::GetIO().DeltaTime, 18.f);

        if (anim.hover > 0.01f)
        {
            window->DrawList->AddRectFilled(
                item_bb.Min, item_bb.Max,
                ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.05f * anim.hover)),
                layout::round);
        }

        if (selected)
        {
            const float bar_x = item_bb.Min.x + 4.f;
            window->DrawList->AddRectFilled(
                { bar_x, item_bb.Min.y + 6.f },
                { bar_x + layout::accent_bar_w, item_bb.Max.y - 6.f },
                ImGui::GetColorU32(colors::Accent), 0.f);
        }

        ImFont* font = fonts::regular();
        const float font_size = 14.f;
        const ImVec4 text_col = selected
            ? colors::Text
            : anim::lerp_color(colors::TextDim, colors::Text, anim.hover);
        if (font)
        {
            window->DrawList->AddText(font, font_size,
                centered_text_pos(font, font_size, item_bb, label, item_bb.Min.x + 12.f),
                ImGui::GetColorU32(text_col), label);
        }
        return pressed;
    }

    bool selectable_item(const char* label, bool selected)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImVec2 pos = window->DC.CursorPos;
        const float item_h = 26.f;
        const ImRect item_bb(pos, pos + ImVec2(ImGui::GetContentRegionAvail().x, item_h));
        ImGuiID id = window->GetID(label);
        ImGui::ItemSize(item_bb);
        if (!ImGui::ItemAdd(item_bb, id)) return false;

        bool hovered = false, held = false;
        const bool pressed = ImGui::ButtonBehavior(item_bb, id, &hovered, &held);

        WidgetAnim& anim = g_anims[id];
        anim::ease(anim.hover, hovered ? 1.f : 0.f, ImGui::GetIO().DeltaTime, 15.f);

        if (anim.hover > 0.01f || selected)
        {
            window->DrawList->AddRectFilled(
                item_bb.Min, item_bb.Max,
                ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.04f * (selected ? 1.f : anim.hover))),
                4.f);
        }

        const ImVec4 text_col = selected
            ? colors::Accent
            : anim::lerp_color(colors::TextDim, colors::Text, anim.hover);
        window->DrawList->AddText(
            { item_bb.Min.x + 8.f, item_bb.GetCenter().y - 7.f },
            ImGui::GetColorU32(text_col), label);

        return pressed;
    }
}
