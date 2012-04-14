//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "BaseAnimatedTextureProxy.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"
#include "materialsystem/ITexture.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "sdk_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CCrowdTextureProxy : public CBaseAnimatedTextureProxy
{
public:
	CCrowdTextureProxy() : m_flFrame(0), m_nFrameCount(0) {}
	virtual ~CCrowdTextureProxy() {}
	virtual float GetAnimationStartTime( void* pBaseEntity ) { return 0; }
	virtual void OnBind( void *pEntity );

private:
	float m_flFrame;
	int m_nFrameCount;
};

EXPOSE_INTERFACE( CCrowdTextureProxy, IMaterialProxy, "CrowdTexture" IMATERIAL_PROXY_INTERFACE_VERSION );

#pragma warning (disable : 4100)

void CCrowdTextureProxy::OnBind( void *pEntity )
{
	Assert ( m_AnimatedTextureVar );

	if( m_AnimatedTextureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE )
	{
		return;
	}

	ITexture *pTexture;
	pTexture = m_AnimatedTextureVar->GetTextureValue();
	int numFrames = pTexture->GetNumAnimationFrames();

	if ( numFrames <= 0 )
	{
		Assert( !"0 frames in material calling animated texture proxy" );
		return;
	}

	// NOTE: Must not use relative time based methods here
	// because the bind proxy can be called many times per frame.
	// Prevent multiple Wrap callbacks to be sent for no wrap mode

	if (SDKGameRules())
		m_FrameRate = max(0, (int)(30 * abs(SDKGameRules()->m_nBallZone) / 100.0f));
	else
		m_FrameRate = 0;

	if (m_nFrameCount != gpGlobals->framecount)
	{
		m_nFrameCount = gpGlobals->framecount;
		m_flFrame = fmod(m_flFrame + m_FrameRate * gpGlobals->frametime, numFrames);
	}

	m_AnimatedTextureFrameNumVar->SetIntValue( (int)m_flFrame );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}