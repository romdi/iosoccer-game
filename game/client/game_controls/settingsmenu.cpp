//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>

#include "settingsmenu.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <FileSystem.h>

#include <vgui_controls/RichText.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>
#include <vgui_controls/TextEntry.h>

#include "IGameUIFuncs.h" // for key bindings
#include <igameresources.h>

#include "vgui_bitmapbutton.h"	//ios

#include <game/client/iviewport.h>
#include <stdlib.h> // MAX_PATH define
#include <stdio.h>
#include "byteswap.h"

#include "c_ball.h"
#include "c_team.h"
#include "sdk_backgroundpanel.h"
#include "sdk_gamerules.h"
#include "c_sdk_player.h"
#include "steam/steam_api.h"

#include "materialsystem/itexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define LABEL_WIDTH 150
#define INPUT_WIDTH 300
#define TEXT_HEIGHT 30

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSettingsMenu::CSettingsMenu(Panel *parent, const char *name) : Panel(parent, name)
{
	m_pPlayerNameLabel = new Label(this, "", "Player Name:");
	m_pPlayerNameText = new TextEntry(this, "");
	m_pClubNameLabel = new Label(this, "", "Club Name:");
	m_pClubNameText = new TextEntry(this, "");
	m_pCountryNameLabel = new Label(this, "", "Country Name:");
	m_pCountryNameList = new ComboBox(this, "", COUNTRY_NAMES_COUNT, false);
	m_pCountryNameList->SetOpenDirection(Menu::DOWN);

	m_pSaveButton = new Button(this, "", "Save Settings");

	SetBgColor(Color(0, 0, 0, 245));
	SetPaintBackgroundEnabled(true);
	SetPaintBackgroundType(2);
	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );

	m_pPlayerNameLabel->SetBounds(0, 0, LABEL_WIDTH, TEXT_HEIGHT);
	m_pPlayerNameText->SetBounds(LABEL_WIDTH, 0, INPUT_WIDTH, TEXT_HEIGHT);
	//m_pNameText->SetEditable(true);
	//m_pNameText->SetEnabled(true);
	m_pPlayerNameText->AddActionSignalTarget( this );
	m_pPlayerNameText->SendNewLine(true); // with the txtEntry Type you need to set it to pass the return key as a message

	m_pClubNameLabel->SetBounds(0, TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pClubNameText->SetBounds(LABEL_WIDTH, TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);

	m_pCountryNameLabel->SetBounds(0, 2 * TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pCountryNameList->SetBounds(LABEL_WIDTH, 2 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	//m_pCountryNameList->GetMenu()->AddActionSignalTarget(this);
	m_pCountryNameList->GetMenu()->MakeReadyForUse();
	m_pCountryNameList->GetMenu()->SetFgColor(Color(0, 0, 0, 255));
	m_pCountryNameList->GetMenu()->SetBgColor(Color(255, 255, 255, 255));
	m_pCountryNameList->SetSelectionUnfocusedBgColor(Color(255, 0, 0, 255));
	m_pCountryNameList->SetSelectionBgColor(Color(255, 255, 0, 255));
	m_pCountryNameList->SetFgColor(Color(0, 0, 0, 255));
	m_pCountryNameList->SetBgColor(Color(255, 255, 255, 255));
	m_pCountryNameList->DeleteAllItems();

	for (int i = 0; i < COUNTRY_NAMES_COUNT; i++)
	{
		KeyValues *kv = new KeyValues("UserData", "index", i);
		m_pCountryNameList->AddItem(g_szCountryNames[i], kv);
		kv->deleteThis();
	}

	m_pSaveButton->SetBounds(LABEL_WIDTH, 4 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pSaveButton->SetCommand("save_settings");
	m_pSaveButton->AddActionSignalTarget(this);

	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();

	if (!pLocal)
		return;

	m_pPlayerNameText->SetText(pLocal->GetPlayerName());
}

void CSettingsMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	return;

	SetBgColor(Color(0, 0, 0, 245));
	SetPaintBackgroundEnabled(true);
	SetPaintBackgroundType(2);
	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );

	m_pPlayerNameLabel->SetBounds(0, 0, LABEL_WIDTH, TEXT_HEIGHT);
	m_pPlayerNameText->SetBounds(INPUT_WIDTH, 0, INPUT_WIDTH, TEXT_HEIGHT);
	//m_pNameText->SetEditable(true);
	//m_pNameText->SetEnabled(true);
	m_pPlayerNameText->AddActionSignalTarget( this );
	m_pPlayerNameText->SendNewLine(true); // with the txtEntry Type you need to set it to pass the return key as a message

	m_pClubNameLabel->SetBounds(0, TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pClubNameText->SetBounds(LABEL_WIDTH, TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);

	m_pCountryNameLabel->SetBounds(0, 2 * TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pCountryNameList->SetBounds(LABEL_WIDTH, 2 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	//m_pCountryNameList->GetMenu()->AddActionSignalTarget(this);
	m_pCountryNameList->DeleteAllItems();

	for (int i = 0; i < COUNTRY_NAMES_COUNT; i++)
	{
		KeyValues *kv = new KeyValues("UserData", "index", i);
		m_pCountryNameList->AddItem(g_szCountryNames[i], kv);
		kv->deleteThis();
	}

	m_pSaveButton->SetBounds(LABEL_WIDTH, 4 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pSaveButton->SetCommand("save_settings");
	m_pSaveButton->AddActionSignalTarget(this);

	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();

	if (!pLocal)
		return;

	m_pPlayerNameText->SetText(pLocal->GetPlayerName());
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSettingsMenu::~CSettingsMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: updates the UI with a new map name and map html page, and sets up the team buttons
//-----------------------------------------------------------------------------
void CSettingsMenu::OnThink()
{
	//RequestFocus();

	if (m_flNextUpdateTime > gpGlobals->curtime)
		return;

	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// IOS Added
//-----------------------------------------------------------------------------
void CSettingsMenu::OnCommand( char const *cmd )
{
	if (!strcmp(cmd, "save_settings"))
	{
		char text[64];
		m_pPlayerNameText->GetText(text, sizeof(text));
		engine->ClientCmd(VarArgs("setinfo name \"%s\"", text));
		m_pClubNameText->GetText(text, sizeof(text));
		engine->ClientCmd(VarArgs("clubname \"%s\"", text));
		KeyValues *kv = m_pCountryNameList->GetActiveItemUserData();
		//kv->GetInt("index")
		engine->ClientCmd(VarArgs("countryname \"%s\"", g_szCountryNames[kv->GetInt("index")]));
	}

	BaseClass::OnCommand(cmd);

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CSettingsMenu::NewLineMessage(KeyValues* data)
{
	// when the txtEntry box posts an action signal it only sets the name
	// so it is wise to check whether that specific txtEntry is indeed in focus
	// Is the Text Entry box in focus?
	if (m_pPlayerNameText->HasFocus())
	{
	// We have caught the message, now we can handle it. I would simply repost it to the OnCommand function 
	// Post the message to our command handler
		this->OnCommand("save_settings");
	}
}
