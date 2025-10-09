// Copyright (C) Benjamín Gajardo (also known as +KZ)

// CNamePlatePartKaizoPlayerPing has code from nameplates.cpp

#include <game/client/gameclient.h>

class CNamePlatePartKaizoPlayerPing : public CNamePlatePart
{
protected:
	ColorRGBA m_Color = ColorRGBA(1.0f, 1.0f, 1.0f, 0.0f);
	void Update(CGameClient &This, const CNamePlateData &Data) override
	{
		if(!g_Config.m_KaizoPingCircles || Data.m_ClientId == This.m_Snap.m_LocalClientId)
		{
			m_Visible = false;
			return;
		}

		if(This.m_aClients[Data.m_ClientId].m_ReceivedPing < 0)
		{
			m_Visible = false;
			return;
		}
		
		m_Visible = true;

		m_Size = vec2(20 + DEFAULT_PADDING, 20 + DEFAULT_PADDING);
		m_Color = ColorRGBA(This.m_aClients[Data.m_ClientId].m_ReceivedPing / 1000.f, 1.0f - This.m_aClients[Data.m_ClientId].m_ReceivedPing / 1000.f, 0.0f, Data.m_Color.a);
	}

public:
	void Render(CGameClient &This, vec2 Pos) const override
	{
		IGraphics::CQuadItem QuadItem(Pos.x - Size().x / 2.0f, Pos.y - Size().y / 2.0f, Size().x, Size().y);
		This.Graphics()->TextureClear();
		This.Graphics()->QuadsBegin();
		This.Graphics()->SetColor(m_Color);
		This.Graphics()->DrawCircle(Pos.x,Pos.y, Size().x/2, 32);
		This.Graphics()->QuadsEnd();
	}

	CNamePlatePartKaizoPlayerPing(CGameClient &This) :
		CNamePlatePart(This) {}
};