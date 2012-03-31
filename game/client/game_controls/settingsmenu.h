//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SETTINGSMENU_H
#define SETTINGSMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_bitmapbutton.h>
#include <vgui_controls/ImagePanel.h>

#include <game/client/iviewport.h>

#include <vgui/KeyCode.h>
#include <UtlVector.h>

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ComboBox.h>

using namespace vgui;

#define COUNTRY_NAMES_COUNT 283

char g_szCountryNames[COUNTRY_NAMES_COUNT][64] = { "Afghanistan", "African Union", "Aland", "Albania", "Alderney", "Algeria", "American Samoa", "Andorra", "Angola", "Anguilla", "Antarctica", "Antigua & Barbuda", "Arab League", "Argentina", "Armenia", "Aruba", "ASEAN", "Australia", "Austria", "Azerbaijan", "Bahamas", "Bahrain", "Bangladesh", "Barbados", "Basque Country", "Belarus", "Belgium", "Belize", "Benin", "Bermuda", "Bhutan", "Bolivia", "Bosnia & Herzegovina", "Botswana", "Bouvet", "Brazil", "British Indian Ocean Territory", "Brunei", "Bulgaria", "Burkina Faso", "Burundi", "Cambodja", "Cameroon", "Canada", "Cape Verde", "CARICOM", "Catalonia", "Cayman Islands", "Central African Republic", "Chad", "Chile", "China", "Christmas", "CIS", "Cocos (Keeling)", "Colombia", "Commonwealth", "Comoros", "Congo Brazzaville", "Congo Kinshasa(Zaire)", "Cook Islands", "Costa Rica", "Cote d'Ivoire", "Croatia", "Cuba", "Curacao", "Cyprus", "Czech Republic", "Denmark", "Djibouti", "Dominica", "Dominican Republic", "Ecuador", "Egypt", "El Salvador", "England", "Equatorial Guinea", "Eritrea", "Estonia", "Ethiopia", "European Union", "Falkland (Malvinas)", "FAO", "Faroes", "Fiji", "Finland", "France", "French Guiana", "French Southern and Antarctic Lands", "Gabon", "Galicia", "Gambia", "Georgia", "Germany", "Ghana", "Gibraltar", "Greece", "Greenland", "Grenada", "Guadeloupe", "Guam", "Guatemala", "Guernsey", "Guinea", "Guinea Bissau", "Guyana", "Haiti", "Heard Island and McDonald", "Honduras", "Hong Kong", "Hungary", "IAEA", "Iceland", "IHO", "India", "Indonezia", "Iran", "Iraq", "Ireland", "Islamic Conference", "Isle of Man", "Israel", "Italy", "Jamaica", "Japan", "Jersey", "Jordan", "Kazakhstan", "Kenya", "Kiribati", "Kosovo", "Kuwait", "Kyrgyzstan", "Laos", "Latvia", "Lebanon", "Lesotho", "Liberia", "Libya", "Liechtenshein", "Lithuania", "Luxembourg", "Macao", "Macedonia", "Madagascar", "Malawi", "Malaysia", "Maldives", "Mali", "Malta", "Marshall Islands", "Martinique", "Mauritania", "Mauritius", "Mayotte", "Mexico", "Micronesia", "Moldova", "Monaco", "Mongolia", "Montenegro", "Montserrat", "Morocco", "Mozambique", "Myanmar(Burma)", "Namibia", "NATO", "Nauru", "Nepal", "Netherlands", "Netherlands Antilles", "New Caledonia", "New Zealand", "Nicaragua", "Niger", "Nigeria", "Niue", "Norfolk", "North Korea", "Northern Cyprus", "Northern Ireland", "Northern Mariana", "Norway", "OAS", "OECD", "Olimpic Movement", "Oman", "OPEC", "Pakistan", "Palau", "Palestine", "Panama", "Papua New Guinea", "Paraguay", "Peru", "Philippines", "Pitcairn", "Poland", "Portugal", "Puerto Rico", "Qatar", "Red Cross", "Reunion", "Romania", "Russian Federation", "Rwanda", "Saint Barthelemy", "Saint Helena", "Saint Lucia", "Saint Martin", "Saint Pierre and Miquelon", "Samoa", "San Marino", "Sao Tome & Principe", "Saudi Arabia", "Scotland", "Senegal", "Serbia(Yugoslavia)", "Seychelles", "Sierra Leone", "Singapore", "Sint Maarten", "Slovakia", "Slovenia", "Solomon Islands", "Somalia", "Somaliland", "South Africa", "South Georgia and South Sandwich", "South Korea", "Southern Sudan", "Spain", "Sri Lanka", "St Kitts & Nevis", "St Vincent & the Grenadines", "Sudan", "Suriname", "Svalbard and Jan Mayen", "Swaziland", "Sweden", "Switzerland", "Syria", "Tahiti(French Polinesia)", "Taiwan", "Tajikistan", "Tanzania", "Thailand", "Timor Leste", "Togo", "Tokelau", "Tonga", "Trinidad & Tobago", "Tristan da Cunha", "Tunisia", "Turkey", "Turkmenistan", "Turks and Caicos Islands", "Tuvalu", "Uganda", "Ukraine", "UNESCO", "UNICEF", "United Arab Emirates", "United Kingdom(Great Britain)", "United Nations", "United States Minor Outlying", "United States of America(USA)", "Uruguay", "Uzbekistan", "Vanutau", "Vatican City", "Venezuela", "Viet Nam", "Virgin Islands British", "Virgin Islands US", "Wales", "Wallis and Futuna", "Western Sahara", "WHO", "WTO", "Yemen", "Zambia", "Zimbabwe" };

//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CSettingsMenu : public Panel
{
private:
	DECLARE_CLASS_SIMPLE( CSettingsMenu, Panel );

	MESSAGE_FUNC_PARAMS( NewLineMessage, "NewLineMessage", data );

public:
	CSettingsMenu(Panel *parent, const char *name);
	virtual ~CSettingsMenu();

	virtual const char *GetName( void ) { return PANEL_TEAM; }
	virtual void OnThink();
	virtual bool HasInputElements( void ) { return true; }
	
	virtual void OnCommand( char const *cmd );
	
protected:
	
	// VGUI2 overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	Label *m_pPlayerNameLabel;
	TextEntry *m_pPlayerNameText;
	Label *m_pClubNameLabel;
	TextEntry *m_pClubNameText;
	Label *m_pCountryNameLabel;
	ComboBox *m_pCountryNameList;
	Button *m_pSaveButton;
	float m_flNextUpdateTime;
};


#endif // TEAMMENU_H
