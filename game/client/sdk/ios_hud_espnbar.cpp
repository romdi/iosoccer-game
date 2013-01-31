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
	void ApplySettings(KeyValues *inResourceData);

private:

	Label *m_pLogo;
	Label *m_pTime;
	Label *m_pTeamNames[2];
	Label *m_pTeamGoals[2];
	Panel *m_pTeamColors[2][2];
	Panel *m_pNotificationPanel;
	Label *m_pNotification;
	Label *m_pInjuryTime;
	Panel *m_pBackgroundPanel;
	match_event_t m_eCurMatchEvent;
	float m_flNotificationStart;
	float m_flInjuryTimeStart;
};

DECLARE_HUDELEMENT( CHudESPNBar );

static CHudESPNBar *g_pHudESPNBar = NULL;

void CC_ReloadScorebar(const CCommand &args)
{
	g_pHudESPNBar->Init();
}

static ConCommand reloadscorebar("reloadscorebar", CC_ReloadScorebar);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudESPNBar::CHudESPNBar( const char *pElementName ) : BaseClass(NULL, "HudScorebar"), CHudElement( pElementName )
{
	g_pHudESPNBar = this;

	SetHiddenBits(HIDEHUD_SCOREBAR);

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_eCurMatchEvent = MATCH_EVENT_NONE;
	m_flNotificationStart = -1;
	m_flInjuryTimeStart = -1;
}

void CHudESPNBar::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudESPNBar::Init( void )
{
	ListenForGameEvent("throw_in");

	SetProportional(false);

	LoadControlSettings("resource/ui/scorebars/default.res");

	m_pLogo = dynamic_cast<Label *>(FindChildByName("Logo", true));
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

	m_pBackgroundPanel = dynamic_cast<Panel *>(FindChildByName("BackgroundPanel", true));

	m_pNotification = new Label(m_pNotificationPanel, "", "");
	int x, y, w, h;
	m_pNotificationPanel->GetBounds(x, y, w, h);
	m_pNotification->SetBounds(0, 0, w, h);
	m_pNotification->SetContentAlignment(Label::a_center);
}

void CHudESPNBar::ApplySettings( KeyValues *inResourceData )
{
	Panel::ApplySettings( inResourceData );
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
				//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("sfs");
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
			m_pNotificationPanel->SetY(m_pBackgroundPanel->GetTall() - pow(1 - fraction, slideDownExp) * m_pNotificationPanel->GetTall());
		}
		else if (gpGlobals->curtime - m_flNotificationStart <= slideDownDuration + stayDuration)
		{
			m_pNotificationPanel->SetY(m_pBackgroundPanel->GetTall());
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

				m_pNotificationPanel->SetY(m_pBackgroundPanel->GetTall() - pow(fraction, slideUpExp) * m_pNotificationPanel->GetTall());
			}
		}	
	}
	else
		m_pNotificationPanel->SetY(m_pBackgroundPanel->GetTall() - m_pNotificationPanel->GetTall());

	if (m_flInjuryTimeStart != -1)
	{
		if (gpGlobals->curtime - m_flInjuryTimeStart <= slideDownDuration)
		{
			float fraction = (gpGlobals->curtime - m_flInjuryTimeStart) / slideDownDuration;
			m_pInjuryTime->SetX(m_pBackgroundPanel->GetWide() - m_pInjuryTime->GetWide() + (1 - pow(1 - fraction, slideDownExp)) * m_pInjuryTime->GetWide());
		}
		else
		{
			m_pInjuryTime->SetX(m_pBackgroundPanel->GetWide());
		}
	}
	else
		m_pInjuryTime->SetX(m_pBackgroundPanel->GetWide() - m_pInjuryTime->GetWide());
}

void CHudESPNBar::FireGameEvent(IGameEvent *event)
{
	if (!Q_strcmp(event->GetName(), "throw_in"))
	{
		//m_flNotificationStart = gpGlobals->curtime;
		//m_pNotification->SetText("THROW-IN");
	}
}