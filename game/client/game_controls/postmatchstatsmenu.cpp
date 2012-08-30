#include "cbase.h"
#include "postmatchstatsmenu.h"
#include "c_playerresource.h"
#include "sdk_gamerules.h"

using namespace vgui;

enum { PANEL_WIDTH = 500, PANEL_HEIGHT = 600 };
enum { NAME_HEIGHT = 30, NAME_VMARGIN = 5 };
enum { VOTE_WIDTH = 100, VOTE_HEIGHT = 30 };
enum { STAT_WIDTH = 400, STAT_HEIGHT = 100, STAT_VMARGIN = 5, STAT_TEXT_WIDTH = 100, STAT_TEXT_HEIGHT = 30 };
enum { CLOSE_WIDTH = 50, CLOSE_HEIGHT = 30, CLOSE_MARGIN = 5 };

void ShowPostMatchStats()
{
	gViewPortInterface->ShowPanel(PANEL_POSTMATCHSTATS, true);
}

//ConCommand showpostmatchstats("showpostmatchstats", ShowPostMatchStats);

CPostMatchStatsMenu::CPostMatchStatsMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_POSTMATCHSTATS)
{
	SetScheme("ClientScheme");

	m_pViewPort = pViewPort;

	//SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	SetProportional( false );
	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetVisible( false );
	//SetTitle("Post Match Statistics", false);
	SetTitle("", false);

	m_pMainPanel = new Panel(this);

	m_pMatchStats[LONGEST_DISTANCE_COVERED] = new MatchStat(m_pMainPanel, "Longest Distance Covered");
	m_pMatchStats[SHORTEST_DISTANCE_COVERED] = new MatchStat(m_pMainPanel, "Shortest Distance Covered");
	m_pMatchStats[HIGHEST_POSSESSION] = new MatchStat(m_pMainPanel, "Highest Ball Possession");
	m_pMatchStats[PLAYERS_CHOICE_MOTM] = new MatchStat(m_pMainPanel, "Players' Choice MOTM");
	m_pMatchStats[EXPERTS_CHOICE_MOTM] = new MatchStat(m_pMainPanel, "Experts' Choice MOTM");

	m_pClose = new Button(m_pMainPanel, "", "Close", this, "close");

	ListenForGameEvent("motmvotingresult");

	for (int i = 0; i < 2; i++)
	{
		m_nPlayersChoiceMotm[i] = 0;
		m_nPlayersChoiceMotmPercentage[i] = 0;
		m_nExpertsChoiceMotm[i] = 0;
		m_nExpertsChoiceMotmPercentage[i] = 0;
	}

	MakePopup();
}

void CPostMatchStatsMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBounds(0, 0, ScreenWidth(), ScreenHeight());
	SetPaintBackgroundEnabled(false);

	m_pMainPanel->SetBounds(GetWide() / 2 - PANEL_WIDTH / 2, GetTall() / 2 - PANEL_HEIGHT / 2, PANEL_WIDTH, PANEL_HEIGHT);
	m_pMainPanel->SetBgColor(Color(0, 0, 0, 240));
	m_pMainPanel->SetPaintBackgroundType(2);

	for (int i = 0; i < MATCH_STATS_COUNT; i++)
	{
		m_pMatchStats[i]->pContainer->SetBounds(PANEL_WIDTH / 2 - STAT_WIDTH / 2, i * (STAT_HEIGHT + STAT_VMARGIN), STAT_WIDTH, STAT_HEIGHT);
		m_pMatchStats[i]->pName->SetBounds(0, 0, STAT_WIDTH, STAT_TEXT_HEIGHT);
		m_pMatchStats[i]->pName->SetContentAlignment(Label::Alignment::a_center);

		for (int j = 0; j < 2; j++)
		{
			m_pMatchStats[i]->pPlayers[j]->SetBounds(j * 2 * STAT_TEXT_WIDTH, STAT_TEXT_HEIGHT, STAT_TEXT_WIDTH, STAT_TEXT_HEIGHT);
			m_pMatchStats[i]->pPlayers[j]->SetContentAlignment(Label::Alignment::a_center);
			m_pMatchStats[i]->pValues[j]->SetBounds(STAT_TEXT_WIDTH + j * 2 * STAT_TEXT_WIDTH, STAT_TEXT_HEIGHT, STAT_TEXT_WIDTH, STAT_TEXT_HEIGHT);
			m_pMatchStats[i]->pValues[j]->SetContentAlignment(Label::Alignment::a_center);
		}
	}

	m_pClose->SetBounds(PANEL_WIDTH - CLOSE_WIDTH - CLOSE_MARGIN, PANEL_HEIGHT - CLOSE_HEIGHT - CLOSE_MARGIN, CLOSE_WIDTH, CLOSE_HEIGHT);
}

void CPostMatchStatsMenu::Update()
{
}

void CPostMatchStatsMenu::ShowPanel(bool state)
{
	//if (BaseClass::IsVisible() == state)
	//	return;

	if (state)
	{
		gViewPortInterface->ShowPanel(PANEL_MOTMVOTING, false);
		Reset();
		Activate();
		SetMouseInputEnabled( true );
		//SetKeyBoardInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
		//SetKeyBoardInputEnabled( false );
	}
}

void CPostMatchStatsMenu::Reset()
{
	if (!g_PR)
		return;

	int shortestDist[2] = { INT_MAX, INT_MAX };
	int longestDist[2] = { -INT_MAX, -INT_MAX };
	int shortestDistPlayer[2] = { 0, 0 };
	int longestDistPlayer[2] = { 0, 0 };
	int highestPossession[2] = { -INT_MAX, -INT_MAX };
	int highestPossessionPlayer[2] = { 0, 0 };

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!g_PR->IsConnected(i) || g_PR->GetTeam(i) != TEAM_A && g_PR->GetTeam(i) != TEAM_B)
			continue;

		int teamIndex = g_PR->GetTeam(i) - TEAM_A;

		int dist = g_PR->GetDistanceCovered(i);

		if (dist < shortestDist[teamIndex])
		{
			shortestDist[teamIndex] = dist;
			shortestDistPlayer[teamIndex] = i;
		}

		if (dist > longestDist[teamIndex])
		{
			longestDist[teamIndex] = dist;
			longestDistPlayer[teamIndex] = i;
		}

		if (g_PR->GetPossession(i) > highestPossession[teamIndex])
		{
			highestPossession[teamIndex] = g_PR->GetPossession(i);
			highestPossessionPlayer[teamIndex] = i;
		}
	}

	for (int i = 0; i < MATCH_STATS_COUNT; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			m_pMatchStats[i]->pPlayers[j]->SetText("");
			m_pMatchStats[i]->pValues[j]->SetText("");
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (longestDistPlayer[i] != 0)
		{
			m_pMatchStats[LONGEST_DISTANCE_COVERED]->pPlayers[i]->SetText(g_PR->GetPlayerName(longestDistPlayer[i]));
			m_pMatchStats[LONGEST_DISTANCE_COVERED]->pValues[i]->SetText(VarArgs("(%.1f km)", longestDist[i] / 1000.0f));
		}

		if (shortestDistPlayer[i] != 0)
		{
			m_pMatchStats[SHORTEST_DISTANCE_COVERED]->pPlayers[i]->SetText(g_PR->GetPlayerName(shortestDistPlayer[i]));
			m_pMatchStats[SHORTEST_DISTANCE_COVERED]->pValues[i]->SetText(VarArgs("(%.1f km)", shortestDist[i] / 1000.0f));
		}

		if (highestPossessionPlayer[i] != 0)
		{
			m_pMatchStats[HIGHEST_POSSESSION]->pPlayers[i]->SetText(g_PR->GetPlayerName(highestPossessionPlayer[i]));
			m_pMatchStats[HIGHEST_POSSESSION]->pValues[i]->SetText(VarArgs("(%d%%)", highestPossession[i]));
		}

		if (m_nPlayersChoiceMotm[i] != 0)
		{
			m_pMatchStats[PLAYERS_CHOICE_MOTM]->pPlayers[i]->SetText(g_PR->GetPlayerName(m_nPlayersChoiceMotm[i]));
			m_pMatchStats[PLAYERS_CHOICE_MOTM]->pValues[i]->SetText(VarArgs("(%d%%)", m_nPlayersChoiceMotmPercentage[i]));
		}

		if (m_nExpertsChoiceMotm[i] != 0)
		{
			m_pMatchStats[EXPERTS_CHOICE_MOTM]->pPlayers[i]->SetText(g_PR->GetPlayerName(m_nExpertsChoiceMotm[i]));
			m_pMatchStats[EXPERTS_CHOICE_MOTM]->pValues[i]->SetText(VarArgs("(%d%%)", m_nExpertsChoiceMotmPercentage[i]));
		}
	}

	MoveToCenterOfScreen();
}

void CPostMatchStatsMenu::OnCommand(const char *cmd)
{
	if (!Q_strcmp(cmd, "close"))
	{
		ShowPanel(false);
	}
	else
		BaseClass::OnCommand(cmd);
}

void CPostMatchStatsMenu::FireGameEvent(IGameEvent *event)
{
	if (!g_PR)
		return;

	if (Q_strcmp(event->GetName(), "motmvotingresult"))
		return;

	m_nPlayersChoiceMotm[0] = event->GetInt("playerschoice_player0");
	m_nPlayersChoiceMotmPercentage[0] = event->GetInt("playerschoice_percentage0");
	m_nPlayersChoiceMotm[1] = event->GetInt("playerschoice_player1");
	m_nPlayersChoiceMotmPercentage[1] = event->GetInt("playerschoice_percentage1");

	m_nExpertsChoiceMotm[0] = event->GetInt("expertschoice_player0");
	m_nExpertsChoiceMotmPercentage[0] = event->GetInt("expertschoice_percentage0");
	m_nExpertsChoiceMotm[1] = event->GetInt("expertschoice_player1");
	m_nExpertsChoiceMotmPercentage[1] = event->GetInt("expertschoice_percentage1");

	ShowPanel(true);
}