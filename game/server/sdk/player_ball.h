#ifndef _PLAYER_BALL_H
#define _PLAYER_BALL_H

#include "cbase.h"
#include "ball.h"

class CBall;

extern CBall *GetNearestPlayerBall(const Vector &pos);

struct BallCannonSettings
{
	Vector pos;
	Vector vel;
	QAngle ang;
	AngularImpulse rot;
	float globalDynamicShotDelay;
};

class CPlayerBall : public CBall
{
public:

	DECLARE_CLASS( CPlayerBall, CBall );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CPlayerBall();
	~CPlayerBall();

	void			SetVel(Vector vel, float spinCoeff, int spinFlags, body_part_t bodyPart, bool markOffsidePlayers, float shotTakerMinDelay);
	void			SaveBallCannonSettings();
	void			RestoreBallCannonSettings();
	void			SetBallCannonMode(bool isBallCannonMode);
	static void		RemoveAllPlayerBalls();
	void			RemovePlayerBall();
	CSDKPlayer		*GetCreator() { return m_pCreator; }
	void			SetCreator(CSDKPlayer *pCreator) { m_pCreator = pCreator; }
	void			TriggerGoal(int team);
	void			TriggerGoalLine(int team);
	void			TriggerSideline();

	void			State_Enter(ball_state_t newState, bool cancelQueuedState);
	void			State_Think();
	void			State_Leave(ball_state_t newState);
	void			State_Transition(ball_state_t nextState, float nextStateMessageDelay = 0, float nextStatePostMessageDelay = 0, bool cancelQueuedState = false);

	void State_NORMAL_Enter();			void State_NORMAL_Think();			void State_NORMAL_Leave(ball_state_t newState);
	void State_KEEPERHANDS_Enter();		void State_KEEPERHANDS_Think();		void State_KEEPERHANDS_Leave(ball_state_t newState);

	void			Touched(bool isShot, body_part_t bodyPart, const Vector &oldVel);
	bool			CheckFoul(bool canShootBall, const Vector &localDirToBall) { return false; }
	void			VPhysicsCollision(int index, gamevcollisionevent_t	*pEvent);
	bool			IsLegallyCatchableByKeeper();

	CNetworkHandle(CSDKPlayer, m_pCreator);

protected:

	bool			m_bIsBallCannonMode;
	BallCannonSettings m_BallCannonSettings;
	CHandle<CSDKPlayer>	m_pLastShooter;
	Vector			m_vLastShotPos;
	BallTouchInfo	m_BallShotInfo;
};

extern CPlayerBall *CreatePlayerBall(const Vector &pos, CSDKPlayer *pCreator);

#endif