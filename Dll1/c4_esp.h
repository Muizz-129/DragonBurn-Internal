#pragma once
#include <Windows.h>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../imgui/imgui.h"
#include <string>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include "types.h"
#include "settings.h"
#include "offsets.h"
#include "entity_utils.h"
#include "../helpers/icons.h"
#include "../helpers/fonts.h"


struct PlantedC4State {
    uintptr_t entity = 0;
    Vec3      world_pos{};
    int       bomb_site = 0;
    bool      being_defused = false;
    float     defuse_end_time = 0.f;
    float     c4_blow_time = 0.f;
    float     time_left = 0.f;
    float     defuse_time_left = 0.f;
    bool      valid = false;
};

inline bool is_valid_ptr(uintptr_t ptr) {
    return (ptr >= 0x10000 && ptr < 0x00007FFFFFFFFFFF);
}

static float get_cur_time() {
    uintptr_t client = Offsets::get_client_base();
    if (!client || !g_offsets.client.dwGlobalVars) return 0.f;

    __try {
        uintptr_t gv_base = client + g_offsets.client.dwGlobalVars;
        uintptr_t gv_ptr = *reinterpret_cast<const uintptr_t*>(gv_base);
        uintptr_t final_gv = is_valid_ptr(gv_ptr) ? gv_ptr : gv_base;
        return *reinterpret_cast<const float*>(final_gv + 0x30); // 0x30 = flCurTime
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0.f;
    }
}

static bool entity_designer_name(uintptr_t entity, char* out, size_t max_len)
{
    if (!is_valid_ptr(entity) || max_len == 0) return false;
    out[0] = 0;

    __try {
        uintptr_t ident = *reinterpret_cast<const uintptr_t*>(entity + g_offsets.CEntityInstance.m_pEntity);
        if (!is_valid_ptr(ident)) return false;

        uintptr_t str_ptr = *reinterpret_cast<const uintptr_t*>(ident + g_offsets.CEntityIdentity.m_designerName);
        if (!is_valid_ptr(str_ptr)) return false;

        const char* src = reinterpret_cast<const char*>(str_ptr);
        size_t i = 0;
        for (; i < max_len - 1 && src[i] != '\0'; i++) {
            out[i] = src[i];
        }
        out[i] = '\0';
        return (i > 0);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static PlantedC4State read_planted_c4()
{
    PlantedC4State state{};
    uintptr_t client = Offsets::get_client_base();
    if (!client) return state;

    uintptr_t planted_c4_ent = 0;

    __try {
        if (g_offsets.client.dwPlantedC4) {
            uintptr_t raw = *reinterpret_cast<const uintptr_t*>(client + g_offsets.client.dwPlantedC4);
            if (is_valid_ptr(raw)) {
                planted_c4_ent = raw;
            }
        }

        if (!planted_c4_ent && g_offsets.client.dwEntityList) {
            uintptr_t entity_list = *reinterpret_cast<const uintptr_t*>(client + g_offsets.client.dwEntityList);
            if (is_valid_ptr(entity_list)) {
                for (uint32_t pg = 0; pg < 8; pg++) {
                    uintptr_t page_addr = EntityList::get_page(entity_list, pg * 512);
                    if (!is_valid_ptr(page_addr)) continue;

                    for (size_t slot = 1; slot < 512; slot++) {
                        uintptr_t ent = *reinterpret_cast<const uintptr_t*>(page_addr + EntityList::ENTRY_STRIDE * slot);
                        if (!is_valid_ptr(ent)) continue;

                        char name[33]{};
                        if (!entity_designer_name(ent, name, sizeof(name))) continue;

                        if (strcmp(name, "planted_c4") == 0 || strcmp(name, "c4_planted") == 0) {
                            planted_c4_ent = ent;
                            break;
                        }
                    }
                    if (planted_c4_ent) break;
                }
            }
        }

        if (!is_valid_ptr(planted_c4_ent)) return state;

        uint8_t ticking = 1;
        if (g_offsets.C4.m_bBombTicking) {
            ticking = *reinterpret_cast<const uint8_t*>(planted_c4_ent + g_offsets.C4.m_bBombTicking);
        }
        if (!ticking) return state;

        uintptr_t scene = *reinterpret_cast<const uintptr_t*>(planted_c4_ent + g_offsets.C_BaseEntity.m_pGameSceneNode);
        if (is_valid_ptr(scene)) {
            state.world_pos = *reinterpret_cast<const Vec3*>(scene + g_offsets.CGameSceneNode.m_vecAbsOrigin);
        }

        if (g_offsets.C4.m_nBombSite)
            state.bomb_site = *reinterpret_cast<const int*>(planted_c4_ent + g_offsets.C4.m_nBombSite);

        if (g_offsets.C4.m_bBeingDefused)
            state.being_defused = (*reinterpret_cast<const uint8_t*>(planted_c4_ent + g_offsets.C4.m_bBeingDefused) != 0);

        if (g_offsets.C4.m_flDefuseCountDown)
            state.defuse_end_time = *reinterpret_cast<const float*>(planted_c4_ent + g_offsets.C4.m_flDefuseCountDown);

        if (g_offsets.C4.m_flC4Blow)
            state.c4_blow_time = *reinterpret_cast<const float*>(planted_c4_ent + g_offsets.C4.m_flC4Blow);

        float cur_time = get_cur_time();
        state.time_left = (state.c4_blow_time > cur_time) ? (state.c4_blow_time - cur_time) : 0.0f;
        state.defuse_time_left = (state.being_defused && state.defuse_end_time > cur_time) ? (state.defuse_end_time - cur_time) : 0.0f;

        state.entity = planted_c4_ent;
        state.valid = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return state;
    }

    return state;
}

static void draw_c4_esp(ImDrawList* draw, const PlantedC4State& c4, int sw, int sh, const Matrix4x4& vm)
{
    if (!g_settings.c4_esp_enabled || !g_settings.master_switch || !c4.valid) return;

    ImVec2 screen;
    if (!w2s(c4.world_pos, vm, sw, sh, screen))
        return;

    ImU32 text_color = ImGui::GetColorU32({ 1.0f, 0.47f, 0.47f, 1.0f });
    ImU32 defuse_color = ImGui::GetColorU32({ 1.0f, 0.84f, 0.47f, 1.0f });
    float font_size = g_settings.esp_font_atlas_size;

    ImU32 shadow = IM_COL32(0, 0, 0, 230);
    auto draw_shadowed = [&](float fs, const char* txt, ImVec2 pos, ImU32 col) {
        ImVec2 ts = ImGui::CalcTextSize(txt);
        ImVec2 p(pos.x - ts.x * 0.5f, pos.y);
        draw->AddText({ p.x - 1, p.y - 1 }, shadow, txt);
        draw->AddText({ p.x + 1, p.y - 1 }, shadow, txt);
        draw->AddText({ p.x - 1, p.y + 1 }, shadow, txt);
        draw->AddText({ p.x + 1, p.y + 1 }, shadow, txt);
        draw->AddText(p, col, txt);
        };

    draw_shadowed(font_size, "PLANTED C4", { screen.x, screen.y - font_size - 4.f }, text_color);

    char site_label[32];
    snprintf(site_label, sizeof(site_label), "[%s]", (c4.bomb_site == 0) ? "A" : "B");
    draw_shadowed(font_size, site_label, { screen.x, screen.y + 2.f }, text_color);

    if (c4.being_defused && c4.defuse_time_left > 0.0f) {
        char buf[64];
        snprintf(buf, sizeof(buf), "DEFUSING (%.1fs)", c4.defuse_time_left);
        draw_shadowed(font_size, buf, { screen.x, screen.y + font_size + 6.f }, defuse_color);
    }
}

static void draw_bomb_timer(const PlantedC4State& c4)
{
    if (!g_settings.show_bomb_timer || !g_settings.master_switch)
        return;

    const float window_w = 260.f;
    const float header_h = 36.f;

    float bx = g_settings.bomb_x;
    float by = g_settings.bomb_y;
    if (bx < 0) { bx = 10.f; g_settings.bomb_x = bx; }
    if (by < 0) { by = 200.f; g_settings.bomb_y = by; }

    ImGui::SetNextWindowPos({ g_settings.bomb_x, g_settings.bomb_y }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({ window_w, header_h + 10.f }, { window_w, 600.f });
    ImGui::SetNextWindowBgAlpha(0.88f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.08f, 0.88f));

    ImGui::Begin("##bomb_timer", nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 w_origin = ImGui::GetWindowPos();
    ImVec2 w_size = ImGui::GetWindowSize();
    ImVec2 w_max = w_origin + w_size;

    dl->AddRectFilled(w_origin, { w_max.x, w_origin.y + 2.f }, IM_COL32(0, 220, 200, 160));
    dl->AddRectFilled({ w_origin.x, w_origin.y + 2.f }, { w_max.x, w_origin.y + 4.f }, IM_COL32(0, 220, 200, 60));

    ImFont* icon_font = fonts::icon();
    if (icon_font) {
        dl->AddText(icon_font, 14.f,
            { w_origin.x + 12.f, w_origin.y + header_h * 0.5f - 7.f },
            IM_COL32(220, 180, 80, 220), icons::target);
    }

    const char* title_str = "BOMB TIMER";
    ImVec2 ts = ImGui::CalcTextSize(title_str);
    float title_x = icon_font ? 32.f : 14.f;
    dl->AddText({ w_origin.x + title_x, w_origin.y + header_h * 0.5f - ts.y * 0.5f },
        IM_COL32(220, 223, 230, 230), title_str);

    float sep_y = w_origin.y + header_h;
    dl->AddRectFilled({ w_origin.x + 10.f, sep_y }, { w_max.x - 10.f, sep_y + 1.f }, IM_COL32(36, 38, 52, 120));

    ImGui::SetCursorPos({ 12.f, header_h + 10.f });

    if (c4.valid && c4.time_left > 0.0f) {
        float remaining = (std::clamp)(c4.time_left, 0.0f, 40.0f);
        float progress = remaining / 40.f;

        const char* site = (c4.bomb_site == 0) ? "A" : "B";
        char site_label[32];
        snprintf(site_label, sizeof(site_label), "Bomb on %s", site);

        ImVec2 sl_size = ImGui::CalcTextSize(site_label);
        ImGui::SetCursorPosX((window_w - sl_size.x) * 0.5f - 6.f);
        ImGui::TextDisabled("%s", site_label);

        char timer_buf[32];
        snprintf(timer_buf, sizeof(timer_buf), "%.2f s", remaining);
        ImVec2 timer_size = ImGui::CalcTextSize(timer_buf);
        ImGui::SetCursorPosX((window_w - timer_size.x) * 0.5f - 6.f);

        ImU32 timer_col;
        if (c4.being_defused && remaining > 5.f)
            timer_col = IM_COL32(32, 200, 190, 235);
        else if (remaining <= 10.f)
            timer_col = IM_COL32(230, 60, 70, 235);
        else
            timer_col = IM_COL32(220, 223, 230, 235);
        ImGui::TextColored(ImColor(timer_col), "%s", timer_buf);

        ImGui::Dummy({ 0.f, 6.f });
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float w = window_w - 24.f;
        float bar_h = 10.f;
        float bar_round = 5.f;

        ImU32 bg_bar = IM_COL32(22, 24, 36, 200);
        ImU32 fill_bar;
        if (c4.being_defused && remaining > 5.f)
            fill_bar = IM_COL32(32, 200, 190, 230);
        else if (remaining <= 10.f)
            fill_bar = IM_COL32(230, 60, 70, 230);
        else
            fill_bar = IM_COL32(0, 220, 200, 200);

        dl->AddRectFilled(pos, { pos.x + w, pos.y + bar_h }, bg_bar, bar_round);
        if (progress > 0.f)
            dl->AddRectFilled(pos, { pos.x + w * progress, pos.y + bar_h }, fill_bar, bar_round);
        ImGui::Dummy({ 0.f, bar_h + 4.f });

        if (c4.being_defused) {
            char def_buf[64];
            snprintf(def_buf, sizeof(def_buf), "Defusing: %.2f s", c4.defuse_time_left);
            ImVec2 def_size = ImGui::CalcTextSize(def_buf);
            ImGui::SetCursorPosX((window_w - def_size.x) * 0.5f - 6.f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.125f, 0.78f, 0.75f, 0.95f));
            ImGui::Text("%s", def_buf);
            ImGui::PopStyleColor();
        }
    }
    else {
        ImGui::Dummy({ 0.f, 4.f });
        const char* idle_str = "C4 not planted";
        ImVec2 is = ImGui::CalcTextSize(idle_str);
        float ix = (window_w - is.x) * 0.5f - 6.f;
        ImGui::SetCursorPosX(ix);
        ImGui::TextDisabled("%s", idle_str);
        ImGui::Dummy({ 0.f, 10.f });
    }

    ImGui::Dummy({ 0.f, 6.f });

    ImVec2 wp = ImGui::GetWindowPos();
    g_settings.bomb_x = wp.x;
    g_settings.bomb_y = wp.y;

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
}

struct DroppedC4State {
    uintptr_t entity = 0;
    Vec3      world_pos{};
    bool      valid = false;
};

inline DroppedC4State read_dropped_c4(bool bomb_planted) {
    DroppedC4State state{};
    if (bomb_planted) return state;

    uintptr_t client = Offsets::get_client_base();
    if (!client || !g_offsets.client.dwWeaponC4) return state;

    __try {
        uintptr_t raw_ptr = *reinterpret_cast<const uintptr_t*>(client + g_offsets.client.dwWeaponC4);
        if (!is_valid_ptr(raw_ptr)) return state;

        uintptr_t c4_entity = *reinterpret_cast<const uintptr_t*>(raw_ptr);
        if (!is_valid_ptr(c4_entity)) return state;

        uint32_t owner = *reinterpret_cast<const uint32_t*>(c4_entity + g_offsets.C_BaseEntity.m_hOwnerEntity);
        if (owner != 0 && owner != 0xFFFFFFFF) {
            return state;
        }

        uintptr_t scene = *reinterpret_cast<const uintptr_t*>(c4_entity + g_offsets.C_BaseEntity.m_pGameSceneNode);
        if (!is_valid_ptr(scene)) return state;

        uintptr_t parent = *reinterpret_cast<const uintptr_t*>(scene + 0x38);

        Vec3 pos = *reinterpret_cast<const Vec3*>(scene + g_offsets.CGameSceneNode.m_vecAbsOrigin);

        if (parent == 0 && (pos.x != 0.f || pos.y != 0.f || pos.z != 0.f)) {
            state.world_pos = pos;
            state.entity = c4_entity;
            state.valid = true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return state;
    }

    return state;
}

static void draw_dropped_c4_esp(ImDrawList* draw, const DroppedC4State& c4, int sw, int sh, const Matrix4x4& vm)
{
    if (!g_settings.c4_esp_enabled || !g_settings.master_switch || !c4.valid) return;

    ImVec2 screen;
    if (!w2s(c4.world_pos, vm, sw, sh, screen))
        return;

    float font_size = g_settings.esp_font_atlas_size;
    ImU32 col = IM_COL32(255, 165, 0, 230);
    ImU32 shadow = IM_COL32(0, 0, 0, 230);
    ImVec2 ts = ImGui::CalcTextSize("DROPPED C4");
    float lx = screen.x - ts.x * 0.5f;
    float ly = screen.y - ts.y - 4.f;
    draw->AddText({ lx - 1, ly - 1 }, shadow, "DROPPED C4");
    draw->AddText({ lx + 1, ly - 1 }, shadow, "DROPPED C4");
    draw->AddText({ lx - 1, ly + 1 }, shadow, "DROPPED C4");
    draw->AddText({ lx + 1, ly + 1 }, shadow, "DROPPED C4");
    draw->AddText({ lx, ly }, col, "DROPPED C4");
}