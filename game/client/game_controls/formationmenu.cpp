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

#include "materialsystem/itexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

enum { FORMATION_BUTTON_WIDTH = 85, FORMATION_BUTTON_HEIGHT = 50 };
enum { FORMATION_HPADDING = (FORMATION_BUTTON_WIDTH / 2 + 40), FORMATION_VPADDING = (FORMATION_BUTTON_HEIGHT / 2 + 15) };

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
		C_Team *pTeam = GetGlobalTeam(TEAM_A + i);
		m_pFormations[i]->SetBounds(i * (GetWide() / 2), 0, GetWide() / 2, GetTall());

		for (int j = 0; j < 11; j++)
		{
			m_pFormationButtons[i][j]->SetBounds(0, 0, FORMATION_BUTTON_WIDTH, FORMATION_BUTTON_HEIGHT);
			m_pFormationButtons[i][j]->SetContentAlignment(Label::a_center);
			m_pFormationButtons[i][j]->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
			color32 enabled = { 255, 255, 255, 255 };
			color32 mouseover = { 150, 150, 150, 255 };
			color32 pressed = { 255, 255, 255, 255 };
			color32 disabled = { 75, 75, 75, 255 };
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", enabled);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "vgui/shirt", mouseover);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_PRESSED, "vgui/shirt", pressed);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_DISABLED, "vgui/shirt", disabled);
			Color black(0, 0, 0, 255);
			m_pFormationButtons[i][j]->SetDefaultColor(black, black);
			m_pFormationButtons[i][j]->SetArmedColor(black, black);
			m_pFormationButtons[i][j]->SetDepressedColor(black, black);
			m_pFormationButtons[i][j]->SetDisabledFgColor1(Color(0, 0, 0, 0));
			m_pFormationButtons[i][j]->SetDisabledFgColor2(black);
			m_pFormationButtons[i][j]->SetPaintBorderEnabled(false);
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
			m_pFormationButtons[i][j]->SetText(g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][j][POS_NAME]]);

			float xDist = (m_pFormations[i]->GetWide() - 2 * FORMATION_HPADDING) / 3;
			float yDist = (m_pFormations[i]->GetTall() - 2 * FORMATION_VPADDING) / 3;
			float xPos = FORMATION_HPADDING + g_Positions[mp_maxplayers.GetInt() - 1][j][POS_XPOS] * xDist - m_pFormationButtons[i][j]->GetWide() / 2;
			float yPos = FORMATION_VPADDING + g_Positions[mp_maxplayers.GetInt() - 1][j][POS_YPOS] * yDist - m_pFormationButtons[i][j]->GetTall() / 2;
			m_pFormationButtons[i][j]->SetPos(xPos, yPos);

			color32 color;
			int cursor;
			bool enable;

			if (playerIndexAtPos[i][j] == 0)
			{
				color32 enabled = { 255, 255, 255, 255 }; 
				color = enabled;
				cursor = dc_hand;
				enable = true;
			}
			else
			{
				//m_pFormationButtons[i][j]->SetText("JOIN");
				color32 human = { 255, 255, 255, 255 };
				color32 bot = { 75, 75, 75, 255 };
				color = gr->IsFakePlayer(playerIndexAtPos[i][j]) ? bot : human;
				cursor = gr->IsFakePlayer(playerIndexAtPos[i][j]) ? dc_hand : dc_arrow;
				enable = gr->IsFakePlayer(playerIndexAtPos[i][j]);
				//m_pFormationButtons[i][j]->SetFgColor(gr->GetTeamColor(TEAM_UNASSIGNED));
				//m_pFormationButtons[i][j]->SetDefaultColor(black, darker);
				//m_pFormationButtons[i][j]->SetArmedColor(black, dark);
				//m_pFormationButtons[i][j]->SetDepressedColor(black, darker);
				//pButton->SetFgColor(gr->GetTeamColor(teamIndex));
				//Color black(0, 0, 0, 255);
				//pButton->SetDefaultColor(black, black);
				//pButton->SetArmedColor(black, black);
				//pButton->SetDepressedColor(black, black);
				//pButton->SetDefaultColor(black, lighter);
				//pButton->SetArmedColor(black, white);
				//pButton->SetDepressedColor(black, lighter);
				//pButton->SetDisabledFgColor1(black);
				//pButton->SetDisabledFgColor2(black);
				//pButton->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
			}

			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", color);
			m_pFormationButtons[i][j]->SetCursor(cursor);
			m_pFormationButtons[i][j]->SetEnabled(enable);
			Color teamColor = GetGlobalTeam(TEAM_A + i)->Get_HudKitColor();
			color32 enabled = { teamColor.r(), teamColor.g(), teamColor.b(), 255 };
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", enabled);
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