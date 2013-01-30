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
	SendPropInt(SENDINFO(m_nInPenBoxOfTeam)),
	SendPropVector(SENDINFO(m_vTargetPos), -1, SPROP_NOSCALE),
	SendPropBool(SENDINFO(m_bIsAtTargetPos)),
	SendPropBool(SENDINFO(m_bHoldAtTargetPos)),
	SendPropTime( SENDINFO( m_flNextClientSettingsChangeTime ) ),
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
	m_bShotButtonsReleased = false;
	m_nTeamToJoin = TEAM_INVALID;
	m_nTeamPosIndexToJoin = 0;
	//m_flNextJoin = gpGlobals->curtime;
	//m_bIsCardBanned = false;
	m_nTeamPosIndex = 0;
	m_nPreferredTeamPosNum = 2;
	m_nPreferredSkin = -1;
	m_pPlayerBall = NULL;
	m_Shared.m_flPlayerAnimEventStartTime = gpGlobals->curtime;
	m_Shared.m_ePlayerAnimEvent = PLAYERANIMEVENT_NONE;
	m_Shared.m_aPlayerAnimEventStartAngle = vec3_origin;
	m_Shared.m_nPlayerAnimEventStartButtons = 0;
	m_nInPenBoxOfTeam = TEAM_INVALID;
	m_ePenaltyState = PENALTY_NONE;
	m_pHoldingBall = NULL;
	m_flNextClientSettingsChangeTime = gpGlobals->curtime;
	m_bLegacySideCurl = false;

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
	if (SDKGameRules()->State_Get() == MATCH_WARMUP && (GetTeamNumber() == TEAM_A || GetTeamNumber() == TEAM_B))
	{
		bool isActive = (m_nButtons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP | IN_DUCK));

		if (isActive)
		{
			m_flLastMoveTime = gpGlobals->curtime;
			m_bIsAway = false;
		}
		else if (gpGlobals->curtime >= m_flLastMoveTime + sv_awaytime_warmup.GetFloat())
		{
			m_bIsAway = true;

			if (gpGlobals->curtime >= m_flLastMoveTime + sv_awaytime_warmup_autospec.GetFloat())
			{
				ChangeTeam(TEAM_SPECTATOR);
				UTIL_ClientPrintAll(HUD_PRINTTALK, "#game_player_benched_away", GetPlayerName());
			}
		}
	}

	if (GetNextCardJoin() != 0 && !SDKGameRules()->IsIntermissionState() && SDKGameRules()->GetMatchDisplayTimeSeconds() >= GetNextCardJoin())
	{
		SetNextCardJoin(0);
	}

	// Remove the player if he's in a team but card banned or in a blocked position
	if (!SDKGameRules()->IsIntermissionState()
		&& (GetTeamNumber() == TEAM_A || GetTeamNumber() == TEAM_B)
		&& (SDKGameRules()->GetMatchDisplayTimeSeconds() < GetNextCardJoin()
		|| SDKGameRules()->GetMatchDisplayTimeSeconds() < GetTeam()->GetPosNextJoinSeconds(GetTeamPosIndex())))
	{
		ChangeTeam(TEAM_SPECTATOR);
	}

	// Prevent the player from reserving a position if he's card banned or if the position is blocked
	if (!SDKGameRules()->IsIntermissionState()
		&& (GetTeamToJoin() == TEAM_A || GetTeamToJoin() == TEAM_B)
		&& (SDKGameRules()->GetMatchDisplayTimeSeconds() < GetNextCardJoin()
		|| SDKGameRules()->GetMatchDisplayTimeSeconds() < GetGlobalTeam(GetTeamToJoin())->GetPosNextJoinSeconds(GetTeamPosIndexToJoin())))
	{
		m_nTeamToJoin = TEAM_INVALID;
	}

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
			int teamToJoin = m_nTeamToJoin;
			if (GetTeamNumber() == TEAM_A || GetTeamNumber() == TEAM_B)
				ChangeTeam(TEAM_SPECTATOR);

			if (pSwapPartner)
			{
				int partnerTeamToJoin = pSwapPartner->GetTeamToJoin();
				if (pSwapPartner->GetTeamNumber() == TEAM_A || pSwapPartner->GetTeamNumber() == TEAM_B)
					pSwapPartner->ChangeTeam(TEAM_SPECTATOR);
				pSwapPartner->m_nTeamToJoin = partnerTeamToJoin;
				pSwapPartner->m_nTeamPosIndex = pSwapPartner->m_nTeamPosIndexToJoin;
				pSwapPartner->ChangeTeam(m_nTeamToJoin);			
			}

			m_nTeamToJoin = teamToJoin;
			m_nTeamPosIndex = m_nTeamPosIndexToJoin;
			ChangeTeam(m_nTeamToJoin);
		}
	}

	//if (m_nRotationTeam != TEAM_INVALID && (SDKGameRules()->IsIntermissionState() || SDKGameRules()->GetMatchDisplayTimeSeconds() >= GetNextJoin()))
	//{
	//	if (TeamPosFree(m_nRotationTeam, m_nRotationTeamPosIndex, false))
	//	{
	//		ChangeTeamPos(m_nRotationTeam, m_nRotationTeamPosIndex, true);
	//	}
	//}

	State_PreThink();

	//UpdateSprint();

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

	m_Shared.m_flNextJump = gpGlobals->curtime;
	m_Shared.m_flNextSlide = gpGlobals->curtime;

	//UseClientSideAnimation();
}

#include "client.h"

bool CSDKPlayer::ChangeTeamPos(int wishTeam, int wishPosIndex, bool setJoinDelay)
{
	if (wishTeam != TEAM_A && wishTeam != TEAM_B && wishTeam != TEAM_SPECTATOR)
		return false;

	if (wishPosIndex < 0 || wishPosIndex > mp_maxplayers.GetInt() - 1)
		return false;

	int oldTeam = GetTeamNumber();

	CSDKPlayer *pPl = NULL;

	if (wishTeam == TEAM_SPECTATOR || IsTeamPosFree(wishTeam, wishPosIndex, false, &pPl))
		ChangeTeam(TEAM_SPECTATOR);

	if (oldTeam == TEAM_A || oldTeam == TEAM_B)
	{
		if (setJoinDelay)
			SetNextJoin(gpGlobals->curtime + mp_joindelay.GetFloat());
		else
			SetNextJoin(gpGlobals->curtime);
	}
	
	if (wishTeam != TEAM_SPECTATOR)
	{
		m_nTeamToJoin = wishTeam;
		m_nTeamPosIndexToJoin = wishPosIndex;
	}

	return true;
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
	if (iTeamNum == iOldTeam)
		return;

	ResetFlags();

	// Immediately tell all clients that he's changing team. This has to be done
	// first, so that all user messages that follow as a result of the team change
	// come after this one, allowing the client to be prepared for them.
	IGameEvent * event = gameeventmanager->CreateEvent( "player_team" );
	if ( event )
	{
		event->SetInt("userid", GetUserID() );
		event->SetInt("team", iTeamNum );
		event->SetInt("oldteam", GetTeamNumber() );
		event->SetBool("disconnect", IsDisconnecting());
		event->SetBool("autoteam", false );
		event->SetBool("silent", false );
		event->SetString("name", GetPlayerName() );
		event->SetInt("teampos", GetTeamPosIndex());

		gameeventmanager->FireEvent( event );
	}

	// Remove him from his current team
	if ( GetTeam() )
		GetTeam()->RemovePlayer( this );

	// Are we being added to a team?
	if ( iTeamNum )
		GetGlobalTeam( iTeamNum )->AddPlayer( this );

	SetTeamNumber(iTeamNum);

	//ResetStats();
	//ResetFlags();

	m_nTeamToJoin = TEAM_INVALID;

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
		//m_nRotationTeam = TEAM_INVALID;
		//m_nRotationTeamPosIndex = 0;

		if( iOldTeam == TEAM_SPECTATOR )
			SetMoveType( MOVETYPE_NONE );

		// transmit changes for player position right away
		//g_pPlayerResource->UpdatePlayerData();

		m_nTeamPosNum = FindUnfilledTeamPosNum();

		if (GetTeamPosType() == GK)
			ChooseKeeperSkin();
		else
			ChoosePlayerSkin();				

		if (iOldTeam != TEAM_A && iOldTeam != TEAM_B)
			State_Transition( STATE_ACTIVE );
	}

	//if (GetTeamNumber() == TEAM_SPECTATOR && GetTeam()->GetCaptain() == this)
	//	GetGlobalTeam(iOldTeam)->FindNewCaptain();
	//else if ((GetTeamNumber() == TEAM_A || GetTeamNumber() == TEAM_B) && GetTeamPosType() == GK)
	//	GetTeam()->FindNewCaptain();

	//GetGlobalTeam(TEAM_A)->FindNewCaptain();
	//GetGlobalTeam(TEAM_B)->FindNewCaptain();

	g_pPlayerResource->UpdatePlayerData();
}

bool isUnfilledNum(int nums[], int num)
{
	if (num == 0 || num == 1)
		return false;

	for (int i = 0; i < 10; i++)
	{
		if (nums[i] == num)
			return false;
	}

	return true;
}

int CSDKPlayer::FindUnfilledTeamPosNum()
{
	if (!mp_custom_shirt_numbers.GetBool())
		return (int)g_Positions[mp_maxplayers.GetInt() - 1][GetTeamPosIndex()][POS_NUMBER];

	if (GetTeamPosType() == GK)
		return 1;

	int teamPosNums[10] = { 0 };

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!CSDKPlayer::IsOnField(pPl) || pPl == this || pPl->GetTeam() != GetTeam() || pPl->GetTeamPosType() == GK)
			continue;

		teamPosNums[pPl->GetTeamPosIndex()] = pPl->GetTeamPosNum();
	}

	int teamPosNum = m_nPreferredTeamPosNum;

	if (!isUnfilledNum(teamPosNums, teamPosNum))
	{
		teamPosNum = (int)g_Positions[mp_maxplayers.GetInt() - 1][GetTeamPosIndex()][POS_NUMBER];

		if (!isUnfilledNum(teamPosNums, teamPosNum))
		{
			for (int i = 2; i <= 11; i++)
			{
				if (isUnfilledNum(teamPosNums, i))
				{
					teamPosNum = i;
					break;
				}
			}
		}
	}

	return teamPosNum;
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
	ChangeTeam(TEAM_SPECTATOR);
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
		{ STATE_ACTIVE,			"STATE_ACTIVE",			&CSDKPlayer::State_ACTIVE_Enter, NULL, &CSDKPlayer::State_ACTIVE_PreThink },
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

void CSDKPlayer::State_OBSERVER_MODE_Enter()
{
	m_iObserverLastMode = OBS_MODE_TVCAM;

	AddEffects(EF_NODRAW);
	SetMoveType(MOVETYPE_OBSERVER);
	AddSolidFlags(FSOLID_NOT_SOLID);
	PhysObjectSleep();

	if ( !IsObserver() )
	{
		// set position to last view offset
		SetAbsOrigin( GetAbsOrigin() + GetViewOffset() );
		SetViewOffset( vec3_origin );
	}
	
	m_afPhysicsFlags |= PFLAG_OBSERVER;

	SetGroundEntity( (CBaseEntity *)NULL );

	SetObserverMode( m_iObserverLastMode );

	if ( gpGlobals->eLoadType != MapLoad_Background )
	{
		ShowViewPortPanel( "specgui" , ModeWantsSpectatorGUI(m_iObserverLastMode) );
	}
}

void CSDKPlayer::State_OBSERVER_MODE_Leave()
{
	m_bForcedObserverMode = false;
	m_afPhysicsFlags &= ~PFLAG_OBSERVER;

	if ( m_iObserverMode == OBS_MODE_NONE )
		return;

	if ( m_iObserverMode  > OBS_MODE_DEATHCAM )
	{
		m_iObserverLastMode = m_iObserverMode;
	}

	m_iObserverMode.Set( OBS_MODE_NONE );

	ShowViewPortPanel( "specmenu", false );
	ShowViewPortPanel( "specgui", false );
	ShowViewPortPanel( "overview", false );
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

	Vector spawnPos = GetSpawnPos(true);
	Vector dir = Vector(0, GetTeam()->m_nForward, 0);
	QAngle ang;
	VectorAngles(dir, ang);
	SetLocalOrigin(spawnPos);
	SetLocalVelocity(vec3_origin);
	SetLocalAngles(ang);
	m_Local.m_vecPunchAngle = vec3_angle;
	m_Local.m_vecPunchAngleVel = vec3_angle;
	SnapEyeAngles(ang);

	m_vPreReplayPos = GetLocalOrigin();
	m_aPreReplayAngles = GetLocalAngles();

	m_flLastShotOnGoal = -1;
	m_flLastMoveTime = gpGlobals->curtime;
	m_bIsAway = false;

	if (SDKGameRules()->m_nShieldType != SHIELD_NONE)
		SetPosOutsideShield();
}

void CSDKPlayer::State_ACTIVE_PreThink()
{
	if (!ShotButtonsPressed())
		SetShotButtonsReleased(true);

	CheckShotCharging();
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
			return false;	//go away
		}

		int team = atoi(args[1]);
		int posIndex = atoi(args[2]);

		if (!SDKGameRules()->IsIntermissionState() && SDKGameRules()->GetMatchDisplayTimeSeconds() < GetNextCardJoin())
			return false;

		if (team == m_nTeamToJoin && posIndex == m_nTeamPosIndexToJoin)
		{
			m_nTeamToJoin = TEAM_INVALID;
			return false;
		}

		// Find team and pos for auto-join
		if (posIndex == -1 || team == TEAM_INVALID)
		{
			int checkTeam = GetGlobalTeam(TEAM_A)->GetNumPlayers() <= GetGlobalTeam(TEAM_B)->GetNumPlayers() ? TEAM_A : TEAM_B;

			for (int i = 0; i < mp_maxplayers.GetInt(); i++)
			{
				CSDKPlayer *pPl = NULL;

				if (IsTeamPosFree(checkTeam, i, false, &pPl))
				{
					team = checkTeam;
					posIndex = i;
					break;
				}
			}
		}

		if (posIndex == -1 || team == TEAM_INVALID)
			return false;

		if (team == GetTeamNumber() && posIndex == GetTeamPosIndex())
			return false;

		if (!SDKGameRules()->IsIntermissionState() && SDKGameRules()->GetMatchDisplayTimeSeconds() < GetGlobalTeam(team)->GetPosNextJoinSeconds(posIndex))
			return false;

		return ChangeTeamPos(team, posIndex, true);
	}
	else if (!Q_stricmp(args[0], "spectate"))
	{
		if (GetTeamNumber() == TEAM_SPECTATOR)
			return false;

		ChangeTeamPos(TEAM_SPECTATOR, 0, true);

		return true;
	}
	else if (!Q_stricmp(args[0], "motmvote"))
	{
		if (args.ArgC() < 3)
		{
			Warning("Need player ids\n");
			return false;
		}

		CSDKPlayer *pPl = NULL;
		pPl = ToSDKPlayer(UTIL_PlayerByIndex(atoi(args[1])));
		if (!pPl)
			return false;
		pPl = ToSDKPlayer(UTIL_PlayerByIndex(atoi(args[2])));
		if (!pPl)
			return false;

		m_nMotmChoiceIds[0] = atoi(args[1]);
		m_nMotmChoiceIds[1] = atoi(args[2]);

		int playerVotes[2][MAX_PLAYERS] = {};
		int playersOnField[2] = { 0, 0 };

		int mostGoals[2] = {};
		int mostGoalsPlayers[2] = {};

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
			if (!CSDKPlayer::IsOnField(pPl))
				continue;

			for (int j = 0; j < 2; j++)
			{
				if (pPl->m_nMotmChoiceIds[j] > 0)
					playerVotes[j][pPl->m_nMotmChoiceIds[j] - 1] += 1;
			}

			playersOnField[pPl->GetTeamNumber() - TEAM_A] += 1;

			if (pPl->GetGoals() > mostGoals[pPl->GetTeamNumber() - TEAM_A])
			{
				mostGoals[pPl->GetTeamNumber() - TEAM_A] = pPl->GetGoals();
				mostGoalsPlayers[pPl->GetTeamNumber() - TEAM_A] = i;
			}
		}

		int mostVotesCount[2] = { 0, 0 };
		int mostVotesPlayer[2] = { 0, 0 };

		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < MAX_PLAYERS; j++)
			{
				if (playerVotes[i][j] > mostVotesCount[i])
				{
					mostVotesCount[i] = playerVotes[i][j];
					mostVotesPlayer[i] = j + 1;
				}
			}
		}
		
		IGameEvent *pEvent = gameeventmanager->CreateEvent("motmvotingresult");
		pEvent->SetInt("playerschoice_player0", mostVotesPlayer[0]);
		pEvent->SetInt("playerschoice_percentage0", mostVotesCount[0] * 100 / max(1, playersOnField[0]));
		pEvent->SetInt("playerschoice_player1", mostVotesPlayer[1]);
		pEvent->SetInt("playerschoice_percentage1", mostVotesCount[1] * 100 / max(1, playersOnField[1]));
		pEvent->SetInt("expertschoice_player0", mostGoalsPlayers[0]);
		pEvent->SetInt("expertschoice_percentage0", 100);
		pEvent->SetInt("expertschoice_player1", mostGoalsPlayers[1]);
		pEvent->SetInt("expertschoice_percentage1", 100);
		gameeventmanager->FireEvent(pEvent);

		return true;
	}
	else if (!Q_stricmp(args[0], "becomecaptain"))
	{

		return true;
	}
	else if (!Q_stricmp(args[0], "requesttimeout"))
	{
		if (this != GetTeam()->GetCaptain())
			return false;

		if (GetTeam()->GetTimeoutsLeft() > 0)
			GetTeam()->SetTimeoutsLeft(GetTeam()->GetTimeoutsLeft() - 1);
		else
			return false;

		GetTeam()->SetWantsTimeout(true);

		return true;
	}
	else if (!Q_stricmp(args[0], "setcaptain"))
	{
		if (this != GetTeam()->GetCaptain())
			return false;

		GetTeam()->SetCaptainPosIndex(atoi(args[1]));
		return true;
	}
	else if (!Q_stricmp(args[0], "setfreekicktaker"))
	{
		if (this != GetTeam()->GetCaptain())
			return false;

		GetTeam()->SetFreekickTakerPosIndex(atoi(args[1]));
		return true;
	}
	else if (!Q_stricmp(args[0], "setpenaltytaker"))
	{
		if (this != GetTeam()->GetCaptain())
			return false;

		GetTeam()->SetPenaltyTakerPosIndex(atoi(args[1]));
		return true;
	}

	return BaseClass::ClientCommand (args);
}

bool CSDKPlayer::IsTeamPosFree(int team, int posIndex, bool ignoreBots, CSDKPlayer **pPlayerOnPos)
{
	if (posIndex < 0 || posIndex > mp_maxplayers.GetInt() - 1)
		return false;

	//if (GetGlobalTeam(team)->GetPosNextJoinSeconds(posIndex) > SDKGameRules()->GetMatchDisplayTimeSeconds())
	//	return false;

	if (g_Positions[mp_maxplayers.GetInt() - 1][posIndex][POS_TYPE] == GK)
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

void CSDKPlayer::SetPreferredSkin(int num)
{
	m_nPreferredSkin = clamp(num, -1, NUM_PLAYER_FACES - 1);

	if (GetTeamNumber() == TEAM_A || GetTeamNumber() == TEAM_B)
	{
		if (GetTeamPosType() == GK)
		{
			int body = m_nBody;
			int ballSkin = m_nSkin - m_nBaseSkin;
			ChooseKeeperSkin();
			m_nBody = body;
			m_nSkin = m_nBaseSkin + ballSkin;
		}
		else
			ChoosePlayerSkin();
	}
}

////////////////////////////////////////////////
// player skins are 0-9 (blocks of 10)
// (shirtpos-2) is always 0-9
//
void CSDKPlayer::ChoosePlayerSkin(void)
{
	int preferred = (m_nPreferredSkin == -1 ? g_IOSRand.RandomInt(0, NUM_PLAYER_FACES - 1) : m_nPreferredSkin);
	m_nSkin = GetTeamPosNum() - 2 + (preferred * 10);		//player skin
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
	int preferred = (m_nPreferredSkin == -1 ? g_IOSRand.RandomInt(0, NUM_PLAYER_FACES - 1) : m_nPreferredSkin);
	m_nSkin = NUM_PLAYER_FACES * 10 + (preferred * NUM_BALL_TYPES);
	m_nBaseSkin = m_nSkin;
	m_nBody = MODEL_KEEPER;
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

void CSDKPlayer::SetPosOutsideShield()
{
	RemoveFlag(FL_SHIELD_KEEP_IN);
	AddFlag(FL_SHIELD_KEEP_OUT);
	m_bHoldAtTargetPos = false;
	m_vTargetPos = vec3_invalid;

	switch (SDKGameRules()->m_nShieldType)
	{
	case SHIELD_KICKOFF:
		//m_vTargetPos = GetTeamPosNum() == 1 ? GetTeam()->m_vPenalty : GetTeam()->m_vPlayerSpawns[GetTeamPosNum() - 1];
		m_vTargetPos = GetSpawnPos(false);
		break;
	case SHIELD_KEEPERHANDS:
		return;
		break;
	default:
		GetTargetPos(GetLocalOrigin(), m_vTargetPos.GetForModify());
		break;
	}

	if (m_vTargetPos == GetLocalOrigin())
	{
		m_bIsAtTargetPos = true;
		if (m_bHoldAtTargetPos)
			AddFlag(FL_ATCONTROLS);
	}
	else
	{
		ActivateRemoteControlling(m_vTargetPos);
	};
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
		float radius = SDKGameRules()->GetShieldRadius() + border;
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

		if (SDKGameRules()->m_nShieldType == SHIELD_FREEKICK && mp_shield_block_6yardbox.GetBool())
		{
			if (GetTeamPosType() != GK || GetTeamNumber() != GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->GetOppTeamNumber())
			{
				int side = GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->GetOppTeamNumber();
				int boxLength = abs(GetGlobalTeam(side)->m_vPenBoxMax.GetY() - GetGlobalTeam(side)->m_vPenBoxMin.GetY()) / 3.0f;
				Vector min = GetGlobalTeam(side)->m_vPenBoxMin + Vector(boxLength * 2, 0, 0) - border;
				Vector max = GetGlobalTeam(side)->m_vPenBoxMax - Vector(boxLength * 2, 0, 0) + border;

				if (GetGlobalTeam(side)->m_nForward == 1)
				{
					max.y -= boxLength * 2;
					min.y -= 500;
				}
				else
				{
					min.y += boxLength * 2;
					max.y += 500;
				}

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
	//Vector spawnPos = pPlayer->GetTeam()->m_vPlayerSpawns[ToSDKPlayer(pPlayer)->GetTeamPosNum() - 1];
	Vector halfField = (SDKGameRules()->m_vFieldMax - SDKGameRules()->m_vFieldMin);
	halfField.y /= 2;
	float xDist = halfField.x / 5;
	float yDist = halfField.y / 5;
	float xPos = g_Positions[mp_maxplayers.GetInt() - 1][GetTeamPosIndex()][POS_XPOS] * xDist + xDist;
	float yPos = g_Positions[mp_maxplayers.GetInt() - 1][GetTeamPosIndex()][POS_YPOS] * yDist + max(mp_shield_freekick_radius.GetInt() + 2 * mp_shield_border.GetInt(), yDist);

	Vector spawnPos;
	if (GetTeam()->m_nForward == 1)
		spawnPos = Vector(SDKGameRules()->m_vFieldMin.GetX(), SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ()) + Vector(xPos, -yPos, 0);
	else
		spawnPos = Vector(SDKGameRules()->m_vFieldMax.GetX(), SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ()) + Vector(-xPos, yPos, 0);

	if (findSafePos)
		FindSafePos(spawnPos);

	return spawnPos;
}

int CSDKPlayer::GetTeamPosNum()
{
	return m_nTeamPosNum;
}

int CSDKPlayer::GetTeamPosType()
{
	return (int)g_Positions[mp_maxplayers.GetInt() - 1][GetTeamPosIndex()][POS_TYPE];
}

void CSDKPlayer::ResetFlags()
{
	m_bIsAtTargetPos = false;
	RemoveFlag(FL_SHIELD_KEEP_IN | FL_SHIELD_KEEP_OUT | FL_REMOTECONTROLLED | FL_FREECAM | FL_CELEB | FL_NO_X_MOVEMENT | FL_NO_Y_MOVEMENT | FL_ATCONTROLS | FL_FROZEN);
	DoServerAnimationEvent(PLAYERANIMEVENT_CANCEL);
	m_pHoldingBall = NULL;
	m_flLastShotOnGoal = -1;
	m_bIsAway = true;
	m_flLastMoveTime = gpGlobals->curtime;
	m_flNextJoin = gpGlobals->curtime;
	m_bIsOffside = false;
	m_ePenaltyState = PENALTY_NONE;
	m_nMotmChoiceIds[0] = 0;
	m_nMotmChoiceIds[1] = 0;
	m_flRemoteControlledStartTime = -1;

	if (GetPlayerBall())
		GetPlayerBall()->RemovePlayerBall();

	if (GetTeamNumber() == TEAM_A || GetTeamNumber() == TEAM_B)
	{
		//RemoveSolidFlags(FSOLID_NOT_SOLID);
		SetCollisionGroup(COLLISION_GROUP_PLAYER);
		RemoveEffects(EF_NODRAW);
	}

	if (GetTeamPosType() == GK && m_nBody == MODEL_KEEPER_AND_BALL)
	{
		m_nBody = MODEL_KEEPER;
	}
}

bool CSDKPlayer::IsNormalshooting(bool noShotOnCancel /*= false*/)
{
	return (m_nButtons & IN_ATTACK) && !IsPowershooting() && (!noShotOnCancel || !(m_nButtons & IN_ATTACK2));
}

bool CSDKPlayer::IsPowershooting(bool noShotOnCancel /*= false*/)
{
	return (m_nButtons & IN_ALT1) && (!noShotOnCancel || !(m_nButtons & IN_ATTACK2));
}

bool CSDKPlayer::IsChargedshooting()
{
	return m_Shared.m_bDoChargedShot;
}

bool CSDKPlayer::IsAutoPassing()
{
	return false; //(m_nButtons & IN_ALT2) != 0;
}

bool CSDKPlayer::IsShooting()
{
	return IsNormalshooting() || IsPowershooting() || IsChargedshooting() || IsAutoPassing();
}

bool CSDKPlayer::ShotButtonsPressed()
{
	return (m_nButtons & (IN_ATTACK | (IN_ATTACK2 | IN_ALT1 | IN_ALT2)));
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

ConVar mp_chat_captain_only("mp_chat_captain_only", "0", FCVAR_NOTIFY);

bool CSDKPlayer::CanSpeak(bool isTeamOnly)
{
	if (!SDKGameRules()->IsIntermissionState() && !isTeamOnly && mp_chat_captain_only.GetBool() && GetTeam()->GetCaptain() != this)
		return false;

	return true;
}

CUtlVector<CPlayerPersistentData *> CPlayerPersistentData::m_PlayerPersistentData;

void CPlayerPersistentData::AllocateData(CSDKPlayer *pPl)
{
	const CSteamID *steamID = engine->GetClientSteamID(pPl->edict());
	const char *sid = engine->GetPlayerNetworkIDString(pPl->edict());
	CPlayerPersistentData *data = NULL;

	for (int i = 0; i < m_PlayerPersistentData.Count(); i++)
	{
		if (m_PlayerPersistentData[i]->m_SteamID != steamID)
			continue;

		data = m_PlayerPersistentData[i];
		break;
	}

	if (!data)
	{
		data = new CPlayerPersistentData;
		data->m_SteamID = engine->GetClientSteamID(pPl->edict());
		Q_strncpy(data->m_szSteamID, engine->GetPlayerNetworkIDString(pPl->edict()), 32);
		data->ResetData();
		m_PlayerPersistentData.AddToTail(data);
	}

	pPl->SetData(data);
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

#include "Filesystem.h"
#include <time.h>

void CPlayerPersistentData::ConvertAllPlayerDataToJson()
{
	const int jsonSize = 20480;
	char *json = new char[jsonSize];
	json[0] = 0;

	Q_strcat(json, "{", jsonSize);

	for (int team = TEAM_A; team <= TEAM_B; team++)
	{
		CTeam *pTeam = GetGlobalTeam(team);

		if (team == TEAM_B)
			Q_strcat(json, ",", jsonSize);

		Q_strcat(json, UTIL_VarArgs("\"%s\":{\"name\":\"%s\",\"goals\":%d,\"possession\":%d,\"matchEvents\":[", (team == TEAM_A ? "homeTeam" : "awayTeam"), GetGlobalTeam(team)->GetShortTeamName(), GetGlobalTeam(team)->GetGoals(), GetGlobalTeam(team)->m_nPossession), jsonSize);
		
		for (int i = 0; i < GetGlobalTeam(team)->m_nMatchEventIndex; i++)
		{
			int minute = ceil(pTeam->m_nMatchEventSeconds[i] / 60.0f);
			char time[16];

			if (pTeam->m_eMatchEventMatchStates[i] == MATCH_FIRST_HALF && minute > 45)
				Q_snprintf(time, sizeof(time), "%d'+%d", 45, min(4, minute - 45));
			else if (pTeam->m_eMatchEventMatchStates[i] == MATCH_SECOND_HALF && minute > 90)
				Q_snprintf(time, sizeof(time), "%d'+%d", 90, min(4, minute - 90));
			else if (pTeam->m_eMatchEventMatchStates[i] == MATCH_EXTRATIME_FIRST_HALF && minute > 105)
				Q_snprintf(time, sizeof(time), "%d'+%d", 105, min(4, minute - 105));
			else if (pTeam->m_eMatchEventMatchStates[i] == MATCH_EXTRATIME_SECOND_HALF && minute > 120)
				Q_snprintf(time, sizeof(time), "%d'+%d", 120, min(4, minute - 120));
			else
				Q_snprintf(time, sizeof(time), "%d'", minute);

			if (i > 0)
				Q_strcat(json, ",", jsonSize);

			Q_strcat(json, UTIL_VarArgs("{\"minute\":\"%s\",\"event\":\"%s\",\"player\":\"%s\"}", time, g_szMatchEventNames[pTeam->m_eMatchEventTypes[i]], pTeam->m_szMatchEventPlayers[i]), jsonSize);
		}
		Q_strcat(json, "]}", jsonSize);
	}

	Q_strcat(json, ",\"players\":[", jsonSize);

	for (int i = 0; i < m_PlayerPersistentData.Count(); i++)
	{
		CPlayerPersistentData *pData = m_PlayerPersistentData[i];

		if (i > 0)
			Q_strcat(json, ",", jsonSize);

		Q_strcat(json, UTIL_VarArgs(
			"{\"steamId\":\"%s\",\"redCards\":%d,\"yellowCards\":%d,\"fouls\":%d,\"foulsSuffered\":%d,\"slidingTackles\":%d,\"slidingTacklesCompleted\":%d,\"goalsConceded\":%d,\"shots\":%d,\"shotsOnGoal\":%d,\"passesCompleted\":%d,\"interceptions\":%d,\"offsides\":%d,\"goals\":%d,\"ownGoals\":%d,\"assists\":%d,\"passes\":%d,\"freeKicks\":%d,\"penalties\":%d,\"corners\":%d,\"throwIns\":%d,\"keeperSaves\":%d,\"goalKicks\":%d,\"possession\":%d,\"distanceCovered\":\"%d\"}",
			pData->m_szSteamID, pData->m_nRedCards, pData->m_nYellowCards, pData->m_nFouls, pData->m_nFoulsSuffered, pData->m_nSlidingTackles, pData->m_nSlidingTacklesCompleted, pData->m_nGoalsConceded, pData->m_nShots, pData->m_nShotsOnGoal, pData->m_nPassesCompleted, pData->m_nInterceptions, pData->m_nOffsides, pData->m_nGoals, pData->m_nOwnGoals, pData->m_nAssists, pData->m_nPasses, pData->m_nFreeKicks, pData->m_nPenalties, pData->m_nCorners, pData->m_nThrowIns, pData->m_nKeeperSaves, pData->m_nGoalKicks, pData->m_nPossession, pData->m_nDistanceCovered
			), jsonSize);
	}

	Q_strcat(json, "]}", jsonSize);

	filesystem->CreateDirHierarchy("statistics", "MOD");

	time_t rawtime;
	struct tm *timeinfo;
	char filename[128];

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(filename, sizeof(filename), "statistics\\matchdata_%Y.%m.%d_%Hh.%Mm.%Ss.json", timeinfo);

	FileHandle_t fh = filesystem->Open(filename, "w", "MOD");
 
	if (fh)
	{
		filesystem->Write(json, Q_strlen(json), fh);  
		filesystem->Close(fh);
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

void CPlayerPersistentData::ResetData()
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
	m_nPossession = 0;
	m_flPossessionTime = 0.0f;
	m_nDistanceCovered = 0;
	m_flExactDistanceCovered = 0.0f;
	m_nNextCardJoin = 0;
}