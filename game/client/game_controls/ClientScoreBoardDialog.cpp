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

#include "sdk_gamerules.h" //include before memdbgon.h		//IOS scoreboard

#include "vgui_avatarimage.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

bool AvatarIndexLessFunc( const int &lhs, const int &rhs )	
{ 
	return lhs < rhs; 
}

#define PANEL_MARGIN		5
#define PANEL_WIDTH			(1024 - 2 * PANEL_MARGIN)
#define PANEL_HEIGHT		(768 - 2 * PANEL_MARGIN)
#define EXTRAINFO_HEIGHT		400
#define EXTRAINFO_MARGIN		5

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClientScoreBoardDialog::CClientScoreBoardDialog(IViewPort *pViewPort) : EditablePanel( NULL, PANEL_SCOREBOARD )
{
	m_iPlayerIndexSymbol = KeyValuesSystem()->GetSymbolForString("playerIndex");
	m_nCloseKey = BUTTON_CODE_INVALID;

	//memset(s_VoiceImage, 0x0, sizeof( s_VoiceImage ));
	TrackerImage = 0;
	m_pViewPort = pViewPort;

	SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);

	// initialize dialog
	SetProportional(false);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	// set the scheme before any child control is created
	//SetScheme("ClientScheme");

	for (int i = 0; i < 2; i++)
	{
		m_pMainPanels[i] = new Panel(this);
		m_pPlayerList[i] = new SectionedListPanel(m_pMainPanels[i], "PlayerList");
		if (m_pPlayerList[i])
			m_pPlayerList[i]->SetVerticalScrollbar(false);

		//LoadControlSettings("Resource/UI/ScoreBoard.res");
		m_iDesiredHeight = GetTall();
		if (m_pPlayerList[i])
			m_pPlayerList[i]->SetVisible( false ); // hide this until we load the images in applyschemesettings
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

	m_pExtraInfoPanel = new Panel(this);
	m_pExtraInfoList = new SectionedListPanel(m_pExtraInfoPanel, "");
	m_pExtraInfoList->SetVerticalScrollbar(false);
	m_pSpectatorNames = new Label(this, "", "");

	m_iAvatarWidth = 34;
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
	m_pExtraInfoList->DeleteAllItems();
	m_pExtraInfoList->RemoveAllSections();
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
	//BaseClass::ApplySchemeSettings( pScheme );
	ImageList *imageList = new ImageList(false);

	if ( m_pImageList )
		delete m_pImageList;
	m_pImageList = new ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();
	memset( &m_iImageAvatars, 0, sizeof(int) * (MAX_PLAYERS+1) );

	for (int i = 0; i < 2; i++)
	{
		m_pMainPanels[i]->SetBounds(i * (GetWide() / 2), 0, GetWide() / 2, GetTall() - EXTRAINFO_HEIGHT);
	}

	m_pExtraInfoPanel->SetBounds(EXTRAINFO_MARGIN, GetTall() - EXTRAINFO_HEIGHT - EXTRAINFO_MARGIN, GetWide() - 2 * EXTRAINFO_MARGIN, EXTRAINFO_HEIGHT);
	m_pExtraInfoPanel->SetBgColor(Color(0, 0, 0, 200));

	m_pSpectatorNames->SetBounds(5, GetTall() - EXTRAINFO_HEIGHT - 20, GetWide(), 20);
	m_pSpectatorNames->SetFgColor(Color(200, 200, 200, 255));

	PostApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: Does dialog-specific customization after applying scheme settings.
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::PostApplySchemeSettings( vgui::IScheme *pScheme )
{
	// resize the images to our resolution
	for (int i = 0; i < m_pImageList->GetImageCount(); i++ )
	{
		int wide, tall;
		m_pImageList->GetImage(i)->GetSize(wide, tall);
		m_pImageList->GetImage(i)->SetSize(scheme()->GetProportionalScaledValueEx( GetScheme(),wide), scheme()->GetProportionalScaledValueEx( GetScheme(),tall));
	}

	for (int i = 0; i < 2; i++)
	{
		if (m_pPlayerList[i])
		{
			m_pPlayerList[i]->SetImageList( m_pImageList, false );
			m_pPlayerList[i]->SetVisible( true );
		}
	}

	// light up scoreboard a bit
	//SetBgColor( Color( 0,0,0,0) );
	SetPaintBorderEnabled(true);
	SetBorder(NULL);
	SetBgColor( Color( 0, 0, 0, 220 ) );
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
	// Set the title

	// Reset();
	for (int i = 0; i < 2; i++)
	{
		m_pPlayerList[i]->DeleteAllItems();
	}
	FillScoreBoard();
	for (int i = 0; i < 2; i++)
	{
		// grow the scoreboard to fit all the players
		int wide, tall;
		m_pPlayerList[i]->GetContentSize(wide, tall);
		tall += GetAdditionalHeight();
		wide = GetWide();
		if (m_iDesiredHeight < tall)
		{
			SetSize(wide, tall);
			m_pPlayerList[i]->SetSize(wide, tall);
		}
		else
		{
			SetSize(wide, m_iDesiredHeight);
			m_pPlayerList[i]->SetSize(wide, m_iDesiredHeight);
		}
	}

	MoveToCenterOfScreen();

	// update every second
	m_fNextUpdateTime = gpGlobals->curtime + 1.0f; 
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdateTeamInfo()
{
	return;
	//IOS Scorebaord
	int sectionId;
	char cstring[256];
	char plural[2];
	wchar_t tn[256];
	IGameResources *gr = GameResources();
	if ( !gr )
		return;

	for (int i = 0; i < 2; i++)
	{
		sectionId = 2;//iTeamSections[i]; //get the section for the team

		if (numPlayersOnTeam[i] == 1)
			sprintf(plural,"");
		else
			sprintf(plural,"s");

		sprintf(cstring, "%s - (%i player%s)", gr->GetShortTeamName(i),  numPlayersOnTeam[i], plural);
		g_pVGuiLocalize->ConvertANSIToUnicode(cstring, tn, sizeof(tn));
		m_pPlayerList[i]->ModifyColumn(sectionId, "name", tn);

		if ( numPlayersOnTeam[i] > 0 )
			teamLatency[i] /= numPlayersOnTeam[i];
		else
			teamLatency[i] = 0;

		wchar_t sz[64];
		swprintf(sz, L"%d", gr->GetTeamGoals(i));
		m_pPlayerList[i]->ModifyColumn(sectionId, "goals", sz);
		if (teamLatency[i] < 1)
		{
			m_pPlayerList[i]->ModifyColumn(sectionId, "ping", L"");
		}
		else
		{
			swprintf(sz, L"%i", teamLatency[i]);
			m_pPlayerList[i]->ModifyColumn(sectionId, "ping", sz);
		}
		teamLatency[i] = 0; //omega; reset so next time when the scorebaord is drawn,
		//it doesn't add the old values in and make the ping
		//grow and grow and grow! ;)

		swprintf(sz, L"%d", m_TeamRedCard[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "redcards", sz);
		swprintf(sz, L"%d", m_TeamYellowCard[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "yellowcards", sz);
		swprintf(sz, L"%d", m_TeamFouls[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "fouls", sz);
		swprintf(sz, L"%d", m_TeamAssists[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "assists", sz);
		swprintf(sz, L"%d", m_TeamPossession[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "possession", sz);
		swprintf(sz, L"%d", m_TeamPasses[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "passes", sz);
		swprintf(sz, L"%d", m_TeamFreeKicks[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "freekicks", sz);
		swprintf(sz, L"%d", m_TeamPenalties[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "penalties", sz);
		swprintf(sz, L"%d", m_TeamCorners[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "corners", sz);
		swprintf(sz, L"%d", m_TeamThrowIns[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "throws", sz);
		swprintf(sz, L"%d", m_TeamKeeperSaves[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "saves", sz);
		swprintf(sz, L"%d", m_TeamGoalKicks[i]);
		m_pPlayerList[i]->ModifyColumn(sectionId, "goalkicks", sz);

		m_pPlayerList[i]->SetSectionFgColor(sectionId, gr->GetTeamColor(i)); //mm colors
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdatePlayerInfo()
{
	m_iSectionId = 0; // 0'th row is a header
	int selectedRow = -1;

	char spectatorNames[1024] = "";
	int spectatorCount = 0;

	// walk all the players and make sure they're in the scoreboard
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		IGameResources *gr = GameResources();

		if ( gr && gr->IsConnected( i ) )
		{
			if (gr->GetTeam(i) != TEAM_A && gr->GetTeam(i) != TEAM_B)
			{
				Q_strncat(spectatorNames, VarArgs("%s %s", (spectatorCount == 0 ? "" : ","), gr->GetPlayerName(i)), sizeof(spectatorNames));
				spectatorCount += 1;
				continue;
			}

			// add the player to the list
			KeyValues *playerData = new KeyValues("data");
			GetPlayerScoreInfo( i, playerData );

			//UpdatePlayerAvatar( i, playerData );

			if (Q_strlen(gr->GetCountryName(i)) > 0)
			{
				IImage *countryFlag = scheme()->GetImage(VarArgs("countryflags/%s", gr->GetCountryName(i)), false);
				countryFlag->SetSize(32, 32);
				int imageIndex = m_pImageList->AddImage(countryFlag);
				playerData->SetInt("country", imageIndex);
			}

			const char *oldName = playerData->GetString("name","");
			char newName[MAX_PLAYER_NAME_LENGTH];

			UTIL_MakeSafeName( oldName, newName, MAX_PLAYER_NAME_LENGTH );


			//IOS scoreboard
			playerData->SetString("name", newName);

			int side = -1;
			int itemID = FindItemIDForPlayerIndex(i, side);
			int playerTeam = gr->GetTeam(i); //omega; set a variable to team so we can reuse it
			int sectionID = 0;//iTeamSections[playerTeam]; //omega; make sure it goes into the proper section

			//omega!!!
			numPlayersOnTeam[playerTeam]++; //increment player count and add this player
			teamLatency[playerTeam]+=playerData->GetInt("ping");
			m_TeamRedCard[playerTeam] += gr->GetRedCards(i);
			m_TeamYellowCard[playerTeam] += gr->GetYellowCards(i);
			m_TeamFouls[playerTeam] += gr->GetFouls(i);
			m_TeamAssists[playerTeam] += gr->GetAssists(i);
			m_TeamPossession[playerTeam] += gr->GetPossession(i);
			m_TeamPasses[playerTeam] += gr->GetPasses(i);
			m_TeamFreeKicks[playerTeam] += gr->GetFreeKicks(i);
			m_TeamPenalties[playerTeam] += gr->GetPenalties(i);
			m_TeamCorners[playerTeam] += gr->GetCorners(i);
			m_TeamThrowIns[playerTeam] += gr->GetThrowIns(i);
			m_TeamKeeperSaves[playerTeam] += gr->GetKeeperSaves(i);
			m_TeamGoalKicks[playerTeam] += gr->GetGoalKicks(i);

			if ( gr->IsLocalPlayer( i ) )
			{
				selectedRow = itemID;
			}
			if (itemID == -1)
			{
				// add a new row
				itemID = m_pPlayerList[playerTeam - TEAM_A]->AddItem( sectionID, playerData );
			}
			else
			{
				// modify the current row
				m_pPlayerList[side]->ModifyItem( itemID, sectionID, playerData );
			}

			// set the row color based on the players team
			m_pPlayerList[playerTeam - TEAM_A]->SetItemFgColor( itemID, gr->GetTeamColor( playerTeam ) );
			m_pPlayerList[playerTeam - TEAM_A]->SetItemFont( itemID, m_pScheme->GetFont("IOSTeamMenuNormal"));

			playerData->deleteThis();
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

	if ( selectedRow != -1 )
	{
		m_pPlayerList[0]->SetSelectedItem(selectedRow);
	}

	char spectatorText[1024];

	if (spectatorCount == 0)
		Q_strncpy(spectatorText, "No spectators", sizeof(spectatorText));
	else
		Q_snprintf(spectatorText, sizeof(spectatorText), "%d %s: %s", spectatorCount, (spectatorCount == 1 ? "Spectator" : "Spectators"), spectatorNames);

	m_pSpectatorNames->SetText(spectatorText);
}

//-----------------------------------------------------------------------------
// Purpose: adds the top header of the scoreboars
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::AddHeader()
{
	m_iSectionId = 0; //make a blank one

	for (int i = 0; i < 2; i++)
	{
		m_iSectionId = 0;
		//m_pPlayerList[i]->AddSection(m_iSectionId, "");
		//m_pPlayerList[i]->SetSectionAlwaysVisible(m_iSectionId);
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "name", "", 0, scheme()->GetProportionalScaledValue(NAME_WIDTH) );

		//m_iSectionId = 0;
		m_pPlayerList[i]->AddSection(m_iSectionId, "");
		m_pPlayerList[i]->SetSectionAlwaysVisible(m_iSectionId);
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "position",		"Pos", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		if ( ShowAvatars() )
		{
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "avatar",		"Avatar", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		}
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "country",		"Country", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue(SMALL_WIDTH) );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "club",			"Club", 0, scheme()->GetProportionalScaledValue(SMALL_WIDTH) );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "name",			"Player", 0, scheme()->GetProportionalScaledValue(NAME_WIDTH) );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goals",		"Goals", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "assists",		"Assists", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//   m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "possession",	"%", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//   m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "redcards",		"Red", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//   m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "yellowcards",	"Yel", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//   m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "fouls",		"Foul", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "passes",		"Pass", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//   m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "freekicks",	"FKs", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//   m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "penalties",	"Pen", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//   m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "corners",		"Cnr", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//   m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "throws",		"Thr", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//   m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "saves",		"Sav", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//   m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goalkicks",	"GKs", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "ping",			"Ping", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "voice",		"Voice", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue(VOICE_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "tracker", "#PlayerTracker", SectionedListPanel::COLUMN_IMAGE, scheme()->GetProportionalScaledValueEx( GetScheme(),FRIENDS_WIDTH) );

	}

	//m_iSectionId = 1; //first team;
	//iTeamSections[TEAM_A] = 1;
	//AddSection(TYPE_TEAM, TEAM_A);
	//iTeamSections[TEAM_B] = 3;
	//AddSection(TYPE_TEAM, TEAM_B);
	//iTeamSections[TEAM_SPECTATOR]   = AddSection(TYPE_SPECTATORS, TEAM_SPECTATOR);
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new section to the scoreboard (i.e the team header)
//-----------------------------------------------------------------------------
int CClientScoreBoardDialog::AddSection(int teamType, int teamNumber)
{
	//IOS Scoreboad
	if ( teamType == TYPE_TEAM )
	{
		int i = teamNumber - TEAM_A;

		m_iSectionId = 1 + i * 2;

		IGameResources *gr = GameResources();

		if ( !gr )
			return -1;

		// setup the team name
		wchar_t *teamName = g_pVGuiLocalize->Find( gr->GetShortTeamName(teamNumber) );
		wchar_t name[64];

		if (!teamName)
		{
			g_pVGuiLocalize->ConvertANSIToUnicode(gr->GetShortTeamName(teamNumber), name, sizeof(name));
			teamName = name;
		}

		m_pPlayerList[i]->AddSection(m_iSectionId, "", StaticPlayerSortFunc);
		m_pPlayerList[i]->SetSectionAlwaysVisible(m_iSectionId);
		m_pPlayerList[i]->SetFgColor(gr->GetTeamColor(teamNumber));

		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "position",		"", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );

		// Avatars are always displayed at 32x32 regardless of resolution
		if ( ShowAvatars() )
		{
			m_pPlayerList[i]->AddColumnToSection( m_iSectionId, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, m_iAvatarWidth );
		}
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "country",		"", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, m_iAvatarWidth );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "club",			"", 0, scheme()->GetProportionalScaledValue(SMALL_WIDTH) );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "name",			teamName, 0, scheme()->GetProportionalScaledValue(NAME_WIDTH) );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goals",		"0", 0, scheme()->GetProportionalScaledValue(SMALL_WIDTH) );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "assists",		"0", 0, scheme()->GetProportionalScaledValue(SMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "possession",	"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "redcards",		"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "yellowcards",	"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "fouls",		"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "passes",		"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "freekicks",	"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "penalties",	"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "corners",		"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "throws",		"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "saves",		"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goalkicks",	"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "ping",			"0", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "voice", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue(VOICE_WIDTH) );
	}

	//m_iSectionId++; //increment for next
	return m_iSectionId;//-1;
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
	int v1 = it1->GetInt("goals");
	int v2 = it2->GetInt("goals");
	if (v1 > v2)
		return true;
	else if (v1 < v2)
		return false;

	// next compare deaths
	v1 = it1->GetInt("assists");
	v2 = it2->GetInt("assists");
	if (v1 > v2)
		return false;
	else if (v1 < v2)
		return true;

	// the same, so compare itemID's (as a sentinel value to get deterministic sorts)
	return itemID1 < itemID2;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
bool CClientScoreBoardDialog::GetPlayerScoreInfo(int playerIndex, KeyValues *kv)
{
	IGameResources *gr = GameResources();

	if (!gr )
		return false;

	kv->SetInt("goals", gr->GetGoals( playerIndex ) );
	kv->SetString("name", gr->GetPlayerName( playerIndex ) );
	kv->SetInt("playerIndex", playerIndex);
	kv->SetInt("assists", gr->GetAssists(playerIndex));
	kv->SetInt("possession", gr->GetPossession(playerIndex));
	kv->SetInt("redcards", gr->GetRedCards(playerIndex));
	kv->SetInt("yellowcards", gr->GetYellowCards(playerIndex));
	kv->SetInt("fouls", gr->GetFouls(playerIndex));
	kv->SetInt("passes", gr->GetPasses(playerIndex));
	kv->SetInt("freekicks", gr->GetFreeKicks(playerIndex));
	kv->SetInt("penalties", gr->GetPenalties(playerIndex));
	kv->SetInt("corners", gr->GetCorners(playerIndex));
	kv->SetInt("throws", gr->GetThrowIns(playerIndex));
	kv->SetInt("saves", gr->GetKeeperSaves(playerIndex));
	kv->SetInt("goalkicks", gr->GetGoalKicks(playerIndex));
	kv->SetString("position", g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][gr->GetTeamPosIndex(playerIndex)][POS_NAME]]);
	//kv->SetString("country", gr->GetCountryName(playerIndex));
	kv->SetString("club", gr->GetClubName(playerIndex));
	kv->SetInt("voice",  s_VoiceImage[GetClientVoiceMgr()->GetSpeakerStatus( playerIndex - 1) ]); 

	if (gr->GetPing( playerIndex ) < 1)
	{
		if ( gr->IsFakePlayer( playerIndex ) )
		{
			kv->SetString("ping", "BOT");
		}
		else
		{
			kv->SetString("ping", "HOST");
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
	//IOS Scoreboard
	for (int i = TEAM_UNASSIGNED; i < TEAMS_COUNT; i++)
	{
		numPlayersOnTeam[i] = 0; //clear!
		//clear anything else for the team
		m_TeamRedCard[i] = 0;
		m_TeamYellowCard[i] = 0;
		m_TeamFouls[i] = 0;
		m_TeamAssists[i] = 0;
		m_TeamPossession[i] = 0;
		m_TeamPasses[i] = 0;
		m_TeamFreeKicks[i] = 0;
		m_TeamPenalties[i] = 0;
		m_TeamCorners[i] = 0;
		m_TeamThrowIns[i] = 0;
		m_TeamKeeperSaves[i] = 0;
		m_TeamGoalKicks[i] = 0;

	}

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
