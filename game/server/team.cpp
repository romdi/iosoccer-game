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


// Datatable
IMPLEMENT_SERVERCLASS_ST_NOBASE(CTeam, DT_Team)
	SendPropInt( SENDINFO(m_iTeamNum), 5 ),
	SendPropInt( SENDINFO(m_nGoals), 0 ),
	SendPropInt( SENDINFO(m_nPossession), 0 ),
	SendPropString( SENDINFO( m_szTeamname ) ),

	SendPropVector(SENDINFO(m_vCornerLeft), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vCornerRight), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vGoalkickLeft), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vGoalkickRight), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vPenalty), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vPenBoxMin), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vPenBoxMax), -1, SPROP_COORD),
	SendPropInt(SENDINFO(m_nForward)),
	SendPropInt(SENDINFO(m_nRight)),

	SendPropArray2( 
		SendProxyArrayLength_PlayerArray,
		SendPropInt("player_array_element", 0, 4, 10, SPROP_UNSIGNED, SendProxy_PlayerList), 
		MAX_PLAYERS, 
		0, 
		"player_array"
		)
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
	memset( m_szTeamname.GetForModify(), 0, sizeof(m_szTeamname) );
	ResetStats();
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
	Q_strncpy( m_szTeamname.GetForModify(), pName, MAX_TEAM_NAME_LENGTH );
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

void CTeam::SetTeamNumber(int teamNum)
{
	m_iTeamNum = teamNum;
}


//-----------------------------------------------------------------------------
// Purpose: Get the team's name
//-----------------------------------------------------------------------------
const char *CTeam::GetName( void )
{
	return m_szTeamname;
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

	if (m_vCornerLeft[0] < SDKGameRules()->m_vFieldMin[0])
		SDKGameRules()->m_vFieldMin.SetX(m_vCornerLeft[0]);

	if (m_vCornerLeft[1] < SDKGameRules()->m_vFieldMin[1])
		SDKGameRules()->m_vFieldMin.SetY(m_vCornerLeft[1]);

	if (m_vCornerRight[0] < SDKGameRules()->m_vFieldMin[0])
		SDKGameRules()->m_vFieldMin.SetX(m_vCornerRight[0]);

	if (m_vCornerRight[1] < SDKGameRules()->m_vFieldMin[1])
		SDKGameRules()->m_vFieldMin.SetY(m_vCornerRight[1]);

	if (m_vCornerLeft[0] > SDKGameRules()->m_vFieldMax[0])
		SDKGameRules()->m_vFieldMax.SetX(m_vCornerLeft[0]);

	if (m_vCornerLeft[1] > SDKGameRules()->m_vFieldMax[1])
		SDKGameRules()->m_vFieldMax.SetY(m_vCornerLeft[1]);

	if (m_vCornerRight[0] > SDKGameRules()->m_vFieldMax[0])
		SDKGameRules()->m_vFieldMax.SetX(m_vCornerRight[0]);

	if (m_vCornerRight[1] > SDKGameRules()->m_vFieldMax[1])
		SDKGameRules()->m_vFieldMax.SetY(m_vCornerRight[1]);

	m_vGoalkickLeft = GetSpotPos(UTIL_VarArgs("info_team%d_goalkick1", index + 1));
	m_vGoalkickRight = GetSpotPos(UTIL_VarArgs("info_team%d_goalkick0", index + 1));
	m_vPenalty = GetSpotPos(UTIL_VarArgs("info_team%d_penalty_spot", index + 1));

	CBaseEntity *pPenBox = gEntList.FindEntityByClassnameNearest("trigger_PenaltyBox", m_vPenalty, 9999);
	pPenBox->CollisionProp()->WorldSpaceTriggerBounds(&m_vPenBoxMin.GetForModify(), &m_vPenBoxMax.GetForModify());

	if (SDKGameRules()->m_vKickOff.GetY() > m_vPenBoxMin.GetY())
		m_vPenBoxMin.SetY(m_vPenBoxMin.GetY() - 150);
	else
		m_vPenBoxMax.SetY(m_vPenBoxMax.GetY() + 150);

	CBaseEntity *pCrossbar = gEntList.FindEntityByName(NULL, UTIL_VarArgs("team%d_crossbar", index + 1));
	if (pCrossbar)
	{
		pCrossbar->SetRenderMode(kRenderTransColor);
		pCrossbar->SetRenderColorA(100);
	}

	for (int j = 0; j < 11; j++)
	{
		m_vPlayerSpawns[j] = GetSpotPos(UTIL_VarArgs("info_team%d_player%d", index + 1, j + 1));
	}

	m_nForward = Sign((SDKGameRules()->m_vKickOff - m_vPlayerSpawns[0]).y);
	m_nRight = Sign((m_vCornerRight - m_vPlayerSpawns[0]).x);
}

void CTeam::ResetStats()
{
	m_flPossessionTime = 0;
	m_nPossession = 0;
	m_nGoals = 0;
}