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
		for (int j = 0; j < 3; j++)
		{
			m_pPlayerStats[i]->AddColumnToSection(m_iSectionId, VarArgs("NameColumn%d", j), "", SectionedListPanel::COLUMN_RIGHT, nameWidth);
			m_pPlayerStats[i]->AddColumnToSection(m_iSectionId, VarArgs("ValueColumn%d", j), "", 0, valueWidth);
		}

		HFont font = m_pScheme->GetFont("IOSTeamMenuNormal");
		KeyValues *pData = new KeyValues("data");
		for (int j = 0; j < 5; j++)
		{
			m_pPlayerStats[i]->AddItem(j, pData);
			m_pPlayerStats[i]->SetItemFont(j, font);
		}
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
		{
			m_pPlayerStats[i]->SetVisible(false);
			continue;
		}

		m_pPlayerStats[i]->SetVisible(true);
		m_pPlayerStats[i]->SetSectionFgColor(0, GetGlobalTeam(TEAM_A + i)->Get_HudKitColor());
		m_pPlayerStats[i]->SetFgColor(GetGlobalTeam(TEAM_A + i)->Get_HudKitColor());
		//m_pPlayerStats[i]->SetSectionDividerColor(0, Color(255, 255, 255, 255));

		wchar_t wszText[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode(gr->GetPlayerName(playerIndices[i]), wszText, sizeof(wszText));
		m_pPlayerStats[i]->ModifyColumn(0, "NameColumn0", wszText);
		g_pVGuiLocalize->ConvertANSIToUnicode(g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][gr->GetTeamPosIndex(playerIndices[i])][POS_NAME]], wszText, sizeof(wszText));
		m_pPlayerStats[i]->ModifyColumn(0, "NameColumn1", wszText);
		g_pVGuiLocalize->ConvertANSIToUnicode(gr->GetCountryName(playerIndices[i]), wszText, sizeof(wszText));
		m_pPlayerStats[i]->ModifyColumn(0, "NameColumn2", wszText);

		KeyValues *pData = new KeyValues("data");

		pData->SetString("NameColumn0", "Goals:");
		pData->SetInt("ValueColumn0", gr->GetGoals(playerIndices[i]));
		pData->SetString("NameColumn1", "Assists:");
		pData->SetInt("ValueColumn1", gr->GetAssists(playerIndices[i]));
		pData->SetString("NameColumn2", "Ping:");
		pData->SetInt("ValueColumn2", gr->GetPing(playerIndices[i]));

		m_pPlayerStats[i]->ModifyItem(0, 0, pData);
		m_pPlayerStats[i]->SetItemFgColor(0, GetGlobalTeam(TEAM_A + i)->Get_HudKitColor());
		pData->Clear();

		pData->SetString("NameColumn0", "Fouls:");
		pData->SetInt("ValueColumn0", gr->GetFouls(playerIndices[i]));
		pData->SetString("NameColumn1", "Yellows:");
		pData->SetInt("ValueColumn1", gr->GetYellowCards(playerIndices[i]));
		pData->SetString("NameColumn2", "Reds:");
		pData->SetInt("ValueColumn2", gr->GetRedCards(playerIndices[i]));

		m_pPlayerStats[i]->ModifyItem(1, 0, pData);
		m_pPlayerStats[i]->SetItemFgColor(1, GetGlobalTeam(TEAM_A + i)->Get_HudKitColor());
		pData->Clear();

		pData->SetString("NameColumn0", "Penalties:");
		pData->SetInt("ValueColumn0", gr->GetPenalties(playerIndices[i]));	
		pData->SetString("NameColumn1", "Goal kicks:");
		pData->SetInt("ValueColumn1", gr->GetGoalKicks(playerIndices[i]));
		pData->SetString("NameColumn2", "Free kicks:");
		pData->SetInt("ValueColumn2", gr->GetFreeKicks(playerIndices[i]));

		m_pPlayerStats[i]->ModifyItem(2, 0, pData);
		m_pPlayerStats[i]->SetItemFgColor(2, GetGlobalTeam(TEAM_A + i)->Get_HudKitColor());
		pData->Clear();

		pData->SetString("NameColumn0", "Distance:");
		pData->SetString("ValueColumn0", VarArgs("%d m", gr->GetDistanceCovered(playerIndices[i])));
		pData->SetString("NameColumn1", "Possession:");
		pData->SetString("ValueColumn1", VarArgs("%d%%", gr->GetPossession(playerIndices[i])));
		pData->SetString("NameColumn2", "Offsides:");
		pData->SetInt("ValueColumn2", gr->GetOffsides(playerIndices[i]));

		m_pPlayerStats[i]->ModifyItem(3, 0, pData);
		m_pPlayerStats[i]->SetItemFgColor(3, GetGlobalTeam(TEAM_A + i)->Get_HudKitColor());
		pData->Clear();

		pData->SetString("NameColumn0", "Corners:");
		pData->SetInt("ValueColumn0", gr->GetCorners(playerIndices[i]));

		m_pPlayerStats[i]->ModifyItem(4, 0, pData);
		m_pPlayerStats[i]->SetItemFgColor(4, GetGlobalTeam(TEAM_A + i)->Get_HudKitColor());

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