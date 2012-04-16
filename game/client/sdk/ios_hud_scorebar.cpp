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
#include <vgui_controls/TextImage.h>
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
	void DoEventSlide();
	void MsgFunc_MatchEvent(bf_read &msg);

protected:
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

private:

	Panel *m_pTimeBar;
	Label *m_pMatchState;
	Label *m_pTime;
	Panel *m_pInjuryTimeBar;
	Label *m_pInjuryTime;
	Label *m_pTeams[2];
	Label *m_pScores[2];
	Panel *m_pTeamBars[2];
	Panel *m_pEventBars[2];
	Panel *m_pTeamColors[2][2];
	CUtlVector<Event_t> m_vEventLists[2];
};

DECLARE_HUDELEMENT( CHudScorebar );
DECLARE_HUD_MESSAGE(CHudScorebar, MatchEvent);

#define	HEIGHT_MARGIN			3
#define HEIGHT_TIMEBAR			35
#define HEIGHT_TEAMBAR			35
#define HPADDING				5
#define VPADDING				3

#define WIDTH_MARGIN			5
#define WIDTH_OVERLAP			20
#define WIDTH_EVENTTYPE			150
#define WIDTH_EVENTTEXT			100
#define WIDTH_TEAM				115
#define WIDTH_SCORE				30
//#define WIDTH_TIMEBAR			150
#define WIDTH_MATCHSTATE		40
#define WIDTH_TIME				70
#define WIDTH_TEAMCOLOR			10
#define WIDTH_INJURYTIME		30

#define WIDTH_TEAMBAR			(HPADDING + WIDTH_TEAM + WIDTH_MARGIN + WIDTH_TEAMCOLOR + WIDTH_MARGIN + WIDTH_SCORE + HPADDING)
#define WIDTH_TIMEBAR			(HPADDING + WIDTH_MATCHSTATE + WIDTH_MARGIN + WIDTH_TIME + HPADDING)

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudScorebar::CHudScorebar( const char *pElementName ) : BaseClass(NULL, "HudScorebar"), CHudElement( pElementName )
{
	//SetHiddenBits(HIDEHUD_PLAYERDEAD);
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pTimeBar = new Panel(this, "TopPanel");
	m_pMatchState = new Label(m_pTimeBar, "MatchStateLabel", "");
	m_pTime = new Label(m_pTimeBar, "TimeLabel", "");
	m_pInjuryTimeBar = new Panel(this, "");
	m_pInjuryTime = new Label(m_pInjuryTimeBar, "", "");

	for (int i = 0; i < 2; i++)
	{
		m_pTeamBars[i] = new Panel(this, VarArgs("TeamPanel%d", i + 1));
		m_pTeams[i] = new Label(m_pTeamBars[i], VarArgs("TeamLabel%d", i), "");
		m_pTeamColors[i][0] = new Panel(m_pTeamBars[i], VarArgs("TeamColor%d", i));
		m_pTeamColors[i][1] = new Panel(m_pTeamBars[i], VarArgs("TeamColor%d", i));
		m_pScores[i] = new Label(m_pTeamBars[i], VarArgs("ScoreLabel%d", i), "");
		m_pEventBars[i] = new Label(this, VarArgs("ScoreLabel%d", i), "");
	}
}

void CHudScorebar::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	SetPos( 30, 30);
	SetSize( ScreenWidth() - 30, 200 );
 	//SetPaintBackgroundType (2); // Rounded corner box
 	//SetPaintBackgroundEnabled(true);
	//SetBgColor( Color( 0, 0, 255, 255 ) );

	Color fgColor = Color(220, 220, 220, 255);
	Color bgColor = Color(35, 30, 40, 255);
	Color bgColorTransparent = Color(30, 30, 40, 200);

	m_pTimeBar->SetBounds(0, 2 * (HEIGHT_TEAMBAR + HEIGHT_MARGIN), WIDTH_TIMEBAR, HEIGHT_TIMEBAR);
	m_pTimeBar->SetPaintBackgroundEnabled(true);
	m_pTimeBar->SetPaintBackgroundType(2);
	m_pTimeBar->SetBgColor(bgColor);

	m_pMatchState->SetBounds(HPADDING, VPADDING, WIDTH_MATCHSTATE, HEIGHT_TIMEBAR - 2 * VPADDING);
	m_pMatchState->SetContentAlignment(Label::a_west);
	m_pMatchState->SetFont(pScheme->GetFont("IOSScorebarSmall"));
	m_pMatchState->SetFgColor(fgColor);

	m_pTime->SetBounds(HPADDING + WIDTH_MATCHSTATE + WIDTH_MARGIN, VPADDING, WIDTH_TIME, HEIGHT_TIMEBAR - 2 * VPADDING);
	m_pTime->SetContentAlignment(Label::a_east);
	m_pTime->SetFont(pScheme->GetFont("IOSScorebar"));
	m_pTime->SetFgColor(fgColor);

	m_pInjuryTimeBar->SetBounds(m_pTimeBar->GetX() + m_pTimeBar->GetWide() - WIDTH_OVERLAP, m_pTimeBar->GetY(), WIDTH_OVERLAP + WIDTH_INJURYTIME + 4 * HPADDING, m_pTimeBar->GetTall());
	m_pInjuryTimeBar->SetPaintBackgroundEnabled(true);
	m_pInjuryTimeBar->SetPaintBackgroundType(2);
	m_pInjuryTimeBar->SetBgColor(bgColorTransparent);
	m_pInjuryTimeBar->SetZPos(-1);
	m_pInjuryTimeBar->SetVisible(false);

	m_pInjuryTime->SetBounds(WIDTH_OVERLAP + HPADDING, VPADDING, WIDTH_INJURYTIME + 2 * HPADDING, m_pInjuryTimeBar->GetTall() - 2 * VPADDING);
	m_pInjuryTime->SetContentAlignment(Label::a_center);
	m_pInjuryTime->SetFont(pScheme->GetFont("IOSScorebar"));
	m_pInjuryTime->SetPaintBackgroundEnabled(true);
	m_pInjuryTime->SetPaintBackgroundType(2);
	m_pInjuryTime->SetFgColor(bgColor);
	m_pInjuryTime->SetBgColor(fgColor);

	for (int i = 0; i < 2; i++)
	{		
		m_pTeamBars[i]->SetBounds(0, i * (HEIGHT_TEAMBAR + HEIGHT_MARGIN), WIDTH_TEAMBAR, HEIGHT_TEAMBAR);
		m_pTeamBars[i]->SetPaintBackgroundEnabled(true);
		m_pTeamBars[i]->SetPaintBackgroundType(2);
		m_pTeamBars[i]->SetBgColor(bgColor);

		m_pTeamColors[i][0]->SetBounds(HPADDING + WIDTH_TEAM + WIDTH_MARGIN, VPADDING, WIDTH_TEAMCOLOR / 2, HEIGHT_TEAMBAR - 2 * VPADDING);
		m_pTeamColors[i][1]->SetBounds(HPADDING + WIDTH_TEAM + WIDTH_MARGIN + WIDTH_TEAMCOLOR / 2, VPADDING, WIDTH_TEAMCOLOR / 2, HEIGHT_TEAMBAR - 2 * VPADDING);

		m_pTeams[i]->SetBounds(HPADDING, VPADDING, WIDTH_TEAM, HEIGHT_TEAMBAR - 2 * VPADDING);
		m_pTeams[i]->SetContentAlignment(Label::a_west);
		m_pTeams[i]->SetTextInset(0, 0);
		//m_pTeams[i]->SetAutoResize(Panel::PIN_TOPLEFT, AUTORESIZE_RIGHT, 10, 0, 10, 0);
		m_pTeams[i]->SetFont(pScheme->GetFont("IOSScorebar"));
		m_pTeams[i]->SetPaintBackgroundType(2);
		//m_pTeams[i]->SetBgColor(Color(100 * i, 100 * (1 - i), 0, 255));
		m_pTeams[i]->SetFgColor(fgColor);
		
		m_pScores[i]->SetBounds(HPADDING + WIDTH_TEAMCOLOR + WIDTH_MARGIN + WIDTH_TEAM + WIDTH_MARGIN, VPADDING, WIDTH_SCORE, HEIGHT_TEAMBAR - 2 * VPADDING);
		m_pScores[i]->SetContentAlignment(Label::a_center);
		m_pScores[i]->SetTextInset(0, 0);
		//m_pScores[i]->SetAutoResize(Panel::PIN_TOPLEFT, AUTORESIZE_RIGHT, 10, 0, 10, 0);
		m_pScores[i]->SetFont(pScheme->GetFont("IOSScorebar"));
		m_pScores[i]->SetPaintBackgroundType(2);
		//m_pScores[i]->SetBgColor(fgColor);
		m_pScores[i]->SetFgColor(fgColor);

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
}

const char *g_szStateNames[32] =
{
	"IN",
	"WU",
	"H1",
	"H1",
	"HT",
	"H2",
	"H2",
	"ETB",
	"ETH1",
	"ETH1",
	"ETHT",
	"ETH2",
	"ETH2",
	"PSB",
	"PS",
	"CD",
	"End"
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::Paint( void )
{
	float flTime = gpGlobals->curtime - SDKGameRules()->m_flStateEnterTime - SDKGameRules()->m_flInjuryTime;
	if (SDKGameRules()->m_flInjuryTimeStart != -1)
		flTime -= gpGlobals->curtime - SDKGameRules()->m_flInjuryTimeStart;
	int nTime;

	switch ( SDKGameRules()->State_Get() )
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

	m_pMatchState->SetText(g_szStateNames[SDKGameRules()->State_Get()]);
	m_pTime->SetText(VarArgs("% 3d:%02d", nTime / 60, nTime % 60));

	if (SDKGameRules()->m_nAnnouncedInjuryTime > 0)
	{
		m_pInjuryTime->SetText(VarArgs("+%d", SDKGameRules()->m_nAnnouncedInjuryTime));
		m_pInjuryTimeBar->SetVisible(true);
	}
	else
		m_pInjuryTimeBar->SetVisible(false);

	for (int team = TEAM_A; team <= TEAM_B; team++)
	{
		m_pTeams[team - TEAM_A]->SetText(GetGlobalTeam(team)->Get_ShortTeamName());
		m_pTeamColors[team - TEAM_A][0]->SetBgColor(GetGlobalTeam(team)->Get_PrimaryKitColor());
		m_pTeamColors[team - TEAM_A][1]->SetBgColor(GetGlobalTeam(team)->Get_SecondaryKitColor());
		m_pScores[team - TEAM_A]->SetText(VarArgs("%d", GetGlobalTeam(team)->Get_Goals()));
	}

	DoEventSlide();
}

#define EVENT_SLIDE_IN_TIME		0.4f
#define EVENT_FADE_IN_TIME		0.4f
#define EVENT_STAY_TIME			2.5f
#define EVENT_SLIDE_OUT_TIME	0.4f

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
	char playerName[MAX_PLAYER_NAME_LENGTH]; 
	msg.ReadString(playerName, sizeof(playerName));
	int teamIndex = msg.ReadByte() - TEAM_A;

	Panel *pEventBox = new Panel(m_pEventBars[teamIndex], "EventPanel");
	pEventBox->SetBounds(0, VPADDING, 0, HEIGHT_TEAMBAR - 2 * VPADDING);
	pEventBox->SetVisible(false);

	int width, height;
	wchar_t unicode[128];

	g_pVGuiLocalize->ConvertANSIToUnicode(g_szMatchEventNames[eventType], unicode, sizeof(unicode));
	surface()->GetTextSize(pScheme->GetFont("IOSMatchEvent"), unicode, width, height);
	Label *pEventType = new Label(pEventBox, "EventTypeLabel", g_szMatchEventNames[eventType]);
	pEventType->SetBounds(HPADDING, 0, width + 2 * HPADDING, pEventBox->GetTall());
	pEventType->SetFont(pScheme->GetFont("IOSMatchEvent"));
	pEventType->SetContentAlignment(Label::a_center);

	g_pVGuiLocalize->ConvertANSIToUnicode(playerName, unicode, sizeof(unicode));
	surface()->GetTextSize(pScheme->GetFont("IOSTeamEvent"), unicode, width, height);
	Label *pEventText = new Label(pEventBox, "EventTextLabel", playerName);
	pEventText->SetBounds(HPADDING + pEventType->GetWide(), 0, width + 2 * HPADDING, pEventBox->GetTall());
	pEventText->SetFont(pScheme->GetFont("IOSTeamEvent"));
	pEventText->SetContentAlignment(Label::a_center);

	pEventBox->SetWide(HPADDING + pEventType->GetWide() + pEventText->GetWide());

	Event_t e = { pEventBox, pEventType, pEventText, gpGlobals->curtime };
	m_vEventLists[teamIndex].AddToTail(e);
}