#pragma once
#include <Windows.h>
#include "../imgui/imgui.h"
#include "settings.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Input {
    inline HWND g_hWnd = nullptr;
    inline WNDPROC g_oWndProc = nullptr;

    inline LRESULT CALLBACK hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_KEYDOWN) {
            if (wParam == g_settings.key_menu || wParam == VK_INSERT) {
                g_settings.menu_open = !g_settings.menu_open;
                return 0;
            }
            if (wParam == g_settings.key_master) {
                g_settings.master_switch = !g_settings.master_switch;
                return 0;
            }
        }

        if (g_settings.menu_open) {

            if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
                return true;

            switch (msg) {
            case WM_MOUSEMOVE:
            case WM_NCMOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
            case WM_INPUT:
                return 0;
            }
        }

        return CallWindowProc(g_oWndProc, hWnd, msg, wParam, lParam);
    }

    inline void hook(HWND hWnd) {
        g_hWnd = hWnd;
        g_oWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
    }

    inline void unhook() {
        if (g_hWnd && g_oWndProc) {
            SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)g_oWndProc);
            g_oWndProc = nullptr;
        }
    }
}