//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side CTeam class
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_team.h"
#include "sdk_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Team's player UtlVector to entindexes
//-----------------------------------------------------------------------------
void RecvProxy_PlayerList(  const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_Team *pTeam = (C_Team*)pOut;
	pTeam->m_aPlayers[pData->m_iElement] = pData->m_Value.m_Int;
}


void RecvProxyArrayLength_PlayerArray( void *pStruct, int objectID, int currentArrayLength )
{
	C_Team *pTeam = (C_Team*)pStruct;
	
	if ( pTeam->m_aPlayers.Size() != currentArrayLength )
		pTeam->m_aPlayers.SetSize( currentArrayLength );
}

void RecvProxy_KitName( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((C_Team *)pStruct)->SetKitName(pData->m_Value.m_pString);
}

IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_Team, DT_Team, CTeam)
	RecvPropInt( RECVINFO(m_iTeamNum)),
	RecvPropInt( RECVINFO(m_nGoals)),
	RecvPropInt( RECVINFO(m_nPossession) ),
	RecvPropInt( RECVINFO(m_nPenaltyGoals) ),
	RecvPropInt( RECVINFO(m_nPenaltyGoalBits) ),
	RecvPropInt( RECVINFO(m_nPenaltyRound) ),
	RecvPropString( RECVINFO(m_szKitName), 0, RecvProxy_KitName),

	RecvPropVector(RECVINFO(m_vCornerLeft)),
	RecvPropVector(RECVINFO(m_vCornerRight)),
	RecvPropVector(RECVINFO(m_vGoalkickLeft)),
	RecvPropVector(RECVINFO(m_vGoalkickRight)),
	RecvPropVector(RECVINFO(m_vPenalty)),
	RecvPropVector(RECVINFO(m_vPenBoxMin)),
	RecvPropVector(RECVINFO(m_vPenBoxMax)),
	RecvPropInt(RECVINFO(m_nForward)),
	RecvPropInt(RECVINFO(m_nRight)),
	
	RecvPropArray2( 
		RecvProxyArrayLength_PlayerArray,
		RecvPropInt( "player_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_PlayerList ), 
		MAX_PLAYERS, 
		0, 
		"player_array"
		)
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_Team )
	DEFINE_PRED_FIELD( m_iTeamNum, FIELD_INTEGER, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_szKitName, FIELD_CHARACTER, MAX_TEAM_NAME_LENGTH, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nGoals, FIELD_INTEGER, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_iPing, FIELD_INTEGER, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_iPacketloss, FIELD_INTEGER, FTYPEDESC_PRIVATE ),
END_PREDICTION_DATA();

// Global list of client side team entities
CUtlVector< C_Team * > g_Teams;

//=================================================================================================
// C_Team functionality

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_Team::C_Team()
{
	m_nGoals = 0;
	m_nPossession = 0;
	memset( m_szKitName, 0, sizeof(m_szKitName) );

	m_iPing = 0;
	m_iPacketloss = 0;

	// Add myself to the global list of team entities
	g_Teams.AddToTail( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_Team::~C_Team()
{
	g_Teams.FindAndRemove( this );
}


void C_Team::RemoveAllPlayers()
{
	m_aPlayers.RemoveAll();
}

void C_Team::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );
}


//-----------------------------------------------------------------------------
// Gets the ith player on the team (may return NULL) 
//-----------------------------------------------------------------------------
C_BasePlayer* C_Team::GetPlayer( int idx )
{
	return (C_BasePlayer*)cl_entitylist->GetEnt(m_aPlayers[idx]);
}

//=================================================================================================
// TEAM HANDLING
//=================================================================================================
// Purpose: 
//-----------------------------------------------------------------------------

int	C_Team::GetTeamNumber( void ) const
{
	return m_iTeamNum;
}

int C_Team::GetOppTeamNumber( void ) const
{
	if (m_iTeamNum != TEAM_A && m_iTeamNum != TEAM_B)
		return m_iTeamNum;

	return m_iTeamNum == TEAM_A ? TEAM_B : TEAM_A;
}

C_Team *C_Team::GetOppTeam( void ) const
{
	if (m_iTeamNum != TEAM_A && m_iTeamNum != TEAM_B)
		return GetGlobalTeam(m_iTeamNum);

	return m_iTeamNum == TEAM_A ? GetGlobalTeam(TEAM_B) : GetGlobalTeam(TEAM_A);
}

bool C_Team::Get_IsClubTeam( void )
{
	return m_pTeamKitInfo->m_bIsClubTeam;
}

bool C_Team::Get_IsRealTeam( void )
{
	return m_pTeamKitInfo->m_bIsRealTeam;
}

bool C_Team::Get_HasTeamCrest( void )
{
	return m_pTeamKitInfo->m_bHasTeamCrest;
}

char *C_Team::Get_TeamCode( void )
{
	return m_pTeamKitInfo->m_szTeamCode;
}

char *C_Team::Get_FullTeamName( void )
{
	if (m_iTeamNum == TEAM_A || m_iTeamNum == TEAM_B)
		return m_pTeamKitInfo->m_szFullTeamName;
	else
		return m_szKitName;
}

char *C_Team::Get_ShortTeamName( void )
{
	if (m_iTeamNum == TEAM_A || m_iTeamNum == TEAM_B)
		return m_pTeamKitInfo->m_szShortTeamName;
	else
		return m_szKitName;
}

char *C_Team::Get_KitName( void )
{
	return m_szKitName;
}

Color &C_Team::Get_HudKitColor()
{
	return m_pTeamKitInfo->m_HudKitColor;
}

Color &C_Team::Get_PrimaryKitColor()
{
	return m_pTeamKitInfo->m_PrimaryKitColor;
}

Color &C_Team::Get_SecondaryKitColor()
{
	return m_pTeamKitInfo->m_SecondaryKitColor;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_Team::Get_Goals( void )
{
	return m_nGoals;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_Team::Get_Ping( void )
{
	return m_iPing;
}

int C_Team::Get_Possession()
{
	return m_nPossession;
}

//-----------------------------------------------------------------------------
// Purpose: Return the number of players in this team
//-----------------------------------------------------------------------------
int C_Team::Get_Number_Players( void )
{
	return m_aPlayers.Size();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the specified player is on this team
//-----------------------------------------------------------------------------
bool C_Team::ContainsPlayer( int iPlayerIndex )
{
	for (int i = 0; i < m_aPlayers.Size(); i++ )
	{
		if ( m_aPlayers[i] == iPlayerIndex )
			return true;
	}

	return false;
}


void C_Team::ClientThink()
{
}

void C_Team::SetKitName(const char *pKitName)
{
	Q_strncpy(m_szKitName, pKitName, MAX_KITNAME_LENGTH);
	if (!Q_strcmp(pKitName, "Unassigned") || !Q_strcmp(pKitName, "Spectator"))
		return;

	TEAMKIT_FILE_INFO_HANDLE hKitHandle;
	if (ReadTeamKitDataFromFileForSlot(filesystem, pKitName, &hKitHandle))
	{
		//Q_strncpy(m_szKitName, pKitName, MAX_KITNAME_LENGTH);
		m_pTeamKitInfo = GetTeamKitInfoFromHandle(hKitHandle);
	}
	else
	{
		m_pTeamKitInfo = m_TeamKitInfoDatabase[0];
		Q_strncpy(m_szKitName, m_pTeamKitInfo->m_szKitName, MAX_KITNAME_LENGTH);
		DownloadTeamKit(pKitName, GetTeamNumber());
		//materials->ReloadTextures();
	}
}


//=================================================================================================
// GLOBAL CLIENT TEAM HANDLING
//=================================================================================================
// Purpose: Get the C_Team for the local player
//-----------------------------------------------------------------------------
C_Team *GetLocalTeam( void )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();

	if ( !player )
		return NULL;
	
	return GetPlayersTeam( player->index );
}

//-----------------------------------------------------------------------------
// Purpose: Get the C_Team for the specified team number
//-----------------------------------------------------------------------------
C_Team *GetGlobalTeam( int iTeamNumber )
{
	for (int i = 0; i < g_Teams.Count(); i++ )
	{
		if ( g_Teams[i]->GetTeamNumber() == iTeamNumber )
			return g_Teams[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the number of teams you can access via GetGlobalTeam() (hence the +1)
//-----------------------------------------------------------------------------
int GetNumTeams()
{
	return g_Teams.Count() + 1; 
}

//-----------------------------------------------------------------------------
// Purpose: Get the team of the specified player
//-----------------------------------------------------------------------------
C_Team *GetPlayersTeam( int iPlayerIndex )
{
	for (int i = 0; i < g_Teams.Count(); i++ )
	{
		if ( g_Teams[i]->ContainsPlayer( iPlayerIndex ) )
			return g_Teams[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get the team of the specified player
//-----------------------------------------------------------------------------
C_Team *GetPlayersTeam( C_BasePlayer *pPlayer )
{
	return GetPlayersTeam( pPlayer->entindex() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the two specified players are on the same team
//-----------------------------------------------------------------------------
bool ArePlayersOnSameTeam( int iPlayerIndex1, int iPlayerIndex2 )
{
	for (int i = 0; i < g_Teams.Count(); i++ )
	{
		if ( g_Teams[i]->ContainsPlayer( iPlayerIndex1 ) && g_Teams[i]->ContainsPlayer( iPlayerIndex2 ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of team managers
//-----------------------------------------------------------------------------
int GetNumberOfTeams( void )
{
	return g_Teams.Size();
}