//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "base_playeranimstate.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"

#include "sdk_playeranimstate.h"
#include "base_playeranimstate.h"
#include "datacache/imdlcache.h"

#ifdef CLIENT_DLL
#include "c_sdk_player.h"
#else
#include "sdk_player.h"
#endif

#define SDK_RUN_SPEED				320.0f
#define SDK_WALK_SPEED				75.0f
#define SDK_CROUCHWALK_SPEED		110.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : CMultiPlayerAnimState*
//-----------------------------------------------------------------------------
CSDKPlayerAnimState* CreateSDKPlayerAnimState( CSDKPlayer *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	// Setup the movement data.
	MultiPlayerMovementData_t movementData;
	movementData.m_flBodyYawRate = 720.0f;
	movementData.m_flRunSpeed = SDK_RUN_SPEED;
	movementData.m_flWalkSpeed = SDK_WALK_SPEED;
	movementData.m_flSprintSpeed = -1.0f;

	// Create animation state for this player.
	CSDKPlayerAnimState *pRet = new CSDKPlayerAnimState( pPlayer, movementData );

	// Specific SDK player initialization.
	pRet->InitSDKAnimState( pPlayer );

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CSDKPlayerAnimState::CSDKPlayerAnimState()
{
	m_pSDKPlayer = NULL;

	// Don't initialize SDK specific variables here. Init them in InitSDKAnimState()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&movementData - 
//-----------------------------------------------------------------------------
CSDKPlayerAnimState::CSDKPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData )
: CMultiPlayerAnimState( pPlayer, movementData )
{
	m_pSDKPlayer = NULL;

	// Don't initialize SDK specific variables here. Init them in InitSDKAnimState()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CSDKPlayerAnimState::~CSDKPlayerAnimState()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initialize Team Fortress specific animation state.
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::InitSDKAnimState( CSDKPlayer *pPlayer )
{
	m_pSDKPlayer = pPlayer;
#if defined ( SDK_USE_PRONE )
	m_iProneActivity = ACT_MP_STAND_TO_PRONE;
	m_bProneTransition = false;
	m_bProneTransitionFirstFrame = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::ClearAnimationState( void )
{
#if defined ( SDK_USE_PRONE )
	m_bProneTransition = false;
	m_bProneTransitionFirstFrame = false;
#endif
	m_bFiring = false;
	m_bReloading = false;
	ClearAnimationLayers();
	BaseClass::ClearAnimationState();
}

void CSDKPlayerAnimState::ClearAnimationLayers()
{
	if ( !GetBasePlayer() )
		return;

	GetBasePlayer()->SetNumAnimOverlays( NUM_LAYERS_WANTED );
	for ( int i=0; i < GetBasePlayer()->GetNumAnimOverlays(); i++ )
	{
		GetBasePlayer()->GetAnimOverlay( i )->SetOrder( CBaseAnimatingOverlay::MAX_OVERLAYS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actDesired - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CSDKPlayerAnimState::TranslateActivity( Activity actDesired )
{
	Activity translateActivity = BaseClass::TranslateActivity( actDesired );

	if ( GetSDKPlayer()->GetActiveWeapon() )
	{
		translateActivity = GetSDKPlayer()->GetActiveWeapon()->ActivityOverride( translateActivity, false );
	}

	return translateActivity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::Update( float eyeYaw, float eyePitch )
{
	// Profile the animation update.
	VPROF( "CMultiPlayerAnimState::Update" );

	// Clear animation overlays because we're about to completely reconstruct them.
	ClearAnimationLayers();

	// Some mods don't want to update the player's animation state if they're dead and ragdolled.
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	// Get the SDK player.
	CSDKPlayer *pSDKPlayer = GetSDKPlayer();
	if ( !pSDKPlayer )
		return;

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = pSDKPlayer->GetModelPtr();
	if ( !pStudioHdr )
		return;

	// Check to see if we should be updating the animation state - dead, ragdolled?
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	// Store the eye angles.
	m_flEyeYaw = AngleNormalize( eyeYaw );
	m_flEyePitch = AngleNormalize( eyePitch );

	// Compute the player sequences.
	ComputeSequences( pStudioHdr );

	if ( SetupPoseParameters( pStudioHdr ) )
	{
		// Pose parameter - what direction are the player's legs running in.
		ComputePoseParam_MoveYaw( pStudioHdr );

		// Pose parameter - Torso aiming (up/down).
		ComputePoseParam_AimPitch( pStudioHdr );

		// Pose parameter - Torso aiming (rotation).
		ComputePoseParam_AimYaw( pStudioHdr );
	}

#ifdef CLIENT_DLL 
	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
	{
		m_pSDKPlayer->SetPlaybackRate( 1.0f );
	}
#endif
}
extern ConVar mp_slammoveyaw;
float SnapYawTo( float flValue );
void CSDKPlayerAnimState::ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr )
{
	// Get the estimated movement yaw.
	EstimateYaw();

	// Get the view yaw.
	float flAngle = AngleNormalize( m_flEyeYaw );

	// Calc side to side turning - the view vs. movement yaw.
	float flYaw = flAngle - m_PoseParameterData.m_flEstimateYaw;
	flYaw = AngleNormalize( -flYaw );

	// Get the current speed the character is running.
	bool bIsMoving;
	float flPlaybackRate = CalcMovementPlaybackRate( &bIsMoving );

	// Setup the 9-way blend parameters based on our speed and direction.
	Vector2D vecCurrentMoveYaw( 0.0f, 0.0f );
	if ( bIsMoving )
	{
		if ( mp_slammoveyaw.GetBool() )
		{
			flYaw = SnapYawTo( flYaw );
		}
		vecCurrentMoveYaw.x = cos( DEG2RAD( flYaw ) ) * flPlaybackRate;
		vecCurrentMoveYaw.y = -sin( DEG2RAD( flYaw ) ) * flPlaybackRate;
	}

	// Set the 9-way blend movement pose parameters.
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, vecCurrentMoveYaw.x );
	//ios GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, -vecCurrentMoveYaw.y ); //Tony; flip it
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, vecCurrentMoveYaw.y );
	m_DebugAnimData.m_vecMoveYaw = vecCurrentMoveYaw;
}

void CSDKPlayerAnimState::ComputeFireSequence( CStudioHdr *pStudioHdr )
{
#ifdef CLIENT_DLL
	//if (m_iFireSequence > -1)
		UpdateLayerSequenceGeneric( pStudioHdr, FIRESEQUENCE_LAYER, m_bFiring, m_flFireCycle, m_iFireSequence, false );
#else
	// Server doesn't bother with different fire sequences.
#endif
}

void CSDKPlayerAnimState::ComputeReloadSequence( CStudioHdr *pStudioHdr )
{
#ifdef CLIENT_DLL

	//Keeper Carry layer
	//if (m_iReloadSequence > -1)
		UpdateLayerSequenceGeneric( pStudioHdr, RELOADSEQUENCE_LAYER, m_bReloading, m_flReloadCycle, m_iReloadSequence, m_bCarryHold);

	//Run Celeb layer
	//UpdateLayerSequenceGeneric( pStudioHdr, RELOADSEQUENCE_LAYER, m_bCeleb, m_flCelebCycle, m_iCelebSequence, m_bCelebHold);
#else
	// Server doesn't bother with different fire sequences.
#endif
}

int CSDKPlayerAnimState::CalcReloadLayerSequence()
{
	int iReloadSequence = GetBasePlayer()->LookupSequence( "CarryBall" );
	return iReloadSequence;
}

int CSDKPlayerAnimState::CalcFireLayerSequence(PlayerAnimEvent_t event)
{
	//ios anim events
	if (event == PLAYERANIMEVENT_KICK)
		return CalcSequenceIndex( "ioskick" );
	else if (event == PLAYERANIMEVENT_PASS)
		return CalcSequenceIndex( "iospass" );
	else if (event == PLAYERANIMEVENT_PASS_STATIONARY)
		return CalcSequenceIndex( "iospass_stationary" );
	else if (event == PLAYERANIMEVENT_VOLLEY)
		return CalcSequenceIndex( "iosvolley" );
	else if (event == PLAYERANIMEVENT_HEELKICK)
		return CalcSequenceIndex( "iosheelkick" );
	else if (event == PLAYERANIMEVENT_HEADER)
		return CalcSequenceIndex( "iosheader" );
	else if (event == PLAYERANIMEVENT_THROWIN)
		return CalcSequenceIndex( "iosthrowin" );
	else if (event == PLAYERANIMEVENT_THROW)
		return CalcSequenceIndex( "iosthrow" );
	else if (event == PLAYERANIMEVENT_SLIDE)
	{
		if (GetBasePlayer()->GetFlags() & FL_CELEB)
			return CalcSequenceIndex( "iosslideceleb" );		//override the normal anim if flag set
		else
			return CalcSequenceIndex( "iosslide" );
	}
	else if (event == PLAYERANIMEVENT_TACKLED_FORWARD)
		return CalcSequenceIndex( "iostackled_forward" );
	else if (event == PLAYERANIMEVENT_TACKLED_BACKWARD)
		return CalcSequenceIndex( "iostackled_backward" );
	else if (event == PLAYERANIMEVENT_DIVINGHEADER)
		return CalcSequenceIndex( "iosdivingheader" );
	
	return -1;
}


int CSDKPlayerAnimState::CalcSequenceIndex( const char *pBaseName, ... )
{
	char szFullName[512];
	va_list marker;
	va_start( marker, pBaseName );
	Q_vsnprintf( szFullName, sizeof( szFullName ), pBaseName, marker );
	va_end( marker );
	int iSequence = GetBasePlayer()->LookupSequence( szFullName );
	
	// Show warnings if we can't find anything here.
	if ( iSequence == -1 )
	{
		static CUtlDict<int,int> dict;
		if ( dict.Find( szFullName ) == -1 )
		{
			dict.Insert( szFullName, 0 );
			Warning( "CalcSequenceIndex: can't find '%s'.\n", szFullName );
		}

		iSequence = 0;
	}

	return iSequence;
}

#ifdef CLIENT_DLL
	void CSDKPlayerAnimState::UpdateLayerSequenceGeneric( CStudioHdr *pStudioHdr, int iLayer, bool &bEnabled, float &flCurCycle, int &iSequence, bool bWaitAtEnd )
	{
		if ( !bEnabled )
			return;

		// Increment the fire sequence's cycle.
		flCurCycle += GetBasePlayer()->GetSequenceCycleRate( pStudioHdr, iSequence ) * gpGlobals->frametime;
		if ( flCurCycle > 1 )
		{
			if ( bWaitAtEnd )
			{
				flCurCycle = 1;
			}
			else
			{
				// Not firing anymore.
				bEnabled = false;
				iSequence = 0;
				return;
			}
		}

		// Now dump the state into its animation layer.
		C_AnimationLayer *pLayer = GetBasePlayer()->GetAnimOverlay( iLayer );

		pLayer->m_flCycle = flCurCycle;
		pLayer->m_nSequence = iSequence;

		pLayer->m_flPlaybackRate = 1.0;
		pLayer->m_flWeight = 1.0f;
		pLayer->m_nOrder = iLayer;
	}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
//-----------------------------------------------------------------------------
void CSDKPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	Activity iGestureActivity = ACT_INVALID;

	switch( event )
	{
	case PLAYERANIMEVENT_CANCEL:
		{
			ClearAnimationState();
		}
		break;
	case PLAYERANIMEVENT_KICK:			//ios
	case PLAYERANIMEVENT_PASS:
	case PLAYERANIMEVENT_VOLLEY:
	case PLAYERANIMEVENT_HEELKICK:
	case PLAYERANIMEVENT_HEADER:
	case PLAYERANIMEVENT_THROWIN:
	case PLAYERANIMEVENT_THROW:
	case PLAYERANIMEVENT_SLIDE:
	case PLAYERANIMEVENT_TACKLED_FORWARD:
	case PLAYERANIMEVENT_TACKLED_BACKWARD:
	case PLAYERANIMEVENT_DIVINGHEADER:
	case PLAYERANIMEVENT_PASS_STATIONARY:
	{
		m_flFireCycle = 0;
		m_iFireSequence = CalcFireLayerSequence( event );
		m_bFiring = m_iFireSequence != -1;
		break;
	}
	case PLAYERANIMEVENT_JUMP:
	case PLAYERANIMEVENT_DIVE_LEFT:
	case PLAYERANIMEVENT_DIVE_RIGHT:
	{
		// Play the jump animation.
		if (!m_bJumping)
		{
			m_bJumping = true;
			m_bFirstJumpFrame = true;
			m_flJumpStartTime = gpGlobals->curtime;
			if (m_bCarryHold)									//dont dive when carrying
			{
				m_KeeperDive = 0;								//normal jump
			}
			else if (event == PLAYERANIMEVENT_DIVE_LEFT)
			{
				m_KeeperDive = PLAYERANIMEVENT_DIVE_LEFT;
				m_fDiveTime = gpGlobals->curtime + 0.8f;
			}
			else if (event == PLAYERANIMEVENT_DIVE_RIGHT)
			{
				m_KeeperDive = PLAYERANIMEVENT_DIVE_RIGHT;
				m_fDiveTime = gpGlobals->curtime + 0.8f;
			}
			else
			{
				if (GetBasePlayer()->GetFlags() & FL_CELEB)				//celeb cartwheel time
				{
					m_KeeperDive = 1;								//needs to be non-zero thats all
					m_fDiveTime = gpGlobals->curtime + 1.0f;
				}
				else
				{
					m_KeeperDive = 0;								//normal jump
				}
			}
		}
		break;
	}

	case PLAYERANIMEVENT_CARRY:
	{
		m_iReloadSequence = CalcReloadLayerSequence();			//add keeper carry as layer
		if ( m_iReloadSequence != -1 )
		{
			m_bReloading = true;
			m_flReloadCycle = 0;
			m_bCarryHold = true;
		}
		break;
	}
	case PLAYERANIMEVENT_CARRY_END:
	{
		m_iReloadSequence = CalcReloadLayerSequence();
		if ( m_iReloadSequence != -1 )
		{
			m_bReloading = true;
			m_flReloadCycle = 1.1f;
			m_bCarryHold = false;
		}
		break;
	}

			/* ios
	case PLAYERANIMEVENT_ATTACK_PRIMARY:
		{
			// Weapon primary fire.
#if defined ( SDK_USE_PRONE )
			if ( m_pSDKPlayer->m_Shared.IsProne() )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_PRONE_PRIMARYFIRE );
			}
			else
#endif //SDK_USE_PRONE
			if ( m_pSDKPlayer->GetFlags() & FL_DUCKING )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE );
			}
			else
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE );
			}

			iGestureActivity = ACT_VM_PRIMARYATTACK;
			break;
		}

	case PLAYERANIMEVENT_VOICE_COMMAND_GESTURE:
		{
			if ( !IsGestureSlotActive( GESTURE_SLOT_ATTACK_AND_RELOAD ) )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, (Activity)nData );
			}
			iGestureActivity = ACT_VM_IDLE; //TODO?
			break;
		}
	case PLAYERANIMEVENT_ATTACK_SECONDARY:
		{
			// Weapon secondary fire.
#if defined ( SDK_USE_PRONE )
			if ( m_pSDKPlayer->m_Shared.IsProne() )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_PRONE_SECONDARYFIRE );
			}
			else
#endif //SDK_USE_PRONE
			if ( m_pSDKPlayer->GetFlags() & FL_DUCKING )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_SECONDARYFIRE );
			}
			else
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_SECONDARYFIRE );
			}

			iGestureActivity = ACT_VM_PRIMARYATTACK;
			break;
		}
	case PLAYERANIMEVENT_ATTACK_PRE:
		{
			if ( m_pSDKPlayer->GetFlags() & FL_DUCKING ) 
			{
				// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc in crouch.
				iGestureActivity = ACT_MP_ATTACK_CROUCH_PREFIRE;
			}
			else
			{
				// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc.
				iGestureActivity = ACT_MP_ATTACK_STAND_PREFIRE;
			}

			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity, false );
			iGestureActivity = ACT_VM_IDLE; //TODO?

			break;
		}
	case PLAYERANIMEVENT_ATTACK_POST:
		{
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_POSTFIRE );
			iGestureActivity = ACT_VM_IDLE; //TODO?
			break;
		}

	case PLAYERANIMEVENT_RELOAD:
		{
			// Weapon reload.
#if defined ( SDK_USE_PRONE )
			if ( m_pSDKPlayer->m_Shared.IsProne() )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_PRONE );
			}
			else
#endif //SDK_USE_PRONE
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH );
			}
			else
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND );
			}
			iGestureActivity = ACT_VM_RELOAD; //Make view reload if it isn't already
			break;
		}
	case PLAYERANIMEVENT_RELOAD_LOOP:
		{
			// Weapon reload.
#if defined ( SDK_USE_PRONE )
			if ( m_pSDKPlayer->m_Shared.IsProne() )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_PRONE_LOOP );
			}
			else
#endif //SDK_USE_PRONE
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_LOOP );
			}
			else
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_LOOP );
			}
			iGestureActivity = ACT_INVALID; //TODO: fix
			break;
		}
	case PLAYERANIMEVENT_RELOAD_END:
		{
			// Weapon reload.
#if defined ( SDK_USE_PRONE )
			if ( m_pSDKPlayer->m_Shared.IsProne() )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_PRONE_END );
			}
			else
#endif //SDK_USE_PRONE
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_END );
			}
			else
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_END );
			}
			iGestureActivity = ACT_INVALID; //TODO: fix
			break;
		}
#if defined ( SDK_USE_PRONE )
	case PLAYERANIMEVENT_STAND_TO_PRONE:
		{
			m_bProneTransition = true;
			m_bProneTransitionFirstFrame = true;
			m_iProneActivity = ACT_MP_STAND_TO_PRONE;
			RestartMainSequence();
			iGestureActivity = ACT_VM_IDLE; //Clear for weapon, we have no stand->prone so just idle.
		}
		break;
	case PLAYERANIMEVENT_CROUCH_TO_PRONE:
		{
			m_bProneTransition = true;
			m_bProneTransitionFirstFrame = true;
			m_iProneActivity = ACT_MP_CROUCH_TO_PRONE;
			RestartMainSequence();
			iGestureActivity = ACT_VM_IDLE; //Clear for weapon, we have no crouch->prone so just idle.
		}
		break;
	case PLAYERANIMEVENT_PRONE_TO_STAND:
		{
			m_bProneTransition = true;
			m_bProneTransitionFirstFrame = true;
			m_iProneActivity = ACT_MP_PRONE_TO_STAND;
			RestartMainSequence();
			iGestureActivity = ACT_VM_IDLE; //Clear for weapon, we have no prone->stand so just idle.
		}
		break;
	case PLAYERANIMEVENT_PRONE_TO_CROUCH:
		{
			m_bProneTransition = true;
			m_bProneTransitionFirstFrame = true;
			m_iProneActivity = ACT_MP_PRONE_TO_CROUCH;
			RestartMainSequence();
			iGestureActivity = ACT_VM_IDLE; //Clear for weapon, we have no prone->crouch so just idle.
		}
		break;
#endif
		
	default:
		{
			BaseClass::DoAnimationEvent( event, nData );
			break;
		}
		*/
	}
	/*
#ifdef CLIENT_DLL
	// Make the weapon play the animation as well
	if ( iGestureActivity != ACT_INVALID && GetSDKPlayer() != CSDKPlayer::GetLocalSDKPlayer())
	{
		CBaseCombatWeapon *pWeapon = GetSDKPlayer()->GetActiveWeapon();
		if ( pWeapon )
		{
			pWeapon->EnsureCorrectRenderingModel();
			pWeapon->SendWeaponAnim( iGestureActivity );
			// Force animation events!
			pWeapon->ResetEventsParity();		// reset event parity so the animation events will occur on the weapon. 
			pWeapon->DoAnimationEvents( pWeapon->GetModelPtr() );
		}
	}
#endif
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
//-----------------------------------------------------------------------------
bool CSDKPlayerAnimState::HandleSwimming( Activity &idealActivity )
{
	bool bInWater = BaseClass::HandleSwimming( idealActivity );

	return bInWater;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSDKPlayerAnimState::HandleMoving( Activity &idealActivity )
{
	return BaseClass::HandleMoving( idealActivity );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSDKPlayerAnimState::HandleDucking( Activity &idealActivity )
{
	if ( m_pSDKPlayer->GetFlags() & FL_DUCKING )
	{
		if ( GetOuterXYSpeed() < MOVING_MINIMUM_SPEED )
		{
			idealActivity = ACT_MP_CROUCH_IDLE;		
		}
		else
		{
			idealActivity = ACT_MP_CROUCHWALK;		
		}

		return true;
	}
	
	return false;
}
#if defined ( SDK_USE_PRONE )
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSDKPlayerAnimState::HandleProne( Activity &idealActivity )
{
	if ( m_pSDKPlayer->m_Shared.IsProne() )
	{
		if ( GetOuterXYSpeed() < MOVING_MINIMUM_SPEED )
		{
			idealActivity = ACT_MP_PRONE_IDLE;		
		}
		else
		{
			idealActivity = ACT_MP_PRONE_CRAWL;		
		}

		return true;
	}
	
	return false;
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSDKPlayerAnimState::HandleProneTransition( Activity &idealActivity )
{
	if ( m_bProneTransition )
	{
		if (m_bProneTransitionFirstFrame)
		{
			m_bProneTransitionFirstFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		//Tony; check the cycle, and then stop overriding
		if ( GetBasePlayer()->GetCycle() >= 0.99 )
			m_bProneTransition = false;
		else
			idealActivity = m_iProneActivity;
	}

	return m_bProneTransition;
}
#endif // SDK_USE_PRONE

#if defined ( SDK_USE_SPRINTING )
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSDKPlayerAnimState::HandleSprinting( Activity &idealActivity )
{
	if ( m_pSDKPlayer->m_Shared.IsSprinting() )
	{
		idealActivity = ACT_SPRINT;		

		return true;
	}
	
	return false;
}
#endif // SDK_USE_SPRINTING


bool CSDKPlayerAnimState::HandleJumping( Activity &idealActivity )
{
	if ( m_bJumping )
	{
		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
		{
			//normal jump - stop whe hits ground
			if (!m_KeeperDive)
			{
				if ( GetBasePlayer()->GetFlags() & FL_ONGROUND )
				{
					m_bJumping = false;
					RestartMainSequence();	// Reset the animation.
				}
			}
			else
			{
				//diving, wait for anim end
				if (gpGlobals->curtime > m_fDiveTime)
				{
					m_bJumping = false;
					RestartMainSequence();	// Reset the animation.
				}
			}
		}
	}
	//hacks - no event - this is crap - markg - ios - why cant they release an sdk where jump works...
	//else if (!(m_pOuter->GetFlags() & FL_ONGROUND) && m_pOuter->GetAbsVelocity().z > 0.0f)
	//{
	//	m_bJumping = true;
	//	m_bFirstJumpFrame = true;
	//	m_flJumpStartTime = gpGlobals->curtime;
	//}


	// Are we still jumping? If so, keep playing the jump animation.
	return m_bJumping;
}


/* ios original sdk
//-----------------------------------------------------------------------------
// Purpose: 
bool CSDKPlayerAnimState::HandleJumping( Activity &idealActivity )
{
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	if ( m_bJumping )
	{
		static bool bNewJump = false; //Tony; the sample dod player models that I'm using don't have the jump anims split up like tf2.

		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Reset if we hit water and start swimming.
		if ( m_pSDKPlayer->GetWaterLevel() >= WL_Waist )
		{
			m_bJumping = false;
			RestartMainSequence();
		}
		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		else if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
		{
			if ( m_pSDKPlayer->GetFlags() & FL_ONGROUND )
			{
				m_bJumping = false;
				RestartMainSequence();

				if ( bNewJump )
				{
					RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );					
				}
			}
		}

		// if we're still jumping
		if ( m_bJumping )
		{
			if ( bNewJump )
			{
				if ( gpGlobals->curtime - m_flJumpStartTime > 0.5 )
				{
					idealActivity = ACT_MP_JUMP_FLOAT;
				}
				else
				{
					idealActivity = ACT_MP_JUMP_START;
				}
			}
			else
			{
				idealActivity = ACT_MP_JUMP;
			}
		}
	}	

	if ( m_bJumping )
		return true;

	return false;
}
*/
//-----------------------------------------------------------------------------
// Purpose: Overriding CMultiplayerAnimState to add prone and sprinting checks as necessary.
// Input  :  - 
// Output : Activity
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
extern ConVar anim_showmainactivity;
#endif
/*
Activity CSDKPlayerAnimState::CalcMainActivity()
{
	Activity idealActivity = ACT_MP_STAND_IDLE;

	if ( HandleJumping( idealActivity ) || 
#if defined ( SDK_USE_PRONE )
		//Tony; handle these before ducking !!
		HandleProneTransition( idealActivity ) ||
		HandleProne( idealActivity ) ||
#endif
		HandleDucking( idealActivity ) || 
		HandleSwimming( idealActivity ) || 
		HandleDying( idealActivity ) 
#if defined ( SDK_USE_SPRINTING )
		|| HandleSprinting( idealActivity )
#endif
		)
	{
		// intentionally blank
	}
	else
	{
		HandleMoving( idealActivity );
	}

	ShowDebugInfo();

	// Client specific.
#ifdef CLIENT_DLL

	if ( anim_showmainactivity.GetBool() )
	{
		DebugShowActivity( idealActivity );
	}

#endif

	return idealActivity;
}
*/

Activity CSDKPlayerAnimState::CalcMainActivity()
{
	//return BaseClass::CalcMainActivity();
	
	float flOuterSpeed = GetOuterXYSpeed();

	CSDKPlayer	*pPlayer = dynamic_cast<CSDKPlayer*>(GetBasePlayer());

	Activity idealActivity = ACT_IDLE;

	if ( HandleJumping(idealActivity) )
	{
		if (pPlayer->GetFlags() & FL_CELEB)
			return ACT_IOS_JUMPCELEB;							//cartwheel celeb
		else if (m_KeeperDive == PLAYERANIMEVENT_DIVE_LEFT)
			return ACT_ROLL_RIGHT;								//wrong way round because animated facing keeper
		else if (m_KeeperDive == PLAYERANIMEVENT_DIVE_RIGHT)
			return ACT_ROLL_LEFT;
		else if (pPlayer->m_nBody > 0)
			return ACT_LEAP;									//keepers jump
		else
			return ACT_HOP;										//normal jump
	}
	else
	{
		if ( pPlayer->GetFlags() & FL_DUCKING )
		{
			idealActivity = ACT_CROUCH;					//ios slidetackle
			//if ( flOuterSpeed > MOVING_MINIMUM_SPEED )
			//	idealActivity = ACT_RUN_CROUCH;
			//else
			//	idealActivity = ACT_CROUCHIDLE;
		}
		//else if ( m_pOuter->GetFlags() & FL_CARRY)			//ios - uses reload layer now
		//{
		//	idealActivity = ACT_IOS_CARRY;
		//}
		else
		{
			if ( flOuterSpeed > MOVING_MINIMUM_SPEED )
			{
				if ( flOuterSpeed > 350.0f )
				{
					idealActivity = ACT_SPRINT;
				}
				else if ( flOuterSpeed > ARBITRARY_RUN_SPEED )
				{
					if (pPlayer->GetFlags() & FL_CELEB)		//now on layer
						idealActivity = ACT_IOS_RUNCELEB;
					else
						idealActivity = ACT_RUN;
				}
				else
				{
					idealActivity = ACT_WALK;
				}
			}
			else
			{
				idealActivity = ACT_IDLE;
			}
		}

		return idealActivity;
	}

}

void CSDKPlayerAnimState::ComputeSequences( CStudioHdr *pStudioHdr )
{
	VPROF( "CBasePlayerAnimState::ComputeSequences" );

	// Lower body (walk/run/idle).
	ComputeMainSequence();

	// The groundspeed interpolator uses the main sequence info.
	UpdateInterpolators();		
	//ios ComputeGestureSequence( pStudioHdr );
	ComputeFireSequence(pStudioHdr);
	ComputeReloadSequence( pStudioHdr );
}

/*
void CSDKPlayerAnimState::ComputeIosSequence(CStudioHdr *pStudioHdr)
{
	#ifdef CLIENT_DLL
	//void CSDKPlayerAnimState::UpdateLayerSequenceGeneric( CStudioHdr *pStudioHdr, int iLayer, bool &bEnabled, float &flCurCycle, int &iSequence, bool bWaitAtEnd )
	//{
		if ( !bEnabled )
			return;

		// Increment the fire sequence's cycle.
		flCurCycle += m_pOuter->GetSequenceCycleRate( pStudioHdr, iSequence ) * gpGlobals->frametime;
		if ( flCurCycle > 1 )
		{
			if ( bWaitAtEnd )
			{
				flCurCycle = 1;
			}
			else
			{
				// Not firing anymore.
				bEnabled = false;
				iSequence = 0;
				return;
			}
		}

		// Now dump the state into its animation layer.
		C_AnimationLayer *pLayer = getbaseplayer->GetAnimOverlay( iLayer );

		pLayer->m_flCycle = flCurCycle;
		pLayer->m_nSequence = iSequence;

		pLayer->m_flPlaybackRate = 1.0;
		pLayer->m_flWeight = 1.0f;
		pLayer->m_nOrder = iLayer;
	//}
#endif
}
*/

float CSDKPlayerAnimState::GetCurrentMaxGroundSpeed()
{
	Activity currentActivity = 	GetBasePlayer()->GetSequenceActivity( GetBasePlayer()->GetSequence() );
	if ( currentActivity == ACT_WALK || currentActivity == ACT_IDLE )
		return ANIM_TOPSPEED_WALK;
	else if ( currentActivity == ACT_RUN )
		return ANIM_TOPSPEED_RUN;
	else if ( currentActivity == ACT_SPRINT )			//IOS
		return ANIM_TOPSPEED_SPRINT;
	else if ( currentActivity == ACT_IOS_CARRY )		//IOS
		return ANIM_TOPSPEED_RUN;
	else if ( currentActivity == ACT_IOS_RUNCELEB )		//IOS
		return ANIM_TOPSPEED_RUN;
	else if ( currentActivity == ACT_RUN_CROUCH )
		return ANIM_TOPSPEED_RUN_CROUCH;
	else
		return 0;
}