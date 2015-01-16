//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Player class data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEAMKIT_PARSE_H
#define TEAMKIT_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

class IFileSystem;
class KeyValues;
class CTeamInfo;

#define MAX_TEAMCODE_LENGTH				8
#define MAX_KITNAME_LENGTH				64
#define MAX_FOLDERNAME_LENGTH			64
#define MAX_SHORTTEAMNAME_LENGTH		32
#define MAX_FULLTEAMNAME_LENGTH			64

struct glyphWithOutline_t
{
    unsigned char glyph;
    unsigned char outline;
};

struct kerning_t
{
	int charBefore;
	int amount;

	kerning_t(int charBefore, int amount) : charBefore(charBefore), amount(amount) {}
};

struct chr_t
{
	int glyph;
	int x;
	int y;
	int width;
	int height;
	int offsetX;
	int offsetY;
	int advanceX;
	CUtlVector<kerning_t> kernings;

	chr_t() : glyph(0), x(0), y(0), width(0), height(0), offsetX(0), offsetY(0), advanceX(0) {}

	int GetKerning(int charBefore) const
	{
		for (int i = 0; i < kernings.Count(); i++)
		{
			if (kernings[i].charBefore == charBefore)
			{
				return kernings[i].amount;
			}
		}

		return 0;
	}
};

class CFontAtlas
{
public:
	glyphWithOutline_t **m_ShirtNamePixels;
	int m_nNamePixelsWidth;
	int m_nNamePixelsHeight;
	chr_t m_NameChars[512];

	glyphWithOutline_t **m_ShirtBackNumberPixels;
	int m_nShirtBackNumberPixelsWidth;
	int m_nShirtBackNumberPixelsHeight;
	chr_t m_ShirtBackNumberChars[64];

	glyphWithOutline_t **m_ShirtAndShortsNumberPixels;
	int m_nShirtAndShortsNumberPixelsWidth;
	int m_nShirtAndShortsNumberPixelsHeight;
	chr_t m_ShirtAndShortsNumberChars[64];

	CFontAtlas(const char *folderPath, bool hasShirtAndShortsNumberAtlas);
	glyphWithOutline_t **ParseInfo(const char *folderPath, const char *type, chr_t *chars, int maxChars, int &width, int &height);
};

class CTeamKitInfo
{
public:

	char		m_szName[MAX_KITNAME_LENGTH];
	char		m_szAuthor[MAX_PLAYER_NAME_LENGTH];
	char		m_szFolderName[MAX_FOLDERNAME_LENGTH];
	Color		m_HudColor;
	Color		m_PrimaryColor;
	Color		m_SecondaryColor;


	Color		m_OutfieldShirtNameFillColor;
	Color		m_OutfieldShirtNameOutlineColor;
	int			m_nOutfieldShirtNameVerticalOffset;

	Color		m_OutfieldShirtBackNumberFillColor;
	Color		m_OutfieldShirtBackNumberOutlineColor;
	int			m_nOutfieldShirtBackNumberVerticalOffset;

	Color		m_OutfieldShortsNumberFillColor;
	Color		m_OutfieldShortsNumberOutlineColor;
	int			m_nOutfieldShortsNumberHorizontalOffset;
	int			m_nOutfieldShortsNumberVerticalOffset;

	bool		m_bHasOutfieldShirtFrontNumber;
	Color		m_OutfieldShirtFrontNumberFillColor;
	Color		m_OutfieldShirtFrontNumberOutlineColor;
	int			m_nOutfieldShirtFrontNumberHorizontalOffset;
	int			m_nOutfieldShirtFrontNumberVerticalOffset;


	Color		m_KeeperShirtNameFillColor;
	Color		m_KeeperShirtNameOutlineColor;
	int			m_nKeeperShirtNameVerticalOffset;

	Color		m_KeeperShirtBackNumberFillColor;
	Color		m_KeeperShirtBackNumberOutlineColor;
	int			m_nKeeperShirtBackNumberVerticalOffset;

	Color		m_KeeperShortsNumberFillColor;
	Color		m_KeeperShortsNumberOutlineColor;
	int			m_nKeeperShortsNumberHorizontalOffset;
	int			m_nKeeperShortsNumberVerticalOffset;

	bool		m_bHasKeeperShirtFrontNumber;
	Color		m_KeeperShirtFrontNumberFillColor;
	Color		m_KeeperShirtFrontNumberOutlineColor;
	int			m_nKeeperShirtFrontNumberHorizontalOffset;
	int			m_nKeeperShirtFrontNumberVerticalOffset;


	CFontAtlas	*m_pFontAtlas;
	CTeamInfo	*m_pTeamInfo;

	static CFontAtlas *m_pDefaultFontAtlas;

	CTeamKitInfo();
};

class CTeamInfo
{
public:

	static void ParseTeamKits();
	static void GetNonClashingTeamKits(char *homeTeam, char *awayTeam, bool clubTeams, bool nationalTeams, bool realTeams, bool fictitiousTeams);
	static CTeamKitInfo *FindTeamByKitName(const char *name);
	static CTeamKitInfo *FindTeamByShortName(const char *name);
	static CTeamKitInfo *FindTeamByCode(const char *name);
	bool		m_bIsClub;
	bool		m_bIsReal;
	bool		m_bHasCrest;
	char		m_szCode[MAX_TEAMCODE_LENGTH];
	char		m_szShortName[MAX_SHORTTEAMNAME_LENGTH];
	char		m_szFullName[MAX_FULLTEAMNAME_LENGTH];
	char		m_szFolderName[MAX_FOLDERNAME_LENGTH];

	CTeamInfo();
	~CTeamInfo();

	static CUtlVector<CTeamInfo *> m_TeamInfo;
	CUtlVector<CTeamKitInfo *> m_TeamKitInfo;

	static float m_flLastUpdateTime;
};

class CBallInfo
{
public:

	static void ParseBallSkins();
	static CUtlVector<CBallInfo *> m_BallInfo;
	char m_szName[MAX_KITNAME_LENGTH];
	char m_szAuthor[MAX_PLAYER_NAME_LENGTH];
	char m_szFolderName[MAX_FOLDERNAME_LENGTH];
	static float m_flLastUpdateTime;

	CBallInfo()
	{
		m_szName[0] = '\0';
		m_szAuthor[0] = '\0';
	}
};

class CPitchInfo
{
public:

	static void ParsePitchTextures();
	static CUtlVector<CPitchInfo *> m_PitchInfo;
	char m_szName[MAX_KITNAME_LENGTH];
	char m_szAuthor[MAX_PLAYER_NAME_LENGTH];
	int m_nType;
	char m_szFolderName[MAX_FOLDERNAME_LENGTH];

	CPitchInfo()
	{
		m_szName[0] = '\0';
		m_szAuthor[0] = '\0';
		m_nType = 0;
	}
};

#endif // TeamKit_INFO_PARSE_H
