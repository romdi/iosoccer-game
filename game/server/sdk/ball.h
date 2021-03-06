#ifndef _BALL_H
#define _BALL_H

#include "props_shared.h"
#include "props.h"
#include "ios_teamkit_parse.h"
#include "sdk_player.h"

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
	FOUL_NORMAL_SECOND_YELLOW_CARD,
	FOUL_NORMAL_RED_CARD,
	FOUL_OFFSIDE,
	FOUL_DOUBLETOUCH,
	FOUL_TIMEWASTING
};

#define FL_SPIN_FORCE_NONE		0
#define FL_SPIN_PERMIT_SIDE		(1 << 0)
#define FL_SPIN_FORCE_BACK		(1 << 1)
#define FL_SPIN_FORCE_TOP		(1 << 2)
#define FL_SPIN_RETAIN_SIDE		(1 << 3)

#define FL_POS_KEEPER					(1 << 0)
#define FL_POS_DEFENDER					(1 << 1)
#define FL_POS_MIDFIELDER				(1 << 2)
#define FL_POS_ATTACKER					(1 << 3)
#define FL_POS_OUTFIELD					(1 << 4)
#define FL_POS_ANY						(1 << 5)
#define FL_POS_LEFT						(1 << 6)
#define FL_POS_CENTER					(1 << 7)
#define FL_POS_RIGHT					(1 << 8)

class CBall;

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
	CNetworkVar(ball_state_t, m_eBallState);
	CNetworkHandle(CSDKPlayer, m_pHoldingPlayer);
	CNetworkString(m_szSkinName, MAX_KITNAME_LENGTH);

	void			SetPhysicsMode(int iMode)	{ m_iPhysicsMode = iMode; }
	int				GetPhysicsMode(void) { return m_iPhysicsMode; }
	int				GetMultiplayerPhysicsMode(void)	{ return m_iPhysicsMode; }
	float			GetMass(void) {	return m_fMass;	}
	int				ObjectCaps(void)	{  return BaseClass::ObjectCaps() |	FCAP_CONTINUOUS_USE; }
	int				UpdateTransmitState();

	virtual	bool		ShouldCollide( int collisionGroup, int contentsMask ) const;

	bool			IsAsleep(void) { return	false; }
	virtual void	Spawn(void);
	void			Think( void	);
	virtual void	VPhysicsCollision(int index, gamevcollisionevent_t	*pEvent);
	void			VPhysicsUpdate(IPhysicsObject *pPhysics);
	bool			CreateVPhysics();
	
	bool			HasQueuedState() { return m_bHasQueuedState; }

	virtual void	Reset();
	void			ReloadSettings();

	void			SetPos(const Vector &pos, bool addBallRadiusZOffset = true, bool freeze = true);
	void			SetAng(const QAngle &ang);
	virtual void	SetVel(Vector vel, float spinCoeff, int spinFlags, body_part_t bodyPart, bool markOffsidePlayers, float minPostDelay, bool resetShotCharging);
	void			SetRot(AngularImpulse rot = NULL);

	inline ball_state_t State_Get( void ) { return m_pCurStateInfo->m_eBallState; }

	CSDKPlayer		*GetCurrentPlayer() { return m_pPl; }
	CSDKPlayer		*GetHoldingPlayer() { return m_pHoldingPlayer; }
	void			EnablePlayerCollisions(bool enable);
	void			AddToPlayerHands(CSDKPlayer *pPl);
	void			RemoveFromPlayerHands(CSDKPlayer *pPl);
	Vector			GetPos();
	QAngle			GetAng();
	Vector			GetVel();
	AngularImpulse	GetRot();

	const char		*GetSkinName() { return m_szSkinName; }
	void			SetSkinName(const char *skinName);

	virtual void State_Transition(ball_state_t nextState, float nextStateMessageDelay = 0, float nextStatePostMessageDelay = 0, bool cancelQueuedState = false) = 0;

	float			GetStateEnterTime() { return m_flStateEnterTime; }
	void			RemoveAllTouches();
	BallTouchInfo	*LastInfo(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);
	CSDKPlayer		*LastPl(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);
	int				LastTeam(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);
	int				LastOppTeam(bool wasShooting, CSDKPlayer *pSkipPl = NULL, CSDKPlayer *pSkipPl2 = NULL, CSDKPlayer *pSkipPl3 = NULL);

	int				InPenBoxOfTeam() { return m_nInPenBoxOfTeam; }
	void			GetPredictedGoalLineCrossPosX(int &xPos, int &team);

protected:

	virtual void State_STATIC_Enter() {};			virtual void State_STATIC_Think() {};			virtual void State_STATIC_Leave(ball_state_t newState) {};
	virtual void State_NORMAL_Enter() {};			virtual void State_NORMAL_Think() {};			virtual void State_NORMAL_Leave(ball_state_t newState)  {};
	virtual void State_KICKOFF_Enter() {};			virtual void State_KICKOFF_Think() {};			virtual void State_KICKOFF_Leave(ball_state_t newState) {};
	virtual void State_THROWIN_Enter() {};			virtual void State_THROWIN_Think() {};			virtual void State_THROWIN_Leave(ball_state_t newState) {};
	virtual void State_GOALKICK_Enter() {};			virtual void State_GOALKICK_Think() {};			virtual void State_GOALKICK_Leave(ball_state_t newState) {};
	virtual void State_CORNER_Enter() {};			virtual void State_CORNER_Think() {};			virtual void State_CORNER_Leave(ball_state_t newState) {};
	virtual void State_GOAL_Enter() {};				virtual void State_GOAL_Think() {};				virtual void State_GOAL_Leave(ball_state_t newState) {};
	virtual void State_FREEKICK_Enter() {};			virtual void State_FREEKICK_Think() {};			virtual void State_FREEKICK_Leave(ball_state_t newState) {};
	virtual void State_PENALTY_Enter() {};			virtual void State_PENALTY_Think() {};			virtual void State_PENALTY_Leave(ball_state_t newState) {};
	virtual void State_KEEPERHANDS_Enter() {};		virtual void State_KEEPERHANDS_Think() {};			virtual void State_KEEPERHANDS_Leave(ball_state_t newState) {};

	virtual void State_Enter(ball_state_t newState, bool cancelQueuedState) = 0;	// Initialize the new state.
	virtual void State_Think() = 0;										// Update the current state.
	virtual void State_Leave(ball_state_t newState) = 0;
	static CBallStateInfo* State_LookupInfo(ball_state_t state);	// Find the state info for the specified state.

	ball_state_t	m_eNextState;
	CBallStateInfo	*m_pCurStateInfo;
	float			m_flStateEnterTime;
	float			m_flStateLeaveTime;

	bool			CanReachBallStandingXY();
	CSDKPlayer		*FindNearestPlayer(int team = TEAM_NONE, int posFlags = FL_POS_OUTFIELD, bool checkIfShooting = false, int ignoredPlayerBits = 0, float radius = -1);
	bool			GetCollisionPoint(bool isDeflection, Vector &collisionPoint);
	bool			CheckPlayerInteraction();
	bool			CheckBodyPartAction();
	bool			CheckCollision();
	bool			DoSlideAction();
	bool			DoStandingTackle();
	bool			DoDivingHeader();
	bool			DoBicycleKick();
	bool			CheckKeeperCatch();
	bool			DoGroundHeightAction(bool markOffsidePlayers);
	bool			DoHipHeightAction();
	bool			DoHeadHeightAction();
	AngularImpulse	CalcSpin(float coeff, int spinFlags);
	float			GetPitchCoeff();
	float			GetNormalshotStrength(float coeff, int strength);
	float			GetPowershotStrength(float coeff, int strength);
	float			GetChargedshotStrength(float coeff, int minStrength, int maxStrength);
	void			UpdateCarrier();
	virtual void	Touched(bool isShot, body_part_t bodyPart, const Vector &oldVel) = 0;
	virtual bool	IsLegallyCatchableByKeeper() = 0;
	void			CheckPenBoxPosition();

	IPhysicsObject	*m_pPhys;
	Vector			m_vTriggerTouchPos;

	CHandle<CSDKPlayer>	m_pPl;
	CUtlVector<BallTouchInfo *> m_Touches;

	QAngle			m_aPlAng, m_aPlCamAng;
	Vector			m_vPlVel, m_vPlVel2D, m_vPlForwardVel2D, m_vPlBackVel2D, m_vPlSideVel2D, m_vPlPos, m_vPlForward, m_vPlForward2D, m_vPlRight, m_vPlUp, m_vPlDirToBall, m_vPlLocalDirToBall;

	Vector			m_vPos, m_vVel, m_vOffsidePos;
	QAngle			m_aAng;
	AngularImpulse	m_vRot;

	int				m_nInPenBoxOfTeam;
	int				m_nWasInPenBoxOfTeam;

	bool			m_bHasQueuedState;

	float			m_flGlobalLastShot;
	float			m_flGlobalDynamicShotDelay;
	bool			m_bHitThePost;
};

#endif
