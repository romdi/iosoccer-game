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
	CCrowdTextureProxy() {}
	virtual ~CCrowdTextureProxy() {}
	virtual float GetAnimationStartTime( void* pBaseEntity );
	virtual void OnBind( void *pEntity );
};

EXPOSE_INTERFACE( CCrowdTextureProxy, IMaterialProxy, "CrowdTexture" IMATERIAL_PROXY_INTERFACE_VERSION );

#pragma warning (disable : 4100)

float CCrowdTextureProxy::GetAnimationStartTime( void* pBaseEntity )
{
	return 0;
}

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
	float startTime = GetAnimationStartTime(pEntity);
	float deltaTime = gpGlobals->curtime - startTime;
	float prevTime = deltaTime - gpGlobals->frametime;

	// Clamp..
	if (deltaTime < 0.0f)
		deltaTime = 0.0f;
	if (prevTime < 0.0f)
		prevTime = 0.0f;

	if (SDKGameRules())
		m_FrameRate = max(0, (int)(30 * abs(SDKGameRules()->m_nBallZone - 50) * 2 / 100.0f));
	else
		m_FrameRate = 0;

	float frame = m_FrameRate * deltaTime;	
	float prevFrame = m_FrameRate * prevTime;

	int intFrame = ((int)frame) % numFrames; 
	int intPrevFrame = ((int)prevFrame) % numFrames;

	// Report wrap situation...
	if (intPrevFrame > intFrame)
	{
		if (m_WrapAnimation)
		{
			AnimationWrapped( pEntity );
		}
		else
		{
			// Only sent the wrapped message once.
			// when we're in non-wrapping mode
			if (prevFrame < numFrames)
				AnimationWrapped( pEntity );
			intFrame = numFrames - 1;
		}
	}

	m_AnimatedTextureFrameNumVar->SetIntValue( intFrame );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}