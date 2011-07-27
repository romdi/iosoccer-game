/********************************************************************
   KEEPER
 
	Date:			07 July 2002 (HL1). Jan 2008 (HL2)
	Author:			Mark Gornall
	Description:	bot goal keeper

********************************************************************/
 
#ifndef KEEPER_H
#define KEEPER_H


// This is our bot class.
class CSDKBot : public CSDKPlayer
{
public:
	bool			m_bBackwards;

	float			m_flNextTurnTime;
	bool			m_bLastTurnToRight;

	float			m_flNextStrafeTime;
	float			m_flSideMove;

	QAngle			m_ForwardAngle;
	QAngle			m_LastAngles;

	//ios
	Vector			m_SpawnPos;
	QAngle			m_SpawnAngle;
	int				m_KeeperMode;
	float			m_fMissTime;
	float			m_fNextDive;

	CUserCmd		m_cmd;
};


//edict_t *BotFindBall(bot_t *bot);
void BotKeeperThink(CSDKBot *bot);
//void BotPlayerThink(CSDKBot *bot, float moved_distance);

extern void RunPlayerMove( CSDKPlayer *fakeclient, CUserCmd &cmd, float frametime );

#endif