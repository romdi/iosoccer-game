//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_playerresource.h"
#include "c_team.h"
#include "gamestringpool.h"

#ifdef HL2MP
#include "hl2mp_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const float PLAYER_RESOURCE_THINK_INTERVAL = 0.2f;
#define PLAYER_UNCONNECTED_NAME	"unconnected"

IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_PlayerResource, DT_PlayerResource, CPlayerResource)
	RecvPropArray3( RECVINFO_ARRAY(m_iPing), RecvPropInt( RECVINFO(m_iPing[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iScore), RecvPropInt( RECVINFO(m_iScore[0]))),
//	RecvPropArray3( RECVINFO_ARRAY(m_iDeaths), RecvPropInt( RECVINFO(m_iDeaths[0]))),		//iosremoved to save bandwidth
	RecvPropArray3( RECVINFO_ARRAY(m_bConnected), RecvPropInt( RECVINFO(m_bConnected[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iTeam), RecvPropInt( RECVINFO(m_iTeam[0]))),
//	RecvPropArray3( RECVINFO_ARRAY(m_bAlive), RecvPropInt( RECVINFO(m_bAlive[0]))),	//iosremoved to save bandwidth
//	RecvPropArray3( RECVINFO_ARRAY(m_iHealth), RecvPropInt( RECVINFO(m_iHealth[0]))),	//iosremoved to save bandwidth

	//ios
	RecvPropArray3( RECVINFO_ARRAY(m_RedCard), RecvPropInt( RECVINFO(m_RedCard[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_YellowCard), RecvPropInt( RECVINFO(m_YellowCard[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Fouls), RecvPropInt( RECVINFO(m_Fouls[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Assists), RecvPropInt( RECVINFO(m_Assists[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Possession), RecvPropInt( RECVINFO(m_Possession[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Passes), RecvPropInt( RECVINFO(m_Passes[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_FreeKicks), RecvPropInt( RECVINFO(m_FreeKicks[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Penalties), RecvPropInt( RECVINFO(m_Penalties[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Corners), RecvPropInt( RECVINFO(m_Corners[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_ThrowIns), RecvPropInt( RECVINFO(m_ThrowIns[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_KeeperSaves), RecvPropInt( RECVINFO(m_KeeperSaves[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_GoalKicks), RecvPropInt( RECVINFO(m_GoalKicks[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Position), RecvPropInt( RECVINFO(m_Position[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Sprint), RecvPropInt( RECVINFO(m_Sprint[0]))),


END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_PlayerResource )

	DEFINE_PRED_ARRAY( m_szName, FIELD_STRING, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iPing, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iScore, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iDeaths, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_bConnected, FIELD_BOOLEAN, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iTeam, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_bAlive, FIELD_BOOLEAN, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iHealth, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),

END_PREDICTION_DATA()	

C_PlayerResource *g_PR;

IGameResources * GameResources( void ) { return g_PR; }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::C_PlayerResource()
{
	memset( m_iPing, 0, sizeof( m_iPing ) );
//	memset( m_iPacketloss, 0, sizeof( m_iPacketloss ) );
	memset( m_iScore, 0, sizeof( m_iScore ) );
	memset( m_iDeaths, 0, sizeof( m_iDeaths ) );
	memset( m_bConnected, 0, sizeof( m_bConnected ) );
	memset( m_iTeam, 0, sizeof( m_iTeam ) );
	memset( m_bAlive, 0, sizeof( m_bAlive ) );
	memset( m_iHealth, 0, sizeof( m_iHealth ) );

	//ios
	memset( m_RedCard, 0, sizeof( m_RedCard ) );
	memset( m_YellowCard, 0, sizeof( m_YellowCard ) );
	memset( m_Fouls, 0, sizeof( m_Fouls ) );
	memset( m_Assists, 0, sizeof( m_Assists ) );
	memset( m_Possession, 0, sizeof( m_Possession ) );
	memset( m_Passes, 0, sizeof( m_Passes ) );
	memset( m_FreeKicks, 0, sizeof( m_FreeKicks ) );
	memset( m_Penalties, 0, sizeof( m_Penalties ) );
	memset( m_Corners, 0, sizeof( m_Corners ) );
	memset( m_ThrowIns, 0, sizeof( m_ThrowIns ) );
	memset( m_KeeperSaves, 0, sizeof( m_KeeperSaves ) );
	memset( m_GoalKicks, 0, sizeof( m_GoalKicks ) );
	memset( m_Position, 0, sizeof( m_Position ) );
	memset( m_Sprint, 0, sizeof( m_Sprint ) );

	for ( int i=0; i<MAX_TEAMS; i++ )
	{
		m_Colors[i] = COLOR_WHITE;	//ios: was grey
	}

#ifdef HL2MP
	m_Colors[TEAM_A] = COLOR_WHITE;	//ioscols scoreboard
	m_Colors[TEAM_B] = COLOR_WHITE;
	m_Colors[TEAM_UNASSIGNED] = COLOR_YELLOW;
#endif

	g_PR = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::~C_PlayerResource()
{
	g_PR = NULL;
}

void C_PlayerResource::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( gpGlobals->curtime + PLAYER_RESOURCE_THINK_INTERVAL );
	}
}

void C_PlayerResource::UpdatePlayerName( int slot )
{
	if ( slot < 1 || slot > MAX_PLAYERS )
	{
		Error( "UpdatePlayerName with bogus slot %d\n", slot );
		return;
	}
	player_info_t sPlayerInfo;
	if ( IsConnected( slot ) && engine->GetPlayerInfo( slot, &sPlayerInfo ) )
	{
		m_szName[slot] = AllocPooledString( sPlayerInfo.name );
	}
	else
	{
		m_szName[slot] = AllocPooledString( PLAYER_UNCONNECTED_NAME );
	}
}

void C_PlayerResource::ClientThink()
{
	BaseClass::ClientThink();

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		UpdatePlayerName( i );
	}

	SetNextClientThink( gpGlobals->curtime + PLAYER_RESOURCE_THINK_INTERVAL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_PlayerResource::GetPlayerName( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return "ERRORNAME";
	}
	
	if ( !IsConnected( iIndex ) )
		return PLAYER_UNCONNECTED_NAME;

	// X360TBD: Network - figure out why the name isn't set
	if ( !m_szName[ iIndex ] || !Q_stricmp( m_szName[ iIndex ], PLAYER_UNCONNECTED_NAME ) )
	{
		// If you get a full "reset" uncompressed update from server, then you can have NULLNAME show up in the scoreboard
		UpdatePlayerName( iIndex );
	}

	// This gets updated in ClientThink, so it could be up to 1 second out of date, oh well.
	return m_szName[iIndex];
}

bool C_PlayerResource::IsAlive(int iIndex )
{
	return m_bAlive[iIndex];
}

int C_PlayerResource::GetTeam(int iIndex )
{
	// ios if ( iIndex < 1 || iIndex > MAX_PLAYERS )
	if ( iIndex < 0 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return 0;
	}
	else
	{
		return m_iTeam[iIndex];
	}
}

const char * C_PlayerResource::GetTeamName(int index)
{
	C_Team *team = GetGlobalTeam( index );

	if ( !team )
		return "Unknown";

	return team->Get_Name();
}

int C_PlayerResource::GetTeamScore(int index)
{
	C_Team *team = GetGlobalTeam( index );

	if ( !team )
		return 0;

	return team->Get_Score();
}

const char *C_PlayerResource::GetScoreTag(int index)
{
	C_Team *team = GetGlobalTeam( index );

	if ( !team )
		return 0;

	return team->GetScoreTag();
}


int C_PlayerResource::GetFrags(int index )
{
	//IOS Scoreboard
    if ( !IsConnected( index ) )
        return 0;

    return m_iScore[index];

	//return 666;
}

bool C_PlayerResource::IsLocalPlayer(int index)
{
	C_BasePlayer *pPlayer =	C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return false;

	return ( index == pPlayer->entindex() );
}


bool C_PlayerResource::IsHLTV(int index)
{
	if ( !IsConnected( index ) )
		return false;

	player_info_t sPlayerInfo;
	
	if ( engine->GetPlayerInfo( index, &sPlayerInfo ) )
	{
		return sPlayerInfo.ishltv;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerResource::IsFakePlayer( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return false;

	// Yuck, make sure it's up to date
	player_info_t sPlayerInfo;
	if ( engine->GetPlayerInfo( iIndex, &sPlayerInfo ) )
	{
		return sPlayerInfo.fakeplayer;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetPing( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iPing[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
/*-----------------------------------------------------------------------------
int	C_PlayerResource::GetPacketloss( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iPacketloss[iIndex];
}*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetPlayerScore( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iScore[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetDeaths( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iDeaths[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetHealth( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iHealth[iIndex];
}

const Color &C_PlayerResource::GetTeamColor(int index )
{
	if ( index < 0 || index >= MAX_TEAMS )
	{
		Assert( false );
		static Color blah;
		return blah;
	}
	else
	{
		return m_Colors[index];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerResource::IsConnected( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
		return false;
	else
		return m_bConnected[iIndex];
}

//ios
int	C_PlayerResource::GetRedCards( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_RedCard[iIndex];
}
int	C_PlayerResource::GetYellowCards( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_YellowCard[iIndex];
}
int	C_PlayerResource::GetFouls( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Fouls[iIndex];
}
int	C_PlayerResource::GetAssists( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Assists[iIndex];
}
int	C_PlayerResource::GetPossession( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Possession[iIndex];
}
int	C_PlayerResource::GetPasses( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Passes[iIndex];
}
int	C_PlayerResource::GetFreeKicks( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_FreeKicks[iIndex];
}
int	C_PlayerResource::GetPenalties( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Penalties[iIndex];
}
int	C_PlayerResource::GetCorners( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Corners[iIndex];
}
int	C_PlayerResource::GetThrowIns( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_ThrowIns[iIndex];
}
int	C_PlayerResource::GetKeeperSaves( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_KeeperSaves[iIndex];
}
int	C_PlayerResource::GetGoalKicks( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_GoalKicks[iIndex];
}
int	C_PlayerResource::GetPosition( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Position[iIndex];
}
int	C_PlayerResource::GetSprint( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Sprint[iIndex];
}
