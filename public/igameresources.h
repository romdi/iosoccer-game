//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: IGameResources interface
//
// $NoKeywords: $
//=============================================================================//

#ifndef IGAMERESOURCES_H
#define IGAMERESOURCES_H

class Color;
class Vector;


abstract_class IGameResources
{
public:
	virtual	~IGameResources() {};

	// Player data access
	virtual bool	IsConnected( int index ) = 0;
	virtual bool	IsFakePlayer( int index ) = 0;
	virtual bool	IsLocalPlayer( int index ) = 0;

	virtual const char *GetPlayerName( int index ) = 0;
	virtual const char *GetSteamName( int index ) = 0;
	virtual const char *GetClubName( int index ) = 0;
	virtual const char *GetNationalTeamName( int index ) = 0;
	virtual int		GetCountryIndex( int index ) = 0;
	virtual int		GetPing( int index ) = 0;
//	virtual int		GetPacketloss( int index ) = 0;
	virtual int		GetTeam( int index ) = 0;
	virtual int		GetSpecTeam( int index ) = 0;
	virtual const Color&	GetPlayerColor( int index ) = 0;

	//ios
	virtual int		GetRedCards( int index ) = 0;
	virtual int		GetYellowCards( int index ) = 0;
	virtual int		GetFouls( int index ) = 0;
	virtual int		GetFoulsSuffered( int index ) = 0;
	virtual int		GetSlidingTackles( int index ) = 0;
	virtual int		GetSlidingTacklesCompleted( int index ) = 0;
	virtual int		GetGoalsConceded( int index ) = 0;
	virtual int		GetShots( int index ) = 0;
	virtual int		GetShotsOnGoal( int index ) = 0;
	virtual int		GetPassesCompleted( int index ) = 0;
	virtual int		GetInterceptions( int index ) = 0;
	virtual int		GetOffsides( int index ) = 0;
	virtual int		GetGoals( int index ) = 0;
	virtual int		GetOwnGoals( int index ) = 0;
	virtual int		GetAssists( int index ) = 0;
	virtual int		GetPossession( int index ) = 0;
	virtual int		GetDistanceCovered( int index ) = 0;
	virtual int		GetPasses( int index ) = 0;
	virtual int		GetFreeKicks( int index ) = 0;
	virtual int		GetPenalties( int index ) = 0;
	virtual int		GetCorners( int index ) = 0;
	virtual int		GetThrowIns( int index ) = 0;
	virtual int		GetKeeperSaves( int index ) = 0;
	virtual int		GetKeeperSavesCaught( int index ) = 0;
	virtual int		GetGoalKicks( int index ) = 0;
	virtual int		GetRatings( int index ) = 0;
	virtual int		GetShirtNumber( int iIndex ) = 0;
	virtual int		GetTeamPosType( int iIndex ) = 0;
	virtual int		GetTeamPosIndex( int iIndex ) = 0;
	virtual int		GetTeamToJoin( int iIndex ) = 0;
	virtual int		GetTeamPosIndexToJoin( int iIndex ) = 0;
	virtual int		GetNextCardJoin( int iIndex ) = 0;
	virtual bool	IsAway( int iIndex ) = 0;
};

extern IGameResources *GameResources( void ); // singelton accessor

#endif

