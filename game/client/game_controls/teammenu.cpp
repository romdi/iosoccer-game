//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>

#include "teammenu.h"

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

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

#define BUTTON_WIDTH		230
#define BUTTON_HEIGHT		130
#define BUTTON_HMARGIN		15
#define BUTTON_VMARGIN		10
#define BUTTON_LEFTMARGIN	25
#define BUTTON_TOPMARGIN	70
#define POSNAME_WIDTH		35
#define NAME_HEIGHT			30
#define INFO_WIDTH			20
#define INFO_HEIGHT			30
#define IMAGE_SIZE			16
#define	STATS_WIDTH			45
#define	STATS_VALUEHEIGHT	18
#define	STATS_TEXTHEIGHT	15
#define	STATS_MARGIN		5
#define	KICKBUTTON_SIZE		20
#define TEAMBUTTON_HEIGHT	40
#define TEAMCREST_SIZE		80
#define	TEAMBUTTON_VMARGIN	7
#define SPECLIST_HEIGHT		50
#define SPECLIST_MARGIN		10
#define SPECBUTTON_WIDTH	90
#define SPECBUTTON_HEIGHT	35
#define SPECBUTTON_MARGIN	7

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTeamMenu::CTeamMenu(Panel *parent, const char *name) : Panel(parent, name)
{
	m_nActiveTeam = 0;
	m_nOldMaxPlayers = 11;

	//SetBounds(0, PANEL_MARGIN, GetWide(), GetTall());

	m_pSpectateButton = new Button(this, "SpectateButton", "Spectate");
	m_pSpectatorNames = new Label(this, "Spectators", "");

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i] = new Panel(this, VarArgs("TeamPanel%d", i));
		m_pTeamNames[i] = new Label(m_pTeamPanels[i], VarArgs("TeamLabel%d", i), "");
		m_pTeamPossession[i] = new Label(m_pTeamPanels[i], "", "");
		m_pTeamPlayerCount[i] = new Label(m_pTeamPanels[i], "", "");
		m_pTeamButtons[i] = new Button(this, "TabButton", VarArgs("Team%d", i + 1));
		m_pTeamCrests[i] = new ImagePanel(this, "");

		for (int j = 0; j < 11; j++)
		{
			PosPanel_t *pPos = new PosPanel_t;
			pPos->pPosPanel = new Panel(m_pTeamPanels[i]);
			pPos->pPlayerName = new Button(pPos->pPosPanel, "Button", "");
			pPos->pClubName = new Label(pPos->pPosPanel, "Label", "");
			pPos->pPosName = new Label(pPos->pPosPanel, "Label", "");
			pPos->pCountryFlag = new ImagePanel(pPos->pPosPanel, "Stat");
			pPos->pKickButton = new Button(pPos->pPosPanel, "KickButton", "x");

			StatPanel_t *pStats = new StatPanel_t;
			pStats->pPanel = new Panel(pPos->pPosPanel);
			pStats->pGoals = new Label(pStats->pPanel, "Stat", "");
			pStats->pGoalText = new Label(pStats->pPanel, "Stat", "");
			pStats->pAssists = new Label(pStats->pPanel, "Stat", "");
			pStats->pAssistText = new Label(pStats->pPanel, "Stat", "");
			pStats->pFouls = new Label(pStats->pPanel, "Stat", "");
			pStats->pFoulsText = new Label(pStats->pPanel, "Stat", "");
			pStats->pYellowCards = new Label(pStats->pPanel, "Stat", "");
			pStats->pYellowCardText = new Label(pStats->pPanel, "Stat", "");
			pStats->pRedCards = new Label(pStats->pPanel, "Stat", "");
			pStats->pRedCardText = new Label(pStats->pPanel, "Stat", "");
			pStats->pPossession = new Label(pStats->pPanel, "Stat", "");
			pStats->pPossessionText = new Label(pStats->pPanel, "Stat", "");
			pStats->pPing = new Label(pStats->pPanel, "Stat", "");
			pStats->pPingText = new Label(pStats->pPanel, "Stat", "");
			pPos->pStatPanel = pStats;

			m_pPosPanels[i][j] = pPos;
		}
	}
}

void CTeamMenu::ApplySchemeSettings(IScheme *pScheme)
{
	m_pScheme = pScheme;
	//BaseClass::PerformLayout();

	//IScheme *pScheme = scheme()->GetIScheme(GetScheme());

	//MoveToCenterOfScreen();

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i]->SetBounds(0, 0, GetWide(), GetTall());
		m_pTeamPanels[i]->SetBgColor(Color(0, 0, 0, 245));
		m_pTeamPanels[i]->SetPaintBackgroundEnabled(true);
		m_pTeamPanels[i]->SetPaintBackgroundType(2);
		m_pTeamPanels[i]->SetVisible(i == m_nActiveTeam);

		m_pTeamButtons[i]->SetCommand(VarArgs("showteam %d", i));
		m_pTeamButtons[i]->AddActionSignalTarget(this);
		m_pTeamButtons[i]->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
		m_pTeamButtons[i]->SetPaintBorderEnabled(false);
		m_pTeamButtons[i]->SetContentAlignment(i == 0 ? Label::a_west : Label::a_east);

		m_pTeamCrests[i]->SetBounds(i == 0 ? 0 : GetWide() - TEAMCREST_SIZE, TEAMBUTTON_VMARGIN, TEAMCREST_SIZE, TEAMCREST_SIZE);
		m_pTeamCrests[i]->SetShouldScaleImage(true);
		m_pTeamCrests[i]->SetZPos(2);

		//m_pTeamNames[i]->SetBounds(50, 0, 550, 50);
		//m_pTeamNames[i]->SetFgColor(Color(200, 200, 200, 255));
		//m_pTeamNames[i]->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
		//m_pTeamNames[i]->SetZPos(1);
		//m_pTeamNames[i]->SetVisible(false);

		//m_pTeamPossession[i]->SetBounds(50, 0, 550, 50);
		//m_pTeamNames[i]->SetFgColor(Color(200, 200, 200, 255));
		//m_pTeamNames[i]->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
		//m_pTeamNames[i]->SetZPos(1);

		m_pSpectateButton->SetBounds(GetWide() - SPECBUTTON_WIDTH - SPECBUTTON_MARGIN, GetTall() - SPECBUTTON_HEIGHT - SPECBUTTON_MARGIN, SPECBUTTON_WIDTH, SPECBUTTON_HEIGHT);
		m_pSpectateButton->SetCommand(VarArgs("jointeam %d 1", TEAM_SPECTATOR));
		m_pSpectateButton->AddActionSignalTarget(this);
		m_pSpectateButton->SetDefaultColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
		m_pSpectateButton->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
		m_pSpectateButton->SetDepressedColor(Color(100, 100, 100, 255), Color(200, 200, 200, 255));
		m_pSpectateButton->SetCursor(dc_hand);
		m_pSpectateButton->SetFont(pScheme->GetFont("IOSTeamMenuNormal"));
		m_pSpectateButton->SetContentAlignment(Label::a_center);
		m_pSpectateButton->SetZPos(1);

		m_pSpectatorNames->SetBounds(SPECLIST_MARGIN, GetTall() - SPECLIST_HEIGHT, GetWide() - SPECBUTTON_WIDTH, SPECLIST_HEIGHT);
		m_pSpectatorNames->SetFgColor(Color(200, 200, 200, 255));
		m_pSpectatorNames->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
		m_pSpectatorNames->SetZPos(1);

		for (int j = 0; j < 11; j++)
		{
			PosPanel_t *pPos = m_pPosPanels[i][j];

			if (!IsValidPosition(j))
				pPos->pPosPanel->SetVisible(false);
			else
			{
				pPos->pPosPanel->SetBounds(g_Positions[mp_maxplayers.GetInt() - 1][j][0] * (BUTTON_WIDTH + BUTTON_HMARGIN) + BUTTON_LEFTMARGIN, g_Positions[mp_maxplayers.GetInt() - 1][j][1] * (BUTTON_HEIGHT + 2 * BUTTON_VMARGIN) + BUTTON_TOPMARGIN, BUTTON_WIDTH, BUTTON_HEIGHT);
				pPos->pPosPanel->SetPaintBackgroundEnabled(true);
				pPos->pPosPanel->SetPaintBackgroundType(2);
				pPos->pPosPanel->SetBgColor(Color(0, 0, 0, 150));
				pPos->pPosPanel->SetVisible(true);
			}

			pPos->pPlayerName->SetBounds(0, pPos->pPosPanel->GetTall() - 2 * NAME_HEIGHT, pPos->pPosPanel->GetWide(), NAME_HEIGHT);
			pPos->pPlayerName->SetCommand(VarArgs("jointeam %d %d", i + TEAM_A, j));
			pPos->pPlayerName->AddActionSignalTarget(this);
			pPos->pPlayerName->SetPaintBackgroundEnabled(true);
			pPos->pPlayerName->SetPaintBorderEnabled(false);
			//pPos->pPlayerName->SetBorder(pScheme->GetBorder("ButtonBorder"));
			//pPos->pPlayerName->SetBgColor(Color(200, 200, 200, 200));
			pPos->pPlayerName->SetButtonBorderEnabled(false);
			pPos->pPlayerName->SetContentAlignment(Label::a_center);
			//pPos->pPlayerName->SetFont(pScheme->GetFont("IOSTeamMenuBig"));

			pPos->pPosName->SetBounds(0, pPos->pPosPanel->GetTall() - NAME_HEIGHT, POSNAME_WIDTH, NAME_HEIGHT);
			pPos->pPosName->SetFgColor(Color(225, 225, 225, 255));
			pPos->pPosName->SetContentAlignment(Label::a_east);
			pPos->pPosName->SetText(g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][j][2]]);
			pPos->pPosName->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
			pPos->pPosName->SetZPos(1);

			pPos->pClubName->SetBounds(0, pPos->pPosPanel->GetTall() - NAME_HEIGHT, pPos->pPosPanel->GetWide(), NAME_HEIGHT);
			pPos->pClubName->SetContentAlignment(Label::a_center);
			pPos->pClubName->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
			pPos->pClubName->SetFgColor(Color(225, 225, 225, 255));

			pPos->pCountryFlag->SetBounds(pPos->pPosPanel->GetWide() - POSNAME_WIDTH, pPos->pPosPanel->GetTall() - NAME_HEIGHT, POSNAME_WIDTH, NAME_HEIGHT);
			pPos->pCountryFlag->SetShouldScaleImage(true);
			pPos->pCountryFlag->SetZPos(1);

			pPos->pKickButton->SetBounds(pPos->pPosPanel->GetWide() - KICKBUTTON_SIZE, pPos->pPosPanel->GetTall() - 2 * NAME_HEIGHT, KICKBUTTON_SIZE, KICKBUTTON_SIZE);
			pPos->pKickButton->AddActionSignalTarget(this);
			//pPos->pKickButton->SetBgColor(Color(200, 200, 200, 200));
			pPos->pKickButton->SetDefaultColor(Color(0, 0, 0, 200), Color(200, 200, 200, 200));
			pPos->pKickButton->SetArmedColor(Color(50, 50, 50, 200), Color(150, 150, 150, 200));
			pPos->pKickButton->SetDepressedColor(Color(100, 100, 100, 200), Color(200, 200, 200, 200));
			pPos->pKickButton->SetCursor(dc_hand);

			StatPanel_t *pStats = pPos->pStatPanel;

			pStats->pPanel->SetBounds(0, pPos->pPosPanel->GetTall() - 2 * NAME_HEIGHT - 2 * (STATS_VALUEHEIGHT + STATS_TEXTHEIGHT), pPos->pPosPanel->GetWide(), 2 * (STATS_VALUEHEIGHT + STATS_TEXTHEIGHT));

			pStats->pGoals->SetBounds(0, 0, STATS_WIDTH, STATS_VALUEHEIGHT);
			pStats->pGoals->SetFgColor(Color(0, 200, 0, 255));
			pStats->pGoals->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
			pStats->pGoals->SetContentAlignment(Label::a_center);

			pStats->pGoalText->SetBounds(0, STATS_VALUEHEIGHT, STATS_WIDTH, STATS_TEXTHEIGHT);
			pStats->pGoalText->SetFgColor(Color(0, 200, 0, 255));
			pStats->pGoalText->SetFont(pScheme->GetFont("IOSTeamMenuSmall"));
			pStats->pGoalText->SetContentAlignment(Label::a_center);

			pStats->pAssists->SetBounds(STATS_WIDTH, 0, STATS_WIDTH, STATS_VALUEHEIGHT);
			pStats->pAssists->SetFgColor(Color(107, 142, 35, 255));
			pStats->pAssists->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
			pStats->pAssists->SetContentAlignment(Label::a_center);

			pStats->pAssistText->SetBounds(STATS_WIDTH, STATS_VALUEHEIGHT, STATS_WIDTH, STATS_TEXTHEIGHT);
			pStats->pAssistText->SetFgColor(Color(107, 142, 35, 255));
			pStats->pAssistText->SetFont(pScheme->GetFont("IOSTeamMenuSmall"));
			pStats->pAssistText->SetContentAlignment(Label::a_center);

			pStats->pFouls->SetBounds(2 * STATS_WIDTH, 0, STATS_WIDTH, STATS_VALUEHEIGHT);
			pStats->pFouls->SetFgColor(Color(200, 99, 71, 255));
			pStats->pFouls->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
			pStats->pFouls->SetContentAlignment(Label::a_center);

			pStats->pFoulsText->SetBounds(2 * STATS_WIDTH, STATS_VALUEHEIGHT, STATS_WIDTH, STATS_TEXTHEIGHT);
			pStats->pFoulsText->SetFgColor(Color(200, 99, 71, 255));
			pStats->pFoulsText->SetFont(pScheme->GetFont("IOSTeamMenuSmall"));
			pStats->pFoulsText->SetContentAlignment(Label::a_center);

			pStats->pYellowCards->SetBounds(3 * STATS_WIDTH, 0, STATS_WIDTH, STATS_VALUEHEIGHT);
			pStats->pYellowCards->SetFgColor(Color(200, 200, 0, 255));
			pStats->pYellowCards->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
			pStats->pYellowCards->SetContentAlignment(Label::a_center);

			pStats->pYellowCardText->SetBounds(3 * STATS_WIDTH, STATS_VALUEHEIGHT, STATS_WIDTH, STATS_TEXTHEIGHT);
			pStats->pYellowCardText->SetFgColor(Color(200, 200, 0, 255));
			pStats->pYellowCardText->SetFont(pScheme->GetFont("IOSTeamMenuSmall"));
			pStats->pYellowCardText->SetContentAlignment(Label::a_center);

			pStats->pRedCards->SetBounds(4 * STATS_WIDTH, 0, STATS_WIDTH, STATS_VALUEHEIGHT);
			pStats->pRedCards->SetFgColor(Color(200, 0, 0, 255));
			pStats->pRedCards->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
			pStats->pRedCards->SetContentAlignment(Label::a_center);

			pStats->pRedCardText->SetBounds(4 * STATS_WIDTH, STATS_VALUEHEIGHT, STATS_WIDTH, STATS_TEXTHEIGHT);
			pStats->pRedCardText->SetFgColor(Color(200, 0, 0, 255));
			pStats->pRedCardText->SetFont(pScheme->GetFont("IOSTeamMenuSmall"));
			pStats->pRedCardText->SetContentAlignment(Label::a_center);

			pStats->pPing->SetBounds(0, STATS_VALUEHEIGHT + STATS_TEXTHEIGHT, STATS_WIDTH, STATS_VALUEHEIGHT);
			pStats->pPing->SetFgColor(Color(200, 200, 200, 255));
			pStats->pPing->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
			pStats->pPing->SetContentAlignment(Label::a_center);

			pStats->pPingText->SetBounds(0, 2 * STATS_VALUEHEIGHT + STATS_TEXTHEIGHT, STATS_WIDTH, STATS_TEXTHEIGHT);
			pStats->pPingText->SetFgColor(Color(200, 200, 200, 255));
			pStats->pPingText->SetFont(pScheme->GetFont("IOSTeamMenuSmall"));
			pStats->pPingText->SetContentAlignment(Label::a_center);

			pStats->pPossession->SetBounds(STATS_WIDTH, STATS_VALUEHEIGHT + STATS_TEXTHEIGHT, STATS_WIDTH, STATS_VALUEHEIGHT);
			pStats->pPossession->SetFgColor(Color(200, 200, 200, 255));
			pStats->pPossession->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
			pStats->pPossession->SetContentAlignment(Label::a_center);

			pStats->pPossessionText->SetBounds(STATS_WIDTH, 2 * STATS_VALUEHEIGHT + STATS_TEXTHEIGHT, STATS_WIDTH, STATS_TEXTHEIGHT);
			pStats->pPossessionText->SetFgColor(Color(200, 200, 200, 255));
			pStats->pPossessionText->SetFont(pScheme->GetFont("IOSTeamMenuSmall"));
			pStats->pPossessionText->SetContentAlignment(Label::a_center);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTeamMenu::~CTeamMenu()
{
}

#define GET_STAT_TEXT(count, letter) (count > 0 ? VarArgs("%d%s", count, letter) : "0")

//-----------------------------------------------------------------------------
// Purpose: updates the UI with a new map name and map html page, and sets up the team buttons
//-----------------------------------------------------------------------------
void CTeamMenu::OnThink()
{
	//BaseClass::Update();

	Color black = Color(0, 0, 0, 255);
	Color darker = Color(75, 75, 75, 255);
	Color dark = Color(125, 125, 125, 255);
	Color light = Color(175, 175, 175, 255);
	Color lighter = Color(225, 225, 225, 255);
	Color white = Color(255, 255, 255, 255);

	if (m_flNextUpdateTime > gpGlobals->curtime)
		return;

	if (m_nOldMaxPlayers != mp_maxplayers.GetInt())
	{
		m_nOldMaxPlayers = mp_maxplayers.GetInt();
		PerformLayout();
	}

	IGameResources *gr = GameResources();
	if (!gr)
		return;

	char spectatorNames[1024] = "Spectators:   ";
	int spectatorCount = 0;

	bool posTaken[2][11] = {};

	// walk all the players and make sure they're in the scoreboard
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!gr->IsConnected(i))
			continue;

		int team = gr->GetTeam(i);
		int pos = gr->GetTeamPosIndex(i);

		if (gr->GetTeamToJoin(i) != TEAM_INVALID)
			team = gr->GetTeamToJoin(i);

		if (team != TEAM_A && team != TEAM_B)
		{
			if (team == TEAM_SPECTATOR)
			{
				Q_strncat(spectatorNames, VarArgs("%s %s", (spectatorCount == 0 ? "" : ","), gr->GetPlayerName(i)), sizeof(spectatorNames));
				spectatorCount += 1;
			}
			continue;
		}

		posTaken[team - TEAM_A][pos] = true;
		PosPanel_t *pPos = m_pPosPanels[team - TEAM_A][pos];
		if (gr->GetTeamToJoin(i) != TEAM_INVALID)
			pPos->pPlayerName->SetText(VarArgs("%s [%d]", gr->GetPlayerName(i), (int)(gr->GetNextJoin(i) - gpGlobals->curtime)));
		else
			pPos->pPlayerName->SetText(gr->GetPlayerName(i));
		pPos->pPlayerName->SetFgColor(gr->GetTeamColor(team));
		pPos->pPlayerName->SetCursor(gr->IsFakePlayer(i) ? dc_hand : dc_arrow);
		pPos->pPlayerName->SetEnabled(gr->IsFakePlayer(i));
		pPos->pPlayerName->SetDefaultColor(black, lighter);
		pPos->pPlayerName->SetArmedColor(black, white);
		pPos->pPlayerName->SetDepressedColor(black, lighter);
		pPos->pPlayerName->SetDisabledFgColor1(Color(0, 0, 0, 0));
		pPos->pPlayerName->SetDisabledFgColor2(black);
		pPos->pPlayerName->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
		pPos->pClubName->SetText(gr->GetClubName(i));
		pPos->pClubName->SetVisible(true);
		//pPos->pPosName->SetFgColor(Color(0, 0, 0, 255));
		if (UTIL_PlayerByIndex(i))
		{
			pPos->pKickButton->SetCommand(VarArgs("kickid %d", UTIL_PlayerByIndex(i)->GetUserID()));
			pPos->pKickButton->SetVisible(false);
		}

		StatPanel_t *pStats = pPos->pStatPanel;

		pStats->pGoals->SetText(GET_STAT_TEXT(gr->GetGoals(i), ""));
		pStats->pGoals->SetVisible(gr->GetGoals(i) > 0);

		pStats->pGoalText->SetText(gr->GetGoals(i) != 1 ? "GOALS" : "GOAL");
		pStats->pGoalText->SetVisible(gr->GetGoals(i) > 0);

		pStats->pAssists->SetText(GET_STAT_TEXT(gr->GetAssists(i), ""));
		pStats->pAssists->SetVisible(gr->GetAssists(i) > 0);

		pStats->pAssistText->SetText(gr->GetAssists(i) != 1 ? "ASSISTS" : "ASSIST");
		pStats->pAssistText->SetVisible(gr->GetAssists(i) > 0);

		pStats->pFouls->SetText(GET_STAT_TEXT(gr->GetFouls(i), ""));
		pStats->pFouls->SetVisible(gr->GetFouls(i) > 0);

		pStats->pFoulsText->SetText(gr->GetFouls(i) != 1 ? "FOULS" : "FOUL");
		pStats->pFoulsText->SetVisible(gr->GetFouls(i) > 0);

		pStats->pYellowCards->SetText(GET_STAT_TEXT(gr->GetYellowCards(i), ""));
		pStats->pYellowCards->SetVisible(gr->GetYellowCards(i) > 0);

		pStats->pYellowCardText->SetText(gr->GetYellowCards(i) != 1 ? "YELLOWS" : "YELLOW");
		pStats->pYellowCardText->SetVisible(gr->GetYellowCards(i) > 0);

		pStats->pRedCards->SetText(GET_STAT_TEXT(gr->GetRedCards(i), ""));
		pStats->pRedCards->SetVisible(gr->GetRedCards(i) > 0);

		pStats->pRedCardText->SetText(gr->GetRedCards(i) != 1 ? "REDS" : "RED");
		pStats->pRedCardText->SetVisible(gr->GetRedCards(i) > 0);

		pStats->pPossession->SetText(VarArgs("%d", gr->GetPossession(i)));
		pStats->pPossession->SetVisible(true);

		pStats->pPossessionText->SetText("POSS.");
		pStats->pPossessionText->SetVisible(true);

		pStats->pPing->SetText(VarArgs("%d", gr->GetPing(i)));
		pStats->pPing->SetVisible(true);

		pStats->pPingText->SetText("PING");
		pStats->pPingText->SetVisible(true);

		if (Q_strlen(gr->GetCountryName(i)) > 0)
		{
			ITexture *pTex = materials->FindTexture(VarArgs("vgui/countryflags/%s", gr->GetCountryName(i)), NULL, false);
			if (!pTex->IsError())
			{
				pPos->pCountryFlag->SetImage(VarArgs("countryflags/%s", gr->GetCountryName(i)));
				pPos->pCountryFlag->SetVisible(true);
			}
			else
				pPos->pCountryFlag->SetVisible(false);
			//steamapicontext->SteamUtils()->GetIPCountry()
		}
		else
			pPos->pCountryFlag->SetVisible(false);
	}

	m_pTeamPanels[m_nActiveTeam]->SetVisible(true);

	m_pTeamButtons[m_nActiveTeam]->SetDefaultColor(black, lighter);
	m_pTeamButtons[m_nActiveTeam]->SetArmedColor(black, lighter);
	m_pTeamButtons[m_nActiveTeam]->SetDepressedColor(black, lighter);
	m_pTeamButtons[m_nActiveTeam]->SetCursor(dc_arrow);
	m_pTeamButtons[m_nActiveTeam]->SetBounds(TEAMCREST_SIZE, TEAMBUTTON_VMARGIN, GetWide() - 2 * TEAMCREST_SIZE, TEAMBUTTON_HEIGHT + 5);
	m_pTeamButtons[m_nActiveTeam]->SetZPos(1);

	m_pTeamPanels[1 - m_nActiveTeam]->SetVisible(false);

	m_pTeamButtons[1 - m_nActiveTeam]->SetDefaultColor(black, light);
	m_pTeamButtons[1 - m_nActiveTeam]->SetArmedColor(black, lighter);
	m_pTeamButtons[1 - m_nActiveTeam]->SetDepressedColor(black, light);
	m_pTeamButtons[1 - m_nActiveTeam]->SetCursor(dc_hand);
	m_pTeamButtons[1 - m_nActiveTeam]->SetZPos(2);
	m_pTeamButtons[1 - m_nActiveTeam]->SetBounds(1 - m_nActiveTeam == 0 ? TEAMCREST_SIZE : GetWide() / 2, TEAMBUTTON_VMARGIN, GetWide() / 2 - TEAMCREST_SIZE, TEAMBUTTON_HEIGHT);

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			if (posTaken[i][j])
				continue;
			
			PosPanel_t *pPos = m_pPosPanels[i][j];
			pPos->pPlayerName->SetText("JOIN");
			pPos->pPlayerName->SetCursor(dc_hand);
			pPos->pPlayerName->SetEnabled(true);
			//pPos->pPlayerName->SetFgColor(gr->GetTeamColor(TEAM_UNASSIGNED));
			pPos->pPlayerName->SetDefaultColor(black, darker);
			pPos->pPlayerName->SetArmedColor(black, dark);
			pPos->pPlayerName->SetDepressedColor(black, darker);
			pPos->pPlayerName->SetFont(m_pScheme->GetFont("IOSTeamMenuBigBold"));
			pPos->pClubName->SetVisible(false);
			pPos->pCountryFlag->SetVisible(false);
			pPos->pKickButton->SetVisible(false);

			StatPanel_t *pStats = pPos->pStatPanel;
			pStats->pGoals->SetVisible(false);
			pStats->pGoalText->SetVisible(false);
			pStats->pAssists->SetVisible(false);
			pStats->pAssistText->SetVisible(false);
			pStats->pFouls->SetVisible(false);
			pStats->pFoulsText->SetVisible(false);
			pStats->pYellowCards->SetVisible(false);
			pStats->pYellowCardText->SetVisible(false);
			pStats->pRedCards->SetVisible(false);
			pStats->pRedCardText->SetVisible(false);
			pStats->pPossession->SetVisible(false);
			pStats->pPossessionText->SetVisible(false);
			pStats->pPing->SetVisible(false);
			pStats->pPingText->SetVisible(false);
		}
	}

	//m_pTeamButtons[0]->SetText(VarArgs("%-40s %2d %-7s %17d%% poss. %17d", pTeamA->Get_Name(), pTeamA->Get_Number_Players(), (pTeamA->Get_Number_Players() == 1 ? "player" : "players"), pTeamA->Get_Possession(), pTeamA->Get_Goals()));
	//m_pTeamButtons[1]->SetText(VarArgs("   %-13d %3d%% poss. %13d %-7s %40s", pTeamB->Get_Goals(), pTeamB->Get_Possession(), pTeamB->Get_Number_Players(), (pTeamB->Get_Number_Players() == 1 ? "player" : "players"), pTeamB->Get_Name()));

	for (int team = TEAM_A; team <= TEAM_B; team++)
	{
		int index = team - TEAM_A;

		m_pTeamButtons[index]->SetText(gr->GetFullTeamName(team));

		ITexture *pTex = materials->FindTexture(VarArgs("vgui/teamcrests/%s", gr->GetTeamKitName(team)), NULL, false);
		if (!pTex->IsError())
		{
			m_pTeamCrests[index]->SetImage(VarArgs("teamcrests/%s", gr->GetTeamKitName(team)));
			m_pTeamCrests[index]->SetVisible(true);
		}
		else
			m_pTeamCrests[index]->SetVisible(false);

	}

	//m_pTeamButtons[0]->SetText(gr->GetFullTeamName(TEAM_A));
	//m_pTeamButtons[1]->SetText(gr->GetFullTeamName(TEAM_B));

	m_pSpectatorNames->SetText(spectatorNames);

	m_flNextUpdateTime = gpGlobals->curtime + 0.25f;
}

//-----------------------------------------------------------------------------
// IOS Added
//-----------------------------------------------------------------------------
void CTeamMenu::OnCommand( char const *cmd )
{
	if (!strnicmp(cmd, "jointeam", 8))
	{
		engine->ClientCmd(cmd);
	}
	else if (!stricmp(cmd, "showteam 0"))
		m_nActiveTeam = 0;
	else if (!stricmp(cmd, "showteam 1"))
		m_nActiveTeam = 1;
	else if (!strnicmp(cmd, "kickid", 6))
	{
		engine->ClientCmd(cmd);
	}
	//Close();

	BaseClass::OnCommand(cmd);

	m_flNextUpdateTime = gpGlobals->curtime;
}
