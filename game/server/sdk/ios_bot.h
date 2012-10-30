//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IOS_BOT_H
#define IOS_BOT_H
#ifdef _WIN32
#pragma once
#endif

#include "sdk_player.h"
#include "ios_mapentities.h"
#include "ball.h"

struct BotHistory
{
	CUserCmd m_cmd;
	CUserCmd m_oldcmd;
};

// This is our bot class.
class CBot : public CSDKPlayer
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

	CUserCmd		m_cmd;
	CUserCmd		m_oldcmd;

	CBall			*m_pBall;
	Vector			m_vBallPos;
	Vector			m_vBallVel;
	Vector			m_vBallDir2D;

	Vector			m_vDirToBall;
	Vector			m_vLocalDirToBall;
	float			m_flAngToBallVel;
	int				m_nPlayerType;

	void BotFrame();
	virtual void BotThink();
	void RunPlayerMove( CUserCmd &cmd, float frametime );
	bool RunMimicCommand(CUserCmd &cmd);
	void FieldBotJoinTeam();

	virtual void PhysicsSimulate();

	virtual bool		ShotButtonsReleased();
	virtual void		SetShotButtonsReleased(bool released);

	CUtlVector<BotHistory> m_BotHistory;

protected:
	float m_flBotNextShot;
};

// If iTeam or iClass is -1, then a team or class is randomly chosen.
CBasePlayer *BotPutInServer( bool bFrozen, int keeper );

void Bot_RunAll();

//extern void RunPlayerMove( CSDKPlayer *fakeclient, CUserCmd &cmd, float frametime );

#endif // SDK_BOT_TEMP_H
