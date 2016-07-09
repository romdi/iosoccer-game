#include "cbase.h"
#include "ios_fieldbot.h"
#include "sdk_player.h"
#include "team.h"
#include "in_buttons.h"

LINK_ENTITY_TO_CLASS(ios_fieldbot, CFieldBot);

ConVar bot_shootatgoal("bot_shootatgoal", "1");

void CFieldBot::BotThink()
{
	if (!ShotButtonsReleased())
		return;

	if (m_vDirToBall.Length2D() > 35)
		BotRunToBall();
	else
		BotShootBall();
}

void CFieldBot::BotShootBall()
{
	Vector shotDir;

	if (bot_shootatgoal.GetBool())
	{
		Vector target = GetOppTeam()->m_vGoalCenter;
		float ownDistToGoal = (GetOppTeam()->m_vGoalCenter - GetLocalOrigin()).Length2D();
		bool isGoalShot = true;

		for (int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CSDKPlayer *pPl = (CSDKPlayer *)UTIL_PlayerByIndex(i);

			if (!CSDKPlayer::IsOnField(pPl))
				continue;

			if (pPl->GetTeamNumber() != GetTeamNumber())
				continue;

			float distToGoal = (GetOppTeam()->m_vGoalCenter - pPl->GetLocalOrigin()).Length2D();
			if (distToGoal < ownDistToGoal)
			{
				target = pPl->GetLocalOrigin();
				isGoalShot = false;
				break;
			}
		}
		
		target.x += g_IOSRand.RandomFloat(-200, 200);
		shotDir = target - GetLocalOrigin();

		if (m_pHoldingBall)
		{
			if (!m_Shared.m_bIsShotCharging)
			{
				m_cmd.buttons |= IN_ATTACK2;
			}
		}
		else if (isGoalShot)
		{
			if (ownDistToGoal > 1000)
			{
				m_cmd.buttons |= IN_ATTACK;
			}
			else
			{
				m_cmd.buttons |= IN_ATTACK;
			}
		}
		else
		{
			m_cmd.buttons |= IN_ATTACK;
		}

		VectorAngles(shotDir, m_cmd.viewangles);
		m_cmd.viewangles[PITCH] = g_IOSRand.RandomFloat(-89, 89);
	}
	else
	{
		if (GetFlags() & FL_ATCONTROLS)
		{
			//float xDir = g_IOSRand.RandomFloat(0.1f, 1) * Sign((SDKGameRules()->m_vKickOff - GetLocalOrigin()).x);
			
		}
		else
		{
		}


		if (m_pHoldingBall)
		{
			if (!m_Shared.m_bIsShotCharging)
			{
				m_cmd.buttons |= IN_ATTACK2;
			}

			int kickOffDir = Sign(SDKGameRules()->m_vKickOff.GetX() - GetLocalOrigin().x);
			shotDir = Vector(g_IOSRand.RandomFloat(0.25f, 1.0f) * kickOffDir, g_IOSRand.RandomFloat(-1, 1), 0);
		}
		else
		{
			m_cmd.buttons |= IN_ATTACK;
			shotDir = Vector(g_IOSRand.RandomFloat(-1, 1), GetTeam()->m_nForward, 0);
		}

		VectorAngles(shotDir, m_cmd.viewangles);
		m_cmd.viewangles[PITCH] = 0;
	}

	if (m_vDirToBall.z > GetPlayerMaxs().z + 10)
		m_cmd.buttons |= IN_JUMP;
}

void CFieldBot::BotRunToBall()
{
	float closestDist = FLT_MAX;
	CSDKPlayer *pClosest = NULL;

	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSDKPlayer *pPl = (CSDKPlayer *)UTIL_PlayerByIndex(i);

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (pPl->GetTeamNumber() != GetTeamNumber())
			continue;

		float dist = (m_vBallPos - pPl->GetLocalOrigin()).Length2D();
		if (dist < closestDist)
		{
			closestDist = dist;
			pClosest = pPl;
		}
	}

	if (pClosest == this)
	{
		VectorAngles(m_vDirToBall, m_cmd.viewangles);
		m_cmd.forwardmove = clamp(m_oldcmd.forwardmove + g_IOSRand.RandomFloat(-mp_runspeed.GetInt(), mp_sprintspeed.GetInt()) * gpGlobals->frametime * 2, mp_runspeed.GetInt(), mp_sprintspeed.GetInt());
	}
	else
	{
		Vector pos = GetLocalOrigin();
		if (pos.x < SDKGameRules()->m_vFieldMin.GetX() - 100 || pos.x > SDKGameRules()->m_vFieldMax.GetX() + 100 || pos.y < SDKGameRules()->m_vFieldMin.GetY() - 100 || pos.y > SDKGameRules()->m_vFieldMax.GetY() + 100)
		{
			QAngle ang;
			VectorAngles(SDKGameRules()->m_vKickOff - pos, ang);
			m_cmd.viewangles[YAW] = ang[YAW];
		}
		else
			m_cmd.viewangles[YAW] = m_oldcmd.viewangles[YAW] + g_IOSRand.RandomFloat(-180, 180) * gpGlobals->frametime * 4;

		m_cmd.forwardmove = clamp(m_oldcmd.forwardmove + g_IOSRand.RandomFloat(-mp_sprintspeed.GetInt(), mp_sprintspeed.GetInt()) * gpGlobals->frametime * 2, -mp_sprintspeed.GetInt() / 2, mp_sprintspeed.GetInt());
		m_cmd.sidemove = clamp(m_oldcmd.sidemove + g_IOSRand.RandomFloat(-mp_sprintspeed.GetInt(), mp_sprintspeed.GetInt()) * gpGlobals->frametime * 2, -mp_sprintspeed.GetInt() / 2, mp_sprintspeed.GetInt() / 2);
	}

	if (m_cmd.forwardmove > mp_runspeed.GetInt() || m_cmd.sidemove > mp_runspeed.GetInt())
		m_cmd.buttons |= IN_SPEED;
}