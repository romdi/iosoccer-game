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
#include "sdk_team.h"

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
	Vector dirToBall = m_vBallPos - GetLocalOrigin();
	//if (dirToBall.Length2D() > 400)
	//{
	//	BotCenter();
	//}
	//else
	{
		BotAdjustPos();
	}
}

void CKeeperBot::BotCenter()
{
	Vector dir = GetTeam()->m_vPlayerSpawns[0] - GetLocalOrigin();
	m_cmd.sidemove = Sign(dir.x) * max(0, dir.Length2D() - 10);
	m_cmd.forwardmove = Sign(dir.y) * max(0, dir.Length2D() - 10);
}

void CKeeperBot::BotAdjustPos()
{
	float modifier;
	QAngle ang = m_oldcmd.viewangles;

	if (m_nBody == MODEL_KEEPER_AND_BALL)
	{
		if (!m_bShotButtonsDepressed)
		{
			m_bShotButtonsDepressed = true;
			m_flBotNextShot = gpGlobals->curtime + 1;
		}
		else if (gpGlobals->curtime >= m_flBotNextShot)
		{
			VectorAngles(Vector(0, GetTeam()->m_nForward, 0), ang);
			modifier = 0.9f;
			m_cmd.buttons |= IN_ATTACK2;
			m_cmd.powershot_strength = 50;
			VectorAngles(Vector(0, GetTeam()->m_nForward, 0), ang);
			ang[YAW] += g_IOSRand.RandomFloat(-45, 45);
			ang[PITCH] = g_IOSRand.RandomFloat(-40, 0);
		}
	}
	else
	{
		if (m_vDirToBall.Length2D() < 50 && m_vDirToBall.z < VEC_HULL_MAX.z + 50)
		{
			modifier = 0.9f;
			m_cmd.buttons |= IN_ATTACK2;
			m_cmd.powershot_strength = 50;
			VectorAngles(Vector(0, GetTeam()->m_nForward, 0), ang);
			ang[YAW] += g_IOSRand.RandomFloat(-45, 45);
			ang[PITCH] = g_IOSRand.RandomFloat(-40, 0);

			if (m_vDirToBall.z > VEC_HULL_MAX.z)
				m_cmd.buttons |= IN_JUMP;
		}
		else
		{
			float ballDistToGoal = (m_vBallPos - GetTeam()->m_vPlayerSpawns[0]).Length2D();
			CSDKPlayer *pClosest = FindClosestPlayerToBall();

			if (ballDistToGoal < 750)
			{
				if (m_vDirToBall.z < 80)
				{
					if (pClosest == this)
						modifier = 1.0f;
					else if (pClosest->GetTeam() != GetTeam())
						modifier = 0.75f;
					else
						modifier = 0.33f;
				}
				else
				{
					modifier = 0.15f;
				}
			}
			else if (ballDistToGoal < 1000 && m_vDirToBall.z < 80 && pClosest == this)
			{
				modifier = 1.0f;
			}
			else
			{
				modifier = 0.33f;
			}

			VectorAngles(m_vDirToBall, ang);
		}

		Vector targetPosDir = GetTeam()->m_vPlayerSpawns[0] + modifier * (m_vBallPos - GetTeam()->m_vPlayerSpawns[0]) - GetLocalOrigin();
		targetPosDir.z = 0;
		float dist = targetPosDir.Length2D();
		VectorNormalizeFast(targetPosDir);
		Vector localDir;
		VectorIRotate(targetPosDir, EntityToWorldTransform(), localDir);
		float speed;
		if (dist < 10)
			speed = 0;
		else if (dist < 100)
			speed = mp_runspeed.GetInt();
		else
			speed = mp_sprintspeed.GetInt();
		//float speed = clamp(dist - 10, 0, mp_runspeed.GetInt());
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