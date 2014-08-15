#ifndef C_MATCH_BALL_H
#define C_MATCH_BALL_H

#include "cbase.h"
#include "c_physicsprop.h"
#include "props_shared.h"
#include "c_props.h"
#include "c_sdk_player.h"
#include "c_ball.h"

class C_MatchBall : public C_Ball
{
public:

	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_MatchBall, C_Ball );

	C_MatchBall();
	~C_MatchBall();

	void OnDataChanged(DataUpdateType_t updateType);

	CHandle<C_SDKPlayer> m_pLastActivePlayer;
	int m_nLastActiveTeam;
	bool m_bNonnormalshotsBlocked;
	bool m_bShotsBlocked;
};

extern C_MatchBall *GetMatchBall();

#endif