//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( GAMEMOVEMENT_H )
#define GAMEMOVEMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "igamemovement.h"
#include "cmodel.h"
#include "tier0/vprof.h"

#define CTEXTURESMAX		512			// max number of textures loaded
#define CBTEXTURENAMEMAX	13			// only load first n chars of name

#define GAMEMOVEMENT_JUMP_TIME				510.0f		// ms approx - based on the 21 unit height jump
//ios #define GAMEMOVEMENT_JUMP_HEIGHT			21.0f		// units
#define GAMEMOVEMENT_JUMP_HEIGHT			15.0f		// units

struct surfacedata_t;

class CBasePlayer;

class CGameMovement : public IGameMovement
{
public:
	DECLARE_CLASS_NOBASE( CGameMovement );
	
	CGameMovement( void );
	virtual			~CGameMovement( void );

	virtual void	SetPlayerSpeed( void );

	virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove );

	virtual void	StartTrackPredictionErrors( CBasePlayer *pPlayer );
	virtual void	FinishTrackPredictionErrors( CBasePlayer *pPlayer );
	virtual void	DiffPrint( char const *fmt, ... );
	virtual const Vector&	GetPlayerMins( bool ducked ) const { return VEC_HULL_MIN; };
	virtual const Vector&	GetPlayerMaxs( bool ducked ) const { return VEC_HULL_MAX; };
	virtual const Vector&	GetPlayerViewOffset( bool ducked ) const { return VEC_VIEW; };

// For sanity checking getting stuck on CMoveData::SetAbsOrigin
	virtual void			TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm );
#define BRUSH_ONLY true
	virtual unsigned int PlayerSolidMask( bool brushOnly = false );	///< returns the solid mask for the given player, so bots can have a more-restrictive set
	CBasePlayer		*player;
	CMoveData *GetMoveData() { return mv; }
protected:
	// Input/Output for this movement
	CMoveData		*mv;

	Vector			m_vecForward;
	Vector			m_vecRight;
	Vector			m_vecUp;

	// Does most of the player movement logic.
	// Returns with origin, angles, and velocity modified in place.
	// were contacted during the move.
	virtual void	PlayerMove(	void );

	void			CheckBallShield(Vector oldPos);

	// Set ground data, etc.
	void			FinishMove( void );

	// Handles both ground friction and water friction
	void			Friction( void );

	virtual void	AirAccelerate( Vector& wishdir, float wishspeed, float accel );

	virtual void	AirMove( void );
	
	virtual bool	CanAccelerate();
	virtual void	Accelerate( Vector& wishdir, float wishspeed, float accel);

	// Only used by players.  Moves along the ground when player is a MOVETYPE_WALK.
	virtual void	WalkMove( void );

	// Try to keep a walking player on the ground when running down slopes etc
	void			StayOnGround( void );

	// Handle MOVETYPE_WALK.
	virtual void	FullWalkMove();

	// Implement this if you want to know when the player collides during OnPlayerMove
	virtual void	OnTryPlayerMoveCollision( trace_t &tr ) {}

	virtual const Vector&	GetPlayerMins( void ) const { return VEC_HULL_MIN; }; // uses local player
	virtual const Vector&	GetPlayerMaxs( void ) const { return VEC_HULL_MAX; }; // uses local player

	typedef enum
	{
		GROUND = 0,
		STUCK,
		LADDER
	} IntervalType_t;

	virtual int		GetCheckInterval( IntervalType_t type );

	// Useful for things that happen periodically. This lets things happen on the specified interval, but
	// spaces the events onto different frames for different players so they don't all hit their spikes
	// simultaneously.
	bool			CheckInterval( IntervalType_t type );


	// Decompoosed gravity
	void			StartGravity( void );
	void			FinishGravity( void );

	// Apply normal ( undecomposed ) gravity
	void			AddGravity( void );

	// Handle movement in noclip mode.
	void			FullNoClipMove( float factor, float maxacceleration );

	// Returns true if he started a jump (ie: should he play the jump animation)?
	virtual bool	CheckJumpButton( void );	// Overridden by each game.
	
	// Player is a Observer chasing another player
	void			FullObserverMove( void );

	// The basic solid body movement clip that slides along multiple planes
	virtual int		TryPlayerMove( Vector *pFirstDest=NULL, trace_t *pFirstTrace=NULL );
	
	// See if the player has a bogus velocity value.
	void			CheckVelocity( void );

	// Does not change the entities velocity at all
	void			PushEntity( Vector& push, trace_t *pTrace );

	// Slide off of the impacting object
	// returns the blocked flags:
	// 0x01 == floor
	// 0x02 == step / wall
	int				ClipVelocity( Vector& in, Vector& normal, Vector& out, float overbounce );

	// If pmove.origin is in a solid position,
	// try nudging slightly on all axis to
	// allow for the cut precision of the net coordinates
	
	// Determine if player is in water, on ground, etc.
	virtual void CategorizePosition( void );

	virtual void	CheckParameters( void );

	virtual	void	ReduceTimers( void );

	bool			IsDead( void ) const;

	virtual void	SetGroundEntity( trace_t *pm );

	virtual void	StepMove( Vector &vecDestination, trace_t &trace );

protected:

	// Performs the collision resolution for fliers.
	void			PerformFlyCollisionResolution( trace_t &pm, Vector &move );

	enum
	{
		// eyes, waist, feet points (since they are all deterministic
		MAX_PC_CACHE_SLOTS = 3,
	};

	// Cache used to remove redundant calls to GetPointContents().
	int m_CachedGetPointContents[ MAX_PLAYERS ][ MAX_PC_CACHE_SLOTS ];
	Vector m_CachedGetPointContentsPoint[ MAX_PLAYERS ][ MAX_PC_CACHE_SLOTS ];	

	Vector			m_vecProximityMins;		// Used to be globals in sv_user.cpp.
	Vector			m_vecProximityMaxs;

	float			m_fFrameTime;

//private:
	bool			m_bSpeedCropped;
};


//-----------------------------------------------------------------------------
// Traces player movement + position
//-----------------------------------------------------------------------------
inline void CGameMovement::TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm )
{
	VPROF( "CGameMovement::TracePlayerBBox" );

	Ray_t ray;
	ray.Init( start, end, GetPlayerMins(), GetPlayerMaxs() );
	UTIL_TraceRay( ray, fMask, mv->m_nPlayerHandle.Get(), collisionGroup, &pm );
}

#endif // GAMEMOVEMENT_H
