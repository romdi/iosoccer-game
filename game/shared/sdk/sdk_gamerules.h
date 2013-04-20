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
	#include "effects.h"
#endif


#ifdef CLIENT_DLL
	#define CSDKGameRules C_SDKGameRules
	#define CSDKGameRulesProxy C_SDKGameRulesProxy
#endif

#ifdef GAME_DLL
void IOS_LogPrintf(char *fmt, ...);
extern ConVar
	sv_replays,
	sv_replay_count,
	sv_replay_duration1,
	sv_replay_duration2,
	sv_replay_duration3,
	sv_highlights,
	sv_awaytime_warmup,
	sv_awaytime_warmup_autospec,
	sv_autostartmatch;
#endif

extern CUniformRandomStream g_IOSRand;

extern ConVar
	r_winddir,
	r_windspeed,
	r_weather_hack,
	r_weather_profile,
	r_rain_radius,
	r_rain_height,
	r_rain_playervelmultiplier,
	r_rain_sidevel,
	r_rain_density,
	r_rain_width,
	r_rain_length,
	r_rain_speed,
	r_rain_alpha,
	r_rain_alphapow,
	r_rain_initialramp,
	r_rain_splashpercentage,
	r_snow_radius,
	r_snow_height,
	r_snow_playervelmultiplier,
	r_snow_sidevel,
	r_snow_density,
	r_snow_width,
	r_snow_length,
	r_snow_speed,
	r_snow_alpha,
	r_snow_alphapow,
	r_snow_initialramp;

enum match_state_t
{
	MATCH_WARMUP = 0,
	MATCH_FIRST_HALF,
	MATCH_HALFTIME,
	MATCH_SECOND_HALF,
	MATCH_EXTRATIME_INTERMISSION,
	MATCH_EXTRATIME_FIRST_HALF,
	MATCH_EXTRATIME_HALFTIME,
	MATCH_EXTRATIME_SECOND_HALF,
	MATCH_PENALTIES_INTERMISSION,
	MATCH_PENALTIES,
	MATCH_COOLDOWN
};

enum ball_shield_type_t
{
	SHIELD_NONE = 0,
	SHIELD_THROWIN,
	SHIELD_FREEKICK,
	SHIELD_CORNER,
	SHIELD_GOALKICK,
	SHIELD_KICKOFF,
	SHIELD_PENALTY,
	SHIELD_KEEPERHANDS
};

class CSDKGameRules;

class CSDKGameRulesStateInfo
{
public:
	match_state_t			m_eMatchState;
	const char				*m_pStateName;

	void (CSDKGameRules::*pfnEnterState)();	// Init and deinit the state.
	void (CSDKGameRules::*pfnThink)();	// Do a PreThink() in this state.
	void (CSDKGameRules::*pfnLeaveState)(match_state_t newState);

	ConVar					*m_MinDurationConVar;
	float					m_flMinDurationDivisor;
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
		Vector vKeeperSidewaysDiveView,
		Vector vKeeperSidewaysDiveHullMin,
		Vector vKeeperSidewaysDiveHullMax,
		Vector vSlideView,
		Vector vSlideHullMin,
		Vector vSlideHullMax,
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
		m_vKeeperSidewaysDiveView = vKeeperSidewaysDiveView;
		m_vKeeperSidewaysDiveHullMin = vKeeperSidewaysDiveHullMin;
		m_vKeeperSidewaysDiveHullMax = vKeeperSidewaysDiveHullMax;
		m_vSlideView = vSlideView;
		m_vSlideHullMin = vSlideHullMin;
		m_vSlideHullMax = vSlideHullMax;
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

	virtual int		PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget, MessageMode_t messageMode );
	//ios
	virtual bool	IsTeamplay( void ) { return true; }
	// Get the view vectors for this mod.
	virtual const CViewVectors* GetViewVectors() const;
	virtual const CSDKViewVectors *GetSDKViewVectors() const;
	//Tony; define a default encryption key.
	virtual const unsigned char *GetEncryptionKey( void ) { return (unsigned char *)"a1b2c3d4"; }

	CSDKGameRules();

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

	//void SetMatchState(match_state_t nMatchState);
#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
	

	virtual ~CSDKGameRules();
	virtual const char *GetGameDescription( void ) { return SDK_GAME_DESCRIPTION; } 
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore ) {};
	virtual void Think();

	virtual CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );

	void InitTeams( void );

	void CreateStandardEntities( void );

	virtual void LevelShutdown( void );

	virtual bool	PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker, MessageMode_t messageMode );

	virtual const char *GetChatPrefix( MessageMode_t messageMode, CBasePlayer *pPlayer );
	virtual const char *GetChatFormat( MessageMode_t messageMode, CBasePlayer *pPlayer );
	virtual const char *GetChatLocation( MessageMode_t messageMode, CBasePlayer *pPlayer );
	
	void	ChooseTeamNames(bool clubTeams, bool countryTeams, bool realTeams, bool fictitiousTeams);

	virtual void	ClientDisconnected( edict_t *pClient );

	virtual void	GoToIntermission( void );

	void			AutobalanceTeams(void);

	virtual void PlayerSpawn( CBasePlayer *pPlayer );

	virtual void ServerActivate();

private:

	void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, bool bIgnoreWorld ) {};

public:
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info ) {};
	const char *GetKillingWeaponName( const CTakeDamageInfo &info, CSDKPlayer *pVictim, int *iWeaponID ) {};

#endif

public:
	CNetworkVar(match_state_t, m_eMatchState);
	CNetworkVar(int, m_nAnnouncedInjuryTime);
	CNetworkVar(float, m_flInjuryTime);

	CNetworkVector(m_vFieldMin);
	CNetworkVector(m_vFieldMax);
	CNetworkVector(m_vKickOff);

	int	GetMapRemainingTime(void);
	int GetMapTime(void);
	void StartRoundtimer(int iDuration);
	inline match_state_t State_Get( void ) { return m_eMatchState; }
	CNetworkVar(float, m_flStateEnterTime);
	float	m_flAdjustedStateEnterTime;
	bool	m_bUseAdjustedStateEnterTime;
	CNetworkVar(float, m_flMatchStartTime);

	void RestartMatch(bool setRandomKickOffTeam, bool setRandomSides);
	int WakeUpAwayPlayers();
	void StartPenalties();

#ifdef GAME_DLL

protected:
	float m_flLastMasterServerPingTime;

	float m_flStateTimeLeft;
	float m_flNextPenalty;
	int m_nPenaltyTakingTeam;
	int m_nPenaltyTakingStartTeam;
	float m_flLastAwayCheckTime;
	bool m_bAdminWantsTimeout;

	CUtlVector<int> m_PlayerRotationMinutes;

	CSDKGameRulesStateInfo		*m_pCurStateInfo;			// Per-state data 
	float						m_flStateTransitionTime;	// Timer for round states
	// State machine handling
	void State_Transition( match_state_t newState );
	void State_Enter(match_state_t newState);	// Initialize the new state.
	void State_Leave(match_state_t newState);	// Cleanup the previous state.
	void State_Think();							// Update the current state.
	static CSDKGameRulesStateInfo* State_LookupInfo(match_state_t state);	// Find the state info for the specified state.

	// State Functions
	void State_WARMUP_Enter();
	void State_WARMUP_Think();
	void State_WARMUP_Leave(match_state_t newState);

	void State_FIRST_HALF_Enter();
	void State_FIRST_HALF_Think();
	void State_FIRST_HALF_Leave(match_state_t newState);

	void State_HALFTIME_Enter();
	void State_HALFTIME_Think();
	void State_HALFTIME_Leave(match_state_t newState);

	void State_SECOND_HALF_Enter();
	void State_SECOND_HALF_Think();
	void State_SECOND_HALF_Leave(match_state_t newState);

	void State_EXTRATIME_INTERMISSION_Enter();
	void State_EXTRATIME_INTERMISSION_Think();
	void State_EXTRATIME_INTERMISSION_Leave(match_state_t newState);

	void State_EXTRATIME_FIRST_HALF_Enter();
	void State_EXTRATIME_FIRST_HALF_Think();
	void State_EXTRATIME_FIRST_HALF_Leave(match_state_t newState);

	void State_EXTRATIME_HALFTIME_Enter();
	void State_EXTRATIME_HALFTIME_Think();
	void State_EXTRATIME_HALFTIME_Leave(match_state_t newState);

	void State_EXTRATIME_SECOND_HALF_Enter();
	void State_EXTRATIME_SECOND_HALF_Think();
	void State_EXTRATIME_SECOND_HALF_Leave(match_state_t newState);

	void State_PENALTIES_INTERMISSION_Enter();
	void State_PENALTIES_INTERMISSION_Think();
	void State_PENALTIES_INTERMISSION_Leave(match_state_t newState);

	void State_PENALTIES_Enter();
	void State_PENALTIES_Think();
	void State_PENALTIES_Leave(match_state_t newState);

	void State_COOLDOWN_Enter();
	void State_COOLDOWN_Think();
	void State_COOLDOWN_Leave(match_state_t newState);

	int m_nFirstHalfKickOffTeam;
	int m_nKickOffTeam;
	int m_nFirstHalfLeftSideTeam;
	CPrecipitation *m_pPrecip;

	void CheckChatText(CBasePlayer *pPlayer, char *text);

	bool m_bMotmVotingPanelShown;
	bool m_bPostMatchStatsPanelShown;

public:

	void SetLeftSideTeam(int team);
	int GetLeftSideTeam();
	int GetRightSideTeam();
	void SetKickOffTeam(int team);
	int GetKickOffTeam();
	void StartMeteringInjuryTime();
	void StopMeteringInjuryTime();
	void ClientSettingsChanged( CBasePlayer *pPlayer );
	void EnableShield(int type, int team, const Vector &pos);
	void DisableShield();
	void SetWeather(PrecipitationType_t type);

	bool ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);

	void SetOffsideLinePositions(float ballPosY, float offsidePlayerPosY, float lastOppPlayerPosY);
	void SetOffsideLinesEnabled(bool enable);

	void SetAdminWantsTimeout(bool state) { m_bAdminWantsTimeout = state; }
	bool AdminWantsTimeout() { return m_bAdminWantsTimeout; }
	
	void SetTimeoutEnd(float time) { m_flTimeoutEnd = time; }

	void ResetMatch();

	void ApplyIntermissionSettings(bool swapTeams);
	bool CheckAutoStart();

#else

public:
	void DrawFieldTeamCrests();
	void DrawOffsideLines();
	void DrawSkyboxOverlay();

private:
	IMaterial *m_pOffsideLineMaterial;

#endif

public:

	float GetTimeoutEnd() { return m_flTimeoutEnd; }

	bool IsIntermissionState();
	bool IsInjuryTime();
	int GetShieldRadius(int team);
	void SetMatchDisplayTimeSeconds(int seconds);
	int GetMatchDisplayTimeSeconds(bool clamped = false, bool ignoreIntermissions = false);

	CNetworkVar(int, m_nShieldType);
	CNetworkVar(int, m_nShieldTeam);
	CNetworkVector(m_vShieldPos);
	CNetworkVar(float, m_flInjuryTimeStart);
	CNetworkVar(int, m_nBallZone);
	CNetworkVar(int, m_nLeftSideTeam);

	CNetworkVar(float, m_flOffsideLineBallPosY);
	CNetworkVar(float, m_flOffsideLineOffsidePlayerPosY);
	CNetworkVar(float, m_flOffsideLineLastOppPlayerPosY);
	CNetworkVar(bool, m_bOffsideLinesEnabled);

	CNetworkVar(float, m_flTimeoutEnd);
};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------

inline CSDKGameRules* SDKGameRules()
{
	return static_cast<CSDKGameRules*>(g_pGameRules);
}

enum
{
	POS_XPOS = 0,
	POS_YPOS,
	POS_TYPE,
	POS_NUMBER
};

enum g_PosNames_t
{
	GK = 0, 
	SWP, LB, RB, CB, LCB, RCB, LWB, RWB,
	LM, RM, DM, CM, AM, LCM, RCM, CDM, CAM,
	LF, RF, CF, ST, SS, LW, RW,
	POS_NAME_COUNT
};

static const int g_nPosKeeper = (1 << GK);
static const int g_nPosDefense = (1 << SWP) + (1 << LB) + (1 << RB) + (1 << CB) + (1 << LCB) + (1 << RCB) + (1 << LWB) + (1 << RWB);
static const int g_nPosMidfield = (1 << LM) + (1 << RM) + (1 << DM) + (1 << CM) + (1 << AM) + (1 << LCM) + (1 << RCM) + (1 << CDM) + (1 << CAM);
static const int g_nPosAttack = (1 << LF) + (1 << RF) + (1 << CF) + (1 << ST) + (1 << SS) + (1 << LW) + (1 << RW);

extern const char g_szPosNames[POS_NAME_COUNT][5];

extern const float g_Positions[11][11][4];

int GetKeeperPosIndex();

#endif // SDK_GAMERULES_H
