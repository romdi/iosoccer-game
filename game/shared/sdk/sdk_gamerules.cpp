//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "multiplay_gamerules.h"
#include "sdk_gamerules.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "weapon_sdkbase.h"
#include "ios_teamkit_parse.h"

extern void Bot_RunAll( void );

#ifdef CLIENT_DLL

	#include "precache_register.h"
	#include "c_sdk_player.h"
	#include "c_team.h"

#else
	
	#include "voice_gamemgr.h"
	#include "sdk_player.h"
	#include "team.h"
	#include "sdk_playerclass_info_parse.h"
	#include "player_resource.h"
	#include "mapentities.h"
	#include "sdk_basegrenade_projectile.h"
	#include "sdk_player.h"		//ios
	#include "game.h"			//ios
	#include "ios_mapentities.h"
	#include "movehelper_server.h"
	#include "ios_replaymanager.h"
#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CUniformRandomStream g_IOSRand;

ConVar	r_winddir( "r_winddir", "0", FCVAR_REPLICATED, "Weather effects wind direction angle" );
ConVar	r_windspeed	( "r_windspeed", "0", FCVAR_REPLICATED, "Weather effects wind speed scalar" );

ConVar r_weather_hack( "r_weather_hack", "0", FCVAR_REPLICATED );
ConVar r_weather_profile( "r_weather_profile", "0", FCVAR_REPLICATED, "Enable/disable rain profiling." );

ConVar r_rain_radius( "r_rain_radius", "700", FCVAR_REPLICATED );
ConVar r_rain_height( "r_rain_height", "500", FCVAR_REPLICATED );
ConVar r_rain_playervelmultiplier( "r_rain_playervelmultiplier", "1", FCVAR_REPLICATED );
ConVar r_rain_sidevel( "r_rain_sidevel", "130", FCVAR_REPLICATED, "How much sideways velocity rain gets." );
ConVar r_rain_density( "r_rain_density","0.5", FCVAR_REPLICATED);
ConVar r_rain_width( "r_rain_width", "0.5", FCVAR_REPLICATED );
ConVar r_rain_length( "r_rain_length", "0.1f", FCVAR_REPLICATED );
ConVar r_rain_speed( "r_rain_speed", "600.0f", FCVAR_REPLICATED );
ConVar r_rain_alpha( "r_rain_alpha", "1", FCVAR_REPLICATED );
ConVar r_rain_alphapow( "r_rain_alphapow", "0.8", FCVAR_REPLICATED );
ConVar r_rain_initialramp( "r_rain_initialramp", "0.6", FCVAR_REPLICATED );
ConVar r_rain_splashpercentage( "r_rain_splashpercentage", "0", FCVAR_REPLICATED ); // N% chance of a rain particle making a splash.

ConVar r_snow_radius( "r_snow_radius", "700", FCVAR_REPLICATED );
ConVar r_snow_height( "r_snow_height", "250", FCVAR_REPLICATED );
ConVar r_snow_playervelmultiplier( "r_snow_playervelmultiplier", "1", FCVAR_REPLICATED );
ConVar r_snow_sidevel( "r_snow_sidevel", "50", FCVAR_REPLICATED, "How much sideways velocity snow gets." );
ConVar r_snow_density( "r_snow_density","0.25", FCVAR_REPLICATED);
ConVar r_snow_width( "r_snow_width", "3", FCVAR_REPLICATED );
ConVar r_snow_length( "r_snow_length", "0.07f", FCVAR_REPLICATED );
ConVar r_snow_speed( "r_snow_speed", "80.0f", FCVAR_REPLICATED );
ConVar r_snow_alpha( "r_snow_alpha", "1", FCVAR_REPLICATED );
ConVar r_snow_alphapow( "r_snow_alphapow", "0.8", FCVAR_REPLICATED );
ConVar r_snow_initialramp( "r_snow_initialramp", "1.0", FCVAR_REPLICATED );

const char g_szPosNames[POS_NAME_COUNT][5] =
{
	"GK", "SWP", "LB", "RB", "CB", "LCB", "RCB", "LWB", "RWB", "LM", "RM", "DM", "CM", "AM", "LF", "RF", "CF", "ST", "SS", "LW", "RW", "LCM", "RCM"
};

#define HIDDEN { -1, -1, -1, -1 }

const float g_Positions[11][11][4] =
{
	{//1
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//2
								{ 1.5f, 1, CM, 10 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//3
					{ 0.5f, 1, LM, 11 }, { 2.5f, 1, RM, 9 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//4
					{ 0.5f, 1, LM, 11 }, { 2.5f, 1, RM, 9 },
								{ 1.5f, 1.5f, CM, 10 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//5
					{ 0.5f, 1, LM, 11 }, { 2.5f, 1, RM, 9 },
						{ 0.5f, 2, LB, 2 }, { 2.5f, 2, RB, 3 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//6
					{ 0.5f, 0, LW, 11 }, { 2.5f, 0, RW, 9 },
								{ 1.5f, 1, CM, 10 },
					{ 0.5f, 2, LB, 2 }, { 2.5f, 2, RB, 3 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//7
								{ 1.5f, 0, CF, 10 },
			{ 0.5f, 1, LM, 8 }, { 1.5f, 1.5f, CM, 6 }, { 2.5f, 1, RM, 7 },
						{ 0.5f, 2, LB, 2 }, { 2.5f, 2, RB, 3 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN, HIDDEN
	},
	{//8
								{ 1.5f, 0, CF, 10 },
			{ 0.5f, 1, LM, 11 }, { 1.5f, 1, CM, 6 }, { 2.5f, 1, RM, 7 },
			{ 0.5f, 2, LB, 3 }, { 1.5f, 2, CB, 4 }, { 2.5f, 2, RB, 5 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN, HIDDEN
	},
	{//9
					{ 0.5f, 0, LW, 11 }, { 2.5f, 0, RW, 9 },
			{ 0.5f, 1, LCM, 11 }, { 1.5f, 0.5f, CM, 10 }, { 2.5f, 1, RCM, 7 },
			{ 0.5f, 2, LB, 2 }, { 1.5f, 2, CB, 3 }, { 2.5f, 2, RB, 4 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN, HIDDEN
	},
	{//10
			{ 0.5f, 0, LW, 11 }, { 1.5f, 0, CF, 10 }, { 2.5f, 0, RW, 9 },
			{ 0.5f, 1, LCM, 8 }, { 1.5f, 1, CM, 6 }, { 2.5f, 1, RCM, 7 },
			{ 0.5f, 2, LB, 2 }, { 1.5f, 2, CB, 3 }, { 2.5f, 2, RB, 4 },
								{ 1.5f, 3, GK, 1 },

		HIDDEN
	},
	{//11
			{ 0.5f, 0, LW, 11 }, { 1.5f, 0, CF, 10 }, { 2.5f, 0, RW, 9 },
			{ 0.5f, 1, LCM, 8 }, { 1.5f, 1, CM, 6 }, { 2.5f, 1, RCM, 7 },
		{ 0, 2, LB, 2 }, { 1, 2, LCB, 3 }, { 2, 2, RCB, 4 }, { 3, 2, RB, 5 },
								{ 1.5f, 3, GK, 1 }
	}
};

bool IsValidPosition(int posIndex)
{
	return g_Positions[mp_maxplayers.GetInt() - 1][posIndex][POS_NUMBER] != -1;
}

int GetKeeperPosIndex()
{
	for (int posIndex = 0; posIndex < 11; posIndex++)
	{
		if (g_Positions[mp_maxplayers.GetInt() - 1][posIndex][POS_NAME] == GK)
			return posIndex;
	}
	return 0;
}

#ifndef CLIENT_DLL

class CSpawnPoint : public CPointEntity
{
public:
	bool IsDisabled() { return m_bDisabled; }
	void InputEnable( inputdata_t &inputdata ) { m_bDisabled = false; }
	void InputDisable( inputdata_t &inputdata ) { m_bDisabled = true; }

private:
	bool m_bDisabled;
	DECLARE_DATADESC();
};

BEGIN_DATADESC(CSpawnPoint)

	// Keyfields
	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),

END_DATADESC();

	LINK_ENTITY_TO_CLASS( info_player_deathmatch, CSpawnPoint );
#if defined( SDK_USE_TEAMS )
	LINK_ENTITY_TO_CLASS( info_player_blue, CSpawnPoint );
	LINK_ENTITY_TO_CLASS( info_player_red, CSpawnPoint );
#endif

#endif


REGISTER_GAMERULES_CLASS( CSDKGameRules );

//#ifdef CLIENT_DLL
//void RecvProxy_MatchState( const CRecvProxyData *pData, void *pStruct, void *pOut )
//{
//	CSDKGameRules *pGamerules = ( CSDKGameRules *)pStruct;
//	match_state_t eMatchState = (match_state_t)pData->m_Value.m_Int;
//	pGamerules->SetMatchState( eMatchState );
//}
//#endif 

BEGIN_NETWORK_TABLE_NOBASE( CSDKGameRules, DT_SDKGameRules )
#if defined ( CLIENT_DLL )
	RecvPropTime( RECVINFO( m_flStateEnterTime ) ),
	//RecvPropFloat( RECVINFO( m_fStart) ),
	//RecvPropInt( RECVINFO( m_iDuration) ),
	RecvPropInt( RECVINFO( m_eMatchState) ),// 0, RecvProxy_MatchState ),
	RecvPropInt( RECVINFO( m_nAnnouncedInjuryTime) ),// 0, RecvProxy_MatchState ),
	RecvPropTime( RECVINFO( m_flInjuryTimeStart) ),// 0, RecvProxy_MatchState ),
	RecvPropTime( RECVINFO( m_flInjuryTime) ),// 0, RecvProxy_MatchState ),

	RecvPropInt(RECVINFO(m_nShieldType)),
	RecvPropInt(RECVINFO(m_nShieldTeam)),
	RecvPropVector(RECVINFO(m_vShieldPos)),

	RecvPropVector(RECVINFO(m_vFieldMin)),
	RecvPropVector(RECVINFO(m_vFieldMax)),
	RecvPropVector(RECVINFO(m_vKickOff)),

	RecvPropInt(RECVINFO(m_nPenaltyRound)),
	RecvPropInt(RECVINFO(m_nPenaltyTakingStartTeam)),

	RecvPropInt(RECVINFO(m_nBallZone)),
#else
	SendPropTime( SENDINFO( m_flStateEnterTime )),
	//SendPropFloat( SENDINFO( m_fStart) ),
	//SendPropInt( SENDINFO( m_iDuration) ),
	SendPropInt( SENDINFO( m_eMatchState )),
	SendPropInt( SENDINFO( m_nAnnouncedInjuryTime )),
	SendPropTime( SENDINFO( m_flInjuryTimeStart )),
	SendPropTime( SENDINFO( m_flInjuryTime )),

	SendPropInt(SENDINFO(m_nShieldType)),
	SendPropInt(SENDINFO(m_nShieldTeam)),
	SendPropVector(SENDINFO(m_vShieldPos), -1, SPROP_COORD),

	SendPropVector(SENDINFO(m_vFieldMin), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vFieldMax), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vKickOff), -1, SPROP_COORD),

	SendPropInt(SENDINFO(m_nPenaltyRound)),
	SendPropInt(SENDINFO(m_nPenaltyTakingStartTeam)),

	SendPropInt(SENDINFO(m_nBallZone)),
#endif
END_NETWORK_TABLE()

#if defined ( SDK_USE_PLAYERCLASSES )
	ConVar mp_allowrandomclass( "mp_allowrandomclass", "1", FCVAR_REPLICATED, "Allow players to select random class" );
#endif


LINK_ENTITY_TO_CLASS( sdk_gamerules, CSDKGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( SDKGameRulesProxy, DT_SDKGameRulesProxy )


#ifdef CLIENT_DLL
	void RecvProxy_SDKGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CSDKGameRules *pRules = SDKGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CSDKGameRulesProxy, DT_SDKGameRulesProxy )
		RecvPropDataTable( "sdk_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_SDKGameRules ), RecvProxy_SDKGameRules )
	END_RECV_TABLE()
#else
	void *SendProxy_SDKGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CSDKGameRules *pRules = SDKGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CSDKGameRulesProxy, DT_SDKGameRulesProxy )
		SendPropDataTable( "sdk_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_SDKGameRules ), SendProxy_SDKGameRules )
	END_SEND_TABLE()
#endif

#ifndef CLIENT_DLL
	ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade","0");		
#endif

ConVar mp_limitteams( 
	"mp_limitteams", 
	"2", 
	FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Max # of players 1 team can have over another (0 disables check)",
	true, 0,	// min value
	true, 30	// max value
);

static CSDKViewVectors g_SDKViewVectors(

	Vector( 0, 0, 58 ),			//VEC_VIEW
	//Vector( 0, 0, 100 ),			//VEC_VIEW
								
	//Vector(-16, -16, 0 ),		//VEC_HULL_MIN
	//Vector( 16,  16,  72 ),		//VEC_HULL_MAX
	
	Vector(-13, -13, 0 ),		//VEC_HULL_MIN
	Vector( 13,  13,  72 ),		//VEC_HULL_MAX
													
	Vector(-16, -16, 0 ),		//VEC_DUCK_HULL_MIN
	Vector( 16,  16, 45 ),		//VEC_DUCK_HULL_MAX
	Vector( 0, 0, 34 ),			//VEC_DUCK_VIEW
													
	Vector(-13, -13, -13 ),		//VEC_OBS_HULL_MIN
	Vector( 13,  13,  13 ),		//VEC_OBS_HULL_MAX
													
	Vector( 0, 0, 14 )			//VEC_DEAD_VIEWHEIGHT
#if defined ( SDK_USE_PRONE )			
	,Vector(-16, -16, 0 ),		//VEC_PRONE_HULL_MIN
	Vector( 16,  16, 24 ),		//VEC_PRONE_HULL_MAX
	Vector( 0,	0, 10 )			//VEC_PRONE_VIEW
#endif
);

const CViewVectors* CSDKGameRules::GetViewVectors() const
{
	return (CViewVectors*)GetSDKViewVectors();
}

const CSDKViewVectors *CSDKGameRules::GetSDKViewVectors() const
{
	return &g_SDKViewVectors;
}

#ifdef CLIENT_DLL


#else

// --------------------------------------------------------------------------------------------------- //
// Voice helper
// --------------------------------------------------------------------------------------------------- //

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		// Dead players can only be heard by other dead team mates
		if ( pTalker->IsAlive() == false )
		{
			if ( pListener->IsAlive() == false )
				return ( pListener->InSameTeam( pTalker ) );

			return false;
		}

		return ( pListener->InSameTeam( pTalker ) );
	}
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;



// --------------------------------------------------------------------------------------------------- //
// Globals.
// --------------------------------------------------------------------------------------------------- //
static const char *s_PreserveEnts[] =
{
	"player",
	"viewmodel",
	"worldspawn",
	"soundent",
	"ai_network",
	"ai_hint",
	"sdk_gamerules",
	"sdk_team_manager",
	"sdk_team_unassigned",
	"sdk_team_blue",
	"sdk_team_red",
	"sdk_player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sprite",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_brush",
	"func_wall",
	"func_illusionary",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_red",
	"info_player_blue",
	"point_viewcontrol",
	"shadow_control",
	"sky_camera",
	"scene_manager",
	"trigger_soundscape",
	"point_commentary_node",
	"func_precipitation",
	"func_team_wall",
	"", // END Marker
};

// --------------------------------------------------------------------------------------------------- //
// Global helper functions.
// --------------------------------------------------------------------------------------------------- //

// World.cpp calls this but we don't use it in SDK.
void InitBodyQue()
{
}


// --------------------------------------------------------------------------------------------------- //
// CSDKGameRules implementation.
// --------------------------------------------------------------------------------------------------- //

CSDKGameRules::CSDKGameRules()
{
	g_IOSRand.SetSeed(Plat_FloatTime() * 1000);

	m_pCurStateInfo = NULL;
	m_nShieldType = SHIELD_NONE;
	m_vShieldPos = vec3_invalid;
	m_flStateTimeLeft = 0;
	m_flNextPenalty = gpGlobals->curtime;
	m_nPenaltyTakingTeam = TEAM_A;
	m_flInjuryTime = 0;
	m_flInjuryTimeStart = -1;
	m_pPrecip = NULL;
	m_nFirstHalfLeftSideTeam = TEAM_A;
	m_nLeftSideTeam = TEAM_A;
	m_nFirstHalfKickOffTeam = TEAM_A;
	m_nKickOffTeam = TEAM_A;

	//ios m_bLevelInitialized = false;
	//m_flMatchStartTime = 0;
}
void CSDKGameRules::ServerActivate()
{
	//CPlayerPersistentData::RemoveAllPlayerData();

	CTeamKitInfo::FindTeamKits();

	InitTeams();

	CBaseEntity *pEnt = gEntList.FindEntityByClassname(NULL, "info_ball_start");
	trace_t tr;
	UTIL_TraceLine(pEnt->GetLocalOrigin(), Vector(0, 0, -500), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr);

	m_vKickOff = Vector(pEnt->GetLocalOrigin().x, pEnt->GetLocalOrigin().y, tr.endpos.z);
	
	float minX = -FLT_MAX;
	float maxX = FLT_MAX;

	CBaseEntity *pSidelineTrigger = NULL;
	while ((pSidelineTrigger = gEntList.FindEntityByClassname(pSidelineTrigger, "trigger_SideLine")) != NULL)
	{
		Vector min, max;
		pSidelineTrigger->CollisionProp()->WorldSpaceTriggerBounds(&min, &max);
		if (max.x > m_vKickOff.GetX())
		{
			if (min.x < maxX)
				maxX = min.x;
		}
		else
		{
			if (max.x > minX)
				minX = max.x;
		}
	}

	float minY = -FLT_MAX;
	float maxY = FLT_MAX;

	CBaseEntity *pGoalTrigger = NULL;
	while ((pGoalTrigger = gEntList.FindEntityByClassname(pGoalTrigger, "trigger_goal")) != NULL)
	{
		Vector min, max;
		pGoalTrigger->CollisionProp()->WorldSpaceTriggerBounds(&min, &max);
		if (max.y > m_vKickOff.GetY())
		{
			if (min.y < maxY)
				maxY = min.y;
		}
		else
		{
			if (max.y > minY)
				minY = max.y;
		}
	}

	m_vFieldMin = Vector(minX, minY, m_vKickOff.GetZ());
	m_vFieldMax = Vector(maxX, maxY, m_vKickOff.GetZ());

	GetGlobalTeam(TEAM_A)->InitFieldSpots(TEAM_A);
	GetGlobalTeam(TEAM_B)->InitFieldSpots(TEAM_B);

	m_pPrecip = (CPrecipitation *)CreateEntityByName("func_precipitation");
	m_pPrecip->SetType(PRECIPITATION_TYPE_NONE);
	m_pPrecip->Spawn();


	//TODO: remove this
	//CBaseEntity *pCrossbar = NULL;
	//while ((pCrossbar = gEntList.FindEntityByModel(pCrossbar, "goalposts.mdl")) != NULL)
	//{
	//	pCrossbar->SetRenderMode(kRenderTransColor);
	//	pCrossbar->SetRenderColorA(75);
	//}

	State_Transition(MATCH_WARMUP);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSDKGameRules::~CSDKGameRules()
{
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_Teams.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: TF2 Specific Client Commands
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CSDKGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CSDKPlayer *pPlayer = ToSDKPlayer( pEdict );
#if 0
	const char *pcmd = args[0];
	if ( FStrEq( pcmd, "somecommand" ) )
	{
		if ( args.ArgC() < 2 )
			return true;

		// Do something here!

		return true;
	}
	else 
#endif
	// Handle some player commands here as they relate more directly to gamerules state
	if ( pPlayer->ClientCommand( args ) )
	{
		return true;
	}
	else if ( BaseClass::ClientCommand( pEdict, args ) )
	{
		return true;
	}
	return false;
}

void CSDKGameRules::Think()
{
	State_Think();
	//BaseClass::Think();		//this breaks stuff, like voice comms!

	GetVoiceGameMgr()->Update( gpGlobals->frametime );

	//Bot_RunAll();	//ios

	//IOS hold on intermission
	if (m_flIntermissionEndTime > gpGlobals->curtime)
		return;

	if ( g_fGameOver )   // someone else quit the game already
	{
		//for (int i = 1; i <= gpGlobals->maxClients; i++)
		//{
		//	CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		//	if (!pPl)
		//		continue;

		//	CPlayerPersistentData::SavePlayerData(pPl);
		//}

		ChangeLevel(); // intermission is over
		return;
	}

	//if (GetMapRemainingTime() < 0)
	//	GoToIntermission();
}

Vector DropToGround( 
					CBaseEntity *pMainEnt, 
					const Vector &vPos, 
					const Vector &vMins, 
					const Vector &vMaxs )
{
	trace_t trace;
	UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
	return trace.endpos;
}

extern ConVar tv_delaymapchange;
#include "hltvdirector.h"
#include "viewport_panel_names.h"

void CSDKGameRules::GoToIntermission( void )
{
	if ( g_fGameOver )
		return;

	g_fGameOver = true;

	float flWaitTime = 0;

	if ( tv_delaymapchange.GetBool() && HLTVDirector()->IsActive() )	
	{
		flWaitTime = HLTVDirector()->GetDelay();
	}
			
	m_flIntermissionEndTime = gpGlobals->curtime + flWaitTime;
}


///////////////////////////////////////////////////
// auto balance teams if mismatched
//
void CSDKGameRules::AutobalanceTeams(void)
{
}

void CSDKGameRules::PlayerSpawn( CBasePlayer *p )
{	
}

void CSDKGameRules::InitTeams( void )
{
	Assert( g_Teams.Count() == 0 );

	g_Teams.Purge();	// just in case

	// Create the team managers
	for ( int i = 0; i < ARRAYSIZE( pszTeamNames ); i++ )
	{
		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "team_manager" ));

		pTeam->Init( pszTeamNames[i], i );

		g_Teams.AddToTail( pTeam );
	}

	ChooseTeamNames(true, true, true, false);

	CreateEntityByName( "sdk_gamerules" );
}

//Source: http://www.compuphase.com/cmetric.htm
double ColorDistance(const Color &e1, const Color &e2)
{
  long rmean = ( (long)e1.r() + (long)e2.r() ) / 2;
  long r = (long)e1.r() - (long)e2.r();
  long g = (long)e1.g() - (long)e2.g();
  long b = (long)e1.b() - (long)e2.b();
  return sqrtl((((512+rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8));
}

void CSDKGameRules::ChooseTeamNames(bool clubTeams, bool countryTeams, bool realTeams, bool fictitiousTeams)
{
	int kitCount = m_TeamKitInfoDatabase.Count();
	//int clubTeamCount = 0;
	//int countryTeamCount = 0;
	//int realTeamCount = 0;
	//int fictitiousTeamCount = 0;

	//for (int i = 0; i < kitCount; i++)
	//{
	//	if (m_TeamKitInfoDatabase[i]->m_bIsClubTeam)
	//		clubTeamCount += 1;
	//	else
	//		countryTeamCount += 1;

	//	if (m_TeamKitInfoDatabase[i]->m_bIsRealTeam)
	//		realTeamCount += 1;
	//	else
	//		fictitiousTeamCount += 1;
	//}

	//if (clubTeamCount == 0 && clubTeams)
	//	clubTeams = false;

	//if (countryTeamCount == 0 && countryTeams)
	//	countryTeams = false;

	//if (realTeamCount == 0 && realTeams)
	//	realTeams = false;

	//if (fictitiousTeamCount == 0 && fictitiousTeams)
	//	fictitiousTeams = false;

	int attemptCount = 0;
	int teamHome;
	int teamAway;

	do
	{
		attemptCount += 1;
		teamHome = g_IOSRand.RandomInt(0, kitCount - 1);
		teamAway = g_IOSRand.RandomInt(0, kitCount - 1);

		if (m_TeamKitInfoDatabase[teamHome]->m_bIsClubTeam && !clubTeams || !m_TeamKitInfoDatabase[teamHome]->m_bIsClubTeam && !countryTeams)
			continue;

		if (m_TeamKitInfoDatabase[teamHome]->m_bIsRealTeam && !realTeams || !m_TeamKitInfoDatabase[teamHome]->m_bIsRealTeam && !fictitiousTeams)
			continue;

		if (m_TeamKitInfoDatabase[teamAway]->m_bIsClubTeam && !clubTeams || !m_TeamKitInfoDatabase[teamAway]->m_bIsClubTeam && !countryTeams)
			continue;

		if (m_TeamKitInfoDatabase[teamAway]->m_bIsRealTeam && !realTeams || !m_TeamKitInfoDatabase[teamAway]->m_bIsRealTeam && !fictitiousTeams)
			continue;

		if (m_TeamKitInfoDatabase[teamHome]->m_PrimaryKitColor == m_TeamKitInfoDatabase[teamAway]->m_PrimaryKitColor ||
			m_TeamKitInfoDatabase[teamHome]->m_bIsClubTeam != m_TeamKitInfoDatabase[teamAway]->m_bIsClubTeam ||
			m_TeamKitInfoDatabase[teamHome]->m_bIsRealTeam != m_TeamKitInfoDatabase[teamAway]->m_bIsRealTeam)
			continue;

		Msg("color distance: %f\n", ColorDistance(m_TeamKitInfoDatabase[teamHome]->m_PrimaryKitColor, m_TeamKitInfoDatabase[teamAway]->m_PrimaryKitColor));

		mp_teamlist.SetValue(UTIL_VarArgs("%s,%s", m_TeamKitInfoDatabase[teamHome]->m_szKitName, m_TeamKitInfoDatabase[teamAway]->m_szKitName));
		IOS_LogPrintf("Setting random teams: %s against %s\n", m_TeamKitInfoDatabase[teamHome]->m_szKitName, m_TeamKitInfoDatabase[teamAway]->m_szKitName);
		GetGlobalTeam(TEAM_A)->SetKitName(m_TeamKitInfoDatabase[teamHome]->m_szKitName);
		GetGlobalTeam(TEAM_B)->SetKitName(m_TeamKitInfoDatabase[teamAway]->m_szKitName);
		break;

	} while (attemptCount < 1000);

	if (attemptCount == 1000)
		Msg("ERROR: No compatible teams found. Check sv_randomteams parameters.\n");
	
}

/* create some proxy entities that we use for transmitting data */
void CSDKGameRules::CreateStandardEntities()
{
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "sdk_player_manager", vec3_origin, vec3_angle );

	// Create the entity that will send our data to the client.
#ifdef _DEBUG
	CBaseEntity *pEnt = 
#endif
		CBaseEntity::Create( "sdk_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );

	CReplayManager *pReplayManager = dynamic_cast<CReplayManager *>(CreateEntityByName("replaymanager"));
	if (pReplayManager)
		pReplayManager->Spawn();
}

void CSDKGameRules::LevelShutdown()
{
	if (g_pReplayManager)
	{
		UTIL_Remove(g_pReplayManager);
		g_pReplayManager = NULL;
	}
}

#endif

ConVar mp_ball_player_collision("mp_ball_player_collision", "0", FCVAR_ARCHIVE | FCVAR_NOTIFY | FCVAR_REPLICATED);

bool CSDKGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		swap(collisionGroup0,collisionGroup1);
	}

	if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_PUSHAWAY )
	{
		if (mp_ball_player_collision.GetBool())
			return true;
		else
			return false;
	}

	//Don't stand on COLLISION_GROUP_WEAPON
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

//Tony; keep this in shared space.
#if defined ( SDK_USE_PLAYERCLASSES )
const char *CSDKGameRules::GetPlayerClassName( int cls, int team )
{
	CSDKTeam *pTeam = GetGlobalSDKTeam( team );

	if( cls == PLAYERCLASS_RANDOM )
	{
		return "#class_random";
	}

	if( cls < 0 || cls >= pTeam->GetNumPlayerClasses() )
	{
		Assert( false );
		return NULL;
	}

	const CSDKPlayerClassInfo &pClassInfo = pTeam->GetPlayerClassInfo( cls );

	return pClassInfo.m_szPrintName;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Init CS ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;

		for (int i=WEAPON_NONE+1;i<WEAPON_MAX;i++)
		{
			//Tony; ignore grenades, shotgun and the crowbar, grenades and shotgun are handled seperately because of their damage type not being DMG_BULLET.
			if (i == SDK_WEAPON_GRENADE || i == SDK_WEAPON_CROWBAR || i == SDK_WEAPON_SHOTGUN)
				continue;

			def.AddAmmoType( WeaponIDToAlias(i), DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 200/*max carry*/, 1, 0 );
		}

		// def.AddAmmoType( BULLET_PLAYER_50AE,		DMG_BULLET, TRACER_LINE, 0, 0, "ammo_50AE_max",		2400, 0, 10, 14 );
		def.AddAmmoType( "shotgun", DMG_BUCKSHOT, TRACER_NONE, 0, 0,	200/*max carry*/, 1, 0 );
		def.AddAmmoType( "grenades", DMG_BLAST, TRACER_NONE, 0, 0,	4/*max carry*/, 1, 0 );

		//Tony; added for the sdk_jeep
		def.AddAmmoType( "JeepAmmo",	DMG_SHOCK,					TRACER_NONE,			"sdk_jeep_weapon_damage",		"sdk_jeep_weapon_damage", "sdk_jeep_max_rounds", BULLET_IMPULSE(650, 8000), 0 );
	}

	return &def;
}


#ifndef CLIENT_DLL

const char *CSDKGameRules::GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if (!pPlayer)
		return "";

	if (bTeamOnly)
		return "(TEAM)";
	else
		return "";
}

const char *CSDKGameRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	const char *pszFormat = NULL;

	if ( !pPlayer )  // dedicated server output
	{
		pszFormat = "SDK_Chat_StadiumAnnouncer";
	}
	else if ( bTeamOnly )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
			pszFormat = "SDK_Chat_Spec";
		else
			pszFormat = "SDK_Chat_Team_Loc";
	}
	else
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR)
			pszFormat = "SDK_Chat_AllSpec";
		else
			pszFormat = "SDK_Chat_All_Loc";
	}

	return pszFormat;
}

const char *CSDKGameRules::GetChatLocation( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if (!pPlayer)
		return "";

	return g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][ToSDKPlayer(pPlayer)->GetTeamPosIndex()][POS_NAME]];
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Find the relationship between players (teamplay vs. deathmatch)
//-----------------------------------------------------------------------------
int CSDKGameRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
#ifndef CLIENT_DLL
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() || IsTeamplay() == false )
		return GR_NOTTEAMMATE;

	if ( (*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp( GetTeamID(pPlayer), GetTeamID(pTarget) ) )
		return GR_TEAMMATE;

#endif

	return GR_NOTTEAMMATE;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Player has just left the game
//-----------------------------------------------------------------------------
void CSDKGameRules::ClientDisconnected( edict_t *pClient )
{
	CSDKPlayer *pPl = (CSDKPlayer *)CBaseEntity::Instance(pClient);
	CPlayerPersistentData::SavePlayerData(pPl);

	BaseClass::ClientDisconnected( pClient );
}
#endif



#ifndef CLIENT_DLL

void CSDKGameRules::RestartMatch()
{
	State_Transition(MATCH_WARMUP);
}

void CC_SV_Restart(const CCommand &args)
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;

	if (args.ArgC() > 1)
		mp_timelimit_warmup.SetValue((float)atof(args[1]));

	SDKGameRules()->RestartMatch();
}

ConCommand sv_restart( "sv_restart", CC_SV_Restart, "Restart game", 0 );


void CSDKGameRules::StartPenalties()
{
	//SetLeftSideTeam(g_IOSRand.RandomInt(TEAM_A, TEAM_B));
	GetBall()->ResetMatch();
	State_Transition(MATCH_PENALTIES);
}

void CC_SV_StartPenalties(const CCommand &args)
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;

	SDKGameRules()->StartPenalties();
}

ConCommand sv_startpenalties( "sv_startpenalties", CC_SV_StartPenalties, "Start penalty shoot-out", 0 );


void CC_SV_RandomTeams(const CCommand &args)
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;

	if (args.ArgC() < 5)
	{
		Msg( "Usage: Set random teams.\nParameters for allowed team type: <club: 0/1> <country: 0/1> <real: 0/1> <fictitious: 0/1>\nExample: sv_randomteams 1 1 1 1\n" );
		return;
	}

	SDKGameRules()->ChooseTeamNames(atoi(args[1]) != 0, atoi(args[2]) != 0, atoi(args[3]) != 0, atoi(args[4]) != 0);
}

ConCommand sv_randomteams( "sv_randomteams", CC_SV_RandomTeams, "", 0 );


void CSDKGameRules::SetWeather(PrecipitationType_t type)
{
	if (m_pPrecip)
		m_pPrecip->SetType(type);
}

#endif

static void OnWeatherTypeChange(IConVar *var, const char *pOldValue, float flOldValue)
{
#ifdef GAME_DLL
	if (SDKGameRules())
		SDKGameRules()->SetWeather((PrecipitationType_t)((ConVar*)var)->GetInt());
#endif
}

ConVar mp_weather("mp_weather", "0", FCVAR_NOTIFY|FCVAR_REPLICATED, "Weather (0 = sunny, 1 = rainy, 2 = snowy)", true, 0, true, 2, OnWeatherTypeChange );

ConVar mp_showstatetransitions( "mp_showstatetransitions", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show game state transitions." );

ConVar mp_timelimit_match( "mp_timelimit_match", "10", FCVAR_NOTIFY|FCVAR_REPLICATED, "match duration in minutes without breaks (90 is real time)", true, 1, true, 90 );
ConVar mp_timelimit_warmup( "mp_timelimit_warmup", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "time before match start" );
ConVar mp_timelimit_cooldown( "mp_timelimit_cooldown", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "time after match end" );
ConVar mp_timelimit_halftime( "mp_timelimit_halftime", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "half time duration" );
ConVar mp_timelimit_extratime_halftime( "mp_timelimit_extratime_halftime", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "extra time halftime duration" );
ConVar mp_timelimit_extratime_intermission( "mp_timelimit_extratime_intermission", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "time before extra time start" );
ConVar mp_timelimit_penalties( "mp_timelimit_penalties", "1", FCVAR_NOTIFY|FCVAR_REPLICATED, "limit for penalties duration" );
ConVar mp_timelimit_penalties_intermission( "mp_timelimit_penalties_intermission", "0.5", FCVAR_NOTIFY|FCVAR_REPLICATED, "time before penalties start" );
ConVar mp_extratime( "mp_extratime", "1", FCVAR_NOTIFY|FCVAR_REPLICATED );
ConVar mp_penalties( "mp_penalties", "1", FCVAR_NOTIFY|FCVAR_REPLICATED );

ConVar mp_shield_throwin_radius("mp_shield_throwin_radius", "180", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_freekick_radius("mp_shield_freekick_radius", "360", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_corner_radius("mp_shield_corner_radius", "360", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_kickoff_radius("mp_shield_kickoff_radius", "360", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_border("mp_shield_border", "20", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_block_6yardbox("mp_shield_block_6yardbox", "1", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_liberal_taker_positioning("mp_shield_liberal_taker_positioning", "0", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_shield_liberal_teammates_positioning("mp_shield_liberal_teammates_positioning", "0", FCVAR_NOTIFY|FCVAR_REPLICATED);

ConVar mp_field_border("mp_field_border", "150", FCVAR_NOTIFY|FCVAR_REPLICATED);

ConVar mp_offside("mp_offside", "1", FCVAR_NOTIFY|FCVAR_REPLICATED);
ConVar mp_joindelay("mp_joindelay", "3", FCVAR_NOTIFY|FCVAR_REPLICATED);

ConVar mp_powershot_fixed_strength("mp_powershot_fixed_strength", "60", FCVAR_NOTIFY|FCVAR_REPLICATED);

ConVar mp_chat_signal_ready("mp_chat_signal_ready", "/ready", FCVAR_NOTIFY);
ConVar mp_chat_signal_ready_timeout("mp_chat_signal_ready_timeout", "10", FCVAR_NOTIFY);

static void OnMaxPlayersChange(IConVar *var, const char *pOldValue, float flOldValue)
{
#ifdef GAME_DLL
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		pPl->ChangeTeamPos(TEAM_SPECTATOR, 0, true);
	}
#endif
}

ConVar mp_maxplayers("mp_maxplayers", "6", FCVAR_NOTIFY|FCVAR_REPLICATED, "Maximum number of players per team <1-11>", true, 1, true, 11, OnMaxPlayersChange);

#ifdef GAME_DLL

void CSDKGameRules::State_Transition( match_state_t newState )
{
	State_Leave();
	State_Enter( newState );
}

void CSDKGameRules::State_Enter( match_state_t newState )
{
	m_eMatchState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );
	m_flStateEnterTime = gpGlobals->curtime;
	m_flInjuryTime = 0.0f;
	m_flInjuryTimeStart = -1;
	m_nAnnouncedInjuryTime = 0;

	if ( mp_showstatetransitions.GetInt() > 0 )
	{
		if ( m_pCurStateInfo )
			IOS_LogPrintf( "Gamerules: entering state '%s'\n", m_pCurStateInfo->m_pStateName );
		else
			IOS_LogPrintf( "Gamerules: entering state #%d\n", newState );
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->m_Shared.SetStamina(100);
	}

	if (IsIntermissionState())
		GetBall()->State_Transition(BALL_NORMAL);
	else
		GetBall()->RemovePlayerBalls();

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		(this->*m_pCurStateInfo->pfnEnterState)();
	}


}

void CSDKGameRules::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}

void CSDKGameRules::State_Think()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnThink )
	{
		if (GetBall())
			m_nBallZone = GetBall()->CalcFieldZone();

		if (m_pCurStateInfo->m_MinDurationConVar == NULL)
			m_flStateTimeLeft = 0.0f;
		else
		{
			m_flStateTimeLeft = (m_flStateEnterTime + m_pCurStateInfo->m_MinDurationConVar->GetFloat() * 60 / m_pCurStateInfo->m_flMinDurationDivisor) - gpGlobals->curtime;

			if (!IsIntermissionState())
				m_flStateTimeLeft += m_flInjuryTime + min(5, m_nAnnouncedInjuryTime + (abs(m_nBallZone) < 50 ? 0 : 5)) * 60 / (90.0f / mp_timelimit_match.GetFloat());
		}

		(this->*m_pCurStateInfo->pfnThink)();
	}
}

CSDKGameRulesStateInfo* CSDKGameRules::State_LookupInfo( match_state_t state )
{
	static CSDKGameRulesStateInfo gameRulesStateInfos[] =
	{
		{ MATCH_WARMUP,						"MATCH_WARMUP",						&CSDKGameRules::State_WARMUP_Enter,					NULL, &CSDKGameRules::State_WARMUP_Think,					&mp_timelimit_warmup, 1	},
		{ MATCH_FIRST_HALF,					"MATCH_FIRST_HALF",					&CSDKGameRules::State_FIRST_HALF_Enter,				NULL, &CSDKGameRules::State_FIRST_HALF_Think,				&mp_timelimit_match, 2 },
		{ MATCH_HALFTIME,					"MATCH_HALFTIME",					&CSDKGameRules::State_HALFTIME_Enter,				NULL, &CSDKGameRules::State_HALFTIME_Think,					&mp_timelimit_halftime, 1 },
		{ MATCH_SECOND_HALF,				"MATCH_SECOND_HALF",				&CSDKGameRules::State_SECOND_HALF_Enter,			NULL, &CSDKGameRules::State_SECOND_HALF_Think,				&mp_timelimit_match, 2 },
		{ MATCH_EXTRATIME_INTERMISSION,		"MATCH_EXTRATIME_INTERMISSION",		&CSDKGameRules::State_EXTRATIME_INTERMISSION_Enter, NULL, &CSDKGameRules::State_EXTRATIME_INTERMISSION_Think,	&mp_timelimit_extratime_intermission, 1	},
		{ MATCH_EXTRATIME_FIRST_HALF,		"MATCH_EXTRATIME_FIRST_HALF",		&CSDKGameRules::State_EXTRATIME_FIRST_HALF_Enter,	NULL, &CSDKGameRules::State_EXTRATIME_FIRST_HALF_Think,		&mp_timelimit_match, 6 },
		{ MATCH_EXTRATIME_HALFTIME,			"MATCH_EXTRATIME_HALFTIME",			&CSDKGameRules::State_EXTRATIME_HALFTIME_Enter,		NULL, &CSDKGameRules::State_EXTRATIME_HALFTIME_Think,		&mp_timelimit_extratime_halftime, 1 },
		{ MATCH_EXTRATIME_SECOND_HALF,		"MATCH_EXTRATIME_SECOND_HALF",		&CSDKGameRules::State_EXTRATIME_SECOND_HALF_Enter,	NULL, &CSDKGameRules::State_EXTRATIME_SECOND_HALF_Think,	&mp_timelimit_match, 6 },
		{ MATCH_PENALTIES_INTERMISSION,		"MATCH_PENALTIES_INTERMISSION",		&CSDKGameRules::State_PENALTIES_INTERMISSION_Enter, NULL, &CSDKGameRules::State_PENALTIES_INTERMISSION_Think,	&mp_timelimit_penalties_intermission, 1 },
		{ MATCH_PENALTIES,					"MATCH_PENALTIES",					&CSDKGameRules::State_PENALTIES_Enter,				NULL, &CSDKGameRules::State_PENALTIES_Think,				&mp_timelimit_penalties, 1 },
		{ MATCH_COOLDOWN,					"MATCH_COOLDOWN",					&CSDKGameRules::State_COOLDOWN_Enter,				NULL, &CSDKGameRules::State_COOLDOWN_Think,					&mp_timelimit_cooldown, 1 },
	};

	for ( int i=0; i < ARRAYSIZE( gameRulesStateInfos ); i++ )
	{
		if ( gameRulesStateInfos[i].m_eMatchState == state )
			return &gameRulesStateInfos[i];
	}

	return NULL;
}

void CSDKGameRules::State_WARMUP_Enter()
{
	GetBall()->ResetMatch();
	GetBall()->State_Transition(BALL_NORMAL, 0, true);
}

void CSDKGameRules::State_WARMUP_Think()
{
	if (m_flStateTimeLeft <= 0.0f)
		State_Transition(MATCH_FIRST_HALF);
}

void CSDKGameRules::State_FIRST_HALF_Enter()
{
	m_nFirstHalfLeftSideTeam = g_IOSRand.RandomInt(TEAM_A, TEAM_B);
	SetLeftSideTeam(m_nFirstHalfLeftSideTeam);
	m_nFirstHalfKickOffTeam = g_IOSRand.RandomInt(TEAM_A, TEAM_B);
	SetKickOffTeam(m_nFirstHalfKickOffTeam);
	GetBall()->State_Transition(BALL_KICKOFF, 0, true);
}

void CSDKGameRules::State_FIRST_HALF_Think()
{
	if ((45 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = g_IOSRand.RandomInt(1, 4);
	}
	else if (m_flStateTimeLeft <= 0 && GetBall()->State_Get() == BALL_NORMAL)
	{
		State_Transition(MATCH_HALFTIME);
	}
}

void CSDKGameRules::State_HALFTIME_Enter()
{
	GetBall()->State_Transition(BALL_NORMAL, 0, true);
	GetBall()->SendNeutralMatchEvent(MATCH_EVENT_HALFTIME);
	GetBall()->EmitSound("Ball.whistle");
	GetBall()->EmitSound("Ball.cheer");
}

void CSDKGameRules::State_HALFTIME_Think()
{
	if (m_flStateTimeLeft <= 0)
		State_Transition(MATCH_SECOND_HALF);
}

void CSDKGameRules::State_SECOND_HALF_Enter()
{
	SetLeftSideTeam(GetGlobalTeam(m_nFirstHalfLeftSideTeam)->GetOppTeamNumber());
	SetKickOffTeam(GetGlobalTeam(m_nFirstHalfKickOffTeam)->GetOppTeamNumber());
	GetBall()->State_Transition(BALL_KICKOFF, 0, true);
}

void CSDKGameRules::State_SECOND_HALF_Think()
{
	if ((90 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = g_IOSRand.RandomInt(1, 4);
	}
	else if (m_flStateTimeLeft <= 0 && GetBall()->State_Get() == BALL_NORMAL)
	{
		if (mp_extratime.GetBool() && GetGlobalTeam(TEAM_A)->GetGoals() == GetGlobalTeam(TEAM_B)->GetGoals())
			State_Transition(MATCH_EXTRATIME_INTERMISSION);
		else if (mp_penalties.GetBool() && GetGlobalTeam(TEAM_A)->GetGoals() == GetGlobalTeam(TEAM_B)->GetGoals())
			State_Transition(MATCH_PENALTIES_INTERMISSION);
		else
			State_Transition(MATCH_COOLDOWN);
	}
}

void CSDKGameRules::State_EXTRATIME_INTERMISSION_Enter()
{
	GetBall()->State_Transition(BALL_NORMAL, 0, true);
	GetBall()->SendNeutralMatchEvent(MATCH_EVENT_FINAL_WHISTLE);
	GetBall()->EmitSound("Ball.whistle");
	GetBall()->EmitSound("Ball.cheer");
}

void CSDKGameRules::State_EXTRATIME_INTERMISSION_Think()
{
	if (m_flStateTimeLeft <= 0)
	{
		State_Transition(MATCH_EXTRATIME_FIRST_HALF);
	}
}

void CSDKGameRules::State_EXTRATIME_FIRST_HALF_Enter()
{
	SetLeftSideTeam(m_nFirstHalfLeftSideTeam);
	SetKickOffTeam(m_nFirstHalfKickOffTeam);
	GetBall()->State_Transition(BALL_KICKOFF, 0, true);
}

void CSDKGameRules::State_EXTRATIME_FIRST_HALF_Think()
{
	if ((105 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = g_IOSRand.RandomInt(1, 4);
	}
	else if (m_flStateTimeLeft <= 0 && GetBall()->State_Get() == BALL_NORMAL)
	{
		State_Transition(MATCH_EXTRATIME_HALFTIME);
	}
}

void CSDKGameRules::State_EXTRATIME_HALFTIME_Enter()
{
	GetBall()->State_Transition(BALL_NORMAL, 0, true);
	GetBall()->SendNeutralMatchEvent(MATCH_EVENT_HALFTIME);
	GetBall()->EmitSound("Ball.whistle");
	GetBall()->EmitSound("Ball.cheer");
}

void CSDKGameRules::State_EXTRATIME_HALFTIME_Think()
{
	if (m_flStateTimeLeft <= 0)
	{
		State_Transition(MATCH_EXTRATIME_SECOND_HALF);
	}
}

void CSDKGameRules::State_EXTRATIME_SECOND_HALF_Enter()
{
	SetLeftSideTeam(GetGlobalTeam(m_nFirstHalfLeftSideTeam)->GetOppTeamNumber());
	SetKickOffTeam(GetGlobalTeam(m_nFirstHalfKickOffTeam)->GetOppTeamNumber());
	GetBall()->State_Transition(BALL_KICKOFF, 0, true);
}

void CSDKGameRules::State_EXTRATIME_SECOND_HALF_Think()
{
	if ((120 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = g_IOSRand.RandomInt(1, 4);
	}
	else if (m_flStateTimeLeft <= 0 && GetBall()->State_Get() == BALL_NORMAL)
	{
		if (mp_penalties.GetBool() && GetGlobalTeam(TEAM_A)->GetGoals() == GetGlobalTeam(TEAM_B)->GetGoals())
			State_Transition(MATCH_PENALTIES_INTERMISSION);
		else
			State_Transition(MATCH_COOLDOWN);
	}
}

void CSDKGameRules::State_PENALTIES_INTERMISSION_Enter()
{
	GetBall()->State_Transition(BALL_NORMAL, 0, true);
	GetBall()->SendNeutralMatchEvent(MATCH_EVENT_FINAL_WHISTLE);
	GetBall()->EmitSound("Ball.whistle");
	GetBall()->EmitSound("Ball.cheer");
}

void CSDKGameRules::State_PENALTIES_INTERMISSION_Think()
{
	if (m_flStateTimeLeft <= 0)
	{
		State_Transition(MATCH_PENALTIES);
	}
}

void CSDKGameRules::State_PENALTIES_Enter()
{
	m_nPenaltyRound = 0;
	GetGlobalTeam(TEAM_A)->m_nPenaltyGoals = 0;
	GetGlobalTeam(TEAM_B)->m_nPenaltyGoals = 0;
	GetGlobalTeam(TEAM_A)->m_nPenaltyGoalBits = 0;
	GetGlobalTeam(TEAM_B)->m_nPenaltyGoalBits = 0;
	m_flNextPenalty = -1;
	m_nPenaltyTakingStartTeam = g_IOSRand.RandomInt(TEAM_A, TEAM_B);
	m_nPenaltyTakingTeam = m_nPenaltyTakingStartTeam;
	SetLeftSideTeam(g_IOSRand.RandomInt(TEAM_A, TEAM_B));
	GetBall()->SetPenaltyState(PENALTY_NONE);
}

void CSDKGameRules::State_PENALTIES_Think()
{
	if (m_flStateTimeLeft <= 0)
	{
		State_Transition(MATCH_COOLDOWN);
		return;
	}

	if (GetBall()->GetPenaltyState() == PENALTY_KICKED || GetBall()->GetPenaltyState() == PENALTY_ABORTED_NO_KEEPER)
	{
		if (m_flNextPenalty == -1)
		{
			m_flNextPenalty = gpGlobals->curtime + 3;
		}
		else if (m_flNextPenalty <= gpGlobals->curtime)
		{
			if (GetBall()->GetPenaltyState() == PENALTY_KICKED)
			{
				GetBall()->State_Transition(BALL_NORMAL, 0, true);

				if (GetGlobalTeam(m_nPenaltyTakingTeam)->GetGoals() > GetGlobalTeam(m_nPenaltyTakingTeam)->m_nPenaltyGoals)
				{
					GetGlobalTeam(m_nPenaltyTakingTeam)->m_nPenaltyGoals += 1;
					GetGlobalTeam(m_nPenaltyTakingTeam)->m_nPenaltyGoalBits |= (1 << (m_nPenaltyRound / 2));
				}
					
				m_nPenaltyRound += 1;

				if (m_nPenaltyTakingTeam != m_nPenaltyTakingStartTeam)
				{
					if (m_nPenaltyRound >= 10)
					{
						if (GetGlobalTeam(TEAM_A)->GetGoals() != GetGlobalTeam(TEAM_B)->GetGoals())
						{
							State_Transition(MATCH_COOLDOWN);
							return;
						}
					}
				}

				m_nPenaltyTakingTeam = GetGlobalTeam(m_nPenaltyTakingTeam)->GetOppTeamNumber();
				SetLeftSideTeam(GetGlobalTeam(GetLeftSideTeam())->GetOppTeamNumber());
			}

			GetBall()->SetPenaltyState(PENALTY_NONE);
		}
	}
	else if (GetBall()->GetPenaltyState() == PENALTY_NONE || GetBall()->GetPenaltyState() == PENALTY_ABORTED_NO_TAKER)
	{
		for (int attemptCount = 1; attemptCount <= 2; attemptCount++)
		{
			int playerIndices[MAX_PLAYERS];

			for (int i = 1; i <= MAX_PLAYERS; i++)
			{
				playerIndices[i - 1] = i;
			}

			for (int i = MAX_PLAYERS - 1; i >= 1; i--)
			{
				int j = g_IOSRand.RandomInt(0, i);
				int tmp = playerIndices[i];
				playerIndices[i] = playerIndices[j];
				playerIndices[j] = tmp;
			}

			CSDKPlayer *pPenTaker = NULL;

			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(playerIndices[i]));

				if (!CSDKPlayer::IsOnField(pPl))
					continue;

				if (pPl->GetTeamNumber() != m_nPenaltyTakingTeam || pPl->m_ePenaltyState == PENALTY_KICKED || pPl->GetTeamPosition() == 1 && pPl->IsBot())
					continue;

				pPenTaker = pPl;
				break;
			}

			if (pPenTaker)
			{
				GetBall()->SetPenaltyTaker(pPenTaker);
				GetBall()->SetPenaltyState(PENALTY_ASSIGNED);
				GetBall()->State_Transition(BALL_PENALTY, 0, true);
				m_flNextPenalty = -1;
				return;
			}
			else
			{
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

					if (!CSDKPlayer::IsOnField(pPl))
						continue;

					pPl->m_ePenaltyState = PENALTY_NONE;
				}
			}
		}

		State_Transition(MATCH_COOLDOWN);
	}
}

void CSDKGameRules::State_COOLDOWN_Enter()
{
	GetBall()->State_Transition(BALL_NORMAL, 0, true);
	GetBall()->SendNeutralMatchEvent(MATCH_EVENT_FINAL_WHISTLE);
	GetBall()->EmitSound("Ball.whistle");
	GetBall()->EmitSound("Ball.cheer");

	//who won?
	int winners = 0;
	int scoreA = GetGlobalTeam( TEAM_A )->GetGoals();
	int scoreB = GetGlobalTeam( TEAM_B )->GetGoals();
	if (scoreA > scoreB)
		winners = TEAM_A;
	if (scoreB > scoreA)
		winners = TEAM_B;

	//for ( int i = 0; i < MAX_PLAYERS; i++ )		//maxclients?
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSDKPlayer *pPlayer = (CSDKPlayer*)UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		//pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );

		//is this player on the winning team
		if (pPlayer->GetTeamNumber() == winners)
		{
			pPlayer->AddFlag (FL_CELEB);
		}
		//else
		//	pPlayer->AddFlag (FL_ATCONTROLS);

		//freezes the players
		//pPlayer->AddFlag (FL_ATCONTROLS);
	}
}

void CSDKGameRules::State_COOLDOWN_Think()
{
	if (m_flStateTimeLeft <= 0)
	{
		GoToIntermission();
	}
}

#endif

#ifdef GAME_DLL

void OnTeamlistChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	if (SDKGameRules())
	{
		char teamlist[256];
		Q_strncpy(teamlist, ((ConVar*)var)->GetString(), sizeof(teamlist));

		char *home = strtok(teamlist, " ,;");
		char *away = strtok(NULL, " ,;");
		if (home == NULL || away == NULL)
			Msg( "Format: mp_teamlist \"<home team>,<away team>\"\n" );
		else
		{
			IOS_LogPrintf("Setting new teams: %s against %s\n", home, away);
			GetGlobalTeam(TEAM_A)->SetKitName(home);
			GetGlobalTeam(TEAM_B)->SetKitName(away);
		}
	}
}
ConVar mp_teamlist("mp_teamlist", "england,brazil", FCVAR_REPLICATED|FCVAR_NOTIFY, "Set team names", &OnTeamlistChange);
ConVar mp_teamrotation("mp_teamrotation", "brazil,germany,italy,scotland,barcelona,bayern,liverpool,milan,palmeiras", 0, "Set available teams");


void CSDKGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	const char *pszClubName = engine->GetClientConVarValue( pPlayer->entindex(), "clubname" );
	((CSDKPlayer *)pPlayer)->SetClubName(pszClubName);

	const char *pszCountryName = engine->GetClientConVarValue( pPlayer->entindex(), "countryname" );
	((CSDKPlayer *)pPlayer)->SetCountryName(pszCountryName);

	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "playername" );

	const char *pszOldName = pPlayer->GetPlayerName();

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strlen(pszName) != 0 && Q_strcmp( pszOldName, pszName ) )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
		if ( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			event->SetString( "oldname", pszOldName );
			event->SetString( "newname", pszName );
			gameeventmanager->FireEvent( event );
		}
		
		pPlayer->SetPlayerName( pszName );
	}
}

void CSDKGameRules::EnableShield(int type, int team, const Vector &pos /*= vec3_origin*/)
{
	DisableShield();

	m_nShieldType = type;
	m_nShieldTeam = team;
	m_vShieldPos = Vector(pos.x, pos.y, SDKGameRules()->m_vKickOff.GetZ());

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->m_bIsAtTargetPos = false;
	}
}

void CSDKGameRules::DisableShield()
{
	m_nShieldType = SHIELD_NONE;

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->RemoveFlag(FL_REMOTECONTROLLED | FL_SHIELD_KEEP_IN | FL_SHIELD_KEEP_OUT);
		pPl->RemoveSolidFlags(FSOLID_NOT_SOLID);
		pPl->m_bIsAtTargetPos = true;
	}
}

void CSDKGameRules::SetLeftSideTeam(int team)
{
	if (team == m_nLeftSideTeam)
		return;

	m_nLeftSideTeam = team;
	GetGlobalTeam(TEAM_A)->InitFieldSpots(team);
	GetGlobalTeam(TEAM_B)->InitFieldSpots(GetGlobalTeam(team)->GetOppTeamNumber());
	IOS_LogPrintf("Left team set to %s\n", GetGlobalTeam(team)->GetKitName());
}

int CSDKGameRules::GetLeftSideTeam()
{
	return m_nLeftSideTeam;
}

int CSDKGameRules::GetRightSideTeam()
{
	return GetGlobalTeam(m_nLeftSideTeam)->GetOppTeamNumber();
}

void CSDKGameRules::SetKickOffTeam(int team)
{
	m_nKickOffTeam = team;
}

int CSDKGameRules::GetKickOffTeam()
{
	return m_nKickOffTeam;
}

CBaseEntity *CSDKGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	return NULL;
}

void CSDKGameRules::StartInjuryTime()
{
	EndInjuryTime();
	m_flInjuryTimeStart = gpGlobals->curtime;
}

void CSDKGameRules::EndInjuryTime()
{
	if (m_flInjuryTimeStart != -1)
	{
		m_flInjuryTime += gpGlobals->curtime - m_flInjuryTimeStart;
		m_flInjuryTimeStart = -1;
	}
}

#endif

bool CSDKGameRules::IsIntermissionState()
{
	switch (State_Get())
	{
	case MATCH_WARMUP:
	case MATCH_HALFTIME:
	case MATCH_EXTRATIME_INTERMISSION:
	case MATCH_EXTRATIME_HALFTIME:
	case MATCH_PENALTIES_INTERMISSION:
	case MATCH_COOLDOWN:
		return true;
	default:
		return false;
	}
}

int CSDKGameRules::GetShieldRadius()
{
	switch (m_nShieldType)
	{
	case SHIELD_THROWIN: return mp_shield_throwin_radius.GetInt();
	case SHIELD_FREEKICK: return mp_shield_freekick_radius.GetInt();
	case SHIELD_CORNER: return mp_shield_corner_radius.GetInt();
	case SHIELD_KICKOFF: return mp_shield_kickoff_radius.GetInt();
	case SHIELD_PENALTY: return mp_shield_kickoff_radius.GetInt();
	default: return 0;
	}
}

int CSDKGameRules::GetMatchDisplayTimeSeconds()
{
	float flTime = gpGlobals->curtime - SDKGameRules()->m_flStateEnterTime - SDKGameRules()->m_flInjuryTime;
	if (SDKGameRules()->m_flInjuryTimeStart != -1)
		flTime -= gpGlobals->curtime - SDKGameRules()->m_flInjuryTimeStart;
	int nTime;

	switch ( SDKGameRules()->State_Get() )
	{
	case MATCH_EXTRATIME_SECOND_HALF:
	case MATCH_EXTRATIME_SECOND_HALF_INJURY_TIME:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat())) + (90 + 15) * 60;
		break;
	case MATCH_EXTRATIME_FIRST_HALF:
	case MATCH_EXTRATIME_FIRST_HALF_INJURY_TIME:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat())) + 90 * 60;
		break;
	case MATCH_SECOND_HALF:
	case MATCH_SECOND_HALF_INJURY_TIME:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat())) + 45 * 60;
		break;
	case MATCH_FIRST_HALF:
	case MATCH_FIRST_HALF_INJURY_TIME:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat()));
		break;
	case MATCH_WARMUP:
		nTime = (int)(flTime - mp_timelimit_warmup.GetFloat() * 60);
		break;
	case MATCH_HALFTIME:
		nTime = (int)(flTime - mp_timelimit_halftime.GetFloat() * 60);
		break;
	case MATCH_EXTRATIME_INTERMISSION:
		nTime = (int)(flTime - mp_timelimit_extratime_intermission.GetFloat() * 60);
		break;
	case MATCH_EXTRATIME_HALFTIME:
		nTime = (int)(flTime - mp_timelimit_extratime_halftime.GetFloat() * 60);
		break;
	case MATCH_PENALTIES_INTERMISSION:
		nTime = (int)(flTime - mp_timelimit_penalties_intermission.GetFloat() * 60);
		break;
	case MATCH_PENALTIES:
		nTime = (int)(flTime - mp_timelimit_penalties.GetFloat() * 60);
		break;
	case MATCH_COOLDOWN:
		nTime = (int)(flTime - mp_timelimit_cooldown.GetFloat() * 60);
		break;
	default:
		nTime = 0;
		break;
	}

	return nTime;
}

#ifdef GAME_DLL

void CSDKGameRules::CheckChatText(CBasePlayer *pPlayer, char *pText)
{
	CSDKPlayer *pPl = ToSDKPlayer(pPlayer);
	CheckChatForReadySignal( pPl, pText );

	BaseClass::CheckChatText( pPlayer, pText );
}

void CSDKGameRules::CheckChatForReadySignal(CSDKPlayer *pPlayer, char *pText)
{
	if (Q_stricmp(pText, mp_chat_signal_ready.GetString()) != 0)
		return;

	pPlayer->m_flLastReadyTime = gpGlobals->curtime;

	bool allReady = true;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl) || pPl->IsBot())
			continue;

		if (pPl->m_flLastReadyTime == -1 || pPl->m_flLastReadyTime + mp_chat_signal_ready_timeout.GetInt() <= gpGlobals->curtime)
		{
			allReady = false;
			break;
		}
	}

	if (allReady)
	{
		mp_timelimit_warmup.SetValue(0.083f);
		State_Transition(MATCH_WARMUP);
	}
}

bool CSDKGameRules::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	return BaseClass::ClientConnected(pEntity, pszName, pszAddress, reject, maxrejectlen);
}

void IOS_LogPrintf( char *fmt, ... )
{
	va_list		argptr;
	char		tempString[1024];
	
	va_start ( argptr, fmt );
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	int seconds = SDKGameRules()->GetMatchDisplayTimeSeconds();

	Q_snprintf( tempString, sizeof(tempString), UTIL_VarArgs("[Match Time: % 3d:%02d] - %s", seconds / 60, seconds % 60, tempString));

	// Print to server console
	engine->LogPrint( tempString );
}

void CC_Bench(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (args.ArgC() < 2)
	{
		Msg("Bench whom?\n");
		return;
	}

	CSDKPlayer *pPl = ToSDKPlayer(!Q_stricmp(args[0], "bench") ? UTIL_PlayerByName(args[1]) : UTIL_PlayerByUserId(atoi(args[1])));

	if (!pPl)
	{
		Msg("Player not found.\n");
		return;
	}
	
	pPl->ChangeTeamPos(TEAM_SPECTATOR, 0, true);
}

static ConCommand bench("bench", CC_Bench);
static ConCommand benchid("benchid", CC_Bench);

void CC_BenchAll(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (args.ArgC() < 2)
	{
		Msg("Usage: \"benchall <0-2>\". 0: All players, 1: Home team players, 2: Away team players\n");
		return;
	}

	int team = atoi(args[1]);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl) || pPl->IsBot())
			continue;

		if (team == 0 || (pPl->GetTeamNumber() - TEAM_A + 1) == team)
			pPl->ChangeTeamPos(TEAM_SPECTATOR, 0, true);
	}
}

static ConCommand benchall("benchall", CC_BenchAll);

#endif





#ifdef CLIENT_DLL

void CC_ReloadTextures(const CCommand &args)
{
	materials->ReloadTextures();
}

static ConCommand reloadtextures("reloadtextures", CC_ReloadTextures);

#include "curl/curl.h"
#include "Filesystem.h"
#include "utlbuffer.h"
  
struct curl_t
{
	char kitName[32];
	int teamNumber;
	//CUtlBuffer buf;
	FileHandle_t fh;
};

// Called when curl receives data from the server
static size_t rcvData(void *ptr, size_t size, size_t nmemb, curl_t* vars)
{
	//Msg((char*)ptr); // up to 989 characters each time
	//CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
	//vars->buf.Put(ptr, nmemb);
	filesystem->Write(ptr, nmemb, vars->fh);
	//filesystem->WriteFile(VarArgs("materials/models/player_new/foobar/%s", vars->filename), "MOD", buf);
	return size * nmemb;
}

ConVar cl_download_url("cl_download_url", "http://downloads.iosoccer.co.uk/teamkits/");
 
unsigned DoCurl( void *params )
{
	curl_t* vars = (curl_t*) params; // always use a struct!

	char *textures[14] = { "kitdata.txt", "2.vtf", "3.vtf", "4.vtf", "5.vtf", "6.vtf", "7.vtf", "8.vtf", "9.vtf", "10.vtf", "11.vtf", "gksocks.vtf", "keeper.vtf", "socks.vtf" };

	for (int i = 0; i < 14; i++)
	{
		//filesystem->RemoveFile(filename);
		//filesystem->RemoveFile(VarArgs("materials/models/player/teams/%s", vars->kitName));
		filesystem->CreateDirHierarchy(VarArgs("materials/models/player/teams/%s", vars->kitName));
		const char *filename = VarArgs("materials/models/player/teams/%s/%s", vars->kitName, textures[i]);
		vars->fh = filesystem->Open(filename, "a+b", "MOD");

		if (!vars->fh)
			continue;

		CURL *curl;
		curl = curl_easy_init();
		//struct curl_slist *headers=NULL;
		//headers = curl_slist_append(headers, "ACCEPT_ENCODING: gzip");
		//curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, VarArgs("%s/%s/%s", cl_download_url.GetString(), vars->kitName, textures[i]));
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rcvData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, vars);
		//curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		filesystem->Close(vars->fh);
	}

	/*materials->ReloadTextures();

	if (vars->teamNumber == TEAM_A || vars->teamNumber == TEAM_B)
	{
		GetGlobalTeam(vars->teamNumber)->SetKitName(vars->kitName);
	}*/

	// clean up the memory
	delete vars;

	return 0;
}

void DownloadTeamKit(const char *pKitName, int teamNumber)
{
	curl_t* vars = new curl_t;
	Q_strncpy(vars->kitName, pKitName, sizeof(vars->kitName));
	vars->teamNumber = teamNumber;
	CreateSimpleThread( DoCurl, vars );
}

void CC_CL_Curl(const CCommand &args)
{
	if (args.ArgC() < 2)
	{
		Msg("Which file?\n");
		return;
	}

	DownloadTeamKit(args[1], TEAM_INVALID);
}

static ConCommand cl_curl("cl_curl", CC_CL_Curl);

#endif
