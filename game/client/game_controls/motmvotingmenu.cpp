#include "cbase.h"
#include "motmvotingmenu.h"
#include "c_playerresource.h"
#include "sdk_gamerules.h"

using namespace vgui;

enum { PANEL_WIDTH = 500, PANEL_HEIGHT = 600 };
enum { NAME_HEIGHT = 30, NAME_VMARGIN = 5 };
enum { VOTE_WIDTH = 100, VOTE_HEIGHT = 30 };

CMotmVotingMenu::CMotmVotingMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_MOTMVOTING)
{
	SetScheme("SourceScheme");

	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	SetProportional( false );
	SetTitleBarVisible( true );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetVisible( false );
	SetTitle("Select your Man Of The Match", false);

	m_pVote = new Button(this, "", "Vote", this, "vote");

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i] = new Panel(this);

		for (int j = 0; j < 11; j++)
		{
			m_pPlayerNames[i][j] = new RadioButton(m_pTeamPanels[i], "", "");
			m_pPlayerNames[i][j]->SetVisible(false);
		}
	}
}

void CMotmVotingMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
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

	SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);

	m_pVote->SetBounds(0, GetTall() - VOTE_HEIGHT, VOTE_WIDTH, VOTE_HEIGHT);

	for (int i = 0; i < 2; i++)
	{
		m_pTeamPanels[i]->SetBounds(i * (GetWide() / 2), 0, GetWide() / 2, GetTall() - VOTE_HEIGHT);

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

	MoveToCenterOfScreen();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!g_PR->IsConnected(i) || g_PR->GetTeam(i) != TEAM_A && g_PR->GetTeam(i) != TEAM_B)
			continue;

		RadioButton *pRadio = m_pPlayerNames[g_PR->GetTeam(i) - TEAM_A][g_PR->GetTeamPosIndex(i)];
		pRadio->SetText(VarArgs("%s - %s", g_szPosNames[g_PR->GetTeamPosType(i)], g_PR->GetPlayerName(i)));
		pRadio->SetCommand(VarArgs("%d", i));
		pRadio->SetEnabled(true);
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
		Close();
	}
	else
		BaseClass::OnCommand(cmd);
}