//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "c_tracer.h"
#include "view.h"
#include "initializer.h"
#include "particles_simple.h"
#include "env_wind_shared.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"
#include "precipitation_shared.h"
#include "fx_water.h"
#include "c_world.h"
#include "iviewrender.h"
#include "engine/IVDebugOverlay.h"
#include "ClientEffectPrecacheSystem.h"
#include "collisionutils.h"
#include "tier0/vprof.h"
#include "viewrender.h"
#include "sdk_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	cl_winddir			( "cl_winddir", "0", FCVAR_CHEAT | FCVAR_ARCHIVE, "Weather effects wind direction angle" );
ConVar	cl_windspeed		( "cl_windspeed", "0", FCVAR_CHEAT | FCVAR_ARCHIVE, "Weather effects wind speed scalar" );

Vector g_vSplashColor( 0.5, 0.5, 0.5 );
float g_flSplashScale = 0.15;
float g_flSplashLifetime = 0.5f;
float g_flSplashAlpha = 0.3f;
ConVar r_rain_splashpercentage( "r_rain_splashpercentage", "0", FCVAR_CHEAT | FCVAR_ARCHIVE ); // N% chance of a rain particle making a splash.


float GUST_INTERVAL_MIN = 1;
float GUST_INTERVAL_MAX = 2;

float GUST_LIFETIME_MIN = 1;
float GUST_LIFETIME_MAX = 3;

float MIN_SCREENSPACE_RAIN_WIDTH = 1;

#ifndef _XBOX

ConVar r_weather_hack( "r_weather_hack", "0", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_weather_profile( "r_weather_profile", "0", FCVAR_CHEAT, "Enable/disable rain profiling." );

ConVar r_rain_radius( "r_rain_radius", "1500", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_rain_height( "r_rain_height", "500", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_rain_playervelmultiplier( "r_rain_playervelmultiplier", "1", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_rain_sidevel( "r_rain_sidevel", "130", FCVAR_CHEAT | FCVAR_ARCHIVE, "How much sideways velocity rain gets." );
ConVar r_rain_density( "r_rain_density","0.001", FCVAR_CHEAT | FCVAR_ARCHIVE);
ConVar r_rain_width( "r_rain_width", "0.5", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_rain_length( "r_rain_length", "0.1f", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_rain_speed( "r_rain_speed", "600.0f", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_rain_alpha( "r_rain_alpha", "0.4", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_rain_alphapow( "r_rain_alphapow", "0.8", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_rain_initialramp( "r_rain_initialramp", "0.6", FCVAR_CHEAT | FCVAR_ARCHIVE );

ConVar r_snow_radius( "r_snow_radius", "1500", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_snow_height( "r_snow_height", "500", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_snow_playervelmultiplier( "r_snow_playervelmultiplier", "1", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_snow_sidevel( "r_snow_sidevel", "130", FCVAR_CHEAT | FCVAR_ARCHIVE, "How much sideways velocity snow gets." );
ConVar r_snow_density( "r_snow_density","0.001", FCVAR_CHEAT | FCVAR_ARCHIVE);
ConVar r_snow_width( "r_snow_width", "0.5", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_snow_length( "r_snow_length", "0.1f", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_snow_speed( "r_snow_speed", "600.0f", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_snow_alpha( "r_snow_alpha", "0.4", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_snow_alphapow( "r_snow_alphapow", "0.8", FCVAR_CHEAT | FCVAR_ARCHIVE );
ConVar r_snow_initialramp( "r_snow_initialramp", "1.0", FCVAR_CHEAT | FCVAR_ARCHIVE );

//Precahce the effects
CLIENTEFFECT_REGISTER_BEGIN( PrecachePrecipitation )
CLIENTEFFECT_MATERIAL( "particle/rain" )
CLIENTEFFECT_MATERIAL( "particle/snow" )
CLIENTEFFECT_REGISTER_END()

//-----------------------------------------------------------------------------
// Precipitation particle type
//-----------------------------------------------------------------------------

class CPrecipitationParticle
{
public:
	Vector	m_Pos;
	Vector	m_Velocity;
	float	m_SpawnTime;				// Note: Tweak with this to change lifetime
	float	m_Mass;
	float	m_Ramp;
	
	float	m_flCurLifetime;
	float	m_flMaxLifetime;
};
						  

class CClient_Precipitation;
static CUtlVector<CClient_Precipitation*> g_Precipitations;

//-----------------------------------------------------------------------------
// Precipitation base entity
//-----------------------------------------------------------------------------

class CClient_Precipitation : public C_BaseEntity
{
class CPrecipitationEffect;
friend class CClient_Precipitation::CPrecipitationEffect;

public:
	DECLARE_CLASS( CClient_Precipitation, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	
	CClient_Precipitation();
	virtual ~CClient_Precipitation();

	// Inherited from C_BaseEntity
	virtual void Precache( );

	void Render();

	void TypeChanged();

private:

	// Creates a single particle
	CPrecipitationParticle* CreateParticle();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ClientThink();

	void Simulate( float dt );

	// Renders the particle
	void RenderParticle( CPrecipitationParticle* pParticle, CMeshBuilder &mb );

	void CreateWaterSplashes();

	// Emits the actual particles
	void EmitParticles( float fTimeDelta );
	
	// Computes where we're gonna emit
	bool ComputeEmissionArea( Vector& origin, Vector2D& size );

	// Gets the remaining lifetime of the particle
	float GetRemainingLifetime( CPrecipitationParticle* pParticle ) const;

	// Computes the wind vector
	static void ComputeWindVector( );

	// simulation methods
	bool SimulateParticle( CPrecipitationParticle* pParticle, float dt );

	void CreateAshParticle( void );
	void CreateRainOrSnowParticle( Vector vSpawnPosition, Vector vVelocity );

	// Information helpful in creating and rendering particles
	IMaterial		*m_MatHandle;	// material used 
	PrecipitationType_t	m_nPrecipType;			// Precip type

	float			m_flColor[4];		// precip color
	float			m_flLifetime;		// Precip lifetime
	float			m_flInitialRamp;	// Initial ramp value
	float			m_flSpeed;		// Precip speed
	float			m_flWidth;		// Tracer width
	float			m_flRemainder;	// particles we should render next time
	float			m_flHalfScreenWidth;	// Precalculated each frame.
	float			m_flDensity;
	float			m_flHeight;
	float			m_flRadius;
	float			m_flLength;
	float			m_flAlpha;

	// Some state used in rendering and simulation
	// Used to modify the rain density and wind from the console

	static Vector s_WindVector;			// Stores the wind speed vector
	
	CUtlLinkedList<CPrecipitationParticle> m_Particles;

private:
	CClient_Precipitation( const CClient_Precipitation & ); // not defined, not accessible
};

// Just receive the normal data table stuff
IMPLEMENT_CLIENTCLASS_DT(CClient_Precipitation, DT_Precipitation, CPrecipitation)
	RecvPropInt( RECVINFO( m_nPrecipType ) )
END_RECV_TABLE()

void DrawPrecipitation()
{
	for ( int i=0; i < g_Precipitations.Count(); i++ )
	{
		g_Precipitations[i]->Render();
	}
}


//-----------------------------------------------------------------------------
// determines if a weather particle has hit something other than air
//-----------------------------------------------------------------------------
static bool IsInAir( const Vector& position )
{
	int contents = enginetrace->GetPointContents( position ); 	
	return (contents & CONTENTS_SOLID) == 0;
}


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

Vector CClient_Precipitation::s_WindVector;		// Stores the wind speed vector

void CClient_Precipitation::OnDataChanged( DataUpdateType_t updateType )
{
	m_MatHandle = INVALID_MATERIAL_HANDLE;

	Precache();

	// Simulate every frame.
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	BaseClass::OnDataChanged( updateType );
}


void CClient_Precipitation::ClientThink()
{
	Simulate( gpGlobals->frametime );
}


//-----------------------------------------------------------------------------
//
// Utility methods for the various simulation functions
//
//-----------------------------------------------------------------------------
inline bool CClient_Precipitation::SimulateParticle( CPrecipitationParticle* pParticle, float dt )
{
	if (GetRemainingLifetime( pParticle ) < 0.0f)
		return false;

	Vector vOldPos = pParticle->m_Pos;

	// Update position
	VectorMA( pParticle->m_Pos, dt, pParticle->m_Velocity, 
				pParticle->m_Pos );

	// wind blows rain around
	for ( int i = 0 ; i < 2 ; i++ )
	{
		if ( pParticle->m_Velocity[i] < s_WindVector[i] )
		{
			pParticle->m_Velocity[i] += ( 5 / pParticle->m_Mass );

			// clamp
			if ( pParticle->m_Velocity[i] > s_WindVector[i] )
				pParticle->m_Velocity[i] = s_WindVector[i];
		}
		else if (pParticle->m_Velocity[i] > s_WindVector[i] )
		{
			pParticle->m_Velocity[i] -= ( 5 / pParticle->m_Mass );

			// clamp.
			if ( pParticle->m_Velocity[i] < s_WindVector[i] )
				pParticle->m_Velocity[i] = s_WindVector[i];
		}
	}

	if (pParticle->m_Pos.z <= SDKGameRules()->m_vKickOff.GetZ())
	{
		if (m_nPrecipType == PRECIPITATION_TYPE_RAIN && r_rain_splashpercentage.GetFloat() != 0 && RandomFloat( 0, 100 ) <= r_rain_splashpercentage.GetFloat())
			DispatchParticleEffect("slime_splash_01", Vector(pParticle->m_Pos.x, pParticle->m_Pos.y, SDKGameRules()->m_vKickOff.GetZ()), vec3_angle, NULL);

		// Tell the framework it's time to remove the particle from the list
		return false;
	}

	// We still want this particle
	return true;
}

void CClient_Precipitation::Simulate( float dt )
{
	if (m_nPrecipType == PRECIPITATION_TYPE_NONE)
		return;

	if (m_nPrecipType == PRECIPITATION_TYPE_RAIN)
	{
		m_flDensity = r_rain_density.GetFloat() / 1000;
		m_flSpeed = r_rain_speed.GetFloat();
		m_flHeight = r_rain_height.GetFloat();
		m_flRadius = r_rain_radius.GetFloat();
		m_flInitialRamp = r_rain_initialramp.GetFloat();
		m_flLength = r_rain_length.GetFloat();
		m_flAlpha = r_rain_alpha.GetFloat();
		m_flWidth = r_rain_width.GetFloat();
	}
	else
	{
		m_flDensity = r_snow_density.GetFloat() / 1000;
		m_flSpeed = r_snow_speed.GetFloat();
		m_flHeight = r_snow_height.GetFloat();
		m_flRadius = r_snow_radius.GetFloat();
		m_flInitialRamp = r_snow_initialramp.GetFloat();
		m_flLength = r_snow_length.GetFloat();
		m_flAlpha = r_snow_alpha.GetFloat();
		m_flWidth = r_snow_width.GetFloat();
	}

	// NOTE: When client-side prechaching works, we need to remove this
	Precache();

	m_flHalfScreenWidth = (float)ScreenWidth() / 2;

	// Our sim methods needs dt	and wind vector
	if ( dt )
	{
		ComputeWindVector( );
	}

	CFastTimer timer;
	timer.Start();

	// Emit new particles
	EmitParticles( dt );

	// Simulate all the particles.
	int iNext;
	for ( int i=m_Particles.Head(); i != m_Particles.InvalidIndex(); i=iNext )
	{
		iNext = m_Particles.Next( i );
		if ( !SimulateParticle( &m_Particles[i], dt ) )
			m_Particles.Remove( i );
	}

	if ( r_weather_profile.GetBool() )
	{
		timer.End();
		engine->Con_NPrintf( 15, "Rain simulation: %du (%d tracers)", timer.GetDuration().GetMicroseconds(), m_Particles.Count() );
	}
}


//-----------------------------------------------------------------------------
// tracer rendering
//-----------------------------------------------------------------------------

inline void CClient_Precipitation::RenderParticle( CPrecipitationParticle* pParticle, CMeshBuilder &mb )
{
	float scale;
	Vector start, delta;

	// make streaks 0.1 seconds long, but prevent from going past end
	float lifetimeRemaining = GetRemainingLifetime( pParticle );
	if (lifetimeRemaining >= m_flLength)
		scale = m_flLength * pParticle->m_Ramp;
	else
		scale = lifetimeRemaining * pParticle->m_Ramp;
	
	// NOTE: We need to do everything in screen space
	Vector3DMultiplyPosition( CurrentWorldToViewMatrix(), pParticle->m_Pos, start );
	if ( start.z > -1 )
		return;

	Vector3DMultiply( CurrentWorldToViewMatrix(), pParticle->m_Velocity, delta );

	// give a spiraling pattern to snow particles
	if ( m_nPrecipType == PRECIPITATION_TYPE_SNOW )
	{
		Vector spiral, camSpiral;
		float s, c;

		if ( pParticle->m_Mass > 1.0f )
		{
			SinCos( gpGlobals->curtime * M_PI * (1+pParticle->m_Mass * 0.1f) + 
					pParticle->m_Mass * 5.0f, &s , &c );

			// only spiral particles with a mass > 1, so some fall straight down
			spiral[0] = 28 * c;
			spiral[1] = 28 * s;
			spiral[2] = 0.0f;

			Vector3DMultiply( CurrentWorldToViewMatrix(), spiral, camSpiral );

			// X and Y are measured in world space; need to convert to camera space
			VectorAdd( start, camSpiral, start );
			VectorAdd( delta, camSpiral, delta );

			if ( start.z > -1 )
				return;
		}

		// shrink the trails on spiraling flakes.
		pParticle->m_Ramp = 0.3f;
	}

	delta[0] *= scale;
	delta[1] *= scale;
	delta[2] *= scale;

	float flWidth = m_flWidth;
	float flAlpha = m_flAlpha;

	// See c_tracer.* for this method
	float flScreenSpaceWidth = flWidth * m_flHalfScreenWidth / -start.z;
	if ( flScreenSpaceWidth < MIN_SCREENSPACE_RAIN_WIDTH )
	{
		// Make the rain tracer at least the min size, but fade its alpha the smaller it gets.
		flAlpha *= flScreenSpaceWidth / MIN_SCREENSPACE_RAIN_WIDTH;
		flWidth = MIN_SCREENSPACE_RAIN_WIDTH * -start.z / m_flHalfScreenWidth;
	}
	flAlpha = pow( flAlpha, r_rain_alphapow.GetFloat() );

	float flColor[4] = { 1, 1, 1, flAlpha };
	Tracer_Draw( &mb, start, delta, flWidth, flColor, 1 );
}

void CClient_Precipitation::Render()
{
	if (m_nPrecipType == PRECIPITATION_TYPE_NONE)
		return;

	//if ( !r_DrawRain.GetInt() )
	//	return;

	// Don't render in monitors or in reflections or refractions.
	if ( CurrentViewID() == VIEW_MONITOR )
		return;

	if ( view->GetDrawFlags() & (DF_RENDER_REFLECTION | DF_RENDER_REFRACTION) )
		return;

	CFastTimer timer;
	timer.Start();

	CMatRenderContextPtr pRenderContext( materials );
	
	// We want to do our calculations in view space.
	VMatrix	tempView;
	pRenderContext->GetMatrix( MATERIAL_VIEW, &tempView );
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->LoadIdentity();

	// Force the user clip planes to use the old view matrix
	pRenderContext->EnableUserClipTransformOverride( true );
	pRenderContext->UserClipTransform( tempView );

	// Draw all the rain tracers.
	pRenderContext->Bind( m_MatHandle );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	if ( pMesh )
	{
		CMeshBuilder mb;
		mb.Begin( pMesh, MATERIAL_QUADS, m_Particles.Count() );

		for ( int i=m_Particles.Head(); i != m_Particles.InvalidIndex(); i=m_Particles.Next( i ) )
		{
			CPrecipitationParticle *p = &m_Particles[i];
			RenderParticle( p, mb );
		}

		mb.End( false, true );
	}

	pRenderContext->EnableUserClipTransformOverride( false );
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->LoadMatrix( tempView );

	if ( r_weather_profile.GetBool() )
	{
		timer.End();
		engine->Con_NPrintf( 16, "Rain render    : %du", timer.GetDuration().GetMicroseconds() );
	}
}


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------

CClient_Precipitation::CClient_Precipitation()
{
	m_nPrecipType = PRECIPITATION_TYPE_NONE;
	m_MatHandle = INVALID_MATERIAL_HANDLE;
	m_flHalfScreenWidth = 1;
	m_flRemainder = 0;
	
	g_Precipitations.AddToTail( this );
}

CClient_Precipitation::~CClient_Precipitation()
{
	g_Precipitations.FindAndRemove( this );
}

//-----------------------------------------------------------------------------
// Precache data
//-----------------------------------------------------------------------------

void CClient_Precipitation::Precache( )
{
	if ( !m_MatHandle )
	{
		// Compute precipitation emission speed
		switch( m_nPrecipType )
		{
		case PRECIPITATION_TYPE_SNOW:
			m_MatHandle = materials->FindMaterial( "particle/snow", TEXTURE_GROUP_CLIENT_EFFECTS );
			break;

		case PRECIPITATION_TYPE_RAIN:
			m_MatHandle = materials->FindMaterial( "particle/rain", TEXTURE_GROUP_CLIENT_EFFECTS );
			break;
		}

		// Store off the color
		m_flColor[0] = 1.0f;
		m_flColor[1] = 1.0f;
		m_flColor[2] = 1.0f;
	}
}

//-----------------------------------------------------------------------------
// Gets the remaining lifetime of the particle
//-----------------------------------------------------------------------------

inline float CClient_Precipitation::GetRemainingLifetime( CPrecipitationParticle* pParticle ) const
{
	float timeSinceSpawn = gpGlobals->curtime - pParticle->m_SpawnTime;
	return m_flLifetime - timeSinceSpawn;
}

//-----------------------------------------------------------------------------
// Creates a particle
//-----------------------------------------------------------------------------

inline CPrecipitationParticle* CClient_Precipitation::CreateParticle()
{
	int i = m_Particles.AddToTail();
	CPrecipitationParticle* pParticle = &m_Particles[i];

	pParticle->m_SpawnTime = gpGlobals->curtime;
	pParticle->m_Ramp = m_flInitialRamp;

	return pParticle;
}


//-----------------------------------------------------------------------------
// Compute the emission area
//-----------------------------------------------------------------------------

bool CClient_Precipitation::ComputeEmissionArea( Vector& origin, Vector2D& size )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	m_flLifetime = m_flHeight / m_flSpeed;

	origin.x = pPlayer->GetLocalOrigin().x + r_rain_playervelmultiplier.GetFloat() * pPlayer->GetLocalVelocity().x - m_flRadius - m_flLifetime * s_WindVector.x;
	origin.y = pPlayer->GetLocalOrigin().y + r_rain_playervelmultiplier.GetFloat() * pPlayer->GetLocalVelocity().y - m_flRadius - m_flLifetime * s_WindVector.y;
	origin.z = pPlayer->GetLocalOrigin().z + m_flHeight;

	size.x = 2 * m_flRadius;
	size.y = 2 * m_flRadius;

	return true;
}

void CClient_Precipitation::CreateRainOrSnowParticle( Vector vSpawnPosition, Vector vVelocity )
{
	// Create the particle
	CPrecipitationParticle* p = CreateParticle();
	if (!p) 
		return;

	VectorCopy( vVelocity, p->m_Velocity );
	p->m_Pos = vSpawnPosition;

	p->m_Velocity[ 0 ] += random->RandomFloat(-r_rain_sidevel.GetInt(), r_rain_sidevel.GetInt());
	p->m_Velocity[ 1 ] += random->RandomFloat(-r_rain_sidevel.GetInt(), r_rain_sidevel.GetInt());

	p->m_Mass = random->RandomFloat( 0.5, 1.5 );
}

//-----------------------------------------------------------------------------
// emit the precipitation particles
//-----------------------------------------------------------------------------

void CClient_Precipitation::EmitParticles( float fTimeDelta )
{
	Vector2D size;
	Vector vel, org;
	
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;
	Vector vPlayerCenter = pPlayer->WorldSpaceCenter();

	// Compute where to emit
	if (!ComputeEmissionArea( org, size ))
		return;

	// clamp this to prevent creating a bunch of rain or snow at one time.
	if( fTimeDelta > 0.075f )
		fTimeDelta = 0.075f;

	/*if (density > 0.01f) 
		density = 0.01f;*/

	// Compute number of particles to emit based on precip density and emission area and dt
	float fParticles = size[0] * size[1] * m_flDensity * fTimeDelta + m_flRemainder; 
	int cParticles = (int)fParticles;
	m_flRemainder = fParticles - cParticles;

	// calculate the max amount of time it will take this flake to fall.
	// This works if we assume the wind doesn't have a z component
	VectorCopy( s_WindVector, vel );
	vel[2] -= m_flSpeed;

	// Emit all the particles
	for ( int i = 0 ; i < cParticles ; i++ )
	{							
		Vector vParticlePos = org;
		vParticlePos[ 0 ] += size[ 0 ] * g_IOSRand.RandomFloat(0, 1);
		vParticlePos[ 1 ] += size[ 1 ] * g_IOSRand.RandomFloat(0, 1);

		// Figure out where the particle should lie in Z by tracing a line from the player's height up to the 
		// desired height and making sure it doesn't hit a wall.
		Vector vPlayerHeight = vParticlePos;
		vPlayerHeight.z = vPlayerCenter.z;

		trace_t trace;
		UTIL_TraceLine( vPlayerHeight, vParticlePos, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &trace );
		if ( trace.fraction < 1 )
		{
			// If we hit a brush, then don't spawn the particle.
			if ( trace.surface.flags & SURF_SKY )
			{
				vParticlePos = trace.endpos;
			}
			else
			{
				continue;
			}
		}

		CreateRainOrSnowParticle( vParticlePos, vel );
	}
}


//-----------------------------------------------------------------------------
// Computes the wind vector
//-----------------------------------------------------------------------------

void CClient_Precipitation::ComputeWindVector( )
{
	// Compute the wind direction
	QAngle windangle( 0, cl_winddir.GetFloat(), 0 );	// used to turn wind yaw direction into a vector

	// Randomize the wind angle and speed slightly to get us a little variation
	windangle[1] = windangle[1] + random->RandomFloat( -10, 10 );
	float windspeed = cl_windspeed.GetFloat() * (1.0 + random->RandomFloat( -0.2, 0.2 ));

	AngleVectors( windangle, &s_WindVector );
	VectorScale( s_WindVector, windspeed, s_WindVector );
}


CHandle<CClient_Precipitation> g_pPrecipHackEnt;

class CPrecipHack : public CAutoGameSystemPerFrame
{
public:
	CPrecipHack( char const *name ) : CAutoGameSystemPerFrame( name )
	{
		m_bLevelInitted = false;
	}

	virtual void LevelInitPostEntity()
	{
		if ( r_weather_hack.GetInt() )
		{
			CClient_Precipitation *pPrecipHackEnt = new CClient_Precipitation;
			pPrecipHackEnt->InitializeAsClientEntity( NULL, RENDER_GROUP_TRANSLUCENT_ENTITY );
			g_pPrecipHackEnt = pPrecipHackEnt;
		}
		m_bLevelInitted = true;
	}
	
	virtual void LevelShutdownPreEntity()
	{
		if ( r_weather_hack.GetInt() && g_pPrecipHackEnt )
		{
			g_pPrecipHackEnt->Release();
		}
		m_bLevelInitted = false;
	}

	virtual void Update( float frametime )
	{
		// Handle changes to the cvar at runtime.
		if ( m_bLevelInitted )
		{
			if ( r_weather_hack.GetInt() && !g_pPrecipHackEnt )
				LevelInitPostEntity();
			else if ( !r_weather_hack.GetInt() && g_pPrecipHackEnt )
				LevelShutdownPreEntity();
		}
	}

	bool m_bLevelInitted;
};
CPrecipHack g_PrecipHack( "CPrecipHack" );

#else

void DrawPrecipitation()
{
}

#endif	// _XBOX

//-----------------------------------------------------------------------------
// EnvWind - global wind info
//-----------------------------------------------------------------------------
class C_EnvWind : public C_BaseEntity
{
public:
	C_EnvWind();

	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_EnvWind, C_BaseEntity );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw( void ) { return false; }

	virtual void	ClientThink( );

private:
	C_EnvWind( const C_EnvWind & );

	CEnvWindShared m_EnvWindShared;
};

// Receive datatables
BEGIN_RECV_TABLE_NOBASE(CEnvWindShared, DT_EnvWindShared)
	RecvPropInt		(RECVINFO(m_iMinWind)),
	RecvPropInt		(RECVINFO(m_iMaxWind)),
	RecvPropInt		(RECVINFO(m_iMinGust)),
	RecvPropInt		(RECVINFO(m_iMaxGust)),
	RecvPropFloat	(RECVINFO(m_flMinGustDelay)),
	RecvPropFloat	(RECVINFO(m_flMaxGustDelay)),
	RecvPropInt		(RECVINFO(m_iGustDirChange)),
	RecvPropInt		(RECVINFO(m_iWindSeed)),
	RecvPropInt		(RECVINFO(m_iInitialWindDir)),
	RecvPropFloat	(RECVINFO(m_flInitialWindSpeed)),
	RecvPropFloat	(RECVINFO(m_flStartTime)),
	RecvPropFloat	(RECVINFO(m_flGustDuration)),
//	RecvPropInt		(RECVINFO(m_iszGustSound)),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_EnvWind, DT_EnvWind, CEnvWind )
	RecvPropDataTable(RECVINFO_DT(m_EnvWindShared), 0, &REFERENCE_RECV_TABLE(DT_EnvWindShared)),
END_RECV_TABLE()


C_EnvWind::C_EnvWind()
{
}

//-----------------------------------------------------------------------------
// Post data update!
//-----------------------------------------------------------------------------
void C_EnvWind::OnDataChanged( DataUpdateType_t updateType )
{
	// Whenever we get an update, reset the entire state.
	// Note that the fields have already been stored by the datatables,
	// but there's still work to be done in the init block
	m_EnvWindShared.Init( entindex(), m_EnvWindShared.m_iWindSeed, 
		m_EnvWindShared.m_flStartTime, m_EnvWindShared.m_iInitialWindDir,
		m_EnvWindShared.m_flInitialWindSpeed );

	SetNextClientThink(0.0f);

	BaseClass::OnDataChanged( updateType );
}

void C_EnvWind::ClientThink( )
{
	// Update the wind speed
	float flNextThink = m_EnvWindShared.WindThink( gpGlobals->curtime );
	SetNextClientThink(flNextThink);
}



//==================================================
// EmberParticle
//==================================================

class CEmberEmitter : public CSimpleEmitter
{
public:
							CEmberEmitter( const char *pDebugName );
	static CSmartPtr<CEmberEmitter>	Create( const char *pDebugName );
	virtual void			UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	virtual Vector			UpdateColor( const SimpleParticle *pParticle );

private:
							CEmberEmitter( const CEmberEmitter & );
};


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
CEmberEmitter::CEmberEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName )
{
}


CSmartPtr<CEmberEmitter> CEmberEmitter::Create( const char *pDebugName )
{
	return new CEmberEmitter( pDebugName );
}


void CEmberEmitter::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
	float	speed = VectorNormalize( pParticle->m_vecVelocity );
	Vector	offset;

	speed -= ( 1.0f * timeDelta );

	offset.Random( -0.025f, 0.025f );
	offset[2] = 0.0f;

	pParticle->m_vecVelocity += offset;
	VectorNormalize( pParticle->m_vecVelocity );

	pParticle->m_vecVelocity *= speed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
Vector CEmberEmitter::UpdateColor( const SimpleParticle *pParticle )
{
	Vector	color;
	float	ramp = 1.0f - ( pParticle->m_flLifetime / pParticle->m_flDieTime );

	color[0] = ( (float) pParticle->m_uchColor[0] * ramp ) / 255.0f;
	color[1] = ( (float) pParticle->m_uchColor[1] * ramp ) / 255.0f;
	color[2] = ( (float) pParticle->m_uchColor[2] * ramp ) / 255.0f;

	return color;
}

//==================================================
// C_Embers
//==================================================

class C_Embers : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_Embers, C_BaseEntity );

					C_Embers();
					~C_Embers();

	void	Start( void );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw( void );
	virtual void	AddEntity( void );

	//Server-side
	int		m_nDensity;
	int		m_nLifetime;
	int		m_nSpeed;
	bool	m_bEmit;

protected:

	void	SpawnEmber( void );

	PMaterialHandle		m_hMaterial;
	TimedEvent			m_tParticleSpawn;
	CSmartPtr<CEmberEmitter> m_pEmitter;

};

//Receive datatable
IMPLEMENT_CLIENTCLASS_DT( C_Embers, DT_Embers, CEmbers )
	RecvPropInt( RECVINFO( m_nDensity ) ),
	RecvPropInt( RECVINFO( m_nLifetime ) ),
	RecvPropInt( RECVINFO( m_nSpeed ) ),
	RecvPropInt( RECVINFO( m_bEmit ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
C_Embers::C_Embers()
{
	m_pEmitter = CEmberEmitter::Create( "C_Embers" );
}

C_Embers::~C_Embers()
{
}

void C_Embers::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_pEmitter->SetSortOrigin( GetAbsOrigin() );

		Start();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_Embers::ShouldDraw()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Embers::Start( void )
{
	//Various setup info
	m_tParticleSpawn.Init( m_nDensity );
	
	m_hMaterial	= m_pEmitter->GetPMaterial( "particle/fire" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Embers::AddEntity( void ) 
{
	if ( m_bEmit == false )
		return;

	float tempDelta = gpGlobals->frametime;

	while( m_tParticleSpawn.NextEvent( tempDelta ) )
	{
		SpawnEmber();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Embers::SpawnEmber( void )
{
	Vector	offset, mins, maxs;
	
	modelinfo->GetModelBounds( GetModel(), mins, maxs );

	//Setup our spawn position
	offset[0] = random->RandomFloat( mins[0], maxs[0] );
	offset[1] = random->RandomFloat( mins[1], maxs[1] );
	offset[2] = random->RandomFloat( mins[2], maxs[2] );

	//Spawn the particle
	SimpleParticle	*sParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof( SimpleParticle ), m_hMaterial, offset );

	if (sParticle == NULL)
		return;

	float	cScale = random->RandomFloat( 0.75f, 1.0f );

	//Set it up
	sParticle->m_flLifetime = 0.0f;
	sParticle->m_flDieTime	= m_nLifetime;

	sParticle->m_uchColor[0]	= m_clrRender->r * cScale;
	sParticle->m_uchColor[1]	= m_clrRender->g * cScale;
	sParticle->m_uchColor[2]	= m_clrRender->b * cScale;
	sParticle->m_uchStartAlpha	= 255;
	sParticle->m_uchEndAlpha	= 0;
	sParticle->m_uchStartSize	= 1;
	sParticle->m_uchEndSize		= 0;
	sParticle->m_flRollDelta	= 0;
	sParticle->m_flRoll			= 0;

	//Set the velocity
	Vector	velocity;

	AngleVectors( GetAbsAngles(), &velocity );

	sParticle->m_vecVelocity = velocity * m_nSpeed;

	sParticle->m_vecVelocity[0]	+= random->RandomFloat( -(m_nSpeed/8), (m_nSpeed/8) );
	sParticle->m_vecVelocity[1]	+= random->RandomFloat( -(m_nSpeed/8), (m_nSpeed/8) );
	sParticle->m_vecVelocity[2]	+= random->RandomFloat( -(m_nSpeed/8), (m_nSpeed/8) );

	UpdateVisibility();
}

//-----------------------------------------------------------------------------
// Quadratic spline beam effect 
//-----------------------------------------------------------------------------
#include "beamdraw.h"

class C_QuadraticBeam : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_QuadraticBeam, C_BaseEntity );

	//virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw( void ) { return true; }
	virtual int		DrawModel( int );

	virtual void	GetRenderBounds( Vector& mins, Vector& maxs )
	{
		ClearBounds( mins, maxs );
		AddPointToBounds( vec3_origin, mins, maxs );
		AddPointToBounds( m_targetPosition, mins, maxs );
		AddPointToBounds( m_controlPosition, mins, maxs );
		mins -= GetRenderOrigin();
		maxs -= GetRenderOrigin();
	}

protected:

	Vector		m_targetPosition;
	Vector		m_controlPosition;
	float		m_scrollRate;
	float		m_flWidth;
};

//Receive datatable
IMPLEMENT_CLIENTCLASS_DT( C_QuadraticBeam, DT_QuadraticBeam, CEnvQuadraticBeam )
	RecvPropVector( RECVINFO(m_targetPosition) ),
	RecvPropVector( RECVINFO(m_controlPosition) ),
	RecvPropFloat( RECVINFO(m_scrollRate) ),
	RecvPropFloat( RECVINFO(m_flWidth) ),
END_RECV_TABLE()

Vector Color32ToVector( const color32 &color )
{
	return Vector( color.r * (1.0/255.0f), color.g * (1.0/255.0f), color.b * (1.0/255.0f) );
}

int	C_QuadraticBeam::DrawModel( int )
{
	Draw_SetSpriteTexture( GetModel(), 0, GetRenderMode() );
	Vector color = Color32ToVector( GetRenderColor() );
	DrawBeamQuadratic( GetRenderOrigin(), m_controlPosition, m_targetPosition, m_flWidth, color, gpGlobals->curtime*m_scrollRate );
	return 1;
}