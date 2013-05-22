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
#include "gamevars_shared.h"

extern void Bot_RunAll( void );

#ifdef CLIENT_DLL

	#include "precache_register.h"
	#include "c_sdk_player.h"
	#include "c_team.h"
	#include "igameresources.h"

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
ConVar r_rain_width( "r_rain_width", "1.5", FCVAR_REPLICATED );
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

#define POS_NONE { -1, -1, -1, -1 }

const float g_Positions[11][11][4] =
{
	{//1
								{ 1.5f, 3, POS_GK, 1 },

		POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE
	},
	{//2
								{ 1.5f, 1, POS_CM, 10 },
								{ 1.5f, 3, POS_GK, 1 },

		POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE
	},
	{//3
					{ 0.5f, 1, POS_LM, 11 }, { 2.5f, 1, POS_RM, 9 },
								{ 1.5f, 3, POS_GK, 1 },

		POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE
	},
	{//4
					{ 0.5f, 1, POS_LM, 11 }, { 2.5f, 1, POS_RM, 9 },
								{ 1.5f, 1.5f, POS_CM, 10 },
								{ 1.5f, 3, POS_GK, 1 },

		POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE
	},
	{//5
					{ 0.5f, 1, POS_LM, 11 }, { 2.5f, 1, POS_RM, 9 },
						{ 0.5f, 2, POS_LB, 2 }, { 2.5f, 2, POS_RB, 3 },
								{ 1.5f, 3, POS_GK, 1 },

		POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE
	},
	{//6
					{ 0.5f, 0, POS_LW, 11 }, { 2.5f, 0, POS_RW, 9 },
								{ 1.5f, 1, POS_CM, 10 },
					{ 0.5f, 2, POS_LB, 2 }, { 2.5f, 2, POS_RB, 3 },
								{ 1.5f, 3, POS_GK, 1 },

		POS_NONE, POS_NONE, POS_NONE, POS_NONE, POS_NONE
	},
	{//7
								{ 1.5f, 0.5f, POS_CAM, 10 },
			{ 0.5f, 0, POS_LW, 8 }, { 1.5f, 1.75f, POS_CDM, 6 }, { 2.5f, 0, POS_RW, 7 },
						{ 0.5f, 2, POS_LB, 2 }, { 2.5f, 2, POS_RB, 3 },
								{ 1.5f, 3, POS_GK, 1 },

		POS_NONE, POS_NONE, POS_NONE, POS_NONE
	},
	{//8
								{ 1.5f, 0, POS_CF, 10 },
			{ 0.5f, 1, POS_LM, 11 }, { 1.5f, 1, POS_CM, 6 }, { 2.5f, 1, POS_RM, 7 },
			{ 0.5f, 2, POS_LB, 3 }, { 1.5f, 2, POS_CB, 4 }, { 2.5f, 2, POS_RB, 5 },
								{ 1.5f, 3, POS_GK, 1 },

		POS_NONE, POS_NONE, POS_NONE
	},
	{//9
					{ 0.5f, 0, POS_LW, 11 }, { 2.5f, 0, POS_RW, 9 },
			{ 0.5f, 1, POS_LCM, 11 }, { 1.5f, 0.5f, POS_CM, 10 }, { 2.5f, 1, POS_RCM, 7 },
			{ 0.5f, 2, POS_LB, 2 }, { 1.5f, 2, POS_CB, 3 }, { 2.5f, 2, POS_RB, 4 },
								{ 1.5f, 3, POS_GK, 1 },

		POS_NONE, POS_NONE
	},
	{//10
			{ 0.5f, 0, POS_LW, 11 }, { 1.5f, 0, POS_CF, 10 }, { 2.5f, 0, POS_RW, 9 },
			{ 0.5f, 1, POS_LCM, 8 }, { 1.5f, 1, POS_CM, 6 }, { 2.5f, 1, POS_RCM, 7 },
			{ 0.5f, 2, POS_LB, 2 }, { 1.5f, 2, POS_CB, 3 }, { 2.5f, 2, POS_RB, 4 },
								{ 1.5f, 3, POS_GK, 1 },

		POS_NONE
	},
	{//11
			{ 0.5f, 0, POS_LW, 11 }, { 1.5f, 0, POS_CF, 10 }, { 2.5f, 0, POS_RW, 9 },
			{ 0.5f, 1, POS_LCM, 8 }, { 1.5f, 1, POS_CM, 6 }, { 2.5f, 1, POS_RCM, 7 },
		{ 0, 2, POS_LB, 2 }, { 1, 2, POS_LCB, 3 }, { 2, 2, POS_RCB, 4 }, { 3, 2, POS_RB, 5 },
								{ 1.5f, 3, POS_GK, 1 }
	}
};

int GetKeeperPosIndex()
{
	for (int posIndex = 0; posIndex < 11; posIndex++)
	{
		if (g_Positions[mp_maxplayers.GetInt() - 1][posIndex][POS_TYPE] == POS_GK)
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
	RecvPropTime( RECVINFO( m_flMatchStartTime ) ),
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

	RecvPropInt(RECVINFO(m_nBallZone)),
	RecvPropInt(RECVINFO(m_nLeftSideTeam)),

	RecvPropFloat(RECVINFO(m_flOffsideLineBallPosY)),
	RecvPropFloat(RECVINFO(m_flOffsideLineOffsidePlayerPosY)),
	RecvPropFloat(RECVINFO(m_flOffsideLineLastOppPlayerPosY)),
	RecvPropInt(RECVINFO(m_bOffsideLinesEnabled)),

	RecvPropTime(RECVINFO(m_flTimeoutEnd)),
#else
	SendPropTime( SENDINFO( m_flStateEnterTime )),
	SendPropTime( SENDINFO( m_flMatchStartTime )),
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

	SendPropInt(SENDINFO(m_nBallZone)),
	SendPropInt(SENDINFO(m_nLeftSideTeam)),

	SendPropFloat(SENDINFO(m_flOffsideLineBallPosY), 0, SPROP_COORD),
	SendPropFloat(SENDINFO(m_flOffsideLineOffsidePlayerPosY), 0, SPROP_COORD),
	SendPropFloat(SENDINFO(m_flOffsideLineLastOppPlayerPosY), 0, SPROP_COORD),
	SendPropBool(SENDINFO(m_bOffsideLinesEnabled)),

	SendPropTime(SENDINFO(m_flTimeoutEnd)),
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

	Vector( 0, 0, 58 ),		
	Vector(-13, -13, 0 ),	
	Vector( 13,  13,  36 ),

	Vector( 0, 0, 58 ),		
	Vector(-13, -13, 0 ),	
	Vector( 13,  13,  36 ),	
													
	Vector(-16, -16, 0 ),		//VEC_DUCK_HULL_MIN
	Vector( 16,  16, 45 ),		//VEC_DUCK_HULL_MAX
	Vector( 0, 0, 34 ),			//VEC_DUCK_VIEW
													
	//Vector(-13, -13, -13 ),		//VEC_OBS_HULL_MIN
	//Vector( 13,  13,  13 ),		//VEC_OBS_HULL_MAX

	Vector(-13, -13, 0 ),		//VEC_OBS_HULL_MIN
	Vector( 13,  13,  58 ),		//VEC_OBS_HULL_MAX
													
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

CSDKGameRules::CSDKGameRules()
{
	g_IOSRand.SetSeed(Plat_FloatTime() * 1000);

#ifdef GAME_DLL
	m_pCurStateInfo = NULL;
	m_nShieldType = SHIELD_NONE;
	m_vShieldPos = vec3_invalid;
	m_flStateTimeLeft = 0;
	m_flLastAwayCheckTime = gpGlobals->curtime;
	m_flNextPenalty = gpGlobals->curtime;
	m_nPenaltyTakingTeam = TEAM_A;
	m_flInjuryTime = 0;
	m_flInjuryTimeStart = -1;
	m_pPrecip = NULL;
	m_nFirstHalfLeftSideTeam = TEAM_A;
	m_nLeftSideTeam = TEAM_A;
	m_nFirstHalfKickOffTeam = TEAM_A;
	m_nKickOffTeam = TEAM_A;
	m_bOffsideLinesEnabled = false;
	m_flOffsideLineBallPosY = 0;
	m_flOffsideLineOffsidePlayerPosY = 0;
	m_flOffsideLineLastOppPlayerPosY = 0;
	m_flMatchStartTime = gpGlobals->curtime;
	m_bUseAdjustedStateEnterTime = false;
	m_flAdjustedStateEnterTime = -FLT_MAX;
	m_flTimeoutEnd = 0;
	m_bAdminWantsTimeout = false;
	m_nOldMaxplayers = mp_maxplayers.GetInt();
#else
	PrecacheMaterial("pitch/offside_line");
	m_pOffsideLineMaterial = materials->FindMaterial( "pitch/offside_line", TEXTURE_GROUP_CLIENT_EFFECTS );
#endif
}

#ifdef CLIENT_DLL


#else

// --------------------------------------------------------------------------------------------------- //
// Voice helper
// --------------------------------------------------------------------------------------------------- //

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		return SDKGameRules()->PlayerRelationship( pListener, pTalker, MM_NONE ) == GR_TEAMMATE;
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

void CSDKGameRules::ServerActivate()
{
	CPlayerPersistentData::ReallocateAllPlayerData();

	CTeamKitInfo::FindTeamKits();

	InitTeams();

	CBaseEntity *pEnt = gEntList.FindEntityByClassname(NULL, "info_ball_start");
	if (!pEnt)
		Error("'info_ball_start' is missing");

	trace_t tr;
	UTIL_TraceLine(pEnt->GetLocalOrigin() + Vector(0, 0, 100), pEnt->GetLocalOrigin() + Vector(0, 0, -500), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr);

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

	if (minX == -FLT_MAX || maxX == FLT_MAX)
		Error("Can't calculate the field width. 'trigger_SideLine' missing or misplaced.");

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

	if (minY == -FLT_MAX || maxY == FLT_MAX)
		Error("Can't calculate the field length. 'trigger_goal' missing or misplaced");

	m_vFieldMin = Vector(minX, minY, m_vKickOff.GetZ());
	m_vFieldMax = Vector(maxX, maxY, m_vKickOff.GetZ());

	GetGlobalTeam(TEAM_A)->InitFieldSpots(TEAM_A);
	GetGlobalTeam(TEAM_B)->InitFieldSpots(TEAM_B);

	m_pPrecip = (CPrecipitation *)CreateEntityByName("func_precipitation");
	m_pPrecip->SetType(PRECIPITATION_TYPE_NONE);
	m_pPrecip->Spawn();

	m_nFirstHalfLeftSideTeam = g_IOSRand.RandomInt(TEAM_A, TEAM_B);
	m_nFirstHalfKickOffTeam = g_IOSRand.RandomInt(TEAM_A, TEAM_B);

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
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "player_manager", vec3_origin, vec3_angle );

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

ConVar mp_ball_player_collision("mp_ball_player_collision", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY | FCVAR_REPLICATED);

bool CSDKGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		swap(collisionGroup0,collisionGroup1);
	}

	if (collisionGroup0 == COLLISION_GROUP_NONSOLID_PLAYER || collisionGroup1 == COLLISION_GROUP_NONSOLID_PLAYER)
		return false;

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

const char *CSDKGameRules::GetChatPrefix( MessageMode_t messageMode, CBasePlayer *pPlayer )
{
	if (!pPlayer)
		return "";

	if (ToSDKPlayer(pPlayer)->GetSpecTeam() == TEAM_A)
		return "HOME";
	else if (ToSDKPlayer(pPlayer)->GetSpecTeam() == TEAM_B)
		return "AWAY";
	else
		return "SPEC";
}

const char *CSDKGameRules::GetChatFormat(MessageMode_t messageMode, CBasePlayer *pPlayer)
{
	const char *pszFormat = NULL;

	if (!pPlayer)  // dedicated server output
	{
		pszFormat = "SDK_Chat_StadiumAnnouncer";
	}
	else if (messageMode == MM_SAY_TEAM)
	{
		if (ToSDKPlayer(pPlayer)->GetSpecTeam() == TEAM_SPECTATOR)
			pszFormat = "SDK_Chat_Spec";
		else
			pszFormat = "SDK_Chat_Team_Loc";
	}
	else if (messageMode == MM_SAY_SPEC)
	{
		if (pPlayer->GetTeamNumber() == TEAM_SPECTATOR)
			pszFormat = "SDK_Chat_Spec";
	}
	else
	{
		if (ToSDKPlayer(pPlayer)->GetSpecTeam() == TEAM_SPECTATOR)
			pszFormat = "SDK_Chat_AllSpec";
		else
			pszFormat = "SDK_Chat_All_Loc";
	}

	return pszFormat;
}

const char *CSDKGameRules::GetChatLocation( MessageMode_t messageMode, CBasePlayer *pPlayer )
{
	if (!pPlayer)
		return "";

	if (pPlayer->GetTeamNumber() == TEAM_SPECTATOR && ToSDKPlayer(pPlayer)->GetSpecTeam() != TEAM_SPECTATOR)
	{
		static char bench[6];
		Q_strncpy(bench, "BENCH", 6);
		return bench;
	}

	return g_szPosNames[(int)g_Positions[mp_maxplayers.GetInt() - 1][ToSDKPlayer(pPlayer)->GetTeamPosIndex()][POS_TYPE]];
}

bool CSDKGameRules::PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker, MessageMode_t messageMode )
{
	return ( PlayerRelationship( pListener, pSpeaker, messageMode ) == GR_TEAMMATE );
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Find the relationship between players (teamplay vs. deathmatch)
//-----------------------------------------------------------------------------
int CSDKGameRules::PlayerRelationship( CBaseEntity *pTarget, CBaseEntity *pSource, MessageMode_t messageMode )
{
#ifndef CLIENT_DLL
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if (!pSource || !pSource->IsPlayer() || !pTarget || !pTarget->IsPlayer())
		return GR_NOTTEAMMATE;

	CSDKPlayer *pSDKSource = dynamic_cast<CSDKPlayer *>(pSource);
	CSDKPlayer *pSDKTarget = dynamic_cast<CSDKPlayer *>(pTarget);

	if (messageMode == MM_SAY_SPEC)
	{
		if (pSDKSource->GetSpecTeam() == TEAM_SPECTATOR && pSDKTarget->GetSpecTeam() == TEAM_SPECTATOR)
			return GR_TEAMMATE;

		if ((pSDKSource->GetTeamNumber() == TEAM_SPECTATOR && pSDKSource->GetSpecTeam() != TEAM_SPECTATOR) && pSDKTarget->GetTeamNumber() == TEAM_SPECTATOR)
			return GR_TEAMMATE;
	}
	else
	{
		if (pSDKSource->GetSpecTeam() == pSDKTarget->GetSpecTeam())
			return GR_TEAMMATE;

		if (pSDKSource->GetSpecTeam() == TEAM_SPECTATOR && pSDKTarget->GetTeamNumber() == TEAM_SPECTATOR);
			return GR_TEAMMATE;
	}
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
	if (pPl)
	{
		if (pPl->GetPlayerBall())
			pPl->GetPlayerBall()->RemovePlayerBall();

		pPl->SetConnected(PlayerDisconnecting);

		// Remove the player from his team
		if (pPl->GetTeam())
			pPl->SetDesiredTeam(TEAM_UNASSIGNED, TEAM_SPECTATOR, 0, true, false);
	}

	BaseClass::ClientDisconnected( pClient );
}

void CSDKGameRules::RestartMatch(bool setRandomKickOffTeam, bool setRandomSides)
{
	if (setRandomKickOffTeam)
		m_nFirstHalfKickOffTeam = g_IOSRand.RandomInt(TEAM_A, TEAM_B);

	if (setRandomSides)
		m_nFirstHalfLeftSideTeam = g_IOSRand.RandomInt(TEAM_A, TEAM_B);

	SDKGameRules()->State_Transition(MATCH_WARMUP);
}

#endif


#ifndef CLIENT_DLL

void CC_SV_MatchMinute(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (args.ArgC() < 2)
		return;

	SDKGameRules()->SetMatchDisplayTimeSeconds(atoi(args[1]) * 60);
}

ConCommand sv_matchminute( "sv_matchminute", CC_SV_MatchMinute, "Set match minute", 0 );

void CC_SV_MatchGoalsHome(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (args.ArgC() < 2)
		return;

	GetGlobalTeam(TEAM_A)->SetGoals(atoi(args[1]));
}

ConCommand sv_matchgoalshome( "sv_matchgoalshome", CC_SV_MatchGoalsHome, "", 0 );

void CC_SV_MatchGoalsAway(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (args.ArgC() < 2)
		return;

	GetGlobalTeam(TEAM_B)->SetGoals(atoi(args[1]));
}

ConCommand sv_matchgoalsaway( "sv_matchgoalsaway", CC_SV_MatchGoalsAway, "", 0 );

void CC_SV_StartTimeout(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (SDKGameRules()->IsIntermissionState() || SDKGameRules()->AdminWantsTimeout() || GetGlobalTeam(TEAM_A)->WantsTimeout()
		|| GetGlobalTeam(TEAM_B)->WantsTimeout() || SDKGameRules()->GetTimeoutEnd() != 0)
		return;

	SDKGameRules()->SetAdminWantsTimeout(true);

	IGameEvent *pEvent = gameeventmanager->CreateEvent("timeout_pending");
	if (pEvent)
	{
		pEvent->SetInt("requesting_team", TEAM_UNASSIGNED);
		gameeventmanager->FireEvent(pEvent);
	}
}

ConCommand sv_starttimeout( "sv_starttimeout", CC_SV_StartTimeout, "", 0 );

void CC_SV_EndTimeout(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (SDKGameRules()->IsIntermissionState())
		return;

	if (SDKGameRules()->AdminWantsTimeout() || GetGlobalTeam(TEAM_A)->WantsTimeout() || GetGlobalTeam(TEAM_B)->WantsTimeout()) // Timeout pending
	{
		SDKGameRules()->SetAdminWantsTimeout(false);
		GetGlobalTeam(TEAM_A)->SetWantsTimeout(false);
		GetGlobalTeam(TEAM_B)->SetWantsTimeout(false);

		IGameEvent *pEvent = gameeventmanager->CreateEvent("end_timeout");
		if (pEvent)
		{
			gameeventmanager->FireEvent(pEvent);
		}
	}
	else if (SDKGameRules()->GetTimeoutEnd() != 0) // Timeout running
	{
		SDKGameRules()->SetTimeoutEnd(gpGlobals->curtime + 5);

		IGameEvent *pEvent = gameeventmanager->CreateEvent("start_timeout");
		if (pEvent)
		{
			pEvent->SetInt("requesting_team", TEAM_UNASSIGNED);
			gameeventmanager->FireEvent(pEvent);
		}
	}
}

ConCommand sv_endtimeout( "sv_endtimeout", CC_SV_EndTimeout, "", 0 );

void CC_SV_ResumeMatch(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (args.ArgC() < 4)
	{
		Msg("Usage: sv_resumematch <goals home team> <goals away team> <match minute>\nExample: sv_resumematch 3 2 67\n");
		return;
	}

	SDKGameRules()->SetMatchDisplayTimeSeconds(atoi(args[3]) * 60);
	GetGlobalTeam(TEAM_A)->SetGoals(atoi(args[1]));
	GetGlobalTeam(TEAM_B)->SetGoals(atoi(args[2]));
}

ConCommand sv_resumematch( "sv_resumematch", CC_SV_ResumeMatch, "", 0 );

void CC_SV_Restart(const CCommand &args)
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;

	if (args.ArgC() > 1)
		mp_timelimit_warmup.SetValue((float)atof(args[1]));

	bool setRandomKickOffTeam;

	if (args.ArgC() > 2)
		setRandomKickOffTeam = (atoi(args[2]) != 0);
	else
		setRandomKickOffTeam = false;

	bool setRandomSides;

	if (args.ArgC() > 3)
		setRandomSides = (atoi(args[3]) != 0);
	else
		setRandomSides = false;

	SDKGameRules()->RestartMatch(setRandomKickOffTeam, setRandomSides);
}

ConCommand sv_restart( "sv_restart", CC_SV_Restart, "Restart game", 0 );

int CSDKGameRules::WakeUpAwayPlayers()
{
	int awayPlayerCount = 0;
	char wakeUpString[2048];
	
	if (State_Get() == MATCH_WARMUP)
		Q_strncpy(wakeUpString, "The match starts when the following players start moving: ", sizeof(wakeUpString));
	else
		Q_strncpy(wakeUpString, "The match continues when the following players start moving: ", sizeof(wakeUpString));

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (pPl->IsAway())
		{
			awayPlayerCount += 1;

			if (awayPlayerCount > 1)
				Q_strcat(wakeUpString, ", ", sizeof(wakeUpString));

			Q_strcat(wakeUpString, pPl->GetPlayerName(), sizeof(wakeUpString));
		}
	}

	if (awayPlayerCount > 0)
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent("wakeupcall");
		if (pEvent)
			gameeventmanager->FireEvent(pEvent);
	}
	else
		Q_strncpy(wakeUpString, "All players are awake", sizeof(wakeUpString));

	UTIL_ClientPrintAll(HUD_PRINTTALK, wakeUpString);

	return awayPlayerCount;
}

void CC_SV_WakeUpCall(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	SDKGameRules()->WakeUpAwayPlayers();
	UTIL_ClientPrintAll(HUD_PRINTCENTER, "Please move if you're here!");
}

ConCommand sv_wakeupcall("sv_wakeupcall", CC_SV_WakeUpCall, "Wake up all players", 0);

ConVar sv_wakeupcall_interval("sv_wakeupcall_interval", "10", FCVAR_NOTIFY);

void CSDKGameRules::StartPenalties()
{
	//SetLeftSideTeam(g_IOSRand.RandomInt(TEAM_A, TEAM_B));
	ResetMatch();
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

ConVar sv_replays("sv_replays", "1", FCVAR_NOTIFY);
ConVar sv_replay_count("sv_replay_count", "2", FCVAR_NOTIFY);
ConVar sv_replay_duration1("sv_replay_duration1", "6", FCVAR_NOTIFY);
ConVar sv_replay_duration2("sv_replay_duration2", "4", FCVAR_NOTIFY);
ConVar sv_replay_duration3("sv_replay_duration3", "4", FCVAR_NOTIFY);
ConVar sv_highlights("sv_highlights", "1", FCVAR_NOTIFY);

static void OnMaxPlayersChange(IConVar *var, const char *pOldValue, float flOldValue)
{
#ifdef GAME_DLL
	if (SDKGameRules())
		SDKGameRules()->SetOldMaxplayers(atoi(pOldValue));

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		pPl->SetDesiredTeam(TEAM_SPECTATOR, pPl->GetTeamNumber(), 0, true, false);
	}

	for (int team = TEAM_A; team <= TEAM_B; team++)
	{
		if (GetGlobalTeam(team))
			GetGlobalTeam(team)->UpdatePosIndices(true);
	}
#endif
}

ConVar mp_maxplayers("mp_maxplayers", "7", FCVAR_NOTIFY|FCVAR_REPLICATED, "Maximum number of players per team <1-11>", true, 1, true, 11, OnMaxPlayersChange);

ConVar sv_autostartmatch("sv_autostartmatch", "1", FCVAR_NOTIFY|FCVAR_REPLICATED, "");
ConVar sv_awaytime_warmup("sv_awaytime_warmup", "30", FCVAR_NOTIFY);
ConVar sv_awaytime_warmup_autospec("sv_awaytime_warmup_autospec", "180", FCVAR_NOTIFY);

#ifdef GAME_DLL

ConVar sv_playerrotation_enabled("sv_playerrotation_enabled", "0", FCVAR_NOTIFY);
ConVar sv_playerrotation_minutes("sv_playerrotation_minutes", "30,60", FCVAR_NOTIFY);

void CSDKGameRules::State_Transition( match_state_t newState )
{
	State_Leave(newState);
	State_Enter( newState );
}

void CSDKGameRules::State_Enter( match_state_t newState )
{
	m_eMatchState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	if (m_bUseAdjustedStateEnterTime)
	{
		m_flStateEnterTime = m_flAdjustedStateEnterTime;
		m_bUseAdjustedStateEnterTime = false;
	}
	else
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

		// Remove the player if he's in a team but card banned or in a blocked position
		if (!IsIntermissionState()
			&& (pPl->GetTeamNumber() == TEAM_A || pPl->GetTeamNumber() == TEAM_B)
			&& (GetMatchDisplayTimeSeconds() < pPl->GetNextCardJoin()
			|| GetMatchDisplayTimeSeconds() < pPl->GetTeam()->GetPosNextJoinSeconds(pPl->GetTeamPosIndex())))
		{
			pPl->SetDesiredTeam(TEAM_SPECTATOR, pPl->GetTeamNumber(), 0, true, false);
		}

		if (!IsIntermissionState())
		{
			pPl->GetPlayerData()->StartNewMatchPeriod();
		}
	}

	if (!IsIntermissionState())
	{
		GetBall()->RemoveAllPlayerBalls();

		GetBall()->StopSound("Crowd.Background1");
		GetBall()->StopSound("Crowd.Background2");
		GetBall()->EmitSound("Crowd.Background1");
		GetBall()->EmitSound("Crowd.Background2");
		GetBall()->StopSound("Crowd.Song");
		GetBall()->EmitSound("Crowd.Song");
	}

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		(this->*m_pCurStateInfo->pfnEnterState)();
	}
}

void CSDKGameRules::State_Leave(match_state_t newState)
{
	if (IsIntermissionState())
	{
		ReplayManager()->StopHighlights();
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (!IsIntermissionState())
		{
			pPl->GetPlayerData()->EndCurrentMatchPeriod();
		}
	}

	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)(newState);
	}
}

void CSDKGameRules::State_Think()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnThink )
	{
		if (GetBall())
			m_nBallZone = GetBall()->CalcFieldZone();

		if (m_pCurStateInfo->m_eMatchState == MATCH_WARMUP && mp_timelimit_warmup.GetFloat() < 0)
			m_flStateTimeLeft = 1.0f;
		else
			m_flStateTimeLeft = (m_flStateEnterTime + m_pCurStateInfo->m_MinDurationConVar->GetFloat() * 60 / m_pCurStateInfo->m_flMinDurationDivisor) - gpGlobals->curtime;

		if (!IsIntermissionState())
		{
			if (g_IOSRand.RandomInt(0, 2400 * (1.0f / gpGlobals->interval_per_tick)) == 0)
			{
				GetBall()->EmitSound("Crowd.Vuvuzela");
				GetBall()->EmitSound("Crowd.Vuvuzela");
				GetBall()->EmitSound("Crowd.Vuvuzela");
			}

			if (g_IOSRand.RandomInt(0, 240 * (1.0f / gpGlobals->interval_per_tick)) == 0)
			{
				GetBall()->EmitSound("Crowd.Cheer");
			}

			int additionalTime = m_nAnnouncedInjuryTime + (abs(m_nBallZone) < 50 ? 0 : 30);
			m_flStateTimeLeft += m_flInjuryTime + additionalTime * 60 / (90.0f / mp_timelimit_match.GetFloat());

			if (sv_playerrotation_enabled.GetBool() && m_PlayerRotationMinutes.Count() > 0 && GetMatchDisplayTimeSeconds(false) / 60 >= m_PlayerRotationMinutes.Head())
			{
				for (int team = TEAM_A; team <= TEAM_B; team++)
				{
					CTeam *pTeam = GetGlobalTeam(team);

					for (int i = 0; i < mp_maxplayers.GetInt(); i++)
					{
						CSDKPlayer *pPl1 = pTeam->GetPlayerByPosIndex(i);
						if (!pPl1)
							continue;

						int j = (i + 1) % mp_maxplayers.GetInt();

						while (j != i)
						{
							CSDKPlayer *pPl2 = pTeam->GetPlayerByPosIndex(j);
							if (pPl2)
							{
								int posIndex = pPl1->GetTeamPosIndex();
								pPl1->SetDesiredTeam(team, team, pPl2->GetTeamPosIndex(), true, false);
								pPl2->SetDesiredTeam(team, team, posIndex, true, false);

								Vector pos = pPl1->GetLocalOrigin();
								pPl1->SetLocalOrigin(pPl2->GetLocalOrigin());
								pPl2->SetLocalOrigin(pos);

								break;
							}

							j = (j + 1) % mp_maxplayers.GetInt();
						}
					}
				}

				m_PlayerRotationMinutes.Remove(0);
			}
		}

		(this->*m_pCurStateInfo->pfnThink)();
	}
}

CSDKGameRulesStateInfo* CSDKGameRules::State_LookupInfo( match_state_t state )
{
	static CSDKGameRulesStateInfo gameRulesStateInfos[] =
	{
		{ MATCH_WARMUP,						"MATCH_WARMUP",						&CSDKGameRules::State_WARMUP_Enter,					&CSDKGameRules::State_WARMUP_Think,					&CSDKGameRules::State_WARMUP_Leave,					&mp_timelimit_warmup, 1	},
		{ MATCH_FIRST_HALF,					"MATCH_FIRST_HALF",					&CSDKGameRules::State_FIRST_HALF_Enter,				&CSDKGameRules::State_FIRST_HALF_Think,				&CSDKGameRules::State_FIRST_HALF_Leave,				&mp_timelimit_match, 2 },
		{ MATCH_HALFTIME,					"MATCH_HALFTIME",					&CSDKGameRules::State_HALFTIME_Enter,				&CSDKGameRules::State_HALFTIME_Think,				&CSDKGameRules::State_HALFTIME_Leave,				&mp_timelimit_halftime, 1 },
		{ MATCH_SECOND_HALF,				"MATCH_SECOND_HALF",				&CSDKGameRules::State_SECOND_HALF_Enter,			&CSDKGameRules::State_SECOND_HALF_Think,			&CSDKGameRules::State_SECOND_HALF_Leave,			&mp_timelimit_match, 2 },
		{ MATCH_EXTRATIME_INTERMISSION,		"MATCH_EXTRATIME_INTERMISSION",		&CSDKGameRules::State_EXTRATIME_INTERMISSION_Enter, &CSDKGameRules::State_EXTRATIME_INTERMISSION_Think,	&CSDKGameRules::State_EXTRATIME_INTERMISSION_Leave,	&mp_timelimit_extratime_intermission, 1	},
		{ MATCH_EXTRATIME_FIRST_HALF,		"MATCH_EXTRATIME_FIRST_HALF",		&CSDKGameRules::State_EXTRATIME_FIRST_HALF_Enter,	&CSDKGameRules::State_EXTRATIME_FIRST_HALF_Think,	&CSDKGameRules::State_EXTRATIME_FIRST_HALF_Leave,	&mp_timelimit_match, 6 },
		{ MATCH_EXTRATIME_HALFTIME,			"MATCH_EXTRATIME_HALFTIME",			&CSDKGameRules::State_EXTRATIME_HALFTIME_Enter,		&CSDKGameRules::State_EXTRATIME_HALFTIME_Think,		&CSDKGameRules::State_EXTRATIME_HALFTIME_Leave,		&mp_timelimit_extratime_halftime, 1 },
		{ MATCH_EXTRATIME_SECOND_HALF,		"MATCH_EXTRATIME_SECOND_HALF",		&CSDKGameRules::State_EXTRATIME_SECOND_HALF_Enter,	&CSDKGameRules::State_EXTRATIME_SECOND_HALF_Think,	&CSDKGameRules::State_EXTRATIME_SECOND_HALF_Leave,	&mp_timelimit_match, 6 },
		{ MATCH_PENALTIES_INTERMISSION,		"MATCH_PENALTIES_INTERMISSION",		&CSDKGameRules::State_PENALTIES_INTERMISSION_Enter, &CSDKGameRules::State_PENALTIES_INTERMISSION_Think,	&CSDKGameRules::State_PENALTIES_INTERMISSION_Leave,	&mp_timelimit_penalties_intermission, 1 },
		{ MATCH_PENALTIES,					"MATCH_PENALTIES",					&CSDKGameRules::State_PENALTIES_Enter,				&CSDKGameRules::State_PENALTIES_Think,				&CSDKGameRules::State_PENALTIES_Leave,				&mp_timelimit_match, 3 },
		{ MATCH_COOLDOWN,					"MATCH_COOLDOWN",					&CSDKGameRules::State_COOLDOWN_Enter,				&CSDKGameRules::State_COOLDOWN_Think,				&CSDKGameRules::State_COOLDOWN_Leave,				&mp_timelimit_cooldown, 1 },
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
	m_flMatchStartTime = gpGlobals->curtime;
	ResetMatch();
	ApplyIntermissionSettings(false);
}

void CSDKGameRules::State_WARMUP_Think()
{
	if (m_flStateTimeLeft <= 0 || CheckAutoStart())
		State_Transition(MATCH_FIRST_HALF);
}

void CSDKGameRules::State_WARMUP_Leave(match_state_t newState)
{
	GetBall()->EmitSound("Ball.Whistle");
	GetBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_FIRST_HALF_Enter()
{
	ReloadSettings();
	GetBall()->State_Transition(BALL_KICKOFF, 0, true);

	IGameEvent *pEvent = gameeventmanager->CreateEvent("wakeupcall");
	if (pEvent)
		gameeventmanager->FireEvent(pEvent);

	UTIL_ClientPrintAll(HUD_PRINTTALK, "#game_match_start");

	GetBall()->EmitSound("Crowd.YNWA");
}

void CSDKGameRules::State_FIRST_HALF_Think()
{
	if ((45 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = g_IOSRand.RandomInt(1, 4);
	}
	else if (m_flStateTimeLeft <= 0 && GetBall()->State_Get() == BALL_NORMAL && !GetBall()->HasQueuedState())
	{
		State_Transition(MATCH_HALFTIME);
	}
}

void CSDKGameRules::State_FIRST_HALF_Leave(match_state_t newState)
{
	GetBall()->EmitSound("Ball.Whistle");
	GetBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_HALFTIME_Enter()
{
	ApplyIntermissionSettings(true);
}

void CSDKGameRules::State_HALFTIME_Think()
{
	if (m_flStateTimeLeft <= 0)
		State_Transition(MATCH_SECOND_HALF);
}

void CSDKGameRules::State_HALFTIME_Leave(match_state_t newState)
{
}

void CSDKGameRules::State_SECOND_HALF_Enter()
{
	GetBall()->State_Transition(BALL_KICKOFF, 0, true);
	GetBall()->EmitSound("Crowd.YNWA");
}

void CSDKGameRules::State_SECOND_HALF_Think()
{
	if ((90 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = g_IOSRand.RandomInt(1, 4);
	}
	else if (m_flStateTimeLeft <= 0 && GetBall()->State_Get() == BALL_NORMAL && !GetBall()->HasQueuedState())
	{
		if (mp_extratime.GetBool() && GetGlobalTeam(TEAM_A)->GetGoals() == GetGlobalTeam(TEAM_B)->GetGoals())
			State_Transition(MATCH_EXTRATIME_INTERMISSION);
		else if (mp_penalties.GetBool() && GetGlobalTeam(TEAM_A)->GetGoals() == GetGlobalTeam(TEAM_B)->GetGoals())
			State_Transition(MATCH_PENALTIES_INTERMISSION);
		else
			State_Transition(MATCH_COOLDOWN);
	}
}

void CSDKGameRules::State_SECOND_HALF_Leave(match_state_t newState)
{
	GetBall()->EmitSound("Ball.Whistle");
	GetBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_EXTRATIME_INTERMISSION_Enter()
{
	ApplyIntermissionSettings(false);
}

void CSDKGameRules::State_EXTRATIME_INTERMISSION_Think()
{
	if (m_flStateTimeLeft <= 0)
		State_Transition(MATCH_EXTRATIME_FIRST_HALF);
}

void CSDKGameRules::State_EXTRATIME_INTERMISSION_Leave(match_state_t newState)
{
}

void CSDKGameRules::State_EXTRATIME_FIRST_HALF_Enter()
{
	GetBall()->State_Transition(BALL_KICKOFF, 0, true);
}

void CSDKGameRules::State_EXTRATIME_FIRST_HALF_Think()
{
	if ((105 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = g_IOSRand.RandomInt(1, 4);
	}
	else if (m_flStateTimeLeft <= 0 && GetBall()->State_Get() == BALL_NORMAL && !GetBall()->HasQueuedState())
	{
		State_Transition(MATCH_EXTRATIME_HALFTIME);
	}
}

void CSDKGameRules::State_EXTRATIME_FIRST_HALF_Leave(match_state_t newState)
{
	GetBall()->EmitSound("Ball.Whistle");
	GetBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_EXTRATIME_HALFTIME_Enter()
{
	ApplyIntermissionSettings(true);
}

void CSDKGameRules::State_EXTRATIME_HALFTIME_Think()
{
	if (m_flStateTimeLeft <= 0)
		State_Transition(MATCH_EXTRATIME_SECOND_HALF);
}

void CSDKGameRules::State_EXTRATIME_HALFTIME_Leave(match_state_t newState)
{
}

void CSDKGameRules::State_EXTRATIME_SECOND_HALF_Enter()
{
	GetBall()->State_Transition(BALL_KICKOFF, 0, true);
}

void CSDKGameRules::State_EXTRATIME_SECOND_HALF_Think()
{
	if ((120 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == 0)
	{
		m_nAnnouncedInjuryTime = g_IOSRand.RandomInt(1, 4);
	}
	else if (m_flStateTimeLeft <= 0 && GetBall()->State_Get() == BALL_NORMAL && !GetBall()->HasQueuedState())
	{
		if (mp_penalties.GetBool() && GetGlobalTeam(TEAM_A)->GetGoals() == GetGlobalTeam(TEAM_B)->GetGoals())
			State_Transition(MATCH_PENALTIES_INTERMISSION);
		else
			State_Transition(MATCH_COOLDOWN);
	}
}

void CSDKGameRules::State_EXTRATIME_SECOND_HALF_Leave(match_state_t newState)
{
	GetBall()->EmitSound("Ball.Whistle");
	GetBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_PENALTIES_INTERMISSION_Enter()
{
	ApplyIntermissionSettings(false);
}

void CSDKGameRules::State_PENALTIES_INTERMISSION_Think()
{
	if (m_flStateTimeLeft <= 0)
		State_Transition(MATCH_PENALTIES);
}

void CSDKGameRules::State_PENALTIES_INTERMISSION_Leave(match_state_t newState)
{
}

void CSDKGameRules::State_PENALTIES_Enter()
{
	for (int i = 0; i < 2; i++)
	{
		GetGlobalTeam(TEAM_A + i)->m_nPenaltyGoals = 0;
		GetGlobalTeam(TEAM_A + i)->m_nPenaltyGoalBits = 0;
		GetGlobalTeam(TEAM_A + i)->m_nPenaltyRound = 0;
	}

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

	if (GetBall()->GetPenaltyState() == PENALTY_KICKED
		|| GetBall()->GetPenaltyState() == PENALTY_SCORED
		|| GetBall()->GetPenaltyState() == PENALTY_SAVED
		|| GetBall()->GetPenaltyState() == PENALTY_ABORTED_NO_KEEPER
		|| GetBall()->GetPenaltyState() == PENALTY_ABORTED_ILLEGAL_MOVE)
	{
		if (m_flNextPenalty == -1)
		{
			m_flNextPenalty = gpGlobals->curtime + 3;
		}
		else if (m_flNextPenalty <= gpGlobals->curtime)
		{
			if (GetBall()->GetPenaltyState() == PENALTY_KICKED
				|| GetBall()->GetPenaltyState() == PENALTY_SCORED
				|| GetBall()->GetPenaltyState() == PENALTY_SAVED
				|| GetBall()->GetPenaltyState() == PENALTY_ABORTED_ILLEGAL_MOVE)
			{
				GetBall()->State_Transition(BALL_NORMAL, 0, true);

				if (GetBall()->GetPenaltyState() == PENALTY_SCORED)
				{
					GetGlobalTeam(m_nPenaltyTakingTeam)->m_Goals += 1;
					GetGlobalTeam(m_nPenaltyTakingTeam)->m_nPenaltyGoals += 1;
					GetGlobalTeam(m_nPenaltyTakingTeam)->m_nPenaltyGoalBits |= (1 << GetGlobalTeam(m_nPenaltyTakingTeam)->m_nPenaltyRound);
				}
				
				GetGlobalTeam(m_nPenaltyTakingTeam)->m_nPenaltyRound += 1;

				if (GetGlobalTeam(m_nPenaltyTakingTeam)->m_nPenaltyRound <= 5)
				{
					int goalDiff = GetGlobalTeam(m_nPenaltyTakingTeam)->m_nPenaltyGoals - GetGlobalTeam(m_nPenaltyTakingTeam)->GetOppTeam()->m_nPenaltyGoals;

					if (goalDiff != 0 && (goalDiff > 5 - GetGlobalTeam(m_nPenaltyTakingTeam)->GetOppTeam()->m_nPenaltyRound || -goalDiff > 5 - GetGlobalTeam(m_nPenaltyTakingTeam)->m_nPenaltyRound))
					{
						State_Transition(MATCH_COOLDOWN);
						return;
					}
				}
				else
				{
					if (m_nPenaltyTakingTeam != m_nPenaltyTakingStartTeam && GetGlobalTeam(TEAM_A)->GetGoals() != GetGlobalTeam(TEAM_B)->GetGoals())
					{
						State_Transition(MATCH_COOLDOWN);
						return;
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

				if (pPl->GetTeamNumber() != m_nPenaltyTakingTeam || pPl->m_ePenaltyState == PENALTY_KICKED || pPl->GetTeamPosType() == POS_GK && pPl->IsBot())
					continue;

				pPenTaker = pPl;
				break;
			}

			if (pPenTaker)
			{
				IGameEvent *pEvent = gameeventmanager->CreateEvent("penalty");
				if (pEvent)
				{
					pEvent->SetInt("taking_team", m_nPenaltyTakingTeam);
					pEvent->SetInt("taking_player_userid", pPenTaker->GetUserID());
					gameeventmanager->FireEvent(pEvent);
				}

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

					if (!CSDKPlayer::IsOnField(pPl) || pPl->GetTeamNumber() != m_nPenaltyTakingTeam)
						continue;

					pPl->m_ePenaltyState = PENALTY_NONE;
				}
			}
		}

		State_Transition(MATCH_COOLDOWN);
	}
}

void CSDKGameRules::State_PENALTIES_Leave(match_state_t newState)
{
	GetBall()->EmitSound("Ball.Whistle");
	GetBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_COOLDOWN_Enter()
{
	ApplyIntermissionSettings(false);

	//who won?
	int winners = 0;
	int scoreA = GetGlobalTeam( TEAM_A )->GetGoals();
	int scoreB = GetGlobalTeam( TEAM_B )->GetGoals();
	if (scoreA > scoreB)
		winners = TEAM_A;
	if (scoreB > scoreA)
		winners = TEAM_B;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSDKPlayer *pPlayer = (CSDKPlayer*)UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		//is this player on the winning team
		if (pPlayer->GetTeamNumber() == winners)
		{
			pPlayer->AddFlag (FL_CELEB);
		}
	}

	CPlayerPersistentData::ConvertAllPlayerDataToJson();
}

void CSDKGameRules::State_COOLDOWN_Think()
{
	if (m_flStateTimeLeft <= 0)
		GoToIntermission();
}

void CSDKGameRules::State_COOLDOWN_Leave(match_state_t newState)
{
}

void CSDKGameRules::ApplyIntermissionSettings(bool swapTeams)
{
	IGameEvent *pEvent = gameeventmanager->CreateEvent("match_state");
	if (pEvent)
	{
		pEvent->SetInt("state", State_Get());
		gameeventmanager->FireEvent(pEvent);
	}

	GetBall()->State_Transition(BALL_STATIC, 0, true);
	GetBall()->SetPos(m_vKickOff);

	if (swapTeams)
	{
		SetLeftSideTeam(GetGlobalTeam(m_nFirstHalfLeftSideTeam)->GetOppTeamNumber());
		SetKickOffTeam(GetGlobalTeam(m_nFirstHalfKickOffTeam)->GetOppTeamNumber());
	}
	else
	{
		SetLeftSideTeam(m_nFirstHalfLeftSideTeam);
		SetKickOffTeam(m_nFirstHalfKickOffTeam);
	}

	EnableShield(SHIELD_KICKOFF, TEAM_A, SDKGameRules()->m_vKickOff);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->SetPosOutsideShield(false);
		pPl->SetLastMoveTime(gpGlobals->curtime);
		pPl->SetAway(true);
	}

	m_flLastAwayCheckTime = gpGlobals->curtime;

	ReplayManager()->StartHighlights();
}

bool CSDKGameRules::CheckAutoStart()
{
	if (sv_autostartmatch.GetBool()
		&& GetGlobalTeam(TEAM_A)->GetNumPlayers() == mp_maxplayers.GetInt()
		&& GetGlobalTeam(TEAM_B)->GetNumPlayers() == mp_maxplayers.GetInt()
		&& gpGlobals->curtime >= m_flLastAwayCheckTime + sv_wakeupcall_interval.GetFloat())
	{
		m_flLastAwayCheckTime = gpGlobals->curtime;
		return (WakeUpAwayPlayers() == 0);
	}

	return false;
}

#endif

void OnTeamnamesChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	#ifdef GAME_DLL
		if (SDKGameRules())
		{
			char val[256];
			Q_strncpy(val, ((ConVar*)var)->GetString(), sizeof(val));

			if (val[0] == 0)
				return;

			char teamstrings[2][128] = {};
			char *result = NULL;

			result = strtok(val, ",");

			if (result != NULL)
				Q_strncpy(teamstrings[0], result, sizeof(teamstrings[0]));

			if (teamstrings[0][0] != 0)
			{
				result = strtok(NULL, ",");

				if (result != NULL)
					Q_strncpy(teamstrings[1], result, sizeof(teamstrings[1]));
			}

			if (teamstrings[0][0] != 0 && teamstrings[1][0] != 0)
			{
				char codes[2][16] = {};
				char names[2][64] = {};

				result = strtok(teamstrings[0], ":");

				if (result != NULL)
					Q_strncpy(codes[0], result, sizeof(codes[0]));

				if (codes[0][0] != 0)
				{
					result = strtok(NULL, ":");

					if (result != NULL)
						Q_strncpy(names[0], result, sizeof(names[0]));

					if (names[0][0] != 0)
					{
						result = strtok(teamstrings[1], ":");

						if (result != NULL)
							Q_strncpy(codes[1], result, sizeof(codes[1]));

						if (codes[1][0] != 0)
						{
							result = strtok(NULL, ":");

							if (result != NULL)
								Q_strncpy(names[1], result, sizeof(names[1]));

							if (names[1][0] != 0)
							{
								GetGlobalTeam(TEAM_A)->SetTeamCode(codes[0]);
								GetGlobalTeam(TEAM_A)->SetShortTeamName(names[0]);

								GetGlobalTeam(TEAM_B)->SetTeamCode(codes[1]);
								GetGlobalTeam(TEAM_B)->SetShortTeamName(names[1]);

								return;
							}
						}
					}
				}
			}

			Msg("Error: Wrong format\n");
		}
	#endif
}

ConVar mp_teamnames("mp_teamnames", "", FCVAR_REPLICATED|FCVAR_NOTIFY, "Override team names. Example: mp_teamnames \"FCB:FC Barcelona,RMA:Real Madrid\"", &OnTeamnamesChange);

#ifdef GAME_DLL

void OnTeamlistChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	if (SDKGameRules())
	{
		char teamlist[256];
		Q_strncpy(teamlist, ((ConVar*)var)->GetString(), sizeof(teamlist));

		char *home = NULL;
		char *away = NULL;

		home = strtok(teamlist, " ,;");

		if (home != NULL)
			away = strtok(NULL, " ,;");

		if (home == NULL || away == NULL)
			Msg( "Format: mp_teamlist \"<home team>,<away team>\"\n" );
		else
		{
			home = strlwr(home);
			away = strlwr(away);
			IOS_LogPrintf("Setting new teams: %s against %s\n", home, away);
			GetGlobalTeam(TEAM_A)->SetKitName(home);
			GetGlobalTeam(TEAM_B)->SetKitName(away);
		}
	}
}

ConVar mp_teamlist("mp_teamlist", "england,brazil", FCVAR_REPLICATED|FCVAR_NOTIFY, "Set team names", &OnTeamlistChange);

ConVar mp_teamrotation("mp_teamrotation", "brazil,germany,italy,scotland,barcelona,bayern,liverpool,milan,palmeiras", 0, "Set available teams");

ConVar mp_clientsettingschangeinterval("mp_clientsettingschangeinterval", "5", FCVAR_REPLICATED|FCVAR_NOTIFY, "");


void CSDKGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	CSDKPlayer *pPl = ToSDKPlayer(pPlayer);

	if (gpGlobals->curtime < pPl->m_flNextClientSettingsChangeTime || (!IsIntermissionState() && (pPl->GetTeamNumber() == TEAM_A || pPl->GetTeamNumber() == TEAM_B)))
		return;

	pPl->m_flNextClientSettingsChangeTime = gpGlobals->curtime + mp_clientsettingschangeinterval.GetFloat();

	int countryIndex = atoi(engine->GetClientConVarValue(pPl->entindex(), "geoipcountryindex"));

	if (countryIndex <= 0 || countryIndex >= COUNTRY_NAMES_COUNT - 1)
	{
		countryIndex = atoi(engine->GetClientConVarValue(pPl->entindex(), "fallbackcountryindex"));

		if (countryIndex <= 0 || countryIndex >= COUNTRY_NAMES_COUNT - 1)
		{
			countryIndex = 0;
		}
	}

	pPl->SetLegacySideCurl(atoi(engine->GetClientConVarValue(pPl->entindex(), "legacysidecurl")) != 0);

	pPl->SetCountryName(countryIndex);

	pPl->SetPreferredTeamPosNum(atoi(engine->GetClientConVarValue(pPl->entindex(), "preferredshirtnumber")));

	int preferredSkin = atoi(engine->GetClientConVarValue(pPl->entindex(), "modelskinindex"));
	if (preferredSkin != pPl->GetPreferredSkin())
		pPl->SetPreferredSkin(preferredSkin);

	char pszName[MAX_PLAYER_NAME_LENGTH];
	Q_strncpy(pszName, engine->GetClientConVarValue( pPl->entindex(), "playername" ), MAX_PLAYER_NAME_LENGTH);

	const char *pszOldName = pPl->GetPlayerName();

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strlen(pszName) != 0 && Q_strcmp( pszOldName, pszName ) )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
		if ( event )
		{
			event->SetInt( "userid", pPl->GetUserID() );
			event->SetString( "oldname", pszOldName );
			event->SetString( "newname", pszName );
			gameeventmanager->FireEvent( event );
		}
		
		pPl->SetPlayerName( pszName );
	}

	char pszClubName[MAX_CLUBNAME_LENGTH];
	Q_strncpy(pszClubName, engine->GetClientConVarValue( pPl->entindex(), "clubname" ), MAX_CLUBNAME_LENGTH);

	const char *pszOldClubName = pPl->GetClubName();

	if (Q_strcmp(pszOldClubName, pszClubName))
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "player_changeclub" );
		if ( event )
		{
			event->SetInt("userid", pPl->GetUserID());
			event->SetString("oldclub", pszOldClubName);
			event->SetString("newclub", pszClubName);
			gameeventmanager->FireEvent(event);
		}
		
		pPl->SetClubName(pszClubName);
	}
}

void CSDKGameRules::EnableShield(int type, int team, const Vector &pos)
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
		pPl->m_flRemoteControlledStartTime = -1;
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
		//pPl->RemoveSolidFlags(FSOLID_NOT_SOLID);
		pPl->SetCollisionGroup(COLLISION_GROUP_PLAYER);
		pPl->m_bIsAtTargetPos = true;
		pPl->m_flRemoteControlledStartTime = -1;
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

void CSDKGameRules::StartMeteringInjuryTime()
{
	StopMeteringInjuryTime();
	m_flInjuryTimeStart = gpGlobals->curtime;
}

void CSDKGameRules::StopMeteringInjuryTime()
{
	if (m_flInjuryTimeStart != -1)
	{
		float timePassed = gpGlobals->curtime - m_flInjuryTimeStart;
		m_flInjuryTime += timePassed;
		m_flInjuryTimeStart = -1;

		//for (int i = 1; i <= gpGlobals->maxClients; i++)
		//{
		//	CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		//	if (!pPl)
		//		continue;

		//	if (pPl->IsCardBanned() && pPl->GetNextJoin() > gpGlobals->curtime)
		//	{
		//		pPl->SetNextJoin(pPl->GetNextJoin() + timePassed);
		//	}
		//}
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

bool CSDKGameRules::IsInjuryTime()
{
	return (m_nAnnouncedInjuryTime > 0);
}

int CSDKGameRules::GetShieldRadius(int team, bool isTaker)
{
	switch (m_nShieldType)
	{
	case SHIELD_THROWIN: return (team == m_nShieldTeam && !isTaker ? mp_shield_throwin_radius_taker.GetInt() : mp_shield_throwin_radius_opponent.GetInt());
	case SHIELD_FREEKICK: return (team == m_nShieldTeam && !isTaker ? mp_shield_freekick_radius_taker.GetInt() : mp_shield_freekick_radius_opponent.GetInt());
	case SHIELD_CORNER: return (team == m_nShieldTeam && !isTaker ? mp_shield_corner_radius_taker.GetInt() : mp_shield_corner_radius_opponent.GetInt());
	case SHIELD_KICKOFF: return mp_shield_kickoff_radius.GetInt();
	case SHIELD_PENALTY: return mp_shield_kickoff_radius.GetInt();
	default: return 0;
	}
}

int CSDKGameRules::GetMatchDisplayTimeSeconds(bool addInjuryTime /*= true*/, bool getCountdownAtIntermissions /*= true*/)
{
	float flTime = gpGlobals->curtime - SDKGameRules()->m_flStateEnterTime - SDKGameRules()->m_flInjuryTime;
	if (SDKGameRules()->m_flInjuryTimeStart != -1)
		flTime -= gpGlobals->curtime - SDKGameRules()->m_flInjuryTimeStart;
	int nTime;

	switch ( SDKGameRules()->State_Get() )
	{
	case MATCH_PENALTIES:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat())) + (mp_extratime.GetBool() ? 120 : 90) * 60;
		if (!addInjuryTime)
			nTime = min((mp_extratime.GetBool() ? 150 : 120) * 60, nTime);
		break;
	case MATCH_EXTRATIME_SECOND_HALF:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat())) + 105 * 60;
		if (!addInjuryTime)
			nTime = min(120 * 60, nTime);
		break;
	case MATCH_EXTRATIME_FIRST_HALF:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat())) + 90 * 60;
		if (!addInjuryTime)
			nTime = min(105 * 60, nTime);
		break;
	case MATCH_SECOND_HALF:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat())) + 45 * 60;
		if (!addInjuryTime)
			nTime = min(90 * 60, nTime);
		break;
	case MATCH_FIRST_HALF:
		nTime = (int)(flTime * (90.0f / mp_timelimit_match.GetFloat()));
		if (!addInjuryTime)
			nTime = min(45 * 60, nTime);
		break;
	case MATCH_WARMUP:
		if (!getCountdownAtIntermissions)
			nTime = 0;
		else
			nTime = (int)(flTime - mp_timelimit_warmup.GetFloat() * 60);
		break;
	case MATCH_HALFTIME:
		if (!getCountdownAtIntermissions)
			nTime = 45 * 60;
		else
			nTime = (int)(flTime - mp_timelimit_halftime.GetFloat() * 60);
		break;
	case MATCH_EXTRATIME_INTERMISSION:
		if (!getCountdownAtIntermissions)
			nTime = 90 * 60;
		else
			nTime = (int)(flTime - mp_timelimit_extratime_intermission.GetFloat() * 60);
		break;
	case MATCH_EXTRATIME_HALFTIME:
		if (!getCountdownAtIntermissions)
			nTime = 105 * 60;
		else
			nTime = (int)(flTime - mp_timelimit_extratime_halftime.GetFloat() * 60);
		break;
	case MATCH_PENALTIES_INTERMISSION:
		if (!getCountdownAtIntermissions)
			nTime = (mp_extratime.GetBool() ? 120 : 90) * 60;
		else
			nTime = (int)(flTime - mp_timelimit_penalties_intermission.GetFloat() * 60);
		break;
	case MATCH_COOLDOWN:
		if (!getCountdownAtIntermissions)
		{
			int minute;

			if (mp_extratime.GetBool() && mp_penalties.GetBool())
				minute = 150;
			else if (mp_extratime.GetBool())
				minute = 120;
			else if (mp_penalties.GetBool())
				minute = 120;
			else
				minute = 90;

			nTime = minute * 60;
		}
		else
			nTime = (int)(flTime - mp_timelimit_cooldown.GetFloat() * 60);
		break;
	default:
		nTime = 0;
		break;
	}

	return nTime;
}

ConVar mp_daytime_enabled("mp_daytime_enabled", "1", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_daytime_start("mp_daytime_start", "19", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_daytime_speed("mp_daytime_speed", "7", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_daytime_transition("mp_daytime_transition", "1.5", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_daytime_sunrise("mp_daytime_sunrise", "8", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar mp_daytime_sunset("mp_daytime_sunset", "20", FCVAR_NOTIFY | FCVAR_REPLICATED);

#ifdef GAME_DLL

#include "filesystem.h"
#include <time.h>

void FlushLog(FileHandle_t logfile, char *text)
{
	time_t rawtime;
	time(&rawtime);
	struct tm *timeinfo;
	timeinfo = localtime(&rawtime);
	char time[64];
	strftime(time, sizeof(time), "%Y.%m.%d_%Hh.%Mm.%Ss", timeinfo);

	filesystem->FPrintf(logfile, "%s - Gamerules: %s\n", time, text);
	filesystem->Flush(logfile);
}

void CSDKGameRules::ResetMatch()
{
	ReloadSettings();

	SetOffsideLinesEnabled(false);
	DisableShield();
	SetTimeoutEnd(0);
	m_flLastAwayCheckTime = gpGlobals->curtime;
	SetAdminWantsTimeout(false);

	FileHandle_t logfile = filesystem->Open("logs/cleanup.log", "a", "MOD");

	FlushLog(logfile, "Start ball reset");
	GetBall()->Reset();
	FlushLog(logfile, "End ball reset");

	FlushLog(logfile, "Start player flags reset");
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		pPl->ResetFlags();
	}

	FlushLog(logfile, "End player flags reset");

	FlushLog(logfile, "Start team stats reset");
	GetGlobalTeam(TEAM_A)->ResetStats();
	GetGlobalTeam(TEAM_B)->ResetStats();
	FlushLog(logfile, "End team stats reset");

	FlushLog(logfile, "Start player stats reset");
	CPlayerPersistentData::ReallocateAllPlayerData();
	FlushLog(logfile, "End player stats reset");

	FlushLog(logfile, "Start replay clean-up");
	ReplayManager()->CleanUp();
	FlushLog(logfile, "End replay clean-up\n");

	filesystem->Close(logfile);
}

void CSDKGameRules::ReloadSettings()
{
	m_PlayerRotationMinutes.RemoveAll();

	if (sv_playerrotation_enabled.GetBool())
	{
		char minutes[128];
		Q_strncpy(minutes, sv_playerrotation_minutes.GetString(), sizeof(minutes));

		char *pch = strtok(minutes, " ,;");
		while (pch)
		{
			m_PlayerRotationMinutes.AddToTail(atoi(pch));
			pch = strtok(NULL, " ,;");
		}
	}

	GetBall()->ReloadSettings();
}

void CSDKGameRules::SetMatchDisplayTimeSeconds(int seconds)
{
	m_bUseAdjustedStateEnterTime = true;
	float minute = seconds / 60.0f;
	match_state_t matchState;

	if (minute >= 120)
	{
		m_flAdjustedStateEnterTime = gpGlobals->curtime - ((seconds - 120 * 60) / (90.0f / mp_timelimit_match.GetFloat()));
		matchState = MATCH_PENALTIES;
	}
	else if (minute >= 105)
	{
		m_flAdjustedStateEnterTime = gpGlobals->curtime - ((seconds - 105 * 60) / (90.0f / mp_timelimit_match.GetFloat()));
		matchState = MATCH_EXTRATIME_SECOND_HALF;
	}
	else if (minute >= 90)
	{
		m_flAdjustedStateEnterTime = gpGlobals->curtime - ((seconds - 90 * 60) / (90.0f / mp_timelimit_match.GetFloat()));
		matchState = MATCH_EXTRATIME_FIRST_HALF;
	}
	else if (minute >= 45)
	{
		m_flAdjustedStateEnterTime = gpGlobals->curtime - ((seconds - 45 * 60) / (90.0f / mp_timelimit_match.GetFloat()));
		matchState = MATCH_SECOND_HALF;
	}
	else
	{
		m_flAdjustedStateEnterTime = gpGlobals->curtime - ((seconds - 0 * 60) / (90.0f / mp_timelimit_match.GetFloat()));
		matchState = MATCH_FIRST_HALF;
	}

	ResetMatch();
	m_flMatchStartTime = gpGlobals->curtime - (seconds / (90.0f / mp_timelimit_match.GetFloat()));
	GetBall()->State_Transition(BALL_STATIC, 0, true);
	SetLeftSideTeam(m_nFirstHalfLeftSideTeam);
	SetKickOffTeam(m_nFirstHalfKickOffTeam);
	State_Transition(matchState);
}

void CSDKGameRules::SetOffsideLinePositions(float ballPosY, float offsidePlayerPosY, float lastOppPlayerPosY)
{
	m_flOffsideLineBallPosY = ballPosY;
	m_flOffsideLineOffsidePlayerPosY = offsidePlayerPosY;
	m_flOffsideLineLastOppPlayerPosY = lastOppPlayerPosY;
	//m_bOffsideLinesEnabled = true;
}

void CSDKGameRules::SetOffsideLinesEnabled(bool enable)
{
	m_bOffsideLinesEnabled = enable;
}

void CSDKGameRules::CheckChatText(CBasePlayer *pPlayer, char *pText)
{
	BaseClass::CheckChatText( pPlayer, pText );
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

	if (pPl->GetTeamNumber() == TEAM_SPECTATOR)
	{
		Msg("Player is already spectating.\n");
		return;
	}
	
	pPl->SetDesiredTeam(TEAM_SPECTATOR, pPl->GetTeamNumber(), 0, true, true);
	UTIL_ClientPrintAll(HUD_PRINTTALK, "#game_player_benched", pPl->GetPlayerName());
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
		if (!CSDKPlayer::IsOnField(pPl)/* || pPl->IsBot()*/)
			continue;

		if (team == 0 || (pPl->GetTeamNumber() - TEAM_A + 1) == team)
		{
			pPl->SetDesiredTeam(TEAM_SPECTATOR, pPl->GetTeamNumber(), 0, true, true);
			UTIL_ClientPrintAll(HUD_PRINTTALK, "#game_player_benched", pPl->GetPlayerName());
		}
	}
}

static ConCommand benchall("benchall", CC_BenchAll);


#else

void SetupVec(Vector& v, int dim1, int dim2, int fixedDim, float dim1Val, float dim2Val, float fixedDimVal)
{
	v[dim1] = dim1Val;
	v[dim2] = dim2Val;
	v[fixedDim] = fixedDimVal;
}

void DrawBoxSide(int dim1, int dim2, int fixedDim, float minX, float minY, float maxX, float maxY, float fixedDimVal, bool bFlip, Vector &color)
{
	Vector v;

	CMatRenderContextPtr pRenderContext( materials );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();

	CMeshBuilder builder;
	builder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

	SetupVec(v, dim1, dim2, fixedDim, minX, maxY, fixedDimVal);
	builder.Position3fv(v.Base());
	builder.Color3fv(color.Base());
	builder.AdvanceVertex();

	SetupVec(v, dim1, dim2, fixedDim, bFlip ? maxX : minX, bFlip ? maxY : minY, fixedDimVal);
	builder.Position3fv(v.Base());
	builder.Color3fv(color.Base());
	builder.AdvanceVertex();

	SetupVec(v, dim1, dim2, fixedDim, bFlip ? minX : maxX, bFlip ? minY : maxY, fixedDimVal);
	builder.Position3fv(v.Base());
	builder.Color3fv(color.Base());
	builder.AdvanceVertex();

	SetupVec(v, dim1, dim2, fixedDim, maxX, minY, fixedDimVal);
	builder.Position3fv(v.Base());
	builder.Color3fv(color.Base());
	builder.AdvanceVertex();

	builder.End();
	pMesh->Draw();
}

void DrawOffsideLine(IMaterial *pMaterial, float posY, Vector &color)
{
	CMatRenderContextPtr pRenderContext( materials );
	// Draw it.
	pRenderContext->Bind( pMaterial );

	Vector mins = Vector(SDKGameRules()->m_vFieldMin.GetX() - 500, posY - 1, SDKGameRules()->m_vKickOff.GetZ());
	Vector maxs = Vector(SDKGameRules()->m_vFieldMax.GetX() + 500, posY + 1, SDKGameRules()->m_vKickOff.GetZ() + 2);

	DrawBoxSide(1, 2, 0, mins[1], mins[2], maxs[1], maxs[2], mins[0], false, color);
	DrawBoxSide(1, 2, 0, mins[1], mins[2], maxs[1], maxs[2], maxs[0], true, color);

	DrawBoxSide(0, 2, 1, mins[0], mins[2], maxs[0], maxs[2], mins[1], true, color);
	DrawBoxSide(0, 2, 1, mins[0], mins[2], maxs[0], maxs[2], maxs[1], false, color);

	DrawBoxSide(0, 1, 2, mins[0], mins[1], maxs[0], maxs[1], mins[2], false, color);
	DrawBoxSide(0, 1, 2, mins[0], mins[1], maxs[0], maxs[1], maxs[2], true, color);
}

void CSDKGameRules::DrawFieldTeamCrests()
{
	if (!IsIntermissionState())
		return;

	for (int i = 0; i < 2; i++)
	{
		if (!GameResources()->HasTeamCrest(i + TEAM_A))
			continue;

		int sign;
		char *material;

		if (i == 0)
		{
			sign = m_nLeftSideTeam == TEAM_A ? 1 : -1;
			material = "vgui/hometeamcrest";
		}
		else
		{
			sign = m_nLeftSideTeam == TEAM_A ? -1 : 1;
			material = "vgui/awayteamcrest";
		}

		Vector right = Vector(1, 0, 0);
		Vector forward = Vector(0, 1, 0);
		float size = 150;
		Vector origin = SDKGameRules()->m_vKickOff;
		origin.y += sign * (40 + size);
		origin.z += 1;

		CMatRenderContextPtr pRenderContext( materials );
		IMaterial *pPreviewMaterial = materials->FindMaterial( material, TEXTURE_GROUP_CLIENT_EFFECTS );
		pRenderContext->Bind( pPreviewMaterial );
		IMesh *pMesh = pRenderContext->GetDynamicMesh();
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,0,0 );
		meshBuilder.Position3fv( (origin + (right * sign * size) + (forward * sign * -size)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,1,0 );
		meshBuilder.Position3fv( (origin + (right * sign * -size) + (forward * sign * -size)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,1,1 );
		meshBuilder.Position3fv( (origin + (right * sign * -size) + (forward * sign * size)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,0,1 );
		meshBuilder.Position3fv( (origin + (right * sign * size) + (forward * sign * size)).Base() );
		meshBuilder.AdvanceVertex();
		meshBuilder.End();
		pMesh->Draw();
	}
}

void CSDKGameRules::DrawOffsideLines()
{
	if (m_bOffsideLinesEnabled)
	{
		DrawOffsideLine(m_pOffsideLineMaterial, m_flOffsideLineBallPosY, Vector(0, 0, 1));
		DrawOffsideLine(m_pOffsideLineMaterial, m_flOffsideLineLastOppPlayerPosY, Vector(0, 1, 0));
		DrawOffsideLine(m_pOffsideLineMaterial, m_flOffsideLineOffsidePlayerPosY, Vector(1, 0, 0));
		//DrawOffsideLine(m_pOffsideLineMaterial, m_flOffsideLinePlayerY);
	}
}

float CSDKGameRules::GetDaytime()
{
	return fmodf(mp_daytime_start.GetFloat() + ((gpGlobals->curtime - m_flMatchStartTime) / 60.0f / 60.0f) * mp_daytime_speed.GetFloat(), 24.0f);
}

void CSDKGameRules::DrawSkyboxOverlay()
{
	if (!mp_daytime_enabled.GetBool())
		return;

	float dayTime = GetDaytime();
	float alpha;

	if (dayTime > mp_daytime_sunrise.GetFloat() && dayTime < mp_daytime_sunset.GetFloat())
		alpha = 0; // day
	else if (dayTime >= mp_daytime_sunset.GetFloat() && dayTime <= mp_daytime_sunset.GetFloat() + mp_daytime_transition.GetFloat())
		alpha = (dayTime - mp_daytime_sunset.GetFloat()) / mp_daytime_transition.GetFloat(); // sunset
	else if (dayTime >= mp_daytime_sunrise.GetFloat() - mp_daytime_transition.GetFloat() && dayTime <= mp_daytime_sunrise.GetFloat())
		alpha = 1 - (dayTime - (mp_daytime_sunrise.GetFloat() - mp_daytime_transition.GetFloat())) / mp_daytime_transition.GetFloat(); // sunrise
	else
		alpha = 1; // night

	alpha = pow(alpha, 0.5f);
	alpha = clamp(alpha, 0, 0.997f);

	CMatRenderContextPtr pRenderContext( materials );
	IMaterial *pPreviewMaterial = materials->FindMaterial( "pitch/offside_line", TEXTURE_GROUP_CLIENT_EFFECTS );
	//IMaterial *pPreviewMaterial = materials->FindMaterial( "debug/debugspritewireframe", TEXTURE_GROUP_OTHER );
	pRenderContext->Bind( pPreviewMaterial );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 6 );

	Vector right;
	Vector forward;
	Vector pos;
	float forwardLength;
	float rightLength;
	const float length = 5000;
	const float width = 4000;
	const float height = 1000;
	const float heightOffset = height - 200;

	for (int i = 0; i < 6; i++)
	{
		switch (i)
		{
		case 0: // Top
			{
				forward = Vector(0, 1, 0);
				right = Vector(1, 0, 0);
				pos = SDKGameRules()->m_vKickOff + Vector(0, 0, height + heightOffset);
				forwardLength = length;
				rightLength = width;
			}
			break;
		case 1: // Front
			{
				forward = Vector(0, 0, 1);
				right = Vector(-1, 0, 0);
				pos = SDKGameRules()->m_vKickOff + Vector(0, length, heightOffset);
				forwardLength = height;
				rightLength = width;
			}
			break;
		case 2: // Right
			{
				forward = Vector(0, 0, 1);
				right = Vector(0, 1, 0);
				pos = SDKGameRules()->m_vKickOff + Vector(width, 0, heightOffset);
				forwardLength = height;
				rightLength = length;
			}
			break;
		case 3: // Left
			{
				forward = Vector(0, 0, 1);
				right = Vector(0, -1, 0);
				pos = SDKGameRules()->m_vKickOff + Vector(-width, 0, heightOffset);
				forwardLength = height;
				rightLength = length;
			}
			break;
		case 4: // Back
			{
				forward = Vector(0, 0, 1);
				right = Vector(1, 0, 0);
				pos = SDKGameRules()->m_vKickOff + Vector(0, -length, heightOffset);
				forwardLength = height;
				rightLength = width;
			}
			break;
		case 5: // Bottom
		default:
			{
				forward = Vector(0, 1, 0);
				right = Vector(-1, 0, 0);
				pos = SDKGameRules()->m_vKickOff + Vector(0, 0, -height + heightOffset);
				forwardLength = length;
				rightLength = width;
			}
			break;
		}

		meshBuilder.Color4f( 0.0, 0.0, 0.0, alpha );
		meshBuilder.TexCoord2f( 0,0,1 );
		meshBuilder.Position3fv( (pos + (right * rightLength) + (forward * forwardLength)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4f( 0.0, 0.0, 0.0, alpha );
		meshBuilder.TexCoord2f( 0,1,1 );
		meshBuilder.Position3fv( (pos + (right * -rightLength) + (forward * forwardLength)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4f( 0.0, 0.0, 0.0, alpha );
		meshBuilder.TexCoord2f( 0,1,0 );
		meshBuilder.Position3fv( (pos + (right * -rightLength) + (forward * -forwardLength)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4f( 0.0, 0.0, 0.0, alpha );
		meshBuilder.TexCoord2f( 0,0,0 );
		meshBuilder.Position3fv( (pos + (right * rightLength) + (forward * -forwardLength)).Base() );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	pMesh->Draw();
}

#endif
