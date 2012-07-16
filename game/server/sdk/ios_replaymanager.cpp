#include "cbase.h"
#include "ios_replaymanager.h"
#include "ball.h"
#include "sdk_gamerules.h"
#include "convar.h"
#include "sdk_playeranimstate.h"

class CReplayBall : public CPhysicsProp //Or just CBaseProp
{
public:
 DECLARE_CLASS( CReplayBall, CPhysicsProp );

 typedef CPhysicsProp BaseClass;
 CReplayBall();


 bool CreateVPhysics( void );
 void Spawn( void );
 virtual void Precache();
 DECLARE_DATADESC();
};

BEGIN_DATADESC( CReplayBall )

END_DATADESC()

LINK_ENTITY_TO_CLASS( replayball, CReplayBall );

bool CReplayBall::CreateVPhysics()
{
 // Create the object in the physics system
	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_SOLID,false );


 // Make sure I get touch called for static geometry
 if ( pPhysicsObject )
 {

  //int flags = pPhysicsObject->GetCallbackFlags();
  //flags |= CALLBACK_GLOBAL_TOUCH_STATIC;

  //pPhysicsObject->SetCallbackFlags(flags);

 }


 return true;
}

void CReplayBall::Precache()
{
 SetModel( BALL_MODEL );
 BaseClass::Precache();
}

CReplayBall::CReplayBall()
{

}

void CReplayBall::Spawn( void )
{
Precache();
SetModelName( MAKE_STRING( BALL_MODEL ) );
CreateVPhysics();
//SetSolid(SOLID_NONE);
}

class CReplayPlayer : public CBaseAnimatingOverlay
{
public:
 DECLARE_CLASS( CReplayPlayer, CBaseAnimatingOverlay );

 typedef CBaseAnimatingOverlay BaseClass;
 CReplayPlayer();

 void Spawn( void );
 virtual void Precache();
 void Think();
 DECLARE_DATADESC();
};

BEGIN_DATADESC( CReplayPlayer )
	DEFINE_THINKFUNC( Think ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( replayplayer, CReplayPlayer );

void CReplayPlayer::Precache()
{
	SetModel( SDK_PLAYER_MODEL );
	BaseClass::Precache();
}

CReplayPlayer::CReplayPlayer()
{

}

void CReplayPlayer::Spawn( void )
{
Precache();
SetModelName( MAKE_STRING( SDK_PLAYER_MODEL ) );
SetSolidFlags(FSOLID_NOT_SOLID);
SetModel( SDK_PLAYER_MODEL );
SetSolid( SOLID_BBOX );
UTIL_SetSize( this, -Vector(20,20,20), Vector(20,20,20) );
SetThink(&CReplayPlayer::Think);
SetNextThink(gpGlobals->curtime);
}

void CReplayPlayer::Think()
{
	//BaseClass::Think();
	SetSimulationTime( gpGlobals->curtime );
	
	SetNextThink( gpGlobals->curtime );
	//SetAnimatedEveryTick( true );
	StudioFrameAdvance();
	DispatchAnimEvents(this);
}

static ConVar sv_replay_duration("sv_replay_duration", "10");

void cc_StartReplay(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (args.ArgC() > 1)
		engine->ServerCommand(UTIL_VarArgs("host_timescale %f\n", (float)atof(args[1])));

	ReplayManager()->StartReplay();
}

static ConCommand start_replay("start_replay", cc_StartReplay);


void cc_StopReplay(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	ReplayManager()->StopReplay();
}

static ConCommand stop_replay("stop_replay", cc_StopReplay);

LINK_ENTITY_TO_CLASS(replay_manager, CReplayManager);

BEGIN_DATADESC( CReplayManager )
	DEFINE_THINKFUNC( Think ),
END_DATADESC()

CReplayManager *g_pReplayManager = NULL;

CReplayManager::CReplayManager()
{
	Assert(!g_pReplayManager);
	g_pReplayManager = this;

	m_bDoReplay = false;
	m_pBall = NULL;
	for (int i = 0; i < 32; i++)
	{
		m_pPlayers[i] = NULL;
	}
}

void CReplayManager::Spawn()
{
	SetThink(&CReplayManager::Think);
	SetNextThink(gpGlobals->curtime);
}

void CReplayManager::Think()
{
	SetNextThink(gpGlobals->curtime);

	if (!GetBall())
		return;

	CheckReplay();
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
		{
			snapshot.pPlayerSnapshot[i - 1] = NULL;
			continue;
		}

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
				pPlayerSnapshot->m_layerRecords[layerIndex].m_playbackrate = currentLayer->m_flPlaybackRate;
			}
		}

		pPlayerSnapshot->m_masterSequence = pPl->GetSequence();
		pPlayerSnapshot->m_masterCycle = pPl->GetCycle();
		pPlayerSnapshot->m_flSimulationTime = pPl->GetSimulationTime();

		pPlayerSnapshot->m_flMoveX = pPl->GetPoseParameter(4);
		pPlayerSnapshot->m_flMoveY = pPl->GetPoseParameter(3);

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
	if (m_bDoReplay)
		return;

	if (m_Snapshots.Count() == 0)
		return;

	m_nSnapshotIndex = 0;
	m_bDoReplay = true;
}

void CReplayManager::StopReplay()
{
	if (!m_bDoReplay)
		return;

	m_bDoReplay = false;
	engine->ServerCommand("host_timescale 1\n");

	UTIL_Remove(m_pBall);

	for (int i = 0; i < 32; i++)
	{
		UTIL_Remove(m_pPlayers[i]);
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

	if (!m_pBall)
	{
		m_pBall = (CReplayBall *)CreateEntityByName("replayball");
		m_pBall->SetRenderMode(kRenderTransColor);
		m_pBall->SetRenderColorA(150);
		m_pBall->Spawn();
	}

	m_pBall->VPhysicsGetObject()->SetPosition(snapshot->pBallSnapshot->pos, snapshot->pBallSnapshot->ang, false);
	m_pBall->VPhysicsGetObject()->SetVelocity(&snapshot->pBallSnapshot->vel, &snapshot->pBallSnapshot->rot);

	for (int i = 0; i < 32; i++)
	{
		PlayerSnapshot *pPlSnap = snapshot->pPlayerSnapshot[i];
		if (!pPlSnap)
		{
			UTIL_Remove(m_pPlayers[i]);
			continue;
		}

		if (!m_pPlayers[i])
		{
			m_pPlayers[i] = (CReplayPlayer *)CreateEntityByName("replayplayer");
			m_pPlayers[i]->SetRenderMode(kRenderTransColor);
			m_pPlayers[i]->SetRenderColorA(150);
			m_pPlayers[i]->Spawn();
			//m_pPlayers[i]->UseClientSideAnimation();
		}

		CBaseAnimatingOverlay *pPl = m_pPlayers[i];

		pPl->SetLocalOrigin(pPlSnap->pos);
		pPl->SetLocalVelocity(pPlSnap->vel);
		pPl->SetLocalAngles(pPlSnap->ang);
		//pPl->SnapEyeAngles(pPlSnap->ang);

		pPl->SetPoseParameter(pPl->GetModelPtr(), 4, pPlSnap->m_flMoveX);
		pPl->SetPoseParameter(pPl->GetModelPtr(), 3, pPlSnap->m_flMoveY);

		pPl->ResetSequence(pPlSnap->m_masterSequence);
		pPl->SetCycle(pPlSnap->m_masterCycle);
		//pPl->SetSimulationTime(pPlSnap->m_flSimulationTime);
		//pPl->SetPlaybackRate(1);

		int layerCount = 8;//pPl->GetNumAnimOverlays();
		pPl->SetNumAnimOverlays(layerCount);

		for( int layerIndex = 0; layerIndex < layerCount; ++layerIndex )
		{
			CAnimationLayer *currentLayer = pPl->GetAnimOverlay(layerIndex);
			if(!currentLayer)
				continue;

			//pPlSnap->m_layerRecords[layerIndex].m_cycle = currentLayer->m_flCycle;
			//pPlSnap->m_layerRecords[layerIndex].m_order = currentLayer->m_nOrder;
			//pPlSnap->m_layerRecords[layerIndex].m_sequence = currentLayer->m_nSequence;
			//pPlSnap->m_layerRecords[layerIndex].m_weight = currentLayer->m_flWeight;

			//Either no interp, or interp failed.  Just use record.
			currentLayer->SetOrder(CBaseAnimatingOverlay::MAX_OVERLAYS);
			currentLayer->m_flCycle = pPlSnap->m_layerRecords[layerIndex].m_cycle;
			currentLayer->m_nOrder = pPlSnap->m_layerRecords[layerIndex].m_order;
			currentLayer->m_nSequence = pPlSnap->m_layerRecords[layerIndex].m_sequence;
			currentLayer->m_flWeight = pPlSnap->m_layerRecords[layerIndex].m_weight;
			currentLayer->m_flPlaybackRate = pPlSnap->m_layerRecords[layerIndex].m_playbackrate;
		}
	}

	m_nSnapshotIndex += 1;
}