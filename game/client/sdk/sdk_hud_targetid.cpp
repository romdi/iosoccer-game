//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_sdk_player.h"
#include "c_playerresource.h"
#include "vgui_EntityPanel.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "sdk_gamerules.h"
#include "c_ios_replaymanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PLAYER_HINT_DISTANCE	150
#define PLAYER_HINT_DISTANCE_SQ	(PLAYER_HINT_DISTANCE*PLAYER_HINT_DISTANCE)

ConVar hud_names_visible("hud_names_visible", "1");
ConVar hud_names_offset("hud_names_offset", "120");

void CC_HudNamesToggle(const CCommand &args)
{
	hud_names_visible.SetValue(!hud_names_visible.GetBool());
}

ConCommand hud_names_toggle("hud_names_toggle", CC_HudNamesToggle);

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSDKTargetId : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CSDKTargetId, vgui::Panel );

public:
	CSDKTargetId( const char *pElementName );
	void Init( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );
	void VidInit( void );

private:
	vgui::HFont		m_hFont;
};

DECLARE_HUDELEMENT( CSDKTargetId );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSDKTargetId::CSDKTargetId( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "TargetID" )
{
	SetHiddenBits(HIDEHUD_PLAYERNAMES);

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_hFont = g_hFontTrebuchet24;
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CSDKTargetId::Init( void )
{
};

void CSDKTargetId::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	//m_hFont = scheme->GetFont( "TargetID", IsProportional() );
	m_hFont = scheme->GetFont("IOSPlayerName");

	SetPaintBackgroundEnabled( false );
	SetBounds(0, 0, ScreenWidth(), ScreenHeight());
}

//-----------------------------------------------------------------------------
// Purpose: clear out string etc between levels
//-----------------------------------------------------------------------------
void CSDKTargetId::VidInit()
{
	CHudElement::VidInit();
}

//-----------------------------------------------------------------------------
// Purpose: Draw function for the element
//-----------------------------------------------------------------------------
void CSDKTargetId::Paint()
{
	if (hud_names_visible.GetInt() == 0)
		return;

	C_ReplayManager *pReplayManager = GetReplayManager();

	if (pReplayManager && pReplayManager->IsReplaying())
		return;

	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();

	if (!pLocal)
		return;

	for (int i = 0; i <= gpGlobals->maxClients; i++)
	{
		C_SDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!pPl || pPl == pLocal)
			continue;

		if (pPl->IsDormant())
			continue;

		const char *printFormatString = NULL;
		wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszPosName[10];
		wchar_t wszPlayerText[MAX_PLAYER_NAME_LENGTH + 10];

		g_pVGuiLocalize->ConvertANSIToUnicode(pPl->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName));
		//g_pVGuiLocalize->ConvertANSIToUnicode(g_szPosNames[GameResources()->GetTeamPosition(i) - 1], wszPosName, sizeof(wszPosName));

		//_snwprintf( wszPlayerText, ARRAYSIZE(wszPlayerText) - 1, L"%s %s", wszPosName, wszPlayerName);
		_snwprintf( wszPlayerText, ARRAYSIZE(wszPlayerText) - 1, L"%s", wszPlayerName);
		wszPlayerText[ ARRAYSIZE(wszPlayerText)-1 ] = '\0';

		int wide, tall;
		int xPos, yPos;
		int zOffset = 120;

		vgui::surface()->GetTextSize(m_hFont, wszPlayerText, wide, tall);

		Color c = GameResources()->GetTeamColor(pPl->GetTeamNumber());
		c = Color(c.r(), c.g(), c.b(), 255);

		Vector pos = pPl->GetLocalOrigin();
		pos.z += VEC_HULL_MAX.z + hud_names_offset.GetInt();
		GetVectorInScreenSpace(pos, xPos, yPos);

		//surface()->DrawSetColor(c);
		//surface()->DrawLine(xPos, yStartPos, xPos, yEndPos);
		surface()->DrawSetTextFont(m_hFont);
		surface()->DrawSetTextPos(xPos - wide / 2, yPos - tall);
		surface()->DrawSetTextColor(c);
		surface()->DrawPrintText(wszPlayerText, wcslen(wszPlayerText));
	}
}
