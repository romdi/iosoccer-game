#include "cbase.h"
#include "ios_hud_radar.h"
#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum { WIDTH = 150, HMARGIN = 30, VMARGIN = 150 };

CHudRadar::CHudRadar(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudRadar")
{
	SetHiddenBits(HIDEHUD_POWERSHOTBAR);

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_flNextUpdate = gpGlobals->curtime;
}

void CHudRadar::ApplySchemeSettings(IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetBounds(ScreenWidth() - HMARGIN - WIDTH, VMARGIN, WIDTH, WIDTH);
}

bool CHudRadar::ShouldDraw()
{
	if (!CHudElement::ShouldDraw())
		return false;

	C_SDKPlayer *pPlayer = C_SDKPlayer::GetLocalSDKPlayer();
	if (!pPlayer || pPlayer->GetTeamNumber() != TEAM_HOME && pPlayer->GetTeamNumber() != TEAM_AWAY)
		return false;

	if (GetMatchBall() && GetMatchBall()->m_eBallState == BALL_STATE_GOAL)
		return false;

	if (GetReplayManager() && GetReplayManager()->IsReplaying())
		return false;

	if (SDKGameRules()->IsCeremony())
		return false;

	return true;
}

void CHudRadar::OnThink(void)
{
	BaseClass::OnThink();
}

void CHudRadar::Paint()
{
	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();

	if (!pLocal)
		return;

	Vector field = SDKGameRules()->m_vFieldMax - SDKGameRules()->m_vFieldMin;
	int width = WIDTH;
	int height = WIDTH * (field.y / field.x);

	SetTall(height);

	Color bg = Color(0, 0, 0, 200);
	surface()->DrawSetColor(bg);
	surface()->DrawFilledRect(0, 0, width, height);

	Color line = Color(255, 255, 255, 50);
	surface()->DrawSetColor(line);
	surface()->DrawOutlinedRect(0, 0, width, height);
	surface()->DrawLine(0, height / 2, width, height / 2);
	surface()->DrawOutlinedCircle(width / 2, height / 2, mp_shield_kickoff_radius.GetInt() / field.x * width, 24);

	Vector penBox = GetGlobalTeam(TEAM_HOME)->m_vPenBoxMax - GetGlobalTeam(TEAM_HOME)->m_vPenBoxMin;
	penBox /= field;
	penBox.x *= width;
	penBox.y *= height;

	surface()->DrawLine(width / 2 - penBox.x / 2, 0, width / 2 - penBox.x / 2, penBox.y);
	surface()->DrawLine(width / 2 - penBox.x / 2, penBox.y, width / 2 + penBox.x / 2, penBox.y);
	surface()->DrawLine(width / 2 + penBox.x / 2, penBox.y, width / 2 + penBox.x / 2, 0);

	surface()->DrawLine(width / 2 - penBox.x / 2, height, width / 2 - penBox.x / 2, height - penBox.y);
	surface()->DrawLine(width / 2 - penBox.x / 2, height - penBox.y, width / 2 + penBox.x / 2, height - penBox.y);
	surface()->DrawLine(width / 2 + penBox.x / 2, height - penBox.y, width / 2 + penBox.x / 2, height);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		C_SDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl || pPl->IsDormant())
			continue;

		Vector pos = pPl->GetLocalOrigin() - SDKGameRules()->m_vFieldMin;
		pos /= field;

		pos.x = clamp(pos.x, 0.0f, 1.0f);

		if (pLocal->GetTeamNumber() != SDKGameRules()->m_nBottomTeam)
			pos.x = 1.0f - pos.x;

		pos.x *= width;

		pos.y = (1.0f - clamp(pos.y, 0.0f, 1.0f));

		if (pLocal->GetTeamNumber() != SDKGameRules()->m_nBottomTeam)
			pos.y = 1.0f - pos.y;

		pos.y *= height;

		Color c = GetGlobalTeam(pPl->GetTeamNumber())->GetHudKitColor();

		surface()->DrawSetColor(c);

		if (pPl == pLocal)
		{
			surface()->DrawFilledRect(pos.x - 3, pos.y - 3, pos.x + 3, pos.y + 3);
			surface()->DrawOutlinedCircle(pos.x, pos.y, 6, 12);
		}
		else if (GameResources()->GetTeamPosType(pPl->index) == POS_GK)
		{
			surface()->DrawFilledRect(pos.x - 2, pos.y - 2, pos.x + 2, pos.y + 2);
			surface()->DrawOutlinedRect(pos.x - 4, pos.y - 4, pos.x + 4, pos.y + 4);
		}
		else
		{
			surface()->DrawFilledRect(pos.x - 4, pos.y - 4, pos.x + 4, pos.y + 4);
		}
	}
}

void CHudRadar::Init()
{

}

void CHudRadar::Reset()
{

}