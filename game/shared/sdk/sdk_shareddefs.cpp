//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// the 1 / 2 / 3 respectively are all identical in our template mod to start, I've made the base ones (pc_class1, pc_class2, pc_class3) and then duplicated them for the teams.
//Tony;  for our template we have two versions.
#if defined ( SDK_USE_PLAYERCLASSES ) && defined ( SDK_USE_TEAMS )
const char *pszTeamBlueClasses[] = 
{
	"blue_class1",
	"blue_class2",
	"blue_class3",
	NULL
};

const char *pszTeamRedClasses[] = 
{
	"red_class1",
	"red_class2",
	"red_class3",
	NULL
};
ConVar	mp_limit_blue_class1(		"mp_limit_blue_class1", "-1", FCVAR_REPLICATED, "Class limit for Blue class 1" );
ConVar	mp_limit_blue_class2(		"mp_limit_blue_class2", "-1", FCVAR_REPLICATED, "Class limit for Blue class 2" );
ConVar	mp_limit_blue_class3(		"mp_limit_blue_class3", "-1", FCVAR_REPLICATED, "Class limit for Blue class 3" );

ConVar	mp_limit_red_class1(		"mp_limit_red_class1", "-1", FCVAR_REPLICATED, "Class limit for Red class 1" );
ConVar	mp_limit_red_class2(		"mp_limit_red_class2", "-1", FCVAR_REPLICATED, "Class limit for Red class 2" );
ConVar	mp_limit_red_class3(		"mp_limit_red_class3", "-1", FCVAR_REPLICATED, "Class limit for Red class 3" );

//Tony; not using teams, but we are using classes
#elif defined ( SDK_USE_PLAYERCLASSES ) && !defined( SDK_USE_TEAMS )
const char *pszPlayerClasses[] =
{
	"pc_class1",
	"pc_class2",
	"pc_class3",
	NULL
};
ConVar	mp_limit_pc_class1(		"mp_limit_pc_class1", "-1", FCVAR_REPLICATED, "Class limit for class 1" );
ConVar	mp_limit_pc_class2(		"mp_limit_pc_class2", "-1", FCVAR_REPLICATED, "Class limit for class 2" );
ConVar	mp_limit_pc_class3(		"mp_limit_pc_class3", "-1", FCVAR_REPLICATED, "Class limit for class 3" );
#endif

char pszTeamNames[4][32] =
{
	"Unassigned",
	"Spectator",
	/* ios
#if defined ( SDK_USE_TEAMS )
	"#SDK_Team_Blue",
	"#SDK_Team_Red",
#endif
	*/
	"Home Team",			//IOS
	"Away Team"
};

// ----------------------------------------------------------------------------- //
// Global Weapon Definitions
// ----------------------------------------------------------------------------- //

//--------------------------------------------------------------------------------------------------------
static const char * s_WeaponAliasInfo[] = 
{
	"none",		// WEAPON_NONE
	"mp5",		// SDK_WEAPON_MP5
	"shotgun",	// SDK_WEAPON_SHOTGUN
	"grenade",	// SDK_WEAPON_GRENADE
	"pistol",	// SDK_WEAPON_PISTOL
	"crowbar",	// SDK_WEAPON_CROWBAR
	NULL,		// WEAPON_NONE
};

//--------------------------------------------------------------------------------------------------------
//
// Given an alias, return the associated weapon ID
//
int AliasToWeaponID( const char *alias )
{
	if (alias)
	{
		for( int i=0; s_WeaponAliasInfo[i] != NULL; ++i )
			if (!Q_stricmp( s_WeaponAliasInfo[i], alias ))
				return i;
	}

	return WEAPON_NONE;
}

//--------------------------------------------------------------------------------------------------------
//
// Given a weapon ID, return its alias
//
const char *WeaponIDToAlias( int id )
{
	if ( (id >= WEAPON_MAX) || (id < 0) )
		return NULL;

	return s_WeaponAliasInfo[id];
}

ConVar mp_walkspeed("mp_walkspeed", "150", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_runspeed("mp_runspeed", "250", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_sprintspeed("mp_sprintspeed", "350", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_remotecontrolledspeed("mp_remotecontrolledspeed", "500", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_keeperdiveduration("mp_keeperdiveduration", "1", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_keeperdivespeed_z("mp_keeperdivespeed_z", "250", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_keepersprintdivecoeff_longside("mp_keepersprintdivecoeff_longside", "1.75", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_keepersprintdivecoeff_shortside("mp_keepersprintdivecoeff_shortside", "1", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_keepersprintdivecoeff_z("mp_keepersprintdivecoeff_z", "1.75", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_keeperdivecoeff_longside("mp_keeperdivecoeff_longside", "1.5", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_keeperdivecoeff_shortside("mp_keeperdivecoeff_shortside", "0.75", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_keeperdivecoeff_z("mp_keeperdivecoeff_z", "1.5", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_slideduration("mp_slideduration", "1", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_slidespeed("mp_slidespeed", "400", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_divingheaderduration("mp_divingheaderduration", "1", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_divingheaderspeed("mp_divingheaderspeed", "400", FCVAR_REPLICATED | FCVAR_NOTIFY);
