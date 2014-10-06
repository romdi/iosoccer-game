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
	glyphWithOutline_t **m_ShirtBackNamePixels;
	int m_nNamePixelsWidth;
	int m_nNamePixelsHeight;
	chr_t m_NameChars[512];

	glyphWithOutline_t **m_ShirtBackNumberPixels;
	int m_nShirtBackNumberPixelsWidth;
	int m_nShirtBackNumberPixelsHeight;
	chr_t m_ShirtBackNumberChars[64];

	glyphWithOutline_t **m_ShirtAndShortsFrontNumberPixels;
	int m_nShirtAndShortsFrontNumberPixelsWidth;
	int m_nShirtAndShortsFrontNumberPixelsHeight;
	chr_t m_ShirtAndShortsFrontNumberChars[64];

	CFontAtlas(const char *folderPath);
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

	Color		m_OutfieldShirtNameAndNumberColor;

	Color		m_OutfieldShirtNumberOutlineColor;

	Color		m_OutfieldShirtBackNameOutlineColor;
	int			m_nOutfieldShirtBackNameVerticalOffset;

	int			m_nOutfieldShirtBackNumberVerticalOffset;

	bool		m_bHasOutfieldShirtFrontNumber;
	int			m_nOutfieldShirtFrontNumberHorizontalOffset;
	int			m_nOutfieldShirtFrontNumberVerticalOffset;

	Color		m_OutfieldShortsFrontNumberColor;
	Color		m_OutfieldShortsFrontNumberOutlineColor;
	int			m_nOutfieldShortsFrontNumberHorizontalOffset;

	Color		m_KeeperShirtNameAndNumberColor;

	Color		m_KeeperShirtNumberOutlineColor;

	Color		m_KeeperShirtBackNameOutlineColor;
	int			m_nKeeperShirtBackNameVerticalOffset;

	int			m_nKeeperShirtBackNumberVerticalOffset;

	bool		m_bHasKeeperShirtFrontNumber;
	int			m_nKeeperShirtFrontNumberHorizontalOffset;
	int			m_nKeeperShirtFrontNumberVerticalOffset;

	Color		m_KeeperShortsFrontNumberColor;
	Color		m_KeeperShortsFrontNumberOutlineColor;
	int			m_nKeeperShortsFrontNumberHorizontalOffset;

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

#endif // TeamKit_INFO_PARSE_H
