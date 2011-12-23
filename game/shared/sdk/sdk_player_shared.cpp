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

bool CSDKPlayer::CanMove( void ) const
{
	bool bValidMoveState = (State_Get() == STATE_ACTIVE || State_Get() == STATE_OBSERVER_MODE);
			
	if ( !bValidMoveState )
	{
		return false;
	}

	return true;
}

// BUG! This is not called on the client at respawn, only first spawn!
void CSDKPlayer::SharedSpawn()
{	
	BaseClass::SharedSpawn();

	// Reset the animation state or we will animate to standing
	// when we spawn

	m_Shared.SetJumping( false );

	//Tony; todo; fix

//	m_flMinNextStepSoundTime = gpGlobals->curtime;
#if defined ( SDK_USE_PRONE )
//	m_bPlayingProneMoveSound = false;
#endif // SDK_USE_PRONE
}
#if defined ( SDK_USE_SPRINTING )
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
#endif // SDK_USE_SPRINTING


//-----------------------------------------------------------------------------
// Purpose: Returns the players mins - overridden for prone
// Input  : 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector CSDKPlayer::GetPlayerMins( void ) const
{
	if ( IsObserver() )
	{
		return VEC_OBS_HULL_MIN;	
	}
	else
	{
		if ( GetFlags() & FL_DUCKING )
		{
			return VEC_DUCK_HULL_MIN;
		}
#if defined ( SDK_USE_PRONE )
		else if ( m_Shared.IsProne() )
		{
			return VEC_PRONE_HULL_MIN;
		}
#endif // SDK_USE_PRONE
		else
		{
			return VEC_HULL_MIN;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:  Returns the players Maxs - overridden for prone
// Input  : 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector CSDKPlayer::GetPlayerMaxs( void ) const
{	
	if ( IsObserver() )
	{
		return VEC_OBS_HULL_MAX;	
	}
	else
	{
		if ( GetFlags() & FL_DUCKING )
		{
			return VEC_DUCK_HULL_MAX;
		}
#if defined ( SDK_USE_PRONE )
		else if ( m_Shared.IsProne() )
		{
			return VEC_PRONE_HULL_MAX;
		}
#endif // SDK_USE_PRONE
		else
		{
			return VEC_HULL_MAX;
		}
	}
}


// --------------------------------------------------------------------------------------------------- //
// CSDKPlayerShared implementation.
// --------------------------------------------------------------------------------------------------- //
CSDKPlayerShared::CSDKPlayerShared()
{
#if defined( SDK_USE_PRONE )
	m_bProne = false;
	m_bForceProneChange = false;
	m_flNextProneCheck = 0;
	m_flUnProneTime = 0;
	m_flGoProneTime = 0;
#endif

#if defined ( SDK_USE_PLAYERCLASSES )
	SetDesiredPlayerClass( PLAYERCLASS_UNDEFINED );
#endif
}

CSDKPlayerShared::~CSDKPlayerShared()
{
}

void CSDKPlayerShared::Init( CSDKPlayer *pPlayer )
{
	m_pOuter = pPlayer;
}

bool CSDKPlayerShared::IsDucking( void ) const
{
	return ( m_pOuter->GetFlags() & FL_DUCKING ) ? true : false;
}

#if defined ( SDK_USE_PRONE )
bool CSDKPlayerShared::IsProne() const
{
	return m_bProne;
}

bool CSDKPlayerShared::IsGettingUpFromProne() const
{
	return ( m_flUnProneTime > 0 );
}

bool CSDKPlayerShared::IsGoingProne() const
{
	return ( m_flGoProneTime > 0 );
}
void CSDKPlayerShared::SetProne( bool bProne, bool bNoAnimation /* = false */ )
{
	m_bProne = bProne;

	if ( bNoAnimation )
	{
		m_flGoProneTime = 0;
		m_flUnProneTime = 0;

		// cancel the view animation!
		m_bForceProneChange = true;
	}

	if ( !bProne /*&& IsSniperZoomed()*/ )	// forceunzoom for going prone is in StartGoingProne
	{
		ForceUnzoom();
	}
}
void CSDKPlayerShared::StartGoingProne( void )
{
	// make the prone sound
	CPASFilter filter( m_pOuter->GetAbsOrigin() );
	filter.UsePredictionRules();
	m_pOuter->EmitSound( filter, m_pOuter->entindex(), "Player.GoProne" );

	// slow to prone speed
	m_flGoProneTime = gpGlobals->curtime + TIME_TO_PRONE;

	m_flUnProneTime = 0.0f;	//reset

	if ( IsSniperZoomed() )
		ForceUnzoom();
}

void CSDKPlayerShared::StandUpFromProne( void )
{	
	// make the prone sound
	CPASFilter filter( m_pOuter->GetAbsOrigin() );
	filter.UsePredictionRules();
	m_pOuter->EmitSound( filter, m_pOuter->entindex(), "Player.UnProne" );

	// speed up to target speed
	m_flUnProneTime = gpGlobals->curtime + TIME_TO_PRONE;

	m_flGoProneTime = 0.0f;	//reset 
}

bool CSDKPlayerShared::CanChangePosition( void )
{
	if ( IsGettingUpFromProne() )
		return false;

	if ( IsGoingProne() )
		return false;

	return true;
}

#endif

#if defined ( SDK_USE_SPRINTING )
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
#endif

void CSDKPlayerShared::SetJumping( bool bJumping )
{
	m_bJumping = bJumping;
	
	if ( IsSniperZoomed() )
	{
		ForceUnzoom();
	}
}

void CSDKPlayerShared::ForceUnzoom( void )
{
//	CWeaponSDKBase *pWeapon = GetActiveSDKWeapon();
//	if( pWeapon && ( pWeapon->GetSDKWpnData().m_WeaponType & WPN_MASK_GUN ) )
//	{
//		CSDKSniperWeapon *pSniper = dynamic_cast<CSDKSniperWeapon *>(pWeapon);
//
//		if ( pSniper )
//		{
//			pSniper->ZoomOut();
//		}
//	}
}

bool CSDKPlayerShared::IsSniperZoomed( void ) const
{
//	CWeaponSDKBase *pWeapon = GetActiveSDKWeapon();
//	if( pWeapon && ( pWeapon->GetSDKWpnData().m_WeaponType & WPN_MASK_GUN ) )
//	{
//		CWeaponSDKBaseGun *pGun = (CWeaponSDKBaseGun *)pWeapon;
//		Assert( pGun );
//		return pGun->IsSniperZoomed();
//	}

	return false;
}

#if defined ( SDK_USE_PLAYERCLASSES )
void CSDKPlayerShared::SetDesiredPlayerClass( int playerclass )
{
	m_iDesiredPlayerClass = playerclass;
}

int CSDKPlayerShared::DesiredPlayerClass( void )
{
	return m_iDesiredPlayerClass;
}

void CSDKPlayerShared::SetPlayerClass( int playerclass )
{
	m_iPlayerClass = playerclass;
}

int CSDKPlayerShared::PlayerClass( void )
{
	return m_iPlayerClass;
}
#endif

#if defined ( SDK_USE_STAMINA ) || defined ( SDK_USE_SPRINTING )
void CSDKPlayerShared::SetStamina( float flStamina )
{
	m_flStamina = clamp( flStamina, 0, 100 );
}
#endif
CWeaponSDKBase* CSDKPlayerShared::GetActiveSDKWeapon() const
{
	CBaseCombatWeapon *pWeapon = m_pOuter->GetActiveWeapon();
	if ( pWeapon )
	{
		Assert( dynamic_cast< CWeaponSDKBase* >( pWeapon ) == static_cast< CWeaponSDKBase* >( pWeapon ) );
		return static_cast< CWeaponSDKBase* >( pWeapon );
	}
	else
	{
		return NULL;
	}
}

void CSDKPlayerShared::ComputeWorldSpaceSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	Vector org = m_pOuter->GetAbsOrigin();

#ifdef SDK_USE_PRONE
	if ( IsProne() )
	{
		static Vector vecProneMin(-44, -44, 0 );
		static Vector vecProneMax(44, 44, 24 );

		VectorAdd( vecProneMin, org, *pVecWorldMins );
		VectorAdd( vecProneMax, org, *pVecWorldMaxs );
	}
	else
#endif
	{
		static Vector vecMin(-32, -32, 0 );
		static Vector vecMax(32, 32, 72 );

		VectorAdd( vecMin, org, *pVecWorldMins );
		VectorAdd( vecMax, org, *pVecWorldMaxs );
	}
}

void CSDKPlayer::InitSpeeds()
{
#if !defined ( SDK_USE_PLAYERCLASSES )
	m_Shared.m_flRunSpeed = SDK_DEFAULT_PLAYER_RUNSPEED;
	m_Shared.m_flSprintSpeed = SDK_DEFAULT_PLAYER_SPRINTSPEED;
	m_Shared.m_flProneSpeed = SDK_DEFAULT_PLAYER_PRONESPEED;
	// Set the absolute max to sprint speed
	//ios SetMaxSpeed( m_Shared.m_flSprintSpeed ); 
	SetMaxSpeed( m_Shared.m_flRunSpeed ); 
	return;
#endif
#if defined ( SDK_USE_PLAYERCLASSES )
		int playerclass = m_Shared.PlayerClass();

		//Tony; error checkings.
		if ( playerclass == PLAYERCLASS_UNDEFINED )
		{
			m_Shared.m_flRunSpeed = SDK_DEFAULT_PLAYER_RUNSPEED;
			m_Shared.m_flSprintSpeed = SDK_DEFAULT_PLAYER_SPRINTSPEED;
			m_Shared.m_flProneSpeed = SDK_DEFAULT_PLAYER_PRONESPEED;
		}
		else
		{
			CSDKTeam *pTeam = GetGlobalSDKTeam( GetTeamNumber() );
			const CSDKPlayerClassInfo &pClassInfo = pTeam->GetPlayerClassInfo( playerclass );

			Assert( pClassInfo.m_iTeam == GetTeamNumber() );

			m_Shared.m_flRunSpeed = pClassInfo.m_flRunSpeed;
			m_Shared.m_flSprintSpeed = pClassInfo.m_flSprintSpeed;
			m_Shared.m_flProneSpeed = pClassInfo.m_flProneSpeed;
		}

		// Set the absolute max to sprint speed
		SetMaxSpeed( m_Shared.m_flSprintSpeed ); 
#endif // SDK_USE_PLAYERCLASSES
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSDKPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	// Only check these when using teams, otherwise it's normal!
#if defined ( SDK_USE_TEAMS )
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT || collisionGroup == COLLISION_GROUP_PROJECTILE )
	{
		switch( GetTeamNumber() )
		{
		case SDK_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_TEAM2 ) )
				return false;
			break;

		case SDK_TEAM_RED:
			if ( !( contentsMask & CONTENTS_TEAM1 ) )
				return false;
			break;
		}
	}
#endif
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

