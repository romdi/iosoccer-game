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
	Label *pType;
	Label *pText;
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

	Panel *m_pTimePanel;
	Label *m_pTimeLabel;
	Label *m_pTeamLabels[2];
	Label *m_pScoreLabels[2];
	Panel *m_pTeamPanels[2];
	Panel *m_pEventPanels[2];
	Panel *m_pTeamColorPanels[2];

	CUtlVector<Event_t> m_vEventList[2];

	IScheme *pScheme;
};

DECLARE_HUDELEMENT( CHudScorebar );
DECLARE_HUD_MESSAGE(CHudScorebar, MatchEvent);

#define	HEIGHT_MARGIN			5
#define HEIGHT_TIMEBAR			35
#define HEIGHT_TEAMBAR			35

#define WIDTH_MARGIN			5
#define WIDTH_OVERLAP			10
#define WIDTH_EVENTTYPE			120
#define WIDTH_EVENTTEXT			150
#define WIDTH_TEAM				150
#define WIDTH_SCORE				35
#define WIDTH_TIME				150
#define WIDTH_TIMEBAR			150
#define WIDTH_TEAMCOLOR			10

#define WIDTH_TEAMBAR			WIDTH_MARGIN + WIDTH_TEAMCOLOR + WIDTH_MARGIN + WIDTH_TEAM + WIDTH_MARGIN + WIDTH_SCORE + WIDTH_MARGIN

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudScorebar::CHudScorebar( const char *pElementName ) : BaseClass(NULL, "HudScorebar"), CHudElement( pElementName )
{
	//SetHiddenBits(HIDEHUD_PLAYERDEAD);
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pTimePanel = new Panel(this, "TopPanel");
	m_pTimeLabel = new Label(m_pTimePanel, "TimeLabel", "");

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i] = new Panel(this, VarArgs("TeamPanel%d", i + 1));
		m_pTeamLabels[i] = new Label(m_pTeamPanels[i], VarArgs("TeamLabel%d", i), "");
		m_pTeamColorPanels[i] = new Panel(m_pTeamPanels[i], VarArgs("TeamColorPanel%d", i));
		m_pScoreLabels[i] = new Label(m_pTeamPanels[i], VarArgs("ScoreLabel%d", i), "");
		m_pEventPanels[i] = new Label(this, VarArgs("ScoreLabel%d", i), "");
	}
}

void CHudScorebar::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	pScheme = scheme;
	
	SetPos( 20, 20);
	SetSize( ScreenWidth() - 50, 200 );
 	//SetPaintBackgroundType (2); // Rounded corner box
 	//SetPaintBackgroundEnabled(true);
	//SetBgColor( Color( 0, 0, 255, 255 ) );

	Color bgColor = Color(25, 25, 25, 255);
	Color bgColorTransparent = Color(25, 25, 25, 200);

	m_pTimePanel->SetBounds(0, 2 * (HEIGHT_TEAMBAR + HEIGHT_MARGIN), WIDTH_TIMEBAR, HEIGHT_TIMEBAR);
	m_pTimePanel->SetPaintBackgroundEnabled(true);
	m_pTimePanel->SetPaintBackgroundType(2);
	m_pTimePanel->SetBgColor(bgColor);

	m_pTimeLabel->SetBounds(WIDTH_MARGIN, 0, WIDTH_TIME, HEIGHT_TIMEBAR);
	//m_pTimeLabel->SetContentAlignment(Label::a_center);
	m_pTimeLabel->SetFont(scheme->GetFont("IOSScorebar"));
	m_pTimeLabel->SetFgColor(Color(255, 255, 255, 255));

	for (int i = 0; i < 2; i++)
	{		
		m_pTeamPanels[i]->SetBounds(0, i * (HEIGHT_TEAMBAR + HEIGHT_MARGIN), WIDTH_TEAMBAR, HEIGHT_TEAMBAR);
		m_pTeamPanels[i]->SetPaintBackgroundEnabled(true);
		m_pTeamPanels[i]->SetPaintBackgroundType(2);
		m_pTeamPanels[i]->SetBgColor(bgColor);

		m_pTeamColorPanels[i]->SetBounds(WIDTH_MARGIN, HEIGHT_MARGIN, WIDTH_TEAMCOLOR, HEIGHT_TEAMBAR - 2 * HEIGHT_MARGIN);
		m_pTeamColorPanels[i]->SetBgColor(Color(100 * i, 100 * (1 - i), 0, 255));

		m_pTeamLabels[i]->SetBounds(WIDTH_MARGIN + WIDTH_TEAMCOLOR + WIDTH_MARGIN, HEIGHT_MARGIN, WIDTH_TEAM, HEIGHT_TEAMBAR - 2 * HEIGHT_MARGIN);
		m_pTeamLabels[i]->SetContentAlignment(Label::a_west);
		m_pTeamLabels[i]->SetTextInset(0, 0);
		//m_pTeamLabels[i]->SetAutoResize(Panel::PIN_TOPLEFT, AUTORESIZE_RIGHT, 10, 0, 10, 0);
		m_pTeamLabels[i]->SetFont(scheme->GetFont("IOSScorebar"));
		m_pTeamLabels[i]->SetPaintBackgroundType(2);
		//m_pTeamLabels[i]->SetBgColor(Color(100 * i, 100 * (1 - i), 0, 255));
		m_pTeamLabels[i]->SetFgColor(Color(255, 255, 255, 255));
		
		m_pScoreLabels[i]->SetBounds(WIDTH_MARGIN + WIDTH_TEAMCOLOR + WIDTH_MARGIN + WIDTH_TEAM + WIDTH_MARGIN, HEIGHT_MARGIN, WIDTH_SCORE, HEIGHT_TEAMBAR - 2 * HEIGHT_MARGIN);
		m_pScoreLabels[i]->SetContentAlignment(Label::a_center);
		m_pScoreLabels[i]->SetTextInset(0, 0);
		//m_pScoreLabels[i]->SetAutoResize(Panel::PIN_TOPLEFT, AUTORESIZE_RIGHT, 10, 0, 10, 0);
		m_pScoreLabels[i]->SetFont(scheme->GetFont("IOSScorebar"));
		m_pScoreLabels[i]->SetPaintBackgroundType(0);
		m_pScoreLabels[i]->SetBgColor(Color(255, 255, 255, 255));
		m_pScoreLabels[i]->SetFgColor(Color(0, 0, 0, 255));

		m_pEventPanels[i]->SetBounds(WIDTH_MARGIN + WIDTH_TEAMCOLOR + WIDTH_MARGIN + WIDTH_TEAM + WIDTH_MARGIN + WIDTH_SCORE + WIDTH_MARGIN - WIDTH_OVERLAP, i * (HEIGHT_TEAMBAR + HEIGHT_MARGIN), 0, HEIGHT_TEAMBAR);
		m_pEventPanels[i]->SetPaintBackgroundType(2);
		m_pEventPanels[i]->SetBgColor(bgColorTransparent);
		m_pEventPanels[i]->SetZPos(-1);
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

	m_pTimeLabel->SetText(stateText);
	m_pTeamLabels[0]->SetText(teamHomeName);
	m_pTeamLabels[1]->SetText(teamAwayName);
	//wchar_t scoreText[32];
	//_snwprintf(scoreText, sizeof(scoreText), L"%s - %s", scoreHome, scoreAway);
	m_pScoreLabels[1]->SetText(scoreHome);
	m_pScoreLabels[0]->SetText(scoreAway);

	DoEventSlide();
}

#define EVENT_STAY_TIME 5
#define EVENT_SLIDE_TIME 0.5f

void CHudScorebar::DoEventSlide()
{
	for (int team = 0; team < 2; team++)
	{
		int prevX2 = 0;
		bool isAdding = false;

		for (int i = 0; i < m_vEventList[team].Count(); i++)
		{
			Event_t *event = &m_vEventList[team][i];
			float timePassed = gpGlobals->curtime - event->startTime;

			// Remove events which slided out on top
			if (timePassed >= EVENT_STAY_TIME + EVENT_SLIDE_TIME)
			{
				event->pType->DeletePanel();
				event->pText->DeletePanel();
				m_vEventList[team].Remove(0);
				/*if (m_vEventList.Count() > 0)
				m_vEventList[0].startTime += EVENT_SLIDE_TIME;*/
				i = -1;
				continue;
			}

			int x = prevX2;

			// Slide out on top
			if (timePassed >= EVENT_STAY_TIME)
			{
				if (i == 0)
					x -= (WIDTH_EVENTTYPE + WIDTH_MARGIN + WIDTH_EVENTTEXT) * min(1, (timePassed - EVENT_STAY_TIME) / EVENT_SLIDE_TIME);
				else
					event->startTime = gpGlobals->curtime - EVENT_STAY_TIME;
			}

			event->pType->SetX(x);
			event->pText->SetX(x + WIDTH_EVENTTYPE + WIDTH_MARGIN);

			// Slide in from bottom after adding
			if (timePassed <= EVENT_SLIDE_TIME)
			{
				if (!isAdding)
				{		
					prevX2 = x + (WIDTH_EVENTTYPE + WIDTH_MARGIN + WIDTH_EVENTTEXT) * max(0, timePassed / EVENT_SLIDE_TIME);
					isAdding = true;
				}
				else
				{	
					event->startTime = gpGlobals->curtime;
				}
			}
			else
			{
				event->pType->SetVisible(true);
				event->pText->SetVisible(true);
				prevX2 = x + (WIDTH_EVENTTYPE + WIDTH_MARGIN + WIDTH_EVENTTEXT);
			}
		}

		m_pEventPanels[team]->SetWide(WIDTH_OVERLAP + prevX2);
	}
}

#include "ehandle.h"
#include "c_sdk_player.h"

void CHudScorebar::MsgFunc_MatchEvent(bf_read &msg)
{
	IGameResources *gr = GameResources();
	match_event_t eventType = (match_event_t)msg.ReadByte();
	int playerIndex = msg.ReadByte();
	int teamIndex = gr->GetTeam(playerIndex) - TEAM_A;
	//C_SDKPlayer *pPlayer1 = (C_SDKPlayer *)CHandle<C_SDKPlayer>::FromIndex(msg.ReadLong());
	//C_SDKPlayer *pPlayer2 = (C_SDKPlayer *)CHandle<C_SDKPlayer>::FromIndex(msg.ReadLong());

	Label *pEventType = new Label(m_pEventPanels[teamIndex], "EventTypeLabel", g_szMatchEventNames[eventType]);
	pEventType->SetFont(pScheme->GetFont("IOSMatchEvent"));
	pEventType->SetContentAlignment(Label::a_center);
	pEventType->SetFgColor(Color(255, 255, 255, 255));
	pEventType->SetBounds(0, HEIGHT_MARGIN, WIDTH_EVENTTYPE, HEIGHT_TEAMBAR - 2 * HEIGHT_MARGIN);
	pEventType->SetAlpha(100);
	pEventType->SetVisible(false);

	Label *pEventText = new Label(m_pEventPanels[teamIndex], "EventTextLabel", gr->GetPlayerName(playerIndex));
	pEventText->SetBounds(0, HEIGHT_MARGIN, WIDTH_EVENTTEXT, HEIGHT_TEAMBAR - 2 * HEIGHT_MARGIN);
	pEventText->SetFgColor(Color(255, 255, 255, 255));
	pEventText->SetFont(pScheme->GetFont("IOSTeamEvent"));
	pEventText->SetContentAlignment(Label::a_center);
	pEventText->SetAlpha(100);
	pEventText->SetVisible(false);

	Event_t e = { pEventType, pEventText, gpGlobals->curtime };
	m_vEventList[teamIndex].AddToTail(e);
}