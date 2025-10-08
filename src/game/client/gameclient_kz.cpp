// Copyright (C) Benjamín Gajardo (also known as +KZ)
//
// CheckNewInput() is from Fast Input commit

#include "gameclient.h"
#include <game/client/prediction/entities/character.h>

void CGameClient::OnKaizoConnected()
{
    m_Collision.m_pWorldCore = &m_GameWorld.m_Core;
    m_Collision.m_pTeamsCore = m_GameWorld.Teams();
}

void CGameClient::UpdateKaizoPrediction()
{
    m_GameWorld.m_Core.m_WorldTickKZ = m_GameWorld.GameTick();
	m_Collision.SetTime(static_cast<double>(m_GameWorld.GameTick() - m_LastRoundStartTick) / m_GameWorld.GameTickSpeed());
	if(g_Config.m_SvGoresQuadsEnable)
		m_Collision.UpdateQuadCache();
}

void CGameClient::HandleKaizoMessage(int MsgId, CUnpacker *pUnpacker, int Conn, bool Dummy, void *pRawMsg)
{
    if (MsgId == NETMSGTYPE_SV_KAIZONETWORKCROWN)
	{
		CNetMsg_Sv_KaizoNetworkCrown *pMsg = (CNetMsg_Sv_KaizoNetworkCrown *)pRawMsg;
		int CrownId = pMsg->m_ClientId;
		if(CrownId >= 0 && CrownId < MAX_CLIENTS)
		{
			m_aClients[CrownId].m_CrownTick = m_GameWorld.GameTick();
		}
	}
}

void CGameClient::HandleKaizoSnapItem(const IClient::CSnapItem *pItem)
{
    if(!pItem)
        return;

    if(pItem->m_Type == NETOBJTYPE_KAIZONETWORKCHARACTER)
    {
        const CNetObj_KaizoNetworkCharacter *pKaizoChar = (const CNetObj_KaizoNetworkCharacter *)pItem->m_pData;
        if(!pKaizoChar)
            return;

        int ClientId = pItem->m_Id;
        if(ClientId < 0 || ClientId >= MAX_CLIENTS)
            return;

        m_aClients[ClientId].m_KaizoCharTick = pKaizoChar->m_Tick;
        m_aClients[ClientId].m_CharFlags = pKaizoChar->m_Flags;
        m_aClients[ClientId].m_KaizoCustomWeapon = pKaizoChar->m_RealCurrentWeapon;

        CCharacter *pChar = m_GameWorld.GetCharacterById(ClientId);

        if(pChar)
            pChar->m_KaizoNetworkChar = *pKaizoChar;
    }
}

bool CGameClient::IsKaizoCharUpdated(int ClientId)
{
    if(m_aClients[ClientId].m_KaizoCharTick >= 0 && m_aClients[ClientId].m_KaizoCharTick > m_GameWorld.GameTick() - m_GameWorld.GameTickSpeed()/5)
        return true;
	return false;
}

void CGameClient::CClientData::KaizoReset()
{
    m_CrownTick = -1;
    m_KaizoCharTick = -1;
    m_CharFlags = 0;
    m_KaizoCustomWeapon = -1;
}

bool CGameClient::CheckNewInput() 
{
	return m_Controls.CheckNewInput();
}

void CGameClient::GetKaizoInfo(CServerInfo *pServerInfo)
{
	Client()->GetServerInfo(pServerInfo);
    m_InstaShield = pServerInfo->m_aGameType[0] == 'i' && pServerInfo->m_aGameType[str_length(pServerInfo->m_aGameType) - 1] == ')';
}

void CGameClient::KaizoReset()
{
    m_InstaShield = false;
}
