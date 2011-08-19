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

#define WALKSPD   150
#define RUNSPD    280	//320 //220   //was220
#define SAVESPD   280	//320 //260   //320

LINK_ENTITY_TO_CLASS(ios_keeperbot, CKeeperBot);

///////////////////////////////////////////////////
// BotKeeperThink
//
// put movement values into m_cmd, then that gets run after
// and moves the bot.
//
void CKeeperBot::BotThink()
{
   float       dist_spawn_to_ball;
   int         o;             //orientation (of keeper)
   float		dist_to_ball = 10000.0f, shot_angle;
   Vector		v_ball, v_goalpos, v_goalball;
   Vector		v_spawntoball;
   Vector		ballPos, pos, ballVel, ballAvel;
   QAngle		angle, spawntoball_angle, ball_vel_angle, goalball_angle, goal_angle, ball_angle;
   int			ballStatus;
   CBall		*pBall = BotFindBall();

	if (!pBall)
		return;

	ballPos = pBall->GetAbsOrigin();
	pBall->VPhysicsGetObject()->GetVelocity( &ballVel, &ballAvel );
	pos = GetAbsOrigin();
	angle = GetLocalAngles();

	o = KeeperStartFrame ();     //setup a few things

	ballStatus = pBall->ballStatus;

	//float KeeperSkill = 0.0f;
	//float KeeperCatch = 0.0f;

	float fSkill = keeperskill.GetFloat();
	if (fSkill > 100) fSkill = 100;
	if (fSkill < 10) fSkill = 10;

	//find angle from facing straight out to the ball
	v_spawntoball = ballPos - m_SpawnPos;
	VectorAngles(v_spawntoball, spawntoball_angle);
	dist_spawn_to_ball = v_spawntoball.Length2D();


   //face the ball by default
   if (v_spawntoball.y * o < 0.0f)
   {
		v_ball = ballPos - pos;
		VectorAngles(v_ball, ball_angle);
		dist_to_ball  = v_ball.Length2D();
		angle.y = ball_angle.y;
		m_cmd.viewangles = angle;
   } 
   else 
   {
		m_cmd.viewangles.y = (-90.0f * o);
		KeeperCentre (o);    //ball is behind keeper so ignore it
   }

	//angle betweem the way the ball is moving and the angle to the keeper
	//so if the different is very small the ball is travelling towards the keeper
	VectorAngles(ballVel, ball_vel_angle);
	shot_angle = fabs(ball_angle.y - ball_vel_angle.y);

	//CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer*>(this);
	//if (!pPlayer)
	//	return;

	if (ballStatus>=BALL_FOUL_PENDING) 
	{
		KeeperCentre (o);
		m_KeeperMode = KEEPER_CLOSESAVE;          //do nothing
	}

	//keeper carrying ball (don't do normal ai)
	if (pBall->m_KeeperCarrying == this) 
	{
		m_cmd.viewangles.y = (-90.0f * o) + g_IOSRand.RandomFloat(-35.0f,35.0f);   //force angle of kick
		SetLocalAngles(m_cmd.viewangles);		//force angle
		float frametime = gpGlobals->frametime;
		RunPlayerMove( m_cmd, frametime );		//run move now to fix up Y rot!! oh dear

		if (pBall->m_KeeperCarryTime < gpGlobals->curtime + 7.0f )	//carry for 3 secs (since carrytime is time+10)
		{
			m_cmd.viewangles.x = -35.0f + g_IOSRand.RandomFloat(-15.0f,15.0f);
			m_cmd.buttons |= IN_ATTACK;
		}
		return;
	}


	//RandomSeed( gpGlobals->curtime );					//wtf do I have to do to get a random number!

	
	//close ball
	if (m_KeeperMode == KEEPER_DEFAULT) 
	{
		if (v_spawntoball.y * o < 0.0f && (dist_spawn_to_ball < 500 || (ballStatus==BALL_GOALKICK_PENDING && pBall->m_team==GetTeamNumber()))) 
		{
			//goal kick
			if (ballStatus==BALL_GOALKICK_PENDING && dist_to_ball < 90) 
			{
				pBall->m_KeeperCarrying = NULL;
				pBall->m_KeeperCarryTime = gpGlobals->curtime + 5.0f; //dont pick up when taking a goal kick

				//pEdict->v.v_angle.x = -23 + RANDOM_FLOAT (-5,5);
				//pEdict->v.v_angle.y = (-90 * o) + RANDOM_FLOAT (-35,35);   //force angle of kick
				m_cmd.viewangles.x = -35.0f + g_IOSRand.RandomFloat(-15.0f,15.0f);
				m_cmd.viewangles.y = (-90.0f * o) + g_IOSRand.RandomFloat(-35.0f,35.0f);   //force angle of kick
				SetLocalAngles(m_cmd.viewangles);		//force angle
				float frametime = gpGlobals->frametime;
				RunPlayerMove( m_cmd, frametime );		//run move now to fix up Y rot!! oh dear

				float fRand = g_IOSRand.RandomFloat(0.0f,100.0f);
				if (fRand<=25.0f) 
				{
					//pEdict->v.v_angle.x = -30;
					//m_cmd.viewangles.x = -30.0f;
					pBall->Use( this, this, USE_PASS, 1.0f);      //pass
				} 
				else 
				{
					pBall->Use( this, this, USE_POWERSHOT_KICK, 1.0f);      //force special goal kick
				}
				pBall->ballStatus=0;	//?
				pBall->m_BallShield=0;	//?

			} 
			//else if (thisEnemy->v.origin.z < pEdict->v.origin.z || thisEnemy->v.velocity.Length2D() < 400 && RANDOM_LONG (0,(int)KeeperSkill)==0) 
			else
			{
				//head towards the ball and kick it
				m_cmd.viewangles.y = ball_angle.y;
				m_cmd.forwardmove = SAVESPD;

				//dont kick if skill says not to
				float fRand = g_IOSRand.RandomFloat(0.0f, 100.0f);
				//Warning ("rand = %f\n", fRand);
				if (fSkill > fRand && m_fMissTime < gpGlobals->curtime)
				{
					m_cmd.buttons |= IN_ATTACK;
					m_fMissTime = gpGlobals->curtime + 0.1f;
				}

				//work out which way to dive (or if to dive at all)           
				if (ballPos.z > pos.z + 40 && dist_to_ball < 40) 
				{
					m_cmd.buttons |= IN_JUMP;
				}
				//dive if ball is low
				else if (shot_angle < 175 && shot_angle > 159 && m_fNextDive < gpGlobals->curtime) 
				{
					if (o==-1)
						m_cmd.buttons |= (IN_MOVELEFT | IN_JUMP);
					else
						m_cmd.buttons |= (IN_MOVERIGHT | IN_JUMP);
					m_fNextDive = gpGlobals->curtime + 2.5f;
				} 
				else if (shot_angle > 185 && shot_angle < 201 && m_fNextDive < gpGlobals->curtime) 
				{
					if (o==-1)
						m_cmd.buttons |= (IN_MOVERIGHT | IN_JUMP);
					else
						m_cmd.buttons |= (IN_MOVELEFT | IN_JUMP);
					m_fNextDive = gpGlobals->curtime + 2.5f;
				} 

				//normal clearance
				//m_cmd.viewangles.x = RandomFloat (-70.0f,-60.0f);
				m_cmd.viewangles.x = g_IOSRand.RandomFloat (-10.0f,-20.0f);
				if (g_IOSRand.RandomInt(0,1)) 
				{
					//m_cmd.viewangles.y += RandomFloat (-30.0f,-35.0f);       //randomness rotation in clearance
					m_cmd.viewangles.y += g_IOSRand.RandomFloat (0.0f,-5.0f);       //randomness rotation in clearance
				} 
				else 
				{
					//pEdict->v.ideal_yaw += RANDOM_LONG (30,35);
					m_cmd.viewangles.y += g_IOSRand.RandomFloat (0.0f,5.0f);
				}

			} 

			m_KeeperMode = KEEPER_CLOSESAVE;
		}
		else
		{
			KeeperCentre (o);
		}
	}
}

#define GOAL_WIDTH   70

#define OFFLINE_NEAR 20
#define OFFLINE_FAR 30

///////////////////////////////////////////////////
// Keeper Head to Centre of Goal
//
void CKeeperBot::KeeperCentre(int o)
{
	//edict_t *pEdict = pEdict;
	Vector pos = GetAbsOrigin();

	//head to centre of goal
	if (pos.x > m_SpawnPos.x + 5) 
	{
		m_cmd.sidemove = RUNSPD * o;
		m_cmd.viewangles.y = m_SpawnAngle.y;
	} 
	else if (pos.x < m_SpawnPos.x - 5) 
	{
		m_cmd.sidemove = -RUNSPD * o;
		m_cmd.viewangles.y = m_SpawnAngle.y;
	}

	if (o==-1) 
	{
		if (pos.y < m_SpawnPos.y - (OFFLINE_NEAR * o)) 
		{
			m_cmd.forwardmove = -RUNSPD * o;
			m_cmd.viewangles.y = m_SpawnAngle.y;
		} 
		else if (pos.y > m_SpawnPos.y - (OFFLINE_FAR * o)) 
		{
			m_cmd.forwardmove = RUNSPD * o;
			m_cmd.viewangles.y = m_SpawnAngle.y;
		}
	} 
	else 
	{
		if (pos.y < m_SpawnPos.y - (OFFLINE_FAR * o)) 
		{
			m_cmd.forwardmove = -RUNSPD * o;
			m_cmd.viewangles.y = m_SpawnAngle.y;
		} 
		else if (pos.y > m_SpawnPos.y - (OFFLINE_NEAR * o)) 
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
	int   o = 1;
	CBaseEntity *pSpot=NULL;
	//get keeper position info because it gets lost in teamplay spawning etc.
	if (GetTeamNumber() == TEAM_A) 
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_team1_player1" );
		o = 1;
	} 
	else if (GetTeamNumber() == TEAM_B) 
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_team2_player1" );
		o = -1;
	}
	if (pSpot) 
	{
		m_SpawnPos = pSpot->GetAbsOrigin();
		m_SpawnAngle = pSpot->GetAbsAngles();
	}
	m_KeeperMode = KEEPER_DEFAULT;
	return o;
}