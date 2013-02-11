//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

#include "sdk_gamerules.h"

#include "takedamageinfo.h"

#include "effect_dispatch_data.h"
#include "weapon_sdkbase.h"
#include "movevars_shared.h"
#include "gamevars_shared.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "engine/ivdebugoverlay.h"
#include "obstacle_pushaway.h"
#include "props_shared.h"

#include "decals.h"
#include "util_shared.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	
	#include "c_sdk_player.h"
	#include "c_team.h"
	#include "prediction.h"
	#include "clientmode_sdk.h"
	#include "vgui_controls/AnimationController.h"
	#include "igameresources.h"

	#define CRecipientFilter C_RecipientFilter
#else
	#include "sdk_player.h"
	#include "team.h"
#endif

ConVar sv_showimpacts("sv_showimpacts", "0", FCVAR_REPLICATED, "Shows client (red) and server (blue) bullet impact point" );

void DispatchEffect( const char *pName, const CEffectData &data );

// BUG! This is not called on the client at respawn, only first spawn!
void CSDKPlayer::SharedSpawn()
{	
	BaseClass::SharedSpawn();

	// Reset the animation state or we will animate to standing
	// when we spawn

	m_Shared.SetJumping( false );
}

void CSDKPlayer::SetSprinting( bool bIsSprinting )
{
	m_Shared.SetSprinting( bIsSprinting );
}

bool CSDKPlayer::IsSprinting( void )
{
	float flVelSqr = GetAbsVelocity().LengthSqr();

	//ios return m_Shared.m_bIsSprinting && ( flVelSqr > 0.5f );
	return m_Shared.IsSprinting() && ( flVelSqr > 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the players mins - overridden for prone
// Input  : 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector CSDKPlayer::GetPlayerMins( void ) const
{
	if ( IsObserver() )
	{
		return VEC_OBS_HULL_MIN;	
	}
	else
	{
		if (GetFlags() & FL_KEEPER_SIDEWAYS_DIVING)
		{
			return VEC_KEEPER_SIDEWAYS_DIVE_HULL_MIN;
		}
		else if (GetFlags() & FL_SLIDING)
		{
			return VEC_SLIDE_HULL_MIN;
		}
		else
		{
			return VEC_HULL_MIN;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:  Returns the players Maxs - overridden for prone
// Input  : 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector CSDKPlayer::GetPlayerMaxs( void ) const
{	
	if ( IsObserver() )
	{
		return VEC_OBS_HULL_MAX;	
	}
	else
	{
		if (GetFlags() & FL_KEEPER_SIDEWAYS_DIVING)
		{
			return VEC_KEEPER_SIDEWAYS_DIVE_HULL_MAX;
		}
		else if (GetFlags() & FL_SLIDING)
		{
			return VEC_SLIDE_HULL_MAX;
		}
		else
		{
			return VEC_HULL_MAX;
		}
	}
}


// --------------------------------------------------------------------------------------------------- //
// CSDKPlayerShared implementation.
// --------------------------------------------------------------------------------------------------- //
CSDKPlayerShared::CSDKPlayerShared()
{
}

CSDKPlayerShared::~CSDKPlayerShared()
{
}

void CSDKPlayerShared::Init( CSDKPlayer *pPlayer )
{
	m_pOuter = pPlayer;
}

void CSDKPlayerShared::SetSprinting( bool bSprinting )
{
	//ios if ( bSprinting && !m_bIsSprinting )
	if ( bSprinting && !IsSprinting())
	{
		StartSprinting();

		// only one penalty per key press
		//if ( m_bGaveSprintPenalty == false )
		//{
		//	m_flStamina -= INITIAL_SPRINT_STAMINA_PENALTY;
		//	m_bGaveSprintPenalty = true;
		//}
		//ios always apply this penalty as we're predicting m_bSprinting
		m_flStamina -= INITIAL_SPRINT_STAMINA_PENALTY;
	}
	//ios else if ( !bSprinting && m_bIsSprinting )
	else if ( !bSprinting && IsSprinting() )
	{
		StopSprinting();
	}
}

// this is reset when we let go of the sprint key
void CSDKPlayerShared::ResetSprintPenalty( void )
{
	m_bGaveSprintPenalty = false;
}

void CSDKPlayerShared::StartSprinting( void )
{
	m_bIsSprinting = true;
}

void CSDKPlayerShared::StopSprinting( void )
{
	m_bIsSprinting = false;
}

void CSDKPlayerShared::SetJumping( bool bJumping )
{
	m_bJumping = bJumping;
}

void CSDKPlayerShared::SetStamina( float flStamina )
{
	m_flStamina = clamp( flStamina, 0, 100 );
}

void CSDKPlayerShared::SetAnimEvent(PlayerAnimEvent_t animEvent)
{
	m_ePlayerAnimEvent = animEvent;
	m_flPlayerAnimEventStartTime = gpGlobals->curtime;
}

//void CSDKPlayerShared::ResetAnimEvent()
//{
//	m_ePlayerAnimEvent = PLAYERANIMEVENT_NONE;
//	GetSDKPlayer()->RemoveFlag(FL_FREECAM);
//}

PlayerAnimEvent_t CSDKPlayerShared::GetAnimEvent()
{
	return m_ePlayerAnimEvent;
}

float CSDKPlayerShared::GetAnimEventStartTime()
{
	return m_flPlayerAnimEventStartTime;
}

QAngle CSDKPlayerShared::GetAnimEventStartAngle()
{
	return QAngle(m_aPlayerAnimEventStartAngle.GetX(), m_aPlayerAnimEventStartAngle.GetY(), m_aPlayerAnimEventStartAngle.GetZ());
}

void CSDKPlayerShared::SetAnimEventStartAngle(QAngle ang)
{
	m_aPlayerAnimEventStartAngle = Vector(ang[PITCH], ang[YAW], ang[ROLL]);
}

int CSDKPlayerShared::GetAnimEventStartButtons()
{
	return m_nPlayerAnimEventStartButtons;
}

void CSDKPlayerShared::SetAnimEventStartButtons(int buttons)
{
	m_nPlayerAnimEventStartButtons = buttons;
}

void CSDKPlayerShared::ComputeWorldSpaceSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	Vector org = GetSDKPlayer()->GetAbsOrigin();

	static Vector vecMin(-32, -32, 0 );
	static Vector vecMax(32, 32, 72 );

	VectorAdd( vecMin, org, *pVecWorldMins );
	VectorAdd( vecMax, org, *pVecWorldMaxs );
}

void CSDKPlayer::InitSpeeds()
{
	//m_Shared.m_flWalkSpeed = mp_walkspeed.GetInt();
	//m_Shared.m_flRunSpeed = mp_runspeed.GetInt();
	//m_Shared.m_flSprintSpeed = mp_sprintspeed.GetInt();
	// Set the absolute max to sprint speed
	//SetMaxSpeed( m_Shared.m_flRunSpeed ); 
	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSDKPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

void CSDKPlayer::MoveToTargetPos(Vector &pos, Vector &vel, QAngle &ang)
{
	Vector dir = m_vTargetPos - pos;
	dir.z = 0;
	float distToTarget = dir.Length2D();
	dir.NormalizeInPlace();

	VectorAngles(dir, ang);

	float wishDist = mp_remotecontrolledspeed.GetInt() * gpGlobals->frametime;
	vel = dir * mp_remotecontrolledspeed.GetInt();
	pos = pos + vel * gpGlobals->frametime;
	pos.z = SDKGameRules()->m_vKickOff.GetZ();

	if (wishDist >= distToTarget)
	{
		trace_t	trace;
		UTIL_TraceHull(m_vTargetPos, m_vTargetPos, GetPlayerMins(), GetPlayerMaxs(), MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER, &trace);

		if (trace.startsolid)
		{
			m_vTargetPos = pos + dir * (GetPlayerMaxs().x - GetPlayerMins().x);
		}
		else
		{
			m_bIsAtTargetPos = true;
			RemoveFlag(FL_REMOTECONTROLLED);
			//RemoveSolidFlags(FSOLID_NOT_SOLID);
			SetCollisionGroup(COLLISION_GROUP_PLAYER);

			if (m_bHoldAtTargetPos)
				AddFlag(FL_ATCONTROLS);

//#ifdef GAME_DLL
//			if (ShotButtonsPressed())
//				m_bShotButtonsReleased = false;
//#endif

			pos = m_vTargetPos;
			vel = vec3_origin;
		}
	}
}

void CSDKPlayer::CheckBallShield(const Vector &oldPos, Vector &newPos, const Vector &oldVel, Vector &newVel, const QAngle &oldAng, QAngle &newAng)
{
	bool stopPlayer = false;
	const float border = (GetFlags() & FL_SHIELD_KEEP_IN) ? -mp_shield_border.GetInt() : mp_shield_border.GetInt();

	if (SDKGameRules()->m_nShieldType != SHIELD_NONE)
	{
		if (SDKGameRules()->m_nShieldType == SHIELD_GOALKICK || 
			SDKGameRules()->m_nShieldType == SHIELD_PENALTY ||
			SDKGameRules()->m_nShieldType == SHIELD_FREEKICK ||
			SDKGameRules()->m_nShieldType == SHIELD_CORNER)
		{
			const float radius = mp_shield_ball_radius.GetFloat();
			Vector dir = newPos - SDKGameRules()->m_vShieldPos;

			if (dir.Length2DSqr() < pow(radius, 2))
			{
				dir.z = 0;
				dir.NormalizeInPlace();
				newPos = SDKGameRules()->m_vShieldPos + dir * radius;
				stopPlayer = true;
			}
		}

		if (SDKGameRules()->m_nShieldType == SHIELD_GOALKICK || 
			SDKGameRules()->m_nShieldType == SHIELD_PENALTY ||
			SDKGameRules()->m_nShieldType == SHIELD_KEEPERHANDS)
		{
			int side = (SDKGameRules()->m_nShieldType == SHIELD_PENALTY ? GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->GetOppTeamNumber() : SDKGameRules()->m_nShieldTeam);
			Vector min = GetGlobalTeam(side)->m_vPenBoxMin - border;
			Vector max = GetGlobalTeam(side)->m_vPenBoxMax + border;

			if (GetFlags() & FL_SHIELD_KEEP_OUT || SDKGameRules()->m_nShieldType == SHIELD_PENALTY)
			{
				if (SDKGameRules()->m_vKickOff.GetY() > min.y)
					min.y -= 500;
				else
					max.y += 500;
			}

			bool isInsideBox = newPos.x > min.x && newPos.y > min.y && newPos.x < max.x && newPos.y < max.y; 
			Vector boxCenter = (min + max) / 2;

			if (GetFlags() & FL_SHIELD_KEEP_OUT && isInsideBox)
			{
				bool oldPosInBox = true;
				if (newPos.x > min.x && oldPos.x <= min.x && newPos.x < boxCenter.x)
				{
					newPos.x = min.x;
					oldPosInBox = false; 
				}
				else if (newPos.x < max.x && oldPos.x >= max.x && newPos.x > boxCenter.x)
				{
					newPos.x = max.x;
					oldPosInBox = false; 
				}

				if (newPos.y > min.y && oldPos.y <= min.y && newPos.y < boxCenter.y)
				{
					newPos.y = min.y;
					oldPosInBox = false; 
				}
				else if (newPos.y < max.y && oldPos.y >= max.y && newPos.y > boxCenter.y)
				{
					newPos.y = max.y;
					oldPosInBox = false; 
				}

				stopPlayer = true;

				if (SDKGameRules()->m_nShieldType == SHIELD_KEEPERHANDS && oldPosInBox)
				{
					Vector goalCenter = (GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->m_vCornerLeft + GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->m_vCornerRight) / 2;
					goalCenter.y -= Sign(GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->m_nForward) * 500;

					if ((goalCenter - newPos).Length2DSqr() < (goalCenter - oldPos).Length2DSqr())
					{
						newPos.x = oldPos.x;
						newPos.y = oldPos.y;
						stopPlayer = true;
					}
					else
						stopPlayer = false;
				}
			}
			else if (GetFlags() & FL_SHIELD_KEEP_IN && !isInsideBox)
			{
				if (newPos.x < min.x)
					newPos.x = min.x;
				else if (newPos.x > max.x)
					newPos.x = max.x;

				if (newPos.y < min.y)
					newPos.y = min.y;
				else if (newPos.y > max.y)
					newPos.y = max.y;

				stopPlayer = true;
			}
		}

		if (SDKGameRules()->m_nShieldType == SHIELD_THROWIN || 
			SDKGameRules()->m_nShieldType == SHIELD_FREEKICK || 
			SDKGameRules()->m_nShieldType == SHIELD_CORNER ||  
			SDKGameRules()->m_nShieldType == SHIELD_KICKOFF ||
			SDKGameRules()->m_nShieldType == SHIELD_PENALTY && (GetFlags() & FL_SHIELD_KEEP_OUT))
		{
			float radius = SDKGameRules()->GetShieldRadius() + border;
			Vector dir = newPos - SDKGameRules()->m_vShieldPos;

			if ((GetFlags() & FL_SHIELD_KEEP_OUT && dir.Length2D() < radius || GetFlags() & FL_SHIELD_KEEP_IN && dir.Length2D() > radius)
				&& (!SDKGameRules()->IsIntermissionState() || mp_shield_block_opponent_half.GetBool()))
			{
				dir.z = 0;
				dir.NormalizeInPlace();
				newPos = SDKGameRules()->m_vShieldPos + dir * radius;
				stopPlayer = true;
			}

			if (SDKGameRules()->m_nShieldType == SHIELD_KICKOFF && (GetFlags() & FL_SHIELD_KEEP_OUT)
				&& (!SDKGameRules()->IsIntermissionState() || mp_shield_block_opponent_half.GetBool()))
			{
				int forward;
				#ifdef CLIENT_DLL
					forward = GetPlayersTeam(this)->m_nForward;
				#else
					forward = GetTeam()->m_nForward;
				#endif
				float yBorder = SDKGameRules()->m_vKickOff.GetY() - abs(border) * forward;
				if (Sign(newPos.y - yBorder) == forward)
				{
					newPos.y = yBorder;
					stopPlayer = true;
				}
			}

			if (SDKGameRules()->m_nShieldType == SHIELD_FREEKICK && mp_shield_block_6yardbox.GetBool())
			{
				int teamPosType;
				#ifdef CLIENT_DLL
					teamPosType = GameResources()->GetTeamPosType(entindex());
				#else
					teamPosType = GetTeamPosType();
				#endif
				if (teamPosType != GK || GetTeamNumber() != GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->GetOppTeamNumber())
				{
					int side = GetGlobalTeam(SDKGameRules()->m_nShieldTeam)->GetOppTeamNumber();
					int boxLength = abs(GetGlobalTeam(side)->m_vPenBoxMax.GetY() - GetGlobalTeam(side)->m_vPenBoxMin.GetY()) / 3.0f;
					Vector min = GetGlobalTeam(side)->m_vPenBoxMin + Vector(boxLength * 2, 0, 0) - border;
					Vector max = GetGlobalTeam(side)->m_vPenBoxMax - Vector(boxLength * 2, 0, 0) + border;

					if (GetGlobalTeam(side)->m_nForward == 1)
					{
						max.y -= boxLength * 2;
						min.y -= 500;
					}
					else
					{
						min.y += boxLength * 2;
						max.y += 500;
					}

					bool isInsideBox = newPos.x > min.x && newPos.y > min.y && newPos.x < max.x && newPos.y < max.y; 
					Vector boxCenter = (min + max) / 2;

					if (isInsideBox)
					{
						if (newPos.x > min.x && oldPos.x <= min.x && newPos.x < boxCenter.x)
							newPos.x = min.x;
						else if (newPos.x < max.x && oldPos.x >= max.x && newPos.x > boxCenter.x)
							newPos.x = max.x;

						if (newPos.y > min.y && oldPos.y <= min.y && newPos.y < boxCenter.y)
							newPos.y = min.y;
						else if (newPos.y < max.y && oldPos.y >= max.y && newPos.y > boxCenter.y)
							newPos.y = max.y;

						stopPlayer = true;
					}
				}
			}
		}
	}

	if (!SDKGameRules()->IsIntermissionState() && mp_field_border_enabled.GetBool())
	{
		float border = mp_field_border.GetInt();
		Vector min = SDKGameRules()->m_vFieldMin - border;
		Vector max = SDKGameRules()->m_vFieldMax + border;

		if (newPos.x < min.x || newPos.y < min.y || newPos.x > max.x || newPos.y > max.y)
		{
			if (newPos.x < min.x)
				newPos.x = min.x;
			else if (newPos.x > max.x)
				newPos.x = max.x;

			if (newPos.y < min.y)
				newPos.y = min.y;
			else if (newPos.y > max.y)
				newPos.y = max.y;

			stopPlayer = true;
		}
	}

	if (stopPlayer)
	{
		//newVel = oldVel;
		trace_t	trace;
		UTIL_TraceHull(newPos, newPos, GetPlayerMins(), GetPlayerMaxs(), MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER, &trace);

		if (trace.startsolid)
		{
			newPos = oldPos;
		}

		newVel.x = (newPos - oldPos).x * 50;
		newVel.y = (newPos - oldPos).y * 50;
		//newPos = pos;
	}
}

void CSDKPlayer::FindSafePos(Vector &startPos)
{
	bool hasSafePos = false;
	int maxCheckDist = (GetPlayerMaxs().x - GetPlayerMins().x) * 5;

	for (int x = 0; x < maxCheckDist; x++)
	{
		for (int y = 0; y < maxCheckDist * 10; y++)
		{
			for (int sign = -1; sign <= 1; sign += 2)
			{
				Vector checkPos = startPos + Vector(x, y, 0);
				trace_t	trace;
				UTIL_TraceHull(checkPos, checkPos, GetPlayerMins(), GetPlayerMaxs(), MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER, &trace);

				if (!trace.startsolid)
				{
					hasSafePos = true;
					startPos = checkPos;
					break;
				}
			}

			if (hasSafePos)
				break;
		}

		if (hasSafePos)
			break;
	}

	if (!hasSafePos)
		startPos.z += GetPlayerMaxs().z * 2;
}

void CSDKPlayer::CheckShotCharging()
{
	if ((m_nButtons & IN_ATTACK))
	{
		if (m_Shared.m_bDoChargedShot)
			m_Shared.m_bDoChargedShot = false;

		if (m_Shared.m_bIsShotCharging)
			m_Shared.m_bIsShotCharging = false;
	}
	else if ((m_nButtons & IN_ATTACK2) && !m_Shared.m_bIsShotCharging)
	{
		m_Shared.m_bDoChargedShot = false;
		m_Shared.m_bIsShotCharging = true;
		m_Shared.m_flShotChargingStart = gpGlobals->curtime;
	}
	else if (m_Shared.m_bIsShotCharging)
	{
		if ((m_nButtons & IN_ATTACK2))
		{
			//m_flShotChargingDuration = gpGlobals->curtime - m_flShotChargingStart;
			//float fraction = m_flShotChargingDuration / mp_chargedshot_increaseduration.GetFloat();
		}
		else
		{
			m_Shared.m_bIsShotCharging = false;
			m_Shared.m_flShotChargingDuration = gpGlobals->curtime - m_Shared.m_flShotChargingStart;
			m_Shared.m_bDoChargedShot = true;
		}
	}
}

void CSDKPlayer::ResetShotCharging()
{
	m_Shared.m_bDoChargedShot = false;
	m_Shared.m_bIsShotCharging = false;
	#ifdef CLIENT_DLL
	if (this == C_SDKPlayer::GetLocalSDKPlayer() && mp_reset_spin_toggles_on_shot.GetBool())
	{
		engine->ClientCmd("-topspin");
		engine->ClientCmd("-backspin");
	}
	#endif	
}