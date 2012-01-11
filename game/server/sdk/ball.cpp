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

static ConVar sv_ball_mass( "sv_ball_mass", "1", FCVAR_ARCHIVE );
static ConVar sv_ball_inertia( "sv_ball_inertia", "1.0", FCVAR_ARCHIVE );
static ConVar sv_ball_rotinertialimit( "sv_ball_rotinertialimit", "0.05", FCVAR_ARCHIVE );
static ConVar sv_ball_damping( "sv_ball_damping", "0.3", FCVAR_ARCHIVE );
static ConVar sv_ball_rotdamping( "sv_ball_rotdamping", "0.3", FCVAR_ARCHIVE );
static ConVar sv_ball_dragcoefficient( "sv_ball_dragcoefficient", "0.47", FCVAR_ARCHIVE );
static ConVar sv_ball_angdragcoefficient( "sv_ball_angdragcoefficient", "0.47", FCVAR_ARCHIVE );
//static ConVar sv_ball_elasticity( "sv_ball_elasticity", "65", FCVAR_ARCHIVE );
//static ConVar sv_ball_friction( "sv_ball_friction", "1", FCVAR_ARCHIVE );
//static ConVar sv_ball_speed( "sv_ball_speed", "1500", FCVAR_ARCHIVE );
static ConVar sv_ball_spin( "sv_ball_spin", "3000", FCVAR_ARCHIVE );
static ConVar sv_ball_maxspin( "sv_ball_maxspin", "1000", FCVAR_ARCHIVE );
static ConVar sv_ball_curve("sv_ball_curve", "250", FCVAR_ARCHIVE);
static ConVar sv_ball_touchcone( "sv_ball_touchcone", "90", FCVAR_ARCHIVE );
static ConVar sv_ball_touchradius( "sv_ball_touchradius", "80", FCVAR_ARCHIVE );
//static ConVar sv_ball_a1_speed( "sv_ball_a1_speed", "500", FCVAR_ARCHIVE );
//static ConVar sv_ball_a1_zoffset( "sv_ball_a1_zoffset", "10", FCVAR_ARCHIVE );
static ConVar sv_ball_radius( "sv_ball_radius", "5.2", FCVAR_ARCHIVE );
//static ConVar sv_ball_gravity( "sv_ball_gravity", "10", FCVAR_ARCHIVE );

static ConVar sv_ball_normalshot_strength("sv_ball_normalshot_strength", "550", FCVAR_ARCHIVE);
static ConVar sv_ball_powershot_strength("sv_ball_powershot_strength", "650", FCVAR_ARCHIVE);
static ConVar sv_ball_keepershot_strength("sv_ball_keepershot_strength", "100", FCVAR_ARCHIVE);

void CC_SendMatchEvent(const CCommand &args)
{
	GetBall()->SendMatchEvent((match_event_t)atoi(args[1]));
}

static ConCommand mp_send_match_event("mp_send_match_event", CC_SendMatchEvent);

void OnTriggerSolidChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	CBaseTrigger *penBox = (CBaseTrigger *)gEntList.FindEntityByClassname(NULL, "trigger_PenaltyBox");
	if (!penBox)
		return;

	if (((ConVar *)var)->GetBool())
		penBox->SetSolid(SOLID_BBOX);
	else
		penBox->SetSolid(SOLID_NONE);

}
static ConVar mp_ball_pushaway("mp_ball_pushaway", "0", FCVAR_REPLICATED|FCVAR_NOTIFY, "Push ball away on touch", &OnTriggerSolidChange);

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

#define SHOT_DELAY 0.5f

CBall::CBall()
{
	g_pBall = this;
	m_eNextState = BALL_NOSTATE;
	m_flStateLeaveTime = gpGlobals->curtime;
	m_bIgnoreTriggers = false;
	m_eBodyPart = BODY_NONE;
	m_eFoulType = FOUL_NONE;
	m_bIsRemoteControlled = false;
	m_bFreeze = false;
}

CBall::~CBall()
{
}

//==========================================================
//	
//	
//==========================================================
void CBall::Spawn (void)
{
	//RomD: Don't fade the ball
	SetFadeDistance(-1, 0);
	DisableAutoFade();

	//VPHYSICS from	CombineBall
	PrecacheModel( ENTITY_MODEL	);
	SetModel( ENTITY_MODEL );

	CreateVPhysics();

	SetThink (&CBall::BallThink);
	SetNextThink( gpGlobals->curtime + 0.01f );

	m_nBody = 0; 
	m_nSkin = g_IOSRand.RandomInt(0,5);
	g_vKickOffSpot = GetLocalOrigin();
	m_pPhys->SetPosition(GetLocalOrigin(), GetLocalAngles(), true);

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
	m_pPhys->EnableGravity(	true );
	m_pPhys->EnableDrag( false );
	float flDamping	= sv_ball_damping.GetFloat(); //0.0f
	float flAngDamping = sv_ball_rotdamping.GetFloat(); //2.5f
	m_pPhys->SetDamping( &flDamping, &flAngDamping );
//	VPhysicsGetObject()->SetInertia( Vector( 0.0023225760f,	0.0023225760f, 0.0023225760f ) );
	SetPhysicsMode(PHYSICS_MULTIPLAYER_SOLID);
	SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );

	m_pPhys->Wake();

	return true;
}

static ConVar sv_replay_duration("sv_replay_duration", "10");

void cc_StartReplay(const CCommand &args)
{
	GetBall()->StartReplay();
}

static ConCommand start_replay("start_replay", cc_StartReplay);
//static ConCommand start_replay("stop_replay", cc_StopReplay);

void CBall::VPhysicsUpdate(IPhysicsObject *pPhysics)
{
	if (m_bDoReplay)
		RestoreReplaySnapshot();
	else if (sv_replay_duration.GetInt() > 0)
		TakeReplaySnapshot();

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

	BaseClass::VPhysicsUpdate(pPhysics);
}

void CBall::TakeReplaySnapshot()
{
	// remove snapshots which are too old
	//while (m_History.Count() > 0 && gpGlobals->curtime - m_History.Head().snapTime > 10)
	//	m_History.Remove(0);

	Vector pos, vel;
	QAngle ang;
	AngularImpulse angImp;

	m_pPhys->GetPosition(&pos, &ang);
	m_pPhys->GetVelocity(&vel, &angImp);

	m_History.AddToTail(BallHistory(gpGlobals->curtime, pos, ang, vel, angImp));
	
	if (m_History.Count() > sv_replay_duration.GetInt() * (1.0f / TICK_INTERVAL))
	{
		m_History.Remove(0);
	}
}

void CBall::StartReplay()
{
	/*for (int i = 0; i < m_History.Count(); i++)
	{
		if (abs((gpGlobals->curtime - m_History[i].snapTime) - duration) > 0.1f)
			m_History.Remove(0);
		else
			break;
	}*/

	if (m_History.Count() > 0)
	{
		m_nReplaySnapshotIndex = 0;
		m_bDoReplay = true;
	}
}

void CBall::RestoreReplaySnapshot()
{
	if (m_History.Count() > 0 && m_nReplaySnapshotIndex < m_History.Count())
	{
		BallHistory history = m_History[m_nReplaySnapshotIndex];
		m_nReplaySnapshotIndex += 1;
		//m_History.Remove(0);
		m_pPhys->SetPosition(history.pos, history.ang, false);
		m_pPhys->SetVelocity(&history.vel, &history.angImp);
	}
	else
	{
		m_bDoReplay = false;
	}
}

//==========================================================
//	
//	
//==========================================================
CBaseEntity	*CreateBall( const Vector &origin )
{
	CBall *pBall = static_cast<CBall*>(	CreateEntityByName(	"football" ) );
	pBall->SetAbsOrigin( origin	+ Vector (0.0f,	0.0f, 0.0f) );
	pBall->SetOwnerEntity( NULL	);
	pBall->SetAbsVelocity( Vector (0.0f, 0.0f, 0.0f) );
	pBall->SetModelName( MAKE_STRING("models/w_fb.mdl" ) );
	pBall->Spawn();
	return pBall;
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
		if (flSpeed > 900.0f)
			pOther->EmitSound ("Player.Oomph");

		//KickedBall((CSDKPlayer*)pOther, false);		//a touch
		m_LastTouch = ToSDKPlayer(pOther);
		if (m_LastTouch->IsOffside())
		{
			m_eFoulType = FOUL_OFFSIDE;
			m_pFoulingPl = m_LastTouch;
			State_Transition(BALL_FREEKICK);
		}

		EmitSound("Ball.touch");
	}

	//Warning ("surfaceprops index %d\n", surfaceProps);

	BaseClass::VPhysicsCollision( index, pEvent );
}

void CBall::SendMatchEvent(match_event_t matchEvent, CSDKPlayer *pPlayer1, CSDKPlayer *pPlayer2)
{
	if (pPlayer1)
	{
		UTIL_LogPrintf( "\"%s<%d><%s><%s>\" triggered \"%d\"\n",
			pPlayer1->GetPlayerName(), pPlayer1->GetUserID(),
			pPlayer1->GetNetworkIDString(), pPlayer1->GetTeam()->GetName(), matchEvent);
	}

	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin(filter, "MatchEvent");
		WRITE_BYTE(matchEvent);
		WRITE_BYTE(pPlayer1 ? pPlayer1->entindex() : 0);
		WRITE_BYTE(pPlayer2 ? pPlayer2->entindex() : 0);
	MessageEnd();
}

CSDKPlayer *CBall::FindNearestPlayer(int nTeam /*= TEAM_INVALID*/, bool ignoreKeepers /*= true*/)
{
	CSDKPlayer *pNearest = NULL;
	float shortestDist = FLT_MAX;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPlayer = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!(
			pPlayer &&
			pPlayer->GetTeamNumber() != TEAM_SPECTATOR &&
			pPlayer->IsAlive() &&
			(nTeam == TEAM_INVALID || pPlayer->GetTeamNumber() == nTeam) &&
			(!ignoreKeepers || pPlayer->GetTeamPosition() != 1)
			))
			continue;

		Vector dir = m_vPos - pPlayer->GetLocalOrigin();
		float xyDist = dir.Length2D();
		float zDist = m_vPos.z - (pPlayer->GetLocalOrigin().z + SDKGameRules()->GetViewVectors()->m_vHullMax.z); //pPlayer->GetPlayerMaxs().z);// 
		float dist = max(xyDist, zDist);

		shortestDist = dist;
		pNearest = pPlayer;	
	}

	return pNearest;
}

void CBall::SetPos(const Vector &pos)
{
	//m_pPhys->SetVelocityInstantaneous(&vec3_origin, &vec3_origin);
	//m_pPhys->SetPosition(pos, NULL, true);
	m_vPos = pos;
	m_vPos.z += m_flPhysRadius;
	m_vVel = vec3_origin;
	m_vAngImp = vec3_origin;
}

void CBall::PreStateHook()
{
	m_pPhys->GetPosition(&m_vPos, &m_aAng);
	m_pPhys->GetVelocity(&m_vVel, &m_vAngImp);
	UpdateCarrier();
}

void CBall::PostStateHook()
{
	if (!m_bFreeze)
		m_pPhys->EnableMotion(true);

	if (m_vVel == vec3_origin)
	{
		AddEffects(EF_NOINTERP);
		m_pPhys->SetPosition(m_vPos, m_aAng, true);
		//SetLocalOrigin(m_vNewPos);
		//m_pPhys->SetVelocity(&m_vNewVel, &m_vNewAngImp);
		m_pPhys->SetVelocityInstantaneous(&m_vVel, &m_vAngImp);
		//SetLocalVelocity(m_vNewVel);
		//VPhysicsUpdate(m_pPhys);
		RemoveEffects(EF_NOINTERP);
	}
	else
	{
		m_pPhys->SetVelocity(&m_vVel, &m_vAngImp);
	}

	if (m_bFreeze)
		m_pPhys->EnableMotion(false);
}

ConVar mp_showballstatetransitions( "mp_showballstatetransitions", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show ball state transitions." );

void CBall::State_Transition( ball_state_t newState, float delay /*= 0.0f*/ )
{
	m_eNextState = newState;
	m_flStateLeaveTime = gpGlobals->curtime + delay;
}

void CBall::State_DoTransition( ball_state_t newState )
{
	m_eNextState = BALL_NOSTATE;
	State_Leave();
	State_Enter( newState );
}

void CBall::State_Enter( ball_state_t newState )
{
	m_eBallState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );
	m_flStateEnterTime = gpGlobals->curtime;

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

void CBall::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}

void CBall::State_Think()
{
	PreStateHook();
	while (m_eNextState != BALL_NOSTATE && m_flStateLeaveTime <= gpGlobals->curtime)
	{
		State_DoTransition(m_eNextState);
	}
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnThink )
	{	
		(this->*m_pCurStateInfo->pfnThink)();
	}
	PostStateHook();
}

CBallStateInfo* CBall::State_LookupInfo( ball_state_t state )
{
	static CBallStateInfo ballStateInfos[] =
	{
		{ BALL_NORMAL,		"BALL_NORMAL",		&CBall::State_NORMAL_Enter,		NULL,							&CBall::State_NORMAL_Think },
		{ BALL_KICKOFF,		"BALL_KICKOFF",		&CBall::State_KICKOFF_Enter,	&CBall::State_KICKOFF_Leave,	&CBall::State_KICKOFF_Think },
		{ BALL_THROWIN,		"BALL_THROWIN",		&CBall::State_THROWIN_Enter,	&CBall::State_THROWIN_Leave,	&CBall::State_THROWIN_Think },
		{ BALL_GOALKICK,	"BALL_GOALKICK",	&CBall::State_GOALKICK_Enter,	&CBall::State_GOALKICK_Leave,	&CBall::State_GOALKICK_Think },
		{ BALL_CORNER,		"BALL_CORNER",		&CBall::State_CORNER_Enter,		&CBall::State_CORNER_Leave,		&CBall::State_CORNER_Think },
		{ BALL_GOAL,		"BALL_GOAL",		&CBall::State_GOAL_Enter,		&CBall::State_GOAL_Leave,		&CBall::State_GOAL_Think },
		{ BALL_FREEKICK,	"BALL_FREEKICK",	&CBall::State_FREEKICK_Enter,	&CBall::State_FREEKICK_Leave,	&CBall::State_FREEKICK_Think },
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
	if (!SetCarrier(FindEligibleCarrier()))
		return;

	if (!DoBodyPartAction())
		return;

	MarkOffsidePlayers();

	if (m_pPl->IsOffside())
	{
		m_eFoulType = FOUL_OFFSIDE;
		m_pFoulingPl = m_pPl;
		State_Transition(BALL_FREEKICK);
	}
}

void CBall::State_KICKOFF_Enter()
{
	if (!SetCarrier(FindNearestPlayer(m_LastPlayer ? m_LastPlayer->GetOppTeamNumber() : TEAM_INVALID)))
	{
		State_Transition(BALL_NORMAL);
		return;
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!(pPl && pPl->GetTeamNumber() >= TEAM_A))
			continue;

		//SDKGameRules()->GetPlayerSpawnSpot(pPlayer);
		//pPlayer->RemoveFlag(FL_ATCONTROLS);
		Vector pos;

		if (pPl == m_pPl)
		{
			pos = g_vKickOffSpot;
			pos.x -= 30;
		}
		else
		{
			pos = GetOwnTeamSpots(pPl)->m_vPlayers[pPl->GetTeamPosition() - 1];
		}

		pPl->WalkToPosition(pos, PLAYER_SPRINTSPEED, 10);
		m_bIsRemoteControlled = true;
	}

	SetPos(g_vKickOffSpot);
	m_bFreeze = true;

	//m_pPl->WalkToPosition(g_vKickOffSpot, m_pPl->m_Shared.m_flSprintSpeed, 50);
	//EnableShield(true, g_vKickOffSpot, Vector(100, 0, 0));
	//SendMatchEvent(MATCH_EVENT_KICKOFF, m_pPl, m_pPl);
}

void CBall::State_KICKOFF_Think()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!(pPl && pPl->GetTeamNumber() >= TEAM_A))
			continue;

		// Wait until everyone has reached his target position
		if (pPl->GetFlags() & FL_REMOTECONTROLLED)
			return;
	}

	if (m_bIsRemoteControlled)
	{
		//m_pPl->m_HoldAnimTime = -1;
		m_pPl->AddFlag(FL_ATCONTROLS);
		//m_pPl->SetLocalOrigin(Vector(g_vKickOffSpot.x - 30, g_vKickOffSpot.y, g_vKickOffSpot.z));
		//m_pPl->SnapEyeAngles(QAngle(0, 0, 0));
		EmitSound("Ball.whistle");
		SendMatchEvent(MATCH_EVENT_KICKOFF);
		m_bIsRemoteControlled = false;
	}

	if (m_pPl->m_nButtons & (IN_ATTACK | IN_ATTACK2))
	{
		m_vVel = m_vPlForward * 100;
		Kicked(BODY_FEET);
		m_bFreeze = false;
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_KICKOFF_Leave()
{
	if (m_pPl)
		m_pPl->RemoveFlag(FL_ATCONTROLS);
}

void CBall::State_THROWIN_Enter()
{
	if (!SetCarrier(FindNearestPlayer(m_LastTouch->GetTeamNumber() == TEAM_A ? TEAM_B : TEAM_A)))
	{ 
		State_Transition(BALL_NORMAL);
		return;
	}

	UnmarkOffsidePlayers();

	int sign = (g_vKickOffSpot - m_vPos).x > 0 ? 1 : -1;
	m_vPos.z = g_flGroundZ;
	m_vPos.x -= 30 * sign;
	//m_pPl->SetLocalOrigin(m_vNewPos);
	m_pPl->WalkToPosition(m_vPos, PLAYER_SPRINTSPEED, 5);
	m_bIsRemoteControlled = true;

	SetPos(m_vPos + Vector(0, 0, VEC_HULL_MAX.z + 2));
	SDKGameRules()->EnableCircShield(m_nPlTeam, 360, m_vPos);
	//m_pPhys->EnableGravity(false);
	m_bFreeze = true;
	m_bIgnoreTriggers = true;

	//m_pPl->m_HoldAnimTime = gpGlobals->curtime + BALL_STATUS_TIME;	//hold player still
	EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");
	SendMatchEvent(MATCH_EVENT_THROWIN);
}

void CBall::State_THROWIN_Think()
{
	if (m_pPl->GetFlags() & FL_REMOTECONTROLLED)
		return;

	if (m_bIsRemoteControlled)
	{
		int sign = (g_vKickOffSpot - m_vPos).x > 0 ? 1 : -1;
		//m_pPl->SnapEyeAngles(QAngle(0, 180 * sign, 0));
		m_pPl->AddFlag(FL_ATCONTROLS);
		//m_pPl->SetLocalVelocity(vec3_origin);
		m_pPl->m_HoldAnimTime = -1;
		m_pPl->SetAnimation(PLAYER_THROWIN);
		m_pPl->DoAnimationEvent(PLAYERANIMEVENT_THROWIN);
		m_bIsRemoteControlled = false;
	}

	if (m_pPl->m_nButtons & (IN_ATTACK | IN_ATTACK2))
	{
		if (m_pPl->m_nButtons & IN_ATTACK)
		{
			m_vVel = m_pPl->EyeDirection3D() * 250;
		}
		else
		{
			m_vVel = m_pPl->EyeDirection3D() * (250 + 500 * GetPowershotModifier() * GetPitchModifier());
		}

		m_pPl->m_flNextShot = gpGlobals->curtime + SHOT_DELAY;
		m_pPl->m_HoldAnimTime = gpGlobals->curtime + 1;
		m_pPl->SetAnimation(PLAYER_THROW);
		m_pPl->DoAnimationEvent(PLAYERANIMEVENT_THROW);
		//m_pPhys->EnableGravity(true);
		m_bFreeze = false;
		State_Transition(BALL_NORMAL);
	}
	else
	{
		//m_aNewAng = m_pPl->GetLocalAngles();
	}
}

void CBall::State_THROWIN_Leave()
{
	SDKGameRules()->DisableShields();
	m_bIgnoreTriggers = false;
	//m_pPl->RemoveFlag(FL_ATCONTROLS);
}

void CBall::State_GOALKICK_Enter()
{
	CSDKPlayer *pKeeper = NULL;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPlayer = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!(pPlayer &&
			pPlayer->GetTeamNumber() != TEAM_SPECTATOR &&
			pPlayer->IsAlive() &&
			pPlayer->GetTeamNumber() != m_LastTouch->GetTeamNumber() &&
			pPlayer->m_TeamPos == 1))
			continue;

		pKeeper = pPlayer;
		break;
	}

	if (!SetCarrier(pKeeper))
	{ 
		State_Transition(BALL_NORMAL);
		return;
	}

	Vector ballPos;
	if ((m_vPos - GetOwnTeamSpots(m_pPl)->m_vPlayers[0]).x > 0)
		ballPos = GetOwnTeamSpots(m_pPl)->m_vGoalkickLeft;
	else
		ballPos = GetOwnTeamSpots(m_pPl)->m_vGoalkickRight;

	int ySign = (ballPos - GetOwnTeamSpots(m_pPl)->m_vPlayers[0] ).y > 0 ? 1 : -1;
	m_pPl->SetLocalOrigin(Vector(ballPos.x, ballPos.y - 200 * ySign, ballPos.z));
	m_pPl->SnapEyeAngles(QAngle(0, 90 * ySign, 0));
	Vector penPos = g_pTeamSpots[m_pPl->GetTeamNumber() - TEAM_A]->m_vPenalty;
	CBaseEntity *pPenBox = gEntList.FindEntityByClassnameNearest("trigger_PenaltyBox", penPos, 99999);
	SDKGameRules()->EnableCircShield(m_pPl->GetTeamNumber(), 360, penPos);
	//SDKGameRules()->EnableRectShield(teamFlag, Vector(penPos.x - 850, penPos.y - 300, 0), Vector(penPos.x + 850, penPos.y + 300, 0));
	Vector min, max;
	//modelinfo->GetModelBounds(pPenBox->GetModel(), min, max);
	pPenBox->CollisionProp()->WorldSpaceTriggerBounds( &min, &max );
	SDKGameRules()->EnableRectShield(m_pPl->GetTeamNumber(), min, max);
	SetPos(ballPos);

	EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");
	SendMatchEvent(MATCH_EVENT_GOALKICK);
}

void CBall::State_GOALKICK_Think()
{
	if (m_pPl->m_nButtons & (IN_ATTACK | IN_ATTACK2))
	{
		if (DoGroundShot())
		{
			MarkOffsidePlayers();
			State_Transition(BALL_NORMAL);
		}
	}
}

void CBall::State_GOALKICK_Leave()
{
	SDKGameRules()->DisableShields();
	//m_pPl->RemoveFlag(FL_ATCONTROLS);
}

void CBall::State_CORNER_Enter()
{
	if (!SetCarrier(FindNearestPlayer(m_LastTouch->GetTeamNumber() == TEAM_A ? TEAM_B : TEAM_A)))
	{
		State_Transition(BALL_NORMAL);
		return;
	}

	UnmarkOffsidePlayers();

	Vector ballPos;
	if ((m_vPos - GetOppTeamSpots(m_pPl)->m_vPlayers[0]).x > 0)
		ballPos = GetOppTeamSpots(m_pPl)->m_vCornerLeft;
	else
		ballPos = GetOppTeamSpots(m_pPl)->m_vCornerRight;

	int xSign = (g_vKickOffSpot - ballPos).x > 0 ? 1 : -1;
	int ySign = (g_vKickOffSpot - ballPos).y > 0 ? 1 : -1;
	//m_pPl->SetLocalOrigin(Vector(ballPos.x - 50 * xSign, ballPos.y - 50 * ySign, ballPos.z));
	//m_pPl->SnapEyeAngles(QAngle(0, 45 * xSign * ySign, 0));
	m_pPl->WalkToPosition(Vector(ballPos.x - 50 * xSign, ballPos.y - 50 * ySign, g_flGroundZ), PLAYER_SPRINTSPEED, 5);
	m_bIsRemoteControlled = true;
	
	SDKGameRules()->EnableCircShield(m_nPlTeam, 360, ballPos);
	SetPos(ballPos);
	m_bFreeze = true;

	EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");
	SendMatchEvent(MATCH_EVENT_CORNER);
}

void CBall::State_CORNER_Think()
{
	if (m_pPl->GetFlags() & FL_REMOTECONTROLLED)
		return;

	if (m_bIsRemoteControlled)
	{
		m_bIsRemoteControlled = false;
	}

	if (m_pPl->m_nButtons & (IN_ATTACK | IN_ATTACK2))
	{
		if (DoGroundShot())
		{
			m_bFreeze = false;
			State_Transition(BALL_NORMAL);
		}
	}
}

void CBall::State_CORNER_Leave()
{
	SDKGameRules()->DisableShields();
}

void CBall::State_GOAL_Enter()
{
	if (!SetCarrier(m_LastPlayer))
	{
		State_Transition(BALL_NORMAL);
		return;
	}

	m_bIgnoreTriggers = true;

	bool ownGoal = false;

	if (m_pPl->GetTeamNumber() == m_team)
	{
		//give scorer a goal
		m_pPl->IncrementFragCount( 1 );
		m_pPl->AddPointsToTeam( 1, false );
	}
	else
	{
		ownGoal = true;
		//add points directly to other team...
		GetGlobalTeam( m_pPl->GetOppTeamNumber() )->AddScore( 1 );
	}

	//EnableCeleb();
	EmitSound("Ball.whistle");
	SendMatchEvent(MATCH_EVENT_GOAL);
	State_Transition(BALL_KICKOFF, 5);
}

void CBall::State_GOAL_Think()
{
}

void CBall::State_GOAL_Leave()
{
	m_bIgnoreTriggers = false;
	//DisableCeleb();
}

void CBall::State_FREEKICK_Enter()
{
	if (!SetCarrier(FindNearestPlayer(m_pFoulingPl->GetTeamNumber())))
	{
		State_Transition(BALL_NORMAL);
		return;
	}

	m_pPl->WalkToPosition(m_vPos, PLAYER_SPRINTSPEED, 50);
	m_bIsRemoteControlled = true;
}

void CBall::State_FREEKICK_Think()
{
	if (m_pPl->GetFlags() & FL_REMOTECONTROLLED)
		return;

	if (m_bIsRemoteControlled)
	{
		m_bIsRemoteControlled = false;
	}

	if (m_pPl->m_nButtons & (IN_ATTACK | IN_ATTACK2))
	{
		if (DoGroundShot())
		{
			MarkOffsidePlayers();
			State_Transition(BALL_NORMAL);
		}
	}
}

void CBall::State_FREEKICK_Leave()
{
}

CSDKPlayer *CBall::FindEligibleCarrier()
{
	CSDKPlayer *pNearest = NULL;
	float shortestDist = FLT_MAX;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPlayer = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!(pPlayer &&
			pPlayer->GetTeamNumber() != TEAM_SPECTATOR &&
			pPlayer->IsAlive()))
			continue;

		if (!((pPlayer->m_nButtons & IN_ATTACK || pPlayer->m_nButtons & IN_ATTACK2) &&
			pPlayer->m_flNextShot <= gpGlobals->curtime))
			continue;

		Vector dir = m_vPos - pPlayer->GetLocalOrigin();
		float xyDist = dir.Length2D();
		float zDist = m_vPos.z - (pPlayer->GetLocalOrigin().z + SDKGameRules()->GetViewVectors()->m_vHullMax.z); //pPlayer->GetPlayerMaxs().z);// 
		float dist = max(xyDist, zDist);

		if (dist > sv_ball_touchradius.GetFloat() || dist >= shortestDist)
			continue;

		dir.z = 0;
		dir.NormalizeInPlace();
		float angle = RAD2DEG(acos(pPlayer->EyeDirection2D().Dot(dir)));
		if (angle > sv_ball_touchcone.GetFloat())
			continue;

		shortestDist = dist;
		pNearest = pPlayer;	
	}

	return pNearest;
}

#define BODY_FEET_START		0
#define BODY_FEET_END		15
#define BODY_VOLLEY_START	15
#define BODY_VOLLEY_END		30
#define BODY_CHEST_START	45
#define BODY_CHEST_END		55
#define BODY_HEAD_START		65
#define BODY_HEAD_END		80

bool CBall::DoBodyPartAction()
{
	float zDist = (m_vPos - m_vPlPos).z;

	if (zDist >= BODY_FEET_START && zDist < BODY_FEET_END)
		return DoGroundShot();

	if (zDist >= BODY_VOLLEY_START && zDist < BODY_VOLLEY_END)
		return DoVolleyShot();

	if (zDist >= BODY_CHEST_START && zDist < BODY_CHEST_END)
		return DoChestDrop();

	if (zDist >= BODY_HEAD_START && zDist < BODY_HEAD_END)
		return DoHeader();

	return false;
}

#define BEST_SHOT_ANGLE -20
#define PITCH_LIMIT 89

float CBall::GetPitchModifier()
{
	//float modifier = (90 - (abs((clamp(m_aPlAng[PITCH], -75, 75) - BEST_SHOT_ANGLE) * 0.5f + BEST_SHOT_ANGLE))) / 90.0f;
	//float modifier = (1 - (cos((90 - abs(clamp(m_aPlAng[PITCH], -90, 90))) / 90.0f * M_PI))) / 2.0f;
	return pow(cos((m_pPl->EyeAngles()[PITCH] - BEST_SHOT_ANGLE) / (PITCH_LIMIT - BEST_SHOT_ANGLE) * M_PI / 2), 2);
}

float CBall::GetPowershotModifier()
{
	int powershotStrength = min(m_pPl->m_nPowershotStrength, m_pPl->m_Shared.GetStamina());
	m_pPl->m_Shared.SetStamina(m_pPl->m_Shared.GetStamina() - powershotStrength);
	return powershotStrength / 100.0f;
}

bool CBall::DoGroundShot()
{
	int shotStrength;

	float modifier = GetPitchModifier();

	//has enough sprint for a powershot?
	if (m_bIsPowershot)
	{
		shotStrength = sv_ball_powershot_strength.GetFloat() * (1 + GetPowershotModifier()) * modifier;
		EmitSound("Ball.kickhard");
		m_pPl->SetAnimation(PLAYER_KICK);
		m_pPl->DoAnimationEvent(PLAYERANIMEVENT_KICK);
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
		//	m_pPl->SetAnimation(PLAYER_KICK);
		//	m_pPl->DoAnimationEvent(PLAYERANIMEVENT_KICK);
		//}
	}

	QAngle shotAngle = m_aPlAng;
	shotAngle[PITCH] = min(-5, m_aPlAng[PITCH]);

	Vector shotDir;
	AngleVectors(shotAngle, &shotDir);

	m_vVel = shotDir * shotStrength;

	//SetBallCurve(shotStrength == 0);
	SetBallCurve(false);

	Kicked(BODY_FEET);

	return true;
}

#define VOLLEY_ANGLE 15

bool CBall::DoVolleyShot()
{
	if (!m_bIsPowershot || m_vPlVel.Length2D() > 10)
		return false;

	Vector dir = m_vPos - m_vPlPos;
	dir.z = 0;
	float angle = RAD2DEG(acos(m_vPlRight.Dot(dir)));

	if (angle > 90)
		angle = abs(angle - 180);

	if (angle > VOLLEY_ANGLE)
		return false;

	m_vVel = m_vPlForward * sv_ball_powershot_strength.GetFloat() * 2;

	SetBallCurve(false);

	EmitSound("Ball.kickhard");
	m_pPl->SetAnimation(PLAYER_VOLLEY);
	m_pPl->DoAnimationEvent(PLAYERANIMEVENT_VOLLEY);

	Kicked(BODY_FEET);

	return true;
}

bool CBall::DoChestDrop()
{
	return false;
}

bool CBall::DoHeader()
{
	if (m_bIsPowershot && 
		m_vPlVel.Length2D() >= PLAYER_SPEED + SPRINT_SPEED - FLT_EPSILON &&
		m_pPl->m_TeamPos > 1 &&
		m_BallInPenaltyBox != -1 &&
		m_pPl->m_PlayerAnim != PLAYER_SLIDE)
	{
		m_vVel = m_vPlForward * (sv_ball_powershot_strength.GetFloat() * 1.5f + m_vPlVel.Length());

		EmitSound("Ball.kickhard");
		m_pPl->SetAnimation (PLAYER_DIVINGHEADER);
		m_pPl->DoAnimationEvent(PLAYERANIMEVENT_DIVINGHEADER);
		m_pPl->m_NextSlideTime = gpGlobals->curtime + 1.5f;
	}
	else
	{
		m_vVel = m_vPlForward * (sv_ball_normalshot_strength.GetFloat() + m_vPlVel.Length());

		EmitSound("Ball.kicknormal");
		m_pPl->SetAnimation (PLAYER_HEADER);
		m_pPl->DoAnimationEvent(PLAYERANIMEVENT_HEADER);
	}
	
	Kicked(BODY_HEAD);

	return true;
}

void CBall::SetBallCurve(bool bReset)
{
	Vector m_vRot(0, 0, 0);

	if (!bReset)
	{
		if (m_pPl->m_nButtons & IN_MOVELEFT) 
		{
			m_vRot += Vector(0, 0, 1);
		} 
		else if (m_pPl->m_nButtons & IN_MOVERIGHT) 
		{
			m_vRot += Vector(0, 0, -1);//-v_up;
		}

		if (m_pPl->m_nButtons & IN_TOPSPIN)
		{
			m_vRot += -m_vPlRight;
		}
		else if (m_pPl->m_nButtons & IN_BACKSPIN)
		{
			m_vRot += m_vPlRight;
		}
	}

	m_vRot.NormalizeInPlace();

	float spin = min(1, m_vVel.Length() / sv_ball_maxspin.GetInt()) * sv_ball_spin.GetFloat();

	m_vAngImp = WorldToLocalRotation(SetupMatrixAngles(m_aAng), m_vRot, spin);
}

void CBall::BallThink( void	)
{
	SetNextThink(gpGlobals->curtime + 0.01f);

	State_Think();
}

void CBall::TriggerGoal(int team)
{
	m_team = team;
	State_Transition(BALL_GOAL);
}

void CBall::TriggerGoalline(int team, int side)
{
	m_side = side;

	if (m_LastTouch->GetTeamNumber() == team)
		State_Transition(BALL_CORNER);
	else
		State_Transition(BALL_GOALKICK);
}

void CBall::TriggerSideline(int side)
{
	State_Transition(BALL_THROWIN);
}

bool CBall::SetCarrier(CSDKPlayer *pPlayer)
{
	if (!pPlayer)
		return false;

	m_pPl = pPlayer;
	UpdateCarrier();

	return true;
}

void CBall::UpdateCarrier()
{
	if (!m_pPl)
		return;

	m_vPlPos = m_pPl->GetLocalOrigin();
	m_vPlVel = m_pPl->GetLocalVelocity();
	m_aPlAng = m_pPl->EyeAngles();
	AngleVectors(m_aPlAng, &m_vPlForward, &m_vPlRight, &m_vPlUp);
	m_nPlTeam = m_pPl->GetTeamNumber();
	m_bIsPowershot = m_pPl->m_nButtons & IN_ATTACK2;
}

void CBall::MarkOffsidePlayers()
{
	for (int i = 1; i < gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (pPl)
			pPl->SetOffside(false);

		if (!(pPl && pPl->GetTeamNumber() == m_nPlTeam))
			continue;

		Vector pos = pPl->GetLocalOrigin();
		int forward = GetOwnTeamSpots(pPl)->m_nForward;

		// In opponent half?
		if (Sign((pos - g_vKickOffSpot).y) != forward)
			continue;

		// Closer to goal than the ball?
		if (Sign((pos - m_vPos).y) != forward)
			continue;

		int nearerPlayerCount = 0;

		// Count players who are nearer to goal
		for (int i = 1; i < gpGlobals->maxClients; i++)
		{
			CSDKPlayer *pOpp = ToSDKPlayer(UTIL_PlayerByIndex(i));
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
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (pPl)
			pPl->SetOffside(false);
	}
}

void CBall::Kicked(body_part_t bodyPart)
{
	m_eBodyPart = bodyPart;
	m_LastTouch = m_LastPlayer = m_LastNonKeeper = m_pPl;
	m_pPl->m_flNextShot = gpGlobals->curtime + SHOT_DELAY;
}