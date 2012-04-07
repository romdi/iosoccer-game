//============================================================================//
//
// Ball.cpp	by Mark	Gornall, Jan 2007.
//
//=============================================================================//

#include "cbase.h"
#include "baseanimating.h"
#include "player_pickup.h"
#include "props_shared.h"
#include "props.h"
#include "sdk_player.h"
#include "sdk_gamerules.h"
#include "in_buttons.h"
#include "nav_mesh.h"
#include "game.h"
#include "ball.h"
#include "team.h"
#include "triggers.h"
#include "ios_mapentities.h"
#include "sdk_shareddefs.h"
#include "ios_replaymanager.h"

ConVar sv_ball_mass( "sv_ball_mass", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY );
//ConVar sv_ball_inertia( "sv_ball_inertia", "1.0", FCVAR_ARCHIVE | FCVAR_NOTIFY  );
//ConVar sv_ball_rotinertialimit( "sv_ball_rotinertialimit", "0.05", FCVAR_ARCHIVE | FCVAR_NOTIFY  );
ConVar sv_ball_damping( "sv_ball_damping", "0.3", FCVAR_ARCHIVE | FCVAR_NOTIFY  );
ConVar sv_ball_rotdamping( "sv_ball_rotdamping", "0.3", FCVAR_ARCHIVE | FCVAR_NOTIFY  );
//ConVar sv_ball_dragcoefficient( "sv_ball_dragcoefficient", "0.47", FCVAR_ARCHIVE | FCVAR_NOTIFY  );
//ConVar sv_ball_angdragcoefficient( "sv_ball_angdragcoefficient", "0.47", FCVAR_ARCHIVE | FCVAR_NOTIFY  );
//ConVar sv_ball_elasticity( "sv_ball_elasticity", "65", FCVAR_ARCHIVE );
//ConVar sv_ball_friction( "sv_ball_friction", "1", FCVAR_ARCHIVE );
//ConVar sv_ball_speed( "sv_ball_speed", "1500", FCVAR_ARCHIVE );
ConVar sv_ball_spin( "sv_ball_spin", "300", FCVAR_ARCHIVE | FCVAR_NOTIFY );
ConVar sv_ball_defaultspin( "sv_ball_defaultspin", "20", FCVAR_ARCHIVE | FCVAR_NOTIFY );
//ConVar sv_ball_maxspin( "sv_ball_maxspin", "1000", FCVAR_ARCHIVE | FCVAR_NOTIFY );
ConVar sv_ball_curve("sv_ball_curve", "150", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_touchcone( "sv_ball_touchcone", "360", FCVAR_ARCHIVE | FCVAR_NOTIFY );
ConVar sv_ball_touchradius( "sv_ball_touchradius", "80", FCVAR_ARCHIVE | FCVAR_NOTIFY );
ConVar sv_ball_keepertouchradius( "sv_ball_keepertouchradius", "80", FCVAR_ARCHIVE | FCVAR_NOTIFY );
//ConVar sv_ball_a1_speed( "sv_ball_a1_speed", "500", FCVAR_ARCHIVE );
//ConVar sv_ball_a1_zoffset( "sv_ball_a1_zoffset", "10", FCVAR_ARCHIVE );
//ConVar sv_ball_radius( "sv_ball_radius", "5.2", FCVAR_ARCHIVE | FCVAR_NOTIFY );
//ConVar sv_ball_gravity( "sv_ball_gravity", "10", FCVAR_ARCHIVE );
//ConVar sv_ball_enable_gravity( "sv_ball_enable_gravity", "1", FCVAR_ARCHIVE );
//ConVar sv_ball_enable_drag( "sv_ball_enable_drag", "1", FCVAR_ARCHIVE );
ConVar sv_ball_shotdelay("sv_ball_shotdelay", "0.25", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_bestshotangle("sv_ball_bestshotangle", "-30", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_volleysideangle("sv_ball_volleysideangle", "30", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_keepersideangle("sv_ball_keepersideangle", "30", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_keepercatchspeed("sv_ball_keepercatchspeed", "200", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_normalshot_strength("sv_ball_normalshot_strength", "650", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_powershot_strength("sv_ball_powershot_strength", "650", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_keepershot_strength("sv_ball_keepershot_strength", "100", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_doubletouchfouls("sv_ball_doubletouchfouls", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_timelimit("sv_ball_timelimit", "10", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_slideangle("sv_ball_slideangle", "30", FCVAR_ARCHIVE | FCVAR_NOTIFY);
ConVar sv_ball_statetransitiondelay("sv_ball_statetransitiondelay", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY);

CBall *CreateBall(const Vector &pos, CSDKPlayer *pOwner)
{
	CBall *pBall = static_cast<CBall*>(CreateEntityByName("football"));
	pBall->SetAbsOrigin(pos);
	pBall->SetOwnerEntity(pOwner);
	pBall->Spawn();
	pBall->SetPos(pos);
	return pBall;
}

void CC_CreatePlayerBall(const CCommand &args)
{
	if (!SDKGameRules()->IsIntermissionState())
		return;

	CSDKPlayer *pPl = ToSDKPlayer(UTIL_GetCommandClient());
	if (!CSDKPlayer::IsOnField(pPl))
		return;

	Vector pos = pPl->GetLocalOrigin() + VEC_VIEW + pPl->EyeDirection3D() * 150;
	pos.z = max(pos.z, SDKGameRules()->m_vKickOff.GetZ());

	if (pPl->GetPlayerBall())
		pPl->GetPlayerBall()->SetPos(pos);
	else
		pPl->SetPlayerBall(CreateBall(pos, pPl));
}

static ConCommand createplayerball("createplayerball", CC_CreatePlayerBall);

CBall *g_pBall = NULL;

CBall *GetBall()
{
	return g_pBall;
}

LINK_ENTITY_TO_CLASS( football,	CBall );

//==========================================================
//	
//	
//==========================================================
BEGIN_DATADESC(	CBall )
	DEFINE_THINKFUNC( BallThink	),
	DEFINE_USEFUNC(	Use	),
	//DEFINE_ENTITYFUNC( BallTouch ),
END_DATADESC()

//==========================================================
//	
//	
//==========================================================
IMPLEMENT_SERVERCLASS_ST( CBall, DT_Ball )
	SendPropInt( SENDINFO( m_iPhysicsMode ), 2,	SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_fMass ),	0, SPROP_NOSCALE ),
	SendPropFloat(SENDINFO(m_flOffsideLineBallY), 0, SPROP_COORD),
	SendPropFloat(SENDINFO(m_flOffsideLinePlayerY), 0, SPROP_COORD),
	SendPropBool(SENDINFO(m_bShowOffsideLine))
	//ios1.1
    //SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
END_SEND_TABLE()

const objectparams_t g_IOSPhysDefaultObjectParams =
{
	NULL,
	1.0, //mass
	1.0, // inertia
	0.1f, // damping
	0.1f, // rotdamping
	0.05f, // rotIntertiaLimit
	"DEFAULT",
	NULL,// game data
	1.f, // volume (leave 0 if you don't have one or call physcollision->CollideVolume() to compute it)
	1.0f, // drag coefficient
	true,// enable collisions?
};

CBall::CBall()
{
	m_eBallState = BALL_NOSTATE;
	m_eNextState = BALL_NOSTATE;
	m_flStateLeaveTime = gpGlobals->curtime;
	m_flStateTimelimit = -1;
	m_bIgnoreTriggers = false;
	m_eFoulType = FOUL_NONE;
	m_pPossessingPl = NULL;
	m_nPossessingTeam = TEAM_INVALID;
	m_flPossessionStart = -1;
	m_pPl = NULL;
	m_nPlTeam = TEAM_INVALID;
	m_bSetNewPos = false;
	m_ePenaltyState = PENALTY_NONE;
}

CBall::~CBall()
{
}

void CBall::RemovePlayerBalls()
{
	CBall *pBall = NULL;

	while (true)
	{
		pBall = static_cast<CBall *>(gEntList.FindEntityByClassname(pBall, "football"));
		if (!pBall)
			break;

		if (pBall != this)
			UTIL_Remove(pBall);
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		pPl->SetPlayerBall(NULL);
	}
}

//==========================================================
//	
//	
//==========================================================
void CBall::Spawn (void)
{
	if (!GetOwnerEntity())
		g_pBall = this;

	//RomD: Don't fade the ball
	SetFadeDistance(-1, 0);
	DisableAutoFade();

	PrecacheModel(BALL_MODEL);
	SetModel(BALL_MODEL);

	CreateVPhysics();

	SetThink (&CBall::BallThink);
	SetNextThink(gpGlobals->curtime + (GetOwnerEntity() ? 0.05f : 0.01f));

	m_nBody = 0; 
	m_nSkin = g_IOSRand.RandomInt(0,5);
	m_pPhys->SetPosition(GetLocalOrigin(), GetLocalAngles(), true);
	m_pPhys->SetVelocityInstantaneous(&vec3_origin, &vec3_origin);

	PrecacheScriptSound( "Ball.kicknormal" );
	PrecacheScriptSound( "Ball.kickhard" );
	PrecacheScriptSound( "Ball.touch" );
	PrecacheScriptSound( "Ball.post" );
	PrecacheScriptSound( "Ball.net" );
	PrecacheScriptSound( "Ball.whistle" );
	PrecacheScriptSound( "Ball.cheer" );

	State_Transition(BALL_NORMAL);
}

bool CBall::CreateVPhysics()
{	
	if (m_pPhys)
	{
		VPhysicsDestroyObject();
		m_pPhys = NULL;
	}

	m_flPhysRadius = 5.0f;
	objectparams_t params =	g_IOSPhysDefaultObjectParams;
	params.pGameData = static_cast<void	*>(this);
	int	nMaterialIndex = physprops->GetSurfaceIndex("ios");
	m_pPhys = physenv->CreateSphereObject( m_flPhysRadius, nMaterialIndex, GetAbsOrigin(), GetAbsAngles(), &params, false );
	if (!m_pPhys)
		return false;

	VPhysicsSetObject( m_pPhys );
	
	SetSolid( SOLID_VPHYSICS );
	SetSolidFlags( FSOLID_NOT_STANDABLE	);
	//UTIL_SetSize( this,	-Vector(5.0f,5.0f,5.0f), Vector(5.0f,5.0f,5.0f)	);
	UTIL_SetSize( this,	-Vector(3.0f,3.0f,3.0f), Vector(3.0f,3.0f,3.0f)	);

	SetMoveType( MOVETYPE_VPHYSICS );

	PhysSetGameFlags(m_pPhys, FVPHYSICS_NO_PLAYER_PICKUP);

	m_pPhys->SetMass(sv_ball_mass.GetFloat());//0.05f	);
	m_fMass	= m_pPhys->GetMass();
	//m_pPhys->EnableGravity(	sv_ball_enable_gravity.GetFloat() );
	m_pPhys->EnableGravity(true);
	//m_pPhys->EnableDrag( sv_ball_enable_drag.GetFloat() );
	m_pPhys->EnableDrag(false);
	//SetElasticity(sv_ball_elasticity.GetFloat());
	//SetGravity(sv_ball_gravity.GetFloat());
	//float drag = sv_ball_dragcoefficient.GetFloat();
	//float angdrag = sv_ball_angdragcoefficient.GetFloat();
	//m_pPhys->SetDragCoefficient(&drag, &angdrag);
	float flDamping	= sv_ball_damping.GetFloat(); //0.0f
	float flAngDamping = sv_ball_rotdamping.GetFloat(); //2.5f
	m_pPhys->SetDamping( &flDamping, &flAngDamping );
	//float drag = 0;
	//m_pPhys->SetDragCoefficient(&drag, &drag);
	//m_pPhys->SetInertia(sv_ball_inertia.GetFloat());
	//VPhysicsGetObject()->SetInertia( Vector( 0.0023225760f,	0.0023225760f, 0.0023225760f ) );
	SetPhysicsMode(PHYSICS_MULTIPLAYER_SOLID);
	//SetPhysicsMode(PHYSICS_MULTIPLAYER_AUTODETECT);
	SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );
	m_pPhys->Wake();

	return true;
}

void CBall::VPhysicsUpdate(IPhysicsObject *pPhysics)
{
	CReplayManager::GetInstance()->CheckReplay();

	bool ignoreTriggers = m_bIgnoreTriggers;

	if (m_bSetNewPos)
	{
		m_bIgnoreTriggers = true;
		m_bSetNewPos = false;
	}
	else
	{
		Vector vel, worldAngImp;
		AngularImpulse angImp;
		m_pPhys->GetVelocity(&vel, &angImp);
		EntityToWorldSpace(angImp, &worldAngImp);
		Vector magnusDir = worldAngImp.Cross(vel);

		float length = vel.Length();
		if (length > 0)
		{
			vel += magnusDir * 1e-6 * sv_ball_curve.GetFloat() * gpGlobals->frametime;
			//m_vVel.NormalizeInPlace();
			//m_vVel *= length;
		}

		VPhysicsGetObject()->SetVelocity(&vel, NULL);
	}

	BaseClass::VPhysicsUpdate(pPhysics);

	m_bIgnoreTriggers = ignoreTriggers;
}


void CBall::VPhysicsCollision( int index, gamevcollisionevent_t	*pEvent	)
{
	Vector preVelocity = pEvent->preVelocity[index];
	float flSpeed =	VectorNormalize( preVelocity );
	int surfaceProps = pEvent->surfaceProps[!index];

	//IOS goal post hacks!!
	if (surfaceProps == 81 && flSpeed > 300.0f)
	{
		EmitSound("Ball.post");
	}
	else
	{
		//if ball is moving fast when we hit something play a sound
		if (flSpeed > 500.0f)
		{
			EmitSound("Ball.touch");
		}
	}
	
	//iosgoalnets 82=iosgoalnets, 30=concrete!!! TEMP!!! until pricey changes nets surfaceprop!
	if ((surfaceProps == 82 /*|| surfaceProps == 30*/) && flSpeed > 300.0f)
	{
		EmitSound("Ball.net");
	}

	///////////////////////////////////////////////////////////////////////
	// player
	//this doesnt seem to get called often enough to be any use!!
	IPhysicsObject *pPhysObj = pEvent->pObjects[!index];
	CBaseEntity *pOther = static_cast<CBaseEntity *>(pPhysObj->GetGameData());
	if (pOther && pOther->IsPlayer())
	{
		CSDKPlayer *pPl = ToSDKPlayer(pOther);
		if (flSpeed > 900.0f)
			pPl->EmitSound ("Player.Oomph");

		if (m_pCurStateInfo->m_eBallState == BALL_NORMAL)
		{
			Vector pos;
			m_pPhys->GetPosition(&pos, NULL);
			Touched(pPl, false, BODY_HEAD);
		}

		EmitSound("Ball.touch");
	}

	//Warning ("surfaceprops index %d\n", surfaceProps);

	BaseClass::VPhysicsCollision( index, pEvent );
}

void CBall::SendMatchEvent(match_event_t matchEvent, CSDKPlayer *pPlayer)
{
	if (!pPlayer)
		pPlayer = m_pPl;

	Assert(pPlayer);

	UTIL_LogPrintf( "\"%s<%d><%s><%s>\" triggered \"%d\"\n",
		pPlayer->GetPlayerName(), pPlayer->GetUserID(),
		pPlayer->GetNetworkIDString(), pPlayer->GetTeam()->GetKitName(), matchEvent);

	DevMsg( "\"%s<%d><%s><%s>\" triggered \"%d\"\n",
		pPlayer->GetPlayerName(), pPlayer->GetUserID(),
		pPlayer->GetNetworkIDString(), pPlayer->GetTeam()->GetKitName(), matchEvent);

	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin(filter, "MatchEvent");
		WRITE_BYTE(matchEvent);
		WRITE_STRING(pPlayer->GetPlayerName());
		WRITE_BYTE(pPlayer->GetTeamNumber());
	MessageEnd();
}

CSDKPlayer *CBall::FindNearestPlayer(int team /*= TEAM_INVALID*/, int posFlags /*= FL_POS_FIELD*/, bool checkIfShooting /*= false*/, int ignoredPlayerBits /*= 0*/)
{
	CSDKPlayer *pNearest = NULL;
	float shortestDist = FLT_MAX;
	//if (team == TEAM_INVALID)
	//	team = g_IOSRand.RandomInt(TEAM_A, TEAM_B);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPlayer = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPlayer))
			continue;

		if (ignoredPlayerBits & (1 << (pPlayer->entindex() - 1)))
			continue;

		if (!(posFlags & FL_POS_ANY) && ((posFlags & FL_POS_KEEPER) && pPlayer->GetTeamPosition() != 1 || (posFlags & FL_POS_FIELD) && pPlayer->GetTeamPosition() == 1))
			continue;

		if (team != TEAM_INVALID && pPlayer->GetTeamNumber() != team)
			continue;

		if (checkIfShooting && (!(pPlayer->m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1))) || pPlayer->m_flNextShot > gpGlobals->curtime))
			continue;

		Vector dir = m_vPos - pPlayer->GetLocalOrigin();
		float xyDist = dir.Length2D();
		float zDist = m_vPos.z - (pPlayer->GetLocalOrigin().z + SDKGameRules()->GetViewVectors()->m_vHullMax.z); //pPlayer->GetPlayerMaxs().z);// 
		float dist = max(xyDist, zDist);

		if (dist < shortestDist)
		{
			shortestDist = dist;
			pNearest = pPlayer;	
		}
	}

	//// If we didn't find a player of a certain team, just look for any other player
	//if (!pNearest && team != TEAM_INVALID)
	//	pNearest = FindNearestPlayer();

	return pNearest;
}

void CBall::SetPos(const Vector &pos)
{
	m_vPos = Vector(pos.x, pos.y, pos.z + m_flPhysRadius);
	m_vVel = vec3_origin;
	m_vRot = vec3_origin;
	m_pPhys->EnableMotion(true);
	m_pPhys->SetVelocityInstantaneous(&vec3_origin, &vec3_origin);
	m_pPhys->SetPosition(m_vPos, m_aAng, true);
	m_pPhys->EnableMotion(false);
	m_bSetNewPos = true;
}

void CBall::SetVel(const Vector &vel)
{
	m_vVel = vel;
	m_pPhys->EnableMotion(true);
	m_pPhys->SetVelocity(&m_vVel, &m_vRot);
}

void CBall::SetRot(const AngularImpulse &rot)
{
	m_vRot = rot;
	m_pPhys->EnableMotion(true);
	m_pPhys->SetVelocity(&m_vVel, &m_vRot);
}

ConVar mp_showballstatetransitions( "mp_showballstatetransitions", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show ball state transitions." );

void CBall::State_Transition( ball_state_t newState, float delay /*= 0.0f*/ )
{
	m_eNextState = newState;
	m_flStateLeaveTime = gpGlobals->curtime + delay;
	m_bIgnoreTriggers = true;
}

void CBall::State_DoTransition( ball_state_t newState )
{
	m_eNextState = BALL_NOSTATE;
	State_Enter( newState );
}

void CBall::State_Enter( ball_state_t newState )
{
	m_eBallState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );
	m_flStateEnterTime = gpGlobals->curtime;
	m_flStateTimelimit = -1;

	m_pPl = NULL;
	m_pOtherPl = NULL;
	SDKGameRules()->DisableShield();
	UnmarkOffsidePlayers();
	m_bIgnoreTriggers = false;
	RemoveEffects(EF_NODRAW);
	m_pPhys->EnableCollisions(true);
	m_pPhys->EnableMotion(true);

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (pPl->GetTeamPosition() == 1)
		{
			if (pPl->m_nBody == MODEL_KEEPER_AND_BALL)
			{
				pPl->m_nBody = MODEL_KEEPER;
				pPl->DoServerAnimationEvent(PLAYERANIMEVENT_CARRY_END);
			}
		}

		pPl->m_bIsAtTargetPos = false;
		pPl->RemoveFlag(FL_REMOTECONTROLLED | FL_CELEB | FL_NO_X_MOVEMENT | FL_NO_Y_MOVEMENT | FL_ATCONTROLS | FL_FROZEN);
		//pPl->SetMoveType(MOVETYPE_WALK);
		pPl->RemoveSolidFlags(FSOLID_NOT_SOLID);
	}

	if (newState == BALL_NORMAL)
		SDKGameRules()->EndInjuryTime();
	else
		SDKGameRules()->StartInjuryTime();

	if ( mp_showballstatetransitions.GetInt() > 0 )
	{
		if ( m_pCurStateInfo )
			Msg( "Ball: entering state '%s'\n", m_pCurStateInfo->m_pStateName );
		else
			Msg( "Ball: entering state #%d\n", newState );
	}

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		(this->*m_pCurStateInfo->pfnEnterState)();
	}
}

void CBall::State_Think()
{
	if (m_flStateLeaveTime > gpGlobals->curtime)
		return;

	m_pPhys->GetPosition(&m_vPos, &m_aAng);
	m_pPhys->GetVelocity(&m_vVel, &m_vRot);

	//if (m_flStateTimelimit != -1 && m_flStateTimelimit <= gpGlobals->curtime)
	if (m_pCurStateInfo && m_pCurStateInfo->m_eBallState != BALL_NORMAL && m_flStateTimelimit != -1 && gpGlobals->curtime >= m_flStateTimelimit)
	{
		if (CSDKPlayer::IsOnField(m_pPl))
			m_pPl->ChangeTeam(TEAM_SPECTATOR);
	}

	while (m_eNextState != BALL_NOSTATE && m_flStateLeaveTime <= gpGlobals->curtime)
	{
		State_DoTransition(m_eNextState);
	}

	if (m_pCurStateInfo && m_pCurStateInfo->pfnThink)
	{	
		(this->*m_pCurStateInfo->pfnThink)();
	}
}

CBallStateInfo* CBall::State_LookupInfo( ball_state_t state )
{
	static CBallStateInfo ballStateInfos[] =
	{
		{ BALL_NORMAL,		"BALL_NORMAL",		&CBall::State_NORMAL_Enter,			&CBall::State_NORMAL_Think },
		{ BALL_KICKOFF,		"BALL_KICKOFF",		&CBall::State_KICKOFF_Enter,		&CBall::State_KICKOFF_Think },
		{ BALL_THROWIN,		"BALL_THROWIN",		&CBall::State_THROWIN_Enter,		&CBall::State_THROWIN_Think },
		{ BALL_GOALKICK,	"BALL_GOALKICK",	&CBall::State_GOALKICK_Enter,		&CBall::State_GOALKICK_Think },
		{ BALL_CORNER,		"BALL_CORNER",		&CBall::State_CORNER_Enter,			&CBall::State_CORNER_Think },
		{ BALL_GOAL,		"BALL_GOAL",		&CBall::State_GOAL_Enter,			&CBall::State_GOAL_Think },
		{ BALL_FREEKICK,	"BALL_FREEKICK",	&CBall::State_FREEKICK_Enter,		&CBall::State_FREEKICK_Think },
		{ BALL_PENALTY,		"BALL_PENALTY",		&CBall::State_PENALTY_Enter,		&CBall::State_PENALTY_Think },
		{ BALL_KEEPERHANDS,	"BALL_KEEPERHANDS",	&CBall::State_KEEPERHANDS_Enter,	&CBall::State_KEEPERHANDS_Think },
	};

	for ( int i=0; i < ARRAYSIZE( ballStateInfos ); i++ )
	{
		if ( ballStateInfos[i].m_eBallState == state )
			return &ballStateInfos[i];
	}

	return NULL;
}

void CBall::State_NORMAL_Enter()
{
}

void CBall::State_NORMAL_Think()
{
	for (int ignoredPlayerBits = 0;;)
	{
		if (SDKGameRules()->State_Get() == MATCH_PENALTIES && m_ePenaltyState == PENALTY_KICKED)
			m_pPl = FindNearestPlayer(m_nFoulingTeam, FL_POS_KEEPER);
		else
			m_pPl = FindNearestPlayer(TEAM_INVALID, FL_POS_ANY, true, ignoredPlayerBits);

		if (!m_pPl)
			return;

		UpdateCarrier();

		if (DoBodyPartAction())
			break;

		if (SDKGameRules()->State_Get() == MATCH_PENALTIES && m_ePenaltyState == PENALTY_KICKED)
			break;

		ignoredPlayerBits |= (1 << (m_pPl->entindex() - 1));
	}

	if (!SDKGameRules()->IsIntermissionState() && !GetOwnerEntity())
		MarkOffsidePlayers();
}

void CBall::State_KICKOFF_Enter()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->RemoveFlag(FL_ATCONTROLS);
		pPl->DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
		//pPl->m_nInPenBoxOfTeam = TEAM_INVALID;
	}

	SetPos(SDKGameRules()->m_vKickOff);
}

void CBall::State_KICKOFF_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		int kickOffTeam;
		if (m_bIsKickOffAfterGoal)
		{
			kickOffTeam = LastOppTeam(true);
		}
		else
		{
			if (SDKGameRules()->GetTeamsSwapped())
				kickOffTeam = SDKGameRules()->GetKickOffTeam() == TEAM_A ? TEAM_B : TEAM_A;
			else
				kickOffTeam = SDKGameRules()->GetKickOffTeam();
		}

		m_pPl = FindNearestPlayer(kickOffTeam);
		if (!m_pPl)
			m_pPl = FindNearestPlayer(GetGlobalTeam(kickOffTeam)->GetOppTeamNumber());
		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_KICKOFF, m_pPl->GetTeamNumber(), SDKGameRules()->m_vKickOff);
		UpdatePossession(m_pPl);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x - m_pPl->GetTeam()->m_nRight * 30, m_vPos.y, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		EmitSound("Ball.whistle");
		SendMatchEvent(MATCH_EVENT_KICKOFF);
		//PlayersAtTargetPos(false);
	}

	if (!CSDKPlayer::IsOnField(m_pOtherPl))
	{
		m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_FIELD, false, (1 << (m_pPl->entindex() - 1)));
		if (m_pOtherPl)
			m_pOtherPl->SetPosInsideShield(Vector(m_vPos.x + m_pPl->GetTeam()->m_nRight * 100, m_vPos.y, SDKGameRules()->m_vKickOff.GetZ()), true);
	}

	if (!PlayersAtTargetPos(false))
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
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit.GetFloat();

	UpdateCarrier();

	if (m_pPl->m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1)))
	{
		m_Touches.RemoveAll();
		SetVel(m_vPlForward2D * 200);
		Kicked(BODY_FEET);
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		if (m_pOtherPl)
			m_pOtherPl->RemoveFlag(FL_ATCONTROLS);
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_THROWIN_Enter()
{
	SetPos(Vector(m_vTriggerTouchPos.x + 0 * Sign(SDKGameRules()->m_vKickOff.GetX() - m_vTriggerTouchPos.x), m_vTriggerTouchPos.y, SDKGameRules()->m_vKickOff.GetZ()));
}

void CBall::State_THROWIN_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(LastOppTeam(false));
		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_THROWIN, m_pPl->GetOppTeamNumber(), m_vPos);
		UpdatePossession(m_pPl);
		m_pPl->SetPosInsideShield(Vector(m_vTriggerTouchPos.x, m_vTriggerTouchPos.y, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		SendMatchEvent(MATCH_EVENT_THROWIN);
		EmitSound("Ball.whistle");
		PlayersAtTargetPos(false);
	}

	//if (!PlayersAtTargetPos(false))
	//	return;

	if (!m_pPl->m_bIsAtTargetPos)
		return;

	if (m_pPl->GetFlags() & FL_ATCONTROLS)
	{
		SetPos(Vector(m_vTriggerTouchPos.x, m_vTriggerTouchPos.y, SDKGameRules()->m_vKickOff.GetZ() + VEC_HULL_MAX.z + 2));
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_THROWIN);
	}

	if (m_flStateTimelimit == -1)
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit.GetFloat();

	UpdateCarrier();

	if (m_pPl->m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1)))
	{
		m_Touches.RemoveAll();
		QAngle ang = m_pPl->EyeAngles();
		if (ang[PITCH] > 20)
			ang[PITCH] = 20;
		Vector dir;
		AngleVectors(ang, &dir);

		if (m_pPl->m_nButtons & IN_ATTACK)
		{
			SetVel(dir * 250);
		}
		else
		{
			SetVel(dir * (250 + 500 * GetPowershotModifier() * GetPitchModifier()));
		}

		Kicked(BODY_HANDS);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_THROW);
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_GOALKICK_Enter()
{
	Vector ballPos;
	if ((m_vTriggerTouchPos - SDKGameRules()->m_vKickOff).x > 0)
		ballPos = GetGlobalTeam(LastOppTeam(false))->m_vGoalkickLeft;
	else
		ballPos = GetGlobalTeam(LastOppTeam(false))->m_vGoalkickRight;

	SetPos(ballPos);
}

void CBall::State_GOALKICK_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(LastOppTeam(false), FL_POS_KEEPER);
		if (!m_pPl)
			m_pPl = FindNearestPlayer(LastOppTeam(false));
		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_GOALKICK, m_pPl->GetTeamNumber());
		UpdatePossession(m_pPl);
		//m_pPl->m_bIsAtTargetPos = true;
		//m_pPl->AddFlag(FL_SHIELD_KEEP_IN | FL_REMOTECONTROLLED);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 100 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), false);
		m_flStateTimelimit = -1;
		SendMatchEvent(MATCH_EVENT_GOALKICK);
		EmitSound("Ball.whistle");
		PlayersAtTargetPos(false);

		//for (int i = 1; i <= gpGlobals->maxClients; i++) 
		//{
		//	CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		//	if (!CSDKPlayer::IsOnField(pPl))
		//		continue;

		//	if (pPl == m_pPl)
		//		continue;

		//	pPl->AddFlag(FL_SHIELD_KEEP_OUT | FL_REMOTECONTROLLED);
		//}
	}

	if (!m_pPl->m_bIsAtTargetPos)
		return;

	//if (!PlayersAtTargetPos(false))
	//	return;

	if (m_flStateTimelimit == -1)
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit.GetFloat();

	UpdateCarrier();

	if (m_pPl->m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1)) && (m_vPos - m_vPlPos).Length2D() <= sv_ball_touchradius.GetFloat())
	{
		m_Touches.RemoveAll();
		DoGroundShot();
		MarkOffsidePlayers();
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_CORNER_Enter()
{
	UnmarkOffsidePlayers();

	Vector ballPos;
	CTeam *pTeam = GetGlobalTeam(LastTeam(false));

	if (Sign((m_vTriggerTouchPos - SDKGameRules()->m_vKickOff).x) == -pTeam->m_nRight)
		ballPos = pTeam->m_vCornerLeft;
	else
		ballPos = pTeam->m_vCornerRight;
	
	SetPos(ballPos);
}

void CBall::State_CORNER_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(LastOppTeam(false));
		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_CORNER, m_pPl->GetOppTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x - 50 * Sign((SDKGameRules()->m_vKickOff - m_vPos).x), m_vPos.y - 50 * Sign((SDKGameRules()->m_vKickOff - m_vPos).y), SDKGameRules()->m_vKickOff[2]), false);
		UpdatePossession(m_pPl);
		m_flStateTimelimit = -1;
		SendMatchEvent(MATCH_EVENT_CORNER);
		EmitSound("Ball.whistle");
		PlayersAtTargetPos(false);
	}

	//if (!PlayersAtTargetPos(false))
	//	return;

	if (m_flStateTimelimit == -1)
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit.GetFloat();

	UpdateCarrier();

	if (m_pPl->m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1)) && (m_vPos - m_vPlPos).Length2D() <= sv_ball_touchradius.GetFloat())
	{
		m_Touches.RemoveAll();
		DoGroundShot();
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_GOAL_Enter()
{
	UpdatePossession(NULL);
	m_bIsKickOffAfterGoal = true;

	if (m_nTeam == LastTeam(true))
	{
		GetGlobalTeam(LastOppTeam(true))->AddGoal();
		SendMatchEvent(MATCH_EVENT_OWNGOAL, LastPl(true));
	}
	else
	{
		CSDKPlayer *pScorer = LastPl(true);
		if (pScorer)
		{
			pScorer->m_Goals += 1;
			SendMatchEvent(MATCH_EVENT_GOAL, LastPl(true));

			CSDKPlayer *pAssister = LastPl(true, pScorer);

			if (pAssister && pAssister->GetTeam() == pScorer->GetTeam())
			{
				pAssister->m_Assists += 1;
				SendMatchEvent(MATCH_EVENT_ASSIST, pAssister);
			}
		}

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

			if (!CSDKPlayer::IsOnField(pPl))
				continue;

			if (pPl->GetTeamNumber() == LastTeam(true))
				pPl->AddFlag(FL_CELEB);
		}

		GetGlobalTeam(LastTeam(true))->AddGoal();
	}

	EmitSound("Ball.whistle");
	State_Transition(BALL_KICKOFF, 5);
}

void CBall::State_GOAL_Think()
{
}

void CBall::State_FREEKICK_Enter()
{
	DisableOffsideLine();
	SetPos(m_vFoulPos);
	if (m_pFoulingPl)
	{
		match_event_t matchEvent = MATCH_EVENT_NONE;
		switch (m_eFoulType)
		{
		case FOUL_NORMAL: matchEvent = MATCH_EVENT_FOUL; break;
		case FOUL_DOUBLETOUCH: matchEvent = MATCH_EVENT_DOUBLETOUCH; break;
		case FOUL_OFFSIDE: matchEvent = MATCH_EVENT_OFFSIDE; break;
		}
		SendMatchEvent(matchEvent, m_pFoulingPl);
	}
}

void CBall::State_FREEKICK_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl = m_pFouledPl;
		if (!CSDKPlayer::IsOnField(m_pPl))
		{
			m_pPl = FindNearestPlayer(GetGlobalTeam(m_nFoulingTeam)->GetOppTeamNumber());
			if (!m_pPl)
				return State_Transition(BALL_NORMAL);
		}

		SDKGameRules()->EnableShield(SHIELD_FREEKICK, m_pPl->GetOppTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 100 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), false);
		UpdatePossession(m_pPl);
		m_flStateTimelimit = -1;
		SendMatchEvent(MATCH_EVENT_FREEKICK);
		EmitSound("Ball.whistle");
		PlayersAtTargetPos(false);
	}

	if (m_flStateTimelimit == -1)
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit.GetFloat();

	UpdateCarrier();

	if (m_pPl->m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1)) && (m_vPos - m_vPlPos).Length2D() <= sv_ball_touchradius.GetFloat())
	{
		m_Touches.RemoveAll();
		DoGroundShot();
		MarkOffsidePlayers();
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_PENALTY_Enter()
{
	if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
	{
		SetPos(GetGlobalTeam(m_nFoulingTeam)->m_vPenalty);
	}
	else
	{
		SetPos(GetGlobalTeam(m_nFoulingTeam)->m_vPenalty);

		if (m_pFoulingPl)
			SendMatchEvent(MATCH_EVENT_FOUL, m_pFoulingPl);
	}
}

void CBall::State_PENALTY_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl = m_pFouledPl;

		if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
		{
			if (!CSDKPlayer::IsOnField(m_pPl))
			{
				m_ePenaltyState = PENALTY_ABORTED_NO_TAKER;
				return State_Transition(BALL_NORMAL);
			}

			SDKGameRules()->EnableShield(SHIELD_PENALTY, m_nFoulingTeam, GetGlobalTeam(m_nFoulingTeam)->m_vPenalty);
			m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 150 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), true);
		}
		else
		{
			if (!CSDKPlayer::IsOnField(m_pPl))
			{
				m_pPl = FindNearestPlayer(GetGlobalTeam(m_nFoulingTeam)->GetOppTeamNumber());
				if (!m_pPl)
					return State_Transition(BALL_NORMAL);
			}

			SDKGameRules()->EnableShield(SHIELD_PENALTY, m_nFoulingTeam, GetGlobalTeam(m_nFoulingTeam)->m_vPenalty);
			m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 150 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), true);
		}

		UpdatePossession(m_pPl);
		m_flStateTimelimit = -1;
		SendMatchEvent(MATCH_EVENT_PENALTY);
		EmitSound("Ball.whistle");
	}

	if (!CSDKPlayer::IsOnField(m_pOtherPl))
	{
		if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
		{
			m_pOtherPl = FindNearestPlayer(m_pPl->GetOppTeamNumber(), FL_POS_KEEPER);
			if (!m_pOtherPl)
			{
				m_ePenaltyState = PENALTY_ABORTED_NO_KEEPER;
				return State_Transition(BALL_NORMAL);
			}
		}
		else
		{
			m_pOtherPl = FindNearestPlayer(m_nInPenBoxOfTeam, FL_POS_KEEPER);
			if (!m_pOtherPl)
				return State_Transition(BALL_NORMAL);

		}

		m_pOtherPl->SetPosInsideShield(m_pOtherPl->GetTeam()->m_vPlayerSpawns[0], true);
	}

	if (!PlayersAtTargetPos(false))
		return;

	if (m_pPl->GetFlags() & FL_ATCONTROLS)
		m_pPl->RemoveFlag(FL_ATCONTROLS);

	if (m_pOtherPl->GetFlags() & FL_ATCONTROLS)
	{
		m_pOtherPl->RemoveFlag(FL_ATCONTROLS);
		m_pOtherPl->AddFlag(FL_NO_Y_MOVEMENT);
	}

	if (m_flStateTimelimit == -1)
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit.GetFloat();

	UpdateCarrier();

	if (m_pPl->m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1)) && (m_vPos - m_vPlPos).Length2D() <= sv_ball_touchradius.GetFloat())
	{
		m_Touches.RemoveAll();
		m_ePenaltyState = PENALTY_KICKED;
		m_pPl->m_ePenaltyState = PENALTY_KICKED;
		m_pOtherPl->RemoveFlag(FL_NO_Y_MOVEMENT);
		DoGroundShot();
		MarkOffsidePlayers();
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_KEEPERHANDS_Enter()
{
}

void CBall::State_KEEPERHANDS_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(m_nInPenBoxOfTeam, FL_POS_KEEPER);
		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_KEEPERHANDS, m_pPl->GetTeamNumber());
		UpdatePossession(m_pPl);
		//m_pPl->m_bIsAtTargetPos = true;
		m_pPl->SetPosInsideShield(vec3_invalid, false);

		m_pPl->m_nBody = MODEL_KEEPER_AND_BALL;
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_CARRY);
		m_pPl->m_nSkin = m_pPl->m_nBaseSkin + m_nSkin;
		SetEffects(EF_NODRAW);
		m_pPhys->EnableCollisions(false);
		m_flStateTimelimit = -1;
		Touched(m_pPl, true, BODY_HANDS);

		if (m_pPl->m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1)))
			m_pPl->m_bShotButtonsDepressed = false;

		PlayersAtTargetPos(false);
	}

	if (!m_pPl->m_bIsAtTargetPos)
		return;

	if (m_flStateTimelimit == -1)
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit.GetFloat();

	UpdateCarrier();

	SetPos(Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + BODY_CHEST_END) + m_vPlForward2D * 2 * VEC_HULL_MAX.x);

	//if (m_nInPenBoxOfTeam != m_pPl->GetTeamNumber())
	//{
	//	Kicked(BODY_HANDS);
	//	MarkOffsidePlayers();
	//	return State_Transition(BALL_NORMAL);
	//}

	if (m_pPl->m_bShotButtonsDepressed && (m_pPl->m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1))) && m_pPl->m_flNextShot <= gpGlobals->curtime)
	{
		m_Touches.RemoveAll();
		if (m_bIsPowershot)
		{
			SetVel(m_vPlForward * sv_ball_powershot_strength.GetFloat() * (1 + GetPowershotModifier()) * GetPitchModifier());
			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KEEPER_HANDS_KICK);
		}
		else
		{
			SetVel(m_vPlForward * sv_ball_powershot_strength.GetFloat() * (1 + GetPowershotModifier()) * GetPitchModifier());
			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KEEPER_HANDS_THROW);
		}

		Kicked(BODY_HANDS);
		MarkOffsidePlayers();
		return State_Transition(BALL_NORMAL);
	}
}

bool CBall::PlayersAtTargetPos(bool holdAtTargetPos)
{
	bool playersAtTarget = true;

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (!pPl->m_bIsAtTargetPos)
		{
			if (!(pPl->GetFlags() & FL_REMOTECONTROLLED))
			{
				pPl->SetPosOutsideShield(holdAtTargetPos);
			}

			if (!pPl->m_bIsAtTargetPos)
				playersAtTarget = false;
		}
	}

	return playersAtTarget;
}

bool CBall::CheckFoul(bool canShootBall)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		Vector plPos = pPl->GetLocalOrigin();

		if (plPos.x < SDKGameRules()->m_vFieldMin.GetX() || plPos.y < SDKGameRules()->m_vFieldMin.GetY() ||
			plPos.x > SDKGameRules()->m_vFieldMax.GetX() || plPos.y > SDKGameRules()->m_vFieldMax.GetY())
			continue;

		if (pPl == m_pPl || pPl->GetTeamNumber() == m_nPlTeam)
			continue;

		Vector dirToPl = pPl->GetLocalOrigin() - m_vPlPos;
		float distToPl = dirToPl.Length2D();

		if (distToPl > sv_ball_touchradius.GetFloat() * 2)
			continue;

		dirToPl.z = 0;
		dirToPl.NormalizeInPlace();
		if (RAD2DEG(acos(m_vPlForward2D.Dot(dirToPl))) > sv_ball_slideangle.GetFloat())
			continue;

		if (canShootBall && distToPl >= (m_vPos - m_vPlPos).Length2D())
			continue;

		m_pPl->m_Fouls += 1;
		m_pPl->m_YellowCards += 1;
		//if (m_pPl->m_YellowCards % 2 == 0)
		//{
		//	m_pPl->m_flNextJoin = gpGlobals->curtime + 30;
		//	m_pPl->ChangeTeam(TEAM_SPECTATOR);
		//}
		pPl->DoAnimationEvent(RAD2DEG(acos(m_vPlForward2D.Dot(pPl->EyeDirection2D()))) <= 90 ? PLAYERANIMEVENT_TACKLED_BACKWARD : PLAYERANIMEVENT_TACKLED_FORWARD);
		TriggerFoul(FOUL_NORMAL, pPl->GetLocalOrigin(), m_pPl, pPl);
		if (pPl->m_nInPenBoxOfTeam == m_nPlTeam)
			State_Transition(BALL_PENALTY, sv_ball_statetransitiondelay.GetFloat());
		else
			State_Transition(BALL_FREEKICK, sv_ball_statetransitiondelay.GetFloat());

		return true;
	}

	return false;
}

void CBall::TriggerFoul(foul_type_t type, Vector pos, CSDKPlayer *pFoulingPl, CSDKPlayer *pFouledPl /*= NULL*/)
{
	m_eFoulType = type;
	m_pFoulingPl = pFoulingPl;
	m_pFouledPl = pFouledPl;
	m_nFoulingTeam = pFoulingPl->GetTeamNumber();
	m_vFoulPos = pos;
}

bool CBall::DoBodyPartAction()
{
	Vector dirToBall = m_vPos - m_vPlPos;
	float zDist = dirToBall.z;
	float xyDist = dirToBall.Length2D();
	Vector localDirToBall;
	VectorIRotate(dirToBall, m_pPl->EntityToWorldTransform(), localDirToBall);
	dirToBall.z = 0;
	dirToBall.NormalizeInPlace();

	if (m_pPl->m_ePlayerAnimEvent == PLAYERANIMEVENT_SLIDE)
	{
		if (xyDist <= sv_ball_touchradius.GetFloat() * 2)
		{
			bool canShootBall = RAD2DEG(acos(m_vPlForward2D.Dot(dirToBall))) <= sv_ball_slideangle.GetFloat() && zDist <= BODY_FEET_END;

			if (CheckFoul(canShootBall))
				return false;

			if (canShootBall)
			{
				SetVel(m_vPlForward2D * m_vPlVel.Length2D() * 2.5f);
				Kicked(BODY_FEET);
				return true;
			}
		}
		return false;
	}

	if (m_pPl->GetTeamPosition() == 1 && m_nInPenBoxOfTeam == m_pPl->GetTeamNumber())
	{
		bool canCatch;

		switch (m_pPl->m_ePlayerAnimEvent)
		{
		case PLAYERANIMEVENT_KEEPER_DIVE_LEFT:
			canCatch = zDist <= BODY_CHEST_END && abs(localDirToBall.x) <= sv_ball_keepertouchradius.GetFloat() / 2 && localDirToBall.y >= 0 && localDirToBall.y <= sv_ball_keepertouchradius.GetFloat() * 2;
			break;
		case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT:
			canCatch = zDist <= BODY_CHEST_END && abs(localDirToBall.x) <= sv_ball_keepertouchradius.GetFloat() / 2 && localDirToBall.y <= 0 && localDirToBall.y >= -sv_ball_keepertouchradius.GetFloat() * 2;
			break;
		case PLAYERANIMEVENT_KEEPER_DIVE_FORWARD:
			canCatch = zDist <= BODY_CHEST_END && localDirToBall.x >= 0 && localDirToBall.x <= sv_ball_keepertouchradius.GetFloat() * 2 && abs(localDirToBall.y) <= sv_ball_keepertouchradius.GetFloat() / 2;
			break;
		case PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD:
			canCatch = zDist <= BODY_HEAD_END + 20 && localDirToBall.x <= 0 && localDirToBall.x >= -sv_ball_keepertouchradius.GetFloat() * 2 && abs(localDirToBall.y) <= sv_ball_keepertouchradius.GetFloat() / 2;
			break;
		case PLAYERANIMEVENT_KEEPER_JUMP:
			canCatch = xyDist <= sv_ball_keepertouchradius.GetFloat() && zDist >= BODY_CHEST_START && zDist <= BODY_HEAD_END + 20;
			break;
		default:
			canCatch = xyDist <= sv_ball_keepertouchradius.GetFloat() && zDist >= BODY_CHEST_START && zDist <= BODY_HEAD_END;
			break;
		}

		if (!canCatch)
		{
			if (zDist >= BODY_FEET_START && zDist < BODY_HIP_END && xyDist <= sv_ball_touchradius.GetInt())
				return DoGroundShot();
			else
				return false;
		}

		if (LastPl(true) == m_pPl && (LastInfo(true)->m_eBallState != BALL_NORMAL || LastInfo(true)->m_eBodyPart == BODY_HANDS))
			return false;

		BallTouchInfo *pInfo = LastInfo(true, m_pPl);
		if (!pInfo)
			return false;

		if (pInfo->m_nTeam == m_pPl->GetTeamNumber() && pInfo->m_eBodyPart != BODY_HEAD && pInfo->m_eBodyPart != BODY_CHEST)
			return false;

		if (m_vVel.Length2D() > sv_ball_keepercatchspeed.GetInt())
		{
			//m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KEEPER_HANDS_PUNCH);
			//if (zDist >= (m_pPl->m_ePlayerAnimEvent == PLAYERANIMEVENT_KEEPER_JUMP ? BODY_HEAD_END + 10 : VEC_HULL_MAX.z))
			if (m_pPl->m_ePlayerAnimEvent == PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD)
			{
				//VectorRotate(m_vVel, QAngle(0, -45, 0), m_vVel);
				SetVel(Vector(m_vVel.x, m_vVel.y, max(m_vVel.z, sv_ball_normalshot_strength.GetFloat() * 0.75f)));
			}
			else
				SetVel(m_vPlForward * m_vVel.Length2D() * 0.75f);

			Kicked(BODY_HANDS);
			return true;
		}
		else
		{
			State_Transition(BALL_KEEPERHANDS);
			return true;
		}
	}

	if (zDist >= BODY_FEET_START && zDist < BODY_FEET_END && xyDist <= sv_ball_touchradius.GetInt())
		return DoGroundShot();

	if (zDist >= BODY_HIP_START && zDist < BODY_HIP_END && xyDist <= sv_ball_touchradius.GetInt())
	{
		if (DoVolleyShot())
			return true;
		else
			return DoChestDrop();
	}

	if (zDist >= BODY_CHEST_START && zDist < BODY_CHEST_END && xyDist <= sv_ball_touchradius.GetInt())
		return DoChestDrop();

	if (zDist >= BODY_HEAD_START && zDist < BODY_HEAD_END && xyDist <= sv_ball_touchradius.GetInt())
		return DoHeader();

	return false;
}

float CBall::GetPitchModifier()
{
	//float modifier = (90 - (abs((clamp(m_aPlAng[PITCH], -75, 75) - BEST_SHOT_ANGLE) * 0.5f + BEST_SHOT_ANGLE))) / 90.0f;
	//float modifier = (1 - (cos((90 - abs(clamp(m_aPlAng[PITCH], -90, 90))) / 90.0f * M_PI))) / 2.0f;
	return pow(cos((m_pPl->EyeAngles()[PITCH] - sv_ball_bestshotangle.GetInt()) / (PITCH_LIMIT - sv_ball_bestshotangle.GetInt()) * M_PI / 2), 2);
}

float CBall::GetPowershotModifier()
{
	int powershotStrength = min(m_pPl->m_nPowershotStrength, m_pPl->m_Shared.GetStamina());
	m_pPl->m_Shared.SetStamina(m_pPl->m_Shared.GetStamina() - powershotStrength);
	return powershotStrength / 100.0f;
}

bool CBall::DoGroundShot()
{
	float shotStrength;

	float modifier = GetPitchModifier();

	//has enough sprint for a powershot?
	if (m_bIsPowershot)
	{
		shotStrength = sv_ball_powershot_strength.GetFloat() * (1 + GetPowershotModifier()) * modifier;
		EmitSound("Ball.kickhard");
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KICK);
	}
	else
	{
		//float modifier = 1 - (abs((m_vPlForward.z + 0.5f) * 0.5f - 0.5f));
		shotStrength = sv_ball_normalshot_strength.GetFloat() * modifier;		//do normal kick instead
		//if (shotStrength < 500)
		//{
		EmitSound("Ball.touch");
		//}
		//else
		//{
		//	EmitSound("Ball.kicknormal");
		//	m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KICK, true);
		//}
	}

	QAngle shotAngle = m_aPlAng;
	shotAngle[PITCH] = min(-5, m_aPlAng[PITCH]);

	Vector shotDir;
	AngleVectors(shotAngle, &shotDir);

	SetVel(shotDir * shotStrength);

	//SetBallCurve(shotStrength == 0);
	SetBallSpin();

	Kicked(BODY_FEET);

	return true;
}

bool CBall::DoVolleyShot()
{
	if (!m_bIsPowershot || m_vPlVel.Length2D() > 50 || m_pPl->GetGroundEntity())
		return false;

	//Vector dirToBall = m_vPos - m_vPlPos;
	//Vector localDirToBall;
	//VectorIRotate(dirToBall, m_pPl->EntityToWorldTransform(), localDirToBall);

	//if (abs(localDirToBall.x) < 10 || abs(localDirToBall.x) > 50)
	//	return false;

	SetVel(m_vPlForward * 1.5f * sv_ball_powershot_strength.GetFloat() * (1 + GetPowershotModifier()) * GetPitchModifier());

	SetBallSpin();

	EmitSound("Ball.kickhard");
	m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_VOLLEY);

	Kicked(BODY_FEET);

	return true;
}

bool CBall::DoChestDrop()
{
	SetVel(m_vPlForward * sv_ball_normalshot_strength.GetFloat() * 0.25f);
	EmitSound("Ball.kicknormal");
	Kicked(BODY_CHEST);

	return true;
}

bool CBall::DoHeader()
{
	if (m_bIsPowershot && 
		m_vPlVel.Length2D() > 0 &&// mp_runspeed.GetInt() - FLT_EPSILON &&
		m_pPl->GetTeamPosition() > 1 &&
		m_nInPenBoxOfTeam == m_pPl->GetOppTeamNumber() &&
		m_pPl->m_flNextSlide <= gpGlobals->curtime)
	{
		SetVel(m_vPlForward * (sv_ball_powershot_strength.GetFloat() * 0.25f + m_vPlVel.Length()) * (1 + GetPowershotModifier()) * GetPitchModifier());
		//SetVel(m_vPlForward * (sv_ball_powershot_strength.GetFloat() * 1.5f + m_vPlVel.Length()));
		EmitSound("Ball.kickhard");
		m_pPl->AddFlag(FL_FREECAM);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_DIVINGHEADER);
	}
	else if (m_bIsPowershot)
	{
		SetVel(m_vPlForward * (sv_ball_powershot_strength.GetFloat() * 0.25f + m_vPlVel.Length()) * (1 + GetPowershotModifier()) * GetPitchModifier());
		EmitSound("Ball.kickhard");
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HEADER);
	}
	else
	{
		SetVel(m_vPlForward * (sv_ball_normalshot_strength.GetFloat() * 0.75f + m_vPlVel.Length()) * GetPitchModifier());
		EmitSound("Ball.kicknormal");
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HEADER_STATIONARY);
	}
	
	Kicked(BODY_HEAD);

	return true;
}

void CBall::SetBallSpin()
{
	Vector rot(0, 0, 0);

	if (m_pPl->m_nButtons & IN_MOVELEFT) 
		rot += Vector(0, 0, 1);
	else if (m_pPl->m_nButtons & IN_MOVERIGHT) 
		rot += Vector(0, 0, -1);

	if (m_pPl->m_nButtons & IN_TOPSPIN)
		rot += -m_vPlRight * 0.75f;
	else if (m_pPl->m_nButtons & IN_BACKSPIN)
		rot += m_vPlRight * 0.75f;

	if (rot.z != 0)
		rot.NormalizeInPlace();
	//float spin = min(1, m_vVel.Length() / sv_ball_maxspin.GetInt()) * sv_ball_spin.GetFloat();
	float spin = m_vVel.Length() * sv_ball_spin.GetInt() / 100.0f;

	AngularImpulse randRot = AngularImpulse(0, 0, 0);
	//if (rot.Length() == 0)
	{
		for (int i = 0; i < 3; i++)
		{
			//randRot[i] = m_vVel.Length() * sv_ball_defaultspin.GetInt() / 100.0f * (g_IOSRand.RandomInt(0, 1) == 1 ? 1 : -1);
			randRot[i] = m_vVel.Length() * g_IOSRand.RandomFloat(-sv_ball_defaultspin.GetInt(), sv_ball_defaultspin.GetInt()) / 100.0f;
		}
	}

	SetRot(WorldToLocalRotation(SetupMatrixAngles(m_aAng), rot, spin) + randRot);
}

void CBall::BallThink( void	)
{
	SetNextThink(gpGlobals->curtime + 0.01f);

	State_Think();
}

void CBall::TriggerGoal(int team)
{
	if (SDKGameRules()->State_Get() == MATCH_PENALTIES && team != m_nFoulingTeam)
		return;

	m_nTeam = team;
	State_Transition(BALL_GOAL, sv_ball_statetransitiondelay.GetFloat());
}

void CBall::TriggerGoalLine(int team)
{
	if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
		return;

	m_pPhys->GetPosition(&m_vTriggerTouchPos, NULL);

	if (LastTeam(false) == team)
		State_Transition(BALL_CORNER, sv_ball_statetransitiondelay.GetFloat());
	else
		State_Transition(BALL_GOALKICK, sv_ball_statetransitiondelay.GetFloat());
}

void CBall::TriggerSideline()
{
	if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
		return;

	if (m_eBallState == BALL_THROWIN || m_eNextState == BALL_THROWIN)
		return;

	Vector ballPos;
	m_pPhys->GetPosition(&ballPos, NULL);
	CBaseEntity *pThrowIn = gEntList.FindEntityByClassnameNearest("info_throw_in", ballPos, 9999);
	if (!pThrowIn)
		return;

	m_vTriggerTouchPos = pThrowIn->GetLocalOrigin();
	State_Transition(BALL_THROWIN, sv_ball_statetransitiondelay.GetFloat());
}

void CBall::TriggerPenaltyBox(int team)
{
	m_nInPenBoxOfTeam = team;
}

void CBall::UpdateCarrier()
{
	if (CSDKPlayer::IsOnField(m_pPl))
	{
		m_vPlPos = m_pPl->GetLocalOrigin();
		m_vPlVel = m_pPl->GetLocalVelocity();
		m_aPlAng = m_pPl->EyeAngles();
		AngleVectors(m_aPlAng, &m_vPlForward, &m_vPlRight, &m_vPlUp);
		m_vPlForward2D = m_vPlForward;
		m_vPlForward2D.z = 0;
		m_vPlForward2D.NormalizeInPlace();
		m_nPlTeam = m_pPl->GetTeamNumber();
		m_nPlPos = m_pPl->GetTeamPosition();
		m_bIsPowershot = (m_pPl->m_nButtons & (IN_ATTACK2 | IN_ALT1)) != 0;
	}
	else
	{
		m_nPlTeam = TEAM_INVALID;
		m_nPlPos = 0;
	}
}

void CBall::MarkOffsidePlayers()
{
	m_vOffsidePos = m_vPos;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (CSDKPlayer::IsOnField(pPl))
			pPl->SetOffside(false);

		if (!(CSDKPlayer::IsOnField(pPl) && pPl != m_pPl && pPl->GetTeamNumber() == LastTeam(true)))
			continue;

		Vector pos = pPl->GetLocalOrigin();
		int forward = pPl->GetTeam()->m_nForward;

		// In opponent half?
		if (Sign((pos - SDKGameRules()->m_vKickOff).y) != forward)
			continue;

		// Closer to goal than the ball?
		if (Sign((pos - m_vPos).y) != forward)
			continue;

		int nearerPlayerCount = 0;

		// Count players who are nearer to goal
		for (int j = 1; j <= gpGlobals->maxClients; j++)
		{
			CSDKPlayer *pOpp = ToSDKPlayer(UTIL_PlayerByIndex(j));
			if (!(pOpp && pOpp->GetTeamNumber() == pPl->GetOppTeamNumber()))
				continue;

			if (Sign((pOpp->GetLocalOrigin() - pos).y) == forward)
				nearerPlayerCount += 1;
		}

		if (nearerPlayerCount <= 1)
			pPl->SetOffside(true);
	}
}

void CBall::UnmarkOffsidePlayers()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (CSDKPlayer::IsOnField(pPl))
			pPl->SetOffside(false);
	}
}

void CBall::Kicked(body_part_t bodyPart)
{
	m_pPl->m_flNextShot = gpGlobals->curtime + sv_ball_shotdelay.GetFloat();
	Touched(m_pPl, true, bodyPart);
}

void CBall::Touched(CSDKPlayer *pPl, bool isShot, body_part_t bodyPart)
{
	if (SDKGameRules()->IsIntermissionState() || GetOwnerEntity())
		return;

	//DevMsg("Touched %0.2f\n", gpGlobals->curtime);
	if (m_Touches.Count() > 0 && m_Touches.Tail().m_pPl == pPl && m_Touches.Tail().m_nTeam == pPl->GetTeamNumber())
	{
		if (sv_ball_doubletouchfouls.GetBool() && m_Touches.Tail().m_eBallState != BALL_NORMAL)
		{
			pPl->m_Fouls += 1;
			TriggerFoul(FOUL_DOUBLETOUCH, pPl->GetLocalOrigin(), pPl);
			State_Transition(BALL_FREEKICK, sv_ball_statetransitiondelay.GetFloat());
			return;
		}
		else
		{
			m_Touches.Tail().m_bIsShot = isShot;
			m_Touches.Tail().m_eBodyPart = bodyPart;
			m_Touches.Tail().m_eBallState = m_pCurStateInfo->m_eBallState;
		}
	}
	else
	{
		UpdatePossession(pPl);
		BallTouchInfo info = { pPl, pPl->GetTeamNumber(), isShot, bodyPart, m_pCurStateInfo->m_eBallState };
		m_Touches.AddToTail(info);
	}
	
	if (pPl->IsOffside())
	{
		pPl->m_Offsides += 1;
		TriggerFoul(FOUL_OFFSIDE, pPl->GetOffsidePos(), pPl);
		EnableOffsideLine(m_vOffsidePos.y, pPl->GetOffsidePos().y);
		State_Transition(BALL_FREEKICK, sv_ball_statetransitiondelay.GetFloat());
	}
}

BallTouchInfo *CBall::LastInfo(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/)
{
	for (int i = m_Touches.Count() - 1; i >= 0; i--)
	{
		if (pSkipPl && m_Touches[i].m_pPl == pSkipPl)
			continue;

		if (!wasShooting || m_Touches[i].m_bIsShot)
			return &m_Touches[i];
	}

	return NULL;
}

CSDKPlayer *CBall::LastPl(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/)
{
	BallTouchInfo *info = LastInfo(wasShooting, pSkipPl);
	if (info && CSDKPlayer::IsOnField(info->m_pPl))
		return info->m_pPl;
	
	return NULL;
}

int CBall::LastTeam(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/)
{
	BallTouchInfo *info = LastInfo(wasShooting, pSkipPl);
	return info ? info->m_nTeam : TEAM_INVALID;
}

int CBall::LastOppTeam(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/)
{
	BallTouchInfo *info = LastInfo(wasShooting, pSkipPl);
	return info ? (info->m_nTeam == TEAM_A ? TEAM_B : TEAM_A) : TEAM_INVALID;
}

void CBall::UpdatePossession(CSDKPlayer *pNewPossessor)
{
	if (m_pPossessingPl == pNewPossessor)
		return;

	if (m_flPossessionStart != -1)
	{
		float duration = gpGlobals->curtime - m_flPossessionStart;
		CTeam *pPossessingTeam = GetGlobalTeam(m_nPossessingTeam);
		CTeam *pOtherTeam = GetGlobalTeam(m_nPossessingTeam == TEAM_A ? TEAM_B : TEAM_A);
		pPossessingTeam->m_flPossessionTime += duration;
		float total = max(1, pPossessingTeam->m_flPossessionTime + pOtherTeam->m_flPossessionTime);
		pPossessingTeam->m_nPossession = 100 * pPossessingTeam->m_flPossessionTime / total;		
		pOtherTeam->m_nPossession = 100 - pPossessingTeam->m_nPossession;

		if (CSDKPlayer::IsOnField(m_pPossessingPl))
		{
			m_pPossessingPl->m_flPossessionTime += duration;
			m_pPossessingPl->m_Possession = 100 * m_pPossessingPl->m_flPossessionTime / total;
		}
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
		m_nPossessingTeam = TEAM_INVALID;
		m_flPossessionStart = -1;
	}
}

void CBall::EnableOffsideLine(float ballPosY, float playerPosY)
{
	m_flOffsideLineBallY = ballPosY;
	m_flOffsideLinePlayerY = playerPosY;
	m_bShowOffsideLine = true;
}

void CBall::DisableOffsideLine()
{
	m_bShowOffsideLine = false;
}

int CBall::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CBall::ResetStats()
{
	m_Touches.RemoveAll();
	m_ePenaltyState = PENALTY_NONE;

	GetGlobalTeam(TEAM_A)->ResetStats();
	GetGlobalTeam(TEAM_B)->ResetStats();

	for (int i = 1; i < gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		pPl->ResetStats();
	}
}

void CBall::SetPenaltyTaker(CSDKPlayer *pPl)
{
	m_pFouledPl = pPl;
	m_nFoulingTeam = pPl->GetOppTeamNumber();
}