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
#include "movevars_shared.h"
#include "ios_replaymanager.h"


ConVar sv_ball_mass( "sv_ball_mass", "1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_damping( "sv_ball_damping", "0.4", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_rotdamping( "sv_ball_rotdamping", "0.3", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_rotinertialimit( "sv_ball_rotinertialimit", "1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_dragcoeff( "sv_ball_dragcoeff", "1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_inertia( "sv_ball_inertia", "1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_drag_enabled("sv_ball_drag_enabled", "1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_spin( "sv_ball_spin", "300", FCVAR_NOTIFY );
ConVar sv_ball_defaultspin( "sv_ball_defaultspin", "10000", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_topspin_coeff( "sv_ball_topspin_coeff", "1.0", FCVAR_NOTIFY );
ConVar sv_ball_backspin_coeff( "sv_ball_backspin_coeff", "1.0", FCVAR_NOTIFY );
ConVar sv_ball_jump_topspin_enabled("sv_ball_jump_topspin_enabled", "1", FCVAR_NOTIFY );
ConVar sv_ball_curve("sv_ball_curve", "150", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_deflectionradius( "sv_ball_deflectionradius", "40", FCVAR_NOTIFY );

ConVar sv_ball_standing_reach( "sv_ball_standing_reach", "60", FCVAR_NOTIFY );

ConVar sv_ball_slidesidereach_ball( "sv_ball_slidesidereach_ball", "60", FCVAR_NOTIFY );
ConVar sv_ball_slideforwardreach_ball( "sv_ball_slideforwardreach_ball", "90", FCVAR_NOTIFY );
ConVar sv_ball_slidesidereach_foul( "sv_ball_slidesidereach_foul", "40", FCVAR_NOTIFY );
ConVar sv_ball_slideforwardreach_foul( "sv_ball_slideforwardreach_foul", "60", FCVAR_NOTIFY );
ConVar sv_ball_slidesidespeedcoeff("sv_ball_slidesidespeedcoeff", "0.66", FCVAR_NOTIFY); 
ConVar sv_ball_slidezstart("sv_ball_slidezstart", "-50", FCVAR_NOTIFY); 
ConVar sv_ball_slidezend("sv_ball_slidezend", "40", FCVAR_NOTIFY); 

ConVar sv_ball_keeper_standing_reach( "sv_ball_keeper_standing_reach", "60", FCVAR_NOTIFY );

ConVar sv_ball_keeper_forwarddive_shortsidereach( "sv_ball_keeper_forwarddive_shortsidereach", "50", FCVAR_NOTIFY );
ConVar sv_ball_keeper_forwarddive_longsidereach( "sv_ball_keeper_forwarddive_longsidereach", "130", FCVAR_NOTIFY );
ConVar sv_ball_keeper_forwarddive_longsidereach_opposite( "sv_ball_keeper_forwarddive_longsidereach_opposite", "40", FCVAR_NOTIFY );
ConVar sv_ball_keeper_forwarddive_zstart( "sv_ball_keeper_forwarddive_zstart", "-50", FCVAR_NOTIFY );
ConVar sv_ball_keeper_forwarddive_zend( "sv_ball_keeper_forwarddive_zend", "40", FCVAR_NOTIFY );

ConVar sv_ball_keeper_sidedive_shortsidereach( "sv_ball_keeper_sidedive_shortsidereach", "50", FCVAR_NOTIFY );
ConVar sv_ball_keeper_sidedive_longsidereach( "sv_ball_keeper_sidedive_longsidereach", "70", FCVAR_NOTIFY );
ConVar sv_ball_keeper_sidedive_longsidereach_opposite( "sv_ball_keeper_sidedive_longsidereach_opposite", "50", FCVAR_NOTIFY );
ConVar sv_ball_keeper_sidedive_zstart( "sv_ball_keeper_sidedive_zstart", "-50", FCVAR_NOTIFY );
ConVar sv_ball_keeper_sidedive_zend( "sv_ball_keeper_sidedive_zend", "70", FCVAR_NOTIFY );

ConVar sv_ball_keeperpunchupstrength("sv_ball_keeperpunchupstrength", "500", FCVAR_NOTIFY);
ConVar sv_ball_keeperdeflectioncoeff("sv_ball_keeperdeflectioncoeff", "1.66", FCVAR_NOTIFY);

ConVar sv_ball_shotdelay_normal("sv_ball_shotdelay_normal", "0.25", FCVAR_NOTIFY);
ConVar sv_ball_shotdelay_setpiece("sv_ball_shotdelay_setpiece", "0.5", FCVAR_NOTIFY);
ConVar sv_ball_shotdelay_global("sv_ball_shotdelay_global", "0.25", FCVAR_NOTIFY);
ConVar sv_ball_shotdelay_global_coeff("sv_ball_shotdelay_global_coeff", "0.75", FCVAR_NOTIFY);
ConVar sv_ball_dynamicshotdelay_enabled("sv_ball_dynamicshotdelay_enabled", "1", FCVAR_NOTIFY);
ConVar sv_ball_dynamicshotdelay_mindelay("sv_ball_dynamicshotdelay_mindelay", "0.05", FCVAR_NOTIFY);
ConVar sv_ball_dynamicshotdelay_maxdelay("sv_ball_dynamicshotdelay_maxdelay", "1.0", FCVAR_NOTIFY);
ConVar sv_ball_dynamicshotdelay_minshotstrength("sv_ball_dynamicshotdelay_minshotstrength", "100", FCVAR_NOTIFY);
ConVar sv_ball_dynamicshotdelay_maxshotstrength("sv_ball_dynamicshotdelay_maxshotstrength", "1600", FCVAR_NOTIFY);
ConVar sv_ball_dynamicbounce_enabled("sv_ball_dynamicbouncedelay_enabled", "1", FCVAR_NOTIFY);

ConVar sv_ball_bestshotangle("sv_ball_bestshotangle", "-30", FCVAR_NOTIFY);

ConVar sv_ball_pitchdown_exponent("sv_ball_pitchdown_exponent", "3", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchdowncoeff("sv_ball_fixedpitchdowncoeff", "0.15", FCVAR_NOTIFY);
ConVar sv_ball_pitchup_exponent("sv_ball_pitchup_exponent", "3", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchupcoeff("sv_ball_fixedpitchupcoeff", "0.0", FCVAR_NOTIFY);

ConVar sv_ball_bestbackspinangle("sv_ball_bestbackspinangle", "-55", FCVAR_NOTIFY);
ConVar sv_ball_pitchdownbackspin_exponent("sv_ball_pitchdownbackspin_exponent", "4", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchdownbackspincoeff("sv_ball_fixedpitchdownbackspincoeff", "0.1", FCVAR_NOTIFY);
ConVar sv_ball_pitchupbackspin_exponent("sv_ball_pitchupbackspin_exponent", "2", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchupbackspincoeff("sv_ball_fixedpitchupbackspincoeff", "0.1", FCVAR_NOTIFY);

ConVar sv_ball_besttopspinangle("sv_ball_besttopspinangle", "-15", FCVAR_NOTIFY);
ConVar sv_ball_pitchdowntopspin_exponent("sv_ball_pitchdowntopspin_exponent", "2", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchdowntopspincoeff("sv_ball_fixedpitchdowntopspincoeff", "0.1", FCVAR_NOTIFY);
ConVar sv_ball_pitchuptopspin_exponent("sv_ball_pitchuptopspin_exponent", "4", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchuptopspincoeff("sv_ball_fixedpitchuptopspincoeff", "0.1", FCVAR_NOTIFY);

ConVar sv_ball_shotwalkcoeff("sv_ball_shotwalkcoeff", "0.5", FCVAR_NOTIFY);
ConVar sv_ball_keepercatchspeed("sv_ball_keepercatchspeed", "500", FCVAR_NOTIFY);
ConVar sv_ball_keeperpickupangle("sv_ball_keeperpickupangle", "45", FCVAR_NOTIFY);

ConVar sv_ball_normalshot_strength("sv_ball_normalshot_strength", "700", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_powershot_strength("sv_ball_powershot_strength", "700", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_chargedshot_minstrength("sv_ball_chargedshot_minstrength", "1100", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_chargedshot_maxstrength("sv_ball_chargedshot_maxstrength", "1600", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);

ConVar sv_ball_normalthrow_strength("sv_ball_normalthrow_strength", "250", FCVAR_NOTIFY);
ConVar sv_ball_powerthrow_strength("sv_ball_powerthrow_strength", "500", FCVAR_NOTIFY);
ConVar sv_ball_chargedthrow_minstrength("sv_ball_chargedthrow_minstrength", "750", FCVAR_NOTIFY);
ConVar sv_ball_chargedthrow_maxstrength("sv_ball_chargedthrow_maxstrength", "1000", FCVAR_NOTIFY);

ConVar sv_ball_normalheader_strength("sv_ball_normalheader_strength", "500", FCVAR_NOTIFY); 
ConVar sv_ball_powerheader_strength("sv_ball_powerheader_strength", "700", FCVAR_NOTIFY); 
ConVar sv_ball_chargedheader_minstrength("sv_ball_chargedheader_minstrength", "900", FCVAR_NOTIFY); 
ConVar sv_ball_chargedheader_maxstrength("sv_ball_chargedheader_maxstrength", "1100", FCVAR_NOTIFY); 

ConVar sv_ball_powerdivingheader_strength("sv_ball_powerdivingheader_strength", "1100", FCVAR_NOTIFY); 
ConVar sv_ball_chargeddivingheader_minstrength("sv_ball_chargeddivingheader_minstrength", "1100", FCVAR_NOTIFY); 
ConVar sv_ball_chargeddivingheader_maxstrength("sv_ball_chargeddivingheader_maxstrength", "1300", FCVAR_NOTIFY);

ConVar sv_ball_normalslide_strength("sv_ball_normalslide_strength", "500", FCVAR_NOTIFY); 
ConVar sv_ball_powerslide_strength("sv_ball_powerslide_strength", "700", FCVAR_NOTIFY); 
ConVar sv_ball_chargedslide_minstrength("sv_ball_chargedslide_minstrength", "900", FCVAR_NOTIFY); 
ConVar sv_ball_chargedslide_maxstrength("sv_ball_chargedslide_maxstrength", "1100", FCVAR_NOTIFY);

ConVar sv_ball_penaltyshot_maxstrength("sv_ball_penaltyshot_maxstrength", "900", FCVAR_NOTIFY);

ConVar sv_ball_keepershot_minangle("sv_ball_keepershot_minangle", "-5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);

ConVar sv_ball_groundshot_minangle("sv_ball_groundshot_minangle", "-10", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_volleyshot_minangle("sv_ball_volleyshot_minangle", "0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_throwin_minangle("sv_ball_throwin_minangle", "-5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_throwin_minstrength("sv_ball_throwin_minstrength", "100", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_autopass_minstrength("sv_ball_autopass_minstrength", "300", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_autopass_maxstrength("sv_ball_autopass_maxstrength", "1600", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_autopass_coeff("sv_ball_autopass_coeff", "1", FCVAR_NOTIFY);
ConVar sv_ball_volleyshot_spincoeff("sv_ball_volleyshot_spincoeff", "1.25", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_doubletouchfouls("sv_ball_doubletouchfouls", "1", FCVAR_NOTIFY);

ConVar sv_ball_timelimit_setpiece("sv_ball_timelimit_setpiece", "15", FCVAR_NOTIFY);
ConVar sv_ball_timelimit_remotecontrolled("sv_ball_timelimit_remotecontrolled", "15", FCVAR_NOTIFY);

ConVar sv_ball_statetransition_activationdelay_normal("sv_ball_statetransition_activationdelay_normal", "1.0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_statetransition_activationdelay_long("sv_ball_statetransition_activationdelay_long", "2.0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_statetransition_messagedelay("sv_ball_statetransition_messagedelay", "1.0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_goalcelebduration("sv_ball_goalcelebduration", "5.0", FCVAR_NOTIFY);
ConVar sv_ball_thinkinterval("sv_ball_thinkinterval", "0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_chestdrop_strength("sv_ball_chestdrop_strength", "200", FCVAR_NOTIFY); 
ConVar sv_ball_chestdrop_angle("sv_ball_chestdrop_angle", "45", FCVAR_NOTIFY); 
ConVar sv_ball_minshotstrength("sv_ball_minshotstrength", "100", FCVAR_NOTIFY);  
ConVar sv_ball_minspeed_passive("sv_ball_minspeed_passive", "1000", FCVAR_NOTIFY); 
ConVar sv_ball_minspeed_bounce("sv_ball_minspeed_bounce", "500", FCVAR_NOTIFY);
ConVar sv_ball_bounce_strength("sv_ball_bounce_strength", "500", FCVAR_NOTIFY);
ConVar sv_ball_player_yellow_red_card_duration("sv_ball_player_yellow_red_card_duration", "6", FCVAR_NOTIFY);
ConVar sv_ball_player_red_card_duration("sv_ball_player_red_card_duration", "9", FCVAR_NOTIFY);
ConVar sv_ball_reset_stamina_on_freekicks("sv_ball_reset_stamina_on_freekicks", "1", FCVAR_NOTIFY);

ConVar sv_ball_bodypos_feet_start("sv_ball_bodypos_feet_start", "-10", FCVAR_NOTIFY);
ConVar sv_ball_bodypos_hip_start("sv_ball_bodypos_hip_start", "15", FCVAR_NOTIFY);
ConVar sv_ball_bodypos_chest_start("sv_ball_bodypos_chest_start", "40", FCVAR_NOTIFY);
ConVar sv_ball_bodypos_head_start("sv_ball_bodypos_head_start", "60", FCVAR_NOTIFY);
ConVar sv_ball_bodypos_head_end("sv_ball_bodypos_head_end", "85", FCVAR_NOTIFY);
ConVar sv_ball_bodypos_keeperarms_end("sv_ball_bodypos_keeperarms_end", "95", FCVAR_NOTIFY);

ConVar sv_ball_foulprobability("sv_ball_foulprobability", "50", FCVAR_NOTIFY);
ConVar sv_ball_yellowcardprobability_forward("sv_ball_yellowcardprobability_forward", "33", FCVAR_NOTIFY);
ConVar sv_ball_yellowcardprobability_backward("sv_ball_yellowcardprobability_backward", "66", FCVAR_NOTIFY);
ConVar sv_ball_goalreplay_count("sv_ball_goalreplay_count", "2", FCVAR_NOTIFY);
ConVar sv_ball_goalreplay_delay("sv_ball_goalreplay_delay", "3", FCVAR_NOTIFY);
ConVar sv_ball_deflectioncoeff("sv_ball_deflectioncoeff", "0.75", FCVAR_NOTIFY);
ConVar sv_ball_update_physics("sv_ball_update_physics", "0", FCVAR_NOTIFY);

ConVar sv_ball_stats_pass_mindist("sv_ball_stats_pass_mindist", "300", FCVAR_NOTIFY);
ConVar sv_ball_stats_assist_maxtime("sv_ball_stats_assist_maxtime", "5", FCVAR_NOTIFY);

ConVar sv_ball_velocity_coeff("sv_ball_velocity_coeff", "1.0", FCVAR_NOTIFY);


CBall *CreateBall(const Vector &pos, CSDKPlayer *pCreator)
{
	CBall *pBall = static_cast<CBall *>(CreateEntityByName("football"));
	pBall->SetCreator(pCreator);
	pBall->SetAbsOrigin(pos);
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

	if (pPl->GetFlags() & FL_REMOTECONTROLLED)
		return;

	Vector pos = pPl->GetLocalOrigin() + VEC_VIEW + pPl->EyeDirection3D() * 150;
	pos.z = max(pos.z, SDKGameRules()->m_vKickOff.GetZ());

	if (pPl->GetPlayerBall())
	{
		if (pPl->GetPlayerBall()->GetHoldingPlayer())
		{
			//pPl->GetPlayerBall()->RemoveFromPlayerHands(pPl->GetPlayerBall()->GetHoldingPlayer());
			pPl->GetPlayerBall()->State_Transition(BALL_NORMAL);
		}

		if (sv_ball_update_physics.GetBool())
			pPl->GetPlayerBall()->CreateVPhysics();

		pPl->GetPlayerBall()->SetPos(pos);
	}
	else
		pPl->SetPlayerBall(CreateBall(pos, pPl));

	pPl->m_Shared.SetStamina(100);
}

static ConCommand createplayerball("createplayerball", CC_CreatePlayerBall);

CBall *g_pBall = NULL;

CBall *GetBall()
{
	return g_pBall;
}

CBall *GetNearestBall(const Vector &pos)
{
	CBall *pNearestBall = GetBall();
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

LINK_ENTITY_TO_CLASS( football,	CBall );

//==========================================================
//	
//	
//==========================================================
BEGIN_DATADESC(	CBall )
	DEFINE_THINKFUNC( Think	),
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
	SendPropEHandle(SENDINFO(m_pCreator)),
	SendPropEHandle(SENDINFO(m_pMatchEventPlayer)),
	SendPropInt(SENDINFO(m_nMatchEventTeam)),
	SendPropEHandle(SENDINFO(m_pMatchSubEventPlayer)),
	SendPropInt(SENDINFO(m_nMatchSubEventTeam)),
	SendPropEHandle(SENDINFO(m_pMatchSubSubEventPlayer)),
	SendPropInt(SENDINFO(m_nMatchSubSubEventTeam)),
	SendPropBool(SENDINFO(m_bIsPlayerBall)),
	SendPropInt(SENDINFO(m_eMatchEvent)),
	SendPropInt(SENDINFO(m_eMatchSubEvent)),
	SendPropInt(SENDINFO(m_eMatchSubSubEvent)),
	SendPropInt(SENDINFO(m_eBallState)),
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
	m_eNextState = BALL_NOSTATE;
	m_flStateLeaveTime = gpGlobals->curtime;
	m_flStateActivationDelay = 0;
	m_flStateTimelimit = -1;
	m_eFoulType = FOUL_NONE;
	m_pPossessingPl = NULL;
	m_nPossessingTeam = TEAM_INVALID;
	m_flPossessionStart = -1;
	m_pPl = NULL;
	m_pMatchEventPlayer = NULL;
	m_pMatchSubEventPlayer = NULL;
	m_pMatchSubSubEventPlayer = NULL;
	m_bSetNewPos = false;
	m_bSetNewVel = false;
	m_bSetNewRot = false;
	m_bHasQueuedState = false;
	m_ePenaltyState = PENALTY_NONE;
	m_pCreator = NULL;
	m_bIsPlayerBall = false;
	m_pHoldingPlayer = NULL;
	m_flGlobalNextShot = gpGlobals->curtime;
	m_flShotStart = -1;
	m_nInPenBoxOfTeam = TEAM_INVALID;
	m_eMatchEvent = MATCH_EVENT_NONE;
	m_eMatchSubEvent = MATCH_EVENT_NONE;
	m_eMatchSubSubEvent = MATCH_EVENT_NONE;
}

CBall::~CBall()
{
	if (!m_bIsPlayerBall)
		g_pBall = NULL;
}

void CBall::RemoveAllPlayerBalls()
{
	CBall *pBall = NULL;

	while (true)
	{
		pBall = static_cast<CBall *>(gEntList.FindEntityByClassname(pBall, "football"));
		if (!pBall)
			break;

		if (pBall == this)
			continue;

		pBall->RemovePlayerBall();
	}
}

void CBall::RemovePlayerBall()
{
	if (GetHoldingPlayer())
		RemoveFromPlayerHands(GetHoldingPlayer());

	if (GetCreator())
		GetCreator()->SetPlayerBall(NULL);

	UTIL_Remove(this);
}

//==========================================================
//	
//	
//==========================================================
void CBall::Spawn (void)
{
	if (!g_pBall && !m_bIsPlayerBall)
		g_pBall = this;

	//RomD: Don't fade the ball
	SetFadeDistance(-1, 0);
	DisableAutoFade();

	PrecacheModel(BALL_MODEL);
	SetModel(BALL_MODEL);

	CreateVPhysics();

	SetThink(&CBall::Think);
	SetNextThink(gpGlobals->curtime + sv_ball_thinkinterval.GetFloat());

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
	params.damping = sv_ball_damping.GetFloat();
	params.mass = sv_ball_mass.GetFloat();
	params.dragCoefficient = sv_ball_dragcoeff.GetFloat();
	params.inertia = sv_ball_inertia.GetFloat();
	params.rotdamping = sv_ball_rotdamping.GetFloat();
	params.rotInertiaLimit = sv_ball_rotinertialimit.GetFloat();
	int	nMaterialIndex = physprops->GetSurfaceIndex("ios");
	m_pPhys = physenv->CreateSphereObject( m_flPhysRadius, nMaterialIndex, GetAbsOrigin(), GetAbsAngles(), &params, false );
	if (!m_pPhys)
		return false;

	VPhysicsSetObject( m_pPhys );
	
	SetSolid( SOLID_VPHYSICS );
	//SetSolidFlags( FSOLID_NOT_STANDABLE	);
	UTIL_SetSize(this, -Vector(m_flPhysRadius, m_flPhysRadius, m_flPhysRadius), Vector(m_flPhysRadius, m_flPhysRadius, m_flPhysRadius));

	SetMoveType( MOVETYPE_VPHYSICS );

	PhysSetGameFlags(m_pPhys, FVPHYSICS_NO_PLAYER_PICKUP);

	m_pPhys->SetMass(sv_ball_mass.GetFloat());//0.05f	);
	m_fMass	= m_pPhys->GetMass();
	//m_pPhys->EnableGravity(	sv_ball_enable_gravity.GetFloat() );
	m_pPhys->EnableGravity(true);
	//m_pPhys->EnableDrag( sv_ball_enable_drag.GetFloat() );
	m_pPhys->EnableDrag(sv_ball_drag_enabled.GetBool());
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
	EnablePlayerCollisions(true);
	m_pPhys->SetBuoyancyRatio(0.5f);
	m_pPhys->Wake();

	return true;
}

void CBall::VPhysicsUpdate(IPhysicsObject *pPhysics)
{
	Vector vel, worldAngImp;
	AngularImpulse angImp;
	m_pPhys->GetVelocity(&vel, &angImp);
	VectorRotate(angImp, EntityToWorldTransform(), worldAngImp);
	Vector magnusDir = worldAngImp.Cross(vel);

	if (vel.Length() > 0)
		vel += magnusDir * 1e-6 * sv_ball_curve.GetFloat() * gpGlobals->frametime;

	VPhysicsGetObject()->SetVelocity(&vel, &angImp);

	BaseClass::VPhysicsUpdate(pPhysics);

	m_bSetNewPos = false;
	m_bSetNewVel = false;
	m_bSetNewRot = false;
}


void CBall::VPhysicsCollision( int index, gamevcollisionevent_t	*pEvent	)
{
	Vector preVelocity = pEvent->preVelocity[index];
	float flSpeed =	VectorNormalize( preVelocity );
	int surfaceProps = pEvent->surfaceProps[!index];

	//IOS goal post hacks!!
	if (surfaceProps == 81 && flSpeed > 300.0f)
	{
		CSDKPlayer *pLastPl = LastPl(true);
		if (pLastPl
			&& (pLastPl->GetOppTeam()->m_vPlayerSpawns[0] - m_vPos).Length2DSqr() < (pLastPl->GetTeam()->m_vPlayerSpawns[0] - m_vPos).Length2DSqr()
			&& gpGlobals->curtime >= pLastPl->m_flLastShotOnGoal + 1)
		{
			pLastPl->SetShots(pLastPl->GetShots() + 1);
			pLastPl->SetShotsOnGoal(pLastPl->GetShotsOnGoal() + 1);
			pLastPl->m_flLastShotOnGoal = gpGlobals->curtime;
		}

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

		if (SDKGameRules()->State_Get() == MATCH_PENALTIES && m_ePenaltyState == PENALTY_KICKED && pPl != m_pPl)
		{
			m_ePenaltyState = PENALTY_SAVED;
		}
		else if (m_pCurStateInfo->m_eBallState == BALL_NORMAL)
		{
			Touched(pPl, false, BODY_PART_UNKNOWN);
		}

		EmitSound("Ball.touch");
	}

	//Warning ("surfaceprops index %d\n", surfaceProps);

	BaseClass::VPhysicsCollision( index, pEvent );
}

void CBall::SetMatchEvent(match_event_t matchEvent, int team, bool forceUpdate)
{
	if (!forceUpdate && (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState))
		return;

	/*if (pPlayer && matchEvent != MATCH_EVENT_NONE)
	{
		IOS_LogPrintf( "\"%s<%d><%s><%s>\" triggered \"%s\"\n", pPlayer->GetPlayerName(), pPlayer->GetUserID(), pPlayer->GetNetworkIDString(), pPlayer->GetTeam()->GetKitName(), g_szMatchEventNames[matchEvent]);
	}*/

	m_eMatchSubEvent = MATCH_EVENT_NONE;
	m_pMatchSubEventPlayer = NULL;
	m_eMatchSubSubEvent = MATCH_EVENT_NONE;
	m_pMatchSubSubEventPlayer = NULL;
	m_eMatchEvent = matchEvent;
	m_pMatchEventPlayer = NULL;
	m_nMatchEventTeam = team;
	m_flLastMatchEventSetTime = gpGlobals->curtime;
}

void CBall::SetMatchEventPlayer(CSDKPlayer *pPlayer, bool forceUpdate)
{
	if (!forceUpdate && (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState))
		return;

	m_pMatchEventPlayer = pPlayer;
	m_nMatchEventTeam = pPlayer->GetTeamNumber();
}

void CBall::SetMatchSubEvent(match_event_t matchEvent, int team, bool forceUpdate)
{
	if (!forceUpdate && (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState))
		return;

	//if (pPlayer)
	//{
	//	IOS_LogPrintf( "\"%s<%d><%s><%s>\" triggered \"%s\"\n", pPlayer->GetPlayerName(), pPlayer->GetUserID(), pPlayer->GetNetworkIDString(), pPlayer->GetTeam()->GetKitName(), g_szMatchEventNames[matchEvent]);
	//}

	m_eMatchSubEvent = matchEvent;
	m_pMatchSubEventPlayer = NULL;
	m_nMatchSubEventTeam = team;
}

void CBall::SetMatchSubEventPlayer(CSDKPlayer *pPlayer, bool forceUpdate)
{
	if (!forceUpdate && (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState))
		return;

	m_pMatchSubEventPlayer = pPlayer;
	m_nMatchSubEventTeam = pPlayer->GetTeamNumber();
}

void CBall::SetMatchSubSubEvent(match_event_t matchEvent, int team, bool forceUpdate)
{
	if (!forceUpdate && (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState))
		return;

	m_eMatchSubSubEvent = matchEvent;
	m_pMatchSubSubEventPlayer = NULL;
	m_nMatchSubSubEventTeam = team;
}

void CBall::SetMatchSubSubEventPlayer(CSDKPlayer *pPlayer, bool forceUpdate)
{
	if (!forceUpdate && (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState))
		return;

	m_pMatchSubSubEventPlayer = pPlayer;
	m_nMatchSubSubEventTeam = pPlayer->GetTeamNumber();
}

CSDKPlayer *CBall::FindNearestPlayer(int team /*= TEAM_INVALID*/, int posFlags /*= FL_POS_FIELD*/, bool checkIfShooting /*= false*/, int ignoredPlayerBits /*= 0*/)
{
	CSDKPlayer *pNearest = NULL;
	float shortestDist = FLT_MAX;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPlayer = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPlayer))
			continue;

		if (ignoredPlayerBits & (1 << (pPlayer->entindex() - 1)))
			continue;

		if (!(posFlags & FL_POS_ANY))
		{
			int posName = (int)g_Positions[mp_maxplayers.GetInt() - 1][pPlayer->GetTeamPosIndex()][POS_TYPE];

			if ((posFlags == FL_POS_FIELD) && !((1 << posName) & (g_nPosDefense + g_nPosMidfield + g_nPosAttack)))
				continue;

			if ((posFlags == FL_POS_KEEPER) && !((1 << posName) & g_nPosKeeper))
				continue;

			if ((posFlags == FL_POS_DEFENDER) && !((1 << posName) & g_nPosDefense))
				continue;

			if ((posFlags == FL_POS_MIDFIELDER) && !((1 << posName) & g_nPosMidfield))
				continue;

			if ((posFlags == FL_POS_ATTACKER) && !((1 << posName) & g_nPosAttack))
				continue;
		}

		if (team != TEAM_INVALID && pPlayer->GetTeamNumber() != team)
			continue;

		if (checkIfShooting && (!pPlayer->IsShooting() || pPlayer->m_flNextShot > gpGlobals->curtime))
			continue;

		Vector dir = m_vPos - pPlayer->GetLocalOrigin();
		float dist = dir.Length2D();

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

void CBall::SetPos(Vector pos)
{
	m_vPos = Vector(pos.x, pos.y, pos.z + m_flPhysRadius);
	m_vVel = vec3_origin;
	m_vRot = vec3_origin;
	m_pPhys->EnableMotion(true);
	m_pPhys->Wake();
	m_pPhys->SetVelocityInstantaneous(&vec3_origin, &vec3_origin);
	m_pPhys->SetPosition(m_vPos, m_aAng, true);
	m_pPhys->EnableMotion(false);
	m_bSetNewPos = true;
}

void CBall::SetVel(Vector vel, float spinCoeff, body_part_t bodyPart, bool isDeflection, bool markOffsidePlayers, bool checkMinShotStrength)
{
	m_vVel = vel * sv_ball_velocity_coeff.GetFloat();

	float length = m_vVel.Length();
	m_vVel.NormalizeInPlace();

	if (checkMinShotStrength)
		length = max(length, sv_ball_minshotstrength.GetInt());

	length = min(length, sv_ball_chargedshot_maxstrength.GetInt());
	m_vVel *= length;
	m_pPhys->EnableMotion(true);
	m_pPhys->Wake();
	m_pPhys->SetVelocity(&m_vVel, &m_vRot);
	m_bSetNewVel = true;

	if (spinCoeff != -1)
		SetSpin(spinCoeff);

	Kicked(bodyPart, isDeflection);
	
	if (markOffsidePlayers)
		MarkOffsidePlayers();
}

void CBall::SetRot(AngularImpulse rot)
{
	m_vRot = rot;
	m_pPhys->EnableMotion(true);
	m_pPhys->Wake();
	m_pPhys->SetVelocity(&m_vVel, &m_vRot);
	m_bSetNewRot = true;
}

ConVar mp_showballstatetransitions( "mp_showballstatetransitions", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show ball state transitions." );

void CBall::State_Transition(ball_state_t newState, float delay /*= 0.0f*/, bool cancelQueuedState /*= false*/)
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
		m_flStateLeaveTime = gpGlobals->curtime + sv_ball_statetransition_messagedelay.GetFloat() + m_flStateActivationDelay;
		m_bHasQueuedState = true;
	}
}

void CBall::State_Enter(ball_state_t newState, bool cancelQueuedState)
{
	if (cancelQueuedState)
	{
		m_eNextState = BALL_NOSTATE;
		m_bHasQueuedState = false;
	}

	m_eBallState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	m_flStateEnterTime = gpGlobals->curtime;
	m_flStateTimelimit = -1;
	m_bNextStateMessageSet = false;

	m_pPl = NULL;
	m_pOtherPl = NULL;

	if ( mp_showballstatetransitions.GetInt() > 0 )
	{
		if ( m_pCurStateInfo )
			IOS_LogPrintf( "Ball: entering state '%s'\n", m_pCurStateInfo->m_pStateName );
		else
			IOS_LogPrintf( "Ball: entering state #%d\n", newState );
	}

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		(this->*m_pCurStateInfo->pfnEnterState)();
	}

	//State_Think();
}

void CBall::State_Leave(ball_state_t newState)
{
	if (!m_bIsPlayerBall)
		SDKGameRules()->DisableShield();

	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)(newState);
	}
}

void CBall::State_Think()
{
	m_pPhys->GetPosition(&m_vPos, &m_aAng);
	m_pPhys->GetVelocity(&m_vVel, &m_vRot);

	if (m_eNextState != BALL_NOSTATE)
	{
		if (!m_bNextStateMessageSet && gpGlobals->curtime >= m_flStateLeaveTime - m_flStateActivationDelay)
		{
			m_bNextStateMessageSet = true;

			switch (m_eNextState)
			{
			case BALL_THROWIN: SetMatchEvent(MATCH_EVENT_THROWIN, LastOppTeam(false), true); break;
			case BALL_GOALKICK: SetMatchEvent(MATCH_EVENT_GOALKICK, LastOppTeam(false), true); break;
			case BALL_CORNER: SetMatchEvent(MATCH_EVENT_CORNER, LastOppTeam(false), true); break;
			//case BALL_KICKOFF: SetMatchEvent(MATCH_EVENT_KICKOFF, SDKGameRules()->GetKickOffTeam(), true); break;
			case BALL_GOAL:
				if (m_nTeam == LastTeam(true))
					SetMatchEvent(MATCH_EVENT_OWNGOAL, LastTeam(true), true);
				else
					SetMatchEvent(MATCH_EVENT_GOAL, LastTeam(true), true);
				break;
			case BALL_FREEKICK:
			case BALL_PENALTY:
				{
					if (m_eNextState == BALL_FREEKICK)
						SetMatchEvent(MATCH_EVENT_FREEKICK, GetGlobalTeam(m_nFoulingTeam)->GetOppTeamNumber(), true);
					else
						SetMatchEvent(MATCH_EVENT_PENALTY, GetGlobalTeam(m_nFoulingTeam)->GetOppTeamNumber(), true);

					match_event_t matchSubEvent;

					switch (m_eFoulType)
					{
					case FOUL_NORMAL_NO_CARD:
						matchSubEvent = MATCH_EVENT_FOUL;
						break;
					case FOUL_NORMAL_YELLOW_CARD:
						if (m_pFoulingPl->GetYellowCards() % 2 != 0)
							matchSubEvent = MATCH_EVENT_YELLOWREDCARD;
						else
							matchSubEvent = MATCH_EVENT_YELLOWCARD;
						break;
					case FOUL_NORMAL_RED_CARD:
						matchSubEvent = MATCH_EVENT_REDCARD;
						break;
					case FOUL_OFFSIDE:
						matchSubEvent = MATCH_EVENT_OFFSIDE;
						SDKGameRules()->SetOffsideLinesEnabled(true);
						break;
					case FOUL_DOUBLETOUCH:
						matchSubEvent = MATCH_EVENT_DOUBLETOUCH;
						break;
					default:
						matchSubEvent = MATCH_EVENT_FOUL;
						break;
					}

					SetMatchSubEvent(matchSubEvent, m_nFoulingTeam, true);
				}
				break;
			}
		}
		else if (gpGlobals->curtime >= m_flStateLeaveTime)
		{
			State_Leave(m_eNextState);
			State_Enter(m_eNextState, true);
		}
	}

	if (m_pCurStateInfo && m_pCurStateInfo->m_eBallState != BALL_NORMAL && m_eNextState == BALL_NOSTATE && m_flStateTimelimit != -1 && gpGlobals->curtime >= m_flStateTimelimit)
	{
		if (CSDKPlayer::IsOnField(m_pPl))
		{
			m_pPl->ChangeTeam(TEAM_SPECTATOR);
		}
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
		{ BALL_STATIC,		"BALL_STATIC",		&CBall::State_STATIC_Enter,			&CBall::State_STATIC_Think,			&CBall::State_STATIC_Leave },
		{ BALL_NORMAL,		"BALL_NORMAL",		&CBall::State_NORMAL_Enter,			&CBall::State_NORMAL_Think,			&CBall::State_NORMAL_Leave },
		{ BALL_KICKOFF,		"BALL_KICKOFF",		&CBall::State_KICKOFF_Enter,		&CBall::State_KICKOFF_Think,		&CBall::State_KICKOFF_Leave },
		{ BALL_THROWIN,		"BALL_THROWIN",		&CBall::State_THROWIN_Enter,		&CBall::State_THROWIN_Think,		&CBall::State_THROWIN_Leave },
		{ BALL_GOALKICK,	"BALL_GOALKICK",	&CBall::State_GOALKICK_Enter,		&CBall::State_GOALKICK_Think,		&CBall::State_GOALKICK_Leave },
		{ BALL_CORNER,		"BALL_CORNER",		&CBall::State_CORNER_Enter,			&CBall::State_CORNER_Think,			&CBall::State_CORNER_Leave },
		{ BALL_GOAL,		"BALL_GOAL",		&CBall::State_GOAL_Enter,			&CBall::State_GOAL_Think,			&CBall::State_GOAL_Leave },
		{ BALL_FREEKICK,	"BALL_FREEKICK",	&CBall::State_FREEKICK_Enter,		&CBall::State_FREEKICK_Think,		&CBall::State_FREEKICK_Leave },
		{ BALL_PENALTY,		"BALL_PENALTY",		&CBall::State_PENALTY_Enter,		&CBall::State_PENALTY_Think,		&CBall::State_PENALTY_Leave },
		{ BALL_KEEPERHANDS,	"BALL_KEEPERHANDS",	&CBall::State_KEEPERHANDS_Enter,	&CBall::State_KEEPERHANDS_Think,	&CBall::State_KEEPERHANDS_Leave },
	};

	for ( int i=0; i < ARRAYSIZE( ballStateInfos ); i++ )
	{
		if ( ballStateInfos[i].m_eBallState == state )
			return &ballStateInfos[i];
	}

	return NULL;
}

void CBall::FindStatePlayer(ball_state_t ballState /*= BALL_NOSTATE*/)
{
	if (ballState == BALL_NOSTATE)
		ballState = State_Get();

	switch (ballState)
	{
	case BALL_THROWIN:
		FindNearestPlayer(LastOppTeam(false));
		break;
	case BALL_GOALKICK:
		break;
	}
}

void CBall::State_STATIC_Enter()
{
	SDKGameRules()->StopMeteringInjuryTime();
}

void CBall::State_STATIC_Think()
{
}

void CBall::State_STATIC_Leave(ball_state_t newState)
{
	if (!SDKGameRules()->IsIntermissionState())
		SDKGameRules()->StartMeteringInjuryTime();
}

void CBall::State_NORMAL_Enter()
{
	m_pPhys->EnableMotion(true);
	EnablePlayerCollisions(true);
	m_pPhys->Wake();
	SDKGameRules()->StopMeteringInjuryTime();
	//SetMatchEvent(MATCH_EVENT_NONE);
	//SetMatchSubEvent(MATCH_EVENT_NONE);
}

void CBall::State_NORMAL_Think()
{
	if (m_eNextState == BALL_GOAL)
		return;

	for (int ignoredPlayerBits = 0;;)
	{
		if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
		{
			if (m_ePenaltyState == PENALTY_KICKED)
				m_pPl = FindNearestPlayer(m_nFoulingTeam, FL_POS_KEEPER, true);
			else
				m_pPl = NULL;
		}
		else
			m_pPl = FindNearestPlayer(TEAM_INVALID, FL_POS_ANY, true, ignoredPlayerBits);

		if (!m_pPl)
			return;

		UpdateCarrier();

		if (DoBodyPartAction())
		{
			if (m_pPl && (!LastInfo(false) || LastInfo(false)->m_eBodyPart != BODY_PART_HANDS))
			{
				if ((m_eMatchEvent != MATCH_EVENT_PASS
					&& m_eMatchEvent != MATCH_EVENT_INTERCEPTION
					&& m_eMatchEvent != MATCH_EVENT_KEEPERSAVE)
					|| gpGlobals->curtime >= m_flLastMatchEventSetTime + 0.5f)
				{
					SetMatchEvent(MATCH_EVENT_DRIBBLE, m_pPl->GetTeamNumber(), false);
					SetMatchEventPlayer(m_pPl, false);
				}
			}
			//m_pMatchEventPlayer = m_pPl;
			//m_nMatchEventTeam = m_pPl->GetTeamNumber();
			//m_eMatchEvent = MATCH_EVENT_NONE;
			//m_eMatchSubEvent = MATCH_EVENT_NONE;
			//m_pMatchSubEventPlayer = NULL;
			break;
		}

		if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
			break;

		ignoredPlayerBits |= (1 << (m_pPl->entindex() - 1));
	}
}

void CBall::State_NORMAL_Leave(ball_state_t newState)
{
	UnmarkOffsidePlayers();
	UpdatePossession(NULL);

	if (!SDKGameRules()->IsIntermissionState())
		SDKGameRules()->StartMeteringInjuryTime();
}

void CBall::State_KICKOFF_Enter()
{
	SetMatchEvent(MATCH_EVENT_KICKOFF, SDKGameRules()->GetKickOffTeam(), true);

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->m_Shared.SetStamina(100);
	}

	SetPos(SDKGameRules()->m_vKickOff);
}

void CBall::State_KICKOFF_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_ATTACKER);
		if (!m_pPl)
			m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_MIDFIELDER);
		if (!m_pPl)
			m_pPl = FindNearestPlayer(SDKGameRules()->GetKickOffTeam(), FL_POS_DEFENDER);

		if (!m_pPl)
			m_pPl = FindNearestPlayer(GetGlobalTeam(SDKGameRules()->GetKickOffTeam())->GetOppTeamNumber());
		if (!m_pPl)
		{
			SDKGameRules()->EnableShield(SHIELD_KICKOFF, GetGlobalTeam(TEAM_A)->GetTeamNumber(), SDKGameRules()->m_vKickOff);
			if (!PlayersAtTargetPos())
				return;

			return State_Transition(BALL_NORMAL);
		}

		SDKGameRules()->EnableShield(SHIELD_KICKOFF, m_pPl->GetTeamNumber(), SDKGameRules()->m_vKickOff);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x - m_pPl->GetTeam()->m_nRight * 30, m_vPos.y, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		EmitSound("Ball.whistle");
		SetMatchEvent(MATCH_EVENT_KICKOFF, m_pPl->GetTeamNumber(), false);
		SetMatchEventPlayer(m_pPl, false);
	}

	if (!CSDKPlayer::IsOnField(m_pOtherPl) || m_pOtherPl == m_pPl)
	{
		m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_ATTACKER, false, (1 << (m_pPl->entindex() - 1)));
		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_MIDFIELDER, false, (1 << (m_pPl->entindex() - 1)));
		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_DEFENDER, false, (1 << (m_pPl->entindex() - 1)));

		if (m_pOtherPl)
			m_pOtherPl->SetPosInsideShield(Vector(m_vPos.x + m_pPl->GetTeam()->m_nRight * 100, m_vPos.y, SDKGameRules()->m_vKickOff.GetZ()), true);
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
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsShooting())
	{
		RemoveAllTouches();
		SetVel(m_vPlForward2D * 250, 0, BODY_PART_FEET, false, false, false);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		if (m_pOtherPl)
			m_pOtherPl->RemoveFlag(FL_ATCONTROLS);
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_KICKOFF_Leave(ball_state_t newState)
{
}

void CBall::State_THROWIN_Enter()
{
	EnablePlayerCollisions(false);
	SetPos(Vector(m_vTriggerTouchPos.x + 0 * Sign(SDKGameRules()->m_vKickOff.GetX() - m_vTriggerTouchPos.x), m_vTriggerTouchPos.y, SDKGameRules()->m_vKickOff.GetZ()));
}

void CBall::State_THROWIN_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(LastOppTeam(false));
		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_THROWIN, m_pPl->GetTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vTriggerTouchPos.x, m_vTriggerTouchPos.y, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		SetMatchEventPlayer(m_pPl, false);
		EmitSound("Ball.whistle");
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
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsShooting())
	{
		QAngle ang = m_pPl->EyeAngles();

		ang[PITCH] = min(sv_ball_throwin_minangle.GetFloat(), m_aPlAng[PITCH]);

		Vector dir;
		AngleVectors(ang, &dir);
		float strength;

		if (m_pPl->IsNormalshooting())
			strength = GetNormalshotStrength(GetPitchCoeff(), sv_ball_normalthrow_strength.GetInt());
		else if (m_pPl->IsPowershooting())
			strength = GetPowershotStrength(GetPitchCoeff(), sv_ball_powerthrow_strength.GetInt());
		else
			strength = GetChargedshotStrength(GetPitchCoeff(), sv_ball_chargedthrow_minstrength.GetInt(), sv_ball_chargedthrow_maxstrength.GetInt());

		Vector vel = dir * max(strength, sv_ball_throwin_minstrength.GetInt());

		m_pPl->SetThrowIns(m_pPl->GetThrowIns() + 1);
		RemoveAllTouches();
		SetVel(vel, 0, BODY_PART_HANDS, false, false, false);
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_THROWIN_Leave(ball_state_t newState)
{
	if (CSDKPlayer::IsOnField(m_pPl))
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_THROW);

	EnablePlayerCollisions(true);
}

void CBall::State_GOALKICK_Enter()
{
	Vector ballPos;
	if (m_vTriggerTouchPos.x - SDKGameRules()->m_vKickOff.GetX() > 0)
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
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 100 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		SetMatchEventPlayer(m_pPl, false);
		EmitSound("Ball.whistle");
	}

	if (!PlayersAtTargetPos())
		return;

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_pPl->SetShotButtonsReleased(false);
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsShooting() && CanTouchBallXY())
	{
		m_pPl->SetGoalKicks(m_pPl->GetGoalKicks() + 1);
		RemoveAllTouches();
		DoGroundShot(false);
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_GOALKICK_Leave(ball_state_t newState)
{
}

void CBall::State_CORNER_Enter()
{
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

		SDKGameRules()->EnableShield(SHIELD_CORNER, m_pPl->GetTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x - 50 * Sign((SDKGameRules()->m_vKickOff - m_vPos).x), m_vPos.y - 50 * Sign((SDKGameRules()->m_vKickOff - m_vPos).y), SDKGameRules()->m_vKickOff[2]), true);
		m_flStateTimelimit = -1;
		SetMatchEventPlayer(m_pPl, false);
		EmitSound("Ball.whistle");
	}

	if (!PlayersAtTargetPos())
		return;

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_pPl->SetShotButtonsReleased(false);
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsShooting() && CanTouchBallXY())
	{
		m_pPl->SetCorners(m_pPl->GetCorners() + 1);
		RemoveAllTouches();
		DoGroundShot(false);
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_CORNER_Leave(ball_state_t newState)
{
}

void CBall::State_GOAL_Enter()
{
	SDKGameRules()->SetKickOffTeam(m_nTeam);
	int scoringTeam;

	if (m_nTeam == LastTeam(true))
	{
		scoringTeam = LastOppTeam(true);

		char matchEventPlayerNames[MAX_MATCH_EVENT_PLAYER_NAME_LENGTH] = {};

		if (LastPl(true))
		{
			LastPl(true)->SetOwnGoals(LastPl(true)->GetOwnGoals() + 1);
			Q_strncpy(matchEventPlayerNames, LastPl(true)->GetPlayerName(), MAX_PLAYER_NAME_LENGTH);
		}

		GetGlobalTeam(scoringTeam)->AddMatchEvent(SDKGameRules()->State_Get(), SDKGameRules()->GetMatchDisplayTimeSeconds(), MATCH_EVENT_OWNGOAL, matchEventPlayerNames);

		SetMatchEventPlayer(LastPl(true), false);	
	}
	else
	{
		scoringTeam = LastTeam(true);

		CSDKPlayer *pScorer = NULL;
		CSDKPlayer *pAssister = NULL;
		CSDKPlayer *pAssister2 = NULL;

		pScorer = LastPl(true);

		if (pScorer)
		{
			pScorer->SetGoals(pScorer->GetGoals() + 1);
			SetMatchEventPlayer(LastPl(true), false);

			pAssister = LastPl(true, pScorer);

			if (pAssister && pAssister->GetTeam() == pScorer->GetTeam() && gpGlobals->curtime - LastInfo(true, pScorer)->m_flTime <= sv_ball_stats_assist_maxtime.GetFloat())
			{
				pAssister->SetAssists(pAssister->GetAssists() + 1);
				SetMatchSubEvent(MATCH_EVENT_ASSIST, pAssister->GetTeamNumber(), true);
				SetMatchSubEventPlayer(pAssister, false);

				pAssister2 = LastPl(true, pScorer, pAssister);

				if (pAssister2 && pAssister2->GetTeam() == pScorer->GetTeam() && gpGlobals->curtime - LastInfo(true, pScorer, pAssister)->m_flTime <= sv_ball_stats_assist_maxtime.GetFloat())
				{
					pAssister2->SetAssists(pAssister2->GetAssists() + 1);
					SetMatchSubSubEvent(MATCH_EVENT_ASSIST, pAssister2->GetTeamNumber(), true);
					SetMatchSubSubEventPlayer(pAssister2, false);
				}
			}
		}

		char matchEventPlayerNames[MAX_MATCH_EVENT_PLAYER_NAME_LENGTH] = {};

		if (pScorer && !pAssister && !pAssister2)
			Q_strncpy(matchEventPlayerNames, UTIL_VarArgs("%s", pScorer->GetPlayerName()), MAX_MATCH_EVENT_PLAYER_NAME_LENGTH);
		else if (pScorer && pAssister && !pAssister2)
			Q_strncpy(matchEventPlayerNames, UTIL_VarArgs("%.14s (%.14s)", pScorer->GetPlayerName(), pAssister->GetPlayerName()), MAX_MATCH_EVENT_PLAYER_NAME_LENGTH);
		else if (pScorer && pAssister && pAssister2)
			Q_strncpy(matchEventPlayerNames, UTIL_VarArgs("%.8s (%.8s, %.8s)", pScorer->GetPlayerName(), pAssister->GetPlayerName(), pAssister2->GetPlayerName()), MAX_MATCH_EVENT_PLAYER_NAME_LENGTH);

		GetGlobalTeam(scoringTeam)->AddMatchEvent(SDKGameRules()->State_Get(), SDKGameRules()->GetMatchDisplayTimeSeconds(), MATCH_EVENT_GOAL, matchEventPlayerNames);
	}

	GetGlobalTeam(scoringTeam)->AddGoal();

	CSDKPlayer *pKeeper = FindNearestPlayer(m_nTeam, FL_POS_KEEPER);
	if (CSDKPlayer::IsOnField(pKeeper))
		pKeeper->SetGoalsConceded(pKeeper->GetGoalsConceded() + 1);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (pPl->GetTeamNumber() == scoringTeam)
			pPl->AddFlag(FL_CELEB);
	}

	EmitSound("Ball.whistle");
	
	float delay = sv_ball_goalcelebduration.GetFloat();

	if (sv_replays.GetBool())
	{
		delay += sv_replay_duration1.GetFloat();

		if (sv_replay_count.GetInt() >= 2)
			delay += sv_replay_duration2.GetFloat();

		if (sv_replay_count.GetInt() >= 3)
			delay += sv_replay_duration3.GetFloat();
	}

	State_Transition(BALL_KICKOFF, delay);

	if (ReplayManager())
		ReplayManager()->StartReplay(sv_replay_count.GetInt(), sv_ball_goalcelebduration.GetInt(), GetGlobalTeam(m_nTeam)->m_vPlayerSpawns[0].y < GetGlobalTeam(m_nTeam)->GetOppTeam()->m_vPlayerSpawns[0].y);
}

void CBall::State_GOAL_Think()
{
}

void CBall::State_GOAL_Leave(ball_state_t newState)
{
	if (ReplayManager())
		ReplayManager()->StopReplay();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->RemoveFlag(FL_CELEB);
		pPl->DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
	}
}

void CBall::State_FREEKICK_Enter()
{
	HandleFoul();

	SetPos(m_vFoulPos);
}

void CBall::State_FREEKICK_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		if ((m_vPos - GetGlobalTeam(m_nFoulingTeam)->GetOppTeam()->m_vPlayerSpawns[0]).Length2D() <= 1000)
			m_pPl = FindNearestPlayer(GetGlobalTeam(m_nFoulingTeam)->GetOppTeamNumber(), FL_POS_KEEPER);
		else if ((m_vPos - GetGlobalTeam(m_nFoulingTeam)->m_vPlayerSpawns[0]).Length2D() <= 1000)
			m_pPl = GetGlobalTeam(m_nFoulingTeam)->GetOppTeam()->GetFreekickTaker();
		else
			m_pPl = m_pFouledPl;

		if (!CSDKPlayer::IsOnField(m_pPl) || m_pPl->GetTeamPosType() == GK && m_pPl->IsBot())
			m_pPl = FindNearestPlayer(GetGlobalTeam(m_nFoulingTeam)->GetOppTeamNumber(), FL_POS_FIELD);

		if (!m_pPl)
			return State_Transition(BALL_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_FREEKICK, m_pPl->GetTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 100 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		SetMatchEventPlayer(m_pPl, false);
		EmitSound("Ball.whistle");
	}

	if (!PlayersAtTargetPos())
		return;

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();

		if (sv_ball_reset_stamina_on_freekicks.GetBool())
		{
			float fieldZone = 100 - (CalcFieldZone() + 100) / 2;
			if (m_pPl->GetTeam()->m_nForward == -1)
				fieldZone = 100 - fieldZone;
			m_pPl->m_Shared.SetStamina(min(m_pPl->m_Shared.GetStamina(), fieldZone));
		}

		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_pPl->SetShotButtonsReleased(false);
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsShooting() && CanTouchBallXY())
	{
		m_pPl->SetFreeKicks(m_pPl->GetFreeKicks() + 1);
		RemoveAllTouches();
		DoGroundShot(true);
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_FREEKICK_Leave(ball_state_t newState)
{
	SDKGameRules()->SetOffsideLinesEnabled(false);
}

void CBall::State_PENALTY_Enter()
{
	if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
	{
		SetPos(GetGlobalTeam(m_nFoulingTeam)->m_vPenalty);
	}
	else
	{
		HandleFoul();

		SetPos(GetGlobalTeam(m_nFoulingTeam)->m_vPenalty);

		if (m_pFoulingPl)
			SetMatchSubEventPlayer(m_pFoulingPl, false);
	}

	m_bPenaltyTakerStartedMoving = false;
}

void CBall::State_PENALTY_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
		{
			m_pPl = m_pFouledPl;
			if (!CSDKPlayer::IsOnField(m_pPl))
			{
				m_ePenaltyState = PENALTY_ABORTED_NO_TAKER;
				return State_Transition(BALL_NORMAL);
			}
		}
		else
		{
			m_pPl = GetGlobalTeam(m_nFoulingTeam)->GetOppTeam()->GetPenaltyTaker();
			if (!m_pPl)
				m_pPl = FindNearestPlayer(GetGlobalTeam(m_nFoulingTeam)->GetOppTeamNumber());
			if (!m_pPl)
				return State_Transition(BALL_NORMAL);
		}

		SDKGameRules()->EnableShield(SHIELD_PENALTY, GetGlobalTeam(m_nFoulingTeam)->GetOppTeamNumber(), GetGlobalTeam(m_nFoulingTeam)->m_vPenalty);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 150 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		SetMatchEventPlayer(m_pPl, false);
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

		Vector pos = m_pOtherPl->GetTeam()->m_vPlayerSpawns[0]; // + Vector(0, m_pOtherPl->GetTeam()->m_nForward * (mp_shield_border.GetInt() + 2), 0);
		m_pOtherPl->SetPosInsideShield(pos, true);
	}

	if (!PlayersAtTargetPos())
		return;

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		//m_pPl->AddFlag(FL_NO_X_MOVEMENT);
		//m_pPl->SetShotButtonsReleased(false);
		m_pOtherPl->RemoveFlag(FL_ATCONTROLS);
		//m_pOtherPl->AddFlag(FL_NO_Y_MOVEMENT);
		m_pOtherPl->AddFlag(FL_ATCONTROLS);
	}

	UpdateCarrier();

	//if (m_bPenaltyTakerStartedMoving && (m_vPlVel.y == 0 || Sign(m_vPlVel.y) != m_pPl->GetTeam()->m_nForward))
	//if ((m_vPlVel.y == 0 || Sign(m_vPlVel.y) != m_pPl->GetTeam()->m_nForward) && (m_vPos - m_vPlPos).Length2D() < 100)
	//{
	//	if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
	//	{
	//		m_ePenaltyState = PENALTY_ABORTED_ILLEGAL_MOVE;
	//		State_Transition(BALL_NORMAL);
	//	}
	//	else
	//	{
	//		TriggerFoul(FOUL_DOUBLETOUCH, m_vPos, m_pPl);
	//		State_Transition(BALL_FREEKICK, sv_ball_statetransition_activationdelay_normal.GetFloat());
	//	}

	//	return;
	//}

	//if (!m_bPenaltyTakerStartedMoving && m_vPlVel.y != 0 && Sign(m_vPlVel.y) == m_pPl->GetTeam()->m_nForward)
	//{
	//	m_bPenaltyTakerStartedMoving = true;
	//}

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsShooting() && CanTouchBallXY())
	{
		m_pPl->SetPenalties(m_pPl->GetPenalties() + 1);
		RemoveAllTouches();
		m_ePenaltyState = PENALTY_KICKED;
		m_pPl->m_ePenaltyState = PENALTY_KICKED;
		DoGroundShot(false);
		State_Transition(BALL_NORMAL);
	}
}

void CBall::State_PENALTY_Leave(ball_state_t newState)
{
	if (CSDKPlayer::IsOnField(m_pPl))
	{
		//m_pPl->RemoveFlag(FL_NO_X_MOVEMENT);
	}

	if (CSDKPlayer::IsOnField(m_pOtherPl))
	{
		//m_pOtherPl->RemoveFlag(FL_NO_Y_MOVEMENT);
		m_pOtherPl->RemoveFlag(FL_ATCONTROLS);
	}
}

void CBall::State_KEEPERHANDS_Enter()
{
	SetPos(m_vPos);
	// Don't ignore triggers when setting the new ball position
	m_bSetNewPos = false;
}

void CBall::State_KEEPERHANDS_Think()
{
	if (m_eNextState == BALL_GOAL)
		return;

	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(m_nInPenBoxOfTeam, FL_POS_KEEPER);
		if (!m_pPl)
		{
			return State_Transition(BALL_NORMAL);
		}

		if (!SDKGameRules()->IsIntermissionState() && !m_bHasQueuedState && SDKGameRules()->State_Get() != MATCH_PENALTIES)
		{
			SDKGameRules()->EnableShield(SHIELD_KEEPERHANDS, m_pPl->GetTeamNumber());
			//m_pPl->SetPosInsideShield(m_pPl->GetLocalOrigin(), true);
			m_pPl->m_bIsAtTargetPos = true;
		}

		m_pPl->SetShotButtonsReleased(false);
		m_pHoldingPlayer = m_pPl;
		m_pPl->m_pHoldingBall = this;
		m_pPl->m_nBody = MODEL_KEEPER_AND_BALL;
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_CARRY);
		m_pPl->m_nSkin = m_pPl->m_nBaseSkin + m_nSkin;
		SetEffects(EF_NODRAW);
		EnablePlayerCollisions(false);
		m_flStateTimelimit = -1;
		Touched(m_pPl, true, BODY_PART_HANDS);
		PlayersAtTargetPos();
	}

	//if (!PlayersAtTargetPos())
	//{
	//	UpdateCarrier();
	//	SetPos(Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_chest_start.GetFloat()) + m_vPlForward2D * 18);

	//	// Don't ignore triggers when setting the new ball position
	//	m_bSetNewPos = false;

	//	return;
	//}

	if (!SDKGameRules()->IsIntermissionState() && !m_bHasQueuedState && SDKGameRules()->State_Get() != MATCH_PENALTIES)
	{
		if (m_flStateTimelimit == -1)
		{
			m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
			//m_pPl->RemoveFlag(FL_ATCONTROLS);
		}
	}

	UpdateCarrier();

	SetPos(Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_chest_start.GetFloat()) + m_vPlForward2D * 18);

	// Don't ignore triggers when setting the new ball position
	m_bSetNewPos = false;

	Vector min = GetGlobalTeam(m_pPl->GetTeamNumber())->m_vPenBoxMin + m_flPhysRadius;
	Vector max = GetGlobalTeam(m_pPl->GetTeamNumber())->m_vPenBoxMax - m_flPhysRadius;

	if (m_vPos.x < min.x || m_vPos.y < min.y || m_vPos.x > max.x || m_vPos.y > max.y)
	{
		RemoveAllTouches();

		Vector dir;
		float vel;

		if (m_pPl->GetTeam()->m_nForward == 1 && m_vPos.y < min.y)
		{
			dir = Vector(g_IOSRand.RandomFloat(-0.5f, 0.5f), 1, g_IOSRand.RandomFloat(1.0f, 1.5f));
			dir.NormalizeInPlace();
			SetPos(Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_chest_start.GetFloat()) + dir * 18);
			m_bSetNewPos = false;
			vel = g_IOSRand.RandomFloat(400, 600);
		}
		else if (m_pPl->GetTeam()->m_nForward == -1 && m_vPos.y > max.y)
		{
			dir = Vector(g_IOSRand.RandomFloat(-0.5f, 0.5f), -1, g_IOSRand.RandomFloat(1.0f, 1.5f));
			dir.NormalizeInPlace();
			SetPos(Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_chest_start.GetFloat()) + dir * 18);
			m_bSetNewPos = false;
			vel = g_IOSRand.RandomFloat(400, 600);
		}
		else
		{
			dir = m_vPlForward2D;
			vel = 300;
		}

		//SetPos(Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_chest_start.GetFloat()) + forward * ((m_pPl->GetPlayerMaxs().x - m_pPl->GetPlayerMins().x) / 2 + m_flPhysRadius + 10));
		SetVel(dir * vel, 0, BODY_PART_HANDS, false, true, true);

		return State_Transition(BALL_NORMAL);
	}

	Vector vel;

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsShooting() && m_pPl->m_flNextShot <= gpGlobals->curtime)
	{
		//Vector hull = Vector(1, 1, 1) * m_flPhysRadius;
		//trace_t tr;
		//UTIL_TraceHull(m_vPos, m_vPos, -hull, hull, MASK_SOLID, m_pPl, COLLISION_GROUP_NONE, &tr);

		//if (!tr.startsolid)
		{
			float spin;

			if (m_pPl->IsNormalshooting())
			{
				vel = m_vPlForward * GetNormalshotStrength(GetPitchCoeff(), sv_ball_normalshot_strength.GetInt());
				m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KEEPER_HANDS_THROW);
				spin = 0;
			}
			else
			{
				QAngle ang = m_pPl->EyeAngles();
				ang[PITCH] = min(sv_ball_keepershot_minangle.GetFloat(), m_aPlAng[PITCH]);
				Vector dir;
				AngleVectors(ang, &dir);

				if (m_pPl->IsPowershooting())
					vel = dir * GetPowershotStrength(GetPitchCoeff(), sv_ball_powershot_strength.GetInt());
				else
					vel = dir * GetChargedshotStrength(GetPitchCoeff(), sv_ball_chargedshot_minstrength.GetInt(), sv_ball_chargedshot_maxstrength.GetInt());

				m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KEEPER_HANDS_KICK);
				spin = sv_ball_volleyshot_spincoeff.GetFloat();
			}

			RemoveAllTouches();
			SetVel(vel, spin, BODY_PART_HANDS, false, true, true);

			return State_Transition(BALL_NORMAL);
		}
	}
}

void CBall::State_KEEPERHANDS_Leave(ball_state_t newState)
{
	RemoveFromPlayerHands(m_pPl);
}

bool CBall::PlayersAtTargetPos()
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
				if (mp_shield_liberal_teammates_positioning.GetBool() && m_pCurStateInfo->m_eBallState != BALL_KICKOFF && m_pCurStateInfo->m_eBallState != BALL_PENALTY && pPl->GetTeamNumber() == m_pPl->GetTeamNumber())
					pPl->SetPosOutsideBall(pPl->GetLocalOrigin());
				else
					pPl->SetPosOutsideShield();
			}

			if (!pPl->m_bIsAtTargetPos)
			{
				if (pPl->m_flRemoteControlledStartTime == -1)
				{
					pPl->m_flRemoteControlledStartTime = gpGlobals->curtime;
					playersAtTarget = false;
				}
				else if (gpGlobals->curtime >= pPl->m_flRemoteControlledStartTime + sv_ball_timelimit_remotecontrolled.GetFloat())
					pPl->ChangeTeam(TEAM_SPECTATOR);
				else
					playersAtTarget = false;
			}
		}
	}

	return playersAtTarget;
}

bool CBall::CanTouchBallXY()
{
	return ((m_vPlDirToBall).Length2D() <= sv_ball_standing_reach.GetInt());
}

bool CBall::CheckFoul()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (pPl == m_pPl || pPl->GetTeamNumber() == m_pPl->GetTeamNumber())
			continue;

		Vector plPos = pPl->GetLocalOrigin();

		if (plPos.x < SDKGameRules()->m_vFieldMin.GetX() || plPos.y < SDKGameRules()->m_vFieldMin.GetY() ||
			plPos.x > SDKGameRules()->m_vFieldMax.GetX() || plPos.y > SDKGameRules()->m_vFieldMax.GetY())
			continue;

		Vector dirToPl = pPl->GetLocalOrigin() - m_vPlPos;
		float distToPl = dirToPl.Length2D();

		Vector localDirToPl;
		VectorIRotate(dirToPl, m_pPl->EntityToWorldTransform(), localDirToPl);

		dirToPl.z = 0;
		dirToPl.NormalizeInPlace();
		//if (RAD2DEG(acos(m_vPlForward2D.Dot(dirToPl))) > sv_ball_slideangle.GetFloat())
		if (localDirToPl.x < 0 || localDirToPl.x > sv_ball_slideforwardreach_foul.GetInt() || abs(localDirToPl.y) > sv_ball_slidesidereach_foul.GetInt())		
			continue;

		if (/*canShootBall && */distToPl >= (m_vPos - m_vPlPos).Length2D())
			continue;

		// It's a foul

		if (g_IOSRand.RandomInt(1, 100) > sv_ball_foulprobability.GetInt())
			continue;

		PlayerAnimEvent_t anim = RAD2DEG(acos(m_vPlForward2D.Dot(pPl->EyeDirection2D()))) <= 90 ? PLAYERANIMEVENT_TACKLED_BACKWARD : PLAYERANIMEVENT_TACKLED_FORWARD;

		pPl->DoAnimationEvent(anim);

		int teammatesCloserToGoalCount = 0;

		bool isCloseToOwnGoal = CalcFieldZone() * m_pPl->GetTeam()->m_nForward < -50;

		if (isCloseToOwnGoal)
		{
			for (int j = 1; j <= gpGlobals->maxClients; j++) 
			{
				CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(j));

				if (!CSDKPlayer::IsOnField(pPl) || pPl == m_pPl || pPl->GetTeamNumber() != m_pPl->GetTeamNumber() || pPl->GetTeamPosType() == GK)
					continue;

				if ((m_pPl->GetTeam()->m_vPlayerSpawns[0] - pPl->GetLocalOrigin()).Length2DSqr() < (m_pPl->GetTeam()->m_vPlayerSpawns[0] - m_vPlPos).Length2DSqr())
					teammatesCloserToGoalCount += 1;
			}
		}

		foul_type_t foulType;

		if (isCloseToOwnGoal && teammatesCloserToGoalCount == 0)
			foulType = FOUL_NORMAL_RED_CARD;
		else if (anim == PLAYERANIMEVENT_TACKLED_FORWARD && g_IOSRand.RandomInt(1, 100) <= sv_ball_yellowcardprobability_forward.GetInt() ||
				 anim == PLAYERANIMEVENT_TACKLED_BACKWARD && g_IOSRand.RandomInt(1, 100) <= sv_ball_yellowcardprobability_backward.GetInt())
			foulType = FOUL_NORMAL_YELLOW_CARD;
		else
			foulType = FOUL_NORMAL_NO_CARD;

		TriggerFoul(foulType, pPl->GetLocalOrigin(), m_pPl, pPl);

		if (pPl->m_nInPenBoxOfTeam == m_pPl->GetTeamNumber())
			State_Transition(BALL_PENALTY, sv_ball_statetransition_activationdelay_long.GetFloat());
		else
			State_Transition(BALL_FREEKICK, sv_ball_statetransition_activationdelay_long.GetFloat());

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
	m_vFoulPos.x = clamp(pos.x, SDKGameRules()->m_vFieldMin.GetX() + 2 * m_flPhysRadius, SDKGameRules()->m_vFieldMax.GetX() - 2 * m_flPhysRadius);
	m_vFoulPos.y = clamp(pos.y, SDKGameRules()->m_vFieldMin.GetY() + 2 * m_flPhysRadius, SDKGameRules()->m_vFieldMax.GetY() - 2 * m_flPhysRadius);
	m_vFoulPos.z = SDKGameRules()->m_vKickOff.GetZ();
}

void CBall::HandleFoul()
{
	switch (m_eFoulType)
	{
	case FOUL_NORMAL_NO_CARD:
	case FOUL_NORMAL_YELLOW_CARD:
	case FOUL_NORMAL_RED_CARD:
		if (CSDKPlayer::IsOnField(m_pFoulingPl))
			m_pFoulingPl->SetFouls(m_pFoulingPl->GetFouls() + 1);
		break;
	}

	if (CSDKPlayer::IsOnField(m_pFoulingPl))
	{
		SetMatchSubEventPlayer(m_pFoulingPl, false);

		if (m_eFoulType == FOUL_NORMAL_YELLOW_CARD)
		{
			m_pFoulingPl->SetYellowCards(m_pFoulingPl->GetYellowCards() + 1);

			if (m_pFoulingPl->GetYellowCards() % 2 != 0)
				m_pFoulingPl->GetTeam()->AddMatchEvent(SDKGameRules()->State_Get(), SDKGameRules()->GetMatchDisplayTimeSeconds(), MATCH_EVENT_YELLOWCARD, m_pFoulingPl->GetPlayerName());
		}

		if (m_eFoulType == FOUL_NORMAL_YELLOW_CARD && m_pFoulingPl->GetYellowCards() % 2 == 0 || m_eFoulType == FOUL_NORMAL_RED_CARD)
		{
			m_pFoulingPl->SetRedCards(m_pFoulingPl->GetRedCards() + 1);
			m_pFoulingPl->GetTeam()->AddMatchEvent(SDKGameRules()->State_Get(), SDKGameRules()->GetMatchDisplayTimeSeconds(), m_eFoulType == FOUL_NORMAL_YELLOW_CARD ? MATCH_EVENT_YELLOWREDCARD : MATCH_EVENT_REDCARD, m_pFoulingPl->GetPlayerName());

			int banDuration = 60 * (m_eFoulType == FOUL_NORMAL_YELLOW_CARD ? sv_ball_player_yellow_red_card_duration.GetFloat() : sv_ball_player_red_card_duration.GetFloat());

			m_pFoulingPl->SetCardBanned(true);
			m_pFoulingPl->SetNextJoin(SDKGameRules()->GetMatchDisplayTimeSeconds() + banDuration);
			int team = m_pFoulingPl->GetTeamNumber();
			int posIndex = m_pFoulingPl->GetTeamPosIndex();
			int posType = m_pFoulingPl->GetTeamPosType();
			m_pFoulingPl->ChangeTeam(TEAM_SPECTATOR);

			if (posType == GK)
			{
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
					if (!CSDKPlayer::IsOnField(pPl) || pPl == m_pFoulingPl || pPl->GetTeamNumber() != team)
						continue;

					pPl->ChangeTeamPos(team, posIndex, false);
					break;
				}
			}
			else
				GetGlobalTeam(team)->SetPosNextJoinSeconds(posIndex, SDKGameRules()->GetMatchDisplayTimeSeconds() + banDuration);

			//m_pFoulingPl->ChangeTeamPos(team, posIndex, false);
		}
	}

	if (CSDKPlayer::IsOnField(m_pFouledPl))
	{
		m_pFouledPl->SetFoulsSuffered(m_pFouledPl->GetFoulsSuffered() + 1);
	}
}

bool CBall::DoBodyPartAction()
{
	Vector dirToBall = m_vPos - m_vPlPos;
	Vector localDirToBall;
	VectorIRotate(dirToBall, m_pPl->EntityToWorldTransform(), localDirToBall);
	float zDist = dirToBall.z;
	float xyDist = dirToBall.Length2D();

	bool canCatch = false;

	if (m_pPl->GetTeamPosType() == GK && m_nInPenBoxOfTeam == m_pPl->GetTeamNumber() && !m_pPl->m_pHoldingBall && m_pPl->m_nInPenBoxOfTeam == m_pPl->GetTeamNumber())
	{
		if (!SDKGameRules()->IsIntermissionState() && SDKGameRules()->State_Get() != MATCH_PENALTIES)
		{
			// Can catch if either another player shot the ball last or before me. Excludes Goal kicks, free kicks and shots from hand, since touches get deleted there.
			if (LastPl(true) != m_pPl || LastPl(true, m_pPl))
			{
				BallTouchInfo *pInfo = LastInfo(false, m_pPl);

				if (pInfo)
				{
					// Can always catch balls touched or shot by opponents. Only check teammate touches and shots.
					if (pInfo->m_nTeam == m_pPl->GetTeamNumber())
					{
						if (!pInfo->m_bIsShot || pInfo->m_eBodyPart == BODY_PART_HEAD || pInfo->m_eBodyPart == BODY_PART_CHEST)
						{
							BallTouchInfo *pShotInfo = LastInfo(true, m_pPl);

							if (pInfo->m_bIsShot || !pShotInfo || pShotInfo->m_nTeam != m_pPl->GetTeamNumber()
								|| pShotInfo->m_eBodyPart == BODY_PART_HEAD || pShotInfo->m_eBodyPart == BODY_PART_CHEST)
							{
								canCatch = true;
							}
						}
					}
					else
					{
						canCatch = true;
					}
				}
			}
		}
		else
		{
			canCatch = true;
		}
	}

	if (canCatch)
		return CheckKeeperCatch();

	if (m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_SLIDE)
		return DoSlideAction();

	if (gpGlobals->curtime < m_flGlobalNextShot)
	{
		if (zDist >= sv_ball_bodypos_feet_start.GetFloat() && zDist < sv_ball_bodypos_head_end.GetFloat() && xyDist <= sv_ball_deflectionradius.GetInt())
		{
			Vector dir = dirToBall;
			dir.z = 0;
			dir.NormalizeInPlace();
			Vector vel = sv_ball_deflectioncoeff.GetFloat() * (m_vVel - 2 * DotProduct(m_vVel, dir) * dir);
			SetVel(vel, -1, BODY_PART_UNKNOWN, true, false, false);

			return true;
		}

		return false;
	}

	if (zDist >= sv_ball_bodypos_feet_start.GetFloat()
		&& zDist < sv_ball_bodypos_hip_start.GetFloat()
		&& CanTouchBallXY())
	{
		return DoGroundShot(true);
	}

	if (zDist >= sv_ball_bodypos_hip_start.GetFloat() && zDist < sv_ball_bodypos_chest_start.GetFloat() && CanTouchBallXY())
	{
		if (DoVolleyShot())
			return true;
		else
			return DoChestDrop();
	}

	if (zDist >= sv_ball_bodypos_chest_start.GetFloat() && zDist < sv_ball_bodypos_head_start.GetFloat() && CanTouchBallXY())
		return DoChestDrop();

	if (zDist >= sv_ball_bodypos_head_start.GetFloat() && zDist < sv_ball_bodypos_head_end.GetFloat() && CanTouchBallXY())
		return DoHeader();

	return false;
}

bool CBall::DoSlideAction()
{
	Vector dirToBall = m_vPos - m_vPlPos;
	float zDist = dirToBall.z;
	Vector localDirToBall;
	VectorIRotate(dirToBall, m_pPl->EntityToWorldTransform(), localDirToBall);

	bool canShootBall = zDist < sv_ball_slidezend.GetFloat()
		&& zDist >= sv_ball_slidezstart.GetFloat()
		&& localDirToBall.x >= 0
		&& localDirToBall.x <= sv_ball_slideforwardreach_ball.GetFloat()
		&& abs(localDirToBall.y) <= sv_ball_slidesidereach_ball.GetFloat();

	if (!canShootBall)
		return false;

	if (!SDKGameRules()->IsIntermissionState() && !m_bHasQueuedState)
	{
		if (CheckFoul())
			return true;
	}

	float shotStrength;

	if (m_pPl->IsNormalshooting())
		shotStrength = GetNormalshotStrength(1.0f, sv_ball_normalslide_strength.GetInt());
	else if (m_pPl->IsPowershooting())
		shotStrength = GetPowershotStrength(1.0f, sv_ball_powerslide_strength.GetInt());
	else
		shotStrength = GetChargedshotStrength(1.0f, sv_ball_chargedslide_minstrength.GetInt(), sv_ball_chargedslide_maxstrength.GetInt());

	Vector forward;
	AngleVectors(QAngle(-15, m_aPlAng[YAW], 0), &forward, NULL, NULL);

	Vector ballVel = forward * shotStrength;

	if (m_pPl->m_nButtons & IN_MOVELEFT)
	{
		VectorYawRotate(ballVel, 45, ballVel);
		ballVel *= sv_ball_slidesidespeedcoeff.GetFloat();
	}
	else if (m_pPl->m_nButtons & IN_MOVERIGHT)
	{
		VectorYawRotate(ballVel, -45, ballVel);
		ballVel *= sv_ball_slidesidespeedcoeff.GetFloat();
	}

	SetVel(ballVel, 0, BODY_PART_FEET, false, true, false);
	m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
	return true;
}

bool CBall::CheckKeeperCatch()
{
	Vector dirToBall = m_vPos - m_vPlPos;
	float zDist = dirToBall.z;
	float xyDist = dirToBall.Length2D();
	Vector localDirToBall;
	VectorIRotate(dirToBall, m_pPl->EntityToWorldTransform(), localDirToBall);

	bool canCatch;

	switch (m_pPl->m_Shared.GetAnimEvent())
	{
	case PLAYERANIMEVENT_KEEPER_DIVE_LEFT:
		canCatch = (zDist < sv_ball_keeper_sidedive_zend.GetInt()
			&& zDist >= sv_ball_keeper_sidedive_zstart.GetInt()
			&& abs(localDirToBall.x) <= sv_ball_keeper_sidedive_shortsidereach.GetInt()
			&& localDirToBall.y >= -sv_ball_keeper_sidedive_longsidereach_opposite.GetInt()
			&& localDirToBall.y <= sv_ball_keeper_sidedive_longsidereach.GetInt());
		break;
	case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT:
		canCatch = (zDist < sv_ball_keeper_sidedive_zend.GetInt()
			&& zDist >= sv_ball_keeper_sidedive_zstart.GetInt()
			&& abs(localDirToBall.x) <= sv_ball_keeper_sidedive_shortsidereach.GetInt()
			&& localDirToBall.y <= sv_ball_keeper_sidedive_longsidereach_opposite.GetInt()
			&& localDirToBall.y >= -sv_ball_keeper_sidedive_longsidereach.GetInt());
		break;
	case PLAYERANIMEVENT_KEEPER_DIVE_FORWARD:
		canCatch = (zDist < sv_ball_keeper_forwarddive_zend.GetInt()
			&& zDist >= sv_ball_keeper_forwarddive_zstart.GetInt()
			&& localDirToBall.x >= -sv_ball_keeper_forwarddive_longsidereach_opposite.GetInt()
			&& localDirToBall.x <= sv_ball_keeper_forwarddive_longsidereach.GetInt()
			&& abs(localDirToBall.y) <= sv_ball_keeper_forwarddive_shortsidereach.GetInt());
		break;
	//case PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD:
	//	canCatch = (zDist < sv_ball_bodypos_keeperarms_end.GetInt()
	//		&& zDist >= sv_ball_keeperdivezstart.GetInt()
	//		&& localDirToBall.x <= 0
	//		&& localDirToBall.x >= -sv_ball_keeperlongsidereach.GetInt() - sv_ball_keeperlongsidereach_opposite.GetInt()
	//		&& abs(localDirToBall.y) <= sv_ball_keepershortsidereach.GetInt());
	//	break;
	case PLAYERANIMEVENT_KEEPER_JUMP:
	default:
		canCatch = (zDist < sv_ball_bodypos_keeperarms_end.GetInt()
			&& zDist >= sv_ball_bodypos_feet_start.GetInt()
			&& xyDist <= sv_ball_keeper_standing_reach.GetInt());
		break;
	}

	if (!canCatch)
		return false;

	if (!SDKGameRules()->IsIntermissionState() && !m_bHasQueuedState && LastTeam(true) != m_pPl->GetTeamNumber())
	{
		m_pPl->SetKeeperSaves(m_pPl->GetKeeperSaves() + 1);
		if (LastPl(true) && gpGlobals->curtime >= LastPl(true)->m_flLastShotOnGoal + 1)
		{
			LastPl(true)->SetShots(LastPl(true)->GetShots() + 1);
			LastPl(true)->SetShotsOnGoal(LastPl(true)->GetShotsOnGoal() + 1);
			LastPl(true)->m_flLastShotOnGoal = gpGlobals->curtime;
		}
	}

	//SetMatchEvent(MATCH_EVENT_KEEPERSAVE, m_pPl->GetTeamNumber(), false);
	//SetMatchEventPlayer(m_pPl, false);

	if (gpGlobals->curtime < m_flGlobalNextShot || m_bHasQueuedState)
	{
		Vector vel;

		if (m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD)
		{
			//vel = Vector(m_vVel.x, m_vVel.y, max(m_vVel.z, sv_ball_keeperpunchupstrength.GetInt()));
			vel = -m_vPlForward2D * m_vVel.Length2D() * sv_ball_keeperdeflectioncoeff.GetFloat();
			float length = vel.Length();
			QAngle ang;
			VectorAngles(vel, ang);
			ang[PITCH] = -45;
			AngleVectors(ang, &vel);
			vel *= length;
		}
		else
		{
			Vector dir;
			QAngle ang = m_aPlAng;
			ang[PITCH] -= 45;
			AngleVectors(ang, &dir);
			vel = dir * m_vVel.Length2D() * sv_ball_keeperdeflectioncoeff.GetFloat();
		}

		SetVel(vel, -1, BODY_PART_HANDS, false, true, false);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
	}
	else
	{
		SetVel(vec3_origin, -1, BODY_PART_HANDS, true, false, false);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);		
		State_Transition(BALL_KEEPERHANDS);
	}

	return true;
}

float CBall::GetPitchCoeff()
{
	//return pow(cos((m_aPlAng[PITCH] - sv_ball_bestshotangle.GetInt()) / (PITCH_LIMIT - sv_ball_bestshotangle.GetInt()) * M_PI / 2), 2);
	// plot 0.5 + (cos(x/89 * pi/2) * 0.5), x=-89..89

	float bestAng = sv_ball_bestshotangle.GetInt();
	float downCoeff = sv_ball_fixedpitchdowncoeff.GetFloat();
	float upCoeff = sv_ball_fixedpitchupcoeff.GetFloat();
	double downExp = sv_ball_pitchdown_exponent.GetFloat();
	double upExp = sv_ball_pitchup_exponent.GetFloat();
	float pitch = m_aPlAng[PITCH];

	float coeff;

	if (pitch >= sv_ball_bestshotangle.GetInt())
	{
		coeff = downCoeff + (1 - downCoeff) * pow(cos((pitch - bestAng) / (PITCH_LIMIT - bestAng) * M_PI / 2), downExp);
	}
	else
	{
		coeff = upCoeff + (1 - upCoeff) * pow(cos((pitch - bestAng) / (PITCH_LIMIT - bestAng) * M_PI / 2), upExp);
	}

	if (m_pPl->m_nButtons & IN_WALK)
		coeff *= sv_ball_shotwalkcoeff.GetFloat();

	//DevMsg("coeff: %.2f\n", coeff);

	return coeff;
}

float CBall::GetNormalshotStrength(float coeff, int strength)
{
	return (coeff * strength);
}

float CBall::GetPowershotStrength(float coeff, int strength)
{
	if (State_Get() == BALL_PENALTY)
		strength = sv_ball_penaltyshot_maxstrength.GetInt();

	return coeff * strength;
}

float CBall::GetChargedshotStrength(float coeff, int minStrength, int maxStrength)
{
	if (State_Get() == BALL_PENALTY)
		minStrength = maxStrength = sv_ball_penaltyshot_maxstrength.GetInt();

	float duration = (m_pPl->m_Shared.m_bIsShotCharging ? gpGlobals->curtime - m_pPl->m_Shared.m_flShotChargingStart : m_pPl->m_Shared.m_flShotChargingDuration);
	float totalTime = gpGlobals->curtime - m_pPl->m_Shared.m_flShotChargingStart;
	float activeTime = min(duration, mp_chargedshot_increaseduration.GetFloat());
	float extra = totalTime - activeTime;
	float increaseFraction = clamp(pow(activeTime / mp_chargedshot_increaseduration.GetFloat(), mp_chargedshot_increaseexponent.GetFloat()), 0, 1);
	float decTime = (pow(1 - increaseFraction, 1.0f / mp_chargedshot_decreaseexponent.GetFloat())) * mp_chargedshot_decreaseduration.GetFloat();
	float decreaseFraction = clamp((decTime + extra) / mp_chargedshot_decreaseduration.GetFloat(), 0, 1);
	float shotStrengthCoeff = 1 - pow(decreaseFraction, mp_chargedshot_decreaseexponent.GetFloat());
	float shotStrength = minStrength + (maxStrength - minStrength) * shotStrengthCoeff;

	return coeff * shotStrength;
}

bool CBall::DoGroundShot(bool markOffsidePlayers)
{
	float spin;
	Vector vel;

	if (m_pPl->IsAutoPassing())
	{
		CSDKPlayer *pPl = m_pPl->FindClosestPlayerToSelf(true, true, 180);
		if (!pPl)
			pPl = m_pPl->FindClosestPlayerToSelf(true, false, 180);
		if (!pPl)
			return false;

		Vector dir = pPl->GetLocalOrigin() - m_vPlPos;
		float length = dir.Length2D();
		QAngle ang;
		VectorAngles(dir, ang);
		ang[PITCH] = -30;
		AngleVectors(ang, &dir);

		float shotStrength = clamp(length * sv_ball_autopass_coeff.GetFloat(), sv_ball_autopass_minstrength.GetInt(), sv_ball_autopass_maxstrength.GetInt());
		float staminaUsage = min(m_pPl->m_Shared.GetStamina(), shotStrength * 100 / sv_ball_autopass_maxstrength.GetInt());
		shotStrength = max(sv_ball_minshotstrength.GetInt(), staminaUsage / 100.0f * sv_ball_autopass_maxstrength.GetInt());
		m_pPl->m_Shared.SetStamina(m_pPl->m_Shared.GetStamina() - staminaUsage);

		vel = dir * shotStrength;
		spin = 0;
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_PASS);
		EmitSound("Ball.kicknormal");
	}
	else
	{
		Vector dirToBall = m_vPos - m_vPlPos;
		Vector localDirToBall;
		VectorIRotate(dirToBall, m_pPl->EntityToWorldTransform(), localDirToBall);

		QAngle shotAngle = m_aPlAng;
		shotAngle[PITCH] = min(sv_ball_groundshot_minangle.GetFloat(), m_aPlAng[PITCH]);

		Vector shotDir;
		AngleVectors(shotAngle, &shotDir);

		float shotStrength;

		if (m_pPl->IsNormalshooting())
			shotStrength = GetNormalshotStrength(GetPitchCoeff(), sv_ball_normalshot_strength.GetInt());
		else if (m_pPl->IsPowershooting())
			shotStrength = GetPowershotStrength(GetPitchCoeff(), sv_ball_powershot_strength.GetInt());
		else
			shotStrength = GetChargedshotStrength(GetPitchCoeff(), sv_ball_chargedshot_minstrength.GetInt(), sv_ball_chargedshot_maxstrength.GetInt());

		vel = shotDir * shotStrength;

		if (vel.Length() > 700)
		{
			PlayerAnimEvent_t anim = PLAYERANIMEVENT_BLANK;
			EmitSound("Ball.kickhard");

			//if (m_vVel.Length() > 800)
			anim = PLAYERANIMEVENT_KICK;
			//else
			//	anim = PLAYERANIMEVENT_PASS;

			m_pPl->DoServerAnimationEvent(anim);
		}
		else
		{
			if (localDirToBall.x < 0 && m_aPlAng[PITCH] <= -45)
				m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HEELKICK);
			else
				m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);

			EmitSound("Ball.touch");
		}

		spin = 1;
	}

	SetVel(vel, spin, BODY_PART_FEET, false, markOffsidePlayers, true);

	return true;
}

bool CBall::DoVolleyShot()
{
	//if (!m_pPl->IsPowershooting() || m_pPl->GetGroundEntity())
	//	return false;

	float shotStrength;

	if (m_pPl->IsNormalshooting())
		shotStrength = GetNormalshotStrength(GetPitchCoeff(), sv_ball_normalshot_strength.GetInt());
	else if (m_pPl->IsPowershooting())
		shotStrength = GetPowershotStrength(GetPitchCoeff(), sv_ball_powershot_strength.GetInt());
	else
		shotStrength = GetChargedshotStrength(GetPitchCoeff(), sv_ball_chargedshot_minstrength.GetInt(), sv_ball_chargedshot_maxstrength.GetInt());

	QAngle shotAngle = m_aPlAng;
	shotAngle[PITCH] = min(sv_ball_volleyshot_minangle.GetFloat(), m_aPlAng[PITCH]);

	Vector shotDir;
	AngleVectors(shotAngle, &shotDir);

	Vector vel = shotDir * shotStrength;

	if (vel.Length() > 700)
	{
		EmitSound("Ball.kickhard");
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_VOLLEY);
	}
	else
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);

	SetVel(vel, sv_ball_volleyshot_spincoeff.GetFloat(), BODY_PART_FEET, false, true, true);

	return true;
}

bool CBall::DoChestDrop()
{
	QAngle ang = QAngle(sv_ball_chestdrop_angle.GetInt(), m_aPlAng[YAW], 0);
	Vector dir;
	AngleVectors(ang, &dir);
	SetVel(m_vPlForwardVel2D + dir * sv_ball_chestdrop_strength.GetInt(), 0, BODY_PART_CHEST, false, true, false);

	return true;
}

bool CBall::DoHeader()
{
	Vector vel;

	if (m_pPl->IsNormalshooting())
	{
		vel = m_vPlForward * GetNormalshotStrength(GetPitchCoeff(), sv_ball_normalheader_strength.GetInt());
		EmitSound("Ball.kicknormal");
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HEADER_STATIONARY);
	}
	else if (m_vPlForwardVel2D.Length2D() >= mp_walkspeed.GetInt() && m_nInPenBoxOfTeam == m_pPl->GetOppTeamNumber() && (m_pPl->m_nButtons & IN_SPEED) && m_pPl->GetGroundEntity())
	{
		Vector forward;
		AngleVectors(QAngle(-5, m_aPlAng[YAW], 0), &forward, NULL, NULL);

		if (m_pPl->IsPowershooting())
			vel = forward * GetPowershotStrength(1.0f, sv_ball_powerdivingheader_strength.GetInt());
		else
			vel = forward * GetChargedshotStrength(1.0f, sv_ball_chargeddivingheader_minstrength.GetInt(), sv_ball_chargeddivingheader_maxstrength.GetInt());

		EmitSound("Ball.kickhard");
		//m_pPl->AddFlag(FL_FREECAM);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_DIVINGHEADER);
	}
	else
	{
		if (m_pPl->IsPowershooting())
			vel = m_vPlForward * GetPowershotStrength(GetPitchCoeff(), sv_ball_powerheader_strength.GetInt());
		else
			vel = m_vPlForward * GetChargedshotStrength(GetPitchCoeff(), sv_ball_chargedheader_minstrength.GetInt(), sv_ball_chargedheader_maxstrength.GetInt());

		EmitSound("Ball.kickhard");
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HEADER);
	}

	SetVel(m_vPlForwardVel2D + vel, 0, BODY_PART_HEAD, false, true, true);

	return true;
}

void CBall::SetSpin(float coeff)
{
	Vector sideRot(0, 0, 0);

	if (m_pPl->m_nButtons & IN_MOVELEFT) 
		sideRot = Vector(0, 0, 1);
	else if (m_pPl->m_nButtons & IN_MOVERIGHT) 
		sideRot = Vector(0, 0, -1);

	float sideSpin = m_vVel.Length() * sv_ball_spin.GetInt() * coeff / 100.0f;

	Vector backTopRot(0, 0, 0);

	float backTopSpin = m_vVel.Length() * sv_ball_spin.GetInt() * coeff / 100.0f;

	if (!sv_ball_jump_topspin_enabled.GetBool() || m_pPl->GetGroundEntity())
	{
		backTopRot = m_vPlRight;
		backTopSpin *= sv_ball_backspin_coeff.GetFloat();
	}
	else if (sv_ball_jump_topspin_enabled.GetBool() && !m_pPl->GetGroundEntity())
	{
		backTopRot = -m_vPlRight;
		backTopSpin *= sv_ball_topspin_coeff.GetFloat();
	}

	AngularImpulse randRot = AngularImpulse(0, 0, 0);
	for (int i = 0; i < 3; i++)
	{
		randRot[i] = sv_ball_defaultspin.GetInt() / 100.0f * (g_IOSRand.RandomInt(0, 1) == 1 ? 1 : -1);
	}

	SetRot(WorldToLocalRotation(SetupMatrixAngles(m_aAng), sideRot, sideSpin) + WorldToLocalRotation(SetupMatrixAngles(m_aAng), backTopRot, backTopSpin) + randRot);
}

void CBall::Think( void	)
{
	SetNextThink(gpGlobals->curtime + sv_ball_thinkinterval.GetFloat());

	State_Think();
}

void CBall::TriggerGoal(int team)
{
	if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
	{
		if (m_ePenaltyState == PENALTY_KICKED && team == m_nFoulingTeam)
		{
			m_ePenaltyState = PENALTY_SCORED;
			GetGlobalTeam(m_nFoulingTeam)->GetOppTeam()->AddGoal();
			SetMatchEvent(MATCH_EVENT_GOAL, m_pFouledPl->GetTeamNumber(), false);
			SetMatchEventPlayer(m_pFouledPl, false);
			m_bHasQueuedState = true;
		}

		return;
	}

	if (LastInfo(true) && LastInfo(true)->m_eBallState == BALL_THROWIN && !LastPl(false, LastPl(true)))
	{
		TriggerGoalLine(team);
		return;
	}

	if (LastTeam(true) != team && LastPl(true) && gpGlobals->curtime >= LastPl(true)->m_flLastShotOnGoal + 1)
	{
		LastPl(true)->SetShots(LastPl(true)->GetShots() + 1);
		LastPl(true)->SetShotsOnGoal(LastPl(true)->GetShotsOnGoal() + 1);
		LastPl(true)->m_flLastShotOnGoal = gpGlobals->curtime;
	}

	m_nTeam = team;
	State_Transition(BALL_GOAL, sv_ball_statetransition_activationdelay_normal.GetFloat());
}

void CBall::TriggerGoalLine(int team)
{
	//DevMsg("Trigger goal line\n");
	if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
		return;

	m_vTriggerTouchPos = GetPos();

	if (LastTeam(true) != team && LastPl(true))
	{
		LastPl(true)->SetShots(LastPl(true)->GetShots() + 1);
	}


	if (LastTeam(false) == team)
	{
		//SetMatchEvent(MATCH_EVENT_CORNER, NULL, LastOppTeam(false));
		State_Transition(BALL_CORNER, sv_ball_statetransition_activationdelay_normal.GetFloat());
	}
	else
	{
		//SetMatchEvent(MATCH_EVENT_THROWIN, NULL, LastOppTeam(false));
		State_Transition(BALL_GOALKICK, sv_ball_statetransition_activationdelay_normal.GetFloat());
	}
}

void CBall::TriggerSideline()
{
	if (SDKGameRules()->State_Get() == MATCH_PENALTIES)
		return;

	Vector ballPos = GetPos();

	CBaseEntity *pThrowIn = gEntList.FindEntityByClassnameNearest("info_throw_in", ballPos, 1000);
	if (!pThrowIn)
		return;

	m_vTriggerTouchPos = pThrowIn->GetLocalOrigin();
	//SetMatchEvent(MATCH_EVENT_THROWIN, NULL, LastOppTeam(false));
	State_Transition(BALL_THROWIN, sv_ball_statetransition_activationdelay_normal.GetFloat());
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
		m_vPlVel2D = Vector(m_vPlVel.x, m_vPlVel.y, 0);
		m_aPlAng = m_pPl->EyeAngles();
		AngleVectors(m_aPlAng, &m_vPlForward, &m_vPlRight, &m_vPlUp);
		m_vPlForward2D = m_vPlForward;
		m_vPlForward2D.z = 0;
		m_vPlForward2D.NormalizeInPlace();
		m_vPlForwardVel2D = m_vPlForward2D * max(0, (m_vPlVel2D.x * m_vPlForward2D.x + m_vPlVel2D.y * m_vPlForward2D.y));
		m_vPlDirToBall = m_vPos - m_vPlPos;
		VectorIRotate(m_vPlDirToBall, m_pPl->EntityToWorldTransform(), m_vPlLocalDirToBall);
	}
}

void CBall::MarkOffsidePlayers()
{
	if (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState || SDKGameRules()->State_Get() == MATCH_PENALTIES)
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

void CBall::UnmarkOffsidePlayers()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (CSDKPlayer::IsOnField(pPl))
			pPl->SetOffside(false);
	}
}

void CBall::Kicked(body_part_t bodyPart, bool isDeflection)
{
	float dynamicDelay = RemapValClamped(m_vVel.Length(), sv_ball_dynamicshotdelay_minshotstrength.GetInt(), sv_ball_dynamicshotdelay_maxshotstrength.GetInt(), sv_ball_dynamicshotdelay_mindelay.GetFloat(), sv_ball_dynamicshotdelay_maxdelay.GetFloat());
	
	float delay;

	if (State_Get() == BALL_NORMAL)
	{
		delay = sv_ball_dynamicshotdelay_enabled.GetBool() ? dynamicDelay : sv_ball_shotdelay_normal.GetFloat();
	}
	else
	{
		delay = sv_ball_shotdelay_setpiece.GetFloat();
	}
	
	m_pPl->m_flNextShot = gpGlobals->curtime + delay;
	m_flGlobalNextShot = gpGlobals->curtime + dynamicDelay * sv_ball_shotdelay_global_coeff.GetFloat(); //gpGlobals->curtime + sv_ball_shotdelay_global.GetFloat();
	Touched(m_pPl, !isDeflection, bodyPart);
}

void CBall::Touched(CSDKPlayer *pPl, bool isShot, body_part_t bodyPart)
{
	if (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState || SDKGameRules()->State_Get() == MATCH_PENALTIES)
		return;

	if (m_Touches.Count() > 0 && m_Touches.Tail()->m_pPl == pPl && m_Touches.Tail()->m_nTeam == pPl->GetTeamNumber()
		&& sv_ball_doubletouchfouls.GetBool() && State_Get() == BALL_NORMAL && m_Touches.Tail()->m_eBallState != BALL_NORMAL
		&& m_Touches.Tail()->m_eBallState != BALL_KEEPERHANDS && pPl->GetTeam()->GetNumPlayers() > 2 && pPl->GetOppTeam()->GetNumPlayers() > 2)
	{
		TriggerFoul(FOUL_DOUBLETOUCH, pPl->GetLocalOrigin(), pPl);
		State_Transition(BALL_FREEKICK, sv_ball_statetransition_activationdelay_long.GetFloat());
		return;
	}
	else
	{
		BallTouchInfo *pInfo = LastInfo(true);
		if (pInfo && CSDKPlayer::IsOnField(pInfo->m_pPl) && pInfo->m_pPl != pPl && (m_vPos - pInfo->m_vBallPos).Length2D() >= sv_ball_stats_pass_mindist.GetInt())
		{ 
			pInfo->m_pPl->SetPasses(pInfo->m_pPl->GetPasses() + 1);

			if (pInfo->m_nTeam == pPl->GetTeamNumber())
			{
				pInfo->m_pPl->SetPassesCompleted(pInfo->m_pPl->GetPassesCompleted() + 1);
				SetMatchEvent(MATCH_EVENT_PASS, pPl->GetTeamNumber(), false);
			}
			else
			{
				if (pPl->GetTeamPosType() == GK && bodyPart == BODY_PART_HANDS)
				{
					pPl->SetKeeperSaves(pPl->GetKeeperSaves() + 1);
					SetMatchEvent(MATCH_EVENT_KEEPERSAVE, pPl->GetTeamNumber(), false);
				}
				else
				{
					pPl->SetInterceptions(pPl->GetInterceptions() + 1);
					SetMatchEvent(MATCH_EVENT_INTERCEPTION, pPl->GetTeamNumber(), false);
				}
			}

			SetMatchEventPlayer(pPl, false);
		}

		UpdatePossession(pPl);
		BallTouchInfo *info = new BallTouchInfo;
		info->m_pPl = pPl;
		info->m_nTeam = pPl->GetTeamNumber();
		info->m_bIsShot = isShot;
		info->m_eBodyPart = bodyPart;
		info->m_eBallState = State_Get();
		info->m_vBallPos = m_vPos;
		info->m_vBallVel = m_vVel;
		info->m_flTime = gpGlobals->curtime;
		m_Touches.AddToTail(info);
	}

	//DevMsg("touches: %d\n", m_Touches.Count());
	
	if (pPl->IsOffside())
	{
		pPl->SetOffsides(pPl->GetOffsides() + 1);
		TriggerFoul(FOUL_OFFSIDE, pPl->GetOffsidePos(), pPl);
		SDKGameRules()->SetOffsideLinePositions(pPl->GetOffsideBallPos().y, pPl->GetOffsidePos().y, pPl->GetOffsideLastOppPlayerPos().y);
		State_Transition(BALL_FREEKICK, sv_ball_statetransition_activationdelay_long.GetFloat());
	}
}

void CBall::RemoveAllTouches()
{
	if (!m_bHasQueuedState)
		m_Touches.PurgeAndDeleteElements();
}

BallTouchInfo *CBall::LastInfo(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/, CSDKPlayer *pSkipPl2 /*= NULL*/)
{
	for (int i = m_Touches.Count() - 1; i >= 0; i--)
	{
		if (pSkipPl && m_Touches[i]->m_pPl == pSkipPl)
			continue;

		if (pSkipPl2 && m_Touches[i]->m_pPl == pSkipPl2)
			continue;

		if (!wasShooting || m_Touches[i]->m_bIsShot)
			return m_Touches[i];
	}

	return NULL;
}

CSDKPlayer *CBall::LastPl(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/, CSDKPlayer *pSkipPl2 /*= NULL*/)
{
	BallTouchInfo *info = LastInfo(wasShooting, pSkipPl, pSkipPl2);
	if (info && CSDKPlayer::IsOnField(info->m_pPl))
		return info->m_pPl;
	
	return NULL;
}

int CBall::LastTeam(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/, CSDKPlayer *pSkipPl2 /*= NULL*/)
{
	BallTouchInfo *info = LastInfo(wasShooting, pSkipPl, pSkipPl2);
	return info ? info->m_nTeam : TEAM_INVALID;
}

int CBall::LastOppTeam(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/, CSDKPlayer *pSkipPl2 /*= NULL*/)
{
	BallTouchInfo *info = LastInfo(wasShooting, pSkipPl, pSkipPl2);
	return info ? (info->m_nTeam == TEAM_A ? TEAM_B : TEAM_A) : TEAM_INVALID;
}

void CBall::UpdatePossession(CSDKPlayer *pNewPossessor)
{
	if (m_pPossessingPl == pNewPossessor)
		return;

	if (m_flPossessionStart != -1)
	{
		float duration = gpGlobals->curtime - m_flPossessionStart;

		GetGlobalTeam(TEAM_A)->m_flPossessionTime = 0;
		GetGlobalTeam(TEAM_B)->m_flPossessionTime = 0;

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

			if (!CSDKPlayer::IsOnField(pPl))
				continue;

			if (pPl == m_pPossessingPl)
				pPl->SetPossessionTime(pPl->GetPossessionTime() + duration);

			pPl->GetTeam()->m_flPossessionTime += pPl->GetPossessionTime();
		}

		float total = GetGlobalTeam(TEAM_A)->m_flPossessionTime + GetGlobalTeam(TEAM_B)->m_flPossessionTime;

		if (total == 0)
			return;

		int possSum = 0;

		float possRemainders[22][2];
		
		int possCount = 0;

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

			if (!CSDKPlayer::IsOnField(pPl))
				continue;

			float poss = 100 * pPl->GetPossessionTime() / total;
			pPl->SetPossession((int)poss);
			float remainder = poss - pPl->GetPossession();

			possSum += pPl->GetPossession();

			if (pPl->GetPossessionTime() == 0.0f)
				continue;

			for (int j = 0; j < 22; j++)
			{
				if (j == possCount)
				{
				}
				else if (possRemainders[j][0] >= remainder)
				{
					continue;
				}
				else if (possRemainders[j][0] < remainder)
				{
					for (int k = 22 - 1; k > j; k--)
					{
						possRemainders[k][0] = possRemainders[k - 1][0];
						possRemainders[k][1] = possRemainders[k - 1][1];
					}
				}

				possRemainders[j][0] = remainder;
				possRemainders[j][1] = i;
				possCount += 1;
				break;
			}
		}

		if (possCount > 0)
		{
			int remainder = 100 - possSum;

			while (remainder > 0)
			{
				for (int i = 0; i < possCount; i++)
				{
					ToSDKPlayer(UTIL_PlayerByIndex(possRemainders[i][1]))->SetPossession(ToSDKPlayer(UTIL_PlayerByIndex(possRemainders[i][1]))->GetPossession() + 1);
					remainder -= 1;

					if (remainder == 0)
						break;
				}
			}
		}

		GetGlobalTeam(TEAM_A)->m_nPossession = 0;
		GetGlobalTeam(TEAM_B)->m_nPossession = 0;

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

			if (!CSDKPlayer::IsOnField(pPl))
				continue;

			pPl->GetTeam()->m_nPossession += pPl->GetPossession();
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

int CBall::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CBall::ResetMatch()
{
	CreateVPhysics();
	m_pPl = NULL;
	m_pOtherPl = NULL;
	m_pMatchEventPlayer = NULL;
	m_pMatchSubEventPlayer = NULL;
	m_pMatchSubSubEventPlayer = NULL;
	m_eMatchEvent = MATCH_EVENT_NONE;
	m_eMatchSubEvent = MATCH_EVENT_NONE;
	m_eMatchSubSubEvent = MATCH_EVENT_NONE;
	RemoveAllTouches();
	m_ePenaltyState = PENALTY_NONE;
	SDKGameRules()->SetOffsideLinesEnabled(false);
	SDKGameRules()->DisableShield();
	UnmarkOffsidePlayers();
	m_bSetNewPos = false;
	m_bSetNewVel = false;
	m_bSetNewRot = false;
	m_bHasQueuedState = false;
	RemoveEffects(EF_NODRAW);
	EnablePlayerCollisions(true);
	m_pPhys->EnableMotion(true);
	m_pPhys->Wake();
	m_pHoldingPlayer = NULL;
	m_pPossessingPl = NULL;
	m_nPossessingTeam = TEAM_INVALID;
	m_flPossessionStart = -1;
	m_flLastMatchEventSetTime = -1;

	GetGlobalTeam(TEAM_A)->ResetStats();
	GetGlobalTeam(TEAM_B)->ResetStats();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		pPl->ResetFlags();
		pPl->ResetStats();
	}

	CPlayerPersistentData::ReallocateAllPlayerData();

	if (ReplayManager())
		ReplayManager()->CleanUp();
}

void CBall::SetPenaltyTaker(CSDKPlayer *pPl)
{
	m_pFouledPl = pPl;
	m_nFoulingTeam = pPl->GetOppTeamNumber();
}

void CBall::EnablePlayerCollisions(bool enable)
{
	SetCollisionGroup(enable ? COLLISION_GROUP_SOLID_BALL : COLLISION_GROUP_NONSOLID_BALL);
}

void CBall::RemoveFromPlayerHands(CSDKPlayer *pPl)
{
	if (CSDKPlayer::IsOnField(pPl) && pPl->GetTeamPosType() == GK && pPl->m_nBody == MODEL_KEEPER_AND_BALL)
	{
		pPl->m_pHoldingBall = NULL;
		pPl->m_nBody = MODEL_KEEPER;
		pPl->DoServerAnimationEvent(PLAYERANIMEVENT_CARRY_END);
	}

	m_pHoldingPlayer = NULL;
	RemoveEffects(EF_NODRAW);
	EnablePlayerCollisions(true);
	m_pPhys->EnableMotion(true);
	m_pPhys->Wake();
}

Vector CBall::GetPos()
{
	if (m_bSetNewPos)
		return m_vPos;
	else
	{
		Vector pos;
		m_pPhys->GetPosition(&pos, NULL);
		return pos;
	}
}

Vector CBall::GetVel()
{
	if (m_bSetNewVel)
		return m_vVel;
	else
	{
		Vector vel;
		m_pPhys->GetVelocity(&vel, NULL);
		return vel;
	}
}

AngularImpulse CBall::GetRot()
{
	if (m_bSetNewRot)
		return m_vRot;
	else
	{
		AngularImpulse rot;
		m_pPhys->GetVelocity(NULL, &rot);
		return rot;
	}
}

float CBall::CalcFieldZone()
{
	Vector pos = GetPos();
	float fieldLength = SDKGameRules()->m_vFieldMax.GetY() - SDKGameRules()->m_vFieldMin.GetY();
	float dist = pos.y - SDKGameRules()->m_vKickOff.GetY();
	return clamp(dist * 100 / (fieldLength / 2), -100, 100);
}