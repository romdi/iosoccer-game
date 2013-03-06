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
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include "clientmode_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define NOTIFICATION_COUNT 4
enum { NOTIFICATION_HEIGHT = 25 };
enum { CENTERFLASH_HEIGHT = 100 };
enum { EXTRAINFO_HEIGHT = 25 };

const float slideDownDuration = 0.5f;
const float slideUpDuration = 0.5f;
const float slideDownExp = 2.0f;
const float slideUpExp = 2.0f;

const float flashDuration = 0.5f;

const float extraInfoFadeDuration = 0.5f;

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
class CHudScorebar : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudScorebar, vgui::EditablePanel );

public:
	CHudScorebar( const char *pElementName );
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
	Label *m_pNotifications[NOTIFICATION_COUNT];
	Label *m_pInjuryTime;
	Panel *m_pBackgroundPanel;
	match_event_t m_eCurMatchEvent;
	float m_flNotificationStart;
	float m_flInjuryTimeStart;
	int m_nCurMatchEventTeam;
	Label *m_pCenterFlash;
	IScheme *m_pScheme;
	Panel *m_pMainPanel;
	float m_flStayDuration;
	Label *m_pExtraInfo;
};

DECLARE_HUDELEMENT( CHudScorebar );

static CHudScorebar *g_pHudScorebar = NULL;

void CC_HudReloadScorebar(const CCommand &args)
{
	g_pHudScorebar->Init();
}

static ConCommand hud_reloadscorebar("hud_reloadscorebar", CC_HudReloadScorebar);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudScorebar::CHudScorebar( const char *pElementName ) : BaseClass(NULL, "HudScorebar"), CHudElement( pElementName )
{
	g_pHudScorebar = this;

	SetHiddenBits(HIDEHUD_SCOREBAR);

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_eCurMatchEvent = MATCH_EVENT_NONE;
	m_flNotificationStart = -1;
	m_flInjuryTimeStart = -1;
	m_nCurMatchEventTeam = TEAM_UNASSIGNED;

	m_pMainPanel = new Panel(this, "");

	for (int i = 0; i < NOTIFICATION_COUNT; i++)
	{
		m_pNotifications[i] = new Label(this, "", "");
	}

	m_pCenterFlash = new Label(this, "", "");

	m_pExtraInfo = new Label(this, "", "");

	m_flStayDuration = 3.0f;

	SetProportional(false);

	LoadControlSettings("resource/ui/scorebars/default.res");
}

void CHudScorebar::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pScheme = pScheme;

	//SetProportional(false);

	//LoadControlSettings("resource/ui/scorebars/default.res");

	SetBounds(0, 0, ScreenWidth(), ScreenHeight());

	m_pMainPanel->SetBounds(30, 30, GetWide() - 30, GetTall() - 30);

	HFont font = m_pScheme->GetFont("IOSScorebarMedium");

	m_pLogo = dynamic_cast<Label *>(FindChildByName("Logo", true));
	m_pLogo->SetParent(m_pMainPanel);
	m_pLogo->SetFont(font);

	m_pTime = dynamic_cast<Label *>(FindChildByName("Time", true));
	m_pTime->SetParent(m_pMainPanel);
	m_pTime->SetFont(font);

	m_pTeamColors[0][0] = dynamic_cast<Panel *>(FindChildByName("HomeTeamFirstColor", true));
	m_pTeamColors[0][0]->SetParent(m_pMainPanel);

	m_pTeamColors[0][1] = dynamic_cast<Panel *>(FindChildByName("HomeTeamSecondColor", true));
	m_pTeamColors[0][1]->SetParent(m_pMainPanel);

	m_pTeamColors[1][0] = dynamic_cast<Panel *>(FindChildByName("AwayTeamFirstColor", true));
	m_pTeamColors[1][0]->SetParent(m_pMainPanel);

	m_pTeamColors[1][1] = dynamic_cast<Panel *>(FindChildByName("AwayTeamSecondColor", true));
	m_pTeamColors[1][1]->SetParent(m_pMainPanel);

	m_pTeamNames[0] = dynamic_cast<Label *>(FindChildByName("HomeTeamName", true));
	m_pTeamNames[0]->SetParent(m_pMainPanel);
	m_pTeamNames[0]->SetFont(font);

	m_pTeamNames[1] = dynamic_cast<Label *>(FindChildByName("AwayTeamName", true));
	m_pTeamNames[1]->SetParent(m_pMainPanel);
	m_pTeamNames[1]->SetFont(font);

	m_pTeamGoals[0] = dynamic_cast<Label *>(FindChildByName("HomeTeamGoals", true));
	m_pTeamGoals[0]->SetParent(m_pMainPanel);
	m_pTeamGoals[0]->SetFont(font);

	m_pTeamGoals[1] = dynamic_cast<Label *>(FindChildByName("AwayTeamGoals", true));
	m_pTeamGoals[1]->SetParent(m_pMainPanel);
	m_pTeamGoals[1]->SetFont(font);

	m_pInjuryTime = dynamic_cast<Label *>(FindChildByName("InjuryTime", true));
	m_pInjuryTime->SetParent(m_pMainPanel);
	m_pInjuryTime->SetFont(font);

	m_pBackgroundPanel = dynamic_cast<Panel *>(FindChildByName("BackgroundPanel", true));
	m_pBackgroundPanel->SetParent(m_pMainPanel);

	m_pNotificationPanel = dynamic_cast<Panel *>(FindChildByName("NotificationPanel", true));
	m_pNotificationPanel->SetTall(NOTIFICATION_HEIGHT);
	m_pNotificationPanel->SetParent(m_pMainPanel);
	m_pNotificationPanel->SetFgColor(Color(0, 0, 0, 255));
	m_pNotificationPanel->SetBgColor(Color(255, 255, 255, 200));

	int x, y, w, h;
	m_pNotificationPanel->GetBounds(x, y, w, h);

	for (int i = 0; i < NOTIFICATION_COUNT; i++)
	{
		m_pNotifications[i]->SetParent(m_pNotificationPanel);
		m_pNotifications[i]->SetFont(font);
		m_pNotifications[i]->SetBounds(0, i * NOTIFICATION_HEIGHT, w, NOTIFICATION_HEIGHT);
		m_pNotifications[i]->SetContentAlignment(Label::a_center);
		m_pNotifications[i]->SetFgColor(Color(0, 0, 0, 255));
	}

	m_pCenterFlash->SetBounds(0, GetTall() * 0.33f - CENTERFLASH_HEIGHT / 2, GetWide(), CENTERFLASH_HEIGHT);
	m_pCenterFlash->SetContentAlignment(Label::a_center);
	m_pCenterFlash->SetFont(m_pScheme->GetFont("IOSImportantEvent"));
	m_pCenterFlash->SetFgColor(Color(255, 255, 255, 255));
	m_pCenterFlash->SetVisible(false);

	m_pExtraInfo->SetContentAlignment(Label::a_center);
	m_pExtraInfo->SetFont(m_pScheme->GetFont("IOSScorebarMediumItalic"));
	m_pExtraInfo->SetFgColor(Color(255, 255, 255, 255));
	m_pExtraInfo->SetBgColor(Color(0, 0, 0, 200));
	m_pExtraInfo->SetParent(m_pMainPanel);
	m_pExtraInfo->SetZPos(-2);
	//m_pExtraInfo->SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::Init( void )
{
	ListenForGameEvent("timeout_pending");
	ListenForGameEvent("timeout");
	ListenForGameEvent("match_state");
	ListenForGameEvent("set_piece");
	ListenForGameEvent("goal");
	ListenForGameEvent("own_goal");
	ListenForGameEvent("foul");
	ListenForGameEvent("penalty");
}

void CHudScorebar::ApplySettings( KeyValues *inResourceData )
{
	Panel::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScorebar::OnThink( void )
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

	if (m_eCurMatchEvent == MATCH_EVENT_TIMEOUT)
	{
		if (SDKGameRules()->GetTimeoutEnd() == -1)
			m_pNotifications[0]->SetText(L"TIMEOUT (∞)");
		else 
		{
			int time = SDKGameRules()->GetTimeoutEnd() - gpGlobals->curtime;
			m_pNotifications[0]->SetText(VarArgs("TIMEOUT: %s (%d:%02d)", g_PR->GetTeamCode(m_nCurMatchEventTeam), time / 60, time % 60));
		}
	}

	if (m_flNotificationStart != -1)
	{
		float timePassed = gpGlobals->curtime - m_flNotificationStart;
		float timeFrac = min(2, timePassed / (flashDuration / 2.0f));
		float colorCoeff = -cos(M_PI * timeFrac) / 2 + 0.5f;
		//m_pNotificationPanel->SetBgColor(Color(255 * colorCoeff, 255 * colorCoeff, 255 * colorCoeff, 255));
		m_pNotificationPanel->SetBgColor(Color(255, 255, 255, 200));

		if (gpGlobals->curtime - m_flNotificationStart <= slideDownDuration)
		{
			float fraction = (gpGlobals->curtime - m_flNotificationStart) / slideDownDuration;
			m_pNotificationPanel->SetY(m_pBackgroundPanel->GetTall() - pow(1 - fraction, slideDownExp) * m_pNotificationPanel->GetTall());
			m_pExtraInfo->SetY(m_pBackgroundPanel->GetTall() - m_pExtraInfo->GetTall());
		}
		else if (gpGlobals->curtime - m_flNotificationStart <= slideDownDuration + m_flStayDuration)
		{
			m_pNotificationPanel->SetY(m_pBackgroundPanel->GetTall());
			m_pCenterFlash->SetText("");

			m_pExtraInfo->SetBounds(m_pNotificationPanel->GetX(), m_pNotificationPanel->GetY() + m_pNotificationPanel->GetTall(), m_pNotificationPanel->GetWide(), EXTRAINFO_HEIGHT);
			
			if (timePassed - slideDownDuration <= 0)
				m_pExtraInfo->SetAlpha(0);
			else if (timePassed - slideDownDuration <= extraInfoFadeDuration)
			{
				float extraFrac = min(1, (timePassed - slideDownDuration) / extraInfoFadeDuration);
				float extraCoeff = pow(extraFrac, 2) * (3 - 2 * extraFrac);
				m_pExtraInfo->SetAlpha(255 * extraCoeff);
			}
			else if (timePassed - slideDownDuration <= m_flStayDuration - extraInfoFadeDuration)
			{
				m_pExtraInfo->SetAlpha(255);
			}
			//else if (timePassed - slideDownDuration <= m_flStayDuration - extraInfoFadeDuration)
			//{
			//	float extraFrac = min(1, (m_flStayDuration - (timePassed - slideDownDuration) - extraInfoFadeDuration) / extraInfoFadeDuration);
			//	float extraCoeff = pow(extraFrac, 2) * (3 - 2 * extraFrac);
			//	m_pExtraInfo->SetAlpha(255 * extraCoeff);
			//}
			//else
			//	m_pExtraInfo->SetAlpha(0);
		}
		else
		{
			float fraction = (gpGlobals->curtime - (m_flNotificationStart + slideDownDuration + m_flStayDuration)) / slideUpDuration;

			if (fraction >= 1)
			{
				fraction = 1;
				m_flNotificationStart = -1;
			}

			m_pNotificationPanel->SetY(m_pBackgroundPanel->GetTall() - pow(fraction, slideUpExp) * (m_pNotificationPanel->GetTall() + m_pExtraInfo->GetTall()));
			m_pCenterFlash->SetText("");
			m_pExtraInfo->SetY(m_pNotificationPanel->GetY() + m_pNotificationPanel->GetTall());
		}	
	}
	else
	{
		m_pNotificationPanel->SetY(m_pBackgroundPanel->GetTall() - m_pNotificationPanel->GetTall());
		m_pCenterFlash->SetText("");
		//m_pNotificationPanel->SetBgColor(Color(0, 0, 0, 255));
		//m_pExtraInfo->SetAlpha(0);
		m_pExtraInfo->SetY(m_pBackgroundPanel->GetTall() - m_pExtraInfo->GetTall());
	}

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

void CHudScorebar::Paint( void )
{
}

char *GetPossessionText()
{
	return VarArgs("%d%% possession %d%%", GetGlobalTeam(TEAM_A)->m_Possession, GetGlobalTeam(TEAM_B)->m_Possession);
}

char *GetShotsOnGoalText()
{
	return VarArgs("%d%% shots on goal %d%%",
		GetGlobalTeam(TEAM_A)->m_ShotsOnGoal * 100 / max(1, GetGlobalTeam(TEAM_A)->m_Shots),
		GetGlobalTeam(TEAM_B)->m_ShotsOnGoal * 100 / max(1, GetGlobalTeam(TEAM_B)->m_Shots));
}

char *GetPassingText()
{
	return VarArgs("%d%% passes completed %d%%",
		GetGlobalTeam(TEAM_A)->m_PassesCompleted * 100 / max(1, GetGlobalTeam(TEAM_A)->m_Passes),
		GetGlobalTeam(TEAM_B)->m_PassesCompleted * 100 / max(1, GetGlobalTeam(TEAM_B)->m_Passes));
}

char *GetOrdinal(int number)
{
	static char ordinal[3];

	if (number % 100 == 11 || number % 100 == 12 || number % 100 == 13)
	{
		Q_strncpy(ordinal, "th", 3);
	}
	else
	{
		switch (number % 10)
		{
		case 1: Q_strncpy(ordinal, "st", 3); break;
		case 2: Q_strncpy(ordinal, "nd", 3); break;
		case 3: Q_strncpy(ordinal, "rd", 3); break;
		default: Q_strncpy(ordinal, "th", 3); break;
		}
	}

	return ordinal;
}

char *GetSetPieceCountText(match_event_t matchEvent, int team)
{
	char text[32] = { 0 };
	int number;

	switch (matchEvent)
	{
	case MATCH_EVENT_THROWIN: 
		number = GetGlobalTeam(team)->m_ThrowIns + 1;
		Q_strncpy(text, "throw-in for", 32);
		break;
	case MATCH_EVENT_GOALKICK: 
		number = GetGlobalTeam(team)->m_GoalKicks + 1;
		Q_strncpy(text, "goal kick for", 32);
		break;
	case MATCH_EVENT_CORNER: 
		number = GetGlobalTeam(team)->m_Corners + 1;
		Q_strncpy(text, "corner kick for", 32);
		break;
	case MATCH_EVENT_FREEKICK: 
		number = GetGlobalTeam(team)->m_FreeKicks + 1;
		Q_strncpy(text, "free kick for", 32);
		break;
	case MATCH_EVENT_FOUL: 
		number = GetGlobalTeam(team)->m_Fouls;
		Q_strncpy(text, "foul for", 32);
		break;
	case MATCH_EVENT_OFFSIDE: 
		number = GetGlobalTeam(team)->m_Offsides;
		Q_strncpy(text, "offside for", 32);
		break;
	}

	return VarArgs("%d%s %s %s", number, GetOrdinal(number), text, GetGlobalTeam(team)->Get_ShortTeamName());
}

void CHudScorebar::FireGameEvent(IGameEvent *event)
{
	m_flNotificationStart = gpGlobals->curtime;
	m_nCurMatchEventTeam = TEAM_UNASSIGNED;
	m_pNotificationPanel->SetTall(NOTIFICATION_HEIGHT);
	m_pExtraInfo->SetVisible(false);

	for (int i = 1; i < NOTIFICATION_COUNT; i++)
		m_pNotifications[i]->SetText("");

	if (!Q_strcmp(event->GetName(), "match_state"))
	{
		if ((m_eCurMatchEvent == MATCH_EVENT_TIMEOUT_PENDING || m_eCurMatchEvent == MATCH_EVENT_TIMEOUT) && (match_event_t)event->GetInt("state") == MATCH_EVENT_NONE)
		{
			m_flStayDuration = 3.0f;
			m_flNotificationStart = gpGlobals->curtime - slideDownDuration - m_flStayDuration;
		}
		else
			m_pNotifications[0]->SetText(VarArgs("%s", g_szMatchEventNames[event->GetInt("state")]));

		m_eCurMatchEvent = (match_event_t)event->GetInt("state");
		m_flStayDuration = 3.0f;
	}
	else if (!Q_strcmp(event->GetName(), "set_piece"))
	{
		m_pNotifications[0]->SetText(VarArgs("%s: %s", g_szMatchEventNames[event->GetInt("type")], g_PR->GetTeamCode(event->GetInt("taking_team"))));
		m_eCurMatchEvent = (match_event_t)event->GetInt("type");
		m_flStayDuration = 3.0f;
		char *text;

		switch ((statistic_type_t)event->GetInt("statistic_type"))
		{
		case STATISTIC_SETPIECECOUNT_TEAM:
			text = GetSetPieceCountText(m_eCurMatchEvent, event->GetInt("taking_team"));
			break;
		case STATISTIC_POSSESSION_TEAM:
			text = GetPossessionText();
			break;
		case STATISTIC_SHOTSONGOAL_TEAM:
			text = GetShotsOnGoalText();
			break;
		case STATISTIC_PASSING_TEAM:
			text = GetPassingText();
			break;
		default:
			text = "";
			break;
		}

		m_pExtraInfo->SetText(text);
		m_pExtraInfo->SetVisible(true);
	}
	else if (!Q_strcmp(event->GetName(), "goal"))
	{
		C_SDKPlayer *pScorer = ToSDKPlayer(USERID2PLAYER(event->GetInt("scorer_userid")));
		C_SDKPlayer *pFirstAssister = ToSDKPlayer(USERID2PLAYER(event->GetInt("first_assister_userid")));
		C_SDKPlayer *pSecondAssister = ToSDKPlayer(USERID2PLAYER(event->GetInt("second_assister_userid")));

		m_pNotifications[0]->SetText(VarArgs("GOAL: %s", g_PR->GetTeamCode(event->GetInt("scoring_team"))));

		if (pScorer)
		{
			m_pNotifications[1]->SetText(VarArgs("%s", pScorer->GetPlayerName()));
			m_pNotificationPanel->SetTall(2 * NOTIFICATION_HEIGHT);
		}
		if (pFirstAssister)
		{
			m_pNotifications[2]->SetText(VarArgs("+ %s", pFirstAssister->GetPlayerName()));
			m_pNotificationPanel->SetTall(3 * NOTIFICATION_HEIGHT);
		}
		if (pSecondAssister)
		{
			m_pNotifications[3]->SetText(VarArgs("+ %s", pSecondAssister->GetPlayerName()));
			m_pNotificationPanel->SetTall(4 * NOTIFICATION_HEIGHT);
		}

		m_eCurMatchEvent = MATCH_EVENT_GOAL;
		m_flStayDuration = 1000;

		if (pScorer)
		{
			int count = g_PR->GetShotsOnGoal(pScorer->entindex()) + 1;
			m_pExtraInfo->SetText(VarArgs("%d%s shot on goal for %s", count, GetOrdinal(count), pScorer->GetPlayerName()));
			m_pExtraInfo->SetVisible(true);
		}
	}
	else if (!Q_strcmp(event->GetName(), "own_goal"))
	{
		C_SDKPlayer *pCauser = ToSDKPlayer(USERID2PLAYER(event->GetInt("causer_userid")));

		m_pNotifications[0]->SetText(VarArgs("OWN GOAL: %s", g_PR->GetTeamCode(event->GetInt("causing_team"))));

		if (pCauser)
		{
			m_pNotifications[1]->SetText(VarArgs("%s", pCauser->GetPlayerName()));
			m_pNotificationPanel->SetTall(2 * NOTIFICATION_HEIGHT);
		}

		m_eCurMatchEvent = MATCH_EVENT_OWNGOAL;
		m_flStayDuration = 1000;
	}
	else if (!Q_strcmp(event->GetName(), "foul"))
	{
		C_SDKPlayer *pFoulingPl = ToSDKPlayer(USERID2PLAYER(event->GetInt("fouling_player_userid")));
		C_SDKPlayer *pFouledPl = ToSDKPlayer(USERID2PLAYER(event->GetInt("fouled_player_userid")));
		C_Team *pFoulingTeam = GetGlobalTeam(event->GetInt("fouling_team"));
		match_event_t setpieceType = (match_event_t)event->GetInt("set_piece_type");
		match_event_t foulType = (match_event_t)event->GetInt("foul_type");

		char *typeText;

		m_pNotifications[0]->SetText(VarArgs("%s: %s", g_szMatchEventNames[setpieceType], pFoulingTeam->GetOppTeam()->Get_TeamCode()));
		m_pNotifications[1]->SetText(VarArgs("%s: %s", g_szMatchEventNames[foulType], (pFoulingPl ? pFoulingPl->GetPlayerName() : pFoulingTeam->Get_TeamCode())));
		m_pNotificationPanel->SetTall(2 * NOTIFICATION_HEIGHT);
	
		m_eCurMatchEvent = setpieceType;
		m_flStayDuration = 5.0f;
		char *text;

		switch ((statistic_type_t)event->GetInt("statistic_type"))
		{
		case STATISTIC_DISTANCETOGOAL:
			text = VarArgs("%dm distance to goal", event->GetInt("distance_to_goal"));
			break;
		case STATISTIC_SETPIECECOUNT_TEAM:
			text = GetSetPieceCountText(m_eCurMatchEvent, pFoulingTeam->GetOppTeamNumber());
			break;
		case STATISTIC_FOULS_TEAM:
			text = GetSetPieceCountText(MATCH_EVENT_FOUL, pFoulingTeam->GetTeamNumber());
			break;
		case STATISTIC_OFFSIDES_TEAM:
			text = GetSetPieceCountText(MATCH_EVENT_OFFSIDE, pFoulingTeam->GetTeamNumber());
			break;
		case STATISTIC_POSSESSION_TEAM:
			text = GetPossessionText();
			break;
		default:
			text = "";
			break;
		}	

		m_pExtraInfo->SetText(text);
		m_pExtraInfo->SetVisible(true);
	}
	else if (!Q_strcmp(event->GetName(), "penalty"))
	{
		C_SDKPlayer *pTaker = ToSDKPlayer(USERID2PLAYER(event->GetInt("taking_player_userid")));

		m_pNotifications[0]->SetText(VarArgs("PENALTY: %s", g_PR->GetTeamCode(event->GetInt("taking_team"))));

		if (pTaker)
		{
			m_pNotifications[1]->SetText(VarArgs("%s", pTaker->GetPlayerName()));
			m_pNotificationPanel->SetTall(2 * NOTIFICATION_HEIGHT);
		}

		m_eCurMatchEvent = MATCH_EVENT_PENALTY;
		m_flStayDuration = 5.0f;
	}
	else if (!Q_strcmp(event->GetName(), "timeout_pending"))
	{
		m_nCurMatchEventTeam = event->GetInt("requesting_team");
		m_eCurMatchEvent = MATCH_EVENT_TIMEOUT_PENDING;

		if (m_nCurMatchEventTeam == TEAM_UNASSIGNED)
			m_pNotifications[0]->SetText("TIMEOUT PENDING");
		else
			m_pNotifications[0]->SetText(VarArgs("TIMEOUT PENDING: %s", g_PR->GetTeamCode(m_nCurMatchEventTeam)));

		m_flStayDuration = 1000;
	}
	else if (!Q_strcmp(event->GetName(), "timeout"))
	{
		m_nCurMatchEventTeam = event->GetInt("requesting_team");
		m_eCurMatchEvent = MATCH_EVENT_TIMEOUT;
		m_flStayDuration = 1000;
	}

	m_pCenterFlash->SetText(g_szMatchEventNames[m_eCurMatchEvent]);
}