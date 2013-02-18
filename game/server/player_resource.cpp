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
#include "team.h"
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
	SendPropArray3( SENDINFO_ARRAY3(m_bConnected), SendPropInt( SENDINFO_ARRAY(m_bConnected), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iTeam), SendPropInt( SENDINFO_ARRAY(m_iTeam), 3 ) ),
	
	//ios
	SendPropArray3( SENDINFO_ARRAY3(m_RedCards), SendPropInt( SENDINFO_ARRAY(m_RedCards), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_YellowCards), SendPropInt( SENDINFO_ARRAY(m_YellowCards), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Fouls), SendPropInt( SENDINFO_ARRAY(m_Fouls), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_FoulsSuffered), SendPropInt( SENDINFO_ARRAY(m_FoulsSuffered), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_SlidingTackles), SendPropInt( SENDINFO_ARRAY(m_SlidingTackles), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_SlidingTacklesCompleted), SendPropInt( SENDINFO_ARRAY(m_SlidingTacklesCompleted), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_GoalsConceded), SendPropInt( SENDINFO_ARRAY(m_GoalsConceded), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Shots), SendPropInt( SENDINFO_ARRAY(m_Shots), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_ShotsOnGoal), SendPropInt( SENDINFO_ARRAY(m_ShotsOnGoal), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_PassesCompleted), SendPropInt( SENDINFO_ARRAY(m_PassesCompleted), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Interceptions), SendPropInt( SENDINFO_ARRAY(m_Interceptions), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Offsides), SendPropInt( SENDINFO_ARRAY(m_Offsides), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Goals), SendPropInt( SENDINFO_ARRAY(m_Goals), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_OwnGoals), SendPropInt( SENDINFO_ARRAY(m_OwnGoals), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Assists), SendPropInt( SENDINFO_ARRAY(m_Assists), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Possession), SendPropInt( SENDINFO_ARRAY(m_Possession), 7, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_DistanceCovered), SendPropInt( SENDINFO_ARRAY(m_DistanceCovered), 7, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Passes), SendPropInt( SENDINFO_ARRAY(m_Passes), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_FreeKicks), SendPropInt( SENDINFO_ARRAY(m_FreeKicks), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Penalties), SendPropInt( SENDINFO_ARRAY(m_Penalties), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_Corners), SendPropInt( SENDINFO_ARRAY(m_Corners), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_ThrowIns), SendPropInt( SENDINFO_ARRAY(m_ThrowIns), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_KeeperSaves), SendPropInt( SENDINFO_ARRAY(m_KeeperSaves), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_GoalKicks), SendPropInt( SENDINFO_ARRAY(m_GoalKicks), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_TeamPosIndex), SendPropInt( SENDINFO_ARRAY(m_TeamPosIndex), 4, SPROP_UNSIGNED  ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_TeamPosNum), SendPropInt( SENDINFO_ARRAY(m_TeamPosNum), 5, SPROP_UNSIGNED  ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_NextCardJoin), SendPropInt( SENDINFO_ARRAY(m_NextCardJoin) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_IsAway), SendPropBool( SENDINFO_ARRAY(m_IsAway) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_TeamToJoin), SendPropInt( SENDINFO_ARRAY(m_TeamToJoin), 3 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_TeamPosIndexToJoin), SendPropInt( SENDINFO_ARRAY(m_TeamPosIndexToJoin), 4, SPROP_UNSIGNED ) ),

	SendPropArray3( SENDINFO_ARRAY3(m_szClubNames), SendPropString( SENDINFO_ARRAY(m_szClubNames), 0, SendProxy_String_tToStringPR ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_CountryNames), SendPropInt( SENDINFO_ARRAY(m_CountryNames), 8, SPROP_UNSIGNED ) ),
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
		m_bConnected.Set( i, 0 );
		m_iTeam.Set( i, 0 );

		//ios
		m_RedCards.Set( i, 0 );
		m_YellowCards.Set( i, 0 );
		m_Fouls.Set( i, 0 );
		m_FoulsSuffered.Set( i, 0 );
		m_SlidingTackles.Set( i, 0 );
		m_SlidingTacklesCompleted.Set( i, 0 );
		m_GoalsConceded.Set( i, 0 );
		m_Shots.Set( i, 0 );
		m_ShotsOnGoal.Set( i, 0 );
		m_PassesCompleted.Set( i, 0 );
		m_Interceptions.Set( i, 0 );
		m_Offsides.Set( i, 0 );
		m_Goals.Set( i, 0 );
		m_OwnGoals.Set( i, 0 );
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
		m_TeamPosIndex.Set( i, 0 );
		m_TeamPosNum.Set( i, 0 );
		m_NextCardJoin.Set( i, 0 );
		m_IsAway.Set( i, 0 );
		m_TeamToJoin.Set( i, 0 );
		m_TeamPosIndexToJoin.Set( i, 0 );

		m_szClubNames.Set( i, MAKE_STRING("") );
		m_CountryNames.Set( i, 0 );
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
	int pingSum[2] = { 0 };
	int pingPlayers[2] = { 0 };
	float distSum[2] = { 0 };
	int passSum[2] = { 0 };
	int passCompletedSum[2] = { 0 };
	int offsides[2] = { 0 };
	int corners[2] = { 0 };
	int goalkicks[2] = { 0 };
	int shots[2] = { 0 };
	int shotsOnGoal[2] = { 0 };
	int fouls[2] = { 0 };
	int foulsSuffered[2] = { 0 };
	int slidingTackles[2] = { 0 };
	int slidingTacklesCompletedSum[2] = { 0 };
	int freekicks[2] = { 0 };
	int goals[2] = { 0 };
	int assists[2] = { 0 };
	int interceptions[2] = { 0 };
	int redcards[2] = { 0 };
	int yellowcards[2] = { 0 };
	int penalties[2] = { 0 };
	int throwins[2] = { 0 };
	int saves[2] = { 0 };
	int owngoals[2] = { 0 };
	int goalsconceded[2] = { 0 };
	int ratings[2] = { 0 };

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		
		if ( pPlayer && pPlayer->IsConnected() )
		{
			m_bConnected.Set( i, 1 );
			m_iTeam.Set( i, pPlayer->GetTeamNumber() );

			CSDKPlayer	*SDKPlayer = ToSDKPlayer(pPlayer);
			if (SDKPlayer)
			{
				m_TeamPosIndex.Set(i, SDKPlayer->GetTeamPosIndex() );
				m_TeamPosNum.Set(i, SDKPlayer->GetTeamPosNum() );
				m_TeamToJoin.Set(i, SDKPlayer->GetTeamToJoin() );
				m_TeamPosIndexToJoin.Set(i, SDKPlayer->GetTeamPosIndexToJoin() );
				m_NextCardJoin.Set(i, SDKPlayer->GetNextCardJoin() );
				m_IsAway.Set(i, SDKPlayer->IsAway() );
				m_CountryNames.Set(i, SDKPlayer->GetCountryName());

				m_szClubNames.Set(i, MAKE_STRING(SDKPlayer->GetClubName()));

				// RomD: Enforce client update, since the value changes without notice
				if (SDKPlayer->m_bClubNameChanged)
				{
					m_szClubNames.GetForModify(i);
					SDKPlayer->m_bClubNameChanged = false;
				}
			}

			// Don't update ping / packetloss everytime

			if (m_nUpdateCounter % 20 == 0)
			{
				// update ping all 20 think ticks = (20*0.1=2seconds)
				int ping, packetloss;
				UTIL_GetPlayerConnectionInfo( i, ping, packetloss );
				
				// calc avg for scoreboard so it's not so jittery
				ping = 0.8f * m_iPing.Get(i) + 0.2f * ping;

				
				m_iPing.Set( i, ping );
				// m_iPacketloss.Set( i, packetloss );

				CSDKPlayer *pPl = ToSDKPlayer(pPlayer);
				if (SDKPlayer)
				{
					m_RedCards.Set(i, max( 0, pPl->GetRedCards() ) );
					m_YellowCards.Set(i, max( 0, pPl->GetYellowCards() ) );
					m_Fouls.Set(i, max( 0, pPl->GetFouls() ) );
					m_FoulsSuffered.Set(i, max( 0, pPl->GetFoulsSuffered() ) );
					m_SlidingTackles.Set(i, max( 0, pPl->GetSlidingTackles() ) );
					m_SlidingTacklesCompleted.Set(i, max( 0, pPl->GetSlidingTacklesCompleted() ) );
					m_GoalsConceded.Set(i, max( 0, pPl->GetGoalsConceded() ) );
					m_Shots.Set(i, max( 0, pPl->GetShots() ) );
					m_ShotsOnGoal.Set(i, max( 0, pPl->GetShotsOnGoal() ) );
					m_PassesCompleted.Set(i, max( 0, pPl->GetPassesCompleted() ) );
					m_Interceptions.Set(i, max( 0, pPl->GetInterceptions() ) );
					m_Offsides.Set(i, max( 0, pPl->GetOffsides() ) );
					m_Goals.Set(i, max( 0, pPl->GetGoals() ) );
					m_OwnGoals.Set(i, max( 0, pPl->GetOwnGoals() ) );
					m_Assists.Set(i, max( 0, pPl->GetAssists() ) );
					m_Possession.Set(i, max( 0, pPl->GetPossession() ) );
					m_DistanceCovered.Set(i, max( 0, pPl->GetDistanceCovered() ) );
					m_Passes.Set(i, max( 0, pPl->GetPasses() ) );
					m_FreeKicks.Set(i, max( 0, pPl->GetFreeKicks() ) );
					m_Penalties.Set(i, max( 0, pPl->GetPenalties() ) );
					m_Corners.Set(i, max( 0, pPl->GetCorners() ) );
					m_ThrowIns.Set(i, max( 0, pPl->GetThrowIns() ) );
					m_KeeperSaves.Set(i, max( 0, pPl->GetKeeperSaves() ) );
					m_GoalKicks.Set(i, max( 0, pPl->GetGoalKicks() ) );

					if (pPl->GetTeamNumber() == TEAM_A || pPl->GetTeamNumber() == TEAM_B)
					{
						int ti = pPl->GetTeamNumber() - TEAM_A;

						if (ping > 0)
						{
							pingSum[ti] += ping;
							pingPlayers[ti] += 1;
						}

						distSum[ti] += pPl->GetDistanceCovered() / 10.0f;

						if (pPl->GetPasses() > 0)
						{
							passSum[ti] += pPl->GetPasses();
							passCompletedSum[ti] += pPl->GetPassesCompleted();
							ratings[ti] += pPl->GetPassesCompleted() * 10 / pPl->GetPasses();
						}

						if (pPl->GetShots() > 0)
						{
							shots[ti] += pPl->GetShots();
							shotsOnGoal[ti] += pPl->GetShotsOnGoal();
						}

						if (pPl->GetSlidingTackles() > 0)
						{
							slidingTackles[ti] += pPl->GetSlidingTackles();
							slidingTacklesCompletedSum[ti] += pPl->GetSlidingTacklesCompleted();
						}

						offsides[ti] += pPl->GetOffsides();
						corners[ti] += pPl->GetCorners();
						goalkicks[ti] += pPl->GetGoalKicks();
						fouls[ti] += pPl->GetFouls();
						foulsSuffered[ti] += pPl->GetFoulsSuffered();
						freekicks[ti] += pPl->GetFreeKicks();
						goals[ti] += pPl->GetGoals();
						assists[ti] += pPl->GetAssists();
						interceptions[ti] += pPl->GetInterceptions();
						redcards[ti] += pPl->GetRedCards();
						yellowcards[ti] += pPl->GetYellowCards();
						penalties[ti] += pPl->GetPenalties();
						throwins[ti] += pPl->GetThrowIns();
						saves[ti] += pPl->GetKeeperSaves();
						owngoals[ti] += pPl->GetOwnGoals();
						goalsconceded[ti] += pPl->GetGoalsConceded();
					}
				}
			}
		}
		else
		{
			m_bConnected.Set( i, 0 );
		}
	}


	for (int team = TEAM_A; team <= TEAM_B; team++)
	{
		int ti = team - TEAM_A;
		CTeam *pTeam = GetGlobalTeam(team);

		if (m_nUpdateCounter % 20 == 0)
		{
			pTeam->m_Ping = pingSum[ti] / max(1, pingPlayers[ti]);
			//pTeam->m_Possession = pTeam->GetPossession();
			pTeam->m_Passes = passSum;
			pTeam->m_PassesCompleted = 100 * passCompletedSum[ti] / max(1, passSum[ti]);
			pTeam->m_DistanceCovered = distSum[ti];
			pTeam->m_Offsides = offsides[ti];
			pTeam->m_Corners = corners[ti];
			pTeam->m_GoalKicks = goalkicks[ti];
			pTeam->m_Shots = shots[ti];
			pTeam->m_ShotsOnGoal = 100 * shotsOnGoal[ti] / max(1, shots[ti]);
			pTeam->m_Fouls = fouls[ti];
			pTeam->m_FoulsSuffered = foulsSuffered[ti];
			pTeam->m_SlidingTackles = slidingTackles[ti];
			pTeam->m_SlidingTacklesCompleted = 100 * slidingTacklesCompletedSum[ti] / max(1, slidingTackles[ti]);
			pTeam->m_FreeKicks = freekicks[ti];
			pTeam->m_Goals = goals[ti];
			pTeam->m_Assists = assists[ti];
			pTeam->m_Interceptions = interceptions[ti];
			pTeam->m_RedCards = redcards[ti];
			pTeam->m_YellowCards = yellowcards[ti];
			pTeam->m_Penalties = penalties[ti];
			pTeam->m_ThrowIns = throwins[ti];
			pTeam->m_KeeperSaves = saves[ti];
			pTeam->m_OwnGoals = owngoals[ti];
			pTeam->m_GoalsConceded = goalsconceded[ti];
			pTeam->m_Rating = 100;
		}
	}
}
