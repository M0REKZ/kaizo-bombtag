// Copyright (C) Benjamín Gajardo (also known as +KZ)
//
// GetClientInfclassVersion() taken from InfClass
// GetKaizoNetworkVersion() based on GetClientInfclassVersion()

#include "server.h"
#include <game/version.h>

int CServer::GetClientInfclassVersion(int ClientId)
{
	if(ClientId == SERVER_DEMO_CLIENT)
	{
		return 1000;
	}

	if(m_aClients[ClientId].m_State == CClient::STATE_INGAME)
	{
		return m_aClients[ClientId].m_InfClassVersion;
	}

	return 0;
}

int CServer::GetKaizoNetworkVersion(int ClientId)
{
    if(ClientId == SERVER_DEMO_CLIENT)
    {
        return KAIZO_NETWORK_VERSION_LATEST;
    }

    if(m_aClients[ClientId].m_State == CClient::STATE_INGAME)
    {
        return m_aClients[ClientId].m_KaizoNetworkVersion;
    }

	return 0;
}