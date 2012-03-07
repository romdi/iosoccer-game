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