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

	void State_STATIC_Enter();			void State_STATIC_Think();			void State_STATIC_Leave(ball_state_t newState);
	void State_NORMAL_Enter();			void State_NORMAL_Think();			void State_NORMAL_Leave(ball_state_t newState);
	void State_KICKOFF_Enter();			void State_KICKOFF_Think();			void State_KICKOFF_Leave(ball_state_t newState);
	void State_THROWIN_Enter();			void State_THROWIN_Think();			void State_THROWIN_Leave(ball_state_t newState);
	void State_GOALKICK_Enter();		void State_GOALKICK_Think();		void State_GOALKICK_Leave(ball_state_t newState);
	void State_CORNER_Enter();			void State_CORNER_Think();			void State_CORNER_Leave(ball_state_t newState);
	void State_GOAL_Enter();			void State_GOAL_Think();			void State_GOAL_Leave(ball_state_t newState);
	void State_FREEKICK_Enter();		void State_FREEKICK_Think();		void State_FREEKICK_Leave(ball_state_t newState);
	void State_PENALTY_Enter();			void State_PENALTY_Think();			void State_PENALTY_Leave(ball_state_t newState);

	void			Spawn();
	void			Reset();
	void			TriggerGoal(int team);
	void			TriggerGoalLine(int team);
	void			TriggerSideline();
	void			GetGoalInfo(bool &isOwnGoal, int &scoringTeam, CSDKPlayer **pScorer, CSDKPlayer **pFirstAssister, CSDKPlayer **pSecondAssister);
	void			SendNotifications();

	CSDKPlayer		*GetCurrentOtherPlayer() { return m_pOtherPl; }
	CSDKPlayer		*GetLastActivePlayer() { return m_pLastActivePlayer; }
	void			SetSetpieceTaker(CSDKPlayer *pPlayer) { m_pSetpieceTaker = pPlayer; m_pPl = NULL; }

	void			Touched(CSDKPlayer *pPl, bool isShot, body_part_t bodyPart, const Vector &oldVel);

	float			m_flStateEnterTime;
	float			m_flStateLeaveTime;
	float			m_flStateActivationDelay;
	float			m_flSetpieceCloseStartTime;
	bool			m_bNextStateMessageSent;

	CHandle<CSDKPlayer>	m_pOtherPl;
	CHandle<CSDKPlayer>	m_pSetpieceTaker;

	CNetworkHandle(CSDKPlayer, m_pLastActivePlayer);
	CNetworkVar(int, m_nLastActiveTeam);
};

extern CMatchBall *GetMatchBall();
extern CMatchBall *CreateMatchBall(const Vector &pos);

#endif