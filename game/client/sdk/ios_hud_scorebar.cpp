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
#include "c_ball.h"
#include "sdk_gamerules.h"
#include <vgui_controls/ImagePanel.h>
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
	void MsgFunc_NeutralMatchEvent(bf_read &msg);

protected:
	virtual void OnThink( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

private:

	Panel *m_pEventBars[2];
	Panel *m_pTeamColors[2][2];
	CUtlVector<Event_t> m_vEventLists[2];
	Label *m_pPlayers[2];
	Label *m_pEvent;
	char  m_szCurrentPlayer[MAX_PLAYER_NAME_LENGTH];
	int	  m_nCurrentPlayerIndex;
	Panel *m_pMainBar;
	Panel *m_pMainBarBG;
	Panel *m_pCenterBar;
	Panel *m_pCenterBarBG;
	Label *m_pTeamNames[2];
	Label *m_pTeamGoals[2];
	Label *m_pNewState;
	Label *m_pNewTime;
	Panel *m_pPenaltyPanels[2];
	Panel *m_pPenalties[2][5];

	Panel		*m_pExtensionBar[2];
	Label		*m_pExtensionText[2];

	Panel	*m_pTeamCrestPanels[2];
	ImagePanel	*m_pTeamCrests[2];

	float m_flNextPlayerUpdate;
};

DECLARE_HUDELEMENT( CHudScorebar );
DECLARE_HUD_MESSAGE(CHudScorebar, MatchEvent);
DECLARE_HUD_MESSAGE(CHudScorebar, NeutralMatchEvent);

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
#define WIDTH_TEAMCOLOR			5
#define WIDTH_INJURYTIME		30

#define WIDTH_TEAMBAR			(HPADDING + WIDTH_TEAM + WIDTH_MARGIN + WIDTH_TEAMCOLOR + WIDTH_MARGIN + WIDTH_SCORE + HPADDING)
#define WIDTH_TIMEBAR			(HPADDING + WIDTH_MATCHSTATE + WIDTH_MARGIN + WIDTH_TIME + HPADDING)

#define EVENT_WIDTH				200
#define EVENT_HEIGHT			200

enum { MAINBAR_WIDTH = 480, MAINBAR_HEIGHT = 40, MAINBAR_MARGIN = 15 };
enum { TEAMNAME_WIDTH = 175, TEAMNAME_MARGIN = 5 };
enum { TEAMGOAL_WIDTH = 30, TEAMGOAL_MARGIN = 10 };
enum { TIME_WIDTH = 120, TIME_MARGIN = 5 };
enum { STATE_WIDTH = 120, STATE_MARGIN = 5 };
enum { TOPEXTENSION_WIDTH = 278, TOPEXTENSION_HEIGHT = MAINBAR_HEIGHT, TOPEXTENSION_MARGIN = 10, TOPEXTENSION_TEXTMARGIN = 5 };
enum { TEAMCREST_SIZE = 70, TEAMCREST_HOFFSET = 482/*265*/, TEAMCREST_VOFFSET = 0, TEAMCREST_PADDING = 5 };
enum { TEAMCOLOR_WIDTH = 5, TEAMCOLOR_HEIGHT = MAINBAR_HEIGHT - 10, TEAMCOLOR_HMARGIN = 5, TEAMCOLOR_VMARGIN = (MAINBAR_HEIGHT - TEAMCOLOR_HEIGHT) / 2 };
enum { CENTERBAR_OFFSET = 5 };
enum { BAR_BORDER = 2 };
enum { PLAYERNAME_MARGIN = 5, PLAYERNAME_OFFSET = 50, PLAYERNAME_WIDTH = 150, PLAYERNAME_HEIGHT = 40 };
enum { PENALTYPANEL_HEIGHT = 30, PENALTYPANEL_PADDING = 5 };

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudScorebar::CHudScorebar( const char *pElementName ) : BaseClass(NULL, "HudScorebar"), CHudElement( pElementName )
{
	SetHiddenBits(HIDEHUD_SCOREBAR);

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pMainBarBG = new Panel(this, "");
	m_pMainBarBG->SetVisible(false);
	m_pCenterBarBG = new Panel(this, "");
	m_pCenterBarBG->SetVisible(false);
	m_pMainBar = new Panel(this, "");
	m_pCenterBar = new Panel(this, "");

	for (int i = 0; i < 2; i++)
	{
		m_pTeamColors[i][0] = new Panel(m_pMainBar, VarArgs("TeamColor%d", i));
		m_pTeamColors[i][1] = new Panel(m_pMainBar, VarArgs("TeamColor%d", i));
		m_pEventBars[i] = new Label(this, VarArgs("ScoreLabel%d", i), "");

		m_pPlayers[i] = new Label(this, "", "");

		m_pTeamNames[i] = new Label(m_pMainBar, "", "");
		m_pTeamGoals[i] = new Label(m_pMainBar, "", "");

		m_pExtensionBar[i] = new Panel(this, "");
		m_pExtensionText[i] = new Label(m_pExtensionBar[i], "", "");
		m_pTeamCrestPanels[i] = new Panel(this, "");
		m_pTeamCrests[i] = new ImagePanel(m_pTeamCrestPanels[i], "");

		m_pPenaltyPanels[i] = new Panel(this, "");

		for (int j = 0; j < 5; j++)
		{
			m_pPenalties[i][j] = new Panel(m_pPenaltyPanels[i], "");
		}
	}

	m_pEvent = new Label(this, "", "");

	m_pNewState = new Label(m_pCenterBar, "", "");
	m_pNewTime = new Label(m_pCenterBar, "", "");

	m_szCurrentPlayer[0] = 0;
	m_flNextPlayerUpdate = gpGlobals->curtime;
}

void CHudScorebar::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	SetBounds(0, 0, ScreenWidth(), 500);
 	//SetPaintBackgroundType (2); // Rounded corner box
 	//SetPaintBackgroundEnabled(true);
	//SetBgColor( Color( 0, 0, 255, 255 ) );

	Color white(255, 255, 255, 255);
	Color black(0, 0, 0, 255);

	m_pMainBarBG->SetBounds(GetWide() / 2 - MAINBAR_WIDTH / 2 - BAR_BORDER, MAINBAR_MARGIN - BAR_BORDER, MAINBAR_WIDTH + 2 * BAR_BORDER, MAINBAR_HEIGHT + 2 * BAR_BORDER);
	m_pMainBarBG->SetBgColor(white);
	m_pMainBarBG->SetPaintBackgroundType(2);

	m_pCenterBarBG->SetBounds(GetWide() / 2 - STATE_WIDTH / 2 - BAR_BORDER, MAINBAR_MARGIN - CENTERBAR_OFFSET - BAR_BORDER, STATE_WIDTH + 2 * BAR_BORDER, MAINBAR_HEIGHT + 2 * CENTERBAR_OFFSET + 2 * BAR_BORDER);
	m_pCenterBarBG->SetBgColor(white);
	m_pCenterBarBG->SetPaintBackgroundType(2);

	m_pMainBar->SetBounds(GetWide() / 2 - MAINBAR_WIDTH / 2, MAINBAR_MARGIN, MAINBAR_WIDTH, MAINBAR_HEIGHT);
	m_pMainBar->SetBgColor(black);
	m_pMainBar->SetPaintBackgroundType(2);
	m_pMainBar->SetZPos(2);

	m_pCenterBar->SetBounds(GetWide() / 2 - STATE_WIDTH / 2, MAINBAR_MARGIN - CENTERBAR_OFFSET, STATE_WIDTH, MAINBAR_HEIGHT + 2 * CENTERBAR_OFFSET);
	m_pCenterBar->SetBgColor(black);
	m_pCenterBar->SetPaintBackgroundType(2);
	m_pCenterBar->SetZPos(3);

	m_pNewState->SetBounds(m_pCenterBar->GetWide() / 2 - STATE_WIDTH / 2, 0, STATE_WIDTH, (MAINBAR_HEIGHT + 2 * CENTERBAR_OFFSET) / 2);
	m_pNewState->SetFgColor(white);
	m_pNewState->SetContentAlignment(Label::a_center);
	m_pNewState->SetFont(pScheme->GetFont("IOSScorebarSmall"));

	m_pNewTime->SetBounds(m_pCenterBar->GetWide() / 2 - TIME_WIDTH / 2, (MAINBAR_HEIGHT + 10) / 2, TIME_WIDTH, (MAINBAR_HEIGHT + 10) / 2);
	m_pNewTime->SetFgColor(white);
	m_pNewTime->SetContentAlignment(Label::a_center);
	m_pNewTime->SetFont(pScheme->GetFont("IOSScorebarMedium"));
	//m_pNewTime->SetBgColor(black);
	//m_pNewTime->SetPaintBackgroundType(2);
	
	m_pEvent->SetBounds(GetWide() / 2 - MAINBAR_WIDTH / 2, MAINBAR_MARGIN + MAINBAR_HEIGHT + 10, MAINBAR_WIDTH, EVENT_HEIGHT);
	m_pEvent->SetContentAlignment(Label::a_north);
	m_pEvent->SetFont(pScheme->GetFont("IOSEvent"));
	m_pEvent->SetFgColor(Color(255, 255, 255, 255));
	//m_pEvent->SetVisible(false);

	Color fgColor = Color(220, 220, 220, 255);
	Color bgColor = Color(35, 30, 40, 255);
	Color bgColorTransparent = Color(30, 30, 40, 200);

	for (int i = 0; i < 2; i++)
	{
		m_pTeamNames[i]->SetBounds(i * (MAINBAR_WIDTH - TEAMNAME_WIDTH), 0, TEAMNAME_WIDTH, MAINBAR_HEIGHT);
		m_pTeamNames[i]->SetFgColor(white);
		m_pTeamNames[i]->SetContentAlignment(Label::a_center);
		m_pTeamNames[i]->SetFont(pScheme->GetFont("IOSScorebar"));

		m_pTeamGoals[i]->SetBounds(MAINBAR_WIDTH / 2 - STATE_WIDTH / 2 - TEAMGOAL_WIDTH - TEAMGOAL_MARGIN + i * (2 * TEAMGOAL_MARGIN + TEAMGOAL_WIDTH + STATE_WIDTH), 0, TEAMGOAL_WIDTH, MAINBAR_HEIGHT);
		m_pTeamGoals[i]->SetFgColor(white);
		m_pTeamGoals[i]->SetContentAlignment(i == 0 ? Label::a_east : Label::a_west);
		m_pTeamGoals[i]->SetFont(pScheme->GetFont("IOSScorebar"));

		m_pExtensionBar[i]->SetBounds(GetWide() / 2 - TOPEXTENSION_WIDTH / 2 + (i == 0 ? -1 : 1) * (MAINBAR_WIDTH / 2 + TOPEXTENSION_WIDTH / 2 - TOPEXTENSION_MARGIN), MAINBAR_MARGIN, TOPEXTENSION_WIDTH, TOPEXTENSION_HEIGHT);
		m_pExtensionBar[i]->SetBgColor(black);
		m_pExtensionBar[i]->SetPaintBackgroundType(2);
		m_pExtensionBar[i]->SetZPos(1);

		m_pExtensionText[i]->SetBounds(TOPEXTENSION_TEXTMARGIN, 0, TOPEXTENSION_WIDTH - 2 * TOPEXTENSION_TEXTMARGIN, TOPEXTENSION_HEIGHT);
		m_pExtensionText[i]->SetFgColor(white);
		m_pExtensionText[i]->SetContentAlignment(Label::a_center);
		m_pExtensionText[i]->SetFont(pScheme->GetFont("IOSScorebarExtraInfo"));

		m_pTeamCrestPanels[i]->SetBounds(GetWide() / 2 - TEAMCREST_SIZE / 2 + (i == 0 ? -1 : 1) * TEAMCREST_HOFFSET, TEAMCREST_VOFFSET, TEAMCREST_SIZE, TEAMCREST_SIZE);
		m_pTeamCrestPanels[i]->SetZPos(3);
		//m_pTeamCrestPanels[i]->SetBgColor(black);
		//m_pTeamCrestPanels[i]->SetPaintBackgroundType(2);

		m_pTeamCrests[i]->SetBounds(TEAMCREST_PADDING, TEAMCREST_PADDING, TEAMCREST_SIZE - 2 * TEAMCREST_PADDING, TEAMCREST_SIZE - 2 * TEAMCREST_PADDING);
		m_pTeamCrests[i]->SetShouldScaleImage(true);
		m_pTeamCrests[i]->SetImage(i == 0 ? "hometeamcrest" : "awayteamcrest");
			
		m_pTeamColors[i][0]->SetBounds(TEAMCOLOR_HMARGIN + i * (MAINBAR_WIDTH - TEAMCOLOR_WIDTH - 2 * TEAMCOLOR_HMARGIN), TEAMCOLOR_VMARGIN, TEAMCOLOR_WIDTH, MAINBAR_HEIGHT - 2 * TEAMCOLOR_VMARGIN);
		m_pTeamColors[i][1]->SetBounds(TEAMCOLOR_HMARGIN + TEAMCOLOR_WIDTH + i * (MAINBAR_WIDTH - 3 * TEAMCOLOR_WIDTH - 2 * TEAMCOLOR_HMARGIN), TEAMCOLOR_VMARGIN, TEAMCOLOR_WIDTH, MAINBAR_HEIGHT - 2 * TEAMCOLOR_VMARGIN);

		m_pEventBars[i]->SetBounds(0, i * (HEIGHT_TEAMBAR + HEIGHT_MARGIN), 0, HEIGHT_TEAMBAR);
		m_pEventBars[i]->SetPaintBackgroundType(2);
		m_pEventBars[i]->SetBgColor(bgColorTransparent);
		m_pEventBars[i]->SetZPos(-1);

		m_pPlayers[i]->SetBounds(m_pMainBar->GetX() + m_pTeamNames[i]->GetX(), MAINBAR_MARGIN + MAINBAR_HEIGHT + 5, m_pTeamNames[i]->GetWide(), PLAYERNAME_HEIGHT);
		m_pPlayers[i]->SetContentAlignment(Label::a_north);
		m_pPlayers[i]->SetFont(pScheme->GetFont("IOSEventPlayer"));
		m_pPlayers[i]->SetFgColor(Color(255, 255, 255, 255));
		//m_pPlayers[i]->SetTextInset(5, 0);
		//m_pPlayers[i]->SetVisible(false);

		m_pPenaltyPanels[i]->SetBounds(m_pMainBar->GetX() + m_pTeamNames[i]->GetX(), MAINBAR_MARGIN + MAINBAR_HEIGHT + PLAYERNAME_HEIGHT, m_pTeamNames[i]->GetWide(), PENALTYPANEL_HEIGHT);
		m_pPenaltyPanels[i]->SetBgColor(Color(0, 0, 0, 255));

		for (int j = 0; j < 5; j++)
		{
			m_pPenalties[i][j]->SetBounds(j * (m_pPenaltyPanels[i]->GetWide() / 5) + PENALTYPANEL_PADDING, PENALTYPANEL_PADDING, m_pPenaltyPanels[i]->GetWide() / 5 - 2 * PENALTYPANEL_PADDING, PENALTYPANEL_HEIGHT - 2 * PENALTYPANEL_PADDING);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::Init( void )
{
	HOOK_HUD_MESSAGE(CHudScorebar, MatchEvent);
	HOOK_HUD_MESSAGE(CHudScorebar, NeutralMatchEvent);
}

const char *g_szStateNames[32] =
{
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
	"CD"
};

const char *g_szLongStateNames[32] =
{
	"WARMUP",
	"FIRST HALF",
	"FIRST HALF",
	"HALF TIME",
	"SECOND HALF",
	"SECOND HALF",
	"EX BREAK",
	"EX FIRST HALF",
	"EX FIRST HALF",
	"EX HALF TIME",
	"EX SECOND HALF",
	"EX SECOND HALF",
	"PEN BREAK",
	"PENALTIES",
	"COOLDOWN"
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::OnThink( void )
{
	if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
	{
		int round = SDKGameRules()->m_nPenaltyRound % 10;
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 5; j++)
			{
				//Color color = (i % 3 == 0 ? Color(255, 0, 0, 255) : (i % 3 == 1 ? Color(0, 255, 0, 255) : Color(100, 100, 100, 255)));
				Color color;
				if (j > round / 2
					|| TEAM_A + i == SDKGameRules()->m_nPenaltyTakingStartTeam && round - j * 2 == 0
					|| TEAM_A + i != SDKGameRules()->m_nPenaltyTakingStartTeam && round - j * 2 <= 1)
				{
					color = Color(100, 100, 100, 255);
				}
				else
				{
					if ((GetGlobalTeam(TEAM_A + i)->m_nPenaltyGoalBits & (1 << (SDKGameRules()->m_nPenaltyRound / 10 * 5 + j))) != 0)
						color = Color(0, 255, 0, 255);
					else
						color = Color(255, 0, 0, 255);
				}
				m_pPenalties[i][j]->SetBgColor(color);
			}
			m_pPenaltyPanels[i]->SetVisible(true);
		}
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			m_pPenaltyPanels[i]->SetVisible(false);
		}
	}

	if (gViewPortInterface->FindPanelByName(PANEL_SCOREBOARD)->IsVisible())
	{
		//wchar_t text[64];
		//_snwprintf(text, ARRAYSIZE(text), L"%d pl.  •   %d%% poss.", GetGlobalTeam(TEAM_A)->GetNumPlayers(), GetGlobalTeam(TEAM_A)->Get_Possession());
		//m_pExtensionText[0]->SetText(text);
		m_pExtensionText[0]->SetText(VarArgs("%d pl.     %d%% poss.", GetGlobalTeam(TEAM_A)->GetNumPlayers(), GetGlobalTeam(TEAM_A)->Get_Possession()));
		m_pExtensionText[1]->SetText(VarArgs("%d%% poss.     %d pl.", GetGlobalTeam(TEAM_B)->Get_Possession(), GetGlobalTeam(TEAM_B)->GetNumPlayers()));

		for (int i = 0; i < 2; i++)
		{
			m_pExtensionText[i]->SetFgColor(GetGlobalTeam(TEAM_A + i)->Get_HudKitColor());
			m_pExtensionBar[i]->SetVisible(true);
			m_pTeamCrestPanels[i]->SetVisible(GameResources()->HasTeamCrest(i + TEAM_A));
		}
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			m_pExtensionBar[i]->SetVisible(false);
			m_pTeamCrestPanels[i]->SetVisible(false);
		}
	}

	int nTime = SDKGameRules()->GetMatchDisplayTimeSeconds();
	nTime = abs(nTime);

	m_pNewState->SetText(g_szLongStateNames[SDKGameRules()->State_Get()]);
	char szInjuryTime[8];
	Q_snprintf(szInjuryTime, sizeof(szInjuryTime), (SDKGameRules()->m_nAnnouncedInjuryTime > 0) ? VarArgs("  +%d", SDKGameRules()->m_nAnnouncedInjuryTime) : "");  
	m_pNewTime->SetText(VarArgs("%d:%02d%s", nTime / 60, nTime % 60, szInjuryTime));

	for (int team = TEAM_A; team <= TEAM_B; team++)
	{
		m_pTeamNames[team - TEAM_A]->SetText(GetGlobalTeam(team)->Get_ShortTeamName());
		m_pTeamNames[team - TEAM_A]->SetFgColor(GetGlobalTeam(team)->Get_HudKitColor());
		m_pTeamColors[team - TEAM_A][0]->SetBgColor(GetGlobalTeam(team)->Get_PrimaryKitColor());
		m_pTeamColors[team - TEAM_A][1]->SetBgColor(GetGlobalTeam(team)->Get_SecondaryKitColor());
		m_pTeamGoals[team - TEAM_A]->SetText(VarArgs("%d", GetGlobalTeam(team)->Get_Goals()));
	}

	C_Ball *pBall = GetBall();
	if (pBall)
	{
		if (SDKGameRules()->IsIntermissionState())
		{
			m_pPlayers[0]->SetText("");
			m_pPlayers[1]->SetText("");
			m_szCurrentPlayer[0] = 0;
		}
		else if (pBall->m_pPl || Q_strlen(m_szCurrentPlayer) > 0)
		{
			if (pBall->m_pPl)
			{
				Q_strncpy(m_szCurrentPlayer, GameResources()->GetPlayerName(pBall->m_pPl->entindex()), sizeof(m_szCurrentPlayer));
				m_nCurrentPlayerIndex = GameResources()->GetTeam(pBall->m_pPl->entindex()) - TEAM_A;
			}

			if (gpGlobals->curtime >= m_flNextPlayerUpdate)
			{
				m_pPlayers[m_nCurrentPlayerIndex]->SetText(m_szCurrentPlayer);
				m_pPlayers[m_nCurrentPlayerIndex]->SetFgColor(GetGlobalTeam(TEAM_A + m_nCurrentPlayerIndex)->Get_HudKitColor());
				m_pPlayers[1 - m_nCurrentPlayerIndex]->SetText("");
				m_flNextPlayerUpdate = gpGlobals->curtime + 1.0f;
			}
		}
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
#include <Windows.h>

void CHudScorebar::MsgFunc_MatchEvent(bf_read &msg)
{
	IScheme *pScheme = scheme()->GetIScheme(GetScheme());

	IGameResources *gr = GameResources();
	match_event_t eventType = (match_event_t)msg.ReadByte();

	m_pEvent->SetText(g_szMatchEventNames[eventType]);
	//m_pPlayers[teamIndex]->SetText(playerName);

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

	if (eventType == MATCH_EVENT_KICKOFF)
	{
		FLASHWINFO flashInfo;
		flashInfo.cbSize = sizeof(FLASHWINFO);
		flashInfo.hwnd = FindWindow(NULL, "IOS Source Dev");
		flashInfo.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
		flashInfo.uCount = 3;
		flashInfo.dwTimeout = 0;
		FlashWindowEx(&flashInfo);
		//SetWindowText(FindWindow(NULL, "IOS Source Dev"), "LIVE - IOS Source Dev");
	}
}

void CHudScorebar::MsgFunc_NeutralMatchEvent(bf_read &msg)
{
	match_event_t eventType = (match_event_t)msg.ReadByte();
	m_pEvent->SetText(g_szMatchEventNames[eventType]);
}