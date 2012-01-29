#ifndef C_BALL_H
#define C_BALL_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "c_physicsprop.h"
#include "props_shared.h"
#include "c_props.h"

class C_Ball;

C_Ball *g_pBall = NULL;

class C_Ball : public C_PhysicsProp, public IMultiplayerPhysics
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_Ball, C_PhysicsProp );

	C_Ball()
	{
		g_pBall = this;
	}

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

	int m_iPhysicsMode;	// One of the PHYSICS_MULTIPLAYER_ defines.	
	float m_fMass;
};

C_Ball *GetBall()
{
	return g_pBall;
}

#endif