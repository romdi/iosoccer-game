#include "cbase.h"
#include "c_ios_replaymanager.h"

IMPLEMENT_CLIENTCLASS_DT(C_ReplayPlayer, DT_ReplayPlayer, CReplayPlayer)
	RecvPropInt(RECVINFO(m_nTeamNumber)),
	RecvPropInt(RECVINFO(m_nTeamPosNum)),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_ReplayManager, DT_ReplayManager, CReplayManager)
	RecvPropBool(RECVINFO(m_bIsReplaying)),
END_RECV_TABLE()

C_ReplayManager *g_pReplayManager = NULL;

C_ReplayManager *GetReplayManager()
{
	return g_pReplayManager;
}

C_ReplayManager::C_ReplayManager()
{
	g_pReplayManager = this;
	m_bIsReplaying = false;
}

C_ReplayManager::~C_ReplayManager()
{
	g_pReplayManager = NULL;
}

bool C_ReplayManager::IsReplaying()
{
	return m_bIsReplaying;
}