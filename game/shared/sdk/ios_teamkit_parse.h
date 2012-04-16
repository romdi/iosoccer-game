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

typedef unsigned short TEAMKIT_FILE_INFO_HANDLE;

#define MAX_TEAMCODE_LENGTH				8
#define MAX_KITNAME_LENGTH				32
#define MAX_SHORTTEAMNAME_LENGTH		32
#define MAX_FULLTEAMNAME_LENGTH			64

//-----------------------------------------------------------------------------
// Purpose: Contains the data read from the player class script files. 
// It's cached so we only read each script file once.
// Each game provides a CreateTeamKitInfo function so it can have game-specific
// data in the player class scripts.
//-----------------------------------------------------------------------------
class CTeamKitInfo
{
public:

	CTeamKitInfo();
	
	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse( KeyValues *pKeyValuesData, const char *szClassName );
	static void FindTeamKits();

	bool		m_bParsedScript;

	bool		m_bIsClubTeam;
	bool		m_bIsRealTeam;
	bool		m_bHasTeamCrest;
	char		m_szTeamCode[MAX_TEAMCODE_LENGTH];
	char		m_szKitName[MAX_KITNAME_LENGTH];
	char		m_szShortTeamName[MAX_SHORTTEAMNAME_LENGTH];
	char		m_szFullTeamName[MAX_FULLTEAMNAME_LENGTH];
	Color		m_PrimaryKitColor;
	Color		m_SecondaryKitColor;
};

// The weapon parse function
bool ReadTeamKitDataFromFileForSlot( IFileSystem* filesystem, const char *szTeamKitName, TEAMKIT_FILE_INFO_HANDLE *phandle);

// If player class info has been loaded for the specified class name, this returns it.
TEAMKIT_FILE_INFO_HANDLE LookupTeamKitInfoSlot( const char *name );

// Given a handle to the player class info, return the class data
CTeamKitInfo *GetTeamKitInfoFromHandle( TEAMKIT_FILE_INFO_HANDLE handle );

// Get the null Player Class object
TEAMKIT_FILE_INFO_HANDLE GetInvalidTeamKitInfoHandle( void );

// Initialize all player class info
void ResetTeamKitInfoDatabase( void );

extern CUtlDict< CTeamKitInfo*, unsigned short > m_TeamKitInfoDatabase;

#endif // TeamKit_INFO_PARSE_H
