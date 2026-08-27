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
    case GrenadeType::SMOKE:   return IM_COL32(180, 180, 180, 255);
    case GrenadeType::MOLOTOV: return IM_COL32(255, 120, 30, 255);
    case GrenadeType::FRAG:    return IM_COL32(80, 220, 80, 255);
    case GrenadeType::FLASH:   return IM_COL32(255, 255, 100, 255);
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

// ============================================================
//  JSON helpers
// ============================================================

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
    // ---- popup state ----
    bool  popup_open = false;
    char  popup_label[128] = {};
    char  popup_throw_type[64] = {};
    int   popup_type = 0;
    char  import_buf[512] = {};
    float pending_px = 0, pending_py = 0, pending_pz = 0;
    float pending_pitch = 0, pending_yaw = 0;

    // ---- delete confirmation state ----
    bool delete_popup_open = false;
    int  delete_map_idx = -1;
    int  delete_spot_idx = -1;

    // current map forwarded from main loop
    std::string current_map;

    int get_loaded_maps_count() const { 
        return (int)all_maps.size(); 
    }

    bool is_holding_grenade = false;
    GrenadeType held_grenade_type = GrenadeType::SMOKE;

    void update_held_weapon(short weapon_def_index) {
        switch (weapon_def_index) {
        case 45: // Smoke
            is_holding_grenade = true;
            held_grenade_type = GrenadeType::SMOKE;
            break;
        case 46: // Molotov
        case 48: // Incendiary
            is_holding_grenade = true;
            held_grenade_type = GrenadeType::MOLOTOV;
            break;
        case 44: // HE Grenade
            is_holding_grenade = true;
            held_grenade_type = GrenadeType::FRAG;
            break;
        case 43: // Flashbang
            is_holding_grenade = true;
            held_grenade_type = GrenadeType::FLASH;
            break;
        default: // Senjata lain (Pistol, Rifle, Pisau dll)
            is_holding_grenade = false;
            break;
        }
    }

    // ----------------------------------------------------------------

    void init(const std::string& path) {
        save_path = path;
        load();
    }

    void reload() {
        load();
    }

    // Call every frame (before ImGui render). Handles key presses.
    void update(float local_x, float local_y, float local_z,
        float view_pitch, float view_yaw,
        const std::string& map_name) {
        current_map = map_name;
        if (!g_settings.grenade_helper_enabled) return;

        // Toggle visibility
        if (g_settings.key_grenade_toggle > 0 && (GetAsyncKeyState(g_settings.key_grenade_toggle) & 1))
        g_settings.grenade_helper_visible = !g_settings.grenade_helper_visible;

        // Add spot — also opens the menu so overlay is interactive
        if ((GetAsyncKeyState(g_settings.key_grenade_add) & 1)
            && !popup_open && !delete_popup_open) {
            pending_px = local_x;
            pending_py = local_y;
            pending_pz = local_z;
            pending_pitch = view_pitch;
            pending_yaw = view_yaw;
            memset(popup_label, 0, sizeof(popup_label));
            memset(popup_throw_type, 0, sizeof(popup_throw_type));
            memset(import_buf, 0, sizeof(import_buf));
            popup_type = 0;
            popup_open = true;
            g_settings.menu_open = true;   // make overlay interactive
        }

        // Delete spot
        if ((GetAsyncKeyState(g_settings.key_grenade_delete) & 1)
            && !popup_open && !delete_popup_open) {
            find_active_spot(local_x, local_y, local_z,
                delete_map_idx, delete_spot_idx);
            if (delete_spot_idx >= 0) {
                delete_popup_open = true;
                g_settings.menu_open = true;
            }
        }
    }

    // Call inside ImGui render pass
    void render_popups() {
        render_add_popup();
        render_delete_popup();
    }

    // Call in draw pass (background draw list)
    void draw(ImDrawList* draw_list,
        float local_x, float local_y, float local_z,
        int sw, int sh) {
        if (!g_settings.grenade_helper_enabled ||
            !g_settings.grenade_helper_visible) return;

        if (!is_holding_grenade) return;

        MapSpots* ms = find_map(current_map);
        if (!ms) return;

        ImFont* font = fonts::regular() ? fonts::regular() : ImGui::GetFont();        float   fs = g_settings.grenade_text_font_size;

        for (auto& spot : ms->spots) {
            if (!spot_passes_filter(spot)) continue;
            bool active = is_player_in_spot(spot, local_x, local_y, local_z);
            draw_ground_circle(draw_list, spot, active, sw, sh, font, fs);
            if (active)
                for (const auto& ap : spot.aim_points)
                    if (aimpoint_passes_filter(ap) && ap.type == held_grenade_type)
                        draw_aim_point(draw_list, ap,
                            local_x, local_y, local_z + 64.0f,
                            sw, sh, font, fs);
        }
    }

    // Must be called each frame before draw()
    void set_view_matrix(const Matrix4x4& vm) { view_matrix_ = vm; }

    // ============================================================
    //  Popups
    // ============================================================

    void render_add_popup() {
        if (!popup_open) return;
        ImGui::OpenPopup("Add Grenade Spot##nade");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, { 0.5f, 0.5f });
        ImGui::SetNextWindowSize({ 440, 0 }, ImGuiCond_Always);

        if (!ImGui::BeginPopupModal("Add Grenade Spot##nade", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) return;

        ImGui::Text("Map: %s", current_map.c_str());
        ImGui::Text("Position: %.2f  %.2f  %.2f",
            pending_px, pending_py, pending_pz);
        ImGui::Text("Angles:   pitch %.2f  yaw %.2f",
            pending_pitch, pending_yaw);
        ImGui::Separator();

        // --- Import ---
        ImGui::Text("Import from setpos/setang:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(
                "You can find coordinates on csnades.gg\n\n"
                "Supported formats:\n"
                "  setpos X Y Z;setang PITCH YAW ROLL\n"
                "  setpos X Y Z;\n"
                "  setang PITCH YAW ROLL\n"
                "(semicolon or newline between the two commands)");

        ImGui::SetNextItemWidth(-70);
        ImGui::InputTextMultiline("##import", import_buf, sizeof(import_buf),
            { -70, 52 });
        ImGui::SameLine();
        if (ImGui::Button("Parse\n##imp", { 62, 52 }))
            parse_setpos_setang(import_buf,
                pending_px, pending_py, pending_pz,
                pending_pitch, pending_yaw);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Parse pasted setpos/setang to fill position and angles.");

        ImGui::TextDisabled(
            "e.g.  setpos -507.98 -662.06 184.60;setang -4.64 84.62 0.00");
        ImGui::Separator();

        // --- Aim point details ---
        ImGui::Text("Aim point label:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##aplabel", popup_label, sizeof(popup_label));

        ImGui::Text("Grenade type:");
        ImGui::RadioButton("Smoke##apt", &popup_type, 0); ImGui::SameLine();
        ImGui::RadioButton("Molotov##apt", &popup_type, 1); ImGui::SameLine();
        ImGui::RadioButton("Frag##apt", &popup_type, 2); ImGui::SameLine();
        ImGui::RadioButton("Flash##apt", &popup_type, 3);

        ImGui::Text("Throw type:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##throwtype", popup_throw_type, sizeof(popup_throw_type));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("e.g.  Jump throw,  D+Jump throw,  Left click");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool can_save = popup_label[0] != 0;
        if (!can_save) ImGui::BeginDisabled();
        if (ImGui::Button("Save##nadesave", { 120, 0 })) {
            AimPoint ap;
            ap.label = popup_label;
            ap.type = static_cast<GrenadeType>(popup_type);
            ap.pitch = pending_pitch;
            ap.yaw = pending_yaw;
            ap.throw_type = popup_throw_type;

            MapSpots& ms = get_or_create_map(current_map);
            GrenadeSpot* nearby_spot = find_nearby_spot(ms,
                pending_px, pending_py, pending_pz);
            if (nearby_spot) {
                nearby_spot->aim_points.push_back(ap);
            }
            else {
                GrenadeSpot spot;
                spot.x = pending_px; spot.y = pending_py; spot.z = pending_pz;
                spot.aim_points.push_back(ap);
                ms.spots.push_back(spot);
            }
            save();
            popup_open = false;
        }
        if (!can_save) {
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextColored({ 1, 0.4f, 0.4f, 1 }, "Label required");
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##nadecancel", { 120, 0 }))
            popup_open = false;

        ImGui::EndPopup();
    }

    void render_delete_popup() {
        if (!delete_popup_open) return;
        ImGui::OpenPopup("Delete Spot?##nadedel");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, { 0.5f, 0.5f });

        if (!ImGui::BeginPopupModal("Delete Spot?##nadedel", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) return;

        if (delete_map_idx >= 0 && delete_map_idx < (int)all_maps.size() &&
            delete_spot_idx >= 0 &&
            delete_spot_idx < (int)all_maps[delete_map_idx].spots.size()) {

            auto& spot = all_maps[delete_map_idx].spots[delete_spot_idx];
            ImGui::Text("Delete this spot and all its aim points?");
            ImGui::Spacing();
            for (const auto& ap : spot.aim_points)
                ImGui::BulletText("%s  (%s)", ap.label.c_str(),
                    grenade_type_name(ap.type));
            ImGui::Spacing();
            ImGui::Separator();

            if (ImGui::Button("Delete##confirmdel", { 120, 0 })) {
                all_maps[delete_map_idx].spots.erase(
                    all_maps[delete_map_idx].spots.begin() + delete_spot_idx);
                all_maps.erase(
                    std::remove_if(all_maps.begin(), all_maps.end(),
                        [](const MapSpots& m) { return m.spots.empty(); }),
                    all_maps.end());
                save();
                delete_popup_open = false;
                delete_map_idx = -1;
                delete_spot_idx = -1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel##canceldel", { 120, 0 })) {
                delete_popup_open = false;
                delete_map_idx = -1;
                delete_spot_idx = -1;
            }
        }
        else {
            delete_popup_open = false;
        }
        ImGui::EndPopup();
    }

    // ============================================================
    //  Menu spot list
    // ============================================================

    void render_spot_list() {
        if (ImGui::Button("Reload JSON Data")) {
            load();
        }
        ImGui::SameLine();
        ImGui::Text("Loaded Maps: %d", (int)all_maps.size());
        ImGui::Separator();

        MapSpots* ms = find_map(current_map);
        if (!ms || ms->spots.empty()) {
            ImGui::TextColored({ 0.5f,0.5f,0.5f,1 }, "No spots for %s",
                current_map.empty() ? "(not in game)"
                : current_map.c_str());
            return;
        }

        ImGui::Text("Spots on %s  (%d)", current_map.c_str(),
            (int)ms->spots.size());
        ImGui::Separator();

        int to_delete_spot = -1, to_delete_ap = -1;

        for (int si = 0; si < (int)ms->spots.size(); si++) {
            auto& spot = ms->spots[si];
            ImGui::PushID(si);
            char header[64];
            snprintf(header, sizeof(header),
                "Spot %d  (%.0f, %.0f, %.0f)###spot%d",
                si + 1, spot.x, spot.y, spot.z, si);

            if (ImGui::CollapsingHeader(header)) {
                for (int ai = 0; ai < (int)spot.aim_points.size(); ai++) {
                    auto& ap = spot.aim_points[ai];
                    ImGui::PushID(ai);
                    ImU32 tc = grenade_type_color(ap.type);
                    ImGui::TextColored(
                        { ((tc >> 0) & 0xFF) / 255.f,
                          ((tc >> 8) & 0xFF) / 255.f,
                          ((tc >> 16) & 0xFF) / 255.f, 1.f },
                        "[%s]", grenade_type_name(ap.type));
                    ImGui::SameLine();
                    ImGui::Text("%s", ap.label.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("— %s  (p:%.1f y:%.1f)",
                        ap.throw_type.c_str(),
                        ap.pitch, ap.yaw);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("X##delap")) {
                        to_delete_spot = si; to_delete_ap = ai;
                    }
                    ImGui::PopID();
                }
                ImGui::Spacing();
                if (ImGui::SmallButton("Delete entire spot##delspot"))
                    to_delete_spot = si;
            }
            ImGui::PopID();
        }

        if (to_delete_spot >= 0) {
            auto& spot = ms->spots[to_delete_spot];
            if (to_delete_ap >= 0) {
                spot.aim_points.erase(spot.aim_points.begin() + to_delete_ap);
                if (spot.aim_points.empty())
                    ms->spots.erase(ms->spots.begin() + to_delete_spot);
            }
            else {
                ms->spots.erase(ms->spots.begin() + to_delete_spot);
            }
            if (ms->spots.empty())
                all_maps.erase(
                    std::remove_if(all_maps.begin(), all_maps.end(),
                        [](const MapSpots& m) { return m.spots.empty(); }),
                    all_maps.end());
            save();
        }
    }

private:
    std::string           save_path;
    std::vector<MapSpots> all_maps;
    Matrix4x4             view_matrix_{};

    // ============================================================
    //  Persistence
    // ============================================================

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
                        ap.throw_type = aj["throw_type"].get<std::string>();
                        spot.aim_points.push_back(ap);
                    }
                    ms.spots.push_back(spot);
                }
                all_maps.push_back(ms);
            }
            printf("[+] Loaded grenade spots (%d maps)\n", (int)all_maps.size());
        }
        catch (const std::exception& e) {
            printf("[!] grenades.json load error: %s\n", e.what());
        }
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
        catch (const std::exception& e) {
            printf("[!] grenades.json save error: %s\n", e.what());
        }
    }

    // ============================================================
    //  Helpers
    // ============================================================

    MapSpots* find_map(const std::string& name) {
        if (name.empty()) return nullptr;
        for (auto& ms : all_maps) {
            // Periksa jika nama map mengandungi teks kunci (cth: "de_mirage")
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

    bool is_player_in_spot(const GrenadeSpot& s,
        float px, float py, float pz) const {
        float dx = px - s.x, dy = py - s.y, dz = pz - s.z;
        float r = g_settings.grenade_circle_radius;
        return (dx * dx + dy * dy) <= r * r && fabsf(dz) < 128.0f;
    }

    void find_active_spot(float px, float py, float pz,
        int& out_map, int& out_spot) const {
        out_map = -1; out_spot = -1;
        for (int mi = 0; mi < (int)all_maps.size(); mi++)
            for (int si = 0; si < (int)all_maps[mi].spots.size(); si++)
                if (is_player_in_spot(all_maps[mi].spots[si], px, py, pz)) {
                    out_map = mi; out_spot = si; return;
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

    GrenadeSpot* find_nearby_spot(MapSpots& ms,
        float px, float py, float pz) {
        static constexpr float MERGE = 5.0f;
        for (auto& s : ms.spots) {
            float dx = px - s.x, dy = py - s.y, dz = pz - s.z;
            if (sqrtf(dx * dx + dy * dy + dz * dz) < MERGE) return &s;
        }
        return nullptr;
    }

    // ============================================================
    //  Projection
    // ============================================================

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

    // ============================================================
    //  Ground circle
    // ============================================================
    void draw_ground_circle(ImDrawList* dl, const GrenadeSpot& spot,
        bool active, int sw, int sh,
        ImFont* font, float fs) {
        static constexpr int   SEGS = 32;
        static constexpr float LABEL_HEIGHT = 30.0f; // world units above spot

        float r = g_settings.grenade_circle_radius;

        std::vector<const AimPoint*> vis;
        for (const auto& ap : spot.aim_points)
            if (aimpoint_passes_filter(ap) && ap.type == held_grenade_type)
                vis.push_back(&ap);
        if (vis.empty()) return;

        ImU32 col = active
            ? float4_to_col(g_settings.grenade_circle_active_color)
            : float4_to_col(g_settings.grenade_circle_color);
        ImU32 txt_col = float4_to_col(g_settings.grenade_text_color);
        float thick = g_settings.grenade_circle_thickness;

        // Project ground center for the line base
        Vec3   ground_world = { spot.x, spot.y, spot.z };
        ImVec2 ground_screen;
        if (!project(ground_world, sw, sh, ground_screen)) return;

        // Project the label anchor point — fixed world height above spot
        // This stays at a consistent screen position regardless of distance
        Vec3   label_world = { spot.x, spot.y, spot.z + LABEL_HEIGHT };
        ImVec2 label_screen;
        bool   label_visible = project(label_world, sw, sh, label_screen);

        // Draw circle ring
        std::vector<ImVec2> pts;
        pts.reserve(SEGS);
        for (int i = 0; i < SEGS; i++) {
            float  a = (float)i / SEGS * 6.28318530f;
            Vec3   wp = { spot.x + r * cosf(a), spot.y + r * sinf(a), spot.z };
            ImVec2 sp;
            if (project(wp, sw, sh, sp))
                pts.push_back(sp);
        }
        if (pts.empty()) return;

        for (int i = 0; i < (int)pts.size(); i++)
            dl->AddLine(pts[i], pts[(i + 1) % (int)pts.size()], col, thick);

        if (!active && label_visible) {
            // Line from ground center up to the world-space label anchor
            dl->AddLine(ground_screen, label_screen,
                apply_opacity(col, 0.6f), 1.0f);

            // Stack labels upward from the label anchor
            float ty = label_screen.y - 4.0f;
            for (int i = (int)vis.size() - 1; i >= 0; i--) {
                const AimPoint* ap = vis[i];
                ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0, ap->label.c_str());
                ty -= ts.y + 1.0f;
                float tx = label_screen.x - ts.x * 0.5f;
                dl->AddText(font, fs, { tx + 1, ty + 1 }, IM_COL32(0, 0, 0, 160), ap->label.c_str());
                dl->AddText(font, fs, { tx,   ty },
                    active ? col : grenade_type_color(ap->type),
                    ap->label.c_str());
            }
        }
    }

    // ============================================================
    //  Aim point
    // ============================================================
    void draw_aim_point(ImDrawList* dl, const AimPoint& ap,
        float cam_x, float cam_y, float cam_z,
        int sw, int sh, ImFont* font, float fs) {

        // --- CS2 coordinate correction ---
        // CS2 uses a left-handed system where yaw increases clockwise
        // and the forward direction at yaw=0 points along +X.
        // Standard trig has yaw=0 along +X, counter-clockwise positive.
        // Subtracting 90° aligns the CS2 yaw with the view matrix forward.
        float pitch_r = ap.pitch * 3.14159265f / 180.0f;
        float yaw_r = ap.yaw * 3.14159265f / 180.0f;

        // CS2 forward direction: yaw=0 points along +X, yaw=90 along +Y.
        // Pitch positive = down, so negate for standard sin convention.
        float cos_p = cosf(pitch_r);
        float dx = cos_p * cosf(yaw_r);
        float dy = cos_p * sinf(yaw_r);
        float dz = -sinf(pitch_r);

        Vec3 aim_world = {
            cam_x + dx * 8000.f,
            cam_y + dy * 8000.f,
            cam_z + dz * 8000.f,
        };

        ImVec2 aim_screen;
        if (!project(aim_world, sw, sh, aim_screen)) return;

        const float margin = 16.f;
        aim_screen.x = std::clamp(aim_screen.x, margin, (float)sw - margin);
        aim_screen.y = std::clamp(aim_screen.y, margin, (float)sh - margin);

        ImVec2 screen_center = { sw * 0.5f, sh * 0.5f };

        ImU32 line_col = float4_to_col(g_settings.grenade_aim_line_color);
        ImU32 dot_col = grenade_type_color(ap.type);
        ImU32 txt_col = float4_to_col(g_settings.grenade_text_color);

        // Dynamic line: screen center → aim dot
        dl->AddLine(screen_center, aim_screen,
            apply_opacity(line_col, 0.8f), 1.0f);

        // Dot (smaller: radius 3)
        dl->AddCircleFilled(aim_screen, 3.0f, dot_col, 16);
        dl->AddCircle(aim_screen, 3.0f, IM_COL32(0, 0, 0, 200), 16, 1.2f);

        // Static two-segment label line
        // Choose side based on which half of screen the dot is in
        float side = (aim_screen.x < sw * 0.5f) ? -1.f : 1.f;
        float vert = (aim_screen.y < sh * 0.5f) ? 1.f : -1.f;

        static constexpr float SEG1 = 20.f;   // diagonal length
        static constexpr float SEG2 = 60.f;   // horizontal length
        static constexpr float C45 = 0.70710678f;

        // Segment 1: 45° away from center
        ImVec2 p1 = {
            aim_screen.x + side * C45 * SEG1,
            aim_screen.y - vert * C45 * SEG1,
        };
        // Segment 2: purely horizontal
        ImVec2 p2 = { p1.x + side * SEG2, p1.y };

        dl->AddLine(aim_screen, p1, apply_opacity(line_col, 0.9f), 1.3f);
        dl->AddLine(p1, p2, apply_opacity(line_col, 0.9f), 1.3f);

        // Text: label on line 1, throw_type on line 2
        const char* label_str = ap.label.c_str();
        const char* throw_str = ap.throw_type.empty() ? nullptr
            : ap.throw_type.c_str();

        ImVec2 lts = font->CalcTextSizeA(fs, FLT_MAX, 0, label_str);
        ImVec2 tts = throw_str
            ? font->CalcTextSizeA(fs, FLT_MAX, 0, throw_str)
            : ImVec2{ 0, 0 };

        float line_gap = 2.f;
        float total_h = lts.y + (throw_str ? tts.y + line_gap : 0.f);
        float max_w = std::max(lts.x, tts.x);

        // Anchor text to p2, horizontally on the far side
        float tx = (side > 0) ? p2.x + 4.f : p2.x - 4.f - max_w;
        float ty = p2.y - total_h * 0.5f;

        tx = std::clamp(tx, 2.f, (float)sw - max_w - 2.f);
        ty = std::clamp(ty, 2.f, (float)sh - total_h - 2.f);

        // Label (full brightness)
        dl->AddText(font, fs, { tx + 1, ty + 1 }, IM_COL32(0, 0, 0, 180), label_str);
        dl->AddText(font, fs, { tx,   ty }, txt_col, label_str);

        // Throw type (slightly dimmer, second line)
        if (throw_str) {
            float ty2 = ty + lts.y + line_gap;
            ImU32 dim_col = apply_opacity(txt_col, 0.72f);
            dl->AddText(font, fs, { tx + 1, ty2 + 1 }, IM_COL32(0, 0, 0, 180), throw_str);
            dl->AddText(font, fs, { tx,   ty2 }, dim_col, throw_str);
        }
    }

    // ============================================================
    //  setpos / setang parser
    //  Handles both formats:
    //    setpos X Y Z;setang P Y R       (single line, semicolon separator)
    //    setpos X Y Z;\nsetang P Y R     (two lines)
    // ============================================================

    static void parse_setpos_setang(const char* buf,
        float& x, float& y, float& z,
        float& pitch, float& yaw) {
        char tmp[512];
        strncpy(tmp, buf, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = 0;
        // Normalise: replace ';' with '\n'
        for (char* p = tmp; *p; p++) if (*p == ';') *p = '\n';

        const char* p = tmp;
        while (*p) {
            while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
            if (strncmp(p, "setpos", 6) == 0) {
                p += 6;
                float a = 0, b = 0, c = 0;
                if (sscanf(p, " %f %f %f", &a, &b, &c) == 3) {
                    x = a;
                    y = b;
                    z = c - 64.0f;
                }
            }
            else if (strncmp(p, "setang", 6) == 0) {
                p += 6;
                float a = 0, b = 0, c = 0;
                if (sscanf(p, " %f %f %f", &a, &b, &c) == 3) {
                    pitch = a; yaw = b;
                }
            }
            while (*p && *p != '\n') p++;
        }
    }
};

inline GrenadeHelper g_grenades;