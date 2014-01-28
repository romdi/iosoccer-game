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

	virtual void SetFreekickTakerPosIndex(int posIndex) { m_nFreekickTakerPosIndex = clamp(posIndex, -1, 10); }
	virtual int GetFreekickTakerPosIndex() { return m_nFreekickTakerPosIndex; }

	virtual void SetPenaltyTakerPosIndex(int posIndex) { m_nPenaltyTakerPosIndex = clamp(posIndex, -1, 10); }
	virtual int GetPenaltyTakerPosIndex() { return m_nPenaltyTakerPosIndex; }

	virtual void SetLeftCornerTakerPosIndex(int posIndex) { m_nLeftCornerTakerPosIndex = clamp(posIndex, -1, 10); }
	virtual int GetLeftCornerTakerPosIndex() { return m_nLeftCornerTakerPosIndex; }

	virtual void SetRightCornerTakerPosIndex(int posIndex) { m_nRightCornerTakerPosIndex = clamp(posIndex, -1, 10); }
	virtual int GetRightCornerTakerPosIndex() { return m_nRightCornerTakerPosIndex; }

	virtual void SetTimeoutsLeft(int amount) { m_nTimeoutsLeft = amount; }
	virtual int GetTimeoutsLeft() { return m_nTimeoutsLeft; }

	virtual void SetTimeoutTimeLeft(int seconds) { m_nTimeoutTimeLeft = seconds; }
	virtual int GetTimeoutTimeLeft() { return m_nTimeoutTimeLeft; }

	virtual CSDKPlayer *GetCaptain() { return GetPlayerByPosIndex(m_nCaptainPosIndex); }
	virtual CSDKPlayer *GetFreekickTaker() { return GetPlayerByPosIndex(m_nFreekickTakerPosIndex); }
	virtual CSDKPlayer *GetPenaltyTaker() { return GetPlayerByPosIndex(m_nPenaltyTakerPosIndex); }
	virtual CSDKPlayer *GetLeftCornerTaker() { return GetPlayerByPosIndex(m_nLeftCornerTakerPosIndex); }
	virtual CSDKPlayer *GetRightCornerTaker() { return GetPlayerByPosIndex(m_nRightCornerTakerPosIndex); }
	virtual void FindNewCaptain();
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
	CNetworkVar( int, m_nFreekickTakerPosIndex );
	CNetworkVar( int, m_nPenaltyTakerPosIndex );
	CNetworkVar( int, m_nLeftCornerTakerPosIndex );
	CNetworkVar( int, m_nRightCornerTakerPosIndex );

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
	CNetworkVar(int, m_DistanceCovered);
	CNetworkVar(int, m_Passes);
	CNetworkVar(int, m_FreeKicks);
	CNetworkVar(int, m_Penalties);
	CNetworkVar(int, m_Corners);
	CNetworkVar(int, m_ThrowIns);
	CNetworkVar(int, m_KeeperSaves);
	CNetworkVar(int, m_GoalKicks);
	CNetworkVar(int, m_Ping);
	CNetworkVar(int, m_Rating);

	Formation *GetFormation();
	int GetFormationIndex() { return m_nFormationIndex; }
	void SetFormationIndex(int index, bool silent);
	int GetQuickTactic() { return m_eQuickTactic; }
	void SetQuickTactic(QuickTactics_t quickTactic) { m_eQuickTactic = quickTactic; }

private:

	CNetworkVar(int, m_nFormationIndex);
	CNetworkVar(int, m_eQuickTactic);
};

extern CUtlVector< CTeam * > g_Teams;
extern CTeam *GetGlobalTeam( int iIndex );
extern int GetNumberOfTeams( void );

#endif // TEAM_H
