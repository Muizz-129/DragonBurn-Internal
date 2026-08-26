#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include "types.h"
#include "settings.h"
#include "input.h"
#include "entity_reader.h"
#include "visible_esp.h"
#include "c4_esp.h"
#include "spectators.h"
#include "projectile_trails.h"
#include "weapon_icons.h"
#include "helpers/fonts.h"
#include "menu.h"

namespace Render {
    inline ID3D11Device*           g_pDevice = nullptr;
    inline ID3D11DeviceContext*    g_pContext = nullptr;
    inline ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
    inline IDXGISwapChain*         g_pPinnedSwapChain = nullptr;
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

        fonts::init(io);
        g_weapon_icons.init(g_pDevice);

        ImGui_ImplWin32_Init(desc.OutputWindow);
        ImGui_ImplDX11_Init(g_pDevice, g_pContext);

        g_Init = true;
    }

    inline void render_frame(IDXGISwapChain* sc) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        int screen_w = (int)io.DisplaySize.x;
        int screen_h = (int)io.DisplaySize.y;
        ImDrawList* draw = ImGui::GetBackgroundDrawList();

        FrameState state = g_entity_reader.read_frame(screen_w, screen_h);

        if (g_settings.master_switch) {
            // Projectile Trails
            g_projectile_trails.set_view_matrix(state.view_matrix);
            g_projectile_trails.update_and_draw(draw, state.entity_list, screen_w, screen_h);

            // Player ESP & Chams
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

            // Spectators
            g_spectators.update(state.entity_list, state.local.pawn, state.local.controller);
            g_spectators.draw(screen_w);
        }

        // Render UI Menu
        if (g_settings.menu_open) {
            Menu::draw();
        }

        ImGui::Render();
        g_pContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}