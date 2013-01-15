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
	virtual void AddPlayer( CBasePlayer *pPlayer );
	virtual void RemovePlayer( CBasePlayer *pPlayer );
	virtual int  GetNumPlayers( void );
	virtual CBasePlayer *GetPlayer( int iIndex );
	CSDKPlayer *GetPlayerByPosIndex(int posIndex);
	int GetPosNextJoinSeconds(int posIndex);
	void SetPosNextJoinSeconds(int posIndex, int seconds);
	void UnblockAllPos();
	void UpdatePosIndices(bool reset);
	virtual void SetCaptainPosIndex(int posIndex) { m_nCaptainPosIndex = clamp(posIndex, 0, mp_maxplayers.GetInt() - 1); }
	virtual void SetFreekickTakerPosIndex(int posIndex) { m_nFreekickTakerPosIndex = clamp(posIndex, 0, mp_maxplayers.GetInt() - 1); }
	virtual void SetPenaltyTakerPosIndex(int posIndex) { m_nPenaltyTakerPosIndex = clamp(posIndex, 0, mp_maxplayers.GetInt() - 1); }
	virtual void SetCornerTakerPosIndex(int posIndex) { m_nCornerTakerPosIndex = clamp(posIndex, 0, mp_maxplayers.GetInt() - 1); }
	virtual void SetThrowinTakerPosIndex(int posIndex) { m_nThrowinTakerPosIndex = clamp(posIndex, 0, mp_maxplayers.GetInt() - 1); }
	virtual void SetTimeoutsLeft(int amount) { m_nTimeoutsLeft = amount; }
	virtual CSDKPlayer *GetCaptain() { return GetPlayerByPosIndex(m_nCaptainPosIndex); }
	virtual CSDKPlayer *GetFreekickTaker() { return GetPlayerByPosIndex(m_nFreekickTakerPosIndex); }
	virtual CSDKPlayer *GetPenaltyTaker() { return GetPlayerByPosIndex(m_nPenaltyTakerPosIndex); }
	virtual int GetTimeoutsLeft() { return m_nTimeoutsLeft; }
	virtual void FindNewCaptain();
	virtual void AddMatchEvent(match_state_t matchState, int seconds, match_event_t event, const char *player);

	//-----------------------------------------------------------------------------
	// Scoring
	//-----------------------------------------------------------------------------
	virtual void AddGoal();
	virtual void SetGoals( int goals );
	virtual int  GetGoals( void );

	void AwardAchievement( int iAchievement );

public:
	CUtlVector< CBasePlayer * >		m_aPlayers;
	int m_PosIndexPlayerIndices[11];

	// Data
	CNetworkString( m_szServerKitName, MAX_TEAM_NAME_LENGTH );
	CNetworkString( m_szTeamCode, MAX_TEAMCODE_LENGTH );
	CNetworkString( m_szShortTeamName, MAX_SHORTTEAMNAME_LENGTH );
	CNetworkVar( int, m_iTeamNum );
	CNetworkVar( int, m_nGoals );
	CNetworkVar( int, m_nPossession );
	CNetworkVar( int, m_nPenaltyGoals );
	CNetworkVar( int, m_nPenaltyGoalBits );
	CNetworkVar( int, m_nPenaltyRound );
	CNetworkVar( int, m_nTimeoutsLeft );

	CNetworkVar( int, m_nCaptainPosIndex );
	CNetworkVar( int, m_nFreekickTakerPosIndex );
	CNetworkVar( int, m_nPenaltyTakerPosIndex );
	CNetworkVar( int, m_nCornerTakerPosIndex );
	CNetworkVar( int, m_nThrowinTakerPosIndex );

	int		m_iDeaths;

	// Spawnpoints
	int		m_iLastSpawn;		// Index of the last spawnpoint used

	float	m_flPossessionTime;

	CNetworkVector(m_vCornerLeft);
	CNetworkVector(m_vCornerRight);
	CNetworkVector(m_vGoalkickLeft);
	CNetworkVector(m_vGoalkickRight);
	CNetworkVector(m_vPenalty);
	CNetworkVector(m_vPenBoxMin);
	CNetworkVector(m_vPenBoxMax);
	CNetworkVar(int, m_nForward);
	CNetworkVar(int, m_nRight);

	Vector m_vPlayerSpawns[11];

	Vector GetSpotPos(const char *name);
	void InitFieldSpots(int team);
	void ResetStats();

	char m_szMatchEventPlayersMemory[MAX_MATCH_EVENTS][MAX_MATCH_EVENT_PLAYER_NAME_LENGTH];
	CNetworkArray( string_t, m_szMatchEventPlayers, MAX_MATCH_EVENTS );
	CNetworkArray(int, m_eMatchEventTypes, MAX_MATCH_EVENTS);
	CNetworkArray(int, m_eMatchEventMatchStates, MAX_MATCH_EVENTS);
	CNetworkArray(int, m_nMatchEventSeconds, MAX_MATCH_EVENTS);
	int m_nMatchEventIndex;

	CNetworkArray(int, m_PosNextJoinSeconds, 11);

};

extern CUtlVector< CTeam * > g_Teams;
extern CTeam *GetGlobalTeam( int iIndex );
extern int GetNumberOfTeams( void );

#endif // TEAM_H
