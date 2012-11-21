//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FORMATIONMENU_H
#define FORMATIONMENU_H
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
class CFormationMenu : public Panel
{
private:
	//DECLARE_CLASS_SIMPLE( CTeamMenu, vgui::Frame );
	DECLARE_CLASS_SIMPLE( CFormationMenu, Panel );
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );

	IScheme *m_pScheme;

public:
	CFormationMenu(Panel *parent, const char *name);
	virtual ~CFormationMenu();

	virtual const char *GetName( void ) { return PANEL_FORMATION; }
	virtual void OnThink();
	virtual bool HasInputElements( void ) { return true; }
	virtual void PerformLayout();
	virtual void OnCommand( char const *cmd );
	void Reset();
	void Update();

	float m_flNextUpdateTime;
	
protected:
	
	// VGUI2 overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	MESSAGE_FUNC_PTR( OnCursorEntered, "OnCursorEntered", panel );
	MESSAGE_FUNC_PTR( OnCursorExited, "OnCursorExited", panel );

	Panel		*m_pFormations[2];
	CBitmapButton *m_pFormationButtons[2][11];
	Label		*m_pToolTips[2][11];
};


#endif // TEAMMENU_H
