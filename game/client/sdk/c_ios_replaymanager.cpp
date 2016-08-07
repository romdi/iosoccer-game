#include "cbase.h"
#include "c_ios_replaymanager.h"

IMPLEMENT_CLIENTCLASS_DT(C_ReplayBall, DT_ReplayBall, CReplayBall)
	RecvPropString(RECVINFO(m_szSkinName)),
END_RECV_TABLE()

C_ReplayBall *g_pReplayBall = NULL;

C_ReplayBall *GetReplayBall()
{
	return g_pReplayBall;
}

C_ReplayBall::C_ReplayBall()
{
	m_szSkinName[0] = '\0';

	g_pReplayBall = this;
}

C_ReplayBall::~C_ReplayBall()
{
	g_pReplayBall = NULL;
}

IMPLEMENT_CLIENTCLASS_DT(C_ReplayPlayer, DT_ReplayPlayer, CReplayPlayer)
	RecvPropInt(RECVINFO(m_nTeamNumber)),
	RecvPropInt(RECVINFO(m_nTeamPosIndex)),
	RecvPropBool(RECVINFO(m_bIsKeeper)),
	RecvPropInt(RECVINFO(m_nShirtNumber)),
	RecvPropInt(RECVINFO(m_nSkinIndex)),
	RecvPropInt(RECVINFO(m_nHairIndex)),
	RecvPropString(RECVINFO(m_szPlayerName)),
	RecvPropString(RECVINFO(m_szShirtName)),
	RecvPropString(RECVINFO(m_szShoeName)),
	RecvPropString(RECVINFO(m_szKeeperGloveName)),
END_RECV_TABLE()

C_ReplayPlayer::C_ReplayPlayer()
{
	m_nTeamNumber = TEAM_HOME;
	m_nTeamPosIndex = 0;
	m_bIsKeeper = false;
	m_nShirtNumber = 2;
	m_nSkinIndex = 0;
	m_nHairIndex = 0;
	m_szPlayerName[0] = '\0';
	m_szShirtName[0] = '\0';
	m_szShoeName[0] = '\0';
	m_szKeeperGloveName[0] = '\0';
}

C_ReplayPlayer::~C_ReplayPlayer()
{
}

IMPLEMENT_CLIENTCLASS_DT(C_ReplayManager, DT_ReplayManager, CReplayManager)
	RecvPropBool(RECVINFO(m_bIsReplaying)),
	RecvPropInt(RECVINFO(m_nReplayRunIndex)),
	RecvPropBool(RECVINFO(m_bAtMinGoalPos)),
END_RECV_TABLE()

C_ReplayManager *g_pReplayManager = NULL;

C_ReplayManager *GetReplayManager()
{
	return g_pReplayManager;
}

C_ReplayManager::C_ReplayManager()
{
	m_bIsReplaying = false;
	m_nReplayRunIndex = 0;
	m_bAtMinGoalPos = true;

	g_pReplayManager = this;
}

C_ReplayManager::~C_ReplayManager()
{
	g_pReplayManager = NULL;
}

bool C_ReplayManager::IsReplaying()
{
	return m_bIsReplaying;
}