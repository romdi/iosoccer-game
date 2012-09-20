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

#include "c_ball.h"
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
	m_pPlayerStats = new SectionedListPanel(this, "");
	m_pPlayerStats->SetVerticalScrollbar(false);

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CStatsMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScheme = pScheme;
}

void CStatsMenu::Reset()
{
	//m_pPlayerStats->RemoveAll();
	m_pPlayerStats->DeleteAllItems();
	m_pPlayerStats->RemoveAllSections();

	m_pPlayerStats->AddSection(0, "");
	m_pPlayerStats->SetSectionAlwaysVisible(0);
	m_pPlayerStats->SetFontSection(0, m_pScheme->GetFont("StatsPlayerName"));
	m_pPlayerStats->SetLineSpacing(30);
	m_pPlayerStats->SetFgColor(Color(255, 255, 255, 255));
	m_pPlayerStats->SetSectionFgColor(0, Color(255, 255, 255, 255));
	const int nameWidth = 145;
	const int valueWidth = 110;
	//m_pPlayerStats[side]->SetSectionDividerColor(0, Color(255, 255, 255, 255));
	for (int j = 0; j < 4; j++)
	{
		m_pPlayerStats->AddColumnToSection(0, VarArgs("NameColumn%d", j), "", 0, nameWidth);
		m_pPlayerStats->AddColumnToSection(0, VarArgs("ValueColumn%d", j), "", 0, valueWidth);
	}

	HFont font = m_pScheme->GetFont("StatsPlayerNameSmall");
	KeyValues *pData = new KeyValues("data");
	for (int j = 0; j < 6; j++)
	{
		m_pPlayerStats->AddItem(j, pData);
		m_pPlayerStats->SetItemFont(j, font);
	}
	pData->deleteThis();
}

void CStatsMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pPlayerStats->SetBounds(0, 0, GetWide(), GetTall());
	m_pPlayerStats->SetPaintBorderEnabled(false);
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

	C_Team *pTeam = GetGlobalTeam(isPlayer ? gr->GetTeam(playerIndex) : playerIndex + 1 + TEAM_A);

	Reset();

	m_pPlayerStats->SetFgColor(pTeam->Get_HudKitColor());
	m_pPlayerStats->SetSectionFgColor(0, pTeam->Get_HudKitColor());
	m_pPlayerStats->SetVisible(true);
	//m_pPlayerStats->Repaint();
	//m_pPlayerStats->SetSectionDividerColor(0, Color(255, 255, 255, 255));

	//if (m_pPlayerStats->GetFgColor() != pTeam->Get_HudKitColor())

	KeyValues *pData = new KeyValues("data");

	wchar_t wszText[64];
	g_pVGuiLocalize->ConvertANSIToUnicode(isPlayer ? VarArgs("%s", gr->GetPlayerName(playerIndex)) : pTeam->Get_ShortTeamName(), wszText, sizeof(wszText));
	m_pPlayerStats->ModifyColumn(0, "NameColumn0", wszText);
	g_pVGuiLocalize->ConvertANSIToUnicode(isPlayer ? VarArgs("%s", gr->GetSteamName(playerIndex)) : "", wszText, sizeof(wszText));
	m_pPlayerStats->ModifyColumn(0, "NameColumn1", wszText);

	//g_pVGuiLocalize->ConvertANSIToUnicode(g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][gr->GetTeamPosIndex(playerIndex)][POS_TYPE]], wszText, sizeof(wszText));
	//m_pPlayerStats->ModifyColumn(0, "NameColumn2", wszText);
	g_pVGuiLocalize->ConvertANSIToUnicode(isPlayer ? VarArgs("%s", g_szCountryNames[gr->GetCountryName(playerIndex)]) : "", wszText, sizeof(wszText));
	m_pPlayerStats->ModifyColumn(0, "NameColumn2", wszText);

	g_pVGuiLocalize->ConvertANSIToUnicode(isPlayer ? VarArgs("%s", gr->GetClubName(playerIndex)) : "", wszText, sizeof(wszText));
	m_pPlayerStats->ModifyColumn(0, "NameColumn3", wszText);

	//UpdatePlayerAvatar(playerIndex, pData);

	//m_pPlayerStats->ModifyColumn(0, "NameColumn3, "

	for (int i = 0; i < STAT_COUNT; i++)
	{
		char statName[32];
		Q_snprintf(statName, sizeof(statName), "%s:", g_szStatNames[i]);
		pData->SetString(VarArgs("NameColumn%d", i % 4), statName);
		pData->SetString(VarArgs("ValueColumn%d", i % 4), kv->GetString(g_szStatIdentifiers[i]));

		if ((i % 4 == 3 || i == STAT_COUNT - 1))
		{
			m_pPlayerStats->ModifyItem(i / 4, 0, pData);
			//m_pPlayerStats->SetItemFgColor(i / 4, pTeam->Get_HudKitColor());
			pData->Clear();
		}
	}

	pData->deleteThis();
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