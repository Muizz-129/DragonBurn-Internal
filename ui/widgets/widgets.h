#pragma once

#include "../../imgui/imgui.h"

#include <map>

// ── Global animation state for widgets ──
struct WidgetAnim
{
    float hover  = 0.f;
    float active = 0.f;
    float pulse  = 0.f;
    float slide  = -1.f;
};

extern std::map<ImGuiID, WidgetAnim> g_anims;

namespace widgets
{
    // ── Panels ──
    // rows = -1 enables auto-height (measures content at end_panel)
    bool begin_panel(const char* title, float width, int rows = -1,
                     bool has_slider = false, bool default_open = true,
                     int keybind_rows = 0);
    void end_panel();

    // ── Checkboxes ──
    bool checkbox(const char* label, bool* value,
                  bool last_element = false, float right_inset = 0.f);

    // ── Combined checkbox + keybind row ──
    bool checkbox_keybind_row(const char* label, bool* value, const char* id,
                              int* key, int* mode, bool last_element = false);

    // ── Sliders ──
    bool slider_float(const char* label, float* value, float min, float max,
                      const char* format = "%.1f", bool last_element = false);

    // ── Combo boxes ──
    bool combo(const char* label, int* index, const char* const items[],
               int count, bool last_element = false);

    // ── Keybind setter (standalone) ──
    bool keybind_setter(const char* id, int* key, int* mode);

    // ── Settings cog popup ──
    bool settings_cog(const char* id, int slot = 0, bool active = false);
    void end_settings_popup();

    // ── Selectable items ──
    bool combo_selectable_item(const char* label, bool selected);
    bool selectable_item(const char* label, bool selected);
}
