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

enum { BAR_WIDTH = 40, BAR_HEIGHT = 200, BAR_VMARGIN = 15, BAR_HMARGIN = 250, BAR_HPADDING = 2, BAR_VPADDING = 2 };
enum { PS_INDICATOR_HEIGHT = 9, PS_INDICATOR_OFFSET = 9, PS_INDICATOR_BORDER = 2 };
enum { SMALLBAR_WIDTH = 80, SMALLBAR_HEIGHT = 14, SMALLBAR_HPADDING = 1, SMALLBAR_VPADDING = 1 };
enum { SMALLPS_INDICATOR_WIDTH = 7, SMALLPS_INDICATOR_OFFSET = 5, SMALLPS_INDICATOR_BORDER = 1 };

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

	float stamina = pPlayer->m_Shared.GetStamina();
	float relStamina = stamina / 100.0f;

	Color fgColor, bgColor;

	Color missingMaxStaminaColor = Color(100, 100, 100, 255);

	if (pPlayer->GetFlags() & FL_REMOTECONTROLLED)
	{
		fgColor = Color(255, 255, 255, 255);
		bgColor = Color(100, 100, 100, 255);
	}
	else if (GetMatchBall() && GetMatchBall()->m_bShotsBlocked)
	{
		fgColor = Color(255, 0, 0, 255);
		bgColor = Color(139, 0, 0, 255);
	}
	else if (GetMatchBall() && GetMatchBall()->m_bNonnormalshotsBlocked)
	{
		fgColor = Color(255, 140, 0, 255);
		bgColor = Color(255, 69, 0, 255);
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
		shotStrength = pPlayer->GetChargedShotStrength();

		// Flash
		if (shotStrength > 0.9f)
		{
			//relStamina = 1;
			bgColor = Color(255, 255, 255, 255);
		}

		drawChargedshotIndicator = true;
	}

	if (centeredstaminabar.GetBool())
	{
		const int CenterX = GetWide() / 2;
		const int CenterY = GetTall() / 2;

		// Draw stamina bar back
		surface()->DrawSetColor(bgColor);
		surface()->DrawFilledRect(
			CenterX - (SMALLBAR_WIDTH / 2),
			CenterY - (SMALLBAR_HEIGHT / 2),
			CenterX + (SMALLBAR_WIDTH / 2),
			CenterY + (SMALLBAR_HEIGHT / 2));

		// Draw missing max stamina
		surface()->DrawSetColor(missingMaxStaminaColor);
		surface()->DrawFilledRect(
			CenterX + (SMALLBAR_WIDTH / 2 - SMALLBAR_HPADDING) - (1.0f - pPlayer->m_Shared.GetMaxStamina() / 100.0f) * (SMALLBAR_WIDTH - 2 * SMALLBAR_VPADDING),
			CenterY - (SMALLBAR_HEIGHT / 2 - SMALLBAR_VPADDING),
			CenterX + (SMALLBAR_WIDTH / 2 - SMALLBAR_HPADDING),
			CenterY + (SMALLBAR_HEIGHT / 2 - SMALLBAR_VPADDING));

		// Draw stamina bar front
		surface()->DrawSetColor(fgColor);
		surface()->DrawFilledRect(
			CenterX - (SMALLBAR_WIDTH / 2 - SMALLBAR_HPADDING),
			CenterY - (SMALLBAR_HEIGHT / 2 - SMALLBAR_VPADDING),
			CenterX - (SMALLBAR_WIDTH / 2 - SMALLBAR_HPADDING) + relStamina * (SMALLBAR_WIDTH - 2 * SMALLBAR_VPADDING),
			CenterY + (SMALLBAR_HEIGHT / 2 - SMALLBAR_VPADDING));

		const int partCount = 4;
		const int hMargin = SMALLBAR_WIDTH / partCount;

		surface()->DrawSetColor(bgColor);

		for (int i = 1; i < partCount; i++)
		{
			surface()->DrawFilledRect(
				CenterX - SMALLBAR_WIDTH / 2 + i * hMargin,
				CenterY - SMALLBAR_HEIGHT / 2,
				CenterX - SMALLBAR_WIDTH / 2 + i * hMargin + 1,
				CenterY + SMALLBAR_HEIGHT / 2);
		}

		if (drawChargedshotIndicator)
		{
			// Draw chargedshot indicator back
			surface()->DrawSetColor(0, 0, 0, 255);
			surface()->DrawFilledRect(
				CenterX - SMALLBAR_WIDTH / 2 + shotStrength * SMALLBAR_WIDTH - SMALLPS_INDICATOR_WIDTH / 2,
				CenterY - SMALLBAR_HEIGHT / 2 - SMALLPS_INDICATOR_OFFSET,
				CenterX - SMALLBAR_WIDTH / 2 + shotStrength * SMALLBAR_WIDTH + SMALLPS_INDICATOR_WIDTH / 2,
				CenterY + SMALLBAR_HEIGHT / 2 + SMALLPS_INDICATOR_OFFSET);

			// Draw chargedshot indicator front
			surface()->DrawSetColor(255, 255, 255, 255);
			surface()->DrawFilledRect(
				CenterX - SMALLBAR_WIDTH / 2 + shotStrength * SMALLBAR_WIDTH - SMALLPS_INDICATOR_WIDTH / 2 + SMALLPS_INDICATOR_BORDER,
				CenterY - SMALLBAR_HEIGHT / 2 - SMALLPS_INDICATOR_OFFSET + SMALLPS_INDICATOR_BORDER,
				CenterX - SMALLBAR_WIDTH / 2 + shotStrength * SMALLBAR_WIDTH + SMALLPS_INDICATOR_WIDTH / 2 - SMALLPS_INDICATOR_BORDER,
				CenterY + SMALLBAR_HEIGHT / 2 + SMALLPS_INDICATOR_OFFSET - SMALLPS_INDICATOR_BORDER);

		}
	}
	else
	{
		const int LeftX = GetWide() - BAR_WIDTH - 2 * BAR_HMARGIN;
		const int LeftY = GetTall() - BAR_HEIGHT - 2 * BAR_VMARGIN;

		// Draw stamina bar back
		surface()->DrawSetColor(bgColor);
		surface()->DrawFilledRect(
			LeftX + BAR_HMARGIN,
			LeftY + BAR_VMARGIN,
			LeftX + BAR_HMARGIN + BAR_WIDTH,
			LeftY + BAR_VMARGIN + BAR_HEIGHT);

		// Draw missing max stamina
		surface()->DrawSetColor(missingMaxStaminaColor);
		surface()->DrawFilledRect(
			LeftX + BAR_HMARGIN + BAR_HPADDING,
			LeftY + BAR_VMARGIN + BAR_VPADDING,
			LeftX + BAR_HMARGIN + BAR_WIDTH - BAR_HPADDING,
			LeftY + BAR_VMARGIN + BAR_VPADDING + (1.0f - pPlayer->m_Shared.GetMaxStamina() / 100.0f) * (BAR_HEIGHT - 2 * BAR_VPADDING));

		// Draw stamina bar front
		surface()->DrawSetColor(fgColor);
		surface()->DrawFilledRect(
			LeftX + BAR_HMARGIN + BAR_HPADDING,
			LeftY + BAR_VMARGIN + BAR_VPADDING + (1 - relStamina) * (BAR_HEIGHT - 2 * BAR_VPADDING),
			LeftX + BAR_HMARGIN + BAR_WIDTH - BAR_HPADDING,
			LeftY + BAR_VMARGIN + BAR_VPADDING + BAR_HEIGHT - 2 * BAR_VPADDING);

		const int partCount = 4;
		const int vMargin = BAR_HEIGHT / partCount;

		surface()->DrawSetColor(bgColor);

		for (int i = 1; i < partCount; i++)
		{
			surface()->DrawFilledRect(
				LeftX + BAR_HMARGIN,
				LeftY + BAR_VMARGIN + i * vMargin,
				LeftX + BAR_HMARGIN + BAR_WIDTH,
				LeftY + BAR_VMARGIN + i * vMargin + 1);
		}

		if (drawChargedshotIndicator)
		{
			// Draw chargedshot indicator back
			surface()->DrawSetColor(0, 0, 0, 255);
			surface()->DrawFilledRect(
				LeftX + BAR_HMARGIN - PS_INDICATOR_OFFSET,
				LeftY + BAR_VMARGIN + (1 - shotStrength) * (BAR_HEIGHT - PS_INDICATOR_HEIGHT),
				LeftX + BAR_HMARGIN + BAR_WIDTH + PS_INDICATOR_OFFSET,
				LeftY + BAR_VMARGIN + (1 - shotStrength) * (BAR_HEIGHT - PS_INDICATOR_HEIGHT) + PS_INDICATOR_HEIGHT);

			// Draw chargedshot indicator front
			surface()->DrawSetColor(255, 255, 255, 255);
			surface()->DrawFilledRect(
				LeftX + BAR_HMARGIN - PS_INDICATOR_OFFSET + PS_INDICATOR_BORDER,
				LeftY + BAR_VMARGIN + (1 - shotStrength) * (BAR_HEIGHT - PS_INDICATOR_HEIGHT) + PS_INDICATOR_BORDER,
				LeftX + BAR_HMARGIN + BAR_WIDTH + PS_INDICATOR_OFFSET - PS_INDICATOR_BORDER,
				LeftY + BAR_VMARGIN + (1 - shotStrength) * (BAR_HEIGHT - PS_INDICATOR_HEIGHT) + PS_INDICATOR_HEIGHT - PS_INDICATOR_BORDER);
		}
	}
}