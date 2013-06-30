//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SPECTATORGUI_H
#define SPECTATORGUI_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui/keycode.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <igameevents.h>
#include "GameEventListener.h"

#include <game/client/iviewport.h>

class KeyValues;

namespace vgui
{
	class TextEntry;
	class Button;
	class Panel;
	class ImagePanel;
	class ComboBox;
}

class IBaseFileSystem;

//-----------------------------------------------------------------------------
// Purpose: the bottom bar panel, this is a separate panel because it
// wants mouse input and the main window doesn't
//----------------------------------------------------------------------------
class CSpectatorMenu : public vgui::Frame, public IViewPortPanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE(  CSpectatorMenu, vgui::Frame );

public:
	CSpectatorMenu( IViewPort *pViewPort );
	~CSpectatorMenu() {}

	virtual const char *GetName( void ) { return PANEL_SPECMENU; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset( void ) {}
	virtual void Update( void );
	virtual bool NeedsUpdate( void );
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );
	virtual void FireGameEvent( IGameEvent *event );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

private:
	// VGUI2 overrides
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PerformLayout();

	vgui::Panel *m_pMainPanel;

	vgui::ComboBox *m_pTargetList;
	vgui::ComboBox *m_pCamModes;
	vgui::ComboBox *m_pTVCamModes;

	vgui::Button *m_pLeftButton;
	vgui::Button *m_pRightButton;

	IViewPort *m_pViewPort;
	ButtonCode_t m_iDuckKey;
};

#endif // SPECTATORGUI_H
