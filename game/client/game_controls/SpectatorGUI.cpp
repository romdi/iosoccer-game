//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>
#include <globalvars_base.h>
#include <cdll_util.h>
#include <KeyValues.h>

#include "spectatorgui.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/TextImage.h>

#include <stdio.h> // _snprintf define

#include <game/client/iviewport.h>
#include "commandmenu.h"
#include "hltvcamera.h"

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Menu.h>
#include "IGameUIFuncs.h" // for key bindings
#include <imapoverview.h>
#include <shareddefs.h>
#include <igameresources.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif

// void DuckMessage(const char *str); // from vgui_teamfortressviewport.cpp

ConVar spec_scoreboard( "spec_scoreboard", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );

CSpectatorGUI *g_pSpectatorGUI = NULL;

using namespace vgui;

ConVar cl_spec_mode(
	"cl_spec_mode",
	"1",
	FCVAR_ARCHIVE | FCVAR_USERINFO | FCVAR_SERVER_CAN_EXECUTE,
	"spectator mode" );



//-----------------------------------------------------------------------------
// Purpose: left and right buttons pointing buttons
//-----------------------------------------------------------------------------
class CSpecButton : public Button
{
public:
	CSpecButton(Panel *parent, const char *panelName): Button(parent, panelName, "") {}

private:
	void ApplySchemeSettings(vgui::IScheme *pScheme)
	{
		Button::ApplySchemeSettings(pScheme);
		SetFont(pScheme->GetFont("Marlett", IsProportional()) );
	}
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSpectatorMenu::CSpectatorMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_SPECMENU )
{
	SetScheme("SourceScheme");

	m_iDuckKey = BUTTON_CODE_INVALID;
		
	m_pViewPort = pViewPort;

	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );
	SetTitleBarVisible( false ); // don't draw a title bar
	SetMoveable( false );
	SetSizeable( false );
	SetProportional(true);

	m_pPlayerList = new ComboBox(this, "playercombo", 10 , false);
	HFont hFallbackFont = scheme()->GetIScheme( GetScheme() )->GetFont( "DefaultVerySmallFallBack", false );
	if ( INVALID_FONT != hFallbackFont )
	{
		m_pPlayerList->SetUseFallbackFont( true, hFallbackFont );
	}

	m_pViewOptions = new ComboBox(this, "viewcombo", 10 , false );
	m_pConfigSettings = new ComboBox(this, "settingscombo", 10 , false );	
	m_pConfigSettings->SetVisible(false);

	m_pLeftButton = new CSpecButton( this, "specprev");
	m_pLeftButton->SetText("3");
	m_pRightButton = new CSpecButton( this, "specnext");
	m_pRightButton->SetText("4");

	m_pPlayerList->SetText("");
	m_pViewOptions->SetText("Camera");
	m_pConfigSettings->SetText("Options");

	m_pPlayerList->SetOpenDirection( Menu::UP );
	m_pViewOptions->SetOpenDirection( Menu::UP );
	m_pConfigSettings->SetOpenDirection( Menu::UP );

	// create view config menu
	CommandMenu *menu = new CommandMenu(m_pConfigSettings, "spectatormenu", gViewPortInterface);
	menu->AddMenuItem("Close", "spec_menu 0", this);
	m_pConfigSettings->SetMenu(menu);	// attach menu to combo box

	// create view mode menu
	menu = new CommandMenu(m_pViewOptions, "spectatormodes", gViewPortInterface);
	menu->AddMenuItem("TV Camera", VarArgs("spec_mode %d", OBS_MODE_TVCAM), this);
	menu->AddMenuItem("Roaming", VarArgs("spec_mode %d", OBS_MODE_ROAMING), this);
	menu->AddMenuItem("Free Chase", VarArgs("spec_mode %d", OBS_MODE_CHASE), this);
	menu->AddMenuItem("Locked Chase", VarArgs("spec_mode %d", OBS_MODE_IN_EYE), this);
	m_pViewOptions->SetMenu(menu);	// attach menu to combo box

	LoadControlSettings("Resource/UI/BottomSpectator.res");
	ListenForGameEvent( "spec_target_updated" );
}

void CSpectatorMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	// need to MakeReadyForUse() on the menus so we can set their bg color before they are displayed
	m_pConfigSettings->GetMenu()->MakeReadyForUse();
	m_pViewOptions->GetMenu()->MakeReadyForUse();
	m_pPlayerList->GetMenu()->MakeReadyForUse();

	if ( g_pSpectatorGUI )
	{
		m_pConfigSettings->GetMenu()->SetBgColor( g_pSpectatorGUI->GetBlackBarColor() );
		m_pViewOptions->GetMenu()->SetBgColor( g_pSpectatorGUI->GetBlackBarColor() );
		m_pPlayerList->GetMenu()->SetBgColor( g_pSpectatorGUI->GetBlackBarColor() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CSpectatorMenu::PerformLayout()
{
	int w,h;
	GetHudSize(w, h);

	// fill the screen
	SetSize(w,GetTall());
}


//-----------------------------------------------------------------------------
// Purpose: Handles changes to combo boxes
//-----------------------------------------------------------------------------
void CSpectatorMenu::OnTextChanged(KeyValues *data)
{
	Panel *panel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );

	vgui::ComboBox *box = dynamic_cast<vgui::ComboBox *>( panel );

	if( box == m_pConfigSettings) // don't change the text in the config setting combo
	{
		m_pConfigSettings->SetText("#Spec_Options");
	}
	else if ( box == m_pPlayerList )
	{
		KeyValues *kv = box->GetActiveItemUserData();
		if ( kv && GameResources() )
		{
			const char *player = kv->GetString("player");

			int playernum;

			if (engine->IsHLTV())
			{
				if (HLTVCamera()->GetPrimaryTarget())
					playernum = HLTVCamera()->GetPrimaryTarget()->entindex();
				else
					playernum = 0;
			}
			else
				playernum = GetSpectatorTarget();

			bool update = false;

			if (!Q_stricmp(player, "ball"))
			{
				if (playernum <= gpGlobals->maxClients)
					update = true;
			}
			else
			{
				if (playernum >= 1 && playernum <= gpGlobals->maxClients)
				{
					const char *currentPlayerName = GameResources()->GetPlayerName( playernum );
					update = Q_strcmp(currentPlayerName, player) != 0;
				}
				else
					update = true;
			}

			if (update)
			{
				if (engine->IsHLTV())
					HLTVCamera()->SetPrimaryTarget(kv->GetInt("index"));
				else
					engine->ClientCmd( VarArgs("spec_player %d", kv->GetInt("index")) );
			}
		}
	}
}

void CSpectatorMenu::OnCommand( const char *command )
{
	if (!stricmp(command, "specnext") )
	{
		engine->ClientCmd("spec_next");
	}
	else if (!stricmp(command, "specprev") )
	{
		engine->ClientCmd("spec_prev");
	}
	else if (!strnicmp(command, "spec_mode", 9))
	{
		engine->ClientCmd(command);
	}
}

void CSpectatorMenu::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

 	if ( Q_strcmp( "spec_target_updated", pEventName ) == 0 )
	{
		IGameResources *gr = GameResources();
		if ( !gr )
			return;

		// make sure the player combo box is up to date
		int playernum;

		if (engine->IsHLTV())
		{
			if (HLTVCamera()->GetPrimaryTarget())
				playernum = HLTVCamera()->GetPrimaryTarget()->entindex();
			else
				playernum = 0;
		}
		else
			playernum = GetSpectatorTarget();

		if ( playernum < 1 || playernum > gpGlobals->maxClients )
		{
			m_pPlayerList->ActivateItemByRow(0);
			return;
		}

		const char *selectedPlayerName = gr->GetPlayerName( playernum );
		const char *currentPlayerName = "";
		KeyValues *kv = m_pPlayerList->GetActiveItemUserData();
		if ( kv )
		{
			currentPlayerName = kv->GetString( "player" );
		}
		if ( !FStrEq( currentPlayerName, selectedPlayerName ) )
		{
			for ( int i=0; i<m_pPlayerList->GetItemCount(); ++i )
			{
				KeyValues *kv = m_pPlayerList->GetItemUserData( i );
				if ( kv && FStrEq( kv->GetString( "player" ), selectedPlayerName ) )
				{
					m_pPlayerList->ActivateItemByRow( i );
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: when duck is pressed it hides the active part of the GUI
//-----------------------------------------------------------------------------
void CSpectatorMenu::OnKeyCodePressed(KeyCode code)
{
	if ( code == m_iDuckKey )
	{
		// hide if DUCK is pressed again
		m_pViewPort->ShowPanel( this, false );
	}
}

void CSpectatorMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
		SetKeyBoardInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
	}

	bool bIsEnabled = true;
	
	m_pLeftButton->SetVisible( bIsEnabled );
	m_pRightButton->SetVisible( bIsEnabled );
	m_pPlayerList->SetVisible( bIsEnabled );
	m_pViewOptions->SetVisible( bIsEnabled );
}

void CSpectatorMenu::Update( void )
{
	IGameResources *gr = GameResources();

	Reset();

	if ( m_iDuckKey == BUTTON_CODE_INVALID )
	{
		m_iDuckKey = gameuifuncs->GetButtonCodeForBind( "duck" );
	}

	if ( !gr )
		return;

	KeyValues *kv = new KeyValues( "UserData", "player", "ball", "index", "0" );
	m_pPlayerList->AddItem( L"Ball", kv );
	kv->deleteThis();

	int iPlayerIndex;
	for ( iPlayerIndex = 1 ; iPlayerIndex <= gpGlobals->maxClients; iPlayerIndex++ )
	{

		// does this slot in the array have a name?
		if ( !gr->IsConnected( iPlayerIndex ) )
			continue;

		if ( gr->IsLocalPlayer( iPlayerIndex ) )
			continue;

		if ( gr->GetTeam(iPlayerIndex) != TEAM_A && gr->GetTeam(iPlayerIndex) != TEAM_B )
			continue;

		wchar_t playerText[ 80 ], playerName[ 64 ], *team, teamName[ 64 ];
		char localizeTeamName[64];
		char szPlayerIndex[16];
		g_pVGuiLocalize->ConvertANSIToUnicode( UTIL_SafeName( gr->GetPlayerName(iPlayerIndex) ), playerName, sizeof( playerName ) );
		g_pVGuiLocalize->ConvertANSIToUnicode(gr->GetTeamCode( gr->GetTeam(iPlayerIndex) ), teamName, sizeof( teamName ) );

		_snwprintf(playerText, ARRAYSIZE(playerText), L"%s | %s", teamName, playerName);
		Q_snprintf( szPlayerIndex, sizeof( szPlayerIndex ), "%d", iPlayerIndex );

		KeyValues *kv = new KeyValues( "UserData", "player", gr->GetPlayerName( iPlayerIndex ), "index", szPlayerIndex );
		m_pPlayerList->AddItem( playerText, kv );
		kv->deleteThis();
	}

	// make sure the player combo box is up to date
	int playernum;

	if (engine->IsHLTV())
	{
		if (HLTVCamera()->GetPrimaryTarget())
			playernum = HLTVCamera()->GetPrimaryTarget()->entindex();
		else
			playernum = 0;
	}
	else
		playernum = GetSpectatorTarget();

	// If ball is spec target
	if (playernum > gpGlobals->maxClients)
	{
		m_pPlayerList->ActivateItemByRow(0);
	}
	else if (playernum >= 1 && playernum <= gpGlobals->maxClients)
	{
		const char *selectedPlayerName = gr->GetPlayerName( playernum );
		for ( iPlayerIndex=0; iPlayerIndex<m_pPlayerList->GetItemCount(); ++iPlayerIndex )
		{
			KeyValues *kv = m_pPlayerList->GetItemUserData( iPlayerIndex );
			if ( kv && FStrEq( kv->GetString( "player" ), selectedPlayerName ) )
			{
				m_pPlayerList->ActivateItemByRow( iPlayerIndex );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// main spectator panel



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSpectatorGUI::CSpectatorGUI(IViewPort *pViewPort) : EditablePanel( NULL, PANEL_SPECGUI )
{
// 	m_bHelpShown = false;
//	m_bInsetVisible = false;
//	m_iDuckKey = KEY_NONE;
	m_bSpecScoreboard = false;

	m_pViewPort = pViewPort;
	g_pSpectatorGUI = this;

	// initialize dialog
	SetVisible(false);
	SetProportional(true);

	// load the new scheme early!!
	//SetScheme("ClientScheme");
	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );

 	m_pBottomBarBlank = new Panel( this, "bottombarblank" );

	// m_pBannerImage = new ImagePanel( m_pTopBar, NULL );

	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	// m_pBannerImage->SetVisible(false);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSpectatorGUI::~CSpectatorGUI()
{
	g_pSpectatorGUI = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the colour of the top and bottom bars
//-----------------------------------------------------------------------------
void CSpectatorGUI::ApplySchemeSettings(IScheme *pScheme)
{
	LoadControlSettings("Resource/UI/Spectator.res");
	m_pBottomBarBlank->SetVisible( false );

	BaseClass::ApplySchemeSettings( pScheme );
	SetBgColor(Color( 0,0,0,0 ) ); // make the background transparent
	m_pBottomBarBlank->SetBgColor(GetBlackBarColor());
	// m_pBottomBar->SetBgColor(Color( 0,0,0,0 ));
	SetPaintBorderEnabled(false);

	SetBorder( NULL );

}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CSpectatorGUI::PerformLayout()
{
	int w,h,x,y;
	GetHudSize(w, h);
	
	// fill the screen
	SetBounds(0,0,w,h);

	// stretch the bottom bar across the screen
	m_pBottomBarBlank->GetPos(x,y);
	m_pBottomBarBlank->SetSize( w, h - y );
}

//-----------------------------------------------------------------------------
// Purpose: checks spec_scoreboard cvar to see if the scoreboard should be displayed
//-----------------------------------------------------------------------------
void CSpectatorGUI::OnThink()
{
	BaseClass::OnThink();

	if ( IsVisible() )
	{
		if ( m_bSpecScoreboard != spec_scoreboard.GetBool() )
		{
			if ( !spec_scoreboard.GetBool() || !gViewPortInterface->GetActivePanel() )
			{
				m_bSpecScoreboard = spec_scoreboard.GetBool();
				gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, m_bSpecScoreboard );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CSpectatorGUI::SetLabelText(const char *textEntryName, const char *text)
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
void CSpectatorGUI::SetLabelText(const char *textEntryName, wchar_t *text)
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
void CSpectatorGUI::MoveLabelToFront(const char *textEntryName)
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
void CSpectatorGUI::ShowPanel(bool bShow)
{
	if ( bShow && !IsVisible() )
	{
		m_bSpecScoreboard = false;
	}
	SetVisible( bShow );
	if ( !bShow && m_bSpecScoreboard )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, false );
	}
}

bool CSpectatorGUI::ShouldShowPlayerLabel( int specmode )
{
	return ( (specmode == OBS_MODE_IN_EYE) ||	(specmode == OBS_MODE_CHASE) );
}
//-----------------------------------------------------------------------------
// Purpose: Updates the gui, rearranges elements
//-----------------------------------------------------------------------------
void CSpectatorGUI::Update()
{
}

//-----------------------------------------------------------------------------
// Purpose: Updates the timer label if one exists
//-----------------------------------------------------------------------------
void CSpectatorGUI::UpdateTimer()
{
}

static void ForwardSpecCmdToServer( const CCommand &args )
{
	if ( engine->IsPlayingDemo() )
		return;

	if ( args.ArgC() == 1 )
	{
		// just forward the command without parameters
		engine->ServerCmd( args[ 0 ] );
	}
	else if ( args.ArgC() == 2 )
	{
		// forward the command with parameter
		char command[128];
		Q_snprintf( command, sizeof(command), "%s \"%s\"", args[ 0 ], args[ 1 ] );
		engine->ServerCmd( command );
	}
}

CON_COMMAND_F( spec_next, "Spectate next player", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	if ( engine->IsHLTV() )
	{
		HLTVCamera()->SpecNextPlayer( false );
	}
	else
	{
		ForwardSpecCmdToServer( args );
	}
}

CON_COMMAND_F( spec_prev, "Spectate previous player", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	if ( engine->IsHLTV() )
	{
		HLTVCamera()->SpecNextPlayer( true );
	}
	else
	{
		ForwardSpecCmdToServer( args );
	}
}

CON_COMMAND_F( spec_mode, "Set spectator mode", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	if ( engine->IsHLTV() )
	{
		// we can choose any mode, not loked to PVS
		int mode;

		if ( args.ArgC() == 2 )
		{
			// set specifc mode
			mode = Q_atoi( args[1] );
		}
		else
		{
			// set next mode 
			mode = HLTVCamera()->GetMode() + 1;

			if ( mode > LAST_PLAYER_OBSERVERMODE )
				mode = OBS_MODE_IN_EYE;
		}

		// handle the command clientside
		HLTVCamera()->SetMode( mode );
	}
	else
	{
		// we spectate on a game server, forward command
		ForwardSpecCmdToServer( args );
	}
}

CON_COMMAND_F( spec_player, "Spectate player by name", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	if ( args.ArgC() != 2 )
		return;

	if ( engine->IsHLTV() )
	{
		HLTVCamera()->SpecNamedPlayer( args[1] );
	}
	else
	{
		ForwardSpecCmdToServer( args );
	}
}



