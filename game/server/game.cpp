//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "game.h"
#include "physics.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void MapCycleFileChangedCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( Q_stricmp( pOldString, mapcyclefile.GetString() ) != 0 )
	{
		if ( GameRules() )
		{
			// For multiplayer games, forces the mapcyclefile to be reloaded
			GameRules()->ResetMapCycleTimeStamp();
		}
	}
}

ConVar	displaysoundlist( "displaysoundlist","0" );
ConVar  mapcyclefile( "mapcyclefile", "mapcycle.txt", FCVAR_NONE, "Name of the .txt file used to cycle the maps on multiplayer servers ", MapCycleFileChangedCallback );
ConVar  servercfgfile( "servercfgfile","server.cfg" );
ConVar  lservercfgfile( "lservercfgfile","listenserver.cfg" );

// multiplayer server rules
ConVar	teamplay( "mp_teamplay","0", FCVAR_REPLICATED|FCVAR_NOTIFY );
ConVar	fraglimit( "mp_fraglimit","0", FCVAR_REPLICATED|FCVAR_NOTIFY );
ConVar	falldamage( "mp_falldamage","0", FCVAR_REPLICATED|FCVAR_NOTIFY );
ConVar	weaponstay( "mp_weaponstay","0", FCVAR_REPLICATED|FCVAR_NOTIFY );
ConVar	forcerespawn( "mp_forcerespawn","1", FCVAR_REPLICATED|FCVAR_NOTIFY );
ConVar	footsteps( "mp_footsteps","1", FCVAR_REPLICATED|FCVAR_NOTIFY );
ConVar	flashlight( "mp_flashlight","0", FCVAR_REPLICATED|FCVAR_NOTIFY );
ConVar	aimcrosshair( "mp_autocrosshair","1", FCVAR_REPLICATED|FCVAR_NOTIFY );
ConVar	decalfrequency( "decalfrequency","30", FCVAR_REPLICATED|FCVAR_NOTIFY );			//IOS increased from 10
ConVar	teamlist( "mp_teamlist","ENGLAND BRAZIL", FCVAR_REPLICATED|FCVAR_NOTIFY );		//IOS
ConVar	scoretag( "mp_scoretag","  IOSS ", FCVAR_REPLICATED|FCVAR_NOTIFY );				//IOS
ConVar	slidetackle( "mp_slidetackle","25", FCVAR_REPLICATED|FCVAR_NOTIFY );				//IOS
ConVar	redcardtime( "mp_redcardtime","10", FCVAR_REPLICATED|FCVAR_NOTIFY );				//IOS
ConVar	offside( "mp_offside","1", FCVAR_REPLICATED|FCVAR_NOTIFY );						//IOS
ConVar	autobalance( "mp_autobalance", "1", FCVAR_REPLICATED|FCVAR_NOTIFY, "autobalance teams after a goal. blocks joining unbalanced teams" );
ConVar	humankeepers( "mp_humankeepers","1", FCVAR_REPLICATED|FCVAR_NOTIFY );						//IOS
ConVar	botkeepers( "mp_botkeepers","1", FCVAR_REPLICATED|FCVAR_NOTIFY );						//IOS
ConVar	keeperskill( "mp_keeperskill","50", FCVAR_REPLICATED|FCVAR_NOTIFY );				//IOS
ConVar	collisionType( "mp_collisiontype","0", FCVAR_REPLICATED|FCVAR_NOTIFY );						//IOS
ConVar	headbounceCount( "mp_headbounce","5", FCVAR_REPLICATED|FCVAR_NOTIFY );						//IOS
ConVar	headbounceTimer( "mp_headbouncetimer","3", FCVAR_REPLICATED|FCVAR_NOTIFY );						//IOS
ConVar	kickDelay( "mp_kickdelay","0.5", FCVAR_REPLICATED|FCVAR_NOTIFY );						//IOS

ConVar	teamoverride( "mp_teamoverride","1" );
ConVar	defaultteam( "mp_defaultteam","0" );
ConVar	allowNPCs( "mp_allowNPCs","1", FCVAR_REPLICATED|FCVAR_NOTIFY );

// Engine Cvars
const ConVar	*g_pDeveloper = NULL;


ConVar suitvolume( "suitvolume", "0.25", FCVAR_ARCHIVE );

class CGameDLL_ConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool	RegisterConCommandBase( ConCommandBase *pCommand )
	{
		// Remember "unlinked" default value for replicated cvars
		bool replicated = pCommand->IsFlagSet( FCVAR_REPLICATED );
		const char *defvalue = NULL;
		if ( replicated && !pCommand->IsCommand() )
		{
			defvalue = ( ( ConVar * )pCommand)->GetDefault();
		}

		// Link to engine's list instead
		cvar->RegisterConCommand( pCommand );

		// Apply any command-line values.
		const char *pValue = cvar->GetCommandLineValue( pCommand->GetName() );
		if( pValue )
		{
			if ( !pCommand->IsCommand() )
			{
				( ( ConVar * )pCommand )->SetValue( pValue );
			}
		}
		else
		{
			// NOTE:  If not overridden at the command line, then if it's a replicated cvar, make sure that it's
			//  value is the server's value.  This solves a problem where think_limit is defined in shared
			//  code but the value is inside and #if defined( _DEBUG ) block and if you have a debug game .dll
			//  and a release client, then the limiit was coming from the client even though the server value 
			//  was the one that was important during debugging.  Now the server trumps the client value for
			//  replicated ConVars by setting the value here after the ConVar has been linked.
			if ( replicated && defvalue && !pCommand->IsCommand() )
			{
				ConVar *var = ( ConVar * )pCommand;
				var->SetValue( defvalue );
			}
		}

		return true;
	}
};

static CGameDLL_ConVarAccessor g_ConVarAccessor;

// Register your console variables here
// This gets called one time when the game is initialied
void InitializeCvars( void )
{
	// Register cvars here:
	ConVar_Register( FCVAR_GAMEDLL, &g_ConVarAccessor ); 

	g_pDeveloper	= cvar->FindVar( "developer" );
}

