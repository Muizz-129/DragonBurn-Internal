#pragma once
#include <windows.h>
static constexpr float REFERENCE_HEIGHT = 1080.0f;

struct MenuSettings {
    bool master_switch = true;
    bool esp_enabled = true;
    bool draw_box = true;
    bool draw_healthbar = true;
    bool draw_health_text = true;
    bool draw_name = true;
    bool draw_weapon = true;
    bool draw_teammates = false;
    bool draw_head = true;
    bool draw_spectators = true;
    bool draw_radar = true;
    bool chams_enabled = true;
    int chams_style = 3;
    bool vis_check_skeleton = false;
    float vis_color_visible[4] = { 0.0f, 1.0f, 0.4f, 1.0f };
    float vis_color_occluded[4] = { 1.0f, 0.2f, 0.2f, 1.0f };

    bool use_vsync = true;
    float target_fps = 144.0f;

    // ---- ESP Theme ----
    bool esp_use_theme = true;
    // Theme color is the "base" accent: box will be darkened, text/icons will be lightened
    float esp_theme_color[4] = { 0.235f, 0.68f, 0.93f, 1.0f };  // default: cyan

    float enemy_fill[4] = { 0.86f, 0.16f, 0.16f, 0.5f };
    float enemy_outline[4] = { 1.00f, 0.24f, 0.24f, 0.35f };
    float enemy_glow[4] = { 1.00f, 0.20f, 0.20f, 0.10f };
    float team_fill[4] = { 0.16f, 0.39f, 0.86f, 0.5f };
    float team_outline[4] = { 0.24f, 0.51f, 1.00f, 0.35f };
    float team_glow[4] = { 0.20f, 0.39f, 1.00f, 0.10f };

    float body_width_scale = 1.0f;
    float head_radius = 6.0f;
    float depth_scale = 500.0f;
    float glow_expand_outer = 6.0f;
    float glow_expand_inner = 3.0f;

    bool menu_open = false;

    // ESP Opacity Drop (distance-based)
    bool  esp_opacity_drop = true;
    float esp_opacity_drop_start = 1500.0f;   // distance at which fade begins
    float esp_opacity_drop_end = 3000.0f;  // distance at which fully faded
    float esp_opacity_drop_min = 0.1f;     // minimum alpha multiplier at max distance

    // Radar
    float radar_size = 330.0f;
    float radar_zoom = 0.35f;
    float radar_x = 35.0f;
    float radar_y = 35.0f;
    bool radar_rotate = true;
    bool radar_circle = true;
    bool radar_rings = false;
    bool radar_names = false;
    float radar_names_font_size = 11.0f;
    float radar_bg_alpha = 0.0f;
    float radar_enemy_color[4] = { 1.00f, 0.30f, 0.30f, 0.90f };
    float radar_team_color[4] = { 0.30f, 0.55f, 1.00f, 0.90f };

    int box_style = 0;
    float box_thickness = 1.5f;
    float box_padding_x = 5.0f;
    float box_padding_y = 5.0f;
    float box_corner_pct = 0.2f;

    // ESP Font — -1 means scan_fonts() will auto-select Tahoma (or first available)
    int esp_font_index = -1;
    float esp_font_atlas_size = 12.0f;

    // Name ESP
    int name_position = 0;
    float name_offset_x = 0.0f;
    float name_offset_y = -3.0f;
    float name_color[4] = { 0.1f, 1.0f, 0.9f, 0.9f };
    float name_shadow_color[4] = { 0.0f, 0.0f, 0.0f, 0.4f };
    bool name_shadow = true;
    float name_font_size = 12.0f;

    // Health bar
    bool healthbar_solid_color = false;       // if true: flat color instead of green->red gradient
    float healthbar_color[4] = { 0.2f, 0.85f, 1.0f, 0.85f };  // solid color when enabled

    // Health text
    float hp_text_color[4] = { 1.0f, 1.0f, 1.0f, 0.7f };
    float hp_text_shadow_color[4] = { 0.0f, 0.0f, 0.0f, 0.4f };
    bool hp_text_shadow = true;
    float hp_font_size = 12.0f;

    // Weapon ESP
    float weapon_color[4] = { 0.8f, 0.8f, 0.8f, 0.85f };
    float weapon_shadow_color[4] = { 0.0f, 0.0f, 0.0f, 0.5f };
    float weapon_icon_color[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
    bool weapon_shadow = true;
    float weapon_font_size = 11.0f;
    bool weapon_show_icon = true;
    bool weapon_show_text = false;
    float weapon_distance_dropoff = 0.3f;

    // Spectator list position
    float spec_x = -1.0f;
    float spec_y = 30.0f;

    // Bomb timer position
    float bomb_x = -1.0f;
    float bomb_y = 200.0f;

    // Grenade Helper
    bool  grenade_helper_enabled = false;
    bool  grenade_helper_visible = true;
    int   key_grenade_toggle = VK_F3;
    int   key_grenade_add = VK_F4;
    int   key_grenade_delete = VK_DELETE;
    float grenade_circle_radius = 10.0f;
    float grenade_circle_thickness = 2.0f;
    float grenade_text_font_size = 12.0f;
    bool  grenade_filter_smoke = true;
    bool  grenade_filter_molotov = true;
    bool  grenade_filter_frag = true;
    bool  grenade_filter_flash = true;
    float grenade_circle_color[4] = { 0.30f, 0.55f, 1.00f, 0.75f };
    float grenade_circle_active_color[4] = { 0.20f, 1.00f, 0.45f, 0.95f };
    float grenade_aim_line_color[4] = { 1.00f, 1.00f, 1.00f, 0.65f };
    float grenade_text_color[4] = { 1.00f, 1.00f, 1.00f, 0.95f };

    // Aimbot
    bool aimbot_enabled = false;
    int key_aimbot = 'X';
    float aimbot_fov = 5.f;
    float aimbot_smooth = 1.0f;
    float aimbot_sensitivity = 1.0f; // in-game sensitivity 
    int  aimbot_bone = 0; // 0=Head,1=Neck,2=Chest,3=Pelvis
    bool aimbot_visible_check = true;
    bool aimbot_team_check = true;
    bool draw_aimbot_fov = false;
    float aimbot_fov_color[4] = { 1.0f, 1.0f, 1.0f, 0.5f };

    // Trigger
    bool  triggerbot_enabled = false;
    bool  triggerbot_always_on = false;
    bool  triggerbot_scoped_only = false;
    int   key_triggerbot = 'X';
    float triggerbot_delay = 50.f;


    // Crosshair
    bool crosshair_enabled = false;
    int crosshair_shape = 0;
    float crosshair_size = 5.0f;
    float crosshair_gap = 2.0f;
    float crosshair_thickness = 1.5f;
    float crosshair_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    bool crosshair_outline = false;
    float crosshair_outline_thickness = 1.0f;
    float crosshair_outline_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    bool crosshair_dot = false;
    float crosshair_dot_size = 1.5f;

    // Menu
    float menu_x = -1.0f;
    float menu_y = -1.0f;
    int menu_tab = 0;

    // Menu style
    float menu_accent_color[4] = { 0.0f, 1.0f, 0.65f, 1.0f };
    float menu_bg_alpha = 1.0f;
    float menu_border_color[4] = { 0.0f, 1.0f, 0.65f, 0.3f };

    // Menu font
    int menu_font_index = -1;
    float menu_font_size = 14.0f;

    // Key binds
    int key_menu = VK_F1;
    int key_master = VK_F2;
    int key_exit = VK_INSERT;

    bool show_is_scoped = true;
    bool show_flashed_esp = true;

    // C4 / Bomb ESP
    bool c4_esp_enabled = true;
    bool show_bomb_timer = true;

    void reset() { *this = MenuSettings{}; }
};

inline MenuSettings g_settings;

// ============================================================
//  ESP Theme helpers
//  When esp_use_theme is true, all per-element colors are
//  derived from esp_theme_color instead of their individual
//  settings.  Enemy uses the raw hue; team uses a blue-shifted
//  complementary tint.
// ============================================================
namespace EspTheme {

    // Scale an rgb channel, clamped to [0,1]
    inline float sc(float v, float f) {
        return v * f < 0.0f ? 0.0f : (v * f > 1.0f ? 1.0f : v * f);
    }

    // Returns a derived color: hue from base rgb scaled by `bright`,
    // alpha = base[3] * a_ratio  (so the theme color picker's alpha is honoured).
    inline void derive(const float base[4], float bright, float a_ratio, float out[4]) {
        out[0] = sc(base[0], bright);
        out[1] = sc(base[1], bright);
        out[2] = sc(base[2], bright);
        out[3] = sc(base[3], a_ratio);   // <-- was: out[3] = a_ratio  (ignored base alpha)
    }

    // Blue-shift for teammates: blend hue toward blue, same alpha fix.
    inline void derive_team(const float base[4], float bright, float a_ratio, float out[4]) {
        float blue[4] = { 0.2f, 0.45f, 1.0f, 1.0f };
        out[0] = sc(base[0] * 0.35f + blue[0] * 0.65f, bright);
        out[1] = sc(base[1] * 0.35f + blue[1] * 0.65f, bright);
        out[2] = sc(base[2] * 0.35f + blue[2] * 0.65f, bright);
        out[3] = sc(base[3], a_ratio);   // <-- same fix
    }

    // Fill all theme-driven color arrays in g_settings for preview/live use.
    // Called only when esp_use_theme == true.
    inline void apply(bool enemy) {
        const float* t = g_settings.esp_theme_color;

        if (enemy) {
            // Box outline: bright, high alpha
            derive(t, 0.85f, 0.70f, g_settings.enemy_outline);
            // Chams fill: medium-bright
            derive(t, 0.55f, 0.55f, g_settings.enemy_fill);
            // Glow: bright, subtle
            derive(t, 0.75f, 0.20f, g_settings.enemy_glow);
            // Name text: bright
            derive(t, 1.15f, 0.92f, g_settings.name_color);
            // Weapon text: slightly dimmer
            derive(t, 0.95f, 0.85f, g_settings.weapon_color);
            // Weapon icon: bright white-tinted
            derive(t, 1.10f, 0.90f, g_settings.weapon_icon_color);
            // Healthbar solid color: bright theme color
            derive(t, 1.05f, 0.85f, g_settings.healthbar_color);
            // HP text: bright theme hue, slightly dimmer than name
            derive(t, 1.05f, 0.86f, g_settings.hp_text_color);
        }
        else {
            derive_team(t, 0.85f, 0.70f, g_settings.team_outline);
            derive_team(t, 0.55f, 0.55f, g_settings.team_fill);
            derive_team(t, 0.75f, 0.20f, g_settings.team_glow);
        }
    }

} // namespace EspTheme

inline float esp_depth_opacity(float depth) {
    if (!g_settings.esp_opacity_drop) return 1.0f;
    float start = g_settings.esp_opacity_drop_start;
    float end = g_settings.esp_opacity_drop_end;
    float mn = g_settings.esp_opacity_drop_min;
    if (depth <= start) return 1.0f;
    if (depth >= end)   return mn;
    float t = (depth - start) / (end - start);
    return 1.0f - t * (1.0f - mn);
}