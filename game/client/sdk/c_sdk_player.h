//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_SDK_PLAYER_H
#define C_SDK_PLAYER_H
#ifdef _WIN32
#pragma once
#endif


#include "sdk_playeranimstate.h"
#include "c_baseplayer.h"
#include "baseparticleentity.h"
#include "sdk_player_shared.h"


class C_SDKPlayer : public C_BasePlayer
{
public:
	DECLARE_CLASS( C_SDKPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_SDKPlayer();
	~C_SDKPlayer();

	static C_SDKPlayer* GetLocalSDKPlayer();

	virtual const QAngle& GetRenderAngles();
	virtual void Spawn();
	virtual void UpdateClientSideAnimation();
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );

	void LookAtBall();

	void FindSafePos(Vector &startPos);

	// Player avoidance
	bool ShouldCollide( int collisionGroup, int contentsMask ) const;
	void AvoidPlayers( CUserCmd *pCmd );
	float m_fNextThinkPushAway;
	virtual bool CreateMove( float flInputSampleTime, CUserCmd *pCmd );
	virtual void ClientThink();
	virtual const QAngle& EyeAngles( void );

	// Have this player play the sounds from his view model's reload animation.
	void PlayReloadEffect();

// Called by shared code.
public:
	virtual void				PreThink( void );

	SDKPlayerState		State_Get() const;

	void				CheckShotCharging();
	void				ResetShotCharging();

	void				CheckLastPressedSingleMoveButton();
	
	void DoAnimationEvent(PlayerAnimEvent_t event);
	virtual bool ShouldDraw();

	virtual C_BaseAnimating * BecomeRagdollOnClient();
	virtual IRagdoll* GetRepresentativeRagdoll() const;

	void FireBullet( 
		Vector vecSrc, 
		const QAngle &shootAngles, 
		float vecSpread, 
		int iDamage, 
		int iBulletType,
		CBaseEntity *pevAttacker,
		bool bDoEffects,
		float x,
		float y ) {};

	//ios added
	//virtual ShadowType_t	ShadowCastType( void );
	//void					PostThink(void);
	//end ios

	void IncreaseShotsFired() { m_iShotsFired++; if (m_iShotsFired > 16) m_iShotsFired = 16; }
	void DecreaseShotsFired() { m_iShotsFired--; if (m_iShotsFired < 0) m_iShotsFired = 0; }
	void ClearShotsFired() { m_iShotsFired = 0; }
	int GetShotsFired() { return m_iShotsFired; }

	virtual void SharedSpawn();
	
	void InitSpeeds( void ); //Tony; called EVERY spawn on server and client after class has been chosen (if any!)

	// Returns true if the player is allowed to attack.
	bool CanAttack( void ) { return false; };

	void SetSprinting( bool bIsSprinting );
	bool IsSprinting( void );

	CSDKPlayerShared m_Shared;

	virtual const Vector	GetPlayerMins( void ) const; // uses local player
	virtual const Vector	GetPlayerMaxs( void ) const; // uses local player

// Not Shared, but public.
public:

#if defined ( SDK_USE_TEAMS )
	bool CanShowTeamMenu();
#endif // SDK_USE_TEAMS

#if defined ( SDK_USE_PLAYERCLASSES )
	bool CanShowClassMenu();
#endif // SDK_USE_PLAYERCLASSES

	void LocalPlayerRespawn( void );

	//Tony; update lookat, if our model has moving eyes setup, they need to be updated.
	void			UpdateLookAt( void );
	int				GetIDTarget() const;
	void			UpdateIDTarget( void );

	//Tony; when model is changed, need to init some stuff.
	virtual CStudioHdr *OnNewModel( void );
	void InitializePoseParams( void );

public: // Public Variables
	CSDKPlayerAnimState *m_PlayerAnimState;
	ShadowType_t ShadowCastType(); //ios
#if defined ( SDK_USE_PRONE )
	bool m_bUnProneToDuck;		//Tony; GAMEMOVEMENT USED VARIABLE
#endif // SDK_USE_PRONE

	QAngle	m_angEyeAngles;
	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;

	CNetworkVar( int, m_iShotsFired );	// number of shots fired recently

	EHANDLE	m_hRagdoll;

	int	m_headYawPoseParam;
	int	m_headPitchPoseParam;
	float m_headYawMin;
	float m_headYawMax;
	float m_headPitchMin;
	float m_headPitchMax;

	Vector m_vLookAtTarget;

	float m_flLastBodyYaw;
	float m_flCurrentHeadYaw;
	float m_flCurrentHeadPitch;

	int	  m_iIDEntIndex;
	int GetArmorValue() { return m_ArmorValue; }

	void ApplyBoneMatrixTransform(matrix3x4_t& transform);

	CNetworkVar(float, m_flStateEnterTime);

private:
	void UpdateSoundEvents();

	CNetworkVar( bool, m_bSpawnInterpCounter );
	bool m_bSpawnInterpCounterCache;

	CNetworkVar( SDKPlayerState, m_iPlayerState );

	C_SDKPlayer( const C_SDKPlayer & );

	int m_ArmorValue;

	class CSDKSoundEvent
	{
	public:
		string_t m_SoundName;
		float m_flEventTime;	// Play the event when gpGlobals->curtime goes past this.
	};
	CUtlLinkedList<CSDKSoundEvent,int> m_SoundEvents;

public:

	CNetworkVector(m_vTargetPos);
	CNetworkVar(bool, m_bIsAtTargetPos);
	CNetworkVar(bool, m_bHoldAtTargetPos);
	CNetworkVar(float, m_flNextClientSettingsChangeTime);
	CNetworkVar(float, m_flNextJoin);
	CNetworkVar(int, m_nInPenBoxOfTeam);
	CNetworkVar(bool, m_bShotButtonsReleased);

	void CheckBallShield(const Vector &oldPos, Vector &newPos, const Vector &oldVel, Vector &newVel, const QAngle &oldAng, QAngle &newAng);
	void MoveToTargetPos(Vector &pos, Vector &vel, QAngle &ang);

	virtual int					DrawModel( int flags );
};


inline C_SDKPlayer* ToSDKPlayer( CBaseEntity *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsPlayer() )
		return NULL;

	return static_cast< C_SDKPlayer* >( pPlayer );
}

inline SDKPlayerState C_SDKPlayer::State_Get() const
{
	return m_iPlayerState;
}

#endif // C_SDK_PLAYER_H
