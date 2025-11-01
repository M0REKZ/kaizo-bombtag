// Copyright (C) Benjamín Gajardo (also known as +KZ)

#include "save.h"

#include <cstdio>

#include "entities/character.h"
#include "gamemodes/DDRace.h"
#include "player.h"
#include "teams.h"
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>

void CSaveTee::SaveKZ(CCharacter *pchr, bool AddPenalty)
{
    m_Health = pchr->m_Health;
    m_CustomWeapon = pchr->m_KaizoNetworkChar.m_RealCurrentWeapon;
	m_BluePortal = pchr->m_BluePortal;
    m_TuneZoneOverrideKZ = pchr->m_TuneZoneOverrideKZ;
    m_NODAMAGE = pchr->m_NODAMAGE;
    m_HasRecoverJumpLaser = pchr->m_HasRecoverJumpLaser;

    for(int i = 0; i < KZ_NUM_CUSTOM_WEAPONS; i++)
    {
	    m_aCustomWeapons[i].m_Ammo = pchr->m_aCustomWeapons[i].m_Ammo;
        m_aCustomWeapons[i].m_Got = pchr->m_aCustomWeapons[i].m_Got;
    }

    m_SavedSubtick.Keep(pchr);
}

bool CSaveTee::LoadKZ(CCharacter *pchr, std::optional<int> Team)
{
    pchr->m_Health = m_Health;
    pchr->m_KaizoNetworkChar.m_RealCurrentWeapon = m_CustomWeapon;
	pchr->m_BluePortal = m_BluePortal;
    pchr->m_TuneZoneOverrideKZ = m_TuneZoneOverrideKZ;
    pchr->m_NODAMAGE = m_NODAMAGE;
    pchr->m_HasRecoverJumpLaser = m_HasRecoverJumpLaser;

    for(int i = 0; i < KZ_NUM_CUSTOM_WEAPONS; i++)
    {
	    pchr->m_aCustomWeapons[i].m_Ammo = m_aCustomWeapons[i].m_Ammo;
        pchr->m_aCustomWeapons[i].m_Got = m_aCustomWeapons[i].m_Got;
    }

    m_SavedSubtick.Restore(pchr);

	return true;
}

char *CSaveTeam::GetKaizoString()
{
	str_format(m_aString, sizeof(m_aString), "%d\t%d\t%d\t%d\t%d", static_cast<int>(m_TeamState), m_MembersCount, m_HighestSwitchNumber, m_TeamLocked, m_Practice);

	for(int i = 0; i < m_MembersCount; i++)
	{
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "\n%s", m_pSavedTees[i].GetKaizoString(this));
		str_append(m_aString, aBuf);
	}

	return m_aString;
}

char *CSaveTee::GetKaizoString(const CSaveTeam *pTeam)
{
	int HookedPlayer = -1;
	if(m_HookedPlayer != -1)
	{
		for(int n = 0; n < pTeam->GetMembersCount(); n++)
		{
			if(m_HookedPlayer == pTeam->m_pSavedTees[n].GetClientId())
			{
				HookedPlayer = n;
				break;
			}
		}
	}

	str_format(m_aString, sizeof(m_aString),
		"%d\t" //Health
        "%d\t" //Custom Weapon
        "%d\t" //Blue Portal
        "%d\t" //Tune Zone override
        "%d\t" //NODAMAGE
        "%d\t" //RecoverJumpLaser
        "%d\t%d\t" //Portalgun Ammo, Got
        "%d\t%d\t" //Attractor beam ammo, got

        //subtick
        "%d\t%d\t%d\t" //Startsubtick, startedtick, startdivisor
        "%d\t%d\t%d\t" //Finishsubtick, finishtick, finishdivisor

        ,

        m_Health,
        m_CustomWeapon,
        m_BluePortal,
        m_TuneZoneOverrideKZ,
        m_NODAMAGE,
        m_HasRecoverJumpLaser,

        m_aCustomWeapons[KZ_CUSTOM_WEAPON_PORTAL_GUN - KZ_CUSTOM_WEAPONS_START].m_Ammo, m_aCustomWeapons[KZ_CUSTOM_WEAPON_PORTAL_GUN - KZ_CUSTOM_WEAPONS_START].m_Got,
        m_aCustomWeapons[KZ_CUSTOM_WEAPON_ATTRACTOR_BEAM - KZ_CUSTOM_WEAPONS_START].m_Ammo, m_aCustomWeapons[KZ_CUSTOM_WEAPON_ATTRACTOR_BEAM - KZ_CUSTOM_WEAPONS_START].m_Got,

        m_SavedSubtick.m_StartSubTick, m_SavedSubtick.m_StartedTickKZ, m_SavedSubtick.m_StartDivisor,
        m_SavedSubtick.m_FinishSubTick, m_SavedSubtick.m_FinishedTickKZ, m_SavedSubtick.m_FinishDivisor
        );
	return m_aString;
}

int CSaveTeam::FromKaizoString(const char *pString)
{
	char aTeamStats[MAX_CLIENTS];
	char aSwitcher[64];
	char aSaveTee[1024];

	char *pCopyPos;
	unsigned int Pos = 0;
	unsigned int LastPos = 0;
	unsigned int StrSize;

	str_copy(m_aString, pString, sizeof(m_aString));

	while(m_aString[Pos] != '\n' && Pos < sizeof(m_aString) && m_aString[Pos]) // find next \n or \0
		Pos++;

	pCopyPos = m_aString + LastPos;
	StrSize = Pos - LastPos + 1;
	if(m_aString[Pos] == '\n')
	{
		Pos++; // skip \n
		LastPos = Pos;
	}

	if(StrSize <= 0)
	{
		dbg_msg("load", "savegame: wrong format (couldn't load teamstats)");
		return 1;
	}

	if(StrSize < sizeof(aTeamStats))
	{
		str_copy(aTeamStats, pCopyPos, StrSize);
		int TeamState;
		int Num = sscanf(aTeamStats, "%d\t%d\t%d\t%d\t%d", &TeamState, &m_MembersCount, &m_HighestSwitchNumber, &m_TeamLocked, &m_Practice);
		m_TeamState = static_cast<ETeamState>(TeamState);
		switch(Num) // Don't forget to update this when you save / load more / less.
		{
		case 4:
			m_Practice = false;
			[[fallthrough]];
		case 5:
			break;
		default:
			dbg_msg("load", "failed to load teamstats");
			dbg_msg("load", "loaded %d vars", Num);
			return Num + 1; // never 0 here
		}
	}
	else
	{
		dbg_msg("load", "savegame: wrong format (couldn't load teamstats, too big)");
		return 1;
	}

	//if(m_pSavedTees)
	//{
	//	delete[] m_pSavedTees;
	//	m_pSavedTees = nullptr;
	//}

	if(m_MembersCount > 64)
	{
		dbg_msg("load", "savegame: team has too many players");
		return 1;
	}
	//else if(m_MembersCount)
	//{
	//	m_pSavedTees = new CSaveTee[m_MembersCount];
	//}

	for(int n = 0; n < m_MembersCount; n++)
	{
		while(m_aString[Pos] != '\n' && Pos < sizeof(m_aString) && m_aString[Pos]) // find next \n or \0
			Pos++;

		pCopyPos = m_aString + LastPos;
		StrSize = Pos - LastPos + 1;
		if(m_aString[Pos] == '\n')
		{
			Pos++; // skip \n
			LastPos = Pos;
		}

		if(StrSize <= 0)
		{
			dbg_msg("load", "savegame: wrong format (couldn't load tee)");
			return 1;
		}

		if(StrSize < sizeof(aSaveTee))
		{
			str_copy(aSaveTee, pCopyPos, StrSize);
			int Num = m_pSavedTees[n].FromKaizoString(aSaveTee);
			if(Num)
			{
				dbg_msg("load", "failed to load tee");
				dbg_msg("load", "loaded %d vars", Num - 1);
				return 1;
			}
		}
		else
		{
			dbg_msg("load", "savegame: wrong format (couldn't load tee, too big)");
			return 1;
		}
	}

	return 0;
}

int CSaveTee::FromKaizoString(const char *pString)
{
	int Num;
	int tempBluePortal = 0;
	int tempNODAMAGE = 0;
	int tempHasRecoverJumpLaser = 0;

	int tempGotCustomWeapon[KZ_NUM_CUSTOM_WEAPONS] = {0};

	Num = sscanf(pString,
		"%d\t" //Health
        "%d\t" //Custom Weapon
        "%d\t" //Blue Portal
        "%d\t" //Tune Zone override
        "%d\t" //NODAMAGE
        "%d\t" //RecoverJumpLaser
        "%d\t%d\t" //Portalgun Ammo, Got
        "%d\t%d\t" //Attractor beam ammo, got

        //subtick
        "%d\t%d\t%d\t" //Startsubtick, startedtick, startdivisor
        "%d\t%d\t%d\t" //Finishsubtick, finishtick, finishdivisor

        ,

        &m_Health,
        &m_CustomWeapon,
        &tempBluePortal,
        &m_TuneZoneOverrideKZ,
        &tempNODAMAGE,
        &tempHasRecoverJumpLaser,

        &m_aCustomWeapons[KZ_CUSTOM_WEAPON_PORTAL_GUN - KZ_CUSTOM_WEAPONS_START].m_Ammo, &tempGotCustomWeapon[KZ_CUSTOM_WEAPON_PORTAL_GUN - KZ_CUSTOM_WEAPONS_START],
        &m_aCustomWeapons[KZ_CUSTOM_WEAPON_ATTRACTOR_BEAM - KZ_CUSTOM_WEAPONS_START].m_Ammo, &tempGotCustomWeapon[KZ_CUSTOM_WEAPON_ATTRACTOR_BEAM - KZ_CUSTOM_WEAPONS_START],

        &m_SavedSubtick.m_StartSubTick, &m_SavedSubtick.m_StartedTickKZ, &m_SavedSubtick.m_StartDivisor,
        &m_SavedSubtick.m_FinishSubTick, &m_SavedSubtick.m_FinishedTickKZ, &m_SavedSubtick.m_FinishDivisor
        );

	m_BluePortal = tempBluePortal;
	m_NODAMAGE = tempNODAMAGE;
	m_HasRecoverJumpLaser = tempHasRecoverJumpLaser;

	for(int i = 0; i < KZ_NUM_CUSTOM_WEAPONS; i++)
	{
		m_aCustomWeapons[i].m_Got = tempGotCustomWeapon[i];
	}

	return 0; //evil
}