#pragma once
#include <d3d11.h>
#include "../imgui/imgui.h"
#include "../Resources\Images.h"

// stb_image for PNG decode from memory
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "../imgui/stb_image.h"

struct MenuTexture {
    ID3D11ShaderResourceView* srv = nullptr;
    int w = 0, h = 0;
    bool loaded = false;
};

// Loaded once at startup, never unloaded until shutdown
inline MenuTexture g_logo;
inline MenuTexture g_btn_aimbot;
inline MenuTexture g_btn_aimbot_p;
inline MenuTexture g_btn_visual;
inline MenuTexture g_btn_visual_p;
inline MenuTexture g_btn_misc;
inline MenuTexture g_btn_misc_p;
inline MenuTexture g_btn_config;
inline MenuTexture g_btn_config_p;

static ID3D11Device* g_menu_tex_device = nullptr;

static bool load_png_texture(const unsigned char* png_data, int png_size,
                              MenuTexture& out, ID3D11Device* device)
{
    if (!device || !png_data || png_size <= 0) return false;

    int w = 0, h = 0, channels = 0;
    unsigned char* img = stbi_load_from_memory(png_data, png_size, &w, &h, &channels, 4);
    if (!img) return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA srd = {};
    srd.pSysMem = img;
    srd.SysMemPitch = w * 4;

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &srd, &tex);
    if (FAILED(hr) || !tex) {
        stbi_image_free(img);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(tex, &srvDesc, &out.srv);
    tex->Release();

    stbi_image_free(img);

    if (FAILED(hr) || !out.srv) return false;

    out.w = w;
    out.h = h;
    out.loaded = true;
    return true;
}

inline void menu_textures_init(ID3D11Device* device)
{
    g_menu_tex_device = device;
    if (!device) return;

    load_png_texture(Images::Logo, sizeof(Images::Logo), g_logo, device);
    load_png_texture(Images::AimbotButton, sizeof(Images::AimbotButton), g_btn_aimbot, device);
    load_png_texture(Images::AimbotButtonPressed, sizeof(Images::AimbotButtonPressed), g_btn_aimbot_p, device);
    load_png_texture(Images::VisualButton, sizeof(Images::VisualButton), g_btn_visual, device);
    load_png_texture(Images::VisualButtonPressed, sizeof(Images::VisualButtonPressed), g_btn_visual_p, device);
    load_png_texture(Images::MiscButton, sizeof(Images::MiscButton), g_btn_misc, device);
    load_png_texture(Images::MiscButtonPressed, sizeof(Images::MiscButtonPressed), g_btn_misc_p, device);
    load_png_texture(Images::ConfigButton, sizeof(Images::ConfigButton), g_btn_config, device);
    load_png_texture(Images::ConfigButtonPressed, sizeof(Images::ConfigButtonPressed), g_btn_config_p, device);

    printf("[+] Menu textures loaded: logo=%d aimbot=%d visual=%d misc=%d config=%d\n",
           g_logo.loaded, g_btn_aimbot.loaded, g_btn_visual.loaded,
           g_btn_misc.loaded, g_btn_config.loaded);
}

inline void menu_textures_shutdown()
{
    auto release = [](MenuTexture& t) {
        if (t.srv) { t.srv->Release(); t.srv = nullptr; }
        t.loaded = false;
    };
    release(g_logo);
    release(g_btn_aimbot); release(g_btn_aimbot_p);
    release(g_btn_visual); release(g_btn_visual_p);
    release(g_btn_misc);   release(g_btn_misc_p);
    release(g_btn_config); release(g_btn_config_p);
}
