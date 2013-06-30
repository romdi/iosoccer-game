//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CAMERA_H
#define CAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "gameeventlistener.h"

enum TVCam_Types_t
{ 
	TVCAM_SIDELINE,
	TVCAM_FIXED_SIDELINE,
	TVCAM_BEHIND_GOAL,
	TVCAM_TOPDOWN,
	TVCAM_GOAL_CORNER,
	TVCAM_FLY_FOLLOW,
	TVCAM_GOAL_LINE
};

class C_Camera : CGameEventListener
{
public:
	C_Camera();
	virtual ~C_Camera();

	void Init();
	void Reset();

	void FireGameEvent( IGameEvent *event );

	int  GetCamMode();	// returns current camera mode
	void SetCamMode(int mode);
	void SpecNextPlayer( bool bInverse );
	void SpecNamedPlayer( const char *szPlayerName );
	
	C_BaseEntity *GetTarget();  // return primary target
	void SetTarget( int nEntity); // set the primary obs target
	void CreateMove(CUserCmd *cmd);
	virtual void		CalcView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	void				ThirdpersonThink(Vector &camOffset, QAngle &camViewAngles);

protected:

	virtual void		CalcChaseCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual void		CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	virtual void		CalcTVCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual void		CalcHawkEyeView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

	void Accelerate( Vector& wishdir, float wishspeed, float accel );

	Vector		m_vCamOrigin;  //current camera origin
	QAngle		m_aCamAngle;   //current camera angle
	float		m_flFOV; // current FOV
	int			m_nCamMode; // current camera mode
	int			m_nTarget;	// first tracked target or 0
	CUserCmd	m_LastCmd;
	Vector		m_vecVelocity;

	float m_flNextUpdate;
	Vector m_vOldTargetPos;
	float m_flLerpTime;
	CUtlVector<Vector> m_BallPos;
	float m_flLastPossessionChange;
	int m_nLastPossessingTeam;
	float m_flPossCoeff;
	float m_flNewPossCoeff;
	float m_flOldPossCoeff;
};


extern C_Camera *Camera();	// get Singleton


#endif
