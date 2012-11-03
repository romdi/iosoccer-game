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
#include "ehandle.h"
#include "c_sdk_player.h"
#include <Windows.h>
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
	void FireGameEvent( IGameEvent *event );

protected:
	virtual void OnThink( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void PaintBackground();

private:

	Panel *m_pTeamColors[2][2];
	CUtlVector<Event_t> m_vEventLists[2];
	Label *m_pPlayers[2];
	Label *m_pSubPlayers[2];
	Label *m_pSubSubPlayers[2];
	Label *m_pEvent;
	Label *m_pSubEvent;
	Label *m_pSubSubEvent;
	Label *m_pImportantEvent;
	Panel *m_pMainBars[2];
	Panel *m_pMainBarBG;
	Panel *m_pCenterBar;
	Panel *m_pCenterBarBG;
	Label *m_pTeamNames[2];
	Label *m_pTeamGoals[2];
	Label *m_pState;
	Label *m_pTime;
	Label *m_pInjuryTime;
	Panel *m_pPenaltyPanels[2];
	Panel *m_pPenalties[2][5];

	Panel		*m_pExtensionBar[2];
	Label		*m_pExtensionText[2];

	Panel	*m_pTeamCrestPanels[2];
	ImagePanel	*m_pTeamCrests[2];

	Label		*m_pHelpText;

	float m_flNextPlayerUpdate;
	float m_flImportantEventStart;
	match_event_t m_eCurMatchEvent;
};

DECLARE_HUDELEMENT( CHudScorebar );

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

enum { MAINBAR_WIDTH = 270, MAINBAR_HEIGHT = 40, MAINBAR_MARGIN = 15 };
enum { TEAMGOAL_WIDTH = 30, TEAMGOAL_MARGIN = 10 };
enum { TIME_WIDTH = 120, TIME_MARGIN = 5, INJURY_TIME_WIDTH = 20, INJURY_TIME_MARGIN = 10 };
enum { STATE_WIDTH = 140, STATE_MARGIN = 5 };
enum { TOPEXTENSION_WIDTH = 248, TOPEXTENSION_HEIGHT = MAINBAR_HEIGHT, TOPEXTENSION_MARGIN = 10, TOPEXTENSION_TEXTMARGIN = 5, TOPEXTENSION_TEXTOFFSET = 20 };
enum { TEAMCOLOR_WIDTH = 5, TEAMCOLOR_HEIGHT = MAINBAR_HEIGHT - 10, TEAMCOLOR_HMARGIN = 5, TEAMCOLOR_VMARGIN = (MAINBAR_HEIGHT - TEAMCOLOR_HEIGHT) / 2 };
enum { TEAMCREST_SIZE = 70, TEAMCREST_HOFFSET = (2 * TEAMCOLOR_HMARGIN + 2 * TEAMCOLOR_WIDTH), TEAMCREST_VOFFSET = 0, TEAMCREST_PADDING = 5 };
enum { CENTERBAR_WIDTH = 140, CENTERBAR_OFFSET = 5 };
enum { BAR_BORDER = 2 };
enum { PLAYERNAME_MARGIN = 5, PLAYERNAME_OFFSET = 50, PLAYERNAME_WIDTH = 150 };
enum { PENALTYPANEL_HEIGHT = 30, PENALTYPANEL_PADDING = 5, PENALTYPANEL_TOPMARGIN = 10 };
enum { TEAMNAME_WIDTH = (MAINBAR_WIDTH - 2 * TEAMGOAL_MARGIN - TEAMGOAL_WIDTH - 2 * TEAMCOLOR_HMARGIN - 2 * TEAMCOLOR_WIDTH - TEAMCREST_SIZE - TEAMCREST_PADDING), TEAMNAME_MARGIN = 10 };
enum { EVENT_MARGIN = 5, EVENT_WIDTH = 200, EVENT_HEIGHT = 35, SUBEVENT_WIDTH = 200, SUBEVENT_HEIGHT = 35, IMPORTANTEVENT_HEIGHT = 100 };
enum { HELPTEXT_HEIGHT = 40, HELPTEXT_BOTTOMMARGIN = 15 };

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
	m_pCenterBar = new Panel(this, "");

	for (int i = 0; i < 2; i++)
	{
		m_pMainBars[i] = new Panel(this, "");

		m_pTeamColors[i][0] = new Panel(m_pMainBars[i], VarArgs("TeamColor%d", i));
		m_pTeamColors[i][1] = new Panel(m_pMainBars[i], VarArgs("TeamColor%d", i));

		m_pPlayers[i] = new Label(this, "", "");
		m_pSubPlayers[i] = new Label(this, "", "");
		m_pSubSubPlayers[i] = new Label(this, "", "");

		m_pTeamNames[i] = new Label(m_pMainBars[i], "", "");
		m_pTeamGoals[i] = new Label(m_pMainBars[i], "", "");

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
	m_pSubEvent = new Label(this, "", "");
	m_pSubSubEvent = new Label(this, "", "");
	m_pImportantEvent = new Label(this, "", "");

	m_pState = new Label(m_pCenterBar, "", "");
	m_pTime = new Label(m_pCenterBar, "", "");
	m_pInjuryTime = new Label(m_pCenterBar, "", "");

	m_pHelpText = new Label(this, "", "");

	m_flNextPlayerUpdate = gpGlobals->curtime;
	m_flImportantEventStart = -1;
	m_eCurMatchEvent = MATCH_EVENT_NONE;
}

void CHudScorebar::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	SetBounds(0, 0, ScreenWidth(), ScreenHeight());
 	//SetPaintBackgroundType (2); // Rounded corner box
 	//SetPaintBackgroundEnabled(true);
	//SetBgColor( Color( 0, 0, 255, 255 ) );

	Color white(255, 255, 255, 255);
	Color black(0, 0, 0, 240);

	//m_pMainBarBG->SetBounds(GetWide() / 2 - MAINBAR_WIDTH / 2 - BAR_BORDER, MAINBAR_MARGIN - BAR_BORDER, MAINBAR_WIDTH + 2 * BAR_BORDER, MAINBAR_HEIGHT + 2 * BAR_BORDER);
	//m_pMainBarBG->SetBgColor(white);
	//m_pMainBarBG->SetPaintBackgroundType(2);

	//m_pCenterBarBG->SetBounds(GetWide() / 2 - STATE_WIDTH / 2 - BAR_BORDER, MAINBAR_MARGIN - CENTERBAR_OFFSET - BAR_BORDER, STATE_WIDTH + 2 * BAR_BORDER, MAINBAR_HEIGHT + 2 * CENTERBAR_OFFSET + 2 * BAR_BORDER);
	//m_pCenterBarBG->SetBgColor(white);
	//m_pCenterBarBG->SetPaintBackgroundType(2);

	m_pCenterBar->SetBounds(GetWide() / 2 - CENTERBAR_WIDTH / 2, MAINBAR_MARGIN - CENTERBAR_OFFSET, CENTERBAR_WIDTH, MAINBAR_HEIGHT + 2 * CENTERBAR_OFFSET);
	m_pCenterBar->SetBgColor(black);
	m_pCenterBar->SetPaintBackgroundType(2);
	m_pCenterBar->SetZPos(3);

	m_pState->SetBounds(m_pCenterBar->GetWide() / 2 - STATE_WIDTH / 2, 0, STATE_WIDTH, (MAINBAR_HEIGHT + 2 * CENTERBAR_OFFSET) / 2);
	m_pState->SetFgColor(white);
	m_pState->SetContentAlignment(Label::a_center);
	m_pState->SetFont(pScheme->GetFont("IOSScorebarSmall"));

	m_pTime->SetBounds(m_pCenterBar->GetWide() / 2 - TIME_WIDTH / 2, (MAINBAR_HEIGHT + 2 * CENTERBAR_OFFSET) / 2, TIME_WIDTH, (MAINBAR_HEIGHT + 2 * CENTERBAR_OFFSET) / 2);
	m_pTime->SetFgColor(white);
	m_pTime->SetContentAlignment(Label::a_center);
	m_pTime->SetFont(pScheme->GetFont("IOSScorebarMedium"));

	m_pInjuryTime->SetBounds(m_pCenterBar->GetWide() - INJURY_TIME_WIDTH - INJURY_TIME_MARGIN, (MAINBAR_HEIGHT + 2 * CENTERBAR_OFFSET) / 2, INJURY_TIME_WIDTH, (MAINBAR_HEIGHT + 2 * CENTERBAR_OFFSET) / 2);
	m_pInjuryTime->SetFgColor(white);
	m_pInjuryTime->SetContentAlignment(Label::a_center);
	m_pInjuryTime->SetFont(pScheme->GetFont("IOSScorebarMedium"));
	
	m_pEvent->SetBounds(GetWide() / 2 - EVENT_WIDTH / 2, MAINBAR_MARGIN + MAINBAR_HEIGHT + CENTERBAR_OFFSET + EVENT_MARGIN, EVENT_WIDTH, EVENT_HEIGHT);
	m_pEvent->SetContentAlignment(Label::a_center);
	m_pEvent->SetFont(pScheme->GetFont("IOSEvent"));
	m_pEvent->SetFgColor(Color(255, 255, 255, 255));
	//m_pEvent->SetVisible(false);

	m_pSubEvent->SetBounds(GetWide() / 2 - EVENT_WIDTH / 2, MAINBAR_MARGIN + MAINBAR_HEIGHT + CENTERBAR_OFFSET + EVENT_MARGIN + EVENT_HEIGHT, EVENT_WIDTH, SUBEVENT_HEIGHT);
	m_pSubEvent->SetContentAlignment(Label::a_center);
	m_pSubEvent->SetFont(pScheme->GetFont("IOSSubEvent"));
	m_pSubEvent->SetFgColor(Color(255, 255, 255, 255));

	m_pSubSubEvent->SetBounds(GetWide() / 2 - EVENT_WIDTH / 2, MAINBAR_MARGIN + MAINBAR_HEIGHT + CENTERBAR_OFFSET + EVENT_MARGIN + 2 * EVENT_HEIGHT, EVENT_WIDTH, SUBEVENT_HEIGHT);
	m_pSubSubEvent->SetContentAlignment(Label::a_center);
	m_pSubSubEvent->SetFont(pScheme->GetFont("IOSSubEvent"));
	m_pSubSubEvent->SetFgColor(Color(255, 255, 255, 255));

	m_pImportantEvent->SetBounds(0, GetTall() / 2 - IMPORTANTEVENT_HEIGHT / 2, GetWide(), IMPORTANTEVENT_HEIGHT);
	m_pImportantEvent->SetContentAlignment(Label::a_center);
	m_pImportantEvent->SetFont(pScheme->GetFont("IOSImportantEvent"));
	m_pImportantEvent->SetFgColor(Color(255, 255, 255, 255));

	m_pHelpText->SetBounds(0, GetTall() - HELPTEXT_HEIGHT - HELPTEXT_BOTTOMMARGIN, GetWide(), HELPTEXT_HEIGHT);
	m_pHelpText->SetContentAlignment(Label::a_south);
	m_pHelpText->SetFont(pScheme->GetFont("IOSHelpText"));
	m_pHelpText->SetFgColor(Color(255, 255, 255, 255));

	for (int i = 0; i < 2; i++)
	{
		m_pMainBars[i]->SetBounds(GetWide() / 2 + (i == 0 ? -CENTERBAR_WIDTH / 2 - MAINBAR_WIDTH : CENTERBAR_WIDTH / 2), MAINBAR_MARGIN, MAINBAR_WIDTH, MAINBAR_HEIGHT);
		m_pMainBars[i]->SetBgColor(black);
		m_pMainBars[i]->SetPaintBackgroundType(i == 0 ? 6 : 4);
		m_pMainBars[i]->SetZPos(2);

		m_pTeamNames[i]->SetBounds(i == 0 ? MAINBAR_WIDTH - TEAMGOAL_MARGIN - TEAMGOAL_WIDTH - TEAMNAME_MARGIN - TEAMNAME_WIDTH : TEAMGOAL_MARGIN + TEAMGOAL_WIDTH + TEAMNAME_MARGIN, 0, TEAMNAME_WIDTH, MAINBAR_HEIGHT);
		m_pTeamNames[i]->SetFgColor(white);
		//m_pTeamNames[i]->SetBgColor(Color(255, 0, 0, 255));
		m_pTeamNames[i]->SetContentAlignment(Label::a_center);
		m_pTeamNames[i]->SetFont(pScheme->GetFont("IOSScorebar"));

		m_pTeamGoals[i]->SetBounds(i == 0 ? MAINBAR_WIDTH - TEAMGOAL_MARGIN - TEAMGOAL_WIDTH : TEAMGOAL_MARGIN, 0, TEAMGOAL_WIDTH, MAINBAR_HEIGHT);
		m_pTeamGoals[i]->SetFgColor(white);
		//m_pTeamGoals[i]->SetBgColor(Color(0, 255, 0, 255));
		m_pTeamGoals[i]->SetContentAlignment(Label::a_center);
		m_pTeamGoals[i]->SetFont(pScheme->GetFont("IOSScorebarBold"));

		m_pExtensionBar[i]->SetBounds(GetWide() / 2 - TOPEXTENSION_WIDTH / 2 + (i == 0 ? -1 : 1) * (MAINBAR_WIDTH / 2 + TOPEXTENSION_WIDTH / 2 - TOPEXTENSION_MARGIN), MAINBAR_MARGIN, TOPEXTENSION_WIDTH, TOPEXTENSION_HEIGHT);
		m_pExtensionBar[i]->SetBgColor(black);
		m_pExtensionBar[i]->SetPaintBackgroundType(2);
		m_pExtensionBar[i]->SetZPos(1);
		m_pExtensionBar[i]->SetVisible(false);

		m_pExtensionText[i]->SetBounds(TOPEXTENSION_TEXTMARGIN + (i == 0 ? 1 : -1) * TOPEXTENSION_TEXTOFFSET, 0, TOPEXTENSION_WIDTH - 2 * TOPEXTENSION_TEXTMARGIN, TOPEXTENSION_HEIGHT);
		m_pExtensionText[i]->SetFgColor(white);
		m_pExtensionText[i]->SetContentAlignment(Label::a_center);
		m_pExtensionText[i]->SetFont(pScheme->GetFont("IOSScorebarExtraInfo"));

		//m_pTeamCrestPanels[i]->SetBounds(GetWide() / 2 - TEAMCREST_SIZE / 2 + (i == 0 ? -1 : 1) * TEAMCREST_HOFFSET, TEAMCREST_VOFFSET, TEAMCREST_SIZE, TEAMCREST_SIZE);
		m_pTeamCrestPanels[i]->SetBounds(i == 0 ? m_pMainBars[i]->GetX() + TEAMCREST_HOFFSET : m_pMainBars[i]->GetX() + MAINBAR_WIDTH - TEAMCREST_HOFFSET - TEAMCREST_SIZE, TEAMCREST_VOFFSET, TEAMCREST_SIZE, TEAMCREST_SIZE);
		m_pTeamCrestPanels[i]->SetZPos(3);
		//m_pTeamCrestPanels[i]->SetBgColor(black);
		//m_pTeamCrestPanels[i]->SetPaintBackgroundType(2);

		m_pTeamCrests[i]->SetBounds(TEAMCREST_PADDING, TEAMCREST_PADDING, TEAMCREST_SIZE - 2 * TEAMCREST_PADDING, TEAMCREST_SIZE - 2 * TEAMCREST_PADDING);
		m_pTeamCrests[i]->SetShouldScaleImage(true);
		m_pTeamCrests[i]->SetImage(i == 0 ? "hometeamcrest" : "awayteamcrest");
		//m_pTeamCrests[i]->SetDrawColor(Color(255, 255, 255, 150));
			
		m_pTeamColors[i][0]->SetBounds(i == 0 ? TEAMCOLOR_HMARGIN : MAINBAR_WIDTH - TEAMCOLOR_HMARGIN - 2 * TEAMCOLOR_WIDTH, TEAMCOLOR_VMARGIN, TEAMCOLOR_WIDTH, MAINBAR_HEIGHT - 2 * TEAMCOLOR_VMARGIN);
		m_pTeamColors[i][1]->SetBounds(i == 0 ? TEAMCOLOR_HMARGIN + TEAMCOLOR_WIDTH : MAINBAR_WIDTH - TEAMCOLOR_HMARGIN - TEAMCOLOR_WIDTH, TEAMCOLOR_VMARGIN, TEAMCOLOR_WIDTH, MAINBAR_HEIGHT - 2 * TEAMCOLOR_VMARGIN);

		m_pPlayers[i]->SetBounds(m_pMainBars[i]->GetX(), MAINBAR_MARGIN + MAINBAR_HEIGHT + CENTERBAR_OFFSET + EVENT_MARGIN, MAINBAR_WIDTH, EVENT_HEIGHT);
		m_pPlayers[i]->SetContentAlignment(Label::a_center);
		m_pPlayers[i]->SetFont(pScheme->GetFont("IOSEventPlayer"));
		m_pPlayers[i]->SetFgColor(Color(255, 255, 255, 255));
		//m_pPlayers[i]->SetTextInset(5, 0);
		//m_pPlayers[i]->SetVisible(false);

		m_pSubPlayers[i]->SetBounds(m_pMainBars[i]->GetX(), MAINBAR_MARGIN + MAINBAR_HEIGHT + CENTERBAR_OFFSET + EVENT_MARGIN + EVENT_HEIGHT, MAINBAR_WIDTH, SUBEVENT_HEIGHT);
		m_pSubPlayers[i]->SetContentAlignment(Label::a_center);
		m_pSubPlayers[i]->SetFont(pScheme->GetFont("IOSSubEventPlayer"));
		m_pSubPlayers[i]->SetFgColor(Color(255, 255, 255, 255));

		m_pSubSubPlayers[i]->SetBounds(m_pMainBars[i]->GetX(), MAINBAR_MARGIN + MAINBAR_HEIGHT + CENTERBAR_OFFSET + EVENT_MARGIN + 2 * EVENT_HEIGHT, MAINBAR_WIDTH, SUBEVENT_HEIGHT);
		m_pSubSubPlayers[i]->SetContentAlignment(Label::a_center);
		m_pSubSubPlayers[i]->SetFont(pScheme->GetFont("IOSSubEventPlayer"));
		m_pSubSubPlayers[i]->SetFgColor(Color(255, 255, 255, 255));

		m_pPenaltyPanels[i]->SetBounds(m_pMainBars[i]->GetX() + m_pTeamNames[i]->GetX(), MAINBAR_MARGIN + MAINBAR_HEIGHT + CENTERBAR_OFFSET + EVENT_MARGIN + EVENT_HEIGHT + PENALTYPANEL_TOPMARGIN, m_pTeamNames[i]->GetWide(), PENALTYPANEL_HEIGHT);
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
	ListenForGameEvent("wakeupcall");
	ListenForGameEvent("throw_in");
}

const char *g_szLongStateNames[32] =
{
	"WARM-UP",
	"FIRST HALF",
	"FIRST HALF",
	"HALF-TIME",
	"SECOND HALF",
	"SECOND HALF",
	"ET BREAK",
	"ET FIRST HALF",
	"ET FIRST HALF",
	"ET HALF-TIME",
	"ET SECOND HALF",
	"ET SECOND HALF",
	"PENALTIES BREAK",
	"PENALTIES",
	"COOL-DOWN"
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::OnThink( void )
{
	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();

	if (!SDKGameRules() || !GetGlobalTeam(TEAM_A) || !GetGlobalTeam(TEAM_B) || !pLocal)
		return;

	if (pLocal->State_Get() == STATE_OBSERVER_MODE)
	{
		if (gpGlobals->curtime - 10 <= pLocal->m_flStateEnterTime)
			m_pHelpText->SetText("Press [TAB] to show scoreboard. Click faded shirt icon to join.");
		else
			m_pHelpText->SetText("");
	}
	else if (pLocal->State_Get() == STATE_ACTIVE)
	{
		if (gpGlobals->curtime - 10 <= pLocal->m_flStateEnterTime)
			m_pHelpText->SetText("[LMB] / [RMB] to shoot. Hold and release [RMB] to perform a charged shot.");
		else
			m_pHelpText->SetText("");
	}

	if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
	{
		for (int i = 0; i < 2; i++)
		{
			int relativeRound = GetGlobalTeam(TEAM_A + i)->m_nPenaltyRound == 0 ? -1 : (GetGlobalTeam(TEAM_A + i)->m_nPenaltyRound - 1) % 5;
			int fullRounds = max(0, GetGlobalTeam(TEAM_A + i)->m_nPenaltyRound - 1) / 5;
			for (int j = 0; j < 5; j++)
			{
				Color color;
				if (j > relativeRound)
					color = Color(100, 100, 100, 255);
				else
				{
					if ((GetGlobalTeam(TEAM_A + i)->m_nPenaltyGoalBits & (1 << (j + fullRounds * 5))) != 0)
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

	//if (gViewPortInterface->FindPanelByName(PANEL_SCOREBOARD)->IsVisible())
	//{
	//	for (int i = 0; i < 2; i++)
	//	{
	//		wchar_t text[64];
	//		//_snwprintf(text, ARRAYSIZE(text), L"%d pl.   •   %d%% poss.", GetGlobalTeam(TEAM_A + i)->GetNumPlayers(), GetGlobalTeam(TEAM_A + i)->Get_Possession());
	//		_snwprintf(text, ARRAYSIZE(text), L"%d pl.   %d%% poss.", GetGlobalTeam(TEAM_A + i)->GetNumPlayers(), GetGlobalTeam(TEAM_A + i)->Get_Possession());
	//		m_pExtensionText[i]->SetText(text);
	//		m_pExtensionText[i]->SetFgColor(GetGlobalTeam(TEAM_A + i)->Get_HudKitColor());
	//		m_pExtensionBar[i]->SetVisible(true);
	//		m_pTeamCrestPanels[i]->SetVisible(GameResources()->HasTeamCrest(TEAM_A + i));
	//	}
	//}
	//else
	//{
	//	for (int i = 0; i < 2; i++)
	//	{
	//		m_pExtensionBar[i]->SetVisible(false);
	//		m_pTeamCrestPanels[i]->SetVisible(false);
	//	}
	//}

	int nTime = SDKGameRules()->GetMatchDisplayTimeSeconds();
	nTime = abs(nTime);

	m_pState->SetText(g_szLongStateNames[SDKGameRules()->State_Get()]);
	char *szInjuryTime = (SDKGameRules()->m_nAnnouncedInjuryTime > 0) ? VarArgs("+%d", SDKGameRules()->m_nAnnouncedInjuryTime) : "";
	m_pInjuryTime->SetText(szInjuryTime);
	m_pTime->SetText(VarArgs("%d:%02d", nTime / 60, nTime % 60));

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
			m_pSubPlayers[0]->SetText("");
			m_pSubPlayers[1]->SetText("");
			m_pEvent->SetText(g_szMatchEventNames[pBall->m_eMatchEvent]);
			m_pEvent->SetFgColor(Color(255, 255, 255, 255));
			m_pSubEvent->SetText("");
			m_pImportantEvent->SetText("");
			m_flNextPlayerUpdate = gpGlobals->curtime;
		}
		else
		{
			m_pEvent->SetText(g_szMatchEventNames[pBall->m_eMatchEvent]);
			int eventTeamIndex = clamp(pBall->m_nMatchEventTeam - TEAM_A, 0, 1);
			m_pEvent->SetFgColor(GetGlobalTeam(TEAM_A + eventTeamIndex)->Get_HudKitColor());

			m_pSubEvent->SetText(g_szMatchEventNames[pBall->m_eMatchSubEvent]);
			int subEventTeamIndex = clamp(pBall->m_nMatchSubEventTeam - TEAM_A, 0, 1);
			m_pSubEvent->SetFgColor(GetGlobalTeam(TEAM_A + subEventTeamIndex)->Get_HudKitColor());

			m_pSubSubEvent->SetText(g_szMatchEventNames[pBall->m_eMatchSubSubEvent]);
			int subSubEventTeamIndex = clamp(pBall->m_nMatchSubSubEventTeam - TEAM_A, 0, 1);
			m_pSubSubEvent->SetFgColor(GetGlobalTeam(TEAM_A + subSubEventTeamIndex)->Get_HudKitColor());

			if (pBall->m_eMatchEvent != m_eCurMatchEvent)
			{
				switch (pBall->m_eMatchEvent)
				{
				case MATCH_EVENT_GOAL:
				case MATCH_EVENT_OWNGOAL:
				case MATCH_EVENT_HALFTIME:
				case MATCH_EVENT_FINAL_WHISTLE:
					m_eCurMatchEvent = pBall->m_eMatchEvent;
					m_flImportantEventStart = gpGlobals->curtime;
					m_pImportantEvent->SetText(g_szMatchEventNames[pBall->m_eMatchEvent]);
					break;
				default:
					m_flImportantEventStart = -1;
					m_pImportantEvent->SetText("");
					break;
				}

				m_eCurMatchEvent = pBall->m_eMatchEvent;
				m_flNextPlayerUpdate = gpGlobals->curtime;
			}
			else
			{
				if (m_eCurMatchEvent != MATCH_EVENT_NONE && m_flImportantEventStart != -1 && gpGlobals->curtime >= m_flImportantEventStart + 1)
				{
					m_flImportantEventStart = -1;
					m_pImportantEvent->SetText("");
				}
			}

			if (pBall->m_eBallState != BALL_NORMAL)
			{
				m_flNextPlayerUpdate = gpGlobals->curtime;
			}

			if (gpGlobals->curtime >= m_flNextPlayerUpdate)
			{
				m_flNextPlayerUpdate = gpGlobals->curtime + 0.5f;

				if (pBall->m_pMatchEventPlayer)
				{
					m_pPlayers[eventTeamIndex]->SetText(pBall->m_pMatchEventPlayer->GetPlayerName());
					m_pPlayers[eventTeamIndex]->SetFgColor(GetGlobalTeam(TEAM_A + eventTeamIndex)->Get_HudKitColor());
					m_pPlayers[1 - eventTeamIndex]->SetText("");
				}
				else
				{
					m_pPlayers[0]->SetText("");
					m_pPlayers[1]->SetText("");
				}

				if (pBall->m_pMatchSubEventPlayer)
				{
					m_pSubPlayers[subEventTeamIndex]->SetText(pBall->m_pMatchSubEventPlayer->GetPlayerName());
					m_pSubPlayers[subEventTeamIndex]->SetFgColor(GetGlobalTeam(TEAM_A + subEventTeamIndex)->Get_HudKitColor());
					m_pSubPlayers[1 - subEventTeamIndex]->SetText("");
				}
				else
				{
					m_pSubPlayers[0]->SetText("");
					m_pSubPlayers[1]->SetText("");
				}

				if (pBall->m_pMatchSubSubEventPlayer)
				{
					m_pSubSubPlayers[subSubEventTeamIndex]->SetText(pBall->m_pMatchSubSubEventPlayer->GetPlayerName());
					m_pSubSubPlayers[subSubEventTeamIndex]->SetFgColor(GetGlobalTeam(TEAM_A + subSubEventTeamIndex)->Get_HudKitColor());
					m_pSubSubPlayers[1 - subSubEventTeamIndex]->SetText("");
				}
				else
				{
					m_pSubSubPlayers[0]->SetText("");
					m_pSubSubPlayers[1]->SetText("");
				}
			}
		}
	}
}

void CHudScorebar::FireGameEvent(IGameEvent *event)
{
	if (!g_PR)
		return;

	if (!Q_strcmp(event->GetName(), "wakeupcall"))
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
	else if (!Q_strcmp(event->GetName(), "throw_in"))
	{
	}
}

void CHudScorebar::PaintBackground()
{

}