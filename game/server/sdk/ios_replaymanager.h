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

struct PlayerSnapshot
{
	CSDKPlayer *pPl;
	Vector pos;
	QAngle ang;
	Vector vel;
	//animation
	PlayerSnapshot(CSDKPlayer *pPl, Vector pos, QAngle ang, Vector vel) : pPl(pPl), pos(pos), ang(ang), vel(vel) {}
};

struct Snapshot
{
	float snaptime;
	BallSnapshot *pBallSnapshot;
	PlayerSnapshot *pPlayerSnapshot[32];
};

class CReplayManager
{
private:
	static CReplayManager	*m_pInstance;
	CReplayManager();

	CUtlVector<Snapshot>	m_Snapshots;
	bool					m_bDoReplay;
	int						m_nSnapshotIndex;

public:
	static CReplayManager *GetInstance();
	void CheckReplay();
	void TakeSnapshot();
	void StartReplay();
	void RestoreSnapshot();
};

#endif