import json
import urllib.request
import os

# Official CS2 Dumper raw JSON links
OFFSETS_URL = "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/offsets.json"
CLIENT_URL = "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.json"

def fetch_json(url):
    req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
    with urllib.request.urlopen(req) as response:
        return json.loads(response.read().decode('utf-8'))

def hex_str(val):
    return f"0x{val:X}" if isinstance(val, int) else str(val)

def get_field(client_data, class_name, field_name):
    try:
        # cs2-dumper format: client.dll -> classes -> ClassName -> fields -> FieldName
        if "client.dll" in client_data and "classes" in client_data["client.dll"]:
            return client_data["client.dll"]["classes"][class_name]["fields"][field_name]
        # Alternative format
        return client_data[class_name]["fields"][field_name]
    except KeyError:
        return 0

def main():
    print("[*] Downloading the latest offsets...")
    offsets_data = fetch_json(OFFSETS_URL).get("client.dll", {})
    client_data = fetch_json(CLIENT_URL)

    # Your offsets.h file template
    header_content = f"""#pragma once
#include <Windows.h>
#include <cstdint>

struct Offsets {{
    // client.dll Offsets
    struct {{
        uintptr_t dwEntityList               = {hex_str(offsets_data.get("dwEntityList", 0))}; 
        uintptr_t dwViewMatrix               = {hex_str(offsets_data.get("dwViewMatrix", 0))};
        uintptr_t dwViewRender               = {hex_str(offsets_data.get("dwViewRender", 0))};
        uintptr_t dwLocalPlayerPawn          = {hex_str(offsets_data.get("dwLocalPlayerPawn", 0))};
        uintptr_t dwLocalPlayerController    = {hex_str(offsets_data.get("dwLocalPlayerController", 0))};
        uintptr_t dwGlobalVars               = {hex_str(offsets_data.get("dwGlobalVars", 0))};
        uintptr_t dwPlantedC4                = {hex_str(offsets_data.get("dwPlantedC4", 0))};
        uintptr_t dwWeaponC4                 = {hex_str(offsets_data.get("dwWeaponC4", 0))};
    }} client;                                
                                             
    // Entity & Pawn                         
    struct {{                                 
        uintptr_t m_iTeamNum                 = {hex_str(get_field(client_data, "C_BaseEntity", "m_iTeamNum"))}; 
        uintptr_t m_pGameSceneNode           = {hex_str(get_field(client_data, "C_BaseEntity", "m_pGameSceneNode"))}; 
        uintptr_t m_iHealth                  = {hex_str(get_field(client_data, "C_BaseEntity", "m_iHealth"))}; 
    }} C_BaseEntity;                          
                                             
    struct {{                                 
        uintptr_t m_vecAbsOrigin             = {hex_str(get_field(client_data, "CGameSceneNode", "m_vecAbsOrigin"))}; 
    }} CGameSceneNode;                        
                                             
    struct {{                                 
        uintptr_t m_modelState               = {hex_str(get_field(client_data, "CSkeletonInstance", "m_modelState"))}; 
    }} CSkeletonInstance;               
                                       
    // Controllers                     
    struct {{                           
        uintptr_t m_hPlayerPawn              = {hex_str(get_field(client_data, "CCSPlayerController", "m_hPlayerPawn"))}; 
        uintptr_t m_hPawn                    = {hex_str(get_field(client_data, "CCSPlayerController", "m_hPawn"))}; 
        uintptr_t m_sSanitizedPlayerName     = {hex_str(get_field(client_data, "CCSPlayerController", "m_sSanitizedPlayerName"))}; 
        uintptr_t m_hObserverPawn            = {hex_str(get_field(client_data, "CCSPlayerController", "m_hObserverPawn"))}; 
    }} CCSPlayerController;

    struct {{
        uintptr_t m_pObserverServices        = {hex_str(get_field(client_data, "C_BasePlayerPawn", "m_pObserverServices"))}; 
    }} C_BasePlayerPawn;

    struct {{
        uintptr_t m_hObserverTarget          = {hex_str(get_field(client_data, "CPlayer_ObserverServices", "m_hObserverTarget"))}; 
    }} CPlayer_ObserverServices;

    struct {{
        uintptr_t m_bIsScoped                = {hex_str(get_field(client_data, "C_CSPlayerPawn", "m_bIsScoped"))}; 
        uintptr_t m_entitySpottedState       = {hex_str(get_field(client_data, "C_CSPlayerPawn", "m_entitySpottedState"))}; 
        uintptr_t m_iIDEntIndex              = {hex_str(get_field(client_data, "C_CSPlayerPawn", "m_iIDEntIndex"))}; 
    }} C_CSPlayerPawn;

    struct {{
        uintptr_t m_vecViewOffset            = {hex_str(get_field(client_data, "C_BaseModelEntity", "m_vecViewOffset"))}; 
    }} C_BaseModelEntity;

    // Weapon Services
    struct {{
        uintptr_t m_pWeaponServices          = {hex_str(get_field(client_data, "C_CSPlayerPawnBase", "m_pWeaponServices"))}; 
        uintptr_t m_flFlashDuration          = {hex_str(get_field(client_data, "C_CSPlayerPawnBase", "m_flFlashDuration"))}; 
    }} C_CSPlayerPawnBase;

    struct {{
        uintptr_t m_hActiveWeapon            = {hex_str(get_field(client_data, "CPlayer_WeaponServices", "m_hActiveWeapon"))}; 
        uintptr_t m_hMyWeapons               = {hex_str(get_field(client_data, "CPlayer_WeaponServices", "m_hMyWeapons"))}; 
    }} CPlayer_WeaponServices;

    struct {{
        uintptr_t m_AttributeManager         = {hex_str(get_field(client_data, "C_EconEntity", "m_AttributeManager"))}; 
    }} C_EconEntity;

    struct {{
        uintptr_t m_Item                     = {hex_str(get_field(client_data, "C_AttributeContainer", "m_Item"))}; 
    }} C_AttributeContainer;

    struct {{
        uintptr_t m_iItemDefinitionIndex     = {hex_str(get_field(client_data, "C_EconItemView", "m_iItemDefinitionIndex"))}; 
    }} C_EconItemView;

    // Spotted State
    struct {{
        uintptr_t m_bSpotted                 = {hex_str(get_field(client_data, "EntitySpottedState_t", "m_bSpotted"))}; 
        uintptr_t m_bSpottedByMask           = {hex_str(get_field(client_data, "EntitySpottedState_t", "m_bSpottedByMask"))}; 
    }} EntitySpottedState_t;

    struct {{
        uintptr_t m_pEntity                  = {hex_str(get_field(client_data, "CEntityInstance", "m_pEntity"))}; 
    }} CEntityInstance;

    struct {{
        uintptr_t m_designerName             = {hex_str(get_field(client_data, "CEntityIdentity", "m_designerName"))}; 
    }} CEntityIdentity;

    // Bomb / C4
    struct {{
        uintptr_t m_bBombTicking             = {hex_str(get_field(client_data, "C_PlantedC4", "m_bBombTicking"))}; 
        uintptr_t m_bBeingDefused            = {hex_str(get_field(client_data, "C_PlantedC4", "m_bBeingDefused"))}; 
        uintptr_t m_flDefuseCountDown        = {hex_str(get_field(client_data, "C_PlantedC4", "m_flDefuseCountDown"))}; 
        uintptr_t m_nBombSite                = {hex_str(get_field(client_data, "C_PlantedC4", "m_nBombSite"))}; 
        uintptr_t m_flC4Blow                 = {hex_str(get_field(client_data, "C_PlantedC4", "m_flC4Blow"))}; 
    }} C4;

    static uintptr_t get_client_base() {{
        static uintptr_t client_base = 0;
        if (!client_base) {{
            client_base = reinterpret_cast<uintptr_t>(GetModuleHandleA("client.dll"));
        }}
        return client_base;
    }}
}};

inline Offsets g_offsets;
"""

    output_path = os.path.join(os.path.dirname(__file__), "offsets.h")
    with open(output_path, "w") as f:
        f.write(header_content)

    print("[+] offsets.h has been successfully updated automatically!")

if __name__ == "__main__":
    main()
