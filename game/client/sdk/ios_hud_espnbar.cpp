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
// implementation of CHudESPNBar class
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
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"

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
class CHudESPNBar : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudESPNBar, vgui::EditablePanel );

public:
	CHudESPNBar( const char *pElementName );
	void Init( void );

protected:
	virtual void OnThink( void );
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	void FireGameEvent(IGameEvent *event);

private:

	Panel *m_pOuterPanel;
	Panel *m_pMainPanel;
	Label *m_pLogo;
	Label *m_pTime;
	Label *m_pTeamNames[2];
	Label *m_pTeamGoals[2];
	Panel *m_pTeamColors[2][2];
	Panel *m_pNotificationPanel;
	Label *m_pNotification;
	Label *m_pInjuryTime;
	match_event_t m_eCurMatchEvent;
	float m_flNotificationStart;
	float m_flInjuryTimeStart;
};

DECLARE_HUDELEMENT( CHudESPNBar );

enum { PANEL_WIDTH = 324, PANEL_HEIGHT = 29, PANEL_MARGIN = 20, PANEL_PADDING = 2 };
enum { LOGO_WIDTH = 50, LOGO_HEIGHT = 25 };
enum { TEAM_COLOR_WIDTH = 35, TEAM_COLOR_HEIGHT = 3 };
enum { TEAM_NAME_WIDTH = 70, TEAM_NAME_HEIGHT= 25 };
enum { TEAM_GOAL_WIDTH = 30, TEAM_GOAL_HEIGHT= 25 };
enum { TIME_WIDTH = 70, TIME_HEIGHT= 25 };
enum { INJURYTIME_WIDTH = 35 };

void CC_ReloadScorebar(const CCommand &args)
{
}

static ConCommand reloadscorebar("reloadscorebar", CC_ReloadScorebar);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudESPNBar::CHudESPNBar( const char *pElementName ) : BaseClass(NULL, "HudScorebar"), CHudElement( pElementName )
{
	SetHiddenBits(HIDEHUD_SCOREBAR);

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetProportional(false);

	LoadControlSettings("resource/ui/scorebars/default.res");

	m_pOuterPanel = new Panel(this, "");
	m_pMainPanel = new Panel(m_pOuterPanel, "");
	//m_pLogo = new Label(m_pMainPanel, "", "");
	m_pLogo = dynamic_cast<Label *>(FindChildByName("Logo", true));
	//m_pTime = new Label(m_pMainPanel, "Time", "");
	m_pTime = dynamic_cast<Label *>(FindChildByName("Time", true));

	m_pTeamColors[0][0] = dynamic_cast<Panel *>(FindChildByName("HomeTeamFirstColor", true));
	m_pTeamColors[0][1] = dynamic_cast<Panel *>(FindChildByName("HomeTeamSecondColor", true));

	m_pTeamColors[1][0] = dynamic_cast<Panel *>(FindChildByName("AwayTeamFirstColor", true));
	m_pTeamColors[1][1] = dynamic_cast<Panel *>(FindChildByName("AwayTeamSecondColor", true));

	m_pTeamNames[0] = dynamic_cast<Label *>(FindChildByName("HomeTeamName", true));
	m_pTeamNames[1] = dynamic_cast<Label *>(FindChildByName("AwayTeamName", true));

	m_pTeamGoals[0] = dynamic_cast<Label *>(FindChildByName("HomeTeamGoals", true));
	m_pTeamGoals[1] = dynamic_cast<Label *>(FindChildByName("AwayTeamGoals", true));

	m_pNotificationPanel = dynamic_cast<Panel *>(FindChildByName("NotificationPanel", true));

	m_pInjuryTime = dynamic_cast<Label *>(FindChildByName("InjuryTime", true));

	//for (int i = 0; i < 2; i++)
	//{
	//	m_pTeamColors[i][0] = new Panel(m_pMainPanel, VarArgs("TeamColor%d", i));
	//	m_pTeamColors[i][1] = new Panel(m_pMainPanel, VarArgs("TeamColor%d", i));

	//	m_pTeamNames[i] = new Label(m_pMainPanel, "", "");

	//	m_pTeamGoals[i] = new Label(m_pMainPanel, "", "");
	//}

	m_pNotification = new Label(m_pNotificationPanel, "", "");
	m_eCurMatchEvent = MATCH_EVENT_NONE;
	m_flNotificationStart = -1;
	m_flInjuryTimeStart = -1;
}

void CHudESPNBar::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	return;
	
	Color white(255, 255, 255, 255);
	Color black(0, 0, 0, 255);

	//SetBounds(PANEL_MARGIN, PANEL_MARGIN, PANEL_WIDTH + 100, PANEL_HEIGHT + 200);
	//SetPaintBackgroundEnabled(false);
	//SetCloseButtonVisible(false);
	//SetKeyBoardInputEnabled(false);
	//SetMouseInputEnabled(false);

	m_pOuterPanel->SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
	m_pOuterPanel->SetBgColor(Color(0, 0, 0, 100));
	m_pOuterPanel->SetZPos(1);

	m_pMainPanel->SetBounds(PANEL_PADDING, PANEL_PADDING, PANEL_WIDTH - 2 * PANEL_PADDING, PANEL_HEIGHT - 2 * PANEL_PADDING);

	//m_pLogo->SetBounds(0, 0, LOGO_WIDTH, LOGO_HEIGHT);
	//m_pLogo->SetFgColor(Color(0, 0, 0, 255));
	//m_pLogo->SetBgColor(Color(255, 255, 255, 255));
	//m_pLogo->SetContentAlignment(Label::a_center);
	//m_pLogo->SetFont(pScheme->GetFont("IOSScorebarMedium"));
	//m_pLogo->SetText("IOS");

	for (int i = 0; i < 2; i++)
	{
		m_pTeamNames[i]->SetBounds(LOGO_WIDTH + (i == 0 ? 0 : TEAM_NAME_WIDTH + 2 * TEAM_GOAL_WIDTH), 0, TEAM_NAME_WIDTH, TEAM_NAME_HEIGHT);
		m_pTeamNames[i]->SetFont(pScheme->GetFont("IOSScorebarMedium"));
		m_pTeamNames[i]->SetFgColor(white);
		m_pTeamNames[i]->SetBgColor(Color(75, 75, 75, 255));
		m_pTeamNames[i]->SetContentAlignment(Label::a_center);

		for (int j = 0; j < 2; j++)
		{
			m_pTeamColors[i][j]->SetBounds(LOGO_WIDTH + (i == 0 ? 0 : TEAM_NAME_WIDTH + 2 * TEAM_GOAL_WIDTH) + (j == 0 ? 0 : TEAM_COLOR_WIDTH) + 1, 0, TEAM_COLOR_WIDTH - 2, TEAM_COLOR_HEIGHT);
			m_pTeamColors[i][j]->SetZPos(1);
		}

		m_pTeamGoals[i]->SetBounds(LOGO_WIDTH + TEAM_NAME_WIDTH + (i == 0 ? 0 : TEAM_GOAL_WIDTH) + 1, 0, TEAM_GOAL_WIDTH - 2, TEAM_GOAL_HEIGHT);
		m_pTeamGoals[i]->SetFont(pScheme->GetFont("IOSScorebarMedium"));
		m_pTeamGoals[i]->SetFgColor(black);
		m_pTeamGoals[i]->SetBgColor(white);
		m_pTeamGoals[i]->SetContentAlignment(Label::a_center);
	}
	
	//m_pTime->SetBounds(LOGO_WIDTH + 2 * TEAM_NAME_WIDTH + 2 * TEAM_GOAL_WIDTH, 0, TIME_WIDTH, TIME_HEIGHT);
	//m_pTime->SetFont(pScheme->GetFont("IOSScorebarMedium"));
	//m_pTime->SetFgColor(black);
	//m_pTime->SetBgColor(white);
	//m_pTime->SetContentAlignment(Label::a_center);

	m_pNotification->SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
	m_pNotification->SetFgColor(Color(255, 255, 255, 255));
	m_pNotification->SetBgColor(Color(0, 0, 0, 200));
	m_pNotification->SetTextInset(5, 0);
	m_pNotification->SetFont(pScheme->GetFont("IOSScorebarMedium"));

	m_pInjuryTime->SetBounds(PANEL_WIDTH - INJURYTIME_WIDTH, 0, INJURYTIME_WIDTH, PANEL_HEIGHT);
	m_pInjuryTime->SetFgColor(Color(255, 255, 255, 255));
	m_pInjuryTime->SetBgColor(Color(0, 0, 0, 200));
	m_pInjuryTime->SetTextInset(5, 0);
	m_pInjuryTime->SetFont(pScheme->GetFont("IOSScorebarMedium"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudESPNBar::Init( void )
{
	ListenForGameEvent("throw_in");
}

char *g_szLongStateNames[32] =
{
	"WARM-UP",
	"FIRST HALF",
	"HALF-TIME",
	"SECOND HALF",
	"ET BREAK",
	"ET FIRST HALF",
	"ET HALF-TIME",
	"ET SECOND HALF",
	"PENALTIES BREAK",
	"PENALTIES",
	"COOL-DOWN"
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudESPNBar::OnThink( void )
{
	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();

	if (!SDKGameRules() || !GetGlobalTeam(TEAM_A) || !GetGlobalTeam(TEAM_B) || !pLocal)
		return;

	//char *szInjuryTime = (SDKGameRules()->m_nAnnouncedInjuryTime > 0) ? VarArgs("+%d", SDKGameRules()->m_nAnnouncedInjuryTime) : "";
	//m_pInjuryTime->SetText(szInjuryTime);

	if (SDKGameRules()->State_Get() == MATCH_WARMUP && mp_timelimit_warmup.GetFloat() < 0)
	{
		m_pTime->SetText(L"∞");
	}
	else
	{
		int time = abs(SDKGameRules()->GetMatchDisplayTimeSeconds(true));
		m_pTime->SetText(VarArgs("%d:%02d", time / 60, time % 60));
	}

	if (SDKGameRules()->m_nAnnouncedInjuryTime > 0 && m_flInjuryTimeStart == -1)
	{
		m_flInjuryTimeStart = gpGlobals->curtime;
		m_pInjuryTime->SetText(VarArgs("+%d", SDKGameRules()->m_nAnnouncedInjuryTime));
	}
	else if (SDKGameRules()->m_nAnnouncedInjuryTime == 0 && m_flInjuryTimeStart != -1)
	{
		m_flInjuryTimeStart = -1;
		m_pInjuryTime->SetText("");
	}

	for (int team = TEAM_A; team <= TEAM_B; team++)
	{
		m_pTeamNames[team - TEAM_A]->SetText(GetGlobalTeam(team)->Get_TeamCode());
		m_pTeamNames[team - TEAM_A]->SetFgColor(Color(255, 255, 255, 255));
		m_pTeamColors[team - TEAM_A][0]->SetBgColor(GetGlobalTeam(team)->Get_PrimaryKitColor());
		m_pTeamColors[team - TEAM_A][1]->SetBgColor(GetGlobalTeam(team)->Get_SecondaryKitColor());
		m_pTeamGoals[team - TEAM_A]->SetText(VarArgs("%d", GetGlobalTeam(team)->Get_Goals()));
	}

	if (GetBall())
	{
		switch (GetBall()->m_eMatchEvent)
		{
		case MATCH_EVENT_DRIBBLE:
		case MATCH_EVENT_PASS:
		case MATCH_EVENT_INTERCEPTION:
		case MATCH_EVENT_KEEPERSAVE:
			break;
		default:
			if (GetBall()->m_eMatchEvent != m_eCurMatchEvent)
			{
				m_flNotificationStart = gpGlobals->curtime;
				m_pNotification->SetText(g_szMatchEventNames[GetBall()->m_eMatchEvent]);
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("sfs");
			}
			break;
		}

		m_eCurMatchEvent = GetBall()->m_eMatchEvent;

		if (GetBall()->m_eMatchEvent == MATCH_EVENT_TIMEOUT && m_flNotificationStart != -1)
		{
			int time = m_flNotificationStart + mp_timeout_duration.GetFloat() - gpGlobals->curtime;

			if (time < 0)
				m_flNotificationStart = -1;
			else
				m_pNotification->SetText(VarArgs("TIMEOUT   %d:%02d", time / 60, time % 60));
		}
	}
}

void CHudESPNBar::Paint( void )
{
	const float slideDownDuration = 0.5f;
	const float stayDuration = 3.0f;
	const float slideUpDuration = 0.5f;

	const float slideDownExp = 2.0f;
	const float slideUpExp = 2.0f;

	if (m_flNotificationStart != -1)
	{
		if (gpGlobals->curtime - m_flNotificationStart <= slideDownDuration)
		{
			float fraction = (gpGlobals->curtime - m_flNotificationStart) / slideDownDuration;
			m_pNotification->SetY((1 - pow(1 - fraction, slideDownExp)) * PANEL_HEIGHT);
		}
		else if (gpGlobals->curtime - m_flNotificationStart <= slideDownDuration + stayDuration)
		{
			m_pNotification->SetY(PANEL_HEIGHT);
		}
		else
		{
			if (GetBall()->m_eMatchEvent != MATCH_EVENT_TIMEOUT)
			{
				float fraction = (gpGlobals->curtime - (m_flNotificationStart + slideDownDuration + stayDuration)) / slideUpDuration;

				if (fraction >= 1)
				{
					fraction = 1;
					m_flNotificationStart = -1;
				}

				m_pNotification->SetY((1 - pow(fraction, slideUpExp)) * PANEL_HEIGHT);
			}
		}	
	}
	else
		m_pNotification->SetY(0);

	if (m_flInjuryTimeStart != -1)
	{
		if (gpGlobals->curtime - m_flInjuryTimeStart <= slideDownDuration)
		{
			float fraction = (gpGlobals->curtime - m_flInjuryTimeStart) / slideDownDuration;
			m_pInjuryTime->SetX(PANEL_WIDTH - INJURYTIME_WIDTH + (1 - pow(1 - fraction, slideDownExp)) * INJURYTIME_WIDTH);
		}
		else
		{
			m_pInjuryTime->SetX(PANEL_WIDTH);
		}
	}
	else
		m_pInjuryTime->SetX(PANEL_WIDTH - INJURYTIME_WIDTH);
}

void CHudESPNBar::FireGameEvent(IGameEvent *event)
{
	if (!Q_strcmp(event->GetName(), "throw_in"))
	{
		//m_flNotificationStart = gpGlobals->curtime;
		//m_pNotification->SetText("THROW-IN");
	}
}