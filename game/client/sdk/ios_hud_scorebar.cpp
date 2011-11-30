//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// battery.cpp
//
// implementation of CHudScorebar class
//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "iclientmode.h"
#include <vgui_controls/controls.h>
#include <vgui_controls/panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <game_controls/baseviewport.h>
#include "clientmode_shared.h"
#include "c_baseplayer.h"
#include "c_team.h"

#include "sdk_gamerules.h"

#include "UtlVector.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_BAT	-1

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Displays suit power (armor) on hud
//-----------------------------------------------------------------------------
class CHudScorebar : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudScorebar, vgui::Panel );

public:
	CHudScorebar( const char *pElementName );
	void Init( void );
	void Reset( void );
	void VidInit( void );
	void DrawText( int x, int y, HFont hFont, Color clr, const wchar_t *szText );

protected:
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

private:
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );
	CPanelAnimationVarAliasType( float, text_xpos, "text_xpos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, text_ypos, "text_ypos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, text_ygap, "text_ygap", "14", "proportional_float" );
};

DECLARE_HUDELEMENT( CHudScorebar );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudScorebar::CHudScorebar( const char *pElementName ) : BaseClass(NULL, "HudScorebar"), CHudElement( pElementName )
{
	SetHiddenBits(HIDEHUD_PLAYERDEAD);
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

void CHudScorebar::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	//DevMsg("paint!\n");
	SetPos( 25, 25);
 	SetSize( 600, 150 );
 	SetPaintBackgroundType (2); // Rounded corner box
 	SetPaintBackgroundEnabled( true );
	SetBgColor( Color( 0, 0, 0, 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::Init( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::Reset( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::VidInit( void )
{
	Reset();
}

void CHudScorebar::DrawText( int x, int y, HFont hFont, Color clr, const wchar_t *szText )
{
	surface()->DrawSetTextPos( x, y );
	surface()->DrawSetTextColor( clr );
	surface()->DrawSetTextFont( hFont );	//reset the font, draw icon can change it
	surface()->DrawUnicodeString( szText, vgui::FONT_DRAW_NONADDITIVE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::Paint( void )
{
	//wchar_t *team1Name = NULL;
	//wchar_t *team2Name = NULL;
	C_Team *teamHome = GetGlobalTeam( 2 );
	C_Team *teamAway = GetGlobalTeam( 3 );
	if ( !teamHome || !teamAway )
		return;

	wchar_t teamHomeName[64];
	wchar_t teamAwayName[64];
	wchar_t time[64];
	wchar_t prefix[16] = L"";

	float flTime = gpGlobals->curtime - SDKGameRules()->m_flStateEnterTime;
	int nTime;

	switch ( SDKGameRules()->m_eMatchState )
	{
	//// Fall through to subtract all previous durations
	//case MATCH_EXTRATIME_SECOND_HALF: case MATCH_EXTRATIME_SECOND_HALF_INJURY_TIME:
	//	flTime -= mp_timelimit_extratime_halftime.GetInt() * 60;
	//	if (wcslen(prefix) == 0) _snwprintf(prefix, sizeof(prefix), L"EX2");
	//case MATCH_EXTRATIME_FIRST_HALF: case MATCH_EXTRATIME_FIRST_HALF_INJURY_TIME:
	//	flTime -= mp_timelimit_extratime_intermission.GetInt() * 60;
	//	if (wcslen(prefix) == 0) _snwprintf(prefix, sizeof(prefix), L"EX1");
	//case MATCH_SECOND_HALF: case MATCH_SECOND_HALF_INJURY_TIME:
	//	flTime -= mp_timelimit_halftime.GetInt() * 60;
	//	if (wcslen(prefix) == 0) _snwprintf(prefix, sizeof(prefix), L"H2");
	//case MATCH_FIRST_HALF: case MATCH_FIRST_HALF_INJURY_TIME:
	//	flTime -= mp_timelimit_warmup.GetInt() * 60;
	//	if (wcslen(prefix) == 0) _snwprintf(prefix, sizeof(prefix), L"H1");

	//	nTime = ( int )( flTime * ( 90.0f / mp_timelimit_match.GetInt() ) );
	//	_snwprintf( time, ARRAYSIZE( time ), L"%s % 3d:%02d", prefix, nTime / 60, nTime % 60 );
	//	break;
	//// End fall through

	case MATCH_EXTRATIME_SECOND_HALF: case MATCH_EXTRATIME_SECOND_HALF_INJURY_TIME:
		if (wcslen(prefix) == 0) _snwprintf(prefix, sizeof(prefix), L"EX2");
	case MATCH_EXTRATIME_FIRST_HALF: case MATCH_EXTRATIME_FIRST_HALF_INJURY_TIME:
		if (wcslen(prefix) == 0) _snwprintf(prefix, sizeof(prefix), L"EX1");
	case MATCH_SECOND_HALF: case MATCH_SECOND_HALF_INJURY_TIME:
		if (wcslen(prefix) == 0) _snwprintf(prefix, sizeof(prefix), L"H2");
	case MATCH_FIRST_HALF: case MATCH_FIRST_HALF_INJURY_TIME:
		if (wcslen(prefix) == 0) _snwprintf(prefix, sizeof(prefix), L"H1");

		nTime = ( int )( flTime * ( 90.0f / mp_timelimit_match.GetInt() ) );
		_snwprintf( time, ARRAYSIZE( time ), L"%s % 3d:%02d", prefix, nTime / 60, nTime % 60 );
		break;

	case MATCH_WARMUP:
		_snwprintf( time, ARRAYSIZE( time ), L"WARMUP" );
		break;
	case MATCH_HALFTIME:
		_snwprintf( time, ARRAYSIZE( time ), L"HALFTIME" );
		break;
	case MATCH_EXTRATIME_INTERMISSION:
		_snwprintf( time, ARRAYSIZE( time ), L"EXTRATIME INTERMISSION" );
		break;
	case MATCH_EXTRATIME_HALFTIME:
		_snwprintf( time, ARRAYSIZE( time ), L"EXTRATIME HALFTIME" );
		break;
	case MATCH_PENALTIES_INTERMISSION:
		_snwprintf( time, ARRAYSIZE( time ), L"PENALTIES INTERMISSION" );
		break;
	case MATCH_PENALTIES:
		_snwprintf( time, ARRAYSIZE( time ), L"PENALTIES" );
		break;
	case MATCH_COOLDOWN:
		_snwprintf( time, ARRAYSIZE( time ), L"COOLDOWN" );
		break;
	}

	//_snwprintf(time, sizeof(time), L"%d", (int)();
	wchar_t scoreHome[3];
	_snwprintf(scoreHome, sizeof(scoreHome), L"%d", teamHome->Get_Score());
	wchar_t scoreAway[3];
	_snwprintf(scoreAway, sizeof(scoreAway), L"%d", teamAway->Get_Score());
	g_pVGuiLocalize->ConvertANSIToUnicode( teamHome->Get_Name(), teamHomeName, sizeof( teamHomeName ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( teamAway->Get_Name(), teamAwayName, sizeof( teamAwayName ) );

	surface()->DrawSetColor(0, 0, 0, 150);
	surface()->DrawFilledRect(0, 0, 500, 50);

	DrawText( 0, 0, m_hTextFont, GameResources()->GetTeamColor(2), teamHomeName);
	DrawText( 100, 0, m_hTextFont, GameResources()->GetTeamColor(0), scoreHome);
	DrawText( 200, 0, m_hTextFont, GameResources()->GetTeamColor(0), time);
	DrawText( 300, 0, m_hTextFont, GameResources()->GetTeamColor(0), scoreAway);
	DrawText( 400, 0, m_hTextFont, GameResources()->GetTeamColor(3), teamAwayName);

	if (SDKGameRules()->m_nAnnouncedInjuryTime > 0)
	{
		surface()->DrawSetColor(0, 0, 0, 150);
		surface()->DrawFilledRect(10, 60, 110, 110);
		surface()->DrawSetTextPos(15, 65);
		wchar_t announcedInjuryTime[8];
		_snwprintf(announcedInjuryTime, sizeof(announcedInjuryTime), L"+%d", SDKGameRules()->m_nAnnouncedInjuryTime);
		surface()->DrawUnicodeString(announcedInjuryTime);
	}
}