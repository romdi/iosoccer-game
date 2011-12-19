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

	float m_flStamina;
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
}

void CHudPowershotBar::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( true );

    //SetSize( ScreenWidth(), ScreenHeight() );
	SetSize(75, 200);
	SetPos(ScreenWidth() - 100, ScreenHeight() - 250);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudPowershotBar::Init( void )
{
	m_flStamina = -1;
	//m_flStaminaLow = -1;
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
	float flCurrentStamina = 0;
	C_SDKPlayer *pPlayer = C_SDKPlayer::GetLocalSDKPlayer();
	if ( !pPlayer )
		return;

	flCurrentStamina = pPlayer->m_Shared.GetStamina();

	// Only update if we've changed stamina
	if ( flCurrentStamina == m_flStamina )
		return;

	//if ( flCurrentStamina >= 100.0f && m_flStamina < 100.0f )
	//{
	//	// we've reached max stamina
	//	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitStaminaMax");
	//}
	//else if ( flCurrentStamina < 100.0f && (m_flStamina >= 100.0f || m_flStamina == STAMINA_INIT) )
	//{
	//	// we've lost stamina
	//	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitStaminaNotMax");
	//}
	m_flStamina = flCurrentStamina;
}

#define SPRINT_TIME           6.0f     //IOS sprint amount 5.5
#define SPRINT_RECHARGE_TIME  12.0f    //IOS time before sprint re-charges
#define SPRINT_SPEED          90.0f    //IOS sprint increase in speed
#define SEGMENTS 3
//-----------------------------------------------------------------------------
// Purpose: draws the stamina bar
//-----------------------------------------------------------------------------
void CHudPowershotBar::Paint()
{
	C_SDKPlayer *pPlayer = C_SDKPlayer::GetLocalSDKPlayer();

	if ( !pPlayer )
		return;

	//IGameResources *gr = GameResources();

	float sprint = m_flStamina / 100.0f;//gr->GetSprint(pPlayer->index) / (SPRINT_TIME * 10);
	int height = 150;
	int width = 25;
	int xOffset = 20;
	int yOffset = 10;

	surface()->DrawSetColor(0, 0, 0, 200);
	//surface()->DrawFilledRect(xOffset - 1, yOffset - 1, xOffset + width + 1, yOffset + height + 1);
	surface()->DrawLine(xOffset - 1, yOffset - 1, xOffset - 1, yOffset + height);
	surface()->DrawLine(xOffset + width, yOffset - 1, xOffset + width, yOffset + height);
	surface()->DrawSetColor(0, 0, 0, 150);
	surface()->DrawFilledRect(xOffset, yOffset, xOffset + width, yOffset + height);
	surface()->DrawSetColor(255 * (1 - sprint), 255 * sprint, 0, 200);
	int y0 = yOffset + height * (1 - sprint);
	surface()->DrawFilledRect(xOffset, y0, xOffset + width, y0 + (int)(height * sprint));

	surface()->DrawSetColor(0, 0, 0, 255);

	float powershotStrength = cl_powershot_strength.GetInt() / 100.0f;
	int x0, x1, linepadding;
	y0 = yOffset + height * (1 - powershotStrength);
	x0 = xOffset - 10;
	x1 = x0 + 10 + width + 10;
	linepadding = 2;
	surface()->DrawSetColor(255, 255, 255, 255);
	surface()->DrawFilledRect(x0, y0 - linepadding, x1, y0 + linepadding);

	//for (int i = 0; i < SEGMENTS + 1; i++)
	//{
	//	y0 = yOffset + height / SEGMENTS * i;
	//	int x0, x1, linepadding;
	//	float powershotStrength = cl_powershot_strength.GetInt() / 100.0f;

	//	if (i == powershotStrength)
	//	{
	//		if (sprint >= (powershotStrength / 5.0f - FLT_EPSILON))
	//			surface()->DrawSetColor(200, 255, 200, 255);
	//		else
	//			surface()->DrawSetColor(255, 200, 200, 255);

	//		x0 = xOffset - 10;
	//		x1 = x0 + 10 + width + 10;
	//		linepadding = 2;
	//		//surface()->DrawSetTextFont(scheme()->GetIScheme(GetScheme())->GetFont("Default"));
	//		//surface()->DrawSetTextColor(255, 255, 255, 150);
	//		//surface()->DrawSetTextPos(0, y0 - 8);
	//		////surface()->DrawSetTextScale(2.0f, 2.0f);
	//		//surface()->DrawPrintText(L">", wcslen(L">"));
	//		////surface()->DrawPolyLine
	//	}
	//	else
	//	{
	//		surface()->DrawSetColor(0, 0, 0, 200);
	//		x0 = xOffset;
	//		x1 = x0 + width;
	//		linepadding = 1;
	//	}

	//	surface()->DrawFilledRect(x0, y0 - linepadding, x1, y0 + linepadding);
	//}
}
//#endif // SDK_USE_STAMINA || SDK_USE_SPRINTING

