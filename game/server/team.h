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

#define MAX_MATCH_EVENTS 16
#define MAX_MATCH_EVENT_PLAYER_NAME_LENGTH 64

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
	virtual void SetCaptain(CSDKPlayer *pCaptain) { m_pCaptain = pCaptain; }
	virtual void SetFreekickTaker(CSDKPlayer *pFreekickTaker) { m_pFreekickTaker = pFreekickTaker; }
	virtual void SetPenaltyTaker(CSDKPlayer *pPenaltyTaker) { m_pPenaltyTaker = pPenaltyTaker; }
	virtual void SetCornerTaker(CSDKPlayer *pCornerTaker) { m_pCornerTaker = pCornerTaker; }
	virtual void SetThrowinTaker(CSDKPlayer *pThrowinTaker) { m_pThrowinTaker = pThrowinTaker; }
	virtual void SetTimeoutsLeft(int amount) { m_nTimeoutsLeft = amount; }
	virtual CSDKPlayer *GetCaptain() { return m_pCaptain; }
	virtual int GetTimeoutsLeft() { return m_nTimeoutsLeft; }
	virtual void FindNewCaptain();
	virtual void AddMatchEvent(int seconds, match_event_t event, const char *player);

	//-----------------------------------------------------------------------------
	// Scoring
	//-----------------------------------------------------------------------------
	virtual void AddGoal();
	virtual void SetGoals( int goals );
	virtual int  GetGoals( void );

	void AwardAchievement( int iAchievement );

public:
	CUtlVector< CBasePlayer * >		m_aPlayers;

	// Data
	CNetworkString( m_szServerKitName, MAX_TEAM_NAME_LENGTH );
	CNetworkVar( int, m_iTeamNum );
	CNetworkVar( int, m_nGoals );
	CNetworkVar( int, m_nPossession );
	CNetworkVar( int, m_nPenaltyGoals );
	CNetworkVar( int, m_nPenaltyGoalBits );
	CNetworkVar( int, m_nPenaltyRound );
	CNetworkVar( int, m_nTimeoutsLeft );

	CNetworkHandle( CSDKPlayer, m_pCaptain );
	CNetworkHandle( CSDKPlayer, m_pFreekickTaker );
	CNetworkHandle( CSDKPlayer, m_pPenaltyTaker );
	CNetworkHandle( CSDKPlayer, m_pCornerTaker );
	CNetworkHandle( CSDKPlayer, m_pThrowinTaker );

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
	CNetworkArray(int, m_nMatchEventSeconds, MAX_MATCH_EVENTS);
	int m_nMatchEventIndex;
};

extern CUtlVector< CTeam * > g_Teams;
extern CTeam *GetGlobalTeam( int iIndex );
extern int GetNumberOfTeams( void );

#endif // TEAM_H
