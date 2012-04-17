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
#include "ball.h"
#include "sdk_gamerules.h"
#include "sdk_shareddefs.h"
#include "steam/steam_api.h"

const int MODEL_PLAYER			= 0;
const int MODEL_KEEPER			= 1;
const int MODEL_KEEPER_AND_BALL	= 2;


#define PLAYER_SPEED 280.0f

#define SPRINT_TIME           6.0f     //IOS sprint amount 5.5
#define SPRINT_RECHARGE_TIME  12.0f    //IOS time before sprint re-charges
#define SPRINT_SPEED          90.0f    //IOS sprint increase in speed

class CBall;

class CPlayerPersistentData
{
public:
	const CSteamID *m_SteamID;
	int m_nYellowCards;
	int m_nRedCards;
	float m_flNextJoin;

//	CPlayerPersistentData(const CSteamID *steamID);
	static void RetrievePlayerData(CSDKPlayer *pPl);
	static void SavePlayerData(CSDKPlayer *pPl);
	static CUtlVector<CPlayerPersistentData *> m_PlayerPersistentData;
};

// Function table for each player state.
class CSDKPlayerStateInfo
{
public:
	SDKPlayerState m_iPlayerState;
	const char *m_pStateName;
	
	void (CSDKPlayer::*pfnEnterState)();	// Init and deinit the state.
	void (CSDKPlayer::*pfnLeaveState)();
	void (CSDKPlayer::*pfnPreThink)();	// Do a PreThink() in this state.
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

	virtual void ChangeTeam( int iTeamNum );

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
	virtual void			PlayerDeathThink( void ) {};
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

	virtual bool	ModeWantsSpectatorGUI( int iMode ) { return ( iMode != OBS_MODE_DEATHCAM && iMode != OBS_MODE_FREEZECAM ); }

private:
	//ios bool SelectSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot );

	void State_Enter( SDKPlayerState newState );	// Initialize the new state.
	void State_Leave();								// Cleanup the previous state.
	void State_PreThink();							// Update the current state.

	// Specific state handler functions.
	void State_WELCOME_Enter();
	void State_WELCOME_PreThink();

	void State_PICKINGTEAM_Enter();
	void State_PICKINGCLASS_Enter();

private:

	void State_ACTIVE_Enter();
	void State_ACTIVE_PreThink();

	void State_OBSERVER_MODE_Enter();
	void State_OBSERVER_MODE_Leave();
	void State_OBSERVER_MODE_PreThink();

	void State_DEATH_ANIM_Enter();
	void State_DEATH_ANIM_PreThink();

	// Find the state info for the specified state.
	static CSDKPlayerStateInfo* State_LookupInfo( SDKPlayerState state );

	// This tells us which state the player is currently in (joining, observer, dying, etc).
	// Each state has a well-defined set of parameters that go with it (ie: observer is movetype_noclip, non-solid,
	// invisible, etc).
	CNetworkVar( SDKPlayerState, m_iPlayerState );

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

	virtual const Vector	GetPlayerMins( void ) const; // uses local player
	virtual const Vector	GetPlayerMaxs( void ) const; // uses local player

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

	void				ChoosePlayerSkin(void);
	void				ChooseKeeperSkin(void);
	bool				TeamPosFree(int team, int pos, bool kickBotKeeper);

	int					m_TeamPos;
	
	float				m_flNextShot;

	float				m_flPossessionTime;

	//stats
	int					m_RedCards;
	int					m_YellowCards;
	int					m_Fouls;
	int					m_Offsides;
	int					m_Goals;
	int					m_Assists;
	int					m_Possession;
	int					m_Passes;
	int					m_FreeKicks;
	int					m_Penalties;
	int					m_Corners;
	int					m_ThrowIns;
	int					m_KeeperSaves;
	int					m_GoalKicks;

	int					GetRedCards(void) { return m_RedCards; }
	int					GetYellowCards(void) { return m_YellowCards; }
	int					GetFouls(void) { return m_Fouls; }
	int					GetOffsides(void) { return m_Offsides; }
	int					GetGoals(void) { return m_Goals; }
	int					GetAssists(void) { return m_Assists; }
	int					GetPossession(void) { return m_Possession; }
	int					GetPasses(void) { return m_Passes; }
	int					GetFreeKicks(void) { return m_FreeKicks; }
	int					GetPenalties(void) { return m_Penalties; }
	int					GetCorners(void) { return m_Corners; }
	int					GetThrowIns(void) { return m_ThrowIns; }
	int					GetKeeperSaves(void) { return m_KeeperSaves; }
	int					GetGoalKicks(void) { return m_GoalKicks; }
	int					GetTeamPosition(void);
	int					GetTeamPosIndex(void) { return m_TeamPos; }
	int					GetTeamToJoin(void) { return m_nTeamToJoin; }
	int					GetNextJoin(void) { return m_flNextJoin; }

	char				*GetClubName() { return m_szClubName; }
	void				SetClubName(const char *name) { Q_strncpy(m_szClubName, name, sizeof(m_szClubName)); m_bClubNameChanged = true; } 

	char				*GetCountryName() { return m_szCountryName; }
	void				SetCountryName(const char *name) { Q_strncpy(m_szCountryName, name, sizeof(m_szCountryName)); m_bCountryNameChanged = true; } 

	float				m_TackleTime;
	bool				m_bTackleDone;
	bool				m_bSlideKick;

	int					m_nBaseSkin;					//keeper skin before ball offset added

	void				LookAtBall(void);

	float				m_JoinTime;

	void				IOSSPlayerCollision(void);

	virtual void		VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	//int					m_BallInPenaltyBox;	 //-1 =	not	in box,	0,1	= teams	box

	char				m_szClubName[32];
	bool				m_bClubNameChanged;

	char				m_szCountryName[32];
	bool				m_bCountryNameChanged;

	Vector				EyeDirection2D();
	Vector				EyeDirection3D();

	void				SetOffside(bool isOffside);
	bool				IsOffside();
	Vector				GetOffsidePos();
	void				SetLastOffsidePlayerPos(Vector pos);
	Vector				GetLastOffsidePlayerPos();
	
	void				SetPosInsideShield(Vector pos, bool holdAtTargetPos);
	void				SetPosOutsideShield(bool holdAtTargetPos);
	void				GetTargetPos(const Vector &pos, Vector &targetPos);

	Vector				GetSpawnPos(bool findSafePos);

	CNetworkVector(m_vTargetPos);
	CNetworkVar(bool, m_bIsAtTargetPos);
	CNetworkVar(bool, m_bHoldAtTargetPos);

	CNetworkVar(int, m_nInPenBoxOfTeam);

	static bool			IsOnField(CSDKPlayer *pPl);

	void				ResetStats();

	bool				m_bShotButtonsDepressed;

	float				m_flNextJoin;
	int					m_nTeamToJoin;

	int					m_ePenaltyState;
	PlayerAnimEvent_t	m_ePlayerAnimEvent;
	float				m_flPlayerAnimEventStart;
	void				SetPlayerBall(CBall *pPlayerBall) { m_pPlayerBall = pPlayerBall; }
	CBall				*GetPlayerBall() { return m_pPlayerBall; }
	void				ChangeTeamPos(int team, int pos, bool instantly = false);

	void CheckBallShield(const Vector &oldPos, Vector &newPos, const Vector &oldVel, Vector &newVel, const QAngle &oldAng, QAngle &newAng);
	void MoveToTargetPos(Vector &pos, Vector &vel, QAngle &ang);

protected:

	bool				m_bIsOffside;
	Vector				m_vOffsidePos;
	CBall				*m_pPlayerBall;
	Vector				m_vLastOffsidePlayerPos;
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
