//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "hud.h"
#include "kbutton.h"
#include "input.h"

#include <vgui/IInput.h>
#include "vgui_controls/controls.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-------------------------------------------------- Constants

//#define CAM_MIN_DIST 30.0
#define CAM_ANGLE_MOVE .5
#define MAX_ANGLE_DIFF 10.0
#define PITCH_MAX 90.0
#define PITCH_MIN 0
#define YAW_MAX  135.0
#define YAW_MIN	 -135.0
#define	DIST	 2
//#define CAM_HULL_OFFSET		9.0    // the size of the bounding hull used for collision checking

//http://developer.valvesoftware.com/wiki/Third_Person_Camera
#define CAM_MIN_DIST		16.0 // Don't let the camera get any closer than ...
#define CAM_MAX_DIST		500.0 // ... or any farther away than ...
#define CAM_SWITCH_DIST		500.0 // the default camera distance when switching 1st to 3rd person
#define CAM_HULL_OFFSET		6.0  // the size of the bounding hull used for collision checking
#define START_TRANS_DIST	40.0 // how close to player when it starts making model translucent
#define TRANS_DELTA	        1.9921875 // Set to 255 / START_TRANS_DIST

static Vector CAM_HULL_MIN(-CAM_HULL_OFFSET,-CAM_HULL_OFFSET,-CAM_HULL_OFFSET);
static Vector CAM_HULL_MAX( CAM_HULL_OFFSET, CAM_HULL_OFFSET, CAM_HULL_OFFSET);

//-------------------------------------------------- Global Variables

static ConVar cam_command( "cam_command", "0", FCVAR_ARCHIVE );	 // tells camera to go to thirdperson
static ConVar cam_snapto( "cam_snapto", "0", FCVAR_ARCHIVE );	 // snap to thirdperson view
static ConVar cam_ideallag( "cam_ideallag", "4.0", FCVAR_ARCHIVE, "Amount of lag used when matching offset to ideal angles in thirdperson view" );
static ConVar cam_idealdelta( "cam_idealdelta", "4.0", FCVAR_ARCHIVE, "Controls the speed when matching offset to ideal angles in thirdperson view" );
ConVar cam_idealyaw( "cam_idealyaw", "0", FCVAR_ARCHIVE );	 // thirdperson yaw
ConVar cam_idealpitch( "cam_idealpitch", "0", FCVAR_ARCHIVE );	 // thirperson pitch
static ConVar cam_dist( "cam_dist", "150", FCVAR_ARCHIVE, "", true, 0, true, 175 );	 // thirdperson distance
static ConVar cam_collision( "cam_collision", "1", FCVAR_ARCHIVE, "When in thirdperson and cam_collision is set to 1, an attempt is made to keep the camera from passing though walls." );
static ConVar cam_showangles( "cam_showangles", "0", FCVAR_CHEAT, "When in thirdperson, print viewangles/idealangles/cameraoffsets to the console." );
static ConVar c_maxpitch( "c_maxpitch", "90", FCVAR_ARCHIVE );
static ConVar c_minpitch( "c_minpitch", "0", FCVAR_ARCHIVE );
static ConVar c_maxyaw( "c_maxyaw",   "135", FCVAR_ARCHIVE );
static ConVar c_minyaw( "c_minyaw",   "-135", FCVAR_ARCHIVE );
static ConVar c_maxdistance( "c_maxdistance",   "500", FCVAR_ARCHIVE );
static ConVar c_mindistance( "c_mindistance",   "30", FCVAR_ARCHIVE );
static ConVar c_orthowidth( "c_orthowidth",   "100", FCVAR_ARCHIVE );
static ConVar c_orthoheight( "c_orthoheight",   "100", FCVAR_ARCHIVE );

static ConVar cl_player_opacity( "cl_player_opacity", "1.0", FCVAR_ARCHIVE);

static kbutton_t cam_pitchup, cam_pitchdown, cam_yawleft, cam_yawright;
static kbutton_t cam_in, cam_out; // -- "cam_move" is unused

extern const ConVar *sv_cheats;

extern ConVar cam_height;


/*
==============================
CAM_Think

==============================
*/
void CInput::CAM_Think( void )
{
	VPROF("CAM_Think");

	Vector idealAngles;
	Vector camOffset;
	QAngle viewangles;
	
	idealAngles[ PITCH ] = cam_idealpitch.GetFloat();
	idealAngles[ YAW ]   = cam_idealyaw.GetFloat();
	idealAngles[ DIST ]  = cam_dist.GetFloat();

	VectorCopy( m_vecCameraOffset, camOffset );

	C_BasePlayer* localPlayer = C_BasePlayer::GetLocalPlayer();

	if( !localPlayer ) 
	{
		// this code can be hit from the main menu, where it will crash
		camOffset[ DIST ] = idealAngles[ DIST ]; // if there's no localplayer to calc from
	}
	else
	{
		if (localPlayer->IsObserver() && localPlayer->GetObserverMode() == OBS_MODE_IN_EYE)
			engine->GetViewAngles( viewangles );
		else
			viewangles = m_aCameraViewAngles;

		camOffset[ YAW ] = cam_idealyaw.GetFloat() + viewangles[ YAW ];
		camOffset[ PITCH ] = cam_idealpitch.GetFloat() + viewangles[ PITCH ];
		camOffset[ DIST ] = cam_dist.GetFloat();

		// move the camera closer to the player if it hit something
		if ( cam_collision.GetInt() )
		{
			trace_t tr;
			float adjDist = idealAngles[ DIST ];

			Vector origin = localPlayer->GetLocalOrigin(); // find our player's origin
			//origin += localPlayer->GetViewOffset(); // and from there, his eye position
			origin.z += cam_height.GetInt();

			Vector camForward;
			AngleVectors( QAngle(camOffset.x, camOffset.y, camOffset.z), &camForward, NULL, NULL ); // get the forward vector

			if (camOffset[PITCH] >= 0)
				adjDist = idealAngles[DIST];
			else
			{
				float coeff = clamp(cos(DEG2RAD(camOffset[PITCH] + 90)), 0.001f, 1.0f);
				adjDist = min((VEC_VIEW.z + cam_height.GetInt() - 5) / coeff, idealAngles[DIST]);
			}

			camOffset[ DIST ] = adjDist;
		}

		if (cl_player_opacity.GetFloat() < 1)
		{
			if (localPlayer->GetRenderMode() != kRenderTransColor)
				localPlayer->SetRenderMode( kRenderTransColor );

			localPlayer->SetRenderColorA(clamp(cl_player_opacity.GetFloat() * 255, 0, 255));
		}
		else
		{
			//localPlayer->SetRenderColorA(255);
			if (localPlayer->GetRenderMode() != kRenderNormal)
				localPlayer->SetRenderMode( kRenderNormal );
		}
	}
	if ( cam_showangles.GetInt() )
	{
		engine->Con_NPrintf( 4, "Pitch: %6.1f   Yaw: %6.1f %38s", viewangles[ PITCH ], viewangles[ YAW ], "view angles" );
		engine->Con_NPrintf( 6, "Pitch: %6.1f   Yaw: %6.1f   Dist: %6.1f %19s", cam_idealpitch.GetFloat(), cam_idealyaw.GetFloat(), cam_dist.GetFloat(), "ideal angles" );
		engine->Con_NPrintf( 8, "Pitch: %6.1f   Yaw: %6.1f   Dist: %6.1f %16s", m_vecCameraOffset[ PITCH ], m_vecCameraOffset[ YAW ], m_vecCameraOffset[ DIST ], "camera offset" );
	}

	m_vecCameraOffset[ PITCH ] = camOffset[ PITCH ];
	m_vecCameraOffset[ YAW ]   = camOffset[ YAW ];
	m_vecCameraOffset[ DIST ]  = camOffset[ DIST ];
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void ClampRange180( float &value )
{
	if ( value >= 180.0f )
	{
		value -= 360.0f;
	}
	else if ( value <= -180.0f )
	{
		value += 360.0f;
	}
}

void CAM_PitchUpDown( const CCommand &args ) { KeyDown( &cam_pitchup, args[1] ); }
void CAM_PitchUpUp( const CCommand &args ) { KeyUp( &cam_pitchup, args[1] ); }
void CAM_PitchDownDown( const CCommand &args ) { KeyDown( &cam_pitchdown, args[1] ); }
void CAM_PitchDownUp( const CCommand &args ) { KeyUp( &cam_pitchdown, args[1] ); }
void CAM_YawLeftDown( const CCommand &args ) { KeyDown( &cam_yawleft, args[1] ); }
void CAM_YawLeftUp( const CCommand &args ) { KeyUp( &cam_yawleft, args[1] ); }
void CAM_YawRightDown( const CCommand &args ) { KeyDown( &cam_yawright, args[1] ); }
void CAM_YawRightUp( const CCommand &args ) { KeyUp( &cam_yawright, args[1] ); }
void CAM_InDown( const CCommand &args ) { KeyDown( &cam_in, args[1] ); }
void CAM_InUp( const CCommand &args ) { KeyUp( &cam_in, args[1] ); }
void CAM_OutDown( const CCommand &args ) { KeyDown( &cam_out, args[1] ); }
void CAM_OutUp( const CCommand &args ) { KeyUp( &cam_out, args[1] ); }

/*
==============================
CAM_GetCameraOffset

==============================
*/
void CInput::CAM_GetCameraOffset( Vector& ofs )
{
	VectorCopy( m_vecCameraOffset, ofs );
}

/*
==============================
Init_Camera

==============================
*/
void CInput::Init_Camera( void )
{
}