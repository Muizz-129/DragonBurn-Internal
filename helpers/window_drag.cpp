#include "window_drag.h"

#include "../imgui/imgui.h"

#include <unordered_map>
#include <windows.h>

namespace window_drag
{
    namespace
    {
        struct drag_state
        {
            bool active = false;
            POINT mouse_start = {};
            POINT win_start = {};
        };

        drag_state& state_for(HWND hwnd)
        {
            static std::unordered_map<HWND, drag_state> states;
            return states[hwnd];
        }
    }

    void handle(HWND hwnd, const ImRect& drag_rect, bool block_drag)
    {
        drag_state& s = state_for(hwnd);

        if (block_drag)
        {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                s.active = false;
            return;
        }

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const bool in_drag_rect = drag_rect.Contains(mouse);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && in_drag_rect)
        {
            s.active = true;
            ::GetCursorPos(&s.mouse_start);
            RECT rc = {};
            ::GetWindowRect(hwnd, &rc);
            s.win_start.x = rc.left;
            s.win_start.y = rc.top;
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            s.active = false;

        if (!s.active)
            return;

        POINT cur = {};
        ::GetCursorPos(&cur);
        ::SetWindowPos(hwnd, nullptr,
            s.win_start.x + (cur.x - s.mouse_start.x),
            s.win_start.y + (cur.y - s.mouse_start.y),
            0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
