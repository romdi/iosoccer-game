#include "cbase.h"
#include "c_ball.h"
#include "c_sdk_player.h"
#include "fx_line.h"
#include "sdk_gamerules.h"

LINK_ENTITY_TO_CLASS(football, C_Ball);

void RecvProxy_SetOffsideLine( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// Have the regular proxy store the data.
	RecvProxy_Int32ToInt32( pData, pStruct, pOut );

	C_Ball *pBall = (C_Ball*)pStruct;
	pBall->SetOffsideLine();
}

IMPLEMENT_CLIENTCLASS_DT( C_Ball, DT_Ball, CBall )
	RecvPropInt( RECVINFO( m_iPhysicsMode ) ),
	RecvPropFloat( RECVINFO( m_fMass ) ),
	RecvPropFloat(RECVINFO(m_flOffsideLineBallY)),
	RecvPropFloat(RECVINFO(m_flOffsideLinePlayerY)),
	RecvPropInt(RECVINFO(m_bShowOffsideLine), 0, RecvProxy_SetOffsideLine),
	//RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),	//ios1.1
END_RECV_TABLE()

C_Ball *g_pBall = NULL;

C_Ball *GetBall()
{
	return g_pBall;
}

void C_Ball::SetOffsideLine()
{
	if (!m_bShowOffsideLine)
		return;

	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();

	if (!pLocal)
		return;

	Vector ballLineStart = Vector(SDKGameRules()->m_vFieldMin.GetX(), m_flOffsideLineBallY - 2, SDKGameRules()->m_vKickOff.GetZ());
	Vector ballLineEnd = Vector(SDKGameRules()->m_vFieldMax.GetX(), m_flOffsideLineBallY + 2, SDKGameRules()->m_vKickOff.GetZ() + 4);
	FX_AddCube(ballLineStart, ballLineEnd, Vector(0, 1, 0), 3, "pitch/offside_line_ball");

	Vector playerLineStart = Vector(SDKGameRules()->m_vFieldMin.GetX(), m_flOffsideLinePlayerY - 2, SDKGameRules()->m_vKickOff.GetZ());
	Vector playerLineEnd = Vector(SDKGameRules()->m_vFieldMax.GetX(), m_flOffsideLinePlayerY + 2, SDKGameRules()->m_vKickOff.GetZ() + 4);
	FX_AddCube(playerLineStart, playerLineEnd, Vector(1, 0, 0), 3, "pitch/offside_line_player");

	m_bShowOffsideLine = false;
}

void C_Ball::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	return;
}