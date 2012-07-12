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

#define COUNTRY_NAMES_COUNT 282

extern char g_szCountryNames[COUNTRY_NAMES_COUNT][64];

//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CSettingsMenu : public Panel
{
private:
	DECLARE_CLASS_SIMPLE( CSettingsMenu, Panel );

	MESSAGE_FUNC_PARAMS( NewLineMessage, "NewLineMessage", data );

	IScheme *m_pScheme;

public:
	CSettingsMenu(Panel *parent, const char *name);
	virtual ~CSettingsMenu();

	virtual const char *GetName( void ) { return PANEL_TEAM; }
	virtual void OnThink();
	virtual bool HasInputElements( void ) { return true; }
	virtual void PerformLayout();
	virtual void OnCommand( char const *cmd );

	float m_flNextUpdateTime;
	
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
};


#endif // TEAMMENU_H
