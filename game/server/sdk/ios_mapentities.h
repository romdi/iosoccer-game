#ifndef IOS_MAPENTITIES_H
#define IOS_MAPENTITIES_H

#include "cbase.h"
#include "sdk_player.h"

class CTeamSpots
{
public:
	Vector
		m_vCornerLeft,
		m_vCornerRight,
		m_vGoalkickLeft,
		m_vGoalkickRight,
		m_vPenalty, m_vPenaltyMin, m_vPenaltyMax,
		m_vPlayers[11];
	int
		m_nLeft,
		m_nRight,
		m_nForward,
		m_nBack;
};

extern CTeamSpots	*g_pTeamSpots[2];
extern Vector		g_vKickOffSpot;
extern Vector		g_vFieldMin;
extern Vector		g_vFieldMax;

extern void InitMapSpots();
extern CTeamSpots *GetOwnTeamSpots(CSDKPlayer *pPl);
extern CTeamSpots *GetOppTeamSpots(CSDKPlayer *pPl);
extern CTeamSpots *GetTeamSpots(int team);
extern float g_flGroundZ;

#endif