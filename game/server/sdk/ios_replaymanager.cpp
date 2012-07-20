#include "cbase.h"
#include "ios_replaymanager.h"
#include "ball.h"
#include "sdk_gamerules.h"
#include "convar.h"
#include "sdk_playeranimstate.h"

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
SetSimulatedEveryTick( true );
SetAnimatedEveryTick( true );
}


BEGIN_DATADESC( CReplayPlayer )
	DEFINE_THINKFUNC( Think ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( replayplayer, CReplayPlayer );

IMPLEMENT_SERVERCLASS_ST(CReplayPlayer, DT_ReplayPlayer)
	SendPropInt(SENDINFO(m_nTeamNumber)),
	SendPropInt(SENDINFO(m_nTeamPosition)),
END_SEND_TABLE()

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
	//SetModelName( MAKE_STRING( SDK_PLAYER_MODEL ) );
	SetSolidFlags(FSOLID_NOT_SOLID);
	SetModel( SDK_PLAYER_MODEL );
	SetSolid( SOLID_BBOX );
	UTIL_SetSize( this, -Vector(20,20,20), Vector(20,20,20) );
	SetSimulatedEveryTick( true );
	SetAnimatedEveryTick( true );
	SetThink(&CReplayPlayer::Think);
	SetNextThink(gpGlobals->curtime);
}

void CReplayPlayer::Think()
{
	//BaseClass::Think();
	//SetSimulationTime( gpGlobals->curtime );
	
	SetNextThink( gpGlobals->curtime );
	//SetAnimatedEveryTick( true );
	StudioFrameAdvance();
	DispatchAnimEvents(this);
}

static ConVar sv_replay_duration("sv_replay_duration", "10");
static ConVar sv_replays("sv_replays", "1");

void cc_StartReplay(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (args.ArgC() > 1)
		engine->ServerCommand(UTIL_VarArgs("host_timescale %f\n", (float)atof(args[1])));

	ReplayManager()->StartReplay(1, 0);
}

static ConCommand start_replay("start_replay", cc_StartReplay);


void cc_StopReplay(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	ReplayManager()->StopReplay();
}

static ConCommand stop_replay("stop_replay", cc_StopReplay);

LINK_ENTITY_TO_CLASS(replaymanager, CReplayManager);

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
	for (int i = 0; i < 22; i++)
	{
		m_pPlayers[i] = NULL;
	}
}

void CReplayManager::Spawn()
{
	SetSimulatedEveryTick( true );
	SetAnimatedEveryTick( true );
	SetThink(&CReplayManager::Think);
	SetNextThink(gpGlobals->curtime);
}

void CReplayManager::Think()
{
	SetNextThink(gpGlobals->curtime);

	if (!sv_replays.GetBool())
		return;

	if (!GetBall())
		return;

	CheckReplay();
}

void CReplayManager::CheckReplay()
{
	if (m_bDoReplay && gpGlobals->curtime >= m_flStartTime)
		RestoreSnapshot();
	else if (!m_bDoReplay && sv_replay_duration.GetInt() > 0)
		TakeSnapshot();
}

void CReplayManager::StartReplay(int numberOfRuns, float startDelay)
{
	m_nMaxReplayRuns = numberOfRuns;
	m_flStartTime = gpGlobals->curtime + startDelay;
	m_nSnapshotIndex = 0;
	m_nReplayRunCount = 0;
	m_bDoReplay = true;
}

void CReplayManager::StopReplay()
{
	if (!m_bDoReplay)
		return;

	m_bDoReplay = false;
	engine->ServerCommand("host_timescale 1\n");

	UTIL_Remove(m_pBall);
	m_pBall = NULL;

	for (int i = 0; i < 22; i++)
	{
		UTIL_Remove(m_pPlayers[i]);
		m_pPlayers[i] = NULL;
	}

	CBall *pRealBall = GetBall();
	if (pRealBall)
	{
		pRealBall->SetRenderMode(kRenderNormal);
		pRealBall->SetRenderColorA(255);
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pRealPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pRealPl)
			continue;

		pRealPl->SetRenderMode(kRenderNormal);
		pRealPl->SetRenderColorA(255);
	}
}

void CopyAnimationLayer(CAnimationLayer *dst, const CAnimationLayer *src)
{
	dst->m_flCycle = src->m_flCycle;
	dst->m_nOrder = src->m_nOrder;
	dst->m_nSequence = src->m_nSequence;
	dst->m_flWeight = src->m_flWeight;
	dst->m_flPlaybackRate = src->m_flPlaybackRate;
	dst->m_bLooping = src->m_bLooping;
	dst->m_bSequenceFinished = src->m_bSequenceFinished;
	dst->m_fFlags = src->m_fFlags;
	dst->m_nPriority = src->m_nPriority;
	dst->m_nActivity = src->m_nActivity;
	dst->m_flLayerAnimtime = src->m_flLayerAnimtime;
	dst->m_flBlendIn = src->m_flBlendIn;
	dst->m_flBlendOut = src->m_flBlendOut;
	dst->m_flPrevCycle = src->m_flPrevCycle;
	dst->m_flKillDelay = src->m_flKillDelay;
	dst->m_flKillRate = src->m_flKillRate;
	dst->m_flLastAccess = src->m_flLastAccess;
	dst->m_flLastEventCheck = src->m_flLastEventCheck;
	dst->m_flLayerFadeOuttime = src->m_flLayerFadeOuttime;
}

void CReplayManager::TakeSnapshot()
{
	Snapshot snap;
	snap.snaptime = gpGlobals->curtime;

	BallSnapshot *pBallSnap = new BallSnapshot;

	GetBall()->VPhysicsGetObject()->GetPosition(&pBallSnap->pos, &pBallSnap->ang);
	GetBall()->VPhysicsGetObject()->GetVelocity(&pBallSnap->vel, &pBallSnap->rot);
	pBallSnap->skin = GetBall()->m_nSkin;

	snap.pBallSnapshot = pBallSnap;

	for (int i = 0; i < 22; i++)
	{
		snap.pPlayerSnapshot[i] = NULL;
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		PlayerSnapshot *pPlSnap = new PlayerSnapshot;

		pPlSnap->pos = pPl->GetLocalOrigin();
		pPlSnap->vel = pPl->GetLocalVelocity();
		pPlSnap->ang = pPl->GetLocalAngles();

		int layerCount = pPl->GetNumAnimOverlays();
		for( int layerIndex = 0; layerIndex < layerCount; ++layerIndex )
		{
			CAnimationLayer *currentLayer = pPl->GetAnimOverlay(layerIndex);
			if(!currentLayer)
				continue;

			CopyAnimationLayer(&pPlSnap->m_animLayers[layerIndex], currentLayer);
		}

		pPlSnap->m_masterSequence = pPl->GetSequence();
		pPlSnap->m_masterCycle = pPl->GetCycle();
		pPlSnap->m_flSimulationTime = pPl->GetSimulationTime();

		pPlSnap->m_flMoveX = pPl->GetPoseParameter(4);
		pPlSnap->m_flMoveY = pPl->GetPoseParameter(3);

		pPlSnap->m_nTeamNumber = pPl->GetTeamNumber();
		pPlSnap->m_nTeamPosition = pPl->GetTeamPosition();
		pPlSnap->m_nSkin = pPl->m_nSkin;
		pPlSnap->m_nBody = pPl->m_nBody;

		snap.pPlayerSnapshot[pPl->GetTeamPosition() - 1 + (pPl->GetTeamNumber() == TEAM_A ? 0 : 11)] = pPlSnap;
	}

	m_Snapshots.AddToTail(snap);
	
	while (m_Snapshots[0].snaptime < gpGlobals->curtime - sv_replay_duration.GetInt())
	{
		delete m_Snapshots[0].pBallSnapshot;
		for (int i = 0; i < 22; i++)
		{
			delete m_Snapshots[0].pPlayerSnapshot[i];
		}
		m_Snapshots.Remove(0);
	}
}

void CReplayManager::RestoreSnapshot()
{
	if (m_nSnapshotIndex >= m_Snapshots.Count())
	{
		m_nReplayRunCount += 1;

		if (m_nReplayRunCount < m_nMaxReplayRuns)
			m_nSnapshotIndex = 0;
		else
		{
			StopReplay();
			return;
		}
	}

	if (m_Snapshots.Count() == 0)
	{
		StopReplay();
		return;
	}

	CBall *pRealBall = GetBall();
	if (pRealBall)
	{
		pRealBall->SetRenderMode(kRenderTransColor);
		pRealBall->SetRenderColorA(50);
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pRealPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pRealPl))
			continue;

		pRealPl->SetRenderMode(kRenderTransColor);
		pRealPl->SetRenderColorA(50);
	}

	Snapshot *pSnap = &m_Snapshots[m_nSnapshotIndex];

	BallSnapshot *pBallSnap = pSnap->pBallSnapshot;

	if (pBallSnap)
	{
		if (!m_pBall)
		{
			m_pBall = (CReplayBall *)CreateEntityByName("replayball");
			//m_pBall->SetRenderMode(kRenderTransColor);
			//m_pBall->SetRenderColorA(150);
			m_pBall->Spawn();
		}

		m_pBall->m_nSkin = pBallSnap->skin;
		m_pBall->VPhysicsGetObject()->SetPosition(pBallSnap->pos, pBallSnap->ang, false);
		m_pBall->VPhysicsGetObject()->SetVelocity(&pBallSnap->vel, &pBallSnap->rot);
	}
	else
	{
		UTIL_Remove(m_pBall);
		m_pBall = NULL;
	}

	for (int i = 0; i < 22; i++)
	{
		PlayerSnapshot *pPlSnap = pSnap->pPlayerSnapshot[i];
		if (!pPlSnap)
		{
			UTIL_Remove(m_pPlayers[i]);
			m_pPlayers[i] = NULL;
			continue;
		}

		if (!m_pPlayers[i])
		{
			m_pPlayers[i] = (CReplayPlayer *)CreateEntityByName("replayplayer");
			//m_pPlayers[i]->SetRenderMode(kRenderTransColor);
			//m_pPlayers[i]->SetRenderColorA(150);
			m_pPlayers[i]->Spawn();
			//m_pPlayers[i]->UseClientSideAnimation();
		}

		CReplayPlayer *pPl = m_pPlayers[i];

		pPl->SetLocalOrigin(pPlSnap->pos);
		pPl->SetLocalVelocity(pPlSnap->vel);
		pPl->SetLocalAngles(pPlSnap->ang);
		//pPl->SnapEyeAngles(pPlSnap->ang);
		pPl->m_nTeamNumber = pPlSnap->m_nTeamNumber;
		pPl->m_nTeamPosition = pPlSnap->m_nTeamPosition;
		pPl->m_nSkin = pPlSnap->m_nSkin;
		pPl->m_nBody = pPlSnap->m_nBody;

		pPl->SetPoseParameter(pPl->GetModelPtr(), 4, pPlSnap->m_flMoveX);
		pPl->SetPoseParameter(pPl->GetModelPtr(), 3, pPlSnap->m_flMoveY);

		pPl->ResetSequence(pPlSnap->m_masterSequence);
		pPl->SetCycle(pPlSnap->m_masterCycle);
		//pPl->SetSimulationTime(pPlSnap->m_flSimulationTime);
		//pPl->SetPlaybackRate(1);

		//if (m_nSnapshotIndex % 20 == 0)
		{
			int count = pPl->GetNumAnimOverlays();
			int layerCount = 8;//pPl->GetNumAnimOverlays();
			pPl->SetNumAnimOverlays(layerCount);

			for( int layerIndex = 0; layerIndex < layerCount; ++layerIndex )
			{
				CAnimationLayer *currentLayer = pPl->GetAnimOverlay(layerIndex);
				if(!currentLayer)
					continue;

				CopyAnimationLayer(currentLayer, &pPlSnap->m_animLayers[layerIndex]);
			}
		}
	}

	m_nSnapshotIndex += 1;
}