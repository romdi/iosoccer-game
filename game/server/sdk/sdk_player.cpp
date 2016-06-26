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

	SendPropTime( SENDINFO( m_flNextJump ) ),
	SendPropTime( SENDINFO( m_flNextSlide) ),

	SendPropTime( SENDINFO( m_flPlayerAnimEventStartTime ) ),
	SendPropVector( SENDINFO( m_aPlayerAnimEventStartAngle ) ),
	SendPropInt( SENDINFO( m_nPlayerAnimEventStartButtons ) ),

	SendPropBool( SENDINFO( m_bIsShotCharging ) ),
	SendPropBool( SENDINFO( m_bDoChargedShot)),
	SendPropTime( SENDINFO( m_flShotChargingStart ) ),
	SendPropTime( SENDINFO( m_flShotChargingDuration ) ),

	SendPropInt(SENDINFO(m_nInPenBoxOfTeam), 3),
	SendPropBool(SENDINFO(m_bShotButtonsReleased)),

	SendPropInt( SENDINFO( m_nLastPressedSingleMoveKey ) ),

	SendPropInt( SENDINFO( m_nBoostRightDive ) ),
	SendPropTime( SENDINFO( m_flBoostRightDiveStart ) ),
	SendPropInt( SENDINFO( m_nBoostForwardDive ) ),
	SendPropTime( SENDINFO( m_flBoostForwardDiveStart ) )

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

	SendPropBool( SENDINFO( m_bJumping ) ),
	SendPropBool( SENDINFO( m_bWasJumping ) ),
	SendPropBool( SENDINFO( m_bFirstJumpFrame ) ),
	SendPropTime( SENDINFO( m_flJumpStartTime ) ),

	SendPropInt( SENDINFO( m_ePlayerAnimEvent ) ),

	SendPropDataTable( "sdksharedlocaldata", 0, &REFERENCE_SEND_TABLE(DT_SDKSharedLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
END_SEND_TABLE()
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

BEGIN_SEND_TABLE_NOBASE( CSDKPlayer, DT_SDKLocalPlayerExclusive )
	//SendPropInt( SENDINFO( m_iShotsFired ), 8, SPROP_UNSIGNED ),
	// send a hi-res origin to the local player for use in prediction
	//new ios1.1 we need this for free roaming mode - do not remove!
    SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 11, SPROP_CHANGES_OFTEN ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 11, SPROP_CHANGES_OFTEN ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angCamViewAngles, 0), 11, SPROP_CHANGES_OFTEN ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angCamViewAngles, 1), 11, SPROP_CHANGES_OFTEN ),
	SendPropVector(SENDINFO(m_vTargetPos), -1, SPROP_COORD),
	SendPropBool(SENDINFO(m_bIsAtTargetPos)),
	SendPropBool(SENDINFO(m_bHoldAtTargetPos)),
	SendPropTime( SENDINFO( m_flNextClientSettingsChangeTime ) ),
	SendPropTime(SENDINFO(m_flNextJoin)),
	SendPropBool( SENDINFO( m_bChargedshotBlocked ) ),
	SendPropBool( SENDINFO( m_bShotsBlocked ) ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CSDKPlayer, DT_SDKNonLocalPlayerExclusive )
	// send a lo-res origin to other players
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 11, SPROP_CHANGES_OFTEN ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 11, SPROP_CHANGES_OFTEN ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angCamViewAngles, 0), 11, SPROP_CHANGES_OFTEN ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angCamViewAngles, 1), 11, SPROP_CHANGES_OFTEN ),
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
	SendPropEHandle(SENDINFO(m_pHoldingBall)),
	SendPropInt(SENDINFO(m_nSkinIndex), 3, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nHairIndex), 3, SPROP_UNSIGNED),
	SendPropString(SENDINFO(m_szShoeName)),
	SendPropString(SENDINFO(m_szKeeperGloveName)),
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

	m_angCamViewAngles.Init();

	m_pCurStateInfo = NULL;	// no state yet
	m_nTeamToJoin = TEAM_NONE;
	m_nTeamPosIndexToJoin = 0;
	m_nSpecTeamToJoin = TEAM_SPECTATOR;
	m_nTeamPosIndex = 0;
	m_nPreferredOutfieldShirtNumber = 2;
	m_nPreferredKeeperShirtNumber = 1;
	m_pPlayerBall = NULL;
	m_Shared.m_flPlayerAnimEventStartTime = gpGlobals->curtime;
	m_Shared.m_ePlayerAnimEvent = PLAYERANIMEVENT_NONE;
	m_Shared.m_aPlayerAnimEventStartAngle = vec3_origin;
	m_Shared.m_nPlayerAnimEventStartButtons = 0;
	m_nModelScale = 100;
	m_nSpecTeam = TEAM_SPECTATOR;
	m_ePenaltyState = PENALTY_NONE;
	m_pHoldingBall = NULL;
	m_flNextClientSettingsChangeTime = gpGlobals->curtime;
	m_bReverseSideCurl = false;
	m_bJoinSilently = false;
	SetChargedshotBlocked(false);
	SetShotsBlocked(false);
	m_bAllowPropCreation = false;

	m_szPlayerName[0] = '\0';
	m_szClubName[0] = '\0';
	m_szShirtName[0] = '\0';

	m_nCountryIndex = 0;

	m_nSkinIndex = 0;
	m_nHairIndex = 0;
	m_nSleeveIndex = 0;
	m_szShoeName.GetForModify()[0] = '\0';
	m_szKeeperGloveName.GetForModify()[0] = '\0';
	m_szPlayerBallSkinName[0] = '\0';

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

void CSDKPlayer::CheckAwayState()
{
	// Check if player is away
	if (SDKGameRules()->IsIntermissionState() && (GetTeamNumber() == TEAM_HOME || GetTeamNumber() == TEAM_AWAY))
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
}

void CSDKPlayer::CheckPosChange()
{
	if (m_nTeamToJoin == TEAM_NONE || gpGlobals->curtime < GetNextJoin())
		return;

	if (m_nTeamToJoin != TEAM_SPECTATOR && (IsCardBanned() || GetGlobalTeam(m_nTeamToJoin)->IsPosBlocked(m_nTeamPosIndexToJoin)))
	{
		m_nTeamToJoin = TEAM_NONE;
		return;
	}

	CSDKPlayer *pSwapPartner = NULL;

	if (!IsTeamPosFree(m_nTeamToJoin, m_nTeamPosIndexToJoin, false, &pSwapPartner))
	{
		if (pSwapPartner && pSwapPartner->IsBot())
		{
			char kickcmd[512];
			Q_snprintf(kickcmd, sizeof(kickcmd), "kickid %i Human player taking the position\n", pSwapPartner->GetUserID());
			engine->ServerCommand(kickcmd);
			pSwapPartner = NULL;
		}
		else if (!pSwapPartner || pSwapPartner->GetTeamToJoin() != GetTeamNumber() || pSwapPartner->GetTeamPosIndexToJoin() != GetTeamPosIndex())
			return;
	}

	Vector partnerPos = vec3_invalid;
	QAngle partnerAng = vec3_angle;

	if (pSwapPartner)
	{
		partnerPos = pSwapPartner->GetLocalOrigin();
		partnerAng = pSwapPartner->GetLocalAngles();
		pSwapPartner->ChangeTeam();
		pSwapPartner->SetPositionAfterTeamChange(GetLocalOrigin(), EyeAngles(), false);
	}

	ChangeTeam();

	if (pSwapPartner)
	{
		pSwapPartner->SetPositionAfterTeamChange(partnerPos, partnerAng, false);
	}
	else if (SDKGameRules()->IsIntermissionState())
	{
		Vector pos = GetSpawnPos();
		SetPositionAfterTeamChange(pos, GetAngleToBall(pos, true), true);
	}
	else
	{
		Vector pos = GetTeam()->GetLastPlayerCoordsByPosIndex(GetTeamPosIndex());

		if (pos != vec3_invalid)
		{
			SetPositionAfterTeamChange(pos, GetAngleToBall(pos, true), true);
		}
		else if (GetTeamPosType() != POS_GK)
		{
			pos = SDKGameRules()->m_vKickOff;

			pos.y -= 50 * GetTeam()->m_nForward;

			if (GetLocalOrigin().x > SDKGameRules()->m_vKickOff.GetX())
				pos.x = SDKGameRules()->m_vFieldMax.GetX() - 50;
			else
				pos.x = SDKGameRules()->m_vFieldMin.GetX() + 50;

			SetPositionAfterTeamChange(pos, GetAngleToBall(pos, true), true);
		}
		else
		{
			pos = GetSpawnPos();
			SetPositionAfterTeamChange(pos, GetAngleToBall(pos, true), true);
		}
	}
}

void CSDKPlayer::PreThink(void)
{
	// Reset the block time when the card ban ends
	if (GetData()->GetNextCardJoin() != 0 && !IsCardBanned())
		GetData()->SetNextCardJoin(0);

	CheckAwayState();

	CheckPosChange();

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

	if (IsBot())
		m_angCamViewAngles = angles;
	else
		m_angCamViewAngles = m_aCamViewAngles;
	
	//LookAtBall();

	//m_BallInPenaltyBox = -1;
}

void CSDKPlayer::LookAtBall(void)
{
	return;

	CBall *pBall;
	
	if (SDKGameRules()->IsIntermissionState())
		pBall = GetNearestPlayerBall(GetLocalOrigin());
	else
		pBall = GetMatchBall();

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
	m_Shared.m_bWasJumping = false;

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
	{
		ChangeTeam();

		if (GetTeamNumber() == TEAM_HOME || GetTeamNumber() == TEAM_AWAY)
		{
			Vector pos = GetSpawnPos();
			SetPositionAfterTeamChange(pos, GetAngleToBall(pos, true), true);
		}
	}

	return true;
}

void CSDKPlayer::ChangeTeam()
{
	if (m_nTeamToJoin == GetTeamNumber() && m_nTeamPosIndexToJoin == GetTeamPosIndex() && m_nSpecTeamToJoin == m_nSpecTeam)
	{
		m_nTeamToJoin = TEAM_NONE;
		m_nTeamPosIndexToJoin = 0;
		m_nSpecTeamToJoin = TEAM_NONE;
		//m_bSetNextJoinDelay = false;

		return;
	}

	GetData()->EndCurrentMatchPeriod();

	if (GetTeam())
		GetTeam()->RemovePlayer(this);

	if (m_nTeamToJoin != TEAM_NONE)
		GetGlobalTeam(m_nTeamToJoin)->AddPlayer(this, m_nTeamPosIndexToJoin);

	int oldTeam = GetTeamNumber();
	SetTeamNumber(m_nTeamToJoin);
	m_nTeamToJoin = TEAM_NONE;

	int oldPosIndex = m_nTeamPosIndex;
	int oldPosType = GetTeamPosType();
	m_nTeamPosIndex = m_nTeamPosIndexToJoin;
	m_nTeamPosIndexToJoin = 0;

	int oldSpecTeam = m_nSpecTeam;
	m_nSpecTeam = m_nSpecTeamToJoin;
	m_nSpecTeamToJoin = TEAM_NONE;

	Reset();

	// update client state 
	if (GetTeamNumber() == TEAM_NONE || GetTeamNumber() == TEAM_SPECTATOR)
	{
		if (State_Get() != PLAYER_STATE_OBSERVER_MODE)
			State_Transition(PLAYER_STATE_OBSERVER_MODE);
	}
	else // active player
	{
		if (!SDKGameRules()->IsIntermissionState())
			GetData()->StartNewMatchPeriod();

		m_nShirtNumber = FindAvailableShirtNumber();

		static int handBodyGroup = FindBodygroupByName("hands");
		SetBodygroup(handBodyGroup, GetTeamPosType() == POS_GK ? 1 : 0);

		static int collarBodyGroup = FindBodygroupByName("collar");
		CTeamKitInfo *pTeamKitInfo = CTeamInfo::FindTeamByKitName(GetTeam()->GetKitName());
		SetBodygroup(collarBodyGroup, GetTeamPosType() == POS_GK ? pTeamKitInfo->m_bKeeperHasCollar : pTeamKitInfo->m_bOutfieldHasCollar);

		if (State_Get() != PLAYER_STATE_ACTIVE)
			State_Transition(PLAYER_STATE_ACTIVE);
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

void CSDKPlayer::SetPositionAfterTeamChange(const Vector &pos, const QAngle &ang, bool findSafePos)
{
	Vector safePos = pos;

	if (findSafePos)
		FindSafePos(safePos);

	QAngle modelAng = ang;
	modelAng[PITCH] = 0;

	SetLocalVelocity(vec3_origin);
	SetLocalAngles(modelAng);
	SetLocalOrigin(safePos);
	SnapEyeAngles(ang);

	if (SDKGameRules()->m_nShieldType != SHIELD_NONE)
		SetPosOutsideShield(true);
}

extern ConVar mp_pitchup, mp_pitchdown;

QAngle CSDKPlayer::GetAngleToBall(const Vector &pos, bool centerPitch)
{
	Vector dir = GetMatchBall()->GetPos() - pos;
	QAngle ang;
	VectorAngles(dir, ang);

	if (centerPitch)
		ang[PITCH] = 0;
	else
		ang[PITCH] = clamp(AngleNormalize(ang[PITCH]), -mp_pitchup.GetFloat(), mp_pitchdown.GetFloat());

	return ang;
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

extern ConVar sv_singlekeeper;

bool CSDKPlayer::ClientCommand( const CCommand &args )
{
	if (!Q_stricmp(args[0], "jointeam")) 
	{
		if (args.ArgC() < 3)
		{
			Warning("Player sent bad jointeam syntax\n");
			return false;
		}

		int team = atoi(args[1]);
		int posIndex = atoi(args[2]);

		if (posIndex < 0 || posIndex > mp_maxplayers.GetInt() - 1 || team < TEAM_SPECTATOR || team > TEAM_AWAY)
			return false;

		// Player is card banned or position is blocked due to a card ban
		if (team != TEAM_SPECTATOR && (IsCardBanned() || GetGlobalTeam(team)->IsPosBlocked(posIndex)))
			return false;

		CSDKPlayer *pSwapPartner = NULL;

		// Notify the previous swap partner that the player is cancelling his swap request
		if ((m_nTeamToJoin == TEAM_HOME || m_nTeamToJoin == TEAM_AWAY)
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
			m_nTeamToJoin = TEAM_NONE;
			return false;
		}

		// Notify the player on the target position that this player wants to swap
		if ((team == TEAM_HOME || team == TEAM_AWAY)
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
		if (GetTeamNumber() != TEAM_HOME && GetTeamNumber() != TEAM_AWAY)
			return true;

		if (GetTeamNumber() == TEAM_HOME && !mp_captaincy_home.GetBool() || GetTeamNumber() == TEAM_AWAY && !mp_captaincy_away.GetBool())
			return true;

		if (this == GetTeam()->GetCaptain())
			GetTeam()->SetCaptainPosIndex(-1);
		else
			GetTeam()->SetCaptainPosIndex(GetTeamPosIndex());

		return true;
	}
	else if (!Q_strcmp(args[0], "settaker"))
	{
		if (args.ArgC() < 2)
			return false;

		if (this != GetTeam()->GetCaptain()
			|| !CSDKPlayer::IsOnField(GetMatchBall()->GetCurrentPlayer(), GetTeamNumber())
			|| (GetMatchBall()->State_Get() != BALL_STATE_GOALKICK
				&& GetMatchBall()->State_Get() != BALL_STATE_FREEKICK
				&& GetMatchBall()->State_Get() != BALL_STATE_CORNER
				&& GetMatchBall()->State_Get() != BALL_STATE_PENALTY
				&& GetMatchBall()->State_Get() != BALL_STATE_THROWIN))
			return true;

		int playerIndex = atoi(args[1]);

		CSDKPlayer *pPl = (CSDKPlayer*)UTIL_PlayerByIndex(playerIndex);

		if (pPl && pPl->GetTeam() == GetTeam() && !pPl->IsBot())
			GetMatchBall()->SetSetpieceTaker(pPl);

		return true;
	}
	else if (!Q_strcmp(args[0], "captain_increase_offensive_level"))
	{
		if (this != GetTeam()->GetCaptain() || (GetTeamNumber() != TEAM_HOME && GetTeamNumber() != TEAM_AWAY))
			return true;

		GetTeam()->IncreaseOffensiveLevel();

		return true;
	}
	else if (!Q_strcmp(args[0], "captain_decrease_offensive_level"))
	{
		if (this != GetTeam()->GetCaptain() || (GetTeamNumber() != TEAM_HOME && GetTeamNumber() != TEAM_AWAY))
			return true;

		GetTeam()->DecreaseOffensiveLevel();

		return true;
	}

	return BaseClass::ClientCommand (args);
}

bool CSDKPlayer::IsTeamPosFree(int team, int posIndex, bool ignoreBots, CSDKPlayer **pPlayerOnPos)
{
	if (posIndex < 0 || posIndex > mp_maxplayers.GetInt() - 1)
		return false;

	if (team == TEAM_SPECTATOR || team == TEAM_NONE)
		return true;

	if (GetGlobalTeam(team)->GetFormation()->positions[posIndex]->type == POS_GK && !IsBot() && !humankeepers.GetBool())
		return false;

	if (GetGlobalTeam(team)->IsPosBlocked(posIndex))
		return false;

	for (int i = 1; i <= gpGlobals->maxClients; i++)	
	{
		CSDKPlayer *pPl = (CSDKPlayer*)UTIL_PlayerByIndex(i);

		if (!pPl || pPl == this)
			continue;

		if (pPl->GetTeamNumber() == team && pPl->GetTeamPosIndex() == posIndex)
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

extern ConVar sv_ball_timelimit_remotecontrolled;

// Make sure that all players are walked to the intended positions when setting shields
bool CSDKPlayer::CheckPlayersAtShieldPos(bool waitUntilOutsideShield)
{
	bool playersAtTargetPos = true;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl))
			continue;

		if (pPl->m_bIsAtTargetPos)
			continue;

		if (!(pPl->GetFlags() & FL_REMOTECONTROLLED))
			pPl->SetPosOutsideShield(false);

		if (pPl->m_bIsAtTargetPos)
			continue;

		if (pPl->m_flRemoteControlledStartTime == -1)
			pPl->m_flRemoteControlledStartTime = gpGlobals->curtime;

		if (gpGlobals->curtime >= pPl->m_flRemoteControlledStartTime + sv_ball_timelimit_remotecontrolled.GetFloat()) // Player timed out and blocks progress, so move him to specs
			pPl->SetDesiredTeam(TEAM_SPECTATOR, pPl->GetTeamNumber(), 0, true, false, false);
		else if (waitUntilOutsideShield || (pPl->GetFlags() & FL_SHIELD_KEEP_IN))
			playersAtTargetPos = false;
	}

	return playersAtTargetPos;
}

void CSDKPlayer::SetPosInsideShield(const Vector &pos, bool holdAtTargetPos)
{
	RemoveFlag(FL_SHIELD_KEEP_OUT);
	AddFlag(FL_SHIELD_KEEP_IN);
	m_vTargetPos = pos;
	m_bHoldAtTargetPos = holdAtTargetPos;

	if (m_vTargetPos == GetLocalOrigin())
	{
		m_bIsAtTargetPos = true;
		if (m_bHoldAtTargetPos)
			AddFlag(FL_ATCONTROLS);
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
		m_vTargetPos = GetSpawnPos();
		break;
	case SHIELD_KEEPERHANDS:
		return;
		break;
	case SHIELD_CEREMONY:
		GetTargetPos(GetLocalOrigin(), m_vTargetPos.GetForModify());
		m_bHoldAtTargetPos = true;
		break;
	case SHIELD_PENALTY:
		GetTargetPos(GetLocalOrigin(), m_vTargetPos.GetForModify());

		if (SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
			m_bHoldAtTargetPos = true;
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

void CSDKPlayer::ActivateRemoteControlling(const Vector &targetPos)
{
	m_vTargetPos = targetPos;
	m_bIsAtTargetPos = false;
	DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
	AddFlag(FL_REMOTECONTROLLED);
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

	if (SDKGameRules()->m_nShieldType == SHIELD_CEREMONY)
	{
		targetPos = SDKGameRules()->m_vKickOff;
		targetPos.y -= GetTeam()->m_nForward * 50;
		targetPos.x = SDKGameRules()->m_vFieldMin.GetX() + GetTeamPosIndex() * 50;
	}

	if (SDKGameRules()->m_nShieldType == SHIELD_PENALTY && SDKGameRules()->State_Get() == MATCH_PERIOD_PENALTIES)
	{
		targetPos = SDKGameRules()->m_vKickOff;
		targetPos.x += (100 + GetTeamPosIndex() * 50) * GetTeam()->m_nForward;
	}

	if (targetPos == vec3_invalid)
		targetPos = pos;
}

bool CSDKPlayer::IsOnField(CSDKPlayer *pPl, int teamNumber/* = TEAM_NONE*/)
{
	return (pPl && pPl->IsConnected() // Is on server
		&& ((teamNumber == TEAM_NONE && (pPl->GetTeamNumber() == TEAM_HOME || pPl->GetTeamNumber() == TEAM_AWAY)) // No specific team given - is on field
			|| (teamNumber != TEAM_NONE && pPl->GetTeamNumber() == teamNumber))); // Specific team given - is in this team
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

Vector CSDKPlayer::GetSpawnPos()
{
	Vector area = (SDKGameRules()->m_vFieldMax - SDKGameRules()->m_vFieldMin);
	area.y /= 2;
	float xOffset = 300;
	float yOffset = 150;
	float yKickOffOffset = mp_shield_kickoff_radius_opponent.GetInt() + mp_shield_border.GetInt();
	float xFrac = clamp(GetTeam()->GetFormation()->positions[GetTeamPosIndex()]->x / 3.0f, 0.0f, 1.0f);
	float yFrac = clamp(GetTeam()->GetFormation()->positions[GetTeamPosIndex()]->y / 3.0f, 0.0f, 1.0f);
	float xPos = xOffset + xFrac * (area.x - 2 * xOffset);
	float yPos = yOffset + yKickOffOffset + yFrac * (area.y - 2 * yOffset - yKickOffOffset);

	Vector spawnPos;
	if (GetTeam()->m_nForward == 1)
		spawnPos = Vector(SDKGameRules()->m_vFieldMin.GetX(), SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ()) + Vector(xPos, -yPos, 0);
	else
		spawnPos = Vector(SDKGameRules()->m_vFieldMax.GetX(), SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ()) + Vector(-xPos, yPos, 0);

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

void CSDKPlayer::Reset()
{
	m_Shared.SetStamina(100);
	InitSprinting();
	m_flNextShot = gpGlobals->curtime;
	m_flNextFoulCheck = gpGlobals->curtime;
	m_bIsAtTargetPos = false;
	RemoveFlags();
	DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
	m_pHoldingBall = NULL;
	m_bIsAway = true;
	m_flLastMoveTime = gpGlobals->curtime;
	//m_flNextJoin = gpGlobals->curtime;
	m_bIsOffside = false;
	m_ePenaltyState = PENALTY_NONE;
	m_flRemoteControlledStartTime = -1;
	SetChargedshotBlocked(false);
	SetShotsBlocked(false);

	if ((GetTeamNumber() == TEAM_HOME || GetTeamNumber() == TEAM_AWAY) && !ReplayManager()->IsReplaying())
	{
		RemoveSolidFlags(FSOLID_NOT_SOLID);
		SetCollisionGroup(COLLISION_GROUP_PLAYER);
		RemoveEffects(EF_NODRAW);
	}

	if (GetPlayerBall())
		GetPlayerBall()->RemovePlayerBall();

	GetMatchBall()->RemovePlayerAssignments(this);
}

void CSDKPlayer::RemoveFlags()
{
	RemoveFlag(FL_SHIELD_KEEP_IN | FL_SHIELD_KEEP_OUT | FL_REMOTECONTROLLED | FL_FREECAM | FL_CELEB | FL_USE_TV_CAM | FL_NO_X_MOVEMENT | FL_NO_Y_MOVEMENT | FL_ATCONTROLS | FL_FROZEN | FL_ONLY_XY_MOVEMENT);
}

bool CSDKPlayer::CanShoot()
{
	return gpGlobals->curtime >= m_flNextShot;
}

void CSDKPlayer::SetChargedshotBlocked(bool blocked)
{
	m_bChargedshotBlocked = IsBot() ? false : blocked;
}

bool CSDKPlayer::ChargedshotBlocked()
{
	return m_bChargedshotBlocked;
}

void CSDKPlayer::SetShotsBlocked(bool blocked)
{
	m_bShotsBlocked = blocked;
}

bool CSDKPlayer::ShotsBlocked()
{
	return m_bShotsBlocked;
}

bool CSDKPlayer::IsCardBanned()
{
	return SDKGameRules()->GetMatchDisplayTimeSeconds(true, false) < GetData()->GetNextCardJoin();
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

	if (SDKGameRules()->IsIntermissionState() && SDKGameRules()->State_Get() != MATCH_PERIOD_COOLDOWN && mp_chat_intermissions_captain_only.GetBool() || !SDKGameRules()->IsIntermissionState() && mp_chat_match_captain_only.GetBool())
		return false;

	return true;
}

void CSDKPlayer::RemoveProps()
{
	while (m_PlayerProps.Count() > 0)
	{
		if (m_PlayerProps[0])
			UTIL_Remove(m_PlayerProps[0]);

		m_PlayerProps.Remove(0);
	}
}

void CSDKPlayer::AllowPropCreation(bool allow)
{
	if (m_bAllowPropCreation && !allow)
		RemoveProps();

	m_bAllowPropCreation = allow;
}

void CSDKPlayer::RemoveAllPlayerProps()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		pPl->RemoveProps();
	}
}

void CC_SV_PropCreation(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
        return;

	if (args.ArgC() < 3)
	{
		Msg("Usage: sv_propcreation <userid> <allowed>{0/1}\nUse userid -1 to allow or disallow prop creation for all players\n");
		return;
	}

	int userId = atoi(args[1]);
	bool allowPropCreation = atoi(args[2]) == 1 ? true : false;

	if (userId == -1)
	{
		Msg("%s prop creation for all players\n", allowPropCreation ? "Allowing" : "Disallowing");

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
			if (!pPl)
				continue;

			pPl->AllowPropCreation(allowPropCreation);
		}
	}
	else
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByUserId(userId));

		if (pPl)
		{
			pPl->AllowPropCreation(allowPropCreation);
			Msg("Prop creation %s for player with user id %d\n", allowPropCreation ? "allowed" : "disallowed", userId);
		}
		else
			Msg("Player with user id %d not found\n", userId);
	}
}

static ConCommand sv_propcreation("sv_propcreation", CC_SV_PropCreation);

#define PLAYER_PROP_COUNT 4

const char *g_szPlayerProps[PLAYER_PROP_COUNT] =
{
	"gb/posts/regularposts",
	"grandstand/net",
	"bench/bench",
	"floodlights/floodlightbar"
};

void CC_CreateProp(const CCommand &args)
{
	CSDKPlayer *pPl = ToSDKPlayer(UTIL_GetCommandClient());
	if (!pPl)
		return;

	if (args.ArgC() < 2)
	{
		char msg[256];

		Q_snprintf(msg, sizeof(msg), "Usage: createprop <proptype>{1-%d} <zoffset> <pitchrotation>\n", PLAYER_PROP_COUNT);

		ClientPrint(pPl, HUD_PRINTCONSOLE, msg);
		return;
	}

	if (!pPl->IsPropCreationAllowed())
	{
		ClientPrint(pPl, HUD_PRINTCONSOLE, "You are not allowed to create props\n");
		return;
	}

	if (!CSDKPlayer::IsOnField(pPl) || !SDKGameRules()->IsIntermissionState() || pPl->GetFlags() & FL_REMOTECONTROLLED)
		return;

	int propType = clamp(atoi(args[1]) - 1, 0, PLAYER_PROP_COUNT - 1);

	const char *modelName = g_szPlayerProps[propType];

	trace_t tr;

	Vector pos = pPl->GetLocalOrigin();
	pos.z = SDKGameRules()->m_vKickOff.GetZ();
	pos += pPl->EyeDirection2D() * 200;

	if (args.ArgC() >= 3)
		pos.z += atoi(args[2]);

	QAngle ang = pPl->GetLocalAngles();

	if (args.ArgC() >= 4)
		ang[PITCH] += atof(args[3]);

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache(true);

	CDynamicProp *pProp = static_cast<CDynamicProp *>(CreateEntityByName("prop_dynamic_override"));

	if (!pProp)
		return;

	char buf[512];
	// Pass in standard key values
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", pos.x, pos.y, pos.z);
	pProp->KeyValue("origin", buf);
	Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", ang.x, ang.y, ang.z);
	pProp->KeyValue("angles", buf);
	Q_snprintf(buf, sizeof(buf), "models/%s.mdl", modelName);
	pProp->KeyValue("model", buf);
	pProp->KeyValue("fademindist", "-1");
	pProp->KeyValue("fademaxdist", "0");
	pProp->KeyValue("fadescale", "1");
	pProp->KeyValue("solid", "6");
	pProp->Precache();
	DispatchSpawn(pProp);
	pProp->Activate();

	pPl->m_PlayerProps.AddToTail(pProp);

	ClientPrint(pPl, HUD_PRINTCONSOLE, "Prop created\n");

	CBaseEntity::SetAllowPrecache(allowPrecache);
}

static ConCommand createprop("createprop", CC_CreateProp);


void CC_DeleteProp(const CCommand &args)
{
	CSDKPlayer *pPl = ToSDKPlayer(UTIL_GetCommandClient());
	if (!pPl)
		return;

	if (args.ArgC() < 2)
	{
		char msg[256];

		Q_snprintf(msg, sizeof(msg), "Usage: deleteprop <proptype>{1-%d}\nDeletes the prop you're looking at filtered by type\nUse 0 for any type\nUse -1 to delete all of your props\n", PLAYER_PROP_COUNT);

		ClientPrint(pPl, HUD_PRINTCONSOLE, msg);
		return;
	}


	if (!pPl->IsPropCreationAllowed() || !SDKGameRules()->IsIntermissionState() || pPl->GetFlags() & FL_REMOTECONTROLLED)
		return;

	int propType = clamp(atoi(args[1]), -1, PLAYER_PROP_COUNT);

	if (propType == -1)
	{
		pPl->RemoveProps();
		ClientPrint(pPl, HUD_PRINTCONSOLE, "All props deleted\n");
		return;
	}
	else if (CSDKPlayer::IsOnField(pPl))
	{
		trace_t tr;

		UTIL_TraceLine(
			pPl->GetLocalOrigin() + VEC_VIEW,
			pPl->GetLocalOrigin() + VEC_VIEW + pPl->EyeDirection3D() * 500,
			MASK_SOLID,
			pPl,
			COLLISION_GROUP_NONE,
			&tr);

		CDynamicProp *pProp = dynamic_cast<CDynamicProp *>(tr.m_pEnt);

		if (pProp)
		{
			int index = pPl->m_PlayerProps.Find(pProp);

			if (index > -1)
			{
				CDynamicProp *pProp = pPl->m_PlayerProps[index];
				char modelName[1024];
				Q_snprintf(modelName, sizeof(modelName), "models/%s.mdl", g_szPlayerProps[propType - 1]);

				if (propType == 0 || propType > 0 && !Q_strcmp(pProp->GetModelName().ToCStr(), modelName))
				{
					UTIL_Remove(pProp);
					pPl->m_PlayerProps.Remove(index);
					ClientPrint(pPl, HUD_PRINTCONSOLE, "Prop deleted\n");
					return;
				}
			}
		}
	}

	ClientPrint(pPl, HUD_PRINTCONSOLE, "No prop found\n");
}

static ConCommand deleteprop("deleteprop", CC_DeleteProp);



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

	if (GetData())
		GetData()->SetLastKnownName(m_szPlayerName);

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
	char sanitizedName[MAX_CLUBNAME_LENGTH];

	Q_strncpy(sanitizedName, name, sizeof(sanitizedName));

	sanitize(sanitizedName);

	trimstr(sanitizedName);

	if (!Q_strcmp(sanitizedName, m_szClubName))
		return;

	Q_strncpy(m_szClubName, sanitizedName, sizeof(m_szClubName));

	m_bClubNameChanged = true;
}

const char *CSDKPlayer::GetNationalTeamName()
{
	return m_szNationalTeamName;
}

void CSDKPlayer::SetNationalTeamName(const char *name)
{
	char sanitizedName[MAX_CLUBNAME_LENGTH];

	Q_strncpy(sanitizedName, name, sizeof(sanitizedName));

	sanitize(sanitizedName);

	trimstr(sanitizedName);

	if (!Q_strcmp(sanitizedName, m_szNationalTeamName))
		return;

	Q_strncpy(m_szNationalTeamName, sanitizedName, sizeof(m_szNationalTeamName));

	m_bNationalTeamNameChanged = true;
}

const char *CSDKPlayer::GetShirtName()
{
	return m_szShirtName;
}

void CSDKPlayer::SetShirtName(const char *name)
{
	char sanitizedName[MAX_SHIRT_NAME_LENGTH];

	Q_strncpy(sanitizedName, name, sizeof(sanitizedName));

	sanitize(sanitizedName);

	trimstr(sanitizedName);

	if (!Q_strcmp(sanitizedName, m_szShirtName))
		return;

	Q_strncpy(m_szShirtName, sanitizedName, sizeof(m_szShirtName));

	if (GetData())
		GetData()->SetLastKnownShirtName(m_szShirtName);

	m_bShirtNameChanged = true;
}

void CSDKPlayer::DrawDebugGeometryOverlays(void) 
{
	BaseClass::DrawDebugGeometryOverlays();
}

void CSDKPlayer::SetShoeName(const char *shoeName)
{
	if (shoeName[0] != '\0' && !Q_strcmp(shoeName, m_szShoeName))
		return;

	int shoeIndex = -1;

	for (int i = 0; i < CShoeInfo::m_ShoeInfo.Count(); i++)
	{
		if (!Q_strcmp(CShoeInfo::m_ShoeInfo[i]->m_szFolderName, shoeName))
		{
			shoeIndex = i;
			break;
		}
	}

	if (shoeIndex == -1)
		shoeIndex = g_IOSRand.RandomInt(0, CShoeInfo::m_ShoeInfo.Count() - 1);

	Q_strncpy(m_szShoeName.GetForModify(), CShoeInfo::m_ShoeInfo[shoeIndex]->m_szFolderName, sizeof(m_szShoeName));
}

void CSDKPlayer::SetKeeperGloveName(const char *keeperGloveName)
{
	if (keeperGloveName[0] != '\0' && !Q_strcmp(keeperGloveName, m_szKeeperGloveName))
		return;

	int keeperGloveIndex = -1;

	for (int i = 0; i < CKeeperGloveInfo::m_KeeperGloveInfo.Count(); i++)
	{
		if (!Q_strcmp(CKeeperGloveInfo::m_KeeperGloveInfo[i]->m_szFolderName, keeperGloveName))
		{
			keeperGloveIndex = i;
			break;
		}
	}

	if (keeperGloveIndex == -1)
		keeperGloveIndex = g_IOSRand.RandomInt(0, CKeeperGloveInfo::m_KeeperGloveInfo.Count() - 1);

	Q_strncpy(m_szKeeperGloveName.GetForModify(), CKeeperGloveInfo::m_KeeperGloveInfo[keeperGloveIndex]->m_szFolderName, sizeof(m_szKeeperGloveName));
}

void CSDKPlayer::SetPlayerBallSkinName(const char *skinName)
{
	for (int i = 0; i < CBallInfo::m_BallInfo.Count(); i++)
	{
		if (!Q_strcmp(CBallInfo::m_BallInfo[i]->m_szFolderName, skinName))
		{
			Q_strncpy(m_szPlayerBallSkinName, skinName, sizeof(m_szPlayerBallSkinName));
			break;
		}
	}
}

int CSDKPlayer::GetSkinIndex()
{
	return m_nSkinIndex;
}

void CSDKPlayer::SetSkinIndex(int index)
{
	m_nSkinIndex = clamp(index, 0, PLAYER_SKIN_COUNT - 1);
	static int headBodyGroup = FindBodygroupByName("head");

	int headIndex;

	switch (m_nSkinIndex)
	{
	case 0: case 1: case 2: default: headIndex = 0; break;
	case 3: headIndex = 1; break;
	case 4: headIndex = 2; break;
	}

	SetBodygroup(headBodyGroup, headIndex);
}

int CSDKPlayer::GetHairIndex()
{
	return m_nHairIndex;
}

void CSDKPlayer::SetHairIndex(int index)
{
	m_nHairIndex = clamp(index, 0, PLAYER_HAIR_COUNT - 1);
	static int hairBodyGroup = FindBodygroupByName("hair");
	SetBodygroup(hairBodyGroup, m_nHairIndex);
}

int CSDKPlayer::GetSleeveIndex()
{
	return m_nSleeveIndex;
}

void CSDKPlayer::SetSleeveIndex(int index)
{
	m_nSleeveIndex = clamp(index, 0, PLAYER_SLEEVE_COUNT - 1);
	static int sleeveBodyGroup = FindBodygroupByName("sleeves");
	SetBodygroup(sleeveBodyGroup, m_nSleeveIndex);

	static int armBodyGroup = FindBodygroupByName("arms");
	SetBodygroup(armBodyGroup, m_nSleeveIndex == 0 ? 1 : 0);
}