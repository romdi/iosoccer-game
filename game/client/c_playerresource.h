//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_PLAYERRESOURCE_H
#define C_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "const.h"
#include "c_baseentity.h"
#include <igameresources.h>

class C_PlayerResource : public C_BaseEntity, public IGameResources
{
	DECLARE_CLASS( C_PlayerResource, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

					C_PlayerResource();
	virtual			~C_PlayerResource();

public : // IGameResources intreface

	// Team data access 
	virtual int		GetTeamGoals( int index );
	virtual bool	IsClubTeam( int index );
	virtual bool	IsRealTeam( int index );
	virtual bool	HasTeamCrest( int index );
	virtual const char *GetTeamCode( int index );
	virtual const char *GetShortTeamName( int index );
	virtual const char *GetFullTeamName( int index );
	virtual const char *GetTeamKitName( int index );
	virtual Color &GetHudTeamKitColor(int index);
	virtual Color &GetPrimaryTeamKitColor(int index);
	virtual Color &GetSecondaryTeamKitColor(int index);
	virtual const Color &GetTeamColor( int index );
	virtual const Color &GetPlayerColor( int index );

	// Player data access
	virtual bool	IsConnected( int index );
	virtual bool	IsFakePlayer( int index );
	virtual bool	IsLocalPlayer( int index  );
	virtual bool	IsHLTV(int index);

	virtual const char *GetPlayerName( int index );
	virtual const char *GetSteamName( int index );
	virtual const char *GetClubName( int index );
	virtual int		GetCountryName( int index );
	virtual int		GetPing( int index );
//	virtual int		GetPacketloss( int index );
	virtual int		GetTeam( int index );

	//ios
	virtual int		GetRedCards( int index );
	virtual int		GetYellowCards( int index );
	virtual int		GetFouls( int index );
	virtual int		GetFoulsSuffered( int index );
	virtual int		GetSlidingTackles( int index );
	virtual int		GetSlidingTacklesCompleted( int index );
	virtual int		GetGoalsConceded( int index );
	virtual int		GetShots( int index );
	virtual int		GetShotsOnGoal( int index );
	virtual int		GetPassesCompleted( int index );
	virtual int		GetInterceptions( int index );
	virtual int		GetOffsides( int index );
	virtual int		GetGoals( int index );
	virtual int		GetOwnGoals( int index );
	virtual int		GetAssists( int index );
	virtual int		GetPossession( int index );
	virtual int		GetDistanceCovered( int index );
	virtual int		GetPasses( int index );
	virtual int		GetFreeKicks( int index );
	virtual int		GetPenalties( int index );
	virtual int		GetCorners( int index );
	virtual int		GetThrowIns( int index );
	virtual int		GetKeeperSaves( int index );
	virtual int		GetGoalKicks( int index );
	virtual int		GetTeamPosNum( int iIndex );
	virtual int		GetTeamPosType( int iIndex );
	virtual int		GetTeamPosIndex( int iIndex );
	virtual int		GetTeamToJoin( int iIndex );
	virtual int		GetTeamPosIndexToJoin( int iIndex );
	virtual int		GetNextCardJoin( int iIndex );
	virtual bool	IsAway( int iIndex );

	virtual void ClientThink();
	virtual	void	OnDataChanged(DataUpdateType_t updateType);

protected:
	void	UpdatePlayerName( int slot );

	// Data for each player that's propagated to all clients
	// Stored in individual arrays so they can be sent down via datatables
	string_t	m_szName[MAX_PLAYERS+1];
	char	m_szClubNames[MAX_PLAYERS+1][MAX_CLUBNAME_LENGTH];
	int		m_CountryNames[MAX_PLAYERS+1];
	int		m_iPing[MAX_PLAYERS+1];
	bool	m_bConnected[MAX_PLAYERS+1];
	int		m_iTeam[MAX_PLAYERS+1];
	
	//ios
	int		m_RedCards[MAX_PLAYERS+1];
	int		m_YellowCards[MAX_PLAYERS+1];
	int		m_Fouls[MAX_PLAYERS+1];
	int		m_FoulsSuffered[MAX_PLAYERS+1];
	int		m_SlidingTackles[MAX_PLAYERS+1];
	int		m_SlidingTacklesCompleted[MAX_PLAYERS+1];
	int		m_GoalsConceded[MAX_PLAYERS+1];
	int		m_Shots[MAX_PLAYERS+1];
	int		m_ShotsOnGoal[MAX_PLAYERS+1];
	int		m_PassesCompleted[MAX_PLAYERS+1];
	int		m_Interceptions[MAX_PLAYERS+1];
	int		m_Offsides[MAX_PLAYERS+1];
	int		m_Goals[MAX_PLAYERS+1];
	int		m_OwnGoals[MAX_PLAYERS+1];
	int		m_Assists[MAX_PLAYERS+1];
	int		m_Possession[MAX_PLAYERS+1];
	int		m_DistanceCovered[MAX_PLAYERS+1];
	int		m_Passes[MAX_PLAYERS+1];
	int		m_FreeKicks[MAX_PLAYERS+1];
	int		m_Penalties[MAX_PLAYERS+1];
	int		m_Corners[MAX_PLAYERS+1];
	int		m_ThrowIns[MAX_PLAYERS+1];
	int		m_KeeperSaves[MAX_PLAYERS+1];
	int		m_GoalKicks[MAX_PLAYERS+1];
	int		m_TeamPosIndex[MAX_PLAYERS+1];
	int		m_TeamPosNum[MAX_PLAYERS+1];

	int		m_TeamToJoin[MAX_PLAYERS+1];
	int		m_TeamPosIndexToJoin[MAX_PLAYERS+1];
	int		m_NextCardJoin[MAX_PLAYERS+1];
	bool	m_IsAway[MAX_PLAYERS+1];
};

extern C_PlayerResource *g_PR;

#endif // C_PLAYERRESOURCE_H
