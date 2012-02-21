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
#include "steam\steam_api.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

#define PANEL_MARGIN		5
#define PANEL_WIDTH			1024 - 2 * PANEL_MARGIN
#define PANEL_HEIGHT		768 - 2 * PANEL_MARGIN
#define BUTTON_WIDTH		200
#define BUTTON_HEIGHT		120
#define BUTTON_MARGIN		20
#define NUMBER_MARGIN		2
#define NUMBER_WIDTH		30
#define NUMBER_HEIGHT		40
#define NAME_WIDTH			BUTTON_SIZE - NUMBER_WIDTH - NUMBER_MARGIN
#define NAME_HEIGHT			30
#define INFO_WIDTH			20
#define INFO_HEIGHT			30
#define IMAGE_SIZE			60
#define	STATS_WIDTH			30
#define	STATS_HEIGHT		30
#define	STATS_MARGIN		5
#define	KICKBUTTON_SIZE		20
#define TABBUTTON_HEIGHT	30
#define TABBUTTON_WIDTH		100
#define TABBUTTON_MARGIN	5

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTeamMenu::CTeamMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_TEAM )
{
	m_pViewPort = pViewPort;
	m_iJumpKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_nActiveTeam = 0;

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
	m_pToggleStats = new Button(this, "Stats", "Show Stats");
	m_bShowStats = false;

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i] = new Panel(this, VarArgs("TeamPanel%d", i));
		m_pTeamNames[i] = new Label(m_pTeamPanels[i], VarArgs("TeamLabel%d", i), "");
		m_szTeamNames[i][0] = 0;
		m_pTabButtons[i] = new Button(this, "TabButton", VarArgs("Team%d", i + 1));

		for (int j = 0; j < 11; j++)
		{
			PosPanel_t *pPos = new PosPanel_t;
			pPos->pPosPanel = new Panel(m_pTeamPanels[i]);
			pPos->pPlayerImage = new ImagePanel(pPos->pPosPanel, "Image");
			pPos->pPosInfo = new Label(pPos->pPosPanel, "Label", "");
			pPos->pPlayerName = new Button(pPos->pPosPanel, "Button", "");
			pPos->pClubName = new Label(pPos->pPosPanel, "Label", "");
			pPos->pPosNumber = new Label(pPos->pPosPanel, "Label", "");
			pPos->pPosName = new Label(pPos->pPosPanel, "Label", "");

			StatPanel_t *pStats = new StatPanel_t;
			pStats->pPanel = new Panel(pPos->pPosPanel);
			pStats->pGoals = new Label(pStats->pPanel, "Stat", "");
			pStats->pAssists = new Label(pStats->pPanel, "Stat", "");
			pStats->pYellows = new Label(pStats->pPanel, "Stat", "");
			pStats->pReds = new Label(pStats->pPanel, "Stat", "");
			pStats->pPossession = new Label(pStats->pPanel, "Stat", "");
			pStats->pPing = new Label(pStats->pPanel, "Stat", "");
			pStats->pCountryFlag = new ImagePanel(pStats->pPanel, "Stat");
			pPos->pStatPanel = pStats;

			pPos->pKickButton = new Button(pPos->pPosPanel, "KickButton", "x");

			m_pPosPanels[i][j] = pPos;
		}
	}
}

void CTeamMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	IScheme *pScheme = scheme()->GetIScheme(GetScheme());

	MoveToCenterOfScreen();

	float pos[11][2] = {
				{ 0.5f, 0 }, { 1.5f, 0 }, { 2.5f, 0 },
				{ 0.5f, 1 }, { 1.5f, 1 }, { 2.5f, 1 },
			{ 0, 2 }, { 1, 2 }, { 2, 2 }, { 3, 2 },
							  { 1.5f, 3 }
	};

	for(int i = 0; i < 2; i++)
	{
		m_pTabButtons[i]->SetBounds(TABBUTTON_MARGIN + i * (TABBUTTON_WIDTH + TABBUTTON_MARGIN), 0, TABBUTTON_WIDTH, TABBUTTON_HEIGHT);
		m_pTabButtons[i]->SetCommand(VarArgs("showteam %d", i));
		m_pTabButtons[i]->AddActionSignalTarget(this);
		m_pTabButtons[i]->SetBgColor(Color(255, 255, 255, 255));
		m_pTabButtons[i]->SetDefaultColor(Color(0, 0, 0, 255), Color(255, 255, 255, 255));
		m_pTabButtons[i]->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
		m_pTabButtons[i]->SetDepressedColor(Color(100, 100, 100, 255), Color(255, 255, 255, 255));
		m_pTabButtons[i]->SetCursor(dc_hand);

		m_pSpectateButton->SetBounds(TABBUTTON_MARGIN + 3 * (TABBUTTON_WIDTH + TABBUTTON_MARGIN), 0, TABBUTTON_WIDTH, TABBUTTON_HEIGHT);
		m_pSpectateButton->SetCommand(VarArgs("jointeam %d 1", TEAM_SPECTATOR));
		m_pSpectateButton->AddActionSignalTarget(this);
		m_pSpectateButton->SetBgColor(Color(255, 255, 255, 255));
		m_pSpectateButton->SetDefaultColor(Color(0, 0, 0, 255), Color(255, 255, 255, 255));
		m_pSpectateButton->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
		m_pSpectateButton->SetDepressedColor(Color(100, 100, 100, 255), Color(255, 255, 255, 255));
		m_pSpectateButton->SetCursor(dc_hand);

		m_pToggleStats->SetBounds(TABBUTTON_MARGIN + 4 * (TABBUTTON_WIDTH + TABBUTTON_MARGIN), 0, TABBUTTON_WIDTH, TABBUTTON_HEIGHT);
		m_pToggleStats->SetCommand("togglestats");
		m_pToggleStats->AddActionSignalTarget(this);
		m_pToggleStats->SetBgColor(Color(255, 255, 255, 255));
		m_pToggleStats->SetDefaultColor(Color(0, 0, 0, 255), Color(255, 255, 255, 255));
		m_pToggleStats->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
		m_pToggleStats->SetDepressedColor(Color(100, 100, 100, 255), Color(255, 255, 255, 255));
		m_pToggleStats->SetVisible(false);

		m_pTeamNames[i]->SetBounds(50, 0, 550, 50);
		//m_pTeamNames[i]->SetTextInset(50, 10);
		//m_pTeamNames[i]->SetPinCorner(Panel::PIN_TOPRIGHT, 10, 10);
		m_pTeamNames[i]->SetFgColor(Color(255, 255, 255, 255));
		m_pTeamNames[i]->SetFont(pScheme->GetFont("IOSScorebar"));

		m_pTeamPanels[i]->SetBounds(0, TABBUTTON_HEIGHT, PANEL_WIDTH, PANEL_HEIGHT - TABBUTTON_HEIGHT);
		m_pTeamPanels[i]->SetBgColor(Color(0, 0, 0, 230));
		m_pTeamPanels[i]->SetPaintBackgroundEnabled(true);
		m_pTeamPanels[i]->SetPaintBackgroundType(2);
		m_pTeamPanels[i]->SetVisible(i == m_nActiveTeam);

		for(int j = 0; j < 11; j++)
		{
			PosPanel_t *pPos = m_pPosPanels[i][j];

			pPos->pPosPanel->SetBounds(pos[j][0] * (BUTTON_WIDTH + BUTTON_MARGIN) + 20, pos[j][1] * (BUTTON_HEIGHT + 2 * BUTTON_MARGIN) + 50, BUTTON_WIDTH, BUTTON_HEIGHT);
			
			pPos->pPlayerImage->SetBounds(pPos->pPosPanel->GetWide() / 2 - IMAGE_SIZE / 2, pPos->pPosPanel->GetTall() / 2 - IMAGE_SIZE / 2 - 2 * NAME_HEIGHT, IMAGE_SIZE, IMAGE_SIZE);
			pPos->pPlayerImage->SetImage("shirt");
			pPos->pPlayerImage->SetShouldScaleImage(true);
			pPos->pPlayerImage->SetVisible(false);

			int possession = 10;
			int goals = 2;
			int assists = 3;
			int fouls = 1;
			int yellows = 1;
			int reds = 1;
			pPos->pPosInfo->SetBounds(0, INFO_HEIGHT, pPos->pPosPanel->GetWide(), INFO_HEIGHT);
			pPos->pPosInfo->SetText(VarArgs("%d%%P/%dG/%dA/%dF/%dY/%dR", possession, goals, assists, fouls, yellows, reds));
			pPos->pPosInfo->SetContentAlignment(Label::a_center);
			pPos->pPosInfo->SetFont(pScheme->GetFont("IOSScorebar"));
			pPos->pPosInfo->SetVisible(false);

			pPos->pPlayerName->SetBounds(0, pPos->pPosPanel->GetTall() - 2 * NAME_HEIGHT, pPos->pPosPanel->GetWide(), NAME_HEIGHT);
			pPos->pPlayerName->SetCommand(VarArgs("jointeam %d %d", i + 2, 11 - j));
			pPos->pPlayerName->AddActionSignalTarget(this);
			pPos->pPlayerName->SetPaintBackgroundEnabled(true);
			pPos->pPlayerName->SetPaintBorderEnabled(false);
			//pPos->pPlayerName->SetBorder(pScheme->GetBorder("ButtonBorder"));
			pPos->pPlayerName->SetBgColor(Color(255, 255, 255, 200));
			pPos->pPlayerName->SetDefaultColor(Color(0, 0, 0, 200), Color(255, 255, 255, 200));
			pPos->pPlayerName->SetArmedColor(Color(50, 50, 50, 200), Color(150, 150, 150, 200));
			pPos->pPlayerName->SetDepressedColor(Color(100, 100, 100, 200), Color(255, 255, 255, 200));
			pPos->pPlayerName->SetButtonBorderEnabled(false);
			pPos->pPlayerName->SetContentAlignment(Label::a_center);
			pPos->pPlayerName->SetFont(pScheme->GetFont("IOSScorebar"));

			pPos->pPosName->SetBounds(0, pPos->pPosPanel->GetTall() - NAME_HEIGHT, NUMBER_WIDTH, NAME_HEIGHT);
			pPos->pPosName->SetText(g_szPosNames[10 - j]);
			pPos->pPosName->SetFont(pScheme->GetFont("IOSScorebar"));
			pPos->pPosName->SetFgColor(Color(255, 255, 255, 255));

			pPos->pClubName->SetBounds(0, pPos->pPosPanel->GetTall() - NAME_HEIGHT, pPos->pPosPanel->GetWide(), NAME_HEIGHT);
			pPos->pClubName->SetContentAlignment(Label::a_center);
			pPos->pClubName->SetFont(pScheme->GetFont("IOSScorebar"));
			pPos->pClubName->SetFgColor(Color(255, 255, 255, 255));

			pPos->pPosNumber->SetBounds(pPos->pPosPanel->GetWide() - NUMBER_WIDTH, pPos->pPosPanel->GetTall() - NAME_HEIGHT, NUMBER_WIDTH, NAME_HEIGHT);
			pPos->pPosNumber->SetText(VarArgs("%d", 11 - j));
			pPos->pPosNumber->SetFont(pScheme->GetFont("IOSScorebar"));
			pPos->pPosNumber->SetFgColor(Color(255, 255, 255, 255));

			StatPanel_t *pStats = pPos->pStatPanel;

			pStats->pPanel->SetBounds(0, pPos->pPosPanel->GetTall() - 2 * NAME_HEIGHT - (2 * STATS_HEIGHT + STATS_MARGIN), pPos->pPosPanel->GetWide(), 2 * STATS_HEIGHT);

			pStats->pGoals->SetBounds(STATS_MARGIN, 0, STATS_WIDTH, STATS_HEIGHT);
			pStats->pGoals->SetFgColor(Color(0, 0, 0, 255));
			pStats->pGoals->SetBgColor(Color(245, 245, 245, 255));
			pStats->pGoals->SetFont(pScheme->GetFont("IOSScorebar"));
			pStats->pGoals->SetContentAlignment(Label::a_center);

			pStats->pAssists->SetBounds(STATS_MARGIN + STATS_WIDTH + STATS_MARGIN, 0, STATS_WIDTH, STATS_HEIGHT);
			pStats->pAssists->SetFgColor(Color(0, 0, 0, 255));
			pStats->pAssists->SetBgColor(Color(176, 196, 222, 255));
			pStats->pAssists->SetFont(pScheme->GetFont("IOSScorebar"));
			pStats->pAssists->SetContentAlignment(Label::a_center);

			pStats->pYellows->SetBounds(STATS_MARGIN + 2 * (STATS_WIDTH + STATS_MARGIN), 0, STATS_WIDTH, STATS_HEIGHT);
			pStats->pYellows->SetFgColor(Color(0, 0, 0, 255));
			pStats->pYellows->SetBgColor(Color(240, 230, 140, 255));
			pStats->pYellows->SetFont(pScheme->GetFont("IOSScorebar"));
			pStats->pYellows->SetContentAlignment(Label::a_center);

			pStats->pReds->SetBounds(STATS_MARGIN + 3 * (STATS_WIDTH + STATS_MARGIN), 0, STATS_WIDTH, STATS_HEIGHT);
			pStats->pReds->SetFgColor(Color(0, 0, 0, 255));
			pStats->pReds->SetBgColor(Color(250, 128, 114, 255));
			pStats->pReds->SetFont(pScheme->GetFont("IOSScorebar"));
			pStats->pReds->SetContentAlignment(Label::a_center);

			pStats->pPossession->SetBounds(STATS_MARGIN + 4 * (STATS_WIDTH + STATS_MARGIN), 0, STATS_WIDTH, STATS_HEIGHT);
			pStats->pPossession->SetFgColor(Color(0, 0, 0, 255));
			pStats->pPossession->SetBgColor(Color(255, 255, 255, 255));
			pStats->pPossession->SetFont(pScheme->GetFont("IOSScorebar"));
			pStats->pPossession->SetContentAlignment(Label::a_center);

			pStats->pPing->SetBounds(STATS_MARGIN, STATS_HEIGHT + STATS_MARGIN, STATS_WIDTH, STATS_HEIGHT);
			pStats->pPing->SetFgColor(Color(0, 0, 0, 255));
			pStats->pPing->SetBgColor(Color(255, 255, 255, 255));
			pStats->pPing->SetFont(pScheme->GetFont("IOSScorebar"));
			pStats->pPing->SetContentAlignment(Label::a_center);

			pStats->pCountryFlag->SetBounds(STATS_MARGIN + STATS_WIDTH + STATS_MARGIN, STATS_HEIGHT + STATS_MARGIN, STATS_WIDTH, STATS_HEIGHT);
			pStats->pCountryFlag->SetShouldScaleImage(true);

			pPos->pKickButton->SetBounds(pPos->pPosPanel->GetWide() - KICKBUTTON_SIZE, 0, KICKBUTTON_SIZE, KICKBUTTON_SIZE);
			pPos->pKickButton->AddActionSignalTarget(this);
			pPos->pKickButton->SetBgColor(Color(255, 255, 255, 200));
			pPos->pKickButton->SetDefaultColor(Color(0, 0, 0, 200), Color(255, 255, 255, 200));
			pPos->pKickButton->SetArmedColor(Color(50, 50, 50, 200), Color(150, 150, 150, 200));
			pPos->pKickButton->SetDepressedColor(Color(100, 100, 100, 200), Color(255, 255, 255, 200));
			pPos->pKickButton->SetCursor(dc_hand);
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
		// get key bindings if shown

		if( m_iJumpKey == BUTTON_CODE_INVALID ) // you need to lookup the jump key AFTER the engine has loaded
		{
			m_iJumpKey = gameuifuncs->GetButtonCodeForBind( "jump" );
		}

		if ( m_iScoreBoardKey == BUTTON_CODE_INVALID ) 
		{
			m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );
		}
		
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

#define GET_STAT_TEXT(count) (count > 0 ? VarArgs("%d", count) : "")

//-----------------------------------------------------------------------------
// Purpose: updates the UI with a new map name and map html page, and sets up the team buttons
//-----------------------------------------------------------------------------
void CTeamMenu::Update()
{
	//BaseClass::Update();

	IGameResources *gr = GameResources();
	if (!gr)
		return;

	bool posTaken[2][11] = {};

	// walk all the players and make sure they're in the scoreboard
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!gr->IsConnected(i))
			continue;

		int team = gr->GetTeam(i);
		int pos = gr->GetTeamPosition(i);

		if (gr->GetTeamToJoin(i) != TEAM_INVALID)
			team = gr->GetTeamToJoin(i);

		if (team != TEAM_A && team != TEAM_B)
			continue;

		posTaken[team - TEAM_A][11 - pos] = true;
		PosPanel_t *pPos = m_pPosPanels[team - TEAM_A][11 - pos];
		if (gr->GetTeamToJoin(i) != TEAM_INVALID)
			pPos->pPlayerName->SetText(VarArgs("%s (%d)", gr->GetPlayerName(i), (int)(gr->GetNextJoin(i) - gpGlobals->curtime)));
		else
			pPos->pPlayerName->SetText(gr->GetPlayerName(i));
		pPos->pPlayerName->SetFgColor(gr->GetTeamColor(team));
		pPos->pPlayerName->SetCursor(gr->IsFakePlayer(i) ? dc_hand : dc_arrow);
		pPos->pPlayerName->SetEnabled(gr->IsFakePlayer(i));
		pPos->pClubName->SetText(gr->GetClubName(i));
		if (UTIL_PlayerByIndex(i))
		{
			pPos->pKickButton->SetCommand(VarArgs("kickid %d", UTIL_PlayerByIndex(i)->GetUserID()));
			pPos->pKickButton->SetVisible(true);
		}

		StatPanel_t *pStats = pPos->pStatPanel;

		pStats->pGoals->SetText(GET_STAT_TEXT(gr->GetGoals(i)));
		pStats->pGoals->SetVisible(gr->GetGoals(i) > 0);

		pStats->pAssists->SetText(GET_STAT_TEXT(gr->GetAssists(i)));
		pStats->pAssists->SetVisible(gr->GetAssists(i) > 0);

		pStats->pYellows->SetText(GET_STAT_TEXT(gr->GetYellowCards(i)));
		pStats->pYellows->SetVisible(gr->GetYellowCards(i) > 0);

		pStats->pReds->SetText(GET_STAT_TEXT(gr->GetRedCards(i)));
		pStats->pReds->SetVisible(gr->GetRedCards(i) > 0);

		pStats->pPossession->SetText(GET_STAT_TEXT(gr->GetPossession(i)));
		pStats->pPossession->SetVisible(false);

		pStats->pPing->SetText(VarArgs("%d", gr->GetPing(i)));
		pStats->pPing->SetVisible(true);

		if (Q_strlen(gr->GetCountryName(i)) > 0)
		{
			pStats->pCountryFlag->SetImage(VarArgs("countryflags/%s", gr->GetCountryName(i)));
			pStats->pCountryFlag->SetVisible(true);
			//steamapicontext->SteamUtils()->GetIPCountry()
		}
	}

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[m_nActiveTeam]->SetVisible(true);
		m_pTeamPanels[1 - m_nActiveTeam]->SetVisible(false);

		for (int j = 0; j < 11; j++)
		{
			if (posTaken[i][j])
				continue;
			
			PosPanel_t *pPos = m_pPosPanels[i][j];
			pPos->pPlayerName->SetText("JOIN");
			pPos->pPlayerName->SetCursor(dc_hand);
			pPos->pPlayerName->SetEnabled(true);
			pPos->pPlayerName->SetFgColor(gr->GetTeamColor(TEAM_UNASSIGNED));
			pPos->pClubName->SetText("");

			StatPanel_t *pStats = pPos->pStatPanel;
			pStats->pGoals->SetVisible(false);
			pStats->pAssists->SetVisible(false);
			pStats->pYellows->SetVisible(false);
			pStats->pReds->SetVisible(false);
			pStats->pPossession->SetVisible(false);
			pStats->pPing->SetVisible(false);
			pStats->pCountryFlag->SetVisible(false);

			pPos->pKickButton->SetVisible(false);
		}
	}
	C_Team *pTeamA = GetGlobalTeam(TEAM_A);
	m_pTeamNames[0]->SetText(VarArgs("%s (%s) - %d players", pTeamA->Get_Name(), pTeamA->Get_FullName(), pTeamA->Get_Number_Players()));
	C_Team *pTeamB = GetGlobalTeam(TEAM_B);
	m_pTeamNames[1]->SetText(VarArgs("%s (%s) - %d players", pTeamB->Get_Name(), pTeamB->Get_FullName(), pTeamB->Get_Number_Players()));

	m_pTabButtons[0]->SetText(gr->GetTeamName(TEAM_A));
	m_pTabButtons[1]->SetText(gr->GetTeamName(TEAM_B));

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
	else if (!stricmp(cmd, "togglestats"))
	{
		m_bShowStats = !m_bShowStats;
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 11; j++)
			{
				m_pPosPanels[i][j]->pPlayerImage->SetVisible(!m_bShowStats);
				m_pPosPanels[i][j]->pPosInfo->SetVisible(m_bShowStats);
			}
		}
		m_pToggleStats->SetText(m_bShowStats ? "Hide Stats" : "Show Stats");
	}
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
	if( m_iJumpKey != BUTTON_CODE_INVALID && m_iJumpKey == code )
	{
		AutoAssign();
	}
	else if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
	{
		//gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
		//gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
		//Close();
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}
