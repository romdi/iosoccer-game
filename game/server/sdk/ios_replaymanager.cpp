#include "cbase.h"
#include "ios_replaymanager.h"
#include "ball.h"
#include "sdk_gamerules.h"
#include "convar.h"

static ConVar sv_replay_duration("sv_replay_duration", "10");

void cc_StartReplay(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	CReplayManager::GetInstance()->StartReplay();
}

static ConCommand start_replay("start_replay", cc_StartReplay);
//static ConCommand start_replay("stop_replay", cc_StopReplay);

CReplayManager *CReplayManager::m_pInstance = NULL;

CReplayManager *CReplayManager::GetInstance()
{
	if (!m_pInstance)
		m_pInstance = new CReplayManager();

	return m_pInstance;
}

CReplayManager::CReplayManager()
{

}

void CReplayManager::CheckReplay()
{
	if (m_bDoReplay)
		RestoreSnapshot();
	else if (sv_replay_duration.GetInt() > 0)
		TakeSnapshot();
}

void CReplayManager::TakeSnapshot()
{
	Vector pos, vel;
	QAngle ang;
	AngularImpulse rot;

	GetBall()->VPhysicsGetObject()->GetPosition(&pos, &ang);
	GetBall()->VPhysicsGetObject()->GetVelocity(&vel, &rot);

	BallSnapshot *pBallSnapshot = new BallSnapshot(pos, ang, vel, rot);
	Snapshot snapshot = { gpGlobals->curtime, pBallSnapshot };

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pos = pPl->GetLocalOrigin();
		vel = pPl->GetLocalVelocity();
		ang = pPl->GetLocalAngles();

		PlayerSnapshot *pPlayerSnapshot = new PlayerSnapshot(pPl, pos, ang, vel);
		snapshot.pPlayerSnapshot[i - 1] = pPlayerSnapshot;
	}

	m_Snapshots.AddToTail(snapshot);
	
	if (m_Snapshots.Count() > sv_replay_duration.GetInt() * (1.0f / TICK_INTERVAL))
	{
		delete m_Snapshots[0].pBallSnapshot;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			delete m_Snapshots[0].pPlayerSnapshot[i - 1];
		}
		m_Snapshots.Remove(0);
	}
}

void CReplayManager::StartReplay()
{
	if (m_Snapshots.Count() > 0)
	{
		m_nSnapshotIndex = 0;
		m_bDoReplay = true;
	}
}

void CReplayManager::RestoreSnapshot()
{
	if (m_Snapshots.Count() > 0 && m_nSnapshotIndex < m_Snapshots.Count())
	{
		Snapshot snapshot = m_Snapshots[m_nSnapshotIndex];
		m_nSnapshotIndex += 1;
		GetBall()->VPhysicsGetObject()->SetPosition(snapshot.pBallSnapshot->pos, snapshot.pBallSnapshot->ang, false);
		GetBall()->VPhysicsGetObject()->SetVelocity(&snapshot.pBallSnapshot->vel, &snapshot.pBallSnapshot->rot);

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
			if (!CSDKPlayer::IsOnField(pPl))
				continue;

			pPl->SetLocalOrigin(snapshot.pPlayerSnapshot[i - 1]->pos);
			pPl->SetLocalVelocity(snapshot.pPlayerSnapshot[i - 1]->vel);
			pPl->SetLocalAngles(snapshot.pPlayerSnapshot[i - 1]->ang);
		}
	}
	else
	{
		m_bDoReplay = false;
	}
}