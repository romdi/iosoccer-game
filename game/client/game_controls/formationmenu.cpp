//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>

#include "formationmenu.h"

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

enum { FORMATION_BUTTON_WIDTH = 85, FORMATION_BUTTON_HEIGHT = 50 };
enum { FORMATION_HPADDING = (FORMATION_BUTTON_WIDTH / 2 + 70), FORMATION_VPADDING = (FORMATION_BUTTON_HEIGHT / 2 + 20), FORMATION_CENTERPADDING = 35 };

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CFormationMenu::CFormationMenu(Panel *parent, const char *name) : Panel(parent, name)
{
	for (int i = 0; i < 2; i++)
	{
		m_pFormations[i] = new Panel(this, "");

		for (int j = 0; j < 11; j++)
		{
			m_pFormationButtons[i][j] = new CBitmapButton(m_pFormations[i], "", "JOIN");
			m_pFormationButtons[i][j]->SetCommand(VarArgs("jointeam %d %d", i + TEAM_A, j));
			m_pFormationButtons[i][j]->AddActionSignalTarget(this);
		}
	}

	m_pTooltip = new Label(this, "", "JOIN");
	m_pTooltip->SetVisible(false);

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CFormationMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScheme = pScheme;
}

void CFormationMenu::Reset()
{
}

void CFormationMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	for (int i = 0; i < 2; i++)
	{
		m_pFormations[i]->SetBounds(i * (GetWide() / 2), 0, GetWide() / 2, GetTall());
		//m_pFormations[i]->SetBgColor(Color(255, 0, 0, 255));

		for (int j = 0; j < 11; j++)
		{
			m_pFormationButtons[i][j]->SetBounds(0, 0, FORMATION_BUTTON_WIDTH, FORMATION_BUTTON_HEIGHT);
			m_pFormationButtons[i][j]->SetContentAlignment(Label::a_center);
			m_pFormationButtons[i][j]->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
			color32 enabled = { 150, 150, 150, 150 };
			color32 mouseover = { 150, 150, 150, 240 };
			color32 pressed = { 255, 255, 255, 240 };
			color32 disabled = { 75, 75, 75, 240 };
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", enabled);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "vgui/shirt", mouseover);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_PRESSED, "vgui/shirt", pressed);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_DISABLED, "vgui/shirt", disabled);
			Color black(0, 0, 0, 240);
			m_pFormationButtons[i][j]->SetDefaultColor(black, black);
			m_pFormationButtons[i][j]->SetArmedColor(black, black);
			m_pFormationButtons[i][j]->SetDepressedColor(black, black);
			m_pFormationButtons[i][j]->SetDisabledFgColor1(Color(0, 0, 0, 0));
			m_pFormationButtons[i][j]->SetDisabledFgColor2(black);
			m_pFormationButtons[i][j]->SetPaintBorderEnabled(false);
			m_pFormationButtons[i][j]->SetName(VarArgs("%d", 0));
		}
	}

	m_pTooltip->SetSize(100, 30);
	m_pTooltip->SetZPos(10);
	m_pTooltip->SetContentAlignment(Label::a_north);
	m_pTooltip->SetFont(m_pScheme->GetFont("Tooltip"));
	//m_pTooltip->SetBgColor(Color(0, 255, 0, 255));

	m_flNextUpdateTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CFormationMenu::~CFormationMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: updates the UI with a new map name and map html page, and sets up the team buttons
//-----------------------------------------------------------------------------
void CFormationMenu::OnThink()
{
	if (m_flNextUpdateTime > gpGlobals->curtime)
		return;

	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
}

void CFormationMenu::Update()
{
	IGameResources *gr = GameResources();
	if (!gr)
		return;

	int playerIndexAtPos[2][11] = {};

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!gr->IsConnected(i))
			continue;

		int team = (gr->GetTeamToJoin(i) == TEAM_A || gr->GetTeamToJoin(i) == TEAM_B ? gr->GetTeamToJoin(i) : gr->GetTeam(i));

		if (team == TEAM_A || team == TEAM_B)
			playerIndexAtPos[team - TEAM_A][gr->GetTeamPosIndex(i)] = i;
	}

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			if (!IsValidPosition(j))
			{
				m_pFormationButtons[i][j]->SetVisible(false);
				continue;
			}

			m_pFormationButtons[i][j]->SetVisible(true);
			m_pFormationButtons[i][j]->SetText(g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][j][POS_TYPE]]);

			float xDist = (m_pFormations[i]->GetWide() - 2 * FORMATION_HPADDING) / 3;
			float yDist = (m_pFormations[i]->GetTall() - 2 * FORMATION_VPADDING) / 3;
			float xPos = FORMATION_HPADDING + g_Positions[mp_maxplayers.GetInt() - 1][j][POS_XPOS] * xDist - m_pFormationButtons[i][j]->GetWide() / 2;
			xPos += (i == 0 ? -1 : 1) * FORMATION_CENTERPADDING;
			float yPos = FORMATION_VPADDING + g_Positions[mp_maxplayers.GetInt() - 1][j][POS_YPOS] * yDist - m_pFormationButtons[i][j]->GetTall() / 2;
			m_pFormationButtons[i][j]->SetPos(xPos, yPos);

			int cursor;
			bool enable;
			bool isTakenByBot;
			bool isFree;
			HFont font;

			if (playerIndexAtPos[i][j] == 0)
			{
				cursor = dc_hand;
				enable = true;
				isTakenByBot = false;
				isFree = true;
				font = m_pScheme->GetFont("IOSTeamMenuBig");
			}
			else
			{
				cursor = gr->IsFakePlayer(playerIndexAtPos[i][j]) ? dc_hand : dc_arrow;
				enable = gr->IsFakePlayer(playerIndexAtPos[i][j]);
				isTakenByBot = gr->IsFakePlayer(playerIndexAtPos[i][j]);
				isFree = false;
				font = m_pScheme->GetFont("IOSTeamMenuBig");
			}

			m_pFormationButtons[i][j]->SetFont(font);
			m_pFormationButtons[i][j]->SetCursor(cursor);
			KeyValues *kv = new KeyValues("Command");
			kv->SetString("command", (isFree || isTakenByBot) ? VarArgs("jointeam %d %d", i + TEAM_A, j) : "");
			kv->SetInt("playerindex", playerIndexAtPos[i][j]);
			kv->SetInt("team", i + TEAM_A);
			m_pFormationButtons[i][j]->SetCommand(kv);
			//kv->deleteThis();
			Color teamColor = GetGlobalTeam(TEAM_A + i)->Get_HudKitColor();
			color32 normal = { teamColor.r(), teamColor.g(), teamColor.b(), isFree ? 10 : 240 };
			color32 hover = { teamColor.r(), teamColor.g(), teamColor.b(), (isFree || isTakenByBot) ? 240 : 240 };
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", normal);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "vgui/shirt", hover);
			//m_pFormationButtons[i][j]->SetName(VarArgs("%d", playerIndexAtPos[i][j]));
		}
	}
}

//-----------------------------------------------------------------------------
// IOS Added
//-----------------------------------------------------------------------------
void CFormationMenu::OnCommand( char const *cmd )
{
	if (!strnicmp(cmd, "jointeam", 8))
	{
		engine->ClientCmd(cmd);
	}
	else
		BaseClass::OnCommand(cmd);

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CFormationMenu::OnTextChanged(KeyValues *data)
{
}

void CFormationMenu::OnCursorEntered(Panel *panel)
{
	char *msg = "JOIN";
	Button *pButton = ((Button *)panel);
	int playerIndex = pButton->GetCommand()->GetInt("playerindex");
	if (playerIndex > 0)
	{
		((CClientScoreBoardDialog *)gViewPortInterface->FindPanelByName(PANEL_SCOREBOARD))->SetHighlightedPlayer(playerIndex);
		if (!GameResources()->IsFakePlayer(playerIndex))
			msg = "";
	}

	m_pTooltip->SetText(msg);
	m_pTooltip->SetParent(pButton->GetParent());
	m_pTooltip->SetBounds(pButton->GetX() + pButton->GetWide() / 2 - 100 / 2, pButton->GetY() + pButton->GetTall(), 100, 30);
	m_pTooltip->SetFgColor(GetGlobalTeam(pButton->GetCommand()->GetInt("team"))->Get_HudKitColor());
	m_pTooltip->SetVisible(true);
}

void CFormationMenu::OnCursorExited(Panel *panel)
{
	((CClientScoreBoardDialog *)gViewPortInterface->FindPanelByName(PANEL_SCOREBOARD))->SetHighlightedPlayer(0);
	m_pTooltip->SetVisible(false);
}