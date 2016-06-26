#include "cbase.h"
#include "player_data.h"
#include "sdk_player.h"
#include "team.h"
#include "Filesystem.h"
#include "ios_replaymanager.h"

CUtlVector<CPlayerData *> CPlayerData::m_PlayerData;

CPlayerData::CPlayerData(CSDKPlayer *pPl)
{
	m_pPl = pPl;
	m_nSteamCommunityID = engine->GetClientSteamID(pPl->edict()) ? engine->GetClientSteamID(pPl->edict())->ConvertToUint64() : 0;
	Q_strncpy(m_szSteamID, engine->GetPlayerNetworkIDString(pPl->edict()), 32);
	Q_strncpy(m_szName, pPl->GetPlayerName(), MAX_PLAYER_NAME_LENGTH);
	Q_strncpy(m_szShirtName, pPl->GetShirtName(), MAX_SHIRT_NAME_LENGTH);
	m_pMatchData = new CPlayerMatchData();
	ResetData();
}

CPlayerData::~CPlayerData()
{
	delete m_pMatchData;
	m_pMatchData = NULL;

	m_PeriodData.PurgeAndDeleteElements();
}

void CPlayerMatchData::ResetData()
{
	m_nRedCards = 0;
	m_nYellowCards = 0;
	m_nFouls = 0;
	m_nFoulsSuffered = 0;
	m_nSlidingTackles = 0;
	m_nSlidingTacklesCompleted = 0;
	m_nGoalsConceded = 0;
	m_nShots = 0;
	m_nShotsOnGoal = 0;
	m_nPassesCompleted = 0;
	m_nInterceptions = 0;
	m_nOffsides = 0;
	m_nGoals = 0;
	m_nOwnGoals = 0;
	m_nAssists = 0;
	m_nPasses = 0;
	m_nFreeKicks = 0;
	m_nPenalties = 0;
	m_nCorners = 0;
	m_nThrowIns = 0;
	m_nKeeperSaves = 0;
	m_nKeeperSavesCaught = 0;
	m_nGoalKicks = 0;
	m_nRating = 0;
	m_nPossession = 0;
	m_flPossessionTime = 0.0f;
	m_nDistanceCovered = 0;
	m_flExactDistanceCovered = 0.0f;
}

void CPlayerPeriodData::ResetData()
{
	BaseClass::ResetData();
}

void CPlayerData::ResetData()
{
	m_nNextCardJoin = 0;
	m_pMatchData->ResetData();
	m_PeriodData.PurgeAndDeleteElements();
}

void CPlayerData::AllocateData(CSDKPlayer *pPl)
{
	CPlayerData *data = NULL;
	unsigned long long steamCommunityID = engine->GetClientSteamID(pPl->edict()) ? engine->GetClientSteamID(pPl->edict())->ConvertToUint64() : 0;

	for (int i = 0; i < m_PlayerData.Count(); i++)
	{
		if (m_PlayerData[i]->m_nSteamCommunityID != steamCommunityID)
			continue;

		data = m_PlayerData[i];
		data->m_pPl = pPl;
		break;
	}

	if (!data)
	{
		data = new CPlayerData(pPl);
		m_PlayerData.AddToTail(data);
	}

	pPl->SetData(data);
}

void CPlayerData::ReallocateAllPlayerData()
{
	m_PlayerData.PurgeAndDeleteElements();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		AllocateData(pPl);
	}
}

void CPlayerData::StartNewMatchPeriod()
{
	CPlayerPeriodData *pPeriodData = new CPlayerPeriodData();

	pPeriodData->ResetData();
	pPeriodData->m_nStartSecond = SDKGameRules()->GetMatchDisplayTimeSeconds();
	pPeriodData->m_nEndSecond = -1;
	pPeriodData->m_nTeam = m_pPl->GetTeamNumber();
	pPeriodData->m_nTeamPosType = m_pPl->GetTeamPosType();
	m_PeriodData.AddToTail(pPeriodData);
}

void CPlayerData::EndCurrentMatchPeriod()
{
	if (m_PeriodData.Count() > 0 && m_PeriodData.Tail()->m_nEndSecond == -1)
	{
		m_PeriodData.Tail()->m_nEndSecond = SDKGameRules()->GetMatchDisplayTimeSeconds();
	}
}

ConVar sv_webserver_matchdata_url("sv_webserver_matchdata_url", "http://simrai.iosoccer.com/matches");
ConVar sv_webserver_matchdata_accesstoken("sv_webserver_matchdata_accesstoken", "");
ConVar sv_webserver_matchdata_enabled("sv_webserver_matchdata_enabled", "0");

static const int JSON_SIZE = 40 * 1024;

#include "curl/curl.h"

struct Curl_t
{
	char json[JSON_SIZE];
	char *memory;
	size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct Curl_t *mem = (struct Curl_t *)userp;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

unsigned CurlSendJSON(void *params)
{
	Curl_t *pVars = (Curl_t *)params;
	pVars->memory = (char *)malloc(1);
	pVars->size = 0;

	CURL *curl;
	CURLcode res;

	/* In windows, this will init the winsock stuff */
	res = curl_global_init(CURL_GLOBAL_DEFAULT);
	/* Check for errors */
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_global_init() failed: %s\n",
			curl_easy_strerror(res));

		delete pVars;

		return 1;
	}

	/* get a curl handle */
	curl = curl_easy_init();
	if (curl) {
		/* First set the URL that is about to receive our POST. */
		curl_easy_setopt(curl, CURLOPT_URL, sv_webserver_matchdata_url.GetString());

		/* Now specify we want to POST data */
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pVars->json);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, pVars);
		/* we want to use our own read function */
		//curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

		/* pointer to pass to our read function */
		//curl_easy_setopt(curl, CURLOPT_READDATA, &pooh);

		/* get verbose debug output please */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "Accept: text/plain");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "charsets: utf-8");
		res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		/* use curl_slist_free_all() after the *perform() call to free this
		list again */

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		long code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
		/* Check for errors */
		if (res != CURLE_OK)
		{
			char msg[128];
			Q_snprintf(msg, sizeof(msg), "Couldn't submit match statistics to web server: cURL error code '%d'. Wrong web server URL or web server down?", res);
			UTIL_ClientPrintAll(HUD_PRINTTALK, msg);
		}
		else
		{
			if (code == 200)
			{
				char msg[128];
				Q_snprintf(msg, sizeof(msg), "Check out this match's statistics at %s/%s", sv_webserver_matchdata_url.GetString(), pVars->memory);
				UTIL_ClientPrintAll(HUD_PRINTTALK, msg);
			}
			else if (code == 401)
			{
				UTIL_ClientPrintAll(HUD_PRINTTALK, "Couldn't submit match statistics to web server: Invalid or revoked API token.");
			}
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();

	if (pVars->memory)
		free(pVars->memory);

	delete pVars;

	return 0;
}

void SendMatchDataToWebserver(const char *json)
{
	Curl_t *pVars = new Curl_t;
	memcpy(pVars->json, json, JSON_SIZE);
	pVars->json[Q_strlen(pVars->json) - 1] = 0;
	Q_strcat(pVars->json, UTIL_VarArgs(",\"access_token\":\"%s\"}", sv_webserver_matchdata_accesstoken.GetString()), JSON_SIZE);
	//Q_strncpy(pVars->json, "{\"foo\": \"bar\"}", JSON_SIZE);
	CreateSimpleThread(CurlSendJSON, pVars);
}

void CPlayerData::ConvertAllPlayerDataToJson()
{
	static const int STAT_TYPE_COUNT = 25;
	static const char statTypes[STAT_TYPE_COUNT][32] =
	{
		"redCards",
		"yellowCards",
		"fouls",
		"foulsSuffered",
		"slidingTackles",
		"slidingTacklesCompleted",
		"goalsConceded",
		"shots",
		"shotsOnGoal",
		"passesCompleted",
		"interceptions",
		"offsides",
		"goals",
		"ownGoals",
		"assists",
		"passes",
		"freeKicks",
		"penalties",
		"corners",
		"throwIns",
		"keeperSaves",
		"goalKicks",
		"possession",
		"distanceCovered",
		"keeperSavesCaught"
	};

	char *json = new char[JSON_SIZE];
	json[0] = 0;

	Q_strcat(json, "{\"matchData\":{", JSON_SIZE);

	Q_strcat(json, "\"statisticTypes\":[", JSON_SIZE);

	for (int i = 0; i < STAT_TYPE_COUNT; i++)
	{
		if (i > 0)
			Q_strcat(json, ",", JSON_SIZE);

		Q_strcat(json, UTIL_VarArgs("\"%s\"", statTypes[i]), JSON_SIZE);
	}

	Q_strcat(json, UTIL_VarArgs("],\"matchInfo\":{\"type\":\"%s\",\"startTime\":%lu,\"endTime\":%lu,\"periods\":%d,\"lastPeriodName\":\"%s\"},", mp_matchinfo.GetString(), SDKGameRules()->m_nRealMatchStartTime, SDKGameRules()->m_nRealMatchEndTime, GetGlobalTeam(TEAM_HOME)->m_PeriodData.Count(), GetGlobalTeam(TEAM_HOME)->m_PeriodData.Count() == 0 ? "<none>" : GetGlobalTeam(TEAM_HOME)->m_PeriodData[GetGlobalTeam(TEAM_HOME)->m_PeriodData.Count() - 1]->m_szMatchPeriodName), JSON_SIZE);

	Q_strcat(json, "\"teams\":[", JSON_SIZE);

	for (int team = TEAM_HOME; team <= TEAM_AWAY; team++)
	{
		CTeam *pTeam = GetGlobalTeam(team);

		if (team == TEAM_AWAY)
			Q_strcat(json, ",", JSON_SIZE);

		bool isMix = (pTeam->GetShortTeamName()[0] == 0);

		Q_strcat(json, "{\"matchTotal\":{", JSON_SIZE);

		Q_strcat(json, UTIL_VarArgs("\"name\":\"%s\",\"side\":\"%s\",\"isMix\":%s,", (isMix ? pTeam->GetKitName() : pTeam->GetShortTeamName()), (team == TEAM_HOME ? "home" : "away"), (isMix ? "true" : "false")), JSON_SIZE);

		Q_strcat(json, UTIL_VarArgs("\"statistics\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",
			pTeam->m_RedCards, pTeam->m_YellowCards, pTeam->m_Fouls, pTeam->m_FoulsSuffered, pTeam->m_SlidingTackles, pTeam->m_SlidingTacklesCompleted, pTeam->m_GoalsConceded, pTeam->m_Shots, pTeam->m_ShotsOnGoal, pTeam->m_PassesCompleted, pTeam->m_Interceptions, pTeam->m_Offsides, pTeam->m_Goals, pTeam->m_OwnGoals, pTeam->m_Assists, pTeam->m_Passes, pTeam->m_FreeKicks, pTeam->m_Penalties, pTeam->m_Corners, pTeam->m_ThrowIns, pTeam->m_KeeperSaves, pTeam->m_GoalKicks, (int)pTeam->m_flPossessionTime, (int)pTeam->m_flExactDistanceCovered, pTeam->m_KeeperSavesCaught
		), JSON_SIZE);

		Q_strcat(json, "},\"matchPeriods\":[", JSON_SIZE);

		for (int i = 0; i < pTeam->m_PeriodData.Count(); i++)
		{
			CTeamPeriodData *pPeriod = pTeam->m_PeriodData[i];

			if (i > 0)
				Q_strcat(json, ",", JSON_SIZE);

			Q_strcat(json, UTIL_VarArgs("{\"period\":%d,\"periodName\":\"%s\",\"announcedInjuryTimeSeconds\":%d,\"actualInjuryTimeSeconds\":%d", i + 1, pPeriod->m_szMatchPeriodName, pPeriod->m_nAnnouncedInjuryTimeSeconds, pPeriod->m_nActualInjuryTimeSeconds), JSON_SIZE);

			Q_strcat(json, UTIL_VarArgs(",\"statistics\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}",
				pPeriod->m_nRedCards, pPeriod->m_nYellowCards, pPeriod->m_nFouls, pPeriod->m_nFoulsSuffered, pPeriod->m_nSlidingTackles, pPeriod->m_nSlidingTacklesCompleted, pPeriod->m_nGoalsConceded, pPeriod->m_nShots, pPeriod->m_nShotsOnGoal, pPeriod->m_nPassesCompleted, pPeriod->m_nInterceptions, pPeriod->m_nOffsides, pPeriod->m_nGoals, pPeriod->m_nOwnGoals, pPeriod->m_nAssists, pPeriod->m_nPasses, pPeriod->m_nFreeKicks, pPeriod->m_nPenalties, pPeriod->m_nCorners, pPeriod->m_nThrowIns, pPeriod->m_nKeeperSaves, pPeriod->m_nGoalKicks, (int)pPeriod->m_flPossessionTime, (int)pPeriod->m_flExactDistanceCovered, pPeriod->m_nKeeperSavesCaught
			), JSON_SIZE);
		}

		Q_strcat(json, "]}", JSON_SIZE);
	}

	Q_strcat(json, "],\"players\":[", JSON_SIZE);

	int playersProcessed = 0;

	for (int i = 0; i < m_PlayerData.Count(); i++)
	{
		CPlayerData *pData = m_PlayerData[i];

		char playerName[MAX_PLAYER_NAME_LENGTH * 2] = {};

		int indexOffset = 0;

		for (int j = 0; j < Q_strlen(pData->m_szName); j++)
		{
			if (pData->m_szName[j] == '"' || pData->m_szName[j] == '\\')
			{
				playerName[j + indexOffset] = '\\';
				playerName[j + indexOffset + 1] = pData->m_szName[j];
				indexOffset += 1;
			}
			else
				playerName[j + indexOffset] = pData->m_szName[j];
		}

		if (playersProcessed > 0)
			Q_strcat(json, ",", JSON_SIZE);

		Q_strcat(json, "{", JSON_SIZE);

		Q_strcat(json, UTIL_VarArgs("\"info\":{\"steamId\":\"%s\",\"name\":\"%s\"}", pData->m_szSteamID, playerName), JSON_SIZE);

		Q_strcat(json, ",\"matchPeriodData\":[", JSON_SIZE);

		for (int j = 0; j < pData->m_PeriodData.Count(); j++)
		{
			CPlayerPeriodData *pMPData = pData->m_PeriodData[j];

			if (j > 0)
				Q_strcat(json, ",", JSON_SIZE);

			int startSecond = pMPData->m_nStartSecond;

			if (startSecond < 0)
			{
				DevMsg("Illegal 'start second' value: %d\n", startSecond);
				startSecond = 0;
			}

			int endSecond = pMPData->m_nEndSecond;

			if (endSecond == -1)
			{
				endSecond = SDKGameRules()->GetMatchDisplayTimeSeconds(true, false);
			}

			Q_strcat(json, UTIL_VarArgs("{\"info\":{\"startSecond\":%d,\"endSecond\":%d,\"team\":\"%s\",\"position\":\"%s\"}", startSecond, endSecond, (pMPData->m_nTeam == TEAM_HOME ? "home" : "away"), g_szPosNames[pMPData->m_nTeamPosType]), JSON_SIZE);

			Q_strcat(json, UTIL_VarArgs(",\"statistics\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}",
				pMPData->m_nRedCards, pMPData->m_nYellowCards, pMPData->m_nFouls, pMPData->m_nFoulsSuffered, pMPData->m_nSlidingTackles, pMPData->m_nSlidingTacklesCompleted, pMPData->m_nGoalsConceded, pMPData->m_nShots, pMPData->m_nShotsOnGoal, pMPData->m_nPassesCompleted, pMPData->m_nInterceptions, pMPData->m_nOffsides, pMPData->m_nGoals, pMPData->m_nOwnGoals, pMPData->m_nAssists, pMPData->m_nPasses, pMPData->m_nFreeKicks, pMPData->m_nPenalties, pMPData->m_nCorners, pMPData->m_nThrowIns, pMPData->m_nKeeperSaves, pMPData->m_nGoalKicks, (int)pMPData->m_flPossessionTime, (int)pMPData->m_flExactDistanceCovered, pMPData->m_nKeeperSavesCaught
			), JSON_SIZE);
		}

		Q_strcat(json, "]}", JSON_SIZE);

		playersProcessed += 1;
	}

	Q_strcat(json, "],\"matchEvents\":[", JSON_SIZE);

	int eventsProcessed = 0;

	for (int i = 0; i < ReplayManager()->GetMatchEventCount(); i++)
	{
		MatchEvent *pMatchEvent = ReplayManager()->GetMatchEvent(i);

		if (eventsProcessed > 0)
			Q_strcat(json, ",", JSON_SIZE);

		Q_strcat(json, UTIL_VarArgs("{\"second\":%d,\"event\":\"%s\",\"period\":\"%s\",\"team\":\"%s\",\"player1SteamId\":\"%s\",\"player2SteamId\":\"%s\",\"player3SteamId\":\"%s\"}", pMatchEvent->second, g_szMatchEventNames[pMatchEvent->matchEventType], g_szMatchPeriodNames[pMatchEvent->matchPeriod], (pMatchEvent->team == TEAM_HOME ? "home" : "away"), pMatchEvent->pPlayer1Data ? pMatchEvent->pPlayer1Data->m_szSteamID : "", pMatchEvent->pPlayer2Data ? pMatchEvent->pPlayer2Data->m_szSteamID : "", pMatchEvent->pPlayer3Data ? pMatchEvent->pPlayer3Data->m_szSteamID : ""), JSON_SIZE);

		eventsProcessed += 1;
	}

	Q_strcat(json, "]}}", JSON_SIZE);

	filesystem->CreateDirHierarchy("statistics", "MOD");

	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char teamNames[2][32];

	for (int team = TEAM_HOME; team <= TEAM_AWAY; team++)
	{
		Q_strncpy(teamNames[team - TEAM_HOME], (GetGlobalTeam(team)->GetTeamCode()[0] == 0 ? GetGlobalTeam(team)->GetKitName() : GetGlobalTeam(team)->GetTeamCode()), 32);
		char *c = teamNames[team - TEAM_HOME];

		while (*c != 0)
		{
			if (*c != '.' && *c != '_' && (*c < 48 || *c > 57 && *c < 65 || *c > 90 && *c < 97 || *c > 122))
				*c = '.';

			c++;
		}
	}

	if (sv_webserver_matchdata_enabled.GetBool())
	{
		SendMatchDataToWebserver(json);
	}

	char time[64];
	strftime(time, sizeof(time), "%Y.%m.%d_%Hh.%Mm.%Ss", timeinfo);

	const char *filename = UTIL_VarArgs("statistics\\%s_%s-vs-%s_%d-%d.json", time, teamNames[0], teamNames[1], GetGlobalTeam(TEAM_HOME)->GetGoals(), GetGlobalTeam(TEAM_AWAY)->GetGoals());

	FileHandle_t fh = filesystem->Open(filename, "w", "MOD");

	if (fh)
	{
		filesystem->Write(json, Q_strlen(json), fh);
		filesystem->Close(fh);

		Msg("Match data file '%s' written\n", filename);
	}

	delete[] json;
}

void CC_SV_SaveMatchData(const CCommand &args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	CPlayerData::ConvertAllPlayerDataToJson();
}

ConCommand sv_savematchdata("sv_savematchdata", CC_SV_SaveMatchData, "", 0);

CTeam *CPlayerData::GetTeam()
{
	return GetGlobalTeam(GetPeriodData()->m_nTeam);
}

CTeam *CPlayerData::GetOppTeam()
{
	return GetTeam()->GetOppTeam();
}

void CPlayerData::AddRedCard()
{
	GetPeriodData()->m_nRedCards += 1;
	GetMatchData()->m_nRedCards += 1;
	GetTeam()->m_RedCards += 1;
	GetTeam()->GetPeriodData()->m_nRedCards += 1;
}

void CPlayerData::AddYellowCard()
{
	GetPeriodData()->m_nYellowCards += 1;
	GetMatchData()->m_nYellowCards += 1;
	GetTeam()->m_YellowCards += 1;
	GetTeam()->GetPeriodData()->m_nYellowCards += 1;
}

void CPlayerData::AddFoul()
{
	GetPeriodData()->m_nFouls += 1;
	GetMatchData()->m_nFouls += 1;
	GetTeam()->m_Fouls += 1;
	GetTeam()->GetPeriodData()->m_nFouls += 1;
}

void CPlayerData::AddFoulSuffered()
{
	GetPeriodData()->m_nFoulsSuffered += 1;
	GetMatchData()->m_nFoulsSuffered += 1;
	GetTeam()->m_FoulsSuffered += 1;
	GetTeam()->GetPeriodData()->m_nFoulsSuffered += 1;
}

void CPlayerData::AddSlidingTackle()
{
	GetPeriodData()->m_nSlidingTackles += 1;
	GetMatchData()->m_nSlidingTackles += 1;
	GetTeam()->m_SlidingTackles += 1;
	GetTeam()->GetPeriodData()->m_nSlidingTackles += 1;
}

void CPlayerData::AddSlidingTackleCompleted()
{
	GetPeriodData()->m_nSlidingTacklesCompleted += 1;
	GetMatchData()->m_nSlidingTacklesCompleted += 1;
	GetTeam()->m_SlidingTacklesCompleted += 1;
	GetTeam()->GetPeriodData()->m_nSlidingTacklesCompleted += 1;
}

void CPlayerData::AddGoalConceded()
{
	GetPeriodData()->m_nGoalsConceded += 1;
	GetMatchData()->m_nGoalsConceded += 1;
	GetTeam()->m_GoalsConceded += 1;
	GetTeam()->GetPeriodData()->m_nGoalsConceded += 1;
}

void CPlayerData::AddShot()
{
	GetPeriodData()->m_nShots += 1;
	GetMatchData()->m_nShots += 1;
	GetTeam()->m_Shots += 1;
	GetTeam()->GetPeriodData()->m_nShots += 1;
}

void CPlayerData::AddShotOnGoal()
{
	GetPeriodData()->m_nShotsOnGoal += 1;
	GetMatchData()->m_nShotsOnGoal += 1;
	GetTeam()->m_ShotsOnGoal += 1;
	GetTeam()->GetPeriodData()->m_nShotsOnGoal += 1;
}

void CPlayerData::AddPassCompleted()
{
	GetPeriodData()->m_nPassesCompleted += 1;
	GetMatchData()->m_nPassesCompleted += 1;
	GetTeam()->m_PassesCompleted += 1;
	GetTeam()->GetPeriodData()->m_nPassesCompleted += 1;
}

void CPlayerData::AddInterception()
{
	GetPeriodData()->m_nInterceptions += 1;
	GetMatchData()->m_nInterceptions += 1;
	GetTeam()->m_Interceptions += 1;
	GetTeam()->GetPeriodData()->m_nInterceptions += 1;
}

void CPlayerData::AddOffside()
{
	GetPeriodData()->m_nOffsides += 1;
	GetMatchData()->m_nOffsides += 1;
	GetTeam()->m_Offsides += 1;
	GetTeam()->GetPeriodData()->m_nOffsides += 1;
}

void CPlayerData::AddGoal()
{
	GetPeriodData()->m_nGoals += 1;
	GetMatchData()->m_nGoals += 1;
	GetTeam()->m_Goals += 1;
	GetTeam()->GetPeriodData()->m_nGoals += 1;
}

void CPlayerData::AddOwnGoal()
{
	GetPeriodData()->m_nOwnGoals += 1;
	GetMatchData()->m_nOwnGoals += 1;

	GetTeam()->m_OwnGoals += 1;
	GetTeam()->GetPeriodData()->m_nOwnGoals += 1;

	GetOppTeam()->m_Goals += 1;
	GetOppTeam()->GetPeriodData()->m_nGoals += 1;
}

void CPlayerData::AddAssist()
{
	GetPeriodData()->m_nAssists += 1;
	GetMatchData()->m_nAssists += 1;
	GetTeam()->m_Assists += 1;
	GetTeam()->GetPeriodData()->m_nAssists += 1;
}

void CPlayerData::AddPossessionTime(float time)
{
	GetPeriodData()->m_flPossessionTime += time;
	GetMatchData()->m_flPossessionTime += time;
	GetTeam()->m_flPossessionTime += time;
	GetTeam()->GetPeriodData()->m_flPossessionTime += time;
}

void CPlayerData::AddExactDistanceCovered(float amount)
{
	GetPeriodData()->m_flExactDistanceCovered += amount;
	GetPeriodData()->m_nDistanceCovered = GetPeriodData()->m_flExactDistanceCovered * 10 / 1000;

	GetMatchData()->m_flExactDistanceCovered += amount;
	GetMatchData()->m_nDistanceCovered = GetMatchData()->m_flExactDistanceCovered * 10 / 1000;

	GetTeam()->m_flExactDistanceCovered += amount;
	GetTeam()->m_DistanceCovered = GetTeam()->m_flExactDistanceCovered * 10 / 1000;

	GetTeam()->GetPeriodData()->m_flExactDistanceCovered += amount;
	GetTeam()->GetPeriodData()->m_nDistanceCovered = GetTeam()->GetPeriodData()->m_flExactDistanceCovered * 10 / 1000;
}

void CPlayerData::AddPass()
{
	GetPeriodData()->m_nPasses += 1;
	GetMatchData()->m_nPasses += 1;
	GetTeam()->m_Passes += 1;
	GetTeam()->GetPeriodData()->m_nPasses += 1;
}

void CPlayerData::AddFreeKick()
{
	GetPeriodData()->m_nFreeKicks += 1;
	GetMatchData()->m_nFreeKicks += 1;
	GetTeam()->m_FreeKicks += 1;
	GetTeam()->GetPeriodData()->m_nFreeKicks += 1;
}

void CPlayerData::AddPenalty()
{
	GetPeriodData()->m_nPenalties += 1;
	GetMatchData()->m_nPenalties += 1;
	GetTeam()->m_Penalties += 1;
	GetTeam()->GetPeriodData()->m_nPenalties += 1;
}

void CPlayerData::AddCorner()
{
	GetPeriodData()->m_nCorners += 1;
	GetMatchData()->m_nCorners += 1;
	GetTeam()->m_Corners += 1;
	GetTeam()->GetPeriodData()->m_nCorners += 1;
}

void CPlayerData::AddThrowIn()
{
	GetPeriodData()->m_nThrowIns += 1;
	GetMatchData()->m_nThrowIns += 1;
	GetTeam()->m_ThrowIns += 1;
	GetTeam()->GetPeriodData()->m_nThrowIns += 1;
}

void CPlayerData::AddKeeperSave()
{
	GetPeriodData()->m_nKeeperSaves += 1;
	GetMatchData()->m_nKeeperSaves += 1;
	GetTeam()->m_KeeperSaves += 1;
	GetTeam()->GetPeriodData()->m_nKeeperSaves += 1;
}

void CPlayerData::AddKeeperSaveCaught()
{
	GetPeriodData()->m_nKeeperSavesCaught += 1;
	GetMatchData()->m_nKeeperSavesCaught += 1;
	GetTeam()->m_KeeperSavesCaught += 1;
	GetTeam()->GetPeriodData()->m_nKeeperSavesCaught += 1;
}

void CPlayerData::AddGoalKick()
{
	GetPeriodData()->m_nGoalKicks += 1;
	GetMatchData()->m_nGoalKicks += 1;
	GetTeam()->m_GoalKicks += 1;
	GetTeam()->GetPeriodData()->m_nGoalKicks += 1;
}
