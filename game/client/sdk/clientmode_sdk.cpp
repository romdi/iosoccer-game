//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include "cbase.h"
#include "hud.h"
#include "clientmode_sdk.h"
#include "cdll_client_int.h"
#include "iinput.h"
#include "vgui/isurface.h"
#include "vgui/ipanel.h"
#include <vgui_controls/AnimationController.h>
#include "ivmodemanager.h"
#include "BuyMenu.h"
#include "filesystem.h"
#include "vgui/ivgui.h"
#include "hud_basechat.h"
#include "view_shared.h"
#include "view.h"
#include "ivrenderview.h"
#include "model_types.h"
#include "iefx.h"
#include "dlight.h"
#include <imapoverview.h>
#include "c_playerresource.h"
#include <keyvalues.h>
#include "text_message.h"
#include "panelmetaclassmgr.h"
#include "weapon_sdkbase.h"
#include "c_sdk_player.h"
#include "c_weapon__stubs.h"		//Tony; add stubs
#include "ios_teamkit_parse.h"
#include "ios_fileupdater.h"
#include "steam/steam_api.h"
#include "ios_update_menu.h"

class CHudChat;

ConVar default_fov( "default_fov", "90", FCVAR_CHEAT );

IClientMode *g_pClientMode = NULL;

float ClientModeSDKNormal::m_flLastMapChange = -1;

//Tony; add stubs for cycler weapon and cubemap.
STUB_WEAPON_CLASS( cycler_weapon,   WeaponCycler,   C_BaseCombatWeapon );
STUB_WEAPON_CLASS( weapon_cubemap,  WeaponCubemap,  C_BaseCombatWeapon );

//-----------------------------------------------------------------------------
// HACK: the detail sway convars are archive, and default to 0.  Existing CS:S players thus have no detail
// prop sway.  We'll force them to DoD's default values for now.  What we really need in the long run is
// a system to apply changes to archived convars' defaults to existing players.
extern ConVar cl_detail_max_sway;
extern ConVar cl_detail_avoid_radius;
extern ConVar cl_detail_avoid_force;
extern ConVar cl_detail_avoid_recover_speed;
// --------------------------------------------------------------------------------- //
// CSDKModeManager.
// --------------------------------------------------------------------------------- //

class CSDKModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( bool commander, bool force ) {}
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) {}
};

static CSDKModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;

// --------------------------------------------------------------------------------- //
// CSDKModeManager implementation.
// --------------------------------------------------------------------------------- //

#define SCREEN_FILE		"scripts/vgui_screens.txt"

void CSDKModeManager::Init()
{
	g_pClientMode = GetClientModeNormal();
	
	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );

	CTeamInfo::ParseTeamKits();
	CShoeInfo::ParseShoes();
	CKeeperGloveInfo::ParseKeeperGloves();
	CBallInfo::ParseBallSkins();

	if (g_pCVar->FindVar("playername")->GetString()[0] == '\0')
		g_pCVar->FindVar("playername")->SetValue(steamapicontext->SteamFriends()->GetPersonaName());
}

void CSDKModeManager::LevelInit( const char *newmap )
{
	g_pClientMode->LevelInit( newmap );
	// HACK: the detail sway convars are archive, and default to 0.  Existing CS:S players thus have no detail
	// prop sway.  We'll force them to DoD's default values for now.
	if ( !cl_detail_max_sway.GetFloat() &&
		!cl_detail_avoid_radius.GetFloat() &&
		!cl_detail_avoid_force.GetFloat() &&
		!cl_detail_avoid_recover_speed.GetFloat() )
	{
		cl_detail_max_sway.SetValue( "5" );
		cl_detail_avoid_radius.SetValue( "64" );
		cl_detail_avoid_force.SetValue( "0.4" );
		cl_detail_avoid_recover_speed.SetValue( "0.25" );
	}

	ClientModeSDKNormal::m_flLastMapChange = gpGlobals->curtime;
}

void CSDKModeManager::LevelShutdown( void )
{
	g_pClientMode->LevelShutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeSDKNormal::ClientModeSDKNormal()
{
}

//-----------------------------------------------------------------------------
// Purpose: If you don't know what a destructor is by now, you are probably going to get fired
//-----------------------------------------------------------------------------
ClientModeSDKNormal::~ClientModeSDKNormal()
{
}

void ClientModeSDKNormal::Init()
{
	BaseClass::Init();
}

void ClientModeSDKNormal::InitViewport()
{
	m_pViewport = new SDKViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

ClientModeSDKNormal g_ClientModeNormal;

IClientMode *GetClientModeNormal()
{
	return &g_ClientModeNormal;
}


ClientModeSDKNormal* GetClientModeSDKNormal()
{
	Assert( dynamic_cast< ClientModeSDKNormal* >( GetClientModeNormal() ) );

	return static_cast< ClientModeSDKNormal* >( GetClientModeNormal() );
}

float ClientModeSDKNormal::GetViewModelFOV( void )
{
	//Tony; retrieve the fov from the view model script, if it overrides it.
	float viewFov = 74.0;

	C_WeaponSDKBase *pWeapon = (C_WeaponSDKBase*)GetActiveWeapon();
	if ( pWeapon )
	{
		viewFov = pWeapon->GetWeaponFOV();
	}
	return viewFov;
}

int ClientModeSDKNormal::GetDeathMessageStartHeight( void )
{
	return m_pViewport->GetDeathMessageStartHeight();
}

#include "iefx.h"
#include "dlight.h"
#include "view.h"
#include "model_types.h"
#include "iosoptions.h"
#include "materialsystem\imaterialsystem.h"
#include "rendertexture.h"
#include "materialsystem/ITexture.h"

CHandle<C_BaseAnimatingOverlay> g_ClassImagePlayer;	// player
Vector camPos = vec3_invalid;
Vector newCamPos = vec3_invalid;
Vector oldCamPos = vec3_invalid;
float oldCamPosTime = FLT_MAX;
int oldBodypart = -1;
bool isAtTarget = false;

bool ShouldRecreateClassImageEntity( C_BaseAnimating* pEnt, const char* pNewModelName )
{
	if ( !pNewModelName || !pNewModelName[0] )
		return false;
	if ( !pEnt )
		return true;
 
	const model_t* pModel = pEnt->GetModel();
 
	if ( !pModel )
		return true;
	const char* pName = modelinfo->GetModelName( pModel );
 
	if ( !pName )
		return true;
	// reload only if names are different
	return( V_stricmp( pName, pNewModelName ) != 0 );
}

void UpdateClassImageEntity(const char* pModelName, float angle, int bodypart)
{
	MDLCACHE_CRITICAL_SECTION();

	ITexture *pRenderTarget = GetPlayerModelTexture();

	if(!pRenderTarget)
		return;
 
	if(!pRenderTarget->IsRenderTarget())
		Msg(" not a render target");

	C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();
 
	if ( !pLocalPlayer )
		return;
 
	C_BaseAnimatingOverlay* pModel = g_ClassImagePlayer.Get();
 
	// Does the entity even exist yet?
	bool recreatePlayer = ShouldRecreateClassImageEntity( pModel, pModelName );
	if ( recreatePlayer )
	{
		// if the pointer already exists, remove it as we create a new one.
		if ( pModel )
			pModel->Remove();
 
		// create a new instance
		pModel = new C_BaseAnimatingOverlay();
		pModel->InitializeAsClientEntity( pModelName, RENDER_GROUP_OPAQUE_ENTITY );
		pModel->AddEffects( EF_NODRAW ); // don't let the renderer draw the model normally
		// have the player stand idle
		pModel->SetSequence( pModel->LookupSequence( "walk_lower" ) );
		pModel->SetPoseParameter( 0, 0.0f ); // move_yaw
		pModel->SetPoseParameter( 1, 0.0f ); // body_pitch, look down a bit
		pModel->SetPoseParameter( 2, 0.0f ); // body_yaw
		pModel->SetPoseParameter( 3, 0.0f ); // move_y
		pModel->SetPoseParameter( 4, 0.0f ); // move_x

		pModel->m_nBody = pLocalPlayer->m_nBody;

		g_ClassImagePlayer = pModel;
	}

	CAppearanceSettingPanel *pPanel = (CAppearanceSettingPanel *)iosOptionsMenu->GetPanel()->GetSettingPanel(SETTING_PANEL_APPEARANCE);

	static int headBodyGroup = pModel->FindBodygroupByName("head");
	pModel->SetBodygroup(headBodyGroup, pPanel->GetPlayerSkinIndex());

	static int hairBodyGroup = pModel->FindBodygroupByName("hair");
	pModel->SetBodygroup(hairBodyGroup, pPanel->GetPlayerHairIndex());

	static int sleeveBodyGroup = pModel->FindBodygroupByName("sleeves");
	pModel->SetBodygroup(sleeveBodyGroup, pPanel->GetPlayerSleeveIndex());

	static int armBodyGroup = pModel->FindBodygroupByName("arms");
	pModel->SetBodygroup(armBodyGroup, pPanel->GetPlayerSleeveIndex() == 0 ? 1 : 0);
 
	Vector origin = pLocalPlayer->EyePosition();

	// move player model in front of our view
	pModel->SetAbsOrigin( origin );
	pModel->SetAbsAngles( QAngle( 0, 180 + angle, 0 ) );
 
	pModel->FrameAdvance( gpGlobals->frametime );

	// Now draw it.
	CViewSetup view;
	// setup the views location, size and fov (amongst others)
	view.x = 0;
	view.y = 0;
	view.width = pRenderTarget->GetActualWidth();
	view.height = pRenderTarget->GetActualHeight();
 
	view.m_bOrtho = false;
	view.fov = 54;
 
	// make sure that we see all of the player model
	Vector vMins, vMaxs;
	pModel->C_BaseAnimating::GetRenderBounds( vMins, vMaxs );

	Vector target;

	if (bodypart == 0)
	{
		target = origin + Vector(-25, 0, vMaxs.z - 9);
	}
	else if (bodypart == 1)
	{
		target = origin + Vector(-45, 0, (vMins.z + vMaxs.z) * 0.5f + 15);
	}
	else
	{
		target = origin + Vector(-25, 0, vMins.z + 20);
	}

	if (bodypart != oldBodypart)
	{
		if (camPos == vec3_invalid)
		{
			camPos = target;
			isAtTarget = true;
		}
		else
		{
			isAtTarget = false;
			newCamPos = target;
			oldCamPosTime = gpGlobals->curtime;
			oldCamPos = camPos;
		}

		oldBodypart = bodypart;
	}
	else if (bodypart == oldBodypart && !isAtTarget)
	{
		static const float interpTime = 0.75f;
		Vector dir = target - oldCamPos;
		float dist = VectorNormalize(dir);
		float timeFrac = min(1.0f, (gpGlobals->curtime - oldCamPosTime) / interpTime);

		if (timeFrac == 1.0f)
		{
			isAtTarget = true;
			camPos = target;
		}
		else
		{
			float frac = pow(timeFrac, 2) * (3 - 2 * timeFrac);
			camPos = oldCamPos + dir * dist * frac;
		}
	}
	else if (bodypart == oldBodypart && isAtTarget)
	{
		camPos = target;
	}

	view.origin = camPos;
	view.angles = QAngle(15, 0, 0);
	//view.m_vUnreflectedOrigin = view.origin;
	view.zNear = VIEW_NEARZ;
	view.zFar = 1000;
	//view.m_bForceAspectRatio1To1 = false;

	const float v = 1.0f;

	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->SetLightingOrigin( vec3_origin );
	pRenderContext->SetAmbientLight(v, v, v);

	static Vector white[6] = 
	{
		Vector(v, v, v),
		Vector(v, v, v),
		Vector(v, v, v),
		Vector(v, v, v),
		Vector(v, v, v),
		Vector(v, v, v),
	};

	g_pStudioRender->SetAmbientLightColors( white );
	g_pStudioRender->SetLocalLights( 0, NULL );
 
	// render it out to the new CViewSetup area
	// it's possible that ViewSetup3D will be replaced in future code releases
	Frustum dummyFrustum;

	// New Function instead of ViewSetup3D...
	render->Push3DView( view, VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH, pRenderTarget, dummyFrustum );

	modelrender->SuppressEngineLighting( true );
	float color[3] = { 1.0f, 1.0f, 1.0f };
	render->SetColorModulation( color );
	render->SetBlend( 1.0f );
 
	pModel->DrawModel( STUDIO_RENDER );

	modelrender->SuppressEngineLighting( false );
 
	render->PopView( dummyFrustum );
}

void ClientModeSDKNormal::PostRenderVGui()
{
}

void ClientModeSDKNormal::PostRenderVGuiOnTop()
{
	if (!CSDKPlayer::GetLocalSDKPlayer())
		return;

	CAppearanceSettingPanel *pAppearanceSettingPanel = (CAppearanceSettingPanel *)iosOptionsMenu->GetPanel()->GetSettingPanel(SETTING_PANEL_APPEARANCE);

	if (!pAppearanceSettingPanel->IsVisible())
		return;

	float angle = pAppearanceSettingPanel->GetPlayerPreviewAngle();
	int bodypart = pAppearanceSettingPanel->GetPlayerBodypart();
	UpdateClassImageEntity("models/player/player.mdl", angle, bodypart);
}

bool ClientModeSDKNormal::CanRecordDemo( char *errorMsg, int length ) const
{
	C_SDKPlayer *player = C_SDKPlayer::GetLocalSDKPlayer();
	if ( !player )
	{
		return true;
	}

	if ( !player->IsAlive() )
	{
		return true;
	}

	return true;
}
