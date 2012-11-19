//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "team.h"
#include "player.h"
#include "team_spawnpoint.h"
#include "sdk_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CUtlVector< CTeam * > g_Teams;

//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the Team's player UtlVector to entindexes
//-----------------------------------------------------------------------------
void SendProxy_PlayerList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CTeam *pTeam = (CTeam*)pData;

	// If this assertion fails, then SendProxyArrayLength_PlayerArray must have failed.
	Assert( iElement < pTeam->m_aPlayers.Size() );

	CBasePlayer *pPlayer = pTeam->m_aPlayers[iElement];
	pOut->m_Int = pPlayer->entindex();
}


int SendProxyArrayLength_PlayerArray( const void *pStruct, int objectID )
{
	CTeam *pTeam = (CTeam*)pStruct;
	return pTeam->m_aPlayers.Count();
}

void SendProxy_String_tToStringT( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	string_t *pString = (string_t*)pData;
	pOut->m_pString = (char*)STRING( *pString );
}

// Datatable
IMPLEMENT_SERVERCLASS_ST_NOBASE(CTeam, DT_Team)
	SendPropInt( SENDINFO(m_iTeamNum), 5 ),
	SendPropInt( SENDINFO(m_nGoals), 0 ),
	SendPropInt( SENDINFO(m_nPossession), 0 ),
	SendPropInt( SENDINFO(m_nPenaltyGoals), 0 ),
	SendPropInt( SENDINFO(m_nPenaltyGoalBits), 0 ),
	SendPropInt( SENDINFO(m_nPenaltyRound)),
	SendPropInt( SENDINFO(m_nTimeoutsLeft)),
	SendPropString( SENDINFO( m_szServerKitName ) ),

	SendPropVector(SENDINFO(m_vCornerLeft), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vCornerRight), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vGoalkickLeft), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vGoalkickRight), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vPenalty), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vPenBoxMin), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vPenBoxMax), -1, SPROP_COORD),
	SendPropInt(SENDINFO(m_nForward)),
	SendPropInt(SENDINFO(m_nRight)),
	SendPropEHandle(SENDINFO(m_pCaptain)),
	SendPropEHandle(SENDINFO(m_pFreekickTaker)),
	SendPropEHandle(SENDINFO(m_pPenaltyTaker)),
	SendPropEHandle(SENDINFO(m_pCornerTaker)),
	SendPropEHandle(SENDINFO(m_pThrowinTaker)),

	SendPropArray2( 
		SendProxyArrayLength_PlayerArray,
		SendPropInt("player_array_element", 0, 4, 10, SPROP_UNSIGNED, SendProxy_PlayerList), 
		MAX_PLAYERS, 
		0, 
		"player_array"
		),

	SendPropArray3( SENDINFO_ARRAY3(m_szMatchEventPlayers), SendPropString( SENDINFO_ARRAY(m_szMatchEventPlayers), 0, SendProxy_String_tToStringT ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_eMatchEventTypes), SendPropInt( SENDINFO_ARRAY(m_eMatchEventTypes), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_nMatchEventSeconds), SendPropInt( SENDINFO_ARRAY(m_nMatchEventSeconds), 13, SPROP_UNSIGNED ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( team_manager, CTeam );

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to the specified team manager
//-----------------------------------------------------------------------------
CTeam *GetGlobalTeam( int iIndex )
{
	if ( iIndex < 0 || iIndex >= GetNumberOfTeams() )
		return NULL;

	return g_Teams[ iIndex ];
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of team managers
//-----------------------------------------------------------------------------
int GetNumberOfTeams( void )
{
	return g_Teams.Size();
}


//-----------------------------------------------------------------------------
// Purpose: Needed because this is an entity, but should never be used
//-----------------------------------------------------------------------------
CTeam::CTeam( void )
{
	memset( m_szServerKitName.GetForModify(), 0, sizeof(m_szServerKitName) );
	ResetStats();
	SetCaptain(NULL);
	SetFreekickTaker(NULL);
	SetPenaltyTaker(NULL);
	SetCornerTaker(NULL);
	SetThrowinTaker(NULL);
	m_nTimeoutsLeft = 3;

	for (int i = 0; i < MAX_MATCH_EVENTS; i++)
	{
		memset(m_szMatchEventPlayersMemory, 0, sizeof(m_szMatchEventPlayersMemory));
		m_szMatchEventPlayers.Set(i, MAKE_STRING(""));
		m_eMatchEventTypes.Set(i, 0);
		m_nMatchEventSeconds.Set(i, 0);
	}

	m_nMatchEventIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeam::~CTeam( void )
{
	m_aPlayers.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame
//-----------------------------------------------------------------------------
void CTeam::Think( void )
{
	DevMsg("foo\n");
}

//-----------------------------------------------------------------------------
// Purpose: Teams are always transmitted to clients
//-----------------------------------------------------------------------------
int CTeam::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Visibility/scanners
//-----------------------------------------------------------------------------
bool CTeam::ShouldTransmitToPlayer( CBasePlayer* pRecipient, CBaseEntity* pEntity )
{
	// Always transmit the observer target to players
	if ( pRecipient && pRecipient->IsObserver() && pRecipient->GetObserverTarget() == pEntity )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
void CTeam::Init( const char *pName, int iNumber )
{
	SetKitName(pName);
	m_iTeamNum = iNumber;
}

int CTeam::GetTeamNumber( void ) const
{
	return m_iTeamNum;
}

int CTeam::GetOppTeamNumber( void ) const
{
	if (m_iTeamNum != TEAM_A && m_iTeamNum != TEAM_B)
		return m_iTeamNum;

	return m_iTeamNum == TEAM_A ? TEAM_B : TEAM_A;
}

CTeam *CTeam::GetOppTeam( void ) const
{
	if (m_iTeamNum != TEAM_A && m_iTeamNum != TEAM_B)
		return GetGlobalTeam(m_iTeamNum);

	return m_iTeamNum == TEAM_A ? GetGlobalTeam(TEAM_B) : GetGlobalTeam(TEAM_A);
}

void CTeam::SetTeamNumber(int teamNum)
{
	m_iTeamNum = teamNum;
}

void CTeam::SetKitName(const char *pName)
{
	Q_strncpy( m_szServerKitName.GetForModify(), pName, MAX_TEAM_NAME_LENGTH );
}

//-----------------------------------------------------------------------------
// Purpose: Get the team's name
//-----------------------------------------------------------------------------
const char *CTeam::GetKitName( void )
{
	return m_szServerKitName;
}

//-----------------------------------------------------------------------------
// Purpose: Update the player's client data
//-----------------------------------------------------------------------------
void CTeam::UpdateClientData( CBasePlayer *pPlayer )
{
}

//-----------------------------------------------------------------------------
// Purpose: Add the specified player to this team. Remove them from their current team, if any.
//-----------------------------------------------------------------------------
void CTeam::AddPlayer( CBasePlayer *pPlayer )
{
	m_aPlayers.AddToTail( pPlayer );
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: Remove this player from the team
//-----------------------------------------------------------------------------
void CTeam::RemovePlayer( CBasePlayer *pPlayer )
{
	m_aPlayers.FindAndRemove( pPlayer );
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: Return the number of players in this team.
//-----------------------------------------------------------------------------
int CTeam::GetNumPlayers( void )
{
	return m_aPlayers.Size();
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific player
//-----------------------------------------------------------------------------
CBasePlayer *CTeam::GetPlayer( int iIndex )
{
	Assert( iIndex >= 0 && iIndex < m_aPlayers.Size() );
	return m_aPlayers[ iIndex ];
}

//------------------------------------------------------------------------------------------------------------------
// SCORING
//-----------------------------------------------------------------------------
// Purpose: Add / Remove score for this team
//-----------------------------------------------------------------------------
void CTeam::AddGoal()
{
	m_nGoals += 1;
}

void CTeam::SetGoals( int goals )
{
	m_nGoals = goals;
}

//-----------------------------------------------------------------------------
// Purpose: Get this team's score
//-----------------------------------------------------------------------------
int CTeam::GetGoals( void )
{
	return m_nGoals;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeam::AwardAchievement( int iAchievement )
{
	Assert( iAchievement >= 0 && iAchievement < 255 );	// must fit in short 

	CRecipientFilter filter;

	int iNumPlayers = GetNumPlayers();

	for ( int i=0;i<iNumPlayers;i++ )
	{
		if ( GetPlayer(i) )
		{
			filter.AddRecipient( GetPlayer(i) );
		}
	}

	UserMessageBegin( filter, "AchievementEvent" );
		WRITE_SHORT( iAchievement );
	MessageEnd();
}

Vector CTeam::GetSpotPos(const char *name)
{
	CBaseEntity *pEnt = gEntList.FindEntityByClassname(NULL, name);
	if (pEnt)
		return Vector(pEnt->GetLocalOrigin().x, pEnt->GetLocalOrigin().y, SDKGameRules()->m_vKickOff.GetZ());
	else
		return vec3_invalid;
}

void CTeam::InitFieldSpots(int team)
{
	int index = team - TEAM_A;
	m_vCornerLeft = GetSpotPos(UTIL_VarArgs("info_team%d_corner%d", index + 1, 1 - index));
	m_vCornerRight = GetSpotPos(UTIL_VarArgs("info_team%d_corner%d", index + 1, index));
	m_vGoalkickLeft = GetSpotPos(UTIL_VarArgs("info_team%d_goalkick1", index + 1));
	m_vGoalkickRight = GetSpotPos(UTIL_VarArgs("info_team%d_goalkick0", index + 1));
	m_vPenalty = GetSpotPos(UTIL_VarArgs("info_team%d_penalty_spot", index + 1));

	CBaseEntity *pPenBox = gEntList.FindEntityByClassnameNearest("trigger_PenaltyBox", m_vPenalty, 9999);
	pPenBox->CollisionProp()->WorldSpaceTriggerBounds(&m_vPenBoxMin.GetForModify(), &m_vPenBoxMax.GetForModify());

	//if (SDKGameRules()->m_vKickOff.GetY() > m_vPenBoxMin.GetY())
	//	m_vPenBoxMin.SetY(m_vPenBoxMin.GetY() - 150);
	//else
	//	m_vPenBoxMax.SetY(m_vPenBoxMax.GetY() + 150);

	for (int j = 0; j < 11; j++)
	{
		m_vPlayerSpawns[j] = GetSpotPos(UTIL_VarArgs("info_team%d_player%d", index + 1, j + 1));
	}

	m_nForward = Sign(SDKGameRules()->m_vKickOff.GetY() - m_vPlayerSpawns[0].y);
	m_nRight = Sign(m_vCornerRight.GetX() - m_vPlayerSpawns[0].x);
}

void CTeam::ResetStats()
{
	m_flPossessionTime = 0;
	m_nPossession = 0;
	m_nGoals = 0;
}

void CTeam::FindNewCaptain()
{
	if (GetTeamNumber() != TEAM_A && GetTeamNumber() != TEAM_B)
		return;

	SetCaptain(NULL);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl) || pPl->GetTeam() != this)
			continue;

		if (pPl->GetTeamPosType() == GK)
		{
			SetCaptain(pPl);
			break;
		}
	}
}

void CTeam::AddMatchEvent(int seconds, match_event_t event, const char *player)
{
	m_nMatchEventSeconds.Set(m_nMatchEventIndex, seconds);
	m_eMatchEventTypes.Set(m_nMatchEventIndex, event);
	Q_strncpy(m_szMatchEventPlayersMemory[m_nMatchEventIndex], player, MAX_MATCH_EVENT_PLAYER_NAME_LENGTH);
	m_szMatchEventPlayers.Set(m_nMatchEventIndex, MAKE_STRING(m_szMatchEventPlayersMemory[m_nMatchEventIndex]));
	m_nMatchEventIndex += 1;
}