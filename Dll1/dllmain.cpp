#include <Windows.h>
#include <iostream>
#include <cstdio>
#include "hooks.h"
#include "render.h"
#include "utils.h"
#include "config.h"
#include "aimbot.h"
#include "rcs.h"
#include "settings.h"

// ASCII color (24-bit TrueColor)
#define C_MINT   "\033[38;2;0;255;180m"  // ASCII front page (Neon Mint)
#define C_SHADOW "\033[38;2;65;70;85m"   // Background shadow (Dark Charcoal)
#define C_MUTED  "\033[38;2;120;125;140m"
#define C_GREEN  "\033[38;2;0;255;128m"
#define C_RED    "\033[38;2;255;70;70m"
#define C_WARN   "\033[38;2;255;190;0m"
#define C_RESET  "\033[0m"

static void init_console() {
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN$", "r", stdin);

    SetConsoleTitleA("DragonBurn CS2 - Internal Debug");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Enable ANSI escape sequences for TrueColor and shadow processing
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
}

static void print_dragonburn_banner() {
    std::cout <<
        "\n"
        C_MINT   "   ██████╗" C_SHADOW "░" C_MINT "██████╗" C_SHADOW "░░" C_MINT "█████╗" C_SHADOW "░░" C_MINT "██████╗" C_SHADOW "░░" C_MINT "██████╗" C_SHADOW "░" C_MINT "███╗" C_SHADOW "░░" C_MINT "██╗██████╗" C_SHADOW "░" C_MINT "██╗" C_SHADOW "░░░" C_MINT "██╗██████╗" C_SHADOW "░" C_MINT "███╗" C_SHADOW "░░" C_MINT "██╗\n"
        C_MINT   "   ██╔══██╗██╔══██╗██╔══██╗██╔════╝" C_SHADOW "░" C_MINT "██╔═══██╗████╗" C_SHADOW "░" C_MINT "██║██╔══██╗██║" C_SHADOW "░░░" C_MINT "██║██╔══██╗████╗" C_SHADOW "░" C_MINT "██║\n"
        C_MINT   "   ██║" C_SHADOW "░░" C_MINT "██║██████╔╝███████║██║" C_SHADOW "░░" C_MINT "███╗██║" C_SHADOW "░░░" C_MINT "██║██╔██╗██║██████╔╝██║" C_SHADOW "░░░" C_MINT "██║██████╔╝██╔██╗██║\n"
        C_MINT   "   ██║" C_SHADOW "░░" C_MINT "██║██╔══██╗██╔══██║██║" C_SHADOW "░░░" C_MINT "██║██║" C_SHADOW "░░░" C_MINT "██║██║╚████║██╔══██╗██║" C_SHADOW "░░░" C_MINT "██║██╔══██╗██║╚████║\n"
        C_MINT   "   ██████╔╝██║" C_SHADOW "░░" C_MINT "██║██║" C_SHADOW "░░" C_MINT "██║╚██████╔╝╚██████╔╝██║" C_SHADOW "░" C_MINT "╚███║██████╔╝╚██████╔╝██║" C_SHADOW "░░" C_MINT "██║██║" C_SHADOW "░" C_MINT "╚███║\n"
        C_SHADOW "   ╚═════╝░╚═╝░░╚═╝╚═╝░░╚═╝░╚═════╝░░╚═════╝░╚═╝░░╚══╝╚═════╝░░╚═════╝░╚═╝░░╚═╝╚═╝░░╚══╝\n"
        C_RESET  "\n";

    std::cout << "   " C_MUTED "[" C_GREEN "+" C_MUTED "] " C_RESET "Injected successfully into CS2 process\n";
    std::cout << "   " C_MUTED "[" C_GREEN "+" C_MUTED "] " C_RESET "Offsets & modules synchronized\n";
    std::cout << "   " C_MUTED "[" C_WARN "!" C_MUTED "] " C_RESET "Press [INSERT] to unhook & eject safely\n\n";
}

static void cleanup_console() {
    fclose(stdout);
    fclose(stdin);
    FreeConsole();
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    HMODULE hModule = reinterpret_cast<HMODULE>(lpParam);

    // 1. Cipta konsol debug dan paparkan banner
    init_console();
    print_dragonburn_banner();

    // 2. Tunggu sehingga client.dll dan gameoverlayrenderer64.dll sedia dimuatkan
    while (!GetModuleHandleA("client.dll") || !GetModuleHandleA("gameoverlayrenderer64.dll")) {
        Sleep(200);
    }

    // 3. Muat konfigurasi
    Config::load(get_dll_directory() + "config.ini");

    // 4. Pasang cangkuk Steam Overlay
    Hooks::hook_thread(lpParam);

    // Mulakan bebenang Aimbot dan RCS
    start_aimbot_thread();
    g_rcs.start();

    // 5. Gelung pemantau butang keluar (VK_INSERT)
    while (true) {
        if (g_settings.key_exit && (GetAsyncKeyState(g_settings.key_exit) & 0x8000)) {
            // Tunggu butang dilepaskan supaya tidak trigger berulang kali
            while (GetAsyncKeyState(g_settings.key_exit) & 0x8000) {
                Sleep(10);
            }
            break;
        }
        Sleep(100);
    }

    // 6. Rutin pembersihan sebelum DLL keluar (Wajib ikut turutan ini)
    std::cout << "   " C_MUTED "[" C_RED "*" C_MUTED "] " C_RESET "Stopping background threads...\n";
    stop_aimbot_thread();
    g_rcs.stop();

    std::cout << "   " C_MUTED "[" C_RED "*" C_MUTED "] " C_RESET "Restoring Steam Overlay pointers...\n";
    Hooks::unhook(); // Pulihkan pointer Steam asal

    // Beri sedikit masa supaya panggilan render frame semasa selesai
    Sleep(150);

    std::cout << "   " C_MUTED "[" C_RED "*" C_MUTED "] " C_RESET "Cleaning up ImGui and DirectX resources...\n";
    Render::shutdown(); // Bersihkan ImGui, WindowProc hook, dan D3D resources

    cleanup_console();

    // Padam DLL dari memori CS2 dan tamatkan bebenang
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

        HANDLE hThread = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        if (hThread) {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}