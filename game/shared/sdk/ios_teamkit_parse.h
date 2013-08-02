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

struct chr_t
{
	int x;
	int y;
	int w;
	int h;

	chr_t() : x(0), y(0), w(0), h(0) {}
};

class CFontAtlas
{
public:
	unsigned char **m_NamePixels;
	int m_nNamePixelsWidth;
	int m_nNamePixelsHeight;
	chr_t m_NameChars[128];

	unsigned char **m_NumberPixels;
	int m_nNumberPixelsWidth;
	int m_nNumberPixelsHeight;
	chr_t m_NumberChars[128];

	CFontAtlas(const char *folderPath);
	unsigned char **ParseInfo(const char *folderPath, const char *type, chr_t *chars, int &width, int &height);
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
	Color		m_OutfieldShirtNameColor;
	Color		m_OutfieldShirtNumberColor;
	int			m_nOutfieldShirtNameOffset;
	int			m_nOutfieldShirtNumberOffset;
	Color		m_KeeperShirtNameColor;
	Color		m_KeeperShirtNumberColor;
	int			m_nKeeperShirtNameOffset;
	int			m_nKeeperShirtNumberOffset;
	CFontAtlas	*m_pFontAtlas;
	CTeamInfo	*m_pTeamInfo;

	static CFontAtlas *m_pDefaultFontAtlas;

	CTeamKitInfo();
};

class CTeamInfo
{
public:

	// Each game can override this to get whatever values it wants from the script.
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
};

#endif // TeamKit_INFO_PARSE_H
