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
	bool cancelled;
	bool finished;
	bool async;
	bool checkOnly;
	int filesToUpdateCount;
	int filesUpdatedCount;
	bool restartRequired;
	bool connectionError;

	IOSUpdateInfo()
	{
		Reset();
	}

	void Reset()
	{
		cancelled = false;
		finished = false;
		async = false;
		checkOnly = false;
		filesToUpdateCount = 0;
		filesUpdatedCount = 0;
		restartRequired = false;
		connectionError = false;
	}
};

class CFileUpdater
{
public:
	static void UpdateFiles(IOSUpdateInfo *pUpdateInfo);
	static int UpdateFinished(IOSUpdateInfo *pUpdateInfo);
};

#endif