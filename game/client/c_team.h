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

	// Data Handling
	virtual int		GetTeamNumber( void ) const;
	virtual bool	Get_IsClubTeam( void );
	virtual bool	Get_IsRealTeam( void );
	virtual bool	Get_HasTeamCrest( void );
	virtual char	*Get_TeamCode( void );
	virtual char	*Get_ShortTeamName( void );
	virtual char	*Get_FullTeamName( void );
	virtual char	*Get_KitName( void );
	virtual Color	&Get_HudKitColor( void );
	virtual Color	&Get_PrimaryKitColor( void );
	virtual Color	&Get_SecondaryKitColor( void );
	virtual int		Get_Goals( void );
	virtual int		Get_Ping( void );

	virtual int		Get_Possession();

	// Player Handling
	virtual int		Get_Number_Players( void );
	virtual bool	ContainsPlayer( int iPlayerIndex );
	C_BasePlayer*	GetPlayer( int idx );

	// for shared code, use the same function name
	virtual int		GetNumPlayers( void ) { return Get_Number_Players(); }

	void	RemoveAllPlayers();

	void	SetKitName(const char *pKitName);

// IClientThinkable overrides.
public:

	virtual	void				ClientThink();


public:

	// Data received from the server
	CUtlVector< int > m_aPlayers;
	CTeamKitInfo *m_pTeamKitInfo;
	int		m_iTeamNum;
	char	m_szKitName[MAX_KITNAME_LENGTH];
	int		m_nGoals;
	int		m_nPossession;

	// Data for the scoreboard
	int		m_iPing;
	int		m_iPacketloss;

	Vector	m_vCornerLeft;
	Vector	m_vCornerRight;
	Vector	m_vGoalkickLeft;
	Vector	m_vGoalkickRight;
	Vector	m_vPenalty;
	Vector	m_vPenBoxMin;
	Vector	m_vPenBoxMax;
	int		m_nForward;
	int		m_nRight;
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
