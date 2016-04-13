//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ios_camera.h"
#include "cdll_client_int.h"
#include "util_shared.h"
#include "prediction.h"
#include "movevars_shared.h"
#include "in_buttons.h"
#include "text_message.h"
#include "vgui_controls/controls.h"
#include "vgui/ILocalize.h"
#include "vguicenterprint.h"
#include "game/client/iviewport.h"
#include <KeyValues.h>
#include "c_match_ball.h"
#include "c_ios_mapentities.h"
#include "iinput.h"
#include "c_playerresource.h"
#include "sdk_gamerules.h"
#include "c_team.h"
#include "c_ios_replaymanager.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_goal_opacity("cl_goal_opacity", "0.25", FCVAR_ARCHIVE, "Goal opacity when it obstructs the view");
ConVar cl_goal_opacity_fieldoffset("cl_goal_opacity_fieldoffset", "20", FCVAR_ARCHIVE, "Offset from the field end where to start making it transparent");

ConVar cl_cam_dist("cl_cam_dist", "175", FCVAR_ARCHIVE, "", true, 0, true, 175);
ConVar cl_cam_height("cl_cam_height", "15", FCVAR_ARCHIVE, "", true, -50, true, 50);
ConVar cl_cam_firstperson("cl_cam_firstperson", "0", FCVAR_ARCHIVE, "");

void CheckAutoTransparentProps(const Vector &pos, const QAngle &ang)
{
	float opacity = clamp(cl_goal_opacity.GetFloat(), 0.0f, 1.0f);

	for (int i = gpGlobals->maxClients; i <= ClientEntityList().GetHighestEntityIndex(); i++)
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntity(i);
		if(!dynamic_cast<C_AutoTransparentProp *>(pEnt))
			continue;

		// Check if the camera is behind the goal line and close to the goal. Use an additional offset so the goal post doesn't get in the way.
		if (opacity != 1.0f
			&& (pos.y <= SDKGameRules()->m_vFieldMin.GetY() + cl_goal_opacity_fieldoffset.GetFloat() && pEnt->GetLocalOrigin().y < SDKGameRules()->m_vKickOff.GetY()
				|| pos.y >= SDKGameRules()->m_vFieldMax.GetY() - cl_goal_opacity_fieldoffset.GetFloat() && pEnt->GetLocalOrigin().y > SDKGameRules()->m_vKickOff.GetY())
			&& pos.x >= SDKGameRules()->m_vKickOff.GetX() - 500 && pos.x <= SDKGameRules()->m_vKickOff.GetX() + 500)
		{
			pEnt->SetRenderMode(kRenderTransColor);
			pEnt->SetRenderColorA(opacity * 255);
		}
		else
		{
			pEnt->SetRenderMode(kRenderNormal);
		}
	}
}

void ResetAutoTransparentProps()
{
	for (int i = gpGlobals->maxClients; i <= ClientEntityList().GetHighestEntityIndex(); i++)
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntity(i);
		if(!dynamic_cast<C_AutoTransparentProp *>(pEnt))
			continue;

		pEnt->SetRenderMode(kRenderNormal);
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// converts all '\r' characters to '\n', so that the engine can deal with the properly
// returns a pointer to str
static wchar_t* ConvertCRtoNL( wchar_t *str )
{
	for ( wchar_t *ch = str; *ch != 0; ch++ )
		if ( *ch == L'\r' )
			*ch = L'\n';
	return str;
}

static C_Camera s_Camera;

C_Camera *Camera() { return &s_Camera; }

C_Camera::C_Camera()
{
	Reset();
}

C_Camera::~C_Camera()
{

}

void C_Camera::Init()
{
	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "hltv_message" );
	
	Reset();
}

void C_Camera::Reset()
{
	m_nCamMode = CAM_MODE_TVCAM;
	m_nTVCamMode = TVCAM_MODE_SIDELINE;
	m_nTarget = 0;
	m_flFOV = 90;
	m_vCamOrigin.Init();
	m_aCamAngle.Init();

	m_LastCmd.Reset();
	m_vecVelocity.Init();

	m_flNextUpdate = gpGlobals->curtime;
	m_vOldTargetPos = vec3_invalid;
	m_flLerpTime = 0;
	m_flLastPossessionChange = 0;
	m_nLastPossessingTeam = TEAM_NONE;
}

int C_Camera::GetCamMode()
{
	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();
	if (!pLocal)
		return CAM_MODE_ROAMING;

	if (GetReplayManager() && GetReplayManager()->IsReplaying())
	{
		return CAM_MODE_TVCAM;
	}
	else if (GetMatchBall() && GetMatchBall()->m_eBallState == BALL_STATE_GOAL || SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
	{
		if (pLocal->IsObserver() || pLocal->GetFlags() & FL_USE_TV_CAM)
			return CAM_MODE_TVCAM;
		else
			return CAM_MODE_FREE_CHASE;
	}
	else
	{
		if (pLocal->IsObserver())
			return m_nCamMode;
		else
			return CAM_MODE_FREE_CHASE;
	}
}

void C_Camera::SetCamMode(int mode)
{
	if (mode == m_nCamMode)
		return;

	m_nCamMode = clamp(mode, 0, CAM_MODE_COUNT);

	IGameEvent *event = gameeventmanager->CreateEvent("cam_mode_updated");
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}

extern ConVar
	sv_replay_instant_first_camera,
	sv_replay_instant_second_camera,
	sv_replay_instant_third_camera,
	sv_replay_highlight_first_camera,
	sv_replay_highlight_second_camera,
	sv_replay_highlight_third_camera;

int C_Camera::GetTVCamMode()
{
	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();
	if (!pLocal)
		return TVCAM_MODE_SIDELINE;

	if (SDKGameRules()->IsCeremony())
	{
		return TVCAM_MODE_SIDELINE;
	}
	else if (GetReplayManager() && GetReplayManager()->IsReplaying())
	{
		if (SDKGameRules()->IsIntermissionState())
		{
			switch (GetReplayManager()->m_nReplayRunIndex)
			{
			case 0: default: return sv_replay_highlight_first_camera.GetInt();
			case 1: return sv_replay_highlight_second_camera.GetInt();
			case 2: return sv_replay_highlight_third_camera.GetInt();
			}
		}
		else
		{
			switch (GetReplayManager()->m_nReplayRunIndex)
			{
			case 0: default: return sv_replay_instant_first_camera.GetInt();
			case 1: return sv_replay_instant_second_camera.GetInt();
			case 2: return sv_replay_instant_third_camera.GetInt();
			}
		}
	}
	else if (GetMatchBall() && GetMatchBall()->m_eBallState == BALL_STATE_GOAL)
	{
		return TVCAM_MODE_CELEBRATION;
	}
	else if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
	{
		return TVCAM_MODE_BEHIND_GOAL;
	}
	else
	{
		return m_nTVCamMode;
	}
}

void C_Camera::SetTVCamMode(int mode)
{
	if (mode == m_nTVCamMode)
		return;

	m_nTVCamMode = clamp(mode, 0, TVCAM_MODE_COUNT);

	IGameEvent *event = gameeventmanager->CreateEvent("tvcam_mode_updated");
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}

C_BaseEntity* C_Camera::GetTarget()
{
	if ( m_nTarget <= 0 )
		return NULL;

	C_BaseEntity* target = ClientEntityList().GetEnt( m_nTarget );

	return target;
}

void C_Camera::SetTarget( int nEntity ) 
{
 	if ( m_nTarget == nEntity )
		return;

	m_nTarget = nEntity;

	IGameEvent *event = gameeventmanager->CreateEvent("spec_target_updated");
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}

void C_Camera::SpecNextTarget(bool inverse)
{
	int start = 0;

	if (m_nTarget > 0 && m_nTarget <= gpGlobals->maxClients)
		start = m_nTarget;
	else if (GetMatchBall() && m_nTarget == GetMatchBall()->entindex())
		start = 0;

	int index = start;

	while ( true )
	{	
		// got next/prev player
		if (inverse)
			index--;
		else
			index++;

		// check bounds
		if ( index < 0 )
		{
			index = gpGlobals->maxClients;
		}
		else if ( index > gpGlobals->maxClients )
		{
			index = 0;
		}

		if ( index == start )
			break; // couldn't find a new valid player

		if (index > 0 && index <= gpGlobals->maxClients)
		{
			C_BasePlayer *pPlayer =	UTIL_PlayerByIndex( index );

			if ( !pPlayer )
				continue;

			// only follow living players 
			if ( pPlayer->IsObserver() )
				continue;
		}

		break; // found a new player
	}

	if (index == 0 && GetMatchBall())
		index = GetMatchBall()->entindex();

	if (index != 0)
		SetTarget( index );
}

void C_Camera::SpecTargetByName(const char *name)
{
	if (!Q_stricmp(name, "ball"))
	{
		if (GetMatchBall())
		{
			SetTarget(GetMatchBall()->entindex());
		}
	}
	else
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			C_BasePlayer *pPlayer =	UTIL_PlayerByIndex(i);

			if (!pPlayer || Q_strcmp(name, pPlayer->GetPlayerName()))
				continue;

			SetTarget(i);
			break;
		}
	}
}

void C_Camera::SpecTargetByIndex(int index)
{
	if (index > gpGlobals->maxClients)
	{
		if (GetMatchBall() && GetMatchBall()->entindex() == index)
			SetTarget(index);
	}
	else
	{
		C_BasePlayer *pPlayer =	UTIL_PlayerByIndex( index );
		if (pPlayer)
			SetTarget(index);
	}
}

void C_Camera::FireGameEvent( IGameEvent * event)
{
	if ( !engine->IsHLTV() )
		return;	// not in HLTV mode

	const char *type = event->GetName();

	if ( Q_strcmp( "game_newmap", type ) == 0 )
	{
		//Reset();	// reset all camera settings

		//// show spectator UI
		//if ( !gViewPortInterface )
		//	return;

		//if ( engine->IsPlayingDemo() )
  //      {
		//	// for demo playback show full menu
		//	gViewPortInterface->ShowPanel( PANEL_SPECMENU, true );

		//	SetCamMode( CAM_MODE_ROAMING );
		//}
		//else
		//{
		//	// during live broadcast only show black bars
		//	gViewPortInterface->ShowPanel( PANEL_SPECGUI, true );
		//}

		return;
	}

	if ( Q_strcmp( "hltv_message", type ) == 0 )
	{
		wchar_t outputBuf[1024];
		const char *pszText = event->GetString( "text", "" );
		
		char *tmpStr = hudtextmessage->LookupString( pszText );
		const wchar_t *pBuf = g_pVGuiLocalize->Find( tmpStr );
		if ( pBuf )
		{
			// Copy pBuf into szBuf[i].
			int nMaxChars = sizeof( outputBuf ) / sizeof( wchar_t );
			wcsncpy( outputBuf, pBuf, nMaxChars );
			outputBuf[nMaxChars-1] = 0;
		}
		else
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( tmpStr, outputBuf, sizeof(outputBuf) );
		}

		internalCenterPrint->Print( ConvertCRtoNL( outputBuf ) );
		return ;
	}
}

// this is a cheap version of FullNoClipMove():
void C_Camera::CreateMove( CUserCmd *cmd)
{
	if ( cmd )
	{
		m_LastCmd = *cmd;
	}
}

// movement code is a copy of CGameMovement::FullNoClipMove()
void C_Camera::CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	Vector wishvel;
	Vector forward, right, up;
	Vector wishdir;
	float wishspeed;
	float factor = sv_specspeed.GetFloat();
	float maxspeed = sv_maxspeed.GetFloat() * factor;

	AngleVectors ( m_LastCmd.viewangles, &forward, &right, &up);  // Determine movement angles

	if ( m_LastCmd.buttons & IN_SPEED )
	{
		factor /= 2.0f;
	}

	// Copy movement amounts
	float fmove = m_LastCmd.forwardmove * factor;
	float smove = m_LastCmd.sidemove * factor;

	VectorNormalize (forward);  // Normalize remainder of vectors
	VectorNormalize (right);    // 

	for (int i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] += m_LastCmd.upmove * factor;

	VectorCopy (wishvel, wishdir);   // Determine magnitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to server defined max speed
	//
	if (wishspeed > maxspeed )
	{
		VectorScale (wishvel, maxspeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}

	if ( sv_specaccelerate.GetFloat() > 0.0 )
	{
		// Set move velocity
		Accelerate ( wishdir, wishspeed, sv_specaccelerate.GetFloat() );

		float spd = VectorLength( m_vecVelocity );
		if (spd < 1.0f)
		{
			m_vecVelocity.Init();
		}
		else
		{
			// Bleed off some speed, but if we have less than the bleed
			//  threshold, bleed the threshold amount.
			float control = (spd < maxspeed/4.0) ? maxspeed/4.0 : spd;

			float friction = sv_friction.GetFloat();

			// Add the amount to the drop amount.
			float drop = control * friction * gpGlobals->frametime;

			// scale the velocity
			float newspeed = spd - drop;
			if (newspeed < 0)
				newspeed = 0;

			// Determine proportion of old speed we are using.
			newspeed /= spd;
			VectorScale( m_vecVelocity, newspeed, m_vecVelocity );
		}
	}
	else
	{
		VectorCopy( wishvel, m_vecVelocity );
	}

	// Just move ( don't clip or anything )
	VectorMA( m_vCamOrigin, gpGlobals->frametime, m_vecVelocity, m_vCamOrigin );

	// get camera angle directly from engine
	engine->GetViewAngles( m_aCamAngle );

	// Zero out velocity if in noaccel mode
	if ( sv_specaccelerate.GetFloat() < 0.0f )
	{
		m_vecVelocity.Init();
	}

	eyeOrigin = m_vCamOrigin;
	eyeAngles = m_aCamAngle;
	fov = m_flFOV;
}

void C_Camera::Accelerate( Vector& wishdir, float wishspeed, float accel )
{
	float addspeed, accelspeed, currentspeed;

	// See if we are changing direction a bit
	currentspeed =m_vecVelocity.Dot(wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of acceleration.
	accelspeed = accel * gpGlobals->frametime * wishspeed;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust velocity.
	for (int i=0 ; i<3 ; i++)
	{
		m_vecVelocity[i] += accelspeed * wishdir[i];	
	}
}

void C_Camera::CalcView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	if (GetCamMode() == CAM_MODE_TVCAM)
	{
		CalcTVCamView(eyeOrigin, eyeAngles, fov);
		ResetAutoTransparentProps();
	}
	else if (GetCamMode() == CAM_MODE_FREE_CHASE || GetCamMode() == CAM_MODE_LOCKED_CHASE)
	{
		CalcChaseCamView(eyeOrigin, eyeAngles, fov);
		CheckAutoTransparentProps(eyeOrigin, eyeAngles);
	}
	else
	{
		CalcRoamingView(eyeOrigin, eyeAngles, fov);
		CheckAutoTransparentProps(eyeOrigin, eyeAngles);
	}

	// Save the last position and angle so when switching to roaming view it starts at the current position
	m_vCamOrigin = eyeOrigin;
	m_aCamAngle = eyeAngles;
}

void C_Camera::CalcHawkEyeView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();

	eyeOrigin = SDKGameRules()->m_vKickOff;
	eyeOrigin.z += 500;
	eyeOrigin.y += pLocal->GetTeam()->m_nForward * ((SDKGameRules()->m_vFieldMax.GetY() - SDKGameRules()->m_vFieldMin.GetY()) * (1.0f / 8.0f));
}

void C_Camera::CalcChaseCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();

	C_BaseEntity *pTarget = NULL;

	if ((pLocal->m_nButtons & IN_ZOOM)
		&& !pLocal->IsObserver()
		&& g_PR->GetTeamPosType(GetLocalPlayerIndex()) == POS_GK
		&& GetMatchBall()
		&& Sign(GetMatchBall()->GetLocalOrigin().y - SDKGameRules()->m_vKickOff.GetY()) == pLocal->GetTeam()->m_nForward)
	{
		CalcHawkEyeView(eyeOrigin, eyeAngles, fov);
		return;
	}

	if (pLocal->IsObserver())
		pTarget = GetTarget();
	else
		pTarget = pLocal;

	if (!pTarget || !pTarget->GetBaseAnimating() && !pTarget->GetModel())
	{
		CalcRoamingView( eyeOrigin, eyeAngles, fov );
		return;
	}

	eyeOrigin = pTarget->EyePosition();
	eyeAngles = pTarget->EyeAngles();

	const QAngle camAngles = ::input->GetCameraAngles();
	Vector &camOffset = ::input->GetCameraOffset();

	float dist = cl_cam_firstperson.GetBool() ? -10 : cl_cam_dist.GetFloat();
	float height = cl_cam_firstperson.GetBool() ? 8 : cl_cam_height.GetFloat();

	if (pLocal->IsObserver() && GetCamMode() == CAM_MODE_LOCKED_CHASE && !dynamic_cast<C_MatchBall *>(pTarget))
	{
		camOffset[PITCH] = eyeAngles[PITCH];
		camOffset[YAW] = eyeAngles[YAW];
	}
	else
	{
		camOffset[PITCH] = camAngles[PITCH];
		camOffset[YAW] = camAngles[YAW];
	}

	if (camOffset[PITCH] >= 0)
	{
		camOffset[ROLL] = dist;
	}
	else
	{
		float coeff = clamp(cos(DEG2RAD(camOffset[PITCH] + 90)), 0.001f, 1.0f);
		camOffset[ROLL] = min((VEC_VIEW.z + height - 5) / coeff, dist);
	}

	eyeAngles[PITCH] = camOffset[PITCH];
	eyeAngles[YAW] = camOffset[YAW];
	eyeAngles[ROLL] = 0;

	Vector camForward, camRight, camUp;
	AngleVectors(eyeAngles, &camForward, &camRight, &camUp);

	VectorMA(eyeOrigin, -camOffset[ROLL], camForward, eyeOrigin);

	eyeOrigin.z += height;

	if (!pLocal->IsObserver())
	{
		// Apply a smoothing offset to smooth out prediction errors.
		Vector vSmoothOffset;
		pLocal->GetPredictionErrorSmoothingVector( vSmoothOffset );
		eyeOrigin += Vector(vSmoothOffset.x, vSmoothOffset.y, 0);
	}
	
	fov = pLocal->GetFOV();
}

void C_Camera::GetTargetPos(Vector &targetPos, Vector &targetVel, bool &atBottomGoal)
{
	C_BaseEntity *pTarget;

	if (GetReplayManager() && GetReplayManager()->IsReplaying())
	{
		pTarget = GetReplayBall();

		if (!pTarget)
			return;

		atBottomGoal = GetReplayManager()->m_bAtMinGoalPos;
		targetPos = pTarget->GetLocalOrigin();
	}
	else
	{
		if (GetMatchBall() && GetMatchBall()->m_eBallState == BALL_STATE_GOAL)
		{
			pTarget = GetMatchBall()->m_pLastActivePlayer;

			if (!pTarget)
				pTarget = GetMatchBall();

			atBottomGoal = pTarget->GetLocalOrigin().y < SDKGameRules()->m_vKickOff.GetY();
			targetPos = pTarget->GetLocalOrigin();
		}
		else
		{
			pTarget = CBasePlayer::GetLocalPlayer()->GetObserverTarget();
			if (!pTarget)
				pTarget = GetMatchBall();

			atBottomGoal = pTarget->GetLocalOrigin().y < SDKGameRules()->m_vKickOff.GetY();
			targetPos = pTarget->GetLocalOrigin();
			targetVel = pTarget->GetLocalVelocity();

			//return;

			// Move the camera towards the defending team's goal
			if (GetMatchBall()->m_nLastActiveTeam != TEAM_NONE)
			{
				if (m_nLastPossessingTeam == TEAM_NONE)
				{
					m_nLastPossessingTeam = GetMatchBall()->m_nLastActiveTeam;
					m_flLastPossessionChange = gpGlobals->curtime;
					m_flPossCoeff = 0;
					m_flOldPossCoeff = 0;
				}
				else
				{
					if (GetMatchBall()->m_nLastActiveTeam != m_nLastPossessingTeam)
					{
						m_nLastPossessingTeam = GetMatchBall()->m_nLastActiveTeam;
						m_flLastPossessionChange = gpGlobals->curtime;
						m_flOldPossCoeff = m_flPossCoeff;
					}

					float timeFrac = min(1.0f, (gpGlobals->curtime - m_flLastPossessionChange) / mp_tvcam_offset_forward_time.GetFloat());
					float frac = pow(timeFrac, 2) * (3 - 2 * timeFrac);
					m_flPossCoeff = Lerp(frac, m_flOldPossCoeff, (float)GetGlobalTeam(GetMatchBall()->m_nLastActiveTeam)->m_nForward);
				}

				if (GetTVCamMode() == TVCAM_MODE_SIDELINE)
				{
					targetPos.y += m_flPossCoeff * mp_tvcam_offset_forward.GetInt();
				}
			}
		}
	}
}

void C_Camera::CalcTVCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	if (!GetMatchBall())
		return;

	bool atBottomGoal;
	Vector targetPos, targetVel;
	int tvcamMode = GetTVCamMode();

	GetTargetPos(targetPos, targetVel, atBottomGoal);

	Vector tmpTargetPos = targetPos;

	if (tvcamMode != TVCAM_MODE_CELEBRATION)
	{
		// Make sure the camera doesn't move too far away from the field borders
		targetPos.x = clamp(targetPos.x, SDKGameRules()->m_vFieldMin.GetX() + mp_tvcam_border_south.GetInt(), SDKGameRules()->m_vFieldMax.GetX() - mp_tvcam_border_north.GetInt());
		targetPos.y = clamp(targetPos.y, SDKGameRules()->m_vFieldMin.GetY() + mp_tvcam_border_west.GetInt(), SDKGameRules()->m_vFieldMax.GetY() - mp_tvcam_border_east.GetInt());
		targetPos.z = SDKGameRules()->m_vKickOff.GetZ();
	}

	if (m_vOldTargetPos == vec3_invalid)
		m_vOldTargetPos = targetPos;
	else
	{
		float speedCoeff = tvcamMode == TVCAM_MODE_CELEBRATION ? mp_tvcam_speed_coeff_fast.GetFloat() : mp_tvcam_speed_coeff.GetFloat();
		float speedExp = tvcamMode == TVCAM_MODE_CELEBRATION ? mp_tvcam_speed_exponent_fast.GetFloat() : mp_tvcam_speed_exponent.GetFloat();
		Vector changeDir = targetPos - m_vOldTargetPos;
		float changeDist = changeDir.Length();
		changeDir.NormalizeInPlace();
		targetPos = m_vOldTargetPos + changeDir * min(changeDist, pow(changeDist * speedCoeff, speedExp) * gpGlobals->frametime);
	}

	switch (GetTVCamMode())
	{
	case TVCAM_MODE_SIDELINE:
		{
			if (SDKGameRules() && SDKGameRules()->IsIntermissionState() && GetReplayManager() && !GetReplayManager()->IsReplaying())
			{
				//float xLength = SDKGameRules()->m_vFieldMax.GetX() - SDKGameRules()->m_vFieldMin.GetX();
				//float yLength = SDKGameRules()->m_vFieldMax.GetY() - SDKGameRules()->m_vFieldMin.GetY();

				float zPos = 450;
				Vector points[4];
				points[0] = Vector(SDKGameRules()->m_vFieldMin.GetX(), SDKGameRules()->m_vFieldMin.GetY(), SDKGameRules()->m_vKickOff.GetZ() + zPos);
				points[1] = Vector(SDKGameRules()->m_vFieldMin.GetX(), SDKGameRules()->m_vFieldMax.GetY(), SDKGameRules()->m_vKickOff.GetZ() + zPos);
				points[2] = Vector(SDKGameRules()->m_vFieldMax.GetX(), SDKGameRules()->m_vFieldMax.GetY(), SDKGameRules()->m_vKickOff.GetZ() + zPos);
				points[3] = Vector(SDKGameRules()->m_vFieldMax.GetX(), SDKGameRules()->m_vFieldMin.GetY(), SDKGameRules()->m_vKickOff.GetZ() + zPos);
				float totalLength = (points[1] - points[0]).Length() + (points[2] - points[1]).Length() + (points[3] - points[2]).Length() + (points[0] - points[3]).Length();

				Vector newPoints[4];

				const float maxDuration = 180.0f;
				float timePassed = fmodf(gpGlobals->curtime, maxDuration);
				
				float lengthSum = 0;
				int offset = 0;
				float length = 0;

				do
				{
					for (int i = 0; i < 4; i++)
						newPoints[i] = points[(i + offset) % 4];

					length = (newPoints[2] - newPoints[1]).Length();
					lengthSum += length;
					offset += 1;
				} while (timePassed > (lengthSum / totalLength * maxDuration));

				float maxStepTime = length / totalLength * maxDuration;

				float frac = 1 - ((lengthSum / totalLength * maxDuration) - timePassed) / maxStepTime;

				Vector output;
				Catmull_Rom_Spline(newPoints[0], newPoints[1], newPoints[2], newPoints[3], frac, output);

/*				float targetDist = (newPoints[2] - newPoints[1]).Length() * frac;
				float epsilon = 10.0f;
				float oldDiff = 0;
				float diff = 0;
				float change = 0.001f;
				bool add = true;

				do
				{
					frac = clamp(frac += (add ? change : -change), 0.0f, 1.0f);

					Catmull_Rom_Spline(newPoints[0], newPoints[1], newPoints[2], newPoints[3], frac, output);
					oldDiff = diff;
					diff = abs((output - newPoints[1]).Length() - targetDist);
					if (diff >= oldDiff)
						add = !add;
				} while (diff > epsilon);*/ 

				eyeOrigin = output;
				VectorAngles(SDKGameRules()->m_vKickOff - output, eyeAngles);
			}
			/*else if (SDKGameRules() && !SDKGameRules()->IsIntermissionState() && gpGlobals->curtime <= SDKGameRules()->m_flStateEnterTime + 4)
			{
				Vector points[4];
				float zPosStart = 450;
				float zPosEnd = 50;
				points[0] = Vector(SDKGameRules()->m_vFieldMin.GetX(), SDKGameRules()->m_vKickOff.GetY() + 10000, SDKGameRules()->m_vKickOff.GetZ() + zPosStart);
				points[1] = Vector(SDKGameRules()->m_vFieldMin.GetX(), SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ() + zPosStart);
				points[2] = Vector(SDKGameRules()->m_vKickOff.GetX(), SDKGameRules()->m_vKickOff.GetY() - 300, SDKGameRules()->m_vKickOff.GetZ() + zPosEnd);
				points[3] = Vector(SDKGameRules()->m_vKickOff.GetX(), SDKGameRules()->m_vKickOff.GetY() + 10000, SDKGameRules()->m_vKickOff.GetZ() + zPosEnd);
				float frac = min(1.0f, (gpGlobals->curtime - SDKGameRules()->m_flStateEnterTime) / 3.0f);
				Vector output;
				Catmull_Rom_Spline(points[0], points[1], points[2], points[3], frac, output);
				eyeOrigin = output;
				VectorAngles(Vector(SDKGameRules()->m_vKickOff.GetX(), SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ() + 100) - output, eyeAngles);
			}*/
			else
			{	
				/*
				Vector innerWindow = Vector(500, 200, 0);
				Vector outerWindow = Vector(750, 500, 1000);

				Vector camPos = m_vOldTargetPos;
				targetPos = tmpTargetPos;

				for (int axis = 0; axis < 3; axis++)
				{
					if ((targetPos[axis] < camPos[axis] - outerWindow[axis] || targetPos[axis] > camPos[axis] - innerWindow[axis])
						&& (targetPos[axis] < camPos[axis] + innerWindow[axis] || targetPos[axis] > camPos[axis] + outerWindow[axis]))
					{
						if (targetVel[axis] > 0)
							camPos[axis] = targetPos[axis] + innerWindow[axis];
						else
							camPos[axis] = targetPos[axis] - innerWindow[axis];
					}
				}

				targetPos = camPos;
				*/

				Vector newPos = Vector(targetPos.x - mp_tvcam_sideline_offset_north.GetInt(), targetPos.y, targetPos.z + mp_tvcam_sideline_offset_height.GetInt());
				Vector offsetTargetPos = Vector(targetPos.x - mp_tvcam_offset_north.GetInt(), targetPos.y, targetPos.z);
				Vector newDir = offsetTargetPos - newPos;
				newDir.NormalizeInPlace();
				eyeOrigin = newPos;
				VectorAngles(newDir, eyeAngles);
			}
		}
		break;
	case TVCAM_MODE_FIXED_SIDELINE:
		{
			Vector newPos = Vector(SDKGameRules()->m_vFieldMin.GetX() - 500, SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ() + 1000);
			Vector offsetTargetPos = Vector(targetPos.x - 500, targetPos.y, targetPos.z);
			Vector newDir = offsetTargetPos - newPos;
			float dist = newDir.Length();
			newDir.NormalizeInPlace();
			newPos = offsetTargetPos - min(750, dist) * newDir;
			eyeOrigin = newPos;
			VectorAngles(newDir, eyeAngles);
		}
		break;
	case TVCAM_MODE_BEHIND_GOAL:
		{
			float yPos = atBottomGoal ? SDKGameRules()->m_vFieldMin.GetY() : SDKGameRules()->m_vFieldMax.GetY();
			Vector goalCenter = Vector((SDKGameRules()->m_vFieldMin.GetX() + SDKGameRules()->m_vFieldMax.GetX()) / 2.0f, yPos, SDKGameRules()->m_vKickOff.GetZ());
			Vector newPos = goalCenter + Vector(0, 300 * (atBottomGoal ? -1 : 1), 300);
			Vector newDir = targetPos - newPos;
			newDir.NormalizeInPlace();
			eyeOrigin = newPos;
			VectorAngles(newDir, eyeAngles);
		}
		break;
	case TVCAM_MODE_TOPDOWN:
		{
			eyeOrigin = Vector(targetPos.x, targetPos.y, SDKGameRules()->m_vKickOff.GetZ() + 1000);
			eyeAngles = QAngle(89, 0, 0);
		}
		break;
	case TVCAM_MODE_FLY_FOLLOW:
		{
			Vector newPos = Vector(targetPos.x, targetPos.y + (atBottomGoal ? 1 : -1) * 500, SDKGameRules()->m_vKickOff.GetZ() + 225);
			Vector newDir = targetPos - newPos;
			newDir.NormalizeInPlace();
			eyeOrigin = newPos;
			VectorAngles(newDir, eyeAngles);
		}
		break;
	case TVCAM_MODE_GOAL_LINE:
		{
			Vector center = Vector(SDKGameRules()->m_vKickOff.GetX(), (atBottomGoal ? SDKGameRules()->m_vFieldMin.GetY() + 5 : SDKGameRules()->m_vFieldMax.GetY() - 5), SDKGameRules()->m_vKickOff.GetZ());
			Vector newPos = center;
			newPos.x -= 350;
			newPos.z += 200;
			QAngle newAng;
			VectorAngles(center - newPos, newAng);
			eyeOrigin = newPos;
			eyeAngles = newAng;
		}
		break;
	case TVCAM_MODE_CELEBRATION:
		{
			Vector newPos = Vector(
				targetPos.x < SDKGameRules()->m_vKickOff.GetX() ? SDKGameRules()->m_vFieldMin.GetX() - 500 : SDKGameRules()->m_vFieldMax.GetX() + 500,
				targetPos.y < SDKGameRules()->m_vKickOff.GetY() ? SDKGameRules()->m_vFieldMin.GetY() - 500 : SDKGameRules()->m_vFieldMax.GetY() + 500,
				SDKGameRules()->m_vKickOff.GetZ() + 1000);
			Vector newDir = targetPos - newPos;
			float dist = newDir.Length();
			newDir.NormalizeInPlace();
			newPos = targetPos - min(300, dist) * newDir;
			eyeOrigin = newPos;
			VectorAngles(newDir, eyeAngles);
		}
		break;
	}

	m_vOldTargetPos = targetPos;

	fov = C_SDKPlayer::GetLocalSDKPlayer()->GetFOV();
}