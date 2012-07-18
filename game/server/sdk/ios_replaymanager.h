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
	CSDKPlayer		*pPl;
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
	int				m_nTeamPosition;
	int				m_nSkin;
	int				m_nBody;
};

struct Snapshot
{
	float snaptime;
	BallSnapshot *pBallSnapshot;
	PlayerSnapshot *pPlayerSnapshot[22];
};


class CReplayBall : public CPhysicsProp
{
public:
	DECLARE_CLASS( CReplayBall, CPhysicsProp );

	typedef CPhysicsProp BaseClass;
	CReplayBall();


	bool CreateVPhysics( void );
	void Spawn( void );
	virtual void Precache();
	DECLARE_DATADESC();
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
	CNetworkVar(int, m_nTeamPosition);
};

class CReplayManager : public CBaseEntity
{
public:
	DECLARE_CLASS(CReplayManager, CBaseEntity);
	DECLARE_DATADESC();
	CReplayManager();
	void CheckReplay();
	void TakeSnapshot();
	void StartReplay(int numberOfRuns, float startDelay);
	void StopReplay();
	void RestoreSnapshot();
	bool IsReplaying() { return m_bDoReplay; }
	void Think();
	void Spawn();

private:
	CUtlVector<Snapshot>	m_Snapshots;
	bool					m_bDoReplay;
	int						m_nSnapshotIndex;
	CReplayBall				*m_pBall;
	CReplayPlayer			*m_pPlayers[32];
	int						m_nReplayRunCount;
	int						m_nMaxReplayRuns;
	float					m_flStartTime;
};

extern CReplayManager *g_pReplayManager;

inline CReplayManager *ReplayManager()
{
	return static_cast<CReplayManager *>(g_pReplayManager);
}

#endif