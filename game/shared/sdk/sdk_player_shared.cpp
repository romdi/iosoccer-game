//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

#include "sdk_gamerules.h"

#include "takedamageinfo.h"

#include "effect_dispatch_data.h"
#include "weapon_sdkbase.h"
#include "movevars_shared.h"
#include "gamevars_shared.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "engine/ivdebugoverlay.h"
#include "obstacle_pushaway.h"
#include "props_shared.h"

#include "decals.h"
#include "util_shared.h"

#ifdef CLIENT_DLL
	
	#include "c_sdk_player.h"
	#include "c_team.h"
	#include "prediction.h"
	#include "clientmode_sdk.h"
	#include "vgui_controls/AnimationController.h"

	#define CRecipientFilter C_RecipientFilter
#else
	#include "sdk_player.h"
	#include "team.h"
#endif
ConVar sv_showimpacts("sv_showimpacts", "0", FCVAR_REPLICATED, "Shows client (red) and server (blue) bullet impact point" );

void DispatchEffect( const char *pName, const CEffectData &data );

// BUG! This is not called on the client at respawn, only first spawn!
void CSDKPlayer::SharedSpawn()
{	
	BaseClass::SharedSpawn();

	// Reset the animation state or we will animate to standing
	// when we spawn

	m_Shared.SetJumping( false );
}

void CSDKPlayer::SetSprinting( bool bIsSprinting )
{
	m_Shared.SetSprinting( bIsSprinting );
}

bool CSDKPlayer::IsSprinting( void )
{
	float flVelSqr = GetAbsVelocity().LengthSqr();

	//ios return m_Shared.m_bIsSprinting && ( flVelSqr > 0.5f );
	return m_Shared.IsSprinting() && ( flVelSqr > 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the players mins - overridden for prone
// Input  : 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector CSDKPlayer::GetPlayerMins( void ) const
{

	return VEC_HULL_MIN;
}

//-----------------------------------------------------------------------------
// Purpose:  Returns the players Maxs - overridden for prone
// Input  : 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector CSDKPlayer::GetPlayerMaxs( void ) const
{	
	return VEC_HULL_MAX;
}


// --------------------------------------------------------------------------------------------------- //
// CSDKPlayerShared implementation.
// --------------------------------------------------------------------------------------------------- //
CSDKPlayerShared::CSDKPlayerShared()
{
}

CSDKPlayerShared::~CSDKPlayerShared()
{
}

void CSDKPlayerShared::Init( CSDKPlayer *pPlayer )
{
	m_pOuter = pPlayer;
}

void CSDKPlayerShared::SetSprinting( bool bSprinting )
{
	//ios if ( bSprinting && !m_bIsSprinting )
	if ( bSprinting && !IsSprinting())
	{
		StartSprinting();

		// only one penalty per key press
		if ( m_bGaveSprintPenalty == false )
		{
			m_flStamina -= INITIAL_SPRINT_STAMINA_PENALTY;
			m_bGaveSprintPenalty = true;
		}
		//ios always apply this penalty as we're predicting m_bSprinting
		m_flStamina -= INITIAL_SPRINT_STAMINA_PENALTY;
	}
	//ios else if ( !bSprinting && m_bIsSprinting )
	else if ( !bSprinting && IsSprinting() )
	{
		StopSprinting();
	}
}

// this is reset when we let go of the sprint key
void CSDKPlayerShared::ResetSprintPenalty( void )
{
	m_bGaveSprintPenalty = false;
}

void CSDKPlayerShared::StartSprinting( void )
{
	m_bIsSprinting = true;
}

void CSDKPlayerShared::StopSprinting( void )
{
	m_bIsSprinting = false;
}

void CSDKPlayerShared::SetJumping( bool bJumping )
{
	m_bJumping = bJumping;
}

void CSDKPlayerShared::SetStamina( float flStamina )
{
	m_flStamina = clamp( flStamina, 0, 100 );
}

void CSDKPlayerShared::ComputeWorldSpaceSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	Vector org = m_pOuter->GetAbsOrigin();

	static Vector vecMin(-32, -32, 0 );
	static Vector vecMax(32, 32, 72 );

	VectorAdd( vecMin, org, *pVecWorldMins );
	VectorAdd( vecMax, org, *pVecWorldMaxs );
}

void CSDKPlayer::InitSpeeds()
{
	m_Shared.m_flRunSpeed = mp_runspeed.GetInt();
	m_Shared.m_flSprintSpeed = mp_sprintspeed.GetInt();
	// Set the absolute max to sprint speed
	SetMaxSpeed( m_Shared.m_flRunSpeed ); 
	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSDKPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

void CSDKPlayer::MoveToTargetPos(Vector &pos, Vector &vel, QAngle &ang)
{
	Vector dir = m_vTargetPos - pos;
	dir.z = 0;
	float distToTarget = dir.Length2D();
	dir.NormalizeInPlace();

	VectorAngles(dir, ang);

	float wishDist = mp_remotecontrolledspeed.GetInt() * gpGlobals->frametime;
	vel = dir * mp_remotecontrolledspeed.GetInt();

	if (wishDist < distToTarget)
	{
		pos = pos + vel * gpGlobals->frametime;
		pos.z = SDKGameRules()->m_vKickOff.GetZ();
	}
	else
	{
		pos = m_vTargetPos;

		trace_t	trace;
		UTIL_TraceHull(pos, pos, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER, &trace);

		if (trace.startsolid)
		{
			m_vTargetPos = pos + dir * VEC_HULL_MAX.x * 2;
		}
		else
		{
			m_bIsAtTargetPos = true;
			RemoveFlag(FL_REMOTECONTROLLED);
			RemoveSolidFlags(FSOLID_NOT_SOLID);

			if (m_bHoldAtTargetPos)
				AddFlag(FL_ATCONTROLS);

#ifdef GAME_DLL
			if (m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1)))
				m_bShotButtonsReleased = false;
#endif

			vel = vec3_origin;
		}
	}
}

void CSDKPlayer::CheckBallShield(const Vector &oldPos, Vector &newPos, const Vector &oldVel, Vector &newVel, const QAngle &oldAng, QAngle &newAng)
{
	bool stopPlayer = false;

	if (SDKGameRules()->m_nShieldType != SHIELD_NONE)
	{
		float border = (GetFlags() & FL_SHIELD_KEEP_IN) ? -mp_shield_border.GetInt() : mp_shield_border.GetInt();

		if (SDKGameRules()->m_nShieldType == SHIELD_GOALKICK || 
			SDKGameRules()->m_nShieldType == SHIELD_PENALTY ||
			SDKGameRules()->m_nShieldType == SHIELD_KEEPERHANDS)
		{
			Vector min = GetGlobalTeam(SDKGameRules()->m_nShieldDir)->m_vPenBoxMin - border;
			Vector max = GetGlobalTeam(SDKGameRules()->m_nShieldDir)->m_vPenBoxMax + border;

			if (GetFlags() & FL_SHIELD_KEEP_OUT || SDKGameRules()->m_nShieldType == SHIELD_PENALTY)
			{
				if (SDKGameRules()->m_vKickOff.GetY() > min.y)
					min.y -= 200;
				else
					max.y += 200;
			}

			bool isInsideBox = newPos.x > min.x && newPos.y > min.y && newPos.x < max.x && newPos.y < max.y; 
			Vector boxCenter = (min + max) / 2;

			if (GetFlags() & FL_SHIELD_KEEP_OUT && isInsideBox)
			{
				if (newPos.x > min.x && oldPos.x <= min.x && newPos.x < boxCenter.x)
					newPos.x = min.x;
				else if (newPos.x < max.x && oldPos.x >= max.x && newPos.x > boxCenter.x)
					newPos.x = max.x;

				if (newPos.y > min.y && oldPos.y <= min.y && newPos.y < boxCenter.y)
					newPos.y = min.y;
				else if (newPos.y < max.y && oldPos.y >= max.y && newPos.y > boxCenter.y)
					newPos.y = max.y;

				stopPlayer = true;
			}
			else if (GetFlags() & FL_SHIELD_KEEP_IN && !isInsideBox)
			{
				if (newPos.x < min.x)
					newPos.x = min.x;
				else if (newPos.x > max.x)
					newPos.x = max.x;

				if (newPos.y < min.y)
					newPos.y = min.y;
				else if (newPos.y > max.y)
					newPos.y = max.y;

				stopPlayer = true;
			}
		}

		if (SDKGameRules()->m_nShieldType == SHIELD_THROWIN || 
			SDKGameRules()->m_nShieldType == SHIELD_FREEKICK || 
			SDKGameRules()->m_nShieldType == SHIELD_CORNER ||  
			SDKGameRules()->m_nShieldType == SHIELD_KICKOFF ||
			SDKGameRules()->m_nShieldType == SHIELD_PENALTY && (GetFlags() & FL_SHIELD_KEEP_OUT))
		{
			float radius = SDKGameRules()->GetShieldRadius() + border;
			Vector dir = newPos - SDKGameRules()->m_vShieldPos;

			if (GetFlags() & FL_SHIELD_KEEP_OUT && dir.Length2D() < radius || GetFlags() & FL_SHIELD_KEEP_IN && dir.Length2D() > radius)
			{
				dir.z = 0;
				dir.NormalizeInPlace();
				newPos = SDKGameRules()->m_vShieldPos + dir * radius;
				stopPlayer = true;
			}

			if (SDKGameRules()->m_nShieldType == SHIELD_KICKOFF && GetFlags() & FL_SHIELD_KEEP_OUT)
			{
				int forward;
				#ifdef CLIENT_DLL
					forward = GetPlayersTeam(this)->m_nForward;
				#else
					forward = GetTeam()->m_nForward;
				#endif
				float yBorder = SDKGameRules()->m_vKickOff.GetY() - abs(border) * forward;
				if (Sign(newPos.y - yBorder) == forward)
				{
					newPos.y = yBorder;
					stopPlayer = true;
				}
			}
		}
	}

	if (!SDKGameRules()->IsIntermissionState())
	{
		float border = mp_field_border.GetInt();
		Vector min = SDKGameRules()->m_vFieldMin - border;
		Vector max = SDKGameRules()->m_vFieldMax + border;

		if (newPos.x < min.x || newPos.y < min.y || newPos.x > max.x || newPos.y > max.y)
		{
			if (newPos.x < min.x)
				newPos.x = min.x;
			else if (newPos.x > max.x)
				newPos.x = max.x;

			if (newPos.y < min.y)
				newPos.y = min.y;
			else if (newPos.y > max.y)
				newPos.y = max.y;

			stopPlayer = true;
		}
	}

	if (stopPlayer)
	{
		//newVel = oldVel;
		trace_t	trace;
		UTIL_TraceHull(newPos, newPos, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER, &trace);

		if (trace.startsolid)
		{
			newPos = oldPos;
		}

		newVel.x = (newPos - oldPos).x * 35;
		newVel.y = (newPos - oldPos).y * 35;
		//newPos = pos;
	}
}