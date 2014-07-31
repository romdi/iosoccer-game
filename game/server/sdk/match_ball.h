#ifndef _MATCH_BALL_H
#define _MATCH_BALL_H

#include "cbase.h"
#include "ball.h"

class CBall;

class CMatchBall : public CBall
{
public:

	DECLARE_CLASS( CMatchBall, CBall );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CMatchBall();
	~CMatchBall();

	void			State_Enter(ball_state_t newState, bool cancelQueuedState);
	void			State_Think();
	void			State_Leave(ball_state_t newState);
	void			State_Transition(ball_state_t newState, float delay = 0.0f, bool cancelQueuedState = false, bool isShortMessageDelay = false);

	void			Spawn();
	void			Reset();

	CSDKPlayer		*GetLastActivePlayer() { return m_pLastActivePlayer; }

	CNetworkHandle(CSDKPlayer, m_pLastActivePlayer);
	CNetworkVar(int, m_nLastActiveTeam);
};

extern CMatchBall *GetMatchBall();
extern CMatchBall *CreateMatchBall(const Vector &pos);

#endif