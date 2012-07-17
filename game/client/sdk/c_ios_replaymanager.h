#ifndef c_IOS_REPLAYMANAGER_H
#define c_IOS_REPLAYMANAGER_H

#include "cbase.h"

class C_ReplayPlayer : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS(C_ReplayPlayer, CBaseAnimatingOverlay);
	DECLARE_CLIENTCLASS();

	int m_nTeamNumber;
	int m_nTeamPosition;
};

#endif