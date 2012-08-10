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
#include "c_ios_replaymanager.h"

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
	ITexture		*m_pNewTexture;		// default texture
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

	const char *team;
	int pos;

	C_SDKPlayer *pPl = dynamic_cast<C_SDKPlayer *>(pEnt);
	if (pPl)
	{
		team = GameResources()->GetTeamKitName(GameResources()->GetTeam(pEnt->index));
		pos = GameResources()->GetTeamPosNum(pEnt->index);
	}
	else
	{
		C_ReplayPlayer *pReplayPlayer = dynamic_cast<C_ReplayPlayer *>(pEnt);
		if (pReplayPlayer)
		{
			team = GameResources()->GetTeamKitName(pReplayPlayer->m_nTeamNumber);
			pos = pReplayPlayer->m_nTeamPosNum;
		}
		else
			return;
	}

	char texture[128];

	if (Q_stricmp(m_szTextureType, "shirt") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/%d", PLAYERTEXTURE_PATH, team, pos);
	else if (Q_stricmp(m_szTextureType, "keeper") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/keeper", PLAYERTEXTURE_PATH, team);
	else if (Q_stricmp(m_szTextureType, "socks") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/socks", PLAYERTEXTURE_PATH, team);
	else if (Q_stricmp(m_szTextureType, "gksocks") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/gksocks", PLAYERTEXTURE_PATH, team);
	else if (Q_stricmp(m_szTextureType, "skin") == 0)
		Q_snprintf(texture, sizeof(texture), "%s", m_pDefaultTexture->GetName());
	else
		Q_snprintf(texture, sizeof(texture), "%s", m_pDefaultTexture->GetName());

	m_pNewTexture = materials->FindTexture(texture, NULL, true);
		
	m_pBaseTextureVar->SetTextureValue(m_pNewTexture);

	GetMaterial()->RecomputeStateSnapshots();
}

EXPOSE_INTERFACE( CPlayerTextureProxy, IMaterialProxy, "PlayerTexture" IMATERIAL_PROXY_INTERFACE_VERSION );


class CTextureProxy : public IMaterialProxy
{
public:
	CTextureProxy();
	~CTextureProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pEntity );
	virtual void Release( void );
	virtual IMaterial *GetMaterial() {return m_pBaseTextureVar->GetOwningMaterial();}

private:
	IMaterialVar	*m_pBaseTextureVar;		// variable for our base texture
	ITexture		*m_pDefaultTexture;		// default texture
	ITexture		*m_pNewTexture;		// default texture
	char m_szTextureType[64];
};

CTextureProxy::CTextureProxy()
{
	m_pBaseTextureVar = NULL;
	m_pDefaultTexture = NULL;
	m_pNewTexture = NULL;
}

CTextureProxy::~CTextureProxy()
{
	// Do nothing
}
void CTextureProxy::Release()
{
	m_pBaseTextureVar = NULL;
	m_pDefaultTexture = NULL;
	m_pNewTexture = NULL;
	delete this;
}

bool CTextureProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
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

void CTextureProxy::OnBind( void *pEntity )
{
	// Bail if no base variable
	if ( !m_pBaseTextureVar )
		return;

	char texture[128];

	if (Q_stricmp(m_szTextureType, "hometeamcrest") == 0 && GameResources()->HasTeamCrest(TEAM_A))
		Q_snprintf(texture, sizeof(texture), "%s/%s/teamcrest", PLAYERTEXTURE_PATH, GameResources()->GetTeamKitName(TEAM_A));
	else if (Q_stricmp(m_szTextureType, "awayteamcrest") == 0 && GameResources()->HasTeamCrest(TEAM_B))
		Q_snprintf(texture, sizeof(texture), "%s/%s/teamcrest", PLAYERTEXTURE_PATH, GameResources()->GetTeamKitName(TEAM_B));
	else
		Q_snprintf(texture, sizeof(texture), "%s", m_pDefaultTexture->GetName());

	m_pNewTexture = materials->FindTexture(texture, NULL, true);
		
	m_pBaseTextureVar->SetTextureValue(m_pNewTexture);

	GetMaterial()->RecomputeStateSnapshots();
}

EXPOSE_INTERFACE( CTextureProxy, IMaterialProxy, "Texture" IMATERIAL_PROXY_INTERFACE_VERSION );