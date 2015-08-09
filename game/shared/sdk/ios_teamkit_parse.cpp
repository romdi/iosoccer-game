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


#ifdef CLIENT_DLL

void CC_ReloadTeamKits(const CCommand &args)
{
	CTeamInfo::ParseTeamKits();
	// For some reason we have to do this twice to prevent kit swapping
	CTeamInfo::ParseTeamKits();
}

static ConCommand reloadteamkits("reloadteamkits", CC_ReloadTeamKits);

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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

// https://www.google.com/design/spec/style/color.html#color-color-palette
Color colors[COLOR_COUNT] = {
	Color(255, 255, 255, 255),		// black => white
	Color(255, 255, 255, 255),		// white
	Color(238, 238, 238, 255),		// gray
	Color(239, 154, 154, 255),		// red
	Color(255, 245, 157, 255),		// yellow
	Color(165, 214, 167, 255),		// green
	Color(128, 222, 234, 255),		// cyan
	Color(144, 202, 249, 255),		// blue
	Color(244, 143, 177, 255)		// magenta
};

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

float CTeamInfo::m_flLastUpdateTime = 0;

CFontAtlas *CTeamKitInfo::m_pDefaultFontAtlas = NULL;

CTeamKitInfo::CTeamKitInfo()
{
	m_szName[0] = 0;
	m_szAuthor[0] = 0;
	m_szFolderName[0] = 0;
	m_HudColor = Color(0, 255, 0, 255);
	m_PrimaryColor = Color(0, 255, 0, 255);
	m_SecondaryColor = Color(0, 255, 0, 255);


	m_OutfieldShirtNameFillColor = Color(0, 255, 0, 255);
	m_OutfieldShirtNameOutlineColor = Color(0, 255, 0, 255);
	m_nOutfieldShirtNameVerticalOffset = 0;

	m_OutfieldShirtBackNumberFillColor = Color(0, 255, 0, 255);
	m_OutfieldShirtBackNumberOutlineColor = Color(0, 255, 0, 255);
	m_nOutfieldShirtBackNumberVerticalOffset = 0;

	m_OutfieldShortsNumberFillColor = Color(0, 255, 0, 255);
	m_OutfieldShortsNumberOutlineColor = Color(0, 255, 0, 255);
	m_nOutfieldShortsNumberHorizontalOffset = 0;
	m_nOutfieldShortsNumberVerticalOffset = 0;

	m_bHasOutfieldShirtFrontNumber = false;
	m_OutfieldShirtFrontNumberFillColor = Color(0, 255, 0, 255);
	m_OutfieldShirtFrontNumberOutlineColor = Color(0, 255, 0, 255);
	m_nOutfieldShirtFrontNumberHorizontalOffset = 0;
	m_nOutfieldShirtFrontNumberVerticalOffset = 0;


	m_KeeperShirtNameFillColor = Color(0, 255, 0, 255);
	m_KeeperShirtNameOutlineColor = Color(0, 255, 0, 255);
	m_nKeeperShirtNameVerticalOffset = 0;

	m_KeeperShirtBackNumberFillColor = Color(0, 255, 0, 255);
	m_KeeperShirtBackNumberOutlineColor = Color(0, 255, 0, 255);
	m_nKeeperShirtBackNumberVerticalOffset = 0;

	m_KeeperShortsNumberFillColor = Color(0, 255, 0, 255);
	m_KeeperShortsNumberOutlineColor = Color(0, 255, 0, 255);
	m_nKeeperShortsNumberHorizontalOffset = 0;
	m_nKeeperShortsNumberVerticalOffset = 0;

	m_bHasKeeperShirtFrontNumber = false;
	m_KeeperShirtFrontNumberFillColor = Color(0, 255, 0, 255);
	m_KeeperShirtFrontNumberOutlineColor = Color(0, 255, 0, 255);
	m_nKeeperShirtFrontNumberHorizontalOffset = 0;
	m_nKeeperShirtFrontNumberVerticalOffset = 0;


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

CFontAtlas::CFontAtlas(const char *folderPath, bool hasShirtAndShortsNumberAtlas)
{
	m_ShirtNamePixels = ParseInfo(folderPath, "shirt_back_name", m_NameChars, sizeof(m_NameChars), m_nNamePixelsWidth, m_nNamePixelsHeight);
	m_ShirtBackNumberPixels = ParseInfo(folderPath, "shirt_back_number", m_ShirtBackNumberChars, sizeof(m_ShirtBackNumberChars), m_nShirtBackNumberPixelsWidth, m_nShirtBackNumberPixelsHeight);

	if (hasShirtAndShortsNumberAtlas)
		m_ShirtAndShortsNumberPixels = ParseInfo(folderPath, "shirt_and_shorts_front_number", m_ShirtAndShortsNumberChars, sizeof(m_ShirtAndShortsNumberChars), m_nShirtAndShortsNumberPixelsWidth, m_nShirtAndShortsNumberPixelsHeight);
	else
		m_ShirtAndShortsNumberPixels = NULL;
}

glyphWithOutline_t **CFontAtlas::ParseInfo(const char *folderPath, const char *type, chr_t *chars, int maxChars, int &width, int &height)
{
	char path[128];

	Q_snprintf(path, sizeof(path), "%s/%s_atlas.tga", folderPath, type);

	if (!filesystem->FileExists(path, "MOD"))
		return NULL;

    FileHandle_t fh = filesystem->Open(path, "rb", "MOD");
	int file_len = filesystem->Size(fh);

    unsigned char* fulldata = new unsigned char[file_len];
    filesystem->Read(fulldata, file_len, fh);
	const int pixelStart = 18;

	width = *(short*)&fulldata[12];
    height = *(short*)&fulldata[14];

	glyphWithOutline_t **pixels = new glyphWithOutline_t *[height];
	for (int i = 0; i < height; i++)
		pixels[i] = new glyphWithOutline_t[width];

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
			int pos = pixelStart + y * width * 4 + x * 4;
			Assert(fulldata[pos + 0] == fulldata[pos + 1] && fulldata[pos + 0] == fulldata[pos + 2]);
			pixels[y][x].glyph = fulldata[pos + 0];
			pixels[y][x].outline = fulldata[pos + 3];
        }
    }

	delete[] fulldata;
    filesystem->Close(fh);

	Q_snprintf(path, sizeof(path), "%s/%s_atlas.txt", folderPath, type);
    fh = filesystem->Open(path, "rb", "MOD");

	file_len = filesystem->Size(fh);
	char *fulltext = new char[file_len];
    filesystem->Read(fulltext, file_len, fh);

	char *pch;
	pch = strtok(fulltext, "\n");

	while (pch != NULL)
	{
		if (!Q_strncmp(pch, "char ", 5))
		{
			int glyph = clamp(atoi(strstr(pch, "id=") + 3), 0, maxChars - 1);
			chr_t &chr = chars[glyph];
			chr.glyph = glyph;
			chr.x = atoi(strstr(pch, "x=") + 2);
			chr.y = atoi(strstr(pch, "y=") + 2);
			chr.width = atoi(strstr(pch, "width=") + 6);
			chr.height = atoi(strstr(pch, "height=") + 7);
			chr.offsetX = atoi(strstr(pch, "xoffset=") + 8);
			chr.offsetY = atoi(strstr(pch, "yoffset=") + 8);
			chr.advanceX = atoi(strstr(pch, "xadvance=") + 9);
		}
		else if (!Q_strncmp(pch, "kerning ", 8))
		{
			int first = atoi(strstr(pch, "first=") + 6);
			int second = atoi(strstr(pch, "second=") + 7);
			int amount = atoi(strstr(pch, "amount=") + 7);
			chars[second].kernings.AddToTail(kerning_t(first, amount));
		}

		pch = strtok(NULL, "\n");
	}

	delete[] fulltext;
    filesystem->Close(fh);

	return pixels;
}

void CTeamInfo::ParseTeamKits()
{
#ifdef CLIENT_DLL
	CTeamKitInfo::m_pDefaultFontAtlas = new CFontAtlas(DEFAULT_FONTATLAS_FOLDER, true);
#endif

	CTeamInfo::m_TeamInfo.PurgeAndDeleteElements();

	CUtlVector<FileInfo_t> teamFolders;
	FindFiles(TEAMKITS_PATH, teamFolders);

	for (int i = 0; i < teamFolders.Count(); i++)
	{
		if (!teamFolders[i].isDirectory)
			continue;

		CTeamInfo *pTeamInfo = new CTeamInfo();
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
				pKitInfo->m_pTeamInfo = pTeamInfo;
				Q_strncpy(pKitInfo->m_szFolderName, teamFolderFiles[j].name, sizeof(pKitInfo->m_szFolderName));

				CUtlVector<FileInfo_t> kitFiles;
				FindFiles(teamFolderFiles[j].path, kitFiles);
				
				bool hasBackNameAtlasTga = false;
				bool hasBackNameAtlasTxt = false;
				bool hasBackNumberAtlasTga = false;
				bool hasBackNumberAtlasTxt = false;
				bool hasShirtAndShortsNumberAtlasTga = false;
				bool hasShirtAndShortsNumberAtlasTxt = false;

				for (int k = 0; k < kitFiles.Count(); k++)
				{
					if (kitFiles[k].isDirectory)
						continue;

					if (!Q_strcmp(kitFiles[k].name, "kitdata.txt"))
					{
						KeyValues *pKV = new KeyValues("KitData");
						pKV->LoadFromFile(filesystem, kitFiles[k].path, "MOD");

						Q_strncpy(pKitInfo->m_szName, pKV->GetString("Name", "???"), sizeof(pKitInfo->m_szName));
						Q_strncpy(pKitInfo->m_szAuthor, pKV->GetString("Author", "???"), sizeof(pKitInfo->m_szAuthor));

						Color c = pKV->GetColor("PrimaryColor");
						pKitInfo->m_PrimaryColor = Color(c.r(), c.g(), c.b(), 255);

						c = pKV->GetColor("SecondaryColor");
						pKitInfo->m_SecondaryColor = Color(c.r(), c.g(), c.b(), 255);

						for (KeyValues *pTypeKey = pKV->GetFirstTrueSubKey(); pTypeKey; pTypeKey = pTypeKey->GetNextTrueSubKey())
						{
							if (!Q_stricmp(pTypeKey->GetName(), "Outfield"))
							{
								pKitInfo->m_bHasOutfieldShirtFrontNumber = false;

								for (KeyValues *pPosKey = pTypeKey->GetFirstTrueSubKey(); pPosKey; pPosKey = pPosKey->GetNextTrueSubKey())
								{
									if (!Q_stricmp(pPosKey->GetName(), "ShirtName"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_OutfieldShirtNameFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_OutfieldShirtNameOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nOutfieldShirtNameVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShirtBackNumber"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_OutfieldShirtBackNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_OutfieldShirtBackNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nOutfieldShirtBackNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShortsNumber"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_OutfieldShortsNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_OutfieldShortsNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nOutfieldShortsNumberHorizontalOffset = pPosKey->GetInt("HorizontalOffset");
										pKitInfo->m_nOutfieldShortsNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShirtFrontNumber"))
									{
										pKitInfo->m_bHasOutfieldShirtFrontNumber = true;
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_OutfieldShirtFrontNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_OutfieldShirtFrontNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nOutfieldShirtFrontNumberHorizontalOffset = pPosKey->GetInt("HorizontalOffset");
										pKitInfo->m_nOutfieldShirtFrontNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
								}
							}
							else if (!Q_stricmp(pTypeKey->GetName(), "Keeper"))
							{
								pKitInfo->m_bHasKeeperShirtFrontNumber = false;

								for (KeyValues *pPosKey = pTypeKey->GetFirstTrueSubKey(); pPosKey; pPosKey = pPosKey->GetNextTrueSubKey())
								{
									if (!Q_stricmp(pPosKey->GetName(), "ShirtName"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_KeeperShirtNameFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_KeeperShirtNameOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nKeeperShirtNameVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShirtBackNumber"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_KeeperShirtBackNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_KeeperShirtBackNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nKeeperShirtBackNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShortsNumber"))
									{
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_KeeperShortsNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_KeeperShortsNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nKeeperShortsNumberHorizontalOffset = pPosKey->GetInt("HorizontalOffset");
										pKitInfo->m_nKeeperShortsNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
									else if (!Q_stricmp(pPosKey->GetName(), "ShirtFrontNumber"))
									{
										pKitInfo->m_bHasKeeperShirtFrontNumber = true;
										c = pPosKey->GetColor("FillColor");
										pKitInfo->m_KeeperShirtFrontNumberFillColor = Color(c.r(), c.g(), c.b(), c.a());
										c = pPosKey->GetColor("OutlineColor");
										pKitInfo->m_KeeperShirtFrontNumberOutlineColor = Color(c.r(), c.g(), c.b(), c.a());
										pKitInfo->m_nKeeperShirtFrontNumberHorizontalOffset = pPosKey->GetInt("HorizontalOffset");
										pKitInfo->m_nKeeperShirtFrontNumberVerticalOffset = pPosKey->GetInt("VerticalOffset");
									}
								}
							}

						}

						RGB primaryRgb = { pKitInfo->m_PrimaryColor.r(), pKitInfo->m_PrimaryColor.g(), pKitInfo->m_PrimaryColor.b() };
						RGB secondaryRgb = { pKitInfo->m_SecondaryColor.r(), pKitInfo->m_SecondaryColor.g(), pKitInfo->m_SecondaryColor.b() };
						HSL primaryHsl, secondaryHsl;
						RGBtoHSL(primaryRgb, primaryHsl);
						colors_t color = classify(primaryHsl);
						pKitInfo->m_HudColor = colors[color];

						pKV->deleteThis();
					}
					else if (!Q_strcmp(kitFiles[k].name, "shirt_back_name_atlas.tga"))
						hasBackNameAtlasTga = true;
					else if (!Q_strcmp(kitFiles[k].name, "shirt_back_name_atlas.txt"))
						hasBackNameAtlasTxt = true;
					else if (!Q_strcmp(kitFiles[k].name, "shirt_back_number_atlas.tga"))
						hasBackNumberAtlasTga = true;
					else if (!Q_strcmp(kitFiles[k].name, "shirt_back_number_atlas.txt"))
						hasBackNumberAtlasTxt = true;
					else if (!Q_strcmp(kitFiles[k].name, "shirt_and_shorts_front_number_atlas.tga"))
						hasShirtAndShortsNumberAtlasTga = true;
					else if (!Q_strcmp(kitFiles[k].name, "shirt_and_shorts_front_number_atlas.txt"))
						hasShirtAndShortsNumberAtlasTxt = true;
				}
				
#ifdef CLIENT_DLL
				if (hasBackNameAtlasTga && hasBackNameAtlasTxt && hasBackNumberAtlasTga && hasBackNumberAtlasTxt)
				{
					pKitInfo->m_pFontAtlas = new CFontAtlas(teamFolderFiles[j].path, hasShirtAndShortsNumberAtlasTga && hasShirtAndShortsNumberAtlasTxt);
				}
				else
					pKitInfo->m_pFontAtlas = CTeamKitInfo::m_pDefaultFontAtlas;
#endif
				
				pTeamInfo->m_TeamKitInfo.AddToTail(pKitInfo);
			}
		}

		CTeamInfo::m_TeamInfo.AddToTail(pTeamInfo);
	}

	m_flLastUpdateTime = gpGlobals->curtime;
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

CUtlVector<CBallInfo *> CBallInfo::m_BallInfo;
float CBallInfo::m_flLastUpdateTime = 0;

void CBallInfo::ParseBallSkins()
{
	CBallInfo::m_BallInfo.PurgeAndDeleteElements();

	CUtlVector<FileInfo_t> ballFolders;
	FindFiles("materials/models/ball/skins/", ballFolders);

	for (int i = 0; i < ballFolders.Count(); i++)
	{
		if (!ballFolders[i].isDirectory)
			continue;

		CBallInfo *pBallInfo = new CBallInfo();
		Q_strncpy(pBallInfo->m_szFolderName, ballFolders[i].name, sizeof(pBallInfo->m_szFolderName));

		CUtlVector<FileInfo_t> ballFolderFiles;
		FindFiles(ballFolders[i].path, ballFolderFiles);

 		for (int j = 0; j < ballFolderFiles.Count(); j++)
		{
			if (ballFolderFiles[j].isDirectory)
				continue;

			if (!Q_strcmp(ballFolderFiles[j].name, "balldata.txt"))
			{
				KeyValues *pKV = new KeyValues("BallData");
				pKV->LoadFromFile(filesystem, ballFolderFiles[j].path, "MOD");

				Q_strncpy(pBallInfo->m_szName, pKV->GetString("Name", "???"), sizeof(pBallInfo->m_szName));
				Q_strncpy(pBallInfo->m_szAuthor, pKV->GetString("Author", "???"), sizeof(pBallInfo->m_szAuthor));

				pKV->deleteThis();
			}
		}

		CBallInfo::m_BallInfo.AddToTail(pBallInfo);
	}

	m_flLastUpdateTime = gpGlobals->curtime;
}

CUtlVector<CPitchInfo *> CPitchInfo::m_PitchInfo;

void CPitchInfo::ParsePitchTextures()
{
	CPitchInfo::m_PitchInfo.PurgeAndDeleteElements();

	CUtlVector<FileInfo_t> pitchFolders;
	FindFiles("materials/pitch/textures", pitchFolders);

	for (int i = 0; i < pitchFolders.Count(); i++)
	{
		if (!pitchFolders[i].isDirectory)
			continue;

		CPitchInfo *pPitchInfo = new CPitchInfo();
		Q_strncpy(pPitchInfo->m_szFolderName, pitchFolders[i].name, sizeof(pPitchInfo->m_szFolderName));

		CUtlVector<FileInfo_t> pitchFolderFiles;
		FindFiles(pitchFolders[i].path, pitchFolderFiles);

		for (int j = 0; j < pitchFolderFiles.Count(); j++)
		{
			if (pitchFolderFiles[j].isDirectory)
				continue;

			if (!Q_strcmp(pitchFolderFiles[j].name, "pitchdata.txt"))
			{
				KeyValues *pKV = new KeyValues("PitchData");
				pKV->LoadFromFile(filesystem, pitchFolderFiles[j].path, "MOD");

				Q_strncpy(pPitchInfo->m_szName, pKV->GetString("Name", "???"), sizeof(pPitchInfo->m_szName));
				Q_strncpy(pPitchInfo->m_szAuthor, pKV->GetString("Author", "???"), sizeof(pPitchInfo->m_szAuthor));
				pPitchInfo->m_nType = pKV->GetInt("Type", 0);

				pKV->deleteThis();
			}
		}

		CPitchInfo::m_PitchInfo.AddToTail(pPitchInfo);
	}
}