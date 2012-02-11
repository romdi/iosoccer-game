#include "cbase.h"
#include "c_ball.h"

LINK_ENTITY_TO_CLASS(football, C_Ball);

IMPLEMENT_CLIENTCLASS_DT( C_Ball, DT_Ball, CBall )
	RecvPropInt( RECVINFO( m_iPhysicsMode ) ),
	RecvPropFloat( RECVINFO( m_fMass ) ),
	//RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),	//ios1.1
END_RECV_TABLE()

C_Ball *g_pBall = NULL;

C_Ball *GetBall()
{
	return g_pBall;
}