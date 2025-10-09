// Copyright (C) Benjamín Gajardo (also known as +KZ)

#include <engine/server.h>
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