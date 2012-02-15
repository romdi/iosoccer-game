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
	RecvPropFloat(RECVINFO(m_flOffsideLineY)),
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

	//FXLineData_t lineData;

	Vector start = Vector(SDKGameRules()->m_vFieldMin.GetX(), m_flOffsideLineY - 2, SDKGameRules()->m_vKickOff.GetZ());
	Vector end = Vector(SDKGameRules()->m_vFieldMax.GetX(), m_flOffsideLineY + 2, SDKGameRules()->m_vKickOff.GetZ() + 4);

	//lineData.m_flDieTime = 10;

	//lineData.m_flStartAlpha= 1;
	//lineData.m_flEndAlpha = 1;

	//lineData.m_flStartScale = 5;
	//lineData.m_flEndScale = 5; 

	//lineData.m_pMaterial = materials->FindMaterial( "effects/splash3", 0, 0 );

	//lineData.m_vecStart = start;
	//lineData.m_vecStartVelocity = vec3_origin;

	//lineData.m_vecEnd = end;
	//lineData.m_vecEndVelocity = vec3_origin;

	//FX_AddLine( lineData );

	//FX_AddQuad(start, 
	//	Vector(0, 0, 1), 
	//	50, 
	//	50, 
	//	1,
	//	1,	// start alpha
	//	1,		// end alpha
	//	1,
	//	0,
	//	0,
	//	Vector(1, 1, 1), 
	//	30, 
	//	"effects/splashwake1", 
	//	0 );

	FX_AddCube(start, end, Vector(1, 1, 1), 10, "football/black");

	m_bShowOffsideLine = false;
}

void C_Ball::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	return;
}