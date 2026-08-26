#pragma once
#include <Windows.h>
#include <cstdint>

struct Offsets {
    // client.dll Offsets
    struct {
        uintptr_t dwEntityList               = 0x2572230; 
        uintptr_t dwViewMatrix               = 0x23CC830;
        uintptr_t dwViewRender               = 0x23CC898;
        uintptr_t dwLocalPlayerPawn          = 0x23C7268;
        uintptr_t dwLocalPlayerController    = 0x23A1F30;
        uintptr_t dwGlobalVars               = 0x20B05F0;
        uintptr_t dwPlantedC4                = 0x2391A18;
        uintptr_t dwWeaponC4                 = 0x233FF10;
    } client;                                
                                             
    // Entity & Pawn                         
    struct {                                 
        uintptr_t m_iTeamNum                 = 0x3E7; 
        uintptr_t m_pGameSceneNode           = 0x330; 
        uintptr_t m_iHealth                  = 0x34C;
        uintptr_t m_hOwnerEntity             = 0x520;
    } C_BaseEntity;                          
                                             
    struct {                                 
        uintptr_t m_vecAbsOrigin             = 0xC8; 
    } CGameSceneNode;                        
                                             
    struct {                                 
        uintptr_t m_modelState               = 0x140; 
    } CSkeletonInstance;               
                                       
    // Controllers                     
    struct {                           
        uintptr_t m_hPlayerPawn              = 0x914; 
        uintptr_t m_hPawn                    = 0x600;
        uintptr_t m_sSanitizedPlayerName     = 0x868; 
        uintptr_t m_hObserverPawn            = 0x918;
        uintptr_t m_iObserverMode            = 0x48;
    } CCSPlayerController;

    struct {
        uintptr_t m_pObserverServices        = 0x1220; 
    } C_BasePlayerPawn;

    struct {
        uintptr_t m_hObserverTarget          = 0x4C; 
    } CPlayer_ObserverServices;

    struct {
        uintptr_t m_bIsScoped                = 0x1C78; 
        uintptr_t m_entitySpottedState       = 0x1C60; 
        uintptr_t m_iIDEntIndex              = 0x342C; 
    } C_CSPlayerPawn;

    struct {
        uintptr_t m_vecViewOffset            = 0xE78; 
    } C_BaseModelEntity;

    // Weapon Services
    struct {
        uintptr_t m_pWeaponServices          = 0x1208;
        uintptr_t m_flFlashDuration          = 0x1428; 
    } C_CSPlayerPawnBase;

    struct {
        uintptr_t m_hActiveWeapon            = 0x60; 
        uintptr_t m_hMyWeapons               = 0x48; 
    } CPlayer_WeaponServices;

    struct {
        uintptr_t m_AttributeManager         = 0x13D0;
    } C_EconEntity;

    struct {
        uintptr_t m_Item                     = 0x50; 
    } C_AttributeContainer;

    struct {
        uintptr_t m_iItemDefinitionIndex     = 0x1BA; 
    } C_EconItemView;

    // Spotted State
    struct {
        uintptr_t m_bSpotted                 = 0x8; 
        uintptr_t m_bSpottedByMask           = 0xC; 
    } EntitySpottedState_t;

    struct {
        uintptr_t m_pEntity                  = 0x10; 
    } CEntityInstance;

    struct {
        uintptr_t m_designerName             = 0x20; 
    } CEntityIdentity;

    // Bomb / C4
    struct {
        uintptr_t m_bBombTicking             = 0x11A0; 
        uintptr_t m_bBeingDefused            = 0x11DC; 
        uintptr_t m_flDefuseCountDown        = 0x11F0; 
        uintptr_t m_nBombSite                = 0x11A4; 
        uintptr_t m_flC4Blow                 = 0x11D0; 
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
