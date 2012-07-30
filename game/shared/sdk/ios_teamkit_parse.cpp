//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <KeyValues.h>
#include <tier0/mem.h>
#include "filesystem.h"
#include "utldict.h"
#include "ammodef.h"

#include "ios_teamkit_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define KITSCRIPT_PATH "materials/models/player/teams"

CUtlDict< CTeamKitInfo*, unsigned short > m_TeamKitInfoDatabase;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : CTeamKitInfo
//-----------------------------------------------------------------------------
static TEAMKIT_FILE_INFO_HANDLE FindTeamKitInfoSlot( const char *name )
{
	// Complain about duplicately defined metaclass names...
	unsigned short lookup = m_TeamKitInfoDatabase.Find( name );
	if ( lookup != m_TeamKitInfoDatabase.InvalidIndex() )
	{
		return lookup;
	}

	CTeamKitInfo *insert = new CTeamKitInfo();

	lookup = m_TeamKitInfoDatabase.Insert( name, insert );
	Assert( lookup != m_TeamKitInfoDatabase.InvalidIndex() );
	return lookup;
}

// Find a class slot, assuming the weapon's data has already been loaded.
TEAMKIT_FILE_INFO_HANDLE LookupTeamKitInfoSlot( const char *name )
{
	return m_TeamKitInfoDatabase.Find( name );
}


// FIXME, handle differently?
static CTeamKitInfo gNullTeamKitInfo;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : handle - 
// Output : CTeamKitInfo
//-----------------------------------------------------------------------------
CTeamKitInfo *GetTeamKitInfoFromHandle( TEAMKIT_FILE_INFO_HANDLE handle )
{
	if ( handle == GetInvalidTeamKitInfoHandle() )
	{
		Assert( !"bad index into playerclass info UtlDict" );
		return &gNullTeamKitInfo;
	}

	return m_TeamKitInfoDatabase[ handle ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : TEAMKIT_FILE_INFO_HANDLE
//-----------------------------------------------------------------------------
TEAMKIT_FILE_INFO_HANDLE GetInvalidTeamKitInfoHandle( void )
{
	return (TEAMKIT_FILE_INFO_HANDLE)m_TeamKitInfoDatabase.InvalidIndex();
}

void ResetTeamKitInfoDatabase( void )
{
	m_TeamKitInfoDatabase.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: Read data on weapon from script file
// Output:  true  - if data2 successfully read
//			false - if data load fails
//-----------------------------------------------------------------------------
bool ReadTeamKitDataFromFileForSlot(IFileSystem* filesystem, const char *szTeamKitName, TEAMKIT_FILE_INFO_HANDLE *phandle)
{
	if (!phandle)
	{
		Assert(0);
		return false;
	}

	*phandle = FindTeamKitInfoSlot(szTeamKitName);
	CTeamKitInfo *pFileInfo = GetTeamKitInfoFromHandle(*phandle);
	Assert(pFileInfo);

	if (pFileInfo->m_bParsedScript)
		return true;

	char filename[128];
	Q_snprintf(filename, sizeof(filename), "%s/%s/kitdata.txt", KITSCRIPT_PATH, szTeamKitName);
	KeyValues *pKV = new KeyValues("KitData");

	if (!pKV->LoadFromFile(filesystem, filename, "MOD"))
	{
		pKV->deleteThis();
		return false;
	}

	pFileInfo->Parse(pKV, szTeamKitName);

	pKV->deleteThis();

	return true;
}

CTeamKitInfo::CTeamKitInfo()
{
	m_bParsedScript = false;

	m_bIsClubTeam = false;
	m_bIsRealTeam = false;
	m_bHasTeamCrest = false;
	m_szTeamCode[0] = 0;
	m_szKitName[0] = 0;
	m_szShortTeamName[0] = 0;
	m_szFullTeamName[0] = 0;
	m_HudKitColor = Color(0, 0, 0, 0);
	m_PrimaryKitColor = Color(0, 0, 0, 0);
	m_SecondaryKitColor = Color(0, 0, 0, 0);;
}

struct RGB
{
	int R;
	int G;
	int B;
};

struct HSL
{
	double H;
	double S;
	double L;
};

void RGBtoHSL(const RGB &rgb, HSL &hsl)
{
	double R = ( rgb.R / 255.0 );                     //RGB from 0 to 255
	double G = ( rgb.G / 255.0 );
	double B = ( rgb.B / 255.0 );

	double minVal = min(min(R, G), B);    //Min. value of RGB
	double maxVal = max(max(R, G), B);    //Max. value of RGB
	double deltaMax = maxVal - minVal;             //Delta RGB value

	double L = ( maxVal + minVal ) / 2;
	double H;
	double S;
	if ( deltaMax == 0 )                     //This is a gray, no chroma...
	{
		H = 0;                                //HSL results from 0 to 1
		S = 0;
	}
	else                                    //Chromatic data...
	{
		if ( L < 0.5 ) S = deltaMax / ( maxVal + minVal );
		else           S = deltaMax / ( 2 - maxVal - minVal );

		double deltaR = ( ( ( maxVal - R ) / 6 ) + ( deltaMax / 2 ) ) / deltaMax;
		double deltaG = ( ( ( maxVal - G ) / 6 ) + ( deltaMax / 2 ) ) / deltaMax;
		double deltaB = ( ( ( maxVal - B ) / 6 ) + ( deltaMax / 2 ) ) / deltaMax;

		if      ( R == maxVal ) H = deltaB - deltaG;
		else if ( G == maxVal ) H = ( 1 / 3.0 ) + deltaR - deltaB;
		else if ( B == maxVal ) H = ( 2 / 3.0 ) + deltaG - deltaR;

		if ( H < 0 ) H += 1;
		if ( H > 1 ) H -= 1;
	}

	hsl.H = H * 360;
	hsl.S = S * 100;
	hsl.L = L * 100;
}

enum colors_t { BLACKS, WHITES, GRAYS, REDS, YELLOWS, GREENS, CYANS, BLUES, MAGENTAS, COLOR_COUNT };
char colorNames[COLOR_COUNT][16] = { "BLACKS", "WHITES", "GRAYS", "REDS", "YELLOWS", "GREENS", "CYANS", "BLUES", "MAGENTAS" };
int substituteColors[COLOR_COUNT][3] = { { 148, 140, 117 }, { 254, 249, 240 }, { 216, 216, 192 }, { 253, 170, 159 }, { 251, 184, 41 }, { 195, 255, 104 }, { 108, 223, 234 }, { 161, 190, 230 }, { 254, 67, 101 } };

colors_t classify(const HSL &hsl)
{
	double hue = hsl.H;
	double sat = hsl.S;
	double lgt = hsl.L;

    if (lgt < 20)  return BLACKS;
    if (lgt > 80)  return WHITES;

    if (sat < 25) return GRAYS;

    if (hue < 30)   return REDS;
    if (hue < 90)   return YELLOWS;
    if (hue < 150)  return GREENS;
    if (hue < 210)  return CYANS;
    if (hue < 270)  return BLUES;
    if (hue < 330)  return MAGENTAS;
	
	return REDS; // (hue >= 330)
}

void CTeamKitInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	m_bParsedScript = true;

	m_bIsClubTeam = pKeyValuesData->GetInt("isclubteam", 0) != 0;
	m_bIsRealTeam = pKeyValuesData->GetInt("isrealteam", 0) != 0;
	m_bHasTeamCrest = pKeyValuesData->GetInt("hasteamcrest", 0) != 0;
	Q_strncpy(m_szTeamCode, pKeyValuesData->GetString("teamcode", "???"), MAX_TEAMCODE_LENGTH);
	Q_strncpy(m_szKitName, pKeyValuesData->GetString("kitname", "???"), MAX_KITNAME_LENGTH);
	Q_strncpy(m_szShortTeamName, pKeyValuesData->GetString("shortteamname", "???"), MAX_SHORTTEAMNAME_LENGTH);
	Q_strncpy(m_szFullTeamName, pKeyValuesData->GetString("fullteamname", "???"), MAX_FULLTEAMNAME_LENGTH);
	Color c = pKeyValuesData->GetColor("primarykitcolor");
	m_PrimaryKitColor = Color(c.r(), c.g(), c.b(), 255);
	c = pKeyValuesData->GetColor("secondarykitcolor");
	m_SecondaryKitColor = Color(c.r(), c.g(), c.b(), 255);

	RGB primaryRgb = { m_PrimaryKitColor.r(), m_PrimaryKitColor.g(), m_PrimaryKitColor.b() };
	RGB secondaryRgb = { m_SecondaryKitColor.r(), m_SecondaryKitColor.g(), m_SecondaryKitColor.b() };
	HSL primaryHsl, secondaryHsl;
	RGBtoHSL(primaryRgb, primaryHsl);
	colors_t color = classify(primaryHsl);
	m_HudKitColor = Color(substituteColors[color][0], substituteColors[color][1], substituteColors[color][2], 255);
}

void CTeamKitInfo::FindTeamKits()
{
	char filefilter[128];
	Q_snprintf(filefilter, sizeof(filefilter), "%s/*.*", KITSCRIPT_PATH);
	FileFindHandle_t findHandle;
	const char *pFilename = filesystem->FindFirstEx(filefilter, "MOD", &findHandle);
	while (pFilename)
	{
		if (Q_strcmp(pFilename, ".") != 0 && Q_strcmp(pFilename, "..") != 0)
		{
			char fullFilename[128];
			Q_snprintf(fullFilename, sizeof(fullFilename), "%s/%s", KITSCRIPT_PATH, pFilename);

			if (filesystem->IsDirectory(fullFilename, "MOD"))
			{
				TEAMKIT_FILE_INFO_HANDLE hTeamKitInfo;
				ReadTeamKitDataFromFileForSlot(filesystem, pFilename, &hTeamKitInfo);
			}
		}

		pFilename = filesystem->FindNext(findHandle);
	}

	filesystem->FindClose(findHandle);
}