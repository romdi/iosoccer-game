//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>

#include "matcheventmenu.h"

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
#include "clientscoreboarddialog.h"

#include "materialsystem/itexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMatchEventMenu::CMatchEventMenu(Panel *parent, const char *name) : Panel(parent, name)
{
	for (int i = 0; i < 2; i++)
	{
		m_pMatchEvents[i] = new SectionedListPanel(this, "");
		m_pMatchEvents[i]->SetVerticalScrollbar(false);
	}

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CMatchEventMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScheme = pScheme;
}

void CMatchEventMenu::Reset()
{
	for (int i = 0; i < 2; i++)
	{
		m_pMatchEvents[i]->DeleteAllItems();
		m_pMatchEvents[i]->RemoveAllSections();
		m_pMatchEvents[i]->AddSection(0, "");
		m_pMatchEvents[i]->SetSectionAlwaysVisible(0);
		m_pMatchEvents[i]->SetFontSection(0, m_pScheme->GetFont("IOSTeamMenuNormal"));
		//m_pMatchEvents[i]->SetLineSpacing(20);
		m_pMatchEvents[i]->SetFgColor(Color(255, 255, 255, 255));
		m_pMatchEvents[i]->SetSectionFgColor(0, Color(255, 255, 255, 255));
		m_pMatchEvents[i]->AddColumnToSection(0, "minute", "Minute", SectionedListPanel::HEADER_CENTER | SectionedListPanel::COLUMN_CENTER, 70);
		m_pMatchEvents[i]->AddColumnToSection(0, "event", "Event", SectionedListPanel::HEADER_CENTER | SectionedListPanel::COLUMN_CENTER, 130);
		m_pMatchEvents[i]->AddColumnToSection(0, "player", "Player", 0, 300);
	}
}

void CMatchEventMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	for (int i = 0; i < 2; i++)
	{
		int offset = 65;
		m_pMatchEvents[i]->SetBounds(i * (GetWide() / 2 + offset), 0, GetWide() / 2 - offset, GetTall());
		m_pMatchEvents[i]->SetPaintBorderEnabled(false);
	}		
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMatchEventMenu::~CMatchEventMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: updates the UI with a new map name and map html page, and sets up the team buttons
//-----------------------------------------------------------------------------
void CMatchEventMenu::OnThink()
{
	if (m_flNextUpdateTime > gpGlobals->curtime)
		return;

	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
}

void CMatchEventMenu::Update()
{
	IGameResources *gr = GameResources();
	if (!gr)
		return;

	Reset();

	for (int i = 0; i < 2; i++)
	{
		m_pMatchEvents[i]->SetFgColor(gr->GetTeamColor(TEAM_A + i));
		m_pMatchEvents[i]->SetSectionFgColor(0, gr->GetTeamColor(TEAM_A + i));
		//m_pMatchEvents[i]->SetVisible(true);

		HFont font = m_pScheme->GetFont("StatsPlayerNameSmall");

		for (int j = 0; j < MAX_MATCH_EVENTS; j++)
		{
			C_Team *pTeam = GetGlobalTeam(TEAM_A + i);

			if (pTeam->m_eMatchEventTypes[j] == MATCH_EVENT_NONE)
				continue;

			KeyValues *pData = new KeyValues("data");
			int minute = ceil(pTeam->m_nMatchEventSeconds[j] / 60.0f);
			char *time;

			if (pTeam->m_eMatchEventMatchStates[j] == MATCH_PERIOD_FIRST_HALF && minute > 45)
				time = VarArgs("%d'+%d", 45, min(4, minute - 45));
			else if (pTeam->m_eMatchEventMatchStates[j] == MATCH_PERIOD_SECOND_HALF && minute > 90)
				time = VarArgs("%d'+%d", 90, min(4, minute - 90));
			else if (pTeam->m_eMatchEventMatchStates[j] == MATCH_PERIOD_EXTRATIME_FIRST_HALF && minute > 105)
				time = VarArgs("%d'+%d", 105, min(4, minute - 105));
			else if (pTeam->m_eMatchEventMatchStates[j] == MATCH_PERIOD_EXTRATIME_SECOND_HALF && minute > 120)
				time = VarArgs("%d'+%d", 120, min(4, minute - 120));
			else
				time = VarArgs("%d'", minute);

			pData->SetString("minute", time);
			pData->SetString("event", g_szMatchEventNames[pTeam->m_eMatchEventTypes[j]]);
			pData->SetString("player", pTeam->m_szMatchEventPlayers[j]);
			m_pMatchEvents[i]->AddItem(0, pData);
			m_pMatchEvents[i]->SetItemFont(j, font);
			pData->deleteThis();
		}
	}
}

//-----------------------------------------------------------------------------
// IOS Added
//-----------------------------------------------------------------------------
void CMatchEventMenu::OnCommand( char const *cmd )
{
	BaseClass::OnCommand(cmd);

	m_flNextUpdateTime = gpGlobals->curtime;
}