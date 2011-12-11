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
	void MsgFunc_MatchEvent(bf_read &msg);

protected:
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

private:
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );
	CPanelAnimationVarAliasType( float, text_xpos, "text_xpos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, text_ypos, "text_ypos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, text_ygap, "text_ygap", "14", "proportional_float" );

	Panel *m_pTopPanel;
	Panel *m_pBottomPanel;
	float m_flNotificationTime;
	Label *m_pEventLabel;
	Label *m_pScorebarLabel;
};

DECLARE_HUDELEMENT( CHudScorebar );
DECLARE_HUD_MESSAGE(CHudScorebar, MatchEvent);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudScorebar::CHudScorebar( const char *pElementName ) : BaseClass(NULL, "HudScorebar"), CHudElement( pElementName )
{
	SetHiddenBits(HIDEHUD_PLAYERDEAD);
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pTopPanel = new Panel(this, "TopPanel");
	m_pBottomPanel = new Panel(this, "BottomPanel");
	m_pEventLabel = new Label(m_pBottomPanel, "EventLabel", "");
	m_pScorebarLabel = new Label(m_pTopPanel, "ScorebarLabel", "");
	m_flNotificationTime = -1;
}

void CHudScorebar::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	
	SetPos( 25, 25);
 	SetSize( 800, 400 );
 	//SetPaintBackgroundType (2); // Rounded corner box
 	//SetPaintBackgroundEnabled(true);
	//SetBgColor( Color( 0, 0, 255, 255 ) );

	m_pTopPanel->SetBounds(0, 0, 600, 50);
	m_pTopPanel->SetPaintBackgroundEnabled(true);
	m_pTopPanel->SetPaintBackgroundType(2);
	m_pTopPanel->SetBgColor(Color(0, 0, 0, 255));

	m_pBottomPanel->SetBounds(0, -110, 600, 100);
	m_pBottomPanel->SetPaintBackgroundEnabled(true);
	m_pBottomPanel->SetPaintBackgroundType(2);
	//m_pBottomPanel->SetPaintBorderEnabled(true);
	m_pBottomPanel->SetBgColor(Color(0, 0, 0, 150));
	//m_pBottomPanel->SetVisible(false);
	m_pBottomPanel->SetZPos(-1);

	m_pEventLabel->SetBounds(20, 20, 500, 50);
	m_pEventLabel->SetFont(scheme->GetFont("IOSMatchEvent"));

	m_pScorebarLabel->SetBounds(10, 10, 500, 30);
	m_pScorebarLabel->SetFont(scheme->GetFont("IOSScorebar"));
	m_pScorebarLabel->SetFgColor(Color(255, 255, 255, 255));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::Init( void )
{
	HOOK_HUD_MESSAGE(CHudScorebar, MatchEvent);
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
	//wchar_t prefix[16] = L"";

	float flTime = gpGlobals->curtime - SDKGameRules()->m_flStateEnterTime;
	int nTime = 0;

	switch ( SDKGameRules()->m_eMatchState )
	{
	case MATCH_EXTRATIME_SECOND_HALF: case MATCH_EXTRATIME_SECOND_HALF_INJURY_TIME:
		nTime = ( int )( flTime * ( 90.0f / mp_timelimit_match.GetInt() ) ) + (90 + 15) * 60;
		_snwprintf( time, ARRAYSIZE( time ), L"EX2 % 3d:%02d", nTime / 60, nTime % 60 );
		break;
	case MATCH_EXTRATIME_FIRST_HALF: case MATCH_EXTRATIME_FIRST_HALF_INJURY_TIME:
		nTime = ( int )( flTime * ( 90.0f / mp_timelimit_match.GetInt() ) ) + 90 * 60;
		_snwprintf( time, ARRAYSIZE( time ), L"EX1 % 3d:%02d", nTime / 60, nTime % 60 );
		break;
	case MATCH_SECOND_HALF: case MATCH_SECOND_HALF_INJURY_TIME:
		nTime = ( int )( flTime * ( 90.0f / mp_timelimit_match.GetInt() ) ) + 45 * 60;
		_snwprintf( time, ARRAYSIZE( time ), L"H2 % 3d:%02d", nTime / 60, nTime % 60 );
		break;
	case MATCH_FIRST_HALF: case MATCH_FIRST_HALF_INJURY_TIME:
		nTime = ( int )( flTime * ( 90.0f / mp_timelimit_match.GetInt() ) );
		_snwprintf( time, ARRAYSIZE( time ), L"H1 % 3d:%02d", nTime / 60, nTime % 60 );
		break;
	case MATCH_WARMUP:
		_snwprintf( time, ARRAYSIZE( time ), L"WARMUP" );
		break;
	case MATCH_HALFTIME:
		_snwprintf( time, ARRAYSIZE( time ), L"HALFTIME" );
		break;
	case MATCH_EXTRATIME_INTERMISSION:
		_snwprintf( time, ARRAYSIZE( time ), L"EX INTERM" );
		break;
	case MATCH_EXTRATIME_HALFTIME:
		_snwprintf( time, ARRAYSIZE( time ), L"EX HALFTIME" );
		break;
	case MATCH_PENALTIES_INTERMISSION:
		_snwprintf( time, ARRAYSIZE( time ), L"PEN INTERM" );
		break;
	case MATCH_PENALTIES:
		_snwprintf( time, ARRAYSIZE( time ), L"PENALTIES" );
		break;
	case MATCH_COOLDOWN:
		_snwprintf( time, ARRAYSIZE( time ), L"COOLDOWN" );
		break;
	default:
		_snwprintf( time, ARRAYSIZE( time ), L"" );
		break;
	}

	//_snwprintf(time, sizeof(time), L"%d", (int)();
	wchar_t scoreHome[3];
	_snwprintf(scoreHome, sizeof(scoreHome), L"%d", teamHome->Get_Score());
	wchar_t scoreAway[3];
	_snwprintf(scoreAway, sizeof(scoreAway), L"%d", teamAway->Get_Score());
	g_pVGuiLocalize->ConvertANSIToUnicode( teamHome->Get_Name(), teamHomeName, sizeof( teamHomeName ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( teamAway->Get_Name(), teamAwayName, sizeof( teamAwayName ) );

	//surface()->DrawSetColor(0, 0, 0, 150);
	//surface()->DrawFilledRect(0, 0, 500, 50);
	//m_pTopPanel->DrawBox(0, 0, 50, 50, Color(255, 0, 0, 200), 1.0f, false);
	//m_pBottomPanel->DrawBox(0, 0, 50, 50, Color(0, 255, 0, 200), 1.0f, false);

	DrawText( 0, 0, m_hTextFont, GameResources()->GetTeamColor(2), teamHomeName);
	DrawText( 100, 0, m_hTextFont, GameResources()->GetTeamColor(0), scoreHome);
	DrawText( 200, 0, m_hTextFont, GameResources()->GetTeamColor(0), time);
	DrawText( 300, 0, m_hTextFont, GameResources()->GetTeamColor(0), scoreAway);
	DrawText( 400, 0, m_hTextFont, GameResources()->GetTeamColor(3), teamAwayName);

	if (SDKGameRules()->m_nAnnouncedInjuryTime > 0)
	{
		surface()->DrawSetColor(0, 0, 0, 150);
		//surface()->DrawFilledRect(10, 60, 110, 110);
		surface()->DrawSetTextPos(15, 65);
		wchar_t announcedInjuryTime[8];
		_snwprintf(announcedInjuryTime, sizeof(announcedInjuryTime), L"+%d", SDKGameRules()->m_nAnnouncedInjuryTime);
		surface()->DrawUnicodeString(announcedInjuryTime);
	}

	wchar_t scorebarText[64];

	_snwprintf(scorebarText, sizeof(scorebarText), L"%s | %s %s - %s %s", time, teamHomeName, scoreHome, scoreAway, teamAwayName);

	m_pScorebarLabel->SetText(scorebarText);

	if (m_flNotificationTime != -1)
	{
		float timeleft = m_flNotificationTime - gpGlobals->curtime;
		if (timeleft >= 4)
		{
			m_pBottomPanel->SetPos(0, 150 * (1 - (timeleft - 4) / 1) - 110);
		}
		else if (timeleft >= 1)
		{
			//float c = sin((1 - abs(timeleft - 1 - 1.5f) / 1.5f) * (M_PI*2));//(int)(timeleft * 10) % 2 == 0 ? 
			float c = 1 - (cos((1 - (timeleft - 1) / 3.0f) * (2*M_PI*5)) + 1) / 2.0f;//(int)(timeleft * 10) % 2 == 0 ? 
			m_pBottomPanel->SetBgColor(Color(c * 200, c * 200, c * 200, 150));
		}
		else if (timeleft >= 0)
		{
			m_pBottomPanel->SetPos(0, 150 * (1 - (1 - timeleft) / 1) - 110);
		}
		else
		{
			m_flNotificationTime = -1;
			//m_pBottomPanel->SetVisible(false);
			//m_pBottomPanel->SetBgColor(Color(0, 0, 0, 150));
		}
	}
}

void CHudScorebar::MsgFunc_MatchEvent(bf_read &msg)
{
	IGameResources *gr = GameResources();
	int eventType = msg.ReadByte();
	int playerIndex = msg.ReadByte();
	m_pEventLabel->SetText(VarArgs("GOAL (%s)", gr->GetPlayerName(playerIndex)));
	m_flNotificationTime = gpGlobals->curtime + 5;
	//m_pBottomPanel->SetPos(0, 0);
	//m_pBottomPanel->SetVisible(true);
}