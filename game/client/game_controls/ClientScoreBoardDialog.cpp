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

#define EXTRAINFO_HEIGHT		280
#define EXTRAINFO_MARGIN		5
#define SPECLIST_HEIGHT			40
#define SPECLIST_MARGIN			5
#define SPECBUTTON_WIDTH		90
#define SPECBUTTON_HEIGHT		35
#define SPECBUTTON_MARGIN		5
#define SIDESEPARATOR_WIDTH		3			
#define SIDESEPARATOR_MARGIN	3			

enum { PANEL_TOPMARGIN = 70, PANEL_MARGIN = 5, PANEL_WIDTH = (1024 - 2 * PANEL_MARGIN), PANEL_HEIGHT = (768 - PANEL_TOPMARGIN - PANEL_MARGIN) };
enum { TEAMCREST_SIZE = 48, TEAMCREST_VMARGIN = 7, TEAMCREST_HOFFSET = 240, TEAMCREST_VOFFSET = 10 };

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

	m_pPlayerStats = new CStatsMenu(m_pExtraInfoPanel, "");
	m_pPlayerStats->SetVisible(false);

	m_pSettingsPanel = new CSettingsMenu(m_pExtraInfoPanel, "");
	m_pSettingsPanel->SetVisible(false);

	m_pFormation = new CFormationMenu(m_pExtraInfoPanel, "");
	m_pFormation->AddActionSignalTarget(this);

	for (int i = 0; i < 2; i++)
	{
		m_pPlayerList[i] = new SectionedListPanel(m_pMainPanel, "PlayerList");
		m_pPlayerList[i]->SetVerticalScrollbar(false);

		//LoadControlSettings("Resource/UI/ScoreBoard.res");
		m_pPlayerList[i]->SetVisible(false); // hide this until we load the images in applyschemesettings
		m_pPlayerList[i]->AddActionSignalTarget(this);
		m_pPlayerList[i]->EnableMouseEnterSelection(true);

		m_pSideSeparators[i] = new Panel(m_pMainPanel);

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

	m_pSpectatorContainer = new Panel(m_pMainPanel);

	m_pSpectatorNames = new Label(m_pSpectatorContainer, "", "");

	m_pSpectateButton = new Button(m_pSpectatorContainer, "SpectateButton", "Spectate");
	m_pSpectateButton->SetCommand(VarArgs("jointeam %d 0", TEAM_SPECTATOR));
	m_pSpectateButton->AddActionSignalTarget(this);

	m_pSettingsButton = new Button(m_pSpectatorContainer, "m_pSettingsButton", "Settings");
	m_pSettingsButton->SetCommand("settings");
	m_pSettingsButton->AddActionSignalTarget(this);

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

	m_pPlayerStats->Reset();

	m_iSectionId = 0;
	m_fNextUpdateTime = 0;
	// add all the sections
	InitScoreboardSections();
}

//-----------------------------------------------------------------------------
// Purpose: adds all the team sections to the scoreboard
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::InitScoreboardSections()
{
	AddHeader(); //create the sections and header //IOS scoreboard
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
	m_pMainPanel->SetBounds(GetWide() / 2 - PANEL_WIDTH / 2, PANEL_TOPMARGIN, PANEL_WIDTH, PANEL_HEIGHT);

	if ( m_pImageList )
		delete m_pImageList;

	m_pImageList = new ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();
	memset( &m_iImageAvatars, 0, sizeof(int) * (MAX_PLAYERS+1) );

	m_pExtraInfoPanel->SetBounds(EXTRAINFO_MARGIN, m_pMainPanel->GetTall() - EXTRAINFO_HEIGHT - EXTRAINFO_MARGIN, m_pMainPanel->GetWide() - 2 * EXTRAINFO_MARGIN, EXTRAINFO_HEIGHT);
	//m_pExtraInfoPanel->SetBgColor(Color(0, 0, 0, 200));

	m_pPlayerStats->SetBounds(0, 0, m_pPlayerStats->GetParent()->GetWide(), m_pPlayerStats->GetParent()->GetTall());
	m_pPlayerStats->PerformLayout();

	m_pFormation->SetBounds(0, 0, m_pExtraInfoPanel->GetWide(), m_pExtraInfoPanel->GetTall());
	m_pFormation->PerformLayout();

	m_pSpectatorContainer->SetBounds(0, m_pMainPanel->GetTall() - EXTRAINFO_HEIGHT - SPECLIST_HEIGHT - SPECLIST_MARGIN, m_pMainPanel->GetWide(), SPECLIST_HEIGHT);
	m_pSpectatorContainer->SetBgColor(Color(0, 0, 0, 150));

	m_pSpectatorFontList[0] = m_pScheme->GetFont("SpectatorListNormal");
	m_pSpectatorFontList[1] = m_pScheme->GetFont("SpectatorListSmall");
	m_pSpectatorFontList[2] = m_pScheme->GetFont("SpectatorListSmaller");
	m_pSpectatorFontList[3] = m_pScheme->GetFont("SpectatorListSmallest");

	m_pSpectatorNames->SetBounds(SPECLIST_MARGIN, 0, m_pSpectatorNames->GetParent()->GetWide() - SPECLIST_MARGIN - 2 * (SPECBUTTON_WIDTH + SPECBUTTON_MARGIN), m_pSpectatorNames->GetParent()->GetTall());
	m_pSpectatorNames->SetFgColor(Color(255, 255, 255, 255));
	m_pSpectatorNames->SetFont(m_pSpectatorFontList[0]);

	m_pSpectateButton->SetBounds(m_pSpectateButton->GetParent()->GetWide() - 2 * (SPECBUTTON_WIDTH + SPECBUTTON_MARGIN), SPECBUTTON_MARGIN, SPECBUTTON_WIDTH, SPECLIST_HEIGHT - 2 * SPECBUTTON_MARGIN);
	m_pSpectateButton->SetDefaultColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
	m_pSpectateButton->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
	m_pSpectateButton->SetDepressedColor(Color(100, 100, 100, 255), Color(200, 200, 200, 255));
	m_pSpectateButton->SetCursor(dc_hand);
	m_pSpectateButton->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
	m_pSpectateButton->SetContentAlignment(Label::a_center);

	m_pSettingsButton->SetBounds(m_pSettingsButton->GetParent()->GetWide() - SPECBUTTON_WIDTH - SPECBUTTON_MARGIN, SPECBUTTON_MARGIN, SPECBUTTON_WIDTH, SPECLIST_HEIGHT - 2 * SPECBUTTON_MARGIN);
	m_pSettingsButton->SetDefaultColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
	m_pSettingsButton->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
	m_pSettingsButton->SetDepressedColor(Color(100, 100, 100, 255), Color(200, 200, 200, 255));
	m_pSettingsButton->SetCursor(dc_hand);
	m_pSettingsButton->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
	m_pSettingsButton->SetContentAlignment(Label::a_center);

	m_pSettingsPanel->SetBounds(0, 0, m_pSettingsPanel->GetParent()->GetWide(), m_pSettingsPanel->GetParent()->GetTall());

	for (int i = 0; i < 2; i++)
	{
		//m_pTeamCrests[i]->SetBounds(GetWide() / 2 - TEAMCREST_SIZE / 2 + (i == 0 ? -1 : 1) * TEAMCREST_HOFFSET, TEAMCREST_VOFFSET, TEAMCREST_SIZE, TEAMCREST_SIZE);
		//m_pTeamCrests[i]->SetShouldScaleImage(true);
		//m_pTeamCrests[i]->SetImage(i == 0 ? "hometeamcrest" : "awayteamcrest");

		m_pPlayerList[i]->SetBounds(i * (m_pMainPanel->GetWide() / 2), 0, m_pMainPanel->GetWide() / 2, m_pMainPanel->GetTall() - EXTRAINFO_HEIGHT - SPECLIST_HEIGHT - SPECLIST_MARGIN);
		m_pPlayerList[i]->SetPaintBorderEnabled(false);

		m_pSideSeparators[i]->SetBgColor(Color(0, 0, 0, 150));
	}

	m_pSideSeparators[0]->SetBounds(m_pMainPanel->GetWide() / 2 - SIDESEPARATOR_WIDTH / 2, SIDESEPARATOR_MARGIN, SIDESEPARATOR_WIDTH, m_pSpectatorContainer->GetY() - 2 * SIDESEPARATOR_MARGIN);
	int yPos = m_pSpectatorContainer->GetY() + m_pSpectatorContainer->GetTall() + SIDESEPARATOR_MARGIN;
	m_pSideSeparators[1]->SetBounds(m_pMainPanel->GetWide() / 2 - SIDESEPARATOR_WIDTH / 2, yPos, SIDESEPARATOR_WIDTH, m_pMainPanel->GetTall() - yPos - SIDESEPARATOR_MARGIN);

	for (int i = 0; i < 2; i++)
	{
		if (m_pPlayerList[i])
		{
			m_pPlayerList[i]->SetImageList( m_pImageList, false );
			m_pPlayerList[i]->SetVisible( true );
		}
	}
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
		Reset();
		Update();
		SetVisible( true );
		MoveToFront();
		SetKeyBoardInputEnabled(false);
		SetMouseInputEnabled(true);
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

	FillScoreBoard();

	for (int i = 0; i < 2; i++)
	{
		m_pTeamCrests[i]->SetVisible(gr->HasTeamCrest(i + TEAM_A));
	}

	if (!m_pSettingsPanel->IsVisible())
	{
		bool showPlayerStats = false;
		int playerIndices[2] = { 0, 0 };

		for (int i = 0; i < 2; i++)
		{
			int itemID = m_pPlayerList[i]->GetSelectedItem();
			if (itemID != -1)
			{
				playerIndices[i] = m_pPlayerList[i]->GetItemData(itemID)->GetInt("playerindex");
				showPlayerStats = true;
			}
		}

		if (showPlayerStats)
			m_pPlayerStats->Update(playerIndices);

		m_pPlayerStats->SetVisible(showPlayerStats);
		m_pFormation->SetVisible(!showPlayerStats);
	}

	m_pFormation->Update();

	m_fNextUpdateTime = gpGlobals->curtime + 0.25f; 
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdateTeamInfo()
{
	//IGameResources *gr = GameResources();
	//if (!gr)
	//	return;

	//int pingSum[2] = { 0, 0 };

	//for (int i = 1; i <= gpGlobals->maxClients; i++)
	//{
	//	if (!gr->IsConnected(i) || gr->GetTeam(i) != TEAM_A && gr->GetTeam(i) != TEAM_B)
	//		continue;

	//	pingSum[gr->GetTeam(i) - TEAM_A] += gr->GetPing(i);
	//}

	//for (int i = 0; i < 2; i++)
	//{
	//	wchar_t wszText[64];
	//	C_Team *pTeam = GetGlobalTeam(TEAM_A + i);
	//	_snwprintf(wszText, ARRAYSIZE(wszText), L"%d", pTeam->GetNumPlayers());
	//	m_pPlayerList[i]->ModifyColumn(0, "playercount", wszText);
	//	_snwprintf(wszText, ARRAYSIZE(wszText), L"%d", pTeam->Get_Goals());
	//	m_pPlayerList[i]->ModifyColumn(0, "goals", wszText);
	//	g_pVGuiLocalize->ConvertANSIToUnicode(pTeam->Get_FullTeamName(), wszText, sizeof(wszText));
	//	m_pPlayerList[i]->ModifyColumn(0, "name", wszText);
	//	_snwprintf(wszText, ARRAYSIZE(wszText), L"%d%%", pTeam->Get_Possession());
	//	m_pPlayerList[i]->ModifyColumn(0, "possession", wszText);
	//	_snwprintf(wszText, ARRAYSIZE(wszText), L"%d", pingSum[0] / pTeam->GetNumPlayers());
	//	m_pPlayerList[i]->ModifyColumn(0, "ping", wszText);
	//}
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

	// walk all the players and make sure they're in the scoreboard
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (gr->IsConnected( i ))
		{
			if (gr->GetTeam(i) != TEAM_A && gr->GetTeam(i) != TEAM_B)
			{
				Q_strncat(spectatorNames, VarArgs("%s %s", (spectatorCount == 0 ? "" : ","), gr->GetPlayerName(i)), sizeof(spectatorNames));
				spectatorCount += 1;

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
				GetPlayerScoreInfo( i, playerData );

				UpdatePlayerAvatar( i, playerData );

				if (Q_strlen(gr->GetCountryName(i)) > 0)
				{
					IImage *countryFlag = scheme()->GetImage(VarArgs("countryflags/%s", gr->GetCountryName(i)), false);
					countryFlag->SetSize(32, 24);
					int imageIndex = m_pImageList->AddImage(countryFlag);
					m_pImageList->GetImage(imageIndex)->SetPos(10, 10);
					playerData->SetInt("country", imageIndex);
				}

				const char *oldName = playerData->GetString("name","");
				char newName[MAX_PLAYER_NAME_LENGTH];

				UTIL_MakeSafeName( oldName, newName, MAX_PLAYER_NAME_LENGTH );

				//IOS scoreboard
				playerData->SetString("name", newName);

				int side = -1;
				int itemID = FindItemIDForPlayerIndex(i, side);
				int team = gr->GetTeam(i); //omega; set a variable to team so we can reuse it
				int teamIndex = team - TEAM_A;
				int sectionID = 0;//iTeamSections[playerTeam]; //omega; make sure it goes into the proper section

				if (itemID == -1)
				{
					// add a new row
					itemID = m_pPlayerList[teamIndex]->AddItem( sectionID, playerData );
				}
				else
				{
					// modify the current row
					m_pPlayerList[side]->ModifyItem( itemID, sectionID, playerData );
				}

				// set the row color based on the players team
				m_pPlayerList[teamIndex]->SetItemFgColor( itemID, gr->GetTeamColor( team ) );
				m_pPlayerList[teamIndex]->SetItemFont( itemID, m_pScheme->GetFont("IOSTeamMenuNormal"));

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

	if (spectatorCount == 0)
		Q_strncpy(spectatorText, "No spectators", sizeof(spectatorText));
	else
		Q_snprintf(spectatorText, sizeof(spectatorText), "%d %s: %s", spectatorCount, (spectatorCount == 1 ? "spectator" : "spectators"), spectatorNames);

	m_pSpectatorNames->SetText(spectatorText);

	for (int i = 0; i < SPEC_FONT_COUNT; i++)
	{
		int width, height;
		m_pSpectatorNames->SetFont(m_pSpectatorFontList[i]);
		m_pSpectatorNames->GetTextImage()->GetContentSize(width, height);

		if (width <= m_pSpectatorNames->GetWide())
			break;
	}
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
		m_pPlayerList[i]->SetLineSpacing(30);
		m_pPlayerList[i]->SetSectionDividerColor(m_iSectionId, Color(255, 255, 255, 255));
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "posname",		"Pos.", 0, 50 );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "country",		"Nat.", SectionedListPanel::COLUMN_IMAGE, 50 );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "club",			"Club", 0,  50);
		if ( ShowAvatars() )
		{
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "avatar",		"", SectionedListPanel::COLUMN_IMAGE, 50 );
		}
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "name",			"Name", 0, 150);
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goals",			"Goals", 0, 50);
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "assists",		"Assists", 0, 50);
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "possession",		"Poss.", 0, 50);
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "ping",			"Ping", 0, 50);
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

#define GET_STAT_TEXT(val) (val > 0 ? VarArgs("%d", val) : "")

//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
bool CClientScoreBoardDialog::GetPlayerScoreInfo(int playerIndex, KeyValues *kv)
{
	IGameResources *gr = GameResources();

	if (!gr )
		return false;

	kv->SetString("goals", GET_STAT_TEXT(gr->GetGoals(playerIndex)));
	kv->SetString("assists", GET_STAT_TEXT(gr->GetAssists(playerIndex)));
	kv->SetString("possession", VarArgs("%d%%", gr->GetPossession(playerIndex)));
	kv->SetString("redcards", GET_STAT_TEXT(gr->GetRedCards(playerIndex)));
	kv->SetString("yellowcards", GET_STAT_TEXT(gr->GetYellowCards(playerIndex)));
	kv->SetString("fouls", GET_STAT_TEXT(gr->GetFouls(playerIndex)));
	kv->SetString("passes", GET_STAT_TEXT(gr->GetPasses(playerIndex)));
	kv->SetString("freekicks", GET_STAT_TEXT(gr->GetFreeKicks(playerIndex)));
	kv->SetString("penalties", GET_STAT_TEXT(gr->GetPenalties(playerIndex)));
	kv->SetString("corners", GET_STAT_TEXT(gr->GetCorners(playerIndex)));
	kv->SetString("throws", GET_STAT_TEXT(gr->GetThrowIns(playerIndex)));
	kv->SetString("saves", GET_STAT_TEXT(gr->GetKeeperSaves(playerIndex)));
	kv->SetString("goalkicks", GET_STAT_TEXT(gr->GetGoalKicks(playerIndex)));
	kv->SetInt("posindex", gr->GetTeamPosIndex(playerIndex));
	kv->SetInt("playerindex", playerIndex);
	//kv->SetString("country", gr->GetCountryName(playerIndex));
	kv->SetString("posname", g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][gr->GetTeamPosIndex(playerIndex)][POS_NAME]]);
	kv->SetString("name", gr->GetPlayerName( playerIndex ) );
	kv->SetString("club", gr->GetClubName(playerIndex));
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
// Purpose: reload the player list on the scoreboard
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::FillScoreBoard()
{
	//ios scoreboard - move so numPlayersOnTeam is calculated first
	UpdatePlayerInfo();

	// update totals information
	UpdateTeamInfo();

	// update player info
	//UpdatePlayerInfo();
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
	if (!strnicmp(cmd, "jointeam", 8))
	{
		engine->ClientCmd(cmd);
	}
	else if (!stricmp(cmd, "settings"))
	{
		bool isVisible = m_pSettingsPanel->IsVisible();
		m_pSettingsPanel->SetVisible(!isVisible);
		m_pSideSeparators[1]->SetVisible(isVisible);
		m_pPlayerStats->SetVisible(!isVisible);
		m_pFormation->SetVisible(isVisible);
	}
	else
		BaseClass::OnCommand(cmd);
}

void CClientScoreBoardDialog::OnItemSelected(KeyValues *data)
{
	//if (m_pSettingsPanel->IsVisible())
	//	return;

	SectionedListPanel *pCurPanel = (SectionedListPanel *)data->GetPtr("panel");
	SectionedListPanel *pOtherPanel = (pCurPanel == m_pPlayerList[0] ? m_pPlayerList[1] : m_pPlayerList[0]);
	int itemID = data->GetInt("itemID");
	int oldItemID = data->GetInt("oldItemID");
	bool select = true;

	if (itemID == -1)
	{
		itemID = oldItemID;
		select = false;
	}

	if (itemID != -1)
	{
		int posindex = pCurPanel->GetItemData(itemID)->GetInt("posindex");

		for (int i = 0; i <= pOtherPanel->GetHighestItemID(); i++)
		{
			if (pOtherPanel->IsItemIDValid(i) && pOtherPanel->GetItemData(i)->GetInt("posindex") == posindex)
			{
				if (select)
					pOtherPanel->SetSelectedItem(i);
				else
				{
					if (pOtherPanel->GetSelectedItem() == i)
						pOtherPanel->ClearSelection();
				}

				break;
			}
		}
	}

	Update();
}