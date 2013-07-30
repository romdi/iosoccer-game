/*
	Color conversions
	Copyright (c) 2011, Cory Nelson (phrosty@gmail.com)
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
		* Redistributions of source code must retain the above copyright
			notice, this list of conditions and the following disclaimer.
		* Redistributions in binary form must reproduce the above copyright
			notice, this list of conditions and the following disclaimer in the
			documentation and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	When possible, constants are given as accurate pre-computed rationals. When not,
	they are given at double precision with a comment on how to compute them.
*/


#include "cbase.h"
#include <KeyValues.h>
#include <tier0/mem.h>
#include "filesystem.h"
#include "utldict.h"
#include "ammodef.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "ios_teamkit_parse.h"
#include "sdk_gamerules.h"
#include "curl/curl.h"
#include "Filesystem.h"
#include "utlbuffer.h"
#include "checksum_md5.h"
#include "threadtools.h"
#ifdef CLIENT_DLL
#include "vgui_controls/messagebox.h"
#include "clientmode_shared.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_download_url("cl_download_url", "http://simrai.iosoccer.com/downloads");

inline double clamphue(double hue)
{
	hue = fmod(hue, M_PI * 2);

	if(hue < 0)
	{
		hue += M_PI * 2;
	}

	return hue;
}

struct RGB
{
	int R;
	int G;
	int B;
};

struct LCh
{
	double L;
	double C;
	double h;
};

struct HSL
{
	double H;
	double S;
	double L;
};


double toLabc(double c)
{
	if (c > 216 / 24389.0) return pow(c, 1 / 3.0);
	return c * (841 / 108.0) + (4 / 49.0);
}

void RGBtoLCh(const RGB &rgb, LCh &lch)
{
	double R = clamp(rgb.R, 0, 255) / 255.0;        //R from 0 to 255
	double G = clamp(rgb.G, 0, 255) / 255.0;        //G from 0 to 255
	double B = clamp(rgb.B, 0, 255) / 255.0;        //B from 0 to 255

	if ( R > 0.04045 )
		R = pow( ( R + 0.055 ) / 1.055 , 2.4);
	else
		R = R / 12.92;
		
	if ( G > 0.04045 )
		G = pow( ( G + 0.055 ) / 1.055 , 2.4);
	else
		G = G / 12.92;
		
	if ( B > 0.04045 )
		B = pow( ( B + 0.055 ) / 1.055 , 2.4);
	else
		B = B / 12.92;

	double X = clamp(0.4124564 * R + 0.3575761 * G + 0.1804375 * B, 0, (31271 / 32902.0));
	double Y = clamp(0.2126729 * R + 0.7151522 * G + 0.0721750 * B, 0, 1);
	double Z = clamp(0.0193339 * R + 0.1191920 * G + 0.9503041 * B, 0, (35827 / 32902.0));

	double X_Lab = toLabc(X / (31271 / 32902.0)); // normalized standard observer D65.
	double Y_Lab = toLabc(Y / 1.0);
	double Z_Lab = toLabc(Z / (35827 / 32902.0));

	double L = clamp(116 * Y_Lab - 16, 0, 100);
	double a = clamp(500 * (X_Lab - Y_Lab), -12500 / 29.0, 12500 / 29.0);
	double b = clamp(200 * (Y_Lab - Z_Lab), -5000 / 29.0, 5000 / 29.0);
	
	double C = clamp(sqrt(a * a + b * b), 0, 4.64238345442629658e2); // 2500*sqrt(1 / 29.0)
	double h = clamphue(atan2(b, a));

	lch.L = L;
	lch.C = C;
	lch.h = h;
}

void RGBtoHSL(const RGB &rgb, HSL &hsl)
{
	double R = ( rgb.R / 255.0 );                     //RGB from 0 to 255
	double G = ( rgb.G / 255.0 );
	double B = ( rgb.B / 255.0 );

	double minVal = min(min( R, G), B );    //Min. value of RGB
	double maxVal = max(max( R, G), B );    //Max. value of RGB
	double deltaMax = maxVal - minVal;             //Delta RGB value

	double L = ( maxVal + minVal ) / 2;
	double H = 0;
	double S = 0;

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

// CIE Delta E 2000
// Note: maximum is about 158 for colors in the sRGB gamut.
double deltaE2000(RGB rgb1, RGB rgb2)
{
	LCh lch1, lch2;
	RGBtoLCh(rgb1, lch1);
	RGBtoLCh(rgb2, lch2);
	
	double avg_L = (lch1.L + lch2.L) * 0.5;
	double delta_L = lch2.L - lch1.L;

	double avg_C = (lch1.C + lch2.C) * 0.5;
	double delta_C = lch1.C - lch2.C;

	double avg_H = (lch1.h + lch2.h) * 0.5;

	if(abs(lch1.h - lch2.h) > M_PI)
	{
		avg_H += M_PI;
	}

	double delta_H = lch2.h - lch1.h;

	if(abs(delta_H) > M_PI)
	{
		if(lch2.h <= lch1.h) delta_H += M_PI * 2;
		else delta_H -= M_PI * 2;
	}

	delta_H = sqrt(lch1.C * lch2.C) * sin(delta_H) * 2;

	double T = 1
	- 0.17 * cos(avg_H - M_PI / 6.0)
	+ 0.24 * cos(avg_H * 2)
	+ 0.32 * cos(avg_H * 3 + M_PI / 30.0)
	- 0.20 * cos(avg_H * 4 - M_PI * 7 / 20.0);

	double SL = avg_L - 50;
	SL *= SL;
	SL = SL * 0.015 / sqrt(SL + 20) + 1;

	double SC = avg_C * 0.045 + 1;

	double SH = avg_C * T * 0.015 + 1;

	double delta_Theta = avg_H / 25.0 - M_PI * 11 / 180.0;
	delta_Theta = exp(delta_Theta * -delta_Theta) * (M_PI / 6.0);

	double RT = pow(avg_C, 7);
	RT = sqrt(RT / (RT + 6103515625)) * sin(delta_Theta) * -2; // 6103515625 = 25^7

	delta_L /= SL;
	delta_C /= SC;
	delta_H /= SH;

	return sqrt(delta_L * delta_L + delta_C * delta_C + delta_H * delta_H + RT * delta_C * delta_H);
}

enum colors_t { BLACKS, WHITES, GRAYS, REDS, YELLOWS, GREENS, CYANS, BLUES, MAGENTAS, COLOR_COUNT };
char colorNames[COLOR_COUNT][16] = { "BLACKS", "WHITES", "GRAYS", "REDS", "YELLOWS", "GREENS", "CYANS", "BLUES", "MAGENTAS" };

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

//int main(int argc, char **argv)
//{
//	while (true)
//	{
//		RGB rgb[2];
//		for (int i = 0; i < 2; i++)
//		{
//			char input[4];
//			printf("RGB%d: ", i + 1);
//			gets(input);
//			rgb[i].R = atof(strtok(input, ", "));
//			rgb[i].G = atof(strtok(NULL, ", "));
//			rgb[i].B = atof(strtok(NULL, ", "));
//		}
//		HSL hsl1, hsl2;
//		RGBtoHSL(rgb[0], hsl1);
//		RGBtoHSL(rgb[1], hsl2);
//		printf("Color classification: %s & %s\n", colorNames[classify(hsl1)], colorNames[classify(hsl2)]);
//		printf("Distance between [%d, %d, %d] and [%d, %d, %d]: %f\n\n", rgb[0].R, rgb[0].G, rgb[0].B, rgb[1].R, rgb[1].G, rgb[1].B, deltaE2000(rgb[0], rgb[1]));
//	}
//
//	return 0;
//}

CFontAtlas *CTeamKitInfo::m_pDefaultFontAtlas = NULL;

CTeamKitInfo::CTeamKitInfo()
{
	m_szName[0] = 0;
	m_szAuthor[0] = 0;
	m_szFolderName[0] = 0;
	m_HudColor = Color(0, 0, 0, 0);
	m_PrimaryColor = Color(0, 0, 0, 0);
	m_SecondaryColor = Color(0, 0, 0, 0);
	m_OutfieldShirtNameColor = Color(0, 0, 0, 0);
	m_nOutfieldShirtNameOffset = 0;
	m_OutfieldShirtNumberColor = Color(0, 0, 0, 0);
	m_nOutfieldShirtNumberOffset = 0;
	m_KeeperShirtNameColor = Color(0, 0, 0, 0);
	m_nKeeperShirtNameOffset = 0;
	m_KeeperShirtNumberColor = Color(0, 0, 0, 0);
	m_nKeeperShirtNumberOffset = 0;
	m_pFontAtlas = NULL;
	m_pTeamInfo = NULL;
}

CUtlVector<CTeamInfo *> CTeamInfo::m_TeamInfo;

CTeamInfo::CTeamInfo()
{
	m_bIsClub = false;
	m_bIsReal = false;
	m_bHasCrest = false;
	m_szCode[0] = 0;
	m_szShortName[0] = 0;
	m_szFullName[0] = 0;
	m_szFolderName[0] = 0;
}

CTeamInfo::~CTeamInfo()
{
	m_TeamKitInfo.PurgeAndDeleteElements();
}

struct FileInfo_t
{
	char name[128];
	char path[128];
	bool isDirectory;
};

void FindFiles(const char *path, CUtlVector<FileInfo_t> &fileInfos)
{
	char filefilter[128];
	Q_snprintf(filefilter, sizeof(filefilter), "%s/*.*", path);
	FileFindHandle_t findHandle;
	const char *filename = filesystem->FindFirstEx(filefilter, "MOD", &findHandle);

	while (filename)
	{
		if (Q_strcmp(filename, ".") && Q_strcmp(filename, ".."))
		{
			FileInfo_t fileInfo;
			Q_strncpy(fileInfo.name, filename, sizeof(fileInfo.name));
			Q_snprintf(fileInfo.path, sizeof(fileInfo.path), "%s/%s", path, filename);
			fileInfo.isDirectory = filesystem->IsDirectory(fileInfo.path, "MOD");
			fileInfos.AddToTail(fileInfo);
		}

		filename = filesystem->FindNext(findHandle);
	}

	filesystem->FindClose(findHandle);
}

#define TEAMKITS_PATH "materials/models/player/teamkits"

#define DEFAULT_FONTATLAS_FOLDER "materials/models/player/default"

CFontAtlas::CFontAtlas(const char *folderPath)
{
	m_NamePixels = ParseInfo(folderPath, "name", m_NameChars, m_nNamePixelsWidth, m_nNamePixelsHeight);
	m_NumberPixels = ParseInfo(folderPath, "number", m_NumberChars, m_nNumberPixelsWidth, m_nNumberPixelsHeight);
}

unsigned char **CFontAtlas::ParseInfo(const char *folderPath, const char *type, chr_t *chars, int &width, int &height)
{
	char path[128];

	Q_snprintf(path, sizeof(path), "%s/%s_atlas.bmp", folderPath, type);
    FileHandle_t fh = filesystem->Open(path, "rb", "MOD");
	int file_len = filesystem->Size(fh);

    unsigned char* fulldata = new unsigned char[file_len];
    filesystem->Read(fulldata, file_len, fh);
	const int pixelStart = 54;

	width = *(int*)&fulldata[18];
    height = *(int*)&fulldata[22];

	unsigned char **pixels = new unsigned char *[height];
	for (int i = 0; i < height; i++)
		pixels[i] = new unsigned char[width];

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
			int pos = pixelStart + y * width + x;
			pixels[height - 1 - y][x] = fulldata[pos];
        }
    }

	delete[] fulldata;
    filesystem->Close(fh);

	Q_snprintf(path, sizeof(path), "%s/%s_atlas.xml", folderPath, type);
    fh = filesystem->Open(path, "rb", "MOD");

	file_len = filesystem->Size(fh);
	char *fulltext = new char[file_len];
    filesystem->Read(fulltext, file_len, fh);
	char *pch;
	pch = strtok(fulltext, "\n");
	while (pch != NULL)
	{
		if (strstr(pch, "<glyph "))
		{
			char chr = *(strstr(pch, "ch=\"") + 4);
			char *origin = strstr(pch, "origin=\"") + 8;
			chars[chr].x = atoi(origin);
			chars[chr].y = atoi(strstr(origin, ",") + 1);
			char *size = strstr(pch, "size=\"") + 6;
			chars[chr].w = atoi(size);
			chars[chr].h = atoi(strstr(size, "x") + 1);
		}
		pch = strtok(NULL, "\n");
	}

	delete[] fulltext;
    filesystem->Close(fh);

	return pixels;
}

struct FileInfo
{
	char path[256];
	char md5[32];
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
#ifdef GAME_DLL
		if (strstr(fileInfo.path, ".txt"))
			fileList.AddToTail(fileInfo);
#else
		fileList.AddToTail(fileInfo);
#endif

		file = strtok(NULL, "\n");
	}
}

unsigned CheckTeamKits(void *params)
{
#ifdef GAME_DLL
	if (!engine->IsDedicatedServer())
	{
		CTeamInfo::ParseTeamKits();
		return 0;
	}
#endif

	const char *fileListPath = "materials/models/player/teamkits/filelist.txt";

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
	Q_snprintf(url, sizeof(url), "%s/teamkits/filelist.txt.gz", cl_download_url.GetString());
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rcvFileListData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
	CURLcode result = curl_easy_perform(curl);
	long code;
	curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &code);
	curl_easy_cleanup(curl);

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
		Q_snprintf(filePath, sizeof(filePath), "%s/%s", TEAMKITS_PATH, serverFileList[i].path);

		char folderPath[256];
		Q_strncpy(folderPath, filePath, sizeof(folderPath));

		char *pos = strrchr(folderPath, '/');
		*pos = '\0';

		if (!filesystem->FileExists(folderPath, "MOD"))
			filesystem->CreateDirHierarchy(folderPath, "MOD");

		TeamKitCurl curlData;

		curlData.fh = filesystem->Open(filePath, "wb", "MOD");

		memset(&curlData.md5Ctx, 0, sizeof(MD5Context_t));
		MD5Init(&curlData.md5Ctx);

		curl = curl_easy_init();
		Q_snprintf(url, sizeof(url), "%s/teamkits/%s.gz", cl_download_url.GetString(), serverFileList[i].path);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rcvKitData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlData);
		CURLcode result = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &code);
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

	CTeamInfo::ParseTeamKits();

//#ifdef CLIENT_DLL
	//((vgui::MessageBox *)params)->CloseModal();
//#endif

	return 0;
}

void CTeamInfo::DownloadTeamKits()
{
//#ifdef CLIENT_DLL
	//vgui::MessageBox *pMessageBox = new vgui::MessageBox("Updating", "Downloading team kits. Please wait...", g_pClientMode->GetViewport());
	//pMessageBox->SetCloseButtonVisible( false ); 
	//pMessageBox->DoModal();
	//CreateSimpleThread(CheckTeamKits, pMessageBox);
//#else
	//CreateSimpleThread(CheckTeamKits, NULL);
//#endif
	CheckTeamKits(NULL);
}

void CTeamInfo::ParseTeamKits()
{
#ifdef CLIENT_DLL
	CTeamKitInfo::m_pDefaultFontAtlas = new CFontAtlas(DEFAULT_FONTATLAS_FOLDER);
#endif

	CTeamInfo::m_TeamInfo.PurgeAndDeleteElements();

	CUtlVector<FileInfo_t> teamFolders;
	FindFiles(TEAMKITS_PATH, teamFolders);

	for (int i = 0; i < teamFolders.Count(); i++)
	{
		if (!teamFolders[i].isDirectory)
			continue;

		CTeamInfo *pTeamInfo = new CTeamInfo();
		CTeamInfo::m_TeamInfo.AddToTail(pTeamInfo);
		Q_strncpy(pTeamInfo->m_szFolderName, teamFolders[i].name, sizeof(pTeamInfo->m_szFolderName));

		CUtlVector<FileInfo_t> teamFolderFiles;
		FindFiles(teamFolders[i].path, teamFolderFiles);

 		for (int j = 0; j < teamFolderFiles.Count(); j++)
		{
			if (!teamFolderFiles[j].isDirectory)
			{
				if (!Q_strcmp(teamFolderFiles[j].name, "teamcrest.vtf"))
				{
					pTeamInfo->m_bHasCrest = true;
				}
				else if (!Q_strcmp(teamFolderFiles[j].name, "teamdata.txt"))
				{
					KeyValues *pKV = new KeyValues("TeamData");
					pKV->LoadFromFile(filesystem, teamFolderFiles[j].path, "MOD");

					pTeamInfo->m_bIsClub = pKV->GetInt("IsClub", 0) != 0;
					pTeamInfo->m_bIsReal = pKV->GetInt("IsReal", 0) != 0;
					Q_strncpy(pTeamInfo->m_szCode, pKV->GetString("Code", "???"), sizeof(pTeamInfo->m_szCode));
					Q_strncpy(pTeamInfo->m_szShortName, pKV->GetString("ShortName", "???"), sizeof(pTeamInfo->m_szShortName));
					Q_strncpy(pTeamInfo->m_szFullName, pKV->GetString("FullName", "???"), sizeof(pTeamInfo->m_szFullName));

					pKV->deleteThis();
				}
			}
			else
			{
				CTeamKitInfo *pKitInfo = new CTeamKitInfo();
				pTeamInfo->m_TeamKitInfo.AddToTail(pKitInfo);
				pKitInfo->m_pTeamInfo = pTeamInfo;
				Q_strncpy(pKitInfo->m_szFolderName, teamFolderFiles[j].name, sizeof(pKitInfo->m_szFolderName));
				Q_strncpy(pKitInfo->m_szName, strtok(teamFolderFiles[j].name, "@"), sizeof(pKitInfo->m_szName));
				Q_strncpy(pKitInfo->m_szAuthor, strtok(NULL, "@"), sizeof(pKitInfo->m_szAuthor));

				CUtlVector<FileInfo_t> kitFiles;
				FindFiles(teamFolderFiles[j].path, kitFiles);
				
				bool hasNameAtlasBmp = false;
				bool hasNameAtlasXml = false;
				bool hasNumberAtlasBmp = false;
				bool hasNumberAtlasXml = false;

				for (int k = 0; k < kitFiles.Count(); k++)
				{
					if (kitFiles[k].isDirectory)
						continue;

					//DevMsg("%s\n", kitFiles[k].name);

					if (!Q_strcmp(kitFiles[k].name, "kitdata.txt"))
					{
						KeyValues *pKV = new KeyValues("KitData");
						pKV->LoadFromFile(filesystem, kitFiles[k].path, "MOD");

						Color c = pKV->GetColor("PrimaryColor");
						pKitInfo->m_PrimaryColor = Color(c.r(), c.g(), c.b(), 255);

						c = pKV->GetColor("SecondaryColor");
						pKitInfo->m_SecondaryColor = Color(c.r(), c.g(), c.b(), 255);

						pKitInfo->m_nOutfieldShirtNameOffset = pKV->GetInt("OutfieldNameOffset");
						c = pKV->GetColor("OutfieldNameColor");
						pKitInfo->m_OutfieldShirtNameColor = Color(c.r(), c.g(), c.b(), c.a());
						pKitInfo->m_nOutfieldShirtNumberOffset = pKV->GetInt("OutfieldNumberOffset");
						c = pKV->GetColor("OutfieldNumberColor");
						pKitInfo->m_OutfieldShirtNumberColor = Color(c.r(), c.g(), c.b(), c.a());

						pKitInfo->m_nKeeperShirtNameOffset = pKV->GetInt("KeeperNameOffset");
						c = pKV->GetColor("KeeperNameColor");
						pKitInfo->m_KeeperShirtNameColor = Color(c.r(), c.g(), c.b(), c.a());
						pKitInfo->m_nKeeperShirtNumberOffset = pKV->GetInt("KeeperNumberOffset");
						c = pKV->GetColor("KeeperNumberColor");
						pKitInfo->m_KeeperShirtNumberColor = Color(c.r(), c.g(), c.b(), c.a());

						//RGB primaryRgb = { pKitInfo->m_PrimaryColor.r(), pKitInfo->m_PrimaryColor.g(), pKitInfo->m_PrimaryColor.b() };
						//RGB secondaryRgb = { pKitInfo->m_SecondaryColor.r(), pKitInfo->m_SecondaryColor.g(), pKitInfo->m_SecondaryColor.b() };
						//HSL primaryHsl, secondaryHsl;
						//RGBtoHSL(primaryRgb, primaryHsl);
						//colors_t color = classify(primaryHsl);
						pKitInfo->m_HudColor = pKitInfo->m_PrimaryColor;//Color(substituteColors[color][0], substituteColors[color][1], substituteColors[color][2], 255);

						pKV->deleteThis();
					}
					else if (!Q_strcmp(kitFiles[k].name, "name_atlas.bmp"))
						hasNameAtlasBmp = true;
					else if (!Q_strcmp(kitFiles[k].name, "name_atlas.xml"))
						hasNameAtlasXml = true;
					else if (!Q_strcmp(kitFiles[k].name, "number_atlas.bmp"))
						hasNumberAtlasBmp = true;
					else if (!Q_strcmp(kitFiles[k].name, "number_atlas.xml"))
						hasNumberAtlasXml = true;
				}
				
#ifdef CLIENT_DLL
				if (hasNameAtlasBmp && hasNameAtlasXml && hasNumberAtlasBmp && hasNumberAtlasXml)
				{
					pKitInfo->m_pFontAtlas = new CFontAtlas(teamFolderFiles[j].path);
				}
				else
					pKitInfo->m_pFontAtlas = CTeamKitInfo::m_pDefaultFontAtlas;
#endif
			}
		}
	}
}

void CTeamInfo::GetNonClashingTeamKits(char *homeTeam, char *awayTeam, bool clubTeams, bool nationalTeams, bool realTeams, bool fictitiousTeams)
{
	int teamCount = m_TeamInfo.Count();
	int attemptCount = 0;
	int homeTeamIndex;
	int awayTeamIndex;

	do
	{
		attemptCount += 1;
		homeTeamIndex = g_IOSRand.RandomInt(0, teamCount - 1);
		awayTeamIndex = g_IOSRand.RandomInt(0, teamCount - 1);

		if (homeTeamIndex == awayTeamIndex)
			continue;

		if (m_TeamInfo[homeTeamIndex]->m_bIsClub && !clubTeams || !m_TeamInfo[homeTeamIndex]->m_bIsClub && !nationalTeams)
			continue;

		if (m_TeamInfo[homeTeamIndex]->m_bIsReal && !realTeams || !m_TeamInfo[homeTeamIndex]->m_bIsReal && !fictitiousTeams)
			continue;

		if (m_TeamInfo[awayTeamIndex]->m_bIsClub && !clubTeams || !m_TeamInfo[awayTeamIndex]->m_bIsClub && !nationalTeams)
			continue;

		if (m_TeamInfo[awayTeamIndex]->m_bIsReal && !realTeams || !m_TeamInfo[awayTeamIndex]->m_bIsReal && !fictitiousTeams)
			continue;

		if (m_TeamInfo[homeTeamIndex]->m_bIsClub != m_TeamInfo[awayTeamIndex]->m_bIsClub ||
			m_TeamInfo[homeTeamIndex]->m_bIsReal != m_TeamInfo[awayTeamIndex]->m_bIsReal)
			continue;

		//Msg("color distance: %f\n", ColorDistance(m_TeamKitInfoDatabase[homeTeamIndex]->m_PrimaryKitColor, m_TeamKitInfoDatabase[awayTeamIndex]->m_PrimaryKitColor));
		Q_snprintf(homeTeam, MAX_KITNAME_LENGTH, "%s/%s", m_TeamInfo[homeTeamIndex]->m_szFolderName, m_TeamInfo[homeTeamIndex]->m_TeamKitInfo[0]->m_szFolderName);
		Q_snprintf(awayTeam, MAX_KITNAME_LENGTH, "%s/%s", m_TeamInfo[awayTeamIndex]->m_szFolderName, m_TeamInfo[awayTeamIndex]->m_TeamKitInfo[0]->m_szFolderName);
		break;

	} while (attemptCount < 1000);
}

CTeamKitInfo *CTeamInfo::FindTeamByKitName(const char *name)
{
	char fullName[MAX_KITNAME_LENGTH];
	Q_strncpy(fullName, name, sizeof(fullName));
	char *teamName = strtok(fullName, "/");
	char *kitName = strtok(NULL, "/");

	for (int i = 0; i < m_TeamInfo.Count(); i++)
	{
		if (Q_strcmp(m_TeamInfo[i]->m_szFolderName, teamName))
			continue;

		for (int j = 0; j < m_TeamInfo[i]->m_TeamKitInfo.Count(); j++)
		{
			if (Q_strcmp(m_TeamInfo[i]->m_TeamKitInfo[j]->m_szFolderName, kitName))
				continue;

			return m_TeamInfo[i]->m_TeamKitInfo[j];
		}
	}

	return NULL;
}

CTeamKitInfo *CTeamInfo::FindTeamByShortName(const char *name)
{
	for (int i = 0; i < m_TeamInfo.Count(); i++)
	{
		if (!Q_strcmp(m_TeamInfo[i]->m_szShortName, name))
		{
			return m_TeamInfo[i]->m_TeamKitInfo[0];
		}
	}

	return NULL;
}

CTeamKitInfo *CTeamInfo::FindTeamByCode(const char *code)
{
	for (int i = 0; i < m_TeamInfo.Count(); i++)
	{
		if (!Q_strcmp(m_TeamInfo[i]->m_szCode, code))
		{
			return m_TeamInfo[i]->m_TeamKitInfo[0];
		}
	}

	return NULL;
}