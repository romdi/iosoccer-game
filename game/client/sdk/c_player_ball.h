#ifndef C_PLAYER_BALL_H
#define C_PLAYER_BALL_H

#include "cbase.h"
#include "c_physicsprop.h"
#include "props_shared.h"
#include "c_props.h"
#include "c_sdk_player.h"
#include "c_ball.h"

class C_PlayerBall : public C_Ball
{
public:

	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_PlayerBall, C_Ball );

	CHandle<C_SDKPlayer> m_pCreator;
};

#endif