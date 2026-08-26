#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../imgui/imgui.h"
#include <vector>
#include <string>
#include <cstdint>
#include "offsets.h"
#include "settings.h"
#include "entity_utils.h"
#include "../helpers/fonts.h"
#include "../helpers/icons.h"
#include "../theme/colors.h"

class SpectatorList {
public:
    std::vector<std::string> spectators;

    void update(uintptr_t entity_list, uintptr_t local_pawn, uintptr_t local_controller) {
        spectators.clear();
        if (!entity_list || !local_pawn) return;

        uint32_t local_handle_player = 0;
        uint32_t local_handle_pawn = 0;
        if (local_controller) {
            local_handle_player = *reinterpret_cast<const uint32_t*>(
                local_controller + g_offsets.CCSPlayerController.m_hPlayerPawn);
            local_handle_pawn = *reinterpret_cast<const uint32_t*>(
                local_controller + g_offsets.CCSPlayerController.m_hPawn);
        }

        uintptr_t first_page = *reinterpret_cast<const uintptr_t*>(
            entity_list + EntityList::PAGE_HEADER);
        if (!first_page) return;

        for (int i = 1; i < EntityList::MAX_PLAYERS; i++) {
            uintptr_t controller = *reinterpret_cast<const uintptr_t*>(
                first_page + EntityList::ENTRY_STRIDE * (i & EntityList::INDEX_MASK));
            if (!controller || controller == local_controller) continue;

            uint32_t player_pawn_h = *reinterpret_cast<const uint32_t*>(
                controller + g_offsets.CCSPlayerController.m_hPlayerPawn);

            uintptr_t player_pawn = 0;
            if (player_pawn_h && player_pawn_h != 0xFFFFFFFF) {
                player_pawn = EntityList::resolve_handle(entity_list, player_pawn_h);
                if (player_pawn) {
                    int health = *reinterpret_cast<const int*>(player_pawn + g_offsets.C_BaseEntity.m_iHealth);
                    if (health > 0) {
                        continue;
                    }
                }
            }

            uint32_t observer_pawn_h = *reinterpret_cast<const uint32_t*>(
                controller + g_offsets.CCSPlayerController.m_hObserverPawn);

            uintptr_t obs_pawn = 0;
            if (observer_pawn_h && observer_pawn_h != 0xFFFFFFFF) {
                obs_pawn = EntityList::resolve_handle(entity_list, observer_pawn_h);
            }
            if (!obs_pawn && player_pawn) {
                obs_pawn = player_pawn;
            }

            if (!obs_pawn || obs_pawn == local_pawn) continue;

            uintptr_t obs_svc = *reinterpret_cast<const uintptr_t*>(
                obs_pawn + g_offsets.C_BasePlayerPawn.m_pObserverServices);
            if (!obs_svc) continue;

            uint32_t obs_target_h = *reinterpret_cast<const uint32_t*>(
                obs_svc + g_offsets.CPlayer_ObserverServices.m_hObserverTarget);
            if (!obs_target_h || obs_target_h == 0xFFFFFFFF) continue;

            uintptr_t resolved_target_pawn = EntityList::resolve_handle(entity_list, obs_target_h);

            bool is_spectating = (resolved_target_pawn == local_pawn) ||
                (local_handle_player && obs_target_h == local_handle_player) ||
                (local_handle_pawn && obs_target_h == local_handle_pawn);

            if (is_spectating) {
                char name[128]{};
                read_player_name(controller, name, sizeof(name));
                if (name[0]) {
                    bool dup = false;
                    for (const auto& s : spectators) {
                        if (s == name) { dup = true; break; }
                    }
                    if (!dup) spectators.push_back(name);
                }
            }
        }
    }

    void draw(int screen_w) {
        if (!g_settings.draw_spectators || !g_settings.master_switch)
            return;

        float window_w = 260.0f;
        const float header_h = 36.f;

        float sx = g_settings.spec_x;
        float sy = g_settings.spec_y;
        if (sx <= 0.0f || sy <= 0.0f) {
            sx = (float)screen_w - window_w - 20.0f;
            sy = 60.0f;
            g_settings.spec_x = sx;
            g_settings.spec_y = sy;
        }

        ImGui::SetNextWindowPos({ sx, sy }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints({ window_w, header_h + 8.f }, { window_w, 1000.f });
        ImGui::SetNextWindowBgAlpha(0.88f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.08f, 0.88f));

        int flags = ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar;

        if (ImGui::Begin("##spectators", nullptr, flags)) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 w_origin = ImGui::GetWindowPos();
            ImVec2 w_size = ImGui::GetWindowSize();
            ImVec2 w_max = w_origin + w_size;

            // Garisan atas (Accent glow bar)
            dl->AddRectFilled(w_origin, { w_max.x, w_origin.y + 2.f }, IM_COL32(0, 220, 200, 160));
            dl->AddRectFilled({ w_origin.x, w_origin.y + 2.f }, { w_max.x, w_origin.y + 4.f }, IM_COL32(0, 220, 200, 60));

            ImFont* icon_font = fonts::icon();
            if (icon_font) {
                dl->AddText(icon_font, 14.f,
                    { w_origin.x + 12.f, w_origin.y + header_h * 0.5f - 7.f },
                    IM_COL32(0, 220, 200, 220), icons::target);
            }

            const char* title_str = "SPECTATORS";
            ImVec2 ts = ImGui::CalcTextSize(title_str);
            float title_x = icon_font ? 32.f : 14.f;
            dl->AddText({ w_origin.x + title_x, w_origin.y + header_h * 0.5f - ts.y * 0.5f },
                IM_COL32(220, 223, 230, 230), title_str);

            char count_s[8];
            snprintf(count_s, sizeof(count_s), "%zu", spectators.size());
            ImVec2 cs = ImGui::CalcTextSize(count_s);
            float badge_w = cs.x + 12.f;
            float badge_h = 18.f;
            float badge_y = w_origin.y + header_h * 0.5f - badge_h * 0.5f;
            ImVec2 b_min(w_max.x - badge_w - 10.f, badge_y);
            ImVec2 b_max(w_max.x - 10.f, badge_y + badge_h);
            dl->AddRectFilled(b_min, b_max, IM_COL32(0, 220, 200, 30), 9.f);
            dl->AddRect(b_min, b_max, IM_COL32(0, 220, 200, 80), 9.f);
            dl->AddText({ (b_min.x + b_max.x - cs.x) * 0.5f, (b_min.y + b_max.y - cs.y) * 0.5f },
                IM_COL32(0, 220, 200, 240), count_s);

            float sep_y = w_origin.y + header_h;
            dl->AddRectFilled({ w_origin.x + 10.f, sep_y }, { w_max.x - 10.f, sep_y + 1.f }, IM_COL32(36, 38, 52, 120));

            ImGui::SetCursorPos({ 12.f, header_h + 8.f });

            for (size_t i = 0; i < spectators.size(); ++i) {
                ImVec2 row_start = ImGui::GetCursorScreenPos();

                if (i % 2 == 1) {
                    dl->AddRectFilled(
                        { w_origin.x + 6.f, row_start.y - 1.f },
                        { w_max.x - 6.f, row_start.y + 20.f },
                        IM_COL32(255, 255, 255, 6), 4.f);
                }

                dl->AddCircleFilled({ w_origin.x + 16.f, row_start.y + 8.f }, 2.5f, IM_COL32(0, 220, 200, 180));
                dl->AddText({ w_origin.x + 26.f, row_start.y }, IM_COL32(220, 223, 230, 230), spectators[i].c_str());
                ImGui::Dummy({ 0.f, 20.f });
            }

            if (!spectators.empty()) {
                ImGui::Dummy({ 0.f, 6.f });
            }
            else {
                ImGui::Dummy({ 0.f, 2.f });
            }

            ImVec2 wp = ImGui::GetWindowPos();
            g_settings.spec_x = wp.x;
            g_settings.spec_y = wp.y;
        }
        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(4);
    }
};

inline SpectatorList g_spectators;