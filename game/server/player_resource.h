//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYER_RESOURCE_H
#define PLAYER_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

class CPlayerResource : public CBaseEntity
{
	DECLARE_CLASS( CPlayerResource, CBaseEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual	int	 ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_DONT_SAVE; }
	virtual void ResourceThink( void );
	virtual void UpdatePlayerData( void );
	virtual int  UpdateTransmitState(void);

protected:
	// Data for each player that's propagated to all clients
	// Stored in individual arrays so they can be sent down via datatables
	CNetworkArray( int, m_iPing, MAX_PLAYERS+1 );
	CNetworkArray( int, m_bConnected, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iTeam, MAX_PLAYERS+1 );
	CNetworkArray( int, m_nSpecTeam, MAX_PLAYERS+1 );

	//ios
	CNetworkArray( int, m_RedCards, MAX_PLAYERS+1 );
	CNetworkArray( int, m_YellowCards, MAX_PLAYERS+1 );
	CNetworkArray( int, m_Fouls, MAX_PLAYERS+1 );
	CNetworkArray( int, m_FoulsSuffered, MAX_PLAYERS+1 );
	CNetworkArray( int, m_SlidingTackles, MAX_PLAYERS+1 );
	CNetworkArray( int, m_SlidingTacklesCompleted, MAX_PLAYERS+1 );
	CNetworkArray( int, m_GoalsConceded, MAX_PLAYERS+1 );
	CNetworkArray( int, m_Shots, MAX_PLAYERS+1 );
	CNetworkArray( int, m_ShotsOnGoal, MAX_PLAYERS+1 );
	CNetworkArray( int, m_PassesCompleted, MAX_PLAYERS+1 );
	CNetworkArray( int, m_Interceptions, MAX_PLAYERS+1 );
	CNetworkArray( int, m_Offsides, MAX_PLAYERS+1 );
	CNetworkArray( int, m_Goals, MAX_PLAYERS+1 );
	CNetworkArray( int, m_OwnGoals, MAX_PLAYERS+1 );
	CNetworkArray( int, m_Assists, MAX_PLAYERS+1 );
	CNetworkArray( int, m_Possession, MAX_PLAYERS+1 );
	CNetworkArray( int, m_DistanceCovered, MAX_PLAYERS+1 );
	CNetworkArray( int, m_Passes, MAX_PLAYERS+1 );
	CNetworkArray( int, m_FreeKicks, MAX_PLAYERS+1 );
	CNetworkArray( int, m_Penalties, MAX_PLAYERS+1 );
	CNetworkArray( int, m_Corners, MAX_PLAYERS+1 );
	CNetworkArray( int, m_ThrowIns, MAX_PLAYERS+1 );
	CNetworkArray( int, m_KeeperSaves, MAX_PLAYERS+1 );
	CNetworkArray( int, m_GoalKicks, MAX_PLAYERS+1 );
	CNetworkArray( int, m_Ratings, MAX_PLAYERS+1 );
	CNetworkArray( int, m_TeamPosIndex, MAX_PLAYERS+1 );
	CNetworkArray( int, m_TeamPosNum, MAX_PLAYERS+1 );
	CNetworkArray( int, m_NextCardJoin, MAX_PLAYERS+1 );
	CNetworkArray( bool, m_IsAway, MAX_PLAYERS+1 );
	CNetworkArray( int, m_TeamToJoin, MAX_PLAYERS+1 );
	CNetworkArray( int, m_TeamPosIndexToJoin, MAX_PLAYERS+1 );
	
	CNetworkArray( string_t, m_szPlayerNames, MAX_PLAYERS+1 );
	CNetworkArray( string_t, m_szClubNames, MAX_PLAYERS+1 );
	CNetworkArray( int, m_CountryIndices, MAX_PLAYERS+1 );
		
	int	m_nUpdateCounter;
};

extern CPlayerResource *g_pPlayerResource;

#endif // PLAYER_RESOURCE_H
