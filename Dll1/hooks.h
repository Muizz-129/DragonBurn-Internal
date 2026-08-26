#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <vector>
#include <cstdio>
#include "render.h"

namespace Hooks {
    typedef HRESULT(NTAPI* Present_t)(IDXGISwapChain*, UINT, UINT);
    typedef HRESULT(NTAPI* ResizeBuffers_t)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    inline Present_t       oPresent = nullptr;
    inline ResizeBuffers_t oResizeBuffers = nullptr;

    inline Present_t* g_pPresentPtr = nullptr;
    inline ResizeBuffers_t* g_pResizePtr = nullptr;

#define PRESENT_PATTERN "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 41 8B F0"
#define STEAM_FALLBACK_PRESENT_OFFSET 0x162200

    inline void show_error_console(const char* error_message, HMODULE hModule) {
        AllocConsole();
        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);

        printf("\n============================================\n");
        printf(" [!] DragonBurn Hook Error: %s\n", error_message);
        printf(" [!] Unloading module to prevent game crash...\n");
        printf("============================================\n\n");
        printf("Closing console and exiting in 5 seconds...\n");

        Sleep(5000);

        if (fp) fclose(fp);
        FreeConsole();

        if (hModule) {
            FreeLibraryAndExitThread(hModule, 0);
        }
    }

    inline uintptr_t scan_steam_pattern(HMODULE hModule, const char* signature) {
        if (!hModule) return 0;
        auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
        if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) return 0;
        auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>((uintptr_t)hModule + dos_header->e_lfanew);
        auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;

        auto pattern_to_byte = [](const char* pattern) {
            std::vector<int> bytes;
            auto start = const_cast<char*>(pattern);
            auto end = start + strlen(pattern);
            for (auto current = start; current < end; ++current) {
                if (*current == '?') {
                    ++current;
                    if (*current == '?') ++current;
                    bytes.push_back(-1);
                }
                else {
                    bytes.push_back(strtoul(current, &current, 16));
                }
            }
            return bytes;
            };

        auto pattern_bytes = pattern_to_byte(signature);
        auto scan_bytes = reinterpret_cast<uint8_t*>(hModule);
        auto s = pattern_bytes.size();
        auto d = pattern_bytes.data();

        for (size_t i = 0; i < size_of_image - s; ++i) {
            bool found = true;
            for (size_t j = 0; j < s; ++j) {
                if (scan_bytes[i + j] != d[j] && d[j] != -1) {
                    found = false;
                    break;
                }
            }
            if (found) return reinterpret_cast<uintptr_t>(&scan_bytes[i]);
        }
        return 0;
    }

    inline HRESULT NTAPI hkResizeBuffers(IDXGISwapChain* sc, UINT bc, UINT w, UINT h, DXGI_FORMAT fmt, UINT flags) {
        if (Render::g_pPinnedSwapChain && sc == Render::g_pPinnedSwapChain) {
            Render::cleanup_render_target();
        }

        HRESULT hr = oResizeBuffers(sc, bc, w, h, fmt, flags);

        if (Render::g_pPinnedSwapChain && sc == Render::g_pPinnedSwapChain && Render::g_pDevice) {
            Render::create_render_target(sc);
        }
        return hr;
    }

    inline HRESULT NTAPI hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
        if (!Render::g_Init) {
            if (++Render::g_SkipCount >= 120) {
                Render::init_imgui(sc);
                if (Render::g_Init) Render::g_pPinnedSwapChain = sc;
            }
            return oPresent(sc, sync, flags);
        }

        if (Render::g_pPinnedSwapChain && sc != Render::g_pPinnedSwapChain)
            return oPresent(sc, sync, flags);

        Render::render_frame(sc);

        return oPresent(sc, sync, flags);
    }

    inline DWORD WINAPI hook_thread(LPVOID lpParam) {
        HMODULE hModule = static_cast<HMODULE>(lpParam);

        HMODULE hOverlay = nullptr;
        for (int i = 0; i < 100 && !hOverlay; i++) {
            hOverlay = GetModuleHandleA("GameOverlayRenderer64.dll");
            if (!hOverlay) Sleep(100);
        }

        if (!hOverlay) {
            show_error_console("Failed to locate GameOverlayRenderer64.dll. Ensure Steam overlay is enabled.", hModule);
            return 0;
        }

        Present_t* pPresentPtr = nullptr;
        ResizeBuffers_t* pResizePtr = nullptr;

        // 2. Pattern scan Steam overlay
        uintptr_t fn_present_wrapper = scan_steam_pattern(hOverlay, PRESENT_PATTERN);
        if (fn_present_wrapper) {
            for (int i = 0; i < 64; ++i) {
                uint8_t* p = (uint8_t*)(fn_present_wrapper + i);
                if (p[0] == 0x48 && p[1] == 0x8B && p[2] == 0x05) {
                    int32_t disp = *(int32_t*)(p + 3);
                    pPresentPtr = (Present_t*)(p + 7 + disp);
                    pResizePtr = (ResizeBuffers_t*)((uintptr_t)pPresentPtr + sizeof(void*));
                    break;
                }
            }
        }

        if (!pPresentPtr) {
            pPresentPtr = (Present_t*)((BYTE*)hOverlay + STEAM_FALLBACK_PRESENT_OFFSET);
            pResizePtr = (ResizeBuffers_t*)((BYTE*)hOverlay + STEAM_FALLBACK_PRESENT_OFFSET + 0x8);
        }

        for (int i = 0; i < 100; i++) {
            if (pPresentPtr && *pPresentPtr) break;
            Sleep(100);
        }

        if (!pPresentPtr || !*pPresentPtr) {
            show_error_console("Failed to locate Present hook pointer. Signatures might be outdated.", hModule);
            return 0;
        }

        g_pPresentPtr = pPresentPtr;
        g_pResizePtr = pResizePtr;

        oPresent = *g_pPresentPtr;
        oResizeBuffers = *g_pResizePtr;
        DWORD oldProt = 0;
        if (VirtualProtect(g_pPresentPtr, sizeof(void*) * 2, PAGE_READWRITE, &oldProt)) {
            *g_pPresentPtr = (Present_t)hkPresent;
            *g_pResizePtr = (ResizeBuffers_t)hkResizeBuffers;
            VirtualProtect(g_pPresentPtr, sizeof(void*) * 2, oldProt, &oldProt);
        }

        return 0;
    }

    inline void unhook() {
        if (g_pPresentPtr && oPresent && g_pResizePtr && oResizeBuffers) {
            DWORD oldProt = 0;
            if (VirtualProtect(g_pPresentPtr, sizeof(void*) * 2, PAGE_READWRITE, &oldProt)) {
                *g_pPresentPtr = oPresent;
                *g_pResizePtr = oResizeBuffers;
                VirtualProtect(g_pPresentPtr, sizeof(void*) * 2, oldProt, &oldProt);
            }

            g_pPresentPtr = nullptr;
            g_pResizePtr = nullptr;
            oPresent = nullptr;
            oResizeBuffers = nullptr;
        }
    }
}