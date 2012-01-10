#include "cbase.h"
#include "triggers.h"
#include "sdk_gamerules.h"
#include "sdk_player.h"
#include "ball.h"
#include "ios_mapentities.h"

class CBallTrigger : public CBaseTrigger
{
public:
	DECLARE_CLASS( CBallTrigger, CBaseTrigger );
	DECLARE_DATADESC();
	COutputEvent m_OnTrigger;

	void Spawn()
	{
		BaseClass::Spawn();
		InitTrigger();
	};
	void StartTouch( CBaseEntity *pOther )
	{
		CBall *pBall = dynamic_cast<CBall *>(pOther);
		if (pBall && !pBall->IgnoreTriggers())
		{
			m_OnTrigger.FireOutput(pOther, this);
			BallStartTouch(pBall);
		}
	};
	virtual void BallStartTouch(CBall *pBall) = 0;
};

BEGIN_DATADESC( CBallTrigger )
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),
END_DATADESC()

const static float CELEBRATION_TIME = 10.0f;

class CTriggerGoal : public CBallTrigger
{
public:
	DECLARE_CLASS( CTriggerGoal, CBallTrigger );
	DECLARE_DATADESC();
	int	m_nTeam;

	void BallStartTouch(CBall *pBall)
	{
		pBall->TriggerGoal(m_nTeam == 1 ? TEAM_A : TEAM_B);
	};
};

BEGIN_DATADESC( CTriggerGoal )
	DEFINE_KEYFIELD( m_nTeam, FIELD_INTEGER, "Team" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_goal, CTriggerGoal );


class CTriggerGoalLine : public CBallTrigger
{
public:
	DECLARE_CLASS( CTriggerGoalLine, CBallTrigger );
	DECLARE_DATADESC();
	int	m_nTeam;
	int	m_nSide;

	void BallStartTouch(CBall *pBall)
	{
		pBall->TriggerGoalline(m_nTeam == 1 ? TEAM_A : TEAM_B, m_nSide);
	};
};

BEGIN_DATADESC( CTriggerGoalLine )
	DEFINE_KEYFIELD( m_nTeam, FIELD_INTEGER, "Team" ),
	DEFINE_KEYFIELD( m_nSide, FIELD_INTEGER, "Side" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_GoalLine, CTriggerGoalLine );


class CTriggerSideLine : public CBallTrigger
{
public:
	DECLARE_CLASS( CTriggerSideLine, CBallTrigger );
	DECLARE_DATADESC();
	int	m_nSide;

	void BallStartTouch(CBall *pBall)
	{
		pBall->TriggerSideline(m_nSide);
	};
};

BEGIN_DATADESC( CTriggerSideLine )
	DEFINE_KEYFIELD( m_nSide, FIELD_INTEGER, "Side" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_SideLine, CTriggerSideLine );


class CTriggerPenaltyBox : public CBallTrigger
{
public:
	DECLARE_CLASS( CTriggerPenaltyBox, CBallTrigger );

	int	m_nTeam;
	int	m_nSide;
	DECLARE_DATADESC();

	void BallStartTouch(CBall *pBall) {};
};

BEGIN_DATADESC( CTriggerPenaltyBox )
	DEFINE_KEYFIELD( m_nTeam, FIELD_INTEGER, "Team" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_PenaltyBox, CTriggerPenaltyBox );

extern CBaseEntity *CreateBall (const Vector &pos);

class CBallStart : public CPointEntity //IOS
{
public:
	DECLARE_CLASS( CBallStart, CPointEntity );
	void Spawn(void)
	{
		CreateBall (GetLocalOrigin());	
	};
};

//ios entities
LINK_ENTITY_TO_CLASS(info_ball_start, CBallStart);	//IOS

LINK_ENTITY_TO_CLASS(info_team1_player1, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player2, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player3, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player4, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player5, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player6, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player7, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player8, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player9, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player10, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player11, CPointEntity);

LINK_ENTITY_TO_CLASS(info_team2_player1, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player2, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player3, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player4, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player5, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player6, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player7, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player8, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player9, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player10, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player11, CPointEntity);

LINK_ENTITY_TO_CLASS(info_team1_goalkick0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_goalkick1, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_goalkick0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_goalkick1, CPointEntity);

LINK_ENTITY_TO_CLASS(info_team1_corner0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_corner1, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_corner0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_corner1, CPointEntity);

LINK_ENTITY_TO_CLASS(info_team1_corner_player0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_corner_player1, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_corner_player0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_corner_player1, CPointEntity);

LINK_ENTITY_TO_CLASS(info_throw_in, CPointEntity);

LINK_ENTITY_TO_CLASS(info_team1_penalty_spot, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_penalty_spot, CPointEntity);

LINK_ENTITY_TO_CLASS(info_stadium, CPointEntity);

CTeamSpots	*g_pTeamSpots[2];
Vector		g_vKickOffSpot;
float		g_flGroundZ;

Vector GetSpotPos(const char *name)
{
	CBaseEntity *pEnt = gEntList.FindEntityByClassname(NULL, name);
	if (pEnt)
		return Vector(pEnt->GetLocalOrigin().x, pEnt->GetLocalOrigin().y, g_flGroundZ);
	else
		return vec3_origin;
}

void InitMapSpots()
{
	CBaseEntity *pEnt = gEntList.FindEntityByClassname(NULL, "info_ball_start");
	trace_t tr;
	UTIL_TraceLine(pEnt->GetLocalOrigin(), Vector(0, 0, -500), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr);
	g_flGroundZ = tr.endpos.z;

	g_vKickOffSpot = GetSpotPos("info_ball_start");

	for (int i = 0; i < 2; i++)
	{
		CTeamSpots *pSpot = new CTeamSpots();
		pSpot->m_vCornerLeft = GetSpotPos(UTIL_VarArgs("info_team%d_corner1", i + 1));
		pSpot->m_vCornerRight = GetSpotPos(UTIL_VarArgs("info_team%d_corner0", i + 1));
		pSpot->m_vGoalkickLeft = GetSpotPos(UTIL_VarArgs("info_team%d_goalkick1", i + 1));
		pSpot->m_vGoalkickRight = GetSpotPos(UTIL_VarArgs("info_team%d_goalkick0", i + 1));
		pSpot->m_vPenalty = GetSpotPos(UTIL_VarArgs("info_team%d_penalty_spot", i + 1));

		for (int j = 0; j < 11; j++)
		{
			pSpot->m_vPlayers[j] = GetSpotPos(UTIL_VarArgs("info_team%d_player%d", i + 1, j + 1));
		}

		pSpot->m_nForward = (g_vKickOffSpot - pSpot->m_vPlayers[0]).y > 0 ? 1 : -1;
		pSpot->m_nBack = -pSpot->m_nForward;
		pSpot->m_nRight = (pSpot->m_vCornerRight - pSpot->m_vPlayers[0]).x > 0 ? 1 : -1;
		pSpot->m_nLeft = -pSpot->m_nRight;

		g_pTeamSpots[i] = pSpot;
	}
}

CTeamSpots *GetOwnTeamSpots(CSDKPlayer *pPl)
{
	return g_pTeamSpots[pPl->GetTeamNumber() - TEAM_A];
}

CTeamSpots *GetOppTeamSpots(CSDKPlayer *pPl)
{
	return g_pTeamSpots[TEAM_B - pPl->GetTeamNumber()];
}