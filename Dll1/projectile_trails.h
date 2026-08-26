#pragma once
#include <vector>
#include <unordered_map>
#include "../imgui/imgui.h"
#include "types.h"
#include "entity_utils.h"

struct Trail {
    std::vector<Vec3> path;
    int type = 0;
    float last_update = 0.0f;
};

class ProjectileTrails {
public:
    std::unordered_map<uintptr_t, Trail> active_trails;
    std::unordered_map<uintptr_t, int> known_designers;

    Matrix4x4 view_matrix_{};

    void set_view_matrix(const Matrix4x4& vm) { view_matrix_ = vm; }

    void update_and_draw(ImDrawList* draw_list, uintptr_t entity_list, int sw, int sh) {
        if (!entity_list) return;

        float current_time = static_cast<float>(ImGui::GetTime());

        static int current_chunk = 0;
        int chunk = current_chunk;
        current_chunk = (current_chunk + 1) % 4;

        uintptr_t list_entry = *reinterpret_cast<const uintptr_t*>(entity_list + (8 * chunk) + 16);
        if (list_entry) {
            static constexpr size_t PAGE_BUF_SIZE = EntityList::ENTRY_STRIDE * 512;
            static uint8_t page_buf[PAGE_BUF_SIZE];

            memcpy(page_buf, reinterpret_cast<const void*>(list_entry), PAGE_BUF_SIZE);

            for (int i = 0; i < 512; i++) {
                int index = chunk * 512 + i;
                if (index < 65) continue;

                uintptr_t entity;
                memcpy(&entity, page_buf + EntityList::ENTRY_STRIDE * i, sizeof(uintptr_t));
                if (!entity) continue;

                uintptr_t entity_identity = *reinterpret_cast<const uintptr_t*>(entity + 0x10);
                if (!entity_identity) continue;

                uintptr_t designer_name_ptr = *reinterpret_cast<const uintptr_t*>(entity_identity + 0x20);
                if (!designer_name_ptr) continue;

                int type = -1;
                auto it = known_designers.find(designer_name_ptr);
                if (it == known_designers.end()) {
                    char name[64] = { 0 };
                    memcpy(name, reinterpret_cast<const void*>(designer_name_ptr), sizeof(name) - 1);

                    if (strstr(name, "smokegrenade_projectile")) type = 0;
                    else if (strstr(name, "molotov_projectile") || strstr(name, "incendiarygrenade_projectile")) type = 1;
                    else if (strstr(name, "hegrenade_projectile")) type = 2;
                    else if (strstr(name, "flashbang_projectile")) type = 3;
                    else if (strstr(name, "decoy_projectile")) type = 4;

                    if (name[0] != '\0') {
                        known_designers[designer_name_ptr] = type;
                    }
                }
                else {
                    type = it->second;
                }

                if (type != -1) {
                    uintptr_t node = *reinterpret_cast<const uintptr_t*>(entity + 0x330);
                    if (!node) continue;

                    Vec3 origin = *reinterpret_cast<const Vec3*>(node + 0xC8);

                    if (active_trails.find(entity) == active_trails.end()) {
                        active_trails[entity].type = type;
                        active_trails[entity].path.push_back(origin);
                        active_trails[entity].last_update = current_time;
                    }
                    else {
                        float dist = distance_sq(active_trails[entity].path.back(), origin);
                        if (dist > 500000.0f) {
                            active_trails[entity].path.clear();
                            active_trails[entity].type = type;
                            active_trails[entity].path.push_back(origin);
                            active_trails[entity].last_update = current_time;
                        }
                        else if (dist > 1.0f) {
                            active_trails[entity].path.push_back(origin);
                            active_trails[entity].last_update = current_time;
                        }
                    }
                }
            }
        }

        for (auto it = active_trails.begin(); it != active_trails.end(); ) {
            if (current_time - it->second.last_update > 1.5f) {
                it = active_trails.erase(it);
                continue;
            }
            draw_trail(draw_list, it->second.path, it->second.type, sw, sh);
            ++it;
        }
    }

private:
    float distance_sq(const Vec3& a, const Vec3& b) {
        float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    }

    bool project(const Vec3& w, int sw, int sh, ImVec2& out) {
        const float* m = &view_matrix_.m[0][0];
        float ww = m[12] * w.x + m[13] * w.y + m[14] * w.z + m[15];
        if (ww < 0.001f) return false;
        float inv = 1.f / ww;
        float x = m[0] * w.x + m[1] * w.y + m[2] * w.z + m[3];
        float y = m[4] * w.x + m[5] * w.y + m[6] * w.z + m[7];
        out.x = sw * 0.5f + x * inv * sw * 0.5f;
        out.y = sh * 0.5f - y * inv * sh * 0.5f;
        return true;
    }

    void draw_trail(ImDrawList* dl, const std::vector<Vec3>& path, int type, int sw, int sh) {
        if (path.size() < 2) return;

        ImU32 color;
        switch (type) {
        case 0: color = IM_COL32(180, 180, 180, 255); break;
        case 1: color = IM_COL32(255, 120, 30, 255); break;
        case 2: color = IM_COL32(80, 220, 80, 255); break;
        case 3: color = IM_COL32(255, 255, 100, 255); break;
        default: color = IM_COL32(200, 200, 200, 255); break;
        }

        ImU32 glow_outer = (color & 0x00FFFFFF) | (25 << 24);
        ImU32 glow_inner = (color & 0x00FFFFFF) | (80 << 24);
        ImU32 core_color = (color & 0x00FFFFFF) | (255 << 24);

        for (size_t i = 0; i < path.size() - 1; i++) {
            ImVec2 p1, p2;
            if (project(path[i], sw, sh, p1) && project(path[i + 1], sw, sh, p2)) {
                dl->AddLine(p1, p2, glow_outer, 10.0f);
                dl->AddLine(p1, p2, glow_inner, 5.0f);
                dl->AddLine(p1, p2, core_color, 2.0f);
            }
        }

        ImVec2 last_p;
        if (project(path.back(), sw, sh, last_p)) {
            dl->AddCircleFilled(last_p, 12.0f, glow_outer);
            dl->AddCircleFilled(last_p, 7.0f, glow_inner);
            dl->AddCircleFilled(last_p, 3.5f, core_color);
            dl->AddCircle(last_p, 3.5f, IM_COL32(0, 0, 0, 255), 0, 1.0f);
        }
    }
};

inline ProjectileTrails g_projectile_trails;