#pragma once
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
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
#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include "../nlohmann/json.hpp"
#include "types.h"
#include "settings.h"
#include "utils.h"
#include "projectile_trails.h"
#include "../helpers/fonts.h"

// ============================================================
//  Data model
// ============================================================

enum class GrenadeType { SMOKE = 0, MOLOTOV, FRAG, FLASH };

inline const char* grenade_type_name(GrenadeType t) {
    switch (t) {
    case GrenadeType::SMOKE:   return "Smoke";
    case GrenadeType::MOLOTOV: return "Molotov";
    case GrenadeType::FRAG:    return "Frag";
    case GrenadeType::FLASH:   return "Flash";
    default:                   return "Unknown";
    }
}

inline ImU32 grenade_type_color(GrenadeType t) {
    switch (t) {
    case GrenadeType::SMOKE:   return IM_COL32(200, 200, 210, 255);
    case GrenadeType::MOLOTOV: return IM_COL32(255, 110, 40, 255);
    case GrenadeType::FRAG:    return IM_COL32(60, 220, 90, 255);
    case GrenadeType::FLASH:   return IM_COL32(255, 230, 80, 255);
    default:                   return IM_COL32(255, 255, 255, 255);
    }
}

struct AimPoint {
    std::string  label;
    GrenadeType  type = GrenadeType::SMOKE;
    float        pitch = 0.0f;
    float        yaw = 0.0f;
    std::string  throw_type;
};

struct GrenadeSpot {
    float                  x = 0, y = 0, z = 0;
    std::vector<AimPoint>  aim_points;
};

struct MapSpots {
    std::string              map_name;
    std::vector<GrenadeSpot> spots;
};

inline GrenadeType parse_grenade_type(const std::string& s) {
    if (s == "molotov") return GrenadeType::MOLOTOV;
    if (s == "frag")    return GrenadeType::FRAG;
    if (s == "flash")   return GrenadeType::FLASH;
    return GrenadeType::SMOKE;
}

inline const char* grenade_type_key(GrenadeType t) {
    switch (t) {
    case GrenadeType::MOLOTOV: return "molotov";
    case GrenadeType::FRAG:    return "frag";
    case GrenadeType::FLASH:   return "flash";
    default:                   return "smoke";
    }
}

// ============================================================
//  Main class
// ============================================================

class GrenadeHelper {
public:
    bool  popup_open = false;
    char  popup_label[128] = {};
    char  popup_throw_type[64] = {};
    int   popup_type = 0;
    char  import_buf[512] = {};
    float pending_px = 0, pending_py = 0, pending_pz = 0;
    float pending_pitch = 0, pending_yaw = 0;

    // Cache kedudukan terkini pemain
    float last_x = 0, last_y = 0, last_z = 0;
    float last_pitch = 0, last_yaw = 0;

    bool delete_popup_open = false;
    int  delete_map_idx = -1;
    int  delete_spot_idx = -1;

    std::string current_map;
    bool is_holding_grenade = false;
    GrenadeType held_grenade_type = GrenadeType::SMOKE;

    int get_loaded_maps_count() const { return (int)all_maps.size(); }

    void update_held_weapon(short weapon_def_index) {
        switch (weapon_def_index) {
        case 45:
            is_holding_grenade = true;
            held_grenade_type = GrenadeType::SMOKE;
            break;
        case 46:
        case 48:
            is_holding_grenade = true;
            held_grenade_type = GrenadeType::MOLOTOV;
            break;
        case 44:
            is_holding_grenade = true;
            held_grenade_type = GrenadeType::FRAG;
            break;
        case 43:
            is_holding_grenade = true;
            held_grenade_type = GrenadeType::FLASH;
            break;
        default:
            is_holding_grenade = false;
            break;
        }
    }

    void init(const std::string& path) {
        save_path = path;
        load();
    }

    void reload() { load(); }

    void open_add_current() {
        pending_px = last_x;
        pending_py = last_y;
        pending_pz = last_z;
        pending_pitch = last_pitch;
        pending_yaw = last_yaw;
        memset(popup_label, 0, sizeof(popup_label));
        memset(popup_throw_type, 0, sizeof(popup_throw_type));
        memset(import_buf, 0, sizeof(import_buf));
        popup_type = static_cast<int>(held_grenade_type);
        popup_open = true;
    }

    void update(float local_x, float local_y, float local_z,
        float view_pitch, float view_yaw,
        const std::string& map_name) {
        current_map = map_name;
        last_x = local_x; last_y = local_y; last_z = local_z;
        last_pitch = view_pitch; last_yaw = view_yaw;

        if (!g_settings.grenade_helper_enabled) return;

        if (g_settings.key_grenade_toggle > 0 && (GetAsyncKeyState(g_settings.key_grenade_toggle) & 1))
            g_settings.grenade_helper_visible = !g_settings.grenade_helper_visible;

        if (g_settings.key_grenade_add > 0 && (GetAsyncKeyState(g_settings.key_grenade_add) & 1)
            && !popup_open && !delete_popup_open) {
            open_add_current();
            g_settings.menu_open = true;
        }

        if (g_settings.key_grenade_delete > 0 && (GetAsyncKeyState(g_settings.key_grenade_delete) & 1)
            && !popup_open && !delete_popup_open) {
            find_active_spot(local_x, local_y, local_z, delete_map_idx, delete_spot_idx);
            if (delete_spot_idx >= 0) {
                delete_popup_open = true;
                g_settings.menu_open = true;
            }
        }
    }

    void render_popups() {
        render_add_popup();
        render_delete_popup();
    }

    void draw(ImDrawList* draw_list,
        float local_x, float local_y, float local_z,
        int sw, int sh) {
        if (!g_settings.grenade_helper_enabled || !g_settings.grenade_helper_visible || !is_holding_grenade) return;

        MapSpots* ms = find_map(current_map);
        if (!ms) return;

        ImFont* font = fonts::regular() ? fonts::regular() : ImGui::GetFont();
        float fs = g_settings.grenade_text_font_size;

        for (auto& spot : ms->spots) {
            if (!spot_passes_filter(spot)) continue;
            bool active = is_player_in_spot(spot, local_x, local_y, local_z);
            draw_ground_circle(draw_list, spot, active, sw, sh, font, fs);
            if (active) {
                for (const auto& ap : spot.aim_points) {
                    if (aimpoint_passes_filter(ap) && ap.type == held_grenade_type) {
                        draw_aim_point(draw_list, ap, local_x, local_y, local_z + 64.0f, sw, sh, font, fs);
                    }
                }
            }
        }
    }

    void set_view_matrix(const Matrix4x4& vm) { view_matrix_ = vm; }

    // ============================================================
    //  UI / Spot Manager Panel for Menu
    // ============================================================

    void render_spot_list() {
        MapSpots* ms = find_map(current_map);
        int spot_count = ms ? (int)ms->spots.size() : 0;

        ImGui::TextDisabled("Current Map: %s (%d Spots)",
            current_map.empty() ? "None" : current_map.c_str(), spot_count);

        if (ImGui::Button("+ Add Current Spot##btn_add_spot", { -1.f, 26.f })) {
            open_add_current();
        }

        ImGui::Spacing();
        if (!ms || ms->spots.empty()) {
            ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.f }, "There are no saved spots for this map");
            return;
        }

        ImGui::BeginChild("##spots_scroll_view", { 0.f, 180.f }, true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        int to_del_spot = -1, to_del_ap = -1;

        for (int si = 0; si < (int)ms->spots.size(); si++) {
            auto& spot = ms->spots[si];
            ImGui::PushID(si);

            char hdr[64];
            snprintf(hdr, sizeof(hdr), "Spot #%d (%d line-ups)###sp_%d", si + 1, (int)spot.aim_points.size(), si);

            if (ImGui::CollapsingHeader(hdr, ImGuiTreeNodeFlags_DefaultOpen)) {
                for (int ai = 0; ai < (int)spot.aim_points.size(); ai++) {
                    auto& ap = spot.aim_points[ai];
                    ImGui::PushID(ai);

                    ImU32 tc = grenade_type_color(ap.type);
                    ImGui::TextColored(
                        { ((tc >> 0) & 0xFF) / 255.f, ((tc >> 8) & 0xFF) / 255.f, ((tc >> 16) & 0xFF) / 255.f, 1.f },
                        "[%s]", grenade_type_name(ap.type));
                    ImGui::SameLine();
                    ImGui::Text("%s", ap.label.c_str());

                    ImGui::SameLine();
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.f);
                    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(180, 50, 50, 180));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(220, 60, 60, 255));
                    if (ImGui::SmallButton("X##del_ap")) {
                        to_del_spot = si;
                        to_del_ap = ai;
                    }
                    ImGui::PopStyleColor(2);

                    ImGui::TextDisabled("  └ %s (P: %.1f, Y: %.1f)",
                        ap.throw_type.empty() ? "Jumpthrow" : ap.throw_type.c_str(), ap.pitch, ap.yaw);

                    ImGui::PopID();
                }

                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 120));
                if (ImGui::SmallButton("Delete Spot")) {
                    to_del_spot = si;
                    to_del_ap = -1;
                }
                ImGui::PopStyleColor();
            }
            ImGui::PopID();
        }

        if (to_del_spot >= 0) {
            auto& spot = ms->spots[to_del_spot];
            if (to_del_ap >= 0) {
                spot.aim_points.erase(spot.aim_points.begin() + to_del_ap);
                if (spot.aim_points.empty())
                    ms->spots.erase(ms->spots.begin() + to_del_spot);
            }
            else {
                ms->spots.erase(ms->spots.begin() + to_del_spot);
            }

            if (ms->spots.empty()) {
                all_maps.erase(
                    std::remove_if(all_maps.begin(), all_maps.end(),
                        [](const MapSpots& m) { return m.spots.empty(); }),
                    all_maps.end());
            }
            save();
        }

        ImGui::EndChild();
    }

private:
    std::string           save_path;
    std::vector<MapSpots> all_maps;
    Matrix4x4             view_matrix_{};

    void load() {
        all_maps.clear();
        std::ifstream f(save_path);
        if (!f) return;
        try {
            nlohmann::json j; f >> j;
            for (const auto& mj : j["maps"]) {
                MapSpots ms;
                ms.map_name = mj["map"].get<std::string>();
                for (const auto& sj : mj["spots"]) {
                    GrenadeSpot spot;
                    spot.x = sj["x"]; spot.y = sj["y"]; spot.z = sj["z"];
                    for (const auto& aj : sj["aim_points"]) {
                        AimPoint ap;
                        ap.label = aj["label"].get<std::string>();
                        ap.type = parse_grenade_type(aj["type"]);
                        ap.pitch = aj["pitch"];
                        ap.yaw = aj["yaw"];
                        ap.throw_type = aj.value("throw_type", "Left click");
                        spot.aim_points.push_back(ap);
                    }
                    ms.spots.push_back(spot);
                }
                all_maps.push_back(ms);
            }
        }
        catch (...) {}
    }

    void save() {
        try {
            nlohmann::json j;
            j["maps"] = nlohmann::json::array();
            for (const auto& ms : all_maps) {
                nlohmann::json mj;
                mj["map"] = ms.map_name;
                mj["spots"] = nlohmann::json::array();
                for (const auto& spot : ms.spots) {
                    nlohmann::json sj;
                    sj["x"] = spot.x; sj["y"] = spot.y; sj["z"] = spot.z;
                    sj["aim_points"] = nlohmann::json::array();
                    for (const auto& ap : spot.aim_points) {
                        nlohmann::json aj;
                        aj["label"] = ap.label;
                        aj["type"] = grenade_type_key(ap.type);
                        aj["pitch"] = ap.pitch;
                        aj["yaw"] = ap.yaw;
                        aj["throw_type"] = ap.throw_type;
                        sj["aim_points"].push_back(aj);
                    }
                    mj["spots"].push_back(sj);
                }
                j["maps"].push_back(mj);
            }
            std::ofstream f(save_path);
            f << j.dump(2);
        }
        catch (...) {}
    }

    MapSpots* find_map(const std::string& name) {
        if (name.empty()) return nullptr;
        for (auto& ms : all_maps) {
            if (name.find(ms.map_name) != std::string::npos || ms.map_name.find(name) != std::string::npos)
                return &ms;
        }
        return nullptr;
    }

    MapSpots& get_or_create_map(const std::string& name) {
        for (auto& ms : all_maps)
            if (ms.map_name == name) return ms;
        all_maps.push_back({ name, {} });
        return all_maps.back();
    }

    bool is_player_in_spot(const GrenadeSpot& s, float px, float py, float pz) const {
        float dx = px - s.x, dy = py - s.y, dz = pz - s.z;
        float r = g_settings.grenade_circle_radius;
        return (dx * dx + dy * dy) <= r * r && fabsf(dz) < 128.0f;
    }

    void find_active_spot(float px, float py, float pz, int& out_map, int& out_spot) const {
        out_map = -1; out_spot = -1;
        for (int mi = 0; mi < (int)all_maps.size(); mi++) {
            for (int si = 0; si < (int)all_maps[mi].spots.size(); si++) {
                if (is_player_in_spot(all_maps[mi].spots[si], px, py, pz)) {
                    out_map = mi; out_spot = si; return;
                }
            }
        }
    }

    bool spot_passes_filter(const GrenadeSpot& s) const {
        for (const auto& ap : s.aim_points)
            if (aimpoint_passes_filter(ap) && ap.type == held_grenade_type)
                return true;
        return false;
    }

    bool aimpoint_passes_filter(const AimPoint& ap) const {
        switch (ap.type) {
        case GrenadeType::SMOKE:   return g_settings.grenade_filter_smoke;
        case GrenadeType::MOLOTOV: return g_settings.grenade_filter_molotov;
        case GrenadeType::FRAG:    return g_settings.grenade_filter_frag;
        case GrenadeType::FLASH:   return g_settings.grenade_filter_flash;
        default: return true;
        }
    }

    GrenadeSpot* find_nearby_spot(MapSpots& ms, float px, float py, float pz) {
        static constexpr float MERGE = 8.0f;
        for (auto& s : ms.spots) {
            float dx = px - s.x, dy = py - s.y, dz = pz - s.z;
            if (sqrtf(dx * dx + dy * dy + dz * dz) < MERGE) return &s;
        }
        return nullptr;
    }

    bool project(const Vec3& w, int sw, int sh, ImVec2& out) const {
        const float* m = &view_matrix_.m[0][0];
        float ww = m[12] * w.x + m[13] * w.y + m[14] * w.z + m[15];
        if (ww < 0.001f) return false;
        float inv = 1.f / ww;
        float x = m[0] * w.x + m[1] * w.y + m[2] * w.z + m[3];
        float y = m[4] * w.x + m[5] * w.y + m[6] * w.z + m[7];
        out.x = sw * 0.5f + x * inv * sw * 0.5f;
        out.y = sh * 0.5f - y * inv * sh * 0.5f;
        return true;
    }

    void draw_ground_circle(ImDrawList* dl, const GrenadeSpot& spot,
        bool active, int sw, int sh, ImFont* font, float fs) {
        static constexpr int SEGS = 32;
        static constexpr float LABEL_HEIGHT = 30.0f;

        float r = g_settings.grenade_circle_radius;
        std::vector<const AimPoint*> vis;
        for (const auto& ap : spot.aim_points)
            if (aimpoint_passes_filter(ap) && ap.type == held_grenade_type)
                vis.push_back(&ap);
        if (vis.empty()) return;

        ImU32 col = active
            ? float4_to_col(g_settings.grenade_circle_active_color)
            : float4_to_col(g_settings.grenade_circle_color);
        float thick = g_settings.grenade_circle_thickness;

        Vec3 ground_world = { spot.x, spot.y, spot.z };
        ImVec2 ground_screen;
        if (!project(ground_world, sw, sh, ground_screen)) return;

        Vec3 label_world = { spot.x, spot.y, spot.z + LABEL_HEIGHT };
        ImVec2 label_screen;
        bool label_visible = project(label_world, sw, sh, label_screen);

        std::vector<ImVec2> pts;
        pts.reserve(SEGS);
        for (int i = 0; i < SEGS; i++) {
            float a = (float)i / SEGS * 6.28318530f;
            Vec3 wp = { spot.x + r * cosf(a), spot.y + r * sinf(a), spot.z };
            ImVec2 sp;
            if (project(wp, sw, sh, sp))
                pts.push_back(sp);
        }
        if (pts.empty()) return;

        for (int i = 0; i < (int)pts.size(); i++)
            dl->AddLine(pts[i], pts[(i + 1) % (int)pts.size()], col, thick);

        if (!active && label_visible) {
            dl->AddLine(ground_screen, label_screen, apply_opacity(col, 0.6f), 1.0f);
            float ty = label_screen.y - 4.0f;
            for (int i = (int)vis.size() - 1; i >= 0; i--) {
                const AimPoint* ap = vis[i];
                ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0, ap->label.c_str());
                ty -= ts.y + 1.0f;
                float tx = label_screen.x - ts.x * 0.5f;
                dl->AddText(font, fs, { tx + 1, ty + 1 }, IM_COL32(0, 0, 0, 160), ap->label.c_str());
                dl->AddText(font, fs, { tx, ty }, active ? col : grenade_type_color(ap->type), ap->label.c_str());
            }
        }
    }

    void draw_aim_point(ImDrawList* dl, const AimPoint& ap,
        float cam_x, float cam_y, float cam_z,
        int sw, int sh, ImFont* font, float fs) {

        float pitch_r = ap.pitch * 3.14159265f / 180.0f;
        float yaw_r = ap.yaw * 3.14159265f / 180.0f;

        float cos_p = cosf(pitch_r);
        float dx = cos_p * cosf(yaw_r);
        float dy = cos_p * sinf(yaw_r);
        float dz = -sinf(pitch_r);

        Vec3 aim_world = { cam_x + dx * 8000.f, cam_y + dy * 8000.f, cam_z + dz * 8000.f };
        ImVec2 aim_screen;
        if (!project(aim_world, sw, sh, aim_screen)) return;

        const float margin = 16.f;
        aim_screen.x = std::clamp(aim_screen.x, margin, (float)sw - margin);
        aim_screen.y = std::clamp(aim_screen.y, margin, (float)sh - margin);

        ImVec2 screen_center = { sw * 0.5f, sh * 0.5f };
        ImU32 line_col = float4_to_col(g_settings.grenade_aim_line_color);
        ImU32 dot_col = grenade_type_color(ap.type);
        ImU32 txt_col = float4_to_col(g_settings.grenade_text_color);

        dl->AddLine(screen_center, aim_screen, apply_opacity(line_col, 0.8f), 1.0f);
        dl->AddCircleFilled(aim_screen, 3.5f, dot_col, 16);
        dl->AddCircle(aim_screen, 3.5f, IM_COL32(0, 0, 0, 200), 16, 1.2f);

        float side = (aim_screen.x < sw * 0.5f) ? -1.f : 1.f;
        float vert = (aim_screen.y < sh * 0.5f) ? 1.f : -1.f;
        static constexpr float SEG1 = 20.f, SEG2 = 60.f, C45 = 0.70710678f;

        ImVec2 p1 = { aim_screen.x + side * C45 * SEG1, aim_screen.y - vert * C45 * SEG1 };
        ImVec2 p2 = { p1.x + side * SEG2, p1.y };

        dl->AddLine(aim_screen, p1, apply_opacity(line_col, 0.9f), 1.3f);
        dl->AddLine(p1, p2, apply_opacity(line_col, 0.9f), 1.3f);

        const char* label_str = ap.label.c_str();
        const char* throw_str = ap.throw_type.empty() ? nullptr : ap.throw_type.c_str();

        ImVec2 lts = font->CalcTextSizeA(fs, FLT_MAX, 0, label_str);
        ImVec2 tts = throw_str ? font->CalcTextSizeA(fs, FLT_MAX, 0, throw_str) : ImVec2{ 0, 0 };

        float line_gap = 2.f;
        float total_h = lts.y + (throw_str ? tts.y + line_gap : 0.f);
        float max_w = std::max(lts.x, tts.x);

        float tx = (side > 0) ? p2.x + 4.f : p2.x - 4.f - max_w;
        float ty = p2.y - total_h * 0.5f;

        tx = std::clamp(tx, 2.f, (float)sw - max_w - 2.f);
        ty = std::clamp(ty, 2.f, (float)sh - total_h - 2.f);

        dl->AddText(font, fs, { tx + 1, ty + 1 }, IM_COL32(0, 0, 0, 180), label_str);
        dl->AddText(font, fs, { tx, ty }, txt_col, label_str);

        if (throw_str) {
            float ty2 = ty + lts.y + line_gap;
            ImU32 dim_col = apply_opacity(txt_col, 0.72f);
            dl->AddText(font, fs, { tx + 1, ty2 + 1 }, IM_COL32(0, 0, 0, 180), throw_str);
            dl->AddText(font, fs, { tx, ty2 }, dim_col, throw_str);
        }
    }

    void render_add_popup() {
        if (!popup_open) return;
        ImGui::OpenPopup("Custom Grenade Spot##nade_modal");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, { 0.5f, 0.5f });
        ImGui::SetNextWindowSize({ 420, 0 }, ImGuiCond_Always);

        if (!ImGui::BeginPopupModal("Custom Grenade Spot##nade_modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        ImGui::TextDisabled("Map: %s", current_map.c_str());
        ImGui::Text("Pos: %.1f, %.1f, %.1f", pending_px, pending_py, pending_pz);
        ImGui::Text("Ang: P: %.1f, Y: %.1f", pending_pitch, pending_yaw);
        ImGui::Separator();

        ImGui::Text("Label / Target:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##aplabel", popup_label, sizeof(popup_label));

        ImGui::Text("Grenade Type:");
        ImGui::RadioButton("Smoke", &popup_type, 0); ImGui::SameLine();
        ImGui::RadioButton("Molotov", &popup_type, 1); ImGui::SameLine();
        ImGui::RadioButton("Frag", &popup_type, 2); ImGui::SameLine();
        ImGui::RadioButton("Flash", &popup_type, 3);

        ImGui::Text("Throw Action (e.g. Jumpthrow, Run+Throw):");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##throwtype", popup_throw_type, sizeof(popup_throw_type));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool can_save = popup_label[0] != 0;
        if (!can_save) ImGui::BeginDisabled();
        if (ImGui::Button("Save Spot", { 120, 26 })) {
            AimPoint ap;
            ap.label = popup_label;
            ap.type = static_cast<GrenadeType>(popup_type);
            ap.pitch = pending_pitch;
            ap.yaw = pending_yaw;
            ap.throw_type = popup_throw_type[0] ? popup_throw_type : "Left click";

            MapSpots& ms = get_or_create_map(current_map);
            GrenadeSpot* nearby = find_nearby_spot(ms, pending_px, pending_py, pending_pz);
            if (nearby) {
                nearby->aim_points.push_back(ap);
            }
            else {
                GrenadeSpot sp;
                sp.x = pending_px; sp.y = pending_py; sp.z = pending_pz;
                sp.aim_points.push_back(ap);
                ms.spots.push_back(sp);
            }
            save();
            popup_open = false;
        }
        if (!can_save) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 100, 26 })) popup_open = false;

        ImGui::EndPopup();
    }

    void render_delete_popup() {
        if (!delete_popup_open) return;
        ImGui::OpenPopup("Delete Spot?##confirm_del");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, { 0.5f, 0.5f });

        if (!ImGui::BeginPopupModal("Delete Spot?##confirm_del", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        if (delete_map_idx >= 0 && delete_map_idx < (int)all_maps.size() &&
            delete_spot_idx >= 0 && delete_spot_idx < (int)all_maps[delete_map_idx].spots.size()) {

            ImGui::Text("Are you sure you want to delete this spot?");
            ImGui::Spacing();
            if (ImGui::Button("Yes, Delete", { 100, 24 })) {
                all_maps[delete_map_idx].spots.erase(all_maps[delete_map_idx].spots.begin() + delete_spot_idx);
                save();
                delete_popup_open = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelled", { 100, 24 })) delete_popup_open = false;
        }
        else {
            delete_popup_open = false;
        }
        ImGui::EndPopup();
    }
};

inline GrenadeHelper g_grenades;