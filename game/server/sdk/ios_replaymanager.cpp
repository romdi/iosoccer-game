#include "cbase.h"
#include "ios_replaymanager.h"
#include "ball.h"
#include "sdk_gamerules.h"
#include "convar.h"
#include "sdk_playeranimstate.h"
#include "team.h"

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
	SendPropInt(SENDINFO(m_nShirtNumber), 7, SPROP_UNSIGNED),
	SendPropString(SENDINFO(m_szPlayerName)),
END_SEND_TABLE()

void CReplayPlayer::Precache()
{
	SetModel(SDK_PLAYER_MODEL);
	BaseClass::Precache();
}

CReplayPlayer::CReplayPlayer()
{
	m_nTeamNumber = 0;
	m_nShirtNumber = 0;
	m_szPlayerName.GetForModify()[0] = 0;
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

float GetReplayStartTime()
{
	return gpGlobals->curtime - max(max(sv_replay_duration1.GetFloat(), sv_replay_duration2.GetFloat()), sv_replay_duration3.GetFloat());
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
	m_flRunDuration = 0;
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

	if (!sv_replays.GetBool())
		return;

	if (!GetBall())
		return;

	CheckReplay();
}

bool matchEventFitsMatchState(const MatchEvent *pMatchEvent, match_period_t matchState)
{
	if (matchState == pMatchEvent->matchState
		|| matchState == MATCH_PERIOD_COOLDOWN
		|| matchState == MATCH_PERIOD_HALFTIME
		|| matchState == MATCH_PERIOD_EXTRATIME_INTERMISSION && pMatchEvent->matchState == MATCH_PERIOD_SECOND_HALF
		|| matchState == MATCH_PERIOD_EXTRATIME_HALFTIME && pMatchEvent->matchState == MATCH_PERIOD_EXTRATIME_FIRST_HALF
		|| matchState == MATCH_PERIOD_PENALTIES_INTERMISSION
		&& (mp_extratime.GetBool() && pMatchEvent->matchState == MATCH_PERIOD_EXTRATIME_SECOND_HALF
		|| !mp_extratime.GetBool() && pMatchEvent->matchState == MATCH_PERIOD_SECOND_HALF))
	{
		return true;
	}

	return false;
}

int CReplayManager::FindNextHighlightReplayIndex(int startIndex, match_period_t matchState)
{
	if (startIndex == -1)
	{
		// Iterate backwards
		for (int i = m_MatchEvents.Count() - 1; i >= 0; i--)
		{
			if (m_MatchEvents[i]->snapshots.Count() > 0 && matchEventFitsMatchState(m_MatchEvents[i], matchState))
				return i;
		}
	}
	else if (startIndex >= 0 && startIndex < m_MatchEvents.Count())
	{
		// Iterate forwards
		for (int i = startIndex; i < m_MatchEvents.Count(); i++)
		{
			if (m_MatchEvents[i]->snapshots.Count() > 0 && matchEventFitsMatchState(m_MatchEvents[i], matchState))
				return i;
		}
	}

	return -1;
}

void CReplayManager::CheckReplay()
{
	if (m_bReplayIsPending && gpGlobals->curtime >= m_flReplayActivationTime || m_bIsReplaying)
		RestoreSnapshot();
	else if (!m_bIsReplaying && !SDKGameRules()->IsIntermissionState()
		&& (!m_bReplayIsPending || m_MatchEvents.Count() > 0 && m_MatchEvents.Tail()->snapshotEndTime >= gpGlobals->curtime))
		TakeSnapshot();
}

extern ConVar sv_ball_goalcelebduration;

void CReplayManager::StartReplay(bool isHighlightReplay)
{
	if (!sv_replays.GetBool())
		return;

	m_bIsHighlightReplay = isHighlightReplay;

	m_nReplayIndex = FindNextHighlightReplayIndex((m_bIsHighlightReplay ? 0 : -1), SDKGameRules()->State_Get());

	if (m_nReplayIndex == -1)
		return;

	MatchEvent *pMatchEvent = m_MatchEvents[m_nReplayIndex];

	m_bIsHighlightStart = true;
	m_bIsReplayStart = true;
	m_flReplayActivationTime = gpGlobals->curtime + (m_bIsHighlightReplay ? 3.0f : sv_ball_goalcelebduration.GetFloat());
	m_nReplayRunIndex = 0;
	m_bReplayIsPending = true;
	m_bIsReplaying = false;
	CalcMaxReplayRunsAndDuration(pMatchEvent, m_flReplayActivationTime);
}

void CReplayManager::StopReplay()
{
	if (m_bReplayIsPending)
	{
		m_bReplayIsPending = false;
		return;
	}

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
		pRealPl->SetLocalOrigin(pRealPl->GetSpawnPos(true));
		QAngle ang;
		VectorAngles(Vector(0, pRealPl->GetTeam()->m_nForward, 0), ang);
		pRealPl->SetLocalAngles(ang);
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
		pPlSnap->shirtNumber = pPl->GetShirtNumber();
		pPlSnap->skin = pPl->m_nSkin;
		pPlSnap->body = pPl->m_nBody;

		pPlSnap->pPlayerData = pPl->GetPlayerData();

		pSnap->pPlayerSnapshot[pPl->GetTeamNumber() - TEAM_A][pPl->GetTeamPosIndex()] = pPlSnap;
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
	
	float replayStartTime = GetReplayStartTime();

	// Remove old snapshots from the list. Only free a snapshot's memory if it isn't part of any highlight.
	while (m_Snapshots.Count() >= 2 && m_Snapshots[0]->snaptime < replayStartTime && m_Snapshots[1]->snaptime <= replayStartTime)
	{
		if (m_Snapshots[0]->replayCount == 0)
			delete m_Snapshots[0];

		m_Snapshots.Remove(0);
	}
}

void CReplayManager::RestoreSnapshot()
{
	float timeSinceReplayStart = gpGlobals->curtime - m_flReplayStartTime;

	if (timeSinceReplayStart > m_flRunDuration)
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
				return;
			}

			m_nReplayRunIndex = 0;
			m_bIsReplayStart = true;
			m_bIsHighlightStart = true;
		}
		else
		{
			StopReplay();
			return;
		}
	}

	MatchEvent *pMatchEvent = m_MatchEvents[m_nReplayIndex];

	if (m_bIsReplayStart)
	{
		m_bIsReplayStart = false;
		m_bAtMinGoalPos = pMatchEvent->atMinGoalPos;
		m_bIsReplaying = true;
		m_bReplayIsPending = false;
		CalcMaxReplayRunsAndDuration(pMatchEvent, gpGlobals->curtime);
		timeSinceReplayStart = 0;

		if (m_bIsHighlightReplay && m_bIsHighlightStart)
		{
			m_bIsHighlightStart = false;

			if (pMatchEvent->matchEventType == MATCH_EVENT_GOAL)
			{
				IGameEvent *pEvent = gameeventmanager->CreateEvent("highlight_goal");
				if (pEvent)
				{
					pEvent->SetInt("second", pMatchEvent->second);
					pEvent->SetInt("match_period", pMatchEvent->matchState);
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
					pEvent->SetInt("match_period", pMatchEvent->matchState);
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
					pEvent->SetInt("match_period", pMatchEvent->matchState);
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
					pEvent->SetInt("match_period", pMatchEvent->matchState);
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
					pEvent->SetInt("match_period", pMatchEvent->matchState);
					pEvent->SetInt("fouling_team", pMatchEvent->team);
					pEvent->SetString("fouling_player", pMatchEvent->pPlayer1Data ? pMatchEvent->pPlayer1Data->m_szName : "");
					gameeventmanager->FireEvent(pEvent);
				}
			}
		}
	}

	CBall *pRealBall = GetBall();
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

	Snapshot *pNextSnap = NULL;
	Snapshot *pSnap = NULL;
	float timeSinceFirstSnap = 0;

	// Walk context looking for any invalidating event
	for (int i = pMatchEvent->snapshots.Count() - 1; i >= 0; i--)
	{
		// remember last record
		pNextSnap = pSnap;

		// get next record
		pSnap = pMatchEvent->snapshots[i];

		timeSinceFirstSnap = pSnap->snaptime - pMatchEvent->snapshots[0]->snaptime;

		// did we find a context smaller than target time ?
		if (timeSinceFirstSnap - m_flReplayStartTimeOffset <= timeSinceReplayStart)
			break; // hurra, stop
	}

	if (!pSnap)
		return;

	float nextTimeSinceFirstSnap;

	if (pNextSnap)
		nextTimeSinceFirstSnap = pNextSnap->snaptime - pMatchEvent->snapshots[0]->snaptime;
	else
		nextTimeSinceFirstSnap = 0;

	BallSnapshot *pBallSnap = pSnap->pBallSnapshot;

	if (pBallSnap)
	{
		if (!m_pBall)
		{
			m_pBall = (CReplayBall *)CreateEntityByName("replayball");
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

	float frac;

	if (pNextSnap &&
		(timeSinceFirstSnap < timeSinceReplayStart) &&
		(timeSinceFirstSnap < nextTimeSinceFirstSnap) )
	{
		// we didn't find the exact time but have a valid previous record
		// so interpolate between these two records;

		// calc fraction between both records
		frac = ( timeSinceReplayStart - (timeSinceFirstSnap - m_flReplayStartTimeOffset) ) / 
			( nextTimeSinceFirstSnap - timeSinceFirstSnap );

		Assert( frac > 0 && frac < 1 ); // should never extrapolate
	}
	else
	{
		// we found the exact record or no other record to interpolate with
		// just copy these values since they are the best we have
		frac = 0.0f;
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
				m_pPlayers[i][j]->Spawn();
				m_pPlayers[i][j]->SetNumAnimOverlays(NUM_LAYERS_WANTED);
			}

			CReplayPlayer *pPl = m_pPlayers[i][j];

			pPl->m_nTeamNumber = pPlSnap->teamNumber;
			pPl->m_nShirtNumber = pPlSnap->shirtNumber;

			if (Q_strcmp(pPl->m_szPlayerName, pPlSnap->pPlayerData->m_szName))
				Q_strncpy(pPl->m_szPlayerName.GetForModify(), pPlSnap->pPlayerData->m_szName, MAX_PLAYER_NAME_LENGTH);

			pPl->m_nSkin = pPlSnap->skin;
			pPl->m_nBody = pPlSnap->body;

			PlayerSnapshot *pNextPlSnap = NULL;

			if (pNextSnap)
				pNextPlSnap = pNextSnap->pPlayerSnapshot[i][j];

			if (frac > 0.0f && pNextPlSnap)
			{
				pPl->SetLocalOrigin(Lerp( frac, pPlSnap->pos, pNextPlSnap->pos  ));
				//pPl->SetLocalVelocity(Lerp( frac, pPlSnap->vel, pNextPlSnap->vel  ));
				pPl->SetLocalAngles(Lerp( frac, pPlSnap->ang, pNextPlSnap->ang ));
			}
			else
			{
				pPl->SetLocalOrigin(pPlSnap->pos);
				//pPl->SetLocalVelocity(pPlSnap->vel);
				pPl->SetLocalAngles(pPlSnap->ang);
			}

			bool interpolationAllowed;

			if (frac > 0.0f && pNextPlSnap && pPlSnap->masterSequence == pNextPlSnap->masterSequence)
			{
				// If the master state changes, all layers will be invalid too, so don't interp (ya know, interp barely ever happens anyway)
				interpolationAllowed = true;
			}
			else
				interpolationAllowed = false;

			// First do the master settings
			if (interpolationAllowed)
			{
				pPl->SetSequence( Lerp( frac, pPlSnap->masterSequence, pNextPlSnap->masterSequence ) );
				pPl->SetCycle( Lerp( frac, pPlSnap->masterCycle, pNextPlSnap->masterCycle ) );

				if( pPlSnap->masterCycle > pNextPlSnap->masterCycle )
				{
					// the older record is higher in frame than the newer, it must have wrapped around from 1 back to 0
					// add one to the newer so it is lerping from .9 to 1.1 instead of .9 to .1, for example.
					float newCycle = Lerp( frac, pPlSnap->masterCycle, pNextPlSnap->masterCycle + 1 );
					pPl->SetCycle(newCycle < 1 ? newCycle : newCycle - 1 );// and make sure .9 to 1.2 does not end up 1.05
				}
				else
				{
					pPl->SetCycle( Lerp( frac, pPlSnap->masterCycle, pNextPlSnap->masterCycle ) );
				}

				pPl->SetPoseParameter(pPl->GetModelPtr(), 4, Lerp(frac, pPlSnap->moveX, pNextPlSnap->moveX));
				pPl->SetPoseParameter(pPl->GetModelPtr(), 3, Lerp(frac, pPlSnap->moveY, pNextPlSnap->moveY));
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
				if(!pTargetLayer)
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

					if(pSourceLayer->order == pNextSourceLayer->order && pSourceLayer->sequence == pNextSourceLayer->sequence)
					{
						// We can't interpolate across a sequence or order change
						if( pSourceLayer->cycle > pNextSourceLayer->cycle )
						{
							// the older record is higher in frame than the newer, it must have wrapped around from 1 back to 0
							// add one to the newer so it is lerping from .9 to 1.1 instead of .9 to .1, for example.
							float newCycle = Lerp( frac, pSourceLayer->cycle, pNextSourceLayer->cycle + 1 );
							pTargetLayer->m_flCycle = newCycle < 1 ? newCycle : newCycle - 1;// and make sure .9 to 1.2 does not end up 1.05
						}
						else
						{
							pTargetLayer->m_flCycle = Lerp(frac, pSourceLayer->cycle, pNextSourceLayer->cycle);
						}

						pTargetLayer->m_flWeight = Lerp(frac, pSourceLayer->weight, pNextSourceLayer->weight);
						pTargetLayer->m_flLayerAnimtime = Lerp(frac, pSourceLayer->layerAnimtime, pNextSourceLayer->layerAnimtime);
						pTargetLayer->m_flBlendIn = Lerp(frac, pSourceLayer->blendIn, pNextSourceLayer->blendIn);
						pTargetLayer->m_flBlendOut = Lerp(frac, pSourceLayer->blendOut, pNextSourceLayer->blendOut);
						pTargetLayer->m_flPrevCycle = Lerp(frac, pSourceLayer->prevCycle, pNextSourceLayer->prevCycle);
						pTargetLayer->m_flKillDelay = Lerp(frac, pSourceLayer->killDelay, pNextSourceLayer->killDelay);
						pTargetLayer->m_flKillRate = Lerp(frac, pSourceLayer->killRate, pNextSourceLayer->killRate);
						pTargetLayer->m_flLayerFadeOuttime = Lerp(frac, pSourceLayer->layerFadeOuttime, pNextSourceLayer->layerFadeOuttime);
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
	if (!sv_replays.GetBool() || !sv_highlights.GetBool())
		return;

	StartReplay(true);
}

void CReplayManager::StopHighlights()
{
	StopReplay();
}

void CReplayManager::CalcMaxReplayRunsAndDuration(const MatchEvent *pMatchEvent, float startTime)
{
	if (pMatchEvent->matchEventType == MATCH_EVENT_GOAL || pMatchEvent->matchEventType == MATCH_EVENT_OWNGOAL)
	{
		m_nMaxReplayRuns = sv_replay_count.GetInt();

		if (m_nReplayRunIndex == 0)
			m_flRunDuration = sv_replay_duration1.GetFloat();
		else if (m_nReplayRunIndex == 1)
			m_flRunDuration = sv_replay_duration2.GetFloat();
		else if (m_nReplayRunIndex == 2)
			m_flRunDuration = sv_replay_duration3.GetFloat();
	}
	else
	{
		m_nMaxReplayRuns = 1;
		m_flRunDuration = 4.0f;
	}

	m_flReplayStartTime = startTime;
	// If the new replay duration is shorter than the recorded time span, set the offset so it starts playing later and finishes with the last snapshot
	m_flReplayStartTimeOffset = max(max(sv_replay_duration1.GetFloat(), sv_replay_duration2.GetFloat()), sv_replay_duration3.GetFloat()) - m_flRunDuration;
}

void CReplayManager::AddMatchEvent(match_event_t type, int team, CSDKPlayer *pPlayer1, CSDKPlayer *pPlayer2/* = NULL*/, CSDKPlayer *pPlayer3/* = NULL*/)
{
	MatchEvent *pMatchEvent = new MatchEvent;

	if (type == MATCH_EVENT_GOAL || type == MATCH_EVENT_OWNGOAL || type == MATCH_EVENT_MISS || type == MATCH_EVENT_KEEPERSAVE || type == MATCH_EVENT_REDCARD)
	{
		float replayStartTime = GetReplayStartTime() + 1.0f;

		for (int i = 0; i < m_Snapshots.Count(); i++)
		{
			if (m_Snapshots[i]->snaptime >= replayStartTime)
			{
				pMatchEvent->snapshots.AddToTail(m_Snapshots[i]);
				m_Snapshots[i]->replayCount += 1;
			}
		}

		pMatchEvent->snapshotEndTime = gpGlobals->curtime + 1.0f;
	}
	else
		pMatchEvent->snapshotEndTime = 0;
	
	pMatchEvent->matchState = SDKGameRules()->State_Get();
	pMatchEvent->matchEventType = type;
	pMatchEvent->second = SDKGameRules()->GetMatchDisplayTimeSeconds();
	pMatchEvent->team = team;
	pMatchEvent->atMinGoalPos = GetBall()->GetPos().y < SDKGameRules()->m_vKickOff.GetY();
	pMatchEvent->pPlayer1Data = pPlayer1 ? pPlayer1->GetPlayerData() : NULL;
	pMatchEvent->pPlayer2Data = pPlayer2 ? pPlayer2->GetPlayerData() : NULL;
	pMatchEvent->pPlayer3Data = pPlayer3 ? pPlayer3->GetPlayerData() : NULL;

	m_MatchEvents.AddToTail(pMatchEvent);

	if (type == MATCH_EVENT_GOAL || type == MATCH_EVENT_OWNGOAL)
	{
		char matchEventPlayerNames[MAX_MATCH_EVENT_PLAYER_NAME_LENGTH] = {};
		if (pPlayer1 && !pPlayer2 && !pPlayer3)
			Q_strncpy(matchEventPlayerNames, UTIL_VarArgs("%s", pPlayer1->GetPlayerName()), MAX_MATCH_EVENT_PLAYER_NAME_LENGTH);
		else if (pPlayer1 && pPlayer2 && !pPlayer3)
			Q_strncpy(matchEventPlayerNames, UTIL_VarArgs("%.13s (%.13s)", pPlayer1->GetPlayerName(), pPlayer2->GetPlayerName()), MAX_MATCH_EVENT_PLAYER_NAME_LENGTH);
		else if (pPlayer1 && pPlayer2 && pPlayer3)
			Q_strncpy(matchEventPlayerNames, UTIL_VarArgs("%.8s (%.8s, %.8s)", pPlayer1->GetPlayerName(), pPlayer2->GetPlayerName(), pPlayer3->GetPlayerName()), MAX_MATCH_EVENT_PLAYER_NAME_LENGTH);

		GetGlobalTeam(pMatchEvent->team)->AddMatchEvent(pMatchEvent->matchState, pMatchEvent->second, pMatchEvent->matchEventType, matchEventPlayerNames);
	}
}