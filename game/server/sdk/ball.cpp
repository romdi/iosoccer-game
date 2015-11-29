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


ConVar
	sv_ball_mass( "sv_ball_mass", "5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY ),
	sv_ball_damping( "sv_ball_damping", "0.01", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY ),
	sv_ball_rotdamping( "sv_ball_rotdamping", "0.7", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY ),
	sv_ball_rotinertialimit( "sv_ball_rotinertialimit", "1.5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY ),
	sv_ball_dragcoeff( "sv_ball_dragcoeff", "1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY ),
	sv_ball_inertia( "sv_ball_inertia", "1.5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY ),
	sv_ball_drag_enabled("sv_ball_drag_enabled", "1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY ),
	
	sv_ball_spin( "sv_ball_spin", "4000", FCVAR_NOTIFY ),
	sv_ball_spin_mincoeff( "sv_ball_spin_mincoeff", "0.0", FCVAR_NOTIFY ),
	sv_ball_defaultspin( "sv_ball_defaultspin", "150", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY ),
	sv_ball_topspin_coeff( "sv_ball_topspin_coeff", "1.25", FCVAR_NOTIFY ),
	sv_ball_backspin_coeff( "sv_ball_backspin_coeff", "0.1", FCVAR_NOTIFY ),
	sv_ball_magnus_coeff("sv_ball_magnus_coeff", "200", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	
	sv_ball_deflectionradius( "sv_ball_deflectionradius", "30", FCVAR_NOTIFY ),
	sv_ball_collisionradius( "sv_ball_collisionradius", "15", FCVAR_NOTIFY ),
	
	sv_ball_standing_reach( "sv_ball_standing_reach", "40", FCVAR_NOTIFY ),
	
	sv_ball_slidesidereach_ball( "sv_ball_slidesidereach_ball", "25", FCVAR_NOTIFY ),
	sv_ball_slideforwardreach_ball( "sv_ball_slideforwardreach_ball", "40", FCVAR_NOTIFY ),
	sv_ball_slidebackwardreach_ball( "sv_ball_slidebackwardreach_ball", "20", FCVAR_NOTIFY ),
	
	sv_ball_slidezstart("sv_ball_slidezstart", "-50", FCVAR_NOTIFY), 
	sv_ball_slidezend("sv_ball_slidezend", "30", FCVAR_NOTIFY), 
	
	sv_ball_keeper_standing_reach( "sv_ball_keeper_standing_reach", "40", FCVAR_NOTIFY ),
	sv_ball_keeper_standing_reach_walking( "sv_ball_keeper_standing_reach_walking", "50", FCVAR_NOTIFY ),
	
	sv_ball_keeper_forwarddive_shortsidereach( "sv_ball_keeper_forwarddive_shortsidereach", "50", FCVAR_NOTIFY ),
	sv_ball_keeper_forwarddive_longsidereach( "sv_ball_keeper_forwarddive_longsidereach", "80", FCVAR_NOTIFY ),
	sv_ball_keeper_forwarddive_longsidereach_opposite( "sv_ball_keeper_forwarddive_longsidereach_opposite", "50", FCVAR_NOTIFY ),
	sv_ball_keeper_forwarddive_zstart( "sv_ball_keeper_forwarddive_zstart", "-50", FCVAR_NOTIFY ),
	sv_ball_keeper_forwarddive_zend( "sv_ball_keeper_forwarddive_zend", "90", FCVAR_NOTIFY ),
	sv_ball_keeper_forwarddive_catchcoeff( "sv_ball_keeper_forwarddive_catchcoeff", "0.5", FCVAR_NOTIFY ),

	sv_ball_keeper_backwarddive_shortsidereach( "sv_ball_keeper_backwarddive_shortsidereach", "50", FCVAR_NOTIFY ),
	sv_ball_keeper_backwarddive_longsidereach( "sv_ball_keeper_backwarddive_longsidereach", "100", FCVAR_NOTIFY ),
	sv_ball_keeper_backwarddive_longsidereach_opposite( "sv_ball_keeper_backwarddive_longsidereach_opposite", "0", FCVAR_NOTIFY ),
	sv_ball_keeper_backwarddive_zstart( "sv_ball_keeper_backwarddive_zstart", "50", FCVAR_NOTIFY ),
	sv_ball_keeper_backwarddive_zend( "sv_ball_keeper_backwarddive_zend", "100", FCVAR_NOTIFY ),
	sv_ball_keeper_backwarddive_catchcoeff( "sv_ball_keeper_backwarddive_catchcoeff", "0.5", FCVAR_NOTIFY ),
	sv_ball_keeper_backwarddive_punchupangle( "sv_ball_keeper_backwarddive_punchupangle", "45", FCVAR_NOTIFY ),
	
	sv_ball_keeper_sidedive_shortsidereach( "sv_ball_keeper_sidedive_shortsidereach", "50", FCVAR_NOTIFY ),
	sv_ball_keeper_sidedive_longsidereach( "sv_ball_keeper_sidedive_longsidereach", "60", FCVAR_NOTIFY ),
	sv_ball_keeper_sidedive_longsidereach_opposite( "sv_ball_keeper_sidedive_longsidereach_opposite", "50", FCVAR_NOTIFY ),
	sv_ball_keeper_sidedive_zstart( "sv_ball_keeper_sidedive_zstart", "-30", FCVAR_NOTIFY ),
	sv_ball_keeper_sidedive_zend( "sv_ball_keeper_sidedive_zend", "70", FCVAR_NOTIFY ),
	sv_ball_keeper_sidedive_catchcenteroffset_side( "sv_ball_keeper_sidedive_catchcenteroffset_side", "0", FCVAR_NOTIFY ),
	sv_ball_keeper_sidedive_catchcenteroffset_z( "sv_ball_keeper_sidedive_catchcenteroffset_z", "40", FCVAR_NOTIFY ),
	
	sv_ball_keeper_punch_minstrength("sv_ball_keeper_punch_minstrength", "900", FCVAR_NOTIFY),
	sv_ball_keeper_punch_sidespeedcoeff("sv_ball_keeper_punch_sidespeedcoeff", "0.1", FCVAR_NOTIFY),
	
	sv_ball_keeperdeflectioncoeff("sv_ball_keeperdeflectioncoeff", "0.5", FCVAR_NOTIFY),
	
	sv_ball_shotdelay_global_coeff("sv_ball_shotdelay_global_coeff", "0.33", FCVAR_NOTIFY),
	sv_ball_keepercatchdelay_sidedive_global_coeff("sv_ball_keepercatchdelay_sidedive_global_coeff", "1.0", FCVAR_NOTIFY),
	sv_ball_keepercatchdelay_forwarddive_global_coeff("sv_ball_keepercatchdelay_forwarddive_global_coeff", "0.75", FCVAR_NOTIFY),
	sv_ball_keepercatchdelay_backwarddive_global_coeff("sv_ball_keepercatchdelay_backwarddive_global_coeff", "1.0", FCVAR_NOTIFY),
	sv_ball_keepercatchdelay_standing_global_coeff("sv_ball_keepercatchdelay_standing_global_coeff", "0.5", FCVAR_NOTIFY),
	sv_ball_dynamicshotdelay_mindelay("sv_ball_dynamicshotdelay_mindelay", "0.2", FCVAR_NOTIFY),
	sv_ball_dynamicshotdelay_maxdelay("sv_ball_dynamicshotdelay_maxdelay", "1.0", FCVAR_NOTIFY),
	sv_ball_dynamicshotdelay_minshotstrength("sv_ball_dynamicshotdelay_minshotstrength", "360", FCVAR_NOTIFY),
	sv_ball_dynamicshotdelay_maxshotstrength("sv_ball_dynamicshotdelay_maxshotstrength", "1440", FCVAR_NOTIFY),

	sv_ball_shottaker_mindelay_short("sv_ball_shottaker_mindelay_short", "0.5", FCVAR_NOTIFY),
	sv_ball_shottaker_mindelay_long("sv_ball_shottaker_mindelay_long", "1.0", FCVAR_NOTIFY),
	
	sv_ball_bestshotangle("sv_ball_bestshotangle", "-20", FCVAR_NOTIFY),
	
	sv_ball_pitchdown_exponent("sv_ball_pitchdown_exponent", "3.0", FCVAR_NOTIFY),
	sv_ball_pitchdown_fixedcoeff("sv_ball_pitchdown_fixedcoeff", "0.3", FCVAR_NOTIFY),
	sv_ball_pitchup_exponent("sv_ball_pitchup_exponent", "3.0", FCVAR_NOTIFY),
	sv_ball_pitchup_fixedcoeff("sv_ball_pitchup_fixedcoeff", "0.3", FCVAR_NOTIFY),

	sv_ball_normalshot_strength("sv_ball_normalshot_strength", "810", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_chargedshot_minstrength("sv_ball_chargedshot_minstrength", "810", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_chargedshot_maxstrength("sv_ball_chargedshot_maxstrength", "1440", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	
	sv_ball_normalheader_strength("sv_ball_normalheader_strength", "450", FCVAR_NOTIFY), 
	sv_ball_chargedheader_minstrength("sv_ball_chargedheader_minstrength", "450", FCVAR_NOTIFY), 
	sv_ball_chargedheader_maxstrength("sv_ball_chargedheader_maxstrength", "900", FCVAR_NOTIFY), 
	
	sv_ball_divingheader_minstrength("sv_ball_divingheader_minstrength", "750", FCVAR_NOTIFY), 
	sv_ball_divingheader_maxstrength("sv_ball_divingheader_maxstrength", "1050", FCVAR_NOTIFY),
	sv_ball_divingheader_minangle("sv_ball_divingheader_minangle", "30", FCVAR_NOTIFY), 
	sv_ball_divingheader_maxangle("sv_ball_divingheader_maxangle", "-30", FCVAR_NOTIFY),

	sv_ball_bicycleshot_minstrength("sv_ball_bicycleshot_minstrength", "750", FCVAR_NOTIFY), 
	sv_ball_bicycleshot_maxstrength("sv_ball_bicycleshot_maxstrength", "1050", FCVAR_NOTIFY),

	sv_ball_slide_strength("sv_ball_slide_strength", "720", FCVAR_NOTIFY), 
	sv_ball_slide_pitchangle("sv_ball_slide_pitchangle", "-15", FCVAR_NOTIFY), 
	sv_ball_slide_playerspeedcoeff("sv_ball_slide_playerspeedcoeff", "1.0", FCVAR_NOTIFY),
	
	sv_ball_keepershot_minangle("sv_ball_keepershot_minangle", "20", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),

	sv_ball_keeperthrow_strength("sv_ball_keeperthrow_strength", "100", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	
	sv_ball_groundshot_minangle("sv_ball_groundshot_minangle", "-7", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),

	sv_ball_volleyshot_minangle("sv_ball_volleyshot_minangle", "60", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_volleyshot_sidespincoeff("sv_ball_volleyshot_sidespincoeff", "1.5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_volleyshot_topspincoeff("sv_ball_volleyshot_topspincoeff", "1.0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_volleyshot_backspincoeff("sv_ball_volleyshot_backspincoeff", "1.0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),

	sv_ball_rainbowflick_fixedangle("sv_ball_rainbowflick_fixedangle", "-50", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_rainbowflick_dynamicangle("sv_ball_rainbowflick_dynamicangle", "-30", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_rainbowflick_strength("sv_ball_rainbowflick_strength", "475", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_rainbowflick_spincoeff("sv_ball_rainbowflick_spincoeff", "1.0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),

	sv_ball_lift_angle("sv_ball_lift_angle", "-90", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_lift_fixedstrength("sv_ball_lift_fixedstrength", "100", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_lift_variablestrength("sv_ball_lift_variablestrength", "300", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),

	sv_ball_roll_strength("sv_ball_roll_strength", "225", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),

	sv_ball_chipshot_angle("sv_ball_chipshot_angle", "-35", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_chipshot_fixedstrength("sv_ball_chipshot_fixedstrength", "125", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_chipshot_dynamicstrength("sv_ball_chipshot_dynamicstrength", "500", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_chipshot_spincoeff("sv_ball_chipshot_spincoeff", "1.75", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),

	sv_ball_finesseshot_spincoeff("sv_ball_finesseshot_spincoeff", "1.33", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_finesseshot_strength("sv_ball_finesseshot_strength", "900", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),

	sv_ball_header_spincoeff("sv_ball_header_spincoeff", "0.5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_header_minangle("sv_ball_header_minangle", "70", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_header_maxangle("sv_ball_header_maxangle", "-40", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_header_playerspeedcoeff("sv_ball_header_playerspeedcoeff", "1.0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),

	sv_ball_chestdrop_strength("sv_ball_chestdrop_strength", "100", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_chestdrop_playerspeedcoeff("sv_ball_chestdrop_playerspeedcoeff", "1.0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_chestdrop_angle("sv_ball_chestdrop_angle", "90", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),

	sv_ball_animation_minstrength_strongshot("sv_ball_animation_minstrength_strongshot", "800", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_animation_minstrength_weakshot("sv_ball_animation_minstrength_weakshot", "600", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	sv_ball_animation_minstrength_dribbleshot("sv_ball_animation_minstrength_dribbleshot", "400", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY),
	
	sv_ball_highlightsdelay_intermissions("sv_ball_highlightsdelay_intermissions", "5.0", FCVAR_NOTIFY),
	sv_ball_highlightsdelay_cooldown("sv_ball_highlightsdelay_cooldown", "30.0", FCVAR_NOTIFY),
	
	sv_ball_bodypos_feet_start("sv_ball_bodypos_feet_start", "-15", FCVAR_NOTIFY),
	sv_ball_bodypos_hip_start("sv_ball_bodypos_hip_start", "20", FCVAR_NOTIFY),
	sv_ball_bodypos_chest_start("sv_ball_bodypos_chest_start", "45", FCVAR_NOTIFY),
	sv_ball_bodypos_head_start("sv_ball_bodypos_head_start", "60", FCVAR_NOTIFY),
	sv_ball_bodypos_head_end("sv_ball_bodypos_head_end", "85", FCVAR_NOTIFY),
	sv_ball_bodypos_keeperarms_end("sv_ball_bodypos_keeperarms_end", "100", FCVAR_NOTIFY),
	
	sv_ball_bodypos_keeperhands("sv_ball_bodypos_keeperhands", "40", FCVAR_NOTIFY),
	
	sv_ball_bodypos_collision_start("sv_ball_bodypos_collision_start", "15", FCVAR_NOTIFY),
	sv_ball_bodypos_collision_end("sv_ball_bodypos_collision_end", "75", FCVAR_NOTIFY),
	
	sv_ball_bodypos_deflection_start("sv_ball_bodypos_deflection_start", "0", FCVAR_NOTIFY),
	sv_ball_bodypos_deflection_end("sv_ball_bodypos_deflection_end", "80", FCVAR_NOTIFY),
	
	sv_ball_maxplayerfinddist("sv_ball_maxplayerfinddist", "200", FCVAR_NOTIFY),
	
	sv_ball_freecamshot_maxangle("sv_ball_freecamshot_maxangle", "60", FCVAR_NOTIFY),
	sv_ball_heelshot_pitch("sv_ball_heelshot_pitch", "-5", FCVAR_NOTIFY),
	sv_ball_heelshot_fixedstrength("sv_ball_heelshot_fixedstrength", "150", FCVAR_NOTIFY),
	sv_ball_heelshot_variablestrength("sv_ball_heelshot_variablestrength", "400", FCVAR_NOTIFY),
	
	sv_ball_keepercatchdelay_poscoeffmin("sv_ball_keepercatchdelay_poscoeffmin", "0.5", FCVAR_NOTIFY),

	sv_ball_collision_passive_mass("sv_ball_collision_passive_mass", "5", FCVAR_NOTIFY),
	sv_ball_collision_passive_coeff("sv_ball_collision_passive_coeff", "0.5", FCVAR_NOTIFY),

	sv_ball_collision_deflection_mass("sv_ball_collision_deflection_mass", "5", FCVAR_NOTIFY),
	sv_ball_collision_deflection_coeff("sv_ball_collision_deflection_coeff", "0.5", FCVAR_NOTIFY),
	
	sv_ball_collision_dribbling_mass("sv_ball_collision_dribbling_mass", "75", FCVAR_NOTIFY),
	sv_ball_collision_dribbling_coeff("sv_ball_collision_dribbling_coeff", "1.0", FCVAR_NOTIFY),

	sv_ball_collision_selfhit_mass("sv_ball_collision_selfhit_mass", "225", FCVAR_NOTIFY),
	sv_ball_collision_selfhit_coeff("sv_ball_collision_selfhit_coeff", "1.0", FCVAR_NOTIFY),

	sv_ball_sandground_enabled("sv_ball_sandground_enabled", "0", FCVAR_NOTIFY),
	sv_ball_sandground_maxshotstrength("sv_ball_sandground_maxshotstrength", "500", FCVAR_NOTIFY),
	sv_ball_sandground_rollfriction("sv_ball_sandground_rollfriction", "30", FCVAR_NOTIFY),
	sv_ball_sandground_bouncefriction("sv_ball_sandground_bouncefriction", "30", FCVAR_NOTIFY);

	

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
	SendPropEHandle(SENDINFO(m_pHoldingPlayer)),
	SendPropInt(SENDINFO(m_eBallState)),
	SendPropString(SENDINFO(m_szSkinName)),

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
	m_eNextState = BALL_STATE_NONE;
	m_pPl = NULL;
	m_pHoldingPlayer = NULL;
	m_bHasQueuedState = false;
	m_bHitThePost = false;
	m_nInPenBoxOfTeam = TEAM_NONE;
	memset(m_szSkinName.GetForModify(), 0, sizeof(m_szSkinName));
}

CBall::~CBall()
{
}

bool CBall::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	return false;
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

	PrecacheModel(BALL_MODEL);
	SetModel(BALL_MODEL);

	CreateVPhysics();

	SetThink(&CBall::Think);
	SetNextThink(gpGlobals->curtime);

	m_nBody = 0;

	int ballSkinIndex = g_IOSRand.RandomInt(0, CBallInfo::m_BallInfo.Count() - 1);
	Q_strncpy(m_szSkinName.GetForModify(), CBallInfo::m_BallInfo[ballSkinIndex]->m_szFolderName, sizeof(m_szSkinName));

	m_pPhys->SetPosition(GetLocalOrigin(), GetLocalAngles(), true);
	m_pPhys->SetVelocityInstantaneous(&vec3_origin, &vec3_origin);

	PrecacheScriptSound("Ball.Kicknormal");
	PrecacheScriptSound("Ball.Kickhard");
	PrecacheScriptSound("Ball.Touch");
	PrecacheScriptSound("Ball.Post");
	PrecacheScriptSound("Ball.Net");
	PrecacheScriptSound("Ball.Whistle");

	State_Transition(BALL_STATE_NORMAL);
}

bool CBall::CreateVPhysics()
{	
	if (m_pPhys)
	{
		VPhysicsDestroyObject();
		m_pPhys = NULL;
	}

	objectparams_t params =	g_IOSPhysDefaultObjectParams;
	params.pGameData = static_cast<void	*>(this);
	params.damping = sv_ball_damping.GetFloat();
	params.mass = sv_ball_mass.GetFloat();
	params.dragCoefficient = sv_ball_dragcoeff.GetFloat();
	params.inertia = sv_ball_inertia.GetFloat();
	params.rotdamping = sv_ball_rotdamping.GetFloat();
	params.rotInertiaLimit = sv_ball_rotinertialimit.GetFloat();
	int	nMaterialIndex = physprops->GetSurfaceIndex(mp_weather.GetInt() == 0 ? "dryball" : (mp_weather.GetInt() == 1 ? "wetball" : "icyball"));
	m_pPhys = physenv->CreateSphereObject( BALL_PHYS_RADIUS, nMaterialIndex, GetAbsOrigin(), GetAbsAngles(), &params, false );
	if (!m_pPhys)
		return false;

	VPhysicsSetObject( m_pPhys );
	
	SetSolid( SOLID_VPHYSICS );
	SetSolidFlags( FSOLID_NOT_STANDABLE	);
	UTIL_SetSize(this, -Vector(BALL_PHYS_RADIUS, BALL_PHYS_RADIUS, BALL_PHYS_RADIUS), Vector(BALL_PHYS_RADIUS, BALL_PHYS_RADIUS, BALL_PHYS_RADIUS));

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
		vel += magnusDir * 1e-6 * sv_ball_magnus_coeff.GetFloat() * gpGlobals->frametime;

	if (sv_ball_sandground_enabled.GetBool())
	{
		if (vel.z <= 0 && m_pPhys->GetContactPoint(NULL, NULL))
		{
			vel -= sv_ball_sandground_rollfriction.GetFloat() * vel * gpGlobals->frametime;
			angImp -= sv_ball_sandground_rollfriction.GetFloat() * angImp * gpGlobals->frametime;
		}
	}

	m_pPhys->SetVelocity(&vel, &angImp);

	BaseClass::VPhysicsUpdate(pPhysics);
}

void CBall::VPhysicsCollision(int index, gamevcollisionevent_t *pEvent)
{
	float speed = pEvent->collisionSpeed;
	int surfaceProps = pEvent->surfaceProps[!index];

	if (surfaceProps == NET_SURFACEPROPS && speed > 300.0f)
		EmitSound("Ball.Net");
	else if (surfaceProps == POST_SURFACEPROPS && speed > 300.0f)
		EmitSound("Ball.Post");
	else if (speed > 500.0f)
		EmitSound("Ball.Touch");

	if (sv_ball_sandground_enabled.GetBool())
	{
		if (surfaceProps == 0)
		{
			Vector vel;
			AngularImpulse angImp;
			m_pPhys->GetVelocity(&vel, &angImp);

			vel -= sv_ball_sandground_bouncefriction.GetFloat() * vel * gpGlobals->frametime;
			angImp -= sv_ball_sandground_bouncefriction.GetFloat() * angImp * gpGlobals->frametime;
			m_pPhys->SetVelocity(&vel, &angImp);
		}
	}
}

CSDKPlayer *CBall::FindNearestPlayer(int team /*= TEAM_NONE*/, int posFlags /*= FL_POS_OUTFIELD*/, bool checkIfShooting /*= false*/, int ignoredPlayerBits /*= 0*/, float radius /*= -1*/)
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
			int posType = (int)pPlayer->GetTeam()->GetFormation()->positions[pPlayer->GetTeamPosIndex()]->type;

			if ((posFlags & FL_POS_OUTFIELD) != 0 && !((1 << posType) & (g_nPosDefense + g_nPosMidfield + g_nPosAttack)))
				continue;

			if ((posFlags & FL_POS_KEEPER) != 0 && !((1 << posType) & g_nPosKeeper))
				continue;

			if ((posFlags & FL_POS_DEFENDER) != 0 && !((1 << posType) & g_nPosDefense))
				continue;

			if ((posFlags & FL_POS_MIDFIELDER) != 0 && !((1 << posType) & g_nPosMidfield))
				continue;

			if ((posFlags & FL_POS_ATTACKER) != 0 && !((1 << posType) & g_nPosAttack))
				continue;

			if ((posFlags & FL_POS_LEFT) != 0 && !((1 << posType) & g_nPosLeft))
				continue;

			if ((posFlags & FL_POS_CENTER) != 0 && !((1 << posType) & g_nPosCenter))
				continue;

			if ((posFlags & FL_POS_RIGHT) != 0 && !((1 << posType) & g_nPosRight))
				continue;
		}

		if (team != TEAM_NONE && pPlayer->GetTeamNumber() != team)
			continue;

		if (checkIfShooting && (!pPlayer->IsShooting() || !pPlayer->CanShoot()) && !pPlayer->IsKeeperDiving())
			continue;

		if (radius != -1 && (pPlayer->GetLocalOrigin() - m_vPos).Length2DSqr() > Sqr(radius))
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
	//if (!pNearest && team != TEAM_NONE)
	//	pNearest = FindNearestPlayer();

	return pNearest;
}

void CBall::SetPos(const Vector &pos, bool addBallRadiusZOffset/* = true*/, bool freeze/* = true*/)
{
	m_vPos = pos;

	if (addBallRadiusZOffset)
		m_vPos.z += BALL_PHYS_RADIUS;

	m_vVel = vec3_origin;
	m_vRot = vec3_origin;
	m_pPhys->EnableMotion(true);
	m_pPhys->Wake();
	m_pPhys->SetVelocityInstantaneous(&vec3_origin, &vec3_origin);
	m_pPhys->SetPosition(m_vPos, m_aAng, true);

	if (freeze)
		m_pPhys->EnableMotion(false);
}

void CBall::SetAng(const QAngle &ang)
{
	m_aAng = ang;
	m_pPhys->SetPosition(m_vPos, m_aAng, false);
}

void CBall::SetVel(Vector vel, float spinCoeff, int spinFlags, body_part_t bodyPart, bool markOffsidePlayers, float shotTakerMinDelay, bool resetShotCharging)
{
	Vector oldVel = m_vVel;

	m_vVel = vel;

	if (sv_ball_sandground_enabled.GetBool())
	{
		if (m_pPhys->GetContactPoint(NULL, NULL))
		{
			float speed = VectorNormalize(m_vVel);
			m_vVel *= min(speed, sv_ball_sandground_maxshotstrength.GetInt());
		}
	}

	m_pPhys->EnableMotion(true);
	m_pPhys->Wake();
	m_pPhys->SetVelocity(&m_vVel, &m_vRot);

	if (spinCoeff != -1)
		SetRot(CalcSpin(spinCoeff, spinFlags));

	float dynamicDelay = RemapValClamped(m_vVel.Length(), sv_ball_dynamicshotdelay_minshotstrength.GetInt(), sv_ball_dynamicshotdelay_maxshotstrength.GetInt(), sv_ball_dynamicshotdelay_mindelay.GetFloat(), sv_ball_dynamicshotdelay_maxdelay.GetFloat());
	
	m_flGlobalLastShot = gpGlobals->curtime;
	m_flGlobalDynamicShotDelay = dynamicDelay;

	m_pPl->m_flNextShot = gpGlobals->curtime + max(dynamicDelay, shotTakerMinDelay);

	if (resetShotCharging)
		m_pPl->ResetShotCharging();

	Touched(true, bodyPart, oldVel);
}

void CBall::SetRot(AngularImpulse rot)
{
	m_vRot = rot;
	m_pPhys->EnableMotion(true);
	m_pPhys->Wake();
	m_pPhys->SetVelocity(&m_vVel, &m_vRot);
}

CBallStateInfo *CBall::State_LookupInfo(ball_state_t state)
{
	static CBallStateInfo ballStateInfos[] =
	{
		{ BALL_STATE_STATIC,		"BALL_STATE_STATIC",		&CBall::State_STATIC_Enter,			&CBall::State_STATIC_Think,			&CBall::State_STATIC_Leave },
		{ BALL_STATE_NORMAL,		"BALL_STATE_NORMAL",		&CBall::State_NORMAL_Enter,			&CBall::State_NORMAL_Think,			&CBall::State_NORMAL_Leave },
		{ BALL_STATE_KICKOFF,		"BALL_STATE_KICKOFF",		&CBall::State_KICKOFF_Enter,		&CBall::State_KICKOFF_Think,		&CBall::State_KICKOFF_Leave },
		{ BALL_STATE_THROWIN,		"BALL_STATE_THROWIN",		&CBall::State_THROWIN_Enter,		&CBall::State_THROWIN_Think,		&CBall::State_THROWIN_Leave },
		{ BALL_STATE_GOALKICK,		"BALL_STATE_GOALKICK",		&CBall::State_GOALKICK_Enter,		&CBall::State_GOALKICK_Think,		&CBall::State_GOALKICK_Leave },
		{ BALL_STATE_CORNER,		"BALL_STATE_CORNER",		&CBall::State_CORNER_Enter,			&CBall::State_CORNER_Think,			&CBall::State_CORNER_Leave },
		{ BALL_STATE_GOAL,			"BALL_STATE_GOAL",			&CBall::State_GOAL_Enter,			&CBall::State_GOAL_Think,			&CBall::State_GOAL_Leave },
		{ BALL_STATE_FREEKICK,		"BALL_STATE_FREEKICK",		&CBall::State_FREEKICK_Enter,		&CBall::State_FREEKICK_Think,		&CBall::State_FREEKICK_Leave },
		{ BALL_STATE_PENALTY,		"BALL_STATE_PENALTY",		&CBall::State_PENALTY_Enter,		&CBall::State_PENALTY_Think,		&CBall::State_PENALTY_Leave },
		{ BALL_STATE_KEEPERHANDS,	"BALL_STATE_KEEPERHANDS",	&CBall::State_KEEPERHANDS_Enter,	&CBall::State_KEEPERHANDS_Think,	&CBall::State_KEEPERHANDS_Leave },
	};

	for (int i = 0; i < ARRAYSIZE(ballStateInfos); i++)
	{
		if (ballStateInfos[i].m_eBallState == state)
			return &ballStateInfos[i];
	}

	return NULL;
}

bool CBall::CanReachBallStandingXY()
{
	return m_vPlDirToBall.Length2D() <= sv_ball_standing_reach.GetFloat();
}

bool CBall::GetCollisionPoint(bool isDeflection, Vector &collisionPoint)
{
	Vector dirToBall = m_vPos - m_vPlPos;
	Vector localDirToBall;
	VectorIRotate(dirToBall, m_pPl->EntityToWorldTransform(), localDirToBall);
	float zDist = dirToBall.z;
	float xyDist = dirToBall.Length2D();

	if (m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_SLIDE
		|| m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_DIVINGHEADER
		|| m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_KEEPER_DIVE_LEFT
		|| m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_KEEPER_DIVE_RIGHT
		|| m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_KEEPER_DIVE_FORWARD
		|| m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD)
	{
		float padding = isDeflection ? 0 : -15;
		Vector plDir;

		if (m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_KEEPER_DIVE_LEFT)
			plDir = -m_vPlRight;
		else if (m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_KEEPER_DIVE_RIGHT)
			plDir = m_vPlRight;
		else
			plDir = m_vPlForward2D;

		Vector plOffsetPos = Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + BALL_PHYS_RADIUS);
		float dot = (m_vPos - plOffsetPos).Dot(plDir);
		Vector closestPoint = plOffsetPos + plDir * dot;
		Vector closestDir = m_vPos - closestPoint;
		float plDistToClosestPoint = (closestPoint - plOffsetPos).Length();

		if ((dot >= 0 && plDistToClosestPoint <= sv_ball_slideforwardreach_ball.GetFloat() + padding
			|| dot < 0 && plDistToClosestPoint <= sv_ball_slidebackwardreach_ball.GetFloat() + padding)
			&& closestDir.Length() <= sv_ball_slidesidereach_ball.GetFloat() + padding)
		{
			if (dot >= 0 && plDistToClosestPoint > (sv_ball_slideforwardreach_ball.GetFloat() + padding) - (sv_ball_slidesidereach_ball.GetFloat() + padding))
			{
				collisionPoint = plOffsetPos + plDir * ((sv_ball_slideforwardreach_ball.GetFloat() + padding) - (sv_ball_slidesidereach_ball.GetFloat() + padding));
				return true;
			}
			else if (dot < 0 && plDistToClosestPoint > (sv_ball_slidebackwardreach_ball.GetFloat() + padding) - (sv_ball_slidesidereach_ball.GetFloat() + padding))
			{
				collisionPoint = plOffsetPos - plDir * ((sv_ball_slidebackwardreach_ball.GetFloat() + padding) - (sv_ball_slidesidereach_ball.GetFloat() + padding));
				return true;
			}
			else
			{
				collisionPoint = closestPoint;
				return true;
			}
		}
	}
	else
	{
		if (isDeflection)
		{
			if (zDist >= sv_ball_bodypos_deflection_start.GetFloat() && zDist < sv_ball_bodypos_deflection_end.GetFloat() && xyDist <= sv_ball_deflectionradius.GetFloat())
			{
				if (zDist > sv_ball_bodypos_deflection_end.GetFloat() - sv_ball_deflectionradius.GetFloat())
				{
					collisionPoint = Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_deflection_end.GetFloat() - sv_ball_deflectionradius.GetFloat());
					return true;
				}
				else if (zDist < sv_ball_bodypos_deflection_start.GetFloat() + sv_ball_deflectionradius.GetFloat())
				{
					collisionPoint = Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_deflection_start.GetFloat() + sv_ball_deflectionradius.GetFloat());
					return true;
				}
				else
				{
					collisionPoint = Vector(m_vPlPos.x, m_vPlPos.y, m_vPos.z);
					return true;
				}
			}
		}
		else
		{
			if (zDist >= sv_ball_bodypos_collision_start.GetFloat() && zDist < sv_ball_bodypos_collision_end.GetFloat() && xyDist <= sv_ball_collisionradius.GetFloat())
			{
				if (zDist > sv_ball_bodypos_collision_end.GetFloat() - sv_ball_collisionradius.GetFloat())
				{
					collisionPoint = Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_collision_end.GetFloat() - sv_ball_collisionradius.GetFloat());
					return true;
				}
				else if (zDist < sv_ball_bodypos_collision_start.GetFloat() + sv_ball_collisionradius.GetFloat())
				{
					collisionPoint = Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_collision_start.GetFloat() + sv_ball_collisionradius.GetFloat());
					return true;
				}
				else
				{
					collisionPoint = Vector(m_vPlPos.x, m_vPlPos.y, m_vPos.z);
					return true;
				}
			}
		}
	}

	return false;
}

extern ConVar sv_player_mass;

bool CBall::DoBodyPartAction()
{
	Vector dirToBall = m_vPos - m_vPlPos;
	Vector localDirToBall;
	VectorIRotate(dirToBall, m_pPl->EntityToWorldTransform(), localDirToBall);
	float zDist = dirToBall.z;

	if ((m_pPl->IsNormalshooting() || m_pPl->IsKeeperDiving())
		&& m_pPl->CanShoot()
		&& m_pPl->GetTeamPosType() == POS_GK
		&& m_nInPenBoxOfTeam == m_pPl->GetTeamNumber()
		&& !m_pPl->m_pHoldingBall
		&& m_pPl->m_Shared.m_nInPenBoxOfTeam == m_pPl->GetTeamNumber())
	{
		if (IsLegallyCatchableByKeeper())
			return CheckKeeperCatch();
	}

	if (CheckCollision())
		return false;

	if (m_pPl->IsKeeperDiving())
		return false;

	if (!m_pPl->IsShooting() || !m_pPl->CanShoot() || gpGlobals->curtime < m_flGlobalLastShot + m_flGlobalDynamicShotDelay * sv_ball_shotdelay_global_coeff.GetFloat())
		return false;

	if (m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_SLIDE)
		return DoSlideAction();

	if (zDist >= sv_ball_bodypos_feet_start.GetFloat()
		&& zDist < sv_ball_bodypos_hip_start.GetFloat()
		&& CanReachBallStandingXY())
	{
		return DoGroundShot(true);
	}

	if (zDist >= sv_ball_bodypos_hip_start.GetFloat() && zDist < sv_ball_bodypos_chest_start.GetFloat() && CanReachBallStandingXY())
		return DoVolleyShot();

	if (zDist >= sv_ball_bodypos_chest_start.GetFloat() && zDist < sv_ball_bodypos_head_start.GetFloat() && CanReachBallStandingXY())
		return DoChestDrop();

	if (zDist >= sv_ball_bodypos_head_start.GetFloat() && zDist < sv_ball_bodypos_head_end.GetFloat() && CanReachBallStandingXY())
		return DoHeader();

	return false;
}

// http://gamedev.stackexchange.com/a/15936
bool CBall::CheckCollision()
{
	Vector dirToBall = m_vPos - m_vPlPos;
	Vector localDirToBall;
	VectorIRotate(dirToBall, m_pPl->EntityToWorldTransform(), localDirToBall);
	bool collide = false;
	float collisionCoeff;
	float ballMass;
	Vector collisionPoint;

	if ((m_pPl->IsShooting() && m_pPl->CanShoot()
		&& gpGlobals->curtime < m_flGlobalLastShot + m_flGlobalDynamicShotDelay * sv_ball_shotdelay_global_coeff.GetFloat()
		|| m_pPl->IsKeeperDiving())
		&& GetCollisionPoint(true, collisionPoint))
	{
		collide = true;
		collisionCoeff = sv_ball_collision_deflection_coeff.GetFloat();
		ballMass = sv_ball_collision_deflection_mass.GetFloat();
	}
	else if ((!m_pPl->IsShooting() || !m_pPl->CanShoot() || m_pPl->IsKeeperDiving()) && GetCollisionPoint(false, collisionPoint))
	{
		collide = true;

		BallTouchInfo *pLastShot = LastInfo(true);

		if (pLastShot && pLastShot->m_pPl == m_pPl)
		{
			if (DotProduct2D(m_vVel.AsVector2D(), m_vPlVel.AsVector2D()) >= 0
				&& DotProduct2D(m_vVel.AsVector2D(), dirToBall.AsVector2D()) >= 0
				&& DotProduct2D(m_vPlVel.AsVector2D(), dirToBall.AsVector2D()) >= 0)
			{
				collisionCoeff = sv_ball_collision_dribbling_coeff.GetFloat();
				ballMass = sv_ball_collision_dribbling_mass.GetFloat();
			}
			else
			{
				collisionCoeff = sv_ball_collision_selfhit_coeff.GetFloat();
				ballMass = sv_ball_collision_selfhit_mass.GetFloat();
			}
		}
		else
		{
			collisionCoeff = sv_ball_collision_passive_coeff.GetFloat();
			ballMass = sv_ball_collision_passive_mass.GetFloat();
		}
	}

	if (!collide)
		return false;

	Vector dirToBallNormal = m_vPos - collisionPoint;
	dirToBallNormal.NormalizeInPlace();
	float dotballVel = m_vVel.Dot(dirToBallNormal);
	float dotPlayerVel = m_vPlVel.Dot(dirToBallNormal);
	float indicator = dotballVel - dotPlayerVel;

	// If indicator is bigger or equal to 0, the ball is either moving away from the player or going the same speed, so there's no need to apply additional velocity
	if (indicator >= 0)
		return false;

	float optimizedP = 2 * indicator / (ballMass + sv_player_mass.GetFloat());
	Vector vel = m_vVel - optimizedP * sv_player_mass.GetFloat() * dirToBallNormal;
	vel *= collisionCoeff;

	if (m_vVel.Length() > 900)
		m_pPl->EmitSound("Player.Oomph");

	Touched(false, BODY_PART_UNKNOWN, m_vVel);

	m_vVel = vel;

	m_pPhys->SetVelocity(&m_vVel, &m_vRot);

	return true;
}

bool CBall::DoSlideAction()
{
	if (!m_pPl->IsShooting())
		return false;

	Vector dirToBall = m_vPos - m_vPlPos;
	float zDist = dirToBall.z;
	Vector localDirToBall;
	VectorIRotate(dirToBall, m_pPl->EntityToWorldTransform(), localDirToBall);

	bool canShootBall = zDist < sv_ball_slidezend.GetFloat()
		&& zDist >= sv_ball_slidezstart.GetFloat()
		&& localDirToBall.x >= -sv_ball_slidebackwardreach_ball.GetFloat()
		&& localDirToBall.x <= sv_ball_slideforwardreach_ball.GetFloat()
		&& abs(localDirToBall.y) <= sv_ball_slidesidereach_ball.GetFloat();
	
	if (!canShootBall)
		return false;

	Vector forward;
	AngleVectors(QAngle(sv_ball_slide_pitchangle.GetFloat(), m_aPlAng[YAW], 0), &forward, NULL, NULL);

	Vector vel = forward * GetNormalshotStrength(GetPitchCoeff(), sv_ball_slide_strength.GetInt());

	vel += m_vPlForwardVel2D * sv_ball_slide_playerspeedcoeff.GetFloat();

	SetVel(vel, 0, FL_SPIN_FORCE_NONE, BODY_PART_FEET, true, sv_ball_shottaker_mindelay_short.GetFloat(), true);

	if (!SDKGameRules()->IsIntermissionState() && State_Get() == BALL_STATE_NORMAL && !HasQueuedState())
		m_pPl->AddSlidingTackleCompleted();

	return true;
}

bool CBall::CheckKeeperCatch()
{
	Vector dirToBall = m_vPos - m_vPlPos;
	Vector localDirToBall;
	VectorIRotate(dirToBall, m_pPl->EntityToWorldTransform(), localDirToBall);

	bool canReach = false;
	static float sqrt2 = sqrt(2.0f);
	float distXY = localDirToBall.Length2D();
	float diveTypeCatchCoeff = 1.0f;
	float ballPosCatchCoeff = 1.0f;

	switch (m_pPl->m_Shared.GetAnimEvent())
	{
	case PLAYERANIMEVENT_KEEPER_DIVE_LEFT:
		{
			canReach = (localDirToBall.z < sv_ball_keeper_sidedive_zend.GetFloat()
				&& localDirToBall.z >= sv_ball_keeper_sidedive_zstart.GetFloat()
				&& abs(localDirToBall.x) <= sv_ball_keeper_sidedive_shortsidereach.GetFloat()
				&& localDirToBall.y >= -sv_ball_keeper_sidedive_longsidereach_opposite.GetFloat()
				&& localDirToBall.y <= sv_ball_keeper_sidedive_longsidereach.GetFloat());

			if (canReach)
			{
				float distY = localDirToBall.y - sv_ball_keeper_sidedive_catchcenteroffset_side.GetFloat(); 
				float maxYReach = (distY >= 0 ? sv_ball_keeper_sidedive_longsidereach.GetFloat() : -sv_ball_keeper_sidedive_longsidereach_opposite.GetFloat()) - sv_ball_keeper_sidedive_catchcenteroffset_side.GetFloat();

				ballPosCatchCoeff = Square(min(1, abs(distY) / maxYReach));
				diveTypeCatchCoeff = sv_ball_keepercatchdelay_sidedive_global_coeff.GetFloat();
			}
		}
		break;
	case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT:
		{
			canReach = (localDirToBall.z < sv_ball_keeper_sidedive_zend.GetFloat()
				&& localDirToBall.z >= sv_ball_keeper_sidedive_zstart.GetFloat()
				&& abs(localDirToBall.x) <= sv_ball_keeper_sidedive_shortsidereach.GetFloat()
				&& localDirToBall.y <= sv_ball_keeper_sidedive_longsidereach_opposite.GetFloat()
				&& localDirToBall.y >= -sv_ball_keeper_sidedive_longsidereach.GetFloat());

			if (canReach)
			{
				float distY = localDirToBall.y - -sv_ball_keeper_sidedive_catchcenteroffset_side.GetFloat(); 
				float maxYReach = (distY >= 0 ? sv_ball_keeper_sidedive_longsidereach_opposite.GetFloat() : -sv_ball_keeper_sidedive_longsidereach.GetFloat()) - -sv_ball_keeper_sidedive_catchcenteroffset_side.GetFloat();

				ballPosCatchCoeff = Square(min(1, abs(distY) / maxYReach));
				diveTypeCatchCoeff = sv_ball_keepercatchdelay_sidedive_global_coeff.GetFloat();
			}
		}
		break;
	case PLAYERANIMEVENT_KEEPER_DIVE_FORWARD:
		canReach = (localDirToBall.z < sv_ball_keeper_forwarddive_zend.GetFloat()
			&& localDirToBall.z >= sv_ball_keeper_forwarddive_zstart.GetFloat()
			&& localDirToBall.x >= -sv_ball_keeper_forwarddive_longsidereach_opposite.GetFloat()
			&& localDirToBall.x <= sv_ball_keeper_forwarddive_longsidereach.GetFloat()
			&& abs(localDirToBall.y) <= sv_ball_keeper_forwarddive_shortsidereach.GetFloat());

		if (canReach)
		{
			ballPosCatchCoeff = sv_ball_keeper_forwarddive_catchcoeff.GetFloat();
			diveTypeCatchCoeff = sv_ball_keepercatchdelay_forwarddive_global_coeff.GetFloat();
		}
		break;
	case PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD:
		canReach = (localDirToBall.z < sv_ball_keeper_backwarddive_zend.GetFloat()
			&& localDirToBall.z >= sv_ball_keeper_backwarddive_zstart.GetFloat()
			&& localDirToBall.x >= -sv_ball_keeper_backwarddive_longsidereach.GetFloat()
			&& localDirToBall.x <= sv_ball_keeper_backwarddive_longsidereach_opposite.GetFloat()
			&& abs(localDirToBall.y) <= sv_ball_keeper_backwarddive_shortsidereach.GetFloat());

		if (canReach)
		{
			ballPosCatchCoeff = sv_ball_keeper_backwarddive_catchcoeff.GetFloat();
			diveTypeCatchCoeff = sv_ball_keepercatchdelay_backwarddive_global_coeff.GetFloat();
		}
		break;
	case PLAYERANIMEVENT_KEEPER_JUMP:
	default: // Keeper standing
		float maxReachXY = m_pPl->m_nButtons & m_pPl->GetKeeper1on1Key() ? sv_ball_keeper_standing_reach_walking.GetFloat() : sv_ball_keeper_standing_reach.GetFloat();

		canReach = (localDirToBall.z < sv_ball_bodypos_keeperarms_end.GetFloat()
			&& localDirToBall.z >= sv_ball_bodypos_feet_start.GetFloat()
			&& distXY <= maxReachXY);

		if (canReach)
		{
			float distY = localDirToBall.y; 
			float maxYReach = distY >= 0 ? maxReachXY : -maxReachXY;

			ballPosCatchCoeff = Square(min(1, abs(distY) / maxYReach));
			diveTypeCatchCoeff = sv_ball_keepercatchdelay_standing_global_coeff.GetFloat();
		}
		break;
	}

	if (!canReach)
		return false;
	
	ballPosCatchCoeff = clamp(ballPosCatchCoeff * (1 - sv_ball_keepercatchdelay_poscoeffmin.GetFloat()) + sv_ball_keepercatchdelay_poscoeffmin.GetFloat(), 0.0f, 1.0f);

	float nextCatch = m_flGlobalLastShot + m_flGlobalDynamicShotDelay * diveTypeCatchCoeff * ballPosCatchCoeff;

	// Check if ball should be punched away
	if (m_bHasQueuedState || gpGlobals->curtime < nextCatch || m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD)
	{
		Vector punchDir;

		QAngle angDiff = m_aPlCamAng - m_aPlAng;

		if (m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD)
		{
			QAngle ang = m_aPlCamAng;
			ang[YAW] += 180;
			ang[PITCH] = -sv_ball_keeper_backwarddive_punchupangle.GetInt();
			AngleVectors(ang, &punchDir);
		}
		else
		{
			// Punch into the player camera direction
			QAngle ang = m_aPlCamAng;

			// Add the player speed to the punch yaw direction, so new players who don't move the camera don't punch straight ahead every time
			float extraYaw = -ZeroSign(m_vPlVel2D.Dot(m_vPlRight)) * m_vPlSideVel2D.Length() * sv_ball_keeper_punch_sidespeedcoeff.GetFloat();

			// Make the extra yaw weaker the closer to 90° the model to free cam yaw difference is
			extraYaw *= 1.0f - clamp(abs(AngleDiff(m_aPlAng[YAW], m_aPlCamAng[YAW])) / 90.0f, 0.0f, 1.0f);

			ang[YAW] += extraYaw;

			AngleVectors(ang, &punchDir);
		}

		Vector vel = punchDir * max(m_vVel.Length2D(), sv_ball_keeper_punch_minstrength.GetFloat()) * sv_ball_keeperdeflectioncoeff.GetFloat();

		SetVel(vel, 0, FL_SPIN_FORCE_NONE, BODY_PART_KEEPERPUNCH, false, sv_ball_shottaker_mindelay_short.GetFloat(), true);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
	}
	// Catch ball instead of punching
	else
	{
		SetVel(vec3_origin, 0, FL_SPIN_FORCE_NONE, BODY_PART_KEEPERCATCH, false, sv_ball_shottaker_mindelay_short.GetFloat(), true);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);		
		State_Transition(BALL_STATE_KEEPERHANDS);
	}

	return true;
}

float CBall::GetPitchCoeff(bool useCamViewAngles/* = false*/)
{
	//return pow(cos((m_aPlAng[PITCH] - sv_ball_bestshotangle.GetInt()) / (PITCH_LIMIT - sv_ball_bestshotangle.GetInt()) * M_PI / 2), 2);
	// plot 0.5 + (cos(x/89 * pi/2) * 0.5), x=-89..89

	float bestAng = sv_ball_bestshotangle.GetFloat();
	float pitch = useCamViewAngles ? m_aPlCamAng[PITCH] : m_aPlAng[PITCH];

	float coeff;

	if (pitch <= bestAng)
	{
		float upCoeff = sv_ball_pitchup_fixedcoeff.GetFloat();
		double upExp = sv_ball_pitchup_exponent.GetFloat();
		coeff = upCoeff + (1 - upCoeff) * pow(cos((pitch - bestAng) / (-mp_pitchup.GetFloat() - bestAng) * M_PI / 2), upExp);		
	}
	else
	{
		float downCoeff = sv_ball_pitchdown_fixedcoeff.GetFloat();
		double downExp = sv_ball_pitchdown_exponent.GetFloat();
		coeff = downCoeff + (1 - downCoeff) * pow(cos((pitch - bestAng) / (mp_pitchdown.GetFloat() - bestAng) * M_PI / 2), downExp);
	}

	//DevMsg("coeff: %.2f\n", coeff);

	return coeff;
}

float CBall::GetNormalshotStrength(float coeff, int strength)
{
	return coeff * strength;
}

float CBall::GetChargedshotStrength(float coeff, int minStrength, int maxStrength)
{
	float shotStrength = minStrength + (maxStrength - minStrength) * m_pPl->GetChargedShotStrength();

	return coeff * shotStrength;
}

bool CBall::DoGroundShot(bool markOffsidePlayers)
{
	int spinFlags = FL_SPIN_FORCE_NONE;
	float spinCoeff = 1.0f;
	float shotStrength = 0;
	QAngle shotAngle;
	Vector vel;
	float pitchCoeff = GetPitchCoeff(true);
	float shotTakerMinDelay = 0.0f;

	if (m_pPl->DoSkillMove())
	{
		if (!m_pPl->GetGroundEntity())
			return false;

		// Fake shot
		if (!m_pPl->IsNormalshooting())
		{
			m_pPl->m_flNextShot = gpGlobals->curtime + sv_ball_shottaker_mindelay_short.GetFloat();
			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KICK);
			m_pPl->ResetShotCharging();
			return false;
		}

		spinFlags = FL_SPIN_FORCE_NONE;
		shotAngle = m_aPlCamAng;
		Vector camDir;
		AngleVectors(m_aPlCamAng, &camDir);
		float angDiff = RAD2DEG(acos(DotProduct2D(m_vPlForward.AsVector2D(), camDir.AsVector2D())));

		// Heel kick
		if (angDiff >= 90)
		{
			shotAngle[PITCH] = sv_ball_heelshot_pitch.GetFloat();
			shotStrength = sv_ball_heelshot_fixedstrength.GetFloat() + sv_ball_heelshot_variablestrength.GetFloat() * pitchCoeff;
			shotTakerMinDelay = sv_ball_shottaker_mindelay_short.GetFloat();
			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HEELKICK);
			EmitSound("Ball.Kicknormal");
		}
		// Ball roll
		else if (m_vPlLocalDirToBall.x >= 0 && !(m_pPl->m_nButtons & IN_FORWARD) && (m_pPl->m_nButtons & IN_MOVELEFT || m_pPl->m_nButtons & IN_MOVERIGHT || m_pPl->m_nButtons & IN_BACK))
		{
			shotTakerMinDelay = 0.0f;
			shotAngle = m_aPlAng;

			if (m_pPl->m_nButtons & IN_MOVELEFT)
			{
				shotAngle = QAngle(0, m_aPlAng[YAW] + 90, 0);
				shotStrength = sv_ball_roll_strength.GetInt();
			}
			else if (m_pPl->m_nButtons & IN_MOVERIGHT)
			{
				shotAngle = QAngle(0, m_aPlAng[YAW] - 90, 0);
				shotStrength = sv_ball_roll_strength.GetInt();
			}
			else
			{
				shotAngle = QAngle(0, m_aPlAng[YAW] + 180, 0);
				shotStrength = sv_ball_roll_strength.GetInt();
			}
		}
		else if (m_pPl->m_nButtons & IN_FORWARD)
		{
			if (m_pPl->m_nButtons & IN_MOVELEFT || m_pPl->m_nButtons & IN_MOVERIGHT)
			{
				// Finesse shot
				shotTakerMinDelay = sv_ball_shottaker_mindelay_short.GetFloat();
				spinFlags = FL_SPIN_PERMIT_SIDE;
				spinCoeff = sv_ball_finesseshot_spincoeff.GetFloat();
				shotStrength = GetNormalshotStrength(pitchCoeff, sv_ball_finesseshot_strength.GetInt());
				shotAngle = m_aPlAng;
				shotAngle[PITCH] = m_aPlCamAng[PITCH];
				shotAngle[PITCH] = min(sv_ball_groundshot_minangle.GetFloat(), shotAngle[PITCH]);
				m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_PASS);
				EmitSound("Ball.Kicknormal");
			}
			else
			{
				// Chip shot
				if (m_vPlLocalDirToBall.x >= 0)
				{
					shotAngle = m_aPlAng;
					shotAngle[PITCH] = sv_ball_chipshot_angle.GetInt();
					shotStrength = sv_ball_chipshot_fixedstrength.GetInt() + sv_ball_chipshot_dynamicstrength.GetInt() * pitchCoeff;
					spinFlags = FL_SPIN_FORCE_BACK;
					spinCoeff = pow(pitchCoeff, 3) * sv_ball_chipshot_spincoeff.GetFloat();
					shotTakerMinDelay = pitchCoeff;
					m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
					EmitSound("Ball.Kicknormal");
				}
				// Rainbow flick
				else
				{
					shotAngle = m_aPlAng;
					shotAngle[PITCH] = sv_ball_rainbowflick_fixedangle.GetInt() + sv_ball_rainbowflick_dynamicangle.GetInt() * (1 - pitchCoeff);
					shotStrength = sv_ball_rainbowflick_strength.GetInt();
					spinFlags = FL_SPIN_FORCE_TOP;
					spinCoeff = sv_ball_rainbowflick_spincoeff.GetFloat();
					shotTakerMinDelay = sv_ball_shottaker_mindelay_long.GetFloat();
					m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_RAINBOW_FLICK);
					EmitSound("Ball.Kicknormal");
				}
			}
		}
		else if (m_vPlLocalDirToBall.x >= 0)
		{
			// Lift ball
			shotAngle = m_aPlAng;
			shotAngle[PITCH] = sv_ball_lift_angle.GetInt();
			shotStrength = sv_ball_lift_fixedstrength.GetInt() + sv_ball_lift_variablestrength.GetInt() * pitchCoeff;
			spinFlags = FL_SPIN_FORCE_NONE;
			spinCoeff = 0;
			shotTakerMinDelay = sv_ball_shottaker_mindelay_short.GetFloat();
			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
			EmitSound("Ball.Touch");
		}
		else
		{
			return false;
		}

		Vector shotDir;
		AngleVectors(shotAngle, &shotDir);
		vel = shotDir * shotStrength;
	}
	else
	{
		spinFlags = FL_SPIN_PERMIT_SIDE;
		shotTakerMinDelay = 0.0f;

		if (m_pPl->IsNormalshooting())
			shotStrength = GetNormalshotStrength(GetPitchCoeff(), sv_ball_normalshot_strength.GetInt());
		else
			shotStrength = GetChargedshotStrength(GetPitchCoeff(), sv_ball_chargedshot_minstrength.GetInt(), sv_ball_chargedshot_maxstrength.GetInt());

		shotAngle = m_aPlAng;
		shotAngle[PITCH] = min(sv_ball_groundshot_minangle.GetFloat(), shotAngle[PITCH]);

		Vector shotDir;
		AngleVectors(shotAngle, &shotDir);
		vel = shotDir * shotStrength;

		if (vel.Length() >= sv_ball_animation_minstrength_strongshot.GetInt())
		{
			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KICK);
			EmitSound("Ball.Kickhard");
		}
		else if (vel.Length() >= sv_ball_animation_minstrength_weakshot.GetInt())
		{
			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KICK);
			EmitSound("Ball.Kicknormal");
		}
		else if (vel.Length() >= sv_ball_animation_minstrength_dribbleshot.GetInt())
		{
			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
			EmitSound("Ball.Touch");
		}
		else
		{
			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
		}
	}

	SetVel(vel, spinCoeff, spinFlags, BODY_PART_FEET, markOffsidePlayers, shotTakerMinDelay, true);

	return true;
}

bool CBall::DoVolleyShot()
{
	float spinCoeff = 1.0f;
	int spinFlags = FL_SPIN_PERMIT_SIDE;
	QAngle shotAngle = m_aPlAng;

	if (m_pPl->DoSkillMove())
	{
		shotAngle[PITCH] = m_aPlCamAng[PITCH];

		if (m_pPl->m_nButtons & IN_MOVELEFT || m_pPl->m_nButtons & IN_MOVERIGHT)
		{
			spinFlags = FL_SPIN_PERMIT_SIDE;

			if (m_pPl->m_nButtons & IN_FORWARD)
			{
				spinFlags |= FL_SPIN_FORCE_TOP;
				spinCoeff = sv_ball_volleyshot_topspincoeff.GetFloat();
			}
			else if (m_pPl->m_nButtons & IN_BACK)
			{
				spinFlags |= FL_SPIN_FORCE_BACK;
				spinCoeff = sv_ball_volleyshot_backspincoeff.GetFloat();
			}
			else
				spinCoeff = sv_ball_volleyshot_sidespincoeff.GetFloat();
		}
		else if (m_pPl->m_nButtons & IN_FORWARD)
		{
			spinCoeff = sv_ball_volleyshot_topspincoeff.GetFloat();
			spinFlags = FL_SPIN_FORCE_TOP;
		}
		else if (m_pPl->m_nButtons & IN_BACK)
		{
			spinCoeff = sv_ball_volleyshot_backspincoeff.GetFloat();
			spinFlags = FL_SPIN_FORCE_BACK;
		}
	}

	float shotStrength;

	if (m_pPl->IsNormalshooting())
		shotStrength = GetNormalshotStrength(GetPitchCoeff(), sv_ball_normalshot_strength.GetInt());
	else
		shotStrength = GetChargedshotStrength(GetPitchCoeff(), sv_ball_chargedshot_minstrength.GetInt(), sv_ball_chargedshot_maxstrength.GetInt());

	shotAngle[PITCH] = min(sv_ball_volleyshot_minangle.GetFloat(), shotAngle[PITCH]);

	Vector shotDir;
	AngleVectors(shotAngle, &shotDir);

	Vector vel = shotDir * shotStrength;

	if (vel.Length() > 700)
	{
		if (vel.Length() > 1000)
			EmitSound("Ball.Kickhard");
		else
			EmitSound("Ball.Kicknormal");

		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_VOLLEY);
	}
	else
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);

	SetVel(vel, spinCoeff, spinFlags, BODY_PART_FEET, true, sv_ball_shottaker_mindelay_short.GetFloat(), true);

	return true;
}

bool CBall::DoChestDrop()
{
	QAngle ang = m_aPlAng;
	ang[PITCH] = sv_ball_chestdrop_angle.GetInt();

	Vector dir;
	AngleVectors(ang, &dir);

	Vector vel = dir * sv_ball_chestdrop_strength.GetInt();
	vel += m_vPlForwardVel2D * sv_ball_chestdrop_playerspeedcoeff.GetFloat();

	m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
	EmitSound("Ball.Touch");

	SetVel(vel, 0.0f, FL_SPIN_FORCE_NONE, BODY_PART_CHEST, true, sv_ball_shottaker_mindelay_short.GetFloat(), false);

	return true;
}

bool CBall::DoHeader()
{
	if (m_pPl->DoSkillMove())
	{
		if (!m_pPl->IsChargedshooting())
			return false;

		Vector camDir;
		AngleVectors(m_aPlCamAng, &camDir);
		float angDiff = RAD2DEG(acos(DotProduct2D(m_vPlForward.AsVector2D(), camDir.AsVector2D())));

		// Bicycle shot
		if (angDiff >= 90)
		{
			QAngle ang = m_aPlCamAng;

			Vector forward;
			AngleVectors(ang, &forward);

			Vector vel = forward * GetChargedshotStrength(GetPitchCoeff(true), sv_ball_bicycleshot_minstrength.GetInt(), sv_ball_bicycleshot_maxstrength.GetInt());

			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BICYCLE_KICK);
			EmitSound("Ball.Kickhard");

			SetVel(vel, 0, FL_SPIN_FORCE_NONE, BODY_PART_FEET, true, sv_ball_shottaker_mindelay_long.GetFloat(), true);
		}
		// Diving header
		else
		{
			if (!m_pPl->GetGroundEntity())
				return false;

			Vector vel, forward;
			QAngle headerAngle = m_aPlAng;

			headerAngle[PITCH] = clamp(headerAngle[PITCH], sv_ball_divingheader_maxangle.GetFloat(), sv_ball_divingheader_minangle.GetFloat());
			AngleVectors(headerAngle, &forward);

			vel = forward * GetChargedshotStrength(1.0f, sv_ball_divingheader_minstrength.GetInt(), sv_ball_divingheader_maxstrength.GetInt());

			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_DIVINGHEADER);
			EmitSound("Ball.Kickhard");
			EmitSound("Player.DivingHeader");

			SetVel(vel, sv_ball_header_spincoeff.GetFloat(), FL_SPIN_PERMIT_SIDE, BODY_PART_HEAD, true, sv_ball_shottaker_mindelay_long.GetFloat(), true);
		}
	}
	else
	{
		Vector vel, forward;

		QAngle headerAngle = m_aPlAng;

		// Normal header
		if (m_pPl->IsNormalshooting())
		{
			headerAngle[PITCH] = clamp(headerAngle[PITCH], sv_ball_header_maxangle.GetFloat(), sv_ball_header_minangle.GetFloat());
			AngleVectors(headerAngle, &forward);

			vel = forward * GetNormalshotStrength(GetPitchCoeff(), sv_ball_normalheader_strength.GetInt());

			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HEADER_STATIONARY);
			EmitSound("Ball.Kickhard");
		}
		// Charged header
		else
		{
			headerAngle[PITCH] = clamp(headerAngle[PITCH], sv_ball_header_maxangle.GetFloat(), sv_ball_header_minangle.GetFloat());
			AngleVectors(headerAngle, &forward);

			vel = forward * GetChargedshotStrength(GetPitchCoeff(), sv_ball_chargedheader_minstrength.GetInt(), sv_ball_chargedheader_maxstrength.GetInt());

			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HEADER);
			EmitSound("Ball.Kickhard");
		}

		// Add player forward move speed to ball speed
		vel += m_vPlForwardVel2D * sv_ball_header_playerspeedcoeff.GetFloat();

		SetVel(vel, sv_ball_header_spincoeff.GetFloat(), FL_SPIN_PERMIT_SIDE, BODY_PART_HEAD, true, sv_ball_shottaker_mindelay_short.GetFloat(), true);
	}

	return true;
}

AngularImpulse CBall::CalcSpin(float coeff, int spinFlags)
{	
	Vector worldRot;
	float pitch = m_aPlAng[PITCH];

	if (spinFlags & FL_SPIN_RETAIN_SIDE)
	{
		VectorRotate(m_vRot, EntityToWorldTransform(), worldRot);
		worldRot.x = 0;
		worldRot.y = 0;
		worldRot *= coeff;
	}
	else
	{
		Vector sideRot = vec3_origin;
		float sideSpin = 0;

		if (spinFlags & FL_SPIN_PERMIT_SIDE)
		{
			sideSpin = sv_ball_spin.GetInt() * coeff * GetPitchCoeff();

			int sidemoveSign = m_pPl->GetSidemoveSign();

			if (sidemoveSign == -1) 
				sideRot = Vector(0, 0, m_pPl->IsLegacySideCurl() ? 1 : -1);
			else if (sidemoveSign == 1) 
				sideRot = Vector(0, 0, m_pPl->IsLegacySideCurl() ? -1 : 1);
		}

		Vector backRot = m_vPlRight;
		float backSpin = 0;

		Vector topRot = -m_vPlRight;
		float topSpin = 0;

		if (spinFlags & FL_SPIN_FORCE_BACK)
			backSpin = sv_ball_spin.GetInt() * coeff;

		if (spinFlags & FL_SPIN_FORCE_TOP)
			topSpin = sv_ball_spin.GetInt() * coeff;

		worldRot = sideRot * sideSpin + backRot * backSpin + topRot * topSpin;
	}

	AngularImpulse randRot = vec3_origin;

	for (int i = 0; i < 3; i++)
	{
		// Add some weak random rotation to all three axes, since a ball which only rotates in one or no axis looks unnatural
		randRot[i] = g_IOSRand.RandomInt(0, 1) == 1 ? 1 : -1;
	}

	worldRot += randRot * sv_ball_defaultspin.GetInt();

	AngularImpulse localRot;

	VectorIRotate(worldRot, EntityToWorldTransform(), localRot);

	return localRot;
}

void CBall::Think( void	)
{
	SetNextThink(gpGlobals->curtime);

	State_Think();
}

void CBall::UpdateCarrier()
{
	if (CSDKPlayer::IsOnField(m_pPl))
	{
		m_vPlPos = m_pPl->GetLocalOrigin();

		m_vPlVel = m_pPl->GetLocalVelocity();
		m_vPlVel2D = Vector(m_vPlVel.x, m_vPlVel.y, 0);

		m_aPlAng = m_pPl->EyeAngles();
		m_aPlAng[PITCH] = clamp(m_aPlAng[PITCH], -mp_pitchup.GetFloat(), mp_pitchdown.GetFloat());
		AngleVectors(m_aPlAng, &m_vPlForward, &m_vPlRight, &m_vPlUp);
		
		m_aPlCamAng = m_pPl->m_aCamViewAngles;
		m_aPlCamAng[PITCH] = clamp(m_aPlCamAng[PITCH], -mp_pitchup.GetFloat(), mp_pitchdown.GetFloat());
		
		m_vPlForward2D = m_vPlForward;
		m_vPlForward2D.z = 0;
		m_vPlForward2D.NormalizeInPlace();

		m_vPlForwardVel2D = m_vPlForward2D * max(0, m_vPlVel2D.Dot(m_vPlForward2D));
		m_vPlBackVel2D = m_vPlForward2D * min(0, m_vPlVel2D.Dot(m_vPlForward2D));
		m_vPlSideVel2D = -m_vPlRight * m_vPlVel2D.Dot(m_vPlRight);
		
		m_vPlDirToBall = m_vPos - m_vPlPos;
		VectorIRotate(m_vPlDirToBall, m_pPl->EntityToWorldTransform(), m_vPlLocalDirToBall);
	}
}

int CBall::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CBall::Reset()
{
	ReloadSettings();
	m_pPl = NULL;
	m_bHasQueuedState = false;
	RemoveEffects(EF_NODRAW);
	EnablePlayerCollisions(true);
	m_pPhys->EnableMotion(true);
	m_pPhys->Wake();
	m_pHoldingPlayer = NULL;
	m_bHitThePost = false;
	m_nInPenBoxOfTeam = TEAM_NONE;
}

void CBall::ReloadSettings()
{
	CreateVPhysics();
}

void CBall::EnablePlayerCollisions(bool enable)
{
	SetCollisionGroup(enable ? COLLISION_GROUP_SOLID_BALL : COLLISION_GROUP_NONSOLID_BALL);
}

void CBall::RemoveFromPlayerHands(CSDKPlayer *pPl)
{
	if (CSDKPlayer::IsOnField(pPl) && pPl->GetTeamPosType() == POS_GK && pPl->m_pHoldingBall.Get() == this)
	{
		pPl->m_pHoldingBall = NULL;
		pPl->DoServerAnimationEvent(PLAYERANIMEVENT_CARRY_END);
	}

	if (!IsMarkedForDeletion())
	{
		m_pHoldingPlayer = NULL;
		RemoveEffects(EF_NODRAW);
		EnablePlayerCollisions(true);
		m_pPhys->EnableMotion(true);
		m_pPhys->Wake();
	}
}

Vector CBall::GetPos()
{
	return m_vPos;
}

QAngle CBall::GetAng()
{
	return m_aAng;
}

Vector CBall::GetVel()
{
	return m_vVel;
}

AngularImpulse CBall::GetRot()
{
	return m_vRot;
}

void CBall::SetSkinName(const char *skinName)
{
	if (m_szSkinName[0] != '\0' && !Q_strcmp(skinName, m_szSkinName))
		return;

	int ballSkinIndex = -1;

	for (int i = 0; i < CBallInfo::m_BallInfo.Count(); i++)
	{
		if (!Q_strcmp(CBallInfo::m_BallInfo[i]->m_szFolderName, skinName))
		{
			ballSkinIndex = i;
			break;
		}
	}

	if (ballSkinIndex == -1)
		ballSkinIndex = g_IOSRand.RandomInt(0, CBallInfo::m_BallInfo.Count() - 1);

	Q_strncpy(m_szSkinName.GetForModify(), CBallInfo::m_BallInfo[ballSkinIndex]->m_szFolderName, sizeof(m_szSkinName));
}

void CBall::CheckPenBoxPosition()
{
	for (int team = TEAM_HOME; team <= TEAM_AWAY; team++)
	{
		Vector min = GetGlobalTeam(team)->m_vPenBoxMin;
		Vector max = GetGlobalTeam(team)->m_vPenBoxMax;

		if (m_vPos.x + BALL_PHYS_RADIUS >= min.x
			&& m_vPos.y + BALL_PHYS_RADIUS >= min.y
			&& m_vPos.x - BALL_PHYS_RADIUS <= max.x
			&& m_vPos.y - BALL_PHYS_RADIUS <= max.y)
		{
			m_nWasInPenBoxOfTeam = m_nInPenBoxOfTeam;
			m_nInPenBoxOfTeam = team;
			return;
		}
	}

	m_nWasInPenBoxOfTeam = m_nInPenBoxOfTeam;
	m_nInPenBoxOfTeam = TEAM_NONE;
}

void CBall::RemoveAllTouches()
{
	if (!m_bHasQueuedState)
		m_Touches.PurgeAndDeleteElements();
}

BallTouchInfo *CBall::LastInfo(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/, CSDKPlayer *pSkipPl2 /*= NULL*/, CSDKPlayer *pSkipPl3 /*= NULL*/)
{
	for (int i = m_Touches.Count() - 1; i >= 0; i--)
	{
		if (pSkipPl && m_Touches[i]->m_pPl == pSkipPl)
			continue;

		if (pSkipPl2 && m_Touches[i]->m_pPl == pSkipPl2)
			continue;

		if (pSkipPl3 && m_Touches[i]->m_pPl == pSkipPl3)
			continue;

		if (!wasShooting || m_Touches[i]->m_bIsShot)
			return m_Touches[i];
	}

	return NULL;
}

CSDKPlayer *CBall::LastPl(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/, CSDKPlayer *pSkipPl2 /*= NULL*/, CSDKPlayer *pSkipPl3 /*= NULL*/)
{
	BallTouchInfo *info = LastInfo(wasShooting, pSkipPl, pSkipPl2, pSkipPl3);
	if (info && CSDKPlayer::IsOnField(info->m_pPl))
		return info->m_pPl;
	
	return NULL;
}

int CBall::LastTeam(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/, CSDKPlayer *pSkipPl2 /*= NULL*/, CSDKPlayer *pSkipPl3 /*= NULL*/)
{
	BallTouchInfo *info = LastInfo(wasShooting, pSkipPl, pSkipPl2, pSkipPl3);
	return info ? info->m_nTeam : TEAM_NONE;
}

int CBall::LastOppTeam(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/, CSDKPlayer *pSkipPl2 /*= NULL*/, CSDKPlayer *pSkipPl3 /*= NULL*/)
{
	BallTouchInfo *info = LastInfo(wasShooting, pSkipPl, pSkipPl2, pSkipPl3);
	return info ? (info->m_nTeam == TEAM_HOME ? TEAM_AWAY : TEAM_HOME) : TEAM_NONE;
}

void CBall::GetPredictedGoalLineCrossPosX(int &xPos, int &team)
{
	Vector vel = GetVel();
	Vector dir = vel;
	dir.z = 0;
	dir.NormalizeInPlace();

	team = TEAM_NONE;

	for (int i = 0; i < 2; i++)
	{
		if (-GetGlobalTeam(i + TEAM_HOME)->m_nForward == ZeroSign(dir.y))
		{
			team = i + TEAM_HOME;
			break;
		}
	}

	if (team == TEAM_NONE)
	{
		xPos = 0;
	}
	else
	{
		Vector pos = GetPos();
		float ang = acos(Vector(0, Sign(vel.y), 0).Dot(dir));
		xPos = pos.x + Sign(vel.x) * tan(ang) * abs(GetGlobalTeam(team)->m_vGoalCenter.GetY() - pos.y);
	}
}