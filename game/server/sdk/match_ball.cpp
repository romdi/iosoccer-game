#include "cbase.h"
#include "match_ball.h"
#include "team.h"
#include "ios_replaymanager.h"

LINK_ENTITY_TO_CLASS( match_ball, CMatchBall );

BEGIN_DATADESC(	CMatchBall )
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CMatchBall, DT_MatchBall )
	SendPropEHandle(SENDINFO(m_pLastActivePlayer)),
	SendPropInt(SENDINFO(m_nLastActiveTeam)),
END_SEND_TABLE()

CMatchBall *CreateMatchBall(const Vector &pos)
{
	CMatchBall *pBall = static_cast<CMatchBall *>(CreateEntityByName("match_ball"));
	pBall->SetAbsOrigin(pos);
	pBall->Spawn();
	pBall->SetPos(pos);

	return pBall;
}

CMatchBall *g_pMatchBall = NULL;

CMatchBall *GetMatchBall()
{
	return g_pMatchBall;
}

CMatchBall::CMatchBall()
{
	g_pMatchBall = NULL;
	m_pLastActivePlayer = NULL;
	m_nLastActiveTeam = TEAM_UNASSIGNED;
}

CMatchBall::~CMatchBall()
{

}

void CMatchBall::Spawn()
{
	//TODO: Move the ball skin parsing to a better spot
	CBallInfo::ParseBallSkins();
	
	g_pMatchBall = this;

	CBall::Spawn();
}

void CMatchBall::Reset()
{
	m_pLastActivePlayer = NULL;

	CBall::Reset();
}

void CMatchBall::State_Transition(ball_state_t newState, float delay /*= 0.0f*/, bool cancelQueuedState /*= false*/, bool isShortMessageDelay /*= false*/)
{
	if (delay == 0)
	{
		State_Leave(newState);
		State_Enter(newState, cancelQueuedState);
	}
	else
	{
		m_eNextState = newState;
		m_flStateActivationDelay = delay;
		m_flStateLeaveTime = gpGlobals->curtime + m_flStateActivationDelay + (isShortMessageDelay ? sv_ball_statetransition_messagedelay_short : sv_ball_statetransition_messagedelay_normal).GetFloat();
		m_bHasQueuedState = true;
	}
}

void CMatchBall::State_Enter(ball_state_t newState, bool cancelQueuedState)
{
	if (cancelQueuedState)
	{
		m_eNextState = BALL_STATE_NONE;
		m_bHasQueuedState = false;
	}

	m_eBallState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	m_flStateEnterTime = gpGlobals->curtime;
	m_flStateTimelimit = -1;
	m_bNextStateMessageSent = false;

	m_pPl = NULL;
	m_pOtherPl = NULL;
	m_pSetpieceTaker = NULL;

	IGameEvent *pEvent = gameeventmanager->CreateEvent("ball_state");
	if (pEvent)
	{
		pEvent->SetInt("state", State_Get());
		gameeventmanager->FireEvent(pEvent);
	}

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		(this->*m_pCurStateInfo->pfnEnterState)();
	}

	//State_Think();
}

void CMatchBall::State_Leave(ball_state_t newState)
{
	SDKGameRules()->DisableShield();

	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)(newState);
	}
}

void CMatchBall::State_Think()
{
	m_pPhys->GetPosition(&m_vPos, &m_aAng);
	m_pPhys->GetVelocity(&m_vVel, &m_vRot);

	if (m_eNextState != BALL_STATE_NONE)
	{
		if (!m_bNextStateMessageSent && gpGlobals->curtime >= m_flStateLeaveTime - m_flStateActivationDelay && m_eNextState != BALL_STATE_KICKOFF)
		{
			SendNotifications();
			m_bNextStateMessageSent = true;
		}
		else if (gpGlobals->curtime >= m_flStateLeaveTime)
		{
			State_Leave(m_eNextState);
			State_Enter(m_eNextState, true);
		}
	}

	if (State_Get() == BALL_STATE_THROWIN || State_Get() == BALL_STATE_CORNER || State_Get() == BALL_STATE_GOALKICK)
	{
		if (SDKGameRules()->CheckTimeout())
			m_flStateTimelimit = -1;
	}

	if (m_pCurStateInfo
		&& State_Get() != BALL_STATE_NORMAL
		&& m_eNextState == BALL_STATE_NONE
		&& CSDKPlayer::IsOnField(m_pPl)
		&& m_flStateTimelimit != -1
		&& gpGlobals->curtime >= m_flStateTimelimit) // Player is afk or timed out
	{
		m_pPl->SetDesiredTeam(TEAM_SPECTATOR, m_pPl->GetTeamNumber(), 0, true, false, false);
	}

	if (m_pCurStateInfo && m_pCurStateInfo->pfnThink)
	{	
		(this->*m_pCurStateInfo->pfnThink)();
	}

	if (m_pPl) // Set info for the client
	{
		m_pLastActivePlayer = m_pPl;
		m_nLastActiveTeam = m_pPl->GetTeamNumber();
	}
}

void CMatchBall::State_THROWIN_Enter()
{
	EnablePlayerCollisions(false);
	SetPos(Vector(m_vTriggerTouchPos.x + 0 * Sign(SDKGameRules()->m_vKickOff.GetX() - m_vTriggerTouchPos.x), m_vTriggerTouchPos.y, SDKGameRules()->m_vKickOff.GetZ()));
}

void CMatchBall::State_THROWIN_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl, LastOppTeam(false)))
	{
		m_pPl = NULL;

		if (CSDKPlayer::IsOnField(m_pSetpieceTaker, LastOppTeam(false)))
			m_pPl = m_pSetpieceTaker;

		if (!m_pPl)
			m_pPl = FindNearestPlayer(LastOppTeam(false));
		if (!m_pPl)
			return State_Transition(BALL_STATE_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_THROWIN, m_pPl->GetTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vTriggerTouchPos.x, m_vTriggerTouchPos.y, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		m_bShotsBlocked = true;
	}

	if (!PlayersAtTargetPos())
		return;

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		SetPos(Vector(m_vTriggerTouchPos.x, m_vTriggerTouchPos.y, SDKGameRules()->m_vKickOff.GetZ() + m_pPl->GetPlayerMaxs().z + 2));
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_THROWIN);
		m_pPl->SetShotButtonsReleased(false);
		m_bShotsBlocked = false;
	}

	UpdateCarrier();

	//Vector handPos;
	//QAngle handAng;
	//m_pPl->GetAttachment("ballrighthand", handPos, handAng);
	//handPos.z -= BALL_PHYS_RADIUS;
	//SetPos(handPos, false);
	//SetAng(handAng);

	if (m_pPl->ShotButtonsReleased() && (m_pPl->IsPowershooting() || m_pPl->IsChargedshooting()))
	{
		QAngle ang = m_aPlAng;

		ang[PITCH] = min(sv_ball_throwin_minangle.GetFloat(), m_aPlAng[PITCH]);

		Vector dir;
		AngleVectors(ang, &dir);
		float strength;

		if (m_pPl->IsPowershooting())
			strength = GetPowershotStrength(GetPitchCoeff(false), sv_ball_powerthrow_strength.GetInt());
		else
			strength = GetChargedshotStrength(GetPitchCoeff(false), sv_ball_chargedthrow_minstrength.GetInt(), sv_ball_chargedthrow_maxstrength.GetInt());

		Vector vel = dir * max(strength, sv_ball_throwin_minstrength.GetInt());

		m_pPl->AddThrowIn();
		RemoveAllTouches();
		SetVel(vel, 0, 0, BODY_PART_HANDS, false, false, false);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_THROWIN_Leave(ball_state_t newState)
{
	if (CSDKPlayer::IsOnField(m_pPl))
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_THROW);

	EnablePlayerCollisions(true);
	m_bShotsBlocked = false;
}

void CMatchBall::State_KICKOFF_Enter()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->m_Shared.SetStamina(100);
	}

	SetPos(SDKGameRules()->m_vKickOff);
}

void CMatchBall::State_KICKOFF_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl, SDKGameRules()->GetKickOffTeam()))
	{
		m_pPl = NULL;

		m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_ATTACKER | FL_POS_LEFT);
		if (!m_pPl)
			m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_ATTACKER | FL_POS_CENTER);
		if (!m_pPl)
			m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_ATTACKER | FL_POS_RIGHT);

		if (!m_pPl)
			m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_MIDFIELDER | FL_POS_LEFT);
		if (!m_pPl)
			m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_MIDFIELDER | FL_POS_CENTER);
		if (!m_pPl)
			m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_MIDFIELDER | FL_POS_RIGHT);

		if (!m_pPl)
			m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_DEFENDER | FL_POS_LEFT);
		if (!m_pPl)
			m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_DEFENDER | FL_POS_CENTER);
		if (!m_pPl)
			m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_DEFENDER | FL_POS_RIGHT);

		if (!m_pPl)
		{
			SDKGameRules()->EnableShield(SHIELD_KICKOFF, GetGlobalTeam(TEAM_A)->GetTeamNumber(), SDKGameRules()->m_vKickOff);
			if (!PlayersAtTargetPos())
				return;

			return State_Transition(BALL_STATE_NORMAL);
		}

		SDKGameRules()->EnableShield(SHIELD_KICKOFF, m_pPl->GetTeamNumber(), SDKGameRules()->m_vKickOff);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x - m_pPl->GetTeam()->m_nRight * 30, m_vPos.y, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		m_bShotsBlocked = true;

		EmitSound("Ball.Whistle");

		IGameEvent *pEvent = gameeventmanager->CreateEvent("kickoff");
		if (pEvent)
		{
			pEvent->SetInt("team", m_pPl->GetTeamNumber());
			gameeventmanager->FireEvent(pEvent);
		}
	}

	if (!CSDKPlayer::IsOnField(m_pOtherPl, SDKGameRules()->GetKickOffTeam()) || m_pOtherPl == m_pPl)
	{
		m_pOtherPl = NULL;

		m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_ATTACKER | FL_POS_RIGHT, false, (1 << (m_pPl->entindex() - 1)));
		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_ATTACKER | FL_POS_CENTER, false, (1 << (m_pPl->entindex() - 1)));
		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_ATTACKER | FL_POS_LEFT, false, (1 << (m_pPl->entindex() - 1)));

		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_MIDFIELDER | FL_POS_RIGHT, false, (1 << (m_pPl->entindex() - 1)));
		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_MIDFIELDER | FL_POS_CENTER, false, (1 << (m_pPl->entindex() - 1)));
		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_MIDFIELDER | FL_POS_LEFT, false, (1 << (m_pPl->entindex() - 1)));

		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_DEFENDER | FL_POS_RIGHT, false, (1 << (m_pPl->entindex() - 1)));
		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_DEFENDER | FL_POS_CENTER, false, (1 << (m_pPl->entindex() - 1)));
		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_DEFENDER | FL_POS_LEFT, false, (1 << (m_pPl->entindex() - 1)));

		if (m_pOtherPl)
		{
			m_pOtherPl->SetPosInsideShield(Vector(m_vPos.x + m_pPl->GetTeam()->m_nRight * 100, m_vPos.y, SDKGameRules()->m_vKickOff.GetZ()), true);
		}
	}

	if (!PlayersAtTargetPos())
		return;

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (pPl->GetFlags() & FL_ATCONTROLS)
		{
			if (pPl != m_pPl && pPl != m_pOtherPl)
				pPl->RemoveFlag(FL_ATCONTROLS);
		}
	}

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->SetShotButtonsReleased(false);
		m_bShotsBlocked = false;
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsShooting())
	{
		RemoveAllTouches();
		SetVel(m_vPlForward2D * 350, 0, 0, BODY_PART_FEET, false, false, false);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		if (m_pOtherPl)
			m_pOtherPl->RemoveFlag(FL_ATCONTROLS);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_KICKOFF_Leave(ball_state_t newState)
{
	m_bShotsBlocked = false;
}

void CMatchBall::State_GOALKICK_Enter()
{
	Vector ballPos;
	if (m_vTriggerTouchPos.x - SDKGameRules()->m_vKickOff.GetX() > 0)
		ballPos = GetGlobalTeam(LastOppTeam(false))->m_vGoalkickLeft;
	else
		ballPos = GetGlobalTeam(LastOppTeam(false))->m_vGoalkickRight;

	SetPos(ballPos);
}

void CMatchBall::State_GOALKICK_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl, LastOppTeam(false)))
	{
		m_pPl = NULL;

		if (CSDKPlayer::IsOnField(m_pSetpieceTaker, LastOppTeam(false)))
			m_pPl = m_pSetpieceTaker;

		if (!m_pPl)
			m_pPl = FindNearestPlayer(LastOppTeam(false), FL_POS_KEEPER);

		if (!m_pPl)
			m_pPl = FindNearestPlayer(LastOppTeam(false));

		if (!m_pPl)
			return State_Transition(BALL_STATE_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_GOALKICK, m_pPl->GetTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 100 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		m_bShotsBlocked = true;
	}

	if (!PlayersAtTargetPos())
		return;

	UpdateCarrier();

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_bShotsBlocked = false;
		m_pPl->SetShotButtonsReleased(false);
	}

	if (IsPlayerClose())
	{
		if (m_flSetpieceCloseStartTime == -1)
			m_flSetpieceCloseStartTime = gpGlobals->curtime;
	}
	else
		m_flSetpieceCloseStartTime = -1;

	if (m_flSetpieceCloseStartTime != -1 && gpGlobals->curtime > m_flSetpieceCloseStartTime + sv_ball_setpiece_close_time.GetFloat())
	{
		m_bNonnormalshotsBlocked = true;
	}
	else
		m_bNonnormalshotsBlocked = false;

	if (!m_bShotsBlocked
		&& m_pPl->ShotButtonsReleased()
		&& CanTouchBallXY()
		&& (m_pPl->IsNormalshooting() || !m_bNonnormalshotsBlocked && (m_pPl->IsPowershooting() || m_pPl->IsChargedshooting())))
	{
		m_pPl->AddGoalKick();
		RemoveAllTouches();
		DoGroundShot(false, sv_ball_goalkick_speedcoeff.GetFloat());
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_GOALKICK_Leave(ball_state_t newState)
{
	m_bNonnormalshotsBlocked = false;
	m_bShotsBlocked = false;
}

void CMatchBall::State_CORNER_Enter()
{
	Vector ballPos;
	CTeam *pTeam = GetGlobalTeam(LastTeam(false));

	if (Sign((m_vTriggerTouchPos - SDKGameRules()->m_vKickOff).x) == -pTeam->m_nRight)
		ballPos = pTeam->m_vCornerLeft;
	else
		ballPos = pTeam->m_vCornerRight;
	
	SetPos(ballPos);
}

void CMatchBall::State_CORNER_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl, LastOppTeam(false)))
	{
		m_pPl = NULL;

		if (CSDKPlayer::IsOnField(m_pSetpieceTaker, LastOppTeam(false)))
			m_pPl = m_pSetpieceTaker;

		if (!m_pPl)
			m_pPl = FindNearestPlayer(LastOppTeam(false));

		if (!m_pPl)
			return State_Transition(BALL_STATE_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_CORNER, m_pPl->GetTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x - 25 * Sign((SDKGameRules()->m_vKickOff - m_vPos).x), m_vPos.y - 25 * Sign((SDKGameRules()->m_vKickOff - m_vPos).y), SDKGameRules()->m_vKickOff[2]), true);
		m_flStateTimelimit = -1;
		m_bShotsBlocked = true;
	}

	if (!PlayersAtTargetPos())
		return;

	UpdateCarrier();
	
	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_bShotsBlocked = false;
		m_pPl->SetShotButtonsReleased(false);
	}

	if (IsPlayerClose())
	{
		if (m_flSetpieceCloseStartTime == -1)
			m_flSetpieceCloseStartTime = gpGlobals->curtime;
	}
	else
		m_flSetpieceCloseStartTime = -1;

	if (gpGlobals->curtime <= m_flStateEnterTime + sv_ball_nonnormalshotsblocktime_corner.GetFloat()
		|| m_flSetpieceCloseStartTime != -1 && gpGlobals->curtime > m_flSetpieceCloseStartTime + sv_ball_setpiece_close_time.GetFloat())
	{
		m_bNonnormalshotsBlocked = true;
	}
	else
		m_bNonnormalshotsBlocked = false;

	if (!m_bShotsBlocked
		&& m_pPl->ShotButtonsReleased()
		&& CanTouchBallXY()
		&& (m_pPl->IsNormalshooting() || !m_bNonnormalshotsBlocked && (m_pPl->IsPowershooting() || m_pPl->IsChargedshooting())))
	{
		//EmitSound("Crowd.Way");
		m_pPl->AddCorner();
		RemoveAllTouches();
		DoGroundShot(false);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_CORNER_Leave(ball_state_t newState)
{
	m_bNonnormalshotsBlocked = false;
	m_bShotsBlocked = false;
}

void CMatchBall::State_GOAL_Enter()
{
	bool isOwnGoal;
	int scoringTeam;
	CSDKPlayer *pScorer = NULL;
	CSDKPlayer *pFirstAssister = NULL;
	CSDKPlayer *pSecondAssister = NULL;
	GetGoalInfo(isOwnGoal, scoringTeam, &pScorer, &pFirstAssister, &pSecondAssister);

	m_pPl = pScorer;

	float delay = sv_ball_goalcelebduration.GetFloat();

	if (sv_replays.GetBool())
	{
		delay += sv_replay_duration1.GetFloat();

		if (sv_replay_count.GetInt() >= 2)
			delay += sv_replay_duration2.GetFloat();

		if (sv_replay_count.GetInt() >= 3)
			delay += sv_replay_duration3.GetFloat();
	}

	IGameEvent *pEvent = gameeventmanager->CreateEvent("transition");
	if (pEvent)
	{
		pEvent->SetInt("firstdelay", sv_ball_goalcelebduration.GetInt());
		pEvent->SetInt("seconddelay", delay);
		gameeventmanager->FireEvent(pEvent);
	}

	State_Transition(BALL_STATE_KICKOFF, delay);

	ReplayManager()->StartReplay(false);
}

void CMatchBall::State_GOAL_Think()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (pPl->GetTeamNumber() == m_nTeam)
			pPl->AddFlag(FL_FROZEN);
		else
			pPl->AddFlag(FL_CELEB);
	}
}

void CMatchBall::State_GOAL_Leave(ball_state_t newState)
{
	ReplayManager()->StopReplay();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->RemoveFlag(FL_CELEB | FL_FROZEN);
		pPl->DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
	}
}

void CMatchBall::State_FREEKICK_Enter()
{
	HandleFoul();
	SetPos(m_vFoulPos);
}

void CMatchBall::State_FREEKICK_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl, m_nFouledTeam))
	{
		m_pPl = NULL;

		if (CSDKPlayer::IsOnField(m_pSetpieceTaker, m_nFouledTeam))
			m_pPl = m_pSetpieceTaker;

		if (!m_pPl && (m_vPos - GetGlobalTeam(m_nFouledTeam)->m_vGoalCenter).Length2D() <= sv_ball_freekickdist_owngoal.GetInt()) // Close to own goal
			m_pPl = FindNearestPlayer(m_nFouledTeam, FL_POS_KEEPER);

		if (!CSDKPlayer::IsOnField(m_pPl) || m_pPl->GetTeamPosType() == POS_GK && m_pPl->IsBot())
			m_pPl = FindNearestPlayer(m_nFouledTeam, FL_POS_FIELD);

		if (!m_pPl)
			return State_Transition(BALL_STATE_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_FREEKICK, m_pPl->GetTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 35 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		m_bShotsBlocked = true;
	}

	if (!PlayersAtTargetPos())
		return;

	UpdateCarrier();

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_flSetpieceCloseStartTime = -1;
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_bShotsBlocked = false;
		m_pPl->SetShotButtonsReleased(false);
	}

	if (IsPlayerClose())
	{
		if (m_flSetpieceCloseStartTime == -1)
			m_flSetpieceCloseStartTime = gpGlobals->curtime;
	}
	else
		m_flSetpieceCloseStartTime = -1;

	if (abs(m_vPos.y - GetGlobalTeam(m_nFoulingTeam)->m_vGoalCenter.GetY()) <= sv_ball_freekickdist_opponentgoal.GetInt()
		&& gpGlobals->curtime <= m_flStateEnterTime + sv_ball_nonnormalshotsblocktime_freekick.GetFloat()
		|| m_flSetpieceCloseStartTime != -1 && gpGlobals->curtime > m_flSetpieceCloseStartTime + sv_ball_setpiece_close_time.GetFloat())
	{
		m_bNonnormalshotsBlocked = true;
	}
	else
		m_bNonnormalshotsBlocked = false;

	if (!m_bShotsBlocked
		&& m_pPl->ShotButtonsReleased()
		&& CanTouchBallXY()
		&& (m_pPl->IsNormalshooting() || !m_bNonnormalshotsBlocked && (m_pPl->IsPowershooting() || m_pPl->IsChargedshooting())))
	{
		//EmitSound("Crowd.Way");
		m_pPl->AddFreeKick();
		RemoveAllTouches();
		DoGroundShot(true, sv_ball_freekick_speedcoeff.GetFloat());
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_FREEKICK_Leave(ball_state_t newState)
{
	SDKGameRules()->SetOffsideLinesEnabled(false);
	m_bNonnormalshotsBlocked = false;
	m_bShotsBlocked = false;
}

void CMatchBall::State_PENALTY_Enter()
{
	if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
	{
		SetPos(GetGlobalTeam(m_nFoulingTeam)->m_vPenalty);
	}
	else
	{
		HandleFoul();
		SetPos(GetGlobalTeam(m_nFoulingTeam)->m_vPenalty);
	}
}

void CMatchBall::State_PENALTY_Think()
{
	bool takerHasChanged = false;

	if (!CSDKPlayer::IsOnField(m_pPl, m_nFouledTeam))
	{
		m_pPl = NULL;

		if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		{
			m_pPl = m_pFouledPl;

			if (!CSDKPlayer::IsOnField(m_pPl, m_nFouledTeam))
			{
				m_ePenaltyState = PENALTY_ABORTED_NO_TAKER;
				return State_Transition(BALL_STATE_NORMAL);
			}
		}
		else
		{
			if (CSDKPlayer::IsOnField(m_pSetpieceTaker, m_nFouledTeam))
				m_pPl = m_pSetpieceTaker;

			if (!m_pPl)
				m_pPl = m_pFouledPl;
			if (!CSDKPlayer::IsOnField(m_pPl))
				m_pPl = FindNearestPlayer(m_nFouledTeam);
			if (!m_pPl)
				return State_Transition(BALL_STATE_NORMAL);
		}

		SDKGameRules()->EnableShield(SHIELD_PENALTY, m_nFouledTeam, GetGlobalTeam(m_nFoulingTeam)->m_vPenalty);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 150 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		m_bShotsBlocked = true;
		takerHasChanged = true;
	}

	if (!CSDKPlayer::IsOnField(m_pOtherPl) || takerHasChanged)
	{
		if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		{
			m_pOtherPl = FindNearestPlayer(m_pPl->GetOppTeamNumber(), FL_POS_KEEPER);
			if (!m_pOtherPl)
			{
				m_ePenaltyState = PENALTY_ABORTED_NO_KEEPER;
				return State_Transition(BALL_STATE_NORMAL);
			}
		}
		else
		{
			m_pOtherPl = FindNearestPlayer(m_nInPenBoxOfTeam, FL_POS_KEEPER);
			if (!m_pOtherPl)
				return State_Transition(BALL_STATE_NORMAL);
		}

		Vector pos = m_pOtherPl->GetTeam()->m_vGoalCenter + Vector(0, m_pOtherPl->GetTeam()->m_nForward * 20, 0);
		m_pOtherPl->SetPosInsideShield(pos, false);
		m_pOtherPl->AddFlag(FL_NO_Y_MOVEMENT);
	}

	if (!PlayersAtTargetPos())
		return;

	UpdateCarrier();

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_flSetpieceCloseStartTime = -1;
		m_pPl->RemoveFlag(FL_ATCONTROLS);
	}

	if (m_bShotsBlocked && gpGlobals->curtime - m_flStateEnterTime > sv_ball_shotsblocktime_penalty.GetFloat())
	{
		m_bShotsBlocked = false;
		m_pPl->SetShotButtonsReleased(false);
	}

	if (IsPlayerClose())
	{
		if (m_flSetpieceCloseStartTime == -1)
			m_flSetpieceCloseStartTime = gpGlobals->curtime;
	}
	else
		m_flSetpieceCloseStartTime = -1;

	if (m_flSetpieceCloseStartTime != -1 && gpGlobals->curtime > m_flSetpieceCloseStartTime + sv_ball_setpiece_close_time.GetFloat())
	{
		m_bNonnormalshotsBlocked = true;
	}
	else
		m_bNonnormalshotsBlocked = false;

	if (!m_bShotsBlocked
		&& m_pPl->ShotButtonsReleased()
		&& CanTouchBallXY()
		&& (m_pPl->IsNormalshooting() || !m_bNonnormalshotsBlocked && (m_pPl->IsPowershooting() || m_pPl->IsChargedshooting())))
	{
		m_pPl->AddPenalty();
		RemoveAllTouches();
		m_ePenaltyState = PENALTY_KICKED;
		m_pPl->m_ePenaltyState = PENALTY_KICKED;
		DoGroundShot(false);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_PENALTY_Leave(ball_state_t newState)
{
	if (CSDKPlayer::IsOnField(m_pOtherPl))
		m_pOtherPl->RemoveFlag(FL_NO_Y_MOVEMENT);

	m_bNonnormalshotsBlocked = false;
	m_bShotsBlocked = false;
}