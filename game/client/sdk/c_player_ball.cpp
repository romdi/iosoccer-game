#include "cbase.h"
#include "c_player_ball.h"

IMPLEMENT_CLIENTCLASS_DT( C_PlayerBall, DT_PlayerBall, CPlayerBall )
	RecvPropEHandle(RECVINFO(m_pCreator)),
END_RECV_TABLE()

C_PlayerBall *g_pLocalPlayerBall = NULL;

C_PlayerBall *GetLocalPlayerBall()
{
	return g_pLocalPlayerBall;
}

C_PlayerBall::C_PlayerBall()
{
}

C_PlayerBall::~C_PlayerBall()
{
	g_pLocalPlayerBall = NULL;
}

void C_PlayerBall::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	 if (updateType == DATA_UPDATE_CREATED)
	 {
		if (!g_pLocalPlayerBall && m_pCreator == C_SDKPlayer::GetLocalSDKPlayer())
		{
			g_pLocalPlayerBall = this;
		}

		SetNextClientThink(CLIENT_THINK_ALWAYS);
	 }
}