#ifndef _PLAYER_BALL_H
#define _PLAYER_BALL_H

#include "cbase.h"
#include "ball.h"

class CBall;

extern CBall *GetNearestPlayerBall(const Vector &pos);

class CPlayerBall : public CBall
{
public:

	DECLARE_CLASS( CPlayerBall, CBall );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CPlayerBall();
	~CPlayerBall();

	void			SetVel(Vector vel, float spinCoeff, int spinFlags, body_part_t bodyPart, bool isDeflection, bool markOffsidePlayers, bool ensureMinShotStrength, float nextShotMinDelay = 0);
	void			SaveBallCannonSettings();
	void			RestoreBallCannonSettings();
	static void		RemoveAllPlayerBalls();
	void			RemovePlayerBall();
	CSDKPlayer		*GetCreator() { return m_pCreator; }
	void			SetCreator(CSDKPlayer *pCreator) { m_pCreator = pCreator; }
	//void			State_Enter(ball_state_t newState, bool cancelQueuedState);
	//void			State_Think();
	//void			State_Leave(ball_state_t newState);

	CNetworkHandle(CSDKPlayer, m_pCreator);

protected:

	bool			m_bIsBallCannonMode;
	Vector			m_vBallCannonPos;
	Vector			m_vBallCannonVel;
	QAngle			m_aBallCannonAng;
	AngularImpulse	m_vBallCannonRot;
};

extern CPlayerBall *CreatePlayerBall(const Vector &pos, CSDKPlayer *pCreator);

#endif