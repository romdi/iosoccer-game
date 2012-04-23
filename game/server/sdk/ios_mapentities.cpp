#include "cbase.h"
#include "triggers.h"
#include "sdk_gamerules.h"
#include "sdk_player.h"
#include "ball.h"
#include "ios_mapentities.h"
#include "team.h"

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
		if (pBall)
		{
			//m_OnTrigger.FireOutput(pOther, this);
			BallStartTouch(pBall);
		}

		CSDKPlayer *pPl = dynamic_cast<CSDKPlayer *>(pOther);
		if (pPl)
		{
			PlayerStartTouch(pPl);
		}
	};

	void EndTouch(CBaseEntity *pOther)
	{
		CBall *pBall = dynamic_cast<CBall *>(pOther);
		if (pBall)
		{
			//m_OnTrigger.FireOutput(pOther, this);
			BallEndTouch(pBall);
		}
	
		CSDKPlayer *pPl = dynamic_cast<CSDKPlayer *>(pOther);
		if (pPl)
		{
			PlayerEndTouch(pPl);
		}
	};
	virtual void BallStartTouch(CBall *pBall) = 0;
	virtual void BallEndTouch(CBall *pBall) {};
	virtual void PlayerStartTouch(CSDKPlayer *pPl) {};
	virtual void PlayerEndTouch(CSDKPlayer *pPl) {};
};

BEGIN_DATADESC( CBallTrigger )
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),
END_DATADESC()


class CTriggerGoal : public CBallTrigger
{
public:
	DECLARE_CLASS( CTriggerGoal, CBallTrigger );
	DECLARE_DATADESC();
	int	m_nTeam;

	void BallStartTouch(CBall *pBall)
	{
		if (pBall->IgnoreTriggers() || SDKGameRules()->IsIntermissionState())
			return;

		int team = m_nTeam == 1 ? SDKGameRules()->GetRightSideTeam() : SDKGameRules()->GetLeftSideTeam();
		pBall->TriggerGoal(team);
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
		if (pBall->IgnoreTriggers() || SDKGameRules()->IsIntermissionState())
			return;

		int team = m_nTeam == 1 ? SDKGameRules()->GetLeftSideTeam() : SDKGameRules()->GetRightSideTeam();
		pBall->TriggerGoalLine(team);
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
		if (pBall->IgnoreTriggers() || SDKGameRules()->IsIntermissionState())
			return;

		pBall->TriggerSideline();
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
	DECLARE_DATADESC();

	void BallStartTouch(CBall *pBall)
	{
		int team = m_nTeam == 1 ? SDKGameRules()->GetLeftSideTeam() : SDKGameRules()->GetRightSideTeam();
		pBall->TriggerPenaltyBox(team);
	};

	void BallEndTouch(CBall *pBall)
	{
		pBall->TriggerPenaltyBox(TEAM_INVALID);
	};

	void PlayerStartTouch(CSDKPlayer *pPl)
	{
		int team = m_nTeam == 1 ? SDKGameRules()->GetLeftSideTeam() : SDKGameRules()->GetRightSideTeam();
		pPl->m_nInPenBoxOfTeam = team;
	};

	void PlayerEndTouch(CSDKPlayer *pPl)
	{
		pPl->m_nInPenBoxOfTeam = TEAM_INVALID;
	};
};

BEGIN_DATADESC( CTriggerPenaltyBox )
	DEFINE_KEYFIELD( m_nTeam, FIELD_INTEGER, "Team" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_PenaltyBox, CTriggerPenaltyBox );

extern CBall *CreateBall (const Vector &pos, CSDKPlayer *pOwner);

class CBallStart : public CPointEntity //IOS
{
public:
	DECLARE_CLASS( CBallStart, CPointEntity );
	void Spawn(void)
	{
		CreateBall(GetLocalOrigin(), NULL);	
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