#ifndef SDK_PLAYER_DATA_H
#define SDK_PLAYER_DATA_H
#pragma once

#include "cbase.h"
#include "sdk_shareddefs.h"
#include "ios_teamkit_parse.h"

class CSDKPlayer;

class CPlayerMatchData
{
public:
	int		m_nRedCards;
	int		m_nYellowCards;
	int		m_nFouls;
	int		m_nFoulsSuffered;
	int		m_nSlidingTackles;
	int		m_nSlidingTacklesCompleted;
	int		m_nGoalsConceded;
	int		m_nShots;
	int		m_nShotsOnGoal;
	int		m_nPassesCompleted;
	int		m_nInterceptions;
	int		m_nOffsides;
	int		m_nGoals;
	int		m_nOwnGoals;
	int		m_nAssists;
	float	m_flPossessionTime;
	int		m_nPossession;
	int		m_nDistanceCovered;
	int		m_nPasses;
	int		m_nFreeKicks;
	int		m_nPenalties;
	int		m_nCorners;
	int		m_nThrowIns;
	int		m_nKeeperSaves;
	int		m_nKeeperSavesCaught;
	int		m_nGoalKicks;
	int		m_nRating;
	float	m_flExactDistanceCovered;

	virtual void ResetData();
};

class CPlayerPeriodData : public CPlayerMatchData
{
public:
	int		m_nStartSecond;
	int		m_nEndSecond;
	int		m_nTeam;
	int		m_nTeamPosType;

	void ResetData();

	typedef CPlayerMatchData BaseClass;
};

class CPlayerData
{
protected:

	CTeam *GetTeam();
	CTeam *GetOppTeam();

public:

	CHandle<CSDKPlayer> m_pPl;
	unsigned long long m_nSteamCommunityID;
	char	m_szSteamID[32];
	char	m_szName[MAX_PLAYER_NAME_LENGTH];
	char	m_szShirtName[MAX_SHIRT_NAME_LENGTH];
	char	m_szShoeName[MAX_KITNAME_LENGTH];
	char	m_szKeeperGloveName[MAX_KITNAME_LENGTH];
	CPlayerMatchData *m_pMatchData;
	CUtlVector<CPlayerPeriodData *> m_PeriodData;
	int		m_nNextCardJoin;

	//	CPlayerData(const CSteamID *steamID);
	static void ReallocateAllPlayerData();
	static void ConvertAllPlayerDataToJson();
	static void AllocateData(CSDKPlayer *pPl);
	static CUtlVector<CPlayerData *> m_PlayerData;
	void ResetData();
	void StartNewMatchPeriod();
	void EndCurrentMatchPeriod();
	CPlayerData(CSDKPlayer *pPl);
	~CPlayerData();

	inline CPlayerMatchData			*GetMatchData() { return m_pMatchData; }
	inline CPlayerPeriodData	*GetPeriodData() { return m_PeriodData.Tail(); }

	int					GetRedCards(void) { return GetMatchData()->m_nRedCards; }
	void				AddRedCard();

	int					GetYellowCards(void) { return GetMatchData()->m_nYellowCards; }
	void				AddYellowCard();

	int					GetFouls(void) { return GetMatchData()->m_nFouls; }
	void				AddFoul();

	int					GetFoulsSuffered(void) { return GetMatchData()->m_nFoulsSuffered; }
	void				AddFoulSuffered();

	int					GetSlidingTackles(void) { return GetMatchData()->m_nSlidingTackles; }
	void				AddSlidingTackle();

	int					GetSlidingTacklesCompleted(void) { return GetMatchData()->m_nSlidingTacklesCompleted; }
	void				AddSlidingTackleCompleted();

	int					GetGoalsConceded(void) { return GetMatchData()->m_nGoalsConceded; }
	void				AddGoalConceded();

	int					GetShots(void) { return GetMatchData()->m_nShots; }
	void				AddShot();

	int					GetShotsOnGoal(void) { return GetMatchData()->m_nShotsOnGoal; }
	void				AddShotOnGoal();

	int					GetPassesCompleted(void) { return GetMatchData()->m_nPassesCompleted; }
	void				AddPassCompleted();

	int					GetInterceptions(void) { return GetMatchData()->m_nInterceptions; }
	void				AddInterception();

	int					GetOffsides(void) { return GetMatchData()->m_nOffsides; }
	void				AddOffside();

	int					GetGoals(void) { return GetMatchData()->m_nGoals; }
	void				AddGoal();

	int					GetOwnGoals(void) { return GetMatchData()->m_nOwnGoals; }
	void				AddOwnGoal();

	int					GetAssists(void) { return GetMatchData()->m_nAssists; }
	void				AddAssist();

	float				GetPossessionTime(void) { return GetMatchData()->m_flPossessionTime; }
	void				AddPossessionTime(float time);

	int					GetPossession(void) { return GetMatchData()->m_nPossession; }
	void				SetPossession(int amount) { GetMatchData()->m_nPossession = amount; }

	float				GetExactDistanceCovered(void) { return GetMatchData()->m_flExactDistanceCovered; }
	void				AddExactDistanceCovered(float amount);

	int					GetDistanceCovered(void) { return GetMatchData()->m_nDistanceCovered; }

	int					GetPasses(void) { return GetMatchData()->m_nPasses; }
	void				AddPass();

	int					GetFreeKicks(void) { return GetMatchData()->m_nFreeKicks; }
	void				AddFreeKick();

	int					GetPenalties(void) { return GetMatchData()->m_nPenalties; }
	void				AddPenalty();

	int					GetCorners(void) { return GetMatchData()->m_nCorners; }
	void				AddCorner();

	int					GetThrowIns(void) { return GetMatchData()->m_nThrowIns; }
	void				AddThrowIn();

	int					GetKeeperSaves(void) { return GetMatchData()->m_nKeeperSaves; }
	void				AddKeeperSave();

	int					GetKeeperSavesCaught(void) { return GetMatchData()->m_nKeeperSavesCaught; }
	void				AddKeeperSaveCaught();

	int					GetGoalKicks(void) { return GetMatchData()->m_nGoalKicks; }
	void				AddGoalKick();

	int					GetRating(void) { return GetMatchData()->m_nRating; }
	void				SetRating(int amount) { GetMatchData()->m_nRating = amount; }

	int					GetNextCardJoin(void) { return m_nNextCardJoin; }
	void				SetNextCardJoin(int seconds) { m_nNextCardJoin = seconds; }

	const char			*GetLastKnownName() { return m_szName; }
	void				SetLastKnownName(const char *name) { Q_strncpy(m_szName, name, MAX_PLAYER_NAME_LENGTH); }

	const char			*GetLastKnownShirtName() { return m_szShirtName; }
	void				SetLastKnownShirtName(const char *name) { Q_strncpy(m_szShirtName, name, MAX_SHIRT_NAME_LENGTH); }

	const char			*GetLastKnownShoeName() { return m_szShoeName; }
	void				SetLastKnownShoeName(const char *name) { Q_strncpy(m_szShoeName, name, MAX_KITNAME_LENGTH); }

	const char			*GetLastKnownKeeperGloveName() { return m_szKeeperGloveName; }
	void				SetLastKnownKeeperGloveName(const char *name) { Q_strncpy(m_szKeeperGloveName, name, MAX_KITNAME_LENGTH); }
};

#endif