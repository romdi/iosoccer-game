#ifndef c_IOS_REPLAYMANAGER_H
#define c_IOS_REPLAYMANAGER_H

#include "cbase.h"

class C_ReplayPlayer : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS(C_ReplayPlayer, CBaseAnimatingOverlay);
	DECLARE_CLIENTCLASS();

	int m_nTeamNumber;
	int m_nTeamPosNum;
};

class C_ReplayManager : public CBaseEntity
{
public:
	DECLARE_CLASS(C_ReplayManager, CBaseEntity);
	DECLARE_CLIENTCLASS();

	C_ReplayManager();
	~C_ReplayManager();
	bool IsReplaying();
	bool m_bIsReplaying;
};

extern C_ReplayManager *GetReplayManager();

#endif