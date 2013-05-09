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
	int m_sequence;
	float m_cycle;
	float m_weight;
	int m_order;
	float m_playbackrate;

	LayerRecord()
	{
		m_sequence = 0;
		m_cycle = 0;
		m_weight = 0;
		m_order = 0;
		m_playbackrate = 0;
	}

	LayerRecord( const LayerRecord& src )
	{
		m_sequence = src.m_sequence;
		m_cycle = src.m_cycle;
		m_weight = src.m_weight;
		m_order = src.m_order;
		m_playbackrate = src.m_playbackrate;
	}
};

struct PlayerSnapshot
{
	//CSDKPlayer		*pPl;
	Vector			pos;
	QAngle			ang;
	Vector			vel;
	CAnimationLayer m_animLayers[CBaseAnimatingOverlay::MAX_OVERLAYS];
	//LayerRecord		m_layerRecords[CBaseAnimatingOverlay::MAX_OVERLAYS];
	int				m_masterSequence;
	float			m_masterCycle;
	float			m_flSimulationTime;
	float			m_flMoveX;
	float			m_flMoveY;
	int				m_nTeamNumber;
	int				m_nTeamPosNum;
	int				m_nSkin;
	int				m_nBody;
	const CPlayerPersistentData *m_pPlayerData;
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

struct Replay
{
	CUtlVector<Snapshot *> m_Snapshots;
	int m_nMatchSeconds;
	bool m_bAtMinGoalPos;

	~Replay()
	{
		m_Snapshots.PurgeAndDeleteElements();
	}
};

struct MatchEvent
{
	match_state_t matchState;
	match_event_t matchEventType;
	int second;
	int team;
	bool atMinGoalPos;
	const CPlayerPersistentData *pPlayerData1;
	const CPlayerPersistentData *pPlayerData2;
	const CPlayerPersistentData *pPlayerData3;
	CUtlVector<Snapshot *> snapshots;

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
	CNetworkString(m_szPlayerName, MAX_PLACE_NAME_LENGTH);

	CAnimationLayer m_AnimLayers[2];
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

	CNetworkVar(bool, m_bIsReplaying);
	CNetworkVar(int, m_nReplayRunIndex);
	CNetworkVar(bool, m_bAtMinGoalPos);

	int UpdateTransmitState()	// always send to all clients
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

private:
	CUtlVector<Snapshot *>	m_Snapshots;
	bool					m_bDoReplay;
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