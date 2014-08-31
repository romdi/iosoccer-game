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
#include "iosoptions.h"
#include "clientmode_sdk.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#include "Filesystem.h"

#include "pixelwriter.h"

class CProceduralRegenerator : public ITextureRegenerator
{
public:
	CProceduralRegenerator( void );
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect );
	virtual void Release( void );
	bool SetPlayerInfo(const char *name, int number, const Color &nameColor, int nameOffset, const Color &numberColor, int numberOffset, bool isKeeper, CFontAtlas *pFontAtlas);
	
private:
	unsigned char **ParseInfo(const char *filename, chr_t *chars, int &width, int &height);
	virtual void WriteText(CPixelWriter &pixelWriter, const char *text, unsigned char **pixels, const chr_t *chars, const int &width, const int &height, int offsetY, const Color &color, bool isKeeper);
	char m_szShirtName[MAX_PLAYER_NAME_LENGTH];
	char m_szShirtNumber[4];
	Color m_NameColor;
	int m_nNameOffset;
	Color m_NumberColor;
	int m_nNumberOffset;
	bool m_bIsKeeper;
	CFontAtlas *m_pFontAtlas;
};

CProceduralRegenerator::CProceduralRegenerator()
{
	m_szShirtName[0] = '\0';
	m_szShirtNumber[0] = '\0';
	m_NameColor = Color(0, 0, 0, 255);
	m_nNameOffset = 0;
	m_NumberColor = Color(0, 0, 0, 255);
	m_nNumberOffset = 0;
	m_bIsKeeper = false;
	m_pFontAtlas = NULL;
}

bool CProceduralRegenerator::SetPlayerInfo(const char *name, int number, const Color &nameColor, int nameOffset, const Color &numberColor, int numberOffset, bool isKeeper, CFontAtlas *pFontAtlas)
{
	bool hasChanged = false;

	if (Q_strcmp(name, m_szShirtName))
	{
		Q_strncpy(m_szShirtName, name, sizeof(m_szShirtName));
		hasChanged = true;
	}

	if (number != atoi(m_szShirtNumber))
	{
		Q_snprintf(m_szShirtNumber, sizeof(m_szShirtNumber), "%d", number);
		hasChanged = true;
	}

	if (nameColor != m_NameColor)
	{
		m_NameColor = nameColor;
		hasChanged = true;
	}

	if (nameOffset != m_nNameOffset)
	{
		m_nNameOffset = nameOffset;
		hasChanged = true;
	}

	if (numberColor != m_NumberColor)
	{
		m_NumberColor = numberColor;
		hasChanged = true;
	}

	if (numberOffset != m_nNumberOffset)
	{
		m_nNumberOffset = numberOffset;
		hasChanged = true;
	}

	if (isKeeper != m_bIsKeeper)
	{
		m_bIsKeeper = isKeeper;
		hasChanged = true;
	}

	if (pFontAtlas != m_pFontAtlas)
	{
		m_pFontAtlas = pFontAtlas;
		hasChanged = true;
	}

	return hasChanged;
}

void CProceduralRegenerator::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
{
	CPixelWriter pixelWriter;
	pixelWriter.SetPixelMemory( pVTFTexture->Format(), 
		pVTFTexture->ImageData(), pVTFTexture->RowSizeInBytes( 0 ) );

	// Now upload the part we've been asked for
	int xmax = pSubRect->x + pSubRect->width;
	//int xmax = pVTFTexture->Width();
	int ymax = pSubRect->y + pSubRect->height;
	//int ymax = pVTFTexture->Height();
	int x, y;

	//x=250-450,y=275-325

	for( y = pSubRect->y; y < ymax; ++y )
	{
		pixelWriter.Seek( pSubRect->x, y );

		for( x=pSubRect->x; x < xmax; ++x )
		{
			int r=0, g=0, b=0, a=0;
			//pixelWriter.ReadPixelNoAdvance( r, g, b, a );
			pixelWriter.WritePixel( r, g, b, a );
		}
	}

	WriteText(pixelWriter, m_szShirtName, m_pFontAtlas->m_NamePixels, m_pFontAtlas->m_NameChars, m_pFontAtlas->m_nNamePixelsWidth, m_pFontAtlas->m_nNamePixelsHeight, m_nNameOffset, m_NameColor, m_bIsKeeper);
	WriteText(pixelWriter, m_szShirtNumber, m_pFontAtlas->m_NumberPixels, m_pFontAtlas->m_NumberChars, m_pFontAtlas->m_nNumberPixelsWidth, m_pFontAtlas->m_nNumberPixelsHeight, m_nNumberOffset, m_NumberColor, m_bIsKeeper);	
}

void CProceduralRegenerator::WriteText(CPixelWriter &pixelWriter, const char *text, unsigned char **pixels, const chr_t *chars, const int &width, const int &height, int offsetY, const Color &color, bool isKeeper)
{
	int offsetX = isKeeper ? 600 : 350;
	int totalWidth = 0;
	int maxHeight = 0;

	for (int i = strlen(text) - 1; i >= 0; i--)
	{
		const chr_t &chr = chars[text[i]];
		totalWidth += chr.w;
		maxHeight = max(maxHeight, chr.h);
	}

	int posX = offsetX - totalWidth / 2;
	int posY = offsetY - maxHeight / 2;

	for (int i = strlen(text) - 1; i >= 0; i--)
	{
		const chr_t &chr = chars[text[i]];

		for(int y = 0; y < chr.h; y++)
		{
			for(int x = 0; x < chr.w; x++)
			{
				pixelWriter.Seek(posX + x, offsetY + y);
				int srcY = chr.y + (chr.h - 1 - y);
				int srcX = chr.x + (chr.w - 1 - x);
				pixelWriter.WritePixel(color.r(), color.g(), color.b(), color.a() * (pixels[srcY][srcX] / 255.0f));
			}
		}

		posX += chr.w;
	}
}

void CProceduralRegenerator::Release()
{
	//delete stuff
}

#define TEAMKITS_PATH "models/player/teamkits"

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
	IMaterialVar	*m_pDetailTextureVar;
	ITexture		*m_pTexture;		// default texture
	char m_szTextureType[64];
	CProceduralRegenerator	*m_pTextureRegen[2][11]; // The regenerator
	CProceduralRegenerator *m_pPreviewTextureRegen;
	float m_flLastPreviewTextureUpdate;
	float m_flLastTextureUpdate[2][11];
};

CPlayerTextureProxy::CPlayerTextureProxy()
{
	m_pBaseTextureVar = NULL;
	m_pDetailTextureVar = NULL;
	m_pTexture = NULL;

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			m_pTextureRegen[i][j] = NULL;
			m_flLastTextureUpdate[2][11] = -1;
		}
	}

	m_pPreviewTextureRegen = NULL;
	m_flLastPreviewTextureUpdate = -1;
}

CPlayerTextureProxy::~CPlayerTextureProxy()
{
}

void CPlayerTextureProxy::Release()
{
//	for (int i = 0; i < 2; i++)
//		for (int j = 0; j < 11; j++)
//			delete m_pTextureRegen[i][j];
//
//	delete m_pPreviewTextureRegen;
//
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
	m_pTexture = m_pBaseTextureVar->GetTextureValue();

	if ( IsErrorTexture( m_pTexture ) )
		return false;
	
	Q_strncpy(m_szTextureType, pKeyValues->GetString("type"), sizeof(m_szTextureType));

	if (!Q_strcmp(m_szTextureType, "shirt") || !Q_strcmp(m_szTextureType, "keepershirt"))
	{
		m_pDetailTextureVar = pMaterial->FindVar("$detail", NULL);
	}

	return true;
}

void CPlayerTextureProxy::OnBind( C_BaseEntity *pEnt )
{
	// Bail if no base variable
	if ( !m_pBaseTextureVar )
		return;

	CTeamKitInfo *pKitInfo;
	const char *teamFolder;
	const char *kitFolder;
	int skinIndex;
	int shirtNumber;
	const char *shirtName;
	bool isKeeper;

	ITexture *pDetailTexture;
	CProceduralRegenerator **pProcReg;
	float *pLastTextureUpdate;

	if (dynamic_cast<C_SDKPlayer *>(pEnt))
	{
		C_SDKPlayer *pPl = dynamic_cast<C_SDKPlayer *>(pEnt);

		C_Team *pTeam = GetGlobalTeam(g_PR->GetTeam(pPl->index));

		teamFolder = pTeam->GetFolderName();
		kitFolder = pTeam->GetKitFolderName();
		pKitInfo = pTeam->GetKitInfo();
		skinIndex = g_PR->GetSkinIndex(pPl->index);
		shirtNumber = g_PR->GetShirtNumber(pPl->index);
		shirtName = g_PR->GetShirtName(pPl->index);
		isKeeper = g_PR->GetTeamPosType(pPl->index) == POS_GK;

		int teamIndex = pTeam->GetTeamNumber() - TEAM_A;
		int posIndex = g_PR->GetTeamPosIndex(pPl->index);

		pDetailTexture = materials->FindTexture(VarArgs("models/player/default/detail_%d_%d", teamIndex, posIndex), NULL, true);
		pProcReg = &m_pTextureRegen[teamIndex][posIndex];
		pLastTextureUpdate = &m_flLastTextureUpdate[teamIndex][posIndex];
	}
	else if (dynamic_cast<C_ReplayPlayer *>(pEnt))
	{
		C_ReplayPlayer *pReplayPl = dynamic_cast<C_ReplayPlayer *>(pEnt);

		C_Team *pTeam = GetGlobalTeam(pReplayPl->m_nTeamNumber);

		teamFolder = pTeam->GetFolderName();
		kitFolder = pTeam->GetKitFolderName();
		pKitInfo = pTeam->GetKitInfo();
		skinIndex = pReplayPl->m_nSkinIndex;
		shirtNumber = pReplayPl->m_nShirtNumber;
		shirtName = pReplayPl->m_szShirtName;
		isKeeper = pReplayPl->m_bIsKeeper;

		int teamIndex = pReplayPl->m_nTeamNumber - TEAM_A;
		int posIndex = pReplayPl->m_nTeamPosIndex;

		pDetailTexture = materials->FindTexture(VarArgs("models/player/default/detail_%d_%d", teamIndex, posIndex), NULL, true);
		pProcReg = &m_pTextureRegen[teamIndex][posIndex];
		pLastTextureUpdate = &m_flLastTextureUpdate[teamIndex][posIndex];
	}
	else if (dynamic_cast<C_BaseAnimatingOverlay *>(pEnt))
	{
		C_BaseAnimatingOverlay *pPlayerModelPreview = dynamic_cast<C_BaseAnimatingOverlay *>(pEnt);

		CAppearanceSettingPanel *pPanel = (CAppearanceSettingPanel *)iosOptionsMenu->GetPanel()->GetSettingPanel(SETTING_PANEL_APPEARANCE);

		pPanel->GetPlayerTeamInfo(&teamFolder, &kitFolder);
		pKitInfo = CTeamInfo::FindTeamByKitName(VarArgs("%s/%s", teamFolder, kitFolder));
		skinIndex = pPanel->GetPlayerSkinIndex();
		shirtNumber = pPanel->GetPlayerOutfieldShirtNumber();
		shirtName = pPanel->GetPlayerShirtName();
		isKeeper = false;

		pDetailTexture = materials->FindTexture("models/player/default/detail_preview", NULL, true);
		pProcReg = &m_pPreviewTextureRegen;
		pLastTextureUpdate = &m_flLastPreviewTextureUpdate;
	}
	else
	{
		return;
	}

	if (!Q_strcmp(m_szTextureType, "shirt") || !Q_strcmp(m_szTextureType, "keepershirt"))
	{
		if (!(*pProcReg))
			*pProcReg = new CProceduralRegenerator();

		pDetailTexture->SetTextureRegenerator(*pProcReg);
		m_pDetailTextureVar->SetTextureValue(pDetailTexture);

		bool needsUpdate;

		if (isKeeper)
			needsUpdate = (*pProcReg)->SetPlayerInfo(shirtName, shirtNumber, pKitInfo->m_KeeperShirtNameColor, pKitInfo->m_nKeeperShirtNameOffset, pKitInfo->m_KeeperShirtNumberColor, pKitInfo->m_nKeeperShirtNumberOffset, true, pKitInfo->m_pFontAtlas);
		else
			needsUpdate = (*pProcReg)->SetPlayerInfo(shirtName, shirtNumber, pKitInfo->m_OutfieldShirtNameColor, pKitInfo->m_nOutfieldShirtNameOffset, pKitInfo->m_OutfieldShirtNumberColor, pKitInfo->m_nOutfieldShirtNumberOffset, false, pKitInfo->m_pFontAtlas);
				
		if (needsUpdate || *pLastTextureUpdate <= ClientModeSDKNormal::m_flLastMapChange)
		{
			pDetailTexture->Download();
			*pLastTextureUpdate = gpGlobals->curtime;
		}
	}

	char texture[128];

	if (Q_stricmp(m_szTextureType, "shirt") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/%s/outfield", TEAMKITS_PATH, teamFolder, kitFolder);
	else if (Q_stricmp(m_szTextureType, "keepershirt") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/%s/keeper", TEAMKITS_PATH, teamFolder, kitFolder);
	else if (Q_stricmp(m_szTextureType, "socks") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/%s/socks", TEAMKITS_PATH, teamFolder, kitFolder);
	else if (Q_stricmp(m_szTextureType, "gksocks") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/%s/gksocks", TEAMKITS_PATH, teamFolder, kitFolder);
	else if (Q_stricmp(m_szTextureType, "skin") == 0)
		Q_snprintf(texture, sizeof(texture), "models/player/skins/skin%d", skinIndex + 1);
	else
		Q_snprintf(texture, sizeof(texture), "%s", m_pTexture->GetName());

	ITexture *pNewTex = materials->FindTexture(texture, NULL, false);

	if (!pNewTex->IsError())
		m_pTexture = pNewTex;

	m_pBaseTextureVar->SetTextureValue(m_pTexture);
		
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

	if (Q_stricmp(m_szTextureType, "hometeamcrest") == 0 && GetGlobalTeam(TEAM_A)->HasCrest())
		Q_snprintf(texture, sizeof(texture), "%s/%s/teamcrest", TEAMKITS_PATH, GetGlobalTeam(TEAM_A)->GetFolderName());
	else if (Q_stricmp(m_szTextureType, "awayteamcrest") == 0 && GetGlobalTeam(TEAM_B)->HasCrest())
		Q_snprintf(texture, sizeof(texture), "%s/%s/teamcrest", TEAMKITS_PATH, GetGlobalTeam(TEAM_B)->GetFolderName());
	else
		Q_snprintf(texture, sizeof(texture), "%s", m_pDefaultTexture->GetName());

	m_pNewTexture = materials->FindTexture(texture, NULL, true);
		
	m_pBaseTextureVar->SetTextureValue(m_pNewTexture);

	GetMaterial()->RecomputeStateSnapshots();
}

EXPOSE_INTERFACE( CTextureProxy, IMaterialProxy, "Texture" IMATERIAL_PROXY_INTERFACE_VERSION );
