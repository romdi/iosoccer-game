//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_sdk_player.h"
#include "c_user_message_register.h"
#include "weapon_sdkbase.h"
#include "c_basetempentity.h"
#include "iclientvehicle.h"
#include "prediction.h"
#include "view.h"
#include "iviewrender.h"
#include "ivieweffects.h"
#include "view_scene.h"
#include "fx.h"
#include "collisionutils.h"
#include "c_team.h"
#include "obstacle_pushaway.h"
#include "bone_setup.h"
#include "cl_animevent.h"
#include "sdk_gamerules.h"
#include "c_match_ball.h"
#include "c_player_ball.h"
#include "igameresources.h"
#include "iinput.h"

#include "ios_requiredclientversion.h"

// memdbgon must be the last include file in a .cpp file!!!
ConVar cl_ragdoll_physics_enable( "cl_ragdoll_physics_enable", "1", 0, "Enable/disable ragdoll physics." );
#include "tier0/memdbgon.h"

#if defined( CSDKPlayer )
	#undef CSDKPlayer
#endif

void OnPlayerNameChange(IConVar *var, const char *pOldValue, float flOldValue)
{
	engine->ClientCmd(VarArgs("setinfo name \"%s\"", ((ConVar *)var)->GetString()));
}

ConVar playername("playername", "", FCVAR_USERINFO | FCVAR_ARCHIVE, "Your name", &OnPlayerNameChange);

ConVar shirtname("shirtname", "", FCVAR_USERINFO | FCVAR_ARCHIVE, "Your shirt name");
ConVar clubname("clubname", "", FCVAR_USERINFO | FCVAR_ARCHIVE, "The name of your club");
ConVar nationalteamname("nationalteamname", "", FCVAR_USERINFO | FCVAR_ARCHIVE, "The name of your club");
ConVar countryindex("countryindex", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "The name of your country");
ConVar preferredoutfieldshirtnumber("preferredoutfieldshirtnumber", "2", FCVAR_USERINFO | FCVAR_ARCHIVE, "Your preferred outfield shirt number");
ConVar preferredkeepershirtnumber("preferredkeepershirtnumber", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "Your preferred keeper shirt number");
ConVar legacysidecurl("legacysidecurl", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "");
ConVar keeperkeybindings("keeperkeybindings", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "");
ConVar modelskinindex("modelskinindex", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "");
ConVar playerballskinname("playerballskinname", "", FCVAR_USERINFO | FCVAR_ARCHIVE, "");

ConVar clientversion("clientversion", g_szRequiredClientVersion, FCVAR_USERINFO | FCVAR_HIDDEN, "");

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		// Create the effect.
		C_SDKPlayer *pPlayer = dynamic_cast< C_SDKPlayer* >( m_hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get() );
		}	
	}

public:
	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
END_RECV_TABLE()

void __MsgFunc_ReloadEffect( bf_read &msg )
{
	int iPlayer = msg.ReadShort();
	C_SDKPlayer *pPlayer = dynamic_cast< C_SDKPlayer* >( C_BaseEntity::Instance( iPlayer ) );
	if ( pPlayer )
		pPlayer->PlayReloadEffect();

}
USER_MESSAGE_REGISTER( ReloadEffect );

// CSDKPlayerShared Data Tables
//=============================

// specific to the local player ( ideally should not be in CSDKPlayerShared! )
BEGIN_RECV_TABLE_NOBASE( CSDKPlayerShared, DT_SDKSharedLocalPlayerExclusive )
#if defined ( SDK_USE_PLAYERCLASSES )
	RecvPropInt( RECVINFO( m_iPlayerClass ) ),
	RecvPropInt( RECVINFO( m_iDesiredPlayerClass ) ),
#endif
	RecvPropTime( RECVINFO( m_flNextJump ) ),
	RecvPropTime( RECVINFO( m_flNextSlide ) ),

	RecvPropTime( RECVINFO( m_flPlayerAnimEventStartTime ) ),
	RecvPropVector( RECVINFO( m_aPlayerAnimEventStartAngle ) ),
	RecvPropInt( RECVINFO( m_nPlayerAnimEventStartButtons ) ),

	RecvPropBool( RECVINFO( m_bIsShotCharging ) ),
	RecvPropBool(RECVINFO(m_bDoChargedShot)),
	RecvPropTime( RECVINFO( m_flShotChargingStart ) ),
	RecvPropTime(RECVINFO(m_flShotChargingDuration)),

	RecvPropInt(RECVINFO(m_nInPenBoxOfTeam)),
	RecvPropBool(RECVINFO(m_bShotButtonsReleased)),

	RecvPropInt( RECVINFO( m_nLastPressedSingleMoveKey ) ),
	RecvPropInt( RECVINFO( m_nBoostRightDive ) ),
	RecvPropTime( RECVINFO( m_flBoostRightDiveStart ) ),
	RecvPropInt( RECVINFO( m_nBoostForwardDive ) ),
	RecvPropTime( RECVINFO( m_flBoostForwardDiveStart ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CSDKPlayerShared, DT_SDKPlayerShared )
#if defined ( SDK_USE_STAMINA ) || defined ( SDK_USE_SPRINTING )
	RecvPropFloat( RECVINFO( m_flStamina ) ),
	RecvPropFloat( RECVINFO( m_flMaxStamina ) ),
#endif

#if defined ( SDK_USE_PRONE )
	RecvPropBool( RECVINFO( m_bProne ) ),
	RecvPropTime( RECVINFO( m_flGoProneTime ) ),
	RecvPropTime( RECVINFO( m_flUnProneTime ) ),
#endif
#if defined( SDK_USE_SPRINTING )
	RecvPropBool( RECVINFO( m_bIsSprinting ) ),
#endif

	RecvPropBool( RECVINFO( m_bJumping ) ),
	RecvPropBool( RECVINFO( m_bWasJumping ) ),
	RecvPropBool( RECVINFO( m_bFirstJumpFrame ) ),
	RecvPropTime( RECVINFO( m_flJumpStartTime ) ),

	RecvPropInt( RECVINFO( m_ePlayerAnimEvent ) ),

	RecvPropDataTable( "sdksharedlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_SDKSharedLocalPlayerExclusive) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_SDKPlayer, DT_SDKLocalPlayerExclusive )
	//new ios1.1
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
	RecvPropFloat( RECVINFO( m_angCamViewAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angCamViewAngles[1] ) ),
	RecvPropVector(RECVINFO(m_vTargetPos)),
	RecvPropBool(RECVINFO(m_bIsAtTargetPos)),
	RecvPropBool(RECVINFO(m_bHoldAtTargetPos)),
	RecvPropTime( RECVINFO( m_flNextClientSettingsChangeTime ) ),
	RecvPropTime(RECVINFO(m_flNextJoin)),
	RecvPropBool(RECVINFO(m_bChargedshotBlocked)),
	RecvPropBool(RECVINFO(m_bShotsBlocked)),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_SDKPlayer, DT_SDKNonLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
	RecvPropFloat( RECVINFO( m_angCamViewAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angCamViewAngles[1] ) ),
END_RECV_TABLE()

// main table
IMPLEMENT_CLIENTCLASS_DT( C_SDKPlayer, DT_SDKPlayer, CSDKPlayer )
	RecvPropDataTable( RECVINFO_DT( m_Shared ), 0, &REFERENCE_RECV_TABLE( DT_SDKPlayerShared ) ),

	RecvPropDataTable( "sdklocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_SDKLocalPlayerExclusive) ),
	RecvPropDataTable( "sdknonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_SDKNonLocalPlayerExclusive) ),

	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),

	RecvPropInt( RECVINFO( m_iPlayerState ) ),
	RecvPropTime( RECVINFO( m_flStateEnterTime ) ),

	RecvPropBool( RECVINFO( m_bSpawnInterpCounter ) ),

	RecvPropInt(RECVINFO(m_nModelScale)),
	RecvPropEHandle(RECVINFO(m_pHoldingBall))

END_RECV_TABLE()

// ------------------------------------------------------------------------------------------ //
// Prediction tables.
// ------------------------------------------------------------------------------------------ //
BEGIN_PREDICTION_DATA_NO_BASE( CSDKPlayerShared )
#if defined ( SDK_USE_STAMINA ) || defined ( SDK_USE_SPRINTING )
	DEFINE_PRED_FIELD( m_flStamina, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flMaxStamina, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
#if defined( SDK_USE_PRONE )
	DEFINE_PRED_FIELD( m_bProne, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flGoProneTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flUnProneTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
#if defined( SDK_USE_SPRINTING )
	DEFINE_PRED_FIELD( m_bIsSprinting, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif

	DEFINE_PRED_FIELD( m_flNextJump, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flNextSlide, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_bJumping, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bWasJumping, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bFirstJumpFrame, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flJumpStartTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_bIsShotCharging, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDoChargedShot, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD( m_flShotChargingStart, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flShotChargingDuration, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_ePlayerAnimEvent, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flPlayerAnimEventStartTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD( m_aPlayerAnimEventStartAngle, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerAnimEventStartButtons, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD(m_nInPenBoxOfTeam, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD( m_bShotButtonsReleased, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_nLastPressedSingleMoveKey, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_nBoostRightDive, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flBoostRightDiveStart, FIELD_TIME, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nBoostForwardDive, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flBoostForwardDiveStart, FIELD_TIME, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()

BEGIN_PREDICTION_DATA( C_SDKPlayer )
	DEFINE_PRED_TYPEDESCRIPTION( m_Shared, CSDKPlayerShared ), //ios source 2007 fixes valve wiki
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	//DEFINE_PRED_FIELD( m_iShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),  
	//DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),	
	//DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	//DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( player, C_SDKPlayer );

ConVar cl_ragdoll_fade_time( "cl_ragdoll_fade_time", "15", FCVAR_CLIENTDLL );
ConVar cl_ragdoll_pronecheck_distance( "cl_ragdoll_pronecheck_distance", "64", FCVAR_GAMEDLL );

class C_SDKRagdoll : public C_BaseAnimatingOverlay
{
public:
	DECLARE_CLASS( C_SDKRagdoll, C_BaseAnimatingOverlay );
	DECLARE_CLIENTCLASS();

	C_SDKRagdoll();
	~C_SDKRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	int GetPlayerEntIndex() const;
	IRagdoll* GetIRagdoll() const;

	virtual void ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );

	void ClientThink( void );
	void StartFadeOut( float fDelay );
	
	bool IsRagdollVisible();
	int BloodColor()
	{
		return BLOOD_COLOR_RED; 
	}
private:

	C_SDKRagdoll( const C_SDKRagdoll & ) {}

	void Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity );

	void CreateRagdoll();


private:

	EHANDLE	m_hPlayer;
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	float m_fDeathTime;
	bool  m_bFadingOut;
};


IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_SDKRagdoll, DT_SDKRagdoll, CSDKRagdoll )
	RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_nModelIndex ) ),
	RecvPropInt( RECVINFO(m_nForceBone) ),
	RecvPropVector( RECVINFO(m_vecForce) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) )
END_RECV_TABLE()


C_SDKRagdoll::C_SDKRagdoll()
{
	m_fDeathTime = -1;
	m_bFadingOut = false;
}

C_SDKRagdoll::~C_SDKRagdoll()
{
	PhysCleanupFrictionSounds( this );
}

void C_SDKRagdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
{
	if ( !pSourceEntity )
		return;
	
	VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t *pDest = GetVarMapping();
    	
	// Find all the VarMapEntry_t's that represent the same variable.
	for ( int i = 0; i < pDest->m_Entries.Count(); i++ )
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		for ( int j=0; j < pSrc->m_Entries.Count(); j++ )
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if ( !Q_strcmp( pSrcEntry->watcher->GetDebugName(),
				pDestEntry->watcher->GetDebugName() ) )
			{
				pDestEntry->watcher->Copy( pSrcEntry->watcher );
				break;
			}
		}
	}
}
void FX_BloodSpray( const Vector &origin, const Vector &normal, float scale, unsigned char r, unsigned char g, unsigned char b, int flags );
void C_SDKRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName )
{
//	DevMsg("C_SDKRagDoll::ImpactTrace: %i\n", iDamageType);
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if ( iDamageType == DMG_BLAST )
	{
		dir *= 4000;  // adjust impact strength
				
		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;  

		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		// Blood spray!
		FX_BloodSpray( hitpos, dir, 3, 72, 0, 0, FX_BLOODSPRAY_ALL  );

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );	
		//Tony; throw in some bleeds! - just use a generic value for damage.
		TraceBleed( 40, dir, pTrace, iDamageType );

	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}


void C_SDKRagdoll::CreateRagdoll()
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_SDKPlayer *pPlayer = dynamic_cast< C_SDKPlayer* >( m_hPlayer.Get() );

	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		if ( !pPlayer->IsLocalPlayer() && pPlayer->IsInterpolationEnabled() )
		{
			Interp_Copy( pPlayer );

			SetAbsAngles( pPlayer->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence() );
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( m_vecRagdollOrigin );

			SetAbsAngles( pPlayer->GetRenderAngles() );

			SetAbsVelocity( m_vecRagdollVelocity );

			int iSeq = LookupSequence( "RagdollSpawn" );	// hax, find a neutral standing pose
			if ( iSeq == -1 )
			{
				Assert( false );	// missing look_idle?
				iSeq = 0;
			}
			
			SetSequence( iSeq );	// look_idle, basic pose
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}		

		m_nBody = pPlayer->GetBody();
	}
	else
	{
		// overwrite network origin so later interpolation will
		// use this position
		SetNetworkOrigin( m_vecRagdollOrigin );

		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );

	}

	SetModelIndex( m_nModelIndex );
	
	// Turn it into a ragdoll.
	if ( cl_ragdoll_physics_enable.GetInt() )
	{
		// Make us a ragdoll..
		m_nRenderFX = kRenderFxRagdoll;

		matrix3x4_t boneDelta0[MAXSTUDIOBONES];
		matrix3x4_t boneDelta1[MAXSTUDIOBONES];
		matrix3x4_t currentBones[MAXSTUDIOBONES];
		const float boneDt = 0.05f;

		if ( pPlayer && pPlayer == C_BasePlayer::GetLocalPlayer() )
		{
			pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
		}
		else
		{
			GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
		}

		InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );
	}
	else
	{
		ClientLeafSystem()->SetRenderGroup( GetRenderHandle(), RENDER_GROUP_TRANSLUCENT_ENTITY );
	}		

	// Fade out the ragdoll in a while
	StartFadeOut( cl_ragdoll_fade_time.GetFloat() );
	SetNextClientThink( gpGlobals->curtime + 5.0f );
}

void C_SDKRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		CreateRagdoll();
	}
	else 
	{
		if ( !cl_ragdoll_physics_enable.GetInt() )
		{
			// Don't let it set us back to a ragdoll with data from the server.
			m_nRenderFX = kRenderFxNone;
		}
	}
}
bool C_SDKRagdoll::IsRagdollVisible()
{
	Vector vMins = Vector(-1,-1,-1);	//WorldAlignMins();
	Vector vMaxs = Vector(1,1,1);	//WorldAlignMaxs();
		
	Vector origin = GetAbsOrigin();
	
	if( !engine->IsBoxInViewCluster( vMins + origin, vMaxs + origin) )
	{
		return false;
	}
	else if( engine->CullBox( vMins + origin, vMaxs + origin ) )
	{
		return false;
	}

	return true;
}

void C_SDKRagdoll::ClientThink( void )
{
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	if ( m_bFadingOut == true )
	{
		int iAlpha = GetRenderColor().a;
		int iFadeSpeed = 600.0f;

		iAlpha = max( iAlpha - ( iFadeSpeed * gpGlobals->frametime ), 0 );

		SetRenderMode( kRenderTransAlpha );
		SetRenderColorA( iAlpha );

		if ( iAlpha == 0 )
		{
			Release();
		}

		return;
	}

	for( int iClient = 1; iClient <= gpGlobals->maxClients; ++iClient )
	{
		C_SDKPlayer *pEnt = static_cast< C_SDKPlayer *> ( UTIL_PlayerByIndex( iClient ) );

		if(!pEnt || !pEnt->IsPlayer())
			continue;

		if ( m_hPlayer == NULL )
			continue;

		if ( pEnt->entindex() == m_hPlayer->entindex() )
			continue;
		
		if ( pEnt->GetHealth() <= 0 )
			continue;
#if defined ( SDK_USE_PRONE )
		if ( pEnt->m_Shared.IsProne() == false )
			continue;
#endif
		Vector vTargetOrigin = pEnt->GetAbsOrigin();
		Vector vMyOrigin =  GetAbsOrigin();

		Vector vDir = vTargetOrigin - vMyOrigin;

		if ( vDir.Length() > cl_ragdoll_pronecheck_distance.GetInt() ) 
			continue;

		SetNextClientThink( CLIENT_THINK_ALWAYS );
		m_bFadingOut = true;
		return;
	}

	//Tony; this is kind of silly, because.. whats the point of fading out?
	// if the player is looking at us, delay the fade
	if ( IsRagdollVisible() )
	{
		StartFadeOut( 5.0 );
		return;
	}

	if ( m_fDeathTime > gpGlobals->curtime )
		return;

	Release(); // Die
}

void C_SDKRagdoll::StartFadeOut( float fDelay )
{
	m_fDeathTime = gpGlobals->curtime + fDelay;
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}
IRagdoll* C_SDKRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

C_BaseAnimating * C_SDKPlayer::BecomeRagdollOnClient()
{
	return NULL;
}


IRagdoll* C_SDKPlayer::GetRepresentativeRagdoll() const
{
	if ( m_hRagdoll.Get() )
	{
		C_SDKRagdoll *pRagdoll = (C_SDKRagdoll*)m_hRagdoll.Get();

		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}



C_SDKPlayer::C_SDKPlayer() : 
	m_iv_angEyeAngles( "C_SDKPlayer::m_iv_angEyeAngles" )
{
	m_PlayerAnimState = CreateSDKPlayerAnimState( this );
	m_Shared.Init(this);

	m_angEyeAngles.Init();
	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

	m_angCamViewAngles.Init();

	m_fNextThinkPushAway = 0.0f;
	m_Shared.m_flPlayerAnimEventStartTime = gpGlobals->curtime;
	m_Shared.m_ePlayerAnimEvent = PLAYERANIMEVENT_NONE;
	m_Shared.m_aPlayerAnimEventStartAngle = vec3_origin;
	m_Shared.m_nPlayerAnimEventStartButtons = 0;

	m_nModelScale = 100;
	m_pHoldingBall = NULL;

	m_hColorCorrection = INVALID_CLIENT_CCHANDLE;
	m_szColorCorrectionFile[0] = '\0';
}


C_SDKPlayer::~C_SDKPlayer()
{
	m_PlayerAnimState->Release();
}


C_SDKPlayer* C_SDKPlayer::GetLocalSDKPlayer()
{
	return ToSDKPlayer( C_BasePlayer::GetLocalPlayer() );
}

const QAngle &C_SDKPlayer::EyeAngles()
{
	if( IsLocalPlayer() )
	{
		return BaseClass::EyeAngles();
	}
	else
	{
		return m_angEyeAngles;
	}
}
const QAngle& C_SDKPlayer::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
		return m_PlayerAnimState->GetRenderAngles();
	}
}

const Vector& C_SDKPlayer::GetRenderOrigin( void )
{
	if ( IsRagdoll() )
	{
		return m_pRagdoll->GetRagdollOrigin();
	}
	else
	{
		static Vector origin = vec3_origin;
		origin = BaseClass::GetRenderOrigin();
		//origin.z = origin.z - VEC_HULL_MAX.z + VEC_HULL_MAX.z * m_nModelScale / 100.0f;
		origin.z += VEC_HULL_MAX.z / 2.0f * (m_nModelScale / 100.0f - 1.0f);

		if (this == GetLocalSDKPlayer())
		{
			Vector vSmoothOffset;
			GetPredictionErrorSmoothingVector( vSmoothOffset );
			origin += Vector(vSmoothOffset.x, vSmoothOffset.y, 0);
			return origin;
		}
		else
			return origin;	
	}
}

void C_SDKPlayer::Spawn()
{
	BaseClass::Spawn();

	m_Shared.m_flNextJump = gpGlobals->curtime;
	m_Shared.m_flNextSlide = gpGlobals->curtime;
	m_Shared.m_bWasJumping = false;
}

void C_SDKPlayer::UpdateClientSideAnimation()
{
	m_PlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );
	LookAtBall();
	BaseClass::UpdateClientSideAnimation();
}

void C_SDKPlayer::LookAtBall(void)
{
	const QAngle camAngles = IsLocalPlayer() ? ::input->GetCameraAngles() : m_angCamViewAngles;
	float yaw = camAngles[YAW] - EyeAngles()[YAW];
	float pitch = camAngles[PITCH];

	if (yaw > 180) yaw -= 360;
	if (yaw < -180) yaw += 360;
	if (pitch > 180) pitch -= 360;
	if (pitch < -180) pitch += 360;

	//if (yaw > 90) yaw = 180 - yaw;
	//if (yaw < -90) yaw = -180 - yaw;

	pitch = clamp(pitch, -60, 60);
	yaw = clamp(yaw, -120, 120);

	float neckVal = clamp(yaw, -50, 50);
	SetBoneController(2, neckVal); // Neck
	yaw -= neckVal;

	float upperSpineVal = clamp(yaw, -40, 40);
	SetBoneController(1, upperSpineVal); // Upper spine
	yaw -= upperSpineVal;

	float lowerSpineVal = clamp(yaw, -30, 30);
	SetBoneController(0, lowerSpineVal); // Lower spine

	SetBoneController(3, pitch);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *C_SDKPlayer::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	InitializePoseParams();

	// Reset the players animation states, gestures
	if ( m_PlayerAnimState )
	{
		m_PlayerAnimState->OnNewModel();
	}

	return hdr;
}
//-----------------------------------------------------------------------------
// Purpose: Clear all pose parameters
//-----------------------------------------------------------------------------
void C_SDKPlayer::InitializePoseParams( void )
{
	CStudioHdr *hdr = GetModelPtr();
	Assert( hdr );
	if ( !hdr )
		return;

	m_headYawPoseParam = LookupPoseParameter( "head_yaw" );
	GetPoseParameterRange( m_headYawPoseParam, m_headYawMin, m_headYawMax );

	m_headPitchPoseParam = LookupPoseParameter( "head_pitch" );
	GetPoseParameterRange( m_headPitchPoseParam, m_headPitchMin, m_headPitchMax );

	for ( int i = 0; i < hdr->GetNumPoseParameters() ; i++ )
	{
		SetPoseParameter( hdr, i, 0.0 );
	}

}
//Tony; TODO!
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/*void C_SDKPlayer::OnPlayerClassChange( void )
{
	// Init the anim movement vars
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5 );
}
*/
void C_SDKPlayer::PostDataUpdate( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );
	
	BaseClass::PostDataUpdate( updateType );

	bool bIsLocalPlayer = IsLocalPlayer();

	if( m_bSpawnInterpCounter != m_bSpawnInterpCounterCache )
	{
		MoveToLastReceivedPosition( true );
		ResetLatched();

		if ( bIsLocalPlayer )
		{
			LocalPlayerRespawn();
		}
		m_bSpawnInterpCounterCache = m_bSpawnInterpCounter.m_Value;
	}

}
// Called every time the player respawns
void C_SDKPlayer::LocalPlayerRespawn( void )
{
	ResetToneMapping(1.0);
#if defined ( SDK_USE_PRONE )
	m_Shared.m_bForceProneChange = true;
	m_bUnProneToDuck = false;
#endif

		InitSpeeds(); //Tony; initialize player speeds.
}

void C_SDKPlayer::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	UpdateVisibility();
}

void C_SDKPlayer::PlayReloadEffect()
{
	// Only play the effect for other players.
	if ( this == C_SDKPlayer::GetLocalSDKPlayer() )
	{
		Assert( false ); // We shouldn't have been sent this message.
		return;
	}

	// Get the view model for our current gun.
	CWeaponSDKBase *pWeapon = NULL;
	if ( !pWeapon )
		return;

	// The weapon needs two models, world and view, but can only cache one. Synthesize the other.
	const CSDKWeaponInfo &info = pWeapon->GetSDKWpnData();
	const model_t *pModel = modelinfo->GetModel( modelinfo->GetModelIndex( info.szViewModel ) );
	if ( !pModel )
		return;
	CStudioHdr studioHdr( modelinfo->GetStudiomodel( pModel ), mdlcache );
	if ( !studioHdr.IsValid() )
		return;

	// Find the reload animation.
	for ( int iSeq=0; iSeq < studioHdr.GetNumSeq(); iSeq++ )
	{
		mstudioseqdesc_t *pSeq = &studioHdr.pSeqdesc( iSeq );

		if ( pSeq->activity == ACT_VM_RELOAD )
		{
			float poseParameters[MAXSTUDIOPOSEPARAM];
			memset( poseParameters, 0, sizeof( poseParameters ) );
			float cyclesPerSecond = Studio_CPS( &studioHdr, *pSeq, iSeq, poseParameters );

			// Now read out all the sound events with their timing
			for ( int iEvent=0; iEvent < pSeq->numevents; iEvent++ )
			{
				mstudioevent_t *pEvent = pSeq->pEvent( iEvent );

				if ( pEvent->event == CL_EVENT_SOUND )
				{
					CSDKSoundEvent event;
					event.m_SoundName = pEvent->options;
					event.m_flEventTime = gpGlobals->curtime + pEvent->cycle / cyclesPerSecond;
					m_SoundEvents.AddToTail( event );
				}
			}

			break;
		}
	}	
}

void C_SDKPlayer::DoAnimationEvent(PlayerAnimEvent_t event)
{
	if ( IsLocalPlayer() )
	{
		if ( ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() ) )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
	m_PlayerAnimState->DoAnimationEvent(event);
	//m_Shared.DoAnimationEvent(event);
}

bool C_SDKPlayer::ShouldDraw( void )
{
	// If we're dead, our ragdoll will be drawn for us instead.
	if ( !IsAlive() )
		return false;

	if( GetTeamNumber() == TEAM_SPECTATOR )
		return false;

#if defined ( SDK_USE_TEAMS )
	if ( State_Get() == STATE_PICKINGTEAM )
		return false;
#endif

#if defined ( SDK_USE_PLAYERCLASSES )
	if ( State_Get() == STATE_PICKINGCLASS )
		return false;
#endif

	if( IsLocalPlayer() && IsRagdoll() )
		return true;

	return BaseClass::ShouldDraw();
}

#if defined ( SDK_USE_PLAYERCLASSES )
bool C_SDKPlayer::CanShowClassMenu( void )
{
	#if defined ( SDK_USE_TEAMS )
		return ( GetTeamNumber() == SDK_TEAM_BLUE || GetTeamNumber() == SDK_TEAM_RED );
	#else
		return ( GetTeamNumber() != TEAM_SPECTATOR );
	#endif
}
#endif

#if defined ( SDK_USE_TEAMS )
bool C_SDKPlayer::CanShowTeamMenu( void )
{
	return true;
}
#endif

ConVar cl_colorcorrection_file("cl_colorcorrection_file", "night");
ConVar cl_colorcorrection_weight("cl_colorcorrection_weight", "0.0");

void C_SDKPlayer::ClientThink()
{
	UpdateSoundEvents();

	// Pass on through to the base class.
	BaseClass::ClientThink();

	bool bFoundViewTarget = false;
	
	Vector vForward;
	AngleVectors( GetLocalAngles(), &vForward );

	for( int iClient = 1; iClient <= gpGlobals->maxClients; ++iClient )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( iClient );
		if(!pEnt || !pEnt->IsPlayer())
			continue;

		if ( pEnt->entindex() == entindex() )
			continue;

		Vector vTargetOrigin = pEnt->GetAbsOrigin();
		Vector vMyOrigin =  GetAbsOrigin();

		Vector vDir = vTargetOrigin - vMyOrigin;
		
		if ( vDir.Length() > 128 ) 
			continue;

		VectorNormalize( vDir );

		if ( DotProduct( vForward, vDir ) < 0.0f )
			 continue;

		m_vLookAtTarget = pEnt->EyePosition();
		bFoundViewTarget = true;
		break;
	}

	if ( bFoundViewTarget == false )
	{
		m_vLookAtTarget = GetAbsOrigin() + vForward * 512;
	}

	UpdateIDTarget();

	// Avoidance
	if ( gpGlobals->curtime >= m_fNextThinkPushAway )
	{
		PerformObstaclePushaway( this );
		m_fNextThinkPushAway =  gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL;
	}

	if (Q_strcmp(m_szColorCorrectionFile, cl_colorcorrection_file.GetString()))
	{
		if (m_hColorCorrection != INVALID_CLIENT_CCHANDLE)
			g_pColorCorrectionMgr->RemoveColorCorrection(m_hColorCorrection);

		m_hColorCorrection = g_pColorCorrectionMgr->AddColorCorrection(VarArgs("materials/correction/%s.raw", cl_colorcorrection_file.GetString()));
		Q_strncpy(m_szColorCorrectionFile, cl_colorcorrection_file.GetString(), sizeof(m_szColorCorrectionFile));
	}

	g_pColorCorrectionMgr->SetColorCorrectionWeight(m_hColorCorrection, cl_colorcorrection_weight.GetFloat());
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_SDKPlayer::UpdateLookAt( void )
{
	// head yaw
	if (m_headYawPoseParam < 0 || m_headPitchPoseParam < 0)
		return;

	// orient eyes
	m_viewtarget = m_vLookAtTarget;

	// Figure out where we want to look in world space.
	QAngle desiredAngles;
	Vector to = m_vLookAtTarget - EyePosition();
	VectorAngles( to, desiredAngles );

	// Figure out where our body is facing in world space.
	QAngle bodyAngles( 0, 0, 0 );
	bodyAngles[YAW] = GetLocalAngles()[YAW];


	float flBodyYawDiff = bodyAngles[YAW] - m_flLastBodyYaw;
	m_flLastBodyYaw = bodyAngles[YAW];
	

	// Set the head's yaw.
	float desired = AngleNormalize( desiredAngles[YAW] - bodyAngles[YAW] );
	desired = clamp( desired, m_headYawMin, m_headYawMax );
	m_flCurrentHeadYaw = ApproachAngle( desired, m_flCurrentHeadYaw, 130 * gpGlobals->frametime );

	// Counterrotate the head from the body rotation so it doesn't rotate past its target.
	m_flCurrentHeadYaw = AngleNormalize( m_flCurrentHeadYaw - flBodyYawDiff );
	desired = clamp( desired, m_headYawMin, m_headYawMax );
	
	SetPoseParameter( m_headYawPoseParam, m_flCurrentHeadYaw );

	
	// Set the head's yaw.
	desired = AngleNormalize( desiredAngles[PITCH] );
	desired = clamp( desired, m_headPitchMin, m_headPitchMax );
	
	m_flCurrentHeadPitch = ApproachAngle( desired, m_flCurrentHeadPitch, 130 * gpGlobals->frametime );
	m_flCurrentHeadPitch = AngleNormalize( m_flCurrentHeadPitch );
	SetPoseParameter( m_headPitchPoseParam, m_flCurrentHeadPitch );
}



int C_SDKPlayer::GetIDTarget() const
{
	return m_iIDEntIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Update this client's target entity
//-----------------------------------------------------------------------------
void C_SDKPlayer::UpdateIDTarget()
{
	if ( !IsLocalPlayer() )
		return;

	// Clear old target and find a new one
	m_iIDEntIndex = 0;

	// don't show id's in any state but active.
	if ( State_Get() != PLAYER_STATE_ACTIVE )
		return;

	trace_t tr;
	Vector vecStart, vecEnd;
	VectorMA( MainViewOrigin(), 1500, MainViewForward(), vecEnd );
	VectorMA( MainViewOrigin(), 10,   MainViewForward(), vecStart );
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		C_BaseEntity *pEntity = tr.m_pEnt;

		if ( pEntity && (pEntity != this) )
		{
			m_iIDEntIndex = pEntity->entindex();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Try to steer away from any players and objects we might interpenetrate
//-----------------------------------------------------------------------------
#define SDK_AVOID_MAX_RADIUS_SQR		5184.0f			// Based on player extents and max buildable extents.
#define SDK_OO_AVOID_MAX_RADIUS_SQR		0.00029f

ConVar sdk_max_separation_force ( "sdk_max_separation_force", "256", FCVAR_CHEAT|FCVAR_HIDDEN );

extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;

void C_SDKPlayer::AvoidPlayers( CUserCmd *pCmd )
{
// Player Avoidance is only active with teams
	// Don't test if the player doesn't exist or is dead.
	if ( IsAlive() == false )
		return;

	// Up vector.
	static Vector vecUp( 0.0f, 0.0f, 1.0f );

	Vector vecSDKPlayerCenter = GetAbsOrigin();
	Vector vecSDKPlayerMin = GetPlayerMins();
	Vector vecSDKPlayerMax = GetPlayerMaxs();
	float flZHeight = vecSDKPlayerMax.z - vecSDKPlayerMin.z;
	vecSDKPlayerCenter.z += 0.5f * flZHeight;
	VectorAdd( vecSDKPlayerMin, vecSDKPlayerCenter, vecSDKPlayerMin );
	VectorAdd( vecSDKPlayerMax, vecSDKPlayerCenter, vecSDKPlayerMax );

	// Find an intersecting player or object.
	int nAvoidPlayerCount = 0;
	C_SDKPlayer *pAvoidPlayerList[MAX_PLAYERS];

	C_SDKPlayer *pIntersectPlayer = NULL;
	float flAvoidRadius = 0.0f;

	Vector vecAvoidCenter, vecAvoidMin, vecAvoidMax;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		//C_SDKPlayer *pAvoidPlayer = static_cast< C_SDKPlayer * >( pTeam->GetPlayer( i ) );
		C_SDKPlayer *pAvoidPlayer = static_cast< C_SDKPlayer * >( UTIL_PlayerByIndex(i) );
		if ( pAvoidPlayer == NULL )
			continue;
		// Is the avoid player me?
		if ( pAvoidPlayer == this )
			continue;

		// Save as list to check against for objects.
		pAvoidPlayerList[nAvoidPlayerCount] = pAvoidPlayer;
		++nAvoidPlayerCount;

		// Check to see if the avoid player is dormant.
		if ( pAvoidPlayer->IsDormant() )
			continue;

		// Is the avoid player solid?
		if ( pAvoidPlayer->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
			continue;

		Vector t1, t2;

		vecAvoidCenter = pAvoidPlayer->GetAbsOrigin();
		vecAvoidMin = pAvoidPlayer->GetPlayerMins();
		vecAvoidMax = pAvoidPlayer->GetPlayerMaxs();
		flZHeight = vecAvoidMax.z - vecAvoidMin.z;
		vecAvoidCenter.z += 0.5f * flZHeight;
		VectorAdd( vecAvoidMin, vecAvoidCenter, vecAvoidMin );
		VectorAdd( vecAvoidMax, vecAvoidCenter, vecAvoidMax );

		if ( IsBoxIntersectingBox( vecSDKPlayerMin, vecSDKPlayerMax, vecAvoidMin, vecAvoidMax ) )
		{
			// Need to avoid this player.
			if ( !pIntersectPlayer )
			{
				pIntersectPlayer = pAvoidPlayer;
				break;
			}
		}
	}

	// Anything to avoid?
	if ( !pIntersectPlayer )
		return;

	// Calculate the push strength and direction.
	Vector vecDelta;

	// Avoid a player - they have precedence.
	if ( pIntersectPlayer )
	{
		VectorSubtract( pIntersectPlayer->WorldSpaceCenter(), vecSDKPlayerCenter, vecDelta );

		Vector vRad = pIntersectPlayer->WorldAlignMaxs() - pIntersectPlayer->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}

	float flPushStrength = RemapValClamped( vecDelta.Length(), flAvoidRadius, 0, 0, sdk_max_separation_force.GetInt() ); //flPushScale;

	//Msg( "PushScale = %f\n", flPushStrength );

	// Check to see if we have enough push strength to make a difference.
	if ( flPushStrength < 0.01f )
		return;

	Vector vecPush;
	if ( GetAbsVelocity().Length2DSqr() > 0.1f )
	{
		Vector vecVelocity = GetAbsVelocity();
		vecVelocity.z = 0.0f;
		CrossProduct( vecUp, vecVelocity, vecPush );
		VectorNormalize( vecPush );
	}
	else
	{
		// We are not moving, but we're still intersecting.
		QAngle angView = pCmd->viewangles;
		angView.x = 0.0f;
		AngleVectors( angView, NULL, &vecPush, NULL );
	}

	// Move away from the other player/object.
	Vector vecSeparationVelocity;
	if ( vecDelta.Dot( vecPush ) < 0 )
	{
		vecSeparationVelocity = vecPush * flPushStrength;
	}
	else
	{
		vecSeparationVelocity = vecPush * -flPushStrength;
	}

	// Don't allow the max push speed to be greater than the max player speed.
	float flMaxPlayerSpeed = MaxSpeed();
	float flCropFraction = 1.33333333f;

	//if ( ( GetFlags() & FL_DUCKING ) && ( GetGroundEntity() != NULL ) )
	//{	
	//	flMaxPlayerSpeed *= flCropFraction;
	//}	

	float flMaxPlayerSpeedSqr = flMaxPlayerSpeed * flMaxPlayerSpeed;

	if ( vecSeparationVelocity.LengthSqr() > flMaxPlayerSpeedSqr )
	{
		vecSeparationVelocity.NormalizeInPlace();
		VectorScale( vecSeparationVelocity, flMaxPlayerSpeed, vecSeparationVelocity );
	}

	QAngle vAngles = pCmd->viewangles;
	vAngles.x = 0;
	Vector currentdir;
	Vector rightdir;

	AngleVectors( vAngles, &currentdir, &rightdir, NULL );

	Vector vDirection = vecSeparationVelocity;

	VectorNormalize( vDirection );

	float fwd = currentdir.Dot( vDirection );
	float rt = rightdir.Dot( vDirection );

	float forward = fwd * flPushStrength;
	float side = rt * flPushStrength;

	//Msg( "fwd: %f - rt: %f - forward: %f - side: %f\n", fwd, rt, forward, side );

	pCmd->forwardmove	+= forward;
	pCmd->sidemove		+= side;

	// Clamp the move to within legal limits, preserving direction. This is a little
	// complicated because we have different limits for forward, back, and side

	//Msg( "PRECLAMP: forwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );

	float flForwardScale = 1.0f;
	if ( pCmd->forwardmove > fabs( cl_forwardspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_forwardspeed.GetFloat() ) / pCmd->forwardmove;
	}
	else if ( pCmd->forwardmove < -fabs( cl_backspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_backspeed.GetFloat() ) / fabs( pCmd->forwardmove );
	}

	float flSideScale = 1.0f;
	if ( fabs( pCmd->sidemove ) > fabs( cl_sidespeed.GetFloat() ) )
	{
		flSideScale = fabs( cl_sidespeed.GetFloat() ) / fabs( pCmd->sidemove );
	}

	float flScale = min( flForwardScale, flSideScale );
	pCmd->forwardmove *= flScale;
	pCmd->sidemove *= flScale;

	//Msg( "Pforwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );
}

bool C_SDKPlayer::CreateMove( float flInputSampleTime, CUserCmd *pCmd )
{	
	static QAngle angMoveAngle( 0.0f, 0.0f, 0.0f );

	VectorCopy( pCmd->viewangles, angMoveAngle );

	BaseClass::CreateMove( flInputSampleTime, pCmd );

	//AvoidPlayers( pCmd );
	//AvoidPhysicsProps(pCmd);

	return true;
}

void C_SDKPlayer::UpdateSoundEvents()
{
	int iNext;
	for ( int i=m_SoundEvents.Head(); i != m_SoundEvents.InvalidIndex(); i = iNext )
	{
		iNext = m_SoundEvents.Next( i );

		CSDKSoundEvent *pEvent = &m_SoundEvents[i];
		if ( gpGlobals->curtime >= pEvent->m_flEventTime )
		{
			CLocalPlayerFilter filter;
			EmitSound( filter, GetSoundSourceIndex(), STRING( pEvent->m_SoundName ) );

			m_SoundEvents.Remove( i );
		}
	}
}

//IOS Added from HLCoders:
ShadowType_t C_SDKPlayer::ShadowCastType( void )
 {
	 if ( !IsVisible() || IsEffectActive(EF_NODRAW | EF_NOSHADOW) )
		return SHADOWS_NONE;
	else
		return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

//void C_SDKPlayer::PostThink(void)
//{
//	if( GetEFlags() & EFL_DIRTY_SHADOWUPDATE )
//		g_pClientShadowMgr->UpdateProjectedTexture( GetShadowHandle(),true );
//}
//end IOS

void C_SDKPlayer::ApplyBoneMatrixTransform(matrix3x4_t& transform)
{
	//BaseClass::ApplyBoneMatrixTransform(transform);

	float scale = m_nModelScale / 100.0f;
	VectorScale( transform[0], scale, transform[0] );
	VectorScale( transform[1], scale, transform[1] );
	VectorScale( transform[2], scale, transform[2] );
}

int C_SDKPlayer::DrawModel( int flags )
{
	int retVal = BaseClass::DrawModel( flags );

	return retVal;
}

void C_SDKPlayer::PreThink()
{
	BaseClass::PreThink();

	if (!ShotButtonsPressed())
		SetShotButtonsReleased(true);

	CheckShotCharging();
	CheckLastPressedSingleMoveButton();
}