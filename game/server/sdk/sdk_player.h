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

extern CUniformRandomStream g_IOSRand;

const int MODEL_PLAYER			= 0;
const int MODEL_KEEPER			= 1;
const int MODEL_KEEPER_AND_BALL	= 2;

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
	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );

	virtual void FlashlightTurnOn( void );
	virtual void FlashlightTurnOff( void );
	virtual int FlashlightIsOn( void );

	virtual void PreThink();
	virtual void PostThink();
	virtual void Spawn();
	virtual void InitialSpawn();

	virtual void GiveDefaultItems();

	// Animstate handles this.
	void SetAnimation( PLAYER_ANIM playerAnim ); //ios {return; }

	virtual void Precache();
	virtual int			OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr );
	virtual void LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles );
	
	CWeaponSDKBase* GetActiveSDKWeapon() const;
	virtual void	CreateViewModel( int viewmodelindex = 0 );

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

#if defined ( SDK_USE_TEAMS )
	virtual void ChangeTeam( int iTeamNum );
#endif
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
		float y );

	CNetworkVarEmbedded( CSDKPlayerShared, m_Shared );
	virtual void			PlayerDeathThink( void );
	virtual bool		ClientCommand( const CCommand &args );

	void IncreaseShotsFired() { m_iShotsFired++; if (m_iShotsFired > 16) m_iShotsFired = 16; }
	void DecreaseShotsFired() { m_iShotsFired--; if (m_iShotsFired < 0) m_iShotsFired = 0; }
	void ClearShotsFired() { m_iShotsFired = 0; }
	int GetShotsFired() { return m_iShotsFired; }

#if defined ( SDK_USE_SPRINTING )
	void SetSprinting( bool bIsSprinting );
#endif // SDK_USE_SPRINTING
	// Returns true if the player is allowed to attack.
	bool CanAttack( void );

	virtual int GetPlayerStance();

	// Called whenever this player fires a shot.
	void NoteWeaponFired();
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
	void State_Enter_WELCOME();
	void State_PreThink_WELCOME();

	void State_Enter_PICKINGTEAM();
	void State_Enter_PICKINGCLASS();

public: //Tony; I had this private but I need it public for initial spawns.
	void MoveToNextIntroCamera();
private:

	void State_Enter_ACTIVE();
	void State_PreThink_ACTIVE();

	void State_Enter_OBSERVER_MODE();
	void State_PreThink_OBSERVER_MODE();

	void State_Enter_DEATH_ANIM();
	void State_PreThink_DEATH_ANIM();

	// Find the state info for the specified state.
	static CSDKPlayerStateInfo* State_LookupInfo( SDKPlayerState state );

	// This tells us which state the player is currently in (joining, observer, dying, etc).
	// Each state has a well-defined set of parameters that go with it (ie: observer is movetype_noclip, non-solid,
	// invisible, etc).
	CNetworkVar( SDKPlayerState, m_iPlayerState );

	CSDKPlayerStateInfo *m_pCurStateInfo;			// This can be NULL if no state info is defined for m_iPlayerState.
	bool HandleCommand_JoinTeam( int iTeam );
#if defined ( SDK_USE_TEAMS )
	bool	m_bTeamChanged;		//have we changed teams this spawn? Used to enforce one team switch per death rule
#endif

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

	CBaseEntity*	EntSelectSpawnPoint();
	bool			CanMove( void ) const;

	virtual void	SharedSpawn();

	virtual const Vector	GetPlayerMins( void ) const; // uses local player
	virtual const Vector	GetPlayerMaxs( void ) const; // uses local player

	virtual void		CommitSuicide( bool bExplode = false, bool bForce = false );

private:
	// Last usercmd we shot a bullet on.
	int m_iLastWeaponFireUsercmd;

	virtual void Weapon_Equip( CBaseCombatWeapon *pWeapon );		//Tony; override so diethink can be cleared
	virtual void ThrowActiveWeapon( void );
	virtual void SDKThrowWeapon( CWeaponSDKBase *pWeapon, const Vector &vecForward, const QAngle &vecAngles, float flDiameter  );
	virtual void SDKThrowWeaponDir( CWeaponSDKBase *pWeapon, const Vector &vecForward, Vector *pVecThrowDir );
	// When the player joins, it cycles their view between trigger_camera entities.
	// This is the current camera, and the time that we'll switch to the next one.
	EHANDLE m_pIntroCamera;
	float m_fIntroCamTime;

	void CreateRagdollEntity();
	void DestroyRagdoll( void );


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
	void				PlayerUse ( void );				//IOS
	CBaseEntity			*FindUseEntity(void);
	//bool				ClientCommand(const char *pcmd);
	//CBaseEntity			*EntSelectSpawnPoint(void);

	void				RequestTeamChange( int iTeamNum );
	void				ConvertSpawnToShirt(void);
	void				ChooseModel(void);
	void				ChoosePlayerSkin(void);
	void				ChooseKeeperSkin(void);
	bool				TeamPosFree (int team, int pos, bool kickBotKeeper = false);
	void				CheckRejoin(void);
	//virtual void		CommitSuicide();

	int					m_TeamPos;								//position on pitch
	int					m_ShirtPos;								//converted spawn point numbers into shirt numbers

	float				m_HoldAnimTime;							//dont let player move until this expires

	int					m_RejoinTime;							//delay before brining up join menu (after kill or red)

	float				m_NextPowerShotCharge;
	float				m_PowerShotStrength;

	float				m_fPossessionTime;

	//stats
	int					m_RedCards;
	int					m_YellowCards;
	int					m_Fouls;
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
	int					GetAssists(void) { return m_Assists; }
	int					GetPossession(void) { return m_Possession; }
	int					GetPasses(void) { return m_Passes; }
	int					GetFreeKicks(void) { return m_FreeKicks; }
	int					GetPenalties(void) { return m_Penalties; }
	int					GetCorners(void) { return m_Corners; }
	int					GetThrowIns(void) { return m_ThrowIns; }
	int					GetKeeperSaves(void) { return m_KeeperSaves; }
	int					GetGoalKicks(void) { return m_GoalKicks; }
	int					GetTeamPosition(void) { return m_ShirtPos; }
	int					GetSprint(void) { if (m_fSprintLeft < 0.0f) return 0; else return ((int)(m_fSprintLeft * 10.0f)); }

	void				UpdateSprint( void );
	bool				InSprint(void);
	float				m_fSprintLeft;
	float				m_fSprintIdle;
	float				m_fSprintUpdate;

	virtual void		Duck( void );

	int					m_PlayerAnim;
	float				m_NextSlideTime;
	float				m_TackleTime;
	CBall*				GetBall(CBall *pNext);
	bool				IsOnPitch(void);
	bool				IsOffPitch(void);
	bool				m_bTackleDone;
	CSDKPlayer*			FindNearestSlideTarget(void);
	CBall*				FindNearestBall(void);
	void				GiveRedCard(void);
	bool				ApplyTackledAnim(CSDKPlayer *pTackled);
	bool				m_bSlideKick;

	int					m_nBaseSkin;					//keeper skin before ball offset added

	void				LookAtBall(void);

	float				m_JoinTime;

	void				IOSSPlayerCollision(void);

	float				m_NextShoot;					//can block this individual from kicking

	float				m_KickDelay;					//frozen while on

	virtual void		VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	//int					m_BallInPenaltyBox;	 //-1 =	not	in box,	0,1	= teams	box

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
