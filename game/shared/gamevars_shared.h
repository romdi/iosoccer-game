//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CS_GAMEVARS_SHARED_H
#define CS_GAMEVARS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"

extern ConVar mp_forcecamera;
extern ConVar mp_allowspectators;
extern ConVar friendlyfire;
extern ConVar mp_fadetoblack;

extern ConVar 
	mp_timelimit_match, 
	mp_timelimit_extratime_halftime, 
	mp_timelimit_extratime_intermission,
	mp_timelimit_halftime, 
	mp_timelimit_warmup,
	mp_timelimit_penalties_intermission,
	mp_timelimit_cooldown,
	mp_extratime,
	mp_penalties,
	mp_showstatetransitions,
	mp_shield_throwin_radius_taker,
	mp_shield_throwin_radius_opponent,
	mp_shield_freekick_radius_taker,
	mp_shield_freekick_radius_opponent,
	mp_shield_corner_radius_taker,
	mp_shield_corner_radius_opponent,
	mp_shield_kickoff_radius,
	mp_shield_ball_radius,
	mp_shield_border,
	mp_shield_block_sixyardbox,
	mp_shield_liberal_taker_positioning,
	mp_shield_liberal_teammates_positioning,
	mp_shield_block_opponent_half,
	mp_field_border,
	mp_field_border_enabled,
	mp_offside,
	mp_joindelay,
	mp_maxplayers,
	mp_weather,
	mp_teamkits,
	mp_teamnames,
	mp_custom_shirt_numbers,
	mp_reset_spin_toggles_on_shot,
	mp_serverinfo,
	mp_matchinfo,
	mp_tvcam_speed_coeff,
	mp_tvcam_speed_coeff_fast,
	mp_tvcam_speed_exponent,
	mp_tvcam_speed_exponent_fast,
	mp_tvcam_border_north,
	mp_tvcam_border_east,
	mp_tvcam_border_south,
	mp_tvcam_border_west,
	mp_tvcam_offset_forward,
	mp_tvcam_offset_forward_time,
	mp_tvcam_offset_north,
	mp_tvcam_sideline_offset_height,
	mp_tvcam_sideline_offset_north,
	mp_tvcam_sideline_zoomcoeff,
	mp_timeout_count,
	mp_timeout_duration;

#endif // CS_GAMEVARS_SHARED_H
