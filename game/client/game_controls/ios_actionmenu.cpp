#include "cbase.h"
#include "ios_actionmenu.h"
#include <igameresources.h>
#include "c_sdk_player.h"
#include "sdk_gamerules.h"

using namespace vgui;

CActionMenu::CActionMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_ACTION)
{
	//SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
	SetTitle("", true);
	SetMoveable(false);
	SetSizeable(false);
	SetTitleBarVisible( false );
	SetProportional(false);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	//SetBgColor(Color(100, 100, 100, 255));

	for (int i = 0; i < MAX_TEAM_PLAYERS; i++)
	{
		m_pPlayerButtons[i] = new Button(this, "", "");
	}

	m_flNextUpdateTime = gpGlobals->curtime;
}

CActionMenu::~CActionMenu()
{
}

void CActionMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScheme = pScheme;
}

void CActionMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	SetBounds(0, 0, ScreenWidth(), ScreenHeight());

	//MoveToCenterOfScreen();

	for (int i = 0; i < MAX_TEAM_PLAYERS; i++)
	{
		m_pPlayerButtons[i]->SetSize(200, 30);
		m_pPlayerButtons[i]->SetDefaultColor(Color(200, 200, 200, 255), Color(0, 0, 0, 200));
		m_pPlayerButtons[i]->SetArmedColor(Color(200, 200, 200, 255), Color(50, 50, 50, 200));
		m_pPlayerButtons[i]->SetDepressedColor(Color(200, 200, 200, 255), Color(0, 0, 0, 200));
		m_pPlayerButtons[i]->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
		m_pPlayerButtons[i]->SetPaintBorderEnabled(false);
		m_pPlayerButtons[i]->SetCursor(dc_hand);
		m_pPlayerButtons[i]->SetVisible(false);
	}
}

void CActionMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();
		m_flNextUpdateTime = gpGlobals->curtime;
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

void CActionMenu::OnKeyCodePressed(KeyCode code)
{
	if (code == KEY_TAB)
		Close();
	else
		BaseClass::OnKeyCodePressed( code );
}

void CActionMenu::OnCommand( char const *cmd )
{
	if (!strncmp(cmd, "selectplayer", 12))
	{
		engine->ClientCmd(cmd);
	}

	BaseClass::OnCommand(cmd);

	//Close();
}

void CActionMenu::Reset()
{
	m_flNextUpdateTime = gpGlobals->curtime;
}

bool CActionMenu::NeedsUpdate()
{
	return m_flNextUpdateTime <= gpGlobals->curtime;
}

void CActionMenu::Update()
{
	m_flNextUpdateTime = gpGlobals->curtime + 0.25f;

	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();
	if (!pLocal)
		return;

	IGameResources *gr = GameResources();
	if (!gr)
		return;

	int buttonIndex = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!gr->IsConnected(i) || i == pLocal->entindex() || gr->GetTeam(i) != pLocal->GetTeamNumber())
			continue;

		m_pPlayerButtons[buttonIndex]->SetText(VarArgs("%s | %s", g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][gr->GetTeamPosIndex(i)][POS_NAME]], gr->GetPlayerName(i)));
		m_pPlayerButtons[buttonIndex]->SetCommand(VarArgs("selectplayer %d", i));
		buttonIndex += 1;
	}

	if (buttonIndex == 0)
		return;

	const float step = 2.0f * M_PI / buttonIndex;

	for (int i = 0; i < MAX_TEAM_PLAYERS; i++)
	{
		if (i < buttonIndex)
		{
			m_pPlayerButtons[i]->SetPos(GetWide() / 2 - m_pPlayerButtons[i]->GetWide() / 2 + sin(step * i) * 300, GetTall() / 2 - m_pPlayerButtons[i]->GetTall() / 2 + cos(step * i) * -200);
			m_pPlayerButtons[i]->SetVisible(true);
		}
		else
		{
			m_pPlayerButtons[i]->SetVisible(false);
		}
	}
}