#ifndef IOSRENDERTARGETS_H_
#define IOSRENDERTARGETS_H_
#ifdef _WIN32
#pragma once
#endif
 
#include "baseclientrendertargets.h" // Base class, with interfaces called by engine and inherited members to init common render   targets
 
// externs
class IMaterialSystem;
class IMaterialSystemHardwareConfig;
 
class CIOSRenderTargets : public CBaseClientRenderTargets
{ 
	// no networked vars 
	DECLARE_CLASS_GAMEROOT( CIOSRenderTargets, CBaseClientRenderTargets );
public: 
	virtual void InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig );
	virtual void ShutdownClientRenderTargets();
 
	ITexture* CreatePlayerModelTexture( IMaterialSystem* pMaterialSystem );
 
private:
	CTextureReference		m_PlayerModelTexture; 
};
 
extern CIOSRenderTargets* IOSRenderTargets;
 
#endif //IOSRENDERTARGETS_H_