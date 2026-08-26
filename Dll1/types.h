#pragma once
#include "../imgui/imgui.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

#include "settings.h"

struct Vec3 {
    float x, y, z;

    float dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    float length() const {
        return sqrtf(x * x + y * y + z * z);
    }

    float length_sqr() const {
        return x * x + y * y + z * z;
    }

    // Returns a normalized copy
    Vec3 normalized() const {
        float len = length();
        if (len < 1e-6f) return { 0,0,0 }; // Avoid division by zero
        return { x / len, y / len, z / len };
    }

    // Operators for Vec3 (if not present)
    Vec3 operator+(const Vec3& other) const { return { x + other.x, y + other.y, z + other.z }; }
    Vec3 operator-(const Vec3& other) const { return { x - other.x, y - other.y, z - other.z }; }
    Vec3 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar }; }
};
struct Matrix4x4 { float m[4][4]; };
struct CBoneData { Vec3 pos; float scale; float quat[4]; };

// Bone indices
static constexpr int BONE_HEAD = 7, BONE_NECK = 6, BONE_SPINE1 = 3,
BONE_SPINE2 = 4, BONE_PELVIS = 1,
BONE_LSHOULDER = 9, BONE_LELBOW = 10, BONE_LHAND = 11,
BONE_RSHOULDER = 13, BONE_RELBOW = 14, BONE_RHAND = 15,
BONE_LHIP = 17, BONE_LKNEE = 18, BONE_LFOOT = 19,
BONE_RHIP = 20, BONE_RKNEE = 21, BONE_RFOOT = 22, BONE_CHEST = 23;
static constexpr int MAX_BONE = 28;

// Simple skeleton connections for wire mode
struct BoneConn { int from, to; };
static constexpr BoneConn SKELETON_CONNECTIONS[] = {
    {BONE_HEAD, BONE_NECK}, {BONE_NECK, BONE_SPINE1},
    {BONE_SPINE1, BONE_SPINE2}, {BONE_SPINE2, BONE_PELVIS},
    {BONE_NECK, BONE_LSHOULDER}, {BONE_LSHOULDER, BONE_LELBOW}, {BONE_LELBOW, BONE_LHAND},
    {BONE_NECK, BONE_RSHOULDER}, {BONE_RSHOULDER, BONE_RELBOW}, {BONE_RELBOW, BONE_RHAND},
    {BONE_PELVIS, BONE_LHIP}, {BONE_LHIP, BONE_LKNEE}, {BONE_LKNEE, BONE_LFOOT},
    {BONE_PELVIS, BONE_RHIP}, {BONE_RHIP, BONE_RKNEE}, {BONE_RKNEE, BONE_RFOOT},
};

struct LimbDef {
    int bone_a, bone_b;
    float width_a, width_b;
};

static constexpr LimbDef BODY_LIMBS[] = {
    {BONE_NECK,    BONE_SPINE1,  8.5f, 10.0f},
    {BONE_SPINE1,  BONE_SPINE2,  10.0f, 9.5f},
    {BONE_SPINE2,  BONE_PELVIS,  9.5f,  9.0f},
    {BONE_LSHOULDER, BONE_LELBOW, 4.5f, 3.8f},
    {BONE_LELBOW,    BONE_LHAND,  3.8f, 2.8f},
    {BONE_RSHOULDER, BONE_RELBOW, 4.5f, 3.8f},
    {BONE_RELBOW,    BONE_RHAND,  3.8f, 2.8f},
    {BONE_LHIP,  BONE_LKNEE,  6.0f, 4.5f},
    {BONE_LKNEE, BONE_LFOOT,  4.5f, 3.2f},
    {BONE_RHIP,  BONE_RKNEE,  6.0f, 4.5f},
    {BONE_RKNEE, BONE_RFOOT,  4.5f, 3.2f},
    {BONE_NECK, BONE_LSHOULDER, 5.0f, 4.5f},
    {BONE_NECK, BONE_RSHOULDER, 5.0f, 4.5f},
    {BONE_PELVIS, BONE_LHIP, 7.0f, 6.0f},
    {BONE_PELVIS, BONE_RHIP, 7.0f, 6.0f},
};
static constexpr int BODY_LIMB_COUNT = sizeof(BODY_LIMBS) / sizeof(BODY_LIMBS[0]);

// Vertical bones for height measurement (head to feet)
static constexpr int BOX_HEIGHT_BONES[] = {
    BONE_HEAD, BONE_NECK, BONE_SPINE1, BONE_SPINE2, BONE_PELVIS,
    BONE_LHIP, BONE_RHIP, BONE_LKNEE, BONE_RKNEE,
    BONE_LFOOT, BONE_RFOOT,
};

// All bones for width reference only (used to detect lateral extent ratio)
static constexpr int BOX_ALL_BONES[] = {
    BONE_HEAD, BONE_NECK, BONE_SPINE1, BONE_SPINE2, BONE_PELVIS,
    BONE_LSHOULDER, BONE_RSHOULDER,
    BONE_LELBOW, BONE_RELBOW,
    BONE_LHAND, BONE_RHAND,
    BONE_LHIP, BONE_RHIP,
    BONE_LKNEE, BONE_RKNEE,
    BONE_LFOOT, BONE_RFOOT,
};

struct PlayerVisuals {
    ImVec2 screens[MAX_BONE];
    float depths[MAX_BONE]{};
    bool visible[MAX_BONE]{};
    int team = 0;
    int health = 0;
    bool valid = false;
    char name[128]{};
    char weapon[64]{};
    uint16_t weapon_def_index = 0;
    Vec3 origin{};
    Vec3 bones_world[MAX_BONE]{};

    Vec3 head_world{};      // bone 6
    Vec3 neck_world{};      // bone 5
    Vec3 chest_world{};     // bone 4
    Vec3 pelvis_world{};    // bone 0
    bool is_scoped = false;
    bool has_c4 = false;
    uint64_t bSpottedByMask = 0;
    bool is_local_spotted = false;
    bool m_bSpotted = false;
    float flash_duration = 0;
    bool bone_occluded[MAX_BONE]{};
};

struct RadarPlayer {
    float x, y, z;
    int team;
    int health;
    bool valid;
    bool is_spotted;
    char name[128];
};

inline ImU32 float4_to_col(const float c[4]) {
    return IM_COL32((int)(c[0] * 255), (int)(c[1] * 255),
        (int)(c[2] * 255), (int)(c[3] * 255));
}

inline ImU32 darken(ImU32 col, float f) {
    return IM_COL32(
        (int)(((col >> 0) & 0xFF) * f), (int)(((col >> 8) & 0xFF) * f),
        (int)(((col >> 16) & 0xFF) * f), (col >> 24) & 0xFF);
}

inline ImU32 alpha_mod(ImU32 col, float a) {
    int alpha = (int)(((col >> 24) & 0xFF) * a);
    return (col & 0x00FFFFFF) | ((alpha & 0xFF) << 24);
}

inline bool w2s(const Vec3& w, const Matrix4x4& vm, int sw, int sh, ImVec2& s) {
    float ww = vm.m[3][0] * w.x + vm.m[3][1] * w.y + vm.m[3][2] * w.z + vm.m[3][3];
    if (ww < 0.001f) return false;
    float inv = 1.0f / ww;
    float x = vm.m[0][0] * w.x + vm.m[0][1] * w.y + vm.m[0][2] * w.z + vm.m[0][3];
    float y = vm.m[1][0] * w.x + vm.m[1][1] * w.y + vm.m[1][2] * w.z + vm.m[1][3];
    s.x = sw * 0.5f + x * inv * sw * 0.5f;
    s.y = sh * 0.5f - y * inv * sh * 0.5f;
    return true;
}

inline bool w2s_depth(const Vec3& w, const Matrix4x4& vm, int sw, int sh,
    ImVec2& s, float& depth) {
    float ww = vm.m[3][0] * w.x + vm.m[3][1] * w.y + vm.m[3][2] * w.z + vm.m[3][3];
    if (ww < 0.001f) return false;
    depth = ww;
    float inv = 1.0f / ww;
    float x = vm.m[0][0] * w.x + vm.m[0][1] * w.y + vm.m[0][2] * w.z + vm.m[0][3];
    float y = vm.m[1][0] * w.x + vm.m[1][1] * w.y + vm.m[1][2] * w.z + vm.m[1][3];
    s.x = sw * 0.5f + x * inv * sw * 0.5f;
    s.y = sh * 0.5f - y * inv * sh * 0.5f;
    return true;
}