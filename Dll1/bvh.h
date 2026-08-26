#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>
#include <shared_mutex>
#include <unordered_set>

#include "types.h"

namespace bvh_mem {
	template<typename T>
	inline T read(uintptr_t addr) {
		if (!addr) return T{};
		return *reinterpret_cast<const T*>(addr);
	}

	inline bool read_raw(uintptr_t addr, void* dest, size_t size) {
		if (!addr || !dest) return false;
		memcpy(dest, reinterpret_cast<const void*>(addr), size);
		return true;
	}

	inline uintptr_t resolve_rip(uintptr_t inst_addr, int32_t offset = 3, int32_t length = 7) {
		if (!inst_addr) return 0;
		int32_t rel = *reinterpret_cast<const int32_t*>(inst_addr + offset);
		return inst_addr + length + rel;
	}

	inline uintptr_t find_pattern(uintptr_t module_base, const char* signature) {
		if (!module_base) return 0;
		auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_base);
		if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) return 0;
		auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(module_base + dos_header->e_lfanew);
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
		auto scan_bytes = reinterpret_cast<uint8_t*>(module_base);
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

	inline uintptr_t find_vtable(uintptr_t module_base, const char* class_name) {
		if (!module_base) return 0;
		auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_base);
		if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) return 0;
		auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(module_base + dos_header->e_lfanew);
		auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;

		std::string mangled = ".?AV";
		mangled += class_name;
		mangled += "@@";

		uintptr_t type_desc = 0;
		auto scan_bytes = reinterpret_cast<const char*>(module_base);
		for (size_t i = 0; i < size_of_image - mangled.length(); ++i) {
			if (memcmp(&scan_bytes[i], mangled.c_str(), mangled.length()) == 0) {
				type_desc = reinterpret_cast<uintptr_t>(&scan_bytes[i]) - 0x10;
				break;
			}
		}
		if (!type_desc) return 0;

		uint32_t type_desc_rva = static_cast<uint32_t>(type_desc - module_base);
		uintptr_t col = 0;
		for (size_t i = 0; i < size_of_image - sizeof(uint32_t); ++i) {
			if (*reinterpret_cast<const uint32_t*>(&scan_bytes[i]) == type_desc_rva) {
				if (i >= 0x0C) {
					col = reinterpret_cast<uintptr_t>(&scan_bytes[i - 0x0C]);
					if (*reinterpret_cast<const uint32_t*>(col) == 1) break;
				}
			}
		}
		if (!col) return 0;

		for (size_t i = 0; i < size_of_image - sizeof(uintptr_t); ++i) {
			if (*reinterpret_cast<const uintptr_t*>(&scan_bytes[i]) == col) {
				return reinterpret_cast<uintptr_t>(&scan_bytes[i + sizeof(uintptr_t)]);
			}
		}
		return 0;
	}
}

class bvh
{
public:
	struct surface_info
	{
		float penetration{};
		std::uint16_t surface_type{};
		std::uint8_t global_index{ 255 };
	};

	struct global_surface_entry
	{
		float unk_00{};
		float unk_04{};
		float penetration_mod{};
		float unk_0C{};
		float unk_10{};
		std::uint16_t surface_type{};
		std::uint16_t pad{};
		std::uint8_t pad2[8]{};
	};

	struct triangle
	{
		Vec3 v0{};
		Vec3 v1{};
		Vec3 v2{};
		surface_info surface{};
	};

	struct trace_result
	{
		bool hit{};
		float fraction{};
		float distance{};
		Vec3 end_pos{};
		Vec3 normal{};
		surface_info surface{};
		std::int32_t triangle_index{ -1 };
	};

	struct hit_entry
	{
		float distance{};
		float fraction{};
		Vec3 position{};
		Vec3 normal{};
		surface_info surface{};
		std::int32_t triangle_index{ -1 };
		bool is_enter{ true };
	};

	struct penetration_segment
	{
		float enter_fraction{};
		float exit_fraction{};
		float enter_distance{};
		float exit_distance{};
		Vec3 enter_pos{};
		Vec3 exit_pos{};
		surface_info enter_surface{};
		surface_info exit_surface{};
		float thickness{};
		float min_pen_mod{};
	};

	void parse();
	void clear();

	[[nodiscard]] trace_result trace_ray(const Vec3& start, const Vec3& end, std::int32_t exclude_tri = -1) const;
	[[nodiscard]] std::vector<hit_entry> trace_ray_all(const Vec3& start, const Vec3& end) const;
	[[nodiscard]] std::vector<penetration_segment> build_segments(const std::vector<hit_entry>& hits, float ray_length) const;

	[[nodiscard]] const std::vector<triangle>& triangles() const;
	[[nodiscard]] std::size_t count() const;
	[[nodiscard]] bool valid() const;

private:
	struct aabb
	{
		float mins[3]{ 1e12f, 1e12f, 1e12f };
		float maxs[3]{ -1e12f, -1e12f, -1e12f };

		void expand(const Vec3& p);
		void expand(const aabb& o);
		[[nodiscard]] int longest_axis() const;
		[[nodiscard]] bool intersects_ray(const float origin[3], const float inv_dir[3], float max_t) const;
	};

	struct bvh_node
	{
		aabb bounds{};
		std::int32_t left{ -1 };
		std::int32_t right{ -1 };
		std::int32_t tri_start{};
		std::int32_t tri_count{};
	};

	void rebuild_accel();
	std::int32_t build_recursive(std::int32_t start, std::int32_t end, std::int32_t depth);

	std::vector<triangle> m_triangles{};
	mutable std::shared_mutex m_mutex{};

	std::vector<bvh_node> m_nodes{};
	std::vector<std::int32_t> m_indices{};
	std::vector<aabb> m_tri_bounds{};
	std::vector<float> m_centroids{};

	static constexpr auto k_max_leaf_tris{ 8 };
	static constexpr auto k_max_depth{ 48 };
};

static constexpr std::size_t k_inner_node_size{ 32 };
static constexpr std::size_t k_outer_node_size{ 48 };

struct inner_node_t
{
	float node_min[3];
	std::uint32_t packed0;
	float node_max[3];
	std::uint32_t packed1;

	[[nodiscard]] std::uint32_t type() const { return packed0 >> 30; }
	[[nodiscard]] std::uint32_t payload() const { return packed0 & 0x3FFFFFFFu; }
};

struct hedge_t
{
	std::uint8_t next;
	std::uint8_t twin;
	std::uint8_t vert;
	std::uint8_t face;
};

struct quat_t { float x, y, z, w; };
struct mat3_t { float m[3][3]; };

static mat3_t quat_to_matrix(const quat_t& q)
{
	const auto xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
	const auto xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
	const auto wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

	mat3_t m{};
	m.m[0][0] = 1 - 2 * (yy + zz);
	m.m[0][1] = 2 * (xy + wz);
	m.m[0][2] = 2 * (xz - wy);
	m.m[1][0] = 2 * (xy - wz);
	m.m[1][1] = 1 - 2 * (xx + zz);
	m.m[1][2] = 2 * (yz + wx);
	m.m[2][0] = 2 * (xz + wy);
	m.m[2][1] = 2 * (yz - wx);
	m.m[2][2] = 1 - 2 * (xx + yy);
	return m;
}

static Vec3 rotate_point(const mat3_t& m, const Vec3& v)
{
	return
	{
		m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z,
		m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z,
		m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z
	};
}

static Vec3 transform_point(const mat3_t& rot, const float scale[3], const float pos[3], const Vec3& local)
{
	const auto scaled = Vec3{ local.x * scale[0], local.y * scale[1], local.z * scale[2] };
	const auto rotated = rotate_point(rot, scaled);
	return { rotated.x + pos[0], rotated.y + pos[1], rotated.z + pos[2] };
}

static bool extract_mesh(std::uintptr_t bvh_ptr, std::uintptr_t vert_ptr, std::uintptr_t tri_ptr, std::uint32_t node_count, const mat3_t& rot, const float scale[3], const float pos[3], std::uintptr_t mat_arr_ptr, std::int32_t mat_count, const std::vector<bvh::global_surface_entry>& global_table, const bvh::surface_info& default_surface, std::vector<bvh::triangle>& out)
{
	if (!bvh_ptr || !vert_ptr || !tri_ptr || node_count == 0 || node_count > 0x1000000)
	{
		return false;
	}

	std::vector<std::uint8_t> bvh_buf(static_cast<std::size_t>(node_count) * k_inner_node_size);
	bvh_mem::read_raw(bvh_ptr, bvh_buf.data(), bvh_buf.size());

	std::uint32_t min_tri = UINT32_MAX, max_tri = 0;
	std::vector<std::pair<std::uint32_t, std::uint32_t>> ranges;
	std::vector<std::uint32_t> stack;
	stack.reserve(256);

	std::uint32_t cursor{ 0 };

	while (true)
	{
		if (cursor >= node_count)
		{
			if (stack.empty()) break;
			cursor = stack.back();
			stack.pop_back();
			continue;
		}

		const auto node = reinterpret_cast<const inner_node_t*>(bvh_buf.data() + static_cast<std::size_t>(cursor) * k_inner_node_size);
		const auto type = node->type();
		const auto payload = node->payload();

		if (type == 3)
		{
			if (payload > 0 && payload < 0x1000000)
			{
				ranges.push_back({ node->packed1, payload });
				if (node->packed1 < min_tri) min_tri = node->packed1;
				if (node->packed1 + payload > max_tri) max_tri = node->packed1 + payload;
			}
			if (stack.empty()) break;
			cursor = stack.back();
			stack.pop_back();
		}
		else
		{
			if (payload == 0)
			{
				if (stack.empty()) break;
				cursor = stack.back();
				stack.pop_back();
				continue;
			}
			if (cursor + payload < node_count) stack.push_back(cursor + payload);
			cursor++;
		}
	}

	if (ranges.empty() || max_tri <= min_tri) return false;

	const auto total_tris = max_tri - min_tri;
	if (total_tris > 0x1000000) return false;

	std::vector<std::int32_t> indices(total_tris * 3);
	bvh_mem::read_raw(tri_ptr + static_cast<std::uintptr_t>(min_tri) * 12, indices.data(), total_tris * 12);

	std::int32_t max_vert{ 0 };
	for (const auto idx : indices) {
		if (idx > max_vert) max_vert = idx;
	}

	if (max_vert <= 0 || max_vert > 0x1000000) return false;

	const auto vert_count = static_cast<std::uint32_t>(max_vert + 1);
	std::vector<float> vertices(vert_count * 3);
	bvh_mem::read_raw(vert_ptr, vertices.data(), static_cast<std::size_t>(vert_count) * 12);

	const bool has_materials = mat_arr_ptr > 0x10000 && mat_count > 0;
	std::vector<std::uint8_t> materials;

	if (has_materials)
	{
		materials.resize(total_tris);
		bvh_mem::read_raw(mat_arr_ptr + static_cast<std::uintptr_t>(min_tri), materials.data(), total_tris);
	}

	const auto global_count = static_cast<int>(global_table.size());
	const auto before = out.size();

	for (const auto& [start, count] : ranges)
	{
		for (std::uint32_t i = 0; i < count; ++i)
		{
			const auto local_idx = start - min_tri + i;
			if (local_idx >= total_tris) continue;

			auto surf = default_surface;

			if (has_materials && local_idx < materials.size())
			{
				const auto gi = materials[local_idx];
				if (gi < global_count)
				{
					const auto& gs = global_table[gi];
					surf.penetration = gs.penetration_mod;
					surf.surface_type = gs.surface_type;
					surf.global_index = gi;
				}
			}

			const auto base = local_idx * 3;
			const auto i0 = indices[base];
			const auto i1 = indices[static_cast<std::size_t>(base) + 1];
			const auto i2 = indices[static_cast<std::size_t>(base) + 2];

			if (i0 < 0 || i1 < 0 || i2 < 0) continue;
			if (static_cast<std::uint32_t>(i0) >= vert_count || static_cast<std::uint32_t>(i1) >= vert_count || static_cast<std::uint32_t>(i2) >= vert_count) continue;

			auto xf = [&](std::int32_t vi) -> Vec3 { return transform_point(rot, scale, pos, { vertices[vi * 3], vertices[vi * 3 + 1], vertices[vi * 3 + 2] }); };

			out.push_back({ .v0 = xf(i0), .v1 = xf(i1), .v2 = xf(i2), .surface = surf });
		}
	}

	return out.size() > before;
}

static bool extract_hull(std::uintptr_t hull_data, float uniform_scale, const bvh::surface_info& surface, std::vector<bvh::triangle>& out)
{
	if (!hull_data) return false;

	std::uint8_t hd[0x100]{};
	bvh_mem::read_raw(hull_data, hd, sizeof(hd));

	const auto vert_count = *reinterpret_cast<const std::int32_t*>(hd + 0x88);
	const auto vert_ptr = *reinterpret_cast<const std::uintptr_t*>(hd + 0x90);
	const auto hedge_count = *reinterpret_cast<const std::int32_t*>(hd + 0xa0);
	const auto hedge_ptr = *reinterpret_cast<const std::uintptr_t*>(hd + 0xa8);
	const auto face_count = *reinterpret_cast<const std::int32_t*>(hd + 0xb8);
	const auto face_ptr = *reinterpret_cast<const std::uintptr_t*>(hd + 0xc0);

	if (vert_count <= 0 || vert_count > 0xffff || hedge_count <= 0 || hedge_count > 0xffff || face_count <= 0 || face_count > 0xffff) return false;
	if (!vert_ptr || !hedge_ptr || !face_ptr) return false;

	std::vector<float> verts(vert_count * 3);
	bvh_mem::read_raw(vert_ptr, verts.data(), static_cast<std::size_t>(vert_count) * 12);

	std::vector<hedge_t> hedges(hedge_count);
	bvh_mem::read_raw(hedge_ptr, hedges.data(), static_cast<std::size_t>(hedge_count) * 4);

	std::vector<std::uint8_t> faces(face_count);
	bvh_mem::read_raw(face_ptr, faces.data(), face_count);

	const auto before = out.size();

	for (int fi = 0; fi < face_count; ++fi)
	{
		const auto start_he = faces[fi];
		if (start_he >= hedge_count) continue;

		std::vector<int> face_verts;
		face_verts.reserve(8);

		auto he = start_he;
		auto safety{ 0 };

		do
		{
			if (he >= hedge_count) break;
			face_verts.push_back(hedges[he].vert);
			he = hedges[he].next;
		} while (he != start_he && ++safety < 64);

		if (face_verts.size() < 3) continue;

		auto vert = [&](int vi) -> Vec3
			{
				if (vi < 0 || vi >= vert_count) return {};
				return { verts[vi * 3] * uniform_scale, verts[vi * 3 + 1] * uniform_scale, verts[vi * 3 + 2] * uniform_scale };
			};

		const auto v0 = vert(face_verts[0]);
		for (std::size_t i = 1; i + 1 < face_verts.size(); ++i)
		{
			out.push_back({ .v0 = v0, .v1 = vert(face_verts[i]), .v2 = vert(face_verts[i + 1]), .surface = surface });
		}
	}

	return out.size() > before;
}

static void process_shape(std::uintptr_t shape_body, std::uintptr_t hull_vtable, std::uintptr_t mesh_vtable, const std::vector<bvh::global_surface_entry>& global_table, std::vector<bvh::triangle>& out)
{
	const auto vtable = bvh_mem::read<std::uintptr_t>(shape_body);

	if (vtable == hull_vtable)
	{
		const auto hull_data = bvh_mem::read<std::uintptr_t>(shape_body + 0xb8);
		if (hull_data > 0x10000 && hull_data < 0x7fffffffffff)
		{
			const auto scale = bvh_mem::read<float>(shape_body + 0xb0);
			bvh::surface_info hull_surface{};
			hull_surface.penetration = bvh_mem::read<float>(shape_body + 0x28);
			extract_hull(hull_data, (scale > 0.0f && std::isfinite(scale)) ? scale : 1.0f, hull_surface, out);
		}
		return;
	}

	if (vtable != mesh_vtable) return;

	const auto mesh_data = bvh_mem::read<std::uintptr_t>(shape_body + 0xc0);
	if (!mesh_data) return;

	bvh::surface_info default_surface{};
	default_surface.penetration = bvh_mem::read<float>(shape_body + 0x28);

	const auto default_damage = bvh_mem::read<float>(shape_body + 0x2c);
	if (default_damage < 0.0f) return;

	std::uint8_t md[0xA0]{};
	bvh_mem::read_raw(mesh_data, md, sizeof(md));

	const auto mat_count = *reinterpret_cast<const std::int32_t*>(md + 0x90);
	const auto mat_arr_ptr = *reinterpret_cast<const std::uintptr_t*>(md + 0x98);
	const bool has_materials = mat_arr_ptr > 0x10000 && mat_count > 0;

	float scale[3]{};
	bvh_mem::read_raw(shape_body + 0xB0, scale, 12);

	if (scale[0] == 0.0f && scale[1] == 0.0f && scale[2] == 0.0f) return;
	if (!std::isfinite(scale[0]) || !std::isfinite(scale[1]) || !std::isfinite(scale[2])) return;

	float world_pos[3]{};
	bvh_mem::read_raw(shape_body + 0x100, world_pos, 12);

	quat_t quat{};
	bvh_mem::read_raw(shape_body + 0x130, &quat, sizeof(quat));

	const auto ql = quat.x * quat.x + quat.y * quat.y + quat.z * quat.z + quat.w * quat.w;
	if (ql < 0.5f || ql > 1.5f) quat = { 0, 0, 0, 1 };

	const auto rot = quat_to_matrix(quat);
	const auto bvh_ptr = *reinterpret_cast<const std::uintptr_t*>(md + 0x20);
	const auto vert_ptr = *reinterpret_cast<const std::uintptr_t*>(md + 0x38);
	const auto tri_ptr = *reinterpret_cast<const std::uintptr_t*>(md + 0x50);

	auto node_count{ 0u };
	for (auto c : { *reinterpret_cast<const std::int32_t*>(md + 0x28), *reinterpret_cast<const std::int32_t*>(md + 0x30), *reinterpret_cast<const std::int32_t*>(md + 0x48), *reinterpret_cast<const std::int32_t*>(md + 0x58) })
	{
		if (c > 0 && c < 0x1000000)
		{
			node_count = static_cast<std::uint32_t>(c);
			break;
		}
	}

	if (node_count > 0)
	{
		extract_mesh(bvh_ptr, vert_ptr, tri_ptr, node_count, rot, scale, world_pos, mat_arr_ptr, mat_count, global_table, default_surface, out);
	}
}

inline void bvh::parse()
{
	uintptr_t client_base = reinterpret_cast<uintptr_t>(GetModuleHandleA("client.dll"));
	uintptr_t vphysics_base = reinterpret_cast<uintptr_t>(GetModuleHandleA("vphysics2.dll"));
	if (!client_base || !vphysics_base) return;

	const auto trace_against_entities_call = bvh_mem::find_pattern(client_base, "E8 ? ? ? ? C7 87 ? ? ? ? ? ? ? ? 48 8D 54 24 ? 48 8B CF");
	if (!trace_against_entities_call) return;

	std::uintptr_t vphys2_world_global = 0;
	for (int i = 5; i < 64; ++i) {
		uint8_t buf[3];
		bvh_mem::read_raw(trace_against_entities_call - i, buf, 3);
		if (buf[0] == 0x48 && buf[1] == 0x8B && buf[2] == 0x0D) {
			vphys2_world_global = bvh_mem::resolve_rip(trace_against_entities_call - i);
			break;
		}
	}

	if (!vphys2_world_global) return;
	const auto vphys2_world = bvh_mem::read<std::uintptr_t>(vphys2_world_global);
	if (!vphys2_world) return;

	const auto get_surface_data_from_handle_fn = bvh_mem::find_pattern(client_base, "48 63 41 ? 48 8B 0D");
	if (!get_surface_data_from_handle_fn) return;

	const auto surface_manager = bvh_mem::read<std::uintptr_t>(bvh_mem::resolve_rip(get_surface_data_from_handle_fn + 4));
	if (!surface_manager) return;

	std::vector<global_surface_entry> global_table;
	{
		const auto array_base = bvh_mem::read<std::uintptr_t>(surface_manager + 40);
		if (array_base)
		{
			std::int32_t surface_count{ 0 };
			for (const auto off : { 32, 36, 24, 28, 48 })
			{
				const auto candidate = bvh_mem::read<std::int32_t>(surface_manager + off);
				if (candidate > 0)
				{
					surface_count = candidate;
					break;
				}
			}

			if (surface_count <= 0)
			{
				for (int i = 0; i < 1024; ++i)
				{
					global_surface_entry sd{};
					bvh_mem::read_raw(array_base + static_cast<std::size_t>(i) * 32, &sd, sizeof(sd));
					if (sd.penetration_mod == 0.0f && sd.surface_type == 0 && sd.unk_00 == 0.0f)
					{
						if (surface_count > 0 && i - surface_count > 8) break;
						continue;
					}
					surface_count = i + 1;
				}
			}

			if (surface_count)
			{
				global_table.resize(surface_count);
				bvh_mem::read_raw(array_base, global_table.data(), static_cast<std::size_t>(surface_count) * sizeof(global_surface_entry));
			}
		}
	}

	const auto inner_world = bvh_mem::read<std::uintptr_t>(vphys2_world + 0x30);
	if (!inner_world) return;

	const auto body_array = bvh_mem::read<std::uintptr_t>(inner_world + 0x110);
	if (!body_array) return;

	const auto body_count = bvh_mem::read<std::int32_t>(body_array + 0x268);
	if (!body_count) return;

	const auto hull_vtable = bvh_mem::find_vtable(vphysics_base, "CRnHullShape");
	const auto mesh_vtable = bvh_mem::find_vtable(vphysics_base, "CRnMeshShape");
	if (!hull_vtable || !mesh_vtable) return;

	std::vector<triangle> fresh;
	fresh.reserve(262144);

	for (std::int32_t body_idx = 0; body_idx < body_count; ++body_idx)
	{
		const auto body = body_array + static_cast<std::uintptr_t>(body_idx) * 88;
		const auto bvh_root = bvh_mem::read<std::int32_t>(body);
		const auto bvh_nodes_ptr = bvh_mem::read<std::uintptr_t>(body + 0x18);
		if (!bvh_nodes_ptr) continue;

		const auto femboys = bvh_mem::read<std::uint32_t>(body + 0x40);
		if (femboys != 2) continue;

		if (bvh_root >= 0)
		{
			const auto count_a = static_cast<std::uint32_t>(bvh_root + 1);
			const auto count_b = static_cast<std::uint32_t>(bvh_mem::read<std::int32_t>(body + 0x08));
			const auto count_c = static_cast<std::uint32_t>(bvh_mem::read<std::int32_t>(body + 0x10));

			std::uint32_t outer_node_count = count_a;
			if (count_b > outer_node_count) outer_node_count = count_b;
			if (count_c > outer_node_count) outer_node_count = count_c;

			if (outer_node_count > 0x100000) continue;

			std::vector<std::uint8_t> outer_buf(outer_node_count * k_outer_node_size);
			bvh_mem::read_raw(bvh_nodes_ptr, outer_buf.data(), outer_buf.size());

			std::vector<std::uintptr_t> leaves;
			leaves.reserve(256);

			std::vector<std::int32_t> outer_stack;
			outer_stack.reserve(128);
			outer_stack.push_back(bvh_root);

			while (!outer_stack.empty())
			{
				const auto idx = outer_stack.back();
				outer_stack.pop_back();
				if (idx < 0 || static_cast<std::uint32_t>(idx) >= outer_node_count) continue;

				const auto node = outer_buf.data() + static_cast<std::uintptr_t>(idx) * k_outer_node_size;
				const auto left = *reinterpret_cast<const std::int32_t*>(node + 12);

				if (left == -1)
				{
					const auto shape_ptr = *reinterpret_cast<const std::uintptr_t*>(node + 0x28);
					if (shape_ptr) leaves.push_back(shape_ptr);
				}
				else
				{
					const auto right = *reinterpret_cast<const std::int32_t*>(node + 28);
					if (left >= 0) outer_stack.push_back(left);
					if (right >= 0) outer_stack.push_back(right);
				}
			}

			std::unordered_set<std::uintptr_t> seen;
			for (const auto shape : leaves)
			{
				if (seen.count(shape)) continue;
				seen.insert(shape);
				process_shape(shape, hull_vtable, mesh_vtable, global_table, fresh);
			}
		}
		else
		{
			const auto shape = bvh_mem::read<std::uintptr_t>(body + 0x28);
			if (shape) process_shape(shape, hull_vtable, mesh_vtable, global_table, fresh);
		}
	}

	{
		std::unique_lock lock(this->m_mutex);
		this->m_triangles = std::move(fresh);
	}

	this->rebuild_accel();
}

inline void bvh::clear()
{
	std::unique_lock lock(this->m_mutex);
	this->m_triangles.clear();
	this->m_nodes.clear();
	this->m_indices.clear();
	this->m_tri_bounds.clear();
	this->m_centroids.clear();
}

inline bvh::trace_result bvh::trace_ray(const Vec3& start, const Vec3& end, std::int32_t exclude_tri) const
{
	trace_result result{};
	result.end_pos = end;

	if (this->m_nodes.empty()) return result;

	const auto dx = end.x - start.x;
	const auto dy = end.y - start.y;
	const auto dz = end.z - start.z;
	const auto max_dist = std::sqrt(dx * dx + dy * dy + dz * dz);
	if (max_dist < 1e-8f) return result;

	const auto inv_dist = 1.0f / max_dist;
	const float dir[3]{ dx * inv_dist, dy * inv_dist, dz * inv_dist };
	const float origin[3]{ start.x, start.y, start.z };
	const float inv_dir[3]{ std::abs(dir[0]) > 1e-8f ? 1.0f / dir[0] : (dir[0] >= 0 ? 1e12f : -1e12f), std::abs(dir[1]) > 1e-8f ? 1.0f / dir[1] : (dir[1] >= 0 ? 1e12f : -1e12f), std::abs(dir[2]) > 1e-8f ? 1.0f / dir[2] : (dir[2] >= 0 ? 1e12f : -1e12f) };

	auto closest_t = max_dist;
	std::int32_t stack[128]{};
	std::int32_t sp{ 0 };
	stack[0] = 0;

	while (sp >= 0)
	{
		const auto& node = this->m_nodes[stack[sp--]];
		if (!node.bounds.intersects_ray(origin, inv_dir, closest_t)) continue;

		if (node.left == -1)
		{
			for (std::int32_t i = node.tri_start; i < node.tri_start + node.tri_count; ++i)
			{
				const auto ti = this->m_indices[i];
				if (ti == exclude_tri) continue;

				const auto& tri = this->m_triangles[ti];

				const auto e1x = tri.v1.x - tri.v0.x, e1y = tri.v1.y - tri.v0.y, e1z = tri.v1.z - tri.v0.z;
				const auto e2x = tri.v2.x - tri.v0.x, e2y = tri.v2.y - tri.v0.y, e2z = tri.v2.z - tri.v0.z;

				const auto hx = dir[1] * e2z - dir[2] * e2y;
				const auto hy = dir[2] * e2x - dir[0] * e2z;
				const auto hz = dir[0] * e2y - dir[1] * e2x;
				const auto a = e1x * hx + e1y * hy + e1z * hz;

				if (a > -1e-8f && a < 1e-8f) continue;

				const auto f = 1.0f / a;
				const auto sx = origin[0] - tri.v0.x, sy = origin[1] - tri.v0.y, sz = origin[2] - tri.v0.z;
				const auto u = f * (sx * hx + sy * hy + sz * hz);
				if (u < 0.0f || u > 1.0f) continue;

				const auto qx = sy * e1z - sz * e1y, qy = sz * e1x - sx * e1z, qz = sx * e1y - sy * e1x;
				const auto v = f * (dir[0] * qx + dir[1] * qy + dir[2] * qz);
				if (v < 0.0f || u + v > 1.0f) continue;

				const auto t = f * (e2x * qx + e2y * qy + e2z * qz);

				if (t > 1e-5f && t < closest_t)
				{
					closest_t = t;
					result.hit = true;
					result.fraction = t / max_dist;
					result.distance = t;
					result.triangle_index = ti;
					result.surface = tri.surface;
					result.end_pos = { origin[0] + dir[0] * t, origin[1] + dir[1] * t, origin[2] + dir[2] * t };

					const auto nx = e1y * e2z - e1z * e2y;
					const auto ny = e1z * e2x - e1x * e2z;
					const auto nz = e1x * e2y - e1y * e2x;
					const auto nl = std::sqrt(nx * nx + ny * ny + nz * nz);
					if (nl > 1e-8f)
					{
						const auto inv_nl = 1.0f / nl;
						result.normal = { nx * inv_nl, ny * inv_nl, nz * inv_nl };
					}
				}
			}
		}
		else if (sp + 2 < 127)
		{
			stack[++sp] = node.right;
			stack[++sp] = node.left;
		}
	}

	return result;
}

inline std::vector<bvh::hit_entry> bvh::trace_ray_all(const Vec3& start, const Vec3& end) const
{
	std::vector<hit_entry> hits;
	if (this->m_nodes.empty()) return hits;

	const auto dx = end.x - start.x;
	const auto dy = end.y - start.y;
	const auto dz = end.z - start.z;
	const auto max_dist = std::sqrt(dx * dx + dy * dy + dz * dz);
	if (max_dist < 1e-8f) return hits;

	const auto inv_dist = 1.0f / max_dist;
	const float dir[3]{ dx * inv_dist, dy * inv_dist, dz * inv_dist };
	const float origin[3]{ start.x, start.y, start.z };
	const float inv_dir[3]{ std::abs(dir[0]) > 1e-8f ? 1.0f / dir[0] : (dir[0] >= 0 ? 1e12f : -1e12f), std::abs(dir[1]) > 1e-8f ? 1.0f / dir[1] : (dir[1] >= 0 ? 1e12f : -1e12f), std::abs(dir[2]) > 1e-8f ? 1.0f / dir[2] : (dir[2] >= 0 ? 1e12f : -1e12f) };

	std::int32_t stack[128]{};
	std::int32_t sp{ 0 };
	stack[0] = 0;

	while (sp >= 0)
	{
		const auto& node = this->m_nodes[stack[sp--]];
		if (!node.bounds.intersects_ray(origin, inv_dir, max_dist)) continue;

		if (node.left == -1)
		{
			for (std::int32_t i = node.tri_start; i < node.tri_start + node.tri_count; ++i)
			{
				const auto ti = this->m_indices[i];
				const auto& tri = this->m_triangles[ti];

				const auto e1x = tri.v1.x - tri.v0.x, e1y = tri.v1.y - tri.v0.y, e1z = tri.v1.z - tri.v0.z;
				const auto e2x = tri.v2.x - tri.v0.x, e2y = tri.v2.y - tri.v0.y, e2z = tri.v2.z - tri.v0.z;

				const auto hx = dir[1] * e2z - dir[2] * e2y;
				const auto hy = dir[2] * e2x - dir[0] * e2z;
				const auto hz = dir[0] * e2y - dir[1] * e2x;
				const auto a = e1x * hx + e1y * hy + e1z * hz;
				if (a > -1e-8f && a < 1e-8f) continue;

				const auto f = 1.0f / a;
				const auto sx = origin[0] - tri.v0.x, sy = origin[1] - tri.v0.y, sz = origin[2] - tri.v0.z;
				const auto u = f * (sx * hx + sy * hy + sz * hz);
				if (u < 0.0f || u > 1.0f) continue;

				const auto qx = sy * e1z - sz * e1y, qy = sz * e1x - sx * e1z, qz = sx * e1y - sy * e1x;
				const auto v = f * (dir[0] * qx + dir[1] * qy + dir[2] * qz);
				if (v < 0.0f || u + v > 1.0f) continue;

				const auto t = f * (e2x * qx + e2y * qy + e2z * qz);
				if (t > 1e-5f && t < max_dist)
				{
					auto nx = e1y * e2z - e1z * e2y;
					auto ny = e1z * e2x - e1x * e2z;
					auto nz = e1x * e2y - e1y * e2x;
					const auto nl = std::sqrt(nx * nx + ny * ny + nz * nz);
					if (nl > 1e-8f)
					{
						const auto inv_nl = 1.0f / nl;
						nx *= inv_nl; ny *= inv_nl; nz *= inv_nl;
					}
					const auto ndot = nx * dir[0] + ny * dir[1] + nz * dir[2];

					hit_entry hit{};
					hit.distance = t;
					hit.fraction = t / max_dist;
					hit.position = { origin[0] + dir[0] * t, origin[1] + dir[1] * t, origin[2] + dir[2] * t };
					hit.normal = { nx, ny, nz };
					hit.surface = tri.surface;
					hit.triangle_index = ti;
					hit.is_enter = (ndot < 0.0f);
					hits.push_back(hit);
				}
			}
		}
		else if (sp + 2 < 127)
		{
			stack[++sp] = node.right;
			stack[++sp] = node.left;
		}
	}

	std::sort(hits.begin(), hits.end(), [](const hit_entry& a, const hit_entry& b) { return a.distance < b.distance; });
	return hits;
}

inline std::vector<bvh::penetration_segment> bvh::build_segments(const std::vector<hit_entry>& hits, float ray_length) const
{
	std::vector<penetration_segment> segments;
	if (hits.empty()) return segments;

	auto sorted = hits;
	for (std::size_t i = 1; i < sorted.size(); ++i)
	{
		auto& prev = sorted[i - 1];
		auto& curr = sorted[i];
		if (!curr.is_enter && prev.is_enter && (curr.fraction - prev.fraction) * ray_length <= (1.0f / 512.0f))
		{
			std::swap(prev, curr);
		}
	}

	auto was_exit{ true };
	auto seg_enter_idx{ -1 };

	for (std::size_t i = 0; i < sorted.size(); ++i)
	{
		const auto& hit = sorted[i];
		const bool is_exit = !hit.is_enter;

		if (is_exit != was_exit)
		{
			was_exit = is_exit;
			if (!is_exit)
			{
				if (seg_enter_idx >= 0 && i > 0)
				{
					const auto& exit_hit = sorted[i - 1];
					penetration_segment seg{};
					seg.enter_fraction = sorted[seg_enter_idx].fraction;
					seg.exit_fraction = exit_hit.fraction;
					seg.enter_distance = sorted[seg_enter_idx].distance;
					seg.exit_distance = exit_hit.distance;
					seg.enter_pos = sorted[seg_enter_idx].position;
					seg.exit_pos = exit_hit.position;
					seg.enter_surface = sorted[seg_enter_idx].surface;
					seg.exit_surface = exit_hit.surface;
					seg.thickness = exit_hit.distance - sorted[seg_enter_idx].distance;
					seg.min_pen_mod = sorted[seg_enter_idx].surface.penetration;

					if (seg.thickness > 0.0f) segments.push_back(seg);
				}
				seg_enter_idx = static_cast<int>(i);
			}
		}
	}

	if (seg_enter_idx >= 0)
	{
		const auto& enter_hit = sorted[seg_enter_idx];
		const auto& last_hit = sorted.back();
		penetration_segment seg{};
		seg.enter_fraction = enter_hit.fraction;
		seg.exit_fraction = last_hit.fraction;
		seg.enter_distance = enter_hit.distance;
		seg.exit_distance = last_hit.distance;
		seg.enter_pos = enter_hit.position;
		seg.exit_pos = last_hit.position;
		seg.enter_surface = enter_hit.surface;
		seg.exit_surface = last_hit.surface;
		seg.thickness = last_hit.distance - enter_hit.distance;
		if (seg.thickness < 1.0f) seg.thickness = 1.0f;
		seg.min_pen_mod = enter_hit.surface.penetration;
		segments.push_back(seg);
	}

	return segments;
}

inline const std::vector<bvh::triangle>& bvh::triangles() const { return this->m_triangles; }
inline std::size_t bvh::count() const { std::shared_lock lock(this->m_mutex); return this->m_triangles.size(); }
inline bool bvh::valid() const { std::shared_lock lock(this->m_mutex); return !this->m_triangles.empty(); }

inline void bvh::aabb::expand(const Vec3& p)
{
	if (p.x < mins[0]) mins[0] = p.x;
	if (p.y < mins[1]) mins[1] = p.y;
	if (p.z < mins[2]) mins[2] = p.z;
	if (p.x > maxs[0]) maxs[0] = p.x;
	if (p.y > maxs[1]) maxs[1] = p.y;
	if (p.z > maxs[2]) maxs[2] = p.z;
}

inline void bvh::aabb::expand(const aabb& o)
{
	for (int i = 0; i < 3; ++i) {
		if (o.mins[i] < mins[i]) mins[i] = o.mins[i];
		if (o.maxs[i] > maxs[i]) maxs[i] = o.maxs[i];
	}
}

inline int bvh::aabb::longest_axis() const
{
	const auto ex = this->maxs[0] - this->mins[0];
	const auto ey = this->maxs[1] - this->mins[1];
	const auto ez = this->maxs[2] - this->mins[2];
	if (ex >= ey && ex >= ez) return 0;
	if (ey >= ez) return 1;
	return 2;
}

inline bool bvh::aabb::intersects_ray(const float origin[3], const float inv_dir[3], float max_t) const
{
	auto tmin{ 0.0f };
	auto tmax = max_t;
	for (int i = 0; i < 3; ++i) {
		auto t0 = (this->mins[i] - origin[i]) * inv_dir[i];
		auto t1 = (this->maxs[i] - origin[i]) * inv_dir[i];
		if (inv_dir[i] < 0.0f) {
			const auto tmp = t0; t0 = t1; t1 = tmp;
		}
		if (t0 > tmin) tmin = t0;
		if (t1 < tmax) tmax = t1;
		if (tmax < tmin) return false;
	}
	return true;
}

inline void bvh::rebuild_accel()
{
	this->m_nodes.clear();
	this->m_indices.clear();
	this->m_tri_bounds.clear();
	this->m_centroids.clear();

	const auto tri_count = static_cast<std::int32_t>(this->m_triangles.size());
	if (tri_count == 0) return;

	this->m_indices.resize(tri_count);
	this->m_tri_bounds.resize(tri_count);
	this->m_centroids.resize(static_cast<std::size_t>(tri_count) * 3);

	for (std::int32_t i = 0; i < tri_count; ++i)
	{
		this->m_indices[i] = i;
		aabb bb{};
		bb.expand(this->m_triangles[i].v0);
		bb.expand(this->m_triangles[i].v1);
		bb.expand(this->m_triangles[i].v2);
		this->m_tri_bounds[i] = bb;

		const auto ci = static_cast<std::size_t>(i) * 3;
		this->m_centroids[ci] = (bb.mins[0] + bb.maxs[0]) * 0.5f;
		this->m_centroids[ci + 1] = (bb.mins[1] + bb.maxs[1]) * 0.5f;
		this->m_centroids[ci + 2] = (bb.mins[2] + bb.maxs[2]) * 0.5f;
	}

	this->m_nodes.reserve(static_cast<std::size_t>(tri_count) * 2);
	this->build_recursive(0, tri_count, 0);
}

inline std::int32_t bvh::build_recursive(std::int32_t start, std::int32_t end, std::int32_t depth)
{
	const auto node_idx = static_cast<std::int32_t>(this->m_nodes.size());
	this->m_nodes.push_back({});

	const auto count = end - start;

	for (std::int32_t i = start; i < end; ++i) {
		this->m_nodes[node_idx].bounds.expand(this->m_tri_bounds[this->m_indices[i]]);
	}

	if (count <= k_max_leaf_tris || depth >= k_max_depth) {
		this->m_nodes[node_idx].tri_start = start;
		this->m_nodes[node_idx].tri_count = count;
		return node_idx;
	}

	aabb centroid_bounds{};
	for (std::int32_t i = start; i < end; ++i) {
		const auto ci = static_cast<std::size_t>(this->m_indices[i]) * 3;
		centroid_bounds.expand(Vec3{ this->m_centroids[ci], this->m_centroids[ci + 1], this->m_centroids[ci + 2] });
	}

	const auto axis = centroid_bounds.longest_axis();
	const auto mid = (centroid_bounds.mins[axis] + centroid_bounds.maxs[axis]) * 0.5f;

	auto partition_point = std::partition(this->m_indices.begin() + start, this->m_indices.begin() + end, [this, axis, mid](std::int32_t idx) {
		return this->m_centroids[static_cast<std::size_t>(idx) * 3 + axis] < mid;
		});
	auto split = static_cast<std::int32_t>(partition_point - this->m_indices.begin());

	if (split == start || split == end) split = start + count / 2;

	auto left_child = this->build_recursive(start, split, depth + 1);
	auto right_child = this->build_recursive(split, end, depth + 1);

	this->m_nodes[node_idx].left = left_child;
	this->m_nodes[node_idx].right = right_child;

	return node_idx;
}

inline bvh g_bvh;