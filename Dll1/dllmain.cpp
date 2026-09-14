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
#include "triggerbot.h"

// ASCII color (24-bit TrueColor)
#define C_MINT   "\033[38;2;0;255;180m"
#define C_SHADOW "\033[38;2;65;70;85m"
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

    // 1. Output: Enable TrueColor & set a 3000-line scrollback buffer
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwOutMode = 0;
    if (GetConsoleMode(hOut, &dwOutMode)) {
        dwOutMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwOutMode);
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        COORD newSize;
        newSize.X = csbi.dwSize.X;
        newSize.Y = 3000; // Scroll-to-top area
        SetConsoleScreenBufferSize(hOut, newSize);
    }

    // 2. Input: Disable QuickEdit Mode so the console does NOT FREEZE when clicked
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD dwInMode = 0;
    if (GetConsoleMode(hIn, &dwInMode)) {
        // You must include ENABLE_EXTENDED_FLAGS when changing QUICK_EDIT_MODE
        dwInMode &= ~ENABLE_QUICK_EDIT_MODE;
        dwInMode |= ENABLE_EXTENDED_FLAGS;
        SetConsoleMode(hIn, dwInMode);
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

    // 1. Create a debug console and display a banner
    init_console();
    print_dragonburn_banner();

    // 2. Wait until client.dll and gameoverlayrenderer64.dll are ready
    while (!GetModuleHandleA("client.dll") || !GetModuleHandleA("gameoverlayrenderer64.dll")) {
        Sleep(200);
    }

    // 3. Load configuration
    Config::load(get_dll_directory() + "config.ini");

    // 4. Hook Steam Overlay (DirectX Render)
    Hooks::hook_thread(lpParam);

    // 5. Start the Aimbot (dwViewAngles) and RCS thread
    start_aimbot_thread();
    start_triggerbot_thread();
    g_rcs.start();

    // 6. Exit button monitor loop (VK_INSERT)
    while (true) {
        if (g_settings.key_exit && (GetAsyncKeyState(g_settings.key_exit) & 0x8000)) {
            while (GetAsyncKeyState(g_settings.key_exit) & 0x8000) {
                Sleep(10);
            }
            break;
        }
        Sleep(100);
    }

    // 7. Cleaning sequence before the DLL is removed
    std::cout << "   " C_MUTED "[" C_RED "*" C_MUTED "] " C_RESET "Stopping background threads...\n";
    stop_aimbot_thread();
    stop_triggerbot_thread();
    g_rcs.stop();

    std::cout << "   " C_MUTED "[" C_RED "*" C_MUTED "] " C_RESET "Restoring Steam Overlay pointers...\n";
    Hooks::unhook();

    std::cout << "   " C_MUTED "[" C_RED "*" C_MUTED "] " C_RESET "Cleaning up ImGui and DirectX resources...\n";
    Render::shutdown();

    Sleep(250);
    cleanup_console();

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