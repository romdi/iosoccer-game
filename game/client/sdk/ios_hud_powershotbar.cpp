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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#if defined ( SDK_USE_STAMINA ) || defined ( SDK_USE_SPRINTING )

static void OnPowershotStrengthChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	//engine->ServerCmd(VarArgs("powershot_strength %d", ((ConVar*)var)->GetInt()));
	//C_BasePlayer::GetLocalPlayer();
}
 
ConVar cl_powershot_strength("cl_powershot_strength", "50", 0, "Powershot Strength (0-100)", true, 0, true, 100, OnPowershotStrengthChange );
//ConVar cl_powershot_fixed_strength("cl_powershot_fixed_strength", "50", FCVAR_ARCHIVE, "Powershot Fixed Strength (0-100)", true, 0, true, 100, OnPowershotStrengthChange );

//extern ConVar cl_powershot_strength;

static void OnIncreasePowershotStrength(const CCommand &args)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: increase_powershot_strength <1-100>\n");
		return;
	}
 
	cl_powershot_strength.SetValue(cl_powershot_strength.GetInt() + atoi(args.Arg(1)));
}

ConCommand increase_powershot_strength("increase_powershot_strength", OnIncreasePowershotStrength, "Increase Powershot Strength By Value");

static void OnDecreasePowershotStrength(const CCommand &args)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: decrease_powershot_strength <1-100>\n");
		return;
	}
 
	cl_powershot_strength.SetValue(cl_powershot_strength.GetInt() - atoi(args.Arg(1)));
}

ConCommand decrease_powershot_strength("decrease_powershot_strength", OnDecreasePowershotStrength, "Decrease Powershot Strength By Value");

ConVar cl_chargedshot_duration("cl_chargedshot_duration", "0.5", FCVAR_ARCHIVE);



class CHudPowershotBar : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudPowershotBar, vgui::Panel );

public:
	CHudPowershotBar( const char *pElementName );
	virtual void	Init( void );
	virtual void	Reset( void );
	virtual void	OnThink( void );
	bool			ShouldDraw( void );

protected:
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint();

	Panel *m_pStaminaPanel;
	Panel *m_pStaminaIndicator;
	Panel *m_pPowershotIndicator;
	Panel *m_pPowershotIndicatorBack;
	float m_flOldStamina;
	float m_flNextUpdate;
	bool m_bIsChargingShot;
	bool m_bIsHoldingShot;
	float m_flChargingStartTime;
	int m_nChargingStartPower;
};

DECLARE_HUDELEMENT( CHudPowershotBar );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudPowershotBar::CHudPowershotBar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudPowershotBar" )
{
	SetHiddenBits(HIDEHUD_POWERSHOTBAR);

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pStaminaPanel = new Panel(this, "StaminaPanel");
	m_pStaminaIndicator = new Panel(m_pStaminaPanel, "StaminaIndicator");
	m_pPowershotIndicatorBack = new Panel(this, "PowershotIndicator");
	m_pPowershotIndicator = new Panel(m_pPowershotIndicatorBack, "PowershotIndicator");
	m_flOldStamina = 100;
	m_flNextUpdate = gpGlobals->curtime;
	m_bIsChargingShot = false;
	m_bIsHoldingShot = false;
	m_flChargingStartTime = gpGlobals->curtime;
	m_nChargingStartPower = 0;
}

#define BAR_WIDTH 40
#define BAR_HEIGHT 200
#define BAR_VMARGIN 15
#define BAR_HMARGIN 250
#define BAR_PADDING 2
#define PS_INDICATOR_HEIGHT 9
#define PS_INDICATOR_OFFSET 9
#define PS_INDICATOR_BORDER 2
#define FIXED_PS_INDICATOR_HEIGHT 3
#define SPIN_MARGIN 7
#define SPIN_HEIGHT 9
#define	SPIN_BORDER 2

void CHudPowershotBar::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings(scheme);

	SetBounds(ScreenWidth() - BAR_WIDTH - 2 * BAR_HMARGIN, ScreenHeight() - BAR_HEIGHT - 2 * BAR_VMARGIN, BAR_WIDTH + 2 * BAR_HMARGIN, BAR_HEIGHT + 2 * BAR_VMARGIN);

	m_pStaminaPanel->SetPaintBackgroundType(2); // Rounded corner box
 	m_pStaminaPanel->SetPaintBackgroundEnabled(true);
	m_pStaminaPanel->SetBgColor(Color(0, 0, 0, 255));
	m_pStaminaPanel->SetBounds(BAR_HMARGIN, SPIN_HEIGHT, BAR_WIDTH, BAR_HEIGHT);

	m_pStaminaIndicator->SetPaintBackgroundType(2); // Rounded corner box
 	m_pStaminaIndicator->SetPaintBackgroundEnabled(true);
	m_pStaminaIndicator->SetBgColor(Color(0, 255, 0, 255) );
	m_pStaminaIndicator->SetBounds(BAR_PADDING, BAR_PADDING, BAR_WIDTH - 2 * BAR_PADDING, (BAR_HEIGHT - 2 * BAR_PADDING) / 2);

 	m_pPowershotIndicatorBack->SetPaintBackgroundEnabled(true);
	m_pPowershotIndicatorBack->SetBgColor(Color(0, 0, 0, 255));
	m_pPowershotIndicatorBack->SetBounds(GetWide() / 2 - BAR_WIDTH / 2 - PS_INDICATOR_OFFSET, GetTall() / 2 - PS_INDICATOR_HEIGHT / 2, BAR_WIDTH + 2 * PS_INDICATOR_OFFSET, PS_INDICATOR_HEIGHT);

 	m_pPowershotIndicator->SetPaintBackgroundEnabled(true);
	m_pPowershotIndicator->SetBgColor(Color(255, 255, 255, 255));
	m_pPowershotIndicator->SetBounds(PS_INDICATOR_BORDER, PS_INDICATOR_BORDER, m_pPowershotIndicatorBack->GetWide() - 2 * PS_INDICATOR_BORDER, m_pPowershotIndicatorBack->GetTall() - 2 * PS_INDICATOR_BORDER);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudPowershotBar::Init( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudPowershotBar::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudPowershotBar::ShouldDraw()
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
void CHudPowershotBar::OnThink( void )
{
	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: draws the stamina bar
//-----------------------------------------------------------------------------
void CHudPowershotBar::Paint()
{
	C_SDKPlayer *pPlayer = C_SDKPlayer::GetLocalSDKPlayer();

	if ( !pPlayer )
		return;

	float stamina = pPlayer->m_Shared.GetStamina();
	float relStamina = stamina / 100.0f;

	Color bgColor;

	if (pPlayer->GetFlags() & FL_REMOTECONTROLLED)
		bgColor = Color(255, 255, 255, 255);
	else
		bgColor = Color(255 * (1 - relStamina), 255 * relStamina, 0, 255);

	m_pPowershotIndicatorBack->SetY(m_pStaminaPanel->GetY() + BAR_PADDING + m_pPowershotIndicatorBack->GetTall() + (1 - cl_powershot_strength.GetInt() / 100.0f) * (BAR_HEIGHT - 2 * BAR_PADDING - 3 * m_pPowershotIndicatorBack->GetTall()));

	float shotStrength;

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

		m_pPowershotIndicatorBack->SetY(m_pStaminaPanel->GetY() + BAR_PADDING + m_pPowershotIndicatorBack->GetTall() + (1 - shotStrength) * (BAR_HEIGHT - 2 * BAR_PADDING - 3 * m_pPowershotIndicatorBack->GetTall()));
		m_pPowershotIndicatorBack->SetVisible(true);
	}
	else
		m_pPowershotIndicatorBack->SetVisible(false);

	m_pStaminaIndicator->SetTall((m_pStaminaPanel->GetTall() - 2 * BAR_PADDING) * relStamina);
	m_pStaminaIndicator->SetY(BAR_PADDING + (m_pStaminaPanel->GetTall() - 2 * BAR_PADDING) - m_pStaminaIndicator->GetTall());
	m_pStaminaIndicator->SetBgColor(bgColor);
}