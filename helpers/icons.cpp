#include "icons.h"
#include "../imgui/imgui_internal.h"

namespace icons
{
    const char* to_utf8(glyph icon)
    {
        static char bufs[8][8] = {};
        const int idx = static_cast<int>(icon) % 8;
        ImTextCharToUtf8(bufs[idx], static_cast<ImWchar>(icon));
        return bufs[idx];
    }
}
