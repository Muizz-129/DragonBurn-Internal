#pragma once
#include <Windows.h>
#include <cstdint>

struct Offsets {
    // client.dll Offsets
    struct {
        uintptr_t dwEntityList               = 0x2571230;
        uintptr_t dwViewMatrix               = 0x23CB830;
        uintptr_t dwViewRender               = 0x23CB898;
        uintptr_t dwLocalPlayerPawn          = 0x23C6268;
        uintptr_t dwLocalPlayerController    = 0x23A0F30;
        uintptr_t dwGlobalVars               = 0x20AF5F0;
        uintptr_t dwPlantedC4                = 0x2390A18;
        uintptr_t dwWeaponC4                 = 0x233EF10;
    } client;                                
                                             
    // Entity & Pawn                         
    struct {                                 
        uintptr_t m_iTeamNum                 = 0x3E7; // uint8
        uintptr_t m_pGameSceneNode           = 0x330; // CGameSceneNode*
        uintptr_t m_iHealth                  = 0x34C; // int32
        uintptr_t m_hOwnerEntity             = 0x520; // CHandle<C_BaseEntity>
    } C_BaseEntity;                          
                                             
    struct {                                 
        uintptr_t m_vecAbsOrigin             = 0xC8; // VectorWS
    } CGameSceneNode;                        
                                             
    struct {                                 
        uintptr_t m_modelState               = 0x140; // CModelState
    } CSkeletonInstance;               
                                       
    // Controllers                     
    struct {                           
        uintptr_t m_hPlayerPawn              = 0x914; // CHandle<C_CSPlayerPawn>
        uintptr_t m_hPawn                    = 0x600; // CHandle<C_CSPlayerPawnBase>
        uintptr_t m_sSanitizedPlayerName     = 0x868; // CUtlString
        uintptr_t m_hObserverPawn            = 0x918; // CHandle<C_CSObserverPawn>
        uintptr_t m_iObserverMode            = 0x48; // uint8
    } CCSPlayerController;

    struct {
        uintptr_t m_pObserverServices        = 0x1220; // CPlayer_ObserverServices*
    } C_BasePlayerPawn;

    struct {
        uintptr_t m_hObserverTarget          = 0x4C; // CHandle<C_BaseEntity>
    } CPlayer_ObserverServices;

    struct {
        uintptr_t m_bIsScoped                = 0x1C78; // bool
        uintptr_t m_entitySpottedState       = 0x1C60; // EntitySpottedState_t
        uintptr_t m_iIDEntIndex              = 0x342C; // CEntityIndex
    } C_CSPlayerPawn;

    struct {
        uintptr_t m_vecViewOffset            = 0xE78; // CNetworkViewOffsetVector
    } C_BaseModelEntity;

    // Weapon Services
    struct {
        uintptr_t m_pWeaponServices          = 0x1208; // CPlayer_WeaponServices*
        uintptr_t m_flFlashDuration          = 0x1428; // float32
    } C_CSPlayerPawnBase;

    struct {
        uintptr_t m_hActiveWeapon            = 0x60; // CHandle<C_BasePlayerWeapon>
        uintptr_t m_hMyWeapons               = 0x48; // C_NetworkUtlVectorBase<CHandle<C_BasePlayerWeapon>> 
    } CPlayer_WeaponServices;

    struct {
        uintptr_t m_AttributeManager         = 0x13D0; // C_AttributeContainer
    } C_EconEntity;

    struct {
        uintptr_t m_Item                     = 0x50; // C_EconItemView
    } C_AttributeContainer;

    struct {
        uintptr_t m_iItemDefinitionIndex     = 0x1BA; // uint16
    } C_EconItemView;

    // Spotted State
    struct {
        uintptr_t m_bSpotted                 = 0x8; // bool
        uintptr_t m_bSpottedByMask           = 0xC; // uint32[2]
    } EntitySpottedState_t;

    struct {
        uintptr_t m_pEntity                  = 0x10; // CEntityIdentity*
    } CEntityInstance;

    struct {
        uintptr_t m_designerName             = 0x20; // CUtlSymbolLarge
    } CEntityIdentity;

    // Bomb / C4
    struct {
        uintptr_t m_bBombTicking             = 0x11A0; // bool
        uintptr_t m_bBeingDefused            = 0x11DC; // bool
        uintptr_t m_flDefuseCountDown        = 0x11F0; // GameTime_t
        uintptr_t m_nBombSite                = 0x11A4; // int32 
        uintptr_t m_flC4Blow                 = 0x11D0; // GameTime_t
    } C4;

    static uintptr_t get_client_base() {
        static uintptr_t client_base = 0;
        if (!client_base) {
            client_base = reinterpret_cast<uintptr_t>(GetModuleHandleA("client.dll"));
        }
        return client_base;
    }
};

inline Offsets g_offsets;
