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
#include "sdk_gamerules.h"
#include "hud_basechat.h"
#include "steam/steam_api.h"

#ifdef HL2MP
#include "hl2mp_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const float PLAYER_RESOURCE_THINK_INTERVAL = 0.2f;
#define PLAYER_UNCONNECTED_NAME	"unconnected"

IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_PlayerResource, DT_PlayerResource, CPlayerResource)
	RecvPropArray3( RECVINFO_ARRAY(m_iPing), RecvPropInt( RECVINFO(m_iPing[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_bConnected), RecvPropInt( RECVINFO(m_bConnected[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iTeam), RecvPropInt( RECVINFO(m_iTeam[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_nSpecTeam), RecvPropInt( RECVINFO(m_nSpecTeam[0]))),

	//ios
	RecvPropArray3( RECVINFO_ARRAY(m_RedCards), RecvPropInt( RECVINFO(m_RedCards[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_YellowCards), RecvPropInt( RECVINFO(m_YellowCards[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Fouls), RecvPropInt( RECVINFO(m_Fouls[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_FoulsSuffered), RecvPropInt( RECVINFO(m_FoulsSuffered[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_SlidingTackles), RecvPropInt( RECVINFO(m_SlidingTackles[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_SlidingTacklesCompleted), RecvPropInt( RECVINFO(m_SlidingTacklesCompleted[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_GoalsConceded), RecvPropInt( RECVINFO(m_GoalsConceded[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Shots), RecvPropInt( RECVINFO(m_Shots[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_ShotsOnGoal), RecvPropInt( RECVINFO(m_ShotsOnGoal[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_PassesCompleted), RecvPropInt( RECVINFO(m_PassesCompleted[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Interceptions), RecvPropInt( RECVINFO(m_Interceptions[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Offsides), RecvPropInt( RECVINFO(m_Offsides[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Goals), RecvPropInt( RECVINFO(m_Goals[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_OwnGoals), RecvPropInt( RECVINFO(m_OwnGoals[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Assists), RecvPropInt( RECVINFO(m_Assists[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Possession), RecvPropInt( RECVINFO(m_Possession[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_DistanceCovered), RecvPropInt( RECVINFO(m_DistanceCovered[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Passes), RecvPropInt( RECVINFO(m_Passes[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_FreeKicks), RecvPropInt( RECVINFO(m_FreeKicks[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Penalties), RecvPropInt( RECVINFO(m_Penalties[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Corners), RecvPropInt( RECVINFO(m_Corners[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_ThrowIns), RecvPropInt( RECVINFO(m_ThrowIns[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_KeeperSaves), RecvPropInt( RECVINFO(m_KeeperSaves[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_GoalKicks), RecvPropInt( RECVINFO(m_GoalKicks[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_Ratings), RecvPropInt( RECVINFO(m_Ratings[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_TeamPosIndex), RecvPropInt( RECVINFO(m_TeamPosIndex[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_ShirtNumber), RecvPropInt( RECVINFO(m_ShirtNumber[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_TeamToJoin), RecvPropInt( RECVINFO(m_TeamToJoin[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_TeamPosIndexToJoin), RecvPropInt( RECVINFO(m_TeamPosIndexToJoin[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_NextCardJoin), RecvPropInt( RECVINFO(m_NextCardJoin[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_IsAway), RecvPropBool( RECVINFO(m_IsAway[0]))),

	RecvPropArray3( RECVINFO_ARRAY(m_szPlayerNames), RecvPropString( RECVINFO(m_szPlayerNames[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_szClubNames), RecvPropString( RECVINFO(m_szClubNames[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_szShirtNames), RecvPropString( RECVINFO(m_szShirtNames[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_CountryIndices), RecvPropInt( RECVINFO(m_CountryIndices[0]))),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_PlayerResource )

	DEFINE_PRED_ARRAY( m_szName, FIELD_STRING, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iPing, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_bConnected, FIELD_BOOLEAN, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iTeam, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_nSpecTeam, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),

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
	memset( m_bConnected, 0, sizeof( m_bConnected ) );
	memset( m_iTeam, 0, sizeof( m_iTeam ) );
	memset( m_nSpecTeam, 0, sizeof( m_nSpecTeam ) );

	//ios
	memset( m_RedCards, 0, sizeof( m_RedCards ) );
	memset( m_YellowCards, 0, sizeof( m_YellowCards ) );
	memset( m_Fouls, 0, sizeof( m_Fouls ) );
	memset( m_FoulsSuffered, 0, sizeof( m_FoulsSuffered ) );
	memset( m_GoalsConceded, 0, sizeof( m_GoalsConceded ) );
	memset( m_Shots, 0, sizeof( m_Shots ) );
	memset( m_ShotsOnGoal, 0, sizeof( m_ShotsOnGoal ) );
	memset( m_PassesCompleted, 0, sizeof( m_PassesCompleted ) );
	memset( m_SlidingTackles, 0, sizeof( m_SlidingTackles ) );
	memset( m_SlidingTacklesCompleted, 0, sizeof( m_SlidingTacklesCompleted ) );
	memset( m_Interceptions, 0, sizeof( m_Interceptions ) );
	memset( m_Offsides, 0, sizeof( m_Offsides ) );
	memset( m_Goals, 0, sizeof( m_Goals ) );
	memset( m_Assists, 0, sizeof( m_Assists ) );
	memset( m_Possession, 0, sizeof( m_Possession ) );
	memset( m_DistanceCovered, 0, sizeof( m_DistanceCovered ) );
	memset( m_Passes, 0, sizeof( m_Passes ) );
	memset( m_FreeKicks, 0, sizeof( m_FreeKicks ) );
	memset( m_Penalties, 0, sizeof( m_Penalties ) );
	memset( m_Corners, 0, sizeof( m_Corners ) );
	memset( m_ThrowIns, 0, sizeof( m_ThrowIns ) );
	memset( m_KeeperSaves, 0, sizeof( m_KeeperSaves ) );
	memset( m_GoalKicks, 0, sizeof( m_GoalKicks ) );
	memset( m_Ratings, 0, sizeof( m_Ratings ) );
	memset( m_TeamPosIndex, 0, sizeof( m_TeamPosIndex ) );
	memset( m_ShirtNumber, 0, sizeof( m_ShirtNumber ) );
	memset( m_TeamToJoin, 0, sizeof( m_TeamToJoin ) );
	memset( m_TeamPosIndexToJoin, 0, sizeof( m_TeamPosIndexToJoin ) );
	memset( m_NextCardJoin, 0, sizeof( m_NextCardJoin ) );
	memset( m_IsAway, 0, sizeof( m_IsAway ) );

	memset( m_szPlayerNames, 0, sizeof( m_szPlayerNames ) );
	memset( m_szClubNames, 0, sizeof( m_szClubNames ) );
	memset( m_szShirtNames, 0, sizeof( m_szShirtNames ) );
	memset( m_CountryIndices, 0, sizeof( m_CountryIndices ) );

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

	return m_szPlayerNames[iIndex];
}

const char *C_PlayerResource::GetSteamName( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return "ERRORNAME";
	}

	if ( !IsConnected( iIndex ) )
		return PLAYER_UNCONNECTED_NAME;

	if (steamapicontext->SteamFriends() && steamapicontext->SteamUtils())
	{
		player_info_t pi;
		if (engine->GetPlayerInfo(iIndex, &pi))
		{
			if (pi.friendsID)
			{
				CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
				return steamapicontext->SteamFriends()->GetFriendPersonaName(steamIDForPlayer);
			}
		}
	}

	return "";
}

const char *C_PlayerResource::GetClubName( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return "ERRORNAME";
	}
	
	if ( !IsConnected( iIndex ) )
		return PLAYER_UNCONNECTED_NAME;

	return m_szClubNames[iIndex];
}

const char *C_PlayerResource::GetShirtName( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return "ERRORNAME";
	}
	
	if ( !IsConnected( iIndex ) )
		return PLAYER_UNCONNECTED_NAME;

	return m_szShirtNames[iIndex];
}

int C_PlayerResource::GetCountryIndex( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return 0;
	}
	
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_CountryIndices[iIndex];
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

int C_PlayerResource::GetSpecTeam(int iIndex )
{
	if ( iIndex < 0 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return 0;
	}
	else
	{
		return m_nSpecTeam[iIndex];
	}
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

const Color &C_PlayerResource::GetPlayerColor(int index )
{
	if ( !IsConnected( index ) )
		return g_ColorWhite;

	if (SDKGameRules()->GetMatchDisplayTimeSeconds(true, false) < GetNextCardJoin(index))
		return g_ColorRed;

	if (GetYellowCards(index) % 2 == 1)
		return g_ColorYellow;

	if (GetTeam(index) == TEAM_SPECTATOR)
		return GetGlobalTeam(GetSpecTeam(index))->GetHudKitColor();

	return GetGlobalTeam(GetTeam(index))->GetHudKitColor();
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

	return m_RedCards[iIndex];
}
int	C_PlayerResource::GetYellowCards( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_YellowCards[iIndex];
}
int	C_PlayerResource::GetFouls( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Fouls[iIndex];
}
int	C_PlayerResource::GetFoulsSuffered( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_FoulsSuffered[iIndex];
}
int	C_PlayerResource::GetSlidingTackles( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_SlidingTackles[iIndex];
}
int	C_PlayerResource::GetSlidingTacklesCompleted( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_SlidingTacklesCompleted[iIndex];
}
int	C_PlayerResource::GetGoalsConceded( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_GoalsConceded[iIndex];
}
int	C_PlayerResource::GetShots( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Shots[iIndex];
}
int	C_PlayerResource::GetShotsOnGoal( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_ShotsOnGoal[iIndex];
}
int	C_PlayerResource::GetPassesCompleted( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_PassesCompleted[iIndex];
}
int	C_PlayerResource::GetInterceptions( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Interceptions[iIndex];
}
int	C_PlayerResource::GetOffsides( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Offsides[iIndex];
}
int	C_PlayerResource::GetGoals( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Goals[iIndex];
}
int	C_PlayerResource::GetOwnGoals( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_OwnGoals[iIndex];
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
int	C_PlayerResource::GetDistanceCovered( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_DistanceCovered[iIndex];
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

int	C_PlayerResource::GetRatings( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_Ratings[iIndex];
}

int	C_PlayerResource::GetShirtNumber( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_ShirtNumber[iIndex];
}

int	C_PlayerResource::GetTeamPosType( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	//Assert(m_TeamPosIndex[iIndex] < GetGlobalTeam(GetTeam(iIndex))->GetFormation()->positions.Count());

	// FIXME: mp_maxplayers seems to get synced before team pos indices do.
	// So if we reduce mp_maxplayers, we might end up with team pos indices which are out of bounds and crash the client.
	// This ugly workaround just returns the GK pos when the index it too high until the new value is synced.
	if (m_TeamPosIndex[iIndex] >= GetGlobalTeam(GetTeam(iIndex))->GetFormation()->positions.Count())
	{
		return POS_GK;
	}

	return (int)GetGlobalTeam(GetTeam(iIndex))->GetFormation()->positions[m_TeamPosIndex[iIndex]]->type;
}

int	C_PlayerResource::GetTeamPosIndex( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_TeamPosIndex[iIndex];
}

int C_PlayerResource::GetTeamToJoin( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
		return false;
	else
		return m_TeamToJoin[iIndex];
}

int C_PlayerResource::GetTeamPosIndexToJoin( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
		return false;
	else
		return m_TeamPosIndexToJoin[iIndex];
}

int C_PlayerResource::GetNextCardJoin( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
		return false;
	else
		return m_NextCardJoin[iIndex];
}

bool C_PlayerResource::IsAway( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
		return false;
	else
		return m_IsAway[iIndex];
}