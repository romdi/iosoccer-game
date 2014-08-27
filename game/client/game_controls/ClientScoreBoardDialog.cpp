//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include <stdio.h>

#include <cdll_client_int.h>
#include <cdll_util.h>
#include <globalvars_base.h>
#include <igameresources.h>
#include "IGameUIFuncs.h" // for key bindings
#include "inputsystem/iinputsystem.h"
#include "clientscoreboarddialog.h"
#include <voice_status.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vstdlib/IKeyValuesSystem.h>

#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ImagePanel.h>	//ios

#include <game/client/iviewport.h>
#include <igameresources.h>
#include "vgui_controls/TextImage.h"
#include "commandmenu.h"

#include <vgui/IInput.h>

#include "sdk_gamerules.h" //include before memdbgon.h		//IOS scoreboard

#include "vgui_avatarimage.h"
#include "c_team.h"

#include "c_match_ball.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

bool AvatarIndexLessFunc( const int &lhs, const int &rhs )	
{ 
	return lhs < rhs; 
}

bool CountryFlagIndexLessFunc( const int &lhs, const int &rhs )	
{ 
	return lhs < rhs; 
}

bool CardsIndexLessFunc( const int &lhs, const int &rhs )	
{ 
	return lhs < rhs; 
}


void CC_Gag(const CCommand &args)
{
	if (args.ArgC() < 2)
	{
		const int listSize = 2048;
		char list[listSize] = "\nindex: name\n";

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			if (!GameResources()->IsConnected(i) || i == GetLocalPlayerIndex())
				continue;

			Q_strcat(list, VarArgs("%d: %s\n", i, GameResources()->GetPlayerName(i)), listSize);
		}

		Q_strcat(list, "Use 'gag <player index>' to gag or ungag\n\n", listSize);

		Msg(list);

		return;
	}

	int playerIndex = atoi(args[1]);

	if (!GameResources()->IsConnected(playerIndex))
	{
		Msg("No player with index %d found\n", playerIndex);
		return;
	}

	if (GetClientVoiceMgr()->IsPlayerBlocked(playerIndex))
	{
		GetClientVoiceMgr()->SetPlayerBlockedState(playerIndex, false);
		Msg("Ungagged player %s\n", GameResources()->GetPlayerName(playerIndex));
	}
	else
	{
		GetClientVoiceMgr()->SetPlayerBlockedState(playerIndex, true);
		Msg("Gagged player %s\n", GameResources()->GetPlayerName(playerIndex));
	}
}

static ConCommand gag("gag", CC_Gag);


enum { PANEL_TOPMARGIN = 70, PANEL_MARGIN = 5, PANEL_WIDTH = (1280 - 2 * PANEL_MARGIN), PANEL_HEIGHT = (768 - 2 * PANEL_MARGIN) };
enum { TEAMCREST_SIZE = 48, TEAMCREST_VMARGIN = 7, TEAMCREST_HOFFSET = 240, TEAMCREST_VOFFSET = 10 };
enum { PLAYERLIST_HEIGHT = 400, PLAYERLIST_BOTTOMMARGIN = 10, PLAYERLISTDIVIDER_WIDTH = 8 };
enum { STATBUTTON_WIDTH = 120, STATBUTTON_HEIGHT = 30, STATBUTTON_HMARGIN = 0, STATBUTTON_VMARGIN = 5 };
enum { EXTRAINFO_HEIGHT = 313, EXTRAINFO_MARGIN = 5 };
enum { SPECLIST_HEIGHT = 30, SPECLIST_PADDING = 5, SPECNAME_WIDTH = 100, SPECTEXT_WIDTH = 150, SPECTEXT_MARGIN = 5, SPECBUTTON_WIDTH = 90, SPECBUTTON_HMARGIN = 5, SPECBUTTON_VMARGIN = 3 };
enum { TOPBAR_HEIGHT = 30 };
enum { SERVERINFO_WIDTH = 448, SERVERINFOLINE_HEIGHT = 2, SERVERINFOLINE_MARGIN = 5 };
enum { MATCHINFO_WIDTH = 448 };

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClientScoreBoardDialog::CClientScoreBoardDialog(IViewPort *pViewPort) : EditablePanel( NULL, PANEL_SCOREBOARD )
{
	SetScheme("ClientScheme");

	m_iPlayerIndexSymbol = KeyValuesSystem()->GetSymbolForString("playerindex");
	m_nCloseKey = BUTTON_CODE_INVALID;

	//memset(s_VoiceImage, 0x0, sizeof( s_VoiceImage ));
	TrackerImage = 0;
	m_pViewPort = pViewPort;

	// initialize dialog
	SetProportional(false);

	m_pMainPanel = new Panel(this);

	m_pExtraInfoPanel = new Panel(m_pMainPanel);

	m_pPlayerListDivider = new Panel(m_pMainPanel);

	m_pStatsMenu = new CStatsMenu(m_pExtraInfoPanel, "");
	m_pStatsMenu->SetVisible(false);
	m_pFormationMenu = new CFormationMenu(m_pExtraInfoPanel, "");
	m_pMatchEventMenu = new CMatchEventMenu(m_pExtraInfoPanel, "");
	m_pMatchEventMenu->SetVisible(false);

	for (int i = 0; i < 2; i++)
	{
		m_pPlayerList[i] = new SectionedListPanel(m_pMainPanel, "PlayerList");
		m_pPlayerList[i]->SetVerticalScrollbar(false);

		//LoadControlSettings("Resource/UI/ScoreBoard.res");
		m_pPlayerList[i]->SetVisible(false); // hide this until we load the images in applyschemesettings
		m_pPlayerList[i]->AddActionSignalTarget(this);
		m_pPlayerList[i]->EnableMouseEnterSelection(true);
	}

	m_HLTVSpectators = 0;

	// update scoreboard instantly if on of these events occure
	if (gameeventmanager)
	{
		ListenForGameEvent( "hltv_status" );
		ListenForGameEvent( "server_spawn" );
	}

	m_pImageList = NULL;

	m_mapAvatarsToImageList.SetLessFunc( AvatarIndexLessFunc );
	m_mapAvatarsToImageList.RemoveAll();
	memset( &m_iImageAvatars, 0, sizeof(int) * (MAX_PLAYERS+1) );

	m_mapCountryFlagsToImageList.SetLessFunc( CountryFlagIndexLessFunc );
	m_mapCountryFlagsToImageList.RemoveAll();
	memset( &m_iCountryFlags, 0, sizeof(int) * (MAX_PLAYERS+1) );

	m_mapCardsToImageList.SetLessFunc( CardsIndexLessFunc );
	m_mapCardsToImageList.RemoveAll();
	memset( &m_iCards, 0, sizeof(int) * (MAX_PLAYERS+1) );

	m_pStatButtonContainer = new Panel(m_pMainPanel);

	m_pStatText = new Label(m_pStatButtonContainer, "", "Statistics");

	for (int i = 0; i < STAT_CATEGORY_COUNT; i++)
	{
		m_pStatButtons[i] = new Button(m_pStatButtonContainer, g_szStatCategoryNames[i], g_szStatCategoryNames[i], this, VarArgs("stat:%d", i));
	}

	m_pSpectatorContainer = new Panel(m_pMainPanel);

	m_pSpectatorNames = new Panel(m_pSpectatorContainer);

	m_pSpectatorText = new Label(m_pSpectatorContainer, "", "");

	for (int i = 0; i < 3; i++)
		m_pSpectateButtons[i] = new Button(m_pMainPanel, "SpectateButton", (i == 0 ? "Spectate" : "Bench"), this, VarArgs("spectate %d", (i == 0 ? TEAM_SPECTATOR : TEAM_A + i - 1)));

	m_pSpecInfo = new Label(m_pMainPanel, "", "");

	m_pTopBar = new Panel(m_pMainPanel, "");

	m_pServerInfo = new Label(m_pMainPanel, "", "");
	m_pServerInfoLine = new Panel(m_pMainPanel, "");

	m_pMatchInfo = new Label(m_pMainPanel, "", "");

	m_pMatchPeriod = new Label(m_pMainPanel, "", "");

	m_pMatchEvents = new Button(m_pStatButtonContainer, "MatchEvents", "Events", this, "showmatchevents");

	m_pToggleCaptainMenu = new Button(m_pStatButtonContainer, "ToggleMenu", "Captain Menu", this, "togglemenu");
	m_pToggleCaptainMenu->SetVisible(false);

	m_pTimeoutHeader = new Label(m_pStatButtonContainer, "", "Timeout");
	m_pTimeoutInfo = new Label(m_pStatButtonContainer, "", "");
	m_pRequestTimeout = new Button(m_pStatButtonContainer, "", "Request", this, "requesttimeout");

	m_pToggleCaptaincy = new Button(m_pMainPanel, "", "Take Captaincy", this, "togglecaptaincy");

	m_nCurStat = DEFAULT_STATS;
	m_nCurSpecIndex = 0;
	m_pCurSpecButton = NULL;

	m_nSelectedPlayerIndex = 0;

	m_eActivePanelType = FORMATION_MENU_NORMAL;

	m_bShowCaptainMenu = false;

	m_bCanSetSetpieceTaker = false;

	MakePopup();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClientScoreBoardDialog::~CClientScoreBoardDialog()
{
	if ( NULL != m_pImageList )
	{
		delete m_pImageList;
		m_pImageList = NULL;
	}
}

//-----------------------------------------------------------------------------
// Call every frame
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::OnThink()
{
	BaseClass::OnThink();

	// NOTE: this is necessary because of the way input works.
	// If a key down message is sent to vgui, then it will get the key up message
	// Sometimes the scoreboard is activated by other vgui menus, 
	// sometimes by console commands. In the case where it's activated by
	// other vgui menus, we lose the key up message because this panel
	// doesn't accept keyboard input. It *can't* accept keyboard input
	// because another feature of the dialog is that if it's triggered
	// from within the game, you should be able to still run around while
	// the scoreboard is up. That feature is impossible if this panel accepts input.
	// because if a vgui panel is up that accepts input, it prevents the engine from
	// receiving that input. So, I'm stuck with a polling solution.
	// 
	// Close key is set to non-invalid when something other than a keybind
	// brings the scoreboard up, and it's set to invalid as soon as the 
	// dialog becomes hidden.
	if ( m_nCloseKey != BUTTON_CODE_INVALID )
	{
		if ( !g_pInputSystem->IsButtonDown( m_nCloseKey ) )
		{
			m_nCloseKey = BUTTON_CODE_INVALID;
			gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, false );
			GetClientVoiceMgr()->StopSquelchMode();
		}
	}
}

//-----------------------------------------------------------------------------
// Called by vgui panels that activate the client scoreboard
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::OnPollHideCode( int code )
{
	m_nCloseKey = (ButtonCode_t)code;
}

//-----------------------------------------------------------------------------
// Purpose: clears everything in the scoreboard and all it's state
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::Reset()
{
	for (int i = 0; i < 2; i++)
	{
		// clear
		m_pPlayerList[i]->DeleteAllItems();
		m_pPlayerList[i]->RemoveAllSections();
	}

	m_pStatsMenu->Reset();
	m_pFormationMenu->Reset();
	m_pMatchEventMenu->Reset();

	m_iSectionId = 0;
	m_fNextUpdateTime = 0;
	// add all the sections
	AddHeader();

	for (int i = 0; i < 2; i++)
	{
		KeyValues *kv = new KeyValues("data");
		kv->SetInt("posindex", 0);
		m_pPlayerList[i]->AddItem(0, kv);
		kv->deleteThis();
		m_pPlayerList[i]->SetItemFont(0, m_pScheme->GetFont("IOSTeamMenuNormalBold"));
		m_pPlayerList[i]->SetLineSpacing(400 / (mp_maxplayers.GetInt() + 2.5f));

		for (int j = 0; j < mp_maxplayers.GetInt(); j++)
		{
			kv = new KeyValues("data");

			if (GameResources())
				kv->SetString("posname", g_szPosNames[(int)GetGlobalTeam(GameResources()->GetTeam(TEAM_A + i))->GetFormation()->positions[j]->type]);

			kv->SetInt("posindex", j + 1);
			m_pPlayerList[i]->AddItem(0, kv);
			kv->deleteThis();
			m_pPlayerList[i]->SetItemFont(j + 1, m_pScheme->GetFont("IOSTeamMenuNormal"));

			if (GetGlobalTeam(TEAM_A + i))
				m_pPlayerList[i]->SetItemFgColor(j + 1, g_ColorGray);

		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets up screen
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::ApplySchemeSettings( IScheme *pScheme )
{
	m_pScheme = pScheme;

	BaseClass::ApplySchemeSettings( pScheme );

	SetBounds(0, 0, ScreenWidth(), ScreenHeight());

	//m_pMainPanel->SetPaintBackgroundType(2);
	m_pMainPanel->SetBgColor(Color(0, 0, 0, 247));
	//m_pMainPanel->SetBounds(GetWide() / 2 - PANEL_WIDTH / 2, PANEL_TOPMARGIN, PANEL_WIDTH, PANEL_HEIGHT);
	m_pMainPanel->SetBounds(GetWide() / 2 - PANEL_WIDTH / 2, GetTall() / 2 - PANEL_HEIGHT / 2, PANEL_WIDTH, PANEL_HEIGHT);
	m_pMainPanel->SetPaintBorderEnabled(false);
	//m_pMainPanel->SetBorder(m_pScheme->GetBorder("BrightBorder"));

	if ( m_pImageList )
		delete m_pImageList;

	m_pImageList = new ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();
	memset( &m_iImageAvatars, 0, sizeof(int) * (MAX_PLAYERS+1) );

	m_mapCountryFlagsToImageList.RemoveAll();
	memset( &m_iCountryFlags, 0, sizeof(int) * (MAX_PLAYERS+1) );

	m_mapCardsToImageList.RemoveAll();
	memset( &m_iCards, 0, sizeof(int) * (MAX_PLAYERS+1) );

	m_pExtraInfoPanel->SetBounds(EXTRAINFO_MARGIN, PLAYERLIST_HEIGHT + PLAYERLIST_BOTTOMMARGIN + SPECLIST_HEIGHT + EXTRAINFO_MARGIN, m_pMainPanel->GetWide() - 2 * EXTRAINFO_MARGIN, EXTRAINFO_HEIGHT);
	//m_pExtraInfoPanel->SetBgColor(Color(255, 0, 0, 200));

	m_pStatsMenu->SetBounds(0, 0, m_pStatsMenu->GetParent()->GetWide(), m_pStatsMenu->GetParent()->GetTall());
	m_pStatsMenu->PerformLayout();

	m_pFormationMenu->SetBounds(0, 0, m_pExtraInfoPanel->GetWide(), m_pExtraInfoPanel->GetTall());
	m_pFormationMenu->PerformLayout();

	m_pMatchEventMenu->SetBounds(0, 0, m_pExtraInfoPanel->GetWide(), m_pExtraInfoPanel->GetTall());
	m_pMatchEventMenu->PerformLayout();

	m_pStatButtonContainer->SetBounds(m_pMainPanel->GetWide() / 2 - (STATBUTTON_WIDTH + 2 * STATBUTTON_HMARGIN) / 2, m_pExtraInfoPanel->GetY(), STATBUTTON_WIDTH + 2 * STATBUTTON_HMARGIN, 6 * STATBUTTON_HEIGHT + STAT_CATEGORY_COUNT * STATBUTTON_HEIGHT - 5 - STATBUTTON_HEIGHT);
	m_pStatButtonContainer->SetZPos(1);

	for (int i = 0; i < 3; i++)
	{
		if (i == 0)
			m_pSpectateButtons[i]->SetBounds(m_pMainPanel->GetWide() / 2 - STATBUTTON_WIDTH / 2, m_pMainPanel->GetTall() - 5 - STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
		else
			m_pSpectateButtons[i]->SetBounds(i == 1 ? 5 : m_pMainPanel->GetWide() - 85, m_pMainPanel->GetTall() - 5 - STATBUTTON_HEIGHT, 80, STATBUTTON_HEIGHT);

		m_pSpectateButtons[i]->SetFont(m_pScheme->GetFont("StatButton"));
		m_pSpectateButtons[i]->SetContentAlignment(Label::a_center);
		m_pSpectateButtons[i]->SetPaintBorderEnabled(false);
		m_pSpectateButtons[i]->SetCursor(dc_hand);
		//m_pSpectateButtons[i]->SetVisible(false);
	}

	m_pToggleCaptaincy->SetSize(80, STATBUTTON_HEIGHT);
	m_pToggleCaptaincy->SetFont(m_pScheme->GetFont("StatButton"));
	m_pToggleCaptaincy->SetContentAlignment(Label::a_center);
	m_pToggleCaptaincy->SetPaintBorderEnabled(false);
	m_pToggleCaptaincy->SetCursor(dc_hand);
	m_pToggleCaptaincy->SetVisible(false);

	m_pMatchEvents->SetBounds(STATBUTTON_HMARGIN, 0 * STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
	m_pMatchEvents->SetFont(m_pScheme->GetFont("StatButton"));
	m_pMatchEvents->SetContentAlignment(Label::a_center);
	m_pMatchEvents->SetPaintBorderEnabled(false);
	//m_pMatchEvents->SetVisible(false);

	m_pToggleCaptainMenu->SetBounds(STATBUTTON_HMARGIN, 2 * STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
	m_pToggleCaptainMenu->SetFont(m_pScheme->GetFont("StatButton"));
	m_pToggleCaptainMenu->SetContentAlignment(Label::a_center);
	m_pToggleCaptainMenu->SetPaintBorderEnabled(false);
	m_pToggleCaptainMenu->SetCursor(dc_hand);

	m_pStatText->SetBounds(STATBUTTON_HMARGIN, 3 * STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
	m_pStatText->SetFont(m_pScheme->GetFont("StatButton"));
	m_pStatText->SetContentAlignment(Label::a_center);

	m_pTimeoutHeader->SetBounds(STATBUTTON_HMARGIN, 4 * STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
	m_pTimeoutHeader->SetFont(m_pScheme->GetFont("StatButton"));
	m_pTimeoutHeader->SetContentAlignment(Label::a_center);
	m_pTimeoutHeader->SetVisible(false);

	m_pTimeoutInfo->SetBounds(STATBUTTON_HMARGIN, 5 * STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
	m_pTimeoutInfo->SetFont(m_pScheme->GetFont("StatButton"));
	m_pTimeoutInfo->SetContentAlignment(Label::a_center);
	m_pTimeoutInfo->SetVisible(false);

	m_pRequestTimeout->SetBounds(STATBUTTON_HMARGIN, 6 * STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
	m_pRequestTimeout->SetFont(m_pScheme->GetFont("StatButton"));
	m_pRequestTimeout->SetContentAlignment(Label::a_center);
	m_pRequestTimeout->SetPaintBorderEnabled(false);
	m_pRequestTimeout->SetCursor(dc_hand);
	m_pRequestTimeout->SetVisible(false);

	for (int i = 0; i < STAT_CATEGORY_COUNT; i++)
	{
		m_pStatButtons[i]->SetBounds(STATBUTTON_HMARGIN, 4 * STATBUTTON_HEIGHT + i * STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
		m_pStatButtons[i]->SetFont(m_pScheme->GetFont("StatButton"));
		m_pStatButtons[i]->SetContentAlignment(Label::a_center);
		m_pStatButtons[i]->SetPaintBorderEnabled(false);
	}

	m_pSpectatorContainer->SetBounds(0, PLAYERLIST_HEIGHT + PLAYERLIST_BOTTOMMARGIN, m_pMainPanel->GetWide(), SPECLIST_HEIGHT);
	m_pSpectatorContainer->SetBgColor(Color(0, 0, 0, 150));
	//m_pSpectatorContainer->SetPaintBackgroundType(2);

	m_pSpectatorFontList[0] = m_pScheme->GetFont("SpectatorListNormal");
	m_pSpectatorFontList[1] = m_pScheme->GetFont("SpectatorListSmall");
	m_pSpectatorFontList[2] = m_pScheme->GetFont("SpectatorListSmaller");
	m_pSpectatorFontList[3] = m_pScheme->GetFont("SpectatorListSmallest");

	m_pSpectatorNames->SetBounds(SPECLIST_PADDING + SPECTEXT_WIDTH + SPECTEXT_MARGIN, 0, m_pSpectatorContainer->GetWide() - (SPECLIST_PADDING + SPECTEXT_WIDTH + SPECTEXT_MARGIN), SPECLIST_HEIGHT);
	
	m_pSpectatorText->SetBounds(SPECLIST_PADDING, 0, SPECTEXT_WIDTH, SPECLIST_HEIGHT);
	m_pSpectatorText->SetFont(m_pScheme->GetFont("SpectatorListNormal"));

	m_pSpecInfo->SetBounds(0, m_pSpectatorContainer->GetY() - SPECLIST_HEIGHT, m_pMainPanel->GetWide(), SPECLIST_HEIGHT);
	m_pSpecInfo->SetZPos(10);
	m_pSpecInfo->SetBgColor(Color(0, 0, 0, 240));
	m_pSpecInfo->SetFont(m_pScheme->GetFont("SpectatorListNormal"));
	m_pSpecInfo->SetContentAlignment(Label::a_center);
	m_pSpecInfo->SetVisible(false);

	m_pTopBar->SetBounds(0, 0, m_pMainPanel->GetWide(), TOPBAR_HEIGHT);
	m_pTopBar->SetBgColor(Color(0, 0, 0, 150));

	m_pServerInfo->SetBounds(0, 0, SERVERINFO_WIDTH, TOPBAR_HEIGHT);
	m_pServerInfo->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
	m_pServerInfo->SetContentAlignment(Label::a_west);
	m_pServerInfo->SetFgColor(Color(255, 255, 255, 255));
	//m_pServerInfo->SetBgColor(Color(0, 0, 0, 150));
	m_pServerInfo->SetTextInset(5, 0);

	m_pMatchInfo->SetBounds(m_pMainPanel->GetWide() - MATCHINFO_WIDTH, 0, MATCHINFO_WIDTH, TOPBAR_HEIGHT);
	m_pMatchInfo->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
	m_pMatchInfo->SetContentAlignment(Label::a_east);
	m_pMatchInfo->SetFgColor(Color(255, 255, 255, 255));
	//m_pMatchInfo->SetBgColor(Color(0, 0, 0, 150));
	m_pMatchInfo->SetTextInset(5, 0);

	m_pMatchPeriod->SetBounds(SERVERINFO_WIDTH, 0, m_pMainPanel->GetWide() - SERVERINFO_WIDTH - MATCHINFO_WIDTH, TOPBAR_HEIGHT);
	m_pMatchPeriod->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
	m_pMatchPeriod->SetContentAlignment(Label::a_center);
	m_pMatchPeriod->SetFgColor(Color(255, 255, 255, 255));
	//m_pMatchPeriod->SetBgColor(Color(0, 0, 0, 150));

	m_pServerInfoLine->SetBounds(SERVERINFOLINE_MARGIN, TOPBAR_HEIGHT, m_pMainPanel->GetWide() - 2 * SERVERINFOLINE_MARGIN, SERVERINFOLINE_HEIGHT);
	m_pServerInfoLine->SetBgColor(Color(255, 255, 255, 255));
	m_pServerInfoLine->SetVisible(false);

	m_pPlayerListDivider->SetBounds(m_pMainPanel->GetWide() / 2 - PLAYERLISTDIVIDER_WIDTH / 2, 0, PLAYERLISTDIVIDER_WIDTH, PLAYERLIST_HEIGHT + PLAYERLIST_BOTTOMMARGIN);
	m_pPlayerListDivider->SetBgColor(Color(0, 0, 0, 150));
	m_pPlayerListDivider->SetVisible(false);

	for (int i = 0; i < 2; i++)
	{
		m_pPlayerList[i]->SetBounds(i * (m_pMainPanel->GetWide() / 2), TOPBAR_HEIGHT, m_pMainPanel->GetWide() / 2, PLAYERLIST_HEIGHT);
		m_pPlayerList[i]->SetPaintBorderEnabled(false);
	}

	for (int i = 0; i < 2; i++)
	{
		if (m_pPlayerList[i])
		{
			m_pPlayerList[i]->SetImageList( m_pImageList, false );
			m_pPlayerList[i]->SetVisible( true );
		}
	}

	input()->SetCursorPos(ScreenWidth() / 2, m_pMainPanel->GetY() + m_pExtraInfoPanel->GetY() - 60);

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::ShowPanel(bool bShow)
{
	// Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
	// going from windowed <-> fullscreen
	if ( m_pImageList == NULL )
	{
		InvalidateLayout( true, true );
	}

	if ( !bShow )
	{
		m_nCloseKey = BUTTON_CODE_INVALID;
	}

	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		//Reset();
		Update();
		SetVisible( true );
		MoveToFront();
		RequestFocus();
		SetKeyBoardInputEnabled(false);
		SetMouseInputEnabled(true);
		input()->SetCursorPos(m_nCursorPosX, m_nCursorPosY);
		SetHighlightedPlayer(0);
	}
	else
	{
		input()->GetCursorPosition(m_nCursorPosX, m_nCursorPosY);
		BaseClass::SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
	}
}

void CClientScoreBoardDialog::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "hltv_status") == 0 )
	{
		// spectators = clients - proxies
		m_HLTVSpectators = event->GetInt( "clients" );
		m_HLTVSpectators -= event->GetInt( "proxies" );
	}

	else if ( Q_strcmp(type, "server_spawn") == 0 )
	{
		// We'll post the message ourselves instead of using SetControlString()
		// so we don't try to translate the hostname.
		const char *hostname = event->GetString( "hostname" );
		Panel *control = FindChildByName( "ServerName" );
		if ( control )
		{
			PostMessage( control, new KeyValues( "SetText", "text", hostname ) );
			control->MoveToFront();

			control->SetFgColor(Color(255,255,255,255));		//server name colour ios
		}
	}

	if( IsVisible() )
		Update();

}

bool CClientScoreBoardDialog::NeedsUpdate( void )
{
	return (m_fNextUpdateTime < gpGlobals->curtime);	
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the internal scoreboard data
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::Update( void )
{
	IGameResources *gr = GameResources();
	if (!gr)
		return;

	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();
	if (!pLocal)
		return;

	static int oldMaxPlayers = 0;

	if (mp_maxplayers.GetInt() != oldMaxPlayers)
	{
		oldMaxPlayers = mp_maxplayers.GetInt();
		Reset();
	}

	UpdatePlayerInfo();
	UpdateTeamInfo();

	m_pServerInfo->SetText(mp_serverinfo.GetString());
	char mapname[MAX_MAP_NAME];
	Q_FileBase(engine->GetLevelName(), mapname, sizeof(mapname));
	m_pMatchInfo->SetText(VarArgs("%s @ %s", mp_matchinfo.GetString(), mapname));
	m_pMatchPeriod->SetText(g_szMatchPeriodShortNames[SDKGameRules()->State_Get()]);

	if (m_eActivePanelType == STATS_MENU)
	{
		if (m_nSelectedPlayerIndex > 0 && !gr->IsConnected(m_nSelectedPlayerIndex))
		{
			m_nSelectedPlayerIndex = 0;
			m_eActivePanelType = FORMATION_MENU_NORMAL;
		}

		if (m_nSelectedPlayerIndex != 0)
		{
			m_pFormationMenu->SetVisible(false);
			m_pStatsMenu->SetVisible(true);
			m_pStatButtonContainer->SetVisible(false);
			m_pMatchEventMenu->SetVisible(false);
		}
	}

	if (m_eActivePanelType == MATCHEVENT_MENU)
	{
		m_pFormationMenu->SetVisible(false);
		m_pStatsMenu->SetVisible(false);
		m_pMatchEventMenu->Update();
		m_pMatchEventMenu->SetVisible(true);
		m_pStatButtonContainer->SetVisible(true);
	}
	
	if (m_eActivePanelType == FORMATION_MENU_NORMAL || m_eActivePanelType == FORMATION_MENU_HIGHLIGHT)
	{
		m_pFormationMenu->Update(m_bShowCaptainMenu);
		m_pFormationMenu->SetVisible(true);
		m_pStatsMenu->SetVisible(false);
		m_pStatButtonContainer->SetVisible(true);
		m_pMatchEventMenu->SetVisible(false);

		bool isOnField = (pLocal->GetTeamNumber() == TEAM_A || pLocal->GetTeamNumber() == TEAM_B);

		if (pLocal->GetTeamNumber() == TEAM_A && mp_captaincy_home.GetBool() || pLocal->GetTeamNumber() == TEAM_B && mp_captaincy_away.GetBool())
		{
			if (GetGlobalTeam(GetLocalPlayerTeam())->GetCaptainPosIndex() == gr->GetTeamPosIndex(GetLocalPlayerIndex()))
				m_pToggleCaptaincy->SetText("- Captain");
			else
				m_pToggleCaptaincy->SetText("+ Captain");

			m_pToggleCaptaincy->SetPos(GetLocalPlayerTeam() == TEAM_A ? 5 : m_pMainPanel->GetWide() - 85, m_pMainPanel->GetTall() - 2 * (5 + STATBUTTON_HEIGHT));
			m_pToggleCaptaincy->SetVisible(true);
		}
		else
			m_pToggleCaptaincy->SetVisible(false);

		m_pSpectateButtons[0]->SetVisible(isOnField || gr->GetSpecTeam(GetLocalPlayerIndex()) != TEAM_SPECTATOR);
		m_pSpectateButtons[1]->SetVisible(isOnField || gr->GetSpecTeam(GetLocalPlayerIndex()) != TEAM_A);
		m_pSpectateButtons[2]->SetVisible(isOnField || gr->GetSpecTeam(GetLocalPlayerIndex()) != TEAM_B);
	}
	else
	{
		for (int i = 0; i < 3; i++)
			m_pSpectateButtons[i]->SetVisible(false);
		m_pToggleCaptaincy->SetVisible(false);
	}

	if ((pLocal->GetTeamNumber() == TEAM_A || pLocal->GetTeamNumber() == TEAM_B)
		&& pLocal->GetTeam()->GetCaptainPosIndex() == gr->GetTeamPosIndex(GetLocalPlayerIndex()))
	{
		// Local player is captain, so show the menu toggle button
		if (!m_pToggleCaptainMenu->IsVisible())
			m_pToggleCaptainMenu->SetVisible(true);
	}
	else
	{
		// Show the statistics menu instead
		if (m_pToggleCaptainMenu->IsVisible())
		{
			m_bShowCaptainMenu = false;
			ToggleMenu();
			m_pToggleCaptainMenu->SetVisible(false);
		}
	}

	if (m_bShowCaptainMenu)
	{
		wchar_t text[32];
		_snwprintf(text, sizeof(text) / sizeof(wchar_t), L"(%d • %d:%02dm)", pLocal->GetTeam()->GetTimeoutsLeft(), pLocal->GetTeam()->GetTimeoutTimeLeft() / 60, pLocal->GetTeam()->GetTimeoutTimeLeft() % 60);
		m_pTimeoutInfo->SetText(text);

		if (SDKGameRules()->m_eTimeoutState != TIMEOUT_STATE_NONE && SDKGameRules()->m_nTimeoutTeam == pLocal->GetTeamNumber())
		{
			m_pRequestTimeout->SetText("End Timeout");
			m_pRequestTimeout->SetCommand("endtimeout");
			m_pRequestTimeout->SetEnabled(true);
		}
		else
		{
			m_pRequestTimeout->SetText("Start Timeout");
			m_pRequestTimeout->SetCommand("requesttimeout");
			m_pRequestTimeout->SetEnabled(SDKGameRules()->m_eTimeoutState == TIMEOUT_STATE_NONE);
		}
	}

	if (GetGlobalTeam(GetLocalPlayerTeam())->GetCaptainPosIndex() == gr->GetTeamPosIndex(GetLocalPlayerIndex())
		&& GetMatchBall()
		&& (GetMatchBall()->State_Get() == BALL_STATE_FREEKICK
			|| GetMatchBall()->State_Get() == BALL_STATE_CORNER
			|| GetMatchBall()->State_Get() == BALL_STATE_GOALKICK
			|| GetMatchBall()->State_Get() == BALL_STATE_THROWIN
			|| GetMatchBall()->State_Get() == BALL_STATE_PENALTY))
	{
		m_bCanSetSetpieceTaker = true;
	}
	else
		m_bCanSetSetpieceTaker = false;

	for (int i = 0; i < 2; i++)
		m_pPlayerList[i]->SetCursor(m_bCanSetSetpieceTaker && i == GetLocalPlayerTeam() - TEAM_A ? dc_hand : dc_arrow);

	m_fNextUpdateTime = gpGlobals->curtime + 0.25f; 
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdateTeamInfo()
{
	if (!GameResources())
		return;

	for (int i = 0; i < 2; i++)
	{
		KeyValues *kv = new KeyValues("data");
		GetTeamInfo(i + TEAM_A, kv);
		m_pPlayerList[i]->ModifyItem(0, 0, kv);

		if (m_nSelectedPlayerIndex == i - 2)
			m_pStatsMenu->Update(m_nSelectedPlayerIndex, kv);

		kv->deleteThis();

		m_pPlayerList[i]->SetItemFgColor(0, g_ColorWhite);

		for (int j = 0; j <= m_pPlayerList[i]->GetHighestItemID(); j++)
		{
			if (m_pPlayerList[i]->IsItemIDValid(j))
			{
				m_pPlayerList[i]->SetItemDividerColor(j, Color(0, 0, 0, 0));
			}
		}

		m_pPlayerList[i]->SetItemDividerColor(0, g_ColorGray);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdatePlayerInfo()
{
	IGameResources *gr = GameResources();
	if (!gr)
		return;

	Color black = Color(0, 0, 0, 255);
	Color darker = Color(75, 75, 75, 255);
	Color dark = Color(125, 125, 125, 255);
	Color light = Color(175, 175, 175, 255);
	Color lighter = Color(225, 225, 225, 255);

	//m_SpecNames.RemoveAll();

	//while (m_pSpectatorContainer->GetChildCount() > 0)
	//{
	//	m_pSpectatorContainer->GetChild(0)->DeletePanel();
	//}

	int playerIndexAtPos[2][11] = {};

	CUtlVector<SpecInfo> specList;

	// walk all the players and make sure they're in the scoreboard
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!gr->IsConnected(i))
			continue;
		
		if (gr->GetTeam(i) != TEAM_A && gr->GetTeam(i) != TEAM_B)
		{
			SpecInfo info;
			info.playerIndex = i;
			Q_strncpy(info.playerName, gr->GetPlayerName(i), sizeof(info.playerName));

			specList.AddToTail(info);

			KeyValues *playerData = new KeyValues("data");
			GetPlayerInfo(i, playerData);

			if (i == m_nSelectedPlayerIndex)
				m_pStatsMenu->Update(m_nSelectedPlayerIndex, playerData);

			playerData->deleteThis();

			if (gr->GetTeamToJoin(i) == TEAM_INVALID)
				continue;
		}
		else
		{
			playerIndexAtPos[gr->GetTeam(i) - TEAM_A][gr->GetTeamPosIndex(i)] = i;

			// add the player to the list
			KeyValues *playerData = new KeyValues("data");
			GetPlayerInfo( i, playerData );

			UpdatePlayerAvatar( i, playerData );

			int side = -1;
			int team = gr->GetTeam(i); //omega; set a variable to team so we can reuse it
			int teamIndex = team - TEAM_A;
			int sectionID = 0;//iTeamSections[playerTeam]; //omega; make sure it goes into the proper section

			int itemID = gr->GetTeamPosIndex(i) + 1;

			m_pPlayerList[teamIndex]->ModifyItem( itemID, 0, playerData );

			// set the row color based on the players team
			m_pPlayerList[teamIndex]->SetItemFgColor(itemID, GetGlobalTeam(team)->GetHudKitColor());

			if (i == m_nSelectedPlayerIndex)
				m_pStatsMenu->Update(m_nSelectedPlayerIndex, playerData);

			playerData->deleteThis();
		}

		int team = gr->GetTeamToJoin(i) != TEAM_INVALID ? gr->GetTeamToJoin(i) : gr->GetTeam(i);
		int teamIndex = team - TEAM_A;
	}

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < mp_maxplayers.GetInt(); j++)
		{
			if (playerIndexAtPos[i][j] == 0)
			{
				KeyValues *emptyData = new KeyValues("data");
				emptyData->SetString("posname", g_szPosNames[(int)GetGlobalTeam(gr->GetTeam(TEAM_A + i))->GetFormation()->positions[j]->type]);
				emptyData->SetInt("posindex", j + 1);
				m_pPlayerList[i]->ModifyItem(j + 1, 0, emptyData);
				emptyData->deleteThis();
				m_pPlayerList[i]->SetItemFgColor(j + 1, g_ColorGray);
			}
		}
	}

	wchar_t specText[32];
	_snwprintf(specText, sizeof(specText) / sizeof(wchar_t), L"%d on TV • %d %s:", m_HLTVSpectators, specList.Count(), (specList.Count() == 1 ? L"spec" : L"specs"));
	m_pSpectatorText->SetText(specText);

	if (specList.Count() != m_SpecList.Count())
	{
		while (specList.Count() > m_pSpectatorNames->GetChildCount())
		{
			new Button(m_pSpectatorNames, "", "");
		}

		while (specList.Count() < m_pSpectatorNames->GetChildCount())
		{
			m_pSpectatorNames->GetChild(m_pSpectatorNames->GetChildCount() - 1)->DeletePanel();
		}
	}

	m_SpecList.CopyArray(specList.Base(), specList.Count());

	int maxNameLength = MAX_PLAYER_NAME_LENGTH;
	int totalWidth = 0;

	do
	{
		totalWidth = 0;

		for (int i = 0; i < specList.Count(); i++)
		{
			Button *pPl = (Button *)m_pSpectatorNames->GetChild(i);

			wchar_t nameFormat[16];
			_snwprintf(nameFormat, sizeof(nameFormat) / sizeof(wchar_t), L"%%.%ds", clamp(maxNameLength, 1, MAX_PLAYER_NAME_LENGTH));

			wchar_t fullName[64];
			g_pVGuiLocalize->ConvertANSIToUnicode(specList[i].playerName, fullName, sizeof(fullName));

			wchar_t truncatedName[64];
			_snwprintf(truncatedName, sizeof(truncatedName) / sizeof(wchar_t), nameFormat, fullName);

			int nextCardJoin = gr->GetNextCardJoin(specList[i].playerIndex);

			wchar_t cardText[64] = {};
			if (SDKGameRules()->GetMatchDisplayTimeSeconds(true, false) < nextCardJoin)
				_snwprintf(cardText, sizeof(cardText) / sizeof(wchar_t), L" [%d:%02d]", nextCardJoin / 60, nextCardJoin % 60);

			wchar_t separator[16] = {};
			if (i > 0)
				_snwprintf(separator, sizeof(separator) / sizeof(wchar_t), L"• ");

			wchar_t text[64];
			_snwprintf(text, sizeof(text) / sizeof(wchar_t), L"%s%s%s", separator, truncatedName, cardText);
			pPl->SetText(text);

			pPl->SetCommand(VarArgs("specindex:%d", specList[i]));
			pPl->AddActionSignalTarget(this);
			pPl->SetBounds(totalWidth, 0, SPECNAME_WIDTH, SPECLIST_HEIGHT);
			pPl->SetDefaultColor(gr->GetPlayerColor(specList[i].playerIndex), Color(0, 0, 0, 0));
			pPl->SetArmedColor(gr->GetPlayerColor(specList[i].playerIndex), Color(75, 75, 75, 150));
			pPl->SetFont(m_pSpectatorFontList[1]);
			pPl->SetContentAlignment(Label::a_center);
			//pPl->SetPaintBackgroundEnabled(false);
			pPl->SetPaintBorderEnabled(false);

			int width, height;
			pPl->GetTextImage()->GetContentSize(width, height);
			width += 5;
			pPl->SetWide(width);
			totalWidth += width;
		}

		maxNameLength -= 1;

	} while (totalWidth > m_pSpectatorNames->GetWide());
}

//-----------------------------------------------------------------------------
// Purpose: adds the top header of the scoreboars
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::AddHeader()
{
	for (int i = 0; i < 2; i++)
	{
		m_iSectionId = 0; //make a blank one

		m_pPlayerList[i]->AddSection(m_iSectionId, "", StaticPlayerSortFunc);
		m_pPlayerList[i]->SetSectionAlwaysVisible(m_iSectionId);
		m_pPlayerList[i]->SetFontSection(m_iSectionId, m_pScheme->GetFont("IOSTeamMenuSmall"));
		m_pPlayerList[i]->SetLineSpacing(29);
		m_pPlayerList[i]->SetFgColor(g_ColorGray);
		m_pPlayerList[i]->SetSectionFgColor(0, g_ColorGray);

		int defaultFlags = SectionedListPanel::HEADER_CENTER | SectionedListPanel::COLUMN_CENTER;

		//m_pPlayerList[i]->SetSectionDividerColor(m_iSectionId, Color(255, 255, 255, 255));
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "posname",		"", defaultFlags, 50 );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "avatar",		"", defaultFlags | SectionedListPanel::COLUMN_IMAGE, 50 );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "name",			"", 0, 185);
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "cardindex",		"",	defaultFlags | SectionedListPanel::COLUMN_IMAGE, 15);

		// Max width = 330

		switch (m_nCurStat)
		{
		case DEFAULT_STATS:
			//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "countryindex",			"Loc.",			defaultFlags | SectionedListPanel::COLUMN_IMAGE, 50);
			//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "nationalityindex",		"Nat.",			defaultFlags | SectionedListPanel::COLUMN_IMAGE, 50);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "number",					"No.",			defaultFlags, 45);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "club",						"Club",			defaultFlags, 70);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "nationalteam",				"Nat.T.",		defaultFlags, 70);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goals",						"Goals",		defaultFlags, 45);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "assists",					"Assists",		defaultFlags, 45);
			//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "rating",					"Rating",		defaultFlags, 40);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "ping",						"Ping",			defaultFlags, 55);
			break;
		case GENERAL:
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "possession",				"Poss.",		defaultFlags, 50);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "distancecovered",			"Distance",		defaultFlags, 75);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "passes",					"Passes",		defaultFlags, 50);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "passescompleted",			"~ compl.",		defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "interceptions",				"Interc.",		defaultFlags, 50);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "turnovers",					"Turnovers",	defaultFlags, 50);
			break;
		case TACKLES:
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "slidingtackles",			"Tackles",		defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "slidingtacklescompleted",	"~ compl.",		defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "fouls",						"Fouls",		defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "foulssuffered",				"~ suffered",	defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "yellowcards",				"Yellows",		defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "redcards",					"Reds",			defaultFlags, 55);
			break;
		//case SET_PIECES:
		//	m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "freekicks",					"Free kicks",	defaultFlags, 52);
		//	m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "penalties",					"Penalties",	defaultFlags, 52);
		//	m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "throwins",					"Throw-ins",	defaultFlags, 52);
		//	m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "corners",					"Corners",		defaultFlags, 52);
		//	m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goalkicks",					"Goal kicks",	defaultFlags, 52);
		//	break;
		case ATTACK:
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goals",						"Goals",		defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "assists",					"Assists",		defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "shots",						"Shots",		defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "shotsongoal",				"~ on goal",	defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "offsides",					"Offsides",		defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "corners",					"Corners",		defaultFlags, 55);
			break;
		case KEEPER:
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "keepersaves",				"Saves",		defaultFlags, 80);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "keepersavescaught",				"~ caught",		defaultFlags, 80);
			//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goalkicks",					"Goal kicks",	defaultFlags, 65);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goalsconceded",				"Goals conc.",	defaultFlags, 80);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "owngoals",					"Own goals",	defaultFlags, 80);
			break;
		default:
			//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, g_szStatIdentifiers[m_nCurStat], g_szStatNames[m_nCurStat], 0, 90);
			break;
		}
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "possession",		"Poss.", 0, 55);
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "voice",		"Voice", SectionedListPanel::COLUMN_IMAGE, scheme()->GetProportionalScaledValue(VOICE_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "tracker", "#PlayerTracker", SectionedListPanel::COLUMN_IMAGE, scheme()->GetProportionalScaledValueEx( GetScheme(),FRIENDS_WIDTH) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CClientScoreBoardDialog::StaticPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2)
{
	KeyValues *it1 = list->GetItemData(itemID1);
	KeyValues *it2 = list->GetItemData(itemID2);
	Assert(it1 && it2);

	// first compare frags
	int v1 = it1->GetInt("posindex");
	int v2 = it2->GetInt("posindex");
	if (v1 > v2)
		return false;
	else if (v1 < v2)
		return true;

	//// next compare deaths
	//v1 = it1->GetInt("assists");
	//v2 = it2->GetInt("assists");
	//if (v1 > v2)
	//	return false;
	//else if (v1 < v2)
	//	return true;

	// the same, so compare itemID's (as a sentinel value to get deterministic sorts)
	return itemID1 < itemID2;
}

// prints the float into dst, returns the number
// of chars in the manner of snprintf. A truncated
// output due to size limit is not altered.
// A \0 is always appended. 
int prettify(float f, char *dst, int max) {
  int c = Q_snprintf(dst, max, "%.1f", f);

  if(c > max) {
    return c;
  }

  // position prior to '\0'
  c--;

  while(dst[c] == '0') {
    c--;
    if(dst[c] == '.') {
      c--;
      break;
    }
  }
  dst[c + 1] = '\0';  
  return c + 1;
}

#define GET_STAT_FTEXT_SHOWZERO(val, format) (VarArgs(format, val))
#define GET_STAT_FTEXT(val, format) (val == 0 ? "" : VarArgs(format, val))
#define GET_STAT_TEXT(val) (GET_STAT_FTEXT(val, "%d"))

//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
bool CClientScoreBoardDialog::GetPlayerInfo(int playerIndex, KeyValues *kv)
{
	IGameResources *gr = GameResources();

	if (!gr)
		return false;

	kv->SetString("goals", GET_STAT_TEXT(gr->GetGoals(playerIndex)));
	kv->SetString("owngoals", GET_STAT_TEXT(gr->GetOwnGoals(playerIndex)));
	kv->SetString("assists", GET_STAT_TEXT(gr->GetAssists(playerIndex)));
	kv->SetString("offsides", GET_STAT_TEXT(gr->GetOffsides(playerIndex)));
	kv->SetString("keepersaves", GET_STAT_TEXT(gr->GetKeeperSaves(playerIndex)));

	if (gr->GetKeeperSaves(playerIndex) > 0)
		kv->SetString("keepersavescaught", GET_STAT_FTEXT_SHOWZERO(gr->GetKeeperSavesCaught(playerIndex) * 100 / max(1, gr->GetKeeperSaves(playerIndex)), "%d%%"));
	else
		kv->SetString("keepersavescaught", "");

	kv->SetString("possession", GET_STAT_FTEXT_SHOWZERO(gr->GetPossession(playerIndex), "%d%%"));
	kv->SetString("turnovers", GET_STAT_TEXT(gr->GetTurnovers(playerIndex)));
	kv->SetString("distancecovered", GET_STAT_FTEXT_SHOWZERO(gr->GetDistanceCovered(playerIndex) / 10.0f, "%.1f km"));
	kv->SetString("redcards", GET_STAT_TEXT(gr->GetRedCards(playerIndex)));
	kv->SetString("yellowcards", GET_STAT_TEXT(gr->GetYellowCards(playerIndex)));
	kv->SetString("fouls", GET_STAT_TEXT(gr->GetFouls(playerIndex)));
	kv->SetString("foulssuffered", GET_STAT_TEXT(gr->GetFoulsSuffered(playerIndex)));
	kv->SetString("slidingtackles", GET_STAT_TEXT(gr->GetSlidingTackles(playerIndex)));

	if (gr->GetSlidingTackles(playerIndex) > 0)
		kv->SetString("slidingtacklescompleted", GET_STAT_FTEXT_SHOWZERO(gr->GetSlidingTacklesCompleted(playerIndex) * 100 / max(1, gr->GetSlidingTackles(playerIndex)), "%d%%"));
	else
		kv->SetString("slidingtacklescompleted", "");

	kv->SetString("passes", GET_STAT_TEXT(gr->GetPasses(playerIndex)));
	kv->SetString("freekicks", GET_STAT_TEXT(gr->GetFreeKicks(playerIndex)));
	kv->SetString("penalties", GET_STAT_TEXT(gr->GetPenalties(playerIndex)));
	kv->SetString("corners", GET_STAT_TEXT(gr->GetCorners(playerIndex)));
	kv->SetString("throwins", GET_STAT_TEXT(gr->GetThrowIns(playerIndex)));
	kv->SetString("goalkicks", GET_STAT_TEXT(gr->GetGoalKicks(playerIndex)));

	if (gr->GetPasses(playerIndex) > 0)
		kv->SetString("passescompleted", GET_STAT_FTEXT_SHOWZERO(gr->GetPassesCompleted(playerIndex) * 100 / max(1, gr->GetPasses(playerIndex)), "%d%%"));
	else
		kv->SetString("passescompleted", "");

	kv->SetString("interceptions", GET_STAT_TEXT(gr->GetInterceptions(playerIndex)));
	kv->SetString("goalsconceded", GET_STAT_TEXT(gr->GetGoalsConceded(playerIndex)));
	kv->SetString("shots", GET_STAT_TEXT(gr->GetShots(playerIndex)));

	if (gr->GetShots(playerIndex) > 0)
		kv->SetString("shotsongoal", GET_STAT_FTEXT_SHOWZERO(gr->GetShotsOnGoal(playerIndex) * 100 / gr->GetShots(playerIndex), "%d%%"));
	else
		kv->SetString("shotsongoal", "");

	kv->SetInt("number", gr->GetShirtNumber(playerIndex));

	kv->SetInt("posindex", gr->GetTeamPosIndex(playerIndex));
	kv->SetInt("playerindex", playerIndex);

	if (gr->GetTeam(playerIndex) == TEAM_SPECTATOR)
	{
		kv->SetString("posname", GetGlobalTeam(gr->GetSpecTeam(playerIndex))->GetCode());
	}
	else
	{
		char *posNameFormat;
		if (GetGlobalTeam(gr->GetTeam(playerIndex))->GetCaptainPosIndex() == gr->GetTeamPosIndex(playerIndex))
			posNameFormat = "(%s)";
		else
			posNameFormat = "%s";
		kv->SetString("posname", VarArgs(posNameFormat, g_szPosNames[(int)GetGlobalTeam(gr->GetTeam(playerIndex))->GetFormation()->positions[gr->GetTeamPosIndex(playerIndex)]->type]));
	}

	kv->SetString("name", gr->GetPlayerName( playerIndex ) );
	kv->SetString("steamname", gr->GetSteamName( playerIndex ) );
	kv->SetString("club", gr->GetClubName(playerIndex));
	kv->SetString("nationalteam", gr->GetNationalTeamName(playerIndex));

	char rating[5];
	if (gr->GetRatings(playerIndex) == 100)
		Q_snprintf(rating, 5, "%d", gr->GetRatings(playerIndex) / 10);
	else
	{
		Q_snprintf(rating, 5, "%d.%d", gr->GetRatings(playerIndex) / 10, gr->GetRatings(playerIndex) % 10);
	}

	kv->SetString("rating", rating);

	kv->SetInt("voice",  s_VoiceImage[GetClientVoiceMgr()->GetSpeakerStatus( playerIndex - 1) ]); 

	if (gr->GetPing( playerIndex ) < 1)
	{
		if ( gr->IsFakePlayer( playerIndex ) )
		{
			kv->SetString("ping", "bot");
		}
		else
		{
			kv->SetString("ping", "0");
		}
	}
	else
	{
		kv->SetInt("ping", gr->GetPing( playerIndex ));
	}

	// setup the tracker column
	//if (g_pFriendsUser)
	//{
	//	unsigned int trackerID = gEngfuncs.GetTrackerIDForPlayer(row);

	//	if (g_pFriendsUser->IsBuddy(trackerID) && trackerID != g_pFriendsUser->GetFriendsID())
	//	{
	//		kv->SetInt("tracker",TrackerImage);
	//	}
	//}

	kv->SetInt("countryindex", GetCountryFlagImageIndex(gr->GetCountryIndex(playerIndex)));
	kv->SetString("countryname", g_szCountryNames[gr->GetCountryIndex(playerIndex)]);

	kv->SetInt("nationalityindex", GetCountryFlagImageIndex(gr->GetNationalityIndex(playerIndex)));
	kv->SetString("nationalityname", g_szCountryNames[gr->GetNationalityIndex(playerIndex)]);

	int cardIndex;

	if (SDKGameRules()->GetMatchDisplayTimeSeconds(true, false) < gr->GetNextCardJoin(playerIndex))
		cardIndex = 2;
	else if (gr->GetYellowCards(playerIndex) % 2 == 1)
		cardIndex = 1;
	else
		cardIndex = 0;


	kv->SetInt("cardindex", GetCardImageIndex(cardIndex));

	return true;
}

#define GET_TSTAT_FTEXT(val, format) (VarArgs(format, val))
#define GET_TSTAT_TEXT(val) (GET_TSTAT_FTEXT(val, "%d"))

bool CClientScoreBoardDialog::GetTeamInfo(int team, KeyValues *kv)
{
	IGameResources *gr = GameResources();
	if (!gr)
		return false;

	C_Team *pTeam = GetGlobalTeam(team);
	int teamIndex = team - TEAM_A;
	bool teamClubInit = false;
	char teamClub[32] = {};
	bool isTeamSameClub = true;
	bool teamNationalTeamInit = false;
	char teamNationalTeam[32] = {};
	bool isTeamSameNationalTeam = true;
	int teamCountry = -1;
	bool isTeamSameCountry = true;
	int teamNationality = -1;
	bool isTeamSameNationality = true;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!gr->IsConnected(i) || gr->GetTeam(i) != team)
			continue;

		if (!teamClubInit)
		{
			Q_strncpy(teamClub, gr->GetClubName(i), sizeof(teamClub));
			teamClubInit = true;
		}

		if (Q_strcmp(gr->GetClubName(i), teamClub))
			isTeamSameClub = false;

		if (!teamNationalTeamInit)
		{
			Q_strncpy(teamNationalTeam, gr->GetNationalTeamName(i), sizeof(teamNationalTeam));
			teamNationalTeamInit = true;
		}

		if (Q_strcmp(gr->GetNationalTeamName(i), teamNationalTeam))
			isTeamSameNationalTeam = false;

		if (teamCountry == -1)
			teamCountry = gr->GetCountryIndex(i);

		if (gr->GetCountryIndex(i) != teamCountry)
			isTeamSameCountry = false;

		if (teamNationality == -1)
			teamNationality = gr->GetNationalityIndex(i);

		if (gr->GetNationalityIndex(i) != teamNationality)
			isTeamSameNationality = false;
	}

	if (!isTeamSameClub)
		Q_strncpy(teamClub, "", sizeof(teamClub));

	if (!isTeamSameNationalTeam)
		Q_strncpy(teamNationalTeam, "", sizeof(teamNationalTeam));

	if (!isTeamSameCountry || teamCountry == -1)
		teamCountry = 0;

	if (!isTeamSameNationality || teamNationality == -1)
		teamNationality = 0;

	kv->SetString("name", pTeam->GetShortName());
	kv->SetInt("playerindex", teamIndex - 2);
	kv->SetInt("posname", pTeam->GetNumPlayers());
	kv->SetInt("index", -1);
	kv->SetInt("countryindex", GetCountryFlagImageIndex(teamCountry));
	kv->SetInt("nationalityindex", GetCountryFlagImageIndex(teamNationality));
	kv->SetString("club", teamClub);
	kv->SetString("nationalteam", teamNationalTeam);
	kv->SetString("ping", GET_TSTAT_TEXT(pTeam->m_Ping));
	kv->SetString("possession", GET_TSTAT_FTEXT(pTeam->m_Possession, "%d%%"));
	kv->SetString("turnovers", GET_TSTAT_TEXT(pTeam->m_Turnovers));
	kv->SetString("passes", GET_TSTAT_TEXT(pTeam->m_Passes));
	kv->SetString("passescompleted", GET_TSTAT_FTEXT(pTeam->m_PassesCompleted * 100 / max(1, pTeam->m_Passes), "%d%%"));
	kv->SetString("distancecovered", GET_TSTAT_FTEXT(pTeam->m_DistanceCovered / 10.0f, "%.1f km"));
	kv->SetString("offsides", GET_TSTAT_TEXT(pTeam->m_Offsides));
	kv->SetString("corners", GET_TSTAT_TEXT(pTeam->m_Corners));
	kv->SetString("goalkicks", GET_TSTAT_TEXT(pTeam->m_GoalKicks));
	kv->SetString("shots", GET_TSTAT_TEXT(pTeam->m_Shots));
	kv->SetString("shotsongoal", GET_TSTAT_FTEXT(pTeam->m_ShotsOnGoal * 100 / max(1, pTeam->m_Shots), "%d%%"));
	kv->SetString("fouls", GET_TSTAT_TEXT(pTeam->m_Fouls));
	kv->SetString("foulssuffered", GET_TSTAT_TEXT(pTeam->m_FoulsSuffered));
	kv->SetString("slidingtackles", GET_TSTAT_TEXT(pTeam->m_SlidingTackles));
	kv->SetString("slidingtacklescompleted", GET_TSTAT_FTEXT(pTeam->m_SlidingTacklesCompleted * 100 / max(1, pTeam->m_SlidingTackles), "%d%%"));
	kv->SetString("freekicks", GET_TSTAT_TEXT(pTeam->m_FreeKicks));
	kv->SetString("goals", GET_TSTAT_TEXT(pTeam->m_Goals));
	kv->SetString("assists", GET_TSTAT_TEXT(pTeam->m_Assists));
	kv->SetString("interceptions", GET_TSTAT_TEXT(pTeam->m_Interceptions));
	kv->SetString("redcards", GET_TSTAT_TEXT(pTeam->m_RedCards));
	kv->SetString("yellowcards", GET_TSTAT_TEXT(pTeam->m_YellowCards));
	kv->SetString("penalties", GET_TSTAT_TEXT(pTeam->m_Penalties));
	kv->SetString("throwins", GET_TSTAT_TEXT(pTeam->m_ThrowIns));
	kv->SetString("keepersaves", GET_TSTAT_TEXT(pTeam->m_KeeperSaves));
	kv->SetString("keepersavescaught", GET_TSTAT_FTEXT(pTeam->m_KeeperSavesCaught * 100 / max(1, pTeam->m_KeeperSaves), "%d%%"));
	kv->SetString("owngoals", GET_TSTAT_TEXT(pTeam->m_OwnGoals));
	kv->SetString("goalsconceded", GET_TSTAT_TEXT(pTeam->m_GoalsConceded));

	char rating[5];
	if (pTeam->m_Rating == 100)
		Q_snprintf(rating, 5, "%d", pTeam->m_Rating / 10);
	else
	{
		Q_snprintf(rating, 5, "%d.%d", pTeam->m_Rating / 10, pTeam->m_Rating % 10);
	}

	kv->SetString("rating", rating);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdatePlayerAvatar( int playerIndex, KeyValues *kv )
{
	// Update their avatar
	if ( kv && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
	{
		player_info_t pi;
		if ( engine->GetPlayerInfo( playerIndex, &pi ) )
		{
			if ( pi.friendsID )
			{
				CSteamID steamIDForPlayer( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );

				// See if the avatar's changed
				int iAvatar = steamapicontext->SteamFriends()->GetFriendAvatar( steamIDForPlayer );
				if ( m_iImageAvatars[playerIndex] != iAvatar )
				{
					m_iImageAvatars[playerIndex] = iAvatar;

					// Now see if we already have that avatar in our list
					int iIndex = m_mapAvatarsToImageList.Find( iAvatar );
					if ( iIndex == m_mapAvatarsToImageList.InvalidIndex() )
					{
						CAvatarImage *pImage = new CAvatarImage();
						pImage->SetAvatarSteamID( steamIDForPlayer );
						pImage->SetAvatarSize( 25, 25 );	// Deliberately non scaling
						int iImageIndex = m_pImageList->AddImage( pImage );

						m_mapAvatarsToImageList.Insert( iAvatar, iImageIndex );
					}
				}

				int iIndex = m_mapAvatarsToImageList.Find( iAvatar );

				if ( iIndex != m_mapAvatarsToImageList.InvalidIndex() )
				{
					kv->SetInt( "avatar", m_mapAvatarsToImageList[iIndex] );

					CAvatarImage *pAvIm = (CAvatarImage *)m_pImageList->GetImage( m_mapAvatarsToImageList[iIndex] );
					pAvIm->UpdateFriendStatus();
				}
			}
		}
	}
}

int CClientScoreBoardDialog::GetCountryFlagImageIndex(int countryIndex)
{
	int imageIndex;

	if (countryIndex > 0)
	{
		int listIndex = m_mapCountryFlagsToImageList.Find(countryIndex);

		if (m_mapCountryFlagsToImageList.IsValidIndex(listIndex))
			imageIndex = m_mapCountryFlagsToImageList[listIndex];
		else
		{
			IImage *countryFlag = scheme()->GetImage(VarArgs("country_flags/%s", g_szCountryISOCodes[countryIndex]), false);
			countryFlag->SetSize(25, 25);
			countryFlag->SetColor(Color(255, 255, 255, 200));
			imageIndex = m_pImageList->AddImage(countryFlag);
			m_mapCountryFlagsToImageList.Insert(countryIndex, imageIndex);
		}
	}
	else
		imageIndex = 0;

	return imageIndex;
}

int CClientScoreBoardDialog::GetCardImageIndex(int cardIndex)
{
	int imageIndex;

	if (cardIndex > 0)
	{
		int listIndex = m_mapCardsToImageList.Find(cardIndex);

		if (m_mapCardsToImageList.IsValidIndex(listIndex))
			imageIndex = m_mapCardsToImageList[listIndex];
		else
		{
			IImage *card = scheme()->GetImage(VarArgs("cards/%s", cardIndex == 1 ? "yellow_card" : "red_card"), false);
			card->SetSize(10, 25);
			card->SetColor(Color(255, 255, 255, 200));
			imageIndex = m_pImageList->AddImage(card);
			m_mapCardsToImageList.Insert(cardIndex, imageIndex);
		}
	}
	else
		imageIndex = 0;

	return imageIndex;
}

//-----------------------------------------------------------------------------
// Purpose: searches for the player in the scoreboard
//-----------------------------------------------------------------------------
int CClientScoreBoardDialog::FindItemIDForPlayerIndex(int playerIndex, int &side)
{
	for (int j = 0; j < 2; j++)
	{
		for (int i = 0; i <= m_pPlayerList[j]->GetHighestItemID(); i++)
		{
			if (m_pPlayerList[j]->IsItemIDValid(i))
			{
				KeyValues *kv = m_pPlayerList[j]->GetItemData(i);
				kv = kv->FindKey(m_iPlayerIndexSymbol);
				if (kv && kv->GetInt() == playerIndex)
				{
					side = j;
					return i;
				}
			}
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::MoveLabelToFront(const char *textEntryName)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Center the dialog on the screen.  (vgui has this method on
//			Frame, but we're an EditablePanel, need to roll our own.)
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::MoveToCenterOfScreen()
{
	int wx, wy, ww, wt;
	surface()->GetWorkspaceBounds(wx, wy, ww, wt);
	SetPos((ww - GetWide()) / 2, (wt - GetTall()) / 2);
}

void CClientScoreBoardDialog::OnCommand( char const *cmd )
{
	if (!Q_strnicmp(cmd, "jointeam", 8))
	{
		engine->ClientCmd(cmd);
	}
	else if (!Q_strnicmp(cmd, "spectate", 8))
	{
		engine->ClientCmd(cmd);
	}
	else if (!Q_strnicmp(cmd, "stat:", 5))
	{
		m_nCurStat = atoi(&cmd[5]);
		m_eActivePanelType = FORMATION_MENU_NORMAL;
		Reset();
		Update();
	}
	else if (!Q_strnicmp(cmd, "specindex:", 10))
	{
		m_nSelectedPlayerIndex = atoi(&cmd[10]);

		if (m_nSelectedPlayerIndex == 0)
			m_eActivePanelType = FORMATION_MENU_NORMAL;
		else
			m_eActivePanelType = STATS_MENU;

		Update();
	}
	else if (!Q_stricmp(cmd, "togglecaptaincy"))
	{
		engine->ClientCmd(cmd);
	}
	else if (!Q_stricmp(cmd, "togglemenu"))
	{
		m_bShowCaptainMenu = !m_bShowCaptainMenu;
		ToggleMenu();
		Update();
	}
	else if (!Q_stricmp(cmd, "requesttimeout"))
	{
		engine->ClientCmd(cmd);
	}
	else if (!Q_stricmp(cmd, "endtimeout"))
	{
		engine->ClientCmd(cmd);
	}
	else if (!Q_stricmp(cmd, "showmatchevents"))
	{
		m_eActivePanelType = MATCHEVENT_MENU;
		Update();
	}
	else if (!Q_stricmp(cmd, "hidematchevents"))
	{		
		m_eActivePanelType = FORMATION_MENU_NORMAL;
		Update();
	}
	else if (!Q_strnicmp(cmd, "formation", 9))
	{		
		engine->ClientCmd(cmd);
	}
	else
		BaseClass::OnCommand(cmd);
}

void CClientScoreBoardDialog::ToggleMenu()
{
	m_pToggleCaptainMenu->SetText(m_bShowCaptainMenu ? "Stats Menu" : "Captain Menu");

	for (int i = 0; i < STAT_CATEGORY_COUNT; i++)
	{
		m_pStatButtons[i]->SetVisible(!m_bShowCaptainMenu);
	}

	m_pTimeoutHeader->SetVisible(m_bShowCaptainMenu);
	m_pTimeoutInfo->SetVisible(m_bShowCaptainMenu);
	m_pRequestTimeout->SetVisible(m_bShowCaptainMenu);
	m_pStatText->SetText(m_bShowCaptainMenu ? "Captain" : "Statistics");
}

void CClientScoreBoardDialog::OnItemSelected(KeyValues *data)
{
	if (m_eActivePanelType == FORMATION_MENU_HIGHLIGHT)
		return;

	int itemId = data->GetInt("itemID");

	if (itemId == -1)
		m_nSelectedPlayerIndex = 0;
	else
	{
		SectionedListPanel *pPanel = (SectionedListPanel *)data->GetPtr("panel");
		if (pPanel)
		{
			KeyValues *kv = pPanel->GetItemData(itemId);
			if (kv)
			{
				m_nSelectedPlayerIndex = kv->GetInt("playerindex", 0);
				//DevMsg("selectedplayer: %d\n", m_nSelectedPlayerIndex);
			}
		}
	}

	if (m_nSelectedPlayerIndex == 0)
		m_eActivePanelType = FORMATION_MENU_NORMAL;
	else
		m_eActivePanelType = STATS_MENU;

	Update();
}

void CClientScoreBoardDialog::OnItemLeftClick(KeyValues *data)
{
	if (m_bCanSetSetpieceTaker
		&& m_nSelectedPlayerIndex > 0
		&& GameResources()->GetTeam(m_nSelectedPlayerIndex) == GetLocalPlayerTeam())
	{
		int playerIndex = ((SectionedListPanel *)data->GetPtr("panel"))->GetItemData(data->GetInt("itemID"))->GetInt("playerindex", 0);
		//DevMsg("changing taker with index: %d\n", playerIndex);
		engine->ClientCmd(VarArgs("settaker %d", playerIndex));
	}
}

void CClientScoreBoardDialog::OnCursorEntered(Panel *panel)
{
	const char *cmd = ((Button *)panel)->GetCommand()->GetString("command");
	if (!Q_strnicmp(cmd, "stat:", 5))
	{
		OnCommand(cmd);
	}
	else if (!Q_strnicmp(cmd, "specindex:", 10))
	{
		OnCommand(cmd);
	}
	else if (!Q_strnicmp(cmd, "showmatchevents", 10))
	{
		OnCommand(cmd);
	}
}

void CClientScoreBoardDialog::OnCursorExited(Panel *panel)
{
	const char *cmd = ((Button *)panel)->GetCommand()->GetString("command");
	if (!Q_strnicmp(cmd, "stat:", 5))
	{
		OnCommand("stat:-1");
	}
	else if (!Q_strnicmp(cmd, "specindex:", 10))
	{
		OnCommand("specindex:0");
	}
	else if (!Q_strnicmp(cmd, "showmatchevents", 10))
	{
		OnCommand("hidematchevents");
	}
}

void CClientScoreBoardDialog::SetHighlightedPlayer(int playerIndex)
{
	int side = -1;
	int itemID = -1;

	if (playerIndex > 0)
		itemID = FindItemIDForPlayerIndex(playerIndex, side);

	if (itemID > -1)
	{
		m_eActivePanelType = FORMATION_MENU_HIGHLIGHT;
		m_pPlayerList[side]->SetSelectedItem(itemID);
	}
	else
	{
		m_pPlayerList[0]->ClearSelection();
		m_pPlayerList[1]->ClearSelection();
		m_eActivePanelType = FORMATION_MENU_NORMAL;
	}
}

void CClientScoreBoardDialog::Paint()
{
	BaseClass::Paint();
}