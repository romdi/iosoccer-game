#ifndef C_BALL_H
#define C_BALL_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "c_physicsprop.h"
#include "props_shared.h"
#include "c_props.h"
#include "c_sdk_player.h"

class C_Ball;

extern C_Ball *g_pBall;

class C_Ball : public C_PhysicsProp, public IMultiplayerPhysics
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_Ball, C_PhysicsProp );

	C_Ball();
	~C_Ball();

	virtual int GetMultiplayerPhysicsMode()
	{
		Assert( m_iPhysicsMode != PHYSICS_MULTIPLAYER_CLIENTSIDE );
		Assert( m_iPhysicsMode != PHYSICS_MULTIPLAYER_AUTODETECT );
		return m_iPhysicsMode;
	}

	virtual float GetMass()
	{
		return m_fMass;
	}
	virtual bool IsAsleep()
	{
		return false;
	}

	void OnDataChanged(DataUpdateType_t updateType);

	int m_iPhysicsMode;	// One of the PHYSICS_MULTIPLAYER_ defines.	
	float m_fMass;
	CHandle<C_SDKPlayer> m_pCreator;
	CHandle<C_SDKPlayer> m_pMatchEventPlayer;
	int m_nMatchEventTeam;
	CHandle<C_SDKPlayer> m_pMatchSubEventPlayer;
	int m_nMatchSubEventTeam;
	bool m_bIsPlayerBall;
	match_event_t m_eMatchEvent;
	match_event_t m_eMatchSubEvent;
	ball_state_t m_eBallState;
};

extern C_Ball *GetBall();

#endif