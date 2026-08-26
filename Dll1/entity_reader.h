#pragma once
#pragma warning(disable: 6262)
#include <Windows.h>
#include <cmath>
#include <cstring>
#include <string>
#include <chrono>
#include <algorithm>
#include "types.h"
#include "offsets.h"
#include "entity_utils.h"
#include "bvh.h"

template<typename T>
inline T read_mem(uintptr_t addr) {
    if (!addr || addr < 0x10000 || addr > 0x7FFFFFFFFFFF) return T{};
    __try {
        return *reinterpret_cast<const T*>(addr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return T{};
    }
}

inline bool read_raw_mem(uintptr_t addr, void* dest, size_t size) {
    if (!addr || !dest || size == 0 || addr < 0x10000 || addr > 0x7FFFFFFFFFFF) return false;
    __try {
        memcpy(dest, reinterpret_cast<const void*>(addr), size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

inline float get_map_scale(const std::string& map_name) {
    if (map_name.find("ar_baggage") != std::string::npos) return 2.539062f;
    if (map_name.find("ar_shoots") != std::string::npos) return 2.687500f;
    if (map_name.find("cs_italy") != std::string::npos) return 4.6f;
    if (map_name.find("cs_office") != std::string::npos) return 4.1f;
    if (map_name.find("de_ancient") != std::string::npos) return 5.0f;
    if (map_name.find("de_anubis") != std::string::npos) return 5.220000f;
    if (map_name.find("de_dust2") != std::string::npos) return 4.4f;
    if (map_name.find("de_dust") != std::string::npos) return 6.0f;
    if (map_name.find("de_inferno") != std::string::npos) return 4.9f;
    if (map_name.find("de_mirage") != std::string::npos) return 5.0f;
    if (map_name.find("de_nuke") != std::string::npos) return 7.0f;
    if (map_name.find("de_overpass") != std::string::npos) return 5.2f;
    if (map_name.find("de_train") != std::string::npos) return 4.082077f;
    if (map_name.find("de_vertigo") != std::string::npos) return 4.0f;
    if (map_name.find("workshop_preview") != std::string::npos) return 1.699219f;
    return 5.0f;
}

struct WeaponInfo {
    uint16_t def_index;
    const char* name;
};

static constexpr WeaponInfo WEAPON_TABLE[] = {
    {1,   "Desert Eagle"}, {2,   "Dual Berettas"}, {3,   "Five-SeveN"}, {4,   "Glock-18"},
    {7,   "AK-47"}, {8,   "AUG"}, {9,   "AWP"}, {10,  "FAMAS"}, {11,  "G3SG1"},
    {13,  "Galil AR"}, {14,  "M249"}, {16,  "M4A4"}, {17,  "MAC-10"}, {19,  "P90"},
    {23,  "MP5-SD"}, {24,  "UMP-45"}, {25,  "XM1014"}, {26,  "PP-Bizon"}, {27,  "MAG-7"},
    {28,  "Negev"}, {29,  "Sawed-Off"}, {30,  "Tec-9"}, {31,  "Zeus x27"}, {32,  "P2000"},
    {33,  "MP7"}, {34,  "MP9"}, {35,  "Nova"}, {36,  "P250"}, {38,  "SCAR-20"},
    {39,  "SG 553"}, {40,  "SSG 08"}, {42,  "Knife"}, {43,  "Flashbang"}, {44,  "HE Grenade"},
    {45,  "Smoke"}, {46,  "Molotov"}, {47,  "Decoy"}, {48,  "Incendiary"}, {49,  "C4"},
    {59,  "Knife"}, {60,  "M4A1-S"}, {61,  "USP-S"}, {63,  "CZ75-Auto"}, {64,  "R8 Revolver"},
    {500, "Bayonet"}, {503, "Shadow Daggers"}, {505, "Flip Knife"}, {506, "Gut Knife"},
    {507, "Karambit"}, {508, "M9 Bayonet"}, {509, "Huntsman Knife"}, {512, "Falchion Knife"},
    {514, "Bowie Knife"}, {515, "Butterfly Knife"}, {516, "Ursus Knife"}, {517, "Navaja Knife"},
    {518, "Stiletto Knife"}, {519, "Talon Knife"}, {520, "Skeleton Knife"}, {521, "Nomad Knife"},
    {522, "Survival Knife"}, {523, "Paracord Knife"}, {525, "Classic Knife"}, {526, "Kukri Knife"}
};
static constexpr int WEAPON_TABLE_SIZE = sizeof(WEAPON_TABLE) / sizeof(WEAPON_TABLE[0]);

inline const WeaponInfo* lookup_weapon(uint16_t def_index) {
    for (int i = 0; i < WEAPON_TABLE_SIZE; i++)
        if (WEAPON_TABLE[i].def_index == def_index)
            return &WEAPON_TABLE[i];
    return nullptr;
}

struct CameraState {
    Vec3  origin{};
    Vec3  angles{};
    float fov = 90.0f;
    bool  valid = false;
};

struct LocalPlayerState {
    uintptr_t pawn = 0;
    uintptr_t observer_pawn = 0;
    uintptr_t controller = 0;
    int team = 0;
    float x = 0, y = 0, z = 0, yaw = 0;
    bool is_scoped = false;
    CameraState camera;
};

struct FrameState {
    Matrix4x4 view_matrix{};
    LocalPlayerState local;
    PlayerVisuals players[64];
    uintptr_t entity_list = 0;
    std::string map_name;
    float map_scale = 5.0f;
    uint16_t local_weapon_def_index = 0;
    int local_player_index = -1;
    int crosshair_entity_index = 0;
};

struct PawnSnapshot {
    static constexpr size_t SIZE = 0x2500;
    uint8_t buf[SIZE];

    template<typename T>
    T get(uintptr_t offset) const {
        if (offset + sizeof(T) > SIZE) return T{};
        T val;
        memcpy(&val, buf + offset, sizeof(T));
        return val;
    }
};

class EntityReader {
public:
    FrameState read_frame(int screen_w, int screen_h) {
        FrameState state{};
        uintptr_t client_base = Offsets::get_client_base();
        if (!client_base) return state;

        uintptr_t global_vars = read_mem<uintptr_t>(client_base + g_offsets.client.dwGlobalVars);
        if (global_vars) {
            uintptr_t current_map_ptr = read_mem<uintptr_t>(global_vars + 0x188);
            if (current_map_ptr != cached_map_ptr && current_map_ptr != 0) {
                char map_name[64] = { 0 };
                if (read_raw_mem(current_map_ptr, map_name, sizeof(map_name))) {
                    cached_map_scale = get_map_scale(map_name);
                    cached_map_ptr = current_map_ptr;
                    cached_map_name = map_name;
                }
            }
        }
        state.map_scale = cached_map_scale;
        state.map_name = cached_map_name;

        read_raw_mem(client_base + g_offsets.client.dwViewMatrix, &state.view_matrix, sizeof(Matrix4x4));

        state.local.pawn = read_mem<uintptr_t>(client_base + g_offsets.client.dwLocalPlayerPawn);
        state.local.controller = read_mem<uintptr_t>(client_base + g_offsets.client.dwLocalPlayerController);
        state.local.observer_pawn = read_mem<uintptr_t>(state.local.controller + g_offsets.CCSPlayerController.m_hObserverPawn);
        state.local_player_index = -1;

        if (state.local.pawn) {
            PawnSnapshot local_snap{};
            read_raw_mem(state.local.pawn, local_snap.buf, PawnSnapshot::SIZE);

            state.local.team = local_snap.get<int>(g_offsets.C_BaseEntity.m_iTeamNum);
            state.local.is_scoped = local_snap.get<bool>(g_offsets.C_CSPlayerPawn.m_bIsScoped);

            uintptr_t local_scene = local_snap.get<uintptr_t>(g_offsets.C_BaseEntity.m_pGameSceneNode);
            if (local_scene) {
                Vec3 origin = read_mem<Vec3>(local_scene + g_offsets.CGameSceneNode.m_vecAbsOrigin);
                state.local.x = origin.x;
                state.local.y = origin.y;
                state.local.z = origin.z;
            }
            state.local.yaw = atan2f(state.view_matrix.m[0][1], state.view_matrix.m[0][0]) * 180.0f / 3.14159265f - 90.0f;
            state.crosshair_entity_index = read_mem<int>(state.local.pawn + g_offsets.C_CSPlayerPawn.m_iIDEntIndex);
        }

        state.local.camera = read_camera(client_base);

        // 3. Entity List
        state.entity_list = read_mem<uintptr_t>(client_base + g_offsets.client.dwEntityList);
        if (!state.entity_list) return state;

        uintptr_t first_page = read_mem<uintptr_t>(state.entity_list + EntityList::PAGE_HEADER);
        if (!first_page) return state;

        static constexpr size_t PAGE_BUF_SIZE = EntityList::ENTRY_STRIDE * 512;
        static uint8_t page_buf[PAGE_BUF_SIZE];
        if (!read_raw_mem(first_page, page_buf, PAGE_BUF_SIZE)) return state;

        for (int i = 1; i < EntityList::MAX_PLAYERS; i++) {
            uintptr_t controller = 0;
            memcpy(&controller, page_buf + EntityList::ENTRY_STRIDE * (i & EntityList::INDEX_MASK), sizeof(uintptr_t));
            if (!controller) continue;

            if (controller == state.local.controller) {
                state.local_player_index = i - 1;
                continue;
            }

            read_player(state, controller, i, screen_w, screen_h);
        }

        if (state.local.pawn && state.entity_list) {
            char dummy_name[64] = { 0 };
            read_weapon(state.local.pawn, state.entity_list, dummy_name, sizeof(dummy_name), state.local_weapon_def_index);
        }

        return state;
    }

private:
    CBoneData bone_buf[MAX_BONE];
    uintptr_t cached_map_ptr = 0;
    float cached_map_scale = 5.0f;
    std::string cached_map_name;
    std::chrono::steady_clock::time_point last_spotted_time[64];

    CameraState read_camera(uintptr_t client_base) {
        CameraState cam;
        uintptr_t view_render = read_mem<uintptr_t>(client_base + g_offsets.client.dwViewRender);
        if (!view_render) return cam;

        uintptr_t view = view_render + 0x10;
        struct ViewData {
            Vec3  origin;
            Vec3  angles;
            float fov;
        } data{};

        if (!read_raw_mem(view, &data, sizeof(data))) return cam;

        if (data.fov <= 0.0f || data.fov > 170.0f) data.fov = 90.0f;

        cam.origin = data.origin;
        cam.angles = data.angles;
        cam.fov = data.fov;
        cam.valid = true;
        return cam;
    }

    static bool check_c4_in_inventory(uintptr_t weapon_services, uintptr_t entity_list) {
        uintptr_t my_weapons_base = weapon_services + g_offsets.CPlayer_WeaponServices.m_hMyWeapons;
        int count = read_mem<int>(my_weapons_base);
        if (count <= 0 || count > 8) return false;

        uintptr_t weapon_array = read_mem<uintptr_t>(my_weapons_base + 8);
        if (!weapon_array) return false;

        for (int i = 0; i < count; i++) {
            uint32_t handle = read_mem<uint32_t>(weapon_array + i * 4);
            if ((handle & 0xFFFFFF) == 0xFFFFFF) continue;

            uintptr_t weapon = EntityList::resolve_handle(entity_list, handle);
            if (!weapon) continue;

            uint16_t def_index = read_mem<uint16_t>(
                weapon + g_offsets.C_EconEntity.m_AttributeManager
                + g_offsets.C_AttributeContainer.m_Item
                + g_offsets.C_EconItemView.m_iItemDefinitionIndex);
            if (def_index == 49) return true;
        }
        return false;
    }

    void read_weapon(uintptr_t pawn, uintptr_t entity_list, char* out_name, size_t max_len, uint16_t& out_def_index) {
        out_name[0] = 0;
        out_def_index = 1;
        if (!pawn || !entity_list) return;

        uintptr_t weapon_services = read_mem<uintptr_t>(pawn + g_offsets.C_CSPlayerPawnBase.m_pWeaponServices);
        if (!weapon_services) return;

        uint32_t handle = read_mem<uint32_t>(weapon_services + g_offsets.CPlayer_WeaponServices.m_hActiveWeapon);
        if (!handle || handle == 0xFFFFFFFF) return;

        uint32_t ent_idx = handle & 0x7FFF;
        uintptr_t list_entry = read_mem<uintptr_t>(entity_list + 0x8 * (ent_idx >> 9) + 16);
        if (!list_entry) return;

        uintptr_t slot_addr = list_entry + EntityList::ENTRY_STRIDE * (ent_idx & 0x1FF);
        uintptr_t entity = read_mem<uintptr_t>(slot_addr);

        char designer_name[64] = { 0 };

        uintptr_t designer_ptr = read_mem<uintptr_t>(slot_addr + 0x20);
        if (designer_ptr) {
            read_raw_mem(designer_ptr, designer_name, sizeof(designer_name));
        }
        if (designer_name[0] == '\0' && entity) {
            uintptr_t identity = read_mem<uintptr_t>(entity + 0x10);
            if (identity) {
                designer_ptr = read_mem<uintptr_t>(identity + 0x20);
                if (designer_ptr) {
                    read_raw_mem(designer_ptr, designer_name, sizeof(designer_name));
                }
            }
        }

        if (designer_name[0] != '\0') {
            const char* s = designer_name;
            if (strstr(s, "ak47"))             out_def_index = 7;
            else if (strstr(s, "deagle"))      out_def_index = 1;
            else if (strstr(s, "awp"))         out_def_index = 9;
            else if (strstr(s, "m4a1_silencer")) out_def_index = 60;
            else if (strstr(s, "m4a1"))        out_def_index = 16;
            else if (strstr(s, "usp"))         out_def_index = 61;
            else if (strstr(s, "glock"))       out_def_index = 4;
            else if (strstr(s, "ssg08"))       out_def_index = 40;
            else if (strstr(s, "galilar"))     out_def_index = 13;
            else if (strstr(s, "famas"))       out_def_index = 10;
            else if (strstr(s, "aug"))         out_def_index = 8;
            else if (strstr(s, "sg556"))       out_def_index = 39;
            else if (strstr(s, "mp9"))         out_def_index = 34;
            else if (strstr(s, "mac10"))       out_def_index = 17;
            else if (strstr(s, "mp7"))         out_def_index = 33;
            else if (strstr(s, "mp5sd"))       out_def_index = 23;
            else if (strstr(s, "ump45"))       out_def_index = 24;
            else if (strstr(s, "p90"))         out_def_index = 19;
            else if (strstr(s, "bizon"))       out_def_index = 26;
            else if (strstr(s, "elite"))       out_def_index = 2;
            else if (strstr(s, "fiveseven"))   out_def_index = 3;
            else if (strstr(s, "hkp2000"))     out_def_index = 32;
            else if (strstr(s, "p250"))        out_def_index = 36;
            else if (strstr(s, "tec9"))        out_def_index = 30;
            else if (strstr(s, "cz75a"))       out_def_index = 63;
            else if (strstr(s, "revolver"))    out_def_index = 64;
            else if (strstr(s, "nova"))        out_def_index = 35;
            else if (strstr(s, "xm1014"))      out_def_index = 25;
            else if (strstr(s, "sawedoff"))    out_def_index = 29;
            else if (strstr(s, "mag7"))        out_def_index = 27;
            else if (strstr(s, "m249"))        out_def_index = 14;
            else if (strstr(s, "negev"))       out_def_index = 28;
            else if (strstr(s, "g3sg1"))       out_def_index = 11;
            else if (strstr(s, "scar20"))      out_def_index = 38;
            else if (strstr(s, "flash"))       out_def_index = 43;
            else if (strstr(s, "hegrenade"))   out_def_index = 44;
            else if (strstr(s, "smoke"))       out_def_index = 45;
            else if (strstr(s, "molotov"))     out_def_index = 46;
            else if (strstr(s, "incgrenade"))  out_def_index = 48;
            else if (strstr(s, "decoy"))       out_def_index = 47;
            else if (strstr(s, "taser"))       out_def_index = 31;
            else if (strstr(s, "knife"))       out_def_index = 42;
            else                               out_def_index = 1;

            const WeaponInfo* info = lookup_weapon(out_def_index);
            if (info)
                snprintf(out_name, max_len, "%s", info->name);
            else
                snprintf(out_name, max_len, "%s", designer_name);
        }
    }

    void read_player(FrameState& state, uintptr_t controller, int i, int screen_w, int screen_h) {
        char name[128]{};
        read_player_name(controller, name, sizeof(name));

        uint32_t pawn_handle = get_pawn_handle(controller);
        if (!pawn_handle) return;

        uintptr_t pawn = EntityList::resolve_handle(state.entity_list, pawn_handle);
        if (!pawn || pawn == state.local.pawn) return;

        static PawnSnapshot snap{};
        if (!read_raw_mem(pawn, snap.buf, PawnSnapshot::SIZE)) return;

        int health = snap.get<int>(g_offsets.C_BaseEntity.m_iHealth);
        if (health <= 0) return;

        int team = snap.get<int>(g_offsets.C_BaseEntity.m_iTeamNum);

        uintptr_t scene_node = snap.get<uintptr_t>(g_offsets.C_BaseEntity.m_pGameSceneNode);
        if (!scene_node) return;

        Vec3 origin = read_mem<Vec3>(scene_node + g_offsets.CGameSceneNode.m_vecAbsOrigin);

        uintptr_t bone_array = read_mem<uintptr_t>(scene_node + g_offsets.CSkeletonInstance.m_modelState + 0x80);
        if (!bone_array) return;

        if (!read_raw_mem(bone_array, bone_buf, sizeof(bone_buf))) return;

        auto& player = state.players[i];
        player.valid = true;
        player.team = team;
        player.health = health;
        player.is_scoped = snap.get<bool>(g_offsets.C_CSPlayerPawn.m_bIsScoped);
        player.origin = origin;
        player.head_world = bone_buf[BONE_HEAD].pos;
        player.neck_world = bone_buf[BONE_NECK].pos;
        player.chest_world = bone_buf[BONE_CHEST].pos;
        player.pelvis_world = bone_buf[BONE_PELVIS].pos;

        memcpy(player.name, name, 128);

        read_weapon(pawn, state.entity_list, player.weapon, sizeof(player.weapon), player.weapon_def_index);

        {
            uintptr_t ws = read_mem<uintptr_t>(pawn + g_offsets.C_CSPlayerPawnBase.m_pWeaponServices);
            if (ws) player.has_c4 = check_c4_in_inventory(ws, state.entity_list);
        }

        for (int b = 0; b < MAX_BONE; b++) {
            player.bones_world[b] = bone_buf[b].pos;
        }

        player.bSpottedByMask = snap.get<uint64_t>(
            g_offsets.C_CSPlayerPawn.m_entitySpottedState
            + g_offsets.EntitySpottedState_t.m_bSpottedByMask);

        const bool is_spotted_ingame = snap.get<bool>(
            g_offsets.C_CSPlayerPawn.m_entitySpottedState
            + g_offsets.EntitySpottedState_t.m_bSpotted);
        player.m_bSpotted = is_spotted_ingame;

        // BVH Trace
        {
            bool traced = false;
            static constexpr int BONES_TO_TRACE[] = {
                BONE_HEAD, BONE_NECK, BONE_SPINE1, BONE_SPINE2, BONE_PELVIS,
                BONE_LSHOULDER, BONE_LELBOW, BONE_LHAND,
                BONE_RSHOULDER, BONE_RELBOW, BONE_RHAND,
                BONE_LHIP, BONE_LKNEE, BONE_LFOOT,
                BONE_RHIP, BONE_RKNEE, BONE_RFOOT,
            };

            if (g_bvh.valid() && g_bvh.count() > 0 && (state.local.camera.valid || state.local.pawn)) {
                Vec3 cam_eye = state.local.camera.valid ? state.local.camera.origin : Vec3{ state.local.x, state.local.y, state.local.z + 64.0f };
                for (int b : BONES_TO_TRACE) {
                    auto tr = g_bvh.trace_ray(cam_eye, bone_buf[b].pos);
                    player.bone_occluded[b] = tr.hit && tr.fraction <= 0.97f;
                }
                traced = true;
            }
            if (!traced) {
                bool spotted_by_local = false;
                if (state.local_player_index >= 0 && player.bSpottedByMask != 0)
                    spotted_by_local = (player.bSpottedByMask >> state.local_player_index) & 1u;
                bool is_spotted = is_spotted_ingame || spotted_by_local;
                for (int b = 0; b < MAX_BONE; b++)
                    player.bone_occluded[b] = !is_spotted;
            }
            player.is_local_spotted = true;
        }

        if (state.crosshair_entity_index > 0) {
            uint32_t pawn_ent_index = pawn_handle & EntityList::HANDLE_MASK;
            if (pawn_ent_index == (uint32_t)state.crosshair_entity_index) {
                for (int b = 0; b < MAX_BONE; b++)
                    player.bone_occluded[b] = false;
            }
        }

        player.flash_duration = snap.get<float>(g_offsets.C_CSPlayerPawnBase.m_flFlashDuration);

        auto now = std::chrono::steady_clock::now();
        if (is_spotted_ingame) last_spotted_time[i] = now;

        auto time_since_spotted = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_spotted_time[i]).count();
        bool is_recently_spotted = (time_since_spotted < 1000);
    }
};