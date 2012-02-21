//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include <stdio.h>

#include <cdll_client_int.h>

#include "classmenu.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <FileSystem.h>

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Panel.h>

#include "vgui_bitmapbutton.h"	//ios

#include "cdll_util.h"
#include "IGameUIFuncs.h" // for key bindings
#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif
#include <game/client/iviewport.h>

#if defined( TF_CLIENT_DLL )
#include "item_inventory.h"
#endif // TF_CLIENT_DLL

#include <stdlib.h> // MAX_PATH define

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#ifdef TF_CLIENT_DLL
#define HUD_CLASSAUTOKILL_FLAGS		( FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO )
#else
#define HUD_CLASSAUTOKILL_FLAGS		( FCVAR_CLIENTDLL | FCVAR_ARCHIVE )
#endif // !TF_CLIENT_DLL

ConVar hud_classautokill( "hud_classautokill", "1", HUD_CLASSAUTOKILL_FLAGS, "Automatically kill player after choosing a new playerclass." );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClassMenu::CClassMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_CLASS)
{
	m_pViewPort = pViewPort;
	m_nGoalsBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_iTeam = 0;

	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);

	// info window about this class
	//ios m_pPanel = new EditablePanel( this, "ClassInfo" );

	LoadControlSettings( "Resource/UI/ClassMenu.res" );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClassMenu::CClassMenu(IViewPort *pViewPort, const char *panelName) : Frame(NULL, panelName)
{
	m_pViewPort = pViewPort;
	m_nGoalsBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_iTeam = 0;

	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);

	// info window about this class
	//ios m_pPanel = new EditablePanel( this, "ClassInfo" );

	// Inheriting classes are responsible for calling LoadControlSettings()!
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CClassMenu::~CClassMenu()
{
}

MouseOverPanelButton* CClassMenu::CreateNewMouseOverPanelButton(EditablePanel *panel)
{ 
	return new MouseOverPanelButton(this, "MouseOverPanelButton", panel);
}


Panel *CClassMenu::CreateControlByName(const char *controlName)
{
	if (!Q_strncmp(controlName, "MaterialButton", 20))			//ios
	{
		return new CBitmapButton(NULL, "BitmapButton", "");
	}

	//if( !Q_stricmp( "MouseOverPanelButton", controlName ) )
	//{
	//	MouseOverPanelButton *newButton = CreateNewMouseOverPanelButton( m_pPanel );

	//	m_mouseoverButtons.AddToTail( newButton );
	//	return newButton;
	//}
	//else
	//{
		return BaseClass::CreateControlByName( controlName );
	//}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassMenu::Reset()
{
	for ( int i = 0 ; i < GetChildCount() ; ++i )
	{
		// Hide the subpanel for the MouseOverPanelButtons
		MouseOverPanelButton *pPanel = dynamic_cast<MouseOverPanelButton *>( GetChild( i ) );

		if ( pPanel )
		{
			pPanel->HidePage();
		}
	}

	// Turn the first button back on again (so we have a default description shown)
	Assert( m_mouseoverButtons.Count() );
	for ( int i=0; i<m_mouseoverButtons.Count(); ++i )
	{
		if ( i == 0 )
		{
			m_mouseoverButtons[i]->ShowPage();	// Show the first page
		}
		else
		{
			m_mouseoverButtons[i]->HidePage();	// Hide the rest
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user picks a class
//-----------------------------------------------------------------------------
void CClassMenu::OnCommand( const char *cmd )
{
//	if ( Q_stricmp( command, "vguicancel" ) )
//	{
//		engine->ClientCmd( const_cast<char *>( command ) );
//
//#ifndef CSTRIKE_DLL
//		// They entered a command to change their class, kill them so they spawn with 
//		// the new class right away
//		if ( hud_classautokill.GetBool() )
//		{
//            engine->ClientCmd( "kill" );
//		}
//#endif // !CSTRIKE_DLL
//	}

	//ios
	if (!stricmp(cmd, "pos 1") )
		engine->ClientCmd("pos 1");
	if (!stricmp(cmd, "pos 2") )
		engine->ClientCmd("pos 2");
	if (!stricmp(cmd, "pos 3") )
		engine->ClientCmd("pos 3");
	if (!stricmp(cmd, "pos 4") )
		engine->ClientCmd("pos 4");
	if (!stricmp(cmd, "pos 5") )
		engine->ClientCmd("pos 5");
	if (!stricmp(cmd, "pos 6") )
		engine->ClientCmd("pos 6");
	if (!stricmp(cmd, "pos 7") )
		engine->ClientCmd("pos 7");
	if (!stricmp(cmd, "pos 8") )
		engine->ClientCmd("pos 8");
	if (!stricmp(cmd, "pos 9") )
		engine->ClientCmd("pos 9");
	if (!stricmp(cmd, "pos 10") )
		engine->ClientCmd("pos 10");
	if (!stricmp(cmd, "pos 11") )
		engine->ClientCmd("pos 11");

	Close();

	gViewPortInterface->ShowBackGround( false );

	BaseClass::OnCommand(cmd);
}

//-----------------------------------------------------------------------------
// Purpose: shows the class menu
//-----------------------------------------------------------------------------
void CClassMenu::ShowPanel(bool bShow)
{
	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );

		// load a default class page
		for ( int i=0; i<m_mouseoverButtons.Count(); ++i )
		{
			if ( i == 0 )
			{
				m_mouseoverButtons[i]->ShowPage();	// Show the first page
			}
			else
			{
				m_mouseoverButtons[i]->HidePage();	// Hide the rest
			}
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
	}
	
	m_pViewPort->ShowBackGround( bShow );
}


void CClassMenu::SetData(KeyValues *data)
{
	m_iTeam = data->GetInt( "team" );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CClassMenu::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the visibility of a button by name
//-----------------------------------------------------------------------------
void CClassMenu::SetVisibleButton(const char *textEntryName, bool state)
{
	Button *entry = dynamic_cast<Button *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetVisible(state);
	}
}

void CClassMenu::OnKeyCodePressed(KeyCode code)
{
	if ( m_nGoalsBoardKey != BUTTON_CODE_INVALID && m_nGoalsBoardKey == code )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
		gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}






