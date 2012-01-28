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

	Vector			m_vBallPos;
	Vector			m_vBallVel;
	AngularImpulse	m_vBallAngImp;
	QAngle			m_aBallAng;

	void BotFrame();
	virtual void BotThink();
	void RunPlayerMove( CUserCmd &cmd, float frametime );
	bool RunMimicCommand(CUserCmd &cmd);
	CBall* BotFindBall();
};

// If iTeam or iClass is -1, then a team or class is randomly chosen.
CBasePlayer *BotPutInServer( bool bFrozen, int keeper );

void Bot_RunAll();

//extern void RunPlayerMove( CSDKPlayer *fakeclient, CUserCmd &cmd, float frametime );

#endif // SDK_BOT_TEMP_H
