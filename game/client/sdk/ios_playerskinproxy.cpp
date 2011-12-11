#include "cbase.h"
#include "ProxyEntity.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"
#include "materialsystem/ITexture.h"
#include <KeyValues.h>

#include "c_sdk_player.h"
#include "c_sdk_team.h"
#include "sdk_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CPlayerSkinProxy : public CEntityMaterialProxy
{
public:
	CPlayerSkinProxy();
	~CPlayerSkinProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( C_BaseEntity *pEnt );
	virtual void Release( void );
	virtual IMaterial *GetMaterial() {return m_pBaseTextureVar->GetOwningMaterial();}

private:
	IMaterialVar	*m_pBaseTextureVar;		// variable for our base texture
	ITexture		*m_pDefaultTexture;		// default texture
	ITexture		*m_pNewTexture;			// replacement texture
	char type[64];
};

CPlayerSkinProxy::CPlayerSkinProxy()
{
	m_pBaseTextureVar = NULL;
	m_pDefaultTexture = NULL;
	m_pNewTexture = NULL;
}

CPlayerSkinProxy::~CPlayerSkinProxy()
{
	// Do nothing
}
void CPlayerSkinProxy::Release()
{
	m_pBaseTextureVar = NULL;
	m_pDefaultTexture = NULL;
	m_pNewTexture = NULL;
	delete this;
}

bool CPlayerSkinProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
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
	
	Q_strncpy(type, pKeyValues->GetString("type"), sizeof(type));

	return true;
}

void CPlayerSkinProxy::OnBind( C_BaseEntity *pEnt )
{
	// Bail if no base variable
	if ( !m_pBaseTextureVar )
		return;

	IMaterial *pMaterial = GetMaterial();

	C_SDKPlayer *pPlayer = (C_SDKPlayer *)pEnt;

	const char *team = pPlayer->GetTeam()->Get_Name();
	int pos = pPlayer->m_ShirtPos;

	if ( strlen(team) > 0 )
	{
		char skin[64];
		
		if (Q_stricmp(type, "shirt") == 0)
		{
			Q_snprintf(skin, sizeof(skin), "models/player_new/%s/%i", team, pos);
			/*
			float c = pow(abs(((int)(5 * gpGlobals->curtime) % 101) - 50) / 50.0f * 1.5f + 0.5f, 2.2f);
			IMaterialVar *var = pMaterial->FindVar("$color", NULL);
			const char* val = var->GetStringValue();
			var->SetValueAutodetectType(VarArgs("[ %f %f %f ]", 1.0f, c, 1.0f));*/
		}
		else if (Q_stricmp(type, "keeper") == 0)
			Q_snprintf(skin, sizeof(skin), "models/player_new/%s/keeper", team);
		else if (Q_stricmp(type, "socks") == 0)
			Q_snprintf(skin, sizeof(skin), "models/player_new/%s/socks", team);
		else if (Q_stricmp(type, "gksocks") == 0)
			Q_snprintf(skin, sizeof(skin), "models/player_new/%s/gksocks", team);
		else if (Q_stricmp(type, "skin") == 0)
		{
			/*float c = pow(abs(((int)(5 * gpGlobals->curtime) % 101) - 50) / 50.0f * 1.5f + 0.5f, 2.2f);
			IMaterialVar *var = pMaterial->FindVar("$color", NULL);
			const char* val = var->GetStringValue();
			var->SetValueAutodetectType(VarArgs("[ %f %f %f ]", c, c, c));*/
			return;
		}

		m_pNewTexture = materials->FindTexture( skin, NULL);
		m_pBaseTextureVar->SetTextureValue( m_pNewTexture );
	}
	else
		m_pBaseTextureVar->SetTextureValue( m_pDefaultTexture );

	GetMaterial()->RecomputeStateSnapshots();
}

EXPOSE_INTERFACE( CPlayerSkinProxy, IMaterialProxy, "PlayerSkin" IMATERIAL_PROXY_INTERFACE_VERSION );