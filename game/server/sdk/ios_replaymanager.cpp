#include "cbase.h"
#include "ios_replaymanager.h"
#include "sdk_gamerules.h"
#include "convar.h"
#include "sdk_playeranimstate.h"
#include "team.h"
#include "player_ball.h"
#include "match_ball.h"

BEGIN_DATADESC( CReplayBall )
END_DATADESC()

LINK_ENTITY_TO_CLASS( replayball, CReplayBall );

IMPLEMENT_SERVERCLASS_ST(CReplayBall, DT_ReplayBall)
	SendPropString(SENDINFO(m_szSkinName)),
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
	m_szSkinName.GetForModify()[0] = '\0';
}

void CReplayBall::Spawn( void )
{
	SetClassname("replay_ball");
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
	SendPropInt(SENDINFO(m_nTeamNumber), 3),
	SendPropInt(SENDINFO(m_nTeamPosIndex), 4, SPROP_UNSIGNED),
	SendPropBool(SENDINFO(m_bIsKeeper)),
	SendPropInt(SENDINFO(m_nShirtNumber), 7, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nSkinIndex), 3, SPROP_UNSIGNED),
	SendPropString(SENDINFO(m_szPlayerName)),
	SendPropString(SENDINFO(m_szShirtName)),
END_SEND_TABLE()

void CReplayPlayer::Precache()
{
	SetModel(SDK_PLAYER_MODEL);
	BaseClass::Precache();
}

CReplayPlayer::CReplayPlayer()
{
	m_nTeamNumber = 0;
	m_nTeamPosIndex = 0;
	m_bIsKeeper = false;
	m_nShirtNumber = 2;
	m_nSkinIndex = 0;
	m_szPlayerName.GetForModify()[0] = '\0';
	m_szShirtName.GetForModify()[0] = '\0';
}

void CReplayPlayer::Spawn( void )
{
	SetClassname("replay_player");
	Precache();
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
	StudioFrameAdvance();
}

LINK_ENTITY_TO_CLASS(replaymanager, CReplayManager);

IMPLEMENT_SERVERCLASS_ST(CReplayManager, DT_ReplayManager)
	SendPropBool(SENDINFO(m_bIsReplaying)),
	SendPropInt(SENDINFO(m_nReplayRunIndex), 3, SPROP_UNSIGNED),
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

	m_pBall = NULL;

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			m_pPlayers[i][j] = NULL;
		}
	}

	m_bIsReplaying = false;
	m_bReplayIsPending = false;
	m_bIsHighlightReplay = false;
	m_bIsReplayStart = true;
	m_bIsHighlightStart = true;
	m_flRunningTime = 0;
}

CReplayManager::~CReplayManager()
{
	CleanUp();
	g_pReplayManager = NULL;
}

void CReplayManager::CleanUp()
{
	StopHighlights();
	StopReplay();

	while (m_Snapshots.Count() > 0)
	{
		if (m_Snapshots[0]->replayCount == 0)
			delete m_Snapshots[0];

		m_Snapshots.Remove(0);
	}

	m_MatchEvents.PurgeAndDeleteElements();
}

void CReplayManager::Spawn()
{
	SetClassname("replay_manager");
	SetSimulatedEveryTick(true);
	SetAnimatedEveryTick(true);
	SetThink(&CReplayManager::Think);
	SetNextThink(gpGlobals->curtime);
}

void CReplayManager::Think()
{
	SetNextThink(gpGlobals->curtime);

	if (!sv_replay_instant_enabled.GetBool() && !sv_replay_highlight_enabled.GetBool())
		return;

	if (!GetMatchBall())
		return;

	CheckReplay();
}

bool matchEventFitsMatchPeriod(const MatchEvent *pMatchEvent, match_period_t matchPeriod)
{
	// Instant replays while match is running
	if (matchPeriod == pMatchEvent->matchPeriod)
		return true;

	// All replays after match end
	if (matchPeriod == MATCH_PERIOD_COOLDOWN)
		return true;

	if (matchPeriod == MATCH_PERIOD_HALFTIME && pMatchEvent->matchPeriod == MATCH_PERIOD_FIRST_HALF)
		return true;

	if (matchPeriod == MATCH_PERIOD_EXTRATIME_INTERMISSION && pMatchEvent->matchPeriod == MATCH_PERIOD_SECOND_HALF)
		return true;

	if (matchPeriod == MATCH_PERIOD_EXTRATIME_HALFTIME && pMatchEvent->matchPeriod == MATCH_PERIOD_EXTRATIME_FIRST_HALF)
		return true;

	if (matchPeriod == MATCH_PERIOD_PENALTIES_INTERMISSION
		&& (mp_extratime.GetBool() && pMatchEvent->matchPeriod == MATCH_PERIOD_EXTRATIME_SECOND_HALF || !mp_extratime.GetBool() && pMatchEvent->matchPeriod == MATCH_PERIOD_SECOND_HALF))
		return true;

	return false;
}

int CReplayManager::FindNextHighlightReplayIndex(int startIndex, match_period_t matchPeriod)
{
	if (startIndex == -1)
	{
		// Iterate backwards
		for (int i = m_MatchEvents.Count() - 1; i >= 0; i--)
		{
			if (m_MatchEvents[i]->snapshots.Count() > 0 && matchEventFitsMatchPeriod(m_MatchEvents[i], matchPeriod))
				return i;
		}
	}
	else if (startIndex >= 0 && startIndex < m_MatchEvents.Count())
	{
		// Iterate forwards
		for (int i = startIndex; i < m_MatchEvents.Count(); i++)
		{
			if (m_MatchEvents[i]->snapshots.Count() > 0 && matchEventFitsMatchPeriod(m_MatchEvents[i], matchPeriod))
				return i;
		}
	}

	return -1;
}

void CReplayManager::CheckReplay()
{
	if (m_bReplayIsPending && gpGlobals->curtime >= m_flReplayActivationTime || m_bIsReplaying)
		PlayReplay();
	else if (!m_bIsReplaying && !SDKGameRules()->IsIntermissionState()
		&& (!m_bReplayIsPending || m_MatchEvents.Count() > 0 && m_MatchEvents.Tail()->snapshotEndTime >= gpGlobals->curtime))
		TakeSnapshot();
}

extern ConVar sv_ball_goalcelebduration;
extern ConVar sv_ball_highlightsdelay_intermissions;
extern ConVar sv_ball_highlightsdelay_cooldown;

void CReplayManager::StartReplay(bool isHighlightReplay)
{
	m_bIsHighlightReplay = isHighlightReplay;

	if (m_bIsHighlightReplay && !sv_replay_highlight_enabled.GetBool() || !m_bIsHighlightReplay && !sv_replay_instant_enabled.GetBool())
		return;

	m_nReplayIndex = FindNextHighlightReplayIndex((m_bIsHighlightReplay ? 0 : -1), SDKGameRules()->State_Get());

	if (m_nReplayIndex == -1)
		return;

	MatchEvent *pMatchEvent = m_MatchEvents[m_nReplayIndex];

	m_bIsHighlightStart = true;
	m_bIsReplayStart = true;

	float replayActivationDelay;

	if (m_bIsHighlightReplay)
		replayActivationDelay = SDKGameRules()->State_Get() == MATCH_PERIOD_COOLDOWN ? sv_ball_highlightsdelay_cooldown.GetFloat() : sv_ball_highlightsdelay_intermissions.GetFloat();
	else
		replayActivationDelay = sv_ball_goalcelebduration.GetFloat();

	m_flReplayActivationTime = gpGlobals->curtime + replayActivationDelay;
	m_nReplayRunIndex = 0;
	m_bReplayIsPending = true;
	m_bIsReplaying = false;
	CalcMaxReplayRunsAndDuration(pMatchEvent, m_flReplayActivationTime);
}

void CReplayManager::StopReplay()
{
	if (m_bReplayIsPending)
		m_bReplayIsPending = false;

	if (!m_bIsReplaying)
		return;

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

	if (IsMarkedForDeletion())
		return;

	CBall *pRealBall = GetMatchBall();
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
		pRealPl->SetLocalOrigin(pRealPl->GetSpawnPos());
		QAngle ang;
		VectorAngles(Vector(0, pRealPl->GetTeam()->m_nForward, 0), ang);
		pRealPl->SetLocalAngles(ang);
		pRealPl->SnapEyeAngles(ang);
		pRealPl->SetLocalVelocity(vec3_origin);
		pRealPl->RemoveEffects(EF_NODRAW);
		//pRealPl->RemoveEFlags(EFL_NOCLIP_ACTIVE);
		pRealPl->SetMoveType(MOVETYPE_WALK);
		pRealPl->RemoveSolidFlags(FSOLID_NOT_SOLID);

		if (pRealPl->GetPlayerBall())
		{
			pRealPl->GetPlayerBall()->RemoveEffects(EF_NODRAW);
			pRealPl->GetPlayerBall()->RemoveSolidFlags(FSOLID_NOT_SOLID);
		}
	}

	if (SDKGameRules()->IsIntermissionState())
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent("match_period");
		if (pEvent)
		{
			pEvent->SetInt("period", SDKGameRules()->State_Get());
			gameeventmanager->FireEvent(pEvent);
		}
	}
}

void CReplayManager::TakeSnapshot()
{
	Snapshot *pSnap = new Snapshot;
	pSnap->snaptime = gpGlobals->curtime;
	pSnap->replayCount = 0;

	BallSnapshot *pBallSnap = new BallSnapshot;

	GetMatchBall()->VPhysicsGetObject()->GetPosition(&pBallSnap->pos, &pBallSnap->ang);
	GetMatchBall()->VPhysicsGetObject()->GetVelocity(&pBallSnap->vel, &pBallSnap->rot);

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

		for(int layerIndex = 0; layerIndex < NUM_LAYERS_WANTED; layerIndex++)
		{
			const CAnimationLayer *pSourceLayer = pPl->GetAnimOverlay(layerIndex);
			if(!pSourceLayer)
				continue;

			LayerRecord *pTargetLayer = &pPlSnap->layerRecords[layerIndex];
			if(!pTargetLayer)
				continue;

			pTargetLayer->cycle = pSourceLayer->m_flCycle;
			pTargetLayer->order = pSourceLayer->m_nOrder;
			pTargetLayer->sequence = pSourceLayer->m_nSequence;
			pTargetLayer->weight = pSourceLayer->m_flWeight;
			pTargetLayer->playbackRate = pSourceLayer->m_flPlaybackRate;
			pTargetLayer->looping = pSourceLayer->m_bLooping;
			pTargetLayer->sequenceFinished = pSourceLayer->m_bSequenceFinished;
			pTargetLayer->flags = pSourceLayer->m_fFlags;
			pTargetLayer->priority = pSourceLayer->m_nPriority;
			pTargetLayer->activity = pSourceLayer->m_nActivity;
			pTargetLayer->layerAnimtime = pSourceLayer->m_flLayerAnimtime;
			pTargetLayer->blendIn = pSourceLayer->m_flBlendIn;
			pTargetLayer->blendOut = pSourceLayer->m_flBlendOut;
			pTargetLayer->prevCycle = pSourceLayer->m_flPrevCycle;
			pTargetLayer->killDelay = pSourceLayer->m_flKillDelay;
			pTargetLayer->killRate = pSourceLayer->m_flKillRate;
			pTargetLayer->lastAccess = pSourceLayer->m_flLastAccess;
			pTargetLayer->lastEventCheck = pSourceLayer->m_flLastEventCheck;
			pTargetLayer->layerFadeOuttime = pSourceLayer->m_flLayerFadeOuttime;
		}

		pPlSnap->masterSequence = pPl->GetSequence();
		pPlSnap->masterCycle = pPl->GetCycle();
		pPlSnap->simulationTime = pPl->GetSimulationTime();

		pPlSnap->moveX = pPl->GetPoseParameter(4);
		pPlSnap->moveY = pPl->GetPoseParameter(3);

		pPlSnap->teamNumber = pPl->GetTeamNumber();
		pPlSnap->teamPosIndex = pPl->GetTeamPosIndex();
		pPlSnap->isKeeper = pPl->GetTeamPosType() == POS_GK;
		pPlSnap->shirtNumber = pPl->GetShirtNumber();
		pPlSnap->skinIndex = pPl->GetSkinIndex();
		pPlSnap->body = pPl->m_nBody;

		pPlSnap->pPlayerData = pPl->GetPlayerData();

		pSnap->pPlayerSnapshot[pPl->GetTeamNumber() - TEAM_HOME][pPl->GetTeamPosIndex()] = pPlSnap;
	}

	m_Snapshots.AddToTail(pSnap);

	// Add the snapshot to existing highlights if their end time hasn't been reached yet
	for (int i = 0; i < m_MatchEvents.Count(); i++)
	{
		if (m_MatchEvents[i]->snapshotEndTime >= gpGlobals->curtime)
		{
			m_MatchEvents[i]->snapshots.AddToTail(pSnap);
			pSnap->replayCount += 1;
		}
	}
	
	float recordStartTime = gpGlobals->curtime - GetLongestReplayDuration();

	// Remove old snapshots from the list. Only free a snapshot's memory if it isn't part of any highlight.
	while (m_Snapshots.Count() >= 2 && m_Snapshots[0]->snaptime < recordStartTime && m_Snapshots[1]->snaptime <= recordStartTime)
	{
		if (m_Snapshots[0]->replayCount == 0)
			delete m_Snapshots[0];

		m_Snapshots.Remove(0);
	}
}

void CReplayManager::PlayReplay()
{
	float elapsedPlayTime = gpGlobals->curtime - m_flReplayStartTime;

	if (elapsedPlayTime > m_flRunningTime)
	{
		if (!FindNextReplay())
			return;
	}

	MatchEvent *pMatchEvent = m_MatchEvents[m_nReplayIndex];

	if (m_bIsReplayStart)
	{
		InitReplay(pMatchEvent);
		elapsedPlayTime = 0;
	}

	HideRealBallAndPlayers();

	float realTimeDuration = m_flRunningTime - m_flSlowMoDuration;

	float elapsedSlowMoTime = elapsedPlayTime - realTimeDuration;

	if (elapsedSlowMoTime > 0)
		elapsedPlayTime = realTimeDuration + elapsedSlowMoTime * m_flSlowMoCoeff;

	// To find the correct snapshot calculate the time passed since we started watching the replay and match it to the right snapshot in our recording list

	Snapshot *pSnap = NULL;
	Snapshot *pNextSnap = NULL;
	float relativeSnapTime = 0;

	// Traverse backwards looking for a recorded snapshot matching the time since replay start
	for (int i = pMatchEvent->snapshots.Count() - 1; i >= 0; i--)
	{
		// Save the snapshot of the previous iteration, so we have a snapshot to interpolate to when we found our target snapshot
		pNextSnap = pSnap;

		// Save the current snapshot
		pSnap = pMatchEvent->snapshots[i];

		// Snapshots have absolute match times, so calculate the relative time span between the first recorded snapshot and the current snapshot
		relativeSnapTime = pSnap->snaptime - pMatchEvent->snapshots[0]->snaptime - m_flReplayStartTimeOffset;

		// We usually only play the last x seconds of a replay instead of the whole thing, so subtract the start time offset from the time since the first snapshot.
		// The first snapshot which time span is equal or shorter than the duration since replay start is the one which should be shown next.
		if (relativeSnapTime <= elapsedPlayTime)
			break;
	}

	// No snapshots in the list
	if (!pSnap)
		return;

	float nextSnapDuration;

	if (pNextSnap)
		nextSnapDuration = pNextSnap->snaptime - pMatchEvent->snapshots[0]->snaptime - m_flReplayStartTimeOffset;
	else
		nextSnapDuration = 0;

	float interpolant;

	if (pNextSnap)
	{
		// Calc fraction between both snapshots
		interpolant = clamp((elapsedPlayTime - relativeSnapTime) / (nextSnapDuration - relativeSnapTime), 0.0f, 1.0f);
	}
	else
	{
		// Exact snapshot time matched or no next snapshot to interpolate to
		interpolant = 0.0f;
	}

	RestoreReplayBallState(pSnap, pNextSnap, interpolant);
	RestoreReplayPlayerStates(pSnap, pNextSnap, interpolant);
}

bool CReplayManager::FindNextReplay()
{
	if (m_nReplayRunIndex < m_nMaxReplayRuns - 1)
	{
		m_nReplayRunIndex += 1;
		m_bIsReplayStart = true;
	}
	else if (m_bIsHighlightReplay)
	{
		m_nReplayIndex = FindNextHighlightReplayIndex(m_nReplayIndex + 1, SDKGameRules()->State_Get());

		if (m_nReplayIndex == -1)
		{
			StopReplay();
			return false;
		}

		m_nReplayRunIndex = 0;
		m_bIsReplayStart = true;
		m_bIsHighlightStart = true;
	}
	else
	{
		StopReplay();
		return false;
	}

	return true;
}

void CReplayManager::InitReplay(MatchEvent *pMatchEvent)
{
	m_bIsReplayStart = false;
	m_bAtMinGoalPos = pMatchEvent->atMinGoalPos;
	m_bIsReplaying = true;
	m_bReplayIsPending = false;
	CalcMaxReplayRunsAndDuration(pMatchEvent, gpGlobals->curtime);

	if (m_bIsHighlightReplay && m_bIsHighlightStart)
	{
		m_bIsHighlightStart = false;

		if (pMatchEvent->matchEventType == MATCH_EVENT_GOAL)
		{
			IGameEvent *pEvent = gameeventmanager->CreateEvent("highlight_goal");
			if (pEvent)
			{
				pEvent->SetInt("second", pMatchEvent->second);
				pEvent->SetInt("match_period", pMatchEvent->matchPeriod);
				pEvent->SetInt("scoring_team", pMatchEvent->team);
				pEvent->SetString("scorer", pMatchEvent->pPlayer1Data ? pMatchEvent->pPlayer1Data->m_szName : "");
				pEvent->SetString("first_assister", pMatchEvent->pPlayer2Data ? pMatchEvent->pPlayer2Data->m_szName : "");
				pEvent->SetString("second_assister", pMatchEvent->pPlayer3Data ? pMatchEvent->pPlayer3Data->m_szName : "");
				gameeventmanager->FireEvent(pEvent);
			}
		}
		else if (pMatchEvent->matchEventType == MATCH_EVENT_OWNGOAL)
		{
			IGameEvent *pEvent = gameeventmanager->CreateEvent("highlight_owngoal");
			if (pEvent)
			{
				pEvent->SetInt("second", pMatchEvent->second);
				pEvent->SetInt("match_period", pMatchEvent->matchPeriod);
				pEvent->SetInt("scoring_team", pMatchEvent->team);
				pEvent->SetString("scorer", pMatchEvent->pPlayer1Data ? pMatchEvent->pPlayer1Data->m_szName : "");
				gameeventmanager->FireEvent(pEvent);
			}
		}
		else if (pMatchEvent->matchEventType == MATCH_EVENT_MISS)
		{
			IGameEvent *pEvent = gameeventmanager->CreateEvent("highlight_miss");
			if (pEvent)
			{
				pEvent->SetInt("second", pMatchEvent->second);
				pEvent->SetInt("match_period", pMatchEvent->matchPeriod);
				pEvent->SetInt("finishing_team", pMatchEvent->team);
				pEvent->SetString("finisher", pMatchEvent->pPlayer1Data ? pMatchEvent->pPlayer1Data->m_szName : "");
				pEvent->SetString("first_assister", pMatchEvent->pPlayer2Data ? pMatchEvent->pPlayer2Data->m_szName : "");
				pEvent->SetString("second_assister", pMatchEvent->pPlayer3Data ? pMatchEvent->pPlayer3Data->m_szName : "");
				gameeventmanager->FireEvent(pEvent);
			}
		}
		else if (pMatchEvent->matchEventType == MATCH_EVENT_KEEPERSAVE)
		{
			IGameEvent *pEvent = gameeventmanager->CreateEvent("highlight_keepersave");
			if (pEvent)
			{
				pEvent->SetInt("second", pMatchEvent->second);
				pEvent->SetInt("match_period", pMatchEvent->matchPeriod);
				pEvent->SetInt("keeper_team", pMatchEvent->team);
				pEvent->SetString("keeper", pMatchEvent->pPlayer1Data ? pMatchEvent->pPlayer1Data->m_szName : "");
				pEvent->SetString("finisher", pMatchEvent->pPlayer2Data ? pMatchEvent->pPlayer2Data->m_szName : "");
				gameeventmanager->FireEvent(pEvent);
			}
		}
		else if (pMatchEvent->matchEventType == MATCH_EVENT_REDCARD)
		{
			IGameEvent *pEvent = gameeventmanager->CreateEvent("highlight_redcard");
			if (pEvent)
			{
				pEvent->SetInt("second", pMatchEvent->second);
				pEvent->SetInt("match_period", pMatchEvent->matchPeriod);
				pEvent->SetInt("fouling_team", pMatchEvent->team);
				pEvent->SetString("fouling_player", pMatchEvent->pPlayer1Data ? pMatchEvent->pPlayer1Data->m_szName : "");
				gameeventmanager->FireEvent(pEvent);
			}
		}
	}
}

void CReplayManager::HideRealBallAndPlayers()
{
	CBall *pRealBall = GetMatchBall();
	if (pRealBall && !(pRealBall->GetEffects() & EF_NODRAW))
	{
		pRealBall->AddEffects(EF_NODRAW);
		pRealBall->AddSolidFlags(FSOLID_NOT_SOLID);
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pRealPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pRealPl))
			continue;

		if (!(pRealPl->GetEffects() & EF_NODRAW))
		{
			pRealPl->AddEffects(EF_NODRAW);
			pRealPl->DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
			pRealPl->AddSolidFlags(FSOLID_NOT_SOLID);
			pRealPl->SetMoveType(MOVETYPE_NONE);
		}

		if (pRealPl->GetPlayerBall() && !(pRealPl->GetPlayerBall()->GetEffects() & EF_NODRAW))
		{
			pRealPl->GetPlayerBall()->AddEffects(EF_NODRAW);
			pRealPl->GetPlayerBall()->AddSolidFlags(FSOLID_NOT_SOLID);
		}
	}
}

void CReplayManager::RestoreReplayBallState(Snapshot *pSnap, Snapshot *pNextSnap, float interpolant)
{
	BallSnapshot *pBallSnap = pSnap->pBallSnapshot;

	if (pBallSnap)
	{
		if (!m_pBall)
		{
			m_pBall = (CReplayBall *)CreateEntityByName("replayball");
			m_pBall->Spawn();
		}

		if (Q_strcmp(m_pBall->m_szSkinName, GetMatchBall()->GetSkinName()))
			Q_strncpy(m_pBall->m_szSkinName.GetForModify(), GetMatchBall()->GetSkinName(), MAX_KITNAME_LENGTH);

		BallSnapshot *pNextBallSnap = NULL;
		
		if (pNextSnap)
			pNextBallSnap = pNextSnap->pBallSnapshot;

		if (interpolant > 0.0f && pNextBallSnap)
		{
			m_pBall->VPhysicsGetObject()->SetPosition(Lerp(interpolant, pBallSnap->pos, pNextBallSnap->pos), Lerp(interpolant, pBallSnap->ang, pNextBallSnap->ang), false);
			m_pBall->VPhysicsGetObject()->SetVelocity(&Lerp(interpolant, pBallSnap->vel, pNextBallSnap->vel), &Lerp(interpolant, pBallSnap->rot, pNextBallSnap->rot));
		}
		else
		{
			m_pBall->VPhysicsGetObject()->SetPosition(pBallSnap->pos, pBallSnap->ang, false);
			m_pBall->VPhysicsGetObject()->SetVelocity(&pBallSnap->vel, &pBallSnap->rot);
		}
	}
	else
	{
		UTIL_Remove(m_pBall);
		m_pBall = NULL;
	}
}

void CReplayManager::RestoreReplayPlayerStates(Snapshot *pSnap, Snapshot *pNextSnap, float interpolant)
{
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
				m_pPlayers[i][j]->Spawn();
				m_pPlayers[i][j]->SetNumAnimOverlays(NUM_LAYERS_WANTED);
			}

			CReplayPlayer *pPl = m_pPlayers[i][j];

			pPl->m_nTeamNumber = pPlSnap->teamNumber;
			pPl->m_nTeamPosIndex = pPlSnap->teamPosIndex;
			pPl->m_bIsKeeper = pPlSnap->isKeeper;
			pPl->m_nShirtNumber = pPlSnap->shirtNumber;

			if (Q_strcmp(pPl->m_szPlayerName, pPlSnap->pPlayerData->m_szName))
				Q_strncpy(pPl->m_szPlayerName.GetForModify(), pPlSnap->pPlayerData->m_szName, MAX_PLAYER_NAME_LENGTH);

			if (Q_strcmp(pPl->m_szShirtName, pPlSnap->pPlayerData->m_szShirtName))
				Q_strncpy(pPl->m_szShirtName.GetForModify(), pPlSnap->pPlayerData->m_szShirtName, MAX_PLAYER_NAME_LENGTH);

			pPl->m_nSkinIndex = pPlSnap->skinIndex;
			pPl->m_nBody = pPlSnap->body;

			PlayerSnapshot *pNextPlSnap = NULL;

			if (pNextSnap)
				pNextPlSnap = pNextSnap->pPlayerSnapshot[i][j];

			if (interpolant > 0.0f && pNextPlSnap)
			{
				pPl->SetLocalOrigin(Lerp(interpolant, pPlSnap->pos, pNextPlSnap->pos));
				//pPl->SetLocalVelocity(Lerp( interpolant, pPlSnap->vel, pNextPlSnap->vel  ));
				pPl->SetLocalAngles(Lerp(interpolant, pPlSnap->ang, pNextPlSnap->ang));
			}
			else
			{
				pPl->SetLocalOrigin(pPlSnap->pos);
				//pPl->SetLocalVelocity(pPlSnap->vel);
				pPl->SetLocalAngles(pPlSnap->ang);
			}

			bool interpolationAllowed;

			if (interpolant > 0.0f && pNextPlSnap && pPlSnap->masterSequence == pNextPlSnap->masterSequence)
			{
				// If the master state changes, all layers will be invalid too, so don't interp (ya know, interp barely ever happens anyway)
				interpolationAllowed = true;
			}
			else
				interpolationAllowed = false;

			// First do the master settings
			if (interpolationAllowed)
			{
				pPl->SetSequence(Lerp(interpolant, pPlSnap->masterSequence, pNextPlSnap->masterSequence));
				pPl->SetCycle(Lerp(interpolant, pPlSnap->masterCycle, pNextPlSnap->masterCycle));

				if (pPlSnap->masterCycle > pNextPlSnap->masterCycle)
				{
					// the older record is higher in frame than the newer, it must have wrapped around from 1 back to 0
					// add one to the newer so it is lerping from .9 to 1.1 instead of .9 to .1, for example.
					float newCycle = Lerp(interpolant, pPlSnap->masterCycle, pNextPlSnap->masterCycle + 1);
					pPl->SetCycle(newCycle < 1 ? newCycle : newCycle - 1);// and make sure .9 to 1.2 does not end up 1.05
				}
				else
				{
					pPl->SetCycle(Lerp(interpolant, pPlSnap->masterCycle, pNextPlSnap->masterCycle));
				}

				pPl->SetPoseParameter(pPl->GetModelPtr(), 4, Lerp(interpolant, pPlSnap->moveX, pNextPlSnap->moveX));
				pPl->SetPoseParameter(pPl->GetModelPtr(), 3, Lerp(interpolant, pPlSnap->moveY, pNextPlSnap->moveY));
			}
			else
			{
				pPl->SetSequence(pPlSnap->masterSequence);
				pPl->SetCycle(pPlSnap->masterCycle);
				pPl->SetPoseParameter(pPl->GetModelPtr(), 4, pPlSnap->moveX);
				pPl->SetPoseParameter(pPl->GetModelPtr(), 3, pPlSnap->moveY);
			}

			// Now do all the layers
			for (int layerIndex = 0; layerIndex < NUM_LAYERS_WANTED; layerIndex++)
			{
				CAnimationLayer *pTargetLayer = pPl->GetAnimOverlay(layerIndex);
				if (!pTargetLayer)
					continue;

				LayerRecord *pSourceLayer = &pPlSnap->layerRecords[layerIndex];

				pTargetLayer->m_nOrder = pSourceLayer->order;
				pTargetLayer->m_nSequence = pSourceLayer->sequence;
				pTargetLayer->m_flPlaybackRate = pSourceLayer->playbackRate;
				pTargetLayer->m_fFlags = pSourceLayer->flags;
				pTargetLayer->m_bLooping = pSourceLayer->looping;
				pTargetLayer->m_bSequenceFinished = pSourceLayer->sequenceFinished;
				pTargetLayer->m_nPriority = pSourceLayer->priority;
				pTargetLayer->m_nActivity = pSourceLayer->activity;
				pTargetLayer->m_flLastAccess = pSourceLayer->lastAccess;
				pTargetLayer->m_flLastEventCheck = pSourceLayer->lastEventCheck;

				if (interpolationAllowed)
				{
					LayerRecord *pNextSourceLayer = &pNextPlSnap->layerRecords[layerIndex];

					if (pSourceLayer->order == pNextSourceLayer->order && pSourceLayer->sequence == pNextSourceLayer->sequence)
					{
						// We can't interpolate across a sequence or order change
						if (pSourceLayer->cycle > pNextSourceLayer->cycle)
						{
							// the older record is higher in frame than the newer, it must have wrapped around from 1 back to 0
							// add one to the newer so it is lerping from .9 to 1.1 instead of .9 to .1, for example.
							float newCycle = Lerp(interpolant, pSourceLayer->cycle, pNextSourceLayer->cycle + 1);
							pTargetLayer->m_flCycle = newCycle < 1 ? newCycle : newCycle - 1;// and make sure .9 to 1.2 does not end up 1.05
						}
						else
						{
							pTargetLayer->m_flCycle = Lerp(interpolant, pSourceLayer->cycle, pNextSourceLayer->cycle);
						}

						pTargetLayer->m_flWeight = Lerp(interpolant, pSourceLayer->weight, pNextSourceLayer->weight);
						pTargetLayer->m_flLayerAnimtime = Lerp(interpolant, pSourceLayer->layerAnimtime, pNextSourceLayer->layerAnimtime);
						pTargetLayer->m_flBlendIn = Lerp(interpolant, pSourceLayer->blendIn, pNextSourceLayer->blendIn);
						pTargetLayer->m_flBlendOut = Lerp(interpolant, pSourceLayer->blendOut, pNextSourceLayer->blendOut);
						pTargetLayer->m_flPrevCycle = Lerp(interpolant, pSourceLayer->prevCycle, pNextSourceLayer->prevCycle);
						pTargetLayer->m_flKillDelay = Lerp(interpolant, pSourceLayer->killDelay, pNextSourceLayer->killDelay);
						pTargetLayer->m_flKillRate = Lerp(interpolant, pSourceLayer->killRate, pNextSourceLayer->killRate);
						pTargetLayer->m_flLayerFadeOuttime = Lerp(interpolant, pSourceLayer->layerFadeOuttime, pNextSourceLayer->layerFadeOuttime);
					}
				}
				else
				{
					//Either no interp, or interp failed.  Just use record.
					pTargetLayer->m_flCycle = pSourceLayer->cycle;
					pTargetLayer->m_flWeight = pSourceLayer->weight;
					pTargetLayer->m_flLayerAnimtime = pSourceLayer->layerAnimtime;
					pTargetLayer->m_flBlendIn = pSourceLayer->blendIn;
					pTargetLayer->m_flBlendOut = pSourceLayer->blendOut;
					pTargetLayer->m_flPrevCycle = pSourceLayer->prevCycle;
					pTargetLayer->m_flKillDelay = pSourceLayer->killDelay;
					pTargetLayer->m_flKillRate = pSourceLayer->killRate;
					pTargetLayer->m_flLayerFadeOuttime = pSourceLayer->layerFadeOuttime;
				}
			}
		}
	}
}

void CReplayManager::StartHighlights()
{
	StartReplay(true);
}

void CReplayManager::StopHighlights()
{
	StopReplay();
}

void CReplayManager::CalcMaxReplayRunsAndDuration(const MatchEvent *pMatchEvent, float startTime)
{
	if (SDKGameRules()->IsIntermissionState())
	{
		m_nMaxReplayRuns = 0;

		if (sv_replay_highlight_first_enabled.GetBool())
		{
			m_nMaxReplayRuns = 1;

			if (sv_replay_highlight_second_enabled.GetBool())
			{
				m_nMaxReplayRuns = 2;

				if (sv_replay_highlight_second_enabled.GetBool())
					m_nMaxReplayRuns = 3;
			}
		}

		switch (m_nReplayRunIndex)
		{
		case 0:
		default:
			m_flRunningTime = sv_replay_highlight_first_duration.GetFloat();
			m_flSlowMoDuration = sv_replay_highlight_first_slowmo_duration.GetFloat();
			m_flSlowMoCoeff = sv_replay_highlight_first_slowmo_coeff.GetFloat();
			break;
		case 1:
			m_flRunningTime = sv_replay_highlight_second_duration.GetFloat();
			m_flSlowMoDuration = sv_replay_highlight_second_slowmo_duration.GetFloat();
			m_flSlowMoCoeff = sv_replay_highlight_second_slowmo_coeff.GetFloat();
			break;
		case 2:
			m_flRunningTime = sv_replay_highlight_third_duration.GetFloat();
			m_flSlowMoDuration = sv_replay_highlight_third_slowmo_duration.GetFloat();
			m_flSlowMoCoeff = sv_replay_highlight_third_slowmo_coeff.GetFloat();
			break;
		}
	}
	else
	{
		m_nMaxReplayRuns = 0;

		if (sv_replay_instant_first_enabled.GetBool())
		{
			m_nMaxReplayRuns = 1;

			if (sv_replay_instant_second_enabled.GetBool())
			{
				m_nMaxReplayRuns = 2;

				if (sv_replay_instant_second_enabled.GetBool())
					m_nMaxReplayRuns = 3;
			}
		}

		switch (m_nReplayRunIndex)
		{
		case 0:
		default:
			m_flRunningTime = sv_replay_instant_first_duration.GetFloat();
			m_flSlowMoDuration = sv_replay_instant_first_slowmo_duration.GetFloat();
			m_flSlowMoCoeff = sv_replay_instant_first_slowmo_coeff.GetFloat();
			break;
		case 1:
			m_flRunningTime = sv_replay_instant_second_duration.GetFloat();
			m_flSlowMoDuration = sv_replay_instant_second_slowmo_duration.GetFloat();
			m_flSlowMoCoeff = sv_replay_instant_second_slowmo_coeff.GetFloat();
			break;
		case 2:
			m_flRunningTime = sv_replay_instant_third_duration.GetFloat();
			m_flSlowMoDuration = sv_replay_instant_third_slowmo_duration.GetFloat();
			m_flSlowMoCoeff = sv_replay_instant_third_slowmo_coeff.GetFloat();
			break;
		}
	}

	m_flReplayStartTime = startTime;

	// If the new replay duration is shorter than the recorded time span, set the offset so it starts playing later and finishes with the last snapshot.
	// The recorded duration is the maximum of all duration convars
	m_flReplayStartTimeOffset = GetLongestReplayDuration() - m_flRunningTime;

	// Compensate for slow mo
	m_flReplayStartTimeOffset += (1.0f - m_flSlowMoCoeff) * m_flSlowMoDuration;
}

void CReplayManager::AddMatchEvent(match_event_t type, int team, CSDKPlayer *pPlayer1, CSDKPlayer *pPlayer2/* = NULL*/, CSDKPlayer *pPlayer3/* = NULL*/)
{
	MatchEvent *pMatchEvent = new MatchEvent;

	if (type == MATCH_EVENT_GOAL || type == MATCH_EVENT_OWNGOAL || type == MATCH_EVENT_MISS || type == MATCH_EVENT_KEEPERSAVE || type == MATCH_EVENT_REDCARD)
	{
		float recordStartTime = gpGlobals->curtime - GetLongestReplayDuration() + sv_replay_endpadding.GetFloat();

		for (int i = 0; i < m_Snapshots.Count(); i++)
		{
			if (m_Snapshots[i]->snaptime >= recordStartTime)
			{
				pMatchEvent->snapshots.AddToTail(m_Snapshots[i]);
				m_Snapshots[i]->replayCount += 1;
			}
		}

		pMatchEvent->snapshotEndTime = gpGlobals->curtime + sv_replay_endpadding.GetFloat();
	}
	else
		pMatchEvent->snapshotEndTime = 0;
	
	pMatchEvent->matchPeriod = SDKGameRules()->State_Get();
	pMatchEvent->matchEventType = type;
	pMatchEvent->second = SDKGameRules()->GetMatchDisplayTimeSeconds();
	pMatchEvent->team = team;
	pMatchEvent->atMinGoalPos = GetMatchBall()->GetPos().y < SDKGameRules()->m_vKickOff.GetY();
	pMatchEvent->pPlayer1Data = pPlayer1 ? pPlayer1->GetPlayerData() : NULL;
	pMatchEvent->pPlayer2Data = pPlayer2 ? pPlayer2->GetPlayerData() : NULL;
	pMatchEvent->pPlayer3Data = pPlayer3 ? pPlayer3->GetPlayerData() : NULL;

	m_MatchEvents.AddToTail(pMatchEvent);

	if (type == MATCH_EVENT_GOAL || type == MATCH_EVENT_OWNGOAL || type == MATCH_EVENT_YELLOWCARD || type == MATCH_EVENT_SECONDYELLOWCARD || type == MATCH_EVENT_REDCARD)
	{
		char matchEventPlayerNames[MAX_MATCH_EVENT_PLAYER_NAME_LENGTH] = {};
		if (pPlayer1 && !pPlayer2 && !pPlayer3)
			Q_strncpy(matchEventPlayerNames, UTIL_VarArgs("%s", pPlayer1->GetPlayerName()), MAX_MATCH_EVENT_PLAYER_NAME_LENGTH);
		else if (pPlayer1 && pPlayer2 && !pPlayer3)
			Q_strncpy(matchEventPlayerNames, UTIL_VarArgs("%.15s (%.15s)", pPlayer1->GetPlayerName(), pPlayer2->GetPlayerName()), MAX_MATCH_EVENT_PLAYER_NAME_LENGTH);
		else if (pPlayer1 && pPlayer2 && pPlayer3)
			Q_strncpy(matchEventPlayerNames, UTIL_VarArgs("%.10s (%.10s, %.10s)", pPlayer1->GetPlayerName(), pPlayer2->GetPlayerName(), pPlayer3->GetPlayerName()), MAX_MATCH_EVENT_PLAYER_NAME_LENGTH);

		GetGlobalTeam(pMatchEvent->team)->AddMatchEvent(pMatchEvent->matchPeriod, pMatchEvent->second, pMatchEvent->matchEventType, matchEventPlayerNames);
	}
}

float CReplayManager::GetLongestReplayDuration()
{
	float instantMax = max(max(sv_replay_instant_first_duration.GetFloat(), sv_replay_instant_second_duration.GetFloat()), sv_replay_instant_third_duration.GetFloat());
	float highlightMax = max(max(sv_replay_highlight_first_duration.GetFloat(), sv_replay_highlight_second_duration.GetFloat()), sv_replay_highlight_third_duration.GetFloat());

	return max(instantMax, highlightMax);
}