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
#include "player_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern int gEvilImpulse101;

//#define SDK_PLAYER_MODEL "models/player/terror.mdl"
//#define SDK_PLAYER_MODEL "models/player/brazil/brazil.mdl"		//need to precache one to start with

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

	//UpdateSprint();

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
	CBall *pBall = GetBall();

	if (!pBall)
		return;

	float yaw, pitch;

	if (GetFlags() & FL_REMOTECONTROLLED)
	{
		yaw = 0;
		pitch = 0;
	}
	else
	{
		float curyaw = GetBoneController(2);
		float curpitch = GetBoneController(3);
		Vector ballPos;
		pBall->VPhysicsGetObject()->GetPosition(&ballPos, NULL);
		Vector dirToBall = ballPos - Vector(GetLocalOrigin().x, GetLocalOrigin().y, GetLocalOrigin().z + VEC_VIEW.z);

		QAngle angToBall;
		VectorAngles(dirToBall, angToBall );

		yaw = angToBall[YAW] - GetLocalAngles()[YAW];
		pitch = angToBall[PITCH] - GetLocalAngles()[PITCH];

		if (yaw > 180) yaw -= 360;
		if (yaw < -180) yaw += 360;
		if (pitch > 180) pitch -= 360;
		if (pitch < -180) pitch += 360;

		if (dirToBall.Length2D() > 10.0f && yaw > -60 && yaw < 60) 
		{   
			//yaw = (curyaw > yaw ? -1 : 1) * 1000 * gpGlobals->frametime;
			yaw = curyaw + (yaw - curyaw) * 0.1f;
			pitch = curpitch + (pitch - curpitch) * 0.1f;
		}
		else 
		{
			//yaw = (curyaw > 0 ? -1 : 1) * 1000 * gpGlobals->frametime;
			yaw = curyaw * 0.9f;
			pitch = curpitch * 0.9f;
		}
	}

	SetBoneController(2, yaw);
	SetBoneController(3, pitch);
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
	PrecacheModel( SDK_PLAYER_MODEL );
	PrecacheScriptSound("Player.Oomph");
	PrecacheScriptSound("Player.DiveKeeper");
	PrecacheScriptSound("Player.Save");
	PrecacheScriptSound("Player.Goal");
	PrecacheScriptSound("Player.Slide");
	PrecacheScriptSound("Player.DivingHeader");
	PrecacheScriptSound("Player.Card");

	BaseClass::Precache();
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
	BaseClass::Spawn();

	//UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: Put the player in the specified team
//-----------------------------------------------------------------------------
//Tony; if we're not using actual teams, we don't need to override this.

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
		State_Transition( STATE_OBSERVER_MODE );
	}
	else // active player
	{
		if( iOldTeam == TEAM_SPECTATOR )
			SetMoveType( MOVETYPE_NONE );

		// transmit changes for player position right away
		g_pPlayerResource->UpdatePlayerData();

		if (iOldTeam != TEAM_A && iOldTeam != TEAM_B)
			State_Transition( STATE_ACTIVE );
	}
}

void CSDKPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();

	m_takedamage = DAMAGE_NO;
	//pl.deadflag = true;
	//m_lifeState = LIFE_DEAD;
	AddEffects( EF_NODRAW );
	//ChangeTeam( TEAM_UNASSIGNED );
	SetThink( NULL );
	InitSpeeds(); //Tony; initialize player speeds.
	SetModel( SDK_PLAYER_MODEL );	//Tony; basically, leave this alone ;) unless you're not using classes or teams, then you can change it to whatever.

	//SharedSpawn();
	Spawn();

	m_RejoinTime = 0;
	m_RedCards=0;
	m_YellowCards=0;
	m_Fouls=0;
	m_Goals=0;
	m_Assists=0;
	m_Passes=0;
	m_FreeKicks=0;
	m_Penalties=0;
	m_Corners=0;
	m_ThrowIns=0;
	m_KeeperSaves=0;
	m_GoalKicks=0;
	m_Possession=0;
	m_flPossessionTime=0.0f;
	ResetFragCount();

	//Spawn();

	//if (!IsBot())
		ChangeTeam(TEAM_SPECTATOR);

	/*if (!IsBot())
		ChangeTeam(
		State_Enter(STATE_OBSERVER_MODE);
	else
		State_Enter(STATE_ACTIVE);*/
}

void CSDKPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	m_PlayerAnimState->DoAnimationEvent( event, nData );
	TE_PlayerAnimEvent( this, event, nData );	// Send to any clients who can see this guy.
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
		{ STATE_ACTIVE,			"STATE_ACTIVE",			&CSDKPlayer::State_ACTIVE_Enter, NULL, &CSDKPlayer::State_ACTIVE_PreThink },
		{ STATE_WELCOME,		"STATE_WELCOME",		&CSDKPlayer::State_WELCOME_Enter, NULL, &CSDKPlayer::State_WELCOME_PreThink },
#if defined ( SDK_USE_TEAMS )
		{ STATE_PICKINGTEAM,	"STATE_PICKINGTEAM",	&CSDKPlayer::State_PICKINGTEAM_Enter, NULL,	&CSDKPlayer::State_WELCOME_PreThink },
#endif
#if defined ( SDK_USE_PLAYERCLASSES )
		{ STATE_PICKINGCLASS,	"STATE_PICKINGCLASS",	&CSDKPlayer::State_PICKINGCLASS_Enter, NULL,	&CSDKPlayer::State_WELCOME_PreThink },
#endif
		{ STATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CSDKPlayer::State_OBSERVER_MODE_Enter,	&CSDKPlayer::State_OBSERVER_MODE_Leave, &CSDKPlayer::State_OBSERVER_MODE_PreThink }
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
void CSDKPlayer::State_WELCOME_Enter()
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

void CSDKPlayer::State_WELCOME_PreThink()
{
	// Update whatever intro camera it's at.
	if( m_pIntroCamera && (gpGlobals->curtime >= m_fIntroCamTime) )
	{
		MoveToNextIntroCamera();
	}
}

void CSDKPlayer::State_OBSERVER_MODE_Enter()
{
	// Always start a spectator session in roaming mode
	m_iObserverLastMode = OBS_MODE_ROAMING;

	//if( m_hObserverTarget == NULL )
	//{
	//	// find a new observer target
	//	CheckObserverSettings();
	//}

	//// Change our observer target to the nearest teammate
	//CTeam *pTeam = GetGlobalTeam( GetTeamNumber() );

	//CBasePlayer *pPlayer;
	//Vector localOrigin = GetAbsOrigin();
	//Vector targetOrigin;
	//float flMinDist = FLT_MAX;
	//float flDist;

	//for ( int i=0;i<pTeam->GetNumPlayers();i++ )
	//{
	//	pPlayer = pTeam->GetPlayer(i);

	//	if ( !pPlayer )
	//		continue;

	//	if ( !IsValidObserverTarget(pPlayer) )
	//		continue;

	//	targetOrigin = pPlayer->GetAbsOrigin();

	//	flDist = ( targetOrigin - localOrigin ).Length();

	//	if ( flDist < flMinDist )
	//	{
	//		m_hObserverTarget.Set( pPlayer );
	//		flMinDist = flDist;
	//	}
	//}

	//pl.deadflag = true;
	//m_lifeState = LIFE_DEAD;
	AddEffects(EF_NODRAW);
	//ChangeTeam(TEAM_SPECTATOR);
	SetMoveType(MOVETYPE_OBSERVER);
	AddSolidFlags(FSOLID_NOT_SOLID);

	StartObserverMode( m_iObserverLastMode );
	PhysObjectSleep();
}

void CSDKPlayer::State_OBSERVER_MODE_Leave()
{
	StopObserverMode();
}

void CSDKPlayer::State_OBSERVER_MODE_PreThink()
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
void CSDKPlayer::State_PICKINGCLASS_Enter()
{
	ShowClassSelectMenu();
	PhysObjectSleep();

}
#endif // SDK_USE_PLAYERCLASSES

#if defined ( SDK_USE_TEAMS )
void CSDKPlayer::State_PICKINGTEAM_Enter()
{
	ShowViewPortPanel( PANEL_TEAM );
	PhysObjectSleep();

}
#endif // SDK_USE_TEAMS

void CSDKPlayer::State_ACTIVE_Enter()
{
	SetMoveType( MOVETYPE_WALK );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
    m_Local.m_iHideHUD = 0;
	PhysObjectWake();

	AddSolidFlags( FSOLID_NOT_STANDABLE );
	m_Shared.SetStamina( 100 );
	m_bTeamChanged = false;
	InitSprinting();
	// update this counter, used to not interp players when they spawn
	m_bSpawnInterpCounter = !m_bSpawnInterpCounter;
	SetContextThink( &CSDKPlayer::SDKPushawayThink, gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, SDK_PUSHAWAY_THINK_CONTEXT );
	//pl.deadflag = false;
	m_flNextShot = gpGlobals->curtime;
	m_hRagdoll = NULL;
	//m_lifeState = LIFE_ALIVE;
	RemoveEffects(EF_NODRAW);

	SetViewOffset( VEC_VIEW );

	SDKGameRules()->GetPlayerSpawnSpot(this);

	//Tony; call spawn again now -- remember; when we add respawn timers etc, to just put them into the spawn queue, and let the queue respawn them.
	//Spawn();
	//RemoveEffects(EF_NODRAW); //ios hack - player spawns invisible sometimes
}

void CSDKPlayer::State_ACTIVE_PreThink()
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

bool CSDKPlayer::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	// No need to lag compensate at all if we're not attacking in this command and
	// we haven't attacked recently.
	if ( !( pCmd->buttons & IN_ATTACK ) && (pCmd->command_number - m_iLastWeaponFireUsercmd > 5) )
		return false;

	return BaseClass::WantsLagCompensationOnEntity( pPlayer, pCmd, pEntityTransmitBits );
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


	/*if (m_KickDelay > gpGlobals->curtime)
		return;
	else if (m_TeamPos>=1 && m_TeamPos<=11)
		RemoveFlag(FL_FROZEN);*/


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
		if ( args.ArgC() < 3)
		{
			Warning( "Player sent bad jointeam syntax\n" );
			return false;	//go away
		}

		int nTeam = clamp(atoi(args[1]), TEAM_SPECTATOR, TEAM_B);

		if (nTeam == TEAM_SPECTATOR)
		{
			//m_TeamPos = 1;
			//ConvertSpawnToShirt();
			//RemoveFlag(FL_FROZEN);
			ChangeTeam(TEAM_SPECTATOR);
		}
		else
		{
			int posWanted = clamp(atoi(args[2]), 1, 11);
			if (TeamPosFree(nTeam, posWanted, true))
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

				ChangeTeam(nTeam);

				//spawn at correct position
				//Spawn();
				//SDKGameRules()->GetPlayerSpawnSpot( this );
				//RemoveEffects( EF_NODRAW );
				//SetSolid( SOLID_BBOX );
				//RemoveFlag(FL_FROZEN);
			}
			else
			{
				ShowViewPortPanel(PANEL_TEAM);
			}
		}
		return true;
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
	for (int i = 1; i <= gpGlobals->maxClients; i++)	
	{
		CSDKPlayer *pPl = (CSDKPlayer*)UTIL_PlayerByIndex(i);

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (pPl->GetTeamNumber() == team && pPl->GetTeamPosition() == pos)
		{
			if (IsBot())
				return false;

			if (!pPl->IsBot() || !kickBotKeeper)
				return false;

			char kickcmd[512];
			Q_snprintf(kickcmd, sizeof(kickcmd), "kickid %i Human player taking the spot\n", pPl->GetUserID());
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
	char *model = "models/player/player.mdl";
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
	State_Transition(STATE_OBSERVER_MODE);
	/*
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
	*/
	if (!IsBot() && m_TeamPos == 1 && botkeepers.GetBool())
	{
		//RomD: Add bot keeper if killed player was a keeper
		BotPutInServer(false, GetTeamNumber() == TEAM_A ? 1 : 2); 
	}
}

/////////////////////////////////////////////////////////////////////////////
//slidetackle - not really related to Duck (which is in gamemovement.cpp)
//
void CSDKPlayer::Duck(void)
{
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

void CSDKPlayer::ResetMatchStats()
{
	m_RedCards = 0;
	m_YellowCards = 0;
	m_Fouls = 0;
	m_Goals = 0;
	m_Assists = 0;
	m_Possession = 0;
	m_Passes = 0;
	m_FreeKicks = 0;
	m_Penalties = 0;
	m_Corners = 0;
	m_ThrowIns = 0;
	m_KeeperSaves = 0;
	m_GoalKicks = 0;
	m_flPossessionTime = 0.0f;

	ResetFragCount();
}

Vector CSDKPlayer::EyeDirection2D( void )
{
	Vector vecReturn = EyeDirection3D();
	vecReturn.z = 0;
	vecReturn.AsVector2D().NormalizeInPlace();

	return vecReturn;
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CSDKPlayer::EyeDirection3D( void )
{
	Vector vecForward;
	AngleVectors( EyeAngles(), &vecForward );
	return vecForward;
}

void CSDKPlayer::SetPosInsideShield(Vector pos, bool holdAtTargetPos)
{
	RemoveFlag(FL_SHIELD_KEEP_OUT);
	AddFlag(FL_REMOTECONTROLLED | FL_SHIELD_KEEP_IN);
	m_vTargetPos = pos;
	m_bIsAtTargetPos = false;
	m_bHoldAtTargetPos = holdAtTargetPos;
	SetMoveType(MOVETYPE_NOCLIP);
}

void CSDKPlayer::SetPosOutsideShield(bool holdAtTargetPos)
{
	RemoveFlag(FL_SHIELD_KEEP_IN);
	AddFlag(FL_SHIELD_KEEP_OUT);
	m_bHoldAtTargetPos = holdAtTargetPos;
	m_bIsAtTargetPos = false;

	switch (SDKGameRules()->m_nShieldType)
	{
	case SHIELD_THROWIN: case SHIELD_FREEKICK: case SHIELD_CORNER:
		{
			Vector dir = GetLocalOrigin() - SDKGameRules()->m_vShieldPos;
			dir.z = 0;
			if (dir.Length2D() >= (SDKGameRules()->m_nShieldRadius + 2 * VEC_HULL_MAX.x))
			{
				m_bIsAtTargetPos = true;
			}
			else
			{
				AddFlag(FL_REMOTECONTROLLED);
				if (dir.Length2D() == 0)
				{
					dir = SDKGameRules()->m_vKickOff - GetLocalOrigin();
					dir.z = 0;
				}
				dir.NormalizeInPlace();
				Vector pos = SDKGameRules()->m_vShieldPos + dir * (SDKGameRules()->m_nShieldRadius + 2 * VEC_HULL_MAX.x);

				float threshold = 0;//150;
				Vector min = SDKGameRules()->m_vFieldMin - threshold;
				Vector max = SDKGameRules()->m_vFieldMax + threshold;

				if (pos.x < min.x || pos.y < min.y || pos.x > max.x || pos.y > max.y)
				{
					pos = SDKGameRules()->m_vShieldPos - dir * (SDKGameRules()->m_nShieldRadius + 2 * VEC_HULL_MAX.x);
				}

				m_vTargetPos = pos;
				m_bIsAtTargetPos = false;
				SetMoveType(MOVETYPE_NOCLIP);
			}
		}
		break;
	case SHIELD_GOALKICK:
		{
			float threshold = 2 * VEC_HULL_MAX.x;
			Vector min = GetGlobalTeam(SDKGameRules()->m_nShieldSide)->m_vPenBoxMin - threshold;
			Vector max = GetGlobalTeam(SDKGameRules()->m_nShieldSide)->m_vPenBoxMax + threshold;
			Vector pos = GetLocalOrigin();
			bool isInsideBox = pos.x > min.x && pos.y > min.y && pos.x < max.x && pos.y < max.y;

			if (!isInsideBox)
			{
				m_bIsAtTargetPos = true;
			}
			else
			{
				AddFlag(FL_REMOTECONTROLLED);
				float targetPosY = GetGlobalTeam(SDKGameRules()->m_nShieldSide)->m_nForward == 1 ? max.y : min.y;
				m_vTargetPos = Vector(pos.x, targetPosY, SDKGameRules()->m_vKickOff.GetZ());
				m_bIsAtTargetPos = false;
				SetMoveType(MOVETYPE_NOCLIP);
			}
		}
		break;
	case SHIELD_KICKOFF:
		{
			AddFlag(FL_REMOTECONTROLLED);
			m_vTargetPos = GetTeam()->m_vPlayerSpawns[GetTeamPosition() - 1];
			m_bIsAtTargetPos = false;
			SetMoveType(MOVETYPE_NOCLIP);
		}
		break;
	case SHIELD_PENALTY:
		break;
	}
}

bool CSDKPlayer::IsOnField(CSDKPlayer *pPl)
{
	return (pPl && pPl->IsConnected() && (pPl->GetTeamNumber() == TEAM_A || pPl->GetTeamNumber() == TEAM_B));
}

bool CSDKPlayer::IsOffside()
{
	return mp_offside.GetBool() ? m_bOffside : false;
}

void CSDKPlayer::SetOffside(bool offside)
{
	if (offside)
		m_vOffsidePos = GetLocalOrigin();

	m_bOffside = offside;
}

Vector CSDKPlayer::GetOffsidePos()
{
	return m_vOffsidePos;
}