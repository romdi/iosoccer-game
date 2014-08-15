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
#include "ios_teamkit_parse.h"

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


	const char *GetSkinName()
	{
		return m_szSkinName;
	}

	virtual bool ShouldInterpolate();

	virtual void ClientThink();

	void OnDataChanged(DataUpdateType_t updateType);

	virtual	bool		ShouldCollide( int collisionGroup, int contentsMask ) const;

	int m_iPhysicsMode;	// One of the PHYSICS_MULTIPLAYER_ defines.	
	float m_fMass;
	//CHandle<C_SDKPlayer> m_pCreator;
	CHandle<C_SDKPlayer> m_pHoldingPlayer;
	bool m_bIsPlayerBall;
	ball_state_t m_eBallState;

	inline ball_state_t State_Get() { return m_eBallState; }

private:

	char m_szSkinName[MAX_KITNAME_LENGTH];
};

#endif