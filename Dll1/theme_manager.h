#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include "../nlohmann/json.hpp"
#include "../imgui/imgui.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

struct UITheme {
    std::string name = "Default Cyan";
    float accent_color[4] = { 0.0f, 0.86f, 0.78f, 1.0f };
    float window_bg[4] = { 0.04f, 0.05f, 0.08f, 0.92f };
    float child_bg[4] = { 0.06f, 0.08f, 0.12f, 0.85f };
    float text_primary[4] = { 0.86f, 0.87f, 0.90f, 1.0f };
    float frame_bg[4] = { 0.09f, 0.11f, 0.16f, 1.0f };
    float window_rounding = 8.0f;
    float frame_rounding = 4.0f;
};

class ThemeManager {
public:
    UITheme current_theme;
    std::vector<std::string> theme_files;
    int selected_theme_index = 0;

    // Dapatkan folder lokasi sebenar fail DLL
    std::string get_dll_directory() {
        char path[MAX_PATH]{};
        HMODULE hm = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(this), &hm)) {
            GetModuleFileNameA(hm, path, sizeof(path));
            std::string full_path(path);
            size_t pos = full_path.find_last_of("\\/");
            if (pos != std::string::npos) {
                return full_path.substr(0, pos + 1);
            }
        }
        return "";
    }

    void refresh_themes() {
        theme_files.clear();
        std::string folder = get_dll_directory() + "themes";

        if (!fs::exists(folder)) {
            fs::create_directories(folder);
            save_theme(folder + "/default.json", current_theme);
        }

        for (const auto& entry : fs::directory_iterator(folder)) {
            if (entry.path().extension() == ".json") {
                theme_files.push_back(entry.path().stem().string());
            }
        }
    }

    void apply_theme(const UITheme& th) {
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = th.window_rounding;
        style.FrameRounding = th.frame_rounding;
        style.ChildRounding = th.window_rounding;

        style.Colors[ImGuiCol_WindowBg] = ImVec4(th.window_bg[0], th.window_bg[1], th.window_bg[2], th.window_bg[3]);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(th.child_bg[0], th.child_bg[1], th.child_bg[2], th.child_bg[3]);
        style.Colors[ImGuiCol_Text] = ImVec4(th.text_primary[0], th.text_primary[1], th.text_primary[2], th.text_primary[3]);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(th.frame_bg[0], th.frame_bg[1], th.frame_bg[2], th.frame_bg[3]);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(th.accent_color[0], th.accent_color[1], th.accent_color[2], th.accent_color[3]);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(th.accent_color[0], th.accent_color[1], th.accent_color[2], th.accent_color[3]);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(th.accent_color[0], th.accent_color[1], th.accent_color[2], 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(th.accent_color[0], th.accent_color[1], th.accent_color[2], 0.8f);
    }

    bool load_theme(const std::string& theme_name) {
        try {
            std::string full_path = get_dll_directory() + "themes/" + theme_name + ".json";
            std::ifstream file(full_path);
            if (!file.is_open()) return false;

            json j;
            file >> j;

            current_theme.name = j.value("name", "Custom");
            for (int i = 0; i < 4; i++) {
                current_theme.accent_color[i] = j["accent"][i];
                current_theme.window_bg[i] = j["window_bg"][i];
                current_theme.child_bg[i] = j["child_bg"][i];
                current_theme.text_primary[i] = j["text"][i];
                current_theme.frame_bg[i] = j["frame_bg"][i];
            }
            current_theme.window_rounding = j.value("window_rounding", 8.0f);
            current_theme.frame_rounding = j.value("frame_rounding", 4.0f);

            apply_theme(current_theme);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool save_theme(const std::string& full_path, const UITheme& th) {
        try {
            json j;
            j["name"] = th.name;
            j["accent"] = th.accent_color;
            j["window_bg"] = th.window_bg;
            j["child_bg"] = th.child_bg;
            j["text"] = th.text_primary;
            j["frame_bg"] = th.frame_bg;
            j["window_rounding"] = th.window_rounding;
            j["frame_rounding"] = th.frame_rounding;

            std::ofstream file(full_path);
            file << j.dump(4);
            return true;
        }
        catch (...) {
            return false;
        }
    }
};

inline ThemeManager g_theme_mgr;