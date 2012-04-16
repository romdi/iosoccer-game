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
		Vector target = GetOppTeam()->m_vPlayerSpawns[0];
		float ownDistToGoal = (GetOppTeam()->m_vPlayerSpawns[0] - GetLocalOrigin()).Length2D();
		bool isGoalShot = true;

		for (int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CSDKPlayer *pPl = (CSDKPlayer *)UTIL_PlayerByIndex(i);

			if (!CSDKPlayer::IsOnField(pPl))
				continue;

			if (pPl->GetTeamNumber() != GetTeamNumber())
				continue;

			float distToGoal = (GetOppTeam()->m_vPlayerSpawns[0] - pPl->GetLocalOrigin()).Length2D();
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
				m_cmd.buttons |= IN_ATTACK;
			else
			{
				m_cmd.buttons |= IN_ATTACK2;
			}
		}
		else
		{
			m_cmd.buttons |= IN_ATTACK2;
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
		m_cmd.buttons |= IN_ATTACK2;
		m_cmd.powershot_strength = 100 * g_IOSRand.RandomFloat(0, 1);
		m_cmd.viewangles[PITCH] = -30 + 30 * (1 - m_cmd.powershot_strength / 100.0f);
	}

	if (m_vDirToBall.z > VEC_HULL_MAX.z + 10)
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
		m_cmd.forwardmove = clamp(m_oldcmd.forwardmove + g_IOSRand.RandomFloat(-200, 200) * gpGlobals->frametime * 2, m_Shared.m_flRunSpeed, m_Shared.m_flSprintSpeed);
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
		//else if (Sign(pos.y - GetTeam()->m_vPlayerSpawns[GetTeamPosition() - 1].y) == GetTeam()->m_nForward)
		//	m_cmd.forwardmove = -mp_runspeed.GetInt();
	}
}

void CFieldBot::BotFetchAndPass()
{
	QAngle angle;

	angle = GetLocalAngles();

	Vector plballdir;
	Vector pldir;
	float closestDist = FLT_MAX;
	CSDKPlayer *pClosest = NULL;
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSDKPlayer *pPlayer = ( CSDKPlayer *) UTIL_PlayerByIndex( i );
		if (!(
			pPlayer &&
			pPlayer->GetTeamNumber() > LAST_SHARED_TEAM &&
			pPlayer->IsAlive() &&
			!(pPlayer->GetFlags() & FL_FAKECLIENT)
			))
			continue;

		float dist = GetLocalOrigin().DistTo(pPlayer->GetLocalOrigin());
		if (dist >= closestDist)
			continue;

		closestDist = dist;
		pClosest = pPlayer;
		plballdir = m_vBallPos - pPlayer->GetLocalOrigin();
		pldir = pPlayer->GetLocalOrigin() - GetLocalOrigin();
		break;
	}

	if (!pClosest)
		return;

	Vector dir = m_vBallPos - GetLocalOrigin();
	m_cmd.buttons &= ~IN_ATTACK;
	m_cmd.buttons &= ~IN_ATTACK2;
	m_cmd.forwardmove = 0;
	float pitch = 0;

	if (plballdir.Length2D() > 150)
	{
		//m_cmd.forwardmove = clamp(dir.Length2D() / 2, mp_walkspeed.GetInt(), mp_sprintspeed.GetInt());
		pitch = clamp(plballdir.Length2D() / -50 + 0, -40, 10); //-45;

		if (dir.Length2D() > 50)
		{
			Vector target;
			target = m_vBallPos + (plballdir / plballdir.Length()) * 50;
			if (dir.Dot(plballdir) > 0) // < 90°
			{
				VectorAngles(Vector(dir.x, dir.y, 0), angle);
				Vector forward, right, up;
				AngleVectors(angle, &forward, &right, &up);
				target += right * 50;
			}
			dir = target - GetLocalOrigin();

			m_cmd.forwardmove = mp_sprintspeed.GetInt() / 3.5f;
			//m_cmd.buttons &= IN_SPEED;
		}
		else
		{
			dir = plballdir * -1;

			/*if (plballdir.Length2D() < 500)
			{
				m_cmd.buttons |= IN_ATTACK;
			}
			else
			{*/
				m_cmd.buttons |= IN_ATTACK2;
				m_cmd.powershot_strength = 100 * min(1, dir.Length2D() / 1000);
			//}

			m_cmd.forwardmove = m_Shared.m_flRunSpeed / 3.5f;
			//m_cmd.buttons &= ~IN_SPEED;
		}
	}

	VectorAngles(Vector(dir.x, dir.y, 0), angle);
	angle[PITCH] = pitch;
	m_LastAngles = angle;
	SetLocalAngles( angle );
	m_cmd.viewangles = angle;
}