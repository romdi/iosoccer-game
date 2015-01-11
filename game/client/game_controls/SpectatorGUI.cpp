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
#include "ios_camera.h"
#include "c_match_ball.h"

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Menu.h>
#include "IGameUIFuncs.h" // for key bindings
#include <imapoverview.h>
#include <shareddefs.h>
#include <igameresources.h>
#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif

using namespace vgui;

enum { PANEL_MARGIN = 5, PANEL_WIDTH = (1024 - 2 * PANEL_MARGIN) };
enum { MAINPANEL_HEIGHT = 50, MAINPANEL_MARGIN = 10, MAINPANEL_ITEM_MARGIN = 10 };
enum { PLAYERLIST_WIDTH = 250, PLAYERLIST_HEIGHT = 30 };
enum { CAMMODES_WIDTH = 130, CAMMODES_HEIGHT = 30 };
enum { TVCAMMODES_WIDTH = 130, TVCAMMODES_HEIGHT = 30 };
enum { BUTTON_WIDTH = 30, BUTTON_HEIGHT = 30, BUTTON_MARGIN = 5 };

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSpectatorMenu::CSpectatorMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_SPECMENU )
{
	SetScheme("ClientScheme");

	m_iDuckKey = BUTTON_CODE_INVALID;
		
	m_pViewPort = pViewPort;

	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( false );
	SetTitleBarVisible( false ); // don't draw a title bar
	SetMoveable( false );
	SetSizeable( false );
	SetProportional(false);

	m_pMainPanel = new Panel(this, "");

	m_pTargetList = new ComboBox(m_pMainPanel, "playercombo", 10 , false);
	m_pTargetList->SetOpenDirection(Menu::UP);
	
	CommandMenu *pTargetListMenu = new CommandMenu(m_pTargetList, "playercombo", gViewPortInterface);
	m_pTargetList->SetMenu(pTargetListMenu);


	m_pCamModes = new ComboBox(m_pMainPanel, "cammodes", 10 , false );
	m_pCamModes->SetOpenDirection(Menu::UP);

	CommandMenu *pCamModeMenu = new CommandMenu(m_pCamModes, "cammodes", gViewPortInterface);
	for (int i = 0; i < CAM_MODE_COUNT; i++)
	{
		pCamModeMenu->AddMenuItem(g_szCamModeNames[i], VarArgs("cam_mode %d", i), this);
	}
	m_pCamModes->SetMenu(pCamModeMenu);
	m_pCamModes->ActivateItemByRow(0);

	
	m_pTVCamModes = new ComboBox(m_pMainPanel, "tvcammodes", 10 , false );	
	m_pTVCamModes->SetOpenDirection(Menu::UP);

	CommandMenu *pTVCamModeMenu = new CommandMenu(m_pTVCamModes, "tvcammodes", gViewPortInterface);
	for (int i = 0; i < TVCAM_MODE_COUNT; i++)
	{
		pTVCamModeMenu->AddMenuItem(g_szTVCamModeNames[i], VarArgs("tvcam_mode %d", i), this);
	}
	m_pTVCamModes->SetMenu(pTVCamModeMenu);
	m_pTVCamModes->ActivateItemByRow(0);


	m_pLeftButton = new Button(m_pMainPanel, "", "<", this, "spec_prev");

	m_pRightButton = new Button(m_pMainPanel, "", ">", this, "spec_next");

	ListenForGameEvent("spec_target_updated");
	ListenForGameEvent("player_team");
	ListenForGameEvent("cam_mode_updated");
	ListenForGameEvent("tvcam_mode_updated");
}

void CSpectatorMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	// need to MakeReadyForUse() on the menus so we can set their bg color before they are displayed
	m_pTVCamModes->GetMenu()->MakeReadyForUse();
	m_pCamModes->GetMenu()->MakeReadyForUse();
	m_pTargetList->GetMenu()->MakeReadyForUse();

	HFont font = pScheme->GetFont("StatButton");

	//if ( g_pSpectatorGUI )
	//{
	//	m_pTVCamModes->GetMenu()->SetBgColor( g_pSpectatorGUI->GetBlackBarColor() );
	//	m_pCamModes->GetMenu()->SetBgColor( g_pSpectatorGUI->GetBlackBarColor() );
	//	m_pTargetList->GetMenu()->SetBgColor( g_pSpectatorGUI->GetBlackBarColor() );
	//}

	SetBounds(ScreenWidth() / 2 - PANEL_WIDTH / 2, 0, PANEL_WIDTH, ScreenHeight());
	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
	
	m_pMainPanel->SetBounds(0, GetTall() - MAINPANEL_HEIGHT - MAINPANEL_MARGIN, GetWide(), MAINPANEL_HEIGHT);
	
	m_pLeftButton->SetBounds(GetWide() / 2 - PLAYERLIST_WIDTH / 2 - BUTTON_WIDTH - BUTTON_MARGIN, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pLeftButton->SetFont(font);
	//m_pLeftButton->SetFgColor(Color(0, 0, 0, 255));
	m_pLeftButton->SetContentAlignment(Label::a_center);

	m_pTargetList->SetBounds(GetWide() / 2 - PLAYERLIST_WIDTH / 2, 0, PLAYERLIST_WIDTH, PLAYERLIST_HEIGHT);
	m_pTargetList->SetFont(font);
	//m_pTargetList->GetMenu()->SetFgColor(Color(0, 0, 0, 255));

	m_pRightButton->SetBounds(GetWide() / 2 + PLAYERLIST_WIDTH / 2 + BUTTON_MARGIN, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pRightButton->SetFont(font);
	//m_pRightButton->SetFgColor(Color(0, 0, 0, 255));
	m_pRightButton->SetContentAlignment(Label::a_center);
	
	m_pTVCamModes->SetBounds(GetWide() - CAMMODES_WIDTH - MAINPANEL_ITEM_MARGIN - TVCAMMODES_WIDTH, 0, TVCAMMODES_WIDTH, TVCAMMODES_HEIGHT);
	m_pTVCamModes->SetFont(font);
	//m_pTVCamModes->GetMenu()->SetFgColor(Color(0, 0, 0, 255));
	
	m_pCamModes->SetBounds(GetWide() - TVCAMMODES_WIDTH, 0, CAMMODES_WIDTH, CAMMODES_HEIGHT);
	m_pCamModes->SetFont(font);
	//m_pCamModes->GetMenu()->SetFgColor(Color(0, 0, 0, 255));
}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CSpectatorMenu::PerformLayout()
{
	//int w,h;
	//GetHudSize(w, h);

	// fill the screen
	//SetSize(w,GetTall());
}


//-----------------------------------------------------------------------------
// Purpose: Handles changes to combo boxes
//-----------------------------------------------------------------------------
void CSpectatorMenu::OnTextChanged(KeyValues *data)
{
}

void CSpectatorMenu::OnCommand( const char *command )
{
	if (!strcmp(command, "spec_next") )
	{
		engine->ClientCmd(command);
	}
	else if (!strcmp(command, "spec_prev") )
	{
		engine->ClientCmd(command);
	}
	else if (!strncmp(command, "spec_mode", 9))
	{
		engine->ClientCmd(command);
	}
	else if (!strncmp(command, "cam_mode", 8))
	{
		m_pTVCamModes->SetVisible(atoi(&command[9]) == CAM_MODE_TVCAM);
		engine->ClientCmd(command);
	}
	else if (!strncmp(command, "tvcam_mode", 10))
	{
		engine->ClientCmd(command);
	}
	else if (!strncmp(command, "spec_by_index", 13))
	{
		engine->ClientCmd(command);
	}
	else if (!strncmp(command, "spec_by_name", 12))
	{
		engine->ClientCmd(command);
	}
}

void CSpectatorMenu::FireGameEvent( IGameEvent * event )
{
 	if (!Q_strcmp("spec_target_updated", event->GetName()))
	{
		IGameResources *gr = GameResources();
		if ( !gr )
			return;

		// make sure the player combo box is up to date
		int playernum;

		// make sure the player combo box is up to date
		int targetIndex;

		if (Camera()->GetTarget())
			targetIndex = Camera()->GetTarget()->entindex();
		else
			targetIndex = 0;

		// If ball is spec target
		if (targetIndex > gpGlobals->maxClients)
		{
			m_pTargetList->ActivateItemByRow(0);
		}
		else if (targetIndex >= 1 && targetIndex <= gpGlobals->maxClients)
		{
			for (int i = 0; i < m_pTargetList->GetMenu()->GetItemCount(); i++)
			{
				if (m_pTargetList->GetMenu()->GetItemUserData(i)->GetInt("index") == targetIndex)
				{
					m_pTargetList->GetMenu()->ActivateItemByRow(i);
					break;
				}
			}
		}
	}
	else if (!Q_strcmp("cam_mode_updated", event->GetName()))
	{
		m_pCamModes->SetText(g_szCamModeNames[Camera()->GetCamMode()]);
		m_pTVCamModes->SetVisible(Camera()->GetCamMode() == CAM_MODE_TVCAM);
	}
	else if (!Q_strcmp("tvcam_mode_updated", event->GetName()))
	{
		m_pTVCamModes->SetText(g_szTVCamModeNames[Camera()->GetTVCamMode()]);
	}
	else if (!Q_strcmp("player_team", event->GetName()))
	{
		C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();
		if (!pLocal || pLocal->GetUserID() == event->GetInt("userid"))
		{
			SetVisible(event->GetInt("newteam") == TEAM_SPECTATOR);
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
		//m_pViewPort->ShowPanel( this, false );
		//SetMouseInputEnabled(!IsMouseInputEnabled());
		//SetKeyBoardInputEnabled(!IsKeyBoardInputEnabled());
	}
}

void CSpectatorMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();
	}
	else
	{
		SetVisible(false);
	}
}

bool CSpectatorMenu::NeedsUpdate()
{
	return IsVisible();
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

	if (!m_pTargetList->IsDropdownVisible())
	{
		m_pTargetList->GetMenu()->DeleteAllItems();

		KeyValues *pKV = new KeyValues("UserData", "index", GetMatchBall() ? GetMatchBall()->entindex() : 0);
		m_pTargetList->GetMenu()->AddMenuItem("Ball", VarArgs("spec_by_index %d", GetMatchBall() ? GetMatchBall()->entindex() : 0), this, pKV);
		pKV->deleteThis();

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			if (!gr->IsConnected(i) || gr->IsLocalPlayer(i) || gr->GetTeam(i) != TEAM_HOME && gr->GetTeam(i) != TEAM_AWAY)
				continue;

			KeyValues *pKV = new KeyValues("UserData", "index", i);
			char playerText[MAX_PLAYER_NAME_LENGTH * 2];
			Q_snprintf(playerText, sizeof(playerText), "%s | %s", GetGlobalTeam(gr->GetTeam(i))->GetCode(), gr->GetPlayerName(i));
			m_pTargetList->GetMenu()->AddMenuItem(playerText, VarArgs("spec_by_index %d", i), this, pKV);
			pKV->deleteThis();
		}

		m_pTargetList->GetMenu()->MakeReadyForUse();
	}
}

CON_COMMAND_F( spec_next, "Spectate next target", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	Camera()->SpecNextTarget(false);
}

CON_COMMAND_F( spec_prev, "Spectate previous target", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	Camera()->SpecNextTarget(true);
}

CON_COMMAND_F(spec_by_index, "Spectate target by index", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (!pPlayer || !pPlayer->IsObserver())
		return;

	if (args.ArgC() != 2)
		return;

	Camera()->SpecTargetByIndex(atoi(args[1]));
}

CON_COMMAND_F(spec_by_name, "Spectate target by name", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (!pPlayer || !pPlayer->IsObserver())
		return;

	if (args.ArgC() != 2)
		return;

	Camera()->SpecTargetByName(args[1]);
}

CON_COMMAND_F(cam_mode, "Set camera mode", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (!pPlayer || !pPlayer->IsObserver())
		return;

	int mode;

	if ( args.ArgC() == 2 )
		mode = Q_atoi(args[1]);
	else
		mode = (Camera()->GetCamMode() + 1) % CAM_MODE_COUNT;

	Camera()->SetCamMode(mode);
}

CON_COMMAND_F(tvcam_mode, "Set TV camera mode", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (!pPlayer || Camera()->GetCamMode() != CAM_MODE_TVCAM)
		return;

	int mode;

	if ( args.ArgC() == 2 )
		mode = Q_atoi(args[1]);
	else
		mode = (Camera()->GetTVCamMode() + 1) % CAM_MODE_COUNT;

	Camera()->SetTVCamMode(mode);
}
