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
#include "c_ball.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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

#define BAR_WIDTH 40
#define BAR_HEIGHT 200
#define BAR_VMARGIN 15
#define BAR_HMARGIN 250
#define BAR_HPADDING 2
#define BAR_VPADDING 2
#define PS_INDICATOR_HEIGHT 9
#define PS_INDICATOR_OFFSET 9
#define PS_INDICATOR_BORDER 2

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

	SetBounds(ScreenWidth() - BAR_WIDTH - 2 * BAR_HMARGIN, ScreenHeight() - BAR_HEIGHT - 2 * BAR_VMARGIN, BAR_WIDTH + 2 * BAR_HMARGIN, BAR_HEIGHT + 2 * BAR_VMARGIN);
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

	float stamina = pPlayer->m_Shared.GetStamina();
	float relStamina = stamina / 100.0f;

	Color fgColor, bgColor;

	if (pPlayer->GetFlags() & FL_REMOTECONTROLLED)
	{
		fgColor = Color(255, 255, 255, 255);
		bgColor = Color(100, 100, 100, 255);
	}
	else if (GetBall() && GetBall()->m_bNonnormalshotsBlocked)
	{
		fgColor = Color(255, 0, 0, 255);
		bgColor = Color(100, 0, 0, 255);
	}
	else
	{
		fgColor = Color(255 * (1 - relStamina), 255 * relStamina, 0, 255);
		bgColor = Color(0, 0, 0, 255);
	}

	float shotStrength;

	bool drawChargedshotIndicator = false;

	if (pPlayer->m_Shared.m_bDoChargedShot || pPlayer->m_Shared.m_bIsShotCharging)
	{
		float currentTime = pPlayer->GetFinalPredictedTime();
		currentTime -= TICK_INTERVAL;
		currentTime += (gpGlobals->interpolation_amount * TICK_INTERVAL);

		float duration = (pPlayer->m_Shared.m_bIsShotCharging ? currentTime - pPlayer->m_Shared.m_flShotChargingStart : pPlayer->m_Shared.m_flShotChargingDuration);
		float totalTime = currentTime - pPlayer->m_Shared.m_flShotChargingStart;
		float activeTime = min(duration, mp_chargedshot_increaseduration.GetFloat());
		float extra = totalTime - activeTime;
		float increaseFraction = clamp(pow(activeTime / mp_chargedshot_increaseduration.GetFloat(), mp_chargedshot_increaseexponent.GetFloat()), 0, 1);
		float decTime = (pow(1 - increaseFraction, 1.0f / mp_chargedshot_decreaseexponent.GetFloat())) * mp_chargedshot_decreaseduration.GetFloat();
		float decreaseFraction = clamp((decTime + extra) / mp_chargedshot_decreaseduration.GetFloat(), 0, 1);
		shotStrength = 1 - pow(decreaseFraction, mp_chargedshot_decreaseexponent.GetFloat());

		// Flash
		if (shotStrength > 0.95f)
		{
			relStamina = 1;
			bgColor = Color(255, 255, 255, 255);
		}

		drawChargedshotIndicator = true;
	}

	// Draw stamina bar back
	surface()->DrawSetColor(bgColor);
	surface()->DrawFilledRect(BAR_HMARGIN, BAR_VMARGIN, BAR_HMARGIN + BAR_WIDTH, BAR_VMARGIN + BAR_HEIGHT);

	// Draw stamina bar front
	surface()->DrawSetColor(fgColor);
	surface()->DrawFilledRect(
		BAR_HMARGIN + BAR_HPADDING,
		BAR_VMARGIN + BAR_VPADDING + (1 - relStamina) * (BAR_HEIGHT - 2 * BAR_VPADDING),
		BAR_HMARGIN + BAR_WIDTH - BAR_HPADDING,
		BAR_VMARGIN + BAR_VPADDING + BAR_HEIGHT - 2 * BAR_VPADDING);

	const int partCount = 4;
	int vMargin = BAR_HEIGHT / partCount;

	surface()->DrawSetColor(bgColor);

	for (int i = 1; i < partCount; i++)
	{
		surface()->DrawFilledRect(BAR_HMARGIN, BAR_VMARGIN + i * vMargin, BAR_HMARGIN + BAR_WIDTH, BAR_VMARGIN + i * vMargin + 1);
	}

	if (drawChargedshotIndicator)
	{
		// Draw chargedshot indicator back
		surface()->DrawSetColor(0, 0, 0, 255);
		surface()->DrawFilledRect(
			BAR_HMARGIN - PS_INDICATOR_OFFSET,
			BAR_VMARGIN + (1 - shotStrength) * (BAR_HEIGHT - PS_INDICATOR_HEIGHT),
			BAR_HMARGIN + BAR_WIDTH + PS_INDICATOR_OFFSET,
			BAR_VMARGIN + (1 - shotStrength) * (BAR_HEIGHT - PS_INDICATOR_HEIGHT) + PS_INDICATOR_HEIGHT);

		// Draw chargedshot indicator front
		surface()->DrawSetColor(255, 255, 255, 255);
		surface()->DrawFilledRect(
			BAR_HMARGIN - PS_INDICATOR_OFFSET + PS_INDICATOR_BORDER,
			BAR_VMARGIN + (1 - shotStrength) * (BAR_HEIGHT - PS_INDICATOR_HEIGHT) + PS_INDICATOR_BORDER,
			BAR_HMARGIN + BAR_WIDTH + PS_INDICATOR_OFFSET - PS_INDICATOR_BORDER,
			BAR_VMARGIN + (1 - shotStrength) * (BAR_HEIGHT - PS_INDICATOR_HEIGHT) + PS_INDICATOR_HEIGHT - PS_INDICATOR_BORDER);

	}
}