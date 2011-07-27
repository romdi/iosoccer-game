//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include <stdio.h>

#include <cdll_client_int.h>

#include "ios_statusmenu.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <FileSystem.h>

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <igameresources.h>


#include "vgui_bitmapbutton.h"	//ios

#include "cdll_util.h"
#include "IGameUIFuncs.h" // for key bindings
#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif
#include <game/client/iviewport.h>

#include <stdlib.h> // MAX_PATH define

#include "tier0/memdbgon.h"

using namespace vgui;

#define BLACK_BAR_COLOR	Color(0, 0, 0, 64)

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CStatusBar::CStatusBar(IViewPort *pViewPort) : Frame( NULL, PANEL_STATUS )
{
	m_bSpecScoreboard = false;

	m_pViewPort = pViewPort;

	// initialize dialog
	SetVisible(false);
	SetTitle("StatusMenu", true);
	SetProportional(true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);
	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );

	// hide the system buttons
	SetTitleBarVisible( false );

	m_pTopBar = new Panel( this, "topbar");
	m_pStatusBarLabel = new Label( m_pTopBar, "playerlabel", "" );

	//m_pStatusBarLabel = new Label( this, "playerlabel", "" );
	m_pStatusBarLabel->SetVisible( false );
	m_pStatusBarLabel->SetContentAlignment(Label::a_center);
	m_Text[0] = 0;
	m_AlphaText = 255;
	m_AlphaBar = 128;

	LoadControlSettings("Resource/UI/StatusBar.res");
	
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	m_pTopBar->SetVisible( true );

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CStatusBar::~CStatusBar()
{
}

//-----------------------------------------------------------------------------
// Purpose: Sets the colour of the top and bottom bars
//-----------------------------------------------------------------------------
void CStatusBar::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBgColor(Color( 0,0,0,0 ) ); // make the background transparent
	m_pTopBar->SetBgColor(BLACK_BAR_COLOR);
	SetPaintBorderEnabled(false);
	SetBorder( NULL );

}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CStatusBar::PerformLayout()
{
	int w,h;
	GetHudSize(w, h);
	
	// fill the screen
	SetBounds(0,0,w,h);
}

//-----------------------------------------------------------------------------
// Purpose: checks spec_scoreboard cvar to see if the scoreboard should be displayed
//-----------------------------------------------------------------------------
void CStatusBar::OnThink()
{
	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: sets the image to display for the banner in the top right corner
//-----------------------------------------------------------------------------
void CStatusBar::SetLogoImage(const char *image)
{
	if ( m_pBannerImage )
	{
		m_pBannerImage->SetImage( scheme()->GetImage(image, false) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CStatusBar::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CStatusBar::SetLabelText(const char *textEntryName, wchar_t *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CStatusBar::MoveLabelToFront(const char *textEntryName)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: shows/hides the buy menu
//-----------------------------------------------------------------------------
void CStatusBar::ShowPanel(bool bShow)
{
	SetVisible( bShow );
	MoveToFront();
}

bool CStatusBar::ShouldShowPlayerLabel( int specmode )
{
	return true;	//ios
}

const static float FADE_TIME = 0.5f;

//-----------------------------------------------------------------------------
// Purpose: Updates the gui, rearranges elements
//-----------------------------------------------------------------------------
void CStatusBar::Update()
{
	int wide, tall;
	int bx, by, bwide, btall;

	GetHudSize(wide, tall);
	m_pTopBar->GetBounds( bx, by, bwide, btall );

	IGameResources *gr = GameResources();
	int playernum = GetSpectatorTarget();


	Color c = Color (0,0,0);
	if (gr)
		c = gr->GetTeamColor( gr->GetTeam(playernum) ); // Player's team color
	m_pStatusBarLabel->SetVisible(true);

	//set text and centre it
	m_pStatusBarLabel->SetText(m_Text);
	int width, height;
	m_pStatusBarLabel->GetContentSize(width, height);
	m_pStatusBarLabel->SetPos((bwide/2) - (width/2), 8);	//why doesn't this autocentre?
	m_pStatusBarLabel->SetSize(width, height);				//wtf? doesnt seem to draw the full string unless i do this

	//set alphas - if they fade
	c[3] = m_AlphaText * m_T;
	m_pStatusBarLabel->SetFgColor( c );
	m_pTopBar->SetBgColor(Color(0, 0, 0, m_AlphaBar * m_T));

	
	if (gpGlobals->curtime < m_StartTime + FADE_TIME)				//fade if at start
	{
		m_T = ((gpGlobals->curtime - m_StartTime) / FADE_TIME);
	}
	else if (gpGlobals->curtime > m_EndTime - FADE_TIME)			//fade if at end
	{
		m_T = ((m_EndTime - gpGlobals->curtime) / FADE_TIME);
	}
	else
	{
		m_T = 1.0f;
	}

	if (gpGlobals->curtime > m_EndTime)
	{
		m_Text[0] = 0;
		ShowPanel(false);
	}

}


void CStatusBar::SetData(KeyValues *data)
{
	char	text[256];
	strcpy (text, data->GetString( "msg" ));

	//add new messages onto end of old
	//if (m_Text[0] != 0)
	//	strcat (m_Text, "; ");
	strcat(m_Text, text);

	m_EndTime = gpGlobals->curtime + 5.0f;
	m_StartTime = gpGlobals->curtime;
	m_T = 0.0f;
}


