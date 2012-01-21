//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
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
	Panel *pEventBox;
	Label *pEventType;
	Label *pEventText;
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

	Panel *m_pTimeBar;
	Label *m_pTime;
	Label *m_pTeams[2];
	Label *m_pScores[2];
	Panel *m_pTeamBars[2];
	Panel *m_pEventBars[2];
	Label *m_pTeamColors[2];
	CUtlVector<Event_t> m_vEventLists[2];
};

DECLARE_HUDELEMENT( CHudScorebar );
DECLARE_HUD_MESSAGE(CHudScorebar, MatchEvent);

#define	HEIGHT_MARGIN			5
#define HEIGHT_TIMEBAR			35
#define HEIGHT_TEAMBAR			35
#define HEIGHT_EVENTBAR
#define BORDER					2

#define WIDTH_MARGIN			5
#define WIDTH_OVERLAP			10
#define WIDTH_EVENTTYPE			150
#define WIDTH_EVENTTEXT			100
#define WIDTH_TEAM				150
#define WIDTH_SCORE				30
#define WIDTH_TIME				150
#define WIDTH_TIMEBAR			150
#define WIDTH_TEAMCOLOR			15

#define WIDTH_TEAMBAR			BORDER + WIDTH_TEAMCOLOR + WIDTH_MARGIN + WIDTH_TEAM + WIDTH_MARGIN + WIDTH_SCORE + BORDER

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudScorebar::CHudScorebar( const char *pElementName ) : BaseClass(NULL, "HudScorebar"), CHudElement( pElementName )
{
	//SetHiddenBits(HIDEHUD_PLAYERDEAD);
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pTimeBar = new Panel(this, "TopPanel");
	m_pTime = new Label(m_pTimeBar, "TimeLabel", "");

	for (int i = 0; i < 2; i++)
	{
		m_pTeamBars[i] = new Panel(this, VarArgs("TeamPanel%d", i + 1));
		m_pTeams[i] = new Label(m_pTeamBars[i], VarArgs("TeamLabel%d", i), "");
		m_pTeamColors[i] = new Label(m_pTeamBars[i], VarArgs("TeamColor%d", i), "");
		m_pScores[i] = new Label(m_pTeamBars[i], VarArgs("ScoreLabel%d", i), "");
		m_pEventBars[i] = new Label(this, VarArgs("ScoreLabel%d", i), "");
	}
}

void CHudScorebar::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	SetPos( 20, 20);
	SetSize( ScreenWidth() - 50, 200 );
 	//SetPaintBackgroundType (2); // Rounded corner box
 	//SetPaintBackgroundEnabled(true);
	//SetBgColor( Color( 0, 0, 255, 255 ) );

	Color bgColor = Color(25, 25, 25, 255);
	Color bgColorTransparent = Color(25, 25, 25, 200);

	m_pTimeBar->SetBounds(0, 2 * (HEIGHT_TEAMBAR + HEIGHT_MARGIN), WIDTH_TIMEBAR, HEIGHT_TIMEBAR);
	m_pTimeBar->SetPaintBackgroundEnabled(true);
	m_pTimeBar->SetPaintBackgroundType(2);
	m_pTimeBar->SetBgColor(bgColor);

	m_pTime->SetBounds(WIDTH_MARGIN, BORDER, WIDTH_TIME, HEIGHT_TIMEBAR - 2 * BORDER);
	//m_pTime->SetContentAlignment(Label::a_center);
	m_pTime->SetFont(pScheme->GetFont("IOSScorebar"));
	m_pTime->SetFgColor(Color(255, 255, 255, 255));

	for (int i = 0; i < 2; i++)
	{		
		m_pTeamBars[i]->SetBounds(0, i * (HEIGHT_TEAMBAR + HEIGHT_MARGIN), WIDTH_TEAMBAR, HEIGHT_TEAMBAR);
		m_pTeamBars[i]->SetPaintBackgroundEnabled(true);
		m_pTeamBars[i]->SetPaintBackgroundType(2);
		m_pTeamBars[i]->SetBgColor(bgColor);

		m_pTeamColors[i]->SetBounds(BORDER, BORDER, WIDTH_TEAMCOLOR, HEIGHT_TEAMBAR - 2 * BORDER);
		m_pTeamColors[i]->SetBgColor(Color(100 * i, 100 * (1 - i), 0, 255));
		m_pTeamColors[i]->SetPaintBackgroundType(2);

		m_pTeams[i]->SetBounds(BORDER + WIDTH_TEAMCOLOR + WIDTH_MARGIN, BORDER, WIDTH_TEAM, HEIGHT_TEAMBAR - 2 * BORDER);
		m_pTeams[i]->SetContentAlignment(Label::a_west);
		m_pTeams[i]->SetTextInset(0, 0);
		//m_pTeams[i]->SetAutoResize(Panel::PIN_TOPLEFT, AUTORESIZE_RIGHT, 10, 0, 10, 0);
		m_pTeams[i]->SetFont(pScheme->GetFont("IOSScorebar"));
		m_pTeams[i]->SetPaintBackgroundType(2);
		//m_pTeams[i]->SetBgColor(Color(100 * i, 100 * (1 - i), 0, 255));
		m_pTeams[i]->SetFgColor(Color(255, 255, 255, 255));
		
		m_pScores[i]->SetBounds(BORDER + WIDTH_TEAMCOLOR + WIDTH_MARGIN + WIDTH_TEAM + WIDTH_MARGIN, BORDER, WIDTH_SCORE, HEIGHT_TEAMBAR - 2 * BORDER);
		m_pScores[i]->SetContentAlignment(Label::a_center);
		m_pScores[i]->SetTextInset(0, 0);
		//m_pScores[i]->SetAutoResize(Panel::PIN_TOPLEFT, AUTORESIZE_RIGHT, 10, 0, 10, 0);
		m_pScores[i]->SetFont(pScheme->GetFont("IOSScorebar"));
		m_pScores[i]->SetPaintBackgroundType(2);
		m_pScores[i]->SetBgColor(Color(255, 255, 255, 255));
		m_pScores[i]->SetFgColor(Color(0, 0, 0, 255));

		m_pEventBars[i]->SetBounds(WIDTH_TEAMBAR - WIDTH_OVERLAP, i * (HEIGHT_TEAMBAR + HEIGHT_MARGIN), 0, HEIGHT_TEAMBAR);
		m_pEventBars[i]->SetPaintBackgroundType(2);
		m_pEventBars[i]->SetBgColor(bgColorTransparent);
		m_pEventBars[i]->SetZPos(-1);
	}
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

	m_pTime->SetText(stateText);
	m_pTeams[0]->SetText(teamHomeName);
	m_pTeams[1]->SetText(teamAwayName);
	//wchar_t scoreText[32];
	//_snwprintf(scoreText, sizeof(scoreText), L"%s - %s", scoreHome, scoreAway);
	m_pScores[1]->SetText(scoreHome);
	m_pScores[0]->SetText(scoreAway);

	DoEventSlide();
}

#define EVENT_SLIDE_IN_TIME		0.5f
#define EVENT_FADE_IN_TIME		0.5f
#define EVENT_STAY_TIME			5.0f
#define EVENT_SLIDE_OUT_TIME	0.5f

void CHudScorebar::DoEventSlide()
{
	for (int team = 0; team < 2; team++)
	{
		int barEnd = WIDTH_OVERLAP;
		bool isAdding = false;

		for (int i = 0; i < m_vEventLists[team].Count(); i++)
		{
			Event_t *event = &m_vEventLists[team][i];
			float timePassed = gpGlobals->curtime - event->startTime;

			// Remove events which slided out on top
			if (timePassed >= EVENT_SLIDE_IN_TIME + EVENT_FADE_IN_TIME + EVENT_STAY_TIME + EVENT_SLIDE_OUT_TIME)
			{
				event->pEventBox->DeletePanel();
				m_vEventLists[team].Remove(0);
				/*if (m_vEventLists.Count() > 0)
				m_vEventLists[0].startTime += EVENT_SLIDE_TIME;*/
				i = -1;
				continue;
			}

			// Slide out
			if (timePassed >= EVENT_SLIDE_IN_TIME + EVENT_FADE_IN_TIME + EVENT_STAY_TIME)
			{
				if (i == 0)
				{
					int leftSlide = event->pEventBox->GetWide() * min(1, (timePassed - EVENT_STAY_TIME - EVENT_FADE_IN_TIME - EVENT_SLIDE_IN_TIME) / EVENT_SLIDE_OUT_TIME);
					event->pEventBox->SetX(barEnd - leftSlide);
					barEnd += event->pEventBox->GetWide() - leftSlide;
				}
				else
					event->startTime = gpGlobals->curtime - EVENT_STAY_TIME;

				continue;
			}

			// Fade in
			if (timePassed >= EVENT_SLIDE_IN_TIME)
			{
				event->pEventBox->SetX(barEnd);
				event->pEventBox->SetVisible(true);
				barEnd += event->pEventBox->GetWide();
				float fade = min(1, (timePassed - EVENT_SLIDE_IN_TIME) / EVENT_FADE_IN_TIME);
				Color color = Color(255, 255, 255, 255 * fade);
				event->pEventType->SetFgColor(Color(50, 50, 50, 255 * fade));
				event->pEventType->SetBgColor(Color(200, 200, 200, 255 * fade));
				event->pEventType->SetPaintBackgroundEnabled(true);
				event->pEventType->SetPaintBackgroundType(2);
				event->pEventText->SetFgColor(Color(200, 200, 200, 255 * fade));
				continue;
			}

			// Slide in
			if (!isAdding)
			{
				barEnd += event->pEventBox->GetWide() * min(1, timePassed / EVENT_SLIDE_IN_TIME);
				isAdding = true;
			}
			else
			{
				event->startTime = gpGlobals->curtime;
			}
		}

		m_pEventBars[team]->SetWide(barEnd);
	}
}

#include "ehandle.h"
#include "c_sdk_player.h"

void CHudScorebar::MsgFunc_MatchEvent(bf_read &msg)
{
	IScheme *pScheme = scheme()->GetIScheme(GetScheme());

	IGameResources *gr = GameResources();
	match_event_t eventType = (match_event_t)msg.ReadByte();
	int playerIndex = msg.ReadByte();
	int teamIndex = gr->GetTeam(playerIndex) - TEAM_A;
	//C_SDKPlayer *pPlayer1 = (C_SDKPlayer *)CHandle<C_SDKPlayer>::FromIndex(msg.ReadLong());
	//C_SDKPlayer *pPlayer2 = (C_SDKPlayer *)CHandle<C_SDKPlayer>::FromIndex(msg.ReadLong());

	Panel *pEventBox = new Panel(m_pEventBars[teamIndex], "EventPanel");
	pEventBox->SetBounds(0, BORDER, WIDTH_EVENTTYPE + WIDTH_MARGIN + WIDTH_EVENTTEXT, HEIGHT_TEAMBAR - 2 * BORDER);
	pEventBox->SetVisible(false);

	Label *pEventType = new Label(pEventBox, "EventTypeLabel", g_szMatchEventNames[eventType]);
	pEventType->SetFont(pScheme->GetFont("IOSMatchEvent"));
	pEventType->SetContentAlignment(Label::a_center);
	pEventType->SetBounds(0, 0, WIDTH_EVENTTYPE, pEventBox->GetTall());

	Label *pEventText = new Label(pEventBox, "EventTextLabel", gr->GetPlayerName(playerIndex));
	pEventText->SetBounds(WIDTH_EVENTTYPE + WIDTH_MARGIN, 0, WIDTH_EVENTTEXT, pEventBox->GetTall());
	pEventText->SetFont(pScheme->GetFont("IOSTeamEvent"));
	pEventText->SetContentAlignment(Label::a_center);

	Event_t e = { pEventBox, pEventType, pEventText, gpGlobals->curtime };
	m_vEventLists[teamIndex].AddToTail(e);
}