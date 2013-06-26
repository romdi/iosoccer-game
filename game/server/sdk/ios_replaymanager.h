#ifndef IOS_REPLAYMANAGER_H
#define IOS_REPLAYMANAGER_H

#include "cbase.h"
#include "sdk_player.h"

struct BallSnapshot
{
	Vector			pos;
	QAngle			ang;
	Vector			vel;
	AngularImpulse	rot;
	int				skin;
	int				effects;
};

struct LayerRecord
{
	int sequence;
	float cycle;
	float weight;
	int order;
	float playbackRate;
	bool looping;
	bool sequenceFinished;
	int flags;
	int priority;
	Activity activity;
	float layerAnimtime;
	float blendIn;
	float blendOut;
	float prevCycle;
	float killDelay;
	float killRate;
	float lastAccess;
	float lastEventCheck;
	float layerFadeOuttime;

	LayerRecord()
	{
		sequence = 0;
		cycle = 0;
		weight = 0;
		order = 0;
		playbackRate = 0;
		looping = false;
		sequenceFinished = false;
		flags = 0;
		priority = 0;
		activity = ACT_INVALID;
		layerAnimtime = 0;
		blendIn = 0;
		blendOut = 0;
		prevCycle = 0;
		killDelay = 0;
		killRate = 0;
		lastAccess = 0;
		lastEventCheck = 0;
		layerFadeOuttime = 0;
	}

	LayerRecord( const LayerRecord& src )
	{
		sequence = src.sequence;
		cycle = src.cycle;
		weight = src.weight;
		order = src.order;
		playbackRate = src.playbackRate;
		looping = src.looping;
		sequenceFinished = src.sequenceFinished;
		flags = src.flags;
		priority = src.priority;
		activity = src.activity;
		layerAnimtime = src.layerAnimtime;
		blendIn = src.blendIn;
		blendOut = src.blendOut;
		prevCycle = src.prevCycle;
		killDelay = src.killDelay;
		killRate = src.killRate;
		lastAccess = src.lastAccess;
		lastEventCheck = src.lastEventCheck;
		layerFadeOuttime = src.layerFadeOuttime;
	}
};

struct PlayerSnapshot
{
	Vector			pos;
	QAngle			ang;
	Vector			vel;
	LayerRecord		layerRecords[NUM_LAYERS_WANTED];
	int				masterSequence;
	float			masterCycle;
	float			simulationTime;
	float			moveX;
	float			moveY;
	int				teamNumber;
	int				teamPosNum;
	int				skin;
	int				body;
	const CPlayerPersistentData *pPlayerData;
};

struct Snapshot
{
	float snaptime;
	int replayCount;
	BallSnapshot *pBallSnapshot;
	PlayerSnapshot *pPlayerSnapshot[2][11];

	~Snapshot()
	{
		delete pBallSnapshot;

		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 11; j++)
			{
				delete pPlayerSnapshot[i][j];
			}
		}
	}

};

struct MatchEvent
{
	match_period_t matchState;
	match_event_t matchEventType;
	int second;
	int team;
	bool atMinGoalPos;
	const CPlayerPersistentData *pPlayer1Data;
	const CPlayerPersistentData *pPlayer2Data;
	const CPlayerPersistentData *pPlayer3Data;
	CUtlVector<Snapshot *> snapshots;
	float snapshotEndTime;

	~MatchEvent()
	{
		while (snapshots.Count() > 0)
		{
			snapshots[0]->replayCount -= 1;

			if (snapshots[0]->replayCount == 0)
				delete snapshots[0];

			snapshots.Remove(0);
		}
	}
};

class CReplayBall : public CPhysicsProp
{
public:
	DECLARE_CLASS( CReplayBall, CPhysicsProp );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	typedef CPhysicsProp BaseClass;
	CReplayBall();

	bool CreateVPhysics( void );
	void Spawn( void );
	virtual void Precache();
};


class CReplayPlayer : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CReplayPlayer, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	typedef CBaseAnimatingOverlay BaseClass;
	CReplayPlayer();

	void Spawn( void );
	virtual void Precache();
	void Think();

	CNetworkVar(int, m_nTeamNumber);
	CNetworkVar(int, m_nTeamPosNum);
	CNetworkString(m_szPlayerName, MAX_PLAYER_NAME_LENGTH);
};

class CReplayManager : public CBaseEntity
{
public:
	DECLARE_CLASS(CReplayManager, CBaseEntity);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CReplayManager();
	~CReplayManager();
	void CheckReplay();
	void TakeSnapshot();
	void StartReplay(bool isHighlightReplay);
	void StopReplay();
	void RestoreSnapshot();
	bool IsReplaying() { return m_bIsReplaying; }
	void Think();
	void Spawn();
	int FindNextHighlightReplayIndex(int startIndex, match_period_t matchState);
	void StartHighlights();
	void StopHighlights();
	void CleanUp();
	void CalcMaxReplayRunsAndDuration(const MatchEvent *pMatchEvent, float startTime);
	void AddMatchEvent(match_event_t type, int team, CSDKPlayer *pPlayer1, CSDKPlayer *pPlayer2 = NULL, CSDKPlayer *pPlayer3 = NULL);
	int GetMatchEventCount() { return m_MatchEvents.Count(); }
	MatchEvent *GetMatchEvent(int index) { return m_MatchEvents[index]; }
	
	CNetworkVar(bool, m_bIsReplaying);
	CNetworkVar(int, m_nReplayRunIndex);
	CNetworkVar(bool, m_bAtMinGoalPos);

	int UpdateTransmitState()	// always send to all clients
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

private:
	CUtlVector<Snapshot *>	m_Snapshots;
	bool					m_bReplayIsPending;
	int						m_nReplayIndex;
	CReplayBall				*m_pBall;
	CReplayPlayer			*m_pPlayers[2][11];
	int						m_nMaxReplayRuns;
	float					m_flReplayActivationTime;
	float					m_flReplayStartTime;
	float					m_flReplayStartTimeOffset;
	bool					m_bIsHighlightReplay;
	bool					m_bIsReplayStart;
	bool					m_bIsHighlightStart;
	float					m_flRunDuration;
	CUtlVector<MatchEvent *> m_MatchEvents;
};

extern CReplayManager *g_pReplayManager;

inline CReplayManager *ReplayManager()
{
	return static_cast<CReplayManager *>(g_pReplayManager);
}

#endif