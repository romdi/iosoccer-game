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
	mp_shield_throwin_radius,
	mp_shield_freekick_radius,
	mp_shield_corner_radius,
	mp_shield_kickoff_radius,
	mp_shield_border,
	mp_shield_block_6yardbox,
	mp_shield_liberal_taker_positioning,
	mp_shield_liberal_teammates_positioning,
	mp_shield_block_opponent_half,
	mp_field_border,
	mp_field_border_enabled,
	mp_offside,
	mp_joindelay,
	mp_maxplayers,
	mp_weather,
	mp_teamlist,
	mp_teamnames,
	mp_powershot_fixed_strength,
	mp_custom_shirt_numbers,
	mp_reset_spin_toggles_on_shot,
	mp_serverinfo,
	mp_matchinfo;

#endif // CS_GAMEVARS_SHARED_H
