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
        if (!EntityList::is_valid_ptr(entity_list)) return;

        float current_time = static_cast<float>(ImGui::GetTime());

        __try {
            // Scan 2 main entity pages (1024 entities) to detect bombs without lag
            for (uint32_t page = 0; page < 2; page++) {
                uintptr_t page_addr = EntityList::get_page(entity_list, page * 512);
                if (!EntityList::is_valid_ptr(page_addr)) continue;

                for (int i = (page == 0 ? 65 : 0); i < 512; i++) {
                    uintptr_t entity = *reinterpret_cast<const uintptr_t*>(page_addr + EntityList::ENTRY_STRIDE * i);
                    if (!EntityList::is_valid_ptr(entity)) continue;

                    uintptr_t entity_identity = *reinterpret_cast<const uintptr_t*>(entity + 0x10);
                    if (!EntityList::is_valid_ptr(entity_identity)) continue;

                    uintptr_t designer_name_ptr = *reinterpret_cast<const uintptr_t*>(entity_identity + 0x20);
                    if (!EntityList::is_valid_ptr(designer_name_ptr)) continue;

                    int type = -1;
                    auto it = known_designers.find(designer_name_ptr);
                    if (it == known_designers.end()) {
                        char name[64] = { 0 };
                        const char* src = reinterpret_cast<const char*>(designer_name_ptr);
                        for (size_t k = 0; k < sizeof(name) - 1 && src[k] != '\0'; ++k) {
                            char c = src[k];
                            if ((unsigned char)c < 0x20 || (unsigned char)c > 0x7E) break;
                            name[k] = c;
                        }

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
                        if (!EntityList::is_valid_ptr(node)) continue;

                        Vec3 origin = *reinterpret_cast<const Vec3*>(node + 0xC8);

                        // Ignore if coordinates are {0,0,0} so that the path is not deleted
                        if (origin.x == 0.0f && origin.y == 0.0f && origin.z == 0.0f) continue;
                        if (origin.length_sqr() < 1.0f) continue;

                        if (active_trails.find(entity) == active_trails.end()) {
                            active_trails[entity].type = type;
                            active_trails[entity].path.push_back(origin);
                            active_trails[entity].last_update = current_time;
                        }
                        else {
                            float dist = distance_sq(active_trails[entity].path.back(), origin);
                            // Only add if there is actual movement
                            if (dist > 1.5f && dist < 500000.0f) {
                                active_trails[entity].path.push_back(origin);
                                active_trails[entity].last_update = current_time;
                            }
                            else if (dist <= 1.5f) {
                                active_trails[entity].last_update = current_time;
                            }
                        }
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        // Trace the path and clear bombs that have already detonated (no update for more than 2.5 seconds)
        for (auto it = active_trails.begin(); it != active_trails.end(); ) {
            if (current_time - it->second.last_update > 2.5f) {
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
        case 0: color = IM_COL32(200, 200, 200, 255); break; // Smoke
        case 1: color = IM_COL32(255, 110, 20, 255);  break; // Molotov
        case 2: color = IM_COL32(80, 220, 80, 255);   break; // Frag
        case 3: color = IM_COL32(255, 220, 50, 255);  break; // Flash
        default: color = IM_COL32(200, 200, 200, 255); break;
        }

        ImU32 glow_outer = (color & 0x00FFFFFF) | (35 << 24);
        ImU32 glow_inner = (color & 0x00FFFFFF) | (90 << 24);
        ImU32 core_color = (color & 0x00FFFFFF) | (255 << 24);

        for (size_t i = 0; i < path.size() - 1; i++) {
            ImVec2 p1, p2;
            if (project(path[i], sw, sh, p1) && project(path[i + 1], sw, sh, p2)) {
                dl->AddLine(p1, p2, glow_outer, 8.0f);
                dl->AddLine(p1, p2, glow_inner, 4.0f);
                dl->AddLine(p1, p2, core_color, 1.8f);
            }
        }

        ImVec2 last_p;
        if (project(path.back(), sw, sh, last_p)) {
            dl->AddCircleFilled(last_p, 9.0f, glow_outer);
            dl->AddCircleFilled(last_p, 5.0f, glow_inner);
            dl->AddCircleFilled(last_p, 2.5f, core_color);
            dl->AddCircle(last_p, 2.5f, IM_COL32(0, 0, 0, 255), 0, 1.0f);
        }
    }
};

inline ProjectileTrails g_projectile_trails;