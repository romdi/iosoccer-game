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

	// Team data access 
	virtual const char		*GetTeamName( int index ) = 0;
	virtual int				GetTeamScore( int index ) = 0;
	virtual const Color&	GetTeamColor( int index ) = 0;

	// Player data access
	virtual bool	IsConnected( int index ) = 0;
	virtual bool	IsAlive( int index ) = 0;
	virtual bool	IsFakePlayer( int index ) = 0;
	virtual bool	IsLocalPlayer( int index ) = 0;

	virtual const char *GetPlayerName( int index ) = 0;
	virtual const char *GetClubName( int index ) = 0;
	virtual int		GetPlayerScore( int index ) = 0;
	virtual int		GetPing( int index ) = 0;
//	virtual int		GetPacketloss( int index ) = 0;
	virtual int		GetDeaths( int index ) = 0;
	virtual int		GetScore( int index ) = 0;
	virtual int		GetTeam( int index ) = 0;
	virtual int		GetHealth( int index ) = 0;

	//ios
	virtual int		GetRedCards( int index ) = 0;
	virtual int		GetYellowCards( int index ) = 0;
	virtual int		GetFouls( int index ) = 0;
	virtual int		GetGoals( int index ) = 0;
	virtual int		GetAssists( int index ) = 0;
	virtual int		GetPossession( int index ) = 0;
	virtual int		GetPasses( int index ) = 0;
	virtual int		GetFreeKicks( int index ) = 0;
	virtual int		GetPenalties( int index ) = 0;
	virtual int		GetCorners( int index ) = 0;
	virtual int		GetThrowIns( int index ) = 0;
	virtual int		GetKeeperSaves( int index ) = 0;
	virtual int		GetGoalKicks( int index ) = 0;
	virtual int		GetTeamPosition( int iIndex ) = 0;
	virtual int		GetShirtPosition( int iIndex ) = 0;
	virtual int		GetTeamToJoin( int iIndex ) = 0;
	virtual float	GetNextJoin( int iIndex ) = 0;
};

extern IGameResources *GameResources( void ); // singelton accessor

#endif

