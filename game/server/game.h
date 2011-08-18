//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef GAME_H
#define GAME_H


#include "globals.h"

extern void GameDLLInit( void );

extern ConVar	displaysoundlist;
extern ConVar	mapcyclefile;
extern ConVar	servercfgfile;
extern ConVar	lservercfgfile;

// multiplayer server rules
extern ConVar	teamplay;
extern ConVar	fraglimit;
extern ConVar	falldamage;
extern ConVar	weaponstay;
extern ConVar	forcerespawn;
extern ConVar	footsteps;
extern ConVar	flashlight;
extern ConVar	aimcrosshair;
extern ConVar	decalfrequency;
//extern ConVar	teamlist;		//ios
extern ConVar	scoretag;		//ios
extern ConVar	slidetackle;	//ios
extern ConVar	redcardtime;	//ios
extern ConVar	offside;		//ios
extern ConVar	autobalance;	//ios
extern ConVar	humankeepers;	//ios
extern ConVar	botkeepers;		//ios
extern ConVar	keeperskill;	//ios
extern ConVar	collisionType;	//ios
extern ConVar	headbounceCount;//ios
extern ConVar	headbounceTimer;//ios
extern ConVar	kickDelay;		//ios

extern ConVar	teamoverride;
extern ConVar	defaultteam;
extern ConVar	allowNPCs;

extern ConVar	suitvolume;

// Engine Cvars
extern const ConVar *g_pDeveloper;
#endif		// GAME_H
