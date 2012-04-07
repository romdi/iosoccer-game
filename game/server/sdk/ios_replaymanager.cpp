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

	if (args.ArgC() > 1)
		engine->ServerCommand(UTIL_VarArgs("host_timescale %f\n", (float)atof(args[1])));

	CReplayManager::GetInstance()->StartReplay();
}

static ConCommand start_replay("start_replay", cc_StartReplay);


void cc_StopReplay(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	CReplayManager::GetInstance()->StopReplay();
}

static ConCommand stop_replay("stop_replay", cc_StopReplay);


CReplayManager *CReplayManager::m_pInstance = NULL;

CReplayManager *CReplayManager::GetInstance()
{
	if (!m_pInstance)
		m_pInstance = new CReplayManager();

	return m_pInstance;
}

CReplayManager::CReplayManager()
{
	m_bDoReplay = false;
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

		int layerCount = pPl->GetNumAnimOverlays();
		for( int layerIndex = 0; layerIndex < layerCount; ++layerIndex )
		{
			CAnimationLayer *currentLayer = pPl->GetAnimOverlay(layerIndex);
			if( currentLayer )
			{
				pPlayerSnapshot->m_layerRecords[layerIndex].m_cycle = currentLayer->m_flCycle;
				pPlayerSnapshot->m_layerRecords[layerIndex].m_order = currentLayer->m_nOrder;
				pPlayerSnapshot->m_layerRecords[layerIndex].m_sequence = currentLayer->m_nSequence;
				pPlayerSnapshot->m_layerRecords[layerIndex].m_weight = currentLayer->m_flWeight;
			}
		}
		pPlayerSnapshot->m_masterSequence = pPl->GetSequence();
		pPlayerSnapshot->m_masterCycle = pPl->GetCycle();

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
	if (m_Snapshots.Count() == 0)
		return;

	m_nSnapshotIndex = 0;
	m_bDoReplay = true;

	GetBall()->VPhysicsGetObject()->EnableMotion(true);
	GetBall()->SetIgnoreTriggers(true);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->AddFlag(FL_REMOTECONTROLLED);
	}
}

void CReplayManager::StopReplay()
{
	m_bDoReplay = false;
	GetBall()->SetIgnoreTriggers(false);
	engine->ServerCommand("host_timescale 1\n");

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->RemoveFlag(FL_REMOTECONTROLLED);
	}
}

void CReplayManager::RestoreSnapshot()
{
	if (m_Snapshots.Count() == 0 || m_nSnapshotIndex >= m_Snapshots.Count())
	{
		StopReplay();
		return;
	}

	Snapshot *snapshot = &m_Snapshots[m_nSnapshotIndex];
	m_nSnapshotIndex += 1;
	GetBall()->VPhysicsGetObject()->SetPosition(snapshot->pBallSnapshot->pos, snapshot->pBallSnapshot->ang, false);
	GetBall()->VPhysicsGetObject()->SetVelocity(&snapshot->pBallSnapshot->vel, &snapshot->pBallSnapshot->rot);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		PlayerSnapshot *pPlSnap = snapshot->pPlayerSnapshot[i - 1];

		pPl->SetLocalOrigin(pPlSnap->pos);
		pPl->SetLocalVelocity(pPlSnap->vel);
		pPl->SetLocalAngles(pPlSnap->ang);

		pPl->SetSequence(pPlSnap->m_masterSequence);
		pPl->SetCycle(pPlSnap->m_masterCycle);

		int layerCount = pPl->GetNumAnimOverlays();
		for( int layerIndex = 0; layerIndex < layerCount; ++layerIndex )
		{
			CAnimationLayer *currentLayer = pPl->GetAnimOverlay(layerIndex);
			if(!currentLayer)
				continue;

			pPlSnap->m_layerRecords[layerIndex].m_cycle = currentLayer->m_flCycle;
			pPlSnap->m_layerRecords[layerIndex].m_order = currentLayer->m_nOrder;
			pPlSnap->m_layerRecords[layerIndex].m_sequence = currentLayer->m_nSequence;
			pPlSnap->m_layerRecords[layerIndex].m_weight = currentLayer->m_flWeight;

			//Either no interp, or interp failed.  Just use record.
			currentLayer->m_flCycle = pPlSnap->m_layerRecords[layerIndex].m_cycle;
			currentLayer->m_nOrder = pPlSnap->m_layerRecords[layerIndex].m_order;
			currentLayer->m_nSequence = pPlSnap->m_layerRecords[layerIndex].m_sequence;
			currentLayer->m_flWeight = pPlSnap->m_layerRecords[layerIndex].m_weight;
		}
	}
}