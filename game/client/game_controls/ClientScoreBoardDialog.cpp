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

#include <vgui/IInput.h>

#include "sdk_gamerules.h" //include before memdbgon.h		//IOS scoreboard

#include "vgui_avatarimage.h"
#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

bool AvatarIndexLessFunc( const int &lhs, const int &rhs )	
{ 
	return lhs < rhs; 
}

enum { PANEL_TOPMARGIN = 70, PANEL_MARGIN = 5, PANEL_WIDTH = (1024 - 2 * PANEL_MARGIN), PANEL_HEIGHT = (720 - PANEL_TOPMARGIN - PANEL_MARGIN) };
enum { TEAMCREST_SIZE = 48, TEAMCREST_VMARGIN = 7, TEAMCREST_HOFFSET = 240, TEAMCREST_VOFFSET = 10 };
enum { PLAYERLIST_HEIGHT = 330, PLAYERLIST_BOTTOMMARGIN = 10, PLAYERLISTDIVIDER_WIDTH = 8 };
enum { STATBUTTON_WIDTH = 120, STATBUTTON_HEIGHT = 30, STATBUTTON_HMARGIN = 5, STATBUTTON_VMARGIN = 30 };
enum { EXTRAINFO_HEIGHT = 275, EXTRAINFO_MARGIN = 5 };
enum { SPECLIST_HEIGHT = 30, SPECLIST_PADDING = 5, SPECNAME_WIDTH = 100, SPECTEXT_WIDTH = 100, SPECTEXT_MARGIN = 5, SPECBUTTON_WIDTH = 90, SPECBUTTON_HMARGIN = 5, SPECBUTTON_VMARGIN = 3 };

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClientScoreBoardDialog::CClientScoreBoardDialog(IViewPort *pViewPort) : EditablePanel( NULL, PANEL_SCOREBOARD )
{
	m_iPlayerIndexSymbol = KeyValuesSystem()->GetSymbolForString("playerindex");
	m_nCloseKey = BUTTON_CODE_INVALID;

	//memset(s_VoiceImage, 0x0, sizeof( s_VoiceImage ));
	TrackerImage = 0;
	m_pViewPort = pViewPort;

	// initialize dialog
	SetProportional(false);

	// set the scheme before any child control is created
	//SetScheme("ClientScheme");

	m_pMainPanel = new Panel(this);

	m_pExtraInfoPanel = new Panel(m_pMainPanel);

	m_pPlayerListDivider = new Panel(m_pMainPanel);

	m_pStatsMenu = new CStatsMenu(m_pExtraInfoPanel, "");
	m_pStatsMenu->SetVisible(false);
	m_pFormationMenu = new CFormationMenu(m_pExtraInfoPanel, "");

	for (int i = 0; i < 2; i++)
	{
		m_pPlayerList[i] = new SectionedListPanel(m_pMainPanel, "PlayerList");
		m_pPlayerList[i]->SetVerticalScrollbar(false);

		//LoadControlSettings("Resource/UI/ScoreBoard.res");
		m_pPlayerList[i]->SetVisible(false); // hide this until we load the images in applyschemesettings
		m_pPlayerList[i]->AddActionSignalTarget(this);
		m_pPlayerList[i]->EnableMouseEnterSelection(true);

		m_pTeamCrests[i] = new ImagePanel(this, "");
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

	m_pStatButtonOuterContainer = new Panel(m_pMainPanel);
	m_pStatButtonInnerContainer = new Panel(m_pStatButtonOuterContainer);

	m_pStatText = new Label(m_pStatButtonInnerContainer, "", "Statistics");

	for (int i = 0; i < STAT_CATEGORY_COUNT; i++)
	{
		m_pStatButtons[i] = new Button(m_pStatButtonInnerContainer, g_szStatCategoryNames[i], g_szStatCategoryNames[i], this, VarArgs("stat:%d", i));
	}

	m_pSpectatorContainer = new Panel(m_pMainPanel);

	m_pSpectatorNames = new Panel(m_pSpectatorContainer);

	m_pSpectatorText = new Label(m_pSpectatorContainer, "", "");

	m_pSpectateButton = new Button(m_pStatButtonInnerContainer, "SpectateButton", "Spectate", this, VarArgs("jointeam %d 0", TEAM_SPECTATOR));

	m_pSpecInfo = new Label(m_pMainPanel, "", "");

	m_pJoinRandom = new Button(m_pStatButtonInnerContainer, "JoinRandom", "Auto-Join", this, VarArgs("jointeam %d -1", TEAM_INVALID));

	m_pBecomeCaptain = new Button(m_pStatButtonInnerContainer, "BecomeCaptain", "Become Captain", this, "becomecaptain");
	m_pBecomeCaptain->SetVisible(false);

	m_pFormationList = new ComboBox(m_pStatButtonInnerContainer, "", 0, false);

	m_nCurStat = DEFAULT_STATS;
	m_nCurSpecIndex = 0;
	m_pCurSpecButton = NULL;

	m_nSelectedPlayerIndex = -2;

	m_bIsStatsMenuEnabled = true;

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

	m_iSectionId = 0;
	m_fNextUpdateTime = 0;
	// add all the sections
	AddHeader();
}

//-----------------------------------------------------------------------------
// Purpose: sets up screen
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::ApplySchemeSettings( IScheme *pScheme )
{
	m_pScheme = pScheme;

	BaseClass::ApplySchemeSettings( pScheme );

	SetBounds(0, 0, ScreenWidth(), ScreenHeight());

	m_pMainPanel->SetPaintBackgroundType(2);
	m_pMainPanel->SetBgColor(Color(0, 0, 0, 240));
	//m_pMainPanel->SetBounds(GetWide() / 2 - PANEL_WIDTH / 2, PANEL_TOPMARGIN, PANEL_WIDTH, PANEL_HEIGHT);
	m_pMainPanel->SetBounds(GetWide() / 2 - PANEL_WIDTH / 2, max(PANEL_TOPMARGIN, GetTall() / 2 - PANEL_HEIGHT / 2), PANEL_WIDTH, PANEL_HEIGHT);
	m_pMainPanel->SetPaintBorderEnabled(false);
	//m_pMainPanel->SetBorder(m_pScheme->GetBorder("BrightBorder"));

	if ( m_pImageList )
		delete m_pImageList;

	m_pImageList = new ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();
	memset( &m_iImageAvatars, 0, sizeof(int) * (MAX_PLAYERS+1) );

	m_pExtraInfoPanel->SetBounds(EXTRAINFO_MARGIN, PLAYERLIST_HEIGHT + PLAYERLIST_BOTTOMMARGIN + SPECLIST_HEIGHT, m_pMainPanel->GetWide() - 2 * EXTRAINFO_MARGIN, EXTRAINFO_HEIGHT);
	//m_pExtraInfoPanel->SetBgColor(Color(0, 0, 0, 200));

	m_pStatsMenu->SetBounds(0, 0, m_pStatsMenu->GetParent()->GetWide(), m_pStatsMenu->GetParent()->GetTall());
	m_pStatsMenu->PerformLayout();

	m_pFormationMenu->SetBounds(0, 0, m_pExtraInfoPanel->GetWide(), m_pExtraInfoPanel->GetTall());
	m_pFormationMenu->PerformLayout();

	m_pStatButtonOuterContainer->SetBounds(m_pMainPanel->GetWide() / 2 - (STATBUTTON_WIDTH + 2 * STATBUTTON_HMARGIN) / 2, m_pExtraInfoPanel->GetY(), STATBUTTON_WIDTH + 2 * STATBUTTON_HMARGIN, m_pMainPanel->GetTall() - m_pExtraInfoPanel->GetY());
	m_pStatButtonOuterContainer->SetZPos(1);
	//m_pStatButtonOuterContainer->SetBgColor(Color(0, 0, 0, 150));

	m_pStatButtonInnerContainer->SetBounds(0, m_pStatButtonOuterContainer->GetTall() / 2 - (4 * STATBUTTON_HEIGHT + STAT_CATEGORY_COUNT * STATBUTTON_HEIGHT) / 2, STATBUTTON_WIDTH + 2 * STATBUTTON_HMARGIN, 4 * STATBUTTON_HEIGHT + STAT_CATEGORY_COUNT * STATBUTTON_HEIGHT);

	m_pStatText->SetBounds(STATBUTTON_HMARGIN, 3 * STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
	m_pStatText->SetFont(m_pScheme->GetFont("StatButton"));
	m_pStatText->SetContentAlignment(Label::a_center);

	m_pSpectateButton->SetBounds(STATBUTTON_HMARGIN, 0, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
	m_pSpectateButton->SetFont(m_pScheme->GetFont("StatButton"));
	m_pSpectateButton->SetContentAlignment(Label::a_center);
	m_pSpectateButton->SetPaintBorderEnabled(false);
	m_pSpectateButton->SetCursor(dc_hand);

	m_pJoinRandom->SetBounds(STATBUTTON_HMARGIN, STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
	m_pJoinRandom->SetFont(m_pScheme->GetFont("StatButton"));
	m_pJoinRandom->SetContentAlignment(Label::a_center);
	m_pJoinRandom->SetPaintBorderEnabled(false);
	m_pJoinRandom->SetCursor(dc_hand);

	m_pBecomeCaptain->SetBounds(STATBUTTON_HMARGIN, 2 * STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
	m_pBecomeCaptain->SetFont(m_pScheme->GetFont("StatButton"));
	m_pBecomeCaptain->SetContentAlignment(Label::a_center);
	m_pBecomeCaptain->SetPaintBorderEnabled(false);
	m_pBecomeCaptain->SetCursor(dc_hand);

	m_pFormationList->SetBounds(STATBUTTON_HMARGIN, 4 * STATBUTTON_HEIGHT + STATBUTTON_HEIGHT, STATBUTTON_WIDTH, STATBUTTON_HEIGHT);
	m_pFormationList->SetFont(m_pScheme->GetFont("StatButton"));
	//m_pFormationList->SetPaintBorderEnabled(false);
	m_pFormationList->SetVisible(false);

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

	m_pSpectatorNames->SetBounds(SPECLIST_PADDING + SPECTEXT_WIDTH + SPECTEXT_MARGIN, 0, m_pSpectatorContainer->GetWide() - (SPECLIST_PADDING + SPECTEXT_WIDTH), SPECLIST_HEIGHT);
	
	m_pSpectatorText->SetBounds(SPECLIST_PADDING, 0, SPECTEXT_WIDTH, SPECLIST_HEIGHT);
	m_pSpectatorText->SetFont(m_pScheme->GetFont("SpectatorListNormal"));

	m_pSpecInfo->SetBounds(0, m_pSpectatorContainer->GetY() - SPECLIST_HEIGHT, m_pMainPanel->GetWide(), SPECLIST_HEIGHT);
	m_pSpecInfo->SetZPos(10);
	m_pSpecInfo->SetBgColor(Color(0, 0, 0, 240));
	m_pSpecInfo->SetFont(m_pScheme->GetFont("SpectatorListNormal"));
	m_pSpecInfo->SetContentAlignment(Label::a_center);
	m_pSpecInfo->SetVisible(false);

	m_pPlayerListDivider->SetBounds(m_pMainPanel->GetWide() / 2 - PLAYERLISTDIVIDER_WIDTH / 2, 0, PLAYERLISTDIVIDER_WIDTH, PLAYERLIST_HEIGHT + PLAYERLIST_BOTTOMMARGIN);
	m_pPlayerListDivider->SetBgColor(Color(0, 0, 0, 150));
	m_pPlayerListDivider->SetVisible(false);

	for (int i = 0; i < 2; i++)
	{
		//m_pTeamCrests[i]->SetBounds(GetWide() / 2 - TEAMCREST_SIZE / 2 + (i == 0 ? -1 : 1) * TEAMCREST_HOFFSET, TEAMCREST_VOFFSET, TEAMCREST_SIZE, TEAMCREST_SIZE);
		//m_pTeamCrests[i]->SetShouldScaleImage(true);
		//m_pTeamCrests[i]->SetImage(i == 0 ? "hometeamcrest" : "awayteamcrest");

		m_pPlayerList[i]->SetBounds(i * (m_pMainPanel->GetWide() / 2), 0, m_pMainPanel->GetWide() / 2, PLAYERLIST_HEIGHT);
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
		input()->SetCursorPos(ScreenWidth() / 2, m_pMainPanel->GetY() + m_pExtraInfoPanel->GetY() + 15);
	}
	else
	{
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

	//if (m_pPlayerList[0]->GetFgColor() != GetGlobalTeam(TEAM_A)->Get_HudKitColor() || m_pPlayerList[1]->GetFgColor() != GetGlobalTeam(TEAM_B)->Get_HudKitColor())
	//{
	//	Reset();
	//}

	bool showFormationMenu = true;

	if (m_bIsStatsMenuEnabled)
	{
		for (int i = 0; i < 2; i++)
		{
			int itemID = m_pPlayerList[i]->GetSelectedItem();

			if (m_pPlayerList[i]->IsItemIDValid(itemID))
			{
				m_nSelectedPlayerIndex = m_pPlayerList[i]->GetItemData(itemID)->GetInt("playerindex");
				showFormationMenu = false;
				break;
			}
			else
				m_nSelectedPlayerIndex = -2;
		}
	}

	if (showFormationMenu)
	{
		m_pFormationMenu->Update();
		m_pFormationMenu->SetVisible(true);
		m_pStatsMenu->SetVisible(false);
		m_pStatButtonOuterContainer->SetVisible(true);
	}
	else
	{
		m_pFormationMenu->SetVisible(false);
		m_pStatsMenu->SetVisible(true);
		m_pStatButtonOuterContainer->SetVisible(false);
	}

	UpdatePlayerInfo();
	UpdateTeamInfo();

	for (int i = 0; i < 2; i++)
	{
		m_pTeamCrests[i]->SetVisible(gr->HasTeamCrest(TEAM_A + i));
		//m_pPlayerList[i]->SetFgColor(GetGlobalTeam(TEAM_A + i)->Get_HudKitColor());
		//m_pPlayerList[i]->SetSectionFgColor(0, GetGlobalTeam(TEAM_A + i)->Get_HudKitColor());
		//m_pPlayerList[i]->Repaint();
	}

	if (m_pFormationList->IsVisible())
	{
		m_pFormationList->RemoveAll();
		KeyValues *kv = new KeyValues("UserData", "index", 0);
		m_pFormationList->AddItem("4-3-3", kv);
		kv->deleteThis();
		kv = new KeyValues("UserData", "index", 1);
		m_pFormationList->AddItem("3-5-2", kv);
		kv->deleteThis();
		kv = new KeyValues("UserData", "index", 2);
		m_pFormationList->AddItem("5-3-2", kv);
		kv->deleteThis();
	}

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

		if (m_nSelectedPlayerIndex == i - 1)
			m_pStatsMenu->Update(m_nSelectedPlayerIndex, kv);

		kv->deleteThis();

		m_pPlayerList[i]->SetItemFgColor(0, GameResources()->GetTeamColor(i + TEAM_A));

		for (int j = 0; j <= m_pPlayerList[i]->GetHighestItemID(); j++)
		{
			if (m_pPlayerList[i]->IsItemIDValid(j))
			{
				m_pPlayerList[i]->SetItemDividerColor(j, Color(0, 0, 0, 0));
			}
		}

		m_pPlayerList[i]->SetItemDividerColor(0, GameResources()->GetTeamColor(i + TEAM_A));
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

	char spectatorNames[1024] = "";
	int spectatorCount = 0;

	bool isPosTaken[2][11] = {};

	Color black = Color(0, 0, 0, 255);
	Color darker = Color(75, 75, 75, 255);
	Color dark = Color(125, 125, 125, 255);
	Color light = Color(175, 175, 175, 255);
	Color lighter = Color(225, 225, 225, 255);
	Color white = Color(255, 255, 255, 255);

	//m_SpecNames.RemoveAll();

	//while (m_pSpectatorContainer->GetChildCount() > 0)
	//{
	//	m_pSpectatorContainer->GetChild(0)->DeletePanel();
	//}

	CUtlVector<SpecInfo> specList;

	// walk all the players and make sure they're in the scoreboard
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (gr->IsConnected( i ))
		{
			if (gr->GetTeam(i) != TEAM_A && gr->GetTeam(i) != TEAM_B)
			{
				//char playerName[MAX_PLAYER_NAME_LENGTH + 16];
				//Q_strncpy(playerName, gr->GetPlayerName(i), sizeof(playerName));
				//int nextJoin = max(0, (int)(gr->GetNextJoin(i) - gpGlobals->curtime));
				//if (nextJoin > 0)
				//	Q_strncat(playerName, VarArgs(" [%d]", nextJoin), sizeof(playerName));
				//Q_strncat(spectatorNames, VarArgs("%s %s", (spectatorCount == 0 ? "" : ","), playerName), sizeof(spectatorNames));
				//spectatorCount += 1;

				//new Button(m_pSpectatorContainer, "", playerName, this, VarArgs("specindex:%d", i));

				SpecInfo info;
				info.playerIndex = i;
				Q_strncpy(info.playerName, gr->GetPlayerName(i), sizeof(info.playerName));

				specList.AddToTail(info);

				// remove the player
				int side = -1;
				int itemID = FindItemIDForPlayerIndex(i, side);
				if (itemID != -1)
				{
					m_pPlayerList[side]->RemoveItem(itemID);
				}

				if (gr->GetTeamToJoin(i) == TEAM_INVALID)
					continue;
			}
			else
			{
				// add the player to the list
				KeyValues *playerData = new KeyValues("data");
				GetPlayerInfo( i, playerData );

				UpdatePlayerAvatar( i, playerData );

				int side = -1;
				int itemID = FindItemIDForPlayerIndex(i, side);
				int team = gr->GetTeam(i); //omega; set a variable to team so we can reuse it
				int teamIndex = team - TEAM_A;
				int sectionID = 0;//iTeamSections[playerTeam]; //omega; make sure it goes into the proper section

				if (itemID != -1)
				{
					if (side == teamIndex)
						m_pPlayerList[side]->ModifyItem( itemID, sectionID, playerData );
					else
					{
						m_pPlayerList[side]->RemoveItem(itemID);
						itemID = -1;
					}
				}

				if (itemID == -1)
				{
					itemID = m_pPlayerList[teamIndex]->AddItem( sectionID, playerData );
				}

				// set the row color based on the players team
				m_pPlayerList[teamIndex]->SetItemFgColor( itemID, gr->GetTeamColor( team ) );
				m_pPlayerList[teamIndex]->SetItemFont( itemID, m_pScheme->GetFont("IOSTeamMenuNormal"));

				if (i == m_nSelectedPlayerIndex)
					m_pStatsMenu->Update(m_nSelectedPlayerIndex, playerData);

				playerData->deleteThis();
			}

			int team = gr->GetTeamToJoin(i) != TEAM_INVALID ? gr->GetTeamToJoin(i) : gr->GetTeam(i);
			int teamIndex = team - TEAM_A;

			isPosTaken[teamIndex][gr->GetTeamPosIndex(i)] = true;
		}
		else
		{
			// remove the player
			int side = -1;
			int itemID = FindItemIDForPlayerIndex(i, side);
			if (itemID != -1)
			{
				m_pPlayerList[side]->RemoveItem(itemID);
			}
		}
	}

	char spectatorText[1024];

	if (specList.Count() == 0)
		Q_strncpy(spectatorText, "No spectators", sizeof(spectatorText));
	else
		Q_snprintf(spectatorText, sizeof(spectatorText), "%d %s:", specList.Count(), (specList.Count() == 1 ? "spectator" : "spectators"));

	m_pSpectatorText->SetText(spectatorText);

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

	int totalWidth = 0;

	for (int i = 0; i < specList.Count(); i++)
	{
		Button *pPl = (Button *)m_pSpectatorNames->GetChild(i);

		char text[32];
		Q_snprintf(text, sizeof(text), "%.7s", specList[i].playerName);

		int nextJoin = max(0, (int)(gr->GetNextJoin(specList[i].playerIndex) - gpGlobals->curtime));

		if (nextJoin > 0)
			Q_strncat(text, VarArgs(" [%d]", nextJoin), sizeof(text));

		if (i < specList.Count() - 1)
			Q_strncat(text, ", ", sizeof(text));

		pPl->SetText(text);
		pPl->SetCommand(VarArgs("specindex:%d", specList[i]));
		pPl->AddActionSignalTarget(this);
		pPl->SetBounds(totalWidth, 0, SPECNAME_WIDTH, SPECLIST_HEIGHT);
		pPl->SetFgColor(Color(255, 255, 255, 255));
		pPl->SetFont(m_pSpectatorFontList[1]);
		pPl->SetContentAlignment(Label::a_center);
		pPl->SetPaintBackgroundEnabled(false);
		pPl->SetPaintBorderEnabled(false);

		int width, height;
		pPl->GetTextImage()->GetContentSize(width, height);
		width += 5;
		pPl->SetWide(width);
		totalWidth += width;
	}

	if (m_nCurSpecIndex > 0 && m_pCurSpecButton && gr->IsConnected(m_nCurSpecIndex) && gr->GetTeam(m_nCurSpecIndex) == TEAM_SPECTATOR)
	{
		m_pSpecInfo->SetText(VarArgs("%s | %s | %s | %s | %d ms", gr->GetPlayerName(m_nCurSpecIndex), gr->GetSteamName(m_nCurSpecIndex), g_szCountryNames[gr->GetCountryName(m_nCurSpecIndex)], gr->GetClubName(m_nCurSpecIndex), gr->GetPing(m_nCurSpecIndex)));
		int width, height;
		m_pSpecInfo->GetContentSize(width, height);
		width += 10;
		int xPos = m_pSpectatorNames->GetX() + m_pCurSpecButton->GetX() + m_pCurSpecButton->GetWide() / 2 - width / 2;
		xPos = clamp(xPos, 0, m_pMainPanel->GetWide() - width);
		int yPos = m_pSpectatorContainer->GetY() + m_pSpectatorContainer->GetTall();
		m_pSpecInfo->SetBounds(xPos, yPos, width, SPECLIST_HEIGHT);
		m_pSpecInfo->SetVisible(true);
	}
	else
		m_pSpecInfo->SetVisible(false);
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
		m_pPlayerList[i]->SetLineSpacing(25);
		m_pPlayerList[i]->SetFgColor(Color(255, 255, 255, 255));
		m_pPlayerList[i]->SetSectionFgColor(0, Color(255, 255, 255, 255));

		int defaultFlags = SectionedListPanel::HEADER_CENTER | SectionedListPanel::COLUMN_CENTER;

		//m_pPlayerList[i]->SetSectionDividerColor(m_iSectionId, Color(255, 255, 255, 255));
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "posname",		"Pos.", defaultFlags, 55 );
		//if ( ShowAvatars() )
		//{
		//	m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "avatar",		"", SectionedListPanel::COLUMN_IMAGE, 50 );
		//}
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "name",			"Name", 0, 180);

		// Max width = 270

		switch (m_nCurStat)
		{
		case DEFAULT_STATS:
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "country",			"Nat.", defaultFlags | SectionedListPanel::COLUMN_IMAGE, 45);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "club",				"Club", defaultFlags, 60);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goals",				"Goals", defaultFlags, 35);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "assists",			"Assists", defaultFlags, 35);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "rating",			"Rating", defaultFlags, 35);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "ping",				"Ping", defaultFlags, 50);
			break;
		case GENERAL:
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "possession",		"Poss.", defaultFlags, 45);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "distancecovered",	"Distance", defaultFlags, 70);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "passes",			"Passes", defaultFlags, 45);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "passescompleted",	"~ compl.", defaultFlags, 55);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "interceptions",		"Interc.", defaultFlags, 45);
			break;
		case TACKLINGS:
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "fouls",				"Fouls", defaultFlags, 65);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "foulssuffered",		"~ suffered", defaultFlags, 65);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "redcards",			"Reds", defaultFlags, 65);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "yellowcards",		"Yellows", defaultFlags, 65);
			break;
		case SET_PIECES:
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "freekicks",			"Free kicks", defaultFlags, 52);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "penalties",			"Penalties", defaultFlags, 52);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "throwins",			"Throw-ins", defaultFlags, 52);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "corners",			"Corners", defaultFlags, 52);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goalkicks",			"Goal kicks", defaultFlags, 52);
			break;
		case KEEPER:
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "saves",				"Saves", defaultFlags, 65);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goalkicks",			"Goal kicks", defaultFlags, 65);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "owngoals",			"Own goals", defaultFlags, 65);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goalsconceded",		"Goals conc.", defaultFlags, 65);
			break;
		case OFFENSE:
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goals",				"Goals", defaultFlags, 52);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "assists",			"Assists", defaultFlags, 52);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "shots",				"Shots", defaultFlags, 52);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "shotsongoal",		"~ on goal", defaultFlags, 52);
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "offsides",			"Offsides", defaultFlags, 52);
			break;
		default:
			//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, g_szStatIdentifiers[m_nCurStat], g_szStatNames[m_nCurStat], 0, 90);
			break;
		}
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "possession",		"Poss.", 0, 55);
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "voice",		"Voice", SectionedListPanel::COLUMN_IMAGE, scheme()->GetProportionalScaledValue(VOICE_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "tracker", "#PlayerTracker", SectionedListPanel::COLUMN_IMAGE, scheme()->GetProportionalScaledValueEx( GetScheme(),FRIENDS_WIDTH) );
		KeyValues *kv = new KeyValues("data");
		kv->SetInt("playerindex", i - 1);
		m_pPlayerList[i]->AddItem(0, kv);
		kv->deleteThis();
		m_pPlayerList[i]->SetItemFont(0, m_pScheme->GetFont("IOSTeamMenuNormalBold"));
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

#define GET_STAT_FTEXT(val, format) (VarArgs(format, val))
#define GET_STAT_TEXT(val) (GET_STAT_FTEXT(val, "%d"))

//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
bool CClientScoreBoardDialog::GetPlayerInfo(int playerIndex, KeyValues *kv)
{
	IGameResources *gr = GameResources();

	if (!gr )
		return false;

	kv->SetString("goals", GET_STAT_TEXT(gr->GetGoals(playerIndex)));
	kv->SetString("owngoals", GET_STAT_TEXT(gr->GetOwnGoals(playerIndex)));
	kv->SetString("assists", GET_STAT_TEXT(gr->GetAssists(playerIndex)));
	kv->SetString("offsides", GET_STAT_TEXT(gr->GetOffsides(playerIndex)));
	kv->SetString("keepersaves", GET_STAT_TEXT(gr->GetKeeperSaves(playerIndex)));
	kv->SetString("possession", GET_STAT_FTEXT(gr->GetPossession(playerIndex), "%d%%"));
	kv->SetString("distancecovered", GET_STAT_FTEXT(gr->GetDistanceCovered(playerIndex) / 1000.0f, "%.1f km"));
	kv->SetString("redcards", GET_STAT_TEXT(gr->GetRedCards(playerIndex)));
	kv->SetString("yellowcards", GET_STAT_TEXT(gr->GetYellowCards(playerIndex)));
	kv->SetString("fouls", GET_STAT_TEXT(gr->GetFouls(playerIndex)));
	kv->SetString("foulssuffered", GET_STAT_TEXT(gr->GetFoulsSuffered(playerIndex)));
	kv->SetString("passes", GET_STAT_TEXT(gr->GetPasses(playerIndex)));
	kv->SetString("freekicks", GET_STAT_TEXT(gr->GetFreeKicks(playerIndex)));
	kv->SetString("penalties", GET_STAT_TEXT(gr->GetPenalties(playerIndex)));
	kv->SetString("corners", GET_STAT_TEXT(gr->GetCorners(playerIndex)));
	kv->SetString("throwins", GET_STAT_TEXT(gr->GetThrowIns(playerIndex)));
	kv->SetString("saves", GET_STAT_TEXT(gr->GetKeeperSaves(playerIndex)));
	kv->SetString("goalkicks", GET_STAT_TEXT(gr->GetGoalKicks(playerIndex)));
	kv->SetString("passescompleted", GET_STAT_FTEXT(gr->GetPassesCompleted(playerIndex) * 100 / max(1, gr->GetPasses(playerIndex)), "%d%%"));
	kv->SetString("interceptions", GET_STAT_TEXT(gr->GetInterceptions(playerIndex)));
	kv->SetString("goalsconceded", GET_STAT_TEXT(gr->GetGoalsConceded(playerIndex)));
	kv->SetString("shots", GET_STAT_TEXT(gr->GetShots(playerIndex)));
	kv->SetString("shotsongoal", GET_STAT_FTEXT(gr->GetShotsOnGoal(playerIndex) * 100 / max(1, gr->GetShots(playerIndex)), "%d%%"));
	kv->SetInt("posindex", gr->GetTeamPosIndex(playerIndex));
	kv->SetInt("playerindex", playerIndex);
	//kv->SetString("country", gr->GetCountryName(playerIndex));
	kv->SetString("posname", g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][gr->GetTeamPosIndex(playerIndex)][POS_TYPE]]);
	kv->SetString("name", gr->GetPlayerName( playerIndex ) );
	kv->SetString("steamname", gr->GetSteamName( playerIndex ) );
	kv->SetString("club", gr->GetClubName(playerIndex));
	char rating[4];
	prettify(gr->GetPassesCompleted(playerIndex) * 10.0f / max(1, gr->GetPasses(playerIndex)), rating, 4);
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

	int imageIndex;

	if (gr->GetCountryName(playerIndex) > 0)
	{
		IImage *countryFlag = scheme()->GetImage(VarArgs("country_flags/%s", g_szCountryISOCodes[gr->GetCountryName(playerIndex)]), false);
		countryFlag->SetSize(25, 25);
		imageIndex = m_pImageList->AddImage(countryFlag);
		//m_pImageList->GetImage(imageIndex)->SetPos(10, 10);
	}
	else
		imageIndex = 0;

	kv->SetInt("country", imageIndex);

	const char *oldName = kv->GetString("name","");
	char newName[MAX_PLAYER_NAME_LENGTH];

	UTIL_MakeSafeName( oldName, newName, MAX_PLAYER_NAME_LENGTH );

	//IOS scoreboard
	kv->SetString("name", newName);

	return true;
}

bool CClientScoreBoardDialog::GetTeamInfo(int team, KeyValues *kv)
{
	IGameResources *gr = GameResources();
	if (!gr)
		return false;

	C_Team *pTeam = GetGlobalTeam(team);
	int teamIndex = team - TEAM_A;
	int pingSum = 0;
	int pingPlayers = 0;
	bool teamClubInit = false;
	char teamClub[32] = {};
	bool isTeamSameClub = true;
	int teamCountry = -1;
	bool isTeamSameCountry = true;
	float distSum = 0;
	int passSum = 0;
	int passCompletedSum = 0;
	int passCompletedPlayerCount = 0;
	int offsides = 0;
	int corners = 0;
	int goalkicks = 0;
	int shots = 0;
	int shotsOnGoal = 0;
	int shotsOnGoalPlayerCount = 0;
	int fouls = 0;
	int foulsSuffered = 0;
	int freekicks = 0;
	int goals = 0;
	int assists = 0;
	int interceptions = 0;
	int redcards = 0;
	int yellowcards = 0;
	int penalties = 0;
	int throwins = 0;
	int saves = 0;
	int owngoals = 0;
	int goalsconceded = 0;
	float ratings = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!gr->IsConnected(i) || gr->GetTeam(i) != team)
			continue;

		if (gr->GetPing(i) > 0)
		{
			pingSum += gr->GetPing(i);
			pingPlayers += 1;
		}

		if (!teamClubInit)
		{
			Q_strncpy(teamClub, gr->GetClubName(i), sizeof(teamClub));
			teamClubInit = true;
		}

		if (Q_strcmp(gr->GetClubName(i), teamClub))
			isTeamSameClub = false;

		if (teamCountry == -1)
			teamCountry = gr->GetCountryName(i);

		if (gr->GetCountryName(i) != teamCountry)
			isTeamSameCountry = false;

		distSum += gr->GetDistanceCovered(i) / 1000.0f;

		if (gr->GetPasses(i) > 0)
		{
			passSum += gr->GetPasses(i);
			passCompletedSum += gr->GetPassesCompleted(i) * 100 / max(1, gr->GetPasses(i));
			passCompletedPlayerCount += 1;
			ratings += gr->GetPassesCompleted(i) * 10 / max(1, gr->GetPasses(i));
		}

		if (gr->GetShots(i) > 0)
		{
			shots += gr->GetShots(i);
			shotsOnGoal += gr->GetShotsOnGoal(i) * 100 / max(1, gr->GetShots(i));
			shotsOnGoalPlayerCount += 1;
		}

		offsides += gr->GetOffsides(i);
		corners += gr->GetCorners(i);
		goalkicks += gr->GetGoalKicks(i);
		fouls += gr->GetFouls(i);
		foulsSuffered += gr->GetFoulsSuffered(i);
		freekicks += gr->GetFreeKicks(i);
		goals += gr->GetGoals(i);
		assists += gr->GetAssists(i);
		interceptions += gr->GetInterceptions(i);
		redcards += gr->GetRedCards(i);
		yellowcards += gr->GetYellowCards(i);
		penalties += gr->GetPenalties(i);
		throwins += gr->GetThrowIns(i);
		saves += gr->GetKeeperSaves(i);
		owngoals += gr->GetOwnGoals(i);
		goalsconceded += gr->GetGoalsConceded(i);
	}

	if (!isTeamSameClub)
		Q_strncpy(teamClub, "", sizeof(teamClub));

	if (!isTeamSameCountry || teamCountry == -1)
		teamCountry = 0;

	kv->SetInt("playerindex", teamIndex - 1);
	kv->SetInt("posname", pTeam->GetNumPlayers());
	kv->SetString("name", pTeam->Get_ShortTeamName());
	int imageIndex;
	if (teamCountry > 0)
	{
		IImage *countryFlag = scheme()->GetImage(VarArgs("country_flags/%s", g_szCountryISOCodes[teamCountry]), false);
		countryFlag->SetSize(26, 26);
		imageIndex = m_pImageList->AddImage(countryFlag);
		m_pImageList->GetImage(imageIndex)->SetPos(10, 10);
	}
	else
		imageIndex = 0;
	kv->SetInt("country", imageIndex);
	kv->SetString("club", teamClub);
	kv->SetString("ping", GET_STAT_TEXT(pingSum / max(1, pingPlayers)));
	kv->SetString("possession", GET_STAT_FTEXT(pTeam->Get_Possession(), "%d%%"));
	kv->SetString("passes", GET_STAT_TEXT(passSum / max(1, pTeam->GetNumPlayers())));
	kv->SetString("passescompleted", GET_STAT_FTEXT(passCompletedSum / max(1, passCompletedPlayerCount), "%d%%"));
	kv->SetString("distancecovered", GET_STAT_FTEXT(distSum / max(1, pTeam->GetNumPlayers()), "%.1f km"));
	kv->SetString("offsides", GET_STAT_TEXT(offsides));
	kv->SetString("corners", GET_STAT_TEXT(corners));
	kv->SetString("goalkicks", GET_STAT_TEXT(goalkicks));
	kv->SetString("shots", GET_STAT_TEXT(shots));
	kv->SetString("shotsongoal", GET_STAT_FTEXT(shotsOnGoal / max(1, shotsOnGoalPlayerCount), "%d%%"));
	kv->SetString("fouls", GET_STAT_TEXT(fouls));
	kv->SetString("foulssuffered", GET_STAT_TEXT(fouls));
	kv->SetString("freekicks", GET_STAT_TEXT(freekicks));
	kv->SetString("goals", GET_STAT_TEXT(goals));
	kv->SetString("assists", GET_STAT_TEXT(assists));
	kv->SetString("interceptions", GET_STAT_TEXT(interceptions / max(1, pTeam->GetNumPlayers())));
	kv->SetString("redcards", GET_STAT_TEXT(redcards));
	kv->SetString("yellowcards", GET_STAT_TEXT(yellowcards));
	kv->SetString("penalties", GET_STAT_TEXT(penalties));
	kv->SetString("throwins", GET_STAT_TEXT(throwins));
	kv->SetString("saves", GET_STAT_TEXT(saves));
	kv->SetString("owngoals", GET_STAT_TEXT(owngoals));
	kv->SetString("goalsconceded", GET_STAT_TEXT(goalsconceded));
	char rating[4];
	prettify(ratings / max(1, passCompletedPlayerCount), rating, 4);
	kv->SetString("rating", rating);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdatePlayerAvatar( int playerIndex, KeyValues *kv )
{
	// Update their avatar
	if ( kv && ShowAvatars() && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
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
						pImage->SetAvatarSize( 32, 32 );	// Deliberately non scaling
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
	else if (!Q_strnicmp(cmd, "stat:", 5))
	{
		m_nCurStat = atoi(&cmd[5]);
		Reset();
		Update();
	}
	else if (!Q_strnicmp(cmd, "specindex:", 10))
	{
		m_nCurSpecIndex = atoi(&cmd[10]);

		if (m_nCurSpecIndex > 0)
		{
			for (int i = 0; i < m_pSpectatorNames->GetChildCount(); i++)
			{
				Button *pPl = (Button *)m_pSpectatorNames->GetChild(i);
				if (!Q_strcmp(pPl->GetCommand()->GetString("command"), cmd))
				{
					m_pCurSpecButton = pPl;
					break;
				}
			}
		}
		else
			m_pCurSpecButton = NULL;

		Update();
	}
	else if (!Q_stricmp(cmd, "becomecaptain"))
	{
		for (int i = 0; i < STAT_CATEGORY_COUNT; i++)
		{
			m_pStatButtons[i]->SetVisible(false);
		}

		m_pFormationList->SetVisible(true);

		engine->ClientCmd(cmd);
	}
	else
		BaseClass::OnCommand(cmd);
}

void CClientScoreBoardDialog::OnItemSelected(KeyValues *data)
{
	//if (m_pTabPanels[TAB_SETTINGS]->IsVisible())
	//	return;

	//SectionedListPanel *pCurPanel = (SectionedListPanel *)data->GetPtr("panel");
	//SectionedListPanel *pOtherPanel = (pCurPanel == m_pPlayerList[0] ? m_pPlayerList[1] : m_pPlayerList[0]);
	//int itemID = data->GetInt("itemID");
	//int oldItemID = data->GetInt("oldItemID");
	//bool select = true;

	//if (itemID == -1)
	//{
	//	itemID = oldItemID;
	//	select = false;
	//}

	//if (itemID != -1)
	//{
	//	int posindex = pCurPanel->GetItemData(itemID)->GetInt("posindex");

	//	for (int i = 0; i <= pOtherPanel->GetHighestItemID(); i++)
	//	{
	//		if (pOtherPanel->IsItemIDValid(i) && pOtherPanel->GetItemData(i)->GetInt("posindex") == posindex)
	//		{
	//			if (select)
	//				pOtherPanel->SetSelectedItem(i);
	//			else
	//			{
	//				if (pOtherPanel->GetSelectedItem() == i)
	//					pOtherPanel->ClearSelection();
	//			}

	//			break;
	//		}
	//	}
	//}

	Update();
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
}

void CClientScoreBoardDialog::SetHighlightedPlayer(int playerIndex)
{
	int side = -1;
	int itemID = -1;

	if (playerIndex > 0)
		itemID = FindItemIDForPlayerIndex(playerIndex, side);

	if (itemID > -1)
	{
		m_bIsStatsMenuEnabled = false;
		m_pPlayerList[side]->SetSelectedItem(itemID);
	}
	else
	{
		m_pPlayerList[0]->ClearSelection();
		m_pPlayerList[1]->ClearSelection();
		m_bIsStatsMenuEnabled = true;
	}
}

void CClientScoreBoardDialog::Paint()
{
	BaseClass::Paint();

	
}