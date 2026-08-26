#pragma once

#include "../../imgui/imgui_internal.h"

namespace topbar
{
    enum class combat_tab : int
    {
        aimbot = 0,
        triggerbot,
        recoil,
        count
    };

    struct state
    {
        combat_tab selected = combat_tab::aimbot;
        bool blocks_drag = false;
        char search[64] = {};
    };

    const char* combat_tab_name(combat_tab tab);

    float draw_header(state& header_state);
}
