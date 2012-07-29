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

char g_szCountryNamesTemp[COUNTRY_NAMES_COUNT][64] = { "Afghanistan", "African Union", "Aland", "Albania", "Alderney", "Algeria", "American Samoa", "Andorra", "Angola", "Anguilla", "Antarctica", "Antigua & Barbuda", "Arab League", "Argentina", "Armenia", "Aruba", "ASEAN", "Australia", "Austria", "Azerbaijan", "Bahamas", "Bahrain", "Bangladesh", "Barbados", "Basque Country", "Belarus", "Belgium", "Belize", "Benin", "Bermuda", "Bhutan", "Bolivia", "Bosnia & Herzegovina", "Botswana", "Bouvet", "Brazil", "British Indian Ocean Territory", "Brunei", "Bulgaria", "Burkina Faso", "Burundi", "Cambodja", "Cameroon", "Canada", "Cape Verde", "CARICOM", "Catalonia", "Cayman Islands", "Central African Republic", "Chad", "Chile", "China", "Christmas", "CIS", "Cocos (Keeling)", "Colombia", "Commonwealth", "Comoros", "Congo-Brazzaville", "Congo-Kinshasa(Zaire)", "Cook Islands", "Costa Rica", "Cote d'Ivoire", "Croatia", "Cuba", "Curacao", "Cyprus", "Czech Republic", "Denmark", "Djibouti", "Dominica", "Dominican Republic", "Ecuador", "Egypt", "El Salvador", "England", "Equatorial Guinea", "Eritrea", "Estonia", "Ethiopia", "European Union", "Falkland (Malvinas)", "FAO", "Faroes", "Fiji", "Finland", "France", "French Southern and Antarctic Lands", "French-Guiana", "Gabon", "Galicia", "Gambia", "Georgia", "Germany", "Ghana", "Gibraltar", "Greece", "Greenland", "Grenada", "Guadeloupe", "Guam", "Guatemala", "Guernsey", "Guinea-Bissau", "Guinea", "Guyana", "Haiti", "Heard Island and McDonald", "Honduras", "Hong Kong", "Hungary", "IAEA", "Iceland", "IHO", "India", "Indonezia", "Iran", "Iraq", "Ireland", "Islamic Conference", "Isle of Man", "Israel", "Italy", "Jamaica", "Japan", "Jersey", "Jordan", "Kazakhstan", "Kenya", "Kiribati", "Kosovo", "Kuwait", "Kyrgyzstan", "Laos", "Latvia", "Lebanon", "Lesotho", "Liberia", "Libya", "Liechtenshein", "Lithuania", "Luxembourg", "Macao", "Macedonia", "Madagascar", "Malawi", "Malaysia", "Maldives", "Mali", "Malta", "Marshall Islands", "Martinique", "Mauritania", "Mauritius", "Mayotte", "Mexico", "Micronesia", "Moldova", "Monaco", "Mongolia", "Montenegro", "Montserrat", "Morocco", "Mozambique", "Myanmar(Burma)", "Namibia", "NATO", "Nauru", "Nepal", "Netherlands Antilles", "Netherlands", "New Caledonia", "New Zealand", "Nicaragua", "Niger", "Nigeria", "Niue", "Norfolk", "North Korea", "Northern Cyprus", "Northern Ireland", "Northern Mariana", "Norway", "OAS", "OECD", "Olimpic Movement", "Oman", "OPEC", "Pakistan", "Palau", "Palestine", "Panama", "Papua New Guinea", "Paraguay", "Peru", "Philippines", "Pitcairn", "Poland", "Portugal", "Puerto Rico", "Qatar", "Red Cross", "Reunion", "Romania", "Russian Federation", "Rwanda", "Saint Barthelemy", "Saint Helena", "Saint Lucia", "Saint Martin", "Saint Pierre and Miquelon", "Samoa", "San Marino", "Sao Tome & Principe", "Saudi Arabia", "Scotland", "Senegal", "Serbia(Yugoslavia)", "Seychelles", "Sierra Leone", "Singapore", "Sint-Maarten", "Slovakia", "Slovenia", "Solomon Islands", "Somalia", "Somaliland", "South Africa", "South Georgia and South Sandwich", "South Korea", "Southern-Sudan", "Spain", "Sri Lanka", "St Kitts & Nevis", "St Vincent & the Grenadines", "Sudan", "Suriname", "Svalbard and Jan Mayen", "Swaziland", "Sweden", "Switzerland", "Syria", "Tahiti(French Polinesia)", "Taiwan", "Tajikistan", "Tanzania", "Thailand", "Timor-Leste", "Togo", "Tokelau", "Tonga", "Trinidad & Tobago", "Tristan-da-Cunha", "Tunisia", "Turkey", "Turkmenistan", "Turks and Caicos Islands", "Tuvalu", "Uganda", "Ukraine", "UNESCO", "UNICEF", "United Arab Emirates", "United Kingdom(Great Britain)", "United Nations", "United States Minor Outlying", "United States of America(USA)", "Uruguay", "Uzbekistan", "Vanutau", "Vatican City", "Venezuela", "Viet Nam", "Virgin Islands British", "Virgin Islands US", "Wales", "Wallis and Futuna", "Western Sahara", "WHO", "WTO", "Yemen", "Zambia", "Zimbabwe" };

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSettingsMenu::CSettingsMenu(Panel *parent, const char *name) : Panel(parent, name)
{
	SetScheme("ClientScheme");

	m_pPlayerNameLabel = new Label(this, "", "Player Name:");
	m_pPlayerNameText = new TextEntry(this, "");
	m_pClubNameLabel = new Label(this, "", "Club Name:");
	m_pClubNameText = new TextEntry(this, "");
	m_pCountryNameLabel = new Label(this, "", "Country Name:");
	m_pCountryNameList = new ComboBox(this, "", COUNTRY_NAMES_COUNT, false);
	m_pCountryNameList->SetOpenDirection(Menu::DOWN);

	m_pSaveButton = new Button(this, "", "Save Settings");

	m_flNextUpdateTime = gpGlobals->curtime;
}

void CSettingsMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScheme = pScheme;
}

void CSettingsMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	m_flNextUpdateTime = gpGlobals->curtime;

	m_pPlayerNameLabel->SetBounds(0, 0, LABEL_WIDTH, TEXT_HEIGHT);
	m_pPlayerNameText->SetBounds(LABEL_WIDTH, 0, INPUT_WIDTH, TEXT_HEIGHT);
	//m_pNameText->SetEditable(true);
	//m_pNameText->SetEnabled(true);
	m_pPlayerNameText->AddActionSignalTarget( this );
	m_pPlayerNameText->SendNewLine(true); // with the txtEntry Type you need to set it to pass the return key as a message
	m_pPlayerNameText->SetFgColor(Color(0, 0, 0, 255));

	m_pClubNameLabel->SetBounds(0, TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pClubNameText->SetBounds(LABEL_WIDTH, TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pClubNameText->SetFgColor(Color(0, 0, 0, 255));

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
	m_pCountryNameList->SetDisabledBgColor(Color(255, 255, 255, 255));
	m_pCountryNameList->SetSelectionTextColor(Color(0, 0, 0, 255));

	m_pSaveButton->SetBounds(LABEL_WIDTH, 4 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pSaveButton->SetCommand("save_settings");
	m_pSaveButton->AddActionSignalTarget(this);
	m_pSaveButton->SetDefaultColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
	m_pSaveButton->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
	m_pSaveButton->SetDepressedColor(Color(100, 100, 100, 255), Color(200, 200, 200, 255));
	m_pSaveButton->SetCursor(dc_hand);
	m_pSaveButton->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
	m_pSaveButton->SetContentAlignment(Label::a_center);
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

	//Update();

	m_flNextUpdateTime = gpGlobals->curtime + 0.25f;
}

void CSettingsMenu::Update()
{
	m_pPlayerNameText->SetText(g_pCVar->FindVar("playername")->GetString());
	m_pCountryNameList->SetText(g_pCVar->FindVar("countryname")->GetString());
	m_pClubNameText->SetText(g_pCVar->FindVar("clubname")->GetString());
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
		engine->ClientCmd(VarArgs("playername \"%s\"", text));
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

void CSettingsMenu::Reset()
{
	m_pCountryNameList->RemoveAll();

	for (int i = 0; i < COUNTRY_NAMES_COUNT; i++)
	{
		KeyValues *kv = new KeyValues("UserData", "index", i);
		m_pCountryNameList->AddItem(g_szCountryNames[i], kv);
		kv->deleteThis();
	}
}