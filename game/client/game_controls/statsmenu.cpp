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

#define VALUE_WIDTH 100
#define NAME_WIDTH 300
#define ROW_HEIGHT 50
#define ROW_MARGIN 5

char g_szStatNames[STAT_COUNT][32] =
{
	"PLAYERS", "GOALS", "POSSESSION", "PING"
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CStatsMenu::CStatsMenu(Panel *parent, const char *name) : Panel(parent, name)
{
	for (int i = 0; i < STAT_COUNT; i++)
	{
		StatRow_t *pStat = new StatRow_t;
		pStat->pPanel = new Panel(this, "");
		pStat->pName = new Label(pStat->pPanel, "", g_szStatNames[i]);
		pStat->pTeams[0] = new Label(pStat->pPanel, "", "0");
		pStat->pTeams[1] = new Label(pStat->pPanel, "", "0");
		m_pStatRows[i] = pStat;
	}

	for (int i = 0; i < 2; i++)
	{
		m_pStatTargets[i] = new ComboBox(this, "", 11, false);
		m_pStatTargets[i]->SetOpenDirection(Menu::DOWN);
		m_pStatTargets[i]->SetText("");
	}

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CStatsMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScheme = pScheme;
}

void CStatsMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	m_flNextUpdateTime = gpGlobals->curtime;

	SetBgColor(Color(0, 0, 0, 245));
	SetPaintBackgroundEnabled(true);
	SetPaintBackgroundType(2);

	for (int i = 0; i < 2; i++)
	{
		m_pStatTargets[i]->SetBounds(i * (GetWide() / 2), 0, GetWide() / 2, ROW_HEIGHT);
		m_pStatTargets[i]->GetMenu()->MakeReadyForUse();
		m_pStatTargets[i]->GetMenu()->SetFgColor(Color(0, 0, 0, 255));
		m_pStatTargets[i]->GetMenu()->SetBgColor(Color(255, 255, 255, 255));
		m_pStatTargets[i]->SetFgColor(Color(0, 0, 0, 255));
		m_pStatTargets[i]->SetBgColor(Color(255, 255, 255, 255));
		m_pStatTargets[i]->GetMenu()->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
		m_pStatTargets[i]->GetMenu()->AddActionSignalTarget(this);
		m_pStatTargets[i]->AddActionSignalTarget(this);
	}

	for (int i = 0; i < STAT_COUNT; i++)
	{
		StatRow_t *pStat = m_pStatRows[i];

		pStat->pPanel->SetBounds(0, (i + 1) * (ROW_HEIGHT + ROW_MARGIN), GetWide(), ROW_HEIGHT);

		pStat->pTeams[0]->SetBounds(0, 0, VALUE_WIDTH, ROW_HEIGHT);
		pStat->pTeams[0]->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
		pStat->pTeams[0]->SetContentAlignment(Label::a_west);

		pStat->pName->SetBounds(VALUE_WIDTH, 0, NAME_WIDTH, ROW_HEIGHT);
		pStat->pName->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
		pStat->pName->SetContentAlignment(Label::a_center);

		pStat->pTeams[1]->SetBounds(VALUE_WIDTH + NAME_WIDTH, 0, VALUE_WIDTH, ROW_HEIGHT);
		pStat->pTeams[1]->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
		pStat->pTeams[1]->SetContentAlignment(Label::a_east);
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

	//C_Team *pTeams[2] = { GetGlobalTeam(TEAM_A), GetGlobalTeam(TEAM_B) };
	IGameResources *gr = GameResources();

	for (int i = 0; i < 2; i++)
	{
		//m_pStatRows[STAT_PLAYERS]->pTeams[i]->SetText(VarArgs("%d", pTeams[i]->Get_Number_Players()));
		//m_pStatRows[STAT_GOALS]->pTeams[i]->SetText(VarArgs("%d", pTeams[i]->Get_Goals()));
		//m_pStatRows[STAT_POSSESSION]->pTeams[i]->SetText(VarArgs("%d", pTeams[i]->Get_Possession()));

		m_pStatTargets[i]->DeleteAllItems();

		for (int j = 1; j <= gpGlobals->maxClients; j++)
		{
			if (!(gr->IsConnected(j) && (gr->GetTeam(j) == TEAM_A || gr->GetTeam(j) == TEAM_B)))
				continue;

			KeyValues *kv = new KeyValues("UserData", "index", j);
			m_pStatTargets[i]->AddItem(gr->GetPlayerName(j), kv);
			kv->deleteThis();
		}
	}

	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
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
	Panel *panel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );
	vgui::ComboBox *box = dynamic_cast<vgui::ComboBox *>( panel );
	IGameResources *gr = GameResources();

	if (box == m_pStatTargets[0] || box == m_pStatTargets[1])
	{
		KeyValues *kv = box->GetActiveItemUserData();
		if ( kv && gr )
		{
			int index = kv->GetInt("index");
			if (!(gr->IsConnected(index) && (gr->GetTeam(index) == TEAM_A || gr->GetTeam(index) == TEAM_B)))
				return;

			int side = box == m_pStatTargets[0] ? 0 : 1;
			IGameResources *gr = GameResources();

			m_pStatRows[STAT_PLAYERS]->pTeams[side]->SetText(g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][gr->GetTeamPosIndex(index)][POS_NAME]]);
			m_pStatRows[STAT_GOALS]->pTeams[side]->SetText(VarArgs("%d", gr->GetGoals(index)));
			m_pStatRows[STAT_POSSESSION]->pTeams[side]->SetText(VarArgs("%d", gr->GetPossession(index)));
		}
	}
}