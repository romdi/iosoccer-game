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
#include "color.h"

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



#define SDK_GAME_DESCRIPTION	"IOSoccer"

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

extern ConVar
	mp_walkspeed,
	mp_runspeed,
	mp_sprintspeed,
	mp_skillspeed,
	mp_ceremonyspeed,
	mp_jumplandingspeed,
	mp_remotecontrolledspeed,
	mp_keepersidewarddive_move_duration,
	mp_keepersidewarddive_idle_duration,
	mp_keeperbackwarddive_move_duration,
	mp_keeperbackwarddive_idle_duration,
	mp_keeperforwarddive_move_duration,
	mp_keeperforwarddive_idle_duration,
	mp_keeperdivespeed_longside,
	mp_keeperdivespeed_shortside,
	mp_keeperdivespeed_z,
	mp_keeperdiveviewcoeff_enabled,
	mp_keeperdiveviewcoeff_pitchdownangle,
	mp_keeperdiveviewcoeff_pitchupangle,
	mp_keeperdive_movebackcoeff,
	mp_slide_move_duration,
	mp_slide_idle_duration,
	mp_slidespeed,
	mp_divingheader_move_duration,
	mp_divingheader_idle_duration,
	mp_divingheaderspeed,
	mp_bicycleshot_idle_duration,
	mp_chargedshot_increaseduration,
	mp_chargedshot_increaseexponent,
	mp_chargedshot_idleduration,
	mp_throwinthrow_idle_duration,
	mp_tackled_idle_duration,
	mp_charging_animation_enabled,
	mp_strengthscaling_enabled,
	mp_strengthscaling_length,
	mp_strengthscaling_exponent;

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
	PLAYER_STATE_NONE = -1,
	// Happily running around in the game.
	// You can't move though if CSGameRules()->IsFreezePeriod() returns true.
	// This state can jump to a bunch of other states like STATE_PICKINGCLASS or STATE_DEATH_ANIM.
	PLAYER_STATE_ACTIVE=0,

	// During these states, you can either be a new player waiting to join, or
	// you can be a live player in the game who wants to change teams.
	// Either way, you can't move while choosing team or class (or while any menu is up).
	
	PLAYER_STATE_OBSERVER_MODE,		// Noclipping around, watching players, etc.

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
	MATCH_EVENT_MATCH_END,
	MATCH_EVENT_EXTRATIME,
	MATCH_EVENT_PENALTIES,
	MATCH_EVENT_WARMUP,
	MATCH_EVENT_OFFSIDE,
	MATCH_EVENT_YELLOWCARD,
	MATCH_EVENT_SECONDYELLOWCARD,
	MATCH_EVENT_REDCARD,
	MATCH_EVENT_ASSIST,
	MATCH_EVENT_DOUBLETOUCH,
	MATCH_EVENT_HALFTIME,
	MATCH_EVENT_KEEPERSAVE,
	MATCH_EVENT_DRIBBLE,
	MATCH_EVENT_PASS,
	MATCH_EVENT_INTERCEPTION,
	MATCH_EVENT_TIMEOUT,
	MATCH_EVENT_TIMEOUT_PENDING,
	MATCH_EVENT_MISS,
	MATCH_EVENT_ADVANTAGE,
	MATCH_EVENT_COUNT
};

static const char *g_szMatchEventNames[MATCH_EVENT_COUNT] =
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
	"MATCH END",
	"EXTRA TIME",
	"PENALTIES",
	"WARM-UP",
	"OFFSIDE",
	"YELLOW CARD",
	"SECOND YELLOW",
	"RED CARD",
	"ASSIST",
	"DOUBLE TOUCH",
	"HALF-TIME",
	"SAVE",
	"DRIBBLE",
	"PASS",
	"INTERCEPTION",
	"TIMEOUT",
	"TIMEOUT PENDING",
	"MISS",
	"ADVANTAGE"
};

enum match_period_t
{
	MATCH_PERIOD_WARMUP = 0,
	MATCH_PERIOD_FIRST_HALF,
	MATCH_PERIOD_HALFTIME,
	MATCH_PERIOD_SECOND_HALF,
	MATCH_PERIOD_EXTRATIME_INTERMISSION,
	MATCH_PERIOD_EXTRATIME_FIRST_HALF,
	MATCH_PERIOD_EXTRATIME_HALFTIME,
	MATCH_PERIOD_EXTRATIME_SECOND_HALF,
	MATCH_PERIOD_PENALTIES_INTERMISSION,
	MATCH_PERIOD_PENALTIES,
	MATCH_PERIOD_COOLDOWN,
	MATCH_PERIOD_COUNT
};

static const char *g_szMatchPeriodNames[MATCH_PERIOD_COUNT] =
{
	"WARM-UP",
	"FIRST HALF",
	"HALF-TIME",
	"SECOND HALF",
	"EXTRA TIME INTERMISSION",
	"EXTRA TIME FIRST HALF",
	"EXTRA TIME HALF-TIME",
	"EXTRA TIME SECOND HALF",
	"PENALTIES INTERMISSION",
	"PENALTIES",
	"COOL-DOWN"
};

static const char *g_szMatchPeriodShortNames[MATCH_PERIOD_COUNT] =
{
	"WARM-UP",
	"FIRST HALF",
	"HALF-TIME",
	"SECOND HALF",
	"ET INT.",
	"ET FIRST HALF",
	"ET HALF-TIME",
	"ET SECOND HALF",
	"PENALTIES INT.",
	"PENALTIES",
	"COOL-DOWN"
};

enum ball_shield_type_t
{
	SHIELD_NONE = 0,
	SHIELD_THROWIN,
	SHIELD_FREEKICK,
	SHIELD_CORNER,
	SHIELD_GOALKICK,
	SHIELD_KICKOFF,
	SHIELD_PENALTY,
	SHIELD_KEEPERHANDS,
	SHIELD_CEREMONY
};

enum ball_state_t
{
	BALL_STATE_NONE = 0,
	BALL_STATE_STATIC,
	BALL_STATE_NORMAL,
	BALL_STATE_GOAL,
	BALL_STATE_CORNER,
	BALL_STATE_GOALKICK,
	BALL_STATE_THROWIN,
	BALL_STATE_FOUL,
	BALL_STATE_KICKOFF,
	BALL_STATE_PENALTY,
	BALL_STATE_FREEKICK,
	BALL_STATE_KEEPERHANDS
};

enum statistic_type_t
{
	STATISTIC_DEFAULT,
	STATISTIC_SETPIECECOUNT_TEAM,
	STATISTIC_SETPIECECOUNT_PLAYER,
	STATISTIC_POSSESSION_TEAM,
	STATISTIC_POSSESSION_PLAYER,
	STATISTIC_DISTANCETOGOAL,
	STATISTIC_KEEPERSAVES_TEAM,
	STATISTIC_KEEPERSAVES_PLAYER,
	STATISTIC_KEEPERSAVESCAUGHT_TEAM,
	STATISTIC_KEEPERSAVESCAUGHT_PLAYER,
	STATISTIC_SHOTSONGOAL_TEAM,
	STATISTIC_SHOTSONGOAL_PLAYER,
	STATISTIC_PASSING_TEAM,
	STATISTIC_PASSING_PLAYER,
	STATISTIC_INTERCEPTIONS_TEAM,
	STATISTIC_INTERCEPTIONS_PLAYER,
	STATISTIC_CARDS_TEAM,
	STATISTIC_CARDS_PLAYER,
	STATISTIC_FOULS_TEAM,
	STATISTIC_FOULS_PLAYER,
	STATISTIC_OFFSIDES_TEAM,
	STATISTIC_OFFSIDES_PLAYER
};

#define MAX_CLUBNAME_LENGTH			6

#define COUNTRY_NAMES_COUNT 194

static const char g_szCountryNames[COUNTRY_NAMES_COUNT][64] = { "", "Afghanistan", "Albania", "Algeria", "Andorra", "Angola", "Antigua and Barbuda", "Argentina", "Armenia", "Australia", "Austria", "Azerbaijan", "Bahamas", "Bahrain", "Bangladesh", "Barbados", "Belarus", "Belgium", "Belize", "Benin", "Bhutan", "Bolivia", "Bosnia and Herzegovina", "Botswana", "Brazil", "Brunei", "Bulgaria", "Burkina Faso", "Burundi", "Cambodia", "Cameroon", "Canada", "Cape Verde", "Central African Republic", "Chad", "Chile", "China", "Colombia", "Comoros", "Congo", "Cook Islands", "Costa Rica", "Cote d'Ivoire", "Croatia", "Cuba", "Cyprus", "Czech Republic", "Democratic Republic of Congo", "Denmark", "Djibouti", "Dominica", "Dominican Republic", "Ecuador", "Egypt", "El Salvador", "Equatorial Guinea", "Eritrea", "Estonia", "Ethiopia", "Fiji", "Finland", "France", "Gabon", "Gambia", "Georgia", "Germany", "Ghana", "Greece", "Greenland", "Grenada", "Guatemala", "Guinea", "Guinea-Bissau", "Guyana", "Haiti", "Honduras", "Hungary", "Iceland", "India", "Indonesia", "Iran", "Iraq", "Ireland", "Israel", "Italy", "Jamaica", "Japan", "Jordan", "Kazakhstan", "Kenya", "Kiribati", "Kuwait", "Kyrgyzstan", "Laos", "Latvia", "Lebanon", "Lesotho", "Liberia", "Libya", "Liechtenstein", "Lithuania", "Luxembourg", "Macedonia", "Madagascar", "Malawi", "Malaysia", "Maldives", "Mali", "Malta", "Marshall Islands", "Mauritania", "Mauritius", "Mexico", "Moldova", "Monaco", "Mongolia", "Montenegro", "Mozambique", "Myanmar", "Namibia", "Nauru", "Nepal", "Netherlands", "New Zealand", "Nicaragua", "Niger", "Nigeria", "North Korea", "Norway", "Oman", "Pakistan", "Palau", "Panama", "Papua New Guinea", "Paraguay", "Peru", "Philippines", "Poland", "Portugal", "Qatar", "Romania", "Russia", "Rwanda", "Saint Kitts and Nevis", "Saint Lucia", "Saint Vincent and the Grenadines", "Samoa", "San Marino", "Sao Tome and Principe", "Saudi Arabia", "Senegal", "Serbia", "Seychelles", "Sierra Leone", "Singapore", "Slovakia", "Slovenia", "Solomon Islands", "Somalia", "South Africa", "South Korea", "South Sudan", "Spain", "Sri Lanka", "Sudan", "Suriname", "Swaziland", "Sweden", "Switzerland", "Syria", "Tajikistan", "Tanzania", "Thailand", "Timor-Leste", "Togo", "Tonga", "Trinidad and Tobago", "Turkey", "Turkmenistan", "Tuvalu", "Uganda", "Ukraine", "United Arab Emirates", "United Kingdom", "United States", "Uruguay", "Uzbekistan", "Vanuatu", "Vatican City", "Venezuela", "Vietnam", "Yemen", "Zambia", "Zimbabwe" };
static const char g_szCountryISOCodes[COUNTRY_NAMES_COUNT][3] = { "", "AF", "AL", "DZ", "AD", "AO", "AG", "AR", "AM", "AU", "AT", "AZ", "BS", "BH", "BD", "BB", "BY", "BE", "BZ", "BJ", "BT", "BO", "BA", "BW", "BR", "BN", "BG", "BF", "BI", "KH", "CM", "CA", "CV", "CF", "TD", "CL", "CN", "CO", "KM", "CG", "CK", "CR", "CI", "HR", "CU", "CY", "CZ", "CD", "DK", "DJ", "DM", "DO", "EC", "EG", "SV", "GQ", "ER", "EE", "ET", "FJ", "FI", "FR", "GA", "GM", "GE", "DE", "GH", "GR", "GL", "GD", "GT", "GN", "GW", "GY", "HT", "HN", "HU", "IS", "IN", "ID", "IR", "IQ", "IE", "IL", "IT", "JM", "JP", "JO", "KZ", "KE", "KI", "KW", "KG", "LA", "LV", "LB", "LS", "LR", "LY", "LI", "LT", "LU", "MK", "MG", "MW", "MY", "MV", "ML", "MT", "MH", "MR", "MU", "MX", "MD", "MC", "MN", "ME", "MZ", "MM", "NA", "NR", "NP", "NL", "NZ", "NI", "NE", "NG", "KP", "NO", "OM", "PK", "PW", "PA", "PG", "PY", "PE", "PH", "PL", "PT", "QA", "RO", "RU", "RW", "KN", "LC", "VC", "WS", "SM", "ST", "SA", "SN", "RS", "SC", "SL", "SG", "SK", "SI", "SB", "SO", "ZA", "KR", "SS", "ES", "LK", "SD", "SR", "SZ", "SE", "CH", "SY", "TJ", "TZ", "TH", "TL", "TG", "TO", "TT", "TR", "TM", "TV", "UG", "UA", "AE", "GB", "US", "UY", "UZ", "VU", "VA", "VE", "VN", "YE", "ZM", "ZW" };

extern Color g_ColorBlue, g_ColorRed, g_ColorGreen, g_ColorYellow, g_ColorGray, g_ColorWhite, g_ColorBlack, g_ColorBrown, g_ColorLime, g_ColorBlueGray, g_ColorOrange, g_ColorTeal, g_ColorCyan, g_ColorPink, g_ColorAmber, g_ColorLightGreen, g_ColorLightBlue, g_ColorPurple;

enum
{
	POS_XPOS = 0,
	POS_YPOS,
	POS_TYPE,
	POS_NUMBER
};

enum PosTypes_t
{
	POS_GK = 0, 
	POS_SWP, POS_LB, POS_RB, POS_CB, POS_LCB, POS_RCB, POS_LWB, POS_RWB,
	POS_LM, POS_RM, POS_DM, POS_CM, POS_AM, POS_LCM, POS_RCM, POS_CDM, POS_CAM,
	POS_LF, POS_RF, POS_CF, POS_ST, POS_SS, POS_LW, POS_RW,
	POS_NAME_COUNT
};

static const int g_nPosKeeper = (1 << POS_GK);
static const int g_nPosDefense = (1 << POS_SWP) + (1 << POS_LB) + (1 << POS_RB) + (1 << POS_CB) + (1 << POS_LCB) + (1 << POS_RCB) + (1 << POS_LWB) + (1 << POS_RWB);
static const int g_nPosMidfield = (1 << POS_LM) + (1 << POS_RM) + (1 << POS_DM) + (1 << POS_CM) + (1 << POS_AM) + (1 << POS_LCM) + (1 << POS_RCM) + (1 << POS_CDM) + (1 << POS_CAM);
static const int g_nPosAttack = (1 << POS_LF) + (1 << POS_RF) + (1 << POS_CF) + (1 << POS_ST) + (1 << POS_SS) + (1 << POS_LW) + (1 << POS_RW);

static const int g_nPosLeft = (1 << POS_LB) + (1 << POS_LCB) + (1 << POS_LWB) + (1 << POS_LM) + (1 << POS_LCM) + (1 << POS_LF) + (1 << POS_LW);
static const int g_nPosCenter = (1 << POS_GK) + (1 << POS_SWP) + (1 << POS_CB) + (1 << POS_DM) + (1 << POS_CM) + (1 << POS_AM) + (1 << POS_CDM) + (1 << POS_CAM) + (1 << POS_CF) + (1 << POS_ST) + (1 << POS_SS);
static const int g_nPosRight = (1 << POS_RB) + (1 << POS_RCB) + (1 << POS_RWB) + (1 << POS_RM) + (1 << POS_RCM) + (1 << POS_RF) + (1 << POS_RW);

static const char *g_szPosNames[POS_NAME_COUNT] =
{
	"GK",
	"SWP", "LB", "RB", "CB", "LCB", "RCB", "LWB", "RWB",
	"LM", "RM", "DM", "CM", "AM", "LCM", "RCM", "CDM", "CAM",
	"LF", "RF", "CF", "ST", "SS", "LW", "RW"
};

enum
{
	MODEL_PLAYER,
	MODEL_KEEPER,
};

#define BALL_PHYS_RADIUS 4.75f

#define PLAYER_SKIN_COUNT 6

enum TimeoutState_t
{
	TIMEOUT_STATE_NONE,
	TIMEOUT_STATE_PENDING,
	TIMEOUT_STATE_ACTIVE
};

#define SURFACEPROPS_POST 84
#define SURFACEPROPS_NET 85

enum penalty_state_t
{
	PENALTY_NONE = 0,
	PENALTY_ASSIGNED,
	PENALTY_KICKED,
	PENALTY_SCORED,
	PENALTY_SAVED,
	PENALTY_MISSED,
	PENALTY_ABORTED_NO_TAKER,
	PENALTY_ABORTED_NO_KEEPER,
	PENALTY_ABORTED_ILLEGAL_MOVE
};

#define FL_FIELD_MATERIAL_GRASS			(1 << 0)
#define FL_FIELD_MATERIAL_ARTIFICIAL	(1 << 1)
#define FL_FIELD_MATERIAL_STREET		(1 << 2)
#define FL_FIELD_MATERIAL_SAND			(1 << 3)
#define FL_FIELD_MATERIAL_MUD			(1 << 4)

enum field_material_t
{
	FIELD_MATERIAL_GRASS = 0,
	FIELD_MATERIAL_ARTIFICIAL,
	FIELD_MATERIAL_STREET,
	FIELD_MATERIAL_SAND,
	FIELD_MATERIAL_MUD,
	FIELD_MATERIAL_COUNT
};

static const char *g_szFieldMaterials[FIELD_MATERIAL_COUNT] =
{
	"grass",
	"artificial",
	"street",
	"sand",
	"mud"
};

enum color_class_t { COLOR_CLASS_BLACK, COLOR_CLASS_WHITE, COLOR_CLASS_GRAY, COLOR_CLASS_RED, COLOR_CLASS_YELLOW, COLOR_CLASS_GREEN, COLOR_CLASS_CYAN, COLOR_CLASS_BLUE, COLOR_CLASS_MAGENTA, COLOR_CLASS_COUNT };

// https://www.google.com/design/spec/style/color.html#color-color-palette

extern Color g_HudColors[];
extern Color g_HudAlternativeColors[];

#endif // SDK_SHAREDDEFS_H
