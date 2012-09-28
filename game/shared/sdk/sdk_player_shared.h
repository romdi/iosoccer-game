//============ Copyright © 1996-2008, Valve Corporation, All rights reserved. ===============//
//
// Purpose: Shared Player Variables / Functions and variables that may or may not be networked
//
//===========================================================================================//

#ifndef SDK_PLAYER_SHARED_H
#define SDK_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "weapon_sdkbase.h"

extern const char *g_szRequiredClientVersion;

#ifdef CLIENT_DLL
class C_SDKPlayer;
#else
class CSDKPlayer;
#endif

class CSDKPlayerShared
{
public:

#ifdef CLIENT_DLL
	friend class C_SDKPlayer;
	typedef C_SDKPlayer OuterClass;
	DECLARE_PREDICTABLE();
#else
	friend class CSDKPlayer;
	typedef CSDKPlayer OuterClass;
#endif

	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CSDKPlayerShared );

	CSDKPlayerShared();
	~CSDKPlayerShared();

	OuterClass *GetSDKPlayer() { return m_pOuter; }

	void	SetStamina( float stamina );
	float	GetStamina( void ) { return m_flStamina; }

	void	Init( OuterClass *pOuter );

	bool	IsSniperZoomed( void ) const { return false; };
	bool	IsDucking( void ) const { return false; }; 

	bool	IsJumping( void ) { return m_bJumping; }
	void	SetJumping( bool bJumping );

	void	ForceUnzoom( void );

	bool	IsSprinting( void ) { return m_bIsSprinting; }

	void	SetSprinting( bool bSprinting );
	void	StartSprinting( void );
	void	StopSprinting( void );

	void	SetAnimEvent(PlayerAnimEvent_t animEvent);
	void	ResetAnimEvent();
	PlayerAnimEvent_t GetAnimEvent();
	float	GetAnimEventStart();

	void ResetSprintPenalty( void );

	void ComputeWorldSpaceSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );
	
	float m_flLastViewAnimationTime;

	//Tony; player speeds; at spawn server and client update both of these based on class (if any)
	//float m_flWalkSpeed;
	//float m_flRunSpeed;
	//float m_flSprintSpeed;


	CNetworkVar(bool, m_bIsShotCharging);
	CNetworkVar(bool, m_bDoChargedShot);
	CNetworkVar(float, m_flShotChargingStart);
	CNetworkVar(float, m_flShotChargingDuration);

	CNetworkVar(bool, m_bJumping);
	CNetworkVar(bool, m_bFirstJumpFrame);
	CNetworkVar(float, m_flJumpStartTime);

	CNetworkVar(float, m_flNextJump);
	CNetworkVar(float, m_flNextSlide);
private:

	CNetworkVar(PlayerAnimEvent_t, m_ePlayerAnimEvent);
	CNetworkVar(float, m_flPlayerAnimEventStart);
	CNetworkVar( bool, m_bIsSprinting );
	bool m_bGaveSprintPenalty;
	CNetworkVar( float, m_flStamina );
	OuterClass *m_pOuter;
};			   




#endif //SDK_PLAYER_SHARED_H