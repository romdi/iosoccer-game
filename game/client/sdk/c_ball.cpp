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
	//RecvPropEHandle(RECVINFO(m_pCreator)),
	RecvPropEHandle(RECVINFO(m_pHoldingPlayer)),
	RecvPropBool(RECVINFO(m_bIsPlayerBall)),
	RecvPropInt(RECVINFO(m_eBallState)),
	RecvPropString(RECVINFO(m_szSkinName))
END_RECV_TABLE()

C_Ball::C_Ball()
{
	m_pHoldingPlayer = NULL;
	m_szSkinName[0] = '\0';
}

C_Ball::~C_Ball()
{
}

void C_Ball::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	 if (updateType == DATA_UPDATE_CREATED)
		SetNextClientThink(CLIENT_THINK_ALWAYS);

	return;
}

bool C_Ball::ShouldInterpolate()
{
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
	return !mp_ball_mass_fix_enabled.GetBool();
}