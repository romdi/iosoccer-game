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
ConVar sv_ball_damping( "sv_ball_damping", "0.01", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_rotdamping( "sv_ball_rotdamping", "0.75", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_rotinertialimit( "sv_ball_rotinertialimit", "1.5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_dragcoeff( "sv_ball_dragcoeff", "1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_inertia( "sv_ball_inertia", "1.5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_drag_enabled("sv_ball_drag_enabled", "1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );

ConVar sv_ball_spin( "sv_ball_spin", "500", FCVAR_NOTIFY );
ConVar sv_ball_defaultspin( "sv_ball_defaultspin", "10000", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar sv_ball_topspin_coeff( "sv_ball_topspin_coeff", "0.1", FCVAR_NOTIFY );
ConVar sv_ball_backspin_coeff( "sv_ball_backspin_coeff", "0.25", FCVAR_NOTIFY );
ConVar sv_ball_jump_topspin_enabled("sv_ball_jump_topspin_enabled", "1", FCVAR_NOTIFY );
ConVar sv_ball_curve("sv_ball_curve", "200", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);

ConVar sv_ball_deflectionradius( "sv_ball_deflectionradius", "30", FCVAR_NOTIFY );

ConVar sv_ball_standing_reach( "sv_ball_standing_reach", "50", FCVAR_NOTIFY );
ConVar sv_ball_standing_cone( "sv_ball_standing_cone", "360", FCVAR_NOTIFY );
ConVar sv_ball_standing_shift( "sv_ball_standing_shift", "0", FCVAR_NOTIFY );

ConVar sv_ball_slideaffectedbydelay( "sv_ball_slideaffectedbydelay", "1", FCVAR_NOTIFY );
ConVar sv_ball_slidesidereach_ball( "sv_ball_slidesidereach_ball", "50", FCVAR_NOTIFY );
ConVar sv_ball_slideforwardreach_ball( "sv_ball_slideforwardreach_ball", "60", FCVAR_NOTIFY );
ConVar sv_ball_slidesidereach_foul( "sv_ball_slidesidereach_foul", "25", FCVAR_NOTIFY );
ConVar sv_ball_slideforwardreach_foul( "sv_ball_slideforwardreach_foul", "60", FCVAR_NOTIFY );
ConVar sv_ball_slidesidespeedcoeff("sv_ball_slidesidespeedcoeff", "0.66", FCVAR_NOTIFY); 
ConVar sv_ball_slidezstart("sv_ball_slidezstart", "-50", FCVAR_NOTIFY); 
ConVar sv_ball_slidezend("sv_ball_slidezend", "40", FCVAR_NOTIFY); 

ConVar sv_ball_keeper_standing_reach_top( "sv_ball_keeper_standing_reach_top", "65", FCVAR_NOTIFY );
ConVar sv_ball_keeper_standing_reach_bottom( "sv_ball_keeper_standing_reach_bottom", "35", FCVAR_NOTIFY );
ConVar sv_ball_keeper_standing_catchcenteroffset_side( "sv_ball_keeper_standing_catchcenteroffset_side", "0", FCVAR_NOTIFY );
ConVar sv_ball_keeper_standing_catchcenteroffset_z( "sv_ball_keeper_standing_catchcenteroffset_z", "50", FCVAR_NOTIFY );

ConVar sv_ball_keeper_forwarddive_shortsidereach( "sv_ball_keeper_forwarddive_shortsidereach", "50", FCVAR_NOTIFY );
ConVar sv_ball_keeper_forwarddive_longsidereach( "sv_ball_keeper_forwarddive_longsidereach", "100", FCVAR_NOTIFY );
ConVar sv_ball_keeper_forwarddive_longsidereach_opposite( "sv_ball_keeper_forwarddive_longsidereach_opposite", "40", FCVAR_NOTIFY );
ConVar sv_ball_keeper_forwarddive_zstart( "sv_ball_keeper_forwarddive_zstart", "-50", FCVAR_NOTIFY );
ConVar sv_ball_keeper_forwarddive_zend( "sv_ball_keeper_forwarddive_zend", "40", FCVAR_NOTIFY );
ConVar sv_ball_keeper_forwarddive_catchcoeff( "sv_ball_keeper_forwarddive_catchcoeff", "0.75", FCVAR_NOTIFY );

ConVar sv_ball_keeper_sidedive_shortsidereach( "sv_ball_keeper_sidedive_shortsidereach", "50", FCVAR_NOTIFY );
ConVar sv_ball_keeper_sidedive_longsidereach( "sv_ball_keeper_sidedive_longsidereach", "60", FCVAR_NOTIFY );
ConVar sv_ball_keeper_sidedive_longsidereach_opposite( "sv_ball_keeper_sidedive_longsidereach_opposite", "50", FCVAR_NOTIFY );
ConVar sv_ball_keeper_sidedive_zstart( "sv_ball_keeper_sidedive_zstart", "-20", FCVAR_NOTIFY );
ConVar sv_ball_keeper_sidedive_zend( "sv_ball_keeper_sidedive_zend", "65", FCVAR_NOTIFY );
ConVar sv_ball_keeper_sidedive_catchcenteroffset_side( "sv_ball_keeper_sidedive_catchcenteroffset_side", "0", FCVAR_NOTIFY );
ConVar sv_ball_keeper_sidedive_catchcenteroffset_z( "sv_ball_keeper_sidedive_catchcenteroffset_z", "40", FCVAR_NOTIFY );

ConVar sv_ball_keeper_punch_maxyawangle( "sv_ball_keeper_punch_maxyawangle", "130", FCVAR_NOTIFY );
ConVar sv_ball_keeper_punch_maxpitchangle( "sv_ball_keeper_punch_maxpitchangle", "130", FCVAR_NOTIFY );
ConVar sv_ball_keeper_punch_pitchoffset( "sv_ball_keeper_punch_pitchoffset", "0", FCVAR_NOTIFY );
ConVar sv_ball_keeper_punch_shortsidecoeff( "sv_ball_keeper_punch_shortsidecoeff", "0.5", FCVAR_NOTIFY );
ConVar sv_ball_keeper_punch_minstrength( "sv_ball_keeper_punch_minstrength", "0", FCVAR_NOTIFY );
ConVar sv_ball_keeper_punch_minpitchangle( "sv_ball_keeper_punch_minpitchangle", "180", FCVAR_NOTIFY );

ConVar sv_ball_keeperpunchupstrength("sv_ball_keeperpunchupstrength", "500", FCVAR_NOTIFY);
ConVar sv_ball_keeperdeflectioncoeff("sv_ball_keeperdeflectioncoeff", "0.66", FCVAR_NOTIFY);

ConVar sv_ball_shotdelay_normal("sv_ball_shotdelay_normal", "0.15", FCVAR_NOTIFY);
ConVar sv_ball_shotdelay_setpiece("sv_ball_shotdelay_setpiece", "0.5", FCVAR_NOTIFY);
ConVar sv_ball_shotdelay_global("sv_ball_shotdelay_global", "0.25", FCVAR_NOTIFY);
ConVar sv_ball_shotdelay_global_coeff("sv_ball_shotdelay_global_coeff", "0.5", FCVAR_NOTIFY);
ConVar sv_ball_keepercatchdelay_global_coeff("sv_ball_keepercatchdelay_global_coeff", "1.0", FCVAR_NOTIFY);
ConVar sv_ball_dynamicshotdelay_enabled("sv_ball_dynamicshotdelay_enabled", "1", FCVAR_NOTIFY);
ConVar sv_ball_dynamicshotdelay_mindelay("sv_ball_dynamicshotdelay_mindelay", "0.2", FCVAR_NOTIFY);
ConVar sv_ball_dynamicshotdelay_maxdelay("sv_ball_dynamicshotdelay_maxdelay", "1.0", FCVAR_NOTIFY);
ConVar sv_ball_dynamicshotdelay_minshotstrength("sv_ball_dynamicshotdelay_minshotstrength", "400", FCVAR_NOTIFY);
ConVar sv_ball_dynamicshotdelay_maxshotstrength("sv_ball_dynamicshotdelay_maxshotstrength", "1500", FCVAR_NOTIFY);
ConVar sv_ball_dynamicbounce_enabled("sv_ball_dynamicbouncedelay_enabled", "1", FCVAR_NOTIFY);

ConVar sv_ball_bestshotangle("sv_ball_bestshotangle", "-20", FCVAR_NOTIFY);

ConVar sv_ball_pitchdown_exponent_normalshot("sv_ball_pitchdown_exponent_normalshot", "2.5", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchdowncoeff_normalshot("sv_ball_fixedpitchdowncoeff_normalshot", "0.35", FCVAR_NOTIFY);
ConVar sv_ball_pitchdown_exponent_nonnormalshot("sv_ball_pitchdown_exponent_nonnormalshot", "2", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchdowncoeff_nonnormalshot("sv_ball_fixedpitchdowncoeff_nonnormalshot", "0.37", FCVAR_NOTIFY);
ConVar sv_ball_pitchup_exponent("sv_ball_pitchup_exponent", "3", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchupcoeff("sv_ball_fixedpitchupcoeff", "0.35", FCVAR_NOTIFY);

ConVar sv_ball_bestbackspinangle("sv_ball_bestbackspinangle", "-55", FCVAR_NOTIFY);
ConVar sv_ball_pitchdownbackspin_exponent("sv_ball_pitchdownbackspin_exponent", "4", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchdownbackspincoeff("sv_ball_fixedpitchdownbackspincoeff", "0.1", FCVAR_NOTIFY);
ConVar sv_ball_pitchupbackspin_exponent("sv_ball_pitchupbackspin_exponent", "2", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchupbackspincoeff("sv_ball_fixedpitchupbackspincoeff", "0.1", FCVAR_NOTIFY);

ConVar sv_ball_besttopspinangle("sv_ball_besttopspinangle", "-10", FCVAR_NOTIFY);
ConVar sv_ball_pitchdowntopspin_exponent("sv_ball_pitchdowntopspin_exponent", "2", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchdowntopspincoeff("sv_ball_fixedpitchdowntopspincoeff", "0.1", FCVAR_NOTIFY);
ConVar sv_ball_pitchuptopspin_exponent("sv_ball_pitchuptopspin_exponent", "4", FCVAR_NOTIFY);
ConVar sv_ball_fixedpitchuptopspincoeff("sv_ball_fixedpitchuptopspincoeff", "0.1", FCVAR_NOTIFY);

ConVar sv_ball_shotwalkcoeff("sv_ball_shotwalkcoeff", "0.5", FCVAR_NOTIFY);
ConVar sv_ball_keepercatchspeed("sv_ball_keepercatchspeed", "1000", FCVAR_NOTIFY);
ConVar sv_ball_keeperpickupangle("sv_ball_keeperpickupangle", "-90", FCVAR_NOTIFY);

ConVar sv_ball_normalshot_strength("sv_ball_normalshot_strength", "900", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_powershot_strength("sv_ball_powershot_strength", "1100", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_chargedshot_minstrength("sv_ball_chargedshot_minstrength", "800", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_chargedshot_maxstrength("sv_ball_chargedshot_maxstrength", "1500", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);

ConVar sv_ball_powerthrow_strength("sv_ball_powerthrow_strength", "800", FCVAR_NOTIFY);
ConVar sv_ball_chargedthrow_minstrength("sv_ball_chargedthrow_minstrength", "500", FCVAR_NOTIFY);
ConVar sv_ball_chargedthrow_maxstrength("sv_ball_chargedthrow_maxstrength", "1100", FCVAR_NOTIFY);

ConVar sv_ball_normalheader_strength("sv_ball_normalheader_strength", "450", FCVAR_NOTIFY); 
ConVar sv_ball_powerheader_strength("sv_ball_powerheader_strength", "700", FCVAR_NOTIFY); 
ConVar sv_ball_chargedheader_minstrength("sv_ball_chargedheader_minstrength", "500", FCVAR_NOTIFY); 
ConVar sv_ball_chargedheader_maxstrength("sv_ball_chargedheader_maxstrength", "1000", FCVAR_NOTIFY); 

ConVar sv_ball_powerdivingheader_strength("sv_ball_powerdivingheader_strength", "1000", FCVAR_NOTIFY); 
ConVar sv_ball_chargeddivingheader_minstrength("sv_ball_chargeddivingheader_minstrength", "600", FCVAR_NOTIFY); 
ConVar sv_ball_chargeddivingheader_maxstrength("sv_ball_chargeddivingheader_maxstrength", "1200", FCVAR_NOTIFY);

ConVar sv_ball_normalslide_strength("sv_ball_normalslide_strength", "700", FCVAR_NOTIFY); 
ConVar sv_ball_powerslide_strength("sv_ball_powerslide_strength", "900", FCVAR_NOTIFY); 
ConVar sv_ball_chargedslide_minstrength("sv_ball_chargedslide_minstrength", "600", FCVAR_NOTIFY); 
ConVar sv_ball_chargedslide_maxstrength("sv_ball_chargedslide_maxstrength", "1100", FCVAR_NOTIFY);

ConVar sv_ball_penaltyshot_maxstrength("sv_ball_penaltyshot_maxstrength", "1200", FCVAR_NOTIFY);

ConVar sv_ball_goalkick_speedcoeff("sv_ball_goalkick_speedcoeff", "1.15", FCVAR_NOTIFY);
ConVar sv_ball_freekick_speedcoeff("sv_ball_freekick_speedcoeff", "1.10", FCVAR_NOTIFY);
ConVar sv_ball_volleyshot_speedcoeff("sv_ball_volleyshot_speedcoeff", "1.125", FCVAR_NOTIFY);

ConVar sv_ball_keepershot_minangle("sv_ball_keepershot_minangle", "-5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);

ConVar sv_ball_groundshot_minangle("sv_ball_groundshot_minangle", "-7", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_volleyshot_minangle("sv_ball_volleyshot_minangle", "0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_throwin_minangle("sv_ball_throwin_minangle", "-5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_throwin_minstrength("sv_ball_throwin_minstrength", "300", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_autopass_minstrength("sv_ball_autopass_minstrength", "500", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_autopass_maxstrength("sv_ball_autopass_maxstrength", "1600", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_autopass_coeff("sv_ball_autopass_coeff", "1", FCVAR_NOTIFY);
ConVar sv_ball_volleyshot_spincoeff("sv_ball_volleyshot_spincoeff", "1.25", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_doubletouchfouls("sv_ball_doubletouchfouls", "1", FCVAR_NOTIFY);

ConVar sv_ball_timelimit_setpiece("sv_ball_timelimit_setpiece", "15", FCVAR_NOTIFY);
ConVar sv_ball_timelimit_remotecontrolled("sv_ball_timelimit_remotecontrolled", "15", FCVAR_NOTIFY);

ConVar sv_ball_statetransition_activationdelay_short("sv_ball_statetransition_activationdelay_short", "0.1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_statetransition_activationdelay_normal("sv_ball_statetransition_activationdelay_normal", "1.25", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_statetransition_activationdelay_long("sv_ball_statetransition_activationdelay_long", "2.0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_statetransition_messagedelay_normal("sv_ball_statetransition_messagedelay_normal", "0.5", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_statetransition_messagedelay_short("sv_ball_statetransition_messagedelay_short", "0.1", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);

ConVar sv_ball_goalcelebduration("sv_ball_goalcelebduration", "5.0", FCVAR_NOTIFY);
ConVar sv_ball_thinkinterval("sv_ball_thinkinterval", "0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar sv_ball_chestdrop_strength("sv_ball_chestdrop_strength", "100", FCVAR_NOTIFY); 
ConVar sv_ball_chestdrop_angle("sv_ball_chestdrop_angle", "80", FCVAR_NOTIFY); 
ConVar sv_ball_minshotstrength("sv_ball_minshotstrength", "200", FCVAR_NOTIFY);  
ConVar sv_ball_minspeed_passive("sv_ball_minspeed_passive", "1000", FCVAR_NOTIFY); 
ConVar sv_ball_minspeed_bounce("sv_ball_minspeed_bounce", "500", FCVAR_NOTIFY);
ConVar sv_ball_bounce_strength("sv_ball_bounce_strength", "500", FCVAR_NOTIFY);
ConVar sv_ball_player_yellow_red_card_duration("sv_ball_player_yellow_red_card_duration", "7.5", FCVAR_NOTIFY);
ConVar sv_ball_player_red_card_duration("sv_ball_player_red_card_duration", "15", FCVAR_NOTIFY);

ConVar sv_ball_bodypos_feet_start("sv_ball_bodypos_feet_start", "-50", FCVAR_NOTIFY);
ConVar sv_ball_bodypos_hip_start("sv_ball_bodypos_hip_start", "15", FCVAR_NOTIFY);
ConVar sv_ball_bodypos_chest_start("sv_ball_bodypos_chest_start", "40", FCVAR_NOTIFY);
ConVar sv_ball_bodypos_head_start("sv_ball_bodypos_head_start", "60", FCVAR_NOTIFY);
ConVar sv_ball_bodypos_head_end("sv_ball_bodypos_head_end", "85", FCVAR_NOTIFY);
ConVar sv_ball_bodypos_keeperarms_end("sv_ball_bodypos_keeperarms_end", "105", FCVAR_NOTIFY);

ConVar sv_ball_yellowcardproximity_forward("sv_ball_yellowcardproximity_forward", "0.5", FCVAR_NOTIFY);
ConVar sv_ball_yellowcardproximity_backward("sv_ball_yellowcardproximity_backward", "0.25", FCVAR_NOTIFY);
ConVar sv_ball_goalreplay_count("sv_ball_goalreplay_count", "2", FCVAR_NOTIFY);
ConVar sv_ball_goalreplay_delay("sv_ball_goalreplay_delay", "1", FCVAR_NOTIFY);
ConVar sv_ball_deflectioncoeff("sv_ball_deflectioncoeff", "0.5", FCVAR_NOTIFY);
ConVar sv_ball_update_physics("sv_ball_update_physics", "0", FCVAR_NOTIFY);

ConVar sv_ball_stats_pass_mindist("sv_ball_stats_pass_mindist", "300", FCVAR_NOTIFY);
ConVar sv_ball_stats_clearance_minspeed("sv_ball_stats_clearance_minspeed", "800", FCVAR_NOTIFY);
ConVar sv_ball_stats_shot_mindist("sv_ball_stats_shot_mindist", "300", FCVAR_NOTIFY);
ConVar sv_ball_stats_save_minspeed("sv_ball_stats_save_minspeed", "800", FCVAR_NOTIFY);
ConVar sv_ball_stats_assist_maxtime("sv_ball_stats_assist_maxtime", "8", FCVAR_NOTIFY);

ConVar sv_ball_velocity_coeff("sv_ball_velocity_coeff", "0.9", FCVAR_NOTIFY);

ConVar sv_ball_freekickdist_owngoal("sv_ball_freekickdist_owngoal", "850", FCVAR_NOTIFY);
ConVar sv_ball_freekickdist_opponentgoal("sv_ball_freekickdist_opponentgoal", "1300", FCVAR_NOTIFY);
ConVar sv_ball_freekickangle_opponentgoal("sv_ball_freekickangle_opponentgoal", "60", FCVAR_NOTIFY);
ConVar sv_ball_closetogoaldist("sv_ball_closetogoaldist", "1300", FCVAR_NOTIFY);

ConVar sv_ball_assign_setpieces("sv_ball_assign_setpieces", "1", FCVAR_NOTIFY);

ConVar sv_ball_nonnormalshotsblocktime_freekick("sv_ball_nonnormalshotsblocktime_freekick", "4.0", FCVAR_NOTIFY);
ConVar sv_ball_nonnormalshotsblocktime_corner("sv_ball_nonnormalshotsblocktime_corner", "4.0", FCVAR_NOTIFY);
ConVar sv_ball_shotsblocktime_penalty("sv_ball_shotsblocktime_penalty", "4.0", FCVAR_NOTIFY);
ConVar sv_ball_blockpowershot("sv_ball_blockpowershot", "1", FCVAR_NOTIFY);


extern ConVar mp_client_sidecurl;

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
			pPl->GetPlayerBall()->State_Transition(BALL_STATE_NORMAL);
		}

		if (sv_ball_update_physics.GetBool())
			pPl->GetPlayerBall()->CreateVPhysics();

		pPl->GetPlayerBall()->SetPos(pos);
	}
	else
		pPl->SetPlayerBall(CreateBall(pos, pPl));

	pPl->GetPlayerBall()->SaveBallCannonSettings();
	pPl->m_Shared.SetStamina(100);
}

static ConCommand createplayerball("createplayerball", CC_CreatePlayerBall);

void CC_ShootPlayerBall(const CCommand &args)
{
	CSDKPlayer *pPl = ToSDKPlayer(UTIL_GetCommandClient());
	if (!CSDKPlayer::IsOnField(pPl) || !pPl->GetPlayerBall())
		return;

	pPl->GetPlayerBall()->RestoreBallCannonSettings();
	pPl->m_Shared.SetStamina(100);
}

static ConCommand shootplayerball("shootplayerball", CC_ShootPlayerBall);

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
	SendPropEHandle(SENDINFO(m_pCurrentPlayer)),
	SendPropInt(SENDINFO(m_nCurrentTeam)),
	SendPropBool(SENDINFO(m_bIsPlayerBall)),
	SendPropInt(SENDINFO(m_eBallState)),
	SendPropInt(SENDINFO(m_bNonnormalshotsBlocked)),
	SendPropInt(SENDINFO(m_bShotsBlocked)),
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
	m_flStateLeaveTime = gpGlobals->curtime;
	m_flStateActivationDelay = 0;
	m_flStateTimelimit = -1;
	m_pPl = NULL;
	m_pOtherPl = NULL;
	m_pCurrentPlayer = NULL;
	m_ePenaltyState = PENALTY_NONE;
	m_bSetNewPos = false;
	m_bSetNewVel = false;
	m_bSetNewRot = false;
	m_bHasQueuedState = false;
	m_pHoldingPlayer = NULL;
	m_pPossessingPl = NULL;
	m_nPossessingTeam = TEAM_INVALID;
	m_flPossessionStart = -1;
	m_flLastMatchEventSetTime = -1;
	m_pScorer = NULL;
	m_pFirstAssister = NULL;
	m_pSecondAssister = NULL;
	m_bNonnormalshotsBlocked = false;
	m_bShotsBlocked = false;
	m_bHitThePost = false;
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

	PrecacheScriptSound("Ball.Kicknormal");
	PrecacheScriptSound("Ball.Kickhard");
	PrecacheScriptSound("Ball.Touch");
	PrecacheScriptSound("Ball.Post");
	PrecacheScriptSound("Ball.Net");
	PrecacheScriptSound("Ball.Whistle");
	PrecacheScriptSound("Crowd.Background1");
	PrecacheScriptSound("Crowd.Background2");
	PrecacheScriptSound("Crowd.Cheer");
	PrecacheScriptSound("Crowd.EndOfPeriod");
	PrecacheScriptSound("Crowd.Goal1");
	PrecacheScriptSound("Crowd.Goal2");
	PrecacheScriptSound("Crowd.Goal3");
	PrecacheScriptSound("Crowd.Save");
	PrecacheScriptSound("Crowd.Miss");
	PrecacheScriptSound("Crowd.Foul");
	PrecacheScriptSound("Crowd.YellowCard");
	PrecacheScriptSound("Crowd.RedCard");
	PrecacheScriptSound("Crowd.Song");
	PrecacheScriptSound("Crowd.Vuvuzela");
	PrecacheScriptSound("Crowd.Way");
	PrecacheScriptSound("Crowd.Easy");

	State_Transition(BALL_STATE_NORMAL);
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
	if (m_bIsBallCannonMode && m_bRestoreBallCannonSettings)
	{
		m_pPhys->SetPosition(m_vBallCannonPos, m_aBallCannonAng, true);
		m_pPhys->SetVelocity(&m_vBallCannonVel, &m_vBallCannonRot);
		m_bRestoreBallCannonSettings = false;
	}
	else
	{
		Vector vel, worldAngImp;
		AngularImpulse angImp;
		m_pPhys->GetVelocity(&vel, &angImp);
		VectorRotate(angImp, EntityToWorldTransform(), worldAngImp);
		Vector magnusDir = worldAngImp.Cross(vel);

		if (vel.Length() > 0)
			vel += magnusDir * 1e-6 * sv_ball_curve.GetFloat() * gpGlobals->frametime;

		VPhysicsGetObject()->SetVelocity(&vel, &angImp);
	}

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
		if (pLastPl && Sign(m_vPos.y - SDKGameRules()->m_vKickOff.GetY()) == pLastPl->GetTeam()->m_nForward) // Check if it's the opponent's goal
		{
			m_bHitThePost = true;
			//pLastPl->AddShot();
			//pLastPl->AddShotOnGoal();
		}

		EmitSound("Ball.Post");
	}
	else
	{
		//if ball is moving fast when we hit something play a sound
		if (flSpeed > 500.0f)
		{
			EmitSound("Ball.Touch");
		}
	}
	
	//iosgoalnets 82=iosgoalnets, 30=concrete!!! TEMP!!! until pricey changes nets surfaceprop!
	if ((surfaceProps == 82 /*|| surfaceProps == 30*/) && flSpeed > 300.0f)
	{
		EmitSound("Ball.Net");
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

		if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES && m_ePenaltyState == PENALTY_KICKED && pPl != m_pPl)
		{
			m_ePenaltyState = PENALTY_SAVED;
		}
		else if (m_pCurStateInfo->m_eBallState == BALL_STATE_NORMAL)
		{
			Touched(pPl, false, BODY_PART_UNKNOWN, preVelocity);
		}

		EmitSound("Ball.Touch");
	}

	//Warning ("surfaceprops index %d\n", surfaceProps);

	BaseClass::VPhysicsCollision( index, pEvent );
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
	Vector oldVel = m_vVel;

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
	{
		SetRot(CalcSpin(spinCoeff, (bodyPart != BODY_PART_HEAD)));
	}

	if (m_bIsPlayerBall && !m_bIsBallCannonMode && m_pPl == GetCreator())
	{
		SaveBallCannonSettings();
	}

	Kicked(bodyPart, isDeflection, oldVel);
	
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

void CBall::SendNotifications()
{
	if (m_eNextState == BALL_STATE_GOAL)
	{
		CSDKPlayer *pKeeper = NULL;
		bool isOwnGoal;

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
			IGameEvent *pEvent = gameeventmanager->CreateEvent("own_goal");
			if (pEvent)
			{
				pEvent->SetInt("causing_team", LastTeam(true, pKeeper));
				pEvent->SetInt("causer_userid", LastPl(true, pKeeper) ? LastPl(true, pKeeper)->GetUserID() : 0);
				gameeventmanager->FireEvent(pEvent);
			}
		}
		else
		{
			IGameEvent *pEvent = gameeventmanager->CreateEvent("goal");
			if (pEvent)
			{
				pEvent->SetInt("scoring_team", LastTeam(true));
				pEvent->SetInt("scorer_userid", m_pScorer ? m_pScorer->GetUserID() : 0);
				pEvent->SetInt("first_assister_userid", m_pFirstAssister ? m_pFirstAssister->GetUserID() : 0);
				pEvent->SetInt("second_assister_userid", m_pSecondAssister ? m_pSecondAssister->GetUserID() : 0);
				gameeventmanager->FireEvent(pEvent);
			}
		}

		EmitSound("Ball.Whistle");
		EmitSound("Crowd.Goal1");
		EmitSound("Crowd.Goal2");
		EmitSound("Crowd.Goal3");
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
					EmitSound("Crowd.Foul");
					break;
				case FOUL_NORMAL_YELLOW_CARD:
					if (m_pFoulingPl->GetYellowCards() % 2 == 0)
						foulType = MATCH_EVENT_SECONDYELLOWCARD;
					else
						foulType = MATCH_EVENT_YELLOWCARD;
					EmitSound("Crowd.YellowCard");
					break;
				case FOUL_NORMAL_RED_CARD:
					foulType = MATCH_EVENT_REDCARD;
					EmitSound("Crowd.RedCard");
					break;
				case FOUL_OFFSIDE:
					foulType = MATCH_EVENT_OFFSIDE;
					SDKGameRules()->SetOffsideLinesEnabled(true);
					EmitSound("Crowd.Foul");
					break;
				case FOUL_DOUBLETOUCH:
					foulType = MATCH_EVENT_DOUBLETOUCH;
					EmitSound("Crowd.Foul");
					break;
				default:
					foulType = MATCH_EVENT_FOUL;
					EmitSound("Crowd.Foul");
					break;
				}

				IGameEvent *pEvent = gameeventmanager->CreateEvent("foul");
				if (pEvent)
				{
					pEvent->SetInt("fouling_player_userid", (m_pFoulingPl ? m_pFoulingPl->GetUserID() : 0));
					pEvent->SetInt("fouled_player_userid", (m_pFouledPl ? m_pFouledPl->GetUserID() : 0));
					pEvent->SetInt("fouling_team", m_nFoulingTeam);
					pEvent->SetInt("foul_type", foulType);
					pEvent->SetInt("set_piece_type", (m_eNextState == BALL_STATE_PENALTY ? MATCH_EVENT_PENALTY : MATCH_EVENT_FREEKICK));

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

void CBall::CheckTimeout()
{
	if (SDKGameRules()->AdminWantsTimeout() || GetGlobalTeam(TEAM_A)->WantsTimeout() || GetGlobalTeam(TEAM_B)->WantsTimeout())
	{
		int timeoutTeam = TEAM_UNASSIGNED;

		if (SDKGameRules()->AdminWantsTimeout())
		{
			timeoutTeam = TEAM_UNASSIGNED;
			SDKGameRules()->SetAdminWantsTimeout(false);
		}

		if (GetGlobalTeam(TEAM_A)->WantsTimeout())
		{
			timeoutTeam = TEAM_A;
			GetGlobalTeam(TEAM_A)->SetWantsTimeout(false);
		}

		if (GetGlobalTeam(TEAM_B)->WantsTimeout())
		{
			timeoutTeam = TEAM_B;
			GetGlobalTeam(TEAM_B)->SetWantsTimeout(false);
		}

		SDKGameRules()->SetTimeoutEnd(timeoutTeam == TEAM_UNASSIGNED ? -1 : gpGlobals->curtime + mp_timeout_duration.GetFloat());

		IGameEvent *pEvent = gameeventmanager->CreateEvent("start_timeout");
		if (pEvent)
		{
			pEvent->SetInt("requesting_team", timeoutTeam);
			gameeventmanager->FireEvent(pEvent);
		}
	}

	if (SDKGameRules()->GetTimeoutEnd() == -1 || gpGlobals->curtime < SDKGameRules()->GetTimeoutEnd())
	{
		m_flStateTimelimit = -1;
		return;
	}
	else if (SDKGameRules()->GetTimeoutEnd() > 0)
	{
		SDKGameRules()->SetTimeoutEnd(0);

		IGameEvent *pEvent = gameeventmanager->CreateEvent("end_timeout");
		if (pEvent)
		{
			gameeventmanager->FireEvent(pEvent);
		}
		UTIL_ClientPrintAll(HUD_PRINTTALK, "#game_match_start");
	}
}

ConVar mp_showballstatetransitions( "mp_showballstatetransitions", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show ball state transitions." );

void CBall::State_Transition(ball_state_t newState, float delay /*= 0.0f*/, bool cancelQueuedState /*= false*/, bool isShortMessageDelay /*= false*/)
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

void CBall::State_Enter(ball_state_t newState, bool cancelQueuedState)
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

	if (!m_bIsPlayerBall)
	{
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
			CheckTimeout();
		}

		if (m_pCurStateInfo
			&& State_Get() != BALL_STATE_NORMAL
			&& m_eNextState == BALL_STATE_NONE
			&& CSDKPlayer::IsOnField(m_pPl)
			&& m_flStateTimelimit != -1
			&& gpGlobals->curtime >= m_flStateTimelimit) // Player is afk or timed out
		{
			m_pPl->SetDesiredTeam(TEAM_SPECTATOR, m_pPl->GetTeamNumber(), 0, true, false);
		}
	}

	if (m_pCurStateInfo && m_pCurStateInfo->pfnThink)
	{	
		(this->*m_pCurStateInfo->pfnThink)();
	}

	if (m_pPl) // Set info for the client
	{
		m_pCurrentPlayer = m_pPl;
		m_nCurrentTeam = m_pPl->GetTeamNumber();
	}
}

CBallStateInfo* CBall::State_LookupInfo( ball_state_t state )
{
	static CBallStateInfo ballStateInfos[] =
	{
		{ BALL_STATE_STATIC,		"BALL_STATE_STATIC",		&CBall::State_STATIC_Enter,			&CBall::State_STATIC_Think,			&CBall::State_STATIC_Leave },
		{ BALL_STATE_NORMAL,		"BALL_STATE_NORMAL",		&CBall::State_NORMAL_Enter,			&CBall::State_NORMAL_Think,			&CBall::State_NORMAL_Leave },
		{ BALL_STATE_KICKOFF,		"BALL_STATE_KICKOFF",		&CBall::State_KICKOFF_Enter,		&CBall::State_KICKOFF_Think,		&CBall::State_KICKOFF_Leave },
		{ BALL_STATE_THROWIN,		"BALL_STATE_THROWIN",		&CBall::State_THROWIN_Enter,		&CBall::State_THROWIN_Think,		&CBall::State_THROWIN_Leave },
		{ BALL_STATE_GOALKICK,		"BALL_STATE_GOALKICK",	&CBall::State_GOALKICK_Enter,		&CBall::State_GOALKICK_Think,		&CBall::State_GOALKICK_Leave },
		{ BALL_STATE_CORNER,		"BALL_STATE_CORNER",		&CBall::State_CORNER_Enter,			&CBall::State_CORNER_Think,			&CBall::State_CORNER_Leave },
		{ BALL_STATE_GOAL,			"BALL_STATE_GOAL",		&CBall::State_GOAL_Enter,			&CBall::State_GOAL_Think,			&CBall::State_GOAL_Leave },
		{ BALL_STATE_FREEKICK,		"BALL_STATE_FREEKICK",	&CBall::State_FREEKICK_Enter,		&CBall::State_FREEKICK_Think,		&CBall::State_FREEKICK_Leave },
		{ BALL_STATE_PENALTY,		"BALL_STATE_PENALTY",		&CBall::State_PENALTY_Enter,		&CBall::State_PENALTY_Think,		&CBall::State_PENALTY_Leave },
		{ BALL_STATE_KEEPERHANDS,	"BALL_STATE_KEEPERHANDS",	&CBall::State_KEEPERHANDS_Enter,	&CBall::State_KEEPERHANDS_Think,	&CBall::State_KEEPERHANDS_Leave },
	};

	for ( int i=0; i < ARRAYSIZE( ballStateInfos ); i++ )
	{
		if ( ballStateInfos[i].m_eBallState == state )
			return &ballStateInfos[i];
	}

	return NULL;
}

void CBall::FindStatePlayer(ball_state_t ballState /*= BALL_STATE_NONE*/)
{
	if (ballState == BALL_STATE_NONE)
		ballState = State_Get();

	switch (ballState)
	{
	case BALL_STATE_THROWIN:
		FindNearestPlayer(LastOppTeam(false));
		break;
	case BALL_STATE_GOALKICK:
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
	if (m_eNextState == BALL_STATE_GOAL)
		return;

	for (int ignoredPlayerBits = 0;;)
	{
		if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
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
			break;

		if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
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

	if (!CSDKPlayer::IsOnField(m_pOtherPl) || m_pOtherPl == m_pPl)
	{
		m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_ATTACKER, false, (1 << (m_pPl->entindex() - 1)));
		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_MIDFIELDER, false, (1 << (m_pPl->entindex() - 1)));
		if (!m_pOtherPl)
			m_pOtherPl = FindNearestPlayer(m_pPl->GetTeamNumber(), FL_POS_DEFENDER, false, (1 << (m_pPl->entindex() - 1)));

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
		SetVel(m_vPlForward2D * 350, 0, BODY_PART_FEET, false, false, false);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		if (m_pOtherPl)
			m_pOtherPl->RemoveFlag(FL_ATCONTROLS);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CBall::State_KICKOFF_Leave(ball_state_t newState)
{
	m_bShotsBlocked = false;
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
		SetVel(vel, 0, BODY_PART_HANDS, false, false, false);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CBall::State_THROWIN_Leave(ball_state_t newState)
{
	if (CSDKPlayer::IsOnField(m_pPl))
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_THROW);

	EnablePlayerCollisions(true);
	m_bShotsBlocked = false;
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
			return State_Transition(BALL_STATE_NORMAL);

		SDKGameRules()->EnableShield(SHIELD_GOALKICK, m_pPl->GetTeamNumber(), m_vPos);
		m_pPl->SetPosInsideShield(Vector(m_vPos.x, m_vPos.y - 100 * m_pPl->GetTeam()->m_nForward, SDKGameRules()->m_vKickOff.GetZ()), true);
		m_flStateTimelimit = -1;
		m_bShotsBlocked = true;
	}

	if (!PlayersAtTargetPos())
		return;

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_pPl->SetShotButtonsReleased(false);
		m_bShotsBlocked = false;
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased() && m_pPl->IsShooting() && CanTouchBallXY())
	{
		m_pPl->AddGoalKick();
		RemoveAllTouches();
		DoGroundShot(false, sv_ball_goalkick_speedcoeff.GetFloat());
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CBall::State_GOALKICK_Leave(ball_state_t newState)
{
	m_bShotsBlocked = false;
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
		if (sv_ball_assign_setpieces.GetBool())
		{
			if (m_vPos.x == GetGlobalTeam(LastTeam(false))->m_vCornerRight.GetX())
				m_pPl = GetGlobalTeam(LastOppTeam(false))->GetLeftCornerTaker();
			else
				m_pPl = GetGlobalTeam(LastOppTeam(false))->GetRightCornerTaker();
		}
		else
			m_pPl = NULL;
		
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

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_bShotsBlocked = false;
		m_bNonnormalshotsBlocked = true;
	}

	if (m_bNonnormalshotsBlocked && gpGlobals->curtime - m_flStateEnterTime > sv_ball_nonnormalshotsblocktime_corner.GetFloat())
	{
		m_bNonnormalshotsBlocked = false;
		m_pPl->SetShotButtonsReleased(false);
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased()
		&& CanTouchBallXY()
		&& (!m_bNonnormalshotsBlocked && m_pPl->IsShooting() || m_bNonnormalshotsBlocked && (m_pPl->IsNormalshooting() || !sv_ball_blockpowershot.GetBool() && m_pPl->IsPowershooting())))
	{
		EmitSound("Crowd.Way");
		m_pPl->AddCorner();
		RemoveAllTouches();
		DoGroundShot(false);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CBall::State_CORNER_Leave(ball_state_t newState)
{
	m_bNonnormalshotsBlocked = false;
	m_bShotsBlocked = false;
}

void CBall::State_GOAL_Enter()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (pPl->GetTeamNumber() != m_nTeam)
			pPl->AddFlag(FL_CELEB);
	}

	float delay = sv_ball_goalcelebduration.GetFloat();

	if (sv_replays.GetBool())
	{
		delay += sv_replay_duration1.GetFloat();

		if (sv_replay_count.GetInt() >= 2)
			delay += sv_replay_duration2.GetFloat();

		if (sv_replay_count.GetInt() >= 3)
			delay += sv_replay_duration3.GetFloat();
	}

	State_Transition(BALL_STATE_KICKOFF, delay);

	ReplayManager()->StartReplay(false);
}

void CBall::State_GOAL_Think()
{
}

void CBall::State_GOAL_Leave(ball_state_t newState)
{
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
		if ((m_vPos - GetGlobalTeam(m_nFouledTeam)->m_vGoalCenter).Length2D() <= sv_ball_freekickdist_owngoal.GetInt()) // Close to own goal
			m_pPl = FindNearestPlayer(m_nFouledTeam, FL_POS_KEEPER);
		else if (abs(m_vPos.y - GetGlobalTeam(m_nFoulingTeam)->m_vGoalCenter.GetY()) <= sv_ball_freekickdist_opponentgoal.GetInt()) // Close to opponent's goal
		{
			if (sv_ball_assign_setpieces.GetBool())
			{
				Vector2D dirToPl = (m_vPos - GetGlobalTeam(m_nFoulingTeam)->m_vGoalCenter).AsVector2D();
				dirToPl.NormalizeInPlace();
				Vector2D yDir = Vector2D(0, GetGlobalTeam(m_nFoulingTeam)->m_nForward);

				if (RAD2DEG(acos(DotProduct2D(yDir, dirToPl))) <= sv_ball_freekickangle_opponentgoal.GetInt())
				{
					m_pPl = GetGlobalTeam(m_nFouledTeam)->GetFreekickTaker();
				}
				else
				{
					if (abs(GetGlobalTeam(m_nFoulingTeam)->m_vCornerRight.GetX() - m_vPos.x) < abs(GetGlobalTeam(m_nFoulingTeam)->m_vCornerLeft.GetX() - m_vPos.x))
						m_pPl = GetGlobalTeam(m_nFouledTeam)->GetLeftCornerTaker();
					else
						m_pPl = GetGlobalTeam(m_nFouledTeam)->GetRightCornerTaker();
				}
			}

			if (!CSDKPlayer::IsOnField(m_pPl))
				m_pPl = m_pFouledPl;
		}
		else
			m_pPl = NULL;

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

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->RemoveFlag(FL_ATCONTROLS);
		m_bShotsBlocked = false;

		if (abs(m_vPos.y - GetGlobalTeam(m_nFoulingTeam)->m_vGoalCenter.GetY()) <= sv_ball_freekickdist_opponentgoal.GetInt())
			m_bNonnormalshotsBlocked = true;
	}

	if (m_bNonnormalshotsBlocked && gpGlobals->curtime - m_flStateEnterTime > sv_ball_nonnormalshotsblocktime_freekick.GetFloat())
	{
		m_bNonnormalshotsBlocked = false;
		m_pPl->SetShotButtonsReleased(false);
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased()
		&& CanTouchBallXY()
		&& (!m_bNonnormalshotsBlocked && m_pPl->IsShooting() || m_bNonnormalshotsBlocked && (m_pPl->IsNormalshooting() || !sv_ball_blockpowershot.GetBool() && m_pPl->IsPowershooting())))
	{
		EmitSound("Crowd.Way");
		m_pPl->AddFreeKick();
		RemoveAllTouches();
		DoGroundShot(true, sv_ball_freekick_speedcoeff.GetFloat());
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CBall::State_FREEKICK_Leave(ball_state_t newState)
{
	SDKGameRules()->SetOffsideLinesEnabled(false);
	m_bNonnormalshotsBlocked = false;
	m_bShotsBlocked = false;
}

void CBall::State_PENALTY_Enter()
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

	m_bPenaltyTakerStartedMoving = false;
}

void CBall::State_PENALTY_Think()
{
	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		{
			m_pPl = m_pFouledPl;
			if (!CSDKPlayer::IsOnField(m_pPl))
			{
				m_ePenaltyState = PENALTY_ABORTED_NO_TAKER;
				return State_Transition(BALL_STATE_NORMAL);
			}
		}
		else
		{
			if (sv_ball_assign_setpieces.GetBool())
				m_pPl = GetGlobalTeam(m_nFouledTeam)->GetPenaltyTaker();
			else
				m_pPl = NULL;

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
	}

	if (!CSDKPlayer::IsOnField(m_pOtherPl))
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
		m_pOtherPl->SetPosInsideShield(pos, true);
	}

	if (!PlayersAtTargetPos())
		return;

	if (m_flStateTimelimit == -1)
	{
		m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		m_pPl->RemoveFlag(FL_ATCONTROLS);
	}

	if (m_bShotsBlocked && gpGlobals->curtime - m_flStateEnterTime > sv_ball_shotsblocktime_penalty.GetFloat())
	{
		m_bShotsBlocked = false;
		m_pPl->SetShotButtonsReleased(false);
	}

	UpdateCarrier();

	if (m_pPl->ShotButtonsReleased() && !m_bShotsBlocked && m_pPl->IsShooting() && CanTouchBallXY())
	{
		m_pPl->AddPenalty();
		RemoveAllTouches();
		m_ePenaltyState = PENALTY_KICKED;
		m_pPl->m_ePenaltyState = PENALTY_KICKED;
		DoGroundShot(false);
		State_Transition(BALL_STATE_NORMAL);
	}
}

void CBall::State_PENALTY_Leave(ball_state_t newState)
{
	if (CSDKPlayer::IsOnField(m_pOtherPl))
	{
		m_pOtherPl->RemoveFlag(FL_ATCONTROLS);
	}

	m_bShotsBlocked = false;
}

void CBall::State_KEEPERHANDS_Enter()
{
	SetPos(m_vPos);
	// Don't ignore triggers when setting the new ball position
	m_bSetNewPos = false;
}

void CBall::State_KEEPERHANDS_Think()
{
	if (m_eNextState == BALL_STATE_GOAL)
		return;

	if (!CSDKPlayer::IsOnField(m_pPl))
	{
		m_pPl = FindNearestPlayer(m_nInPenBoxOfTeam, FL_POS_KEEPER);
		if (!m_pPl)
		{
			return State_Transition(BALL_STATE_NORMAL);
		}

		if (!SDKGameRules()->IsIntermissionState() && !m_bHasQueuedState && SDKGameRules()->State_Get() != MATCH_PERIOD_PENALTIES)
		{
			SDKGameRules()->EnableShield(SHIELD_KEEPERHANDS, m_pPl->GetTeamNumber(), m_vPos);
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
		PlayersAtTargetPos();
	}

	if (!SDKGameRules()->IsIntermissionState() && !m_bHasQueuedState && SDKGameRules()->State_Get() != MATCH_PERIOD_PENALTIES)
	{
		if (m_flStateTimelimit == -1)
		{
			m_flStateTimelimit = gpGlobals->curtime + sv_ball_timelimit_setpiece.GetFloat();
		}
	}

	UpdateCarrier();

	SetPos(Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_chest_start.GetFloat()) + m_vPlForward2D * 18);

	// Don't ignore triggers when setting the new ball position
	m_bSetNewPos = false;

	Vector min = GetGlobalTeam(m_pPl->GetTeamNumber())->m_vPenBoxMin + m_flPhysRadius;
	Vector max = GetGlobalTeam(m_pPl->GetTeamNumber())->m_vPenBoxMax - m_flPhysRadius;

	// Ball outside the penalty box
	if (m_vPos.x < min.x || m_vPos.y < min.y || m_vPos.x > max.x || m_vPos.y > max.y)
	{
		Vector dir, pos;
		float vel;

		// Throw the ball towards the kick-off spot instead of where the player is looking if the ball is behind the goal line
		if (m_pPl->GetTeam()->m_nForward == 1 && m_vPos.y < min.y || m_pPl->GetTeam()->m_nForward == -1 && m_vPos.y > max.y)
		{
			QAngle ang = QAngle(g_IOSRand.RandomFloat(-55, -40), m_pPl->GetTeam()->m_nForward * 90 - m_pPl->GetTeam()->m_nForward * Sign(m_vPos.x - SDKGameRules()->m_vKickOff.GetX()) * g_IOSRand.RandomFloat(15, 25), 0);
			AngleVectors(ang, &dir);
			vel = g_IOSRand.RandomFloat(700, 800);
			pos = Vector(m_vPos.x, (m_pPl->GetTeam()->m_nForward == 1 ? min.y : max.y) + m_pPl->GetTeam()->m_nForward * 36, m_vPos.z);
		}
		else
		{
			dir = m_vPlForward2D;
			vel = 300;
			pos = Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_chest_start.GetFloat()) + m_vPlForward2D * 36;
		}

		RemoveAllTouches();
		SetPos(pos);
		m_bSetNewPos = false;
		SetVel(dir * vel, 0, BODY_PART_KEEPERHANDS, false, true, true);

		return State_Transition(BALL_STATE_NORMAL);
	}

	Vector vel;

	if (m_pPl->ShotButtonsReleased() && (m_pPl->IsPowershooting() || m_pPl->IsChargedshooting()) && m_pPl->m_flNextShot <= gpGlobals->curtime)
	{
		float spin;

		if (m_pPl->IsPowershooting())
		{
			vel = m_vPlForward * GetNormalshotStrength(GetPitchCoeff(true), sv_ball_normalshot_strength.GetInt());
			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KEEPER_HANDS_THROW);
			spin = 0;
		}
		else
		{
			QAngle ang = m_aPlAng;
			ang[PITCH] = min(sv_ball_keepershot_minangle.GetFloat(), m_aPlAng[PITCH]);
			Vector dir;
			AngleVectors(ang, &dir);
			vel = dir * GetChargedshotStrength(GetPitchCoeff(false), sv_ball_chargedshot_minstrength.GetInt(), sv_ball_chargedshot_maxstrength.GetInt());
			m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_KEEPER_HANDS_KICK);
			spin = sv_ball_volleyshot_spincoeff.GetFloat();

			if (vel.Length() > 1000)
				EmitSound("Ball.Kickhard");
			else
				EmitSound("Ball.Kicknormal");
		}

		RemoveAllTouches();
		SetPos(Vector(m_vPlPos.x, m_vPlPos.y, m_vPlPos.z + sv_ball_bodypos_chest_start.GetFloat()) + m_vPlForward2D * 36);
		m_bSetNewPos = false;
		SetVel(vel, spin, BODY_PART_KEEPERHANDS, false, true, true);

		return State_Transition(BALL_STATE_NORMAL);
	}
}

void CBall::State_KEEPERHANDS_Leave(ball_state_t newState)
{
	RemoveFromPlayerHands(m_pPl);
}

// Make sure that all players are walked to the intended positions when setting shields
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
				if (mp_shield_liberal_teammates_positioning.GetBool() && m_pCurStateInfo->m_eBallState != BALL_STATE_KICKOFF && m_pCurStateInfo->m_eBallState != BALL_STATE_PENALTY && pPl->GetTeamNumber() == m_pPl->GetTeamNumber())
					pPl->SetPosOutsideBall(pPl->GetLocalOrigin());
				else
					pPl->SetPosOutsideShield(false);
			}

			if (!pPl->m_bIsAtTargetPos)
			{
				if (pPl->m_flRemoteControlledStartTime == -1)
				{
					pPl->m_flRemoteControlledStartTime = gpGlobals->curtime;
					playersAtTarget = false;
				}
				else if (gpGlobals->curtime >= pPl->m_flRemoteControlledStartTime + sv_ball_timelimit_remotecontrolled.GetFloat()) // Player timed out and blocks progress, so move him to specs
					pPl->SetDesiredTeam(TEAM_SPECTATOR, pPl->GetTeamNumber(), 0, true, false);
				else
					playersAtTarget = false;
			}
		}
	}

	return playersAtTarget;
}

bool CBall::CanTouchBallXY()
{
	Vector circleCenter = m_vPlPos + m_vPlForward2D * sv_ball_standing_shift.GetFloat();
	Vector dirToBall = m_vPos - circleCenter;
	if (dirToBall.Length2DSqr() <= pow(sv_ball_standing_reach.GetFloat(), 2))
	{
		dirToBall.NormalizeInPlace();

		if (RAD2DEG(acos(m_vPlForward2D.Dot(dirToBall))) <= sv_ball_standing_cone.GetFloat() / 2)
			return true;
	}

	return false;
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

		//if (plPos.x < SDKGameRules()->m_vFieldMin.GetX() || plPos.y < SDKGameRules()->m_vFieldMin.GetY() ||
		//	plPos.x > SDKGameRules()->m_vFieldMax.GetX() || plPos.y > SDKGameRules()->m_vFieldMax.GetY())
		//	continue;

		Vector dirToPl = pPl->GetLocalOrigin() - m_vPlPos;
		float distToPl = dirToPl.Length2D();

		Vector localDirToPl;
		VectorIRotate(dirToPl, m_pPl->EntityToWorldTransform(), localDirToPl);

		dirToPl.z = 0;
		dirToPl.NormalizeInPlace();
		if (localDirToPl.x < 0 || localDirToPl.x > sv_ball_slideforwardreach_foul.GetInt() || abs(localDirToPl.y) > sv_ball_slidesidereach_foul.GetInt())		
			continue;

		if (distToPl >= (m_vPos - m_vPlPos).Length2D())
			continue;

		// It's a foul

		PlayerAnimEvent_t anim = RAD2DEG(acos(m_vPlForward2D.Dot(pPl->EyeDirection2D()))) <= 90 ? PLAYERANIMEVENT_TACKLED_BACKWARD : PLAYERANIMEVENT_TACKLED_FORWARD;

		pPl->DoAnimationEvent(anim);

		int teammatesCloserToGoalCount = 0;

		bool isCloseToOwnGoal = ((m_vPos - m_pPl->GetTeam()->m_vGoalCenter).Length2D() <= sv_ball_closetogoaldist.GetInt());

		if (isCloseToOwnGoal)
		{
			for (int j = 1; j <= gpGlobals->maxClients; j++) 
			{
				CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(j));

				if (!CSDKPlayer::IsOnField(pPl) || pPl == m_pPl || pPl->GetTeamNumber() != m_pPl->GetTeamNumber())
					continue;

				if ((m_pPl->GetTeam()->m_vGoalCenter - pPl->GetLocalOrigin()).Length2DSqr() < (m_pPl->GetTeam()->m_vGoalCenter - m_vPlPos).Length2DSqr())
					teammatesCloserToGoalCount += 1;
			}
		}


		float proximity = clamp(1 - abs(localDirToPl.y) / sv_ball_slidesidereach_foul.GetFloat(), 0.0f, 1.0f);

		foul_type_t foulType;

		if (isCloseToOwnGoal && teammatesCloserToGoalCount <= 1)
			foulType = FOUL_NORMAL_RED_CARD;
		else if (anim == PLAYERANIMEVENT_TACKLED_FORWARD && proximity >= sv_ball_yellowcardproximity_forward.GetFloat() ||
				 anim == PLAYERANIMEVENT_TACKLED_BACKWARD && proximity >= sv_ball_yellowcardproximity_backward.GetFloat())
			foulType = FOUL_NORMAL_YELLOW_CARD;
		else
			foulType = FOUL_NORMAL_NO_CARD;

		TriggerFoul(foulType, pPl->GetLocalOrigin(), m_pPl, pPl);
		m_pFoulingPl->AddFoul();
		m_pFouledPl->AddFoulSuffered();

		if (m_eFoulType == FOUL_NORMAL_YELLOW_CARD)
			m_pFoulingPl->AddYellowCard();

		if (m_eFoulType == FOUL_NORMAL_YELLOW_CARD && m_pFoulingPl->GetYellowCards() % 2 == 0 || m_eFoulType == FOUL_NORMAL_RED_CARD)
		{
			m_pFoulingPl->AddRedCard();
			int banDuration = 60 * (m_eFoulType == FOUL_NORMAL_YELLOW_CARD ? sv_ball_player_yellow_red_card_duration.GetFloat() : sv_ball_player_red_card_duration.GetFloat());
			int nextJoin = SDKGameRules()->GetMatchDisplayTimeSeconds(false) + banDuration;
			m_pFoulingPl->SetNextCardJoin(nextJoin);

			if (m_pFoulingPl->GetTeamPosType() != POS_GK)
				m_pFoulingPl->GetTeam()->SetPosNextJoinSeconds(m_pFoulingPl->GetTeamPosIndex(), nextJoin);

			ReplayManager()->AddMatchEvent(m_eFoulType == FOUL_NORMAL_YELLOW_CARD ? MATCH_EVENT_SECONDYELLOWCARD : MATCH_EVENT_REDCARD, m_nFoulingTeam, m_pFoulingPl);
		}
		else if (m_eFoulType == FOUL_NORMAL_YELLOW_CARD)
		{
			ReplayManager()->AddMatchEvent(MATCH_EVENT_YELLOWCARD, m_nFoulingTeam, m_pFoulingPl);
		}

		if (pPl->m_nInPenBoxOfTeam == m_pPl->GetTeamNumber())
			State_Transition(BALL_STATE_PENALTY, sv_ball_statetransition_activationdelay_long.GetFloat());
		else
			State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_activationdelay_long.GetFloat());

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
	m_nFouledTeam = pFoulingPl->GetOppTeamNumber();
	m_vFoulPos.x = clamp(pos.x, SDKGameRules()->m_vFieldMin.GetX() + 2 * m_flPhysRadius, SDKGameRules()->m_vFieldMax.GetX() - 2 * m_flPhysRadius);
	m_vFoulPos.y = clamp(pos.y, SDKGameRules()->m_vFieldMin.GetY() + 2 * m_flPhysRadius, SDKGameRules()->m_vFieldMax.GetY() - 2 * m_flPhysRadius);
	m_vFoulPos.z = SDKGameRules()->m_vKickOff.GetZ();

	// Move the ball to the edge of the penalty box if the foul happened inside. This will probably only be relevant for double touch fouls.

	Vector min = GetGlobalTeam(m_nFoulingTeam)->m_vPenBoxMin - m_flPhysRadius;
	Vector max = GetGlobalTeam(m_nFoulingTeam)->m_vPenBoxMax + m_flPhysRadius;

	// Ball inside the penalty box
	if (m_vFoulPos.x > min.x && m_vFoulPos.x < max.x)
	{
		if (GetGlobalTeam(m_nFoulingTeam)->m_nForward == 1 && m_vFoulPos.y < max.y)
			m_vFoulPos.y = max.y;
		else if (GetGlobalTeam(m_nFoulingTeam)->m_nForward == -1 && m_vFoulPos.y > min.y)
			m_vFoulPos.y = min.y;
	}
}

void CBall::HandleFoul()
{
	if (CSDKPlayer::IsOnField(m_pFoulingPl))
	{
		if (m_eFoulType == FOUL_NORMAL_YELLOW_CARD && m_pFoulingPl->GetYellowCards() % 2 == 0 || m_eFoulType == FOUL_NORMAL_RED_CARD)
		{
			int team = m_pFoulingPl->GetTeamNumber();
			int posIndex = m_pFoulingPl->GetTeamPosIndex();
			int posType = m_pFoulingPl->GetTeamPosType();
			m_pFoulingPl->SetDesiredTeam(TEAM_SPECTATOR, m_pFoulingPl->GetTeamNumber(), 0, true, false);

			if (posType == POS_GK)
			{
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
					if (!CSDKPlayer::IsOnField(pPl) || pPl == m_pFoulingPl || pPl->GetTeamNumber() != team)
						continue;

					pPl->SetDesiredTeam(team, team, posIndex, true, false);
					break;
				}
			}
		}
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

	if (m_pPl->IsNormalshooting()
		&& m_pPl->GetTeamPosType() == POS_GK
		&& m_nInPenBoxOfTeam == m_pPl->GetTeamNumber()
		&& !m_pPl->m_pHoldingBall
		&& m_pPl->m_nInPenBoxOfTeam == m_pPl->GetTeamNumber())
	{
		if (SDKGameRules()->IsIntermissionState() || SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		{
			canCatch = true;
		}
		else
		{
			BallTouchInfo *pLastTouch = LastInfo(false, m_pPl);
			BallTouchInfo *pLastShot = LastInfo(true, m_pPl);

			if (pLastTouch && pLastShot)
			{
				// Can catch if opponent has touched or shot the ball
				if (pLastTouch->m_nTeam != m_pPl->GetTeamNumber() || pLastShot->m_nTeam != m_pPl->GetTeamNumber())
				{
					canCatch = true;
				}
				else
				{
					if (pLastShot->m_eBodyPart == BODY_PART_HEAD || pLastShot->m_eBodyPart == BODY_PART_CHEST)
					{
						// Check if any opponent has touched the ball since the last set piece. Only allow back-passes with the head or chest if this is true to prevent abuse.
						for (int i = m_Touches.Count() - 1; i >= 0; i--)
						{
							if (m_Touches[i]->m_nTeam != m_pPl->GetTeamNumber())
							{
								canCatch = true;
								break;
							}
						}
					}
				}
			}
		}
	}

	if (canCatch)
		return CheckKeeperCatch();

	if (m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_SLIDE && !sv_ball_slideaffectedbydelay.GetBool())
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

	if (m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_SLIDE && sv_ball_slideaffectedbydelay.GetBool())
		return DoSlideAction();

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
	if (m_vPlForwardVel2D.Length2DSqr() == 0)
		return false;

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

	float coeff = clamp(m_vPlForwardVel2D.Length2D() / mp_slidespeed.GetFloat(), 0.0f, 1.0f);

	float shotStrength;

	if (m_pPl->IsNormalshooting())
		shotStrength = GetNormalshotStrength(coeff, sv_ball_normalslide_strength.GetInt());
	else if (m_pPl->IsPowershooting())
		shotStrength = GetPowershotStrength(coeff, sv_ball_powerslide_strength.GetInt());
	else
		shotStrength = GetChargedshotStrength(coeff, sv_ball_chargedslide_minstrength.GetInt(), sv_ball_chargedslide_maxstrength.GetInt());

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

	SetVel(ballVel, 0, BODY_PART_FEET, false, true, true);

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
	float punchAngYaw = m_aPlAng[YAW];
	float punchAngPitch = 0;
	float catchCoeff = 1.0f;
	static float sqrt2 = sqrt(2.0f);
	float distXY = localDirToBall.Length2D();
	float distXZ = sqrt(Sqr(localDirToBall.x) + Sqr(localDirToBall.z));

	switch (m_pPl->m_Shared.GetAnimEvent())
	{
	case PLAYERANIMEVENT_KEEPER_DIVE_LEFT:
		canReach = (localDirToBall.z < sv_ball_keeper_sidedive_zend.GetInt()
			&& localDirToBall.z >= sv_ball_keeper_sidedive_zstart.GetInt()
			&& abs(localDirToBall.x) <= sv_ball_keeper_sidedive_shortsidereach.GetInt()
			&& localDirToBall.y >= -sv_ball_keeper_sidedive_longsidereach_opposite.GetInt()
			&& localDirToBall.y <= sv_ball_keeper_sidedive_longsidereach.GetInt());

		if (canReach)
		{
			float distY = localDirToBall.y - sv_ball_keeper_sidedive_catchcenteroffset_side.GetInt(); 
			float distZ = localDirToBall.z - sv_ball_keeper_sidedive_catchcenteroffset_z.GetInt(); 

			float maxYReach = (distY >= 0 ? sv_ball_keeper_sidedive_longsidereach.GetInt() : -sv_ball_keeper_sidedive_longsidereach_opposite.GetInt()) - sv_ball_keeper_sidedive_catchcenteroffset_side.GetInt();
			punchAngYaw += abs(distY) / maxYReach * sv_ball_keeper_punch_maxyawangle.GetInt();

			float maxZReach = (distZ >= 0 ? sv_ball_keeper_sidedive_zend.GetInt() : sv_ball_keeper_sidedive_zstart.GetInt()) - sv_ball_keeper_sidedive_catchcenteroffset_z.GetInt();
			punchAngPitch -= abs(distZ) / maxZReach * sv_ball_keeper_punch_maxpitchangle.GetInt();

			catchCoeff = sqrt(pow(abs(distY) / maxYReach, 2) + pow(abs(distZ) / maxZReach, 2)) / sqrt2;
		}
		break;
	case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT:
		canReach = (localDirToBall.z < sv_ball_keeper_sidedive_zend.GetInt()
			&& localDirToBall.z >= sv_ball_keeper_sidedive_zstart.GetInt()
			&& abs(localDirToBall.x) <= sv_ball_keeper_sidedive_shortsidereach.GetInt()
			&& localDirToBall.y <= sv_ball_keeper_sidedive_longsidereach_opposite.GetInt()
			&& localDirToBall.y >= -sv_ball_keeper_sidedive_longsidereach.GetInt());

		if (canReach)
		{
			float distY = localDirToBall.y - -sv_ball_keeper_sidedive_catchcenteroffset_side.GetInt(); 
			float distZ = localDirToBall.z - sv_ball_keeper_sidedive_catchcenteroffset_z.GetInt(); 

			float maxYReach = (distY >= 0 ? sv_ball_keeper_sidedive_longsidereach_opposite.GetInt() : -sv_ball_keeper_sidedive_longsidereach.GetInt()) - -sv_ball_keeper_sidedive_catchcenteroffset_side.GetInt();
			punchAngYaw += abs(distY) / maxYReach * sv_ball_keeper_punch_maxyawangle.GetInt();

			float maxZReach = (distZ >= 0 ? sv_ball_keeper_sidedive_zend.GetInt() : sv_ball_keeper_sidedive_zstart.GetInt()) - sv_ball_keeper_sidedive_catchcenteroffset_z.GetInt();
			punchAngPitch -= abs(distZ) / maxZReach * sv_ball_keeper_punch_maxpitchangle.GetInt();

			catchCoeff = sqrt(pow(abs(distY) / maxYReach, 2) + pow(abs(distZ) / maxZReach, 2)) / sqrt2;
		}
		break;
	case PLAYERANIMEVENT_KEEPER_DIVE_FORWARD:
		canReach = (localDirToBall.z < sv_ball_keeper_forwarddive_zend.GetInt()
			&& localDirToBall.z >= sv_ball_keeper_forwarddive_zstart.GetInt()
			&& localDirToBall.x >= -sv_ball_keeper_forwarddive_longsidereach_opposite.GetInt()
			&& localDirToBall.x <= sv_ball_keeper_forwarddive_longsidereach.GetInt()
			&& abs(localDirToBall.y) <= sv_ball_keeper_forwarddive_shortsidereach.GetInt());

		if (canReach)
		{
			catchCoeff = sv_ball_keeper_forwarddive_catchcoeff.GetFloat();
		}
		break;
	case PLAYERANIMEVENT_KEEPER_JUMP:
	default:
		float maxReachXY = (localDirToBall.z < sv_ball_keeper_standing_catchcenteroffset_z.GetInt() ? sv_ball_keeper_standing_reach_bottom.GetFloat() : sv_ball_keeper_standing_reach_top.GetFloat());

		canReach = (localDirToBall.z < sv_ball_bodypos_keeperarms_end.GetInt()
			&& localDirToBall.z >= sv_ball_bodypos_feet_start.GetInt()
			&& distXY <= maxReachXY);

		if (canReach)
		{
			float distY = localDirToBall.y - sv_ball_keeper_standing_catchcenteroffset_side.GetInt(); 
			float distZ = localDirToBall.z - sv_ball_keeper_standing_catchcenteroffset_z.GetInt(); 

			float maxYReach = (distY >= 0 ? maxReachXY : -maxReachXY) - sv_ball_keeper_standing_catchcenteroffset_side.GetInt();
			punchAngYaw += abs(distY) / maxYReach * sv_ball_keeper_punch_maxyawangle.GetInt();

			float maxZReach = (distZ >= 0 ? sv_ball_bodypos_keeperarms_end.GetInt() : sv_ball_bodypos_feet_start.GetInt()) - sv_ball_keeper_standing_catchcenteroffset_z.GetInt();
			punchAngPitch -= abs(distZ) / maxZReach * sv_ball_keeper_punch_maxpitchangle.GetInt();

			catchCoeff = sqrt(pow(abs(distY) / maxYReach, 2) + pow(abs(distZ) / maxZReach, 2)) / sqrt2;
		}
		break;
	}

	if (!canReach)
		return false;

	float catchTimeLeft = m_flGlobalNextKeeperCatch - gpGlobals->curtime;
	catchTimeLeft *= catchCoeff;

	if (m_bHasQueuedState || (catchTimeLeft > 0.0f && (!LastInfo(true) || LastInfo(true)->m_eBallState != BALL_STATE_PENALTY))) // Punch ball away
	{
		Vector punchDirYaw;
		AngleVectors(QAngle(0, punchAngYaw, 0), &punchDirYaw);
		Vector punchDirPitch;
		AngleVectors(QAngle(punchAngPitch, m_aPlAng[YAW], 0), &punchDirPitch);

		if (m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_KEEPER_DIVE_LEFT || m_pPl->m_Shared.GetAnimEvent() == PLAYERANIMEVENT_KEEPER_DIVE_RIGHT)
			punchDirPitch *= sv_ball_keeper_punch_shortsidecoeff.GetFloat();
		else
			punchDirYaw *= sv_ball_keeper_punch_shortsidecoeff.GetFloat();

		Vector punchDir = punchDirYaw + punchDirPitch;
		punchDir.NormalizeInPlace();
		QAngle punchAngle;
		VectorAngles(punchDir, punchAngle);

		if (punchAngle[PITCH] > 180)
			punchAngle[PITCH] -= 360;

		punchAngle[PITCH] = min(punchAngle[PITCH], sv_ball_keeper_punch_minpitchangle.GetInt());
		punchAngle[PITCH] += sv_ball_keeper_punch_pitchoffset.GetInt();
		AngleVectors(punchAngle, &punchDir);
		Vector vel = punchDir * max(m_vVel.Length2D(), sv_ball_keeper_punch_minstrength.GetInt()) * sv_ball_keeperdeflectioncoeff.GetFloat();
		SetVel(vel, -1, BODY_PART_KEEPERPUNCH, false, false, false);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);
	}
	else // Catch ball
	{
		SetVel(vec3_origin, 0, BODY_PART_KEEPERCATCH, false, false, false);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);		
		State_Transition(BALL_STATE_KEEPERHANDS);
	}

	return true;
}

float CBall::GetPitchCoeff(bool isNormalShot)
{
	//return pow(cos((m_aPlAng[PITCH] - sv_ball_bestshotangle.GetInt()) / (PITCH_LIMIT - sv_ball_bestshotangle.GetInt()) * M_PI / 2), 2);
	// plot 0.5 + (cos(x/89 * pi/2) * 0.5), x=-89..89

	float bestAng = sv_ball_bestshotangle.GetInt();
	float pitch = m_aPlAng[PITCH];

	float coeff;

	if (pitch <= sv_ball_bestshotangle.GetInt())
	{
		float upCoeff = sv_ball_fixedpitchupcoeff.GetFloat();
		double upExp = sv_ball_pitchup_exponent.GetFloat();
		coeff = upCoeff + (1 - upCoeff) * pow(cos((pitch - bestAng) / (mp_pitchup_max.GetFloat() - bestAng) * M_PI / 2), upExp);
	}
	else
	{
		if (isNormalShot)
		{
			float downCoeff = sv_ball_fixedpitchdowncoeff_normalshot.GetFloat();
			double downExp = sv_ball_pitchdown_exponent_normalshot.GetFloat();
			coeff = downCoeff + (1 - downCoeff) * pow(cos((pitch - bestAng) / (mp_pitchdown_max.GetFloat() - bestAng) * M_PI / 2), downExp);		
		}
		else
		{
			float downCoeff = sv_ball_fixedpitchdowncoeff_nonnormalshot.GetFloat();
			double downExp = sv_ball_pitchdown_exponent_nonnormalshot.GetFloat();
			coeff = downCoeff + (1 - downCoeff) * pow(cos((pitch - bestAng) / (mp_pitchdown_max.GetFloat() - bestAng) * M_PI / 2), downExp);		
		}
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
	if (State_Get() == BALL_STATE_PENALTY)
		strength = sv_ball_penaltyshot_maxstrength.GetInt();

	return coeff * strength;
}

float CBall::GetChargedshotStrength(float coeff, int minStrength, int maxStrength)
{
	if (State_Get() == BALL_STATE_PENALTY)
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

bool CBall::DoGroundShot(bool markOffsidePlayers, float velCoeff /*= 1.0f*/)
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
		EmitSound("Ball.Kicknormal");
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
			shotStrength = GetNormalshotStrength(GetPitchCoeff(true), sv_ball_normalshot_strength.GetInt());
		else if (m_pPl->IsPowershooting())
			shotStrength = GetPowershotStrength(GetPitchCoeff(false), sv_ball_powershot_strength.GetInt());
		else
			shotStrength = GetChargedshotStrength(GetPitchCoeff(false), sv_ball_chargedshot_minstrength.GetInt(), sv_ball_chargedshot_maxstrength.GetInt());

		vel = shotDir * shotStrength * velCoeff;

		if (vel.Length() > 700)
		{
			PlayerAnimEvent_t anim = PLAYERANIMEVENT_BLANK;
			anim = PLAYERANIMEVENT_KICK;
			m_pPl->DoServerAnimationEvent(anim);

			if (vel.Length() > 1000)
				EmitSound("Ball.Kickhard");
			else
				EmitSound("Ball.Kicknormal");
		}
		else
		{
			if (localDirToBall.x < 0 && m_aPlAng[PITCH] <= -45)
				m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HEELKICK);
			else
				m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_BLANK);

			EmitSound("Ball.Touch");
		}

		spin = 1;
	}

	SetVel(vel, spin, BODY_PART_FEET, false, markOffsidePlayers, true);

	return true;
}

bool CBall::DoVolleyShot()
{
	float shotStrength;

	if (m_pPl->IsNormalshooting())
		shotStrength = GetNormalshotStrength(GetPitchCoeff(true), sv_ball_normalshot_strength.GetInt());
	else if (m_pPl->IsPowershooting())
		shotStrength = GetPowershotStrength(GetPitchCoeff(false), sv_ball_powershot_strength.GetInt());
	else
		shotStrength = GetChargedshotStrength(GetPitchCoeff(false), sv_ball_chargedshot_minstrength.GetInt(), sv_ball_chargedshot_maxstrength.GetInt());

	QAngle shotAngle = m_aPlAng;
	shotAngle[PITCH] = min(sv_ball_volleyshot_minangle.GetFloat(), m_aPlAng[PITCH]);

	Vector shotDir;
	AngleVectors(shotAngle, &shotDir);

	Vector vel = shotDir * shotStrength * sv_ball_volleyshot_speedcoeff.GetFloat();

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
		vel = m_vPlForward * GetNormalshotStrength(GetPitchCoeff(true), sv_ball_normalheader_strength.GetInt());
		EmitSound("Ball.Kicknormal");
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HEADER_STATIONARY);
	}
	else if (m_vPlForwardVel2D.Length2D() >= mp_walkspeed.GetInt()
		&& (m_nInPenBoxOfTeam == m_pPl->GetTeamNumber() || m_nInPenBoxOfTeam == m_pPl->GetOppTeamNumber())
		&& (m_pPl->m_nButtons & IN_SPEED) && m_pPl->GetGroundEntity())
	{
		Vector forward;
		AngleVectors(QAngle(-5, m_aPlAng[YAW], 0), &forward, NULL, NULL);

		if (m_pPl->IsPowershooting())
			vel = forward * GetPowershotStrength(1.0f, sv_ball_powerdivingheader_strength.GetInt());
		else
			vel = forward * GetChargedshotStrength(1.0f, sv_ball_chargeddivingheader_minstrength.GetInt(), sv_ball_chargeddivingheader_maxstrength.GetInt());

		EmitSound("Ball.Kickhard");
		EmitSound("Player.DivingHeader");
		//m_pPl->AddFlag(FL_FREECAM);
		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_DIVINGHEADER);
	}
	else
	{
		if (m_pPl->IsPowershooting())
			vel = m_vPlForward * GetPowershotStrength(GetPitchCoeff(false), sv_ball_powerheader_strength.GetInt());
		else
			vel = m_vPlForward * GetChargedshotStrength(GetPitchCoeff(false), sv_ball_chargedheader_minstrength.GetInt(), sv_ball_chargedheader_maxstrength.GetInt());

		if (vel.Length() > 1000)
			EmitSound("Ball.Kickhard");
		else
			EmitSound("Ball.Kicknormal");

		m_pPl->DoServerAnimationEvent(PLAYERANIMEVENT_HEADER);
	}

	SetVel(m_vPlForwardVel2D + vel, -1, BODY_PART_HEAD, false, true, true);

	return true;
}

AngularImpulse CBall::CalcSpin(float coeff, bool applyTopspin)
{	
	Vector sideRot = vec3_origin;
	float sideSpin = 0;

	if (coeff > 0)
	{
		sideSpin = m_vVel.Length() * sv_ball_spin.GetInt() * coeff / 100.0f;

		if ((m_pPl->m_nButtons & IN_MOVELEFT) && (!(m_pPl->m_nButtons & IN_MOVERIGHT) || (mp_sidemove_override.GetBool() || mp_curl_override.GetBool()) && m_pPl->m_Shared.m_nLastPressedSingleMoveKey == IN_MOVERIGHT)) 
		{
			sideRot = Vector(0, 0, (m_pPl->IsLegacySideCurl() && mp_client_sidecurl.GetBool()) ? 1 : -1);
		}
		else if ((m_pPl->m_nButtons & IN_MOVERIGHT) && (!(m_pPl->m_nButtons & IN_MOVELEFT) || (mp_sidemove_override.GetBool() || mp_curl_override.GetBool()) && m_pPl->m_Shared.m_nLastPressedSingleMoveKey == IN_MOVELEFT)) 
		{
			sideRot = Vector(0, 0, (m_pPl->IsLegacySideCurl() && mp_client_sidecurl.GetBool()) ? -1 : 1);
		}
	}


	Vector backTopRot = vec3_origin;
	float backTopSpin = 0;

	if (coeff > 0)
	{
		backTopSpin = m_vVel.Length() * sv_ball_spin.GetInt() * coeff / 100.0f;

		if (!sv_ball_jump_topspin_enabled.GetBool() || m_pPl->GetGroundEntity() && !(m_pPl->m_nButtons & IN_JUMP) || !applyTopspin)
		{
			backTopRot = m_vPlRight;
			backTopSpin *= sv_ball_backspin_coeff.GetFloat();
		}
		else if (sv_ball_jump_topspin_enabled.GetBool() && (!m_pPl->GetGroundEntity() || (m_pPl->m_nButtons & IN_JUMP) != 0))
		{
			backTopRot = -m_vPlRight;
			backTopSpin *= sv_ball_topspin_coeff.GetFloat();
		}
	}


	AngularImpulse randRot = vec3_origin;

	if (sideRot == vec3_origin && backTopRot == vec3_origin)
	{
		for (int i = 0; i < 3; i++)
		{
			// Add some random rotation if there isn't any side, back or top spin, since a non-rotating ball looks unnatural
			randRot[i] = sv_ball_defaultspin.GetInt() / 100.0f * (g_IOSRand.RandomInt(0, 1) == 1 ? 1 : -1);
		}
	}


	// The angular impulse is applied locally in the physics engine, so rotate from the player angles
	return (WorldToLocalRotation(SetupMatrixAngles(m_aAng), sideRot, sideSpin) + WorldToLocalRotation(SetupMatrixAngles(m_aAng), backTopRot, backTopSpin) + randRot);
}

void CBall::Think( void	)
{
	SetNextThink(gpGlobals->curtime + sv_ball_thinkinterval.GetFloat());

	State_Think();
}

void CBall::TriggerGoal(int team)
{
	m_nTeam = team;

	if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
	{
		if (m_ePenaltyState == PENALTY_KICKED && m_nTeam == m_nFoulingTeam)
		{
			m_ePenaltyState = PENALTY_SCORED;
			//GetGlobalTeam(m_nFouledTeam)->AddGoal();
			m_bHasQueuedState = true;
		}

		return;
	}

	if (LastInfo(true) && LastInfo(true)->m_eBallState == BALL_STATE_THROWIN && !LastPl(false, LastPl(true)))
	{
		TriggerGoalLine(m_nTeam);
		return;
	}

	if (LastTeam(true) != m_nTeam && LastPl(true))
	{
		LastPl(true)->AddShot();
		LastPl(true)->AddShotOnGoal();
	}

	SDKGameRules()->SetKickOffTeam(m_nTeam);

	int scoringTeam;
	CSDKPlayer *pKeeper = NULL;
	bool isOwnGoal;

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

		if (LastPl(true))
			LastPl(true)->AddOwnGoal();

		ReplayManager()->AddMatchEvent(MATCH_EVENT_OWNGOAL, scoringTeam, LastPl(true));
	}
	else
	{
		scoringTeam = LastTeam(true, pKeeper);

		m_pScorer = NULL;
		m_pFirstAssister = NULL;
		m_pSecondAssister = NULL;

		m_pScorer = LastPl(true, pKeeper);

		if (m_pScorer)
		{
			m_pScorer->AddGoal();

			m_pFirstAssister = LastPl(true, pKeeper, m_pScorer);

			if (m_pFirstAssister && m_pFirstAssister->GetTeam() == m_pScorer->GetTeam() && gpGlobals->curtime - LastInfo(true, pKeeper, m_pScorer)->m_flTime <= sv_ball_stats_assist_maxtime.GetFloat())
			{
				m_pFirstAssister->AddAssist();

				m_pSecondAssister = LastPl(true, pKeeper, m_pScorer, m_pFirstAssister);

				if (m_pSecondAssister && m_pSecondAssister->GetTeam() == m_pScorer->GetTeam() && gpGlobals->curtime - LastInfo(true, pKeeper, m_pScorer, m_pFirstAssister)->m_flTime <= sv_ball_stats_assist_maxtime.GetFloat())
				{
					m_pSecondAssister->AddAssist();
				}
				else
					m_pSecondAssister = NULL;
			}
			else
				m_pFirstAssister = NULL;
		}
		else
			m_pScorer = NULL;

		ReplayManager()->AddMatchEvent(MATCH_EVENT_GOAL, scoringTeam, m_pScorer, m_pFirstAssister, m_pSecondAssister);
	}

	//GetGlobalTeam(scoringTeam)->AddGoal();

	CSDKPlayer *pGoalKeeper = FindNearestPlayer(m_nTeam, FL_POS_KEEPER);
	if (CSDKPlayer::IsOnField(pGoalKeeper))
		pGoalKeeper->AddGoalConceded();

	State_Transition(BALL_STATE_GOAL, sv_ball_statetransition_activationdelay_short.GetFloat(), false, true);
}

void CBall::TriggerGoalLine(int team)
{
	//DevMsg("Trigger goal line\n");
	if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		return;

	m_vTriggerTouchPos = GetPos();

	BallTouchInfo *pInfo = LastInfo(true);

	if (pInfo->m_nTeam != team && pInfo->m_pPl && (m_vTriggerTouchPos - pInfo->m_vBallPos).Length2DSqr() >= pow(sv_ball_stats_shot_mindist.GetFloat(), 2.0f)) // Don't add a missed shot or pass if the player accidentally dribbles the ball out
	{
		float minX = GetGlobalTeam(team)->m_vSixYardBoxMin.GetX();
		float maxX = GetGlobalTeam(team)->m_vSixYardBoxMax.GetX();

		if (m_bHitThePost || m_vTriggerTouchPos.x >= minX && m_vTriggerTouchPos.x <= maxX) // Bounced off the post or crossed the goal line inside the six-yard box
		{
			LastPl(true)->AddShot();
			EmitSound("Crowd.Miss");
			ReplayManager()->AddMatchEvent(MATCH_EVENT_MISS, GetGlobalTeam(team)->GetOppTeamNumber(), LastPl(true));
		}
		else
			LastPl(true)->AddPass();
	}

	if (LastTeam(false) == team)
	{
		//SetMatchEvent(MATCH_EVENT_CORNER, NULL, LastOppTeam(false));
		State_Transition(BALL_STATE_CORNER, sv_ball_statetransition_activationdelay_normal.GetFloat());
	}
	else
	{
		//SetMatchEvent(MATCH_EVENT_THROWIN, NULL, LastOppTeam(false));
		State_Transition(BALL_STATE_GOALKICK, sv_ball_statetransition_activationdelay_normal.GetFloat());
	}
}

void CBall::TriggerSideline()
{
	if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		return;

	Vector ballPos = GetPos();

	BallTouchInfo *pInfo = LastInfo(true);
	CSDKPlayer *pLastPl = LastPl(true);

	if (pInfo && CSDKPlayer::IsOnField(pLastPl))
	{
		if (m_bHitThePost) // Goal post hits don't trigger a statistic change right away, since we don't know if it ends up being a goal or a miss. So do the check here.
			pLastPl->AddShot();
		else if (pInfo->m_eBodyPart != BODY_PART_KEEPERPUNCH
			&& GetVel().Length2DSqr() < pow(sv_ball_stats_clearance_minspeed.GetFloat(), 2.0f)
			&& (ballPos - pInfo->m_vBallPos).Length2DSqr() >= pow(sv_ball_stats_pass_mindist.GetFloat(), 2.0f)) // Pass or interception if over a distance threshold and wasn't punched away by a keeper
		{
			pLastPl->AddPass();
		}
	}

	CBaseEntity *pThrowIn = gEntList.FindEntityByClassnameNearest("info_throw_in", ballPos, 1000);
	if (!pThrowIn)
		return;

	m_vTriggerTouchPos = pThrowIn->GetLocalOrigin();
	//SetMatchEvent(MATCH_EVENT_THROWIN, NULL, LastOppTeam(false));
	State_Transition(BALL_STATE_THROWIN, sv_ball_statetransition_activationdelay_normal.GetFloat());
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
		m_aPlAng[PITCH] = RemapValClamped(m_aPlAng[PITCH], -mp_pitchup.GetFloat(), mp_pitchdown.GetFloat(), -mp_pitchup_remap.GetFloat(), mp_pitchdown_remap.GetFloat());
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

void CBall::Kicked(body_part_t bodyPart, bool isDeflection, const Vector &oldVel)
{
	float dynamicDelay = RemapValClamped(m_vVel.Length(), sv_ball_dynamicshotdelay_minshotstrength.GetInt(), sv_ball_dynamicshotdelay_maxshotstrength.GetInt(), sv_ball_dynamicshotdelay_mindelay.GetFloat(), sv_ball_dynamicshotdelay_maxdelay.GetFloat());
	
	float delay;

	if (State_Get() == BALL_STATE_NORMAL)
	{
		delay = sv_ball_dynamicshotdelay_enabled.GetBool() ? dynamicDelay : sv_ball_shotdelay_normal.GetFloat();
	}
	else
	{
		delay = sv_ball_shotdelay_setpiece.GetFloat();
	}
	
	m_pPl->m_flNextShot = gpGlobals->curtime + delay;
	m_flGlobalNextShot = gpGlobals->curtime + dynamicDelay * sv_ball_shotdelay_global_coeff.GetFloat();
	m_flGlobalNextKeeperCatch = gpGlobals->curtime + dynamicDelay * sv_ball_keepercatchdelay_global_coeff.GetFloat();
	Touched(m_pPl, !isDeflection, bodyPart, oldVel);
}

void CBall::Touched(CSDKPlayer *pPl, bool isShot, body_part_t bodyPart, const Vector &oldVel)
{
	if (SDKGameRules()->IsIntermissionState() || m_bHasQueuedState || SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
		return;

	if (m_Touches.Count() > 0 && m_Touches.Tail()->m_pPl == pPl && m_Touches.Tail()->m_nTeam == pPl->GetTeamNumber()
		&& sv_ball_doubletouchfouls.GetBool() && State_Get() == BALL_STATE_NORMAL && m_Touches.Tail()->m_eBallState != BALL_STATE_NORMAL
		&& m_Touches.Tail()->m_eBallState != BALL_STATE_KEEPERHANDS && pPl->GetTeam()->GetNumPlayers() > 2 && pPl->GetOppTeam()->GetNumPlayers() > 2) // Double touch foul
	{
		TriggerFoul(FOUL_DOUBLETOUCH, pPl->GetLocalOrigin(), pPl);
		State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_activationdelay_long.GetFloat());

		return;
	}

	// Regular touch
	BallTouchInfo *pInfo = LastInfo(true);
	CSDKPlayer *pLastPl = LastPl(true);

	if (pInfo && CSDKPlayer::IsOnField(pLastPl) && pLastPl != pPl)
	{ 
		if (pInfo->m_nTeam != pPl->GetTeamNumber() && (bodyPart == BODY_PART_KEEPERPUNCH
			|| bodyPart == BODY_PART_KEEPERCATCH && oldVel.Length2DSqr() >= pow(sv_ball_stats_save_minspeed.GetInt(), 2.0f))) // All fast balls by an opponent which are caught or punched away by the keeper count as shots on goal
		{
			pPl->AddKeeperSave();
			pLastPl->AddShot();
			pLastPl->AddShotOnGoal();
			EmitSound("Crowd.Save");
			ReplayManager()->AddMatchEvent(MATCH_EVENT_KEEPERSAVE, pPl->GetTeamNumber(), pPl, pLastPl);
		}
		else if ((m_vPos - pInfo->m_vBallPos).Length2DSqr() >= pow(sv_ball_stats_pass_mindist.GetInt(), 2.0f) && pInfo->m_eBodyPart != BODY_PART_KEEPERPUNCH) // Pass or interception
		{
			if (m_bHitThePost)
			{
				pLastPl->AddShot();
			}
			else
			{
				pLastPl->AddPass();

				if (pInfo->m_nTeam == pPl->GetTeamNumber()) // Pass to teammate
				{
					pLastPl->AddPassCompleted();
				}
				else // Intercepted by opponent
				{
					pPl->AddInterception();
				}
			}
		}
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
	m_bHitThePost = false;

	//DevMsg("touches: %d\n", m_Touches.Count());
	
	if (pPl->IsOffside())
	{
		pPl->AddOffside();
		TriggerFoul(FOUL_OFFSIDE, pPl->GetOffsidePos(), pPl);
		SDKGameRules()->SetOffsideLinePositions(pPl->GetOffsideBallPos().y, pPl->GetOffsidePos().y, pPl->GetOffsideLastOppPlayerPos().y);
		State_Transition(BALL_STATE_FREEKICK, sv_ball_statetransition_activationdelay_long.GetFloat());
	}
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
	return info ? info->m_nTeam : TEAM_INVALID;
}

int CBall::LastOppTeam(bool wasShooting, CSDKPlayer *pSkipPl /*= NULL*/, CSDKPlayer *pSkipPl2 /*= NULL*/, CSDKPlayer *pSkipPl3 /*= NULL*/)
{
	BallTouchInfo *info = LastInfo(wasShooting, pSkipPl, pSkipPl2, pSkipPl3);
	return info ? (info->m_nTeam == TEAM_A ? TEAM_B : TEAM_A) : TEAM_INVALID;
}

void CBall::UpdatePossession(CSDKPlayer *pNewPossessor)
{
	if (m_pPossessingPl == pNewPossessor)
		return;

	if (m_flPossessionStart != -1)
	{
		float duration = gpGlobals->curtime - m_flPossessionStart;

		if (m_pPossessingPl)
			m_pPossessingPl->AddPossessionTime(duration);

		float total = GetGlobalTeam(TEAM_A)->m_flPossessionTime + GetGlobalTeam(TEAM_B)->m_flPossessionTime;

		GetGlobalTeam(TEAM_A)->m_Possession = floor(GetGlobalTeam(TEAM_A)->m_flPossessionTime * 100 / max(1, total) + 0.5f);
		GetGlobalTeam(TEAM_B)->m_Possession = 100 - GetGlobalTeam(TEAM_A)->m_Possession;

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
		m_nPossessingTeam = TEAM_INVALID;
		m_flPossessionStart = -1;
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
	m_pOtherPl = NULL;
	m_pCurrentPlayer = NULL;
	RemoveAllTouches();
	m_ePenaltyState = PENALTY_NONE;
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
	m_pScorer = NULL;
	m_pFirstAssister = NULL;
	m_pSecondAssister = NULL;
	m_bNonnormalshotsBlocked = false;
	m_bShotsBlocked = false;
	m_bHitThePost = false;
}

void CBall::ReloadSettings()
{
	CreateVPhysics();
}

void CBall::SetPenaltyTaker(CSDKPlayer *pPl)
{
	m_pFouledPl = pPl;
	m_nFouledTeam = pPl->GetTeamNumber();
	m_nFoulingTeam = pPl->GetOppTeamNumber();
}

void CBall::EnablePlayerCollisions(bool enable)
{
	SetCollisionGroup(enable ? COLLISION_GROUP_SOLID_BALL : COLLISION_GROUP_NONSOLID_BALL);
}

void CBall::RemoveFromPlayerHands(CSDKPlayer *pPl)
{
	if (CSDKPlayer::IsOnField(pPl) && pPl->GetTeamPosType() == POS_GK && pPl->m_nBody == MODEL_KEEPER_AND_BALL)
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

QAngle CBall::GetAng()
{
	if (m_bSetNewPos)
		return m_aAng;
	else
	{
		QAngle ang;
		m_pPhys->GetPosition(NULL, &ang);
		return ang;
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

void CBall::SaveBallCannonSettings()
{
	m_vBallCannonPos = GetPos();
	m_aBallCannonAng = GetAng();
	m_vBallCannonVel = GetVel();
	m_vBallCannonRot = GetRot();
	m_bIsBallCannonMode = false;
	//m_pPhys->GetPosition(&m_vBallCannonPos, &m_aBallCannonAng);
	//m_pPhys->GetVelocity(&m_vBallCannonVel, &m_vBallCannonRot);
}

void CBall::RestoreBallCannonSettings()
{
	State_Transition(BALL_STATE_NORMAL);
	m_bRestoreBallCannonSettings = true;
	m_bIsBallCannonMode = true;
	//m_pPhys->SetVelocityInstantaneous(&vec3_origin, &vec3_origin);
	//m_pPhys->SetPosition(m_vBallCannonPos, m_aBallCannonAng, true);
	//m_pPhys->SetVelocity(&m_vBallCannonVel, &m_vBallCannonRot);
}