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
	RecvPropInt( RECVINFO(m_nPenaltyGoals) ),
	RecvPropInt( RECVINFO(m_nPenaltyGoalBits) ),
	RecvPropInt( RECVINFO(m_nPenaltyRound) ),
	RecvPropInt( RECVINFO(m_nTimeoutsLeft) ),
	RecvPropString( RECVINFO(m_szServerKitName), 0, RecvProxy_KitName),
	RecvPropString( RECVINFO(m_szTeamCode) ),
	RecvPropString( RECVINFO(m_szShortTeamName) ),

	RecvPropVector(RECVINFO(m_vCornerLeft)),
	RecvPropVector(RECVINFO(m_vCornerRight)),
	RecvPropVector(RECVINFO(m_vGoalkickLeft)),
	RecvPropVector(RECVINFO(m_vGoalkickRight)),
	RecvPropVector(RECVINFO(m_vPenalty)),
	RecvPropVector(RECVINFO(m_vPenBoxMin)),
	RecvPropVector(RECVINFO(m_vPenBoxMax)),
	RecvPropInt(RECVINFO(m_nForward)),
	RecvPropInt(RECVINFO(m_nRight)),
	RecvPropInt(RECVINFO(m_nCaptainPosIndex)),
	RecvPropInt(RECVINFO(m_nFreekickTakerPosIndex)),
	RecvPropInt(RECVINFO(m_nPenaltyTakerPosIndex)),
	RecvPropInt(RECVINFO(m_nLeftCornerTakerPosIndex)),
	RecvPropInt(RECVINFO(m_nRightCornerTakerPosIndex)),
	
	RecvPropArray2( 
		RecvProxyArrayLength_PlayerArray,
		RecvPropInt( "player_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_PlayerList ), 
		MAX_PLAYERS, 
		0, 
		"player_array"
		),

	RecvPropArray3( RECVINFO_ARRAY(m_szMatchEventPlayers), RecvPropString( RECVINFO(m_szMatchEventPlayers[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_eMatchEventTypes), RecvPropInt( RECVINFO(m_eMatchEventTypes[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_eMatchEventMatchStates), RecvPropInt( RECVINFO(m_eMatchEventMatchStates[0]))),
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
	RecvPropInt(RECVINFO(m_GoalKicks)),
	RecvPropInt(RECVINFO(m_Ping)),
	RecvPropInt(RECVINFO(m_Rating)),
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
	m_szKitName[0] = 0;
	m_szServerKitName[0] = 0;
	m_szDownloadKitName[0] = 0;

	m_szTeamCode[0] = 0;
	m_szShortTeamName[0] = 0;

	m_iPing = 0;
	m_iPacketloss = 0;

	for (int i = 0; i < MAX_MATCH_EVENTS; i++)
	{
		memset(m_szMatchEventPlayers, 0, sizeof(m_szMatchEventPlayers));
		memset(m_eMatchEventTypes, 0, sizeof(m_eMatchEventTypes));
		memset(m_eMatchEventMatchStates, 0, sizeof(m_eMatchEventMatchStates));
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
	if (m_iTeamNum == TEAM_A || m_iTeamNum == TEAM_B)
	{
		if (m_szTeamCode[0] != 0)
			return m_szTeamCode;
		else
			return m_pTeamKitInfo->m_szTeamCode;
	}
	else
		return "";
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
	{
		if (m_szShortTeamName[0] != 0)
			return m_szShortTeamName;
		else
			return m_pTeamKitInfo->m_szShortTeamName;
	}
	else
		return m_szKitName;
}

char *C_Team::Get_KitName( void )
{
	return m_szKitName;
}

Color &C_Team::Get_HudKitColor()
{
	if (GetTeamNumber() == TEAM_A)
	{
		static Color col = Color(220, 209, 180, 255);
		return col;
	}
	else if (GetTeamNumber() == TEAM_B)
	{
		static Color col = Color(197, 224, 220, 255);
		return col;
	}
	else
	{
		static Color col = Color(255, 255, 255, 255);
		return col;
	}

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
	return m_Goals;
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
	return m_Possession;
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
	if (m_bKitDownloadFinished)
	{
		m_bKitDownloadFinished = false;
		materials->ReloadTextures();
		TEAMKIT_FILE_INFO_HANDLE hKitHandle;
		if (ReadTeamKitDataFromFileForSlot(filesystem, m_szDownloadKitName, &hKitHandle))
		{
			Q_strncpy(m_szKitName, m_szDownloadKitName, MAX_KITNAME_LENGTH);
			m_pTeamKitInfo = GetTeamKitInfoFromHandle(hKitHandle);
		}
	}
}


void CC_ReloadTextures(const CCommand &args)
{
	materials->ReloadTextures();
}

static ConCommand reloadtextures("reloadtextures", CC_ReloadTextures);

#include "curl/curl.h"
#include "Filesystem.h"
#include "utlbuffer.h"
  
struct curl_t
{
	char kitName[32];
	int teamNumber;
	//CUtlBuffer buf;
	FileHandle_t fh;
};

// Called when curl receives data from the server
static size_t rcvData(void *ptr, size_t size, size_t nmemb, curl_t* vars)
{
	//Msg((char*)ptr); // up to 989 characters each time
	//CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
	//vars->buf.Put(ptr, nmemb);
	filesystem->Write(ptr, nmemb, vars->fh);
	//filesystem->WriteFile(VarArgs("materials/models/player_new/foobar/%s", vars->filename), "MOD", buf);
	return size * nmemb;
}

ConVar cl_download_url("cl_download_url", "http://downloads.iosoccer.co.uk/teamkits/");

#define KITFILECOUNT 15

#include "hud_basechat.h"

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

unsigned DoCurl( void *params )
{
	curl_t* vars = (curl_t*) params; // always use a struct!

	ChatMsg("Downloading %s kit files...\n", vars->kitName);

	const char *textures[KITFILECOUNT] = { "2.vtf", "3.vtf", "4.vtf", "5.vtf", "6.vtf", "7.vtf", "8.vtf", "9.vtf", "10.vtf", "11.vtf", "gksocks.vtf", "keeper.vtf", "socks.vtf", "teamcrest.vtf", "kitdata.txt" };

	CURL *curl;
	curl = curl_easy_init();
	char url[512];
	Q_snprintf(url, sizeof(url), "%s/%s/%s", cl_download_url.GetString(), vars->kitName, "kitdata.txt");
	curl_easy_setopt(curl, CURLOPT_URL, url);
	CURLcode result = curl_easy_perform(curl);
	long code;
	curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &code);
	curl_easy_cleanup(curl);

	if (code >= 400)
	{
		ChatMsg("%s kit not found on server. Wrong kit name?\n", vars->kitName);
		return -1;
	}

	char path[512];
	Q_snprintf(path, sizeof(path), "materials/models/player/teams/%s", vars->kitName);
	filesystem->CreateDirHierarchy(path, "MOD");

	for (int i = 0; i < KITFILECOUNT; i++)
	{
		char filename[512];
		Q_snprintf(filename, sizeof(filename), "materials/models/player/teams/%s/%s", vars->kitName, textures[i]);
		vars->fh = filesystem->Open(filename, "wb", "MOD");

		if (!vars->fh)
			continue;

		CURL *curl;
		curl = curl_easy_init();
		char url[512];
		Q_snprintf(url, sizeof(url), "%s/%s/%s", cl_download_url.GetString(), vars->kitName, textures[i]);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		//curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rcvData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, vars);
		CURLcode result = curl_easy_perform(curl);
		long code;
		curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &code);
		curl_easy_cleanup(curl);

		filesystem->Close(vars->fh);

		if (code >= 400)
		{
			ChatMsg("%s kit file %s not found on server\n", vars->kitName, textures[i]);
			filesystem->RemoveFile(filename, "MOD");
		}
		else
		{
			ChatMsg("Downloaded %s kit file %s (%d/%d)\n", vars->kitName, textures[i], i + 1, KITFILECOUNT);
		}
	}

	ChatMsg("Downloaded all %s kit files\nApplying %s kit...\n", vars->kitName, vars->kitName);

	//Q_strncpy(GetGlobalTeam(vars->teamNumber)->m_szDownloadKitName, vars->kitName, sizeof(GetGlobalTeam(vars->teamNumber)->m_szDownloadKitName));
	GetGlobalTeam(vars->teamNumber)->m_bKitDownloadFinished = true;

	// clean up the memory
	delete vars;

	return 0;
}

void C_Team::DownloadTeamKit(const char *pKitName, int teamNumber)
{
	curl_t* vars = new curl_t;
	Q_strncpy(vars->kitName, pKitName, sizeof(vars->kitName));
	vars->teamNumber = teamNumber;
	CreateSimpleThread( DoCurl, vars );
}


void C_Team::SetKitName(const char *pKitName)
{
	//if (!Q_strcmp(pKitName, "Unassigned") || !Q_strcmp(pKitName, "Spectator"))
	//	return;

	Q_strncpy(m_szServerKitName, pKitName, MAX_KITNAME_LENGTH);

	if (GetTeamNumber() == TEAM_INVALID || GetTeamNumber() == TEAM_UNASSIGNED || GetTeamNumber() == TEAM_SPECTATOR)
	{
		Q_strncpy(m_szKitName, m_szServerKitName, MAX_KITNAME_LENGTH);
		return;
	}

	if (!Q_stricmp(m_szServerKitName, m_szKitName) || !Q_stricmp(m_szServerKitName, m_szDownloadKitName))
		return;

	TEAMKIT_FILE_INFO_HANDLE hKitHandle;
	if (ReadTeamKitDataFromFileForSlot(filesystem, m_szServerKitName, &hKitHandle))
	{
		Q_strncpy(m_szKitName, m_szServerKitName, MAX_KITNAME_LENGTH);
		m_pTeamKitInfo = GetTeamKitInfoFromHandle(hKitHandle);
	}
	else
	{
		ChatMsg("%s kit not found on disk\n", m_szServerKitName);
		m_pTeamKitInfo = GetTeamKitInfoFromHandle(LookupTeamKitInfoSlot(GetTeamNumber() == TEAM_A ? "germany" : "brazil"));
		Q_strncpy(m_szKitName, m_pTeamKitInfo->m_szKitName, MAX_KITNAME_LENGTH);
		Q_strncpy(m_szDownloadKitName, m_szServerKitName, MAX_KITNAME_LENGTH);
		DownloadTeamKit(pKitName, GetTeamNumber());
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