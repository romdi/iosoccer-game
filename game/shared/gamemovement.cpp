//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "gamemovement.h"
#include "in_buttons.h"
#include <stdarg.h>
#include "movevars_shared.h"
#include "engine/IEngineTrace.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "decals.h"
#include "coordsize.h"
#include "rumble_shared.h"
#include "sdk_shareddefs.h"
#include "sdk_gamerules.h"

#if defined(HL2_DLL) || defined(HL2_CLIENT_DLL)
	#include "hl_movedata.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	STOP_EPSILON		0.1
#define	MAX_CLIP_PLANES		5

#include "filesystem.h"
#include <stdarg.h>

extern IFileSystem *filesystem;

#ifndef CLIENT_DLL
	#include "env_player_surface_trigger.h"
	static ConVar dispcoll_drawplane( "dispcoll_drawplane", "0" );
#endif

#ifdef CLIENT_DLL
	#include "c_sdk_player.h"
	#include "c_team.h"
	#include "c_playerresource.h"
	#include "c_player_ball.h"
	#include "c_match_ball.h"
#else
	#include "sdk_player.h"
	#include "team.h"
	#include "player_ball.h"
	#include "match_ball.h"
#endif

// tickcount currently isn't set during prediction, although gpGlobals->curtime and
// gpGlobals->frametime are. We should probably set tickcount (to player->m_nTickBase),
// but we're REALLY close to shipping, so we can change that later and people can use
// player->CurrentCommandNumber() in the meantime.
#define tickcount USE_PLAYER_CURRENT_COMMAND_NUMBER__INSTEAD_OF_TICKCOUNT

#if defined( HL2_DLL )
ConVar xc_uncrouch_on_jump( "xc_uncrouch_on_jump", "1", FCVAR_ARCHIVE, "Uncrouch when jump occurs" );
#endif

#if defined( HL2_DLL ) || defined( HL2_CLIENT_DLL )
ConVar player_limit_jump_speed( "player_limit_jump_speed", "1", FCVAR_REPLICATED );
#endif

// option_duck_method is a carrier convar. Its sole purpose is to serve an easy-to-flip
// convar which is ONLY set by the X360 controller menu to tell us which way to bind the
// duck controls. Its value is meaningless anytime we don't have the options window open.
ConVar option_duck_method("option_duck_method", "1", FCVAR_REPLICATED|FCVAR_ARCHIVE );// 0 = HOLD to duck, 1 = Duck is a toggle

// Roughly how often we want to update the info about the ground surface we're on.
// We don't need to do this very often.
#define CATEGORIZE_GROUND_SURFACE_INTERVAL			0.3f
#define CATEGORIZE_GROUND_SURFACE_TICK_INTERVAL   ( (int)( CATEGORIZE_GROUND_SURFACE_INTERVAL / TICK_INTERVAL ) )

#define CHECK_STUCK_INTERVAL			1.0f
#define CHECK_STUCK_TICK_INTERVAL		( (int)( CHECK_STUCK_INTERVAL / TICK_INTERVAL ) )

#define CHECK_STUCK_INTERVAL_SP			0.2f
#define CHECK_STUCK_TICK_INTERVAL_SP	( (int)( CHECK_STUCK_INTERVAL_SP / TICK_INTERVAL ) )

#define CHECK_LADDER_INTERVAL			0.2f
#define CHECK_LADDER_TICK_INTERVAL		( (int)( CHECK_LADDER_INTERVAL / TICK_INTERVAL ) )

#define	NUM_CROUCH_HINTS	3

extern IGameMovement *g_pGameMovement;

// See shareddefs.h
#if PREDICTION_ERROR_CHECK_LEVEL > 0

static ConVar diffcheck( "diffcheck", "0", FCVAR_REPLICATED );

class IDiffMgr
{
public:
	virtual void StartCommand( bool bServer, int nCommandNumber ) = 0;
	virtual void AddToDiff( bool bServer, int nCommandNumber, char const *string ) = 0;
	virtual void Validate( bool bServer, int nCommandNumber ) = 0;
};

static IDiffMgr *g_pDiffMgr = NULL;

class CDiffStr
{
public:
	CDiffStr()
	{
		m_str[ 0 ] = 0;
	}

	CDiffStr( char const *str )
	{
		Q_strncpy( m_str, str, sizeof( m_str ) );
	}

	CDiffStr( const CDiffStr &src )
	{
		Q_strncpy( m_str, src.m_str, sizeof( m_str ) );
	}

	char const *String()
	{
		return m_str;
	}
private:

	char m_str[ 128 ];
};

// Per tick data
class CDiffInfo
{
public:
	CDiffInfo() : m_nCommandNumber( 0 ) {}
	CDiffInfo( const CDiffInfo& src )
	{
		m_nCommandNumber = src.m_nCommandNumber;
		for ( int i = 0; i < src.m_Lines.Count(); ++i )
		{
			m_Lines.AddToTail( src.m_Lines[ i ] );
		}
	}

	static bool Less( const CDiffInfo& lhs, const CDiffInfo& rhs )
	{
		return lhs.m_nCommandNumber < rhs.m_nCommandNumber;
	}
	int							m_nCommandNumber;
	CUtlVector< CDiffStr >	m_Lines;
	bool						m_bChecked;
};

class CDiffManager : public IDiffMgr
{
public:
	CDiffManager() : 
		m_Client( 0, 0, CDiffInfo::Less ),
		m_Server( 0, 0, CDiffInfo::Less ),
		m_flLastSpew( -1.0f )
	{
		g_pDiffMgr = this;
	}

	virtual void StartCommand( bool bServer, int nCommandNumber )
	{
#if defined( CLIENT_DLL )

		if ( !diffcheck.GetInt() )
			return;

		g_pDiffMgr = reinterpret_cast< IDiffMgr * >( diffcheck.GetInt() );
		g_pDiffMgr->StartCommand( bServer, nCommandNumber );
		return;
#endif

		// Msg( "%s Startcommand %d\n", bServer ? "sv" : "cl", nCommandNumber );

		diffcheck.SetValue( reinterpret_cast< int >( this ) );

		Assert( CBaseEntity::IsServer() );

		CUtlRBTree< CDiffInfo, int >& rb = bServer ? m_Server : m_Client;

		CDiffInfo search;
		search.m_nCommandNumber = nCommandNumber;
		int idx = rb.Find( search );
		if ( idx == rb.InvalidIndex() )
		{
			idx = rb.Insert( search );
		}

		CDiffInfo *slot = &rb[ idx ];
		slot->m_Lines.RemoveAll();
	}

	virtual void AddToDiff( bool bServer, int nCommandNumber, char const *string )
	{
#if defined( CLIENT_DLL )

		if ( !diffcheck.GetInt() )
			return;

		g_pDiffMgr = reinterpret_cast< IDiffMgr * >( diffcheck.GetInt() );
		g_pDiffMgr->AddToDiff( bServer, nCommandNumber, string );
		return;
#endif
		Assert( CBaseEntity::IsServer() );

		// Msg( "%s Add %d %s\n", bServer ? "sv" : "cl", nCommandNumber, string );

		CUtlRBTree< CDiffInfo, int >& rb = bServer ? m_Server : m_Client;

		CDiffInfo search;
		search.m_nCommandNumber = nCommandNumber;
		int idx = rb.Find( search );
		if ( idx == rb.InvalidIndex() )
		{
			Assert( 0 );
			idx = rb.Insert( search );
		}

		CDiffInfo *slot = &rb[ idx ];
		CDiffStr line( string );
		slot->m_Lines.AddToTail( line );
	}

	enum EMismatched
	{
		DIFFCHECK_NOTREADY = 0,
		DIFFCHECK_MATCHED,
		DIFFCHECK_DIFFERS
	};

	bool ClientRecordExists( int cmd )
	{
		CDiffInfo clsearch;
		clsearch.m_nCommandNumber = cmd;
		int clidx = m_Client.Find( clsearch );
		return m_Client.IsValidIndex( clidx );
	}

	EMismatched IsMismatched( int svidx )
	{
		CDiffInfo *serverslot = &m_Server[ svidx ];

		// Now find the client version of this one
		CDiffInfo clsearch;
		clsearch.m_nCommandNumber = serverslot->m_nCommandNumber;
		int clidx = m_Client.Find( clsearch );
		if ( clidx == m_Client.InvalidIndex() )
			return DIFFCHECK_NOTREADY;

		// Now compare them
		CDiffInfo *clientslot = &m_Client[ clidx ];

		bool bSpew = false;
		if ( serverslot->m_Lines.Count() != 
			clientslot->m_Lines.Count() )
		{
			return DIFFCHECK_DIFFERS;
		}

		int maxSlot = max( serverslot->m_Lines.Count(), clientslot->m_Lines.Count() );
		if ( !bSpew )
		{
			for ( int i = 0; i < maxSlot; ++i )
			{
				CDiffStr *sv = NULL;
				CDiffStr *cl = NULL;
				if ( i < serverslot->m_Lines.Count() )
				{
					sv = &serverslot->m_Lines[ i ];
				}
				if ( i < clientslot->m_Lines.Count() )
				{
					cl = &clientslot->m_Lines[ i ];
				}

				if ( Q_stricmp( sv ? sv->String() : "(missing)", cl ? cl->String() : "(missing)" ) )
				{
					return DIFFCHECK_DIFFERS;
				}
			}
		}

		return DIFFCHECK_MATCHED;
	}

	virtual void Validate( bool bServer, int nCommandNumber )
	{
#if defined( CLIENT_DLL )

		if ( !diffcheck.GetInt() )
			return;

		g_pDiffMgr = reinterpret_cast< IDiffMgr * >( diffcheck.GetInt() );
		g_pDiffMgr->Validate( bServer, nCommandNumber );
		return;
#endif
		Assert( CBaseEntity::IsServer() );

		// Only do this on the client
		if ( !bServer )
			return;

		// Find the last server command number
		if ( m_Server.Count() <= 0 )
			return;

		int svidx = m_Server.LastInorder();
		EMismatched eMisMatched = IsMismatched( svidx );
		if ( eMisMatched == DIFFCHECK_NOTREADY )
		{
			return;
		}

		if ( eMisMatched == DIFFCHECK_DIFFERS )
		{
			CUtlVector< int > vecPrev;

			int nCur = svidx;
			do 
			{
				int prev = m_Server.PrevInorder( nCur );
				if ( m_Server.IsValidIndex( prev ) && 
					ClientRecordExists( m_Server[ prev ].m_nCommandNumber ) )
				{
					//SpewRecords( "prev", prev );
					vecPrev.AddToHead( prev );
				}
				else
				{
					break;
				}

				nCur = prev;
			} while ( vecPrev.Count() < 10 );

			Msg( "-----\n" );

			for ( int p = 0; p < vecPrev.Count(); ++p )
			{
				SpewRecords( "prev", vecPrev[ p ] );
			}

			SpewRecords( "bad ", svidx );
		}
	}

	void SpewRecords( char const *prefix, int svidx )
	{
		CDiffInfo *serverslot = &m_Server[ svidx ];

		// Now find the client version of this one
		CDiffInfo clsearch;
		clsearch.m_nCommandNumber = serverslot->m_nCommandNumber;
		int clidx = m_Client.Find( clsearch );
		if ( clidx == m_Client.InvalidIndex() )
			return;

		// Now compare them
		CDiffInfo *clientslot = &m_Client[ clidx ];

		int maxSlot = max( serverslot->m_Lines.Count(), clientslot->m_Lines.Count() );

		for ( int i = 0; i < maxSlot; ++i )
		{
			char const *sv = "(missing)";
			char const *cl = "(missing)";

			if ( i < serverslot->m_Lines.Count() )
			{
				sv = serverslot->m_Lines[ i ].String();
			}
			if ( i < clientslot->m_Lines.Count() )
			{
				cl = clientslot->m_Lines[ i ].String();
			}

			bool bDiffers = Q_stricmp( sv, cl ) ? true : false;

			Msg( "%s%s%d:  sv[%50.50s] cl[%50.50s]\n",
				prefix,
				bDiffers ? "+++" : "   ",
				serverslot->m_nCommandNumber, 
				sv,
				cl );
		}
	}
private:

	CUtlRBTree< CDiffInfo, int >	m_Server;
	CUtlRBTree< CDiffInfo, int >	m_Client;
	float							m_flLastSpew;
};

static CDiffManager g_DiffMgr;

void DiffPrint( bool bServer, int nCommandNumber, char const *fmt, ... )
{
	// Only track stuff for local player
	CBasePlayer *pPlayer = CBaseEntity::GetPredictionPlayer();
	if ( pPlayer && pPlayer->entindex() != 1 )
	{
		return;
	}

	va_list		argptr;
	char		string[1024];
	va_start (argptr,fmt);
	int len = Q_vsnprintf(string, sizeof( string ), fmt,argptr);
	va_end (argptr);

	if ( g_pDiffMgr )
	{
		// Strip any \n at the end that the user accidently put int
		if ( len > 0 && string[ len -1 ] == '\n' )
		{
			string[ len - 1 ] = 0;
		}
		
		g_pDiffMgr->AddToDiff( bServer, nCommandNumber, string );
	}
}

void _CheckV( int tick, char const *ctx, const Vector &vel )
{
	DiffPrint( CBaseEntity::IsServer(), tick, "%20.20s %f %f %f", ctx, vel.x, vel.y, vel.z );
}

#define CheckV( tick, ctx, vel ) _CheckV( tick, ctx, vel );

static void StartCommand( bool bServer, int nCommandNumber )
{
	// Only track stuff for local player
	CBasePlayer *pPlayer = CBaseEntity::GetPredictionPlayer();
	if ( pPlayer && pPlayer->entindex() != 1 )
	{
		return;
	}

	if ( g_pDiffMgr )
	{
		g_pDiffMgr->StartCommand( bServer, nCommandNumber );
	}
}

static void Validate( bool bServer, int nCommandNumber )
{
	// Only track stuff for local player
	CBasePlayer *pPlayer = CBaseEntity::GetPredictionPlayer();
	if ( pPlayer && pPlayer->entindex() != 1 )
	{
		return;
	}


	if ( g_pDiffMgr )
	{
		g_pDiffMgr->Validate( bServer, nCommandNumber );
	}
}

void CGameMovement::DiffPrint( char const *fmt, ... )
{
	if ( !player )
		return;

	va_list		argptr;
	char		string[1024];
	va_start (argptr,fmt);
	Q_vsnprintf(string, sizeof( string ), fmt,argptr);
	va_end (argptr);

	::DiffPrint( CBaseEntity::IsServer(), player->CurrentCommandNumber(), "%s", string );
}

#else
static void DiffPrint( bool bServer, int nCommandNumber, char const *fmt, ... )
{
	// Nothing
}
static void StartCommand( bool bServer, int nCommandNumber )
{
}

static void Validate( bool bServer, int nCommandNumber )
{
}

#define CheckV( tick, ctx, vel )

void CGameMovement::DiffPrint( char const *fmt, ... )
{
}

#endif // !PREDICTION_ERROR_CHECK_LEVEL

#ifndef _XBOX
void COM_Log( char *pszFile, char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	FileHandle_t fp;
	char *pfilename;
	
	if ( !pszFile )
	{
		pfilename = "hllog.txt";
	}
	else
	{
		pfilename = pszFile;
	}
	va_start (argptr,fmt);
	Q_vsnprintf(string, sizeof( string ), fmt,argptr);
	va_end (argptr);

	fp = filesystem->Open( pfilename, "a+t");
	if (fp)
	{
		filesystem->FPrintf(fp, "%s", string);
		filesystem->Close(fp);
	}
}
#endif

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Debug - draw the displacement collision plane.
//-----------------------------------------------------------------------------
void DrawDispCollPlane( CBaseTrace *pTrace )
{
	float flLength = 30.0f;

	// Create a basis, based on the impact normal.
	int nMajorAxis = 0;
	Vector vecBasisU, vecBasisV, vecNormal;
	vecNormal = pTrace->plane.normal;
	float flAxisValue = vecNormal[0];
	if ( fabs( vecNormal[1] ) > fabs( flAxisValue ) ) { nMajorAxis = 1; flAxisValue = vecNormal[1]; }
	if ( fabs( vecNormal[2] ) > fabs( flAxisValue ) ) { nMajorAxis = 2; }
	if ( ( nMajorAxis == 1 ) || ( nMajorAxis == 2 ) )
	{
		vecBasisU.Init( 1.0f, 0.0f, 0.0f );
	}
	else
	{
		vecBasisU.Init( 0.0f, 1.0f, 0.0f );
	}

	vecBasisV = vecNormal.Cross( vecBasisU );
	VectorNormalize( vecBasisV );

	vecBasisU = vecBasisV.Cross( vecNormal );
	VectorNormalize( vecBasisU );

	// Create the impact point.  Push off the surface a bit.
	Vector vecImpactPoint = pTrace->startpos + pTrace->fraction * ( pTrace->endpos - pTrace->startpos );
	vecImpactPoint += vecNormal;

	// Generate a quad to represent the plane.
	Vector vecPlanePoints[4];
	vecPlanePoints[0] = vecImpactPoint + ( vecBasisU * -flLength ) + ( vecBasisV * -flLength );
	vecPlanePoints[1] = vecImpactPoint + ( vecBasisU * -flLength ) + ( vecBasisV * flLength );
	vecPlanePoints[2] = vecImpactPoint + ( vecBasisU * flLength ) + ( vecBasisV * flLength );
	vecPlanePoints[3] = vecImpactPoint + ( vecBasisU * flLength ) + ( vecBasisV * -flLength );

#if 0
	// Test facing.
	Vector vecEdges[2];
	vecEdges[0] = vecPlanePoints[1] - vecPlanePoints[0];
	vecEdges[1] = vecPlanePoints[2] - vecPlanePoints[0];
	Vector vecCross = vecEdges[0].Cross( vecEdges[1] );
	if ( vecCross.Dot( vecNormal ) < 0.0f )
	{
		// Reverse winding.
	}
#endif

	// Draw the plane.
	NDebugOverlay::Triangle( vecPlanePoints[0], vecPlanePoints[1], vecPlanePoints[2], 125, 125, 125, 125, false, 5.0f );
	NDebugOverlay::Triangle( vecPlanePoints[0], vecPlanePoints[2], vecPlanePoints[3], 125, 125, 125, 125, false, 5.0f );

	NDebugOverlay::Line( vecPlanePoints[0], vecPlanePoints[1], 255, 255, 255, false, 5.0f );
	NDebugOverlay::Line( vecPlanePoints[1], vecPlanePoints[2], 255, 255, 255, false, 5.0f );
	NDebugOverlay::Line( vecPlanePoints[2], vecPlanePoints[3], 255, 255, 255, false, 5.0f );
	NDebugOverlay::Line( vecPlanePoints[3], vecPlanePoints[0], 255, 255, 255, false, 5.0f );

	// Draw the normal.
	NDebugOverlay::Line( vecImpactPoint, vecImpactPoint + ( vecNormal * flLength ), 255, 0, 0, false, 5.0f );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructs GameMovement interface
//-----------------------------------------------------------------------------
CGameMovement::CGameMovement( void )
{
	mv = NULL;

	memset( m_flStuckCheckTime, 0, sizeof(m_flStuckCheckTime) );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameMovement::~CGameMovement( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Allow bots etc to use slightly different solid masks
//-----------------------------------------------------------------------------
//unsigned int CGameMovement::PlayerSolidMask( bool brushOnly )
//{
//	return ( brushOnly ) ? MASK_PLAYERSOLID_BRUSHONLY : MASK_PLAYERSOLID;
//}

unsigned int CGameMovement::PlayerSolidMask( bool brushOnly )
{
	int mask = 0;
	switch ( player->GetTeamNumber() )
	{
	case TEAM_HOME:
		mask = CONTENTS_TEAM1;
		break;

	case TEAM_AWAY:
		mask = CONTENTS_TEAM2;
		break;
	}
	return ( mask | (( brushOnly ) ? MASK_PLAYERSOLID_BRUSHONLY : MASK_PLAYERSOLID) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : int
//-----------------------------------------------------------------------------
int CGameMovement::GetCheckInterval( IntervalType_t type )
{
	int tickInterval = 1;
	switch ( type )
	{
	default:
		tickInterval = 1;
		break;
	case GROUND:
		tickInterval = CATEGORIZE_GROUND_SURFACE_TICK_INTERVAL;
		break;
	case STUCK:
		// If we are in the process of being "stuck", then try a new position every command tick until m_StuckLast gets reset back down to zero
		if ( player->m_StuckLast != 0 )
		{
			tickInterval = 1;
		}
		else
		{
			if ( gpGlobals->maxClients == 1 )
			{
				tickInterval = CHECK_STUCK_TICK_INTERVAL_SP;
			}
			else
			{
				tickInterval = CHECK_STUCK_TICK_INTERVAL;
			}
		}
		break;
	case LADDER:
		tickInterval = CHECK_LADDER_TICK_INTERVAL;
		break;
	}
	return tickInterval;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameMovement::CheckInterval( IntervalType_t type )
{
	//int tickInterval = GetCheckInterval( type );

	//if ( g_bMovementOptimizations )
	//{
	//	return (player->CurrentCommandNumber() + player->entindex()) % tickInterval == 0;
	//}
	//else
	{
		return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::CheckParameters( void )
{
	QAngle	v_angle;

	SetPlayerSpeed();

	if ( player->GetMoveType() != MOVETYPE_ISOMETRIC &&
		 player->GetMoveType() != MOVETYPE_NOCLIP )
	{
		float spd;
		float maxspeed;

		spd = ( mv->m_flForwardMove * mv->m_flForwardMove ) +
			  ( mv->m_flSideMove * mv->m_flSideMove ) +
			  ( mv->m_flUpMove * mv->m_flUpMove );

		maxspeed = mv->m_flClientMaxSpeed;
		if ( maxspeed != 0.0 )
		{
			mv->m_flMaxSpeed = min( maxspeed, mv->m_flMaxSpeed );
		}

		// Same thing but only do the sqrt if we have to.
		if ( ( spd != 0.0 ) && ( spd > mv->m_flMaxSpeed*mv->m_flMaxSpeed ) )
		{
			float fRatio = mv->m_flMaxSpeed / sqrt( spd );
			mv->m_flForwardMove *= fRatio;
			mv->m_flSideMove    *= fRatio;
			mv->m_flUpMove      *= fRatio;
		}
	}

	if ( player->GetFlags() & FL_FROZEN )
	{
		mv->m_flForwardMove = 0;
		mv->m_flSideMove    = 0;
		mv->m_flUpMove      = 0;
	}

	v_angle = mv->m_vecAngles;
	v_angle = v_angle + player->m_Local.m_vecPunchAngle;

	mv->m_vecAngles[ROLL] = 0.0; // v_angle[ ROLL ];
	mv->m_vecAngles[PITCH] = v_angle[PITCH];
	mv->m_vecAngles[YAW]   = v_angle[YAW];

	// Adjust client view angles to match values used on server.
	if ( mv->m_vecAngles[YAW] > 180.0f )
	{
		mv->m_vecAngles[YAW] -= 360.0f;
	}
}

void CGameMovement::ReduceTimers( void )
{
	CSDKPlayer *pPl = (CSDKPlayer *)player;

	int teamPosType;
#ifdef CLIENT_DLL
	teamPosType = GameResources()->GetTeamPosType(pPl->entindex());
#else
	teamPosType = pPl->GetTeamPosType();
#endif

	if (GetMatchBall()->m_eBallState != BALL_STATE_GOAL
		&& (mv->m_nButtons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT))
		&& (mv->m_nButtons & IN_SPEED))
	{
		pPl->m_Shared.SetStamina(pPl->m_Shared.GetStamina() - mp_stamina_drain.GetInt() * gpGlobals->frametime);
	}
	else
	{
		pPl->m_Shared.SetStamina(pPl->m_Shared.GetStamina() + mp_stamina_replenish.GetInt() * gpGlobals->frametime);
	}

	float frame_msec = 1000.0f * gpGlobals->frametime;

	if ( player->m_Local.m_flDucktime > 0 )
	{
		player->m_Local.m_flDucktime -= frame_msec;
		if ( player->m_Local.m_flDucktime < 0 )
		{
			player->m_Local.m_flDucktime = 0;
		}
	}
	if ( player->m_Local.m_flDuckJumpTime > 0 )
	{
		player->m_Local.m_flDuckJumpTime -= frame_msec;
		if ( player->m_Local.m_flDuckJumpTime < 0 )
		{
			player->m_Local.m_flDuckJumpTime = 0;
		}
	}
	if ( player->m_Local.m_flJumpTime > 0 )
	{
		player->m_Local.m_flJumpTime -= frame_msec;
		if ( player->m_Local.m_flJumpTime < 0 )
		{
			player->m_Local.m_flJumpTime = 0;
		}
	}
	if ( player->m_flSwimSoundTime > 0 )
	{
		player->m_flSwimSoundTime -= frame_msec;
		if ( player->m_flSwimSoundTime < 0 )
		{
			player->m_flSwimSoundTime = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMove - 
//-----------------------------------------------------------------------------
void CGameMovement::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove )
{
	Assert( pMove && pPlayer );

	float flStoreFrametime = gpGlobals->frametime;

	//!!HACK HACK: Adrian - slow down all player movement by this factor.
	//!!Blame Yahn for this one.
	gpGlobals->frametime *= pPlayer->GetLaggedMovementValue();

	// Cropping movement speed scales mv->m_fForwardSpeed etc. globally
	// Once we crop, we don't want to recursively crop again, so we set the crop
	//  flag globally here once per usercmd cycle.
	m_bSpeedCropped = false;
	
	// StartTrackPredictionErrors should have set this
	Assert( player == pPlayer );
	player = pPlayer;

	mv = pMove;
	mv->m_flMaxSpeed = sv_maxspeed.GetFloat();

	// CheckV( player->CurrentCommandNumber(), "StartPos", mv->GetAbsOrigin() );

	DiffPrint( "start %f %f %f", mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, mv->GetAbsOrigin().z );

	// Run the command.
	PlayerMove();

	FinishMove();

	DiffPrint( "end %f %f %f", mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, mv->GetAbsOrigin().z );

	// CheckV( player->CurrentCommandNumber(), "EndPos", mv->GetAbsOrigin() );

	//This is probably not needed, but just in case.
	gpGlobals->frametime = flStoreFrametime;

	player = NULL;
}

void CGameMovement::StartTrackPredictionErrors( CBasePlayer *pPlayer )
{
	player = pPlayer;

#if PREDICTION_ERROR_CHECK_LEVEL > 0
	StartCommand( CBaseEntity::IsServer(), player->CurrentCommandNumber() );
#endif
}

void CGameMovement::FinishTrackPredictionErrors( CBasePlayer *pPlayer )
{
#if PREDICTION_ERROR_CHECK_LEVEL > 0
	Assert( player == pPlayer );

	// DiffPrint( "end %f", player->m_Local.m_vecPunchAngleVel.m_Value.x );

	// Call validate at end of checking
	Validate( CBaseEntity::IsServer(), player->CurrentCommandNumber() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Sets ground entity
//-----------------------------------------------------------------------------
void CGameMovement::FinishMove( void )
{
	mv->m_nOldButtons = mv->m_nButtons;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::StartGravity( void )
{
	float ent_gravity;
	
	if (player->GetGravity())
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	mv->m_vecVelocity[2] -= (ent_gravity * sv_gravity.GetFloat() * 0.5 * gpGlobals->frametime );
	mv->m_vecVelocity[2] += player->GetBaseVelocity()[2] * gpGlobals->frametime;

	Vector temp = player->GetBaseVelocity();
	temp[ 2 ] = 0;
	player->SetBaseVelocity( temp );

	CheckVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: Does the basic move attempting to climb up step heights.  It uses
//          the mv->GetAbsOrigin() and mv->m_vecVelocity.  It returns a new
//          new mv->GetAbsOrigin(), mv->m_vecVelocity, and mv->m_outStepHeight.
//-----------------------------------------------------------------------------
void CGameMovement::StepMove( Vector &vecDestination, trace_t &trace )
{
	Vector vecEndPos;
	VectorCopy( vecDestination, vecEndPos );

	// Try sliding forward both on ground and up 16 pixels
	//  take the move that goes farthest
	Vector vecPos, vecVel;
	VectorCopy( mv->GetAbsOrigin(), vecPos );
	VectorCopy( mv->m_vecVelocity, vecVel );

	// Slide move down.
	TryPlayerMove( &vecEndPos, &trace );
	
	// Down results.
	Vector vecDownPos, vecDownVel;
	VectorCopy( mv->GetAbsOrigin(), vecDownPos );
	VectorCopy( mv->m_vecVelocity, vecDownVel );
	
	// Reset original values.
	mv->SetAbsOrigin( vecPos );
	VectorCopy( vecVel, mv->m_vecVelocity );
	
	// Move up a stair height.
	VectorCopy( mv->GetAbsOrigin(), vecEndPos );
	if ( player->m_Local.m_bAllowAutoMovement )
	{
		//vecEndPos.z += player->m_Local.m_flStepSize + DIST_EPSILON;
		vecEndPos.z += 2.0f + DIST_EPSILON;		//ios - no steps
	}
	
	TracePlayerBBox( mv->GetAbsOrigin(), vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	if ( !trace.startsolid && !trace.allsolid )
	{
		mv->SetAbsOrigin( trace.endpos );
	}
	
	// Slide move up.
	TryPlayerMove();
	
	// Move down a stair (attempt to).
	VectorCopy( mv->GetAbsOrigin(), vecEndPos );
	if ( player->m_Local.m_bAllowAutoMovement )
	{
		//vecEndPos.z -= player->m_Local.m_flStepSize + DIST_EPSILON;
		vecEndPos.z -= 2.0f + DIST_EPSILON;		//ios - no steps
	}
		
	TracePlayerBBox( mv->GetAbsOrigin(), vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	
	// If we are not on the ground any more then use the original movement attempt.
	if ( trace.plane.normal[2] < 0.7 )
	{
		mv->SetAbsOrigin( vecDownPos );
		VectorCopy( vecDownVel, mv->m_vecVelocity );
		float flStepDist = mv->GetAbsOrigin().z - vecPos.z;
		if ( flStepDist > 0.0f )
		{
			mv->m_outStepHeight += flStepDist;
		}
		return;
	}
	
	// If the trace ended up in empty space, copy the end over to the origin.
	if ( !trace.startsolid && !trace.allsolid )
	{
		mv->SetAbsOrigin( trace.endpos );
	}
	
	// Copy this origin to up.
	Vector vecUpPos;
	VectorCopy( mv->GetAbsOrigin(), vecUpPos );
	
	// decide which one went farther
	float flDownDist = ( vecDownPos.x - vecPos.x ) * ( vecDownPos.x - vecPos.x ) + ( vecDownPos.y - vecPos.y ) * ( vecDownPos.y - vecPos.y );
	float flUpDist = ( vecUpPos.x - vecPos.x ) * ( vecUpPos.x - vecPos.x ) + ( vecUpPos.y - vecPos.y ) * ( vecUpPos.y - vecPos.y );
	if ( flDownDist > flUpDist )
	{
		mv->SetAbsOrigin( vecDownPos );
		VectorCopy( vecDownVel, mv->m_vecVelocity );
	}
	else 
	{
		// copy z value from slide move
		mv->m_vecVelocity.z = vecDownVel.z;
	}
	
	float flStepDist = mv->GetAbsOrigin().z - vecPos.z;
	if ( flStepDist > 0 )
	{
		mv->m_outStepHeight += flStepDist;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::Friction( void )
{
	float	speed, newspeed, control;
	float	friction;
	float	drop;

	// Calculate speed
	speed = VectorLength( mv->m_vecVelocity );
	
	// If too slow, return
	if (speed < 0.1f)
	{
		return;
	}

	drop = 0;

	// apply ground friction
	if (player->GetGroundEntity() != NULL)  // On an entity that is the ground
	{
		friction = sv_friction.GetFloat() * player->m_surfaceFriction;

		// Bleed off some speed, but if we have less than the bleed
		//  threshold, bleed the threshold amount.

		control = (speed < sv_stopspeed.GetFloat()) ? sv_stopspeed.GetFloat() : speed;

		// Add the amount to the drop amount.
		drop += control*friction*gpGlobals->frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;

	if ( newspeed != speed )
	{
		// Determine proportion of old speed we are using.
		newspeed /= speed;
		// Adjust velocity according to proportion.
		VectorScale( mv->m_vecVelocity, newspeed, mv->m_vecVelocity );
	}

 	mv->m_outWishVel -= (1.f-newspeed) * mv->m_vecVelocity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FinishGravity( void )
{
	float ent_gravity;

	if ( player->GetGravity() )
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Get the correct velocity for the end of the dt 
  	mv->m_vecVelocity[2] -= (ent_gravity * sv_gravity.GetFloat() * gpGlobals->frametime * 0.5);

	CheckVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wishdir - 
//			accel - 
//-----------------------------------------------------------------------------
void CGameMovement::AirAccelerate( Vector& wishdir, float wishspeed, float accel )
{
	int i;
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;
	
	if (player->pl.deadflag)
		return;

	// Cap speed
	if (wishspd > 30)
		wishspd = 30;

	// Determine veer amount
	currentspeed = mv->m_vecVelocity.Dot(wishdir);

	// See how much to add
	addspeed = wishspd - currentspeed;

	// If not adding any, done.
	if (addspeed <= 0)
		return;

	// Determine acceleration speed after acceleration
	accelspeed = accel * wishspeed * gpGlobals->frametime * player->m_surfaceFriction;

	// Cap it
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	// Adjust pmove vel.
	for (i=0 ; i<3 ; i++)
	{
		mv->m_vecVelocity[i] += accelspeed * wishdir[i];
		mv->m_outWishVel[i] += accelspeed * wishdir[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::AirMove( void )
{
	int			i;
	Vector		wishvel;
	float		fmove, smove;
	Vector		wishdir;
	float		wishspeed;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles
	
	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;
	
	// Zero out z components of movement vectors
	forward[2] = 0;
	right[2]   = 0;
	VectorNormalize(forward);  // Normalize remainder of vectors
	VectorNormalize(right);    // 

	for (i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if ( wishspeed != 0 && (wishspeed > mv->m_flMaxSpeed))
	{
		VectorScale (wishvel, mv->m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = mv->m_flMaxSpeed;
	}
	
	AirAccelerate( wishdir, wishspeed, sv_airaccelerate.GetFloat() );

	// Add in any base velocity to the current velocity.
	VectorAdd(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	TryPlayerMove();

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
}


bool CGameMovement::CanAccelerate()
{
	// Dead players don't accelerate.
	if (player->pl.deadflag)
		return false;

	// If waterjumping, don't accelerate
	if (player->m_flWaterJumpTime)
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wishdir - 
//			wishspeed - 
//			accel - 
//-----------------------------------------------------------------------------
void CGameMovement::Accelerate( Vector& wishdir, float wishspeed, float accel )
{
	int i;
	float addspeed, accelspeed, currentspeed;

	// This gets overridden because some games (CSPort) want to allow dead (observer) players
	// to be able to move around.
	if ( !CanAccelerate() )
		return;

	//omega; WALLSTRAFE FIX

	// See if we are changing direction a bit
	//	currentspeed = mv->m_vecVelocity.Dot(wishdir);

	currentspeed = sqrt( DotProduct(mv->m_vecVelocity, mv->m_vecVelocity) );
	//omega; END WALLSTRAFE FIX

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of accleration.
	accelspeed = accel * gpGlobals->frametime * wishspeed * player->m_surfaceFriction;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	// Adjust velocity.
	for (i=0 ; i<3 ; i++)
	{
		mv->m_vecVelocity[i] += accelspeed * wishdir[i];	
	}
}

//-----------------------------------------------------------------------------
// Purpose: Try to keep a walking player on the ground when running down slopes etc
//-----------------------------------------------------------------------------
void CGameMovement::StayOnGround( void )
{
	trace_t trace;
	Vector start( mv->GetAbsOrigin() );
	Vector end( mv->GetAbsOrigin() );
	start.z += 2;
	end.z -= player->GetStepSize();

	// See how far up we can go without getting stuck

	TracePlayerBBox( mv->GetAbsOrigin(), start, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	start = trace.endpos;

	// using trace.startsolid is unreliable here, it doesn't get set when
	// tracing bounding box vs. terrain

	// Now trace down from a known safe position
	TracePlayerBBox( start, end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	if ( trace.fraction > 0.0f &&			// must go somewhere
		trace.fraction < 1.0f &&			// must hit something
		!trace.startsolid &&				// can't be embedded in a solid
		trace.plane.normal[2] >= 0.7 )		// can't hit a steep slope that we can't stand on anyway
	{
		float flDelta = fabs(mv->GetAbsOrigin().z - trace.endpos.z);

		//This is incredibly hacky. The real problem is that trace returning that strange value we can't network over.
		if ( flDelta > 0.5f * COORD_RESOLUTION)
		{
			mv->SetAbsOrigin( trace.endpos );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::WalkMove( void )
{
	Vector wishvel;
	float spd;
	float fmove, smove;
	Vector wishdir;
	float wishspeed;

	Vector dest;
	trace_t pm;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	CHandle< CBaseEntity > oldground;
	oldground = player->GetGroundEntity();
	
	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;

	// Zero out z components of movement vectors
	if ( forward[2] != 0 )
	{
		forward[2] = 0;
		VectorNormalize( forward );
	}

	if ( right[2] != 0 )
	{
		right[2] = 0;
		VectorNormalize( right );
	}

	for (int i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	
	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to server defined max speed
	//
	if ((wishspeed != 0.0f) && (wishspeed > mv->m_flMaxSpeed))
	{
		VectorScale (wishvel, mv->m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = mv->m_flMaxSpeed;
	}

	// Set pmove velocity
	mv->m_vecVelocity[2] = 0;
	Accelerate ( wishdir, wishspeed, sv_accelerate.GetFloat() );
	mv->m_vecVelocity[2] = 0;

	spd = VectorLength( mv->m_vecVelocity );

	if ( spd < 1.0f )
	{
		mv->m_vecVelocity = vec3_origin;
		return;
	}

	float landingSpeed = mp_jumplandingspeed.GetFloat();

	if (ToSDKPlayer(player)->m_Shared.m_bWasJumping && spd > landingSpeed)
	{
		VectorNormalize(mv->m_vecVelocity);
		mv->m_vecVelocity *= landingSpeed;
		ToSDKPlayer(player)->m_Shared.m_bWasJumping = false;
	}

	// first try just moving to the destination	
	dest[0] = mv->GetAbsOrigin()[0] + mv->m_vecVelocity[0]*gpGlobals->frametime;
	dest[1] = mv->GetAbsOrigin()[1] + mv->m_vecVelocity[1]*gpGlobals->frametime;	
	dest[2] = mv->GetAbsOrigin()[2];

	// first try moving directly to the next spot
	TracePlayerBBox( mv->GetAbsOrigin(), dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );

	// If we made it all the way, then copy trace end as new player position.
	mv->m_outWishVel += wishdir * wishspeed;

	if ( pm.fraction == 1 )
	{
		mv->SetAbsOrigin( pm.endpos );
		StayOnGround();
		return;
	}

	// Don't walk up stairs if not on ground.
	if ( oldground == NULL && player->GetWaterLevel()  == 0 )
	{
		return;
	}

	//if (pm.m_pEnt->GetMoveType() == MOVETYPE_VPHYSICS)		//ios vphysics hacks
	//{
	//	MoveHelper( )->AddToTouched( pm, mv->m_vecVelocity );		
	//}
	//else
	//	StepMove( dest, pm );

	StepMove( dest, pm );
	StayOnGround();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FullWalkMove( )
{
	CSDKPlayer *pPl = ToSDKPlayer(player);
	Vector oldPos = mv->GetAbsOrigin();
	Vector oldVel = mv->m_vecVelocity;
	QAngle oldAng = mv->m_vecAbsViewAngles;

	bool isWalkMove = false;

	if (CheckActionOverTime())
	{
		TryPlayerMove();
	}
	else if (!SDKGameRules()->IsCeremony() && CheckActionStart() && CheckActionOverTime())
	{
#ifdef GAME_DLL
		if (!CSDKPlayer::IsOnField(pPl))
			return;
#endif

		TryPlayerMove();
	}
	else
	{
		StartGravity();

		// Fricion is handled before we add in any base velocity. That way, if we are on a conveyor, 
		//  we don't slow when standing still, relative to the conveyor.
		if (player->GetGroundEntity() != NULL)
		{
			mv->m_vecVelocity[2] = 0.0;
			Friction();
		}

		// Make sure velocity is valid.
		CheckVelocity();

		if (player->GetGroundEntity() != NULL)
		{
			WalkMove();
			isWalkMove = true;
		}
		else
		{
			AirMove();  // Take into account movement when in air.
		}

		// Set final flags.
		CategorizePosition();

		// Make sure velocity is valid.
		CheckVelocity();

		FinishGravity();

		// If we are on ground, no downward velocity.
		if ( player->GetGroundEntity() != NULL )
		{
			mv->m_vecVelocity[2] = 0;
		}
	}

	Vector newPos = mv->GetAbsOrigin();

	if (player->GetFlags() & FL_NO_X_MOVEMENT)
	{
		mv->m_vecVelocity[0] = 0;
		mv->SetAbsOrigin(Vector(oldPos.x, newPos.y, newPos.z));
	}

	if (player->GetFlags() & FL_NO_Y_MOVEMENT)
	{
		mv->m_vecVelocity[1] = 0;
		mv->SetAbsOrigin(Vector(newPos.x, oldPos.y, newPos.z));
	}

	newPos = mv->GetAbsOrigin();
	Vector newVel = mv->m_vecVelocity;
	QAngle newAng = mv->m_vecAbsViewAngles;

	pPl->CheckBallShield(oldPos, newPos, oldVel, newVel, oldAng, newAng);

	mv->SetAbsOrigin(newPos);
	mv->m_vecVelocity = newVel;
	mv->m_vecAbsViewAngles = newAng;

	Vector forward, right, up;
	AngleVectors(mv->m_vecAbsViewAngles, &forward, &right, &up);

	pPl->m_Shared.m_nInPenBoxOfTeam = TEAM_NONE;

	for (int team = TEAM_HOME; team <= TEAM_AWAY; team++)
	{
		float halfBounds = (player->GetPlayerMaxs().x - player->GetPlayerMins().x) / 2;

		Vector min = GetGlobalTeam(team)->m_vPenBoxMin;
		Vector max = GetGlobalTeam(team)->m_vPenBoxMax;

		if (GetGlobalTeam(team)->m_nForward == 1)
			min.y -= 500;
		else
			max.y += 500;

		if (mv->GetAbsOrigin().x >= min.x - halfBounds
			&& mv->GetAbsOrigin().y >= min.y - halfBounds
			&& mv->GetAbsOrigin().x <= max.x + halfBounds
			&& mv->GetAbsOrigin().y <= max.y + halfBounds)
		{
			pPl->m_Shared.m_nInPenBoxOfTeam = team;
			break;
		}
	}

#ifdef GAME_DLL
	if (!SDKGameRules()->IsIntermissionState() && GetMatchBall()->State_Get() == BALL_STATE_NORMAL && !GetMatchBall()->HasQueuedState() && newPos != oldPos)
	{
		pPl->GetData()->AddExactDistanceCovered((newPos - oldPos).Length2D() * 2.54f / 100);
	}
#endif
}

void CGameMovement::MoveToTargetPos()
{
	#ifdef GAME_DLL
		Vector pos = mv->GetAbsOrigin();
		Vector vel = mv->m_vecVelocity;
		QAngle ang = mv->m_vecAbsViewAngles;

		ToSDKPlayer(player)->MoveToTargetPos(pos, vel, ang);

		mv->SetAbsOrigin(pos);
		mv->m_vecVelocity = vel;
		mv->m_vecAbsViewAngles = ang;
		player->SnapEyeAngles(ang);
	#endif
}

bool CGameMovement::CheckActionOverTime()
{
	CSDKPlayer *pPl = ToSDKPlayer(player);

#ifdef GAME_DLL
	if (!CSDKPlayer::IsOnField(pPl))
		return true;
#endif

	float timePassed = gpGlobals->curtime - pPl->m_Shared.GetActionStartTime();
	Vector forward, right, up;
	AngleVectors(mv->m_vecViewAngles, &forward, &right, &up);
	Vector forward2D = forward;
	forward2D.z = 0;
	forward2D.NormalizeInPlace();

	int sidemoveSign = pPl->GetSidemoveSign();

	switch (pPl->m_Shared.GetAction())
	{
	case PLAYERANIMEVENT_KEEPER_DIVE_FORWARD:
	case PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD:
	case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT:
	case PLAYERANIMEVENT_KEEPER_DIVE_LEFT:
	{
		if (timePassed > mp_keeperdive_move_duration.GetFloat() + mp_keeperdive_idle_duration.GetFloat())
		{
			pPl->DoAnimationEvent(PLAYERANIMEVENT_NONE);
			return false;
		}

		int forwardmoveSign = (mv->m_nButtons & IN_FORWARD) ? 1 : ((mv->m_nButtons & IN_BACK) ? -1 : 0);
		mv->m_vecVelocity = forward2D * forwardmoveSign * mp_keeperdivespeed_longside.GetInt() + right * sidemoveSign * mp_keeperdivespeed_longside.GetInt();
		float speed = mv->m_vecVelocity.NormalizeInPlace();
		mv->m_vecVelocity *= min(speed, mp_keeperdivespeed_longside.GetInt());
		mv->m_vecVelocity *= max(0, (1 - pow(timePassed / mp_keeperdive_move_duration.GetFloat(), 2)));

		if (pPl->m_Shared.GetAction() == PLAYERANIMEVENT_KEEPER_DIVE_FORWARD)
			mv->m_vecVelocity.z = 0;
		else if (pPl->m_Shared.GetAction() == PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD)
			mv->m_vecVelocity.z = mp_jump_height.GetInt();
		else
			mv->m_vecVelocity.z = mp_keeperdivespeed_z.GetInt();

		break;
	}
	case PLAYERANIMEVENT_SLIDE_TACKLE:
	case PLAYERANIMEVENT_CELEB_SLIDE:
	{
		if (timePassed > mp_slide_move_duration.GetFloat() + mp_slide_idle_duration.GetFloat())
		{
			pPl->DoAnimationEvent(PLAYERANIMEVENT_NONE);
			return false;
		}

		AngleVectors(pPl->m_Shared.GetActionStartAngle(), &forward2D);
		forward2D.z = 0;
		forward2D.NormalizeInPlace();

		mv->m_vecVelocity = forward2D * mp_slidespeed.GetInt() * max(0, (1 - pow(timePassed / mp_slide_move_duration.GetFloat(), 2)));
		break;
	}
	case PLAYERANIMEVENT_TACKLED_FORWARD:
	case PLAYERANIMEVENT_TACKLED_BACKWARD:
	{
		if (timePassed > mp_tackled_idle_duration.GetFloat())
		{
			pPl->DoAnimationEvent(PLAYERANIMEVENT_NONE);
			return false;
		}

		mv->m_vecVelocity = vec3_origin;
		break;
	}
	case PLAYERANIMEVENT_THROW_IN_THROW:
	{
		if (timePassed > mp_throwinthrow_idle_duration.GetFloat())
		{
			pPl->DoAnimationEvent(PLAYERANIMEVENT_NONE);
			return false;
		}

		mv->m_vecVelocity = vec3_origin;
		break;
	}
	case PLAYERANIMEVENT_DIVING_HEADER:
	{
		if (timePassed > mp_divingheader_move_duration.GetFloat() + mp_divingheader_idle_duration.GetFloat())
		{
			pPl->DoAnimationEvent(PLAYERANIMEVENT_NONE);
			return false;
		}

		mv->m_vecVelocity = forward2D * mp_divingheaderspeed.GetInt() * max(0, (1 - pow(timePassed / mp_divingheader_move_duration.GetFloat(), 2)));
		break;
	}
	case PLAYERANIMEVENT_BICYCLE_KICK:
	{
		if (timePassed > mp_bicycleshot_move_duration.GetFloat() + mp_bicycleshot_idle_duration.GetFloat())
		{
			pPl->DoAnimationEvent(PLAYERANIMEVENT_NONE);
			return false;
		}

		mv->m_vecVelocity = Vector(0, 0, 0);
		break;
	}
	default:
	{
		return false;
	}
	}

	mv->m_vecVelocity.z -= sv_gravity.GetFloat() * timePassed;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FullNoClipMove( float factor, float maxacceleration )
{
	Vector wishvel;
	Vector forward, right, up;
	Vector wishdir;
	float wishspeed;
	float maxspeed = sv_maxspeed.GetFloat() * factor;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	if ( mv->m_nButtons & IN_SPEED )
	{
		factor /= 2.0f;
	}
	
	// Copy movement amounts
	float fmove = mv->m_flForwardMove * factor;
	float smove = mv->m_flSideMove * factor;
	
	VectorNormalize (forward);  // Normalize remainder of vectors
	VectorNormalize (right);    // 

	for (int i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] += mv->m_flUpMove * factor;

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to server defined max speed
	//
	if (wishspeed > maxspeed )
	{
		VectorScale (wishvel, maxspeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}

	if ( maxacceleration > 0.0 )
	{
		// Set pmove velocity
		Accelerate ( wishdir, wishspeed, maxacceleration );

		float spd = VectorLength( mv->m_vecVelocity );
		if (spd < 1.0f)
		{
			mv->m_vecVelocity.Init();
			return;
		}
		
		// Bleed off some speed, but if we have less than the bleed
		//  threshhold, bleed the theshold amount.
		float control = (spd < maxspeed/4.0) ? maxspeed/4.0 : spd;
		
		float friction = sv_friction.GetFloat() * player->m_surfaceFriction;
				
		// Add the amount to the drop amount.
		float drop = control * friction * gpGlobals->frametime;

		// scale the velocity
		float newspeed = spd - drop;
		if (newspeed < 0)
			newspeed = 0;

		// Determine proportion of old speed we are using.
		newspeed /= spd;
		VectorScale( mv->m_vecVelocity, newspeed, mv->m_vecVelocity );
	}
	else
	{
		VectorCopy( wishvel, mv->m_vecVelocity );
	}

	// Just move ( don't clip or anything )
	Vector out;
	VectorMA( mv->GetAbsOrigin(), gpGlobals->frametime, mv->m_vecVelocity, out );
	mv->SetAbsOrigin( out );

	// Zero out velocity if in noaccel mode
	if ( maxacceleration < 0.0f )
	{
		mv->m_vecVelocity.Init();
	}
}

bool CGameMovement::CheckActionStart()
{
	CSDKPlayer *pPl = ToSDKPlayer(player);

	if (pPl->GetFlags() & (FL_FROZEN | FL_ATCONTROLS | FL_REMOTECONTROLLED | FL_ONLY_XY_MOVEMENT) || !pPl->GetGroundEntity())
		return false;

	PlayerAnimEvent_t animEvent = PLAYERANIMEVENT_NONE;

	if (mv->m_nButtons & IN_JUMP)
	{
		if (pPl->IsInOwnBoxAsKeeper())
		{
			int sidemoveSign = pPl->GetSidemoveSign();

			if (sidemoveSign == -1)
				animEvent = PLAYERANIMEVENT_KEEPER_DIVE_LEFT;
			else if (sidemoveSign == 1)
				animEvent = PLAYERANIMEVENT_KEEPER_DIVE_RIGHT;
			else
				animEvent = PLAYERANIMEVENT_KEEPER_JUMP;
		}
		else
		{
			animEvent = PLAYERANIMEVENT_JUMP;
		}
	}
	else if (mv->m_nButtons & IN_DUCK)
	{
		if (pPl->IsInOwnBoxAsKeeper() && !(mv->m_nButtons & IN_SKILL))
		{
			if (mv->m_nButtons & IN_FORWARD)
				animEvent = PLAYERANIMEVENT_KEEPER_DIVE_FORWARD;
			else if (mv->m_nButtons & IN_BACK)
				animEvent = PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD;
		}
		else
		{
			if (mv->m_nButtons & IN_FORWARD)
			{
				if ((mv->m_nButtons & IN_GESTURE) && ((pPl->GetFlags() & FL_CELEB) || SDKGameRules()->State_Get() == MATCH_PERIOD_WARMUP))
				{
					animEvent = PLAYERANIMEVENT_CELEB_SLIDE;
				}
				else
				{
					animEvent = PLAYERANIMEVENT_SLIDE_TACKLE;

#ifdef GAME_DLL
					if (!SDKGameRules()->IsIntermissionState() && GetMatchBall()->State_Get() == BALL_STATE_NORMAL && !GetMatchBall()->HasQueuedState())
					{
						pPl->GetData()->AddSlidingTackle();

						GetMatchBall()->CheckFoul(pPl);

						if (!CSDKPlayer::IsOnField(pPl))
							return true;
					}
#endif				
				}
			}
		}
	}
	else if (mv->m_nButtons & IN_SKILL)
	{
		if ((mv->m_nButtons & IN_FORWARD) && pPl->IsChargedshooting())
		{
			animEvent = PLAYERANIMEVENT_DIVING_HEADER;
			pPl->AddFlag(FL_FREECAM);
		}
		else if ((mv->m_nButtons & IN_BACK) && pPl->IsChargedshooting())
		{
			animEvent = PLAYERANIMEVENT_BICYCLE_KICK;
			pPl->AddFlag(FL_FREECAM);
		}
	}

	switch (animEvent)
	{
	case PLAYERANIMEVENT_NONE:
	{
		return true;
	}
	case PLAYERANIMEVENT_JUMP:
	case PLAYERANIMEVENT_KEEPER_JUMP:
	{
		mv->m_vecVelocity.z = mp_jump_height.GetInt();
		SetGroundEntity(NULL);
		player->PlayStepSound((Vector &)mv->GetAbsOrigin(), player->m_pSurfaceData, 1.0, true);
		break;
	}
	case PLAYERANIMEVENT_KEEPER_DIVE_LEFT:
	case PLAYERANIMEVENT_KEEPER_DIVE_RIGHT:
	case PLAYERANIMEVENT_KEEPER_DIVE_FORWARD:
	case PLAYERANIMEVENT_KEEPER_DIVE_BACKWARD:
	{
		SetGroundEntity(NULL);
		pPl->ResetShotCharging();
		MoveHelper()->StartSound(mv->GetAbsOrigin(), "Player.DiveKeeper");
		break;
	}
	case PLAYERANIMEVENT_DIVING_HEADER:
	case PLAYERANIMEVENT_BICYCLE_KICK:
	{
		MoveHelper()->StartSound(mv->GetAbsOrigin(), "Player.DivingHeader");
	}
	case PLAYERANIMEVENT_SLIDE_TACKLE:
	case PLAYERANIMEVENT_CELEB_SLIDE:
	{
		MoveHelper()->StartSound(mv->GetAbsOrigin(), "Player.Slide");
	}
	}

	pPl->m_Shared.SetActionStartAngle(mv->m_vecAbsViewAngles);
	pPl->m_Shared.SetActionStartButtons(mv->m_nButtons);
	pPl->DoAnimationEvent(animEvent);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CGameMovement::TryPlayerMove( Vector *pFirstDest, trace_t *pFirstTrace )
{
	int			bumpcount, numbumps;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[MAX_CLIP_PLANES];
	Vector		primal_velocity, original_velocity;
	Vector      new_velocity;
	int			i, j;
	trace_t	pm;
	Vector		end;
	float		time_left, allFraction;
	int			blocked;		
	
	numbumps  = 4;           // Bump up to four times
	
	blocked   = 0;           // Assume not blocked
	numplanes = 0;           //  and not sliding along any planes

	VectorCopy (mv->m_vecVelocity, original_velocity);  // Store original velocity
	VectorCopy (mv->m_vecVelocity, primal_velocity);
	
	allFraction = 0;
	time_left = gpGlobals->frametime;   // Total time for this movement operation.

	new_velocity.Init();

	for (bumpcount=0 ; bumpcount < numbumps; bumpcount++)
	{
		if ( mv->m_vecVelocity.Length() == 0.0 )
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.
		VectorMA( mv->GetAbsOrigin(), time_left, mv->m_vecVelocity, end );

		// If their velocity Z is 0, then we can avoid an extra trace here during WalkMove.
		if ( pFirstDest && end == *pFirstDest )
			pm = *pFirstTrace;
		else
		{
			TracePlayerBBox( mv->GetAbsOrigin(), end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
		}

		allFraction += pm.fraction;

		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if (pm.allsolid)
		{	
			// entity is trapped in another solid
			VectorCopy (vec3_origin, mv->m_vecVelocity);
			return 4;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove.origin and 
		//  zero the plane counter.
		if( pm.fraction > 0 )
		{	
			if ( numbumps > 0 && pm.fraction == 1 )
			{
				// There's a precision issue with terrain tracing that can cause a swept box to successfully trace
				// when the end position is stuck in the triangle.  Re-run the test with an uswept box to catch that
				// case until the bug is fixed.
				// If we detect getting stuck, don't allow the movement
				trace_t stuck;
				TracePlayerBBox( pm.endpos, pm.endpos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, stuck );
				if ( stuck.startsolid || stuck.fraction != 1.0f )
				{
					//Msg( "Player will become stuck!!!\n" );
					VectorCopy (vec3_origin, mv->m_vecVelocity);
					break;
				}
			}

			// actually covered some distance
			mv->SetAbsOrigin( pm.endpos);
			VectorCopy (mv->m_vecVelocity, original_velocity);
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if (pm.fraction == 1)
		{
			 break;		// moved the entire distance
		}

		// Save entity that blocked us (since fraction was < 1.0)
		//  for contact
		// Add it if it's not already in the list!!!
		MoveHelper( )->AddToTouched( pm, mv->m_vecVelocity );

		// If the plane we hit has a high z component in the normal, then
		//  it's probably a floor
		if (pm.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
		}
		// If the plane has a zero z component in the normal, then it's a 
		//  step or wall
		if (!pm.plane.normal[2])
		{
			blocked |= 2;		// step / wall
		}

		// Reduce amount of m_flFrameTime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * pm.fraction;

		// Did we run out of planes to clip against?
		if (numplanes >= MAX_CLIP_PLANES)
		{	
			// this shouldn't really happen
			//  Stop our movement if so.
			VectorCopy (vec3_origin, mv->m_vecVelocity);
			//Con_DPrintf("Too many planes 4\n");

			break;
		}

		// Set up next clipping plane
		VectorCopy (pm.plane.normal, planes[numplanes]);
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		//

		// reflect player velocity 
		// Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
		//  and pressing forward and nobody was really using this bounce/reflection feature anyway...
		if ( numplanes == 1 &&
			player->GetMoveType() == MOVETYPE_WALK &&
			player->GetGroundEntity() == NULL )	
		{
			for ( i = 0; i < numplanes; i++ )
			{
				if ( planes[i][2] > 0.7  )
				{
					// floor or slope
					ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
					VectorCopy( new_velocity, original_velocity );
				}
				else
				{
					ClipVelocity( original_velocity, planes[i], new_velocity, 1.0 + sv_bounce.GetFloat() * (1 - player->m_surfaceFriction) );
				}
			}

			VectorCopy( new_velocity, mv->m_vecVelocity );
			VectorCopy( new_velocity, original_velocity );
		}
		else
		{
			for (i=0 ; i < numplanes ; i++)
			{
				ClipVelocity (
					original_velocity,
					planes[i],
					mv->m_vecVelocity,
					1);

				for (j=0 ; j<numplanes ; j++)
					if (j != i)
					{
						// Are we now moving against this plane?
						if (mv->m_vecVelocity.Dot(planes[j]) < 0)
							break;	// not ok
					}
				if (j == numplanes)  // Didn't have to clip, so we're ok
					break;
			}
			
			// Did we go all the way through plane set
			if (i != numplanes)
			{	// go along this plane
				// pmove.velocity is set in clipping call, no need to set again.
				;  
			}
			else
			{	// go along the crease
				if (numplanes != 2)
				{
					VectorCopy (vec3_origin, mv->m_vecVelocity);
					break;
				}
				CrossProduct (planes[0], planes[1], dir);
				dir.NormalizeInPlace();
				d = dir.Dot(mv->m_vecVelocity);
				VectorScale (dir, d, mv->m_vecVelocity );
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			//
			d = mv->m_vecVelocity.Dot(primal_velocity);
			if (d <= 0)
			{
				//Con_DPrintf("Back\n");
				VectorCopy (vec3_origin, mv->m_vecVelocity);
				break;
			}
		}
	}

	if ( allFraction == 0 )
	{
		VectorCopy (vec3_origin, mv->m_vecVelocity);
	}

	return blocked;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : axis - 
// Output : const char
//-----------------------------------------------------------------------------
#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)
const char *DescribeAxis( int axis )
{
	static char sz[ 32 ];

	switch ( axis )
	{
	case 0:
		Q_strncpy( sz, "X", sizeof( sz ) );
		break;
	case 1:
		Q_strncpy( sz, "Y", sizeof( sz ) );
		break;
	case 2:
	default:
		Q_strncpy( sz, "Z", sizeof( sz ) );
		break;
	}

	return sz;
}
#else
const char *DescribeAxis( int axis );
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::CheckVelocity( void )
{
	int i;

	//
	// bound velocity
	//

	Vector org = mv->GetAbsOrigin();

	for (i=0; i < 3; i++)
	{
		// See if it's bogus.
		if (IS_NAN(mv->m_vecVelocity[i]))
		{
			DevMsg( 1, "PM  Got a NaN velocity %s\n", DescribeAxis( i ) );
			mv->m_vecVelocity[i] = 0;
		}

		if (IS_NAN(org[i]))
		{
			DevMsg( 1, "PM  Got a NaN origin on %s\n", DescribeAxis( i ) );
			org[ i ] = 0;
			mv->SetAbsOrigin( org );
		}

		// Bound it.
		if (mv->m_vecVelocity[i] > sv_maxvelocity.GetFloat()) 
		{
			DevMsg( 1, "PM  Got a velocity too high on %s\n", DescribeAxis( i ) );
			mv->m_vecVelocity[i] = sv_maxvelocity.GetFloat();
		}
		else if (mv->m_vecVelocity[i] < -sv_maxvelocity.GetFloat())
		{
			DevMsg( 1, "PM  Got a velocity too low on %s\n", DescribeAxis( i ) );
			mv->m_vecVelocity[i] = -sv_maxvelocity.GetFloat();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::AddGravity( void )
{
	float ent_gravity;

	if ( player->m_flWaterJumpTime )
		return;

	if (player->GetGravity())
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Add gravity incorrectly
	mv->m_vecVelocity[2] -= (ent_gravity * sv_gravity.GetFloat() * gpGlobals->frametime);
	mv->m_vecVelocity[2] += player->GetBaseVelocity()[2] * gpGlobals->frametime;
	Vector temp = player->GetBaseVelocity();
	temp[2] = 0;
	player->SetBaseVelocity( temp );
	
	CheckVelocity();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : push - 
// Output : trace_t
//-----------------------------------------------------------------------------
void CGameMovement::PushEntity( Vector& push, trace_t *pTrace )
{
	Vector	end;
		
	VectorAdd (mv->GetAbsOrigin(), push, end);
	TracePlayerBBox( mv->GetAbsOrigin(), end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, *pTrace );
	mv->SetAbsOrigin( pTrace->endpos );

	// So we can run impact function afterwards.
	// If
	if ( pTrace->fraction < 1.0 && !pTrace->allsolid )
	{
		MoveHelper( )->AddToTouched( *pTrace, mv->m_vecVelocity );
	}
}	


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : in - 
//			normal - 
//			out - 
//			overbounce - 
// Output : int
//-----------------------------------------------------------------------------
int CGameMovement::ClipVelocity( Vector& in, Vector& normal, Vector& out, float overbounce )
{
	float	backoff;
	float	change;
	float angle;
	int		i, blocked;
	
	angle = normal[ 2 ];

	blocked = 0x00;         // Assume unblocked.
	if (angle > 0)			// If the plane that is blocking us has a positive z component, then assume it's a floor.
		blocked |= 0x01;	// 
	if (!angle)				// If the plane has no Z, it is vertical (wall/step)
		blocked |= 0x02;	// 
	

	// Determine how far along plane to slide based on incoming direction.
	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change; 
	}
	
	// iterate once to make sure we aren't still moving through the plane
	float adjust = DotProduct( out, normal );
	if( adjust < 0.0f )
	{
		out -= ( normal * adjust );
//		Msg( "Adjustment = %lf\n", adjust );
	}

	// Return blocking flags.
	return blocked;
}


#define CHECKSTUCK_MINTIME 0.05  // Don't check again too quickly.

#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)
Vector rgv3tStuckTable[54];
#else
extern Vector rgv3tStuckTable[54];
#endif

#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CreateStuckTable( void )
{
	float x, y, z;
	int idx;
	int i;
	float zi[3];
	static int firsttime = 1;

	if ( !firsttime )
		return;

	firsttime = 0;

	memset(rgv3tStuckTable, 0, sizeof(rgv3tStuckTable));

	idx = 0;
	// Little Moves.
	x = y = 0;
	// Z moves
	for (z = -0.125 ; z <= 0.125 ; z += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	x = z = 0;
	// Y moves
	for (y = -0.125 ; y <= 0.125 ; y += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -0.125 ; x <= 0.125 ; x += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for ( x = - 0.125; x <= 0.125; x += 0.250 )
	{
		for ( y = - 0.125; y <= 0.125; y += 0.250 )
		{
			for ( z = - 0.125; z <= 0.125; z += 0.250 )
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}

	// Big Moves.
	x = y = 0;
	zi[0] = 0.0f;
	zi[1] = 1.0f;
	zi[2] = 6.0f;

	for (i = 0; i < 3; i++)
	{
		// Z moves
		z = zi[i];
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	x = z = 0;

	// Y moves
	for (y = -2.0f ; y <= 2.0f ; y += 2.0)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -2.0f ; x <= 2.0f ; x += 2.0f)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for (i = 0 ; i < 3; i++)
	{
		z = zi[i];
		
		for (x = -2.0f ; x <= 2.0f ; x += 2.0f)
		{
			for (y = -2.0f ; y <= 2.0f ; y += 2.0)
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}
	Assert( idx < sizeof(rgv3tStuckTable)/sizeof(rgv3tStuckTable[0]));
}
#else
extern void CreateStuckTable( void );
#endif

int GetRandomStuckOffsets( CBasePlayer *pPlayer, Vector& offset)
{
 // Last time we did a full
	int idx;
	idx = pPlayer->m_StuckLast++;

	VectorCopy(rgv3tStuckTable[idx % 54], offset);

	return (idx % 54);
}

void ResetStuckOffsets( CBasePlayer *pPlayer )
{
	pPlayer->m_StuckLast = 0;
}

int CGameMovement::CheckStuck( void )
{
	Vector base;
	Vector offset;
	Vector test;
	EntityHandle_t hitent;
	int idx;
	float fTime;
	trace_t traceresult;

	CreateStuckTable();

	hitent = TestPlayerPosition( mv->GetAbsOrigin(), COLLISION_GROUP_PLAYER_MOVEMENT, traceresult );

#ifdef CLIENT_DLL
	#define CBall C_Ball
#endif

	if ( hitent == INVALID_ENTITY_HANDLE || dynamic_cast<CBall *>(traceresult.m_pEnt) )
	{
		ResetStuckOffsets( player );
		return 0;
	}

	// Deal with stuckness...
#ifndef _LINUX
	if ( developer.GetBool() )
	{
		bool isServer = player->IsServer();
		engine->Con_NPrintf( isServer, "%s stuck on object %i/%s", 
			isServer ? "server" : "client",
			hitent.GetEntryIndex(), MoveHelper()->GetName(hitent) );
	}
#endif

	VectorCopy( mv->GetAbsOrigin(), base );

	// 
	// Deal with precision error in network.
	// 
	// World or BSP model
	if ( !player->IsServer() )
	{
		if ( MoveHelper()->IsWorldEntity( hitent ) )
		{
			int nReps = 0;
			ResetStuckOffsets( player );
			do 
			{
				GetRandomStuckOffsets( player, offset );
				VectorAdd( base, offset, test );
				
				if ( TestPlayerPosition( test, COLLISION_GROUP_PLAYER_MOVEMENT, traceresult ) == INVALID_ENTITY_HANDLE )
				{
					ResetStuckOffsets( player );
					mv->SetAbsOrigin( test );
					return 0;
				}
				nReps++;
			} while (nReps < 54);
		}
	}

	// Only an issue on the client.
	idx = player->IsServer() ? 0 : 1;

	fTime = engine->Time();
	// Too soon?
	if ( m_flStuckCheckTime[ player->entindex() ][ idx ] >=  fTime - CHECKSTUCK_MINTIME )
	{
		return 1;
	}
	m_flStuckCheckTime[ player->entindex() ][ idx ] = fTime;

	MoveHelper( )->AddToTouched( traceresult, mv->m_vecVelocity );
	GetRandomStuckOffsets( player, offset );
	VectorAdd( base, offset, test );

	if ( TestPlayerPosition( test, COLLISION_GROUP_PLAYER_MOVEMENT, traceresult ) == INVALID_ENTITY_HANDLE)
	{
		ResetStuckOffsets( player );
		mv->SetAbsOrigin( test );
		return 0;
	}

	return 1;
}

CBaseHandle CGameMovement::TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm )
{
	Ray_t ray;
	ray.Init( pos, pos, player->GetPlayerMins(), player->GetPlayerMaxs() );
	UTIL_TraceRay( ray, PlayerSolidMask(), mv->m_nPlayerHandle.Get(), collisionGroup, &pm );
	if ( (pm.contents & PlayerSolidMask()) && pm.m_pEnt )
	{
		return pm.m_pEnt->GetRefEHandle();
	}
	else
	{	
		return INVALID_EHANDLE_INDEX;
	}
}

void CGameMovement::SetGroundEntity( trace_t *pm )
{
	CBaseEntity *newGround = pm ? pm->m_pEnt : NULL;

	CBaseEntity *oldGround = player->GetGroundEntity();
	Vector vecBaseVelocity = player->GetBaseVelocity();

	if ( !oldGround && newGround )
	{
		// Subtract ground velocity at instant we hit ground jumping
		vecBaseVelocity -= newGround->GetAbsVelocity(); 
		vecBaseVelocity.z = newGround->GetAbsVelocity().z;
	}
	else if ( oldGround && !newGround )
	{
		// Add in ground velocity at instant we started jumping
 		vecBaseVelocity += oldGround->GetAbsVelocity();
		vecBaseVelocity.z = oldGround->GetAbsVelocity().z;
	}

	player->SetBaseVelocity( vecBaseVelocity );
	player->SetGroundEntity( newGround );

	// If we are on something...

	if ( newGround )
	{
		// Standing on an entity other than the world, so signal that we are touching something.
		if ( !pm->DidHitWorld() )
		{
			MoveHelper()->AddToTouched( *pm, mv->m_vecVelocity );
		}

		mv->m_vecVelocity.z = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Traces the player's collision bounds in quadrants, looking for a plane that
// can be stood upon (normal's z >= 0.7f).  Regardless of success or failure,
// replace the fraction and endpos with the original ones, so we don't try to
// move the player down to the new floor and get stuck on a leaning wall that
// the original trace hit first.
//-----------------------------------------------------------------------------
void TracePlayerBBoxForGround( const Vector& start, const Vector& end, const Vector& minsSrc,
							  const Vector& maxsSrc, IHandleEntity *player, unsigned int fMask,
							  int collisionGroup, trace_t& pm )
{
	VPROF( "TracePlayerBBoxForGround" );

	Ray_t ray;
	Vector mins, maxs;

	float fraction = pm.fraction;
	Vector endpos = pm.endpos;

	// Check the -x, -y quadrant
	mins = minsSrc;
	maxs.Init( min( 0, maxsSrc.x ), min( 0, maxsSrc.y ), maxsSrc.z );
	ray.Init( start, end, mins, maxs );
	UTIL_TraceRay( ray, fMask, player, collisionGroup, &pm );
	if ( pm.m_pEnt && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the +x, +y quadrant
	mins.Init( max( 0, minsSrc.x ), max( 0, minsSrc.y ), minsSrc.z );
	maxs = maxsSrc;
	ray.Init( start, end, mins, maxs );
	UTIL_TraceRay( ray, fMask, player, collisionGroup, &pm );
	if ( pm.m_pEnt && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the -x, +y quadrant
	mins.Init( minsSrc.x, max( 0, minsSrc.y ), minsSrc.z );
	maxs.Init( min( 0, maxsSrc.x ), maxsSrc.y, maxsSrc.z );
	ray.Init( start, end, mins, maxs );
	UTIL_TraceRay( ray, fMask, player, collisionGroup, &pm );
	if ( pm.m_pEnt && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the +x, -y quadrant
	mins.Init( max( 0, minsSrc.x ), minsSrc.y, minsSrc.z );
	maxs.Init( maxsSrc.x, min( 0, maxsSrc.y ), maxsSrc.z );
	ray.Init( start, end, mins, maxs );
	UTIL_TraceRay( ray, fMask, player, collisionGroup, &pm );
	if ( pm.m_pEnt && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	pm.fraction = fraction;
	pm.endpos = endpos;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &input - 
//-----------------------------------------------------------------------------
void CGameMovement::CategorizePosition( void )
{
	Vector point;
	trace_t pm;

	// Reset this each time we-recategorize, otherwise we have bogus friction when we jump into water and plunge downward really quickly
	player->m_surfaceFriction = 1.0f;

	// if the player hull point one unit down is solid, the player
	// is on ground
	
	// see if standing on something solid	

	// Doing this before we move may introduce a potential latency in water detection, but
	// doing it after can get us stuck on the bottom in water if the amount we move up
	// is less than the 1 pixel 'threshold' we're about to snap to.	Also, we'll call
	// this several times per frame, so we really need to avoid sticking to the bottom of
	// water on each call, and the converse case will correct itself if called twice.

	// observers don't have a ground entity
	if ( player->IsObserver() )
		return;

	float flOffset = 2.0f;

	point[0] = mv->GetAbsOrigin()[0];
	point[1] = mv->GetAbsOrigin()[1];
	point[2] = mv->GetAbsOrigin()[2] - flOffset;

	Vector bumpOrigin;
	bumpOrigin = mv->GetAbsOrigin();

	// Shooting up really fast.  Definitely not on ground.
	// On ladder moving up, so not on ground either
	// NOTE: 145 is a jump.
#define NON_JUMP_VELOCITY 140.0f

	float zvel = mv->m_vecVelocity[2];
	bool bMovingUpRapidly = zvel > NON_JUMP_VELOCITY;
	float flGroundEntityVelZ = 0.0f;
	if ( bMovingUpRapidly )
	{
		// Tracker 73219, 75878:  ywb 8/2/07
		// After save/restore (and maybe at other times), we can get a case where we were saved on a lift and 
		//  after restore we'll have a high local velocity due to the lift making our abs velocity appear high.  
		// We need to account for standing on a moving ground object in that case in order to determine if we really 
		//  are moving away from the object we are standing on at too rapid a speed.  Note that CheckJump already sets
		//  ground entity to NULL, so this wouldn't have any effect unless we are moving up rapidly not from the jump button.
		CBaseEntity *ground = player->GetGroundEntity();
		if ( ground )
		{
			flGroundEntityVelZ = ground->GetAbsVelocity().z;
			bMovingUpRapidly = ( zvel - flGroundEntityVelZ ) > NON_JUMP_VELOCITY;
		}
	}

	// Was on ground, but now suddenly am not
	if ( bMovingUpRapidly )   
	{
		SetGroundEntity( NULL );
	}
	else
	{
		// Try and move down.
		TracePlayerBBox( bumpOrigin, point, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );
		
		// Was on ground, but now suddenly am not.  If we hit a steep plane, we are not on ground
		if ( !pm.m_pEnt || pm.plane.normal[2] < 0.7 )
		{
			// Test four sub-boxes, to see if any of them would have found shallower slope we could actually stand on
			TracePlayerBBoxForGround( bumpOrigin, point, player->GetPlayerMins(), player->GetPlayerMaxs(), mv->m_nPlayerHandle.Get(), MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );
			if ( !pm.m_pEnt || pm.plane.normal[2] < 0.7 )
			{
				SetGroundEntity( NULL );
				// probably want to add a check for a +z velocity too!
				if ( ( mv->m_vecVelocity.z > 0.0f ) && 
					( player->GetMoveType() != MOVETYPE_NOCLIP ) )
				{
					player->m_surfaceFriction = 0.25f;
				}
			}
			else
			{
				SetGroundEntity( &pm );
			}
		}
		else
		{
			SetGroundEntity( &pm );  // Otherwise, point to index of ent under us.
		}
	}
}

static ConVar sv_optimizedmovement( "sv_optimizedmovement", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

#include "sdk_gamerules.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::PlayerMove( void )
{
	VPROF( "CGameMovement::PlayerMove" );

	CheckParameters();
	
	// clear output applied velocity
	mv->m_outWishVel.Init();
	mv->m_outJumpVel.Init();

	MoveHelper( )->ResetTouchList();                    // Assume we don't touch anything

	ReduceTimers();

	AngleVectors (mv->m_vecViewAngles, &m_vecForward, &m_vecRight, &m_vecUp );  // Determine movement angles

	// Always try and unstick us unless we are using a couple of the movement modes
	if ( player->GetMoveType() != MOVETYPE_NOCLIP && 
		 player->GetMoveType() != MOVETYPE_NONE && 		 
		 player->GetMoveType() != MOVETYPE_ISOMETRIC && 
		 (player->GetTeamNumber() == TEAM_HOME || player->GetTeamNumber() == TEAM_AWAY) &&
		 !(player->GetFlags() & FL_REMOTECONTROLLED))
	{
		//Vector pos = mv->GetAbsOrigin();
		//ToSDKPlayer(player)->FindSafePos(pos);
		//mv->SetAbsOrigin(pos);
		if ( CheckInterval( STUCK ) )
		{
			if ( CheckStuck() )
			{
				// Can't move, we're stuck
				return;  
			}
		}
	}

	// Now that we are "unstuck", see where we are (player->GetWaterLevel() and type, player->GetGroundEntity()).
	if ( player->GetMoveType() != MOVETYPE_WALK ||
		mv->m_bGameCodeMovedPlayer || 
		!sv_optimizedmovement.GetBool()  )
	{
		CategorizePosition();
	}
	else
	{
		if ( mv->m_vecVelocity.z > 250.0f )
		{
			SetGroundEntity( NULL );
		}
	}

	// If we are not on ground, store off how fast we are moving down
	if ( player->GetGroundEntity() == NULL )
	{
		player->m_Local.m_flFallVelocity = -mv->m_vecVelocity[ 2 ];
	}

	player->UpdateStepSound( player->m_pSurfaceData, mv->GetAbsOrigin(), mv->m_vecVelocity );

	// Handle movement modes.
	switch (player->GetMoveType())
	{
		case MOVETYPE_NONE:
		case MOVETYPE_FLY:
		case MOVETYPE_FLYGRAVITY:
		case MOVETYPE_ISOMETRIC:
			break;

		case MOVETYPE_NOCLIP:
			//if (player->GetFlags() & FL_REMOTECONTROLLED)
			//	MoveToTargetPos();
			//else
				FullNoClipMove( sv_noclipspeed.GetFloat(), sv_noclipaccelerate.GetFloat() );
			break;

		case MOVETYPE_WALK:
			if (player->GetFlags() & FL_REMOTECONTROLLED)
				MoveToTargetPos();
			else
				FullWalkMove();
			break;

		default:
			DevMsg( 1, "Bogus pmove player movetype %i on (%i) 0=cl 1=sv\n", player->GetMoveType(), player->IsServer());
			break;
	}
}

//-----------------------------------------------------------------------------
// Performs the collision resolution for fliers.
//-----------------------------------------------------------------------------
void CGameMovement::PerformFlyCollisionResolution( trace_t &pm, Vector &move )
{
	Vector base;
	float vel;
	float backoff;

	switch (player->GetMoveCollide())
	{
	case MOVECOLLIDE_FLY_CUSTOM:
		// Do nothing; the velocity should have been modified by touch
		// FIXME: It seems wrong for touch to modify velocity
		// given that it can be called in a number of places
		// where collision resolution do *not* in fact occur

		// Should this ever occur for players!?
		Assert(0);
		break;

	case MOVECOLLIDE_FLY_BOUNCE:	
	case MOVECOLLIDE_DEFAULT:
		{
			if (player->GetMoveCollide() == MOVECOLLIDE_FLY_BOUNCE)
				backoff = 2.0 - player->m_surfaceFriction;
			else
				backoff = 1;

			ClipVelocity (mv->m_vecVelocity, pm.plane.normal, mv->m_vecVelocity, backoff);
		}
		break;

	default:
		// Invalid collide type!
		Assert(0);
		break;
	}

	// stop if on ground
	if (pm.plane.normal[2] > 0.7)
	{		
		base.Init();
		if (mv->m_vecVelocity[2] < sv_gravity.GetFloat() * gpGlobals->frametime)
		{
			// we're rolling on the ground, add static friction.
			SetGroundEntity( &pm ); 
			mv->m_vecVelocity[2] = 0;
		}

		vel = DotProduct( mv->m_vecVelocity, mv->m_vecVelocity );

		// Con_DPrintf("%f %f: %.0f %.0f %.0f\n", vel, trace.fraction, ent->velocity[0], ent->velocity[1], ent->velocity[2] );

		if (vel < (30 * 30) || (player->GetMoveCollide() != MOVECOLLIDE_FLY_BOUNCE))
		{
			SetGroundEntity( &pm ); 
			mv->m_vecVelocity.Init();
		}
		else
		{
			VectorScale (mv->m_vecVelocity, (1.0 - pm.fraction) * gpGlobals->frametime * 0.9, move);
			PushEntity( move, &pm );
		}
		VectorSubtract( mv->m_vecVelocity, base, mv->m_vecVelocity );
	}
}

void CGameMovement::SetPlayerSpeed()
{
	CSDKPlayer *pPl = (CSDKPlayer *)player;

	float flMaxSpeed;

	if (pPl->GetFlags() & FL_REMOTECONTROLLED)
	{
		flMaxSpeed = mp_remotecontrolledspeed.GetInt();
	}
	else if (SDKGameRules()->IsCeremony())
	{
		flMaxSpeed = mp_ceremonyspeed.GetInt();
	}
	else if (mv->m_nButtons & IN_WALK)
	{
		flMaxSpeed = mp_walkspeed.GetInt();
	}
	else if (pPl->DoSkillMove())
	{
		flMaxSpeed = mp_skillspeed.GetInt();
	}
	else
	{
		if ((mv->m_nButtons & IN_SPEED) && pPl->m_Shared.GetStamina() > 0 && (mv->m_nButtons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)))
			flMaxSpeed = mp_sprintspeed.GetInt();
		else
			flMaxSpeed = mp_runspeed.GetInt();
	}

	mv->m_flClientMaxSpeed = flMaxSpeed;
}