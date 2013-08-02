#include "cbase.h"
#include "ios_fileupdater.h"
#include "curl/curl.h"
#include "Filesystem.h"
#include "utlbuffer.h"
#include "checksum_md5.h"
#include "threadtools.h"
#include "ios_teamkit_parse.h"

ConVar cl_download_url("cl_download_url", "http://simrai.iosoccer.com/downloads/clientfiles/iosoccer");

struct FileInfo
{
	char path[256];
	char md5[33];
};

struct TeamKitCurl
{
	FileHandle_t fh;
	MD5Context_t md5Ctx;
};

static size_t rcvFileListData(void *ptr, size_t size, size_t nmemb, CUtlBuffer &buffer)
{
	buffer.Put(ptr, nmemb);

	return size * nmemb;
}

static size_t rcvKitData(void *ptr, size_t size, size_t nmemb, TeamKitCurl *vars)
{
	filesystem->Write(ptr, nmemb, vars->fh);
	MD5Update(&vars->md5Ctx, (unsigned char *)ptr, nmemb);

	return size * nmemb;
}

void GetFileList(char *fileListString, CUtlVector<FileInfo> &fileList)
{
	char *file = strtok(fileListString, "\n");

	while (file != NULL)
	{
		FileInfo fileInfo;
		Q_strncpy(fileInfo.path, file, min(strcspn(file, ":") + 1, sizeof(fileInfo.path)));
		Q_strncpy(fileInfo.md5, strstr(file, ":") + 1, sizeof(fileInfo.md5));
//#ifdef GAME_DLL
//		if (strstr(fileInfo.path, ".txt"))
//			fileList.AddToTail(fileInfo);
//#else
		fileList.AddToTail(fileInfo);
//#endif

		file = strtok(NULL, "\n");
	}
}

unsigned PerformUpdate(void *params)
{
	IOSUpdateInfo *pUpdateInfo = (IOSUpdateInfo *)params;

	const char *fileListPath = "filelist.txt";

	CUtlVector<FileInfo> localFileList;

	if (filesystem->FileExists(fileListPath, "MOD"))
	{
		FileHandle_t fh = filesystem->Open(fileListPath, "r", "MOD");
		int fileSize = filesystem->Size(fh);
		char *localFileListString = new char[fileSize + 1];
 
		filesystem->Read((void *)localFileListString, fileSize, fh);
		localFileListString[fileSize] = 0; // null terminator
 
		filesystem->Close(fh);
		GetFileList(localFileListString, localFileList);
		delete[] localFileListString;
	}

	CUtlBuffer buffer;
	CURL *curl;
	curl = curl_easy_init();
	char url[512];
	Q_snprintf(url, sizeof(url), "%s/filelist.txt.gz", cl_download_url.GetString());
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rcvFileListData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
	CURLcode result = curl_easy_perform(curl);
	long code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	curl_easy_cleanup(curl);

	if (result != CURLE_OK || code != 200)
	{
		pUpdateInfo->connectionError = true;
		pUpdateInfo->filesToUpdateCount = 0;
		return 1;
	}

	char *serverFileListString = new char[buffer.Size() + 1];
	buffer.GetString(serverFileListString);
	serverFileListString[buffer.Size()] = 0;

	CUtlVector<FileInfo> serverFileList;

	GetFileList(serverFileListString, serverFileList);

	delete[] serverFileListString;

	for (int i = 0; i < localFileList.Count(); i++)
	{
		for (int j = 0; j < serverFileList.Count(); j++)
		{
			if (!Q_strcmp(serverFileList[j].path, localFileList[i].path))
			{
				if (!Q_strcmp(serverFileList[j].md5, localFileList[i].md5))
					serverFileList.Remove(j);
				else
				{
					localFileList.Remove(i);
					i -= 1;
				}

				break;
			}
		}
	}

	pUpdateInfo->filesToUpdateCount = serverFileList.Count();

	if (pUpdateInfo->checkOnly || pUpdateInfo->filesToUpdateCount == 0)
		return 0;

	FileHandle_t fh = filesystem->Open(fileListPath, "w", "MOD");

	for (int i = 0; i < localFileList.Count(); i++)
	{
		char str[256];
		Q_snprintf(str, sizeof(str), "%s%s:%s", i == 0 ? "" : "\n", localFileList[i].path, localFileList[i].md5);
		filesystem->Write(str, strlen(str), fh);
	}

	for (int i = 0; i < serverFileList.Count(); i++)
	{
		char filePath[256];
		Q_snprintf(filePath, sizeof(filePath), "%s", serverFileList[i].path);

		char folderPath[256];
		Q_strncpy(folderPath, filePath, sizeof(folderPath));

		char *pos = strrchr(folderPath, '/');
		if (pos)
		{
			*pos = '\0';

			if (!filesystem->FileExists(folderPath, "MOD"))
				filesystem->CreateDirHierarchy(folderPath, "MOD");
		}

		if (!Q_strcmp(&filePath[strlen(filePath) - 4], ".dll") && filesystem->FileExists(filePath, "MOD"))
		{
			char newFilePath[256];
			Q_snprintf(newFilePath, sizeof(newFilePath), "%s_old", filePath);

			if (filesystem->FileExists(newFilePath, "MOD"))
				filesystem->RemoveFile(newFilePath, "MOD");

			filesystem->RenameFile(filePath, newFilePath, "MOD");

			pUpdateInfo->restartRequired = true;
		}

		TeamKitCurl curlData;

		curlData.fh = filesystem->Open(filePath, "wb", "MOD");

		memset(&curlData.md5Ctx, 0, sizeof(MD5Context_t));
		MD5Init(&curlData.md5Ctx);

		curl = curl_easy_init();
		Q_snprintf(url, sizeof(url), "%s/%s.gz", cl_download_url.GetString(), serverFileList[i].path);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rcvKitData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlData);
		CURLcode result = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
		curl_easy_cleanup(curl);

		filesystem->Close(curlData.fh);

		unsigned char digest[MD5_DIGEST_LENGTH];
		MD5Final(digest, &curlData.md5Ctx);
		char hexDigest[MD5_DIGEST_LENGTH * 2 + 1];

		for (int j = 0; j < MD5_DIGEST_LENGTH; j++)
			Q_snprintf(&hexDigest[j * 2], sizeof(hexDigest) - (j * 2), "%02x", digest[j]);

		char str[256];
		Q_snprintf(str, sizeof(str), "%s%s:%s", localFileList.Count() == 0 && i == 0 ? "" : "\n", serverFileList[i].path, hexDigest);
		filesystem->Write(str, strlen(str), fh);
	}

	filesystem->Close(fh);

	CFileUpdater::UpdateFinished();

	return 0;
}

void CFileUpdater::UpdateFiles(IOSUpdateInfo *pUpdateInfo)
{
	PerformUpdate(pUpdateInfo);
}

void CFileUpdater::UpdateFinished()
{
}