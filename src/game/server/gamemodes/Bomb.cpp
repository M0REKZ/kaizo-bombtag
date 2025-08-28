#include "Bomb.h"

#include <base/color.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>
#include <engine/shared/protocol.h>
#include <engine/server/server.h>
#include <game/gamecore.h>
#include <game/generated/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <random>

#include <game/server/entities/kz/kz_pickup.h>
#include <game/server/entities/kz/kz_gun.h>
#include <game/server/entities/kz/kz_light.h>
#include <game/server/entities/kz/mine.h>

// Exchange this to a string that identifies your game mode.
// DM, TDM and CTF are reserved for teeworlds original modes.
// DDraceNetwork and TestDDraceNetwork are used by DDNet.
#define GAME_TYPE_NAME "BOMB++"

CGameControllerBomb::CGameControllerBomb(class CGameContext *pGameServer) :
	IGameController(pGameServer), m_Teams(pGameServer)
{
	m_pGameType = GAME_TYPE_NAME;

	m_RoundActive = false;
	for(auto &aPlayer : m_aPlayers)
	{
		aPlayer.m_State = STATE_NONE;
		aPlayer.m_Bomb = false;
	}

	m_apFlags[0] = 0;
	m_apFlags[1] = 0;

	m_flagstand_temp_i_0 = 0;
	m_flagstand_temp_i_1 = 0; 
}

CGameControllerBomb::~CGameControllerBomb() = default;

void CGameControllerBomb::OnCharacterSpawn(CCharacter *pChr)
{
	int ClientId = pChr->GetPlayer()->GetCid();
	IGameController::OnCharacterSpawn(pChr);
	pChr->SetTeams(&m_Teams);
	//pChr->SetTeleports(&m_TeleOuts, &m_TeleCheckOuts);
	m_Teams.OnCharacterSpawn(ClientId);

	pChr->SetArmor(10);
	pChr->GiveWeapon(WEAPON_GUN, true);
	pChr->SetWeapon(WEAPON_HAMMER);

	if(m_aPlayers[ClientId].m_Bomb && m_RoundActive)
		MakeBomb(ClientId, m_aPlayers[ClientId].m_Tick);

	if(m_RoundActive)
		SetSkin(pChr->GetPlayer());
}

int CGameControllerBomb::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon)
{
	int ClientId = pVictim->GetPlayer()->GetCid();
	if(m_aPlayers[ClientId].m_State == STATE_ALIVE)
	{
		EliminatePlayer(ClientId);
	}

	if(m_aPlayers[ClientId].m_State > STATE_ACTIVE && m_RoundActive)
	{
		GameServer()->SendBroadcast("You will automatically rejoin the game when the round is over", ClientId);
		m_aPlayers[ClientId].m_State = STATE_ACTIVE;
	}
	
	int HadFlag = 0;

	// drop flags
	for(CFlag *pFlag : m_apFlags)
	{
		if(pFlag && pKiller && pKiller->GetCharacter() && pFlag->GetCarrier() == pKiller->GetCharacter())
			HadFlag |= 2;
		if(pFlag && pFlag->GetCarrier() == pVictim)
		{
			GameServer()->CreateSoundGlobal(SOUND_CTF_DROP);
			GameServer()->SendGameMsg(protocol7::GAMEMSG_CTF_DROP, -1);
			pFlag->Drop();
			// https://github.com/ddnet-insta/ddnet-insta/issues/156
			pFlag->m_pLastCarrier = nullptr;

			HadFlag |= 1;
		}
		if(pFlag && pFlag->GetCarrier() == pVictim)
			pFlag->SetCarrier(0);
	}

	return HadFlag;
}

void CGameControllerBomb::OnPlayerConnect(CPlayer *pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	GameServer()->Score()->LoadPlayerGamesWon(pPlayer->GetCid(), Server()->ClientName(pPlayer->GetCid()));
	int ClientId = pPlayer->GetCid();

	if(pPlayer->GetTeam() == TEAM_SPECTATORS && m_aPlayers[ClientId].m_State != STATE_ACTIVE)
		m_aPlayers[ClientId].m_State = STATE_SPECTATING;

	if(!Server()->ClientPrevIngame(ClientId))
	{
		char aBuf[512];
		char aClientName[64];
		GameServer()->IdentifyClientName(ClientId, aClientName, sizeof(aClientName));
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s using %s", Server()->ClientName(ClientId), GetTeamName(pPlayer->GetTeam()), aClientName);
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);
		GameServer()->SendDiscordChatMessage(-1,aBuf); //+KZ
		GameServer()->SendChatTarget(ClientId, "BOMB Mod (Merged with Kaizo Network by +KZ). Website: m0rekz.github.io");
		GameServer()->SendChatTarget(ClientId, "Original source code: https://git.ddstats.tw/furo/ddnet-bombtag");
	}
	if(m_RoundActive && m_aPlayers[ClientId].m_State != STATE_SPECTATING)
	{
		GameServer()->SendBroadcast("There's currently a game in progress, you'll join once the round is over!", ClientId);
	}
	SetSkin(pPlayer);
}

void CGameControllerBomb::OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason)
{
	IGameController::OnPlayerDisconnect(pPlayer, pReason);
	m_aPlayers[pPlayer->GetCid()].m_State = STATE_NONE;
}

void CGameControllerBomb::OnReset()
{
	IGameController::OnReset();
	m_Teams.Reset();

	// Bombtag reset
	for(auto &aPlayer : m_aPlayers)
	{
		if(aPlayer.m_State >= STATE_ACTIVE)
		{
			aPlayer.m_State = STATE_ACTIVE;
			aPlayer.m_Bomb = false;
			aPlayer.m_CollateralKills = 0;
			aPlayer.m_RoundsSurvived = 0;
			aPlayer.m_HammerKills = 0;
		}
		m_RoundActive = false;
	}
}

void CGameControllerBomb::DoAfkLogic()
{
	if(!m_RoundActive)
		return;

	for(auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		CCharacter *pChr = pPlayer->GetCharacter();
		if(!pChr)
			continue;

		if(m_aPlayers[pPlayer->GetCid()].m_State == STATE_SPECTATING)
			continue;

		int Angle = (std::atan2(pChr->Core()->m_Input.m_TargetX, pChr->Core()->m_Input.m_TargetY) * 100.0f);
		int AfkHash = pChr->Core()->m_Input.m_PlayerFlags + Angle + pChr->Core()->m_Direction + pChr->Core()->m_Jumps;

		if(AfkHash == m_aPlayers[pPlayer->GetCid()].m_AfkHash)
		{
			m_aPlayers[pPlayer->GetCid()].m_TicksAfk++;
		}
		else
		{
			m_aPlayers[pPlayer->GetCid()].m_TicksAfk = 0;
		}

		m_aPlayers[pPlayer->GetCid()].m_AfkHash = AfkHash;
		if(m_aPlayers[pPlayer->GetCid()].m_TicksAfk > 20 * SERVER_TICK_SPEED)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "'%s' has been moved to spectators due to inactivity.", Server()->ClientName(pPlayer->GetCid()));
			GameServer()->SendChat(-1, TEAM_ALL, aBuf);
			GameServer()->m_apPlayers[pPlayer->GetCid()]->SetTeam(TEAM_SPECTATORS, false);
			m_aPlayers[pPlayer->GetCid()].m_TicksAfk = 0;
			m_aPlayers[pPlayer->GetCid()].m_State = STATE_SPECTATING;
		}
	}
}

void CGameControllerBomb::Tick()
{
	IGameController::Tick();
	FlagTick(); //+KZ
	DoAfkLogic();

	// Change to enqueued map
	if(!m_RoundActive && !m_Warmup && str_length(m_aEnqueuedMap))
	{
		Server()->ChangeMap(m_aEnqueuedMap);
		str_copy(m_aEnqueuedMap, "");
		return;
	}

	if(AmountOfPlayers(STATE_ACTIVE) == 1 && !m_RoundActive)
	{
		int Sequence = Server()->Tick() % (SERVER_TICK_SPEED * 3);
		if(Sequence == 50)
			GameServer()->SendBroadcast("Waiting for players.", -1);
		else if(Sequence == 100)
			GameServer()->SendBroadcast("Waiting for players..", -1);
		else if(Sequence == 0)
			GameServer()->SendBroadcast("Waiting for players...", -1);
	}
	if(!m_RoundActive && AmountOfPlayers(STATE_ACTIVE) + AmountOfPlayers(STATE_ALIVE) > 1 && !m_Warmup)
	{
		GameServer()->SendBroadcast("Game started", -1);
		StartBombRound();
	}
	DoWinCheck();
	if(m_RoundActive)
		SetSkins();

	for(auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(m_aPlayers[pPlayer->GetCid()].m_Bomb)
			continue;

		CCharacter *pChr = pPlayer->GetCharacter();
		if(!pChr)
			continue;

		pChr->GiveWeapon(WEAPON_HAMMER);
		pChr->GiveWeapon(WEAPON_GUN, true);
	}
}

void CGameControllerBomb::DoTeamChange(class CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	Team = ClampTeam(Team);
	if(Team == pPlayer->GetTeam())
		return;

	if(Team == TEAM_SPECTATORS)
	{
		if(m_aPlayers[pPlayer->GetCid()].m_State == STATE_ALIVE)
			EliminatePlayer(pPlayer->GetCid());

		m_aPlayers[pPlayer->GetCid()].m_State = STATE_SPECTATING;
	}

	IGameController::DoTeamChange(pPlayer, Team, DoChatMsg);
	if(m_RoundActive)
		SetSkin(pPlayer);
}

int CGameControllerBomb::GetPlayerTeam(int ClientId) const
{
	return m_Teams.m_Core.Team(ClientId);
}

void CGameControllerBomb::SetSkins()
{
	for(auto &pPlayer : GameServer()->m_apPlayers)
		if(pPlayer)
			SetSkin(pPlayer);
}

void CGameControllerBomb::SetSkin(CPlayer *pPlayer)
{
	if(m_aPlayers[pPlayer->GetCid()].m_Bomb)
	{
		str_copy(pPlayer->m_TeeInfos.m_aSkinName, "bomb", sizeof(pPlayer->m_TeeInfos.m_aSkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = 0;

		if(m_aPlayers[pPlayer->GetCid()].m_Tick < (3 * SERVER_TICK_SPEED))
		{
			pPlayer->m_TeeInfos.m_ColorFeet = 16777215; // white
			pPlayer->m_TeeInfos.m_UseCustomColor = 1;
			ColorRGBA Color = ColorRGBA(255 - m_aPlayers[pPlayer->GetCid()].m_Tick, 0, 0);
			pPlayer->m_TeeInfos.m_ColorBody = color_cast<ColorHSLA>(Color).PackAlphaLast(false);
		}
	}
	else if(!((CServer*)Server())->m_aClients[pPlayer->GetCid()].m_KZBot)
	{
		CSkinInfo *pRealSkin = &m_aPlayers[pPlayer->GetCid()].m_RealSkin;
		str_copy(pPlayer->m_TeeInfos.m_aSkinName, pRealSkin->m_aSkinName, sizeof(pPlayer->m_TeeInfos.m_aSkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = pRealSkin->m_UseCustomColor;
		pPlayer->m_TeeInfos.m_ColorBody = pRealSkin->m_aSkinBodyColor;
		pPlayer->m_TeeInfos.m_ColorFeet = pRealSkin->m_aSkinFeetColor;
	}
	else
	{
		str_copy(pPlayer->m_TeeInfos.m_aSkinName, "0_Cyborg Greyfox_KZ", sizeof(pPlayer->m_TeeInfos.m_aSkinName));
		for(int p = 0; p < protocol7::NUM_SKINPARTS; p++)
		{
			pPlayer->m_TeeInfos.m_aUseCustomColors[p] = true;

			if(p==0)
			{
				str_copy(pPlayer->m_TeeInfos.m_apSkinPartNames[p], "fox", sizeof(pPlayer->m_TeeInfos.m_apSkinPartNames[p]));
				pPlayer->m_TeeInfos.m_aSkinPartColors[p] = 1769560;
				continue;
			}

			if(p==1)
			{
				str_copy(pPlayer->m_TeeInfos.m_apSkinPartNames[p], "warpaint", sizeof(pPlayer->m_TeeInfos.m_apSkinPartNames[p]));
				pPlayer->m_TeeInfos.m_aSkinPartColors[p] = 4278190080;
				continue;
			}
			
			if(p==2)
			{
				str_copy(pPlayer->m_TeeInfos.m_apSkinPartNames[p], "hair", sizeof(pPlayer->m_TeeInfos.m_apSkinPartNames[p]));
				continue;
			}

			if(p==5)
			{
				str_copy(pPlayer->m_TeeInfos.m_apSkinPartNames[p], "negative", sizeof(pPlayer->m_TeeInfos.m_apSkinPartNames[p]));
				pPlayer->m_TeeInfos.m_aSkinPartColors[p] = 65408;
				continue;
			}
			

			str_copy(pPlayer->m_TeeInfos.m_apSkinPartNames[p], "standard", sizeof(pPlayer->m_TeeInfos.m_apSkinPartNames[p]));
        }
	}
}

void CGameControllerBomb::MakeRandomBomb(int Count)
{
	int Playing[MAX_CLIENTS];
	int Players = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
		if(m_aPlayers[i].m_State == STATE_ALIVE)
			Playing[Players++] = i;

	std::random_device RandomDevice;
	std::mt19937 g(RandomDevice());
	std::shuffle(Playing, Playing + Players, g);

	for(int i = 0; i < Count; i++)
	{
		MakeBomb(Playing[i], g_Config.m_BombtagSecondsToExplosion * SERVER_TICK_SPEED);
	}
}

void CGameControllerBomb::MakeBomb(int ClientId, int Ticks)
{
	GameServer()->SendBroadcast("", m_aPlayers[ClientId].m_Bomb); // clear previous broadcast
	m_aPlayers[ClientId].m_Bomb = true;
	m_aPlayers[ClientId].m_Tick = Ticks;

	CCharacter *pChr = GameServer()->m_apPlayers[ClientId]->GetCharacter();
	if(pChr)
	{
		if(g_Config.m_BombtagBombWeapon != WEAPON_HAMMER)
			pChr->GiveWeapon(WEAPON_HAMMER, true);

		pChr->GiveWeapon(g_Config.m_BombtagBombWeapon);
		pChr->SetWeapon(g_Config.m_BombtagBombWeapon);
	}

	GameServer()->SendBroadcast("You are the new bomb!\nHit another player before the time runs out!", ClientId);
}

int CGameControllerBomb::GetAutoTeam(int NotThisId)
{
	const int Team = TEAM_RED;
	m_aPlayers[NotThisId].m_State = STATE_ACTIVE;
	char aTeamJoinError[512];
	CanJoinTeam(Team, NotThisId, aTeamJoinError, sizeof(aTeamJoinError));
	return Team;
}

bool CGameControllerBomb::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
{
	if(!m_RoundActive && Team != TEAM_SPECTATORS)
	{
		str_copy(pErrorReason, "", ErrorReasonSize);
		m_aPlayers[NotThisId].m_State = STATE_ACTIVE;
		return true;
	}
	if(Team == TEAM_SPECTATORS)
	{
		m_aPlayers[NotThisId].m_State = STATE_SPECTATING;
		m_aPlayers[NotThisId].m_Bomb = false;
		str_copy(pErrorReason, "You are a spectator now\nYou won't join when a new round begins", ErrorReasonSize);
		return true;
	}
	else
	{
		m_aPlayers[NotThisId].m_State = STATE_ACTIVE;
		str_copy(pErrorReason, "You will join the game when the round is over", ErrorReasonSize);
		return false;
	}
}

void CGameControllerBomb::OnTakeDamage(int Dmg, int From, int To, int Weapon)
{
	if(From == To || From == -1 || To == -1)
		return;

	if(Weapon < 0)
		return;

	if(Weapon != WEAPON_HAMMER && !m_aPlayers[From].m_Bomb)
		return;

	if(m_aPlayers[From].m_Bomb && Weapon != g_Config.m_BombtagBombWeapon)
		return;

	if(m_aPlayers[From].m_Bomb && !m_aPlayers[To].m_Bomb)
	{
		// new bomb
		GameServer()->SendBroadcast("", From);
		m_aPlayers[From].m_Bomb = false;
		MakeBomb(To, m_aPlayers[From].m_Tick);

		CCharacter *pOldBombChr = GameServer()->m_apPlayers[From]->GetCharacter();
		CCharacter *pNewBombChr = GameServer()->m_apPlayers[To]->GetCharacter();

		if(pOldBombChr && pNewBombChr)
		{
			pOldBombChr->GiveWeapon(g_Config.m_BombtagBombWeapon, true);
			pOldBombChr->GiveWeapon(WEAPON_HAMMER, true);
			pOldBombChr->SetWeapon(WEAPON_HAMMER);

			pNewBombChr->GiveWeapon(g_Config.m_BombtagBombWeapon, false);
			if(m_aPlayers[To].m_Tick < g_Config.m_BombtagMinSecondsToExplosion * SERVER_TICK_SPEED && g_Config.m_BombtagMinSecondsToExplosion)
				m_aPlayers[To].m_Tick = g_Config.m_BombtagMinSecondsToExplosion * SERVER_TICK_SPEED;
			pNewBombChr->SetWeapon(g_Config.m_BombtagBombWeapon);
		}
	}
	else if(!m_aPlayers[From].m_Bomb && m_aPlayers[To].m_Bomb && m_aPlayers[To].m_Tick > 0)
	{
		// damage to bomb
		m_aPlayers[To].m_Tick -= g_Config.m_BombtagBombDamage * SERVER_TICK_SPEED;
		UpdateTimer();

		// Increase stats if they killed the player
		if(m_aPlayers[To].m_Tick <= 0)
		{
			m_aPlayers[From].m_HammerKills++;
		}
	}
	else if(!m_aPlayers[From].m_Bomb && !m_aPlayers[To].m_Bomb && g_Config.m_BombtagHammerFreeze)
	{
		CCharacter *pChr = GameServer()->m_apPlayers[To]->GetCharacter();
		if(!pChr)
			return;

		CCharacterCore NewCore = pChr->GetCore();
		NewCore.m_FreezeEnd = Server()->Tick() + g_Config.m_BombtagHammerFreeze;
		NewCore.m_FreezeStart = Server()->Tick();
		pChr->m_FreezeTime = g_Config.m_BombtagHammerFreeze;
		pChr->SetCore(NewCore);
	}
}

void CGameControllerBomb::DoWinCheck()
{
	if(!m_RoundActive)
		return;

	if(AmountOfPlayers(STATE_ALIVE) <= 1)
	{
		EndBombRound(true);
		m_RoundActive = false;
		GameServer()->m_World.m_Paused = true;
		m_GameOverTick = Server()->Tick();
		DoWarmup(3);
		for(auto &aPlayer : m_aPlayers)
		{
			if(aPlayer.m_State == STATE_ALIVE)
			{
				aPlayer.m_State = STATE_ACTIVE;
				aPlayer.m_Bomb = false;
			}
		}
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(AmountOfBombs() == 0)
		{
			if(AmountOfPlayers(STATE_ALIVE) >= 2)
				EndBombRound(false);
			else
				EndBombRound(true);
		}
		if(m_aPlayers[i].m_Bomb)
		{
			if(m_aPlayers[i].m_Tick % SERVER_TICK_SPEED == 0)
				UpdateTimer();
			if(m_aPlayers[i].m_Tick <= 0)
			{
				ExplodeBomb(i);
			}
			m_aPlayers[i].m_Tick--;
		}
	}
	// Move killed players to spectator
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			if(m_aPlayers[i].m_State == STATE_ACTIVE && !m_Warmup)
			{
				if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				{
					GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS, true);
				}
			}
		}
	}
}

int CGameControllerBomb::AmountOfPlayers(int State = STATE_ACTIVE)
{
	int Amount = 0;
	for(auto &aPlayer : m_aPlayers)
		if(aPlayer.m_State == State)
			Amount++;
	return Amount;
}

int CGameControllerBomb::AmountOfBombs()
{
	int Amount = 0;
	for(auto &aPlayer : m_aPlayers)
		if(aPlayer.m_Bomb)
			Amount++;
	return Amount;
}

void CGameControllerBomb::EndBombRound(bool RealEnd)
{
	if(!m_RoundActive)
		return;

	int Alive = 0;
	for(auto &aPlayer : m_aPlayers)
	{
		if(aPlayer.m_State == STATE_ALIVE && !aPlayer.m_Bomb)
		{
			Alive++;
			aPlayer.m_RoundsSurvived++;
		}
	}

	if(!RealEnd)
	{
		const int BombsPerPlayer = g_Config.m_BombtagBombsPerPlayer;
		MakeRandomBomb(std::ceil((Alive / (float)BombsPerPlayer) - (BombsPerPlayer == 1 ? 1 : 0)));
	}
	else
	{
		bool WinnerAnnounced = false;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aPlayers[i].m_State == STATE_ALIVE)
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "'%s' won the round!", Server()->ClientName(i));
				GameServer()->SendChat(-1, TEAM_ALL, aBuf);
				GameServer()->m_apPlayers[i]->m_Score = GameServer()->m_apPlayers[i]->m_Score.value_or(0) + 1;
				auto pPlayer = m_aPlayers[i];
				GameServer()->Score()->SaveStats(Server()->ClientName(i), true, pPlayer.m_HammerKills, pPlayer.m_CollateralKills, pPlayer.m_RoundsSurvived);
				WinnerAnnounced = true;
				break;
			}
		}
		if(!WinnerAnnounced)
		{
			GameServer()->SendChat(-1, TEAM_ALL, "Noone won the round!");
		}

		if(m_WasMysteryRound)
		{
			GameServer()->Console()->ExecuteFile(g_Config.m_SvMysteryRoundsResetFileName);
			m_WasMysteryRound = false;
		}

		std::vector<std::string> vTemp;
		while(g_Config.m_BombtagMysteryChance && rand() % 101 <= g_Config.m_BombtagMysteryChance)
		{
			if(vTemp.size() == Server()->GetMysteryRoundsSize())
			{
				break;
			}
			if(!m_WasMysteryRound)
			{
				GameServer()->Console()->ExecuteFile(g_Config.m_SvMysteryRoundsResetFileName);
				GameServer()->SendChat(-1, TEAM_ALL, "MYSTERY ROUND!");
			}
			const char *pLine = GameServer()->Server()->GetMysteryRoundLine();
			if(pLine)
			{
				if(std::find(vTemp.begin(), vTemp.end(), pLine) != vTemp.end())
				{
					continue;
				}
				GameServer()->Console()->ExecuteLine(pLine);
				m_WasMysteryRound = true;
				vTemp.emplace_back(pLine);
			}
		}

		EndRound();
		DoWarmup(3);
		m_RoundActive = false;
		EndRound();
		DoWarmup(3);
		for(auto &aPlayer : m_aPlayers)
		{
			if(aPlayer.m_State == STATE_ALIVE)
			{
				aPlayer.m_State = STATE_ACTIVE;
				aPlayer.m_Bomb = false;
			}
		}
	}
}

void CGameControllerBomb::ExplodeBomb(int ClientId)
{
	if(!GameServer()->m_apPlayers[ClientId])
		return;

	GameServer()->CreateExplosion(GameServer()->m_apPlayers[ClientId]->m_ViewPos, ClientId, WEAPON_GAME, true, 0);
	GameServer()->CreateSound(GameServer()->m_apPlayers[ClientId]->m_ViewPos, SOUND_GRENADE_EXPLODE);
	m_aPlayers[ClientId].m_State = STATE_ACTIVE;

	// Collateral damage
	for(auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!g_Config.m_BombtagCollateralDamage)
			break;

		if(!pPlayer)
			continue;

		CCharacter *pChr = pPlayer->GetCharacter();
		if(!pChr)
			continue;

		if(ClientId == pPlayer->GetCid())
			continue;

		if(distance(pPlayer->m_ViewPos, GameServer()->m_apPlayers[ClientId]->m_ViewPos) <= 96)
		{
			GameServer()->m_apPlayers[ClientId]->KillCharacter();
			EliminatePlayer(pPlayer->GetCid(), true);
			m_aPlayers[ClientId].m_CollateralKills++;
		}
	}
	GameServer()->m_apPlayers[ClientId]->KillCharacter();
	EliminatePlayer(ClientId);
}

void CGameControllerBomb::EliminatePlayer(int ClientId, bool Collateral)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "'%s' eliminated%s!", Server()->ClientName(ClientId), Collateral ? " by collateral damage" : "");
	GameServer()->SendChat(-1, TEAM_ALL, aBuf);
	auto pPlayer = m_aPlayers[ClientId];
	GameServer()->Score()->SaveStats(Server()->ClientName(ClientId), false, pPlayer.m_HammerKills, pPlayer.m_CollateralKills, pPlayer.m_RoundsSurvived);

	m_aPlayers[ClientId].m_Bomb = false;
	m_aPlayers[ClientId].m_State = STATE_ACTIVE;
}

void CGameControllerBomb::StartBombRound()
{
	int Players = 0;
	m_RoundActive = true;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aPlayers[i].m_State == STATE_ACTIVE)
		{
			GameServer()->m_apPlayers[i]->SetTeam(TEAM_FLOCK, true);
			m_aPlayers[i].m_State = STATE_ALIVE;
			Players++;
		}
	}
	const int BombsPerPlayer = g_Config.m_BombtagBombsPerPlayer;
	MakeRandomBomb(std::ceil((Players / (float)BombsPerPlayer) - (BombsPerPlayer == 1 ? 1 : 0)));
}

void CGameControllerBomb::UpdateTimer()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
		{
			if(m_aPlayers[i].m_Bomb)
			{
				GameServer()->CreateDamageInd(GameServer()->m_apPlayers[i]->m_ViewPos, 0, m_aPlayers[i].m_Tick / SERVER_TICK_SPEED);
				GameServer()->m_apPlayers[i]->GetCharacter()->SetArmor(m_aPlayers[i].m_Tick / SERVER_TICK_SPEED);
			}
		}
	}
}

void CGameControllerBomb::OnSkinChange(const char *pSkin, bool UseCustomColor, int ColorBody, int ColorFeet, int ClientId)
{
	CSkinInfo *pRealSkin = &m_aPlayers[ClientId].m_RealSkin;

	if(!str_find_nocase(pSkin, "bomb"))
		str_copy(pRealSkin->m_aSkinName, pSkin);
	else
		str_copy(pRealSkin->m_aSkinName, "default");

	pRealSkin->m_UseCustomColor = UseCustomColor;
	pRealSkin->m_aSkinBodyColor = ColorBody;
	pRealSkin->m_aSkinFeetColor = ColorFeet;
}

bool CGameControllerBomb::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	IGameController::OnEntity(Index, x, y, Layer, Flags, Initial, Number);

	const vec2 Pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED)
		Team = TEAM_RED;
	if(Index == ENTITY_FLAGSTAND_BLUE)
		Team = TEAM_BLUE;

	//twplus begin +KZ
	if(!(Team == -1 || m_apFlags[Team]))
	{
		CFlag *F = new CFlag(&GameServer()->m_World, Team);
		//F->m_StandPos = Pos;
		F->m_Pos = Pos;
		m_apFlags[Team] = F;
		GameServer()->m_World.InsertEntity(F);
	}
	
	if (Team == TEAM_RED && m_flagstand_temp_i_0 < 10) {
		//m_flagstands_0[m_flagstand_temp_i_0] = Pos;
		m_apFlags[Team]->m_StandPositions[m_flagstand_temp_i_0] = Pos;
		m_flagstand_temp_i_0++;
		m_apFlags[Team]->m_no_stands = m_flagstand_temp_i_0;
	}
	if (Team == TEAM_BLUE && m_flagstand_temp_i_1 < 10) {
		//m_flagstands_1[m_flagstand_temp_i_1] = Pos;
		m_apFlags[Team]->m_StandPositions[m_flagstand_temp_i_1] = Pos;
		m_flagstand_temp_i_1++;
		m_apFlags[Team]->m_no_stands = m_flagstand_temp_i_1;
	}
	if (Team == -1)
	{
		return IGameController::OnEntity(Index, x, y, Layer, Flags, Initial, Number);;
	}

	return true;
}

void CGameControllerBomb::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	int FlagCarrierRed = FLAG_MISSING;
	if(m_apFlags[TEAM_RED])
	{
		if(m_apFlags[TEAM_RED]->m_AtStand)
			FlagCarrierRed = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_RED]->GetCarrier() && m_apFlags[TEAM_RED]->GetCarrier()->GetPlayer())
			FlagCarrierRed = m_apFlags[TEAM_RED]->GetCarrier()->GetPlayer()->GetCid();
		else
			FlagCarrierRed = FLAG_TAKEN;
	}

	int FlagCarrierBlue = FLAG_MISSING;
	if(m_apFlags[TEAM_BLUE])
	{
		if(m_apFlags[TEAM_BLUE]->m_AtStand)
			FlagCarrierBlue = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_BLUE]->GetCarrier() && m_apFlags[TEAM_BLUE]->GetCarrier()->GetPlayer())
			FlagCarrierBlue = m_apFlags[TEAM_BLUE]->GetCarrier()->GetPlayer()->GetCid();
		else
			FlagCarrierBlue = FLAG_TAKEN;
	}

	if(Server()->IsSixup(SnappingClient))
	{
		protocol7::CNetObj_GameDataFlag *pGameDataObj = Server()->SnapNewItem<protocol7::CNetObj_GameDataFlag>(0);
		if(!pGameDataObj)
			return;

		pGameDataObj->m_FlagCarrierRed = FlagCarrierRed;
		pGameDataObj->m_FlagCarrierBlue = FlagCarrierBlue;
	}
	else
	{
		CNetObj_GameData *pGameDataObj = Server()->SnapNewItem<CNetObj_GameData>(0);
		if(!pGameDataObj)
			return;

		pGameDataObj->m_FlagCarrierRed = FlagCarrierRed;
		pGameDataObj->m_FlagCarrierBlue = FlagCarrierBlue;

		pGameDataObj->m_TeamscoreRed = 0;
		pGameDataObj->m_TeamscoreBlue = 0;
	}
}

void CGameControllerBomb::FlagTick()
{
	if(GameServer()->m_World.m_ResetRequested || GameServer()->m_World.m_Paused)
		return;

	for(int FlagColor = 0; FlagColor < 2; FlagColor++)
	{
		CFlag *pFlag = m_apFlags[FlagColor];

		if(!pFlag)
			continue;

		//
		if(pFlag->GetCarrier())
		{
			// forbid holding flags in ddrace teams
			if(!g_Config.m_SvSoloServer && GameServer()->GetDDRaceTeam(pFlag->GetCarrier()->GetPlayer()->GetCid()))
			{
				GameServer()->CreateSoundGlobal(SOUND_CTF_DROP);
				GameServer()->SendGameMsg(protocol7::GAMEMSG_CTF_DROP, -1);
				pFlag->Drop();
				continue;
			}

		}
		else
		{
			CCharacter *apCloseCCharacters[MAX_CLIENTS];
			int Num = GameServer()->m_World.FindEntities(pFlag->GetPos(), CFlag::ms_PhysSize, (CEntity **)apCloseCCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
			for(int i = 0; i < Num; i++)
			{
				if(!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_SPECTATORS || GameServer()->Collision()->IntersectLine(pFlag->GetPos(), apCloseCCharacters[i]->GetPos(), NULL, NULL))
					continue;

				// only allow flag grabs in team 0
				if(!g_Config.m_SvSoloServer && GameServer()->GetDDRaceTeam(apCloseCCharacters[i]->GetPlayer()->GetCid()))
					continue;

				if(pFlag->GetOtherFlag() && pFlag->GetOtherFlag()->m_pCarrier == apCloseCCharacters[i]) //+KZ dont grab flag if he is already with a flag
					continue;

				// cooldown for recollect after dropping the flag
				if(pFlag->m_pLastCarrier == apCloseCCharacters[i] && (pFlag->m_DropTick + Server()->TickSpeed()) > Server()->Tick())
					continue;

				{
					// take the flag

					pFlag->Grab(apCloseCCharacters[i]);

					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "flag_grab player='%d:%s' team=%d",
						pFlag->GetCarrier()->GetPlayer()->GetCid(),
						Server()->ClientName(pFlag->GetCarrier()->GetPlayer()->GetCid()),
						pFlag->GetCarrier()->GetPlayer()->GetTeam());
					GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
					GameServer()->SendGameMsg(protocol7::GAMEMSG_CTF_GRAB, FlagColor, -1);
					break;
				}
			}
		}
	}
}

bool CGameControllerBomb::OnEntityKZ(int Index, int x, int y, int Layer, int Flags, bool Initial, unsigned char Number, int64_t Value1, int64_t Value2, int64_t Value3)
{
	int PickupType = -1;
	int PickupSubtype = -1;

	int aSides[8] = {0,0,0,0,0,0,0,0};
	if(GameServer()->Collision()->DDNetLayerExists(Layer))
	{
		aSides[0] = GameServer()->Collision()->Entity(x, y + 1, Layer);
		aSides[1] = GameServer()->Collision()->Entity(x + 1, y + 1, Layer);
		aSides[2] = GameServer()->Collision()->Entity(x + 1, y, Layer);
		aSides[3] = GameServer()->Collision()->Entity(x + 1, y - 1, Layer);
		aSides[4] = GameServer()->Collision()->Entity(x, y - 1, Layer);
		aSides[5] = GameServer()->Collision()->Entity(x - 1, y - 1, Layer);
		aSides[6] = GameServer()->Collision()->Entity(x - 1, y, Layer);
		aSides[7] = GameServer()->Collision()->Entity(x - 1, y + 1, Layer);
	}

	if(Index == KZ_TILE_PORTAL_GUN)
	{
		PickupType = POWERUP_WEAPON;
		PickupSubtype = KZ_CUSTOM_WEAPON_PORTAL_GUN;
	}

	const vec2 Pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);

	if(Index == KZ_TILE_TURRET)
	{
		new CKZGun(&GameServer()->m_World,Pos,true,false,Layer,Number);
		m_ShowHealth = true;
		return true;
	}

	if(Index == KZ_TILE_TURRET_EXPLOSIVE)
	{
		new CKZGun(&GameServer()->m_World,Pos,true,true,Layer,Number);
		m_ShowHealth = true;
		return true;
	}

	if(Index == KZ_TILE_DAMAGE_LASER)
	{
		int aSides2[8] = {0,0,0,0,0,0,0,0};
		if(GameServer()->Collision()->DDNetLayerExists(Layer))
		{
			aSides2[0] = GameServer()->Collision()->Entity(x, y + 2, Layer);
			aSides2[1] = GameServer()->Collision()->Entity(x + 2, y + 2, Layer);
			aSides2[2] = GameServer()->Collision()->Entity(x + 2, y, Layer);
			aSides2[3] = GameServer()->Collision()->Entity(x + 2, y - 2, Layer);
			aSides2[4] = GameServer()->Collision()->Entity(x, y - 2, Layer);
			aSides2[5] = GameServer()->Collision()->Entity(x - 2, y - 2, Layer);
			aSides2[6] = GameServer()->Collision()->Entity(x - 2, y, Layer);
			aSides2[7] = GameServer()->Collision()->Entity(x - 2, y + 2, Layer);
		}

		float AngularSpeed = 0.0f;
		
		AngularSpeed = (pi / 360) * Value1;

		for(int i = 0; i < 8; i++)
		{
			if(aSides[i] >= ENTITY_LASER_SHORT && aSides[i] <= ENTITY_LASER_LONG)
			{
				CKZLight *pLight = new CKZLight(&GameServer()->m_World, Pos, pi / 4 * i, 32 * 3 + 32 * (aSides[i] - ENTITY_LASER_SHORT) * 3, Layer, Number);
				pLight->m_AngularSpeed = AngularSpeed;
				m_ShowHealth = true;
				if(aSides2[i] >= ENTITY_LASER_C_SLOW && aSides2[i] <= ENTITY_LASER_C_FAST)
				{
					pLight->m_Speed = 1 + (aSides2[i] - ENTITY_LASER_C_SLOW) * 2;
					pLight->m_CurveLength = pLight->m_Length;
				}
				else if(aSides2[i] >= ENTITY_LASER_O_SLOW && aSides2[i] <= ENTITY_LASER_O_FAST)
				{
					pLight->m_Speed = 1 + (aSides2[i] - ENTITY_LASER_O_SLOW) * 2;
					pLight->m_CurveLength = 0;
				}
				else
					pLight->m_CurveLength = pLight->m_Length;
			}
		}
	}

	if(Index == KZ_TILE_MINE)
	{
		new CMine(&GameServer()->m_World, Pos, Number);
		m_ShowHealth = true;
	}

	if(Index == KZ_TILE_DAMAGE_ZONE || Index == KZ_TILE_HEALTH_ZONE)
	{
		m_ShowHealth = true;
	}

	if(PickupType != -1)
	{
		if(PickupSubtype != -1)
		{
			switch (PickupSubtype)
			{
				case KZ_CUSTOM_WEAPON_PORTAL_GUN:
				{
					CKZPickup *pPickup = new CKZPickup(&GameServer()->m_World, PickupType, PickupSubtype, Layer, (int)Number, Flags);
					pPickup->m_Pos = Pos;
					return true;
				}
				break;
			

			}
		}
	}

	return false;
}
