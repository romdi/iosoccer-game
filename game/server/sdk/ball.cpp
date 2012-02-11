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
	m_pPossessingPl = NULL;
	m_nPossessingTeam = TEAM_INVALID;
	m_flPossessionStart = -1;
	m_pPl = NULL;
	m_nPlTeam = TEAM_INVALID;
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
	m_pPhys->SetPosition(GetLocalOrigin(), GetLocalAngles(), true);

	PrecacheScriptSound( "Ball.kicknormal" );
	PrecacheScriptSound( "Ball.kickhard" );
	PrecacheScriptSound( "Ball.touch" );
	PrecacheScriptSound( "Ball.post" );
	PrecacheScriptSound( "Ball.net" );
	PrecacheScriptSound( "Ball.whistle" );
	PrecacheScriptSound( "Ball.cheer" );

	//PrecacheMaterial("sprites/physBeam.vmt");
	//m_pOffsideLine = CBeam::BeamCreate("sprites/physBeam.vmt", 10);
	//m_pOffsideLine->SetColor(255, 100, 100);
	//DisableOffsideLine();
	//EnableOffsideLine(SDKGameRules()->m_vKickOff[1]);

	////
	//CBeam *pBeam;
	//pBeam = CBeam::BeamCreate("sprites/physBeam.vmt", 10);
	//pBeam->SetColor(150, 255, 150);
	//pBeam->SetAbsStartPos(SDKGameRules()->m_vKickOff - Vector(500, 0, 0));
	//pBeam->SetAbsEndPos(SDKGameRules()->m_vKickOff + Vector(500, 0, 0));

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

		Touched(ToSDKPlayer(pOther), false);

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
		pPlayer->GetNetworkIDString(), pPlayer->GetTeam()->GetName(), matchEvent);

	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin(filter, "MatchEvent");
		WRITE_BYTE(matchEvent);
		WRITE_BYTE(pPlayer ? pPlayer->entindex() : 1);
	MessageEnd();
}

CSDKPlayer *CBall::FindNearestPlayer(int team /*= TEAM_INVALID*/, int posFlags /*= FL_POS_FIELD*/, bool checkIfShooting /*= false*/)
{
	CSDKPlayer *pNearest = NULL;
	float shortestDist = FLT_MAX;
	//if (team == TEAM_INVALID)
	//	team = g_IOSRand.RandomInt(TEAM_A, TEAM_B);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPlayer = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!PlOnField(pPlayer))
			continue;

		if (!(posFlags & FL_POS_ANY) && ((posFlags & FL_POS_KEEPER) && pPlayer->GetTeamPosition() != 1 || (posFlags & FL_POS_FIELD) && pPlayer->GetTeamPosition() == 1))
			continue;

		if (team != TEAM_INVALID && pPlayer->GetTeamNumber() != team)
			continue;

		if (checkIfShooting && !(pPlayer->m_nButtons & (IN_ATTACK | IN_ATTACK2) && pPlayer->m_flNextShot <= gpGlobals->curtime))
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
	bool ignoreTriggers = m_bIgnoreTriggers;
	m_bIgnoreTriggers = true;
	m_pPhys->EnableMotion(true);
	m_pPhys->SetVelocityInstantaneous(&vec3_origin, &vec3_origin);
	m_pPhys->SetPosition(m_vPos, m_aAng, true);
	m_pPhys->EnableMotion(false);
	m_bIgnoreTriggers = ignoreTriggers;
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

void CBall::SetPlPos(const Vector &pos)
{
	m_vPlPos = pos;
	m_pPl->SetLocalOrigin(m_vPlPos);
}

void CBall::SetPlAng(const QAngle &ang)
{
	m_aPlAng = ang;
	m_pPl->SnapEyeAngles(m_aPlAng);
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
	State_Enter( newState );
}

void CBall::State_Enter( ball_state_t newState )
{
	m_eBallState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );
	m_flStateEnterTime = gpGlobals->curtime;

	m_pPl = NULL;
	SDKGameRules()->DisableShield();

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!PlOnField(pPl))
			continue;

		pPl->m_bIsAtTargetPos = false;
	}

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
	if (m_flStateLeaveTime > gpGlobals->curtime)
		return;

	m_pPhys->GetPosition(&m_vPos, &m_aAng);
	m_pPhys->GetVelocity(&m_vVel, &m_vRot);

	while (m_eNextState != BALL_NOSTATE && m_flStateLeaveTime <= gpGlobals->curtime)
	{
		State_DoTransition(m_eNextState);
	}

	if ( m_pCurStateInfo && m_pCurStateInfo->pfnThink )
	{	
		(this->*m_pCurStateInfo->pfnThink)();
	}
}

CBallStateInfo* CBall::State_LookupInfo( ball_state_t state )
{
	static CBallStateInfo ballStateInfos[] =
	{
		{ BALL_NORMAL,		"BALL_NORMAL",		&CBall::State_NORMAL_Enter,		NULL,							&CBall::State_NORMAL_Think },
		{ BALL_KICKOFF,		"BALL_KICKOFF",		&CBall::State_KICKOFF_Enter,	NULL,							&CBall::State_KICKOFF_Think },
		{ BALL_THROWIN,		"BALL_THROWIN",		&CBall::State_THROWIN_Enter,	NULL,							&CBall::State_THROWIN_Think },
		{ BALL_GOALKICK,	"BALL_GOALKICK",	&CBall::State_GOALKICK_Enter,	NULL,							&CBall::State_GOALKICK_Think },
		{ BALL_CORNER,		"BALL_CORNER",		&CBall::State_CORNER_Enter,		NULL,							&CBall::State_CORNER_Think },
		{ BALL_GOAL,		"BALL_GOAL",		&CBall::State_GOAL_Enter,		NULL,							&CBall::State_GOAL_Think },
		{ BALL_FREEKICK,	"BALL_FREEKICK",	&CBall::State_FREEKICK_Enter,	NULL,							&CBall::State_FREEKICK_Think },
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
	m_bIgnoreTriggers = false;
}

void CBall::State_NORMAL_Think()
{
	m_pPl = FindNearestPlayer(TEAM_INVALID, FL_POS_ANY, true);
	if (!m_pPl)
		return;

	if (!IsPlayerCloseEnough(m_pPl))
		return;

	UpdateCarrier();

	if (!DoBodyPartAction())
		return;

	if (m_pPl->IsOffside())
	{
		m_eFoulType = FOUL_OFFSIDE;
		m_pFoulingPl = m_pPl;
		m_nFoulingTeam = m_pPl->GetTeamNumber();
		State_Transition(BALL_FREEKICK, 3);
	}
	else
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
		pPl->m_HoldAnimTime = -1;
		pPl->SetAnimation(PLAYER_IDLE);
		pPl->DoAnimationEvent(PLAYERANIMEVENT_CANCEL);
	}

	m_bIgnoreTriggers = false;
	SetPos(SDKGameRules()->m_vKickOff);
}

void CBall::State_KICKOFF_Think()
{
	if (!PlOnField(m_pPl))
	{
		int kickOffTeam;
		if (m_bRegularKickOff)
		{
			if (SDKGameRules()->GetTeamsSwapped())
				kickOffTeam = SDKGameRules()->GetKickOffTeam() == TEAM_A ? TEAM_B : TEAM_A;
			else
				kickOffTeam = SDKGameRules()->GetKickOffTeam();
		}
		else
		{
			kickOffTeam = LastOppTeam(true);
		}

		m_pPl = FindNearestPlayer(kickOffTeam);
		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		SDKGameRules()->EnableStaticShield(SHIELD_KICKOFF, kickOffTeam);
		UpdatePossession(m_pPl);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x - m_pPl->GetTeam()->m_nRight * 30, m_vPos.y, SDKGameRules()->m_vKickOff[2]), true);
		EmitSound("Ball.whistle");
		SendMatchEvent(MATCH_EVENT_KICKOFF);
	}

	if (!PlayersAtTargetPos(true))
		return;

	if (m_pPl->GetFlags() & FL_REMOTECONTROLLED)
	{
		m_pPl->RemoveFlag(FL_REMOTECONTROLLED);
		m_pPl->SetMoveType(MOVETYPE_WALK);
		m_pPl->AddFlag(FL_ATCONTROLS);
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!PlOnField(pPl))
			continue;

		if (pPl->GetFlags() & FL_REMOTECONTROLLED)
		{
			pPl->RemoveFlag(FL_REMOTECONTROLLED);
			pPl->SetMoveType(MOVETYPE_WALK);
		}
	}

	UpdateCarrier();

	if (m_pPl->m_nButtons & (IN_ATTACK | IN_ATTACK2))
	{
		SetVel(m_vPlForward * 200);
		Kicked(BODY_FEET);
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_THROWIN_Enter()
{
	UnmarkOffsidePlayers();
	m_bIgnoreTriggers = true;
	SetPos(Vector(m_vTriggerTouchPos.x - 10 * Sign((SDKGameRules()->m_vKickOff - m_vTriggerTouchPos).x), m_vTriggerTouchPos.y, SDKGameRules()->m_vKickOff.GetZ()));
}

void CBall::State_THROWIN_Think()
{
	if (!PlOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(LastOppTeam(false));
		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		SDKGameRules()->EnableCircleShield(m_vPos);
		UpdatePossession(m_pPl);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y, SDKGameRules()->m_vKickOff.GetZ()), true);
		SendMatchEvent(MATCH_EVENT_THROWIN);
		EmitSound("Ball.whistle");
	}

	if (!PlayersAtTargetPos(false))
		return;

	if (m_pPl->GetFlags() & FL_REMOTECONTROLLED)
	{
		SetPos(Vector(m_vPos.x, m_vPos.y, SDKGameRules()->m_vKickOff.GetZ() + VEC_HULL_MAX.z + 2));
		m_pPl->RemoveFlag(FL_REMOTECONTROLLED);
		m_pPl->SetMoveType(MOVETYPE_WALK);
		m_pPl->AddFlag(FL_ATCONTROLS);
		m_pPl->m_HoldAnimTime = -1;
		m_pPl->SetAnimation(PLAYER_THROWIN);
		m_pPl->DoAnimationEvent(PLAYERANIMEVENT_THROWIN);
		return; // Give bots time to detect FL_ATCONTROLS
	}

	UpdateCarrier();

	if (m_pPl->m_nButtons & (IN_ATTACK | IN_ATTACK2))
	{
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
		m_pPl->m_HoldAnimTime = gpGlobals->curtime + 0.75f;
		m_pPl->SetAnimation(PLAYER_THROW);
		m_pPl->DoAnimationEvent(PLAYERANIMEVENT_THROW);

		Vector2D dirToKickOff = Vector2D((SDKGameRules()->m_vKickOff - m_vPos).x, 0);
		dirToKickOff.NormalizeInPlace();
		Vector2D vel = m_vVel.AsVector2D();
		vel.NormalizeInPlace();

		if (RAD2DEG(acos(DotProduct2D(dirToKickOff, vel))) > 80)
			State_Transition(BALL_THROWIN, 1);
		else
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
	if (!PlOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(LastOppTeam(false), FL_POS_KEEPER);
		if (!m_pPl)
			m_pPl = FindNearestPlayer(LastOppTeam(false));
		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		SDKGameRules()->EnableStaticShield(SHIELD_GOALKICK, LastOppTeam(false));
		UpdatePossession(m_pPl);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 200 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff[2]), false);
		//SDKGameRules()->EnableCircShield(FL_SHIELD_PLAYER, m_pPl->entindex(), 360, GetTeamSpots(LastOppTeam(false))->m_vPenalty);
		//SDKGameRules()->EnableRectShield(FL_SHIELD_PLAYER, m_pPl->entindex(), GetTeamSpots(LastOppTeam(false))->m_vPenaltyMin, GetTeamSpots(LastOppTeam(false))->m_vPenaltyMax, false);
		SendMatchEvent(MATCH_EVENT_GOALKICK);
		EmitSound("Ball.whistle");
	}

	if (!PlayersAtTargetPos(false))
		return;

	UpdateCarrier();

	if (m_pPl->m_nButtons & (IN_ATTACK | IN_ATTACK2) && IsPlayerCloseEnough(m_pPl))
	{
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
	if (!PlOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(LastOppTeam(false));
		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		SDKGameRules()->EnableCircleShield(m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x - 50 * Sign((SDKGameRules()->m_vKickOff - m_vPos).x), m_vPos.y - 50 * Sign((SDKGameRules()->m_vKickOff - m_vPos).y), SDKGameRules()->m_vKickOff[2]), false);
		UpdatePossession(m_pPl);
		SendMatchEvent(MATCH_EVENT_CORNER);
		EmitSound("Ball.whistle");
	}

	if (!PlayersAtTargetPos(false))
		return;

	UpdateCarrier();

	if (m_pPl->m_nButtons & (IN_ATTACK | IN_ATTACK2) && IsPlayerCloseEnough(m_pPl))
	{
		DoGroundShot();
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_GOAL_Enter()
{
	UpdatePossession(NULL);
	m_bIgnoreTriggers = true;
	m_bRegularKickOff = false;

	if (m_team == LastTeam(true))
	{
		if (LastPl(true))
			LastPl(true)->IncrementFragCount(1);

		GetGlobalTeam(LastTeam(true))->AddScore(1);
	}
	else if (m_team == LastOppTeam(true))
	{
		GetGlobalTeam(LastOppTeam(true))->AddScore(1);
	}

	if (m_Touches.Count() >= 2)
		m_Touches.RemoveMultiple(0, m_Touches.Count() - 1);

	SendMatchEvent(MATCH_EVENT_GOAL, LastPl(true));
	EmitSound("Ball.whistle");
	State_Transition(BALL_KICKOFF, 5);
}

void CBall::State_GOAL_Think()
{
}

void CBall::State_FREEKICK_Enter()
{

}

void CBall::State_FREEKICK_Think()
{
	if (!PlOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(GetGlobalTeam(m_nFoulingTeam)->GetOppTeamNumber());
		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		UpdatePossession(m_pPl);
		SetPlPos(Vector(m_vPos.x, m_vPos.y - 200 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff[2]));
		SetPlAng(QAngle(0, 45, 0));
		SDKGameRules()->EnableCircleShield(m_vPos);
	}

	UpdateCarrier();

	if (m_pPl->m_nButtons & (IN_ATTACK | IN_ATTACK2) && IsPlayerCloseEnough(m_pPl))
	{
		DoGroundShot();
		MarkOffsidePlayers();
		State_Transition(BALL_NORMAL);
	}
}

bool CBall::PlayersAtTargetPos(bool holdAtTargetPos)
{
	//repeat and check for players without flag fl_remote_controlled
	//if taker left field, choose new player
		//iterate over all players on field
			//if player == taker then set target pos right away
			//otherwise calculate nearest pos outside shield

	bool playersAtTarget = true;

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!PlOnField(pPl))
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

bool CBall::IsPlayerCloseEnough(CSDKPlayer *pPl)
{
	Vector dir = m_vPos - pPl->GetLocalOrigin();
	float xyDist = dir.Length2D();
	float zDist = m_vPos.z - (pPl->GetLocalOrigin().z + SDKGameRules()->GetViewVectors()->m_vHullMax.z); //pPlayer->GetPlayerMaxs().z);// 
	float dist = max(xyDist, zDist);

	if (dist > sv_ball_touchradius.GetFloat())
		return false;

	dir.z = 0;
	dir.NormalizeInPlace();
	float angle = RAD2DEG(acos(pPl->EyeDirection2D().Dot(dir)));
	if (angle > sv_ball_touchcone.GetFloat())
		return false;

	return true;
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

	SetVel(shotDir * shotStrength);

	//SetBallCurve(shotStrength == 0);
	SetBallCurve();

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

	SetVel(m_vPlForward * sv_ball_powershot_strength.GetFloat() * 2);

	SetBallCurve();

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
		m_nBallInPenaltyBox != TEAM_INVALID &&
		m_pPl->m_PlayerAnim != PLAYER_SLIDE)
	{
		SetVel(m_vPlForward * (sv_ball_powershot_strength.GetFloat() * 1.5f + m_vPlVel.Length()));

		EmitSound("Ball.kickhard");
		m_pPl->SetAnimation (PLAYER_DIVINGHEADER);
		m_pPl->DoAnimationEvent(PLAYERANIMEVENT_DIVINGHEADER);
		m_pPl->m_NextSlideTime = gpGlobals->curtime + 1.5f;
	}
	else
	{
		SetVel(m_vPlForward * (sv_ball_normalshot_strength.GetFloat() + m_vPlVel.Length()));

		EmitSound("Ball.kicknormal");
		m_pPl->SetAnimation (PLAYER_HEADER);
		m_pPl->DoAnimationEvent(PLAYERANIMEVENT_HEADER);
	}
	
	Kicked(BODY_HEAD);

	return true;
}

void CBall::SetBallCurve()
{
	Vector rot(0, 0, 0);

	if (m_pPl->m_nButtons & IN_MOVELEFT) 
		rot += Vector(0, 0, 1);
	else if (m_pPl->m_nButtons & IN_MOVERIGHT) 
		rot += Vector(0, 0, -1);

	if (m_pPl->m_nButtons & IN_TOPSPIN)
		rot += -m_vPlRight;
	else if (m_pPl->m_nButtons & IN_BACKSPIN)
		rot += m_vPlRight;

	rot.NormalizeInPlace();
	float spin = min(1, m_vVel.Length() / sv_ball_maxspin.GetInt()) * sv_ball_spin.GetFloat();

	AngularImpulse randRot = AngularImpulse(g_IOSRand.RandomInt(-100, 100), g_IOSRand.RandomInt(-100, 100), g_IOSRand.RandomInt(-100, 100));
	SetRot(WorldToLocalRotation(SetupMatrixAngles(m_aAng), rot, spin) + randRot);
}

void CBall::BallThink( void	)
{
	SetNextThink(gpGlobals->curtime + 0.01f);

	State_Think();
}

void CBall::TriggerGoal(int team)
{
	if (SDKGameRules()->GetTeamsSwapped())
	{
		team = team == TEAM_A ? TEAM_B : TEAM_A;
	}
	m_team = team;
	State_Transition(BALL_GOAL);
}

void CBall::TriggerGoalLine(int team, float touchPosY)
{
	if (SDKGameRules()->GetTeamsSwapped())
	{
		team = team == TEAM_A ? TEAM_B : TEAM_A;
	}

	m_pPhys->GetPosition(&m_vTriggerTouchPos, NULL);
	m_vTriggerTouchPos.y = touchPosY;

	if (LastTeam(false) == team)
		State_Transition(BALL_CORNER);
	else
		State_Transition(BALL_GOALKICK);
}

void CBall::TriggerSideline(float touchPosX)
{
	Vector vel;
	m_pPhys->GetVelocity(&vel, NULL);
	m_pPhys->GetPosition(&m_vTriggerTouchPos, NULL);
	m_vTriggerTouchPos.x = touchPosX;
	//DevMsg("trigger touch pos: %0.2f\n", m_vTriggerTouchPos.x);

	if (Sign(vel.x) != Sign(SDKGameRules()->m_vKickOff.GetX() - m_vTriggerTouchPos.x))
		State_Transition(BALL_THROWIN);
}

void CBall::TriggerPenaltyBox(int team)
{
	m_nBallInPenaltyBox = team;
}

void CBall::UpdateCarrier()
{
	if (PlOnField(m_pPl))
	{
		m_vPlPos = m_pPl->GetLocalOrigin();
		m_vPlVel = m_pPl->GetLocalVelocity();
		m_aPlAng = m_pPl->EyeAngles();
		AngleVectors(m_aPlAng, &m_vPlForward, &m_vPlRight, &m_vPlUp);
		m_nPlTeam = m_pPl->GetTeamNumber();
		m_nPlPos = m_pPl->GetTeamPosition();
		m_bIsPowershot = (m_pPl->m_nButtons & IN_ATTACK2) != 0;
	}
	else
	{
		m_nPlTeam = TEAM_INVALID;
		m_nPlPos = 0;
	}
}

bool CBall::PlOnField(CSDKPlayer *pPl)
{
	return (pPl && pPl->IsConnected() && (pPl->GetTeamNumber() == TEAM_A || pPl->GetTeamNumber() == TEAM_B));
}

void CBall::MarkOffsidePlayers()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (PlOnField(pPl))
			pPl->SetOffside(false);

		if (!(PlOnField(pPl) && pPl != m_pPl && pPl->GetTeamNumber() == LastTeam(true)))
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

		if (pPl)
			pPl->SetOffside(false);
	}
}

void CBall::Kicked(body_part_t bodyPart)
{
	m_eBodyPart = bodyPart;
	m_pPl->m_flNextShot = gpGlobals->curtime + SHOT_DELAY;
	Touched(m_pPl, true);
}

void CBall::Touched(CSDKPlayer *pPl, bool isShot)
{
	if (m_Touches.Count() > 0 && m_Touches.Tail().m_pPl == pPl && m_Touches.Tail().m_pPl->GetTeamNumber() == pPl->GetTeamNumber())
	{
		m_Touches.Tail().m_bIsShot = isShot;
	}
	else
	{
		UpdatePossession(pPl);
		CBallTouchInfo info = { pPl, pPl->GetTeamNumber(), isShot };
		m_Touches.AddToTail(info);
	}
	
	if (pPl->IsOffside())
	{
		m_eFoulType = FOUL_OFFSIDE;
		m_pFoulingPl = pPl;
		m_nFoulingTeam = pPl->GetTeamNumber();
		EnableOffsideLine(m_vPos.y);
		State_Transition(BALL_FREEKICK);
	}
}

CBallTouchInfo *CBall::LastInfo(bool wasShooting)
{
	for (int i = m_Touches.Count() - 1; i >= 0; i--)
	{
		if (!wasShooting || m_Touches[i].m_bIsShot)
			return &m_Touches[i];
	}

	return NULL;
}

CSDKPlayer *CBall::LastPl(bool wasShooting)
{
	CBallTouchInfo *info = LastInfo(wasShooting);
	return info ? info->m_pPl : NULL;
}

int CBall::LastTeam(bool wasShooting)
{
	CBallTouchInfo *info = LastInfo(wasShooting);
	return info ? info->m_nTeam : TEAM_INVALID;
}

int CBall::LastOppTeam(bool wasShooting)
{
	CBallTouchInfo *info = LastInfo(wasShooting);
	return info ? (info->m_nTeam == TEAM_A ? TEAM_B : TEAM_A) : TEAM_INVALID;
}

void CBall::UpdatePossession(CSDKPlayer *pNewPossessor)
{
	if (m_pPossessingPl == pNewPossessor)
		return;

	if (m_flPossessionStart != -1)
	{
		float duration = gpGlobals->curtime - m_flPossessionStart;
		CTeam *possessingTeam = GetGlobalTeam(m_nPossessingTeam);
		CTeam *otherTeam = GetGlobalTeam(m_nPossessingTeam == TEAM_A ? TEAM_B : TEAM_A);
		possessingTeam->m_flPossessionTime += duration;
		float total = max(1, possessingTeam->m_flPossessionTime + otherTeam->m_flPossessionTime);
		possessingTeam->m_nPossession = 100 * possessingTeam->m_flPossessionTime / total;		
		otherTeam->m_nPossession = 100 - possessingTeam->m_nPossession;

		if (PlOnField(m_pPossessingPl))
		{
			m_pPossessingPl->m_flPossessionTime += duration;
			m_pPossessingPl->m_Possession = 100 * m_pPossessingPl->m_flPossessionTime / total;
		}
	}

	if (PlOnField(pNewPossessor))
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

void CBall::EnableOffsideLine(float yPos)
{
	//m_pOffsideLine->SetAbsStartPos(Vector(SDKGameRules()->m_vFieldMin[0], yPos, SDKGameRules()->m_vKickOff[2] + 10));
	//m_pOffsideLine->SetAbsEndPos(Vector(SDKGameRules()->m_vFieldMax[0], yPos, SDKGameRules()->m_vKickOff[2] + 10));
	//m_pOffsideLine->SetAbsStartPos(SDKGameRules()->m_vKickOff + Vector(-500, 0, 100));
	//m_pOffsideLine->SetAbsEndPos(SDKGameRules()->m_vKickOff + Vector(500, 0, 100));
	//m_pOffsideLine->SetAbsStartPos(SDKGameRules()->m_vKickOff - Vector(500, 0, 0));
	//m_pOffsideLine->SetAbsEndPos(SDKGameRules()->m_vKickOff + Vector(500, 0, 0));
	//m_pOffsideLine->TurnOn();
}

void CBall::DisableOffsideLine()
{
	m_pOffsideLine->TurnOff();
}