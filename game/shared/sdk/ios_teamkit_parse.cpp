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
	m_szTeamCode[0] = 0;
	m_szKitName[0] = 0;
	m_szShortTeamName[0] = 0;
	m_szFullTeamName[0] = 0;
	m_PrimaryKitColor = Color(0, 0, 0, 0);
	m_SecondaryKitColor = Color(0, 0, 0, 0);;
}

void CTeamKitInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	m_bParsedScript = true;

	m_bIsClubTeam = pKeyValuesData->GetInt("isclubteam", 0) != 0;
	m_bIsRealTeam = pKeyValuesData->GetInt("isrealteam", 0) != 0;
	Q_strncpy(m_szTeamCode, pKeyValuesData->GetString("teamcode", "???"), MAX_TEAMCODE_LENGTH);
	Q_strncpy(m_szKitName, pKeyValuesData->GetString("kitname", "???"), MAX_KITNAME_LENGTH);
	Q_strncpy(m_szShortTeamName, pKeyValuesData->GetString("shortteamname", "???"), MAX_SHORTTEAMNAME_LENGTH);
	Q_strncpy(m_szFullTeamName, pKeyValuesData->GetString("fullteamname", "???"), MAX_FULLTEAMNAME_LENGTH);
	Color c = pKeyValuesData->GetColor("primarykitcolor");
	m_PrimaryKitColor = Color(c.r(), c.g(), c.b(), 255);
	c = pKeyValuesData->GetColor("secondarykitcolor");
	m_SecondaryKitColor = Color(c.r(), c.g(), c.b(), 255);
}

void CTeamKitInfo::FindTeamKits()
{
	char filefilter[128];
	Q_snprintf(filefilter, sizeof(filefilter), "%s/*.*", KITSCRIPT_PATH);
	FileFindHandle_t findHandle;
	const char *pFilename = filesystem->FindFirstEx(filefilter, "MOD", &findHandle);
	while (pFilename)
	{
		TEAMKIT_FILE_INFO_HANDLE hTeamKitInfo;
		ReadTeamKitDataFromFileForSlot(filesystem, pFilename, &hTeamKitInfo);
		pFilename = filesystem->FindNext(findHandle);
	}

	filesystem->FindClose(findHandle);
}