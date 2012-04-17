#include "cbase.h"
#include "matchmenu.h"
#include "statsmenu.h"
#include "settingsmenu.h"

using namespace vgui;

#define BUTTON_WIDTH		200
#define BUTTON_HEIGHT		50
#define BUTTON_MARGIN		5
#define BUTTON_LEFTMARGIN	10	
#define PANEL_MARGIN		5
#define PANEL_WIDTH			(1024 - 2 * PANEL_MARGIN)
#define PANEL_HEIGHT		(768 - 2 * PANEL_MARGIN)

CMatchMenu::CMatchMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_MATCH)
{
	SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
	SetTitle("", true);
	SetMoveable(false);
	SetSizeable(false);
	SetTitleBarVisible( false );
	SetProportional(false);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	//SetBgColor(Color(100, 100, 100, 255));

	//m_pTeamMenu->Activate();
	m_pTabPanels[TAB_TEAM_JOIN] = new CTeamMenu(this, "");
	m_pTabPanels[TAB_TEAM_STATS] = new CStatsMenu(this, "");
	m_pTabPanels[TAB_PLAYER_SETTINGS] = new CSettingsMenu(this, "");

	for (int i = 0; i < TAB_COUNT; i++)
	{
		m_pTabButtons[i] = new Button(this, "", "");
		m_pTabPanels[i]->SetVisible(false);
	}	

	m_nActiveTab = TAB_TEAM_JOIN;
	m_pTabPanels[TAB_TEAM_JOIN]->SetVisible(true);

	m_flNextUpdateTime = gpGlobals->curtime;
}

CMatchMenu::~CMatchMenu()
{
}

void CMatchMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScheme = pScheme;
}

void CMatchMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	MoveToCenterOfScreen();

	for (int i = 0; i < TAB_COUNT; i++)
	{
		m_pTabButtons[i]->SetBounds(BUTTON_LEFTMARGIN + i * (BUTTON_WIDTH + BUTTON_MARGIN), 0, BUTTON_WIDTH, BUTTON_HEIGHT);
		m_pTabButtons[i]->SetCommand(VarArgs("showtab %d", i));
		m_pTabButtons[i]->SetDefaultColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
		m_pTabButtons[i]->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
		m_pTabButtons[i]->SetDepressedColor(Color(100, 100, 100, 255), Color(200, 200, 200, 255));
		m_pTabButtons[i]->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
		m_pTabButtons[i]->SetPaintBorderEnabled(false);
		m_pTabButtons[i]->SetCursor(dc_hand);

		m_pTabPanels[i]->SetBounds(0, BUTTON_HEIGHT, PANEL_WIDTH, PANEL_HEIGHT - BUTTON_HEIGHT);
	}

	m_pTabButtons[TAB_TEAM_JOIN]->SetText("Teams");
	m_pTabButtons[TAB_TEAM_STATS]->SetText("Statistics");
	m_pTabButtons[TAB_PLAYER_SETTINGS]->SetText("Settings");
}

void CMatchMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();
		dynamic_cast<CTeamMenu *>(m_pTabPanels[TAB_TEAM_JOIN])->m_flNextUpdateTime = gpGlobals->curtime;
		dynamic_cast<CStatsMenu *>(m_pTabPanels[TAB_TEAM_STATS])->m_flNextUpdateTime = gpGlobals->curtime;
		dynamic_cast<CSettingsMenu *>(m_pTabPanels[TAB_PLAYER_SETTINGS])->m_flNextUpdateTime = gpGlobals->curtime;
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

void CMatchMenu::OnKeyCodePressed(KeyCode code)
{
	if (code == KEY_TAB)
		Close();
	else
		BaseClass::OnKeyCodePressed( code );
}

void CMatchMenu::OnCommand( char const *cmd )
{
	if (!strncmp(cmd, "showtab", 7))
	{
		int oldActiveTab = m_nActiveTab;
		m_nActiveTab = atoi(&cmd[8]);

		if (m_nActiveTab != oldActiveTab)
		{
			m_pTabPanels[oldActiveTab]->SetVisible(false);
			m_pTabPanels[m_nActiveTab]->SetVisible(true);
		}
	}

	BaseClass::OnCommand(cmd);

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CMatchMenu::Reset()
{
	m_flNextUpdateTime = gpGlobals->curtime;
}

bool CMatchMenu::NeedsUpdate()
{
	return m_flNextUpdateTime <= gpGlobals->curtime;
}

void CMatchMenu::Update()
{
	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
}