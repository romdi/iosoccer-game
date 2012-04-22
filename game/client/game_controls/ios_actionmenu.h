#ifndef ACTIONMENU_H
#define ACTIONMENU_H

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_bitmapbutton.h>
#include <vgui_controls/ImagePanel.h>

#include <game/client/iviewport.h>

#include <vgui/KeyCode.h>
#include <UtlVector.h>

#include <vgui_controls/EditablePanel.h>

#include "teammenu.h"

using namespace vgui;

#define MAX_TEAM_PLAYERS 10

class CActionMenu : public Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CActionMenu, Frame );

	Button *m_pPlayerButtons[MAX_TEAM_PLAYERS];

	float m_flNextUpdateTime;
	IScheme *m_pScheme;

public:
	CActionMenu(IViewPort *pViewPort);
	virtual ~CActionMenu();

	virtual const char *GetName( void ) { return PANEL_ACTION; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate( void );
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );
	
	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	virtual void OnThink() {};
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void PerformLayout();
	virtual void OnCommand( char const *cmd );
};

#endif