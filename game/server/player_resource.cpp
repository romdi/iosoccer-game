//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "player.h"
#include "player_resource.h"
#include "sdk_player.h"				//ios
#include <coordsize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void SendProxy_String_tToStringPR( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	string_t *pString = (string_t*)pData;
	pOut->m_pString = (char*)STRING( *pString );
}

// Datatable
IMPLEMENT_SERVERCLASS_ST_NOBASE(CPlayerResource, DT_PlayerResource)
//	SendPropArray( SendPropString( SENDINFO(m_szName[0]) ), SENDARRAYINFO(m_szName) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iPing), SendPropInt( SENDINFO_ARRAY(m_iPing), 10, SPROP_UNSIGNED ) ),
//	SendPropArray( SendPropInt( SENDINFO_ARRAY(m_iPacketloss), 7, SPROP_UNSIGNED ), m_iPacketloss ),
	SendPropArray3( SENDINFO_ARRAY3(m_nGoals), SendPropInt( SENDINFO_ARRAY(m_nGoals), 12 ) ),
//	SendPropArray3( SENDINFO_ARRAY3(m_iDeaths), SendPropInt( SENDINFO_ARRAY(m_iDeaths), 12 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bConnected), SendPropInt( SENDINFO_ARRAY(m_bConnected), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iTeam), SendPropInt( SENDINFO_ARRAY(m_iTeam), 4 ) ),
//	SendPropArray3( SENDINFO_ARRAY3(m_bAlive), SendPropInt( SENDINFO_ARRAY(m_bAlive), 1, SPROP_UNSIGNED ) ),
//	SendPropArray3( SENDINFO_ARRAY3(m_iHealth), SendPropInt( SENDINFO_ARRAY(m_iHealth), 10, SPROP_UNSIGNED ) ),
	
	//ios
	SendPropArray3( SENDINFO_ARRAY3(m_RedCard), SendPropInt( SENDINFO_ARRAY(m_RedCard), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_YellowCard), SendPropInt( SENDINFO_ARRAY(m_YellowCard), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Fouls), SendPropInt( SENDINFO_ARRAY(m_Fouls), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Offsides), SendPropInt( SENDINFO_ARRAY(m_Offsides), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Goals), SendPropInt( SENDINFO_ARRAY(m_Goals), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Assists), SendPropInt( SENDINFO_ARRAY(m_Assists), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Possession), SendPropInt( SENDINFO_ARRAY(m_Possession), 7, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_DistanceCovered), SendPropInt( SENDINFO_ARRAY(m_DistanceCovered), -1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Passes), SendPropInt( SENDINFO_ARRAY(m_Passes), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_FreeKicks), SendPropInt( SENDINFO_ARRAY(m_FreeKicks), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Penalties), SendPropInt( SENDINFO_ARRAY(m_Penalties), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Corners), SendPropInt( SENDINFO_ARRAY(m_Corners), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_ThrowIns), SendPropInt( SENDINFO_ARRAY(m_ThrowIns), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_KeeperSaves), SendPropInt( SENDINFO_ARRAY(m_KeeperSaves), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_GoalKicks), SendPropInt( SENDINFO_ARRAY(m_GoalKicks), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_TeamPosition), SendPropInt( SENDINFO_ARRAY(m_TeamPosition), 5 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_NextJoin), SendPropFloat( SENDINFO_ARRAY(m_NextJoin), -1, SPROP_NOSCALE ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_TeamToJoin), SendPropInt( SENDINFO_ARRAY(m_TeamToJoin), 5 ) ),

	SendPropArray3( SENDINFO_ARRAY3(m_szClubNames), SendPropString( SENDINFO_ARRAY(m_szClubNames), 0, SendProxy_String_tToStringPR ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_szCountryNames), SendPropString( SENDINFO_ARRAY(m_szCountryNames), 0, SendProxy_String_tToStringPR ) ),
	//SendPropArray( SendPropString( SENDINFO_ARRAY( m_szClubName ), 0, SendProxy_String_tToString ), m_szClubName ),
	
END_SEND_TABLE()

BEGIN_DATADESC( CPlayerResource )

	// DEFINE_ARRAY( m_iPing, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_ARRAY( m_iPacketloss, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_ARRAY( m_nGoals, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_ARRAY( m_iDeaths, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_ARRAY( m_bConnected, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_FIELD( m_flNextPingUpdate, FIELD_FLOAT ),
	// DEFINE_ARRAY( m_iTeam, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_ARRAY( m_bAlive, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_ARRAY( m_iHealth, FIELD_INTEGER, MAX_PLAYERS+1 ),
	// DEFINE_FIELD( m_nUpdateCounter, FIELD_INTEGER ),

	// Function Pointers
	DEFINE_FUNCTION( ResourceThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( player_manager, CPlayerResource );

CPlayerResource *g_pPlayerResource;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerResource::Spawn( void )
{
	for ( int i=0; i < MAX_PLAYERS+1; i++ )
	{
		m_iPing.Set( i, 0 );
		m_nGoals.Set( i, 0 );
//		m_iDeaths.Set( i, 0 );
		m_bConnected.Set( i, 0 );
		m_iTeam.Set( i, 0 );
//		m_bAlive.Set( i, 0 );

		//ios
		m_RedCard.Set( i, 0 );
		m_YellowCard.Set( i, 0 );
		m_Fouls.Set( i, 0 );
		m_Offsides.Set( i, 0 );
		m_Goals.Set( i, 0 );
		m_Assists.Set( i, 0 );
		m_Possession.Set( i, 0 );
		m_DistanceCovered.Set( i, 0 );
		m_Passes.Set( i, 0 );
		m_FreeKicks.Set( i, 0 );
		m_Penalties.Set( i, 0 );
		m_Corners.Set( i, 0 );
		m_ThrowIns.Set( i, 0 );
		m_KeeperSaves.Set( i, 0 );
		m_GoalKicks.Set( i, 0 );
		m_TeamPosition.Set( i, 0 );
		m_NextJoin.Set( i, 0 );
		m_TeamToJoin.Set( i, 0 );

		m_szClubNames.Set( i, MAKE_STRING("") );
		m_szCountryNames.Set( i, MAKE_STRING("") );
	}

	SetThink( &CPlayerResource::ResourceThink );
	SetNextThink( gpGlobals->curtime );
	m_nUpdateCounter = 0;
}

//-----------------------------------------------------------------------------
// Purpose: The Player resource is always transmitted to clients
//-----------------------------------------------------------------------------
int CPlayerResource::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper for the virtual GrabPlayerData Think function
//-----------------------------------------------------------------------------
void CPlayerResource::ResourceThink( void )
{
	m_nUpdateCounter++;

	UpdatePlayerData();

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerResource::UpdatePlayerData( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		
		if ( pPlayer && pPlayer->IsConnected() )
		{
			m_nGoals.Set( i, pPlayer->FragCount() );
//			m_iDeaths.Set( i, pPlayer->DeathCount() );
			m_bConnected.Set( i, 1 );
			m_iTeam.Set( i, pPlayer->GetTeamNumber() );
//			m_bAlive.Set( i, pPlayer->IsAlive()?1:0 );
//			m_iHealth.Set(i, max( 0, pPlayer->GetHealth() ) );

			CSDKPlayer	*SDKPlayer = ToSDKPlayer(pPlayer);
			if (SDKPlayer)
			{
				m_RedCard.Set(i, max( 0, SDKPlayer->GetRedCards() ) );
				m_YellowCard.Set(i, max( 0, SDKPlayer->GetYellowCards() ) );
				m_Fouls.Set(i, max( 0, SDKPlayer->GetFouls() ) );
				m_Offsides.Set(i, max( 0, SDKPlayer->GetOffsides() ) );
				m_Goals.Set(i, max( 0, SDKPlayer->GetGoals() ) );
				m_Assists.Set(i, max( 0, SDKPlayer->GetAssists() ) );
				m_Possession.Set(i, max( 0, SDKPlayer->GetPossession() ) );
				m_DistanceCovered.Set(i, max( 0, SDKPlayer->GetDistanceCovered() ) );
				m_Passes.Set(i, max( 0, SDKPlayer->GetPasses() ) );
				m_FreeKicks.Set(i, max( 0, SDKPlayer->GetFreeKicks() ) );
				m_Penalties.Set(i, max( 0, SDKPlayer->GetPenalties() ) );
				m_Corners.Set(i, max( 0, SDKPlayer->GetCorners() ) );
				m_ThrowIns.Set(i, max( 0, SDKPlayer->GetThrowIns() ) );
				m_KeeperSaves.Set(i, max( 0, SDKPlayer->GetKeeperSaves() ) );
				m_GoalKicks.Set(i, max( 0, SDKPlayer->GetGoalKicks() ) );
				m_TeamPosition.Set(i, SDKPlayer->GetTeamPosIndex() );
				m_TeamToJoin.Set(i, SDKPlayer->GetTeamToJoin() );
				m_NextJoin.Set(i, SDKPlayer->GetNextJoin() );

				m_szClubNames.Set(i, MAKE_STRING(SDKPlayer->GetClubName()));

				// RomD: Enforce client update, since the value changes without notice
				if (SDKPlayer->m_bClubNameChanged)
				{
					m_szClubNames.GetForModify(i);
					SDKPlayer->m_bClubNameChanged = false;
				}

				m_szCountryNames.Set(i, MAKE_STRING(SDKPlayer->GetCountryName()));

				// RomD: Enforce client update, since the value changes without notice
				if (SDKPlayer->m_bCountryNameChanged)
				{
					m_szCountryNames.GetForModify(i);
					SDKPlayer->m_bCountryNameChanged = false;
				}
			}

			// Don't update ping / packetloss everytime

			if ( !(m_nUpdateCounter%20) )
			{
				// update ping all 20 think ticks = (20*0.1=2seconds)
				int ping, packetloss;
				UTIL_GetPlayerConnectionInfo( i, ping, packetloss );
				
				// calc avg for scoreboard so it's not so jittery
				ping = 0.8f * m_iPing.Get(i) + 0.2f * ping;

				
				m_iPing.Set( i, ping );
				// m_iPacketloss.Set( i, packetloss );
			}
		}
		else
		{
			m_bConnected.Set( i, 0 );
		}
	}
}
