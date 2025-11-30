//+KZ has code from character.cpp
#include "character.h"

#include "laser.h"
#include "pickup.h"
#include "projectile.h"

#include <antibot/antibot_data.h>

#include <base/log.h>
#include <base/vmath.h>

#include <engine/antibot.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>
#include <generated/server_data.h>

#include <game/mapitems.h>
#include <game/version.h>

#include <game/params_kz.h> //+KZ

#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/server/teams.h>
#include <game/team_state.h>

#include <game/server/entities/kz/portal_projectile.h>
#include <game/server/entities/kz/portal_laser.h>
#include <game/server/entities/kz/portal.h>

void CCharacter::HandleSubTickStartFinish()
{
	return; //FIXME: causes bugs, remove when fixed

    //static int LastCheckedTick = -1;

    int Length = (int)length(m_PrevVelKZ);

    vec2 CheckingPos;
    vec2 Fraction = (m_Pos - m_PrevPos)/Length;
    int MapIndex = 0;
    int GameIndex = 0;
    int FrontIndex = 0;
    int MapCorners[4] = {0,0,0,0};
    int GameCorners[4] = {0,0,0,0};
    int FrontCorners[4] = {0,0,0,0};

    for(int i = 0; i <= Length; i++)
    {
        if(i == Length) //final pos
            CheckingPos = m_Pos;
        else
            CheckingPos = m_PrevPos + (Fraction * i);

        MapIndex = Collision()->GetMapIndex(CheckingPos);

        MapCorners[0] = Collision()->GetMapIndex(CheckingPos - vec2(GetProximityRadius()/3,GetProximityRadius()/3));
        MapCorners[1] = Collision()->GetMapIndex(CheckingPos + vec2(GetProximityRadius()/3,GetProximityRadius()/3));
        MapCorners[2] = Collision()->GetMapIndex(vec2(CheckingPos.x - (GetProximityRadius()/3), CheckingPos.y + (GetProximityRadius()/3)));
        MapCorners[3] = Collision()->GetMapIndex(vec2(CheckingPos.x + (GetProximityRadius()/3), CheckingPos.x - (GetProximityRadius()/3)));

        GameIndex = Collision()->GetTileIndex(MapIndex);
        FrontIndex = Collision()->GetFrontTileIndex(MapIndex);

        for(int j = 0; j < 4; j++)
        {
            GameCorners[j] = Collision()->GetTileIndex(GameCorners[j]);
            FrontCorners[j] = Collision()->GetFrontTileIndex(GameCorners[j]);
        }

        if(GameServer()->m_pController->HandleCharacterSubTickStart(this, CheckingPos, i, Length) || GameIndex == TILE_START || FrontIndex == TILE_START ||
            GameCorners[0] == TILE_START || GameCorners[1] == TILE_START || GameCorners[2] == TILE_START || GameCorners[3] == TILE_START ||
            FrontCorners[0] == TILE_START || FrontCorners[1] == TILE_START || FrontCorners[2] == TILE_START || FrontCorners[3] == TILE_START)
        {
            m_StartSubTick = i;
            m_StartDivisor = Length;
            m_StartedTickKZ = Server()->Tick();
        }

        if(GameServer()->m_pController->HandleCharacterSubTickFinish(this, CheckingPos, i, Length) && m_FinishSubTick < 0 && m_StartSubTick >= 0 && (GameIndex == TILE_FINISH || FrontIndex == TILE_FINISH ||
            GameCorners[0] == TILE_FINISH || GameCorners[1] == TILE_FINISH || GameCorners[2] == TILE_FINISH || GameCorners[3] == TILE_FINISH ||
            FrontCorners[0] == TILE_FINISH || FrontCorners[1] == TILE_FINISH || FrontCorners[2] == TILE_FINISH || FrontCorners[3] == TILE_FINISH))
        {
            m_FinishSubTick = i;
            m_FinishDivisor = Length;
            m_FinishedTickKZ = Server()->Tick();
            break;
        }
    }

    //LastCheckedTick = Server()->Tick();
}

void CCharacter::HandleKZTiles()
{
	CKZTile *pKZTile = Collision()->GetKZGameTile(m_Pos);
	CKZTile *pKZTileFront = Collision()->GetKZFrontTile(m_Pos);

	if(!pKZTile && !pKZTileFront)
		return;

	if((pKZTile && pKZTile->m_Index == KZ_TILE_PORTAL_RESET) || (pKZTileFront && pKZTileFront->m_Index == KZ_TILE_PORTAL_RESET))
	{
		ResetPortals();
	}

	if(pKZTile && pKZTile->m_Index == KZ_TILE_SOUND_PLAY && m_LastSoundPlayed != pKZTile->m_Value1 && (pKZTile->m_Number ? Switchers()[pKZTile->m_Number].m_aStatus[Team()] : true))
	{
		GameServer()->CreateMapSoundEvent(m_Pos, pKZTile->m_Value1, TeamMask());
		m_LastSoundPlayed = pKZTile->m_Value1;
	}
	else if(pKZTileFront && pKZTileFront->m_Index == KZ_TILE_SOUND_PLAY && m_LastSoundPlayed != pKZTileFront->m_Value1 && (pKZTileFront->m_Number ? Switchers()[pKZTileFront->m_Number].m_aStatus[Team()] : true))
	{
		GameServer()->CreateMapSoundEvent(m_Pos, pKZTileFront->m_Value1, TeamMask());
		m_LastSoundPlayed = pKZTileFront->m_Value1;
	}
	else if(!(pKZTile && pKZTile->m_Index == KZ_TILE_SOUND_PLAY) && !(pKZTileFront && pKZTileFront->m_Index == KZ_TILE_SOUND_PLAY))
	{
		m_LastSoundPlayed = -1;
	}

	if(pKZTile && pKZTile->m_Index == KZ_TILE_SOUND_PLAY_LOCAL && m_LastLocalSoundPlayed != pKZTile->m_Value1 && (pKZTile->m_Number ? Switchers()[pKZTile->m_Number].m_aStatus[Team()] : true))
	{
		CNetMsg_Sv_MapSoundGlobal Msg;
		Msg.m_SoundId = (int)pKZTile->m_Value1;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCid());
		m_LastLocalSoundPlayed = pKZTile->m_Value1;
	}
	else if(pKZTileFront && pKZTileFront->m_Index == KZ_TILE_SOUND_PLAY_LOCAL && m_LastLocalSoundPlayed != pKZTileFront->m_Value1 && (pKZTileFront->m_Number ? Switchers()[pKZTileFront->m_Number].m_aStatus[Team()] : true))
	{
		CNetMsg_Sv_MapSoundGlobal Msg;
		Msg.m_SoundId = (int)pKZTileFront->m_Value1;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCid());
		m_LastLocalSoundPlayed = pKZTileFront->m_Value1;
	}
	else if(!(pKZTile && pKZTile->m_Index == KZ_TILE_SOUND_PLAY_LOCAL) && !(pKZTileFront && pKZTileFront->m_Index == KZ_TILE_SOUND_PLAY_LOCAL))
	{
		m_LastLocalSoundPlayed = -1;
	}

	if(pKZTile && pKZTile->m_Index == KZ_TILE_SOUND_PLAY_LOCAL_IN_POS && m_LastLocalInPosSoundPlayed != pKZTile->m_Value1 && (pKZTile->m_Number ? Switchers()[pKZTile->m_Number].m_aStatus[Team()] : true))
	{
		GameServer()->CreateMapSoundEventForClient(m_Pos, pKZTile->m_Value1, m_pPlayer->GetCid(), TeamMask());
		m_LastLocalInPosSoundPlayed = pKZTile->m_Value1;
	}
	else if(pKZTileFront && pKZTileFront->m_Index == KZ_TILE_SOUND_PLAY_LOCAL_IN_POS && m_LastLocalInPosSoundPlayed != pKZTileFront->m_Value1 && (pKZTileFront->m_Number ? Switchers()[pKZTileFront->m_Number].m_aStatus[Team()] : true))
	{
		GameServer()->CreateMapSoundEventForClient(m_Pos, pKZTileFront->m_Value1, m_pPlayer->GetCid(), TeamMask());
		m_LastLocalInPosSoundPlayed = pKZTileFront->m_Value1;
	}
	else if(!(pKZTile && pKZTile->m_Index == KZ_TILE_SOUND_PLAY_LOCAL_IN_POS) && !(pKZTileFront && pKZTileFront->m_Index == KZ_TILE_SOUND_PLAY_LOCAL_IN_POS))
	{
		m_LastLocalInPosSoundPlayed = -1;
	}

	if(pKZTile && pKZTile->m_Index == KZ_TILE_HEALTH_ZONE && (pKZTile->m_Number ? Switchers()[pKZTile->m_Number].m_aStatus[Team()] : true) && Server()->Tick() % Server()->TickSpeed() == 0)
	{
		IncreaseHealth((int)pKZTile->m_Value1 + 1);
		GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH, TeamMask());
	}
	else if(pKZTileFront && pKZTileFront->m_Index == KZ_TILE_HEALTH_ZONE && (pKZTileFront->m_Number ? Switchers()[pKZTileFront->m_Number].m_aStatus[Team()] : true) && Server()->Tick() % Server()->TickSpeed() == 0)
	{
		IncreaseHealth((int)pKZTileFront->m_Value1 + 1);
		GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH, TeamMask());
	}

	if(pKZTile && pKZTile->m_Index == KZ_TILE_DAMAGE_ZONE && (pKZTile->m_Number ? Switchers()[pKZTile->m_Number].m_aStatus[Team()] : true) && Server()->Tick() % Server()->TickSpeed() == 0)
	{
		TakeDamageVanilla(vec2(0,0),(int)pKZTile->m_Value1 + 1,m_pPlayer ? m_pPlayer->GetCid() : -1, WEAPON_WORLD);
	}
	else if(pKZTileFront && pKZTileFront->m_Index == KZ_TILE_DAMAGE_ZONE && (pKZTileFront->m_Number ? Switchers()[pKZTileFront->m_Number].m_aStatus[Team()] : true) && Server()->Tick() % Server()->TickSpeed() == 0)
	{
		TakeDamageVanilla(vec2(0,0),(int)pKZTile->m_Value1 + 1,m_pPlayer ? m_pPlayer->GetCid() : -1, WEAPON_WORLD);
	}

	if(!m_HasRecoverJumpLaser && pKZTile && pKZTile->m_Index == KZ_TILE_LASER_RECOVER_JUMP_ON && (pKZTile->m_Number ? Switchers()[pKZTile->m_Number].m_aStatus[Team()] : true))
	{
		m_HasRecoverJumpLaser = true;
		GameServer()->SendChatTarget(m_pPlayer->GetCid(),"You can now recover your jumps by shooting yourself with laser");
	}
	else if(!m_HasRecoverJumpLaser && pKZTileFront && pKZTileFront->m_Index == KZ_TILE_LASER_RECOVER_JUMP_ON && (pKZTileFront->m_Number ? Switchers()[pKZTileFront->m_Number].m_aStatus[Team()] : true))
	{
		m_HasRecoverJumpLaser = true;
		GameServer()->SendChatTarget(m_pPlayer->GetCid(),"You can now recover your jumps by shooting yourself with laser");
	}

	if(m_HasRecoverJumpLaser && pKZTile && pKZTile->m_Index == KZ_TILE_LASER_RECOVER_JUMP_OFF && (pKZTile->m_Number ? Switchers()[pKZTile->m_Number].m_aStatus[Team()] : true))
	{
		m_HasRecoverJumpLaser = false;
		GameServer()->SendChatTarget(m_pPlayer->GetCid(),"You can no longer recover your jumps by shooting yourself with laser");
	}
	else if(m_HasRecoverJumpLaser && pKZTileFront && pKZTileFront->m_Index == KZ_TILE_LASER_RECOVER_JUMP_OFF && (pKZTileFront->m_Number ? Switchers()[pKZTileFront->m_Number].m_aStatus[Team()] : true))
	{
		m_HasRecoverJumpLaser = false;
		GameServer()->SendChatTarget(m_pPlayer->GetCid(),"You can no longer recover your jumps by shooting yourself with laser");
	}

	//+KZ Game Only Tiles
	if(pKZTile)
	{
		if(!m_NODAMAGE && pKZTile->m_Index == KZ_GAMETILE_NO_DAMAGE && (pKZTile->m_Number ? Switchers()[pKZTile->m_Number].m_aStatus[Team()] : true))
		{
			m_NODAMAGE = true;
			GameServer()->SendChatTarget(m_pPlayer->GetCid(),"Now you can not take damage");
		}

		if(!IsSuper() && pKZTile->m_Index == KZ_GAMETILE_HITTABLE_SWITCH && pKZTile->m_Number)
		{
			if(!(Server()->Tick() % Server()->TickSpeed()) && !BitWiseAndInt64(pKZTile->m_Value3,KZ_HITTABLE_SWITCH_FLAG_NO_HAMMER))
			{
				GameServer()->SendBroadcast("Hammer the button to use it", m_pPlayer->GetCid(), false);
			}

			if(m_Core.m_ActiveWeapon == WEAPON_HAMMER && !BitWiseAndInt64(pKZTile->m_Value3,KZ_HITTABLE_SWITCH_FLAG_NO_HAMMER) && (m_Input.m_Fire & 1) && !m_StillPressingFire)
			{
				switch(pKZTile->m_Value1) //Type
				{
					case 0: //switch deactivate
						{
							Switchers()[pKZTile->m_Number].m_aStatus[Team()] = true;
							Switchers()[pKZTile->m_Number].m_aEndTick[Team()] = 0;
							Switchers()[pKZTile->m_Number].m_aType[Team()] = TILE_SWITCHOPEN;
							Switchers()[pKZTile->m_Number].m_aLastUpdateTick[Team()] = Server()->Tick();
						}
						break;
					case 1: //switch timed deactivate
						{
							Switchers()[pKZTile->m_Number].m_aStatus[Team()] = true;
							Switchers()[pKZTile->m_Number].m_aEndTick[Team()] = Server()->Tick() + 1 + pKZTile->m_Value2 * Server()->TickSpeed();
							Switchers()[pKZTile->m_Number].m_aType[Team()] = TILE_SWITCHTIMEDOPEN;
							Switchers()[pKZTile->m_Number].m_aLastUpdateTick[Team()] = Server()->Tick();
						}
						break;
					case 2: //switch timed activate
						{
							Switchers()[pKZTile->m_Number].m_aStatus[Team()] = false;
							Switchers()[pKZTile->m_Number].m_aEndTick[Team()] = Server()->Tick() + 1 + pKZTile->m_Value2 * Server()->TickSpeed();
							Switchers()[pKZTile->m_Number].m_aType[Team()] = TILE_SWITCHTIMEDCLOSE;
							Switchers()[pKZTile->m_Number].m_aLastUpdateTick[Team()] = Server()->Tick();
						}
						break;
					case 3: //switch activate
						{
							Switchers()[pKZTile->m_Number].m_aStatus[Team()] = false;
							Switchers()[pKZTile->m_Number].m_aEndTick[Team()] = 0;
							Switchers()[pKZTile->m_Number].m_aType[Team()] = TILE_SWITCHCLOSE;
							Switchers()[pKZTile->m_Number].m_aLastUpdateTick[Team()] = Server()->Tick();
						}
						break;
					case 4: // +KZ switch toggle
						{
							Switchers()[pKZTile->m_Number].m_aStatus[Team()] = !Switchers()[pKZTile->m_Number].m_aStatus[Team()];
							if(Switchers()[pKZTile->m_Number].m_aStatus[Team()])
							{
								Switchers()[pKZTile->m_Number].m_aType[Team()] = TILE_SWITCHOPEN;
							}
							else
							{
								Switchers()[pKZTile->m_Number].m_aType[Team()] = TILE_SWITCHCLOSE;
							}
							Switchers()[pKZTile->m_Number].m_aEndTick[Team()] = 0;
							Switchers()[pKZTile->m_Number].m_aLastUpdateTick[Team()] = Server()->Tick();
						}
						break;

					default:
						break;
				}

				vec2 HitPos = m_Pos;

				HitPos.x += 16 - ((int)HitPos.x % 32);
				HitPos.y += 16 - ((int)HitPos.y % 32);

				GameServer()->CreateHammerHit(HitPos,TeamMask());
			}
		}

		if(!m_SpecTile && pKZTile->m_Index == KZ_GAMETILE_SPEC_POS && (pKZTile->m_Number ? Switchers()[pKZTile->m_Number].m_aStatus[Team()] : true))
		{
			m_SpecTile = true;
			m_SpecTilePos.x = (float)pKZTile->m_Value1 * 32 + 16;
			m_SpecTilePos.y = (float)pKZTile->m_Value2 * 32 + 16;
		}
		else if(m_SpecTile && pKZTile->m_Index != KZ_GAMETILE_SPEC_POS && (pKZTile->m_Number ? !(Switchers()[pKZTile->m_Number].m_aStatus[Team()]) : true))
		{
			m_SpecTile = false;
		}
	}

	//+KZ Front Only Tiles
	if(pKZTileFront)
	{
		if(pKZTileFront->m_Index == KZ_FRONTTILE_FORCE_POS && (pKZTileFront->m_Number ? Switchers()[pKZTileFront->m_Number].m_aStatus[Team()] : true))
		{
			
			//Value 1 = Angle
			int Angle = std::clamp((int)pKZTileFront->m_Value1,0,360);
			float RadAngle = Angle * 3.14159f/180.f;
			vec2 Dir;
			Dir.y = sin(RadAngle);
			Dir.x = cos(RadAngle);
			Dir = normalize(Dir);

			//Value 2 = Force
			//Add Coordinates to Position without caring about anything muahahahaha
			int Force = pKZTileFront->m_Value2;
			m_Core.m_Pos += Dir * Force;
			m_Pos += Dir * Force;
			

			//Value 3 = Speed Limiter
			vec2 TempVel = m_Core.m_Vel;
			int MaxSpeed = pKZTileFront->m_Value3;

			constexpr float MaxSpeedScale = 5.0f;
			if(MaxSpeed < 0)
			{
				float MaxRampSpeed = GetTuning(m_TuneZone)->m_VelrampRange / (50 * log(maximum((float)GetTuning(m_TuneZone)->m_VelrampCurvature, 1.01f)));
				MaxSpeed = maximum(MaxRampSpeed, GetTuning(m_TuneZone)->m_VelrampStart / 50) * MaxSpeedScale;
			}

			// (signed) length of projection
			float CurrentDirectionalSpeed = dot(Dir, m_Core.m_Vel);
			float TempMaxSpeed = MaxSpeed / MaxSpeedScale;
			if(CurrentDirectionalSpeed + Force > TempMaxSpeed)
				TempVel += Dir * (TempMaxSpeed - CurrentDirectionalSpeed);
			else
				TempVel += Dir * Force;

			m_Core.m_Vel = ClampVel(m_MoveRestrictions, TempVel);
		}

		if(!IsSuper() && !m_Core.m_Invincible && pKZTileFront->m_Index == KZ_FRONTTILE_KZ_PLAYER_TELEPORT && (pKZTileFront->m_Number ? Switchers()[pKZTileFront->m_Number].m_aStatus[Team()] : true))
		{
			int TeleNumber = std::clamp((int)pKZTileFront->m_Value2,1,255);
			int TeleOut = 0;

			switch (pKZTileFront->m_Value1) //Type
			{
			case 0: //Red Teleport
				TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleOuts(TeleNumber - 1).size());
				m_Core.m_Pos = Collision()->TeleOuts(TeleNumber - 1)[TeleOut];
				
				m_Core.m_Vel = vec2(0, 0);

				if(!g_Config.m_SvTeleportHoldHook)
				{
					ResetHook();
					GameWorld()->ReleaseHooked(GetPlayer()->GetCid());
				}
				if(g_Config.m_SvTeleportLoseWeapons)
				{
					ResetPickups();
				}
				
				break;
			case 1:
				TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleOuts(TeleNumber - 1).size());
				m_Core.m_Pos = Collision()->TeleOuts(TeleNumber - 1)[TeleOut];
				if(!g_Config.m_SvTeleportHoldHook)
				{
					ResetHook();
				}
				if(g_Config.m_SvTeleportLoseWeapons)
					ResetPickups();
				break;
			default:
				break;
			}
		}

		if(!m_ForcedTuneKZ && pKZTileFront->m_Index == KZ_FRONTTILE_TUNE_SWITCHABLE && (pKZTileFront->m_Number ? Switchers()[pKZTileFront->m_Number].m_aStatus[Team()] : true))
		{
			m_TuneZone = std::clamp((int)pKZTileFront->m_Value1,0,255);
			m_ForcedTuneKZ = true;
		}
		else if(m_ForcedTuneKZ && pKZTileFront->m_Index != KZ_FRONTTILE_TUNE_SWITCHABLE && (pKZTileFront->m_Number ? !(Switchers()[pKZTileFront->m_Number].m_aStatus[Team()]) : true))
		{
			m_ForcedTuneKZ = false;
		}

		if(pKZTileFront->m_Index == KZ_FRONTTILE_TUNE_LOCK && (pKZTileFront->m_Number ? Switchers()[pKZTileFront->m_Number].m_aStatus[Team()] : true))
		{
			m_TuneZoneOverrideKZ = std::clamp((int)pKZTileFront->m_Value1,-1,255);
		}
	}
}

void CCharacter::HandleQuads()
{
	std::vector<SKZQuadData *> apQuads = Collision()->GetQuadsAt(m_Pos);

	int TileId = 0;
	int Number = 0;
	int Delay = 0;

	for(std::vector<SKZQuadData *>::size_type i = 0; i < apQuads.size(); i++)
	{
		TileId = Collision()->QuadTypeToTileId(apQuads[i]);

		if(TileId == -1) //Kaizo-Insta Quad
		{
			if(apQuads[i]->m_pQuad)
			{
				TileId = apQuads[i]->m_pQuad->m_ColorEnvOffset;
				Number = apQuads[i]->m_pQuad->m_aColors[0].r;
				Delay = apQuads[i]->m_pQuad->m_aColors[0].g;
			}
			else
			{
				TileId = TILE_AIR;
			}
		}

		switch (TileId) // Gores tiles
		{
		case TILE_FREEZE:
			Freeze();
			continue;
		case TILE_UNFREEZE:
			UnFreeze();
			continue;
		case TILE_DEATH:
			Die(m_pPlayer->GetCid(),WEAPON_WORLD);
			continue;
		case TILE_TELECHECKINEVIL:
			{
				if(m_Core.m_Super || m_Core.m_Invincible)
					continue;
				// first check if there is a TeleCheckOut for the current recorded checkpoint, if not check previous checkpoints
				for(int k = m_TeleCheckpoint - 1; k >= 0; k--)
				{
					if(!Collision()->TeleCheckOuts(k).empty())
					{
						int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleCheckOuts(k).size());
						m_Core.m_Pos = Collision()->TeleCheckOuts(k)[TeleOut];
						m_Core.m_Vel = vec2(0, 0);

						if(!g_Config.m_SvTeleportHoldHook)
						{
							ResetHook();
							GameWorld()->ReleaseHooked(GetPlayer()->GetCid());
						}

						continue;
					}
				}
				// if no checkpointout have been found (or if there no recorded checkpoint), teleport to start
				vec2 SpawnPos;
				if(GameServer()->m_pController->CanSpawn(m_pPlayer->GetTeam(), &SpawnPos, GameServer()->GetDDRaceTeam(GetPlayer()->GetCid())))
				{
					m_Core.m_Pos = SpawnPos;
					m_Core.m_Vel = vec2(0, 0);

					if(!g_Config.m_SvTeleportHoldHook)
					{
						ResetHook();
						GameWorld()->ReleaseHooked(GetPlayer()->GetCid());
					}
				}
			}
			continue;
		}

		//Kaizo-Insta extra tiles
		if(TileId == TILE_DFREEZE)
		{
			m_Core.m_DeepFrozen = true;
		}
		if(TileId == TILE_LFREEZE)
		{
			m_Core.m_LiveFrozen = true;
		}
		if(TileId == TILE_DUNFREEZE)
		{
			m_Core.m_DeepFrozen = false;
		}
		if(TileId == TILE_LUNFREEZE)
		{
			m_Core.m_LiveFrozen = false;
		}

		// endless hook
		if(TileId == TILE_EHOOK_ENABLE)
		{
			SetEndlessHook(true);
		}
		else if(TileId == TILE_EHOOK_DISABLE)
		{
			SetEndlessHook(false);
		}

		// hit others
		if((TileId == TILE_HIT_DISABLE) && (!m_Core.m_HammerHitDisabled || !m_Core.m_ShotgunHitDisabled || !m_Core.m_GrenadeHitDisabled || !m_Core.m_LaserHitDisabled))
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't hit others");
			m_Core.m_HammerHitDisabled = true;
			m_Core.m_ShotgunHitDisabled = true;
			m_Core.m_GrenadeHitDisabled = true;
			m_Core.m_LaserHitDisabled = true;
		}
		else if((TileId == TILE_HIT_ENABLE) && (m_Core.m_HammerHitDisabled || m_Core.m_ShotgunHitDisabled || m_Core.m_GrenadeHitDisabled || m_Core.m_LaserHitDisabled))
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can hit others");
			m_Core.m_ShotgunHitDisabled = false;
			m_Core.m_GrenadeHitDisabled = false;
			m_Core.m_HammerHitDisabled = false;
			m_Core.m_LaserHitDisabled = false;
		}

		// collide with others
		if((TileId == TILE_NPC_DISABLE) && !m_Core.m_CollisionDisabled)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't collide with others");
			m_Core.m_CollisionDisabled = true;
		}
		else if((TileId == TILE_NPC_ENABLE) && m_Core.m_CollisionDisabled)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can collide with others");
			m_Core.m_CollisionDisabled = false;
		}

		// hook others
		if((TileId == TILE_NPH_DISABLE) && !m_Core.m_HookHitDisabled)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't hook others");
			m_Core.m_HookHitDisabled = true;
		}
		else if((TileId == TILE_NPH_ENABLE) && m_Core.m_HookHitDisabled)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can hook others");
			m_Core.m_HookHitDisabled = false;
		}

		// unlimited air jumps
		if((TileId == TILE_UNLIMITED_JUMPS_ENABLE) && !m_Core.m_EndlessJump)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You have unlimited air jumps");
			m_Core.m_EndlessJump = true;
		}
		else if((TileId == TILE_UNLIMITED_JUMPS_DISABLE) && m_Core.m_EndlessJump)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You don't have unlimited air jumps");
			m_Core.m_EndlessJump = false;
		}

		// walljump
		if(TileId == TILE_WALLJUMP)
		{
			if(m_Core.m_Vel.y > 0 && m_Core.m_Colliding && m_Core.m_LeftWall)
			{
				m_Core.m_LeftWall = false;
				m_Core.m_JumpedTotal = m_Core.m_Jumps >= 2 ? m_Core.m_Jumps - 2 : 0;
				m_Core.m_Jumped = 1;
			}
		}

		// jetpack gun
		if(((TileId == TILE_JETPACK_ENABLE)) && !m_Core.m_Jetpack)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You have a jetpack gun");
			m_Core.m_Jetpack = true;
		}
		else if(((TileId == TILE_JETPACK_DISABLE)) && m_Core.m_Jetpack)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You lost your jetpack gun");
			m_Core.m_Jetpack = false;
		}

		// Teleport gun
		if((TileId == TILE_TELE_GUN_ENABLE) && !m_Core.m_HasTelegunGun)
		{
			m_Core.m_HasTelegunGun = true;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport gun enabled");
		}
		else if((TileId == TILE_TELE_GUN_DISABLE) && m_Core.m_HasTelegunGun)
		{
			m_Core.m_HasTelegunGun = false;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport gun disabled");
		}

		if((TileId == TILE_TELE_GRENADE_ENABLE) && !m_Core.m_HasTelegunGrenade)
		{
			m_Core.m_HasTelegunGrenade = true;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport grenade enabled");
		}
		else if((TileId == TILE_TELE_GRENADE_DISABLE) && m_Core.m_HasTelegunGrenade)
		{
			m_Core.m_HasTelegunGrenade = false;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport grenade disabled");
		}

		if((TileId == TILE_TELE_LASER_ENABLE) && !m_Core.m_HasTelegunLaser)
		{
			m_Core.m_HasTelegunLaser = true;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport laser enabled");
		}
		else if((TileId == TILE_TELE_LASER_DISABLE) && m_Core.m_HasTelegunLaser)
		{
			m_Core.m_HasTelegunLaser = false;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport laser disabled");
		}
		else if(TileId == TILE_HIT_ENABLE && m_Core.m_HammerHitDisabled && Delay == WEAPON_HAMMER)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can hammer hit others");
			m_Core.m_HammerHitDisabled = false;
		}
		else if(TileId == TILE_HIT_DISABLE && !(m_Core.m_HammerHitDisabled) && Delay == WEAPON_HAMMER)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't hammer hit others");
			m_Core.m_HammerHitDisabled = true;
		}
		else if(TileId == TILE_HIT_ENABLE && m_Core.m_ShotgunHitDisabled && Delay == WEAPON_SHOTGUN)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can shoot others with shotgun");
			m_Core.m_ShotgunHitDisabled = false;
		}
		else if(TileId == TILE_HIT_DISABLE && !(m_Core.m_ShotgunHitDisabled) && Delay == WEAPON_SHOTGUN)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't shoot others with shotgun");
			m_Core.m_ShotgunHitDisabled = true;
		}
		else if(TileId == TILE_HIT_ENABLE && m_Core.m_GrenadeHitDisabled && Delay == WEAPON_GRENADE)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can shoot others with grenade");
			m_Core.m_GrenadeHitDisabled = false;
		}
		else if(TileId == TILE_HIT_DISABLE && !(m_Core.m_GrenadeHitDisabled) && Delay == WEAPON_GRENADE)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't shoot others with grenade");
			m_Core.m_GrenadeHitDisabled = true;
		}
		else if(TileId == TILE_HIT_ENABLE && m_Core.m_LaserHitDisabled && Delay == WEAPON_LASER)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can shoot others with laser");
			m_Core.m_LaserHitDisabled = false;
		}
		else if(TileId == TILE_HIT_DISABLE && !(m_Core.m_LaserHitDisabled) && Delay == WEAPON_LASER)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't shoot others with laser");
			m_Core.m_LaserHitDisabled = true;
		}
		else if(TileId == TILE_JUMP)
		{
			int NewJumps = Delay;
			if(NewJumps == 255)
			{
				NewJumps = -1;
			}

			if(NewJumps != m_Core.m_Jumps)
			{
				char aBuf[256];
				if(NewJumps == -1)
					str_copy(aBuf, "You only have your ground jump now");
				else if(NewJumps == 1)
					str_format(aBuf, sizeof(aBuf), "You can jump %d time", NewJumps);
				else
					str_format(aBuf, sizeof(aBuf), "You can jump %d times", NewJumps);
				GameServer()->SendChatTarget(GetPlayer()->GetCid(), aBuf);
				m_Core.m_Jumps = NewJumps;
			}
		}

		// handle switch tiles
		if(TileId == TILE_SWITCHOPEN && Team() != TEAM_SUPER && Number > 0)
		{
			Switchers()[Number].m_aStatus[Team()] = true;
			Switchers()[Number].m_aEndTick[Team()] = 0;
			Switchers()[Number].m_aType[Team()] = TILE_SWITCHOPEN;
			Switchers()[Number].m_aLastUpdateTick[Team()] = Server()->Tick();
		}
		else if(TileId == TILE_SWITCHTIMEDOPEN && Team() != TEAM_SUPER && Number > 0)
		{
			Switchers()[Number].m_aStatus[Team()] = true;
			Switchers()[Number].m_aEndTick[Team()] = Server()->Tick() + 1 + Delay * Server()->TickSpeed();
			Switchers()[Number].m_aType[Team()] = TILE_SWITCHTIMEDOPEN;
			Switchers()[Number].m_aLastUpdateTick[Team()] = Server()->Tick();
		}
		else if(TileId == TILE_SWITCHTIMEDCLOSE && Team() != TEAM_SUPER && Number > 0)
		{
			Switchers()[Number].m_aStatus[Team()] = false;
			Switchers()[Number].m_aEndTick[Team()] = Server()->Tick() + 1 + Delay * Server()->TickSpeed();
			Switchers()[Number].m_aType[Team()] = TILE_SWITCHTIMEDCLOSE;
			Switchers()[Number].m_aLastUpdateTick[Team()] = Server()->Tick();
		}
		else if(TileId == TILE_SWITCHCLOSE && Team() != TEAM_SUPER && Number > 0)
		{
			Switchers()[Number].m_aStatus[Team()] = false;
			Switchers()[Number].m_aEndTick[Team()] = 0;
			Switchers()[Number].m_aType[Team()] = TILE_SWITCHCLOSE;
			Switchers()[Number].m_aLastUpdateTick[Team()] = Server()->Tick();
		}

		//Teleports
		if(!g_Config.m_SvOldTeleportHook && !g_Config.m_SvOldTeleportWeapons && TileId == TILE_TELEIN && !Collision()->TeleOuts(Number - 1).empty())
		{
			if(!(m_Core.m_Super || m_Core.m_Invincible))
			{
				int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleOuts(Number - 1).size());
				m_Core.m_Pos = Collision()->TeleOuts(Number - 1)[TeleOut];
				if(!g_Config.m_SvTeleportHoldHook)
				{
					ResetHook();
				}
				if(g_Config.m_SvTeleportLoseWeapons)
					ResetPickups();
			}
		}
		if(TileId == TILE_TELEINEVIL && !Collision()->TeleOuts(Number - 1).empty())
		{
			if(!(m_Core.m_Super || m_Core.m_Invincible))
			{
				int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleOuts(Number - 1).size());
				m_Core.m_Pos = Collision()->TeleOuts(Number - 1)[TeleOut];
				if(!g_Config.m_SvOldTeleportHook && !g_Config.m_SvOldTeleportWeapons)
				{
					m_Core.m_Vel = vec2(0, 0);

					if(!g_Config.m_SvTeleportHoldHook)
					{
						ResetHook();
						GameWorld()->ReleaseHooked(GetPlayer()->GetCid());
					}
					if(g_Config.m_SvTeleportLoseWeapons)
					{
						ResetPickups();
					}
				}
			}
		}
		if(TileId == TILE_TELECHECKIN)
		{
			if(!(m_Core.m_Super || m_Core.m_Invincible))
			{
				bool dontteletospawn = false;
				// first check if there is a TeleCheckOut for the current recorded checkpoint, if not check previous checkpoints
				for(int k = m_TeleCheckpoint - 1; k >= 0; k--)
				{
					if(!Collision()->TeleCheckOuts(k).empty())
					{
						int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleCheckOuts(k).size());
						m_Core.m_Pos = Collision()->TeleCheckOuts(k)[TeleOut];

						if(!g_Config.m_SvTeleportHoldHook)
						{
							ResetHook();
						}
						dontteletospawn = true;
						break;
					}
				}
				// if no checkpointout have been found (or if there no recorded checkpoint), teleport to start
				if(!dontteletospawn)
				{
					vec2 SpawnPos;
					if(GameServer()->m_pController->CanSpawn(m_pPlayer->GetTeam(), &SpawnPos, GameServer()->GetDDRaceTeam(GetPlayer()->GetCid())))
					{
						m_Core.m_Pos = SpawnPos;

						if(!g_Config.m_SvTeleportHoldHook)
						{
							ResetHook();
						}
					}
				}
			}
		}
		GameServer()->m_pController->HandleCharacterQuad(this, apQuads[i]);
	}
}

void CCharacter::ResetPortals()
{
	for(CPortalKZ *p = (CPortalKZ *)GameWorld()->FindFirst(CGameWorld::CUSTOM_ENTTYPE_PORTAL); p; p = (CPortalKZ *)p->TypeNext())
	{
		if(p->m_Owner == m_pPlayer->GetCid())
		{
			p->Reset();
			CPortalKZ *p2 = p->GetOtherPortal();
			if(p2)
			{
				p2->Reset();
			}
			break;
		}
	}
}

bool CCharacter::TakeDamageVanilla(vec2 Force, int Dmg, int From, int Weapon)
{
	m_Core.m_Vel += Force;

	if(m_NODAMAGE || IsSuper() || m_Core.m_Invincible)
		return false;

	// m_pPlayer only inflicts half damage on self //+KZ modified for damage obstacles
	if(Weapon >= 0 && From == m_pPlayer->GetCid())
		Dmg = maximum(1, Dmg/2);

	int OldHealth = m_Health;
	if(Dmg)
	{
		if(g_Config.m_SvKaizoVanillaMode && m_Armor)
		{
			if(Dmg > 1)
			{
				m_Health--;
				Dmg--;
			}

			if(Dmg > m_Armor)
			{
				Dmg -= m_Armor;
				m_Armor = 0;
			}
			else
			{
				m_Armor -= Dmg;
				Dmg = 0;
			}
		}

		m_Health -= Dmg;
	}

	// create healthmod indicator
	GameServer()->CreateDamageInd(m_Pos, Server()->Tick(), OldHealth-m_Health, TeamMask());

	// do damage Hit sound
    if(Weapon < 0)
	    GameServer()->CreateSound(m_Pos, SOUND_HIT, TeamMask());

	// check for death
	if(m_Health <= 0)
	{
		Die(From, Weapon);
	}

	if(Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);

	SetEmote(EMOTE_PAIN, Server()->Tick() + 500 * Server()->TickSpeed() / 1000);

	return true;
}
