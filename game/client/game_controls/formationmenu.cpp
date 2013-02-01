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

enum { FORMATION_BUTTON_WIDTH = 90, FORMATION_BUTTON_HEIGHT = 55 };
enum { FORMATION_HPADDING = (FORMATION_BUTTON_WIDTH / 2 + 40), FORMATION_VTOPPADDING = (FORMATION_BUTTON_HEIGHT / 2 + 0), FORMATION_VBOTTOMPADDING = (FORMATION_BUTTON_HEIGHT / 2 + 22), FORMATION_CENTERPADDING = 35 };
enum { TOOLTIP_WIDTH = 100, TOOLTIP_HEIGHT = 20 };
enum { TICK_WIDTH = 25, TICK_HEIGHT = 20 };

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

			m_pToolTips[i][j] = new Label(m_pFormations[i], "", "");
			//m_pToolTips[i][j]->SetVisible(false);
		}
	}

	for (int i = 0; i < 11; i++)
	{
		m_pCaptainTicks[i] = new Button(m_pFormations[0], "", "CP", this, VarArgs("setcaptain %d", i));
		m_pFreekickTakerTicks[i] = new Button(m_pFormations[0], "", "FK", this, VarArgs("setfreekicktaker %d", i));
		m_pPenaltyTakerTicks[i] = new Button(m_pFormations[0], "", "PK", this, VarArgs("setpenaltytaker %d", i));
		m_pLeftCornerTakerTicks[i] = new Button(m_pFormations[0], "", "LC", this, VarArgs("setleftcornertaker %d", i));
		m_pRightCornerTakerTicks[i] = new Button(m_pFormations[0], "", "RC", this, VarArgs("setrightcornertaker %d", i));
		m_pThrowinTakerTicks[i] = new Button(m_pFormations[0], "", "TI", this, VarArgs("setthrowintaker %d", i));
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
		//m_pFormations[i]->SetBgColor(Color(255, 0, 0, 255));

		for (int j = 0; j < 11; j++)
		{
			m_pFormationButtons[i][j]->SetBounds(0, 0, FORMATION_BUTTON_WIDTH, FORMATION_BUTTON_HEIGHT);
			m_pFormationButtons[i][j]->SetContentAlignment(Label::a_center);
			m_pFormationButtons[i][j]->SetTextInset(0, 0);
			m_pFormationButtons[i][j]->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
			color32 enabled = { 150, 150, 150, 150 };
			color32 mouseover = { 150, 150, 150, 240 };
			color32 pressed = { 255, 255, 255, 240 };
			color32 disabled = { 75, 75, 75, 240 };
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", enabled);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "vgui/shirt", mouseover);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_PRESSED, "vgui/shirt", pressed);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_DISABLED, "vgui/shirt", disabled);
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
			m_pToolTips[i][j]->SetFont(m_pScheme->GetFont("Tooltip"));
			m_pToolTips[i][j]->SetPaintBackgroundEnabled(false);
			m_pToolTips[i][j]->SetPaintBorderEnabled(false);
			m_pToolTips[i][j]->SetFgColor(Color(255, 255, 255, 255));
		}
	}

	for (int i = 0; i < 11; i++)
	{
		m_pCaptainTicks[i]->SetZPos(100);
		m_pCaptainTicks[i]->SetCursor(dc_hand);
		m_pCaptainTicks[i]->SetVisible(false);
		m_pCaptainTicks[i]->SetContentAlignment(Label::a_center);
		m_pCaptainTicks[i]->SetFont(m_pScheme->GetFont("Tooltip"));

		m_pFreekickTakerTicks[i]->SetZPos(100);
		m_pFreekickTakerTicks[i]->SetCursor(dc_hand);
		m_pFreekickTakerTicks[i]->SetVisible(false);
		m_pFreekickTakerTicks[i]->SetContentAlignment(Label::a_center);
		m_pFreekickTakerTicks[i]->SetFont(m_pScheme->GetFont("Tooltip"));

		m_pPenaltyTakerTicks[i]->SetZPos(100);
		m_pPenaltyTakerTicks[i]->SetCursor(dc_hand);
		m_pPenaltyTakerTicks[i]->SetVisible(false);
		m_pPenaltyTakerTicks[i]->SetContentAlignment(Label::a_center);
		m_pPenaltyTakerTicks[i]->SetFont(m_pScheme->GetFont("Tooltip"));

		m_pLeftCornerTakerTicks[i]->SetZPos(100);
		m_pLeftCornerTakerTicks[i]->SetCursor(dc_hand);
		m_pLeftCornerTakerTicks[i]->SetVisible(false);
		m_pLeftCornerTakerTicks[i]->SetContentAlignment(Label::a_center);
		m_pLeftCornerTakerTicks[i]->SetFont(m_pScheme->GetFont("Tooltip"));

		m_pRightCornerTakerTicks[i]->SetZPos(100);
		m_pRightCornerTakerTicks[i]->SetCursor(dc_hand);
		m_pRightCornerTakerTicks[i]->SetVisible(false);
		m_pRightCornerTakerTicks[i]->SetContentAlignment(Label::a_center);
		m_pRightCornerTakerTicks[i]->SetFont(m_pScheme->GetFont("Tooltip"));

		m_pThrowinTakerTicks[i]->SetZPos(100);
		m_pThrowinTakerTicks[i]->SetCursor(dc_hand);
		m_pThrowinTakerTicks[i]->SetVisible(false);
		m_pThrowinTakerTicks[i]->SetContentAlignment(Label::a_center);
		m_pThrowinTakerTicks[i]->SetFont(m_pScheme->GetFont("Tooltip"));
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

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!gr->IsConnected(i))
			continue;

		//if (i == GetLocalPlayerIndex() && gr->GetTeamToJoin(i) != TEAM_INVALID)
		//{
		//	statusAtPos[gr->GetTeamToJoin(i) - TEAM_A][gr->GetTeamPosIndexToJoin(i)] = 1;
		//}

		if (gr->GetTeam(i) == TEAM_A || gr->GetTeam(i) == TEAM_B)
		{
			playerIndexAtPos[gr->GetTeam(i) - TEAM_A][gr->GetTeamPosIndex(i)] = i;
		}
		else if (gr->GetTeamToJoin(i) == TEAM_A || gr->GetTeamToJoin(i) == TEAM_B)
		{
			if (playerIndexAtPos[gr->GetTeamToJoin(i) - TEAM_A][gr->GetTeamPosIndexToJoin(i)] == 0)
			{
				playerIndexAtPos[gr->GetTeamToJoin(i) - TEAM_A][gr->GetTeamPosIndexToJoin(i)] = i;
			}
			
			if (gr->GetTeamToJoin(i) == gr->GetTeam(GetLocalPlayerIndex()) && gr->GetTeamPosIndexToJoin(i) == gr->GetTeamPosIndex(GetLocalPlayerIndex()))
			{
				if (gr->GetTeam(i) != TEAM_SPECTATOR)
					swapperAtPos[gr->GetTeam(i) - TEAM_A][gr->GetTeamPosIndex(i)] = true;
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
				m_pCaptainTicks[j]->SetVisible(false);
				m_pFreekickTakerTicks[j]->SetVisible(false);
				m_pPenaltyTakerTicks[j]->SetVisible(false);
				m_pLeftCornerTakerTicks[j]->SetVisible(false);
				m_pRightCornerTakerTicks[j]->SetVisible(false);
				continue;
			}

			bool posBlocked = SDKGameRules()->GetMatchDisplayTimeSeconds(false, true) < GetGlobalTeam(TEAM_A + i)->m_PosNextJoinSeconds[j];

			Color c;

			if (posBlocked)
				c = g_ColorRed;
			else if (playerIndexAtPos[i][j] > 0)
				c = gr->GetPlayerColor(playerIndexAtPos[i][j]);
			else
				c = g_ColorWhite;

			color32 taken = { c.r(), c.g(), c.b(), 240 };
			color32 free = { c.r(), c.g(), c.b(), 10 };
			color32 hover = { c.r(), c.g(), c.b(), 240 };
			color32 pressed = { c.r(), c.g(), c.b(), 10 };

			m_pFormationButtons[i][j]->SetVisible(true);
			m_pFormationButtons[i][j]->SetText(VarArgs("%s", g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][j][POS_TYPE]]));

			m_pToolTips[i][j]->SetVisible(true);

			if (m_bShowCaptainMenu && gr->GetTeam(GetLocalPlayerIndex()) == TEAM_A + i)
			{
				m_pCaptainTicks[j]->SetParent(m_pFormations[i]);
				m_pCaptainTicks[j]->SetVisible(true);
				m_pCaptainTicks[j]->SetBounds(m_pFormationButtons[i][j]->GetX() + FORMATION_BUTTON_WIDTH / 2 - TICK_WIDTH / 2, m_pFormationButtons[i][j]->GetY() + m_pFormationButtons[i][j]->GetTall(), TICK_WIDTH, TICK_HEIGHT);
				if (GetGlobalTeam(TEAM_A + i)->Get_CaptainPosIndex() == j)
				{
					m_pCaptainTicks[j]->SetDefaultColor(Color(0, 0, 0, 255), Color(taken.r, taken.g, taken.b, taken.a));
				}
				else
				{
					m_pCaptainTicks[j]->SetDefaultColor(Color(255, 255, 255, 255), Color(free.r, free.g, free.b, 0));
				}

				m_pFreekickTakerTicks[j]->SetParent(m_pFormations[i]);
				m_pFreekickTakerTicks[j]->SetVisible(true);
				m_pFreekickTakerTicks[j]->SetBounds(m_pFormationButtons[i][j]->GetX() - TICK_WIDTH + TICK_WIDTH * 0.75f, m_pFormationButtons[i][j]->GetY() + m_pFormationButtons[i][j]->GetTall() - TICK_WIDTH / 2, TICK_WIDTH, TICK_HEIGHT);
				if (GetGlobalTeam(TEAM_A + i)->Get_FreekickTakerPosIndex() == j)
				{
					m_pFreekickTakerTicks[j]->SetDefaultColor(Color(0, 0, 0, 255), Color(taken.r, taken.g, taken.b, taken.a));
				}
				else
				{
					m_pFreekickTakerTicks[j]->SetDefaultColor(Color(255, 255, 255, 255), Color(free.r, free.g, free.b, 0));
				}

				m_pPenaltyTakerTicks[j]->SetParent(m_pFormations[i]);
				m_pPenaltyTakerTicks[j]->SetVisible(true);
				m_pPenaltyTakerTicks[j]->SetBounds(m_pFormationButtons[i][j]->GetX() + FORMATION_BUTTON_WIDTH - TICK_WIDTH * 0.75f, m_pFormationButtons[i][j]->GetY() + m_pFormationButtons[i][j]->GetTall() - TICK_WIDTH / 2, TICK_WIDTH, TICK_HEIGHT);
				if (GetGlobalTeam(TEAM_A + i)->Get_PenaltyTakerPosIndex() == j)
				{
					m_pPenaltyTakerTicks[j]->SetDefaultColor(Color(0, 0, 0, 255), Color(taken.r, taken.g, taken.b, taken.a));
				}
				else
				{
					m_pPenaltyTakerTicks[j]->SetDefaultColor(Color(255, 255, 255, 255), Color(free.r, free.g, free.b, 0));
				}

				m_pLeftCornerTakerTicks[j]->SetParent(m_pFormations[i]);
				m_pLeftCornerTakerTicks[j]->SetVisible(true);
				m_pLeftCornerTakerTicks[j]->SetBounds(m_pFormationButtons[i][j]->GetX() - TICK_WIDTH + TICK_WIDTH * 0.75f, m_pFormationButtons[i][j]->GetY() + m_pFormationButtons[i][j]->GetTall() - TICK_WIDTH * 1.25f, TICK_WIDTH, TICK_HEIGHT);
				if (GetGlobalTeam(TEAM_A + i)->Get_LeftCornerTakerPosIndex() == j)
				{
					m_pLeftCornerTakerTicks[j]->SetDefaultColor(Color(0, 0, 0, 255), Color(taken.r, taken.g, taken.b, taken.a));
				}
				else
				{
					m_pLeftCornerTakerTicks[j]->SetDefaultColor(Color(255, 255, 255, 255), Color(free.r, free.g, free.b, 0));
				}

				m_pRightCornerTakerTicks[j]->SetParent(m_pFormations[i]);
				m_pRightCornerTakerTicks[j]->SetVisible(true);
				m_pRightCornerTakerTicks[j]->SetBounds(m_pFormationButtons[i][j]->GetX() + FORMATION_BUTTON_WIDTH - TICK_WIDTH * 0.75f, m_pFormationButtons[i][j]->GetY() + m_pFormationButtons[i][j]->GetTall() - TICK_WIDTH * 1.25f, TICK_WIDTH, TICK_HEIGHT);
				if (GetGlobalTeam(TEAM_A + i)->Get_RightCornerTakerPosIndex() == j)
				{
					m_pRightCornerTakerTicks[j]->SetDefaultColor(Color(0, 0, 0, 255), Color(taken.r, taken.g, taken.b, taken.a));
				}
				else
				{
					m_pRightCornerTakerTicks[j]->SetDefaultColor(Color(255, 255, 255, 255), Color(free.r, free.g, free.b, 0));
				}
			}
			else if (!m_bShowCaptainMenu)
			{
				m_pCaptainTicks[j]->SetVisible(false);
				m_pFreekickTakerTicks[j]->SetVisible(false);
				m_pPenaltyTakerTicks[j]->SetVisible(false);
				m_pLeftCornerTakerTicks[j]->SetVisible(false);
				m_pRightCornerTakerTicks[j]->SetVisible(false);
			}

			float xDist = (m_pFormations[i]->GetWide() - 2 * FORMATION_HPADDING) / 3;
			float yDist = (m_pFormations[i]->GetTall() - FORMATION_VTOPPADDING - FORMATION_VBOTTOMPADDING) / 3;
			float xPos = FORMATION_HPADDING + g_Positions[mp_maxplayers.GetInt() - 1][j][POS_XPOS] * xDist - m_pFormationButtons[i][j]->GetWide() / 2;
			xPos += (i == 0 ? -1 : 1) * FORMATION_CENTERPADDING;
			float yPos = FORMATION_VTOPPADDING + g_Positions[mp_maxplayers.GetInt() - 1][j][POS_YPOS] * yDist - m_pFormationButtons[i][j]->GetTall() / 2;
			m_pFormationButtons[i][j]->SetPos(xPos, yPos);

			bool isFree = (playerIndexAtPos[i][j] == 0);

			m_pFormationButtons[i][j]->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
			m_pFormationButtons[i][j]->SetCursor(m_bShowCaptainMenu ? dc_arrow : dc_hand);

			KeyValues *kv = new KeyValues("Command");

			char *cmd = VarArgs("jointeam %d %d", i + TEAM_A, j);

			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", isFree ? free : taken);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "vgui/shirt", m_bShowCaptainMenu ? (isFree ? free : taken) : hover);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_PRESSED, "vgui/shirt", pressed);
			//m_pFormationButtons[i][j]->SetName(VarArgs("%d", playerIndexAtPos[i][j]));

			m_pToolTips[i][j]->SetBounds(m_pFormationButtons[i][j]->GetX() + m_pFormationButtons[i][j]->GetWide() / 2 - TOOLTIP_WIDTH / 2, m_pFormationButtons[i][j]->GetY() + m_pFormationButtons[i][j]->GetTall() - 3, TOOLTIP_WIDTH, TOOLTIP_HEIGHT);
			m_pToolTips[i][j]->SetFgColor(c);

			KeyValues *old = m_pFormationButtons[i][j]->GetCommand();
			kv->SetString("command", cmd);
			kv->SetInt("playerindex", playerIndexAtPos[i][j]);
			kv->SetInt("posindex", j);
			kv->SetInt("team", i + TEAM_A);
			kv->SetInt("selected", old->GetInt("selected"));

			char *msg;

			if (kv->GetInt("selected") == 1)
			{
				if (kv->GetInt("playerindex") > 0)
				{
					if (kv->GetInt("playerindex") == GetLocalPlayerIndex())
					{
						if (GameResources()->GetTeam(GetLocalPlayerIndex()) == TEAM_SPECTATOR)
							msg = "CANCEL";
						else
							msg = "YOU";
					}
					else
					{
						if (GameResources()->GetTeamToJoin(GetLocalPlayerIndex()) == GameResources()->GetTeam(kv->GetInt("playerindex"))
							&& GameResources()->GetTeamPosIndexToJoin(GetLocalPlayerIndex()) == GameResources()->GetTeamPosIndex(kv->GetInt("playerindex")))
							msg = "CANCEL";
						else
							msg = "SWAP";
					}
				}
				else
					msg = "JOIN";
			}
			else
			{
				if (gr->GetTeamToJoin(GetLocalPlayerIndex()) - TEAM_A == i && gr->GetTeamPosIndexToJoin(GetLocalPlayerIndex()) == j)
				{
					if (playerIndexAtPos[i][j] == GetLocalPlayerIndex())
						msg = "JOINING";
					else
						msg = "SWAPPING";
				}
				else if (swapperAtPos[i][j])
					msg = "SWAPPER";
				else
					msg = "";
			}

			m_pToolTips[i][j]->SetText(msg);

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