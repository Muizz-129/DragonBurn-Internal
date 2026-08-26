#pragma once

#include "../imgui/imgui.h"

namespace icons
{
    enum class glyph : ImWchar
    {
        cog = 0xEA6C,
        search = 0xE8E0,
        keyboard = 0xEA51,
        check = 0xE87B,
        arrow_up = 0xEA8D,
        arrow_down = 0xEA8A,
        dot_3 = 0xEAD6,
        target_1 = 0xE89B,
        target = 0xE824,
    };

    const char* to_utf8(glyph icon);

    inline constexpr const char* cog = "\xEE\xA9\xAC";
    inline constexpr const char* search = "\xEE\xA3\xA0";
    inline constexpr const char* keyboard = "\xEE\xA9\x91";
    inline constexpr const char* check = "\xEE\xA1\xBB";
    inline constexpr const char* arrow_up = "\xEE\xAA\x8D";
    inline constexpr const char* arrow_down = "\xEE\xAA\x8A";
    inline constexpr const char* dot_3 = "\xEE\xAB\x96";
    inline constexpr const char* target_1 = "\xEE\xA2\x9B";
    inline constexpr const char* target = "\xEE\xA0\xA4";
}
