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

	LayerRecord()
	{
		sequence = 0;
		cycle = 0;
		weight = 0;
		order = 0;
		playbackRate = 0;
	}

	LayerRecord( const LayerRecord& src )
	{
		sequence = src.sequence;
		cycle = src.cycle;
		weight = src.weight;
		order = src.order;
		playbackRate = src.playbackRate;
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
	bool isInReplay;
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
	match_state_t matchState;
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
		snapshots.PurgeAndDeleteElements();
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
	void StartReplay(int numberOfRuns, float startDelay, int index = -1, bool isHighlightReplay = false);
	void StopReplay();
	void RestoreSnapshot();
	bool IsReplaying() { return m_bIsReplaying; }
	void Think();
	void Spawn();
	int FindNextHighlightReplayIndex(int startIndex, match_state_t matchState);
	void StartHighlights();
	void StopHighlights();
	void CleanUp();
	void CalcReplayDuration(float startTime);
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