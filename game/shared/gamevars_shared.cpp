//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "gamevars_shared.h"
#include "sdk_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
void MPForceCameraCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( mp_forcecamera.GetInt() < OBS_ALLOW_ALL || mp_forcecamera.GetInt() >= OBS_ALLOW_NUM_MODES )
	{
		mp_forcecamera.SetValue( OBS_ALLOW_TEAM );
	}
}
#endif 

// some shared cvars used by game rules
ConVar mp_forcecamera( 
	"mp_forcecamera", 
	"0", 
	FCVAR_REPLICATED,
	"Restricts spectator modes for dead players"
#ifdef GAME_DLL
	, MPForceCameraCallback 
#endif
	);
	
ConVar mp_allowspectators(
	"mp_allowspectators", 
	"1.0", 
	FCVAR_REPLICATED,
	"toggles whether the server allows spectator mode or not" );

ConVar friendlyfire(
	"mp_friendlyfire",
	"0",
	FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Allows team members to injure other members of their team"
	);

ConVar mp_fadetoblack( 
	"mp_fadetoblack", 
	"0", 
	FCVAR_REPLICATED | FCVAR_NOTIFY, 
	"fade a player's screen to black when he dies" );

static void OnWeatherTypeChange(IConVar *var, const char *pOldValue, float flOldValue)
{
#ifdef GAME_DLL
	if (SDKGameRules())
		SDKGameRules()->SetWeather((PrecipitationType_t)((ConVar*)var)->GetInt());
#endif
}

ConVar mp_weather("mp_weather", "0", FCVAR_NOTIFY|FCVAR_REPLICATED, "Weather (0 = sunny, 1 = rainy, 2 = snowy)", true, 0, true, 2, OnWeatherTypeChange );

ConVar mp_showstatetransitions( "mp_showstatetransitions", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show game state transitions." );

ConVar mp_timelimit_match( "mp_timelimit_match", "20", FCVAR_NOTIFY|FCVAR_REPLICATED, "match duration in minutes without breaks (90 is real time)", true, 0.1f, true, 90 );
ConVar mp_timelimit_warmup( "mp_timelimit_warmup", "1", FCVAR_NOTIFY|FCVAR_REPLICATED, "time before match start" );
ConVar mp_timelimit_cooldown( "mp_timelimit_cooldown", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "time after match end" );
ConVar mp_timelimit_halftime( "mp_timelimit_halftime", "1", FCVAR_NOTIFY|FCVAR_REPLICATED, "half time duration" );
ConVar mp_timelimit_extratime_halftime( "mp_timelimit_extratime_halftime", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "extra time halftime duration" );
ConVar mp_timelimit_extratime_intermission( "mp_timelimit_extratime_intermission", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "time before extra time start" );
ConVar mp_timelimit_penalties_intermission( "mp_timelimit_penalties_intermission", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "time before penalties start" );
ConVar mp_injurytime_min("mp_injurytime_min", "1", FCVAR_NOTIFY);
ConVar mp_injurytime_max("mp_injurytime_max", "4", FCVAR_NOTIFY);
ConVar mp_extratime( "mp_extratime", "1", FCVAR_NOTIFY|FCVAR_REPLICATED );
ConVar mp_penalties( "mp_penalties", "1", FCVAR_NOTIFY|FCVAR_REPLICATED );

ConVar mp_shield_throwin_radius_taker("mp_shield_throwin_radius_taker", "60", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_throwin_radius_opponent("mp_shield_throwin_radius_opponent", "180", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_freekick_radius_taker("mp_shield_freekick_radius_taker", "60", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_freekick_radius_opponent("mp_shield_freekick_radius_opponent", "360", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_corner_radius_taker("mp_shield_corner_radius_taker", "60", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_corner_radius_opponent("mp_shield_corner_radius_opponent", "360", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_kickoff_radius("mp_shield_kickoff_radius", "360", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_ball_radius("mp_shield_ball_radius", "30", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_border("mp_shield_border", "20", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_block_sixyardbox("mp_shield_block_sixyardbox", "1", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_liberal_taker_positioning("mp_shield_liberal_taker_positioning", "0", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_liberal_teammates_positioning("mp_shield_liberal_teammates_positioning", "0", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_block_opponent_half("mp_shield_block_opponent_half", "1", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_field_border("mp_field_border", "175", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_field_border_enabled("mp_field_border_enabled", "1", FCVAR_NOTIFY|FCVAR_REPLICATED);

ConVar mp_offside("mp_offside", "1", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_joindelay("mp_joindelay", "2", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_joincoordduration("mp_joincoordduration", "5", FCVAR_NOTIFY|FCVAR_REPLICATED);

ConVar mp_custom_shirt_numbers("mp_custom_shirt_numbers", "1", FCVAR_NOTIFY);

ConVar mp_reset_spin_toggles_on_shot("mp_reset_spin_toggles_on_shot", "0", FCVAR_NOTIFY | FCVAR_REPLICATED);

ConVar mp_serverinfo("mp_serverinfo", "IOS Server - Enjoy your stay", FCVAR_NOTIFY | FCVAR_REPLICATED);

ConVar mp_matchinfo("mp_matchinfo", "Public Match", FCVAR_NOTIFY | FCVAR_REPLICATED);

ConVar mp_tvcam_speed_coeff("mp_tvcam_speed_coeff", "0.033", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_speed_coeff_fast("mp_tvcam_speed_coeff_fast", "0.066", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_speed_exponent("mp_tvcam_speed_exponent", "2.5", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_speed_exponent_fast("mp_tvcam_speed_exponent_fast", "3.0", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_border_east("mp_tvcam_border_east", "400", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_border_west("mp_tvcam_border_west", "400", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_border_north("mp_tvcam_border_north", "900", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_border_south("mp_tvcam_border_south", "500", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_offset_forward("mp_tvcam_offset_forward", "500", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_offset_forward_time("mp_tvcam_offset_forward_time", "3", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_offset_north("mp_tvcam_offset_north", "250", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_sideline_offset_height("mp_tvcam_sideline_offset_height", "600", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_sideline_offset_north("mp_tvcam_sideline_offset_north", "750", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_sideline_zoomcoeff("mp_tvcam_sideline_zoomcoeff", "0.5", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_tvcam_firstreplaycam("mp_tvcam_firstreplaycam", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "0: sideline, 1: fixed sideline, 2: behind goal, 3: topdown, 4: fly follow, 5: goal line, 6: celebration");
ConVar mp_tvcam_secondreplaycam("mp_tvcam_secondreplaycam", "4", FCVAR_NOTIFY | FCVAR_REPLICATED, "0: sideline, 1: fixed sideline, 2: behind goal, 3: topdown, 4: fly follow, 5: goal line, 6: celebration");

ConVar mp_timeout_count("mp_timeout_count", "3", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_timeout_duration("mp_timeout_duration", "5", FCVAR_NOTIFY | FCVAR_REPLICATED);

ConVar mp_pitchtexture("mp_pitchtexture", "0", FCVAR_NOTIFY | FCVAR_REPLICATED);
