//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STATSMENU_H
#define STATSMENU_H
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
#include <vgui_controls/sectionedlistpanel.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CStatsMenu : public Panel
{
private:
	//DECLARE_CLASS_SIMPLE( CTeamMenu, vgui::Frame );
	DECLARE_CLASS_SIMPLE( CStatsMenu, Panel );
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );

	IScheme *m_pScheme;

public:
	CStatsMenu(Panel *parent, const char *name);
	virtual ~CStatsMenu();

	virtual const char *GetName( void ) { return PANEL_TEAM; }
	virtual void OnThink();
	virtual bool HasInputElements( void ) { return true; }
	virtual void PerformLayout();
	virtual void OnCommand( char const *cmd );
	void Reset();
	void Reset(int team);
	void Update(int *playerIndices);

	float m_flNextUpdateTime;
	
protected:
	
	// VGUI2 overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	SectionedListPanel *m_pPlayerStats[2];
};


#endif // TEAMMENU_H
