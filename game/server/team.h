//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEAM_H
#define TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "utlvector.h"
#include "sdk_player.h"
#include "ios_teamkit_parse.h"

#define MAX_MATCH_EVENTS 16
#define MAX_MATCH_EVENT_PLAYER_NAME_LENGTH 32

class CBasePlayer;
class CTeamSpawnPoint;

struct LastPlayerCoords
{
	Vector coords;
	float leaveTime;
};

class CTeamMatchPeriodData
{
public:
	int		m_nRedCards;
	int		m_nYellowCards;
	int		m_nFouls;
	int		m_nFoulsSuffered;
	int		m_nSlidingTackles;
	int		m_nSlidingTacklesCompleted;
	int		m_nGoalsConceded;
	int		m_nShots;
	int		m_nShotsOnGoal;
	int		m_nPassesCompleted;
	int		m_nInterceptions;
	int		m_nOffsides;
	int		m_nGoals;
	int		m_nOwnGoals;
	int		m_nAssists;
	float	m_flPossessionTime;
	int		m_nPossession;
	int		m_nTurnovers;
	int		m_nDistanceCovered;
	int		m_nPasses;
	int		m_nFreeKicks;
	int		m_nPenalties;
	int		m_nCorners;
	int		m_nThrowIns;
	int		m_nKeeperSaves;
	int		m_nKeeperSavesCaught;
	int		m_nGoalKicks;
	int		m_nRating;
	float	m_flExactDistanceCovered;

	char m_szMatchPeriodName[32];
	int m_nAnnouncedInjuryTimeSeconds;
	int m_nActualInjuryTimeSeconds;

	virtual void ResetData();

	CTeamMatchPeriodData(const char *szMatchPeriodName) { Q_strncpy(m_szMatchPeriodName, szMatchPeriodName, sizeof(m_szMatchPeriodName)); }
};

class CTeam : public CBaseEntity
{
	DECLARE_CLASS( CTeam, CBaseEntity );
public:
	CTeam( void );
	virtual ~CTeam( void );

	DECLARE_SERVERCLASS();

	virtual void Precache( void ) { return; };

	virtual void Think( void );
	virtual int  UpdateTransmitState( void );

	//-----------------------------------------------------------------------------
	// Initialization
	//-----------------------------------------------------------------------------
	virtual void		Init( const char *pName, int iNumber );

	virtual int		GetTeamNumber( void ) const;
	virtual int		GetOppTeamNumber( void ) const;
	CTeam			*GetOppTeam( void ) const;
	virtual const char *GetTeamCode( void );
	virtual const char *GetShortTeamName( void );
	virtual void	SetTeamNumber(int teamNum);
	void			SetKitName(const char *pName);
	void			SetTeamCode(const char *pCode);
	void			SetShortTeamName(const char *pName);

	//-----------------------------------------------------------------------------
	// Data Handling
	//-----------------------------------------------------------------------------
	virtual const char *GetKitName( void );
	virtual void		UpdateClientData( CBasePlayer *pPlayer );
	virtual bool		ShouldTransmitToPlayer( CBasePlayer* pRecipient, CBaseEntity* pEntity );

	//-----------------------------------------------------------------------------
	// Players
	//-----------------------------------------------------------------------------
	virtual void AddPlayer( CBasePlayer *pPlayer, int posIndex );
	virtual void RemovePlayer( CBasePlayer *pPlayer );
	virtual int  GetNumPlayers( void );
	virtual CBasePlayer *GetPlayer( int iIndex );
	CSDKPlayer *GetPlayerByPosIndex(int posIndex);
	int GetPosIndexByPosType(PosTypes_t posType);
	CSDKPlayer *GetPlayerByPosType(PosTypes_t posType);
	int GetPosNextJoinSeconds(int posIndex);
	void SetPosNextJoinSeconds(int posIndex, int seconds);
	void UnblockAllPos();
	void UpdatePosIndices(bool reset);
	Vector GetLastPlayerCoordsByPosIndex(int posIndex);

	virtual void SetCaptainPosIndex(int posIndex) { m_nCaptainPosIndex = clamp(posIndex, -1, 10); }
	virtual int GetCaptainPosIndex() { return m_nCaptainPosIndex; }

	virtual void SetTimeoutsLeft(int amount) { m_nTimeoutsLeft = amount; }
	virtual int GetTimeoutsLeft() { return m_nTimeoutsLeft; }

	virtual void SetTimeoutTimeLeft(int seconds) { m_nTimeoutTimeLeft = seconds; }
	virtual int GetTimeoutTimeLeft() { return m_nTimeoutTimeLeft; }

	virtual CSDKPlayer *GetCaptain() { return GetPlayerByPosIndex(m_nCaptainPosIndex); }
	virtual void AddMatchEvent(match_period_t matchPeriod, int seconds, match_event_t event, const char *text);

	//-----------------------------------------------------------------------------
	// Scoring
	//-----------------------------------------------------------------------------
	//virtual void AddGoal();
	virtual void SetGoals( int goals );
	virtual int  GetGoals( void );
	virtual void SetPossession( int possession );
	virtual int  GetPossession( void );

	void AwardAchievement( int iAchievement );

public:
	CUtlVector< CBasePlayer * >		m_aPlayers;
	int m_PosIndexPlayerIndices[11];
	LastPlayerCoords m_LastPlayerCoordsByPosIndex[11];

	// Data
	CNetworkString( m_szServerKitName, MAX_TEAM_NAME_LENGTH );
	CNetworkString( m_szServerCode, MAX_TEAMCODE_LENGTH );
	CNetworkString( m_szServerShortName, MAX_SHORTTEAMNAME_LENGTH );
	CNetworkVar( int, m_iTeamNum );
	CNetworkVar( int, m_nPenaltyGoals );
	CNetworkVar( int, m_nPenaltyGoalBits );
	CNetworkVar( int, m_nPenaltyRound );
	CNetworkVar( int, m_nTimeoutsLeft );
	CNetworkVar( int, m_nTimeoutTimeLeft );

	CNetworkVar( int, m_nCaptainPosIndex );

	int		m_iDeaths;

	// Spawnpoints
	int		m_iLastSpawn;		// Index of the last spawnpoint used

	float	m_flPossessionTime;
	float	m_flExactDistanceCovered;

	CNetworkVector(m_vCornerLeft);
	CNetworkVector(m_vCornerRight);
	CNetworkVector(m_vGoalkickLeft);
	CNetworkVector(m_vGoalkickRight);
	CNetworkVector(m_vPenalty);
	CNetworkVector(m_vGoalCenter);
	CNetworkVector(m_vPenBoxMin);
	CNetworkVector(m_vPenBoxMax);
	CNetworkVector(m_vSixYardBoxMin);
	CNetworkVector(m_vSixYardBoxMax);
	CNetworkVar(int, m_nForward);
	CNetworkVar(int, m_nRight);

	Vector GetSpotPos(const char *name);
	void InitFieldSpots(int team);
	void ResetStats();

	char m_szMatchEventPlayersMemory[MAX_MATCH_EVENTS][MAX_MATCH_EVENT_PLAYER_NAME_LENGTH];
	CNetworkArray( string_t, m_szMatchEventPlayers, MAX_MATCH_EVENTS );
	CNetworkArray(int, m_eMatchEventTypes, MAX_MATCH_EVENTS);
	CNetworkArray(int, m_eMatchEventMatchPeriods, MAX_MATCH_EVENTS);
	CNetworkArray(int, m_nMatchEventSeconds, MAX_MATCH_EVENTS);
	int m_nMatchEventIndex;

	CNetworkArray(int, m_PosNextJoinSeconds, 11);

	CNetworkVar(int, m_RedCards);
	CNetworkVar(int, m_YellowCards);
	CNetworkVar(int, m_Fouls);
	CNetworkVar(int, m_FoulsSuffered);
	CNetworkVar(int, m_SlidingTackles);
	CNetworkVar(int, m_SlidingTacklesCompleted);
	CNetworkVar(int, m_GoalsConceded);
	CNetworkVar(int, m_Shots);
	CNetworkVar(int, m_ShotsOnGoal);
	CNetworkVar(int, m_PassesCompleted);
	CNetworkVar(int, m_Interceptions);
	CNetworkVar(int, m_Offsides);
	CNetworkVar(int, m_Goals);
	CNetworkVar(int, m_OwnGoals);
	CNetworkVar(int, m_Assists);
	CNetworkVar(int, m_Possession);
	CNetworkVar(int, m_Turnovers);
	CNetworkVar(int, m_DistanceCovered);
	CNetworkVar(int, m_Passes);
	CNetworkVar(int, m_FreeKicks);
	CNetworkVar(int, m_Penalties);
	CNetworkVar(int, m_Corners);
	CNetworkVar(int, m_ThrowIns);
	CNetworkVar(int, m_KeeperSaves);
	CNetworkVar(int, m_KeeperSavesCaught);
	CNetworkVar(int, m_GoalKicks);
	CNetworkVar(int, m_Ping);
	CNetworkVar(int, m_Rating);

	Formation *GetFormation();
	int GetFormationIndex() { return m_nFormationIndex; }
	void SetFormationIndex(int index, bool silent);

	CUtlVector<CTeamMatchPeriodData *> m_MatchPeriodData;

	CTeamMatchPeriodData *GetMatchPeriodData() { return m_MatchPeriodData.Tail(); }

private:

	CNetworkVar(int, m_nFormationIndex);
};

extern CUtlVector< CTeam * > g_Teams;
extern CTeam *GetGlobalTeam( int iIndex );
extern int GetNumberOfTeams( void );

#endif // TEAM_H
