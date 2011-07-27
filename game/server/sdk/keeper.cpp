/********************************************************************
   VSBOT_KEEPER


	Date:				8 July 2002
	Author:			mark gornall
	Description:	vs bot goal keeper

********************************************************************/
/*
#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "client.h"
#include "player.h"
#include "items.h"
#include "weapons.h"
#include "soundent.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "pm_defs.h"
#include "monsters.h"

#include "vsbot.h"

#include <sys/types.h>
#include <sys/stat.h>
*/

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

#include "keeper.h"

#include "game.h"

class CSDKBot;

#define KEEPER_DEFAULT     0
#define KEEPER_MOVEFORSAVE 1
#define KEEPER_CLOSESAVE   2
#define KEEPER_CLOSEBALL   3


static void KeeperCentre (CSDKBot *pBot, int o);
static int  KeeperIsInGoalMouth (CSDKBot *pBot);
static int  KeeperStartFrame (CSDKBot *pBot);

static void PlayerCentre (CSDKBot *pBot, int o);
static int  PlayerIsInGoalMouth (CSDKBot *pBot);
static int  PlayerStartFrame (CSDKBot *pBot);

#define FRONTPOST_ANGLE   75
#define FRONTPOST_DIST    90 

#define WALKSPD   150
#define RUNSPD    280	//320 //220   //was220
#define SAVESPD   280	//320 //260   //320


///////////////////////////////////////////////////
// BotFindBall
//
CBall *BotFindBall (CSDKBot *pBot)
{
	CBaseEntity *pEnt = gEntList.FindEntityByClassnameNearest( "football", pBot->GetAbsOrigin(), 10000.0f);
	CBall *pBall = dynamic_cast<CBall*>( pEnt );
	return pBall;
}



///////////////////////////////////////////////////
// BotKeeperThink
//
// put movement values into m_cmd, then that gets run after
// and moves the bot.
//
void BotKeeperThink (CSDKBot *pBot)
{
   float       dist_spawn_to_ball;
   int         o;             //orientation (of keeper)
   float		dist_to_ball = 10000.0f, shot_angle;
   Vector		v_ball, v_goalpos, v_goalball;
   Vector		v_spawntoball;
   Vector		ballPos, pos, ballVel, ballAvel;
   QAngle		angle, spawntoball_angle, ball_vel_angle, goalball_angle, goal_angle, ball_angle;
   int			ballStatus;
   CBall		*pBall = BotFindBall(pBot);

	if (!pBall)
		return;

	ballPos = pBall->GetAbsOrigin();
	pBall->VPhysicsGetObject()->GetVelocity( &ballVel, &ballAvel );
	pos = pBot->GetAbsOrigin();
	angle = pBot->GetLocalAngles();

	o = KeeperStartFrame (pBot);     //setup a few things

	ballStatus = pBall->ballStatus;

	//float KeeperSkill = 0.0f;
	//float KeeperCatch = 0.0f;

	float fSkill = keeperskill.GetFloat();
	if (fSkill > 100) fSkill = 100;
	if (fSkill < 10) fSkill = 10;

	//find angle from facing straight out to the ball
	v_spawntoball = ballPos - pBot->m_SpawnPos;
	VectorAngles(v_spawntoball, spawntoball_angle);
	dist_spawn_to_ball = v_spawntoball.Length2D();


   //face the ball by default
   if (v_spawntoball.y * o < 0.0f)
   {
		v_ball = ballPos - pos;
		VectorAngles(v_ball, ball_angle);
		dist_to_ball  = v_ball.Length2D();
		angle.y = ball_angle.y;
		pBot->m_cmd.viewangles = angle;
   } 
   else 
   {
		pBot->m_cmd.viewangles.y = (-90.0f * o);
		KeeperCentre (pBot, o);    //ball is behind keeper so ignore it
   }

	//angle betweem the way the ball is moving and the angle to the keeper
	//so if the different is very small the ball is travelling towards the keeper
	VectorAngles(ballVel, ball_vel_angle);
	shot_angle = fabs(ball_angle.y - ball_vel_angle.y);

	//CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer*>(pBot);
	//if (!pPlayer)
	//	return;

	if (ballStatus>=BALL_FOUL_PENDING) 
	{
		KeeperCentre (pBot, o);
		pBot->m_KeeperMode = KEEPER_CLOSESAVE;          //do nothing
	}

	//keeper carrying ball (don't do normal ai)
	if (pBall->m_KeeperCarrying == pBot) 
	{
		pBot->m_cmd.viewangles.y = (-90.0f * o) + g_IOSRand.RandomFloat(-35.0f,35.0f);   //force angle of kick
		pBot->SetLocalAngles(pBot->m_cmd.viewangles);		//force angle
		float frametime = gpGlobals->frametime;
		RunPlayerMove( pBot, pBot->m_cmd, frametime );		//run move now to fix up Y rot!! oh dear

		if (pBall->m_KeeperCarryTime < gpGlobals->curtime + 7.0f )	//carry for 3 secs (since carrytime is time+10)
		{
			pBot->m_cmd.viewangles.x = -35.0f + g_IOSRand.RandomFloat(-15.0f,15.0f);
			pBot->m_cmd.buttons |= IN_ATTACK;
		}
		return;
	}


	//RandomSeed( gpGlobals->curtime );					//wtf do I have to do to get a random number!

	
	//close ball
	if (pBot->m_KeeperMode == KEEPER_DEFAULT) 
	{
		if (v_spawntoball.y * o < 0.0f && (dist_spawn_to_ball < 500 || (ballStatus==BALL_GOALKICK_PENDING && pBall->m_team==pBot->GetTeamNumber()))) 
		{
			//goal kick
			if (ballStatus==BALL_GOALKICK_PENDING && dist_to_ball < 90) 
			{
				pBall->m_KeeperCarrying = NULL;
				pBall->m_KeeperCarryTime = gpGlobals->curtime + 5.0f; //dont pick up when taking a goal kick

				//pEdict->v.v_angle.x = -23 + RANDOM_FLOAT (-5,5);
				//pEdict->v.v_angle.y = (-90 * o) + RANDOM_FLOAT (-35,35);   //force angle of kick
				pBot->m_cmd.viewangles.x = -35.0f + g_IOSRand.RandomFloat(-15.0f,15.0f);
				pBot->m_cmd.viewangles.y = (-90.0f * o) + g_IOSRand.RandomFloat(-35.0f,35.0f);   //force angle of kick
				pBot->SetLocalAngles(pBot->m_cmd.viewangles);		//force angle
				float frametime = gpGlobals->frametime;
				RunPlayerMove( pBot, pBot->m_cmd, frametime );		//run move now to fix up Y rot!! oh dear

				float fRand = g_IOSRand.RandomFloat(0.0f,100.0f);
				if (fRand<=25.0f) 
				{
					//pEdict->v.v_angle.x = -30;
					//pBot->m_cmd.viewangles.x = -30.0f;
					pBall->Use( pBot, pBot, USE_PASS, 1.0f);      //pass
				} 
				else 
				{
					pBall->Use( pBot, pBot, USE_POWERSHOT_KICK, 1.0f);      //force special goal kick
				}
				pBall->ballStatus=0;	//?
				pBall->m_BallShield=0;	//?

			} 
			//else if (pBot->pBotEnemy->v.origin.z < pEdict->v.origin.z || pBot->pBotEnemy->v.velocity.Length2D() < 400 && RANDOM_LONG (0,(int)KeeperSkill)==0) 
			else
			{
				//head towards the ball and kick it
				pBot->m_cmd.viewangles.y = ball_angle.y;
				pBot->m_cmd.forwardmove = SAVESPD;

				//dont kick if skill says not to
				float fRand = g_IOSRand.RandomFloat(0.0f, 100.0f);
				//Warning ("rand = %f\n", fRand);
				if (fSkill > fRand && pBot->m_fMissTime < gpGlobals->curtime)
				{
					pBot->m_cmd.buttons |= IN_ATTACK;
					pBot->m_fMissTime = gpGlobals->curtime + 0.1f;
				}

				//work out which way to dive (or if to dive at all)           
				if (ballPos.z > pos.z + 40 && dist_to_ball < 40) 
				{
					pBot->m_cmd.buttons |= IN_JUMP;
				}
				//dive if ball is low
				else if (shot_angle < 175 && shot_angle > 159 && pBot->m_fNextDive < gpGlobals->curtime) 
				{
					if (o==-1)
						pBot->m_cmd.buttons |= (IN_MOVELEFT | IN_JUMP);
					else
						pBot->m_cmd.buttons |= (IN_MOVERIGHT | IN_JUMP);
					pBot->m_fNextDive = gpGlobals->curtime + 2.5f;
				} 
				else if (shot_angle > 185 && shot_angle < 201 && pBot->m_fNextDive < gpGlobals->curtime) 
				{
					if (o==-1)
						pBot->m_cmd.buttons |= (IN_MOVERIGHT | IN_JUMP);
					else
						pBot->m_cmd.buttons |= (IN_MOVELEFT | IN_JUMP);
					pBot->m_fNextDive = gpGlobals->curtime + 2.5f;
				} 

				//normal clearance
				//pBot->m_cmd.viewangles.x = RandomFloat (-70.0f,-60.0f);
				pBot->m_cmd.viewangles.x = g_IOSRand.RandomFloat (-10.0f,-20.0f);
				if (g_IOSRand.RandomInt(0,1)) 
				{
					//pBot->m_cmd.viewangles.y += RandomFloat (-30.0f,-35.0f);       //randomness rotation in clearance
					pBot->m_cmd.viewangles.y += g_IOSRand.RandomFloat (0.0f,-5.0f);       //randomness rotation in clearance
				} 
				else 
				{
					//pEdict->v.ideal_yaw += RANDOM_LONG (30,35);
					pBot->m_cmd.viewangles.y += g_IOSRand.RandomFloat (0.0f,5.0f);
				}

			} 

			pBot->m_KeeperMode = KEEPER_CLOSESAVE;
		}
		else
		{
			KeeperCentre (pBot, o);
		}
	}

  /*
	//ball close but ball away from spawn (but dont go miles from spawn point)
	if (pBot->m_KeeperMode == KEEPER_DEFAULT) 
	{
		if (v_spawntoball.y * o < 0.0f && dist_to_ball < 400 && dist_spawn_to_ball < 500) 
		{
			//head towards the ball and kick it
   			pEdict->v.ideal_yaw = ball_angle.y;
			pBot->f_move_speed = SAVESPD;     

			if (RANDOM_LONG (0,(int)KeeperSkill)==0) 
			{
				pEdict->v.button |= IN_ATTACK;                     //boot
			}

			//check for keeper catch
			if (dist_to_ball < 35 && (pBot->pBotEnemy->v.origin.z < pEdict->v.origin.z + 30) 
						&& ballStatus==BALL_NORMAL
						&& (RANDOM_LONG (0,(int)KeeperCatch*0.25f) == 0)
						&& pBall->keeperCarryTime < gpGlobals->time - 1.0f) 
			{
				pBall->m_KeeperCarrying = pPlayer;
				pPlayer->SetAnimation (PLAYER_WALK);
				BotFixIdealYaw(pEdict);
				return;
			}

			//work out which way to dive (or if to dive at all)
			if (pBot->pBotEnemy->v.origin.z < pEdict->v.origin.z && ballStatus==BALL_NORMAL) 
			{ 
				//dive if ball is low
				if (shot_angle < 180 && shot_angle > 159 ) 
				{
					pPlayer->SetAnimation (PLAYER_DIVERIGHT);
				} else if (shot_angle > 181 && shot_angle < 201) 
				{
					pPlayer->SetAnimation (PLAYER_DIVELEFT);
				} else if (pBot->pBotEnemy->v.origin.z > pEdict->v.origin.z + 40 && RANDOM_LONG (0,(int)KeeperSkill)==0) 
				{
					pEdict->v.button |= IN_JUMP;
					pEdict->v.button |= IN_ATTACK;
				}
			}
			pBot->m_KeeperMode = KEEPER_CLOSEBALL;
		}
	}


   //keeper angle method   
   if (pBot->m_KeeperMode == KEEPER_DEFAULT) {
                                                                                 
      if (v_spawntoball.y * o < 0.0f && pBot->pBotEnemy->v.velocity.Length2D() > 450 && dist_to_ball < 1000) {

         if (shot_angle >= 179 && shot_angle <= 181) {
            //stay still ball heading this way
            if (dist_to_ball < 100 && pBot->pBotEnemy->v.origin.z - 20 > pEdict->v.origin.z)
               pEdict->v.button |= IN_JUMP;     //if ball is high then jump and close

         } else if (shot_angle < 179 && shot_angle > 149 ) {
            pBot->f_strafe_speed = RUNSPD * o;                //move
            pEdict->v.ideal_yaw = pBot->m_SpawnAngle.y;          //face out when moving
            if (!KeeperIsInGoalMouth(pBot))  //new v2.0
               KeeperCentre (pBot, o);

         } else if (shot_angle > 181 && shot_angle < 211) {
            pBot->f_strafe_speed = -RUNSPD * o;
            pEdict->v.ideal_yaw = pBot->m_SpawnAngle.y;
            if (!KeeperIsInGoalMouth(pBot))  //new v2.0
               KeeperCentre (pBot, o);

         } else if (!KeeperIsInGoalMouth(pBot)) {
           KeeperCentre (pBot, o);
         }

         pBot->m_KeeperMode = KEEPER_MOVEFORSAVE;
      }
   }


   //process the default behavior
   if (pBot->m_KeeperMode == KEEPER_DEFAULT) {
      //dont move if ball is behind the keeper
      if (v_spawntoball.y * o < 0.0f) { 
   
         //is the ball to the left or right?
         if (spawntoball_angle.y > pBot->m_SpawnAngle.y && spawntoball_angle.y > pBot->m_SpawnAngle.y+FRONTPOST_ANGLE ) {

            //has the keeper moved far enough already? (check both bounds for both keeper orientations)
            if (pEdict->v.origin.x < (pBot->m_SpawnPos.x + FRONTPOST_DIST) && pEdict->v.origin.x > (pBot->m_SpawnPos.x - FRONTPOST_DIST)) {
               pBot->f_strafe_speed = -RUNSPD;
               pBot->f_move_speed = 0;     
               pEdict->v.ideal_yaw = pBot->m_SpawnAngle.y;
            }
         } else if (spawntoball_angle.y < pBot->m_SpawnAngle.y && spawntoball_angle.y < pBot->m_SpawnAngle.y-FRONTPOST_ANGLE ){
            if (pEdict->v.origin.x < (pBot->m_SpawnPos.x + FRONTPOST_DIST) && pEdict->v.origin.x > (pBot->m_SpawnPos.x - FRONTPOST_DIST)) {
               pBot->f_strafe_speed = RUNSPD;
               pBot->f_move_speed = 0;     
               pEdict->v.ideal_yaw = pBot->m_SpawnAngle.y;
            }
         } else {
            KeeperCentre (pBot, o);
         }
      }
   }

	BotFixIdealYaw(pEdict);

	*/
}

#define GOAL_WIDTH   70

/*
///////////////////////////////////////////////////
// KeeperIsInGoalMouth
//
static int KeeperIsInGoalMouth (CSDKBot *pBot)
{
   if (pBot->pEdict->v.origin.x < (pBot->m_SpawnPos.x + GOAL_WIDTH) 
      && pBot->pEdict->v.origin.x > (pBot->m_SpawnPos.x - GOAL_WIDTH))
      return true;
   else
      return false;
}

*/
#define OFFLINE_NEAR 20
#define OFFLINE_FAR 30

///////////////////////////////////////////////////
// Keeper Head to Centre of Goal
//
static void KeeperCentre (CSDKBot *pBot, int o)
{
	//edict_t *pEdict = pBot->pEdict;
	Vector pos = pBot->GetAbsOrigin();

	//head to centre of goal
	if (pos.x > pBot->m_SpawnPos.x + 5) 
	{
		pBot->m_cmd.sidemove = RUNSPD * o;
		pBot->m_cmd.viewangles.y = pBot->m_SpawnAngle.y;
	} 
	else if (pos.x < pBot->m_SpawnPos.x - 5) 
	{
		pBot->m_cmd.sidemove = -RUNSPD * o;
		pBot->m_cmd.viewangles.y = pBot->m_SpawnAngle.y;
	}

	if (o==-1) 
	{
		if (pos.y < pBot->m_SpawnPos.y - (OFFLINE_NEAR * o)) 
		{
			pBot->m_cmd.forwardmove = -RUNSPD * o;
			pBot->m_cmd.viewangles.y = pBot->m_SpawnAngle.y;
		} 
		else if (pos.y > pBot->m_SpawnPos.y - (OFFLINE_FAR * o)) 
		{
			pBot->m_cmd.forwardmove = RUNSPD * o;
			pBot->m_cmd.viewangles.y = pBot->m_SpawnAngle.y;
		}
	} 
	else 
	{
		if (pos.y < pBot->m_SpawnPos.y - (OFFLINE_FAR * o)) 
		{
			pBot->m_cmd.forwardmove = -RUNSPD * o;
			pBot->m_cmd.viewangles.y = pBot->m_SpawnAngle.y;
		} 
		else if (pos.y > pBot->m_SpawnPos.y - (OFFLINE_NEAR * o)) 
		{
			pBot->m_cmd.forwardmove = RUNSPD * o;
			pBot->m_cmd.viewangles.y = pBot->m_SpawnAngle.y;
		}
	}
}


///////////////////////////////////////////////////
// Keeper Start Frame
// Set up a few things first
static int KeeperStartFrame (CSDKBot *pBot)
{
	int   o = 1;
	CBaseEntity *pSpot=NULL;
	//get keeper position info because it gets lost in teamplay spawning etc.
	if (pBot->GetTeamNumber() == TEAM_A) 
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_team1_player1" );
		o = 1;
	} 
	else if (pBot->GetTeamNumber() == TEAM_B) 
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_team2_player1" );
		o = -1;
	}
	if (pSpot) 
	{
		pBot->m_SpawnPos = pSpot->GetAbsOrigin();
		pBot->m_SpawnAngle = pSpot->GetAbsAngles();
	}
	pBot->m_KeeperMode = KEEPER_DEFAULT;
	//pBot->f_move_speed = 0;     
	//pBot->f_strafe_speed = 0;
	//pBot->pEdict->v.v_angle.x = pBot->m_SpawnAngle.x;
	return o;
}

























//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////



/*

#define PLAYER_INSIDE 1

float gClosestPlayerDistToBall = 9999.0f;
CSDKBot *gClosestPlayerToBall = NULL;
int   gStatusWait = 0;

void BotPlayerThink (CSDKBot *pBot, float moved_dist)
{
//   float dist_from_goal;
//   int  angle_to_entity;
   float       dist_spawn_to_ball, dist_ball_to_targetgoal;
   int         o;             //orientation (of keeper)
	edict_t     *pEdict = pBot->pEdict;
   float       dist_to_ball = 10000.0f, dist_player_from_spawn, dist_player_from_spawn_x, dist_player_from_spawn_y;
   Vector      ball_angle, v_ball, v_goalpos, goal_angle, v_goalball, goalball_angle;
   Vector      v_spawntoball, spawntoball_angle, ball_vel_angle, v_spawntoplayer;
   int         ballStatus;
   
   o = PlayerStartFrame (pBot);     //setup a few things

   ballStatus = pBall->ballStatus;

   
   //find angle from facing straight out to the ball
   v_spawntoball        = pBot->pBotEnemy->v.origin - pBot->m_SpawnPos;
   spawntoball_angle    = UTIL_VecToAngles( v_spawntoball );
   dist_spawn_to_ball   = v_spawntoball.Length2D();

   v_spawntoplayer      = pEdict->v.origin - pBot->m_SpawnPos;
   dist_player_from_spawn = v_spawntoplayer.Length2D();
   dist_player_from_spawn_x = fabs(pEdict->v.origin.x - pBot->m_SpawnPos.x);
   dist_player_from_spawn_y = fabs(pEdict->v.origin.y - pBot->m_SpawnPos.y);

   v_ball               = pBot->pBotEnemy->v.origin - pEdict->v.origin;
   ball_angle           = UTIL_VecToAngles( v_ball );
   dist_to_ball         = v_ball.Length2D();
   pEdict->v.ideal_yaw  = ball_angle.y;

   CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance(pEdict);
   pPlayer->SetAnimation (PLAYER_WALK);

   if ( pBall->m_KeeperCarrying || ballStatus==BALL_GOALKICK_PENDING) {
      PlayerCentre (pBot, o);
      pBot->m_KeeperMode = KEEPER_CLOSESAVE;          //do nothing
   }

   //precalc ball distance from target goalmouth
   CBaseEntity *pSpot=NULL;
   if (pBot->CSDKBoteam == 0) {
      pSpot = UTIL_FindEntityByClassname( pSpot, "info_team1_player1" );
   } else {
      pSpot = UTIL_FindEntityByClassname( pSpot, "info_team2_player1" );
   }
   if (pSpot)
      dist_ball_to_targetgoal = (pBot->pBotEnemy->v.origin - pSpot->edict()->v.origin).Length2D();

   //close ball
   if (pBot->m_KeeperMode == KEEPER_DEFAULT) {

      //shoot / pass / tackle - when (very near ball) OR (been dribbling) OR (in opposions box AND quite near ball)
      if (dist_to_ball < 50 || pBot->m_DribbleCount > RANDOM_LONG(10,20) || (dist_ball_to_targetgoal < 512  && dist_to_ball < 200))
      {
         pBot->f_move_speed = SAVESPD;     
         //try to face the attack dir
         CBaseEntity *pSpot=NULL;
         if (pBot->CSDKBoteam == 0) {    
            pSpot = UTIL_FindEntityByClassname( pSpot, "info_team1_player1" );
         } else {
            pSpot = UTIL_FindEntityByClassname( pSpot, "info_team2_player1" );
         }
         if (pSpot) {
            Vector shotVec = pSpot->edict()->v.origin - pEdict->v.origin;
            Vector shot_angle = UTIL_VecToAngles( shotVec );
            pEdict->v.ideal_yaw = shot_angle.y;
            if (dist_ball_to_targetgoal < 512) //close in shot - aim for corner?
               pEdict->v.ideal_yaw += RANDOM_LONG (-15,15);
            pEdict->v.v_angle.x = RANDOM_LONG (-3,-8);
            pEdict->v.fixangle = TRUE; 
            if (ballStatus != 0)
            {
               //wait before taking status actions
               if (gStatusWait > 100)
               {
                  pEdict->v.v_angle.x = RANDOM_LONG (-30,-60);
                  pEdict->v.fixangle = TRUE; 
                  pEdict->v.button |= IN_ATTACK;     //shoot from throw/fk/corner
                  gStatusWait = 0;
               }
               else
               {
                  gStatusWait++;
                  pBot->f_move_speed = SAVESPD;     
                  pEdict->v.fixangle = TRUE;
	               BotFixIdealYaw(pEdict);
                  pBot->m_DribbleCount = 0;
                  return;
               }
            }
            else if (pBot->bot_class > 8)
            {
               //if a forward then SHOOT or PASS
               if (RANDOM_LONG(0,5) > 0)
               {
                  pEdict->v.button |= IN_ATTACK;
                  pEdict->v.button |= IN_ATTACK2;
               }
               else
               {
                  pEdict->v.button |= IN_ATTACK2;
               }
            }
            else if (pBot->bot_class > 5)
            {
               //if a midfielder then PASS
               pEdict->v.button |= IN_ATTACK2;
            } 
            else
            {
               if (RANDOM_LONG(0,1) > 0)
               {
                  //if defender kick and slide
                  pEdict->v.v_angle.x = RANDOM_LONG (-30,-60);
                  pEdict->v.button |= IN_ATTACK;
               }
               else
               {
                  pEdict->v.button |= IN_ATTACK2;
               }

            }
         }
         pBot->f_move_speed = SAVESPD;     
         pEdict->v.fixangle = TRUE;
	      BotFixIdealYaw(pEdict);
         pBot->m_DribbleCount = 0;
         gStatusWait = 0;
         return;
      }         

      //dribble / turn
      if (dist_to_ball < 80)
      {
         pBot->f_move_speed = SAVESPD;     
         //try to face the attack dir
         CBaseEntity *pSpot=NULL;
         if (pBot->CSDKBoteam == 0) {
            pSpot = UTIL_FindEntityByClassname( pSpot, "info_team1_player1" );
         } else {
            pSpot = UTIL_FindEntityByClassname( pSpot, "info_team2_player1" );
         }
         if (pSpot) {

            //TURN
            Vector shotVec = pSpot->edict()->v.origin - pEdict->v.origin;
            Vector shot_angle = UTIL_VecToAngles( shotVec );
            pEdict->v.ideal_yaw = shot_angle.y;
            pEdict->v.v_angle.x = 40;
            pEdict->v.fixangle = TRUE; 
            //always dribble when turning
            pEdict->v.button |= IN_ATTACK;

            //if taking corner etc then aim high instead
            if (ballStatus != 0)
            {
                  pEdict->v.v_angle.x = RANDOM_LONG (-30,-60);
                  pEdict->v.fixangle = TRUE; 
            }
         }
         pBot->f_move_speed = SAVESPD;     
         pEdict->v.fixangle = TRUE;
	      BotFixIdealYaw(pEdict);
         pBot->m_DribbleCount++;

         //if there's a player dribbling then he is the chaser
         gClosestPlayerDistToBall = dist_to_ball;
         gClosestPlayerToBall = pBot;

         return;
      }


      //head towards the ball until we reach the limit of our playing position
      int wide = 200;
      //if (pBot->bot_class == 9 || pBot->bot_class == 10)
      //   wide = 550;

      if ((dist_player_from_spawn_x < wide && dist_player_from_spawn_y < 920))
      {
         if (pBot->m_ChangeMode != PLAYER_INSIDE)
            pBot->m_ChangeModeTime = gpGlobals->time + 0.25f;

         //change mode
         pBot->m_ChangeMode = PLAYER_INSIDE;

         if (pBot->m_ChangeModeTime < gpGlobals->time) {
            //yes target ball
   	      pEdict->v.ideal_yaw = ball_angle.y;
            pBot->f_move_speed = 370;
            pEdict->v.maxspeed = 370;
         } else {
            //no keep heading to centre          
            PlayerCentre(pBot, o);
         }

      }
      else if (CVAR_GET_FLOAT("mp_chaserbot") && (dist_to_ball < gClosestPlayerDistToBall || gClosestPlayerToBall == pBot))
      {
         //if a real player on the same team as the bot touched the ball then don't chase
         if (GetTheBall() && ballStatus == 0 && pBall->LastPlayer && pBall->LastPlayer->vampire != pBot->CSDKBoteam)
         {
            gClosestPlayerDistToBall = 9999999.9f;
            gClosestPlayerToBall = NULL;
         }
         else
         {
            //chaserbot
  	         pEdict->v.ideal_yaw = ball_angle.y;
            pBot->f_move_speed = 370;
            pEdict->v.maxspeed = 370;
            gClosestPlayerDistToBall = dist_to_ball;
            gClosestPlayerToBall = pBot;
         }


      }
      else //we're already outside area, time to head back?
      {
         if (pBot->m_ChangeMode == PLAYER_INSIDE)
            pBot->m_ChangeModeTime = gpGlobals->time + 0.25f;

         //change mode
         pBot->m_ChangeMode = !PLAYER_INSIDE;

         if (pBot->m_ChangeModeTime < gpGlobals->time) {
            PlayerCentre (pBot, o);
         } else {
   	      pEdict->v.ideal_yaw = ball_angle.y;
            pBot->f_move_speed = SAVESPD;     
         }
      }
   }

   BotFixIdealYaw(pEdict);

}

#define GOAL_WIDTH   70

///////////////////////////////////////////////////
// KeeperIsInGoalMouth
//
static int PlayerIsInGoalMouth (CSDKBot *pBot)
{
   if (pBot->pEdict->v.origin.x < (pBot->m_SpawnPos.x + 1000) 
      && pBot->pEdict->v.origin.x > (pBot->m_SpawnPos.x - 1000))
      return true;
   else
      return false;
}


#define OFFLINE_NEAR 20
#define OFFLINE_FAR 30

///////////////////////////////////////////////////
// Player Head to pos
//
static void PlayerCentre (CSDKBot *pBot, int o)
{

	edict_t *pEdict = pBot->pEdict;

   //head to centre of goal
   if (pEdict->v.origin.x > pBot->m_SpawnPos.x + 5) {
      pBot->f_strafe_speed = RUNSPD * o;
      pEdict->v.ideal_yaw = pBot->m_SpawnAngle.y;
   } else if (pEdict->v.origin.x < pBot->m_SpawnPos.x - 5) {
      pBot->f_strafe_speed = -RUNSPD * o;
      pEdict->v.ideal_yaw = pBot->m_SpawnAngle.y;
   }

   if (o==-1) {
      if (pEdict->v.origin.y < pBot->m_SpawnPos.y - (OFFLINE_NEAR * o)) {
         pBot->f_move_speed = -RUNSPD * o;     
         pEdict->v.ideal_yaw = pBot->m_SpawnAngle.y;
      } else if (pEdict->v.origin.y > pBot->m_SpawnPos.y - (OFFLINE_FAR * o)) {
         pBot->f_move_speed = RUNSPD * o;     
         pEdict->v.ideal_yaw = pBot->m_SpawnAngle.y;
      }
   } else {
      if (pEdict->v.origin.y < pBot->m_SpawnPos.y - (OFFLINE_FAR * o)) {
         pBot->f_move_speed = -RUNSPD * o;     
         pEdict->v.ideal_yaw = pBot->m_SpawnAngle.y;
      } else if (pEdict->v.origin.y > pBot->m_SpawnPos.y - (OFFLINE_NEAR * o)) {
         pBot->f_move_speed = RUNSPD * o;     
         pEdict->v.ideal_yaw = pBot->m_SpawnAngle.y;
      }
   }

}


///////////////////////////////////////////////////
// Keeper Start Frame
// Set up a few things first
static int PlayerStartFrame (CSDKBot *pBot)
{
   int   o, target = -1, targetTeam = 0;
   char  sp[256];

   //convert the bots spawn position into a target position they should head after kickoff
   //eg the forwards run towards the opposition defenders area.
   switch (pBot->bot_class)
   {
   case 11:
      target = 6;          //pos on field
      targetTeam = 0;      //flag it means oppositions "3" not it's own
      break;
   case 10:
      target = 8;
      targetTeam = 1;
      break;
   case 9:
      target = 7;
      targetTeam = 1;
      break;
   case 8:
      target = 10;
      targetTeam = 0;
      break;
   case 7:
      target = 9;
      targetTeam = 0;
      break;
   case 6:
      target = 11;
      targetTeam = 0;
      break;
   case 5:
      target = 4;
      targetTeam = 0;
      break;
   case 4:
      target = 3;
      targetTeam = 0;
      break;
   case 3:
      target = 2;
      targetTeam = 0;
      break;
   case 2:
   default:
      target = 5;
      targetTeam = 0;
      break;
   }

   if (target<0)
      return 1;

   CBaseEntity *pSpot=NULL;
   //get keeper position info because it gets lost in teamplay spawning etc.
   if (pBot->CSDKBoteam == 0) {
      
      if (targetTeam==1) {
         sprintf (sp, "info_team1_player%d", target);
         o = 1;
      } else {
         sprintf (sp, "info_team2_player%d", target);
         o = -1;
      }
      pSpot = UTIL_FindEntityByClassname( pSpot, sp );
   } else {
      if (targetTeam==1) {
         sprintf (sp, "info_team2_player%d", target);
         o = -1;
      } else {
         sprintf (sp, "info_team1_player%d", target);
         o = 1;
      }
      pSpot = UTIL_FindEntityByClassname( pSpot, sp );
   }
   if (pSpot) {
      pBot->m_SpawnPos = pSpot->edict()->v.origin;
      pBot->m_SpawnAngle = pSpot->edict()->v.angles;
   }
   pBot->m_KeeperMode = KEEPER_DEFAULT;
   pBot->f_move_speed = 0;     
   pBot->f_strafe_speed = 0;
   pBot->pEdict->v.v_angle.x = pBot->m_SpawnAngle.x;
   return o;
}
*/

