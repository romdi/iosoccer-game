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
	m_pPlayerStats->SetFgColor(Color(0, 0, 0, 255));
	m_pPlayerStats->SetSectionFgColor(0, Color(0, 0, 0, 255));
	const int nameWidth = 140;
	const int valueWidth = 105;
	//m_pPlayerStats[side]->SetSectionDividerColor(0, Color(255, 255, 255, 255));
	for (int j = 0; j < 4; j++)
	{
		m_pPlayerStats->AddColumnToSection(0, VarArgs("NameColumn%d", j), "", 0/*SectionedListPanel::COLUMN_RIGHT*/, nameWidth);
		m_pPlayerStats->AddColumnToSection(0, VarArgs("ValueColumn%d", j), "", j == 4 ? SectionedListPanel::HEADER_IMAGE : 0, valueWidth);
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

void CStatsMenu::Update(int playerIndex)
{
	IGameResources *gr = GameResources();
	if (!gr)
		return;

	C_Team *pTeam = GetGlobalTeam(gr->GetTeam(playerIndex));

	Reset();

	m_pPlayerStats->SetFgColor(pTeam->Get_HudKitColor());
	m_pPlayerStats->SetSectionFgColor(0, pTeam->Get_HudKitColor());
	m_pPlayerStats->SetVisible(true);
	//m_pPlayerStats->Repaint();
	//m_pPlayerStats->SetSectionDividerColor(0, Color(255, 255, 255, 255));

	//if (m_pPlayerStats->GetFgColor() != pTeam->Get_HudKitColor())

	KeyValues *pData = new KeyValues("data");

	wchar_t wszText[64];
	g_pVGuiLocalize->ConvertANSIToUnicode(gr->GetPlayerName(playerIndex), wszText, sizeof(wszText));
	m_pPlayerStats->ModifyColumn(0, "NameColumn0", wszText);

	_swprintf(wszText, L"");

	if (steamapicontext->SteamFriends() && steamapicontext->SteamUtils())
	{
		player_info_t pi;
		if (engine->GetPlayerInfo(playerIndex, &pi))
		{
			if (pi.friendsID)
			{
				CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
				g_pVGuiLocalize->ConvertANSIToUnicode(steamapicontext->SteamFriends()->GetFriendPersonaName(steamIDForPlayer), wszText, sizeof(wszText));
			}
		}
	}

	m_pPlayerStats->ModifyColumn(0, "NameColumn1", wszText);

	//g_pVGuiLocalize->ConvertANSIToUnicode(g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][gr->GetTeamPosIndex(playerIndex)][POS_TYPE]], wszText, sizeof(wszText));
	//m_pPlayerStats->ModifyColumn(0, "NameColumn2", wszText);
	g_pVGuiLocalize->ConvertANSIToUnicode(gr->GetCountryName(playerIndex), wszText, sizeof(wszText));
	m_pPlayerStats->ModifyColumn(0, "NameColumn2", wszText);

	//UpdatePlayerAvatar(playerIndex, pData);

	//m_pPlayerStats->ModifyColumn(0, "NameColumn3, "

	pData->SetString("NameColumn0", "Goals:");
	pData->SetInt("ValueColumn0", gr->GetGoals(playerIndex));
	pData->SetString("NameColumn1", "Assists:");
	pData->SetInt("ValueColumn1", gr->GetAssists(playerIndex));
	pData->SetString("NameColumn2", "Ping:");
	pData->SetInt("ValueColumn2", gr->GetPing(playerIndex));
	pData->SetString("NameColumn3", "Fouls suffered:");
	pData->SetInt("ValueColumn3", gr->GetFoulsSuffered(playerIndex));

	m_pPlayerStats->ModifyItem(0, 0, pData);
	m_pPlayerStats->SetItemFgColor(0, pTeam->Get_HudKitColor());
	pData->Clear();

	pData->SetString("NameColumn0", "Fouls:");
	pData->SetInt("ValueColumn0", gr->GetFouls(playerIndex));
	pData->SetString("NameColumn1", "Yellows:");
	pData->SetInt("ValueColumn1", gr->GetYellowCards(playerIndex));
	pData->SetString("NameColumn2", "Reds:");
	pData->SetInt("ValueColumn2", gr->GetRedCards(playerIndex));
	pData->SetString("NameColumn3", "Goals conceded:");
	pData->SetInt("ValueColumn3", gr->GetGoalsConceded(playerIndex));

	m_pPlayerStats->ModifyItem(1, 0, pData);
	m_pPlayerStats->SetItemFgColor(1, pTeam->Get_HudKitColor());
	pData->Clear();

	pData->SetString("NameColumn0", "Penalties:");
	pData->SetInt("ValueColumn0", gr->GetPenalties(playerIndex));	
	pData->SetString("NameColumn1", "Goal kicks:");
	pData->SetInt("ValueColumn1", gr->GetGoalKicks(playerIndex));
	pData->SetString("NameColumn2", "Free kicks:");
	pData->SetInt("ValueColumn2", gr->GetFreeKicks(playerIndex));
	pData->SetString("NameColumn3", "Passes:");
	pData->SetInt("ValueColumn3", gr->GetPasses(playerIndex));

	m_pPlayerStats->ModifyItem(2, 0, pData);
	m_pPlayerStats->SetItemFgColor(2, pTeam->Get_HudKitColor());
	pData->Clear();

	pData->SetString("NameColumn0", "Distance:");
	pData->SetString("ValueColumn0", VarArgs("%0.1f km", gr->GetDistanceCovered(playerIndex) / 1000.0f));
	pData->SetString("NameColumn1", "Possession:");
	pData->SetString("ValueColumn1", VarArgs("%d%%", gr->GetPossession(playerIndex)));
	pData->SetString("NameColumn2", "Offsides:");
	pData->SetInt("ValueColumn2", gr->GetOffsides(playerIndex));
	pData->SetString("NameColumn3", "Passes completed:");
	pData->SetString("ValueColumn3", VarArgs("%d%%", gr->GetPassesCompleted(playerIndex) * 100 / max(1, gr->GetPasses(playerIndex))));

	m_pPlayerStats->ModifyItem(3, 0, pData);
	m_pPlayerStats->SetItemFgColor(3, pTeam->Get_HudKitColor());
	pData->Clear();

	pData->SetString("NameColumn0", "Corners:");
	pData->SetInt("ValueColumn0", gr->GetCorners(playerIndex));	
	pData->SetString("NameColumn1", "Own goals:");
	pData->SetInt("ValueColumn1", gr->GetOwnGoals(playerIndex));
	pData->SetString("NameColumn2", "Saves:");
	pData->SetInt("ValueColumn2", gr->GetKeeperSaves(playerIndex));
	pData->SetString("NameColumn3", "Shots:");
	pData->SetInt("ValueColumn3", gr->GetShots(playerIndex));

	m_pPlayerStats->ModifyItem(4, 0, pData);
	m_pPlayerStats->SetItemFgColor(4, pTeam->Get_HudKitColor());
	pData->Clear();

	pData->SetString("NameColumn0", "Shots on goal:");
	pData->SetInt("ValueColumn0", gr->GetShotsOnGoal(playerIndex));
	pData->SetString("NameColumn1", "Interceptions:");
	pData->SetInt("ValueColumn1", gr->GetInterceptions(playerIndex));

	m_pPlayerStats->ModifyItem(5, 0, pData);
	m_pPlayerStats->SetItemFgColor(5, pTeam->Get_HudKitColor());
	pData->Clear();

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