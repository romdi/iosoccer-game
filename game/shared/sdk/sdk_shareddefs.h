//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SDK_SHAREDDEFS_H
#define SDK_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"

//=========================
// GAMEPLAY RELATED OPTIONS
//=========================
// NOTES: The Wizard automatically replaces these strings! If you extract the source as is, you will have to add the defines manually!
//
// Will your mod be team based?
// define SDK_USE_TEAMS
//#define SDK_USE_TEAMS

//
// Do you use player classes?
// define SDK_USE_PLAYERCLASSES
//#define SDK_USE_PLAYERCLASSES

//================================
// PLAYER MOVEMENT RELATED OPTIONS
//================================

//
// Do your players have stamina? - this is a pre-requisite for sprinting, if you define sprinting, and don't uncomment this, it will be included anyway.
// define SDK_USE_STAMINA
#define SDK_USE_STAMINA

//
// Are your players able to sprint?
// define SDK_USE_SPRINTING
#define SDK_USE_SPRINTING

//Tony; stamina is a pre-requisite to sprinting, if you don't declare stamina but you do declare sprinting
//stamina needs to be included.
#if defined ( SDK_USE_SPRINTING ) && !defined( SDK_USE_STAMINA )
#define SDK_USE_STAMINA
#endif

//
// Can your players go prone?
// define SDK_USE_PRONE
//#define SDK_USE_PRONE

//=====================
// EXTRA WEAPON OPTIONS
//=====================

//
// If you're allowing sprinting, do you want to be able to shoot while sprinting?
// define SDK_SHOOT_WHILE_SPRINTING
//#define SDK_SHOOT_WHILE_SPRINTING

//
// Do you want your players to be able to shoot while climing ladders?
// define SDK_SHOOT_ON_LADDERS
//#define SDK_SHOOT_ON_LADDERS

//
// Do you want your players to be able to shoot while jumping?
// define SDK_SHOOT_WHILE_JUMPING
//#define SDK_SHOOT_WHILE_JUMPING



#define SDK_GAME_DESCRIPTION	"IOS:S"

//================================================================================
// Most elements below here are specific to the options above.
//================================================================================

#if defined ( SDK_USE_TEAMS )

enum sdkteams_e
	{
		SDK_TEAM_A = LAST_SHARED_TEAM+1,
		SDK_TEAM_B,
	};

#endif // SDK_USE_TEAMS

#if defined ( SDK_USE_PRONE )

	#define TIME_TO_PRONE	1.2f
	#define VEC_PRONE_HULL_MIN	SDKGameRules()->GetSDKViewVectors()->m_vProneHullMin
	#define VEC_PRONE_HULL_MAX	SDKGameRules()->GetSDKViewVectors()->m_vProneHullMax
	#define VEC_PRONE_VIEW SDKGameRules()->GetSDKViewVectors()->m_vProneView

#endif // SDK_USE_PRONE

#if defined ( SDK_USE_SPRINTING )

	#define INITIAL_SPRINT_STAMINA_PENALTY 15
	#define LOW_STAMINA_THRESHOLD	35

#endif // SDK_USE_SPRINTING

#if defined ( SDK_USE_PLAYERCLASSES )
	#define SDK_NUM_PLAYERCLASSES 3		//Tony; our template sample has 3 player classes.
	#define SDK_PLAYERCLASS_IMAGE_LENGTH 64

	#define PLAYERCLASS_RANDOM		-2
	#define PLAYERCLASS_UNDEFINED	-1

	#if defined ( SDK_USE_TEAMS )
		//Tony; using teams with classes, so make sure the team class panel names are defined.
		#define PANEL_CLASS_BLUE		"class_blue"
		#define PANEL_CLASS_RED			"class_red"

		extern const char *pszTeamBlueClasses[];
		extern const char *pszTeamRedClasses[];
	#else
		#define PANEL_CLASS_NOTEAMS		"class_noteams"
		extern const char *pszPlayerClasses[];
	#endif // SDK_USE_TEAMS

#endif // SDK_USE_PLAYERCLASSES

#define SDK_PLAYER_MODEL "models/player/player.mdl"

extern char pszTeamNames[4][32];

extern ConVar mp_walkspeed;
extern ConVar mp_runspeed;
extern ConVar mp_sprintspeed;
extern ConVar mp_remotecontrolledspeed;
extern ConVar mp_keeperdiveduration;
extern ConVar mp_keeperdivespeed_z;
extern ConVar mp_keeperdivecoeff_longside;
extern ConVar mp_keeperdivecoeff_shortside;
extern ConVar mp_keeperdivecoeff_z;
extern ConVar mp_keepersprintdivecoeff_longside;
extern ConVar mp_keepersprintdivecoeff_shortside;
extern ConVar mp_keepersprintdivecoeff_z;
extern ConVar mp_slideduration;
extern ConVar mp_slidespeed;
extern ConVar mp_divingheaderduration;
extern ConVar mp_divingheaderspeed;

//--------------------------------------------------------------------------------------------------------
//
// Weapon IDs for all SDK Game weapons
//
typedef enum
{
	WEAPON_NONE = 0,

	SDK_WEAPON_NONE = WEAPON_NONE,
	SDK_WEAPON_MP5,
	SDK_WEAPON_SHOTGUN,
	SDK_WEAPON_GRENADE,
	SDK_WEAPON_PISTOL,
	SDK_WEAPON_CROWBAR,

	
	WEAPON_MAX,		// number of weapons weapon index
} SDKWeaponID;

typedef enum
{
	FM_AUTOMATIC = 0,
	FM_SEMIAUTOMATIC,
	FM_BURST,

} SDK_Weapon_Firemodes;

const char *WeaponIDToAlias( int id );
int AliasToWeaponID( const char *alias );


// The various states the player can be in during the join game process.
enum SDKPlayerState
{
	// Happily running around in the game.
	// You can't move though if CSGameRules()->IsFreezePeriod() returns true.
	// This state can jump to a bunch of other states like STATE_PICKINGCLASS or STATE_DEATH_ANIM.
	STATE_ACTIVE=0,
	
	// This is the state you're in when you first enter the server.
	// It's switching between intro cameras every few seconds, and there's a level info 
	// screen up.
	STATE_WELCOME,			// Show the level intro screen.
	
	// During these states, you can either be a new player waiting to join, or
	// you can be a live player in the game who wants to change teams.
	// Either way, you can't move while choosing team or class (or while any menu is up).
#if defined ( SDK_USE_TEAMS )
	STATE_PICKINGTEAM,			// Choosing team.
#endif
#if defined ( SDK_USE_PLAYERCLASSES )
	STATE_PICKINGCLASS,			// Choosing class.
#endif
	
	STATE_DEATH_ANIM,			// Playing death anim, waiting for that to finish.
	STATE_OBSERVER_MODE,		// Noclipping around, watching players, etc.

	NUM_PLAYER_STATES
};
#define SDK_PLAYER_DEATH_TIME	5.0f	//Minimum Time before respawning

// Special Damage types
enum
{
	SDK_DMG_CUSTOM_NONE = 0,
	SDK_DMG_CUSTOM_SUICIDE,
};

// Player avoidance
#define PUSHAWAY_THINK_INTERVAL		(1.0f / 20.0f)

enum match_event_t
{
	MATCH_EVENT_NONE = 0,
	MATCH_EVENT_GOAL,
	MATCH_EVENT_OWNGOAL,
	MATCH_EVENT_FREEKICK,
	MATCH_EVENT_GOALKICK,
	MATCH_EVENT_KICKOFF,
	MATCH_EVENT_CORNER,
	MATCH_EVENT_THROWIN,
	MATCH_EVENT_FOUL,
	MATCH_EVENT_PENALTY,
	MATCH_EVENT_PLAY_ON,
	MATCH_EVENT_FINAL_WHISTLE,
	MATCH_EVENT_OFFSIDE,
	MATCH_EVENT_YELLOWCARD,
	MATCH_EVENT_REDCARD,
	MATCH_EVENT_ASSIST,
	MATCH_EVENT_DOUBLETOUCH,
	MATCH_EVENT_HALFTIME,
	MATCH_EVENT_KEEPERSAVE,
	NUM_MATCH_EVENTS
};

static const char *g_szMatchEventNames[32] =
{
	"",
	"GOAL",
	"OWN GOAL",
	"FREE KICK",
	"GOAL KICK",
	"KICK-OFF",
	"CORNER KICK",
	"THROW-IN",
	"FOUL",
	"PENALTY",
	"PLAY ON",
	"FINAL WHISTLE",
	"OFFSIDE",
	"YELLOW CARD",
	"RED CARD",
	"ASSIST",
	"DOUBLE TOUCH",
	"HALF-TIME",
	"SAVE",
	NULL
};

enum ball_state_t
{
	BALL_NOSTATE = 0,
	BALL_NORMAL,
	BALL_GOAL,
	BALL_CORNER,
	BALL_GOALKICK,
	BALL_THROWIN,
	BALL_FOUL,
	BALL_KICKOFF,
	BALL_PENALTY,
	BALL_FREEKICK,
	BALL_KEEPERHANDS
};

#endif // SDK_SHAREDDEFS_H
