#ifndef FILE_UPDATER_H
#define FILE_UPDATER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

#ifdef CLIENT_DLL
#include "c_sdk_player.h"
#else
#include "sdk_player.h"
#endif

struct IOSUpdateInfo
{
	bool checkOnly;
	int filesToUpdateCount;
	bool restartRequired;
	bool connectionError;
#ifdef CLIENT_DLL
	CHandle<C_SDKPlayer> pClient;
#else
	CHandle<CSDKPlayer> pClient;
#endif

	IOSUpdateInfo()
	{
		checkOnly = false;
		filesToUpdateCount = 0;
		restartRequired = false;
		connectionError = false;
		pClient = NULL;
	}
};

class CFileUpdater
{
public:
	static void UpdateFiles(IOSUpdateInfo *pUpdateInfo);
	static int UpdateFinished(IOSUpdateInfo *pUpdateInfo);
};

#endif