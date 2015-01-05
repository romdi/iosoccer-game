//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side CTeam class
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TEAM_H
#define C_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "utlvector.h"
#include "client_thinklist.h"
#include "ios_teamkit_parse.h"
#include "c_sdk_player.h"
#include "sdk_gamerules.h"

#define MAX_MATCH_EVENTS 16
#define MAX_MATCH_EVENT_PLAYER_NAME_LENGTH 64

class C_BasePlayer;

class C_Team : public C_BaseEntity
{
	DECLARE_CLASS( C_Team, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

					C_Team();
	virtual			~C_Team();

	virtual void	PreDataUpdate( DataUpdateType_t updateType );
	virtual void	Spawn();
	// Data Handling
	virtual int		GetTeamNumber( void ) const;
	virtual int		GetOppTeamNumber( void ) const;
	C_Team			*GetOppTeam( void ) const;
	virtual bool	IsClub( void );
	virtual bool	IsReal( void );
	virtual bool	HasCrest( void );
	virtual char	*GetCode( void );
	virtual char	*GetShortName( void );
	virtual char	*GetFullName( void );
	virtual char	*GetKitName( void );
	virtual char	*GetKitFolderName( void );
	virtual char	*GetFolderName( void );
	virtual Color	&GetHudKitColor( void );
	virtual Color	&GetPrimaryKitColor( void );
	virtual Color	&GetSecondaryKitColor( void );
	virtual CTeamKitInfo *GetKitInfo();
	virtual int		GetGoals( void );
	virtual int		GetPing( void );

	virtual int		GetPossession();

	virtual int	GetCaptainPosIndex() { return m_nCaptainPosIndex; }

	virtual int		GetTimeoutsLeft() { return m_nTimeoutsLeft; }
	virtual int		GetTimeoutTimeLeft() { return m_nTimeoutTimeLeft; }

	// Player Handling
	virtual bool	ContainsPlayer( int iPlayerIndex );
	C_BasePlayer*	GetPlayer( int idx );

	// for shared code, use the same function name
	virtual int		GetNumPlayers( void ) { return m_aPlayers.Count(); }

	void	RemoveAllPlayers();

	void	SetKitName(const char *pKitName);

// IClientThinkable overrides.
public:

	virtual	void				ClientThink();


public:

	bool	m_bKitDownloadFinished;
	char	m_szDownloadKitName[MAX_KITNAME_LENGTH];

	// Data received from the server
	CUtlVector< int > m_aPlayers;
	CTeamKitInfo *m_pKitInfo;
	int		m_iTeamNum;
	char	m_szServerKitName[MAX_KITNAME_LENGTH];
	char	m_szServerCode[MAX_TEAMCODE_LENGTH];
	char	m_szServerShortName[MAX_SHORTTEAMNAME_LENGTH];
	int		m_nPenaltyGoals;
	int		m_nPenaltyGoalBits;
	int		m_nPenaltyRound;
	int		m_nTimeoutsLeft;
	int		m_nTimeoutTimeLeft;

	int m_nCaptainPosIndex;

	// Data for the scoreboard
	int		m_iPing;
	int		m_iPacketloss;

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
	int		m_nForward;
	int		m_nRight;

	char m_szMatchEventPlayers[MAX_MATCH_EVENTS][MAX_MATCH_EVENT_PLAYER_NAME_LENGTH];
	int m_eMatchEventTypes[MAX_MATCH_EVENTS];
	int m_eMatchEventMatchPeriods[MAX_MATCH_EVENTS];
	int m_nMatchEventSeconds[MAX_MATCH_EVENTS];

	int m_PosNextJoinSeconds[11];

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
	CNetworkVar(int, m_KeeperSavesCaught);
	CNetworkVar(int, m_GoalKicks);
	CNetworkVar(int, m_Ping);
	CNetworkVar(int, m_Rating);

	Formation *GetFormation();

private:

	CNetworkVar(int, m_nFormationIndex);
};


// Global list of client side team entities
extern CUtlVector< C_Team * > g_Teams;

// Global team handling functions
C_Team *GetLocalTeam( void );
C_Team *GetGlobalTeam( int iTeamNumber );
C_Team *GetPlayersTeam( int iPlayerIndex );
C_Team *GetPlayersTeam( C_BasePlayer *pPlayer );
bool ArePlayersOnSameTeam( int iPlayerIndex1, int iPlayerIndex2 );
extern int GetNumberOfTeams( void );

#endif // C_TEAM_H
