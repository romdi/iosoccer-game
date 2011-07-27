//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "c_baseplayer.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "c_sdk_team.h"
#include "sdk_gamerules.h"

#include <igameresources.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SUITPOWER_INIT -1
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include <vgui_controls/Panel.h>

//-----------------------------------------------------------------------------
// Purpose: Shows the logo top left, time and score.
//-----------------------------------------------------------------------------
class CHudLogo : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudLogo, vgui::Panel );

public:
	CHudLogo( const char *pElementName );
	virtual void	Init( void );
	virtual void	Reset( void );
	virtual void	OnThink( void );
	bool			ShouldDraw( void );

protected:
	virtual void	Paint();

private:
	CPanelAnimationVar( Color, m_AuxPowerColor, "AuxPowerColor", "255 255 255 255" );
	CPanelAnimationVar( int, m_iAuxPowerDisabledAlpha, "AuxPowerDisabledAlpha", "170" );

	CPanelAnimationVarAliasType( float, m_flBarInsetX, "BarInsetX", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarInsetY, "BarInsetY", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarWidth, "BarWidth", "80", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarHeight, "BarHeight", "10", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarChunkWidth, "BarChunkWidth", "10", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarChunkGap, "BarChunkGap", "2", "proportional_float" );

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVarAliasType( float, text_xpos, "text_xpos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, text_ypos, "text_ypos", "20", "proportional_float" );
	CPanelAnimationVarAliasType( float, text2_xpos, "text2_xpos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, text2_ypos, "text2_ypos", "40", "proportional_float" );
	CPanelAnimationVarAliasType( float, text2_gap, "text2_gap", "10", "proportional_float" );

	float m_flSuitPower;
	int m_nSuitPowerLow;
	int m_iActiveSuitDevices;

	float m_fPrevSprint;

	int	GetScaledTime(void);

	int	m_nImport;
};	

DECLARE_HUDELEMENT( CHudLogo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudLogo::CHudLogo( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudLogo" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

   m_nImport = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImport, "vgui/logobutton" , true, true);

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLogo::Init( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLogo::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudLogo::ShouldDraw()
{
	C_BasePlayer *pPlayer = (C_BasePlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	//if (pPlayer->GetTeamNumber() < TEAM_A)
	//	return FALSE;

	return TRUE;

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLogo::OnThink( void )
{
}

//-----------------------------------------------------------------------------
//	Draw top left logo
//
//	scripts/hudlayout.red/hudlogo
//-----------------------------------------------------------------------------
void CHudLogo::Paint()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	IGameResources *gr = GameResources();

	SetBgColor(Color(0,0,0,0));


	///////////////draw background coloured rectangles first

	//logo rect
	surface()->DrawSetColor( Color(255,255,255,200) );
	surface()->DrawFilledRect( 0, 0, m_flBarWidth, m_flBarHeight );
	//draw clock rect
	surface()->DrawSetColor( Color(0,0,0,200) );
	surface()->DrawFilledRect( m_flBarWidth, 0, m_flBarWidth*2, m_flBarHeight );	//x,y, xEND!!!, YEND!!
	//score rect
	surface()->DrawSetColor( Color(182, 83, 10, 200) );
	surface()->DrawFilledRect( m_flBarWidth*2, 0, m_flBarWidth*4, m_flBarHeight );

	//////////////overlay the glass button texture
	surface()->DrawSetColor( Color(255, 255, 255, 128) );
	SetPaintBorderEnabled(false);
	surface()->DrawSetTexture( m_nImport );
	surface()->DrawTexturedRect( 0, 0, m_flBarWidth*4, m_flBarHeight );



	/////////////LOGO/////////////////////////////////////////
	// draw logo text
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(Color(0,0,0,200));
	surface()->DrawSetTextPos(text_xpos-10, text_ypos);		//fine tune xpos

	wchar_t wtag[7];
	C_SDKTeam *pTeam = (C_SDKTeam*)GetGlobalTeam( 0 );		//why have it in all teams - doesnt seem right.
	vgui::localize()->ConvertANSIToUnicode( pTeam->GetScoreTag(), wtag, sizeof(wtag)  );
	surface()->DrawPrintText(wtag, wcslen(wtag));

	/////////////CLOCK/////////////////////////////////////////

	surface()->DrawSetTextColor(Color(255,255,255,200));
	surface()->DrawSetTextPos(text_xpos+m_flBarWidth-2, text_ypos);
	int t = GetScaledTime();
	int mins = (int)(t / 60.0f);
	int secs = t % 60;
	char matchtime[32];
	Q_snprintf (matchtime, sizeof(matchtime), " %02i:%02i", mins, secs);
	wchar wmatchtime[32];
	vgui::localize()->ConvertANSIToUnicode( matchtime, wmatchtime, sizeof(wmatchtime)  );
	surface()->DrawPrintText(wmatchtime, wcslen(wmatchtime));


	/////////////SCORE/////////////////////////////////////////
	//draw the team initials and score background twice as wide - orange bg / black text 

	surface()->DrawSetTextColor(Color(0,0,0,200));
	surface()->DrawSetTextPos(text_xpos+2*m_flBarWidth, text_ypos);
	char teama[64], teamb[64];
	wchar_t wteama[64], wteamb[64];
	Q_strncpy(teama, gr->GetTeamName(TEAM_A), 4);
	Q_strncpy(teamb, gr->GetTeamName(TEAM_B), 4);
	vgui::localize()->ConvertANSIToUnicode( teama, wteama, sizeof(wteama)  );
	vgui::localize()->ConvertANSIToUnicode( teamb, wteamb, sizeof(wteamb)  );
	wchar_t scoretext[64];
	swprintf(scoretext, L"%s %d - %d %s", wteama, gr->GetTeamScore(TEAM_A), gr->GetTeamScore(TEAM_B), wteamb);
	surface()->DrawPrintText(scoretext, wcslen(scoretext));
}


//-----------------------------------------------------------------------------
//	scale map time to 90mins.
//
//
//-----------------------------------------------------------------------------
int CHudLogo::GetScaledTime(void)
{
	C_SDKGameRules *pGR = (C_SDKGameRules*)g_pGameRules;
	if (!pGR)
		return 0;

	if (!pGR->m_iDuration)
		return pGR->GetMapTime();

	float scaledTime = 0;

	float timeleft = pGR->GetMapRemainingTime();

	if (timeleft)
		scaledTime = (90*60) - (timeleft) * ((90*60)/pGR->m_iDuration);

	if (scaledTime > (90*60))
		return (90*60);

	if (scaledTime < 0.0f)
		scaledTime = 0.0f;

	return scaledTime;
}