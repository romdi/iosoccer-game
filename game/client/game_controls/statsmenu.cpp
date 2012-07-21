//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>

#include "statsmenu.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <FileSystem.h>

#include <vgui_controls/RichText.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>

#include "IGameUIFuncs.h" // for key bindings
#include <igameresources.h>

#include "vgui_bitmapbutton.h"	//ios

#include <game/client/iviewport.h>
#include <stdlib.h> // MAX_PATH define
#include <stdio.h>
#include "byteswap.h"

#include "c_ball.h"
#include "c_team.h"
#include "sdk_backgroundpanel.h"
#include "sdk_gamerules.h"
#include "c_sdk_player.h"
#include "steam/steam_api.h"

#include "materialsystem/itexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CStatsMenu::CStatsMenu(Panel *parent, const char *name) : Panel(parent, name)
{
	for (int i = 0; i < 2; i++)
	{
		m_pPlayerStats[i] = new SectionedListPanel(this, "");
		m_pPlayerStats[i]->SetVerticalScrollbar(false);
	}

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CStatsMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScheme = pScheme;
}

void CStatsMenu::Reset()
{
	for (int i = 0; i < 2; i++)
	{
		m_pPlayerStats[i]->DeleteAllItems();
		m_pPlayerStats[i]->RemoveAllSections();

		int m_iSectionId = 0;

		m_pPlayerStats[i]->AddSection(m_iSectionId, "");
		m_pPlayerStats[i]->SetSectionAlwaysVisible(0);
		m_pPlayerStats[i]->SetFontSection(m_iSectionId, m_pScheme->GetFont("StatsPlayerName"));
		m_pPlayerStats[i]->SetLineSpacing(30);
		const int nameWidth = 110;
		const int valueWidth = 65;
		m_pPlayerStats[i]->SetSectionDividerColor(m_iSectionId, Color(255, 255, 255, 255));
		m_pPlayerStats[i]->AddColumnToSection(m_iSectionId, "NameColumn0", "", 0, nameWidth);
		m_pPlayerStats[i]->AddColumnToSection(m_iSectionId, "ValueColumn0", "", 0, valueWidth);
		m_pPlayerStats[i]->AddColumnToSection(m_iSectionId, "NameColumn1", "", 0, nameWidth);
		m_pPlayerStats[i]->AddColumnToSection(m_iSectionId, "ValueColumn1", "", 0, valueWidth);
		m_pPlayerStats[i]->AddColumnToSection(m_iSectionId, "NameColumn2", "", 0, nameWidth);
		m_pPlayerStats[i]->AddColumnToSection(m_iSectionId, "ValueColumn2", "", 0, valueWidth);
		m_pPlayerStats[i]->AddColumnToSection(m_iSectionId, "NameColumn3", "", 0, nameWidth);
		m_pPlayerStats[i]->AddColumnToSection(m_iSectionId, "ValueColumn3", "", 0, valueWidth);

		HFont font = m_pScheme->GetFont("IOSTeamMenuNormal");
		KeyValues *pData = new KeyValues("data");
		m_pPlayerStats[i]->AddItem(0, pData);
		m_pPlayerStats[i]->SetItemFont(0, font);
		m_pPlayerStats[i]->AddItem(0, pData);
		m_pPlayerStats[i]->SetItemFont(1, font);
		m_pPlayerStats[i]->AddItem(0, pData);
		m_pPlayerStats[i]->SetItemFont(2, font);
		m_pPlayerStats[i]->AddItem(0, pData);
		m_pPlayerStats[i]->SetItemFont(3, font);
		m_pPlayerStats[i]->AddItem(0, pData);
		m_pPlayerStats[i]->SetItemFont(4, font);
		pData->deleteThis();
	}
}

void CStatsMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	for (int i = 0; i < 2; i++)
	{
		m_pPlayerStats[i]->SetBounds(i * (GetWide() / 2), 0, GetWide() / 2, GetTall());
		m_pPlayerStats[i]->SetPaintBorderEnabled(false);
	}

	m_flNextUpdateTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CStatsMenu::~CStatsMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: updates the UI with a new map name and map html page, and sets up the team buttons
//-----------------------------------------------------------------------------
void CStatsMenu::OnThink()
{
	if (m_flNextUpdateTime > gpGlobals->curtime)
		return;

	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
}

void CStatsMenu::Update(int *playerIndices)
{
	IGameResources *gr = GameResources();
	if (!gr)
		return;

	for (int i = 0; i < 2; i++)
	{
		if (playerIndices[i] == 0)
			continue;

		wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode(gr->GetPlayerName(playerIndices[i]), wszPlayerName, sizeof(wszPlayerName));
		m_pPlayerStats[i]->ModifyColumn(0, "NameColumn0", wszPlayerName);

		KeyValues *pData = new KeyValues("data");

		pData->SetString("NameColumn0", "Goals:");
		pData->SetInt("ValueColumn0", gr->GetGoals(playerIndices[i]));
		pData->SetString("NameColumn1", "Assists:");
		pData->SetInt("ValueColumn1", gr->GetAssists(playerIndices[i]));
		pData->SetString("NameColumn2", "Ping:");
		pData->SetInt("ValueColumn2", gr->GetPing(playerIndices[i]));

		m_pPlayerStats[i]->ModifyItem(0, 0, pData);
		pData->Clear();

		pData->SetString("NameColumn0", "Fouls:");
		pData->SetInt("ValueColumn0", gr->GetFouls(playerIndices[i]));
		pData->SetString("NameColumn1", "Yellows:");
		pData->SetInt("ValueColumn1", gr->GetYellowCards(playerIndices[i]));
		pData->SetString("NameColumn2", "Reds:");
		pData->SetInt("ValueColumn2", gr->GetRedCards(playerIndices[i]));

		m_pPlayerStats[i]->ModifyItem(1, 0, pData);
		pData->Clear();

		pData->SetString("NameColumn0", "Penalties:");
		pData->SetInt("ValueColumn0", gr->GetPenalties(playerIndices[i]));	
		pData->SetString("NameColumn1", "Goal kicks:");
		pData->SetInt("ValueColumn1", gr->GetGoalKicks(playerIndices[i]));
		pData->SetString("NameColumn2", "Free kicks:");
		pData->SetInt("ValueColumn2", gr->GetFreeKicks(playerIndices[i]));

		m_pPlayerStats[i]->ModifyItem(2, 0, pData);
		pData->Clear();

		pData->SetString("NameColumn0", "Distance:");
		pData->SetString("ValueColumn0", VarArgs("%d m", gr->GetDistanceCovered(playerIndices[i])));
		pData->SetString("NameColumn1", "Possession:");
		pData->SetString("ValueColumn1", VarArgs("%d%%", gr->GetPossession(playerIndices[i])));
		pData->SetString("NameColumn2", "Offsides:");
		pData->SetInt("ValueColumn2", gr->GetOffsides(playerIndices[i]));

		m_pPlayerStats[i]->ModifyItem(3, 0, pData);
		pData->Clear();

		pData->SetString("NameColumn0", "Corners:");
		pData->SetInt("ValueColumn0", gr->GetCorners(playerIndices[i]));

		m_pPlayerStats[i]->ModifyItem(4, 0, pData);

		pData->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// IOS Added
//-----------------------------------------------------------------------------
void CStatsMenu::OnCommand( char const *cmd )
{
	BaseClass::OnCommand(cmd);

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CStatsMenu::OnTextChanged(KeyValues *data)
{
}