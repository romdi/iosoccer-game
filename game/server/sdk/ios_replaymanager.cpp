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
	SendPropInt(SENDINFO(m_nTeamNumber)),
	SendPropInt(SENDINFO(m_nTeamPosNum)),
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
	m_nTeamPosNum = 0;
	m_szPlayerName.GetForModify()[0] = 0;
}

void CReplayPlayer::Spawn( void )
{
	SetClassname("replay_player");
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

float GetReplayStartTime()
{
	return gpGlobals->curtime - max(max(sv_replay_duration1.GetFloat(), sv_replay_duration2.GetFloat()), sv_replay_duration3.GetFloat());
}

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

bool matchEventFitsMatchState(const MatchEvent *pMatchEvent, match_state_t matchState)
{
	if (matchState == MATCH_COOLDOWN
		|| matchState == MATCH_HALFTIME
		|| matchState == MATCH_EXTRATIME_INTERMISSION && pMatchEvent->matchState == MATCH_SECOND_HALF
		|| matchState == MATCH_EXTRATIME_HALFTIME && pMatchEvent->matchState == MATCH_EXTRATIME_FIRST_HALF
		|| matchState == MATCH_PENALTIES_INTERMISSION
		&& (mp_extratime.GetBool() && pMatchEvent->matchState == MATCH_EXTRATIME_SECOND_HALF
		|| !mp_extratime.GetBool() && pMatchEvent->matchState == MATCH_SECOND_HALF))
	{
		return true;
	}

	return false;
}

int CReplayManager::FindNextHighlightReplayIndex(int startIndex, match_state_t matchState)
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
	if (m_bDoReplay && gpGlobals->curtime >= m_flReplayActivationTime)
		RestoreSnapshot();
	else if (!m_bDoReplay && !SDKGameRules()->IsIntermissionState())
		TakeSnapshot();
}

void CReplayManager::StartReplay(int numberOfRuns, float startDelay, int index /*= -1*/, bool isHighlightReplay /*= false*/)
{
	if (!sv_replays.GetBool())
		return;

	m_nReplayIndex = FindNextHighlightReplayIndex(index, SDKGameRules()->State_Get());

	if (m_nReplayIndex == -1)
		return;

	m_bIsHighlightReplay = isHighlightReplay;
	m_bIsHighlightStart = true;
	m_bIsReplayStart = true;
	m_nMaxReplayRuns = numberOfRuns;
	m_flReplayActivationTime = gpGlobals->curtime + startDelay;
	m_nReplayRunIndex = 0;
	m_bDoReplay = true;
	CalcReplayDuration(m_flReplayActivationTime);
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

	if (SDKGameRules()->State_Get() == MATCH_COOLDOWN)
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent("match_state");
		if (pEvent)
		{
			pEvent->SetInt("state", MATCH_EVENT_MATCH_END);
			gameeventmanager->FireEvent(pEvent);
		}
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
	Snapshot *pSnap = new Snapshot;
	pSnap->snaptime = gpGlobals->curtime;
	pSnap->isInReplay = false;

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
			CAnimationLayer *animLayer = pPl->GetAnimOverlay(layerIndex);
			if(!animLayer)
				continue;

			CopyAnimationLayer(&pPlSnap->m_animLayers[layerIndex], animLayer);
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

		pPlSnap->m_pPlayerData = pPl->GetData();

		pSnap->pPlayerSnapshot[pPl->GetTeamNumber() - TEAM_A][pPl->GetTeamPosIndex()] = pPlSnap;
	}

	m_Snapshots.AddToTail(pSnap);
	
	float replayStartTime = GetReplayStartTime();

	while (m_Snapshots.Count() >= 2 && m_Snapshots[0]->snaptime < replayStartTime && m_Snapshots[1]->snaptime <= replayStartTime)
	{
		if (!m_Snapshots[0]->isInReplay)
			delete m_Snapshots[0];

		m_Snapshots.Remove(0);
	}
}

void CReplayManager::RestoreSnapshot()
{
	float timeSinceReplayStart = gpGlobals->curtime - m_flReplayStartTime;

	if (timeSinceReplayStart >= m_flRunDuration)
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
		CalcReplayDuration(gpGlobals->curtime);
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
					pEvent->SetInt("scoring_team", pMatchEvent->team);
					pEvent->SetString("scorer", pMatchEvent->pPlayerData1 ? pMatchEvent->pPlayerData1->m_szName : "");
					pEvent->SetString("first_assister", pMatchEvent->pPlayerData2 ? pMatchEvent->pPlayerData2->m_szName : "");
					pEvent->SetString("second_assister", pMatchEvent->pPlayerData3 ? pMatchEvent->pPlayerData3->m_szName : "");
					gameeventmanager->FireEvent(pEvent);
				}
			}
			else if (pMatchEvent->matchEventType == MATCH_EVENT_CHANCE)
			{
				IGameEvent *pEvent = gameeventmanager->CreateEvent("highlight_chance");
				if (pEvent)
				{
					pEvent->SetInt("second", pMatchEvent->second);
					pEvent->SetInt("finishing_team", pMatchEvent->team);
					pEvent->SetString("finisher", pMatchEvent->pPlayerData1 ? pMatchEvent->pPlayerData1->m_szName : "");
					pEvent->SetString("first_assister", pMatchEvent->pPlayerData2 ? pMatchEvent->pPlayerData2->m_szName : "");
					pEvent->SetString("second_assister", pMatchEvent->pPlayerData3 ? pMatchEvent->pPlayerData3->m_szName : "");
					gameeventmanager->FireEvent(pEvent);
				}
			}
			else if (pMatchEvent->matchEventType == MATCH_EVENT_REDCARD)
			{
				IGameEvent *pEvent = gameeventmanager->CreateEvent("highlight_redcard");
				if (pEvent)
				{
					pEvent->SetInt("second", pMatchEvent->second);
					pEvent->SetInt("fouling_team", pMatchEvent->team);
					pEvent->SetString("fouling_player", pMatchEvent->pPlayerData1 ? pMatchEvent->pPlayerData1->m_szName : "");
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
			pRealPl->m_vPreReplayPos = pRealPl->GetLocalOrigin();
			pRealPl->m_aPreReplayAngles = pRealPl->GetLocalAngles();
			pRealPl->AddEffects(EF_NODRAW);
			pRealPl->DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
			pRealPl->AddSolidFlags(FSOLID_NOT_SOLID);
			pRealPl->SetObserverMode(OBS_MODE_TVCAM);
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

			pPl->m_nTeamNumber = pPlSnap->m_nTeamNumber;
			pPl->m_nTeamPosNum = pPlSnap->m_nTeamPosNum;

			if (Q_strcmp(pPl->m_szPlayerName, pPlSnap->m_pPlayerData->m_szName))
				Q_strncpy(pPl->m_szPlayerName.GetForModify(), pPlSnap->m_pPlayerData->m_szName, MAX_PLAYER_NAME_LENGTH);

			pPl->m_nSkin = pPlSnap->m_nSkin;
			pPl->m_nBody = pPlSnap->m_nBody;

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

			if (frac > 0.0f && pNextPlSnap && pPlSnap->m_masterSequence == pNextPlSnap->m_masterSequence)
			{
				// If the master state changes, all layers will be invalid too, so don't interp (ya know, interp barely ever happens anyway)
				interpolationAllowed = true;
			}
			else
				interpolationAllowed = false;

			// First do the master settings
			if (interpolationAllowed)
			{
				pPl->SetSequence( Lerp( frac, pPlSnap->m_masterSequence, pNextPlSnap->m_masterSequence ) );
				pPl->SetCycle( Lerp( frac, pPlSnap->m_masterCycle, pNextPlSnap->m_masterCycle ) );

				if( pPlSnap->m_masterCycle > pNextPlSnap->m_masterCycle )
				{
					// the older record is higher in frame than the newer, it must have wrapped around from 1 back to 0
					// add one to the newer so it is lerping from .9 to 1.1 instead of .9 to .1, for example.
					float newCycle = Lerp( frac, pPlSnap->m_masterCycle, pNextPlSnap->m_masterCycle + 1 );
					pPl->SetCycle(newCycle < 1 ? newCycle : newCycle - 1 );// and make sure .9 to 1.2 does not end up 1.05
				}
				else
				{
					pPl->SetCycle( Lerp( frac, pPlSnap->m_masterCycle, pNextPlSnap->m_masterCycle ) );
				}

				pPl->SetPoseParameter(pPl->GetModelPtr(), 4, Lerp(frac, pPlSnap->m_flMoveX, pNextPlSnap->m_flMoveX));
				pPl->SetPoseParameter(pPl->GetModelPtr(), 3, Lerp(frac, pPlSnap->m_flMoveY, pNextPlSnap->m_flMoveY));
			}
			else
			{
				pPl->SetSequence(pPlSnap->m_masterSequence);
				pPl->SetCycle(pPlSnap->m_masterCycle);
				pPl->SetPoseParameter(pPl->GetModelPtr(), 4, pPlSnap->m_flMoveX);
				pPl->SetPoseParameter(pPl->GetModelPtr(), 3, pPlSnap->m_flMoveY);
			}

			// Now do all the layers
			for (int layerIndex = 0; layerIndex < NUM_LAYERS_WANTED; layerIndex++)
			{
				CAnimationLayer *animLayer = pPl->GetAnimOverlay(layerIndex);
				if(!animLayer)
					continue;

				CAnimationLayer &animLayerSnap = pPlSnap->m_animLayers[layerIndex];

				animLayer->m_nOrder = animLayerSnap.m_nOrder;
				animLayer->m_nSequence = animLayerSnap.m_nSequence;
				animLayer->m_fFlags = animLayerSnap.m_fFlags;
				animLayer->m_bLooping = animLayerSnap.m_bLooping;
				animLayer->m_bSequenceFinished = animLayerSnap.m_bSequenceFinished;
				animLayer->m_flPlaybackRate = animLayerSnap.m_flPlaybackRate;
				animLayer->m_nPriority = animLayerSnap.m_nPriority;
				animLayer->m_nActivity = animLayerSnap.m_nActivity;
				animLayer->m_flLastAccess = animLayerSnap.m_flLastAccess;
				animLayer->m_flLastEventCheck = animLayerSnap.m_flLastEventCheck;

				if (interpolationAllowed)
				{
					CAnimationLayer &nextAnimLayerSnap = pNextPlSnap->m_animLayers[layerIndex];

					if(animLayerSnap.m_nOrder == nextAnimLayerSnap.m_nOrder && animLayerSnap.m_nSequence == nextAnimLayerSnap.m_nSequence)
					{
						// We can't interpolate across a sequence or order change
						if( animLayerSnap.m_flCycle > nextAnimLayerSnap.m_flCycle )
						{
							// the older record is higher in frame than the newer, it must have wrapped around from 1 back to 0
							// add one to the newer so it is lerping from .9 to 1.1 instead of .9 to .1, for example.
							float newCycle = Lerp( frac, animLayerSnap.m_flCycle.Get(), nextAnimLayerSnap.m_flCycle + 1 );
							animLayer->m_flCycle = newCycle < 1 ? newCycle : newCycle - 1;// and make sure .9 to 1.2 does not end up 1.05
						}
						else
						{
							animLayer->m_flCycle = Lerp( frac, animLayerSnap.m_flCycle.Get(), nextAnimLayerSnap.m_flCycle.Get()  );
						}

						animLayer->m_flWeight = Lerp( frac, animLayerSnap.m_flWeight.Get(), nextAnimLayerSnap.m_flWeight.Get()  );

						animLayer->m_flLayerAnimtime = Lerp(frac, animLayerSnap.m_flLayerAnimtime, nextAnimLayerSnap.m_flLayerAnimtime);
						animLayer->m_flBlendIn = Lerp(frac, animLayerSnap.m_flBlendIn, nextAnimLayerSnap.m_flBlendIn);
						animLayer->m_flBlendOut = Lerp(frac, animLayerSnap.m_flBlendOut, nextAnimLayerSnap.m_flBlendOut);
						animLayer->m_flPrevCycle = Lerp(frac, animLayerSnap.m_flPrevCycle.Get(), nextAnimLayerSnap.m_flPrevCycle.Get());
						animLayer->m_flKillDelay = Lerp(frac, animLayerSnap.m_flKillDelay, nextAnimLayerSnap.m_flKillDelay);
						animLayer->m_flKillRate = Lerp(frac, animLayerSnap.m_flKillRate, nextAnimLayerSnap.m_flKillRate);
						animLayer->m_flLayerFadeOuttime = Lerp(frac, animLayerSnap.m_flLayerFadeOuttime, nextAnimLayerSnap.m_flLayerFadeOuttime);
					}
				}
				else
				{
					//Either no interp, or interp failed.  Just use record.
					animLayer->m_flCycle = animLayerSnap.m_flCycle;
					animLayer->m_flWeight = animLayerSnap.m_flWeight;

					animLayer->m_flLayerAnimtime = animLayerSnap.m_flLayerAnimtime;
					animLayer->m_flBlendIn = animLayerSnap.m_flBlendIn;
					animLayer->m_flBlendOut = animLayerSnap.m_flBlendOut;
					animLayer->m_flPrevCycle = animLayerSnap.m_flPrevCycle;
					animLayer->m_flKillDelay = animLayerSnap.m_flKillDelay;
					animLayer->m_flKillRate = animLayerSnap.m_flKillRate;
					animLayer->m_flLayerFadeOuttime = animLayerSnap.m_flLayerFadeOuttime;
				}
			}
		}
	}
}

void CReplayManager::StartHighlights()
{
	if (!sv_replays.GetBool() || !sv_highlights.GetBool())
		return;

	StartReplay(2, 5, 0, true);
}

void CReplayManager::StopHighlights()
{
	if (!sv_replays.GetBool() || !sv_highlights.GetBool() || !m_bDoReplay)
		return;

	StopReplay();
}

void CReplayManager::CalcReplayDuration(float startTime)
{
	if (m_nReplayRunIndex == 0)
		m_flRunDuration = sv_replay_duration1.GetFloat();
	else if (m_nReplayRunIndex == 1)
		m_flRunDuration = sv_replay_duration2.GetFloat();
	else if (m_nReplayRunIndex == 2)
		m_flRunDuration = sv_replay_duration3.GetFloat();

	m_flReplayStartTime = startTime;
	m_flReplayStartTimeOffset = max(max(sv_replay_duration1.GetFloat(), sv_replay_duration2.GetFloat()), sv_replay_duration3.GetFloat()) - m_flRunDuration;
}

void CReplayManager::AddMatchEvent(match_event_t type, int team, CSDKPlayer *pPlayer1, CSDKPlayer *pPlayer2/* = NULL*/, CSDKPlayer *pPlayer3/* = NULL*/)
{
	MatchEvent *pMatchEvent = new MatchEvent;

	if (type == MATCH_EVENT_GOAL || type == MATCH_EVENT_CHANCE || type == MATCH_EVENT_REDCARD)
	{
		float replayStartTime = GetReplayStartTime();

		for (int i = 0; i < m_Snapshots.Count(); i++)
		{
			if (m_Snapshots[i]->snaptime >= replayStartTime)
			{
				pMatchEvent->snapshots.AddToTail(m_Snapshots[i]);
				m_Snapshots[i]->isInReplay = true;
			}
		}
	}

	pMatchEvent->matchState = SDKGameRules()->State_Get();
	pMatchEvent->matchEventType = type;
	pMatchEvent->second = SDKGameRules()->GetMatchDisplayTimeSeconds();
	pMatchEvent->team = team;
	pMatchEvent->atMinGoalPos = GetBall()->GetPos().y < SDKGameRules()->m_vKickOff.GetY();
	pMatchEvent->pPlayerData1 = pPlayer1 ? pPlayer1->GetData() : NULL;
	pMatchEvent->pPlayerData2 = pPlayer2 ? pPlayer2->GetData() : NULL;
	pMatchEvent->pPlayerData3 = pPlayer3 ? pPlayer3->GetData() : NULL;

	m_MatchEvents.AddToTail(pMatchEvent);

	if (type == MATCH_EVENT_GOAL)
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