
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

#define	BALL_NORMAL				 0
#define	BALL_GOAL				 1
#define	BALL_CORNER				 2
#define	BALL_GOALKICK			 3
#define	BALL_THROWIN			 4
#define	BALL_FOUL				 5
#define BALL_KICKOFF			 6
#define	BALL_PENALTY			 7
#define	BALL_GOALKICK_PENDING	 8
#define	BALL_CORNERKICK_PENDING	 9
#define	BALL_THROWIN_PENDING	 10
#define	BALL_FOUL_PENDING		 11
#define	BALL_PENALTY_PENDING	 12
#define	BALL_KICKOFF_PENDING	 13

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

	CBall::CBall()	{ m_iPhysicsMode = PHYSICS_MULTIPLAYER_AUTODETECT; }

	void			SetPhysicsMode(int iMode)	{ m_iPhysicsMode = iMode; }
	int				GetPhysicsMode(void) { return m_iPhysicsMode; }
	int				GetMultiplayerPhysicsMode(void)	{ return m_iPhysicsMode; }
	float			GetMass(void) {	return m_fMass;	}
	int				ObjectCaps(void)	{  return BaseClass::ObjectCaps() |	FCAP_CONTINUOUS_USE; }

private:
	void			RunCheck();
	CSDKPlayer		*SelectShooter();
	bool			SelectAction();
	bool			DoGroundShot();
	bool			DoVolleyShot();
	bool			DoChestDrop();
	bool			DoHeader();
	void			SetBallCurve();

	CSDKPlayer		*m_pPl; // Player who may shoot
	Vector			m_vPlVel;
	Vector			m_vPlPos;
	QAngle			m_aPlAng;
	Vector			m_vPlForward;
	Vector			m_vPlRight;
	Vector			m_vPlUp;

	Vector			m_vPos;
	Vector			m_vVel;
	QAngle			m_aAng;
	AngularImpulse	m_vAngImp;
	Vector			m_vNewVel;
	AngularImpulse	m_vNewAngImp;

	bool			m_bIsPowershot;
	bool			m_bSetVel;
	bool			m_bSetAngImp;

public:
	bool			IsAsleep(void) { return	false; }
	virtual	void	Spawn(void);
	void			BallThink( void	);
	void			Use( CBaseEntity *pActivator, CBaseEntity *pCaller,	USE_TYPE useType, float	value );
	void			BallTouch( CBaseEntity *pOther );
	void			VPhysicsCollision( int index, gamevcollisionevent_t	*pEvent	);
	//void			VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent );

	float			m_NextShoot;


//	void ResetPos (	void );
	void HandleKickOff ( void	);
//	void Dribble( CBaseEntity *pOther, int pPlayerh	);
	int	 Shoot ( CBaseEntity *pOther, bool isPowershot=false);
//	int	 GoalKick (	CBaseEntity	*pOther);
//	int	 CornerKick	( CBaseEntity *pOther);
	int	 Pass (	CBaseEntity	*pOther);
//	int	 PowerShot ( CBaseEntity *pOther);
	CBaseEntity	*FindTarget	(CBaseEntity *pPlayer);
	int	GetRelativeBallPos (CBaseEntity	*pOther);
//	void PlayShootSound	( CBaseEntity *pOther, int hard);
//	void PlayPassSound ( CBaseEntity *pOther);
//	void PlayTouchSound	( CBaseEntity *pOther);
	void ProcessStatus (void);
	void ShieldBall	(CSDKPlayer	*pPlayer);
	void UpdateBallShield (void);
	void ShieldOff (void);
	CBaseEntity	*FindPlayerForAction (int team,	int	allowkeepers); //who should	take corner	etc
	CBaseEntity	*FindThrowInEnt	(void);				   //find throwin position marker
	CSDKPlayer	*FindKeeperForAction (int team);	   //who should	take goal kick
//	void  FreezeKeepersForPenalty (int team);		   //freeze	and	position human keepers
//	void  DoubleTouchFoul (CSDKPlayer *player);
	int	KeepersBall	(void);
//	int	KeepersBallBot (void);
	int	KeepersBallHuman (void);
	int	CheckKeeperCatch (CSDKPlayer *keeper);
	int	CheckKeeperKick (CSDKPlayer *keeper);
//	int	CheckHumanKeeperPass (CSDKPlayer *keeper); 

	int	  CheckOffSide (CSDKPlayer *kicker);
	int	  CheckOffsideReceive (CSDKPlayer* pOther);
	int	  CheckOffsideReceiver (CSDKPlayer*	pOther,	int	i);
	int	  PlayerInOwnHalf (CSDKPlayer *pOther);
	void  ClearOffsideFlags	(void);
//	void  ClearGlowShells (void);
	int	  NextOffsideSlot (void);
	void  RecordOffside	(CSDKPlayer	*offside, CSDKPlayer *kicker, CSDKPlayer *against);

	CSDKPlayer		*m_OffsidePlayer[MAX_OFFS];
	CSDKPlayer		*m_OffsideKicker[MAX_OFFS];
	float			m_OffsideTimeout[MAX_OFFS];
	Vector			m_OffsideFoulPos[MAX_OFFS];
	int				m_NumOffsidePlayers;

	float			nextBallSound;
	float			nextShootSound;
	float			m_NextPass;
	float			m_NextTouch;
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

/*
	void		psInit (void);
	void EXPORT	psThink	(void);
	CSDKPlayer *psFindPenTaker (int	team);
	void		psMovePlayers (int end);
	void		psProcessStatus	(void);
	void		psCheckPenaltyKick (CSDKPlayer *kicker);
	int			m_psMode;
	int			m_psNextPen;
	int			m_penaltiesTaken0;
	int			m_penaltiesTaken1;
	int			m_Team0PlayerToTakeTeamPos;
	int			m_Team1PlayerToTakeTeamPos;
	CSDKPlayer *m_PenaltyTaker;
	float		m_PenTime;
	int			m_PenKicked;
*/

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

	void			SendVGUIStatusMessage (const char *title, const char *text, bool bShow, bool bMainStatus=false);
	void			SendMainStatus(int other=0);

	int				m_TeamGoal;			//team that scored
	float			m_KickOffTime;

	void			HandleGoal(void);
	void			HandleCorner(void);
	void			HandleGoalkick(void);
	void			HandleThrowin(void);
	void			HandleFoul(void);
	void			HandlePenalty(void);

	void			RespawnPlayers(void);

	CSDKPlayer		*m_pPowerShotUser;
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

	float			m_FreezeBallTime;
	Vector			m_FreezeBallPos;

	CSDKPlayer*		m_pKeeperParry;

	void			ApplyKickDelayToAll(void);

	void			RemoveFlagFromAll(int flag);

	int				m_KickDelay;

	void			SetPhysics();
};

#endif
