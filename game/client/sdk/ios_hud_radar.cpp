#include "cbase.h"
#include "ios_hud_radar.h"
#include "c_team.h"
#include "c_ball.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CON_COMMAND_F(captain_increase_offensive_level, "", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	engine->ServerCmd("captain_increase_offensive_level\n");
}

CON_COMMAND_F(captain_decrease_offensive_level, "", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	engine->ServerCmd("captain_decrease_offensive_level\n");
}

enum { WIDTH = 150, HMARGIN = 30, VMARGIN = 150, LEVEL_OUTER_WIDTH = 7, LEVEL_BORDER = 1, LEVEL_MARGIN = 5, LEVEL_INNER_MARGIN = 2 };

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

	SetBounds(ScreenWidth() - HMARGIN - WIDTH - LEVEL_MARGIN - LEVEL_OUTER_WIDTH, VMARGIN, LEVEL_OUTER_WIDTH + LEVEL_MARGIN + WIDTH, WIDTH);
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
	int xOffset = LEVEL_OUTER_WIDTH + LEVEL_MARGIN;

	SetTall(height);

	Color bg = Color(0, 0, 0, 200);
	surface()->DrawSetColor(bg);
	surface()->DrawFilledRect(xOffset, 0, xOffset + width, height);

	Color line = Color(255, 255, 255, 50);
	surface()->DrawSetColor(line);
	surface()->DrawOutlinedRect(xOffset, 0, xOffset + width, height);
	surface()->DrawLine(xOffset, height / 2, xOffset + width, height / 2);
	surface()->DrawOutlinedCircle(xOffset + width / 2, height / 2, mp_shield_kickoff_radius.GetInt() / field.x * width, 24);

	Vector penBox = GetGlobalTeam(TEAM_HOME)->m_vPenBoxMax - GetGlobalTeam(TEAM_HOME)->m_vPenBoxMin;
	penBox /= field;
	penBox.x *= width;
	penBox.y *= height;

	surface()->DrawLine(xOffset + width / 2 - penBox.x / 2, 0, xOffset + width / 2 - penBox.x / 2, penBox.y);
	surface()->DrawLine(xOffset + width / 2 - penBox.x / 2, penBox.y, xOffset + width / 2 + penBox.x / 2, penBox.y);
	surface()->DrawLine(xOffset + width / 2 + penBox.x / 2, penBox.y, xOffset + width / 2 + penBox.x / 2, 0);

	surface()->DrawLine(xOffset + width / 2 - penBox.x / 2, height, xOffset + width / 2 - penBox.x / 2, height - penBox.y);
	surface()->DrawLine(xOffset + width / 2 - penBox.x / 2, height - penBox.y, xOffset + width / 2 + penBox.x / 2, height - penBox.y);
	surface()->DrawLine(xOffset + width / 2 + penBox.x / 2, height - penBox.y, xOffset + width / 2 + penBox.x / 2, height);

	Color levelInner = Color(255, 255, 255, 255);
	Color levelBorder = Color(0, 0, 0, 255);

	const int levelCount = 5;

	int sectionHeight = (height - (levelCount - 1) * LEVEL_INNER_MARGIN - levelCount * 2 * LEVEL_BORDER) / levelCount;

	for (int i = -(levelCount - 1) / 2; i <= (levelCount - 1) / 2; i++)
	{
		if (pLocal->GetTeam()->GetOffensiveLevel() >= 0 && (i < 0 || i > pLocal->GetTeam()->GetOffensiveLevel())
			|| pLocal->GetTeam()->GetOffensiveLevel() < 0 && (i > 0 || i < pLocal->GetTeam()->GetOffensiveLevel()))
			continue;

		int yMargin = ((levelCount - 1) - (i + (levelCount - 1) / 2)) * (sectionHeight + 2 * LEVEL_BORDER + LEVEL_INNER_MARGIN);

		surface()->DrawSetColor(levelBorder);
		surface()->DrawFilledRect(0, yMargin, LEVEL_OUTER_WIDTH, yMargin + sectionHeight + 2 * LEVEL_BORDER);
		surface()->DrawSetColor(levelInner);
		surface()->DrawFilledRect(LEVEL_BORDER, yMargin + LEVEL_BORDER, LEVEL_OUTER_WIDTH - LEVEL_BORDER, yMargin + LEVEL_BORDER + sectionHeight);
	}

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
			surface()->DrawFilledRect(xOffset + pos.x - 2, pos.y - 2, xOffset + pos.x + 2, pos.y + 2);
			surface()->DrawOutlinedCircle(xOffset + pos.x, pos.y, 5, 12);
		}
		else if (GameResources()->GetTeamPosType(pPl->index) == POS_GK)
		{
			surface()->DrawFilledRect(xOffset + pos.x - 1, pos.y - 1, xOffset + pos.x + 1, pos.y + 1);
			surface()->DrawOutlinedRect(xOffset + pos.x - 3, pos.y - 3, xOffset + pos.x + 3, pos.y + 3);
		}
		else
		{
			surface()->DrawFilledRect(xOffset + pos.x - 3, pos.y - 3, xOffset + pos.x + 3, pos.y + 3);
		}
	}

	if (GetMatchBall())
	{
		Vector pos = GetMatchBall()->GetLocalOrigin() - SDKGameRules()->m_vFieldMin;
		pos /= field;

		pos.x = clamp(pos.x, 0.0f, 1.0f);

		if (pLocal->GetTeamNumber() != SDKGameRules()->m_nBottomTeam)
			pos.x = 1.0f - pos.x;

		pos.x *= width;

		pos.y = (1.0f - clamp(pos.y, 0.0f, 1.0f));

		if (pLocal->GetTeamNumber() != SDKGameRules()->m_nBottomTeam)
			pos.y = 1.0f - pos.y;

		pos.y *= height;

		surface()->DrawSetColor(Color(255, 255, 255, 255));
		surface()->DrawOutlinedCircle(xOffset + pos.x, pos.y, 1, 12);
		surface()->DrawOutlinedCircle(xOffset + pos.x, pos.y, 2, 12);
		surface()->DrawOutlinedCircle(xOffset + pos.x, pos.y, 3, 12);
	}
}

void CHudRadar::Init()
{

}

void CHudRadar::Reset()
{

}