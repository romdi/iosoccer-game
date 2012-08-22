#include "cbase.h"
#include "motmvotingmenu.h"
#include "c_playerresource.h"
#include "sdk_gamerules.h"

using namespace vgui;

enum { PANEL_WIDTH = 500, PANEL_HEIGHT = 600 };
enum { NAME_HEIGHT = 30, NAME_VMARGIN = 5 };
enum { VOTE_WIDTH = 100, VOTE_HEIGHT = 30 };

void ShowMotmVoting()
{
	gViewPortInterface->ShowPanel(PANEL_MOTMVOTING, true);
}

ConCommand showmotmvoting("showmotmvoting", ShowMotmVoting);

CMotmVotingMenu::CMotmVotingMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_MOTMVOTING)
{
	SetScheme("ClientScheme");

	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	SetProportional( false );
	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetVisible( false );
	SetTitle("", false);

	m_pMainPanel = new Panel(this);

	m_pVote = new Button(m_pMainPanel, "", "Vote", this, "vote");

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i] = new Panel(m_pMainPanel);

		for (int j = 0; j < 11; j++)
		{
			m_pPlayerNames[i][j] = new RadioButton(m_pTeamPanels[i], "", "");
			m_pPlayerNames[i][j]->SetVisible(false);
		}
	}

	MakePopup();
}

void CMotmVotingMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBounds(0, 0, ScreenWidth(), ScreenHeight());
	SetPaintBackgroundEnabled(false);

	m_pMainPanel->SetBounds(GetWide() / 2 - PANEL_WIDTH / 2, GetTall() / 2 - PANEL_HEIGHT / 2, PANEL_WIDTH, PANEL_HEIGHT);
	m_pMainPanel->SetBgColor(Color(0, 0, 0, 240));
	m_pMainPanel->SetPaintBackgroundType(2);

	m_pVote->SetBounds(PANEL_WIDTH - VOTE_WIDTH, PANEL_HEIGHT - VOTE_HEIGHT, VOTE_WIDTH, VOTE_HEIGHT);
}

void CMotmVotingMenu::Update()
{
}

void CMotmVotingMenu::ShowPanel(bool state)
{
	//if (BaseClass::IsVisible() == state)
	//	return;

	if (state)
	{
		Reset();
		Activate();
		SetMouseInputEnabled( true );
		SetKeyBoardInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
	}
}

void CMotmVotingMenu::Reset()
{
	if (!g_PR)
		return;

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i]->SetBounds(i * (PANEL_WIDTH / 2), 0, PANEL_WIDTH / 2, PANEL_HEIGHT - VOTE_HEIGHT);

		for (int j = 0; j < 11; j++)
		{
			m_pPlayerNames[i][j]->SetBounds(0, j * (NAME_HEIGHT + NAME_VMARGIN), m_pTeamPanels[i]->GetWide(), NAME_HEIGHT);
			if (j < mp_maxplayers.GetInt())
			{
				m_pPlayerNames[i][j]->SetText(VarArgs("%s", g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][j][POS_TYPE]]));
				m_pPlayerNames[i][j]->SetVisible(true);
				m_pPlayerNames[i][j]->SetEnabled(false);
			}
			else
				m_pPlayerNames[i][j]->SetVisible(false);
		}
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!g_PR->IsConnected(i) || g_PR->GetTeam(i) != TEAM_A && g_PR->GetTeam(i) != TEAM_B)
			continue;

		RadioButton *pRadio = m_pPlayerNames[g_PR->GetTeam(i) - TEAM_A][g_PR->GetTeamPosIndex(i)];
		pRadio->SetText(VarArgs("[%s] %s", g_szPosNames[g_PR->GetTeamPosType(i)], g_PR->GetPlayerName(i)));
		pRadio->SetCommand(VarArgs("%d", i));
		pRadio->SetEnabled(i != GetLocalPlayerIndex());
	}
}

void CMotmVotingMenu::OnCommand(const char *cmd)
{
	if (!Q_strcmp(cmd, "vote"))
	{
		int selectedPlayerIds[2] = {};

		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < mp_maxplayers.GetInt(); j++)
			{
				if (!m_pPlayerNames[i][j]->IsSelected())
					continue;

				selectedPlayerIds[i] = m_pPlayerNames[i][j]->GetCommand()->GetFirstValue()->GetInt();
				break;
			}
		}
		engine->ClientCmd(VarArgs("motmvote %d %d", selectedPlayerIds[0], selectedPlayerIds[1]));
		ShowPanel(false);
	}
	else
		BaseClass::OnCommand(cmd);
}