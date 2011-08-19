//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "sdk_player.h"
#include "sdk_team.h"
#include "sdk_gamerules.h"
#include "weapon_sdkbase.h"
#include "predicted_viewmodel.h"
#include "iservervehicle.h"
#include "viewport_panel_names.h"
#include "info_camera_link.h"
#include "GameStats.h"
#include "obstacle_pushaway.h"
#include "in_buttons.h"
#include "team.h"	//IOS
#include "fmtstr.h"	//IOS
#include "game.h" //IOS
#include "ios_bot.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern int gEvilImpulse101;

CUniformRandomStream g_IOSRand;

//#define SDK_PLAYER_MODEL "models/player/terror.mdl"
//#define SDK_PLAYER_MODEL "models/player/brazil/brazil.mdl"		//need to precache one to start with

#define PLAYER_SPEED 280.0f

#define SPRINT_TIME           6.0f     //IOS sprint amount 5.5
#define SPRINT_RECHARGE_TIME  12.0f    //IOS time before sprint re-charges
#define SPRINT_SPEED          90.0f    //IOS sprint increase in speed

ConVar SDK_ShowStateTransitions( "sdk_ShowStateTransitions", "-2", FCVAR_CHEAT, "sdk_ShowStateTransitions <ent index or -1 for all>. Show player state transitions." );


EHANDLE g_pLastDMSpawn;
#if defined ( SDK_USE_TEAMS )
EHANDLE g_pLastBlueSpawn;
EHANDLE g_pLastRedSpawn;
#endif
// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

					CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name )
					{
					}

	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nData ), 32 )
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
	CPVSFilter filter( (const Vector&)pPlayer->EyePosition() );

	//Tony; pull the player who is doing it out of the recipientlist, this is predicted!!
	//ios: this may come from the ball -> not predicted
	//filter.RemoveRecipient( pPlayer );

	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

// -------------------------------------------------------------------------------- //
// Tables.
// -------------------------------------------------------------------------------- //
BEGIN_DATADESC( CSDKPlayer )
DEFINE_THINKFUNC( SDKPushawayThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( player, CSDKPlayer );
PRECACHE_REGISTER(player);

// CSDKPlayerShared Data Tables
//=============================

// specific to the local player
BEGIN_SEND_TABLE_NOBASE( CSDKPlayerShared, DT_SDKSharedLocalPlayerExclusive )
#if defined ( SDK_USE_PLAYERCLASSES )
	SendPropInt( SENDINFO( m_iPlayerClass), 4 ),
	SendPropInt( SENDINFO( m_iDesiredPlayerClass ), 4 ),
#endif
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CSDKPlayerShared, DT_SDKPlayerShared )
#if defined ( SDK_USE_STAMINA ) || defined ( SDK_USE_SPRINTING )
	SendPropFloat( SENDINFO( m_flStamina ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#endif

#if defined ( SDK_USE_PRONE )
	SendPropBool( SENDINFO( m_bProne ) ),
	SendPropTime( SENDINFO( m_flGoProneTime ) ),
	SendPropTime( SENDINFO( m_flUnProneTime ) ),
#endif
#if defined ( SDK_USE_SPRINTING )
	SendPropBool( SENDINFO( m_bIsSprinting ) ),
#endif
	SendPropDataTable( "sdksharedlocaldata", 0, &REFERENCE_SEND_TABLE(DT_SDKSharedLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
END_SEND_TABLE()
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

BEGIN_SEND_TABLE_NOBASE( CSDKPlayer, DT_SDKLocalPlayerExclusive )
	SendPropInt( SENDINFO( m_iShotsFired ), 8, SPROP_UNSIGNED ),
	// send a hi-res origin to the local player for use in prediction
	//new ios1.1 we need this for free roaming mode - do not remove!
    SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	//new
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
//	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),

//ios	SendPropInt( SENDINFO( m_ArmorValue ), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CSDKPlayer, DT_SDKNonLocalPlayerExclusive )
	// send a lo-res origin to other players
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),
END_SEND_TABLE()


// main table
IMPLEMENT_SERVERCLASS_ST( CSDKPlayer, DT_SDKPlayer )
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseAnimating", "m_nNewSequenceParity" ),
	SendPropExclude( "DT_BaseAnimating", "m_nResetEventsParity" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	
	// playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	// Data that only gets sent to the local player.
	SendPropDataTable( SENDINFO_DT( m_Shared ), &REFERENCE_SEND_TABLE( DT_SDKPlayerShared ) ),

	// Data that only gets sent to the local player.
	SendPropDataTable( "sdklocaldata", 0, &REFERENCE_SEND_TABLE(DT_SDKLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
	// Data that gets sent to all other players
	SendPropDataTable( "sdknonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_SDKNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),

	SendPropEHandle( SENDINFO( m_hRagdoll ) ),

	SendPropInt( SENDINFO( m_iPlayerState ), Q_log2( NUM_PLAYER_STATES )+1, SPROP_UNSIGNED ),

	SendPropBool( SENDINFO( m_bSpawnInterpCounter ) ),

	SendPropInt(SENDINFO(m_TeamPos)),
	SendPropInt(SENDINFO(m_ShirtPos)),
END_SEND_TABLE()

class CSDKRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CSDKRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

public:
	// In case the client has the player entity, we transmit the player index.
	// In case the client doesn't have it, we transmit the player's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

LINK_ENTITY_TO_CLASS( sdk_ragdoll, CSDKRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CSDKRagdoll, DT_SDKRagdoll )
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) )
END_SEND_TABLE()


// -------------------------------------------------------------------------------- //

void cc_CreatePredictionError_f()
{
	CBaseEntity *pEnt = CBaseEntity::Instance( 1 );
	pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + Vector( 63, 0, 0 ) );
}

ConCommand cc_CreatePredictionError( "CreatePredictionError", cc_CreatePredictionError_f, "Create a prediction error", FCVAR_CHEAT );

void CSDKPlayer::SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize )
{
	BaseClass::SetupVisibility( pViewEntity, pvs, pvssize );

	int area = pViewEntity ? pViewEntity->NetworkProp()->AreaNum() : NetworkProp()->AreaNum();
	PointCameraSetupVisibility( this, area, pvs, pvssize );
}

CSDKPlayer::CSDKPlayer()
{
	//Tony; create our player animation state.
	m_PlayerAnimState = CreateSDKPlayerAnimState( this );
	m_iLastWeaponFireUsercmd = 0;
	
	m_Shared.Init( this );

	UseClientSideAnimation();

	m_angEyeAngles.Init();

	m_pCurStateInfo = NULL;	// no state yet

}


CSDKPlayer::~CSDKPlayer()
{
	DestroyRagdoll();
	m_PlayerAnimState->Release();
}


CSDKPlayer *CSDKPlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CSDKPlayer::s_PlayerEdict = ed;
	return (CSDKPlayer*)CreateEntityByName( className );
}

void CSDKPlayer::LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles )
{
	BaseClass::LeaveVehicle( vecExitPoint, vecExitAngles );

	//teleport physics shadow too
	// Vector newPos = GetAbsOrigin();
	// QAngle newAng = GetAbsAngles();

	// Teleport( &newPos, &newAng, &vec3_origin );
}

void CSDKPlayer::PreThink(void)
{
	State_PreThink();

	// Riding a vehicle?
	if ( IsInAVehicle() )	
	{
		// make sure we update the client, check for timed damage and update suit even if we are in a vehicle
		UpdateClientData();		
		CheckTimeBasedDamage();

		// Allow the suit to recharge when in the vehicle.
		CheckSuitUpdate();
		
		WaterMove();	
		return;
	}

	UpdateSprint();

	CheckRejoin();

	BaseClass::PreThink();
}


void CSDKPlayer::PostThink()
{
	BaseClass::PostThink();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );
	
	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

    m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );
	
	LookAtBall();

	//IOSSPlayerCollision();

	//m_BallInPenaltyBox = -1;
}

void CSDKPlayer::LookAtBall(void)
{
	//IOS LOOKAT Ball. Turn head to look at ball - headtracking.
	CBall *pBall = FindNearestBall();
	if (pBall) 
	{
		float curyaw = GetBoneController(2);
		//float curpitch = GetBoneController(3);

		float distSq = (pBall->GetAbsOrigin() - GetAbsOrigin()).Length2DSqr();
		if (distSq > 72.0f*72.0f)		//6 ft
		{
  			QAngle ball_angle;
			VectorAngles((pBall->GetAbsOrigin() - GetAbsOrigin()), ball_angle );

			float yaw = ball_angle.y - GetAbsAngles().y;
			//float pitch = ball_angle.x - GetAbsAngles().x;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;
			//if (pitch > 180) pitch -= 360;
			//if (pitch < -180) pitch += 360;

			if (yaw > -30 && yaw < 30) 
			{   
				yaw = curyaw + (yaw-curyaw)*0.1f;
				//pitch = curpitch + (pitch-curpitch)*0.1f;
				//only move head if ball is in forward view cone.
				SetBoneController(2, yaw);
				//IOSS 1.0a removed cos it vibrates SetBoneController(3, pitch);
			}
			else 
			{
				SetBoneController(2, curyaw * 0.9f );
				//IOSS 1.0a removed cos it vibrates SetBoneController(3, curpitch * 0.9f );
			}

			//stop keeper wobbly head when carrying
			//if (m_TeamPos == 1 && pBall->m_KeeperCarrying)
			//{
			//	SetBoneController(2, 0 );
			//	SetBoneController(3, 0 );
			//}
		}
		else
		{
			SetBoneController(2, curyaw * 0.9f );
			//IOSS 1.0a removed cos it vibrates SetBoneController(3, curpitch * 0.9f );
		}
	}
}

#define IOSSCOLDIST (36.0f * 36.0f)

void CSDKPlayer::IOSSPlayerCollision(void)
{
	static int start = 1;
	static int end = gpGlobals->maxClients;

	Vector posUs = GetAbsOrigin();
	Vector posThem;
	Vector diff;
	Vector norm;

	for (int i = start; i <= end; i++) 
	{
		CSDKPlayer *pPlayer = (CSDKPlayer *)UTIL_PlayerByIndex (i);

		if ( !pPlayer )
			continue;

		if (pPlayer==this)
			continue;

		posThem = pPlayer->GetAbsOrigin();

		diff = posUs - posThem;
		diff.z = 0.0f;

		norm = diff;
		norm.NormalizeInPlace();

		if (diff.Length2DSqr() < IOSSCOLDIST)
		{
			posUs += norm * 1.0f;
			SetAbsOrigin(posUs);
		}
	}
}


void CSDKPlayer::Precache()
{

	//Tony; go through our list of player models that we may be using and cache them
	/* ios
	int i = 0;
	while( pszPossiblePlayerModels[i] != NULL )
	{
		PrecacheModel( pszPossiblePlayerModels[i] );
		i++;
	}	
	*/

	//PrecacheModel( SDK_PLAYER_MODEL );

	//precache every model?
	PrecacheModel("models/player/brazil/brazil.mdl");
	//PrecacheModel("models/player/england/england.mdl");

	PrecacheScriptSound("Player.Oomph");
	PrecacheScriptSound("Player.DiveKeeper");
	PrecacheScriptSound("Player.Save");
	PrecacheScriptSound("Player.Goal");
	PrecacheScriptSound("Player.Slide");
	PrecacheScriptSound("Player.DivingHeader");
	PrecacheScriptSound("Player.Card");

	BaseClass::Precache();
}

//Tony; this is where default items go when not using playerclasses!
void CSDKPlayer::GiveDefaultItems()
{
#if !defined ( SDK_USE_PLAYERCLASSES )
	if ( State_Get() == STATE_ACTIVE )
	{
		CBasePlayer::GiveAmmo( 30,	"pistol");
		CBasePlayer::GiveAmmo( 30,	"mp5");
		CBasePlayer::GiveAmmo( 12,	"shotgun");
		CBasePlayer::GiveAmmo( 5,	"grenades" );

		GiveNamedItem( "weapon_pistol" );
		GiveNamedItem( "weapon_mp5" );
		GiveNamedItem( "weapon_shotgun" );
		GiveNamedItem( "weapon_crowbar" );
		GiveNamedItem( "weapon_grenade" );
	}
#endif
}
#define SDK_PUSHAWAY_THINK_CONTEXT	"SDKPushawayThink"
void CSDKPlayer::SDKPushawayThink(void)
{
	// Push physics props out of our way.
	PerformObstaclePushaway( this );
	SetNextThink( gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, SDK_PUSHAWAY_THINK_CONTEXT );
}

void CSDKPlayer::Spawn()
{
	/* ios
	SetModel( SDK_PLAYER_MODEL );	//Tony; basically, leave this alone ;) unless you're not using classes or teams, then you can change it to whatever.
	
	SetBloodColor( BLOOD_COLOR_RED );
	
	SetMoveType( MOVETYPE_WALK );
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	//Tony; if we're spawning in active state, equip the suit so the hud works. -- Gotta love base code !
	if ( State_Get() == STATE_ACTIVE )
	{
		EquipSuit( false );
//Tony; bleh, don't do this here.
//		GiveDefaultItems();
	}

	m_hRagdoll = NULL;
	
	BaseClass::Spawn();
#if defined ( SDK_USE_STAMINA ) || defined ( SDK_USE_SPRINTING )
	m_Shared.SetStamina( 100 );
#endif

#if defined ( SDK_USE_TEAMS )
	m_bTeamChanged	= false;
#endif

#if defined ( SDK_USE_PRONE )
	InitProne();
#endif

#if defined ( SDK_USE_SPRINTING )
	InitSprinting();
#endif

	// update this counter, used to not interp players when they spawn
	m_bSpawnInterpCounter = !m_bSpawnInterpCounter;

	InitSpeeds(); //Tony; initialize player speeds.

	SetArmorValue(SpawnArmorValue());

	SetContextThink( &CSDKPlayer::SDKPushawayThink, gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, SDK_PUSHAWAY_THINK_CONTEXT );
	pl.deadflag = false;
	*/

	
	if (!GetModelPtr())
		SetModel( SDK_PLAYER_MODEL );		//only do it first time through, then let the team stuff set it.

	SetMoveType( MOVETYPE_WALK );
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	//SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	m_hRagdoll = NULL;
	
	BaseClass::Spawn();
	//ios
	SetMaxSpeed(PLAYER_SPEED);

	m_NextShoot = 0;
	m_KickDelay = 0;

	//SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );

	//ios hide/nocollision/freeze players on menu
	AddEffects( EF_NODRAW );
	SetSolid( SOLID_NONE );
	AddFlag(FL_FROZEN);

	AddFlag(FL_ONGROUND);		//doesn't seem to get start on initial join?

	//// Select the scanner's idle sequence
	//SetSequence( LookupSequence("iospass") );
	//// Set the animation speed to 100%
	//SetPlaybackRate( 1.0f );
	//// Tell the client to animate this model
	//UseClientSideAnimation();
}

/* ios
bool CSDKPlayer::SelectSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot )
{
	// Find the next spawn spot.
	pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

	if ( pSpot == NULL ) // skip over the null point
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

	CBaseEntity *pFirstSpot = pSpot;
	do 
	{
		if ( pSpot )
		{
			// check if pSpot is valid
			if ( g_pGameRules->IsSpawnPointValid( pSpot, this ) )
			{
				if ( pSpot->GetAbsOrigin() == Vector( 0, 0, 0 ) )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
					continue;
				}

				// if so, go to pSpot
				return true;
			}
		}
		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

	DevMsg("CSDKPlayer::SelectSpawnSpot: couldn't find valid spawn point.\n");

	return true;
}
*/
/* ios
CBaseEntity* CSDKPlayer::EntSelectSpawnPoint()
{
	CBaseEntity *pSpot = NULL;

	const char *pSpawnPointName = "";

	switch( GetTeamNumber() )
	{
#if defined ( SDK_USE_TEAMS )
	case SDK_TEAM_BLUE:
		{
			pSpawnPointName = "info_player_blue";
			pSpot = g_pLastBlueSpawn;
			if ( SelectSpawnSpot( pSpawnPointName, pSpot ) )
			{
				g_pLastBlueSpawn = pSpot;
			}
		}
		break;
	case SDK_TEAM_RED:
		{
			pSpawnPointName = "info_player_red";
			pSpot = g_pLastRedSpawn;
			if ( SelectSpawnSpot( pSpawnPointName, pSpot ) )
			{
				g_pLastRedSpawn = pSpot;
			}
		}		
		break;
#endif // SDK_USE_TEAMS
	case TEAM_UNASSIGNED:
		{
			pSpawnPointName = "info_player_deathmatch";
			pSpot = g_pLastDMSpawn;
			if ( SelectSpawnSpot( pSpawnPointName, pSpot ) )
			{
				g_pLastDMSpawn = pSpot;
			}
		}		
		break;
	case TEAM_SPECTATOR:
	default:
		{
			pSpot = CBaseEntity::Instance( INDEXENT(0) );
		}
		break;		
	}

	if ( !pSpot )
	{
		Warning( "PutClientInServer: no %s on level\n", pSpawnPointName );
		return CBaseEntity::Instance( INDEXENT(0) );
	}

	return pSpot;
} 
*/
//-----------------------------------------------------------------------------
// Purpose: Put the player in the specified team
//-----------------------------------------------------------------------------
//Tony; if we're not using actual teams, we don't need to override this.
#if defined ( SDK_USE_TEAMS )
void CSDKPlayer::ChangeTeam( int iTeamNum )
{
	if ( !GetGlobalTeam( iTeamNum ) )
	{
		Warning( "CSDKPlayer::ChangeTeam( %d ) - invalid team index.\n", iTeamNum );
		return;
	}

	int iOldTeam = GetTeamNumber();

	// if this is our current team, just abort
	if ( iTeamNum == iOldTeam )
		return;
	
	m_bTeamChanged = true;

	// do the team change:
	BaseClass::ChangeTeam( iTeamNum );

	// update client state 
	if ( iTeamNum == TEAM_UNASSIGNED )
	{
		State_Transition( STATE_OBSERVER_MODE );
	}
	else if ( iTeamNum == TEAM_SPECTATOR )
	{
		RemoveAllItems( true );
		
		State_Transition( STATE_OBSERVER_MODE );
	}
	else // active player
	{
		if ( !IsDead() )
		{
			// Kill player if switching teams while alive
			CommitSuicide();

			// add 1 to frags to balance out the 1 subtracted for killing yourself
			IncrementFragCount( 1 );
		}

		if( iOldTeam == TEAM_SPECTATOR )
			SetMoveType( MOVETYPE_NONE );
//Tony; pop up the class menu if we're using classes, otherwise just spawn.
#if defined ( SDK_USE_PLAYERCLASSES )
		// Put up the class selection menu.
		State_Transition( STATE_PICKINGCLASS );
#else
		State_Transition( STATE_ACTIVE );
#endif
	}
}

#endif // SDK_USE_TEAMS

/*ios
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKPlayer::CommitSuicide( bool bExplode = false, bool bForce false )
void CSDKPlayer::CommitSuicide( bool bExplode, bool bForce )
{
	// Don't suicide if we haven't picked a class for the first time, or we're not in active state
	if (
#if defined ( SDK_USE_PLAYERCLASSES )
		m_Shared.PlayerClass() == PLAYERCLASS_UNDEFINED || 
#endif
		State_Get() != STATE_ACTIVE 
		)
		return;
	
	m_iSuicideCustomKillFlags = SDK_DMG_CUSTOM_SUICIDE;

	BaseClass::CommitSuicide( bExplode, bForce );
}
*/
void CSDKPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();

	//ios State_Enter( STATE_WELCOME );


	//const ConVar *hostname = cvar->FindVar( "hostname" );
	//const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

	// open info panel on client showing MOTD:
	KeyValues *data = new KeyValues("data");
	//data->SetString( "title", title );		// info panel title
	//data->SetString( "type", "1" );			// show userdata from stringtable entry
	//data->SetString( "msg",	"motd" );		// use this stringtable entry
	//data->SetString( "cmd", "impulse 101" );// exec this command if panel closed
	data->SetString("team1", GetGlobalTeam( TEAM_A )->GetName());
	data->SetString("team2", GetGlobalTeam( TEAM_B )->GetName());

	//ShowViewPortPanel( PANEL_INFO, true, data );
	ShowViewPortPanel( PANEL_TEAM, true, data );		//ios - send team names in data?

	m_RejoinTime = 0;
	m_RedCards=0;
	m_YellowCards=0;
	m_Fouls=0;
	m_Assists=0;
	m_Passes=0;
	m_FreeKicks=0;
	m_Penalties=0;
	m_Corners=0;
	m_ThrowIns=0;
	m_KeeperSaves=0;
	m_GoalKicks=0;
	m_Possession=0;
	m_fPossessionTime=0.0f;
	ResetFragCount();

	m_fSprintLeft = SPRINT_TIME;
	m_fSprintIdle = 0.0f;

	m_NextShoot = 0;
	m_KickDelay = 0;

	data->deleteThis();
}
void CSDKPlayer::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	//Tony; disable prediction filtering, and call the baseclass.
	CDisablePredictionFiltering disabler;
	BaseClass::TraceAttack( inputInfo, vecDir, ptr );
}
int CSDKPlayer::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	CBaseEntity *pInflictor = info.GetInflictor();

	if ( !pInflictor )
		return 0;

	if ( GetMoveType() == MOVETYPE_NOCLIP || GetMoveType() == MOVETYPE_OBSERVER )
		return 0;

	float flArmorBonus = 0.5f;
	float flArmorRatio = 0.5f;
	float flDamage = info.GetDamage();

	bool bCheckFriendlyFire = false;
	bool bFriendlyFire = friendlyfire.GetBool();
	//Tony; only check teams in teamplay
	if ( gpGlobals->teamplay )
		bCheckFriendlyFire = true;

	if ( bFriendlyFire || ( bCheckFriendlyFire && pInflictor->GetTeamNumber() != GetTeamNumber() ) || pInflictor == this ||	info.GetAttacker() == this )
	{
		if ( bFriendlyFire && (info.GetDamageType() & DMG_BLAST) == 0 )
		{
			if ( pInflictor->GetTeamNumber() == GetTeamNumber() && bCheckFriendlyFire)
			{
				flDamage *= 0.35; // bullets hurt teammates less
			}
		}

		// keep track of amount of damage last sustained
		m_lastDamageAmount = flDamage;
		// Deal with Armour
		if ( ArmorValue() && !( info.GetDamageType() & (DMG_FALL | DMG_DROWN)) )
		{
			float flNew = flDamage * flArmorRatio;
			float flArmor = (flDamage - flNew) * flArmorBonus;

			// Does this use more armor than we have?
			if (flArmor > ArmorValue() )
			{
				//armorHit = (int)(flArmor);

				flArmor = ArmorValue();
				flArmor *= (1/flArmorBonus);
				flNew = flDamage - flArmor;
				SetArmorValue( 0 );
			}
			else
			{
				int oldValue = (int)(ArmorValue());
			
				if ( flArmor < 0 )
					 flArmor = 1;

				SetArmorValue( oldValue - flArmor );
				//armorHit = oldValue - (int)(pev->armorvalue);
			}
			
			flDamage = flNew;
			
			info.SetDamage( flDamage );
		}

		// round damage to integer
		info.SetDamage( (int)flDamage );

		if ( info.GetDamage() <= 0 )
			return 0;

		CSingleUserRecipientFilter user( this );
		user.MakeReliable();
		UserMessageBegin( user, "Damage" );
			WRITE_BYTE( (int)info.GetDamage() );
			WRITE_VEC3COORD( info.GetInflictor()->WorldSpaceCenter() );
		MessageEnd();

		// Do special explosion damage effect
		if ( info.GetDamageType() & DMG_BLAST )
		{
			OnDamagedByExplosion( info );
		}

		gamestats->Event_PlayerDamage( this, info );

		return CBaseCombatCharacter::OnTakeDamage( info );
	}
	else
	{
		return 0;
	}
}

int CSDKPlayer::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// set damage type sustained
	m_bitsDamageType |= info.GetDamageType();

	if ( !CBaseCombatCharacter::OnTakeDamage_Alive( info ) )
		return 0;

	// fire global game event

	IGameEvent * event = gameeventmanager->CreateEvent( "player_hurt" );

	if ( event )
	{
		event->SetInt("userid", GetUserID() );
		event->SetInt("health", max(0, m_iHealth) );
		event->SetInt("armor", max(0, ArmorValue()) );

		if ( info.GetDamageType() & DMG_BLAST )
		{
			event->SetInt( "hitgroup", HITGROUP_GENERIC );
		}
		else
		{
			event->SetInt( "hitgroup", LastHitGroup() );
		}

		CBaseEntity * attacker = info.GetAttacker();
		const char *weaponName = "";

		if ( attacker->IsPlayer() )
		{
			CBasePlayer *player = ToBasePlayer( attacker );
			event->SetInt("attacker", player->GetUserID() ); // hurt by other player

			CBaseEntity *pInflictor = info.GetInflictor();
			if ( pInflictor )
			{
				if ( pInflictor == player )
				{
					// If the inflictor is the killer,  then it must be their current weapon doing the damage
					if ( player->GetActiveWeapon() )
					{
						weaponName = player->GetActiveWeapon()->GetClassname();
					}
				}
				else
				{
					weaponName = STRING( pInflictor->m_iClassname );  // it's just that easy
				}
			}
		}
		else
		{
			event->SetInt("attacker", 0 ); // hurt by "world"
		}

		if ( strncmp( weaponName, "weapon_", 7 ) == 0 )
		{
			weaponName += 7;
		}
		else if( strncmp( weaponName, "grenade", 9 ) == 0 )	//"grenade_projectile"	
		{
			weaponName = "grenade";
		}

		event->SetString( "weapon", weaponName );
		event->SetInt( "priority", 5 );

		gameeventmanager->FireEvent( event );
	}
	
	return 1;
}

void CSDKPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	ThrowActiveWeapon();

	// show killer in death cam mode
	// chopped down version of SetObserverTarget without the team check
	if( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
	{
		// set new target
		m_hObserverTarget.Set( info.GetAttacker() ); 

		// reset fov to default
		SetFOV( this, 0 );
	}
	else
		m_hObserverTarget.Set( NULL );

	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.
	//ios CreateRagdollEntity();

	State_Transition( STATE_DEATH_ANIM );	// Transition into the dying state.

	//Tony; after transition, remove remaining items
	RemoveAllItems( true );

	BaseClass::Event_Killed( info );

}
void CSDKPlayer::ThrowActiveWeapon( void )
{
	CWeaponSDKBase *pWeapon = (CWeaponSDKBase *)GetActiveWeapon();

	if( pWeapon && pWeapon->CanWeaponBeDropped() )
	{
		QAngle gunAngles;
		VectorAngles( BodyDirection2D(), gunAngles );

		Vector vecForward;
		AngleVectors( gunAngles, &vecForward, NULL, NULL );

		float flDiameter = sqrt( CollisionProp()->OBBSize().x * CollisionProp()->OBBSize().x + CollisionProp()->OBBSize().y * CollisionProp()->OBBSize().y );

		pWeapon->Holster(NULL);
		SwitchToNextBestWeapon( pWeapon );
		SDKThrowWeapon( pWeapon, vecForward, gunAngles, flDiameter );
	}
}
void CSDKPlayer::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );
	dynamic_cast<CWeaponSDKBase*>(pWeapon)->SetDieThink( false );	//Make sure the context think for removing is gone!!

}
void CSDKPlayer::SDKThrowWeapon( CWeaponSDKBase *pWeapon, const Vector &vecForward, const QAngle &vecAngles, float flDiameter  )
{
	Vector vecOrigin;
	CollisionProp()->RandomPointInBounds( Vector( 0.5f, 0.5f, 0.5f ), Vector( 0.5f, 0.5f, 1.0f ), &vecOrigin );

	// Nowhere in particular; just drop it.
	Vector vecThrow;
	SDKThrowWeaponDir( pWeapon, vecForward, &vecThrow );

	Vector vecOffsetOrigin;
	VectorMA( vecOrigin, flDiameter, vecThrow, vecOffsetOrigin );

	trace_t	tr;
	UTIL_TraceLine( vecOrigin, vecOffsetOrigin, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		
	if ( tr.startsolid || tr.allsolid || ( tr.fraction < 1.0f && tr.m_pEnt != pWeapon ) )
	{
		//FIXME: Throw towards a known safe spot?
		vecThrow.Negate();
		VectorMA( vecOrigin, flDiameter, vecThrow, vecOffsetOrigin );
	}

	vecThrow *= random->RandomFloat( 150.0f, 240.0f );

	pWeapon->SetAbsOrigin( vecOrigin );
	pWeapon->SetAbsAngles( vecAngles );
	pWeapon->Drop( vecThrow );
	pWeapon->SetRemoveable( false );
	Weapon_Detach( pWeapon );

	pWeapon->SetDieThink( true );
}

void CSDKPlayer::SDKThrowWeaponDir( CWeaponSDKBase *pWeapon, const Vector &vecForward, Vector *pVecThrowDir )
{
	VMatrix zRot;
	MatrixBuildRotateZ( zRot, random->RandomFloat( -60.0f, 60.0f ) );

	Vector vecThrow;
	Vector3DMultiply( zRot, vecForward, *pVecThrowDir );

	pVecThrowDir->z = random->RandomFloat( -0.5f, 0.5f );
	VectorNormalize( *pVecThrowDir );
}

void CSDKPlayer::PlayerDeathThink()
{
	//overridden, do nothing - our states handle this now
}
void CSDKPlayer::CreateRagdollEntity()
{
	// If we already have a ragdoll, don't make another one.
	CSDKRagdoll *pRagdoll = dynamic_cast< CSDKRagdoll* >( m_hRagdoll.Get() );

	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
		pRagdoll = NULL;
	}
	Assert( pRagdoll == NULL );

	if ( !pRagdoll )
	{
		// create a new one
		pRagdoll = dynamic_cast< CSDKRagdoll* >( CreateEntityByName( "sdk_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = Vector(0,0,0);
	}

	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;
}
//-----------------------------------------------------------------------------
// Purpose: Destroy's a ragdoll, called when a player is disconnecting.
//-----------------------------------------------------------------------------
void CSDKPlayer::DestroyRagdoll( void )
{
	CSDKRagdoll *pRagdoll = dynamic_cast<CSDKRagdoll*>( m_hRagdoll.Get() );	
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
	}
}

void CSDKPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	m_PlayerAnimState->DoAnimationEvent( event, nData );
	TE_PlayerAnimEvent( this, event, nData );	// Send to any clients who can see this guy.
}

CWeaponSDKBase* CSDKPlayer::GetActiveSDKWeapon() const
{
	return dynamic_cast< CWeaponSDKBase* >( GetActiveWeapon() );
}

void CSDKPlayer::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CPredictedViewModel *vm = ( CPredictedViewModel * )CreateEntityByName( "predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

void CSDKPlayer::CheatImpulseCommands( int iImpulse )
{
	return; //ios

	if ( !sv_cheats->GetBool() )
	{
		return;
	}

	if ( iImpulse != 101 )
	{
		BaseClass::CheatImpulseCommands( iImpulse );
		return ;
	}
	gEvilImpulse101 = true;

	EquipSuit();
	
	if ( GetHealth() < 100 )
	{
		TakeHealth( 25, DMG_GENERIC );
	}

	gEvilImpulse101		= false;
}


void CSDKPlayer::FlashlightTurnOn( void )
{
	//ios AddEffects( EF_DIMLIGHT );
}

void CSDKPlayer::FlashlightTurnOff( void )
{
	//ios RemoveEffects( EF_DIMLIGHT );
}

int CSDKPlayer::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}
/* ios
bool CSDKPlayer::ClientCommand( const CCommand &args )
{
	const char *pcmd = args[0];
	if ( FStrEq( pcmd, "jointeam" ) ) 
	{
		if ( args.ArgC() < 2 )
		{
			Warning( "Player sent bad jointeam syntax\n" );
		}

		int iTeam = atoi( args[1] );
		HandleCommand_JoinTeam( iTeam );
		return true;
	}
	else if( !Q_strncmp( pcmd, "cls_", 4 ) )
	{
#if defined ( SDK_USE_PLAYERCLASSES )
		CSDKTeam *pTeam = GetGlobalSDKTeam( GetTeamNumber() );

		Assert( pTeam );

		int iClassIndex = PLAYERCLASS_UNDEFINED;

		if( pTeam->IsClassOnTeam( pcmd, iClassIndex ) )
		{
			HandleCommand_JoinClass( iClassIndex );
		}
		else
		{
			DevMsg( "player tried to join a class that isn't on this team ( %s )\n", pcmd );
			ShowClassSelectMenu();
		}
#endif
		return true;
	}
	else if ( FStrEq( pcmd, "spectate" ) )
	{
		// instantly join spectators
		HandleCommand_JoinTeam( TEAM_SPECTATOR );
		return true;
	}
	else if ( FStrEq( pcmd, "joingame" ) )
	{
		// player just closed MOTD dialog
		if ( m_iPlayerState == STATE_WELCOME )
		{
//Tony; using teams, go to picking team.
#if defined( SDK_USE_TEAMS )
			State_Transition( STATE_PICKINGTEAM );
//Tony; not using teams, but we are using classes, so go straight to class picking.
#elif !defined ( SDK_USE_TEAMS ) && defined ( SDK_USE_PLAYERCLASSES )
			State_Transition( STATE_PICKINGCLASS );
//Tony; not using teams or classes, go straight to active.
#else
			State_Transition( STATE_ACTIVE );
#endif
		}
		
		return true;
	}
	else if ( FStrEq( pcmd, "joinclass" ) ) 
	{
#if defined ( SDK_USE_PLAYERCLASSES )
		if ( args.ArgC() < 2 )
		{
			Warning( "Player sent bad joinclass syntax\n" );
		}

		int iClass = atoi( args[1] );
		HandleCommand_JoinClass( iClass );
#endif
		return true;
	}
	else if ( FStrEq( pcmd, "menuopen" ) )
	{
#if defined ( SDK_USE_PLAYERCLASSES )
		SetClassMenuOpen( true );
#endif
		return true;
	}
	else if ( FStrEq( pcmd, "menuclosed" ) )
	{
#if defined ( SDK_USE_PLAYERCLASSES )
		SetClassMenuOpen( false );
#endif
		return true;
	}
	else if ( FStrEq( pcmd, "droptest" ) )
	{
		ThrowActiveWeapon();
		return true;
	}

	return BaseClass::ClientCommand( args );
}
*/
// returns true if the selection has been handled and the player's menu 
// can be closed...false if the menu should be displayed again
/*
bool CSDKPlayer::HandleCommand_JoinTeam( int team )
{
	CSDKGameRules *mp = SDKGameRules();
	int iOldTeam = GetTeamNumber();
	if ( !GetGlobalTeam( team ) )
	{
		Warning( "HandleCommand_JoinTeam( %d ) - invalid team index.\n", team );
		return false;
	}

#if defined ( SDK_USE_TEAMS )
	// If we already died and changed teams once, deny
	if( m_bTeamChanged && team != TEAM_SPECTATOR && iOldTeam != TEAM_SPECTATOR )
	{
		ClientPrint( this, HUD_PRINTCENTER, "game_switch_teams_once" );
		return true;
	}
#endif
	if ( team == TEAM_UNASSIGNED )
	{
		// Attempt to auto-select a team, may set team to T, CT or SPEC
		team = mp->SelectDefaultTeam();

		if ( team == TEAM_UNASSIGNED )
		{
			// still team unassigned, try to kick a bot if possible	
			 
			ClientPrint( this, HUD_PRINTTALK, "#All_Teams_Full" );

			team = TEAM_SPECTATOR;
		}
	}

	if ( team == iOldTeam )
		return true;	// we wouldn't change the team

#if defined ( SDK_USE_TEAMS )
	if ( mp->TeamFull( team ) )
	{
		if ( team == SDK_TEAM_BLUE )
		{
			ClientPrint( this, HUD_PRINTTALK, "#BlueTeam_Full" );
		}
		else if ( team == SDK_TEAM_RED )
		{
			ClientPrint( this, HUD_PRINTTALK, "#RedTeam_Full" );
		}
		ShowViewPortPanel( PANEL_TEAM );
		return false;
	}
#endif

	if ( team == TEAM_SPECTATOR )
	{
		// Prevent this if the cvar is set
		if ( !mp_allowspectators.GetInt() && !IsHLTV() )
		{
			ClientPrint( this, HUD_PRINTTALK, "#Cannot_Be_Spectator" );
			ShowViewPortPanel( PANEL_TEAM );
			return false;
		}

		ChangeTeam( TEAM_SPECTATOR );

		return true;
	}
	
	// If the code gets this far, the team is not TEAM_UNASSIGNED

	// Player is switching to a new team (It is possible to switch to the
	// same team just to choose a new appearance)
#if defined ( SDK_USE_TEAMS )
	if (mp->TeamStacked( team, GetTeamNumber() ))//players are allowed to change to their own team so they can just change their model
	{
		// The specified team is full
		ClientPrint( 
			this,
			HUD_PRINTCENTER,
			( team == SDK_TEAM_BLUE ) ?	"#BlueTeam_full" : "#RedTeam_full" );

		ShowViewPortPanel( PANEL_TEAM );
		return false;
	}
#endif
	// Switch their actual team...
	ChangeTeam( team );

#if defined ( SDK_USE_PLAYERCLASSES )
	// Force them to choose a new class
	m_Shared.SetDesiredPlayerClass( PLAYERCLASS_UNDEFINED );
	m_Shared.SetPlayerClass( PLAYERCLASS_UNDEFINED );
#endif
	return true;
}

#if defined ( SDK_USE_PLAYERCLASSES )
//Tony; we don't have to check anything special for SDK_USE_TEAMS here; it's all pretty generic, except for the one assert.
bool CSDKPlayer::HandleCommand_JoinClass( int iClass )
{
	Assert( GetTeamNumber() != TEAM_SPECTATOR );
#if defined ( SDK_USE_TEAMS )
	Assert( GetTeamNumber() != TEAM_UNASSIGNED );
#endif

	if( GetTeamNumber() == TEAM_SPECTATOR )
		return false;

	if( iClass == PLAYERCLASS_UNDEFINED )
		return false;	//they typed in something weird

	int iOldPlayerClass = m_Shared.DesiredPlayerClass();

	// See if we're joining the class we already are
	if( iClass == iOldPlayerClass )
		return true;

	if( !SDKGameRules()->IsPlayerClassOnTeam( iClass, GetTeamNumber() ) )
		return false;

	const char *classname = SDKGameRules()->GetPlayerClassName( iClass, GetTeamNumber() );

	if( SDKGameRules()->CanPlayerJoinClass( this, iClass ) )
	{
		m_Shared.SetDesiredPlayerClass( iClass );	//real class value is set when the player spawns

//Tony; don't do this until we have a spawn timer!!
//		if( State_Get() == STATE_PICKINGCLASS )
//			State_Transition( STATE_OBSERVER_MODE );

		if( iClass == PLAYERCLASS_RANDOM )
		{
			if( IsAlive() )
			{
				ClientPrint(this, HUD_PRINTTALK, "#game_respawn_asrandom" );
			}
			else
			{
				ClientPrint(this, HUD_PRINTTALK, "#game_spawn_asrandom" );
			}
		}
		else
		{
			if( IsAlive() )
			{
				ClientPrint(this, HUD_PRINTTALK, "#game_respawn_as", classname );
			}
			else
			{
				ClientPrint(this, HUD_PRINTTALK, "#game_spawn_as", classname );
			}
		}

		IGameEvent * event = gameeventmanager->CreateEvent( "player_changeclass" );
		if ( event )
		{
			event->SetInt( "userid", GetUserID() );
			event->SetInt( "class", iClass );

			gameeventmanager->FireEvent( event );
		}
	}
	else
	{
		ClientPrint(this, HUD_PRINTTALK, "#game_class_limit", classname );
		ShowClassSelectMenu();
	}

	// Incase we don't get the class menu message before the spawn timer
	// comes up, fake that we've closed the menu.
	SetClassMenuOpen( false );

	//Tony; TODO; this is temp, I may integrate with the teamplayroundrules; If I do, there will be wavespawn too.
	//if ( State_Get() == STATE_PICKINGCLASS || IsDead() )	//Tony; undone, don't transition if dead; only go into active state at this point if we were picking class.
	if ( State_Get() == STATE_PICKINGCLASS )
		State_Transition( STATE_ACTIVE ); //Done picking stuff and we're in the pickingclass state, or dead, so we can spawn now.

	return true;
}

void CSDKPlayer::ShowClassSelectMenu()
{
#if defined ( SDK_USE_TEAMS )
	if ( GetTeamNumber() == SDK_TEAM_BLUE )
	{
		ShowViewPortPanel( PANEL_CLASS_BLUE );
	}
	else if ( GetTeamNumber() == SDK_TEAM_RED	)
	{
		ShowViewPortPanel( PANEL_CLASS_RED );
	}

#else
	if ( GetTeamNumber() == TEAM_UNASSIGNED )
		ShowViewPortPanel( PANEL_CLASS_NOTEAMS );
#endif
}
void CSDKPlayer::SetClassMenuOpen( bool bOpen )
{
	m_bIsClassMenuOpen = bOpen;
}

bool CSDKPlayer::IsClassMenuOpen( void )
{
	return m_bIsClassMenuOpen;
}
#endif // SDK_USE_PLAYERCLASSES
*/
#if defined ( SDK_USE_PRONE )
//-----------------------------------------------------------------------------
// Purpose: Initialize prone at spawn.
//-----------------------------------------------------------------------------
void CSDKPlayer::InitProne( void )
{
	m_Shared.SetProne( false, true );
	m_bUnProneToDuck = false;
}
#endif // SDK_USE_PRONE

#if defined ( SDK_USE_SPRINTING )
void CSDKPlayer::InitSprinting( void )
{
	m_Shared.SetSprinting( false );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not we are allowed to sprint now.
//-----------------------------------------------------------------------------
bool CSDKPlayer::CanSprint()
{
	return ( 
		//!IsWalking() &&									// Not if we're walking
		!( m_Local.m_bDucked && !m_Local.m_bDucking ) &&	// Nor if we're ducking
		(GetWaterLevel() != 3) );							// Certainly not underwater
}
#endif // SDK_USE_SPRINTING
// ------------------------------------------------------------------------------------------------ //
// Player state management.
// ------------------------------------------------------------------------------------------------ //

void CSDKPlayer::State_Transition( SDKPlayerState newState )
{
	State_Leave();
	State_Enter( newState );
}


void CSDKPlayer::State_Enter( SDKPlayerState newState )
{
	m_iPlayerState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	if ( SDK_ShowStateTransitions.GetInt() == -1 || SDK_ShowStateTransitions.GetInt() == entindex() )
	{
		if ( m_pCurStateInfo )
			Msg( "ShowStateTransitions: entering '%s'\n", m_pCurStateInfo->m_pStateName );
		else
			Msg( "ShowStateTransitions: entering #%d\n", newState );
	}
	
	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
		(this->*m_pCurStateInfo->pfnEnterState)();
}


void CSDKPlayer::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}


void CSDKPlayer::State_PreThink()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnPreThink )
	{
		(this->*m_pCurStateInfo->pfnPreThink)();
	}
}


CSDKPlayerStateInfo* CSDKPlayer::State_LookupInfo( SDKPlayerState state )
{
	// This table MUST match the 
	static CSDKPlayerStateInfo playerStateInfos[] =
	{
		{ STATE_ACTIVE,			"STATE_ACTIVE",			&CSDKPlayer::State_Enter_ACTIVE, NULL, &CSDKPlayer::State_PreThink_ACTIVE },
		{ STATE_WELCOME,		"STATE_WELCOME",		&CSDKPlayer::State_Enter_WELCOME, NULL, &CSDKPlayer::State_PreThink_WELCOME },
#if defined ( SDK_USE_TEAMS )
		{ STATE_PICKINGTEAM,	"STATE_PICKINGTEAM",	&CSDKPlayer::State_Enter_PICKINGTEAM, NULL,	&CSDKPlayer::State_PreThink_WELCOME },
#endif
#if defined ( SDK_USE_PLAYERCLASSES )
		{ STATE_PICKINGCLASS,	"STATE_PICKINGCLASS",	&CSDKPlayer::State_Enter_PICKINGCLASS, NULL,	&CSDKPlayer::State_PreThink_WELCOME },
#endif
		{ STATE_DEATH_ANIM,		"STATE_DEATH_ANIM",		&CSDKPlayer::State_Enter_DEATH_ANIM,	NULL, &CSDKPlayer::State_PreThink_DEATH_ANIM },
		{ STATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CSDKPlayer::State_Enter_OBSERVER_MODE,	NULL, &CSDKPlayer::State_PreThink_OBSERVER_MODE }
	};

	for ( int i=0; i < ARRAYSIZE( playerStateInfos ); i++ )
	{
		if ( playerStateInfos[i].m_iPlayerState == state )
			return &playerStateInfos[i];
	}

	return NULL;
}
void CSDKPlayer::PhysObjectSleep()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Sleep();
}


void CSDKPlayer::PhysObjectWake()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Wake();
}
void CSDKPlayer::State_Enter_WELCOME()
{
	// Important to set MOVETYPE_NONE or our physics object will fall while we're sitting at one of the intro cameras.
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );

	PhysObjectSleep();

	// Show info panel
	if ( IsBot() )
	{
		// If they want to auto join a team for debugging, pretend they clicked the button.
		CCommand args;
		args.Tokenize( "joingame" );
		ClientCommand( args );
	}
	else
	{
		const ConVar *hostname = cvar->FindVar( "hostname" );
		const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

		// open info panel on client showing MOTD:
		KeyValues *data = new KeyValues("data");
		data->SetString( "title", title );		// info panel title
		data->SetString( "type", "1" );			// show userdata from stringtable entry
		data->SetString( "msg",	"motd" );		// use this stringtable entry
		data->SetString( "cmd", "joingame" );// exec this command if panel closed

		ShowViewPortPanel( PANEL_INFO, true, data );

		data->deleteThis();
	}	
}

void CSDKPlayer::MoveToNextIntroCamera()
{
	m_pIntroCamera = gEntList.FindEntityByClassname( m_pIntroCamera, "point_viewcontrol" );

	// if m_pIntroCamera is NULL we just were at end of list, start searching from start again
	if(!m_pIntroCamera)
		m_pIntroCamera = gEntList.FindEntityByClassname(m_pIntroCamera, "point_viewcontrol");

	// find the target
	CBaseEntity *Target = NULL;
	
	if( m_pIntroCamera )
	{
		Target = gEntList.FindEntityByName( NULL, STRING(m_pIntroCamera->m_target) );
	}

	// if we still couldn't find a camera, goto T spawn
	if(!m_pIntroCamera)
		m_pIntroCamera = gEntList.FindEntityByClassname(m_pIntroCamera, "info_player_terrorist");

	SetViewOffset( vec3_origin );	// no view offset
	UTIL_SetSize( this, vec3_origin, vec3_origin ); // no bbox

	if( !Target ) //if there are no cameras(or the camera has no target, find a spawn point and black out the screen
	{
		if ( m_pIntroCamera.IsValid() )
			SetAbsOrigin( m_pIntroCamera->GetAbsOrigin() + VEC_VIEW );

		SetAbsAngles( QAngle( 0, 0, 0 ) );
		
		m_pIntroCamera = NULL;  // never update again
		return;
	}
	

	Vector vCamera = Target->GetAbsOrigin() - m_pIntroCamera->GetAbsOrigin();
	Vector vIntroCamera = m_pIntroCamera->GetAbsOrigin();
	
	VectorNormalize( vCamera );
		
	QAngle CamAngles;
	VectorAngles( vCamera, CamAngles );

	SetAbsOrigin( vIntroCamera );
	SetAbsAngles( CamAngles );
	SnapEyeAngles( CamAngles );
	m_fIntroCamTime = gpGlobals->curtime + 6;
}

void CSDKPlayer::State_PreThink_WELCOME()
{
	// Update whatever intro camera it's at.
	if( m_pIntroCamera && (gpGlobals->curtime >= m_fIntroCamTime) )
	{
		MoveToNextIntroCamera();
	}
}

void CSDKPlayer::State_Enter_DEATH_ANIM()
{
	if ( HasWeapons() )
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}

	// Used for a timer.
	m_flDeathTime = gpGlobals->curtime;

	StartObserverMode( OBS_MODE_DEATHCAM );	// go to observer mode

	RemoveEffects( EF_NODRAW );	// still draw player body
}


void CSDKPlayer::State_PreThink_DEATH_ANIM()
{
	// If the anim is done playing, go to the next state (waiting for a keypress to 
	// either respawn the guy or put him into observer mode).
	if ( GetFlags() & FL_ONGROUND )
	{
		float flForward = GetAbsVelocity().Length() - 20;
		if (flForward <= 0)
		{
			SetAbsVelocity( vec3_origin );
		}
		else
		{
			Vector vAbsVel = GetAbsVelocity();
			VectorNormalize( vAbsVel );
			vAbsVel *= flForward;
			SetAbsVelocity( vAbsVel );
		}
	}

	if ( gpGlobals->curtime >= (m_flDeathTime + SDK_PLAYER_DEATH_TIME ) )	// let the death cam stay going up to min spawn time.
	{
		m_lifeState = LIFE_DEAD;

		StopAnimation();

		AddEffects( EF_NOINTERP );

		if ( GetMoveType() != MOVETYPE_NONE && (GetFlags() & FL_ONGROUND) )
			SetMoveType( MOVETYPE_NONE );
	}

	//Tony; if we're now dead, and not changing classes, spawn
	if ( m_lifeState == LIFE_DEAD )
	{
#if defined ( SDK_USE_PLAYERCLASSES )
		//Tony; if the class menu is open, don't respawn them, wait till they're done.
		if (IsClassMenuOpen())
			return;
#endif
		State_Transition( STATE_ACTIVE );
	}
}

void CSDKPlayer::State_Enter_OBSERVER_MODE()
{
	// Always start a spectator session in roaming mode
	m_iObserverLastMode = OBS_MODE_ROAMING;

	if( m_hObserverTarget == NULL )
	{
		// find a new observer target
		CheckObserverSettings();
	}

	// Change our observer target to the nearest teammate
	CTeam *pTeam = GetGlobalTeam( GetTeamNumber() );

	CBasePlayer *pPlayer;
	Vector localOrigin = GetAbsOrigin();
	Vector targetOrigin;
	float flMinDist = FLT_MAX;
	float flDist;

	for ( int i=0;i<pTeam->GetNumPlayers();i++ )
	{
		pPlayer = pTeam->GetPlayer(i);

		if ( !pPlayer )
			continue;

		if ( !IsValidObserverTarget(pPlayer) )
			continue;

		targetOrigin = pPlayer->GetAbsOrigin();

		flDist = ( targetOrigin - localOrigin ).Length();

		if ( flDist < flMinDist )
		{
			m_hObserverTarget.Set( pPlayer );
			flMinDist = flDist;
		}
	}

	StartObserverMode( m_iObserverLastMode );
	PhysObjectSleep();
}

void CSDKPlayer::State_PreThink_OBSERVER_MODE()
{

	//Tony; if we're in eye, or chase, validate the target - if it's invalid, find a new one, or go back to roaming
	if (  m_iObserverMode == OBS_MODE_IN_EYE || m_iObserverMode == OBS_MODE_CHASE )
	{
		//Tony; if they're not on a spectating team use the cbaseplayer validation method.
		if ( GetTeamNumber() != TEAM_SPECTATOR )
			ValidateCurrentObserverTarget();
		else
		{
			if ( !IsValidObserverTarget( m_hObserverTarget.Get() ) )
			{
				// our target is not valid, try to find new target
				CBaseEntity * target = FindNextObserverTarget( false );
				if ( target )
				{
					// switch to new valid target
					SetObserverTarget( target );	
				}
				else
				{
					// let player roam around
					ForceObserverMode( OBS_MODE_ROAMING );
				}
			}
		}
	}
}

#if defined ( SDK_USE_PLAYERCLASSES )
void CSDKPlayer::State_Enter_PICKINGCLASS()
{
	ShowClassSelectMenu();
	PhysObjectSleep();

}
#endif // SDK_USE_PLAYERCLASSES

#if defined ( SDK_USE_TEAMS )
void CSDKPlayer::State_Enter_PICKINGTEAM()
{
	ShowViewPortPanel( PANEL_TEAM );
	PhysObjectSleep();

}
#endif // SDK_USE_TEAMS

void CSDKPlayer::State_Enter_ACTIVE()
{
	SetMoveType( MOVETYPE_WALK );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
    m_Local.m_iHideHUD = 0;
	PhysObjectWake();

	//Tony; call spawn again now -- remember; when we add respawn timers etc, to just put them into the spawn queue, and let the queue respawn them.
	Spawn();
	RemoveEffects(EF_NODRAW); //ios hack - player spawns invisible sometimes
}

void CSDKPlayer::State_PreThink_ACTIVE()
{
}

int CSDKPlayer::GetPlayerStance()
{
#if defined ( SDK_USE_PRONE )
	if (m_Shared.IsProne() || ( m_Shared.IsGoingProne() || m_Shared.IsGettingUpFromProne() ))
		return PINFO_STANCE_PRONE;
#endif

#if defined ( SDK_USE_SPRINTING )
	if (IsSprinting())
		return PINFO_STANCE_SPRINTING;
#endif
	if (m_Local.m_bDucking)
		return PINFO_STANCE_DUCKING;
	else
		return PINFO_STANCE_STANDING;
}

void CSDKPlayer::NoteWeaponFired( void )
{
	Assert( m_pCurrentCommand );
	if( m_pCurrentCommand )
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

bool CSDKPlayer::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	// No need to lag compensate at all if we're not attacking in this command and
	// we haven't attacked recently.
	if ( !( pCmd->buttons & IN_ATTACK ) && (pCmd->command_number - m_iLastWeaponFireUsercmd > 5) )
		return false;

	return BaseClass::WantsLagCompensationOnEntity( pPlayer, pCmd, pEntityTransmitBits );
}

//IOS

#define	IOS_USE_RADIUS		45.f
#define	IOS_HEADER_RADIUS	45.f


CBaseEntity *CSDKPlayer::FindUseEntity()
{
	Vector forward, up;
	EyeVectors( &forward, NULL, &up );
	forward.z = 0.0f;
	VectorNormalize(forward);
	CBaseEntity *pObject  = NULL;

	trace_t tr;
	// Search for objects in a sphere (tests for entities that are not solid, yet still useable)
	Vector searchCenter = GetAbsOrigin(); //EyePosition();

	for ( CEntitySphereQuery sphere( searchCenter, IOS_USE_RADIUS ); ( pObject = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( !pObject )
			continue;

		if ( !IsUseableEntity( pObject, FCAP_CONTINUOUS_USE ) )	//only ball has this?
			continue;

		// see if it's more roughly in front of the player than previous guess
		Vector point;
		pObject->CollisionProp()->CalcNearestPoint( searchCenter, &point );

		//Vector dir = point - searchCenter;
		//dir.z = 0.0f;
		//VectorNormalize(dir);
		//float dot = DotProduct( dir, forward );
		//Warning ("dot = %f\n", dot);
		// Need to be looking at the object more or less
		////if ( dot < -0.7 )
		//if ( dot < 0.0f )
		//	continue;

		return pObject;
	}

	//check for near head
	searchCenter += Vector (0.0f, 0.0f, 60.f);
	pObject  = NULL;
	for ( CEntitySphereQuery sphere( searchCenter, IOS_HEADER_RADIUS ); ( pObject = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( !pObject )
			continue;

		if ( !IsUseableEntity( pObject, FCAP_CONTINUOUS_USE ) )	//only ball has this?
			continue;

		// see if it's more roughly in front of the player than previous guess
		Vector point;
		pObject->CollisionProp()->CalcNearestPoint( searchCenter, &point );

		//Vector dir = point - searchCenter;
		//dir.z = 0.0f;
		//VectorNormalize(dir);
		//float dot = DotProduct( dir, forward );
		//Warning ("dot = %f\n", dot);
		// Need to be looking at the object more or less
		//if ( dot < -0.7 )
		//if ( dot < 0.0f )
		//	continue;

		return pObject;
	}




	return pObject;
}


//-----------------------------------------------------------------------------
// Purpose: Handles USE keypress //IOS
//-----------------------------------------------------------------------------
void CSDKPlayer::PlayerUse ( void )
{

	if (m_TeamPos < 1)
		return;

	//not allowed to interact with ball just now
	if (m_NextShoot > gpGlobals->curtime)
		return;

	//POWERSHOT - CHARGING UP (not USING ball)
	//if (m_nButtons & IN_ALT1)
	//{
	//	//first time
	//	if (m_NextPowerShotCharge==0.0f)
	//	{
	//		m_PowerShotStrength = 0.0f;
	//	}
	//	//charge over time
	//	if (m_NextPowerShotCharge < gpGlobals->curtime)
	//	{
	//		m_PowerShotStrength += 0.1f;
	//		m_NextPowerShotCharge = gpGlobals->curtime + 0.1f;
	//		if (m_PowerShotStrength > 2.0f)
	//			m_PowerShotStrength = 0.0f;
	//		//Warning ("power=%2f\n", m_PowerShotStrength);
	//	}
	//}
	//else
	//{
	//	if (m_NextPowerShotCharge < gpGlobals->curtime)
	//	{
	//		m_NextPowerShotCharge = gpGlobals->curtime + 0.1f;
	//		if (m_PowerShotStrength > 0.0f)
	//			m_PowerShotStrength -= 0.1f;
	//		if (m_PowerShotStrength < 0.0f)
	//			m_PowerShotStrength = 0.0f;
	//		//Warning ("power=%2f\n", m_PowerShotStrength);
	//	}
	//}





	// Was use pressed or released?
	if ( ! ((m_nButtons | m_afButtonPressed | m_afButtonReleased) & (IN_USE | IN_ATTACK | IN_ATTACK2 | IN_ALT1)) 
		&& !m_bSlideKick)
		return;

	if ( IsObserver() )
	{
		// do special use operation in oberserver mode
		if ( m_afButtonPressed & IN_USE )
			ObserverUse( true );
		else if ( m_afButtonReleased & IN_USE )
			ObserverUse( false );
		
		return;
	}


	CBaseEntity *pUseEntity = FindUseEntity();
	// Found an object
	if ( pUseEntity )
	{

		int caps = pUseEntity->ObjectCaps();
		variant_t emptyVariant;
		//if ( ( (m_nButtons & (IN_USE | IN_ATTACK | IN_ATTACK2 | IN_ALT1)) && (caps & FCAP_CONTINUOUS_USE) ) || ( (m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE|FCAP_ONOFF_USE)) )
		//	|| (m_afButtonReleased & (IN_USE | IN_ATTACK | IN_ATTACK2 | IN_ALT1)) )
		//{
			if ( caps & FCAP_CONTINUOUS_USE )
			{
				m_afPhysicsFlags |= PFLAG_USING;
			}

			//POWERSHOT - KICK
			//if (m_afButtonReleased & IN_ALT1)	
			if (m_nButtons & IN_ALT1)				//after scoring a great goal in ios pub - the "released" seems to hard
			{
				pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_POWERSHOT_KICK );
			}
			//SHOOT
			else if ( (m_nButtons & IN_ATTACK) || m_bSlideKick)
			{
				pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_SHOOT );
			}
			//PASS
			else if ( m_nButtons & IN_ATTACK2  || m_bSlideKick)
			{
				pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_PASS );
			}
		//}
	
/*
			if ( pUseEntity->ObjectCaps() & FCAP_ONOFF_USE )
			{
				pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_ON );
			}
			else
			{
				pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_TOGGLE );
			}
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ( (m_afButtonReleased & IN_USE) && (pUseEntity->ObjectCaps() & FCAP_ONOFF_USE) )	// BUGBUG This is an "off" use
		{
			pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_OFF );
		}
		*/
	}
	else if ( m_afButtonPressed & IN_USE )
	{
		//EmitSound( "Player.UseDeny" );
	}

	m_bSlideKick = false;
}


//ios TODO: move to animstate class

// Set the activity based on an event or current state
void CSDKPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;
	char szAnim[64];

	float speed;


	//ios - if hold anim playing just return
	if (m_HoldAnimTime > gpGlobals->curtime)
		return;
	else if (m_HoldAnimTime != -1)						//-1 means something else is using fl_atcontrols
		RemoveFlag(FL_ATCONTROLS);


	if (m_KickDelay > gpGlobals->curtime)
		return;
	else if (m_TeamPos>=1 && m_TeamPos<=11)
		RemoveFlag(FL_FROZEN);


	speed = GetAbsVelocity().Length2D();

	//if (GetFlags() & (FL_FROZEN|FL_ATCONTROLS))
	if (GetFlags() & (FL_FROZEN))
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	Activity idealActivity = ACT_WALK;// TEMP!!!!!

	// This could stand to be redone. Why is playerAnim abstracted from activity? (sjb)
	if (playerAnim == PLAYER_JUMP)
	{
		idealActivity = ACT_HOP;
	}
	else if (playerAnim == PLAYER_DIVE_LEFT)
	{
		idealActivity = ACT_ROLL_LEFT;
	}
	else if (playerAnim == PLAYER_DIVE_RIGHT)
	{
		idealActivity = ACT_ROLL_RIGHT;
	}
	else if (playerAnim == PLAYER_SUPERJUMP)
	{
		idealActivity = ACT_LEAP;
	}
	else if (playerAnim == PLAYER_DIE)
	{
		if ( m_lifeState == LIFE_ALIVE )
		{
			idealActivity = GetDeathActivity();
		}
	}
	else if (playerAnim == PLAYER_ATTACK1)
	{
		if ( m_Activity == ACT_HOVER	|| 
			 m_Activity == ACT_SWIM		||
			 m_Activity == ACT_HOP		||
			 m_Activity == ACT_LEAP		||
			 m_Activity == ACT_DIESIMPLE )
		{
			idealActivity = m_Activity;
		}
		else
		{
			idealActivity = ACT_RANGE_ATTACK1;
		}
	}
	else if (playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK)
	{
		if ( !( GetFlags() & FL_ONGROUND ) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP) )	// Still jumping
		{
			idealActivity = m_Activity;
		}
		else if ( GetWaterLevel() > 1 )
		{
			if ( speed == 0 )
				idealActivity = ACT_HOVER;
			else
				idealActivity = ACT_SWIM;
		}
		else
		{
			//ios idealActivity = ACT_WALK;
			idealActivity = ACT_IOS_JUMPCELEB;
		}
	}
	else if (playerAnim == PLAYER_VOLLEY)
	{
		if (m_HoldAnimTime < gpGlobals->curtime)
		{
			m_HoldAnimTime = gpGlobals->curtime + 0.6f;
			AddFlag(FL_ATCONTROLS);
			SetAbsVelocity( vec3_origin );
		}
	}
	else if (playerAnim == PLAYER_HEELKICK)
	{
		if (m_HoldAnimTime < gpGlobals->curtime)
		{
			m_HoldAnimTime = gpGlobals->curtime + 0.5333f;
			AddFlag(FL_ATCONTROLS);
			SetAbsVelocity( vec3_origin );
		}
	}
	else if (playerAnim == PLAYER_THROWIN)
	{
		//hold arms up - timer is held on by throwin code in ball.cpp
	}
	else if (playerAnim == PLAYER_THROW)
	{
		//actually throw the ball from a throwin
		if (m_HoldAnimTime < gpGlobals->curtime)
		{
			m_HoldAnimTime = gpGlobals->curtime + 0.5333f;
			AddFlag(FL_ATCONTROLS);
			SetAbsVelocity( vec3_origin );
		}
	}
	else if (playerAnim == PLAYER_SLIDE)
	{
		//slide tackle hold
		if (m_HoldAnimTime < gpGlobals->curtime)
		{
			m_HoldAnimTime = gpGlobals->curtime + 1.7f;
		}
	}
	else if (playerAnim == PLAYER_TACKLED_FORWARD)
	{
		if (m_HoldAnimTime < gpGlobals->curtime)
		{
			m_HoldAnimTime = gpGlobals->curtime + 3.0f;
			AddFlag(FL_ATCONTROLS);
			SetAbsVelocity( vec3_origin );
		}
	}
	else if (playerAnim == PLAYER_TACKLED_BACKWARD)
	{
		if (m_HoldAnimTime < gpGlobals->curtime)
		{
			m_HoldAnimTime = gpGlobals->curtime + 3.0f;
			AddFlag(FL_ATCONTROLS);
			SetAbsVelocity( vec3_origin );
		}
	}
	else if (playerAnim == PLAYER_DIVINGHEADER)
	{
		if (m_HoldAnimTime < gpGlobals->curtime)
		{
			m_HoldAnimTime = gpGlobals->curtime + 1.5f;
			EmitSound ("Player.DivingHeader");
		}
	}


	m_PlayerAnim = playerAnim;
	
	if (idealActivity == ACT_RANGE_ATTACK1)
	{
		if ( GetFlags() & FL_DUCKING )	// crouching
		{
			Q_strncpy( szAnim, "crouch_shoot_" ,sizeof(szAnim));
		}
		else
		{
			Q_strncpy( szAnim, "ref_shoot_" ,sizeof(szAnim));
		}
		Q_strncat( szAnim, m_szAnimExtension ,sizeof(szAnim), COPY_ALL_CHARACTERS );
		animDesired = LookupSequence( szAnim );
		if (animDesired == -1)
			animDesired = 0;

		if ( GetSequence() != animDesired || !SequenceLoops() )
		{
			SetCycle( 0 );
		}

		// Tracker 24588:  In single player when firing own weapon this causes eye and punchangle to jitter
		//if (!SequenceLoops())
		//{
		//	AddEffects( EF_NOINTERP );
		//}

		SetActivity( idealActivity );
		ResetSequence( animDesired );
	}
	else if (idealActivity == ACT_WALK)
	{
		if (GetActivity() != ACT_RANGE_ATTACK1 || IsActivityFinished())
		{
			if ( GetFlags() & FL_DUCKING )	// crouching
			{
				Q_strncpy( szAnim, "crouch_aim_" ,sizeof(szAnim));
			}
			else
			{
				Q_strncpy( szAnim, "ref_aim_" ,sizeof(szAnim));
			}
			Q_strncat( szAnim, m_szAnimExtension,sizeof(szAnim), COPY_ALL_CHARACTERS );
			animDesired = LookupSequence( szAnim );
			if (animDesired == -1)
				animDesired = 0;
			SetActivity( ACT_WALK );
		}
		else
		{
			animDesired = GetSequence();
		}
	}
	else
	{
		if ( GetActivity() == idealActivity)
			return;
	
		SetActivity( idealActivity );

		animDesired = SelectWeightedSequence( m_Activity );

		// Already using the desired animation?
		if (GetSequence() == animDesired)
			return;

		ResetSequence( animDesired );
		SetCycle( 0 );
		return;
	}

	// Already using the desired animation?
	if (GetSequence() == animDesired)
		return;

	//Msg( "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	ResetSequence( animDesired );
	SetCycle( 0 );
}

//ConVar mp_autobalance( "mp_autobalance", "1", FCVAR_REPLICATED|FCVAR_NOTIFY, "autobalance teams after a goal. blocks joining unbalanced teams" );

bool CSDKPlayer::ClientCommand( const CCommand &args )
{
	const char *pcmd = args[0];

	if ( FStrEq( pcmd, "jointeam" ) ) 
	{
		if ( args.ArgC() < 2)
		{
			Warning( "Player sent bad jointeam syntax\n" );
			return false;	//go away
		}

		//block joining from on pitch etc
		if (GetTeamNumber() != TEAM_UNASSIGNED)
			return false;
		if (m_TeamPos > 0)
			return false;

		int iTeam = atoi( args[1] );

		if (iTeam < TEAM_UNASSIGNED || iTeam > TEAM_B)
			return false;

		CSDKGameRules* pGamerules = static_cast <CSDKGameRules*>(g_pGameRules);

		switch (iTeam)
		{
			case TEAM_UNASSIGNED:		//AUTO
			default:
			{
				pGamerules->CountTeams();
				int na = pGamerules->m_PlayersOnTeam[TEAM_A];
				int nb = pGamerules->m_PlayersOnTeam[TEAM_B];
				if (na > nb)
					ChangeTeam(TEAM_B);
				else
					ChangeTeam(TEAM_A);
				ChooseModel();
				ShowViewPortPanel( PANEL_CLASS ); 
			}
			break;

			case TEAM_SPECTATOR:		//SPEC
				ChangeTeam(TEAM_SPECTATOR);
				Spawn();
				m_TeamPos = -1;
				RemoveFlag(FL_FROZEN);
				return true;
			break;

			case TEAM_A:
			case TEAM_B:
				pGamerules->CountTeams();
				//only check if autobalance is on
				float flDoAutobalance = autobalance.GetBool();
				if (flDoAutobalance)
				{
					//limit teams as you join
					int na = pGamerules->m_PlayersOnTeam[TEAM_A];
					int nb = pGamerules->m_PlayersOnTeam[TEAM_B];
					int diff = na-nb;
					//check for trying to join the "wrong" team (build in a tolerance)
					if ((diff < -1 && iTeam==TEAM_B) || (diff > 1 && iTeam==TEAM_A)) 
					{
						ShowViewPortPanel( PANEL_TEAM ); 
						return true;
					}
				}

				ChangeTeam( iTeam );
				ChooseModel();
				ShowViewPortPanel( PANEL_CLASS ); 
			break;
		}

		return true;
	}

	if ( FStrEq( pcmd, "pos" ) ) 
	{
		if ( args.ArgC() < 2)
		{
			return false;	//go away
		}

		int posWanted = atoi (args[1]);	//teampos matches spawn points on the map 2-11

		if (posWanted < 1 || posWanted > 11)
			return false;

		if (GetTeamNumber() < TEAM_A)
			return false;

		//block pos when already got a teampos
		if (m_TeamPos > 0)
			return false;

		if (TeamPosFree(GetTeamNumber(), posWanted, true))
		{
			m_TeamPos = posWanted;	//teampos matches spawn points on the map 2-11

			ConvertSpawnToShirt();					//shirtpos is the formation number 3,4,5,2 11,8,6,7 10,9

			if (m_ShirtPos > 1)
			{
				ChoosePlayerSkin();				
			}
			else
			{
				ChooseKeeperSkin();
			}

			//spawn at correct position
			Spawn();
			g_pGameRules->GetPlayerSpawnSpot( this );
			RemoveEffects( EF_NODRAW );
			SetSolid( SOLID_BBOX );
			RemoveFlag(FL_FROZEN);
			return true;
		}
		else
		{
			ShowViewPortPanel( PANEL_CLASS );
			return true;
		}
	}


	return BaseClass::ClientCommand (args);
}


/////////////////////////////////////////
//check if this IOS team position is free
//
bool CSDKPlayer::TeamPosFree (int team, int pos, bool kickBotKeeper)
{
	//stop keeper from being picked unless mp_keepers is 0 
	//(otherwise you can pick keeper just before bots spawn)

	if (pos == 1 && !humankeepers.GetBool())
		return false;

	//normal check
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )	
	{
		CSDKPlayer *plr = (CSDKPlayer*)UTIL_PlayerByIndex( i );

		if (!plr)
			continue;

		if (!plr->IsPlayer())
			continue;

		if ( !plr->IsConnected() )
			continue;

		//check for null,disconnected player
		if (strlen(plr->GetPlayerName()) == 0)
			continue;

		if (plr->GetTeamNumber() == team && plr->m_TeamPos == pos)
		{
			if (!plr->IsBot() || !kickBotKeeper)
				return false;

			char kickcmd[512];
			Q_snprintf(kickcmd, sizeof(kickcmd), "kickid %i Human player taking the spot\n", plr->GetUserID());
			engine->ServerCommand(kickcmd);
			return true;
		}
	}
	return true;
}

static const int NUM_PLAYER_FACES = 6;
static const int NUM_BALL_TYPES = 6;

////////////////////////////////////////////////
// player skins are 0-9 (blocks of 10)
// (shirtpos-2) is always 0-9
//
void CSDKPlayer::ChoosePlayerSkin(void)
{
	m_nSkin = m_ShirtPos-2 + (g_IOSRand.RandomInt(0,NUM_PLAYER_FACES-1) * 10);		//player skin
	m_nBody = MODEL_PLAYER;
}


///////////////////////////////////////////////
// keeper skins start at (faces*10) e.g. 60.
// so index into the start of a particular face group
// then (later) by adding on the ball skin number to this
// skin we will get the keeper with the correct ball.
//
void CSDKPlayer::ChooseKeeperSkin(void)
{
	m_nSkin = NUM_PLAYER_FACES*10 + (g_IOSRand.RandomInt(0,NUM_PLAYER_FACES-1) * NUM_BALL_TYPES);
	m_nBaseSkin = m_nSkin;
	m_nBody = MODEL_KEEPER;
}

void CSDKPlayer::ChooseModel(void)
{
	//TODO: replace with generic player model
	char *model = "models/player/barcelona/barcelona.mdl";
	PrecacheModel(model);
	SetModel (model);
}


void CSDKPlayer::ConvertSpawnToShirt(void)
{
	switch (m_TeamPos)
	{
		case 1: m_ShirtPos = 1; break;
		case 2: m_ShirtPos = 3; break;
		case 3: m_ShirtPos = 4; break;
		case 4: m_ShirtPos = 5; break;
		case 5: m_ShirtPos = 2; break;
		case 6: m_ShirtPos = 11; break;
		case 7: m_ShirtPos = 8; break;
		case 8: m_ShirtPos = 6; break;
		case 9: m_ShirtPos = 7; break;
		case 10: m_ShirtPos = 10; break;
		case 11: m_ShirtPos = 9; break;
	}
}

#include "team.h"

void CSDKPlayer::RequestTeamChange( int iTeamNum )
{
	if ( iTeamNum == 0 )	// this player chose to auto-select his team
	{
		int iNumTeamA = GetGlobalTeam( TEAM_A )->GetNumPlayers();
		int iNumTeamB = GetGlobalTeam ( TEAM_B )->GetNumPlayers();

		if ( iNumTeamA > iNumTeamB)
			iTeamNum = TEAM_B;
		else if ( iNumTeamB > iNumTeamA)
			iTeamNum = TEAM_A;
		else
			iTeamNum = TEAM_A;	// the default team
	}

	if ( !GetGlobalTeam( iTeamNum ) )
	{
		Warning( "CBasePlayer::ChangeTeam( %d ) - invalid team index.\n", iTeamNum );
		return;
	}

	// if this is our current team, just abort
	if ( iTeamNum == GetTeamNumber() )
		return;

	ChangeTeam (iTeamNum);		// TeamChanges are actually done at the round start.

	if ( IsBot() == false )
	{
		// Now force the player to choose a model
		if ( iTeamNum == TEAM_A )
			ShowViewPortPanel( PANEL_CLASS_CT );
		else if ( iTeamNum == TEAM_B )
			ShowViewPortPanel( PANEL_CLASS_TEAMB );
	}
	else
	{
		//if ( iTeamNum == TEAM_A )
		//{
		//	int iRandom = random->RandomInt(0,2);
		//	switch (iRandom)
		//	{
		//		case 0 : RequestModel( "models/player/desertfox.mdl" ); m_iPlayerModel = PLAYER_DESERTFOX; break;
		//		case 1 : RequestModel( "models/player/arctic.mdl" ); m_iPlayerModel = PLAYER_ARCTIC; break;
		//		case 2 : RequestModel( "models/player/snake.mdl" ); m_iPlayerModel = PLAYER_SNAKE; break;
		//		default : RequestModel( "models/player/snake.mdl" ); m_iPlayerModel = PLAYER_SNAKE; break;
		//	}
		//}
		//else if ( iTeamNum == TEAM_B )
		//{
		//	int iRandom = random->RandomInt(0,2);
		//	switch (iRandom)
		//	{
		//		case 0 :RequestModel( "models/player/gign.mdl" ); m_iPlayerModel = PLAYER_GIGN; break;
		//		case 1 :RequestModel( "models/player/gsg9.mdl" ); m_iPlayerModel = PLAYER_GSG9; break;
		//		case 2 :RequestModel( "models/player/devgru.mdl" ); m_iPlayerModel = PLAYER_DEVGRU; break;
		//		default : RequestModel( "models/player/devgru.mdl" ); m_iPlayerModel = PLAYER_DEVGRU; break;
		//	}
		//}
	}

}


extern CBaseEntity *FindPlayerStart(const char *pszClassName);
extern CBaseEntity	*g_pLastSpawn;
/*
============
EntSelectSpawnPoint

Returns the entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/
CBaseEntity *CSDKPlayer::EntSelectSpawnPoint(void)
{
	CBaseEntity *pSpot;
	edict_t		*player;
	char		szSpawnName[120];
	player = edict();

												
	if ( GetTeamNumber() == TEAM_A )
		Q_snprintf( szSpawnName , sizeof(szSpawnName) , "info_team1_player%d", m_TeamPos );
	else
		Q_snprintf( szSpawnName , sizeof(szSpawnName) , "info_team2_player%d", m_TeamPos );

	pSpot = FindPlayerStart(szSpawnName);

	if ( !pSpot  )
	{
		pSpot = BaseClass::EntSelectSpawnPoint();
	}

	g_pLastSpawn = pSpot;
	return pSpot;
}


void CSDKPlayer::CheckRejoin(void)
{
	if (m_RejoinTime && m_RejoinTime < gpGlobals->curtime)
	{
		//show menu again
		KeyValues *data = new KeyValues("data");
		data->SetString("team1", GetGlobalTeam( TEAM_A )->GetName());
		data->SetString("team2", GetGlobalTeam( TEAM_B )->GetName());
		ShowViewPortPanel( PANEL_TEAM, true, data );		//ios - send team names in data?
		m_RejoinTime = 0;
	}
}

void CSDKPlayer::CommitSuicide( bool bExplode /*= false*/, bool bForce /*= false*/ )
{
	int teamNumber = GetTeamNumber();
	int teamPos = m_TeamPos;
	bool isBot = IsBot();

	if (GetTeamNumber()==TEAM_SPECTATOR)
	{
		m_RejoinTime = gpGlobals->curtime + 3;
		ChangeTeam( TEAM_UNASSIGNED );
		m_TeamPos = -1;
		return;
	}

	if( !IsAlive() )
		return;
		
	// prevent suiciding too often
	if ( m_fNextSuicideTime > gpGlobals->curtime )
		return;

	// don't let them suicide for 5 seconds after suiciding
	m_fNextSuicideTime = gpGlobals->curtime + 5;  

	m_RejoinTime = gpGlobals->curtime + 3;

	ChangeTeam( TEAM_UNASSIGNED );
	m_TeamPos = -1;

	//check all balls for interaction with this player
	CBall	*pBall = GetBall(NULL);
	while (pBall)
	{
		if (pBall->m_BallShieldPlayer == this)		//remove ball shield
		{
			pBall->ballStatusTime = 0;
			pBall->ShieldOff();
		}

		if (pBall->m_Foulee == this)
			pBall->m_Foulee = NULL;

		if (pBall->m_KeeperCarrying == this)
			pBall->DropBall();

		pBall = GetBall(pBall);
	}

	// have the player kill themself
	m_iHealth = 0;
	Event_Killed( CTakeDamageInfo( this, this, 0, DMG_NEVERGIB ) );
	Event_Dying();

	if (!isBot && teamPos == 1 && botkeepers.GetBool())
	{
		//RomD: Add bot keeper if killed player was a keeper
		BotPutInServer(false, teamNumber == TEAM_A ? 1 : 2); 
	}
}


bool CSDKPlayer::InSprint(void)
{
	return ((m_nButtons & IN_RELOAD) ? 1 : 0);
}

void CSDKPlayer::UpdateSprint(void)
{
	//if pressing sprint, set speed and reduce sprint
	if (m_nButtons & IN_RELOAD)
	{
		if (m_fSprintLeft > 0.0f) 
		{
			if (GetAbsVelocity().Length2D() > 10.0f)
			{
				SetMaxSpeed(PLAYER_SPEED + SPRINT_SPEED);
				m_fSprintLeft -= gpGlobals->frametime;
			}
		}
		else
		{
			SetMaxSpeed(PLAYER_SPEED);		//none left
			m_fSprintIdle = 0.0f;
		}
		return;
	}

	//not pressing sprint so recharge 
	if (m_fSprintIdle < gpGlobals->curtime) 
	{
		m_fSprintIdle = gpGlobals->curtime + 0.25f;
		m_fSprintLeft += (SPRINT_TIME / (SPRINT_RECHARGE_TIME * 4.0f));
		if (m_fSprintLeft > SPRINT_TIME)
			m_fSprintLeft = SPRINT_TIME;
	}

	SetMaxSpeed(PLAYER_SPEED);
}


/////////////////////////////////////////////////////////////////////////////
//slidetackle - not really related to Duck (which is in gamemovement.cpp)
//
void CSDKPlayer::Duck(void)
{
//#if 1
//	//debug force test a foul/penalty
//	if (m_PlayerAnim == PLAYER_SLIDE)
//	{
//		CBall *pBall = FindNearestBall();
//		pBall->m_Foulee = this;							//us
//		pBall->m_Foulee = NULL;
//		pBall->m_FoulPos = pBall->GetAbsOrigin();
//		if (pBall->m_BallInPenaltyBox!=-1 && pBall->m_BallInPenaltyBox != GetTeamNumber())	//oppositions box
//			pBall->ballStatus = BALL_PENALTY;
//		else
//			pBall->ballStatus = BALL_FOUL;
//		pBall->ballStatusTime = gpGlobals->curtime + 3.0f;
//		pBall->m_team = GetTeamNumber();				//foul on our team - this time.
//		pBall->m_FoulInPenaltyBox = pBall->m_BallInPenaltyBox;
//		return;
//	}
//#endif

	//first time
	if (m_PlayerAnim != PLAYER_SLIDE)
	{
		//not on the pitch
		if (!IsOnPitch())
			return;

		if (m_nButtons & IN_DUCK && GetAbsVelocity().Length() > 200.0f)
		{
			CBall	*pBall = GetBall(NULL);
			//see if we can slide - check all balls on the pitch
			while (pBall)
			{
				//lets goal celeb through to slide but rejects other ball states
				if ((pBall->ballStatus > 0 && (pBall->ballStatus != BALL_KICKOFF)) || pBall->m_BallShieldPlayer == this)
					return;

				pBall = GetBall(pBall);
			}

			if (m_NextSlideTime < gpGlobals->curtime)
			{
				SetAnimation (PLAYER_SLIDE);
				DoAnimationEvent(PLAYERANIMEVENT_SLIDE);
				m_NextSlideTime = gpGlobals->curtime + 3.0f;	//time before we can slide again
				m_TackleTime = gpGlobals->curtime + 0.5f;		//tackle and stop moving
				EmitSound ("Player.Slide");
				m_bTackleDone = false;
			}
		}
	}

	//do the tackle
	if (m_PlayerAnim == PLAYER_SLIDE && m_TackleTime < gpGlobals->curtime && !m_bTackleDone)
	{
		m_bTackleDone = true;
		AddFlag(FL_ATCONTROLS);								//freeze movement now (this gets cleared in SetAnimation when slide ends)
		m_bSlideKick = true;								//record a kick ourself since m_nButtons gets cleared

		//find nearest player:
		CSDKPlayer *pClosest = FindNearestSlideTarget();

		CBall *pBall = FindNearestBall();

		if (!pBall)										//no ball!
			return;

		if (pBall->ballStatus > 0)						//dont do tackle unless in normal play
			return;

		if (!pClosest)									//found no one
			return;

		if (pClosest->m_TeamPos<=1)						//dont foul keepers or spectators
			return;

		if ((pBall->GetAbsOrigin() - pClosest->GetAbsOrigin()).Length2D() > 144)
			return;

		int slideTackle = (int)slidetackle.GetFloat();
		slideTackle = min(slideTackle, 100);
		slideTackle = max(0,slideTackle);
		if (g_IOSRand.RandomInt(0,100) > slideTackle)
			return;

		if (pClosest->IsPlayer() && pClosest->GetTeamNumber() == GetTeamNumber())
			return;

		float fClosestDist = (GetAbsOrigin() - pClosest->GetAbsOrigin()).Length2D();
		float fBallDist = (GetAbsOrigin() - pBall->GetAbsOrigin()).Length2D();

		//if (fBallDist < fClosestDist)
		//{
		//	Warning ("clean tackle\n");
		//	return;
		//}

		//we've fouled for sure
		bool bLegalTackle = ApplyTackledAnim (pClosest);
		EmitAmbientSound(entindex(), GetAbsOrigin(), "Player.Card");

		//IOSS 1.1 fix, let the anim play even if it's clean
		if (fBallDist < fClosestDist)
		{
			Warning ("clean tackle\n");
			return;
		}

		pBall->m_Foulee = (CSDKPlayer*)pClosest;
		pBall->m_FoulPos = pBall->GetAbsOrigin();
		//pBall->m_team = GetTeamNumber();	//our team, not the team of the person who's taking the kick!
		//give FK to other team:
		if (GetTeamNumber()==TEAM_A)
			pBall->m_team = TEAM_B;
		else
			pBall->m_team = TEAM_A;

		pBall->m_FoulInPenaltyBox = pBall->m_BallInPenaltyBox;
		if (pBall->m_BallInPenaltyBox == GetTeamNumber())
		{
			pBall->ballStatus = BALL_PENALTY;
			bLegalTackle = FALSE;
		}
		else
		{
			pBall->ballStatus = BALL_FOUL;
		}
		pBall->ballStatusTime = gpGlobals->curtime + 3.0f;
		//EmitAmbientSound(entindex(), GetAbsOrigin(), "Player.Card");

		//do foul message before card message
		char  msg[256];
		const char	title[16] = "FOUL\n";
		Q_snprintf (msg, sizeof (msg), "%s fouled by %s. ", pClosest->GetPlayerName(), GetPlayerName());
		pBall->SendVGUIStatusMessage (title, msg, true);

		//check for foul, yellow or red depending on distance from ball
		if (fBallDist < fClosestDist+64 && bLegalTackle) 
		{
			m_Fouls++;
			if (pBall->ballStatus == BALL_PENALTY)
				pBall->SendMainStatus(BALL_MAINSTATUS_PENALTY);
			else
				pBall->SendMainStatus(BALL_MAINSTATUS_FOUL);
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"foul\"\n", GetPlayerName(), GetUserID(),GetNetworkIDString(), GetTeam()->GetName() );
		}
		else if (fBallDist < fClosestDist+128)
		{
			m_YellowCards++;
			if (pBall->ballStatus == BALL_PENALTY)
				pBall->SendMainStatus(BALL_MAINSTATUS_PENALTY);
			else
				pBall->SendMainStatus(BALL_MAINSTATUS_YELLOW);
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"yellowcard\"\n", GetPlayerName(), GetUserID(),GetNetworkIDString(), GetTeam()->GetName() );

			if (!(m_YellowCards & 0x0001))   //is it a second yellow?
			{
				if (pBall->ballStatus == BALL_PENALTY)
					pBall->SendMainStatus(BALL_MAINSTATUS_PENALTY);
				else
					pBall->SendMainStatus(BALL_MAINSTATUS_RED);
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"redcard\"\n", GetPlayerName(), GetUserID(),GetNetworkIDString(), GetTeam()->GetName() );
				GiveRedCard();
			}
		} 
		else 
		{
			if (pBall->ballStatus == BALL_PENALTY)
				pBall->SendMainStatus(BALL_MAINSTATUS_PENALTY);
			else
				pBall->SendMainStatus(BALL_MAINSTATUS_RED);
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"redcard\"\n", GetPlayerName(), GetUserID(),GetNetworkIDString(), GetTeam()->GetName() );
			GiveRedCard();
		}
	}
}

/////////////////////////////////////////
//
// Choose anim depending on dir of tackle.
//	return true = from front or side = legal
//  return false = from behind
bool CSDKPlayer::ApplyTackledAnim(CSDKPlayer *pTackled)
{
	Vector vForward;
	AngleVectors (pTackled->EyeAngles(), &vForward);
	Vector vTackleDir = (pTackled->GetAbsOrigin() - GetAbsOrigin());
	VectorNormalize(vTackleDir);
	bool bLegalTackle = true;

	float fDot = DotProduct (vForward, vTackleDir);

	if (fDot > 0.0f)
	{
		pTackled->DoAnimationEvent(PLAYERANIMEVENT_TACKLED_FORWARD);		//send the event
		SetAnimation (PLAYER_TACKLED_FORWARD);								//freezes movement
	}
	else
	{
		pTackled->DoAnimationEvent(PLAYERANIMEVENT_TACKLED_BACKWARD);
		SetAnimation (PLAYER_TACKLED_BACKWARD);
	}

	if (fDot > 0.8f)														//if it's quite behind then illegal.
		bLegalTackle = false;

	return bLegalTackle;
}

/////////////////////////////////////////
//
// IOS give red card to this player
//
void CSDKPlayer::GiveRedCard(void)
{
	float t = redcardtime.GetFloat();							//time to keep player out, 0=disable

	m_RedCards++;												//inc

	if (t)
	{
		CommitSuicide();										//this gives a default rejoin of 3
		m_RejoinTime = gpGlobals->curtime + (t * m_RedCards);	//this makes it longer
	}
}


////////////////////////////////////////////////
//
CSDKPlayer	*CSDKPlayer::FindNearestSlideTarget(void)
{
	CBaseEntity *pTarget = NULL;
	CBaseEntity *pClosest = NULL;
	Vector		vecLOS;
	float flMaxDot = VIEW_FIELD_WIDE;
	float flDot;
	Vector	vForwardUs;
	AngleVectors (EyeAngles(),	&vForwardUs);
	
	while ((pTarget = gEntList.FindEntityInSphere( pTarget, GetAbsOrigin(), 64 )) != NULL)
	{
		if (pTarget->IsPlayer() && IsOnPitch()) 
		{
			vecLOS = pTarget->GetAbsOrigin() - GetAbsOrigin();
			vecLOS.z = 0.0f;		//remove the UP
			VectorNormalize(vecLOS);
			if (vecLOS.x==0.0f && vecLOS.y==0.0f)
				continue;
			flDot = DotProduct (vecLOS, vForwardUs);
			if (flDot > flMaxDot)
			{  
				flMaxDot = flDot;	//find nearest thing in view (using dist means not neccessarily the most direct onfront)
         		pClosest = pTarget;
			}
		}
	}
	return (CSDKPlayer*)pClosest;
}


////////////////////////////////////////////////
//
CBall *CSDKPlayer::FindNearestBall(void)
{
	CBall *pClosest = NULL;
	float distSq, maxDistSq = FLT_MAX;

	CBall	*pBall = GetBall(NULL);
	while (pBall)
	{
		distSq = ((pBall->GetAbsOrigin()-GetAbsOrigin()) * (pBall->GetAbsOrigin()-GetAbsOrigin())).LengthSqr();
		if (distSq < maxDistSq)
		{
			maxDistSq = distSq;
			pClosest = pBall;
		}
		pBall = GetBall(pBall);
	}
	return pClosest;
}


////////////////////////////////////////////////
//
CBall *CSDKPlayer::GetBall(CBall *pNext)
{
	return dynamic_cast<CBall*>(gEntList.FindEntityByClassname( pNext, "football" ));
}

////////////////////////////////////////////////
//
bool CSDKPlayer::IsOnPitch(void)
{
	CBaseEntity *pSpotTL=NULL;
	CBaseEntity *pSpotBR=NULL;

	pSpotTL = gEntList.FindEntityByClassname( NULL, "info_team1_corner0" );
	pSpotBR  = gEntList.FindEntityByClassname( NULL, "info_team2_corner1" );

	if (pSpotTL==NULL || pSpotBR==NULL)
		return true;     //daft mapper didn't put corner flags in the map

	if (GetAbsOrigin().x > pSpotTL->GetAbsOrigin().x &&
		GetAbsOrigin().x < pSpotBR->GetAbsOrigin().x &&
		GetAbsOrigin().y < pSpotTL->GetAbsOrigin().y &&
		GetAbsOrigin().y > pSpotBR->GetAbsOrigin().y)
		return true;
	else
		return false;
}

//paint decal check. OPPOSITE and more strict than IsPlayerOnPitch
bool CSDKPlayer::IsOffPitch(void)
{
	CBaseEntity *pSpotTL=NULL;
	CBaseEntity *pSpotBR=NULL;

	//use the player corener pos to get a safe border around the pitch
	pSpotTL = gEntList.FindEntityByClassname( NULL, "info_team1_corner_player0" );
	pSpotBR  = gEntList.FindEntityByClassname( NULL, "info_team2_corner_player1" );

	//cant spray at all on maps without off-pitch areas. ie always on pitch
	if (pSpotTL==NULL || pSpotBR==NULL)
		return false;

	if (GetAbsOrigin().x > pSpotTL->GetAbsOrigin().x &&
		GetAbsOrigin().x < pSpotBR->GetAbsOrigin().x &&
		GetAbsOrigin().y < pSpotTL->GetAbsOrigin().y &&
		GetAbsOrigin().y > pSpotBR->GetAbsOrigin().y)
		return false;	//dont spray on pitch area
	else
		return true;	//ok spray around edge
}

//-----------------------------------------------------------------------------
// The ball doesnt get every collision, so try player - for dribbling
//
//-----------------------------------------------------------------------------
void CSDKPlayer::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{

	//if (m_TeamPos < 1)
	//	return;

	////not allowed to interact with ball just now
	//if (m_NextShoot > gpGlobals->curtime)
	//	return;

	//CBaseEntity *pUseEntity = FindUseEntity();
	//variant_t emptyVariant;
	//// Found an object
	//if ( pUseEntity )
	//{
	//	pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_SHOOT );
	//}
}
