#pragma once

#include "../imgui/imgui_internal.h"

struct HWND__;

namespace window_drag
{
    void handle(HWND__* hwnd, const ImRect& drag_rect, bool block_drag);
}
