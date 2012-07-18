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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

bool AvatarIndexLessFunc( const int &lhs, const int &rhs )	
{ 
	return lhs < rhs; 
}

#define PANEL_MARGIN			5
#define PANEL_WIDTH				(1024 - 2 * PANEL_MARGIN)
#define PANEL_HEIGHT			(768 - 2 * PANEL_MARGIN)
#define EXTRAINFO_HEIGHT		330
#define EXTRAINFO_MARGIN		5
#define FORMATION_BUTTON_WIDTH	85
#define FORMATION_BUTTON_HEIGHT	50
#define SPECLIST_HEIGHT			40
#define SPECLIST_MARGIN			5
#define SPECBUTTON_WIDTH		90
#define SPECBUTTON_HEIGHT		35
#define SPECBUTTON_MARGIN		5
#define SIDESEPARATOR_WIDTH		3			
#define SIDESEPARATOR_MARGIN	3			

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

	// initialize dialog
	SetProportional(false);

	// set the scheme before any child control is created
	//SetScheme("ClientScheme");

	m_pExtraInfoPanel = new Panel(this);

	for (int i = 0; i < 2; i++)
	{
		m_pPlayerList[i] = new SectionedListPanel(this, "PlayerList");
		m_pPlayerList[i]->SetVerticalScrollbar(false);

		//LoadControlSettings("Resource/UI/ScoreBoard.res");
		m_pPlayerList[i]->SetVisible( false ); // hide this until we load the images in applyschemesettings
		m_pPlayerList[i]->AddActionSignalTarget(this);
		m_pPlayerList[i]->EnableMouseEnterSelection(true);

		m_pFormations[i] = new Panel(m_pExtraInfoPanel);

		m_pSideSeparators[i] = new Panel(this);

		for (int j = 0; j < 11; j++)
		{
			m_pFormationButtons[i][j] = new CBitmapButton(m_pFormations[i], "", "JOIN");
			m_pFormationButtons[i][j]->SetCommand(VarArgs("jointeam %d %d", i + TEAM_A, j));
			m_pFormationButtons[i][j]->AddActionSignalTarget(this);
		}
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

	m_pSpectatorContainer = new Panel(this);

	m_pSpectatorNames = new Label(m_pSpectatorContainer, "", "");
	m_pSpectateButton = new Button(m_pSpectatorContainer, "SpectateButton", "Spectate");
	m_pSettingsButton = new Button(m_pSpectatorContainer, "m_pSettingsButton", "Settings");

	m_pPlayerStats = new SectionedListPanel(m_pExtraInfoPanel, "");
	m_pPlayerStats->SetVerticalScrollbar(false);
	m_pPlayerStats->SetVisible(false);

	m_pSettingsPanel = new CSettingsMenu(m_pExtraInfoPanel, "");
	m_pSettingsPanel->SetVisible(false);

	m_iAvatarWidth = 34;

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

	m_pPlayerStats->DeleteAllItems();
	m_pPlayerStats->RemoveAllSections();

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

	if ( m_pImageList )
		delete m_pImageList;

	m_pImageList = new ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();
	memset( &m_iImageAvatars, 0, sizeof(int) * (MAX_PLAYERS+1) );
	
	SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);

	m_pExtraInfoPanel->SetBounds(EXTRAINFO_MARGIN, GetTall() - EXTRAINFO_HEIGHT - EXTRAINFO_MARGIN, GetWide() - 2 * EXTRAINFO_MARGIN, EXTRAINFO_HEIGHT);
	//m_pExtraInfoPanel->SetBgColor(Color(0, 0, 0, 200));

	m_pPlayerStats->SetBounds(0, 0, m_pPlayerStats->GetParent()->GetWide(), m_pPlayerStats->GetParent()->GetTall());
	m_pPlayerStats->SetPaintBorderEnabled(false);
	//m_pSpectatorContainer->SetZPos(-2);

	m_pSpectatorContainer->SetBounds(0, GetTall() - EXTRAINFO_HEIGHT - SPECLIST_HEIGHT - SPECLIST_MARGIN, GetWide(), SPECLIST_HEIGHT);
	m_pSpectatorContainer->SetBgColor(Color(0, 0, 0, 150));

	m_pSpectatorFontList[0] = m_pScheme->GetFont("SpectatorListNormal");
	m_pSpectatorFontList[1] = m_pScheme->GetFont("SpectatorListSmall");
	m_pSpectatorFontList[2] = m_pScheme->GetFont("SpectatorListSmaller");
	m_pSpectatorFontList[3] = m_pScheme->GetFont("SpectatorListSmallest");

	m_pSpectatorNames->SetBounds(SPECLIST_MARGIN, 0, m_pSpectatorNames->GetParent()->GetWide() - SPECLIST_MARGIN - 2 * (SPECBUTTON_WIDTH + SPECBUTTON_MARGIN), m_pSpectatorNames->GetParent()->GetTall());
	m_pSpectatorNames->SetFgColor(Color(255, 255, 255, 255));
	m_pSpectatorNames->SetFont(m_pSpectatorFontList[0]);

	m_pSpectateButton->SetBounds(m_pSpectateButton->GetParent()->GetWide() - 2 * (SPECBUTTON_WIDTH + SPECBUTTON_MARGIN), SPECBUTTON_MARGIN, SPECBUTTON_WIDTH, SPECLIST_HEIGHT - 2 * SPECBUTTON_MARGIN);
	m_pSpectateButton->SetCommand(VarArgs("jointeam %d 0", TEAM_SPECTATOR));
	m_pSpectateButton->AddActionSignalTarget(this);
	m_pSpectateButton->SetDefaultColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
	m_pSpectateButton->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
	m_pSpectateButton->SetDepressedColor(Color(100, 100, 100, 255), Color(200, 200, 200, 255));
	m_pSpectateButton->SetCursor(dc_hand);
	m_pSpectateButton->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
	m_pSpectateButton->SetContentAlignment(Label::a_center);

	m_pSettingsButton->SetBounds(m_pSettingsButton->GetParent()->GetWide() - SPECBUTTON_WIDTH - SPECBUTTON_MARGIN, SPECBUTTON_MARGIN, SPECBUTTON_WIDTH, SPECLIST_HEIGHT - 2 * SPECBUTTON_MARGIN);
	m_pSettingsButton->SetCommand("settings");
	m_pSettingsButton->AddActionSignalTarget(this);
	m_pSettingsButton->SetDefaultColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
	m_pSettingsButton->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
	m_pSettingsButton->SetDepressedColor(Color(100, 100, 100, 255), Color(200, 200, 200, 255));
	m_pSettingsButton->SetCursor(dc_hand);
	m_pSettingsButton->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
	m_pSettingsButton->SetContentAlignment(Label::a_center);

	m_pSettingsPanel->SetBounds(0, 0, m_pSettingsPanel->GetParent()->GetWide(), m_pSettingsPanel->GetParent()->GetTall());

	for (int i = 0; i < 2; i++)
	{
		m_pFormations[i]->SetBounds(i * (m_pFormations[i]->GetParent()->GetWide() / 2), 0, m_pFormations[i]->GetParent()->GetWide() / 2, m_pFormations[i]->GetParent()->GetTall());
		//m_pFormations[i]->SetBgColor(Color(0, 0, 0, 240));

		for (int j = 0; j < 11; j++)
		{
			m_pFormationButtons[i][j]->SetBounds(0, 0, FORMATION_BUTTON_WIDTH, FORMATION_BUTTON_HEIGHT);
			m_pFormationButtons[i][j]->SetContentAlignment(Label::a_center);
			m_pFormationButtons[i][j]->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
			color32 enabled = { 255, 255, 255, 255 };
			color32 mouseover = { 150, 150, 150, 255 };
			color32 pressed = { 255, 255, 255, 255 };
			color32 disabled = { 75, 75, 75, 255 };
			Color black(0, 0, 0, 255);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", enabled);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, "vgui/shirt", mouseover);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_PRESSED, "vgui/shirt", pressed);
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_DISABLED, "vgui/shirt", disabled);
			m_pFormationButtons[i][j]->SetDefaultColor(black, black);
			m_pFormationButtons[i][j]->SetArmedColor(black, black);
			m_pFormationButtons[i][j]->SetDepressedColor(black, black);
			m_pFormationButtons[i][j]->SetDisabledFgColor1(Color(0, 0, 0, 0));
			m_pFormationButtons[i][j]->SetDisabledFgColor2(black);
			m_pFormationButtons[i][j]->SetPaintBorderEnabled(false);
		}

		m_pPlayerList[i]->SetBounds(i * (GetWide() / 2), 0, GetWide() / 2, GetTall() - EXTRAINFO_HEIGHT - SPECLIST_HEIGHT - SPECLIST_MARGIN);
		m_pPlayerList[i]->SetPaintBorderEnabled(false);

		m_pSideSeparators[i]->SetBgColor(Color(0, 0, 0, 150));
	}

	m_pSideSeparators[0]->SetBounds(GetWide() / 2 - SIDESEPARATOR_WIDTH / 2, SIDESEPARATOR_MARGIN, SIDESEPARATOR_WIDTH, m_pSpectatorContainer->GetY() - 2 * SIDESEPARATOR_MARGIN);
	int yPos = m_pSpectatorContainer->GetY() + m_pSpectatorContainer->GetTall() + SIDESEPARATOR_MARGIN;
	m_pSideSeparators[1]->SetBounds(GetWide() / 2 - SIDESEPARATOR_WIDTH / 2, yPos, SIDESEPARATOR_WIDTH, GetTall() - yPos - SIDESEPARATOR_MARGIN);

	SetPaintBorderEnabled(true);
	//SetBorder(NULL);
	SetBgColor(Color(0, 0, 0, 250));

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
	// Set the title

	// Reset();
	//for (int i = 0; i < 2; i++)
	//{
	//	m_pPlayerList[i]->DeleteAllItems();
	//}

	FillScoreBoard();

	if (!m_pSettingsPanel->IsVisible())
	{
		for (int i = 0; i < 2; i++)
		{
			int itemID = m_pPlayerList[i]->GetSelectedItem();

			if (itemID != -1)
			{
				IGameResources *gr = GameResources();
				if (!gr)
					break;

				int index = m_pPlayerList[i]->GetItemData(itemID)->GetInt("playerIndex");

				//KeyValues *playerData = new KeyValues("data");
				//GetPlayerScoreInfo(index, playerData);
				//playerData->deleteThis();

				wchar_t wszPlayerName[MAX_PLACE_NAME_LENGTH];
				g_pVGuiLocalize->ConvertANSIToUnicode(gr->GetPlayerName(index), wszPlayerName, sizeof(wszPlayerName));
				m_pPlayerStats->ModifyColumn(0, "NameColumn0", wszPlayerName);

				KeyValues *pData = new KeyValues("data");

				pData->SetString("NameColumn0", "Goals:");
				pData->SetInt("ValueColumn0", gr->GetGoals(index));
				pData->SetString("NameColumn1", "Assists:");
				pData->SetInt("ValueColumn1", gr->GetAssists(index));
				pData->SetString("NameColumn2", "Ping:");
				pData->SetInt("ValueColumn2", gr->GetPing(index));
				pData->SetString("NameColumn3", "Possession:");
				pData->SetInt("ValueColumn3", gr->GetPossession(index));

				m_pPlayerStats->ModifyItem(0, 0, pData);

				pData->SetString("NameColumn0", "Fouls:");
				pData->SetInt("ValueColumn0", gr->GetFouls(index));
				pData->SetString("NameColumn1", "Yellows:");
				pData->SetInt("ValueColumn1", gr->GetYellowCards(index));
				pData->SetString("NameColumn2", "Reds:");
				pData->SetInt("ValueColumn2", gr->GetRedCards(index));
				pData->SetString("NameColumn3", "Offsides:");
				pData->SetInt("ValueColumn3", gr->GetOffsides(index));

				m_pPlayerStats->ModifyItem(1, 0, pData);

				pData->SetString("NameColumn0", "Penalties:");
				pData->SetInt("ValueColumn0", gr->GetPenalties(index));	
				pData->SetString("NameColumn1", "Goal kicks:");
				pData->SetInt("ValueColumn1", gr->GetGoalKicks(index));
				pData->SetString("NameColumn2", "Free kicks:");
				pData->SetInt("ValueColumn2", gr->GetFreeKicks(index));
				pData->SetString("NameColumn3", "Corner kicks:");
				pData->SetInt("ValueColumn3", gr->GetCorners(index));

				m_pPlayerStats->ModifyItem(2, 0, pData);

				pData->deleteThis();
			}

			m_pPlayerStats->SetVisible(itemID != -1);
			m_pSideSeparators[1]->SetVisible(itemID == -1);

			for (int j = 0; j < 2; j++)
			{
				m_pFormations[j]->SetVisible(itemID == -1);
			}

			if (itemID != -1)
				break;
		}
	}

	MoveToCenterOfScreen();

	m_fNextUpdateTime = gpGlobals->curtime + 0.25f; 
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::UpdateTeamInfo()
{
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

				//UpdatePlayerAvatar( i, playerData );

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

			CBitmapButton *pButton = m_pFormationButtons[teamIndex][gr->GetTeamPosIndex(i)];

			if (gr->GetTeamToJoin(i) != TEAM_INVALID)
			{
				//pButton->SetText(VarArgs("%s [%d]", gr->GetPlayerName(i), max(0, (int)(gr->GetNextJoin(i) - gpGlobals->curtime))));
			}
			else
			{
				bool spotReserved = false;

				for (int j = 1; j <= gpGlobals->maxClients; j++)
				{
					if (j == i || !gr->IsConnected(j))
						continue;

					if (gr->GetTeamToJoin(j) == gr->GetTeam(i) && gr->GetTeamPosIndex(j) == gr->GetTeamPosIndex(i))
					{
						spotReserved = true;
						break;
					}
				}

				if (!spotReserved)
					;//pButton->SetText(gr->GetPlayerName(i));
			}

			//pButton->SetFgColor(gr->GetTeamColor(teamIndex));
			pButton->SetCursor(gr->IsFakePlayer(i) ? dc_hand : dc_arrow);
			pButton->SetEnabled(gr->IsFakePlayer(i));
			color32 disabled = { 75, 75, 75, 255 };
			pButton->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", disabled);
			//Color black(0, 0, 0, 255);
			//pButton->SetDefaultColor(black, black);
			//pButton->SetArmedColor(black, black);
			//pButton->SetDepressedColor(black, black);
			//pButton->SetDefaultColor(black, lighter);
			//pButton->SetArmedColor(black, white);
			//pButton->SetDepressedColor(black, lighter);
			//pButton->SetDisabledFgColor1(black);
			//pButton->SetDisabledFgColor2(black);
			//pButton->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
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

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			if (!IsValidPosition(j))
			{
				m_pFormationButtons[i][j]->SetVisible(false);
				continue;
			}

			m_pFormationButtons[i][j]->SetVisible(true);
			m_pFormationButtons[i][j]->SetText(g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][j][POS_NAME]]);

			float xDist = m_pFormations[i]->GetWide() / 5;
			float yDist = m_pFormations[i]->GetTall() / 5;
			float xPos = g_Positions[mp_maxplayers.GetInt() - 1][j][POS_XPOS] * xDist + xDist - m_pFormationButtons[i][j]->GetWide() / 2;
			float yPos = g_Positions[mp_maxplayers.GetInt() - 1][j][POS_YPOS] * yDist + yDist - m_pFormationButtons[i][j]->GetTall() / 2;
			m_pFormationButtons[i][j]->SetPos(xPos, yPos);

			if (isPosTaken[i][j])
				continue;

			//m_pFormationButtons[i][j]->SetText("JOIN");
			m_pFormationButtons[i][j]->SetCursor(dc_hand);
			m_pFormationButtons[i][j]->SetEnabled(true);
			color32 enabled = { 255, 255, 255, 255 };
			m_pFormationButtons[i][j]->SetImage(CBitmapButton::BUTTON_ENABLED, "vgui/shirt", enabled);
			//m_pFormationButtons[i][j]->SetFgColor(gr->GetTeamColor(TEAM_UNASSIGNED));
			//m_pFormationButtons[i][j]->SetDefaultColor(black, darker);
			//m_pFormationButtons[i][j]->SetArmedColor(black, dark);
			//m_pFormationButtons[i][j]->SetDepressedColor(black, darker);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds the top header of the scoreboars
//-----------------------------------------------------------------------------
void CClientScoreBoardDialog::AddHeader()
{
	m_iSectionId = 0; //make a blank one

	for (int i = 0; i < 2; i++)
	{
		m_pPlayerList[i]->AddSection(m_iSectionId, "", StaticPlayerSortFunc);
		m_pPlayerList[i]->SetSectionAlwaysVisible(m_iSectionId);
		m_pPlayerList[i]->SetFontSection(m_iSectionId, m_pScheme->GetFont("IOSTeamMenuSmall"));
		m_pPlayerList[i]->SetLineSpacing(30);
		m_pPlayerList[i]->SetSectionDividerColor(m_iSectionId, Color(255, 255, 255, 255));
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "posname",		"", 0, 50 );
		if ( ShowAvatars() )
		{
			m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "avatar",		"", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		}
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "country",		"", SectionedListPanel::COLUMN_IMAGE, 50 );
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "club",			"Club", 0,  50);
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "name",			"Name", 0, 200);
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "goals",			"Goals", 0, 50);
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "assists",		"Assists", 0, 50);
		m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "ping",			"Ping", 0, 50);
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "voice",		"Voice", SectionedListPanel::COLUMN_IMAGE, scheme()->GetProportionalScaledValue(VOICE_WIDTH) );
		//m_pPlayerList[i]->AddColumnToSection(m_iSectionId, "tracker", "#PlayerTracker", SectionedListPanel::COLUMN_IMAGE, scheme()->GetProportionalScaledValueEx( GetScheme(),FRIENDS_WIDTH) );
	}

		m_pPlayerStats->AddSection(0, "");
		m_pPlayerStats->SetSectionAlwaysVisible(0);
		m_pPlayerStats->SetFontSection(0, m_pScheme->GetFont("StatsPlayerName"));
		m_pPlayerStats->SetLineSpacing(30);
		m_pPlayerStats->SetSectionDividerColor(m_iSectionId, Color(255, 255, 255, 255));
		m_pPlayerStats->AddColumnToSection(0, "NameColumn0", "", 0, 125);
		m_pPlayerStats->AddColumnToSection(0, "ValueColumn0", "", 0, 75);
		m_pPlayerStats->AddColumnToSection(0, "NameColumn1", "", 0, 125);
		m_pPlayerStats->AddColumnToSection(0, "ValueColumn1", "", 0, 75);
		m_pPlayerStats->AddColumnToSection(0, "NameColumn2", "", 0, 125);
		m_pPlayerStats->AddColumnToSection(0, "ValueColumn2", "", 0, 75);
		m_pPlayerStats->AddColumnToSection(0, "NameColumn3", "", 0, 125);
		m_pPlayerStats->AddColumnToSection(0, "ValueColumn3", "", 0, 75);

		KeyValues *pData = new KeyValues("data");

		m_pPlayerStats->AddItem(0, pData);
		m_pPlayerStats->SetItemFont(0, m_pScheme->GetFont("IOSTeamMenuNormal"));
		m_pPlayerStats->AddItem(0, pData);
		m_pPlayerStats->SetItemFont(1, m_pScheme->GetFont("IOSTeamMenuNormal"));
		m_pPlayerStats->AddItem(0, pData);
		m_pPlayerStats->SetItemFont(2, m_pScheme->GetFont("IOSTeamMenuNormal"));

		pData->deleteThis();

		//KeyValues *pStats = new KeyValues("data");
		//pStats->

		//m_pPlayerStats->AddItem(0, pStats);

		//pStats->deleteThis();

		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "posname",		"Pos", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//if ( ShowAvatars() )
		//{
		//	m_pPlayerStats->AddColumnToSection(m_iSectionId, "avatar",		"Avatar", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//}
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "country",		"Country", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue(SMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "club",			"Club", 0, scheme()->GetProportionalScaledValue(SMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "name",			"Player", 0, scheme()->GetProportionalScaledValue(NAME_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "goals",		"Goals", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "assists",		"Assists", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "possession",	"%", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "redcards",		"Red", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "yellowcards",	"Yel", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "fouls",		"Foul", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "passes",		"Pass", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "freekicks",	"FKs", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "penalties",	"Pen", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "corners",		"Cnr", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "throws",		"Thr", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "saves",		"Sav", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "goalkicks",	"GKs", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "ping",			"Ping", 0, scheme()->GetProportionalScaledValue(VSMALL_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "voice",		"Voice", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue(VOICE_WIDTH) );
		//m_pPlayerStats->AddColumnToSection(m_iSectionId, "tracker", "#PlayerTracker", SectionedListPanel::COLUMN_IMAGE, scheme()->GetProportionalScaledValueEx( GetScheme(),FRIENDS_WIDTH) );
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
	kv->SetString("possession", GET_STAT_TEXT(gr->GetPossession(playerIndex)));
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
	kv->SetInt("playerIndex", playerIndex);
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

		for (int i = 0; i < 2; i++)
		{
			m_pFormations[i]->SetVisible(isVisible);
		}
	}
	else
		BaseClass::OnCommand(cmd);
}

void CClientScoreBoardDialog::OnItemSelected(KeyValues *data)
{
	if (!m_pSettingsPanel->IsVisible())
		Update();
}