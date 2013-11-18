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
#include "voice_status.h"
#include "view.h"
#include "c_team.h"
#include "clientmode_shared.h"
#include "ios_emote_menu.h"

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

struct Emote
{
	int emoteId;
	float startTime;
};

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
	void FireGameEvent(IGameEvent *event);

private:
	vgui::HFont		m_hFont;
	Emote		m_Emotes[11];
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

	for (int i = 0; i < 11; i++)
	{
		m_Emotes[i].emoteId = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CSDKTargetId::Init( void )
{
	ListenForGameEvent("emote");
}

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

void DrawPlayerName(HFont font, const Vector &origin, const char *playerName, int teamNumber)
{
	wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
	g_pVGuiLocalize->ConvertANSIToUnicode(playerName, wszPlayerName, sizeof(wszPlayerName));

	int wide, tall;
	vgui::surface()->GetTextSize(font, wszPlayerName, wide, tall);

	Color c = GetGlobalTeam(teamNumber)->GetHudKitColor();

	Vector pos = origin;
	pos.z += VEC_HULL_MAX.z + hud_names_offset.GetInt();

	int xPos, yPos;
	GetVectorInScreenSpace(pos, xPos, yPos);

	surface()->DrawSetTextFont(font);
	surface()->DrawSetTextColor(c);
	surface()->DrawSetTextPos(xPos - wide / 2, yPos - tall);
	surface()->DrawPrintText(wszPlayerName, wcslen(wszPlayerName));
}

void DrawPlayerEmote(HFont font, C_SDKPlayer *pPl, Emote &emote)
{
	const float maxEmoteDuration = mp_emote_duration.GetFloat();
	float emoteDuration = gpGlobals->curtime - emote.startTime;

	if (emoteDuration > maxEmoteDuration)
	{
		emote.emoteId = -1;
		return;
	}

	Vector pos = pPl->GetLocalOrigin();
	pos.z += VEC_HULL_MAX.z;

	int xPos, yPos;
	bool isOnscreen = GetVectorInScreenSpace(pos, xPos, yPos);

	wchar_t wszText[32];
	g_pVGuiLocalize->ConvertANSIToUnicode(g_szEmotes[emote.emoteId], wszText, sizeof(wszText));

	int wide, tall;
	vgui::surface()->GetTextSize(font, wszText, wide, tall);

	float coeff = emoteDuration / maxEmoteDuration;

	Color fade = Color(255, 255, 255, 255 * (1 - coeff));

	surface()->DrawSetTextFont(font);
	surface()->DrawSetTextColor(fade);
	surface()->DrawSetTextPos(xPos - wide / 2, yPos - tall - coeff * 4 * tall);
	surface()->DrawPrintText(wszText, wcslen(wszText));
}

//-----------------------------------------------------------------------------
// Purpose: Draw function for the element
//-----------------------------------------------------------------------------
void CSDKTargetId::Paint()
{
	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();

	if (!pLocal)
		return;

	if (GetReplayManager() && GetReplayManager()->IsReplaying())
	{		
		for (int i = gpGlobals->maxClients; i <= ClientEntityList().GetHighestEntityIndex(); i++)
		{
			C_ReplayPlayer *pPl = dynamic_cast<C_ReplayPlayer *>(ClientEntityList().GetBaseEntity(i));
			if (!pPl)
				continue;

			if (hud_names_visible.GetBool())
				DrawPlayerName(m_hFont, pPl->GetLocalOrigin(), pPl->m_szPlayerName, pPl->m_nTeamNumber);
		}
	}
	else
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			C_SDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
			if (!pPl || pPl->IsDormant())
				continue;

			if (pPl != pLocal && hud_names_visible.GetBool())
				DrawPlayerName(m_hFont, pPl->GetLocalOrigin(), pPl->GetPlayerName(), pPl->GetTeamNumber());

			if (pPl->GetTeamNumber() == pLocal->GetTeamNumber())
			{
				int teamPosIndex = GameResources()->GetTeamPosIndex(pPl->index);
				if (m_Emotes[teamPosIndex].emoteId != -1)
				{
					DrawPlayerEmote(m_hFont, pPl, m_Emotes[teamPosIndex]);
				}
			}
		}
	}
}

void CSDKTargetId::FireGameEvent(IGameEvent *event)
{
	if (!Q_strcmp(event->GetName(), "emote"))
	{
		C_SDKPlayer *pPl = ToSDKPlayer(USERID2PLAYER(event->GetInt("userid")));

		if (C_SDKPlayer::GetLocalPlayer() && pPl && pPl->GetTeamNumber() == C_SDKPlayer::GetLocalPlayer()->GetTeamNumber())
		{
			int emoteId = event->GetInt("emoteid");

			if (emoteId >= 0 && emoteId < EMOTE_COUNT)
			{
				int teamPosIndex = GameResources()->GetTeamPosIndex(pPl->index);
				m_Emotes[teamPosIndex].emoteId = emoteId;
				m_Emotes[teamPosIndex].startTime = gpGlobals->curtime;
			}
		}
	}
}