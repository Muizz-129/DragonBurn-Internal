#include "fonts.h"
#include "../Dll1/fontello_data.h"
#include "../Dll1/fonts_data.h"
#include <Windows.h>
#include <string>
#include <filesystem>

namespace
{
    struct font_slot
    {
        ImFont* regular_15 = nullptr;
        ImFont* regular_14 = nullptr;
        ImFont* regular_13 = nullptr;
        ImFont* regular_16 = nullptr;
        ImFont* bold_15 = nullptr;
        ImFont* bold_16 = nullptr;
        ImFont* bold_18 = nullptr;
        ImFont* brand_20 = nullptr;
        ImFont* brand_24 = nullptr;
        ImFont* brand_28 = nullptr;
        ImFont* icon_14 = nullptr;
        ImFont* icon_12 = nullptr;
    };

    font_slot g_fonts;

    static const ImWchar icon_range[] = { 0xE800, 0xF8FF, 0 };

    std::string get_dll_directory()
    {
        char path[MAX_PATH]{};
        HMODULE hm = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&g_fonts), &hm)) {
            GetModuleFileNameA(hm, path, sizeof(path));
            std::string full_path(path);
            size_t pos = full_path.find_last_of("\\/");
            if (pos != std::string::npos) {
                return full_path.substr(0, pos + 1);
            }
        }
        return "";
    }

    ImFont* load_inter(float size, bool bold)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImFontConfig cfg;
        cfg.SizePixels = size;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        cfg.FontDataOwnedByAtlas = false;
        std::string font_path = get_dll_directory() + "Assets/NotoSans_Condensed-Bold.ttf";
        if (std::filesystem::exists(font_path)) {
            return io.Fonts->AddFontFromFileTTF(font_path.c_str(), size, &cfg);
        }
        const auto& data = bold ? fonts_data::inter_bold : fonts_data::inter_regular;
        auto data_size = bold ? fonts_data::inter_bold_size : fonts_data::inter_regular_size;
        return io.Fonts->AddFontFromMemoryTTF((void*)data, (int)data_size, size, &cfg);
    }

    ImFont* load_icon(float size)
    {
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        cfg.GlyphMinAdvanceX = size;

        ImGuiIO& io = ImGui::GetIO();
        return io.Fonts->AddFontFromMemoryTTF(
            (void*)fontello_data::ttf, (int)fontello_data::ttf_size, size, &cfg, icon_range);
    }

    void rebuild_atlas()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        g_fonts.regular_15 = load_inter(15.f, false);
        g_fonts.regular_14 = load_inter(14.f, false);
        g_fonts.regular_13 = load_inter(13.f, false);
        g_fonts.regular_16 = load_inter(16.f, false);
        g_fonts.bold_15 = load_inter(15.f, true);
        g_fonts.bold_16 = load_inter(16.f, true);
        g_fonts.bold_18 = load_inter(18.f, true);
        g_fonts.brand_20 = load_inter(20.f, true);
        g_fonts.brand_24 = load_inter(24.f, true);
        g_fonts.brand_28 = load_inter(28.f, true);
        g_fonts.icon_14 = load_icon(14.f);
        g_fonts.icon_12 = load_icon(12.f);

        io.FontDefault = g_fonts.regular_15;
        io.Fonts->Build();
    }
}

namespace fonts
{
    void init()
    {
        rebuild_atlas();
    }

    void load_icons_only()
    {
        g_fonts.regular_15 = load_inter(15.f, false);
        g_fonts.regular_14 = load_inter(14.f, false);
        g_fonts.regular_13 = load_inter(13.f, false);
        g_fonts.regular_16 = load_inter(16.f, false);
        g_fonts.bold_15 = load_inter(15.f, true);
        g_fonts.bold_16 = load_inter(16.f, true);
        g_fonts.bold_18 = load_inter(18.f, true);
        g_fonts.brand_20 = load_inter(20.f, true);
        g_fonts.brand_24 = load_inter(24.f, true);
        g_fonts.brand_28 = load_inter(28.f, true);
        g_fonts.icon_14 = load_icon(14.f);
        g_fonts.icon_12 = load_icon(12.f);
    }

    void shutdown()
    {
        g_fonts = {};
    }

    ImFont* regular() { return g_fonts.regular_15; }
    ImFont* regular_12() { return g_fonts.regular_14; }
    ImFont* regular_11() { return g_fonts.regular_13; }
    ImFont* bold_14() { return g_fonts.bold_16; }
    ImFont* regular_14() { return g_fonts.regular_16; }
    ImFont* bold() { return g_fonts.bold_15; }
    ImFont* bold_16() { return g_fonts.bold_18; }
    ImFont* brand_18() { return g_fonts.brand_20; }
    ImFont* brand_title() { return g_fonts.brand_24; }
    ImFont* brand() { return g_fonts.brand_28; }
    ImFont* icon() { return g_fonts.icon_14; }
    ImFont* icon_12() { return g_fonts.icon_12; }
}