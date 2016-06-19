#include "cbase.h"
#include <KeyValues.h>
#include <tier0/mem.h>
#include "filesystem.h"
#include "utldict.h"
#include "ammodef.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "ios_teamkit_parse.h"
#include "sdk_gamerules.h"
#include "curl/curl.h"
#include "Filesystem.h"
#include "utlbuffer.h"
#include "checksum_md5.h"
#include "threadtools.h"
#include "ios_color_classifier.h"

#ifdef CLIENT_DLL

#include "vgui_controls/messagebox.h"
#include "clientmode_shared.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL

void CC_ReloadTeamKits(const CCommand &args)
{
	CTeamInfo::ParseTeamKits();
	// For some reason we have to do this twice to prevent kit swapping
	CTeamInfo::ParseTeamKits();
}

static ConCommand reloadteamkits("reloadteamkits", CC_ReloadTeamKits);

#endif

float CTeamInfo::m_flLastUpdateTime = 0;

CFontAtlas *CTeamKitInfo::m_pDefaultFontAtlas = NULL;

CTeamKitInfo::CTeamKitInfo()
{
	m_szName[0] = 0;
	m_szAuthor[0] = 0;
	m_szFolderName[0] = 0;
	m_PrimaryColor = Color(0, 255, 0, 255);
	m_HudPrimaryColor = Color(0, 255, 0, 255);
	m_HudPrimaryColorClass = COLOR_CLASS_WHITE;
	m_SecondaryColor = Color(0, 255, 0, 255);
	m_HudSecondaryColor = Color(0, 255, 0, 255);
	m_HudSecondaryColorClass = COLOR_CLASS_WHITE;


	m_bOutfieldHasCollar = false;

	m_OutfieldShirtNameFillColor = Color(0, 255, 0, 255);
	m_OutfieldShirtNameOutlineColor = Color(0, 255, 0, 255);
	m_nOutfieldShirtNameVerticalOffset = 0;

	m_OutfieldShirtBackNumberFillColor = Color(0, 255, 0, 255);
	m_OutfieldShirtBackNumberOutlineColor = Color(0, 255, 0, 255);
	m_nOutfieldShirtBackNumberVerticalOffset = 0;

	m_OutfieldShortsNumberFillColor = Color(0, 255, 0, 255);
	m_OutfieldShortsNumberOutlineColor = Color(0, 255, 0, 255);
	m_nOutfieldShortsNumberHorizontalOffset = 0;
	m_nOutfieldShortsNumberVerticalOffset = 0;

	m_bHasOutfieldShirtFrontNumber = false;
	m_OutfieldShirtFrontNumberFillColor = Color(0, 255, 0, 255);
	m_OutfieldShirtFrontNumberOutlineColor = Color(0, 255, 0, 255);
	m_nOutfieldShirtFrontNumberHorizontalOffset = 0;
	m_nOutfieldShirtFrontNumberVerticalOffset = 0;


	m_bKeeperHasCollar = false;

	m_KeeperShirtNameFillColor = Color(0, 255, 0, 255);
	m_KeeperShirtNameOutlineColor = Color(0, 255, 0, 255);
	m_nKeeperShirtNameVerticalOffset = 0;

	m_KeeperShirtBackNumberFillColor = Color(0, 255, 0, 255);
	m_KeeperShirtBackNumberOutlineColor = Color(0, 255, 0, 255);
	m_nKeeperShirtBackNumberVerticalOffset = 0;

	m_KeeperShortsNumberFillColor = Color(0, 255, 0, 255);
	m_KeeperShortsNumberOutlineColor = Color(0, 255, 0, 255);
	m_nKeeperShortsNumberHorizontalOffset = 0;
	m_nKeeperShortsNumberVerticalOffset = 0;

	m_bHasKeeperShirtFrontNumber = false;
	m_KeeperShirtFrontNumberFillColor = Color(0, 255, 0, 255);
	m_KeeperShirtFrontNumberOutlineColor = Color(0, 255, 0, 255);
	m_nKeeperShirtFrontNumberHorizontalOffset = 0;
	m_nKeeperShirtFrontNumberVerticalOffset = 0;


	m_pFontAtlas = NULL;
	m_pTeamInfo = NULL;
}

CUtlVector<CTeamInfo *> CTeamInfo::m_TeamInfo;

CTeamInfo::CTeamInfo()
{
	m_bIsClub = false;
	m_bIsReal = false;
	m_bHasCrest = false;
	m_szCode[0] = 0;
	m_szShortName[0] = 0;
	m_szFullName[0] = 0;
	m_szFolderName[0] = 0;
}

CTeamInfo::~CTeamInfo()
{
	m_TeamKitInfo.PurgeAndDeleteElements();
}

struct FileInfo_t
{
	char name[128];
	char path[128];
	bool isDirectory;
};

void FindFiles(const char *path, CUtlVector<FileInfo_t> &fileInfos)
{
	char filefilter[128];
	Q_snprintf(filefilter, sizeof(filefilter), "%s/*.*", path);
	FileFindHandle_t findHandle;
	const char *filename = filesystem->FindFirstEx(filefilter, "MOD", &findHandle);

	while (filename)
	{
		if (Q_strcmp(filename, ".") && Q_strcmp(filename, ".."))
		{
			FileInfo_t fileInfo;
			Q_strncpy(fileInfo.name, filename, sizeof(fileInfo.name));
			Q_snprintf(fileInfo.path, sizeof(fileInfo.path), "%s/%s", path, filename);
			fileInfo.isDirectory = filesystem->IsDirectory(fileInfo.path, "MOD");
			fileInfos.AddToTail(fileInfo);
		}

		filename = filesystem->FindNext(findHandle);
	}

	filesystem->FindClose(findHandle);
}

#define TEAMKITS_PATH "materials/models/player/teamkits"

#define DEFAULT_FONTATLAS_FOLDER "materials/models/player/default"

CFontAtlas::CFontAtlas(const char *folderPath, bool hasShirtAndShortsNumberAtlas)
{
	m_ShirtNamePixels = ParseInfo(folderPath, "shirt_back_name", m_NameChars, m_nNamePixelsWidth, m_nNamePixelsHeight);
	m_ShirtBackNumberPixels = ParseInfo(folderPath, "shirt_back_number", m_ShirtBackNumberChars, m_nShirtBackNumberPixelsWidth, m_nShirtBackNumberPixelsHeight);

	if (hasShirtAndShortsNumberAtlas)
		m_ShirtAndShortsNumberPixels = ParseInfo(folderPath, "shirt_and_shorts_front_number", m_ShirtAndShortsNumberChars, m_nShirtAndShortsNumberPixelsWidth, m_nShirtAndShortsNumberPixelsHeight);
	else
		m_ShirtAndShortsNumberPixels = NULL;
}

glyphWithOutline_t **CFontAtlas::ParseInfo(const char *folderPath, const char *type, CUtlVector<chr_t> &chars, int &width, int &height)
{
	char path[128];

	Q_snprintf(path, sizeof(path), "%s/%s_atlas.tga", folderPath, type);

	if (!filesystem->FileExists(path, "MOD"))
		return NULL;

    FileHandle_t fh = filesystem->Open(path, "rb", "MOD");
	int file_len = filesystem->Size(fh);

    unsigned char* fulldata = new unsigned char[file_len];
    filesystem->Read(fulldata, file_len, fh);
	const int pixelStart = 18;

	width = *(short*)&fulldata[12];
    height = *(short*)&fulldata[14];

	glyphWithOutline_t **pixels = new glyphWithOutline_t *[height];
	for (int i = 0; i < height; i++)
		pixels[i] = new glyphWithOutline_t[width];

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
			int pos = pixelStart + y * width * 4 + x * 4;
			Assert(fulldata[pos + 0] == fulldata[pos + 1] && fulldata[pos + 0] == fulldata[pos + 2]);
			pixels[y][x].glyph = fulldata[pos + 0];
			pixels[y][x].outline = fulldata[pos + 3];
        }
    }

	delete[] fulldata;
    filesystem->Close(fh);

	Q_snprintf(path, sizeof(path), "%s/%s_atlas.txt", folderPath, type);
    fh = filesystem->Open(path, "rb", "MOD");

	file_len = filesystem->Size(fh);
	char *fulltext = new char[file_len];
    filesystem->Read(fulltext, file_len, fh);

	char *pch;
	pch = strtok(fulltext, "\n");

	while (pch != NULL)
	{
		if (!Q_strncmp(pch, "char ", 5))
		{
			chr_t chr;
			chr.glyph = atoi(strstr(pch, "id=") + 3);
			chr.x = atoi(strstr(pch, "x=") + 2);
			chr.y = atoi(strstr(pch, "y=") + 2);
			chr.width = atoi(strstr(pch, "width=") + 6);
			chr.height = atoi(strstr(pch, "height=") + 7);
			chr.offsetX = atoi(strstr(pch, "xoffset=") + 8);
			chr.offsetY = atoi(strstr(pch, "yoffset=") + 8);
			chr.advanceX = atoi(strstr(pch, "xadvance=") + 9);
			chars.AddToTail(chr);
		}
		else if (!Q_strncmp(pch, "kerning ", 8))
		{
			int first = atoi(strstr(pch, "first=") + 6);
			int second = atoi(strstr(pch, "second=") + 7);
			int amount = atoi(strstr(pch, "amount=") + 7);

			FindCharById(chars, second)->kernings.AddToTail(kerning_t(first, amount));
		}

		pch = strtok(NULL, "\n");
	}

	delete[] fulltext;
    filesystem->Close(fh);

	return pixels;
}

chr_t *CFontAtlas::FindCharById(CUtlVector<chr_t> &chars, int id)
{
	for (int i = 0; i < chars.Count(); i++)
	{
		if (chars[i].glyph == id)
			return &chars[i];
	}

	return NULL;
}

void CTeamInfo::ParseTeamKits()
{
#ifdef CLIENT_DLL
	CTeamKitInfo::m_pDefaultFontAtlas = new CFontAtlas(DEFAULT_FONTATLAS_FOLDER, true);
#endif

	CTeamInfo::m_TeamInfo.PurgeAndDeleteElements();

	CUtlVector<FileInfo_t> teamFolders;
	FindFiles(TEAMKITS_PATH, teamFolders);

	for (int i = 0; i < teamFolders.Count(); i++)
	{
		if (!teamFolders[i].isDirectory)
			continue;

		CTeamInfo *pTeamInfo = new CTeamInfo();
		Q_strncpy(pTeamInfo->m_szFolderName, teamFolders[i].name, sizeof(pTeamInfo->m_szFolderName));

		CUtlVector<FileInfo_t> teamFolderFiles;
		FindFiles(teamFolders[i].path, teamFolderFiles);

 		for (int j = 0; j < teamFolderFiles.Count(); j++)
		{
			if (!teamFolderFiles[j].isDirectory)
			{
				if (!Q_strcmp(teamFolderFiles[j].name, "teamcrest.vtf"))
				{
					pTeamInfo->m_bHasCrest = true;
				}
				else if (!Q_strcmp(teamFolderFiles[j].name, "teamdata.txt"))
				{
					KeyValues *pKV = new KeyValues("TeamData");
					pKV->LoadFromFile(filesystem, teamFolderFiles[j].path, "MOD");

					pTeamInfo->m_bIsClub = pKV->GetInt("IsClub", 0) != 0;
					pTeamInfo->m_bIsReal = pKV->GetInt("IsReal", 0) != 0;
					Q_strncpy(pTeamInfo->m_szCode, pKV->GetString("Code", "???"), sizeof(pTeamInfo->m_szCode));
					Q_strncpy(pTeamInfo->m_szShortName, pKV->GetString("ShortName", "???"), sizeof(pTeamInfo->m_szShortName));
					Q_strncpy(pTeamInfo->m_szFullName, pKV->GetString("FullName", "???"), sizeof(pTeamInfo->m_szFullName));

					pKV->deleteThis();
				}
			}
			else
			{
				CTeamKitInfo *pKitInfo = new CTeamKitInfo();
				pKitInfo->m_pTeamInfo = pTeamInfo;
				Q_strncpy(pKitInfo->m_szFolderName, teamFolderFiles[j].name, sizeof(pKitInfo->m_szFolderName));

				CUtlVector<FileInfo_t> kitFiles;
				FindFiles(teamFolderFiles[j].path, kitFiles);
				
				bool hasBackNameAtlasTga = false;
				bool hasBackNameAtlasTxt = false;
				bool hasBackNumberAtlasTga = false;
				bool hasBackNumberAtlasTxt = false;
				bool hasShirtAndShortsNumberAtlasTga = false;
				bool hasShirtAndShortsNumberAtlasTxt = false;

				for (int k = 0; k < kitFiles.Count(); k++)
				{
					if (kitFiles[k].isDirectory)
						continue;

					if (!Q_strcmp(kitFiles[k].name, "kitdata.txt"))
					{
						KeyValues *pKV = new KeyValues("KitData");
						pKV->LoadFromFile(filesystem, kitFiles[k].path, "MOD");

						Q_strncpy(pKitInfo->m_szName, pKV->GetString("Name", "???"), sizeof(pKitInfo->m_szName));
						Q_strncpy(pKitInfo->m_szAuthor, pKV->GetString("Author", "???"), sizeof(pKitInfo->m_szAuthor));

						Color c = pKV->GetColor("PrimaryColor");
						pKitInfo->m_PrimaryColor = Color(c.r(), c.g(), c.b(), 255);

						c = pKV->GetColor("SecondaryColor");
						pKitInfo->m_SecondaryColor = Color(c.r(), c.g(), c.b(), 255);

						for (KeyValues *pTypeKey = pKV->GetFirstTrueSubKey(); pTypeKey; pTypeKey = pTypeKey->GetNextTrueSubKey())
						{
							if (!Q_stricmp(pTypeKey->GetName(), "Outfield"))
							{
								pKitInfo->m_bHasOutfieldShirtFrontNumber = false;
								pKitInfo->m_bOutfieldHasCollar = pTypeKey->GetInt("HasCollar", 0) != 0;

								for (KeyValues *pPosKey = pTypeKey->GetFirstTrueSubKey(); pPosKey; pPosKey = pPosKey->GetNextTrueSubKey())
								{
									if (!Q_stricmp(pPosKey->GetName(), "ShirtName"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_OutfieldShirtNameFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_OutfieldShirtNameOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nOutfieldShirtNameVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShirtBackNumber"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_OutfieldShirtBackNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_OutfieldShirtBackNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nOutfieldShirtBackNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShortsNumber"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_OutfieldShortsNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_OutfieldShortsNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nOutfieldShortsNumberHorizontalOffset = pPosKey->GetInt("HorizontalOffset");
										pKitInfo->m_nOutfieldShortsNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShirtFrontNumber"))
									{
										pKitInfo->m_bHasOutfieldShirtFrontNumber = true;
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_OutfieldShirtFrontNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_OutfieldShirtFrontNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nOutfieldShirtFrontNumberHorizontalOffset = pPosKey->GetInt("HorizontalOffset");
										pKitInfo->m_nOutfieldShirtFrontNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
								}
							}
							else if (!Q_stricmp(pTypeKey->GetName(), "Keeper"))
							{
								pKitInfo->m_bHasKeeperShirtFrontNumber = false;
								pKitInfo->m_bKeeperHasCollar = pTypeKey->GetInt("HasCollar", 0) != 0;

								for (KeyValues *pPosKey = pTypeKey->GetFirstTrueSubKey(); pPosKey; pPosKey = pPosKey->GetNextTrueSubKey())
								{
									if (!Q_stricmp(pPosKey->GetName(), "ShirtName"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_KeeperShirtNameFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_KeeperShirtNameOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nKeeperShirtNameVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShirtBackNumber"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_KeeperShirtBackNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_KeeperShirtBackNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nKeeperShirtBackNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShortsNumber"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_KeeperShortsNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_KeeperShortsNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nKeeperShortsNumberHorizontalOffset = pPosKey->GetInt("HorizontalOffset");
										pKitInfo->m_nKeeperShortsNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShirtFrontNumber"))
									{
										pKitInfo->m_bHasKeeperShirtFrontNumber = true;
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_KeeperShirtFrontNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_KeeperShirtFrontNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nKeeperShirtFrontNumberHorizontalOffset = pPosKey->GetInt("HorizontalOffset");
										pKitInfo->m_nKeeperShirtFrontNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
								}
							}
						}

						pKitInfo->m_HudPrimaryColorClass = CColorClassifier::Classify(pKitInfo->m_PrimaryColor);
						pKitInfo->m_HudPrimaryColor = g_HudColors[pKitInfo->m_HudPrimaryColorClass];

						pKitInfo->m_HudSecondaryColorClass = CColorClassifier::Classify(pKitInfo->m_SecondaryColor);
						pKitInfo->m_HudSecondaryColor = g_HudColors[pKitInfo->m_HudSecondaryColorClass];

						pKV->deleteThis();
					}
					else if (!Q_strcmp(kitFiles[k].name, "shirt_back_name_atlas.tga"))
						hasBackNameAtlasTga = true;
					else if (!Q_strcmp(kitFiles[k].name, "shirt_back_name_atlas.txt"))
						hasBackNameAtlasTxt = true;
					else if (!Q_strcmp(kitFiles[k].name, "shirt_back_number_atlas.tga"))
						hasBackNumberAtlasTga = true;
					else if (!Q_strcmp(kitFiles[k].name, "shirt_back_number_atlas.txt"))
						hasBackNumberAtlasTxt = true;
					else if (!Q_strcmp(kitFiles[k].name, "shirt_and_shorts_front_number_atlas.tga"))
						hasShirtAndShortsNumberAtlasTga = true;
					else if (!Q_strcmp(kitFiles[k].name, "shirt_and_shorts_front_number_atlas.txt"))
						hasShirtAndShortsNumberAtlasTxt = true;
				}
				
#ifdef CLIENT_DLL
				if (hasBackNameAtlasTga && hasBackNameAtlasTxt && hasBackNumberAtlasTga && hasBackNumberAtlasTxt)
				{
					pKitInfo->m_pFontAtlas = new CFontAtlas(teamFolderFiles[j].path, hasShirtAndShortsNumberAtlasTga && hasShirtAndShortsNumberAtlasTxt);
				}
				else
					pKitInfo->m_pFontAtlas = CTeamKitInfo::m_pDefaultFontAtlas;
#endif
				
				pTeamInfo->m_TeamKitInfo.AddToTail(pKitInfo);
			}
		}

		CTeamInfo::m_TeamInfo.AddToTail(pTeamInfo);
	}

	m_flLastUpdateTime = gpGlobals->curtime;
}

void CTeamInfo::GetNonClashingTeamKits(char *homeTeam, char *awayTeam, bool clubTeams, bool nationalTeams, bool realTeams, bool fictitiousTeams)
{
	int teamCount = m_TeamInfo.Count();
	int attemptCount = 0;
	int homeTeamIndex;
	int awayTeamIndex;

	do
	{
		attemptCount += 1;
		homeTeamIndex = g_IOSRand.RandomInt(0, teamCount - 1);
		awayTeamIndex = g_IOSRand.RandomInt(0, teamCount - 1);

		if (homeTeamIndex == awayTeamIndex)
			continue;

		if (m_TeamInfo[homeTeamIndex]->m_bIsClub && !clubTeams || !m_TeamInfo[homeTeamIndex]->m_bIsClub && !nationalTeams)
			continue;

		if (m_TeamInfo[homeTeamIndex]->m_bIsReal && !realTeams || !m_TeamInfo[homeTeamIndex]->m_bIsReal && !fictitiousTeams)
			continue;

		if (m_TeamInfo[awayTeamIndex]->m_bIsClub && !clubTeams || !m_TeamInfo[awayTeamIndex]->m_bIsClub && !nationalTeams)
			continue;

		if (m_TeamInfo[awayTeamIndex]->m_bIsReal && !realTeams || !m_TeamInfo[awayTeamIndex]->m_bIsReal && !fictitiousTeams)
			continue;

		if (m_TeamInfo[homeTeamIndex]->m_bIsClub != m_TeamInfo[awayTeamIndex]->m_bIsClub ||
			m_TeamInfo[homeTeamIndex]->m_bIsReal != m_TeamInfo[awayTeamIndex]->m_bIsReal)
			continue;

		//Msg("color distance: %f\n", ColorDistance(m_TeamKitInfoDatabase[homeTeamIndex]->m_PrimaryKitColor, m_TeamKitInfoDatabase[awayTeamIndex]->m_PrimaryKitColor));
		Q_snprintf(homeTeam, MAX_KITNAME_LENGTH, "%s/%s", m_TeamInfo[homeTeamIndex]->m_szFolderName, m_TeamInfo[homeTeamIndex]->m_TeamKitInfo[0]->m_szFolderName);
		Q_snprintf(awayTeam, MAX_KITNAME_LENGTH, "%s/%s", m_TeamInfo[awayTeamIndex]->m_szFolderName, m_TeamInfo[awayTeamIndex]->m_TeamKitInfo[0]->m_szFolderName);
		break;

	} while (attemptCount < 1000);
}

CTeamKitInfo *CTeamInfo::FindTeamByKitName(const char *name)
{
	char fullName[MAX_KITNAME_LENGTH];
	Q_strncpy(fullName, name, sizeof(fullName));
	char *teamName = strtok(fullName, "/");
	char *kitName = strtok(NULL, "/");

	for (int i = 0; i < m_TeamInfo.Count(); i++)
	{
		if (Q_strcmp(m_TeamInfo[i]->m_szFolderName, teamName))
			continue;

		for (int j = 0; j < m_TeamInfo[i]->m_TeamKitInfo.Count(); j++)
		{
			if (Q_strcmp(m_TeamInfo[i]->m_TeamKitInfo[j]->m_szFolderName, kitName))
				continue;

			return m_TeamInfo[i]->m_TeamKitInfo[j];
		}
	}

	return NULL;
}

CTeamKitInfo *CTeamInfo::FindTeamByShortName(const char *name)
{
	for (int i = 0; i < m_TeamInfo.Count(); i++)
	{
		if (!Q_strcmp(m_TeamInfo[i]->m_szShortName, name))
		{
			return m_TeamInfo[i]->m_TeamKitInfo[0];
		}
	}

	return NULL;
}

CTeamKitInfo *CTeamInfo::FindTeamByCode(const char *code)
{
	for (int i = 0; i < m_TeamInfo.Count(); i++)
	{
		if (!Q_strcmp(m_TeamInfo[i]->m_szCode, code))
		{
			return m_TeamInfo[i]->m_TeamKitInfo[0];
		}
	}

	return NULL;
}


CUtlVector<CShoeInfo *> CShoeInfo::m_ShoeInfo;
float CShoeInfo::m_flLastUpdateTime = 0;

void CShoeInfo::ParseShoes()
{
	CShoeInfo::m_ShoeInfo.PurgeAndDeleteElements();

	CUtlVector<FileInfo_t> shoeFolders;
	FindFiles("materials/models/player/shoes/", shoeFolders);

	for (int i = 0; i < shoeFolders.Count(); i++)
	{
		if (!shoeFolders[i].isDirectory)
			continue;

		CShoeInfo *pShoeInfo = new CShoeInfo();
		Q_strncpy(pShoeInfo->m_szFolderName, shoeFolders[i].name, sizeof(pShoeInfo->m_szFolderName));

		CUtlVector<FileInfo_t> shoeFolderFiles;
		FindFiles(shoeFolders[i].path, shoeFolderFiles);

		for (int j = 0; j < shoeFolderFiles.Count(); j++)
		{
			if (shoeFolderFiles[j].isDirectory)
				continue;

			if (!Q_strcmp(shoeFolderFiles[j].name, "shoedata.txt"))
			{
				KeyValues *pKV = new KeyValues("ShoeData");
				pKV->LoadFromFile(filesystem, shoeFolderFiles[j].path, "MOD");

				Q_strncpy(pShoeInfo->m_szName, pKV->GetString("Name", "???"), sizeof(pShoeInfo->m_szName));
				Q_strncpy(pShoeInfo->m_szAuthor, pKV->GetString("Author", "???"), sizeof(pShoeInfo->m_szAuthor));

				pKV->deleteThis();
			}
		}

		CShoeInfo::m_ShoeInfo.AddToTail(pShoeInfo);
	}

	m_flLastUpdateTime = gpGlobals->curtime;
}


CUtlVector<CKeeperGloveInfo *> CKeeperGloveInfo::m_KeeperGloveInfo;
float CKeeperGloveInfo::m_flLastUpdateTime = 0;

void CKeeperGloveInfo::ParseKeeperGloves()
{
	CKeeperGloveInfo::m_KeeperGloveInfo.PurgeAndDeleteElements();

	CUtlVector<FileInfo_t> keeperGloveFolders;
	FindFiles("materials/models/player/keepergloves/", keeperGloveFolders);

	for (int i = 0; i < keeperGloveFolders.Count(); i++)
	{
		if (!keeperGloveFolders[i].isDirectory)
			continue;

		CKeeperGloveInfo *pKeeperGloveInfo = new CKeeperGloveInfo();
		Q_strncpy(pKeeperGloveInfo->m_szFolderName, keeperGloveFolders[i].name, sizeof(pKeeperGloveInfo->m_szFolderName));

		CUtlVector<FileInfo_t> keeperGloveFolderFiles;
		FindFiles(keeperGloveFolders[i].path, keeperGloveFolderFiles);

		for (int j = 0; j < keeperGloveFolderFiles.Count(); j++)
		{
			if (keeperGloveFolderFiles[j].isDirectory)
				continue;

			if (!Q_strcmp(keeperGloveFolderFiles[j].name, "keeperglovedata.txt"))
			{
				KeyValues *pKV = new KeyValues("KeeperGloveData");
				pKV->LoadFromFile(filesystem, keeperGloveFolderFiles[j].path, "MOD");

				Q_strncpy(pKeeperGloveInfo->m_szName, pKV->GetString("Name", "???"), sizeof(pKeeperGloveInfo->m_szName));
				Q_strncpy(pKeeperGloveInfo->m_szAuthor, pKV->GetString("Author", "???"), sizeof(pKeeperGloveInfo->m_szAuthor));

				pKV->deleteThis();
			}
		}

		CKeeperGloveInfo::m_KeeperGloveInfo.AddToTail(pKeeperGloveInfo);
	}

	m_flLastUpdateTime = gpGlobals->curtime;
}


CUtlVector<CBallInfo *> CBallInfo::m_BallInfo;
float CBallInfo::m_flLastUpdateTime = 0;

void CBallInfo::ParseBallSkins()
{
	CBallInfo::m_BallInfo.PurgeAndDeleteElements();

	CUtlVector<FileInfo_t> ballFolders;
	FindFiles("materials/models/ball/skins/", ballFolders);

	for (int i = 0; i < ballFolders.Count(); i++)
	{
		if (!ballFolders[i].isDirectory)
			continue;

		CBallInfo *pBallInfo = new CBallInfo();
		Q_strncpy(pBallInfo->m_szFolderName, ballFolders[i].name, sizeof(pBallInfo->m_szFolderName));

		CUtlVector<FileInfo_t> ballFolderFiles;
		FindFiles(ballFolders[i].path, ballFolderFiles);

 		for (int j = 0; j < ballFolderFiles.Count(); j++)
		{
			if (ballFolderFiles[j].isDirectory)
				continue;

			if (!Q_strcmp(ballFolderFiles[j].name, "balldata.txt"))
			{
				KeyValues *pKV = new KeyValues("BallData");
				pKV->LoadFromFile(filesystem, ballFolderFiles[j].path, "MOD");

				Q_strncpy(pBallInfo->m_szName, pKV->GetString("Name", "???"), sizeof(pBallInfo->m_szName));
				Q_strncpy(pBallInfo->m_szAuthor, pKV->GetString("Author", "???"), sizeof(pBallInfo->m_szAuthor));

				pKV->deleteThis();
			}
		}

		CBallInfo::m_BallInfo.AddToTail(pBallInfo);
	}

	m_flLastUpdateTime = gpGlobals->curtime;
}

CUtlVector<CPitchInfo *> CPitchInfo::m_PitchInfo;

void CPitchInfo::ParsePitchTextures()
{
	CPitchInfo::m_PitchInfo.PurgeAndDeleteElements();

	CUtlVector<FileInfo_t> pitchFolders;
	FindFiles("materials/pitch/textures", pitchFolders);

	for (int i = 0; i < pitchFolders.Count(); i++)
	{
		if (!pitchFolders[i].isDirectory)
			continue;

		CPitchInfo *pPitchInfo = new CPitchInfo();
		Q_strncpy(pPitchInfo->m_szFolderName, pitchFolders[i].name, sizeof(pPitchInfo->m_szFolderName));

		CUtlVector<FileInfo_t> pitchFolderFiles;
		FindFiles(pitchFolders[i].path, pitchFolderFiles);

		for (int j = 0; j < pitchFolderFiles.Count(); j++)
		{
			if (pitchFolderFiles[j].isDirectory)
				continue;

			if (!Q_strcmp(pitchFolderFiles[j].name, "pitchdata.txt"))
			{
				KeyValues *pKV = new KeyValues("PitchData");
				pKV->LoadFromFile(filesystem, pitchFolderFiles[j].path, "MOD");

				Q_strncpy(pPitchInfo->m_szName, pKV->GetString("Name", "???"), sizeof(pPitchInfo->m_szName));
				Q_strncpy(pPitchInfo->m_szAuthor, pKV->GetString("Author", "???"), sizeof(pPitchInfo->m_szAuthor));
				pPitchInfo->m_nType = pKV->GetInt("Type", 0);

				pKV->deleteThis();
			}
		}

		CPitchInfo::m_PitchInfo.AddToTail(pPitchInfo);
	}
}