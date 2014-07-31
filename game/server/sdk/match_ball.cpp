#include "cbase.h"
#include "match_ball.h"

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

extern ConVar sv_ball_statetransition_messagedelay_short;
extern ConVar sv_ball_statetransition_messagedelay_normal;

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

void CMatchBall::Reset()
{
	m_pLastActivePlayer = NULL;

	CBall::Reset();
}