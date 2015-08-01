//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>

#include "statsmenu.h"

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

#include "c_match_ball.h"
#include "c_team.h"
#include "sdk_backgroundpanel.h"
#include "sdk_gamerules.h"
#include "c_sdk_player.h"
#include "steam/steam_api.h"
#include "clientscoreboarddialog.h"

#include "materialsystem/itexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CStatsMenu::CStatsMenu(Panel *parent, const char *name) : Panel(parent, name)
{
	for (int i = 0; i < 3; i++)
	{
		m_pPlayerStats[i] = new SectionedListPanel(this, "");
		m_pPlayerStats[i]->SetVerticalScrollbar(false);
	}

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CStatsMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScheme = pScheme;
}

void CStatsMenu::Reset()
{
	for (int i = 0; i < 3; i++)
	{
		m_pPlayerStats[i]->RemoveAll();
		m_pPlayerStats[i]->RemoveAllSections();

		int sectionId = 0;

		m_pPlayerStats[i]->AddSection(sectionId, "");
		m_pPlayerStats[i]->SetSectionAlwaysVisible(sectionId);
		m_pPlayerStats[i]->SetFontSection(sectionId, m_pScheme->GetFont("ScoreboardStatLine"));
		m_pPlayerStats[i]->SetLineSpacing(25);
		m_pPlayerStats[i]->SetFgColor(Color(255, 255, 255, 255));
		m_pPlayerStats[i]->SetSectionFgColor(0, Color(255, 255, 255, 255));

		int defaultFlags = SectionedListPanel::HEADER_CENTER | SectionedListPanel::COLUMN_CENTER;

		if (i == 0)
		{
			int columnWidth = GetWide() / 11;

			m_pPlayerStats[i]->AddColumnToSection(sectionId, "posname",					"Pos.",			defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "name",					"Name",			0,				3 * columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "steamname",				"Steam name",	0,				3 * columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "countryname",				"Location",		0,				2 * columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "club",					"Club",			defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "nationalteam",			"National team",defaultFlags,	columnWidth);
		}
		else if (i == 1)
		{
			int columnWidth = GetWide() / 11;

			m_pPlayerStats[i]->AddColumnToSection(sectionId, "rating",					"Rating",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "ping",					"Ping",			defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "goals",					"Goals",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "assists",					"Assists",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "possession",				"Poss.",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "distancecovered",			"Distance",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "passes",					"Passes",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "passescompleted",			"~ compl.",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "interceptions",			"Interc.",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "slidingtackles",			"Tackles",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "slidingtacklescompleted",	"~ compl.",		defaultFlags,	columnWidth);
		}
		else
		{
			int columnWidth = GetWide() / 12;

			m_pPlayerStats[i]->AddColumnToSection(sectionId, "fouls",					"Fouls",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "foulssuffered",			"~ suffered",	defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "yellowcards",				"Yellows",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "redcards",				"Reds",			defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "corners",					"Corners",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "keepersaves",				"Saves",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "keepersavescaught",		"Saves",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "goalsconceded",			"Goals conc.",	defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "owngoals",				"Own goals",	defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "shots",					"Shots",		defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "shotsongoal",				"~ on goal",	defaultFlags,	columnWidth);
			m_pPlayerStats[i]->AddColumnToSection(sectionId, "offsides",				"Offsides",		defaultFlags,	columnWidth);
		}

		KeyValues *kv = new KeyValues("data");
		////kv->SetInt("playerindex", i - 1);
		m_pPlayerStats[i]->AddItem(0, kv);
		kv->deleteThis();
		m_pPlayerStats[i]->SetItemFont(0, m_pScheme->GetFont("ScoreboardInfo"));
	}
}

void CStatsMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	for (int i = 0; i < 3; i++)
	{
		m_pPlayerStats[i]->SetBounds(0, i * (GetTall() / 3), GetWide(), GetTall() / 3);
		m_pPlayerStats[i]->SetPaintBorderEnabled(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CStatsMenu::~CStatsMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: updates the UI with a new map name and map html page, and sets up the team buttons
//-----------------------------------------------------------------------------
void CStatsMenu::OnThink()
{
	if (m_flNextUpdateTime > gpGlobals->curtime)
		return;

	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
}

void CStatsMenu::Update(int playerIndex, KeyValues *kv)
{
	IGameResources *gr = GameResources();
	if (!gr)
		return;

	bool isPlayer = playerIndex > 0;

	C_Team *pTeam = GetGlobalTeam(isPlayer ? gr->GetTeam(playerIndex) : playerIndex + 2 + TEAM_HOME);

	Reset();

	for (int i = 0; i < 3; i++)
	{
		m_pPlayerStats[i]->SetFgColor(g_ColorGray);
		m_pPlayerStats[i]->SetSectionFgColor(0, g_ColorGray);
		//m_pPlayerStats[i]->SetVisible(true);
		//m_pPlayerStats[i]->Repaint();
		//m_pPlayerStats[i]->SetSectionDividerColor(0, Color(255, 255, 255, 255));

		//if (m_pPlayerStats[i]->GetFgColor() != pTeam->Get_HudKitColor())

		m_pPlayerStats[i]->ModifyItem(0, 0, kv);
	}
}

//-----------------------------------------------------------------------------
// IOS Added
//-----------------------------------------------------------------------------
void CStatsMenu::OnCommand( char const *cmd )
{
	BaseClass::OnCommand(cmd);

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CStatsMenu::OnTextChanged(KeyValues *data)
{
}

void CStatsMenu::UpdatePlayerAvatar(int playerIndex, KeyValues *kv)
{
	//// Update their avatar
	//if ( kv && ShowAvatars() && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
	//{
	//	player_info_t pi;
	//	if ( engine->GetPlayerInfo( playerIndex, &pi ) )
	//	{
	//		if ( pi.friendsID )
	//		{
	//			CSteamID steamIDForPlayer( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );

	//			// See if the avatar's changed
	//			int iAvatar = steamapicontext->SteamFriends()->GetFriendAvatar( steamIDForPlayer );
	//			if ( m_iImageAvatars[playerIndex] != iAvatar )
	//			{
	//				m_iImageAvatars[playerIndex] = iAvatar;

	//				// Now see if we already have that avatar in our list
	//				int iIndex = m_mapAvatarsToImageList.Find( iAvatar );
	//				if ( iIndex == m_mapAvatarsToImageList.InvalidIndex() )
	//				{
	//					CAvatarImage *pImage = new CAvatarImage();
	//					pImage->SetAvatarSteamID( steamIDForPlayer );
	//					pImage->SetAvatarSize( 32, 32 );	// Deliberately non scaling
	//					int iImageIndex = m_pImageList->AddImage( pImage );

	//					m_mapAvatarsToImageList.Insert( iAvatar, iImageIndex );
	//				}
	//			}

	//			int iIndex = m_mapAvatarsToImageList.Find( iAvatar );

	//			if ( iIndex != m_mapAvatarsToImageList.InvalidIndex() )
	//			{
	//				kv->SetInt( "avatar", m_mapAvatarsToImageList[iIndex] );

	//				CAvatarImage *pAvIm = (CAvatarImage *)m_pImageList->GetImage( m_mapAvatarsToImageList[iIndex] );
	//				pAvIm->UpdateFriendStatus();
	//			}
	//		}
	//	}
	//}
}