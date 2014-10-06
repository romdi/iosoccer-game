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

ConVar cl_staminabar_x("cl_staminabar_x", "30", FCVAR_ARCHIVE);
ConVar cl_staminabar_y("cl_staminabar_y", "-30", FCVAR_ARCHIVE);
ConVar cl_staminabar_width("cl_staminabar_width", "200", FCVAR_ARCHIVE);
ConVar cl_staminabar_height("cl_staminabar_height", "12", FCVAR_ARCHIVE);
ConVar cl_staminabar_vertical("cl_staminabar_vertical", "0", FCVAR_ARCHIVE);
ConVar cl_staminabar_border("cl_staminabar_border", "2", FCVAR_ARCHIVE);

ConVar cl_shotbar_x("cl_shotbar_x", "30", FCVAR_ARCHIVE);
ConVar cl_shotbar_y("cl_shotbar_y", "-50", FCVAR_ARCHIVE);
ConVar cl_shotbar_width("cl_shotbar_width", "200", FCVAR_ARCHIVE);
ConVar cl_shotbar_height("cl_shotbar_height", "12", FCVAR_ARCHIVE);
ConVar cl_shotbar_vertical("cl_shotbar_vertical", "0", FCVAR_ARCHIVE);
ConVar cl_shotbar_border("cl_shotbar_border", "2", FCVAR_ARCHIVE);

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
	if (!pPlayer || pPlayer->GetTeamNumber() != TEAM_A && pPlayer->GetTeamNumber() != TEAM_B)
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

	float stamina = pPlayer->m_Shared.GetStamina() / 100.0f;
	

	Color staminaFgColor, staminaBgColor, shotFgColor, shotBgColor;

	bool shotUnusable = true;

	if (pPlayer->GetFlags() & FL_REMOTECONTROLLED)
		shotFgColor = Color(100, 100, 100, 255);
	else if (pPlayer->m_bShotsBlocked)
		shotFgColor = Color(139, 0, 0, 255);
	else if (pPlayer->m_bChargedshotBlocked)
		shotFgColor = Color(255, 69, 0, 255);
	else
		shotUnusable = false;

	float shotStrength;

	if (shotUnusable)
	{
		shotStrength = 1.0f;
	}
	else
	{
		shotStrength = pPlayer->m_Shared.m_bDoChargedShot || pPlayer->m_Shared.m_bIsShotCharging ? pPlayer->GetChargedShotStrength() : 0;
		shotFgColor = Color(255 * (1 - shotStrength), 255, 255, 255);
	}

	staminaFgColor = Color(255 * (1 - stamina), 255 * stamina, 0, 255);
	staminaBgColor = Color(0, 0, 0, 255);

	shotBgColor = Color(0, 0, 0, 255);

	int centerX;
	int centerY;

	centerX = cl_staminabar_x.GetInt() > 0 ?
		cl_staminabar_x.GetInt() + cl_staminabar_border.GetInt() + cl_staminabar_width.GetInt() / 2
		: GetWide() + cl_staminabar_x.GetInt() - cl_staminabar_border.GetInt() - cl_staminabar_width.GetInt() / 2;
	
	centerY = cl_staminabar_y.GetInt() > 0 ?
		cl_staminabar_y.GetInt() + cl_staminabar_border.GetInt() + cl_staminabar_height.GetInt() / 2
		: GetTall() + cl_staminabar_y.GetInt() - cl_staminabar_border.GetInt() - cl_staminabar_height.GetInt() / 2;

	// Draw stamina bar back
	surface()->DrawSetColor(staminaBgColor);
	surface()->DrawFilledRect(
		centerX - (cl_staminabar_width.GetInt() / 2 + cl_staminabar_border.GetInt()),
		centerY - (cl_staminabar_height.GetInt() / 2 + cl_staminabar_border.GetInt()),
		centerX + (cl_staminabar_width.GetInt() / 2 + cl_staminabar_border.GetInt()),
		centerY + (cl_staminabar_height.GetInt() / 2 + cl_staminabar_border.GetInt()));

	// Draw stamina bar front
	surface()->DrawSetColor(staminaFgColor);

	if (cl_staminabar_vertical.GetBool())
	{
		surface()->DrawFilledRect(
			centerX - (cl_staminabar_width.GetInt() / 2),
			centerY - (cl_staminabar_height.GetInt() / 2) + (1 - stamina) * (cl_staminabar_width.GetInt()),
			centerX + (cl_staminabar_width.GetInt() / 2),
			centerY + (cl_staminabar_height.GetInt() / 2));
	}
	else
	{
		surface()->DrawFilledRect(
			centerX - (cl_staminabar_width.GetInt() / 2),
			centerY - (cl_staminabar_height.GetInt() / 2),
			centerX - (cl_staminabar_width.GetInt() / 2) + stamina * (cl_staminabar_width.GetInt()),
			centerY + (cl_staminabar_height.GetInt() / 2));
	}

	centerX = cl_shotbar_x.GetInt() > 0 ?
		cl_shotbar_x.GetInt() + cl_shotbar_border.GetInt() + cl_shotbar_width.GetInt() / 2
		: GetWide() + cl_shotbar_x.GetInt() - cl_shotbar_border.GetInt() - cl_shotbar_width.GetInt() / 2;
	
	centerY = cl_shotbar_y.GetInt() > 0 ?
		cl_shotbar_y.GetInt() + cl_shotbar_border.GetInt() + cl_shotbar_height.GetInt() / 2
		: GetTall() + cl_shotbar_y.GetInt() - cl_shotbar_border.GetInt() - cl_shotbar_height.GetInt() / 2;

	// Draw shot bar back
	surface()->DrawSetColor(shotBgColor);
	surface()->DrawFilledRect(
		centerX - (cl_shotbar_width.GetInt() / 2 + cl_shotbar_border.GetInt()),
		centerY - (cl_shotbar_height.GetInt() / 2 + cl_shotbar_border.GetInt()),
		centerX + (cl_shotbar_width.GetInt() / 2 + cl_shotbar_border.GetInt()),
		centerY + (cl_shotbar_height.GetInt() / 2 + cl_shotbar_border.GetInt()));

	// Draw shot bar front
	surface()->DrawSetColor(shotFgColor);

	if (cl_shotbar_vertical.GetBool())
	{
		surface()->DrawFilledRect(
			centerX - (cl_shotbar_width.GetInt() / 2),
			centerY - (cl_shotbar_height.GetInt() / 2) + (1 - shotStrength) * (cl_shotbar_height.GetInt()),
			centerX + (cl_shotbar_width.GetInt() / 2),
			centerY + (cl_shotbar_height.GetInt() / 2));
	}
	else
	{
		surface()->DrawFilledRect(
			centerX - (cl_shotbar_width.GetInt() / 2),
			centerY - (cl_shotbar_height.GetInt() / 2),
			centerX - (cl_shotbar_width.GetInt() / 2) + shotStrength * (cl_shotbar_width.GetInt()),
			centerY + (cl_shotbar_height.GetInt() / 2));
	}
}