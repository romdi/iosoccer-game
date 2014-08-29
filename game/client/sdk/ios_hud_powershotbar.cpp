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

extern ConVar centeredstaminabar;

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

enum { BAR_WIDTH = 80, BAR_HEIGHT = 14, BAR_HPADDING = 2, BAR_VPADDING = 6, SHOTBAR_HEIGHT = 6 };
enum { INDICATOR_WIDTH = 9, INDICATOR_OFFSET = 7, INDICATOR_BORDER = 1 };

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
	if (!pPlayer || pPlayer->GetTeamNumber() != TEAM_A && pPlayer->GetTeamNumber() != TEAM_B)
		return false;

	if (GetMatchBall() && GetMatchBall()->m_eBallState == BALL_STATE_GOAL)
		return false;

	if (GetReplayManager() && GetReplayManager()->IsReplaying())
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

	float stamina = pPlayer->m_Shared.GetStamina() / 100.0f;
	float shotStrength = pPlayer->m_Shared.m_bDoChargedShot || pPlayer->m_Shared.m_bIsShotCharging ? pPlayer->GetChargedShotStrength() : 0;

	Color staminaFgColor, staminaBgColor, shotFgColor;

	if (pPlayer->GetFlags() & FL_REMOTECONTROLLED)
		staminaBgColor = Color(100, 100, 100, 255);
	else if (GetMatchBall() && GetMatchBall()->m_bShotsBlocked)
		staminaBgColor = Color(139, 0, 0, 255);
	else if (GetMatchBall() && GetMatchBall()->m_bChargedshotBlocked)
		staminaBgColor = Color(255, 69, 0, 255);
	else
		staminaBgColor = Color(0, 0, 0, 255);

	staminaFgColor = Color(255 * (1 - stamina), 255 * stamina, 0, 255);
	staminaBgColor = Color(0, 0, 0, 255);

	shotFgColor = Color(255, 255, 255, 255);

	int centerX = GetWide() / 2;
	int centerY = min(GetTall() - 30, GetTall() / 2 + (100 -::input->GetCameraAngles()[PITCH]) * (2 * GetTall() / 480.0f));

	// Draw stamina bar back
	surface()->DrawSetColor(staminaBgColor);
	surface()->DrawFilledRect(
		centerX - (BAR_WIDTH / 2 + BAR_HPADDING),
		centerY - (BAR_HEIGHT / 2 + BAR_VPADDING),
		centerX + (BAR_WIDTH / 2 + BAR_HPADDING),
		centerY + (BAR_HEIGHT / 2 + BAR_VPADDING));

	// Draw stamina bar front
	surface()->DrawSetColor(staminaFgColor);
	surface()->DrawFilledRect(
		centerX - (BAR_WIDTH / 2),
		centerY - (BAR_HEIGHT / 2),
		centerX - (BAR_WIDTH / 2) + stamina * (BAR_WIDTH),
		centerY + (BAR_HEIGHT / 2));

	// Draw shot bars
	surface()->DrawSetColor(shotFgColor);
	surface()->DrawFilledRect(
		centerX - (BAR_WIDTH / 2),
		centerY - (BAR_HEIGHT / 2 + 4),
		centerX - (BAR_WIDTH / 2) + shotStrength * (BAR_WIDTH),
		centerY - (BAR_HEIGHT / 2 - 2));

	surface()->DrawSetColor(shotFgColor);
	surface()->DrawFilledRect(
		centerX - (BAR_WIDTH / 2),
		centerY + (BAR_HEIGHT / 2 - 2),
		centerX - (BAR_WIDTH / 2) + shotStrength * (BAR_WIDTH),
		centerY + (BAR_HEIGHT / 2 + 4));
}