#pragma once

#include "../imgui/imgui.h"

namespace colors
{
    inline ImVec4 rgba(int r, int g, int b, int a = 255)
    {
        return ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
    }

    // ── Background system ──  deep off-black, fully opaque, rich depth
    inline ImVec4 Bg             = rgba(8, 9, 14);
    inline ImVec4 ShellTint      = rgba(10, 12, 18);
    inline ImVec4 Surface        = rgba(12, 14, 22);
    inline ImVec4 SidebarBg      = rgba(10, 11, 17);
    inline ImVec4 PanelBg        = rgba(16, 18, 28);
    inline ImVec4 PanelHeader    = rgba(20, 22, 34);
    inline ImVec4 WidgetBg       = rgba(22, 24, 36);
    inline ImVec4 WidgetHover    = rgba(32, 34, 48);
    inline ImVec4 Border         = rgba(36, 38, 52);
    inline ImVec4 BorderLight    = rgba(46, 48, 64);

    // ── Accent (teal) ──  single accent, consistent everywhere
    inline ImVec4 Accent         = rgba(0, 220, 200);
    inline ImVec4 AccentHover    = rgba(30, 240, 220);
    inline ImVec4 AccentDark     = rgba(0, 165, 150);
    inline ImVec4 AccentDim      = rgba(0, 220, 200, 140);
    inline ImVec4 AccentGlow     = rgba(0, 220, 200, 100);

    // ── Text ──  warm off-white hierarchy
    inline ImVec4 Text           = rgba(220, 223, 230);
    inline ImVec4 TextDim        = rgba(140, 144, 158);
    inline ImVec4 TextMuted      = rgba(90, 94, 110);
    inline ImVec4 TextBright     = rgba(245, 245, 250);

    // ── Tabs / Navigation ──
    inline ImVec4 TabSurface     = rgba(16, 18, 26);
    inline ImVec4 TabHover       = rgba(24, 26, 36);
    inline ImVec4 TabActive      = rgba(22, 24, 34);
    inline ImVec4 TabPill        = rgba(26, 28, 40);
    inline ImVec4 TabStripBg     = rgba(14, 16, 22);
    inline ImVec4 TabTextActive  = rgba(245, 245, 250);
    inline ImVec4 TabTextIdle    = rgba(80, 84, 100);

    // ── Popups ──
    inline ImVec4 PopupBg       = rgba(14, 16, 24);
    inline ImVec4 DropdownBg    = rgba(14, 16, 24);

    // ── Keybind ──
    inline ImVec4 KeybindBg        = rgba(18, 20, 30);
    inline ImVec4 KeybindHover     = rgba(26, 28, 40);
    inline ImVec4 KeybindCapture   = rgba(0, 200, 180);
    inline ImVec4 KeybindText      = rgba(230, 230, 235);
    inline ImVec4 KeybindMuted     = rgba(100, 104, 120);

    // ── Gradient endpoints ──
    inline ImVec4 GradTop       = rgba(18, 20, 30);
    inline ImVec4 GradBot       = rgba(24, 26, 38);
    inline ImVec4 SideAccentTop = rgba(0, 200, 180, 120);
    inline ImVec4 SideAccentBot = rgba(0, 130, 120, 30);
    inline ImVec4 BrandGlow     = rgba(0, 220, 200, 60);

    // ── Shadows ──  rich depth layers
    inline ImVec4 ShadowDark    = rgba(0, 0, 0, 120);
    inline ImVec4 ShadowLight   = rgba(0, 0, 0, 60);

    // ── Status ──
    inline ImVec4 Green         = rgba(60, 210, 120);
    inline ImVec4 Red           = rgba(230, 60, 70);
    inline ImVec4 Yellow        = rgba(230, 190, 40);
    inline ImVec4 Blue          = rgba(50, 140, 230);
}
