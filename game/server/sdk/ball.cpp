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

static ConVar sv_ball_mass( "sv_ball_mass", "50", FCVAR_ARCHIVE );
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


static const float KICK_DELAY  = 2.0f;		//at throwins etc

#define SHOT_DELAY 0.5f

#define SPRINT_TIME           6.0f     //IOS sprint amount 5.5
#define SPRINT_RECHARGE_TIME  12.0f    //IOS time before sprint re-charges
#define SPRINT_SPEED          90.0f    //IOS sprint increase in speed
#define MAX_POWERSHOT_STRENGTH 5.0f

void CBall::RunCheck()
{
	VPhysicsGetObject()->GetPosition(&m_vPos, &m_aAng);
	VPhysicsGetObject()->GetVelocity(&m_vVel, &m_vAngImp);

	m_bSetVel = false;
	m_bSetAngImp = false;

	m_pPl = SelectShooter();

	if (!m_pPl)
		return;

	m_vPlPos = m_pPl->GetLocalOrigin();
	m_vPlVel = m_pPl->GetLocalVelocity();
	m_aPlAng = m_pPl->EyeAngles();
	AngleVectors(m_aPlAng, &m_vPlForward, &m_vPlRight, &m_vPlUp);

	m_bIsPowershot = m_pPl->m_nButtons & IN_ALT1;

	if (!SelectAction())
		return;

	ballStatus = BALL_NORMAL;

	m_pPl->m_flNextShot = gpGlobals->curtime + SHOT_DELAY;

	m_bSetVel = true;

	//if (m_vVel.Length() >= 500)
	//	m_vNewVel += m_vVel;
}

CSDKPlayer *CBall::SelectShooter()
{
	CSDKPlayer *pNearest = NULL;
	float smallestDist = FLT_MAX;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPlayer = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!(pPlayer &&
			pPlayer->GetTeamNumber() != TEAM_SPECTATOR &&
			pPlayer->IsAlive()))
			continue;

		if (!((pPlayer->m_nButtons & IN_ATTACK || pPlayer->m_nButtons & IN_ALT1) &&
			pPlayer->m_flNextShot <= gpGlobals->curtime))
			continue;

		Vector dir = m_vPos - pPlayer->GetLocalOrigin();
		float xyDist = dir.Length2D();
		float zDist = m_vPos.z - (pPlayer->GetLocalOrigin().z + SDKGameRules()->GetViewVectors()->m_vHullMax.z); //pPlayer->GetPlayerMaxs().z);// 
		float dist = max(xyDist, zDist);

		if (dist > sv_ball_touchradius.GetFloat() || dist >= smallestDist)
			continue;

		dir.z = 0;
		dir.NormalizeInPlace();
		float angle = RAD2DEG(acos(pPlayer->EyeDirection2D().Dot(dir)));
		if (angle > sv_ball_touchcone.GetFloat())
			continue;

		smallestDist = dist;
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

bool CBall::SelectAction()
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

#define BEST_SHOT_ANGLE 20

bool CBall::DoGroundShot()
{
	float powershotPercentage = m_pPl->m_nPowershotStrength / MAX_POWERSHOT_STRENGTH;
	float shotStrength;

	float modifier = (90 - (abs((clamp(m_aPlAng[PITCH], -75, 75) - BEST_SHOT_ANGLE) * 0.5f + BEST_SHOT_ANGLE))) / 90.0f;

	//has enough sprint for a powershot?
	if (m_bIsPowershot && m_pPl->m_fSprintLeft >= (SPRINT_TIME * powershotPercentage - 0.001f)) // - 0.1f for floating point comparison craziness
	{
		//float modifier = 1 - (abs(m_vPlForward.z) * 0.5f);
		shotStrength = sv_ball_powershot_strength.GetFloat() + (sv_ball_powershot_strength.GetFloat() * powershotPercentage);
		m_pPl->m_fSprintLeft = max(0, m_pPl->m_fSprintLeft - SPRINT_TIME * powershotPercentage);                   //reduce sprint energy
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
	shotAngle[PITCH] = min(-10, m_aPlAng[PITCH]);

	Vector shotDir;
	AngleVectors(shotAngle, &shotDir);

	m_vNewVel = shotDir * shotStrength;
	SetBallCurve();

	return true;
}

#define VOLLEY_ANGLE 15

bool CBall::DoVolleyShot()
{
	if (!m_bIsPowershot || m_vPlVel.Length2D() <= 10)
		return false;

	Vector dir = m_vPos - m_vPlPos;
	dir.z = 0;
	float angle = RAD2DEG(acos(m_vPlRight.Dot(dir)));
	if (angle > 90)
		angle = abs(angle - 180);
	if (angle > VOLLEY_ANGLE)
		return false;

	m_vNewVel = m_vPlForward * sv_ball_powershot_strength.GetFloat() * 2;

	EmitSound("Ball.kickhard");
	m_pPl->SetAnimation(PLAYER_VOLLEY);
	m_pPl->DoAnimationEvent(PLAYERANIMEVENT_VOLLEY);

	return true;
}

bool CBall::DoChestDrop()
{
	return false;
}

bool CBall::DoHeader()
{
	if (m_bIsPowershot && m_vPlVel.Length2D() >= PLAYER_SPEED + SPRINT_SPEED - 0.001f)
	{
		m_vNewVel = m_vPlForward * (sv_ball_powershot_strength.GetFloat() * 1.5f + m_vPlVel.Length());

		EmitSound("Ball.kickhard");
		m_pPl->SetAnimation (PLAYER_DIVINGHEADER);
		m_pPl->DoAnimationEvent(PLAYERANIMEVENT_DIVINGHEADER);
	}
	else
	{
		m_vNewVel = m_vPlForward * (sv_ball_normalshot_strength.GetFloat() + m_vPlVel.Length());

		EmitSound("Ball.kicknormal");
		m_pPl->SetAnimation (PLAYER_HEADER);
		m_pPl->DoAnimationEvent(PLAYERANIMEVENT_HEADER);
	}

	return true;
}

void CBall::SetBallCurve()
{
	Vector m_vRot(0, 0, 0);

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

	m_vRot.NormalizeInPlace();

	float spin;

	if (m_vRot.IsZero())
	{
		// weak backspin on every shot
		m_vRot = m_vPlRight;
		spin = sv_ball_spin.GetFloat() / 3;
	}
	else
	{
		spin = sv_ball_spin.GetFloat();
	}

	m_vNewAngImp = WorldToLocalRotation(SetupMatrixAngles(m_aAng), m_vRot, spin);

	m_bSetAngImp = true;
}

void CBall::BallThink( void	)
{
	RunCheck();
	
	if (m_bSetVel)
		m_vVel = m_vNewVel;

	if (m_bSetAngImp)
		m_vAngImp = m_vNewAngImp;
	
	Vector worldAngImp;
	EntityToWorldSpace(m_vAngImp, &worldAngImp);

	Vector magnusDir = worldAngImp.Cross(m_vVel);

	float length = m_vVel.Length();
	if (length > 0)
	{
		m_vVel += magnusDir * 1e-8 * sv_ball_curve.GetFloat();
		//m_vVel.NormalizeInPlace();
		//m_vVel *= length;
	}

	VPhysicsGetObject()->SetVelocity(&m_vVel, &m_vAngImp);

	if (KeepersBall())
		return;

	if (ballStatus)
		ProcessStatus();

	UpdateBallShield();

	m_BallInPenaltyBox  = -1;

	SetNextThink(gpGlobals->curtime + 0.01f);
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
	SetSolid( SOLID_VPHYSICS );
	SetSolidFlags( FSOLID_NOT_STANDABLE	);
	//UTIL_SetSize( this,	-Vector(5.0f,5.0f,5.0f), Vector(5.0f,5.0f,5.0f)	);
	UTIL_SetSize( this,	-Vector(3.0f,3.0f,3.0f), Vector(3.0f,3.0f,3.0f)	);
	objectparams_t params =	g_IOSPhysDefaultObjectParams;
	params.pGameData = static_cast<void	*>(this);
	int	nMaterialIndex = physprops->GetSurfaceIndex("ios");
	IPhysicsObject *pPhysicsObject = physenv->CreateSphereObject( 5.0f,	nMaterialIndex,	GetAbsOrigin(),	GetAbsAngles(),	&params, false );
	if ( !pPhysicsObject )
		return;

	VPhysicsSetObject( pPhysicsObject );
	SetMoveType( MOVETYPE_VPHYSICS );
	pPhysicsObject->Wake();

	PhysSetGameFlags( pPhysicsObject, FVPHYSICS_NO_PLAYER_PICKUP );

	float fColType = collisionType.GetFloat();

	if (fColType==1.0f)
		SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );	//ios1.1 server var mp_collisiontype - requires map change

	SetPhysics();

	SetUse(	&CBall::Use	);
	//SetTouch (&CBall::BallTouch);
	SetThink (&CBall::BallThink);
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_nBody = 0; 
	m_nSkin = g_IOSRand.RandomInt(0,5);
	m_TeamGoal = 0;

	PrecacheScriptSound( "Ball.kicknormal" );
	PrecacheScriptSound( "Ball.kickhard" );
	PrecacheScriptSound( "Ball.touch" );
	PrecacheScriptSound( "Ball.post" );
	PrecacheScriptSound( "Ball.net" );
	PrecacheScriptSound( "Ball.whistle" );
	PrecacheScriptSound( "Ball.cheer" );
}

void CBall::SetPhysics()
{
	m_fMass	= VPhysicsGetObject()->GetMass();

	VPhysicsGetObject()->SetMass( 0.05f	);
	VPhysicsGetObject()->EnableGravity(	true );
	VPhysicsGetObject()->EnableDrag( false );
	float flDamping	= sv_ball_damping.GetFloat(); //0.0f
	float flAngDamping = sv_ball_rotdamping.GetFloat(); //2.5f
	VPhysicsGetObject()->SetDamping( &flDamping, &flAngDamping );
//	VPhysicsGetObject()->SetInertia( Vector( 0.0023225760f,	0.0023225760f, 0.0023225760f ) );

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


//==========================================================
//	
//	
//==========================================================
void CBall::Use	( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	return;
	int ok = false;

	if (!pActivator	|| !pActivator->IsPlayer() )	
		return;

	if (useType == USE_SHOOT)
		ok = Shoot (pActivator);

	if (useType == USE_PASS)
		ok = Pass (pActivator);

	if (useType == USE_POWERSHOT_KICK)
		ok = Shoot(pActivator, true);




//	 if	(useType ==	USE_GOALKICK)
//		ok = GoalKick (pActivator);

//	 if	(useType ==	USE_POWERSHOT)
//		ok = PowerShot (pActivator);

   /*trap the ball - deaden	vel
   if (value ==	1.0	&& CVAR_GET_FLOAT("mp_allowtrap")) {
	  if (((CSDKPlayer*)pActivator)->SprintLeft	> 0.0f)
	  {
		 pev->velocity.x = pev->velocity.x * 0.9f;
		 pev->velocity.y = pev->velocity.y * 0.9f;
		 ((CSDKPlayer*)pActivator)->SprintLeft -= (4 * gpGlobals->frametime);	//reduce sprint	energy when	using trap
	  }
   }*/

   //didn't	actually manage	to kick	so quit
   if (!ok)
	  return;

   //psCheckPenaltyKick((CSDKPlayer*)pActivator);


   KickedBall((CSDKPlayer*)pActivator);			//do all the things associated with kick/touch of the ball

}

//////////////////////////////////////////////
// do everything associated with a kick 
// or touch
void CBall::KickedBall(CSDKPlayer *pPlayer, const bool bKick)
{
	//record two players to touch the ball
	if (pPlayer->IsPlayer()) 
	{
		if (m_LastTouch != pPlayer)						//remember the player before me
			m_LastTouchBeforeMe = m_LastTouch;

		if (!(pPlayer->GetFlags() &	FL_FAKECLIENT))
			m_LastPlayer = pPlayer;						//record last real player

		if (pPlayer->m_TeamPos != 1)
			m_LastNonKeeper = pPlayer;					//record last non keeper

		m_LastTouch = pPlayer;							//record last touch (inc goal keepers)

		AddAssist (pPlayer);
		UpdatePossession (pPlayer); 
	}

	CheckOffsideReceive (pPlayer);	//did this player just receive an offside ball

	if (ballStatus!=BALL_THROWIN_PENDING && ballStatus!=BALL_GOALKICK_PENDING && ballStatus!=BALL_CORNERKICK_PENDING)
	{
		CheckOffSide (pPlayer);		//did this player just create a possible offside kick
	}
	else
		ClearOffsideFlags();		//?

	//if I've just made a throwin stop passing off the throw
	if (ballStatus == BALL_THROWIN_PENDING) 
	{
		if (bKick)
		{
			m_NextPass = gpGlobals->curtime + 0.25f;			//1.0c
			//m_NextTouch	= gpGlobals->curtime + 0.1f;
			pPlayer->m_HoldAnimTime = 0.0f;
			pPlayer->SetAnimation (PLAYER_THROW);
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_THROW );
		}
		else
		{
			return;	//touching players head
		}
	}

	//stop keepers picking up right away after a GK - 1.0c
	if (ballStatus == BALL_GOALKICK_PENDING)
		m_NextKeeperCatch = gpGlobals->curtime + 2.0f;

	//check if I am "double touching" from a corner etc //i.e. if the player who touches is the same as the player who just took the throwin
	//(ignore keepers, bots or human)
	if ((pPlayer == m_PlayerWhoTook) && 
		(ballStatus==0) && (ballStatus!=BALL_THROWIN_PENDING) && 
		(gpGlobals->curtime > m_NextDoubleTouch) && (pPlayer->m_TeamPos!=1))
	{
		DoubleTouchFoul (m_PlayerWhoTook);
	}

	//clr status if this player has taken goalkick/corner/throwin/foul etc
	//(it clrs for everyone even if they arent the 'taker')
	if (ballStatus >= BALL_GOALKICK_PENDING) 
	{
		if (ballStatus == BALL_PENALTY_PENDING)
			RemoveFlagFromAll(FL_ATCONTROLS);			//need to clr from all players/keepers since this kicker may not be the keeper
			//pPlayer->RemoveFlag(FL_ATCONTROLS);		//let keeper move again

		ballStatus = 0;

		//removed IOSS 1.0a - I think sometimes other players touch it before the actual taker, 
		//because they dont get teleported for a frame. I dont know if this is a fix yet.
		//if (pPlayer == m_BallShieldPlayer)		
			ShieldOff();	  

		m_PlayerWhoTook	= pPlayer;	//record the player	who	took the throwin/corner/free kick etc 
		m_NextDoubleTouch = gpGlobals->curtime + 0.25f;   //was 0.5 in	beta2

		if (m_KeeperCarrying)
			m_KeeperCarrying->DoAnimationEvent( PLAYERANIMEVENT_CARRY_END );
			//m_KeeperCarrying->RemoveFlag (FL_CARRY);

		m_KeeperCarrying = NULL;
	} 
	else 
	{
		if (pPlayer != m_PlayerWhoTook) 
		{
			m_PlayerWhoTook = NULL;
			m_DoubleTouchCount	= 0;
		}	 
	}
}



//==========================================================
//	
//	
//==========================================================
void CBall::VPhysicsCollision( int index, gamevcollisionevent_t	*pEvent	)
{
	curve = Vector(0.0f, 0.0f, 0.0f);  //loose any curve from a previous shot

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

		KickedBall((CSDKPlayer*)pOther, false);		//a touch

		EmitSound("Ball.touch");
	}

	//Warning ("surfaceprops index %d\n", surfaceProps);

	BaseClass::VPhysicsCollision( index, pEvent );
}


//static const float NORMAL_KICK_STRENGTH = 1.0f * 1.0f; //* 1.15f;			//pre 1.0c 1.1f;
//static const float POWER_KICK_STRENGTH = sv_powershot_strength.GetFloat();// 1.15f * 1.2f;
//static const float KEEPER_KICK_STRENGTH = 1.4f * 1.0f; //1.3f;



//void CBall::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
//{
//	Vector preVelocity = pEvent->preVelocity[index];
//	float flSpeed =	VectorNormalize( preVelocity );
//
//	///////////////////////////////////////////////////////////////////////
//	// player
//	//this doesnt seem to get called often enough to be any use!!
//	IPhysicsObject *pPhysObj = pEvent->pObjects[!index];
//	CBaseEntity *pOther = static_cast<CBaseEntity *>(pPhysObj->GetGameData());
//	if (pOther && pOther->IsPlayer())
//	{
//		if (flSpeed > 900.0f)
//			pOther->EmitSound ("Player.Oomph");
//
//		KickedBall((CSDKPlayer*)pOther, false);		//a touch
//
//		EmitSound("Ball.touch");
//	}
//}

//==========================================================
//	
//	
//==========================================================

int	CBall::Shoot (CBaseEntity *pOther, bool isPowershot)
{
	float		shotStrength = 0.0f;
	Vector		vel, avel = Vector (0.0f, 0.0f, 0.0f);
	float		powershot = sv_ball_normalshot_strength.GetFloat();
	CSDKPlayer	*pPlayer = ToSDKPlayer(pOther);

	if (m_NextShoot	> gpGlobals->curtime)
		return false;

	float powershotPercentage = pPlayer->m_nPowershotStrength / MAX_POWERSHOT_STRENGTH;

	//has enough sprint for a powershot?
	if (isPowershot && pPlayer->m_fSprintLeft >= (SPRINT_TIME * powershotPercentage - 0.001f)) // - 0.1f for floating point comparison craziness
	{
		powershot = 0.1f * sv_ball_powershot_strength.GetFloat() + (sv_ball_powershot_strength.GetFloat() * powershotPercentage);
		pPlayer->m_fSprintLeft = max(0, pPlayer->m_fSprintLeft - SPRINT_TIME * powershotPercentage);                   //reduce sprint energy
	}
	else
	{
		powershot = sv_ball_normalshot_strength.GetFloat();		//do normal kick instead
	}

	if (CheckKeeperKick(pPlayer) || (ballStatus == BALL_GOALKICK_PENDING && pPlayer->m_TeamPos == 1) )
	{
		powershot = sv_ball_keepershot_strength.GetFloat();				//remove carry and kick hard
		//do kick anim when keeper - this will do two events if kicking up?
		pPlayer->SetAnimation (PLAYER_KICK);
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_KICK );
	}
	else if (CheckKeeperCatch(pPlayer))
	{
		//keeper catch
		return false;
	}
	else
	{
		//check if keeper kick that may have been a save?
		if (pPlayer->m_TeamPos == 1 && pOther != m_LastTouch && m_BallInPenaltyBox != -1)
			pPlayer->m_KeeperSaves++;
	}

	//work out strength of kick
	int	ballrel	= GetRelativeBallPos(pOther);
	if (ballrel	== BALL_NEAR_FEET && ballStatus != BALL_CORNERKICK_PENDING) 
	{
		shotStrength = 700.0f;
		m_PlayerWhoHeaded = NULL;
	} 
	else if	(ballrel ==	BALL_NEAR_HEAD && ballStatus !=	BALL_THROWIN_PENDING) 
	{
		shotStrength = 450.0f; //ioss 600.0f;
		UpdateHeadbounce(pPlayer);
	} 
	else if (ballStatus ==	BALL_THROWIN_PENDING)
	{
		shotStrength = 600;
		m_PlayerWhoHeaded = NULL;
	}
	else if (ballStatus ==	BALL_CORNERKICK_PENDING)
	{
		shotStrength = 750;
		m_PlayerWhoHeaded = NULL;
	}
	else 
	{
		shotStrength = 0;//300.0f;
		m_PlayerWhoHeaded = NULL;
	}

	//fix strength for kick from hands
	if (powershot==sv_ball_keepershot_strength.GetFloat())
		shotStrength = 640.0f;


	//kick it
	Vector		v_forward, v_right, v_up;
	//CSDKPlayer	*pPlayer = ToSDKPlayer( pOther	);
	bool	bInSlide = ((CSDKPlayer*)pPlayer)->m_PlayerAnim == PLAYER_SLIDE ? 1 : 0;

	AngleVectors(pPlayer->EyeAngles(), &v_forward, &v_right, &v_up );

	//EyeVectors( &forward, NULL, &up );

	QAngle ang = pPlayer->EyeAngles();
	if ( ang.x > 180.0f )							//normalise angle (up=270, fwd = ~0, ~365)
		ang.x -= 360.0f;
	else if ( ang.x < -180.0f )
		ang.x += 360.0f;
	if (ang.x < -60.0f)								//clamp vertical kicks (up = -89)
		ang.x = -60.0f;

	AngleVectors (ang, &vel);

	//*** this is a mess - why have vel and v_forward.z ? ***

	//if powershoot down, flatten out the shot
	if (powershot == sv_ball_powershot_strength.GetFloat() && vel.z < 0)
	{
		v_forward.z = 0.05f;
		vel.z = 0.05f;
	}

	if (vel.z < 0.0f)
	{
		vel.z = 0.0f;
		//shotStrength *=	0.65f;	//was 0.5 in 1.0b				//damp kick	when trying	to dribble
		shotStrength *= vel.Length2D() * 0.75f;		//1.0c relate dribble to angle better
		if (shotStrength < 300.0f)			//min	//was 100 before 1.0c
			shotStrength = 300.0f;
		QAngle angle(-1.0f, 0, 0);
		//why punch?
		//pPlayer->SetPunchAngle( angle );
		//play small kick sound
		if (shotStrength > 400)				//1.0c
			EmitSound("Ball.touch");
	}
	else
	{
		vel.z = v_forward.z * 2.5f;		//make z aim more sensitive 
		avel.y = g_IOSRand.RandomFloat (-30 * shotStrength, 30 * shotStrength);   //spin if lifted into air
		avel.z = g_IOSRand.RandomFloat (-15 * shotStrength, 15 * shotStrength);
		QAngle angle(-5.0f, 0, 0);
		//why punch?
		//pPlayer->SetPunchAngle( angle );
		//play kick sound
		if (ballStatus != BALL_THROWIN_PENDING)
		{
			if (powershot==sv_ball_normalshot_strength.GetFloat())
				EmitSound("Ball.kicknormal");
			else
				EmitSound("Ball.kickhard");
		}

		if (ballrel	== BALL_NEAR_FEET)
		{
			//get height above ground using longwided method!
			Vector pos;
			float playerHeight, ballHeight;
			extern CNavMesh *TheNavMesh;
			TheNavMesh->GetSimpleGroundHeight(pPlayer->GetAbsOrigin(), &playerHeight);
			TheNavMesh->GetSimpleGroundHeight(GetAbsOrigin(), &ballHeight);
			playerHeight = pPlayer->GetAbsOrigin().z - playerHeight;
			ballHeight = GetAbsOrigin().z - ballHeight;

			//if (!(pPlayer->GetFlags() & FL_ONGROUND) && ballHeight > 9.0f && playerHeight > 6.0f)
			if (ballHeight > 9.0f && pPlayer->GetLocalVelocity().Length2D() < 50)
			{
				shotStrength = 720;
				vel.z *= 0.5f;

				if (!bInSlide)		//bot keepers dont volley 1.0c
				{
					if (!(pPlayer->GetFlags() & FL_FAKECLIENT))
					{
						((CSDKPlayer*)pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_VOLLEY );
						pPlayer->SetAnimation (PLAYER_VOLLEY);
					}
					else
					{
						((CSDKPlayer*)pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_JUMP );
						pPlayer->SetAnimation (PLAYER_JUMP);
					}
				}
			}
			else if (ang.x <= -50.0f)
			{
				//check for heel kick
				Vector dir = GetAbsOrigin() - pPlayer->GetAbsOrigin();
				dir.z = 0.0f;
				VectorNormalize(dir);
				Vector fwd = v_forward;
				fwd.z = 0.0f;
				VectorNormalize(fwd);
				float dot = DotProduct( dir, fwd );

				if (!bInSlide)
				{
					//ball must be behind us
					if ( dot < -0.8f )
					{
						((CSDKPlayer*)pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_HEELKICK );
						pPlayer->SetAnimation (PLAYER_HEELKICK);
						shotStrength *= 0.9f;
						powershot=sv_ball_normalshot_strength.GetFloat();
					}
					else
					{
						pPlayer->SetAnimation (PLAYER_KICK);
						((CSDKPlayer*)pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_KICK );
					}
				}
			}
			else
			{
				if (!bInSlide)
				{
					pPlayer->SetAnimation (PLAYER_KICK);   //kick anim - does this have any effect!?
					((CSDKPlayer*)pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_KICK );

				}
			}
		}
		else if (ballrel == BALL_NEAR_HEAD)
		{
			if (!bInSlide)
			{
				//check for diving header
				if (	(pPlayer->m_nButtons & IN_FORWARD) 
					&& (pPlayer->GetAbsVelocity().Length2D() > 220)
					&& (pPlayer->m_TeamPos > 1) 
					&& (m_BallInPenaltyBox != -1) 
					&& (powershot==sv_ball_powershot_strength.GetFloat())
					&& ((CSDKPlayer*)pPlayer)->m_PlayerAnim != PLAYER_SLIDE )
				{
					((CSDKPlayer*)pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_DIVINGHEADER );
					pPlayer->SetAnimation (PLAYER_DIVINGHEADER);
					((CSDKPlayer*)pPlayer)->m_NextSlideTime = gpGlobals->curtime + 1.5f;
					shotStrength = 600;
					vel.z *= 0.1f;
				}

				//if (m_pKeeperParry == pPlayer)
				//{
				//	((CSDKPlayer*)pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_JUMP );
				//	pPlayer->SetAnimation (PLAYER_JUMP);
				//}
				//head anim
				//if in the air play jumping header anim
				else if (!(pPlayer->GetFlags() & FL_ONGROUND))
				{
					pPlayer->SetAnimation (PLAYER_HEADER);
					((CSDKPlayer*)pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_HEADER );
				}
			}
		}
	}

	//reduce strength if sprinting
	//don't reduce for now to make more predictable for players
	//if (((CSDKPlayer*)pPlayer)->InSprint())
	//	powershot *= 0.9f;

	vel.NormalizeInPlace();	//test - added Jan08
	vel *= shotStrength * powershot /** kickStrength*/;

	VPhysicsGetObject()->SetVelocity(&vel, NULL);								//kick it


	//SetBallCurve(pPlayer, vel, avel, v_right, v_up);
	
	m_NextShoot	= gpGlobals->curtime + 0.2f;

	//m_NextShoot	= gpGlobals->curtime + 0.1f;

	//if (pPlayer->m_fSprintLeft > 0.0f && powershot == sv_powershot_strength.GetFloat())
	//{
	//	pPlayer->m_fSprintLeft = max(0, pPlayer->m_fSprintLeft - 3);                   //reduce sprint energy
	//}

	//m_pKeeperParry = NULL;

	return true;
}

//static ConVar sv_ball_spin("sv_ball_spin", "10000", FCVAR_ARCHIVE);

void CBall::UpdateHeadbounce(CSDKPlayer *pPlayer)
{
	//float fHTimer = headbounceTimer.GetFloat();
	float fHCount = headbounceCount.GetFloat();

	if (ballStatus == 0) 
	{
		if (m_PlayerWhoHeaded==pPlayer)
		{
			m_HeadTouch++;

			//block this player only
			//if (fHTimer)
			//	pPlayer->m_NextShoot = gpGlobals->curtime + (fHTimer * m_HeadTouch);

			if (m_HeadTouch > fHCount)
			{
				pPlayer->ApplyTackledAnim(pPlayer);
				m_HeadTouch = 0;
				m_PlayerWhoHeaded = NULL;
			}
		} 
		else 
		{
			m_PlayerWhoHeaded = pPlayer;
			m_HeadTouch = 0;
		}
	}
}

#define VIEW_FIELD_60_DEGREES   0.5f
#define VIEW_FIELD_90_DEGREES   0.0f

//==========================================================
// FindTarget - find target to pass to.
//	
//==========================================================
CBaseEntity *CBall::FindTarget (CBaseEntity *pPlayer)
{
	CBaseEntity *pTarget = NULL;
	CBaseEntity *pClosest = NULL;
	Vector		vecLOS;
	float flMaxDot = VIEW_FIELD_90_DEGREES;   //b4		//float flMaxDot = VIEW_FIELD_WIDE;       //b3 (135)	//float flMaxDot = VIEW_FIELD_NARROW;     //b1 (45)
	float flDot;
	Vector	vForwardUs;

	AngleVectors (pPlayer->EyeAngles(),	&vForwardUs);	//UTIL_MakeVectors ( pOther->pev->v_angle );// so we know which way player (pOther)
	
	while ((pTarget = gEntList.FindEntityInSphere( pTarget, pPlayer->GetAbsOrigin(), 4096 )) != NULL)
	{
		if (pTarget->IsPlayer()
			&& pPlayer->IsPlayer()
			&& ((CSDKPlayer*)pTarget)->m_TeamPos > 1 
			&& ((CSDKPlayer*)pTarget)->GetTeamNumber()==((CSDKPlayer*)pPlayer)->GetTeamNumber()
			&& ((CSDKPlayer*)pTarget)->IsOnPitch()
			) {

			//vecLOS = (VecBModelOrigin( pObject->pev ) - (pOther->GetAbsOrigin() + pOther->pev->view_ofs));	
			//vecLOS = UTIL_ClampVectorToBox( vecLOS, pObject->pev->size * 0.5 );
			vecLOS = pTarget->GetAbsOrigin() - pPlayer->GetAbsOrigin();
			vecLOS.z = 0.0f;		//remove the UP (Z!!)
			VectorNormalize(vecLOS);
			if (vecLOS.x==0.0f && vecLOS.y==0.0f)
				continue;
			flDot = DotProduct (vecLOS, vForwardUs);
			if (flDot > flMaxDot)
			{  
				flMaxDot = flDot;	//find nearest thing in view (using dist means not neccessarily the most direct onfront)
         		pClosest = pTarget;
			}
		}
	}
	return pClosest;
}

//==========================================================
//
//	
//==========================================================
int CBall::Pass ( CBaseEntity *pOther)
{
   Vector      dist;
   CBaseEntity *pEntity = NULL;
   float       length  = 0.0f;
   int         throwout = false;

   if (m_NextPass > gpGlobals->curtime)
      return false;

   if (ballStatus == BALL_THROWIN_PENDING)
      return false;

   if (ballStatus == BALL_PENALTY_PENDING)
      return false;

   	bool	bInSlide = ((CSDKPlayer*)pOther)->m_PlayerAnim == PLAYER_SLIDE ? 1 : 0;

   throwout = CheckKeeperKick ((CSDKPlayer*)pOther);   //see if we're holding a ball and should pass now

   pEntity = FindTarget (pOther);

   //is it a teammate etc?
   if (pEntity && pEntity->IsPlayer() && pEntity != pOther) 
   {
		Vector ballVel, ballAvel;
		VPhysicsGetObject()->GetVelocity( &ballVel, &ballAvel );

		Vector OriginThru;
		//find point ahead of target player using their current movement direction
		//float mag = pEntity->pev->velocity.Length2D();
		float mag = pEntity->GetAbsVelocity().Length2D();
		Vector unitvel = pEntity->GetAbsVelocity();
		VectorNormalize(unitvel);
		unitvel.x *= 1.2; unitvel.y *= 1.2;                   //lookahead more (beta3)
		OriginThru.x = pEntity->GetAbsOrigin().x + unitvel.x * mag; 
		OriginThru.y = pEntity->GetAbsOrigin().y + unitvel.y * mag; 
		OriginThru.z = 0.0f;
		dist = OriginThru - GetAbsOrigin();
		dist.z = 0.0f;
		length = sqrt( dist.x * dist.x + dist.y * dist.y );
		ballVel.z = 0;     //beta3 damp volley passes - this gets overwritten by the next line?!?!?
		ballVel = dist;
		VectorNormalize(ballVel);
		//use vertical angle to pass in air or on ground
		//UTIL_MakeVectors ( pOther->pev->v_angle );
		Vector vForward;
		AngleVectors (pOther->EyeAngles(), &vForward);

		//if it's a keeper diving then damp the pass
		//TODOif ( pOther && (((CSDKPlayer*)pOther)->m_TeamPos == 1) && !(((CSDKPlayer*)pOther)->pev->flags & FL_ONGROUND) )
		//   length = 600;

		//clamp length before pass specific modifiers
		if (length < 700)
			length = 700;      //min pass strength  (was 450 before beta4)

		if (length > 900)
			length = 900;      //max pass strength	(was 850 before 1.0d)

		//pass looking down
		//if (gpGlobals->v_forward.z < 0.0f) {
		if (vForward.z < 0.0f) 
		{
			//pev->velocity.z = 0.20;
			ballVel.z = 0.2f;
			if (GetFlags() & FL_ONGROUND) 
			{        
				//length *= 0.95f;                                //beta3 boost ground pass strength
			} 
			else 
			{
				if (GetRelativeBallPos (pOther)!=BALL_NEAR_FEET && !throwout)     //throw/pass ball out strong
					length *= 0.45f;                                   //weaker pass if headed
				//else
				//   length *= 0.65f;
			}
		}
		else
		{
			//pass looking up
			//ballVel.z += vForward.z * 6.0f;							//pev->velocity.z += gpGlobals->v_forward.z * 6;  //* 3;   //beta2
			ballVel.z = vForward.z * 2.5f;		//make z aim more sensitive ; 
			if (GetRelativeBallPos (pOther)!=BALL_NEAR_FEET && !throwout)
				length *= 0.45f;                                    //weaker pass if headed
			//else
			//   length *= 0.65;
		}

		//if (length < 200)     //beta2
		//   length = 200;      //min pass strength

		//pev->velocity = pev->velocity * length;

		ballVel.NormalizeInPlace();
		ballVel = ballVel * length;

		//ioss - damp final mag to stop high shots being too strong
		//float finalMag = ballVel.Length();
		//if (finalMag > 1000)
		//	ballVel *= (1000/finalMag);


		//TODOPlayPassSound(pOther);
		if (pOther->IsPlayer() /*TODO&& !(pOther->pev->iuser4 & 1)*/) //check not slidetackling
		{
			//dont change anim for very small touches - like miss-hits etc
			if (length > 650)
			{
				int ballPos = GetRelativeBallPos (pOther);
				if (ballPos==BALL_NEAR_FEET || throwout)
				{
					if (!bInSlide)
					{
						//pass when moving
						if (pOther->GetAbsVelocity().Length2D() > 0)
							((CSDKPlayer*)pOther)->DoAnimationEvent( PLAYERANIMEVENT_PASS );
						else
							((CSDKPlayer*)pOther)->DoAnimationEvent( PLAYERANIMEVENT_PASS_STATIONARY );
					}
				}
				else if (ballPos==BALL_NEAR_HEAD /*TODO&& !(pOther->pev->iuser4 & 1) && !((CSDKPlayer*)pOther)->Tackled*/)   //4.0a check for death by headtouch before playing this anim
				{
					//header when moving
					//TODO:
					//if (pOther->GetAbsVelocity().Length2D() > 0)
					//	((CSDKPlayer*)pOther)->SetAnimation (PLAYER_HEADER);    //b4 pass anim
					//else
					//	((CSDKPlayer*)pOther)->SetAnimation (PLAYER_HEADER_STATIONARY);    //b4 pass anim
				}
			}
			((CSDKPlayer*)pOther)->m_Passes++;

		}
		VPhysicsGetObject()->SetVelocity(&ballVel, &ballAvel);								//kick it
		EmitSound("Ball.kicknormal");
		m_NextPass = gpGlobals->curtime + 0.4f;		//beta4 0.4f;     //was 0.2f b1-b3a
		return true;
	}
	else
	{
		//no-one to pass to, do a regular kick
		return Shoot(pOther);
	}

	return false;
}





//static ConVar sv_ball_curve("sv_ball_curve", "1.0", FCVAR_ARCHIVE);

//==========================================================
//	
//	
//==========================================================
/*
void CBall::BallThink( void	)
{
	RunCheck();
	//if (m_FreezeBallTime > gpGlobals->curtime)
	//{
	//	Vector zero = Vector(0.0f, 0.0f, 0.0f);
	//	const QAngle zeroAng = QAngle(0.0f, 0.0f, 0.0f);
	//	VPhysicsGetObject()->SetPosition(m_FreezeBallPos, zeroAng, true);
	//	VPhysicsGetObject()->SetVelocityInstantaneous( &zero, &zero);
	//}

	//try to fix bug where ball continues to curve! should get cleared in vphysicscollision
	//if (GetFlags() & FL_ONGROUND)
	//	curve.x = curve.y = curve.z = 0.0f;

	//QAngle angles;
	//VPhysicsGetObject()->GetPosition(NULL, &angles);
	//AngularImpulse ang = WorldToLocalRotation( SetupMatrixAngles(angles), m_vRot, sv_ball_spin.GetInt() );
	//VPhysicsGetObject()->SetVelocity(NULL, &ang);

	Vector vel;
	AngularImpulse angImp;
	VPhysicsGetObject()->GetVelocity(&vel, &angImp);
	//Vector rot = LocaltoWorld(
	Vector curve = vel.Cross(angImp);
	//QAngle angles;
	//VectorAngles(angImp, angles);
	//VectorNormalizeFast(curve);
	vel += curve / sv_ball_curve.GetFloat();
//	VPhysicsGetObject()->SetVelocity(&vel, NULL);

	////curve in air
	//Vector ballVel, ballAvel;
	//VPhysicsGetObject()->GetVelocity( &ballVel, &ballAvel );
	////ballVel += curve * 1.0f;
	//ballVel += curve * 0.1667f;							//6x faster update 1.0c
	//VPhysicsGetObject()->SetVelocity(&ballVel, NULL);

	//SetNextThink( gpGlobals->curtime + 0.1f );
	SetNextThink( gpGlobals->curtime + 0.01667f );		//6x faster update 1.0c

	if (KeepersBall())
		return;

	if (ballStatus)
		ProcessStatus();


	//this didn't seem to move the ball when it was excuted. - it crashes the server when thrown into crowd!
	//if ball rolls off head put it back on
	//if (ballStatus == BALL_THROWIN_PENDING && GetAbsOrigin().z < m_FoulPos.z - 10) 
	//{
	//	Warning ("resetting\n");
	//	SetAbsOrigin(m_FoulPos);
	//	Vector zero = Vector(0.0f,0.0f,0.0f);
	//	VPhysicsGetObject()->SetVelocityInstantaneous( &zero, &zero);
	//}


	UpdateBallShield();

	m_BallInPenaltyBox  = -1;
}
*/
/*
void CBall::BallTouch( CBaseEntity *pOther )
{
	if (pOther->IsPlayer())
	{
		KickedBall((CSDKPlayer*)pOther, false);		//a touch

		//ignore touches unless it's in normal play
		if (ballStatus == 0)
		{
			Vector vel, avel;
			vel = pOther->GetAbsVelocity() * 0.1f;
			vel.z = 0.0f;
			VPhysicsGetObject()->AddVelocity(&vel, &avel);
		}
	}
}*/


//==========================================================
// See if the ball is near the players feet, head
// or elswehere
//==========================================================
int	CBall::GetRelativeBallPos	(CBaseEntity *pOther)
{
   //keepers can always	kick
   if (pOther->GetFlags() &	FL_FAKECLIENT)
	  return BALL_NEAR_FEET;

   //check ball	pos	to player
   if (GetAbsOrigin().z < pOther->GetAbsOrigin().z + 30.0f)
   {
	  //DevMsg ( "low\n" );
	  return BALL_NEAR_FEET;
   } 
   else if (GetAbsOrigin().z > pOther->GetAbsOrigin().z + 50.0f) 
   {
	  //DevMsg ( "high\n" );
	  return BALL_NEAR_HEAD;
   }
   else
   {
	  //DevMsg ( "med\n" );
	  return BALL_ELSEWHERE;
   }
}


//==========================================================
//	Sends a VGUIMenu message to everyone.
// 
//==========================================================
void CBall::SendVGUIStatusMessage (const char *title, const char *text, bool bShow, bool bMainStatus)
{
	CReliableBroadcastRecipientFilter filter;
	char name[256] = "status";

	//send big display? (default is the status bar display)
	if (bMainStatus)
		strcpy (&name[0], "mainstatus");

	KeyValues *data = new KeyValues("data");
	data->SetString( "title", title );			// info panel title
	//data->SetString( "type", "1" );			// show userdata from stringtable entry
	data->SetString( "msg",	text );				// use this stringtable entry
	//data->SetString( "cmd", "impulse 101" );	// exec this command if panel closed

	int count = 0;
	KeyValues *subkey = NULL;

	if ( data )
	{
		subkey = data->GetFirstSubKey();
		while ( subkey )
		{
			count++; subkey = subkey->GetNextKey();
		}

		subkey = data->GetFirstSubKey(); // reset 
	}

	UserMessageBegin( filter, "VGUIMenu" );
		WRITE_STRING( name ); // menu name
		WRITE_BYTE( bShow?1:0 );
		WRITE_BYTE( count );
		
		// write additional data (be carefull not more than 192 bytes!)
		while ( subkey )
		{
			WRITE_STRING( subkey->GetName() );
			WRITE_STRING( subkey->GetString() );
			subkey = subkey->GetNextKey();
		}
	MessageEnd();
}


//==========================================================
//	Display the big status according to ball status
// 
//==========================================================
void CBall::SendMainStatus(int other)
{
	char	title[16] = "MAIN";
	char	text[256] = "";

	switch (ballStatus)
	{
		case BALL_GOAL:
			strcpy (&text[0], "GOAL!");
		break;
		case BALL_CORNER:
			strcpy (&text[0], "CORNER");
		break;
		case BALL_GOALKICK:
			strcpy (&text[0], "GOAL KICK");
		break;
		case BALL_THROWIN:
			strcpy (&text[0], "THROW IN");
		break;
		case BALL_FOUL:
			//check kind of foul
			if (other == BALL_MAINSTATUS_YELLOW)
				strcpy (&text[0], "YELLOW");
			else if (other == BALL_MAINSTATUS_RED)
				strcpy (&text[0], "RED");
			else if (other == BALL_MAINSTATUS_OFFSIDE)
				strcpy (&text[0], "OFFSIDE");
			else
				strcpy (&text[0], "FOUL");
		break;

		case BALL_PENALTY:
			strcpy (&text[0], "PENALTY!");
		break;

		default:
			if (other == BALL_MAINSTATUS_PLAYON)
				strcpy (&text[0], "PLAY ON");
			if (other == BALL_MAINSTATUS_FINAL_WHISTLE)
				strcpy (&text[0], "FINAL WHISTLE");
		break;
	}

	SendVGUIStatusMessage (title, text, true, true);
}


//==========================================================
//
// 
//==========================================================
void CBall::ProcessStatus (void)
{

	//check for play on
	if (ballStatus >= BALL_GOALKICK_PENDING)
	{
		if (ballStatusTime < gpGlobals->curtime)
		{
			SendMainStatus(BALL_MAINSTATUS_PLAYON);
			ballStatus = 0;
			if (m_BallShieldPlayer)
				m_BallShieldPlayer->GiveRedCard();
			ShieldOff();
			EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");
			m_DoubleTouchCount = 0;
			m_PlayerWhoTook = NULL;
			RemoveFlagFromAll(FL_ATCONTROLS);
		}
	}

	//if ball status then dont count in possession
	if (ballStatus!=0)
		m_possStart = gpGlobals->curtime;


	switch (ballStatus)
	{
		case BALL_GOAL:
		{
			HandleGoal();
			break;
		}

		case BALL_KICKOFF:
		{
			HandleKickOff();
			break;
		}

		case BALL_CORNER:
		{
			HandleCorner();
			break;
		}

		case BALL_GOALKICK:
		{
			HandleGoalkick();
			break;
		}

		case BALL_THROWIN:
		{
			HandleThrowin();
			break;
		}

		case BALL_FOUL:
		{
			HandleFoul();
			break;
		}

		case BALL_PENALTY:
		{
			HandlePenalty();
			break;
		}
	}
}



const static float CELEBRATION_TIME = 10.0f;
const static float BALL_STATUS_TIME = 15.0f;

//==========================================================
// Do this when a goal first happens
// 
//==========================================================
void CBall::HandleGoal(void)
{
	bool ownGoal = false;

	DropBall();

	//ballStatus = BALL_KICKOFF;
	//EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");

	//m_KickOffTime = gpGlobals->curtime + CELEBRATION_TIME;
	//ballStatusTime = gpGlobals->curtime + CELEBRATION_TIME;


	//award goal
	CSDKPlayer	*scorer = NULL;
	if (m_LastPlayer) 
		scorer = m_LastPlayer;

	//see if it was a keeper and if they made an own goal (so not really them)
	if (scorer && scorer->m_TeamPos == 1 && scorer->GetTeamNumber() != m_TeamGoal) 
	{
		scorer = m_LastNonKeeper;
		//if this fails then try last player once more
		if (!scorer)
			scorer = m_LastPlayer;
	}

	if (!scorer)
	{
		//m_TeamGoal = 0;
		return;
	}

	ballStatusTime = gpGlobals->curtime + CELEBRATION_TIME;
	SendMainStatus();
	ballStatus = BALL_KICKOFF;
	EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");

	if (scorer->GetTeamNumber() == m_TeamGoal)
	{
		//give scorer a goal
		scorer->IncrementFragCount( 1 );
		scorer->AddPointsToTeam( 1, false );
	}
	else
	{
		ownGoal = true;
		//add points directly to other team...
		GetGlobalTeam( m_TeamGoal )->AddScore( 1 );
	}

	//send vgui text over
	const char	title[16] = "GOAL!\n";
	char	msg[64];
	if (!ownGoal)
	{
		Q_snprintf (msg, sizeof (msg), "%s scored! ", scorer->GetPlayerName());
		SendVGUIStatusMessage (title, msg, true);

		CheckForAssist(scorer);

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"goal\"\n", 
					scorer->GetPlayerName(), scorer->GetUserID(),
					scorer->GetNetworkIDString(), scorer->GetTeam()->GetName() );

	}
	else
	{
		Q_snprintf (msg, sizeof (msg), "%s scored an own goal. ", scorer->GetPlayerName());
		SendVGUIStatusMessage (title, msg, true);

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"owngoal\"\n", 
					scorer->GetPlayerName(), scorer->GetUserID(),
					scorer->GetNetworkIDString(), scorer->GetTeam()->GetName() );
	}


	//only do if it's a stadium
	if (gEntList.FindEntityByClassname( NULL, "info_stadium" ))
		scorer->EmitAmbientSound(scorer->entindex(), scorer->GetAbsOrigin(), "Player.Goal");

	EnableCeleb();

}


//==========================================================
// Set the "celeb" flag for everyone on the scoring team.
// So client anims can change
//==========================================================
void CBall::EnableCeleb(void)
{

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex (i);

		if (!pPlayer)
			continue;

		if (pPlayer->GetTeamNumber() != m_TeamGoal)
			continue;

		pPlayer->AddFlag(FL_CELEB);
	}

}

void CBall::DisableCeleb(void)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex (i);

		if (!pPlayer)
			continue;

		if (pPlayer->GetTeamNumber() != m_TeamGoal)
			continue;

		pPlayer->RemoveFlag(FL_CELEB);
	}
}



//==========================================================
// Do this when a corner first happens
// 
//==========================================================
void CBall::HandleCorner(void)
{
	if (ballStatusTime > gpGlobals->curtime)
		return;

	//find a valid player
	CBaseEntity *playerToTake = NULL;
	if (m_Foulee) 
	{
		playerToTake = m_Foulee;                        //use player near when corner created
		m_Foulee = NULL;
	}
	else 
	{
		playerToTake = FindPlayerForAction (m_team,0);
	}
	//teleport to corner (correct side) - Ball & Player
	CBaseEntity *pSpot=NULL;
	CBaseEntity *pSpotPlayer=NULL;

	if (m_team == TEAM_A) 
	{
		if (m_side==0) 
		{
			pSpot = gEntList.FindEntityByClassname( NULL, "info_team2_corner0" );
			pSpotPlayer = gEntList.FindEntityByClassname( NULL, "info_team2_corner_player0" );
		} 
		else 
		{
			pSpot = gEntList.FindEntityByClassname( NULL, "info_team2_corner1" );
			pSpotPlayer = gEntList.FindEntityByClassname( NULL, "info_team2_corner_player1" );
		}
	} else {
		if (m_side==0) 
		{
			pSpot = gEntList.FindEntityByClassname( NULL, "info_team1_corner0" );
			pSpotPlayer = gEntList.FindEntityByClassname( NULL, "info_team1_corner_player0" );
		} 
		else 
		{
			pSpot = gEntList.FindEntityByClassname( NULL, "info_team1_corner1" );
			pSpotPlayer = gEntList.FindEntityByClassname( NULL, "info_team1_corner_player1" );
		}
	}

	//move ball to corner
	if (pSpot) 
	{
		VPhysicsGetObject()->SetPosition(pSpot->GetAbsOrigin(), pSpot->GetAbsAngles(), true);
		Vector zero = Vector(0.0f,0.0f,0.0f);
		VPhysicsGetObject()->SetVelocityInstantaneous( &zero, &zero);
	}
	//move player to corner
	if (playerToTake && pSpotPlayer) 
	{
		Vector origin = pSpotPlayer->GetAbsOrigin();
		origin.z = playerToTake->GetAbsOrigin().z;
		playerToTake->SetAbsOrigin(origin);

		playerToTake->SetAbsAngles (pSpotPlayer->GetAbsAngles());
		((CSDKPlayer*)playerToTake)->SnapEyeAngles( pSpotPlayer->GetAbsAngles() );

		playerToTake->RemoveFlag(FL_ATCONTROLS);        //incase happened in slide
		//put up ball shield
		ShieldBall ((CSDKPlayer*)playerToTake);
		//m_NextShoot = gpGlobals->curtime + KICK_DELAY;			//cant kick right away
		((CSDKPlayer*)playerToTake)->m_KickDelay = gpGlobals->curtime + kickDelay.GetFloat();
		playerToTake->AddFlag(FL_FROZEN);


		((CSDKPlayer*)playerToTake)->m_Corners++;

		//start timeout/foul
		//ballStatus = BALL_CORNERKICK_PENDING;
		//ballStatusTime = gpGlobals->curtime + BALL_STATUS_TIME;

		char  msg[64];
		const char	title[16] = "CORNER\n";
		Q_snprintf (msg, sizeof (msg), "%s to take corner. ", ((CSDKPlayer*)playerToTake)->GetPlayerName());
		SendVGUIStatusMessage (title, msg, true);

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"corner\"\n", 
					((CSDKPlayer*)playerToTake)->GetPlayerName(), ((CSDKPlayer*)playerToTake)->GetUserID(),
					((CSDKPlayer*)playerToTake)->GetNetworkIDString(), ((CSDKPlayer*)playerToTake)->GetTeam()->GetName() );
	} 
	//else 
	//{
	//	ballStatus = 0;                              //no one to take corner - just finish - (this may retrigger the corner)
	//}

	ballStatus = BALL_CORNERKICK_PENDING;
	ballStatusTime = gpGlobals->curtime + BALL_STATUS_TIME;
	EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");
}



//==========================================================
// Do this when a goal kick first happens
// 
//==========================================================
void CBall::HandleGoalkick(void)
{
	if (ballStatusTime > gpGlobals->curtime)
		return;

	CSDKPlayer *playerToTake = NULL;
	playerToTake = FindKeeperForAction (m_team);
	//move ball to position
	CBaseEntity *pSpot=NULL;
	//get keeper position info because it gets lost in teamplay spawning etc.
	if (m_team == TEAM_A) 
	{
		if (m_side==0)
			pSpot = gEntList.FindEntityByClassname( NULL, "info_team1_goalkick0" );
		else
			pSpot = gEntList.FindEntityByClassname( NULL, "info_team1_goalkick1" );
	} 
	else 
	{
		if (m_side==0)
			pSpot = gEntList.FindEntityByClassname( NULL, "info_team2_goalkick0" );
		else
			pSpot = gEntList.FindEntityByClassname( NULL, "info_team2_goalkick1" );
	}

	if (pSpot) 
	{
		VPhysicsGetObject()->SetPosition(pSpot->GetAbsOrigin(), pSpot->GetAbsAngles(), true);
		Vector zero = Vector(0.0f,0.0f,0.0f);
		VPhysicsGetObject()->SetVelocityInstantaneous( &zero, &zero);
	}

	//nominate player and exclude others from ball area
	//set pending mode
	if (playerToTake) 
	{
		//ballStatus = BALL_GOALKICK_PENDING;
		//ballStatusTime = gpGlobals->curtime + BALL_STATUS_TIME;     //time to take the goal kick
		ShieldBall (playerToTake);

		ApplyKickDelayToAll();										//1.0c stop players stealing GK

		//m_NextShoot = gpGlobals->curtime + KICK_DELAY;			//cant kick right away

		playerToTake->m_GoalKicks++;

		char  msg[64];
		const char	title[16] = "GOALKICK\n";
		Q_snprintf (msg, sizeof (msg), "%s to take goal kick. ", playerToTake->GetPlayerName());
		SendVGUIStatusMessage (title, msg, true);

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"goalkick\"\n", 
					playerToTake->GetPlayerName(), playerToTake->GetUserID(),
					playerToTake->GetNetworkIDString(), playerToTake->GetTeam()->GetName() );
	} 
	//else 
	//{
	//	ballStatus = 0;
	//}

	ballStatus = BALL_GOALKICK_PENDING;
	ballStatusTime = gpGlobals->curtime + BALL_STATUS_TIME;     //time to take the goal kick
	EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");
	ClrAssists();
}


//==========================================================
// Do this when a goal kick first happens
// 
//==========================================================
void CBall::HandleThrowin(void)
{
	if (ballStatusTime > gpGlobals->curtime)
		return;

	//find a valid player
	CSDKPlayer *playerToTake = NULL;
	if (m_Foulee) 
	{
		playerToTake = m_Foulee;                        //use player near when throwin created
		m_Foulee = NULL;
	}
	else 
	{
		playerToTake = (CSDKPlayer*)FindPlayerForAction (m_team,0);  //or find someone else
	}

	//teleport Ball & Player
	CBaseEntity *pThrowIn=NULL;
	pThrowIn = FindThrowInEnt();
	if (playerToTake && pThrowIn) 
	{
		((CSDKPlayer*)playerToTake)->SetAnimation (PLAYER_THROWIN);
		((CSDKPlayer*)playerToTake)->DoAnimationEvent( PLAYERANIMEVENT_THROWIN );
		((CSDKPlayer*)playerToTake)->m_HoldAnimTime = gpGlobals->curtime + BALL_STATUS_TIME;	//hold player still

		playerToTake->SetLocalOrigin( pThrowIn->GetAbsOrigin() + Vector(0,0,1) );
		playerToTake->SetAbsOrigin( pThrowIn->GetAbsOrigin() + Vector(0,0,1) );
		playerToTake->SetAbsVelocity( vec3_origin );
		playerToTake->SetLocalAngles( pThrowIn->GetLocalAngles() );
		playerToTake->SnapEyeAngles( pThrowIn->GetLocalAngles() );

		playerToTake->SetAbsAngles(pThrowIn->GetAbsAngles());
		((CSDKPlayer*)playerToTake)->SnapEyeAngles( pThrowIn->GetAbsAngles() );
		playerToTake->AddFlag(FL_ATCONTROLS);
		playerToTake->SetAbsVelocity( vec3_origin );

		//put ball on players head
		Vector ballOrigin = playerToTake->GetAbsOrigin();
		ballOrigin.z += 82;
		VPhysicsGetObject()->SetPosition(ballOrigin, pThrowIn->GetAbsAngles(), true);
		Vector zero = Vector(0.0f,0.0f,0.0f);
		VPhysicsGetObject()->SetVelocityInstantaneous( &zero, &zero);

		////tells ballthink to try and keep the ball at this pos. stops it rolling?
		//m_FreezeBallTime = gpGlobals->curtime + 0.1f;
		//m_FreezeBallPos = ballOrigin;

		//m_FoulPos = ballOrigin;      //save the ball's throwin pos on players head

		//put up ball shield
		ShieldBall (playerToTake);
		//m_NextShoot = gpGlobals->curtime + KICK_DELAY;			//cant kick right away

		//this breaks player?!
		//((CSDKPlayer*)playerToTake)->m_KickDelay = gpGlobals->curtime + KICK_DELAY;
		//playerToTake->AddFlag(FL_FROZEN);

		(playerToTake)->m_ThrowIns++;

		//start timeout/foul
		ballStatus = BALL_THROWIN_PENDING;
		ballStatusTime = gpGlobals->curtime + BALL_STATUS_TIME;
		EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");

		char  msg[64];
		const char	title[16] = "THROWIN\n";
		Q_snprintf (msg, sizeof (msg), "%s to take throw in. ", playerToTake->GetPlayerName());
		SendVGUIStatusMessage (title, msg, true);

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"throwin\"\n", 
					(playerToTake)->GetPlayerName(), (playerToTake)->GetUserID(),
					(playerToTake)->GetNetworkIDString(), (playerToTake)->GetTeam()->GetName() );
	} 
	else 
	{
		ballStatus = 0;                              //noone or no mapents to take throwin so quit
	}
}


//==========================================================
// Do this when a foul first happens
// 
//==========================================================
void CBall::HandleFoul(void)
{
	if (ballStatusTime > gpGlobals->curtime)
		return;

	//find a valid player
	CBaseEntity *playerToTake = NULL, *pSpot=NULL;
	CSDKPlayer *pKeeper = FindKeeperForAction (m_team);

	//if it's in our box pick our keeper (human only)
	if ((m_FoulInPenaltyBox != -1)  && pKeeper && (m_FoulInPenaltyBox==pKeeper->GetTeamNumber()) && !(pKeeper->GetFlags() & FL_FAKECLIENT))
		playerToTake = pKeeper;							//use keeper if it's in his box
	else if (m_Foulee)			
		playerToTake = m_Foulee;						//person who was fouled
	else 
		playerToTake = FindPlayerForAction (m_team,0);  //or find someone else

	m_Foulee = NULL;
	m_FoulInPenaltyBox = -1;

	//find 'shooting' direction so we know which way to take free kick
	if (m_team==TEAM_A) 
	{
		//shoot to opposite goalie direction
		pSpot = gEntList.FindEntityByClassname( NULL, "info_team2_player1" );	//team b
	}
	else 
	{
		pSpot = gEntList.FindEntityByClassname( NULL, "info_team1_player1" );	//team a
	}


	//teleport Ball & Player
	if (playerToTake && pSpot) 
	{
		//facing opposition
		Vector dir = (pSpot->GetAbsOrigin() - m_FoulPos);	//vec from foul to other keeper
		VectorNormalize(dir);

		//move back from opposition
		Vector pos = m_FoulPos;
		pos.x = (m_FoulPos.x - (74*dir.x));
		pos.y = (m_FoulPos.y - (74*dir.y));
		playerToTake->SetAbsOrigin(pos);

		QAngle goal_angle, playerAngle;
		VectorAngles(dir, goal_angle);
		playerAngle = playerToTake->GetAbsAngles();
		playerAngle.y = goal_angle.y;
		playerToTake->SetAbsAngles (playerAngle);
		((CSDKPlayer*)playerToTake)->SnapEyeAngles( playerAngle );

		VPhysicsGetObject()->SetPosition(m_FoulPos, playerAngle, true);		//place the ball, the angle doesnt matter so use players
		Vector zero = Vector(0.0f,0.0f,0.0f);
		VPhysicsGetObject()->SetVelocityInstantaneous( &zero, &zero);

		//tells ballthink to try and keep the ball at this pos. stops it rolling?
		m_FreezeBallTime = gpGlobals->curtime + 0.1f;
		m_FreezeBallPos = m_FoulPos;

		//put up ball shield
		ShieldBall ((CSDKPlayer*)playerToTake);
		//m_NextShoot = gpGlobals->curtime + KICK_DELAY;			//cant kick right away
		((CSDKPlayer*)playerToTake)->m_KickDelay = gpGlobals->curtime + kickDelay.GetFloat();
		playerToTake->AddFlag(FL_FROZEN);

		//start timeout/foul
		ballStatus = BALL_FOUL_PENDING;
		ballStatusTime = gpGlobals->curtime + BALL_STATUS_TIME;

		char  msg[64];
		const char	title[16] = "FOUL\n";
		Q_snprintf (msg, sizeof (msg), "%s to take free kick. ", ((CSDKPlayer*)playerToTake)->GetPlayerName());
		SendVGUIStatusMessage (title, msg, true);

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"freekick\"\n", 
					((CSDKPlayer*)playerToTake)->GetPlayerName(), ((CSDKPlayer*)playerToTake)->GetUserID(),
					((CSDKPlayer*)playerToTake)->GetNetworkIDString(), ((CSDKPlayer*)playerToTake)->GetTeam()->GetName() );
	} 
	else 
	{
		ballStatus = 0; 
	}

	EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");
	ClrAssists();
}



//==========================================================
//
// 
//==========================================================
void CBall::HandlePenalty(void)
{
	if (ballStatusTime > gpGlobals->curtime)
		return;

	//find a valid player
	CBaseEntity *playerToTake = NULL, *pSpot=NULL, *pPenaltySpot=NULL;
	if (m_Foulee) 
	{
		playerToTake = m_Foulee;
		m_Foulee = NULL;
	}
	else 
	{
		playerToTake = FindPlayerForAction (m_team,0);  //or find someone else
	}

	if (m_team==TEAM_A)
		FreezeKeepersForPenalty(TEAM_B);
	else
		FreezeKeepersForPenalty(TEAM_A);


	//find 'shooting' direction so we know which way to take free kick
	if (m_team==TEAM_A) 
	{
		//shoot to opposite goalie direction
		pSpot = gEntList.FindEntityByClassname( NULL, "info_team2_player1" );
		pPenaltySpot = gEntList.FindEntityByClassname( NULL, "info_team2_penalty_spot" );
	}
	else 
	{
		pSpot = gEntList.FindEntityByClassname( NULL, "info_team1_player1" );
		pPenaltySpot = gEntList.FindEntityByClassname( NULL, "info_team1_penalty_spot" );
	}


	//teleport Ball & Player
	if (playerToTake && pSpot && pPenaltySpot) 
	{
		//facing opposition
		Vector dir = (pSpot->GetAbsOrigin() - pPenaltySpot->GetAbsOrigin());
		VectorNormalize(dir);

		//move back from opposition
		Vector pos = pPenaltySpot->GetAbsOrigin();
		pos.x = (pos.x - (256*dir.x));
		pos.y = (pos.y - (256*dir.y));
		playerToTake->SetAbsOrigin(pos);

		//face opposition
		QAngle goal_angle, playerAngle;
		VectorAngles(dir, goal_angle);
		playerAngle = playerToTake->GetAbsAngles();
		playerAngle.y = goal_angle.y;
		playerToTake->SetAbsAngles (playerAngle);
		((CSDKPlayer*)playerToTake)->SnapEyeAngles( playerAngle );

		VPhysicsGetObject()->SetPosition(pPenaltySpot->GetAbsOrigin(), playerAngle, true);		//place the ball, the angle doesnt matter so use players
		Vector zero = Vector(0.0f,0.0f,0.0f);
		VPhysicsGetObject()->SetVelocityInstantaneous( &zero, &zero);

		//tells ballthink to try and keep the ball at this pos. stops it rolling?
		m_FreezeBallTime = gpGlobals->curtime + 0.1f;
		m_FreezeBallPos = pPenaltySpot->GetAbsOrigin();

		//put up ball shield
		ShieldBall ((CSDKPlayer*)playerToTake);
		//m_NextShoot = gpGlobals->curtime + KICK_DELAY;			//cant kick right away
		((CSDKPlayer*)playerToTake)->m_KickDelay = gpGlobals->curtime + kickDelay.GetFloat();
		playerToTake->AddFlag(FL_FROZEN);

		//start timeout/foul
		ballStatus = BALL_PENALTY_PENDING;
		ballStatusTime = gpGlobals->curtime + BALL_STATUS_TIME;
		EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");

		char  msg[64];
		const char	title[16] = "PENALTY\n";
		Q_snprintf (msg, sizeof (msg), "%s to take penalty. ", ((CSDKPlayer*)playerToTake)->GetPlayerName());
		SendVGUIStatusMessage (title, msg, true);

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"penalty\"\n", 
					((CSDKPlayer*)playerToTake)->GetPlayerName(), ((CSDKPlayer*)playerToTake)->GetUserID(),
					((CSDKPlayer*)playerToTake)->GetNetworkIDString(), ((CSDKPlayer*)playerToTake)->GetTeam()->GetName() );

		((CSDKPlayer*)playerToTake)->m_Penalties++;
	} 
	else 
	{
		ballStatus = 0;                              //noone or no mapents to take throwin so quit
	}

	ClrAssists();
}





//==========================================================
// Kick off - reset pos and teleport player to kick
// 
//==========================================================
void CBall::HandleKickOff ( void )
{
	if (ballStatusTime > gpGlobals->curtime)
		return;

   //if in penaly shootout mode don't do a kickoff after a goal.
   //if (m_psMode != 0)
   //{
   //   ballStatus = 0;
   //   return;
   //}
	((CSDKGameRules*)g_pGameRules)->AutobalanceTeams();


	//reset everyone, then choose kicker
	RespawnPlayers();

	//remove celeb anims
	DisableCeleb();

	CBaseEntity *pSpot=NULL, *pSpotBall=NULL;
	//get keeper position info because it gets lost in teamplay spawning etc.
	pSpotBall = gEntList.FindEntityByClassname( NULL, "info_ball_start" );

	//this is always 0,0,0 position?!
	if (pSpotBall) 
	{
		SetAbsOrigin(pSpotBall->GetAbsOrigin());
		VPhysicsGetObject()->SetPosition(pSpotBall->GetLocalOrigin(), pSpotBall->GetAbsAngles(), false);
		Vector zero = Vector(0.0f,0.0f,0.0f);
		VPhysicsGetObject()->SetVelocityInstantaneous( &zero, &zero);

		//tells ballthink to try and keep the ball at this pos. stops it rolling?
		m_FreezeBallTime = gpGlobals->curtime + 0.1f;
		m_FreezeBallPos = pSpotBall->GetAbsOrigin();
	}

	//find a valid player
	CBaseEntity *playerToTake = NULL;

	//if coming from a sv_restart we dont have a team so pick a random one now
	if (m_KickOff != TEAM_A && m_KickOff != TEAM_B)
		m_KickOff = g_IOSRand.RandomInt(2,3);

	playerToTake = FindPlayerForAction (m_KickOff,0);

	//find 'shooting' direction so we know which way to kick
	pSpot = NULL;
	if (m_KickOff==TEAM_A) 
	{
		pSpot = gEntList.FindEntityByClassname( NULL, "info_team2_player1" );	//shoot to opposite goalie direction
	} 
	else 
	{
		pSpot = gEntList.FindEntityByClassname( NULL, "info_team1_player1" );
	}

	m_KickOff = -1;

	if (playerToTake && pSpot) 
	{
		Vector dir = (pSpot->GetAbsOrigin() - pSpotBall->GetAbsOrigin());	//vector stright up the pitch
		VectorNormalize(dir);
		QAngle goal_angle;
		VectorAngles(dir, goal_angle);
		Vector pos = playerToTake->GetAbsOrigin();
		Vector origin = pSpotBall->GetAbsOrigin();
		origin.z = pos.z;
		if (dir.x > 0)
			origin.x -= 70;								//move to side of ball
		else
			origin.x += 70;
		origin.y -= (32.0f*dir.y);
		playerToTake->SetAbsOrigin(origin);

		//face the ball
		QAngle spawntoball_angle, playerAngle;
		VectorAngles( pSpotBall->GetAbsOrigin() - playerToTake->GetAbsOrigin(),  spawntoball_angle);
		playerAngle = playerToTake->GetAbsAngles();
		playerAngle.y = spawntoball_angle.y;
		playerToTake->SetAbsAngles (playerAngle);
		((CSDKPlayer*)playerToTake)->SnapEyeAngles( playerAngle );

		playerToTake->RemoveFlag(FL_ATCONTROLS);        //incase happened in slide
		//put up ball shield
		ShieldBall ((CSDKPlayer*)playerToTake);
		//m_NextShoot = gpGlobals->curtime + KICK_DELAY;			//cant kick right away
		((CSDKPlayer*)playerToTake)->m_KickDelay = gpGlobals->curtime + kickDelay.GetFloat();
		playerToTake->AddFlag(FL_FROZEN);

		//start timeout/foul
		ballStatus = BALL_KICKOFF_PENDING;
		ballStatusTime = gpGlobals->curtime + BALL_STATUS_TIME;     //time to take the kick off

		char  msg[64];
		const char	title[16] = "KICK OFF\n";
		Q_snprintf (msg, sizeof (msg), "%s to kick off. ", ((CSDKPlayer*)playerToTake)->GetPlayerName());
		SendVGUIStatusMessage (title, msg, true);

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"kickoff\"\n", 
					((CSDKPlayer*)playerToTake)->GetPlayerName(), ((CSDKPlayer*)playerToTake)->GetUserID(),
					((CSDKPlayer*)playerToTake)->GetNetworkIDString(), ((CSDKPlayer*)playerToTake)->GetTeam()->GetName() );
	} 
	else 
	{
		ballStatus = 0;                              //no one to take - just finish
	}

	EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");


	//allow for change after each goal
	//gUseOffsideRule = CVAR_GET_FLOAT("mp_offside");
	m_fUseOffsideRule = offside.GetFloat();

	//gBallFriction = CVAR_GET_FLOAT("mp_ballfriction"); //0-10
	//gBallFriction = min(gBallFriction, 10);
	//gBallFriction = max(0,gBallFriction);

	//pev->movetype = MOVETYPE_BOUNCE;
	//pev->effects &= ~EF_NODRAW;

	if (m_KeeperCarrying)
		m_KeeperCarrying->DoAnimationEvent( PLAYERANIMEVENT_CARRY_END );
		//m_KeeperCarrying->RemoveFlag (FL_CARRY);
	m_KeeperCarrying = NULL;

	ClrAssists();

	//gGoalScorer = NULL;
}


//==========================================================
// FindPlayerForAction - find player to take corner/etc
// 
//==========================================================
CBaseEntity *CBall::FindPlayerForAction (int team, int allowkeepers)
{
	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	float maxDist=99999.0f, dist;
	int saftey = 10000;

	while ((pObject = gEntList.FindEntityInSphere( pObject,GetAbsOrigin(), 8192 )) != NULL)	
	{
		if (pObject->IsPlayer() 
			/*&& !(pObject->pev->flags & FL_FAKECLIENT)*/
			&& ((CSDKPlayer*)pObject)->GetTeamNumber()==team
			//&& !(((CSDKPlayer*)pObject)->m_afPhysicsFlags & PFLAG_OBSERVER)
			&& ((CSDKPlayer*)pObject)->m_TeamPos!=-1) 
		{ 
			if (!allowkeepers && ((CSDKPlayer*)pObject)->m_TeamPos == 1)
				continue;

			dist = (GetAbsOrigin() - pObject->GetAbsOrigin()).Length2D();
			if (dist < maxDist ) 
			{
				maxDist = dist;
				pClosest = pObject;
			}
		}
		saftey--;
		if (saftey==0)
			break;
	}

	if (!saftey)
		Warning ("Cant find player for action\n");

	return pClosest;
}

//==========================================================
//
// 
//==========================================================
CSDKPlayer *CBall::FindKeeperForAction (int team)
{
	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	float maxDist=99999.0f, dist;

	while ((pObject = gEntList.FindEntityInSphere( pObject,GetAbsOrigin(), 16384 )) != NULL)	
	{
		if (pObject->IsPlayer() && ((CSDKPlayer*)pObject)->GetTeamNumber()==team && ((CSDKPlayer*)pObject)->m_TeamPos==1) 
		{ 
			dist = (GetAbsOrigin() - pObject->GetAbsOrigin()).Length2D();
			if (dist < maxDist ) 
			{
				maxDist = dist;
				pClosest = pObject;
			}
		}
	}

	return (CSDKPlayer*)pClosest;
}

//==========================================================
//
// 
//==========================================================
CBaseEntity *CBall::FindThrowInEnt (void)
{
	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	float maxDist=99999.0f, dist;
	int saftey = 10000;

	pObject = gEntList.FindEntityByClassname(NULL, "info_throw_in");

	while (pObject)	
	{
		dist = (m_FoulPos - pObject->GetAbsOrigin()).Length2D();
		if (dist < maxDist ) 
		{
			maxDist = dist;
      		pClosest = pObject;
		}
		pObject = gEntList.FindEntityByClassname(pObject, "info_throw_in");
		saftey--;
		if (saftey==0)
			break;
	}

	if (!saftey)
		Warning ("Cant find throw in entity\n");

	return pClosest;
}

///////////////////////////////////////////////////
// respawn all playing players
//
void CBall::RespawnPlayers (void)
{
	//for ( int i = 0; i < MAX_PLAYERS; i++ )
	//{
	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex (i);

		if ( !pPlayer )
			continue;

		if (pPlayer->GetTeamNumber() < TEAM_A)
			continue;

		g_pGameRules->GetPlayerSpawnSpot( pPlayer );
	}
}



///////////////////////////////////////////////////
// ShieldBall
//
void CBall::ShieldBall (CSDKPlayer *pPlayer)
{
   m_BallShield = true;
   m_BallShieldPlayer = pPlayer;		//only this player allowed inside Shielded area
   UpdateBallShield();					//1.0c
}

void CBall :: ShieldOff (void)
{
   m_BallShield = false;
   m_BallShieldPlayer = NULL;
}

#define SHIELD_NORMAL   360.0f                        //ten yards (10yards = 10 * 3ft * 12inches)
#define SHIELD_LARGE    (18.0f * 3.0f * 12.0f)        //eightteen yards
#define SHIELD_SMALL    252.0f                        //seven yards

///////////////////////////////////////////////////
// UpdateBallShield
//
void CBall::UpdateBallShield (void)
{
	CBaseEntity *pSpot=NULL;

	if (!m_BallShield)
		return;

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex (i);

		if (!pPlayer)
			continue;

		if (pPlayer->GetTeamNumber() < TEAM_A)						//spec and unassigned
			continue;
		if (pPlayer->m_TeamPos < 1 || pPlayer->m_TeamPos > 11)
			continue;
		//check for null,disconnected player
		if (strlen(pPlayer->GetPlayerName()) == 0)
			continue;
		//always let bot goalies work
		if ((pPlayer->GetFlags() & FL_FAKECLIENT) && pPlayer->m_TeamPos==1)
			continue;
		//check if this is the player allowed in to kick/throw etc
		if (pPlayer == m_BallShieldPlayer)
			continue;
		//if this player is playing as a goalie (not in penalty shootout
		//if (pPlayer->teamPos == 1 && !CVAR_GET_FLOAT("mp_keepers") && (ballStatus==BALL_PENALTY_PENDING && m_psMode==0))
		//exclude the keeper that is saving a penalty
		if (pPlayer->m_TeamPos == 1 && ballStatus==BALL_PENALTY_PENDING && (pPlayer->GetFlags() & FL_ATCONTROLS))
			continue;
		//if opposition keeper during penalty shootout
		//if (m_psMode!=0 && pPlayer->teamPos==1 && ballShieldPlayer && ballShieldPlayer->vampire != pPlayer->vampire)
		//	continue;


		//do special ball sheild at kick off to keep everyone in the correct half
		if (ballStatus == BALL_KICKOFF_PENDING) 
		{
			if (!PlayerInOwnHalf (pPlayer)) 
			{
				pSpot=NULL;
				if (pPlayer->GetTeamNumber()==TEAM_A) 
				{
					pSpot = gEntList.FindEntityByClassname( NULL, "info_team1_player1" );
				}
				else 
				{
					pSpot = gEntList.FindEntityByClassname( NULL, "info_team2_player1" );
				}
				if (pSpot) 
				{
					//Vector vec = (pSpot->pev->origin - pPlayer->pev->origin).Normalize()*32;
					//vec.z = 0;
					//pPlayer->pev->origin = pPlayer->pev->origin + vec;
					Vector vec = (pSpot->GetAbsOrigin() - pPlayer->GetAbsOrigin());
					VectorNormalize(vec);
					vec *= 32.0f;
					vec.z = 0;
					pPlayer->SetAbsOrigin(pPlayer->GetAbsOrigin() + vec);
					pPlayer->SetAbsVelocity(Vector(0.0f, 0.0f, 0.0f));
				}
			}
		}


		float   shieldDist = SHIELD_NORMAL;  //10yards = 10 * 3ft * 12inches

		if (ballStatus==BALL_PENALTY_PENDING)
			shieldDist = SHIELD_LARGE;       //15ft during penalty to stop people staying in the goal mouth

		if (m_KeeperCarrying)
			shieldDist = SHIELD_SMALL;

		//is this player near the ball
		if ((pPlayer->GetAbsOrigin() - GetAbsOrigin()).Length2D() < shieldDist) 
		{  

			//teleport the player a little way towards the centre of the pitch
			//(or towards their own goalie if its a kick off)
			pSpot=NULL;
			if (ballStatus == BALL_KICKOFF_PENDING) 
			{          
				if (pPlayer->GetTeamNumber()==TEAM_A) 
				{
					pSpot = gEntList.FindEntityByClassname( NULL, "info_team1_player1" );
				} 
				else 
				{
					pSpot = gEntList.FindEntityByClassname( NULL, "info_team2_player1" );
				}
			} 
			else 
			{
				pSpot = gEntList.FindEntityByClassname( pSpot, "info_ball_start" );
			}

			Vector vec = (pSpot->GetAbsOrigin() - pPlayer->GetAbsOrigin());
			VectorNormalize(vec);
			vec*=16.0f;
			vec.z = 0; //clr the vertical component
			//keep teleporting on this vector until out side 10 yards
			while ((pPlayer->GetAbsOrigin() - GetAbsOrigin()).Length2D() < shieldDist) 
			{
				pPlayer->SetAbsOrigin(pPlayer->GetAbsOrigin() + vec);
				pPlayer->SetAbsVelocity(Vector(0.0f, 0.0f, 0.0f));
			}

			//pPlayer->pev->flags &= ~FL_ONTRAIN;       //let them move if we bounced them

		} //end shield check


		//check players if outside doulbe the shield dist
		if ( (pPlayer->GetAbsOrigin() - GetAbsOrigin()).Length2D() < 2*shieldDist) 
		{  

			pSpot=NULL;
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_ball_start" );
	                    
			Vector vec = (pSpot->GetAbsOrigin() - pPlayer->GetAbsOrigin());
			VectorNormalize(vec);
			vec *= 16.0f;
			vec.z = 0; //clr the vertical component

			int clear = false;
			while (!clear)
			{
				//check if this position has another player already there?
				CBaseEntity *ent = NULL;
				clear = true;
				while ( (ent = gEntList.FindEntityInSphere( ent, pPlayer->GetAbsOrigin(), 128 )) != NULL )  //was 96 in v1.0, 128 in v2.0
				{
					//teleport the player some more
					if ( ent->IsPlayer() && ent != pPlayer) 
					{
						while ((pPlayer->GetAbsOrigin() - ent->GetAbsOrigin()).Length2D() < 48) 
						{
							pPlayer->SetAbsOrigin(pPlayer->GetAbsOrigin() + vec);
							pPlayer->SetAbsVelocity(Vector(0.0f, 0.0f, 0.0f));
							clear = false;
						}
					}
				}
			}
		}    
	}



/*
		//do special ball sheild at kick off to keep everyone in the correct half
		if (ballStatus == BALL_KICKOFF_PENDING) 
		{
			if (!PlayerInOwnHalf (pPlayer)) 
			{
				pSpot=NULL;
				if (pPlayer->GetTeamNumber()==TEAM_A) 
				{
					pSpot = gEntList.FindEntityByClassname( NULL, "info_team1_player1" );
				} 
				else 
				{
					pSpot = gEntList.FindEntityByClassname( NULL, "info_team2_player1" );
				}
				if (pSpot) 
				{
					Vector vec = (pSpot->GetAbsOrigin() - pPlayer->GetAbsOrigin());
					VectorNormalize(vec);
					vec *= 32.0f;
					vec.z = 0;
					pPlayer->SetAbsOrigin(pPlayer->GetAbsOrigin() + vec);
					pPlayer->SetAbsVelocity(Vector(0.0f, 0.0f, 0.0f));
				}
			}
		}



		float   shieldDist = SHIELD_NORMAL;  //10yards = 10 * 3ft * 12inches

		if (ballStatus==BALL_PENALTY_PENDING)
			shieldDist = SHIELD_LARGE;       //15ft during penalty to stop people staying in the goal mouth

		if (m_KeeperCarrying)
			shieldDist = SHIELD_SMALL;

		//is this player near the ball
		if ( (pPlayer->GetAbsOrigin() - GetAbsOrigin()).Length2D() < shieldDist) 
		{
			//teleport the player a little way towards the centre of the pitch
			//(or towards their own goalie if its a kick off)
			//IOS - or vector from ball to player so they go straight back?
			pSpot=NULL;
			Vector vec;
			vec = pPlayer->GetAbsOrigin() - GetAbsOrigin();		//IOSS
			VectorNormalize(vec);
			vec *= 16.0f;
			vec.z = 0; //clr the vertical component
			//keep teleporting on this vector until out side 10 yards
			while ((pPlayer->GetAbsOrigin() - GetAbsOrigin()).Length2D() < shieldDist) 
			{
				pPlayer->SetAbsOrigin(pPlayer->GetAbsOrigin() + vec);
				pPlayer->SetAbsVelocity(Vector(0.0f, 0.0f, 0.0f));
			}

			//TODOpPlayer->pev->flags &= ~FL_ONTRAIN;       //let them move if we bounced them

		} //end shield check


		//check players if outside double the shield dist
		if ( (pPlayer->GetAbsOrigin() - GetAbsOrigin()).Length2D() < 2*shieldDist) 
		{
			pSpot=NULL;
			//pSpot = gEntList.FindEntityByClassname( pSpot, "info_ball_start" );
	            
			//Vector vec = (pSpot->GetAbsOrigin() - pPlayer->GetAbsOrigin());
			Vector vec = pPlayer->GetAbsOrigin() - GetAbsOrigin();		//IOSS
			VectorNormalize(vec);
			vec *= 16.0f;
			vec.z = 0; //clr the vertical component

			int clear = false;
			while (!clear)
			{
				//check if this position has another player already there?
				CBaseEntity *ent = NULL;
				clear = true;
				while ( (ent = gEntList.FindEntityInSphere( ent, pPlayer->GetAbsOrigin(), 128 )) != NULL )  //was 96 in v1.0, 128 in v2.0
				{
					//teleport the player some more
					if ( ent->IsPlayer() && ent != pPlayer) //TODO&& !(((CSDKPlayer*)ent)->m_afPhysicsFlags & PFLAG_OBSERVER)) 
					{
						while ((pPlayer->GetAbsOrigin() - ent->GetAbsOrigin()).Length2D() < 48) 
						{
							pPlayer->SetAbsOrigin(pPlayer->GetAbsOrigin() + vec);
							pPlayer->SetAbsVelocity(Vector(0.0f, 0.0f, 0.0f));
							clear = false;
						}
					}
				}
			}
		}    
	}
	*/
}







//////////////////////////////////////////////////////////////////////////////////////////////////



void CBall::AddAssist (CSDKPlayer *pNewTouch)
{
   if (ballStatus > 0 && ballStatus < BALL_GOALKICK_PENDING)      //3.0a allow assists from corners/freekicks/goalkicks
      return;

   for (int i = MAX_ASSIST-1; i > 0; i--)
      m_Assist[i] = m_Assist[i-1];

   m_Assist[0] = pNewTouch;
}

 
void CBall::AssistDisconnect (CSDKPlayer *discon)
{
   for (int i=0; i<MAX_ASSIST; i++)
   {
      if (discon == m_Assist[i])
         m_Assist[i] = NULL;
   }
}

void CBall::ClrAssists (void)
{
   for (int i=0; i<MAX_ASSIST; i++)
      m_Assist[i] = NULL;
}


void CBall::CheckForAssist (CSDKPlayer *scorer)
{
   CSDKPlayer *assist1 = NULL, *assist2 = NULL;
   int foundScorer = false;
   char msg[64];

   if (!scorer)
      return;

   for (int i=0; i<MAX_ASSIST; i++)
   {
      if (!m_Assist[i])
         continue;

      //the scorer should be top of the list but just in case the keeper kicked it after
      //the scoring shot keep looking until we find the entry that really scored.
      if (m_Assist[i] == scorer)
      {
         foundScorer = true;
         continue;
      }
      
      //scorer not found yet - skip and keep looking.
      if (!foundScorer)
         continue;

      //if we find an opposition player in the list after the scorer but before 
      //an 'assist' player then quit as there was no assist.
	  if (m_Assist[i]->GetTeamNumber() != scorer->GetTeamNumber())
         break;

      //found first assist, store it and continue.
      if (!assist1) {
         assist1 = m_Assist[i];
         continue;
      }
      
      //if we find another assist that isn't the same as assist1 store it and done.
      if (assist1 && !assist2 && m_Assist[i] != assist1)
      {
         assist2 = m_Assist[i];   
         break;
      }
   }

   if (assist1)
   {
		//update assist score
		assist1->m_Assists++;
		Q_snprintf (msg, sizeof (msg), "Assist by %s ", assist1->GetPlayerName());
		SendVGUIStatusMessage ("", msg, true);
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"assist\"\n", 
					assist1->GetPlayerName(), assist1->GetUserID(),
					assist1->GetNetworkIDString(), assist1->GetTeam()->GetName() );

   }

   if (assist2)
   {
		//update assist score
		assist2->m_Assists++;
		Q_snprintf (msg, sizeof (msg), "and %s ", assist2->GetPlayerName());
		SendVGUIStatusMessage ("", msg, true);
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"assist\"\n", 
					assist2->GetPlayerName(), assist2->GetUserID(),
					assist2->GetNetworkIDString(), assist2->GetTeam()->GetName() );
   }

   ClrAssists();
}





/////////////////////////////////////////////////////////////////////
//
// check for catch
//
int CBall::CheckKeeperCatch (CSDKPlayer *pKeeper)
{
	int otherKeeper = FALSE;

	if (m_KeeperCarrying)
		return false;

	if (!pKeeper)
		return false;

	//only catch during	normal play
	if (ballStatus != 0)
		return false;

	if(pKeeper->m_TeamPos != 1)
		return false;

	//only catch if we are in our own penalty area (m_BallInPenaltyBox is flipped so opposite team gets penalty kick)
	if (m_BallInPenaltyBox != pKeeper->GetTeamNumber())
		return false;


	//special case for last touch by opposite keeper	- 4.0a
	if (m_LastTouchBeforeMe && m_LastTouchBeforeMe->m_TeamPos==1 && m_LastTouchBeforeMe!=pKeeper)
		otherKeeper = true;

	//IOSS
	if (m_LastTouch && m_LastTouch->m_TeamPos==1 && m_LastTouch!=pKeeper)
 		otherKeeper = true;

	//check for passback
	if (m_LastNonKeeper && !otherKeeper)	
	{
		if (m_LastNonKeeper->GetTeam() == pKeeper->GetTeam())	
		{
			//ALERT (at_console, "passback\n");
			return	false;
		}
	}

	//trying to catch too soon after a kick from hands
	if (m_NextKeeperCatch > gpGlobals->curtime)
		return false;

	Vector pos, vel;
	VPhysicsGetObject()->GetPosition(&pos, NULL);
	VPhysicsGetObject()->GetVelocity(&vel, NULL);

	//if	ball is	near hands then	catch
	float distToBall = (pos - pKeeper->GetLocalOrigin()).Length2D();
	bool bCaught = false;

	int playerHeight = SDKGameRules()->GetViewVectors()->m_vHullMax.z;
	Vector playerPos = pKeeper->GetLocalOrigin();
	float handPos = playerPos.z + playerHeight / 2 ;
	float distToHead = pos.z - (playerPos.z + playerHeight);
	if (distToHead < 0)
		distToHead = FLT_MAX;

	float distToFoot = abs(pos.z - playerPos.z);

	if (distToBall < 50 && ballStatus==BALL_NORMAL)
	{
		if (vel.Length2D() < 200)
		{
			bCaught = true;
		}
		else
		{
			Vector newVel;
			if (distToHead < 50)
			{		
				newVel = vel + Vector(0, 0, 200);
			}
			else if (pos.z < handPos + 25 && pos.z > handPos - 25)
			{
				newVel = -vel;
			}
			else if (distToFoot < 10)
			{
				newVel = -vel;
			}
			VPhysicsGetObject()->SetVelocity(&newVel, NULL);
			return true;
		}
	}

	if (!bCaught)
		return false;

	//ok we caught the ball:
	m_KeeperCarrying = pKeeper;
	//m_KeeperCarrying->AddFlag (FL_CARRY);
	m_KeeperCarrying->DoAnimationEvent(PLAYERANIMEVENT_CARRY);	//Tell keeper to carry ball!
	//m_KeeperCarrying->m_nBody = m_nSkin + 2;	//switch to model carrying ball (which is linked to balls m_nSkin)
	m_KeeperCarrying->m_nBody = MODEL_KEEPER_AND_BALL;
	m_KeeperCarrying->m_nSkin = m_KeeperCarrying->m_nBaseSkin + m_nSkin;	//add on an offset by the balls skin number to the keeper.
	m_KeeperCarryTime =	gpGlobals->curtime + 10.0f;
	ballStatusTime = gpGlobals->curtime + BALL_STATUS_TIME;

	SetEffects(EF_NODRAW);                                  //stop drawing real ball

	//pev->origin.x = keeperCarrying->pev->origin.x;
	//pev->origin.y = keeperCarrying->pev->origin.y;
	//pev->origin.z = keeperCarrying->pev->origin.z;
	//keeperCarryTime = gpGlobals->time + 10.0f;
	//romd test ShieldBall (m_KeeperCarrying);
	//m_NextShoot = gpGlobals->curtime + KICK_DELAY;			//cant kick right away

	//EMIT_SOUND(ENT(pev), CHAN_WEAPON, "ball/whistle.wav", 1, ATTN_NONE);

	if (pKeeper != m_LastTouch)
	{
		pKeeper->m_KeeperSaves++;
		if (gEntList.FindEntityByClassname( NULL, "info_stadium" ))
			pKeeper->EmitAmbientSound(pKeeper->entindex(), pKeeper->GetAbsOrigin(), "Player.Save");
	}

	return true;
}


/////////////////////////////////////////////////////////////////////
//
// Keeper drop the ball, if carrying
//
void CBall::DropBall(void)
{
	if (m_KeeperCarrying)
	{
		RemoveEffects (EF_NODRAW);                                 //draw ball again
		m_KeeperCarrying->m_nBody = MODEL_KEEPER;	               //restore keeper model
		//m_KeeperCarrying->RemoveFlag (FL_CARRY);
		m_KeeperCarrying->DoAnimationEvent( PLAYERANIMEVENT_CARRY_END );
		m_KeeperCarrying = NULL;
		ShieldOff();
	}
}

/////////////////////////////////////////////////////////////////////
//
// Handle human carrying
//
int CBall::KeepersBallHuman (void)
{
   //see if it's time to kick or if real ball leaves penalty area just kick it quickly
   if (m_KeeperCarryTime < gpGlobals->curtime || m_BallInPenaltyBox==-1) 
   {      
	  DropBall();
      return false;
   }


	//hold real ball near player at correct height
	Vector vForward;
	AngleVectors (m_KeeperCarrying->EyeAngles(), &vForward);	//UTIL_MakeVectors ( keeperCarrying->pev->v_angle );
	vForward.z = 0.0f;                           //keep roughly horizontal
	VectorNormalize(vForward);
	

   //pev->origin = keeperCarrying->pev->origin + (48*gpGlobals->v_forward);
	Vector ballOrigin = GetAbsOrigin();
	//ballOrigin = m_KeeperCarrying->GetAbsOrigin() + (32 * vForward);
	ballOrigin = m_KeeperCarrying->GetAbsOrigin() + (48 * vForward);		//1.0d so new collision doesnt keep hitting
	ballOrigin.z = m_KeeperCarrying->GetAbsOrigin().z + 30.0f;
	//SetAbsOrigin(ballOrigin);
	VPhysicsGetObject()->SetPosition(ballOrigin, GetAbsAngles(), true);
	
	SetAbsVelocity( Vector (0.0f, 0.0f, 0.0f) );
	//pev->velocity.x = 0;
	//pev->velocity.z = 0;

	//do things from normal loop that would get missed.
	UpdateBallShield();
	m_BallInPenaltyBox = -1;

   return TRUE;
}



/////////////////////////////////////////////////////////////////////
//
// If the keeper is carrying the ball, handle it now
//
int CBall::KeepersBall (void)
{
   int   result;

   if (!m_KeeperCarrying)
      return false;


   //TODO if (keeperCarrying->pev->flags & FL_FAKECLIENT)
   //{

   //   //bots check for passback
   //   if (LastNonKeeper) {
   //      if (LastNonKeeper->vampire == keeperCarrying->vampire) {
   //         keeperCarrying = NULL;
   //         //ALERT (at_console, "passback\n");
   //         return false;
   //      }
   //   }
   //}

   //process keeper catch - human or bot
   //if (m_KeeperCarrying->GetFlags() & FL_FAKECLIENT)
   //   result = KeepersBallBot();
   //else
      result = KeepersBallHuman();

   return (result);
}


int CBall::CheckKeeperKick (CSDKPlayer *keeper)
{
	if (!keeper)
		return false;

	//if (keeper->GetFlags() & FL_FAKECLIENT)
	//	return false;

	if(keeper->m_TeamPos != 1)
		return false;

	//if we are already carrying then kick / cancel catch
	if (m_KeeperCarrying==keeper && (m_KeeperCarryTime < gpGlobals->curtime + 9.0f)) 
	{
		ballStatus=0;
		RemoveEffects (EF_NODRAW);                                 //draw ball again
		m_KeeperCarrying->m_nBody = MODEL_KEEPER;	                 //restore keeper model
		//m_KeeperCarrying->RemoveFlag (FL_CARRY);
		m_KeeperCarrying->DoAnimationEvent( PLAYERANIMEVENT_CARRY_END );
		m_KeeperCarrying = NULL;
		ShieldOff();
		//pev->origin.z = keeperCarrying->pev->origin.z;
		m_NextKeeperCatch = gpGlobals->curtime + 1.0f;                 //stop pickup right after a kick
		return true;
	}

	return false;
}






//float gUseOffsideRule = 1;		//TODO needs mp_offside cvar to set/unset (for indoor etc)

//Theory - when ever a ball is kicked see if someone is offside.
//Whenever someone receives a ball see if offside was flagged
//when the ball was kicked, if so, they must be offside.
//Requires map to be laid out with pitch Up/Down in Hammer.
//This doesnt work if more than one players are offside and
//you pass to the one that *isnt* furthest offside (since he's the
//only one caught by this function)
int CBall :: CheckOffSide (CSDKPlayer *kicker)
{
	CSDKPlayer *OtherUp = NULL, *OtherDown = NULL;
	CSDKPlayer *pPlayer = NULL;
	float       OtherDistUp = -99999.0f, OtherDistDown = 99999.0f;
	int         checkteam, i;
   
	if (!m_fUseOffsideRule)
		return false;

	//check for null
	if (!kicker)
		return false;

	ClearOffsideFlags();

	checkteam = kicker->GetTeamNumber();

	//find players on other team at extremes
	for (i = 1; i <= gpGlobals->maxClients; i++) 
	{
		pPlayer = (CSDKPlayer*)UTIL_PlayerByIndex (i);

		if (!pPlayer)
			continue;
		//if (pPlayer->vampire == VS_NOSTATUS)
		//	continue;
		//if (pPlayer->model == MODEL_NONE)
		//	continue;
		if (pPlayer->m_TeamPos < 1)
			continue;
		if (strlen(pPlayer->GetPlayerName()) == 0)
			continue;
		//if (pPlayer->m_afPhysicsFlags & PFLAG_OBSERVER)
		//	continue;
		if (pPlayer->m_TeamPos == 1)     //debug - comment out so you can be offside against keepers
			continue;
		if (pPlayer->GetTeamNumber() == checkteam)
			continue;

		//find player on opposite team furthest to the Up
		if (pPlayer->GetAbsOrigin().y > OtherDistUp)
		{
			OtherUp = pPlayer;
			OtherDistUp = pPlayer->GetAbsOrigin().y;
		}
		//find player on opposite team furthest to the Down
		if (pPlayer->GetAbsOrigin().y < OtherDistDown)
		{
			OtherDown = pPlayer;
			OtherDistDown = pPlayer->GetAbsOrigin().y;
		}
	}

	if (!OtherDown || !OtherUp)   //no opposition players to be offside against
		return false;

	//find all who are offside
	for (i = 1; i <= gpGlobals->maxClients; i++) 
	{
		pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex (i);

		if (!pPlayer)
			continue;
		//if (pPlayer->vampire == VS_NOSTATUS)
		//	continue;
		//if (pPlayer->model == MODEL_NONE)
		//	continue;
		if (pPlayer->m_TeamPos < 1)
			continue;
		if (strlen(pPlayer->GetPlayerName()) == 0)
			continue;
		//if (pPlayer->m_afPhysicsFlags & PFLAG_OBSERVER)
		//	continue;
		if (pPlayer->GetTeamNumber() != checkteam)
			continue;
		if (pPlayer == kicker)
			continue;
		if (PlayerInOwnHalf(pPlayer))
			continue;
		if (!pPlayer->IsOnPitch())    //beta 3.0a - maybe?
			continue;

		if (checkteam==TEAM_A)
		{
			//no opposition is on front of this player and he is also on front of kicker
			//if ((pPlayer->pev->origin.y < OtherDistDown) && (pPlayer->pev->origin.y < kicker->pev->origin.y))
			//no opposition is on front of this player and he is also on front of ball
			if ((pPlayer->GetAbsOrigin().y < OtherDistDown) && (pPlayer->GetAbsOrigin().y < GetAbsOrigin().y))
			{
				RecordOffside (pPlayer, kicker, OtherDown);
			}
		}
		else
		{
			//if ((pPlayer->pev->origin.y > OtherDistUp) && (pPlayer->pev->origin.y > kicker->pev->origin.y))
			if ((pPlayer->GetAbsOrigin().y > OtherDistUp) && (pPlayer->GetAbsOrigin().y > GetAbsOrigin().y))
			{
				RecordOffside (pPlayer, kicker, OtherUp);
			}
		}
	}

   return false;
}


////////////////////////////////////////////////////////////
//
int CBall::CheckOffsideReceive (CSDKPlayer* pOther)
{

	for (int i=0; i<m_NumOffsidePlayers; i++)
	{
		if (pOther == m_OffsidePlayer[i])
			CheckOffsideReceiver (pOther, i);
	}

	ClearOffsideFlags();
	return false;
}



//when a player touches/kicks or passes the ball
//check to see if it was 'offside' when we did.
//if not we must cancel the offside state since the
//other side must have touched it first
int CBall::CheckOffsideReceiver (CSDKPlayer* pOther, int i)
{
	//check if this player was offside when teammate kicked ball
	if (m_fUseOffsideRule && (m_OffsideTimeout[i] > gpGlobals->curtime)
		&& ballStatus==0 && (pOther!=m_OffsideKicker[i]))
	{
		ballStatus = BALL_FOUL;

		//m_team = !pOther->vampire; //give foul to opposition
	   if (pOther->GetTeamNumber() == TEAM_A)
			m_team = TEAM_B;
		else
			m_team = TEAM_A;
		m_OffsidePlayer[i] = NULL;
		m_OffsideKicker[i] = NULL;

		//need to keep current pos so can be placed for foul to be taken
		m_FoulPos.x = m_OffsideFoulPos[i].x;   //beta 3.0a. place foul where receive was when they
		m_FoulPos.y = m_OffsideFoulPos[i].y;   //were caught offside and not where they are now.
		m_FoulPos.z = m_OffsideFoulPos[i].z;
		m_Foulee = (CSDKPlayer*)FindPlayerForAction (m_team,0);   //find who was nearest as soon as it went out (store in foulee)

		ballStatusTime = gpGlobals->curtime + 3.0f;

		SendMainStatus(BALL_MAINSTATUS_OFFSIDE);

		char  msg[64];
		const char	title[16] = "OFFSIDE\n";
		Q_snprintf (msg, sizeof (msg), "%s was offside. ", ((CSDKPlayer*)pOther)->GetPlayerName());
		SendVGUIStatusMessage (title, msg, true);
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"offside\"\n", 
					((CSDKPlayer*)pOther)->GetPlayerName(), ((CSDKPlayer*)pOther)->GetUserID(),
					((CSDKPlayer*)pOther)->GetNetworkIDString(), ((CSDKPlayer*)pOther)->GetTeam()->GetName() );
		return true;
	}

	return false;
}


//////////////////////////////////////////////////////////
//
void CBall::ClearOffsideFlags (void)
{
	for (int i=0; i<MAX_OFFS; i++)
	{
		m_OffsidePlayer[i] = NULL;
		m_OffsideKicker[i] = NULL;
		m_OffsideTimeout[i] = 0.0f;
	}
	m_NumOffsidePlayers = 0;
	//ClearGlowShells();
}


//////////////////////////////////////////////////////////
//
int CBall::NextOffsideSlot (void)
{
	for (int i=0; i<MAX_OFFS; i++)
	{
		if (m_OffsidePlayer[i] == NULL)
			return (i);
	}
	return (-1);
}


//////////////////////////////////////////////////////////
//
void CBall::RecordOffside (CSDKPlayer *offside, CSDKPlayer *kicker, CSDKPlayer *against)
{
	int   i = NextOffsideSlot();
	m_OffsidePlayer[i] = offside;
	m_OffsideKicker[i] = kicker;
	m_OffsideTimeout[i] = gpGlobals->curtime + 10.0f;
	m_OffsideFoulPos[i].x = offside->GetAbsOrigin().x;
	m_OffsideFoulPos[i].y = offside->GetAbsOrigin().y;
	m_OffsideFoulPos[i].z = GetAbsOrigin().z;
   
	m_NumOffsidePlayers = i+1;

	//int showoff = CVAR_GET_FLOAT("mp_showoffside");
	//if (showoff > 0)
	//{
	//	//glowshell player who is offside in red
	//	offside->pev->rendercolor.x = 250.0f; offside->pev->rendercolor.y = 0.0f; offside->pev->rendercolor.z = 0.0f;
	//	offside->pev->renderamt = 30;
	//	offside->pev->renderfx = kRenderFxGlowShell; 
	//}
	//if (showoff > 1)
	//{
	//	//kicker in blue
	//	kicker->pev->rendercolor.x = 0.0f; kicker->pev->rendercolor.y = 0.0f; kicker->pev->rendercolor.z = 250.0f;
	//	kicker->pev->renderamt = 30;
	//	kicker->pev->renderfx = kRenderFxGlowShell; 
	//}
	//if (showoff > 2)
	//{
	//	//opposition who they are offside against
	//	against->pev->rendercolor.x = 0.0f; against->pev->rendercolor.y = 250.0f; against->pev->rendercolor.z = 0.0f;
	//	against->pev->renderamt = 30;
	//	against->pev->renderfx = kRenderFxGlowShell; 
	//}
}


///////////////////////////////////////////////////
// stop human keepers moving during penalties etc
// (need to move them to spawn)
//
void CBall::FreezeKeepersForPenalty (int team)
{
   CBaseEntity *pSpot=NULL;

   //if (CVAR_GET_FLOAT("mp_keepers"))
   //   return;

  	//if (!CVAR_GET_FLOAT("mp_freezekeeper"))
   //   return;

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex (i);

		if (!pPlayer)
			continue;

		//found a keeper - only do the correct one
		if (pPlayer->m_TeamPos == 1 && pPlayer->GetTeamNumber()==team)
		{
			if (team==TEAM_A) 
			{
				//find goalie spawns
				pSpot = gEntList.FindEntityByClassname( NULL, "info_team1_player1" );
			} 
			else 
			{
				pSpot = gEntList.FindEntityByClassname( NULL, "info_team2_player1" );
			}

			if (pSpot) 
			{
				//freeze them (because this function is only called once)
				pPlayer->AddFlag(FL_ATCONTROLS);
				pPlayer->m_HoldAnimTime = -1;			//1.0c atcontrols get cleared every frame in player anim
				//move them to correct point
				Vector pos = pPlayer->GetAbsOrigin();
				pos.x = pSpot->GetAbsOrigin().x;
				pos.y = pSpot->GetAbsOrigin().y;
				pPlayer->SetAbsOrigin(pos);
			}
		}
	}
}


//////////////////////////////////////////////////////////
//
int CBall::PlayerInOwnHalf (CSDKPlayer *pOther)
{
	CBaseEntity *pSpotOther=NULL, *pSpotMine=NULL;
	float       distOther=0.0f, distMine=0.0f;

	if (pOther->GetTeamNumber()==TEAM_A)
	{
		//find goalie spawns
		pSpotOther = gEntList.FindEntityByClassname( NULL, "info_team2_player1" );
		pSpotMine  = gEntList.FindEntityByClassname( NULL, "info_team1_player1" );
	}
	else 
	{
		pSpotOther = gEntList.FindEntityByClassname( NULL, "info_team1_player1" );
		pSpotMine  = gEntList.FindEntityByClassname( NULL, "info_team2_player1" );
	}
	distOther = fabs (pOther->GetAbsOrigin().y - pSpotOther->GetAbsOrigin().y);
	distMine  = fabs (pOther->GetAbsOrigin().y - pSpotMine->GetAbsOrigin().y);
	//if I'm closer to my goal than I am to the opposition then I must be in my own half
	if (distMine < distOther) 
	{
		return true;
	}
	else 
	{
		return false;
	}
}

/////////////////////////////////////////////////////////
//give double touch foul if appropriate
void CBall::DoubleTouchFoul (CSDKPlayer *player)
{
	int svDoubleTouch = 0;

	//must be some players before we use this rule. remove this check in debug so we get doubletouches all the time
#ifdef NDEBUG
	if (GetGlobalTeam( TEAM_A )->GetNumPlayers() < 3 || GetGlobalTeam( TEAM_B )->GetNumPlayers() < 3)
	{
		m_PlayerWhoTook = NULL;
		m_DoubleTouchCount = 0;
		return;
	}
#endif

   //dont bother with doubletouch fouls during penalty shootout (4.0)
   //if (m_psMode != PS_OFF && m_psMode != PS_DONE)
   //{
   //   playerWhoTook = NULL;
   //   m_DoubleTouchCount = 0;
   //   return;
   //}
      
   //check cvar
  	//if (!(svDoubleTouch = (int)CVAR_GET_FLOAT("mp_doubletouch")))
   //   return;
	svDoubleTouch = 1;

	m_DoubleTouchCount++;

	if (m_DoubleTouchCount < svDoubleTouch)
		return;

	m_Foulee = NULL;
	EmitAmbientSound(entindex(), GetAbsOrigin(), "Ball.whistle");

   //get this message in before the 'card' messages
	char  msg[64];
	const char	title[16] = "FOUL\n";
	Q_snprintf (msg, sizeof (msg), "%s double touch fouled. ", player->GetPlayerName());
	SendVGUIStatusMessage (title, msg, true);

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"doubletouch\"\n", 
				player->GetPlayerName(), player->GetUserID(),
				player->GetNetworkIDString(), player->GetTeam()->GetName() );


   player->m_Fouls++;
   
   //common to foul/yellow/red
   ballStatus = BALL_FOUL;
   if (player->GetTeamNumber() == TEAM_A)
		m_team = TEAM_B;
   else
		m_team = TEAM_A;

   //need to keep current pos so can be placed for foul to be taken
   m_FoulPos = player->GetAbsOrigin();
   m_FoulPos.z += 8.0f;

   //m_FoulPos.x = player->pev->origin.x;
   //m_FoulPos.y = player->pev->origin.y;
   //m_FoulPos.z = pev->origin.z;
	ballStatusTime = gpGlobals->curtime + 3.0f;
	EmitAmbientSound(entindex(), GetAbsOrigin(), "Player.Card");

	//clr after foul.
	m_PlayerWhoTook = NULL;
	m_DoubleTouchCount = 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////



void CBall::ResetPossession (void)
{
    m_poss[0] = 0.0f;
    m_poss[1] = 0.0f;
    m_possStart = 0.0f;
    m_possTeam = -1;
	m_pPossPlayer = NULL;
}


void CBall::UpdatePossession (CSDKPlayer *pPlayer)
{
	if (!pPlayer)
		return;

	if (pPlayer->GetTeamNumber() < TEAM_A)
		return;

	//give it to the prev player (i.e.when its received thats our possession amount)
	//if the next kick is by the same team then add this time onto the their total
	if (m_pPossPlayer && m_possTeam >= TEAM_A)
	{
		m_poss[m_possTeam-TEAM_A] += (gpGlobals->curtime - m_possStart); //add on some valid possession
		m_pPossPlayer->m_fPossessionTime += (gpGlobals->curtime - m_possStart);
	}
	m_possStart = gpGlobals->curtime;
	m_possTeam = pPlayer->GetTeamNumber();
	m_pPossPlayer = pPlayer;


	//now go through every player and work out their individual percentage possession of the total
	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex (i);

		if (!pPlayer)
			continue;
		if (pPlayer->GetTeamNumber() < TEAM_A)
			continue;

		if (pPlayer->GetTeamNumber()==TEAM_A)
		{
			if (m_poss[0])
				pPlayer->m_Possession = (int)( ((pPlayer->m_fPossessionTime / (m_poss[0] + m_poss[1])) * 100) + 0.5f );
		}
		else
		{
			if (m_poss[1])
				pPlayer->m_Possession = (int)( ((pPlayer->m_fPossessionTime / (m_poss[0] + m_poss[1])) * 100) + 0.5f );
		}
	}


}


int CBall::GetTeamPossession (int team)
{
	team -= TEAM_A;	//convert to 0-1.

   if ((m_poss[0] + m_poss[1])==0)   //div by zero
      return (0);
      
   return (int)( ((m_poss[team] / (m_poss[0] + m_poss[1])) * 100) + 0.5f );
}



void CBall::ApplyKickDelayToAll(void)
{

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex (i);

		if (!pPlayer)
			continue;

		if (pPlayer->GetTeamNumber() < TEAM_A)						//spec and unassigned
			continue;
		if (pPlayer->m_TeamPos < 1 || pPlayer->m_TeamPos > 11)
			continue;
		//check for null,disconnected player
		if (strlen(pPlayer->GetPlayerName()) == 0)
			continue;
		//always let goalies work
		if (pPlayer->GetFlags() & FL_FAKECLIENT && pPlayer->m_TeamPos==1)
			continue;
		//check if this is the player allowed in to kick/throw etc
		if (pPlayer == m_BallShieldPlayer)
			continue;

		pPlayer->m_NextShoot = gpGlobals->curtime + KICK_DELAY;
	}
}


void CBall::RemoveFlagFromAll(int flag)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex (i);

		if (!pPlayer)
			continue;

		if (pPlayer->GetTeamNumber() < TEAM_A)						//spec and unassigned
			continue;
		if (pPlayer->m_TeamPos < 1 || pPlayer->m_TeamPos > 11)
			continue;
		//check for null,disconnected player
		if (strlen(pPlayer->GetPlayerName()) == 0)
			continue;

		pPlayer->RemoveFlag(flag);
	}
}



