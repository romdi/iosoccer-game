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

#define PANEL_MARGIN		5
#define PANEL_WIDTH			(1024 - 2 * PANEL_MARGIN)
#define PANEL_HEIGHT		(768 - 2 * PANEL_MARGIN)
#define BUTTON_WIDTH		230
#define BUTTON_HEIGHT		130
#define BUTTON_HMARGIN		15
#define BUTTON_VMARGIN		10
#define BUTTON_LEFTMARGIN	25
#define BUTTON_TOPMARGIN	70
#define NUMBER_MARGIN		2
#define NUMBER_WIDTH		30
#define NUMBER_HEIGHT		40
#define NAME_HEIGHT			30
#define INFO_WIDTH			20
#define INFO_HEIGHT			30
#define IMAGE_SIZE			16
#define	STATS_WIDTH			45
#define	STATS_VALUEHEIGHT	18
#define	STATS_TEXTHEIGHT	15
#define	STATS_MARGIN		5
#define	KICKBUTTON_SIZE		20
#define TABBUTTON_HEIGHT	40
#define TABBUTTON_WIDTH		(PANEL_WIDTH / 2)
#define TABBUTTON_MARGIN	7
#define SPECLIST_HEIGHT		50
#define SPECLIST_MARGIN		10
#define SPECBUTTON_WIDTH	90
#define SPECBUTTON_HEIGHT	35
#define SPECBUTTON_MARGIN	7

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTeamMenu::CTeamMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_TEAM )
{
	m_pViewPort = pViewPort;
	m_iJumpKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_nGoalsBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_nActiveTeam = 0;
	m_nOldMaxPlayers = 11;

	SetBounds(PANEL_MARGIN, PANEL_MARGIN, PANEL_WIDTH, PANEL_HEIGHT);
	SetTitle("", true);
	SetMoveable(false);
	SetSizeable(false);
	SetTitleBarVisible( false );
	SetProportional(false);
	SetPaintBackgroundEnabled(false);
	//SetPaintBackgroundType(2);
	SetPaintBorderEnabled(false);

	m_pSpectateButton = new Button(this, "SpectateButton", "Spectate");
	m_pSpectatorNames = new Label(this, "Spectators", "");

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i] = new Panel(this, VarArgs("TeamPanel%d", i));
		m_pTeamNames[i] = new Label(m_pTeamPanels[i], VarArgs("TeamLabel%d", i), "");
		m_pTeamPossession[i] = new Label(m_pTeamPanels[i], "", "");
		m_pTeamPlayerCount[i] = new Label(m_pTeamPanels[i], "", "");
		m_pTabButtons[i] = new Button(this, "TabButton", VarArgs("Team%d", i + 1));

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

void CTeamMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	IScheme *pScheme = scheme()->GetIScheme(GetScheme());

	MoveToCenterOfScreen();

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i]->SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
		m_pTeamPanels[i]->SetBgColor(Color(0, 0, 0, 245));
		m_pTeamPanels[i]->SetPaintBackgroundEnabled(true);
		m_pTeamPanels[i]->SetPaintBackgroundType(2);
		m_pTeamPanels[i]->SetVisible(i == m_nActiveTeam);

		m_pTabButtons[i]->SetBounds(i * TABBUTTON_WIDTH, TABBUTTON_MARGIN, TABBUTTON_WIDTH, TABBUTTON_HEIGHT);
		m_pTabButtons[i]->SetCommand(VarArgs("showteam %d", i));
		m_pTabButtons[i]->AddActionSignalTarget(this);
		m_pTabButtons[i]->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
		m_pTabButtons[i]->SetPaintBorderEnabled(false);
		m_pTabButtons[i]->SetContentAlignment(i == 0 ? Label::a_west : Label::a_east);
		m_pTabButtons[i]->SetZPos(1);

		//m_pTeamNames[i]->SetBounds(50, 0, 550, 50);
		//m_pTeamNames[i]->SetFgColor(Color(200, 200, 200, 255));
		//m_pTeamNames[i]->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
		//m_pTeamNames[i]->SetZPos(1);
		//m_pTeamNames[i]->SetVisible(false);

		//m_pTeamPossession[i]->SetBounds(50, 0, 550, 50);
		//m_pTeamNames[i]->SetFgColor(Color(200, 200, 200, 255));
		//m_pTeamNames[i]->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
		//m_pTeamNames[i]->SetZPos(1);

		m_pSpectateButton->SetBounds(PANEL_WIDTH - SPECBUTTON_WIDTH - SPECBUTTON_MARGIN, PANEL_HEIGHT - SPECBUTTON_HEIGHT - SPECBUTTON_MARGIN, SPECBUTTON_WIDTH, SPECBUTTON_HEIGHT);
		m_pSpectateButton->SetCommand(VarArgs("jointeam %d 1", TEAM_SPECTATOR));
		m_pSpectateButton->AddActionSignalTarget(this);
		m_pSpectateButton->SetDefaultColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
		m_pSpectateButton->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
		m_pSpectateButton->SetDepressedColor(Color(100, 100, 100, 255), Color(200, 200, 200, 255));
		m_pSpectateButton->SetCursor(dc_hand);
		m_pSpectateButton->SetFont(pScheme->GetFont("IOSTeamMenuNormal"));
		m_pSpectateButton->SetContentAlignment(Label::a_center);
		m_pSpectateButton->SetZPos(1);

		m_pSpectatorNames->SetBounds(SPECLIST_MARGIN, PANEL_HEIGHT - SPECLIST_HEIGHT, PANEL_WIDTH - SPECBUTTON_WIDTH, SPECLIST_HEIGHT);
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
			pPos->pPlayerName->SetFont(pScheme->GetFont("IOSTeamMenuBig"));

			pPos->pPosName->SetBounds(0, pPos->pPosPanel->GetTall() - 2 * NAME_HEIGHT, NUMBER_WIDTH, NAME_HEIGHT);
			pPos->pPosName->SetContentAlignment(Label::a_east);
			pPos->pPosName->SetText(g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][j][2]]);
			pPos->pPosName->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
			pPos->pPosName->SetZPos(1);

			pPos->pClubName->SetBounds(0, pPos->pPosPanel->GetTall() - NAME_HEIGHT, pPos->pPosPanel->GetWide(), NAME_HEIGHT);
			pPos->pClubName->SetContentAlignment(Label::a_center);
			pPos->pClubName->SetFont(pScheme->GetFont("IOSTeamMenuBig"));
			pPos->pClubName->SetFgColor(Color(200, 200, 200, 255));

			pPos->pCountryFlag->SetBounds(pPos->pPosPanel->GetWide() - NUMBER_WIDTH, pPos->pPosPanel->GetTall() - 2 * NAME_HEIGHT, NUMBER_WIDTH, NAME_HEIGHT);
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

void CTeamMenu::SetData(KeyValues *data)
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTeamMenu::~CTeamMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets the text color of the map description field
//-----------------------------------------------------------------------------
void CTeamMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	
}

//-----------------------------------------------------------------------------
// Purpose: makes the user choose the auto assign option
//-----------------------------------------------------------------------------
void CTeamMenu::AutoAssign()
{
	engine->ClientCmd("jointeam 0");
	//OnClose();
}

void CTeamMenu::Reset()
{
	m_flNextUpdateTime = gpGlobals->curtime;
}

bool CTeamMenu::NeedsUpdate()
{
	return m_flNextUpdateTime <= gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: shows the team menu
//-----------------------------------------------------------------------------
void CTeamMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();
		Reset();
		Update();
		SetMouseInputEnabled( true );
		SetKeyBoardInputEnabled( true );
		// get key bindings if shown

		if( m_iJumpKey == BUTTON_CODE_INVALID ) // you need to lookup the jump key AFTER the engine has loaded
		{
			m_iJumpKey = gameuifuncs->GetButtonCodeForBind( "jump" );
		}

		if ( m_nGoalsBoardKey == BUTTON_CODE_INVALID ) 
		{
			m_nGoalsBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );
		}
		
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
	}
}

#define GET_STAT_TEXT(count, letter) (count > 0 ? VarArgs("%d%s", count, letter) : "0")

//-----------------------------------------------------------------------------
// Purpose: updates the UI with a new map name and map html page, and sets up the team buttons
//-----------------------------------------------------------------------------
void CTeamMenu::Update()
{
	//BaseClass::Update();

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
		pPos->pPlayerName->SetDefaultColor(Color(0, 0, 0, 255), Color(150, 150, 150, 255));
		pPos->pPlayerName->SetArmedColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
		pPos->pPlayerName->SetDepressedColor(Color(0, 0, 0, 255), Color(150, 150, 150, 255));
		pPos->pPlayerName->SetDisabledFgColor1(Color(0, 0, 0, 0));
		pPos->pPlayerName->SetDisabledFgColor2(Color(0, 0, 0, 255));
		pPos->pClubName->SetText(gr->GetClubName(i));
		pPos->pClubName->SetVisible(true);
		pPos->pPosName->SetFgColor(Color(0, 0, 0, 255));
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

	m_pTabButtons[m_nActiveTeam]->SetDefaultColor(Color(200, 200, 200, 255), Color(0, 0, 0, 0));
	m_pTabButtons[m_nActiveTeam]->SetArmedColor(Color(200, 200, 200, 255), Color(0, 0, 0, 0));
	m_pTabButtons[m_nActiveTeam]->SetDepressedColor(Color(200, 200, 200, 255), Color(0, 0, 0, 0));
	m_pTabButtons[m_nActiveTeam]->SetCursor(dc_arrow);

	m_pTeamPanels[1 - m_nActiveTeam]->SetVisible(false);

	m_pTabButtons[1 - m_nActiveTeam]->SetDefaultColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
	m_pTabButtons[1 - m_nActiveTeam]->SetArmedColor(Color(0, 0, 0, 255), Color(150, 150, 150, 255));
	m_pTabButtons[1 - m_nActiveTeam]->SetDepressedColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
	m_pTabButtons[1 - m_nActiveTeam]->SetCursor(dc_hand);

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
			pPos->pPlayerName->SetDefaultColor(Color(200, 200, 200, 255), Color(50, 50, 50, 255));
			pPos->pPlayerName->SetArmedColor(Color(200, 200, 200, 255), Color(100, 100, 100, 255));
			pPos->pPlayerName->SetDepressedColor(Color(200, 200, 200, 255), Color(50, 50, 50, 255));
			pPos->pClubName->SetVisible(false);
			pPos->pPosName->SetFgColor(Color(200, 200, 200, 255));
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
	C_Team *pTeamA = GetGlobalTeam(TEAM_A);
	m_pTabButtons[0]->SetText(VarArgs("%-40s %2d %-7s %17d%% poss. %17d", pTeamA->Get_Name(), pTeamA->Get_Number_Players(), (pTeamA->Get_Number_Players() == 1 ? "player" : "players"), pTeamA->Get_Possession(), pTeamA->Get_Goals()));
	C_Team *pTeamB = GetGlobalTeam(TEAM_B);
	m_pTabButtons[1]->SetText(VarArgs("   %-13d %3d%% poss. %13d %-7s %40s", pTeamB->Get_Goals(), pTeamB->Get_Possession(), pTeamB->Get_Number_Players(), (pTeamB->Get_Number_Players() == 1 ? "player" : "players"), pTeamB->Get_Name()));

	//m_pTabButtons[0]->SetText(gr->GetFullTeamName(TEAM_A));
	//m_pTabButtons[1]->SetText(gr->GetFullTeamName(TEAM_B));

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

void CTeamMenu::OnKeyCodePressed(KeyCode code)
{
	//if( m_iJumpKey != BUTTON_CODE_INVALID && m_iJumpKey == code )
	//{
	//	AutoAssign();
	//}
	//else if ( m_nGoalsBoardKey != BUTTON_CODE_INVALID && m_nGoalsBoardKey == code )
	//{
	//	//gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
	//	//gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
	//	//Close();
	//}
	//else
	//{
	//	BaseClass::OnKeyCodePressed( code );
	//}

	if (code == KEY_TAB)
		Close();
	else
		BaseClass::OnKeyCodePressed( code );
}
