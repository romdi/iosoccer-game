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

// Spectator Movement modes
enum Cam_Modes_t
{
	CAM_MODE_TVCAM,
	CAM_MODE_ROAMING,
	CAM_MODE_FREE_CHASE,
	CAM_MODE_LOCKED_CHASE,
	CAM_MODE_COUNT,
};

static const char *g_szCamModeNames[CAM_MODE_COUNT] =
{
	"TV Camera",
	"Roaming",
	"Free Chase",
	"Locked Chase"
};

enum TVCam_Modes_t
{ 
	TVCAM_MODE_SIDELINE,
	TVCAM_MODE_FIXED_SIDELINE,
	TVCAM_MODE_BEHIND_GOAL,
	TVCAM_MODE_TOPDOWN,
	TVCAM_MODE_FLY_FOLLOW,
	TVCAM_MODE_GOAL_LINE,
	TVCAM_MODE_CELEBRATION,
	TVCAM_MODE_COUNT
};

static const char *g_szTVCamModeNames[TVCAM_MODE_COUNT] =
{
	"Sideline",
	"Fixed Sideline",
	"Behind Goal",
	"Top-Down",
	"Fly Follow",
	"Goal Line",
	"Celebration",
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
	int  GetTVCamMode();
	void SetTVCamMode(int mode);
	void SpecNextTarget(bool inverse);
	void SpecTargetByName(const char *name);
	void SpecTargetByIndex(int index);
	
	C_BaseEntity *GetTarget();  // return primary target
	void SetTarget( int nEntity); // set the primary obs target
	void CreateMove(CUserCmd *cmd);
	virtual void		CalcView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	void				ThirdpersonThink(Vector &camOffset, QAngle &camViewAngles);

	void CalcChaseCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	void CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	void CalcTVCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	void CalcHawkEyeView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);

protected:

	void GetTargetPos(Vector &targetPos, Vector &targetVel, bool &atBottomGoal);

	void Accelerate( Vector& wishdir, float wishspeed, float accel );

	Vector		m_vCamOrigin;  //current camera origin
	QAngle		m_aCamAngle;   //current camera angle
	float		m_flFOV; // current FOV
	int			m_nCamMode; // current camera mode
	int			m_nTVCamMode; // current camera mode
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
