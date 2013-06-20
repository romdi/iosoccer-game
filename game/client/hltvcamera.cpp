//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hltvcamera.h"
#include "cdll_client_int.h"
#include "util_shared.h"
#include "prediction.h"
#include "movevars_shared.h"
#include "in_buttons.h"
#include "text_message.h"
#include "vgui_controls/controls.h"
#include "vgui/ILocalize.h"
#include "vguicenterprint.h"
#include "game/client/iviewport.h"
#include <KeyValues.h>
#include "c_ball.h"

#ifdef CSTRIKE_DLL
	#include "c_cs_player.h"
#endif

ConVar spec_autodirector( "spec_autodirector", "1", FCVAR_CLIENTDLL | FCVAR_CLIENTCMD_CAN_EXECUTE, "Auto-director chooses best view modes while spectating" );

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CHASE_CAM_DISTANCE		96.0f
#define WALL_OFFSET				6.0f

static Vector WALL_MIN(-WALL_OFFSET,-WALL_OFFSET,-WALL_OFFSET);
static Vector WALL_MAX(WALL_OFFSET,WALL_OFFSET,WALL_OFFSET);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// converts all '\r' characters to '\n', so that the engine can deal with the properly
// returns a pointer to str
static wchar_t* ConvertCRtoNL( wchar_t *str )
{
	for ( wchar_t *ch = str; *ch != 0; ch++ )
		if ( *ch == L'\r' )
			*ch = L'\n';
	return str;
}

static C_HLTVCamera s_HLTVCamera;

C_HLTVCamera *HLTVCamera() { return &s_HLTVCamera; }

C_HLTVCamera::C_HLTVCamera()
{
	Reset();
}

C_HLTVCamera::~C_HLTVCamera()
{

}

void C_HLTVCamera::Init()
{
	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "hltv_message" );
	
	Reset();
}

void C_HLTVCamera::Reset()
{
	m_nCameraMode = OBS_MODE_TVCAM;
	m_iTraget1 = m_iTraget2 = 0;
	m_flFOV = 90;
	m_vCamOrigin.Init();
	m_aCamAngle.Init();

	m_LastCmd.Reset();
	m_vecVelocity.Init();
}

int C_HLTVCamera::GetMode()
{
	return m_nCameraMode;	
}

C_BaseEntity* C_HLTVCamera::GetPrimaryTarget()
{
	if ( m_iTraget1 <= 0 )
		return NULL;

	C_BaseEntity* target = ClientEntityList().GetEnt( m_iTraget1 );

	return target;
}

void C_HLTVCamera::SetMode(int iMode)
{
	if ( m_nCameraMode == iMode )
		return;

    Assert( iMode > OBS_MODE_NONE && iMode <= LAST_PLAYER_OBSERVERMODE );

	m_nCameraMode = iMode;
}

void C_HLTVCamera::SetPrimaryTarget( int nEntity ) 
{
 	if ( m_iTraget1 == nEntity )
		return;

	m_iTraget1 = nEntity;

	if ( GetMode() == OBS_MODE_ROAMING )
	{
		Vector vOrigin;
		QAngle aAngles;
		float flFov;
	}
	else if ( GetMode() == OBS_MODE_CHASE )
	{
		C_BaseEntity* target = ClientEntityList().GetEnt( m_iTraget1 );
		if ( target )
		{
			QAngle eyeAngle = target->EyeAngles();
			prediction->SetViewAngles( eyeAngle );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "spec_target_updated" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}

void C_HLTVCamera::SpecNextPlayer( bool bInverse )
{
	int start = 0;

	if ( m_iTraget1 > 0 && m_iTraget1 <= gpGlobals->maxClients )
		start = m_iTraget1;
	else if (GetBall() && m_iTraget1 == GetBall()->entindex())
		start = 0;

	int index = start;

	while ( true )
	{	
		// got next/prev player
		if ( bInverse )
			index--;
		else
			index++;

		// check bounds
		if ( index < 0 )
		{
			index = gpGlobals->maxClients;
		}
		else if ( index > gpGlobals->maxClients )
		{
			index = 0;
		}

		if ( index == start )
			break; // couldn't find a new valid player

		if (index > 0 && index <= gpGlobals->maxClients)
		{
			C_BasePlayer *pPlayer =	UTIL_PlayerByIndex( index );

			if ( !pPlayer )
				continue;

			// only follow living players 
			if ( pPlayer->IsObserver() )
				continue;
		}

		break; // found a new player
	}

	if (index == 0 && GetBall())
		index = GetBall()->entindex();

	if (index != 0)
		SetPrimaryTarget( index );
}

void C_HLTVCamera::SpecNamedPlayer( const char *szPlayerName )
{
	for ( int index = 1; index <= gpGlobals->maxClients; ++index )
	{
		C_BasePlayer *pPlayer =	UTIL_PlayerByIndex( index );

		if ( !pPlayer )
			continue;

		if ( !FStrEq( szPlayerName, pPlayer->GetPlayerName() ) )
			continue;

		SetPrimaryTarget( index );
		return;
	}
}

void C_HLTVCamera::FireGameEvent( IGameEvent * event)
{
	if ( !engine->IsHLTV() )
		return;	// not in HLTV mode

	const char *type = event->GetName();

	if ( Q_strcmp( "game_newmap", type ) == 0 )
	{
		Reset();	// reset all camera settings

		// show spectator UI
		if ( !gViewPortInterface )
			return;

		if ( engine->IsPlayingDemo() )
        {
			// for demo playback show full menu
			gViewPortInterface->ShowPanel( PANEL_SPECMENU, true );

			SetMode( OBS_MODE_ROAMING );
		}
		else
		{
			// during live broadcast only show black bars
			gViewPortInterface->ShowPanel( PANEL_SPECGUI, true );
		}

		return;
	}

	if ( Q_strcmp( "hltv_message", type ) == 0 )
	{
		wchar_t outputBuf[1024];
		const char *pszText = event->GetString( "text", "" );
		
		char *tmpStr = hudtextmessage->LookupString( pszText );
		const wchar_t *pBuf = g_pVGuiLocalize->Find( tmpStr );
		if ( pBuf )
		{
			// Copy pBuf into szBuf[i].
			int nMaxChars = sizeof( outputBuf ) / sizeof( wchar_t );
			wcsncpy( outputBuf, pBuf, nMaxChars );
			outputBuf[nMaxChars-1] = 0;
		}
		else
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( tmpStr, outputBuf, sizeof(outputBuf) );
		}

		internalCenterPrint->Print( ConvertCRtoNL( outputBuf ) );
		return ;
	}
}

// this is a cheap version of FullNoClipMove():
void C_HLTVCamera::CreateMove( CUserCmd *cmd)
{
	if ( cmd )
	{
		m_LastCmd = *cmd;
	}
}

// movement code is a copy of CGameMovement::FullNoClipMove()
void C_HLTVCamera::CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	Vector wishvel;
	Vector forward, right, up;
	Vector wishdir;
	float wishspeed;
	float factor = sv_specspeed.GetFloat();
	float maxspeed = sv_maxspeed.GetFloat() * factor;

	AngleVectors ( m_LastCmd.viewangles, &forward, &right, &up);  // Determine movement angles

	if ( m_LastCmd.buttons & IN_SPEED )
	{
		factor /= 2.0f;
	}

	// Copy movement amounts
	float fmove = m_LastCmd.forwardmove * factor;
	float smove = m_LastCmd.sidemove * factor;

	VectorNormalize (forward);  // Normalize remainder of vectors
	VectorNormalize (right);    // 

	for (int i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] += m_LastCmd.upmove * factor;

	VectorCopy (wishvel, wishdir);   // Determine magnitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to server defined max speed
	//
	if (wishspeed > maxspeed )
	{
		VectorScale (wishvel, maxspeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}

	if ( sv_specaccelerate.GetFloat() > 0.0 )
	{
		// Set move velocity
		Accelerate ( wishdir, wishspeed, sv_specaccelerate.GetFloat() );

		float spd = VectorLength( m_vecVelocity );
		if (spd < 1.0f)
		{
			m_vecVelocity.Init();
		}
		else
		{
			// Bleed off some speed, but if we have less than the bleed
			//  threshold, bleed the threshold amount.
			float control = (spd < maxspeed/4.0) ? maxspeed/4.0 : spd;

			float friction = sv_friction.GetFloat();

			// Add the amount to the drop amount.
			float drop = control * friction * gpGlobals->frametime;

			// scale the velocity
			float newspeed = spd - drop;
			if (newspeed < 0)
				newspeed = 0;

			// Determine proportion of old speed we are using.
			newspeed /= spd;
			VectorScale( m_vecVelocity, newspeed, m_vecVelocity );
		}
	}
	else
	{
		VectorCopy( wishvel, m_vecVelocity );
	}

	// Just move ( don't clip or anything )
	VectorMA( m_vCamOrigin, gpGlobals->frametime, m_vecVelocity, m_vCamOrigin );

	// get camera angle directly from engine
	engine->GetViewAngles( m_aCamAngle );

	// Zero out velocity if in noaccel mode
	if ( sv_specaccelerate.GetFloat() < 0.0f )
	{
		m_vecVelocity.Init();
	}

	eyeOrigin = m_vCamOrigin;
	eyeAngles = m_aCamAngle;
	fov = m_flFOV;
}

void C_HLTVCamera::Accelerate( Vector& wishdir, float wishspeed, float accel )
{
	float addspeed, accelspeed, currentspeed;

	// See if we are changing direction a bit
	currentspeed =m_vecVelocity.Dot(wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of acceleration.
	accelspeed = accel * gpGlobals->frametime * wishspeed;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust velocity.
	for (int i=0 ; i<3 ; i++)
	{
		m_vecVelocity[i] += accelspeed * wishdir[i];	
	}
}