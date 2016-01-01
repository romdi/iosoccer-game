//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side CTeam class
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_team.h"
#include "sdk_gamerules.h"
#include "hud_basechat.h"

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
	RecvPropInt( RECVINFO(m_nPenaltyGoals) ),
	RecvPropInt( RECVINFO(m_nPenaltyGoalBits) ),
	RecvPropInt( RECVINFO(m_nPenaltyRound) ),
	RecvPropInt( RECVINFO(m_nTimeoutsLeft) ),
	RecvPropInt( RECVINFO(m_nTimeoutTimeLeft) ),
	RecvPropString( RECVINFO(m_szServerKitName), 0, RecvProxy_KitName),
	RecvPropString( RECVINFO(m_szServerCode) ),
	RecvPropString( RECVINFO(m_szServerShortName) ),

	RecvPropVector(RECVINFO(m_vCornerLeft)),
	RecvPropVector(RECVINFO(m_vCornerRight)),
	RecvPropVector(RECVINFO(m_vGoalkickLeft)),
	RecvPropVector(RECVINFO(m_vGoalkickRight)),
	RecvPropVector(RECVINFO(m_vPenalty)),
	RecvPropVector(RECVINFO(m_vGoalCenter)),
	RecvPropVector(RECVINFO(m_vPenBoxMin)),
	RecvPropVector(RECVINFO(m_vPenBoxMax)),
	RecvPropVector(RECVINFO(m_vSixYardBoxMin)),
	RecvPropVector(RECVINFO(m_vSixYardBoxMax)),
	RecvPropInt(RECVINFO(m_nForward)),
	RecvPropInt(RECVINFO(m_nRight)),
	RecvPropIntWithMinusOneFlag(RECVINFO(m_nCaptainPosIndex)),
	
	RecvPropArray2( 
		RecvProxyArrayLength_PlayerArray,
		RecvPropInt( "player_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_PlayerList ), 
		MAX_PLAYERS, 
		0, 
		"player_array"
		),

	RecvPropArray3( RECVINFO_ARRAY(m_szMatchEventPlayers), RecvPropString( RECVINFO(m_szMatchEventPlayers[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_eMatchEventTypes), RecvPropInt( RECVINFO(m_eMatchEventTypes[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_eMatchEventMatchPeriods), RecvPropInt( RECVINFO(m_eMatchEventMatchPeriods[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_nMatchEventSeconds), RecvPropInt( RECVINFO(m_nMatchEventSeconds[0]))),

	RecvPropArray3( RECVINFO_ARRAY(m_PosNextJoinSeconds), RecvPropInt( RECVINFO(m_PosNextJoinSeconds[0]))),

	RecvPropInt(RECVINFO(m_RedCards)),
	RecvPropInt(RECVINFO(m_YellowCards)),
	RecvPropInt(RECVINFO(m_Fouls)),
	RecvPropInt(RECVINFO(m_FoulsSuffered)),
	RecvPropInt(RECVINFO(m_SlidingTackles)),
	RecvPropInt(RECVINFO(m_SlidingTacklesCompleted)),
	RecvPropInt(RECVINFO(m_GoalsConceded)),
	RecvPropInt(RECVINFO(m_Shots)),
	RecvPropInt(RECVINFO(m_ShotsOnGoal)),
	RecvPropInt(RECVINFO(m_PassesCompleted)),
	RecvPropInt(RECVINFO(m_Interceptions)),
	RecvPropInt(RECVINFO(m_Offsides)),
	RecvPropInt(RECVINFO(m_Goals)),
	RecvPropInt(RECVINFO(m_OwnGoals)),
	RecvPropInt(RECVINFO(m_Assists)),
	RecvPropInt(RECVINFO(m_Possession)),
	RecvPropInt(RECVINFO(m_DistanceCovered)),
	RecvPropInt(RECVINFO(m_Passes)),
	RecvPropInt(RECVINFO(m_FreeKicks)),
	RecvPropInt(RECVINFO(m_Penalties)),
	RecvPropInt(RECVINFO(m_Corners)),
	RecvPropInt(RECVINFO(m_ThrowIns)),
	RecvPropInt(RECVINFO(m_KeeperSaves)),
	RecvPropInt(RECVINFO(m_KeeperSavesCaught)),
	RecvPropInt(RECVINFO(m_GoalKicks)),
	RecvPropInt(RECVINFO(m_Ping)),
	RecvPropInt(RECVINFO(m_Rating)),

	RecvPropInt(RECVINFO(m_nFormationIndex)),
	RecvPropInt(RECVINFO(m_nOffensiveLevel)),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_Team )
	DEFINE_PRED_FIELD( m_iTeamNum, FIELD_INTEGER, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_szServerKitName, FIELD_CHARACTER, MAX_TEAM_NAME_LENGTH, FTYPEDESC_PRIVATE ),
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
	m_bKitDownloadFinished = false;

	m_Goals = 0;
	m_Possession = 0;
	m_szServerKitName[0] = 0;
	m_szDownloadKitName[0] = 0;

	m_szServerCode[0] = 0;
	m_szServerShortName[0] = 0;

	m_iPing = 0;
	m_iPacketloss = 0;

	m_nFormationIndex = 0;

	m_nOffensiveLevel = 0;

	for (int i = 0; i < MAX_MATCH_EVENTS; i++)
	{
		memset(m_szMatchEventPlayers, 0, sizeof(m_szMatchEventPlayers));
		memset(m_eMatchEventTypes, 0, sizeof(m_eMatchEventTypes));
		memset(m_eMatchEventMatchPeriods, 0, sizeof(m_eMatchEventMatchPeriods));
		memset(m_nMatchEventSeconds, 0, sizeof(m_nMatchEventSeconds));

		memset(m_PosNextJoinSeconds, 0, sizeof(m_PosNextJoinSeconds));
	}

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

void C_Team::Spawn()
{
	BaseClass::Spawn();

	SetNextClientThink(CLIENT_THINK_ALWAYS);
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
	if (m_iTeamNum != TEAM_HOME && m_iTeamNum != TEAM_AWAY)
		return m_iTeamNum;

	return m_iTeamNum == TEAM_HOME ? TEAM_AWAY : TEAM_HOME;
}

C_Team *C_Team::GetOppTeam( void ) const
{
	if (m_iTeamNum != TEAM_HOME && m_iTeamNum != TEAM_AWAY)
		return GetGlobalTeam(m_iTeamNum);

	return m_iTeamNum == TEAM_HOME ? GetGlobalTeam(TEAM_AWAY) : GetGlobalTeam(TEAM_HOME);
}

bool C_Team::IsClub( void )
{
	return m_pKitInfo->m_pTeamInfo->m_bIsClub;
}

bool C_Team::IsReal( void )
{
	return m_pKitInfo->m_pTeamInfo->m_bIsReal;
}

bool C_Team::HasCrest( void )
{
	return m_pKitInfo->m_pTeamInfo->m_bHasCrest;
}

char *C_Team::GetCode( void )
{
	if (m_iTeamNum == TEAM_HOME || m_iTeamNum == TEAM_AWAY)
	{
		if (m_szServerCode[0] != 0)
			return m_szServerCode;
		else
			return m_pKitInfo->m_pTeamInfo->m_szCode;
	}
	else
		return "";
}

char *C_Team::GetFullName( void )
{
	if (m_iTeamNum == TEAM_HOME || m_iTeamNum == TEAM_AWAY)
		return m_pKitInfo->m_pTeamInfo->m_szFullName;
	else
		return "";
}

char *C_Team::GetShortName( void )
{
	if (m_iTeamNum == TEAM_HOME || m_iTeamNum == TEAM_AWAY)
	{
		if (m_szServerShortName[0] != 0)
			return m_szServerShortName;
		else
			return m_pKitInfo->m_pTeamInfo->m_szShortName;
	}
	else
		return "";
}

char *C_Team::GetKitName( void )
{
	if (m_iTeamNum == TEAM_HOME || m_iTeamNum == TEAM_AWAY)
	{
		return m_pKitInfo->m_szName;
	}
	else
		return "";
}

char *C_Team::GetKitFolderName( void )
{
	if (m_iTeamNum == TEAM_HOME || m_iTeamNum == TEAM_AWAY)
	{
		return m_pKitInfo->m_szFolderName;
	}
	else
		return "";
}

char *C_Team::GetFolderName( void )
{
	if (m_iTeamNum == TEAM_HOME || m_iTeamNum == TEAM_AWAY)
	{
		return m_pKitInfo->m_pTeamInfo->m_szFolderName;
	}
	else
		return "";
}

Color &C_Team::GetHudKitColor()
{
	Color *color;

	if (GetTeamNumber() == TEAM_HOME || GetTeamNumber() == TEAM_AWAY)
		color = &m_pKitInfo->m_HudPrimaryColor;
	else
		color = &g_ColorWhite;

	if (GetTeamNumber() == TEAM_AWAY)
	{
		if (*color == GetGlobalTeam(TEAM_HOME)->GetHudKitColor())
		{
			color = &m_pKitInfo->m_HudSecondaryColor;

			if (*color == GetGlobalTeam(TEAM_HOME)->GetHudKitColor())
				color = &g_HudAlternativeColors[m_pKitInfo->m_HudPrimaryColorClass];
		}
	}

	return *color;
}

Color &C_Team::GetPrimaryKitColor()
{
	return m_pKitInfo->m_PrimaryColor;
}

Color &C_Team::GetSecondaryKitColor()
{
	return m_pKitInfo->m_SecondaryColor;
}

CTeamKitInfo *C_Team::GetKitInfo()
{
	return m_pKitInfo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_Team::GetGoals( void )
{
	return m_Goals;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_Team::GetPing( void )
{
	return m_iPing;
}

int C_Team::GetPossession()
{
	return m_Possession;
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

void ChatMsg(const char *format, ...)
{
	char buffer[256];
	va_list args;
	va_start (args, format);
	vsnprintf (buffer, 255, format, args);
	Msg(buffer);
	((CBaseHudChat *)gHUD.FindElement("CHudChat"))->Printf(CHAT_FILTER_NONE, buffer);	
	va_end (args);
}


void C_Team::SetKitName(const char *pKitName)
{
	Q_strncpy(m_szServerKitName, pKitName, MAX_KITNAME_LENGTH);

	if (m_pKitInfo && !Q_stricmp(m_szServerKitName, m_pKitInfo->m_szName) || !Q_stricmp(m_szServerKitName, m_szDownloadKitName))
		return;

	CTeamKitInfo *pKitInfo = CTeamInfo::FindTeamByKitName(m_szServerKitName);

	if (pKitInfo)
	{
		m_pKitInfo = pKitInfo;
	}
	else
	{
		if (GetTeamNumber() == TEAM_NONE || GetTeamNumber() == TEAM_SPECTATOR)
		{
			m_pKitInfo = CTeamInfo::m_TeamInfo[0]->m_TeamKitInfo[0];
		}
		else
		{
			ChatMsg("%s kit not found on disk. Try updating your files.\n", m_szServerKitName);
		}
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

Formation *C_Team::GetFormation()
{
	// FIXME: Clamping this to avoid problems when mp_maxplayers is lowered, since mp_maxplayers seems to get synced before other variables do
	return SDKGameRules()->GetFormations()[clamp(m_nFormationIndex, 0, SDKGameRules()->GetFormations().Count() - 1)];
}

int C_Team::GetOffensiveLevel()
{
	return m_nOffensiveLevel;
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