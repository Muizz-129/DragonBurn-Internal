#include <Windows.h>
#include "hooks.h"
#include "utils.h"
#include "config.h"
#include "aimbot.h"

DWORD WINAPI MainThread(LPVOID lpParam) {
    // Load settings from config file
    Config::load(get_dll_directory() + _xor_("config.ini").c_str());

    // Start aimbot worker thread
    start_aimbot_thread();

    // Initialize Steam overlay hook
    Hooks::hook_thread(lpParam);

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