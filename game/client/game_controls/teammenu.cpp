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

void UpdateCursorState();
// void DuckMessage(const char *str);

// helper function
const char *GetStringTeamColor( int i )
{
	switch( i )
	{
	case 0:
		return "team0";

	case 1:
		return "team1";

	case 2:
		return "team2";

	case 3:
		return "team3";

	case 4:
	default:
		return "team4";
	}
}



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
	
	m_szTeamNames[0][0] = 0;
	m_szTeamNames[1][0] = 0;

	m_pTeamPanels[0] = new Panel(this, "HomeTeamPanel");
	m_pTeamPanels[1] = new Panel(this, "AwayTeamPanel");

	m_pTeamNames[0] = new Label(m_pTeamPanels[0], "HomeTeamLabel", "");
	m_pTeamNames[1] = new Label(m_pTeamPanels[1], "AwayTeamLabel", "");

	MakeTeamButtons();
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

void CTeamMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	MoveToCenterOfScreen();

	int s = 128;

	int pos[11][2] = {
				{ s*0.5f, 0 }, { s*1.5f, 0 }, { s*2.5f, 0 },
			{ s*0.5f, s }, { s*1.5f, s }, { s*2.5f, s },
		{ 0, s*2 }, { s, s*2 }, { s*2, s*2 }, { s*3, s*2 },
						{ s*1.5f, s*3 }
	};

	for(int i = 0; i < 2; i++)
	{
		m_pTeamNames[i]->SetBounds(0, 0, 550, 50);
		m_pTeamNames[i]->SetTextInset(50, 10);
		//m_pTeamNames[i]->SetPinCorner(Panel::PIN_TOPRIGHT, 10, 10);
		m_pTeamNames[i]->SetFgColor(Color(255, 255, 255, 255));
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		HFont font = pScheme->GetFont("IOSUbuntuFont");
		m_pTeamNames[i]->SetFont(font);

		m_pTeamPanels[i]->SetBounds(0 + i * 650, 0, 550, 700);
		m_pTeamPanels[i]->SetBgColor(Color(0, 0, 0, 200));
		m_pTeamPanels[i]->SetPaintBackgroundEnabled(true);
		m_pTeamPanels[i]->SetPaintBackgroundType(2);

		for(int j = 0; j < 11; j++)
		{
			CBitmapButton *button = m_pPosButtons[i][j];
			button->SetBounds(pos[j][0], pos[j][1] + 100, s, s);
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
	OnClose();
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

	bool posTaken[2][11];

	// walk all the players and make sure they're in the scoreboard
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		if (gr->IsConnected( i ) )
		{
			int team = gr->GetTeam(i);
			int pos = gr->GetPosition(i);
			if ((team == TEAM_A || team == TEAM_B) && pos >= 1 && pos <= 11)
			{
				CBitmapButton *button = m_pPosButtons[team - 2][11 - pos];
				char buttonText[32];
				Q_snprintf( buttonText, sizeof(buttonText), "%s", gr->GetPlayerName(i));
				button->SetText( buttonText );
				button->SetEnabled(false);
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
				char buttonText[32];
				Q_snprintf(buttonText, sizeof(buttonText), "Pos &%d", 11 - j);
				m_pPosButtons[i][j]->SetText(buttonText);
				m_pPosButtons[i][j]->SetEnabled(true);
			}
		}
	}

	m_pTeamNames[0]->SetText(gr->GetTeamName(TEAM_A));
	m_pTeamNames[1]->SetText(gr->GetTeamName(TEAM_B));

	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
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

	Close();

	BaseClass::OnCommand(cmd);
}

void CTeamMenu::MakeTeamButtons()
{
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			CBitmapButton *button = new CBitmapButton(m_pTeamPanels[i], "BitmapButton", "");

			IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

			char buttonText[32];
			Q_snprintf( buttonText, sizeof(buttonText), "Pos &%d", 11 - j); 
			button->SetText( buttonText );
			button->SetTextColorState(Label::CS_BRIGHT);
			button->SetContentAlignment(Label::a_south);

			button->SetFgColor(Color(100, 255, 100, 255));	
			button->SetCommand(VarArgs("jointeam %d %d", i + 2, 11 - j));
			button->AddActionSignalTarget(this);
			button->SetVisible(true);
			button->SetBgColor(Color(100, 255, 100, 255));
			button->SetPaintBackgroundEnabled(true);
			button->SetPaintBorderEnabled(false);

			color32 color = { 255, 255, 255, 255 };
			button->SetImage(CBitmapButton::BUTTON_ENABLED, "gui/glassbutt", color);
			button->SetImage(CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "gui/glassbuttover", color);
			button->SetImage(CBitmapButton::BUTTON_PRESSED, "gui/glassbuttdown", color);
			button->SetImage(CBitmapButton::BUTTON_DISABLED, "gui/glassbutt", color);

			m_pPosButtons[i][j] = button;
		}
	}
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
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
		gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}
