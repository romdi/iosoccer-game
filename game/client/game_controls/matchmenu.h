#ifndef MATCHMENU_H
#define MATCHMENU_H

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

enum tab_t
{
	TAB_TEAM_JOIN = 0,
	TAB_TEAM_STATS,
	TAB_PLAYER_SETTINGS,
	TAB_COUNT
};

class CMatchMenu : public Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CMatchMenu, Frame );

	Button *m_pTabButtons[TAB_COUNT];
	Panel *m_pTabPanels[TAB_COUNT];

	int m_nActiveTab;
	float m_flNextUpdateTime;

public:
	CMatchMenu(IViewPort *pViewPort);
	virtual ~CMatchMenu();

	virtual const char *GetName( void ) { return PANEL_MATCH; }
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