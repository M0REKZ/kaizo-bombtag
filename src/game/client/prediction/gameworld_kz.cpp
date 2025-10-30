// Copyright (C) Benjamín Gajardo (also known as +KZ)

// OnGameTile() has code from Pointer's TW+

#include <game/collision.h>
#include "gameworld.h"

void CGameWorld::OnCopyWorld(CGameWorld *pFrom)
{
	for(int i = 0; i < 4; i++)
	{
		m_PointerTelePositions[i].m_X = pFrom->m_PointerTelePositions[i].m_X;
		m_PointerTelePositions[i].m_Y = pFrom->m_PointerTelePositions[i].m_Y;
		m_PointerTelePositions[i].m_Exists = pFrom->m_PointerTelePositions[i].m_Exists;
	}
}

void CGameWorld::OnGameTile(int X, int Y, const CTile *pTile)
{
    if(!pTile)
        return;

    int Index = pTile->m_Index;

	if(Index > 128)
		return;

	switch(Index)
	{
	case POINTER_TILE_TELEONE:
		if(!m_PointerTelePositions[0].m_Exists)
		{
			m_PointerTelePositions[0].m_X = X;
			m_PointerTelePositions[0].m_Y = Y;
			m_PointerTelePositions[0].m_Exists = true;
		}
		break;
	case POINTER_TILE_TELETWO:
		if(!m_PointerTelePositions[1].m_Exists)
		{
			m_PointerTelePositions[1].m_X = X;
			m_PointerTelePositions[1].m_Y = Y;
			m_PointerTelePositions[1].m_Exists = true;
		}
		break;
	case POINTER_TILE_TELETHREE:
		if(!m_PointerTelePositions[2].m_Exists)
		{
			m_PointerTelePositions[2].m_X = X;
			m_PointerTelePositions[2].m_Y = Y;
			m_PointerTelePositions[2].m_Exists = true;
		}
		break;
	case POINTER_TILE_TELEFOUR:
		if(!m_PointerTelePositions[3].m_Exists)
		{
			m_PointerTelePositions[3].m_X = X;
			m_PointerTelePositions[3].m_Y = Y;
			m_PointerTelePositions[3].m_Exists = true;
		}
		break;
	}
}

void CGameWorld::OnConnected()
{
	for(int i = 0; i < 4; i++)
	{
		m_PointerTelePositions[i].m_X = 0;
		m_PointerTelePositions[i].m_Y = 0;
		m_PointerTelePositions[i].m_Exists = false;
	}

	const CTile *pTiles = m_pCollision->GameLayer();
	for(int y = 0; y < m_pCollision->GetHeight(); y++)
	{
		for(int x = 0; x < m_pCollision->GetWidth(); x++)
		{
			OnGameTile(x, y, &pTiles[y * m_pCollision->GetWidth() + x]);
		}
	}
}
