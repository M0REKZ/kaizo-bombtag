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
        if(!g_Config.m_SvOldTeleportHook && !g_Config.m_SvOldTeleportWeapons && z && !Collision()->TeleOuts(z - 1).empty())
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
        if(evilz && !Collision()->TeleOuts(evilz - 1).empty())
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
                if(!Collision()->TeleCheckOuts(k).empty())
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
                if(!Collision()->TeleCheckOuts(k).empty())
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