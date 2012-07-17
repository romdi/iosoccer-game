#include "cbase.h"
#include "c_ios_replaymanager.h"

IMPLEMENT_CLIENTCLASS_DT(C_ReplayPlayer, DT_ReplayPlayer, CReplayPlayer)
	RecvPropInt(RECVINFO(m_nTeamNumber)),
	RecvPropInt(RECVINFO(m_nTeamPosition)),
END_RECV_TABLE()