//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for SDK Game
//
// $NoKeywords: $
//=============================================================================//

#ifndef SDK_PLAYER_H
#define SDK_PLAYER_H
#pragma once

#include "basemultiplayerplayer.h"
#include "server_class.h"
#include "sdk_playeranimstate.h"
#include "sdk_player_shared.h"
#include "sdk_gamerules.h"
#include "sdk_shareddefs.h"
#include "steam/steam_api.h"
#include "ios_teamkit_parse.h"
#include "props.h"
#include "player_data.h"

class CBall;
class CPlayerBall;
class CMatchBall;

// Function table for each player state.
class CSDKPlayerStateInfo
{
public:
	SDKPlayerState m_iPlayerState;
	const char *m_pStateName;
	
	void (CSDKPlayer::*pfnEnterState)();	// Init and deinit the state.
	void (CSDKPlayer::*pfnPreThink)();	// Do a PreThink() in this state.
	void (CSDKPlayer::*pfnLeaveState)();
};

//=============================================================================
// >> SDK Game player
//=============================================================================
class CSDKPlayer : public CBaseMultiplayerPlayer
{
public:
	DECLARE_CLASS( CSDKPlayer, CBaseMultiplayerPlayer );
	DECLARE_SERVERCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

	CSDKPlayer();
	~CSDKPlayer();

	static CSDKPlayer *CreatePlayer( const char *className, edict_t *ed );
	static CSDKPlayer* Instance( int iEnt );

	// This passes the event to the client's and server's CPlayerAnimState.
	void DoServerAnimationEvent(PlayerAnimEvent_t event);
	void DoAnimationEvent(PlayerAnimEvent_t event);

	virtual void FlashlightTurnOn( void ) {};
	virtual void FlashlightTurnOff( void ) {};
	virtual int FlashlightIsOn( void ) { return 0; };

	virtual void PreThink();
	virtual void PostThink();
	virtual void Spawn();
	virtual void InitialSpawn();

	virtual void GiveDefaultItems() {};

	// Animstate handles this.
	void SetAnimation( PLAYER_ANIM playerAnim ); //ios {return; }

	virtual void Precache();
	virtual int			OnTakeDamage( const CTakeDamageInfo &inputInfo ) { return 0; };
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info ) { return 0; };
	virtual void Event_Killed( const CTakeDamageInfo &info ) {};
	virtual void TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr ) {};
	virtual void LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles ) {};
	
	virtual void	CreateViewModel( int viewmodelindex = 0 ) {};

	virtual void	CheatImpulseCommands( int iImpulse );
	
	virtual int		SpawnArmorValue( void ) const { return m_iSpawnArmorValue; }
	virtual void	SetSpawnArmorValue( int i ) { m_iSpawnArmorValue = i; }

	CNetworkQAngle( m_angEyeAngles );	// Copied from EyeAngles() so we can send it to the client.
	CNetworkQAngle( m_angCamViewAngles );
	CNetworkVar( int, m_iShotsFired );	// number of shots fired recently

	// Tracks our ragdoll entity.
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 
#if defined ( SDK_USE_PLAYERCLASSES )
	int GetPlayerClassAsString( char *pDest, int iDestSize );
	void SetClassMenuOpen( bool bIsOpen );
	bool IsClassMenuOpen( void );
#endif
	void PhysObjectSleep();
	void PhysObjectWake();

	virtual void ChangeTeam();

	// Player avoidance
	virtual	bool		ShouldCollide( int collisionGroup, int contentsMask ) const;
	void SDKPushawayThink(void);

// In shared code.
public:
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

	CNetworkVarEmbedded( CSDKPlayerShared, m_Shared );
	virtual bool		ClientCommand( const CCommand &args );

	void IncreaseShotsFired() { m_iShotsFired++; if (m_iShotsFired > 16) m_iShotsFired = 16; }
	void DecreaseShotsFired() { m_iShotsFired--; if (m_iShotsFired < 0) m_iShotsFired = 0; }
	void ClearShotsFired() { m_iShotsFired = 0; }
	int GetShotsFired() { return m_iShotsFired; }

#if defined ( SDK_USE_SPRINTING )
	void SetSprinting( bool bIsSprinting );
#endif // SDK_USE_SPRINTING
	// Returns true if the player is allowed to attack.
	bool CanAttack( void ) { return false; };

	virtual int GetPlayerStance();

	// Called whenever this player fires a shot.
	void NoteWeaponFired() {};
	virtual bool WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;

// ------------------------------------------------------------------------------------------------ //
// Player state management.
// ------------------------------------------------------------------------------------------------ //
public:

	void State_Transition( SDKPlayerState newState );
	SDKPlayerState State_Get() const;				// Get the current state.

	virtual const Vector	GetPlayerMins( void ) const; // uses local player
	virtual const Vector	GetPlayerMaxs( void ) const; // uses local player

	virtual bool		CanHearAndReadChatFrom( CBasePlayer *pPlayer );
	virtual bool		CanSpeak(MessageMode_t messageMode);
	virtual const char	*GetPlayerName();
	virtual void		SetPlayerName(const char *name);

private:
	void State_Enter( SDKPlayerState newState );	// Initialize the new state.
	void State_Leave();								// Cleanup the previous state.
	void State_PreThink();							// Update the current state.

	void State_PICKINGTEAM_Enter();
	void State_PICKINGCLASS_Enter();

	void State_ACTIVE_Enter();
	void State_ACTIVE_PreThink();
	void State_ACTIVE_Leave();

	void State_OBSERVER_MODE_Enter();
	void State_OBSERVER_MODE_PreThink();
	void State_OBSERVER_MODE_Leave();

	// Find the state info for the specified state.
	static CSDKPlayerStateInfo* State_LookupInfo( SDKPlayerState state );

	// This tells us which state the player is currently in (joining, observer, dying, etc).
	// Each state has a well-defined set of parameters that go with it (ie: observer is movetype_noclip, non-solid,
	// invisible, etc).
	CNetworkVar( SDKPlayerState, m_iPlayerState );
	CNetworkVar(float, m_flStateEnterTime);

	CSDKPlayerStateInfo *m_pCurStateInfo;			// This can be NULL if no state info is defined for m_iPlayerState.
	bool HandleCommand_JoinTeam( int iTeam );

#if defined ( SDK_USE_PLAYERCLASSES )
	bool HandleCommand_JoinClass( int iClass );
	void ShowClassSelectMenu();
	bool m_bIsClassMenuOpen;
#endif

#if defined ( SDK_USE_PRONE )
	void InitProne( void );
#endif // SDK_USE_PRONE

#if defined ( SDK_USE_SPRINTING )
	void InitSprinting( void );
	bool IsSprinting( void );
	bool CanSprint( void );
#endif // SDK_USE_SPRINTING

	void InitSpeeds( void ); //Tony; called EVERY spawn on server and client after class has been chosen (if any!)

	// from CBasePlayer
	void			SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize );

	virtual void	SharedSpawn();

private:
	// Last usercmd we shot a bullet on.
	int m_iLastWeaponFireUsercmd;

	virtual void Weapon_Equip( CBaseCombatWeapon *pWeapon ) {};		//Tony; override so diethink can be cleared
	virtual void ThrowActiveWeapon( void ) {};
	virtual void SDKThrowWeapon( CWeaponSDKBase *pWeapon, const Vector &vecForward, const QAngle &vecAngles, float flDiameter  ) {};
	virtual void SDKThrowWeaponDir( CWeaponSDKBase *pWeapon, const Vector &vecForward, Vector *pVecThrowDir ) {};
	// When the player joins, it cycles their view between trigger_camera entities.
	// This is the current camera, and the time that we'll switch to the next one.
	EHANDLE m_pIntroCamera;
	float m_fIntroCamTime;

	void CreateRagdollEntity() {};
	void DestroyRagdoll( void ) {};


	CSDKPlayerAnimState *m_PlayerAnimState;

	CNetworkVar( bool, m_bSpawnInterpCounter );

	int m_iSpawnArmorValue;
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_ArmorValue );
public:
#if defined ( SDK_USE_PRONE )
	bool m_bUnProneToDuck;		//Tony; GAMEMOVEMENT USED VARIABLE
#endif // SDK_USE_PRONE

public:
	//virtual void		SetAnimation( PLAYER_ANIM playerAnim );
	void				PlayerUse ( void ) {};				//IOS
	CBaseEntity			*FindUseEntity(void) { return NULL; };
	//bool				ClientCommand(const char *pcmd);
	//CBaseEntity			*EntSelectSpawnPoint(void);

	void				DrawDebugGeometryOverlays(void);

	bool				IsTeamPosFree(int team, int posIndex, bool ignoreBots, CSDKPlayer **pPlayerOnPos);

	int					m_nTeamPosIndex;
	int					m_nShirtNumber;
	int					m_nPreferredOutfieldShirtNumber;
	int					m_nPreferredKeeperShirtNumber;
	char				m_szPlayerBallSkinName[MAX_KITNAME_LENGTH];

	float				m_flNextShot;
	float				m_flNextFoulCheck;

	float				GetNextJoin() { return m_flNextJoin; }
	void				SetNextJoin(float time) { m_flNextJoin = time; }

	int					GetShirtNumber(void);

	int					GetTeamPosType(void);

	int					GetTeamPosIndex(void) { return m_nTeamPosIndex; }

	int					GetTeamToJoin(void) { return m_nTeamToJoin; }

	int					GetTeamPosIndexToJoin(void) { return m_nTeamPosIndexToJoin; }

	void				SetPositionAfterTeamChange(const Vector &pos, const QAngle &ang, bool findSafePos);
	QAngle				GetAngleToBall(const Vector &pos, bool centerPitch);

	void				SetPreferredOutfieldShirtNumber(int num);
	void				SetPreferredKeeperShirtNumber(int num);
	int					FindAvailableShirtNumber();

	int					GetSkinIndex();
	void				SetSkinIndex(int index);

	int					GetHairIndex();
	void				SetHairIndex(int index);

	int					GetSleeveIndex();
	void				SetSleeveIndex(int index);

	const char 			*GetShoeName() { return m_szShoeName; }
	void				SetShoeName(const char *shoeName);

	const char 			*GetKeeperGloveName() { return m_szKeeperGloveName; }
	void				SetKeeperGloveName(const char *keeperGloveName);

	const char 			*GetPlayerBallSkinName() { return m_szPlayerBallSkinName; }
	void				SetPlayerBallSkinName(const char *skinName);

	int					GetSpecTeam() { return m_nSpecTeam; }

	void				FindSafePos(Vector &startPos);

	void				Reset();
	void				RemoveFlags();

	const char			*GetClubName();
	void				SetClubName(const char *name);

	const char			*GetNationalTeamName();
	void				SetNationalTeamName(const char *name);

	const char			*GetShirtName();
	void				SetShirtName(const char *name);

	int					GetCountryIndex() { return m_nCountryIndex; }
	void				SetCountryIndex(int index) { m_nCountryIndex = index; }

	bool				IsReverseSideCurl() { return m_bReverseSideCurl; } 
	void				SetReverseSideCurl(bool enable) { m_bReverseSideCurl = enable; }

	bool				ShotButtonsPressed();
	bool				ShotButtonsReleased();
	void				SetShotButtonsReleased(bool released);

	void				SetChargedshotBlocked(bool blocked);
	bool				ChargedshotBlocked();

	void				SetShotsBlocked(bool blocked);
	bool				ShotsBlocked();

	bool				IsCardBanned();

	void				LookAtBall(void);

	float				m_flRemoteControlledStartTime;

	virtual void		VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	char				m_szPlayerName[MAX_PLAYER_NAME_LENGTH];
	bool				m_bPlayerNameChanged;

	char				m_szClubName[MAX_CLUBNAME_LENGTH];
	bool				m_bClubNameChanged;

	char				m_szNationalTeamName[MAX_CLUBNAME_LENGTH];
	bool				m_bNationalTeamNameChanged;

	char				m_szShirtName[MAX_SHIRT_NAME_LENGTH];
	bool				m_bShirtNameChanged;

	int					m_nCountryIndex;

	bool				m_bReverseSideCurl;

	Vector				EyeDirection2D();
	Vector				EyeDirection3D();

	void				SetOffside(bool isOffside);
	void				SetOffsidePos(Vector pos);
	bool				IsOffside();
	Vector				GetOffsidePos();
	void				SetOffsideLastOppPlayerPos(Vector pos);
	Vector				GetOffsideLastOppPlayerPos();
	void				SetOffsideBallPos(Vector pos);
	Vector				GetOffsideBallPos();
	
	void				SetPosInsideShield(const Vector &pos, bool holdAtTargetPos);
	void				SetPosOutsideShield(bool teleport);
	void				GetTargetPos(const Vector &pos, Vector &targetPos);
	void				ActivateRemoteControlling(const Vector &targetPos);

	Vector				GetSpawnPos();

	bool				IsAway() { return m_bIsAway; }
	void				SetAway(bool away) { m_bIsAway = away; }

	void				SetLastMoveTime(float time) { m_flLastMoveTime = time; }

	CNetworkVector(m_vTargetPos);
	CNetworkVar(bool, m_bIsAtTargetPos);
	CNetworkVar(bool, m_bHoldAtTargetPos);
	CNetworkVar(float, m_flNextClientSettingsChangeTime);
	CNetworkVar(float, m_flNextJoin);
	CNetworkVar(int, m_nModelScale);
	CNetworkString(m_szShoeName, MAX_KITNAME_LENGTH);
	CNetworkString(m_szKeeperGloveName, MAX_KITNAME_LENGTH);

	static bool			IsOnField(CSDKPlayer *pPl, int teamNumber = TEAM_NONE);
	static bool			CheckPlayersAtShieldPos(bool waitUntilOutsideShield);

	void				CheckShotCharging();
	void				ResetShotCharging();
	float				GetChargedShotStrength();
	float				GetChargedShotIncreaseDuration();

	void				CheckLastPressedSingleMoveButton();

	int					m_nTeamToJoin;
	int					m_nTeamPosIndexToJoin;
	int					m_nSpecTeamToJoin;
	bool				m_bJoinSilently;

	int					m_ePenaltyState;
	void				SetPlayerBall(CPlayerBall *pPlayerBall) { m_pPlayerBall = pPlayerBall; }
	CPlayerBall			*GetPlayerBall() { return m_pPlayerBall; }
	bool				SetDesiredTeam(int desiredTeam, int desiredSpecTeam, int desiredPosIndex, bool switchInstantly, bool setNextJoinDelay, bool silent);

	void				CheckBallShield(const Vector &oldPos, Vector &newPos, const Vector &oldVel, Vector &newVel, const QAngle &oldAng, QAngle &newAng);
	void				MoveToTargetPos(Vector &pos, Vector &vel, QAngle &ang);
	int					GetSidemoveSign();

	bool				IsKeeperDiving();
	bool				IsNormalshooting();
	bool				IsChargedshooting();
	bool				IsShooting();
	bool				CanShoot();
	bool				DoSkillMove();
	bool				DoGesture();
	bool				IsInOwnBoxAsKeeper();

	CSDKPlayer			*FindClosestPlayerToSelf(bool teammatesOnly, bool forwardOnly = false, float maxYawAngle = 360);

	void				AllowPropCreation(bool allow);
	bool				IsPropCreationAllowed() { return m_bAllowPropCreation; }
	void				RemoveProps();

	CNetworkHandle(CBall, m_pHoldingBall);

	CUtlVector<CDynamicProp *> m_PlayerProps;
	static void		RemoveAllPlayerProps();

	inline void	SetData(CPlayerData *data) { m_pData = data; }
	inline CPlayerData *GetData() { return m_pData; }

protected:

	bool				m_bIsAway;
	float				m_flLastMoveTime;
	bool				m_bIsOffside;
	Vector				m_vOffsidePos;
	CHandle<CPlayerBall>	m_pPlayerBall;
	Vector				m_vOffsideLastOppPlayerPos;
	Vector				m_vOffsideBallPos;
	int					m_nSpecTeam;
	bool				m_bAllowPropCreation;

	CNetworkVar(bool, m_bChargedshotBlocked);
	CNetworkVar(bool, m_bShotsBlocked);

	CNetworkVar(int, m_nSkinIndex);
	CNetworkVar(int, m_nHairIndex);
	int m_nSleeveIndex;

	void CheckPosChange();
	void CheckAwayState();

	CPlayerData *m_pData;
};


inline CSDKPlayer *ToSDKPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

#ifdef _DEBUG
	Assert( dynamic_cast<CSDKPlayer*>( pEntity ) != 0 );
#endif
	return static_cast< CSDKPlayer* >( pEntity );
}

inline SDKPlayerState CSDKPlayer::State_Get() const
{
	return m_iPlayerState;
}

#endif	// SDK_PLAYER_H
