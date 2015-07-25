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

#include "c_match_ball.h"
#include "c_team.h"
#include "sdk_backgroundpanel.h"
#include "sdk_gamerules.h"
#include "c_sdk_player.h"
#include "steam/steam_api.h"
#include "clientscoreboarddialog.h"
#include "sdk_gamerules.h"

#include "materialsystem/itexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ConVar sv_singlekeeper;

enum { FORMATION_BUTTON_WIDTH = 90, FORMATION_BUTTON_HEIGHT = 55 };
enum { FORMATION_HPADDING = (FORMATION_BUTTON_WIDTH / 2 + 80), FORMATION_VTOPPADDING = (FORMATION_BUTTON_HEIGHT / 2 + 10), FORMATION_VBOTTOMPADDING = (FORMATION_BUTTON_HEIGHT / 2 + 20), FORMATION_CENTERPADDING = 40 };
enum { TOOLTIP_WIDTH = 100, TOOLTIP_HEIGHT = 20 };
enum { FORMATION_NAME_WIDTH = 100, FORMATION_NAME_HEIGHT = 25, FORMATION_NAME_HMARGIN = 80, FORMATION_NAME_VMARGIN = 7 };

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
			m_pFormationButtons[i][j]->SetCommand(VarArgs("jointeam %d %d", i + TEAM_HOME, j));
			m_pFormationButtons[i][j]->AddActionSignalTarget(this);

			m_pToolTips[i][j] = new Label(m_pFormations[i], "", "");
			//m_pToolTips[i][j]->SetVisible(false);
		}
	}

	m_flNextUpdateTime = gpGlobals->curtime;
	m_bShowCaptainMenu = false;
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

	Color black(0, 0, 0, 240);

	for (int i = 0; i < 2; i++)
	{
		m_pFormations[i]->SetBounds(i * (GetWide() / 2), 0, GetWide() / 2, GetTall());

		for (int j = 0; j < 11; j++)
		{
			m_pFormationButtons[i][j]->SetBounds(0, 0, FORMATION_BUTTON_WIDTH, FORMATION_BUTTON_HEIGHT);
			m_pFormationButtons[i][j]->SetContentAlignment(Label::a_center);
			m_pFormationButtons[i][j]->SetTextInset(0, 0);
			m_pFormationButtons[i][j]->SetFont(m_pScheme->GetFont("ScoreboardShirtPos"));
			/*color32 enabled = { 150, 150, 150, 150 };
			color32 mouseover = { 150, 150, 150, 240 };
			color32 pressed = { 255, 255, 255, 240 };
			color32 disabled = { 75, 75, 75, 240 };
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", enabled);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "vgui/shirt", mouseover);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_PRESSED, "vgui/shirt", pressed);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_DISABLED, "vgui/shirt", disabled);*/
			m_pFormationButtons[i][j]->SetDefaultColor(black, black);
			m_pFormationButtons[i][j]->SetArmedColor(black, black);
			m_pFormationButtons[i][j]->SetDepressedColor(black, black);
			m_pFormationButtons[i][j]->SetDisabledFgColor1(Color(0, 0, 0, 0));
			m_pFormationButtons[i][j]->SetDisabledFgColor2(black);
			m_pFormationButtons[i][j]->SetPaintBorderEnabled(false);
			m_pFormationButtons[i][j]->SetName(VarArgs("%d", 0));

			m_pToolTips[i][j]->SetZPos(10);
			m_pToolTips[i][j]->SetTextInset(0, 0);
			m_pToolTips[i][j]->SetContentAlignment(Label::a_north);
			m_pToolTips[i][j]->SetFont(m_pScheme->GetFont("ScoreboardShirtText"));
			m_pToolTips[i][j]->SetPaintBackgroundEnabled(false);
			m_pToolTips[i][j]->SetPaintBorderEnabled(false);
			m_pToolTips[i][j]->SetFgColor(Color(255, 255, 255, 255));
		}
	}

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

void CFormationMenu::Update(bool showCaptainMenu)
{
	IGameResources *gr = GameResources();
	if (!gr)
		return;

	m_bShowCaptainMenu = showCaptainMenu;

	int playerIndexAtPos[2][11] = {};
	bool swapperAtPos[2][11] = {};
	int keeperCount = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!gr->IsConnected(i))
			continue;

		if (gr->GetTeam(i) == TEAM_HOME || gr->GetTeam(i) == TEAM_AWAY)
		{
			playerIndexAtPos[gr->GetTeam(i) - TEAM_HOME][gr->GetTeamPosIndex(i)] = i;
			
			if (gr->GetTeamPosType(i) == POS_GK)
				keeperCount += 1;
		}
		
		if (gr->GetTeamToJoin(i) == TEAM_HOME || gr->GetTeamToJoin(i) == TEAM_AWAY)
		{
			if (playerIndexAtPos[gr->GetTeamToJoin(i) - TEAM_HOME][gr->GetTeamPosIndexToJoin(i)] == 0)
			{
				playerIndexAtPos[gr->GetTeamToJoin(i) - TEAM_HOME][gr->GetTeamPosIndexToJoin(i)] = i;
			}
			
			if (gr->GetTeamToJoin(i) == gr->GetTeam(GetLocalPlayerIndex()) && gr->GetTeamPosIndexToJoin(i) == gr->GetTeamPosIndex(GetLocalPlayerIndex()))
			{
				if (gr->GetTeam(i) != TEAM_SPECTATOR)
					swapperAtPos[gr->GetTeam(i) - TEAM_HOME][gr->GetTeamPosIndex(i)] = true;
			}
		}
	}

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			if (j > mp_maxplayers.GetInt() - 1)
			{
				m_pFormationButtons[i][j]->SetVisible(false);
				m_pToolTips[i][j]->SetVisible(false);
				continue;
			}

			bool posBlocked = SDKGameRules()->GetMatchDisplayTimeSeconds(true, false) < GetGlobalTeam(TEAM_HOME + i)->m_PosNextJoinSeconds[j]
							  || sv_singlekeeper.GetInt() == 2 && GetGlobalTeam(TEAM_HOME + i)->GetFormation()->positions[j]->type == POS_GK && playerIndexAtPos[i][j] == 0 && keeperCount == 1;

			Color color = posBlocked ? g_ColorRed : g_ColorGray;

			color32 taken = { color.r(), color.g(), color.b(), 240 };
			color32 free = { color.r(), color.g(), color.b(), 20 };
			color32 hover = { color.r(), color.g(), color.b(), 240 };
			color32 pressed = { color.r(), color.g(), color.b(), 20 };

			m_pFormationButtons[i][j]->SetVisible(true);
			m_pFormationButtons[i][j]->SetText(VarArgs("%s", g_szPosNames[GetGlobalTeam(TEAM_HOME + i)->GetFormation()->positions[j]->type]));

			m_pToolTips[i][j]->SetVisible(true);

			float xDist = (m_pFormations[i]->GetWide() - 2 * FORMATION_HPADDING) / 3;
			float yDist = (m_pFormations[i]->GetTall() - FORMATION_VTOPPADDING - FORMATION_VBOTTOMPADDING) / 3;
			float xPos = FORMATION_HPADDING + GetGlobalTeam(TEAM_HOME + i)->GetFormation()->positions[j]->x * xDist - m_pFormationButtons[i][j]->GetWide() / 2;
			xPos += (i == 0 ? -1 : 1) * FORMATION_CENTERPADDING;
			float yPos = FORMATION_VTOPPADDING + GetGlobalTeam(TEAM_HOME + i)->GetFormation()->positions[j]->y * yDist - m_pFormationButtons[i][j]->GetTall() / 2;
			m_pFormationButtons[i][j]->SetPos(xPos, yPos);

			bool isFree = (playerIndexAtPos[i][j] == 0);

			m_pFormationButtons[i][j]->SetCursor(m_bShowCaptainMenu ? dc_arrow : dc_hand);

			KeyValues *kv = new KeyValues("Command");

			char *cmd = VarArgs("jointeam %d %d", i + TEAM_HOME, j);

			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", isFree ? free : taken);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "vgui/shirt", m_bShowCaptainMenu ? (isFree ? free : taken) : hover);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_PRESSED, "vgui/shirt", pressed);
			//m_pFormationButtons[i][j]->SetName(VarArgs("%d", playerIndexAtPos[i][j]));

			m_pToolTips[i][j]->SetBounds(m_pFormationButtons[i][j]->GetX() + m_pFormationButtons[i][j]->GetWide() / 2 - TOOLTIP_WIDTH / 2, m_pFormationButtons[i][j]->GetY() + m_pFormationButtons[i][j]->GetTall() - 3, TOOLTIP_WIDTH, TOOLTIP_HEIGHT);

			KeyValues *old = m_pFormationButtons[i][j]->GetCommand();
			kv->SetString("command", cmd);
			kv->SetInt("playerindex", playerIndexAtPos[i][j]);
			kv->SetInt("posindex", j);
			kv->SetInt("team", i + TEAM_HOME);
			kv->SetInt("selected", old->GetInt("selected"));

			bool localPlayerWantsToJoinPos = gr->GetTeamToJoin(GetLocalPlayerIndex()) == i + TEAM_HOME && gr->GetTeamPosIndexToJoin(GetLocalPlayerIndex()) == j;

			int alpha = 240;

			char *msg;

			if (kv->GetInt("selected") == 1)
			{
				if (kv->GetInt("playerindex") > 0)
				{
					if (playerIndexAtPos[i][j] == GetLocalPlayerIndex())
					{
						if (localPlayerWantsToJoinPos)
							msg = "CANCEL";
						else
							msg = "YOU";
					}
					else
					{
						if (localPlayerWantsToJoinPos)
							msg = "CANCEL";
						else
							msg = "SWAP";
					}
				}
				else
				{
					if (posBlocked)
						msg = "BLOCKED";
					else
						msg = "JOIN";
				}
			}
			else
			{
				if (localPlayerWantsToJoinPos)
				{
					if (playerIndexAtPos[i][j] == GetLocalPlayerIndex())
						msg = "JOINING";
					else
						msg = "SWAPPING";
				}
				else if (swapperAtPos[i][j])
					msg = "SWAPPER";
				else
				{
					if (playerIndexAtPos[i][j] > 0)
						msg = VarArgs("%d", gr->GetShirtNumber(playerIndexAtPos[i][j]));//VarArgs("%d | %s", gr->GetShirtNumber(playerIndexAtPos[i][j]), gr->GetPlayerName(playerIndexAtPos[i][j]));
					else
					{
						alpha = 20;

						if (posBlocked)
							msg = "BLOCKED";
						else
							msg = "JOIN";
					}
				}
			}

			m_pToolTips[i][j]->SetText(msg);

			color = Color(color.r(), color.g(), color.b(), alpha);
			m_pToolTips[i][j]->SetFgColor(color);

			m_pFormationButtons[i][j]->SetCommand(kv);
		}
	}
}

//-----------------------------------------------------------------------------
// IOS Added
//-----------------------------------------------------------------------------
void CFormationMenu::OnCommand( char const *cmd )
{
	if (!Q_strnicmp(cmd, "jointeam", 8))
	{
		if (m_bShowCaptainMenu)
			return;

		engine->ClientCmd(cmd);
	}
	else if (!Q_strnicmp(cmd, "set", 3))
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
	if (m_bShowCaptainMenu)
		return;

	KeyValues *old = ((Button *)panel)->GetCommand();
	((CClientScoreBoardDialog *)gViewPortInterface->FindPanelByName(PANEL_SCOREBOARD))->SetHighlightedPlayer(old->GetInt("playerindex"));
	//KeyValues *kv = ((Button *)panel)->GetCommand();
	//kv->SetInt("selected", 1);
	//((Button *)panel)->SetCommand(kv);
	KeyValues *kv = new KeyValues("Command");
	kv->SetString("command", old->GetString("command"));
	kv->SetInt("playerindex", old->GetInt("playerindex"));
	kv->SetInt("posindex", old->GetInt("posindex"));
	kv->SetInt("team", old->GetInt("team"));
	//KeyValues *kv = ((Button *)panel)->GetCommand();
	kv->SetInt("selected", 1);
	((Button *)panel)->SetCommand(kv);

	Update(m_bShowCaptainMenu);
}

void CFormationMenu::OnCursorExited(Panel *panel)
{
	if (m_bShowCaptainMenu)
		return;

	KeyValues *old = ((Button *)panel)->GetCommand();
	((CClientScoreBoardDialog *)gViewPortInterface->FindPanelByName(PANEL_SCOREBOARD))->SetHighlightedPlayer(0);
	KeyValues *kv = new KeyValues("Command");
	kv->SetString("command", old->GetString("command"));
	kv->SetInt("playerindex", old->GetInt("playerindex"));
	kv->SetInt("posindex", old->GetInt("posindex"));
	kv->SetInt("team", old->GetInt("team"));
	//KeyValues *kv = ((Button *)panel)->GetCommand();
	kv->SetInt("selected", 0);
	((Button *)panel)->SetCommand(kv);

	Update(m_bShowCaptainMenu);
}