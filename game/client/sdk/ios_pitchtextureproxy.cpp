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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PITCH_TEXTURE_COUNT 8

const char *g_szPitchTextures[PITCH_TEXTURE_COUNT] =
{
	"grass_stripe",
	"grass_stripe_narrow",
	"pitch2_astro",
	"pitch2_frozen",
	"pitch2_grass",
	"pitch2_grass_alt",
	"pitch2_mud",
	"pitch2_sand"
};

class CPitchTextureProxy : public IMaterialProxy
{
public:
	CPitchTextureProxy();
	~CPitchTextureProxy();
	virtual bool Init(IMaterial *pMaterial, KeyValues *pKeyValues);
	virtual void OnBind(void *pC_BaseEntity);
	virtual void Release();
	virtual IMaterial *GetMaterial() { return m_pBaseTextureVar->GetOwningMaterial(); }

private:
	IMaterialVar	*m_pBaseTextureVar;		// variable for our base texture
	ITexture		*m_pDefaultTexture;		// default texture
	ITexture		*m_pNewTexture;		// default texture
};

CPitchTextureProxy::CPitchTextureProxy()
{
	m_pBaseTextureVar = NULL;
	m_pDefaultTexture = NULL;
	m_pNewTexture = NULL;
}

CPitchTextureProxy::~CPitchTextureProxy()
{
	// Do nothing
}
void CPitchTextureProxy::Release()
{
	m_pBaseTextureVar = NULL;
	m_pDefaultTexture = NULL;
	m_pNewTexture = NULL;
	delete this;
}

bool CPitchTextureProxy::Init(IMaterial *pMaterial, KeyValues *pKeyValues)
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
	
	return true;
}

void CPitchTextureProxy::OnBind(void *pC_BaseEntity)
{
	// Bail if no base variable
	if ( !m_pBaseTextureVar )
		return;

	char texture[64];

	Q_snprintf(texture, sizeof(texture), "pitch/%s.vtf", g_szPitchTextures[clamp(mp_pitchtexture.GetInt(), 0, PITCH_TEXTURE_COUNT - 1)]);

	m_pNewTexture = materials->FindTexture(texture, NULL, true);
		
	m_pBaseTextureVar->SetTextureValue(m_pNewTexture);

	//GetMaterial()->RecomputeStateSnapshots();
}

EXPOSE_INTERFACE(CPitchTextureProxy, IMaterialProxy, "PitchTexture" IMATERIAL_PROXY_INTERFACE_VERSION);