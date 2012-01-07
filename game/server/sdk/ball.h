
#ifndef _BALL_H
#define _BALL_H

#include "props_shared.h"
#include "props.h"
#include "sdk_player.h"
#include "in_buttons.h"



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
	BALL_GOALKICK_PENDING,
	BALL_CORNERKICK_PENDING,
	BALL_THROWIN_PENDING,
	BALL_FOUL_PENDING,
	BALL_PENALTY_PENDING,
	BALL_KICKOFF_PENDING
};

#define	PS_OFF					 0
#define	PS_BESTOFFIVE			 1
#define	PS_SUDDENDEATH			 2
#define	PS_DONE					 3


#define BALL_MAINSTATUS_FOUL	0
#define BALL_MAINSTATUS_YELLOW	1
#define BALL_MAINSTATUS_RED		2
#define BALL_MAINSTATUS_OFFSIDE	3
#define BALL_MAINSTATUS_PLAYON	4
#define BALL_MAINSTATUS_PENALTY	5
#define BALL_MAINSTATUS_FINAL_WHISTLE		6

class CBall;

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

//==========================================================
//	
//	
//==========================================================
class CBall	: public CPhysicsProp, public IMultiplayerPhysics
{
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

private:
	CSDKPlayer		*FindEligibleCarrier();
	bool			SelectAction();
	bool			DoGroundShot();
	bool			DoVolleyShot();
	bool			DoChestDrop();
	bool			DoHeader();
	void			SetBallCurve(bool bReset);
	float			GetPitchModifier();
	float			GetPowershotModifier();

	IPhysicsObject	*m_pPhys;

	CSDKPlayer		*m_pPl; // Player who may shoot
	Vector			m_vPlVel;
	Vector			m_vPlPos;
	QAngle			m_aPlAng;
	Vector			m_vPlForward;
	Vector			m_vPlRight;
	Vector			m_vPlUp;

	Vector			m_vPos;
	Vector			m_vNewPos;
	Vector			m_vVel;
	Vector			m_vNewVel;
	QAngle			m_aAng;
	QAngle			m_aNewAng;
	AngularImpulse	m_vAngImp;
	AngularImpulse	m_vNewAngImp;

	Vector			m_vSpawnPos;

	bool			m_bIsPowershot;
	
	CUtlVector<BallHistory>	m_History;
	bool			m_bDoReplay;
	int				m_nReplaySnapshotIndex;

public:
	void			SendMatchEvent(match_event_t matchEvent, CSDKPlayer *pPlayer1 = NULL, CSDKPlayer *pPlayer2 = NULL);

	void			TakeReplaySnapshot();
	void			StartReplay();
	void			RestoreReplaySnapshot();

	bool			IsAsleep(void) { return	false; }
	virtual	void	Spawn(void);
	void			BallThink( void	);
	void			BallTouch( CBaseEntity *pOther );
	void			VPhysicsCollision( int index, gamevcollisionevent_t	*pEvent	);
	void			VPhysicsUpdate(IPhysicsObject *pPhysics);
	bool			CreateVPhysics();

	void HandleKickOff ( void	);
	int	 Shoot ( CBaseEntity *pOther, bool isPowershot=false);
	int	 Pass (	CBaseEntity	*pOther);
	CBaseEntity	*FindTarget	(CBaseEntity *pPlayer);
	void ProcessStatus (void);
	void ShieldBall	(CSDKPlayer	*pPlayer);
	void UpdateBallShield (void);
	void ShieldOff (void);
	CBaseEntity	*FindPlayerForAction (int team,	int	allowkeepers); //who should	take corner	etc
	CBaseEntity	*FindThrowInEnt	(void);				   //find throwin position marker
	CSDKPlayer	*FindKeeperForAction (int team);	   //who should	take goal kick
	int	KeepersBall	(void);
	int	KeepersBallHuman (void);
	int	CheckKeeperCatch (CSDKPlayer *keeper);
	int	CheckKeeperKick (CSDKPlayer *keeper);

	int	  CheckOffSide (CSDKPlayer *kicker);
	int	  CheckOffsideReceive (CSDKPlayer* pOther);
	int	  CheckOffsideReceiver (CSDKPlayer*	pOther,	int	i);
	int	  PlayerInOwnHalf (CSDKPlayer *pOther);
	void  ClearOffsideFlags	(void);
	int	  NextOffsideSlot (void);
	void  RecordOffside	(CSDKPlayer	*offside, CSDKPlayer *kicker, CSDKPlayer *against);

	CSDKPlayer		*m_OffsidePlayer[MAX_OFFS];
	CSDKPlayer		*m_OffsideKicker[MAX_OFFS];
	float			m_OffsideTimeout[MAX_OFFS];
	Vector			m_OffsideFoulPos[MAX_OFFS];
	int				m_NumOffsidePlayers;

	float			m_NextDoubleTouch;
	int				m_DoubleTouchCount;
	int				m_HeadTouch;
	CSDKPlayer		*m_PlayerWhoHeaded;

	CSDKPlayer		*m_LastTouch;		 //last	touch by anyone	- for corners/goal kicks etc
	CSDKPlayer		*m_LastPlayer;		 //last	touch (by a	real player) - for goals
	CSDKPlayer		*m_LastNonKeeper;	 //last	touch by a real	player who isn't a keeper
	CSDKPlayer		*m_LastTouchBeforeMe;//last touch before me

	CSDKPlayer		*m_PlayerWhoTook;	 //player who took throwin etc

	CSDKPlayer		*m_KeeperCarrying;
	float			m_KeeperCarryTime;
	int				m_NextKeeperCatch;

	int				ballStatus;			  //corner,	goalkick, throwin, foul, penalty
	float			ballStatusTime;		  //time to	process	the	status change (gives a grace period	when ball goes out of play etc)
	int				m_team;				  //team the ball can be kicked	by (during a corner	etc) (0=any)
	int				m_side;				  //side of	the	pitch the corner/goalkick should be	taken from
	int				m_BallInPenaltyBox;	 //-1 =	not	in box,	0,1	= teams	box
	int				m_FoulInPenaltyBox;	 //-1 =	not	in box,	0,1	= teams	box - recorded when foul occurs
	Vector			curve;

	int				m_BallShield;			//1=on, 0=off,	exclusion zone around ball
	CSDKPlayer		*m_BallShieldPlayer;	//player allowed into exclusion	zone
	Vector			m_FoulPos;				//where to	take foul from
	CSDKPlayer		*m_Foulee;				//who was fouled, i.e. who should take freekick/penalty

	int				m_KickOff;			//which	team should	kick off next

	CSDKPlayer		*m_Assist[MAX_ASSIST];
	void			AddAssist (CSDKPlayer *pNewTouch);
	void			AssistDisconnect (CSDKPlayer *discon);
	void			CheckForAssist (CSDKPlayer *scorer);
	void			ClrAssists (void);

	float			m_poss[2];		  //total time of possession
	float			m_possStart;	  //start time of current possession							
	int				m_possTeam;		  //team currently in possession
	CSDKPlayer*		m_pPossPlayer;		//player who kicked the ball before
	void			ResetPossession	(void);
	void			UpdatePossession (CSDKPlayer *pPlayer);
	int				GetTeamPossession (int team);

	int				m_TeamGoal;			//team that scored
	float			m_KickOffTime;

	void			HandleGoal(void);
	void			HandleCorner(void);
	void			HandleGoalkick(void);
	void			HandleThrowin(void);
	void			HandleFoul(void);
	void			HandlePenalty(void);

	int				m_PowerShotStrength;
	void			ChargePowerShot();

	void			EnableCeleb(void);
	void			DisableCeleb(void);

	void			FreezeKeepersForPenalty(int team);

	void			DoubleTouchFoul(CSDKPlayer *player);

	void			KickedBall(CSDKPlayer *pPlayer, bool bKick = true);

	void			DropBall(void);

	void			UpdateHeadbounce(CSDKPlayer *pPlayer);

	float			m_fUseOffsideRule;

	CSDKPlayer*		m_pKeeperParry;

	void			ApplyKickDelayToAll(void);

	void			RemoveFlagFromAll(int flag);

private:
	ball_state_t	m_eBallState;
	ball_state_t	m_eNextState;
	float			m_flStateEnterTime;
	float			m_flStateLeaveTime;
	CBallStateInfo	*m_pCurStateInfo;

	bool			m_bIgnoreTriggers;

public:
	CSDKPlayer *FindNearestPlayer(int nTeam = TEAM_INVALID, bool ignoreKeepers = true);
	void SetPos(const Vector &pos);
	void TriggerGoal(int team);
	void TriggerGoalline(int team, int side);
	void TriggerSideline(int side);
	bool IgnoreTriggers() { return m_bIgnoreTriggers; };

	void PreStateHook();
	void PostStateHook();

	void State_Transition( ball_state_t newState, float delay = 0.0f );
	void State_DoTransition( ball_state_t newState );
	void State_Enter(ball_state_t newState);	// Initialize the new state.
	void State_Leave();										// Cleanup the previous state.
	void State_Think();										// Update the current state.
	static CBallStateInfo* State_LookupInfo(ball_state_t state);	// Find the state info for the specified state.

	void State_Enter_NORMAL();
	void State_Think_NORMAL();

	void State_Enter_KICKOFF();
	void State_Think_KICKOFF();
	void State_Leave_KICKOFF();

	void State_Enter_THROWIN();
	void State_Think_THROWIN();
	void State_Leave_THROWIN();

	void State_Enter_GOALKICK();
	void State_Think_GOALKICK();
	void State_Leave_GOALKICK();

	void State_Enter_CORNER();
	void State_Think_CORNER();
	void State_Leave_CORNER();
	
	void State_Enter_GOAL();
	void State_Think_GOAL();
	void State_Leave_GOAL();
};

#endif
