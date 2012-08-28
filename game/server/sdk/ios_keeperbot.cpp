/********************************************************************
   VSBOT_KEEPER


	Date:				8 July 2002
	Author:			mark gornall
	Description:	vs bot goal keeper

********************************************************************/

#include "cbase.h"
#include "player.h"
#include "sdk_player.h"
#include "in_buttons.h"
#include "movehelper_server.h"
#include "gameinterface.h"

#include "baseanimating.h"
#include "player_pickup.h"
#include "props_shared.h"
#include "props.h"
#include "sdk_player.h"
#include "in_buttons.h"

#include "ball.h"

#include "ios_keeperbot.h"
#include "game.h"
#include "team.h"

class CKeeperBot;

LINK_ENTITY_TO_CLASS(ios_keeperbot, CKeeperBot);

#define KEEPER_CLOSE_COEFF 0.999f
#define KEEPER_MID_COEFF 0.20f
#define KEEPER_FAR_COEFF 0.001f

///////////////////////////////////////////////////
// BotKeeperThink
//
// put movement values into m_cmd, then that gets run after
// and moves the bot.
//
void CKeeperBot::BotThink()
{
	// Prevent bot from running all the way back to his goal on penalties
	if (SDKGameRules()->State_Get() != MATCH_PENALTIES || (GetTeam()->m_vPlayerSpawns[0] - GetLocalOrigin()).Length2D() < 1000)
		BotAdjustPos();
}

void CKeeperBot::BotCenter()
{
	Vector dir = GetTeam()->m_vPlayerSpawns[0] - GetLocalOrigin();
	m_cmd.sidemove = Sign(dir.x) * max(0, dir.Length2D() - 10);
	m_cmd.forwardmove = Sign(dir.y) * max(0, dir.Length2D() - 10);
}

void CKeeperBot::BotAdjustPos()
{
	float modifier = KEEPER_MID_COEFF;
	QAngle ang = m_oldcmd.viewangles;
	Vector target = GetTeam()->m_vPlayerSpawns[0];

	if (m_vBallVel.Length2D() > 750 && m_flAngToBallVel < 60)
	{
		float yDist = GetTeam()->m_vPlayerSpawns[0].y - m_vBallPos.y;
		float vAng = acos(Sign(yDist) * m_vBallDir2D.y);
		float xDist = Sign(m_vBallDir2D.x) * abs(yDist) * tan(vAng);
		target.x = clamp(m_vBallPos.x + xDist, GetTeam()->m_vPlayerSpawns[0].x - 150, GetTeam()->m_vPlayerSpawns[0].x + 150);
	}

	if (m_pBall->State_Get() == BALL_KEEPERHANDS && m_pBall->GetCurrentPlayer() == this)
	{
		if (ShotButtonsReleased())
		{
			modifier = KEEPER_CLOSE_COEFF;
			//m_cmd.buttons |= (IN_ATTACK2 | IN_ATTACK);
			CSDKPlayer *pPl = FindClosestPlayerToSelf(true, true);
			if (!pPl && SDKGameRules()->IsIntermissionState())
				pPl = FindClosestPlayerToSelf(false, true);

			if (pPl)
			{
				m_cmd.powershot_strength = 66;
				VectorAngles(pPl->GetLocalOrigin() - GetLocalOrigin(), ang);
				ang[PITCH] = g_IOSRand.RandomFloat(-40, 0);
				//m_flBotNextShot = gpGlobals->curtime + 1;
			}
			else
			{
				m_cmd.powershot_strength = g_IOSRand.RandomFloat(50, 100);
				VectorAngles(Vector(0, GetTeam()->m_nForward, 0), ang);
				ang[YAW] += g_IOSRand.RandomFloat(-45, 45);
				ang[PITCH] = g_IOSRand.RandomFloat(-40, 0);
				//m_flBotNextShot = gpGlobals->curtime + 1;
			}
		}
	}
	else// if (gpGlobals->curtime >= m_flBotNextShot)
	{
		VectorAngles(m_vDirToBall, ang);
		float ballDistToGoal = (m_vBallPos - GetTeam()->m_vPlayerSpawns[0]).Length2D();
		CSDKPlayer *pClosest = FindClosestPlayerToBall();

		if (ballDistToGoal < 750 && m_vDirToBall.Length2D() < 200 && m_vDirToBall.z < 200)
		{
			modifier = KEEPER_CLOSE_COEFF;// max(0.15f, 1 - ballDistToGoal / 750);
			m_cmd.powershot_strength = 50;
			VectorAngles(Vector(0, GetTeam()->m_nForward, 0), ang);
			bool diving = false;

			if (m_flAngToBallVel < 60 && m_flAngToBallVel > 15)
			{
				if (abs(m_vDirToBall.x) > 50 && abs(m_vDirToBall.x) < 200 && m_vDirToBall.z < 150 && abs(m_vDirToBall.x) < 150 && m_vBallVel.Length() > 200)
				{
					m_cmd.buttons |= IN_JUMP;
					m_cmd.buttons |= Sign(m_vDirToBall.x) == GetTeam()->m_nRight ? IN_MOVERIGHT : IN_MOVELEFT;
					//m_cmd.buttons |= (IN_ATTACK2 | IN_ATTACK);
					diving = true;
				}
				else if (m_vDirToBall.z > 100 && m_vDirToBall.z < 150 && m_vDirToBall.Length2D() < 100)
				{
					m_cmd.buttons |= IN_JUMP;
					//m_cmd.buttons |= (IN_ATTACK2 | IN_ATTACK);
					diving = true;
				}
				else if (abs(m_vDirToBall.y) > 50 && abs(m_vDirToBall.y) < 200 && m_vDirToBall.z < 100 && abs(m_vDirToBall.x) < 50 && m_vBallVel.Length() < 200 && pClosest != this)
				{
					m_cmd.buttons |= IN_JUMP;
					m_cmd.buttons |= Sign(m_vLocalDirToBall.x) == GetTeam()->m_nForward ? IN_FORWARD : IN_BACK;
					//m_cmd.buttons |= (IN_ATTACK2 | IN_ATTACK);
					diving = true;
				}
			}

			if (!diving)
			{
				if (m_vDirToBall.Length2D() < 60)
				{
					modifier = KEEPER_CLOSE_COEFF;
					//m_cmd.buttons |= (IN_ATTACK2 | IN_ATTACK);
					CSDKPlayer *pPl = FindClosestPlayerToSelf(true, true);
					if (!pPl && SDKGameRules()->IsIntermissionState())
						pPl = FindClosestPlayerToSelf(false, true);

					if (pPl)
					{
						m_cmd.powershot_strength = 66;
						VectorAngles(pPl->GetLocalOrigin() - GetLocalOrigin(), ang);
						ang[PITCH] = g_IOSRand.RandomFloat(-40, 0);
						//m_flBotNextShot = gpGlobals->curtime + 1;
					}
					else
					{
						m_cmd.powershot_strength = g_IOSRand.RandomFloat(50, 100);
						VectorAngles(Vector(0, GetTeam()->m_nForward, 0), ang);
						ang[YAW] += g_IOSRand.RandomFloat(-45, 45);
						ang[PITCH] = g_IOSRand.RandomFloat(-40, 0);
						//m_flBotNextShot = gpGlobals->curtime + 1;
					}
				}
				else
					modifier = KEEPER_CLOSE_COEFF;
			}
		}
		else if (ballDistToGoal < 1250 && m_flAngToBallVel < 60 && m_vBallVel.Length2D() > 300 && (m_vBallVel.z > 100 || m_vDirToBall.z > 100))
		{
			modifier = KEEPER_FAR_COEFF;
		}
		else if (ballDistToGoal < 1000 && m_vDirToBall.z < 80 && m_vBallVel.Length2D() < 300 && m_vBallVel.z < 100)
		{
			if ((pClosest == this || ballDistToGoal < 750))
				modifier = KEEPER_CLOSE_COEFF;
			else
				modifier = KEEPER_MID_COEFF;
		}
		else
		{
			modifier = KEEPER_MID_COEFF;
		}

		m_cmd.viewangles = ang;
		m_LastAngles = m_cmd.viewangles;
		SetLocalAngles(m_cmd.viewangles);
		SnapEyeAngles(ang);

		Vector targetPosDir = target + modifier * (m_vBallPos - target) - GetLocalOrigin();
		targetPosDir.z = 0;
		float dist = targetPosDir.Length2D();
		VectorNormalizeFast(targetPosDir);
		Vector localDir;
		VectorIRotate(targetPosDir, EntityToWorldTransform(), localDir);
		//float speed;
		//if (dist < 10)
		//	speed = 0;
		//else if (dist < 100)
		//	speed = mp_runspeed.GetInt();
		//else
		//	speed = mp_sprintspeed.GetInt();
		//float speed = clamp(dist - 10, 0, mp_runspeed.GetInt());
		float speed = 0;

		if (dist > 20)
			speed = clamp(3 * dist, 0, mp_sprintspeed.GetInt());

		if (speed > mp_runspeed.GetInt())
			m_cmd.buttons |= IN_SPEED;

		m_cmd.forwardmove = localDir.x * speed;
		m_cmd.sidemove = -localDir.y * speed;

		if (m_cmd.forwardmove > 0)
			m_cmd.buttons |= IN_FORWARD;
		else if (m_cmd.forwardmove < 0)
			m_cmd.buttons |= IN_BACK;

		if (m_cmd.sidemove > 0)
			m_cmd.buttons |= IN_RIGHT;
		else if (m_cmd.sidemove < 0)
			m_cmd.buttons |= IN_MOVELEFT;
	}

	m_cmd.viewangles = ang;
	m_cmd.buttons |= IN_ALT1;
}

CSDKPlayer *CKeeperBot::FindClosestPlayerToBall()
{
	float shortestDist = FLT_MAX;
	CSDKPlayer *pClosest = NULL;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		float dist = (m_vBallPos - pPl->GetLocalOrigin()).Length2D();

		if (dist < shortestDist)
		{
			shortestDist = dist;
			pClosest = pPl;
		}
	}

	return pClosest;
}