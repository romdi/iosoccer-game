#include "cbase.h"
#include "ProxyEntity.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"
#include "materialsystem/ITexture.h"
#include <KeyValues.h>

#include "c_sdk_player.h"
#include "c_team.h"
#include "sdk_gamerules.h"
#include "c_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CPlayerTextureProxy : public CEntityMaterialProxy
{
public:
	CPlayerTextureProxy();
	~CPlayerTextureProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( C_BaseEntity *pEnt );
	virtual void Release( void );
	virtual IMaterial *GetMaterial() {return m_pBaseTextureVar->GetOwningMaterial();}

private:
	IMaterialVar	*m_pBaseTextureVar;		// variable for our base texture
	ITexture		*m_pDefaultTexture;		// default texture
	ITexture		*m_pNewTexture;			// replacement texture
	char m_szTextureType[64];
};

CPlayerTextureProxy::CPlayerTextureProxy()
{
	m_pBaseTextureVar = NULL;
	m_pDefaultTexture = NULL;
	m_pNewTexture = NULL;
}

CPlayerTextureProxy::~CPlayerTextureProxy()
{
	// Do nothing
}
void CPlayerTextureProxy::Release()
{
	m_pBaseTextureVar = NULL;
	m_pDefaultTexture = NULL;
	m_pNewTexture = NULL;
	delete this;
}

bool CPlayerTextureProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
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

#define PLAYERTEXTURE_PATH "models/player/teams"

void CPlayerTextureProxy::OnBind( C_BaseEntity *pEnt )
{
	// Bail if no base variable
	if ( !m_pBaseTextureVar )
		return;

	const char *team = GameResources()->GetTeamKitName(GameResources()->GetTeam(pEnt->index));
	int pos = GameResources()->GetTeamPosition(pEnt->index);

	char texture[64];

	if (Q_stricmp(m_szTextureType, "shirt") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/%d", PLAYERTEXTURE_PATH, team, pos);
	else if (Q_stricmp(m_szTextureType, "keeper") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/keeper", PLAYERTEXTURE_PATH, team);
	else if (Q_stricmp(m_szTextureType, "socks") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/socks", PLAYERTEXTURE_PATH, team);
	else if (Q_stricmp(m_szTextureType, "gksocks") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/gksocks", PLAYERTEXTURE_PATH, team);
	else if (Q_stricmp(m_szTextureType, "skin") == 0)
		Q_snprintf(texture, sizeof(texture), m_pDefaultTexture->GetName(), team);

	m_pDefaultTexture = materials->FindTexture(texture, NULL, true);
		
	m_pBaseTextureVar->SetTextureValue(m_pDefaultTexture);

	GetMaterial()->RecomputeStateSnapshots();
}

EXPOSE_INTERFACE( CPlayerTextureProxy, IMaterialProxy, "PlayerTexture" IMATERIAL_PROXY_INTERFACE_VERSION );