#ifndef FILE_UPDATER_H
#define FILE_UPDATER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

struct IOSUpdateInfo
{
	bool checkOnly;
	int filesToUpdateCount;
	bool restartRequired;
	bool connectionError;

	IOSUpdateInfo()
	{
		checkOnly = false;
		filesToUpdateCount = 0;
		restartRequired = false;
		connectionError = false;
	}
};

class CFileUpdater
{
public:
	static void UpdateFiles(IOSUpdateInfo *pUpdateInfo);
	static void UpdateFinished();
};

#endif