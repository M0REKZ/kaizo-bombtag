// Copyright (C) Benjamín Gajardo (also known as +KZ)

// KaizoAntibotTick has some code from FoxNet

#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/version.h>
#include "player.h"

void CPlayer::OnKaizoSnap(int SnappingClient, int Id)
{
    if(Server()->GetKaizoNetworkVersion(SnappingClient) >= KAIZO_NETWORK_VERSION_PLAYER_PING && m_LastAckedTick != -1)
	{
		CNetObj_KaizoNetworkPlayerPing *pKaizoPlayerPing = Server()->SnapNewItem<CNetObj_KaizoNetworkPlayerPing>(Id);
		if(!pKaizoPlayerPing)
			return;

		int diff = Server()->Tick() - m_LastAckedTick;

		if(diff > 50)
			diff = 50;

		pKaizoPlayerPing->m_Ping = (int)(diff * 1000/Server()->TickSpeed());
	}
}

void CPlayer::OnKaizoTick()
{
	if(g_Config.m_SvKaizoAntibot)
		KaizoAntibotTick();
}

void CPlayer::KaizoAntibotTick()
{
	if(Server()->ClientSlotEmpty(m_ClientId))
		return;

	IServer::CClientInfo Info;
	if(!Server()->GetClientInfo(m_ClientId, &Info))
		return;

	if(Info.m_pDDNetVersionStr && str_find(Info.m_pDDNetVersionStr, "imacrack")) // free version of a bot client sends this.
	{
		Server()->Ban(m_ClientId, 240 * 60, "Cheat client detected! Download the official DDNet client from ddnet.org", false);
		return;
	}
}
