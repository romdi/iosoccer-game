#include "cbase.h"
#include "c_ball.h"
#include "c_sdk_player.h"
#include "fx_line.h"
#include "sdk_gamerules.h"
#include "c_team.h"
#include "ios_camera.h"
#include "beamdraw.h"
#include "view.h"
#include "collisionutils.h"

ConVar cl_ball_halo_enabled("cl_ball_halo_enabled", "1", FCVAR_ARCHIVE);

LINK_ENTITY_TO_CLASS(football, C_Ball);

IMPLEMENT_CLIENTCLASS_DT( C_Ball, DT_Ball, CBall )
	RecvPropInt( RECVINFO( m_iPhysicsMode ) ),
	RecvPropFloat( RECVINFO( m_fMass ) ),
	//RecvPropEHandle(RECVINFO(m_pCreator)),
	RecvPropEHandle(RECVINFO(m_pHoldingPlayer)),
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
		m_pHoldingPlayer->GetAttachment("ball_right_hand", handPos, handAng);
		SetLocalOrigin(handPos);
		SetLocalAngles(handAng);
	}

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

bool C_Ball::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	return false;
}

int C_Ball::DrawModel(int flags)
{
	if (cl_ball_halo_enabled.GetBool())
	{
		float dist = (GetLocalOrigin() - CurrentViewOrigin()).Length();
		float sizeCoeff = RemapValClamped(dist, 100.0f, 5000.0f, 0.0f, 1.0f);
		float alphaCoeff = RemapValClamped(dist, 100.0f, 1000.0f, 0.0f, 1.0f);

		IMaterial *pMaterial = materials->FindMaterial("sprites/circle", NULL, false);

		CMatRenderContextPtr pRenderContext(materials);
		pRenderContext->Bind(pMaterial);

		color32 color = { 255, 255, 255, (alphaCoeff * 0.5f) * 255 };
		float size = 2 * (BALL_PHYS_RADIUS + sizeCoeff * BALL_PHYS_RADIUS * 3.0f);
		DrawSprite(GetLocalOrigin(), size, size, color);
	}

	return BaseClass::DrawModel(flags);
}