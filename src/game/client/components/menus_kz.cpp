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

	Left.HSplitTop(20.0f, &Label, &MainView);
	Ui()->DoLabel(&Label, Localize("Kaizo Settings"), 20.0f, TEXTALIGN_ML);
	Left.HSplitTop(25.0f, &Label, &Left);

	Right.HSplitTop(20.0f, &Label, &MainView);
	Ui()->DoLabel(&Label, Localize("PvP Settings"), 20.0f, TEXTALIGN_ML);
	Right.HSplitTop(25.0f, &Label, &Right);

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

	Right.HSplitTop(2.0f, nullptr, &Right);

    Right.HSplitTop(20.0f, &Button, &Right);
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

	Right.HSplitTop(2.0f, nullptr, &Right);

    Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_KaizoKillingSpreeSparkles, Localize("Killing spree sparkles"), g_Config.m_KaizoKillingSpreeSparkles, &Button))
	{
		g_Config.m_KaizoKillingSpreeSparkles ^= 1;
	}

	Right.HSplitTop(2.0f, nullptr, &Right);

    Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_KaizoPredictPointerTWPlus, Localize("Predict Pointer's TW+"), g_Config.m_KaizoPredictPointerTWPlus, &Button))
	{
		g_Config.m_KaizoPredictPointerTWPlus ^= 1;
	}

	Right.HSplitTop(2.0f, nullptr, &Right);

    Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_KaizoPredictVanillaHammerFix, Localize("Fix Vanilla hammer hit prediction through walls"), g_Config.m_KaizoPredictVanillaHammerFix, &Button))
	{
		g_Config.m_KaizoPredictVanillaHammerFix ^= 1;
	}

	Left.HSplitTop(2.0f, nullptr, &Left);

    Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoShowClientType, Localize("Try to identify custom clients of other players and show a icon above them"), g_Config.m_KaizoShowClientType, &Button))
	{
		g_Config.m_KaizoShowClientType ^= 1;
	}

	Left.HSplitTop(2.0f, nullptr, &Left);

    Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoSendClientType, Localize("Let know other clients we are using Kaizo Network"), g_Config.m_KaizoSendClientType, &Button))
	{
		g_Config.m_KaizoSendClientType ^= 1;
	}

	Left.HSplitTop(2.0f, nullptr, &Left);

    Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoRotatingHammer, Localize("Rotating hammer (like other weapons)"), g_Config.m_KaizoRotatingHammer, &Button))
	{
		g_Config.m_KaizoRotatingHammer ^= 1;
	}

	Left.HSplitTop(2.0f, nullptr, &Left);

    Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoPredictTeleToDeath, Localize("Predict death effect if teleport leads to Death"), g_Config.m_KaizoPredictTeleToDeath, &Button))
	{
		g_Config.m_KaizoPredictTeleToDeath ^= 1;
	}

	// dangerous settings, some communities may consider it a cheat, chill communities may not, use at your own risk!
	Left.HSplitTop(40.0f, &Label, &MainView);
	Ui()->DoLabel(&Label, Localize("Dangerous Settings!"), 20.0f, TEXTALIGN_ML);
	Left.HSplitTop(45.0f, &Label, &Left);
	Left.HSplitTop(20.0f, &Label, &MainView);

	SLabelProperties DangerLabelProps;
	DangerLabelProps.SetColor(ColorRGBA(1.f,0.f,0.f));
	Ui()->DoLabel(&Label, Localize("Some communities may consider these features as cheats while others may not.\nUSE AT YOUR OWN RISK! (Some wont work in some servers and it is NOT a bug)"), 10.0f, TEXTALIGN_ML, DangerLabelProps);
	Left.HSplitTop(25.0f, &Label, &Left);

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoOldModsZooming, Localize("Zooming in Old non-DDNet mods (only if zoom is not prohibited)"), g_Config.m_KaizoOldModsZooming, &Button))
	{
		g_Config.m_KaizoOldModsZooming ^= 1;
	}

	Left.HSplitTop(2.0f, nullptr, &Left);

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_KaizoShowRechargeBar, Localize("Show weapon recharge bar"), g_Config.m_KaizoShowRechargeBar, &Button))
	{
		g_Config.m_KaizoShowRechargeBar ^= 1;
	}
}
