#include "cbase.h"
#include "ios_replaymanager.h"
#include "ball.h"
#include "sdk_gamerules.h"
#include "convar.h"
#include "sdk_playeranimstate.h"
#include "team.h"

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

BEGIN_DATADESC( CReplayBall )
END_DATADESC()

LINK_ENTITY_TO_CLASS( replayball, CReplayBall );

IMPLEMENT_SERVERCLASS_ST(CReplayBall, DT_ReplayBall)
END_SEND_TABLE()

bool CReplayBall::CreateVPhysics()
{
	// Create the object in the physics system
	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal(SOLID_BBOX, FSOLID_NOT_SOLID, false);

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
	SetModel(BALL_MODEL);
	BaseClass::Precache();
}

CReplayBall::CReplayBall()
{

}

void CReplayBall::Spawn( void )
{
	Precache();
	SetModelName(MAKE_STRING(BALL_MODEL));
	CreateVPhysics();
	//SetSolid(SOLID_NONE);
	SetSimulatedEveryTick(true);
	SetAnimatedEveryTick(true);
}


BEGIN_DATADESC( CReplayPlayer )
	DEFINE_THINKFUNC( Think ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( replayplayer, CReplayPlayer );

IMPLEMENT_SERVERCLASS_ST(CReplayPlayer, DT_ReplayPlayer)
	SendPropInt(SENDINFO(m_nTeamNumber)),
	SendPropInt(SENDINFO(m_nTeamPosNum)),
END_SEND_TABLE()

void CReplayPlayer::Precache()
{
	SetModel(SDK_PLAYER_MODEL);
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
	SetSimulatedEveryTick(true);
	SetAnimatedEveryTick(true);
	SetThink(&CReplayPlayer::Think);
	SetNextThink(gpGlobals->curtime);
}

void CReplayPlayer::Think()
{
	SetNextThink( gpGlobals->curtime );
	//BaseClass::Think();
	//SetSimulationTime( gpGlobals->curtime );
	
	//AddGestureSequence(m_AnimLayers[0].m_nSequence);
	//CopyAnimationLayer(GetAnimOverlay(0), &m_AnimLayers[0]);
	//AddGestureSequence(m_AnimLayers[1].m_nSequence);
	//CopyAnimationLayer(GetAnimOverlay(1), &m_AnimLayers[1]);

	//SetAnimatedEveryTick( true );
	StudioFrameAdvance();
	//DispatchAnimEvents(this);
}

void cc_StartReplay(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	ReplayManager()->StartReplay(1, 0, true);
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

IMPLEMENT_SERVERCLASS_ST(CReplayManager, DT_ReplayManager)
	SendPropBool(SENDINFO(m_bIsReplaying)),
	SendPropInt(SENDINFO(m_nReplayRunIndex)),
	SendPropBool(SENDINFO(m_bAtMinGoalPos)),
END_SEND_TABLE()

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

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			m_pPlayers[i][j] = NULL;
		}
	}

	m_bIsReplaying = false;
	m_bIsHighlightReplay = false;
	m_nHighlightReplayIndex = 0;
	m_flRunDuration = 0;
}

CReplayManager::~CReplayManager()
{
	CleanUp();
	g_pReplayManager = NULL;
}

void CReplayManager::CleanUp()
{
	if (m_pBall)
	{
		UTIL_Remove(m_pBall);
	}

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			if (m_pPlayers[i][j])
			{
				UTIL_Remove(m_pPlayers[i][j]);
			}
		}
	}

	m_Snapshots.PurgeAndDeleteElements();
	m_Replays.PurgeAndDeleteElements();
	m_nHighlightReplayIndex = 0;
}

void CReplayManager::Spawn()
{
	SetSimulatedEveryTick(true);
	SetAnimatedEveryTick(true);
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
	else if (!m_bDoReplay && !SDKGameRules()->IsIntermissionState())
		TakeSnapshot();
}

void CReplayManager::StartReplay(int numberOfRuns, float startDelay, bool atMinGoalPos)
{
	if (!sv_replays.GetBool())
		return;

	m_nMaxReplayRuns = numberOfRuns;
	m_flStartTime = gpGlobals->curtime + startDelay;
	m_nReplayRunIndex = 0;
	m_bDoReplay = true;
	m_bAtMinGoalPos = atMinGoalPos;
	SaveReplay();
	m_nReplayIndex = m_Replays.Count() - 1;
	CalcSnapshotIndexRange();
}

void CReplayManager::StopReplay()
{
	if (!sv_replays.GetBool())
		return;

	if (!m_bDoReplay)
		return;

	m_bDoReplay = false;
	m_bIsReplaying = false;

	if (m_pBall)
	{
		UTIL_Remove(m_pBall);
		m_pBall = NULL;
	}

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			if (m_pPlayers[i][j])
			{
				UTIL_Remove(m_pPlayers[i][j]);
				m_pPlayers[i][j] = NULL;
			}
		}
	}

	CBall *pRealBall = GetBall();
	if (pRealBall)
	{
		//pRealBall->SetRenderMode(kRenderNormal);
		//pRealBall->SetRenderColorA(255);
		pRealBall->RemoveEffects(EF_NODRAW);
		pRealBall->RemoveSolidFlags(FSOLID_NOT_SOLID);
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pRealPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pRealPl))
			continue;

		//pRealPl->SetRenderMode(kRenderNormal);
		//pRealPl->SetRenderColorA(255);
		pRealPl->StopObserverMode();
		pRealPl->SetLocalOrigin(pRealPl->GetSpawnPos(true));
		QAngle ang;
		VectorAngles(Vector(0, pRealPl->GetTeam()->m_nForward, 0), ang);
		pRealPl->SetLocalAngles(ang);
		pRealPl->SetLocalVelocity(vec3_origin);
		//pRealPl->SetLocalOrigin(pRealPl->m_vPreReplayPos);
		//pRealPl->SetLocalAngles(pRealPl->m_aPreReplayAngles);
		pRealPl->RemoveEffects(EF_NODRAW);
		//pRealPl->RemoveEFlags(EFL_NOCLIP_ACTIVE);
		pRealPl->SetMoveType(MOVETYPE_WALK);
		pRealPl->RemoveSolidFlags(FSOLID_NOT_SOLID);
		pRealPl->SetViewOffset(VEC_VIEW);

		if (pRealPl->GetPlayerBall())
		{
			pRealPl->GetPlayerBall()->RemoveEffects(EF_NODRAW);
			pRealPl->GetPlayerBall()->RemoveSolidFlags(FSOLID_NOT_SOLID);
		}
	}
}

void CReplayManager::TakeSnapshot()
{
	Snapshot *pSnap = new Snapshot;
	pSnap->snaptime = gpGlobals->curtime;

	BallSnapshot *pBallSnap = new BallSnapshot;

	GetBall()->VPhysicsGetObject()->GetPosition(&pBallSnap->pos, &pBallSnap->ang);
	GetBall()->VPhysicsGetObject()->GetVelocity(&pBallSnap->vel, &pBallSnap->rot);
	pBallSnap->skin = GetBall()->m_nSkin;
	pBallSnap->effects = GetBall()->GetEffects();

	pSnap->pBallSnapshot = pBallSnap;

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			pSnap->pPlayerSnapshot[i][j] = NULL;
		}
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
		pPlSnap->m_nTeamPosNum = pPl->GetTeamPosNum();
		pPlSnap->m_nSkin = pPl->m_nSkin;
		pPlSnap->m_nBody = pPl->m_nBody;

		pSnap->pPlayerSnapshot[pPl->GetTeamNumber() - TEAM_A][pPl->GetTeamPosIndex()] = pPlSnap;
	}

	m_Snapshots.AddToTail(pSnap);
	
	while (m_Snapshots[0]->snaptime < gpGlobals->curtime - max(max(sv_replay_duration1.GetFloat(), sv_replay_duration2.GetFloat()), sv_replay_duration3.GetFloat()))
	{
		//delete m_Snapshots[0]->pBallSnapshot;

		//for (int i = 0; i < 2; i++)
		//{
		//	for (int j = 0; j < 11; j++)
		//	{
		//		delete m_Snapshots[0]->pPlayerSnapshot[i][j];
		//	}
		//}

		delete m_Snapshots[0];

		m_Snapshots.Remove(0);
	}
}

void CReplayManager::RestoreSnapshot()
{
	Replay *pReplay = m_Replays[m_nReplayIndex];

	if (m_nSnapshotCurIndex >= m_nSnapshotEndIndex)
	{
		if (m_nReplayRunIndex < m_nMaxReplayRuns - 1)
		{
			m_nReplayRunIndex += 1;
			CalcSnapshotIndexRange();
		}
		else
		{
			if (m_bIsHighlightReplay && m_nReplayIndex < m_Replays.Count() - 1)
			{
				m_nReplayRunIndex = 0;
				m_nReplayIndex += 1;
				CalcSnapshotIndexRange();
			}
			else
			{
				StopReplay();
			}

			return;
		}
	}

	m_bAtMinGoalPos = pReplay->m_bAtMinGoalPos;

	//if (pReplay->m_Snapshots.Count() == 0)
	//{
	//	StopReplay();
	//	return;
	//}

	m_bIsReplaying = true;

	CBall *pRealBall = GetBall();
	if (pRealBall && !(pRealBall->GetEffects() & EF_NODRAW))
	{
		//pRealBall->SetRenderMode(kRenderTransColor);
		//pRealBall->SetRenderColorA(50);
		pRealBall->AddEffects(EF_NODRAW);
		pRealBall->AddSolidFlags(FSOLID_NOT_SOLID);
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pRealPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pRealPl))
			continue;

		//pRealPl->SetRenderMode(kRenderTransColor);
		//pRealPl->SetRenderColorA(50);
		if (!(pRealPl->GetEffects() & EF_NODRAW))
		{
			pRealPl->m_vPreReplayPos = pRealPl->GetLocalOrigin();
			pRealPl->m_aPreReplayAngles = pRealPl->GetLocalAngles();
			pRealPl->AddEffects(EF_NODRAW);
			pRealPl->DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
			//pRealPl->SetMoveType(MOVETYPE_NOCLIP);
			//pRealPl->AddEFlags(EFL_NOCLIP_ACTIVE);
			pRealPl->AddSolidFlags(FSOLID_NOT_SOLID);
			//if (pRealPl->GetMoveType() != MOVETYPE_OBSERVER)
			pRealPl->SetObserverMode(OBS_MODE_TVCAM);
		}

		if (pRealPl->GetPlayerBall() && !(pRealPl->GetPlayerBall()->GetEffects() & EF_NODRAW))
		{
			pRealPl->GetPlayerBall()->AddEffects(EF_NODRAW);
			pRealPl->GetPlayerBall()->AddSolidFlags(FSOLID_NOT_SOLID);
		}
	}

	Snapshot *pSnap = pReplay->m_Snapshots[m_nSnapshotCurIndex];

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

		m_pBall->SetEffects(pBallSnap->effects);
		m_pBall->m_nSkin = pBallSnap->skin;
		m_pBall->VPhysicsGetObject()->SetPosition(pBallSnap->pos, pBallSnap->ang, false);
		m_pBall->VPhysicsGetObject()->SetVelocity(&pBallSnap->vel, &pBallSnap->rot);
	}
	else
	{
		UTIL_Remove(m_pBall);
		m_pBall = NULL;
	}

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			PlayerSnapshot *pPlSnap = pSnap->pPlayerSnapshot[i][j];
			if (!pPlSnap)
			{
				UTIL_Remove(m_pPlayers[i][j]);
				m_pPlayers[i][j] = NULL;
				continue;
			}

			if (!m_pPlayers[i][j])
			{
				m_pPlayers[i][j] = (CReplayPlayer *)CreateEntityByName("replayplayer");
				//m_pPlayers[i]->SetRenderMode(kRenderTransColor);
				//m_pPlayers[i]->SetRenderColorA(150);
				m_pPlayers[i][j]->Spawn();
				//m_pPlayers[i]->UseClientSideAnimation();
				m_pPlayers[i][j]->SetNumAnimOverlays(NUM_LAYERS_WANTED);
			}

			CReplayPlayer *pPl = m_pPlayers[i][j];

			pPl->SetLocalOrigin(pPlSnap->pos);
			pPl->SetLocalVelocity(pPlSnap->vel);
			pPl->SetLocalAngles(pPlSnap->ang);
			//pPl->SnapEyeAngles(pPlSnap->ang);
			pPl->m_nTeamNumber = pPlSnap->m_nTeamNumber;
			pPl->m_nTeamPosNum = pPlSnap->m_nTeamPosNum;
			pPl->m_nSkin = pPlSnap->m_nSkin;
			pPl->m_nBody = pPlSnap->m_nBody;

			pPl->SetPoseParameter(pPl->GetModelPtr(), 4, pPlSnap->m_flMoveX);
			pPl->SetPoseParameter(pPl->GetModelPtr(), 3, pPlSnap->m_flMoveY);

			pPl->SetSequence(pPlSnap->m_masterSequence);
			pPl->SetCycle(pPlSnap->m_masterCycle);
			pPl->SetSimulationTime(pPlSnap->m_flSimulationTime);
			pPl->SetPlaybackRate(1);

			//CopyAnimationLayer(&pPl->m_AnimLayers[0], &pPlSnap->m_animLayers[0]);
			//CopyAnimationLayer(&pPl->m_AnimLayers[1], &pPlSnap->m_animLayers[1]);

			//if (m_nSnapshotIndex % 20 == 0)
			{
				//int count = pPl->GetNumAnimOverlays();
				//const int layerCount = ;//pPl->GetNumAnimOverlays();
				//pPl->SetNumAnimOverlays(NUM_LAYERS_WANTED);

				for( int layerIndex = 0; layerIndex < NUM_LAYERS_WANTED; ++layerIndex )
				{
					CAnimationLayer *currentLayer = pPl->GetAnimOverlay(layerIndex);
					if(!currentLayer)
						continue;

					//currentLayer->m_flCycle = 0.5f;
					//currentLayer->m_nSequence = 8;
					//currentLayer->m_flPlaybackRate = 1.0;
					//currentLayer->m_flWeight = 1.0f;
					//currentLayer->m_nOrder = layerIndex;

					//pPl->StudioFrameAdvanceManual( gpGlobals->frametime );
					//pPl->DispatchAnimEvents(pPl);
					CopyAnimationLayer(currentLayer, &pPlSnap->m_animLayers[layerIndex]);
					//currentLayer->m_fFlags |= ANIM_LAYER_ACTIVE;
					//currentLayer->MarkActive();
					//currentLayer->StudioFrameAdvance(gpGlobals->frametime, pPl);
				}
				//pPl->StudioFrameAdvance();
				//pPl->DispatchAnimEvents(pPl);
			}
		}
	}

	m_nSnapshotCurIndex += 1;
}

void CReplayManager::SaveReplay()
{
	//Replay replay;
	//replay.m_Snapshots.CopyArray(m_Snapshots.Base(), m_Snapshots.Count());
	//m_Replays.SetCount(1);
	//m_Replays[0] = replay;
	Replay *pReplay = new Replay;
	pReplay->m_Snapshots = m_Snapshots;
	pReplay->m_nMatchSeconds = SDKGameRules()->GetMatchDisplayTimeSeconds();
	pReplay->m_bAtMinGoalPos = m_bAtMinGoalPos;
	//for (int i = 0; i < pReplay->m_Snapshots.Count(); i++)
	//{
	//	pReplay->m_Snapshots[i]->isHighlight = true;
	//}

	m_Replays.AddToTail(pReplay);

	m_Snapshots.RemoveAll();
}

void CReplayManager::StartHighlights()
{
	if (!sv_replays.GetBool() || !sv_highlights.GetBool() || m_nHighlightReplayIndex > m_Replays.Count() - 1)
		return;

	m_bIsHighlightReplay = true;
	m_nReplayIndex = m_nHighlightReplayIndex;
	m_nMaxReplayRuns = 2;
	m_flStartTime = gpGlobals->curtime + 3;
	m_nReplayRunIndex = 0;
	m_bDoReplay = true;
	CalcSnapshotIndexRange();
}

void CReplayManager::StopHighlights()
{
	if (!sv_replays.GetBool() || !sv_highlights.GetBool() || !m_bDoReplay)
		return;

	m_nHighlightReplayIndex = m_nReplayIndex;
	m_bIsHighlightReplay = false;
	StopReplay();
}

void CReplayManager::CalcSnapshotIndexRange()
{
	if (m_nReplayRunIndex == 0)
		m_flRunDuration = sv_replay_duration1.GetFloat();
	else if (m_nReplayRunIndex == 1)
		m_flRunDuration = sv_replay_duration2.GetFloat();
	else if (m_nReplayRunIndex == 2)
		m_flRunDuration = sv_replay_duration3.GetFloat();

	Replay *pReplay = m_Replays[m_nReplayIndex];
	float endTime = pReplay->m_Snapshots.Tail()->snaptime;
	m_nSnapshotEndIndex = pReplay->m_Snapshots.Count() - 1;
	m_nSnapshotStartIndex = 0;

	for (int i = pReplay->m_Snapshots.Count() - 1; i >= 0; i--)
	{
		if (pReplay->m_Snapshots[i]->snaptime < endTime - m_flRunDuration)
		{
			m_nSnapshotStartIndex = i + 1;
			break;
		}
	}

	m_nSnapshotCurIndex = m_nSnapshotStartIndex;
}