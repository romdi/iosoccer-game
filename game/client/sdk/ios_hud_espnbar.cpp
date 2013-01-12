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
class CHudESPNBar : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudESPNBar, vgui::Panel );

public:
	CHudESPNBar( const char *pElementName );
	void Init( void );

protected:
	virtual void OnThink( void );
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

private:

	Panel *m_pOuterPanel;
	Panel *m_pMainPanel;
	ImagePanel *m_pLogo;
	Label *m_pTime;
	Label *m_pTeamNames[2];
	Label *m_pTeamGoals[2];
	Panel *m_pTeamColors[2][2];
	Label *m_pNotification;
	match_event_t m_eCurMatchEvent;
	float m_flNotificationStart;
};

DECLARE_HUDELEMENT( CHudESPNBar );

enum { PANEL_WIDTH = 324, PANEL_HEIGHT = 29, PANEL_MARGIN = 20, PANEL_PADDING = 2 };
enum { LOGO_WIDTH = 50, LOGO_HEIGHT = 25 };
enum { TEAM_COLOR_WIDTH = 35, TEAM_COLOR_HEIGHT = 3 };
enum { TEAM_NAME_WIDTH = 70, TEAM_NAME_HEIGHT= 25 };
enum { TEAM_GOAL_WIDTH = 30, TEAM_GOAL_HEIGHT= 25 };
enum { TIME_WIDTH = 70, TIME_HEIGHT= 25 };

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudESPNBar::CHudESPNBar( const char *pElementName ) : BaseClass(NULL, "HudScorebar"), CHudElement( pElementName )
{
	SetHiddenBits(HIDEHUD_SCOREBAR);

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pOuterPanel = new Panel(this, "");
	m_pMainPanel = new Panel(m_pOuterPanel, "");
	m_pLogo = new ImagePanel(m_pMainPanel, "");
	m_pTime = new Label(m_pMainPanel, "", "");

	for (int i = 0; i < 2; i++)
	{
		m_pTeamColors[i][0] = new Panel(m_pMainPanel, VarArgs("TeamColor%d", i));
		m_pTeamColors[i][1] = new Panel(m_pMainPanel, VarArgs("TeamColor%d", i));

		m_pTeamNames[i] = new Label(m_pMainPanel, "", "");

		m_pTeamGoals[i] = new Label(m_pMainPanel, "", "");
	}

	m_pNotification = new Label(this, "", "");
	m_eCurMatchEvent = MATCH_EVENT_NONE;
	m_flNotificationStart = -1;
}

void CHudESPNBar::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	Color white(255, 255, 255, 255);
	Color black(0, 0, 0, 255);

	SetBounds(PANEL_MARGIN, PANEL_MARGIN, PANEL_WIDTH, PANEL_HEIGHT + 200);

	m_pOuterPanel->SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
	m_pOuterPanel->SetBgColor(Color(120, 120, 120, 255));
	m_pOuterPanel->SetZPos(1);

	m_pMainPanel->SetBounds(PANEL_PADDING, PANEL_PADDING, PANEL_WIDTH - 2 * PANEL_PADDING, PANEL_HEIGHT - 2 * PANEL_PADDING);

	m_pLogo->SetBounds(0, 0, LOGO_WIDTH, LOGO_HEIGHT);
	m_pLogo->SetBgColor(Color(200, 200, 200, 255));

	for (int i = 0; i < 2; i++)
	{
		m_pTeamNames[i]->SetBounds(LOGO_WIDTH + (i == 0 ? 0 : TEAM_NAME_WIDTH + 2 * TEAM_GOAL_WIDTH), 0, TEAM_NAME_WIDTH, TEAM_NAME_HEIGHT);
		m_pTeamNames[i]->SetFont(pScheme->GetFont("IOSScorebarMedium"));
		m_pTeamNames[i]->SetFgColor(white);
		m_pTeamNames[i]->SetBgColor(Color(50, 50, 50, 255));
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
	
	m_pTime->SetBounds(LOGO_WIDTH + 2 * TEAM_NAME_WIDTH + 2 * TEAM_GOAL_WIDTH, 0, TIME_WIDTH, TIME_HEIGHT);
	m_pTime->SetFont(pScheme->GetFont("IOSScorebarMedium"));
	m_pTime->SetFgColor(black);
	m_pTime->SetBgColor(white);
	m_pTime->SetContentAlignment(Label::a_center);

	m_pNotification->SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
	m_pNotification->SetBgColor(Color(0, 0, 0, 200));
	m_pNotification->SetTextInset(5, 5);
	m_pNotification->SetFont(pScheme->GetFont("IOSScorebarMedium"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudESPNBar::Init( void )
{
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
		if (GetBall()->m_eMatchEvent != m_eCurMatchEvent)
		{
			if (GetBall()->m_eMatchEvent == MATCH_EVENT_GOAL)
			{
				m_flNotificationStart = gpGlobals->curtime;
				m_pNotification->SetText("GOAL");
			}

			m_eCurMatchEvent = GetBall()->m_eMatchEvent;
		}
	}
}

void CHudESPNBar::Paint( void )
{
	if (m_flNotificationStart != -1)
	{
		if (gpGlobals->curtime - m_flNotificationStart <= 2.0f)
		{
			float fraction = (gpGlobals->curtime - m_flNotificationStart) / 2.0f;
			m_pNotification->SetY((1 - pow(1 - fraction, 2)) * PANEL_HEIGHT);
		}
		else if (gpGlobals->curtime - m_flNotificationStart <= 2.0f + 3.0f)
		{
			m_pNotification->SetY(PANEL_HEIGHT);
		}
		else
		{
			float fraction = (gpGlobals->curtime - (m_flNotificationStart + 2.0f + 3.0f)) / 2.0f;

			if (fraction >= 1)
			{
				fraction = 1;
				m_flNotificationStart = -1;
			}

			m_pNotification->SetY((1 - pow(fraction, 2)) * PANEL_HEIGHT);
		}	
	}
}