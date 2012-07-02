
#ifndef _BALL_H
#define _BALL_H

#include "props_shared.h"
#include "props.h"
#include "sdk_player.h"
#include "in_buttons.h"
#include "beam_shared.h"


#define	BALL_MODEL	 "models/w_fb.mdl"

#define	BALL_ELSEWHERE	0
#define	BALL_NEAR_FEET	1
#define	BALL_NEAR_HEAD	2

#define	MAX_OFFS		16
#define	MAX_ASSIST		10

enum ball_state_t
{
	BALL_NOSTATE = 0,
	BALL_NORMAL,
	BALL_GOAL,
	BALL_CORNER,
	BALL_GOALKICK,
	BALL_THROWIN,
	BALL_FOUL,
	BALL_KICKOFF,
	BALL_PENALTY,
	BALL_FREEKICK,
	BALL_KEEPERHANDS
};

enum body_part_t
{
	BODY_NONE = 0,
	BODY_FEET,
	BODY_HIP,
	BODY_CHEST,
	BODY_HEAD,
	BODY_HANDS
};

enum foul_type_t
{
	FOUL_NONE = -1,
	FOUL_NORMAL = 0,
	FOUL_OFFSIDE,
	FOUL_DOUBLETOUCH,
	FOUL_TIMEWASTING
};

enum penalty_state_t
{
	PENALTY_NONE = 0,
	PENALTY_ASSIGNED,
	PENALTY_KICKED,
	PENALTY_ABORTED_NO_TAKER,
	PENALTY_ABORTED_NO_KEEPER,
};

#define BODY_FEET_START		0
#define BODY_FEET_END		15
#define BODY_HIP_START		15
#define BODY_HIP_END		40
#define BODY_CHEST_START	40
#define BODY_CHEST_END		60
#define BODY_HEAD_START		60
#define BODY_HEAD_END		80

#define PITCH_LIMIT			89

#define FL_POS_KEEPER					1
#define FL_POS_DEFENDER					2
#define FL_POS_MIDFIELDER				4
#define FL_POS_ATTACKER					8
#define FL_POS_FIELD					16
#define FL_POS_ANY						32

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
extern CBall *GetNearestBall(const Vector &pos);

struct CBallStateInfo
{
	ball_state_t			m_eBallState;
	const char				*m_pStateName;

	void (CBall::*pfnEnterState)();	// Init and deinit the state.
	void (CBall::*pfnThink)();	// Do a PreThink() in this state.
};

struct BallTouchInfo
{
	CHandle<CSDKPlayer>	m_pPl;
	int				m_nTeam;
	bool			m_bIsShot;
	body_part_t		m_eBodyPart;
	ball_state_t	m_eBallState;
};

struct MatchEventPlayerInfo
{
	CHandle<CSDKPlayer>	pPl;
	const char			*szPlayerName;
	int					team;
	int					userId;
	const char			*szNetworkIDString;				
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

	void		RemovePlayerBalls();

	void			SetPhysicsMode(int iMode)	{ m_iPhysicsMode = iMode; }
	int				GetPhysicsMode(void) { return m_iPhysicsMode; }
	int				GetMultiplayerPhysicsMode(void)	{ return m_iPhysicsMode; }
	float			GetMass(void) {	return m_fMass;	}
	int				ObjectCaps(void)	{  return BaseClass::ObjectCaps() |	FCAP_CONTINUOUS_USE; }
	int				UpdateTransmitState();

	void			SendMatchEvent(match_event_t matchEvent, CSDKPlayer *pPlayer = NULL);
	void			SendMatchEvent(match_event_t matchEvent, MatchEventPlayerInfo *pMatchEventPlayerInfo);
	void			SendMatchEvent(match_event_t matchEvent, const char *szPlayerName, int playerTeam, int playerUserID, const char *szPlayerNetworkIDString);

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
	void			TriggerPenaltyBox(int team);
	bool			IgnoreTriggers() { return m_bIgnoreTriggers; }
	void			SetIgnoreTriggers(bool ignoreTriggers) { m_bIgnoreTriggers = ignoreTriggers; }

	void			State_Transition( ball_state_t newState, float delay = 0.0f );

	void			ResetMatch();

	CNetworkVar(float, m_flOffsideLineBallPosY);
	CNetworkVar(float, m_flOffsideLineOffsidePlayerPosY);
	CNetworkVar(float, m_flOffsideLineLastOppPlayerPosY);
	CNetworkVar(bool, m_bOffsideLinesEnabled);
	
	void			SetPos(const Vector &pos);
	void			SetVel(const Vector &vel);
	void			SetRot(const AngularImpulse &rot = NULL);

	void			SetPenaltyState(penalty_state_t penaltyState) { m_ePenaltyState = penaltyState; }
	penalty_state_t	GetPenaltyState() { return m_ePenaltyState; }

	void			SetPenaltyTaker(CSDKPlayer *pPl);

	inline ball_state_t State_Get( void ) { return m_eBallState; }

	CSDKPlayer		*GetCurrentPlayer() { return m_pPl; }

	void			SetCreator(CSDKPlayer *pCreator);
	void			EnablePlayerCollisions(bool enable);

private:

	void State_NORMAL_Enter();		void State_NORMAL_Think();
	void State_KICKOFF_Enter();		void State_KICKOFF_Think();
	void State_THROWIN_Enter();		void State_THROWIN_Think();
	void State_GOALKICK_Enter();	void State_GOALKICK_Think();
	void State_CORNER_Enter();		void State_CORNER_Think();
	void State_GOAL_Enter();		void State_GOAL_Think();
	void State_FREEKICK_Enter();	void State_FREEKICK_Think();
	void State_PENALTY_Enter();		void State_PENALTY_Think();
	void State_KEEPERHANDS_Enter();		void State_KEEPERHANDS_Think();

	void State_PreThink();
	void State_PostThink();
	void State_Enter(ball_state_t newState);	// Initialize the new state.
	void State_Think();										// Update the current state.
	static CBallStateInfo* State_LookupInfo(ball_state_t state);	// Find the state info for the specified state.
	ball_state_t	m_eBallState;
	ball_state_t	m_eNextState;
	float			m_flStateEnterTime;
	float			m_flStateLeaveTime;
	float			m_flStateTimelimit;
	CBallStateInfo	*m_pCurStateInfo;

	void			MarkOffsidePlayers();
	void			UnmarkOffsidePlayers();
	void			SetOffsideLinePositions(float ballPosY, float offsidePlayerPosY, float lastOppPlayerPosY);
	void			SetOffsideLinesEnabled(bool enable);

	bool			PlayersAtTargetPos(bool holdAtTargetPos);
	bool			CheckFoul(bool canShootBall);
	void			TriggerFoul(foul_type_t type, Vector pos, CSDKPlayer *pFoulingPl, CSDKPlayer *pFouledPl = NULL);
	CSDKPlayer		*FindNearestPlayer(int team = TEAM_INVALID, int posFlags = FL_POS_FIELD, bool checkIfShooting = false, int ignoredPlayerBits = 0);
	bool			DoBodyPartAction();
	bool			DoSlideAction();
	bool			CheckKeeperCatch();
	bool			DoGroundShot();
	bool			DoVolleyShot();
	bool			DoChestDrop();
	bool			DoHeader();
	void			SetBallSpin();
	float			GetPitchModifier();
	float			GetPowershotModifier();
	void			UpdateCarrier();
	void			Kicked(body_part_t bodyPart);
	void			Touched(CSDKPlayer *pPl, bool isShot, body_part_t bodyPart);
	BallTouchInfo	*LastInfo(bool wasShooting, CSDKPlayer *pSkipPl = NULL);
	CSDKPlayer		*LastPl(bool wasShooting, CSDKPlayer *pSkipPl = NULL);
	int				LastTeam(bool wasShooting, CSDKPlayer *pSkipPl = NULL);
	int				LastOppTeam(bool wasShooting, CSDKPlayer *pSkipPl = NULL);
	void			UpdatePossession(CSDKPlayer *pNewPossessor);

	IPhysicsObject	*m_pPhys;
	float			m_flPhysRadius;
	Vector			m_vTriggerTouchPos;
	
	CHandle<CSDKPlayer>	m_pPl;				  // Current player for state
	CHandle<CSDKPlayer>	m_pOtherPl;

	QAngle			m_aPlAng;
	Vector			m_vPlVel, m_vPlPos, m_vPlForward, m_vPlForward2D, m_vPlRight, m_vPlUp;
	int				m_nPlTeam;
	int				m_nPlPos;

	CHandle<CSDKPlayer>	m_pFouledPl;
	CHandle<CSDKPlayer>	m_pFoulingPl;
	int				m_nFoulingTeam;
	foul_type_t		m_eFoulType;
	Vector			m_vFoulPos;

	Vector			m_vPos, m_vVel, m_vOffsidePos;
	QAngle			m_aAng;
	AngularImpulse	m_vRot;

	int				m_nTeam;				  //team the ball can be kicked	by (during a corner	etc) (0=any)
	int				m_nInPenBoxOfTeam;	 //-1 =	not	in box,	0,1	= teams	box

	bool			m_bIgnoreTriggers;
	bool			m_bSetNewPos;
	
	CHandle<CSDKPlayer>	m_pPossessingPl;
	int				m_nPossessingTeam;
	float			m_flPossessionStart;

	CUtlVector<BallTouchInfo> m_Touches;

	penalty_state_t m_ePenaltyState;

	MatchEventPlayerInfo m_MatchEventPlayerInfo;

	CHandle<CSDKPlayer> m_pCreator;

	float			m_flNextShot;
};

#endif
