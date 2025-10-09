// Copyright (C) Benjamín Gajardo (also known as +KZ)
//
// RenderSettingsKaizo has code from menus_settings.cpp

#include <game/localization.h>
#include "menus.h"

void CMenus::RenderSettingsKaizo(CUIRect MainView)
{
	CUIRect Button;

	CUIRect Label;

	CUIRect Left, Right;

	MainView.VSplitMid(&Left, &Right, 20.0f);

	MainView.HSplitTop(20.0f, &Label, &MainView);
	Ui()->DoLabel(&Label, Localize("Kaizo Settings"), 20.0f, TEXTALIGN_ML);

	Left.HSplitTop(25.0f, &Label, &Left);

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoFastInput, Localize("Fast input"), g_Config.m_KaizoFastInput, &Button))
	{
		g_Config.m_KaizoFastInput ^= 1;
	}

    Left.HSplitTop(2.0f, nullptr, &Left);

    Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoShowCrowns, Localize("Show crowns on Kaizo servers"), g_Config.m_KaizoShowCrowns, &Button))
	{
		g_Config.m_KaizoShowCrowns ^= 1;
	}

	Left.HSplitTop(2.0f, nullptr, &Left);

    Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoPredictDDNetTeleport, Localize("Predict DDNet teleports if there is only 1 exit"), g_Config.m_KaizoPredictDDNetTeleport, &Button))
	{
		g_Config.m_KaizoPredictDDNetTeleport ^= 1;
	}

	Left.HSplitTop(2.0f, nullptr, &Left);

    Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoInstaShieldShield, Localize("InstaShield Shield"), g_Config.m_KaizoInstaShieldShield, &Button))
	{
		g_Config.m_KaizoInstaShieldShield ^= 1;
	}

	Left.HSplitTop(2.0f, nullptr, &Left);

    Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoPredictDeathTiles, Localize("Predict Death tiles"), g_Config.m_KaizoPredictDeathTiles, &Button))
	{
		g_Config.m_KaizoPredictDeathTiles ^= 1;
	}

	Left.HSplitTop(2.0f, nullptr, &Left);

    Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoPingCircles, Localize("Show ping circles above players on Kaizo servers"), g_Config.m_KaizoPingCircles, &Button))
	{
		g_Config.m_KaizoPingCircles ^= 1;
	}

	Left.HSplitTop(2.0f, nullptr, &Left);

    Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoSleepingInMenuPlayers, Localize("Show players navigating menu as sleeping tees"), g_Config.m_KaizoSleepingInMenuPlayers, &Button))
	{
		g_Config.m_KaizoSleepingInMenuPlayers ^= 1;
	}
}
