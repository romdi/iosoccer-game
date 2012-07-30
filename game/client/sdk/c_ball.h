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
	int DrawModel(int flags);

	int m_iPhysicsMode;	// One of the PHYSICS_MULTIPLAYER_ defines.	
	float m_fMass;
	float m_flOffsideLineBallPosY;
	float m_flOffsideLineOffsidePlayerPosY;
	float m_flOffsideLineLastOppPlayerPosY;
	bool m_bOffsideLinesEnabled;
	IMaterial *m_pOffsideLineMaterial;
	CHandle<C_SDKPlayer> m_pPl;
	CHandle<C_SDKPlayer> m_pCreator;
};

extern C_Ball *GetBall();

#endif