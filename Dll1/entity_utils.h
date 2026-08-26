#pragma once
#include <Windows.h>
#include <cstdint>
#include <cstring>
#include "offsets.h"

namespace EntityList {
    constexpr size_t ENTRY_STRIDE = 112;     // 0x70
    constexpr size_t PAGE_HEADER = 16;       // 0x10
    constexpr size_t PAGE_PTR_SIZE = 8;
    constexpr uint32_t HANDLE_MASK = 0x7FFF;
    constexpr uint32_t PAGE_SHIFT = 9;
    constexpr uint32_t INDEX_MASK = 0x1FF;
    constexpr int MAX_PLAYERS = 64;

    inline uintptr_t get_page(uintptr_t list, uint32_t handle) {
        if (!list) return 0;
        uintptr_t addr = list + PAGE_PTR_SIZE * ((handle & HANDLE_MASK) >> PAGE_SHIFT) + PAGE_HEADER;
        return *reinterpret_cast<const uintptr_t*>(addr);
    }

    inline uintptr_t get_entry(uintptr_t page, uint32_t handle) {
        if (!page) return 0;
        uintptr_t addr = page + ENTRY_STRIDE * (handle & INDEX_MASK);
        return *reinterpret_cast<const uintptr_t*>(addr);
    }

    inline uintptr_t resolve_handle(uintptr_t list, uint32_t handle) {
        if (!handle) return 0;
        uintptr_t page = get_page(list, handle);
        if (!page) return 0;
        return get_entry(page, handle);
    }
}

inline uint32_t get_pawn_handle(uintptr_t controller) {
    if (!controller || controller < 0x10000 || controller > 0x7FFFFFFFFFFF) return 0;
    __try {
        return *reinterpret_cast<const uint32_t*>(controller + g_offsets.CCSPlayerController.m_hPlayerPawn);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

inline void read_player_name(uintptr_t controller, char* out, size_t max_len) {
    if (!out || max_len == 0) return;
    out[0] = '\0';
    if (!controller) return;

    __try {
        uintptr_t ptr = *reinterpret_cast<const uintptr_t*>(controller + g_offsets.CCSPlayerController.m_sSanitizedPlayerName);

        if (!ptr || ptr < 0x10000 || ptr > 0x7FFFFFFFFFFF) return;

        const char* src = reinterpret_cast<const char*>(ptr);
        size_t i = 0;
        for (; i < max_len - 1 && src[i] != '\0'; ++i) {
            char c = src[i];
            out[i] = ((unsigned char)c < 0x20) ? ' ' : c;
        }
        out[i] = '\0';
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        out[0] = '\0';
    }
}