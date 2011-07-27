//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STATUSMENU_H
#define STATUSMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>
#include <UtlVector.h>
#include <vgui/ILocalize.h>
#include <vgui/KeyCode.h>
#include <game/client/iviewport.h>

#include "mouseoverpanelbutton.h"
namespace vgui
{
	class TextEntry;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the class menu
//-----------------------------------------------------------------------------
class CStatusBar : public vgui::Frame, public IViewPortPanel
{
	DECLARE_CLASS_SIMPLE( CStatusBar, vgui::Frame );

public:
	CStatusBar( IViewPort *pViewPort );
	virtual ~CStatusBar();

	virtual const char *GetName( void ) { return PANEL_STATUS; }
	virtual void SetData(KeyValues *data);
	virtual void Reset() {};
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return true; }
	virtual bool HasInputElements( void ) { return false; }
	virtual void ShowPanel( bool bShow );
	
	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	virtual void OnThink();

	virtual int GetTopBarHeight() { return m_pTopBar->GetTall(); }
	virtual int GetBottomBarHeight() { return m_pBottomBarBlank->GetTall(); }
	
	virtual bool ShouldShowPlayerLabel( int specmode );
	
protected:

	void SetLabelText(const char *textEntryName, const char *text);
	void SetLabelText(const char *textEntryName, wchar_t *text);
	void MoveLabelToFront(const char *textEntryName);
	void UpdateTimer();
	void SetLogoImage(const char *image);

private:	
	enum { INSET_OFFSET = 2 } ; 

	// vgui overrides
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
//	virtual void OnCommand( const char *command );

	vgui::Panel *m_pTopBar;
	vgui::Panel *m_pBottomBarBlank;

	vgui::ImagePanel *m_pBannerImage;
	vgui::Label *m_pStatusBarLabel;

	IViewPort *m_pViewPort;

	// bool m_bHelpShown;
	// bool m_bInsetVisible;
	bool m_bSpecScoreboard;


	char	m_Text[1024];
	int		m_AlphaText, m_AlphaBar;

	float	m_EndTime;
	float	m_StartTime;
	float	m_T;						//parametric
};



#endif // CLASSMENU_H
