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

#include "sdk_backgroundpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTeamMenu::CTeamMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_TEAM )
{
	m_pViewPort = pViewPort;
	m_iJumpKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_nActiveTeam = 0;

	SetSize(960, 720);
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
			PosPanel_t *pPanel = new PosPanel_t;
			pPanel->pPosPanel = new Panel(m_pTeamPanels[i]);
			pPanel->pPlayerImage = new ImagePanel(pPanel->pPosPanel, "Image");
			pPanel->pPosInfo = new Label(pPanel->pPosPanel, "Label", "");
			pPanel->pPlayerName = new Button(pPanel->pPosPanel, "Button", "");
			pPanel->pClubName = new Label(pPanel->pPosPanel, "Label", "");
			pPanel->pPosNumber = new Label(pPanel->pPosPanel, "Label", "");
			pPanel->pPosName = new Label(pPanel->pPosPanel, "Label", "");
			m_pPosPanels[i][j] = pPanel;
		}
	}
}

#define BUTTON_WIDTH	200
#define BUTTON_HEIGHT	120
#define BUTTON_MARGIN	20
#define NUMBER_MARGIN	2
#define NUMBER_WIDTH	30
#define NUMBER_HEIGHT	40
#define NAME_WIDTH		BUTTON_SIZE - NUMBER_WIDTH - NUMBER_MARGIN
#define NAME_HEIGHT		30
#define INFO_WIDTH		20
#define INFO_HEIGHT		30
#define IMAGE_SIZE		60

const char *g_szPosNames[32] =
{
	"GK",
	"RB",
	"CB",
	"CB",
	"LB",
	"RM",
	"CM",
	"LM",
	"RF",
	"CF",
	"LF",
	NULL
};

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
		m_pTabButtons[i]->SetBounds(100 * i, 0, 100, 50);
		m_pTabButtons[i]->SetCommand(VarArgs("showteam %d", i));
		m_pTabButtons[i]->AddActionSignalTarget(this);

		m_pTeamNames[i]->SetBounds(0, 0, 550, 50);
		//m_pTeamNames[i]->SetTextInset(50, 10);
		//m_pTeamNames[i]->SetPinCorner(Panel::PIN_TOPRIGHT, 10, 10);
		m_pTeamNames[i]->SetFgColor(Color(255, 255, 255, 255));
		m_pTeamNames[i]->SetFont(pScheme->GetFont("IOSScorebar"));

		m_pTeamPanels[i]->SetBounds(0, 50, 960, 670);
		m_pTeamPanels[i]->SetBgColor(Color(0, 0, 0, 200));
		m_pTeamPanels[i]->SetPaintBackgroundEnabled(true);
		m_pTeamPanels[i]->SetPaintBackgroundType(2);
		m_pTeamPanels[i]->SetVisible(i == m_nActiveTeam);

		for(int j = 0; j < 11; j++)
		{
			PosPanel_t *pPanel = m_pPosPanels[i][j];

			pPanel->pPosPanel->SetBounds(pos[j][0] * (BUTTON_WIDTH + BUTTON_MARGIN) + 20, pos[j][1] * (BUTTON_HEIGHT + 2 * BUTTON_MARGIN) + 50, BUTTON_WIDTH, BUTTON_HEIGHT);
			
			pPanel->pPlayerImage->SetBounds(pPanel->pPosPanel->GetWide() / 2 - IMAGE_SIZE / 2, pPanel->pPosPanel->GetTall() / 2 - IMAGE_SIZE / 2 - 2 * NAME_HEIGHT, IMAGE_SIZE, IMAGE_SIZE);
			pPanel->pPlayerImage->SetImage("shirt");
			pPanel->pPlayerImage->SetShouldScaleImage(true);

			int possession = 10;
			int goals = 2;
			int assists = 3;
			int fouls = 1;
			int yellows = 1;
			int reds = 1;
			pPanel->pPosInfo->SetBounds(0, INFO_HEIGHT, pPanel->pPosPanel->GetWide(), INFO_HEIGHT);
			pPanel->pPosInfo->SetText(VarArgs("%d%%P/%dG/%dA/%dF/%dY/%dR", possession, goals, assists, fouls, yellows, reds));
			pPanel->pPosInfo->SetContentAlignment(Label::a_center);
			pPanel->pPosInfo->SetFont(pScheme->GetFont("IOSScorebar"));
			pPanel->pPosInfo->SetVisible(false);

			pPanel->pPlayerName->SetBounds(0, pPanel->pPosPanel->GetTall() - 2 * NAME_HEIGHT, pPanel->pPosPanel->GetWide(), NAME_HEIGHT);
			pPanel->pPlayerName->SetCommand(VarArgs("jointeam %d %d", i + 2, 11 - j));
			pPanel->pPlayerName->AddActionSignalTarget(this);
			pPanel->pPlayerName->SetPaintBackgroundEnabled(true);
			pPanel->pPlayerName->SetPaintBorderEnabled(false);
			pPanel->pPlayerName->SetPaintBackgroundType(2);
			pPanel->pPlayerName->SetBgColor(Color(255, 255, 255, 200));
			pPanel->pPlayerName->SetDefaultColor(Color(0, 0, 0, 200), Color(255, 255, 255, 200));
			pPanel->pPlayerName->SetArmedColor(Color(50, 50, 50, 200), Color(150, 150, 150, 200));
			pPanel->pPlayerName->SetDepressedColor(Color(100, 100, 100, 200), Color(255, 255, 255, 200));
			pPanel->pPlayerName->SetButtonBorderEnabled(false);
			pPanel->pPlayerName->SetContentAlignment(Label::a_center);
			pPanel->pPlayerName->SetFont(pScheme->GetFont("IOSScorebar"));

			pPanel->pPosName->SetBounds(0, pPanel->pPosPanel->GetTall() - NAME_HEIGHT, NUMBER_WIDTH, NAME_HEIGHT);
			pPanel->pPosName->SetText(g_szPosNames[10 - j]);
			pPanel->pPosName->SetFont(pScheme->GetFont("IOSScorebar"));

			pPanel->pClubName->SetBounds(0, pPanel->pPosPanel->GetTall() - NAME_HEIGHT, pPanel->pPosPanel->GetWide(), NAME_HEIGHT);
			pPanel->pClubName->SetContentAlignment(Label::a_center);
			pPanel->pClubName->SetFont(pScheme->GetFont("IOSScorebar"));

			pPanel->pPosNumber->SetBounds(pPanel->pPosPanel->GetWide() - NUMBER_WIDTH, pPanel->pPosPanel->GetTall() - NAME_HEIGHT, NUMBER_WIDTH, NAME_HEIGHT);
			pPanel->pPosNumber->SetText(VarArgs("%d", 11 - j));
			pPanel->pPosNumber->SetFont(pScheme->GetFont("IOSScorebar"));
		}
	}

	m_pSpectateButton->SetBounds(100 * 3, 0, 100, 50);
	m_pSpectateButton->SetCommand(VarArgs("jointeam %d 1", TEAM_SPECTATOR));
	m_pSpectateButton->AddActionSignalTarget(this);

	m_pToggleStats->SetBounds(100 * 4, 0, 100, 50);
	m_pToggleStats->SetCommand("togglestats");
	m_pToggleStats->AddActionSignalTarget(this);
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
		if (gr->IsConnected(i))
		{
			int team = gr->GetTeam(i);
			int pos = gr->GetTeamPosition(i);
			if ((team == TEAM_A || team == TEAM_B) && pos >= 1 && pos <= 11)
			{
				m_pPosPanels[team - 2][11 - pos]->pPlayerName->SetText(gr->GetPlayerName(i));
				m_pPosPanels[team - 2][11 - pos]->pPlayerName->SetCursor(dc_arrow);
				m_pPosPanels[team - 2][11 - pos]->pPlayerName->SetEnabled(Q_strncmp(gr->GetPlayerName(i), "KEEPER", 6) == 0);
				m_pPosPanels[team - 2][11 - pos]->pClubName->SetText(gr->GetClubName(i));
				posTaken[team - 2][11 - pos] = true;
			}	
		}
	}

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[m_nActiveTeam]->SetVisible(true);
		m_pTeamPanels[1 - m_nActiveTeam]->SetVisible(false);

		for (int j = 0; j < 11; j++)
		{
			if (!posTaken[i][j])
			{
				m_pPosPanels[i][j]->pPlayerName->SetText("JOIN");
				m_pPosPanels[i][j]->pPlayerName->SetCursor(dc_hand);
				m_pPosPanels[i][j]->pPlayerName->SetEnabled(true);
				m_pPosPanels[i][j]->pClubName->SetText("");
			}
		}
	}

	m_pTeamNames[0]->SetText(gr->GetTeamName(TEAM_A));
	m_pTeamNames[1]->SetText(gr->GetTeamName(TEAM_B));

	m_flNextUpdateTime = gpGlobals->curtime + 0.5f;
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
