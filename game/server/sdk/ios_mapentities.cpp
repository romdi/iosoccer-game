#include "cbase.h"
#include "triggers.h"
#include "sdk_gamerules.h"
#include "sdk_player.h"
#include "ball.h"

class CTriggerGoal : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerGoal, CBaseTrigger );

	void Spawn( void );
	void StartTouch( CBaseEntity *pOther );
	//void Touch( CBaseEntity *pOther );
	//void Untouch( CBaseEntity *pOther );

	int	m_iTeam;

	DECLARE_DATADESC();

	// Outputs
	COutputEvent m_OnTrigger;
};

BEGIN_DATADESC( CTriggerGoal )
	DEFINE_KEYFIELD( m_iTeam, FIELD_INTEGER, "Team" ),
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_goal, CTriggerGoal );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerGoal::Spawn( )
{
	BaseClass::Spawn();
	InitTrigger();
}

const static float CELEBRATION_TIME = 10.0f;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CTriggerGoal::StartTouch( CBaseEntity *pOther )
//void CTriggerGoal::Touch( CBaseEntity *pOther )
{
	//if (!PassesTriggerFilters(pOther))
	//	return;

	//if (dynamic_cast<CBall *>(pOther))
	if (FClassnameIs( pOther, "football" ))
	{
		m_OnTrigger.FireOutput(pOther, this);

		CBall	*pBall = (CBall*)pOther;

		if (pBall->ballStatus==0)
		{
			//if the keeper is carrying the ball, only award a goal if they hold it in the net for a while
			//stops a bug where the ball is in the net on the same frame they catch?
			//also stops silly mistakes where keeper backs over the line accidentally
			//if (pBall->m_KeeperCarrying)
			//{
			//	m_KeeperGoalFrameCount++;
			//	if (m_KeeperGoalFrameCount < 60)
			//		return;
			//}

			//m_KeeperGoalFrameCount = 0;

			//nb: the teams goal trigger is at the other teams end, so team1 goal means
			//team1 has scored into oppositions net. map must be setup like this.
			if (m_iTeam==1)
			{
				pBall->m_TeamGoal = TEAM_A;
				pBall->m_KickOff = TEAM_B;
				//SDKGameRules()->m_Team1Goal = true;
			}
			else if (m_iTeam==2)
			{
				pBall->m_TeamGoal = TEAM_B;
				pBall->m_KickOff = TEAM_A;
				//SDKGameRules()->m_Team2Goal = true;
			}
			//Warning( "GOAL For Team %d!\n", m_iTeam );

			pBall->ballStatus = BALL_GOAL;		//flag goal occured so corners etc get ignored
			//pBall->ballStatusTime = gpGlobals->curtime + CELEBRATION_TIME;
			//pBall->SendMainStatus();
		}
	}
}




///////////////////////////////////////////////////////////////////////////////////////////////
//
// Goal Line Trigger -> Goal Kick or Corner
//
///////////////////////////////////////////////////////////////////////////////////////////////
class CTriggerGoalLine : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerGoalLine, CBaseTrigger );

	void Spawn( void );
	void StartTouch( CBaseEntity *pOther );
	int	m_iTeam;
	int	m_iSide;
	DECLARE_DATADESC();

	// Outputs
	COutputEvent m_OnTrigger;
};

BEGIN_DATADESC( CTriggerGoalLine )
	DEFINE_KEYFIELD( m_iTeam, FIELD_INTEGER, "Team" ),
	DEFINE_KEYFIELD( m_iSide, FIELD_INTEGER, "Side" ),
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_GoalLine, CTriggerGoalLine );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerGoalLine::Spawn( )
{
	BaseClass::Spawn();
	InitTrigger();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CTriggerGoalLine::StartTouch( CBaseEntity *pOther )
{
	if (FClassnameIs(pOther, "football") && ((CBall*)pOther)->ballStatus==0) 
	{
		CBall	*pBall = (CBall*)pOther;

		m_OnTrigger.FireOutput(pOther, this);

		//give to other team (team who didnt kick it out)
		if (pBall->m_LastTouch->GetTeamNumber() == TEAM_A)
			pBall->m_team = TEAM_B;
		else
			pBall->m_team = TEAM_A;


		//n.b. map team numbers are 1 or 2, our teamnum is 2,3
		if (m_iTeam == pBall->m_LastTouch->GetTeamNumber() - (TEAM_A-1))
		{
			pBall->ballStatus = BALL_CORNER;
			pBall->m_Foulee = (CSDKPlayer*)((CBall*)pOther)->FindPlayerForAction (pBall->m_team,0);   //find who was nearest as soon as it went out (store in foulee)
		} 
		else 
		{
			pBall->ballStatus = BALL_GOALKICK;
		}
		pBall->ballStatusTime = gpGlobals->curtime + 2.0f;

		//side of goal mouth
		pBall->m_side = m_iSide;

		((CBall*)pOther)->SendMainStatus();
		//Warning( "CORNER For Team %d!\n", m_iTeam );
	}
}





///////////////////////////////////////////////////////////////////////////////////////////////
//
// Side Line Trigger -> Throw Ins
//
///////////////////////////////////////////////////////////////////////////////////////////////
class CTriggerSideLine : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerSideLine, CBaseTrigger );

	void Spawn( void );
	void StartTouch( CBaseEntity *pOther );
	int	m_iSide;
	DECLARE_DATADESC();

	// Outputs
	//COutputEvent m_OnTrigger;
};

BEGIN_DATADESC( CTriggerSideLine )
	DEFINE_KEYFIELD( m_iSide, FIELD_INTEGER, "Side" ),
	//DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_SideLine, CTriggerSideLine );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerSideLine::Spawn( )
{
	BaseClass::Spawn();
	InitTrigger();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CTriggerSideLine::StartTouch( CBaseEntity *pOther )
{
	if (FClassnameIs(pOther, "football") && ((CBall*)pOther)->ballStatus==0) 
	{
		//m_OnTrigger.FireOutput(pOther, this);

		//CBall	*pBall = (CBall*)pOther;

		CBall *pBall = dynamic_cast< CBall* >(pOther);

		if (!pBall)
		{
			Warning ("NOT A BALL!");
			return;
		}
    
		pBall->ballStatus = BALL_THROWIN;
		pBall->ballStatusTime = gpGlobals->curtime + 2.0f;

		//pBall->m_team = !pBall->m_LastTouch->GetTeamNumber();  //team who didnt kick it out
		if (pBall->m_LastTouch->GetTeamNumber() == TEAM_A)
			pBall->m_team = TEAM_B;
		else
			pBall->m_team = TEAM_A;

		pBall->m_side = m_iSide;
		pBall->m_FoulPos = pBall->GetAbsOrigin();							// ((CBall*)pOther)->pev->origin;      //record where ball went out at first
		pBall->m_Foulee = (CSDKPlayer*)((CBall*)pOther)->FindPlayerForAction (((CBall*)pOther)->m_team,0);   //find who was nearest as soon as it went out (store in foulee)

		((CBall*)pOther)->SendMainStatus();

		//Warning( "THROWIN For Team %d!\n", pBall->m_team );
	}
}





///////////////////////////////////////////////////////////////////////////////////////////////
//
// Penalty Box Trigger -> Penalties
//
///////////////////////////////////////////////////////////////////////////////////////////////
class CTriggerPenaltyBox : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerPenaltyBox, CBaseTrigger );

	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void StartTouch( CBaseEntity *pOther );
	int	m_iTeam;
	int	m_iSide;
	DECLARE_DATADESC();

	// Outputs
	COutputEvent m_OnTrigger;
};

BEGIN_DATADESC( CTriggerPenaltyBox )
	DEFINE_KEYFIELD( m_iTeam, FIELD_INTEGER, "Team" ),
	DEFINE_KEYFIELD( m_iSide, FIELD_INTEGER, "Side" ),
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_PenaltyBox, CTriggerPenaltyBox );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerPenaltyBox::Spawn( )
{
	BaseClass::Spawn();
	InitTrigger();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CTriggerPenaltyBox::Touch( CBaseEntity *pOther )
{
	if (FClassnameIs(pOther, "football")) 
	{
		CBall	*pBall = (CBall*)pOther; 
		//pBall->m_BallInPenaltyBox = !(m_iTeam-1);  //whos penalty box is it 1 or 2 (-1 means outside box)

		if (m_iTeam == 1)
			pBall->m_BallInPenaltyBox = TEAM_A;
		else
			pBall->m_BallInPenaltyBox = TEAM_B;
	}
	//else if (FClassnameIs(pOther, "player"))
	//{
	//	CSDKPlayer	*pPlayer = (CSDKPlayer*)pOther;

	//	if (m_iTeam == 1)
	//		pPlayer->m_BallInPenaltyBox = TEAM_A;
	//	else
	//		pPlayer->m_BallInPenaltyBox = TEAM_B;
	//}
}


//-----------------------------------------------------------------------------
// do ontrigger when ball first goes in pen area
//
//-----------------------------------------------------------------------------
void CTriggerPenaltyBox::StartTouch( CBaseEntity *pOther )
{
	if (FClassnameIs(pOther, "football")) 
	{
		m_OnTrigger.FireOutput(pOther, this);
	}
}



class CBaseBallStart : public CPointEntity //IOS
{
public:

	DECLARE_CLASS( CBaseBallStart, CPointEntity );

	void Spawn (void);

private:
};

extern CBaseEntity *CreateBall (const Vector &pos);

//Create ball entity on map startup - IOS
void CBaseBallStart::Spawn(void)
{

	Vector pos = GetAbsOrigin();
	Vector lpos = GetLocalOrigin();
	CreateBall (pos);
	
}


//ios entities
LINK_ENTITY_TO_CLASS(info_ball_start, CBaseBallStart);	//IOS

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

