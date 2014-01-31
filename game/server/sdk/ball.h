
#ifndef _BALL_H
#define _BALL_H

#include "props_shared.h"
#include "props.h"
#include "sdk_player.h"
#include "in_buttons.h"
#include "ios_teamkit_parse.h"
#include "SpriteTrail.h"

#define	BALL_MODEL	 "models/ball/ball.mdl"

enum body_part_t
{
	BODY_PART_NONE = 0,
	BODY_PART_FEET,
	BODY_PART_HIP,
	BODY_PART_CHEST,
	BODY_PART_HEAD,
	BODY_PART_HANDS,
	BODY_PART_KEEPERCATCH,
	BODY_PART_KEEPERPUNCH,
	BODY_PART_KEEPERHANDS,
	BODY_PART_UNKNOWN
};

enum foul_type_t
{
	FOUL_NONE = 0,
	FOUL_NORMAL_NO_CARD,
	FOUL_NORMAL_YELLOW_CARD,
	FOUL_NORMAL_RED_CARD,
	FOUL_OFFSIDE,
	FOUL_DOUBLETOUCH,
	FOUL_TIMEWASTING
};

enum penalty_state_t
{
	PENALTY_NONE = 0,
	PENALTY_ASSIGNED,
	PENALTY_KICKED,
	PENALTY_SCORED,
	PENALTY_SAVED,
	PENALTY_ABORTED_NO_TAKER,
	PENALTY_ABORTED_NO_KEEPER,
	PENALTY_ABORTED_ILLEGAL_MOVE
};

#define FL_SPIN_PERMIT_BACK		(1 << 0)
#define FL_SPIN_PERMIT_TOP		(1 << 1)
#define FL_SPIN_PERMIT_SIDE		(1 << 2)
#define FL_SPIN_FORCE_BACK		(1 << 3)
#define FL_SPIN_FORCE_TOP		(1 << 4)
#define FL_SPIN_FORCE_SIDE		(1 << 5)

#define FL_SPIN_PERMIT_ALL		(FL_SPIN_PERMIT_BACK | FL_SPIN_PERMIT_TOP | FL_SPIN_PERMIT_SIDE)

#define FL_POS_KEEPER					(1 << 0)
#define FL_POS_DEFENDER					(1 << 1)
#define FL_POS_MIDFIELDER				(1 << 2)
#define FL_POS_ATTACKER					(1 << 3)
#define FL_POS_FIELD					(1 << 4)
#define FL_POS_ANY						(1 << 5)
#define FL_POS_LEFT						(1 << 6)
#define FL_POS_CENTER					(1 << 7)
#define FL_POS_RIGHT					(1 << 8)

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
	void (CBall::*pfnLeaveState)(ball_state_t newState);	// Do a PreThink() in this state.
};

struct BallTouchInfo
{
	CHandle<CSDKPlayer>	m_pPl;
	int				m_nTeam;
	bool			m_bIsShot;
	body_part_t		m_eBodyPart;
	ball_state_t	m_eBallState;
	Vector			m_vBallPos;
	Vector			m_vBallVel;
	float			m_flTime;
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
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CBall();//	{ m_iPhysicsMode = PHYSICS_MULTIPLAYER_AUTODETECT; }
	~CBall();

	CNetworkVar( int, m_iPhysicsMode );	// One of the PHYSICS_MULTIPLAYER_ defines.	
	CNetworkVar( float,	m_fMass	);
	CNetworkHandle(CSDKPlayer, m_pCreator);
	CNetworkVar(bool, m_bIsPlayerBall);
	CNetworkVar(ball_state_t, m_eBallState);
	CNetworkHandle(CSDKPlayer, m_pLastActivePlayer);
	CNetworkHandle(CSDKPlayer, m_pHoldingPlayer);
	CNetworkVar(int, m_nLastActiveTeam);
	CNetworkVar(bool, m_bNonnormalshotsBlocked);
	CNetworkVar(bool, m_bShotsBlocked);
	CNetworkString(m_szSkinName, MAX_KITNAME_LENGTH);

	void			RemoveAllPlayerBalls();
	void			RemovePlayerBall();

	void			SetPhysicsMode(int iMode)	{ m_iPhysicsMode = iMode; }
	int				GetPhysicsMode(void) { return m_iPhysicsMode; }
	int				GetMultiplayerPhysicsMode(void)	{ return m_iPhysicsMode; }
	float			GetMass(void) {	return m_fMass;	}
	int				ObjectCaps(void)	{  return BaseClass::ObjectCaps() |	FCAP_CONTINUOUS_USE; }
	int				UpdateTransmitState();

	virtual	bool		ShouldCollide( int collisionGroup, int contentsMask ) const;

	bool			IsAsleep(void) { return	false; }
	void			Spawn(void);
	void			Think( void	);
	void			VPhysicsCollision( int index, gamevcollisionevent_t	*pEvent	);
	void			VPhysicsUpdate(IPhysicsObject *pPhysics);
	bool			CreateVPhysics();
	
	void			GetGoalInfo(bool &isOwnGoal, int &scoringTeam, CSDKPlayer **pScorer, CSDKPlayer **pFirstAssister, CSDKPlayer **pSecondAssister);
	void			TriggerGoal(int team);
	void			TriggerGoalLine(int team);
	void			TriggerSideline();
	void			TriggerPenaltyBox(int team);
	bool			IsSettingNewPos() { return m_bSetNewPos; }
	bool			HasQueuedState() { return m_bHasQueuedState; }

	void			SendNotifications();

	void			State_Transition(ball_state_t newState, float delay = 0.0f, bool cancelQueuedState = false, bool isShortMessageDelay = false);

	void			Reset();
	void			ReloadSettings();

	void			SetPos(const Vector &pos, bool teleport = true);
	void			SetAng(const QAngle &ang);
	void			SetVel(Vector vel, float spinCoeff, int spinFlags, body_part_t bodyPart, bool isDeflection, bool markOffsidePlayers, bool ensureMinShotStrength, float nextShotMinDelay = 0);
	void			SetRot(AngularImpulse rot = NULL);

	void			SetPenaltyState(penalty_state_t penaltyState) { m_ePenaltyState = penaltyState; }
	penalty_state_t	GetPenaltyState() { return m_ePenaltyState; }

	void			SetPenaltyTaker(CSDKPlayer *pPl);

	inline ball_state_t State_Get( void ) { return m_pCurStateInfo->m_eBallState; }

	CSDKPlayer		*GetLastActivePlayer() { return m_pLastActivePlayer; }
	CSDKPlayer		*GetCurrentPlayer() { return m_pPl; }
	CSDKPlayer		*GetCurrentOtherPlayer() { return m_pOtherPl; }
	CSDKPlayer		*GetCreator() { return m_pCreator; }
	void			SetCreator(CSDKPlayer *pCreator) { m_pCreator = pCreator; m_bIsPlayerBall = (pCreator != NULL); }
	CSDKPlayer		*GetHoldingPlayer() { return m_pHoldingPlayer; }
	void			EnablePlayerCollisions(bool enable);
	void			RemoveFromPlayerHands(CSDKPlayer *pPl);
	Vector			GetPos();
	QAngle			GetAng();
	Vector			GetVel();
	AngularImpulse	GetRot();
	float			CalcFieldZone();
	void			UpdatePossession(CSDKPlayer *pNewPossessor);
	void			SaveBallCannonSettings();
	void			RestoreBallCannonSettings();

	const char		*GetSkinName() { return m_szSkinName; }
	void			SetSkinName(const char *skinName);

private:

	void State_STATIC_Enter();			void State_STATIC_Think();			void State_STATIC_Leave(ball_state_t newState);
	void State_NORMAL_Enter();			void State_NORMAL_Think();			void State_NORMAL_Leave(ball_state_t newState);
	void State_KICKOFF_Enter();			void State_KICKOFF_Think();			void State_KICKOFF_Leave(ball_state_t newState);
	void State_THROWIN_Enter();			void State_THROWIN_Think();			void State_THROWIN_Leave(ball_state_t newState);
	void State_GOALKICK_Enter();		void State_GOALKICK_Think();		void State_GOALKICK_Leave(ball_state_t newState);
	void State_CORNER_Enter();			void State_CORNER_Think();			void State_CORNER_Leave(ball_state_t newState);
	void State_GOAL_Enter();			void State_GOAL_Think();			void State_GOAL_Leave(ball_state_t newState);
	void State_FREEKICK_Enter();		void State_FREEKICK_Think();		void State_FREEKICK_Leave(ball_state_t newState);
	void State_PENALTY_Enter();			void State_PENALTY_Think();			void State_PENALTY_Leave(ball_state_t newState);
	void State_KEEPERHANDS_Enter();		void State_KEEPERHANDS_Think();		void State_KEEPERHANDS_Leave(ball_state_t newState);

	void State_PreThink();
	void State_PostThink();
	void State_Enter(ball_state_t newState, bool cancelQueuedState);	// Initialize the new state.
	void State_Think();										// Update the current state.
	void State_Leave(ball_state_t newState);
	static CBallStateInfo* State_LookupInfo(ball_state_t state);	// Find the state info for the specified state.

	void			FindStatePlayer(ball_state_t ballState = BALL_STATE_NONE);

	ball_state_t	m_eNextState;
	float			m_flStateEnterTime;
	float			m_flStateLeaveTime;
	float			m_flStateActivationDelay;
	float			m_flStateTimelimit;
	bool			m_bNextStateMessageSent;
	CBallStateInfo	*m_pCurStateInfo;

	void			MarkOffsidePlayers();
	void			UnmarkOffsidePlayers();

	void			HandleFoul();
	bool			PlayersAtTargetPos();
	bool			CanTouchBallXY();
	bool			CheckFoul(bool canShootBall, const Vector &localDirToBall);
	void			TriggerFoul(foul_type_t type, Vector pos, CSDKPlayer *pFoulingPl, CSDKPlayer *pFouledPl = NULL);
	CSDKPlayer		*FindNearestPlayer(int team = TEAM_INVALID, int posFlags = FL_POS_FIELD, bool checkIfShooting = false, int ignoredPlayerBits = 0, float radius = -1);
	bool			DoBodyPartAction();
	bool			DoSlideAction();
	bool			CheckKeeperCatch();
	bool			DoGroundShot(bool markOffsidePlayers, float velCoeff = 1.0f);
	bool			DoVolleyShot();
	bool			DoHeader();
	AngularImpulse	CalcSpin(float coeff, int spinFlags);
	float			GetPitchCoeff(bool isNormalShot, bool useCamViewAngles = false);
	float			GetNormalshotStrength(float coeff, int strength);
	float			GetPowershotStrength(float coeff, int strength);
	float			GetChargedshotStrength(float coeff, int minStrength, int maxStrength);
	void			UpdateCarrier();
	void			Touched(CSDKPlayer *pPl, bool isShot, body_part_t bodyPart, const Vector &oldVel);
	void			RemoveAllTouches();
	BallTouchInfo	*LastInfo(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);
	CSDKPlayer		*LastPl(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);
	int				LastTeam(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);
	int				LastOppTeam(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);

	IPhysicsObject	*m_pPhys;
	float			m_flPhysRadius;
	Vector			m_vTriggerTouchPos;

	CHandle<CSDKPlayer>	m_pPl;
	CHandle<CSDKPlayer>	m_pOtherPl;

	QAngle			m_aPlAng, m_aPlCamAng;
	Vector			m_vPlVel, m_vPlVel2D, m_vPlForwardVel2D, m_vPlPos, m_vPlForward, m_vPlForward2D, m_vPlRight, m_vPlUp, m_vPlDirToBall, m_vPlLocalDirToBall;

	CHandle<CSDKPlayer>	m_pFouledPl;
	CHandle<CSDKPlayer>	m_pFoulingPl;
	int				m_nFouledTeam;
	int				m_nFoulingTeam;
	foul_type_t		m_eFoulType;
	Vector			m_vFoulPos;

	Vector			m_vPos, m_vVel, m_vOffsidePos;
	QAngle			m_aAng;
	AngularImpulse	m_vRot;

	int				m_nTeam;				  //team the ball can be kicked	by (during a corner	etc) (0=any)
	int				m_nInPenBoxOfTeam;	 //-1 =	not	in box,	0,1	= teams	box

	bool			m_bHasQueuedState;
	bool			m_bSetNewPos;
	bool			m_bSetNewVel;
	bool			m_bSetNewRot;
	
	CHandle<CSDKPlayer>	m_pPossessingPl;
	int				m_nPossessingTeam;
	float			m_flPossessionStart;

	CUtlVector<BallTouchInfo *> m_Touches;

	penalty_state_t m_ePenaltyState;

	MatchEventPlayerInfo m_MatchEventPlayerInfo;

	//CHandle<CSDKPlayer> m_pHoldingPlayer;

	float			m_flGlobalLastShot;
	float			m_flGlobalDynamicShotDelay;

	bool			m_bPenaltyTakerStartedMoving;

	float			m_flLastMatchEventSetTime;

	bool			m_bIsBallCannonMode;
	Vector			m_vBallCannonPos;
	Vector			m_vBallCannonVel;
	QAngle			m_aBallCannonAng;
	AngularImpulse	m_vBallCannonRot;

	bool			m_bHitThePost;

	CHandle<CSpriteTrail> m_pGlowTrail;
};

#endif
