#include "cbase.h"
#include "ios_fieldbot.h"
#include "sdk_player.h"
#include "team.h"

LINK_ENTITY_TO_CLASS(ios_fieldbot, CFieldBot);

ConVar bot_shootatgoal("bot_shootatgoal", "1");

void CFieldBot::BotThink()
{
	if (m_vDirToBall.Length2D() > 50)
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
		
		if (isGoalShot)
		{
			if (ownDistToGoal > 1000)
				m_cmd.buttons |= IN_ALT1;
			else
			{
				m_cmd.buttons |= IN_ALT1;
			}
		}
		else
		{
			m_cmd.buttons |= IN_ALT1;
		}

		VectorAngles(shotDir, m_cmd.viewangles);
		m_cmd.powershot_strength = g_IOSRand.RandomInt(0, 100);
		m_cmd.viewangles[PITCH] = g_IOSRand.RandomFloat(-89, 89);
	}
	else
	{
		if (GetFlags() & FL_ATCONTROLS)
		{
			//float xDir = g_IOSRand.RandomFloat(0.1f, 1) * Sign((SDKGameRules()->m_vKickOff - GetLocalOrigin()).x);
			float xDir = g_IOSRand.RandomFloat(-1, 1);
			shotDir = Vector(xDir, GetTeam()->m_nForward, 0);
		}
		else
		{
			shotDir = Vector(g_IOSRand.RandomFloat(-1, 1), GetTeam()->m_nForward, 0);
		}

		VectorAngles(shotDir, m_cmd.viewangles);
		m_cmd.buttons |= IN_ALT1;
		m_cmd.powershot_strength = 100 * g_IOSRand.RandomFloat(0, 1);
		m_cmd.viewangles[PITCH] = -30 + 30 * (1 - m_cmd.powershot_strength / 100.0f);
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
		m_cmd.forwardmove = clamp(m_oldcmd.forwardmove + g_IOSRand.RandomFloat(-200, 200) * gpGlobals->frametime * 2, mp_runspeed.GetInt(), mp_sprintspeed.GetInt());
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
		m_cmd.forwardmove = clamp(m_oldcmd.forwardmove + g_IOSRand.RandomFloat(-200, 200) * gpGlobals->frametime * 2, -mp_walkspeed.GetInt() / 2, mp_walkspeed.GetInt());
		m_cmd.sidemove = clamp(m_oldcmd.sidemove + g_IOSRand.RandomFloat(-200, 200) * gpGlobals->frametime * 2, -mp_walkspeed.GetInt() / 2, mp_walkspeed.GetInt() / 2);
		//Vector forward = Vector(0, GetTeam()->m_nForward, 0);
		//VectorAngles(forward, m_cmd.viewangles);
		//
		//if (!m_bIsOffside && Sign(m_vDirToBall.y) == GetTeam()->m_nForward)
		//	m_cmd.forwardmove = mp_runspeed.GetInt();
		//else if (Sign(pos.y - GetTeam()->m_vPlayerSpawns[GetShirtNumber() - 1].y) == GetTeam()->m_nForward)
		//	m_cmd.forwardmove = -mp_runspeed.GetInt();
	}
}