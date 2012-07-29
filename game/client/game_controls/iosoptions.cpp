#include "cbase.h"
#include "iosoptions.h"
#include "ienginevgui.h"

class CIOSOptionsMenu : public IIOSOptionsMenu
{
private:
	CIOSOptionsPanel *m_pIOSOptionsPanel;
	vgui::VPANEL m_hParent;
 
public:
	CIOSOptionsMenu( void )
	{
		m_pIOSOptionsPanel = NULL;
	}
 
	void Create( vgui::VPANEL parent )
	{
		// Create immediately
		m_pIOSOptionsPanel = new CIOSOptionsPanel(parent);
	}
 
	void Destroy( void )
	{
		if ( m_pIOSOptionsPanel )
		{
			m_pIOSOptionsPanel->SetParent( (vgui::Panel *)NULL );
			delete m_pIOSOptionsPanel;
		}
	}
	CIOSOptionsPanel *GetPanel()
	{
		return m_pIOSOptionsPanel;
	}
};

static CIOSOptionsMenu g_IOSOptionsMenu;
IIOSOptionsMenu *iosOptionsMenu = (IIOSOptionsMenu *)&g_IOSOptionsMenu;

void CC_IOSOptionsMenu(const CCommand &args)
{
	if (!iosOptionsMenu->GetPanel()->IsVisible())
		iosOptionsMenu->GetPanel()->Activate();
	else
		iosOptionsMenu->GetPanel()->Close();
}

ConCommand iosoptionsmenu("iosoptionsmenu", CC_IOSOptionsMenu);



#define LABEL_WIDTH 150
#define INPUT_WIDTH 300
#define TEXT_HEIGHT 30

char g_szCountryNames[COUNTRY_NAMES_COUNT][64] = { "Afghanistan", "African Union", "Aland", "Albania", "Alderney", "Algeria", "American Samoa", "Andorra", "Angola", "Anguilla", "Antarctica", "Antigua & Barbuda", "Arab League", "Argentina", "Armenia", "Aruba", "ASEAN", "Australia", "Austria", "Azerbaijan", "Bahamas", "Bahrain", "Bangladesh", "Barbados", "Basque Country", "Belarus", "Belgium", "Belize", "Benin", "Bermuda", "Bhutan", "Bolivia", "Bosnia & Herzegovina", "Botswana", "Bouvet", "Brazil", "British Indian Ocean Territory", "Brunei", "Bulgaria", "Burkina Faso", "Burundi", "Cambodja", "Cameroon", "Canada", "Cape Verde", "CARICOM", "Catalonia", "Cayman Islands", "Central African Republic", "Chad", "Chile", "China", "Christmas", "CIS", "Cocos (Keeling)", "Colombia", "Commonwealth", "Comoros", "Congo-Brazzaville", "Congo-Kinshasa(Zaire)", "Cook Islands", "Costa Rica", "Cote d'Ivoire", "Croatia", "Cuba", "Curacao", "Cyprus", "Czech Republic", "Denmark", "Djibouti", "Dominica", "Dominican Republic", "Ecuador", "Egypt", "El Salvador", "England", "Equatorial Guinea", "Eritrea", "Estonia", "Ethiopia", "European Union", "Falkland (Malvinas)", "FAO", "Faroes", "Fiji", "Finland", "France", "French Southern and Antarctic Lands", "French-Guiana", "Gabon", "Galicia", "Gambia", "Georgia", "Germany", "Ghana", "Gibraltar", "Greece", "Greenland", "Grenada", "Guadeloupe", "Guam", "Guatemala", "Guernsey", "Guinea-Bissau", "Guinea", "Guyana", "Haiti", "Heard Island and McDonald", "Honduras", "Hong Kong", "Hungary", "IAEA", "Iceland", "IHO", "India", "Indonezia", "Iran", "Iraq", "Ireland", "Islamic Conference", "Isle of Man", "Israel", "Italy", "Jamaica", "Japan", "Jersey", "Jordan", "Kazakhstan", "Kenya", "Kiribati", "Kosovo", "Kuwait", "Kyrgyzstan", "Laos", "Latvia", "Lebanon", "Lesotho", "Liberia", "Libya", "Liechtenshein", "Lithuania", "Luxembourg", "Macao", "Macedonia", "Madagascar", "Malawi", "Malaysia", "Maldives", "Mali", "Malta", "Marshall Islands", "Martinique", "Mauritania", "Mauritius", "Mayotte", "Mexico", "Micronesia", "Moldova", "Monaco", "Mongolia", "Montenegro", "Montserrat", "Morocco", "Mozambique", "Myanmar(Burma)", "Namibia", "NATO", "Nauru", "Nepal", "Netherlands Antilles", "Netherlands", "New Caledonia", "New Zealand", "Nicaragua", "Niger", "Nigeria", "Niue", "Norfolk", "North Korea", "Northern Cyprus", "Northern Ireland", "Northern Mariana", "Norway", "OAS", "OECD", "Olimpic Movement", "Oman", "OPEC", "Pakistan", "Palau", "Palestine", "Panama", "Papua New Guinea", "Paraguay", "Peru", "Philippines", "Pitcairn", "Poland", "Portugal", "Puerto Rico", "Qatar", "Red Cross", "Reunion", "Romania", "Russian Federation", "Rwanda", "Saint Barthelemy", "Saint Helena", "Saint Lucia", "Saint Martin", "Saint Pierre and Miquelon", "Samoa", "San Marino", "Sao Tome & Principe", "Saudi Arabia", "Scotland", "Senegal", "Serbia(Yugoslavia)", "Seychelles", "Sierra Leone", "Singapore", "Sint-Maarten", "Slovakia", "Slovenia", "Solomon Islands", "Somalia", "Somaliland", "South Africa", "South Georgia and South Sandwich", "South Korea", "Southern-Sudan", "Spain", "Sri Lanka", "St Kitts & Nevis", "St Vincent & the Grenadines", "Sudan", "Suriname", "Svalbard and Jan Mayen", "Swaziland", "Sweden", "Switzerland", "Syria", "Tahiti(French Polinesia)", "Taiwan", "Tajikistan", "Tanzania", "Thailand", "Timor-Leste", "Togo", "Tokelau", "Tonga", "Trinidad & Tobago", "Tristan-da-Cunha", "Tunisia", "Turkey", "Turkmenistan", "Turks and Caicos Islands", "Tuvalu", "Uganda", "Ukraine", "UNESCO", "UNICEF", "United Arab Emirates", "United Kingdom(Great Britain)", "United Nations", "United States Minor Outlying", "United States of America(USA)", "Uruguay", "Uzbekistan", "Vanutau", "Vatican City", "Venezuela", "Viet Nam", "Virgin Islands British", "Virgin Islands US", "Wales", "Wallis and Futuna", "Western Sahara", "WHO", "WTO", "Yemen", "Zambia", "Zimbabwe" };

enum { PADDING = 10, TOP_PADDING = 20 };
enum { BUTTON_WIDTH = 100, BUTTON_HEIGHT = 30, BUTTON_MARGIN = 5 };

CIOSOptionsPanel::CIOSOptionsPanel(VPANEL parent) : BaseClass(NULL, "IOSOptionsPanel")
{
	//SetScheme("ClientScheme");

	SetParent(parent);
	m_pContent = new Panel(this, "");
	m_pPlayerNameLabel = new Label(m_pContent, "", "Player Name:");
	m_pPlayerNameText = new TextEntry(m_pContent, "");
	m_pClubNameLabel = new Label(m_pContent, "", "Club Name:");
	m_pClubNameText = new TextEntry(m_pContent, "");
	m_pCountryNameLabel = new Label(m_pContent, "", "Country Name:");
	m_pCountryNameList = new ComboBox(m_pContent, "", COUNTRY_NAMES_COUNT, false);
	m_pCountryNameList->SetOpenDirection(Menu::DOWN);

	m_pOKButton = new Button(m_pContent, "", "OK");
	m_pCancelButton = new Button(m_pContent, "", "Cancel");
	m_pSaveButton = new Button(m_pContent, "", "Apply");

	m_pCountryNameList->RemoveAll();

	for (int i = 0; i < COUNTRY_NAMES_COUNT; i++)
	{
		KeyValues *kv = new KeyValues("UserData", "index", i);
		m_pCountryNameList->AddItem(g_szCountryNames[i], kv);
		kv->deleteThis();
	}
}

CIOSOptionsPanel::~CIOSOptionsPanel()
{
}

void CIOSOptionsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	m_pScheme = pScheme;
	BaseClass::ApplySchemeSettings( pScheme );

	SetTitle("PLAYER SETTINGS", false);
	SetProportional(false);
	SetSizeable(false);
	SetBounds(0, 0, 500, 200);
	SetBgColor(Color(0, 0, 0, 255));
	SetPaintBackgroundEnabled(true);
	MoveToCenterOfScreen();

	m_pContent->SetBounds(PADDING, PADDING + TOP_PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING - TOP_PADDING);

	m_pPlayerNameLabel->SetBounds(0, 0, LABEL_WIDTH, TEXT_HEIGHT);
	m_pPlayerNameText->SetBounds(LABEL_WIDTH, 0, INPUT_WIDTH, TEXT_HEIGHT);
	//m_pNameText->SetEditable(true);
	//m_pNameText->SetEnabled(true);
	m_pPlayerNameText->AddActionSignalTarget( this );
	m_pPlayerNameText->SendNewLine(true); // with the txtEntry Type you need to set it to pass the return key as a message
	//m_pPlayerNameText->SetFgColor(Color(0, 0, 0, 255));

	m_pClubNameLabel->SetBounds(0, TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pClubNameText->SetBounds(LABEL_WIDTH, TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	//m_pClubNameText->SetFgColor(Color(0, 0, 0, 255));

	m_pCountryNameLabel->SetBounds(0, 2 * TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pCountryNameList->SetBounds(LABEL_WIDTH, 2 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	//m_pCountryNameList->GetMenu()->AddActionSignalTarget(this);
	m_pCountryNameList->GetMenu()->MakeReadyForUse();
	//m_pCountryNameList->GetMenu()->SetFgColor(Color(0, 0, 0, 255));
	//m_pCountryNameList->GetMenu()->SetBgColor(Color(255, 255, 255, 255));
	//m_pCountryNameList->SetSelectionUnfocusedBgColor(Color(255, 0, 0, 255));
	//m_pCountryNameList->SetSelectionBgColor(Color(255, 255, 0, 255));
	//m_pCountryNameList->SetFgColor(Color(0, 0, 0, 255));
	//m_pCountryNameList->SetBgColor(Color(255, 255, 255, 255));
	//m_pCountryNameList->SetDisabledBgColor(Color(255, 255, 255, 255));
	//m_pCountryNameList->SetSelectionTextColor(Color(0, 0, 0, 255));

	m_pOKButton->SetBounds(m_pContent->GetWide() - 3 * BUTTON_WIDTH - 2 * BUTTON_MARGIN, m_pContent->GetTall() - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pOKButton->SetCommand("save_and_close");
	m_pOKButton->AddActionSignalTarget(this);

	m_pCancelButton->SetBounds(m_pContent->GetWide() - 2 * BUTTON_WIDTH - BUTTON_MARGIN, m_pContent->GetTall() - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pCancelButton->SetCommand("close");
	m_pCancelButton->AddActionSignalTarget(this);

	m_pSaveButton->SetBounds(m_pContent->GetWide() - BUTTON_WIDTH, m_pContent->GetTall() - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pSaveButton->SetCommand("save_settings");
	m_pSaveButton->AddActionSignalTarget(this);
	//m_pSaveButton->SetDefaultColor(Color(0, 0, 0, 255), Color(200, 200, 200, 255));
	//m_pSaveButton->SetArmedColor(Color(50, 50, 50, 255), Color(150, 150, 150, 255));
	//m_pSaveButton->SetDepressedColor(Color(100, 100, 100, 255), Color(200, 200, 200, 255));
	//m_pSaveButton->SetCursor(dc_hand);
	//m_pSaveButton->SetFont(m_pScheme->GetFont("IOSTeamMenuNormal"));
	//m_pSaveButton->SetContentAlignment(Label::a_center);
}

void CIOSOptionsPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CIOSOptionsPanel::OnThink()
{
	BaseClass::OnThink();

	//SetTall((int)(gpGlobals->curtime * 100) % 100);
	//m_pSettingsPanel->Update();
}

void CIOSOptionsPanel::OnCommand(const char *cmd)
{
	if (!stricmp(cmd, "save_settings") || !stricmp(cmd, "save_and_close"))
	{
		char text[64];
		m_pPlayerNameText->GetText(text, sizeof(text));
		g_pCVar->FindVar("playername")->SetValue(text);
		m_pClubNameText->GetText(text, sizeof(text));
		g_pCVar->FindVar("clubname")->SetValue(text);
		m_pCountryNameList->GetText(text, sizeof(text));
		g_pCVar->FindVar("countryname")->SetValue(text);

		if (!stricmp(cmd, "save_and_close"))
			Close();
	}
	else if (!stricmp(cmd, "close"))
	{
		Close();
	}
	else
		BaseClass::OnCommand(cmd);
}

void CIOSOptionsPanel::Activate()
{
	BaseClass::Activate();

	if (Q_strlen(g_pCVar->FindVar("playername")->GetString()) == 0)
		g_pCVar->FindVar("playername")->SetValue(g_pCVar->FindVar("name")->GetString());

	m_pPlayerNameText->SetText(g_pCVar->FindVar("playername")->GetString());
	m_pCountryNameList->SetText(g_pCVar->FindVar("countryname")->GetString());
	m_pClubNameText->SetText(g_pCVar->FindVar("clubname")->GetString());
}