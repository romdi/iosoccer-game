#include "cbase.h"
#include "ios_hud_radar.h"
#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_radar_show("cl_radar_show", "1");

enum { SIZE = 200, HMARGIN = 30, VMARGIN = 200 };

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

	SetBounds(ScreenWidth() - HMARGIN - SIZE, VMARGIN, SIZE, SIZE);
}

bool CHudRadar::ShouldDraw()
{
	if (!CHudElement::ShouldDraw())
		return false;

	if (!cl_radar_show.GetBool())
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

	float rotation = -::input->GetCameraAngles()[YAW] - 90;

	Vector field = SDKGameRules()->m_vFieldMax - SDKGameRules()->m_vFieldMin;
	float height = SIZE * 0.75f;
	float width = height / (field.y / field.x);

	Color black = Color(0, 0, 0, 255);
	surface()->DrawSetColor(black);

	const int pointCount = 6;

	int pointsX[pointCount];
	pointsX[0] = -width / 2;
	pointsX[1] = width / 2;
	pointsX[2] = width / 2;
	pointsX[3] = -width / 2;
	pointsX[4] = -width / 2;
	pointsX[5] = width / 2;

	int pointsY[pointCount];
	pointsY[0] = -height / 2;
	pointsY[1] = -height / 2;
	pointsY[2] = height / 2;
	pointsY[3] = height / 2;
	pointsY[4] = 0;
	pointsY[5] = 0;

	for (int i = 0; i < pointCount; i++)
	{
		Vector point = Vector(pointsX[i], pointsY[i], 0);
		VectorYawRotate(point, -rotation, point);
		pointsX[i] = point.x + SIZE / 2;
		pointsY[i] = point.y + SIZE / 2;
	}

	surface()->DrawPolyLine(pointsX, pointsY, pointCount - 2);
	surface()->DrawLine(pointsX[4], pointsY[4], pointsX[5], pointsY[5]);

	const int radius = 4;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		C_SDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl || pPl->IsDormant())
			continue;

		Vector offset = pPl->GetLocalOrigin() - SDKGameRules()->m_vKickOff;
		offset /= field / 2;
		offset.x *= width / 2;
		offset.y *= height / 2;

		VectorYawRotate(offset, rotation, offset);
		
		Color c = GetGlobalTeam(pPl->GetTeamNumber())->GetHudKitColor();

		surface()->DrawSetColor(c);

		if (pPl == pLocal)
		{
			surface()->DrawOutlinedRect(SIZE / 2 - offset.x - radius, SIZE / 2 + offset.y - radius, SIZE / 2 - offset.x + radius, SIZE / 2 + offset.y + radius);
		}
		else if (GameResources()->GetTeamPosType(pPl->index) == POS_GK)
		{
			surface()->DrawLine(SIZE / 2 - offset.x - radius, SIZE / 2 + offset.y - radius, SIZE / 2 - offset.x + radius, SIZE / 2 + offset.y + radius);
			surface()->DrawLine(SIZE / 2 - offset.x + radius, SIZE / 2 + offset.y - radius, SIZE / 2 - offset.x - radius, SIZE / 2 + offset.y + radius);
		}
		else
		{
			surface()->DrawOutlinedCircle(SIZE / 2 - offset.x, SIZE / 2 + offset.y, radius, 16);
		}
	}
}

void CHudRadar::Init()
{

}

void CHudRadar::Reset()
{

}