
#ifndef _BALL_H
#define _BALL_H

#include "props_shared.h"
#include "props.h"
#include "sdk_player.h"
#include "in_buttons.h"
#include "beam_shared.h"


#define	ENTITY_MODEL	 "models/w_fb.mdl"

#define	BALL_ELSEWHERE	0
#define	BALL_NEAR_FEET	1
#define	BALL_NEAR_HEAD	2

#define	MAX_OFFS		16
#define	MAX_ASSIST		10

enum ball_state_t
{
	BALL_NOSTATE = -1,
	BALL_NORMAL = 0,
	BALL_GOAL,
	BALL_CORNER,
	BALL_GOALKICK,
	BALL_THROWIN,
	BALL_FOUL,
	BALL_KICKOFF,
	BALL_PENALTY,
	BALL_FREEKICK,
	BALL_GOALKICK_PENDING,
	BALL_CORNERKICK_PENDING,
	BALL_THROWIN_PENDING,
	BALL_FOUL_PENDING,
	BALL_PENALTY_PENDING,
	BALL_KICKOFF_PENDING
};

enum body_part_t
{
	BODY_NONE = -1,
	BODY_FEET = 0,
	BODY_CHEST,
	BODY_HEAD
};

enum foul_type_t
{
	FOUL_NONE = -1,
	FOUL_NORMAL = 0,
	FOUL_OFFSIDE,
	FOUL_DOUBLETOUCH,
	FOUL_TIMEWASTING
};

#define FL_POS_KEEPER					(1<<0)
#define FL_POS_DEFENDER					(1<<1)
#define FL_POS_MIDFIELDER				(1<<2)
#define FL_POS_ATTACKER					(1<<3)
#define FL_POS_FIELD					(FL_POS_DEFENDER | FL_POS_MIDFIELDER | FL_POS_ATTACKER)
#define FL_POS_ALL						(FL_POS_KEEPER | FL_POS_FIELD)

#define	PS_OFF							0
#define	PS_BESTOFFIVE					1
#define	PS_SUDDENDEATH					2
#define	PS_DONE							3

#define BALL_MAINSTATUS_FOUL			0
#define BALL_MAINSTATUS_YELLOW			1
#define BALL_MAINSTATUS_RED				2
#define BALL_MAINSTATUS_OFFSIDE			3
#define BALL_MAINSTATUS_PLAYON			4
#define BALL_MAINSTATUS_PENALTY			5
#define BALL_MAINSTATUS_FINAL_WHISTLE	6

class CBall;

extern CBall *GetBall();

struct CBallStateInfo
{
	ball_state_t			m_eBallState;
	const char				*m_pStateName;

	void (CBall::*pfnEnterState)();	// Init and deinit the state.
	void (CBall::*pfnLeaveState)();
	void (CBall::*pfnThink)();	// Do a PreThink() in this state.
};

struct BallHistory
{
	float snapshotTime;
	Vector pos;
	QAngle ang;
	Vector vel;
	AngularImpulse angImp;

	BallHistory(float snapshotTime, Vector pos, QAngle ang, Vector vel, AngularImpulse angImp) : snapshotTime(snapshotTime), pos(pos), ang(ang), vel(vel), angImp(angImp) {}
};

struct CBallTouchInfo
{
	CSDKPlayer	*m_pPl;
	int			m_nTeam;
	bool		m_bIsShot;
};

//==========================================================
//	
//	
//==========================================================
class CBall	: public CPhysicsProp, public IMultiplayerPhysics
{
public:
	DECLARE_CLASS( CBall, CPhysicsProp );
	CNetworkVar( int, m_iPhysicsMode );	// One of the PHYSICS_MULTIPLAYER_ defines.	
	CNetworkVar( float,	m_fMass	);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CBall();//	{ m_iPhysicsMode = PHYSICS_MULTIPLAYER_AUTODETECT; }
	~CBall();

	void			SetPhysicsMode(int iMode)	{ m_iPhysicsMode = iMode; }
	int				GetPhysicsMode(void) { return m_iPhysicsMode; }
	int				GetMultiplayerPhysicsMode(void)	{ return m_iPhysicsMode; }
	float			GetMass(void) {	return m_fMass;	}
	int				ObjectCaps(void)	{  return BaseClass::ObjectCaps() |	FCAP_CONTINUOUS_USE; }

	void			SendMatchEvent(match_event_t matchEvent, CSDKPlayer *pPlayer1 = NULL);

	void			TakeReplaySnapshot();
	void			StartReplay();
	void			RestoreReplaySnapshot();

	bool			IsAsleep(void) { return	false; }
	void			Spawn(void);
	void			BallThink( void	);
	void			BallTouch( CBaseEntity *pOther );
	void			VPhysicsCollision( int index, gamevcollisionevent_t	*pEvent	);
	void			VPhysicsUpdate(IPhysicsObject *pPhysics);
	bool			CreateVPhysics();
	
	void			TriggerGoal(int team);
	void			TriggerGoalLine(int team);
	void			TriggerSideline();
	bool			GetIgnoreTriggers() { return m_bIgnoreTriggers; };
	void			SetIgnoreTriggers(bool ignoreTriggers) { m_bIgnoreTriggers = ignoreTriggers; };
	void			SetRegularKickOff(bool regular) { m_bRegularKickOff = regular; };

	void			State_Transition( ball_state_t newState, float delay = 0.0f );

private:
	void State_NORMAL_Enter();		void State_NORMAL_Think();		void State_NORMAL_Leave();
	void State_KICKOFF_Enter();		void State_KICKOFF_Think();		void State_KICKOFF_Leave();
	void State_THROWIN_Enter();		void State_THROWIN_Think();		void State_THROWIN_Leave();
	void State_GOALKICK_Enter();	void State_GOALKICK_Think();	void State_GOALKICK_Leave();
	void State_CORNER_Enter();		void State_CORNER_Think();		void State_CORNER_Leave();
	void State_GOAL_Enter();		void State_GOAL_Think();		void State_GOAL_Leave();
	void State_FREEKICK_Enter();	void State_FREEKICK_Think();	void State_FREEKICK_Leave();

	void State_PreThink();
	void State_PostThink();
	void State_Enter(ball_state_t newState);	// Initialize the new state.
	void State_Leave();										// Cleanup the previous state.
	void State_Think();										// Update the current state.
	void State_DoTransition( ball_state_t newState );
	static CBallStateInfo* State_LookupInfo(ball_state_t state);	// Find the state info for the specified state.
	ball_state_t	m_eBallState;
	ball_state_t	m_eNextState;
	float			m_flStateEnterTime;
	float			m_flStateLeaveTime;
	CBallStateInfo	*m_pCurStateInfo;
	
	void			SetPos(const Vector &pos);
	void			MarkOffsidePlayers();
	void			UnmarkOffsidePlayers();
	void			EnableOffsideLine(float yPos);
	void			DisableOffsideLine();

	CSDKPlayer		*FindNearestPlayer(int team = TEAM_INVALID, int posFlags = FL_POS_FIELD);
	CSDKPlayer		*FindEligibleCarrier();
	bool			DoBodyPartAction();
	bool			DoGroundShot();
	bool			DoVolleyShot();
	bool			DoChestDrop();
	bool			DoHeader();
	void			SetBallCurve(bool bReset);
	float			GetPitchModifier();
	float			GetPowershotModifier();
	void			UpdateCarrier();
	bool			PlOnField(CSDKPlayer *pPl);
	void			Kicked(body_part_t bodyPart);
	void			Touched(CSDKPlayer *pPl, bool isShot);
	CBallTouchInfo	*LastInfo(bool wasShooting);
	CSDKPlayer		*LastPl(bool wasShooting);
	int				LastTeam(bool wasShooting);
	int				LastOppTeam(bool wasShooting);
	void			UpdatePossession(CSDKPlayer *pNewPossessor);

	IPhysicsObject	*m_pPhys;
	float			m_flPhysRadius;
	bool			m_bFreeze;
	Vector			m_vTriggerTouchPos;
	
	CSDKPlayer		*m_pPl;				  // Current player for state

	QAngle			m_aPlAng;
	Vector			m_vPlVel, m_vPlPos, m_vPlForward, m_vPlRight, m_vPlUp;
	int				m_nPlTeam;
	int				m_nPlPos;
	bool			m_bIsPowershot;
	bool			m_bIsRemoteControlled;
	body_part_t		m_eBodyPart;

	CSDKPlayer		*m_pFoulingPl;
	int				m_nFoulingTeam;
	foul_type_t		m_eFoulType;

	Vector			m_vPos, m_vVel;
	QAngle			m_aAng;
	AngularImpulse	m_vAngImp;
	
	CUtlVector<BallHistory>	m_History;
	bool			m_bDoReplay;
	int				m_nReplaySnapshotIndex;

	int				m_team;				  //team the ball can be kicked	by (during a corner	etc) (0=any)
	int				m_side;				  //side of	the	pitch the corner/goalkick should be	taken from
	int				m_BallInPenaltyBox;	 //-1 =	not	in box,	0,1	= teams	box
	int				m_FoulInPenaltyBox;	 //-1 =	not	in box,	0,1	= teams	box - recorded when foul occurs

	bool			m_bIgnoreTriggers;
	
	CSDKPlayer		*m_pPossessingPl;
	int				m_nPossessingTeam;
	float			m_flPossessionStart;

	bool			m_bRegularKickOff;

	CUtlVector<CBallTouchInfo> m_Touches;

	CBeam *m_pOffsideLine;
};

#endif
