#include "cbase.h"
#include "c_ball.h"
#include "c_sdk_player.h"
#include "fx_line.h"
#include "sdk_gamerules.h"
#include "c_team.h"
#include "ios_camera.h"

LINK_ENTITY_TO_CLASS(football, C_Ball);

IMPLEMENT_CLIENTCLASS_DT( C_Ball, DT_Ball, CBall )
	RecvPropInt( RECVINFO( m_iPhysicsMode ) ),
	RecvPropFloat( RECVINFO( m_fMass ) ),
	RecvPropEHandle(RECVINFO(m_pCreator)),
	RecvPropEHandle(RECVINFO(m_pLastActivePlayer)),
	RecvPropEHandle(RECVINFO(m_pHoldingPlayer)),
	RecvPropInt(RECVINFO(m_nLastActiveTeam)),
	RecvPropBool(RECVINFO(m_bIsPlayerBall)),
	RecvPropInt(RECVINFO(m_eBallState)),
	RecvPropBool(RECVINFO(m_bNonnormalshotsBlocked)),
	RecvPropBool(RECVINFO(m_bShotsBlocked)),
	RecvPropString(RECVINFO(m_szSkinName))
END_RECV_TABLE()

C_Ball *g_pBall = NULL;

C_Ball *GetBall()
{
	return g_pBall;
}

C_Ball::C_Ball()
{
	m_pCreator = NULL;
	m_bIsPlayerBall = false;
	m_pLastActivePlayer = NULL;
	m_pHoldingPlayer = NULL;
	m_nLastActiveTeam = TEAM_UNASSIGNED;
	m_szSkinName[0] = '\0';
}

C_Ball::~C_Ball()
{
	if (!m_bIsPlayerBall)
		g_pBall = NULL;
}

void C_Ball::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	 if (updateType == DATA_UPDATE_CREATED)
	 {
		if (!g_pBall && !m_bIsPlayerBall)
		{
			g_pBall = this;
			Camera()->SetTarget(this->entindex());
		}

		SetNextClientThink(CLIENT_THINK_ALWAYS);
	 }

	return;
}

bool C_Ball::ShouldInterpolate()
{
	if (this == GetBall())
		return true;
	else
		return BaseClass::ShouldInterpolate();
}

void C_Ball::ClientThink()
{
	if (m_pHoldingPlayer)
	{
		Vector handPos;
		QAngle handAng;
		m_pHoldingPlayer->GetAttachment("keeperballrighthand", handPos, handAng);
		SetLocalOrigin(handPos);
		SetLocalAngles(handAng);
	}

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

bool C_Ball::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	return false;
}