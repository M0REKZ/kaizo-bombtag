// Copyright (C) Benjamín Gajardo (also known as +KZ)
//
// RenderKaizoPickup() has some code from items.cpp

#include <generated/client_data.h>
#include <generated/protocol.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include "items.h"

void CItems::OnInitKZ()
{
	Graphics()->QuadsSetSubset(0, 0, 1, 1);
	m_CrownOffset = Graphics()->QuadContainerAddSprite(m_ItemsQuadContainerIndex, -21.f, -21.f, 42.f, 42.f);

	Graphics()->QuadsSetSubset(0, 0, 1, 1);
	m_TurretOffset_1 = Graphics()->QuadContainerAddSprite(m_ItemsQuadContainerIndex, -16.f, -16.f, 32.f, 32.f);

	Graphics()->QuadsSetSubset(0, 0, 1, 1);
	m_TurretOffset_2 = Graphics()->QuadContainerAddSprite(m_ItemsQuadContainerIndex, -16.f, -16.f, 32.f, 32.f);

	Graphics()->QuadsSetSubset(0, 0, 1, 1);
	m_MineOffset = Graphics()->QuadContainerAddSprite(m_ItemsQuadContainerIndex, -16.f, -16.f, 32.f, 32.f);

	//TODO: maybe find a way to deduplicate the code below with the one in players_kz.cpp

	float ScaleX, ScaleY;
    Graphics()->GetSpriteScale(&g_pData->m_aSprites[SPRITE_KZ_PORTAL], ScaleX, ScaleY);
    Graphics()->QuadsSetSubset(0, 0, 1, 1);
	m_KaizoWeaponsOffsets[KZ_CUSTOM_WEAPON_PORTAL_GUN - KZ_CUSTOM_WEAPONS_START] = Graphics()->QuadContainerAddSprite(m_ItemsQuadContainerIndex, 92 * ScaleX, 92 * ScaleY);
    Graphics()->GetSpriteScale(&g_pData->m_aSprites[SPRITE_KZ_ATTRACTOR], ScaleX, ScaleY);
    Graphics()->QuadsSetSubset(0, 0, 1, 1);
	m_KaizoWeaponsOffsets[KZ_CUSTOM_WEAPON_ATTRACTOR_BEAM - KZ_CUSTOM_WEAPONS_START] = Graphics()->QuadContainerAddSprite(m_ItemsQuadContainerIndex, 92 * ScaleX, 92 * ScaleY);
}

void CItems::RenderCrown()
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!GameClient()->m_Snap.m_aCharacters[i].m_Active || GameClient()->m_aClients[i].m_Team == TEAM_SPECTATORS)
			continue;

		if(GameClient()->m_aClients[i].m_CrownTick != -1 && GameClient()->m_aClients[i].m_CrownTick + Client()->GameTickSpeed() > Client()->GameTick(g_Config.m_ClDummy))
		{
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_KZ_CROWN].m_Id);
			Graphics()->QuadsSetRotation(0);
			Graphics()->SetColor(1.f, 1.f, 1.f, 1.f);
			Graphics()->RenderQuadContainerAsSprite(m_ItemsQuadContainerIndex, m_CrownOffset, GameClient()->m_aClients[i].m_RenderPos.x, GameClient()->m_aClients[i].m_RenderPos.y - 64.0f, 1.0f, 1.0f);
		}
	}
}

void CItems::HandleKaizoSnapItem(const IClient::CSnapItem &Item, bool Front)
{
	if(Front)
	{
		//TODO
	}
	else
	{
		if(Item.m_Type == NETOBJTYPE_KAIZONETWORKTURRET)
		{
			RenderTurret((CNetObj_KaizoNetworkTurret *)Item.m_pData);
		}
		else if(Item.m_Type == NETOBJTYPE_KAIZONETWORKMINE)
		{
			RenderMine((CNetObj_KaizoNetworkMine *)Item.m_pData);
		}
		else if(Item.m_Type == NETOBJTYPE_KAIZONETWORKPICKUP)
		{
			RenderKaizoPickup((CNetObj_KaizoNetworkPickup *)Item.m_pData);
		}
	}
}

void CItems::RenderTurret(CNetObj_KaizoNetworkTurret *pTurret)
{
    if(!pTurret)
		return;

	int Type = pTurret->m_Type;
	int Offset = 0;

	if(Type == 0)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_KZ_TURRET_1].m_Id);
		Offset = m_TurretOffset_1;
	}
	else if(Type == 1)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_KZ_TURRET_2].m_Id);
		Offset = m_TurretOffset_2;
	}
	else
	{	//Fallback to 1
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_KZ_TURRET_1].m_Id);
		Offset = m_TurretOffset_1;
	}

	Graphics()->QuadsSetRotation(Client()->GameTick(g_Config.m_ClDummy) * pi * 2 / 3);
	Graphics()->SetColor(1.f, 1.f, 1.f, 1.f);
	Graphics()->RenderQuadContainerAsSprite(m_ItemsQuadContainerIndex, Offset, pTurret->m_X, pTurret->m_Y, 1.0f, 1.0f);
}

void CItems::RenderMine(CNetObj_KaizoNetworkMine *pMine)
{
	if(!pMine)
		return;

	int Tick = Client()->GameTick(g_Config.m_ClDummy);

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_KZ_MINE].m_Id);
	Graphics()->QuadsSetRotation(Tick%360 * pi/180);
	Graphics()->SetColor(1.f, 1.f, 1.f, 1.f);
	Graphics()->RenderQuadContainerAsSprite(m_ItemsQuadContainerIndex, m_MineOffset, pMine->m_X, pMine->m_Y + 8 * sin((float)Tick / 25.0f), 1.0f, 1.0f);
}

void CItems::RenderKaizoPickup(CNetObj_KaizoNetworkPickup *pPickup)
{
	if(!pPickup)
		return;

	if(pPickup->m_Type < KZ_CUSTOM_WEAPONS_START || pPickup->m_Type >= KZ_NUM_CUSTOM_WEAPONS)
		return;

	int SwitcherTeam = GameClient()->SwitchStateTeam();
	auto &aSwitchers = GameClient()->Switchers();
	int Tick = Client()->GameTick(g_Config.m_ClDummy);
	
	switch(pPickup->m_Type)
	{
		case KZ_CUSTOM_WEAPON_PORTAL_GUN:
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_KZ_PORTAL].m_Id);
			break;
		case KZ_CUSTOM_WEAPON_ATTRACTOR_BEAM:
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_KZ_ATTRACTOR].m_Id);
			break;
	}
	Graphics()->QuadsSetRotation(0);

	if(pPickup->m_Switch > 0 && pPickup->m_Switch < (int)aSwitchers.size() && !aSwitchers[pPickup->m_Switch].m_aStatus[SwitcherTeam])
		Graphics()->SetColor(1.f, 1.f, 1.f, 0.3f);
	else
		Graphics()->SetColor(1.f, 1.f, 1.f, 1.f);
	Graphics()->RenderQuadContainerAsSprite(m_ItemsQuadContainerIndex, m_KaizoWeaponsOffsets[pPickup->m_Type - KZ_CUSTOM_WEAPONS_START], pPickup->m_X, pPickup->m_Y + 8 * sin((float)Tick / 25.0f), 1.0f, 1.0f);
}
