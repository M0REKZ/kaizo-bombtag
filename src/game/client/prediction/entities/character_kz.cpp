// Copyright (C) Benjamín Gajardo (also known as +KZ)
//
// KaizoPredictNormalTiles has code from character.cpp (server version)

#include <engine/shared/config.h>
#include <game/collision.h>
#include "character.h"

void CCharacter::KaizoPredictNormalTiles(int Index)
{
    int MapIndex = Index;

    if(g_Config.m_KaizoPredictDDNetTeleport && GameWorld()->m_WorldConfig.m_IsDDRace)
    {
        int z = Collision()->IsTeleport(MapIndex);
        if(!g_Config.m_SvOldTeleportHook && !g_Config.m_SvOldTeleportWeapons && z && Collision()->TeleOuts(z - 1).size() == 1)
        {
            if(m_Core.m_Super || m_Core.m_Invincible)
                return;
            int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleOuts(z - 1).size());
            m_Core.m_Pos = Collision()->TeleOuts(z - 1)[TeleOut];
            m_DontMixPredictedPos = true; //+KZ added this
            if(!g_Config.m_SvTeleportHoldHook)
            {
                ResetHook();
            }
            if(g_Config.m_SvTeleportLoseWeapons)
                ResetPickups();
            return;
        }
        int evilz = Collision()->IsEvilTeleport(MapIndex);
        if(evilz && Collision()->TeleOuts(evilz - 1).size() == 1)
        {
            if(m_Core.m_Super || m_Core.m_Invincible)
                return;
            int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleOuts(evilz - 1).size());
            m_Core.m_Pos = Collision()->TeleOuts(evilz - 1)[TeleOut];
            m_DontMixPredictedPos = true; //+KZ added this
            if(!g_Config.m_SvOldTeleportHook && !g_Config.m_SvOldTeleportWeapons)
            {
                m_Core.m_Vel = vec2(0, 0);

                if(!g_Config.m_SvTeleportHoldHook)
                {
                    ResetHook();
                    GameWorld()->ReleaseHooked(GetCid());
                }
                if(g_Config.m_SvTeleportLoseWeapons)
                {
                    ResetPickups();
                }
            }
            return;
        }
        if(Collision()->IsCheckEvilTeleport(MapIndex))
        {
            if(m_Core.m_Super || m_Core.m_Invincible)
                return;
            // first check if there is a TeleCheckOut for the current recorded checkpoint, if not check previous checkpoints
            for(int k = m_TeleCheckpoint - 1; k >= 0; k--)
            {
                if(Collision()->TeleCheckOuts(k).size() == 1)
                {
                    int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleCheckOuts(k).size());
                    m_Core.m_Pos = Collision()->TeleCheckOuts(k)[TeleOut];
                    m_Core.m_Vel = vec2(0, 0);
                    m_DontMixPredictedPos = true; //+KZ added this

                    if(!g_Config.m_SvTeleportHoldHook)
                    {
                        ResetHook();
                        GameWorld()->ReleaseHooked(GetCid());
                    }

                    return;
                }
            }
            // if no checkpointout have been found (or if there no recorded checkpoint), teleport to start
            return;
        }
        if(Collision()->IsCheckTeleport(MapIndex))
        {
            if(m_Core.m_Super || m_Core.m_Invincible)
                return;
            // first check if there is a TeleCheckOut for the current recorded checkpoint, if not check previous checkpoints
            for(int k = m_TeleCheckpoint - 1; k >= 0; k--)
            {
                if(Collision()->TeleCheckOuts(k).size() == 1)
                {
                    int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleCheckOuts(k).size());
                    m_Core.m_Pos = Collision()->TeleCheckOuts(k)[TeleOut];
                    m_DontMixPredictedPos = true; //+KZ added this

                    if(!g_Config.m_SvTeleportHoldHook)
                    {
                        ResetHook();
                    }

                    return;
                }
            }
            // if no checkpointout have been found (or if there no recorded checkpoint), teleport to start
            return;
        }
    }
    // From Pointer's TW+
    if(g_Config.m_KaizoPredictPointerTWPlus && GameWorld()->m_WorldConfig.m_IsPointerTWPlus)
    {
        int CornersTileIds[4];

        CornersTileIds[0] = Collision()->GetTileIndex(Collision()->GetMapIndex(vec2(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f)));
        CornersTileIds[1] = Collision()->GetTileIndex(Collision()->GetMapIndex(vec2(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f)));
        CornersTileIds[2] = Collision()->GetTileIndex(Collision()->GetMapIndex(vec2(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f)));
        CornersTileIds[3] = Collision()->GetTileIndex(Collision()->GetMapIndex(vec2(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f)));

        // speedup
        if((CornersTileIds[0] == POINTER_TILE_SPEEDUPFAST ||
                CornersTileIds[1] == POINTER_TILE_SPEEDUPFAST ||
                CornersTileIds[2] == POINTER_TILE_SPEEDUPFAST ||
                CornersTileIds[3] == POINTER_TILE_SPEEDUPFAST))
        {
            m_Core.m_Vel += {0, -5};
        }
        
        if(m_Core.m_TriggeredEvents & COREEVENT_GROUND_JUMP)
        {
            if (Collision()->GetTileIndex(Collision()->GetMapIndex(vec2(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + 2*32))) == POINTER_TILE_SPEEDUPFAST ||
                Collision()->GetTileIndex(Collision()->GetMapIndex(vec2(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + 2*32))) == POINTER_TILE_SPEEDUPFAST)
                m_Core.m_Vel = {m_Core.m_Vel.x, m_Core.m_Vel.y * 2};
        }

        // teleports
        if (CornersTileIds[0] == POINTER_TILE_TELEONE ||
            CornersTileIds[1] == POINTER_TILE_TELEONE ||
            CornersTileIds[2] == POINTER_TILE_TELEONE ||
            CornersTileIds[3] == POINTER_TILE_TELEONE)
        {
            if (!m_InPointerTele) {
                m_InPointerTele = true;
                int x = GameWorld()->m_PointerTelePositions[0].m_X;
                int y = GameWorld()->m_PointerTelePositions[0].m_Y;
                int tx = GameWorld()->m_PointerTelePositions[1].m_X;
                int ty = GameWorld()->m_PointerTelePositions[1].m_Y;
                vec2 start = {(float)x, (float)y};
                vec2 end = {(float)tx, (float)ty};
                m_Pos = m_Pos - start * 32 + end * 32;
                m_Core.m_Pos = m_Pos;
            }
        }
        else if (CornersTileIds[0] == POINTER_TILE_TELETWO ||
                CornersTileIds[1] == POINTER_TILE_TELETWO ||
                CornersTileIds[2] == POINTER_TILE_TELETWO ||
                CornersTileIds[3] == POINTER_TILE_TELETWO)
        {
            if (!m_InPointerTele) {
                m_InPointerTele = true;
                int x = GameWorld()->m_PointerTelePositions[1].m_X;
                int y = GameWorld()->m_PointerTelePositions[1].m_Y;
                int tx = GameWorld()->m_PointerTelePositions[0].m_X;
                int ty = GameWorld()->m_PointerTelePositions[0].m_Y;
                vec2 start = {(float)x, (float)y};
                vec2 end = {(float)tx, (float)ty};
                m_Pos = m_Pos - start * 32 + end * 32;
                m_Core.m_Pos = m_Pos;
            }
        }
        else if (CornersTileIds[0] == POINTER_TILE_TELETHREE ||
                CornersTileIds[1] == POINTER_TILE_TELETHREE ||
                CornersTileIds[2] == POINTER_TILE_TELETHREE ||
                CornersTileIds[3] == POINTER_TILE_TELETHREE)
        {
            if (!m_InPointerTele) {
                m_InPointerTele = true;
                int x = GameWorld()->m_PointerTelePositions[2].m_X;
                int y = GameWorld()->m_PointerTelePositions[2].m_Y;
                int tx = GameWorld()->m_PointerTelePositions[3].m_X;
                int ty = GameWorld()->m_PointerTelePositions[3].m_Y;
                vec2 start = {(float)x, (float)y};
                vec2 end = {(float)tx, (float)ty};
                m_Pos = m_Pos - start * 32 + end * 32;
                m_Core.m_Pos = m_Pos;
            }
        }
        else if (CornersTileIds[0] == POINTER_TILE_TELEFOUR ||
                CornersTileIds[1] == POINTER_TILE_TELEFOUR ||
                CornersTileIds[2] == POINTER_TILE_TELEFOUR ||
                CornersTileIds[3] == POINTER_TILE_TELEFOUR)
        {
            if (!m_InPointerTele) {
                m_InPointerTele = true;
                int x = GameWorld()->m_PointerTelePositions[3].m_X;
                int y = GameWorld()->m_PointerTelePositions[3].m_Y;
                int tx = GameWorld()->m_PointerTelePositions[2].m_X;
                int ty = GameWorld()->m_PointerTelePositions[2].m_Y;
                vec2 start = {(float)x, (float)y};
                vec2 end = {(float)tx, (float)ty};
                m_Pos = m_Pos - start * 32 + end * 32;
                m_Core.m_Pos = m_Pos;
            }
        }
        else
        {
            // you are not not in a teleport
            m_InPointerTele = false;
        }
    }
}

void CCharacter::ResetPickups()
{
	for(int i = WEAPON_SHOTGUN; i < NUM_WEAPONS - 1; i++)
	{
		m_Core.m_aWeapons[i].m_Got = false;
		if(m_Core.m_ActiveWeapon == i)
			m_Core.m_ActiveWeapon = WEAPON_GUN;
	}
}