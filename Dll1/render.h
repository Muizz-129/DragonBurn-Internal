#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_dx11.h"
#include "types.h"
#include "settings.h"
#include "utils.h"
#include "input.h"
#include "entity_reader.h"
#include "visible_esp.h"
#include "c4_esp.h"
#include "spectators.h"
#include "projectile_trails.h"
#include "weapon_icons.h"
#include "../helpers/fonts.h"
#include "menu.h"
#include "config.h"
#include "crosshair.h"
#include "grenades.h"
#include "aimbot.h"

namespace Render {
    inline ID3D11Device* g_pDevice = nullptr;
    inline ID3D11DeviceContext* g_pContext = nullptr;
    inline ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
    inline IDXGISwapChain* g_pPinnedSwapChain = nullptr;
    inline bool                    g_Init = false;
    inline int                     g_SkipCount = 0;
    inline EntityReader            g_entity_reader;

    inline void create_render_target(IDXGISwapChain* pSwapChain) {
        ID3D11Texture2D* pBackBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
        if (pBackBuffer) {
            g_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
            pBackBuffer->Release();
        }
    }

    inline void cleanup_render_target() {
        if (g_pRenderTargetView) {
            if (g_pContext) g_pContext->OMSetRenderTargets(0, nullptr, nullptr);
            g_pRenderTargetView->Release();
            g_pRenderTargetView = nullptr;
        }
    }

    inline void init_imgui(IDXGISwapChain* pSwapChain) {
        if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pDevice)))
            return;
        g_pDevice->GetImmediateContext(&g_pContext);

        DXGI_SWAP_CHAIN_DESC desc;
        pSwapChain->GetDesc(&desc);

        create_render_target(pSwapChain);
        Input::hook(desc.OutputWindow);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;

        fonts::init();
        g_weapon_icons.init(g_pDevice);

        menu_textures_init(g_pDevice);

        ImGui_ImplWin32_Init(desc.OutputWindow);
        ImGui_ImplDX11_Init(g_pDevice, g_pContext);

        std::string dll_dir = get_dll_directory();
        Config::load(dll_dir + "config.ini");
        g_grenades.init(dll_dir + "grenades.json");

        g_Init = true;
    }

    inline void render_frame(IDXGISwapChain* sc) {
        if (!g_pRenderTargetView) {
            create_render_target(sc);
            if (!g_pRenderTargetView) return;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        int screen_w = 0;
        int screen_h = 0;
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) {
            D3D11_TEXTURE2D_DESC tex_desc;
            pBackBuffer->GetDesc(&tex_desc);
            screen_w = (int)tex_desc.Width;
            screen_h = (int)tex_desc.Height;
            pBackBuffer->Release();
        }

        ImGuiIO& io = ImGui::GetIO();
        if (screen_w > 0 && screen_h > 0) {
            io.DisplaySize = ImVec2((float)screen_w, (float)screen_h);
        }
        else {
            screen_w = (int)io.DisplaySize.x;
            screen_h = (int)io.DisplaySize.y;
        }

        io.MouseDrawCursor = g_settings.menu_open;

        ImGui::NewFrame();
        ImDrawList* draw = ImGui::GetBackgroundDrawList();

        FrameState state = g_entity_reader.read_frame(screen_w, screen_h);

        AimbotFrame ab_frame{};
        ab_frame.local_pawn = state.local.pawn;
        ab_frame.local_team = state.local.team;
        ab_frame.local_player_index = state.local_player_index;
        ab_frame.screen_w = screen_w;
        ab_frame.screen_h = screen_h;
        ab_frame.camera_valid = state.local.camera.valid;
        ab_frame.camera_fov = state.local.camera.fov;
        ab_frame.eye_origin = state.local.camera.origin;
        ab_frame.view_angles = state.local.camera.angles;
        ab_frame.is_scoped = state.local.is_scoped;
        ab_frame.local_weapon_def_index = state.local_weapon_def_index;

        for (int i = 0; i < 64; i++) {
            if (!state.players[i].valid) continue;
            ab_frame.targets[i].valid = true;
            ab_frame.targets[i].health = state.players[i].health;
            ab_frame.targets[i].team = state.players[i].team;
            ab_frame.targets[i].bSpottedByMask = state.players[i].bSpottedByMask;
            ab_frame.targets[i].head_pos = state.players[i].head_world;
            ab_frame.targets[i].neck_pos = state.players[i].neck_world;
            ab_frame.targets[i].chest_pos = state.players[i].chest_world;
            ab_frame.targets[i].pelvis_pos = state.players[i].pelvis_world;
        }
        g_aimbot_data.publish(ab_frame);

        if (g_settings.master_switch) {

            g_projectile_trails.set_view_matrix(state.view_matrix);
            g_projectile_trails.update_and_draw(draw, state.entity_list, screen_w, screen_h);

            for (int i = 0; i < 64; i++) {
                auto& p = state.players[i];
                if (!p.valid) continue;

                for (int b = 0; b < MAX_BONE; b++) {
                    p.visible[b] = w2s_depth(p.bones_world[b], state.view_matrix, screen_w, screen_h, p.screens[b], p.depths[b]);
                }
                g_esp.draw_player(draw, p, state.local.team, screen_w, screen_h, i, state.local.is_scoped);
            }

            // Bomb & Timer
            PlantedC4State c4 = read_planted_c4();
            draw_c4_esp(draw, c4, screen_w, screen_h, state.view_matrix);
            draw_bomb_timer(c4);

            DroppedC4State dropped_c4 = read_dropped_c4(c4.valid);
            if (dropped_c4.valid) {
                draw_dropped_c4_esp(draw, dropped_c4, screen_w, screen_h, state.view_matrix);
            }

            // Spectators List
            g_spectators.update(state.entity_list, state.local.pawn, state.local.controller);
            g_spectators.draw(screen_w);

            // Crosshair
            if (g_settings.crosshair_enabled) {
                Crosshair::Config cc{};
                cc.enabled = g_settings.crosshair_enabled;
                cc.shape = g_settings.crosshair_shape;
                cc.size = g_settings.crosshair_size;
                cc.gap = g_settings.crosshair_gap;
                cc.thickness = g_settings.crosshair_thickness;
                cc.color = float4_to_col(g_settings.crosshair_color);
                cc.outline = g_settings.crosshair_outline;
                cc.outline_thickness = g_settings.crosshair_outline_thickness;
                cc.outline_color = float4_to_col(g_settings.crosshair_outline_color);
                cc.dot = g_settings.crosshair_dot;
                cc.dot_size = g_settings.crosshair_dot_size;
                g_crosshair.draw(draw, screen_w, screen_h, cc);
            }

            // FOV Circle
            if (g_settings.aimbot_enabled && g_settings.draw_aimbot_fov) {
                float cam_fov = (state.local.camera.valid && state.local.camera.fov > 0.0f)
                    ? state.local.camera.fov
                    : 90.0f;

                float rad_aim = (g_settings.aimbot_fov * 0.5f) * (3.14159265f / 180.0f);
                float rad_cam = (cam_fov * 0.5f) * (3.14159265f / 180.0f);
                float fov_radius = (tanf(rad_aim) / tanf(rad_cam)) * (screen_w * 0.5f);

                ImVec2 center = ImVec2((float)screen_w * 0.5f, (float)screen_h * 0.5f);
                ImU32 fov_col = float4_to_col(g_settings.aimbot_fov_color);

                draw->AddCircle(center, fov_radius, fov_col, 64, 1.2f);
            }

            // Grenade Helper
            if (g_settings.grenade_helper_enabled) {
                g_grenades.set_view_matrix(state.view_matrix);
                g_grenades.update_held_weapon(state.local_weapon_def_index);
                g_grenades.update(state.local.x, state.local.y, state.local.z,
                    state.local.camera.angles.x, state.local.camera.angles.y,
                    state.map_name);
                g_grenades.draw(draw, state.local.x, state.local.y, state.local.z, screen_w, screen_h);
            }
        }

        g_grenades.render_popups();

        static bool was_menu_open = false;

        if (g_settings.menu_open) {
            g_menu.render();
            was_menu_open = true;
        }
        else if (was_menu_open) {
            Config::save(get_dll_directory() + "config.ini");
            was_menu_open = false;
        }

        ImGui::Render();
        g_pContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}