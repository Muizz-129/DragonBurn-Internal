#pragma once
#include <d3d11.h>
#include <wincodec.h>
#include "../imgui/imgui.h"
#include <string>
#include <unordered_map>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <algorithm>
#include <memory>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "windowscodecs.lib")

struct WeaponIconMapping {
    uint16_t def_index;
    const char* filename;
};

static constexpr WeaponIconMapping ICON_FILENAME_MAP[] = {
    {1,   "deagle"},
    {2,   "elite"},
    {3,   "fiveseven"},
    {4,   "glock"},
    {7,   "ak47"},
    {8,   "aug"},
    {9,   "awp"},
    {10,  "famas"},
    {11,  "g3sg1"},
    {13,  "galilar"},
    {14,  "m249"},
    {16,  "m4a1"},
    {17,  "mac10"},
    {19,  "p90"},
    {23,  "mp5sd"},
    {24,  "ump45"},
    {25,  "xm1014"},
    {26,  "bizon"},
    {27,  "mag7"},
    {28,  "negev"},
    {29,  "sawedoff"},
    {30,  "tec9"},
    {31,  "taser"},
    {32,  "hkp2000"},
    {33,  "mp7"},
    {34,  "mp9"},
    {35,  "nova"},
    {36,  "p250"},
    {38,  "scar20"},
    {39,  "sg556"},
    {40,  "ssg08"},
    {42,  "knife"},
    {43,  "flashbang"},
    {44,  "hegrenade"},
    {45,  "smokegrenade"},
    {46,  "molotov"},
    {47,  "decoy"},
    {48,  "incgrenade"},
    {49,  "c4"},
    {59,  "knife_t"},
    {60,  "m4a1_silencer"},
    {61,  "usp_silencer"},
    {63,  "cz75a"},
    {64,  "revolver"},
    {500, "knife_bayonet"},
    {503, "knife_css"},
    {505, "knife_flip"},
    {506, "knife_gut"},
    {507, "knife_karambit"},
    {508, "knife_m9_bayonet"},
    {509, "knife_tactical"},
    {512, "knife_falchion"},
    {514, "knife_survival_bowie"},
    {515, "knife_butterfly"},
    {516, "knife_ursus"},
    {517, "knife_gypsy_jackknife"},
    {518, "knife_stiletto"},
    {519, "knife_widowmaker"},
    {520, "knife_skeleton"},
    {521, "knife_outdoor"},
    {522, "knife_canis"},
    {523, "knife_cord"},
    {525, "knife_classic"},
    {526, "knife_kukri"},
};
static constexpr int ICON_MAP_SIZE = sizeof(ICON_FILENAME_MAP) / sizeof(ICON_FILENAME_MAP[0]);

struct LoadedIcon {
    ID3D11ShaderResourceView* srv = nullptr;
    float aspect_ratio = 1.5f;
};

class WeaponIcons {
public:
    void init(ID3D11Device* device) {
        d3d_device = device;
        if (!device) return;

        CoInitialize(nullptr);

        char dll_path[MAX_PATH]{};
        HMODULE hm = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)this, &hm);
        GetModuleFileNameA(hm, dll_path, sizeof(dll_path));
        std::string base_dir(dll_path);
        size_t p = base_dir.find_last_of("\\/");
        std::filesystem::path icon_dir = (p != std::string::npos) ? std::filesystem::path(base_dir.substr(0, p + 1)) / "icons" : std::filesystem::path("icons");

        if (!std::filesystem::exists(icon_dir)) {
            if (std::filesystem::exists("icons")) {
                icon_dir = "icons";
            }
            else {
                return;
            }
        }

        std::unordered_map<std::string, std::vector<uint16_t>> name_to_indices;
        for (int i = 0; i < ICON_MAP_SIZE; i++) {
            std::string key = ICON_FILENAME_MAP[i].filename;
            name_to_indices[key].push_back(ICON_FILENAME_MAP[i].def_index);
        }

        for (auto& entry : std::filesystem::directory_iterator(icon_dir)) {
            if (!entry.is_regular_file()) continue;
            auto path = entry.path();
            auto ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext != ".png" && ext != ".bmp" && ext != ".jpg" &&
                ext != ".jpeg" && ext != ".tiff" && ext != ".gif" &&
                ext != ".ico") continue;

            auto stem = path.stem().string();
            std::string stem_lower = stem;
            std::transform(stem_lower.begin(), stem_lower.end(), stem_lower.begin(), ::tolower);

            auto it = name_to_indices.find(stem_lower);
            if (it == name_to_indices.end()) {
                if (stem_lower.substr(0, 7) == "weapon_")
                    it = name_to_indices.find(stem_lower.substr(7));
                if (it == name_to_indices.end() && stem_lower.substr(0, 5) == "knife")
                    it = name_to_indices.find("knife");
            }

            if (it == name_to_indices.end()) continue;

            // Muat terus guna wchar_t path untuk elak ralat decoding karakter Windows
            LoadedIcon icon = load_image_texture_w(path.c_str());
            if (icon.srv) {
                for (uint16_t def_idx : it->second) {
                    if (icons.find(def_idx) == icons.end()) {
                        if (&def_idx != &it->second[0])
                            icon.srv->AddRef();
                        icons[def_idx] = icon;
                    }
                }
            }
        }

        // Kongsi ikon pisau asas
        auto knife_it = icons.find(42);
        if (knife_it != icons.end()) {
            for (int i = 0; i < ICON_MAP_SIZE; i++) {
                uint16_t idx = ICON_FILENAME_MAP[i].def_index;
                std::string fn = ICON_FILENAME_MAP[i].filename;
                if (fn.substr(0, 5) == "knife" && icons.find(idx) == icons.end()) {
                    knife_it->second.srv->AddRef();
                    icons[idx] = knife_it->second;
                }
            }
        }
    }

    void shutdown() {
        for (auto& [idx, icon] : icons) {
            if (icon.srv) icon.srv->Release();
        }
        icons.clear();
    }

    ImTextureID get_icon(uint16_t def_index) {
        auto it = icons.find(def_index);
        if (it != icons.end())
            return (ImTextureID)it->second.srv;
        return nullptr;
    }

    float get_icon_aspect(uint16_t def_index) {
        auto it = icons.find(def_index);
        if (it != icons.end())
            return it->second.aspect_ratio;
        return 1.5f;
    }

    bool has_any_icons() const {
        return !icons.empty();
    }

private:
    ID3D11Device* d3d_device = nullptr;
    std::unordered_map<uint16_t, LoadedIcon> icons;

    LoadedIcon load_image_texture_w(const wchar_t* wpath) {
        LoadedIcon result = { nullptr, 1.5f };
        if (!d3d_device || !wpath) return result;

        IWICImagingFactory* wic_factory = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory, (void**)&wic_factory);
        if (FAILED(hr) || !wic_factory) return result;

        IWICBitmapDecoder* decoder = nullptr;
        hr = wic_factory->CreateDecoderFromFilename(
            wpath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);

        if (FAILED(hr) || !decoder) {
            wic_factory->Release();
            return result;
        }

        IWICBitmapFrameDecode* frame = nullptr;
        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr) || !frame) {
            decoder->Release();
            wic_factory->Release();
            return result;
        }

        IWICFormatConverter* converter = nullptr;
        hr = wic_factory->CreateFormatConverter(&converter);
        if (FAILED(hr) || !converter) {
            frame->Release();
            decoder->Release();
            wic_factory->Release();
            return result;
        }

        hr = converter->Initialize(
            frame, GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0f,
            WICBitmapPaletteTypeCustom);
        if (FAILED(hr)) {
            converter->Release();
            frame->Release();
            decoder->Release();
            wic_factory->Release();
            return result;
        }

        UINT width = 0, height = 0;
        converter->GetSize(&width, &height);
        if (width == 0 || height == 0) {
            converter->Release();
            frame->Release();
            decoder->Release();
            wic_factory->Release();
            return result;
        }

        result.aspect_ratio = (float)width / (float)height;

        UINT stride = width * 4;
        UINT buf_size = stride * height;
        auto pixels = std::make_unique<uint8_t[]>(buf_size);

        hr = converter->CopyPixels(nullptr, stride, buf_size, pixels.get());
        if (FAILED(hr)) {
            converter->Release();
            frame->Release();
            decoder->Release();
            wic_factory->Release();
            return result;
        }

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init_data = {};
        init_data.pSysMem = pixels.get();
        init_data.SysMemPitch = stride;

        ID3D11Texture2D* tex = nullptr;
        hr = d3d_device->CreateTexture2D(&desc, &init_data, &tex);

        if (SUCCEEDED(hr) && tex) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.Format = desc.Format;
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Texture2D.MipLevels = 1;

            d3d_device->CreateShaderResourceView(tex, &srv_desc, &result.srv);
            tex->Release();
        }

        converter->Release();
        frame->Release();
        decoder->Release();
        wic_factory->Release();

        return result;
    }
};

inline WeaponIcons g_weapon_icons;