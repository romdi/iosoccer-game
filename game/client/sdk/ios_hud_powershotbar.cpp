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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#if defined ( SDK_USE_STAMINA ) || defined ( SDK_USE_SPRINTING )

static void OnPowershotStrengthChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	//engine->ServerCmd(VarArgs("powershot_strength %d", ((ConVar*)var)->GetInt()));
	//C_BasePlayer::GetLocalPlayer();
}
 
ConVar cl_powershot_strength("cl_powershot_strength", "50", 0, "Powershot Strength (0-100)", true, 0, true, 100, OnPowershotStrengthChange );

//extern ConVar cl_powershot_strength;

static void OnIncreasePowershotStrength(const CCommand &args)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: increase_powershot_strength <1-4>\n");
		return;
	}
 
	cl_powershot_strength.SetValue(cl_powershot_strength.GetInt() + atoi(args.Arg(1)));
}

ConCommand increase_powershot_strength("increase_powershot_strength", OnIncreasePowershotStrength, "Increase Powershot Strength By Value");

static void OnDecreasePowershotStrength(const CCommand &args)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: decrease_powershot_strength <1-4>\n");
		return;
	}
 
	cl_powershot_strength.SetValue(cl_powershot_strength.GetInt() - atoi(args.Arg(1)));
}

ConCommand decrease_powershot_strength("decrease_powershot_strength", OnDecreasePowershotStrength, "Decrease Powershot Strength By Value");



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
	Panel *m_pPowershotIndicator;
	float m_flOldStamina;
	float m_flNextUpdate;
};

DECLARE_HUDELEMENT( CHudPowershotBar );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudPowershotBar::CHudPowershotBar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudPowershotBar" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits(HIDEHUD_PLAYERDEAD);
	//SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_pStaminaPanel = new Panel(this, "StaminaPanel");
	m_pPowershotIndicator = new Panel(this, "PowershotIndicator");
	m_flOldStamina = 100;
	m_flNextUpdate = gpGlobals->curtime;
}

#define WIDTH 40
#define HEIGHT 200
#define MARGIN 25
#define PADDING 2
#define HEIGHT_INDICATOR 9

void CHudPowershotBar::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

 	SetPaintBackgroundType (2); // Rounded corner box
 	SetPaintBackgroundEnabled(true);
	SetBgColor( Color( 0, 0, 0, 255 ) );
	SetBounds(ScreenWidth() - WIDTH - MARGIN, ScreenHeight() - HEIGHT - MARGIN, WIDTH, HEIGHT);

	m_pStaminaPanel->SetPaintBackgroundType (2); // Rounded corner box
 	m_pStaminaPanel->SetPaintBackgroundEnabled(true);
	m_pStaminaPanel->SetBgColor( Color( 0, 255, 0, 255 ) );
	m_pStaminaPanel->SetBounds(PADDING, PADDING, WIDTH - 2 * PADDING, HEIGHT - 2 * PADDING);

 	m_pPowershotIndicator->SetPaintBackgroundEnabled(true);
	m_pPowershotIndicator->SetBgColor( Color( 255, 255, 255, 255 ) );
	m_pPowershotIndicator->SetBounds(PADDING, HEIGHT / 2 - HEIGHT_INDICATOR / 2, WIDTH - 2 * PADDING, HEIGHT_INDICATOR);
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
	C_SDKPlayer *pPlayer = C_SDKPlayer::GetLocalSDKPlayer();
	if ( !pPlayer )
		return false;
	if ( pPlayer->State_Get() != STATE_ACTIVE )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudPowershotBar::OnThink( void )
{
	BaseClass::OnThink();

	//if (m_flNextUpdate <= gpGlobals->curtime)
	//{
	//	m_flOldStamina = 
	//	m_flNextUpdate = gpGlobals->curtime + 1.0f;
	//}
}

//-----------------------------------------------------------------------------
// Purpose: draws the stamina bar
//-----------------------------------------------------------------------------
void CHudPowershotBar::Paint()
{
	C_SDKPlayer *pPlayer = C_SDKPlayer::GetLocalSDKPlayer();

	if ( !pPlayer )
		return;

	//float stamina = Lerp(0.25f * gpGlobals->frametime, m_flOldStamina, pPlayer->m_Shared.GetStamina());

	float stamina = pPlayer->m_Shared.GetStamina();
	//float staminaDiff = pPlayer->m_Shared.GetStamina() - m_flOldStamina;

	//if (staminaDiff >= 5)
	//{
	//	stamina = min(pPlayer->m_Shared.GetStamina(), m_flOldStamina + 1 * gpGlobals->frametime);
	//}
	//else if (staminaDiff <= -5)
	//{
	//	stamina = max(pPlayer->m_Shared.GetStamina(), m_flOldStamina - 1 * gpGlobals->frametime);
	//}

	//m_flOldStamina = stamina;

	float relStamina = stamina / 100.0f;

	m_pStaminaPanel->SetTall(GetTall() * relStamina - 2 * PADDING);
	m_pStaminaPanel->SetY(GetTall() - PADDING - m_pStaminaPanel->GetTall());

	Color bgColor;

	if (pPlayer->GetFlags() & FL_REMOTECONTROLLED)
		bgColor = Color(255, 255, 255, 255);
	else
		bgColor = Color(255 * (1 - relStamina), 255 * relStamina, 0, 255);

	m_pStaminaPanel->SetBgColor(bgColor);

	m_pPowershotIndicator->SetY(PADDING + m_pPowershotIndicator->GetTall() + (1 - cl_powershot_strength.GetInt() / 100.0f) * (HEIGHT - 2 * PADDING - 3 * m_pPowershotIndicator->GetTall()));
}