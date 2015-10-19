#include "cbase.h"
#include "match_ball.h"
#include "team.h"
#include "ios_replaymanager.h"

extern ConVar
sv_ball_bodypos_keeperhands,
sv_ball_chargedshot_minstrength,
sv_ball_chargedshot_maxstrength,
sv_ball_keepershot_minangle,
sv_ball_maxplayerfinddist,
sv_ball_shottaker_mindelay_short,
sv_ball_shottaker_mindelay_long,
sv_ball_keeperthrow_strength,
sv_ball_slidezstart,
sv_ball_slidezend,
sv_ball_slidesidereach_ball,
sv_ball_slideforwardreach_ball,
sv_ball_slidebackwardreach_ball;

ConVar
	sv_ball_statetransition_postmessagedelay_short("sv_ball_statetransition_postmessagedelay_short", "0.1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_statetransition_postmessagedelay_normal("sv_ball_statetransition_postmessagedelay_normal", "1.25", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_statetransition_postmessagedelay_long("sv_ball_statetransition_postmessagedelay_long", "2.0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_statetransition_messagedelay_normal("sv_ball_statetransition_messagedelay_normal", "0.5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_statetransition_messagedelay_short("sv_ball_statetransition_messagedelay_short", "0.1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_yellowcardballdist_forward("sv_ball_yellowcardballdist_forward", "70", FCVAR_NOTIFY),
	sv_ball_yellowcardballdist_backward("sv_ball_yellowcardballdist_backward", "70", FCVAR_NOTIFY),
	sv_ball_goalreplay_count("sv_ball_goalreplay_count", "2", FCVAR_NOTIFY),
	sv_ball_goalreplay_delay("sv_ball_goalreplay_delay", "1", FCVAR_NOTIFY),
	sv_ball_stats_pass_mindist("sv_ball_stats_pass_mindist", "300", FCVAR_NOTIFY),
	sv_ball_stats_clearance_minspeed("sv_ball_stats_clearance_minspeed", "720", FCVAR_NOTIFY),
	sv_ball_stats_shot_mindist("sv_ball_stats_shot_mindist", "270", FCVAR_NOTIFY),
	sv_ball_stats_save_minspeed("sv_ball_stats_save_minspeed", "720", FCVAR_NOTIFY),
	sv_ball_stats_assist_maxtime("sv_ball_stats_assist_maxtime", "8", FCVAR_NOTIFY),
	sv_ball_advantage_duration_freekick("sv_ball_advantage_duration_freekick", "3", FCVAR_NOTIFY),
	sv_ball_advantage_ignore_duration_freekick("sv_ball_advantage_ignore_duration_freekick", "1", FCVAR_NOTIFY),
	sv_ball_advantage_duration_penalty("sv_ball_advantage_duration_penalty", "1.5", FCVAR_NOTIFY),
	sv_ball_advantage_ignore_duration_penalty("sv_ball_advantage_ignore_duration_penalty", "1", FCVAR_NOTIFY),
	sv_ball_offsidedist("sv_ball_offsidedist", "60", FCVAR_NOTIFY),
	sv_ball_goalcelebduration("sv_ball_goalcelebduration", "8.0", FCVAR_NOTIFY),
	sv_ball_setpiece_close_time("sv_ball_setpiece_close_time", "0.75", FCVAR_NOTIFY),
	sv_ball_setpiece_close_dist( "sv_ball_setpiece_close_dist", "75", FCVAR_NOTIFY ),
	sv_ball_doubletouchfouls("sv_ball_doubletouchfouls", "1", FCVAR_NOTIFY),
	sv_ball_player_secondyellowcard_banduration("sv_ball_player_secondyellowcard_banduration", "15", FCVAR_NOTIFY),
	sv_ball_player_redcard_banduration("sv_ball_player_redcard_banduration", "15", FCVAR_NOTIFY),
	sv_ball_freekickdist_owngoal("sv_ball_freekickdist_owngoal", "850", FCVAR_NOTIFY),
	sv_ball_freekickdist_opponentgoal("sv_ball_freekickdist_opponentgoal", "1300", FCVAR_NOTIFY),
	sv_ball_freekickangle_opponentgoal("sv_ball_freekickangle_opponentgoal", "60", FCVAR_NOTIFY),
	sv_ball_closetogoaldist("sv_ball_closetogoaldist", "1300", FCVAR_NOTIFY),
	sv_ball_chargedshotblocktime_freekick("sv_ball_chargedshotblocktime_freekick", "4.0", FCVAR_NOTIFY),
	sv_ball_chargedshotblocktime_corner("sv_ball_chargedshotblocktime_corner", "4.0", FCVAR_NOTIFY),
	sv_ball_shotsblocktime_penalty("sv_ball_shotsblocktime_penalty", "4.0", FCVAR_NOTIFY),
	sv_ball_slidesidereach_foul( "sv_ball_slidesidereach_foul", "10", FCVAR_NOTIFY ),
	sv_ball_slideforwardreach_foul( "sv_ball_slideforwardreach_foul", "25", FCVAR_NOTIFY ),
	sv_ball_slidebackwardreach_foul( "sv_ball_slidebackwardreach_foul", "20", FCVAR_NOTIFY ),
	sv_ball_powerthrow_strength("sv_ball_powerthrow_strength", "720", FCVAR_NOTIFY),
	sv_ball_chargedthrow_minstrength("sv_ball_chargedthrow_minstrength", "450", FCVAR_NOTIFY),
	sv_ball_chargedthrow_maxstrength("sv_ball_chargedthrow_maxstrength", "990", FCVAR_NOTIFY),
	sv_ball_timelimit_setpiece("sv_ball_timelimit_setpiece", "15", FCVAR_NOTIFY),
	sv_ball_timelimit_remotecontrolled("sv_ball_timelimit_remotecontrolled", "15", FCVAR_NOTIFY),
	sv_ball_throwin_minangle("sv_ball_throwin_minangle", "-5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_throwin_minstrength("sv_ball_throwin_minstrength", "270", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_foulcheckdelay("sv_ball_foulcheckdelay", "2.0", FCVAR_NOTIFY),
	sv_ball_foulcheckmaxdist("sv_ball_foulcheckmaxdist", "200", FCVAR_NOTIFY);


LINK_ENTITY_TO_CLASS( match_ball, CMatchBall );

BEGIN_DATADESC(	CMatchBall )
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CMatchBall, DT_MatchBall )
	SendPropEHandle(SENDINFO(m_pLastActivePlayer)),
	SendPropInt(SENDINFO(m_nLastActiveTeam)),
END_SEND_TABLE()

CMatchBall *g_pMatchBall = NULL;

CMatchBall *CreateMatchBall(const Vector &pos)
{
	if (!g_pMatchBall)
		g_pMatchBall = static_cast<CMatchBall *>(CreateEntityByName("match_ball"));

	g_pMatchBall->SetAbsOrigin(pos);
	g_pMatchBall->Spawn();
	g_pMatchBall->SetPos(pos);

	return g_pMatchBall;
}

CMatchBall *GetMatchBall()
{
	return g_pMatchBall;
}

CMatchBall::CMatchBall()
{
	m_pLastActivePlayer = NULL;
	m_nLastActiveTeam = TEAM_NONE;
	m_pPossessingPl = NULL;
	m_nPossessingTeam = TEAM_NONE;
	m_flPossessionStart = -1;
	m_ePenaltyState = PENALTY_NONE;
	m_bIsAdvantage = false;
	m_bBallInAirAfterThrowIn = false;
	m_flStateTimelimit = -1;
}

CMatchBall::~CMatchBall()
{
	g_pMatchBall = NULL;
}

void CMatchBall::Spawn()
{
	//TODO: Move the ball skin parsing to a better spot
	CBallInfo::ParseBallSkins();
	
	CBall::Spawn();
}

void CMatchBall::Reset()
{
	CBall::Reset();

	m_pLastActivePlayer = NULL;
	m_pSetpieceTaker = NULL;
	m_flStateLeaveTime = gpGlobals->curtime;
	m_flNextStateMessageTime = -1;
	m_pOtherPl = NULL;
	m_pPossessingPl = NULL;
	m_nPossessingTeam = TEAM_NONE;
	m_flPossessionStart = -1;
	m_ePenaltyState = PENALTY_NONE;
	m_bIsAdvantage = false;
	m_bBallInAirAfterThrowIn = false;
	m_flStateTimelimit = -1;

	UnmarkOffsidePlayers();
	RemoveAllTouches();
}

void CMatchBall::State_Transition(ball_state_t nextState, float nextStateMessageDelay /*= 0*/, float nextStatePostMessageDelay /*= 0*/, bool cancelQueuedState /*= false*/)
{
	if (nextState != BALL_STATE_KEEPERHANDS)
		m_bIsAdvantage = false;

	if (nextState != BALL_STATE_NORMAL)
		m_bBallInAirAfterThrowIn = false;

	if (nextStateMessageDelay == 0 && nextStatePostMessageDelay == 0)
	{
		State_Leave(nextState);
		State_Enter(nextState, cancelQueuedState);
	}
	else
	{
		m_eNextState = nextState;
		m_flNextStateMessageTime = gpGlobals->curtime + nextStateMessageDelay;
		m_flStateLeaveTime = gpGlobals->curtime + nextStateMessageDelay + nextStatePostMessageDelay;
		m_bHasQueuedState = true;

		if (State_Get() != BALL_STATE_GOAL)
			SDKGameRules()->AddBallStateTransitionTime(nextStateMessageDelay + nextStatePostMessageDelay);
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
	m_flNextStateMessageTime = -1;

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

	if (CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl->SetChargedshotBlocked(false);
		m_pPl->SetShotsBlocked(false);
	}

	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)(newState);
	}
}

void CMatchBall::State_Think()
{
	m_pPhys->GetPosition(&m_vPos, &m_aAng);
	m_pPhys->GetVelocity(&m_vVel, &m_vRot);

	CheckFieldZone();
	CheckPenBoxPosition();

	if (m_eNextState != BALL_STATE_NONE)
	{
		if (m_flNextStateMessageTime != -1 && gpGlobals->curtime >= m_flNextStateMessageTime && m_eNextState != BALL_STATE_KICKOFF)
		{
			SendNotifications();
			m_flNextStateMessageTime = -1;
		}
		else if (gpGlobals->curtime >= m_flStateLeaveTime)
		{
			State_Leave(m_eNextState);
			State_Enter(m_eNextState, true);
			return;
		}
	}

	if (State_Get() == BALL_STATE_THROWIN || State_Get() == BALL_STATE_CORNER || State_Get() == BALL_STATE_GOALKICK || State_Get() == BALL_STATE_KICKOFF)
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

void CMatchBall::State_STATIC_Enter()
{
	SDKGameRules()->StopMeteringClockStoppedTime();
}

void CMatchBall::State_STATIC_Think()
{
}

void CMatchBall::State_STATIC_Leave(ball_state_t newState)
{
	if (!SDKGameRules()->IsIntermissionState())
		SDKGameRules()->StartMeteringClockStoppedTime();
}

void CMatchBall::State_NORMAL_Enter()
{
	m_pPhys->EnableMotion(true);
	EnablePlayerCollisions(true);
	m_pPhys->Wake();
	SDKGameRules()->StopMeteringClockStoppedTime();
	//SetMatchEvent(MATCH_EVENT_NONE);
	//SetMatchSubEvent(MATCH_EVENT_NONE);
}

void CMatchBall::State_NORMAL_Think()
{
	if (m_eNextState == BALL_STATE_GOAL)
		return;

	if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
	{
		if (m_ePenaltyState == PENALTY_KICKED && !CheckGoal() && !CheckGoalLine())
		{
			m_pPl = GetGlobalTeam(m_nFoulingTeam)->GetPlayerByPosType(POS_GK);

			if (m_pPl && (m_pPl->IsShooting() || m_pPl->IsKeeperDiving()))
			{
				UpdateCarrier();
				DoBodyPartAction();
			}
		}

		return;
	}

	if (!SDKGameRules()->IsIntermissionState() && !m_bHasQueuedState)
	{
		CheckAdvantage();

		if (!CheckOffside())
		{
			if (!CheckGoal())
			{
				if (!CheckGoalLine())
				{
					CheckSideline();
				}
			}
		}
	}

	for (int ignoredPlayerBits = 0;;)
	{
		m_pPl = FindNearestPlayer(TEAM_NONE, FL_POS_ANY, false, ignoredPlayerBits, sv_ball_maxplayerfinddist.GetFloat());

		if (!m_pPl)
			return;

		UpdateCarrier();

		// The current player was able to perform an action, so exit the loop
		if (DoBodyPartAction())
			break;

		// Exclude the current player from subsequent checks
		ignoredPlayerBits |= (1 << (m_pPl->entindex() - 1));

		m_pPl = NULL;
	}
}

void CMatchBall::State_NORMAL_Leave(ball_state_t newState)
{
	UnmarkOffsidePlayers();
	UpdatePossession(NULL);

	if (!SDKGameRules()->IsIntermissionState())
		SDKGameRules()->StartMeteringClockStoppedTime();
}

void CMatchBall::State_THROWIN_Enter()
{
	EnablePlayerCollisions(false);
}

void CMatchBall::State_THROWIN_Think()
{
	Vector groundPos = Vector(m_vTriggerTouchPos.x < SDKGameRules()->m_vKickOff.GetX() ? SDKGameRules()->m_vFieldMin.GetX() - BALL_PHYS_RADIUS - 15 : SDKGameRules()->m_vFieldMax.GetX() + BALL_PHYS_RADIUS + 15, m_vTriggerTouchPos.y, SDKGameRules()->m_vKickOff.GetZ());

	if (!CSDKPlayer::IsOnField(m_pPl, LastOppTeam(false)))
	{
		SetPos(groundPos);

		m_pPl = NULL;

		if (CSDKPlayer::IsOnField(m_pSetpieceTaker, LastOppTeam(false)))
			m_pPl = m_pSetpieceTaker;

		if (!m_pPl)
			m_pPl = FindNearestPlayer(LastOppTeam(false));

		if (!m_pPl)
		{
			SetPos(groundPos + Vector(30 * Sign(SDKGameRules()->m_vKickOff.GetX() - m_vTriggerTouchPos.x), 0, 0));
			return State_Transition(BALL_STATE_NORMAL);
		}

		SDKGameRules()->EnableShield(SHIELD_THROWIN, m_pPl->GetTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(groundPos, true);
		m_flStateTimelimit = -1;
		m_pPl->SetShotsBlocked(true);
	}

	if (!CSDKPlayer::PlayersAtTargetPos())
		return;

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		SetPos(groundPos + Vector(0, 0, m_pPl->GetPlayerMaxs().z + 2));
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_THROWIN);
		m_pPl->SetShotButtonsReleased(false);
		m_pPl->SetShotsBlocked(false);
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsChargedshooting())
	{
		QAngle ang = m_aPlAng;

		ang[PITCH] = min(sv_ball_throwin_minangle.GetFloat(), m_aPlAng[PITCH]);

		Vector dir;
		AngleVectors(ang, &dir);
		float strength = GetChargedshotStrength(GetPitchCoeff(), sv_ball_chargedthrow_minstrength.GetInt(), sv_ball_chargedthrow_maxstrength.GetInt());

		Vector vel = dir * max(strength, sv_ball_throwin_minstrength.GetInt());

		m_pPl->AddThrowIn();
		RemoveAllTouches();
		SetVel(vel, 0, FL_SPIN_FORCE_NONE, BODY_PART_HANDS, false, sv_ball_shottaker_mindelay_long.GetFloat(), true);
		m_bBallInAirAfterThrowIn = true;
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_THROWIN_Leave(ball_state_t newState)
{
	if (CSDKPlayer::IsOnField(m_pPl))
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_THROW);

	EnablePlayerCollisions(true);
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
			if (SDKGameRules()->m_nShieldType != SHIELD_KICKOFF)
				SDKGameRules()->EnableShield(SHIELD_KICKOFF, GetGlobalTeam(TEAM_HOME)->GetTeamNumber(), SDKGameRules()->m_vKickOff);

			if (!CSDKPlayer::PlayersAtTargetPos())
				return;

			return State_Transition(BALL_STATE_NORMAL);
		}

		SDKGameRules()->EnableShield(SHIELD_KICKOFF, m_pPl->GetTeamNumber(), SDKGameRules()->m_vKickOff);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x - m_pPl->GetTeam()->m_nRight * 30, m_vPos.y, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		m_pPl->SetShotsBlocked(true);

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

	if (!CSDKPlayer::PlayersAtTargetPos())
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
		m_pPl->SetShotsBlocked(false);
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsShooting())
	{
		RemoveAllTouches();
		SetVel(m_vPlForward2D * 350, 0, FL_SPIN_FORCE_NONE, BODY_PART_FEET, false, sv_ball_shottaker_mindelay_short.GetFloat(), true);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		if (m_pOtherPl)
			m_pOtherPl->RemoveFlag(FL_ATCONTROLS);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_KICKOFF_Leave(ball_state_t newState)
{
}

void CMatchBall::State_GOALKICK_Enter()
{
	Vector ballPos;

	CTeam *pTeam = GetGlobalTeam(LastOppTeam(false));

	if (Sign(m_vTriggerTouchPos.x - SDKGameRules()->m_vKickOff.GetX()) == pTeam->m_nRight)
		ballPos = pTeam->m_vGoalkickRight;
	else
		ballPos = pTeam->m_vGoalkickLeft;

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
			m_pPl = GetGlobalTeam(LastOppTeam(false))->GetPlayerByPosType(POS_GK);

		if (!m_pPl)
			m_pPl = FindNearestPlayer(LastOppTeam(false));

		if (!m_pPl)
			return State_Transition(BALL_STATE_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_GOALKICK, m_pPl->GetTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 100 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		m_pPl->SetShotsBlocked(true);
	}

	if (!CSDKPlayer::PlayersAtTargetPos())
		return;

	UpdateCarrier();

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_pPl->SetShotsBlocked(false);
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
		m_pPl->SetChargedshotBlocked(true);
	}
	else
		m_pPl->SetChargedshotBlocked(false);

	if (!m_pPl->ShotsBlocked()
		&& m_pPl->ShotButtonsReleased()
		&& CanReachBallStandingXY()
		&& (m_pPl->IsNormalshooting() || !m_pPl->ChargedshotBlocked() && m_pPl->IsChargedshooting()))
	{
		m_pPl->AddGoalKick();
		RemoveAllTouches();
		DoGroundShot(false);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_GOALKICK_Leave(ball_state_t newState)
{
}

void CMatchBall::State_CORNER_Enter()
{
	Vector ballPos;
	CTeam *pTeam = GetGlobalTeam(LastTeam(false));

	if (Sign(m_vTriggerTouchPos.x - SDKGameRules()->m_vKickOff.GetX()) == pTeam->m_nRight)
		ballPos = pTeam->m_vCornerRight;
	else
		ballPos = pTeam->m_vCornerLeft;
	
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
		m_pPl->SetShotsBlocked(true);
	}

	if (!CSDKPlayer::PlayersAtTargetPos())
		return;

	UpdateCarrier();
	
	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_pPl->SetShotsBlocked(false);
		m_pPl->SetShotButtonsReleased(false);
	}

	if (IsPlayerClose())
	{
		if (m_flSetpieceCloseStartTime == -1)
			m_flSetpieceCloseStartTime = gpGlobals->curtime;
	}
	else
		m_flSetpieceCloseStartTime = -1;

	if (gpGlobals->curtime <= m_flStateEnterTime + sv_ball_chargedshotblocktime_corner.GetFloat()
		|| m_flSetpieceCloseStartTime != -1 && gpGlobals->curtime > m_flSetpieceCloseStartTime + sv_ball_setpiece_close_time.GetFloat())
	{
		m_pPl->SetChargedshotBlocked(true);
	}
	else
		m_pPl->SetChargedshotBlocked(false);

	if (!m_pPl->ShotsBlocked()
		&& m_pPl->ShotButtonsReleased()
		&& CanReachBallStandingXY()
		&& (m_pPl->IsNormalshooting() || !m_pPl->ChargedshotBlocked() && m_pPl->IsChargedshooting()))
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

	State_Transition(BALL_STATE_KICKOFF, 0, delay);

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
			pPl->AddFlag(FL_ATCONTROLS | FL_USE_TV_CAM);
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

		pPl->RemoveFlag(FL_CELEB | FL_ATCONTROLS | FL_USE_TV_CAM);
		pPl->DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
	}
}

void CMatchBall::State_FREEKICK_Enter()
{
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
			m_pPl = GetGlobalTeam(m_nFouledTeam)->GetPlayerByPosType(POS_GK);

		if (!CSDKPlayer::IsOnField(m_pPl) || m_pPl->GetTeamPosType() == POS_GK && m_pPl->IsBot())
			m_pPl = FindNearestPlayer(m_nFouledTeam);

		if (!m_pPl)
			return State_Transition(BALL_STATE_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_FREEKICK, m_pPl->GetTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 35 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		m_pPl->SetShotsBlocked(true);
	}

	if (!CSDKPlayer::PlayersAtTargetPos())
		return;

	UpdateCarrier();

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_flSetpieceCloseStartTime = -1;
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_pPl->SetShotsBlocked(false);
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
		&& gpGlobals->curtime <= m_flStateEnterTime + sv_ball_chargedshotblocktime_freekick.GetFloat()
		|| m_flSetpieceCloseStartTime != -1 && gpGlobals->curtime > m_flSetpieceCloseStartTime + sv_ball_setpiece_close_time.GetFloat())
	{
		m_pPl->SetChargedshotBlocked(true);
	}
	else
		m_pPl->SetChargedshotBlocked(false);

	if (!m_pPl->ShotsBlocked()
		&& m_pPl->ShotButtonsReleased()
		&& CanReachBallStandingXY()
		&& (m_pPl->IsNormalshooting() || !m_pPl->ChargedshotBlocked() && m_pPl->IsChargedshooting()))
	{
		//EmitSound("Crowd.Way");
		m_pPl->AddFreeKick();
		RemoveAllTouches();
		DoGroundShot(true);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_FREEKICK_Leave(ball_state_t newState)
{
	SDKGameRules()->SetOffsideLinesEnabled(false);
}

void CMatchBall::State_PENALTY_Enter()
{
	if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
	{
		SetPos(GetGlobalTeam(m_nFoulingTeam)->m_vPenalty);
	}
	else
	{
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
				SetPenaltyState(PENALTY_ABORTED_NO_TAKER);
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
		m_pPl->SetShotsBlocked(true);
		takerHasChanged = true;
	}

	if (!CSDKPlayer::IsOnField(m_pOtherPl) || takerHasChanged)
	{
		if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		{
			m_pOtherPl = GetGlobalTeam(m_pPl->GetOppTeamNumber())->GetPlayerByPosType(POS_GK);
			if (!m_pOtherPl)
			{
				SetPenaltyState(PENALTY_ABORTED_NO_KEEPER);
				return State_Transition(BALL_STATE_NORMAL);
			}
		}
		else
		{
			m_pOtherPl = GetGlobalTeam(m_nInPenBoxOfTeam)->GetPlayerByPosType(POS_GK);
			if (!m_pOtherPl)
				return State_Transition(BALL_STATE_NORMAL);
		}

		Vector pos = m_pOtherPl->GetTeam()->m_vGoalCenter + Vector(0, m_pOtherPl->GetTeam()->m_nForward * 5, 0);
		m_pOtherPl->SetPosInsideShield(pos, false);
		m_pOtherPl->AddFlag(FL_NO_Y_MOVEMENT);
	}

	if (!CSDKPlayer::PlayersAtTargetPos())
		return;

	UpdateCarrier();

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_flSetpieceCloseStartTime = -1;
		m_pPl->RemoveFlag(FL_ATCONTROLS);
	}

	if (m_pPl->ShotsBlocked() && gpGlobals->curtime - m_flStateEnterTime > sv_ball_shotsblocktime_penalty.GetFloat())
	{
		m_pPl->SetShotsBlocked(false);
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
		m_pPl->SetChargedshotBlocked(true);
	}
	else
		m_pPl->SetChargedshotBlocked(false);

	if (!m_pPl->ShotsBlocked()
		&& m_pPl->ShotButtonsReleased()
		&& CanReachBallStandingXY()
		&& (m_pPl->IsNormalshooting() || !m_pPl->ChargedshotBlocked() && m_pPl->IsChargedshooting()))
	{
		if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		{
			SetPenaltyState(PENALTY_KICKED);
			m_pPl->m_ePenaltyState = PENALTY_KICKED;
		}
		else
			m_pPl->AddPenalty();

		RemoveAllTouches();
		DoGroundShot(false);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_PENALTY_Leave(ball_state_t newState)
{
	if (CSDKPlayer::IsOnField(m_pOtherPl))
		m_pOtherPl->RemoveFlag(FL_NO_Y_MOVEMENT);
}

void CMatchBall::State_KEEPERHANDS_Enter()
{
	SetPos(m_vPos, false);
}

void CMatchBall::State_KEEPERHANDS_Think()
{
	int wasOrIsinPenBoxOfTeam = m_nInPenBoxOfTeam != TEAM_NONE ? m_nInPenBoxOfTeam : m_nWasInPenBoxOfTeam;

	if (!CSDKPlayer::IsOnField(m_pPl, wasOrIsinPenBoxOfTeam))
	{
		m_pPl = GetGlobalTeam(wasOrIsinPenBoxOfTeam)->GetPlayerByPosType(POS_GK);

		if (!m_pPl)
			return State_Transition(BALL_STATE_NORMAL);

		if (!SDKGameRules()->IsIntermissionState() && !m_bHasQueuedState && SDKGameRules()->State_Get() != MATCH_PERIOD_PENALTIES)
		{
			SDKGameRules()->EnableShield(SHIELD_KEEPERHANDS, m_pPl->GetTeamNumber(), m_vPos);
			m_pPl->m_bIsAtTargetPos = true;
		}

		m_pPl->SetShotButtonsReleased(false);
		m_pHoldingPlayer = m_pPl;
		m_pPl->m_pHoldingBall = this;
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_CARRY);
		EnablePlayerCollisions(false);
		m_flStateTimelimit = -1;
		CSDKPlayer::PlayersAtTargetPos();
	}

	if (!SDKGameRules()->IsIntermissionState() && !m_bHasQueuedState && SDKGameRules()->State_Get() != MATCH_PERIOD_PENALTIES)
	{
		if (m_flStateTimelimit == -1)
		{
			m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		}
	}

	UpdateCarrier();

	Vector min = GetGlobalTeam(m_pPl->GetTeamNumber())->m_vPenBoxMin - Vector(BALL_PHYS_RADIUS, BALL_PHYS_RADIUS, 0);
	Vector max = GetGlobalTeam(m_pPl->GetTeamNumber())->m_vPenBoxMax + Vector(BALL_PHYS_RADIUS, BALL_PHYS_RADIUS, 0);

	// Ball outside the penalty box
	if (m_nInPenBoxOfTeam == TEAM_NONE)
	{
		Vector vel, pos;

		// Throw the ball towards the kick-off spot if it's behind the goal line and either would end up inside the goal or is in a map with a walled field
		if ((m_pPl->GetTeam()->m_nForward == 1 && m_vPos.y < min.y || m_pPl->GetTeam()->m_nForward == -1 && m_vPos.y > max.y)
			&& (SDKGameRules()->HasWalledField()
				|| (m_vPos.x >= m_pPl->GetTeam()->m_vGoalCenter.GetX() - SDKGameRules()->m_vGoalTriggerSize.x
					&& m_vPos.x <= m_pPl->GetTeam()->m_vGoalCenter.GetX() + SDKGameRules()->m_vGoalTriggerSize.x)))
		{
			pos = Vector(m_vPos.x, (m_pPl->GetTeam()->m_nForward == 1 ? SDKGameRules()->m_vFieldMin.GetY() - BALL_PHYS_RADIUS : SDKGameRules()->m_vFieldMax.GetY() + BALL_PHYS_RADIUS) + m_pPl->GetTeam()->m_nForward * 36, m_vPos.z);
			vel = 25 * Vector(0, m_pPl->GetTeam()->m_nForward, 0);
		}
		else
		{
			pos = m_vPos;
			vel = m_vPlVel;
		}

		RemoveAllTouches();
		SetPos(pos, false);
		SetVel(vel, 0, FL_SPIN_FORCE_NONE, BODY_PART_KEEPERHANDS, true, sv_ball_shottaker_mindelay_short.GetFloat(), true);

		return State_Transition(BALL_STATE_NORMAL);
	}

	Vector handPos;
	QAngle handAng;
	m_pPl->GetAttachment("keeperballrighthand", handPos, handAng);
	SetPos(handPos, false);
	SetAng(handAng);

	if (m_bIsAdvantage)
	{
		if (m_bIsPenalty)
			State_Transition(BALL_STATE_PENALTY, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());
		else
			State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());

		return;
	}

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsChargedshooting() && m_pPl->CanShoot())
	{
		Vector vel;
		int spinFlags;
		PlayerAnimEvent_t animEvent;

		if (m_aPlAng[PITCH] > sv_ball_keepershot_minangle.GetInt())
		{
			Vector dir;
			AngleVectors(m_aPlAng, &dir);
			vel = m_vPlForwardVel2D + dir * sv_ball_keeperthrow_strength.GetInt();
			spinFlags = FL_SPIN_FORCE_NONE;
			animEvent = PLAYERANIMEVENT_KEEPER_HANDS_THROW;
		}
		else
		{
			QAngle ang = m_aPlAng;
			ang[PITCH] = min(sv_ball_keepershot_minangle.GetFloat(), m_aPlAng[PITCH]);
			Vector dir;
			AngleVectors(m_aPlAng, &dir);
			vel = dir * GetChargedshotStrength(GetPitchCoeff(), sv_ball_chargedshot_minstrength.GetInt(), sv_ball_chargedshot_maxstrength.GetInt());
			spinFlags = FL_SPIN_PERMIT_SIDE;
			animEvent = PLAYERANIMEVENT_KEEPER_HANDS_KICK;

			if (vel.Length() > 1000)
				EmitSound("Ball.Kickhard");
			else
				EmitSound("Ball.Kicknormal");
		}

		m_pPl->DoServerAnimationEvent(animEvent);
		RemoveAllTouches();
		SetPos(Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_keeperhands.GetFloat()) + m_vPlForward2D * 36, false);
		SetVel(vel, 1.0f, spinFlags, BODY_PART_KEEPERHANDS, true, sv_ball_shottaker_mindelay_short.GetFloat(), true);

		return State_Transition(BALL_STATE_NORMAL);
	}
}

void CMatchBall::State_KEEPERHANDS_Leave(ball_state_t newState)
{
	RemoveFromPlayerHands(m_pPl);
}

bool CMatchBall::CheckSideline()
{
	if (m_bBallInAirAfterThrowIn)
		return false;

	if (m_vPos.x + BALL_PHYS_RADIUS >= SDKGameRules()->m_vFieldMin.GetX() && m_vPos.x - BALL_PHYS_RADIUS <= SDKGameRules()->m_vFieldMax.GetX())
		return false;

	if (m_bIsAdvantage)
	{
		if (m_bIsPenalty)
			State_Transition(BALL_STATE_PENALTY, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());
		else
			State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());

		return true;
	}

	if (SDKGameRules()->HasWalledField())
	{
		float safePosX = m_vPos.x < SDKGameRules()->m_vKickOff.GetX() ? SDKGameRules()->m_vFieldMin.GetX() + 50 : SDKGameRules()->m_vFieldMax.GetX() - 50;
		SetPos(Vector(safePosX, m_vPos.y, SDKGameRules()->m_vKickOff.GetZ()), true, false);
		return true;
	}

	BallTouchInfo *pInfo = LastInfo(true);
	CSDKPlayer *pLastPl = LastPl(true);

	if (pInfo && CSDKPlayer::IsOnField(pLastPl))
	{
		if (m_bHitThePost) // Goal post hits don't trigger a statistic change right away, since we don't know if it ends up being a goal or a miss. So do the check here.
			pLastPl->AddShot();
		else if (pInfo->m_eBodyPart != BODY_PART_KEEPERPUNCH
			&& GetVel().Length2DSqr() < pow(sv_ball_stats_clearance_minspeed.GetFloat(), 2.0f)
			&& (m_vPos - pInfo->m_vBallPos).Length2DSqr() >= pow(sv_ball_stats_pass_mindist.GetFloat(), 2.0f)) // Pass or interception if over a distance threshold and wasn't punched away by a keeper
		{
			pLastPl->AddPass();
		}
	}

	m_vTriggerTouchPos = m_vPos;

	//SetMatchEvent(MATCH_EVENT_THROWIN, NULL, LastOppTeam(false));
	State_Transition(BALL_STATE_THROWIN, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_normal.GetFloat());

	return true;
}

bool CMatchBall::CheckGoalLine()
{
	int team = TEAM_NONE;

	if (m_vPos.y + BALL_PHYS_RADIUS < SDKGameRules()->m_vFieldMin.GetY())
		team = SDKGameRules()->GetBottomTeam();
	else if (m_vPos.y - BALL_PHYS_RADIUS > SDKGameRules()->m_vFieldMax.GetY())
		team = GetGlobalTeam(SDKGameRules()->GetBottomTeam())->GetOppTeamNumber();

	if (team == TEAM_NONE)
		return false;

	if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
	{
		SetPenaltyState(PENALTY_MISSED);
		m_bHasQueuedState = true;
		return true;
	}

	if (m_bIsAdvantage)
	{
		if (m_bIsPenalty)
			State_Transition(BALL_STATE_PENALTY, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());
		else
			State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());

		return true;
	}

	if (SDKGameRules()->HasWalledField())
	{
		float safePosY = m_vPos.y < SDKGameRules()->m_vKickOff.GetY() ? SDKGameRules()->m_vFieldMin.GetY() + 50 : SDKGameRules()->m_vFieldMax.GetY() - 50;
		SetPos(Vector(m_vPos.x, safePosY, SDKGameRules()->m_vKickOff.GetZ()), true, false);
		return true;
	}

	m_vTriggerTouchPos = m_vPos;

	BallTouchInfo *pInfo = LastInfo(true);

	if (pInfo->m_nTeam != team && pInfo->m_pPl && (m_vTriggerTouchPos - pInfo->m_vBallPos).Length2DSqr() >= pow(sv_ball_stats_shot_mindist.GetFloat(), 2.0f)) // Don't add a missed shot or pass if the player accidentally dribbles the ball out
	{
		float minX = GetGlobalTeam(team)->m_vSixYardBoxMin.GetX();
		float maxX = GetGlobalTeam(team)->m_vSixYardBoxMax.GetX();

		if (m_bHitThePost || m_vTriggerTouchPos.x >= minX && m_vTriggerTouchPos.x <= maxX) // Bounced off the post or crossed the goal line inside the six-yard box
		{
			LastPl(true)->AddShot();
			//EmitSound("Crowd.Miss");
			ReplayManager()->AddMatchEvent(MATCH_EVENT_MISS, GetGlobalTeam(team)->GetOppTeamNumber(), LastPl(true));
		}
		else
			LastPl(true)->AddPass();
	}

	if (LastTeam(false) == team)
	{
		//SetMatchEvent(MATCH_EVENT_CORNER, NULL, LastOppTeam(false));
		State_Transition(BALL_STATE_CORNER, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_normal.GetFloat());
	}
	else
	{
		//SetMatchEvent(MATCH_EVENT_THROWIN, NULL, LastOppTeam(false));
		State_Transition(BALL_STATE_GOALKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_normal.GetFloat());
	}

	return true;
}

bool CMatchBall::CheckGoal()
{
	int team = TEAM_NONE;

	if (m_vPos.x >= SDKGameRules()->m_vKickOff.GetX() - SDKGameRules()->m_vGoalTriggerSize.x
		&& m_vPos.x <= SDKGameRules()->m_vKickOff.GetX() + SDKGameRules()->m_vGoalTriggerSize.x
		&& m_vPos.z <= SDKGameRules()->m_vKickOff.GetZ() + SDKGameRules()->m_vGoalTriggerSize.z)
	{
		if (m_vPos.y + BALL_PHYS_RADIUS < SDKGameRules()->m_vFieldMin.GetY())
			team = SDKGameRules()->GetBottomTeam();
		else if (m_vPos.y - BALL_PHYS_RADIUS > SDKGameRules()->m_vFieldMax.GetY())
			team = GetGlobalTeam(SDKGameRules()->GetBottomTeam())->GetOppTeamNumber();
	}

	if (team == TEAM_NONE)
		return false;

	m_nTeam = team;

	// Don't count the goal if there was advantage play for the team which concedes the goal
	if (m_bIsAdvantage && m_nTeam == m_nFouledTeam)
	{
		if (m_bIsPenalty)
			State_Transition(BALL_STATE_PENALTY, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());
		else
			State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());

		return true;
	}

	if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
	{
		if (m_nTeam == m_nFoulingTeam)
		{
			SetPenaltyState(PENALTY_SCORED);
			m_bHasQueuedState = true;
		}

		return true;
	}

	if (LastInfo(true) && LastInfo(true)->m_eBallState == BALL_STATE_THROWIN && !LastPl(false, LastPl(true)))
	{
		if (LastTeam(true) == team)
		{
			//SetMatchEvent(MATCH_EVENT_CORNER, NULL, LastOppTeam(false));
			State_Transition(BALL_STATE_CORNER, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_normal.GetFloat());
		}
		else
		{
			//SetMatchEvent(MATCH_EVENT_THROWIN, NULL, LastOppTeam(false));
			State_Transition(BALL_STATE_GOALKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_normal.GetFloat());
		}

		return true;
	}

	if (LastTeam(true) != m_nTeam && LastPl(true))
	{
		LastPl(true)->AddShot();
		LastPl(true)->AddShotOnGoal();
	}

	SDKGameRules()->SetKickOffTeam(m_nTeam);

	bool isOwnGoal;
	int scoringTeam;
	CSDKPlayer *pScorer = NULL;
	CSDKPlayer *pFirstAssister = NULL;
	CSDKPlayer *pSecondAssister = NULL;
	GetGoalInfo(isOwnGoal, scoringTeam, &pScorer, &pFirstAssister, &pSecondAssister);

	if (isOwnGoal)
	{
		if (pScorer)
			pScorer->AddOwnGoal();

		ReplayManager()->AddMatchEvent(MATCH_EVENT_OWNGOAL, scoringTeam, pScorer);
	}
	else
	{
		if (pScorer)
			pScorer->AddGoal();

		if (pFirstAssister)
			pFirstAssister->AddAssist();

		if (pSecondAssister)
			pSecondAssister->AddAssist();

		ReplayManager()->AddMatchEvent(MATCH_EVENT_GOAL, scoringTeam, pScorer, pFirstAssister, pSecondAssister);
	}

	CSDKPlayer *pKeeper = GetGlobalTeam(m_nTeam)->GetPlayerByPosType(POS_GK);
	if (pKeeper)
		pKeeper->AddGoalConceded();

	State_Transition(BALL_STATE_GOAL, sv_ball_statetransition_messagedelay_short.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());

	return true;
}

void CMatchBall::CheckFieldZone()
{
	float fieldLength = SDKGameRules()->m_vFieldMax.GetY() - SDKGameRules()->m_vFieldMin.GetY();
	float dist = GetGlobalTeam(TEAM_HOME)->m_nForward * (m_vPos.y - SDKGameRules()->m_vKickOff.GetY());
	m_flFieldZone = clamp(dist * 100 / (fieldLength / 2), -100, 100);
}

bool CMatchBall::CheckOffside()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl) || !pPl->IsOffside())
			continue;

		Vector dir = (m_vPos - pPl->GetLocalOrigin());

		if (dir.z > sv_ball_offsidedist.GetFloat() || dir.Length2DSqr() > pow(sv_ball_offsidedist.GetFloat(), 2))
			continue;

		if (m_bIsAdvantage)
		{
			if (m_bIsPenalty)
				State_Transition(BALL_STATE_PENALTY, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());
			else
				State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());
		}
		else
		{
			pPl->AddOffside();
			SetFoulParams(FOUL_OFFSIDE, pPl->GetOffsidePos(), pPl);
			SDKGameRules()->SetOffsideLinePositions(pPl->GetOffsideBallPos().y, pPl->GetOffsidePos().y, pPl->GetOffsideLastOppPlayerPos().y);
			State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_long.GetFloat());
		}

		return true;
	}

	return false;
}

void CMatchBall::SendNotifications()
{
	if (m_eNextState == BALL_STATE_GOAL)
	{
		bool isOwnGoal;
		int scoringTeam;
		CSDKPlayer *pScorer = NULL;
		CSDKPlayer *pFirstAssister = NULL;
		CSDKPlayer *pSecondAssister = NULL;
		GetGoalInfo(isOwnGoal, scoringTeam, &pScorer, &pFirstAssister, &pSecondAssister);

		if (isOwnGoal)
		{
			IGameEvent *pEvent = gameeventmanager->CreateEvent("own_goal");
			if (pEvent)
			{
				pEvent->SetInt("causing_team", GetGlobalTeam(scoringTeam)->GetOppTeamNumber());
				pEvent->SetInt("causer_userid", pScorer ? pScorer->GetUserID() : 0);
				gameeventmanager->FireEvent(pEvent);
			}
		}
		else
		{
			IGameEvent *pEvent = gameeventmanager->CreateEvent("goal");
			if (pEvent)
			{
				pEvent->SetInt("scoring_team", scoringTeam);
				pEvent->SetInt("scorer_userid", pScorer ? pScorer->GetUserID() : 0);
				pEvent->SetInt("first_assister_userid", pFirstAssister ? pFirstAssister->GetUserID() : 0);
				pEvent->SetInt("second_assister_userid", pSecondAssister ? pSecondAssister->GetUserID() : 0);
				gameeventmanager->FireEvent(pEvent);
			}
		}

		EmitSound("Ball.Whistle");
		//EmitSound("Crowd.Goal1");
		//EmitSound("Crowd.Goal2");
		//EmitSound("Crowd.Goal3");
	}
	else
	{
		switch (m_eNextState)
		{
		case BALL_STATE_THROWIN:
			{
				IGameEvent *pEvent = gameeventmanager->CreateEvent("set_piece");
				if (pEvent)
				{
					pEvent->SetInt("type", MATCH_EVENT_THROWIN);
					pEvent->SetInt("taking_team", LastOppTeam(false));
					statistic_type_t statType;

					switch (g_IOSRand.RandomInt(0, 1))
					{
					case 0:
						statType = STATISTIC_POSSESSION_TEAM;
						break;
					case 1:
						statType = STATISTIC_PASSING_TEAM;
						break;
					default:
						statType = STATISTIC_POSSESSION_TEAM;
						break;
					}

					pEvent->SetInt("statistic_type", statType);
					gameeventmanager->FireEvent(pEvent);
				}
				EmitSound("Ball.Whistle");
			}
			break;
		case BALL_STATE_GOALKICK:
			{
				IGameEvent *pEvent = gameeventmanager->CreateEvent("set_piece");
				if (pEvent)
				{
					pEvent->SetInt("type", MATCH_EVENT_GOALKICK);
					pEvent->SetInt("taking_team", LastOppTeam(false));
					pEvent->SetInt("statistic_type", STATISTIC_SHOTSONGOAL_TEAM);
					gameeventmanager->FireEvent(pEvent);
				}
				EmitSound("Ball.Whistle");
			}
			break;
		case BALL_STATE_CORNER:
			{
				IGameEvent *pEvent = gameeventmanager->CreateEvent("set_piece");
				if (pEvent)
				{
					pEvent->SetInt("type", MATCH_EVENT_CORNER);
					pEvent->SetInt("taking_team", LastOppTeam(false));
					pEvent->SetInt("statistic_type", STATISTIC_SETPIECECOUNT_TEAM);
					gameeventmanager->FireEvent(pEvent);
				}
				EmitSound("Ball.Whistle");
			}
			break;
		case BALL_STATE_FREEKICK:
		case BALL_STATE_PENALTY:
			{
				match_event_t foulType;

				switch (m_eFoulType)
				{
				case FOUL_NORMAL_NO_CARD:
					foulType = MATCH_EVENT_FOUL;
					//EmitSound("Crowd.Foul");
					break;
				case FOUL_NORMAL_YELLOW_CARD:
					foulType = MATCH_EVENT_YELLOWCARD;
					//EmitSound("Crowd.YellowCard");
					break;
				case FOUL_NORMAL_SECOND_YELLOW_CARD:
					foulType = MATCH_EVENT_SECONDYELLOWCARD;
					//EmitSound("Crowd.YellowCard");
					break;
				case FOUL_NORMAL_RED_CARD:
					foulType = MATCH_EVENT_REDCARD;
					//EmitSound("Crowd.RedCard");
					break;
				case FOUL_OFFSIDE:
					foulType = MATCH_EVENT_OFFSIDE;
					SDKGameRules()->SetOffsideLinesEnabled(true);
					//EmitSound("Crowd.Foul");
					break;
				case FOUL_DOUBLETOUCH:
					foulType = MATCH_EVENT_DOUBLETOUCH;
					//EmitSound("Crowd.Foul");
					break;
				default:
					foulType = MATCH_EVENT_FOUL;
					//EmitSound("Crowd.Foul");
					break;
				}

				IGameEvent *pEvent = gameeventmanager->CreateEvent("foul");
				if (pEvent)
				{
					pEvent->SetInt("fouling_player_userid", (m_pFoulingPl ? m_pFoulingPl->GetUserID() : 0));
					pEvent->SetInt("fouled_player_userid", (m_pFouledPl ? m_pFouledPl->GetUserID() : 0));
					pEvent->SetInt("fouling_team", m_nFoulingTeam);
					pEvent->SetInt("foul_type", foulType);
					pEvent->SetInt("set_piece_type", m_eNextState == BALL_STATE_PENALTY ? MATCH_EVENT_PENALTY : MATCH_EVENT_FREEKICK);

					float distToGoal = (m_vFoulPos - GetGlobalTeam(m_nFoulingTeam)->m_vGoalCenter).Length2D();

					statistic_type_t statType;

					if (m_eNextState == BALL_STATE_FREEKICK && distToGoal <= sv_ball_freekickdist_opponentgoal.GetInt())
						statType = STATISTIC_DISTANCETOGOAL;
					else if (m_eNextState == BALL_STATE_FREEKICK && m_eFoulType == FOUL_OFFSIDE)
						statType = STATISTIC_OFFSIDES_TEAM;
					else if (m_eNextState == BALL_STATE_FREEKICK && m_eFoulType != FOUL_DOUBLETOUCH)
						statType = STATISTIC_FOULS_TEAM;
					else
						statType = (g_IOSRand.RandomInt(0, 2) == 0 ? STATISTIC_POSSESSION_TEAM : STATISTIC_SETPIECECOUNT_TEAM);

					pEvent->SetInt("statistic_type", statType);
					pEvent->SetInt("distance_to_goal", distToGoal * 2.54f / 100);
					gameeventmanager->FireEvent(pEvent);
				}

				EmitSound("Ball.Whistle");
			}
			break;
		}
	}
}

void CMatchBall::GetGoalInfo(bool &isOwnGoal, int &scoringTeam, CSDKPlayer **pScorer, CSDKPlayer **pFirstAssister, CSDKPlayer **pSecondAssister)
{
	CSDKPlayer *pKeeper = NULL;

	// Prevent own goals from keeper punches
	if (LastInfo(true)->m_eBodyPart == BODY_PART_KEEPERPUNCH)
	{
		isOwnGoal = false;
		pKeeper = LastPl(true);
	}
	else if (m_nTeam == LastTeam(true))
		isOwnGoal = true;
	else
		isOwnGoal = false;

	if (isOwnGoal)
	{
		scoringTeam = LastOppTeam(true);
		*pScorer = LastPl(true);
	}
	else
	{
		scoringTeam = LastTeam(true, pKeeper);
		*pScorer = LastPl(true, pKeeper);

		if (!*pScorer)
			return;

		*pFirstAssister = LastPl(true, pKeeper, *pScorer);

		if (!*pFirstAssister
			|| (*pFirstAssister)->GetTeam() != (*pScorer)->GetTeam()
			|| gpGlobals->curtime - LastInfo(true, pKeeper, *pScorer)->m_flTime > sv_ball_stats_assist_maxtime.GetFloat())
		{
			*pFirstAssister = NULL;
			return;
		}

		*pSecondAssister = LastPl(true, pKeeper, *pScorer, *pFirstAssister);

		if (!*pSecondAssister
			|| (*pSecondAssister)->GetTeam() != (*pScorer)->GetTeam()
			|| gpGlobals->curtime - LastInfo(true, pKeeper, *pScorer, *pFirstAssister)->m_flTime > sv_ball_stats_assist_maxtime.GetFloat())
		{
			*pSecondAssister = NULL;
			return;
		}
	}
}

void CMatchBall::Touched(bool isShot, body_part_t bodyPart, const Vector &oldVel)
{
	// Check if touch should be recorded and statistics updated
	if (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState || SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		return;

	// Check if double touch foul
	if (m_Touches.Count() > 0 && m_Touches.Tail()->m_pPl == m_pPl && m_Touches.Tail()->m_nTeam == m_pPl->GetTeamNumber()
		&& sv_ball_doubletouchfouls.GetBool() && State_Get() == BALL_STATE_NORMAL && m_Touches.Tail()->m_eBallState != BALL_STATE_NORMAL
		&& m_Touches.Tail()->m_eBallState != BALL_STATE_KEEPERHANDS && m_pPl->GetTeam()->GetNumPlayers() > 2 && m_pPl->GetOppTeam()->GetNumPlayers() > 2)
	{
		SetFoulParams(FOUL_DOUBLETOUCH, m_pPl->GetLocalOrigin(), m_pPl);
		State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_long.GetFloat());

		return;
	}

	// Only update statistics on shots instead of touches
	if (isShot)
	{
		BallTouchInfo *pInfo = LastInfo(true);
		CSDKPlayer *pLastPl = LastPl(true);

		if (pInfo && CSDKPlayer::IsOnField(pLastPl) && pLastPl != m_pPl)
		{
			if (pInfo->m_nTeam != m_pPl->GetTeamNumber()
				&& (bodyPart == BODY_PART_KEEPERPUNCH
					|| bodyPart == BODY_PART_KEEPERCATCH
					&& oldVel.Length2DSqr() >= pow(sv_ball_stats_save_minspeed.GetInt(), 2.0f))) // All fast balls by an opponent which are caught or punched away by the keeper count as shots on goal
			{
				m_pPl->AddKeeperSave();

				if (bodyPart == BODY_PART_KEEPERCATCH)
					m_pPl->AddKeeperSaveCaught();

				pLastPl->AddShot();
				pLastPl->AddShotOnGoal();
				ReplayManager()->AddMatchEvent(MATCH_EVENT_KEEPERSAVE, m_pPl->GetTeamNumber(), m_pPl, pLastPl);
			}
			// Pass or interception
			else if ((m_vPos - pInfo->m_vBallPos).Length2DSqr() >= pow(sv_ball_stats_pass_mindist.GetInt(), 2.0f) && pInfo->m_eBodyPart != BODY_PART_KEEPERPUNCH)
			{
				if (m_bHitThePost)
				{
					pLastPl->AddShot();
				}
				else
				{
					pLastPl->AddPass();

					// Pass to teammate
					if (pInfo->m_nTeam == m_pPl->GetTeamNumber())
					{
						pLastPl->AddPassCompleted();
					}
					// Intercepted by opponent
					else
					{
						m_pPl->AddInterception();
					}
				}
			}
		}

		m_bHitThePost = false;
	}

	UpdatePossession(m_pPl);
	BallTouchInfo *info = new BallTouchInfo;
	info->m_pPl = m_pPl;
	info->m_nTeam = m_pPl->GetTeamNumber();
	info->m_bIsShot = isShot;
	info->m_eBodyPart = bodyPart;
	info->m_eBallState = State_Get();
	info->m_vBallPos = m_vPos;
	info->m_vBallVel = m_vVel;
	info->m_flTime = gpGlobals->curtime;
	m_Touches.AddToTail(info);
	m_bBallInAirAfterThrowIn = false;

	if (m_pPl->IsOffside())
	{
		if (m_bIsAdvantage)
		{
			if (m_bIsPenalty)
				State_Transition(BALL_STATE_PENALTY, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());
			else
				State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());
		}
		else
		{
			m_pPl->AddOffside();
			SetFoulParams(FOUL_OFFSIDE, m_pPl->GetOffsidePos(), m_pPl);
			SDKGameRules()->SetOffsideLinePositions(m_pPl->GetOffsideBallPos().y, m_pPl->GetOffsidePos().y, m_pPl->GetOffsideLastOppPlayerPos().y);
			State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_long.GetFloat());
		}
	}
}

bool CMatchBall::CheckFoul(CSDKPlayer *pPl)
{
	if (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState)
		return false;

	if (gpGlobals->curtime < pPl->m_flNextFoulCheck)
		return false;

	Vector dirToBall = GetPos() - pPl->GetTeamNumber();

	if (dirToBall.Length2D() > sv_ball_foulcheckmaxdist.GetInt())
		return false;

	float zDist = dirToBall.z;
	Vector localDirToBall;
	VectorIRotate(dirToBall, pPl->EntityToWorldTransform(), localDirToBall);

	bool canShootBall = zDist < sv_ball_slidezend.GetFloat()
		&& zDist >= sv_ball_slidezstart.GetFloat()
		&& localDirToBall.x >= -sv_ball_slidebackwardreach_ball.GetFloat()
		&& localDirToBall.x <= sv_ball_slideforwardreach_ball.GetFloat()
		&& abs(localDirToBall.y) <= sv_ball_slidesidereach_ball.GetFloat();

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pOtherPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pOtherPl))
			continue;

		if (pOtherPl == pPl || pOtherPl->GetTeamNumber() == pPl->GetTeamNumber())
			continue;

		Vector dirToOtherPl = pOtherPl->GetLocalOrigin() - pPl->GetLocalOrigin();

		Vector localDirToOtherPl;
		VectorIRotate(dirToOtherPl, pPl->EntityToWorldTransform(), localDirToOtherPl);

		dirToOtherPl.z = 0;
		dirToOtherPl.NormalizeInPlace();

		// Can't reach the other player
		if (localDirToOtherPl.x < -sv_ball_slidebackwardreach_foul.GetInt()
			|| localDirToOtherPl.x > sv_ball_slideforwardreach_foul.GetInt()
			|| abs(localDirToOtherPl.y) > sv_ball_slidesidereach_foul.GetInt())
			continue;

		// Can shoot the ball and ball is closer to the player than the opponent
		if (canShootBall && localDirToBall.x <= localDirToOtherPl.x)
			continue;

		// It's a foul

		PlayerAnimEvent_t anim = RAD2DEG(acos(pPl->EyeDirection2D().Dot(pOtherPl->EyeDirection2D()))) <= 90 ? PLAYERANIMEVENT_TACKLED_BACKWARD : PLAYERANIMEVENT_TACKLED_FORWARD;
		pOtherPl->DoServerAnimationEvent(anim);

		pPl->m_flNextFoulCheck = gpGlobals->curtime + sv_ball_foulcheckdelay.GetFloat();

		int teammatesCloserToGoalCount = 0;

		bool isCloseToOwnGoal = ((GetPos() - pPl->GetTeam()->m_vGoalCenter).Length2D() <= sv_ball_closetogoaldist.GetInt());

		if (isCloseToOwnGoal)
		{
			for (int j = 1; j <= gpGlobals->maxClients; j++) 
			{
				CSDKPlayer *pOtherPl = ToSDKPlayer(UTIL_PlayerByIndex(j));

				if (!CSDKPlayer::IsOnField(pOtherPl) || pOtherPl == pPl || pOtherPl->GetTeamNumber() != pPl->GetTeamNumber())
					continue;

				if ((pPl->GetTeam()->m_vGoalCenter - pOtherPl->GetLocalOrigin()).Length2DSqr() < (pPl->GetTeam()->m_vGoalCenter - pPl->GetLocalOrigin()).Length2DSqr())
					teammatesCloserToGoalCount += 1;
			}
		}

		bool isPenalty = pOtherPl->m_Shared.m_nInPenBoxOfTeam == pPl->GetTeamNumber();

		foul_type_t foulType;

		if (isCloseToOwnGoal && teammatesCloserToGoalCount <= 1)
			foulType = FOUL_NORMAL_RED_CARD;
		else if (anim == PLAYERANIMEVENT_TACKLED_FORWARD && localDirToBall.Length2DSqr() >= Sqr(sv_ball_yellowcardballdist_forward.GetFloat()) ||
				 anim == PLAYERANIMEVENT_TACKLED_BACKWARD && localDirToBall.Length2DSqr() >= Sqr(sv_ball_yellowcardballdist_backward.GetFloat()))
			foulType = pPl->GetYellowCards() % 2 == 0 ? FOUL_NORMAL_YELLOW_CARD : FOUL_NORMAL_SECOND_YELLOW_CARD;
		else
			foulType = FOUL_NORMAL_NO_CARD;

		if (!m_bIsAdvantage || !m_bIsPenalty || isPenalty)
			SetFoulParams(foulType, pOtherPl->GetLocalOrigin(), pPl, pOtherPl);

		pPl->AddFoul();
		pOtherPl->AddFoulSuffered();

		if (foulType == FOUL_NORMAL_YELLOW_CARD || foulType == FOUL_NORMAL_SECOND_YELLOW_CARD)
			pPl->AddYellowCard();

		if (foulType == FOUL_NORMAL_SECOND_YELLOW_CARD || foulType == FOUL_NORMAL_RED_CARD)
		{
			pPl->AddRedCard();
			int banDuration = 60 * (foulType == FOUL_NORMAL_SECOND_YELLOW_CARD ? sv_ball_player_secondyellowcard_banduration.GetFloat() : sv_ball_player_redcard_banduration.GetFloat());
			int nextJoin = SDKGameRules()->GetMatchDisplayTimeSeconds(false) + banDuration;
			pPl->SetNextCardJoin(nextJoin);

			ReplayManager()->AddMatchEvent(foulType == FOUL_NORMAL_SECOND_YELLOW_CARD ? MATCH_EVENT_SECONDYELLOWCARD : MATCH_EVENT_REDCARD, pPl->GetTeamNumber(), pPl);

			int team = pPl->GetTeamNumber();
			int posIndex = pPl->GetTeamPosIndex();
			int posType = pPl->GetTeamPosType();

			pPl->SetDesiredTeam(TEAM_SPECTATOR, team, 0, true, false, false);

			if (posType != POS_GK)
			{
				// Block carded player's pos
				GetGlobalTeam(team)->SetPosNextJoinSeconds(posIndex, nextJoin);
			}
			else
			{
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					CSDKPlayer *pOtherPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
					if (!CSDKPlayer::IsOnField(pOtherPl) || pOtherPl == pPl || pOtherPl->GetTeamNumber() != team)
						continue;

					// Switch another player to the keeper pos and block this player's pos instead
					pOtherPl->GetTeam()->SetPosNextJoinSeconds(pOtherPl->GetTeamPosIndex(), nextJoin);
					pOtherPl->SetDesiredTeam(team, team, posIndex, true, false, false);
					break;
				}
			}
		}
		else if (foulType == FOUL_NORMAL_YELLOW_CARD)
		{
			ReplayManager()->AddMatchEvent(MATCH_EVENT_YELLOWCARD, pPl->GetTeamNumber(), pPl);
		}

		// Don't overwrite a pending penalty with a free-kick
		if (m_bIsAdvantage && m_bIsPenalty && !isPenalty)
		{
			State_Transition(BALL_STATE_PENALTY, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());
		}
		else
		{
			IGameEvent *pEvent = gameeventmanager->CreateEvent("foul");
			if (pEvent)
			{
				match_event_t foulMatchEvent;

				switch (m_eFoulType)
				{
				case FOUL_NORMAL_NO_CARD:
					foulMatchEvent = MATCH_EVENT_FOUL;
					break;
				case FOUL_NORMAL_YELLOW_CARD:
					foulMatchEvent = MATCH_EVENT_YELLOWCARD;
					break;
				case FOUL_NORMAL_SECOND_YELLOW_CARD:
					foulMatchEvent = MATCH_EVENT_SECONDYELLOWCARD;
					break;
				case FOUL_NORMAL_RED_CARD:
					foulMatchEvent = MATCH_EVENT_REDCARD;
					break;
				default:
					foulMatchEvent = MATCH_EVENT_FOUL;
					break;
				}

				pEvent->SetInt("fouling_player_userid", m_pFoulingPl->GetUserID());
				pEvent->SetInt("fouled_player_userid", m_pFouledPl->GetUserID());
				pEvent->SetInt("fouling_team", m_nFoulingTeam);
				pEvent->SetInt("foul_type", foulMatchEvent);
				pEvent->SetInt("set_piece_type", MATCH_EVENT_ADVANTAGE);
				pEvent->SetInt("statistic_type", STATISTIC_DEFAULT);
				gameeventmanager->FireEvent(pEvent);
			}

			m_bIsAdvantage = true;
			m_flFoulTime = gpGlobals->curtime;
			m_bIsPenalty = isPenalty;
		}

		return true;
	}

	return false;
}

void CMatchBall::SetFoulParams(foul_type_t type, Vector pos, CSDKPlayer *pFoulingPl, CSDKPlayer *pFouledPl /*= NULL*/)
{
	m_eFoulType = type;
	m_pFoulingPl = pFoulingPl;
	m_pFouledPl = pFouledPl;
	m_nFoulingTeam = pFoulingPl->GetTeamNumber();
	m_nFouledTeam = pFoulingPl->GetOppTeamNumber();

	float foulPosFieldOffset = SDKGameRules()->HasWalledField() ? 100 : 15;

	m_vFoulPos.x = clamp(pos.x, SDKGameRules()->m_vFieldMin.GetX() + foulPosFieldOffset, SDKGameRules()->m_vFieldMax.GetX() - foulPosFieldOffset);
	m_vFoulPos.y = clamp(pos.y, SDKGameRules()->m_vFieldMin.GetY() + foulPosFieldOffset, SDKGameRules()->m_vFieldMax.GetY() - foulPosFieldOffset);
	m_vFoulPos.z = SDKGameRules()->m_vKickOff.GetZ();

	// Move the ball to the edge of the penalty box if the foul happened inside. This will probably only be relevant for double touch fouls.

	Vector min = GetGlobalTeam(m_nFoulingTeam)->m_vPenBoxMin - BALL_PHYS_RADIUS;
	Vector max = GetGlobalTeam(m_nFoulingTeam)->m_vPenBoxMax + BALL_PHYS_RADIUS;

	// Ball inside the penalty box
	if (m_vFoulPos.x > min.x && m_vFoulPos.x < max.x)
	{
		if (GetGlobalTeam(m_nFoulingTeam)->m_nForward == 1 && m_vFoulPos.y < max.y)
			m_vFoulPos.y = max.y;
		else if (GetGlobalTeam(m_nFoulingTeam)->m_nForward == -1 && m_vFoulPos.y > min.y)
			m_vFoulPos.y = min.y;
	}
}

bool CMatchBall::IsPlayerClose()
{
	return (m_vPos - m_vPlPos).Length2DSqr() <= pow(sv_ball_setpiece_close_dist.GetFloat(), 2);
}

void CMatchBall::SetVel(Vector vel, float spinCoeff, int spinFlags, body_part_t bodyPart, bool markOffsidePlayers, float shotTakerMinDelay, bool resetShotCharging)
{
	CBall::SetVel(vel, spinCoeff, spinFlags, bodyPart, markOffsidePlayers, shotTakerMinDelay, resetShotCharging);

	if (markOffsidePlayers)
		MarkOffsidePlayers();
}

extern ConVar sv_singlekeeper;

void CMatchBall::MarkOffsidePlayers()
{
	if (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState || SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		return;

	m_vOffsidePos = m_vPos;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (pPl)
			pPl->SetOffside(false);

		if (!CSDKPlayer::IsOnField(pPl) || pPl == m_pPl || pPl->GetTeamNumber() != m_pPl->GetTeamNumber())
			continue;

		Vector pos = pPl->GetLocalOrigin();
		int forward = pPl->GetTeam()->m_nForward;

		// In opponent half?
		if (Sign((pos - SDKGameRules()->m_vKickOff).y) != forward)
			continue;

		// Player closer to goal than the ball?
		if (Sign(pos.y - m_vPos.y) != forward)
			continue;

		int oppPlayerCount = 0;
		int nearerPlayerCount = 0;
		CSDKPlayer *pLastPl = NULL;
		float shortestDist = FLT_MAX;

		// Count opponent players who are nearer to the goal
		for (int j = 1; j <= gpGlobals->maxClients; j++)
		{
			CSDKPlayer *pOpp = ToSDKPlayer(UTIL_PlayerByIndex(j));
			if (!CSDKPlayer::IsOnField(pOpp) || pOpp->GetTeamNumber() == pPl->GetTeamNumber())
				continue;

			oppPlayerCount += 1;

			if (Sign(pOpp->GetLocalOrigin().y - pos.y) == forward)
			{
				nearerPlayerCount += 1;
			}
			else
			{
				float dist = abs(pos.y - pOpp->GetLocalOrigin().y);
				if (dist < shortestDist)
				{
					shortestDist = dist;
					pLastPl = pOpp;
				}
			}
		}

		// If the defending team doesn't have a keeper, just assume there is one at his goal line and increase the counter by one.
		// This is mainly relevant for matches with sv_singlekeeper enabled when the keeper spot of the defending team is empty when the attackers play a pass.
		// An attacker can be caught offside then, even if a defender is closer to the goal than he is.
		// It also helps preventing abuse by keepers who go to spec during an attack to force false offsides.
		if (!m_pPl->GetOppTeam()->GetPlayerByPosType(POS_GK))
		{
			nearerPlayerCount += 1;
		}

		// Require at least two opponent players on the field to consider an offside. Useful in public matches with few players.
		if (oppPlayerCount >= 2 && nearerPlayerCount <= 1)
		{
			pPl->SetOffside(true);
			pPl->SetOffsidePos(pPl->GetLocalOrigin());
			pPl->SetOffsideBallPos(m_vPos);

			Vector lastSafePos;

			if (abs(pLastPl->GetLocalOrigin().y - pPl->GetLocalOrigin().y) < abs(SDKGameRules()->m_vKickOff.GetY() - pPl->GetLocalOrigin().y))
				lastSafePos = pLastPl->GetLocalOrigin();
			else
				lastSafePos = SDKGameRules()->m_vKickOff;

			pPl->SetOffsideLastOppPlayerPos(lastSafePos);
		}
	}
}

void CMatchBall::UnmarkOffsidePlayers()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (CSDKPlayer::IsOnField(pPl))
			pPl->SetOffside(false);
	}
}

void CMatchBall::UpdatePossession(CSDKPlayer *pNewPossessor)
{
	if (m_pPossessingPl == pNewPossessor)
		return;

	if (m_flPossessionStart != -1)
	{
		float duration = gpGlobals->curtime - m_flPossessionStart;

		if (CSDKPlayer::IsOnField(m_pPossessingPl))
			m_pPossessingPl->AddPossessionTime(duration);

		float total = GetGlobalTeam(TEAM_HOME)->m_flPossessionTime + GetGlobalTeam(TEAM_AWAY)->m_flPossessionTime;

		GetGlobalTeam(TEAM_HOME)->m_Possession = floor(GetGlobalTeam(TEAM_HOME)->m_flPossessionTime * 100 / max(1, total) + 0.5f);
		GetGlobalTeam(TEAM_AWAY)->m_Possession = 100 - GetGlobalTeam(TEAM_HOME)->m_Possession;

		int possSum = 0;

		struct remainder_t
		{
			int dataIndex;
			float remainder;
			remainder_t() : dataIndex(0), remainder(0) {}
		};

		remainder_t *sortedRemainders = new remainder_t[CPlayerPersistentData::m_PlayerPersistentData.Count()];

		for (int i = 0; i < CPlayerPersistentData::m_PlayerPersistentData.Count(); i++)
		{
			CPlayerMatchData *pData = CPlayerPersistentData::m_PlayerPersistentData[i]->m_pMatchData;
			
			float exactPossession = pData->m_flPossessionTime * 100 / max(1, total);
			pData->m_nPossession = (int)exactPossession;
			float remainder = exactPossession - pData->m_nPossession;

			possSum += pData->m_nPossession;

			for (int j = 0; j <= i; j++)
			{
				if (sortedRemainders[j].remainder <= remainder)
				{
					for (int k = i; k > j; k--)
					{
						sortedRemainders[k] = sortedRemainders[k - 1];
					}

					sortedRemainders[j].dataIndex = i;
					sortedRemainders[j].remainder = remainder;

					break;
				}
			}
		}

		for (int i = 0; i < CPlayerPersistentData::m_PlayerPersistentData.Count(); i++)
		{
			if (possSum == 100)
				break;

			CPlayerMatchData *pData = CPlayerPersistentData::m_PlayerPersistentData[sortedRemainders[i].dataIndex]->m_pMatchData;
			pData->m_nPossession += 1;
			possSum += 1;
		}

		delete[] sortedRemainders;
	}

	if (CSDKPlayer::IsOnField(pNewPossessor))
	{
		m_pPossessingPl = pNewPossessor;
		m_nPossessingTeam = pNewPossessor->GetTeamNumber();
		m_flPossessionStart = gpGlobals->curtime;
	}
	else
	{
		m_pPossessingPl = NULL;
		m_nPossessingTeam = TEAM_NONE;
		m_flPossessionStart = -1;
	}
}

// -100 = ball at the goal line of the home team
// 0 = ball at the the half-way line
// 100 = ball at the goal line of the away team
float CMatchBall::GetFieldZone()
{
	return m_flFieldZone;
}

void CMatchBall::SetPenaltyTaker(CSDKPlayer *pPl)
{
	m_pFouledPl = pPl;
	m_nFouledTeam = pPl->GetTeamNumber();
	m_nFoulingTeam = pPl->GetOppTeamNumber();
	m_pFoulingPl = GetGlobalTeam(m_nFoulingTeam)->GetPlayerByPosType(POS_GK);
}

void CMatchBall::VPhysicsCollision(int index, gamevcollisionevent_t *pEvent)
{
	float speed = pEvent->collisionSpeed;
	int surfaceProps = pEvent->surfaceProps[!index];

	if (surfaceProps == POST_SURFACEPROPS && speed > 300.0f)
	{
		CSDKPlayer *pLastPl = LastPl(true);
		if (pLastPl && Sign(m_vPos.y - SDKGameRules()->m_vKickOff.GetY()) == pLastPl->GetTeam()->m_nForward) // Check if it's the opponent's goal
			m_bHitThePost = true;
	}

	m_bBallInAirAfterThrowIn = false;

	CBall::VPhysicsCollision(index, pEvent);
}

bool CMatchBall::IsLegallyCatchableByKeeper()
{
	if (SDKGameRules()->IsIntermissionState() || SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		return true;

	// Skip the keeper
	BallTouchInfo *pLastTouch = LastInfo(false, m_pPl);
	BallTouchInfo *pLastShot = LastInfo(true, m_pPl);

	if (pLastTouch && pLastShot)
	{
		// Can catch if opponent has touched or shot the ball
		if (pLastTouch->m_nTeam != m_pPl->GetTeamNumber() || pLastShot->m_nTeam != m_pPl->GetTeamNumber())
			return true;

		if (pLastShot->m_eBodyPart == BODY_PART_HEAD || pLastShot->m_eBodyPart == BODY_PART_CHEST)
		{
			// Only allow the keeper to pick up the ball if the shot or touch before the last header or chest action was by an opponent
			for (int i = m_Touches.Count() - 1; i >= 1; i--)
			{
				if (m_Touches[i]->m_bIsShot && m_Touches[i]->m_pPl != m_pPl)
				{
					for (int j = i - 1; j >= 0; j--)
					{
						if (m_Touches[j]->m_bIsShot && m_Touches[j]->m_pPl != m_pPl)
						{
							if (m_Touches[j]->m_nTeam != pLastTouch->m_nTeam)
								return true;

							break;
						}
					}

					break;
				}
			}
		}
	}

	return false;
}

void CMatchBall::CheckAdvantage()
{
	// Check if we're beyond the initial ignore period after a foul during which no checks should be performed until ball possession is settled
	if (m_bIsAdvantage && !SDKGameRules()->IsIntermissionState() && !m_bHasQueuedState
		&& (m_bIsPenalty && gpGlobals->curtime > m_flFoulTime + sv_ball_advantage_ignore_duration_penalty.GetFloat()
			|| !m_bIsPenalty && gpGlobals->curtime > m_flFoulTime + sv_ball_advantage_ignore_duration_freekick.GetFloat()))
	{
		// Check if the advantage period is over
		if (m_bIsPenalty && gpGlobals->curtime > m_flFoulTime + sv_ball_advantage_duration_penalty.GetFloat()
			|| !m_bIsPenalty && gpGlobals->curtime > m_flFoulTime + sv_ball_advantage_duration_freekick.GetFloat())
		{
			if (m_bIsPenalty)
			{
				// Always give a penalty if the advantage time is over and no goal was scored by the fouled team
				State_Transition(BALL_STATE_PENALTY, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_short.GetFloat());
			}
			else
			{
				//todo add check if towards goal if goal scoring
				CSDKPlayer *pPl = FindNearestPlayer();
				BallTouchInfo *pLastInfo = LastInfo(true);

				if (pPl && pPl->GetTeamNumber() == m_nFoulingTeam // Fouling team player is closest
					|| pLastInfo && pLastInfo->m_nTeam == m_nFoulingTeam // Fouling team shot last
					|| pLastInfo && pLastInfo->m_flTime <= m_flFoulTime + sv_ball_advantage_ignore_duration_freekick.GetFloat()) // Last shot happened shortly after the foul was committed
				{
					State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_long.GetFloat());
				}
			}

			m_bIsAdvantage = false;
		}
		// After ignore period, but before end of advantage
		else
		{
			if (!m_bIsPenalty)
			{
				if (m_vVel.Length2D() < 500)
				{
					CSDKPlayer *pPl = FindNearestPlayer();

					if (pPl && pPl->GetTeamNumber() == m_nFoulingTeam)
						State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_messagedelay_normal.GetFloat(), sv_ball_statetransition_postmessagedelay_long.GetFloat());
				}
				else if (LastPl(true) && LastPl(true)->GetTeamNumber() == m_nFouledTeam && m_nInPenBoxOfTeam == m_nFoulingTeam)
				{
					int xPos, team;
					GetPredictedGoalLineCrossPosX(xPos, team);

					if (team == LastPl(true)->GetOppTeamNumber()
						&& xPos >= GetGlobalTeam(m_nFoulingTeam)->m_vPenBoxMin.GetX()
						&& xPos <= GetGlobalTeam(m_nFoulingTeam)->m_vPenBoxMax.GetX())
					{
						m_bIsAdvantage = false;
					}
				}
			}
		}
	}
}

void CMatchBall::SetPenaltyState(penalty_state_t penaltyState)
{
	m_ePenaltyState = penaltyState;

	if (m_ePenaltyState != PENALTY_ASSIGNED && m_ePenaltyState != PENALTY_SCORED)
		return;

	IGameEvent *pEvent = gameeventmanager->CreateEvent("penalty_shootout");
	if (pEvent)
	{
		pEvent->SetInt("taking_team", m_nFouledTeam);
		pEvent->SetInt("taker_userid", m_pFouledPl ? m_pFouledPl->GetUserID() : 0);
		pEvent->SetInt("keeper_userid", m_pOtherPl ? m_pOtherPl->GetUserID() : (m_pFoulingPl ? m_pFoulingPl->GetUserID() : 0));
		pEvent->SetInt("penalty_state", m_ePenaltyState);
		gameeventmanager->FireEvent(pEvent);
	}
}

void CMatchBall::SetSetpieceTaker(CSDKPlayer *pPlayer)
{
	m_pSetpieceTaker = pPlayer;

	if (CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl->SetShotsBlocked(false);
		m_pPl->SetChargedshotBlocked(false);
		m_pPl->RemoveFlags();
	}

	m_pPl = NULL;
}