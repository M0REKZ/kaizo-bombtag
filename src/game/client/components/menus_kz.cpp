// Copyright (C) Benjamín Gajardo (also known as +KZ)
//
// 

#include <game/localization.h>
#include "menus.h"

void CMenus::RenderSettingsKaizo(CUIRect MainView)
{
	CUIRect Button;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_KaizoFastInput, Localize("Fast input"), g_Config.m_KaizoFastInput, &Button))
	{
		g_Config.m_KaizoFastInput ^= 1;
	}

    MainView.HSplitTop(2.0f, nullptr, &MainView);

    MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_KaizoShowCrowns, Localize("Show crowns on Kaizo servers"), g_Config.m_KaizoShowCrowns, &Button))
	{
		g_Config.m_KaizoShowCrowns ^= 1;
	}

	MainView.HSplitTop(2.0f, nullptr, &MainView);

    MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_KaizoPredictDDNetTeleport, Localize("Predict DDNet teleports if there is only 1 exit"), g_Config.m_KaizoPredictDDNetTeleport, &Button))
	{
		g_Config.m_KaizoPredictDDNetTeleport ^= 1;
	}

	MainView.HSplitTop(2.0f, nullptr, &MainView);

    MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_KaizoInstaShieldShield, Localize("InstaShield Shield"), g_Config.m_KaizoInstaShieldShield, &Button))
	{
		g_Config.m_KaizoInstaShieldShield ^= 1;
	}
}
