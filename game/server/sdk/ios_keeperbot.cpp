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

#define KEEPER_DEFAULT     0
#define KEEPER_MOVEFORSAVE 1
#define KEEPER_CLOSESAVE   2
#define KEEPER_CLOSEBALL   3

#define FRONTPOST_ANGLE   75
#define FRONTPOST_DIST    90 

#define WALKSPD   100
#define RUNSPD    100//280	//320 //220   //was220
#define SAVESPD   100//280	//320 //260   //320

LINK_ENTITY_TO_CLASS(ios_keeperbot, CKeeperBot);

///////////////////////////////////////////////////
// BotKeeperThink
//
// put movement values into m_cmd, then that gets run after
// and moves the bot.
//
void CKeeperBot::BotThink()
{
	// Prevent bot from running all the way back to his goal on penalties
	if (SDKGameRules()->State_Get() != MATCH_PENALTIES || (GetFlags() & FL_NO_Y_MOVEMENT))
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
	float modifier = 0.25f;
	QAngle ang = m_oldcmd.viewangles;
	Vector target = GetTeam()->m_vPlayerSpawns[0];

	if (m_vBallVel.Length2D() > 750 && m_flAngToBallVel < 60)
	{
		float yDist = GetTeam()->m_vPlayerSpawns[0].y - m_vBallPos.y;
		float vAng = acos(Sign(yDist) * m_vBallDir2D.y);
		float xDist = Sign(m_vBallDir2D.x) * abs(yDist) * tan(vAng);
		target.x = clamp(m_vBallPos.x + xDist, GetTeam()->m_vPlayerSpawns[0].x - 150, GetTeam()->m_vPlayerSpawns[0].x + 150);
	}

	if (m_pBall->State_Get() == BALL_KEEPERHANDS && GetBall()->GetCurrentPlayer() == this)
	{
		if (!m_bShotButtonsReleased)
		{
			m_bShotButtonsReleased = true;
			m_flBotNextShot = gpGlobals->curtime + 1;
		}
		else if (gpGlobals->curtime >= m_flBotNextShot)
		{
			modifier = 0.99f;
			m_cmd.buttons |= IN_ATTACK2;
			m_cmd.powershot_strength = 50;
			VectorAngles(Vector(0, GetTeam()->m_nForward, 0), ang);
			ang[YAW] += g_IOSRand.RandomFloat(-45, 45);
			ang[PITCH] = g_IOSRand.RandomFloat(-40, 0);
			//m_flBotNextShot = gpGlobals->curtime + 1;
		}
	}
	else// if (gpGlobals->curtime >= m_flBotNextShot)
	{
		VectorAngles(m_vDirToBall, ang);
		float ballDistToGoal = (m_vBallPos - GetTeam()->m_vPlayerSpawns[0]).Length2D();
		CSDKPlayer *pClosest = FindClosestPlayerToBall();

		if (ballDistToGoal < 750 && m_vDirToBall.Length2D() < 200 && m_vDirToBall.z < 200)
		{
			modifier = 0.99f;// max(0.15f, 1 - ballDistToGoal / 750);
			m_cmd.powershot_strength = 50;
			VectorAngles(Vector(0, GetTeam()->m_nForward, 0), ang);
			bool diving = false;

			if (m_flAngToBallVel < 60 && m_flAngToBallVel > 15)
			{
				if (abs(m_vDirToBall.x) > 50 && abs(m_vDirToBall.x) < 200 && m_vDirToBall.z < 150 && abs(m_vDirToBall.x) < 150 && m_vBallVel.Length() > 200)
				{
					m_cmd.buttons |= IN_JUMP;
					m_cmd.buttons |= Sign(m_vDirToBall.x) == GetTeam()->m_nRight ? IN_MOVERIGHT : IN_MOVELEFT;
					m_cmd.buttons |= IN_ATTACK2;
					diving = true;
				}
				else if (m_vDirToBall.z > 100 && m_vDirToBall.z < 150 && m_vDirToBall.Length2D() < 100)
				{
					m_cmd.buttons |= IN_JUMP;
					m_cmd.buttons |= IN_ATTACK2;
					diving = true;
				}
				else if (abs(m_vDirToBall.y) > 50 && abs(m_vDirToBall.y) < 200 && m_vDirToBall.z < 100 && abs(m_vDirToBall.x) < 50 && m_vBallVel.Length() < 200 && pClosest != this)
				{
					m_cmd.buttons |= IN_JUMP;
					m_cmd.buttons |= Sign(m_vLocalDirToBall.x) == GetTeam()->m_nForward ? IN_FORWARD : IN_BACK;
					m_cmd.buttons |= IN_ATTACK2;
					diving = true;
				}
			}

			if (!diving)
			{
				if (m_vDirToBall.Length2D() < 50)
				{
					ang[YAW] += g_IOSRand.RandomFloat(-45, 45);
					ang[PITCH] = g_IOSRand.RandomFloat(-40, 0);
					m_cmd.buttons |= IN_ATTACK2;
				}
				else
					modifier = 0.99f;
			}
		}
		else if (ballDistToGoal < 1250 && m_flAngToBallVel < 60 && m_vBallVel.Length2D() > 300 && (m_vBallVel.z > 100 || m_vDirToBall.z > 100))
		{
			modifier = 0.01f;
		}
		else if (ballDistToGoal < 1000 && m_vDirToBall.z < BODY_HEAD_END && m_vBallVel.Length2D() < 300 && m_vBallVel.z < 100)
		{
			if ((pClosest == this || ballDistToGoal < 750))
				modifier = 0.99f;
			else
				modifier = 0.25f;
		}
		else
		{
			modifier = 0.25f;
		}

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
		m_cmd.forwardmove = localDir.x * speed;
		m_cmd.sidemove = -localDir.y * speed;
	}

	m_cmd.viewangles = ang;
}

CSDKPlayer *CKeeperBot::FindClosestPlayerToBall()
{
	float closestDist = FLT_MAX;
	CSDKPlayer *pClosest = NULL;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		float dist = (m_vBallPos - pPl->GetLocalOrigin()).Length2D();

		if (dist < closestDist)
		{
			closestDist = dist;
			pClosest = pPl;
		}
	}

	return pClosest;
}