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
	void State_KEEPERHANDS_Enter();		void State_KEEPERHANDS_Think();		void State_KEEPERHANDS_Leave(ball_state_t newState);

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

	void			Touched(bool isShot, body_part_t bodyPart, const Vector &oldVel);
	bool			CheckFoul(bool canShootBall, const Vector &localDirToBall);
	void			SetFoulParams(foul_type_t type, Vector pos, CSDKPlayer *pFoulingPl, CSDKPlayer *pFouledPl = NULL);
	bool			IsPlayerClose();
	void			SetVel(Vector vel, float spinCoeff, int spinFlags, body_part_t bodyPart, bool isDeflection, bool markOffsidePlayers, bool ensureMinShotStrength, float nextShotMinDelay = 0);
	void			MarkOffsidePlayers();
	void			UnmarkOffsidePlayers();
	float			CalcFieldZone();
	void			UpdatePossession(CSDKPlayer *pNewPossessor);
	void			SetPenaltyState(penalty_state_t penaltyState);
	penalty_state_t	GetPenaltyState() { return m_ePenaltyState; }
	void			SetPenaltyTaker(CSDKPlayer *pPl);
	bool			PlayersAtTargetPos();
	void			VPhysicsCollision(int index, gamevcollisionevent_t	*pEvent);
	void			RemoveAllTouches();
	BallTouchInfo	*LastInfo(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);
	CSDKPlayer		*LastPl(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);
	int				LastTeam(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);
	int				LastOppTeam(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);
	bool			IsLegallyCatchableByKeeper();
	bool			UseDribblingCollision();
	Vector			GetLastShotPos();
	void			CheckAdvantage();

	float			m_flStateActivationDelay;
	float			m_flSetpieceCloseStartTime;
	bool			m_bNextStateMessageSent;
	float			m_flStateTimelimit;

	CHandle<CSDKPlayer>	m_pOtherPl;
	CHandle<CSDKPlayer>	m_pSetpieceTaker;

	CNetworkHandle(CSDKPlayer, m_pLastActivePlayer);
	CNetworkVar(int, m_nLastActiveTeam);

	CNetworkVar(bool, m_bChargedshotBlocked);
	CNetworkVar(bool, m_bShotsBlocked);

	CHandle<CSDKPlayer>	m_pTurnoverPlayer;
	CHandle<CSDKPlayer>	m_pPossessingPl;
	int				m_nPossessingTeam;
	float			m_flPossessionStart;

	CUtlVector<BallTouchInfo *> m_Touches;

	penalty_state_t m_ePenaltyState;

	CHandle<CSDKPlayer>	m_pFouledPl;
	CHandle<CSDKPlayer>	m_pFoulingPl;
	int				m_nFouledTeam;
	int				m_nFoulingTeam;
	foul_type_t		m_eFoulType;
	Vector			m_vFoulPos;
	bool			m_bIsAdvantage;
	float			m_flFoulTime;
	bool			m_bIsPenalty;
	int				m_nTeam;
};

extern CMatchBall *GetMatchBall();
extern CMatchBall *CreateMatchBall(const Vector &pos);

#endif