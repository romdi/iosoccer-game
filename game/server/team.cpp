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
	SendPropInt( SENDINFO(m_iTeamNum), 3 ),
	SendPropInt( SENDINFO(m_nPenaltyGoals), 5, SPROP_UNSIGNED),
	SendPropInt( SENDINFO(m_nPenaltyGoalBits), 32, SPROP_UNSIGNED),
	SendPropInt( SENDINFO(m_nPenaltyRound), 5, SPROP_UNSIGNED),
	SendPropInt( SENDINFO(m_nTimeoutsLeft), 4, SPROP_UNSIGNED),
	SendPropInt( SENDINFO(m_nTimeoutTimeLeft), 10, SPROP_UNSIGNED),
	SendPropString( SENDINFO( m_szServerKitName ) ),
	SendPropString( SENDINFO( m_szServerCode ) ),
	SendPropString( SENDINFO( m_szServerShortName ) ),

	SendPropVector(SENDINFO(m_vCornerLeft), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vCornerRight), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vGoalkickLeft), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vGoalkickRight), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vPenalty), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vGoalCenter), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vPenBoxMin), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vPenBoxMax), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vSixYardBoxMin), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vSixYardBoxMax), -1, SPROP_COORD),
	SendPropInt(SENDINFO(m_nForward), 2),
	SendPropInt(SENDINFO(m_nRight), 2),
	SendPropIntWithMinusOneFlag(SENDINFO(m_nCaptainPosIndex), 4),
	SendPropIntWithMinusOneFlag(SENDINFO(m_nFreekickTakerPosIndex), 4),
	SendPropIntWithMinusOneFlag(SENDINFO(m_nPenaltyTakerPosIndex), 4),
	SendPropIntWithMinusOneFlag(SENDINFO(m_nLeftCornerTakerPosIndex), 4),
	SendPropIntWithMinusOneFlag(SENDINFO(m_nRightCornerTakerPosIndex), 4),

	SendPropArray2( 
		SendProxyArrayLength_PlayerArray,
		SendPropInt("player_array_element", 0, 4, 10, SPROP_UNSIGNED, SendProxy_PlayerList), 
		MAX_PLAYERS, 
		0, 
		"player_array"
		),

	SendPropArray3( SENDINFO_ARRAY3(m_szMatchEventPlayers), SendPropString( SENDINFO_ARRAY(m_szMatchEventPlayers), 0, SendProxy_String_tToStringT ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_eMatchEventTypes), SendPropInt( SENDINFO_ARRAY(m_eMatchEventTypes), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_eMatchEventMatchPeriods), SendPropInt( SENDINFO_ARRAY(m_eMatchEventMatchPeriods), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_nMatchEventSeconds), SendPropInt( SENDINFO_ARRAY(m_nMatchEventSeconds), 13, SPROP_UNSIGNED ) ),

	SendPropArray3( SENDINFO_ARRAY3(m_PosNextJoinSeconds), SendPropInt( SENDINFO_ARRAY(m_PosNextJoinSeconds), 13, SPROP_UNSIGNED ) ),

	SendPropInt(SENDINFO(m_RedCards), 4, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_YellowCards), 4, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Fouls), 6, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_FoulsSuffered), 6, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_SlidingTackles), 8, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_SlidingTacklesCompleted), 8, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_GoalsConceded), 5, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Shots), 6, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_ShotsOnGoal), 6, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_PassesCompleted), 8, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Interceptions), 8, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Offsides), 6, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Goals), 5, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_OwnGoals), 5, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Assists), 5, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Possession), 7, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_DistanceCovered), 11, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Passes), 9, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_FreeKicks), 6, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Penalties), 6, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Corners), 6, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_ThrowIns), 6, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_KeeperSaves), 6, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_GoalKicks), 5, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Ping), 10, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_Rating), 7, SPROP_UNSIGNED),

	SendPropInt(SENDINFO(m_nFormationIndex), 4, SPROP_UNSIGNED),
	SendPropIntWithMinusOneFlag(SENDINFO(m_eQuickTactic), 4),
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
	memset( m_szServerCode.GetForModify(), 0, sizeof(m_szServerCode) );
	memset( m_szServerShortName.GetForModify(), 0, sizeof(m_szServerShortName) );
	ResetStats();
	UpdatePosIndices(true);
	m_nTimeoutsLeft = mp_timeout_count.GetInt();
	m_nTimeoutTimeLeft = mp_timeout_duration.GetInt() * 60;
	m_nFormationIndex = 0;
	m_eQuickTactic = QUICKTACTIC_NONE;
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
	return true;
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

const char *CTeam::GetTeamCode( void )
{
	return m_szServerCode;
}

const char *CTeam::GetShortTeamName( void )
{
	return m_szServerShortName;
}

void CTeam::SetTeamNumber(int teamNum)
{
	m_iTeamNum = teamNum;
}

void CTeam::SetKitName(const char *pName)
{
	Q_strncpy( m_szServerKitName.GetForModify(), pName, MAX_TEAM_NAME_LENGTH );
}

void CTeam::SetTeamCode(const char *pCode)
{
	Q_strncpy( m_szServerCode.GetForModify(), pCode, MAX_TEAMCODE_LENGTH );
}

void CTeam::SetShortTeamName(const char *pName)
{
	Q_strncpy( m_szServerShortName.GetForModify(), pName, MAX_SHORTTEAMNAME_LENGTH );
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
void CTeam::AddPlayer( CBasePlayer *pPlayer, int posIndex )
{
	m_aPlayers.AddToTail( pPlayer );
	m_PosIndexPlayerIndices[posIndex] = m_aPlayers.Count() - 1;
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: Remove this player from the team
//-----------------------------------------------------------------------------
void CTeam::RemovePlayer( CBasePlayer *pPlayer )
{
	if ((GetTeamNumber() == TEAM_A || GetTeamNumber() == TEAM_B) && pPlayer == GetCaptain())
		SetCaptainPosIndex(-1);

	m_aPlayers.FindAndRemove( pPlayer );

	UpdatePosIndices(false);

	NetworkStateChanged();
}

void CTeam::UpdatePosIndices(bool reset)
{
	for (int i = 0; i < 11; i++)
		m_PosIndexPlayerIndices[i] = -1;

	for (int i = 0; i < m_aPlayers.Count(); i++)
	{
		m_PosIndexPlayerIndices[ToSDKPlayer(m_aPlayers[i])->GetTeamPosIndex()] = i;
	}

	if (reset)
	{
		SetCaptainPosIndex(-1);
		SetFreekickTakerPosIndex(-1);
		SetPenaltyTakerPosIndex(-1);
		SetLeftCornerTakerPosIndex(-1);
		SetRightCornerTakerPosIndex(-1);
		UnblockAllPos();
	}
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

CSDKPlayer *CTeam::GetPlayerByPosIndex(int posIndex)
{
	if (posIndex < 0 || posIndex > 10 || m_PosIndexPlayerIndices[posIndex] == -1)
		return NULL;
	else
		return ToSDKPlayer(m_aPlayers[m_PosIndexPlayerIndices[posIndex]]);
}

int CTeam::GetPosIndexByPosType(PosTypes_t posType)
{
	for (int i = 0; i < GetFormation()->positions.Count(); i++)
	{
		if (GetFormation()->positions[i]->type == posType)
			return i;
	}

	return -1;
}

CSDKPlayer *CTeam::GetPlayerByPosType(PosTypes_t posType)
{
	return GetPlayerByPosIndex(GetPosIndexByPosType(posType));
}

int CTeam::GetPosNextJoinSeconds(int posIndex)
{
	return m_PosNextJoinSeconds[posIndex];
}

void CTeam::SetPosNextJoinSeconds(int posIndex, int seconds)
{
	m_PosNextJoinSeconds.Set(posIndex, seconds);
}

void CTeam::UnblockAllPos()
{
	for (int i = 0; i < 11; i++)
		m_PosNextJoinSeconds.Set(i, 0);
}

//------------------------------------------------------------------------------------------------------------------
// SCORING
//-----------------------------------------------------------------------------
// Purpose: Add / Remove score for this team
//-----------------------------------------------------------------------------
//void CTeam::AddGoal()
//{
//	m_Goals += 1;
//}

void CTeam::SetGoals( int goals )
{
	m_Goals = goals;
}

//-----------------------------------------------------------------------------
// Purpose: Get this team's score
//-----------------------------------------------------------------------------
int CTeam::GetGoals( void )
{
	return m_Goals;
}

void CTeam::SetPossession( int possession )
{
	m_Possession = possession;
}

int CTeam::GetPossession( void )
{
	return m_Possession;
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
	{
		Error(UTIL_VarArgs("'%s' missing", name));
		return vec3_invalid;
	}
}

void CTeam::InitFieldSpots(int team)
{
	int index = team - TEAM_A;
	m_vCornerLeft = GetSpotPos(UTIL_VarArgs("info_team%d_corner%d", index + 1, 1 - index));
	m_vCornerRight = GetSpotPos(UTIL_VarArgs("info_team%d_corner%d", index + 1, index));
	m_vGoalkickLeft = GetSpotPos(UTIL_VarArgs("info_team%d_goalkick1", index + 1));
	m_vGoalkickRight = GetSpotPos(UTIL_VarArgs("info_team%d_goalkick0", index + 1));
	m_vPenalty = GetSpotPos(UTIL_VarArgs("info_team%d_penalty_spot", index + 1));

	m_vGoalCenter = Vector(SDKGameRules()->m_vKickOff.GetX(),
		(m_vPenalty.GetY() < SDKGameRules()->m_vKickOff.GetY() ? SDKGameRules()->m_vFieldMin.GetY() : SDKGameRules()->m_vFieldMax.GetY()),
		SDKGameRules()->m_vKickOff.GetZ());

	CBaseEntity *pPenBox = gEntList.FindEntityByClassnameNearest("trigger_PenaltyBox", m_vPenalty, 9999);

	if (!pPenBox)
		Error("'trigger_PenaltyBox' missing");

	pPenBox->CollisionProp()->WorldSpaceTriggerBounds(&m_vPenBoxMin.GetForModify(), &m_vPenBoxMax.GetForModify());

	CBaseEntity *pGoalTrigger = gEntList.FindEntityByClassnameNearest("trigger_goal", m_vPenalty, 9999);

	Vector goalMin, goalMax;
	pGoalTrigger->CollisionProp()->WorldSpaceTriggerBounds(&goalMin, &goalMax);

	float sixYardLength = (goalMax.x - goalMin.x) / 4 * 3;

	m_vSixYardBoxMin = m_vGoalCenter - Vector(sixYardLength / 3 * 5, 0, 0);
	m_vSixYardBoxMax = m_vGoalCenter + Vector(sixYardLength / 3 * 5, sixYardLength, 0);

	m_nForward = Sign(SDKGameRules()->m_vKickOff.GetY() - m_vGoalCenter.GetY());
	m_nRight = Sign(m_vCornerRight.GetX() - m_vGoalCenter.GetX());
}

void CTeam::ResetStats()
{
	m_RedCards = 0;
	m_YellowCards = 0;
	m_Fouls = 0;
	m_FoulsSuffered = 0;
	m_SlidingTackles = 0;
	m_SlidingTacklesCompleted = 0;
	m_GoalsConceded = 0;
	m_Shots = 0;
	m_ShotsOnGoal = 0;
	m_PassesCompleted = 0;
	m_Interceptions = 0;
	m_Offsides = 0;
	m_Goals = 0;
	m_OwnGoals = 0;
	m_Assists = 0;
	m_Possession = 0;
	m_DistanceCovered = 0;
	m_Passes = 0;
	m_FreeKicks = 0;
	m_Penalties = 0;
	m_Corners = 0;
	m_ThrowIns = 0;
	m_KeeperSaves = 0;
	m_GoalKicks = 0;
	m_flPossessionTime = 0;
	m_flExactDistanceCovered = 0;
	m_nMatchEventIndex = 0;
	m_Rating = 0;
	m_Ping = 0;
	m_nTimeoutsLeft = mp_timeout_count.GetInt();
	m_nTimeoutTimeLeft = mp_timeout_duration.GetInt() * 60;

	for (int i = 0; i < MAX_MATCH_EVENTS; i++)
	{
		memset(m_szMatchEventPlayersMemory, 0, sizeof(m_szMatchEventPlayersMemory));
		m_szMatchEventPlayers.Set(i, MAKE_STRING(""));
		m_eMatchEventTypes.Set(i, 0);
		m_eMatchEventMatchPeriods.Set(i, 0);
		m_nMatchEventSeconds.Set(i, 0);
	}

	UnblockAllPos();
}

void CTeam::FindNewCaptain()
{
	if (GetTeamNumber() != TEAM_A && GetTeamNumber() != TEAM_B)
		return;

	SetCaptainPosIndex(0);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!CSDKPlayer::IsOnField(pPl) || pPl->GetTeam() != this)
			continue;

		if (pPl->GetTeamPosType() == POS_GK)
		{
			SetCaptainPosIndex(pPl->GetTeamPosIndex());
			break;
		}
	}
}

void CTeam::AddMatchEvent(match_period_t matchPeriod, int seconds, match_event_t event, const char *text)
{
	if (m_nMatchEventIndex == MAX_MATCH_EVENTS)
		return;

	m_nMatchEventSeconds.Set(m_nMatchEventIndex, seconds);
	m_eMatchEventTypes.Set(m_nMatchEventIndex, event);
	m_eMatchEventMatchPeriods.Set(m_nMatchEventIndex, matchPeriod);
	Q_strncpy(m_szMatchEventPlayersMemory[m_nMatchEventIndex], text, MAX_MATCH_EVENT_PLAYER_NAME_LENGTH);
	m_szMatchEventPlayers.Set(m_nMatchEventIndex, MAKE_STRING(m_szMatchEventPlayersMemory[m_nMatchEventIndex]));
	m_nMatchEventIndex += 1;
}

Formation *CTeam::GetFormation()
{
	return SDKGameRules()->GetFormations()[m_nFormationIndex];
}

void CTeam::SetFormationIndex(int index, bool silent)
{
	if (index == m_nFormationIndex || index < 0 || index > SDKGameRules()->GetFormations().Count() - 1)
		return;

	if (!silent)
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent("team_formation");
		if (pEvent)
		{
			pEvent->SetInt("team", GetTeamNumber());
			pEvent->SetInt("old_formation", m_nFormationIndex);
			pEvent->SetInt("new_formation", index);
			gameeventmanager->FireEvent(pEvent);
		}
	}

	m_nFormationIndex = index;
}