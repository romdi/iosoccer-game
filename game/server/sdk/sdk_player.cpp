//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "sdk_player.h"
#include "team.h"
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
#include "ios_replaymanager.h"
#include "Filesystem.h"
#include "client.h"
#include <time.h>

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
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, bool sendToPlayerClient )
{
	CPVSFilter filter( (const Vector&)pPlayer->EyePosition() );

	//Tony; pull the player who is doing it out of the recipientlist, this is predicted!!
	//ios: this may come from the ball -> not predicted
	if (!sendToPlayerClient)
		filter.RemoveRecipient( pPlayer );

	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
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
	SendPropFloat( SENDINFO( m_flMaxStamina ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#endif

#if defined ( SDK_USE_PRONE )
	SendPropBool( SENDINFO( m_bProne ) ),
	SendPropTime( SENDINFO( m_flGoProneTime ) ),
	SendPropTime( SENDINFO( m_flUnProneTime ) ),
#endif
#if defined ( SDK_USE_SPRINTING )
	SendPropBool( SENDINFO( m_bIsSprinting ) ),
#endif

	SendPropTime( SENDINFO( m_flNextJump ) ),
	SendPropTime( SENDINFO( m_flNextSlide) ),

	SendPropBool( SENDINFO( m_bJumping ) ),
	SendPropBool( SENDINFO( m_bFirstJumpFrame ) ),
	SendPropTime( SENDINFO( m_flJumpStartTime ) ),

	SendPropBool( SENDINFO( m_bIsShotCharging ) ),
	SendPropBool( SENDINFO( m_bDoChargedShot ) ),
	SendPropTime( SENDINFO( m_flShotChargingStart ) ),
	SendPropTime( SENDINFO( m_flShotChargingDuration ) ),
	SendPropInt( SENDINFO( m_ePlayerAnimEvent ) ),
	SendPropTime( SENDINFO( m_flPlayerAnimEventStartTime ) ),
	SendPropVector( SENDINFO( m_aPlayerAnimEventStartAngle ) ),
	SendPropInt( SENDINFO( m_nPlayerAnimEventStartButtons ) ),

	SendPropInt( SENDINFO( m_nLastPressedSingleMoveKey ) ),

	SendPropDataTable( "sdksharedlocaldata", 0, &REFERENCE_SEND_TABLE(DT_SDKSharedLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
END_SEND_TABLE()
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

BEGIN_SEND_TABLE_NOBASE( CSDKPlayer, DT_SDKLocalPlayerExclusive )
	//SendPropInt( SENDINFO( m_iShotsFired ), 8, SPROP_UNSIGNED ),
	// send a hi-res origin to the local player for use in prediction
	//new ios1.1 we need this for free roaming mode - do not remove!
    SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	//new
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
//	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),

//ios	SendPropInt( SENDINFO( m_ArmorValue ), 8, SPROP_UNSIGNED ),
	SendPropInt(SENDINFO(m_nInPenBoxOfTeam), 3),
	SendPropVector(SENDINFO(m_vTargetPos), -1, SPROP_COORD),
	SendPropBool(SENDINFO(m_bIsAtTargetPos)),
	SendPropBool(SENDINFO(m_bHoldAtTargetPos)),
	SendPropTime( SENDINFO( m_flNextClientSettingsChangeTime ) ),
	SendPropTime(SENDINFO(m_flNextJoin)),
	SendPropBool(SENDINFO(m_bShotButtonsReleased)),
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
	SendPropTime( SENDINFO( m_flStateEnterTime )),

	SendPropBool( SENDINFO( m_bSpawnInterpCounter ) ),

	SendPropInt(SENDINFO(m_nModelScale), 8, SPROP_UNSIGNED),
	SendPropEHandle(SENDINFO(m_pHoldingBall))
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


void CC_MP_PlayerModelScale(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (args.ArgC() < 3)
	{
		Msg("Usage: mp_player_model_scale <userid> <scale 0-255>\n");
		return;
	}

	CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByUserId(atoi(args[1])));

	if (!pPl)
	{
		Msg("Player not found.\n");
		return;
	}

	int scale = atoi(args[2]);

	if (scale < 0)
	{
		Msg("Scale too small.\n");
		return;
	}

	if (scale > 255)
	{
		Msg("Scale too big.\n");
		return;
	}
	
	int oldScale = pPl->m_nModelScale;

	pPl->m_nModelScale = scale;

	UTIL_ClientPrintAll(HUD_PRINTTALK, "#game_player_model_scale", pPl->GetPlayerName(), scale == 100 ? "is back to normal size" : (scale - oldScale > 0 ? "grows..." : "shrinks..."));
}

ConCommand mp_player_model_scale("mp_player_model_scale", CC_MP_PlayerModelScale, "", 0);


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

	m_hRagdoll = NULL;

	m_angEyeAngles.Init();

	m_pCurStateInfo = NULL;	// no state yet
	m_bShotButtonsReleased = false;
	m_nTeamToJoin = TEAM_INVALID;
	m_nTeamPosIndexToJoin = 0;
	m_nSpecTeamToJoin = TEAM_SPECTATOR;
	m_nTeamPosIndex = 0;
	m_nPreferredOutfieldShirtNumber = 2;
	m_nPreferredKeeperShirtNumber = 1;
	m_szPlayerBallSkinName[0] = '\0';
	m_pPlayerBall = NULL;
	m_Shared.m_flPlayerAnimEventStartTime = gpGlobals->curtime;
	m_Shared.m_ePlayerAnimEvent = PLAYERANIMEVENT_NONE;
	m_Shared.m_aPlayerAnimEventStartAngle = vec3_origin;
	m_Shared.m_nPlayerAnimEventStartButtons = 0;
	m_nInPenBoxOfTeam = TEAM_INVALID;
	m_nModelScale = 100;
	m_nSpecTeam = TEAM_SPECTATOR;
	m_ePenaltyState = PENALTY_NONE;
	m_pHoldingBall = NULL;
	m_flNextClientSettingsChangeTime = gpGlobals->curtime;
	m_bLegacySideCurl = false;
	m_bInvertKeeperSprint = true;
	m_bJoinSilently = false;

	m_szPlayerName[0] = '\0';
	m_szClubName[0] = '\0';
	m_szShirtName[0] = '\0';

	m_nSkinIndex = 0;

	m_iPlayerState = PLAYER_STATE_NONE;

	m_pData = NULL;
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
	// Check if player is away
	if (SDKGameRules()->IsIntermissionState() && (GetTeamNumber() == TEAM_A || GetTeamNumber() == TEAM_B))
	{
		bool isActive = ((m_nButtons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP | IN_DUCK)) != 0);

		// Bots are never away
		if (isActive || IsBot())
		{
			m_flLastMoveTime = gpGlobals->curtime;
			m_bIsAway = false;
		}
		else if (gpGlobals->curtime >= m_flLastMoveTime + sv_awaytime_warmup.GetFloat())
		{
			m_bIsAway = true;

			if (gpGlobals->curtime >= m_flLastMoveTime + sv_awaytime_warmup_autospec.GetFloat())
			{
				SetDesiredTeam(TEAM_SPECTATOR, GetTeamNumber(), 0, true, false, false);
				UTIL_ClientPrintAll(HUD_PRINTTALK, "#game_player_benched_away", GetPlayerName());
			}
		}
	}

	// Reset the block time when the card ban ends
	if (GetNextCardJoin() != 0 && !SDKGameRules()->IsIntermissionState() && SDKGameRules()->GetMatchDisplayTimeSeconds() >= GetNextCardJoin())
	{
		SetNextCardJoin(0);
	}

	// Prevent the player from reserving a position if he's card banned or if the position is blocked
	if (!SDKGameRules()->IsIntermissionState()
		&& (GetTeamToJoin() == TEAM_A || GetTeamToJoin() == TEAM_B)
		&& (SDKGameRules()->GetMatchDisplayTimeSeconds() < GetNextCardJoin()
		|| SDKGameRules()->GetMatchDisplayTimeSeconds() < GetGlobalTeam(GetTeamToJoin())->GetPosNextJoinSeconds(GetTeamPosIndexToJoin())))
	{
		m_nTeamToJoin = TEAM_INVALID;
	}

	// Let the player take the desired position
	if (m_nTeamToJoin != TEAM_INVALID
		&& gpGlobals->curtime >= GetNextJoin()
		&& (SDKGameRules()->IsIntermissionState()	
			|| SDKGameRules()->GetMatchDisplayTimeSeconds() >= GetNextCardJoin()
			&& SDKGameRules()->GetMatchDisplayTimeSeconds() >= GetGlobalTeam(m_nTeamToJoin)->GetPosNextJoinSeconds(m_nTeamPosIndexToJoin)))
	{
		bool canJoin = true;

		CSDKPlayer *pSwapPartner = NULL;

		if (!IsTeamPosFree(m_nTeamToJoin, m_nTeamPosIndexToJoin, false, &pSwapPartner))
		{
			if (pSwapPartner && pSwapPartner->IsBot())
			{
				char kickcmd[512];
				Q_snprintf(kickcmd, sizeof(kickcmd), "kickid %i Human player taking the position\n", pSwapPartner->GetUserID());
				engine->ServerCommand(kickcmd);
				pSwapPartner = NULL;
				canJoin = true;
			}
			else if (pSwapPartner && pSwapPartner->GetTeamToJoin() == GetTeamNumber() && pSwapPartner->GetTeamPosIndexToJoin() == GetTeamPosIndex())
				canJoin = true;
			else
				canJoin = false;
		}

		if (canJoin)
		{
			Vector partnerPos = vec3_invalid;

			if (pSwapPartner)
			{
				partnerPos = pSwapPartner->GetLocalOrigin();
				pSwapPartner->ChangeTeam();
				pSwapPartner->SetLocalOrigin(GetLocalOrigin());
			}

			ChangeTeam();

			if (pSwapPartner)
			{
				SetLocalOrigin(partnerPos);
			}
		}
	}

	State_PreThink();

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
	
	//LookAtBall();

	//m_BallInPenaltyBox = -1;
}

void CSDKPlayer::LookAtBall(void)
{
	CBall *pBall = GetNearestBall(GetLocalOrigin());

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
	// FIXME: Head pitch movement is jerky
	//SetBoneController(3, pitch);
}

void CSDKPlayer::Precache()
{
	PrecacheModel( SDK_PLAYER_MODEL );
	PrecacheScriptSound("Player.Oomph");
	PrecacheScriptSound("Player.DiveKeeper");
	PrecacheScriptSound("Player.Slide");
	PrecacheScriptSound("Player.DivingHeader");

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

	m_Shared.m_flNextJump = gpGlobals->curtime;
	m_Shared.m_flNextSlide = gpGlobals->curtime;

	AddSolidFlags(FSOLID_NOT_STANDABLE);

	//UseClientSideAnimation();
}

bool CSDKPlayer::SetDesiredTeam(int desiredTeam, int desiredSpecTeam, int desiredPosIndex, bool switchInstantly, bool setNextJoinDelay, bool silent)
{
	m_nTeamToJoin = desiredTeam;
	m_nTeamPosIndexToJoin = desiredPosIndex;
	m_nSpecTeamToJoin = desiredSpecTeam;
	m_bJoinSilently = silent;

	if (setNextJoinDelay)
		SetNextJoin(gpGlobals->curtime + mp_joindelay.GetFloat());
	else
		SetNextJoin(gpGlobals->curtime);

	if (switchInstantly)
		ChangeTeam();

	return true;
}

void CSDKPlayer::ChangeTeam()
{
	if (m_nTeamToJoin == GetTeamNumber() && m_nTeamPosIndexToJoin == GetTeamPosIndex() && m_nSpecTeamToJoin == m_nSpecTeam)
	{
		m_nTeamToJoin = TEAM_INVALID;
		m_nTeamPosIndexToJoin = 0;
		m_nSpecTeamToJoin = TEAM_INVALID;
		//m_bSetNextJoinDelay = false;

		return;
	}

	GetPlayerData()->EndCurrentMatchPeriod();

	if (GetTeam())
		GetTeam()->RemovePlayer(this);

	if (m_nTeamToJoin != TEAM_UNASSIGNED)
		GetGlobalTeam(m_nTeamToJoin)->AddPlayer(this, m_nTeamPosIndexToJoin);

	int oldTeam = GetTeamNumber();
	SetTeamNumber(m_nTeamToJoin);
	m_nTeamToJoin = TEAM_INVALID;

	int oldPosIndex = m_nTeamPosIndex;
	int oldPosType = GetTeamPosType();
	m_nTeamPosIndex = m_nTeamPosIndexToJoin;
	m_nTeamPosIndexToJoin = 0;

	int oldSpecTeam = m_nSpecTeam;
	m_nSpecTeam = m_nSpecTeamToJoin;
	m_nSpecTeamToJoin = TEAM_INVALID;

	// update client state 
	if (GetTeamNumber() == TEAM_UNASSIGNED || GetTeamNumber() == TEAM_SPECTATOR)
	{
		ResetFlags();

		if (State_Get() != PLAYER_STATE_OBSERVER_MODE)
		{
			State_Transition(PLAYER_STATE_OBSERVER_MODE);
		}
	}
	else // active player
	{
		if (!SDKGameRules()->IsIntermissionState())
			GetPlayerData()->StartNewMatchPeriod();

		m_nShirtNumber = FindAvailableShirtNumber();

		if (GetTeamPosType() == POS_GK)
			m_nBody = MODEL_KEEPER;
		else
			m_nBody = MODEL_PLAYER;

		ResetFlags();

		if (State_Get() != PLAYER_STATE_ACTIVE)
			State_Transition(PLAYER_STATE_ACTIVE);
		
		Vector dir = Vector(0, GetTeam()->m_nForward, 0);
		QAngle ang;
		VectorAngles(dir, ang);
		SetLocalVelocity(vec3_origin);
		SetLocalAngles(ang);
		SetLocalOrigin(GetSpawnPos(true));
		SnapEyeAngles(ang);

		if (SDKGameRules()->m_nShieldType != SHIELD_NONE)
			SetPosOutsideShield(true);
	}

	IGameEvent *event = gameeventmanager->CreateEvent("player_team");
	if (event)
	{
		event->SetInt("userid", GetUserID());
		event->SetInt("newteam", GetTeamNumber());
		event->SetInt("oldteam", oldTeam);
		event->SetBool("disconnect", IsDisconnecting());
		event->SetString("name", GetPlayerName());
		event->SetInt("newteampos", GetTeamPosIndex());
		event->SetInt("oldteampos", oldPosIndex);
		event->SetInt("newspecteam", m_nSpecTeam);
		event->SetInt("oldspecteam", oldSpecTeam);
		event->SetInt("maxplayers", (SDKGameRules()->UseOldMaxplayers() ? SDKGameRules()->GetOldMaxplayers() : mp_maxplayers.GetInt()));
		event->SetBool("silent", m_bJoinSilently);

		gameeventmanager->FireEvent( event );
	}

	g_pPlayerResource->UpdatePlayerData();
}

void CSDKPlayer::SetPreferredOutfieldShirtNumber(int num)
{
	m_nPreferredOutfieldShirtNumber = clamp(num, 2, 99);
	m_nShirtNumber = FindAvailableShirtNumber();
}

void CSDKPlayer::SetPreferredKeeperShirtNumber(int num)
{
	m_nPreferredKeeperShirtNumber = clamp(num, 1, 99);
	m_nShirtNumber = FindAvailableShirtNumber();
}

bool IsFreeShirtNumber(int nums[], int num)
{
	for (int i = 0; i < 11; i++)
	{
		if (nums[i] == num)
			return false;
	}

	return true;
}

int CSDKPlayer::FindAvailableShirtNumber()
{
	if (!mp_custom_shirt_numbers.GetBool())
		return (int)GetTeam()->GetFormation()->positions[GetTeamPosIndex()]->number;

	int shirtNumbers[11] = {};

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl) || pPl == this || pPl->GetTeam() != GetTeam())
			continue;

		shirtNumbers[pPl->GetTeamPosIndex()] = pPl->GetShirtNumber();
	}

	int newNumber = GetTeamPosType() == POS_GK ? m_nPreferredKeeperShirtNumber : m_nPreferredOutfieldShirtNumber;

	if (!IsFreeShirtNumber(shirtNumbers, newNumber))
	{
		newNumber = (int)GetTeam()->GetFormation()->positions[GetTeamPosIndex()]->number;

		if (!IsFreeShirtNumber(shirtNumbers, newNumber))
		{
			int startNumber = GetTeamPosType() == POS_GK ? 1 : 2;

			for (int i = startNumber; i <= 99; i++)
			{
				if (IsFreeShirtNumber(shirtNumbers, i))
				{
					newNumber = i;
					break;
				}
			}
		}
	}

	return newNumber;
}

void CSDKPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();

	m_takedamage = DAMAGE_NO;
	AddEffects( EF_NODRAW );
	SetThink( NULL );
	InitSpeeds(); //Tony; initialize player speeds.
	SetModel( SDK_PLAYER_MODEL );	//Tony; basically, leave this alone ;) unless you're not using classes or teams, then you can change it to whatever.
	Spawn();
	SetDesiredTeam(TEAM_SPECTATOR, TEAM_SPECTATOR, 0, true, false, false);
}

void CSDKPlayer::DoServerAnimationEvent(PlayerAnimEvent_t event)
{
	m_PlayerAnimState->DoAnimationEvent( event );
	//m_Shared.DoAnimationEvent( event );
	TE_PlayerAnimEvent( this, event, true );	// Send to any clients who can see this guy.
}

void CSDKPlayer::DoAnimationEvent(PlayerAnimEvent_t event)
{
	m_PlayerAnimState->DoAnimationEvent( event );
	//m_Shared.DoAnimationEvent( event );
	TE_PlayerAnimEvent( this, event, false );	// Send to any clients who can see this guy.
}

void CSDKPlayer::CheatImpulseCommands( int iImpulse )
{
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
	m_flStateEnterTime = gpGlobals->curtime;
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
		{ PLAYER_STATE_ACTIVE,			"STATE_ACTIVE",			&CSDKPlayer::State_ACTIVE_Enter, &CSDKPlayer::State_ACTIVE_PreThink, &CSDKPlayer::State_ACTIVE_Leave },
		{ PLAYER_STATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CSDKPlayer::State_OBSERVER_MODE_Enter, &CSDKPlayer::State_OBSERVER_MODE_PreThink, &CSDKPlayer::State_OBSERVER_MODE_Leave }
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

void CSDKPlayer::State_OBSERVER_MODE_Enter()
{
	AddEffects(EF_NODRAW);
	SetMoveType(MOVETYPE_NONE);
	AddSolidFlags(FSOLID_NOT_SOLID);
	PhysObjectSleep();
	SetLocalOrigin(SDKGameRules()->m_vKickOff + Vector(0, 0, 100));
	//SetViewOffset(vec3_origin);
	//SetViewOffset(VEC_VIEW);
	SetGroundEntity(NULL);
}

void CSDKPlayer::State_OBSERVER_MODE_PreThink()
{
}

void CSDKPlayer::State_OBSERVER_MODE_Leave()
{
}

void CSDKPlayer::State_ACTIVE_Enter()
{
	SetMoveType(MOVETYPE_WALK);
	RemoveSolidFlags(FSOLID_NOT_SOLID);
    //m_Local.m_iHideHUD = 0;
	PhysObjectWake();
	// update this counter, used to not interp players when they spawn
	m_bSpawnInterpCounter = !m_bSpawnInterpCounter;
	//SetContextThink( &CSDKPlayer::SDKPushawayThink, gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, SDK_PUSHAWAY_THINK_CONTEXT );
	RemoveEffects(EF_NODRAW);
	SetViewOffset(VEC_VIEW);
}

void CSDKPlayer::State_ACTIVE_PreThink()
{
	if (!ShotButtonsPressed())
		SetShotButtonsReleased(true);

	CheckShotCharging();
	CheckLastPressedSingleMoveButton();
}

void CSDKPlayer::State_ACTIVE_Leave()
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
}

//ConVar mp_autobalance( "mp_autobalance", "1", FCVAR_REPLICATED|FCVAR_NOTIFY, "autobalance teams after a goal. blocks joining unbalanced teams" );

bool CSDKPlayer::ClientCommand( const CCommand &args )
{
	if (!Q_stricmp(args[0], "jointeam")) 
	{
		if (args.ArgC() < 3)
		{
			Warning("Player sent bad jointeam syntax\n");
			return true;	//go away
		}

		int team = atoi(args[1]);
		int posIndex = atoi(args[2]);

		if (posIndex < 0 || posIndex > mp_maxplayers.GetInt() - 1 || team < TEAM_SPECTATOR || team > TEAM_B)
			return true;

		// Player is card banned or position is blocked due to a card ban
		if (!SDKGameRules()->IsIntermissionState()
			&& (SDKGameRules()->GetMatchDisplayTimeSeconds() < GetNextCardJoin()
				|| SDKGameRules()->GetMatchDisplayTimeSeconds() < GetGlobalTeam(team)->GetPosNextJoinSeconds(posIndex)))
		{
			return true;
		}

		CSDKPlayer *pSwapPartner = NULL;

		// Notify the previous swap partner that the player is cancelling his swap request
		if ((m_nTeamToJoin == TEAM_A || m_nTeamToJoin == TEAM_B)
			&& !IsTeamPosFree(m_nTeamToJoin, m_nTeamPosIndexToJoin, true, &pSwapPartner)
			&& pSwapPartner)
		{
			char *partnerMsg;
			char *msg;

			if (GetTeamNumber() == TEAM_SPECTATOR)
			{
				partnerMsg = "#game_player_cancel_pos_swap_spec";
				msg = "#game_player_cancel_pos_swap_spec_pov";
			}
			else
			{
				partnerMsg = "#game_player_cancel_pos_swap_field";
				msg = "#game_player_cancel_pos_swap_field_pov";
			}

			ClientPrint(pSwapPartner, HUD_PRINTTALK, partnerMsg, GetPlayerName());
			ClientPrint(this, HUD_PRINTTALK, msg, pSwapPartner->GetPlayerName());
		}

		// Cancel the request if the desired position is the current position or the current target position
		if (team == GetTeamNumber() && posIndex == GetTeamPosIndex() || team == m_nTeamToJoin && posIndex == m_nTeamPosIndexToJoin)
		{
			m_nTeamToJoin = TEAM_INVALID;
			return true;
		}

		// Notify the player on the target position that this player wants to swap
		if ((team == TEAM_A || team == TEAM_B)
			&& !IsTeamPosFree(team, posIndex, true, &pSwapPartner)
			&& pSwapPartner)
		{
			char *partnerMsg;
			char *msg;

			if (GetTeamNumber() == TEAM_SPECTATOR)
			{
				partnerMsg = "#game_player_pos_swap_spec";
				msg = "#game_player_pos_swap_spec_pov";
			}
			else
			{
				partnerMsg = "#game_player_pos_swap_field";
				msg = "#game_player_pos_swap_field_pov";
			}

			ClientPrint(pSwapPartner, HUD_PRINTTALK, partnerMsg, GetPlayerName());
			ClientPrint(this, HUD_PRINTTALK, msg, pSwapPartner->GetPlayerName());
		}

		return SetDesiredTeam(team, team, posIndex, false, true, false);
	}
	else if (!Q_stricmp(args[0], "spectate"))
	{
		int specTeam = ((args.ArgC() < 2) ? TEAM_SPECTATOR : atoi(args[1]));

		if (GetTeamNumber() == TEAM_SPECTATOR && m_nSpecTeam == specTeam)
			return true;

		bool switchInstantly = (GetTeamNumber() != TEAM_SPECTATOR);
		SetDesiredTeam(TEAM_SPECTATOR, specTeam, 0, switchInstantly, true, false);

		return true;
	}
	else if (!Q_stricmp(args[0], "requesttimeout"))
	{
		if (SDKGameRules()->IsIntermissionState()
			|| this != GetTeam()->GetCaptain()
			|| SDKGameRules()->GetTimeoutState() != TIMEOUT_STATE_NONE
			|| GetTeam()->GetTimeoutTimeLeft() <= 0
			|| GetTeam()->GetTimeoutsLeft() <= 0)
			return true;

		SDKGameRules()->SetTimeoutState(TIMEOUT_STATE_PENDING);
		SDKGameRules()->SetTimeoutTeam(GetTeamNumber());
		GetTeam()->SetTimeoutsLeft(GetTeam()->GetTimeoutsLeft() - 1);

		IGameEvent *pEvent = gameeventmanager->CreateEvent("timeout_pending");
		if (pEvent)
		{
			pEvent->SetInt("requesting_team", GetTeamNumber());
			gameeventmanager->FireEvent(pEvent);
		}

		UTIL_ClientPrintAll(HUD_PRINTCENTER, "Timeout pending");

		return true;
	}
	else if (!Q_stricmp(args[0], "endtimeout"))
	{
		if (SDKGameRules()->IsIntermissionState()
			|| this != GetTeam()->GetCaptain()
			|| SDKGameRules()->GetTimeoutState() == TIMEOUT_STATE_NONE
			|| SDKGameRules()->GetTimeoutTeam() != GetTeamNumber())
			return true;

		SDKGameRules()->EndTimeout();

		return true;
	}
	else if (!Q_stricmp(args[0], "togglecaptaincy"))
	{
		if (GetTeamNumber() != TEAM_A && GetTeamNumber() != TEAM_B)
			return true;

		if (this == GetTeam()->GetCaptain())
			GetTeam()->SetCaptainPosIndex(-1);
		else
			GetTeam()->SetCaptainPosIndex(GetTeamPosIndex());

		return true;
	}
	else if (!Q_stricmp(args[0], "setfreekicktaker"))
	{
		if (this != GetTeam()->GetCaptain())
			return true;

		int posIndex = atoi(args[1]);

		// Toggle off
		if (GetTeam()->GetFreekickTakerPosIndex() == posIndex)
			posIndex = -1;

		GetTeam()->SetFreekickTakerPosIndex(posIndex);
		return true;
	}
	else if (!Q_stricmp(args[0], "setpenaltytaker"))
	{
		if (this != GetTeam()->GetCaptain())
			return true;

		int posIndex = atoi(args[1]);

		// Toggle off
		if (GetTeam()->GetPenaltyTakerPosIndex() == posIndex)
			posIndex = -1;

		GetTeam()->SetPenaltyTakerPosIndex(posIndex);
		return true;
	}
	else if (!Q_stricmp(args[0], "setleftcornertaker"))
	{
		if (this != GetTeam()->GetCaptain())
			return true;

		int posIndex = atoi(args[1]);

		// Toggle off
		if (GetTeam()->GetLeftCornerTakerPosIndex() == posIndex)
			posIndex = -1;

		GetTeam()->SetLeftCornerTakerPosIndex(posIndex);
		return true;
	}
	else if (!Q_stricmp(args[0], "setrightcornertaker"))
	{
		if (this != GetTeam()->GetCaptain())
			return true;

		int posIndex = atoi(args[1]);

		// Toggle off
		if (GetTeam()->GetRightCornerTakerPosIndex() == posIndex)
			posIndex = -1;

		GetTeam()->SetRightCornerTakerPosIndex(posIndex);
		return true;
	}
	else if (!Q_stricmp(args[0], "formation"))
	{
		if (args.ArgC() < 2)
			return true;

		if (this != GetTeam()->GetCaptain())
			return true;

		GetTeam()->SetFormationIndex(atoi(args[1]), false);
		return true;
	}
	else if (!Q_stricmp(args[0], "quicktactic"))
	{
		if (args.ArgC() < 2)
			return true;

		if (this != GetTeam()->GetCaptain())
			return true;

		QuickTactics_t quickTactic = (QuickTactics_t)atoi(args[1]);

		if (quickTactic == QUICKTACTIC_NONE || quickTactic == GetTeam()->GetQuickTactic())
			GetTeam()->SetQuickTactic(QUICKTACTIC_NONE);
		else
			GetTeam()->SetQuickTactic(quickTactic);

		return true;
	}

	return BaseClass::ClientCommand (args);
}

bool CSDKPlayer::IsTeamPosFree(int team, int posIndex, bool ignoreBots, CSDKPlayer **pPlayerOnPos)
{
	if (posIndex < 0 || posIndex > mp_maxplayers.GetInt() - 1)
		return false;

	if (team == TEAM_SPECTATOR || team == TEAM_UNASSIGNED)
		return true;

	if (GetTeam()->GetFormation()->positions[posIndex]->type == POS_GK)
	{
		if (!IsBot() && !humankeepers.GetBool())
			return false;
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)	
	{
		CSDKPlayer *pPl = (CSDKPlayer*)UTIL_PlayerByIndex(i);

		if (!pPl || pPl == this)
			continue;

		if (pPl->GetTeamNumber() == team && pPl->GetTeamPosIndex() == posIndex || pPl->GetTeamToJoin() == team && pPl->GetTeamPosIndexToJoin() == posIndex)
		{
			if (IsBot() || !pPl->IsBot() || !ignoreBots)
			{
				*pPlayerOnPos = pPl;
				return false;
			}

			return true;
		}
	}
	return true;
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

const Vector CSDKPlayer::GetVisualLocalOrigin()
{
	Vector origin = GetLocalOrigin();
	// FIXME: Hack, because the ios player model moves up, but the bounding box doesn't
	if (!GetGroundEntity())
		origin.z += 20;

	return origin;
}

void CSDKPlayer::SetPosInsideShield(const Vector &pos, bool holdAtTargetPos)
{
	RemoveFlag(FL_SHIELD_KEEP_OUT);
	AddFlag(FL_SHIELD_KEEP_IN);
	m_vTargetPos = pos;
	m_bHoldAtTargetPos = holdAtTargetPos;
	//SetMoveType(MOVETYPE_NOCLIP);

	switch (SDKGameRules()->m_nShieldType)
	{
	case SHIELD_KICKOFF:
		break;
	case SHIELD_THROWIN:
	case SHIELD_PENALTY:
		break;
	case SHIELD_GOALKICK:
	case SHIELD_CORNER:
	case SHIELD_FREEKICK:
		if (mp_shield_liberal_taker_positioning.GetBool())
		{
			//m_vTargetPos = GetLocalOrigin();
			//AddFlag(FL_SHIELD_KEEP_IN);
			GetTargetPos(GetLocalOrigin(), m_vTargetPos.GetForModify());
			//RemoveFlag(FL_SHIELD_KEEP_IN);
			SetPosOutsideBall(m_vTargetPos);
		}
		break;
	}

	if (m_vTargetPos == GetLocalOrigin())
	{
		m_bIsAtTargetPos = true;
		if (m_bHoldAtTargetPos)
			AddFlag(FL_ATCONTROLS);

		//if (ShotButtonsPressed())
		//	m_bShotButtonsReleased = false;
	}
	else
	{
		ActivateRemoteControlling(m_vTargetPos);
	}
}

void CSDKPlayer::SetPosOutsideShield(bool teleport)
{
	RemoveFlag(FL_SHIELD_KEEP_IN);
	AddFlag(FL_SHIELD_KEEP_OUT);
	m_bHoldAtTargetPos = false;
	m_vTargetPos = vec3_invalid;

	switch (SDKGameRules()->m_nShieldType)
	{
	case SHIELD_KICKOFF:
		m_vTargetPos = GetSpawnPos(true);
		break;
	case SHIELD_KEEPERHANDS:
		return;
		break;
	default:
		GetTargetPos(GetLocalOrigin(), m_vTargetPos.GetForModify());
		break;
	}

	if (m_vTargetPos == GetLocalOrigin() || teleport)
	{
		if (teleport)
			SetLocalOrigin(m_vTargetPos);

		m_bIsAtTargetPos = true;
		if (m_bHoldAtTargetPos)
			AddFlag(FL_ATCONTROLS);
	}
	else
	{
		ActivateRemoteControlling(m_vTargetPos);
	}
}

void CSDKPlayer::SetPosOutsideBall(const Vector &playerPos)
{
	RemoveFlag(FL_SHIELD_KEEP_IN | FL_SHIELD_KEEP_OUT);

	Vector ballPos = GetBall()->GetPos();

	Vector ballPlayerDir = playerPos - ballPos;

	if (ballPlayerDir.Length2D() >= 1.5f * (GetPlayerMaxs().x - GetPlayerMins().x))
	{
		m_bIsAtTargetPos = true;
	}
	else
	{
		Vector moveDir = Vector(0, -GetTeam()->m_nForward, 0);
		moveDir *= 2 * (GetPlayerMaxs().x - GetPlayerMins().x);
		ActivateRemoteControlling(ballPos + moveDir);
	}
}

void CSDKPlayer::ActivateRemoteControlling(const Vector &targetPos)
{
	m_vTargetPos = targetPos;
	m_bIsAtTargetPos = false;
	DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
	AddFlag(FL_REMOTECONTROLLED);
	//AddSolidFlags(FSOLID_NOT_SOLID);
	SetCollisionGroup(COLLISION_GROUP_NONSOLID_PLAYER);
}

void CSDKPlayer::GetTargetPos(const Vector &pos, Vector &targetPos)
{
	float border = (GetFlags() & FL_SHIELD_KEEP_IN) ? -mp_shield_border.GetInt() : mp_shield_border.GetInt();

	if (SDKGameRules()->m_nShieldType == SHIELD_GOALKICK || 
		SDKGameRules()->m_nShieldType == SHIELD_PENALTY ||
		SDKGameRules()->m_nShieldType == SHIELD_KEEPERHANDS)
	{
		int side = (SDKGameRules()->m_nShieldType == SHIELD_PENALTY ? GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->GetOppTeamNumber() : SDKGameRules()->m_nShieldTeam);
		Vector min = GetGlobalTeam(side)->m_vPenBoxMin - border;
		Vector max = GetGlobalTeam(side)->m_vPenBoxMax + border;

		if (GetFlags() & FL_SHIELD_KEEP_OUT || SDKGameRules()->m_nShieldType == SHIELD_PENALTY)
		{
			if (SDKGameRules()->m_vKickOff.GetY() > min.y)
				min.y -= 500;
			else
				max.y += 500;
		}

		bool isInsideBox = pos.x > min.x && pos.y > min.y && pos.x < max.x && pos.y < max.y; 

		if ((GetFlags() & FL_SHIELD_KEEP_OUT) && isInsideBox)
		{
			targetPos = Vector(pos.x, pos.y, SDKGameRules()->m_vKickOff.GetZ());
			targetPos.y = GetGlobalTeam(side)->m_nForward == 1 ? max.y : min.y;

			/*if (entindex() % 2 == 0)
				targetPos.x -= (entindex() - 1) * (VEC_HULL_MAX.x * 2);
			else
				targetPos.x += entindex() * (VEC_HULL_MAX.x * 2);*/
		}
		else if ((GetFlags() & FL_SHIELD_KEEP_IN))
		{
			if (isInsideBox)
				targetPos = pos;
		}
	}

	if (SDKGameRules()->m_nShieldType == SHIELD_THROWIN || 
		SDKGameRules()->m_nShieldType == SHIELD_FREEKICK || 
		SDKGameRules()->m_nShieldType == SHIELD_CORNER ||  
		SDKGameRules()->m_nShieldType == SHIELD_KICKOFF ||
		SDKGameRules()->m_nShieldType == SHIELD_PENALTY && (GetFlags() & FL_SHIELD_KEEP_OUT))
	{
		float radius = SDKGameRules()->GetShieldRadius(GetTeamNumber(), ((GetFlags() & FL_SHIELD_KEEP_IN) != 0)) + border;
		Vector tempPos = (SDKGameRules()->m_nShieldType == SHIELD_PENALTY && targetPos != vec3_invalid) ? targetPos : pos;

		Vector dir = tempPos - SDKGameRules()->m_vShieldPos;

		if ((GetFlags() & FL_SHIELD_KEEP_OUT) && dir.Length2D() < radius)
		{
			Vector moveDir;

			if (SDKGameRules()->m_nShieldType == SHIELD_PENALTY)
			{
				moveDir = GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->m_vPenalty - SDKGameRules()->m_vShieldPos;
			}
			else
			{
				moveDir = GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->GetOppTeam()->m_vPenalty - SDKGameRules()->m_vShieldPos;
			}

			moveDir.z = 0;
			moveDir.NormalizeInPlace();
			targetPos = SDKGameRules()->m_vShieldPos + moveDir * radius;

			//float ang = acos((targetPos.x - SDKGameRules()->m_vShieldPos.GetX()) / radius);
			//if (entindex() % 2 == 0)
			//	ang -= (entindex() - 1) * (VEC_HULL_MAX.x * 2);
			//else
			//	ang += entindex() * (VEC_HULL_MAX.x * 2);
			//targetPos.x = SDKGameRules()->m_vShieldPos.GetX() + radius * cos(ang);
			//targetPos.y = SDKGameRules()->m_vShieldPos.GetY() + radius * sin(ang);
		}
		else if ((GetFlags() & FL_SHIELD_KEEP_IN))
		{
			if (dir.Length2D() <= radius)
				targetPos = pos;
		}

		if (SDKGameRules()->m_nShieldType == SHIELD_FREEKICK && mp_shield_block_sixyardbox.GetBool())
		{
			if (GetTeamPosType() != POS_GK || GetTeamNumber() != GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->GetOppTeamNumber())
			{
				int side = GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->GetOppTeamNumber();
				Vector min = GetGlobalTeam(side)->m_vSixYardBoxMin - border;
				Vector max = GetGlobalTeam(side)->m_vSixYardBoxMax + border;

				if (GetGlobalTeam(side)->m_nForward == 1)
					min.y -= 500;
				else
					max.y += 500;

				bool isInsideBox = pos.x > min.x && pos.y > min.y && pos.x < max.x && pos.y < max.y; 

				if (isInsideBox)
				{
					targetPos = Vector(pos.x, pos.y, SDKGameRules()->m_vKickOff.GetZ());
					targetPos.y = GetGlobalTeam(side)->m_nForward == 1 ? max.y : min.y;
				}
			}
		}
	}

	if (targetPos == vec3_invalid)
		targetPos = pos;
}

bool CSDKPlayer::IsOnField(CSDKPlayer *pPl)
{
	return (pPl && pPl->IsConnected() && (pPl->GetTeamNumber() == TEAM_A || pPl->GetTeamNumber() == TEAM_B));
}

bool CSDKPlayer::IsOffside()
{
	return mp_offside.GetBool() ? m_bIsOffside : false;
}

void CSDKPlayer::SetOffside(bool isOffside)
{
	m_bIsOffside = isOffside;
}

void CSDKPlayer::SetOffsidePos(Vector pos)
{
	m_vOffsidePos = GetLocalOrigin();
}

Vector CSDKPlayer::GetOffsidePos()
{
	return m_vOffsidePos;
}

void CSDKPlayer::SetOffsideLastOppPlayerPos(Vector pos)
{
	m_vOffsideLastOppPlayerPos = pos;
}

Vector CSDKPlayer::GetOffsideLastOppPlayerPos()
{
	return m_vOffsideLastOppPlayerPos;
}

void CSDKPlayer::SetOffsideBallPos(Vector pos)
{
	m_vOffsideBallPos = pos;
}

Vector CSDKPlayer::GetOffsideBallPos()
{
	return m_vOffsideBallPos;
}

Vector CSDKPlayer::GetSpawnPos(bool findSafePos)
{
	//Vector spawnPos = pPlayer->GetTeam()->m_vPlayerSpawns[ToSDKPlayer(pPlayer)->GetShirtNumber() - 1];
	Vector halfField = (SDKGameRules()->m_vFieldMax - SDKGameRules()->m_vFieldMin);
	halfField.y /= 2;
	float xDist = halfField.x / 5;
	float yDist = halfField.y / 5;
	float xPos = GetTeam()->GetFormation()->positions[GetTeamPosIndex()]->x * xDist + xDist;
	float yPos = GetTeam()->GetFormation()->positions[GetTeamPosIndex()]->y * yDist + max(mp_shield_kickoff_radius.GetInt() + 2 * mp_shield_border.GetInt(), yDist);

	Vector spawnPos;
	if (GetTeam()->m_nForward == 1)
		spawnPos = Vector(SDKGameRules()->m_vFieldMin.GetX(), SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ()) + Vector(xPos, -yPos, 10);
	else
		spawnPos = Vector(SDKGameRules()->m_vFieldMax.GetX(), SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ()) + Vector(-xPos, yPos, 10);

	if (findSafePos)
		FindSafePos(spawnPos);

	return spawnPos;
}

int CSDKPlayer::GetShirtNumber()
{
	return m_nShirtNumber;
}

int CSDKPlayer::GetTeamPosType()
{
	Assert(GetTeamPosIndex() < GetTeam()->GetFormation()->positions.Count());
	return (int)GetTeam()->GetFormation()->positions[GetTeamPosIndex()]->type;
}

void CSDKPlayer::ResetFlags()
{
	m_Shared.SetStamina(100);
	InitSprinting();
	m_flNextShot = gpGlobals->curtime;
	m_bIsAtTargetPos = false;
	RemoveFlag(FL_SHIELD_KEEP_IN | FL_SHIELD_KEEP_OUT | FL_REMOTECONTROLLED | FL_FREECAM | FL_CELEB | FL_NO_X_MOVEMENT | FL_NO_Y_MOVEMENT | FL_ATCONTROLS | FL_FROZEN);
	DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
	m_pHoldingBall = NULL;
	m_bIsAway = true;
	m_flLastMoveTime = gpGlobals->curtime;
	//m_flNextJoin = gpGlobals->curtime;
	m_bIsOffside = false;
	m_ePenaltyState = PENALTY_NONE;
	m_flRemoteControlledStartTime = -1;

	if (GetPlayerBall())
		GetPlayerBall()->RemovePlayerBall();

	if ((GetTeamNumber() == TEAM_A || GetTeamNumber() == TEAM_B) && !ReplayManager()->IsReplaying())
	{
		RemoveSolidFlags(FSOLID_NOT_SOLID);
		SetCollisionGroup(COLLISION_GROUP_PLAYER);
		RemoveEffects(EF_NODRAW);
	}
}

bool CSDKPlayer::IsNormalshooting()
{
	return (m_nButtons & IN_ATTACK) != 0;
}

bool CSDKPlayer::IsPowershooting()
{
	return ((m_nButtons & IN_ALT1) != 0);
}

bool CSDKPlayer::IsChargedshooting()
{
	return m_Shared.m_bDoChargedShot;
}

bool CSDKPlayer::IsShooting()
{
	return IsNormalshooting() || IsPowershooting() || IsChargedshooting();
}

bool CSDKPlayer::ShotButtonsPressed()
{
	return ((m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1 | IN_ALT2))) != 0);
}

bool CSDKPlayer::ShotButtonsReleased()
{
	return m_bShotButtonsReleased;
}

void CSDKPlayer::SetShotButtonsReleased(bool released)
{
	m_bShotButtonsReleased = released;
}

CSDKPlayer *CSDKPlayer::FindClosestPlayerToSelf(bool teammatesOnly, bool forwardOnly /*= false*/, float maxYawAngle /*= 360*/)
{
	float shortestDist = FLT_MAX;
	CSDKPlayer *pClosest = NULL;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl) || pPl == this)
			continue;

		if (teammatesOnly && pPl->GetTeamNumber() != GetTeamNumber())
			continue;

		Vector dir = pPl->GetLocalOrigin() - GetLocalOrigin();
		dir.z = 0;
		float dist = dir.Length2D();
		dir.NormalizeInPlace();

		if (forwardOnly && Sign(dir.y) != GetTeam()->m_nForward)
			continue;

		if (maxYawAngle < 360 && abs(RAD2DEG(acos(EyeDirection2D().Dot(dir)))) > maxYawAngle / 2)
			continue;

		if (dist < shortestDist)
		{
			shortestDist = dist;
			pClosest = pPl;
		}
	}

	return pClosest;
}

ConVar mp_chat_match_captain_only("mp_chat_match_captain_only", "0", FCVAR_NOTIFY);
ConVar mp_chat_intermissions_captain_only("mp_chat_intermissions_captain_only", "0", FCVAR_NOTIFY);

bool CSDKPlayer::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	// can always hear the console unless we're ignoring all chat
	if ( !pPlayer )
		return m_iIgnoreGlobalChat != CHAT_IGNORE_ALL;

	// check if we're ignoring all chat
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_ALL )
		return false;

	// check if we're ignoring all but teammates
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_TEAM && g_pGameRules->PlayerRelationship( this, pPlayer, MM_NONE ) != GR_TEAMMATE )
		return false;

	// can't hear dead players if we're alive
	//if ( pPlayer->m_lifeState != LIFE_ALIVE && m_lifeState == LIFE_ALIVE )
	//	return false;

	return true;
}


bool CSDKPlayer::CanSpeak(MessageMode_t messageMode)
{
	if (messageMode != MM_SAY || GetTeam()->GetCaptain() == this)
		return true;

	if (SDKGameRules()->IsIntermissionState() && mp_chat_intermissions_captain_only.GetBool() || !SDKGameRules()->IsIntermissionState() && mp_chat_match_captain_only.GetBool())
		return false;

	return true;
}

#include <ctype.h>

char *trimstr(char *str)
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

void sanitize(char *str)
{
	for (char *chr = str; chr != NULL && *chr != '\0'; chr++)
	{
		// Replace reserved characters:
		// #: Signals a localization string if used as first character
		// %: Used in localization strings as placeholders
		// 1-5: Used in localization strings as color codes
		if ((chr == str && *chr == '#') || *chr == '%' || *chr == 1 || *chr == 2 || *chr == 3 || *chr == 4 || *chr == 5)
			*chr = '*';
	}
}

const char *CSDKPlayer::GetPlayerName()
{
	return m_szPlayerName;
}

void CSDKPlayer::SetPlayerName(const char *name)
{
	static const int reservedNameCount = 7;
	static const char *reservedNames[reservedNameCount] = { "KeeperBotHome", "KeeperBotAway", "FieldBot", "SourceTV", "Source TV", "IOSTV", "IOS TV" };

	char sanitizedName[MAX_PLAYER_NAME_LENGTH];

	Q_strncpy(sanitizedName, name, sizeof(sanitizedName));

	sanitize(sanitizedName);

	trimstr(sanitizedName);

	if (sanitizedName[0] == '\0')
		Q_strncpy(sanitizedName, "unnamed", sizeof(sanitizedName));

	if (!Q_strcmp(sanitizedName, m_szPlayerName))
		return;

	if (Q_strcmp(engine->GetPlayerNetworkIDString(this->edict()), "BOT"))
	{
		for (int i = 0; i < reservedNameCount; i++)
		{
			if (!Q_stricmp(reservedNames[i], sanitizedName))
			{
				Q_snprintf(sanitizedName, sizeof(sanitizedName), "Not %s", reservedNames[i]);
				break;
			}
		}
	}

	int duplicateNameCount = 0;

	// Check if the name the player wants is already taken by another player
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl || pPl == this)
			continue;

		// If the name is taken, prepend an index to the name and check all players again until we have a unique name.
		if (!Q_stricmp(pPl->GetPlayerName(), sanitizedName))
		{
			duplicateNameCount += 1;
			Q_snprintf(sanitizedName, sizeof(sanitizedName), "(%d)%s", duplicateNameCount, pPl->GetPlayerName());
			i = 0;
			continue;
		}
	}

	Q_strncpy(m_szPlayerName, sanitizedName, sizeof(m_szPlayerName));

	if (GetPlayerData())
		SetLastKnownName(m_szPlayerName);

	if (GetShirtName()[0] == '\0')
		SetShirtName(m_szPlayerName);

	m_bPlayerNameChanged = true;

	engine->ClientCommand(edict(), UTIL_VarArgs("setinfo name \"%s\"", m_szPlayerName));
}

const char *CSDKPlayer::GetClubName()
{
	return m_szClubName;
}

void CSDKPlayer::SetClubName(const char *name)
{
	char sanitizedName[MAX_PLAYER_NAME_LENGTH];

	Q_strncpy(sanitizedName, name, sizeof(sanitizedName));

	sanitize(sanitizedName);

	trimstr(sanitizedName);

	if (!Q_strcmp(sanitizedName, m_szClubName))
		return;

	Q_strncpy(m_szClubName, sanitizedName, sizeof(m_szClubName));

	m_bClubNameChanged = true;
}

const char *CSDKPlayer::GetShirtName()
{
	return m_szShirtName;
}

void CSDKPlayer::SetShirtName(const char *name)
{
	char sanitizedName[MAX_PLAYER_NAME_LENGTH];

	Q_strncpy(sanitizedName, name, sizeof(sanitizedName));

	sanitize(sanitizedName);

	trimstr(sanitizedName);

	if (!Q_strcmp(sanitizedName, m_szShirtName))
		return;

	Q_strncpy(m_szShirtName, sanitizedName, sizeof(m_szShirtName));

	if (GetPlayerData())
		SetLastKnownShirtName(m_szShirtName);

	m_bShirtNameChanged = true;
}

void CSDKPlayer::AddRedCard()
{
	GetMatchPeriodData()->m_nRedCards += 1;
	GetMatchData()->m_nRedCards += 1;
	GetTeam()->m_RedCards += 1;
}

void CSDKPlayer::AddYellowCard()
{
	GetMatchPeriodData()->m_nYellowCards += 1;
	GetMatchData()->m_nYellowCards += 1;
	GetTeam()->m_YellowCards += 1;
}

void CSDKPlayer::AddFoul()
{
	GetMatchPeriodData()->m_nFouls += 1;
	GetMatchData()->m_nFouls += 1;
	GetTeam()->m_Fouls += 1;
}

void CSDKPlayer::AddFoulSuffered()
{
	GetMatchPeriodData()->m_nFoulsSuffered += 1;
	GetMatchData()->m_nFoulsSuffered += 1;
	GetTeam()->m_FoulsSuffered += 1;
}

void CSDKPlayer::AddSlidingTackle()
{
	GetMatchPeriodData()->m_nSlidingTackles += 1;
	GetMatchData()->m_nSlidingTackles += 1;
	GetTeam()->m_SlidingTackles += 1;
}

void CSDKPlayer::AddSlidingTackleCompleted()
{
	GetMatchPeriodData()->m_nSlidingTacklesCompleted += 1;
	GetMatchData()->m_nSlidingTacklesCompleted += 1;
	GetTeam()->m_SlidingTacklesCompleted += 1;
}

void CSDKPlayer::AddGoalConceded()
{
	GetMatchPeriodData()->m_nGoalsConceded += 1;
	GetMatchData()->m_nGoalsConceded += 1;
	GetTeam()->m_GoalsConceded += 1;
}

void CSDKPlayer::AddShot()
{
	GetMatchPeriodData()->m_nShots += 1;
	GetMatchData()->m_nShots += 1;
	GetTeam()->m_Shots += 1;
}

void CSDKPlayer::AddShotOnGoal()
{
	GetMatchPeriodData()->m_nShotsOnGoal += 1;
	GetMatchData()->m_nShotsOnGoal += 1;
	GetTeam()->m_ShotsOnGoal += 1;
}

void CSDKPlayer::AddPassCompleted()
{
	GetMatchPeriodData()->m_nPassesCompleted += 1;
	GetMatchData()->m_nPassesCompleted += 1;
	GetTeam()->m_PassesCompleted += 1;
}

void CSDKPlayer::AddInterception()
{
	GetMatchPeriodData()->m_nInterceptions += 1;
	GetMatchData()->m_nInterceptions += 1;
	GetTeam()->m_Interceptions += 1;
}

void CSDKPlayer::AddOffside()
{
	GetMatchPeriodData()->m_nOffsides += 1;
	GetMatchData()->m_nOffsides += 1;
	GetTeam()->m_Offsides += 1;
}

void CSDKPlayer::AddGoal()
{
	GetMatchPeriodData()->m_nGoals += 1;
	GetMatchData()->m_nGoals += 1;
	GetTeam()->m_Goals += 1;
}

void CSDKPlayer::AddOwnGoal()
{
	GetMatchPeriodData()->m_nOwnGoals += 1;
	GetMatchData()->m_nOwnGoals += 1;
	GetTeam()->m_OwnGoals += 1;
	GetOppTeam()->m_Goals += 1;
}

void CSDKPlayer::AddAssist()
{
	GetMatchPeriodData()->m_nAssists += 1;
	GetMatchData()->m_nAssists += 1;
	GetTeam()->m_Assists += 1;
}

void CSDKPlayer::AddPossessionTime(float time)
{
	GetMatchPeriodData()->m_flPossessionTime += time;
	GetMatchData()->m_flPossessionTime += time;
	GetTeam()->m_flPossessionTime += time;
}

void CSDKPlayer::AddExactDistanceCovered(float amount)
{
	GetMatchPeriodData()->m_flExactDistanceCovered += amount;
	GetMatchPeriodData()->m_nDistanceCovered = GetMatchPeriodData()->m_flExactDistanceCovered * 10 / 1000;
	GetMatchData()->m_flExactDistanceCovered += amount;
	GetMatchData()->m_nDistanceCovered = GetMatchData()->m_flExactDistanceCovered * 10 / 1000;
	GetTeam()->m_flExactDistanceCovered += amount;
	GetTeam()->m_DistanceCovered = GetTeam()->m_flExactDistanceCovered * 10 / 1000;
}

void CSDKPlayer::AddPass()
{
	GetMatchPeriodData()->m_nPasses += 1;
	GetMatchData()->m_nPasses += 1;
	GetTeam()->m_Passes += 1;
}

void CSDKPlayer::AddFreeKick()
{
	GetMatchPeriodData()->m_nFreeKicks += 1;
	GetMatchData()->m_nFreeKicks += 1;
	GetTeam()->m_FreeKicks += 1;
}

void CSDKPlayer::AddPenalty()
{
	GetMatchPeriodData()->m_nPenalties += 1;
	GetMatchData()->m_nPenalties += 1;
	GetTeam()->m_Penalties += 1;
}

void CSDKPlayer::AddCorner()
{
	GetMatchPeriodData()->m_nCorners += 1;
	GetMatchData()->m_nCorners += 1;
	GetTeam()->m_Corners += 1;
}

void CSDKPlayer::AddThrowIn()
{
	GetMatchPeriodData()->m_nThrowIns += 1;
	GetMatchData()->m_nThrowIns += 1;
	GetTeam()->m_ThrowIns += 1;
}

void CSDKPlayer::AddKeeperSave()
{
	GetMatchPeriodData()->m_nKeeperSaves += 1;
	GetMatchData()->m_nKeeperSaves += 1;
	GetTeam()->m_KeeperSaves += 1;
}

void CSDKPlayer::AddGoalKick()
{
	GetMatchPeriodData()->m_nGoalKicks += 1;
	GetMatchData()->m_nGoalKicks += 1;
	GetTeam()->m_GoalKicks += 1;
}

void CSDKPlayer::DrawDebugGeometryOverlays(void) 
{
	BaseClass::DrawDebugGeometryOverlays();
}

void CSDKPlayer::SetPlayerBallSkinName(const char *skinName)
{
	for (int i = 0; i < CBallInfo::m_BallInfo.Count(); i++)
	{
		if (!Q_strcmp(skinName, CBallInfo::m_BallInfo[i]->m_szFolderName))
		{
			Q_strncpy(m_szPlayerBallSkinName, skinName, sizeof(m_szPlayerBallSkinName));
			break;
		}
	}
}

CUtlVector<CPlayerPersistentData *> CPlayerPersistentData::m_PlayerPersistentData;

CPlayerPersistentData::CPlayerPersistentData(CSDKPlayer *pPl)
{
	m_pPl = pPl;
	m_nSteamCommunityID = engine->GetClientSteamID(pPl->edict()) ? engine->GetClientSteamID(pPl->edict())->ConvertToUint64() : 0;
	Q_strncpy(m_szSteamID, engine->GetPlayerNetworkIDString(pPl->edict()), 32);
	Q_strncpy(m_szName, pPl->GetPlayerName(), MAX_PLAYER_NAME_LENGTH);
	Q_strncpy(m_szShirtName, pPl->GetShirtName(), MAX_PLAYER_NAME_LENGTH);
	m_pMatchData = new CPlayerMatchData();
	ResetData();
}

CPlayerPersistentData::~CPlayerPersistentData()
{
	delete m_pMatchData;
	m_pMatchData = NULL;

	m_MatchPeriodData.PurgeAndDeleteElements();
}

void CPlayerMatchData::ResetData()
{
	m_nRedCards = 0;
	m_nYellowCards = 0;
	m_nFouls = 0;
	m_nFoulsSuffered = 0;
	m_nSlidingTackles = 0;
	m_nSlidingTacklesCompleted = 0;
	m_nGoalsConceded = 0;
	m_nShots = 0;
	m_nShotsOnGoal = 0;
	m_nPassesCompleted = 0;
	m_nInterceptions = 0;
	m_nOffsides = 0;
	m_nGoals = 0;
	m_nOwnGoals = 0;
	m_nAssists = 0;
	m_nPasses = 0;
	m_nFreeKicks = 0;
	m_nPenalties = 0;
	m_nCorners = 0;
	m_nThrowIns = 0;
	m_nKeeperSaves = 0;
	m_nGoalKicks = 0;
	m_nRating = 0;
	m_nPossession = 0;
	m_flPossessionTime = 0.0f;
	m_nDistanceCovered = 0;
	m_flExactDistanceCovered = 0.0f;
}

void CPlayerMatchPeriodData::ResetData()
{
	BaseClass::ResetData();
}

void CPlayerPersistentData::ResetData()
{
	m_nNextCardJoin = 0;
	m_flMaxStamina = 100;
	m_pPl->m_Shared.SetMaxStamina(100, false);

	m_pMatchData->ResetData();
	m_MatchPeriodData.PurgeAndDeleteElements();
}

void CPlayerPersistentData::AllocateData(CSDKPlayer *pPl)
{
	CPlayerPersistentData *data = NULL;
	unsigned long long steamCommunityID = engine->GetClientSteamID(pPl->edict()) ? engine->GetClientSteamID(pPl->edict())->ConvertToUint64() : 0;

	for (int i = 0; i < m_PlayerPersistentData.Count(); i++)
	{
		if (m_PlayerPersistentData[i]->m_nSteamCommunityID != steamCommunityID)
			continue;

		data = m_PlayerPersistentData[i];
		data->m_pPl = pPl;
		data->m_pPl->m_Shared.SetMaxStamina(data->m_flMaxStamina, false);
		break;
	}

	if (!data)
	{
		data = new CPlayerPersistentData(pPl);
		m_PlayerPersistentData.AddToTail(data);
	}

	pPl->SetData(data);
}

void CPlayerPersistentData::AddToAllMaxStaminas(float staminaToAdd)
{
	for (int i = 0; i < m_PlayerPersistentData.Count(); i++)
	{
		CPlayerPersistentData *pData = m_PlayerPersistentData[i];

		pData->m_flMaxStamina = min(100, pData->m_flMaxStamina + staminaToAdd);

		if (pData->m_pPl)
			pData->m_pPl->m_Shared.SetMaxStamina(pData->m_flMaxStamina, false);
	}
}

void CPlayerPersistentData::ReallocateAllPlayerData()
{
	m_PlayerPersistentData.PurgeAndDeleteElements();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		AllocateData(pPl);
	}
}

void CPlayerPersistentData::StartNewMatchPeriod()
{
	m_MatchPeriodData.AddToTail(new CPlayerMatchPeriodData());
	m_MatchPeriodData.Tail()->ResetData();
	m_MatchPeriodData.Tail()->m_nStartSecond = SDKGameRules()->GetMatchDisplayTimeSeconds();
	m_MatchPeriodData.Tail()->m_nEndSecond = -1;
	m_MatchPeriodData.Tail()->m_nTeam = m_pPl->GetTeamNumber();
	m_MatchPeriodData.Tail()->m_nTeamPosType = m_pPl->GetTeamPosType();
}

void CPlayerPersistentData::EndCurrentMatchPeriod()
{
	if (m_MatchPeriodData.Count() > 0 && m_MatchPeriodData.Tail()->m_nEndSecond == -1)
	{
		m_MatchPeriodData.Tail()->m_nEndSecond = SDKGameRules()->GetMatchDisplayTimeSeconds();
	}
}

ConVar sv_webserver_matchdata_url("sv_webserver_matchdata_url", "http://simrai.iosoccer.com/matches");
ConVar sv_webserver_matchdata_accesstoken("sv_webserver_matchdata_accesstoken", "");
ConVar sv_webserver_matchdata_enabled("sv_webserver_matchdata_enabled", "0");

static const int JSON_SIZE = 40 * 1024;

#include "curl/curl.h"

struct Curl_t
{
	char json[JSON_SIZE];
	char *memory;
	size_t size;
};
 
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct Curl_t *mem = (struct Curl_t *)userp;
 
  mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

unsigned CurlSendJSON(void *params)
{
	Curl_t *pVars = (Curl_t *)params;
	pVars->memory = (char *)malloc(1);
	pVars->size = 0;

	CURL *curl;
	CURLcode res;

	/* In windows, this will init the winsock stuff */ 
	res = curl_global_init(CURL_GLOBAL_DEFAULT);
	/* Check for errors */ 
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_global_init() failed: %s\n",
			curl_easy_strerror(res));

		delete pVars;

		return 1;
	}

	/* get a curl handle */ 
	curl = curl_easy_init();
	if(curl) {
		/* First set the URL that is about to receive our POST. */ 
		curl_easy_setopt(curl, CURLOPT_URL, sv_webserver_matchdata_url.GetString());

		/* Now specify we want to POST data */ 
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pVars->json);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, pVars);
		/* we want to use our own read function */ 
		//curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

		/* pointer to pass to our read function */ 
		//curl_easy_setopt(curl, CURLOPT_READDATA, &pooh);

		/* get verbose debug output please */ 
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "Accept: text/plain");  
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "charsets: utf-8"); 
		res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		/* use curl_slist_free_all() after the *perform() call to free this
		list again */ 

		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);
		long code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
		/* Check for errors */ 
		if (res != CURLE_OK)
		{
			char msg[128];
			Q_snprintf(msg, sizeof(msg), "Couldn't submit match statistics to web server: cURL error code '%d'. Wrong web server URL or web server down?", res);
			UTIL_ClientPrintAll(HUD_PRINTTALK, msg);
		}
		else
		{
			if (code == 200)
			{
				char msg[128];
				Q_snprintf(msg, sizeof(msg), "Check out this match's statistics at %s/%s", sv_webserver_matchdata_url.GetString(), pVars->memory);
				UTIL_ClientPrintAll(HUD_PRINTTALK, msg);
			}
			else if (code == 401)
			{
				UTIL_ClientPrintAll(HUD_PRINTTALK, "Couldn't submit match statistics to web server: Invalid or revoked API token.");
			}
		}

		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();

	if(pVars->memory)
		free(pVars->memory);

	delete pVars;

	return 0;
}

void SendMatchDataToWebserver(const char *json)
{
	Curl_t *pVars = new Curl_t;
	memcpy(pVars->json, json, JSON_SIZE);
	pVars->json[Q_strlen(pVars->json) - 1] = 0;
	Q_strcat(pVars->json, UTIL_VarArgs(",\"access_token\":\"%s\"}", sv_webserver_matchdata_accesstoken.GetString()), JSON_SIZE);
	//Q_strncpy(pVars->json, "{\"foo\": \"bar\"}", JSON_SIZE);
	CreateSimpleThread(CurlSendJSON, pVars);
}

void CPlayerPersistentData::ConvertAllPlayerDataToJson()
{
	static const int STAT_TYPE_COUNT = 24;
	static const char statTypes[STAT_TYPE_COUNT][32] =
	{
		"redCards",
		"yellowCards",
		"fouls",
		"foulsSuffered",
		"slidingTackles",
		"slidingTacklesCompleted",
		"goalsConceded",
		"shots",
		"shotsOnGoal",
		"passesCompleted",
		"interceptions",
		"offsides",
		"goals",
		"ownGoals",
		"assists",
		"passes",
		"freeKicks",
		"penalties",
		"corners",
		"throwIns",
		"keeperSaves",
		"goalKicks",
		"possession",
		"distanceCovered"
	};

	char *json = new char[JSON_SIZE];
	json[0] = 0;

	Q_strcat(json, "{\"matchData\":{", JSON_SIZE);

	Q_strcat(json, "\"statisticTypes\":[", JSON_SIZE);

	for (int i = 0; i < STAT_TYPE_COUNT; i++)
	{
		if (i > 0)
			Q_strcat(json, ",", JSON_SIZE);

		Q_strcat(json, UTIL_VarArgs("\"%s\"", statTypes[i]), JSON_SIZE);
	}

	Q_strcat(json, UTIL_VarArgs("],\"matchInfo\":{\"type\":\"%s\",\"startTime\":%lu,\"endTime\":%lu", mp_matchinfo.GetString(), SDKGameRules()->m_nRealMatchStartTime, SDKGameRules()->m_nRealMatchEndTime), JSON_SIZE);

	Q_strcat(json, "},\"teams\":[", JSON_SIZE);

	for (int team = TEAM_A; team <= TEAM_B; team++)
	{
		CTeam *pTeam = GetGlobalTeam(team);

		if (team == TEAM_B)
			Q_strcat(json, ",", JSON_SIZE);

		bool isMix = (pTeam->GetShortTeamName()[0] == 0);

		Q_strcat(json, UTIL_VarArgs("{\"info\":{\"name\":\"%s\",\"side\":\"%s\",\"isMix\":%s}", (isMix ? pTeam->GetKitName() : pTeam->GetShortTeamName()), (team == TEAM_A ? "home" : "away"), (isMix ? "true" : "false")), JSON_SIZE);

		Q_strcat(json, UTIL_VarArgs(",\"statistics\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}",
			pTeam->m_RedCards, pTeam->m_YellowCards, pTeam->m_Fouls, pTeam->m_FoulsSuffered, pTeam->m_SlidingTackles, pTeam->m_SlidingTacklesCompleted, pTeam->m_GoalsConceded, pTeam->m_Shots, pTeam->m_ShotsOnGoal, pTeam->m_PassesCompleted, pTeam->m_Interceptions, pTeam->m_Offsides, pTeam->m_Goals, pTeam->m_OwnGoals, pTeam->m_Assists, pTeam->m_Passes, pTeam->m_FreeKicks, pTeam->m_Penalties, pTeam->m_Corners, pTeam->m_ThrowIns, pTeam->m_KeeperSaves, pTeam->m_GoalKicks, (int)pTeam->m_flPossessionTime, (int)pTeam->m_flExactDistanceCovered
			), JSON_SIZE);
	}

	Q_strcat(json, "],\"players\":[", JSON_SIZE);

	int playersProcessed = 0;

	for (int i = 0; i < m_PlayerPersistentData.Count(); i++)
	{
		CPlayerPersistentData *pData = m_PlayerPersistentData[i];

		char playerName[MAX_PLAYER_NAME_LENGTH * 2] = {};

		int indexOffset = 0;

		for (int j = 0; j < Q_strlen(pData->m_szName); j++)
		{
			if (pData->m_szName[j] == '"' || pData->m_szName[j] == '\\')
			{
				playerName[j + indexOffset] = '\\';
				playerName[j + indexOffset + 1] = pData->m_szName[j];
				indexOffset += 1;
			}
			else
				playerName[j + indexOffset] = pData->m_szName[j];
		}

		if (playersProcessed > 0)
			Q_strcat(json, ",", JSON_SIZE);

		Q_strcat(json, "{", JSON_SIZE);

		Q_strcat(json, UTIL_VarArgs("\"info\":{\"steamId\":\"%s\",\"name\":\"%s\"}", pData->m_szSteamID, playerName), JSON_SIZE);

		Q_strcat(json, ",\"matchPeriodData\":[", JSON_SIZE);

		for (int j = 0; j < pData->m_MatchPeriodData.Count(); j++)
		{
			CPlayerMatchPeriodData *pMPData = pData->m_MatchPeriodData[j];

			if (j > 0)
				Q_strcat(json, ",", JSON_SIZE);

			int startSecond = pMPData->m_nStartSecond;

			if (startSecond < 0)
			{
				DevMsg("Illegal 'start second' value: %d\n", startSecond);
				startSecond = 0;
			}

			int endSecond = pMPData->m_nEndSecond;

			if (endSecond == -1)
			{
				endSecond = SDKGameRules()->GetMatchDisplayTimeSeconds(true, false);
			}

			Q_strcat(json, UTIL_VarArgs("{\"info\":{\"startSecond\":%d,\"endSecond\":%d,\"team\":\"%s\",\"position\":\"%s\"}", startSecond, endSecond, (pMPData->m_nTeam == TEAM_A ? "home" : "away"), g_szPosNames[pMPData->m_nTeamPosType]), JSON_SIZE);

			Q_strcat(json, UTIL_VarArgs(",\"statistics\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}",
				pMPData->m_nRedCards, pMPData->m_nYellowCards, pMPData->m_nFouls, pMPData->m_nFoulsSuffered, pMPData->m_nSlidingTackles, pMPData->m_nSlidingTacklesCompleted, pMPData->m_nGoalsConceded, pMPData->m_nShots, pMPData->m_nShotsOnGoal, pMPData->m_nPassesCompleted, pMPData->m_nInterceptions, pMPData->m_nOffsides, pMPData->m_nGoals, pMPData->m_nOwnGoals, pMPData->m_nAssists, pMPData->m_nPasses, pMPData->m_nFreeKicks, pMPData->m_nPenalties, pMPData->m_nCorners, pMPData->m_nThrowIns, pMPData->m_nKeeperSaves, pMPData->m_nGoalKicks, (int)pMPData->m_flPossessionTime, (int)pMPData->m_flExactDistanceCovered
				), JSON_SIZE);
		}

		Q_strcat(json, "]}", JSON_SIZE);

		playersProcessed += 1;
	}

	Q_strcat(json, "],\"matchEvents\":[", JSON_SIZE);

	int eventsProcessed = 0;

	for (int i = 0; i < ReplayManager()->GetMatchEventCount(); i++)
	{
		MatchEvent *pMatchEvent = ReplayManager()->GetMatchEvent(i);

		if (eventsProcessed > 0)
			Q_strcat(json, ",", JSON_SIZE);

		Q_strcat(json, UTIL_VarArgs("{\"second\":%d,\"event\":\"%s\",\"period\":\"%s\",\"team\":\"%s\",\"player1SteamId\":\"%s\",\"player2SteamId\":\"%s\",\"player3SteamId\":\"%s\"}", pMatchEvent->second, g_szMatchEventNames[pMatchEvent->matchEventType], g_szMatchPeriodNames[pMatchEvent->matchPeriod], (pMatchEvent->team == TEAM_A ? "home" : "away"), pMatchEvent->pPlayer1Data ? pMatchEvent->pPlayer1Data->m_szSteamID : "", pMatchEvent->pPlayer2Data ? pMatchEvent->pPlayer2Data->m_szSteamID : "", pMatchEvent->pPlayer3Data ? pMatchEvent->pPlayer3Data->m_szSteamID : ""), JSON_SIZE);

		eventsProcessed += 1;
	}

	Q_strcat(json, "]}}", JSON_SIZE);

	filesystem->CreateDirHierarchy("statistics", "MOD");

	time_t rawtime;
	struct tm *timeinfo;

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	char teamNames[2][32];

	for (int team = TEAM_A; team <= TEAM_B; team++)
	{
		Q_strncpy(teamNames[team - TEAM_A], (GetGlobalTeam(team)->GetTeamCode()[0] == 0 ? GetGlobalTeam(team)->GetKitName() : GetGlobalTeam(team)->GetTeamCode()), 32);
		char *c = teamNames[team - TEAM_A];

		while (*c != 0)
		{
			if (*c != '.' && *c != '_' && (*c < 48 || *c > 57 && *c < 65 || *c > 90 && *c < 97 || *c > 122))
				*c = '.';

			c++;
		}
	}

	if (sv_webserver_matchdata_enabled.GetBool())
	{
		SendMatchDataToWebserver(json);
	}
	
	char time[64];
	strftime(time, sizeof(time), "%Y.%m.%d_%Hh.%Mm.%Ss", timeinfo);

	const char *filename = UTIL_VarArgs("statistics\\%s_%s-vs-%s_%d-%d.json", time, teamNames[0], teamNames[1], GetGlobalTeam(TEAM_A)->GetGoals(), GetGlobalTeam(TEAM_B)->GetGoals());

	FileHandle_t fh = filesystem->Open(filename, "w", "MOD");
 
	if (fh)
	{
		filesystem->Write(json, Q_strlen(json), fh);  
		filesystem->Close(fh);

		Msg("Match data file '%s' written\n", filename);
	}

	delete[] json;
}

void CC_SV_SaveMatchData(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	CPlayerPersistentData::ConvertAllPlayerDataToJson();
}

ConCommand sv_savematchdata("sv_savematchdata", CC_SV_SaveMatchData, "", 0);