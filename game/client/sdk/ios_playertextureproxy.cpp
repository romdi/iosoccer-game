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
	char m_szPlayerName[MAX_PLAYER_NAME_LENGTH];
	char m_szPlayerNumber[4];
	Color m_NameColor;
	int m_nNameOffset;
	Color m_NumberColor;
	int m_nNumberOffset;
	bool m_bIsKeeper;
	CFontAtlas *m_pFontAtlas;
};

CProceduralRegenerator::CProceduralRegenerator()
{
	m_szPlayerName[0] = '\0';
	m_szPlayerNumber[0] = '\0';
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

	if (Q_strcmp(name, m_szPlayerName))
	{
		Q_strncpy(m_szPlayerName, name, sizeof(m_szPlayerName));
		hasChanged = true;
	}

	if (number != atoi(m_szPlayerNumber))
	{
		Q_snprintf(m_szPlayerNumber, sizeof(m_szPlayerNumber), "%d", number);
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

	WriteText(pixelWriter, m_szPlayerName, m_pFontAtlas->m_NamePixels, m_pFontAtlas->m_NameChars, m_pFontAtlas->m_nNamePixelsWidth, m_pFontAtlas->m_nNamePixelsHeight, m_nNameOffset, m_NameColor, m_bIsKeeper);
	WriteText(pixelWriter, m_szPlayerNumber, m_pFontAtlas->m_NumberPixels, m_pFontAtlas->m_NumberChars, m_pFontAtlas->m_nNumberPixelsWidth, m_pFontAtlas->m_nNumberPixelsHeight, m_nNumberOffset, m_NumberColor, m_bIsKeeper);	
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

#define PLAYERTEXTURE_PATH "models/player/teams"

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
	IMaterial *m_pMaterial;
};

CPlayerTextureProxy::CPlayerTextureProxy()
{
	m_pBaseTextureVar = NULL;
	m_pDetailTextureVar = NULL;
	m_pTexture = NULL;

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 11; j++)
			m_pTextureRegen[i][j] = NULL;
}

CPlayerTextureProxy::~CPlayerTextureProxy()
{
	// Do nothing
}
void CPlayerTextureProxy::Release()
{
	m_pBaseTextureVar = NULL;
	m_pTexture = NULL;
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 11; j++)
			m_pTextureRegen[i][j] = NULL;
	delete this;
}

bool CPlayerTextureProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	#ifdef ALLPROXIESFAIL
	return false;
	#endif

	m_pMaterial = pMaterial;

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
		//m_pTextureRegen = new CProceduralRegenerator();
	}

	return true;
}

void CPlayerTextureProxy::OnBind( C_BaseEntity *pEnt )
{
	// Bail if no base variable
	if ( !m_pBaseTextureVar )
		return;

	const char *teamFolder;
	const char *kitFolder;
	int shirtNumber;

	C_SDKPlayer *pPl = dynamic_cast<C_SDKPlayer *>(pEnt);
	if (pPl)
	{
		C_Team *pTeam = GetGlobalTeam(g_PR->GetTeam(pEnt->index));
		teamFolder = pTeam->GetFolderName();
		kitFolder = pTeam->GetKitFolderName();
		shirtNumber = g_PR->GetShirtNumber(pEnt->index);
		const char *name = g_PR->GetPlayerName(pEnt->index);
		int teamNumber = pTeam->GetTeamNumber();
		int teamIndex = teamNumber - TEAM_A;
		int posIndex = g_PR->GetTeamPosIndex(pEnt->index);

		if (!Q_strcmp(m_szTextureType, "shirt") || !Q_strcmp(m_szTextureType, "keepershirt"))
		{
			ITexture *pDetailTexture = materials->FindTexture(VarArgs("models/player/default/detail_%d_%d", teamIndex, posIndex), NULL, true);

			if (!m_pTextureRegen[teamIndex][posIndex])
				m_pTextureRegen[teamIndex][posIndex] = new CProceduralRegenerator();

			pDetailTexture->SetTextureRegenerator(m_pTextureRegen[teamIndex][posIndex]);
			m_pDetailTextureVar->SetTextureValue(pDetailTexture);

			bool needsUpdate;

			if (g_PR->GetTeamPosType(pEnt->index) == POS_GK)
				needsUpdate = m_pTextureRegen[teamIndex][posIndex]->SetPlayerInfo(name, shirtNumber, pTeam->GetKeeperShirtNameColor(), pTeam->GetKeeperShirtNameOffset(), pTeam->GetKeeperShirtNumberColor(), pTeam->GetKeeperShirtNumberOffset(), true, pTeam->GetFontAtlas());
			else
				needsUpdate = m_pTextureRegen[teamIndex][posIndex]->SetPlayerInfo(name, shirtNumber, pTeam->GetOutfieldShirtNameColor(), pTeam->GetOutfieldShirtNameOffset(), pTeam->GetOutfieldShirtNumberColor(), pTeam->GetOutfieldShirtNumberOffset(), false, pTeam->GetFontAtlas());
				
			if (needsUpdate)
				pDetailTexture->Download();
		}
	}
	else
	{
		C_ReplayPlayer *pReplayPlayer = dynamic_cast<C_ReplayPlayer *>(pEnt);
		if (pReplayPlayer)
		{
			teamFolder = GetGlobalTeam(pReplayPlayer->m_nTeamNumber)->GetFolderName();
			kitFolder = GetGlobalTeam(pReplayPlayer->m_nTeamNumber)->GetKitFolderName();
			shirtNumber = pReplayPlayer->m_nShirtNumber;
		}
		else
		{
			C_BaseAnimatingOverlay *pPlayerModelPreview = dynamic_cast<C_BaseAnimatingOverlay *>(pEnt);
			if (pPlayerModelPreview)
			{
				CAppearanceSettingPanel *pPanel = (CAppearanceSettingPanel *)iosOptionsMenu->GetPanel()->GetSettingPanel(SETTING_PANEL_APPEARANCE);
				pPanel->GetPlayerTeamInfo(&teamFolder, &kitFolder);
				shirtNumber = pPanel->GetPlayerOutfieldShirtNumber();
			}
			else
				return;
		}
	}

	char texture[128];

	if (Q_stricmp(m_szTextureType, "shirt") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/%s/outfield", PLAYERTEXTURE_PATH, teamFolder, kitFolder);
	else if (Q_stricmp(m_szTextureType, "keepershirt") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/%s/keeper", PLAYERTEXTURE_PATH, teamFolder, kitFolder);
	else if (Q_stricmp(m_szTextureType, "socks") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/%s/socks", PLAYERTEXTURE_PATH, teamFolder, kitFolder);
	else if (Q_stricmp(m_szTextureType, "gksocks") == 0)
		Q_snprintf(texture, sizeof(texture), "%s/%s/%s/gksocks", PLAYERTEXTURE_PATH, teamFolder, kitFolder);
	else if (Q_stricmp(m_szTextureType, "skin") == 0)
		Q_snprintf(texture, sizeof(texture), "%s", m_pTexture->GetName());
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
		Q_snprintf(texture, sizeof(texture), "%s/%s/teamcrest", PLAYERTEXTURE_PATH, GetGlobalTeam(TEAM_A)->GetFolderName());
	else if (Q_stricmp(m_szTextureType, "awayteamcrest") == 0 && GetGlobalTeam(TEAM_B)->HasCrest())
		Q_snprintf(texture, sizeof(texture), "%s/%s/teamcrest", PLAYERTEXTURE_PATH, GetGlobalTeam(TEAM_B)->GetFolderName());
	else
		Q_snprintf(texture, sizeof(texture), "%s", m_pDefaultTexture->GetName());

	m_pNewTexture = materials->FindTexture(texture, NULL, true);
		
	m_pBaseTextureVar->SetTextureValue(m_pNewTexture);

	GetMaterial()->RecomputeStateSnapshots();
}

EXPOSE_INTERFACE( CTextureProxy, IMaterialProxy, "Texture" IMATERIAL_PROXY_INTERFACE_VERSION );
