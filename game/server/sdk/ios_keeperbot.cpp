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
}

#define GOAL_WIDTH   70

#define OFFLINE_NEAR 20
#define OFFLINE_FAR 30

void CKeeperBot::KeeperCentre(int o)
{
	//edict_t *pEdict = pEdict;
	Vector pos = GetAbsOrigin();
	Vector ballPos;
	BotFindBall()->VPhysicsGetObject()->GetPosition(&ballPos, NULL);
	Vector newPos = m_SpawnPos + (ballPos - m_SpawnPos) / 3;

	//head to centre of goal
	if (pos.x > newPos.x + 5) 
	{
		m_cmd.sidemove = RUNSPD * o;
		m_cmd.viewangles.y = m_SpawnAngle.y;
	} 
	else if (pos.x < newPos.x - 5) 
	{
		m_cmd.sidemove = -RUNSPD * o;
		m_cmd.viewangles.y = m_SpawnAngle.y;
	}

	if (o==-1) 
	{
		if (pos.y < newPos.y - (OFFLINE_NEAR * o)) 
		{
			m_cmd.forwardmove = -RUNSPD * o;
			m_cmd.viewangles.y = m_SpawnAngle.y;
		} 
		else if (pos.y > newPos.y - (OFFLINE_FAR * o)) 
		{
			m_cmd.forwardmove = RUNSPD * o;
			m_cmd.viewangles.y = m_SpawnAngle.y;
		}
	} 
	else 
	{
		if (pos.y < newPos.y - (OFFLINE_FAR * o)) 
		{
			m_cmd.forwardmove = -RUNSPD * o;
			m_cmd.viewangles.y = m_SpawnAngle.y;
		} 
		else if (pos.y > newPos.y - (OFFLINE_NEAR * o)) 
		{
			m_cmd.forwardmove = RUNSPD * o;
			m_cmd.viewangles.y = m_SpawnAngle.y;
		}
	}
}

///////////////////////////////////////////////////
// Keeper Start Frame
// Set up a few things first
int CKeeperBot::KeeperStartFrame()
{
	return 0;
}