#include "cbase.h"
#include "ios_emote_menu.h"
#include <igameresources.h>
#include "c_sdk_player.h"
#include "sdk_gamerules.h"

using namespace vgui;

CEmoteMenu::CEmoteMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_ACTION)
{
	SetTitle("", true);
	SetMoveable(false);
	SetSizeable(false);
	SetTitleBarVisible( false );
	SetProportional(false);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	m_szEmote[0] = '\0';

	for (int i = 0; i < EMOTE_COUNT; i++)
	{
		m_pEmoteButtons[i] = new Button(this, "", g_szEmotes[i], this, g_szEmotes[i]);
	}
}

CEmoteMenu::~CEmoteMenu()
{
}

void CEmoteMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScheme = pScheme;
}

void CEmoteMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	SetBounds(0, 0, ScreenWidth(), ScreenHeight());

	const float step = 2.0f * M_PI / EMOTE_COUNT;

	for (int i = 0; i < EMOTE_COUNT; i++)
	{
		m_pEmoteButtons[i]->SetSize(180, 30);
		m_pEmoteButtons[i]->SetDefaultColor(Color(200, 200, 200, 255), Color(0, 0, 0, 200));
		m_pEmoteButtons[i]->SetArmedColor(Color(200, 200, 200, 255), Color(50, 50, 50, 200));
		m_pEmoteButtons[i]->SetDepressedColor(Color(200, 200, 200, 255), Color(0, 0, 0, 200));
		m_pEmoteButtons[i]->SetFont(m_pScheme->GetFont("IOSTeamMenuBig"));
		m_pEmoteButtons[i]->SetPaintBorderEnabled(false);
		m_pEmoteButtons[i]->SetCursor(dc_hand);
		m_pEmoteButtons[i]->SetContentAlignment(Label::a_center);

		m_pEmoteButtons[i]->SetPos(GetWide() / 2 - m_pEmoteButtons[i]->GetWide() / 2 + sin(step * i) * 300, GetTall() / 2 - m_pEmoteButtons[i]->GetTall() / 2 + cos(step * i) * -250);
	}
}

void CEmoteMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
		SetKeyBoardInputEnabled( false );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );

		if (m_szEmote[0] != '\0')
			engine->ClientCmd(VarArgs("say_team %s", m_szEmote));
	}
}

void CEmoteMenu::OnKeyCodePressed(KeyCode code)
{
	BaseClass::OnKeyCodePressed( code );
}

void CEmoteMenu::OnCommand( char const *cmd )
{
}

void CEmoteMenu::Reset()
{
}

bool CEmoteMenu::NeedsUpdate()
{
	return false;
}

void CEmoteMenu::Update()
{
}

void CEmoteMenu::OnCursorEntered(Panel *panel)
{
	const char *cmd = ((Button *)panel)->GetCommand()->GetString("command");
	Q_strncpy(m_szEmote, cmd, sizeof(m_szEmote));
}

void CEmoteMenu::OnCursorExited(Panel *panel)
{
	m_szEmote[0] = '\0';
}