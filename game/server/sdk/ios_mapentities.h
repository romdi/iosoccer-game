#ifndef IOS_MAPENTITIES_H
#define IOS_MAPENTITIES_H

class CTeamSpots
{
public:
	Vector
		m_vCornerLeft,
		m_vCornerRight,
		m_vGoalkickLeft,
		m_vGoalkickRight,
		m_vPenalty,
		m_vPlayers[11];


};

extern CTeamSpots	*g_pTeamSpots[2];
extern Vector		g_vKickOffSpot;

extern void InitMapSpots();
extern CTeamSpots *GetOwnTeamSpots(CSDKPlayer *pPl);
extern CTeamSpots *GetOpponentTeamSpots(CSDKPlayer *pPl);

#endif