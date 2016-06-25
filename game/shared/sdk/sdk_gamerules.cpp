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
#include "movevars_shared.h"

extern void Bot_RunAll( void );

#ifdef CLIENT_DLL

	#include "precache_register.h"
	#include "c_sdk_player.h"
	#include "c_team.h"
	#include "igameresources.h"
	#include "ios_camera.h"
	#include "c_match_ball.h"
	#include "c_ios_replaymanager.h"

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
	#include <time.h>
	#include "ios_requiredclientversion.h"
	#include "ios_fileupdater.h"
	#include "player_ball.h"
	#include "match_ball.h"
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

void CSDKGameRules::SetupFormations()
{
	for (int i = 0; i < 11; i++)
		m_Formations[i].PurgeAndDeleteElements();

	Formation *f;

	// 1
	f = new Formation("0");
	f->positions.AddToTail(new Position(1.5f, 3, POS_GK, 1));
	m_Formations[0].AddToTail(f);

	// 2
	f = new Formation("1");
	f->positions.AddToTail(new Position(1.5f, 1, POS_CM, 10));
	f->positions.AddToTail(new Position(1.5f, 3, POS_GK, 1));
	m_Formations[1].AddToTail(f);

	// 3
	f = new Formation("2");
	f->positions.AddToTail(new Position(0.5f, 1, POS_LM, 11));
	f->positions.AddToTail(new Position(2.5f, 1, POS_RM, 7));
	f->positions.AddToTail(new Position(1.5f, 3, POS_GK, 1));
	m_Formations[2].AddToTail(f);

	// 4
	f = new Formation("1-2");
	f->positions.AddToTail(new Position(0.5f, 1, POS_LM, 11));
	f->positions.AddToTail(new Position(2.5f, 1, POS_RM, 7));
	f->positions.AddToTail(new Position(1.5f, 2, POS_CB, 3));
	f->positions.AddToTail(new Position(1.5f, 3, POS_GK, 1));
	m_Formations[3].AddToTail(f);

	// 5
	f = new Formation("1-2-1");
	f->positions.AddToTail(new Position(1.5f, 0, POS_CF, 9));
	f->positions.AddToTail(new Position(0.5f, 1, POS_LM, 11));
	f->positions.AddToTail(new Position(2.5f, 1, POS_RM, 7));
	f->positions.AddToTail(new Position(1.5f, 2, POS_CB, 3));
	f->positions.AddToTail(new Position(1.5f, 3, POS_GK, 1));
	m_Formations[4].AddToTail(f);

	// 6
	f = new Formation("2-1-2");
	f->positions.AddToTail(new Position(0.5f, 0, POS_LF, 11));
	f->positions.AddToTail(new Position(2.5f, 0, POS_RF, 7));
	f->positions.AddToTail(new Position(1.5f, 1, POS_CM, 10));
	f->positions.AddToTail(new Position(0.5f, 2, POS_LB, 2));
	f->positions.AddToTail(new Position(2.5f, 2, POS_RB, 5));
	f->positions.AddToTail(new Position(1.5f, 3, POS_GK, 1));
	m_Formations[5].AddToTail(f);

	// 7
	f = new Formation("3-1-2");
	f->positions.AddToTail(new Position(0.5f, 0, POS_LF, 11));
	f->positions.AddToTail(new Position(2.5f, 0, POS_RF, 7));
	f->positions.AddToTail(new Position(1.5f, 1, POS_CM, 10));
	f->positions.AddToTail(new Position(0.5f, 2, POS_LB, 2));
	f->positions.AddToTail(new Position(1.5f, 2, POS_CB, 3));
	f->positions.AddToTail(new Position(2.5f, 2, POS_RB, 5));
	f->positions.AddToTail(new Position(1.5f, 3, POS_GK, 1));
	m_Formations[6].AddToTail(f);

	// 8
	f = new Formation("3-3-1");
	f->positions.AddToTail(new Position(1.5f, 0, POS_CF, 9));
	f->positions.AddToTail(new Position(0.5f, 1, POS_LW, 11));
	f->positions.AddToTail(new Position(1.5f, 1, POS_CM, 10));
	f->positions.AddToTail(new Position(2.5f, 1, POS_RW, 7));
	f->positions.AddToTail(new Position(0.5f, 2, POS_LB, 5));
	f->positions.AddToTail(new Position(1.5f, 2, POS_CB, 3));
	f->positions.AddToTail(new Position(2.5f, 2, POS_RB, 2));
	f->positions.AddToTail(new Position(1.5f, 3, POS_GK, 1));
	m_Formations[7].AddToTail(f);

	// 9
	f = new Formation("3-2-3");
	f->positions.AddToTail(new Position(0.5f, 0, POS_LW, 11));
	f->positions.AddToTail(new Position(1.5f, 0, POS_CF, 9));
	f->positions.AddToTail(new Position(2.5f, 0, POS_RW, 7));
	f->positions.AddToTail(new Position(1.0f, 1, POS_LCM, 10));
	f->positions.AddToTail(new Position(2.0f, 1, POS_RCM, 8));
	f->positions.AddToTail(new Position(0.5f, 2, POS_LB, 5));
	f->positions.AddToTail(new Position(1.5f, 2, POS_CB, 3));
	f->positions.AddToTail(new Position(2.5f, 2, POS_RB, 2));
	f->positions.AddToTail(new Position(1.5f, 3, POS_GK, 1));
	m_Formations[8].AddToTail(f);

	// 10
	f = new Formation("3-3-3");
	f->positions.AddToTail(new Position(0.5f, 0, POS_LW, 11));
	f->positions.AddToTail(new Position(1.5f, 0, POS_CF, 9));
	f->positions.AddToTail(new Position(2.5f, 0, POS_RW, 7));
	f->positions.AddToTail(new Position(0.5f, 1, POS_LCM, 8));
	f->positions.AddToTail(new Position(1.5f, 1, POS_CM, 6));
	f->positions.AddToTail(new Position(2.5f, 1, POS_RCM, 10));
	f->positions.AddToTail(new Position(0.5f, 2, POS_LB, 5));
	f->positions.AddToTail(new Position(1.5f, 2, POS_CB, 3));
	f->positions.AddToTail(new Position(2.5f, 2, POS_RB, 2));
	f->positions.AddToTail(new Position(1.5f, 3, POS_GK, 1));
	m_Formations[9].AddToTail(f);

	// 11
	f = new Formation("4-3-3");
	f->positions.AddToTail(new Position(0.5f, 0, POS_LW, 11));
	f->positions.AddToTail(new Position(1.5f, 0, POS_CF, 9));
	f->positions.AddToTail(new Position(2.5f, 0, POS_RW, 7));
	f->positions.AddToTail(new Position(0.5f, 1, POS_LCM, 10));
	f->positions.AddToTail(new Position(1.5f, 1, POS_CM, 6));
	f->positions.AddToTail(new Position(2.5f, 1, POS_RCM, 8));
	f->positions.AddToTail(new Position(0.0f, 2, POS_LB, 5));
	f->positions.AddToTail(new Position(1.0f, 2, POS_LCB, 4));
	f->positions.AddToTail(new Position(2.0f, 2, POS_RCB, 3));
	f->positions.AddToTail(new Position(3.0f, 2, POS_RB, 2));
	f->positions.AddToTail(new Position(1.5f, 3, POS_GK, 1));
	m_Formations[10].AddToTail(f);
}

CUtlVector<Formation *> &CSDKGameRules::GetFormations()
{
	int maxplayers;
#ifdef GAME_DLL
	maxplayers = UseOldMaxplayers() ? GetOldMaxplayers() : mp_maxplayers.GetInt();
#else
	maxplayers = mp_maxplayers.GetInt();
#endif
	return m_Formations[maxplayers - 1];
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

BEGIN_NETWORK_TABLE_NOBASE( CSDKGameRules, DT_SDKGameRules )
#if defined ( CLIENT_DLL )
	RecvPropTime( RECVINFO( m_flStateEnterTime ) ),
	RecvPropTime( RECVINFO( m_flMatchStartTime ) ),
	RecvPropInt( RECVINFO( m_eMatchPeriod) ),
	RecvPropIntWithMinusOneFlag( RECVINFO( m_nAnnouncedInjuryTime) ),
	RecvPropTime( RECVINFO( m_flClockStoppedStart) ),
	RecvPropTime( RECVINFO( m_flClockStoppedTime) ),

	RecvPropInt(RECVINFO(m_nShieldType)),
	RecvPropInt(RECVINFO(m_nShieldTeam)),
	RecvPropVector(RECVINFO(m_vShieldPos)),

	RecvPropVector(RECVINFO(m_vFieldMin)),
	RecvPropVector(RECVINFO(m_vFieldMax)),
	RecvPropVector(RECVINFO(m_vKickOff)),

	RecvPropInt(RECVINFO(m_nBallZone)),
	RecvPropInt(RECVINFO(m_nBottomTeam)),

	RecvPropInt(RECVINFO(m_nTimeoutTeam)),
	RecvPropInt(RECVINFO(m_eTimeoutState)),
	RecvPropTime(RECVINFO(m_flTimeoutEnd)),

	RecvPropFloat(RECVINFO(m_flOffsideLineBallPosY)),
	RecvPropFloat(RECVINFO(m_flOffsideLineOffsidePlayerPosY)),
	RecvPropFloat(RECVINFO(m_flOffsideLineLastOppPlayerPosY)),
	RecvPropBool(RECVINFO(m_bOffsideLinesEnabled)),
	RecvPropBool(RECVINFO(m_bSprayLinesEnabled)),
	RecvPropBool(RECVINFO(m_bIsCeremony)),
	RecvPropString(RECVINFO(m_szPitchTextureName)),

#else
	SendPropTime( SENDINFO( m_flStateEnterTime )),
	SendPropTime( SENDINFO( m_flMatchStartTime )),
	//SendPropFloat( SENDINFO( m_fStart) ),
	//SendPropInt( SENDINFO( m_iDuration) ),
	SendPropInt( SENDINFO( m_eMatchPeriod ), 4, SPROP_UNSIGNED),
	SendPropIntWithMinusOneFlag( SENDINFO( m_nAnnouncedInjuryTime ), 4),
	SendPropTime( SENDINFO( m_flClockStoppedStart )),
	SendPropTime( SENDINFO( m_flClockStoppedTime )),

	SendPropInt(SENDINFO(m_nShieldType), 3, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nShieldTeam), 3),
	SendPropVector(SENDINFO(m_vShieldPos), -1, SPROP_COORD),

	SendPropVector(SENDINFO(m_vFieldMin), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vFieldMax), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vKickOff), -1, SPROP_COORD),

	SendPropInt(SENDINFO(m_nBallZone), 8),
	SendPropInt(SENDINFO(m_nBottomTeam), 3),

	SendPropInt(SENDINFO(m_nTimeoutTeam), 3),
	SendPropInt(SENDINFO(m_eTimeoutState), 2, SPROP_UNSIGNED),
	SendPropTime(SENDINFO(m_flTimeoutEnd)),

	SendPropFloat(SENDINFO(m_flOffsideLineBallPosY), -1, SPROP_COORD),
	SendPropFloat(SENDINFO(m_flOffsideLineOffsidePlayerPosY), -1, SPROP_COORD),
	SendPropFloat(SENDINFO(m_flOffsideLineLastOppPlayerPosY), -1, SPROP_COORD),
	SendPropBool(SENDINFO(m_bOffsideLinesEnabled)),
	SendPropBool(SENDINFO(m_bSprayLinesEnabled)),
	SendPropBool(SENDINFO(m_bIsCeremony)),
	SendPropString(SENDINFO(m_szPitchTextureName)),

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
	
	Vector(-11, -11, 0 ),		//VEC_HULL_MIN
	Vector( 11,  11,  80 ),		//VEC_HULL_MAX

	Vector( 0, 0, 58 ),		
	Vector(-12, -12, 0 ),	
	Vector( 12,  12,  36 ),

	Vector( 0, 0, 58 ),		
	Vector(-12, -12, 0 ),	
	Vector( 12,  12,  36 ),	
													
	Vector(-12, -12, 0 ),		//VEC_DUCK_HULL_MIN
	Vector( 12,  12, 45 ),		//VEC_DUCK_HULL_MAX
	Vector( 0, 0, 34 ),			//VEC_DUCK_VIEW
													
	//Vector(-13, -13, -13 ),		//VEC_OBS_HULL_MIN
	//Vector( 13,  13,  13 ),		//VEC_OBS_HULL_MAX

	Vector(-12, -12, 0 ),		//VEC_OBS_HULL_MIN
	Vector( 12,  12,  58 ),		//VEC_OBS_HULL_MAX
													
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

CViewVectors* CSDKGameRules::GetViewVectorsToModify()
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

	SetupFormations();

	m_szPitchTextureName.GetForModify()[0] = '\0';

#ifdef GAME_DLL
	m_pCurStateInfo = NULL;
	m_nShieldType = SHIELD_NONE;
	m_vShieldPos = vec3_invalid;
	m_flStateTimeLeft = 0;
	m_flLastAwayCheckTime = gpGlobals->curtime;
	m_flNextPenalty = gpGlobals->curtime;
	m_nPenaltyTakingTeam = TEAM_HOME;
	m_flClockStoppedTime = 0;
	m_flClockStoppedStart = -1;
	m_flBallStateTransitionTime = 0;
	m_pPrecip = NULL;
	m_nFirstHalfBottomTeam = TEAM_HOME;
	m_nBottomTeam = TEAM_HOME;
	m_bIsCeremony = false;
	m_nTimeoutTeam = TEAM_NONE;
	m_eTimeoutState = TIMEOUT_STATE_NONE;
	m_flTimeoutEnd = 0;
	m_nFirstHalfKickOffTeam = TEAM_HOME;
	m_nKickOffTeam = TEAM_HOME;
	m_bOffsideLinesEnabled = false;
	m_bSprayLinesEnabled = false;
	m_flOffsideLineBallPosY = 0;
	m_flOffsideLineOffsidePlayerPosY = 0;
	m_flOffsideLineLastOppPlayerPosY = 0;
	m_flMatchStartTime = gpGlobals->curtime;
	m_bUseAdjustedStateEnterTime = false;
	m_flAdjustedStateEnterTime = -FLT_MAX;
	m_nOldMaxplayers = mp_maxplayers.GetInt();
	m_bUseOldMaxplayers = false;
	m_nRealMatchStartTime = 0;
	m_nRealMatchEndTime = 0;
	m_bHasWalledField = false;
	m_bIsTrainingMap = false;
	m_nAllowedFieldMaterials = FL_FIELD_MATERIAL_GRASS;

	m_flLastMasterServerPingTime = -FLT_MAX;
	m_bIsPingingMasterServer = false;

	m_pServerUpdateInfo = new IOSUpdateInfo();

#else
	PrecacheMaterial("pitch/offside_line");
	m_pOffsideLineMaterial = materials->FindMaterial( "pitch/offside_line", TEXTURE_GROUP_CLIENT_EFFECTS );

	PrecacheMaterial("pitch/spray_line");
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
	"sdk_TEAM_NONE",
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
	CPlayerData::ReallocateAllPlayerData();

	CTeamInfo::ParseTeamKits();
	CShoeInfo::ParseShoes();
	CKeeperGloveInfo::ParseKeeperGloves();
	CBallInfo::ParseBallSkins();
	CPitchInfo::ParsePitchTextures();

	SetPitchTextureName(CPitchInfo::m_PitchInfo[0]->m_szFolderName);

	InitTeams();

	InitFieldSpots();

	CreateMatchBall(m_vKickOff);

	GetMatchBall()->SetSkinName(CBallInfo::m_BallInfo[0]->m_szFolderName);

	m_pPrecip = (CPrecipitation *)CreateEntityByName("func_precipitation");
	m_pPrecip->SetType(PRECIPITATION_TYPE_NONE);
	m_pPrecip->Spawn();

	m_nFirstHalfBottomTeam = g_IOSRand.RandomInt(TEAM_HOME, TEAM_AWAY);
	m_nFirstHalfKickOffTeam = g_IOSRand.RandomInt(TEAM_HOME, TEAM_AWAY);

	m_bIsCeremony = false;

	State_Transition(MATCH_PERIOD_WARMUP);
}

void CSDKGameRules::InitFieldSpots()
{
	CInfoStadium *pInfoStadium = (CInfoStadium *)gEntList.FindEntityByClassname(NULL, "info_stadium");

	if (pInfoStadium)
	{
		m_bHasWalledField = pInfoStadium->m_bHasWalledField;
		m_bIsTrainingMap = pInfoStadium->m_bIsTrainingMap;

		m_nAllowedFieldMaterials = 0;

		if (pInfoStadium->m_bAllowGrassFieldMaterial)
			m_nAllowedFieldMaterials |= FL_FIELD_MATERIAL_GRASS;

		if (pInfoStadium->m_bAllowArtificialFieldMaterial)
			m_nAllowedFieldMaterials |= FL_FIELD_MATERIAL_ARTIFICIAL;

		if (pInfoStadium->m_bAllowStreetFieldMaterial)
			m_nAllowedFieldMaterials |= FL_FIELD_MATERIAL_STREET;

		if (pInfoStadium->m_bAllowSandFieldMaterial)
			m_nAllowedFieldMaterials |= FL_FIELD_MATERIAL_SAND;

		if (pInfoStadium->m_bAllowMudFieldMaterial)
			m_nAllowedFieldMaterials |= FL_FIELD_MATERIAL_MUD;

		if (m_nAllowedFieldMaterials == 0)
			m_nAllowedFieldMaterials = FL_FIELD_MATERIAL_GRASS;
	}
	else
	{
		m_bHasWalledField = false;
		m_bIsTrainingMap = false;
		m_nAllowedFieldMaterials = FL_FIELD_MATERIAL_GRASS;
	}

	Vector fieldMin, fieldMax;
	CBaseEntity *pField = gEntList.FindEntityByClassname(NULL, "trigger_field");
	if (!pField)
		Error("'trigger_field' entity is missing from map");
	pField->CollisionProp()->WorldSpaceAABB(&fieldMin, &fieldMax);
	fieldMin += Vector(1, 1, 1);
	fieldMax -= Vector(1, 1, 1);

	Vector goalMin, goalMax;
	CBaseEntity *pGoal = gEntList.FindEntityByClassname(NULL, "trigger_goal");
	if (!pGoal)
		Error("'trigger_goal' entity is missing from map");
	pGoal->CollisionProp()->WorldSpaceAABB(&goalMin, &goalMax);
	goalMin += Vector(1, 1, 1);
	goalMax -= Vector(1, 1, 1);

	m_vKickOff = fieldMin;

	trace_t tr;
	UTIL_TraceLine(m_vKickOff + Vector(0, 0, 100), m_vKickOff + Vector(0, 0, -500), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr);
	m_vKickOff.SetZ(tr.endpos.z);

	m_vFieldMin = m_vKickOff - Vector(fieldMax.x - fieldMin.x, fieldMax.y - fieldMin.y, 0);
	m_vFieldMax = m_vKickOff + Vector(fieldMax.x - fieldMin.x, fieldMax.y - fieldMin.y, 0);

	m_vGoalTriggerSize = goalMax - goalMin;

	GetGlobalTeam(TEAM_HOME)->InitFieldSpots(true);
	GetGlobalTeam(TEAM_AWAY)->InitFieldSpots(false);
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

ConVar sv_master_legacy_mode_hack_enabled("sv_master_legacy_mode_hack_enabled", "0", 0);
ConVar sv_master_legacy_mode_hack_interval("sv_master_legacy_mode_hack_interval", "5", 0);
ConVar sv_master_legacy_mode_hack_duration("sv_master_legacy_mode_hack_duration", "0.05", 0);

void CSDKGameRules::Think()
{
	CheckServerUpdateStatus();

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

		//	CPlayerData::SavePlayerData(pPl);
		//}

		ChangeLevel(); // intermission is over
		return;
	}

	if (sv_master_legacy_mode_hack_enabled.GetBool())
	{
		if (!m_bIsPingingMasterServer && gpGlobals->curtime >= m_flLastMasterServerPingTime + sv_master_legacy_mode_hack_interval.GetFloat() * 60)
		{
			engine->ServerCommand("sv_master_legacy_mode 0\n");
			engine->ServerCommand("heartbeat\n");
			m_bIsPingingMasterServer = true;
			m_flLastMasterServerPingTime = gpGlobals->curtime;
		}
		else if (m_bIsPingingMasterServer && gpGlobals->curtime >= m_flLastMasterServerPingTime + sv_master_legacy_mode_hack_duration.GetFloat() * 60)
		{
			engine->ServerCommand("sv_master_legacy_mode 1\n");
			m_bIsPingingMasterServer = false;
		}
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

void CSDKGameRules::ChooseTeamNames(bool clubTeams, bool nationalTeams, bool realTeams, bool fictitiousTeams)
{
	char homeTeam[MAX_KITNAME_LENGTH] = {};
	char awayTeam[MAX_KITNAME_LENGTH] = {};
	CTeamInfo::GetNonClashingTeamKits(homeTeam, awayTeam, clubTeams, nationalTeams, realTeams, fictitiousTeams);

	if (homeTeam[0] != '\0' && awayTeam[0] != '\0')
	{
		IOS_LogPrintf("Setting random teams: %s against %s\n", homeTeam, awayTeam);
		GetGlobalTeam(TEAM_HOME)->SetKitName(homeTeam);
		GetGlobalTeam(TEAM_AWAY)->SetKitName(awayTeam);
	}
	else
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
}

#endif

ConVar mp_ball_player_collision("mp_ball_player_collision", "0", FCVAR_NOTIFY | FCVAR_REPLICATED);

bool CSDKGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		swap(collisionGroup0,collisionGroup1);
	}

	if (collisionGroup0 == COLLISION_GROUP_NONSOLID_PLAYER || collisionGroup1 == COLLISION_GROUP_NONSOLID_PLAYER)
		return false;

	if (collisionGroup0 == COLLISION_GROUP_NONSOLID_BALL ||
		collisionGroup1 == COLLISION_GROUP_NONSOLID_BALL)
	{
		return false;
	}

	if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_SOLID_BALL )
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

	if (ToSDKPlayer(pPlayer)->GetSpecTeam() == TEAM_HOME)
		return "HOME";
	else if (ToSDKPlayer(pPlayer)->GetSpecTeam() == TEAM_AWAY)
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

	if (pPlayer->GetTeamNumber() == TEAM_SPECTATOR)
	{
		if (ToSDKPlayer(pPlayer)->GetSpecTeam() == TEAM_SPECTATOR)
		{
			static char spec[5];
			Q_strncpy(spec, "SPEC", sizeof(spec));
			return spec;
		}
		else
		{
			static char bench[6];
			Q_strncpy(bench, "BENCH", sizeof(bench));
			return bench;
		}
	}

	return g_szPosNames[ToSDKPlayer(pPlayer)->GetTeam()->GetFormation()->positions[ToSDKPlayer(pPlayer)->GetTeamPosIndex()]->type];
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

		if (pSDKSource->GetSpecTeam() == TEAM_SPECTATOR && pSDKTarget->GetTeamNumber() == TEAM_SPECTATOR)
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

		pPl->RemoveProps();

		pPl->SetConnected(PlayerDisconnecting);

		// Remove the player from his team
		if (pPl->GetTeam())
			pPl->SetDesiredTeam(TEAM_NONE, TEAM_SPECTATOR, 0, true, false, false);
	}

	BaseClass::ClientDisconnected( pClient );
}

void CSDKGameRules::RestartMatch(bool isCeremony, int kickOffTeam, int bottomTeam)
{
	IGameEvent *pEvent = gameeventmanager->CreateEvent("match_restart");
	if (pEvent)
		gameeventmanager->FireEvent(pEvent);

	m_bIsCeremony = isCeremony;

	switch (kickOffTeam)
	{
		case -1: break;
		case 0: m_nFirstHalfKickOffTeam = g_IOSRand.RandomInt(TEAM_HOME, TEAM_AWAY); break;
		case 1: m_nFirstHalfKickOffTeam = TEAM_HOME; break;
		case 2: m_nFirstHalfKickOffTeam = TEAM_AWAY; break;
	}

	switch (bottomTeam)
	{
		case -1: break;
		case 0: m_nFirstHalfBottomTeam = g_IOSRand.RandomInt(TEAM_HOME, TEAM_AWAY); break;
		case 1: m_nFirstHalfBottomTeam = TEAM_HOME; break;
		case 2: m_nFirstHalfBottomTeam = TEAM_AWAY; break;
	}

	SDKGameRules()->State_Transition(MATCH_PERIOD_WARMUP);
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

	GetGlobalTeam(TEAM_HOME)->SetGoals(atoi(args[1]));
}

ConCommand sv_matchgoalshome( "sv_matchgoalshome", CC_SV_MatchGoalsHome, "", 0 );

void CC_SV_MatchGoalsAway(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (args.ArgC() < 2)
		return;

	GetGlobalTeam(TEAM_AWAY)->SetGoals(atoi(args[1]));
}

ConCommand sv_matchgoalsaway( "sv_matchgoalsaway", CC_SV_MatchGoalsAway, "", 0 );

void CC_SV_StartTimeout(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (SDKGameRules()->IsIntermissionState() || SDKGameRules()->GetTimeoutState() != TIMEOUT_STATE_NONE)
		return;

	SDKGameRules()->SetTimeoutState(TIMEOUT_STATE_PENDING);
	SDKGameRules()->SetTimeoutTeam(TEAM_NONE);

	IGameEvent *pEvent = gameeventmanager->CreateEvent("timeout_pending");
	if (pEvent)
	{
		pEvent->SetInt("requesting_team", TEAM_NONE);
		gameeventmanager->FireEvent(pEvent);
	}
}

ConCommand sv_starttimeout( "sv_starttimeout", CC_SV_StartTimeout, "", 0 );

void CC_SV_EndTimeout(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (SDKGameRules()->IsIntermissionState() || SDKGameRules()->GetTimeoutState() == TIMEOUT_STATE_NONE)
		return;

	SDKGameRules()->EndTimeout();
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
	GetGlobalTeam(TEAM_HOME)->SetGoals(atoi(args[1]));
	GetGlobalTeam(TEAM_AWAY)->SetGoals(atoi(args[2]));
}

ConCommand sv_resumematch( "sv_resumematch", CC_SV_ResumeMatch, "", 0 );


void CC_SV_Restart(const CCommand &args)
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;

	if (args.ArgC() > 1)
		mp_timelimit_warmup.SetValue((float)atof(args[1]));

	bool isCeremony;

	if (args.ArgC() > 2)
		isCeremony = atoi(args[2]) != 0;
	else
		isCeremony = false;

	int kickOffTeam;

	if (args.ArgC() > 3)
		kickOffTeam = atoi(args[3]);
	else
		kickOffTeam = -1;

	int bottomTeam;

	if (args.ArgC() > 4)
		bottomTeam = atoi(args[4]);
	else
		bottomTeam = -1;

	SDKGameRules()->RestartMatch(isCeremony, kickOffTeam, bottomTeam);
}

ConCommand sv_restart( "sv_restart", CC_SV_Restart, "Usage: sv_restart <countdown in minutes> <is ceremony> <kick-off team> <bottom team>", 0 );


int CSDKGameRules::WakeUpAwayPlayers()
{
	int awayPlayerCount = 0;
	char wakeUpString[2048];
	
	if (State_Get() == MATCH_PERIOD_WARMUP)
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

ConVar sv_wakeupcall_interval("sv_wakeupcall_interval", "15", FCVAR_NOTIFY);

void CSDKGameRules::StartPenalties()
{
	//SetBottomTeam(g_IOSRand.RandomInt(TEAM_HOME, TEAM_AWAY));
	ResetMatch();
	State_Transition(MATCH_PERIOD_PENALTIES);
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


ConVar sv_replay_endpadding							("sv_replay_endpadding",						"1.0",		FCVAR_NOTIFY);


ConVar sv_replay_instant_enabled					("sv_replay_instant_enabled",					"1",		FCVAR_NOTIFY);

ConVar sv_replay_instant_first_enabled				("sv_replay_instant_first_enabled",				"1",		FCVAR_NOTIFY);
ConVar sv_replay_instant_first_camera				("sv_replay_instant_first_camera",				"0",		FCVAR_NOTIFY | FCVAR_REPLICATED, "0: sideline, 1: fixed sideline, 2: behind goal, 3: topdown, 4: fly follow, 5: goal line, 6: celebration");
ConVar sv_replay_instant_first_duration				("sv_replay_instant_first_duration",			"6",		FCVAR_NOTIFY);
ConVar sv_replay_instant_first_slowmo_duration		("sv_replay_instant_first_slowmo_duration",		"0",		FCVAR_NOTIFY);
ConVar sv_replay_instant_first_slowmo_coeff			("sv_replay_instant_first_slowmo_coeff",		"0",		FCVAR_NOTIFY);

ConVar sv_replay_instant_second_enabled				("sv_replay_instant_second_enabled",			"1",		FCVAR_NOTIFY);
ConVar sv_replay_instant_second_camera				("sv_replay_instant_second_camera",				"2",		FCVAR_NOTIFY | FCVAR_REPLICATED, "0: sideline, 1: fixed sideline, 2: behind goal, 3: topdown, 4: fly follow, 5: goal line, 6: celebration");
ConVar sv_replay_instant_second_duration			("sv_replay_instant_second_duration",			"6",		FCVAR_NOTIFY);
ConVar sv_replay_instant_second_slowmo_duration		("sv_replay_instant_second_slowmo_duration",	"6",		FCVAR_NOTIFY);
ConVar sv_replay_instant_second_slowmo_coeff		("sv_replay_instant_second_slowmo_coeff",		"0.5",		FCVAR_NOTIFY);

ConVar sv_replay_instant_third_enabled				("sv_replay_instant_third_enabled",				"1",		FCVAR_NOTIFY);
ConVar sv_replay_instant_third_camera				("sv_replay_instant_third_camera",				"5",		FCVAR_NOTIFY | FCVAR_REPLICATED, "0: sideline, 1: fixed sideline, 2: behind goal, 3: topdown, 4: fly follow, 5: goal line, 6: celebration");
ConVar sv_replay_instant_third_duration				("sv_replay_instant_third_duration",			"4",		FCVAR_NOTIFY);
ConVar sv_replay_instant_third_slowmo_duration		("sv_replay_instant_third_slowmo_duration",		"4",		FCVAR_NOTIFY);
ConVar sv_replay_instant_third_slowmo_coeff			("sv_replay_instant_third_slowmo_coeff",		"0.33",		FCVAR_NOTIFY);


ConVar sv_replay_highlight_enabled					("sv_replay_highlight_enabled",					"1",		FCVAR_NOTIFY);

ConVar sv_replay_highlight_first_enabled			("sv_replay_highlight_first_enabled",			"1",		FCVAR_NOTIFY);
ConVar sv_replay_highlight_first_camera				("sv_replay_highlight_first_camera",			"0",		FCVAR_NOTIFY | FCVAR_REPLICATED, "0: sideline, 1: fixed sideline, 2: behind goal, 3: topdown, 4: fly follow, 5: goal line, 6: celebration");
ConVar sv_replay_highlight_first_duration			("sv_replay_highlight_first_duration",			"4",		FCVAR_NOTIFY);
ConVar sv_replay_highlight_first_slowmo_duration	("sv_replay_highlight_first_slowmo_duration",	"0",		FCVAR_NOTIFY);
ConVar sv_replay_highlight_first_slowmo_coeff		("sv_replay_highlight_first_slowmo_coeff",		"0",		FCVAR_NOTIFY);

ConVar sv_replay_highlight_second_enabled			("sv_replay_highlight_second_enabled",			"0",		FCVAR_NOTIFY);
ConVar sv_replay_highlight_second_camera			("sv_replay_highlight_second_camera",			"2",		FCVAR_NOTIFY | FCVAR_REPLICATED, "0: sideline, 1: fixed sideline, 2: behind goal, 3: topdown, 4: fly follow, 5: goal line, 6: celebration");
ConVar sv_replay_highlight_second_duration			("sv_replay_highlight_second_duration",			"4",		FCVAR_NOTIFY);
ConVar sv_replay_highlight_second_slowmo_duration	("sv_replay_highlight_second_slowmo_duration",	"4",		FCVAR_NOTIFY);
ConVar sv_replay_highlight_second_slowmo_coeff		("sv_replay_highlight_second_slowmo_coeff",		"0",		FCVAR_NOTIFY);

ConVar sv_replay_highlight_third_enabled			("sv_replay_highlight_third_enabled",			"0",		FCVAR_NOTIFY);
ConVar sv_replay_highlight_third_camera				("sv_replay_highlight_third_camera",			"5",		FCVAR_NOTIFY | FCVAR_REPLICATED, "0: sideline, 1: fixed sideline, 2: behind goal, 3: topdown, 4: fly follow, 5: goal line, 6: celebration");
ConVar sv_replay_highlight_third_duration			("sv_replay_highlight_third_duration",			"4",		FCVAR_NOTIFY);
ConVar sv_replay_highlight_third_slowmo_duration	("sv_replay_highlight_third_slowmo_duration",	"4",		FCVAR_NOTIFY);
ConVar sv_replay_highlight_third_slowmo_coeff		("sv_replay_highlight_third_slowmo_coeff",		"0",		FCVAR_NOTIFY);



static void OnMaxPlayersChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	// Do nothing if the new and old values are identical
	if (((ConVar *)var)->GetInt() == atoi(pOldValue))
		return;

#ifdef GAME_DLL

	if (SDKGameRules())
	{
		SDKGameRules()->SetOldMaxplayers(atoi(pOldValue));
		SDKGameRules()->SetUseOldMaxplayers(true);
	}

	for (int team = TEAM_HOME; team <= TEAM_AWAY; team++)
	{
		if (GetGlobalTeam(team))
		{
			GetGlobalTeam(team)->SetFormationIndex(0, true);
		}
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->SetDesiredTeam(TEAM_SPECTATOR, pPl->GetTeamNumber(), 0, true, false, true);
	}

	if (SDKGameRules())
	{
		SDKGameRules()->SetUseOldMaxplayers(false);
	}

	for (int team = TEAM_HOME; team <= TEAM_AWAY; team++)
	{
		if (GetGlobalTeam(team))
		{
			GetGlobalTeam(team)->UpdatePosIndices(true);
		}
	}

#endif
}

ConVar mp_maxplayers("mp_maxplayers", "11", FCVAR_NOTIFY|FCVAR_REPLICATED, "Maximum number of players per team <1-11>", true, 1, true, 11, OnMaxPlayersChange);


void OnCaptaincyHomeChange(IConVar *var, const char *pOldValue, float flOldValue)
{
#ifdef GAME_DLL
	if (!SDKGameRules())
		return;

	bool captaincy = ((ConVar*)var)->GetBool();

	if (!captaincy)
		GetGlobalTeam(TEAM_HOME)->SetCaptainPosIndex(-1);
#endif
}

ConVar mp_captaincy_home("mp_captaincy_home", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "", &OnCaptaincyHomeChange);

void OnCaptaincyAwayChange(IConVar *var, const char *pOldValue, float flOldValue)
{
#ifdef GAME_DLL
	if (!SDKGameRules())
		return;

	bool captaincy = ((ConVar*)var)->GetBool();

	if (!captaincy)
		GetGlobalTeam(TEAM_AWAY)->SetCaptainPosIndex(-1);
#endif
}

ConVar mp_captaincy_away("mp_captaincy_away", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "", &OnCaptaincyAwayChange);


ConVar sv_autostartmatch("sv_autostartmatch", "1", FCVAR_NOTIFY|FCVAR_REPLICATED, "");
ConVar sv_awaytime_warmup("sv_awaytime_warmup", "15", FCVAR_NOTIFY);
ConVar sv_awaytime_warmup_autospec("sv_awaytime_warmup_autospec", "3600", FCVAR_NOTIFY);

ConVar sv_singlekeeper("sv_singlekeeper", "0", FCVAR_NOTIFY | FCVAR_REPLICATED);

#ifdef GAME_DLL

ConVar sv_playerrotation_enabled("sv_playerrotation_enabled", "0", FCVAR_NOTIFY);
ConVar sv_playerrotation_minutes("sv_playerrotation_minutes", "30,60", FCVAR_NOTIFY);

ConVar sv_singlekeeper_switchvalue("sv_singlekeeper_switchvalue", "0", FCVAR_NOTIFY);

void CSDKGameRules::State_Transition( match_period_t newState )
{
	State_Leave(newState);
	State_Enter( newState );
}

void CSDKGameRules::State_Enter( match_period_t newState )
{
	m_eMatchPeriod = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	if (m_bUseAdjustedStateEnterTime)
	{
		m_flStateEnterTime = m_flAdjustedStateEnterTime;
		m_bUseAdjustedStateEnterTime = false;
	}
	else
		m_flStateEnterTime = gpGlobals->curtime;

	m_flClockStoppedTime = 0.0f;
	m_flClockStoppedStart = -1;
	m_nAnnouncedInjuryTime = -1;
	m_flBallStateTransitionTime = 0.0f;

	if ( mp_showstatetransitions.GetInt() > 0 )
	{
		if ( m_pCurStateInfo )
			IOS_LogPrintf( "Gamerules: entering state '%s'\n", m_pCurStateInfo->m_pStateName );
		else
			IOS_LogPrintf( "Gamerules: entering state #%d\n", newState );
	}

	IGameEvent *pEvent = gameeventmanager->CreateEvent("match_period");
	if (pEvent)
	{
		pEvent->SetInt("period", State_Get());
		gameeventmanager->FireEvent(pEvent);
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->m_Shared.SetStamina(100);

		if (!IsIntermissionState())
			pPl->GetData()->StartNewMatchPeriod();
	}

	if (!IsIntermissionState())
	{
		CPlayerBall::RemoveAllPlayerBalls();
		CSDKPlayer::RemoveAllPlayerProps();

		for (int i = 0; i < 2; i++)
		{
			CTeamPeriodData *pTeamData = new CTeamPeriodData(g_szMatchPeriodNames[m_pCurStateInfo->m_eMatchPeriod]);
			pTeamData->ResetData();
			GetGlobalTeam(TEAM_HOME + i)->m_PeriodData.AddToTail(pTeamData); 
		}
	}

	if (State_Get() != MATCH_PERIOD_WARMUP)
	{
		m_bIsCeremony = false;
	}

	UTIL_ClientPrintAll(HUD_PRINTCENTER, g_szMatchPeriodNames[m_pCurStateInfo->m_eMatchPeriod]);

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		(this->*m_pCurStateInfo->pfnEnterState)();
	}
}

void CSDKGameRules::State_Leave(match_period_t newState)
{
	if (IsIntermissionState())
	{
		ReplayManager()->StopHighlights();
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			if (GetGlobalTeam(TEAM_HOME + i)->m_PeriodData.Count() == 0)
				continue;

			CTeamPeriodData *pTeamData = GetGlobalTeam(TEAM_HOME + i)->m_PeriodData.Tail();
			pTeamData->m_nAnnouncedInjuryTimeSeconds = m_nAnnouncedInjuryTime == -1 ? -1 : m_nAnnouncedInjuryTime * 60;
			pTeamData->m_nActualInjuryTimeSeconds = GetMatchDisplayTimeSeconds() - GetMatchDisplayTimeSeconds(false);
		}
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (!IsIntermissionState())
		{
			pPl->GetData()->EndCurrentMatchPeriod();
		}
	}

	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)(newState);
	}
}

void CSDKGameRules::CalcPeriodTimeLeft()
{
	if (m_pCurStateInfo->m_eMatchPeriod == MATCH_PERIOD_WARMUP && mp_timelimit_warmup.GetFloat() < 0)
		m_flStateTimeLeft = 1.0f;
	else
		m_flStateTimeLeft = (m_flStateEnterTime + m_pCurStateInfo->m_MinDurationConVar->GetFloat() * 60 / m_pCurStateInfo->m_flMinDurationDivisor) - gpGlobals->curtime;

	if (!IsIntermissionState())
	{
		m_flStateTimeLeft += m_flClockStoppedTime;

		if (m_nAnnouncedInjuryTime > -1)
		{
			int additionalTime = m_nAnnouncedInjuryTime + (abs(m_nBallZone) < 50 ? 0 : 30);
			m_flStateTimeLeft += additionalTime * 60 / (90.0f / mp_timelimit_match.GetFloat());
		}
	}
}

void CSDKGameRules::CheckSingleKeeperSideSwitching()
{
	// If there's only one keeper for both teams, switch him to the other team when the ball comes near the goal
	if (sv_singlekeeper.GetBool() && abs(m_nBallZone) > sv_singlekeeper_switchvalue.GetFloat())
	{
		CSDKPlayer *pKeeper = NULL;

		// If ball is on left side and no keeper on left side team
		if (m_nBallZone < 0 && !GetGlobalTeam(TEAM_HOME)->GetPlayerByPosType(POS_GK))
		{
			pKeeper = GetGlobalTeam(TEAM_AWAY)->GetPlayerByPosType(POS_GK);
		}
		// If ball is on right side and no keeper on right side team
		else if (m_nBallZone >= 0 && !GetGlobalTeam(TEAM_AWAY)->GetPlayerByPosType(POS_GK))
		{
			pKeeper = GetGlobalTeam(TEAM_HOME)->GetPlayerByPosType(POS_GK);
		}

		if (pKeeper)
		{
			pKeeper->SetDesiredTeam(pKeeper->GetOppTeamNumber(), pKeeper->GetOppTeamNumber(), pKeeper->GetTeamPosIndex(), true, false, true);
		}
	}
}

void CSDKGameRules::CheckPlayerRotation()
{
	// Check for player auto-rotation
	if (sv_playerrotation_enabled.GetBool() && m_PlayerRotationMinutes.Count() > 0 && GetMatchDisplayTimeSeconds(false) / 60 >= m_PlayerRotationMinutes.Head()
		&& (m_PlayerRotationMinutes.Head() != 45 && m_PlayerRotationMinutes.Head() != 90 && m_PlayerRotationMinutes.Head() != 105
			|| m_PlayerRotationMinutes.Head() == 45 && SDKGameRules()->State_Get() == MATCH_PERIOD_SECOND_HALF
			|| m_PlayerRotationMinutes.Head() == 90 && SDKGameRules()->State_Get() == MATCH_PERIOD_EXTRATIME_FIRST_HALF
			|| m_PlayerRotationMinutes.Head() == 105 && SDKGameRules()->State_Get() == MATCH_PERIOD_EXTRATIME_SECOND_HALF))
	{
		for (int team = TEAM_HOME; team <= TEAM_AWAY; team++)
		{
			CTeam *pTeam = GetGlobalTeam(team);

			for (int i = mp_maxplayers.GetInt() - 1; i >= 1; i--)
			{
				Vector pl1Pos, pl1Vel, pl2Pos, pl2Vel;
				QAngle pl1ModelAng, pl1EyeAng, pl2ModelAng, pl2EyeAng;

				CSDKPlayer *pPl1 = pTeam->GetPlayerByPosIndex(i);

				if (pPl1)
				{
					pl1Pos = pPl1->GetLocalOrigin();
					pl1Vel = pPl1->GetLocalVelocity();
					pl1ModelAng = pPl1->GetLocalAngles();
					pl1EyeAng = pPl1->EyeAngles();
				}

				CSDKPlayer *pPl2 = pTeam->GetPlayerByPosIndex(i - 1);

				if (pPl2)
				{
					pl2Pos = pPl2->GetLocalOrigin();
					pl2Vel = pPl2->GetLocalVelocity();
					pl2ModelAng = pPl2->GetLocalAngles();
					pl2EyeAng = pPl2->EyeAngles();
				}

				if (pPl1 && !pPl1->IsBot() && (!pPl2 || !pPl2->IsBot()))
				{
					pPl1->SetDesiredTeam(team, team, i - 1, true, false, true);

					if (pPl2)
					{
						pPl1->SetLocalOrigin(pl2Pos);
						pPl1->SetLocalVelocity(pl2Vel);
						pPl1->SetLocalAngles(pl2ModelAng);
						pPl1->SnapEyeAngles(pl2EyeAng);
					}
				}

				if (pPl2 && !pPl2->IsBot() && (!pPl1 || !pPl1->IsBot()))
				{
					pPl2->SetDesiredTeam(team, team, i, true, false, true);

					if (pPl1)
					{
						pPl2->SetLocalOrigin(pl1Pos);
						pPl2->SetLocalVelocity(pl1Vel);
						pPl2->SetLocalAngles(pl1ModelAng);
						pPl2->SnapEyeAngles(pl1EyeAng);
					}
				}
			}
		}

		m_PlayerRotationMinutes.Remove(0);
	}
}

void CSDKGameRules::State_Think()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnThink )
	{
		if (GetMatchBall())
			m_nBallZone = GetMatchBall()->GetFieldZone();

		CalcPeriodTimeLeft();

		if (!IsIntermissionState())
		{
			CheckSingleKeeperSideSwitching();
			CheckPlayerRotation();
		}

		(this->*m_pCurStateInfo->pfnThink)();
	}
}

CSDKGameRulesStateInfo* CSDKGameRules::State_LookupInfo( match_period_t state )
{
	static CSDKGameRulesStateInfo gameRulesStateInfos[] =
	{
		{ MATCH_PERIOD_WARMUP,						"MATCH_PERIOD_WARMUP",						&CSDKGameRules::State_WARMUP_Enter,					&CSDKGameRules::State_WARMUP_Think,					&CSDKGameRules::State_WARMUP_Leave,					&mp_timelimit_warmup, 1	},
		{ MATCH_PERIOD_FIRST_HALF,					"MATCH_PERIOD_FIRST_HALF",					&CSDKGameRules::State_FIRST_HALF_Enter,				&CSDKGameRules::State_FIRST_HALF_Think,				&CSDKGameRules::State_FIRST_HALF_Leave,				&mp_timelimit_match, 2 },
		{ MATCH_PERIOD_HALFTIME,					"MATCH_PERIOD_HALFTIME",					&CSDKGameRules::State_HALFTIME_Enter,				&CSDKGameRules::State_HALFTIME_Think,				&CSDKGameRules::State_HALFTIME_Leave,				&mp_timelimit_halftime, 1 },
		{ MATCH_PERIOD_SECOND_HALF,					"MATCH_PERIOD_SECOND_HALF",					&CSDKGameRules::State_SECOND_HALF_Enter,			&CSDKGameRules::State_SECOND_HALF_Think,			&CSDKGameRules::State_SECOND_HALF_Leave,			&mp_timelimit_match, 2 },
		{ MATCH_PERIOD_EXTRATIME_INTERMISSION,		"MATCH_PERIOD_EXTRATIME_INTERMISSION",		&CSDKGameRules::State_EXTRATIME_INTERMISSION_Enter, &CSDKGameRules::State_EXTRATIME_INTERMISSION_Think,	&CSDKGameRules::State_EXTRATIME_INTERMISSION_Leave,	&mp_timelimit_extratime_intermission, 1	},
		{ MATCH_PERIOD_EXTRATIME_FIRST_HALF,		"MATCH_PERIOD_EXTRATIME_FIRST_HALF",		&CSDKGameRules::State_EXTRATIME_FIRST_HALF_Enter,	&CSDKGameRules::State_EXTRATIME_FIRST_HALF_Think,	&CSDKGameRules::State_EXTRATIME_FIRST_HALF_Leave,	&mp_timelimit_match, 6 },
		{ MATCH_PERIOD_EXTRATIME_HALFTIME,			"MATCH_PERIOD_EXTRATIME_HALFTIME",			&CSDKGameRules::State_EXTRATIME_HALFTIME_Enter,		&CSDKGameRules::State_EXTRATIME_HALFTIME_Think,		&CSDKGameRules::State_EXTRATIME_HALFTIME_Leave,		&mp_timelimit_extratime_halftime, 1 },
		{ MATCH_PERIOD_EXTRATIME_SECOND_HALF,		"MATCH_PERIOD_EXTRATIME_SECOND_HALF",		&CSDKGameRules::State_EXTRATIME_SECOND_HALF_Enter,	&CSDKGameRules::State_EXTRATIME_SECOND_HALF_Think,	&CSDKGameRules::State_EXTRATIME_SECOND_HALF_Leave,	&mp_timelimit_match, 6 },
		{ MATCH_PERIOD_PENALTIES_INTERMISSION,		"MATCH_PERIOD_PENALTIES_INTERMISSION",		&CSDKGameRules::State_PENALTIES_INTERMISSION_Enter, &CSDKGameRules::State_PENALTIES_INTERMISSION_Think,	&CSDKGameRules::State_PENALTIES_INTERMISSION_Leave,	&mp_timelimit_penalties_intermission, 1 },
		{ MATCH_PERIOD_PENALTIES,					"MATCH_PERIOD_PENALTIES",					&CSDKGameRules::State_PENALTIES_Enter,				&CSDKGameRules::State_PENALTIES_Think,				&CSDKGameRules::State_PENALTIES_Leave,				&mp_timelimit_match, 3 },
		{ MATCH_PERIOD_COOLDOWN,					"MATCH_PERIOD_COOLDOWN",					&CSDKGameRules::State_COOLDOWN_Enter,				&CSDKGameRules::State_COOLDOWN_Think,				&CSDKGameRules::State_COOLDOWN_Leave,				&mp_timelimit_cooldown, 1 },
	};

	for ( int i=0; i < ARRAYSIZE( gameRulesStateInfos ); i++ )
	{
		if ( gameRulesStateInfos[i].m_eMatchPeriod == state )
			return &gameRulesStateInfos[i];
	}

	return NULL;
}

void CSDKGameRules::State_WARMUP_Enter()
{
	m_flMatchStartTime = gpGlobals->curtime;
	ResetMatch();

	SetBottomTeam(m_nFirstHalfBottomTeam);
	SetKickOffTeam(m_nFirstHalfKickOffTeam);

	ApplyIntermissionSettings(false, true);
}

void CSDKGameRules::State_WARMUP_Think()
{
	if (m_flStateTimeLeft <= 0 || CheckAutoStart())
		State_Transition(MATCH_PERIOD_FIRST_HALF);
	else
	{
		if (m_bIsCeremony && m_nShieldType == SHIELD_CEREMONY && CSDKPlayer::CheckPlayersAtShieldPos(true))
			DisableShield();
	}
}

void CSDKGameRules::State_WARMUP_Leave(match_period_t newState)
{
	GetMatchBall()->EmitSound("Ball.Whistle");
	//GetMatchBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_FIRST_HALF_Enter()
{
	SetBottomTeam(m_nFirstHalfBottomTeam);
	SetKickOffTeam(m_nFirstHalfKickOffTeam);

	m_nRealMatchStartTime = time(NULL);

	ReloadSettings();
	GetMatchBall()->State_Transition(BALL_STATE_KICKOFF, 0, 0, true);

	IGameEvent *pEvent = gameeventmanager->CreateEvent("wakeupcall");
	if (pEvent)
		gameeventmanager->FireEvent(pEvent);

	UTIL_ClientPrintAll(HUD_PRINTTALK, "#game_match_start");
	//UTIL_ClientPrintAll(HUD_PRINTCENTER, "LIVE");

	//GetMatchBall()->EmitSound("Crowd.YNWA");
}

void CSDKGameRules::State_FIRST_HALF_Think()
{
	if ((45 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == -1)
	{
		CalcAnnouncedInjuryTime();
	}
	else if (m_flStateTimeLeft <= 0 && GetMatchBall()->State_Get() == BALL_STATE_NORMAL && !GetMatchBall()->HasQueuedState())
	{
		State_Transition(MATCH_PERIOD_HALFTIME);
	}
}

void CSDKGameRules::State_FIRST_HALF_Leave(match_period_t newState)
{
	GetMatchBall()->EmitSound("Ball.Whistle");
	//GetMatchBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_HALFTIME_Enter()
{
	ApplyIntermissionSettings(true, false);
}

void CSDKGameRules::State_HALFTIME_Think()
{
	if (m_flStateTimeLeft <= 0)
		State_Transition(MATCH_PERIOD_SECOND_HALF);
}

void CSDKGameRules::State_HALFTIME_Leave(match_period_t newState)
{
}

void CSDKGameRules::State_SECOND_HALF_Enter()
{
	SetBottomTeam(GetGlobalTeam(m_nFirstHalfBottomTeam)->GetOppTeamNumber());
	SetKickOffTeam(GetGlobalTeam(m_nFirstHalfKickOffTeam)->GetOppTeamNumber());

	GetMatchBall()->State_Transition(BALL_STATE_KICKOFF, 0, 0, true);
	//GetMatchBall()->EmitSound("Crowd.YNWA");
}

void CSDKGameRules::State_SECOND_HALF_Think()
{
	if ((90 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == -1)
	{
		CalcAnnouncedInjuryTime();
	}
	else if (m_flStateTimeLeft <= 0 && GetMatchBall()->State_Get() == BALL_STATE_NORMAL && !GetMatchBall()->HasQueuedState())
	{
		if (mp_extratime.GetBool() && GetGlobalTeam(TEAM_HOME)->GetGoals() == GetGlobalTeam(TEAM_AWAY)->GetGoals())
			State_Transition(MATCH_PERIOD_EXTRATIME_INTERMISSION);
		else if (mp_penalties.GetBool() && GetGlobalTeam(TEAM_HOME)->GetGoals() == GetGlobalTeam(TEAM_AWAY)->GetGoals())
			State_Transition(MATCH_PERIOD_PENALTIES_INTERMISSION);
		else
			State_Transition(MATCH_PERIOD_COOLDOWN);
	}
}

void CSDKGameRules::State_SECOND_HALF_Leave(match_period_t newState)
{
	GetMatchBall()->EmitSound("Ball.Whistle");
	//GetMatchBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_EXTRATIME_INTERMISSION_Enter()
{
	ApplyIntermissionSettings(true, false);
}

void CSDKGameRules::State_EXTRATIME_INTERMISSION_Think()
{
	if (m_flStateTimeLeft <= 0)
		State_Transition(MATCH_PERIOD_EXTRATIME_FIRST_HALF);
}

void CSDKGameRules::State_EXTRATIME_INTERMISSION_Leave(match_period_t newState)
{
}

void CSDKGameRules::State_EXTRATIME_FIRST_HALF_Enter()
{
	SetBottomTeam(m_nFirstHalfBottomTeam);
	SetKickOffTeam(m_nFirstHalfKickOffTeam);

	GetMatchBall()->State_Transition(BALL_STATE_KICKOFF, 0, 0, true);
}

void CSDKGameRules::State_EXTRATIME_FIRST_HALF_Think()
{
	if ((105 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == -1)
	{
		CalcAnnouncedInjuryTime();
	}
	else if (m_flStateTimeLeft <= 0 && GetMatchBall()->State_Get() == BALL_STATE_NORMAL && !GetMatchBall()->HasQueuedState())
	{
		State_Transition(MATCH_PERIOD_EXTRATIME_HALFTIME);
	}
}

void CSDKGameRules::State_EXTRATIME_FIRST_HALF_Leave(match_period_t newState)
{
	GetMatchBall()->EmitSound("Ball.Whistle");
	//GetMatchBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_EXTRATIME_HALFTIME_Enter()
{
	ApplyIntermissionSettings(true, false);
}

void CSDKGameRules::State_EXTRATIME_HALFTIME_Think()
{
	if (m_flStateTimeLeft <= 0)
		State_Transition(MATCH_PERIOD_EXTRATIME_SECOND_HALF);
}

void CSDKGameRules::State_EXTRATIME_HALFTIME_Leave(match_period_t newState)
{
}

void CSDKGameRules::State_EXTRATIME_SECOND_HALF_Enter()
{
	SetBottomTeam(GetGlobalTeam(m_nFirstHalfBottomTeam)->GetOppTeamNumber());
	SetKickOffTeam(GetGlobalTeam(m_nFirstHalfKickOffTeam)->GetOppTeamNumber());

	GetMatchBall()->State_Transition(BALL_STATE_KICKOFF, 0, 0, true);
}

void CSDKGameRules::State_EXTRATIME_SECOND_HALF_Think()
{
	if ((120 * 60 - GetMatchDisplayTimeSeconds()) <= 60 && m_nAnnouncedInjuryTime == -1)
	{
		CalcAnnouncedInjuryTime();
	}
	else if (m_flStateTimeLeft <= 0 && GetMatchBall()->State_Get() == BALL_STATE_NORMAL && !GetMatchBall()->HasQueuedState())
	{
		if (mp_penalties.GetBool() && GetGlobalTeam(TEAM_HOME)->GetGoals() == GetGlobalTeam(TEAM_AWAY)->GetGoals())
			State_Transition(MATCH_PERIOD_PENALTIES_INTERMISSION);
		else
			State_Transition(MATCH_PERIOD_COOLDOWN);
	}
}

void CSDKGameRules::State_EXTRATIME_SECOND_HALF_Leave(match_period_t newState)
{
	GetMatchBall()->EmitSound("Ball.Whistle");
	//GetMatchBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_PENALTIES_INTERMISSION_Enter()
{
	ApplyIntermissionSettings(true, false);
}

void CSDKGameRules::State_PENALTIES_INTERMISSION_Think()
{
	if (m_flStateTimeLeft <= 0)
		State_Transition(MATCH_PERIOD_PENALTIES);
}

void CSDKGameRules::State_PENALTIES_INTERMISSION_Leave(match_period_t newState)
{
}

void CSDKGameRules::State_PENALTIES_Enter()
{
	for (int i = 0; i < 2; i++)
	{
		GetGlobalTeam(TEAM_HOME + i)->m_nPenaltyGoals = 0;
		GetGlobalTeam(TEAM_HOME + i)->m_nPenaltyGoalBits = 0;
		GetGlobalTeam(TEAM_HOME + i)->m_nPenaltyRound = 0;
	}

	m_flNextPenalty = -1;
	m_nPenaltyTakingStartTeam = g_IOSRand.RandomInt(TEAM_HOME, TEAM_AWAY);
	m_nPenaltyTakingTeam = m_nPenaltyTakingStartTeam;
	SetBottomTeam(g_IOSRand.RandomInt(TEAM_HOME, TEAM_AWAY));
	GetMatchBall()->SetPenaltyState(PENALTY_NONE);
}

void CSDKGameRules::State_PENALTIES_Think()
{
	if (m_flStateTimeLeft <= 0)
	{
		State_Transition(MATCH_PERIOD_COOLDOWN);
		return;
	}

	if (GetMatchBall()->GetPenaltyState() == PENALTY_KICKED
		|| GetMatchBall()->GetPenaltyState() == PENALTY_SCORED
		|| GetMatchBall()->GetPenaltyState() == PENALTY_SAVED
		|| GetMatchBall()->GetPenaltyState() == PENALTY_MISSED
		|| GetMatchBall()->GetPenaltyState() == PENALTY_ABORTED_NO_KEEPER
		|| GetMatchBall()->GetPenaltyState() == PENALTY_ABORTED_ILLEGAL_MOVE)
	{
		if (m_flNextPenalty == -1)
		{
			m_flNextPenalty = gpGlobals->curtime + 5;
		}
		else if (m_flNextPenalty <= gpGlobals->curtime)
		{
			if (GetMatchBall()->GetPenaltyState() == PENALTY_KICKED
				|| GetMatchBall()->GetPenaltyState() == PENALTY_SCORED
				|| GetMatchBall()->GetPenaltyState() == PENALTY_SAVED
				|| GetMatchBall()->GetPenaltyState() == PENALTY_MISSED
				|| GetMatchBall()->GetPenaltyState() == PENALTY_ABORTED_ILLEGAL_MOVE)
			{
				GetMatchBall()->State_Transition(BALL_STATE_NORMAL, 0, 0, true);

				if (GetMatchBall()->GetPenaltyState() == PENALTY_SCORED)
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
						State_Transition(MATCH_PERIOD_COOLDOWN);
						return;
					}
				}
				else
				{
					if (m_nPenaltyTakingTeam != m_nPenaltyTakingStartTeam && GetGlobalTeam(TEAM_HOME)->GetGoals() != GetGlobalTeam(TEAM_AWAY)->GetGoals())
					{
						State_Transition(MATCH_PERIOD_COOLDOWN);
						return;
					}
				}

				m_nPenaltyTakingTeam = GetGlobalTeam(m_nPenaltyTakingTeam)->GetOppTeamNumber();
				SetBottomTeam(GetGlobalTeam(GetBottomTeam())->GetOppTeamNumber());
			}

			GetMatchBall()->SetPenaltyState(PENALTY_NONE);
		}
	}
	else if (GetMatchBall()->GetPenaltyState() == PENALTY_NONE || GetMatchBall()->GetPenaltyState() == PENALTY_ABORTED_NO_TAKER)
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
				GetMatchBall()->SetPenaltyTaker(pPenTaker);
				GetMatchBall()->SetPenaltyState(PENALTY_ASSIGNED);
				GetMatchBall()->State_Transition(BALL_STATE_PENALTY, 0, 0, true);
				m_flNextPenalty = -1;
				break;
			}
			else
			{
				if (attemptCount == 1)
				{
					// Can't find a player who hasn't already taken a penalty on the first attempt, so reset all and start over
					for (int i = 1; i <= gpGlobals->maxClients; i++)
					{
						CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

						if (!CSDKPlayer::IsOnField(pPl) || pPl->GetTeamNumber() != m_nPenaltyTakingTeam)
							continue;

						pPl->m_ePenaltyState = PENALTY_NONE;
					}
				}
				else
				{
					// Can't find any suitable player even after the second attempt, so bail out
					State_Transition(MATCH_PERIOD_COOLDOWN);
					return;
				}
			}
		}
	}

	CSDKPlayer *pPenTaker = GetMatchBall()->GetPenaltyTaker();
	CSDKPlayer *pKeeper = pPenTaker ? pPenTaker->GetOppTeam()->GetPlayerByPosType(POS_GK) : NULL;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		// Use TV camera mode for all players other than the penalty taker and the opponent keeper
		if (pPl == pPenTaker || pPl == pKeeper)
			pPl->RemoveFlag(FL_USE_TV_CAM);
		else
			pPl->AddFlag(FL_USE_TV_CAM);
	}
}

void CSDKGameRules::State_PENALTIES_Leave(match_period_t newState)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		pPl->RemoveFlag(FL_USE_TV_CAM);
	}

	GetMatchBall()->EmitSound("Ball.Whistle");
	//GetMatchBall()->EmitSound("Crowd.EndOfPeriod");
}

void CSDKGameRules::State_COOLDOWN_Enter()
{
	m_nRealMatchEndTime = time(NULL);

	ApplyIntermissionSettings(true, false);

	//who won?
	int winners = 0;
	int scoreA = GetGlobalTeam( TEAM_HOME )->GetGoals();
	int scoreB = GetGlobalTeam( TEAM_AWAY )->GetGoals();
	if (scoreA > scoreB)
		winners = TEAM_HOME;
	if (scoreB > scoreA)
		winners = TEAM_AWAY;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSDKPlayer *pPlayer = (CSDKPlayer*)UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		//is this player on the winning team
		if (pPlayer->GetTeamNumber() == winners)
		{
			pPlayer->AddFlag(FL_CELEB);
		}
	}

	CPlayerData::ConvertAllPlayerDataToJson();
}

void CSDKGameRules::State_COOLDOWN_Think()
{
	if (m_flStateTimeLeft <= 0)
		GoToIntermission();
}

void CSDKGameRules::State_COOLDOWN_Leave(match_period_t newState)
{
}

void CSDKGameRules::ApplyIntermissionSettings(bool startHighlights, bool movePlayers)
{
	if (movePlayers)
	{
		GetMatchBall()->State_Transition(BALL_STATE_NORMAL, 0, 0, true);
		GetMatchBall()->SetPos(m_vKickOff);

		if (m_bIsCeremony)
			EnableShield(SHIELD_CEREMONY, TEAM_HOME, SDKGameRules()->m_vKickOff);
		else
			EnableShield(SHIELD_KICKOFF, TEAM_HOME, SDKGameRules()->m_vKickOff);
	}
	else
		GetMatchBall()->UpdatePossession(NULL);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (movePlayers)
			pPl->SetPosOutsideShield(false);

		pPl->SetLastMoveTime(gpGlobals->curtime);
		pPl->SetAway(true);
	}

	m_flLastAwayCheckTime = gpGlobals->curtime;

	if (startHighlights)
		ReplayManager()->StartHighlights();
}

bool CSDKGameRules::CheckAutoStart()
{
	int requiredPlayerCount = mp_maxplayers.GetInt() * 2;

	if (sv_singlekeeper.GetBool())
		requiredPlayerCount -= 1;

	if (sv_autostartmatch.GetBool()
		&& !m_bIsCeremony
		&& GetGlobalTeam(TEAM_HOME)->GetNumPlayers() + GetGlobalTeam(TEAM_AWAY)->GetNumPlayers() >= requiredPlayerCount
		&& gpGlobals->curtime >= m_flLastAwayCheckTime + sv_wakeupcall_interval.GetFloat())
	{
		m_flLastAwayCheckTime = gpGlobals->curtime;
		return (WakeUpAwayPlayers() == 0);
	}

	return false;
}

void CSDKGameRules::CalcAnnouncedInjuryTime()
{
	int adjustedTransitionTime = m_flBallStateTransitionTime / 60 * (90.0f / mp_timelimit_match.GetFloat()) * mp_injurytime_coeff.GetFloat();
	m_nAnnouncedInjuryTime = clamp(adjustedTransitionTime, mp_injurytime_min.GetInt(), mp_injurytime_max.GetInt());
}

#include <ctype.h>

char *trim(char *str)
{
	size_t len = 0;
	char *frontp = str - 1;
	char *endp = NULL;

	if( str == NULL )
		return NULL;

	if( str[0] == '\0' )
		return str;

	len = strlen(str);
	endp = str + len;

	/* Move the front and back pointers to address
	* the first non-whitespace characters from
	* each end.
	*/
	while( isspace(*(++frontp)) );
	while( isspace(*(--endp)) && endp != frontp );

	if( str + len - 1 != endp )
		*(endp + 1) = '\0';
	else if( frontp != str &&  endp == frontp )
		*str = '\0';

	/* Shift the string so that it starts at str so
	* that if it's dynamically allocated, we can
	* still free it on the returned pointer.  Note
	* the reuse of endp to mean the front of the
	* string buffer now.
	*/
	endp = str;
	if( frontp != str )
	{
		while( *frontp ) *endp++ = *frontp++;
		*endp = '\0';
	}

	return str;
}

void OnTeamnamesChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	if (!SDKGameRules())
		return;

	char val[256];
	Q_strncpy(val, ((ConVar*)var)->GetString(), sizeof(val));

	if (val[0] == 0)
		return;

	char homeString[128] = {};
	char awayString[128] = {};
	char *result = NULL;

	result = strtok(val, ",");

	if (result != NULL)
		Q_strncpy(homeString, result, sizeof(homeString));

	if (homeString[0] != '\0')
	{
		result = strtok(NULL, ",");

		if (result != NULL)
			Q_strncpy(awayString, result, sizeof(awayString));
	}

	if (homeString[0] != '\0' && awayString[0] != '\0')
	{
		char homeCode[MAX_TEAMCODE_LENGTH] = {};
		char homeName[MAX_SHORTTEAMNAME_LENGTH] = {};
		char awayCode[MAX_TEAMCODE_LENGTH] = {};
		char awayName[MAX_SHORTTEAMNAME_LENGTH] = {};

		result = strtok(homeString, ":");

		if (result != NULL)
			Q_strncpy(homeCode, result, sizeof(homeCode));

		if (homeCode[0] != '\0')
		{
			result = strtok(NULL, ":");

			if (result != NULL)
				Q_strncpy(homeName, result, sizeof(homeName));

			if (homeName[0] != '\0')
			{
				result = strtok(awayString, ":");

				if (result != NULL)
					Q_strncpy(awayCode, result, sizeof(awayCode));

				if (awayCode[0] != '\0')
				{
					result = strtok(NULL, ":");

					if (result != NULL)
						Q_strncpy(awayName, result, sizeof(awayName));

					if (awayName[0] != '\0')
					{
						GetGlobalTeam(TEAM_HOME)->SetTeamCode(trim(homeCode));
						GetGlobalTeam(TEAM_HOME)->SetShortTeamName(trim(homeName));

						GetGlobalTeam(TEAM_AWAY)->SetTeamCode(trim(awayCode));
						GetGlobalTeam(TEAM_AWAY)->SetShortTeamName(trim(awayName));

						return;
					}
				}
			}
		}
	}

	Msg("Error: Wrong format\n");
}

ConVar mp_teamnames("mp_teamnames", "", FCVAR_NOTIFY, "Override team names. Example: mp_teamnames \"FCB:FC Barcelona,RMA:Real Madrid\"", &OnTeamnamesChange);


void CC_SV_UpdateServer(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	SDKGameRules()->StartServerUpdate();
}

ConCommand sv_update_server("sv_update_server", CC_SV_UpdateServer, "", 0);

void CSDKGameRules::StartServerUpdate()
{
	Msg("Server Updater: Updating all server files. Please wait...\n");

	m_pServerUpdateInfo->Reset();
	m_pServerUpdateInfo->async = true;
	CFileUpdater::UpdateFiles(m_pServerUpdateInfo);
}

void CSDKGameRules::CheckServerUpdateStatus()
{
	if (!m_pServerUpdateInfo->finished)
		return;

	const char *msg;

	if (m_pServerUpdateInfo->connectionError)
		msg = "Server Updater: Couldn't connect to the update server.";
	else if (m_pServerUpdateInfo->checkOnly)
		msg = "Server Updater: Check for changes successful.";
	else
	{
		if (m_pServerUpdateInfo->filesToUpdateCount == 0)
			msg = "Server Updater: All server files are up to date.";
		else
		{
			CTeamInfo::ParseTeamKits();
			CShoeInfo::ParseShoes();
			CKeeperGloveInfo::ParseKeeperGloves();
			CBallInfo::ParseBallSkins();
			CPitchInfo::ParsePitchTextures();

			if (m_pServerUpdateInfo->restartRequired)
				msg = "Server Updater: Server files successfully updated. A server restart is required to use the new binaries.";
			else
				msg = "Server Updater: Server files successfully updated. A server restart might be required to use the new files.";
		}
	}

	char consoleMsg[256];
	Q_snprintf(consoleMsg, sizeof(consoleMsg), "%s\n", msg);
	Msg(consoleMsg);

	UTIL_ClientPrintAll(HUD_PRINTCONSOLE, msg);

	m_pServerUpdateInfo->finished = false;
}

void CC_MP_Teamkits(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	if (args.ArgC() == 1)
	{
		char list[10240] = {};
		int teamCount = 0;
		int kitCount = 0;

		Q_strcat(list, "\n----------------------------------------\n", sizeof(list));

		for (int i = 0; i < CTeamInfo::m_TeamInfo.Count(); i++)
		{
			teamCount += 1;
			kitCount = 0;
			Q_strcat(list, UTIL_VarArgs("%s:\n", CTeamInfo::m_TeamInfo[i]->m_szShortName), sizeof(list));
			for (int j = 0; j < CTeamInfo::m_TeamInfo[i]->m_TeamKitInfo.Count(); j++)
			{
				kitCount += 1;
				Q_strcat(list, UTIL_VarArgs("    %d: %s [by %s]\n", teamCount * 100 + kitCount, CTeamInfo::m_TeamInfo[i]->m_TeamKitInfo[j]->m_szName, CTeamInfo::m_TeamInfo[i]->m_TeamKitInfo[j]->m_szAuthor), sizeof(list));
			}
		}

		Q_strcat(list, "----------------------------------------\n", sizeof(list));

		Q_strcat(list, "\nUse 'mp_teamkits <home kit number> <away kit number>' to set the kits. E.g. 'mp_teamkits 103 202'\n\n", sizeof(list));

		Msg(list);
	}
	else if (args.ArgC() == 3)
	{
		int homeNumber = atoi(args[1]);
		int homeTeamIndex = homeNumber / 100 - 1;
		int homeKitIndex = homeNumber % 100 - 1;

		int awayNumber = atoi(args[2]);
		int awayTeamIndex = awayNumber / 100 - 1;
		int awayKitIndex = awayNumber % 100 - 1;

		char homeKit[256] = {};
		char awayKit[256] = {};

		if (homeTeamIndex >= 0 && CTeamInfo::m_TeamInfo.Count() > homeTeamIndex
			&& homeKitIndex >= 0 && CTeamInfo::m_TeamInfo[homeTeamIndex]->m_TeamKitInfo.Count() > homeKitIndex)
			Q_snprintf(homeKit, sizeof(homeKit), "%s/%s", CTeamInfo::m_TeamInfo[homeTeamIndex]->m_szFolderName, CTeamInfo::m_TeamInfo[homeTeamIndex]->m_TeamKitInfo[homeKitIndex]->m_szFolderName);

		if (awayTeamIndex >= 0 && CTeamInfo::m_TeamInfo.Count() > awayTeamIndex
			&& awayKitIndex >= 0 && CTeamInfo::m_TeamInfo[awayTeamIndex]->m_TeamKitInfo.Count() > awayKitIndex)
			Q_snprintf(awayKit, sizeof(awayKit), "%s/%s", CTeamInfo::m_TeamInfo[awayTeamIndex]->m_szFolderName, CTeamInfo::m_TeamInfo[awayTeamIndex]->m_TeamKitInfo[awayKitIndex]->m_szFolderName);

		if (homeKit[0] != '\0' && awayKit[0] != '\0')
		{
			GetGlobalTeam(TEAM_HOME)->SetKitName(homeKit);
			GetGlobalTeam(TEAM_AWAY)->SetKitName(awayKit);
		}
		else
		{
			if (homeKit[0] == '\0')
			{
				Msg("Error: Home team or kit not found.\n");
			}
			if (awayKit[0] == '\0')
			{
				Msg("Error: Away team or kit not found.\n");
			}
		}
	}
	else
	{
		Msg("Error: Wrong syntax.\n");
	}
}

ConCommand mp_teamkits("mp_teamkits", CC_MP_Teamkits, "", 0);


void CC_MP_BallSkin(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	if (args.ArgC() == 1)
	{
		char list[2048] = {};
		int ballCount = 0;

		Q_strcat(list, "\n----------------------------------------\n", sizeof(list));

		for (int i = 0; i < CBallInfo::m_BallInfo.Count(); i++)
		{
			ballCount += 1;
			Q_strcat(list, UTIL_VarArgs("%d: %s [by %s]\n", ballCount, CBallInfo::m_BallInfo[i]->m_szName, CBallInfo::m_BallInfo[i]->m_szAuthor), sizeof(list));
		}

		Q_strcat(list, "----------------------------------------\n", sizeof(list));

		Q_strcat(list, "\nUse 'mp_ballskin <ball number>' to set the ball. E.g. 'mp_ballskin 3'\n\n", sizeof(list));

		Msg(list);
	}
	else if (args.ArgC() == 2)
	{
		int ballSkinIndex = atoi(args[1]) - 1;

		char ballSkinName[256] = {};

		if (ballSkinIndex >= 0 && ballSkinIndex < CBallInfo::m_BallInfo.Count())
			Q_snprintf(ballSkinName, sizeof(ballSkinName), "%s", CBallInfo::m_BallInfo[ballSkinIndex]->m_szFolderName);

		if (ballSkinName[0] != '\0')
			GetMatchBall()->SetSkinName(ballSkinName);
		else
			Msg("Error: Ball skin not found.\n");
	}
	else
	{
		Msg("Error: Wrong syntax.\n");
	}
}

ConCommand mp_ballskin("mp_ballskin", CC_MP_BallSkin, "", 0);


void CC_MP_PitchTexture(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	if (args.ArgC() == 1)
	{
		char list[2048] = {};
		int pitchCount = 0;

		Q_strcat(list, "\n----------------------------------------\n", sizeof(list));

		for (int i = 0; i < CPitchInfo::m_PitchInfo.Count(); i++)
		{
			pitchCount += 1;
			Q_strcat(list, UTIL_VarArgs("%d: %s (%s) [by %s]\n", pitchCount, CPitchInfo::m_PitchInfo[i]->m_szName, g_szFieldMaterials[CPitchInfo::m_PitchInfo[i]->m_nType], CPitchInfo::m_PitchInfo[i]->m_szAuthor), sizeof(list));
		}

		Q_strcat(list, "----------------------------------------\n", sizeof(list));

		Q_strcat(list, "\nUse 'mp_pitchtexture <pitch number>' to set the pitch. E.g. 'mp_pitchtexture 3'\n\n", sizeof(list));

		Msg(list);
	}
	else if (args.ArgC() == 2)
	{
		int pitchTextureIndex = atoi(args[1]) - 1;

		char pitchTextureName[256] = {};

		if (pitchTextureIndex >= 0 && pitchTextureIndex < CPitchInfo::m_PitchInfo.Count())
			Q_snprintf(pitchTextureName, sizeof(pitchTextureName), "%s", CPitchInfo::m_PitchInfo[pitchTextureIndex]->m_szFolderName);

		if (pitchTextureName[0] != '\0')
			SDKGameRules()->SetPitchTextureName(pitchTextureName);
		else
			Msg("Error: Pitch texture not found.\n");
	}
	else
	{
		Msg("Error: Wrong syntax.\n");
	}
}

ConCommand mp_pitchtexture("mp_pitchtexture", CC_MP_PitchTexture, "", 0);


ConVar mp_clientsettingschangeinterval("mp_clientsettingschangeinterval", "2", FCVAR_REPLICATED|FCVAR_NOTIFY, "");


void CSDKGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	CSDKPlayer *pPl = ToSDKPlayer(pPlayer);

	if (!Q_strcmp(engine->GetPlayerNetworkIDString(pPlayer->edict()), "BOT")
		|| gpGlobals->curtime < pPl->m_flNextClientSettingsChangeTime
		|| (!IsIntermissionState() && (pPl->GetTeamNumber() == TEAM_HOME || pPl->GetTeamNumber() == TEAM_AWAY)))
		return;

	pPl->m_flNextClientSettingsChangeTime = gpGlobals->curtime + mp_clientsettingschangeinterval.GetFloat();

	int countryIndex = atoi(engine->GetClientConVarValue(pPl->entindex(), "countryindex"));

	if (countryIndex <= 0 || countryIndex > COUNTRY_NAMES_COUNT - 1)
		countryIndex = 0;

	pPl->SetCountryIndex(countryIndex);

	pPl->SetReverseSideCurl(atoi(engine->GetClientConVarValue(pPl->entindex(), "reversesidecurl")) != 0);

	pPl->SetPreferredOutfieldShirtNumber(atoi(engine->GetClientConVarValue(pPl->entindex(), "preferredoutfieldshirtnumber")));
	pPl->SetPreferredKeeperShirtNumber(atoi(engine->GetClientConVarValue(pPl->entindex(), "preferredkeepershirtnumber")));

	pPl->SetSkinIndex(atoi(engine->GetClientConVarValue(pPl->entindex(), "modelskinindex")));

	pPl->SetHairIndex(atoi(engine->GetClientConVarValue(pPl->entindex(), "modelhairindex")));

	pPl->SetSleeveIndex(atoi(engine->GetClientConVarValue(pPl->entindex(), "modelsleeveindex")));

	pPl->SetShoeName(engine->GetClientConVarValue(pPl->entindex(), "modelshoename"));

	pPl->SetKeeperGloveName(engine->GetClientConVarValue(pPl->entindex(), "modelkeeperglovename"));

	pPl->SetPlayerBallSkinName(engine->GetClientConVarValue(pPl->entindex(), "playerballskinname"));

	char pszName[MAX_PLAYER_NAME_LENGTH];
	Q_strncpy(pszName, engine->GetClientConVarValue( pPl->entindex(), "playername" ), MAX_PLAYER_NAME_LENGTH);

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive

	char oldPlayerName[MAX_PLAYER_NAME_LENGTH];

	Q_strncpy(oldPlayerName, pPl->GetPlayerName(), sizeof(oldPlayerName));

	pPl->SetPlayerName(pszName);

	if (Q_strcmp(pPl->GetPlayerName(), oldPlayerName))
	{
		IGameEvent * event = gameeventmanager->CreateEvent("player_changename");
		if ( event )
		{
			event->SetInt("userid", pPl->GetUserID());
			event->SetString("oldname", oldPlayerName);
			event->SetString("newname", pPl->GetPlayerName());
			gameeventmanager->FireEvent(event);
		}
		
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

	char pszNationalTeamName[MAX_CLUBNAME_LENGTH];
	Q_strncpy(pszNationalTeamName, engine->GetClientConVarValue( pPl->entindex(), "nationalteamname" ), MAX_CLUBNAME_LENGTH);

	const char *pszOldNationalTeamName = pPl->GetNationalTeamName();

	if (Q_strcmp(pszOldNationalTeamName, pszNationalTeamName))
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "player_changenationalteam" );
		if ( event )
		{
			event->SetInt("userid", pPl->GetUserID());
			event->SetString("oldnationalteam", pszOldNationalTeamName);
			event->SetString("newnationalteam", pszNationalTeamName);
			gameeventmanager->FireEvent(event);
		}
		
		pPl->SetNationalTeamName(pszNationalTeamName);
	}

	pPl->SetShirtName(engine->GetClientConVarValue(pPl->entindex(), "shirtname"));
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

		pPl->RemoveFlags();
		//pPl->RemoveSolidFlags(FSOLID_NOT_SOLID);
		pPl->SetCollisionGroup(COLLISION_GROUP_PLAYER);

		if (!pPl->m_bIsAtTargetPos)
		{
			Vector pos = pPl->GetLocalOrigin();
			pPl->FindSafePos(pos);
			pPl->SetLocalOrigin(pos);
		}

		pPl->m_bIsAtTargetPos = true;
		pPl->m_flRemoteControlledStartTime = -1;
	}
}

void CSDKGameRules::SetBottomTeam(int team)
{
	if (team == m_nBottomTeam)
		return;

	m_nBottomTeam = team;

	GetGlobalTeam(m_nBottomTeam)->InitFieldSpots(true);
	GetGlobalTeam(GetGlobalTeam(m_nBottomTeam)->GetOppTeamNumber())->InitFieldSpots(false);
}

int CSDKGameRules::GetBottomTeam()
{
	return m_nBottomTeam;
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

void CSDKGameRules::StartMeteringClockStoppedTime()
{
	StopMeteringClockStoppedTime();
	m_flClockStoppedStart = gpGlobals->curtime;
}

void CSDKGameRules::StopMeteringClockStoppedTime()
{
	if (m_flClockStoppedStart != -1)
	{
		float timePassed = gpGlobals->curtime - m_flClockStoppedStart;
		m_flClockStoppedTime += timePassed;
		m_flClockStoppedStart = -1;
	}
}

void CSDKGameRules::AddBallStateTransitionTime(float duration)
{
	m_flBallStateTransitionTime += duration;
}

#endif

bool CSDKGameRules::IsIntermissionState()
{
	switch (State_Get())
	{
	case MATCH_PERIOD_WARMUP:
	case MATCH_PERIOD_HALFTIME:
	case MATCH_PERIOD_EXTRATIME_INTERMISSION:
	case MATCH_PERIOD_EXTRATIME_HALFTIME:
	case MATCH_PERIOD_PENALTIES_INTERMISSION:
	case MATCH_PERIOD_COOLDOWN:
		return true;
	default:
		return false;
	}
}

int CSDKGameRules::GetShieldRadius(int team, bool isTaker)
{
	switch (m_nShieldType)
	{
	case SHIELD_THROWIN: return (team == m_nShieldTeam && !isTaker ? mp_shield_throwin_radius_teammate.GetInt() : mp_shield_throwin_radius_opponent.GetInt());
	case SHIELD_FREEKICK: return (team == m_nShieldTeam && !isTaker ? mp_shield_freekick_radius_teammate.GetInt() : mp_shield_freekick_radius_opponent.GetInt());
	case SHIELD_CORNER: return (team == m_nShieldTeam && !isTaker ? mp_shield_corner_radius_teammate.GetInt() : mp_shield_corner_radius_opponent.GetInt());
	case SHIELD_KICKOFF: return (team == m_nShieldTeam && !isTaker ? mp_shield_kickoff_radius_teammate.GetInt() : mp_shield_kickoff_radius_opponent.GetInt());
	case SHIELD_PENALTY: return mp_shield_kickoff_radius_opponent.GetInt();
	default: return 0;
	}
}

float CSDKGameRules::GetMatchDisplayTimeSeconds(bool addInjuryTime /*= true*/, bool getCountdownAtIntermissions /*= true*/)
{
	float realSeconds = gpGlobals->curtime - SDKGameRules()->m_flStateEnterTime - SDKGameRules()->m_flClockStoppedTime;

	if (SDKGameRules()->m_flClockStoppedStart != -1)
		realSeconds -= gpGlobals->curtime - SDKGameRules()->m_flClockStoppedStart;

	float matchSeconds;

	switch ( SDKGameRules()->State_Get() )
	{
	case MATCH_PERIOD_PENALTIES:
		matchSeconds = (mp_extratime.GetBool() ? 120 : 90) * 60;
		break;
	case MATCH_PERIOD_EXTRATIME_SECOND_HALF:
		matchSeconds = (int)(realSeconds * (90.0f / mp_timelimit_match.GetFloat())) + 105 * 60;
		if (!addInjuryTime)
			matchSeconds = min(120 * 60, matchSeconds);
		break;
	case MATCH_PERIOD_EXTRATIME_FIRST_HALF:
		matchSeconds = (int)(realSeconds * (90.0f / mp_timelimit_match.GetFloat())) + 90 * 60;
		if (!addInjuryTime)
			matchSeconds = min(105 * 60, matchSeconds);
		break;
	case MATCH_PERIOD_SECOND_HALF:
		matchSeconds = (int)(realSeconds * (90.0f / mp_timelimit_match.GetFloat())) + 45 * 60;
		if (!addInjuryTime)
			matchSeconds = min(90 * 60, matchSeconds);
		break;
	case MATCH_PERIOD_FIRST_HALF:
		matchSeconds = (int)(realSeconds * (90.0f / mp_timelimit_match.GetFloat()));
		if (!addInjuryTime)
			matchSeconds = min(45 * 60, matchSeconds);
		break;
	case MATCH_PERIOD_WARMUP:
		if (!getCountdownAtIntermissions)
			matchSeconds = 0;
		else
			matchSeconds = (int)(realSeconds - mp_timelimit_warmup.GetFloat() * 60);
		break;
	case MATCH_PERIOD_HALFTIME:
		if (!getCountdownAtIntermissions)
			matchSeconds = 45 * 60;
		else
			matchSeconds = (int)(realSeconds - mp_timelimit_halftime.GetFloat() * 60);
		break;
	case MATCH_PERIOD_EXTRATIME_INTERMISSION:
		if (!getCountdownAtIntermissions)
			matchSeconds = 90 * 60;
		else
			matchSeconds = (int)(realSeconds - mp_timelimit_extratime_intermission.GetFloat() * 60);
		break;
	case MATCH_PERIOD_EXTRATIME_HALFTIME:
		if (!getCountdownAtIntermissions)
			matchSeconds = 105 * 60;
		else
			matchSeconds = (int)(realSeconds - mp_timelimit_extratime_halftime.GetFloat() * 60);
		break;
	case MATCH_PERIOD_PENALTIES_INTERMISSION:
		if (!getCountdownAtIntermissions)
			matchSeconds = (mp_extratime.GetBool() ? 120 : 90) * 60;
		else
			matchSeconds = (int)(realSeconds - mp_timelimit_penalties_intermission.GetFloat() * 60);
		break;
	case MATCH_PERIOD_COOLDOWN:
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

			matchSeconds = minute * 60;
		}
		else
			matchSeconds = (int)(realSeconds - mp_timelimit_cooldown.GetFloat() * 60);
		break;
	default:
		matchSeconds = 0;
		break;
	}

	return matchSeconds;
}

ConVar mp_daytime_enabled("mp_daytime_enabled", "0", FCVAR_NOTIFY | FCVAR_REPLICATED);
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
	SetSprayLinesEnabled(false);
	DisableShield();
	m_nTimeoutTeam = TEAM_NONE;
	m_eTimeoutState = TIMEOUT_STATE_NONE;
	m_flTimeoutEnd = 0;
	m_flLastAwayCheckTime = gpGlobals->curtime;

	GetMatchBall()->Reset();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		pPl->Reset();
	}

	GetGlobalTeam(TEAM_HOME)->ResetStats();
	GetGlobalTeam(TEAM_AWAY)->ResetStats();

	CPlayerData::ReallocateAllPlayerData();

	ReplayManager()->CleanUp();
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

	GetMatchBall()->ReloadSettings();
}

void CSDKGameRules::SetMatchDisplayTimeSeconds(int seconds)
{
	m_bUseAdjustedStateEnterTime = true;
	float minute = seconds / 60.0f;
	match_period_t matchPeriod;

	if (minute >= 120)
	{
		m_flAdjustedStateEnterTime = gpGlobals->curtime - ((seconds - 120 * 60) / (90.0f / mp_timelimit_match.GetFloat()));
		matchPeriod = MATCH_PERIOD_PENALTIES;
	}
	else if (minute >= 105)
	{
		m_flAdjustedStateEnterTime = gpGlobals->curtime - ((seconds - 105 * 60) / (90.0f / mp_timelimit_match.GetFloat()));
		matchPeriod = MATCH_PERIOD_EXTRATIME_SECOND_HALF;
	}
	else if (minute >= 90)
	{
		m_flAdjustedStateEnterTime = gpGlobals->curtime - ((seconds - 90 * 60) / (90.0f / mp_timelimit_match.GetFloat()));
		matchPeriod = MATCH_PERIOD_EXTRATIME_FIRST_HALF;
	}
	else if (minute >= 45)
	{
		m_flAdjustedStateEnterTime = gpGlobals->curtime - ((seconds - 45 * 60) / (90.0f / mp_timelimit_match.GetFloat()));
		matchPeriod = MATCH_PERIOD_SECOND_HALF;
	}
	else
	{
		m_flAdjustedStateEnterTime = gpGlobals->curtime - ((seconds - 0 * 60) / (90.0f / mp_timelimit_match.GetFloat()));
		matchPeriod = MATCH_PERIOD_FIRST_HALF;
	}

	ResetMatch();
	m_flMatchStartTime = gpGlobals->curtime - (seconds / (90.0f / mp_timelimit_match.GetFloat()));
	GetMatchBall()->State_Transition(BALL_STATE_STATIC, 0, 0, true);
	SetBottomTeam(m_nFirstHalfBottomTeam);
	SetKickOffTeam(m_nFirstHalfKickOffTeam);
	State_Transition(matchPeriod);
}

bool CSDKGameRules::CheckTimeout()
{
	if (GetTimeoutState() == TIMEOUT_STATE_PENDING)
	{
		SetTimeoutState(TIMEOUT_STATE_ACTIVE);

		if (GetTimeoutTeam() == TEAM_NONE)
			SetTimeoutEnd(-1);
		else
			SetTimeoutEnd(gpGlobals->curtime + GetGlobalTeam(GetTimeoutTeam())->GetTimeoutTimeLeft());

		IGameEvent *pEvent = gameeventmanager->CreateEvent("start_timeout");
		if (pEvent)
		{
			pEvent->SetInt("requesting_team", GetTimeoutTeam());
			gameeventmanager->FireEvent(pEvent);
		}

		return true;
	}
	else if (GetTimeoutState() == TIMEOUT_STATE_ACTIVE)
	{
		if (GetTimeoutEnd() != -1 && gpGlobals->curtime >= GetTimeoutEnd())
			return EndTimeout();

		return true;
	}

	return false;
}

bool CSDKGameRules::EndTimeout()
{
	if (SDKGameRules()->GetTimeoutState() == TIMEOUT_STATE_PENDING)
	{
		SDKGameRules()->SetTimeoutState(TIMEOUT_STATE_NONE);

		IGameEvent *pEvent = gameeventmanager->CreateEvent("end_timeout");
		if (pEvent)
		{
			gameeventmanager->FireEvent(pEvent);
		}
	}
	else if (SDKGameRules()->GetTimeoutState() == TIMEOUT_STATE_ACTIVE)
	{
		if (SDKGameRules()->GetTimeoutTeam() == TEAM_NONE && GetTimeoutEnd() != -1)
		{
			SDKGameRules()->SetTimeoutState(TIMEOUT_STATE_NONE);

			UTIL_ClientPrintAll(HUD_PRINTTALK, "#game_match_start");
			UTIL_ClientPrintAll(HUD_PRINTCENTER, "LIVE");

			return false;
		}
		
		if (SDKGameRules()->GetTimeoutTeam() != TEAM_NONE)
		{
			GetGlobalTeam(SDKGameRules()->GetTimeoutTeam())->SetTimeoutTimeLeft(max(0, (int)(SDKGameRules()->GetTimeoutEnd() - gpGlobals->curtime)));
			SDKGameRules()->SetTimeoutTeam(TEAM_NONE);
		}

		SDKGameRules()->SetTimeoutEnd(gpGlobals->curtime + 10);

		IGameEvent *pEvent = gameeventmanager->CreateEvent("start_timeout");
		if (pEvent)
		{
			pEvent->SetInt("requesting_team", TEAM_NONE);
			gameeventmanager->FireEvent(pEvent);
		}

		UTIL_ClientPrintAll(HUD_PRINTCENTER, "Match resuming in 10 seconds");
	}

	return true;
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

void CSDKGameRules::SetSprayLinesEnabled(bool enable)
{
	m_bSprayLinesEnabled = enable;
}

void CSDKGameRules::CheckChatText(CBasePlayer *pPlayer, char *pText)
{
	BaseClass::CheckChatText( pPlayer, pText );
}

ConVar sv_required_client_version("sv_required_client_version", g_szRequiredClientVersion);

bool CSDKGameRules::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	if (Q_strcmp(engine->GetPlayerNetworkIDString(pEntity), "BOT"))
	{
		const char *pszClientVersion = engine->GetClientConVarValue(engine->IndexOfEdict(pEntity), "clientversion");
		if (Q_strcmp(pszClientVersion, sv_required_client_version.GetString()))
		{
			Q_snprintf(reject, maxrejectlen, "%s", "GAME UPDATE REQUIRED");
			return false;
		}
	}

	return BaseClass::ClientConnected(pEntity, pszName, pszAddress, reject, maxrejectlen);
}

void CSDKGameRules::SetPitchTextureName(const char *textureName)
{
	if (m_szPitchTextureName[0] != '\0' && !Q_strcmp(textureName, m_szPitchTextureName))
		return;

	int pitchTextureIndex = -1;

	for (int i = 0; i < CPitchInfo::m_PitchInfo.Count(); i++)
	{
		if (!Q_strcmp(CPitchInfo::m_PitchInfo[i]->m_szFolderName, textureName))
		{
			pitchTextureIndex = i;
			break;
		}
	}

	if (pitchTextureIndex == -1)
		pitchTextureIndex = g_IOSRand.RandomInt(0, CPitchInfo::m_PitchInfo.Count() - 1);

	Q_strncpy(m_szPitchTextureName.GetForModify(), CPitchInfo::m_PitchInfo[pitchTextureIndex]->m_szFolderName, sizeof(m_szPitchTextureName));
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
	
	pPl->SetDesiredTeam(TEAM_SPECTATOR, TEAM_SPECTATOR, 0, true, true, true);
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

		if (team == 0 || pPl->GetTeamNumber() == team + TEAM_HOME - 1)
		{
			pPl->SetDesiredTeam(TEAM_SPECTATOR, TEAM_SPECTATOR, 0, true, true, true);
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

	Vector mins = Vector(SDKGameRules()->m_vFieldMin.GetX(), posY - 1, SDKGameRules()->m_vKickOff.GetZ());
	Vector maxs = Vector(SDKGameRules()->m_vFieldMax.GetX(), posY + 1, SDKGameRules()->m_vKickOff.GetZ() + 2);

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

	for (int team = TEAM_HOME; team <= TEAM_AWAY; team++)
	{
		if (!GetGlobalTeam(team)->HasCrest())
			continue;

		char *material = team == TEAM_HOME ? "vgui/hometeamcrest" : "vgui/awayteamcrest";
		int sign = team == m_nBottomTeam ? -1 : 1;

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

void CSDKGameRules::DrawSprayLines()
{
	if (!m_bSprayLinesEnabled)
		return;

	float length = 75;
	float width = 2;
	int radius = mp_shield_freekick_radius_opponent.GetInt() + mp_shield_border.GetInt();
	Vector dir = GetGlobalTeam(m_nShieldTeam)->GetOppTeam()->m_vGoalCenter - m_vShieldPos;
	dir.NormalizeInPlace();
	Vector ort = Vector(-dir.y, dir.x, dir.z);
	Vector origin = m_vShieldPos + dir * (radius - width);
	origin.z += 1;

	CMatRenderContextPtr pRenderContext( materials );
	IMaterial *pPreviewMaterial = materials->FindMaterial("pitch/spray_line", TEXTURE_GROUP_CLIENT_EFFECTS);
	pRenderContext->Bind( pPreviewMaterial );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
	meshBuilder.TexCoord2f( 0,0,0 );
	meshBuilder.Position3fv( (origin + (ort * -length) + (dir * -width)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
	meshBuilder.TexCoord2f( 0,1,0 );
	meshBuilder.Position3fv( (origin + (ort * length) + (dir * -width)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
	meshBuilder.TexCoord2f( 0,1,1 );
	meshBuilder.Position3fv( (origin + (ort * length) + (dir * width)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
	meshBuilder.TexCoord2f( 0,0,1 );
	meshBuilder.Position3fv( (origin + (ort * -length) + (dir * width)).Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.End();
	pMesh->Draw();
}

void CSDKGameRules::DrawGoalTeamCrests()
{
	return;

	for (int i = 0; i < 2; i++)
	{
		if (!GetGlobalTeam(i + TEAM_HOME)->HasCrest())
			continue;

		int sign;
		char *material;

		if (i == 0)
		{
			sign = m_nBottomTeam == TEAM_HOME ? 1 : -1;
			material = "vgui/hometeamcrest";
		}
		else
		{
			sign = m_nBottomTeam == TEAM_HOME ? -1 : 1;
			material = "vgui/awayteamcrest";
		}

		Vector right = Vector(-1, 0, 0);
		Vector up = Vector(0, 0, -1);
		float size = 150;
		Vector origin = GetGlobalTeam(i + TEAM_HOME)->m_vGoalCenter;
		origin.y += sign * (40 + size);
		origin.z += 350;

		CMatRenderContextPtr pRenderContext( materials );
		IMaterial *pPreviewMaterial = materials->FindMaterial( material, TEXTURE_GROUP_CLIENT_EFFECTS );
		pRenderContext->Bind( pPreviewMaterial );
		IMesh *pMesh = pRenderContext->GetDynamicMesh();
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,0,0 );
		meshBuilder.Position3fv( (origin + (right * sign * size) + (up * -size)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,1,0 );
		meshBuilder.Position3fv( (origin + (right * sign * -size) + (up * -size)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,1,1 );
		meshBuilder.Position3fv( (origin + (right * sign * -size) + (up * size)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,0,1 );
		meshBuilder.Position3fv( (origin + (right * sign * size) + (up * size)).Base() );
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

void CSDKGameRules::DrawCelebScreen()
{
	C_SDKPlayer *pPl = C_SDKPlayer::GetLocalSDKPlayer();
	if(!pPl)
		return;

	if (!GetMatchBall() || GetMatchBall()->m_eBallState != BALL_STATE_GOAL)
		return;

	if (GetReplayManager() && GetReplayManager()->IsReplaying())
		return;

	if (!(pPl->GetFlags() & FL_CELEB))
		return;

	C_BaseEntity *pTarget = GetMatchBall()->m_pLastActivePlayer;
	if (!pTarget)
		return;

	char *material = "_rt_celebcam";
	float size = 50;
	Vector origin;
	QAngle angles;
	float fov;

	Camera()->CalcTVCamView(origin, angles, fov);

	Vector forward, right, up;
	AngleVectors(angles, &forward, &right, &up);

	CMatRenderContextPtr pRenderContext( materials );
	IMaterial *pPreviewMaterial = materials->FindMaterial( material, TEXTURE_GROUP_CLIENT_EFFECTS );
	pRenderContext->Bind( pPreviewMaterial );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4f( 1.0, 1.0, 1.0, 0.5 );
	meshBuilder.TexCoord2f( 0,0,1 );
	meshBuilder.Position3fv( (origin + (-right * size) + (-up * size)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4f( 1.0, 1.0, 1.0, 0.5 );
	meshBuilder.TexCoord2f( 0,1,1 );
	meshBuilder.Position3fv( (origin + (right * size) + (-up * size)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4f( 1.0, 1.0, 1.0, 0.5 );
	meshBuilder.TexCoord2f( 0,1,0 );
	meshBuilder.Position3fv( (origin + (right * size) + (up * size)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4f( 1.0, 1.0, 1.0, 0.5 );
	meshBuilder.TexCoord2f( 0,0,0 );
	meshBuilder.Position3fv( (origin + (-right * size) + (up * size)).Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.End();
	pMesh->Draw();
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
