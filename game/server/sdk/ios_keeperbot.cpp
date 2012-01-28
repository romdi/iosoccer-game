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
	Vector dir = GetOwnTeamSpots(this)->m_vPlayers[0] - GetLocalOrigin();
	m_cmd.sidemove = Sign(dir.x) * max(0, dir.Length2D() - 10);
	m_cmd.forwardmove = Sign(dir.y) * max(0, dir.Length2D() - 10);
}

void CKeeperBot::BotAdjustPos()
{
	Vector ballDir = m_vBallPos - GetLocalOrigin();
	float modifier;

	if (ballDir.Length2D() < 100)
	{
		modifier = 0.9f;
		m_cmd.buttons = IN_ATTACK;
	}
	else if (m_vBallPos.WithinAABox(GetOwnTeamSpots(this)->m_vPenaltyMin, GetOwnTeamSpots(this)->m_vPenaltyMax))
	{
		modifier = 0.75f;
	}
	else
	{
		modifier = 0.33f;
	}

	Vector targetPosDir = GetOwnTeamSpots(this)->m_vPlayers[0] + modifier * (m_vBallPos - GetOwnTeamSpots(this)->m_vPlayers[0]) - GetLocalOrigin();
	QAngle ang;
	VectorAngles(ballDir, ang);
	m_cmd.viewangles = ang;
	if (m_cmd.viewangles[YAW] > 180.0f )
		m_cmd.viewangles[YAW] -= 360.0f;
	else if ( m_cmd.viewangles[YAW] < -180.0f )
		m_cmd.viewangles[YAW] += 360.0f;

	if (m_cmd.viewangles[PITCH] > 180.0f )
		m_cmd.viewangles[PITCH] -= 360.0f;
	else if ( m_cmd.viewangles[PITCH] < -180.0f )
		m_cmd.viewangles[PITCH] += 360.0f;

	SnapEyeAngles(m_cmd.viewangles);
	m_LastAngles = m_cmd.viewangles;
	SetLocalAngles(m_cmd.viewangles);

	Vector targetPos = targetPosDir + GetLocalOrigin();
	Vector dir = Vector(targetPosDir.x, targetPosDir.y, 0);
	float length = targetPosDir.Length2D();
	VectorNormalizeFast(dir);
	Vector forward, right;
	AngleVectors(ang, &forward, &right, NULL);
	//m_cmd.sidemove = GetOwnTeamSpots(this)->m_nLeft * Sign(localPos.y) * max(0, abs(localPos.y) - 10);
	//m_cmd.forwardmove = GetOwnTeamSpots(this)->m_nBack * Sign(localPos.x) * max(0, abs(localPos.x) - 10);
	//m_cmd.sidemove = sin(acos(DotProduct(forward, dir))) * max(0, length - 10);
	//m_cmd.forwardmove = cos(acos(DotProduct(forward, dir))) * max(0, length - 10);
	//m_cmd.forwardmove = cos(acos(DotProduct(dir, forward))) * max(0, length - 10);
	//m_cmd.sidemove = sin(acos(DotProduct(dir, forward))) * max(0, length - 10);
	//targetPos = forward * f + right * r;
	//forward = Vector(forward.x, forward.y, 0);
	//VectorNormalizeFast(forward);
	//float angle = acos(DotProduct2D(forward.AsVector2D(), dir.AsVector2D()));
	//Vector newDir = cos(angle) * forward + sin(angle) * right;
	//VectorNormalizeFast(ballDir);
	//m_cmd.forwardmove = cos(angle) * max(0, length / 2 - 10);
	//m_cmd.sidemove = sin(angle) * max(0, length / 2 - 10);
}