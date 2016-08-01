//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "sdk_hud_stamina.h"
#include "hud_macros.h"
#include "c_sdk_player.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <igameresources.h>
#include "c_baseplayer.h"
#include "in_buttons.h"
#include "sdk_gamerules.h"
#include "c_ios_replaymanager.h"
#include "c_match_ball.h"
#include "view.h"
#include "ios_camera.h"
#include "iinput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum { STAMINABAR_VMARGIN = 30, STAMINABAR_WIDTH = 200, STAMINABAR_HEIGHT = 6, STAMINABAR_BORDER = 2 };
enum { SHOTBAR_VMARGIN = 44, SHOTBAR_WIDTH = 200, SHOTBAR_HEIGHT = 6, SHOTBAR_BORDER = 2 };

class CHudChargedshotBar : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudChargedshotBar, vgui::Panel );

public:
	CHudChargedshotBar( const char *pElementName );
	virtual void	Init( void );
	virtual void	Reset( void );
	virtual void	OnThink( void );
	bool			ShouldDraw( void );

protected:
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint();

	float m_flNextUpdate;
};

DECLARE_HUDELEMENT( CHudChargedshotBar );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudChargedshotBar::CHudChargedshotBar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudChargedshotBar" )
{
	SetHiddenBits(HIDEHUD_POWERSHOTBAR);

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_flNextUpdate = gpGlobals->curtime;
}

void CHudChargedshotBar::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings(scheme);

	SetBounds(0, 0, ScreenWidth(), ScreenHeight());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudChargedshotBar::Init( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudChargedshotBar::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudChargedshotBar::ShouldDraw()
{
	if (!CHudElement::ShouldDraw())
		return false;

	C_SDKPlayer *pPlayer = C_SDKPlayer::GetLocalSDKPlayer();
	if (!pPlayer || pPlayer->GetTeamNumber() != TEAM_HOME && pPlayer->GetTeamNumber() != TEAM_AWAY)
		return false;

	if (GetMatchBall() && GetMatchBall()->m_eBallState == BALL_STATE_GOAL)
		return false;

	if (GetReplayManager() && GetReplayManager()->IsReplaying())
		return false;

	if (SDKGameRules()->IsCeremony())
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudChargedshotBar::OnThink( void )
{
	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: draws the stamina bar
//-----------------------------------------------------------------------------
void CHudChargedshotBar::Paint()
{
	C_SDKPlayer *pPlayer = C_SDKPlayer::GetLocalSDKPlayer();

	if ( !pPlayer )
		return;

	if (!SDKGameRules())
		return;

	float stamina = pPlayer->m_Shared.GetStamina() / 100.0f;
	
	Color staminaFgColor = g_ColorOrange;
	Color staminaBorderColor = Color(0, 0, 0, 255);
	Color staminaBgColor = Color(33, 33, 33, 255);

	// Draw shot bar border
	surface()->DrawSetColor(staminaBorderColor);
	surface()->DrawFilledRect(
		ScreenWidth() / 2 - STAMINABAR_WIDTH / 2 - STAMINABAR_BORDER,
		ScreenHeight() - STAMINABAR_VMARGIN - STAMINABAR_HEIGHT - 2 * STAMINABAR_BORDER,
		ScreenWidth() / 2 - STAMINABAR_WIDTH / 2 - STAMINABAR_BORDER + STAMINABAR_WIDTH + 2 * STAMINABAR_BORDER,
		ScreenHeight() - STAMINABAR_VMARGIN - STAMINABAR_HEIGHT - 2 * STAMINABAR_BORDER + STAMINABAR_HEIGHT + 2 * STAMINABAR_BORDER
	);

	// Draw shot bar background
	surface()->DrawSetColor(staminaBgColor);
	surface()->DrawFilledRect(
		ScreenWidth() / 2 - STAMINABAR_WIDTH / 2,
		ScreenHeight() - STAMINABAR_VMARGIN - STAMINABAR_HEIGHT - STAMINABAR_BORDER,
		ScreenWidth() / 2 + STAMINABAR_WIDTH / 2,
		ScreenHeight() - STAMINABAR_VMARGIN - STAMINABAR_HEIGHT - STAMINABAR_BORDER + STAMINABAR_HEIGHT
	);

	// Draw shot bar front
	surface()->DrawSetColor(staminaFgColor);
	surface()->DrawFilledRect(
		ScreenWidth() / 2 - STAMINABAR_WIDTH / 2,
		ScreenHeight() - STAMINABAR_VMARGIN - STAMINABAR_HEIGHT - STAMINABAR_BORDER,
		ScreenWidth() / 2 - STAMINABAR_WIDTH / 2 + stamina * STAMINABAR_WIDTH,
		ScreenHeight() - STAMINABAR_VMARGIN - STAMINABAR_HEIGHT - STAMINABAR_BORDER + STAMINABAR_HEIGHT
	);

	surface()->DrawSetColor(staminaBorderColor);

	for (int i = 1; i <= 3; i++)
	{
		surface()->DrawFilledRect(
			ScreenWidth() / 2 - STAMINABAR_WIDTH / 2 + i * 0.25f * STAMINABAR_WIDTH - 1,
			ScreenHeight() - STAMINABAR_VMARGIN - STAMINABAR_HEIGHT - STAMINABAR_BORDER,
			ScreenWidth() / 2 - STAMINABAR_WIDTH / 2 + i * 0.25f * STAMINABAR_WIDTH + 1,
			ScreenHeight() - STAMINABAR_VMARGIN - STAMINABAR_HEIGHT - STAMINABAR_BORDER + STAMINABAR_HEIGHT
		);
	}

	Color shotFgColor;
	Color shotBorderColor = Color(0, 0, 0, 255);
	Color shotBgColor = Color(33, 33, 33, 255);
	bool shotBlocked = true;

	if (pPlayer->GetFlags() & FL_REMOTECONTROLLED)
		shotFgColor = g_ColorBrown;
	else if (pPlayer->m_bShotsBlocked)
		shotFgColor = g_ColorRed;
	else if (pPlayer->m_bChargedshotBlocked)
		shotFgColor = g_ColorOrange;
	else
	{
		shotFgColor = g_ColorLightBlue;
		shotBlocked = false;
	}

	float shotStrength = shotBlocked ? 1.0f : pPlayer->GetChargedShotStrength();

	// Draw shot bar border
	surface()->DrawSetColor(shotBorderColor);
	surface()->DrawFilledRect(
		ScreenWidth() / 2 - SHOTBAR_WIDTH / 2 - SHOTBAR_BORDER,
		ScreenHeight() - SHOTBAR_VMARGIN - SHOTBAR_HEIGHT - 2 * SHOTBAR_BORDER,
		ScreenWidth() / 2 - SHOTBAR_WIDTH / 2 - SHOTBAR_BORDER + SHOTBAR_WIDTH + 2 * SHOTBAR_BORDER,
		ScreenHeight() - SHOTBAR_VMARGIN - SHOTBAR_HEIGHT - 2 * SHOTBAR_BORDER + SHOTBAR_HEIGHT + 2 * SHOTBAR_BORDER
	);

	// Draw shot bar background
	surface()->DrawSetColor(shotBgColor);
	surface()->DrawFilledRect(
		ScreenWidth() / 2 - SHOTBAR_WIDTH / 2,
		ScreenHeight() - SHOTBAR_VMARGIN - SHOTBAR_HEIGHT - SHOTBAR_BORDER,
		ScreenWidth() / 2 + SHOTBAR_WIDTH / 2,
		ScreenHeight() - SHOTBAR_VMARGIN - SHOTBAR_HEIGHT - SHOTBAR_BORDER + SHOTBAR_HEIGHT
	);

	// Draw shot bar front
	surface()->DrawSetColor(shotFgColor);
	surface()->DrawFilledRect(
		ScreenWidth() / 2 - SHOTBAR_WIDTH / 2,
		ScreenHeight() - SHOTBAR_VMARGIN - SHOTBAR_HEIGHT - SHOTBAR_BORDER,
		ScreenWidth() / 2 - SHOTBAR_WIDTH / 2 + shotStrength * SHOTBAR_WIDTH,
		ScreenHeight() - SHOTBAR_VMARGIN - SHOTBAR_HEIGHT - SHOTBAR_BORDER + SHOTBAR_HEIGHT
	);

	surface()->DrawSetColor(shotBorderColor);

	for (int i = 1; i <= 3; i++)
	{
		surface()->DrawFilledRect(
			ScreenWidth() / 2 - SHOTBAR_WIDTH / 2 + i * 0.25f * SHOTBAR_WIDTH - 1,
			ScreenHeight() - SHOTBAR_VMARGIN - SHOTBAR_HEIGHT - SHOTBAR_BORDER,
			ScreenWidth() / 2 - SHOTBAR_WIDTH / 2 + i * 0.25f * SHOTBAR_WIDTH + 1,
			ScreenHeight() - SHOTBAR_VMARGIN - SHOTBAR_HEIGHT - SHOTBAR_BORDER + SHOTBAR_HEIGHT
		);
	}
}