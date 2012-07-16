#ifndef IOS_REPLAYMANAGER_H
#define IOS_REPLAYMANAGER_H

#include "cbase.h"
#include "sdk_player.h"

struct BallSnapshot
{
	Vector pos;
	QAngle ang;
	Vector vel;
	AngularImpulse rot;

	BallSnapshot(Vector pos, QAngle ang, Vector vel, AngularImpulse rot) : pos(pos), ang(ang), vel(vel), rot(rot) {}
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
	CSDKPlayer *pPl;
	Vector pos;
	QAngle ang;
	Vector vel;
	LayerRecord				m_layerRecords[CBaseAnimatingOverlay::MAX_OVERLAYS];
	int						m_masterSequence;
	float					m_masterCycle;
	float					m_flSimulationTime;
	float					m_flMoveX;
	float					m_flMoveY;
	PlayerSnapshot(CSDKPlayer *pPl, Vector pos, QAngle ang, Vector vel) : pPl(pPl), pos(pos), ang(ang), vel(vel) {}
};

struct Snapshot
{
	float snaptime;
	BallSnapshot *pBallSnapshot;
	PlayerSnapshot *pPlayerSnapshot[32];
};

class CReplayManager : public CBaseEntity
{
public:
	DECLARE_CLASS(CReplayManager, CBaseEntity);
	DECLARE_DATADESC();
	CReplayManager();
	void CheckReplay();
	void TakeSnapshot();
	void StartReplay();
	void StopReplay();
	void RestoreSnapshot();
	bool IsReplaying() { return m_bDoReplay; }
	void Think();
	void Spawn();
private:
	CUtlVector<Snapshot>	m_Snapshots;
	bool					m_bDoReplay;
	int						m_nSnapshotIndex;
	CPhysicsProp			*m_pBall;
	CBaseAnimatingOverlay	*m_pPlayers[32];
};

extern CReplayManager *g_pReplayManager;

inline CReplayManager *ReplayManager()
{
	return static_cast<CReplayManager *>(g_pReplayManager);
}

#endif