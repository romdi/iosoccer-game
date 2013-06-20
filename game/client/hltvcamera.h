//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HLTVCAMERA_H
#define HLTVCAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "gameeventlistener.h"

class C_HLTVCamera : CGameEventListener
{
public:
	C_HLTVCamera();
	virtual ~C_HLTVCamera();

	void Init();
	void Reset();

	void FireGameEvent( IGameEvent *event );

	void SetMode(int iMode);
	void SpecNextPlayer( bool bInverse );
	void SpecNamedPlayer( const char *szPlayerName );
	
	int  GetMode();	// returns current camera mode
	C_BaseEntity *GetPrimaryTarget();  // return primary target
	void SetPrimaryTarget( int nEntity); // set the primary obs target
	void CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	void CreateMove(CUserCmd *cmd);

protected:
	void Accelerate( Vector& wishdir, float wishspeed, float accel );

	Vector		m_vCamOrigin;  //current camera origin
	QAngle		m_aCamAngle;   //current camera angle
	float		m_flFOV; // current FOV
	int			m_nCameraMode; // current camera mode
	int			m_iTraget1;	// first tracked target or 0
	int			m_iTraget2; // second tracked target or 0
	CUserCmd	m_LastCmd;
	Vector		m_vecVelocity;
};


extern C_HLTVCamera *HLTVCamera();	// get Singleton



#endif // HLTVCAMERA_H
