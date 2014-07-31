#include "cbase.h"
#include "c_player_ball.h"

IMPLEMENT_CLIENTCLASS_DT( C_PlayerBall, DT_PlayerBall, CPlayerBall )
	RecvPropEHandle(RECVINFO(m_pCreator)),
END_RECV_TABLE()