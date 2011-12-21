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
	void ShowMatchEvent();
	void MsgFunc_MatchEvent(bf_read &msg);

protected:
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

private:

	Panel *m_pScorebarPanel;
	Label *m_pScorebarTimeLabel;
	Label *m_pScorebarTeamLabels[2];
	Label *m_pScorebarScoreLabel;

	Panel *m_pEventPanel;
	Panel *m_pEventTypePanel;
	Panel *m_pEventTeamPanels[2];
	Label *m_pEventTypeLabel;
	Label *m_pEventTeamLabels[2];

	float m_flNotificationTime;
	float m_flEventStart;
	bool m_bFlash;
};

DECLARE_HUDELEMENT( CHudScorebar );
DECLARE_HUD_MESSAGE(CHudScorebar, MatchEvent);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudScorebar::CHudScorebar( const char *pElementName ) : BaseClass(NULL, "HudScorebar"), CHudElement( pElementName )
{
	//SetHiddenBits(HIDEHUD_PLAYERDEAD);
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pScorebarPanel = new Panel(this, "TopPanel");
	m_pScorebarTimeLabel = new Label(m_pScorebarPanel, "TimeLabel", "");
	m_pScorebarScoreLabel = new Label(m_pScorebarPanel, "ScoreLabel", "");
	m_pEventPanel = new Panel(this, "BottomPanel");
	m_pEventTypePanel = new Panel(m_pEventPanel, "EventPanel");
	for (int i = 0; i < 2; i++)
	{
		m_pScorebarTeamLabels[i] = new Label(m_pScorebarPanel, VarArgs("TeamLabel%d", i), "");
		m_pEventTeamPanels[i] = new Panel(m_pEventPanel, VarArgs("TeamPanel%d", i + 1));
		m_pEventTeamLabels[i] = new Label(m_pEventTeamPanels[i], VarArgs("PlayerLabel%d", i + 1), "");
	}
	m_pEventTypeLabel = new Label(m_pEventTypePanel, "EventLabel", "");
	m_pScorebarTimeLabel = new Label(m_pScorebarPanel, "ScorebarLabel", "");
	m_flNotificationTime = -1;
	m_flEventStart = -1;
}

void CHudScorebar::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	
	SetPos( 25, 25);
 	SetSize( 1000, 400 );
 	//SetPaintBackgroundType (2); // Rounded corner box
 	//SetPaintBackgroundEnabled(true);
	//SetBgColor( Color( 0, 0, 255, 255 ) );

	m_pScorebarPanel->SetBounds(0, 0, 1000, 50);
	m_pScorebarPanel->SetPaintBackgroundEnabled(true);
	m_pScorebarPanel->SetPaintBackgroundType(2);
	m_pScorebarPanel->SetBgColor(Color(0, 0, 0, 255));

	m_pScorebarTimeLabel->SetBounds(0, 0, 200, 50);
	m_pScorebarTimeLabel->SetContentAlignment(Label::a_center);
	m_pScorebarTimeLabel->SetFont(scheme->GetFont("IOSScorebar"));

	m_pScorebarScoreLabel->SetBounds(500, 0, 100, 50);
	m_pScorebarScoreLabel->SetContentAlignment(Label::a_center);
	m_pScorebarScoreLabel->SetFont(scheme->GetFont("IOSScorebar"));

	m_pEventPanel->SetBounds(0, 40 - 60, 1000, 60);
	m_pEventPanel->SetZPos(-1);

	m_pEventTypePanel->SetPaintBackgroundEnabled(true);
	m_pEventTypePanel->SetPaintBackgroundType(2);
	m_pEventTypePanel->SetBgColor(Color(0, 0, 0, 150));
	m_pEventTypePanel->SetBounds(0, 0, 200, 60);

	for (int i = 0; i < 2; i++)
	{
		m_pScorebarTeamLabels[i]->SetBounds(i * 400 + 250, 5, 200, 40);
		m_pScorebarTeamLabels[i]->SetContentAlignment(Label::a_center);
		m_pScorebarTeamLabels[i]->SetTextInset(10, 0);
		//m_pScorebarTeamLabels[i]->SetAutoResize(Panel::PIN_TOPLEFT, AUTORESIZE_RIGHT, 10, 0, 10, 0);
		m_pScorebarTeamLabels[i]->SetFont(scheme->GetFont("IOSScorebar"));
		m_pScorebarTeamLabels[i]->SetPaintBackgroundType(2);
		m_pScorebarTeamLabels[i]->SetBgColor(Color(100 * i, 100 * (1 - i), 0, 150));

		m_pEventTeamPanels[i]->SetBounds(i * 400 + 250, 0, 200, 60);
		m_pEventTeamPanels[i]->SetPaintBackgroundEnabled(true);
		m_pEventTeamPanels[i]->SetPaintBackgroundType(2);
		m_pEventTeamPanels[i]->SetBgColor(Color(0, 0, 0, 150));

		m_pEventTeamLabels[i]->SetBounds(0, 10, 200, 50);
		m_pEventTeamLabels[i]->SetFgColor(Color(255, 255, 255, 255));
		m_pEventTeamLabels[i]->SetFont(scheme->GetFont("IOSTeamEvent"));
		m_pEventTeamLabels[i]->SetContentAlignment(Label::a_center);
	}

	m_pEventTypeLabel->SetBounds(0, 10, 200, 50);
	m_pEventTypeLabel->SetFont(scheme->GetFont("IOSMatchEvent"));
	m_pEventTypeLabel->SetContentAlignment(Label::a_center);

	//m_pScorebarTimeLabel->SetBounds(10, 10, 500, 30);
	//m_pScorebarTimeLabel->SetFont(scheme->GetFont("IOSScorebar"));
	//m_pScorebarTimeLabel->SetFgColor(Color(255, 255, 255, 255));
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

const char *g_szStateNames[32] =
{
	"Init",
	"Warmup",
	"1st Half",
	"1st Half",
	"Halftime",
	"2nd Half",
	"2nd Half",
	"Extratime",
	"Ex 1st Half",
	"Ex 1st Half",
	"Ex Halftime"
	"Ex 2nd Half",
	"Ex 2nd Half",
	"Penalties"
	"Penalties",
	"Cooldown",
	"End"
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::Paint( void )
{
	C_Team *teamHome = GetGlobalTeam( 2 );
	C_Team *teamAway = GetGlobalTeam( 3 );
	if ( !teamHome || !teamAway )
		return;

	wchar_t teamHomeName[64];
	wchar_t teamAwayName[64];
	//wchar_t curTime[64];

	float flTime = gpGlobals->curtime - SDKGameRules()->m_flStateEnterTime;
	int nTime;

	//_snwprintf(curTime, sizeof(curTime), L"%s", g_szStateNames[SDKGameRules()->m_eMatchState]

	switch ( SDKGameRules()->m_eMatchState )
	{
	case MATCH_EXTRATIME_SECOND_HALF: case MATCH_EXTRATIME_SECOND_HALF_INJURY_TIME:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetInt())) + (90 + 15) * 60;
		break;
	case MATCH_EXTRATIME_FIRST_HALF: case MATCH_EXTRATIME_FIRST_HALF_INJURY_TIME:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetInt())) + 90 * 60;
		break;
	case MATCH_SECOND_HALF: case MATCH_SECOND_HALF_INJURY_TIME:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetInt())) + 45 * 60;
		break;
	case MATCH_FIRST_HALF: case MATCH_FIRST_HALF_INJURY_TIME:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetInt()));
		break;
	case MATCH_WARMUP:
		nTime = (int)(flTime - mp_timelimit_warmup.GetInt() * 60);
		break;
	case MATCH_HALFTIME:
		nTime = (int)(flTime - mp_timelimit_halftime.GetInt() * 60);
		break;
	case MATCH_EXTRATIME_INTERMISSION:
		nTime = (int)(flTime - mp_timelimit_extratime_intermission.GetInt() * 60);
		break;
	case MATCH_EXTRATIME_HALFTIME:
		nTime = (int)(flTime - mp_timelimit_extratime_halftime.GetInt() * 60);
		break;
	case MATCH_PENALTIES_INTERMISSION:
		nTime = (int)(flTime - mp_timelimit_penalties_intermission.GetInt() * 60);
		break;
	case MATCH_PENALTIES:
		nTime = (int)(flTime - mp_timelimit_penalties.GetInt() * 60);
		break;
	case MATCH_COOLDOWN:
		nTime = (int)(flTime - mp_timelimit_cooldown.GetInt() * 60);
		break;
	default:
		nTime = 0;
		break;
	}

	nTime = abs(nTime);

	char stateText[32];
	char *timeText = nTime > 0 ? VarArgs("% 3d:%02d", nTime / 60, nTime % 60) : VarArgs(" 3%d", nTime);
	Q_snprintf(stateText, sizeof(stateText), "%s %s", g_szStateNames[SDKGameRules()->m_eMatchState], timeText);

	//_snwprintf(time, sizeof(time), L"%d", (int)();
	wchar_t scoreHome[3];
	_snwprintf(scoreHome, sizeof(scoreHome), L"%d", teamHome->Get_Score());
	wchar_t scoreAway[3];
	_snwprintf(scoreAway, sizeof(scoreAway), L"%d", teamAway->Get_Score());
	g_pVGuiLocalize->ConvertANSIToUnicode( teamHome->Get_FullName(), teamHomeName, sizeof( teamHomeName ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( teamAway->Get_FullName(), teamAwayName, sizeof( teamAwayName ) );

	m_pScorebarTimeLabel->SetText(stateText);
	m_pScorebarTeamLabels[0]->SetText(teamHomeName);
	m_pScorebarTeamLabels[1]->SetText(teamAwayName);
	wchar_t scoreText[32];
	_snwprintf(scoreText, sizeof(scoreText), L"%s - %s", scoreHome, scoreAway);
	m_pScorebarScoreLabel->SetText(scoreText);

	if (m_flEventStart != -1)
		ShowMatchEvent();
}

#define SHOW_DURATION 5
#define FADE_DURATION 1
#define FLASH_COUNT 3

void CHudScorebar::ShowMatchEvent()
{
	float timeleft = m_flNotificationTime - gpGlobals->curtime;
	float timePassed = gpGlobals->curtime - m_flEventStart;

	if (timePassed <= FADE_DURATION)
	{
		//m_pEventPanel->SetPos(0, 150 * (1 - (timePassed - 4) / FADE_DURATION) - m_pEventPanel->GetTall());
		//m_pEventPanel->SetPos(0, m_pEventPanel->GetTall() * (timePassed / FADE_DURATION - 1) - 10);
		int pos = pow(cos((timePassed / FADE_DURATION) * M_PI / 2), 2) * -m_pEventPanel->GetTall() + 40;
		m_pEventPanel->SetPos(0, pos);
	}
	else if (timePassed <= FADE_DURATION + SHOW_DURATION)
	{
		//float c = sin((1 - abs(timeleft - 1 - 1.5f) / 1.5f) * (M_PI*2));//(int)(timeleft * 10) % 2 == 0 ? 
		float colorModifier;
		if (m_bFlash)
			//colorModifier = 1 - (cos((1 - (timeleft - 1) / 3.0f) * (2*M_PI*5)) + 1) / 2.0f;//(int)(timeleft * 10) % 2 == 0 ? 
			colorModifier = pow(cos(((timePassed - FADE_DURATION) / SHOW_DURATION - 0.5f) * M_PI * FLASH_COUNT), 2);
		else
			colorModifier = 0;

		m_pEventTypePanel->SetBgColor(Color(colorModifier * 200, colorModifier * 200, colorModifier * 200, 150));
	}
	else if (timePassed <= FADE_DURATION + SHOW_DURATION + FADE_DURATION)
	{
		int pos = pow(cos((1 - (timePassed - FADE_DURATION - SHOW_DURATION) / FADE_DURATION) * M_PI / 2), 2) * -m_pEventPanel->GetTall() + 40;
		m_pEventPanel->SetPos(0, pos);
	}
	else
	{
		m_flNotificationTime = -1;
		m_flEventStart = -1;
		//m_pEventPanel->SetVisible(false);
		//m_pEventPanel->SetBgColor(Color(0, 0, 0, 150));
	}
}

void CHudScorebar::MsgFunc_MatchEvent(bf_read &msg)
{
	IGameResources *gr = GameResources();
	match_event_t eventType = (match_event_t)msg.ReadByte();
	int playerIndex = msg.ReadByte();
	m_pEventTypeLabel->SetText(VarArgs("%s", g_szMatchEventNames[eventType]));
	char *club = VarArgs("%s", gr->GetPlayerName(playerIndex));
	m_pEventTeamLabels[0]->SetText(club);
	int wide, tall;
	m_pEventTeamLabels[0]->GetContentSize(wide, tall);
	//m_pEventTeamLabels[0]->SetBounds(m_pEventTeamLabels[0]->GetParent()->GetWide() / 2 - wide / 2, m_pEventTeamLabels[0]->GetParent()->GetTall() / 2 - tall / 2, wide, tall);
	m_flEventStart = gpGlobals->curtime;
	m_flNotificationTime = gpGlobals->curtime + 5;
	m_bFlash = eventType == MATCH_EVENT_GOAL ? true : false;
	//m_pEventPanel->SetPos(0, 0);
	//m_pEventPanel->SetVisible(true);
}