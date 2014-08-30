#include "cbase.h"
#include "c_match_ball.h"
#include "ios_camera.h"

IMPLEMENT_CLIENTCLASS_DT( C_MatchBall, DT_MatchBall, CMatchBall )
	RecvPropEHandle(RECVINFO(m_pLastActivePlayer)),
	RecvPropInt(RECVINFO(m_nLastActiveTeam)),
END_RECV_TABLE()

C_MatchBall *g_pBall = NULL;

C_MatchBall *GetMatchBall()
{
	return g_pBall;
}

C_MatchBall::C_MatchBall()
{
}

C_MatchBall::~C_MatchBall()
{
	g_pBall = NULL;
}

void C_MatchBall::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	 if (updateType == DATA_UPDATE_CREATED)
	 {
		if (!g_pBall)
		{
			g_pBall = this;
			Camera()->SetTarget(this->entindex());
		}

		SetNextClientThink(CLIENT_THINK_ALWAYS);
	 }
}