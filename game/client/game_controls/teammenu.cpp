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

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i] = new Panel(this, VarArgs("TeamPanel%d", i));
		m_pTeamNames[i] = new Label(m_pTeamPanels[i], VarArgs("TeamLabel%d", i), "");
		m_szTeamNames[i][0] = 0;
		m_pTabButtons[i] = new Button(this, "TabButton", VarArgs("Team%d", i + 1));

		for (int j = 0; j < 11; j++)
		{
			m_pPosButtons[i][j] = new CBitmapButton(m_pTeamPanels[i], "Button", "");
			m_pPosInfos[i][j] = new Label(m_pPosButtons[i][j], "Info", "");
			m_pPosNumbers[i][j] = new Label(m_pPosButtons[i][j], "Number", "");
			m_pPlayerNames[i][j] = new Label(m_pPosButtons[i][j], "Name", "");
			m_pPosNames[i][j] = new Label(m_pPosButtons[i][j], "Name", "");
		}
	}
}

//void CTeamMenu::PaintBackground()
//{
//	int wide, tall;
//	GetSize( wide, tall );
//	int offset = 50;
//
//	DrawRoundedBackground(Color(150, 150, 150, 200), wide / 2 - offset, tall - 100, 0, 100 );
//
//	DrawRoundedBackground(Color(150, 150, 150, 200), wide / 2 - offset, tall - 100, wide / 2 + offset, 100 );
//}
//
//void CTeamMenu::PaintBorder()
//{
//	int wide, tall;
//	GetSize( wide, tall );
//	int offset = 50;
//
//	DrawRoundedBorder(Color(100, 100, 100, 200), wide / 2 - offset, tall - 100, 0, 100 );
//
//	DrawRoundedBorder(Color(100, 100, 100, 200), wide / 2 - offset, tall - 100, wide / 2 + offset, 100 );
//}

#define BUTTON_WIDTH	200
#define BUTTON_HEIGHT	120
#define BUTTON_MARGIN	20
#define NUMBER_MARGIN	2
#define NUMBER_WIDTH	30
#define NUMBER_HEIGHT	40
#define NAME_WIDTH		BUTTON_SIZE - NUMBER_WIDTH - NUMBER_MARGIN
#define NAME_HEIGHT		30
#define INFO_WIDTH		20

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
			CBitmapButton *button = m_pPosButtons[i][j];
			Label *info = m_pPosInfos[i][j];
			Label *number = m_pPosNumbers[i][j];
			Label *playername = m_pPlayerNames[i][j];
			Label *posname = m_pPosNames[i][j];

			//button->SetContentAlignment(Label::a_center);
			button->SetCommand(VarArgs("jointeam %d %d", i + 2, 11 - j));
			button->AddActionSignalTarget(this);
			button->SetPaintBackgroundEnabled(true);
			button->SetPaintBorderEnabled(false);
			button->SetPaintBackgroundType(2);
			button->SetBounds(pos[j][0] * (BUTTON_WIDTH + BUTTON_MARGIN) + 20, pos[j][1] * (BUTTON_HEIGHT + 2 * BUTTON_MARGIN) + 50, BUTTON_WIDTH, BUTTON_HEIGHT);
			button->SetCursor(dc_hand);
			//button->SetDefaultColor(Color(), Color(0,0,0,100));
			//button->SetArmedColor(Color(), Color(150, 150, 150, 100));
			//button->SetDepressedColor(Color(), Color(255, 255, 255, 100));
			button->SetButtonBorderEnabled(false);
			color32 enabled = { 200, 200, 200, 255 };
			color32 mouseover = { 225, 225, 255, 255 };
			color32 pressed = { 255, 255, 255, 255 };
			color32 disabled = { 150, 150, 150, 255 };
			button->SetImage(CBitmapButton::BUTTON_ENABLED, "gui/shirt", enabled);
			button->SetImage(CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "gui/shirt", mouseover);
			button->SetImage(CBitmapButton::BUTTON_PRESSED, "gui/shirt", pressed);
			button->SetImage(CBitmapButton::BUTTON_DISABLED, "gui/shirt", disabled);

			info->SetBounds(0, button->GetTall() * 0.33f, INFO_WIDTH, button->GetTall() * 0.66f);
			//info->SetPaintBackgroundEnabled(true);
			//info->SetPaintBackgroundType(2);
			//info->SetBgColor(Color(0, 0, 0, 255));
			info->SetFgColor(Color(255, 255, 255, 255));
			int possession = 10;
			int goals = 2;
			int assists = 3;
			int fouls = 1;
			int yellows = 1;
			int reds = 1;
			info->SetText(VarArgs("%d%%\n%d\n%d", possession, goals, assists));
			//info->SetContentAlignment(Label::a_center);

			number->SetText(VarArgs("%d", 11 - j));
			number->SetContentAlignment(Label::a_center);
			number->SetFont(pScheme->GetFont("IOSScorebar"));
			number->SetFgColor(Color(0, 0, 0, 255));
			//number->SetBgColor(Color(255, 255, 255, 255));
			//number->SetPaintBackgroundEnabled(true);
			//number->SetPaintBackgroundType(2);
			number->SetBounds(button->GetWide() / 2 - NUMBER_WIDTH / 2, button->GetTall() / 2 - NUMBER_HEIGHT / 2, NUMBER_WIDTH, NUMBER_HEIGHT);

			playername->SetText(VarArgs(""));
			playername->SetContentAlignment(Label::a_center);
			playername->SetFont(pScheme->GetFont("IOSScorebar"));
			playername->SetFgColor(Color(0, 0, 0, 255));	
			playername->SetBounds(0, button->GetTall() - NAME_HEIGHT - NAME_HEIGHT / 2, button->GetWide(), NAME_HEIGHT);

			posname->SetText(g_szPosNames[10 - j]);
			posname->SetContentAlignment(Label::a_center);
			posname->SetFont(pScheme->GetFont("IOSScorebar"));
			posname->SetFgColor(Color(0, 0, 0, 255));	
			posname->SetBounds(0, NAME_HEIGHT / 2, button->GetWide(), NAME_HEIGHT);
		}
	}

	m_pSpectateButton->SetBounds(100 * 3, 0, 100, 50);
	m_pSpectateButton->SetCommand(VarArgs("jointeam %d 1", TEAM_SPECTATOR));
	m_pSpectateButton->AddActionSignalTarget(this);

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
		
		//set teamnames on join team panel buttons
		SetLabelText("teamabutton", m_szTeamNames[0]);
		SetLabelText("teambbutton", m_szTeamNames[1]);
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
				CBitmapButton *button = m_pPosButtons[team - 2][11 - pos];
				m_pPlayerNames[team - 2][11 - pos]->SetText(gr->GetPlayerName(i));
				m_pPosButtons[team - 2][11 - pos]->SetEnabled(Q_strncmp(gr->GetPlayerName(i), "KEEPER", 6) == 0);
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
				m_pPlayerNames[i][j]->SetText("");
				m_pPosButtons[i][j]->SetEnabled(true);
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

	//Close();

	BaseClass::OnCommand(cmd);

	m_flNextUpdateTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CTeamMenu::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
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
