#include "cbase.h"
#include "ios_rendertargets.h"
#include "materialsystem\imaterialsystem.h"
#include "rendertexture.h"
 
ITexture* CIOSRenderTargets::CreatePlayerModelTexture( IMaterialSystem* pMaterialSystem )
{
//	DevMsg("Creating Scope Render Target: _rt_Scope\n");
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_playermodel",
		256, 512, RT_SIZE_OFFSCREEN,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED, 
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
}
 
//-----------------------------------------------------------------------------
// Purpose: Called by the engine in material system init and shutdown.
//			Clients should override this in their inherited version, but the base
//			is to init all standard render targets for use.
// Input  : pMaterialSystem - the engine's material system (our singleton is not yet inited at the time this is called)
//			pHardwareConfig - the user hardware config, useful for conditional render target setup
//-----------------------------------------------------------------------------
void CIOSRenderTargets::InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig )
{ 
	m_PlayerModelTexture.Init( CreatePlayerModelTexture( pMaterialSystem ) ); 
 
	// Water effects & camera from the base class (standard HL2 targets) 
	BaseClass::InitClientRenderTargets( pMaterialSystem, pHardwareConfig );
}
 
//-----------------------------------------------------------------------------
// Purpose: Shut down each CTextureReference we created in InitClientRenderTargets.
//			Called by the engine in material system shutdown.
// Input  :  - 
//-----------------------------------------------------------------------------
void CIOSRenderTargets::ShutdownClientRenderTargets()
{ 
	m_PlayerModelTexture.Shutdown();
 
	// Clean up standard HL2 RTs (camera and water) 
	BaseClass::ShutdownClientRenderTargets();
}
 
//add the interface!
static CIOSRenderTargets g_IOSRenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CIOSRenderTargets, IClientRenderTargets, CLIENTRENDERTARGETS_INTERFACE_VERSION, g_IOSRenderTargets  );
CIOSRenderTargets* IOSRenderTargets = &g_IOSRenderTargets;