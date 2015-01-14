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
	bool SetPlayerInfo(const char *name, int number, const Color &shirtNameAndNumberColor, const Color &shirtNumberOutlineColor, const Color &shirtBackNameOutlineColor,
						int shirtBackNameVerticalOffset, int shirtBackNumberVerticalOffset, bool hasShirtFrontNumber, int shirtFrontNumberHorizontalOffset, int shirtFrontNumberVerticalOffset, bool hasShortsFrontNumber,
						const Color &shortsFrontNumberColor, const Color &shortsFrontNumberOutlineColor, int shortsFrontNumberHorizontalOffset, int shortsFrontNumberVerticalOffset, bool isKeeper, CFontAtlas *pFontAtlas);
private:
	virtual void WriteText(CPixelWriter &pixelWriter, const char *text, glyphWithOutline_t **pixels, const chr_t *chars, const int &width, const int &height, int offsetX, int offsetY, const Color &color, bool isKeeper, bool isOutline);
	char m_szShirtName[MAX_PLAYER_NAME_LENGTH];
	char m_szShirtNumber[4];
	bool m_bIsKeeper;
	CFontAtlas *m_pFontAtlas;

	Color m_ShirtNameAndNumberColor;
	
	Color m_ShirtNumberOutlineColor;

	Color m_ShirtBackNameOutlineColor;
	int m_nShirtBackNameVerticalOffset;

	int m_nShirtBackNumberVerticalOffset;

	bool m_bHasShirtFrontNumber;
	int m_nShirtFrontNumberHorizontalOffset;
	int m_nShirtFrontNumberVerticalOffset;

	bool m_bHasShortsFrontNumber;
	Color m_ShortsFrontNumberColor;
	Color m_ShortsFrontNumberOutlineColor;
	int m_nShortsFrontNumberHorizontalOffset;
	int m_nShortsFrontNumberVerticalOffset;
};

CProceduralRegenerator::CProceduralRegenerator()
{
	m_szShirtName[0] = '\0';
	m_szShirtNumber[0] = '\0';
	m_bIsKeeper = false;
	m_pFontAtlas = NULL;

	m_ShirtNameAndNumberColor = Color(0, 0, 0, 255);
	
	m_ShirtNumberOutlineColor = Color(0, 0, 0, 255);

	m_ShirtBackNameOutlineColor = Color(0, 0, 0, 255);
	m_nShirtBackNameVerticalOffset = 0;

	m_nShirtBackNumberVerticalOffset = 0;

	m_bHasShirtFrontNumber = false;
	m_nShirtFrontNumberHorizontalOffset = 0;
	m_nShirtFrontNumberVerticalOffset = 0;

	m_bHasShortsFrontNumber = false;
	m_ShortsFrontNumberColor = Color(0, 0, 0, 255);
	m_ShortsFrontNumberOutlineColor = Color(0, 0, 0, 255);
	m_nShortsFrontNumberHorizontalOffset = 0;
	m_nShortsFrontNumberVerticalOffset = 0;
}

bool CProceduralRegenerator::SetPlayerInfo(const char *name, int number, const Color &shirtNameAndNumberColor, const Color &shirtNumberOutlineColor, const Color &shirtBackNameOutlineColor,
											int shirtBackNameVerticalOffset, int shirtBackNumberVerticalOffset, bool hasShirtFrontNumber, int shirtFrontNumberHorizontalOffset, int shirtFrontNumberVerticalOffset, bool hasShortsFrontNumber,
											const Color &shortsFrontNumberColor, const Color &shortsFrontNumberOutlineColor, int shortsFrontNumberHorizontalOffset, int shortsFrontNumberVerticalOffset, bool isKeeper, CFontAtlas *pFontAtlas)
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

	if (shirtNumberOutlineColor != m_ShirtNumberOutlineColor)
	{
		m_ShirtNumberOutlineColor = shirtNumberOutlineColor;
		hasChanged = true;
	}

	if (shirtBackNameOutlineColor != m_ShirtBackNameOutlineColor)
	{
		m_ShirtBackNameOutlineColor = shirtBackNameOutlineColor;
		hasChanged = true;
	}

	if (shirtBackNameVerticalOffset != m_nShirtBackNameVerticalOffset)
	{
		m_nShirtBackNameVerticalOffset = shirtBackNameVerticalOffset;
		hasChanged = true;
	}

	if (shirtBackNumberVerticalOffset != m_nShirtBackNumberVerticalOffset)
	{
		m_nShirtBackNumberVerticalOffset = shirtBackNumberVerticalOffset;
		hasChanged = true;
	}

	if (hasShirtFrontNumber != m_bHasShirtFrontNumber)
	{
		m_bHasShirtFrontNumber = hasShirtFrontNumber;
		hasChanged = true;
	}

	if (shirtFrontNumberHorizontalOffset != m_nShirtFrontNumberHorizontalOffset)
	{
		m_nShirtFrontNumberHorizontalOffset = shirtFrontNumberHorizontalOffset;
		hasChanged = true;
	}

	if (shirtFrontNumberVerticalOffset != m_nShirtFrontNumberVerticalOffset)
	{
		m_nShirtFrontNumberVerticalOffset = shirtFrontNumberVerticalOffset;
		hasChanged = true;
	}

	if (hasShortsFrontNumber != m_bHasShortsFrontNumber)
	{
		m_bHasShortsFrontNumber = hasShortsFrontNumber;
		hasChanged = true;
	}

	if (shortsFrontNumberColor != m_ShortsFrontNumberColor)
	{
		m_ShortsFrontNumberColor = shortsFrontNumberColor;
		hasChanged = true;
	}

	if (shortsFrontNumberOutlineColor != m_ShortsFrontNumberOutlineColor)
	{
		m_ShortsFrontNumberOutlineColor = shortsFrontNumberOutlineColor;
		hasChanged = true;
	}

	if (shortsFrontNumberHorizontalOffset != m_nShortsFrontNumberHorizontalOffset)
	{
		m_nShortsFrontNumberHorizontalOffset = shortsFrontNumberHorizontalOffset;
		hasChanged = true;
	}

	if (shortsFrontNumberVerticalOffset != m_nShortsFrontNumberVerticalOffset)
	{
		m_nShortsFrontNumberVerticalOffset = shortsFrontNumberVerticalOffset;
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
	pixelWriter.SetPixelMemory( pVTFTexture->Format(), pVTFTexture->ImageData(), pVTFTexture->RowSizeInBytes( 0 ) );

	// Now upload the part we've been asked for
	int xmax = pSubRect->x + pSubRect->width;
	//int xmax = pVTFTexture->Width();
	int ymax = pSubRect->y + pSubRect->height;
	//int ymax = pVTFTexture->Height();
	int x, y;

	//x=250-450,y=275-325

	for (y = pSubRect->y; y < ymax; y++)
	{
		pixelWriter.Seek(pSubRect->x, y);

		for (x=pSubRect->x; x < xmax; x++)
		{
			int r=0, g=0, b=0, a=0;
			//pixelWriter.ReadPixelNoAdvance( r, g, b, a );
			pixelWriter.WritePixel(r, g, b, a);
		}
	}

	static int keeperHorizontalOffset = 600;
	static int outfieldHorizontalOffset = 350;


	// Write outline pixels
	
	// Shirt back name
	WriteText(pixelWriter, m_szShirtName, m_pFontAtlas->m_ShirtBackNamePixels, m_pFontAtlas->m_NameChars, m_pFontAtlas->m_nNamePixelsWidth, m_pFontAtlas->m_nNamePixelsHeight, m_bIsKeeper ? keeperHorizontalOffset : outfieldHorizontalOffset, m_nShirtBackNameVerticalOffset, m_ShirtBackNameOutlineColor, m_bIsKeeper, true);
	
	// Shirt back number
	WriteText(pixelWriter, m_szShirtNumber, m_pFontAtlas->m_ShirtBackNumberPixels, m_pFontAtlas->m_ShirtBackNumberChars, m_pFontAtlas->m_nShirtBackNumberPixelsWidth, m_pFontAtlas->m_nShirtBackNumberPixelsHeight, m_bIsKeeper ? keeperHorizontalOffset : outfieldHorizontalOffset, m_nShirtBackNumberVerticalOffset, m_ShirtNumberOutlineColor, m_bIsKeeper, true);	
	
	// Shirt front number
	if (m_bHasShirtFrontNumber)
		WriteText(pixelWriter, m_szShirtNumber, m_pFontAtlas->m_ShirtAndShortsFrontNumberPixels, m_pFontAtlas->m_ShirtAndShortsFrontNumberChars, m_pFontAtlas->m_nShirtAndShortsFrontNumberPixelsWidth, m_pFontAtlas->m_nShirtAndShortsFrontNumberPixelsHeight, m_nShirtFrontNumberHorizontalOffset, m_nShirtFrontNumberVerticalOffset, m_ShirtNumberOutlineColor, m_bIsKeeper, true);	
	
	// Shorts front number
	if (m_bHasShortsFrontNumber)
		WriteText(pixelWriter, m_szShirtNumber, m_pFontAtlas->m_ShirtAndShortsFrontNumberPixels, m_pFontAtlas->m_ShirtAndShortsFrontNumberChars, m_pFontAtlas->m_nShirtAndShortsFrontNumberPixelsWidth, m_pFontAtlas->m_nShirtAndShortsFrontNumberPixelsHeight, m_nShortsFrontNumberHorizontalOffset, m_nShortsFrontNumberVerticalOffset, m_ShortsFrontNumberOutlineColor, m_bIsKeeper, true);	
	

	// Write glyph pixels
	
	// Shirt back name
	WriteText(pixelWriter, m_szShirtName, m_pFontAtlas->m_ShirtBackNamePixels, m_pFontAtlas->m_NameChars, m_pFontAtlas->m_nNamePixelsWidth, m_pFontAtlas->m_nNamePixelsHeight, m_bIsKeeper ? keeperHorizontalOffset : outfieldHorizontalOffset, m_nShirtBackNameVerticalOffset, m_ShirtNameAndNumberColor, m_bIsKeeper, false);
	
	// Shirt back number
	WriteText(pixelWriter, m_szShirtNumber, m_pFontAtlas->m_ShirtBackNumberPixels, m_pFontAtlas->m_ShirtBackNumberChars, m_pFontAtlas->m_nShirtBackNumberPixelsWidth, m_pFontAtlas->m_nShirtBackNumberPixelsHeight, m_bIsKeeper ? keeperHorizontalOffset : outfieldHorizontalOffset, m_nShirtBackNumberVerticalOffset, m_ShirtNameAndNumberColor, m_bIsKeeper, false);	
	
	// Shirt front number
	if (m_bHasShirtFrontNumber)	
		WriteText(pixelWriter, m_szShirtNumber, m_pFontAtlas->m_ShirtAndShortsFrontNumberPixels, m_pFontAtlas->m_ShirtAndShortsFrontNumberChars, m_pFontAtlas->m_nShirtAndShortsFrontNumberPixelsWidth, m_pFontAtlas->m_nShirtAndShortsFrontNumberPixelsHeight, m_nShirtFrontNumberHorizontalOffset, m_nShirtFrontNumberVerticalOffset, m_ShirtNameAndNumberColor, m_bIsKeeper, false);	
	
	// Shorts front number
	if (m_bHasShortsFrontNumber)	
		WriteText(pixelWriter, m_szShirtNumber, m_pFontAtlas->m_ShirtAndShortsFrontNumberPixels, m_pFontAtlas->m_ShirtAndShortsFrontNumberChars, m_pFontAtlas->m_nShirtAndShortsFrontNumberPixelsWidth, m_pFontAtlas->m_nShirtAndShortsFrontNumberPixelsHeight, m_nShortsFrontNumberHorizontalOffset, m_nShortsFrontNumberVerticalOffset, m_ShortsFrontNumberColor, m_bIsKeeper, false);	
}

struct compColor_t
{
	int r;
	int g;
	int b;
	int a;

	compColor_t() : r(0), g(0), b(0), a(0) {}
	compColor_t(const Color &col) : r(col.r()), g(col.g()), b(col.b()), a(col.a()) {}
};

// Use Porter-Duff "over" mode for alpha compositing
void GetCompositeColor(const compColor_t &colorA, const compColor_t &colorB, compColor_t &compColor)
{
	float aR = colorA.r / 255.0f;
	float aG = colorA.g / 255.0f;
	float aB = colorA.b / 255.0f;
	float aA = colorA.a / 255.0f;

	float bR = colorB.r / 255.0f;
	float bG = colorB.g / 255.0f;
	float bB = colorB.b / 255.0f;
	float bA = colorB.a / 255.0f;

	float alpha = aA + bA * (1 - aA);

	compColor.r = (aR * aA + bR * bA * (1 - aA)) / alpha * 255;
	compColor.g = (aG * aA + bG * bA * (1 - aA)) / alpha * 255;
	compColor.b = (aB * aA + bB * bA * (1 - aA)) / alpha * 255;
	compColor.a = alpha * 255;
}

void CProceduralRegenerator::WriteText(CPixelWriter &pixelWriter, const char *text, glyphWithOutline_t **pixels, const chr_t *chars, const int &width, const int &height, int offsetX, int offsetY, const Color &color, bool isKeeper, bool isOutline)
{
	// Name and Number are upside down on the texture

	int totalWidth = 0;
	int maxHeight = 0;

	for (int i = 0; i < strlen(text); i++)
	{
		const chr_t &chr = chars[text[i]];
		totalWidth += chr.advanceX;

		if (i > 0)
			totalWidth += chr.GetKerning(text[i - 1]);

		maxHeight = max(maxHeight, chr.height + chr.offsetY);
	}

	int posX = offsetX + totalWidth / 2;
	int posY = offsetY + maxHeight / 2;

	for (int i = 0; i < strlen(text); i++)
	{
		const chr_t &chr = chars[text[i]];

		if (i > 0)
			posX -= chr.GetKerning(text[i - 1]);

		for (int y = 0; y < chr.height; y++)
		{
			for (int x = 0; x < chr.width; x++)
			{
				pixelWriter.Seek(clamp(posX - x - chr.offsetX, 0, 1023), clamp(posY - y - chr.offsetY, 0, 1023));
				int srcY = chr.y + y;
				int srcX = chr.x + x;

				compColor_t curCol;
				pixelWriter.ReadPixelNoAdvance(curCol.r, curCol.g, curCol.b, curCol.a);

				compColor_t col(color);
				col.a = isOutline ? pixels[srcY][srcX].outline : pixels[srcY][srcX].glyph;

				compColor_t compCol;
				GetCompositeColor(col, curCol, compCol);
				pixelWriter.WritePixel(compCol.r, compCol.g, compCol.b, compCol.a);
			}
		}

		posX -= chr.advanceX;
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

		int teamIndex = pTeam->GetTeamNumber() - TEAM_HOME;
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

		int teamIndex = pReplayPl->m_nTeamNumber - TEAM_HOME;
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
		{
			needsUpdate = (*pProcReg)->SetPlayerInfo(shirtName, shirtNumber, pKitInfo->m_KeeperShirtNameAndNumberColor, pKitInfo->m_KeeperShirtNumberOutlineColor,
							pKitInfo->m_KeeperShirtBackNameOutlineColor, pKitInfo->m_nKeeperShirtBackNameVerticalOffset, pKitInfo->m_nKeeperShirtBackNumberVerticalOffset, pKitInfo->m_bHasKeeperShirtFrontNumber,
							pKitInfo->m_nKeeperShirtFrontNumberHorizontalOffset, pKitInfo->m_nKeeperShirtFrontNumberVerticalOffset, pKitInfo->m_bHasKeeperShortsFrontNumber, pKitInfo->m_KeeperShortsFrontNumberColor,
							pKitInfo->m_KeeperShortsFrontNumberOutlineColor, pKitInfo->m_nKeeperShortsFrontNumberHorizontalOffset, pKitInfo->m_nKeeperShortsFrontNumberVerticalOffset, true, pKitInfo->m_pFontAtlas);
		}
		else
		{
			needsUpdate = (*pProcReg)->SetPlayerInfo(shirtName, shirtNumber, pKitInfo->m_OutfieldShirtNameAndNumberColor, pKitInfo->m_OutfieldShirtNumberOutlineColor,
							pKitInfo->m_OutfieldShirtBackNameOutlineColor, pKitInfo->m_nOutfieldShirtBackNameVerticalOffset, pKitInfo->m_nOutfieldShirtBackNumberVerticalOffset, pKitInfo->m_bHasOutfieldShirtFrontNumber,
							pKitInfo->m_nOutfieldShirtFrontNumberHorizontalOffset, pKitInfo->m_nOutfieldShirtFrontNumberVerticalOffset, pKitInfo->m_bHasOutfieldShortsFrontNumber, pKitInfo->m_OutfieldShortsFrontNumberColor,
							pKitInfo->m_OutfieldShortsFrontNumberOutlineColor, pKitInfo->m_nOutfieldShortsFrontNumberHorizontalOffset, pKitInfo->m_nOutfieldShortsFrontNumberVerticalOffset, false, pKitInfo->m_pFontAtlas);
		}
				
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

	if (Q_stricmp(m_szTextureType, "hometeamcrest") == 0 && GetGlobalTeam(TEAM_HOME)->HasCrest())
		Q_snprintf(texture, sizeof(texture), "%s/%s/teamcrest", TEAMKITS_PATH, GetGlobalTeam(TEAM_HOME)->GetFolderName());
	else if (Q_stricmp(m_szTextureType, "awayteamcrest") == 0 && GetGlobalTeam(TEAM_AWAY)->HasCrest())
		Q_snprintf(texture, sizeof(texture), "%s/%s/teamcrest", TEAMKITS_PATH, GetGlobalTeam(TEAM_AWAY)->GetFolderName());
	else
		Q_snprintf(texture, sizeof(texture), "%s", m_pDefaultTexture->GetName());

	m_pNewTexture = materials->FindTexture(texture, NULL, true);
		
	m_pBaseTextureVar->SetTextureValue(m_pNewTexture);

	GetMaterial()->RecomputeStateSnapshots();
}

EXPOSE_INTERFACE( CTextureProxy, IMaterialProxy, "Texture" IMATERIAL_PROXY_INTERFACE_VERSION );
