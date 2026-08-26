#pragma once

#include "../imgui/imgui.h"

namespace fonts
{
    void init();
    void load_icons_only();
    void shutdown();

    ImFont* regular();
    ImFont* regular_12();
    ImFont* regular_11();
    ImFont* regular_14();
    ImFont* bold();
    ImFont* bold_14();
    ImFont* bold_16();
    ImFont* brand_18();
    ImFont* brand_title();
    ImFont* brand();
    ImFont* icon();
    ImFont* icon_12();
}
