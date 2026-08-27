#pragma once
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include "settings.h"

class Config {
public:
    static bool save(const std::string& path) {
        std::ofstream f(path);
        if (!f) return false;

        f << "[ESP]\n";
        write(f, "master_switch", g_settings.master_switch);
        write(f, "esp_enabled", g_settings.esp_enabled);
        write(f, "draw_box", g_settings.draw_box);
        write(f, "draw_healthbar", g_settings.draw_healthbar);
        write(f, "draw_health_text", g_settings.draw_health_text);
        write(f, "draw_name", g_settings.draw_name);
        write(f, "draw_weapon", g_settings.draw_weapon);
        write(f, "draw_teammates", g_settings.draw_teammates);
        write(f, "draw_head", g_settings.draw_head);
        write(f, "chams_enabled", g_settings.chams_enabled);
        write(f, "chams_style", g_settings.chams_style);
        write(f, "esp_use_theme", g_settings.esp_use_theme);
        write_arr(f, "esp_theme_color", g_settings.esp_theme_color, 4);
        write(f, "esp_opacity_drop", g_settings.esp_opacity_drop);
        write(f, "esp_opacity_drop_start", g_settings.esp_opacity_drop_start);
        write(f, "esp_opacity_drop_end", g_settings.esp_opacity_drop_end);
        write(f, "esp_opacity_drop_min", g_settings.esp_opacity_drop_min);
        write(f, "c4_esp_enabled", g_settings.c4_esp_enabled);
        write(f, "show_bomb_timer", g_settings.show_bomb_timer);

        f << "\n[Font]\n";
        write(f, "esp_font_index", g_settings.esp_font_index);

        f << "\n[Name]\n";
        write(f, "name_position", g_settings.name_position);
        write(f, "name_offset_x", g_settings.name_offset_x);
        write(f, "name_offset_y", g_settings.name_offset_y);
        write_arr(f, "name_color", g_settings.name_color, 4);
        write_arr(f, "name_shadow_color", g_settings.name_shadow_color, 4);
        write(f, "name_shadow", g_settings.name_shadow);
        write(f, "name_font_size", g_settings.name_font_size);

        f << "\n[HealthText]\n";
        write_arr(f, "hp_text_color", g_settings.hp_text_color, 4);
        write_arr(f, "hp_text_shadow_color", g_settings.hp_text_shadow_color, 4);
        write(f, "hp_text_shadow", g_settings.hp_text_shadow);
        write(f, "hp_font_size", g_settings.hp_font_size);
        write(f, "healthbar_solid_color", g_settings.healthbar_solid_color);
        write_arr(f, "healthbar_color", g_settings.healthbar_color, 4);

        f << "\n[Weapon]\n";
        write_arr(f, "weapon_color", g_settings.weapon_color, 4);
        write_arr(f, "weapon_shadow_color", g_settings.weapon_shadow_color, 4);
        write_arr(f, "weapon_icon_color", g_settings.weapon_icon_color, 4);
        write(f, "weapon_shadow", g_settings.weapon_shadow);
        write(f, "weapon_font_size", g_settings.weapon_font_size);
        write(f, "weapon_show_icon", g_settings.weapon_show_icon);
        write(f, "weapon_show_text", g_settings.weapon_show_text);
        write(f, "weapon_distance_dropoff", g_settings.weapon_distance_dropoff);

        f << "\n[Spectators]\n";
        write(f, "draw_spectators", g_settings.draw_spectators);
        write(f, "spec_x", g_settings.spec_x);
        write(f, "spec_y", g_settings.spec_y);
        write(f, "bomb_x", g_settings.bomb_x);
        write(f, "bomb_y", g_settings.bomb_y);

        f << "\n[Radar]\n";
        write(f, "draw_radar", g_settings.draw_radar);
        write(f, "radar_circle", g_settings.radar_circle);
        write(f, "radar_rotate", g_settings.radar_rotate);
        write(f, "radar_rings", g_settings.radar_rings);
        write(f, "radar_names", g_settings.radar_names);
        write(f, "radar_names_font_size", g_settings.radar_names_font_size);
        write(f, "radar_size", g_settings.radar_size);
        write(f, "radar_zoom", g_settings.radar_zoom);
        write(f, "radar_bg_alpha", g_settings.radar_bg_alpha);
        write(f, "radar_x", g_settings.radar_x);
        write(f, "radar_y", g_settings.radar_y);
        write_arr(f, "radar_enemy_color", g_settings.radar_enemy_color, 4);
        write_arr(f, "radar_team_color", g_settings.radar_team_color, 4);

        f << "\n[Grenades]\n";
        write(f, "grenade_helper_enabled", g_settings.grenade_helper_enabled);
        write(f, "grenade_helper_visible", g_settings.grenade_helper_visible);
        write(f, "key_grenade_toggle", g_settings.key_grenade_toggle);
        write(f, "key_grenade_add", g_settings.key_grenade_add);
        write(f, "key_grenade_delete", g_settings.key_grenade_delete);
        write(f, "grenade_circle_radius", g_settings.grenade_circle_radius);
        write(f, "grenade_circle_thickness", g_settings.grenade_circle_thickness);
        write(f, "grenade_text_font_size", g_settings.grenade_text_font_size);
        write(f, "grenade_filter_smoke", g_settings.grenade_filter_smoke);
        write(f, "grenade_filter_molotov", g_settings.grenade_filter_molotov);
        write(f, "grenade_filter_frag", g_settings.grenade_filter_frag);
        write(f, "grenade_filter_flash", g_settings.grenade_filter_flash);
        write_arr(f, "grenade_circle_color", g_settings.grenade_circle_color, 4);
        write_arr(f, "grenade_circle_active_color", g_settings.grenade_circle_active_color, 4);
        write_arr(f, "grenade_aim_line_color", g_settings.grenade_aim_line_color, 4);
        write_arr(f, "grenade_text_color", g_settings.grenade_text_color, 4);

        f << "\n[Colors]\n";
        write_arr(f, "enemy_fill", g_settings.enemy_fill, 4);
        write_arr(f, "enemy_outline", g_settings.enemy_outline, 4);
        write_arr(f, "enemy_glow", g_settings.enemy_glow, 4);
        write_arr(f, "team_fill", g_settings.team_fill, 4);
        write_arr(f, "team_outline", g_settings.team_outline, 4);
        write_arr(f, "team_glow", g_settings.team_glow, 4);

        f << "\n[Body]\n";
        write(f, "body_width_scale", g_settings.body_width_scale);
        write(f, "head_radius", g_settings.head_radius);
        write(f, "depth_scale", g_settings.depth_scale);
        write(f, "glow_expand_outer", g_settings.glow_expand_outer);
        write(f, "glow_expand_inner", g_settings.glow_expand_inner);

        f << "\n[Misc]\n";
        write(f, "use_vsync", g_settings.use_vsync);
        write(f, "target_fps", g_settings.target_fps);

        f << "\n[Box]\n";
        write(f, "box_style", g_settings.box_style);
        write(f, "box_thickness", g_settings.box_thickness);
        write(f, "box_padding_x", g_settings.box_padding_x);
        write(f, "box_padding_y", g_settings.box_padding_y);
        write(f, "box_corner_pct", g_settings.box_corner_pct);

        f << "\n[Crosshair]\n";
        write(f, "crosshair_enabled", g_settings.crosshair_enabled);
        write(f, "crosshair_shape", g_settings.crosshair_shape);
        write(f, "crosshair_size", g_settings.crosshair_size);
        write(f, "crosshair_gap", g_settings.crosshair_gap);
        write(f, "crosshair_thickness", g_settings.crosshair_thickness);
        write_arr(f, "crosshair_color", g_settings.crosshair_color, 4);
        write(f, "crosshair_outline", g_settings.crosshair_outline);
        write(f, "crosshair_outline_thickness", g_settings.crosshair_outline_thickness);
        write_arr(f, "crosshair_outline_color", g_settings.crosshair_outline_color, 4);
        write(f, "crosshair_dot", g_settings.crosshair_dot);
        write(f, "crosshair_dot_size", g_settings.crosshair_dot_size);

        f << "\n[Menu]\n";
        write(f, "menu_x", g_settings.menu_x);
        write(f, "menu_y", g_settings.menu_y);

        f << "\n[KeyBinds]\n";
        write(f, "key_menu", g_settings.key_menu);
        write(f, "key_master", g_settings.key_master);
        write(f, "key_exit", g_settings.key_exit);

        f << "\n[MenuStyle]\n";
        write_arr(f, "menu_accent_color", g_settings.menu_accent_color, 4);
        write(f, "menu_bg_alpha", g_settings.menu_bg_alpha);
        write_arr(f, "menu_border_color", g_settings.menu_border_color, 4);
        write(f, "menu_font_index", g_settings.menu_font_index);
        write(f, "menu_font_size", g_settings.menu_font_size);

        f << "\n[Aimbot]\n";
        write(f, "aimbot_enabled", g_settings.aimbot_enabled);
        write(f, "aimbot_fov", g_settings.aimbot_fov);
        write(f, "key_aimbot", g_settings.key_aimbot);
        write(f, "aimbot_smooth", g_settings.aimbot_smooth);
        write(f, "aimbot_sensitivity", g_settings.aimbot_sensitivity);
        write(f, "aimbot_bone", g_settings.aimbot_bone);
        write(f, "aimbot_team_check", g_settings.aimbot_team_check);
        write(f, "draw_aimbot_fov", g_settings.draw_aimbot_fov);
        write_arr(f, "aimbot_fov_color", g_settings.aimbot_fov_color, 4);

        f << "\n[Triggerbot]\n";
        write(f, "triggerbot_enabled", g_settings.triggerbot_enabled);
        write(f, "key_triggerbot", g_settings.key_triggerbot);
        write(f, "triggerbot_delay", g_settings.triggerbot_delay);
        write(f, "triggerbot_scoped_only", g_settings.triggerbot_scoped_only);
        write(f, "triggerbot_always_on", g_settings.triggerbot_always_on);

        f << "\n[Chams]\n";
        write(f, "vis_check_skeleton", g_settings.vis_check_skeleton);
        write_arr(f, "vis_color_visible", g_settings.vis_color_visible, 4);
        write_arr(f, "vis_color_occluded", g_settings.vis_color_occluded, 4);

        f.close();
        return true;
    }

    static bool load(const std::string& path) {
        std::ifstream f(path);
        if (!f) return false;

        std::unordered_map<std::string, std::string> kv;
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '[' || line[0] == '#' || line[0] == ';')
                continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = trim(line.substr(0, eq));
            std::string val = trim(line.substr(eq + 1));
            kv[key] = val;
        }

        read(kv, "master_switch", g_settings.master_switch);
        read(kv, "esp_enabled", g_settings.esp_enabled);
        read(kv, "esp_use_theme", g_settings.esp_use_theme);
        read_arr(kv, "esp_theme_color", g_settings.esp_theme_color, 4);
        read(kv, "esp_opacity_drop", g_settings.esp_opacity_drop);
        read(kv, "esp_opacity_drop_start", g_settings.esp_opacity_drop_start);
        read(kv, "esp_opacity_drop_end", g_settings.esp_opacity_drop_end);
        read(kv, "esp_opacity_drop_min", g_settings.esp_opacity_drop_min);
        read(kv, "healthbar_solid_color", g_settings.healthbar_solid_color);
        read_arr(kv, "healthbar_color", g_settings.healthbar_color, 4);
        read(kv, "draw_box", g_settings.draw_box);
        read(kv, "draw_healthbar", g_settings.draw_healthbar);
        read(kv, "draw_health_text", g_settings.draw_health_text);
        read(kv, "draw_name", g_settings.draw_name);
        read(kv, "draw_weapon", g_settings.draw_weapon);
        read(kv, "draw_teammates", g_settings.draw_teammates);
        read(kv, "draw_head", g_settings.draw_head);
        read(kv, "chams_enabled", g_settings.chams_enabled);
        read(kv, "chams_style", g_settings.chams_style);
        read(kv, "esp_font_index", g_settings.esp_font_index);
        read(kv, "c4_esp_enabled", g_settings.c4_esp_enabled);
        read(kv, "show_bomb_timer", g_settings.show_bomb_timer);

        read(kv, "name_position", g_settings.name_position);
        read(kv, "name_offset_x", g_settings.name_offset_x);
        read(kv, "name_offset_y", g_settings.name_offset_y);
        read_arr(kv, "name_color", g_settings.name_color, 4);
        read_arr(kv, "name_shadow_color", g_settings.name_shadow_color, 4);
        read(kv, "name_shadow", g_settings.name_shadow);
        read(kv, "name_font_size", g_settings.name_font_size);

        read_arr(kv, "hp_text_color", g_settings.hp_text_color, 4);
        read_arr(kv, "hp_text_shadow_color", g_settings.hp_text_shadow_color, 4);
        read(kv, "hp_text_shadow", g_settings.hp_text_shadow);
        read(kv, "hp_font_size", g_settings.hp_font_size);

        read_arr(kv, "weapon_color", g_settings.weapon_color, 4);
        read_arr(kv, "weapon_shadow_color", g_settings.weapon_shadow_color, 4);
        read_arr(kv, "weapon_icon_color", g_settings.weapon_icon_color, 4);
        read(kv, "weapon_shadow", g_settings.weapon_shadow);
        read(kv, "weapon_font_size", g_settings.weapon_font_size);
        read(kv, "weapon_show_icon", g_settings.weapon_show_icon);
        read(kv, "weapon_show_text", g_settings.weapon_show_text);
        read(kv, "weapon_distance_dropoff", g_settings.weapon_distance_dropoff);

        read(kv, "draw_spectators", g_settings.draw_spectators);
        read(kv, "spec_x", g_settings.spec_x);
        read(kv, "spec_y", g_settings.spec_y);
        read(kv, "bomb_x", g_settings.bomb_x);
        read(kv, "bomb_y", g_settings.bomb_y);

        read(kv, "draw_radar", g_settings.draw_radar);
        read(kv, "radar_circle", g_settings.radar_circle);
        read(kv, "radar_rotate", g_settings.radar_rotate);
        read(kv, "radar_rings", g_settings.radar_rings);
        read(kv, "radar_names", g_settings.radar_names);
        read(kv, "radar_names_font_size", g_settings.radar_names_font_size);
        read(kv, "radar_size", g_settings.radar_size);
        read(kv, "radar_zoom", g_settings.radar_zoom);
        read(kv, "radar_bg_alpha", g_settings.radar_bg_alpha);
        read(kv, "radar_x", g_settings.radar_x);
        read(kv, "radar_y", g_settings.radar_y);
        read_arr(kv, "radar_enemy_color", g_settings.radar_enemy_color, 4);
        read_arr(kv, "radar_team_color", g_settings.radar_team_color, 4);

        read(kv, "grenade_helper_enabled", g_settings.grenade_helper_enabled);
        read(kv, "grenade_helper_visible", g_settings.grenade_helper_visible);
        read(kv, "key_grenade_toggle", g_settings.key_grenade_toggle);
        read(kv, "key_grenade_add", g_settings.key_grenade_add);
        read(kv, "key_grenade_delete", g_settings.key_grenade_delete);
        read(kv, "grenade_circle_radius", g_settings.grenade_circle_radius);
        read(kv, "grenade_circle_thickness", g_settings.grenade_circle_thickness);
        read(kv, "grenade_text_font_size", g_settings.grenade_text_font_size);
        read(kv, "grenade_filter_smoke", g_settings.grenade_filter_smoke);
        read(kv, "grenade_filter_molotov", g_settings.grenade_filter_molotov);
        read(kv, "grenade_filter_frag", g_settings.grenade_filter_frag);
        read(kv, "grenade_filter_flash", g_settings.grenade_filter_flash);
        read_arr(kv, "grenade_circle_color", g_settings.grenade_circle_color, 4);
        read_arr(kv, "grenade_circle_active_color", g_settings.grenade_circle_active_color, 4);
        read_arr(kv, "grenade_aim_line_color", g_settings.grenade_aim_line_color, 4);
        read_arr(kv, "grenade_text_color", g_settings.grenade_text_color, 4);

        read_arr(kv, "enemy_fill", g_settings.enemy_fill, 4);
        read_arr(kv, "enemy_outline", g_settings.enemy_outline, 4);
        read_arr(kv, "enemy_glow", g_settings.enemy_glow, 4);
        read_arr(kv, "team_fill", g_settings.team_fill, 4);
        read_arr(kv, "team_outline", g_settings.team_outline, 4);
        read_arr(kv, "team_glow", g_settings.team_glow, 4);
        read(kv, "body_width_scale", g_settings.body_width_scale);
        read(kv, "head_radius", g_settings.head_radius);
        read(kv, "depth_scale", g_settings.depth_scale);
        read(kv, "glow_expand_outer", g_settings.glow_expand_outer);
        read(kv, "glow_expand_inner", g_settings.glow_expand_inner);
        read(kv, "use_vsync", g_settings.use_vsync);
        read(kv, "target_fps", g_settings.target_fps);
        read(kv, "box_style", g_settings.box_style);
        read(kv, "box_thickness", g_settings.box_thickness);
        read(kv, "box_padding_x", g_settings.box_padding_x);
        read(kv, "box_padding_y", g_settings.box_padding_y);
        read(kv, "box_corner_pct", g_settings.box_corner_pct);

        read(kv, "crosshair_enabled", g_settings.crosshair_enabled);
        read(kv, "crosshair_shape", g_settings.crosshair_shape);
        read(kv, "crosshair_size", g_settings.crosshair_size);
        read(kv, "crosshair_gap", g_settings.crosshair_gap);
        read(kv, "crosshair_thickness", g_settings.crosshair_thickness);
        read_arr(kv, "crosshair_color", g_settings.crosshair_color, 4);
        read(kv, "crosshair_outline", g_settings.crosshair_outline);
        read(kv, "crosshair_outline_thickness", g_settings.crosshair_outline_thickness);
        read_arr(kv, "crosshair_outline_color", g_settings.crosshair_outline_color, 4);
        read(kv, "crosshair_dot", g_settings.crosshair_dot);
        read(kv, "crosshair_dot_size", g_settings.crosshair_dot_size);

        read(kv, "menu_x", g_settings.menu_x);
        read(kv, "menu_y", g_settings.menu_y);
        read(kv, "key_menu", g_settings.key_menu);
        read(kv, "key_master", g_settings.key_master);
        read(kv, "key_exit", g_settings.key_exit);
        read_arr(kv, "menu_accent_color", g_settings.menu_accent_color, 4);
        read(kv, "menu_bg_alpha", g_settings.menu_bg_alpha);
        read_arr(kv, "menu_border_color", g_settings.menu_border_color, 4);
        read(kv, "menu_font_index", g_settings.menu_font_index);
        read(kv, "menu_font_size", g_settings.menu_font_size);

        read(kv, "aimbot_enabled", g_settings.aimbot_enabled);
        read(kv, "aimbot_fov", g_settings.aimbot_fov);
        read(kv, "key_aimbot", g_settings.key_aimbot);
        read(kv, "aimbot_smooth", g_settings.aimbot_smooth);
        read(kv, "aimbot_sensitivity", g_settings.aimbot_sensitivity);
        read(kv, "aimbot_bone", g_settings.aimbot_bone);
        read(kv, "aimbot_team_check", g_settings.aimbot_team_check);
        read(kv, "draw_aimbot_fov", g_settings.draw_aimbot_fov);
        read_arr(kv, "aimbot_fov_color", g_settings.aimbot_fov_color, 4);

        read(kv, "triggerbot_enabled", g_settings.triggerbot_enabled);
        read(kv, "key_triggerbot", g_settings.key_triggerbot);
        read(kv, "triggerbot_delay", g_settings.triggerbot_delay);
        read(kv, "triggerbot_scoped_only", g_settings.triggerbot_scoped_only);
        read(kv, "triggerbot_always_on", g_settings.triggerbot_always_on);

        read(kv, "vis_check_skeleton", g_settings.vis_check_skeleton);
        read_arr(kv, "vis_color_visible", g_settings.vis_color_visible, 4);
        read_arr(kv, "vis_color_occluded", g_settings.vis_color_occluded, 4);

        return true;
    }

private:
    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }
    static void write(std::ofstream& f, const char* key, bool v) { f << key << " = " << (v ? 1 : 0) << "\n"; }
    static void write(std::ofstream& f, const char* key, int v) { f << key << " = " << v << "\n"; }
    static void write(std::ofstream& f, const char* key, float v) { f << key << " = " << v << "\n"; }
    static void write_arr(std::ofstream& f, const char* key, const float* v, int n) {
        f << key << " = ";
        for (int i = 0; i < n; i++) { if (i) f << ","; f << v[i]; }
        f << "\n";
    }
    static void read(const std::unordered_map<std::string, std::string>& kv, const char* key, bool& v) {
        auto it = kv.find(key); if (it != kv.end()) v = (std::stoi(it->second) != 0);
    }
    static void read(const std::unordered_map<std::string, std::string>& kv, const char* key, int& v) {
        auto it = kv.find(key); if (it != kv.end()) v = std::stoi(it->second);
    }
    static void read(const std::unordered_map<std::string, std::string>& kv, const char* key, float& v) {
        auto it = kv.find(key); if (it != kv.end()) v = std::stof(it->second);
    }
    static void read_arr(const std::unordered_map<std::string, std::string>& kv, const char* key, float* v, int n) {
        auto it = kv.find(key); if (it == kv.end()) return;
        std::istringstream ss(it->second); std::string token;
        for (int i = 0; i < n && std::getline(ss, token, ','); i++) v[i] = std::stof(token);
    }
};