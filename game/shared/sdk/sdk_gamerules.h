//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef SDK_GAMERULES_H
#define SDK_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif


#include "teamplay_gamerules.h"
#include "convar.h"
#include "gamevars_shared.h"
#include "weapon_sdkbase.h"

#ifdef CLIENT_DLL
	#include "c_baseplayer.h"
#else
	#include "player.h"
	#include "sdk_player.h"
	#include "utlqueue.h"
	#include "playerclass_info_parse.h"

#endif


#ifdef CLIENT_DLL
	#define CSDKGameRules C_SDKGameRules
	#define CSDKGameRulesProxy C_SDKGameRulesProxy
#endif

enum match_state_t
{
	MATCH_INIT = 0,
	MATCH_WARMUP,
	MATCH_FIRST_HALF,
	MATCH_FIRST_HALF_INJURY_TIME,
	MATCH_HALFTIME,
	MATCH_SECOND_HALF,
	MATCH_SECOND_HALF_INJURY_TIME,
	MATCH_EXTRATIME_INTERMISSION,
	MATCH_EXTRATIME_FIRST_HALF,
	MATCH_EXTRATIME_FIRST_HALF_INJURY_TIME,
	MATCH_EXTRATIME_HALFTIME,
	MATCH_EXTRATIME_SECOND_HALF,
	MATCH_EXTRATIME_SECOND_HALF_INJURY_TIME,
	MATCH_PENALTIES_INTERMISSION,
	MATCH_PENALTIES,
	MATCH_COOLDOWN,
	MATCH_END
};

class CSDKGameRules;

class CSDKGameRulesStateInfo
{
public:
	match_state_t			m_nMatchState;
	const char				*m_pStateName;

	void (CSDKGameRules::*pfnEnterState)();	// Init and deinit the state.
	void (CSDKGameRules::*pfnLeaveState)();
	void (CSDKGameRules::*pfnThink)();	// Do a PreThink() in this state.
};

class CSDKGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CSDKGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CSDKViewVectors : public CViewVectors
{
public:
	CSDKViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight
#if defined ( SDK_USE_PRONE )
		,Vector vProneHullMin,
		Vector vProneHullMax,
		Vector vProneView
#endif
		) :
			CViewVectors( 
				vView,
				vHullMin,
				vHullMax,
				vDuckHullMin,
				vDuckHullMax,
				vDuckView,
				vObsHullMin,
				vObsHullMax,
				vDeadViewHeight )
	{
#if defined( SDK_USE_PRONE )
		m_vProneHullMin = vProneHullMin;
		m_vProneHullMax = vProneHullMax;
		m_vProneView = vProneView;
#endif 
	}
#if defined ( SDK_USE_PRONE )
	Vector m_vProneHullMin;
	Vector m_vProneHullMax;	
	Vector m_vProneView;
#endif
};

class CSDKGameRules : public CTeamplayRules
{
public:
	DECLARE_CLASS( CSDKGameRules, CTeamplayRules );

	virtual bool	ShouldCollide( int collisionGroup0, int collisionGroup1 );

	virtual int		PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	//ios
	virtual bool	IsTeamplay( void ) { return true; }
	// Get the view vectors for this mod.
	virtual const CViewVectors* GetViewVectors() const;
	virtual const CSDKViewVectors *GetSDKViewVectors() const;
	//Tony; define a default encryption key.
	virtual const unsigned char *GetEncryptionKey( void ) { return (unsigned char *)"a1b2c3d4"; }

	//Tony; in shared space
#if defined ( SDK_USE_PLAYERCLASSES )
	const char *GetPlayerClassName( int cls, int team );
#endif

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

	void SetMatchState(int nMatchState);
#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
	
	CSDKGameRules();
	virtual ~CSDKGameRules();
	virtual const char *GetGameDescription( void ) { return SDK_GAME_DESCRIPTION; } 
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore );
	virtual void Think();

	void InitTeams( void );

	void CreateStandardEntities( void );

	virtual const char *GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer );
	virtual const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );

	
	//IOS
	int		m_PlayersOnTeam[TEAMS_COUNT];
	void	ChooseTeamNames(void);
	void	CountTeams(void);

	virtual void	ClientDisconnected( edict_t *pClient );		//ios

	virtual void	GoToIntermission( void );		//ios ver

	void			AutobalanceTeams(void);


	//ios CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	//ios bool IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );

#if defined ( SDK_USE_PLAYERCLASSES )
	bool IsPlayerClassOnTeam( int cls, int team );
	bool CanPlayerJoinClass( CSDKPlayer *pPlayer, int cls );
	void ChooseRandomClass( CSDKPlayer *pPlayer );
	bool ReachedClassLimit( int team, int cls );
	int CountPlayerClass( int team, int cls );
	int GetClassLimit( int team, int cls );
#endif 
	//bool TeamFull( int team_id );
	//bool TeamStacked( int iNewTeam, int iCurTeam );
	//int SelectDefaultTeam( void );

	virtual void ServerActivate();
protected:
	//void CheckPlayerPositions( void );

private:
	//void CheckLevelInitialized( void );
	//bool m_bLevelInitialized;

	Vector2D	m_vecPlayerPositions[MAX_PLAYERS];

#if defined ( SDK_USE_TEAMS )
	int	m_iSpawnPointCount_Blue;	//number of blue spawns on the map
	int	m_iSpawnPointCount_Red;	//number of red spawns on the map
#endif // SDK_USE_TEAMS

	void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, bool bIgnoreWorld );

public:
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	const char *GetKillingWeaponName( const CTakeDamageInfo &info, CSDKPlayer *pVictim, int *iWeaponID );

#endif

/* ios
public:
	float GetMapRemainingTime();	// time till end of map, -1 if timelimit is disabled
	float GetMapElapsedTime();		// How much time has elapsed since the map started.

private:
	CNetworkVar( float, m_flGameStartTime );
*/

public:
	CNetworkVar(float, m_fStart);			//from wiki
	CNetworkVar(int, m_iDuration);
	CNetworkVar(match_state_t, m_nMatchState);
	int	GetMapRemainingTime(void);				//ios
	int GetMapTime(void);
	void StartRoundtimer(int iDuration);
	inline match_state_t State_Get( void ) { return m_nMatchState; }
	float m_flStateEnterTime;
	float m_flStateInjuryTime;

#ifdef GAME_DLL
protected:
	CSDKGameRulesStateInfo		*m_pCurStateInfo;			// Per-state data 
	float						m_flStateTransitionTime;	// Timer for round states
	// State machine handling
	void State_Transition( match_state_t newState );
	void State_Enter(match_state_t newState);	// Initialize the new state.
	void State_Leave();										// Cleanup the previous state.
	void State_Think();										// Update the current state.
	static CSDKGameRulesStateInfo* State_LookupInfo(match_state_t state);	// Find the state info for the specified state.

	// State Functions
	void State_Enter_INIT();
	void State_Think_INIT();

	void State_Enter_WARMUP();
	void State_Think_WARMUP();

	void State_Enter_FIRST_HALF();
	void State_Think_FIRST_HALF();

	void State_Enter_HALFTIME();
	void State_Think_HALFTIME();

	void State_Enter_SECOND_HALF();
	void State_Think_SECOND_HALF();

	void State_Enter_EXTRATIME_INTERMISSION();
	void State_Think_EXTRATIME_INTERMISSION();

	void State_Enter_EXTRATIME_FIRST_HALF();
	void State_Think_EXTRATIME_FIRST_HALF();

	void State_Enter_EXTRATIME_HALFTIME();
	void State_Think_EXTRATIME_HALFTIME();

	void State_Enter_EXTRATIME_SECOND_HALF();
	void State_Think_EXTRATIME_SECOND_HALF();

	void State_Enter_PENALTIES_INTERMISSION();
	void State_Think_PENALTIES_INTERMISSION();

	void State_Enter_PENALTIES();
	void State_Think_PENALTIES();

	void State_Enter_COOLDOWN();
	void State_Think_COOLDOWN();

	void State_Enter_END();
	void State_Think_END();

#endif

};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------

inline CSDKGameRules* SDKGameRules()
{
	return static_cast<CSDKGameRules*>(g_pGameRules);
}


#endif // SDK_GAMERULES_H
