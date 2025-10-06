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
}