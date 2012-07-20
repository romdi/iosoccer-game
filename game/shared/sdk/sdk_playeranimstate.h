//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SDK_PLAYERANIMSTATE_H
#define SDK_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif


#include "convar.h"
#include "multiplayer_animstate.h"

#if defined( CLIENT_DLL )
class C_SDKPlayer;
#define CSDKPlayer C_SDKPlayer
#else
class CSDKPlayer;
#endif

#define MOVING_MINIMUM_SPEED 0.5f
#define ARBITRARY_RUN_SPEED		175.0f

// ------------------------------------------------------------------------------------------------ //
// CPlayerAnimState declaration.
// ------------------------------------------------------------------------------------------------ //
class CSDKPlayerAnimState : public CMultiPlayerAnimState
{
public:
	
	DECLARE_CLASS( CSDKPlayerAnimState, CMultiPlayerAnimState );

	CSDKPlayerAnimState();
	CSDKPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData );
	~CSDKPlayerAnimState();

	void InitSDKAnimState( CSDKPlayer *pPlayer );
	CSDKPlayer *GetSDKPlayer( void )							{ return m_pSDKPlayer; }

	virtual void ClearAnimationState();
	virtual Activity TranslateActivity( Activity actDesired );
	virtual void Update( float eyeYaw, float eyePitch );

	void	DoAnimationEvent(PlayerAnimEvent_t event);

	bool	HandleJumping( Activity &idealActivity );

	float GetCurrentMaxGroundSpeed();

	//Tony; overriding because the SDK Player models pose parameter is flipped the opposite direction
	virtual void		ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr );

	virtual Activity CalcMainActivity();	

private:
	
	CSDKPlayer   *m_pSDKPlayer;
	bool		m_bInAirWalk;
#if defined ( SDK_USE_PRONE )
	Activity	m_iProneActivity;
	bool		m_bProneTransition;
	bool		m_bProneTransitionFirstFrame;
#endif

	float		m_flHoldDeployedPoseUntilTime;

	void ComputeSequences( CStudioHdr *pStudioHdr );
	void ComputeIosSequence(CStudioHdr *pStudioHdr);
	void UpdateLayerSequenceGeneric( CStudioHdr *pStudioHdr, int iLayer, bool &bEnabled, float &flCurCycle, int &iSequence, bool bWaitAtEnd );
	int CalcPrimaryActionSequence(PlayerAnimEvent_t event);
	void ComputePrimaryActionSequence(CStudioHdr *pStudioHdr);
	void ComputeSecondaryActionSequence( CStudioHdr *pStudioHdr );
	int CalcSecondaryActionSequence();
	int CalcSequenceIndex( const char *pBaseName, ... );
	void ClearAnimationLayers();

	bool m_bIsPrimaryActionSequenceActive;						// If this is on, then it'll continue the fire animation in the fire layer
										// until it completes.
	int m_iPrimaryActionSequence;				// (For any sequences in the fire layer, including grenade throw).
	float m_flPrimaryActionSequenceCycle;
	bool m_bIsSecondaryActionSequenceActive;
	float m_flSecondaryActionSequenceCycle;
	int m_iSecondaryActionSequence;
};

CSDKPlayerAnimState *CreateSDKPlayerAnimState( CSDKPlayer *pPlayer );

#define PRIMARYACTIONSEQUENCE_LAYER		0
#define SECONDARYACTIONSEQUENCE_LAYER	(PRIMARYACTIONSEQUENCE_LAYER + 1)
#define NUM_LAYERS_WANTED		(SECONDARYACTIONSEQUENCE_LAYER + 1)

#define ANIM_TOPSPEED_WALK			150
#define ANIM_TOPSPEED_RUN			250			//ios - was 250 in sdk
#define ANIM_TOPSPEED_RUN_CROUCH	85
#define ANIM_TOPSPEED_SPRINT		350	//ios sprint - without this anim didnt work?!

#endif // SDK_PLAYERANIMSTATE_H
