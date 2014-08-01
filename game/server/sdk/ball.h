#ifndef _BALL_H
#define _BALL_H

#include "props_shared.h"
#include "props.h"
#include "ios_teamkit_parse.h"
#include "sdk_player.h"

#define	BALL_MODEL	 "models/ball/ball.mdl"

extern ConVar
	sv_ball_mass,
	sv_ball_damping,
	sv_ball_rotdamping,
	sv_ball_rotinertialimit,
	sv_ball_dragcoeff,
	sv_ball_inertia,
	sv_ball_drag_enabled,
	
	sv_ball_spin,
	sv_ball_spin_exponent,
	sv_ball_defaultspin,
	sv_ball_topspin_coeff,
	sv_ball_backspin_coeff,
	sv_ball_curve,
	
	sv_ball_deflectionradius,
	sv_ball_collisionradius,
	
	sv_ball_standing_reach_shortside,
	sv_ball_standing_reach_longside,
	sv_ball_standing_reach_shift,
	sv_ball_standing_reach_ellipse,
	
	sv_ball_slidesidereach_ball,
	sv_ball_slideforwardreach_ball,
	sv_ball_slidebackwardreach_ball,
	
	sv_ball_slidesidereach_foul,
	sv_ball_slideforwardreach_foul,
	sv_ball_slidebackwardreach_foul,
	
	sv_ball_slidesidespeedcoeff,
	sv_ball_slidezstart,
	sv_ball_slidezend,
	
	sv_ball_keeper_standing_reach_top,
	sv_ball_keeper_standing_reach_bottom,
	sv_ball_keeper_standing_catchcenteroffset_side,
	sv_ball_keeper_standing_catchcenteroffset_z,
	
	sv_ball_keeper_forwarddive_shortsidereach,
	sv_ball_keeper_forwarddive_longsidereach,
	sv_ball_keeper_forwarddive_longsidereach_opposite,
	sv_ball_keeper_forwarddive_zstart,
	sv_ball_keeper_forwarddive_zend,
	sv_ball_keeper_forwarddive_catchcoeff,
	
	sv_ball_keeper_sidedive_shortsidereach,
	sv_ball_keeper_sidedive_longsidereach,
	sv_ball_keeper_sidedive_longsidereach_opposite,
	sv_ball_keeper_sidedive_zstart,
	sv_ball_keeper_sidedive_zend,
	sv_ball_keeper_sidedive_catchcenteroffset_side,
	sv_ball_keeper_sidedive_catchcenteroffset_z,
	
	sv_ball_keeper_punch_maxyawangle,
	sv_ball_keeper_punch_maxpitchangle,
	sv_ball_keeper_punch_pitchoffset,
	sv_ball_keeper_punch_shortsidecoeff,
	sv_ball_keeper_punch_minstrength,
	sv_ball_keeper_punch_minpitchangle,
	
	sv_ball_keeperpunchupstrength,
	sv_ball_keeperdeflectioncoeff,
	
	sv_ball_shotdelay_setpiece,
	sv_ball_shotdelay_global_coeff,
	sv_ball_keepercatchdelay_sidedive_global_coeff,
	sv_ball_keepercatchdelay_forwarddive_global_coeff,
	sv_ball_keepercatchdelay_standing_global_coeff,
	sv_ball_dynamicshotdelay_mindelay,
	sv_ball_dynamicshotdelay_maxdelay,
	sv_ball_dynamicshotdelay_minshotstrength,
	sv_ball_dynamicshotdelay_maxshotstrength,
	sv_ball_dynamicbounce_enabled,
	
	sv_ball_bestshotangle,
	
	sv_ball_pitchdown_exponent_normalshot,
	sv_ball_fixedpitchdowncoeff_normalshot,
	sv_ball_pitchdown_exponent_nonnormalshot,
	sv_ball_fixedpitchdowncoeff_nonnormalshot,
	sv_ball_pitchup_exponent_normalshot,
	sv_ball_fixedpitchupcoeff_normalshot,
	sv_ball_pitchup_exponent_nonnormalshot,
	sv_ball_fixedpitchupcoeff_nonnormalshot,
	
	sv_ball_bestbackspinangle_start,
	sv_ball_bestbackspinangle_end,
	
	sv_ball_besttopspinangle_start,
	sv_ball_besttopspinangle_end,
	
	sv_ball_keepercatchspeed,
	sv_ball_keeperpickupangle,
	
	sv_ball_normalshot_strength,
	sv_ball_powershot_strength,
	sv_ball_chargedshot_minstrength,
	sv_ball_chargedshot_maxstrength,
	
	sv_ball_powerthrow_strength,
	sv_ball_chargedthrow_minstrength,
	sv_ball_chargedthrow_maxstrength,
	
	sv_ball_normalheader_strength,
	sv_ball_powerheader_strength,
	sv_ball_chargedheader_minstrength,
	sv_ball_chargedheader_maxstrength,
	
	sv_ball_powerdivingheader_strength,
	sv_ball_chargeddivingheader_minstrength,
	sv_ball_chargeddivingheader_maxstrength,
	
	sv_ball_header_mindelay,
	
	sv_ball_slide_strength,
	
	sv_ball_goalkick_speedcoeff,
	sv_ball_freekick_speedcoeff,
	sv_ball_volleyshot_speedcoeff,
	
	sv_ball_keepershot_minangle,
	
	sv_ball_groundshot_minangle,
	sv_ball_volleyshot_minangle,
	sv_ball_throwin_minangle,
	sv_ball_throwin_minstrength,
	sv_ball_autopass_minstrength,
	sv_ball_autopass_maxstrength,
	sv_ball_autopass_coeff,
	sv_ball_volleyshot_spincoeff,
	sv_ball_rainbowflick_spincoeff,
	sv_ball_rainbowflick_angle,
	sv_ball_rainbowflick_dist,
	sv_ball_header_spincoeff,
	sv_ball_doubletouchfouls,
	
	sv_ball_timelimit_setpiece,
	sv_ball_timelimit_remotecontrolled,
	
	sv_ball_setpiece_close_time,
	sv_ball_setpiece_close_dist,
	
	sv_ball_statetransition_activationdelay_short,
	sv_ball_statetransition_activationdelay_normal,
	sv_ball_statetransition_activationdelay_long,
	sv_ball_statetransition_messagedelay_normal,
	sv_ball_statetransition_messagedelay_short,
	
	sv_ball_goalcelebduration,
	sv_ball_highlightsdelay_intermissions,
	sv_ball_highlightsdelay_cooldown,
	sv_ball_thinkinterval,
	sv_ball_chestdrop_strength,
	sv_ball_chestdrop_angle,
	sv_ball_minshotstrength,
	sv_ball_minspeed_passive,
	sv_ball_minspeed_bounce,
	sv_ball_bounce_strength,
	sv_ball_player_yellow_red_card_duration,
	sv_ball_player_red_card_duration,
	
	sv_ball_bodypos_feet_start,
	sv_ball_bodypos_hip_start,
	sv_ball_bodypos_head_start,
	sv_ball_bodypos_head_end,
	sv_ball_bodypos_keeperarms_end,
	
	sv_ball_bodypos_keeperhands,
	
	sv_ball_bodypos_collision_start,
	sv_ball_bodypos_collision_end,
	
	sv_ball_bodypos_deflection_start,
	sv_ball_bodypos_deflection_end,
	
	sv_ball_yellowcardballdist_forward,
	sv_ball_yellowcardballdist_backward,
	sv_ball_goalreplay_count,
	sv_ball_goalreplay_delay,
	sv_ball_deflectioncoeff,
	sv_ball_collisioncoeff,
	sv_ball_update_physics,
	
	sv_ball_stats_pass_mindist,
	sv_ball_stats_clearance_minspeed,
	sv_ball_stats_shot_mindist,
	sv_ball_stats_save_minspeed,
	sv_ball_stats_assist_maxtime,
	
	sv_ball_velocity_coeff,
	
	sv_ball_freekickdist_owngoal,
	sv_ball_freekickdist_opponentgoal,
	sv_ball_freekickangle_opponentgoal,
	sv_ball_closetogoaldist,
	
	sv_ball_nonnormalshotsblocktime_freekick,
	sv_ball_nonnormalshotsblocktime_corner,
	sv_ball_shotsblocktime_penalty,
	
	sv_ball_maxcheckdist,
	
	sv_ball_freecamshot_maxangle,
	sv_ball_heelshot_strength,
	
	sv_ball_offsidedist,
	
	sv_ball_turnovertime;

//class CSDKPlayer;

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
	CNetworkVar(bool, m_bNonnormalshotsBlocked);
	CNetworkVar(bool, m_bShotsBlocked);
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
	void			VPhysicsCollision( int index, gamevcollisionevent_t	*pEvent	);
	void			VPhysicsUpdate(IPhysicsObject *pPhysics);
	bool			CreateVPhysics();
	
	virtual void	TriggerGoal(int team) = 0;
	virtual void	TriggerGoalLine(int team) = 0;
	virtual void	TriggerSideline() = 0;
	void			TriggerPenaltyBox(int team);
	bool			IsSettingNewPos() { return m_bSetNewPos; }
	bool			HasQueuedState() { return m_bHasQueuedState; }

	virtual void	Reset();
	void			ReloadSettings();

	void			SetPos(const Vector &pos, bool teleport = true);
	void			SetAng(const QAngle &ang);
	virtual void	SetVel(Vector vel, float spinCoeff, int spinFlags, body_part_t bodyPart, bool isDeflection, bool markOffsidePlayers, bool ensureMinShotStrength, float nextShotMinDelay = 0);
	void			SetRot(AngularImpulse rot = NULL);

	void			SetPenaltyState(penalty_state_t penaltyState) { m_ePenaltyState = penaltyState; }
	penalty_state_t	GetPenaltyState() { return m_ePenaltyState; }

	void			SetPenaltyTaker(CSDKPlayer *pPl);

	inline ball_state_t State_Get( void ) { return m_pCurStateInfo->m_eBallState; }

	CSDKPlayer		*GetCurrentPlayer() { return m_pPl; }
	CSDKPlayer		*GetHoldingPlayer() { return m_pHoldingPlayer; }
	void			EnablePlayerCollisions(bool enable);
	void			RemoveFromPlayerHands(CSDKPlayer *pPl);
	Vector			GetPos();
	QAngle			GetAng();
	Vector			GetVel();
	AngularImpulse	GetRot();
	float			CalcFieldZone();
	void			UpdatePossession(CSDKPlayer *pNewPossessor);

	const char		*GetSkinName() { return m_szSkinName; }
	void			SetSkinName(const char *skinName);

	virtual void State_Transition(ball_state_t newState, float delay = 0.0f, bool cancelQueuedState = false, bool isShortMessageDelay = false) = 0;

protected:

	virtual void State_KEEPERHANDS_Enter();			virtual void State_KEEPERHANDS_Think();			virtual void State_KEEPERHANDS_Leave(ball_state_t newState);

	virtual void State_STATIC_Enter() {};			virtual void State_STATIC_Think() {};		virtual void State_STATIC_Leave(ball_state_t newState) {};
	virtual void State_NORMAL_Enter() {};			virtual void State_NORMAL_Think() {};			virtual void State_NORMAL_Leave(ball_state_t newState)  {};
	virtual void State_KICKOFF_Enter() {};			virtual void State_KICKOFF_Think() {};			virtual void State_KICKOFF_Leave(ball_state_t newState) {};
	virtual void State_THROWIN_Enter() {};			virtual void State_THROWIN_Think() {};			virtual void State_THROWIN_Leave(ball_state_t newState) {};
	virtual void State_GOALKICK_Enter() {};			virtual void State_GOALKICK_Think() {};			virtual void State_GOALKICK_Leave(ball_state_t newState) {};
	virtual void State_CORNER_Enter() {};			virtual void State_CORNER_Think() {};			virtual void State_CORNER_Leave(ball_state_t newState) {};
	virtual void State_GOAL_Enter() {};				virtual void State_GOAL_Think() {};				virtual void State_GOAL_Leave(ball_state_t newState) {};
	virtual void State_FREEKICK_Enter() {};			virtual void State_FREEKICK_Think() {};			virtual void State_FREEKICK_Leave(ball_state_t newState) {};
	virtual void State_PENALTY_Enter() {};			virtual void State_PENALTY_Think() {};			virtual void State_PENALTY_Leave(ball_state_t newState) {};

	virtual void State_Enter(ball_state_t newState, bool cancelQueuedState) = 0;	// Initialize the new state.
	virtual void State_Think() = 0;										// Update the current state.
	virtual void State_Leave(ball_state_t newState) = 0;
	static CBallStateInfo* State_LookupInfo(ball_state_t state);	// Find the state info for the specified state.

	void			FindStatePlayer(ball_state_t ballState = BALL_STATE_NONE);

	ball_state_t	m_eNextState;
	float			m_flStateTimelimit;
	CBallStateInfo	*m_pCurStateInfo;

	void			MarkOffsidePlayers();
	void			UnmarkOffsidePlayers();

	void			HandleFoul();
	bool			PlayersAtTargetPos();
	bool			CanTouchBallXY();
	bool			IsPlayerClose();
	bool			CheckFoul(bool canShootBall, const Vector &localDirToBall);
	void			TriggerFoul(foul_type_t type, Vector pos, CSDKPlayer *pFoulingPl, CSDKPlayer *pFouledPl = NULL);
	CSDKPlayer		*FindNearestPlayer(int team = TEAM_INVALID, int posFlags = FL_POS_FIELD, bool checkIfShooting = false, int ignoredPlayerBits = 0, float radius = -1);
	bool			IsInDeflectRange(bool isCollision);
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

	CHandle<CSDKPlayer>	m_pTurnoverPlayer;

	CUtlVector<BallTouchInfo *> m_Touches;

	penalty_state_t m_ePenaltyState;

	MatchEventPlayerInfo m_MatchEventPlayerInfo;

	//CHandle<CSDKPlayer> m_pHoldingPlayer;

	float			m_flGlobalLastShot;
	float			m_flGlobalDynamicShotDelay;

	float			m_flLastMatchEventSetTime;

	bool			m_bHitThePost;
};

#endif
