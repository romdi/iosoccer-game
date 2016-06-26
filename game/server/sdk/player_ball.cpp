#include "cbase.h"
#include "player_ball.h"
#include "match_ball.h"
#include "sdk_player.h"
#include "team.h"

extern ConVar
	sv_ball_bodypos_keeperhands,
	sv_ball_chargedshot_minstrength,
	sv_ball_chargedshot_maxstrength,
	sv_ball_keepershot_minangle,
	sv_ball_maxplayerfinddist,
	sv_ball_keeperthrow_strength,
	sv_ball_keeperthrow_minpostdelay;

ConVar sv_ball_update_physics("sv_ball_update_physics", "0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);


LINK_ENTITY_TO_CLASS( player_ball, CPlayerBall );

BEGIN_DATADESC(	CPlayerBall )
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPlayerBall, DT_PlayerBall )
	SendPropEHandle(SENDINFO(m_pCreator)),
END_SEND_TABLE()

CPlayerBall *CreatePlayerBall(const Vector &pos, CSDKPlayer *pCreator)
{
	CPlayerBall *pBall = static_cast<CPlayerBall *>(CreateEntityByName("player_ball"));
	pBall->SetCreator(pCreator);
	pBall->SetAbsOrigin(pos);
	pBall->Spawn();
	pBall->SetPos(pos);

	return pBall;
}

void CC_CreatePlayerBall(const CCommand &args)
{
	if (!SDKGameRules()->IsIntermissionState() || SDKGameRules()->IsCeremony())
		return;

	CSDKPlayer *pPl = ToSDKPlayer(UTIL_GetCommandClient());
	if (!CSDKPlayer::IsOnField(pPl))
		return;

	if (pPl->GetFlags() & FL_REMOTECONTROLLED)
		return;

	trace_t tr;

	CTraceFilterSkipTwoEntities traceFilter(pPl, pPl->GetPlayerBall(), COLLISION_GROUP_NONE);

	UTIL_TraceHull(
		pPl->GetLocalOrigin() + VEC_VIEW,
		pPl->GetLocalOrigin() + VEC_VIEW + pPl->EyeDirection3D() * 150,
		-Vector(BALL_PHYS_RADIUS, BALL_PHYS_RADIUS, BALL_PHYS_RADIUS),
		Vector(BALL_PHYS_RADIUS, BALL_PHYS_RADIUS, BALL_PHYS_RADIUS),
		MASK_SOLID,
		&traceFilter,
		&tr);

	Vector pos = tr.endpos;

	if (pPl->GetPlayerBall())
	{
		if (pPl->GetPlayerBall()->GetHoldingPlayer())
		{
			//pPl->GetPlayerBall()->RemoveFromPlayerHands(pPl->GetPlayerBall()->GetHoldingPlayer());
			pPl->GetPlayerBall()->State_Transition(BALL_STATE_NORMAL);
		}

		if (sv_ball_update_physics.GetBool())
			pPl->GetPlayerBall()->CreateVPhysics();

		pPl->GetPlayerBall()->SetPos(pos, false);
		pPl->GetPlayerBall()->RemoveAllTouches();
	}
	else
		pPl->SetPlayerBall(CreatePlayerBall(pos, pPl));

	//pPl->GetPlayerBall()->m_nSkin = pPl->GetPlayerBallSkin() == -1 ? g_IOSRand.RandomInt(0, BALL_SKIN_COUNT - 1) : pPl->GetPlayerBallSkin();
	pPl->GetPlayerBall()->SetSkinName(pPl->GetPlayerBallSkinName());
	pPl->GetPlayerBall()->SetBallCannonMode(false);
	pPl->GetPlayerBall()->SaveBallCannonSettings();
	pPl->m_Shared.SetStamina(100);
}

static ConCommand createplayerball("createplayerball", CC_CreatePlayerBall);

void CC_ShootPlayerBall(const CCommand &args)
{
	CSDKPlayer *pPl = ToSDKPlayer(UTIL_GetCommandClient());
	if (!CSDKPlayer::IsOnField(pPl) || !pPl->GetPlayerBall())
		return;

	pPl->GetPlayerBall()->SetBallCannonMode(true);
	pPl->GetPlayerBall()->RestoreBallCannonSettings();
	pPl->m_Shared.SetStamina(100);
}

static ConCommand shootplayerball("shootplayerball", CC_ShootPlayerBall);

CBall *GetNearestPlayerBall(const Vector &pos)
{
	CBall *pNearestBall = GetMatchBall();
	Vector ballPos = pNearestBall->GetPos();
	float shortestDist = (ballPos - pos).Length2DSqr();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl || !CSDKPlayer::IsOnField(pPl))
			continue;

		CBall *pBall = pPl->GetPlayerBall();
		if (!pBall)
			continue;

		Vector ballPos = pBall->GetPos();

		float dist = (ballPos - pos).Length2DSqr();
		if (dist < shortestDist)
		{
			shortestDist = dist;
			pNearestBall = pBall;
		}
	}

	return pNearestBall;
}

CPlayerBall::CPlayerBall()
{
	m_pCreator = NULL;
	m_pLastShooter = NULL;
	m_vLastShotPos = vec3_invalid;
}

CPlayerBall::~CPlayerBall()
{
	if (GetHoldingPlayer())
		RemoveFromPlayerHands(GetHoldingPlayer());

	if (GetCreator())
		GetCreator()->SetPlayerBall(NULL);
}

void CPlayerBall::State_Transition(ball_state_t nextState, float nextStateMessageDelay /*= 0*/, float nextStatePostMessageDelay /*= 0*/, bool cancelQueuedState /*= false*/)
{
	State_Leave(nextState);
	State_Enter(nextState, cancelQueuedState);
}

void CPlayerBall::State_Enter(ball_state_t newState, bool cancelQueuedState)
{
	m_eBallState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );
	m_flStateEnterTime = gpGlobals->curtime;

	m_pPl = NULL;

	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		(this->*m_pCurStateInfo->pfnEnterState)();
	}
}

void CPlayerBall::State_Leave(ball_state_t newState)
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)(newState);
	}
}

void CPlayerBall::State_Think()
{
	m_pPhys->GetPosition(&m_vPos, &m_aAng);
	m_pPhys->GetVelocity(&m_vVel, &m_vRot);

	CheckPenBoxPosition();

	if (m_pCurStateInfo && m_pCurStateInfo->pfnThink)
	{	
		(this->*m_pCurStateInfo->pfnThink)();
	}
}

void CPlayerBall::SetVel(Vector vel, float spinCoeff, int spinFlags, body_part_t bodyPart, bool markOffsidePlayers, float minPostDelay, bool resetShotCharging)
{
	CBall::SetVel(vel, spinCoeff, spinFlags, bodyPart, markOffsidePlayers, minPostDelay, resetShotCharging);

	if (!m_bIsBallCannonMode && m_pPl == GetCreator())
		SaveBallCannonSettings();
}

void CPlayerBall::SaveBallCannonSettings()
{
	m_BallCannonSettings.pos = GetPos();
	m_BallCannonSettings.ang = GetAng();
	m_BallCannonSettings.vel = GetVel();
	m_BallCannonSettings.rot = GetRot();
	m_BallCannonSettings.globalDynamicShotDelay = m_flGlobalDynamicShotDelay;
}

void CPlayerBall::RestoreBallCannonSettings()
{
	State_Transition(BALL_STATE_NORMAL);
	m_flGlobalLastShot = gpGlobals->curtime;
	m_flGlobalDynamicShotDelay = m_BallCannonSettings.globalDynamicShotDelay;
	m_pPhys->SetVelocityInstantaneous(&vec3_origin, &vec3_origin);
	m_pPhys->SetPosition(m_BallCannonSettings.pos, m_BallCannonSettings.ang, true);
	m_pPhys->SetVelocity(&m_BallCannonSettings.vel, &m_BallCannonSettings.rot);
}

void CPlayerBall::SetBallCannonMode(bool isBallCannonMode)
{
	m_bIsBallCannonMode = isBallCannonMode;
}

void CPlayerBall::RemoveAllPlayerBalls()
{
	CPlayerBall *pBall = NULL;

	while (true)
	{
		pBall = static_cast<CPlayerBall *>(gEntList.FindEntityByClassname(pBall, "player_ball"));
		if (!pBall)
			break;

		pBall->RemovePlayerBall();
	}
}

void CPlayerBall::RemovePlayerBall()
{
	UTIL_Remove(this);
}

void CPlayerBall::State_NORMAL_Enter()
{
	m_pPhys->EnableMotion(true);
	EnablePlayerCollisions(true);
	m_pPhys->Wake();
}

void CPlayerBall::State_NORMAL_Think()
{
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

void CPlayerBall::State_NORMAL_Leave(ball_state_t newState)
{
}

void CPlayerBall::State_KEEPERHANDS_Enter()
{
	SetPos(m_vPos, false);
}

void CPlayerBall::State_KEEPERHANDS_Think()
{
	int wasOrIsinPenBoxOfTeam = m_nInPenBoxOfTeam != TEAM_NONE ? m_nInPenBoxOfTeam : m_nWasInPenBoxOfTeam;

	if (!CSDKPlayer::IsOnField(m_pPl, wasOrIsinPenBoxOfTeam))
	{
		m_pPl = GetGlobalTeam(wasOrIsinPenBoxOfTeam)->GetPlayerByPosType(POS_GK);

		if (!m_pPl)
			return State_Transition(BALL_STATE_NORMAL);

		m_pPl->SetShotButtonsReleased(false);
		AddToPlayerHands(m_pPl);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HOLD);
	}

	UpdateCarrier();

	// Ball outside the penalty box
	if (m_nInPenBoxOfTeam == TEAM_NONE)
	{
		Vector vel, pos;

		bool isBehindGoalLine = m_pPl->GetTeam()->m_nForward == 1 && m_vPos.y + BALL_PHYS_RADIUS < m_pPl->GetTeam()->m_vPenBoxMin.GetY() || m_pPl->GetTeam()->m_nForward == -1 && m_vPos.y - BALL_PHYS_RADIUS > m_pPl->GetTeam()->m_vPenBoxMax.GetY();
		bool isInsideGoal = m_vPos.x + BALL_PHYS_RADIUS >= m_pPl->GetTeam()->m_vGoalCenter.GetX() - SDKGameRules()->m_vGoalTriggerSize.x && m_vPos.x - BALL_PHYS_RADIUS <= m_pPl->GetTeam()->m_vGoalCenter.GetX() + SDKGameRules()->m_vGoalTriggerSize.x;

		float zPos = max(m_vPos.z, SDKGameRules()->m_vKickOff.GetZ() + BALL_PHYS_RADIUS);

		// Throw the ball towards the kick-off spot if it's behind the goal line and either would end up inside the goal or is in a map with a walled field
		if (isBehindGoalLine && (isInsideGoal || SDKGameRules()->HasWalledField()))
		{
			float yPos = (m_pPl->GetTeam()->m_nForward == 1 ? SDKGameRules()->m_vFieldMin.GetY() - BALL_PHYS_RADIUS : SDKGameRules()->m_vFieldMax.GetY() + BALL_PHYS_RADIUS) + m_pPl->GetTeam()->m_nForward * 36;
			pos = Vector(m_vPos.x, yPos, zPos);
			vel = 25 * Vector(0, m_pPl->GetTeam()->m_nForward, 0);
		}
		else
		{
			pos = Vector(m_vPos.x, m_vPos.y, zPos);
			vel = m_vPlVel2D;
		}

		SetPos(pos, false);
		SetVel(vel, 0, FL_SPIN_FORCE_NONE, BODY_PART_KEEPERHANDS, true, sv_ball_keeperthrow_minpostdelay.GetFloat(), true);

		return State_Transition(BALL_STATE_NORMAL);
	}

	Vector handPos;
	QAngle handAng;
	m_pPl->GetAttachment("ball_right_hand", handPos, handAng);
	SetPos(handPos, false);
	SetAng(handAng);

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsShooting() && m_pPl->CanShoot())
	{
		Vector vel;
		int spinFlags;
		PlayerAnimEvent_t animEvent;

		if (m_pPl->IsNormalshooting())
		{
			Vector dir;
			AngleVectors(m_aPlAng, &dir);
			vel = m_vPlForwardVel2D + dir * GetNormalshotStrength(GetPitchCoeff(), sv_ball_keeperthrow_strength.GetInt());
			spinFlags = FL_SPIN_FORCE_NONE;
			animEvent = PLAYERANIMEVENT_KEEPER_HANDS_THROW;
		}
		else
		{
			QAngle ang = m_aPlAng;
			ang[PITCH] = min(sv_ball_keepershot_minangle.GetFloat(), m_aPlAng[PITCH]);
			Vector dir;
			AngleVectors(ang, &dir);
			vel = dir * GetChargedshotStrength(GetPitchCoeff(), sv_ball_chargedshot_minstrength.GetInt(), sv_ball_chargedshot_maxstrength.GetInt());
			spinFlags = FL_SPIN_PERMIT_SIDE;
			animEvent = PLAYERANIMEVENT_KEEPER_HANDS_VOLLEY;

			if (vel.Length() > 1000)
				EmitSound("Ball.Kickhard");
			else
				EmitSound("Ball.Kicknormal");
		}

		m_pPl->DoServerAnimationEvent(animEvent);
		RemoveAllTouches();
		SetPos(Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_keeperhands.GetFloat()) + m_vPlForward2D * 36, false);
		SetVel(vel, 1.0f, spinFlags, BODY_PART_KEEPERHANDS, true, sv_ball_keeperthrow_minpostdelay.GetFloat(), true);

		return State_Transition(BALL_STATE_NORMAL);
	}
}

void CPlayerBall::State_KEEPERHANDS_Leave(ball_state_t newState)
{
	RemoveFromPlayerHands(m_pPl);
}

void CPlayerBall::Touched(bool isShot, body_part_t bodyPart, const Vector &oldVel)
{
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
}

void CPlayerBall::VPhysicsCollision(int index, gamevcollisionevent_t *pEvent)
{
	CBall::VPhysicsCollision(index, pEvent);
}

bool CPlayerBall::IsLegallyCatchableByKeeper()
{
	return true;
}