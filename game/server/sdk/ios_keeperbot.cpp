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

#include "player_ball.h"
#include "match_ball.h"

#include "ios_keeperbot.h"
#include "game.h"
#include "team.h"

class CKeeperBot;

LINK_ENTITY_TO_CLASS(ios_keeperbot, CKeeperBot);

#define KEEPER_CLOSE_COEFF 0.99f
#define KEEPER_MID_COEFF 0.15f
#define KEEPER_FAR_COEFF 0.01f

extern ConVar
	sv_ball_bodypos_keeperhands,
	sv_ball_chargedshot_minstrength,
	sv_ball_chargedshot_maxstrength,
	sv_ball_keepershot_minangle,
	sv_ball_maxplayerfinddist;

ConVar mp_botkeeperspeed( "mp_botkeeperspeed", "100", FCVAR_NOTIFY, "Bot keeper speed <0-100>" );
ConVar mp_botkeeperdelay( "mp_botkeeperdelay", "30", FCVAR_NOTIFY, "Reaction time in ms" );


///////////////////////////////////////////////////
// BotKeeperThink
//
// put movement values into m_cmd, then that gets run after
// and moves the bot.
//
void CKeeperBot::BotThink()
{
	// Prevent bot from running all the way back to his goal on penalties
	if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES && (GetTeam()->m_vGoalCenter - GetLocalOrigin()).Length2D() > 1000)
		return;

	CUserCmd *pCmd = new CUserCmd();

	BotCalcCommand(*pCmd);

	m_Commands.AddToTail(new BotCommand(pCmd, gpGlobals->curtime));
	
	while (m_Commands.Count() > max(1, TIME_TO_TICKS(mp_botkeeperdelay.GetInt() / 1000.0f)))
	{
		delete m_Commands[0];
		m_Commands.Remove(0);
	}

	m_cmd = *m_Commands[0]->pCmd;
}

void CKeeperBot::BotCalcCommand(CUserCmd &cmd)
{
	float modifier = KEEPER_MID_COEFF;
	QAngle ang = m_oldcmd.viewangles;
	QAngle camAng = m_oldcmd.camviewangles;
	Vector ballDirToGoal = GetTeam()->m_vGoalCenter - m_vBallPos;
	float ballDistToGoal = ballDirToGoal.Length2D();
	Vector goalCenterWithOffset = GetTeam()->m_vGoalCenter + GetTeam()->m_nForward * Vector(0, 100, 0);
	Vector goalBehindPos = GetTeam()->m_vGoalCenter - GetTeam()->m_nForward * Vector(0, 200, 0);
	Vector dir = m_vBallPos - GetTeam()->m_vGoalCenter;
	dir.z = 0;
	dir.NormalizeInPlace();
	Vector target = GetTeam()->m_vGoalCenter + dir * 150;
	//ballDirToGoal.NormalizeInPlace();
	//float ballGoalDot = DotProduct2D(m_vBallVel.AsVector2D(), ballDirToGoal.AsVector2D());

	if (m_flAngToBallMoveDir < 60 && m_flAngToBallMoveDir > 15 && m_vBallVel.Length2D() > 200)
	{
		float yDist = GetTeam()->m_vGoalCenter.GetY() - m_vBallPos.y;
		float vAng = acos(Sign(yDist) * m_vBallDir2D.y);
		float xDist = Sign(m_vBallDir2D.x) * abs(yDist) * tan(vAng);
		target = GetTeam()->m_vGoalCenter;
		target.x = clamp(m_vBallPos.x + xDist, GetTeam()->m_vGoalCenter.GetX() - 150, GetTeam()->m_vGoalCenter.GetX() + 150);
	}

	if (m_pHoldingBall)
	{
		if (ShotButtonsReleased())
		{
			float chargeTime;
			Vector passTarget;

			CSDKPlayer *pPlayerTarget = FindClosestPlayerToSelf(true, true);

			if (!pPlayerTarget && SDKGameRules()->IsIntermissionState())
				pPlayerTarget = FindClosestPlayerToSelf(false, true);

			if (pPlayerTarget)
				passTarget = pPlayerTarget->GetLocalOrigin();
			else
				passTarget = GetOppTeam()->m_vGoalCenter;

			Vector dir = passTarget - GetLocalOrigin();
			float dist = dir.NormalizeInPlace();
			float distCoeff = pow(clamp(dist / ((SDKGameRules()->m_vFieldMax.GetY() - SDKGameRules()->m_vFieldMin.GetY()) / 1.85f), 0.0f, 1.0f), 1.5f);
			VectorAngles(dir, ang);
			ang[PITCH] = -5 + distCoeff * -30;
			VectorAngles(dir, camAng);
			chargeTime = distCoeff * mp_chargedshot_increaseduration.GetFloat();

			if (gpGlobals->curtime > m_pHoldingBall->GetStateEnterTime() + 1.5f)
			{
				if (m_Shared.m_bDoChargedShot)
				{
				}
				else if (m_Shared.m_bIsShotCharging)
				{
					if (gpGlobals->curtime >= m_Shared.m_flShotChargingStart + chargeTime)
					{
					}
					else if (gpGlobals->curtime >= m_Shared.m_flShotChargingStart + chargeTime / 2)
					{
						cmd.buttons |= IN_ATTACK2;
						target.y = GetTeam()->m_nForward == 1 ? GetTeam()->m_vPenBoxMax.GetY() - 300 : GetTeam()->m_vPenBoxMin.GetY() + 300;
					}
					else
					{
						cmd.buttons |= IN_ATTACK2;
						target.y = GetTeam()->m_nForward == 1 ? GetTeam()->m_vPenBoxMax.GetY() - 100 : GetTeam()->m_vPenBoxMin.GetY() + 100;
					}
				}
				else
				{
					cmd.buttons |= IN_ATTACK2;
				}
			}
		}
	}
	else
	{
		cmd.buttons |= IN_ATTACK;

		CSDKPlayer *pClosestPl = FindClosestPlayerToBall(false);

		//VectorRotate(m_vDirToBall, -ang, m_vLocalDirToBall);

		Vector dirToShotPos = m_pBall->GetLastShotPos() - GetLocalOrigin();

		//VectorAngles(dirToShotPos, ang);
		VectorAngles(Vector(0, GetTeam()->m_nForward, 0), ang);
		VectorAngles(m_vDirToBall, camAng);

		if (m_pBall->State_Get() == BALL_STATE_FREEKICK || m_pBall->State_Get() == BALL_STATE_CORNER)
		{
			modifier = KEEPER_FAR_COEFF;
		}
		else if (ballDistToGoal < 1250)
		{
			// High ball
			if (m_vDirToBall.z > 50 && m_vBallVel.z > 100 || m_vDirToBall.z > 100)
			{
				modifier = KEEPER_FAR_COEFF;
			}
			// Slow ball
			else if (m_vBallVel.Length() < 500 && pClosestPl == this)
			{
				modifier = KEEPER_CLOSE_COEFF;
			}
			// Near ball
			else if (ballDistToGoal < 750 && m_vDirToBall.Length2D() < 300)
			{
				modifier = KEEPER_CLOSE_COEFF;
			}
			// Normal ball
			else
			{
				modifier = max(KEEPER_FAR_COEFF, 1 - pow(min(1, ballDistToGoal / 750.0f), 2));
			}
		}
		else
		{
			modifier = KEEPER_MID_COEFF;
		}

		if (abs(m_vLocalDirToBall.x) > 50 && abs(m_vLocalDirToBall.x) < 225 && m_vDirToBall.z < 50 && abs(m_vLocalDirToBall.y) < 40 && m_vBallVel.Length() < 750 && pClosestPl != this)
		{
			cmd.buttons |= IN_DUCK;
			cmd.buttons |= Sign(m_vLocalDirToBall.x) == 1 ? IN_FORWARD : IN_BACK;
		}
		else if (m_vDirToBall.z <= 80 && m_vDirToBall.Length2D() < 50)
		{
		}
		else if (m_vLocalDirToBall.z > 80 && m_vDirToBall.z < 150 && m_vDirToBall.Length2D() < 50)
		{
			cmd.buttons |= IN_JUMP;
		}
		else if (m_flAngToBallMoveDir < 60 && m_flAngToBallMoveDir > 15
			&& abs(m_vLocalDirToBall.y) > 40 && abs(m_vLocalDirToBall.y) < 350 && m_vDirToBall.z < 100 && abs(m_vLocalDirToBall.x) < 150 && m_vBallVel.Length() > 200)
		{
			cmd.buttons |= IN_JUMP;
			cmd.buttons |= Sign(m_vLocalDirToBall.y) == 1 ? IN_MOVELEFT : IN_MOVERIGHT;
		}
	}

	cmd.viewangles = ang;
	cmd.camviewangles = camAng;
	//SetLocalAngles(cmd.viewangles);
	//SnapEyeAngles(ang);
	Vector targetPosDir;

	if (m_pHoldingBall)
		targetPosDir = target - GetLocalOrigin();
	else if (m_vBallPos.y < SDKGameRules()->m_vFieldMin.GetY() || m_vBallPos.y > SDKGameRules()->m_vFieldMax.GetY())
		targetPosDir = goalCenterWithOffset - GetLocalOrigin();
	else
		targetPosDir = target + modifier * (m_vBallPos - target) - GetLocalOrigin();

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

	if (dist > 30)
		speed = clamp(5 * dist, 0, mp_sprintspeed.GetInt() * (mp_botkeeperspeed.GetInt() / 100.0f));

	if (speed > mp_runspeed.GetInt())
		cmd.buttons |= IN_SPEED;

	cmd.forwardmove = localDir.x * speed;
	cmd.sidemove = -localDir.y * speed;

	if (cmd.forwardmove > 0)
		cmd.buttons |= IN_FORWARD;
	else if (cmd.forwardmove < 0)
		cmd.buttons |= IN_BACK;

	if (!m_pHoldingBall || (cmd.buttons & IN_ATTACK2) || !m_Shared.m_bIsShotCharging)
	{
		if (cmd.sidemove > 0)
			cmd.buttons |= IN_RIGHT;
		else if (cmd.sidemove < 0)
			cmd.buttons |= IN_MOVELEFT;
	}
}

CSDKPlayer *CKeeperBot::FindClosestPlayerToBall(bool ignoreSelf)
{
	float shortestDist = FLT_MAX;
	CSDKPlayer *pClosest = NULL;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (pPl == this && ignoreSelf)
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