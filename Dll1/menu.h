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

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include <map>

#include "settings.h"
#include "types.h"
#include "utils.h"
#include "weapon_icons.h"
#include "theme_manager.h"

#include "../theme/colors.h"
#include "../theme/layout.h"
#include "../helpers/fonts.h"
#include "../ui/os_widgets.h"
#include "../ui/draw/fx.h"
#include "../ui/menu_textures.h"

class Menu {
public:
    void toggle() { g_settings.menu_open = !g_settings.menu_open; }

    void render() {
        if (!g_settings.menu_open) return;

        ImGuiIO& io = ImGui::GetIO();
        const float win_w = layout::window_w;
        const float win_h = layout::window_h;
        const ImVec2 center((io.DisplaySize.x - win_w) * 0.5f, (io.DisplaySize.y - win_h) * 0.5f);

        ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({ win_w, win_h }, ImGuiCond_FirstUseEver);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });

        ImGui::Begin("##dragonburn_menu", &g_settings.menu_open,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBackground);

        ImGui::PopStyleVar(3);

        const ImVec2 origin = ImGui::GetWindowPos();
        const ImRect window_rect(origin, origin + ImVec2(win_w, win_h));
        const float rounding = 8.f;
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // ── Outer shell ──
        dl->AddRectFilled(window_rect.Min, window_rect.Max,
            ImGui::GetColorU32(colors::ShellTint), rounding);
        dl->AddRect(window_rect.Min, window_rect.Max,
            ImGui::GetColorU32(ImVec4(colors::Accent.x, colors::Accent.y, colors::Accent.z, 0.10f)),
            rounding);

        // ── Drag strip at top ──
        const float drag_strip_h = 6.f;
        const ImRect drag_strip_rect(origin, { origin.x + win_w, origin.y + drag_strip_h });

        // ── Sidebar ──
        const float sidebar_w = 56.f;
        const float sidebar_y = origin.y + drag_strip_h;
        const ImRect sidebar_rect(
            { origin.x, sidebar_y },
            { origin.x + sidebar_w, origin.y + win_h });

        // Sidebar background
        dl->AddRectFilled(sidebar_rect.Min, sidebar_rect.Max,
            ImGui::GetColorU32(colors::SidebarBg));
        // Subtle right border
        dl->AddRectFilled(
            { sidebar_rect.Max.x - 1.f, sidebar_rect.Min.y },
            { sidebar_rect.Max.x, sidebar_rect.Max.y },
            ImGui::GetColorU32(ImVec4(colors::Accent.x, colors::Accent.y, colors::Accent.z, 0.06f)));

        // Logo at top of sidebar
        const float logo_size = 36.f;
        const float logo_x = sidebar_rect.GetCenter().x - logo_size * 0.5f;
        const float logo_y = sidebar_rect.Min.y + 10.f;

        if (g_logo.loaded) {
            dl->AddImage((ImTextureID)g_logo.srv,
                { logo_x, logo_y },
                { logo_x + logo_size, logo_y + logo_size });
        }
        else {
            if (ImFont* brand = fonts::brand_18()) {
                dl->AddText(brand, 16.f,
                    fx::snap_pos({ sidebar_rect.GetCenter().x, logo_y + logo_size * 0.3f }),
                    ImGui::GetColorU32(colors::Accent), "DB");
            }
        }

        // Tab buttons (vertical stack)
        struct TabDef { MenuTexture* normal; MenuTexture* pressed; };
        TabDef tabs[4] = {
            {&g_btn_aimbot, &g_btn_aimbot_p},
            {&g_btn_visual, &g_btn_visual_p},
            {&g_btn_misc,   &g_btn_misc_p},
            {&g_btn_config, &g_btn_config_p},
        };
        const char* tab_ids[4] = { "##tab0","##tab1","##tab2","##tab3" };

        const float tab_area_y = sidebar_rect.Min.y + logo_size + 18.f;
        const float tab_h = 46.f;
        const float tab_avail = win_h - drag_strip_h - (tab_area_y - sidebar_rect.Min.y) - 10.f;
        const float tab_gap = ImMax(2.f, (tab_avail - tab_h * 4.f) / 5.f);
        float ty = tab_area_y + tab_gap;

        for (int i = 0; i < 4; ++i) {
            const ImRect tab_bb(
                { origin.x + 4.f, ty },
                { origin.x + sidebar_w - 4.f, ty + tab_h });
            ImGui::SetCursorScreenPos(tab_bb.Min);
            ImGui::InvisibleButton(tab_ids[i], tab_bb.GetSize());

            if (ImGui::IsItemClicked())
                m_page = i;

            bool hovered = ImGui::IsItemHovered();
            bool active = (m_page == i);

            // Active tab: left accent bar
            if (active) {
                dl->AddRectFilled(
                    { origin.x + 1.f, tab_bb.Min.y + 4.f },
                    { origin.x + 3.f, tab_bb.Max.y - 4.f },
                    ImGui::GetColorU32(colors::Accent), 1.5f);
            }

            // Hover highlight
            if (hovered && !active) {
                dl->AddRectFilled(tab_bb.Min, tab_bb.Max,
                    ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.035f)), 4.f);
            }

            // Tab image
            MenuTexture* tex = active ? tabs[i].pressed : tabs[i].normal;
            if (tex->loaded) {
                float img_w = (float)tex->w;
                float img_h = (float)tex->h;
                float scale = ImMin(tab_h / img_h, tab_bb.GetWidth() / img_w) * 0.75f;
                float draw_w = img_w * scale;
                float draw_h = img_h * scale;
                float img_x = tab_bb.GetCenter().x - draw_w * 0.5f;
                float img_y = tab_bb.GetCenter().y - draw_h * 0.5f;

                ImU32 col = active ? IM_COL32(255, 255, 255, 255) : IM_COL32(160, 160, 175, 255);
                dl->AddImage((ImTextureID)tex->srv,
                    { img_x, img_y }, { img_x + draw_w, img_y + draw_h },
                    { 0.f, 0.f }, { 1.f, 1.f }, col);
            }
            else {
                // Fallback: single-letter icons
                const char letters[4] = { 'A', 'V', 'M', 'C' };
                if (ImFont* f = fonts::regular_14()) {
                    float fs = 15.f;
                    char lbl[2] = { letters[i], 0 };
                    ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0.f, lbl);
                    ImU32 col = active ? ImGui::GetColorU32(colors::TextBright)
                        : ImGui::GetColorU32(colors::TextMuted);
                    dl->AddText(f, fs,
                        fx::snap_pos({ tab_bb.GetCenter().x - ts.x * 0.5f,
                                        tab_bb.GetCenter().y - ts.y * 0.5f }),
                        col, lbl);
                }
            }

            ty += tab_h + tab_gap;
        }

        // ── Content area ──
        const float content_x = origin.x + sidebar_w;
        const float content_y = origin.y + drag_strip_h;
        const float content_w = win_w - sidebar_w;
        const float content_h = win_h - drag_strip_h;
        const float pad = 14.f;

        ImGui::SetCursorScreenPos({ content_x + pad, content_y + pad });
        ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.f, 0.f, 0.f, 0.f });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 6.f, 4.f });
        ImGui::BeginChild("##page", { content_w - pad * 2.f, content_h - pad * 2.f },
            false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::PopStyleVar(2);

        // Content area subtle gradient background
        {
            ImRect cr(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
            dl->AddRectFilled(cr.Min, cr.Max,
                ImGui::GetColorU32(colors::Surface), 0.f);
            dl->AddRectFilled(cr.Min, { cr.Max.x, cr.Min.y + 1.f },
                ImGui::GetColorU32(ImVec4(colors::Accent.x, colors::Accent.y, colors::Accent.z, 0.08f)));
        }

        render_page();

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::End();
    }

private:
    int m_page = 0;
    int m_filter_idx = 0;
    static constexpr const char* nade_filter_items[] = { "All", "Smoke", "Molotov", "Frag", "Flash" };

    std::map<ImGuiID, float> m_kb_hover;
    std::map<ImGuiID, float> m_kb_capt;

    bool m_bind_waiting_aimbot = false;
    bool m_bind_waiting_trigger = false;
    bool m_bind_waiting_menu = false;
    bool m_bind_waiting_master = false;
    bool m_bind_waiting_exit = false;
    bool m_reset_popup_open = false;

    void render_page() {
        switch (m_page) {
        case 0: page_aimbot();  break;
        case 1: page_visuals(); break;
        case 2: page_misc();    break;
        case 3: page_config();  break;
        }
    }

    void keybind_button(const char* label, int* key, bool* waiting) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);
        ImGui::TextDisabled(label);
        ImGui::SameLine();
        os::align_right(70.f);

        if (*waiting) {
            int p = scan_any_key(true);
            if (p > 0) { *key = p; *waiting = false; }
            else if (p == -1) *waiting = false;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float kb_w = 70.f, kb_h = 22.f;
        ImRect kb_rect(p, { p.x + kb_w, p.y + kb_h });

        char btn_id[64];
        snprintf(btn_id, sizeof(btn_id), "##kb_%s", label);
        ImGui::InvisibleButton(btn_id, { kb_w, kb_h });
        bool hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) *waiting = true;

        float dt = ImGui::GetIO().DeltaTime;
        ImGuiID id = ImGui::GetID(btn_id);
        auto& ho = m_kb_hover[id];
        auto& ca = m_kb_capt[id];
        os::detail::ease(ho, hovered ? 1.f : 0.f, dt, 14.f);
        os::detail::ease(ca, *waiting ? 1.f : 0.f, dt, 16.f);

        ImVec4 bg = os::detail::lerp_col(colors::KeybindBg, colors::KeybindHover, ho);
        if (ca > 0.01f)
            bg = os::detail::lerp_col(bg, colors::KeybindCapture, ca);
        dl->AddRectFilled(kb_rect.Min, kb_rect.Max,
            ImGui::GetColorU32(bg), 5.f);

        ImVec4 border_col = os::detail::lerp_col(
            { colors::Border.x, colors::Border.y, colors::Border.z, 0.25f },
            colors::Accent, ca);
        dl->AddRect(kb_rect.Min, kb_rect.Max,
            ImGui::GetColorU32(border_col), 5.f);

        if (ImFont* icon_font = fonts::icon()) {
            float icon_size = 11.f;
            dl->AddText(icon_font, icon_size,
                { kb_rect.Min.x + 6.f, kb_rect.GetCenter().y - icon_size * 0.5f + 1.f },
                ca > 0.01f ? IM_COL32(255, 255, 255, 220)
                : ImGui::GetColorU32(colors::KeybindMuted),
                icons::keyboard);
        }

        if (*waiting && ca > 0.01f) {
            os::detail::draw_listening_dots(dl,
                { kb_rect.GetCenter().x, kb_rect.GetCenter().y + 1.f },
                colors::KeybindText);
        }
        else {
            const char* kname = *key > 0 ? vk_name(*key) : "None";
            ImFont* font = fonts::regular();
            float fs = 12.f;
            if (font) {
                ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.f, kname);
                ImVec2 tp = { kb_rect.Min.x + 18.f, kb_rect.GetCenter().y - ts.y * 0.5f };
                dl->AddText(font, fs, { tp.x + 1.f, tp.y + 1.f },
                    IM_COL32(0, 0, 0, 40), kname);
                dl->AddText(font, fs, tp,
                    ImGui::GetColorU32(*key > 0 ? colors::KeybindText : colors::KeybindMuted),
                    kname);
            }
            else {
                ImGui::SetCursorScreenPos({ p.x + 4.f, p.y + 3.f });
                ImGui::Text("%s", kname);
            }
        }
    }

    void page_aimbot() {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetCursorPos({ 15.f, 4.f });

        os::gradient_text("Aimbot");
        os::put_switch("Enable", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.aimbot_enabled);
        if (g_settings.aimbot_enabled) {
            keybind_button("Hotkey", &g_settings.key_aimbot, &m_bind_waiting_aimbot);
            os::put_slider_float("FOV", 10.f, &g_settings.aimbot_fov, 1.f, 360.f, "%.0f");
            os::put_slider_float("Smoothness", 10.f, &g_settings.aimbot_smooth, 1.f, 20.f, "%.0f");
            os::put_slider_float("Sensitivity", 10.f, &g_settings.aimbot_sensitivity, 0.1f, 5.0f, "%.2f");

            const char* bones[] = { "Head", "Neck", "Chest", "Pelvis" };
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);
                ImGui::TextDisabled("Bone");
                ImGui::SameLine();
                os::align_right(160.f);
                ImGui::SetNextItemWidth(160.f);
                ImGui::Combo("###aim_bone", &g_settings.aimbot_bone, bones, 4);
            }

            os::put_switch("Team Check", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.aimbot_team_check);
            os::put_switch("Draw FOV Circle", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.draw_aimbot_fov,
                true, "###fovc", g_settings.aimbot_fov_color);
        }

        ImGui::NextColumn();
        ImGui::SetCursorPosY(4.f);

        os::gradient_text("Triggerbot");
        os::put_switch("Enable", 5.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.triggerbot_enabled);
        if (g_settings.triggerbot_enabled) {
            keybind_button("Hotkey", &g_settings.key_triggerbot, &m_bind_waiting_trigger);
            os::put_switch("Always Active", 5.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.triggerbot_always_on);
            os::put_switch("Scoped Only", 5.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.triggerbot_scoped_only);
            os::put_slider_float("Delay (ms)", 5.f, &g_settings.triggerbot_delay, 0.f, 300.f, "%.0f");
        }

        ImGui::Columns(1);
    }

    void page_visuals() {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetCursorPos({ 15.f, 4.f });

        os::gradient_text("ESP");
        os::put_switch("Enabled", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.esp_enabled);
        if (g_settings.esp_enabled) {
            os::put_switch("Draw Teammates", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.draw_teammates);
            os::put_switch("Show Scoped", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.show_is_scoped);
            os::put_switch("Show Flashed", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.show_flashed_esp);
            os::put_switch("C4 ESP", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.c4_esp_enabled);
            os::put_switch("Bomb Timer", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.show_bomb_timer);

            os::put_switch("Box", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.draw_box,
                true, "###box_col", g_settings.enemy_fill);
            if (g_settings.draw_box) {
                const char* box_styles[] = { "Corners", "Full", "Dashed" };
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);
                ImGui::TextDisabled("Box Style");
                ImGui::SameLine();
                os::align_right(160.f);
                ImGui::SetNextItemWidth(160.f);
                ImGui::Combo("###box_style", &g_settings.box_style, box_styles, 3);
            }

            os::put_switch("Health Bar", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.draw_healthbar);
            os::put_switch("Health Text", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.draw_health_text);
            os::put_switch("Name", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.draw_name);
            os::put_switch("Weapon", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.draw_weapon);

            os::put_switch("Skeleton", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.chams_enabled);
            if (g_settings.chams_enabled) {
                const char* styles[] = { "Filled", "Wire", "Glow", "Skeleton" };
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);
                ImGui::TextDisabled("Style");
                ImGui::SameLine();
                os::align_right(160.f);
                ImGui::SetNextItemWidth(160.f);
                ImGui::Combo("###chams_style", &g_settings.chams_style, styles, 4);
                os::put_switch("Visibility Check", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.vis_check_skeleton,
                    true, "###vis_c", g_settings.vis_color_visible);
                if (g_settings.vis_check_skeleton) {
                    os::put_color_edit("Occluded", "###occ_c", 10.f,
                        ImGui::GetFrameHeight() * 1.7f, g_settings.vis_color_occluded);
                }
            }

            os::put_switch("Distance Opacity", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.esp_opacity_drop);
        }

        ImGui::NextColumn();
        ImGui::SetCursorPosY(4.f);

        os::gradient_text("ESP Theme");
        os::put_switch("Use Theme Color", 5.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.esp_use_theme);
        if (g_settings.esp_use_theme) {
            os::put_color_edit("Theme", "###tc", 5.f, ImGui::GetFrameHeight() * 1.7f, g_settings.esp_theme_color);
        }
        else {
            os::put_color_edit("Enemy Fill", "###ef", 5.f, ImGui::GetFrameHeight() * 1.7f, g_settings.enemy_fill);
            os::put_color_edit("Enemy Outline", "###eo", 5.f, ImGui::GetFrameHeight() * 1.7f, g_settings.enemy_outline);
            os::put_color_edit("Enemy Glow", "###eg", 5.f, ImGui::GetFrameHeight() * 1.7f, g_settings.enemy_glow);
            os::put_color_edit("Team Fill", "###tf", 5.f, ImGui::GetFrameHeight() * 1.7f, g_settings.team_fill);
            os::put_color_edit("Team Outline", "###to", 5.f, ImGui::GetFrameHeight() * 1.7f, g_settings.team_outline);
            os::put_color_edit("Team Glow", "###tg", 5.f, ImGui::GetFrameHeight() * 1.7f, g_settings.team_glow);
        }

        ImGui::NewLine();
        os::gradient_text("Body Tuning");
        os::put_slider_float("Body Width", 5.f, &g_settings.body_width_scale, 0.3f, 3.f, "%.2f");
        os::put_slider_float("Head Radius", 5.f, &g_settings.head_radius, 1.f, 10.f, "%.1f");
        os::put_slider_float("Depth Scale", 5.f, &g_settings.depth_scale, 100.f, 1500.f, "%.0f");
        os::put_slider_float("Glow Outer", 5.f, &g_settings.glow_expand_outer, 0.f, 15.f, "%.1f");

        ImGui::NewLine();
        os::gradient_text("Radar");
        os::put_switch("Show", 5.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.draw_radar);
        os::put_switch("Circle", 5.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.radar_circle);
        os::put_switch("Rotate", 5.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.radar_rotate);
        os::put_switch("Range Rings", 5.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.radar_rings);
        os::put_switch("Player Names", 5.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.radar_names);
        os::put_color_edit("Enemy", "###rade", 5.f, ImGui::GetFrameHeight() * 1.7f, g_settings.radar_enemy_color);
        os::put_color_edit("Team", "###radt", 5.f, ImGui::GetFrameHeight() * 1.7f, g_settings.radar_team_color);
        if (g_settings.radar_names)
            os::put_slider_float("Name Font", 5.f, &g_settings.radar_names_font_size, 8.f, 20.f, "%.0f");

        ImGui::Columns(1);
    }

    void page_misc() {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetCursorPos({ 15.f, 4.f });

        os::gradient_text("Misc");
        os::put_switch("Spectator List", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.draw_spectators);
        os::put_switch("Crosshair", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.crosshair_enabled,
            true, "###xcol", g_settings.crosshair_color);
        if (g_settings.crosshair_enabled) {
            const char* shapes[] = { "Cross", "T-Shape", "Circle", "Dot", "Cross+Circle" };
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);
            ImGui::TextDisabled("Shape");
            ImGui::SameLine();
            os::align_right(160.f);
            ImGui::SetNextItemWidth(160.f);
            ImGui::Combo("###xh_shape", &g_settings.crosshair_shape, shapes, 5);
            os::put_slider_float("Size", 10.f, &g_settings.crosshair_size, 0.5f, 20.f, "%.1f");
            os::put_slider_float("Thickness", 10.f, &g_settings.crosshair_thickness, 0.5f, 5.f, "%.1f");
            if (g_settings.crosshair_shape <= 1 || g_settings.crosshair_shape == 4)
                os::put_slider_float("Gap", 10.f, &g_settings.crosshair_gap, -10.f, 10.f, "%.1f");
            os::put_switch("Outline", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.crosshair_outline);
            if (g_settings.crosshair_shape != 3)
                os::put_switch("Center Dot", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.crosshair_dot);
        }

        ImGui::NextColumn();
        ImGui::SetCursorPosY(4.f);

        os::gradient_text("Grenade Helper");
        os::put_switch("Enabled", 5.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.grenade_helper_enabled);
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.f);
            ImGui::TextDisabled("Filter");
            ImGui::SameLine();
            os::align_right(160.f);
            ImGui::SetNextItemWidth(160.f);
            ImGui::Combo("###nade_filt", &m_filter_idx, nade_filter_items, IM_ARRAYSIZE(nade_filter_items));
            update_nade_filter();
        }
        os::put_color_edit("Circle", "###ncc", 5.f, 30.f, g_settings.grenade_circle_color);
        os::put_color_edit("Active Circle", "###nac", 5.f, 30.f, g_settings.grenade_circle_active_color);
        os::put_color_edit("Aim Line", "###nal", 5.f, 30.f, g_settings.grenade_aim_line_color);
        os::put_color_edit("Text", "###ntc", 5.f, 30.f, g_settings.grenade_text_color);
        os::put_slider_float("Circle Radius", 5.f, &g_settings.grenade_circle_radius, 10.f, 150.f, "%.0f");
        os::put_slider_float("Circle Thick.", 5.f, &g_settings.grenade_circle_thickness, 0.5f, 4.f, "%.1f");

        ImGui::Columns(1);
    }

    void update_nade_filter() {
        g_settings.grenade_filter_smoke = (m_filter_idx == 0 || m_filter_idx == 1);
        g_settings.grenade_filter_molotov = (m_filter_idx == 0 || m_filter_idx == 2);
        g_settings.grenade_filter_frag = (m_filter_idx == 0 || m_filter_idx == 3);
        g_settings.grenade_filter_flash = (m_filter_idx == 0 || m_filter_idx == 4);
    }

    void page_config() {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetCursorPos({ 15.f, 4.f });

        os::gradient_text("General");
        os::put_switch("Master Switch", 10.f, ImGui::GetFrameHeight() * 1.7f, &g_settings.master_switch);
        os::put_slider_float("Target FPS", 10.f, &g_settings.target_fps, 30.f, 240.f, "%.0f");
        os::put_slider_float("Background Alpha", 10.f, &g_settings.menu_bg_alpha, 0.3f, 1.f, "%.2f");

        ImGui::NewLine();
        os::gradient_text("Key Binds");
        keybind_button("Menu Toggle", &g_settings.key_menu, &m_bind_waiting_menu);
        keybind_button("Master Toggle", &g_settings.key_master, &m_bind_waiting_master);
        keybind_button("Exit", &g_settings.key_exit, &m_bind_waiting_exit);

        ImGui::NextColumn();
        ImGui::SetCursorPosY(4.f);

        os::gradient_text("UI Themes");
        {
            const ImVec2 btn_avail = ImGui::GetContentRegionAvail();
            if (g_theme_mgr.theme_files.empty()) {
                g_theme_mgr.refresh_themes();
            }

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.f);
            if (ImGui::Button("Refresh Theme List", { btn_avail.x - 10.f, 26.f })) {
                g_theme_mgr.refresh_themes();
            }

            ImGui::Dummy({ 0.f, 4.f });

            if (!g_theme_mgr.theme_files.empty()) {
                if (g_theme_mgr.selected_theme_index >= (int)g_theme_mgr.theme_files.size())
                    g_theme_mgr.selected_theme_index = 0;

                std::string preview = g_theme_mgr.theme_files[g_theme_mgr.selected_theme_index];
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.f);
                ImGui::SetNextItemWidth(btn_avail.x - 10.f);

                if (ImGui::BeginCombo("##theme_select", preview.c_str())) {
                    for (int n = 0; n < (int)g_theme_mgr.theme_files.size(); n++) {
                        bool is_selected = (g_theme_mgr.selected_theme_index == n);
                        if (ImGui::Selectable(g_theme_mgr.theme_files[n].c_str(), is_selected)) {
                            g_theme_mgr.selected_theme_index = n;
                            g_theme_mgr.load_theme(g_theme_mgr.theme_files[n]);
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
            else {
                ImGui::TextDisabled("  No .json files in /themes");
            }
        }

        ImGui::NewLine();
        os::gradient_text("Config");
        {
            const ImVec2 btn_avail = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(colors::WidgetBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(colors::WidgetHover));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(colors::Accent));
            if (ImGui::Button("Reset All Settings", { btn_avail.x - 10.f, 28.f }))
                m_reset_popup_open = true;
            ImGui::PopStyleColor(3);
        }

        if (m_reset_popup_open) {
            ImGui::OpenPopup("Reset?");
            m_reset_popup_open = false;
        }
        if (ImGui::BeginPopupModal("Reset?", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
        {
            ImDrawList* m_dl = ImGui::GetWindowDrawList();
            ImVec2 mp = ImGui::GetWindowPos();
            ImVec2 ms = ImGui::GetWindowSize();

            fx::gradient_rect_v(m_dl, { mp, mp + ms },
                colors::PanelBg, colors::Surface, 8.f);
            m_dl->AddRectFilled(mp, { mp.x + ms.x, mp.y + 2.f },
                ImGui::GetColorU32(colors::AccentDim));

            ImFont* f = fonts::regular();
            if (f) {
                ImGui::SetCursorPosY(16.f);
                ImGui::PushFont(f);
                ImGui::SetCursorPosX((ms.x - ImGui::CalcTextSize("Reset ALL settings to defaults?").x) * 0.5f);
                ImGui::TextDisabled("Reset ALL settings to defaults?");
                ImGui::PopFont();
            }

            ImGui::Dummy({ 0.f, 4.f });
            ImGui::SetCursorPosX((ms.x - 208.f) * 0.5f);

            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(colors::WidgetBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImVec4(0.6f, 0.2f, 0.2f, 1.f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(colors::Red));
            if (ImGui::Button("Yes, Reset", { 100.f, 26.f })) {
                g_settings.reset();
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(colors::WidgetBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(colors::WidgetHover));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(colors::Accent));
            if (ImGui::Button("Cancel", { 100.f, 26.f }))
                ImGui::CloseCurrentPopup();
            ImGui::PopStyleColor(3);

            ImGui::EndPopup();
        }

        ImGui::Columns(1);
    }
};

inline Menu g_menu;