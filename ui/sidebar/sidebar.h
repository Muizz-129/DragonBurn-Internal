#pragma once

#include "../../imgui/imgui_internal.h"

namespace sidebar
{
    enum class tab : int
    {
        combat = 0,
        visuals,
        misc,
        settings,
        count
    };

    enum class aim_subtab : int
    {
        legit = 0,
        rage,
        count
    };

    struct state
    {
        tab selected = tab::combat;
        aim_subtab aim_mode = aim_subtab::legit;
        bool blocks_drag = false;
        char search[64] = {};
    };

    const char* tab_name(tab t);

    void draw(const ImRect& bounds, state& sidebar_state);
}
