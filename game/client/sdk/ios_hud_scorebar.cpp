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

using namespace vgui;

struct Event_t
{
	Label *pEventType;
	Label *pHomeTeam;
	Label *pAwayTeam;
	float startTime;
};

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
	void DoEventSlide();
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
	Label *m_pEventTypeLabels[3];
	Label *m_pEventTeamLabels[2][3];

	CUtlVector<Event_t> m_vEventList;

	float m_flNotificationTime;
	float m_flEventStart;
	bool m_bFlash;

	IScheme *pScheme;
	int m_nTargetHeight;
	int m_nTargetY;
	float m_flCurrentHeight;
	float m_flCurrentY;
};

DECLARE_HUDELEMENT( CHudScorebar );
DECLARE_HUD_MESSAGE(CHudScorebar, MatchEvent);

#define HEIGHT_SCOREBAR			40
#define HEIGHT_EVENTBAR			400
#define HEIGHT_OVERLAP			10
#define HEIGHT_EVENTLABEL		30
#define WIDTH_TEAMLABEL			220
#define WIDTH_TIME				150
#define WIDTH_SCORE				65
#define WIDTH_MARGIN			10
#define WIDTH_TEAMLABELMARGIN	5
#define WIDTH_SCOREBAR			WIDTH_MARGIN + WIDTH_TIME + WIDTH_MARGIN + WIDTH_TEAMLABEL + WIDTH_MARGIN + WIDTH_SCORE + WIDTH_MARGIN + WIDTH_TEAMLABEL + WIDTH_MARGIN

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
		for (int j = 0; j < 3; j++)
		{
			m_pEventTeamLabels[i][j] = new Label(m_pEventTeamPanels[i], VarArgs("PlayerLabel%d%d", i + 1, j + 1), "");
			if (i == 0)
				m_pEventTypeLabels[j] = new Label(m_pEventTypePanel, VarArgs("EventLabel%d", i + 1), "");
		}
	}
	
	m_pScorebarTimeLabel = new Label(m_pScorebarPanel, "ScorebarLabel", "");
	m_flNotificationTime = -1;
	m_flEventStart = -1;
	m_nTargetHeight = 0;
	m_nTargetY = 0;
	m_flCurrentHeight = 0.0f;
	m_flCurrentY = 0.0f;
}

void CHudScorebar::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	pScheme = scheme;
	
	SetPos( 25, 25);
	SetSize( WIDTH_SCOREBAR, 500 );
 	//SetPaintBackgroundType (2); // Rounded corner box
 	//SetPaintBackgroundEnabled(true);
	//SetBgColor( Color( 0, 0, 255, 255 ) );

	Color bgColor = Color(0, 0, 0, 255);
	Color bgColorTransparent = Color(0, 0, 0, 200);

	m_pScorebarPanel->SetBounds(0, 0, WIDTH_SCOREBAR, HEIGHT_SCOREBAR);
	m_pScorebarPanel->SetPaintBackgroundEnabled(true);
	m_pScorebarPanel->SetPaintBackgroundType(2);
	m_pScorebarPanel->SetBgColor(bgColor);

	m_pScorebarTimeLabel->SetBounds(WIDTH_MARGIN, 0, WIDTH_TIME, HEIGHT_SCOREBAR);
	//m_pScorebarTimeLabel->SetContentAlignment(Label::a_center);
	m_pScorebarTimeLabel->SetFont(scheme->GetFont("IOSScorebar"));
	m_pScorebarTimeLabel->SetFgColor(Color(255, 255, 255, 255));

	m_pScorebarScoreLabel->SetBounds(WIDTH_MARGIN + WIDTH_TIME + WIDTH_MARGIN + WIDTH_TEAMLABEL + WIDTH_MARGIN, 0, WIDTH_SCORE, HEIGHT_SCOREBAR);
	m_pScorebarScoreLabel->SetContentAlignment(Label::a_center);
	m_pScorebarScoreLabel->SetFont(scheme->GetFont("IOSScorebar"));
	m_pScorebarScoreLabel->SetFgColor(Color(255, 255, 255, 255));

	m_pEventPanel->SetBounds(0, HEIGHT_SCOREBAR - HEIGHT_OVERLAP, WIDTH_SCOREBAR, HEIGHT_EVENTBAR + HEIGHT_OVERLAP);
	m_pEventPanel->SetZPos(-1);

	m_pEventTypePanel->SetPaintBackgroundEnabled(true);
	m_pEventTypePanel->SetPaintBackgroundType(2);
	m_pEventTypePanel->SetBgColor(bgColorTransparent);
	m_pEventTypePanel->SetBounds(WIDTH_MARGIN, 0, WIDTH_TIME, HEIGHT_OVERLAP);

	for (int i = 0; i < 2; i++)
	{
		m_pScorebarTeamLabels[i]->SetBounds(WIDTH_MARGIN + WIDTH_TIME + WIDTH_MARGIN + (i == 0 ? 0 : WIDTH_TEAMLABEL + WIDTH_MARGIN + WIDTH_SCORE + WIDTH_MARGIN), WIDTH_TEAMLABELMARGIN, WIDTH_TEAMLABEL, HEIGHT_SCOREBAR - 2 * WIDTH_TEAMLABELMARGIN);
		m_pScorebarTeamLabels[i]->SetContentAlignment(Label::a_center);
		m_pScorebarTeamLabels[i]->SetTextInset(0, 0);
		//m_pScorebarTeamLabels[i]->SetAutoResize(Panel::PIN_TOPLEFT, AUTORESIZE_RIGHT, 10, 0, 10, 0);
		m_pScorebarTeamLabels[i]->SetFont(scheme->GetFont("IOSScorebar"));
		m_pScorebarTeamLabels[i]->SetPaintBackgroundType(2);
		m_pScorebarTeamLabels[i]->SetBgColor(Color(100 * i, 100 * (1 - i), 0, 150));
		m_pScorebarTeamLabels[i]->SetFgColor(Color(255, 255, 255, 255));

		m_pEventTeamPanels[i]->SetBounds(WIDTH_MARGIN + WIDTH_TIME + WIDTH_MARGIN + (i == 0 ? 0 : WIDTH_TEAMLABEL + WIDTH_MARGIN + WIDTH_SCORE + WIDTH_MARGIN), 0, WIDTH_TEAMLABEL, HEIGHT_OVERLAP);
		m_pEventTeamPanels[i]->SetPaintBackgroundEnabled(true);
		m_pEventTeamPanels[i]->SetPaintBackgroundType(2);
		m_pEventTeamPanels[i]->SetBgColor(bgColorTransparent);

		for (int j = 0; j < 3; j++)
		{
			m_pEventTeamLabels[i][j]->SetBounds(0, HEIGHT_OVERLAP + j * HEIGHT_EVENTLABEL, WIDTH_TEAMLABEL, HEIGHT_EVENTLABEL);
			m_pEventTeamLabels[i][j]->SetFgColor(Color(255, 255, 255, 255));
			m_pEventTeamLabels[i][j]->SetFont(scheme->GetFont("IOSTeamEvent"));
			m_pEventTeamLabels[i][j]->SetContentAlignment(Label::a_center);
			m_pEventTeamLabels[i][j]->SetVisible(false);

			if (i == 0)
			{
				m_pEventTypeLabels[j]->SetFont(scheme->GetFont("IOSMatchEvent"));
				m_pEventTypeLabels[j]->SetContentAlignment(Label::a_center);
				m_pEventTypeLabels[j]->SetBounds(0, HEIGHT_OVERLAP + j * HEIGHT_EVENTLABEL, WIDTH_TIME, HEIGHT_EVENTLABEL);
				m_pEventTypeLabels[j]->SetFgColor(Color(255, 255, 255, 255));
				m_pEventTypeLabels[j]->SetVisible(false);
			}
		}
	}

	m_nTargetHeight = HEIGHT_OVERLAP;
	m_nTargetY = 0;

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
	"Ex Halftime",
	"Ex 2nd Half",
	"Ex 2nd Half",
	"Penalties",
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
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat())) + (90 + 15) * 60;
		break;
	case MATCH_EXTRATIME_FIRST_HALF: case MATCH_EXTRATIME_FIRST_HALF_INJURY_TIME:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat())) + 90 * 60;
		break;
	case MATCH_SECOND_HALF: case MATCH_SECOND_HALF_INJURY_TIME:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat())) + 45 * 60;
		break;
	case MATCH_FIRST_HALF: case MATCH_FIRST_HALF_INJURY_TIME:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat()));
		break;
	case MATCH_WARMUP:
		nTime = (int)(flTime - mp_timelimit_warmup.GetFloat() * 60);
		break;
	case MATCH_HALFTIME:
		nTime = (int)(flTime - mp_timelimit_halftime.GetFloat() * 60);
		break;
	case MATCH_EXTRATIME_INTERMISSION:
		nTime = (int)(flTime - mp_timelimit_extratime_intermission.GetFloat() * 60);
		break;
	case MATCH_EXTRATIME_HALFTIME:
		nTime = (int)(flTime - mp_timelimit_extratime_halftime.GetFloat() * 60);
		break;
	case MATCH_PENALTIES_INTERMISSION:
		nTime = (int)(flTime - mp_timelimit_penalties_intermission.GetFloat() * 60);
		break;
	case MATCH_PENALTIES:
		nTime = (int)(flTime - mp_timelimit_penalties.GetFloat() * 60);
		break;
	case MATCH_COOLDOWN:
		nTime = (int)(flTime - mp_timelimit_cooldown.GetFloat() * 60);
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

	DoEventSlide();
}

void CHudScorebar::DoEventSlide()
{
	if (m_vEventList.Count() == 0)
	{
		for (int i = 0; i < m_pEventPanel->GetChildCount(); i++)
		{
			m_nTargetHeight = m_flCurrentHeight = HEIGHT_OVERLAP;
			m_nTargetY = m_flCurrentY = 0;
			m_pEventPanel->GetChild(i)->SetTall(m_flCurrentHeight);
			m_pEventPanel->GetChild(i)->SetY(m_flCurrentY);
			return;
		}
	}

	for (int i = 0; i < m_vEventList.Count(); i++)
	{
		Event_t *event = &m_vEventList[i];

		int alpha = 255;

		if (event->pEventType->GetY() + event->pEventType->GetTall() < m_pEventTypePanel->GetTall())
		{
			int alpha = event->pEventType->GetFgColor().a() + 100 * gpGlobals->frametime;
		}
		event->pEventType->SetFgColor(Color(255, 255, 255, alpha));
		event->pHomeTeam->SetFgColor(Color(255, 255, 255, alpha));
		event->pAwayTeam->SetFgColor(Color(255, 255, 255, alpha));

		if (event->startTime == -1)
		{
			if (m_pEventTypePanel->GetY() + event->pHomeTeam->GetY() + event->pHomeTeam->GetTall() <= HEIGHT_OVERLAP)
			{
				delete event->pEventType;
				delete event->pHomeTeam;
				delete event->pAwayTeam;
				m_vEventList.Remove(0);
			}
		}
		else if (gpGlobals->curtime - event->startTime <= 1)
		{
	/*		int alpha = 255 * (gpGlobals->curtime - event->startTime);
			event->pEventType->SetFgColor(Color(255, 255, 255, alpha));
			event->pHomeTeam->SetFgColor(Color(255, 255, 255, alpha));
			event->pAwayTeam->SetFgColor(Color(255, 255, 255, alpha));*/
		}
		else if (gpGlobals->curtime - event->startTime >= 6)
		{
			m_nTargetY -= HEIGHT_EVENTLABEL;
			event->startTime = -1;
		/*	if (height == HEIGHT_OVERLAP)
			{
				delete event.pEventType;
				delete event.pHomeTeam;
				delete event.pAwayTeam;
				m_vEventList.Remove(0);
			}*/
		}
	}

	for (int i = 0; i < m_pEventPanel->GetChildCount(); i++)
	{		
		//if (heightDiff > 0)
		//	m_pEventPanel->GetChild(i)->SetTall(m_pEventPanel->GetChild(i)->GetTall() + heightDiff);
		//else if (heightDiff < 0)
		//	m_pEventPanel->GetChild(i)->SetY(m_pEventPanel->GetChild(i)->GetY() + heightDiff);
		if (m_nTargetHeight > m_flCurrentHeight)
		{
			m_flCurrentHeight += 10 * gpGlobals->frametime;
			m_pEventPanel->GetChild(i)->SetTall(m_flCurrentHeight);
		}
		if (m_nTargetY < m_flCurrentY)
		{
			m_flCurrentY -= 10 * gpGlobals->frametime;
			m_pEventPanel->GetChild(i)->SetY(m_flCurrentY);
		}
	}
}

#include "ehandle.h"
#include "c_sdk_player.h"

void CHudScorebar::MsgFunc_MatchEvent(bf_read &msg)
{
	IGameResources *gr = GameResources();
	match_event_t eventType = (match_event_t)msg.ReadByte();
	int playerIndices[] = { msg.ReadByte(), msg.ReadByte() };
	//C_SDKPlayer *pPlayer1 = (C_SDKPlayer *)CHandle<C_SDKPlayer>::FromIndex(msg.ReadLong());
	//C_SDKPlayer *pPlayer2 = (C_SDKPlayer *)CHandle<C_SDKPlayer>::FromIndex(msg.ReadLong());

	Label *pEventType = new Label(m_pEventTypePanel, "EventTypeLabel", g_szMatchEventNames[eventType]);
	pEventType->SetFont(pScheme->GetFont("IOSMatchEvent"));
	pEventType->SetContentAlignment(Label::a_center);
	pEventType->SetFgColor(Color(255, 255, 255, 0));
	pEventType->SetBounds(0, HEIGHT_OVERLAP + m_vEventList.Count() * HEIGHT_EVENTLABEL, WIDTH_TIME, HEIGHT_EVENTLABEL);
	//pEventType->SetAlpha(0);

	Label *pTeams[2];
	for (int i = 0; i < 2; i++)
	{
		pTeams[i] = new Label(m_pEventTeamPanels[i], VarArgs("TeamLabel%d", i + 1), playerIndices[i] == 0 ? "" : gr->GetPlayerName(playerIndices[i]));
		pTeams[i]->SetBounds(0, HEIGHT_OVERLAP + m_vEventList.Count() * HEIGHT_EVENTLABEL, WIDTH_TEAMLABEL, HEIGHT_EVENTLABEL);
		pTeams[i]->SetFgColor(Color(255, 255, 255, 0));
		pTeams[i]->SetFont(pScheme->GetFont("IOSTeamEvent"));
		pTeams[i]->SetContentAlignment(Label::a_center);
		//pTeams[i]->SetAlpha(0);
	}

	float startTime = gpGlobals->curtime;

	Event_t e = { pEventType, pTeams[0], pTeams[1], startTime };
	m_vEventList.AddToTail(e);

	m_nTargetHeight += HEIGHT_EVENTLABEL;

	m_flEventStart = gpGlobals->curtime;
	m_flNotificationTime = gpGlobals->curtime + 5;
	m_bFlash = eventType == MATCH_EVENT_GOAL ? true : false;
}