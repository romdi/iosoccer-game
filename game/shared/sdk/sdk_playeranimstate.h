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

#if defined( CLIENT_DLL )
class C_SDKPlayer;
#define CSDKPlayer C_SDKPlayer
#else
class CSDKPlayer;
#endif

#define MOVING_MINIMUM_SPEED 0.5f

enum PlayerAnimEvent_t
{
	PLAYERANIMEVENT_NONE,
	PLAYERANIMEVENT_JUMP,
	PLAYERANIMEVENT_KICK_DRIBBLE,
	PLAYERANIMEVENT_KICK_WEAK,
	PLAYERANIMEVENT_KICK_STRONG,
	PLAYERANIMEVENT_VOLLEY,
	PLAYERANIMEVENT_HEEL_KICK,
	PLAYERANIMEVENT_HEADER_WEAK,
	PLAYERANIMEVENT_HEADER_STRONG,
	PLAYERANIMEVENT_THROW_IN_THROW,
	PLAYERANIMEVENT_KEEPER_JUMP,
	PLAYERANIMEVENT_KEEPER_DIVE_FORWARD,
	PLAYERANIMEVENT_KEEPER_DIVE_RIGHT_FORWARD,
	PLAYERANIMEVENT_KEEPER_DIVE_RIGHT,
	PLAYERANIMEVENT_KEEPER_DIVE_RIGHT_BACKWARD,
	PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD,
	PLAYERANIMEVENT_KEEPER_DIVE_LEFT_BACKWARD,
	PLAYERANIMEVENT_KEEPER_DIVE_LEFT,
	PLAYERANIMEVENT_KEEPER_DIVE_LEFT_FORWARD,
	PLAYERANIMEVENT_KEEPER_HANDS_THROW,
	PLAYERANIMEVENT_KEEPER_HANDS_VOLLEY,
	PLAYERANIMEVENT_KEEPER_HANDS_PUNCH,
	PLAYERANIMEVENT_SLIDE_TACKLE,
	PLAYERANIMEVENT_TACKLED_FORWARD,
	PLAYERANIMEVENT_TACKLED_BACKWARD,
	PLAYERANIMEVENT_DIVING_HEADER,
	PLAYERANIMEVENT_KEEPER_HANDS_CARRY,
	PLAYERANIMEVENT_THROW_IN_CARRY,
	PLAYERANIMEVENT_CARRY_END,
	PLAYERANIMEVENT_LIFT_UP,
	PLAYERANIMEVENT_BALL_ROLL_LEFT,
	PLAYERANIMEVENT_BALL_ROLL_RIGHT,
	PLAYERANIMEVENT_FAKE_SHOT,
	PLAYERANIMEVENT_RAINBOW_FLICK,
	PLAYERANIMEVENT_BICYCLE_KICK,
	PLAYERANIMEVENT_GESTURE_POINT,
	PLAYERANIMEVENT_GESTURE_WAVE,
	PLAYERANIMEVENT_CELEB_SLIDE,
	PLAYERANIMEVENT_COUNT
};


struct MultiPlayerPoseData_t
{
	int			m_iMoveX;
	int			m_iMoveY;
	int			m_iAimYaw;
	int			m_iAimPitch;
	int			m_iBodyHeight;

	float		m_flEstimateYaw;
	float		m_flLastAimTurnTime;

	void Init()
	{
		m_iMoveX = 0;
		m_iMoveY = 0;
		m_iAimYaw = 0;
		m_iAimPitch = 0;
		m_iBodyHeight = 0;
		m_flEstimateYaw = 0.0f;
		m_flLastAimTurnTime = 0.0f;
	}
};

struct DebugPlayerAnimData_t
{
	float		m_flSpeed;
	float		m_flAimPitch;
	float		m_flAimYaw;
	float		m_flBodyHeight;
	Vector2D	m_vecMoveYaw;

	void Init()
	{
		m_flSpeed = 0.0f;
		m_flAimPitch = 0.0f;
		m_flAimYaw = 0.0f;
		m_flBodyHeight = 0.0f;
		m_vecMoveYaw.Init();
	}
};

// ------------------------------------------------------------------------------------------------ //
// CPlayerAnimState declaration.
// ------------------------------------------------------------------------------------------------ //
class CSDKPlayerAnimState
{
public:
	
	DECLARE_CLASS_NOBASE(CSDKPlayerAnimState);

	CSDKPlayerAnimState();
	CSDKPlayerAnimState( CBasePlayer *pPlayer );
	~CSDKPlayerAnimState();

	CSDKPlayer *GetSDKPlayer( void )							{ return m_pSDKPlayer; }

	virtual void ClearAnimationState();
	virtual Activity TranslateActivity( Activity actDesired );
	virtual void Update( float eyeYaw, float eyePitch );
	virtual void Release( void ) { delete this; }
	void	DoAnimationEvent(PlayerAnimEvent_t event);
	CSDKPlayer *GetBasePlayer( void ) { return m_pSDKPlayer; }
	bool	HandleJumping( Activity &idealActivity );
	bool ShouldUpdateAnimState();
	float GetCurrentMaxGroundSpeed();
	virtual Activity CalcMainActivity();
	bool	m_bForceAimYaw;
	void ResetGroundSpeed( void );

	const QAngle& GetRenderAngles();
	void OnNewModel( void );

#ifdef CLIENT_DLL
	float m_flLastGroundSpeedUpdateTime;
	CInterpolatedVar<float> m_iv_flMaxGroundSpeed;
#endif

private:

	float m_flMaxGroundSpeed;
	CSDKPlayer   *m_pSDKPlayer;
	bool		m_bInAirWalk;
	float		m_flHoldDeployedPoseUntilTime;
	QAngle				m_angRender;
	bool						m_bPoseParameterInit;
	MultiPlayerPoseData_t		m_PoseParameterData;

	float m_flEyeYaw;
	float m_flEyePitch;
	float m_flGoalFeetYaw;
	float m_flCurrentFeetYaw;
	float m_flLastAimTurnTime;

	// Jumping.
	bool	m_bJumping;
	float	m_flJumpStartTime;	
	bool	m_bFirstJumpFrame;

	// Swimming.
	bool	m_bInSwim;
	bool	m_bFirstSwimFrame;

	// Dying
	bool	m_bDying;
	bool	m_bFirstDyingFrame;

	Activity m_eCurrentMainSequenceActivity;
	
	DebugPlayerAnimData_t		m_DebugAnimData;

	float CalcMovementPlaybackRate( bool *bIsMoving );
	void GetOuterAbsVelocity( Vector& vel );
	float GetOuterXYSpeed();
	void ComputeMainSequence();
	void UpdateInterpolators();
	void ComputeSequences( CStudioHdr *pStudioHdr );
	void UpdateLayerSequenceGeneric( CStudioHdr *pStudioHdr, int iLayer, bool &bEnabled, float &flCurCycle, int &iSequence, bool bWaitAtEnd );
	int CalcActionSequence(PlayerAnimEvent_t event);
	int CalcCarrySequence(PlayerAnimEvent_t event);
	int CalcGestureSequence(PlayerAnimEvent_t event);
	void ComputeActionSequence(CStudioHdr *pStudioHdr);
	void ComputeCarrySequence(CStudioHdr *pStudioHdr);
	void ComputeGestureSequence( CStudioHdr *pStudioHdr );
	int CalcSequenceIndex( const char *pBaseName, ... );
	void ClearAnimationLayers();
	virtual void RestartMainSequence();

	int m_nActionSequence;
	bool m_bIsActionSequenceActive;
	float m_flActionSequenceCycle;
	bool m_bActionSequenceWaitAtEnd;

	int m_nCarrySequence;
	float m_flCarrySequenceCycle;
	bool m_bIsCarrySequenceActive;

	int m_nGestureSequence;
	bool m_bIsGestureSequenceActive;
	float m_flGestureSequenceCycle;
	bool m_bGestureSequenceWaitAtEnd;

	bool				SetupPoseParameters( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr );
	void				ComputePoseParam_BodyHeight( CStudioHdr *pStudioHdr );
	virtual void		EstimateYaw( void );
	void				ConvergeYawAngles( float flGoalYaw, float flYawRate, float flDeltaTime, float &flCurrentYaw );

	virtual int SelectWeightedSequence( Activity activity );
};

CSDKPlayerAnimState *CreateSDKPlayerAnimState( CSDKPlayer *pPlayer );

#define ACTIONSEQUENCE_LAYER		0
#define CARRYSEQUENCE_LAYER			1
#define GESTURESEQUENCE_LAYER		2
#define NUM_LAYERS_WANTED			3

#define ANIM_TOPSPEED_WALK			150
#define ANIM_TOPSPEED_RUN			250			//ios - was 250 in sdk
#define ANIM_TOPSPEED_RUN_CROUCH	85
#define ANIM_TOPSPEED_SPRINT		350	//ios sprint - without this anim didnt work?!

#endif // SDK_PLAYERANIMSTATE_H
