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
	#include "c_sdk_team.h"
	#include "prediction.h"
	#include "clientmode_sdk.h"
	#include "vgui_controls/AnimationController.h"

	#define CRecipientFilter C_RecipientFilter
#else
	#include "sdk_player.h"
	#include "sdk_team.h"
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

void CSDKPlayer::CheckBallShield(const Vector &oldPos, Vector &newPos, const Vector &oldVel, Vector &newVel, const QAngle &oldAng, QAngle &newAng)
{
	bool stopPlayer = false;
	Vector pos = newPos;
	Vector walkDir = vec3_origin;
	Vector targetPos = vec3_origin;

	if (SDKGameRules()->m_nShieldType != SHIELD_NONE)
	{
		float threshold = 2 * (GetFlags() & FL_SHIELD_KEEP_IN ? -VEC_HULL_MAX.x : VEC_HULL_MAX.x);

		if (SDKGameRules()->m_nShieldType == SHIELD_GOALKICK || 
			SDKGameRules()->m_nShieldType == SHIELD_PENALTY)
		{
			Vector min = GetGlobalTeam(SDKGameRules()->m_nShieldDir)->m_vPenBoxMin;
			Vector max = GetGlobalTeam(SDKGameRules()->m_nShieldDir)->m_vPenBoxMax;

			min.x -= threshold;
			min.y -= threshold;
			max.x += threshold;
			max.y += threshold;

			if (GetFlags() & FL_SHIELD_KEEP_OUT || SDKGameRules()->m_nShieldType == SHIELD_PENALTY)
			{
				if (SDKGameRules()->m_vKickOff.GetY() > min.y)
					min.y -= 200;
				else
					max.y += 200;
			}

			bool isInsideBox = pos.x > min.x && pos.y > min.y && pos.x < max.x && pos.y < max.y; 
			Vector boxCenter = (min + max) / 2;

			if (GetFlags() & FL_SHIELD_KEEP_OUT && isInsideBox)
			{
				if (pos.x > min.x && oldPos.x <= min.x && pos.x < boxCenter.x)
					pos.x = min.x;
				else if (pos.x < max.x && oldPos.x >= max.x && pos.x > boxCenter.x)
					pos.x = max.x;

				if (pos.y > min.y && oldPos.y <= min.y && pos.y < boxCenter.y)
					pos.y = min.y;
				else if (pos.y < max.y && oldPos.y >= max.y && pos.y > boxCenter.y)
					pos.y = max.y;

				stopPlayer = true;
			}
			else if (GetFlags() & FL_SHIELD_KEEP_IN && !isInsideBox)
			{
				if (pos.x < min.x)
					pos.x = min.x;
				else if (pos.x > max.x)
					pos.x = max.x;

				if (pos.y < min.y)
					pos.y = min.y;
				else if (pos.y > max.y)
					pos.y = max.y;

				stopPlayer = true;
			}

			if ((GetFlags() & FL_REMOTECONTROLLED) && m_vTargetPos == vec3_invalid)
			{
				m_vTargetPos = Vector(oldPos.x, oldPos.y, SDKGameRules()->m_vKickOff.GetZ());
				m_vTargetPos.SetY(GetGlobalTeam(SDKGameRules()->m_nShieldDir)->m_nForward == 1 ? max.y : min.y);
			}
		}

		if (SDKGameRules()->m_nShieldType == SHIELD_THROWIN || 
			SDKGameRules()->m_nShieldType == SHIELD_FREEKICK || 
			SDKGameRules()->m_nShieldType == SHIELD_CORNER ||  
			SDKGameRules()->m_nShieldType == SHIELD_KICKOFF ||
			SDKGameRules()->m_nShieldType == SHIELD_PENALTY && GetFlags() & FL_SHIELD_KEEP_OUT)
		{
			float radius = SDKGameRules()->GetShieldRadius() + threshold;
			Vector dir = pos - SDKGameRules()->m_vShieldPos;

			if (GetFlags() & FL_SHIELD_KEEP_OUT && dir.Length2D() < radius || GetFlags() & FL_SHIELD_KEEP_IN && dir.Length2D() > radius)
			{
				dir.z = 0;
				dir.NormalizeInPlace();
				pos = SDKGameRules()->m_vShieldPos + dir * radius;
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
				float yBorder = SDKGameRules()->m_vKickOff.GetY() - abs(threshold) * forward;
				if (Sign(pos.y - yBorder) == forward)
				{
					pos.y = yBorder;
					stopPlayer = true;
				}
			}

			if ((GetFlags() & FL_REMOTECONTROLLED) && m_vTargetPos == vec3_invalid)
			{
				Vector moveDir;
				if (SDKGameRules()->m_nShieldType == SHIELD_PENALTY)
					moveDir = Vector(0, GetGlobalTeam(SDKGameRules()->m_nShieldDir)->m_nForward, 0);
				else
				{
					moveDir = GetGlobalTeam(SDKGameRules()->m_nShieldDir)->m_vPenalty - oldPos;
					moveDir.z = 0;
					moveDir.NormalizeInPlace();
				}

				m_vTargetPos = SDKGameRules()->m_vShieldPos + moveDir * radius;

				//Vector shieldDir = oldPos - SDKGameRules()->m_vShieldPos;
				//if (shieldDir.Length2D() == 0)
				//	shieldDir = SDKGameRules()->m_vKickOff - oldPos;
				//shieldDir.z = 0;
				//shieldDir.NormalizeInPlace();
				//m_vTargetPos = SDKGameRules()->m_vShieldPos + shieldDir * radius;
			}
		}
	}

	if (!SDKGameRules()->IsIntermissionState())
	{
		float threshold = 150;
		Vector min = SDKGameRules()->m_vFieldMin - threshold;
		Vector max = SDKGameRules()->m_vFieldMax + threshold;

		if (pos.x < min.x || pos.y < min.y || pos.x > max.x || pos.y > max.y)
		{
			if (pos.x < min.x)
				pos.x = min.x;
			else if (pos.x > max.x)
				pos.x = max.x;

			if (pos.y < min.y)
				pos.y = min.y;
			else if (pos.y > max.y)
				pos.y = max.y;

			stopPlayer = true;
		}
	}

	if (GetFlags() & FL_REMOTECONTROLLED)
	{
		if (oldPos == m_vTargetPos || !m_bMoveToExactPos && !stopPlayer)
		{
			m_bIsAtTargetPos = true;
			RemoveFlag(FL_REMOTECONTROLLED);

			if (m_bHoldAtTargetPos)
				AddFlag(FL_ATCONTROLS);

			newVel = vec3_origin;
			newPos = oldPos;
		}
		else
		{
			Vector dir = m_vTargetPos - oldPos;
			dir.z = 0;
			float distToTarget = dir.Length2D();
			dir.NormalizeInPlace();

			VectorAngles(dir, newAng);
			//mv->m_vecAbsViewAngles = mv->m_vecViewAngles = mv->m_vecAngles;
			//pPl->SnapEyeAngles(mv->m_vecAngles);
			//mv->m_flForwardMove = mp_runspeed.GetInt();
	
			float wishDist = mp_sprintspeed.GetInt() * gpGlobals->frametime;
			newVel = dir * mp_sprintspeed.GetInt();

			if (wishDist < distToTarget)
				newPos = oldPos + newVel * gpGlobals->frametime;
			else
				newPos = m_vTargetPos;

			newPos.z = SDKGameRules()->m_vKickOff.GetZ();
		}
	}
	else
	{
		if (stopPlayer)
		{
			newVel = oldVel;
			newVel.x = (pos - oldPos).x * 35;
			newVel.y = (pos - oldPos).y * 35;
			newPos = pos;
		}
	}
}