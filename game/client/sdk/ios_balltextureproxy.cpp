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
#include "ProxyEntity.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"
#include "materialsystem/ITexture.h"
#include "c_ball.h"
#include "c_ios_replaymanager.h"
#include "iosoptions.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CBallTextureProxy : public CEntityMaterialProxy
{
public:
	CBallTextureProxy();
	~CBallTextureProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( C_BaseEntity *pEnt );
	virtual void Release( void );
	virtual IMaterial *GetMaterial() {return m_pBaseTextureVar->GetOwningMaterial();}

private:
	IMaterialVar	*m_pBaseTextureVar;		// variable for our base texture
	ITexture		*m_pDefaultTexture;		// default texture
	ITexture		*m_pNewTexture;		// default texture
	char m_szTextureType[64];
};

CBallTextureProxy::CBallTextureProxy()
{
	m_pBaseTextureVar = NULL;
	m_pDefaultTexture = NULL;
	m_pNewTexture = NULL;
}

CBallTextureProxy::~CBallTextureProxy()
{
	// Do nothing
}
void CBallTextureProxy::Release()
{
	m_pBaseTextureVar = NULL;
	m_pDefaultTexture = NULL;
	m_pNewTexture = NULL;
	delete this;
}

bool CBallTextureProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	#ifdef ALLPROXIESFAIL
	return false;
	#endif
	
	// Check for $basetexture variable
	m_pBaseTextureVar = pMaterial->FindVar( "$basetexture", NULL );

	if ( !m_pBaseTextureVar )
		return false;

	// Set default texture and make sure its not an error texture
	m_pDefaultTexture = m_pBaseTextureVar->GetTextureValue();

	if ( IsErrorTexture( m_pDefaultTexture ) )
		return false;
	
	Q_strncpy(m_szTextureType, pKeyValues->GetString("type"), sizeof(m_szTextureType));

	return true;
}

void CBallTextureProxy::OnBind( C_BaseEntity *pEnt )
{
	// Bail if no base variable
	if ( !m_pBaseTextureVar )
		return;

	const char *name;

	if (dynamic_cast<C_Ball *>(pEnt))
	{
		C_Ball *pBall = dynamic_cast<C_Ball *>(pEnt);
		name = pBall->GetSkinName();
	}
	else if (dynamic_cast<C_BaseAnimatingOverlay *>(pEnt))
	{
		CAppearanceSettingPanel *pPanel = (CAppearanceSettingPanel *)iosOptionsMenu->GetPanel()->GetSettingPanel(SETTING_PANEL_APPEARANCE);
		name = pPanel->GetBallName();
	}
	else if (dynamic_cast<C_ReplayBall *>(pEnt))
	{
		C_ReplayBall *pReplayBall = dynamic_cast<C_ReplayBall *>(pEnt);
		name = pReplayBall->m_szSkinName;
	}
	else
	{
		return;
	}

	char texture[128];
	Q_snprintf(texture, sizeof(texture), "models/ball/skins/%s/ball.vtf", name);
	m_pNewTexture = materials->FindTexture(texture, NULL, true);
	m_pBaseTextureVar->SetTextureValue(m_pNewTexture);

	GetMaterial()->RecomputeStateSnapshots();
}

EXPOSE_INTERFACE( CBallTextureProxy, IMaterialProxy, "BallTexture" IMATERIAL_PROXY_INTERFACE_VERSION );