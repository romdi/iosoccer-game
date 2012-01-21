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

	SetSize(1200, 700);
	SetTitle("", true);
	SetMoveable(false);
	SetSizeable(false);
	SetTitleBarVisible( false );
	SetProportional(false);
	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	m_pSpectateButton = new Button(this, "SpectateButton", "Spectate", this, VarArgs("jointeam %d 1", TEAM_SPECTATOR));

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i] = new Panel(this, VarArgs("TeamPanel%d", i));
		m_pTeamNames[i] = new Label(m_pTeamPanels[i], VarArgs("TeamLabel%d", i), "");
		m_szTeamNames[i][0] = 0;
		for (int j = 0; j < 11; j++)
		{
			m_pPosButtons[i][j] = new Button(m_pTeamPanels[i], "Button", "");
			m_pPosInfoPanels[i][j] = new Panel(m_pPosButtons[i][j], "Panel");
			m_pPosNumbers[i][j] = new Label(m_pPosInfoPanels[i][j], "Number", "");
			m_pPosNames[i][j] = new Label(m_pPosInfoPanels[i][j], "Name", "");
		}
	}
}

void CTeamMenu::PaintBackground()
{
	int wide, tall;
	GetSize( wide, tall );
	int offset = 50;

	DrawRoundedBackground(Color(150, 150, 150, 200), wide / 2 - offset, tall - 100, 0, 100 );

	DrawRoundedBackground(Color(150, 150, 150, 200), wide / 2 - offset, tall - 100, wide / 2 + offset, 100 );
}

void CTeamMenu::PaintBorder()
{
	int wide, tall;
	GetSize( wide, tall );
	int offset = 50;

	DrawRoundedBorder(Color(100, 100, 100, 200), wide / 2 - offset, tall - 100, 0, 100 );

	DrawRoundedBorder(Color(100, 100, 100, 200), wide / 2 - offset, tall - 100, wide / 2 + offset, 100 );
}

#define BUTTON_SIZE		110
#define BUTTON_MARGIN	5
#define NUMBER_MARGIN	2
#define NUMBER_WIDTH	20
#define NAME_WIDTH		BUTTON_SIZE - NUMBER_WIDTH - NUMBER_MARGIN
#define NAME_HEIGHT		30

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
		m_pTeamNames[i]->SetBounds(0, 0, 550, 50);
		//m_pTeamNames[i]->SetTextInset(50, 10);
		//m_pTeamNames[i]->SetPinCorner(Panel::PIN_TOPRIGHT, 10, 10);
		m_pTeamNames[i]->SetFgColor(Color(255, 255, 255, 255));
		m_pTeamNames[i]->SetFont(pScheme->GetFont("IOSScorebar"));

		m_pTeamPanels[i]->SetBounds(0 + i * 650, 0, 550, 700);
		m_pTeamPanels[i]->SetBgColor(Color(0, 0, 0, 200));
		m_pTeamPanels[i]->SetPaintBackgroundEnabled(true);
		m_pTeamPanels[i]->SetPaintBackgroundType(2);

		for(int j = 0; j < 11; j++)
		{
			Button *button = m_pPosButtons[i][j];
			Panel *info = m_pPosInfoPanels[i][j];
			Label *number = m_pPosNumbers[i][j];
			Label *name = m_pPosNames[i][j];

			//button->SetContentAlignment(Label::a_center);
			button->SetCommand(VarArgs("jointeam %d %d", i + 2, 11 - j));
			button->AddActionSignalTarget(this);
			button->SetPaintBackgroundEnabled(true);
			button->SetPaintBorderEnabled(false);
			button->SetPaintBackgroundType(2);
			button->SetBounds(pos[j][0] * (BUTTON_SIZE + BUTTON_MARGIN) + 50, pos[j][1] * (BUTTON_SIZE + BUTTON_MARGIN) + 100, BUTTON_SIZE, BUTTON_SIZE);
			button->SetCursor(dc_hand);
			button->SetDefaultColor(Color(), Color(0,0,0,100));
			button->SetArmedColor(Color(), Color(150, 150, 150, 100));
			button->SetDepressedColor(Color(), Color(255, 255, 255, 100));
			button->SetButtonBorderEnabled(false);

			info->SetBounds(0, button->GetTall() - NAME_HEIGHT, BUTTON_SIZE, NAME_HEIGHT);
			info->SetPaintBackgroundEnabled(true);
			info->SetPaintBackgroundType(2);
			info->SetBgColor(Color(0, 0, 0, 255));

			number->SetText(VarArgs("%d", 11 - j));
			number->SetContentAlignment(Label::a_center);
			number->SetFont(pScheme->GetFont("IOSScorebar"));
			number->SetFgColor(Color(0, 0, 0, 255));
			number->SetBgColor(Color(255, 255, 255, 255));	
			number->SetPaintBackgroundEnabled(true);
			number->SetPaintBackgroundType(2);
			number->SetBounds(NUMBER_MARGIN, NUMBER_MARGIN, NUMBER_WIDTH, NAME_HEIGHT - 2 * NUMBER_MARGIN);

			name->SetText(VarArgs(""));
			name->SetContentAlignment(Label::a_center);
			name->SetFont(pScheme->GetFont("IOSScorebar"));
			name->SetFgColor(Color(255, 255, 255, 255));	
			name->SetBounds(NUMBER_MARGIN + NUMBER_WIDTH, 0, NAME_WIDTH, NAME_HEIGHT);
		}
	}

	m_pSpectateButton->SetBounds(GetWide() / 2 - 100 / 2, 0, 100, 50);
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
				Button *button = m_pPosButtons[team - 2][11 - pos];
				m_pPosNames[team - 2][11 - pos]->SetText(gr->GetPlayerName(i));
				m_pPosButtons[team - 2][11 - pos]->SetEnabled(Q_strncmp(gr->GetPlayerName(i), "KEEPER", 6) == 0);
				posTaken[team - 2][11 - pos] = true;
			}	
		}
	}

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			if (!posTaken[i][j])
			{
				m_pPosNames[i][j]->SetText("");
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

	//Close();

	BaseClass::OnCommand(cmd);
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
